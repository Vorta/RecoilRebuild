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
#include "zclass.h"
#include <math.h>
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
        int vertexCount,
        int renderParam,
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
 * Original inline helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp); no standalone retail body.
 * Purpose: compute the polygon facing normal and apply backface/show-backface culling.
 * Keep the edge vectors as aggregates: VC5 scalar-temporary reuse corrupts
 * the normal Z calculation when these are six independent float locals.
 * Retail forms the facing value with the reviewed ZMTH_VECTOR_DOT island
 * (normal, first scratch vertex) in both render paths.
 */
/**
 * Purpose: backface facing dot of the surface normal with the first scratch vertex.
 * Raw assembly: the reviewed full-XYZ ZMTH_VECTOR_DOT island, Pro-reviewed for the
 * expansions in RenderNodeSoftware and RenderNodeHardware (run 2026-10-05T18-30-40).
 * The VC5/x86 branch passes the first zClipVert (layout x, y, z) to the island as a
 * read-only zVec3 operand; the C branch reads the vertex through its own type.
 */
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
#define GMOD_DRAW_FACING_DOT(result, normal, vertex) ZMTH_VECTOR_DOT(result, normal, (const zVec3*)(&(vertex)))
#else
#define GMOD_DRAW_FACING_DOT(result, normal, vertex)                                                                   \
    ((result) = (normal)->x * (vertex).x + (normal)->y * (vertex).y + (normal)->z * (vertex).z)
#endif

#define ComputeSurfaceNormalAndCull(vertexCount, showBackFace, outNormal, outScanConvertMode, visible)                 \
    do {                                                                                                               \
        (visible) = 0;                                                                                                 \
        if ((vertexCount) >= 3) {                                                                                      \
            if ((outScanConvertMode) != 0) {                                                                           \
                *((int*)(outScanConvertMode)) = 1;                                                                     \
            }                                                                                                          \
            const zVec3& v0 = g_Clip_PolyVertsScratch[0];                                                              \
            const zVec3& v1 = g_Clip_PolyVertsScratch[1];                                                              \
            const zVec3& v2 = g_Clip_PolyVertsScratch[2];                                                              \
            const zVec3 edgeA = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };                                             \
            const zVec3 edgeB = { v0.x - v1.x, v0.y - v1.y, v0.z - v1.z };                                             \
            (outNormal)->x = edgeB.z * edgeA.y - edgeB.y * edgeA.z;                                                    \
            (outNormal)->y = edgeB.x * edgeA.z - edgeB.z * edgeA.x;                                                    \
            (outNormal)->z = edgeB.y * edgeA.x - edgeB.x * edgeA.y;                                                    \
            float facing;                                                                                              \
            GMOD_DRAW_FACING_DOT(facing, (outNormal), v0);                                                             \
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
 * Representation contract: g_Clip_PolyVertsScratch holds zVec3 records (gmod.h) and
 * g_Clip_PolyVerts is a zClipVert array (zclip_rect.h: three floats, sizeof 0x0c
 * asserted); the zMath batch projection and the zRndr submit entries take
 * zVec3/zProjectedPoint views of the clip storage (zmth_types.h: three floats
 * each). Retail passes the array addresses (0x57c5c4/0x57c8c4) directly and the
 * callees access only the float members at +0/+4/+8 with a 0x0c stride.
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
            (rejected) = fabs(twiceArea) < gModel_SmallPolyRejectArea2x ? 1 : 0;                                       \
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
            (verts)[remapIndex].x = g_zClipAlt_RemapScaleX * (verts)[remapIndex].x + g_zClipAlt_RemapBiasX;            \
            (verts)[remapIndex].y = g_zClipAlt_RemapScaleY * (verts)[remapIndex].y + g_zClipAlt_RemapBiasY;            \
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
     * @recoil-match byte
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
     * @recoil-match byte
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
     * @recoil-match byte
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
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
     *
     *
     * Raw assembly: the reviewed vector-dot island in the per-entry backface
     * test (GMOD_DRAW_FACING_DOT, retail [0x4772ba,0x4772d9)), and the reviewed
     * transform-point island in the two ZMTH_MAT_TRANSFORM_POINT_BATCH expansions,
     * retail [0x476dca,0x476e2e) (mode 2) and [0x47700a,0x47706e) (main path);
     * Pro run 2026-10-07T09-38-31, rows Z5a/Z5b.
     *
     *
     * Purpose: render a display-instance node through the software renderer path.
     * Retail clears eax on both exits, so the renderer entry returns 0.
     */
    int __fastcall RenderNodeSoftware(CZNodePartial * node, int clipMask)
    {
        zDiPartial* const di = (zDiPartial*)(node->userDataOrDiRef);
        zMat4x3 matrixScratch;
        int outDepthFade;
        int outActiveLightState;
        int outLensFlareVisible;

        zMath::MatStackPushPtr((float*)(&matrixScratch));
        switch (di->mode) {
        default:
            zMathMatSetupCamera();
            zRndr::g_perspectiveTextureEnabled = 0;
            break;
        case 2: {
            zMathMatSetupCamera();
            if ((di->flags & 8) != 0 && di->blendScale != 0.0 && di->blendVertCount != 0) {
                zMathVec3ArrayAddScaled(
                    g_zModel_TransformedVerts,
                    di->verts,
                    di->blendVerts,
                    di->blendVertCount,
                    di->blendScale
                );
                zMath::MatTransformPointBatchInPlace(g_zModel_TransformedVerts, di->vertCount);
            } else {
                // Retail calls the batch with no count test: requires vertCount > 0, verts and
                // g_zModel_TransformedVerts each holding vertCount elements, no overlap.
                ZMTH_MAT_TRANSFORM_POINT_BATCH(di->verts, g_zModel_TransformedVerts, di->vertCount);
            }

            // Retail reads the first entry's material colour without a null test.
            const unsigned int pointColor = di->entries->material->packedColor;
            for (int vertexIndex = 0; vertexIndex < di->vertCount; ++vertexIndex) {
                zVec3* const transformed = &g_zModel_TransformedVerts[vertexIndex];
                if (transformed->z > gClipRect_Primary.zMin) {
                    zProjectedPoint projectedPoint;
                    const int rendererPath = g_zVideo_ActiveRendererPath;
                    if (rendererPath != 0) {
                        zMathProjectSphereBatch(transformed, (zProjectedSphere*)(&projectedPoint), 1);
                    } else {
                        zMath::ProjectPointBatch(transformed, &projectedPoint, 1);
                    }

                    if (ProjectedPointInClipBounds(projectedPoint)) {
                        if (rendererPath != 0) {
                            g_zVideo_pfnDrawPointColor16((zVideo_XyzVertex*)(&projectedPoint), pointColor, 1);
                        } else {
                            zRndrLensFlareQueueProjectedSample(&projectedPoint, (int)pointColor, 0);
                        }
                    }
                }
            }

            zMath::MatStackPopPtr();
            zRndr::g_perspectiveTextureEnabled = 0;
            return 0;
        }
        case 1:
            if ((di->flags & 0x10) != 0) {
                zMathMatLoadView();
                zRndr::g_perspectiveTextureEnabled = 0;
            } else {
                zMathMatLoadProjection(g_zVideo_pActiveProjectionViewContext->eulerAngles.y);
                zRndr::g_perspectiveTextureEnabled = 0;
            }
            break;
        case 0:
            zMathMatSetupCamera();
            zRndr::g_perspectiveTextureEnabled = 1;
            break;
        }

        if (di->entryCount > 0) {
            zDi::EvalBoundingSphereLightingFlags(di, &outDepthFade, &outActiveLightState, &outLensFlareVisible);
            if ((di->flags & 8) != 0 && di->blendScale != 0.0 && di->blendVertCount != 0) {
                zMathVec3ArrayAddScaled(
                    g_zModel_TransformedVerts,
                    di->verts,
                    di->blendVerts,
                    di->blendVertCount,
                    di->blendScale
                );
                zMath::MatTransformPointBatchInPlace(g_zModel_TransformedVerts, di->vertCount);
            } else if (di->vertCount != 0) {
                // The != 0 test is retail's; the batch still requires vertCount > 0, verts and
                // g_zModel_TransformedVerts each holding vertCount elements, no overlap.
                ZMTH_MAT_TRANSFORM_POINT_BATCH(di->verts, g_zModel_TransformedVerts, di->vertCount);
            }
        }

        for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex) {
            ApplySoftwareDepthScale(di->pointEntries[pointIndex].depthBiasWord);
            zModel_PointEntryPartial* const pointEntry = &di->pointEntries[pointIndex];
            zVec3* const pointCams = pointEntry->pointCamList;
            switch (pointEntry->mode) {
            case 1:
                pointEntry->elapsedTime += g_FrameDeltaTimeSec;
                if (pointEntry->elapsedTime > pointEntry->timerSec) {
                    ++pointEntry->pointCamPackedState;
                    if ((unsigned short)pointEntry->pointCamPackedState >= pointEntry->pointCamCount) {
                        pointEntry->pointCamPackedState &= 0xffff0000;
                    }
                    pointEntry->elapsedTime = 0.0f;
                }
                zModelRenderPointQueueEntry(
                    &pointCams[(unsigned short)pointEntry->pointCamPackedState],
                    pointEntry->packedColor16,
                    pointEntry
                );
                if ((unsigned short)pointEntry->pointCamPackedState > 0) {
                    zModelRenderPointQueueEntry(
                        &pointEntry->pointCamList[(unsigned short)pointEntry->pointCamPackedState - 1],
                        (unsigned short)((unsigned int)pointEntry->pointCamPackedState >> 16),
                        pointEntry
                    );
                }
                break;
            case 0:
                if (pointEntry->behavior == 1) {
                    pointEntry->elapsedTime += g_FrameDeltaTimeSec;
                    if (pointEntry->elapsedTime > pointEntry->timerSec) {
                        const unsigned short packedColor = pointEntry->packedColor16;
                        const unsigned int packedState = (unsigned int)pointEntry->pointCamPackedState;
                        pointEntry->elapsedTime = 0.0f;
                        pointEntry->packedColor16 = (unsigned short)(packedState >> 16);
                        pointEntry->pointCamPackedState
                            = (int)((packedState & 0xffff) | ((unsigned int)packedColor << 16));
                    }
                }
                zModelRenderPointQueueEntry(pointCams, pointEntry->packedColor16, pointEntry);
                break;
            }
        }

        gClipRect_Primary.flags = clipMask;
        zDiEntryPartial* entry = di->entries;
        zDiEntryPartial* const entriesEnd = &entry[di->entryCount];
        for (; entry < entriesEnd; ++entry) {
            zRndrSetPaletteRemapKeyFromRgb01(0, 0.0f);
            zRndrSetPaletteRemapKey(0, 0.0f);
            zRndrSetPaletteShadeRecipeIndex(0);

            {
                // Retail gathers whole zVec3 records without index or count validation (count-down copy).
                int copyCount = (int)(entry->flagsAndIndexCount & 0xff);
                const int* copyIndices = (const int*)(entry->vertexIndices);
                zVec3* copyDest = g_Clip_PolyVertsScratch;
                const zVec3* const copySource = g_zModel_TransformedVerts;
                do {
                    *copyDest++ = copySource[*copyIndices++];
                } while (--copyCount);
            }

            zVec3 surfaceNormal;
            {
                const zVec3& v0 = g_Clip_PolyVertsScratch[0];
                const zVec3& v1 = g_Clip_PolyVertsScratch[1];
                const zVec3& v2 = g_Clip_PolyVertsScratch[2];
                const zVec3 edgeA = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
                const zVec3 edgeB = { v0.x - v1.x, v0.y - v1.y, v0.z - v1.z };
                surfaceNormal.x = edgeB.z * edgeA.y - edgeB.y * edgeA.z;
                surfaceNormal.y = edgeB.x * edgeA.z - edgeB.z * edgeA.x;
                surfaceNormal.z = edgeB.y * edgeA.x - edgeB.x * edgeA.y;
            }
            const int showBackFace = (int)((entry->flagsAndIndexCount >> 8) & 1);
            float facing;
            GMOD_DRAW_FACING_DOT(facing, &surfaceNormal, g_Clip_PolyVertsScratch[0]);
            int scanConvertMode;
            if (facing < -g_zModel_BFETolerance) {
                scanConvertMode = 1;
            } else {
                if (showBackFace == 0 || !(facing > g_zModel_BFETolerance)) {
                    continue;
                }
                surfaceNormal.x = -surfaceNormal.x;
                surfaceNormal.y = -surfaceNormal.y;
                surfaceNormal.z = -surfaceNormal.z;
                scanConvertMode = 0;
            }

            int clippedCount = (int)(entry->flagsAndIndexCount & 0xff);
            zModel_MaterialPartial* const material = entry->material;
            int lightsEnabled = outActiveLightState;
            // Loop-level per-entry state, matching retail's frame: one depth scale for the three submit
            // paths ([ebp-0x10]), the triangle copy below edgeB ([ebp-0x80]) and the textured path's
            // lighting mode/remap key ([ebp-0x48]/[ebp-0x18]).
            zVec3 triClipVerts[3];
            float depthScale;
            int lightingMode;
            int remapKey;
            if ((material->flags & 0x0100) != 0) {
                if ((material->flags & 0x0400) != 0) {
                    zModel_Material::UpdateCycleIfNeeded(material);
                }

                int shadeMode;
                int shadeTexture;
                if (((material->currentTextureDirectoryEntry != 0 ? material->currentTextureDirectoryEntry->image : 0)
                            ->formatFlagsPacked
                        & 2)
                    != 0) {
                    if (g_zModel_SoftwarePathActive != 0) {
                        shadeTexture = 1;
                        lightingMode = 0;
                    } else {
                        shadeTexture = 0;
                    }
                } else {
                    shadeTexture = 1;
                    lightingMode = 1;
                }

                if (shadeTexture != 0) {
                    for (int i = 0; i < clippedCount; ++i) {
                        g_Clip_PolyAttr1[i] = 0.0f;
                        g_Clip_PolyAttr0[i] = 0.0f;
                    }
                    shadeMode = 0;
                    remapKey = 0;
                    if (outDepthFade != 0 && zModel_Light::BuildAttr0DepthFade(clippedCount, &remapKey) != 0) {
                        remapKey &= lightingMode;
                        if (g_zModel_SoftwarePathActive != 0 && remapKey == 0
                            && material->currentTextureDirectoryEntry->image->palette != 0) {
                            zRndrSetPaletteRemapKey(&gModel_AmbientPaletteRemapRecipe, g_Clip_PolyAttr0[0]);
                        } else {
                            zRndrSetPaletteShadeRecipeIndex(&gModel_AmbientPaletteRemapRecipe);
                            shadeMode = 1;
                        }
                        lightsEnabled = 0;
                    }
                    if (lightsEnabled != 0
                        && zModel_Light::SetActiveLights(
                               &surfaceNormal,
                               clippedCount,
                               &shadeMode,
                               &lightingMode,
                               (int)(material->currentTextureDirectoryEntry->image->palette)
                           ) != 0) {
                        remapKey |= lightingMode;
                        shadeMode = 2;
                    }
                    if (shadeMode == 1) {
                        zRndr::CommitFogColorParamsIfChanged();
                    }
                } else {
                    shadeMode = 0;
                }

                memcpy(g_Clip_PolyUvs, entry->uvPairs, (size_t)clippedCount * sizeof(zClipUV));

                if (shadeMode != 0) {
                    if ((clipMask & 0x30) != 0
                        && zClipRect::ClipPolyNearZ_WithAttr0(&gClipRect_Primary, &clippedCount) == 0) {
                        continue;
                    }
                    zRndr::g_scanConvertMode = scanConvertMode;
                    ProjectScratchToClipVerts(clippedCount);
                    {
                        const zClipVert* current = g_Clip_PolyVerts;
                        float twiceArea = 0.0f;
                        const zClipVert* previous = &g_Clip_PolyVerts[clippedCount - 1];
                        int areaCount = clippedCount;
                        do {
                            twiceArea += current->y * previous->x - previous->y * current->x;
                            previous = current;
                            ++current;
                        } while (--areaCount);
                        if (fabsf(twiceArea) < gModel_SmallPolyRejectArea2x) {
                            continue;
                        }
                    }
                    memcpy(triClipVerts, g_Clip_PolyVerts, sizeof(triClipVerts));
                    if ((clipMask & 0x0f) != 0
                        && zClipRect::ClipPoly_NoUV_WithAttr0_Alt(&gClipRect_Primary, &clippedCount) == 0) {
                        continue;
                    }

                    const int vertexCount = clippedCount;
                    depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                    zRndr::g_inverseDepthBias = 0.0f;
                    zRndr::g_inverseDepthScale = depthScale;
                    zRndrSubmitTexturedPolyPerVertexAlphaOrShade(
                        (zVec3*)g_Clip_PolyVerts,
                        (zVec3*)g_Clip_PolyVertsScratch,
                        triClipVerts,
                        (zVec2*)g_Clip_PolyUvs,
                        g_Clip_PolyAttr0,
                        0,
                        vertexCount,
                        material->currentTextureDirectoryEntry,
                        remapKey,
                        gModel_RenderVertexAlphaEnabled
                    );

                    clippedCount = vertexCount;
                    if (gAltClipPassEnabled != 0 && zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, vertexCount) != 0
                        && zClipRect::ClipPoly_NoUV(&gClipRect_Alt, &clippedCount) != 0) {
                        RemapAltProjectedVerts(triClipVerts, 3);
                        RemapAltProjectedVerts(g_Clip_PolyVerts, clippedCount);
                        zRndr::g_inverseDepthBias = gClipRect_Primary.zMin;
                        zRndr::g_inverseDepthScale = depthScale;
                        if (shadeMode != 2) {
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
                                remapKey,
                                gModel_RenderVertexAlphaEnabled
                            );
                        }
                    }
                    continue;
                }

                if ((clipMask & 0x30) != 0 && zClipRect::ClipPolyNearZ(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }
                zRndr::g_scanConvertMode = scanConvertMode;
                ProjectScratchToClipVerts(clippedCount);
                {
                    const zClipVert* current = g_Clip_PolyVerts;
                    float twiceArea = 0.0f;
                    const zClipVert* previous = &g_Clip_PolyVerts[clippedCount - 1];
                    int areaCount = clippedCount;
                    do {
                        twiceArea += current->y * previous->x - previous->y * current->x;
                        previous = current;
                        ++current;
                    } while (--areaCount);
                    if (fabsf(twiceArea) < gModel_SmallPolyRejectArea2x) {
                        continue;
                    }
                }
                memcpy(triClipVerts, g_Clip_PolyVerts, sizeof(triClipVerts));
                if ((clipMask & 0x0f) != 0 && zClipRect::ClipPoly_NoUV(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }

                const int vertexCount = clippedCount;
                depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                zRndr::g_inverseDepthBias = 0.0f;
                zRndr::g_inverseDepthScale = depthScale;
                zRndrSubmitTexturedPolyUniformAlphaOrShade(
                    (zVec3*)g_Clip_PolyVerts,
                    (zVec3*)g_Clip_PolyVertsScratch,
                    triClipVerts,
                    (zVec2*)g_Clip_PolyUvs,
                    vertexCount,
                    material->currentTextureDirectoryEntry,
                    gModel_RenderAlphaScaleCurrent,
                    gModel_RenderVertexAlphaEnabled
                );

                clippedCount = vertexCount;
                if (gAltClipPassEnabled != 0 && zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, vertexCount) != 0
                    && zClipRect::ClipPoly_NoUV(&gClipRect_Alt, &clippedCount) != 0) {
                    RemapAltProjectedVerts(triClipVerts, 3);
                    RemapAltProjectedVerts(g_Clip_PolyVerts, clippedCount);
                    zRndr::g_inverseDepthBias = gClipRect_Primary.zMin;
                    zRndr::g_inverseDepthScale = depthScale;
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
                }
                continue;
            } else {
                // Retail stores outFade before packedColor (0x4778b0/0x4778b3).
                int shadeMode = 0;
                float outFade = 0.0f;
                int packedColor = material->packedColor;
                if (outDepthFade != 0 && zModel_Light::EvalBatchSphereFade(&outFade) != 0) {
                    shadeMode = 1;
                }
                // Retail passes a literal triangle count of 3 to the light-weight builder.
                if (lightsEnabled != 0 && zModelLightBuildLightWeights(&surfaceNormal, 3, &packedColor, outFade) != 0) {
                    shadeMode = 2;
                }
                if (shadeMode == 1) {
                    zRndr::CommitFogColorParamsIfChanged();
                    float scale255;
                    zFloat::Set255f(&scale255);
                    scale255 -= 1.0f;
                    zRndr::BlendPackedColor565WithFogInPlace(&packedColor, (int)(scale255 * outFade));
                }

                if ((clipMask & 0x30) != 0 && zClipRect::ClipPolyZRange_NoUV(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }
                zRndr::g_scanConvertMode = scanConvertMode;
                ProjectScratchToClipVerts(clippedCount);
                {
                    const zClipVert* current = g_Clip_PolyVerts;
                    float twiceArea = 0.0f;
                    const zClipVert* previous = &g_Clip_PolyVerts[clippedCount - 1];
                    int areaCount = clippedCount;
                    do {
                        twiceArea += current->y * previous->x - previous->y * current->x;
                        previous = current;
                        ++current;
                    } while (--areaCount);
                    if (fabsf(twiceArea) < gModel_SmallPolyRejectArea2x) {
                        continue;
                    }
                }
                memcpy(triClipVerts, g_Clip_PolyVerts, sizeof(triClipVerts));
                if ((clipMask & 0x0f) != 0 && zClipRect::ClipPoly_NoUV(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }

                const int vertexCount = clippedCount;
                depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                zRndr::g_inverseDepthBias = 0.0f;
                zRndr::g_inverseDepthScale = depthScale;
                zRndrSubmitPolyWithSpanList(
                    (zVec3*)g_Clip_PolyVerts,
                    triClipVerts,
                    packedColor,
                    MaterialAlphaInt(material),
                    vertexCount,
                    gModel_RenderVertexAlphaEnabled
                );

                clippedCount = vertexCount;
                if (gAltClipPassEnabled != 0 && zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, vertexCount) != 0
                    && zClipRect::ClipPoly_NoUV(&gClipRect_Alt, &clippedCount) != 0) {
                    RemapAltProjectedVerts(triClipVerts, 3);
                    RemapAltProjectedVerts(g_Clip_PolyVerts, clippedCount);
                    zRndr::g_inverseDepthBias = gClipRect_Primary.zMin;
                    zRndr::g_inverseDepthScale = depthScale;
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

        zMath::MatStackPopPtr();
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-rendernodehardware
     * @recoil-artifact defines .text recoil:function:0x477b30: zModel::RenderNodeHardware
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
     * @recoil-match byte
     *
     * Raw assembly: the reviewed vector-dot island in the per-entry backface
     * test (GMOD_DRAW_FACING_DOT, retail [0x4781d5,0x4781f4)), and the reviewed
     * transform-point island in the three ZMTH_MAT_TRANSFORM_POINT_BATCH expansions,
     * retail [0x477c0b,0x477c6f) (mode 2), [0x477e4b,0x477eaf) (main path) and
     * [0x477f26,0x477f8a) (normals, point form); Pro run 2026-10-07T09-38-31,
     * rows Z5c/Z5d/Z5e.
     *
     *
     * Purpose: render a display-instance node through the hardware renderer path.
     * Retail clears eax on both exits, so the renderer entry returns 0.
     */
    int __fastcall RenderNodeHardware(CZNodePartial * node, int clipMask)
    {
        zDiPartial* const di = (zDiPartial*)(node->userDataOrDiRef);
        zMat4x3 matrixScratch;
        int outDepthFade;
        int outActiveLightState;
        int outLensFlareVisible;
        zClipUV perspectiveUvs[0x400];

        zMath::MatStackPushPtr((float*)(&matrixScratch));
        switch (di->mode) {
        default:
            zMathMatSetupCamera();
            zRndr::g_perspectiveTextureEnabled = 0;
            break;
        case 2: {
            zMathMatSetupCamera();
            if ((di->flags & 8) != 0 && di->blendScale != 0.0 && di->blendVertCount != 0) {
                zMathVec3ArrayAddScaled(
                    g_zModel_TransformedVerts,
                    di->verts,
                    di->blendVerts,
                    di->blendVertCount,
                    di->blendScale
                );
                zMath::MatTransformPointBatchInPlace(g_zModel_TransformedVerts, di->vertCount);
            } else {
                // Retail calls the batch with no count test: requires vertCount > 0, verts and
                // g_zModel_TransformedVerts each holding vertCount elements, no overlap.
                ZMTH_MAT_TRANSFORM_POINT_BATCH(di->verts, g_zModel_TransformedVerts, di->vertCount);
            }

            // Retail reads the first entry's material colour without a null test.
            const unsigned int pointColor = di->entries->material->packedColor;
            for (int vertexIndex = 0; vertexIndex < di->vertCount; ++vertexIndex) {
                zVec3* const transformed = &g_zModel_TransformedVerts[vertexIndex];
                if (transformed->z > gClipRect_Primary.zMin) {
                    zProjectedPoint projectedPoint;
                    const int rendererPath = g_zVideo_ActiveRendererPath;
                    if (rendererPath != 0) {
                        zMathProjectSphereBatch(transformed, (zProjectedSphere*)(&projectedPoint), 1);
                    } else {
                        zMath::ProjectPointBatch(transformed, &projectedPoint, 1);
                    }

                    if (ProjectedPointInClipBounds(projectedPoint)) {
                        if (rendererPath != 0) {
                            g_zVideo_pfnDrawPointColor16((zVideo_XyzVertex*)(&projectedPoint), pointColor, 1);
                        } else {
                            zRndrLensFlareQueueProjectedSample(&projectedPoint, (int)pointColor, 0);
                        }
                    }
                }
            }

            zMath::MatStackPopPtr();
            zRndr::g_perspectiveTextureEnabled = 0;
            return 0;
        }
        case 1:
            if ((di->flags & 0x10) != 0) {
                zMathMatLoadView();
                zRndr::g_perspectiveTextureEnabled = 0;
            } else {
                zMathMatLoadProjection(g_zVideo_pActiveProjectionViewContext->eulerAngles.y);
                zRndr::g_perspectiveTextureEnabled = 0;
            }
            break;
        case 0:
            zMathMatSetupCamera();
            zRndr::g_perspectiveTextureEnabled = 1;
            break;
        }

        if (di->entryCount > 0) {
            zDi::EvalBoundingSphereLightingFlags(di, &outDepthFade, &outActiveLightState, &outLensFlareVisible);
            if ((di->flags & 8) != 0 && di->blendScale != 0.0 && di->blendVertCount != 0) {
                zMathVec3ArrayAddScaled(
                    g_zModel_TransformedVerts,
                    di->verts,
                    di->blendVerts,
                    di->blendVertCount,
                    di->blendScale
                );
                zMath::MatTransformPointBatchInPlace(g_zModel_TransformedVerts, di->vertCount);
            } else if (di->vertCount != 0) {
                // The != 0 test is retail's; the batch still requires vertCount > 0, verts and
                // g_zModel_TransformedVerts each holding vertCount elements, no overlap.
                ZMTH_MAT_TRANSFORM_POINT_BATCH(di->verts, g_zModel_TransformedVerts, di->vertCount);
            }

            if (g_zModel_VertexShadingEnabled != 0 && di->normalCount != 0) {
                // Binary32 zero stores, as retail's immediates (0x477edc..0x477eea).
                zVec3 origin = { 0.0f, 0.0f, 0.0f };
                // The != 0 test is retail's; the batch still requires normalCount > 0, normals and
                // g_zModel_TransformedNormals each holding normalCount elements, no overlap.
                ZMTH_MAT_TRANSFORM_POINT_BATCH(di->normals, g_zModel_TransformedNormals, di->normalCount);
                zMath::MatTransformPointBatchInPlace(&origin, 1);
                zVec3* normal = g_zModel_TransformedNormals;
                for (int normalIndex = 0; normalIndex < di->normalCount; ++normalIndex, ++normal) {
                    normal->x -= origin.x;
                    normal->y -= origin.y;
                    normal->z -= origin.z;
                    zMath::Vec3Normalize(normal);
                }
            }
        }

        // Retail animates the point-camera entries exactly as the software path does.
        for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex) {
            zModel_PointEntryPartial* const pointEntry = &di->pointEntries[pointIndex];
            zVec3* const pointCams = pointEntry->pointCamList;
            switch (pointEntry->mode) {
            case 1:
                pointEntry->elapsedTime += g_FrameDeltaTimeSec;
                if (pointEntry->elapsedTime > pointEntry->timerSec) {
                    ++pointEntry->pointCamPackedState;
                    if ((unsigned short)pointEntry->pointCamPackedState >= pointEntry->pointCamCount) {
                        pointEntry->pointCamPackedState &= 0xffff0000;
                    }
                    pointEntry->elapsedTime = 0.0f;
                }
                zModelRenderPointQueueEntry(
                    &pointCams[(unsigned short)pointEntry->pointCamPackedState],
                    pointEntry->packedColor16,
                    pointEntry
                );
                if ((unsigned short)pointEntry->pointCamPackedState > 0) {
                    zModelRenderPointQueueEntry(
                        &pointEntry->pointCamList[(unsigned short)pointEntry->pointCamPackedState - 1],
                        (unsigned short)((unsigned int)pointEntry->pointCamPackedState >> 16),
                        pointEntry
                    );
                }
                break;
            case 0:
                if (pointEntry->behavior == 1) {
                    pointEntry->elapsedTime += g_FrameDeltaTimeSec;
                    if (pointEntry->elapsedTime > pointEntry->timerSec) {
                        const unsigned short packedColor = pointEntry->packedColor16;
                        const unsigned int packedState = (unsigned int)pointEntry->pointCamPackedState;
                        pointEntry->packedColor16 = (unsigned short)(packedState >> 16);
                        pointEntry->pointCamPackedState
                            = (int)((packedState & 0xffff) | ((unsigned int)packedColor << 16));
                        pointEntry->elapsedTime = 0.0f;
                    }
                }
                zModelRenderPointQueueEntry(pointCams, pointEntry->packedColor16, pointEntry);
                break;
            }
        }

        gClipRect_Primary.flags = clipMask;
        zDiEntryPartial* entry = di->entries;
        zDiEntryPartial* const entriesEnd = &entry[di->entryCount];
        for (; entry < entriesEnd; ++entry) {
            {
                // Retail gathers whole zVec3 records without index or count validation (count-down copy).
                int copyCount = (int)(entry->flagsAndIndexCount & 0xff);
                const int* copyIndices = (const int*)(entry->vertexIndices);
                zVec3* copyDest = g_Clip_PolyVertsScratch;
                const zVec3* const copySource = g_zModel_TransformedVerts;
                do {
                    *copyDest++ = copySource[*copyIndices++];
                } while (--copyCount);
            }

            zVec3 surfaceNormal;
            {
                const zVec3& v0 = g_Clip_PolyVertsScratch[0];
                const zVec3& v1 = g_Clip_PolyVertsScratch[1];
                const zVec3& v2 = g_Clip_PolyVertsScratch[2];
                const zVec3 edgeA = { v2.x - v1.x, v2.y - v1.y, v2.z - v1.z };
                const zVec3 edgeB = { v0.x - v1.x, v0.y - v1.y, v0.z - v1.z };
                surfaceNormal.x = edgeB.z * edgeA.y - edgeB.y * edgeA.z;
                surfaceNormal.y = edgeB.x * edgeA.z - edgeB.z * edgeA.x;
                surfaceNormal.z = edgeB.y * edgeA.x - edgeB.x * edgeA.y;
            }
            const int showBackFace = (int)((entry->flagsAndIndexCount >> 8) & 1);
            float facing;
            GMOD_DRAW_FACING_DOT(facing, &surfaceNormal, g_Clip_PolyVertsScratch[0]);
            if (!(facing < -g_zModel_BFETolerance)) {
                if (showBackFace == 0 || !(facing > g_zModel_BFETolerance)) {
                    continue;
                }
                surfaceNormal.x = -surfaceNormal.x;
                surfaceNormal.y = -surfaceNormal.y;
                surfaceNormal.z = -surfaceNormal.z;
            }

            if (g_zModel_VertexShadingEnabled != 0 && di->normalCount != 0
                && (entry->flagsAndIndexCount & 0x0200) != 0) {
                // Retail gathers the normals without index validation.
                g_zModel_CurrentPolyNormals = g_zModel_CurrentPolyNormalsStorage;
                int copyCount = (int)(entry->flagsAndIndexCount & 0xff);
                const int* copyIndices = (const int*)(entry->normalIndices);
                const zVec3* const copySource = g_zModel_TransformedNormals;
                zVec3* copyDest = g_zModel_CurrentPolyNormalsStorage;
                do {
                    *copyDest++ = copySource[*copyIndices++];
                } while (--copyCount);
            } else {
                g_zModel_CurrentPolyNormals = 0;
            }

            int clippedCount = (int)(entry->flagsAndIndexCount & 0xff);
            zModel_MaterialPartial* const material = entry->material;
            for (int attrIndex = 0; attrIndex < clippedCount; ++attrIndex) {
                g_Clip_PolyAttr1[attrIndex] = 0.0f;
                g_Clip_PolyAttr0[attrIndex] = 0.0f;
            }

            int lightingVaries = 0;
            int lightingFlags = 0;
            // One lightVaries for both lighting paths (retail [ebp-0x18] in each).
            int lightVaries;
            if ((material->flags & 0x0100) != 0) {
                if ((material->flags & 0x0400) != 0) {
                    zModel_Material::UpdateCycleIfNeeded(material);
                }
                if (outDepthFade != 0 && zModel_Light::BuildAttr1Falloff(clippedCount, &lightingVaries) != 0) {
                    lightingFlags = 1;
                }
                lightVaries = lightingVaries;
                if (outActiveLightState != 0
                    && zModel_Light::SetActiveLights(&surfaceNormal, clippedCount, &lightingFlags, &lightVaries, 0)
                        != 0) {
                    lightingFlags |= 2;
                    lightingVaries |= lightVaries;
                }
                if ((lightingFlags & ~0x0c) == 0) {
                    for (int attrIndex = 0; attrIndex < clippedCount; ++attrIndex) {
                        g_Clip_PolyAttr1[attrIndex] = 1.0f;
                        g_Clip_PolyAttr0[attrIndex] = 1.0f;
                    }
                }

                memcpy(g_Clip_PolyUvs, entry->uvPairs, (size_t)clippedCount * sizeof(zClipUV));

                if (lightingFlags != 0) {
                    if ((clipMask & 0x30) != 0) {
                        const int previousCount = clippedCount;
                        if ((lightingVaries != 0
                                    ? zClipRect::ClipPolyZRange_WithAttr012(&gClipRect_Primary, &clippedCount)
                                    : zClipRect::ClipPolyNearZ(&gClipRect_Primary, &clippedCount))
                            == 0) {
                            continue;
                        }
                        if (lightingVaries == 0 && previousCount < clippedCount) {
                            for (int attrIndex = previousCount; attrIndex < clippedCount; ++attrIndex) {
                                g_Clip_PolyAttr0[attrIndex] = g_Clip_PolyAttr0[0];
                                g_Clip_PolyAttr1[attrIndex] = g_Clip_PolyAttr1[0];
                                g_Clip_PolyAttr2[attrIndex] = g_Clip_PolyAttr2[0];
                            }
                        }
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
                    if ((clipMask & 0x0f) != 0) {
                        const int previousCount = clippedCount;
                        if ((lightingVaries != 0 ? zClipRect::ClipPoly_WithAttr012(&gClipRect_Primary, &clippedCount)
                                                 : zClipRect::ClipPoly(&gClipRect_Primary, &clippedCount))
                            == 0) {
                            continue;
                        }
                        if (lightingVaries == 0 && previousCount < clippedCount) {
                            for (int attrIndex = previousCount; attrIndex < clippedCount; ++attrIndex) {
                                g_Clip_PolyAttr0[attrIndex] = g_Clip_PolyAttr0[0];
                                g_Clip_PolyAttr1[attrIndex] = g_Clip_PolyAttr1[0];
                                g_Clip_PolyAttr2[attrIndex] = g_Clip_PolyAttr2[0];
                            }
                        }
                    }

                    for (int perspectiveIndex = 0; perspectiveIndex < clippedCount; ++perspectiveIndex) {
                        const float depth = 1.0f / g_Clip_PolyVerts[perspectiveIndex].z;
                        perspectiveUvs[perspectiveIndex].u = depth * g_Clip_PolyUvs[perspectiveIndex].u;
                        perspectiveUvs[perspectiveIndex].v = g_Clip_PolyUvs[perspectiveIndex].v * depth;
                    }
                    if (entry->drawFlags != 0) {
                        const float depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                        for (int depthIndex = 0; depthIndex < clippedCount; ++depthIndex) {
                            g_Clip_PolyVerts[depthIndex].z = depthScale * g_Clip_PolyVerts[depthIndex].z;
                        }
                    }
                    if (g_zModel_CurrentPolyNormals != 0) {
                        g_zVideo_pfnSubmitPolygonLit(
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
                        g_zVideo_pfnSubmitPolygon(
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

                    if (gAltClipPassEnabled != 0 && zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, clippedCount) != 0) {
                        const int previousCount = clippedCount;
                        if ((lightingVaries != 0 ? zClipRect::ClipPoly_WithAttr012(&gClipRect_Alt, &clippedCount)
                                                 : zClipRect::ClipPoly(&gClipRect_Alt, &clippedCount))
                            == 0) {
                            continue;
                        }
                        if (lightingVaries == 0 && previousCount < clippedCount) {
                            for (int attrIndex = previousCount; attrIndex < clippedCount; ++attrIndex) {
                                g_Clip_PolyAttr0[attrIndex] = g_Clip_PolyAttr0[0];
                                g_Clip_PolyAttr1[attrIndex] = g_Clip_PolyAttr1[0];
                                g_Clip_PolyAttr2[attrIndex] = g_Clip_PolyAttr2[0];
                            }
                        }
                        for (int remapIndex = 0; remapIndex < clippedCount; ++remapIndex) {
                            const float depth = 1.0f / g_Clip_PolyVerts[remapIndex].z;
                            g_Clip_PolyVerts[remapIndex].x
                                = g_zClipAlt_RemapScaleX * g_Clip_PolyVerts[remapIndex].x + g_zClipAlt_RemapBiasX;
                            g_Clip_PolyVerts[remapIndex].y
                                = g_zClipAlt_RemapScaleY * g_Clip_PolyVerts[remapIndex].y + g_zClipAlt_RemapBiasY;
                            g_Clip_PolyVerts[remapIndex].z += 1.0f / gClipRect_Primary.zMin;
                            perspectiveUvs[remapIndex].u = depth * g_Clip_PolyUvs[remapIndex].u;
                            perspectiveUvs[remapIndex].v = g_Clip_PolyUvs[remapIndex].v * depth;
                        }
                        if ((lightingFlags & 2) != 0) {
                            g_zVideo_pfnSubmitPolygon(
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
                            g_zVideo_pfnSubmitPolyRenderClass(
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

                if ((clipMask & 0x30) != 0 && zClipRect::ClipPolyNearZ(&gClipRect_Primary, &clippedCount) == 0) {
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
                if ((clipMask & 0x0f) != 0 && zClipRect::ClipPoly(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }

                // Retail divides by the projected reciprocal depth without a zero test.
                for (int perspectiveIndex = 0; perspectiveIndex < clippedCount; ++perspectiveIndex) {
                    const float depth = 1.0f / g_Clip_PolyVerts[perspectiveIndex].z;
                    perspectiveUvs[perspectiveIndex].u = depth * g_Clip_PolyUvs[perspectiveIndex].u;
                    perspectiveUvs[perspectiveIndex].v = g_Clip_PolyUvs[perspectiveIndex].v * depth;
                }
                if (entry->drawFlags != 0) {
                    const float depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                    for (int depthIndex = 0; depthIndex < clippedCount; ++depthIndex) {
                        g_Clip_PolyVerts[depthIndex].z = depthScale * g_Clip_PolyVerts[depthIndex].z;
                    }
                }
                g_zVideo_pfnSubmitPolyRenderClass(
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

                if (gAltClipPassEnabled != 0 && zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, clippedCount) != 0
                    && zClipRect::ClipPoly(&gClipRect_Alt, &clippedCount) != 0) {
                    for (int remapIndex = 0; remapIndex < clippedCount; ++remapIndex) {
                        const float depth = 1.0f / g_Clip_PolyVerts[remapIndex].z;
                        g_Clip_PolyVerts[remapIndex].x
                            = g_zClipAlt_RemapScaleX * g_Clip_PolyVerts[remapIndex].x + g_zClipAlt_RemapBiasX;
                        g_Clip_PolyVerts[remapIndex].y
                            = g_zClipAlt_RemapScaleY * g_Clip_PolyVerts[remapIndex].y + g_zClipAlt_RemapBiasY;
                        g_Clip_PolyVerts[remapIndex].z += 1.0f / gClipRect_Primary.zMin;
                        perspectiveUvs[remapIndex].u = depth * g_Clip_PolyUvs[remapIndex].u;
                        perspectiveUvs[remapIndex].v = g_Clip_PolyUvs[remapIndex].v * depth;
                    }
                    g_zVideo_pfnSubmitPolyRenderClass(
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
                continue;
            }

            if (outDepthFade != 0 && zModel_Light::BuildAttr1Falloff(clippedCount, &lightingVaries) != 0) {
                lightingFlags = 1;
            }
            lightVaries = lightingVaries;
            if (outActiveLightState != 0
                && zModel_Light::SetActiveLights(&surfaceNormal, clippedCount, &lightingFlags, &lightVaries, 0) != 0) {
                lightingFlags |= 2;
                lightingVaries |= lightVaries;
            }

            int vertexCount;
            if (lightingFlags != 0) {
                if ((clipMask & 0x30) != 0
                    && zClipRect::ClipPolyZRange_NoUV_WithAttribs(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }
                zMathProjectSphereBatch(
                    (const zVec3*)g_Clip_PolyVertsScratch,
                    (zProjectedSphere*)g_Clip_PolyVerts,
                    clippedCount
                );
                if ((clipMask & 0x0f) != 0
                    && zClipRect::ClipPoly_NoUV_WithAttr012_Alt(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }
                vertexCount = clippedCount;
                if (entry->drawFlags != 0) {
                    const float depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                    for (int depthIndex = 0; depthIndex < vertexCount; ++depthIndex) {
                        g_Clip_PolyVerts[depthIndex].z = depthScale * g_Clip_PolyVerts[depthIndex].z;
                    }
                }
                g_zVideo_pfnSubmitPolyColorAttr(
                    (zVideo_XyzVertex*)g_Clip_PolyVerts,
                    material->packedColor,
                    (zVideo_ColorRgbFloat*)&material->colorRgb,
                    g_Clip_PolyAttr1,
                    (lightingFlags & 4) != 0 ? g_Clip_PolyAttr0 : 0,
                    (lightingFlags & 1) != 0 ? g_Clip_PolyAttr2 : 0,
                    MaterialAlphaInt(material),
                    vertexCount,
                    entry->drawFlags,
                    gModel_RenderVertexAlphaEnabled
                );
            } else {
                if ((clipMask & 0x30) != 0 && zClipRect::ClipPolyZRange_NoUV(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }
                zMathProjectSphereBatch(
                    (const zVec3*)g_Clip_PolyVertsScratch,
                    (zProjectedSphere*)g_Clip_PolyVerts,
                    clippedCount
                );
                if ((clipMask & 0x0f) != 0 && zClipRect::ClipPoly_NoUV_Alt(&gClipRect_Primary, &clippedCount) == 0) {
                    continue;
                }
                vertexCount = clippedCount;
                if (entry->drawFlags != 0) {
                    const float depthScale = (float)(int)entry->drawFlags * g_zRndr_InverseZTolerance + 1.0f;
                    for (int depthIndex = 0; depthIndex < vertexCount; ++depthIndex) {
                        g_Clip_PolyVerts[depthIndex].z = depthScale * g_Clip_PolyVerts[depthIndex].z;
                    }
                }
                g_zVideo_pfnSubmitPolyFlatColor16(
                    (zVideo_XyzVertex*)g_Clip_PolyVerts,
                    material->packedColor,
                    MaterialAlphaInt(material),
                    vertexCount,
                    entry->drawFlags,
                    gModel_RenderVertexAlphaEnabled
                );
            }

            // Retail rewrites the clip count from the submitted count before the alternate pass.
            clippedCount = vertexCount;
            if (gAltClipPassEnabled != 0 && zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, vertexCount) != 0
                && zClipRect::ClipPoly_NoUV_Alt(&gClipRect_Alt, &clippedCount) != 0) {
                for (int remapIndex = 0; remapIndex < clippedCount; ++remapIndex) {
                    g_Clip_PolyVerts[remapIndex].x
                        = g_zClipAlt_RemapScaleX * g_Clip_PolyVerts[remapIndex].x + g_zClipAlt_RemapBiasX;
                    g_Clip_PolyVerts[remapIndex].y
                        = g_zClipAlt_RemapScaleY * g_Clip_PolyVerts[remapIndex].y + g_zClipAlt_RemapBiasY;
                    g_Clip_PolyVerts[remapIndex].z += 1.0f / gClipRect_Primary.zMin;
                }
                g_zVideo_pfnSubmitPolyFlatColor16(
                    (zVideo_XyzVertex*)g_Clip_PolyVerts,
                    material->packedColor,
                    MaterialAlphaInt(material),
                    clippedCount,
                    entry->drawFlags,
                    gModel_RenderVertexAlphaEnabled
                );
            }
        }

        zMath::MatStackPopPtr();
        return 0;
    }
} // namespace zModel

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zvideo-frustumtestsphereclipmask
 * @recoil-artifact defines .text recoil:function:0x478c70: zVideoFrustumTestSphereClipMask.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-match byte
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
    zVec3 delta;
    const int oldMask = *clipMaskInOut;
    *clipMaskInOut = 0;

    if ((oldMask & 0x10) > 0) {
        zMath::Vec3Subtract(sphereCenter, &g_zVideo_pActiveProjectionViewContext->nearClipCenter, &delta);
        float dot;
        ZMTH_VECTOR_DOT(dot, &delta, &g_zVideo_pActiveProjectionViewContext->worldFrustumNormals[4]);
        if (dot >= radius) {
            *clipMaskInOut = 0;
        } else {
            if (-radius >= dot) {
                return 0x10;
            }
            *clipMaskInOut = 0x10;
        }
    }

    zMath::Vec3Subtract(sphereCenter, &g_zVideo_pActiveProjectionViewContext->cameraPos, &delta);
    if ((oldMask & 1) > 0) {
        float dot;
        ZMTH_VECTOR_DOT(dot, &delta, &g_zVideo_pActiveProjectionViewContext->worldFrustumNormals[0]);
        if (-radius >= dot) {
            return 1;
        }
        if (dot < radius) {
            *clipMaskInOut |= 1;
        }
    }

    if ((oldMask & 2) > 0) {
        float dot;
        ZMTH_VECTOR_DOT(dot, &delta, &g_zVideo_pActiveProjectionViewContext->worldFrustumNormals[1]);
        if (-radius >= dot) {
            return 2;
        }
        if (dot < radius) {
            *clipMaskInOut |= 2;
        }
    }

    if ((oldMask & 4) > 0) {
        float dot;
        ZMTH_VECTOR_DOT(dot, &delta, &g_zVideo_pActiveProjectionViewContext->worldFrustumNormals[2]);
        if (-radius >= dot) {
            return 4;
        }
        if (dot < radius) {
            *clipMaskInOut |= 4;
        }
    }

    if ((oldMask & 8) > 0) {
        float dot;
        ZMTH_VECTOR_DOT(dot, &delta, &g_zVideo_pActiveProjectionViewContext->worldFrustumNormals[3]);
        if (-radius >= dot) {
            return 8;
        }
        if (dot < radius) {
            *clipMaskInOut |= 8;
        }
    }

    if ((oldMask & 0x20) > 0) {
        zMath::Vec3Subtract(sphereCenter, &g_zVideo_pActiveProjectionViewContext->farClipCenter, &delta);
        float dot;
        ZMTH_VECTOR_DOT(dot, &delta, &g_zVideo_pActiveProjectionViewContext->worldFrustumNormals[5]);
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
 * @recoil-match byte
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
        // Retail seeds the bounds from the scrolled first UV before storing it (as in the one-axis paths).
        float minU = deltaU + uvs[0].u;
        float minV = deltaV + uvs[0].v;
        float maxU = minU;
        float maxV = minV;
        uvs[0].u = minU;
        uvs[0].v = minV;
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
        OptCatalogSurfaceTextureHandle* handle;
        OptCatalogDamageMaskSurface* srcSurface;
        OptCatalogDamageMaskSurface* dstSurface;
        unsigned short* dstPixels;
        unsigned int dstPitch;
        // Retail keeps the source window as a RECT beside stampRect; srcRect.top stays in a register.
        RECT srcRect;
        RECT stampRect;
        int srcX;
        int outX;
        int srcY;
        int outY;

        if (OptCatalogIsDamageMaskEnabled() == 0) {
            return;
        }

        if (hitEvent->surfaceRef == 0) {
            return;
        }

        if ((hitEvent->surfaceRef->flags & 0x0100) == 0 || (hitEvent->surfaceRef->flags & 0x0200) == 0
            || (hitEvent->surfaceRef->flags & 0x0400) != 0) {
            return;
        }

        while (g_OptCatalogDamageMaskPhaseU > 1.01f) {
            g_OptCatalogDamageMaskPhaseU -= 1.0f;
        }
        while (g_OptCatalogDamageMaskPhaseV > 1.01f) {
            g_OptCatalogDamageMaskPhaseV -= 1.0f;
        }
        while (g_OptCatalogDamageMaskPhaseU < -0.01f) {
            g_OptCatalogDamageMaskPhaseU += 1.0f;
        }
        while (g_OptCatalogDamageMaskPhaseV < -0.01f) {
            g_OptCatalogDamageMaskPhaseV += 1.0f;
        }

        handle = (OptCatalogSurfaceTextureHandle*)g_OptCatalogDamageMaskHandles[g_OptCatalogDamageMaskSlotIndex];
        srcSurface = handle != 0 ? handle->surface : 0;
        handle = hitEvent->surfaceRef->textureHandle;
        dstSurface = handle != 0 ? handle->surface : 0;
        if (srcSurface == 0 || dstSurface == 0 || srcSurface->format != 0 || dstSurface->format != 0) {
            return;
        }

        int dstX = (int)(dstSurface->width * g_OptCatalogDamageMaskPhaseU) - (srcSurface->width >> 1);
        int dstY = (int)(dstSurface->height * g_OptCatalogDamageMaskPhaseV) - (srcSurface->height >> 1);

        // Retail centres an oversized stamp and shifts, rather than clips, one that overhangs an edge.
        if (srcSurface->width > dstSurface->width) {
            srcRect.left = (srcSurface->width - dstSurface->width) >> 1;
            srcRect.right = srcSurface->width - srcRect.left;
            stampRect.left = 0;
            stampRect.right = dstSurface->width;
        } else {
            srcRect.left = 0;
            srcRect.right = srcSurface->width;
            stampRect.left = dstX;
            stampRect.right = dstX + srcRect.right;
            if (dstX < 0) {
                stampRect.right -= dstX;
                stampRect.left = 0;
            } else if (stampRect.right > dstSurface->width) {
                stampRect.left = stampRect.left - stampRect.right + dstSurface->width;
                stampRect.right = dstSurface->width;
            }
        }

        if (srcSurface->height > dstSurface->height) {
            srcRect.top = (srcSurface->height - dstSurface->height) >> 1;
            srcRect.bottom = srcSurface->height - srcRect.top;
            stampRect.top = 0;
            stampRect.bottom = dstSurface->height;
        } else {
            srcRect.top = 0;
            srcRect.bottom = srcSurface->height;
            stampRect.top = dstY;
            stampRect.bottom = dstY + srcRect.bottom;
            if (dstY < 0) {
                stampRect.bottom -= dstY;
                stampRect.top = 0;
            } else if (stampRect.bottom > dstSurface->height) {
                stampRect.top = stampRect.top - stampRect.bottom + dstSurface->height;
                stampRect.bottom = dstSurface->height;
            }
        }

        if (hitEvent->surfaceRef->textureHandle->textureRecord == 0) {
            dstPixels = dstSurface->pixels;
            dstPitch = dstSurface->width;
        } else {
            if (g_zVideo_pfnTextureRecordLockUploadSurface(
                    hitEvent->surfaceRef->textureHandle->textureRecord,
                    (void**)&dstPixels,
                    (int*)&dstPitch
                )
                == 0) {
                return;
            }
            dstPitch >>= 1;
        }

        // Rows are indexed from per-row bases: retail sets up both pixel walkers after each row's bounds test.
        if (srcSurface->alpha != 0) {
            if (zRndr::g_pixelPackGreenBits == 6) {
                for (srcY = srcRect.top, outY = stampRect.top; srcY < srcRect.bottom; ++srcY, ++outY) {
                    unsigned short* dstRow = dstPixels + outY * dstPitch;
                    const unsigned short* srcRow = srcSurface->pixels + srcY * srcSurface->width;
                    const unsigned char* alphaRow = srcSurface->alpha + srcY * srcSurface->width;
                    for (srcX = srcRect.left, outX = stampRect.left; srcX < srcRect.right; ++srcX, ++outX) {
                        const int alpha = alphaRow[srcX];
                        if (alpha != 0) {
                            const unsigned short srcPixel = srcRow[srcX];
                            if (alpha <= 3) {
                                continue;
                            }
                            if (alpha >= 0xfc) {
                                dstRow[outX] = srcPixel;
                            } else {
                                const int dstPixel = dstRow[outX];
                                const int blended = dstPixel
                                    + ((((srcPixel & 0xf800) - (dstPixel & 0xf800)) * alpha >> 8) & 0xfffff800);
                                const int green
                                    = (((srcPixel & 0x07e0) - (dstPixel & 0x07e0)) * alpha >> 8) & 0xffffffe0;
                                const int blue = ((srcPixel & 0x001f) - (blended & 0x001f)) * alpha >> 8;
                                dstRow[outX] = (unsigned short)(blended + (blue + green));
                            }
                        }
                    }
                }
            } else {
                for (srcY = srcRect.top, outY = stampRect.top; srcY < srcRect.bottom; ++srcY, ++outY) {
                    unsigned short* dstRow = dstPixels + outY * dstPitch;
                    const unsigned short* srcRow = srcSurface->pixels + srcY * srcSurface->width;
                    const unsigned char* alphaRow = srcSurface->alpha + srcY * srcSurface->width;
                    for (srcX = srcRect.left, outX = stampRect.left; srcX < srcRect.right; ++srcX, ++outX) {
                        const int alpha = alphaRow[srcX];
                        if (alpha != 0) {
                            const unsigned short srcPixel = srcRow[srcX];
                            if (alpha <= 7) {
                                continue;
                            }
                            if (alpha >= 0xfc) {
                                dstRow[outX] = srcPixel;
                            } else {
                                const unsigned short dstPixel = dstRow[outX];
                                const int red = (((srcPixel & 0x7c00) - (dstPixel & 0x7c00)) * alpha >> 8) & 0xfffffc00;
                                const int green
                                    = (((srcPixel & 0x03e0) - (dstPixel & 0x03e0)) * alpha >> 8) & 0xffffffe0;
                                const int blue = ((srcPixel & 0x001f) - (dstPixel & 0x001f)) * alpha >> 8;
                                dstRow[outX] = (unsigned short)(blue + green + red + dstPixel);
                            }
                        }
                    }
                }
            }
        } else {
            for (srcY = srcRect.top, outY = stampRect.top; srcY < srcRect.bottom; ++srcY, ++outY) {
                unsigned short* dstRow = dstPixels + outY * dstSurface->width;
                const unsigned short* srcRow = srcSurface->pixels + srcY * srcSurface->width;
                for (srcX = srcRect.left, outX = stampRect.left; srcX < srcRect.right; ++srcX, ++outX) {
                    if (srcRow[srcX] != 0) {
                        dstRow[outX] = srcRow[srcX];
                    }
                }
            }
        }

        if (hitEvent->surfaceRef->textureHandle->textureRecord != 0) {
            g_zVideo_pfnTextureRecordUnlockUploadSurface(hitEvent->surfaceRef->textureHandle->textureRecord);
            g_zVideo_pfnTextureRecordFinalizeUpload(hitEvent->surfaceRef->textureHandle->textureRecord, &stampRect, 0);
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
