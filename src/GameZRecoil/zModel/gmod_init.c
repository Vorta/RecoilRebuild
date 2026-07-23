#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGeometry/zgeo.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zReader/zreader.h"
#include "recoil/recoil_types.h"
#include "zclass.h"
#include <ctype.h>

/**
 * Reimplements data 0x57d40c: g_zModel_VertexShadingEnabled.
 * Purpose: gate vertex-shading behavior for zModel render paths.
 */
int g_zModel_VertexShadingEnabled = 0;
/*
 * BN identifies the gmod_init.c diagnostics as three initialized .data char
 * arrays in this order, including the VC alignment padding between rows.
 */
/**
 * Reimplements data 0x4e0f28: g_zModel_SourceFile_GmodInitC.
 * Purpose: store the writable source-file path passed to gmod_init diagnostics.
 */
char g_zModel_SourceFile_GmodInitC[0x27] = "D:\\Proj\\GameZRecoil\\zModel\\gmod_init.c";
/**
 * Reimplements data 0x4e0f50: g_zModel_SetModel3dArraySizeAlreadySetFmt.
 * Purpose: store the writable display-instance pool capacity diagnostic format.
 */
char g_zModel_SetModel3dArraySizeAlreadySetFmt[0x3a] =
    "Error setting model3d array size; size already set to %d.";
/**
 * Reimplements data 0x4e0f8c: g_zModel_TextureScrollNullPtrErrorMsg.
 * Purpose: store the writable null display-instance texture-world diagnostic.
 */
char g_zModel_TextureScrollNullPtrErrorMsg[0x33] =
    "ERROR setting model texture scroll data; Null ptr.";
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SourceFile_GmodInitC) == 0x27);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SetModel3dArraySizeAlreadySetFmt) == 0x3a);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_TextureScrollNullPtrErrorMsg) == 0x33);

namespace {
const double kVisibleContributionThreshold = 1.0 / 255.0;

/**
 * Original static helper observed in zModel render point/lighting paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: transform one model-space point by the current zMath matrix.
 */
zVec3 TransformPointByCurrentMatrix(
    const zVec3 *point
) {
    const zMat4x3 *matrix = (const zMat4x3 *)(*zMath::g_currentMatrixPtrSlot);
    zVec3 out = {0};
    out.x = point->x * matrix->xx + point->y * matrix->yx + point->z * matrix->zx + matrix->posX;
    out.y = point->x * matrix->xy + point->y * matrix->yy + point->z * matrix->zy + matrix->posY;
    out.z = point->x * matrix->xz + point->y * matrix->yz + point->z * matrix->zz + matrix->posZ;
    return out;
}

/**
 * Original static helper observed in zModel render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: test whether graphics option flag bit 0 is enabled.
 */
bool ModelGraphicsFlagBit0Enabled() {
    return gModel_pGraphicsFlags != 0 && ((*gModel_pGraphicsFlags & 1) != 0);
}

/**
 * Original static helper observed in zModel scrolling-texture update paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: compute the renderer-dependent UV wrap extent for one axis.
 */
int TextureWrapExtent(
    const zModel_TextureScrollInfoPartial *textureInfo,
    bool uAxis
) {
    const int baseExtent = g_zVideo_ActiveRendererPath != 0 ? 0x80 : 0x800;
    const unsigned char shift = uAxis ? textureInfo->wrapShiftU : textureInfo->wrapShiftV;
    return (int)((unsigned int)(baseExtent) >> shift);
}

/**
 * Original static helper observed in zModel scrolling-texture update paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: compute a whole-tile UV correction for coordinates outside the wrap extent.
 */
int ComputeWrapCorrection(
    float minValue,
    float maxValue,
    int wrapExtent
) {
    const int minFloor = (int)(floor(minValue));
    const int maxCeil = (int)(ceil(maxValue));
    if (minFloor <= -wrapExtent) {
        return wrapExtent - maxCeil;
    }
    if (maxCeil >= wrapExtent) {
        return -(minFloor + wrapExtent);
    }
    return 0;
}

/**
 * Original static helper observed in zModel scrolling-texture update paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: apply a whole-tile UV correction to a polygon UV array.
 */
void ApplyUvCorrection(
    zModel_Uv *uvs,
    int uvCount,
    int correctionU,
    int correctionV
) {
    if (correctionU == 0 && correctionV == 0) {
        return;
    }

    for (int i = 0; i < uvCount; ++i) {
        uvs[i].u += (float)(correctionU);
        uvs[i].v += (float)(correctionV);
    }
}

/**
 * Original static helper observed in zModel point and software render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: test whether a projected point lies inside the active projection clip bounds.
 */
bool ProjectedPointInClipBounds(
    const zProjectedPoint &point
) {
    return !(point.x < g_zVideo_ProjectClipLeft) && !(point.y < g_zVideo_ProjectClipTop) &&
           !(point.x > g_zVideo_ProjectClipRight) && !(point.y > g_zVideo_ProjectClipBottom);
}

typedef void(__fastcall *DrawPointColor16Proc)(
    zProjectedPoint *point,
    unsigned int packedColor16,
    int pointCount
);
typedef void(__fastcall *SubmitPolyFlatColor16Proc)(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    int alpha,
    int renderParam,
    int vertexCount,
    int queueMode
);
typedef void(__fastcall *SubmitPolyColorAttrProc)(
    zVideo_XyzVertex *vertices,
    unsigned int packedColor16,
    zVideo_ColorRgbFloat *baseColor,
    float *attr1,
    float *attr0,
    float *attr2,
    int alpha,
    int vertexCount,
    unsigned int renderParam,
    int queueMode
);
typedef void(__fastcall *SubmitPolyRenderClassProc)(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *texCoords,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
typedef void(__fastcall *SubmitPolygonProc)(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
typedef void(__fastcall *SubmitPolygonLitProc)(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *uvPairs,
    float *attr1,
    float *attr0,
    float *attr2,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);

/**
 * Recovered original static helper in D:\Proj\GameZRecoil\zModel\zmodel.cpp.
 * No standalone retail function; observed callers are address-backed zModel
 * display-instance paths in this source file.
 * Purpose: return the display-instance pointer stored on a scene node.
 */
zDiPartial *NodeDisplayInstance(
    zClass_NodePartial *node
) {
    return node != 0 ? (zDiPartial *)(node->userDataOrDiRef) : 0;
}

/**
 * Original static helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: prepare transformed display-instance vertices, including optional blend vertices.
 */
void PrepareTransformedVertices(
    zDiPartial *di
) {
    if (di->verts == 0 || di->vertCount <= 0) {
        return;
    }

    if ((di->flags & 8) != 0 && di->blendVerts != 0 && di->blendVertCount > 0 &&
        di->blendScale != 0.0f) {
        zMath_Vec3Array_AddScaled(
            g_zModel_TransformedVerts,
            di->verts,
            di->blendVerts,
            di->blendVertCount,
            di->blendScale
        );
        if (di->vertCount > di->blendVertCount) {
            memcpy(
                &g_zModel_TransformedVerts[di->blendVertCount],
                &di->verts[di->blendVertCount],
                (size_t)(di->vertCount - di->blendVertCount) * sizeof(zVec3)
            );
        }
    } else {
        memcpy(
            g_zModel_TransformedVerts,
            di->verts,
            (size_t)(di->vertCount) * sizeof(zVec3)
        );
    }

    zMath::MatTransformPointBatchInPlace(
        g_zModel_TransformedVerts,
        di->vertCount
    );
}

/**
 * Original static helper observed in zModel hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: transform and normalize display-instance normals for per-vertex shading.
 */
void PrepareTransformedNormals(
    zDiPartial *di
) {
    if (g_zModel_VertexShadingEnabled == 0 || di->normals == 0 || di->normalCount <= 0) {
        return;
    }

    memcpy(
        g_zModel_TransformedNormals,
        di->normals,
        (size_t)(di->normalCount) * sizeof(zVec3)
    );
    zMath::Vec3ArrayTransformDirection(
        g_zModel_TransformedNormals,
        di->normalCount
    );
    for (int i = 0; i < di->normalCount; ++i) {
        zMath::Vec3Normalize(&g_zModel_TransformedNormals[i]);
    }
}

/**
 * Original static helper observed in zModel polygon render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: gather an entry's transformed vertices into the clip scratch polygon.
 */
int CopyEntryVerticesToScratch(
    zDiPartial *di,
    zDiEntryPartial *entry,
    int vertexCount
) {
    int *indices = (int *)(entry->vertexIndices);
    if (indices == 0) {
        return 0;
    }

    for (int i = 0; i < vertexCount; ++i) {
        const int vertexIndex = indices[i];
        if (vertexIndex < 0 || vertexIndex >= di->vertCount) {
            return 0;
        }
        const zVec3 &src = g_zModel_TransformedVerts[vertexIndex];
        g_Clip_PolyVertsScratch[i].x = src.x;
        g_Clip_PolyVertsScratch[i].y = src.y;
        g_Clip_PolyVertsScratch[i].z = src.z;
    }
    return 1;
}

/**
 * Original static helper observed in zModel polygon render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: gather an entry's transformed normals for the current polygon when present.
 */
void CopyEntryNormalsToCurrent(
    zDiPartial *di,
    zDiEntryPartial *entry,
    int vertexCount
) {
    g_zModel_CurrentPolyNormals = 0;
    if (g_zModel_VertexShadingEnabled == 0 || di->normalCount <= 0 ||
        (entry->flagsAndIndexCount & 0x0200) == 0 || entry->normalIndices == 0) {
        return;
    }

    int *indices = (int *)(entry->normalIndices);
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
void ClearPolyAttributes(
    int vertexCount
) {
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
void FillPolyAttributes(
    float value,
    int vertexCount
) {
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
int BuildPolyAttributes(
    const zVec3 *surfaceNormal,
    int vertexCount
) {
    int attrFlags = 0;
    int lightingMode = 0;

    if (gModel_FogEnabled != 0) {
        attrFlags |= zModel_Light::BuildAttr1Falloff(
            vertexCount,
            &lightingMode
        ) != 0 ? 1 : 0;
    }

    if (gModel_HasActiveLights != 0) {
        int lightFlags = 0;
        attrFlags |= zModel_Light::SetActiveLights(
                         (zVec3 *)(surfaceNormal),
                         vertexCount,
                         &lightFlags,
                         &lightingMode,
                         0
                     ) != 0
                         ? 1
                         : 0;
    }

    if (attrFlags == 0) {
        FillPolyAttributes(
            1.0f,
            vertexCount
        );
    }
    return attrFlags;
}

/**
 * Original static helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: compute the polygon facing normal and apply backface/show-backface culling.
 */
int ComputeSurfaceNormalAndCull(
    int vertexCount,
    int showBackFace,
    zVec3 *outNormal,
    int *outScanConvertMode
) {
    if (vertexCount < 3) {
        return 0;
    }
    if (outScanConvertMode != 0) {
        *outScanConvertMode = 1;
    }

    const zClipVert &v0 = g_Clip_PolyVertsScratch[0];
    const zClipVert &v1 = g_Clip_PolyVertsScratch[1];
    const zClipVert &v2 = g_Clip_PolyVertsScratch[2];

    const float v2xMinusV1x = v2.x - v1.x;
    const float v2yMinusV1y = v2.y - v1.y;
    const float v2zMinusV1z = v2.z - v1.z;
    const float v0xMinusV1x = v0.x - v1.x;
    const float v0yMinusV1y = v0.y - v1.y;
    const float v0zMinusV1z = v0.z - v1.z;

    // Original 0x476cf0/0x477b30 display face test uses this exact
    // compiler-era arithmetic, including the non-cross-product Z term.
    outNormal->x = v0zMinusV1z * v2yMinusV1y - v0yMinusV1y * v2zMinusV1z;
    outNormal->y = v0xMinusV1x * v2zMinusV1z - v0zMinusV1z * v2xMinusV1x;
    outNormal->z = v0yMinusV1y * v2xMinusV1x - v0xMinusV1x * v2zMinusV1z;

    const float facing = outNormal->x * v0.x + outNormal->y * v0.y + outNormal->z * v0.z;
    if (facing < -g_zModel_BFETolerance) {
        return 1;
    }
    if (showBackFace != 0 && facing > g_zModel_BFETolerance) {
        outNormal->x = -outNormal->x;
        outNormal->y = -outNormal->y;
        outNormal->z = -outNormal->z;
        if (outScanConvertMode != 0) {
            *outScanConvertMode = 0;
        }
        return 1;
    }
    return 0;
}

/**
 * Original static helper observed in zModel textured polygon render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: copy an entry's UV pairs into the current clip UV scratch array.
 */
void CopyEntryUvsToScratch(
    zDiEntryPartial *entry,
    int vertexCount
) {
    if (g_Clip_PolyUvs == 0 || entry->uvPairs == 0) {
        return;
    }
    memcpy(
        g_Clip_PolyUvs,
        entry->uvPairs,
        (size_t)(vertexCount) * sizeof(zClipUV)
    );
}

/**
 * Original static helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: project the current scratch polygon into the clip vertex buffer.
 */
void ProjectScratchToClipVerts(
    int vertexCount
) {
    if (g_zVideo_ActiveRendererPath == 0) {
        zMath::ProjectPointBatch(
            (const zVec3 *)g_Clip_PolyVertsScratch,
            (zProjectedPoint *)g_Clip_PolyVerts,
            vertexCount
        );
    } else {
        zMath_ProjectSphereBatch(
            (const zVec3 *)g_Clip_PolyVertsScratch,
            (zProjectedSphere *)g_Clip_PolyVerts,
            vertexCount
        );
    }
}

/**
 * Original static helper observed in zModel polygon render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: apply the encoded depth bias to projected clip vertices.
 */
void ApplyDepthBiasToProjectedVerts(
    unsigned int drawFlags,
    int vertexCount
) {
    const float depthScale =
        (float)((short)(drawFlags & 0xffff)) * g_zRndr_InverseZTolerance + 1.0f;
    for (int i = 0; i < vertexCount; ++i) {
        g_Clip_PolyVerts[i].z *= depthScale;
    }
}

/**
 * Original static helper observed in zModel untextured polygon render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: clip and project a polygon without UV coordinates.
 */
int ClipAndProjectNoUv(
    zClipRectPartial *clipRect,
    int *vertexCount,
    int hasAttributes
) {
    if (hasAttributes != 0) {
        if (zClipRect::ClipPolyZRange_NoUV_WithAttribs(
            clipRect,
            vertexCount
        ) == 0) {
            return 0;
        }
    } else if (zClipRect::ClipPolyZRange_NoUV(
        clipRect,
        vertexCount
    ) == 0) {
        return 0;
    }

    ProjectScratchToClipVerts(*vertexCount);

    if (hasAttributes != 0) {
        return zClipRect::ClipPoly_NoUV_WithAttr012_Alt(
            clipRect,
            vertexCount
        );
    }
    return zClipRect::ClipPoly_NoUV(
        clipRect,
        vertexCount
    );
}

/**
 * Original static helper observed in zModel textured software render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: clip, project, and perspective-correct a textured polygon.
 */
int ClipAndProjectUv(
    zClipRectPartial *clipRect,
    int *vertexCount,
    int hasAttributes
) {
    if (hasAttributes != 0) {
        if (zClipRect::ClipPolyZRange_WithAttr012(
            clipRect,
            vertexCount
        ) == 0) {
            return 0;
        }
    } else if (zClipRect::ClipPolyNearZ(
        clipRect,
        vertexCount
    ) == 0) {
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
        return zClipRect::ClipPoly_WithAttr012(
            clipRect,
            vertexCount
        );
    }
    return zClipRect::ClipPoly(
        clipRect,
        vertexCount
    );
}

/**
 * Original static helper observed in zModel hardware textured render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: multiply current clip UVs by projected reciprocal depth.
 */
void MultiplyUvsByProjectedReciprocalZ(
    int vertexCount
) {
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
void FillPerspectiveUvsForHardwareSubmit(
    zClipUV *outUvs,
    int vertexCount
) {
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
void FillConstantAttrsForGeneratedClipVerts(
    int previousCount,
    int vertexCount
) {
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
int ClipAndProjectHardwareUv(
    zClipRectPartial *clipRect,
    int *vertexCount,
    int hasAttributes
) {
    if ((clipRect->flags & 0x30) != 0) {
        if (hasAttributes != 0) {
            if (zClipRect::ClipPolyZRange_WithAttr012(
                clipRect,
                vertexCount
            ) == 0) {
                return 0;
            }
        } else if (zClipRect::ClipPolyNearZ(
            clipRect,
            vertexCount
        ) == 0) {
            return 0;
        }
    }

    ProjectScratchToClipVerts(*vertexCount);
    MultiplyUvsByProjectedReciprocalZ(*vertexCount);

    if ((clipRect->flags & 0x0f) != 0) {
        const int previousCount = *vertexCount;
        if (hasAttributes != 0) {
            if (zClipRect::ClipPoly_WithAttr012(
                clipRect,
                vertexCount
            ) == 0) {
                return 0;
            }
        } else if (zClipRect::ClipPoly(
            clipRect,
            vertexCount
        ) == 0) {
            return 0;
        }

        if (hasAttributes == 0 && previousCount < *vertexCount) {
            FillConstantAttrsForGeneratedClipVerts(
                previousCount,
                *vertexCount
            );
        }
    }

    return 1;
}

/**
 * Original static helper observed in zModel software textured render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: clip and project a software textured polygon with optional vertex shade.
 */
int ClipAndProjectSoftwareTextured(
    zClipRectPartial *clipRect,
    int *vertexCount,
    int hasPerVertexShade
) {
    if (hasPerVertexShade != 0) {
        if (zClipRect::ClipPolyNearZ_WithAttr0(
            clipRect,
            vertexCount
        ) == 0) {
            return 0;
        }
    } else if (zClipRect::ClipPolyNearZ(
        clipRect,
        vertexCount
    ) == 0) {
        return 0;
    }

    ProjectScratchToClipVerts(*vertexCount);
    if (hasPerVertexShade != 0) {
        return zClipRect::ClipPoly_NoUV_WithAttr0_Alt(
            clipRect,
            vertexCount
        );
    }
    return zClipRect::ClipPoly_NoUV(
        clipRect,
        vertexCount
    );
}

/**
 * Original static helper observed in zModel software/hardware render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: reject projected polygons whose screen-space area is below the configured threshold.
 */
int RejectProjectedSmallPoly(
    int vertexCount
) {
    if (vertexCount <= 0) {
        return 1;
    }

    float twiceArea = 0.0f;
    zClipVert *previous = &g_Clip_PolyVerts[vertexCount - 1];
    for (int i = 0; i < vertexCount; ++i) {
        zClipVert *const current = &g_Clip_PolyVerts[i];
        twiceArea += current->y * previous->x - previous->y * current->x;
        previous = current;
    }

    return fabs(twiceArea) < gModel_SmallPolyRejectArea2x ? 1 : 0;
}

/**
 * Original static helper observed in zModel software triangle render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: copy the first three projected clip vertices into a triangle buffer.
 */
void CopyProjectedTriVerts(
    zVec3 *triVerts
) {
    memcpy(
        triVerts,
        g_Clip_PolyVerts,
        3 * sizeof(zVec3)
    );
}

/**
 * Original static helper observed in zModel software render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: remap projected vertices through the alternate clip-space mapping.
 */
void RemapAltProjectedVerts(
    zVec3 *verts,
    int vertexCount
) {
    for (int i = 0; i < vertexCount; ++i) {
        zClipAlt::RemapPointXYInPlace(&verts[i].x);
    }
}

/**
 * Original static helper observed in zModel software render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: set the renderer inverse-depth bias and scale from draw flags.
 */
void ApplySoftwareDepthScale(
    unsigned int drawFlags
) {
    zRndr::g_inverseDepthBias = 0.0f;
    zRndr::g_inverseDepthScale =
        (float)((short)(drawFlags & 0xffff)) * g_zRndr_InverseZTolerance + 1.0f;
}

/**
 * Original static helper observed in zModel material render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: convert material alpha flags to the current integer render alpha.
 */
int MaterialAlphaInt(
    const zModel_MaterialPartial *material
) {
    const int alpha = (int)(material->flags & 0xff);
    return (int)((float)(alpha)*gModel_RenderAlphaScaleCurrent);
}

/**
 * Original static helper observed in zModel material render paths
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: convert material alpha flags to normalized floating render alpha.
 */
float MaterialAlphaFloat(
    const zModel_MaterialPartial *material
) {
    return (float)(MaterialAlphaInt(material)) * (1.0f / 255.0f);
}

/**
 * Recovered original static helper in D:\Proj\GameZRecoil\zModel\zmodel.cpp.
 * No standalone retail function; observed callers are address-backed zModel
 * material render paths in this source file.
 * Purpose: return the current render-class pointer for a material texture entry.
 */
zVideo_RenderClass *MaterialRenderClass(
    zModel_MaterialPartial *material
) {
    if (material == 0 || material->currentTextureDirectoryEntry == 0) {
        return 0;
    }
    return (zVideo_RenderClass *)(material->currentTextureDirectoryEntry->texture);
}
} // namespace

/**
 * Reimplements data 0x57d92c: gModel_DisplayInitWriteOnlyFlag.
 * Authored zModel display-init lifecycle global.
 * Purpose: record that display initialization has run.
 */
int gModel_DisplayInitWriteOnlyFlag = 0;
/**
 * Reimplements data 0x576210: gModel_RenderMode.
 * Authored zModel display-init lifecycle global.
 * Purpose: select the default model render mode during display initialization.
 */
int gModel_RenderMode = 0;
int g_zModel_DisplayClipMode = 0;
int g_zModel_DisplayClipX = 0;
int g_zModel_DisplayClipY = 0;
float g_zModel_DisplayClipWidth = 0.0f;
float g_zModel_DisplayClipHeight = 0.0f;
float g_zModel_DisplayClipMaxX = 0.0f;
float g_zModel_DisplayClipMaxY = 0.0f;
int g_zModel_DisplayClipReserved = 0;
void *g_zModel_SpanOcclusionProc = 0;
float g_zModel_ViewScaleX = 0.0f;
int g_zModel_ViewScaleYRaw = 0;
float g_zModel_ViewScaleZ = 0.0f;
float g_zModel_FogStart = 0.0f;
float g_zModel_FogEnd = 0.0f;
float g_zModel_FogHeightHigh = 0.0f;
float g_zModel_FogHeightLow = 0.0f;
float g_zModel_FogDistanceInvRange = 0.0f;
float g_zModel_FogHeightInvRange = 0.0f;
float g_zModel_FogDensity = 0.0f;
/**
 * Reimplements data 0x57d928: gModel_DisplayClearedWriteOnlyFlag.
 * Authored zModel display-init lifecycle global.
 * Purpose: clear the display lifecycle write-only state before fog defaults are installed.
 */
int gModel_DisplayClearedWriteOnlyFlag = 0;
int g_zModel_FogReserved = 0;
float g_zModel_FogScale = 0.0f;
/**
 * Reimplements data 0x4e0fc0: Symbol.
 * Authored zModel display global.
 * Purpose: store the backface-elimination tolerance scalar used by display passes.
 */
float g_zModel_BFETolerance = 0.005f;
zVec3 g_zModel_SharedVec3ScratchAStorage[0x400] = {0};
zVec3 g_zModel_SharedVec3ScratchBStorage[0x400] = {0};
/**
 * Reimplements data 0x57c2bc: g_zModel_TransformedVerts.
 * Authored zModel display scratch pointer global.
 * Purpose: point transformed-vertex passes at the primary shared Vec3 scratch buffer.
 */
zVec3 *g_zModel_TransformedVerts = 0;
/**
 * Reimplements data 0x57c2c0: g_zModel_TransformedNormals.
 * Authored zModel display scratch pointer global.
 * Purpose: point transformed-normal passes at the secondary shared Vec3 scratch buffer.
 */
zVec3 *g_zModel_TransformedNormals = 0;
/**
 * Reimplements data 0x57d97c: Symbol.
 * Authored zModel display global.
 * Purpose: point scratch users at the primary shared transformed-vector buffer.
 */
zVec3 *g_zModel_SharedVec3ScratchA = 0;
/**
 * Reimplements data 0x57d980: Symbol.
 * Authored zModel display global.
 * Purpose: point scratch users at the secondary shared transformed-vector buffer.
 */
zVec3 *g_zModel_SharedVec3ScratchB = 0;
/**
 * Reimplements data 0x57d984: Symbol.
 * Authored zModel display global.
 * Purpose: alias point-in-polygon vertices to the current primary scratch buffer.
 */
zVec3 *g_zModel_PointInPolygonVertices = 0;
/**
 * Reimplements data 0x57d988: Symbol.
 * Authored zModel display global.
 * Purpose: alias point-in-polygon edge normals to the current secondary scratch buffer.
 */
zVec3 *g_zModel_PointInPolygonEdgeNormals = 0;
/**
 * Reimplements data 0x57d98c: Symbol.
 * Authored zModel display global.
 * Purpose: track the number of points in the current point-in-polygon scratch set.
 */
int g_zModel_PointInPolygonVertexCount = 0;
/**
 * Reimplements data 0x57d990: Symbol.
 * Authored zModel display global.
 * Purpose: store the world-space texture U origin used by model display setup.
 */
float g_zModel_TextureWorldBaseU = 0.0f;
/**
 * Reimplements data 0x57d994: Symbol.
 * Authored zModel display global.
 * Purpose: store the world-space texture V origin used by model display setup.
 */
float g_zModel_TextureWorldBaseV = 0.0f;
/**
 * Reimplements data 0x57d998: Symbol.
 * Authored zModel display global.
 * Purpose: store the world-space texture U scale used by model display setup.
 */
float g_zModel_TextureWorldPerMeterU = 0.0f;
/**
 * Reimplements data 0x57d99c: Symbol.
 * Authored zModel display global.
 * Purpose: store the world-space texture V scale used by model display setup.
 */
float g_zModel_TextureWorldPerMeterV = 0.0f;
int g_zModel_ScratchCounters[8] = {0};
float g_zModel_PointInPolyTolX = 0.0f;
float g_zModel_PointInPolyTolY = 0.0f;
unsigned char g_zModel_DamageMaskStorage[0x200] = {0};
void *g_zModel_DamageMaskCurrent = 0;
/**
 * Reimplements data 0x57d9a0: Symbol.
 * Authored OptCatalog damage-mask global.
 * Purpose: gate whether damage-mask stamping is active for hit surfaces.
 */
int g_OptCatalogDamageMaskEnabled = 0;
/**
 * Reimplements data 0x57d9a4: Symbol.
 * Authored OptCatalog damage-mask global.
 * Purpose: select which registered damage-mask handle slot is active.
 */
int g_OptCatalogDamageMaskSlotIndex = 0;
void *g_OptCatalogDamageMaskHandles[3] = {0};
/**
 * Reimplements data 0x57d9b4: Symbol.
 * Authored OptCatalog damage-mask global.
 * Purpose: store the current damage-mask U phase before stamp wrapping.
 */
float g_OptCatalogDamageMaskPhaseU = 0.0f;
/**
 * Reimplements data 0x57d9b8: Symbol.
 * Authored OptCatalog damage-mask global.
 * Purpose: store the current damage-mask V phase before stamp wrapping.
 */
float g_OptCatalogDamageMaskPhaseV = 0.0f;
int g_zModel_OptCatalogAux0 = 0;
int g_zModel_OptCatalogAux1 = 0;
/**
 * Reimplements data 0x57d9bc: Symbol.
 * Authored zModel display global.
 * Purpose: provide the fallback graphics-flags storage when the options catalog has no entry.
 */
int gModel_DefaultGraphicsFlags = 0;
/**
 * Reimplements data 0x57d9c0: gModel_pGraphicsFlags.
 * Authored zModel display global.
 * Purpose: point model display code at the active graphics-flags integer value.
 */
int *gModel_pGraphicsFlags = 0;
extern "C" {
/**
 * Reimplements data 0x57d9e0: gModel_RenderFn.
 * Authored zModel display global.
 * Purpose: dispatch visible model nodes to the active renderer path.
 */
zClass_RenderFn gModel_RenderFn = 0;
/**
 * Reimplements data 0x57d9e4: gModel_ClipMaskStack.
 * Authored zModel display global.
 * Purpose: store nested model clip masks for zClass render traversal.
 */
int gModel_ClipMaskStack[0x10] = {0};
/**
 * Reimplements data 0x57da24: gModel_ClipMaskStackTop.
 * Authored zModel display global.
 * Purpose: track the current entry in the model clip-mask stack.
 */
int *gModel_ClipMaskStackTop = 0;
}
/**
 * Reimplements data 0x4dd90c: Symbol.
 * Authored variant-filter global.
 * Purpose: gate whether variant tag comparisons filter model display entries.
 */
int g_Variant_FilterEnabled = 1;
zTag4Partial g_VariantTag_Current = {0};
zTag4Partial g_Variant_CurrentTag = {0};

namespace {
/**
 * Original static helper observed in zModel_Display projected-sphere callers
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: truncate a projected floating-point coordinate to integer screen space.
 */
int TruncateToInt(
    float value
) {
    return (int)(value);
}

/**
 * Original static helper observed in zModel_Display projected-sphere callers
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: query whether the span occlusion buffer leaves a projected column visible.
 */
bool TestSpanColumnVisible(
    int columnIndex
) {
    int isVisible = 0;
    zRndr_SpanOcclusion_TestColumnVisibility(
        columnIndex,
        &isVisible
    );
    return isVisible > 0;
}

/**
 * Original static helper observed in zModel_Display damage-mask stamping
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: wrap a damage-mask UV phase into the repeat range used by the stamp.
 */
void WrapDamageMaskPhase(
    float *phase
) {
    while (*phase > 1.01f) {
        *phase -= 1.0f;
    }
    while (*phase < -0.01f) {
        *phase += 1.0f;
    }
}

/**
 * Original static helper observed in zModel_Display damage-mask stamping
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: clamp one source/destination axis for a damage-mask stamp copy.
 */
void ClampDamageMaskAxis(
    int dstCoord,
    int srcSize,
    int dstSize,
    int *srcBegin,
    int *srcEnd,
    int *dstBegin
) {
    if (srcSize > dstSize) {
        *dstBegin = 0;
        *srcBegin = (srcSize - dstSize) >> 1;
        *srcEnd = srcSize - *srcBegin;
        return;
    }

    *srcBegin = 0;
    *srcEnd = srcSize;
    *dstBegin = dstCoord;
    if (dstCoord < 0) {
        *dstBegin = 0;
        *srcEnd = srcSize;
    } else if (dstCoord + srcSize > dstSize) {
        *dstBegin = dstCoord - (dstCoord + srcSize) + dstSize;
    }
}

/**
 * Original static helper observed in zModel_Display damage-mask stamping
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: blend one RGB565 destination pixel with a damage-mask source pixel.
 */
unsigned short BlendDamageMaskPixel565(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
) {
    unsigned int dst = dstPixel;
    const unsigned int src = srcPixel;
    dst += ((((src & 0xf800) - (dst & 0xf800)) * alpha) >> 8) & 0xfffff800;
    const unsigned int green = ((((src & 0x07e0) - (dstPixel & 0x07e0)) * alpha) >> 8) & 0xffffffe0;
    const unsigned int blue = (((src & 0x001f) - (dst & 0x001f)) * alpha) >> 8;
    return (unsigned short)(dst + green + blue);
}

/**
 * Original static helper observed in zModel_Display damage-mask stamping
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: blend one RGB555 destination pixel with a damage-mask source pixel.
 */
unsigned short BlendDamageMaskPixel555(
    unsigned short dstPixel,
    unsigned short srcPixel,
    int alpha
) {
    const unsigned int dst = dstPixel;
    const unsigned int src = srcPixel;
    const unsigned int red = ((((src & 0x7c00) - (dst & 0x7c00)) * alpha) >> 8) & 0xfffffc00;
    const unsigned int green = ((((src & 0x03e0) - (dst & 0x03e0)) * alpha) >> 8) & 0xffffffe0;
    const unsigned int blue = (((src & 0x001f) - (dst & 0x001f)) * alpha) >> 8;
    return (unsigned short)(dst + red + green + blue);
}

} // namespace

/**
 * Reimplements data 0x57624c: gModel_SmallPolyRejectArea2x.
 * Purpose: cache the doubled small-polygon reject-area threshold.
 */
float gModel_SmallPolyRejectArea2x = 0.0f;
/**
 * Reimplements data 0x576250: gModel_SmallPolyRejectArea20x.
 * Purpose: cache the twenty-times small-polygon reject-area threshold.
 */
float gModel_SmallPolyRejectArea20x = 0.0f;

extern "C" {
/**
 * Source owner evidence: zClipAlt is a namespace/data utility cluster over alternate clip rectangles,
 * remap globals, and typed zClipRect/zMath provider calls.
 * Evidence: BN facts for 0x476120, 0x479f90, 0x4766a0, and 0x47a1d0 show no constructor,
 * destructor, table write, or class-instance field access; the functions operate on file-scope
 * rectangle/remap state and passed camera/rect records.
 * Purpose: Keep the recovered alternate-clip state as typed source-level globals rather than a
 * class/table scaffold.
 */

/**
 * Reimplements data 0x57628c: g_zClipAlt_SourceLeft.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Hold the source rectangle left edge for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceLeft = 0.0f;

/**
 * Reimplements data 0x576290: g_zClipAlt_SourceTop.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Hold the source rectangle top edge for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceTop = 0.0f;

/**
 * Reimplements data 0x576294: g_zClipAlt_SourceRight.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Hold the source rectangle right edge for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceRight = 0.0f;

/**
 * Reimplements data 0x576298: g_zClipAlt_SourceBottom.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Hold the source rectangle bottom edge for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceBottom = 0.0f;

/**
 * Reimplements data 0x57629c: g_zClipAlt_SourceWidth.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Cache the source rectangle width for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceWidth = 0.0f;

/**
 * Reimplements data 0x5762a0: g_zClipAlt_SourceHeight.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Cache the source rectangle height for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceHeight = 0.0f;

/**
 * Data owner: zClipAlt target clipping rectangle.
 * Purpose: Hold the alternate clipping bounds used by zClipRect rejection and clipping routines.
 */
zClipRectPartial gClipRect_Alt = {0};

/**
 * Reimplements data 0x5762a4: g_zClipAlt_RemapOffsetX.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the source-to-target X offset for alternate clipped points.
 */
float g_zClipAlt_RemapOffsetX = 0.0f;

/**
 * Reimplements data 0x5762a8: g_zClipAlt_RemapOffsetY.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the source-to-target Y offset for alternate clipped points.
 */
float g_zClipAlt_RemapOffsetY = 0.0f;

/**
 * Reimplements data 0x5762ac: g_zClipAlt_RemapScaleX.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the X scale used to remap alternate clipped points.
 */
float g_zClipAlt_RemapScaleX = 0.0f;

/**
 * Reimplements data 0x5762b0: g_zClipAlt_RemapScaleY.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the Y scale used to remap alternate clipped points.
 */
float g_zClipAlt_RemapScaleY = 0.0f;

/**
 * Reimplements data 0x5762b4: g_zClipAlt_RemapBiasX.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the X bias used to remap alternate clipped points.
 */
float g_zClipAlt_RemapBiasX = 0.0f;

/**
 * Reimplements data 0x5762b8: g_zClipAlt_RemapBiasY.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the Y bias used to remap alternate clipped points.
 */
float g_zClipAlt_RemapBiasY = 0.0f;

/**
 * Reimplements data 0x5669e4: g_zClipAlt_BiasIncludesPrimaryOrigin.
 * Data owner: zClipAlt remap state.
 * Purpose: Select whether remap bias includes the primary clip origin.
 */
int g_zClipAlt_BiasIncludesPrimaryOrigin = 0;

/**
 * Reimplements data 0x576254: gAltClipSourceRectValid.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Record whether the alternate clipping source rectangle has been configured.
 */
int gAltClipSourceRectValid = 0;

/**
 * Reimplements data 0x57da2c: gAltClipPassEnabled.
 * Data owner: zClipAlt pass state.
 * Purpose: Record whether the alternate clipping pass is enabled.
 */
int gAltClipPassEnabled = 0;
}

/**
 * Reimplements data 0x57c8c4: g_Clip_PolyVerts.
 * Data owner: zClipRect polygon clipping scratch vertices.
 * Purpose: Hold the active polygon vertex stream for XY clipping and rejection.
 */
zClipVert g_Clip_PolyVerts[0x40] = {0};

/**
 * Reimplements data 0x57c5c4: g_Clip_PolyVertsScratch.
 * Data owner: zClipRect polygon clipping scratch vertices.
 * Purpose: Hold the alternate polygon vertex stream for Z-range clipping passes.
 */
zClipVert g_Clip_PolyVertsScratch[0x40] = {0};

/**
 * Reimplements data 0x57cbc4: g_Clip_PolyUvsStorage.
 * Data owner: zClipRect polygon clipping scratch UV storage.
 * Purpose: Provide default UV storage for clipping passes that preserve texture coordinates.
 */
zClipUV g_Clip_PolyUvsStorage[0x40] = {0};

/**
 * Reimplements data 0x57cdc4: g_Clip_PolyUvs.
 * Data owner: zClipRect polygon clipping scratch UV cursor.
 * Purpose: Select the active UV stream used by polygon clipping passes.
 */
zClipUV *g_Clip_PolyUvs = 0;

/**
 * Data owner: zClipRect primary clipping rectangle.
 * Purpose: Hold the primary screen clip bounds used by model and alternate clipping callers.
 */
zClipRectPartial gClipRect_Primary = {0};

namespace {
const int kClipBufferCapacity = 0x40;

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: test whether one vertex is inside the near-Z clipping plane.
 */
bool IsInsideNear(
    const zClipVert &vertex,
    float zMin
) {
    return vertex.z >= zMin;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: test whether one scalar is on or above a minimum clip bound.
 */
bool IsInsideMin(
    float value,
    float minValue
) {
    return value >= minValue;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: test whether one scalar is below a maximum clip bound.
 */
bool IsInsideMax(
    float value,
    float maxValue
) {
    return value < maxValue;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: interpolate one vertex and clamp the resulting Z value to a clip plane.
 */
zClipVert InterpolateVert(
    const zClipVert &a,
    const zClipVert &b,
    float t,
    float z
) {
    zClipVert out = {0};
    out.x = a.x + (b.x - a.x) * t;
    out.y = a.y + (b.y - a.y) * t;
    out.z = z;
    return out;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: interpolate one vertex on an XY clip axis and clamp that axis to the clip bound.
 */
zClipVert InterpolateVertOnAxis(
    const zClipVert &a,
    const zClipVert &b,
    float t,
    int axis,
    float bound
) {
    zClipVert out = {0};
    out.x = a.x + (b.x - a.x) * t;
    out.y = a.y + (b.y - a.y) * t;
    out.z = a.z + (b.z - a.z) * t;
    if (axis == 0) {
        out.x = bound;
    } else {
        out.y = bound;
    }
    return out;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: interpolate one texture-coordinate pair between clipped polygon edges.
 */
zClipUV InterpolateUv(
    const zClipUV &a,
    const zClipUV &b,
    float t
) {
    zClipUV out = {0};
    out.u = a.u + (b.u - a.u) * t;
    out.v = a.v + (b.v - a.v) * t;
    return out;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: interpolate one per-vertex scalar attribute between clipped polygon edges.
 */
float InterpolateFloat(
    float a,
    float b,
    float t
) {
    return a + (b - a) * t;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: append one clipped vertex and UV pair to a bounded scratch stream.
 */
void AppendClipped(
    zClipVert *verts,
    zClipUV *uvs,
    int &count,
    const zClipVert &vert,
    const zClipUV &uv
) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    uvs[count] = uv;
    ++count;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: append one clipped vertex to a bounded scratch stream.
 */
void AppendClippedVert(
    zClipVert *verts,
    int &count,
    const zClipVert &vert
) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    ++count;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: append one clipped vertex, UV pair, and scalar attribute to bounded scratch streams.
 */
void AppendClippedWithAttr(
    zClipVert *verts,
    zClipUV *uvs,
    float *attrs,
    int &count,
    const zClipVert &vert,
    const zClipUV &uv,
    float attr
) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    uvs[count] = uv;
    attrs[count] = attr;
    ++count;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: append one clipped vertex, UV pair, and three scalar attributes to bounded scratch streams.
 */
void AppendClippedWithAttr012(
    zClipVert *verts,
    zClipUV *uvs,
    float *attr0,
    float *attr1,
    float *attr2,
    int &count,
    const zClipVert &vert,
    const zClipUV &uv,
    float value0,
    float value1,
    float value2
) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    uvs[count] = uv;
    attr0[count] = value0;
    attr1[count] = value1;
    attr2[count] = value2;
    ++count;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: append one clipped vertex and three scalar attributes to bounded scratch streams.
 */
void AppendClippedVertWithAttr012(
    zClipVert *verts,
    float *attr0,
    float *attr1,
    float *attr2,
    int &count,
    const zClipVert &vert,
    float value0,
    float value1,
    float value2
) {
    if (count >= kClipBufferCapacity) {
        return;
    }

    verts[count] = vert;
    attr0[count] = value0;
    attr1[count] = value1;
    attr2[count] = value2;
    ++count;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: clip one vertex stream against a single XY plane.
 */
int ClipVertsAgainstPlane(
    const zClipVert *source,
    int sourceCount,
    zClipVert *dest,
    int axis,
    float bound,
    bool clipMin
) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = source[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(
        prevValue,
        bound
    ) : IsInsideMax(
        prevValue,
        bound
    );

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = source[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(
                currValue,
                bound
            ) : IsInsideMax(
                currValue,
                bound
            );

        if (prevInside != currInside) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            if (destCount < kClipBufferCapacity) {
                dest[destCount] = InterpolateVertOnAxis(
                    prevVert,
                    currVert,
                    t,
                    axis,
                    bound
                );
                ++destCount;
            }
        }

        if (currInside && destCount < kClipBufferCapacity) {
            dest[destCount] = currVert;
            ++destCount;
        }

        prevVert = currVert;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: clip vertex and UV streams against a single XY plane.
 */
int ClipVertsUvsAgainstPlane(
    const zClipVert *sourceVerts,
    const zClipUV *sourceUvs,
    int sourceCount,
    zClipVert *destVerts,
    zClipUV *destUvs,
    int axis,
    float bound,
    bool clipMin
) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = sourceVerts[sourceCount - 1];
    zClipUV prevUv = sourceUvs[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(
        prevValue,
        bound
    ) : IsInsideMax(
        prevValue,
        bound
    );

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = sourceVerts[i];
        const zClipUV currUv = sourceUvs[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(
                currValue,
                bound
            ) : IsInsideMax(
                currValue,
                bound
            );

        if (prevInside != currInside && destCount < kClipBufferCapacity) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            destVerts[destCount] = InterpolateVertOnAxis(
                prevVert,
                currVert,
                t,
                axis,
                bound
            );
            destUvs[destCount] = InterpolateUv(
                prevUv,
                currUv,
                t
            );
            ++destCount;
        }

        if (currInside && destCount < kClipBufferCapacity) {
            destVerts[destCount] = currVert;
            destUvs[destCount] = currUv;
            ++destCount;
        }

        prevVert = currVert;
        prevUv = currUv;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: clip vertex and first-attribute streams against a single XY plane.
 */
int ClipVertsAttr0AgainstPlane(
    const zClipVert *sourceVerts,
    const float *sourceAttrs,
    int sourceCount,
    zClipVert *destVerts,
    float *destAttrs,
    int axis,
    float bound,
    bool clipMin
) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = sourceVerts[sourceCount - 1];
    float prevAttr = sourceAttrs[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(
        prevValue,
        bound
    ) : IsInsideMax(
        prevValue,
        bound
    );

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = sourceVerts[i];
        const float currAttr = sourceAttrs[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(
                currValue,
                bound
            ) : IsInsideMax(
                currValue,
                bound
            );

        if (prevInside != currInside && destCount < kClipBufferCapacity) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            destVerts[destCount] = InterpolateVertOnAxis(
                prevVert,
                currVert,
                t,
                axis,
                bound
            );
            destAttrs[destCount] = InterpolateFloat(
                prevAttr,
                currAttr,
                t
            );
            ++destCount;
        }

        if (currInside && destCount < kClipBufferCapacity) {
            destVerts[destCount] = currVert;
            destAttrs[destCount] = currAttr;
            ++destCount;
        }

        prevVert = currVert;
        prevAttr = currAttr;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: clip vertex and three-attribute streams against a single XY plane.
 */
int ClipVertsAttr012AgainstPlane(
    const zClipVert *sourceVerts,
    const float *sourceAttr0,
    const float *sourceAttr1,
    const float *sourceAttr2,
    int sourceCount,
    zClipVert *destVerts,
    float *destAttr0,
    float *destAttr1,
    float *destAttr2,
    int axis,
    float bound,
    bool clipMin
) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = sourceVerts[sourceCount - 1];
    float prevAttr0 = sourceAttr0[sourceCount - 1];
    float prevAttr1 = sourceAttr1[sourceCount - 1];
    float prevAttr2 = sourceAttr2[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(
        prevValue,
        bound
    ) : IsInsideMax(
        prevValue,
        bound
    );

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = sourceVerts[i];
        const float currAttr0 = sourceAttr0[i];
        const float currAttr1 = sourceAttr1[i];
        const float currAttr2 = sourceAttr2[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(
                currValue,
                bound
            ) : IsInsideMax(
                currValue,
                bound
            );

        if (prevInside != currInside && destCount < kClipBufferCapacity) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            destVerts[destCount] = InterpolateVertOnAxis(
                prevVert,
                currVert,
                t,
                axis,
                bound
            );
            destAttr0[destCount] = InterpolateFloat(
                prevAttr0,
                currAttr0,
                t
            );
            destAttr1[destCount] = InterpolateFloat(
                prevAttr1,
                currAttr1,
                t
            );
            destAttr2[destCount] = InterpolateFloat(
                prevAttr2,
                currAttr2,
                t
            );
            ++destCount;
        }

        if (currInside && destCount < kClipBufferCapacity) {
            destVerts[destCount] = currVert;
            destAttr0[destCount] = currAttr0;
            destAttr1[destCount] = currAttr1;
            destAttr2[destCount] = currAttr2;
            ++destCount;
        }

        prevVert = currVert;
        prevAttr0 = currAttr0;
        prevAttr1 = currAttr1;
        prevAttr2 = currAttr2;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: clip vertex, UV, and three-attribute streams against a single XY plane.
 */
int ClipVertsUvsAttr012AgainstPlane(
    const zClipVert *sourceVerts,
    const zClipUV *sourceUvs,
    const float *sourceAttr0,
    const float *sourceAttr1,
    const float *sourceAttr2,
    int sourceCount,
    zClipVert *destVerts,
    zClipUV *destUvs,
    float *destAttr0,
    float *destAttr1,
    float *destAttr2,
    int axis,
    float bound,
    bool clipMin
) {
    int destCount = 0;
    if (sourceCount <= 0) {
        return 0;
    }

    zClipVert prevVert = sourceVerts[sourceCount - 1];
    zClipUV prevUv = sourceUvs[sourceCount - 1];
    float prevAttr0 = sourceAttr0[sourceCount - 1];
    float prevAttr1 = sourceAttr1[sourceCount - 1];
    float prevAttr2 = sourceAttr2[sourceCount - 1];
    float prevValue = axis == 0 ? prevVert.x : prevVert.y;
    bool prevInside = clipMin ? IsInsideMin(
        prevValue,
        bound
    ) : IsInsideMax(
        prevValue,
        bound
    );

    for (int i = 0; i < sourceCount; ++i) {
        const zClipVert currVert = sourceVerts[i];
        const zClipUV currUv = sourceUvs[i];
        const float currAttr0 = sourceAttr0[i];
        const float currAttr1 = sourceAttr1[i];
        const float currAttr2 = sourceAttr2[i];
        const float currValue = axis == 0 ? currVert.x : currVert.y;
        const bool currInside =
            clipMin ? IsInsideMin(
                currValue,
                bound
            ) : IsInsideMax(
                currValue,
                bound
            );

        if (prevInside != currInside && destCount < kClipBufferCapacity) {
            const float t = (bound - prevValue) / (currValue - prevValue);
            destVerts[destCount] = InterpolateVertOnAxis(
                prevVert,
                currVert,
                t,
                axis,
                bound
            );
            destUvs[destCount] = InterpolateUv(
                prevUv,
                currUv,
                t
            );
            destAttr0[destCount] = InterpolateFloat(
                prevAttr0,
                currAttr0,
                t
            );
            destAttr1[destCount] = InterpolateFloat(
                prevAttr1,
                currAttr1,
                t
            );
            destAttr2[destCount] = InterpolateFloat(
                prevAttr2,
                currAttr2,
                t
            );
            ++destCount;
        }

        if (currInside && destCount < kClipBufferCapacity) {
            destVerts[destCount] = currVert;
            destUvs[destCount] = currUv;
            destAttr0[destCount] = currAttr0;
            destAttr1[destCount] = currAttr1;
            destAttr2[destCount] = currAttr2;
            ++destCount;
        }

        prevVert = currVert;
        prevUv = currUv;
        prevAttr0 = currAttr0;
        prevAttr1 = currAttr1;
        prevAttr2 = currAttr2;
        prevValue = currValue;
        prevInside = currInside;
    }

    return destCount;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: run the shared no-UV XY clipping pass over the active polygon vertex stream.
 */
int ClipPolyNoUvCore(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchA[kClipBufferCapacity] = {0};
    zClipVert scratchB[kClipBufferCapacity] = {0};
    const zClipVert *source = g_Clip_PolyVerts;
    zClipVert *dest = scratchA;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsAgainstPlane(
            source,
            count,
            dest,
            0,
            clipRect->xMin,
            true
        );
        source = dest;
        dest = scratchB;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsAgainstPlane(
            source,
            count,
            dest,
            0,
            clipRect->xMaxAlt,
            false
        );
        source = dest;
        dest = dest == scratchA ? scratchB : scratchA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsAgainstPlane(
            source,
            count,
            dest,
            1,
            clipRect->yMin,
            true
        );
        source = dest;
        dest = dest == scratchA ? scratchB : scratchA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsAgainstPlane(
            source,
            count,
            dest,
            1,
            clipRect->yMaxAlt,
            false
        );
        source = dest;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (source != g_Clip_PolyVerts) {
        memcpy(
            g_Clip_PolyVerts,
            source,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
    }
    return 1;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: run the shared UV-preserving XY clipping pass over active polygon streams.
 */
int ClipPolyUvCore(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVertsA[kClipBufferCapacity] = {0};
    zClipVert scratchVertsB[kClipBufferCapacity] = {0};
    zClipUV scratchUvsA[kClipBufferCapacity] = {0};
    zClipUV scratchUvsB[kClipBufferCapacity] = {0};
    const zClipVert *sourceVerts = g_Clip_PolyVerts;
    const zClipUV *sourceUvs = g_Clip_PolyUvs;
    zClipVert *destVerts = scratchVertsA;
    zClipUV *destUvs = scratchUvsA;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsUvsAgainstPlane(
            sourceVerts,
            sourceUvs,
            count,
            destVerts,
            destUvs,
            0,
            clipRect->xMin,
            true
        );
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        destVerts = scratchVertsB;
        destUvs = scratchUvsB;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsUvsAgainstPlane(
            sourceVerts,
            sourceUvs,
            count,
            destVerts,
            destUvs,
            0,
            clipRect->xMaxAlt,
            false
        );
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destUvs = wroteA ? scratchUvsB : scratchUvsA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsUvsAgainstPlane(
            sourceVerts,
            sourceUvs,
            count,
            destVerts,
            destUvs,
            1,
            clipRect->yMin,
            true
        );
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destUvs = wroteA ? scratchUvsB : scratchUvsA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsUvsAgainstPlane(
            sourceVerts,
            sourceUvs,
            count,
            destVerts,
            destUvs,
            1,
            clipRect->yMaxAlt,
            false
        );
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (sourceVerts != g_Clip_PolyVerts) {
        memcpy(
            g_Clip_PolyVerts,
            sourceVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
    }
    if (sourceUvs != g_Clip_PolyUvs) {
        memcpy(
            g_Clip_PolyUvs,
            sourceUvs,
            (size_t)(outputCount) * sizeof(zClipUV)
        );
    }
    return 1;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: run the shared no-UV first-attribute XY clipping pass over active polygon streams.
 */
int ClipPolyAttr0NoUvCore(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVertsA[kClipBufferCapacity] = {0};
    zClipVert scratchVertsB[kClipBufferCapacity] = {0};
    float scratchAttrsA[kClipBufferCapacity] = {0};
    float scratchAttrsB[kClipBufferCapacity] = {0};
    const zClipVert *sourceVerts = g_Clip_PolyVerts;
    const float *sourceAttrs = g_Clip_PolyAttr0;
    zClipVert *destVerts = scratchVertsA;
    float *destAttrs = scratchAttrsA;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsAttr0AgainstPlane(
            sourceVerts,
            sourceAttrs,
            count,
            destVerts,
            destAttrs,
            0,
            clipRect->xMin,
            true
        );
        sourceVerts = destVerts;
        sourceAttrs = destAttrs;
        destVerts = scratchVertsB;
        destAttrs = scratchAttrsB;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsAttr0AgainstPlane(
            sourceVerts,
            sourceAttrs,
            count,
            destVerts,
            destAttrs,
            0,
            clipRect->xMaxAlt,
            false
        );
        sourceVerts = destVerts;
        sourceAttrs = destAttrs;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destAttrs = wroteA ? scratchAttrsB : scratchAttrsA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsAttr0AgainstPlane(
            sourceVerts,
            sourceAttrs,
            count,
            destVerts,
            destAttrs,
            1,
            clipRect->yMin,
            true
        );
        sourceVerts = destVerts;
        sourceAttrs = destAttrs;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destAttrs = wroteA ? scratchAttrsB : scratchAttrsA;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsAttr0AgainstPlane(
            sourceVerts,
            sourceAttrs,
            count,
            destVerts,
            destAttrs,
            1,
            clipRect->yMaxAlt,
            false
        );
        sourceVerts = destVerts;
        sourceAttrs = destAttrs;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (sourceVerts != g_Clip_PolyVerts) {
        memcpy(
            g_Clip_PolyVerts,
            sourceVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
    }
    if (sourceAttrs != g_Clip_PolyAttr0) {
        memcpy(
            g_Clip_PolyAttr0,
            sourceAttrs,
            (size_t)(outputCount) * sizeof(float)
        );
    }
    return 1;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: run the shared no-UV three-attribute XY clipping pass over active polygon streams.
 */
int ClipPolyAttr012NoUvCore(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVertsA[kClipBufferCapacity] = {0};
    zClipVert scratchVertsB[kClipBufferCapacity] = {0};
    float scratchAttr0A[kClipBufferCapacity] = {0};
    float scratchAttr0B[kClipBufferCapacity] = {0};
    float scratchAttr1A[kClipBufferCapacity] = {0};
    float scratchAttr1B[kClipBufferCapacity] = {0};
    float scratchAttr2A[kClipBufferCapacity] = {0};
    float scratchAttr2B[kClipBufferCapacity] = {0};
    const zClipVert *sourceVerts = g_Clip_PolyVerts;
    const float *sourceAttr0 = g_Clip_PolyAttr0;
    const float *sourceAttr1 = g_Clip_PolyAttr1;
    const float *sourceAttr2 = g_Clip_PolyAttr2;
    zClipVert *destVerts = scratchVertsA;
    float *destAttr0 = scratchAttr0A;
    float *destAttr1 = scratchAttr1A;
    float *destAttr2 = scratchAttr2A;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsAttr012AgainstPlane(
            sourceVerts,
            sourceAttr0,
            sourceAttr1,
            sourceAttr2,
            count,
            destVerts,
            destAttr0,
            destAttr1,
            destAttr2,
            0,
            clipRect->xMin,
            true
        );
        sourceVerts = destVerts;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        destVerts = scratchVertsB;
        destAttr0 = scratchAttr0B;
        destAttr1 = scratchAttr1B;
        destAttr2 = scratchAttr2B;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsAttr012AgainstPlane(
            sourceVerts,
            sourceAttr0,
            sourceAttr1,
            sourceAttr2,
            count,
            destVerts,
            destAttr0,
            destAttr1,
            destAttr2,
            0,
            clipRect->xMaxAlt,
            false
        );
        sourceVerts = destVerts;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destAttr0 = wroteA ? scratchAttr0B : scratchAttr0A;
        destAttr1 = wroteA ? scratchAttr1B : scratchAttr1A;
        destAttr2 = wroteA ? scratchAttr2B : scratchAttr2A;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsAttr012AgainstPlane(
            sourceVerts,
            sourceAttr0,
            sourceAttr1,
            sourceAttr2,
            count,
            destVerts,
            destAttr0,
            destAttr1,
            destAttr2,
            1,
            clipRect->yMin,
            true
        );
        sourceVerts = destVerts;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destAttr0 = wroteA ? scratchAttr0B : scratchAttr0A;
        destAttr1 = wroteA ? scratchAttr1B : scratchAttr1A;
        destAttr2 = wroteA ? scratchAttr2B : scratchAttr2A;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsAttr012AgainstPlane(
            sourceVerts,
            sourceAttr0,
            sourceAttr1,
            sourceAttr2,
            count,
            destVerts,
            destAttr0,
            destAttr1,
            destAttr2,
            1,
            clipRect->yMaxAlt,
            false
        );
        sourceVerts = destVerts;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (sourceVerts != g_Clip_PolyVerts) {
        memcpy(
            g_Clip_PolyVerts,
            sourceVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
        memcpy(
            g_Clip_PolyAttr0,
            sourceAttr0,
            (size_t)(outputCount) * sizeof(float)
        );
        memcpy(
            g_Clip_PolyAttr1,
            sourceAttr1,
            (size_t)(outputCount) * sizeof(float)
        );
        memcpy(
            g_Clip_PolyAttr2,
            sourceAttr2,
            (size_t)(outputCount) * sizeof(float)
        );
    }
    return 1;
}

/**
 * Original static helper recovered from the zClipRect source-file cluster.
 * Purpose: run the shared UV and three-attribute XY clipping pass over active polygon streams.
 */
int ClipPolyAttr012UvCore(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVertsA[kClipBufferCapacity] = {0};
    zClipVert scratchVertsB[kClipBufferCapacity] = {0};
    zClipUV scratchUvsA[kClipBufferCapacity] = {0};
    zClipUV scratchUvsB[kClipBufferCapacity] = {0};
    float scratchAttr0A[kClipBufferCapacity] = {0};
    float scratchAttr0B[kClipBufferCapacity] = {0};
    float scratchAttr1A[kClipBufferCapacity] = {0};
    float scratchAttr1B[kClipBufferCapacity] = {0};
    float scratchAttr2A[kClipBufferCapacity] = {0};
    float scratchAttr2B[kClipBufferCapacity] = {0};
    const zClipVert *sourceVerts = g_Clip_PolyVerts;
    const zClipUV *sourceUvs = g_Clip_PolyUvs;
    const float *sourceAttr0 = g_Clip_PolyAttr0;
    const float *sourceAttr1 = g_Clip_PolyAttr1;
    const float *sourceAttr2 = g_Clip_PolyAttr2;
    zClipVert *destVerts = scratchVertsA;
    zClipUV *destUvs = scratchUvsA;
    float *destAttr0 = scratchAttr0A;
    float *destAttr1 = scratchAttr1A;
    float *destAttr2 = scratchAttr2A;
    int count = *vertexCount;
    int outputCount = 0;
    bool clippedAnyPlane = false;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = ClipVertsUvsAttr012AgainstPlane(
            sourceVerts,
            sourceUvs,
            sourceAttr0,
            sourceAttr1,
            sourceAttr2,
            count,
            destVerts,
            destUvs,
            destAttr0,
            destAttr1,
            destAttr2,
            0,
            clipRect->xMin,
            true
        );
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        destVerts = scratchVertsB;
        destUvs = scratchUvsB;
        destAttr0 = scratchAttr0B;
        destAttr1 = scratchAttr1B;
        destAttr2 = scratchAttr2B;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x02) != 0) {
        outputCount = ClipVertsUvsAttr012AgainstPlane(
            sourceVerts,
            sourceUvs,
            sourceAttr0,
            sourceAttr1,
            sourceAttr2,
            count,
            destVerts,
            destUvs,
            destAttr0,
            destAttr1,
            destAttr2,
            0,
            clipRect->xMaxAlt,
            false
        );
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destUvs = wroteA ? scratchUvsB : scratchUvsA;
        destAttr0 = wroteA ? scratchAttr0B : scratchAttr0A;
        destAttr1 = wroteA ? scratchAttr1B : scratchAttr1A;
        destAttr2 = wroteA ? scratchAttr2B : scratchAttr2A;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x04) != 0) {
        outputCount = ClipVertsUvsAttr012AgainstPlane(
            sourceVerts,
            sourceUvs,
            sourceAttr0,
            sourceAttr1,
            sourceAttr2,
            count,
            destVerts,
            destUvs,
            destAttr0,
            destAttr1,
            destAttr2,
            1,
            clipRect->yMin,
            true
        );
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        const bool wroteA = destVerts == scratchVertsA;
        destVerts = wroteA ? scratchVertsB : scratchVertsA;
        destUvs = wroteA ? scratchUvsB : scratchUvsA;
        destAttr0 = wroteA ? scratchAttr0B : scratchAttr0A;
        destAttr1 = wroteA ? scratchAttr1B : scratchAttr1A;
        destAttr2 = wroteA ? scratchAttr2B : scratchAttr2A;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if ((clipRect->flags & 0x08) != 0) {
        outputCount = ClipVertsUvsAttr012AgainstPlane(
            sourceVerts,
            sourceUvs,
            sourceAttr0,
            sourceAttr1,
            sourceAttr2,
            count,
            destVerts,
            destUvs,
            destAttr0,
            destAttr1,
            destAttr2,
            1,
            clipRect->yMaxAlt,
            false
        );
        sourceVerts = destVerts;
        sourceUvs = destUvs;
        sourceAttr0 = destAttr0;
        sourceAttr1 = destAttr1;
        sourceAttr2 = destAttr2;
        count = outputCount;
        clippedAnyPlane = true;
    }

    if (!clippedAnyPlane) {
        outputCount = 0;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (sourceVerts != g_Clip_PolyVerts) {
        memcpy(
            g_Clip_PolyVerts,
            sourceVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
        memcpy(
            g_Clip_PolyUvs,
            sourceUvs,
            (size_t)(outputCount) * sizeof(zClipUV)
        );
        memcpy(
            g_Clip_PolyAttr0,
            sourceAttr0,
            (size_t)(outputCount) * sizeof(float)
        );
        memcpy(
            g_Clip_PolyAttr2,
            sourceAttr2,
            (size_t)(outputCount) * sizeof(float)
        );
        memcpy(
            g_Clip_PolyAttr1,
            sourceAttr1,
            (size_t)(outputCount) * sizeof(float)
        );
    }
    return 1;
}
} // namespace

namespace {
/**
 * Original static helper observed in callers 0x476190 and 0x4761e0
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: cache the reciprocal distance-fog range when the range is
 * nonzero.
 */
void UpdateDistanceInvRange(float range) {
    if (range != 0.0f) {
        gModel_FogDistanceInvRange = 1.0f / range;
    }
}

/**
 * Original static helper observed in callers 0x476220 and 0x476260
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: cache the reciprocal height-fog range when the range is nonzero.
 */
void UpdateHeightInvRange(float range) {
    if (range != 0.0f) {
        gModel_FogHeightInvRange = 1.0f / range;
    }
}
} // namespace

/**
 * Recovered helper: zVideo_SubtractVec3.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this zVec3 subtraction pattern for near, camera, and far
 * frustum-center deltas.
 * Purpose: subtract one zVec3 from another and return the delta.
 */
static zVec3 zVideo_SubtractVec3(
    zVec3 *lhs,
    zVec3 *rhs
) {
    zVec3 delta;
    delta.x = lhs->x - rhs->x;
    delta.y = lhs->y - rhs->y;
    delta.z = lhs->z - rhs->z;
    return delta;
}

/**
 * Recovered helper: zVideo_DotVec3.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this x/y/z multiply-add dot-product pattern for every
 * frustum plane comparison.
 * Purpose: compute the dot product of two zVec3 values.
 */
static float zVideo_DotVec3(
    zVec3 *lhs,
    zVec3 *rhs
) {
    return lhs->x * rhs->x + lhs->y * rhs->y + lhs->z * rhs->z;
}

/**
 * Recovered helper: zVideo_TestSpherePlane.
 * Original-source helper evidence: no standalone retail function is present;
 * 0x478c70 inlines this sphere/plane reject-or-clip test for the side and far
 * frustum planes.
 * Purpose: test one sphere against one frustum plane and update the clip mask.
 */
static int zVideo_TestSpherePlane(
    zVec3 *delta,
    zVec3 *normal,
    float radius,
    int planeBit,
    int *clipMaskInOut
) {
    const float dot = zVideo_DotVec3(
        delta,
        normal
    );
    if (-radius >= dot) {
        return planeBit;
    }

    if (dot < radius) {
        *clipMaskInOut |= planeBit;
    }

    return 0;
}

/**
 * Reimplements 0x475c40: zModel_Display_Init
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: initialize zModel display globals, fog defaults, scratch buffers, and damage-mask state.
 */
int zModel_Display_Init() {
    gModel_DisplayInitWriteOnlyFlag = 1;

    gModel_RenderMode = 2;
    g_zModel_DisplayClipMode = 2;
    g_zModel_SpanOcclusionProc = (void *)(&zModel::RenderNodeSoftware);
    gModel_RenderFn = zModel::RenderNodeSoftware;
    gAltClipPassEnabled = 0;
    gModel_ClipMaskStackTop = gModel_ClipMaskStack;
    g_zModel_DisplayClipX = 0;
    g_zModel_DisplayClipY = 0;
    g_zModel_DisplayClipWidth = 320.0f;
    g_zModel_DisplayClipHeight = 200.0f;
    g_zModel_DisplayClipMaxX = 319.0f;
    g_zModel_DisplayClipMaxY = 199.0f;
    gModel_SmallPolyRejectArea2x = 4.0f;
    gModel_SmallPolyRejectArea20x = 40.0f;
    g_zModel_DisplayClipReserved = 0;
    gModel_DisplayClearedWriteOnlyFlag = 0;

    gModel_FogEnabled = 1;
    gModel_FogLinearModeEnabled = 1;
    gModel_FogDistanceStart = 500.0f;
    gModel_FogDistanceEnd = 700.0f;
    gModel_FogDistanceInvRange = 0.005f;
    gModel_FogHeightHigh = 300.0f;
    gModel_FogHeightLow = 200.0f;
    gModel_FogHeightInvRange = 0.01f;
    gModel_FogDensity = 2.0f;
    gModel_RenderVertexAlphaEnabled = 0;
    gModel_RenderAlphaScaleCurrent = 1.0f;

    g_zModel_FogStart = 500.0f;
    g_zModel_FogEnd = 700.0f;
    g_zModel_FogHeightHigh = 300.0f;
    g_zModel_FogHeightLow = 200.0f;
    g_zModel_FogDistanceInvRange = 0.005f;
    g_zModel_FogHeightInvRange = 0.01f;
    g_zModel_FogDensity = 2.0f;
    g_zModel_FogReserved = 0;
    g_zModel_FogScale = 1.0f;

    if (g_zVideo_ActiveRendererPath != 0) {
        g_zRndr_InverseZTolerance = 0.02f;
        g_zVideo_InverseZTolerancePending = 0.02f;
    } else {
        g_zRndr_InverseZTolerance = 0.01f;
    }

    g_zModel_TransformedVerts = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_PointInPolygonVertices = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_TransformedNormals = g_zModel_SharedVec3ScratchBStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;
    g_zModel_PointInPolygonEdgeNormals = g_zModel_SharedVec3ScratchBStorage;
    g_zModel_PointInPolygonVertexCount = 0;
    {
        for (int counterIndex = 0; counterIndex < 8; ++counterIndex) {
            g_zModel_ScratchCounters[counterIndex] = 0;
        }
    }
    g_zModel_PointInPolyTolX = 0.2f;
    g_zModel_PointInPolyTolY = 0.2f;
    g_Clip_PolyUvs = g_Clip_PolyUvsStorage;

    g_zModel_DamageMaskCurrent = g_zModel_DamageMaskStorage;
    g_OptCatalogDamageMaskSlotIndex = 0;
    g_zModel_OptCatalogAux0 = 0;
    g_zModel_OptCatalogAux1 = 0;
    gModel_DefaultGraphicsFlags = -1;

    zOptionEntryPartial *graphicsFlagsOption =
        zGame::Options_FindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    gModel_pGraphicsFlags =
        graphicsFlagsOption != 0 ? &graphicsFlagsOption->payloadOrBuffer : &gModel_DefaultGraphicsFlags;

    zTag4::Clear(&g_Variant_CurrentTag);
    return 0;
}

namespace zModel_Display {
/**
 * Reimplements 0x475e60: zModel_Display::ShutdownThunk
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: registration thunk that invokes zModel_Display::Shutdown.
 */
int ShutdownThunk() {
    Shutdown();
    return 0;
}
} // namespace zModel_Display

namespace zModel {
/**
 * Reimplements 0x475e70: zModel::Init
 * (D:\Proj\GameZRecoil\zModel\gmod_init.c).
 * Purpose: initialize zModel material and display-instance pools and choose the render path.
 */
int Init() {
    zModel_Matl::InitGlobals();

    if (g_zVideo_ActiveRendererPath != 0) {
        gModel_RenderFn = zModel::RenderNodeHardware;
        g_zModel_SoftwarePathActive = 0;
    } else {
        g_zModel_SoftwarePathActive = 1;
    }

    gModel_ClipMaskStackTop = gModel_ClipMaskStack;

    int capacity = g_zModel_DiPoolCapacity;
    if (capacity == 0) {
        capacity = 1750;
        g_zModel_DiPoolCapacity = capacity;
    }

    const size_t poolBytes = (size_t)(capacity) * sizeof(zDiPartial);
    g_zModel_DiPoolBase = (zDiPartial *)(malloc(poolBytes));
    memset(
        g_zModel_DiPoolBase,
        0,
        poolBytes
    );
    g_zModel_DiPoolFreeHeadIndex = 0;
    for (int i = 0; i < capacity - 1; ++i) {
        g_zModel_DiPoolBase[i].nextFreeIndex = i + 1;
    }
    if (capacity > 0) {
        g_zModel_DiPoolBase[capacity - 1].nextFreeIndex = -1;
    }
    g_zModel_DiPoolInUseCount = 0;
    return 0;
}
} // namespace zModel

namespace zModel_Display {
/**
 * Reimplements 0x475f60: zModel_Display::Reset
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: free all currently in-use display-instance pool entries.
 */
int Reset() {
    if (g_zModel_DiPoolCapacity > 0) {
        for (int i = 0; i < g_zModel_DiPoolInUseCount; ++i) {
            zModel_DiPool::FreeIfUnreferenced(&g_zModel_DiPoolBase[i]);
        }
    }

    return 0;
}
} // namespace zModel_Display

namespace zModel_Display {
/**
 * Reimplements 0x475fa0: zModel_Display::Shutdown
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: shut down display materials and release the display-instance pool.
 */
int Shutdown() {
    zModel_MatlBuffer::Shutdown();
    if (g_zModel_DiPoolCapacity > 0) {
        Reset();
        free(g_zModel_DiPoolBase);
        g_zModel_DiPoolBase = 0;
    }

    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    return 0;
}
} // namespace zModel_Display

namespace zModel {
/**
 * Reimplements 0x475ff0: zModel::SetDisplayInstancePoolCapacity
 * (D:\Proj\GameZRecoil\zModel\gmod_init.c).
 * Purpose: set the display-instance pool capacity before zModel initialization.
 */
void __fastcall SetDisplayInstancePoolCapacity(
    int capacity
) {
    if (g_zModel_DiPoolCapacity != 0) {
        zError::ReportOld(
            0x200,
            g_zModel_SourceFile_GmodInitC,
            0x1be,
            g_zModel_SetModel3dArraySizeAlreadySetFmt,
            g_zModel_DiPoolCapacity
        );
        return;
    }

    g_zModel_DiPoolCapacity = capacity;
}
} // namespace zModel

namespace zModel {
/**
 * Reimplements 0x476020: zModel::SetSoftwarePathActive
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: update the software render path flag when no hardware renderer is active.
 */
void __fastcall SetSoftwarePathActive(
    int active
) {
    if (g_zVideo_ActiveRendererPath == 0) {
        g_zModel_SoftwarePathActive = active;
    }
}
} // namespace zModel

namespace zModel {
/**
 * Reimplements 0x476030: zModel::SetVertexShadingEnabled
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: set the global vertex-shading enable flag.
 */
void __fastcall SetVertexShadingEnabled(
    int enabled
) {
    g_zModel_VertexShadingEnabled = enabled;
}
} // namespace zModel

/**
 * Reimplements 0x476040: zModel_FogTargetColorOverride_SetCurrent
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: optionally copy a fog-target override color and always store its
 * blend weight.
 */
void __fastcall zModel_FogTargetColorOverride_SetCurrent(
    zColorRgb *colorRgb01,
    float weight
) {
    if (colorRgb01 != 0) {
        g_zModel_FogTargetColorOverride.colorRgb01 = *colorRgb01;
    }
    g_zModel_FogTargetColorOverride.weight = weight;
}

/**
 * Reimplements 0x476070: zModel_RenderAlphaScale_SetCurrent
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the current render alpha-scale value.
 */
void __stdcall zModel_RenderAlphaScale_SetCurrent(
    float scale
) {
    gModel_RenderAlphaScaleCurrent = scale;
}

/**
 * Reimplements 0x476080: zModel_RenderVertexAlphaEnabled_SetCurrent
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the current vertex-alpha enabled flag.
 */
void __fastcall zModel_RenderVertexAlphaEnabled_SetCurrent(
    int enabled
) {
    gModel_RenderVertexAlphaEnabled = enabled;
}

namespace zModel {
/**
 * Reimplements 0x476090: zModel::SetTextureWorldPerMeter
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: set global texture-world scale per meter.
 */
void __stdcall SetTextureWorldPerMeter(
    float worldPerMeterU,
    float worldPerMeterV
) {
    g_zModel_TextureWorldPerMeterU = worldPerMeterU;
    g_zModel_TextureWorldPerMeterV = worldPerMeterV;
}
} // namespace zModel

namespace zModel {
/**
 * Reimplements 0x4760b0: zModel::SetTextureWorldBase
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: set global texture-world base coordinates.
 */
void __stdcall SetTextureWorldBase(
    float worldBaseU,
    float worldBaseV
) {
    g_zModel_TextureWorldBaseU = worldBaseU;
    g_zModel_TextureWorldBaseV = worldBaseV;
}
} // namespace zModel

namespace zModel {
/**
 * Reimplements 0x4760d0: zModel::SetDiTextureWorldPerMeter
 * (D:\Proj\GameZRecoil\zModel\gmod_init.c).
 * Purpose: set display-instance texture-world mapping flags and scale.
 */
int __fastcall SetDiTextureWorldPerMeter(
    zDiPartial *di,
    int worldSpaceEnabled,
    float textureWorldPerMeter,
    int textureWorldAxis
) {
    if (di == 0) {
        zError::ReportOld(
            0x200,
            g_zModel_SourceFile_GmodInitC,
            0x285,
            g_zModel_TextureScrollNullPtrErrorMsg
        );
        return 1;
    }

    di->flags = (di->flags & ~0x20) | ((worldSpaceEnabled & 1) << 5);
    di->textureWorldPerMeter = textureWorldPerMeter;
    di->textureWorldAxis = textureWorldAxis;
    return 0;
}
} // namespace zModel

namespace zClipAlt {
/**
 * Reimplements 0x476120: zClipAlt::SetSourceRect.
 *
 * Purpose: cache the source rectangle extents used to remap alternate clipped
 * points into the active target rectangle.
 */
void __fastcall SetSourceRect(
    const zClipAltFloatRect *rect
) {
    g_zClipAlt_SourceLeft = rect->left;
    g_zClipAlt_SourceTop = rect->top;
    g_zClipAlt_SourceRight = rect->right;
    g_zClipAlt_SourceBottom = rect->bottom;
    g_zClipAlt_SourceWidth = rect->right - rect->left;
    gAltClipSourceRectValid = 1;
    g_zClipAlt_SourceHeight = rect->bottom - rect->top;
}
} // namespace zClipAlt

/**
 * Reimplements 0x476170: zModel_Fog_SetEnabled
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the current fog-enabled flag.
 */
void __fastcall zModel_Fog_SetEnabled(
    int enabled
) {
    gModel_FogEnabled = enabled;
}

/**
 * Reimplements 0x476180: zModel_Fog_IsEnabled
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: return the current fog-enabled flag.
 */
int zModel_Fog_IsEnabled() {
    return gModel_FogEnabled;
}

/**
 * Reimplements 0x476190: zModel_Fog_SetDistanceStart
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the distance-fog start value and refresh the cached inverse
 * range against the current end value.
 */
void __stdcall zModel_Fog_SetDistanceStart(
    float distanceStart
) {
    const float range = gModel_FogDistanceEnd - distanceStart;
    gModel_FogDistanceStart = distanceStart;
    UpdateDistanceInvRange(range);
}

/**
 * Reimplements 0x4761d0: zModel_Fog_GetDistanceStart
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: return the current distance-fog start value.
 */
float zModel_Fog_GetDistanceStart() {
    return gModel_FogDistanceStart;
}

/**
 * Reimplements 0x4761e0: zModel_Fog_SetDistanceEnd
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the distance-fog end value and refresh the cached inverse
 * range against the current start value.
 */
void __stdcall zModel_Fog_SetDistanceEnd(
    float distanceEnd
) {
    const float range = distanceEnd - gModel_FogDistanceStart;
    gModel_FogDistanceEnd = distanceEnd;
    UpdateDistanceInvRange(range);
}

/**
 * Reimplements 0x476220: zModel_Fog_SetHeightHigh
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the high height-fog bound and refresh the cached inverse
 * vertical range.
 */
void __stdcall zModel_Fog_SetHeightHigh(
    float heightHigh
) {
    const float range = heightHigh - gModel_FogHeightLow;
    gModel_FogHeightHigh = heightHigh;
    UpdateHeightInvRange(range);
}

/**
 * Reimplements 0x476260: zModel_Fog_SetHeightLow
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the low height-fog bound and refresh the cached inverse
 * vertical range.
 */
void __stdcall zModel_Fog_SetHeightLow(
    float heightLow
) {
    const float range = gModel_FogHeightHigh - heightLow;
    gModel_FogHeightLow = heightLow;
    UpdateHeightInvRange(range);
}

/**
 * Reimplements 0x4762a0: zModel_Fog_SetDensity
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the current fog density scalar.
 */
void __stdcall zModel_Fog_SetDensity(
    float density
) {
    gModel_FogDensity = density;
}

/**
 * Reimplements 0x4762b0: zModel_Fog_SetLinearModeEnabled
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: store the linear fog mode enabled flag.
 */
void __fastcall zModel_Fog_SetLinearModeEnabled(
    int enabled
) {
    gModel_FogLinearModeEnabled = enabled;
}

/**
 * Reimplements 0x4762c0: zModel_Fog_SetColorRgb01
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: copy the fog RGB color and update hardware renderer fog color when
 * the active renderer path requires it.
 */
void __fastcall zModel_Fog_SetColorRgb01(
    zColorRgb *rgb01
) {
    memcpy(
        &gModel_FogColorRgb01,
        rgb01,
        sizeof(gModel_FogColorRgb01)
    );
    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo::SetFogColorFromRgb01((zVideo_ColorRgbFloat *)(rgb01));
    }
}

/**
 * Reimplements 0x4762f0: zModel_Fog_ApplyCurrentColor
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: apply the current fog color through the renderer's clamped RGB path.
 */
void zModel_Fog_ApplyCurrentColor() {
    zRndr::FogColor_SetRgb01Clamped(&gModel_FogColorRgb01);
}

namespace zRndr {
/**
 * Reimplements 0x476300: zRndr::SetInverseZTolerance
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: update the software inverse-Z tolerance and mirror it to the active renderer path.
 */
void __stdcall SetInverseZTolerance(
    float inverseZTolerance
) {
    g_zRndr_InverseZTolerance = inverseZTolerance;
    if (g_zVideo_ActiveRendererPath != 0) {
        g_zVideo_InverseZTolerancePending = inverseZTolerance;
    }
}
} // namespace zRndr

namespace zTag4 {
/**
 * Reimplements 0x476320: zTag4::Clear
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: reset a variant tag set to the empty sentinel state.
 */
void __fastcall Clear(
    zTag4Partial *tag
) {
    if (tag == 0) {
        return;
    }

    tag->count = 0;
    tag->tags[0] = 0xff;
    tag->tags[1] = 0xff;
    tag->tags[2] = 0xff;
}
} // namespace zTag4

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
} // namespace zDi

namespace VariantTag {
/**
 * Reimplements 0x476370: VariantTag::TagsOverlap
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
 * Purpose: test whether two variant tag sets pass the active filter.
 */
int __fastcall TagsOverlap(
    const zTag4Partial *tagA,
    const zTag4Partial *tagB
) {
    if (g_Variant_FilterEnabled == 0) {
        return 1;
    }

    const unsigned char countA = tagA->count;
    if (countA == 0 || tagB->count == 0) {
        return 1;
    }

    {
        for (int indexA = 0; indexA < countA; ++indexA) {
            const unsigned char tagIdA = tagA->tags[indexA];
            if (tagIdA == 0xff) {
                return 1;
            }

            {
                for (int indexB = 0; indexB < tagB->count; ++indexB) {
                    const unsigned char tagIdB = tagB->tags[indexB];
                    if (tagIdB == 0xff || tagIdA == tagIdB) {
                        return 1;
                    }
                }
            }
        }
    }

    return 0;
}
} // namespace VariantTag

namespace VariantTag {
/**
 * Reimplements 0x476400: VariantTag::CurrentAllowsId
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
 * Purpose: test whether one variant id is accepted by the active tag filter.
 */
int __fastcall CurrentAllowsId(
    int variantId
) {
    if (g_Variant_FilterEnabled == 0) {
        return 1;
    }

    if (variantId == 0xff) {
        return 1;
    }

    const unsigned char count = g_Variant_CurrentTag.count;
    if (count == 0) {
        return 1;
    }

    const unsigned char id = (unsigned char)(variantId);
    for (int i = 0; i < count; ++i) {
        const unsigned char tag = g_Variant_CurrentTag.tags[i];
        if (tag == 0xff || tag == id) {
            return 1;
        }
    }

    return 0;
}
} // namespace VariantTag

namespace zModel {
/**
 * Reimplements 0x476460: zModel::SetBackfaceEliminationToleranceScalar
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: store the global backface-elimination tolerance scalar.
 */
void __stdcall SetBackfaceEliminationToleranceScalar(
    float scalar
) {
    g_zModel_BFETolerance = scalar;
}
} // namespace zModel

namespace zModel {
/**
 * Reimplements 0x476470: zModel::GetBackfaceEliminationToleranceScalar
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: return the current global backface-elimination tolerance scalar.
 */
float GetBackfaceEliminationToleranceScalar() {
    return g_zModel_BFETolerance;
}
} // namespace zModel

namespace zMath {
/**
 * Reimplements 0x476480: zMath::ProjectPointAndClampToScreenClip.
 * Purpose: transforms one point through camera scratch B, projects it, and
 * clamps it to the active screen clip rectangle.
 */
int __fastcall ProjectPointAndClampToScreenClip(
    const zVec3 *srcPoint,
    zVec3 *dstPoint
) {
    zMat4x3 slotBuffer = {0};
    MatStackPushPtr((float *)(&slotBuffer));
    MatLoadCameraScratchB();

    if (*g_currentMatrixIdentityFlagSlot != 0) {
        *dstPoint = *srcPoint;
    } else {
        const zMat4x3 *const matrix = (const zMat4x3 *)(*g_currentMatrixPtrSlot);
        dstPoint->x = srcPoint->x * matrix->xx + srcPoint->y * matrix->yx +
                      srcPoint->z * matrix->zx + matrix->posX;
        dstPoint->z = srcPoint->x * matrix->xz + srcPoint->y * matrix->yz +
                      srcPoint->z * matrix->zz + matrix->posZ;
        dstPoint->y = srcPoint->x * matrix->xy + srcPoint->y * matrix->yy +
                      srcPoint->z * matrix->zy + matrix->posY;
    }

    MatStackPopPtr();

    if (dstPoint->z <= gClipRect_Primary.zMin) {
        int result = 8;
        if (-gClipRect_Primary.zMin <= dstPoint->z) {
            dstPoint->z = gClipRect_Primary.zMin;
        } else {
            dstPoint->z = -dstPoint->z;
        }

        ProjectPointBatch(
            dstPoint,
            (zProjectedPoint *)(dstPoint),
            1
        );
        if (dstPoint->x < -5000.0f) {
            dstPoint->x = -5000.0f;
        } else if (dstPoint->x > 5000.0f) {
            dstPoint->x = 5000.0f;
        }

        dstPoint->y = g_zVideo_ProjectClipBottom;
        dstPoint->x = (dstPoint->x + g_zVideo_ProjectClipLeft + 5000.0f) /
                      (10000.0f / (gClipRect_Primary.xMaxAlt - g_zVideo_ProjectClipLeft));
        return result;
    }

    ProjectPointBatch(
        dstPoint,
        (zProjectedPoint *)(dstPoint),
        1
    );

    int result = 0;
    if (dstPoint->x < g_zVideo_ProjectClipLeft) {
        dstPoint->x = g_zVideo_ProjectClipLeft;
        result = 1;
    } else if (dstPoint->x > g_zVideo_ProjectClipRight) {
        dstPoint->x = g_zVideo_ProjectClipRight;
        result = 2;
    }

    if (dstPoint->y < g_zVideo_ProjectClipTop) {
        dstPoint->y = g_zVideo_ProjectClipTop;
        return 4;
    }
    if (dstPoint->y >= g_zVideo_ProjectClipBottom) {
        dstPoint->y = g_zVideo_ProjectClipBottom - 1.0f;
        return 8;
    }

    return result;
}
} // namespace zMath

namespace zClipAlt {
/**
 * Reimplements 0x4766a0: zClipAlt::RemapPointXYInPlace
 * (D:\Proj\Battlesport\zClip.cpp).
 *
 * Purpose: reject a point outside the alternate clip rectangle or remap its XY
 * coordinates into source-rectangle space in place.
 */
int __fastcall RemapPointXYInPlace(
    float *point
) {
    g_Clip_PolyVerts[0].x = point[0];
    g_Clip_PolyVerts[0].y = point[1];
    if (zClipRect::TrivialRejectPolyXY(
        &gClipRect_Alt,
        1
    ) == 0) {
        return 0;
    }

    point[0] = g_zClipAlt_RemapScaleX * point[0] + g_zClipAlt_RemapBiasX;
    point[1] = g_zClipAlt_RemapScaleY * point[1] + g_zClipAlt_RemapBiasY;
    return 1;
}
} // namespace zClipAlt

namespace zScene {
/**
 * Reimplements 0x476700: zScene::TestProjectedSphereVisible
 * (GameZRecoil/zModel/gmod_scene.c).
 * Purpose: project a bounding sphere and test representative span-buffer columns for visibility.
 */
int __fastcall TestProjectedSphereVisible(
    zVec3 *center,
    float radius
) {
    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadCameraScratchB();

    zVec3 viewPoint = *center;
    zMath::MatTransformPointBatchInPlace(
        &viewPoint,
        1
    );
    zMath::MatStackPopPtr();

    const float depthMinusRadius = viewPoint.z - radius;
    if (depthMinusRadius <= 0.0000999999975f) {
        return 1;
    }

    zProjectedPoint projectedPoint = {0};
    zMath::ProjectPointBatch(
        &viewPoint,
        &projectedPoint,
        1
    );
    const zVec2 screenScale = zMath_Project_GetLastScreenScaleXY();
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

    const int centerY = TruncateToInt(projectedPoint.y);
    int columnMin = centerY - projectedRadius;
    if (gClipRect_Primary.yMax - 2.0f < (float)(columnMin)) {
        return 0;
    }

    int columnMax = centerY + projectedRadius;
    if ((float)(columnMax) <= gClipRect_Primary.yMin) {
        return 0;
    }

    const int clipXMin = TruncateToInt(gClipRect_Primary.xMin);
    if (clipXMin > zRndr::g_spanAllocCursor->sampleXMin) {
        zRndr::g_spanAllocCursor->sampleXMin = clipXMin;
    }

    const int savedSampleXMin = zRndr::g_spanAllocCursor->sampleXMin;
    const int clipXMax = TruncateToInt(gClipRect_Primary.xMax - 2.0f);
    if (clipXMax < zRndr::g_spanAllocCursor->sampleXMax) {
        zRndr::g_spanAllocCursor->sampleXMax = clipXMax;
    }

    zRndr::g_spanAllocCursor->invDepth = 1.0f / depthMinusRadius;
    zRndr::g_spanAllocCursor->invDepthStep = zRndr::g_spanAllocCursor->invDepth;
    zRndr::g_spanAllocCursor->depthSlope = 0.0f;

    const int clipYMin = TruncateToInt(gClipRect_Primary.yMin + 1.0f);
    if (clipYMin > columnMin) {
        columnMin = clipYMin;
    }

    if (TestSpanColumnVisible(columnMin)) {
        return 1;
    }

    const int clipYMax = TruncateToInt(gClipRect_Primary.yMax - 2.0f);
    if (clipYMax < columnMax) {
        columnMax = clipYMax;
    }

    zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
    if (TestSpanColumnVisible(columnMax)) {
        return 1;
    }

    const int columnDelta = columnMax - columnMin;
    if (columnDelta <= 1) {
        return 0;
    }

    int midColumn = (columnDelta >> 1) + columnMin;
    zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
    if (TestSpanColumnVisible(midColumn)) {
        return 1;
    }

    int columnIndex;
    for (columnIndex = midColumn - 8; columnIndex > columnMin; columnIndex -= 8) {
        zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
        if (TestSpanColumnVisible(columnIndex)) {
            return 1;
        }
    }

    for (columnIndex = midColumn + 8; columnIndex < columnMax; columnIndex += 8) {
        zRndr::g_spanAllocCursor->sampleXMin = savedSampleXMin;
        if (TestSpanColumnVisible(columnIndex)) {
            return 1;
        }
    }

    return 0;
}
} // namespace zScene

namespace zDi {
/**
 * Reimplements 0x476a50: zDi::EvalBoundingSphereLightingFlags
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: evaluate fog, active-light, and lens-flare visibility flags for a display instance.
 */
void __fastcall EvalBoundingSphereLightingFlags(
    zDiPartial *self,
    int *outDepthFade,
    int *outActiveLightState,
    int *outLensFlareVisible
) {
    zVec3 mappedPoint = self->bboxCenter;
    if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
        mappedPoint = TransformPointByCurrentMatrix(&self->bboxCenter);
    }

    if (gModel_FogEnabled != 0 && (self->flags & 2) != 0 &&
        zModel_Light::EvalSphereFogFade(
            &mappedPoint,
            self->bboxRadius
        ) >
            kVisibleContributionThreshold) {
        *outDepthFade = 1;
    } else {
        *outDepthFade = 0;
    }

    int activeLightContributionCount = 0;
    if (ModelGraphicsFlagBit0Enabled()) {
        if (gModel_HasActiveLights != 0 && (self->flags & 1) != 0) {
            activeLightContributionCount =
                zModel_Light::PointInPolygonTestRadiusXZ(
                    &mappedPoint,
                    self->bboxRadius
                );
            *outActiveLightState = activeLightContributionCount > 0 ? 1 : 0;
        } else {
            *outActiveLightState = 0;
        }

        if (g_zModel_FogTargetColorOverride.weight > kVisibleContributionThreshold) {
            ++activeLightContributionCount;
            *outActiveLightState = 1;
        }
    } else {
        *outActiveLightState = 0;
    }

    if (activeLightContributionCount <= 1) {
        *outLensFlareVisible = 0;
        return;
    }

    zColorRgb fogColorRgb01 = {0};
    float totalWeight = 0.0f;
    float maxWeight = 0.0f;

    for (int i = 0; i < gModel_ActiveLightCount; ++i) {
        zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[i];
        if (entry.contributesToLighting == 0) {
            continue;
        }

        zClass_LightDataPartial *light = entry.light;
        if (g_zModel_SoftwarePathActive != 0 && light->isPointMode != 0) {
            continue;
        }

        const float weight = g_Clip_PolyAttr0[i];
        fogColorRgb01.red += light->specularColor.red * weight;
        fogColorRgb01.green += light->specularColor.green * weight;
        fogColorRgb01.blue += light->specularColor.blue * weight;
        totalWeight += weight;
        if (maxWeight < weight) {
            maxWeight = weight;
        }
    }

    (void)maxWeight;

    if (g_zModel_FogTargetColorOverride.weight > kVisibleContributionThreshold) {
        fogColorRgb01.red += g_zModel_FogTargetColorOverride.colorRgb01.red;
        fogColorRgb01.green += g_zModel_FogTargetColorOverride.colorRgb01.green;
        fogColorRgb01.blue += g_zModel_FogTargetColorOverride.colorRgb01.blue;
        totalWeight += g_zModel_FogTargetColorOverride.weight;
    }

    const float invTotalWeight = 1.0f / totalWeight;
    fogColorRgb01.red *= invTotalWeight;
    fogColorRgb01.green *= invTotalWeight;
    fogColorRgb01.blue *= invTotalWeight;
    zRndr::SetFogTargetColorRgb01Clamped(&fogColorRgb01);
    *outLensFlareVisible = 1;
}
} // namespace zDi

namespace zModel {
/**
 * Reimplements 0x476cf0: zModel::RenderNodeSoftware
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: render a display-instance node through the software renderer path.
 */
void __fastcall RenderNodeSoftware(
    zClass_NodePartial *node,
    int clipMask
) {
    zDiPartial *const di = NodeDisplayInstance(node);
    if (di == 0) {
        return;
    }

    zMat4x3 matrixScratch = {0};
    zMath::MatStackPushPtr((float *)(&matrixScratch));
    if (di->mode == 1) {
        if ((di->flags & 0x10) != 0) {
            zMath_Mat_LoadView();
        } else if (g_zVideo_pActiveViewContext != 0) {
            zMath_Mat_LoadProjection(g_zVideo_pActiveViewContext->frustumYaw);
        }
        zRndr::g_perspectiveTextureEnabled = 0;
    } else {
        zMath_Mat_SetupCamera();
        zRndr::g_perspectiveTextureEnabled = di->mode == 0 ? 1 : 0;
    }

    PrepareTransformedVertices(di);

    if ((di->flags & 8) != 0 && di->entries != 0 && di->entryCount > 0) {
        const unsigned int pointColor = di->entries[0].material != 0
                                            ? di->entries[0].material->packedColor
                                            : 0;
        for (int vertexIndex = 0; vertexIndex < di->vertCount; ++vertexIndex) {
            zVec3 *const transformed = &g_zModel_TransformedVerts[vertexIndex];
            if (transformed->z <= gClipRect_Primary.zMin) {
                continue;
            }

            zProjectedPoint projectedPoint = {0};
            if (g_zVideo_ActiveRendererPath == 0) {
                zMath::ProjectPointBatch(
                    transformed,
                    &projectedPoint,
                    1
                );
            } else {
                zMath_ProjectSphereBatch(
                    transformed,
                    (zProjectedSphere *)(&projectedPoint),
                    1
                );
            }

            if (!ProjectedPointInClipBounds(projectedPoint)) {
                continue;
            }

            if (g_zVideo_ActiveRendererPath == 0) {
                zRndr_LensFlare_QueueProjectedSample(
                    &projectedPoint,
                    (int)(pointColor & 0xffff),
                    0
                );
            } else {
                zVideo_DrawPointColor16Proc drawPoint = g_zVideo_pfnDrawPointColor16;
                drawPoint(
                    (zVideo_XyzVertex *)(&projectedPoint),
                    pointColor & 0xffff,
                    1
                );
            }
        }
    }

    if (di->entryCount <= 0 || di->entries == 0) {
        zMath::MatStackPopPtr();
        zRndr::g_perspectiveTextureEnabled = 0;
        return;
    }

    int outDepthFade = 0;
    int outActiveLightState = 0;
    int outLensFlareVisible = 0;
    zDi::EvalBoundingSphereLightingFlags(
        di,
        &outDepthFade,
        &outActiveLightState,
        &outLensFlareVisible
    );

    gClipRect_Primary.flags = clipMask;
    for (int entryIndex = 0; entryIndex < di->entryCount; ++entryIndex) {
        zDiEntryPartial *const entry = &di->entries[entryIndex];
        zModel_MaterialPartial *const material = entry->material;
        int vertexCount = (int)(entry->flagsAndIndexCount & 0xff);
        if (material == 0 || vertexCount < 3 || vertexCount > 0x40 ||
            CopyEntryVerticesToScratch(
                di,
                entry,
                vertexCount
            ) == 0) {
            continue;
        }

        zRndr_SetPaletteRemapKeyFromRgb01(
            0,
            0.0f
        );
        zRndr_SetPaletteRemapKey(
            0,
            0.0f
        );
        zRndr_SetPaletteShadeRecipeIndex(0);

        zVec3 surfaceNormal = {0};
        int scanConvertMode = 1;
        if (ComputeSurfaceNormalAndCull(
            vertexCount,
            (entry->flagsAndIndexCount & 0x0100) != 0,
            &surfaceNormal,
            &scanConvertMode
        ) == 0) {
            continue;
        }

        const int isTextured = (material->flags & 0x0100) != 0;
        int hasPerVertexShade = 0;
        int preservePaletteRemapKey = 0;
        int packedColor = material->packedColor;
        if (isTextured != 0) {
            if ((material->flags & 0x0400) != 0) {
                zModel_Material::UpdateCycleIfNeeded(material);
            }

            CopyEntryUvsToScratch(
                entry,
                vertexCount
            );
            for (int i = 0; i < vertexCount; ++i) {
                g_Clip_PolyAttr0[i] = 0.0f;
            }

            int lightingMode = 0;
            if (outDepthFade != 0 &&
                zModel_Light::BuildAttr0DepthFade(
                    vertexCount,
                    &preservePaletteRemapKey
                ) != 0) {
                hasPerVertexShade = 1;
            }

            if (outActiveLightState != 0) {
                int lightFlags = 0;
                int usePaletteRemap = 0;
                if (material->currentTextureDirectoryEntry != 0 &&
                    material->currentTextureDirectoryEntry->image != 0 &&
                    material->currentTextureDirectoryEntry->image->palette != 0) {
                    usePaletteRemap = 1;
                }
                if (zModel_Light::SetActiveLights(
                    &surfaceNormal,
                    vertexCount,
                    &lightFlags,
                    &lightingMode,
                    usePaletteRemap
                ) != 0) {
                    hasPerVertexShade = 1;
                }
                preservePaletteRemapKey |= lightingMode;
                if (lightFlags == 1) {
                    zRndr::CommitFogColorParamsIfChanged();
                }
            }
        } else if (outDepthFade != 0) {
            float outFade = 0.0f;
            if (zModel_Light::EvalBatchSphereFade(&outFade) != 0) {
                hasPerVertexShade = 1;
            }
            if (outActiveLightState != 0 &&
                zModel_Light_BuildLightWeights(
                    &surfaceNormal,
                    vertexCount,
                    &packedColor,
                    outFade
                ) != 0) {
                hasPerVertexShade = 2;
            }
            if (hasPerVertexShade == 1) {
                zRndr::CommitFogColorParamsIfChanged();
                float scale255 = 0.0f;
                zFloat::Set255f(&scale255);
                scale255 -= 1.0f;
                zRndr::BlendPackedColor565WithFogInPlace(
                    &packedColor,
                    (int)(outFade * scale255)
                );
            }
        } else if (outActiveLightState != 0 &&
                   zModel_Light_BuildLightWeights(
                       &surfaceNormal,
                       vertexCount,
                       &packedColor,
                       0.0f
                   ) != 0) {
            hasPerVertexShade = 2;
        }

        int clippedCount = vertexCount;
        if (isTextured != 0) {
            if (ClipAndProjectSoftwareTextured(
                &gClipRect_Primary,
                &clippedCount,
                hasPerVertexShade
            ) == 0 ||
                RejectProjectedSmallPoly(clippedCount) != 0) {
                continue;
            }

            zVec3 triClipVerts[3];
            CopyProjectedTriVerts(triClipVerts);
            ApplySoftwareDepthScale(entry->drawFlags);
            zRndr::g_scanConvertMode = scanConvertMode;
            if (hasPerVertexShade == 2) {
                zRndr_SubmitTexturedPolyPerVertexAlphaOrShade(
                    (zVec3 *)g_Clip_PolyVerts,
                    (zVec3 *)g_Clip_PolyVertsScratch,
                    triClipVerts,
                    (zVec2 *)g_Clip_PolyUvs,
                    g_Clip_PolyAttr0,
                    0,
                    clippedCount,
                    material->currentTextureDirectoryEntry,
                    preservePaletteRemapKey,
                    gModel_RenderVertexAlphaEnabled
                );
            } else {
                zRndr_SubmitTexturedPolyUniformAlphaOrShade(
                    (zVec3 *)g_Clip_PolyVerts,
                    (zVec3 *)g_Clip_PolyVertsScratch,
                    triClipVerts,
                    (zVec2 *)g_Clip_PolyUvs,
                    clippedCount,
                    material->currentTextureDirectoryEntry,
                    gModel_RenderAlphaScaleCurrent,
                    gModel_RenderVertexAlphaEnabled
                );
            }

            if (gAltClipPassEnabled != 0) {
                clippedCount = vertexCount;
                CopyEntryVerticesToScratch(
                    di,
                    entry,
                    clippedCount
                );
                CopyEntryUvsToScratch(
                    entry,
                    clippedCount
                );
                if (ClipAndProjectSoftwareTextured(
                    &gClipRect_Alt,
                    &clippedCount,
                    hasPerVertexShade
                ) != 0) {
                    RemapAltProjectedVerts(
                        (zVec3 *)triClipVerts,
                        3
                    );
                    RemapAltProjectedVerts(
                        (zVec3 *)g_Clip_PolyVerts,
                        clippedCount
                    );
                    zRndr::g_inverseDepthBias = gClipRect_Primary.zMin;
                    if (hasPerVertexShade == 2) {
                        zRndr_SubmitTexturedPolyPerVertexAlphaOrShade(
                            (zVec3 *)g_Clip_PolyVerts,
                            0,
                            triClipVerts,
                            (zVec2 *)g_Clip_PolyUvs,
                            g_Clip_PolyAttr0,
                            0,
                            clippedCount,
                            material->currentTextureDirectoryEntry,
                            preservePaletteRemapKey,
                            gModel_RenderVertexAlphaEnabled
                        );
                    } else {
                        zRndr_SubmitTexturedPolyUniformAlphaOrShade(
                            (zVec3 *)g_Clip_PolyVerts,
                            0,
                            triClipVerts,
                            (zVec2 *)g_Clip_PolyUvs,
                            clippedCount,
                            material->currentTextureDirectoryEntry,
                            gModel_RenderAlphaScaleCurrent,
                            gModel_RenderVertexAlphaEnabled
                        );
                    }
                }
            }
        } else {
            if ((clipMask & 0x30) != 0 &&
                zClipRect::ClipPolyZRange_NoUV(
                    &gClipRect_Primary,
                    &clippedCount
                ) == 0) {
                continue;
            }

            ProjectScratchToClipVerts(clippedCount);
            if (RejectProjectedSmallPoly(clippedCount) != 0) {
                continue;
            }

            zVec3 triClipVerts[3];
            CopyProjectedTriVerts(triClipVerts);
            if ((clipMask & 0x0f) != 0 &&
                zClipRect::ClipPoly_NoUV(
                    &gClipRect_Primary,
                    &clippedCount
                ) == 0) {
                continue;
            }

            ApplySoftwareDepthScale(entry->drawFlags);
            zRndr::g_scanConvertMode = scanConvertMode;
            zRndr_SubmitPolyWithSpanList(
                (zVec3 *)g_Clip_PolyVerts,
                triClipVerts,
                packedColor,
                MaterialAlphaInt(material),
                clippedCount,
                gModel_RenderVertexAlphaEnabled
            );

            if (gAltClipPassEnabled != 0) {
                clippedCount = vertexCount;
                CopyEntryVerticesToScratch(
                    di,
                    entry,
                    clippedCount
                );
                if (zClipRect::TrivialRejectPolyXY(
                    &gClipRect_Alt,
                    clippedCount
                ) != 0 &&
                    zClipRect::ClipPoly_NoUV(
                        &gClipRect_Alt,
                        &clippedCount
                    ) != 0) {
                    RemapAltProjectedVerts(
                        triClipVerts,
                        3
                    );
                    RemapAltProjectedVerts(
                        (zVec3 *)g_Clip_PolyVerts,
                        clippedCount
                    );
                    zRndr::g_inverseDepthBias = gClipRect_Primary.zMin;
                    zRndr_SubmitPolyWithSpanList(
                        (zVec3 *)g_Clip_PolyVerts,
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
    zRndr::g_perspectiveTextureEnabled = 0;
}
} // namespace zModel

namespace zModel {
/**
 * Reimplements 0x477b30: zModel::RenderNodeHardware
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: render a display-instance node through the hardware renderer path.
 */
void __fastcall RenderNodeHardware(
    zClass_NodePartial *node,
    int clipMask
) {
    zDiPartial *const di = NodeDisplayInstance(node);
    if (di == 0) {
        return;
    }

    zMat4x3 matrixScratch = {0};
    zMath::MatStackPushPtr((float *)(&matrixScratch));
    if (di->mode == 1) {
        if ((di->flags & 0x10) != 0) {
            zMath_Mat_LoadView();
        } else if (g_zVideo_pActiveViewContext != 0) {
            zMath_Mat_LoadProjection(g_zVideo_pActiveViewContext->frustumYaw);
        }
    } else {
        zMath_Mat_SetupCamera();
    }

    PrepareTransformedVertices(di);
    PrepareTransformedNormals(di);

    if ((di->flags & 8) != 0 && di->pointEntries != 0) {
        for (int pointIndex = 0; pointIndex < di->pointCount; ++pointIndex) {
            zModel_PointEntryPartial *const pointEntry = &di->pointEntries[pointIndex];
            if (pointEntry->pointCamList != 0 && pointEntry->pointCamCount > 0) {
                zModel_RenderPointQueueEntry(
                    &pointEntry->pointCamList[0],
                    pointEntry->packedColor16,
                    pointEntry
                );
            }
        }
    }

    gClipRect_Primary.flags = clipMask;
    for (int entryIndex = 0; entryIndex < di->entryCount; ++entryIndex) {
        zDiEntryPartial *const entry = &di->entries[entryIndex];
        zModel_MaterialPartial *const material = entry->material;
        int vertexCount = (int)(entry->flagsAndIndexCount & 0xff);
        if (material == 0 || vertexCount < 3 || vertexCount > 0x40 ||
            CopyEntryVerticesToScratch(
                di,
                entry,
                vertexCount
            ) == 0) {
            continue;
        }

        zVec3 surfaceNormal = {0};
        if (ComputeSurfaceNormalAndCull(
            vertexCount,
            (entry->flagsAndIndexCount & 0x0100) != 0,
            &surfaceNormal,
            0
        ) == 0) {
            continue;
        }

        CopyEntryNormalsToCurrent(
            di,
            entry,
            vertexCount
        );
        ClearPolyAttributes(vertexCount);
        const int hasAttributes = BuildPolyAttributes(
            &surfaceNormal,
            vertexCount
        );

        if ((material->flags & 0x0100) == 0) {
            int clippedCount = vertexCount;
            if (ClipAndProjectNoUv(
                &gClipRect_Primary,
                &clippedCount,
                hasAttributes
            ) == 0) {
                continue;
            }

            ApplyDepthBiasToProjectedVerts(
                entry->drawFlags,
                clippedCount
            );
            if (hasAttributes != 0) {
                SubmitPolyColorAttrProc submit = g_zVideo_pfnSubmitPolyColorAttr;
                submit(
                    (zVideo_XyzVertex *)g_Clip_PolyVerts,
                    material->packedColor,
                    (zVideo_ColorRgbFloat *)(&material->colorRgb),
                    g_Clip_PolyAttr1,
                    g_Clip_PolyAttr0,
                    g_Clip_PolyAttr2,
                    MaterialAlphaInt(material),
                    clippedCount,
                    entry->drawFlags,
                    gModel_RenderVertexAlphaEnabled
                );
            } else {
                SubmitPolyFlatColor16Proc submit = g_zVideo_pfnSubmitPolyFlatColor16;
                submit(
                    (zVideo_XyzVertex *)g_Clip_PolyVerts,
                    material->packedColor,
                    MaterialAlphaInt(material),
                    entry->drawFlags,
                    clippedCount,
                    gModel_RenderVertexAlphaEnabled
                );
            }

            if (gAltClipPassEnabled != 0) {
                clippedCount = vertexCount;
                CopyEntryVerticesToScratch(
                    di,
                    entry,
                    clippedCount
                );
                if (ClipAndProjectNoUv(
                    &gClipRect_Alt,
                    &clippedCount,
                    hasAttributes
                ) != 0) {
                    for (int i = 0; i < clippedCount; ++i) {
                        zClipAlt::RemapPointXYInPlace(&g_Clip_PolyVerts[i].x);
                    }
                }
            }
            continue;
        }

        if ((material->flags & 0x0400) != 0) {
            zModel_Material::UpdateCycleIfNeeded(material);
        }

        CopyEntryUvsToScratch(
            entry,
            vertexCount
        );
        int clippedCount = vertexCount;
        if (ClipAndProjectHardwareUv(
            &gClipRect_Primary,
            &clippedCount,
            hasAttributes
        ) == 0) {
            continue;
        }

        zClipUV perspectiveUvs[0x40] = {0};
        FillPerspectiveUvsForHardwareSubmit(
            perspectiveUvs,
            clippedCount
        );
        ApplyDepthBiasToProjectedVerts(
            entry->drawFlags,
            clippedCount
        );
        zVideo_RenderClass *const renderClass = MaterialRenderClass(material);
        if (hasAttributes != 0 || g_zModel_CurrentPolyNormals != 0) {
            SubmitPolygonProc submit = g_zVideo_pfnSubmitPolygon;
            submit(
                (zVideo_XyzVertex *)g_Clip_PolyVerts,
                (zVideo_TexCoord *)perspectiveUvs,
                g_Clip_PolyAttr1,
                g_Clip_PolyAttr0,
                g_Clip_PolyAttr2,
                clippedCount,
                renderClass,
                entry->drawFlags,
                MaterialAlphaFloat(material),
                gModel_RenderVertexAlphaEnabled
            );
        } else {
            SubmitPolyRenderClassProc submit = g_zVideo_pfnSubmitPolyRenderClass;
            submit(
                (zVideo_XyzVertex *)g_Clip_PolyVerts,
                (zVideo_TexCoord *)perspectiveUvs,
                clippedCount,
                renderClass,
                entry->drawFlags,
                MaterialAlphaFloat(material),
                gModel_RenderVertexAlphaEnabled
            );
        }

        if (gAltClipPassEnabled != 0) {
            clippedCount = vertexCount;
            CopyEntryVerticesToScratch(
                di,
                entry,
                clippedCount
            );
            CopyEntryUvsToScratch(
                entry,
                clippedCount
            );
            if (ClipAndProjectUv(
                &gClipRect_Alt,
                &clippedCount,
                hasAttributes
            ) != 0) {
                for (int i_265 = 0; i_265 < clippedCount; ++i_265) {
                    zClipAlt::RemapPointXYInPlace(&g_Clip_PolyVerts[i_265].x);
                }
            }
        }
    }

    zMath::MatStackPopPtr();
}
} // namespace zModel

/**
 * Reimplements 0x478c70: zVideo_FrustumTestSphereClipMask.
 * Provisional source-placement hypothesis: GameZRecoil/zModel/zModel_Display.cpp.
 * Purpose: reject or clip a sphere against the active view frustum planes.
 *
 * Evidence: BN reads the projection view-context global at 0x576214, clears
 * the incoming clip mask, tests near and far centers separately, and tests
 * side planes against camera-position deltas while accumulating clip bits.
 */
int __fastcall zVideo_FrustumTestSphereClipMask(
    zVec3 *sphereCenter,
    int *clipMaskInOut,
    float radius
) {
    const int oldMask = *clipMaskInOut;
    *clipMaskInOut = 0;

    zClass_CameraDataPartial *viewContext = g_zVideo_pActiveProjectionViewContext;
    zVec3 delta;
    if ((oldMask & 0x10) != 0) {
        delta = zVideo_SubtractVec3(
            sphereCenter,
            &viewContext->nearClipCenter
        );
        const float dot = zVideo_DotVec3(
            &delta,
            &viewContext->worldFrustumNormals[4]
        );
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
    delta = zVideo_SubtractVec3(
        sphereCenter,
        &viewContext->cameraPos
    );

    if ((oldMask & 1) != 0) {
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[0],
            radius,
            1,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 2) != 0) {
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[1],
            radius,
            2,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 4) != 0) {
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[2],
            radius,
            4,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 8) != 0) {
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[3],
            radius,
            8,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    if ((oldMask & 0x20) != 0) {
        viewContext = g_zVideo_pActiveProjectionViewContext;
        delta = zVideo_SubtractVec3(
            sphereCenter,
            &viewContext->farClipCenter
        );
        const int result = zVideo_TestSpherePlane(
            &delta,
            &viewContext->worldFrustumNormals[5],
            radius,
            0x20,
            clipMaskInOut
        );
        if (result != 0) {
            return result;
        }
    }

    return 0;
}

/**
 * Reimplements 0x478fc0: zModel_Instance_UpdateScrollingTexturesIfNeeded
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: update all scrolling-texture surface entries once per video frame.
 */
int __fastcall zModel_Instance_UpdateScrollingTexturesIfNeeded(
    zModel_InstancePartial *instance
) {
    if (instance == 0) {
        return -1;
    }

    if (instance->scrollingTextureFrameTick == g_zVideo_FrameTick) {
        return 0;
    }

    instance->scrollingTextureFrameTick = g_zVideo_FrameTick;
    for (int i = 0; i < instance->surfaceEntryCount; ++i) {
        zModel_InstanceSurfaceEntryPartial *entry = &instance->surfaceEntries[i];
        zModel_MaterialTextureBindingPartial *material = entry->materialBinding;
        if ((material->flags & 1) == 0) {
            continue;
        }

        zModel_Instance_UpdateScrollingTextures(
            material->textureRef->textureInfo,
            entry->uvs,
            &instance->scrollRateU,
            (int)(entry->vertexCountAndFlags & 0xff)
        );
    }

    return 0;
}

/**
 * Reimplements 0x479020: zModel_RenderPointQueueEntry
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: project and submit one display-instance point/lens-flare queue entry.
 */
void __fastcall zModel_RenderPointQueueEntry(
    const zVec3 *pointPos,
    int packedColor16,
    zModel_PointEntryPartial *pointEntry
) {
    zVec3 transformedPoint = *pointPos;
    if (*zMath::g_currentMatrixIdentityFlagSlot == 0) {
        transformedPoint = TransformPointByCurrentMatrix(pointPos);
    }

    if (transformedPoint.z <= gClipRect_Primary.zMin) {
        return;
    }

    zProjectedPoint projectedPoint = {0};
    if (g_zVideo_ActiveRendererPath == 0) {
        zMath::ProjectPointBatch(
            &transformedPoint,
            &projectedPoint,
            1
        );
    } else {
        zMath_ProjectSphereBatch(
            &transformedPoint,
            (zProjectedSphere *)(&projectedPoint),
            1
        );
    }

    if (!ProjectedPointInClipBounds(projectedPoint)) {
        return;
    }

    const int color16 = packedColor16 & 0xffff;
    const int source = (int)((int)(&pointEntry->lensFlareSource[0]));
    if (g_zVideo_ActiveRendererPath == 0) {
        zRndr_LensFlare_QueueProjectedSample(
            &projectedPoint,
            color16,
            source
        );
        return;
    }

    const int depthBias = (short)(pointEntry->depthBiasWord & 0xffff);
    projectedPoint.reciprocalZ =
        (((float)(depthBias)*g_zRndr_InverseZTolerance) + 1.0f) * projectedPoint.reciprocalZ;

    zVideo_DrawPointColor16Proc drawPoint = g_zVideo_pfnDrawPointColor16;
    drawPoint(
        (zVideo_XyzVertex *)(&projectedPoint),
        (unsigned int)(color16),
        1
    );
    zRndr_LensFlare_QueueProjectedSample(
        &projectedPoint,
        color16,
        source
    );
}

/**
 * Reimplements 0x4791c0: zModel_Instance_UpdateScrollingTextures
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: advance scrolling texture UVs for one surface entry and wrap them into range.
 */
void __fastcall zModel_Instance_UpdateScrollingTextures(
    const zModel_TextureScrollInfoPartial *textureInfo,
    zModel_Uv *uvs,
    const float *scrollRates,
    int uvCount
) {
    if (uvCount <= 0) {
        return;
    }

    const float rateU = scrollRates[0];
    const float rateV = scrollRates[1];
    if (rateU == 0.0f && rateV == 0.0f) {
        return;
    }

    const float deltaU = rateU * g_FrameDeltaTimeSec;
    const float deltaV = rateV * g_FrameDeltaTimeSec;

    float minU = uvs[0].u + deltaU;
    float maxU = minU;
    float minV = uvs[0].v + deltaV;
    float maxV = minV;
    uvs[0].u = minU;
    uvs[0].v = minV;

    for (int i = 1; i < uvCount; ++i) {
        if (rateU != 0.0f) {
            const float u = uvs[i].u + deltaU;
            uvs[i].u = u;
            if (u < minU) {
                minU = u;
            }
            if (u > maxU) {
                maxU = u;
            }
        }

        if (rateV != 0.0f) {
            const float v = uvs[i].v + deltaV;
            uvs[i].v = v;
            if (v < minV) {
                minV = v;
            }
            if (v > maxV) {
                maxV = v;
            }
        }
    }

    int correctionU = 0;
    if (rateU != 0.0f) {
        correctionU = ComputeWrapCorrection(
            minU,
            maxU,
            TextureWrapExtent(textureInfo, true)
        );
    }

    int correctionV = 0;
    if (rateV != 0.0f) {
        correctionV = ComputeWrapCorrection(
            minV,
            maxV,
            TextureWrapExtent(textureInfo, false)
        );
    }

    ApplyUvCorrection(
        uvs,
        uvCount,
        correctionU,
        correctionV
    );
}

namespace OptCatalog {
/**
 * Reimplements 0x479660: OptCatalog::ApplyDamageMaskStampOnHit
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: stamp the active damage mask onto an eligible OptCatalog hit surface.
 */
void __fastcall ApplyDamageMaskStampOnHit(
    OptCatalogHitEventPartial *hitEvent
) {
    if (OptCatalog_IsDamageMaskEnabled() == 0) {
        return;
    }

    OptCatalogSurfaceMaterialRef *const surfaceRef = hitEvent->surfaceRef;
    if (surfaceRef == 0) {
        return;
    }

    const unsigned int materialFlags = surfaceRef->flags;
    if ((materialFlags & 0x0100) == 0 || (materialFlags & 0x0200) == 0 ||
        (materialFlags & 0x0400) != 0) {
        return;
    }

    WrapDamageMaskPhase(&g_OptCatalogDamageMaskPhaseU);
    WrapDamageMaskPhase(&g_OptCatalogDamageMaskPhaseV);

    OptCatalogSurfaceTextureHandle *const srcHandle = (OptCatalogSurfaceTextureHandle *)
        g_OptCatalogDamageMaskHandles[g_OptCatalogDamageMaskSlotIndex];
    OptCatalogDamageMaskSurface *const srcSurface = srcHandle != 0 ? srcHandle->surface : 0;
    OptCatalogSurfaceTextureHandle *const dstHandle = surfaceRef->textureHandle;
    OptCatalogDamageMaskSurface *const dstSurface = dstHandle != 0 ? dstHandle->surface : 0;
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
    ClampDamageMaskAxis(
        dstX,
        srcWidth,
        dstWidth,
        &srcXBegin,
        &srcXEnd,
        &dstX
    );
    ClampDamageMaskAxis(
        dstY,
        srcHeight,
        dstHeight,
        &srcYBegin,
        &srcYEnd,
        &dstY
    );

    unsigned short *dstPixels = dstSurface->pixels;
    int dstStride = dstWidth;
    const bool hasTextureRecord = dstHandle->textureRecord != 0;
    if (hasTextureRecord) {
        zVideo_TextureRecordLockUploadSurfaceProc lockUploadSurface =
            g_zVideo_pfnTextureRecordLockUploadSurface;
        if (lockUploadSurface(
            dstHandle->textureRecord,
            (void **)&dstPixels,
            &dstStride
        ) == 0) {
            return;
        }
        dstStride >>= 1;
    }

    if (srcSurface->alpha == 0) {
        for (int srcY = srcYBegin, outY = dstY; srcY < srcYEnd; ++srcY, ++outY) {
            unsigned short *dst = dstPixels + outY * dstWidth + dstX;
            unsigned short *src = srcSurface->pixels + srcY * srcWidth + srcXBegin;
            for (int srcX = srcXBegin; srcX < srcXEnd; ++srcX, ++src) {
                if (*src != 0) {
                    *dst = *src;
                }
                ++dst;
            }
        }
    } else if (zRndr::g_pixelPackGreenBits == 6) {
        for (int srcY = srcYBegin, outY = dstY; srcY < srcYEnd; ++srcY, ++outY) {
            unsigned short *dst = dstPixels + outY * dstStride + dstX;
            unsigned short *src = srcSurface->pixels + srcY * srcWidth + srcXBegin;
            unsigned char *alpha = srcSurface->alpha + srcY * srcWidth + srcXBegin;
            for (int srcX = srcXBegin; srcX < srcXEnd; ++srcX, ++src, ++alpha, ++dst) {
                const int alphaValue = *alpha;
                if (alphaValue == 0 || alphaValue <= 3) {
                    continue;
                }
                if (alphaValue >= 0xfc) {
                    *dst = *src;
                } else {
                    *dst = BlendDamageMaskPixel565(
                        *dst,
                        *src,
                        alphaValue
                    );
                }
            }
        }
    } else {
        for (int srcY = srcYBegin, outY = dstY; srcY < srcYEnd; ++srcY, ++outY) {
            unsigned short *dst = dstPixels + outY * dstStride + dstX;
            unsigned short *src = srcSurface->pixels + srcY * srcWidth + srcXBegin;
            unsigned char *alpha = srcSurface->alpha + srcY * srcWidth + srcXBegin;
            for (int srcX = srcXBegin; srcX < srcXEnd; ++srcX, ++src, ++alpha, ++dst) {
                const int alphaValue = *alpha;
                if (alphaValue == 0 || alphaValue <= 7) {
                    continue;
                }
                if (alphaValue >= 0xfc) {
                    *dst = *src;
                } else {
                    *dst = BlendDamageMaskPixel555(
                        *dst,
                        *src,
                        alphaValue
                    );
                }
            }
        }
    }

    if (hasTextureRecord) {
        zVideo_TextureRecordUnlockUploadSurfaceProc unlockUploadSurface =
            g_zVideo_pfnTextureRecordUnlockUploadSurface;
        zVideo_TextureRecordFinalizeUploadProc finalizeUpload =
            g_zVideo_pfnTextureRecordFinalizeUpload;
        unlockUploadSurface(dstHandle->textureRecord);
        finalizeUpload(
            dstHandle->textureRecord,
            &dstX,
            0
        );
    }
}
} // namespace OptCatalog

namespace OptCatalog {
/**
 * Reimplements 0x479c50: OptCatalog::SetDamageMaskSlotIndex
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: select the active damage-mask handle slot.
 */
void __fastcall SetDamageMaskSlotIndex(
    int slotIndex
) {
    g_OptCatalogDamageMaskSlotIndex = slotIndex;
}
} // namespace OptCatalog

namespace OptCatalog {
/**
 * Reimplements 0x479c60: OptCatalog::RegisterDamageMaskSlotPtr
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: register a damage-mask texture handle in the active OptCatalog slot.
 */
void __fastcall RegisterDamageMaskSlotPtr(
    void *slotPtr
) {
    g_OptCatalogDamageMaskEnabled = 1;
    g_OptCatalogDamageMaskHandles[g_OptCatalogDamageMaskSlotIndex] = slotPtr;
}
} // namespace OptCatalog

/**
 * Reimplements 0x479c80: OptCatalog_IsDamageMaskEnabled
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: report whether OptCatalog damage-mask stamping is currently enabled.
 */
int OptCatalog_IsDamageMaskEnabled() {
    return g_OptCatalogDamageMaskEnabled;
}

/**
 * Reimplements 0x479c90: OptCatalog_SetDamageMaskUv
 * (GameZRecoil/zModel/zModel_Display.cpp).
 * Purpose: set the current damage-mask UV phase used by the OptCatalog stamp pass.
 */
void __stdcall OptCatalog_SetDamageMaskUv(
    float u,
    float v
) {
    g_OptCatalogDamageMaskPhaseU = u;
    g_OptCatalogDamageMaskPhaseV = v;
}

/**
 * Reimplements 0x479cb0: OptCatalog_SetDamageMaskEnabled
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: update the global OptCatalog damage-mask enable flag.
 */
void __fastcall OptCatalog_SetDamageMaskEnabled(
    int enabled
) {
    g_OptCatalogDamageMaskEnabled = enabled;
}

/**
 * Reimplements 0x479cc0: OptCatalog_IsDamageMaskSlotPtrRegistered
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: test whether a damage-mask slot already references the supplied handle.
 */
int __fastcall OptCatalog_IsDamageMaskSlotPtrRegistered(
    void *slotPtr
) {
    for (int i = 0; i < 3; ++i) {
        if (g_OptCatalogDamageMaskHandles[i] == slotPtr) {
            return 1;
        }
    }

    return 0;
}

/**
 * Reimplements 0x479ce0: zVideo_SetActiveViewContext.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Data evidence: BN stores the supplied camera context into the projection
 * context cache at 0x576214, updates gClipRect_Primary at 0x576218, and writes
 * the project clip floats at 0x57623c..0x576248 before zMath projection setup.
 * Purpose: provide the recovered zVideo_SetActiveViewContext behavior.
 */
void __fastcall zVideo_SetActiveViewContext(
    zClass_CameraDataPartial *viewContext
) {
    g_zVideo_pActiveProjectionViewContext = viewContext;

    if (viewContext->nearClip < 1.0f) {
        viewContext->nearClip = 1.0f;
    }

    gClipRect_Primary.zMin = viewContext->nearClip + viewContext->nearClip;
    if (g_zVideo_ActiveRendererPath == 0) {
        zVideo_dd3d::SetQuadBatchDepthAndRhw(1.0f / gClipRect_Primary.zMin);
        viewContext = g_zVideo_pActiveProjectionViewContext;
    }

    gClipRect_Primary.zMax = viewContext->farClip;

    int windowX;
    int windowY;
    if (zClass_Window::gwWindowGetSize(
        viewContext->windowNode,
        &windowX,
        &windowY
    ) != 0) {
        windowX = 0;
        windowY = 0;
    }

    viewContext = g_zVideo_pActiveProjectionViewContext;
    int width;
    int height;
    if (zClass_Window::gwWindowGetResolution(
        viewContext->windowNode,
        &width,
        &height
    ) != 0) {
        width = zVideo::GetPrimarySurfaceWidth();
        height = zVideo::GetPrimarySurfaceHeight();
    }

    const int rightPx = windowX + width;
    const int bottomPx = windowY + height;
    const float left = (float)(windowX);
    const float top = (float)(windowY);
    const float right = (float)(rightPx);
    const float bottom = (float)(bottomPx);
    float viewportOriginX;
    float viewportOriginY;
    float viewportBottom;
    float projectClipLeft;

    if (g_zVideo_ActiveRendererPath == 0) {
        viewportOriginX = left;
        viewportOriginY = top;
        viewportBottom = bottom;
        projectClipLeft = left;
        gClipRect_Primary.xMin = left + 0.5f - 0.999000013f;
        gClipRect_Primary.xMax = right + 1.49900007f;
        gClipRect_Primary.xMaxAlt = right + 0.5f - 0.00100000005f;
        gClipRect_Primary.yMin = top + 0.5f - 0.999000013f;
        gClipRect_Primary.yMax = bottom + 1.49900007f;
        gClipRect_Primary.yMaxAlt = bottom + 0.5f - 0.00100000005f;
    } else {
        const float rightWithSlop = right + 0.00100000005f;
        const float bottomWithSlop = bottom + 0.00100000005f;
        viewportOriginX = left;
        viewportOriginY = top;
        viewportBottom = bottomWithSlop;
        projectClipLeft = left;
        gClipRect_Primary.xMin = left;
        gClipRect_Primary.xMax = rightWithSlop;
        gClipRect_Primary.xMaxAlt = rightWithSlop;
        gClipRect_Primary.yMin = top;
        gClipRect_Primary.yMax = bottomWithSlop;
        gClipRect_Primary.yMaxAlt = bottomWithSlop;
    }

    g_zVideo_ProjectClipLeft = projectClipLeft;
    g_zVideo_ProjectClipTop = viewportOriginY;
    g_zVideo_ProjectClipRight = right - 0.00100000005f;
    g_zVideo_ProjectClipBottom = viewportBottom - 0.00100000005f;

    viewContext = g_zVideo_pActiveProjectionViewContext;
    zMath_Setup_Projection(
        viewportOriginX,
        viewportOriginY,
        (float)(width) * 0.5f,
        (float)(height) * 0.5f,
        viewContext->viewportScaleX,
        viewContext->viewportScaleY,
        viewContext->nearClip,
        viewContext->farClip
    );

    int fovXBits;
    int fovYBits;
    memcpy(
        &fovXBits,
        &viewContext->fovX,
        sizeof(fovXBits)
    );
    memcpy(
        &fovYBits,
        &viewContext->fovY,
        sizeof(fovYBits)
    );
    zMath_SetScreenSize(
        fovXBits,
        fovYBits
    );
}

namespace zClipAlt {
/**
 * Reimplements 0x479f90: zClipAlt::SetTargetRect
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 *
 * Purpose: configure the alternate clipping rectangle and source-to-target
 * coordinate remap scale and bias.
 */
void __fastcall SetTargetRect(
    const zClipAltFloatRect *rect,
    int replicate
) {
    gClipRect_Alt.flags = 0x0f;
    gClipRect_Alt.xMin = rect->left;
    gClipRect_Alt.yMin = rect->top;
    gClipRect_Alt.xMax = rect->right;
    gClipRect_Alt.yMax = rect->bottom;
    gClipRect_Alt.xMaxAlt = rect->right;
    gClipRect_Alt.yMaxAlt = rect->bottom;

    g_zClipAlt_RemapOffsetX = rect->left - g_zClipAlt_SourceLeft;
    g_zClipAlt_RemapOffsetY = rect->top - g_zClipAlt_SourceTop;
    g_zClipAlt_RemapScaleX = g_zClipAlt_SourceWidth / (rect->right - rect->left);
    g_zClipAlt_RemapScaleY = g_zClipAlt_SourceHeight / (rect->bottom - rect->top);

    float primaryOriginX = gClipRect_Primary.xMin;
    float primaryOriginY = gClipRect_Primary.yMin;
    if (replicate != 0) {
        primaryOriginX *= 0.5f;
        primaryOriginY *= 0.5f;
    }

    g_zClipAlt_RemapBiasX = g_zClipAlt_SourceLeft - gClipRect_Alt.xMin * g_zClipAlt_RemapScaleX;
    g_zClipAlt_RemapBiasY = g_zClipAlt_SourceTop - gClipRect_Alt.yMin * g_zClipAlt_RemapScaleY;

    if (g_zClipAlt_BiasIncludesPrimaryOrigin != 0) {
        g_zClipAlt_RemapBiasX += primaryOriginX;
        g_zClipAlt_RemapBiasY += primaryOriginY;
    }
}
} // namespace zClipAlt

/**
 * Reimplements 0x47a0c0: zVideo_UpdateProjectionStateFromCameraData.
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: provide the recovered zVideo_UpdateProjectionStateFromCameraData behavior.
 */
void __fastcall zVideo_UpdateProjectionStateFromCameraData(
    zClass_CameraDataPartial *cameraData
) {
    zMat4x3 slotBuffer = {0};
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();

    zMat4x3 yawSlotBuffer = {0};
    zMath::MatStackPushAndCloneParent((float *)(&yawSlotBuffer));
    cameraData->localFrustumLeftNormal.x = 1.0f;
    cameraData->localFrustumLeftNormal.y = 0.0f;
    cameraData->localFrustumLeftNormal.z = 0.0f;
    zMath::MatRotateY(cameraData->frustumYaw);
    zMath_Vec3Array_UntransformDirection(
        &cameraData->localFrustumLeftNormal,
        1
    );
    zMath::MatStackPopPtr();

    cameraData->localFrustumRightNormal.x = -cameraData->localFrustumLeftNormal.x;
    cameraData->localFrustumRightNormal.y = cameraData->localFrustumLeftNormal.y;
    cameraData->localFrustumRightNormal.z = cameraData->localFrustumLeftNormal.z;

    cameraData->localFrustumBottomNormal.x = 0.0f;
    cameraData->localFrustumBottomNormal.y = -1.0f;
    cameraData->localFrustumBottomNormal.z = 0.0f;
    zMath::MatRotateX(cameraData->frustumPitch);
    zMath_Vec3Array_UntransformDirection(
        &cameraData->localFrustumBottomNormal,
        1
    );
    zMath::MatStackPopPtr();

    cameraData->localFrustumTopNormal.x = cameraData->localFrustumBottomNormal.x;
    cameraData->localFrustumTopNormal.y = -cameraData->localFrustumBottomNormal.y;
    cameraData->localFrustumTopNormal.z = cameraData->localFrustumBottomNormal.z;

    cameraData->localFrustumNearNormal.x = 0.0f;
    cameraData->localFrustumNearNormal.y = 0.0f;
    cameraData->localFrustumNearNormal.z = -1.0f;
    cameraData->localFrustumFarNormal.x = 0.0f;
    cameraData->localFrustumFarNormal.y = 0.0f;
    cameraData->localFrustumFarNormal.z = 1.0f;
}

/**
 * Reimplements 0x47a1d0: zClipAlt_BuildFrustumPlanes
 * (GameZRecoil/zModel/zmodel.cpp).
 *
 * Purpose: transform the camera's local frustum normals into world-space
 * clipping planes for the alternate clipping pass.
 */
void __fastcall zClipAlt_BuildFrustumPlanes(
    zClass_CameraDataPartial *cameraData
) {
    zMath::MatStackPushPtr(cameraData->worldTransform);
    zMath_Mat_TransformNormalBatch(
        &cameraData->localFrustumLeftNormal,
        cameraData->worldFrustumNormals,
        6
    );
    zMath::MatStackPopPtr();
}

namespace zClipRect {
/**
 * Reimplements 0x47a200: zClipRect::ClipPolyZRange_NoUV
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip the scratch polygon vertex stream against the configured Z range without attributes.
 */
int __fastcall ClipPolyZRange_NoUV(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert clippedVerts[kClipBufferCapacity];
    int outputCount;
    int edgeIndex;
    zClipRectPartial *rect = clipRect;
    int *count = vertexCount;
    const int flags = rect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < *count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < rect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < *count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < rect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return *count >= 3 ? 1 : 0;
    }

    outputCount = 0;
    edgeIndex = 0;

    int prevIndex = *count - 1;
    for (; edgeIndex < *count; ++edgeIndex) {
        const zClipVert &prevVert = g_Clip_PolyVertsScratch[prevIndex];
        const zClipVert &currVert = g_Clip_PolyVertsScratch[edgeIndex];
        if (
            prevVert.z >= rect->zMin
            && currVert.z >= rect->zMin
        ) {
            clippedVerts[outputCount] = currVert;
            ++outputCount;
        } else if (
            prevVert.z >= rect->zMin
            && currVert.z < rect->zMin
        ) {
            const float t =
                (rect->zMin - prevVert.z) /
                (currVert.z - prevVert.z);
            clippedVerts[outputCount].x =
                prevVert.x +
                (currVert.x - prevVert.x) * t;
            clippedVerts[outputCount].y =
                prevVert.y +
                (currVert.y - prevVert.y) * t;
            clippedVerts[outputCount].z = rect->zMin;
            ++outputCount;
        } else if (currVert.z >= rect->zMin) {
            const float t =
                (rect->zMin - prevVert.z) /
                (currVert.z - prevVert.z);
            clippedVerts[outputCount].x =
                prevVert.x +
                (currVert.x - prevVert.x) * t;
            clippedVerts[outputCount].y =
                prevVert.y +
                (currVert.y - prevVert.y) * t;
            clippedVerts[outputCount].z = rect->zMin;
            ++outputCount;
            clippedVerts[outputCount] = currVert;
            ++outputCount;
        }

        prevIndex = edgeIndex;
    }

    *count = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(
        g_Clip_PolyVertsScratch,
        clippedVerts,
        (size_t)(outputCount) * sizeof(zClipVert)
    );
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47a4e0: zClipRect::ClipPolyZRange_NoUV_WithAttribs
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip the scratch polygon vertex stream against the configured Z range while preserving three attributes.
 */
int __fastcall ClipPolyZRange_NoUV_WithAttribs(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    const int count = *vertexCount;
    const int flags = clipRect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    zClipVert clippedVerts[kClipBufferCapacity] = {0};
    float clippedAttr0[kClipBufferCapacity] = {0};
    float clippedAttr1[kClipBufferCapacity] = {0};
    float clippedAttr2[kClipBufferCapacity] = {0};
    int outputCount = 0;

    if (count > 0) {
        zClipVert prevVert = g_Clip_PolyVertsScratch[count - 1];
        float prevAttr0 = g_Clip_PolyAttr0[count - 1];
        float prevAttr1 = g_Clip_PolyAttr1[count - 1];
        float prevAttr2 = g_Clip_PolyAttr2[count - 1];
        bool prevInside = IsInsideNear(
            prevVert,
            clipRect->zMin
        );

        for (int i = 0; i < count; ++i) {
            const zClipVert currVert = g_Clip_PolyVertsScratch[i];
            const float currAttr0 = g_Clip_PolyAttr0[i];
            const float currAttr1 = g_Clip_PolyAttr1[i];
            const float currAttr2 = g_Clip_PolyAttr2[i];
            const bool currInside = IsInsideNear(
                currVert,
                clipRect->zMin
            );

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                AppendClippedVertWithAttr012(
                    clippedVerts,
                    clippedAttr0,
                    clippedAttr1,
                    clippedAttr2,
                    outputCount,
                    InterpolateVert(
                        prevVert,
                        currVert,
                        t,
                        clipRect->zMin
                    ),
                    InterpolateFloat(
                        prevAttr0,
                        currAttr0,
                        t
                    ),
                    InterpolateFloat(
                        prevAttr1,
                        currAttr1,
                        t
                    ),
                    InterpolateFloat(prevAttr2, currAttr2, t)
                );
            }

            if (currInside) {
                AppendClippedVertWithAttr012(
                    clippedVerts,
                    clippedAttr0,
                    clippedAttr1,
                    clippedAttr2,
                    outputCount,
                    currVert,
                    currAttr0,
                    currAttr1,
                    currAttr2
                );
            }

            prevVert = currVert;
            prevAttr0 = currAttr0;
            prevAttr1 = currAttr1;
            prevAttr2 = currAttr2;
            prevInside = currInside;
        }
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(
        g_Clip_PolyVertsScratch,
        clippedVerts,
        (size_t)(outputCount) * sizeof(zClipVert)
    );
    memcpy(
        g_Clip_PolyAttr0,
        clippedAttr0,
        (size_t)(outputCount) * sizeof(float)
    );
    memcpy(
        g_Clip_PolyAttr1,
        clippedAttr1,
        (size_t)(outputCount) * sizeof(float)
    );
    memcpy(
        g_Clip_PolyAttr2,
        clippedAttr2,
        (size_t)(outputCount) * sizeof(float)
    );
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47aa80: zClipRect::ClipPolyNearZ
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip the scratch polygon vertex and UV streams against the configured near Z plane.
 */
int __fastcall ClipPolyNearZ(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    const int flags = clipRect->flags;
    int i;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (i = 0; i < *vertexCount && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (i = 0; i < *vertexCount && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return *vertexCount >= 3 ? 1 : 0;
    }

    zClipVert clippedVerts[kClipBufferCapacity];
    zClipUV clippedUvs[kClipBufferCapacity];
    zClipVert *outVert = clippedVerts;
    zClipUV *outUv = clippedUvs;
    int outputCount = 0;
    int prevIndex = *vertexCount - 1;

    for (i = 0; i < *vertexCount; ++i) {
        zClipVert *prevVert = &g_Clip_PolyVertsScratch[prevIndex];
        zClipVert *currVert = &g_Clip_PolyVertsScratch[i];
        zClipUV *prevUv = &g_Clip_PolyUvs[prevIndex];
        zClipUV *currUv = &g_Clip_PolyUvs[i];

        if (
            prevVert->z >= clipRect->zMin
            && currVert->z >= clipRect->zMin
        ) {
            *outVert = *currVert;
            *outUv = *currUv;
            ++outVert;
            ++outUv;
            ++outputCount;
        } else if (
            prevVert->z >= clipRect->zMin
            && currVert->z < clipRect->zMin
        ) {
            const float t =
                (clipRect->zMin - prevVert->z) /
                (currVert->z - prevVert->z);
            outVert->x = prevVert->x + (currVert->x - prevVert->x) * t;
            outVert->y = prevVert->y + (currVert->y - prevVert->y) * t;
            outVert->z = clipRect->zMin;
            outUv->u = prevUv->u + (currUv->u - prevUv->u) * t;
            outUv->v = prevUv->v + (currUv->v - prevUv->v) * t;
            ++outVert;
            ++outUv;
            ++outputCount;
        } else if (currVert->z >= clipRect->zMin) {
            const float t =
                (clipRect->zMin - prevVert->z) /
                (currVert->z - prevVert->z);
            outVert->x = prevVert->x + (currVert->x - prevVert->x) * t;
            outVert->y = prevVert->y + (currVert->y - prevVert->y) * t;
            outVert->z = clipRect->zMin;
            outUv->u = prevUv->u + (currUv->u - prevUv->u) * t;
            outUv->v = prevUv->v + (currUv->v - prevUv->v) * t;
            ++outVert;
            ++outUv;
            ++outputCount;

            *outVert = *currVert;
            *outUv = *currUv;
            ++outVert;
            ++outUv;
            ++outputCount;
        }

        prevIndex = i;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(
        g_Clip_PolyVertsScratch,
        clippedVerts,
        (size_t)(outputCount) * sizeof(zClipVert)
    );
    memcpy(
        g_Clip_PolyUvs,
        clippedUvs,
        (size_t)(outputCount) * sizeof(zClipUV)
    );
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47af60: zClipRect::ClipPolyNearZ_WithAttr0
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip the scratch polygon vertex, UV, and first-attribute streams against near Z.
 */
int __fastcall ClipPolyNearZ_WithAttr0(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    const int count = *vertexCount;
    const int flags = clipRect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    int result = 0;
    if (allInsideNear != result) {
        result = count >= 3;
        return result;
    }

    zClipVert clippedVerts[kClipBufferCapacity] = {0};
    zClipUV clippedUvs[kClipBufferCapacity] = {0};
    float clippedAttrs[kClipBufferCapacity] = {0};
    int outputCount = 0;

    if (count > 0) {
        zClipVert prevVert = g_Clip_PolyVertsScratch[count - 1];
        zClipUV prevUv = g_Clip_PolyUvs[count - 1];
        float prevAttr = g_Clip_PolyAttr0[count - 1];
        bool prevInside = IsInsideNear(
            prevVert,
            clipRect->zMin
        );

        for (int i = 0; i < count; ++i) {
            const zClipVert currVert = g_Clip_PolyVertsScratch[i];
            const zClipUV currUv = g_Clip_PolyUvs[i];
            const float currAttr = g_Clip_PolyAttr0[i];
            const bool currInside = IsInsideNear(
                currVert,
                clipRect->zMin
            );

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                AppendClippedWithAttr(
                    clippedVerts,
                    clippedUvs,
                    clippedAttrs,
                    outputCount,
                    InterpolateVert(
                        prevVert,
                        currVert,
                        t,
                        clipRect->zMin
                    ),
                    InterpolateUv(
                        prevUv,
                        currUv,
                        t
                    ),
                    InterpolateFloat(prevAttr, currAttr, t)
                );
            }

            if (currInside) {
                AppendClippedWithAttr(
                    clippedVerts,
                    clippedUvs,
                    clippedAttrs,
                    outputCount,
                    currVert,
                    currUv,
                    currAttr
                );
            }

            prevVert = currVert;
            prevUv = currUv;
            prevAttr = currAttr;
            prevInside = currInside;
        }
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(
        g_Clip_PolyVertsScratch,
        clippedVerts,
        (size_t)(outputCount) * sizeof(zClipVert)
    );
    memcpy(
        g_Clip_PolyUvs,
        clippedUvs,
        (size_t)(outputCount) * sizeof(zClipUV)
    );
    memcpy(
        g_Clip_PolyAttr0,
        clippedAttrs,
        (size_t)(outputCount) * sizeof(float)
    );
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47b540: zClipRect::ClipPoly_NoUV_Alt
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip the active polygon vertex stream against enabled XY bounds without UVs.
 */
int __fastcall ClipPoly_NoUV_Alt(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVerts[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert *currVert = &g_Clip_PolyVerts[i];

                if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x >= clipRect->xMin
                ) {
                    scratchVerts[outputCount] = *currVert;
                    ++outputCount;
                } else if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x < clipRect->xMin
                ) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;

                    scratchVerts[outputCount] = *currVert;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = 1;
    }

    if ((clipRect->flags & 0x02) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];

                if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x < clipRect->xMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                } else if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x >= clipRect->xMaxAlt
                ) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x04) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];

                if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y >= clipRect->yMin
                ) {
                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                } else if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y < clipRect->yMin
                ) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x08) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];

                if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y < clipRect->yMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                } else if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y >= clipRect->yMaxAlt
                ) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        parity = (parity + 1) % 2;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (parity == 1) {
        memcpy(
            g_Clip_PolyVerts,
            scratchVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
    }
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47bd30: zClipRect::ClipPoly_NoUV_WithAttr012_Alt
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip active polygon vertex and three-attribute streams against enabled XY bounds.
 */
int __fastcall ClipPoly_NoUV_WithAttr012_Alt(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVerts[kClipBufferCapacity];
    float scratchAttr0[kClipBufferCapacity];
    float scratchAttr1[kClipBufferCapacity];
    float scratchAttr2[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert *currVert = &g_Clip_PolyVerts[i];
                float prevAttr0 = g_Clip_PolyAttr0[prevIndex];
                float prevAttr1 = g_Clip_PolyAttr1[prevIndex];
                float prevAttr2 = g_Clip_PolyAttr2[prevIndex];
                float currAttr0 = g_Clip_PolyAttr0[i];
                float currAttr1 = g_Clip_PolyAttr1[i];
                float currAttr2 = g_Clip_PolyAttr2[i];

                if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x >= clipRect->xMin
                ) {
                    scratchVerts[outputCount] = *currVert;
                    scratchAttr0[outputCount] = currAttr0;
                    scratchAttr1[outputCount] = currAttr1;
                    scratchAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x < clipRect->xMin
                ) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    scratchAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    scratchAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    scratchAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    scratchAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;

                    scratchVerts[outputCount] = *currVert;
                    scratchAttr0[outputCount] = currAttr0;
                    scratchAttr1[outputCount] = currAttr1;
                    scratchAttr2[outputCount] = currAttr2;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = 1;
    }

    if ((clipRect->flags & 0x02) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        float *sourceAttr0;
        float *sourceAttr1;
        float *sourceAttr2;
        float *destAttr0;
        float *destAttr1;
        float *destAttr2;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceAttr0 = scratchAttr0;
            sourceAttr1 = scratchAttr1;
            sourceAttr2 = scratchAttr2;
            destVerts = g_Clip_PolyVerts;
            destAttr0 = g_Clip_PolyAttr0;
            destAttr1 = g_Clip_PolyAttr1;
            destAttr2 = g_Clip_PolyAttr2;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttr0 = g_Clip_PolyAttr0;
            sourceAttr1 = g_Clip_PolyAttr1;
            sourceAttr2 = g_Clip_PolyAttr2;
            destVerts = scratchVerts;
            destAttr0 = scratchAttr0;
            destAttr1 = scratchAttr1;
            destAttr2 = scratchAttr2;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x < clipRect->xMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x >= clipRect->xMaxAlt
                ) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x04) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        float *sourceAttr0;
        float *sourceAttr1;
        float *sourceAttr2;
        float *destAttr0;
        float *destAttr1;
        float *destAttr2;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceAttr0 = scratchAttr0;
            sourceAttr1 = scratchAttr1;
            sourceAttr2 = scratchAttr2;
            destVerts = g_Clip_PolyVerts;
            destAttr0 = g_Clip_PolyAttr0;
            destAttr1 = g_Clip_PolyAttr1;
            destAttr2 = g_Clip_PolyAttr2;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttr0 = g_Clip_PolyAttr0;
            sourceAttr1 = g_Clip_PolyAttr1;
            sourceAttr2 = g_Clip_PolyAttr2;
            destVerts = scratchVerts;
            destAttr0 = scratchAttr0;
            destAttr1 = scratchAttr1;
            destAttr2 = scratchAttr2;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y >= clipRect->yMin
                ) {
                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y < clipRect->yMin
                ) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x08) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        float *sourceAttr0;
        float *sourceAttr1;
        float *sourceAttr2;
        float *destAttr0;
        float *destAttr1;
        float *destAttr2;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceAttr0 = scratchAttr0;
            sourceAttr1 = scratchAttr1;
            sourceAttr2 = scratchAttr2;
            destVerts = g_Clip_PolyVerts;
            destAttr0 = g_Clip_PolyAttr0;
            destAttr1 = g_Clip_PolyAttr1;
            destAttr2 = g_Clip_PolyAttr2;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttr0 = g_Clip_PolyAttr0;
            sourceAttr1 = g_Clip_PolyAttr1;
            sourceAttr2 = g_Clip_PolyAttr2;
            destVerts = scratchVerts;
            destAttr0 = scratchAttr0;
            destAttr1 = scratchAttr1;
            destAttr2 = scratchAttr2;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y < clipRect->yMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y >= clipRect->yMaxAlt
                ) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        parity = (parity + 1) % 2;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (parity == 1) {
        memcpy(
            g_Clip_PolyVerts,
            scratchVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
        memcpy(
            g_Clip_PolyAttr0,
            scratchAttr0,
            (size_t)(outputCount) * sizeof(float)
        );
        memcpy(
            g_Clip_PolyAttr1,
            scratchAttr1,
            (size_t)(outputCount) * sizeof(float)
        );
        memcpy(
            g_Clip_PolyAttr2,
            scratchAttr2,
            (size_t)(outputCount) * sizeof(float)
        );
    }
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47cdc0: zClipRect::ClipPoly_NoUV
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip the primary polygon vertex stream against enabled XY bounds without UVs.
 */
int __fastcall ClipPoly_NoUV(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVerts[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert *currVert = &g_Clip_PolyVerts[i];

                if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x >= clipRect->xMin
                ) {
                    scratchVerts[outputCount].x = currVert->x;
                    scratchVerts[outputCount].y = currVert->y;
                    ++outputCount;
                } else if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x < clipRect->xMin
                ) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    ++outputCount;

                    scratchVerts[outputCount].x = currVert->x;
                    scratchVerts[outputCount].y = currVert->y;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = 1;
    }

    if ((clipRect->flags & 0x02) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];

                if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x < clipRect->xMaxAlt
                ) {
                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                } else if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x >= clipRect->xMaxAlt
                ) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    ++outputCount;

                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x04) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];

                if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y >= clipRect->yMin
                ) {
                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                } else if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y < clipRect->yMin
                ) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    ++outputCount;

                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x08) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];

                if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y < clipRect->yMaxAlt
                ) {
                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                } else if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y >= clipRect->yMaxAlt
                ) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    ++outputCount;

                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        parity = (parity + 1) % 2;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (parity == 1) {
        memcpy(
            g_Clip_PolyVerts,
            scratchVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
    }
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47d3f0: zClipRect::ClipPoly
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip active polygon vertex and UV streams against enabled XY bounds.
 */
int __fastcall ClipPoly(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVerts[kClipBufferCapacity];
    zClipUV scratchUvs[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert *currVert = &g_Clip_PolyVerts[i];
                zClipUV *prevUv = &g_Clip_PolyUvs[prevIndex];
                zClipUV *currUv = &g_Clip_PolyUvs[i];

                if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x >= clipRect->xMin
                ) {
                    scratchVerts[outputCount] = *currVert;
                    scratchUvs[outputCount] = *currUv;
                    ++outputCount;
                } else if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x < clipRect->xMin
                ) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    scratchUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    scratchUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;

                    scratchVerts[outputCount] = *currVert;
                    scratchUvs[outputCount] = *currUv;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = 1;
    }

    if ((clipRect->flags & 0x02) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        zClipUV *sourceUvs;
        zClipUV *destUvs;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceUvs = scratchUvs;
            destVerts = g_Clip_PolyVerts;
            destUvs = g_Clip_PolyUvs;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceUvs = g_Clip_PolyUvs;
            destVerts = scratchVerts;
            destUvs = scratchUvs;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                zClipUV *prevUv = &sourceUvs[prevIndex];
                zClipUV *currUv = &sourceUvs[i];

                if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x < clipRect->xMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                } else if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x >= clipRect->xMaxAlt
                ) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x04) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        zClipUV *sourceUvs;
        zClipUV *destUvs;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceUvs = scratchUvs;
            destVerts = g_Clip_PolyVerts;
            destUvs = g_Clip_PolyUvs;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceUvs = g_Clip_PolyUvs;
            destVerts = scratchVerts;
            destUvs = scratchUvs;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                zClipUV *prevUv = &sourceUvs[prevIndex];
                zClipUV *currUv = &sourceUvs[i];

                if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y >= clipRect->yMin
                ) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                } else if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y < clipRect->yMin
                ) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x08) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        zClipUV *sourceUvs;
        zClipUV *destUvs;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceUvs = scratchUvs;
            destVerts = g_Clip_PolyVerts;
            destUvs = g_Clip_PolyUvs;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceUvs = g_Clip_PolyUvs;
            destVerts = scratchVerts;
            destUvs = scratchUvs;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                zClipUV *prevUv = &sourceUvs[prevIndex];
                zClipUV *currUv = &sourceUvs[i];

                if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y < clipRect->yMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                } else if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y >= clipRect->yMaxAlt
                ) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        parity = (parity + 1) % 2;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (parity == 1) {
        memcpy(
            g_Clip_PolyVerts,
            scratchVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
        memcpy(
            g_Clip_PolyUvs,
            scratchUvs,
            (size_t)(outputCount) * sizeof(zClipUV)
        );
    }
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47dfb0: zClipRect::ClipPoly_NoUV_WithAttr0_Alt
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip the active polygon vertex and first-attribute streams against enabled XY bounds.
 */
int __fastcall ClipPoly_NoUV_WithAttr0_Alt(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVerts[kClipBufferCapacity];
    float scratchAttrs[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert *currVert = &g_Clip_PolyVerts[i];
                float prevAttr = g_Clip_PolyAttr0[prevIndex];
                float currAttr = g_Clip_PolyAttr0[i];

                if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x >= clipRect->xMin
                ) {
                    scratchVerts[outputCount] = *currVert;
                    scratchAttrs[outputCount] = currAttr;
                    ++outputCount;
                } else if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x < clipRect->xMin
                ) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchAttrs[outputCount] =
                        prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchAttrs[outputCount] =
                        prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;

                    scratchVerts[outputCount] = *currVert;
                    scratchAttrs[outputCount] = currAttr;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = 1;
    }

    if ((clipRect->flags & 0x02) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        float *sourceAttrs;
        float *destAttrs;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceAttrs = scratchAttrs;
            destVerts = g_Clip_PolyVerts;
            destAttrs = g_Clip_PolyAttr0;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttrs = g_Clip_PolyAttr0;
            destVerts = scratchVerts;
            destAttrs = scratchAttrs;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                float prevAttr = sourceAttrs[prevIndex];
                float currAttr = sourceAttrs[i];

                if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x < clipRect->xMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    destAttrs[outputCount] = currAttr;
                    ++outputCount;
                } else if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x >= clipRect->xMaxAlt
                ) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttrs[outputCount] =
                        prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttrs[outputCount] =
                        prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destAttrs[outputCount] = currAttr;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x04) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        float *sourceAttrs;
        float *destAttrs;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceAttrs = scratchAttrs;
            destVerts = g_Clip_PolyVerts;
            destAttrs = g_Clip_PolyAttr0;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttrs = g_Clip_PolyAttr0;
            destVerts = scratchVerts;
            destAttrs = scratchAttrs;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                float prevAttr = sourceAttrs[prevIndex];
                float currAttr = sourceAttrs[i];

                if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y >= clipRect->yMin
                ) {
                    destVerts[outputCount] = *currVert;
                    destAttrs[outputCount] = currAttr;
                    ++outputCount;
                } else if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y < clipRect->yMin
                ) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttrs[outputCount] =
                        prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttrs[outputCount] =
                        prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destAttrs[outputCount] = currAttr;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x08) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        float *sourceAttrs;
        float *destAttrs;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceAttrs = scratchAttrs;
            destVerts = g_Clip_PolyVerts;
            destAttrs = g_Clip_PolyAttr0;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttrs = g_Clip_PolyAttr0;
            destVerts = scratchVerts;
            destAttrs = scratchAttrs;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                float prevAttr = sourceAttrs[prevIndex];
                float currAttr = sourceAttrs[i];

                if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y < clipRect->yMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    destAttrs[outputCount] = currAttr;
                    ++outputCount;
                } else if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y >= clipRect->yMaxAlt
                ) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttrs[outputCount] =
                        prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttrs[outputCount] =
                        prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destAttrs[outputCount] = currAttr;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        parity = (parity + 1) % 2;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (parity == 1) {
        memcpy(
            g_Clip_PolyVerts,
            scratchVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
        memcpy(
            g_Clip_PolyAttr0,
            scratchAttrs,
            (size_t)(outputCount) * sizeof(float)
        );
    }
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47e900: zClipRect::ClipPolyZRange_WithAttr012
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip the scratch polygon vertex, UV, and three-attribute streams against the Z range.
 */
int __fastcall ClipPolyZRange_WithAttr012(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    const int count = *vertexCount;
    const int flags = clipRect->flags;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (int i = 0; i < count && allBeyondFar != 0; ++i) {
            if (g_Clip_PolyVertsScratch[i].z < clipRect->zMax) {
                allBeyondFar = 0;
            }
        }

        if (allBeyondFar != 0) {
            return 0;
        }
    }

    if ((flags & 0x10) == 0) {
        return 1;
    }

    int allInsideNear = 1;
    for (int i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    zClipVert clippedVerts[kClipBufferCapacity] = {0};
    zClipUV clippedUvs[kClipBufferCapacity] = {0};
    float clippedAttr0[kClipBufferCapacity] = {0};
    float clippedAttr1[kClipBufferCapacity] = {0};
    float clippedAttr2[kClipBufferCapacity] = {0};
    int outputCount = 0;

    if (count > 0) {
        zClipVert prevVert = g_Clip_PolyVertsScratch[count - 1];
        zClipUV prevUv = g_Clip_PolyUvs[count - 1];
        float prevAttr0 = g_Clip_PolyAttr0[count - 1];
        float prevAttr1 = g_Clip_PolyAttr1[count - 1];
        float prevAttr2 = g_Clip_PolyAttr2[count - 1];
        bool prevInside = IsInsideNear(
            prevVert,
            clipRect->zMin
        );

        for (int i = 0; i < count; ++i) {
            const zClipVert currVert = g_Clip_PolyVertsScratch[i];
            const zClipUV currUv = g_Clip_PolyUvs[i];
            const float currAttr0 = g_Clip_PolyAttr0[i];
            const float currAttr1 = g_Clip_PolyAttr1[i];
            const float currAttr2 = g_Clip_PolyAttr2[i];
            const bool currInside = IsInsideNear(
                currVert,
                clipRect->zMin
            );

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                AppendClippedWithAttr012(
                    clippedVerts,
                    clippedUvs,
                    clippedAttr0,
                    clippedAttr1,
                    clippedAttr2,
                    outputCount,
                    InterpolateVert(
                        prevVert,
                        currVert,
                        t,
                        clipRect->zMin
                    ),
                    InterpolateUv(
                        prevUv,
                        currUv,
                        t
                    ),
                    InterpolateFloat(
                        prevAttr0,
                        currAttr0,
                        t
                    ),
                    InterpolateFloat(
                        prevAttr1,
                        currAttr1,
                        t
                    ),
                    InterpolateFloat(prevAttr2, currAttr2, t)
                );
            }

            if (currInside) {
                AppendClippedWithAttr012(
                    clippedVerts,
                    clippedUvs,
                    clippedAttr0,
                    clippedAttr1,
                    clippedAttr2,
                    outputCount,
                    currVert,
                    currUv,
                    currAttr0,
                    currAttr1,
                    currAttr2
                );
            }

            prevVert = currVert;
            prevUv = currUv;
            prevAttr0 = currAttr0;
            prevAttr1 = currAttr1;
            prevAttr2 = currAttr2;
            prevInside = currInside;
        }
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(
        g_Clip_PolyVertsScratch,
        clippedVerts,
        (size_t)(outputCount) * sizeof(zClipVert)
    );
    memcpy(
        g_Clip_PolyUvs,
        clippedUvs,
        (size_t)(outputCount) * sizeof(zClipUV)
    );
    memcpy(
        g_Clip_PolyAttr0,
        clippedAttr0,
        (size_t)(outputCount) * sizeof(float)
    );
    memcpy(
        g_Clip_PolyAttr2,
        clippedAttr2,
        (size_t)(outputCount) * sizeof(float)
    );
    memcpy(
        g_Clip_PolyAttr1,
        clippedAttr1,
        (size_t)(outputCount) * sizeof(float)
    );
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x47efd0: zClipRect::ClipPoly_WithAttr012
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Purpose: Clip active polygon vertex, UV, and three-attribute streams against enabled XY bounds.
 */
int __fastcall ClipPoly_WithAttr012(
    zClipRectPartial *clipRect,
    int *vertexCount
) {
    zClipVert scratchVerts[kClipBufferCapacity];
    zClipUV scratchUvs[kClipBufferCapacity];
    float scratchAttr0[kClipBufferCapacity];
    float scratchAttr1[kClipBufferCapacity];
    float scratchAttr2[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert *currVert = &g_Clip_PolyVerts[i];
                zClipUV *prevUv = &g_Clip_PolyUvs[prevIndex];
                zClipUV *currUv = &g_Clip_PolyUvs[i];
                float prevAttr0 = g_Clip_PolyAttr0[prevIndex];
                float prevAttr1 = g_Clip_PolyAttr1[prevIndex];
                float prevAttr2 = g_Clip_PolyAttr2[prevIndex];
                float currAttr0 = g_Clip_PolyAttr0[i];
                float currAttr1 = g_Clip_PolyAttr1[i];
                float currAttr2 = g_Clip_PolyAttr2[i];

                if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x >= clipRect->xMin
                ) {
                    scratchVerts[outputCount] = *currVert;
                    scratchUvs[outputCount] = *currUv;
                    scratchAttr0[outputCount] = currAttr0;
                    scratchAttr1[outputCount] = currAttr1;
                    scratchAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (
                    prevVert->x >= clipRect->xMin
                    && currVert->x < clipRect->xMin
                ) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    scratchUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    scratchAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    scratchAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    scratchAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t =
                        (clipRect->xMin - prevVert->x) /
                        (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    scratchUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    scratchAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    scratchAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    scratchAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;

                    scratchVerts[outputCount] = *currVert;
                    scratchUvs[outputCount] = *currUv;
                    scratchAttr0[outputCount] = currAttr0;
                    scratchAttr1[outputCount] = currAttr1;
                    scratchAttr2[outputCount] = currAttr2;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = 1;
    }

    if ((clipRect->flags & 0x02) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        zClipUV *sourceUvs;
        zClipUV *destUvs;
        float *sourceAttr0;
        float *sourceAttr1;
        float *sourceAttr2;
        float *destAttr0;
        float *destAttr1;
        float *destAttr2;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceUvs = scratchUvs;
            sourceAttr0 = scratchAttr0;
            sourceAttr1 = scratchAttr1;
            sourceAttr2 = scratchAttr2;
            destVerts = g_Clip_PolyVerts;
            destUvs = g_Clip_PolyUvs;
            destAttr0 = g_Clip_PolyAttr0;
            destAttr1 = g_Clip_PolyAttr1;
            destAttr2 = g_Clip_PolyAttr2;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceUvs = g_Clip_PolyUvs;
            sourceAttr0 = g_Clip_PolyAttr0;
            sourceAttr1 = g_Clip_PolyAttr1;
            sourceAttr2 = g_Clip_PolyAttr2;
            destVerts = scratchVerts;
            destUvs = scratchUvs;
            destAttr0 = scratchAttr0;
            destAttr1 = scratchAttr1;
            destAttr2 = scratchAttr2;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                zClipUV *prevUv = &sourceUvs[prevIndex];
                zClipUV *currUv = &sourceUvs[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x < clipRect->xMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (
                    prevVert->x < clipRect->xMaxAlt
                    && currVert->x >= clipRect->xMaxAlt
                ) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t =
                        (clipRect->xMaxAlt - prevVert->x) /
                        (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y =
                        prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x04) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        zClipUV *sourceUvs;
        zClipUV *destUvs;
        float *sourceAttr0;
        float *sourceAttr1;
        float *sourceAttr2;
        float *destAttr0;
        float *destAttr1;
        float *destAttr2;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceUvs = scratchUvs;
            sourceAttr0 = scratchAttr0;
            sourceAttr1 = scratchAttr1;
            sourceAttr2 = scratchAttr2;
            destVerts = g_Clip_PolyVerts;
            destUvs = g_Clip_PolyUvs;
            destAttr0 = g_Clip_PolyAttr0;
            destAttr1 = g_Clip_PolyAttr1;
            destAttr2 = g_Clip_PolyAttr2;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceUvs = g_Clip_PolyUvs;
            sourceAttr0 = g_Clip_PolyAttr0;
            sourceAttr1 = g_Clip_PolyAttr1;
            sourceAttr2 = g_Clip_PolyAttr2;
            destVerts = scratchVerts;
            destUvs = scratchUvs;
            destAttr0 = scratchAttr0;
            destAttr1 = scratchAttr1;
            destAttr2 = scratchAttr2;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                zClipUV *prevUv = &sourceUvs[prevIndex];
                zClipUV *currUv = &sourceUvs[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y >= clipRect->yMin
                ) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (
                    prevVert->y >= clipRect->yMin
                    && currVert->y < clipRect->yMin
                ) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t =
                        (clipRect->yMin - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x08) != 0) {
        zClipVert *sourceVerts;
        zClipVert *destVerts;
        zClipUV *sourceUvs;
        zClipUV *destUvs;
        float *sourceAttr0;
        float *sourceAttr1;
        float *sourceAttr2;
        float *destAttr0;
        float *destAttr1;
        float *destAttr2;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            sourceUvs = scratchUvs;
            sourceAttr0 = scratchAttr0;
            sourceAttr1 = scratchAttr1;
            sourceAttr2 = scratchAttr2;
            destVerts = g_Clip_PolyVerts;
            destUvs = g_Clip_PolyUvs;
            destAttr0 = g_Clip_PolyAttr0;
            destAttr1 = g_Clip_PolyAttr1;
            destAttr2 = g_Clip_PolyAttr2;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            sourceUvs = g_Clip_PolyUvs;
            sourceAttr0 = g_Clip_PolyAttr0;
            sourceAttr1 = g_Clip_PolyAttr1;
            sourceAttr2 = g_Clip_PolyAttr2;
            destVerts = scratchVerts;
            destUvs = scratchUvs;
            destAttr0 = scratchAttr0;
            destAttr1 = scratchAttr1;
            destAttr2 = scratchAttr2;
        }

        outputCount = 0;
        const int count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            for (int i = 0; i < count; ++i) {
                zClipVert *prevVert = &sourceVerts[prevIndex];
                zClipVert *currVert = &sourceVerts[i];
                zClipUV *prevUv = &sourceUvs[prevIndex];
                zClipUV *currUv = &sourceUvs[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y < clipRect->yMaxAlt
                ) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (
                    prevVert->y < clipRect->yMaxAlt
                    && currVert->y >= clipRect->yMaxAlt
                ) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t =
                        (clipRect->yMaxAlt - prevVert->y) /
                        (currVert->y - prevVert->y);
                    destVerts[outputCount].x =
                        prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z =
                        prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u =
                        prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v =
                        prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] =
                        prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] =
                        prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] =
                        prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;

                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                }

                prevIndex = i;
            }
        }

        parity = (parity + 1) % 2;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (parity == 1) {
        memcpy(
            g_Clip_PolyVerts,
            scratchVerts,
            (size_t)(outputCount) * sizeof(zClipVert)
        );
        memcpy(
            g_Clip_PolyUvs,
            scratchUvs,
            (size_t)(outputCount) * sizeof(zClipUV)
        );
        memcpy(
            g_Clip_PolyAttr0,
            scratchAttr0,
            (size_t)(outputCount) * sizeof(float)
        );
        memcpy(
            g_Clip_PolyAttr2,
            scratchAttr2,
            (size_t)(outputCount) * sizeof(float)
        );
        memcpy(
            g_Clip_PolyAttr1,
            scratchAttr1,
            (size_t)(outputCount) * sizeof(float)
        );
    }
    return 1;
}
} // namespace zClipRect

namespace zClipRect {
/**
 * Reimplements 0x4803b0: zClipRect::TrivialRejectPolyXY
 * Source: D:\Proj\Battlesport\zClip.cpp
 * Evidence: Current BN/status show this as a leaf zClipRect namespace helper over g_Clip_PolyVerts.
 * Purpose: Reject polygons whose active vertices all fall outside one enabled XY clip plane.
 */
int __fastcall TrivialRejectPolyXY(
    zClipRectPartial *clipRect,
    int vertexCount
) {
    const int flags = clipRect->flags;
    if (flags == 0) {
        return 1;
    }

    if ((flags & 0x01) != 0) {
        int allOutside = 1;
        for (int i = 0; i < vertexCount && allOutside != 0; ++i) {
            if (g_Clip_PolyVerts[i].x >= clipRect->xMin) {
                allOutside = 0;
            }
        }
        if (allOutside != 0) {
            return 0;
        }
    }

    if ((flags & 0x02) != 0) {
        int allOutside = 1;
        for (int i = 0; i < vertexCount && allOutside != 0; ++i) {
            if (g_Clip_PolyVerts[i].x < clipRect->xMax) {
                allOutside = 0;
            }
        }
        if (allOutside != 0) {
            return 0;
        }
    }

    if ((flags & 0x04) != 0) {
        int allOutside = 1;
        for (int i = 0; i < vertexCount && allOutside != 0; ++i) {
            if (g_Clip_PolyVerts[i].y >= clipRect->yMin) {
                allOutside = 0;
            }
        }
        if (allOutside != 0) {
            return 0;
        }
    }

    if ((flags & 0x08) != 0) {
        int allOutside = 1;
        for (int i = 0; i < vertexCount && allOutside != 0; ++i) {
            if (g_Clip_PolyVerts[i].y < clipRect->yMax) {
                allOutside = 0;
            }
        }
        if (allOutside != 0) {
            return 0;
        }
    }

    return 1;
}
} // namespace zClipRect

namespace zModel {
/**
     * Reimplements 0x4804c0: zModel::UpdateSmallPolyRejectThresholds
     * (Battlesport/zModel/gmod_scene.c).
     *
     * Purpose: cache the doubled and twenty-times small-polygon reject-area
     * thresholds used by projected model clipping.
     */
    void __stdcall UpdateSmallPolyRejectThresholds(float baseRejectArea) {
        const float doubledArea = baseRejectArea + baseRejectArea;
        gModel_SmallPolyRejectArea2x = doubledArea;
        gModel_SmallPolyRejectArea20x = doubledArea * 10.0f;
    }
} // namespace zModel

namespace zReader {
/**
 * Reimplements 0x4804e0: zReader::FindGlobalStringPrefixIndex
 * (Battlesport/zUtil/zrdr_global.c).
 *
 * Purpose: find the global string-table prefix that matches the start of a
 * reader token and is followed by the token end or whitespace.
 */
int __fastcall FindGlobalStringPrefixIndex(
    const char *text
) {
    if (text == 0) {
        return -1;
    }

    for (int index = 0; index < g_zRndr_GlobalStringCount; ++index) {
        const char *const prefix = g_zRndr_GlobalStringTable[index];
        const size_t prefixLength = strlen(prefix);
        if (strlen(text) < prefixLength) {
            continue;
        }

        const int nextChar = text[prefixLength];
        /* Original zrdr_global.c used the VC5 C ctype macro shape; the C++
           header would call imported isspace instead of touching these CRT
           globals. */
        if (
            nextChar != '\0'
            && (MB_CUR_MAX > 1
                    ? _isctype(
                          nextChar,
                          _SPACE
                      )
                    : (_pctype[nextChar] & _SPACE)) == 0
        ) {
            continue;
        }

        if (_strnicmp(
            text,
            prefix,
            prefixLength
        ) == 0) {
            return index;
        }
    }

    return -1;
}
} // namespace zReader
