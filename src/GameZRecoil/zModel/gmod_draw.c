// zModel compilation unit between gmod_tag.c and gmod_clip.c, inferred from the
// retail object boundary [0x476460, 0x479ce0): its .rdata pooled constants
// [0x4d2a40, 0x4d2a80) repeat 1.0f that gmod_init.c (0x4d2a10) and gmod_clip.c
// (0x4d2a8c) pool separately. Original filename unresolved; gmod_draw.c is a
// provisional name (2026-10-02).

#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "recoil/recoil_types.h"
#include "zclass.h"
#include <ctype.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace
{
/**
 * Original source helper expression observed in zModel render point/lighting paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: transform one model-space point by the current zMath matrix.
 */
#define TransformPointByCurrentMatrix(point, out)                                                                      \
    do {                                                                                                               \
        const zMat4x3* const currentMatrix = (const zMat4x3*)(*zMath::g_currentMatrixPtrSlot);                         \
        (out).x = (point)->x * currentMatrix->xx + (point)->y * currentMatrix->yx + (point)->z * currentMatrix->zx     \
            + currentMatrix->posX;                                                                                     \
        (out).y = (point)->x * currentMatrix->xy + (point)->y * currentMatrix->yy + (point)->z * currentMatrix->zy     \
            + currentMatrix->posY;                                                                                     \
        (out).z = (point)->x * currentMatrix->xz + (point)->y * currentMatrix->yz + (point)->z * currentMatrix->zz     \
            + currentMatrix->posZ;                                                                                     \
    } while (0)

// Retail literal-pool double 0x3F70101029AA03B0, not the exact 1.0 / 255.0.
#define kVisibleContributionThreshold 0.003921569

/**
 * Original source helper expression observed in zModel render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: test whether graphics option flag bit 0 is enabled.
 */
#define ModelGraphicsFlagBit0Enabled() ((*gModel_pGraphicsFlags & 1) != 0)

/**
 * Original inline expression observed in zModel point and software render
 * paths (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: test whether a projected point lies inside the active projection clip bounds.
 */
#define ProjectedPointInClipBounds(point)                                                                              \
    (!((point).x < g_zVideo_ProjectClipLeft) && !((point).y < g_zVideo_ProjectClipTop)                                 \
        && !((point).x > g_zVideo_ProjectClipRight) && !((point).y > g_zVideo_ProjectClipBottom))

    typedef void(__fastcall * DrawPointColor16Proc)(
        zProjectedPoint * point,
        unsigned int packedColor16,
        int pointCount
    );
    typedef void(__fastcall * SubmitPolyFlatColor16Proc)(
        zVideo_XyzVertex * vertices,
        unsigned int packedColor16,
        int alpha,
        int renderParam,
        int vertexCount,
        int queueMode
    );
    typedef void(__fastcall * SubmitPolyColorAttrProc)(
        zVideo_XyzVertex * vertices,
        unsigned int packedColor16,
        zVideo_ColorRgbFloat* baseColor,
        float* attr1,
        float* attr0,
        float* attr2,
        int alpha,
        int vertexCount,
        unsigned int renderParam,
        int queueMode
    );
    typedef void(__fastcall * SubmitPolyRenderClassProc)(
        zVideo_XyzVertex * vertices,
        zVideo_TexCoord * texCoords,
        int vertexCount,
        zVideo_RenderClass* renderClass,
        unsigned int renderParam,
        float alpha,
        int queueMode
    );
    typedef void(__fastcall * SubmitPolygonProc)(
        zVideo_XyzVertex * vertices,
        zVideo_TexCoord * uvPairs,
        float* attr1,
        float* attr0,
        float* attr2,
        int vertexCount,
        zVideo_RenderClass* renderClass,
        unsigned int renderParam,
        float alpha,
        int queueMode
    );
    typedef void(__fastcall * SubmitPolygonLitProc)(
        zVideo_XyzVertex * vertices,
        zVideo_TexCoord * uvPairs,
        float* attr1,
        float* attr0,
        float* attr2,
        int vertexCount,
        zVideo_RenderClass* renderClass,
        unsigned int renderParam,
        float alpha,
        int queueMode
    );

/**
 * Recovered original helper expression in D:\Proj\GameZRecoil\zModel\zmodel.cpp.
 * No standalone retail function; observed callers are address-backed zModel
 * display-instance paths in this source file.
 * Purpose: return the display-instance pointer stored on a scene node.
 */
#define NodeDisplayInstance(node) ((node) != 0 ? (zDiPartial*)((node)->userDataOrDiRef) : 0)

/**
 * Original inline helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: prepare transformed display-instance vertices, including optional blend vertices.
 */
#define PrepareTransformedVertices(di)                                                                                 \
    do {                                                                                                               \
        if ((di)->verts != 0 && (di)->vertCount > 0) {                                                                 \
            if (((di)->flags & 8) != 0 && (di)->blendVerts != 0 && (di)->blendVertCount > 0                            \
                && (di)->blendScale != 0.0) {                                                                          \
                zMathVec3ArrayAddScaled(                                                                               \
                    g_zModel_TransformedVerts,                                                                         \
                    (di)->verts,                                                                                       \
                    (di)->blendVerts,                                                                                  \
                    (di)->blendVertCount,                                                                              \
                    (di)->blendScale                                                                                   \
                );                                                                                                     \
                if ((di)->vertCount > (di)->blendVertCount) {                                                          \
                    memcpy(                                                                                            \
                        &g_zModel_TransformedVerts[(di)->blendVertCount],                                              \
                        &(di)->verts[(di)->blendVertCount],                                                            \
                        (size_t)((di)->vertCount - (di)->blendVertCount) * sizeof(zVec3)                               \
                    );                                                                                                 \
                }                                                                                                      \
            } else {                                                                                                   \
                memcpy(g_zModel_TransformedVerts, (di)->verts, (size_t)((di)->vertCount) * sizeof(zVec3));             \
            }                                                                                                          \
            zMath::MatTransformPointBatchInPlace(g_zModel_TransformedVerts, (di)->vertCount);                          \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper observed in zModel hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: transform and normalize display-instance normals for per-vertex shading.
 */
#define PrepareTransformedNormals(di)                                                                                  \
    do {                                                                                                               \
        if (g_zModel_VertexShadingEnabled != 0 && (di)->normalCount > 0) {                                             \
            zVec3 origin = { 0 };                                                                                      \
            if (*zMath::g_currentMatrixIdentityFlagSlot != 0) {                                                        \
                memcpy(g_zModel_TransformedNormals, (di)->normals, (size_t)(di)->normalCount * sizeof(zVec3));         \
            } else {                                                                                                   \
                for (int normalIndex = 0; normalIndex < (di)->normalCount; ++normalIndex) {                            \
                    TransformPointByCurrentMatrix(                                                                     \
                        &(di)->normals[normalIndex],                                                                   \
                        g_zModel_TransformedNormals[normalIndex]                                                       \
                    );                                                                                                 \
                }                                                                                                      \
            }                                                                                                          \
            zMath::MatTransformPointBatchInPlace(&origin, 1);                                                          \
            for (int normalIndex = 0; normalIndex < (di)->normalCount; ++normalIndex) {                                \
                zVec3* normal = &g_zModel_TransformedNormals[normalIndex];                                             \
                normal->x -= origin.x;                                                                                 \
                normal->y -= origin.y;                                                                                 \
                normal->z -= origin.z;                                                                                 \
                zMath::Vec3Normalize(normal);                                                                          \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper observed in zModel polygon render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: gather an entry's transformed vertices into the clip scratch polygon.
 */
#define CopyEntryVerticesToScratch(di, entry, vertexCount, copied)                                                     \
    do {                                                                                                               \
        int* copyIndices = (int*)((entry)->vertexIndices);                                                             \
        (copied) = copyIndices != 0;                                                                                   \
        for (int copyIndex = 0; (copied) != 0 && copyIndex < (vertexCount); ++copyIndex) {                             \
            const int vertexIndex = copyIndices[copyIndex];                                                            \
            if (vertexIndex < 0 || vertexIndex >= (di)->vertCount) {                                                   \
                (copied) = 0;                                                                                          \
            } else {                                                                                                   \
                const zVec3& src = g_zModel_TransformedVerts[vertexIndex];                                             \
                g_Clip_PolyVertsScratch[copyIndex].x = src.x;                                                          \
                g_Clip_PolyVertsScratch[copyIndex].y = src.y;                                                          \
                g_Clip_PolyVertsScratch[copyIndex].z = src.z;                                                          \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

    /**
     * Original static helper observed in zModel polygon render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: gather an entry's transformed normals for the current polygon when present.
     */
    void CopyEntryNormalsToCurrent(zDiPartial * di, zDiEntryPartial * entry, int vertexCount)
    {
        g_zModel_CurrentPolyNormals = 0;
        if (g_zModel_VertexShadingEnabled == 0 || di->normalCount <= 0 || (entry->flagsAndIndexCount & 0x0200) == 0
            || entry->normalIndices == 0) {
            return;
        }

        int* indices = (int*)(entry->normalIndices);
        for (int i = 0; i < vertexCount; ++i) {
            const int normalIndex = indices[i];
            if (normalIndex < 0 || normalIndex >= di->normalCount) {
                g_zModel_CurrentPolyNormals = 0;
                return;
            }
            g_zModel_CurrentPolyNormalsStorage[i] = g_zModel_TransformedNormals[normalIndex];
        }
        g_zModel_CurrentPolyNormals = g_zModel_CurrentPolyNormalsStorage;
    }

    /**
     * Original static helper observed in zModel polygon render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: clear the three clip-attribute arrays for a polygon.
     */
    void ClearPolyAttributes(int vertexCount)
    {
        for (int i = 0; i < vertexCount; ++i) {
            g_Clip_PolyAttr0[i] = 0.0f;
            g_Clip_PolyAttr1[i] = 0.0f;
            g_Clip_PolyAttr2[i] = 0.0f;
        }
    }

    /**
     * Original static helper observed in zModel polygon render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: fill the three clip-attribute arrays with one constant value.
     */
    void FillPolyAttributes(float value, int vertexCount)
    {
        for (int i = 0; i < vertexCount; ++i) {
            g_Clip_PolyAttr0[i] = value;
            g_Clip_PolyAttr1[i] = value;
            g_Clip_PolyAttr2[i] = value;
        }
    }

    /**
     * Original static helper observed in zModel polygon render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: build fog/light clip attributes for a polygon and fill defaults when unused.
     */
    int BuildPolyAttributes(const zVec3* surfaceNormal, int vertexCount)
    {
        int attrFlags = 0;
        int lightingMode = 0;

        if (gModel_FogEnabled != 0) {
            attrFlags |= zModel_Light::BuildAttr1Falloff(vertexCount, &lightingMode) != 0 ? 1 : 0;
        }

        if (gModel_HasActiveLights != 0) {
            int lightFlags = 0;
            attrFlags
                |= zModel_Light::SetActiveLights((zVec3*)(surfaceNormal), vertexCount, &lightFlags, &lightingMode, 0)
                    != 0
                ? 1
                : 0;
        }

        if (attrFlags == 0) {
            FillPolyAttributes(1.0f, vertexCount);
        }
        return attrFlags;
    }

/**
 * Original inline helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: compute the polygon facing normal and apply backface/show-backface culling.
 * Keep the edge vectors as aggregates: VC5 scalar-temporary reuse corrupts
 * the normal Z calculation when these are six independent float locals.
 */
#define ComputeSurfaceNormalAndCull(vertexCount, showBackFace, outNormal, outScanConvertMode, visible)                 \
    do {                                                                                                               \
        (visible) = 0;                                                                                                 \
        if ((vertexCount) >= 3) {                                                                                      \
            if ((outScanConvertMode) != 0) {                                                                           \
                *((int*)(outScanConvertMode)) = 1;                                                                     \
            }                                                                                                          \
            const zClipVert& v0 = g_Clip_PolyVertsScratch[0];                                                          \
            const zClipVert& v1 = g_Clip_PolyVertsScratch[1];                                                          \
            const zClipVert& v2 = g_Clip_PolyVertsScratch[2];                                                          \
            const zVec3 edgeA = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };                                             \
            const zVec3 edgeB = { v0.x - v1.x, v0.y - v1.y, v0.z - v1.z };                                             \
            (outNormal)->x = edgeB.z * edgeA.y - edgeB.y * edgeA.z;                                                    \
            (outNormal)->y = edgeB.x * edgeA.z - edgeB.z * edgeA.x;                                                    \
            (outNormal)->z = edgeB.y * edgeA.x - edgeB.x * edgeA.y;                                                    \
            const float facing = (outNormal)->x * v0.x + (outNormal)->y * v0.y + (outNormal)->z * v0.z;                \
            if (facing < -g_zModel_BFETolerance) {                                                                     \
                (visible) = 1;                                                                                         \
            } else if ((showBackFace) != 0 && facing > g_zModel_BFETolerance) {                                        \
                (outNormal)->x = -(outNormal)->x;                                                                      \
                (outNormal)->y = -(outNormal)->y;                                                                      \
                (outNormal)->z = -(outNormal)->z;                                                                      \
                if ((outScanConvertMode) != 0) {                                                                       \
                    *((int*)(outScanConvertMode)) = 0;                                                                 \
                }                                                                                                      \
                (visible) = 1;                                                                                         \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper observed in zModel textured polygon render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: copy an entry's UV pairs into the current clip UV scratch array.
 */
#define CopyEntryUvsToScratch(entry, vertexCount)                                                                      \
    do {                                                                                                               \
        if (g_Clip_PolyUvs != 0 && (entry)->uvPairs != 0) {                                                            \
            zClipUV* sourceUvs = (zClipUV*)((entry)->uvPairs);                                                         \
            for (int uvIndex = 0; uvIndex < (vertexCount); ++uvIndex) {                                                \
                g_Clip_PolyUvs[uvIndex] = sourceUvs[uvIndex];                                                          \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: project the current scratch polygon into the clip vertex buffer.
 */
#define ProjectScratchToClipVerts(vertexCount)                                                                         \
    do {                                                                                                               \
        zMath::ProjectPointBatch(                                                                                      \
            (const zVec3*)g_Clip_PolyVertsScratch,                                                                     \
            (zProjectedPoint*)g_Clip_PolyVerts,                                                                        \
            (vertexCount)                                                                                              \
        );                                                                                                             \
    } while (0)

    /**
     * Original static helper observed in zModel polygon render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: apply the encoded depth bias to projected clip vertices.
     */
    void ApplyDepthBiasToProjectedVerts(unsigned int drawFlags, int vertexCount)
    {
        const float depthScale = (float)((short)(drawFlags & 0xffff)) * g_zRndr_InverseZTolerance + 1.0f;
        for (int i = 0; i < vertexCount; ++i) {
            g_Clip_PolyVerts[i].z *= depthScale;
        }
    }

    /**
     * Original static helper observed in zModel untextured polygon render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: clip and project a polygon without UV coordinates.
     */
    int ClipAndProjectNoUv(zClipRectPartial * clipRect, int* vertexCount, int hasAttributes)
    {
        if (hasAttributes != 0) {
            if (zClipRect::ClipPolyZRange_NoUV_WithAttribs(clipRect, vertexCount) == 0) {
                return 0;
            }
        } else if (zClipRect::ClipPolyZRange_NoUV(clipRect, vertexCount) == 0) {
            return 0;
        }

        ProjectScratchToClipVerts(*vertexCount);

        if (hasAttributes != 0) {
            return zClipRect::ClipPoly_NoUV_WithAttr012_Alt(clipRect, vertexCount);
        }
        return zClipRect::ClipPoly_NoUV(clipRect, vertexCount);
    }

    /**
     * Original static helper observed in zModel textured software render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: clip, project, and perspective-correct a textured polygon.
     */
    int ClipAndProjectUv(zClipRectPartial * clipRect, int* vertexCount, int hasAttributes)
    {
        if (hasAttributes != 0) {
            if (zClipRect::ClipPolyZRange_WithAttr012(clipRect, vertexCount) == 0) {
                return 0;
            }
        } else if (zClipRect::ClipPolyNearZ(clipRect, vertexCount) == 0) {
            return 0;
        }

        for (int i = 0; i < *vertexCount; ++i) {
            g_Clip_PolyUvs[i].u *= g_Clip_PolyVertsScratch[i].z;
            g_Clip_PolyUvs[i].v *= g_Clip_PolyVertsScratch[i].z;
        }

        ProjectScratchToClipVerts(*vertexCount);
        for (int i_79 = 0; i_79 < *vertexCount; ++i_79) {
            if (g_Clip_PolyVerts[i_79].z != 0.0f) {
                g_Clip_PolyUvs[i_79].u /= g_Clip_PolyVerts[i_79].z;
                g_Clip_PolyUvs[i_79].v /= g_Clip_PolyVerts[i_79].z;
            }
        }

        if (hasAttributes != 0) {
            return zClipRect::ClipPoly_WithAttr012(clipRect, vertexCount);
        }
        return zClipRect::ClipPoly(clipRect, vertexCount);
    }

    /**
     * Original static helper observed in zModel hardware textured render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: multiply current clip UVs by projected reciprocal depth.
     */
    void MultiplyUvsByProjectedReciprocalZ(int vertexCount)
    {
        for (int i = 0; i < vertexCount; ++i) {
            g_Clip_PolyUvs[i].u *= g_Clip_PolyVerts[i].z;
            g_Clip_PolyUvs[i].v *= g_Clip_PolyVerts[i].z;
        }
    }

    /**
     * Original static helper observed in zModel hardware submit paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: convert clipped reciprocal-depth UVs back to submit-time perspective UVs.
     */
    void FillPerspectiveUvsForHardwareSubmit(zClipUV * outUvs, int vertexCount)
    {
        for (int i = 0; i < vertexCount; ++i) {
            if (g_Clip_PolyVerts[i].z != 0.0f) {
                const float depth = 1.0f / g_Clip_PolyVerts[i].z;
                outUvs[i].u = g_Clip_PolyUvs[i].u * depth;
                outUvs[i].v = g_Clip_PolyUvs[i].v * depth;
            } else {
                outUvs[i].u = g_Clip_PolyUvs[i].u;
                outUvs[i].v = g_Clip_PolyUvs[i].v;
            }
        }
    }

    /**
     * Original static helper observed in zModel hardware clip paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: initialize attributes for clip-generated vertices from the first source vertex.
     */
    void FillConstantAttrsForGeneratedClipVerts(int previousCount, int vertexCount)
    {
        for (int i = previousCount; i < vertexCount; ++i) {
            g_Clip_PolyAttr0[i] = g_Clip_PolyAttr0[0];
            g_Clip_PolyAttr1[i] = g_Clip_PolyAttr1[0];
            g_Clip_PolyAttr2[i] = g_Clip_PolyAttr2[0];
        }
    }

    /**
     * Original static helper observed in zModel hardware textured render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: clip UVs as u*rhw, then submit u/rhw on the DD3D path.
     */
    int ClipAndProjectHardwareUv(zClipRectPartial * clipRect, int* vertexCount, int hasAttributes)
    {
        if ((clipRect->flags & 0x30) != 0) {
            if (hasAttributes != 0) {
                if (zClipRect::ClipPolyZRange_WithAttr012(clipRect, vertexCount) == 0) {
                    return 0;
                }
            } else if (zClipRect::ClipPolyNearZ(clipRect, vertexCount) == 0) {
                return 0;
            }
        }

        ProjectScratchToClipVerts(*vertexCount);
        MultiplyUvsByProjectedReciprocalZ(*vertexCount);

        if ((clipRect->flags & 0x0f) != 0) {
            const int previousCount = *vertexCount;
            if (hasAttributes != 0) {
                if (zClipRect::ClipPoly_WithAttr012(clipRect, vertexCount) == 0) {
                    return 0;
                }
            } else if (zClipRect::ClipPoly(clipRect, vertexCount) == 0) {
                return 0;
            }

            if (hasAttributes == 0 && previousCount < *vertexCount) {
                FillConstantAttrsForGeneratedClipVerts(previousCount, *vertexCount);
            }
        }

        return 1;
    }

/**
 * Original inline helper observed in zModel software textured render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: clip and project a software textured polygon with optional vertex shade.
 */
#define ClipAndProjectSoftwareTextured(clipRect, vertexCount, hasPerVertexShade, clipped)                              \
    do {                                                                                                               \
        (clipped) = zClipRect::ClipPolyNearZ_WithAttr0((clipRect), (vertexCount));                                     \
        if ((clipped) != 0) {                                                                                          \
            ProjectScratchToClipVerts(*(vertexCount));                                                                 \
            (clipped) = zClipRect::ClipPoly_NoUV_WithAttr0_Alt((clipRect), (vertexCount));                             \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: reject projected polygons whose screen-space area is below the configured threshold.
 */
#define RejectProjectedSmallPoly(vertexCount, rejected)                                                                \
    do {                                                                                                               \
        if ((vertexCount) <= 0) {                                                                                      \
            (rejected) = 1;                                                                                            \
        } else {                                                                                                       \
            float twiceArea = 0.0f;                                                                                    \
            zClipVert* previous = &g_Clip_PolyVerts[(vertexCount) - 1];                                                \
            for (int areaIndex = 0; areaIndex < (vertexCount); ++areaIndex) {                                          \
                zClipVert* const current = &g_Clip_PolyVerts[areaIndex];                                               \
                twiceArea += current->y * previous->x - previous->y * current->x;                                      \
                previous = current;                                                                                    \
            }                                                                                                          \
            if (twiceArea < 0.0f) {                                                                                    \
                twiceArea = -twiceArea;                                                                                \
            }                                                                                                          \
            (rejected) = twiceArea < gModel_SmallPolyRejectArea2x ? 1 : 0;                                             \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper observed in zModel software triangle render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: copy the first three projected clip vertices into a triangle buffer.
 */
#define CopyProjectedTriVerts(triVerts)                                                                                \
    do {                                                                                                               \
        (triVerts)[0] = *(zVec3*)(&g_Clip_PolyVerts[0]);                                                               \
        (triVerts)[1] = *(zVec3*)(&g_Clip_PolyVerts[1]);                                                               \
        (triVerts)[2] = *(zVec3*)(&g_Clip_PolyVerts[2]);                                                               \
    } while (0)

/**
 * Original inline helper observed in zModel software render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: remap projected vertices through the alternate clip-space mapping.
 */
#define RemapAltProjectedVerts(verts, vertexCount)                                                                     \
    do {                                                                                                               \
        for (int remapIndex = 0; remapIndex < (vertexCount); ++remapIndex) {                                           \
            zClipAlt::RemapPointXYInPlace(&(verts)[remapIndex].x);                                                     \
        }                                                                                                              \
    } while (0)

/**
 * Original inline helper observed in zModel software render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: set the renderer inverse-depth bias and scale from draw flags.
 */
#define ApplySoftwareDepthScale(drawFlags)                                                                             \
    do {                                                                                                               \
        zRndr::g_inverseDepthBias = 0.0f;                                                                              \
        zRndr::g_inverseDepthScale = (float)((short)((drawFlags) & 0xffff)) * g_zRndr_InverseZTolerance + 1.0f;        \
    } while (0)

/**
 * Original source helper expression observed in zModel material render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: convert material alpha flags to the current integer render alpha.
 */
#define MaterialAlphaInt(material) ((int)((float)((int)((material)->flags & 0xff)) * gModel_RenderAlphaScaleCurrent))

    /**
     * Original static helper observed in zModel material render paths
     * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
     * Purpose: convert material alpha flags to normalized floating render alpha.
     */
    float MaterialAlphaFloat(const zModel_MaterialPartial* material)
    {
        return (float)(MaterialAlphaInt(material)) * (1.0f / 255.0f);
    }

    /**
     * Recovered original static helper in D:\Proj\GameZRecoil\zModel\zmodel.cpp.
     * No standalone retail function; observed callers are address-backed zModel
     * material render paths in this source file.
     * Purpose: return the current render-class pointer for a material texture entry.
     */
    zVideo_RenderClass* MaterialRenderClass(zModel_MaterialPartial * material)
    {
        if (material == 0 || material->currentTextureDirectoryEntry == 0) {
            return 0;
        }
        return (zVideo_RenderClass*)(material->currentTextureDirectoryEntry->texture);
    }

/**
 * Original source helper expression observed in zModel_Display projected-sphere callers
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: truncate a projected floating-point coordinate to integer screen space.
 */
#define TruncateToInt(value) ((int)(value))

} // namespace

/**
 * Recovered helper: zVideoSubtractVec3.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this zVec3 subtraction pattern for near, camera, and far
 * frustum-center deltas.
 * Purpose: subtract one zVec3 from another and return the delta.
 */
static zVec3 zVideoSubtractVec3(zVec3* lhs, zVec3* rhs)
{
    zVec3 delta;
    delta.x = lhs->x - rhs->x;
    delta.y = lhs->y - rhs->y;
    delta.z = lhs->z - rhs->z;
    return delta;
}

/**
 * Recovered helper: zVideoDotVec3.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this x/y/z multiply-add dot-product pattern for every
 * frustum plane comparison.
 * Purpose: compute the dot product of two zVec3 values.
 */
static float zVideoDotVec3(zVec3* lhs, zVec3* rhs)
{
    return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z;
}

/**
 * Recovered helper: zVideoTestSpherePlane.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this sphere/plane reject-or-clip test for the side and far
 * frustum planes.
 * Purpose: test one sphere against one frustum plane and update the clip mask.
 */
static int zVideoTestSpherePlane(zVec3* delta, zVec3* normal, float radius, int planeBit, int* clipMaskInOut)
{
    const float dot = zVideoDotVec3(delta, normal);
    if (-radius >= dot) {
        return planeBit;
    }

    if (dot < radius) {
        *clipMaskInOut |= planeBit;
    }

    return 0;
}

namespace zModel
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-setbackfaceeliminationtolerancescalar
     * @recoil-artifact defines .text recoil:function:0x476460: zModel::SetBackfaceEliminationToleranceScalar
     * @recoil-match byte
     *
     * Purpose: store the global backface-elimination tolerance scalar.
     */
    void __stdcall SetBackfaceEliminationToleranceScalar(float scalar)
    {
        g_zModel_BFETolerance = scalar;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-getbackfaceeliminationtolerancescalar
     * @recoil-artifact defines .text recoil:function:0x476470: zModel::GetBackfaceEliminationToleranceScalar
     * @recoil-match byte
     *
     * Purpose: return the current global backface-elimination tolerance scalar.
     */
    float __cdecl GetBackfaceEliminationToleranceScalar()
    {
        return g_zModel_BFETolerance;
    }
} // namespace zModel

namespace zMath
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-draw-zmath-projectpointandclamptoscreenclip
     * @recoil-artifact defines .text recoil:function:0x476480: zMath::ProjectPointAndClampToScreenClip.
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
     *
     *
     * Purpose: transforms one point through camera scratch B, projects it, and
     * clamps it to the active screen clip rectangle.
     * Placement: retail places it between gmod_draw.c's 0x476470 and 0x4766a0.
     */
    int __fastcall ProjectPointAndClampToScreenClip(const zVec3* srcPoint, zVec3* dstPoint)
    {
        int result;
        zMat4x3 slotBuffer;
        MatStackPushPtr((float*)(&slotBuffer));
        MatLoadCameraScratchB();
        ZMTH_MAT_TRANSFORM_POINT_BATCH(srcPoint, dstPoint, 1);
        MatStackPopPtr();

        if (dstPoint->z > gClipRect_Primary.zMin) {
            ProjectPointBatch(dstPoint, (zProjectedPoint*)(dstPoint), 1);

            result = 0;
            if (dstPoint->x < g_zVideo_ProjectClipLeft) {
                dstPoint->x = g_zVideo_ProjectClipLeft;
                result = 1;
            } else if (dstPoint->x > g_zVideo_ProjectClipRight) {
                dstPoint->x = g_zVideo_ProjectClipRight;
                result = 2;
            }

            if (dstPoint->y < g_zVideo_ProjectClipTop) {
                dstPoint->y = g_zVideo_ProjectClipTop;
                result = 4;
            } else if (dstPoint->y >= g_zVideo_ProjectClipBottom) {
                dstPoint->y = g_zVideo_ProjectClipBottom - 1.0f;
                result = 8;
            }
        } else {
            result = 8;
            if (-gClipRect_Primary.zMin > dstPoint->z) {
                dstPoint->z *= -1.0f;
            } else {
                dstPoint->z = gClipRect_Primary.zMin;
            }

            ProjectPointBatch(dstPoint, (zProjectedPoint*)(dstPoint), 1);
            dstPoint->y = g_zVideo_ProjectClipBottom;
            if (dstPoint->x < -5000.0f) {
                dstPoint->x = -5000.0f;
            } else if (dstPoint->x >= 5000.0f) {
                dstPoint->x = 5000.0f;
            }
            dstPoint->x = (dstPoint->x + g_zVideo_ProjectClipLeft - -5000.0f)
                / ((5000.0f - -5000.0f) / (gClipRect_Primary.xMaxAlt - g_zVideo_ProjectClipLeft));
        }
        return result;
    }
} // namespace zMath

namespace zClipAlt
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zclipalt-remappointxyinplace
     * @recoil-artifact defines .text recoil:function:0x4766a0: zClipAlt::RemapPointXYInPlace
     *
     *
     * Purpose: reject a point outside the alternate clip rectangle or remap its XY
     * coordinates into source-rectangle space in place.
     */
    int __fastcall RemapPointXYInPlace(float* point)
    {
        g_Clip_PolyVerts[0].x = point[0];
        g_Clip_PolyVerts[0].y = point[1];
        if (zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, 1) != 0) {
            point[0] = g_zClipAlt_RemapScaleX * point[0] + g_zClipAlt_RemapBiasX;
            point[1] = g_zClipAlt_RemapScaleY * point[1] + g_zClipAlt_RemapBiasY;
            return 1;
        }

        return 0;
    }
} // namespace zClipAlt

namespace zScene
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zscene-testprojectedspherevisible
     * @recoil-artifact defines .text recoil:function:0x476700: zScene::TestProjectedSphereVisible
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
     *
     *
     * Purpose: project a bounding sphere and test representative span-buffer columns for visibility.
     */
    int __fastcall TestProjectedSphereVisible(zVec3 * center, float radius)
    {
        zMat4x3 slotBuffer;
        zMath::MatStackPushPtr((float*)(&slotBuffer));
        zMath::MatLoadCameraScratchB();

        zVec3 viewPoint;
        ZMTH_MAT_TRANSFORM_POINT_BATCH(center, &viewPoint, 1);
        zMath::MatStackPopPtr();

        const float depthMinusRadius = viewPoint.z - radius;
        if (depthMinusRadius <= 0.0000999999975f) {
            return 1;
        }

        zProjectedPoint projectedPoint;
        zMath::ProjectPointBatch(&viewPoint, &projectedPoint, 1);
        const zVec2 screenScale = zMathProjectGetLastScreenScaleXY();
        const int projectedRadius = TruncateToInt((screenScale.x * radius) / depthMinusRadius);
        if (projectedRadius < 1) {
            return 0;
        }

        const int centerX = TruncateToInt(projectedPoint.x);
        zRndr::g_spanAllocCursor->sampleXMin = centerX - projectedRadius;
        if ((float)(zRndr::g_spanAllocCursor->sampleXMin) >= gClipRect_Primary.xMax) {
            return 0;
        }

        zRndr::g_spanAllocCursor->sampleXMax = centerX + projectedRadius;
        if ((float)(zRndr::g_spanAllocCursor->sampleXMax) < gClipRect_Primary.xMin) {
            return 0;
        }

        // The center row is reused as the middle sample row below.
        int midColumn = TruncateToInt(projectedPoint.y);
        int columnMin = midColumn - projectedRadius;
        if ((float)(columnMin) > gClipRect_Primary.yMax - 2.0f) {
            return 0;
        }

        int columnMax = midColumn + projectedRadius;
        if ((float)(columnMax) <= gClipRect_Primary.yMin) {
            return 0;
        }

        const int clipXMin = TruncateToInt(gClipRect_Primary.xMin);
        zRndr::g_spanAllocCursor->sampleXMin
            = clipXMin > zRndr::g_spanAllocCursor->sampleXMin ? clipXMin : zRndr::g_spanAllocCursor->sampleXMin;

        const int savedSampleXMin = zRndr::g_spanAllocCursor->sampleXMin;
        const int clipXMax = TruncateToInt(gClipRect_Primary.xMax - 2.0f);
        zRndr::g_spanAllocCursor->sampleXMax
            = clipXMax < zRndr::g_spanAllocCursor->sampleXMax ? clipXMax : zRndr::g_spanAllocCursor->sampleXMax;

        zRndr::g_spanAllocCursor->invDepth = 1.0f / depthMinusRadius;
        zRndr::g_spanAllocCursor->invDepthStep = zRndr::g_spanAllocCursor->invDepth;
        zRndr::g_spanAllocCursor->depthSlope = 0.0f;

        const int clipYMin = TruncateToInt(gClipRect_Primary.yMin + 1.0f);
        if (clipYMin > columnMin) {
            columnMin = clipYMin;
        }

        int isVisible;
        zRndrSpanOcclusionTestColumnVisibility(columnMin, &isVisible);
        if (isVisible > 0) {
            return 1;
        }

        const int clipYMax = TruncateToInt(gClipRect_Primary.yMax - 2.0f);
        if (clipYMax < columnMax) {
            columnMax = clipYMax;
        }

        zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
        zRndrSpanOcclusionTestColumnVisibility(columnMax, &isVisible);
        if (isVisible > 0) {
            return 1;
        }

        const int columnDelta = columnMax - columnMin;
        if (columnDelta > 1) {
            midColumn = (columnDelta >> 1) + columnMin;
            zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
            zRndrSpanOcclusionTestColumnVisibility(midColumn, &isVisible);
            if (isVisible > 0) {
                return 1;
            }
        } else if (columnDelta < 16) {
            return 0;
        }

        int columnIndex;
        for (columnIndex = midColumn - 8; columnIndex > columnMin; columnIndex -= 8) {
            zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
            zRndrSpanOcclusionTestColumnVisibility(columnIndex, &isVisible);
            if (isVisible > 0) {
                return 1;
            }
        }

        for (columnIndex = midColumn + 8; columnIndex < columnMax; columnIndex += 8) {
            zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
            zRndrSpanOcclusionTestColumnVisibility(columnIndex, &isVisible);
            if (isVisible > 0) {
                return 1;
            }
        }

        return 0;
    }
} // namespace zScene

namespace zDi
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zdi-evalboundingspherelightingflags
     * @recoil-artifact defines .text recoil:function:0x476a50: zDi::EvalBoundingSphereLightingFlags
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
     *
     *
     * Purpose: evaluate fog, active-light, and lens-flare visibility flags for a display instance.
     */
    void __fastcall EvalBoundingSphereLightingFlags(
        zDiPartial * self,
        int* outDepthFade,
        int* outActiveLightState,
        int* outLensFlareVisible
    )
    {
        zVec3 mappedPoint;
        ZMTH_MAT_TRANSFORM_POINT_BATCH(&self->bboxCenter, &mappedPoint, 1);

        if (gModel_FogEnabled != 0 && (self->flags & 2) != 0
            && zModel_Light::EvalSphereFogFade(&mappedPoint, self->bboxRadius) > kVisibleContributionThreshold) {
            *outDepthFade = 1;
        } else {
            *outDepthFade = 0;
        }

        int activeLightContributionCount;
        if (ModelGraphicsFlagBit0Enabled()) {
            if (gModel_HasActiveLights != 0 && (self->flags & 1) != 0) {
                activeLightContributionCount = zModel_Light::PointInPolygonTestRadiusXZ(&mappedPoint, self->bboxRadius);
                *outActiveLightState = activeLightContributionCount > 0 ? 1 : 0;
            } else {
                activeLightContributionCount = 0;
                *outActiveLightState = activeLightContributionCount;
            }

            if (g_zModel_FogTargetColorOverride.weight > kVisibleContributionThreshold) {
                ++activeLightContributionCount;
                *outActiveLightState = 1;
            }
        } else {
            activeLightContributionCount = 0;
            *outActiveLightState = activeLightContributionCount;
        }

        if (activeLightContributionCount > 1) {
            float invTotalWeight;
            zColorRgb fogColorRgb01;
            fogColorRgb01.blue = 0.0f;
            fogColorRgb01.green = 0.0f;
            fogColorRgb01.red = 0.0f;
            float totalWeight = 0.0f;
            float maxWeight = 0.0f;

            zModel_ActiveLightEntryLive* entry = gModel_ActiveLights;
            for (int i = 0; i < gModel_ActiveLightCount; ++i, ++entry) {
                if (entry->contributesToLighting == 0) {
                    continue;
                }

                if (g_zModel_SoftwarePathActive != 0 && entry->light->isDirectedSource != 0) {
                    continue;
                }

                CZLightDataPartial* light = entry->light;
                fogColorRgb01.red += light->specularColor.red * g_Clip_PolyAttr0[i];
                fogColorRgb01.green += light->specularColor.green * g_Clip_PolyAttr0[i];
                fogColorRgb01.blue += light->specularColor.blue * g_Clip_PolyAttr0[i];
                totalWeight += g_Clip_PolyAttr0[i];
                if (maxWeight < g_Clip_PolyAttr0[i]) {
                    maxWeight = g_Clip_PolyAttr0[i];
                }
            }

            (void)maxWeight;

            if (g_zModel_FogTargetColorOverride.weight > kVisibleContributionThreshold) {
                fogColorRgb01.red += g_zModel_FogTargetColorOverride.colorRgb01.red;
                fogColorRgb01.green += g_zModel_FogTargetColorOverride.colorRgb01.green;
                fogColorRgb01.blue += g_zModel_FogTargetColorOverride.colorRgb01.blue;
                totalWeight += g_zModel_FogTargetColorOverride.weight;
            }

            invTotalWeight = 1.0f / totalWeight;
            fogColorRgb01.red *= invTotalWeight;
            fogColorRgb01.green *= invTotalWeight;
            fogColorRgb01.blue *= invTotalWeight;
            zRndr::SetFogTargetColorRgb01Clamped(&fogColorRgb01);
            *outLensFlareVisible = 1;
        } else {
            *outLensFlareVisible = 0;
        }
    }
} // namespace zDi

namespace zModel
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-rendernodesoftware
     * @recoil-artifact defines .text recoil:function:0x476cf0: zModel::RenderNodeSoftware
     *
     *
     * Purpose: render a display-instance node through the software renderer path.
     */
    void __fastcall RenderNodeSoftware(CZNodePartial * node, int clipMask)
    {
        zDiPartial* const di = NodeDisplayInstance(node);
        if (di == 0) {
            return;
        }

        zMat4x3 matrixScratch = { 0 };
        zMath::MatStackPushPtr((float*)(&matrixScratch));
        switch (di->mode) {
        default:
            zMathMatSetupCamera();
            zRndr::g_perspectiveTextureEnabled = 0;
            break;
        case 2: {
            zMathMatSetupCamera();

            PrepareTransformedVertices(di);

            {
                const unsigned int pointColor = di->entries[0].material != 0 ? di->entries[0].material->packedColor : 0;
                for (int vertexIndex = 0; vertexIndex < di->vertCount; ++vertexIndex) {
                    zVec3* const transformed = &g_zModel_TransformedVerts[vertexIndex];
                    if (transformed->z <= gClipRect_Primary.zMin) {
                        continue;
                    }

                    zProjectedPoint projectedPoint = { 0 };
                    if (g_zVideo_ActiveRendererPath != 0) {
                        zMathProjectSphereBatch(transformed, (zProjectedSphere*)(&projectedPoint), 1);
                    } else {
                        zMath::ProjectPointBatch(transformed, &projectedPoint, 1);
                    }

                    if (!ProjectedPointInClipBounds(projectedPoint)) {
                        continue;
                    }

                    if (g_zVideo_ActiveRendererPath != 0) {
                        g_zVideo_pfnDrawPointColor16((zVideo_XyzVertex*)(&projectedPoint), pointColor & 0xffff, 1);
                    } else {
                        zRndrLensFlareQueueProjectedSample(&projectedPoint, (int)(pointColor & 0xffff), 0);
                    }
                }
            }

            zMath::MatStackPopPtr();
            zRndr::g_perspectiveTextureEnabled = 0;
            return;
        }
        case 1:
            if ((di->flags & 0x10) != 0) {
                zMathMatLoadView();
            } else {
                zMathMatLoadProjection(g_zVideo_pActiveProjectionViewContext->eulerAngles.y);
            }
            zRndr::g_perspectiveTextureEnabled = 0;
            break;
        case 0:
            zMathMatSetupCamera();
            zRndr::g_perspectiveTextureEnabled = 1;
            break;
        }

        int outDepthFade = 0;
        int outActiveLightState = 0;
        int outLensFlareVisible = 0;
        if (di->entryCount > 0) {
            zDi::EvalBoundingSphereLightingFlags(di, &outDepthFade, &outActiveLightState, &outLensFlareVisible);

            PrepareTransformedVertices(di);
        }

        if ((di->flags & 8) != 0 && di->pointEntries != 0) {
            for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex) {
                zModel_PointEntryPartial* const pointEntry = &di->pointEntries[pointIndex];
                if (pointEntry->pointCamList == 0 || pointEntry->pointCamCount <= 0) {
                    continue;
                }

                if (pointEntry->pointCamCount == 1) {
                    zModelRenderPointQueueEntry(&pointEntry->pointCamList[0], pointEntry->packedColor16, pointEntry);
                } else {
                    for (int pointCamIndex = 0; pointCamIndex < pointEntry->pointCamCount; ++pointCamIndex) {
                        zModelRenderPointQueueEntry(
                            &pointEntry->pointCamList[pointCamIndex],
                            pointEntry->packedColor16,
                            pointEntry
                        );
                    }
                }
            }
        }

        gClipRect_Primary.flags = clipMask;
        for (int entryIndex = 0; entryIndex < di->entryCount; ++entryIndex) {
            zDiEntryPartial* const entry = &di->entries[entryIndex];
            zModel_MaterialPartial* const material = entry->material;
            int vertexCount = (int)(entry->flagsAndIndexCount & 0xff);
            if (material == 0 || vertexCount < 3 || vertexCount > 0x40) {
                continue;
            }
            int entryVerticesCopied = 0;
            CopyEntryVerticesToScratch(di, entry, vertexCount, entryVerticesCopied);
            if (entryVerticesCopied == 0) {
                continue;
            }

            zRndrSetPaletteRemapKeyFromRgb01(0, 0.0f);
            zRndrSetPaletteRemapKey(0, 0.0f);
            zRndrSetPaletteShadeRecipeIndex(0);
            if ((material->flags & 0x0400) != 0) {
                zModel_Material::UpdateCycleIfNeeded(material);
            }

            const int isTextured = (material->flags & 0x0100) != 0;
            int hasPerVertexShade = 0;
            int preservePaletteRemapKey = 0;
            int packedColor = material->packedColor;
            if (isTextured != 0) {
                for (int i = 0; i < vertexCount; ++i) {
                    g_Clip_PolyAttr0[i] = 0.0f;
                }
                if (outDepthFade != 0
                    && zModel_Light::BuildAttr0DepthFade(vertexCount, &preservePaletteRemapKey) != 0) {
                    hasPerVertexShade = 1;
                    zRndrSetPaletteRemapKey(0, 0.0f);
                    zRndrSetPaletteShadeRecipeIndex(0);
                }
            }

            zVec3 surfaceNormal = { 0 };
            int scanConvertMode = 1;
            int surfaceVisible = 0;
            ComputeSurfaceNormalAndCull(
                vertexCount,
                (entry->flagsAndIndexCount & 0x0100) != 0,
                &surfaceNormal,
                &scanConvertMode,
                surfaceVisible
            );
            if (surfaceVisible == 0) {
                continue;
            }

            if (isTextured != 0) {
                CopyEntryUvsToScratch(entry, vertexCount);

                int lightingMode = 0;
                if (outActiveLightState != 0) {
                    int lightFlags = 0;
                    int usePaletteRemap = 0;
                    if (material->currentTextureDirectoryEntry != 0
                        && material->currentTextureDirectoryEntry->image != 0
                        && material->currentTextureDirectoryEntry->image->palette != 0) {
                        usePaletteRemap = 1;
                    }
                    if (zModel_Light::SetActiveLights(
                            &surfaceNormal,
                            vertexCount,
                            &lightFlags,
                            &lightingMode,
                            usePaletteRemap
                        )
                        != 0) {
                        hasPerVertexShade = 1;
                    }
                    preservePaletteRemapKey |= lightingMode;
                    if (lightFlags == 1) {
                        zRndr::CommitFogColorParamsIfChanged();
                    }
                }
            }

            int clippedCount = vertexCount;
            if (isTextured != 0) {
                int polygonClipped = 0;
                ClipAndProjectSoftwareTextured(&gClipRect_Primary, &clippedCount, hasPerVertexShade, polygonClipped);
                if (polygonClipped == 0) {
                    continue;
                }
                int smallPolyRejected = 0;
                RejectProjectedSmallPoly(clippedCount, smallPolyRejected);
                if (smallPolyRejected != 0) {
                    continue;
                }

                zVec3 triClipVerts[3];
                CopyProjectedTriVerts(triClipVerts);
                ApplySoftwareDepthScale(entry->drawFlags);
                zRndr::g_scanConvertMode = scanConvertMode;
                zRndrSubmitTexturedPolyPerVertexAlphaOrShade(
                    (zVec3*)g_Clip_PolyVerts,
                    (zVec3*)g_Clip_PolyVertsScratch,
                    triClipVerts,
                    (zVec2*)g_Clip_PolyUvs,
                    g_Clip_PolyAttr0,
                    0,
                    clippedCount,
                    material->currentTextureDirectoryEntry,
                    preservePaletteRemapKey,
                    gModel_RenderVertexAlphaEnabled
                );

                if (gAltClipPassEnabled != 0) {
                    clippedCount = vertexCount;
                    CopyEntryVerticesToScratch(di, entry, clippedCount, entryVerticesCopied);
                    CopyEntryUvsToScratch(entry, clippedCount);
                    if (zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, clippedCount) != 0) {
                        polygonClipped = zClipRect::ClipPoly_NoUV(&gClipRect_Alt, &clippedCount);
                    } else {
                        polygonClipped = 0;
                    }
                    if (polygonClipped != 0) {
                        zRndr::g_inverseDepthBias = gClipRect_Primary.zMin;
                        if (hasPerVertexShade != 2) {
                            zRndrSubmitTexturedPolyUniformAlphaOrShade(
                                (zVec3*)g_Clip_PolyVerts,
                                0,
                                triClipVerts,
                                (zVec2*)g_Clip_PolyUvs,
                                clippedCount,
                                material->currentTextureDirectoryEntry,
                                gModel_RenderAlphaScaleCurrent,
                                gModel_RenderVertexAlphaEnabled
                            );
                        } else {
                            zRndrSubmitTexturedPolyPerVertexAlphaOrShade(
                                (zVec3*)g_Clip_PolyVerts,
                                0,
                                triClipVerts,
                                (zVec2*)g_Clip_PolyUvs,
                                g_Clip_PolyAttr0,
                                0,
                                clippedCount,
                                material->currentTextureDirectoryEntry,
                                preservePaletteRemapKey,
                                gModel_RenderVertexAlphaEnabled
                            );
                        }
                    }
                }
            } else {
                if (zClipRect::ClipPolyNearZ(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }
                ProjectScratchToClipVerts(clippedCount);
                if ((clipMask & 0x0f) != 0 && zClipRect::ClipPoly_NoUV(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }

                if (outDepthFade == 0 && outActiveLightState == 0) {
                    zVec3 unlitTriClipVerts[3];
                    CopyProjectedTriVerts(unlitTriClipVerts);
                    ApplySoftwareDepthScale(entry->drawFlags);
                    zRndr::g_scanConvertMode = scanConvertMode;
                    zRndrSubmitTexturedPolyUniformAlphaOrShade(
                        (zVec3*)g_Clip_PolyVerts,
                        (zVec3*)g_Clip_PolyVertsScratch,
                        unlitTriClipVerts,
                        (zVec2*)g_Clip_PolyUvs,
                        clippedCount,
                        material->currentTextureDirectoryEntry,
                        gModel_RenderAlphaScaleCurrent,
                        gModel_RenderVertexAlphaEnabled
                    );

                    if (gAltClipPassEnabled != 0) {
                        clippedCount = vertexCount;
                        CopyEntryVerticesToScratch(di, entry, clippedCount, entryVerticesCopied);
                        if (zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, clippedCount) != 0
                            && zClipRect::ClipPoly_NoUV(&gClipRect_Alt, &clippedCount) != 0) {
                            zRndr::g_inverseDepthBias = gClipRect_Primary.zMin;
                            zRndrSubmitTexturedPolyUniformAlphaOrShade(
                                (zVec3*)g_Clip_PolyVerts,
                                0,
                                unlitTriClipVerts,
                                (zVec2*)g_Clip_PolyUvs,
                                clippedCount,
                                material->currentTextureDirectoryEntry,
                                gModel_RenderAlphaScaleCurrent,
                                gModel_RenderVertexAlphaEnabled
                            );
                        }
                    }
                    continue;
                }

                float outFade = 0.0f;
                if (outDepthFade != 0) {
                    if (zModel_Light::EvalBatchSphereFade(&outFade) != 0) {
                        hasPerVertexShade = 1;
                    }
                }
                if (outActiveLightState != 0
                    && zModelLightBuildLightWeights(&surfaceNormal, vertexCount, &packedColor, outFade) != 0) {
                    hasPerVertexShade = 2;
                }
                if (outDepthFade != 0 && hasPerVertexShade == 1) {
                    zRndr::CommitFogColorParamsIfChanged();
                    float scale255 = 0.0f;
                    zFloat::Set255f(&scale255);
                    scale255 -= 1.0f;
                    zRndr::BlendPackedColor565WithFogInPlace(&packedColor, (int)(outFade * scale255));
                }

                if ((clipMask & 0x30) != 0 && zClipRect::ClipPolyZRange_NoUV(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }
                ProjectScratchToClipVerts(clippedCount);
                if ((clipMask & 0x0f) != 0 && zClipRect::ClipPoly_NoUV(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }

                int smallPolyRejected = 0;
                RejectProjectedSmallPoly(clippedCount, smallPolyRejected);
                if (smallPolyRejected != 0) {
                    continue;
                }

                zVec3 triClipVerts[3];
                CopyProjectedTriVerts(triClipVerts);

                ApplySoftwareDepthScale(entry->drawFlags);
                zRndr::g_scanConvertMode = scanConvertMode;
                zRndrSubmitPolyWithSpanList(
                    (zVec3*)g_Clip_PolyVerts,
                    triClipVerts,
                    packedColor,
                    MaterialAlphaInt(material),
                    clippedCount,
                    gModel_RenderVertexAlphaEnabled
                );

                if (gAltClipPassEnabled != 0) {
                    clippedCount = vertexCount;
                    CopyEntryVerticesToScratch(di, entry, clippedCount, entryVerticesCopied);
                    if (zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, clippedCount) != 0
                        && zClipRect::ClipPoly_NoUV(&gClipRect_Alt, &clippedCount) != 0) {
                        zRndr::g_inverseDepthBias = gClipRect_Primary.zMin;
                        zRndrSubmitPolyWithSpanList(
                            (zVec3*)g_Clip_PolyVerts,
                            triClipVerts,
                            packedColor,
                            MaterialAlphaInt(material),
                            clippedCount,
                            gModel_RenderVertexAlphaEnabled
                        );
                    }
                }
            }
        }

        zMath::MatStackPopPtr();
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-rendernodehardware
     * @recoil-artifact defines .text recoil:function:0x477b30: zModel::RenderNodeHardware
     *
     *
     * Purpose: render a display-instance node through the hardware renderer path.
     */
    void __fastcall RenderNodeHardware(CZNodePartial * node, int clipMask)
    {
        zDiPartial* const di = NodeDisplayInstance(node);
        if (di == 0) {
            return;
        }

        zMat4x3 matrixScratch = { 0 };
        zMath::MatStackPushPtr((float*)(&matrixScratch));
        switch (di->mode) {
        default:
            zMathMatSetupCamera();
            zRndr::g_perspectiveTextureEnabled = 0;
            break;
        case 2: {
            zMathMatSetupCamera();

            PrepareTransformedVertices(di);

            {
                const unsigned int pointColor = di->entries[0].material != 0 ? di->entries[0].material->packedColor : 0;
                for (int vertexIndex = 0; vertexIndex < di->vertCount; ++vertexIndex) {
                    zVec3* const transformed = &g_zModel_TransformedVerts[vertexIndex];
                    if (transformed->z <= gClipRect_Primary.zMin) {
                        continue;
                    }

                    zProjectedPoint projectedPoint = { 0 };
                    if (g_zVideo_ActiveRendererPath != 0) {
                        zMathProjectSphereBatch(transformed, (zProjectedSphere*)(&projectedPoint), 1);
                    } else {
                        zMath::ProjectPointBatch(transformed, &projectedPoint, 1);
                    }
                    if (!ProjectedPointInClipBounds(projectedPoint)) {
                        continue;
                    }
                    if (g_zVideo_ActiveRendererPath != 0) {
                        g_zVideo_pfnDrawPointColor16((zVideo_XyzVertex*)(&projectedPoint), pointColor & 0xffff, 1);
                    } else {
                        zRndrLensFlareQueueProjectedSample(&projectedPoint, (int)(pointColor & 0xffff), 0);
                    }
                }
            }

            zMath::MatStackPopPtr();
            zRndr::g_perspectiveTextureEnabled = 0;
            return;
        }
        case 1:
            if ((di->flags & 0x10) != 0) {
                zMathMatLoadView();
            } else {
                zMathMatLoadProjection(g_zVideo_pActiveProjectionViewContext->eulerAngles.y);
            }
            zRndr::g_perspectiveTextureEnabled = 0;
            break;
        case 0:
            zMathMatSetupCamera();
            zRndr::g_perspectiveTextureEnabled = 1;
            break;
        }

        int outDepthFade = 0;
        int outActiveLightState = 0;
        int outLensFlareVisible = 0;
        if (di->entryCount > 0) {
            zDi::EvalBoundingSphereLightingFlags(di, &outDepthFade, &outActiveLightState, &outLensFlareVisible);

            PrepareTransformedVertices(di);
            PrepareTransformedNormals(di);
        }

        if ((di->flags & 8) != 0 && di->pointEntries != 0) {
            for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex) {
                zModel_PointEntryPartial* const pointEntry = &di->pointEntries[pointIndex];
                if (pointEntry->pointCamList == 0 || pointEntry->pointCamCount <= 0) {
                    continue;
                }

                if (pointEntry->pointCamCount == 1) {
                    zModelRenderPointQueueEntry(&pointEntry->pointCamList[0], pointEntry->packedColor16, pointEntry);
                } else {
                    for (int pointCamIndex = 0; pointCamIndex < pointEntry->pointCamCount; ++pointCamIndex) {
                        zModelRenderPointQueueEntry(
                            &pointEntry->pointCamList[pointCamIndex],
                            pointEntry->packedColor16,
                            pointEntry
                        );
                    }
                }
            }
        }

        gClipRect_Primary.flags = clipMask;
        for (int entryIndex = 0; entryIndex < di->entryCount; ++entryIndex) {
            zDiEntryPartial* const entry = &di->entries[entryIndex];
            zModel_MaterialPartial* const material = entry->material;
            int vertexCount = (int)(entry->flagsAndIndexCount & 0xff);
            if (material == 0 || vertexCount < 3 || vertexCount > 0x40) {
                continue;
            }
            int entryVerticesCopied = 0;
            CopyEntryVerticesToScratch(di, entry, vertexCount, entryVerticesCopied);
            if (entryVerticesCopied == 0) {
                continue;
            }

            zVec3 surfaceNormal = { 0 };
            int surfaceVisible = 0;
            ComputeSurfaceNormalAndCull(
                vertexCount,
                (entry->flagsAndIndexCount & 0x0100) != 0,
                &surfaceNormal,
                0,
                surfaceVisible
            );
            if (surfaceVisible == 0) {
                continue;
            }

            g_zModel_CurrentPolyNormals = 0;
            if (g_zModel_VertexShadingEnabled != 0 && di->normalCount > 0 && (entry->flagsAndIndexCount & 0x0200) != 0
                && entry->normalIndices != 0) {
                int* normalIndices = (int*)(entry->normalIndices);
                g_zModel_CurrentPolyNormals = g_zModel_CurrentPolyNormalsStorage;
                for (int normalSlot = 0; normalSlot < vertexCount; ++normalSlot) {
                    const int normalIndex = normalIndices[normalSlot];
                    if (normalIndex < 0 || normalIndex >= di->normalCount) {
                        g_zModel_CurrentPolyNormals = 0;
                        break;
                    }
                    g_zModel_CurrentPolyNormalsStorage[normalSlot] = g_zModel_TransformedNormals[normalIndex];
                }
            }

            for (int attrIndex = 0; attrIndex < vertexCount; ++attrIndex) {
                g_Clip_PolyAttr0[attrIndex] = 0.0f;
                g_Clip_PolyAttr1[attrIndex] = 0.0f;
            }

            int lightingFlags = 0;
            int lightingVaries = 0;
            int clippedCount = vertexCount;
            int polygonVisible = 1;
            zClipUV perspectiveUvs[0x400];

            if ((material->flags & 0x0100) != 0) {
                if ((material->flags & 0x0400) != 0) {
                    zModel_Material::UpdateCycleIfNeeded(material);
                }
                if (outDepthFade != 0) {
                    if (zModel_Light::BuildAttr1Falloff(vertexCount, &lightingVaries) != 0) {
                        lightingFlags = 1;
                    }
                }
                int lightVaries = lightingVaries;
                if (outActiveLightState != 0) {
                    if (zModel_Light::SetActiveLights(&surfaceNormal, vertexCount, &lightingFlags, &lightVaries, 0)
                        != 0) {
                        lightingFlags |= 2;
                        lightingVaries |= lightVaries;
                    }
                }
                if ((lightingFlags & ~0x0c) == 0) {
                    for (int attrIndex = 0; attrIndex < vertexCount; ++attrIndex) {
                        g_Clip_PolyAttr0[attrIndex] = 1.0f;
                        g_Clip_PolyAttr1[attrIndex] = 1.0f;
                    }
                }

                CopyEntryUvsToScratch(entry, vertexCount);

                if (lightingFlags != 0) {
                    clippedCount = vertexCount;
                    polygonVisible = 1;
                    if ((gClipRect_Primary.flags & 0x30) != 0) {
                        const int previousCount = clippedCount;
                        if (lightingVaries != 0) {
                            polygonVisible = zClipRect::ClipPolyZRange_WithAttr012(&gClipRect_Primary, &clippedCount);
                        } else {
                            polygonVisible = zClipRect::ClipPolyNearZ(&gClipRect_Primary, &clippedCount);
                        }
                        if (polygonVisible != 0 && lightingVaries == 0 && previousCount < clippedCount) {
                            for (int attrIndex = previousCount; attrIndex < clippedCount; ++attrIndex) {
                                g_Clip_PolyAttr0[attrIndex] = g_Clip_PolyAttr0[0];
                                g_Clip_PolyAttr1[attrIndex] = g_Clip_PolyAttr1[0];
                                g_Clip_PolyAttr2[attrIndex] = g_Clip_PolyAttr2[0];
                            }
                        }
                    }
                    if (polygonVisible != 0) {
                        zMathProjectSphereBatch(
                            (const zVec3*)g_Clip_PolyVertsScratch,
                            (zProjectedSphere*)g_Clip_PolyVerts,
                            clippedCount
                        );
                        for (int uvIndex = 0; uvIndex < clippedCount; ++uvIndex) {
                            g_Clip_PolyUvs[uvIndex].u *= g_Clip_PolyVerts[uvIndex].z;
                            g_Clip_PolyUvs[uvIndex].v *= g_Clip_PolyVerts[uvIndex].z;
                        }
                    }
                    if (polygonVisible != 0 && (gClipRect_Primary.flags & 0x0f) != 0) {
                        const int previousCount = clippedCount;
                        if (lightingVaries != 0) {
                            polygonVisible = zClipRect::ClipPoly_WithAttr012(&gClipRect_Primary, &clippedCount);
                        } else {
                            polygonVisible = zClipRect::ClipPoly(&gClipRect_Primary, &clippedCount);
                        }
                        if (polygonVisible != 0 && lightingVaries == 0 && previousCount < clippedCount) {
                            for (int attrIndex = previousCount; attrIndex < clippedCount; ++attrIndex) {
                                g_Clip_PolyAttr0[attrIndex] = g_Clip_PolyAttr0[0];
                                g_Clip_PolyAttr1[attrIndex] = g_Clip_PolyAttr1[0];
                                g_Clip_PolyAttr2[attrIndex] = g_Clip_PolyAttr2[0];
                            }
                        }
                    }
                    if (polygonVisible == 0) {
                        continue;
                    }

                    for (int uvIndex = 0; uvIndex < clippedCount; ++uvIndex) {
                        const float depth = 1.0f / g_Clip_PolyVerts[uvIndex].z;
                        perspectiveUvs[uvIndex].u = g_Clip_PolyUvs[uvIndex].u * depth;
                        perspectiveUvs[uvIndex].v = g_Clip_PolyUvs[uvIndex].v * depth;
                    }
                    for (int depthIndex = 0; depthIndex < clippedCount; ++depthIndex) {
                        g_Clip_PolyVerts[depthIndex].z
                            *= (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                    }
                    if (g_zModel_CurrentPolyNormals != 0) {
                        SubmitPolygonLitProc* submitSlot = (SubmitPolygonLitProc*)&g_zVideo_pfnSubmitPolygonLit;
                        (*submitSlot)(
                            (zVideo_XyzVertex*)g_Clip_PolyVerts,
                            (zVideo_TexCoord*)perspectiveUvs,
                            g_Clip_PolyAttr1,
                            (lightingFlags & 4) != 0 ? g_Clip_PolyAttr0 : 0,
                            (lightingFlags & 1) != 0 ? g_Clip_PolyAttr2 : 0,
                            clippedCount,
                            material->currentTextureDirectoryEntry != 0
                                ? (zVideo_RenderClass*)(material->currentTextureDirectoryEntry->texture)
                                : 0,
                            entry->drawFlags,
                            gModel_RenderAlphaScaleCurrent,
                            gModel_RenderVertexAlphaEnabled
                        );
                    } else {
                        SubmitPolygonProc* submitSlot = (SubmitPolygonProc*)&g_zVideo_pfnSubmitPolygon;
                        (*submitSlot)(
                            (zVideo_XyzVertex*)g_Clip_PolyVerts,
                            (zVideo_TexCoord*)perspectiveUvs,
                            g_Clip_PolyAttr1,
                            (lightingFlags & 4) != 0 ? g_Clip_PolyAttr0 : 0,
                            (lightingFlags & 1) != 0 ? g_Clip_PolyAttr2 : 0,
                            clippedCount,
                            material->currentTextureDirectoryEntry != 0
                                ? (zVideo_RenderClass*)(material->currentTextureDirectoryEntry->texture)
                                : 0,
                            entry->drawFlags,
                            gModel_RenderAlphaScaleCurrent,
                            gModel_RenderVertexAlphaEnabled
                        );
                    }

                    if (gAltClipPassEnabled != 0) {
                        polygonVisible = zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, clippedCount);
                        const int previousCount = clippedCount;
                        if (polygonVisible != 0) {
                            if (lightingVaries != 0) {
                                polygonVisible = zClipRect::ClipPoly_WithAttr012(&gClipRect_Alt, &clippedCount);
                            } else {
                                polygonVisible = zClipRect::ClipPoly(&gClipRect_Alt, &clippedCount);
                            }
                        }
                        if (polygonVisible != 0 && lightingVaries == 0 && previousCount < clippedCount) {
                            for (int attrIndex = previousCount; attrIndex < clippedCount; ++attrIndex) {
                                g_Clip_PolyAttr0[attrIndex] = g_Clip_PolyAttr0[0];
                                g_Clip_PolyAttr1[attrIndex] = g_Clip_PolyAttr1[0];
                                g_Clip_PolyAttr2[attrIndex] = g_Clip_PolyAttr2[0];
                            }
                        }
                        if (polygonVisible != 0) {
                            for (int remapIndex = 0; remapIndex < clippedCount; ++remapIndex) {
                                const float depth = 1.0f / g_Clip_PolyVerts[remapIndex].z;
                                g_Clip_PolyVerts[remapIndex].x
                                    = g_zClipAlt_RemapScaleX * g_Clip_PolyVerts[remapIndex].x + g_zClipAlt_RemapBiasX;
                                g_Clip_PolyVerts[remapIndex].y
                                    = g_zClipAlt_RemapScaleY * g_Clip_PolyVerts[remapIndex].y + g_zClipAlt_RemapBiasY;
                                g_Clip_PolyVerts[remapIndex].z += 1.0f / gClipRect_Primary.zMin;
                                perspectiveUvs[remapIndex].u = g_Clip_PolyUvs[remapIndex].u * depth;
                                perspectiveUvs[remapIndex].v = g_Clip_PolyUvs[remapIndex].v * depth;
                            }
                            if ((lightingFlags & 2) != 0) {
                                SubmitPolygonProc* submitSlot = (SubmitPolygonProc*)&g_zVideo_pfnSubmitPolygon;
                                (*submitSlot)(
                                    (zVideo_XyzVertex*)g_Clip_PolyVerts,
                                    (zVideo_TexCoord*)perspectiveUvs,
                                    g_Clip_PolyAttr1,
                                    (lightingFlags & 4) != 0 ? g_Clip_PolyAttr0 : 0,
                                    0,
                                    clippedCount,
                                    material->currentTextureDirectoryEntry != 0
                                        ? (zVideo_RenderClass*)(material->currentTextureDirectoryEntry->texture)
                                        : 0,
                                    entry->drawFlags,
                                    gModel_RenderAlphaScaleCurrent,
                                    gModel_RenderVertexAlphaEnabled
                                );
                            } else {
                                zVideo_RenderClass* const renderClass = material->currentTextureDirectoryEntry != 0
                                    ? (zVideo_RenderClass*)material->currentTextureDirectoryEntry->texture
                                    : 0;
                                (*g_zVideo_pfnSubmitPolyRenderClass)(
                                    (zVideo_XyzVertex*)g_Clip_PolyVerts,
                                    (zVideo_TexCoord*)perspectiveUvs,
                                    clippedCount,
                                    renderClass,
                                    entry->drawFlags,
                                    gModel_RenderAlphaScaleCurrent,
                                    gModel_RenderVertexAlphaEnabled
                                );
                            }
                        }
                    }
                    continue;
                }

                clippedCount = vertexCount;
                polygonVisible = 1;
                if ((gClipRect_Primary.flags & 0x30) != 0) {
                    polygonVisible = zClipRect::ClipPolyNearZ(&gClipRect_Primary, &clippedCount);
                }
                if (polygonVisible == 0) {
                    continue;
                }
                zMathProjectSphereBatch(
                    (const zVec3*)g_Clip_PolyVertsScratch,
                    (zProjectedSphere*)g_Clip_PolyVerts,
                    clippedCount
                );
                for (int uvIndex = 0; uvIndex < clippedCount; ++uvIndex) {
                    g_Clip_PolyUvs[uvIndex].u *= g_Clip_PolyVerts[uvIndex].z;
                    g_Clip_PolyUvs[uvIndex].v *= g_Clip_PolyVerts[uvIndex].z;
                }
                if ((gClipRect_Primary.flags & 0x0f) != 0) {
                    polygonVisible = zClipRect::ClipPoly(&gClipRect_Primary, &clippedCount);
                }
                if (polygonVisible == 0) {
                    continue;
                }

                for (int perspectiveIndex = 0; perspectiveIndex < clippedCount; ++perspectiveIndex) {
                    if (g_Clip_PolyVerts[perspectiveIndex].z != 0.0f) {
                        const float depth = 1.0f / g_Clip_PolyVerts[perspectiveIndex].z;
                        perspectiveUvs[perspectiveIndex].u = g_Clip_PolyUvs[perspectiveIndex].u * depth;
                        perspectiveUvs[perspectiveIndex].v = g_Clip_PolyUvs[perspectiveIndex].v * depth;
                    } else {
                        perspectiveUvs[perspectiveIndex] = g_Clip_PolyUvs[perspectiveIndex];
                    }
                }
                for (int depthIndex = 0; depthIndex < clippedCount; ++depthIndex) {
                    g_Clip_PolyVerts[depthIndex].z *= (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                }
                zVideo_RenderClass* const renderClass = material->currentTextureDirectoryEntry != 0
                    ? (zVideo_RenderClass*)(material->currentTextureDirectoryEntry->texture)
                    : 0;
                SubmitPolyRenderClassProc* submitSlot = (SubmitPolyRenderClassProc*)&g_zVideo_pfnSubmitPolyRenderClass;
                (*submitSlot)(
                    (zVideo_XyzVertex*)g_Clip_PolyVerts,
                    (zVideo_TexCoord*)perspectiveUvs,
                    clippedCount,
                    renderClass,
                    entry->drawFlags,
                    gModel_RenderAlphaScaleCurrent,
                    gModel_RenderVertexAlphaEnabled
                );

                if (gAltClipPassEnabled != 0) {
                    if (zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, clippedCount) != 0
                        && zClipRect::ClipPoly(&gClipRect_Alt, &clippedCount) != 0) {
                        for (int remapIndex = 0; remapIndex < clippedCount; ++remapIndex) {
                            const float depth = 1.0f / g_Clip_PolyVerts[remapIndex].z;
                            g_Clip_PolyVerts[remapIndex].x
                                = g_zClipAlt_RemapScaleX * g_Clip_PolyVerts[remapIndex].x + g_zClipAlt_RemapBiasX;
                            g_Clip_PolyVerts[remapIndex].y
                                = g_zClipAlt_RemapScaleY * g_Clip_PolyVerts[remapIndex].y + g_zClipAlt_RemapBiasY;
                            g_Clip_PolyVerts[remapIndex].z += 1.0f / gClipRect_Primary.zMin;
                            perspectiveUvs[remapIndex].u = g_Clip_PolyUvs[remapIndex].u * depth;
                            perspectiveUvs[remapIndex].v = g_Clip_PolyUvs[remapIndex].v * depth;
                        }
                        SubmitPolyRenderClassProc* altSubmitSlot
                            = (SubmitPolyRenderClassProc*)&g_zVideo_pfnSubmitPolyRenderClass;
                        (*altSubmitSlot)(
                            (zVideo_XyzVertex*)g_Clip_PolyVerts,
                            (zVideo_TexCoord*)perspectiveUvs,
                            clippedCount,
                            material->currentTextureDirectoryEntry != 0
                                ? (zVideo_RenderClass*)(material->currentTextureDirectoryEntry->texture)
                                : 0,
                            entry->drawFlags,
                            gModel_RenderAlphaScaleCurrent,
                            gModel_RenderVertexAlphaEnabled
                        );
                    }
                }
                continue;
            }

            if (outDepthFade != 0) {
                if (zModel_Light::BuildAttr1Falloff(vertexCount, &lightingVaries) != 0) {
                    lightingFlags = 1;
                }
            }
            int lightVaries = lightingVaries;
            if (outActiveLightState != 0) {
                if (zModel_Light::SetActiveLights(&surfaceNormal, vertexCount, &lightingFlags, &lightVaries, 0) != 0) {
                    lightingFlags |= 2;
                    lightingVaries |= lightVaries;
                }
            }
            clippedCount = vertexCount;
            if (lightingFlags != 0) {
                if ((gClipRect_Primary.flags & 0x30) != 0) {
                    polygonVisible = zClipRect::ClipPolyZRange_NoUV_WithAttribs(&gClipRect_Primary, &clippedCount);
                }
                if (polygonVisible != 0) {
                    zMathProjectSphereBatch(
                        (const zVec3*)g_Clip_PolyVertsScratch,
                        (zProjectedSphere*)g_Clip_PolyVerts,
                        clippedCount
                    );
                    if ((gClipRect_Primary.flags & 0x0f) != 0) {
                        polygonVisible = zClipRect::ClipPoly_NoUV_WithAttr012_Alt(&gClipRect_Primary, &clippedCount);
                    }
                }
                if (polygonVisible != 0) {
                    if (entry->drawFlags != 0) {
                        const float depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                        for (int depthIndex = 0; depthIndex < clippedCount; ++depthIndex) {
                            g_Clip_PolyVerts[depthIndex].z *= depthScale;
                        }
                    }
                    const int materialAlpha = MaterialAlphaInt(material);
                    SubmitPolyColorAttrProc* colorAttrSubmitSlot
                        = (SubmitPolyColorAttrProc*)&g_zVideo_pfnSubmitPolyColorAttr;
                    (*colorAttrSubmitSlot)(
                        (zVideo_XyzVertex*)g_Clip_PolyVerts,
                        material->packedColor & 0xffff,
                        (zVideo_ColorRgbFloat*)&material->colorRgb,
                        g_Clip_PolyAttr1,
                        (lightingFlags & 4) != 0 ? g_Clip_PolyAttr0 : 0,
                        (lightingFlags & 1) != 0 ? g_Clip_PolyAttr2 : 0,
                        materialAlpha,
                        clippedCount,
                        entry->drawFlags,
                        gModel_RenderVertexAlphaEnabled
                    );
                }
            } else {
                if ((gClipRect_Primary.flags & 0x30) != 0) {
                    polygonVisible = zClipRect::ClipPolyZRange_NoUV(&gClipRect_Primary, &clippedCount);
                }
                if (polygonVisible != 0) {
                    zMathProjectSphereBatch(
                        (const zVec3*)g_Clip_PolyVertsScratch,
                        (zProjectedSphere*)g_Clip_PolyVerts,
                        clippedCount
                    );
                    if ((gClipRect_Primary.flags & 0x0f) != 0) {
                        polygonVisible = zClipRect::ClipPoly_NoUV_Alt(&gClipRect_Primary, &clippedCount);
                    }
                }
                if (polygonVisible != 0) {
                    if (entry->drawFlags != 0) {
                        const float depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                        for (int depthIndex = 0; depthIndex < clippedCount; ++depthIndex) {
                            g_Clip_PolyVerts[depthIndex].z *= depthScale;
                        }
                    }
                    const int materialAlpha = MaterialAlphaInt(material);
                    SubmitPolyFlatColor16Proc* flatSubmitSlot
                        = (SubmitPolyFlatColor16Proc*)&g_zVideo_pfnSubmitPolyFlatColor16;
                    (*flatSubmitSlot)(
                        (zVideo_XyzVertex*)g_Clip_PolyVerts,
                        material->packedColor & 0xffff,
                        materialAlpha,
                        entry->drawFlags,
                        clippedCount,
                        gModel_RenderVertexAlphaEnabled
                    );
                }
            }
            if (polygonVisible != 0 && gAltClipPassEnabled != 0) {
                polygonVisible = zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, clippedCount);
                if (polygonVisible != 0) {
                    polygonVisible = zClipRect::ClipPoly_NoUV_Alt(&gClipRect_Alt, &clippedCount);
                }
                if (polygonVisible != 0) {
                    for (int remapIndex = 0; remapIndex < clippedCount; ++remapIndex) {
                        g_Clip_PolyVerts[remapIndex].x
                            = g_zClipAlt_RemapScaleX * g_Clip_PolyVerts[remapIndex].x + g_zClipAlt_RemapBiasX;
                        g_Clip_PolyVerts[remapIndex].y
                            = g_zClipAlt_RemapScaleY * g_Clip_PolyVerts[remapIndex].y + g_zClipAlt_RemapBiasY;
                        g_Clip_PolyVerts[remapIndex].z += 1.0f / gClipRect_Primary.zMin;
                    }
                    const int materialAlpha = MaterialAlphaInt(material);
                    SubmitPolyFlatColor16Proc* altFlatSubmitSlot
                        = (SubmitPolyFlatColor16Proc*)&g_zVideo_pfnSubmitPolyFlatColor16;
                    (*altFlatSubmitSlot)(
                        (zVideo_XyzVertex*)g_Clip_PolyVerts,
                        material->packedColor & 0xffff,
                        materialAlpha,
                        entry->drawFlags,
                        clippedCount,
                        gModel_RenderVertexAlphaEnabled
                    );
                }
            }
        }

        zMath::MatStackPopPtr();
    }
} // namespace zModel

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zvideo-frustumtestsphereclipmask
 * @recoil-artifact defines .text recoil:function:0x478c70: zVideoFrustumTestSphereClipMask.
 *
 *
 * Provisional source-placement hypothesis: GameZRecoil/zModel/zModel_Display.cpp.
 * Purpose: reject or clip a sphere against the active view frustum planes.
 *
 * Evidence: BN reads the projection view-context global at 0x576214, clears
 * the incoming clip mask, tests near and far centers separately, and tests
 * side planes against camera-position deltas while accumulating clip bits.
 */
int __fastcall zVideoFrustumTestSphereClipMask(zVec3* sphereCenter, float radius, int* clipMaskInOut)
{
    const int oldMask = *clipMaskInOut;
    *clipMaskInOut = 0;

    CZCameraDataPartial* viewContext = g_zVideo_pActiveProjectionViewContext;
    zVec3 delta;
    if ((oldMask & 0x10) != 0) {
        delta.x = sphereCenter->x - viewContext->nearClipCenter.x;
        delta.y = sphereCenter->y - viewContext->nearClipCenter.y;
        delta.z = sphereCenter->z - viewContext->nearClipCenter.z;
        const float dot = delta.x * viewContext->worldFrustumNormals[4].x
            + delta.y * viewContext->worldFrustumNormals[4].y + delta.z * viewContext->worldFrustumNormals[4].z;
        if (dot < radius) {
            if (-radius >= dot) {
                return 0x10;
            }
            *clipMaskInOut = 0x10;
        } else {
            *clipMaskInOut = 0;
        }
    }

    viewContext = g_zVideo_pActiveProjectionViewContext;
    delta.x = sphereCenter->x - viewContext->cameraPos.x;
    delta.y = sphereCenter->y - viewContext->cameraPos.y;
    delta.z = sphereCenter->z - viewContext->cameraPos.z;

    if ((oldMask & 1) != 0) {
        const float dot = delta.x * viewContext->worldFrustumNormals[0].x
            + delta.y * viewContext->worldFrustumNormals[0].y + delta.z * viewContext->worldFrustumNormals[0].z;
        if (-radius >= dot) {
            return 1;
        }
        if (dot < radius) {
            *clipMaskInOut |= 1;
        }
    }

    if ((oldMask & 2) != 0) {
        const float dot = delta.x * viewContext->worldFrustumNormals[1].x
            + delta.y * viewContext->worldFrustumNormals[1].y + delta.z * viewContext->worldFrustumNormals[1].z;
        if (-radius >= dot) {
            return 2;
        }
        if (dot < radius) {
            *clipMaskInOut |= 2;
        }
    }

    if ((oldMask & 4) != 0) {
        const float dot = delta.x * viewContext->worldFrustumNormals[2].x
            + delta.y * viewContext->worldFrustumNormals[2].y + delta.z * viewContext->worldFrustumNormals[2].z;
        if (-radius >= dot) {
            return 4;
        }
        if (dot < radius) {
            *clipMaskInOut |= 4;
        }
    }

    if ((oldMask & 8) != 0) {
        const float dot = delta.x * viewContext->worldFrustumNormals[3].x
            + delta.y * viewContext->worldFrustumNormals[3].y + delta.z * viewContext->worldFrustumNormals[3].z;
        if (-radius >= dot) {
            return 8;
        }
        if (dot < radius) {
            *clipMaskInOut |= 8;
        }
    }

    if ((oldMask & 0x20) != 0) {
        viewContext = g_zVideo_pActiveProjectionViewContext;
        delta.x = sphereCenter->x - viewContext->farClipCenter.x;
        delta.y = sphereCenter->y - viewContext->farClipCenter.y;
        delta.z = sphereCenter->z - viewContext->farClipCenter.z;
        const float dot = delta.x * viewContext->worldFrustumNormals[5].x
            + delta.y * viewContext->worldFrustumNormals[5].y + delta.z * viewContext->worldFrustumNormals[5].z;
        if (-radius >= dot) {
            return 0x20;
        }
        if (dot < radius) {
            *clipMaskInOut |= 0x20;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-instance-updatescrollingtexturesifneeded
 * @recoil-artifact defines .text recoil:function:0x478fc0: zModelInstanceUpdateScrollingTexturesIfNeeded
 * @recoil-match byte
 *
 * Purpose: update all scrolling-texture surface entries once per video frame.
 */
int __fastcall zModelInstanceUpdateScrollingTexturesIfNeeded(zModel_InstancePartial* instance)
{
    if (instance == 0) {
        return -1;
    }

    if (instance->scrollingTextureFrameTick == g_zVideo_FrameTick) {
        return 0;
    }

    instance->scrollingTextureFrameTick = g_zVideo_FrameTick;
    zModel_InstanceSurfaceEntryPartial* entry = instance->surfaceEntries;
    for (int i = 0; i < instance->surfaceEntryCount; ++i, ++entry) {
        const int vertexCount = (int)(entry->vertexCountAndFlags & 0xff);
        zModel_MaterialTextureBindingPartial* material = entry->materialBinding;
        if ((material->flags & 0x0100) == 0) {
            continue;
        }

        zModelInstanceUpdateScrollingTextures(
            material->textureRef->textureInfo,
            entry->uvs,
            &instance->scrollRateU,
            vertexCount
        );
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-renderpointqueueentry
 * @recoil-artifact defines .text recoil:function:0x479020: zModelRenderPointQueueEntry
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
 *
 *
 * Purpose: project and submit one display-instance point/lens-flare queue entry.
 * Retail note: the lens-flare test takes the address of lensFlareEnabled
 * (lea/test), not its value; it is preserved as recovered, not cleaned up.
 */
void __fastcall
zModelRenderPointQueueEntry(const zVec3* pointPos, unsigned short packedColor16, zModel_PointEntryPartial* pointEntry)
{
    zVec3 transformedPoint;
    ZMTH_MAT_TRANSFORM_POINT_BATCH(pointPos, &transformedPoint, 1);

    if (transformedPoint.z <= gClipRect_Primary.zMin) {
        return;
    }

    zProjectedPoint projectedPoint;
    if (g_zVideo_ActiveRendererPath != 0) {
        zMathProjectSphereBatch(&transformedPoint, (zProjectedSphere*)(&projectedPoint), 1);
    } else {
        zMath::ProjectPointBatch(&transformedPoint, &projectedPoint, 1);
    }

    if (!ProjectedPointInClipBounds(projectedPoint)) {
        return;
    }

    if (g_zVideo_ActiveRendererPath != 0) {
        const int depthBias = (short)(pointEntry->depthBiasWord & 0xffff);
        projectedPoint.reciprocalZ
            = (((float)(depthBias)*g_zRndr_InverseZTolerance) + 1.0f) * projectedPoint.reciprocalZ;
        g_zVideo_pfnDrawPointColor16((zVideo_XyzVertex*)(&projectedPoint), packedColor16, 1);
        if (&((zRndr_LensFlareSource*)pointEntry->lensFlareSource)->lensFlareEnabled != 0) {
            zRndrLensFlareQueueProjectedSample(&projectedPoint, packedColor16, (int)(&pointEntry->lensFlareSource[0]));
        }
    } else {
        zRndrLensFlareQueueProjectedSample(&projectedPoint, packedColor16, (int)(&pointEntry->lensFlareSource[0]));
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-instance-updatescrollingtextures
 * @recoil-artifact defines .text recoil:function:0x4791c0: zModelInstanceUpdateScrollingTextures
 *
 *
 * Purpose: advance scrolling texture UVs for one surface entry and wrap them into range.
 */
void __fastcall zModelInstanceUpdateScrollingTextures(
    const zModel_TextureScrollInfoPartial* textureInfo,
    zModel_Uv* uvs,
    const float* scrollRates,
    int uvCount
)
{
    const unsigned int wrapExtent = g_zVideo_ActiveRendererPath != 0 ? 0x80 : 0x800;
    int i;

    if (scrollRates[0] != 0.0f && scrollRates[1] != 0.0f) {
        const float deltaU = scrollRates[0] * g_FrameDeltaTimeSec;
        const float deltaV = g_FrameDeltaTimeSec * scrollRates[1];
        uvs[0].u = deltaU + uvs[0].u;
        uvs[0].v = deltaV + uvs[0].v;
        float minU = uvs[0].u;
        float maxU = uvs[0].u;
        float minV = uvs[0].v;
        float maxV = uvs[0].v;
        for (i = 1; i < uvCount; ++i) {
            uvs[i].u = deltaU + uvs[i].u;
            uvs[i].v = deltaV + uvs[i].v;
            if (uvs[i].u < minU) {
                minU = uvs[i].u;
            } else if (uvs[i].u > maxU) {
                maxU = uvs[i].u;
            }
            if (uvs[i].v < minV) {
                minV = uvs[i].v;
            } else if (uvs[i].v > maxV) {
                maxV = uvs[i].v;
            }
        }

        const int floorMinU = (int)floor(minU);
        const int floorMinV = (int)floor(minV);
        const int ceilMaxU = (int)ceil(maxU);
        const int ceilMaxV = (int)ceil(maxV);
        const int extentU = wrapExtent >> textureInfo->wrapShiftU;
        const int extentV = wrapExtent >> textureInfo->wrapShiftV;
        int correctionU = 0;
        int correctionV = 0;
        if (floorMinU <= -extentU) {
            correctionU = extentU - ceilMaxU;
        } else if (ceilMaxU >= extentU) {
            correctionU = -(floorMinU + extentU);
        }
        if (floorMinV <= -extentV) {
            correctionV = extentV - ceilMaxV;
        } else if (ceilMaxV >= extentV) {
            correctionV = -(floorMinV + extentV);
        }

        if (correctionU != 0 && correctionV != 0) {
            for (i = 0; i < uvCount; ++i) {
                uvs[i].u += (float)correctionU;
                uvs[i].v += (float)correctionV;
            }
        } else if (correctionU != 0) {
            for (i = 0; i < uvCount; ++i) {
                uvs[i].u += (float)correctionU;
            }
        } else if (correctionV != 0) {
            for (i = 0; i < uvCount; ++i) {
                uvs[i].v += (float)correctionV;
            }
        }
    } else if (scrollRates[0] != 0.0f) {
        const float deltaU = scrollRates[0] * g_FrameDeltaTimeSec;
        float minU = deltaU + uvs[0].u;
        float maxU = minU;
        uvs[0].u = minU;
        for (i = 1; i < uvCount; ++i) {
            uvs[i].u = deltaU + uvs[i].u;
            if (uvs[i].u < minU) {
                minU = uvs[i].u;
            } else if (uvs[i].u > maxU) {
                maxU = uvs[i].u;
            }
        }

        const int floorMinU = (int)floor(minU);
        const int ceilMaxU = (int)ceil(maxU);
        const int extentU = wrapExtent >> textureInfo->wrapShiftU;
        int correctionU = 0;
        if (floorMinU <= -extentU) {
            correctionU = extentU - ceilMaxU;
        } else if (ceilMaxU >= extentU) {
            correctionU = -(floorMinU + extentU);
        }

        if (correctionU != 0) {
            for (i = 0; i < uvCount; ++i) {
                uvs[i].u += (float)correctionU;
            }
        }
    } else if (scrollRates[1] != 0.0f) {
        const float deltaV = g_FrameDeltaTimeSec * scrollRates[1];
        float minV = deltaV + uvs[0].v;
        float maxV = minV;
        uvs[0].v = minV;
        for (i = 1; i < uvCount; ++i) {
            uvs[i].v = deltaV + uvs[i].v;
            if (uvs[i].v < minV) {
                minV = uvs[i].v;
            } else if (uvs[i].v > maxV) {
                maxV = uvs[i].v;
            }
        }

        const int floorMinV = (int)floor(minV);
        const int ceilMaxV = (int)ceil(maxV);
        const int extentV = wrapExtent >> textureInfo->wrapShiftV;
        int correctionV = 0;
        if (floorMinV <= -extentV) {
            correctionV = extentV - ceilMaxV;
        } else if (ceilMaxV >= extentV) {
            correctionV = -(floorMinV + extentV);
        }

        if (correctionV != 0) {
            for (i = 0; i < uvCount; ++i) {
                uvs[i].v += (float)correctionV;
            }
        }
    }
}

namespace OptCatalog
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-optcatalog-applydamagemaskstamponhit
     * @recoil-artifact defines .text recoil:function:0x479660: OptCatalog::ApplyDamageMaskStampOnHit
     *
     *
     * Purpose: stamp the active damage mask onto an eligible OptCatalog hit surface.
     */
    void __fastcall ApplyDamageMaskStampOnHit(OptCatalogHitEventPartial * hitEvent)
    {
        if (OptCatalogIsDamageMaskEnabled() == 0) {
            return;
        }

        OptCatalogSurfaceMaterialRef* const surfaceRef = hitEvent->surfaceRef;
        if (surfaceRef == 0) {
            return;
        }

        const unsigned int materialFlags = surfaceRef->flags;
        if ((materialFlags & 0x0100) == 0 || (materialFlags & 0x0200) == 0 || (materialFlags & 0x0400) != 0) {
            return;
        }

        while (g_OptCatalogDamageMaskPhaseU > 1.01f) {
            g_OptCatalogDamageMaskPhaseU -= 1.0f;
        }
        while (g_OptCatalogDamageMaskPhaseU < -0.01f) {
            g_OptCatalogDamageMaskPhaseU += 1.0f;
        }
        while (g_OptCatalogDamageMaskPhaseV > 1.01f) {
            g_OptCatalogDamageMaskPhaseV -= 1.0f;
        }
        while (g_OptCatalogDamageMaskPhaseV < -0.01f) {
            g_OptCatalogDamageMaskPhaseV += 1.0f;
        }

        OptCatalogSurfaceTextureHandle* const srcHandle
            = (OptCatalogSurfaceTextureHandle*)g_OptCatalogDamageMaskHandles[g_OptCatalogDamageMaskSlotIndex];
        OptCatalogDamageMaskSurface* const srcSurface = srcHandle != 0 ? srcHandle->surface : 0;
        OptCatalogSurfaceTextureHandle* const dstHandle = surfaceRef->textureHandle;
        OptCatalogDamageMaskSurface* const dstSurface = dstHandle != 0 ? dstHandle->surface : 0;
        if (srcSurface == 0 || dstSurface == 0 || srcSurface->format != 0 || dstSurface->format != 0) {
            return;
        }

        const int dstWidth = dstSurface->width;
        const int dstHeight = dstSurface->height;
        const int srcWidth = srcSurface->width;
        const int srcHeight = srcSurface->height;
        int dstX = (int)(dstWidth * g_OptCatalogDamageMaskPhaseU) - (srcWidth >> 1);
        int dstY = (int)(dstHeight * g_OptCatalogDamageMaskPhaseV) - (srcHeight >> 1);
        int srcXBegin = 0;
        int srcXEnd = 0;
        int srcYBegin = 0;
        int srcYEnd = 0;
        if (srcWidth > dstWidth) {
            dstX = 0;
            srcXBegin = (srcWidth - dstWidth) >> 1;
            srcXEnd = srcWidth - srcXBegin;
        } else {
            srcXBegin = 0;
            srcXEnd = srcWidth;
            if (dstX < 0) {
                dstX = 0;
                srcXEnd = srcWidth;
            } else if (dstX + srcWidth > dstWidth) {
                dstX = dstX - (dstX + srcWidth) + dstWidth;
            }
        }
        if (srcHeight > dstHeight) {
            dstY = 0;
            srcYBegin = (srcHeight - dstHeight) >> 1;
            srcYEnd = srcHeight - srcYBegin;
        } else {
            srcYBegin = 0;
            srcYEnd = srcHeight;
            if (dstY < 0) {
                dstY = 0;
                srcYEnd = srcHeight;
            } else if (dstY + srcHeight > dstHeight) {
                dstY = dstY - (dstY + srcHeight) + dstHeight;
            }
        }

        unsigned short* dstPixels = dstSurface->pixels;
        int dstStride = dstWidth;
        const bool hasTextureRecord = dstHandle->textureRecord != 0;
        if (hasTextureRecord) {
            if (g_zVideo_pfnTextureRecordLockUploadSurface(dstHandle->textureRecord, (void**)&dstPixels, &dstStride)
                == 0) {
                return;
            }
            dstStride >>= 1;
        }

        if (srcSurface->alpha == 0) {
            for (int srcY = srcYBegin, outY = dstY; srcY < srcYEnd; ++srcY, ++outY) {
                unsigned short* dst = dstPixels + outY * dstWidth + dstX;
                unsigned short* src = srcSurface->pixels + srcY * srcWidth + srcXBegin;
                for (int srcX = srcXBegin; srcX < srcXEnd; ++srcX, ++src) {
                    if (*src != 0) {
                        *dst = *src;
                    }
                    ++dst;
                }
            }
        } else if (zRndr::g_pixelPackGreenBits == 6) {
            for (int srcY = srcYBegin, outY = dstY; srcY < srcYEnd; ++srcY, ++outY) {
                unsigned short* dst = dstPixels + outY * dstStride + dstX;
                unsigned short* src = srcSurface->pixels + srcY * srcWidth + srcXBegin;
                unsigned char* alpha = srcSurface->alpha + srcY * srcWidth + srcXBegin;
                for (int srcX = srcXBegin; srcX < srcXEnd; ++srcX, ++src, ++alpha, ++dst) {
                    const int alphaValue = *alpha;
                    if (alphaValue == 0 || alphaValue <= 3) {
                        continue;
                    }
                    if (alphaValue >= 0xfc) {
                        *dst = *src;
                    } else {
                        const unsigned int dstPixel = *dst;
                        const unsigned int srcPixel = *src;
                        unsigned int blended = dstPixel;
                        blended += ((((srcPixel & 0xf800) - (dstPixel & 0xf800)) * alphaValue) >> 8) & 0xfffff800;
                        const unsigned int green
                            = ((((srcPixel & 0x07e0) - (dstPixel & 0x07e0)) * alphaValue) >> 8) & 0xffffffe0;
                        const unsigned int blue = (((srcPixel & 0x001f) - (blended & 0x001f)) * alphaValue) >> 8;
                        *dst = (unsigned short)(blended + green + blue);
                    }
                }
            }
        } else {
            for (int srcY = srcYBegin, outY = dstY; srcY < srcYEnd; ++srcY, ++outY) {
                unsigned short* dst = dstPixels + outY * dstStride + dstX;
                unsigned short* src = srcSurface->pixels + srcY * srcWidth + srcXBegin;
                unsigned char* alpha = srcSurface->alpha + srcY * srcWidth + srcXBegin;
                for (int srcX = srcXBegin; srcX < srcXEnd; ++srcX, ++src, ++alpha, ++dst) {
                    const int alphaValue = *alpha;
                    if (alphaValue == 0 || alphaValue <= 7) {
                        continue;
                    }
                    if (alphaValue >= 0xfc) {
                        *dst = *src;
                    } else {
                        const unsigned int dstPixel = *dst;
                        const unsigned int srcPixel = *src;
                        const unsigned int red
                            = ((((srcPixel & 0x7c00) - (dstPixel & 0x7c00)) * alphaValue) >> 8) & 0xfffffc00;
                        const unsigned int green
                            = ((((srcPixel & 0x03e0) - (dstPixel & 0x03e0)) * alphaValue) >> 8) & 0xffffffe0;
                        const unsigned int blue = (((srcPixel & 0x001f) - (dstPixel & 0x001f)) * alphaValue) >> 8;
                        *dst = (unsigned short)(dstPixel + red + green + blue);
                    }
                }
            }
        }

        if (hasTextureRecord) {
            g_zVideo_pfnTextureRecordUnlockUploadSurface(dstHandle->textureRecord);
            g_zVideo_pfnTextureRecordFinalizeUpload(dstHandle->textureRecord, &dstX, 0);
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-optcatalog-setdamagemaskslotindex
     * @recoil-artifact defines .text recoil:function:0x479c50: OptCatalog::SetDamageMaskSlotIndex
     * @recoil-match byte
     *
     * Purpose: select the active damage-mask handle slot.
     */
    void __fastcall SetDamageMaskSlotIndex(int slotIndex)
    {
        g_OptCatalogDamageMaskSlotIndex = slotIndex;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-optcatalog-registerdamagemaskslotptr
     * @recoil-artifact defines .text recoil:function:0x479c60: OptCatalog::RegisterDamageMaskSlotPtr
     * @recoil-match byte
     *
     * Purpose: register a damage-mask texture handle in the active OptCatalog slot.
     */
    void __fastcall RegisterDamageMaskSlotPtr(void* slotPtr)
    {
        g_OptCatalogDamageMaskEnabled = 1;
        g_OptCatalogDamageMaskHandles[g_OptCatalogDamageMaskSlotIndex] = slotPtr;
    }
} // namespace OptCatalog

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-optcatalog-isdamagemaskenabled
 * @recoil-artifact defines .text recoil:function:0x479c80: OptCatalogIsDamageMaskEnabled
 * @recoil-match byte
 *
 * Purpose: report whether OptCatalog damage-mask stamping is currently enabled.
 */
int __cdecl OptCatalogIsDamageMaskEnabled()
{
    return g_OptCatalogDamageMaskEnabled;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-optcatalog-setdamagemaskuv
 * @recoil-artifact defines .text recoil:function:0x479c90: OptCatalogSetDamageMaskUv
 * @recoil-match byte
 *
 * Purpose: set the current damage-mask UV phase used by the OptCatalog stamp pass.
 */
void __stdcall OptCatalogSetDamageMaskUv(float u, float v)
{
    g_OptCatalogDamageMaskPhaseU = u;
    g_OptCatalogDamageMaskPhaseV = v;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-optcatalog-setdamagemaskenabled
 * @recoil-artifact defines .text recoil:function:0x479cb0: OptCatalogSetDamageMaskEnabled
 * @recoil-match byte
 *
 * Purpose: update the global OptCatalog damage-mask enable flag.
 */
void __fastcall OptCatalogSetDamageMaskEnabled(int enabled)
{
    g_OptCatalogDamageMaskEnabled = enabled;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-optcatalog-isdamagemaskslotptrregistered
 * @recoil-artifact defines .text recoil:function:0x479cc0: OptCatalogIsDamageMaskSlotPtrRegistered
 * @recoil-match byte
 *
 * Purpose: test whether a damage-mask slot already references the supplied handle.
 */
int __fastcall OptCatalogIsDamageMaskSlotPtrRegistered(void* slotPtr)
{
    for (int i = 0; i < 3; ++i) {
        if (g_OptCatalogDamageMaskHandles[i] == slotPtr) {
            return 1;
        }
    }

    return 0;
}
