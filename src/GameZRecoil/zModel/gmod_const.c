#include "recoil/Mfc42Abi.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/Time/time.h"
#include "zdi.h"

#include "Battlesport/player.h"
#include "GameZRecoil/include/zDi.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"

#include <malloc.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Reimplements data 0x576200: Symbol.
 * Authored zModel display-instance pool global.
 * Purpose: record the configured display-instance pool capacity.
 */
int g_zModel_DiPoolCapacity = 0;
/**
 * Reimplements data 0x576204: Symbol.
 * Authored zModel display-instance pool global.
 * Purpose: point at the allocated display-instance pool storage.
 */
zDiPartial *g_zModel_DiPoolBase = 0;
/**
 * Reimplements data 0x576208: Symbol.
 * Authored zModel display-instance pool global.
 * Purpose: count display-instance pool entries currently allocated.
 */
int g_zModel_DiPoolInUseCount = 0;
/**
 * Reimplements data 0x57620c: Symbol.
 * Authored zModel display-instance pool global.
 * Purpose: hold the head index of the display-instance free list.
 */
int g_zModel_DiPoolFreeHeadIndex = 0;

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

    /**
     * Original static helper observed in zDi polygon construction paths
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: normalize UV coordinates to a local tile origin.
     */
    void NormalizeUvTileOrigin(
        zClipUV *uvPairs,
        int uvCount
    ) {
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

        const float baseU = (float)(floor(minU));
        const float baseV = (float)(floor(minV));
        for (int i_107 = 0; i_107 < uvCount; ++i_107) {
            uvPairs[i_107].u -= baseU;
            uvPairs[i_107].v -= baseV;
        }
    }
}

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
/**
 * Reimplements data 0x4e1378: g_zModel_ConstVertexWarnThreshold.
 * Purpose: store the vertex and normal count warning threshold.
 */
double g_zModel_ConstVertexWarnThreshold = 921.6;
/**
 * Reimplements data 0x4e1380: g_zModel_NormalMergeEpsilon.
 * Purpose: store the normal merge epsilon used when adding model normals.
 */
double g_zModel_NormalMergeEpsilon = 0.0001;
/**
 * Reimplements data 0x4e1388: g_zModel_CoplanarTolerance.
 * Purpose: store the coplanar polygon tolerance.
 */
double g_zModel_CoplanarTolerance = 0.001;
/**
 * Reimplements data 0x4e1390: g_zModel_ColinearTolerance.
 * Purpose: store the colinear polygon tolerance.
 */
double g_zModel_ColinearTolerance = 0.001;
float g_zModel_UvQuantizeBias = -0.001953125f;
float g_zModel_UvQuantizeScale = 256.0f;
float g_zModel_UvQuantizeInvScale = 0.00390625f;

namespace zModel_Const {



}

namespace zModel_DiPool {




}

/*
 * Address-backed gmod_const.c function contribution in natural retail order.
 */
namespace {
    const char *kClsDiSourceFile = "D:\\Proj\\GameZRecoil\\zClass\\cls_di.c";
    const int kNodeClassCamera = 1;
    const int kNodeClassObject3D = 5;
    const int kNodeClassLod = 6;
    const int kNodeClassSequence = 7;
    const int kNodeClassAnimate = 8;
    const int kNodeClassLight = 9;
    const int kNodeClassSound = 10;
    const int kNodeFlagEnabledForPick = 0x04;
    const int kNodeFlagRaycastable = 0x10;
    const int kNodeFlagPointCandidate = 0x20;
    const int kNodeFlagFilterRegionCandidate = 0x40;
    const int kNodeFlagCachedBoundsValid = 0x100;
    const int kNodeFlagRequiresLineOfSight = 1 << 22;
    const int kNodeFlagUseLocalMatrixMode3 = 0x80000;
    const int kNodeFlagClearDuringPick = 0x02000000;
    const int kObjectFlagTransformDirty = 0x01;
    const int kObjectFlagNoPickMatrixPush = 0x08;
    const int kObjectFlagUseCachedWorldMatrix = 0x20;
    const int kMaxPickCandidates = 0x20;
    const double kPickEdgeInsideEpsilon = -0.0001;
    const unsigned short kPickFaceBatchDamageMaskUvFlag = 0x0100;
    const unsigned short kPickFaceTexturedDamageMaskFlag = 0x0200;

    /**
     * Original static helper observed in cls_di polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: absolute float component for dominant-axis selection.
     */
    float AbsFloat(float value) {
        return value < 0.0f ? -value : value;
    }

    /**
     * Original static helper observed in cls_di segment-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: read float sign bits for side-test rejection.
     */
    unsigned int FloatBits(float value) {
        union {
            float f;
            unsigned int u;
        } bits = {value};
        return bits.u;
    }

    /**
     * Original static helper observed in cls_di segment-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: three-component dot product for plane side tests.
     */
    float Dot3(
        const zVec3 *a,
        const zVec3 *b
    ) {
        return a->x * b->x + a->y * b->y + a->z * b->z;
    }

    /**
     * Original static helper observed in cls_di segment-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build the segment endpoint delta used by plane tests.
     */
    zVec3 Delta3(
        const zVec3 *a,
        const zVec3 *b
    ) {
        zVec3 result = {a->x - b->x, a->y - b->y, a->z - b->z};
        return result;
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: projected 2D edge cross product for winding tests.
     */
    double ProjectedEdgeCross(
        const zVec3 *edgeStart,
        const zVec3 *edgeEnd,
        const zVec3 *point,
        int axis
    ) {
        switch (axis) {
        case 0:
            return (edgeEnd->y - edgeStart->y) * (point->z - edgeStart->z) -
                   (edgeEnd->z - edgeStart->z) * (point->y - edgeStart->y);
        case 1:
            return (edgeEnd->x - edgeStart->x) * (point->z - edgeStart->z) -
                   (edgeEnd->z - edgeStart->z) * (point->x - edgeStart->x);
        default:
            return (edgeEnd->x - edgeStart->x) * (point->y - edgeStart->y) -
                   (edgeEnd->y - edgeStart->y) * (point->x - edgeStart->x);
        }
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: choose the dominant normal component for 2D projection.
     */
    int DominantAxis(const zVec3 *normal) {
        int axis = 0;
        float maxAbs = AbsFloat(normal->x);
        const float absY = AbsFloat(normal->y);
        if (absY > maxAbs) {
            maxAbs = absY;
            axis = 1;
        }
        const float absZ = AbsFloat(normal->z);
        if (absZ > maxAbs) {
            axis = 2;
        }
        return axis;
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: read the normal component selected by DominantAxis.
     */
    float DominantAxisComponent(
        const zVec3 *normal,
        int axis
    ) {
        if (axis == 0) {
            return normal->x;
        }
        if (axis == 1) {
            return normal->y;
        }
        return normal->z;
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: orient projected polygon winding against the dominant axis.
     */
    int ProjectedWindingSign(
        const zVec3 *normal,
        int axis
    ) {
        const int componentIsNegative = DominantAxisComponent(
            normal,
            axis
        ) < 0.0f ? 1 : 0;
        if (axis == 1) {
            return componentIsNegative != 0 ? 1 : -1;
        }
        return componentIsNegative != 0 ? -1 : 1;
    }

    /**
     * Original static helper observed in cls_di projected-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: test whether the projected segment-plane hit lies inside the polygon.
     */
    bool PointInProjectedPolygon(
        const zVec3 *polygonVertices,
        int vertexCount,
        const zVec3 *point,
        const zVec3 *normal
    ) {
        const int axis = DominantAxis(normal);
        const int windingSign = ProjectedWindingSign(
            normal,
            axis
        );

        {
            for (int edgeIndex = vertexCount - 1; edgeIndex >= 0; --edgeIndex) {
                const zVec3 *edgeStart = &polygonVertices[edgeIndex];
                const zVec3 *edgeEnd = &polygonVertices[(edgeIndex + 1) % vertexCount];
                const double edgeValue =
                    (double)(windingSign)*ProjectedEdgeCross(
                        edgeStart,
                        edgeEnd,
                        point,
                        axis
                    );
                if (edgeValue <= kPickEdgeInsideEpsilon) {
                    return false;
                }
            }
        }

        return true;
    }

    /**
     * Original static helper observed in cls_di segment-polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build a pick candidate from one segment-plane intersection and projected polygon test.
     */
    bool BuildPickCandidateForSegmentVsPolygonCore(
        zClassDiPickCandidateEntry * candidate,
        const zVec3 *segmentStart,
        const zVec3 *segmentEnd,
        const zVec3 *polygonVertices,
        int vertexCount,
        int cullBackface,
        int *outDominantAxis
    ) {
        zMath_Vec3_TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &candidate->surfaceNormal
        );

        const zVec3 endDelta = Delta3(
            segmentEnd,
            &polygonVertices[0]
        );
        const float endSide = Dot3(
            &endDelta,
            &candidate->surfaceNormal
        );
        if (cullBackface == 0 && endSide >= 0.0f) {
            return false;
        }

        const zVec3 startDelta = Delta3(
            segmentStart,
            &polygonVertices[0]
        );
        const float startSide = Dot3(
            &startDelta,
            &candidate->surfaceNormal
        );
        if (((FloatBits(startSide) ^ FloatBits(endSide)) & 0x80000000u) == 0) {
            return false;
        }

        const float t = startSide / (startSide - endSide);
        const zVec3 segmentDelta = Delta3(
            segmentEnd,
            segmentStart
        );
        candidate->hitPos.x = segmentStart->x + t * segmentDelta.x;
        candidate->hitPos.y = segmentStart->y + t * segmentDelta.y;
        candidate->hitPos.z = segmentStart->z + t * segmentDelta.z;

        const int dominantAxis = DominantAxis(&candidate->surfaceNormal);
        if (outDominantAxis != 0) {
            *outDominantAxis = dominantAxis;
        }

        return PointInProjectedPolygon(
            polygonVertices,
            vertexCount,
            &candidate->hitPos,
            &candidate->surfaceNormal
        );
    }

    /**
     * Original static helper observed in cls_di segment-batch polygon callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: compute a batched segment-plane hit point before polygon inclusion tests.
     */
    bool BuildBatchSegmentPlaneHit(
        zClassDiPickCandidateEntry * candidate,
        const zClass_DiSegmentEndpoints *segment,
        const zVec3 *polygonVertices,
        const zVec3 *normal,
        int cullBackface
    ) {
        const zVec3 endDelta = Delta3(
            &segment->end,
            &polygonVertices[0]
        );
        const float endSide = Dot3(
            &endDelta,
            normal
        );
        if (cullBackface == 0 && endSide >= 0.0f) {
            return false;
        }

        const zVec3 startDelta = Delta3(
            &segment->start,
            &polygonVertices[0]
        );
        const float startSide = Dot3(
            &startDelta,
            normal
        );
        if (((FloatBits(startSide) ^ FloatBits(endSide)) & 0x80000000u) == 0) {
            return false;
        }

        const float t = startSide / (startSide - endSide);
        const zVec3 segmentDelta = Delta3(
            &segment->end,
            &segment->start
        );
        candidate->hitPos.x = segment->start.x + t * segmentDelta.x;
        candidate->hitPos.y = segment->start.y + t * segmentDelta.y;
        candidate->hitPos.z = segment->start.z + t * segmentDelta.z;
        return true;
    }

    /**
     * Original static helper observed in cls_di segment-batch polygon callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append the current polygon hit to the player-probe candidate buffer.
     */
    void AppendBatchPolygonCandidate(
        zClass_NodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * buffer,
        const zVec3 *normal,
        const zModel_PickFaceEntry *faceEntry
    ) {
        if (buffer->candidateCount >= kMaxPickCandidates) {
            return;
        }

        zClassDiPickCandidateEntry *entry = &buffer->entries[buffer->candidateCount];
        entry->surfaceNormal = *normal;
        entry->node = candidateOwner;
        entry->scenePayload = faceEntry->scenePayload;
        ++buffer->candidateCount;
    }

    /**
     * Original static helper observed in cls_di damage-mask polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: solve hit UV coordinates in the dominant projected plane.
     */
    void SolvePickCandidateUvForProjectedPlane(
        const zClassDiPickCandidateEntry *candidate,
        const zVec3 *polygonVertices,
        const zModel_PickFaceUvData *faceUvData,
        zVec2 *outUv,
        int dominantAxis
    ) {
        float uGrad0;
        float uGrad1;
        float vGrad0;
        float vGrad1;

        if (dominantAxis == 0) {
            zMath_SolveLinearGradient2D(
                &uGrad0,
                &uGrad1,
                polygonVertices[0].y,
                polygonVertices[0].z,
                polygonVertices[1].y,
                polygonVertices[1].z,
                polygonVertices[2].y,
                polygonVertices[2].z,
                faceUvData->uvs[0].x,
                faceUvData->uvs[1].x,
                faceUvData->uvs[2].x
            );
            zMath_SolveLinearGradient2D(
                &vGrad0,
                &vGrad1,
                polygonVertices[0].y,
                polygonVertices[0].z,
                polygonVertices[1].y,
                polygonVertices[1].z,
                polygonVertices[2].y,
                polygonVertices[2].z,
                faceUvData->uvs[0].y,
                faceUvData->uvs[1].y,
                faceUvData->uvs[2].y
            );

            outUv->x = (candidate->hitPos.y - polygonVertices[0].y) * uGrad0 +
                       (candidate->hitPos.z - polygonVertices[0].z) * uGrad1 + faceUvData->uvs[0].x;
            outUv->y = (candidate->hitPos.y - polygonVertices[0].y) * vGrad0 +
                       (candidate->hitPos.z - polygonVertices[0].z) * vGrad1 + faceUvData->uvs[0].y;
            return;
        }

        if (dominantAxis == 1) {
            zMath_SolveLinearGradient2D(
                &uGrad0,
                &uGrad1,
                polygonVertices[0].x,
                polygonVertices[0].z,
                polygonVertices[1].x,
                polygonVertices[1].z,
                polygonVertices[2].x,
                polygonVertices[2].z,
                faceUvData->uvs[0].x,
                faceUvData->uvs[1].x,
                faceUvData->uvs[2].x
            );
            zMath_SolveLinearGradient2D(
                &vGrad0,
                &vGrad1,
                polygonVertices[0].x,
                polygonVertices[0].z,
                polygonVertices[1].x,
                polygonVertices[1].z,
                polygonVertices[2].x,
                polygonVertices[2].z,
                faceUvData->uvs[0].y,
                faceUvData->uvs[1].y,
                faceUvData->uvs[2].y
            );

            outUv->x = (candidate->hitPos.z - polygonVertices[0].z) * uGrad1 +
                       (candidate->hitPos.x - polygonVertices[0].x) * uGrad0 + faceUvData->uvs[0].x;
            outUv->y = (candidate->hitPos.z - polygonVertices[0].z) * vGrad1 +
                       (candidate->hitPos.x - polygonVertices[0].x) * vGrad0 + faceUvData->uvs[0].y;
            return;
        }

        zMath_SolveLinearGradient2D(
            &uGrad0,
            &uGrad1,
            polygonVertices[0].x,
            polygonVertices[0].y,
            polygonVertices[1].x,
            polygonVertices[1].y,
            polygonVertices[2].x,
            polygonVertices[2].y,
            faceUvData->uvs[0].x,
            faceUvData->uvs[1].x,
            faceUvData->uvs[2].x
        );
        zMath_SolveLinearGradient2D(
            &vGrad0,
            &vGrad1,
            polygonVertices[0].x,
            polygonVertices[0].y,
            polygonVertices[1].x,
            polygonVertices[1].y,
            polygonVertices[2].x,
            polygonVertices[2].y,
            faceUvData->uvs[0].y,
            faceUvData->uvs[1].y,
            faceUvData->uvs[2].y
        );

        outUv->x = (candidate->hitPos.y - polygonVertices[0].y) * uGrad1 +
                   (candidate->hitPos.x - polygonVertices[0].x) * uGrad0 + faceUvData->uvs[0].x;
        outUv->y = (candidate->hitPos.y - polygonVertices[0].y) * vGrad1 +
                   (candidate->hitPos.x - polygonVertices[0].x) * vGrad0 + faceUvData->uvs[0].y;
    }

    /**
     * Original static helper; no standalone retail function exists. Observed
     * in address-backed transformed pick callers including 0x484e00,
     * 0x4857f0, and 0x485d10 through the active matrix slot access pattern.
     * Purpose: return the current model matrix used by cls_di point and normal
     * transforms.
     */
    const zMat4x3 *CurrentMatrix() {
        return (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    }

    /**
     * Original static helper observed in cls_di transformed face-pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: transform a world-space query point into the active model matrix space.
     */
    zVec3 TransformWorldPointToModel(const zVec3 *point) {
        const zMat4x3 *matrix = CurrentMatrix();
        const float x = point->x - matrix->posX;
        const float y = point->y - matrix->posY;
        const float z = point->z - matrix->posZ;

        zVec3 result = {x * matrix->xx + y * matrix->xy + z * matrix->xz,
            x * matrix->yx + y * matrix->yy + z * matrix->yz,
            x * matrix->zx + y * matrix->zy + z * matrix->zz};
        return result;
    }

    /**
     * Original static helper observed in cls_di transformed face-pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: transform a model-space hit point into world space.
     */
    zVec3 TransformModelPointToWorld(const zVec3 *point) {
        const zMat4x3 *matrix = CurrentMatrix();

        zVec3 result = {point->x * matrix->xx + point->y * matrix->yx + point->z * matrix->zx +
                            matrix->posX,
            point->x * matrix->xy + point->y * matrix->yy + point->z * matrix->zy + matrix->posY,
            point->x * matrix->xz + point->y * matrix->yz + point->z * matrix->zz + matrix->posZ};
        return result;
    }

    /**
     * Original static helper observed in cls_di transformed face-pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: rotate a model-space surface normal into world space.
     */
    zVec3 TransformModelVectorToWorld(const zVec3 *vec) {
        const zMat4x3 *matrix = CurrentMatrix();

        zVec3 result = {vec->x * matrix->xx + vec->y * matrix->yx + vec->z * matrix->zx,
            vec->x * matrix->xy + vec->y * matrix->yy + vec->z * matrix->zy,
            vec->x * matrix->xz + vec->y * matrix->yz + vec->z * matrix->zz};
        return result;
    }

    /**
     * Original static helper observed in cls_di mesh and polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: gather indexed face vertices into the four-entry DI scratch buffer.
     */
    void CopyFaceVerticesToScratch(
        const zVec3 *vertices,
        const int *vertexIndices,
        unsigned int vertexCount
    ) {
        for (unsigned int i = 0; i < vertexCount; ++i) {
            g_zClass_DiFaceVertexScratch4[i] = vertices[vertexIndices[i]];
        }
    }

    /**
     * Original static helper; no standalone retail function exists. Observed
     * in address-backed segment and point pick callers including 0x445650,
     * 0x445b20, and 0x445c20 as the typed mesh face payload access.
     * Purpose: view the node DI payload as polygon/mesh pick face data.
     */
    zModel_PickFaceData *NodePickFaceData(zClass_NodePartial * node) {
        return (zModel_PickFaceData *)((unsigned int)(node->userDataOrDiRef));
    }

    /**
     * Original static helper observed in cls_di pick traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append the current node to the active pick-candidate cursor.
     */
    void AppendCurrentCandidateNode(zClass_NodePartial * node) {
        g_DiPickCandidateCursor->node = node;
        ++g_DiPickCandidateCursor;
        ++g_DiPickCandidateBuffer->candidateCount;
    }

    /**
     * Original static helper observed in cls_di pick traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: test whether traversal should stop after the first accepted candidate.
     */
    bool BreakOnFirstCandidateHit() {
        return g_cls_di_BreakOnFirstCandidate != 0 && g_DiPickCandidateBuffer->candidateCount > 0;
    }

    /**
     * Original static helper observed in cls_di pick traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: convert the active candidate count into the original no-hit return value.
     */
    int NoCandidatesReturn() {
        return g_DiPickCandidateBuffer->candidateCount <= 0 ? 1 : 0;
    }

    /**
     * Original static helper observed in cls_di segment bounds callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: choose the smaller floating-point segment bound.
     */
    float MinFloat(
        float a,
        float b
    ) {
        return a < b ? a : b;
    }

    /**
     * Original static helper observed in cls_di segment bounds callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: choose the larger floating-point segment bound.
     */
    float MaxFloat(
        float a,
        float b
    ) {
        return a > b ? a : b;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: expand a bounding box into the local eight-corner scratch order.
     */
    void CopyBBoxToCornersLocal(
        const zBBox3f *bbox,
        zBBoxCorners *outCorners
    ) {
        const float minX = bbox->minX;
        const float minY = bbox->minY;
        const float minZ = bbox->minZ;
        const float maxX = bbox->maxX;
        const float maxY = bbox->maxY;
        const float maxZ = bbox->maxZ;

        float *values = outCorners->values;
        values[0] = minX;
        values[1] = minY;
        values[2] = maxZ;
        values[3] = maxX;
        values[4] = minY;
        values[5] = maxZ;
        values[6] = maxX;
        values[7] = minY;
        values[8] = minZ;
        values[9] = minX;
        values[10] = minY;
        values[11] = minZ;
        values[12] = minX;
        values[13] = maxY;
        values[14] = maxZ;
        values[15] = maxX;
        values[16] = maxY;
        values[17] = maxZ;
        values[18] = maxX;
        values[19] = maxY;
        values[20] = minZ;
        values[21] = minX;
        values[22] = maxY;
        values[23] = minZ;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: apply the optional node-name prefix filter for region hits.
     */
    int FilterRegionNodeNameAllowed(zClass_NodePartial * node) {
        const char *prefix = g_zClass_cls_di_FilterRegions_NodeNamePrefix;
        if (prefix == 0) {
            return 1;
        }

        return strncmp(
            node->name,
            prefix,
            strlen(prefix)
        ) == 0 ? 1 : 0;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: compute squared clearance outside the active filter sphere.
     */
    float FilterRegionClearanceDistanceSq(
        const zVec3 *boundsCenter,
        float boundsRadius
    ) {
        if (g_zClass_cls_di_FilterRegions_EnableClearanceCheck == 0) {
            return 0.0f;
        }

        float clearance =
            zMath::Vec3DeltaLength(
                g_zClass_cls_di_FilterRegions_Center,
                boundsCenter
            ) -
            boundsRadius;
        if (clearance < 0.0f) {
            return 0.0f;
        }

        return clearance * clearance;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: reject nodes whose bounds center is hidden by the active world raycast.
     */
    int FilterRegionLineOfSightBlocked(
        zClass_NodePartial * node,
        const zVec3 *boundsCenter
    ) {
        zClass_NodePartial *world = g_zClass_cls_di_FilterRegions_LineOfSightWorld;
        if (world == 0 || (node->flags & kNodeFlagRequiresLineOfSight) == 0) {
            return 0;
        }

        PlayerProbeSampleCandidateBuffer rayData = {0};
        zClass_cls_di::SetBreakOnFirstCandidate(1);
        zClass_cls_di::SetStopAfterFirstHit(0x40000);
        zClass_Class::gwNodeSetRaycastable(
            node,
            0
        );
        zVec3 *center = g_zClass_cls_di_FilterRegions_Center;
        const int result = zClass_cls_di::RaycastFindClosest(
            world,
            &rayData,
            center->x,
            center->y,
            center->z,
            boundsCenter->x,
            boundsCenter->y,
            boundsCenter->z
        );
        zClass_Class::gwNodeSetRaycastable(
            node,
            1
        );
        zClass_cls_di::SetBreakOnFirstCandidate(0);

        return result == 0 && rayData.candidateCount != 0 ? 1 : 0;
    }

    /**
     * Original static helper observed in cls_di filter-region callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append one filter-region hit entry to the active raycast hit list.
     */
    void AppendFilterRegionHit(
        zClass_NodePartial * node,
        const zVec3 *hitPos,
        float distanceSq
    ) {
        OptCatalogRaycastHitList *hitList = g_zClass_cls_di_FilterRegions_OutHitList;
        OptCatalogRaycastHitEntry *entry = &hitList->hits[hitList->hitCount];
        entry->hitNode = node;
        entry->pos = *hitPos;
        entry->surfaceRef = 0;
        entry->distance = distanceSq;
        ++hitList->hitCount;
    }

    /**
     * Original static helper observed in cls_di segment-grid traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: choose the signed grid step direction for a ray delta.
     */
    int RayGridStep(float delta) {
        if (delta > 0.0f) {
            return 1;
        }
        if (delta < 0.0f) {
            return -1;
        }
        return 0;
    }

    /**
     * Original static helper observed in cls_di segment child traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append the node when its pick-face data produces a segment hit.
     */
    void AppendNodeFaceCandidateIfHit(zClass_NodePartial * node) {
        zModel_PickFaceData *faceData = NodePickFaceData(node);
        if (faceData != 0 && zClass_cls_di::AppendPickCandidatesForFace(
                                 faceData,
                                 g_DiPickCandidateCursor,
                                 &g_DiPickQueryPoint,
                                 &g_DiSegmentEnd
                             ) != 0) {
            AppendCurrentCandidateNode(node);
        }
    }

    /**
     * Original static helper observed in cls_di segment-grid traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: shift the active ray packet into or out of a world-area cell.
     */
    void OffsetActiveRayPacket(
        float offsetX,
        float offsetZ
    ) {
        g_DiPickQueryPoint.x += offsetX;
        g_DiPickQueryPoint.z += offsetZ;
        g_DiSegmentEnd.x += offsetX;
        g_DiSegmentEnd.z += offsetZ;
        g_DiSegmentMinX += offsetX;
        g_DiSegmentMaxX += offsetX;
        g_DiSegmentMinZ += offsetZ;
        g_DiSegmentMaxZ += offsetZ;
    }

    /**
     * Original static helper observed in cls_di segment-grid traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: restore candidate hit positions from local cell space to world space.
     */
    void OffsetCandidatesFromCell(
        PlayerProbeSampleCandidateBuffer * rayData,
        int firstCandidate,
        float offsetX,
        float offsetZ
    ) {
        for (int i = firstCandidate; i < rayData->candidateCount; ++i) {
            rayData->entries[i].hitPos.x -= offsetX;
            rayData->entries[i].hitPos.z -= offsetZ;
        }
    }

    /**
     * Original static helper observed in cls_di segment-grid traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: scan one world-area cell for raycastable segment children.
     */
    void ProcessWorldAreaPickCell(
        zWorldAreaPartial * area,
        int nodeCountHint
    ) {
        for (int i = 0; i < area->childCount; ++i) {
            zClass_NodePartial *node = area->childList[i];
            const int flags = node->flags;
            if ((flags & kNodeFlagEnabledForPick) != 0 && (flags & kNodeFlagRaycastable) != 0) {
                zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(
                    node,
                    nodeCountHint
                );
            }

            if (BreakOnFirstCandidateHit()) {
                break;
            }
        }
    }

    /**
     * Original static helper observed in cls_di segment child traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: recurse over list-B children with optional enabled/raycastable filtering.
     */
    void RecurseListBChildren(
        zClass_NodePartial * node,
        bool requireEnabledRaycastFlags
    ) {
        {
            for (int childIndex = 0; childIndex < node->listCountB; ++childIndex) {
                zClass_NodePartial *child = node->listB[childIndex];
                if (!requireEnabledRaycastFlags ||
                    ((child->flags & kNodeFlagEnabledForPick) != 0 &&
                        (child->flags & kNodeFlagRaycastable) != 0)) {
                    zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(
                        child,
                        node->listCountB
                    );
                }

                if (BreakOnFirstCandidateHit()) {
                    break;
                }
            }
        }
    }

    /**
     * Original static helper observed in cls_di bbox pick/filter callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: compute min/max extents from the eight transformed bbox corners.
     */
    void ComputeBBoxExtents(
        const zBBoxCorners *corners,
        float *outMinX,
        float *outMaxX,
        float *outMinY,
        float *outMaxY,
        float *outMinZ,
        float *outMaxZ
    ) {
        const float *values = corners->values;
        *outMinX = values[0];
        *outMaxX = values[0];
        *outMinY = values[1];
        *outMaxY = values[1];
        *outMinZ = values[2];
        *outMaxZ = values[2];

        for (int i = 1; i < 8; ++i) {
            const float *corner = &values[i * 3];
            if (corner[0] < *outMinX) {
                *outMinX = corner[0];
            } else if (corner[0] > *outMaxX) {
                *outMaxX = corner[0];
            }
            if (corner[1] < *outMinY) {
                *outMinY = corner[1];
            } else if (corner[1] > *outMaxY) {
                *outMaxY = corner[1];
            }
            if (corner[2] < *outMinZ) {
                *outMinZ = corner[2];
            } else if (corner[2] > *outMaxZ) {
                *outMaxZ = corner[2];
            }
        }
    }

    /**
     * Original static helper observed in cls_di bbox face-test callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: copy one bbox corner into the DI four-vertex scratch polygon.
     */
    void CopyBBoxCornerToScratch(
        const zBBoxCorners *bboxCorners,
        int sourceCorner,
        int scratchCorner
    ) {
        const float *src = &bboxCorners->values[sourceCorner * 3];
        zVec3 *dst = &g_zClass_DiFaceVertexScratch4[scratchCorner];
        dst->x = src[0];
        dst->y = src[1];
        dst->z = src[2];
    }

    /**
     * Original static helper observed in cls_di single-segment bbox callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build and test one bbox face polygon against the active segment.
     */
    bool TestBBoxFace(
        zClassDiPickCandidateEntry * candidate,
        const zVec3 *segmentStart,
        const zVec3 *segmentEnd,
        int corner0,
        int corner1,
        int corner2,
        int corner3,
        const zBBoxCorners *bboxCorners
    ) {
        CopyBBoxCornerToScratch(
            bboxCorners,
            corner0,
            0
        );
        CopyBBoxCornerToScratch(
            bboxCorners,
            corner1,
            1
        );
        CopyBBoxCornerToScratch(
            bboxCorners,
            corner2,
            2
        );
        CopyBBoxCornerToScratch(
            bboxCorners,
            corner3,
            3
        );
        return zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(
                   candidate,
                   segmentStart,
                   segmentEnd,
                   g_zClass_DiFaceVertexScratch4,
                   4,
                   0
               ) != 0;
    }

    /**
     * Original static helper observed in cls_di segment-batch bbox callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build and test one bbox face polygon against the active segment batch.
     */
    int TestSegmentBatchBBoxFace(
        zClass_NodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch,
        int *activeMask,
        int segmentCount,
        const zBBoxCorners *bboxCorners,
        zModel_PickFaceEntry *faceEntry,
        int corner0,
        int corner1,
        int corner2,
        int corner3
    ) {
        CopyBBoxCornerToScratch(
            bboxCorners,
            corner0,
            0
        );
        CopyBBoxCornerToScratch(
            bboxCorners,
            corner1,
            1
        );
        CopyBBoxCornerToScratch(
            bboxCorners,
            corner2,
            2
        );
        CopyBBoxCornerToScratch(
            bboxCorners,
            corner3,
            3
        );
        return zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon(
            candidateOwner,
            outCandidateBuffersBySegment,
            segmentEndpointsByBatch,
            activeMask,
            segmentCount,
            g_zClass_DiFaceVertexScratch4,
            faceEntry
        );
    }

    /**
     * Original static helper; no standalone retail function exists. Observed
     * in address-backed segment-batch callers including 0x4476f0 and
     * 0x486290 as the packed segment endpoint view of the active point array.
     * Purpose: reinterpret the active pick point array as segment endpoint
     * pairs for batched segment traversal.
     */
    zClass_DiSegmentEndpoints *SegmentEndpointBatchFromPickPointArray() {
        return (zClass_DiSegmentEndpoints *)((void *)(g_DiPickPointArray));
    }

    /**
     * Original static helper observed in cls_di segment-bbox callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: reject segment bounds that do not overlap a candidate box.
     */
    bool SegmentBoundsOverlapBox(
        const zClass_DiSegmentBounds *bounds,
        float minX,
        float maxX,
        float minY,
        float maxY,
        float minZ,
        float maxZ
    ) {
        return bounds->maxX > minX && bounds->minX < maxX && bounds->maxY > minY &&
               bounds->minY < maxY && bounds->maxZ > minZ && bounds->minZ < maxZ;
    }

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: copy the per-segment active mask for recursive filtering.
     */
    void CopySegmentActiveMask(
        int *dst,
        const int *src
    ) {
        memcpy(
            dst,
            src,
            (size_t)(g_DiPickPointCount) * sizeof(int)
        );
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: convert a world coordinate to a grid cell coordinate.
     */
    int GridCoordFromWorld(
        float value,
        float origin,
        float invCellSize
    ) {
        return (int)(floor((value - origin) * invCellSize));
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: clamp a grid coordinate into the world-area grid range.
     */
    int ClampGridCoord(
        int coord,
        int count
    ) {
        if (coord < 0) {
            return 0;
        }
        if (coord >= count) {
            return count - 1;
        }
        return coord;
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: shift all active segment endpoints and bounds in the XZ plane.
     */
    void OffsetSegmentBatchXZ(
        float offsetX,
        float offsetZ
    ) {
        zClass_DiSegmentEndpoints *segments = SegmentEndpointBatchFromPickPointArray();
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            segments[i].start.x += offsetX;
            segments[i].start.z += offsetZ;
            segments[i].end.x += offsetX;
            segments[i].end.z += offsetZ;

            g_DiSegmentBounds[i].minX += offsetX;
            g_DiSegmentBounds[i].maxX += offsetX;
            g_DiSegmentBounds[i].minZ += offsetZ;
            g_DiSegmentBounds[i].maxZ += offsetZ;
        }
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: snapshot candidate counts before processing a clamped grid cell.
     */
    void SaveSegmentCandidateCounts(int *candidateCounts) {
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            candidateCounts[i] = g_DiPickCandidateBuffer[i].candidateCount;
        }
    }

    /**
     * Original static helper observed in cls_di segment-batch grid callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: restore new candidate positions from clamped cell space to world space.
     */
    void RestoreClampedSegmentCandidatePositions(
        const int *firstNewCandidate,
        float offsetX,
        float offsetZ
    ) {
        for (int i = 0; i < g_DiPickPointCount; ++i) {
            PlayerProbeSampleCandidateBuffer *buffer = &g_DiPickCandidateBuffer[i];
            for (int j = firstNewCandidate[i]; j < buffer->candidateCount; ++j) {
                buffer->entries[j].hitPos.x -= offsetX;
                buffer->entries[j].hitPos.z -= offsetZ;
            }
        }
    }

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: build axis-aligned segment bounds from start and end points.
     */
    void BuildSegmentBoundsFromEndpoints(
        const zClass_DiSegmentEndpoints *segments,
        zClass_DiSegmentBounds *bounds
    ) {
        bounds->minX = segments->start.x < segments->end.x ? segments->start.x : segments->end.x;
        bounds->maxX = segments->start.x < segments->end.x ? segments->end.x : segments->start.x;
        bounds->minY = segments->start.y < segments->end.y ? segments->start.y : segments->end.y;
        bounds->maxY = segments->start.y < segments->end.y ? segments->end.y : segments->start.y;
        bounds->minZ = segments->start.z < segments->end.z ? segments->start.z : segments->end.z;
        bounds->maxZ = segments->start.z < segments->end.z ? segments->end.z : segments->start.z;
    }

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: test whether segment bounds overlap the world grid in XZ.
     */
    bool SegmentBoundsOverlapWorldXZ(
        const zClass_DiSegmentBounds *bounds,
        const zClass_WorldDataPartial *worldData
    ) {
        return bounds->minX < worldData->worldMaxX && bounds->maxX >= worldData->originX &&
               bounds->minZ <= worldData->originZ && bounds->maxZ > worldData->worldMaxZ;
    }

    zDiPartial *NodeDiRef(zClass_NodePartial * node);

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: filter the current node's pick faces against the active segment batch.
     */
    void FilterCurrentSegmentRegions(
        zClass_NodePartial * node,
        int *activeMask
    ) {
        zModel_PickFaceData *faceData = (zModel_PickFaceData *)((void *)(NodeDiRef(node)));
        if (faceData != 0) {
            zClass_cls_di::FilterRegionsAgainstPolygon(
                node,
                faceData,
                SegmentEndpointBatchFromPickPointArray(),
                activeMask,
                g_DiPickPointCount,
                g_DiPickCandidateBuffer
            );
        }
    }

    /**
     * Original static helper observed in cls_di segment-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: recurse over list-B children for the active segment batch.
     */
    void RecurseSegmentBatchChildren(
        zClass_NodePartial * node,
        int *activeMask,
        bool requirePickFlags
    ) {
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if (!requirePickFlags || ((child->flags & kNodeFlagEnabledForPick) != 0 &&
                                         (child->flags & kNodeFlagRaycastable) != 0)) {
                zClass_cls_di::BuildPickCandidatesForSegmentsRecursive(
                    child,
                    node->listCountB,
                    activeMask
                );
            }

            if (BreakOnFirstCandidateHit()) {
                break;
            }
        }
    }

    /**
     * Original static helper; no standalone retail function exists. Observed
     * in address-backed cls_di point and segment traversal callers including
     * 0x443f80, 0x444890, 0x444c50, 0x444d10, and 0x446440.
     * Purpose: view the node payload pointer as a zDi record for point and
     * segment pick tests.
     */
    zDiPartial *NodeDiRef(zClass_NodePartial * node) {
        return (zDiPartial *)((unsigned int)(node->userDataOrDiRef));
    }

    /**
     * Original static helper observed in cls_di point-query traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: apply the optional variant id gate for point-query nodes.
     */
    bool NodePassesQueryVariant(zClass_NodePartial * node) {
        return (node->flags & 0x01000000) == 0 || VariantTag::CurrentAllowsId(node->nodeType) != 0;
    }

    /**
     * Original static helper observed in cls_di point-query traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: test the node flags required for point-query candidates.
     */
    bool NodePassesQueryFlags(zClass_NodePartial * node) {
        return (node->flags & kNodeFlagEnabledForPick) != 0 && (node->flags & 0x08) != 0;
    }

    /**
     * Original static helper observed in cls_di point-query traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: append the node when its DI payload accepts the active query point.
     */
    void AppendQueryPointCandidateIfHit(zClass_NodePartial * node) {
        zDiPartial *di = NodeDiRef(node);
        if (di == 0) {
            return;
        }

        PlayerProbeSampleCandidateBuffer *buffer = g_DiPickCandidateBuffer;
        zClassDiPickCandidateEntry *outCandidate = &buffer->entries[buffer->candidateCount];
        if (zDi::BuildPickCandidateForQueryPoint(
            di,
            outCandidate,
            &g_DiPickQueryPoint
        ) != 0) {
            AppendCurrentCandidateNode(node);
        }
    }

    /**
     * Original static helper observed in cls_di point-query traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: recurse over list-B children for the active single point query.
     */
    void RecurseQueryPointChildren(
        zClass_NodePartial * node,
        int cullCount,
        bool requireQueryFlags
    ) {
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if (!requireQueryFlags || NodePassesQueryFlags(child)) {
                zClass_cls_di::BuildPickCandidateList(
                    child,
                    cullCount
                );
            }
        }
    }

    /**
     * Original static helper observed in cls_di point-batch traversal callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: recurse over list-B children for the active point batch query.
     */
    void RecursePointBatchChildren(
        zClass_NodePartial * node,
        int depth,
        int *hitFlags,
        bool requireQueryFlags
    ) {
        for (int i = 0; i < node->listCountB; ++i) {
            zClass_NodePartial *child = node->listB[i];
            if (!requireQueryFlags || NodePassesQueryFlags(child)) {
                zClass_cls_di::BuildPickCandidatesForPoints(
                    child,
                    depth,
                    hitFlags
                );
            }
        }
    }

    /**
     * Original static helper observed in cls_di transformed mesh and polygon pick callers
     * (D:\Proj\GameZRecoil\zClass\cls_di.c).
     * Purpose: copy or transform model vertices into the shared world-space scratch buffer.
     */
    void TransformVerticesToSharedScratch(
        const zVec3 *vertices,
        int vertexCount
    ) {
        if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
            memcpy(
                g_zModel_SharedVec3ScratchB,
                vertices,
                (size_t)(vertexCount) * sizeof(zVec3)
            );
            return;
        }

        for (int i = 0; i < vertexCount; ++i) {
            g_zModel_SharedVec3ScratchB[i] = TransformModelPointToWorld(&vertices[i]);
        }
    }
}


namespace zModel_Const {
    /**
     * Reimplements 0x481530: zModel_Const::GetVertexMergeEpsilon
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: return the global vertex-merge epsilon.
     */
    float GetVertexMergeEpsilon() {
        return g_zModel_ConstVertexMergeEpsilon;
    }
} // namespace zModel_Const

namespace zModel_Const {
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
} // namespace zModel_Const

namespace zModel_Const {
    /**
     * Reimplements 0x481550: zModel_Const::SetCoplanarTolerance
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: set the global coplanar polygon tolerance.
     */
    void __stdcall SetCoplanarTolerance(float tolerance) {
        g_zModel_CoplanarTolerance = tolerance;
    }
} // namespace zModel_Const

namespace zModel_Const {
    /**
     * Reimplements 0x481560: zModel_Const::SetColinearTolerance
     * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
     * Purpose: set the global colinear polygon tolerance.
     */
    void __stdcall SetColinearTolerance(float tolerance) {
        g_zModel_ColinearTolerance = tolerance;
    }
} // namespace zModel_Const

namespace zDi {
/**
 * Reimplements 0x481570: zDi::PtrToIndexOrMinus1
 * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
 * Purpose: convert a display-instance pool pointer to its pool index, or -1 for null.
 */
int __fastcall PtrToIndexOrMinus1(
    zDiPartial *self
) {
    if (self == 0) {
        return -1;
    }

    return (int)(self - g_zModel_DiPoolBase);
}
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x4815a0: zDi::IndexToPtrOrNull
 * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
 * Purpose: convert a non-negative display-instance pool index to its entry pointer.
 */
zDiPartial *__fastcall IndexToPtrOrNull(
    int index
) {
    if (index < 0) {
        return 0;
    }

    return &g_zModel_DiPoolBase[index];
}
} // namespace zDi

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
} // namespace zModel_DiPool

namespace zModel_DiPool {
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
} // namespace zModel_DiPool

namespace zModel_DiPool {
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
} // namespace zModel_DiPool

namespace zModel_DiPool {
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
} // namespace zModel_DiPool

namespace zModel_DiPool {
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
} // namespace zModel_DiPool

namespace zModel_DiPool {
/**
 * Reimplements 0x482080: zModel_DiPool::AllocFromFreeList
 * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
 * Purpose: allocate and initialize a display-instance pool entry from the free list.
 */
zDiPartial *AllocFromFreeList() {
    const int slotIndex = g_zModel_DiPoolFreeHeadIndex;
    if (slotIndex < 0) {
        zError::ReportOld(
            0x400,
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            0x4a1,
            "ERROR: Creating Model3D; model buffer full."
        );
        return 0;
    }

    zDiPartial *const entry = &g_zModel_DiPoolBase[slotIndex];
    g_zModel_DiPoolFreeHeadIndex = entry->nextFreeIndex;
    g_zModel_DiPoolInUseCount += 1;
    memset(
        entry,
        0,
        offsetof(zDiPartial, nextFreeIndex)
    );
    entry->flags = (entry->flags & 0xffffffdf) | 0x03;
    return entry;
}
} // namespace zModel_DiPool

namespace zModel_DiPool {
/**
 * Reimplements 0x4820f0: zModel_DiPool::FreeIfUnreferenced
 * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
 * Purpose: release an unreferenced display-instance entry back to the pool free list.
 */
int __fastcall FreeIfUnreferenced(
    zDiPartial *di
) {
    if (di == 0) {
        return 5;
    }

    if (di->refCount != 0) {
        return 1;
    }

    zDi::FreeContents(di);
    memset(
        di,
        0,
        offsetof(zDiPartial, nextFreeIndex)
    );

    const ptrdiff_t slotIndex = di - g_zModel_DiPoolBase;
    g_zModel_DiPoolBase[slotIndex].nextFreeIndex = g_zModel_DiPoolFreeHeadIndex;
    --g_zModel_DiPoolInUseCount;
    g_zModel_DiPoolFreeHeadIndex = (int)(slotIndex);
    return 0;
}
} // namespace zModel_DiPool

namespace zDi {
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
} // namespace zDi

namespace zDi {
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
} // namespace zDi

namespace zUtil {
/**
 * Reimplements 0x4826a0: zUtil::StoreInt32.
 * Purpose: Stores the supplied 32-bit integer through the destination pointer.
 */
void __fastcall StoreInt32(
    int *outValue,
    int value
) {
    *outValue = value;
}
} // namespace zUtil

namespace zDi {
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
} // namespace zDi

namespace zDi {
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
} // namespace zDi

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
} // namespace zDi

namespace zDi {
    /**
     * Reimplements 0x482700: zDi::Release
     * (GameZRecoil/zDi/zdi.c).
     * Purpose: decrement a display-instance reference count.
     */
    int __fastcall Release(zDiPartial * self) {
        --self->refCount;
        return 0;
    }
} // namespace zDi

namespace zDi {
    /**
     * Reimplements 0x482710: zDi::GetRefCount
     * (GameZRecoil/zModel/gdi.c).
     * Purpose: return a display-instance reference count.
     */
    int __fastcall GetRefCount(zDiPartial * self) {
        return self->refCount;
    }
} // namespace zDi

namespace zModel_Const {
/**
 * Reimplements 0x482720: zModel_Const::AddOrMergeVertex
 * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
 * Purpose: find an existing nearby vertex or append a new display-instance vertex.
 */
int __fastcall AddOrMergeVertex(
    zDiPartial *self,
    zVec3 *point
) {
    for (int vertexIndex = 0; vertexIndex < self->vertCount; ++vertexIndex) {
        const zVec3 *const existingPoint = &self->verts[vertexIndex];
        if (fabs(existingPoint->x - point->x) <= g_zModel_ConstVertexMergeEpsilon &&
            fabs(existingPoint->y - point->y) <= g_zModel_ConstVertexMergeEpsilon &&
            fabs(existingPoint->z - point->z) <= g_zModel_ConstVertexMergeEpsilon) {
            return vertexIndex;
        }
    }

    if ((double)(self->vertCount) > g_zModel_ConstVertexWarnThreshold) {
        sprintf(
            g_zError_DebugMsgBuffer,
            "%s: Line %d: WARNING: Model vertex count = %d\n",
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            1783,
            self->vertCount
        );
        sprintf(
            g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
            "         Approaching max allowable: %d\n",
            1024
        );
        zError::EmitDebugBuffer(1);
        return -1;
    }

    const int appendedVertexIndex = self->vertCount;
    self->verts =
        (zVec3 *)(realloc(
            self->verts,
            (size_t)(appendedVertexIndex + 1) * sizeof(zVec3)
        ));
    self->verts[appendedVertexIndex] = *point;
    self->vertCount = appendedVertexIndex + 1;
    return appendedVertexIndex;
}
} // namespace zModel_Const

namespace zModel_Const {
/**
 * Reimplements 0x482860: zModel_Const::AddOrMergeVertexAndNormal
 * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
 * Purpose: find or append a vertex plus its blend-normal delta.
 */
int __fastcall AddOrMergeVertexAndNormal(
    zDiPartial *self,
    zVec3 *point,
    zVec3 *normal
) {
    zVec3 blendNormalDelta;
    blendNormalDelta.x = normal->x - point->x;
    blendNormalDelta.y = normal->y - point->y;
    blendNormalDelta.z = normal->z - point->z;

    for (int vertexIndex = 0; vertexIndex < self->vertCount; ++vertexIndex) {
        const zVec3 *const existingPoint = &self->verts[vertexIndex];
        const zVec3 *const existingBlend = &self->blendVerts[vertexIndex];
        if (existingPoint->x == point->x && existingPoint->y == point->y &&
            existingPoint->z == point->z && existingBlend->x == blendNormalDelta.x &&
            existingBlend->y == blendNormalDelta.y && existingBlend->z == blendNormalDelta.z) {
            return vertexIndex;
        }
    }

    const int appendedVertexIndex = self->vertCount;
    self->verts =
        (zVec3 *)(realloc(
            self->verts,
            (size_t)(appendedVertexIndex + 1) * sizeof(zVec3)
        ));
    self->verts[appendedVertexIndex] = *point;

    self->blendVerts =
        (zVec3 *)(realloc(
            self->blendVerts,
            (size_t)(appendedVertexIndex + 1) * sizeof(zVec3)
        ));
    self->blendVerts[appendedVertexIndex] = blendNormalDelta;

    self->vertCount = appendedVertexIndex + 1;
    self->blendVertCount = self->vertCount;
    if ((double)(self->vertCount) > g_zModel_ConstVertexWarnThreshold) {
        sprintf(
            g_zError_DebugMsgBuffer,
            "%s: Line %d: WARNING: Model vertex count = %d\n",
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            1896,
            self->vertCount
        );
        sprintf(
            g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
            "         Approaching max allowable: %d\n",
            1024
        );
        zError::EmitDebugBuffer(1);
        return -1;
    }

    return appendedVertexIndex;
}
} // namespace zModel_Const

namespace zModel_Const {
/**
 * Reimplements 0x482a10: zModel_Const::FindOrAppendNormalIndex
 * (D:\Proj\GameZRecoil\zModel\gmod_const.c).
 * Purpose: find an existing nearby normal or append a new normal.
 */
int __fastcall FindOrAppendNormalIndex(
    zDiPartial *self,
    zVec3 *normal
) {
    for (int normalIndex = 0; normalIndex < self->normalCount; ++normalIndex) {
        const zVec3 *const existingNormal = &self->normals[normalIndex];
        if (fabs(existingNormal->x - normal->x) < g_zModel_NormalMergeEpsilon &&
            fabs(existingNormal->y - normal->y) < g_zModel_NormalMergeEpsilon &&
            fabs(existingNormal->z - normal->z) < g_zModel_NormalMergeEpsilon) {
            return normalIndex;
        }
    }

    const int appendedNormalIndex = self->normalCount;
    self->normals =
        (zVec3 *)(realloc(
            self->normals,
            (size_t)(appendedNormalIndex + 1) * sizeof(zVec3)
        ));
    self->normals[appendedNormalIndex] = *normal;
    self->normalCount = appendedNormalIndex + 1;
    if ((double)(self->normalCount) > g_zModel_ConstVertexWarnThreshold) {
        sprintf(
            g_zError_DebugMsgBuffer,
            "%s: Line %d: WARNING: Model normal count = %d\n",
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            1972,
            self->normalCount
        );
        sprintf(
            g_zError_DebugMsgBuffer + strlen(g_zError_DebugMsgBuffer),
            "         Approaching max allowable: %d\n",
            1024
        );
        zError::EmitDebugBuffer(1);
        return -1;
    }

    return appendedNormalIndex;
}
} // namespace zModel_Const

namespace zModel_Const {
/**
 * Reimplements 0x482b40: zModel_Const::RemoveColinearVerticesInPlace
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: remove colinear vertices from a polygon point array in place.
 */
int __fastcall RemoveColinearVerticesInPlace(
    int *vertexCount,
    zVec3 *points,
    zClipUV *,
    zVec3 *,
    zClipUV *
) {
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
                    currentVertex - 1,
                    currentVertex,
                    &outNormal,
                    &points[nextIndex]
                );

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
} // namespace zModel_Const

namespace zModel_Const {
/**
 * Reimplements 0x482c60: zModel_Const::SetNormalizedCrossFromVertexTriplet
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: compute and normalize the cross product from three polygon vertices.
 */
zVec3 *__fastcall SetNormalizedCrossFromVertexTriplet(
    zVec3 *vertex0,
    zVec3 *vertex1,
    zVec3 *outNormal,
    zVec3 *vertex2
) {
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
    if (fabs(normalX) > g_zModel_ColinearTolerance || fabs(normalY) > g_zModel_ColinearTolerance ||
        fabs(normalZ) > g_zModel_ColinearTolerance) {
        length = sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ);
    }

    double scale = 0.0;
    if (fabs(length) > g_zModel_ColinearTolerance) {
        scale = 1.0 / length;
    }

    outNormal->x = (float)(normalX * scale);
    outNormal->y = (float)(normalY * scale);
    outNormal->z = (float)(normalZ * scale);
    return outNormal;
}
} // namespace zModel_Const

namespace zModel_Const {
/**
 * Reimplements 0x482db0: zModel_Const::IsPolygonCoplanar
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: test whether every polygon vertex lies within the coplanar tolerance.
 */
int __fastcall IsPolygonCoplanar(
    int vertexCount,
    zVec3 *vertices
) {
    zGeometry_PlaneEquationPartial plane;
    ComputePolygonPlaneEquation(
        vertexCount,
        vertices,
        &plane
    );

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
} // namespace zModel_Const

namespace zModel_Const {
/**
 * Reimplements 0x482e30: zModel_Const::ComputePolygonPlaneEquation
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: compute a normalized plane equation for a polygon.
 */
zGeometry_PlaneEquationPartial *__fastcall ComputePolygonPlaneEquation(
    int vertexCount,
    zVec3 *vertices,
    zGeometry_PlaneEquationPartial *outPlane
) {
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
        normalLength = (float)(sqrt(normalX * normalX + normalY * normalY + normalZ * normalZ));
    }

    float inverseNormalLength = 0.0f;
    if (normalLength != 0.0f) {
        inverseNormalLength = 1.0f / normalLength;
    }

    outPlane->a = normalX * inverseNormalLength;
    outPlane->b = normalY * inverseNormalLength;
    outPlane->c = normalZ * inverseNormalLength;
    outPlane->d =
        -((sumX * normalX + sumY * normalY + sumZ * normalZ) / ((float)(vertexCount)*normalLength));
    return outPlane;
}
} // namespace zModel_Const

namespace zModel_Const {
/**
 * Reimplements 0x482fe0: zModel_Const::SplitPolygonChunkedByVertexLimit
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: triangulate a polygon into fan triangles for AddPolygonEx.
 */
void __fastcall SplitPolygonChunkedByVertexLimit(
    zDiPartial *self,
    int totalVertexCount,
    zVec3 *points,
    zVec3 *entryNormals,
    zClipUV *uvPairsA,
    zVec3 *normalsA,
    zVec3 *normalsBInput,
    zClipUV *uvPairsBInput,
    zModel_MaterialPartial *material,
    unsigned int drawFlags,
    int flagBit8,
    const int *userTag
) {
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

        zDi::AddPolygonEx(
            self,
            3,
            trianglePoints,
            entryNormals != 0 ? triangleEntryNormals : 0,
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

namespace zDi {
/**
 * Reimplements 0x483240: zDi::AddPolygonSplitByVertexLimit
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: split an oversized polygon into overlapping chunks within the vertex limit.
 */
void __fastcall AddPolygonSplitByVertexLimit(
    zDiPartial *self,
    int totalVertexCount,
    zVec3 *points,
    zVec3 *entryNormals,
    zClipUV *uvPairsA,
    zVec3 *normalsA,
    zVec3 *normalsBInput,
    zClipUV *uvPairsBInput,
    zModel_MaterialPartial *material,
    unsigned int drawFlags,
    int flagBit8,
    const int *userTag,
    int maxChunkVertexCount
) {
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
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
                0xa16,
                "Attempting to add polygon with only %d verts",
                vertexCount
            );
        }

        AddPolygonEx(
            self,
            vertexCount,
            chunkPoints,
            entryNormals != 0 ? chunkEntryNormals : 0,
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
    } while (chunkStartVertexIndex < totalVertexCount - 1);
}
} // namespace zDi

namespace zModel_Const {
/**
 * Reimplements 0x483510: zModel_Const::QuantizeAndNormalizeUvPairs
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: quantize UV pairs and normalize them to a local tile origin.
 */
void __fastcall QuantizeAndNormalizeUvPairs(
    int vertexCount,
    zClipUV *uvPairs
) {
    if (vertexCount > 0) {
        for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            zClipUV *const uv = &uvPairs[vertexIndex];
            const int uFixed = (int)((uv->u - g_zModel_UvQuantizeBias) * g_zModel_UvQuantizeScale);
            uv->u = (float)(uFixed)*g_zModel_UvQuantizeInvScale;

            const int vFixed = (int)((uv->v - g_zModel_UvQuantizeBias) * g_zModel_UvQuantizeScale);
            uv->v = (float)(vFixed)*g_zModel_UvQuantizeInvScale;
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

namespace zDi {
/**
 * Reimplements 0x483610: zDi::AddPolygon
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: add a polygon entry without explicit per-entry normals.
 */
int __fastcall AddPolygon(
    zDiPartial *self,
    int pointCount,
    zVec3 *points,
    zClipUV *uvPairsA,
    zVec3 *normalsA,
    zVec3 *normalsB,
    zClipUV *uvPairsB,
    zModel_MaterialPartial *material,
    unsigned int drawFlags,
    int flagBit8,
    const int *userTag
) {
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
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x483650: zDi::AddPolygonEx
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: add a polygon entry with optional normals, UVs, splitting, and generated UV repair.
 */
int __fastcall AddPolygonEx(
    zDiPartial *self,
    int vertexCount,
    zVec3 *points,
    zVec3 *entryNormals,
    zClipUV *uvPairsA,
    zVec3 *normalsA,
    zVec3 *normalsB,
    zClipUV *uvPairsB,
    zModel_MaterialPartial *material,
    unsigned int drawFlags,
    int flagBit8,
    const int *userTag
) {
    if (vertexCount < 3) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            0xae4,
            "ERROR: You're trying to add a Polygon with only (%d) verts",
            vertexCount
        );
        return 1;
    }

    if (vertexCount >= 58) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            0xaed,
            "Poly vertex count approaching limit (%d / %d)",
            vertexCount,
            0x40
        );
        return 1;
    }

    const int originalVertexCount = vertexCount;
    if (zModel_Const::RemoveColinearVerticesInPlace(
            &vertexCount,
            points,
            uvPairsA,
            normalsB,
            uvPairsB
        ) != 0 &&
        vertexCount < 3) {
        zError::ReportOld(
            0x100,
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            0xb0d,
            "Discarding Polygon: (%d of %d) verts after 'check_colinearity()'",
            vertexCount,
            originalVertexCount
        );
        return 1;
    }

    if (vertexCount > 3 && zModel_Const::IsPolygonCoplanar(
        vertexCount,
        points
    ) == 0) {
        zError::ReportOld(
            0x100,
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            0xb19,
            "Attempting to add non-planar polygon (%d verts), triangulating...",
            vertexCount
        );
        zModel_Const::SplitPolygonChunkedByVertexLimit(
            self,
            originalVertexCount,
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

    if (vertexCount > g_zModel_MaxPolygonVertexCountBeforeSplit) {
        AddPolygonSplitByVertexLimit(
            self,
            originalVertexCount,
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

    zDiEntryPartial *entries = (zDiEntryPartial *)(realloc(
        self->entries,
        (size_t)(self->entryCount + 1) * sizeof(zDiEntryPartial)
    ));
    self->entries = entries;

    zDiEntryPartial *const entry = &entries[self->entryCount];
    memset(
        entry,
        0,
        sizeof(zDiEntryPartial)
    );
    entry->flagsAndIndexCount =
        (unsigned int)(vertexCount & 0xff) | ((unsigned int)(flagBit8 & 1) << 8);
    if (entryNormals != 0) {
        entry->flagsAndIndexCount |= 0x200;
    }
    entry->drawFlags = drawFlags;
    entry->vertexIndices = malloc((size_t)(vertexCount) * sizeof(int));
    if (entryNormals != 0) {
        entry->normalIndices = malloc((size_t)(vertexCount) * sizeof(int));
    }

    int *vertexIndices = (int *)(entry->vertexIndices);
    int *normalIndices = (int *)(entry->normalIndices);
    zVec3 *pointCursor = points;
    zVec3 *normalBCursor = normalsB;
    zVec3 *entryNormalCursor = entryNormals;
    for (int i = 0; i < vertexCount; ++i) {
        if (normalsA != 0) {
            vertexIndices[i] =
                zModel_Const::AddOrMergeVertexAndNormal(
                    self,
                    pointCursor,
                    normalBCursor
                );
            ++normalBCursor;
        } else {
            vertexIndices[i] = zModel_Const::AddOrMergeVertex(
                self,
                pointCursor
            );
        }
        if (vertexIndices[i] < 0) {
            return 1;
        }

        if (entryNormals != 0) {
            normalIndices[i] = zModel_Const::FindOrAppendNormalIndex(
                self,
                entryNormalCursor
            );
            ++entryNormalCursor;
        }
        ++pointCursor;
    }

    if ((material->flags & 0x0100) != 0) {
        entry->uvPairs = malloc((size_t)(vertexCount) * sizeof(zClipUV));
        memcpy(
            entry->uvPairs,
            uvPairsA,
            (size_t)(vertexCount) * sizeof(zClipUV)
        );
        NormalizeUvTileOrigin(
            (zClipUV *)(entry->uvPairs),
            vertexCount
        );
    }

    entry->material = material;
    RebuildGeneratedUvPairsForEntry(
        self,
        self->entryCount
    );
    if ((material->flags & 0x0100) != 0) {
        zModel_Const::QuantizeAndNormalizeUvPairs(
            vertexCount,
            (zClipUV *)(entry->uvPairs)
        );
    }
    memcpy(
        &entry->variantTagInitialized,
        userTag,
        sizeof(*userTag)
    );

    ++self->entryCount;
    return 0;
}
} // namespace zDi

namespace zDi {
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
} // namespace zDi

namespace zDi {
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
} // namespace zDi

namespace zDi {
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
} // namespace zDi

namespace zDi {
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
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x483f80: zDi::BuildBlendVertsFromConnectivity
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: build blend-vertex offsets from connectivity and exclusion rules.
 */
void __fastcall BuildBlendVertsFromConnectivity(
    zDiPartial *self,
    int *excludedVertexIndices,
    float blendY,
    int excludedVertexCount,
    int minSharedVertexCount
) {
    const int vertCount = self->vertCount;
    self->blendVerts = (zVec3 *)(realloc(
        self->blendVerts,
        (size_t)(vertCount) * sizeof(zVec3)
    ));

    int *const blendDisabledMask = (int *)(malloc((size_t)(vertCount) * sizeof(int)));
    int *const vertexReferenceCounts = (int *)(malloc((size_t)(vertCount) * sizeof(int)));

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
        int *const vertexIndices = (int *)(entry->vertexIndices);
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
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x484140: zDi::SetEntryValueForAllEntries
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: set the draw-flags value for every display-instance polygon entry.
 */
void __fastcall SetEntryValueForAllEntries(
    zDiPartial *self,
    unsigned int entryValue
) {
    if (self == 0) {
        return;
    }

    for (int i = 0; i < self->entryCount; ++i) {
        self->entries[i].drawFlags = entryValue;
    }
}
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x484170: zDi::SetShowBackFaceForAllEntries
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: update the show-backface bit on every display-instance polygon entry.
 */
void __fastcall SetShowBackFaceForAllEntries(
    zDiPartial *self,
    int enabled
) {
    const unsigned int showBackFaceBit = (enabled & 1) << 8;
    for (int i = 0; i < self->entryCount; ++i) {
        self->entries[i].flagsAndIndexCount =
            (self->entries[i].flagsAndIndexCount & ~0x0100u) | showBackFaceBit;
    }
}
} // namespace zDi

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
} // namespace zDi

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
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x484230: zDi::ResetCurrentVariant
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: reset the current material cycle frame on the first entry.
 */
void __fastcall ResetCurrentVariant(
    zDiPartial *self
) {
    zModel_MaterialPartial *const material = self->entries->material;
    zModel_MaterialCyclePartial *const cycle = material->cycle;
    if (cycle != 0) {
        cycle->currentFrame = 0.0f;
        material->currentTextureDirectoryEntry = cycle->frameTable[0];
    }
}
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x484250: zDi::SetCurrentVariantCycleTextureCount
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: configure the current material cycle texture count.
 */
int __fastcall SetCurrentVariantCycleTextureCount(
    zDiPartial *self,
    int textureCount
) {
    if (self == 0) {
        sprintf(
            g_zError_DebugMsgBuffer,
            "%s(%d): ERROR setting model cycle texture. Model 3D pointer is NULL.\n",
            "D:\\Proj\\GameZRecoil\\zModel\\gmod_const.c",
            0xf3f
        );
        fprintf(
            stderr,
            g_zError_DebugMsgBuffer
        );
        return -1;
    }

    zModel_MaterialPartial *const material = self->entries->material;
    if (material != 0) {
        zModel_Material::SetCycleTextureCount(
            material,
            textureCount
        );
        return 0;
    }

    // Original code reaches this only for a null material pointer and then
    // dereferences it while clearing the cycle-texture flag.
    material->flags = (unsigned short)(material->flags & 0xfbff);
    return 0;
}
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x4842b0: zDi::SetCurrentVariant
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: select the current texture-cycle variant frame.
 */
void __fastcall SetCurrentVariant(
    zDiPartial *self,
    int variantIndex
) {
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
    cycle->currentFrame = (float)(variantIndex);
}
} // namespace zDi

namespace zModel_Instance {
/**
 * Reimplements 0x4842f0: zModel_Instance::SetCycleTextureLoop
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: set the cycle loop flag on an instance's first material entry.
 */
int __fastcall SetCycleTextureLoop(
    zDiPartial *instance,
    int loopEnabled
) {
    if (instance == 0) {
        return 0;
    }

    return zModel_Material::SetCycleTextureLoop(
        instance->entries->material,
        loopEnabled
    );
}
} // namespace zModel_Instance

namespace zDi {
/**
 * Reimplements 0x484310: zDi::SetCurrentVariantCycleTextureSpeed
 * (D:\Proj\GameZRecoil\zDi\zdi.cpp).
 * Purpose: set the cycle speed for the current material variant.
 */
int __fastcall SetCurrentVariantCycleTextureSpeed(
    zDiPartial *self,
    float cycleSpeed
) {
    if (self == 0) {
        return 0;
    }

    return zModel_Material::SetCycleTextureSpeed(
        self->entries->material,
        cycleSpeed
    );
}
} // namespace zDi

namespace zModel_Instance {
/**
 * Reimplements 0x484330: zModel_Instance::AddCycleTexture
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: add a cycle texture to an instance's first material entry.
 */
int __fastcall AddCycleTexture(
    zDiPartial *instance,
    zImage_TexDirEntryPartial *textureDirectoryEntry
) {
    if (instance == 0) {
        return 0;
    }

    return zModel_Material::AddCycleTexture(
        instance->entries->material,
        textureDirectoryEntry
    );
}
} // namespace zModel_Instance

namespace zDi {
/**
 * Reimplements 0x484350: zDi::SetObject3DColorModeForMaterials
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: apply an object3D color mode to untextured materials.
 */
void __fastcall SetObject3DColorModeForMaterials(
    zDiPartial *self,
    int colorMode
) {
    zDiEntryPartial *entry = self->entries;
    for (int i = 0; i < self->entryCount; ++i, ++entry) {
        zModel_MaterialPartial *material = entry->material;
        if ((material->flags & 0x0100) != 0) {
            continue;
        }

        material->colorRgb.red = (float)(colorMode);
        material->colorRgb.green = 0.0f;
        material->colorRgb.blue = 0.0f;
        material->packedColor = (unsigned short)((material->packedColor & 0x00ff) |
                                                 (((unsigned int)(colorMode) & 0xff) << 8));
        material->colorScalar = 1.0f;
    }
}
} // namespace zDi

namespace zDi {
/**
 * Reimplements 0x4843b0: zDi::RebuildGeneratedUvPairsForEntry
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: rebuild generated UV pairs for polygon vertices beyond the first triangle.
 */
void __fastcall RebuildGeneratedUvPairsForEntry(
    zDiPartial *self,
    int entryIndex
) {
    zDiEntryPartial *const entry = &self->entries[entryIndex];
    const int vertexCount = (int)(entry->flagsAndIndexCount & 0xff);
    if (entry->material == 0 || (entry->material->flags & 0x0100) == 0 || vertexCount <= 3) {
        return;
    }

    int *const vertexIndices = (int *)(entry->vertexIndices);
    zClipUV *const uvPairs = (zClipUV *)(entry->uvPairs);
    const zVec3 *const vertex0 = &self->verts[vertexIndices[0]];
    const zVec3 *const vertex1 = &self->verts[vertexIndices[1]];
    const zVec3 *const vertex2 = &self->verts[vertexIndices[2]];

    zVec3 triangleNormal;
    zMath_Vec3_TriangleNormal(
        vertex0,
        vertex1,
        vertex2,
        &triangleNormal
    );
    zMath::Vec3Normalize(&triangleNormal);

    const float absX = (float)(fabs(triangleNormal.x));
    const float absY = (float)(fabs(triangleNormal.y));
    const float absZ = (float)(fabs(triangleNormal.z));

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
        vertex0A,
        vertex0B,
        vertex1A,
        vertex1B,
        vertex2A,
        vertex2B,
        uvPairs[0].u,
        uvPairs[1].u,
        uvPairs[2].u
    );
    const zClipUV vGradient = zModel_Const::SolveTriScalarGradient2D(
        vertex0A,
        vertex0B,
        vertex1A,
        vertex1B,
        vertex2A,
        vertex2B,
        uvPairs[0].v,
        uvPairs[1].v,
        uvPairs[2].v
    );

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
/**
 * Reimplements 0x484860: zModel_Const::SolveTriScalarGradient2D
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
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
) {
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
} // namespace zModel_Const

namespace zDi {
    /**
     * Reimplements 0x484960: zDi::BuildPickCandidateForQueryPoint.
     * Provenance: address-backed reconstruction placed in the cls_di runtime
     * surface from current Binary Ninja behavior/global evidence.
     * Purpose: preserve the recovered pick-face helper behavior used by cls_di.
     */
    int __fastcall BuildPickCandidateForQueryPoint(
        zDiPartial * self,
        zClassDiPickCandidateEntry * outCandidate,
        const zVec3 *queryPoint
    ) {
        if (self == 0 || self->entryCount == 0) {
            return 0;
        }

        const zVec3 *vertices = self->verts;
        if ((self->flags & 0x08) != 0 && self->blendScale != 0.0f && self->blendVertCount != 0) {
            zMath_Vec3Array_AddScaled(
                g_zModel_SharedVec3ScratchA,
                self->verts,
                self->blendVerts,
                self->blendVertCount,
                self->blendScale
            );
            vertices = g_zModel_SharedVec3ScratchA;
        }

        TransformVerticesToSharedScratch(
            vertices,
            self->vertCount
        );

        {
            for (int entryIndex = 0; entryIndex < self->entryCount; ++entryIndex) {
                zDiEntryPartial *entry = &self->entries[entryIndex];
                const int vertexCount = (int)(entry->flagsAndIndexCount & 0xffu);
                const int *vertexIndices = (const int *)(entry->vertexIndices);
                CopyFaceVerticesToScratch(
                    g_zModel_SharedVec3ScratchB,
                    vertexIndices,
                    (unsigned int)(vertexCount)
                );

                if (zClass_cls_di::TryGetPolygonHitAtQueryXZ(
                        outCandidate,
                        g_zClass_DiFaceVertexScratch4,
                        queryPoint->x,
                        queryPoint->z,
                        vertexCount
                    ) != 0 &&
                    outCandidate->hitPos.y <= queryPoint->y) {
                    memcpy(
                        &outCandidate->variantTag,
                        &entry->variantTagInitialized,
                        sizeof(outCandidate->variantTag)
                    );
                    outCandidate->scenePayload = entry->material;
                    return 1;
                }
            }
        }

        return 0;
    }
} // namespace zDi

namespace zModelConst {
    /**
     * Reimplements 0x484b70: zModelConst::AddFaceToPlayerProbeSampleBuckets.
     * Provenance: address-backed reconstruction placed in the cls_di runtime
     * surface from current Binary Ninja behavior/global evidence.
     * Purpose: preserve the recovered pick-face helper behavior used by cls_di.
     */
    void __fastcall AddFaceToPlayerProbeSampleBuckets(
        zClass_NodePartial * node,
        PlayerProbeSampleCandidateBuffer * outputBuckets,
        const zVec3 *samplePoints,
        const int *sampleMaskSeeds,
        int samplePointCount,
        float maxProjectedY,
        const zVec3 *polygonVertices,
        const zModel_PickFaceEntry *faceEntry
    ) {
        zVec3 normal;
        zMath_Vec3_TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &normal
        );
        if (normal.y <= 0.0f) {
            return;
        }

        int activeFlags[0x20];
        for (int i = 0; i < samplePointCount; ++i) {
            activeFlags[i] = sampleMaskSeeds[i];
        }

        int anyActive = 1;
        const int vertexCount = (int)(faceEntry->flagsAndVertexCount & 0xffu);
        for (int edgeEnd = vertexCount - 1; edgeEnd >= 0 && anyActive != 0; --edgeEnd) {
            const int edgeStart = edgeEnd == vertexCount - 1 ? 0 : edgeEnd + 1;
            const zVec3 *start = &polygonVertices[edgeStart];
            const zVec3 *end = &polygonVertices[edgeEnd];
            const float dx = end->x - start->x;
            const float dz = start->z - end->z;

            anyActive = 0;
            for (int sampleIndex = 0; sampleIndex < samplePointCount; ++sampleIndex) {
                if (activeFlags[sampleIndex] != 0) {
                    const zVec3 *point = &samplePoints[sampleIndex];
                    const float edgeTest = (point->x - end->x) * dz + (point->z - end->z) * dx;
                    activeFlags[sampleIndex] = edgeTest > -0.0001f ? 1 : 0;
                    if (activeFlags[sampleIndex] != 0) {
                        anyActive = 1;
                    }
                }
            }
        }

        if (anyActive == 0 || samplePointCount <= 0) {
            return;
        }

        const float invNormalY = 1.0f / normal.y;
        const float xSlope = -normal.x * invNormalY;
        const float zSlope = -normal.z * invNormalY;
        for (int sampleIndex = 0; sampleIndex < samplePointCount; ++sampleIndex) {
            if (activeFlags[sampleIndex] != 0) {
                PlayerProbeSampleCandidateBuffer *bucket = &outputBuckets[sampleIndex];
                if (bucket->candidateCount < 0x20) {
                    zClassDiPickCandidateEntry *entry = &bucket->entries[bucket->candidateCount];
                    entry->surfaceNormal = normal;
                    entry->hitPos.y =
                        (samplePoints[sampleIndex].z - polygonVertices[0].z) * zSlope +
                        (samplePoints[sampleIndex].x - polygonVertices[0].x) * xSlope +
                        polygonVertices[0].y;
                    if (entry->hitPos.y <= maxProjectedY) {
                        entry->node = node;
                        entry->variantTag = faceEntry->variantTag;
                        entry->scenePayload = faceEntry->scenePayload;
                        ++bucket->candidateCount;
                    }
                }
            }
        }
    }
} // namespace zModelConst

namespace zClass_cls_di {
    /**
     * Reimplements 0x484e00: zClass_cls_di::PickTestMeshAtQueryXZ.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    void __fastcall PickTestMeshAtQueryXZ(
        zClass_NodePartial * node,
        zModel_PickFaceData * faceData,
        const zVec3 *samplePoints,
        const int *sampleMaskSeeds,
        int samplePointCount,
        float maxProjectedY,
        PlayerProbeSampleCandidateBuffer *outputBuckets
    ) {
        if (faceData == 0 || faceData->faceCount == 0) {
            return;
        }

        const zVec3 *vertices = faceData->baseVertices;
        if ((faceData->flags & 0x08) != 0 && faceData->morphWeight != 0.0f &&
            faceData->morphVertexCount != 0) {
            zMath_Vec3Array_AddScaled(
                g_zModel_SharedVec3ScratchA,
                faceData->baseVertices,
                faceData->morphVertices,
                faceData->morphVertexCount,
                faceData->morphWeight
            );
            vertices = g_zModel_SharedVec3ScratchA;
        }

        TransformVerticesToSharedScratch(
            vertices,
            faceData->vertexCount
        );

        for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
            const zModel_PickFaceEntry *face = &faceData->faces[faceIndex];
            const int vertexCount = (int)(face->flagsAndVertexCount & 0xffu);
            CopyFaceVerticesToScratch(
                g_zModel_SharedVec3ScratchB,
                face->vertexIndices,
                (unsigned int)(vertexCount)
            );
            zModelConst::AddFaceToPlayerProbeSampleBuckets(
                node,
                outputBuckets,
                samplePoints,
                sampleMaskSeeds,
                samplePointCount,
                maxProjectedY,
                g_zClass_DiFaceVertexScratch4,
                face
            );
        }
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x484fc0: zClass_cls_di::AppendPickCandidatesForFace.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall AppendPickCandidatesForFace(
        const zModel_PickFaceData *faceData,
        zClassDiPickCandidateEntry *candidate,
        const zVec3 *segmentStart,
        const zVec3 *segmentEnd
    ) {
        if (faceData == 0 || faceData->faceCount == 0) {
            return 0;
        }

        const zVec3 *vertices = faceData->baseVertices;
        if ((faceData->flags & 8) != 0 && faceData->morphWeight != 0.0f &&
            faceData->morphVertexCount != 0) {
            zMath_Vec3Array_AddScaled(
                g_zModel_SharedVec3ScratchA,
                faceData->baseVertices,
                faceData->morphVertices,
                faceData->morphVertexCount,
                faceData->morphWeight
            );
            vertices = g_zModel_SharedVec3ScratchA;
        }

        zVec3 queryPoint = {0};
        zVec3 localSegmentEnd = {0};
        if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {
            queryPoint = *segmentStart;
            localSegmentEnd = *segmentEnd;
        } else {
            queryPoint = TransformWorldPointToModel(segmentStart);
            localSegmentEnd = TransformWorldPointToModel(segmentEnd);
        }

        {
            for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
                const zModel_PickFaceEntry *face = &faceData->faces[faceIndex];
                const unsigned int flagsAndVertexCount = face->flagsAndVertexCount;
                const unsigned int vertexCount = flagsAndVertexCount & 0xffu;
                CopyFaceVerticesToScratch(
                    vertices,
                    face->vertexIndices,
                    vertexCount
                );

                const int cullBackface = (int)((flagsAndVertexCount >> 8) & 1u);
                int hit = 0;
                if ((face->scenePayload->flags & kPickFaceTexturedDamageMaskFlag) != 0) {
                    zVec2 outUv = {0};
                    hit = BuildPickCandidateForSegmentVsPolygonWithUv(
                        candidate,
                        &queryPoint,
                        &localSegmentEnd,
                        g_zClass_DiFaceVertexScratch4,
                        face->faceUvData,
                        &outUv,
                        (int)(vertexCount),
                        cullBackface
                    );
                } else {
                    hit = BuildPickCandidateForSegmentVsPolygon(
                        candidate,
                        &queryPoint,
                        &localSegmentEnd,
                        g_zClass_DiFaceVertexScratch4,
                        (int)(vertexCount),
                        cullBackface
                    );
                }

                if (hit == 0) {
                    continue;
                }

                candidate->scenePayload = face->scenePayload;
                if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
                    candidate->hitPos = TransformModelPointToWorld(&candidate->hitPos);
                    candidate->surfaceNormal =
                        TransformModelVectorToWorld(&candidate->surfaceNormal);
                }

                return 1;
            }
        }

        return 0;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x485380: zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentVsBBoxFaces(
        const zBBoxCorners *bboxCorners,
        zClassDiPickCandidateEntry *candidate,
        const zVec3 *segmentStart,
        const zVec3 *segmentEnd
    ) {
        candidate->scenePayload = 0;

        if (TestBBoxFace(candidate, segmentStart, segmentEnd, 0, 4, 7, 3, bboxCorners) ||
            TestBBoxFace(
                candidate,
                segmentStart,
                segmentEnd,
                0,
                1,
                5,
                4,
                bboxCorners
            ) ||
            TestBBoxFace(
                candidate,
                segmentStart,
                segmentEnd,
                5,
                1,
                2,
                6,
                bboxCorners
            ) ||
            TestBBoxFace(
                candidate,
                segmentStart,
                segmentEnd,
                7,
                6,
                2,
                3,
                bboxCorners
            ) ||
            TestBBoxFace(
                candidate,
                segmentStart,
                segmentEnd,
                0,
                3,
                2,
                1,
                bboxCorners
            ) ||
            TestBBoxFace(
                candidate,
                segmentStart,
                segmentEnd,
                4,
                5,
                6,
                7,
                bboxCorners
            )) {
            return 1;
        }

        return 0;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x4856d0: zClass_cls_di::TryGetPolygonHitAtQueryXZ.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall TryGetPolygonHitAtQueryXZ(
        zClassDiPickCandidateEntry * candidate,
        const zVec3 *polygonVertices,
        float queryX,
        float queryZ,
        int vertexCount
    ) {
        {
            for (int currentIndex = 0; currentIndex < vertexCount; ++currentIndex) {
                const int previousIndex = currentIndex == 0 ? vertexCount - 1 : currentIndex - 1;
                const zVec3 *previous = &polygonVertices[previousIndex];
                const zVec3 *current = &polygonVertices[currentIndex];
                const float edge = (queryX - previous->x) * (current->z - previous->z) +
                                   (queryZ - previous->z) * (previous->x - current->x);
                if (edge <= -0.0001f) {
                    return 0;
                }
            }
        }

        zMath_Vec3_TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &candidate->surfaceNormal
        );

        if (candidate->surfaceNormal.y == 0.0f) {
            candidate->hitPos.y = polygonVertices[0].y;
            return 1;
        }

        candidate->hitPos.y = polygonVertices[0].y -
                              ((queryX - polygonVertices[0].x) * candidate->surfaceNormal.x +
                                  (queryZ - polygonVertices[0].z) * candidate->surfaceNormal.z) /
                                  candidate->surfaceNormal.y;
        return 1;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x4857f0: zClass_cls_di::BuildPickCandidateForSegmentVsPolygon.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidateForSegmentVsPolygon(
        zClassDiPickCandidateEntry * candidate,
        const zVec3 *segmentStart,
        const zVec3 *segmentEnd,
        const zVec3 *polygonVertices,
        int vertexCount,
        int cullBackface
    ) {
        return BuildPickCandidateForSegmentVsPolygonCore(
                   candidate,
                   segmentStart,
                   segmentEnd,
                   polygonVertices,
                   vertexCount,
                   cullBackface,
                   0
               )
                   ? 1
                   : 0;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x485d10: zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidateForSegmentVsPolygonWithUv(
        zClassDiPickCandidateEntry * candidate,
        const zVec3 *segmentStart,
        const zVec3 *segmentEnd,
        const zVec3 *polygonVertices,
        const zModel_PickFaceUvData *faceUvData,
        zVec2 *outUv,
        int vertexCount,
        int cullBackface
    ) {
        int dominantAxis = 0;
        if (!BuildPickCandidateForSegmentVsPolygonCore(
                candidate,
                segmentStart,
                segmentEnd,
                polygonVertices,
                vertexCount,
                cullBackface,
                &dominantAxis
            )) {
            return 0;
        }

        SolvePickCandidateUvForProjectedPlane(
            candidate,
            polygonVertices,
            faceUvData,
            outUv,
            dominantAxis
        );
        OptCatalog_SetDamageMaskUv(
            outUv->x,
            outUv->y
        );
        return 1;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x486290: zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygon.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentBatchVsPolygon(
        zClass_NodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch,
        int *activeMask,
        int segmentCount,
        zVec3 *polygonVertices,
        zModel_PickFaceEntry *faceEntry
    ) {
        int *localActive = segmentCount > 0 ? (int *)(_alloca(sizeof(int) * segmentCount)) : 0;
        for (int i = 0; i < segmentCount; ++i) {
            localActive[i] = activeMask[i];
        }

        zVec3 normal;
        zMath_Vec3_TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &normal
        );

        const int cullBackface = (int)((faceEntry->flagsAndVertexCount >> 8) & 1u);
        int anyActive = 0;
        for (int planeIndex = 0; planeIndex < segmentCount; ++planeIndex) {
            if (localActive[planeIndex] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[planeIndex];
            if (buffer->candidateCount >= kMaxPickCandidates ||
                !BuildBatchSegmentPlaneHit(
                    &buffer->entries[buffer->candidateCount],
                    &segmentEndpointsByBatch[planeIndex],
                    polygonVertices,
                    &normal,
                    cullBackface
                )) {
                localActive[planeIndex] = 0;
                continue;
            }

            anyActive = 1;
        }

        if (anyActive == 0) {
            return 0;
        }

        const int vertexCount = (int)(faceEntry->flagsAndVertexCount & 0xffu);
        for (int polygonIndex = 0; polygonIndex < segmentCount; ++polygonIndex) {
            if (localActive[polygonIndex] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[polygonIndex];
            const zClassDiPickCandidateEntry *entry = &buffer->entries[buffer->candidateCount];
            localActive[polygonIndex] =
                PointInProjectedPolygon(
                    polygonVertices,
                    vertexCount,
                    &entry->hitPos,
                    &normal
                ) ? 1
                                                                                               : 0;
        }

        anyActive = 0;
        for (int appendIndex = 0; appendIndex < segmentCount; ++appendIndex) {
            if (localActive[appendIndex] != 0) {
                anyActive = 1;
                AppendBatchPolygonCandidate(
                    candidateOwner,
                    &outCandidateBuffersBySegment[appendIndex],
                    &normal,
                    faceEntry
                );
            }
        }

        return anyActive;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x4869a0:
     * zClass_cls_di::BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence for the expanded raycast/filter runtime slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
        zClass_NodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch,
        int *activeMask,
        int segmentCount,
        zVec3 *polygonVertices,
        zModel_PickFaceUvData *faceUvData,
        zVec2 *scratchUv,
        zModel_PickFaceEntry *faceEntry
    ) {
        int *localActive = segmentCount > 0 ? (int *)(_alloca(sizeof(int) * segmentCount)) : 0;
        for (int i = 0; i < segmentCount; ++i) {
            localActive[i] = activeMask[i];
        }

        zVec3 normal;
        zMath_Vec3_TriangleNormal(
            &polygonVertices[0],
            &polygonVertices[1],
            &polygonVertices[2],
            &normal
        );

        const int cullBackface = (int)((faceEntry->flagsAndVertexCount >> 8) & 1u);
        int anyActive = 0;
        for (int planeIndex = 0; planeIndex < segmentCount; ++planeIndex) {
            if (localActive[planeIndex] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[planeIndex];
            if (buffer->candidateCount >= kMaxPickCandidates ||
                !BuildBatchSegmentPlaneHit(
                    &buffer->entries[buffer->candidateCount],
                    &segmentEndpointsByBatch[planeIndex],
                    polygonVertices,
                    &normal,
                    cullBackface
                )) {
                localActive[planeIndex] = 0;
                continue;
            }

            anyActive = 1;
        }

        if (anyActive == 0) {
            return 0;
        }

        const int vertexCount = (int)(faceEntry->flagsAndVertexCount & 0xffu);
        for (int polygonIndex = 0; polygonIndex < segmentCount; ++polygonIndex) {
            if (localActive[polygonIndex] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer = &outCandidateBuffersBySegment[polygonIndex];
            const zClassDiPickCandidateEntry *entry = &buffer->entries[buffer->candidateCount];
            localActive[polygonIndex] =
                PointInProjectedPolygon(
                    polygonVertices,
                    vertexCount,
                    &entry->hitPos,
                    &normal
                ) ? 1
                                                                                               : 0;
        }

        const int damageMaskEnabled = OptCatalog_IsDamageMaskEnabled();
        const int dominantAxis = DominantAxis(&normal);
        anyActive = 0;
        for (int damageMaskIndex = 0; damageMaskIndex < segmentCount; ++damageMaskIndex) {
            if (localActive[damageMaskIndex] == 0) {
                continue;
            }

            PlayerProbeSampleCandidateBuffer *buffer =
                &outCandidateBuffersBySegment[damageMaskIndex];
            if (buffer->candidateCount >= kMaxPickCandidates) {
                continue;
            }

            if (damageMaskEnabled != 0) {
                const zClassDiPickCandidateEntry *entry = &buffer->entries[buffer->candidateCount];
                zClassDiPickCandidateEntry uvCandidate = {0};
                uvCandidate.hitPos = entry->hitPos;
                uvCandidate.surfaceNormal = normal;
                SolvePickCandidateUvForProjectedPlane(
                    &uvCandidate,
                    polygonVertices,
                    faceUvData,
                    scratchUv,
                    dominantAxis
                );
                OptCatalog_SetDamageMaskUv(
                    scratchUv->x,
                    scratchUv->y
                );
            }

            anyActive = 1;
            AppendBatchPolygonCandidate(
                candidateOwner,
                buffer,
                &normal,
                faceEntry
            );
        }

        return anyActive;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x487350: zClass_cls_di::FilterRegionsAgainstPolygon.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    void __fastcall FilterRegionsAgainstPolygon(
        zClass_NodePartial * candidateOwner,
        zModel_PickFaceData * faceData,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch,
        int *activeMask,
        int segmentCount,
        PlayerProbeSampleCandidateBuffer *outCandidateBuffersBySegment
    ) {
        if (faceData == 0 || faceData->faceCount == 0) {
            return;
        }

        const zVec3 *vertices = faceData->baseVertices;
        if ((faceData->flags & 0x08) != 0 && faceData->morphWeight != 0.0f &&
            faceData->morphVertexCount != 0) {
            zMath_Vec3Array_AddScaled(
                g_zModel_SharedVec3ScratchA,
                faceData->baseVertices,
                faceData->morphVertices,
                faceData->morphVertexCount,
                faceData->morphWeight
            );
            vertices = g_zModel_SharedVec3ScratchA;
        }

        TransformVerticesToSharedScratch(
            vertices,
            faceData->vertexCount
        );

        zVec2 scratchUv = {0.0f, 0.0f};
        for (int faceIndex = 0; faceIndex < faceData->faceCount; ++faceIndex) {
            zModel_PickFaceEntry *face = &faceData->faces[faceIndex];
            const unsigned int vertexCount = face->flagsAndVertexCount & 0xffu;
            CopyFaceVerticesToScratch(
                g_zModel_SharedVec3ScratchB,
                face->vertexIndices,
                vertexCount
            );

            if ((face->scenePayload->flags & kPickFaceBatchDamageMaskUvFlag) != 0) {
                BuildPickCandidatesForSegmentBatchVsPolygonWithDamageMaskUv(
                    candidateOwner,
                    outCandidateBuffersBySegment,
                    segmentEndpointsByBatch,
                    activeMask,
                    segmentCount,
                    g_zClass_DiFaceVertexScratch4,
                    face->faceUvData,
                    &scratchUv,
                    face
                );
            } else {
                BuildPickCandidatesForSegmentBatchVsPolygon(
                    candidateOwner,
                    outCandidateBuffersBySegment,
                    segmentEndpointsByBatch,
                    activeMask,
                    segmentCount,
                    g_zClass_DiFaceVertexScratch4,
                    face
                );
            }
        }
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x487540: zClass_cls_di::FilterRegionsAgainstPolygonWithDamageMaskUv.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterRegionsAgainstPolygonWithDamageMaskUv(
        zClass_NodePartial * candidateOwner,
        PlayerProbeSampleCandidateBuffer * outCandidateBuffersBySegment,
        zClass_DiSegmentEndpoints * segmentEndpointsByBatch,
        int *activeMask,
        int segmentCount,
        const zBBoxCorners *bboxCorners
    ) {
        zModel_PickFaceEntry faceEntry;
        memset(
            &faceEntry,
            0,
            sizeof(faceEntry)
        );
        faceEntry.flagsAndVertexCount = 4;

        int result = 0;
        if (TestSegmentBatchBBoxFace(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                bboxCorners,
                &faceEntry,
                0,
                4,
                7,
                3
            ) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                bboxCorners,
                &faceEntry,
                0,
                1,
                5,
                4
            ) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                bboxCorners,
                &faceEntry,
                1,
                2,
                6,
                5
            ) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                bboxCorners,
                &faceEntry,
                2,
                3,
                7,
                6
            ) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                bboxCorners,
                &faceEntry,
                0,
                3,
                2,
                1
            ) != 0) {
            result = 1;
        }
        if (TestSegmentBatchBBoxFace(
                candidateOwner,
                outCandidateBuffersBySegment,
                segmentEndpointsByBatch,
                activeMask,
                segmentCount,
                bboxCorners,
                &faceEntry,
                4,
                5,
                6,
                7
            ) != 0) {
            result = 1;
        }

        return result;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x487900: zClass_cls_di::FilterRegionsAgainstMeshFaces.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterRegionsAgainstMeshFaces(
        zVec3 * meshVertices,
        int faceCount
    ) {
        g_zModel_PointInPolygonVertexCount = 0;
        if (faceCount > 0x40) {
            return 0;
        }

        if (faceCount > 0) {
            {
                for (int vertexIndex = faceCount - 1; vertexIndex >= 0; --vertexIndex) {
                    const int nextIndex = vertexIndex == faceCount - 1 ? 0 : vertexIndex + 1;
                    g_zModel_PointInPolygonVertices[vertexIndex] = meshVertices[vertexIndex];

                    zVec3 *edgeNormal = &g_zModel_PointInPolygonEdgeNormals[vertexIndex];
                    edgeNormal->x = meshVertices[nextIndex].z - meshVertices[vertexIndex].z;
                    edgeNormal->y = 0.0f;
                    edgeNormal->z = meshVertices[vertexIndex].x - meshVertices[nextIndex].x;
                    zMath::Vec3Normalize(edgeNormal);
                }
            }
        }

        g_zModel_PointInPolygonVertexCount = faceCount;
        return 1;
    }
} // namespace zClass_cls_di

namespace zClass_cls_di {
    /**
     * Reimplements 0x4879c0: zClass_cls_di::FilterRegionsAgainstHexahedronFaces.
     * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
     * behavior/global evidence; native smoke coverage exercises the owner slice.
     * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
     */
    int __fastcall FilterRegionsAgainstHexahedronFaces(
        zVec3 * center,
        float radius
    ) {
        zVec3 *vertex = g_zModel_PointInPolygonVertices;
        zVec3 *edgeNormal = g_zModel_PointInPolygonEdgeNormals;

        for (int vertexIndex = 0; vertexIndex < g_zModel_PointInPolygonVertexCount; ++vertexIndex) {
            const float distance =
                (center->x - vertex->x) * edgeNormal->x + (center->z - vertex->z) * edgeNormal->z;
            if (distance < radius) {
                return 0;
            }

            ++vertex;
            ++edgeNormal;
        }

        return 1;
    }
} // namespace zClass_cls_di
