// zModel compilation unit between gmod_draw.c and gmod_matl.c, inferred from the
// retail object boundary [0x479ce0, 0x4804e0): its .rdata pooled constants
// [0x4d2a80, 0x4d2aac) repeat 1.0f and 2.0f that gmod_draw.c pools
// separately. Original filename unresolved; gmod_clip.c is a provisional name
// (2026-10-02).

#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zclass.h"
#include <string.h>

enum { kClipBufferCapacity = 0x40 };

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zvideo-setactiveviewcontext
 * @recoil-artifact defines .text recoil:function:0x479ce0: zVideoSetActiveViewContext.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVideo.cpp.
 * Data evidence: BN stores the supplied camera context into the projection
 * context cache at 0x576214, updates gClipRect_Primary at 0x576218, and writes
 * the project clip floats at 0x57623c..0x576248 before zMath projection setup.
 * Purpose: provide the recovered zVideoSetActiveViewContext behavior.
 */
void __fastcall zVideoSetActiveViewContext(CZCameraDataPartial* viewContext)
{
    int origin[2];
    int size[2];
    float left;
    float top;
    float right;
    float bottom;
    int fovXBits;
    int fovYBits;
    g_zVideo_pActiveProjectionViewContext = viewContext;

    if (g_zVideo_pActiveProjectionViewContext->nearClip < 1.0) {
        g_zVideo_pActiveProjectionViewContext->nearClip = 1.0f;
    }

    gClipRect_Primary.zMin
        = g_zVideo_pActiveProjectionViewContext->nearClip + g_zVideo_pActiveProjectionViewContext->nearClip;
    if (g_zVideo_ActiveRendererPath == 0) {
        SetQuadBatchDepthAndRhw(1.0f / gClipRect_Primary.zMin);
    }

    gClipRect_Primary.zMax = g_zVideo_pActiveProjectionViewContext->farClip;

    if (gwWindowGetSize(g_zVideo_pActiveProjectionViewContext->windowNode, &origin[0], &origin[1]) != 0) {
        origin[0] = 0;
        origin[1] = 0;
    }

    if (gwWindowGetResolution(g_zVideo_pActiveProjectionViewContext->windowNode, &size[0], &size[1]) != 0) {
        size[0] = GetPrimarySurfaceWidth();
        size[1] = GetPrimarySurfaceHeight();
    }

    if (g_zVideo_ActiveRendererPath != 0) {
        float rightWithSlop;
        float bottomWithSlop;
        left = (float)(origin[0]);
        right = (float)(origin[0] + size[0]);
        rightWithSlop = right + 0.00100000005f;
        top = (float)(origin[1]);
        bottom = (float)(origin[1] + size[1]);
        bottomWithSlop = bottom + 0.00100000005f;
        gClipRect_Primary.xMin = left;
        gClipRect_Primary.xMax = rightWithSlop;
        gClipRect_Primary.xMaxAlt = rightWithSlop;
        gClipRect_Primary.yMin = top;
        gClipRect_Primary.yMax = bottomWithSlop;
        gClipRect_Primary.yMaxAlt = bottomWithSlop;
    } else {
        left = (float)(origin[0]);
        top = (float)(origin[1]);
        right = (float)(origin[0] + size[0]);
        bottom = (float)(origin[1] + size[1]);
        gClipRect_Primary.xMin = left + 0.5f - 0.999000013f;
        gClipRect_Primary.xMax = right + 1.49900007f;
        gClipRect_Primary.xMaxAlt = right + 0.5f - 0.00100000005f;
        gClipRect_Primary.yMin = top + 0.5f - 0.999000013f;
        gClipRect_Primary.yMax = bottom + 1.49900007f;
        gClipRect_Primary.yMaxAlt = bottom + 0.5f - 0.00100000005f;
    }

    g_zVideo_ProjectClipLeft = left;
    g_zVideo_ProjectClipTop = top;
    g_zVideo_ProjectClipRight = right - 0.00100000005f;
    g_zVideo_ProjectClipBottom = bottom - 0.00100000005f;

    zMathSetupProjection(
        left,
        top,
        (float)(size[0]) * 0.5f,
        (float)(size[1]) * 0.5f,
        g_zVideo_pActiveProjectionViewContext->viewportScaleX,
        g_zVideo_pActiveProjectionViewContext->viewportScaleY,
        g_zVideo_pActiveProjectionViewContext->nearClip,
        g_zVideo_pActiveProjectionViewContext->farClip
    );

    memcpy(&fovXBits, &g_zVideo_pActiveProjectionViewContext->fovX, sizeof(fovXBits));
    memcpy(&fovYBits, &g_zVideo_pActiveProjectionViewContext->fovY, sizeof(fovYBits));
    zMathSetScreenSize(fovXBits, fovYBits);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zclipalt-settargetrect
 * @recoil-artifact defines .text recoil:function:0x479f90: zClipAlt::SetTargetRect
 * @recoil-match byte
 *
 * Purpose: configure the alternate clipping rectangle and source-to-target
 * coordinate remap scale and bias.
 */
void __fastcall SetTargetRect(const zClipAltFloatRect* rect, int replicate)
{
    float primaryOriginX;
    float primaryOriginY;
    gClipRect_Alt.flags = 0x0f;
    gClipRect_Alt.xMin = rect->left;
    gClipRect_Alt.xMax = rect->right;
    gClipRect_Alt.yMin = rect->top;
    gClipRect_Alt.yMaxAlt = gClipRect_Alt.yMax = rect->bottom;
    gClipRect_Alt.xMaxAlt = gClipRect_Alt.xMax;

    g_zClipAlt_RemapOffsetX = rect->left - g_zClipAlt_SourceLeft;
    g_zClipAlt_RemapOffsetY = rect->top - g_zClipAlt_SourceTop;
    g_zClipAlt_RemapScaleX = g_zClipAlt_SourceWidth / (rect->right - rect->left);
    g_zClipAlt_RemapScaleY = g_zClipAlt_SourceHeight / (rect->bottom - rect->top);

    primaryOriginX = gClipRect_Primary.xMin;
    primaryOriginY = gClipRect_Primary.yMin;
    if (replicate != 0) {
        primaryOriginX *= 0.5f;
        primaryOriginY *= 0.5f;
    }

    if (g_zClipAlt_BiasIncludesPrimaryOrigin != 0) {
        g_zClipAlt_RemapBiasX = g_zClipAlt_SourceLeft - gClipRect_Alt.xMin * g_zClipAlt_RemapScaleX + primaryOriginX;
        g_zClipAlt_RemapBiasY = g_zClipAlt_SourceTop - gClipRect_Alt.yMin * g_zClipAlt_RemapScaleY + primaryOriginY;
    } else {
        g_zClipAlt_RemapBiasX = g_zClipAlt_SourceLeft - gClipRect_Alt.xMin * g_zClipAlt_RemapScaleX;
        g_zClipAlt_RemapBiasY = g_zClipAlt_SourceTop - gClipRect_Alt.yMin * g_zClipAlt_RemapScaleY;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zvideo-updateprojectionstatefromcameradata
 * @recoil-artifact defines .text recoil:function:0x47a0c0: zVideoUpdateProjectionStateFromCameraData.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/zVideo/zVideo.cpp.
 * Purpose: provide the recovered zVideoUpdateProjectionStateFromCameraData behavior.
 */
void __fastcall zVideoUpdateProjectionStateFromCameraData(CZCameraDataPartial* cameraData)
{
    zMat4x3 slotBuffer;
    zMat4x3 yawSlotBuffer;
    MatStackPushPtr((float*)(&slotBuffer));
    MatLoadIdentity();

    MatStackPushAndCloneParent((float*)(&yawSlotBuffer));
    cameraData->localFrustumLeftNormal.x = 1.0f;
    cameraData->localFrustumLeftNormal.y = 0.0f;
    cameraData->localFrustumLeftNormal.z = 0.0f;
    MatRotateY(cameraData->frustumYaw);
    zMathVec3ArrayTransformDirection(&cameraData->localFrustumLeftNormal, 1);
    MatStackPopPtr();

    cameraData->localFrustumRightNormal.x = -cameraData->localFrustumLeftNormal.x;
    cameraData->localFrustumRightNormal.y = cameraData->localFrustumLeftNormal.y;
    cameraData->localFrustumRightNormal.z = cameraData->localFrustumLeftNormal.z;

    cameraData->localFrustumBottomNormal.x = 0.0f;
    cameraData->localFrustumBottomNormal.y = -1.0f;
    cameraData->localFrustumBottomNormal.z = 0.0f;
    MatRotateX(cameraData->frustumPitch);
    zMathVec3ArrayTransformDirection(&cameraData->localFrustumBottomNormal, 1);
    MatStackPopPtr();

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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zclipalt-buildfrustumplanes
 * @recoil-artifact defines .text recoil:function:0x47a1d0: zClipAltBuildFrustumPlanes
 * @recoil-match byte
 *
 * Purpose: transform the camera's local frustum normals into world-space
 * clipping planes for the alternate clipping pass.
 */
void __fastcall zClipAltBuildFrustumPlanes(CZCameraDataPartial* cameraData)
{
    MatStackPushPtr(cameraData->worldTransform);
    zMathMatTransformDirectionBatch(&cameraData->localFrustumLeftNormal, cameraData->worldFrustumNormals, 6);
    MatStackPopPtr();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippolyzrange-nouv
 * @recoil-artifact defines .text recoil:function:0x47a200: zClipRect::ClipPolyZRange_NoUV
 *
 *
 * Purpose: Clip the scratch polygon vertex stream against the configured Z range without attributes.
 */
int __fastcall ClipPolyZRange_NoUV(zClipRectPartial* clipRect, int* vertexCount)
{
    zVec3 clippedVerts[kClipBufferCapacity];
    int outputCount;
    int edgeIndex;
    zClipRectPartial* rect = clipRect;
    int* count = vertexCount;
    const int flags = rect->flags;
    int allInsideNear;
    int i;
    int prevIndex;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (i = 0; i < *count && allBeyondFar != 0; ++i) {
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

    allInsideNear = 1;
    for (i = 0; i < *count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < rect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return *count >= 3 ? 1 : 0;
    }

    outputCount = 0;
    edgeIndex = 0;

    prevIndex = *count - 1;
    for (; edgeIndex < *count; ++edgeIndex) {
        const zVec3* prevVert = &g_Clip_PolyVertsScratch[prevIndex];
        const zVec3* currVert = &g_Clip_PolyVertsScratch[edgeIndex];
        if (prevVert->z >= rect->zMin && currVert->z >= rect->zMin) {
            clippedVerts[outputCount] = *currVert;
            ++outputCount;
        } else if (prevVert->z >= rect->zMin && currVert->z < rect->zMin) {
            const float t = (rect->zMin - prevVert->z) / (currVert->z - prevVert->z);
            clippedVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
            clippedVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
            clippedVerts[outputCount].z = rect->zMin;
            ++outputCount;
        } else if (currVert->z >= rect->zMin) {
            const float t = (rect->zMin - prevVert->z) / (currVert->z - prevVert->z);
            clippedVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
            clippedVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
            clippedVerts[outputCount].z = rect->zMin;
            ++outputCount;
            clippedVerts[outputCount] = *currVert;
            ++outputCount;
        }

        prevIndex = edgeIndex;
    }

    *count = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    memcpy(g_Clip_PolyVertsScratch, clippedVerts, (size_t)(outputCount) * sizeof(zVec3));
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippolyzrange-nouv-withattribs
 * @recoil-artifact defines .text recoil:function:0x47a4e0: zClipRect::ClipPolyZRange_NoUV_WithAttribs
 *
 *
 * Purpose: Clip the scratch polygon vertex stream against the configured Z range while preserving three attributes.
 */
int __fastcall ClipPolyZRange_NoUV_WithAttribs(zClipRectPartial* clipRect, int* vertexCount)
{
    const int count = *vertexCount;
    const int flags = clipRect->flags;
    int allInsideNear;
    int i;
    zVec3 clippedVerts[kClipBufferCapacity] = { 0 };
    float clippedAttr0[kClipBufferCapacity] = { 0 };
    float clippedAttr1[kClipBufferCapacity] = { 0 };
    float clippedAttr2[kClipBufferCapacity] = { 0 };
    int outputCount;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (i = 0; i < count && allBeyondFar != 0; ++i) {
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

    allInsideNear = 1;
    for (i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    outputCount = 0;

    if (count > 0) {
        zVec3 prevVert = g_Clip_PolyVertsScratch[count - 1];
        float prevAttr0 = g_Clip_PolyAttr0[count - 1];
        float prevAttr1 = g_Clip_PolyAttr1[count - 1];
        float prevAttr2 = g_Clip_PolyAttr2[count - 1];
        int prevInside = prevVert.z >= clipRect->zMin;

        for (i = 0; i < count; ++i) {
            const zVec3 currVert = g_Clip_PolyVertsScratch[i];
            const float currAttr0 = g_Clip_PolyAttr0[i];
            const float currAttr1 = g_Clip_PolyAttr1[i];
            const float currAttr2 = g_Clip_PolyAttr2[i];
            const int currInside = currVert.z >= clipRect->zMin;

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                zVec3 intersection = { 0 };
                intersection.x = prevVert.x + (currVert.x - prevVert.x) * t;
                intersection.y = prevVert.y + (currVert.y - prevVert.y) * t;
                intersection.z = clipRect->zMin;
                if (outputCount < kClipBufferCapacity) {
                    clippedVerts[outputCount] = intersection;
                    clippedAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    clippedAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    clippedAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                }
            }

            if (currInside && outputCount < kClipBufferCapacity) {
                clippedVerts[outputCount] = currVert;
                clippedAttr0[outputCount] = currAttr0;
                clippedAttr1[outputCount] = currAttr1;
                clippedAttr2[outputCount] = currAttr2;
                ++outputCount;
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

    memcpy(g_Clip_PolyVertsScratch, clippedVerts, (size_t)(outputCount) * sizeof(zVec3));
    memcpy(g_Clip_PolyAttr0, clippedAttr0, (size_t)(outputCount) * sizeof(float));
    memcpy(g_Clip_PolyAttr1, clippedAttr1, (size_t)(outputCount) * sizeof(float));
    memcpy(g_Clip_PolyAttr2, clippedAttr2, (size_t)(outputCount) * sizeof(float));
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippolynearz
 * @recoil-artifact defines .text recoil:function:0x47aa80: zClipRect::ClipPolyNearZ
 *
 *
 * Purpose: Clip the scratch polygon vertex and UV streams against the configured near Z plane.
 */
int __fastcall ClipPolyNearZ(zClipRectPartial* clipRect, int* vertexCount)
{
    const int flags = clipRect->flags;
    int i;
    int allInsideNear;
    zVec3 clippedVerts[kClipBufferCapacity];
    zClipUV clippedUvs[kClipBufferCapacity];
    zVec3* outVert;
    zClipUV* outUv;
    int outputCount;
    int prevIndex;

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

    allInsideNear = 1;
    for (i = 0; i < *vertexCount && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return *vertexCount >= 3 ? 1 : 0;
    }

    outVert = clippedVerts;
    outUv = clippedUvs;
    outputCount = 0;
    prevIndex = *vertexCount - 1;

    for (i = 0; i < *vertexCount; ++i) {
        zVec3* prevVert = &g_Clip_PolyVertsScratch[prevIndex];
        zVec3* currVert = &g_Clip_PolyVertsScratch[i];
        zClipUV* prevUv = &g_Clip_PolyUvs[prevIndex];
        zClipUV* currUv = &g_Clip_PolyUvs[i];

        if (prevVert->z >= clipRect->zMin && currVert->z >= clipRect->zMin) {
            *outVert = *currVert;
            *outUv = *currUv;
            ++outVert;
            ++outUv;
            ++outputCount;
        } else if (prevVert->z >= clipRect->zMin && currVert->z < clipRect->zMin) {
            const float t = (clipRect->zMin - prevVert->z) / (currVert->z - prevVert->z);
            outVert->x = prevVert->x + (currVert->x - prevVert->x) * t;
            outVert->y = prevVert->y + (currVert->y - prevVert->y) * t;
            outVert->z = clipRect->zMin;
            outUv->u = prevUv->u + (currUv->u - prevUv->u) * t;
            outUv->v = prevUv->v + (currUv->v - prevUv->v) * t;
            ++outVert;
            ++outUv;
            ++outputCount;
        } else if (currVert->z >= clipRect->zMin) {
            const float t = (clipRect->zMin - prevVert->z) / (currVert->z - prevVert->z);
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

    memcpy(g_Clip_PolyVertsScratch, clippedVerts, (size_t)(outputCount) * sizeof(zVec3));
    memcpy(g_Clip_PolyUvs, clippedUvs, (size_t)(outputCount) * sizeof(zClipUV));
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippolynearz-withattr0
 * @recoil-artifact defines .text recoil:function:0x47af60: zClipRect::ClipPolyNearZ_WithAttr0
 *
 *
 * Purpose: Clip the scratch polygon vertex, UV, and first-attribute streams against near Z.
 */
int __fastcall ClipPolyNearZ_WithAttr0(zClipRectPartial* clipRect, int* vertexCount)
{
    const int count = *vertexCount;
    const int flags = clipRect->flags;
    int allInsideNear;
    int i;
    int result;
    zVec3 clippedVerts[kClipBufferCapacity] = { 0 };
    zClipUV clippedUvs[kClipBufferCapacity] = { 0 };
    float clippedAttrs[kClipBufferCapacity] = { 0 };
    int outputCount;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (i = 0; i < count && allBeyondFar != 0; ++i) {
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

    allInsideNear = 1;
    for (i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    result = 0;
    if (allInsideNear != result) {
        result = count >= 3;
        return result;
    }

    outputCount = 0;

    if (count > 0) {
        zVec3 prevVert = g_Clip_PolyVertsScratch[count - 1];
        zClipUV prevUv = g_Clip_PolyUvs[count - 1];
        float prevAttr = g_Clip_PolyAttr0[count - 1];
        int prevInside = prevVert.z >= clipRect->zMin;

        for (i = 0; i < count; ++i) {
            const zVec3 currVert = g_Clip_PolyVertsScratch[i];
            const zClipUV currUv = g_Clip_PolyUvs[i];
            const float currAttr = g_Clip_PolyAttr0[i];
            const int currInside = currVert.z >= clipRect->zMin;

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                zVec3 intersection = { 0 };
                zClipUV intersectionUv = { 0 };
                intersection.x = prevVert.x + (currVert.x - prevVert.x) * t;
                intersection.y = prevVert.y + (currVert.y - prevVert.y) * t;
                intersection.z = clipRect->zMin;
                intersectionUv.u = prevUv.u + (currUv.u - prevUv.u) * t;
                intersectionUv.v = prevUv.v + (currUv.v - prevUv.v) * t;
                if (outputCount < kClipBufferCapacity) {
                    clippedVerts[outputCount] = intersection;
                    clippedUvs[outputCount] = intersectionUv;
                    clippedAttrs[outputCount] = prevAttr + (currAttr - prevAttr) * t;
                    ++outputCount;
                }
            }

            if (currInside && outputCount < kClipBufferCapacity) {
                clippedVerts[outputCount] = currVert;
                clippedUvs[outputCount] = currUv;
                clippedAttrs[outputCount] = currAttr;
                ++outputCount;
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

    memcpy(g_Clip_PolyVertsScratch, clippedVerts, (size_t)(outputCount) * sizeof(zVec3));
    memcpy(g_Clip_PolyUvs, clippedUvs, (size_t)(outputCount) * sizeof(zClipUV));
    memcpy(g_Clip_PolyAttr0, clippedAttrs, (size_t)(outputCount) * sizeof(float));
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippoly-nouv-alt
 * @recoil-artifact defines .text recoil:function:0x47b540: zClipRect::ClipPoly_NoUV_Alt
 *
 *
 * Purpose: Clip the active polygon vertex stream against enabled XY bounds without UVs.
 */
int __fastcall ClipPoly_NoUV_Alt(zClipRectPartial* clipRect, int* vertexCount)
{
    zClipVert scratchVerts[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        int count;
        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert* currVert = &g_Clip_PolyVerts[i];

                if (prevVert->x >= clipRect->xMin && currVert->x >= clipRect->xMin) {
                    scratchVerts[outputCount] = *currVert;
                    ++outputCount;
                } else if (prevVert->x >= clipRect->xMin && currVert->x < clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        int count;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];

                if (prevVert->x < clipRect->xMaxAlt && currVert->x < clipRect->xMaxAlt) {
                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                } else if (prevVert->x < clipRect->xMaxAlt && currVert->x >= clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        int count;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];

                if (prevVert->y >= clipRect->yMin && currVert->y >= clipRect->yMin) {
                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                } else if (prevVert->y >= clipRect->yMin && currVert->y < clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        int count;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];

                if (prevVert->y < clipRect->yMaxAlt && currVert->y < clipRect->yMaxAlt) {
                    destVerts[outputCount] = *currVert;
                    ++outputCount;
                } else if (prevVert->y < clipRect->yMaxAlt && currVert->y >= clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
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
        memcpy(g_Clip_PolyVerts, scratchVerts, (size_t)(outputCount) * sizeof(zClipVert));
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippoly-nouv-withattr012-alt
 * @recoil-artifact defines .text recoil:function:0x47bd30: zClipRect::ClipPoly_NoUV_WithAttr012_Alt
 *
 *
 * Purpose: Clip active polygon vertex and three-attribute streams against enabled XY bounds.
 */
int __fastcall ClipPoly_NoUV_WithAttr012_Alt(zClipRectPartial* clipRect, int* vertexCount)
{
    zClipVert scratchVerts[kClipBufferCapacity];
    float scratchAttr0[kClipBufferCapacity];
    float scratchAttr1[kClipBufferCapacity];
    float scratchAttr2[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        int count;
        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert* currVert = &g_Clip_PolyVerts[i];
                float prevAttr0 = g_Clip_PolyAttr0[prevIndex];
                float prevAttr1 = g_Clip_PolyAttr1[prevIndex];
                float prevAttr2 = g_Clip_PolyAttr2[prevIndex];
                float currAttr0 = g_Clip_PolyAttr0[i];
                float currAttr1 = g_Clip_PolyAttr1[i];
                float currAttr2 = g_Clip_PolyAttr2[i];

                if (prevVert->x >= clipRect->xMin && currVert->x >= clipRect->xMin) {
                    scratchVerts[outputCount] = *currVert;
                    scratchAttr0[outputCount] = currAttr0;
                    scratchAttr1[outputCount] = currAttr1;
                    scratchAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (prevVert->x >= clipRect->xMin && currVert->x < clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    scratchAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    scratchAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    scratchAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    scratchAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        float* sourceAttr0;
        float* sourceAttr1;
        float* sourceAttr2;
        float* destAttr0;
        float* destAttr1;
        float* destAttr2;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (prevVert->x < clipRect->xMaxAlt && currVert->x < clipRect->xMaxAlt) {
                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (prevVert->x < clipRect->xMaxAlt && currVert->x >= clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        float* sourceAttr0;
        float* sourceAttr1;
        float* sourceAttr2;
        float* destAttr0;
        float* destAttr1;
        float* destAttr2;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (prevVert->y >= clipRect->yMin && currVert->y >= clipRect->yMin) {
                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (prevVert->y >= clipRect->yMin && currVert->y < clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        float* sourceAttr0;
        float* sourceAttr1;
        float* sourceAttr2;
        float* destAttr0;
        float* destAttr1;
        float* destAttr2;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (prevVert->y < clipRect->yMaxAlt && currVert->y < clipRect->yMaxAlt) {
                    destVerts[outputCount] = *currVert;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (prevVert->y < clipRect->yMaxAlt && currVert->y >= clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
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
        memcpy(g_Clip_PolyVerts, scratchVerts, (size_t)(outputCount) * sizeof(zClipVert));
        memcpy(g_Clip_PolyAttr0, scratchAttr0, (size_t)(outputCount) * sizeof(float));
        memcpy(g_Clip_PolyAttr1, scratchAttr1, (size_t)(outputCount) * sizeof(float));
        memcpy(g_Clip_PolyAttr2, scratchAttr2, (size_t)(outputCount) * sizeof(float));
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippoly-nouv
 * @recoil-artifact defines .text recoil:function:0x47cdc0: zClipRect::ClipPoly_NoUV
 *
 *
 * Purpose: Clip the primary polygon vertex stream against enabled XY bounds without UVs.
 */
int __fastcall ClipPoly_NoUV(zClipRectPartial* clipRect, int* vertexCount)
{
    zClipVert scratchVerts[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        int count;
        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert* currVert = &g_Clip_PolyVerts[i];

                if (prevVert->x >= clipRect->xMin && currVert->x >= clipRect->xMin) {
                    scratchVerts[outputCount].x = currVert->x;
                    scratchVerts[outputCount].y = currVert->y;
                    ++outputCount;
                } else if (prevVert->x >= clipRect->xMin && currVert->x < clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        int count;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];

                if (prevVert->x < clipRect->xMaxAlt && currVert->x < clipRect->xMaxAlt) {
                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                } else if (prevVert->x < clipRect->xMaxAlt && currVert->x >= clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        int count;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];

                if (prevVert->y >= clipRect->yMin && currVert->y >= clipRect->yMin) {
                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                } else if (prevVert->y >= clipRect->yMin && currVert->y < clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        int count;
        if (parity != 0) {
            sourceVerts = scratchVerts;
            destVerts = g_Clip_PolyVerts;
        } else {
            sourceVerts = g_Clip_PolyVerts;
            destVerts = scratchVerts;
        }

        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];

                if (prevVert->y < clipRect->yMaxAlt && currVert->y < clipRect->yMaxAlt) {
                    destVerts[outputCount].x = currVert->x;
                    destVerts[outputCount].y = currVert->y;
                    ++outputCount;
                } else if (prevVert->y < clipRect->yMaxAlt && currVert->y >= clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
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
        memcpy(g_Clip_PolyVerts, scratchVerts, (size_t)(outputCount) * sizeof(zClipVert));
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippoly
 * @recoil-artifact defines .text recoil:function:0x47d3f0: zClipRect::ClipPoly
 *
 *
 * Purpose: Clip active polygon vertex and UV streams against enabled XY bounds.
 */
int __fastcall ClipPoly(zClipRectPartial* clipRect, int* vertexCount)
{
    zClipVert scratchVerts[kClipBufferCapacity];
    zClipUV scratchUvs[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        int count;
        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert* currVert = &g_Clip_PolyVerts[i];
                zClipUV* prevUv = &g_Clip_PolyUvs[prevIndex];
                zClipUV* currUv = &g_Clip_PolyUvs[i];

                if (prevVert->x >= clipRect->xMin && currVert->x >= clipRect->xMin) {
                    scratchVerts[outputCount] = *currVert;
                    scratchUvs[outputCount] = *currUv;
                    ++outputCount;
                } else if (prevVert->x >= clipRect->xMin && currVert->x < clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    scratchUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    scratchUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        zClipUV* sourceUvs;
        zClipUV* destUvs;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                zClipUV* prevUv = &sourceUvs[prevIndex];
                zClipUV* currUv = &sourceUvs[i];

                if (prevVert->x < clipRect->xMaxAlt && currVert->x < clipRect->xMaxAlt) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                } else if (prevVert->x < clipRect->xMaxAlt && currVert->x >= clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        zClipUV* sourceUvs;
        zClipUV* destUvs;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                zClipUV* prevUv = &sourceUvs[prevIndex];
                zClipUV* currUv = &sourceUvs[i];

                if (prevVert->y >= clipRect->yMin && currVert->y >= clipRect->yMin) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                } else if (prevVert->y >= clipRect->yMin && currVert->y < clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        zClipUV* sourceUvs;
        zClipUV* destUvs;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                zClipUV* prevUv = &sourceUvs[prevIndex];
                zClipUV* currUv = &sourceUvs[i];

                if (prevVert->y < clipRect->yMaxAlt && currVert->y < clipRect->yMaxAlt) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    ++outputCount;
                } else if (prevVert->y < clipRect->yMaxAlt && currVert->y >= clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
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
        memcpy(g_Clip_PolyVerts, scratchVerts, (size_t)(outputCount) * sizeof(zClipVert));
        memcpy(g_Clip_PolyUvs, scratchUvs, (size_t)(outputCount) * sizeof(zClipUV));
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippoly-nouv-withattr0-alt
 * @recoil-artifact defines .text recoil:function:0x47dfb0: zClipRect::ClipPoly_NoUV_WithAttr0_Alt
 *
 *
 * Purpose: Clip the active polygon vertex and first-attribute streams against enabled XY bounds.
 */
int __fastcall ClipPoly_NoUV_WithAttr0_Alt(zClipRectPartial* clipRect, int* vertexCount)
{
    zClipVert scratchVerts[kClipBufferCapacity];
    float scratchAttrs[kClipBufferCapacity];
    zClipVert* sourceVerts;
    float* sourceAttrs;
    zClipVert* destVerts;
    float* destAttrs;
    int outputCount = 0;
    int parity = 0;
    int prevIndex;
    int i;
    float t;

    // Retail clips x/y and attr0 only; z of emitted vertices is left as found.
    if ((clipRect->flags & 0x01) != 0) {
        destVerts = scratchVerts;
        destAttrs = scratchAttrs;
        prevIndex = *vertexCount - 1;
        for (i = 0; i < *vertexCount; ++i) {
            if (g_Clip_PolyVerts[prevIndex].x >= clipRect->xMin && g_Clip_PolyVerts[i].x >= clipRect->xMin) {
                destVerts->x = g_Clip_PolyVerts[i].x;
                destVerts->y = g_Clip_PolyVerts[i].y;
                ++destVerts;
                *destAttrs = g_Clip_PolyAttr0[i];
                ++destAttrs;
                ++outputCount;
            } else if (g_Clip_PolyVerts[prevIndex].x < clipRect->xMin && g_Clip_PolyVerts[i].x < clipRect->xMin) {
                // Both outside: nothing is emitted.
            } else if (g_Clip_PolyVerts[prevIndex].x >= clipRect->xMin && g_Clip_PolyVerts[i].x < clipRect->xMin) {
                t = (clipRect->xMin - g_Clip_PolyVerts[prevIndex].x)
                    / (g_Clip_PolyVerts[i].x - g_Clip_PolyVerts[prevIndex].x);
                destVerts->x = clipRect->xMin;
                destVerts->y
                    = g_Clip_PolyVerts[prevIndex].y + (g_Clip_PolyVerts[i].y - g_Clip_PolyVerts[prevIndex].y) * t;
                ++destVerts;
                *destAttrs = g_Clip_PolyAttr0[prevIndex] + (g_Clip_PolyAttr0[i] - g_Clip_PolyAttr0[prevIndex]) * t;
                ++destAttrs;
                ++outputCount;
            } else if (g_Clip_PolyVerts[prevIndex].x < clipRect->xMin && g_Clip_PolyVerts[i].x >= clipRect->xMin) {
                t = (clipRect->xMin - g_Clip_PolyVerts[prevIndex].x)
                    / (g_Clip_PolyVerts[i].x - g_Clip_PolyVerts[prevIndex].x);
                destVerts->x = clipRect->xMin;
                destVerts->y
                    = g_Clip_PolyVerts[prevIndex].y + (g_Clip_PolyVerts[i].y - g_Clip_PolyVerts[prevIndex].y) * t;
                ++destVerts;
                *destAttrs = g_Clip_PolyAttr0[prevIndex] + (g_Clip_PolyAttr0[i] - g_Clip_PolyAttr0[prevIndex]) * t;
                ++destAttrs;
                ++outputCount;
                destVerts->x = g_Clip_PolyVerts[i].x;
                destVerts->y = g_Clip_PolyVerts[i].y;
                ++destVerts;
                *destAttrs = g_Clip_PolyAttr0[i];
                ++destAttrs;
                ++outputCount;
            }

            prevIndex = i;
        }

        *vertexCount = outputCount;
        parity = 1;
    }

    if ((clipRect->flags & 0x02) != 0) {
        if (parity == 0) {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttrs = g_Clip_PolyAttr0;
            destVerts = scratchVerts;
            destAttrs = scratchAttrs;
        } else {
            sourceVerts = scratchVerts;
            sourceAttrs = scratchAttrs;
            destVerts = g_Clip_PolyVerts;
            destAttrs = g_Clip_PolyAttr0;
        }

        outputCount = 0;
        prevIndex = *vertexCount - 1;
        for (i = 0; i < *vertexCount; ++i) {
            if (sourceVerts[prevIndex].x < clipRect->xMaxAlt && sourceVerts[i].x < clipRect->xMaxAlt) {
                destVerts->x = sourceVerts[i].x;
                destVerts->y = sourceVerts[i].y;
                ++destVerts;
                *destAttrs = sourceAttrs[i];
                ++destAttrs;
                ++outputCount;
            } else if (sourceVerts[prevIndex].x >= clipRect->xMaxAlt && sourceVerts[i].x >= clipRect->xMaxAlt) {
                // Both outside: nothing is emitted.
            } else if (sourceVerts[prevIndex].x < clipRect->xMaxAlt && sourceVerts[i].x >= clipRect->xMaxAlt) {
                t = (clipRect->xMaxAlt - sourceVerts[prevIndex].x) / (sourceVerts[i].x - sourceVerts[prevIndex].x);
                destVerts->x = clipRect->xMaxAlt;
                destVerts->y = sourceVerts[prevIndex].y + (sourceVerts[i].y - sourceVerts[prevIndex].y) * t;
                ++destVerts;
                *destAttrs = sourceAttrs[prevIndex] + (sourceAttrs[i] - sourceAttrs[prevIndex]) * t;
                ++destAttrs;
                ++outputCount;
            } else if (sourceVerts[prevIndex].x >= clipRect->xMaxAlt && sourceVerts[i].x < clipRect->xMaxAlt) {
                t = (clipRect->xMaxAlt - sourceVerts[prevIndex].x) / (sourceVerts[i].x - sourceVerts[prevIndex].x);
                destVerts->x = clipRect->xMaxAlt;
                destVerts->y = sourceVerts[prevIndex].y + (sourceVerts[i].y - sourceVerts[prevIndex].y) * t;
                ++destVerts;
                *destAttrs = sourceAttrs[prevIndex] + (sourceAttrs[i] - sourceAttrs[prevIndex]) * t;
                ++destAttrs;
                ++outputCount;
                destVerts->x = sourceVerts[i].x;
                destVerts->y = sourceVerts[i].y;
                ++destVerts;
                *destAttrs = sourceAttrs[i];
                ++destAttrs;
                ++outputCount;
            }

            prevIndex = i;
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x04) != 0) {
        if (parity == 0) {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttrs = g_Clip_PolyAttr0;
            destVerts = scratchVerts;
            destAttrs = scratchAttrs;
        } else {
            sourceVerts = scratchVerts;
            sourceAttrs = scratchAttrs;
            destVerts = g_Clip_PolyVerts;
            destAttrs = g_Clip_PolyAttr0;
        }

        outputCount = 0;
        prevIndex = *vertexCount - 1;
        for (i = 0; i < *vertexCount; ++i) {
            if (sourceVerts[prevIndex].y >= clipRect->yMin && sourceVerts[i].y >= clipRect->yMin) {
                destVerts->x = sourceVerts[i].x;
                destVerts->y = sourceVerts[i].y;
                ++destVerts;
                *destAttrs = sourceAttrs[i];
                ++destAttrs;
                ++outputCount;
            } else if (sourceVerts[prevIndex].y < clipRect->yMin && sourceVerts[i].y < clipRect->yMin) {
                // Both outside: nothing is emitted.
            } else if (sourceVerts[prevIndex].y >= clipRect->yMin && sourceVerts[i].y < clipRect->yMin) {
                t = (clipRect->yMin - sourceVerts[prevIndex].y) / (sourceVerts[i].y - sourceVerts[prevIndex].y);
                destVerts->y = clipRect->yMin;
                destVerts->x = sourceVerts[prevIndex].x + (sourceVerts[i].x - sourceVerts[prevIndex].x) * t;
                ++destVerts;
                *destAttrs = sourceAttrs[prevIndex] + (sourceAttrs[i] - sourceAttrs[prevIndex]) * t;
                ++destAttrs;
                ++outputCount;
            } else if (sourceVerts[prevIndex].y < clipRect->yMin && sourceVerts[i].y >= clipRect->yMin) {
                t = (clipRect->yMin - sourceVerts[prevIndex].y) / (sourceVerts[i].y - sourceVerts[prevIndex].y);
                destVerts->y = clipRect->yMin;
                destVerts->x = sourceVerts[prevIndex].x + (sourceVerts[i].x - sourceVerts[prevIndex].x) * t;
                ++destVerts;
                *destAttrs = sourceAttrs[prevIndex] + (sourceAttrs[i] - sourceAttrs[prevIndex]) * t;
                ++destAttrs;
                ++outputCount;
                destVerts->x = sourceVerts[i].x;
                destVerts->y = sourceVerts[i].y;
                ++destVerts;
                *destAttrs = sourceAttrs[i];
                ++destAttrs;
                ++outputCount;
            }

            prevIndex = i;
        }

        *vertexCount = outputCount;
        parity = (parity + 1) % 2;
    }

    if ((clipRect->flags & 0x08) != 0) {
        if (parity == 0) {
            sourceVerts = g_Clip_PolyVerts;
            sourceAttrs = g_Clip_PolyAttr0;
            destVerts = scratchVerts;
            destAttrs = scratchAttrs;
        } else {
            sourceVerts = scratchVerts;
            sourceAttrs = scratchAttrs;
            destVerts = g_Clip_PolyVerts;
            destAttrs = g_Clip_PolyAttr0;
        }

        outputCount = 0;
        prevIndex = *vertexCount - 1;
        for (i = 0; i < *vertexCount; ++i) {
            if (sourceVerts[prevIndex].y < clipRect->yMaxAlt && sourceVerts[i].y < clipRect->yMaxAlt) {
                destVerts->x = sourceVerts[i].x;
                destVerts->y = sourceVerts[i].y;
                ++destVerts;
                *destAttrs = sourceAttrs[i];
                ++destAttrs;
                ++outputCount;
            } else if (sourceVerts[prevIndex].y >= clipRect->yMaxAlt && sourceVerts[i].y >= clipRect->yMaxAlt) {
                // Both outside: nothing is emitted.
            } else if (sourceVerts[prevIndex].y < clipRect->yMaxAlt && sourceVerts[i].y >= clipRect->yMaxAlt) {
                t = (clipRect->yMaxAlt - sourceVerts[prevIndex].y) / (sourceVerts[i].y - sourceVerts[prevIndex].y);
                destVerts->y = clipRect->yMaxAlt;
                destVerts->x = sourceVerts[prevIndex].x + (sourceVerts[i].x - sourceVerts[prevIndex].x) * t;
                ++destVerts;
                *destAttrs = sourceAttrs[prevIndex] + (sourceAttrs[i] - sourceAttrs[prevIndex]) * t;
                ++destAttrs;
                ++outputCount;
            } else if (sourceVerts[prevIndex].y >= clipRect->yMaxAlt && sourceVerts[i].y < clipRect->yMaxAlt) {
                t = (clipRect->yMaxAlt - sourceVerts[prevIndex].y) / (sourceVerts[i].y - sourceVerts[prevIndex].y);
                destVerts->y = clipRect->yMaxAlt;
                destVerts->x = sourceVerts[prevIndex].x + (sourceVerts[i].x - sourceVerts[prevIndex].x) * t;
                ++destVerts;
                *destAttrs = sourceAttrs[prevIndex] + (sourceAttrs[i] - sourceAttrs[prevIndex]) * t;
                ++destAttrs;
                ++outputCount;
                destVerts->x = sourceVerts[i].x;
                destVerts->y = sourceVerts[i].y;
                ++destVerts;
                *destAttrs = sourceAttrs[i];
                ++destAttrs;
                ++outputCount;
            }

            prevIndex = i;
        }

        parity = (parity + 1) % 2;
    }

    *vertexCount = outputCount;
    if (outputCount < 3) {
        return 0;
    }

    if (parity == 1) {
        memcpy(g_Clip_PolyVerts, scratchVerts, (size_t)(outputCount) * sizeof(zClipVert));
        memcpy(g_Clip_PolyAttr0, scratchAttrs, (size_t)(*vertexCount) * sizeof(float));
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippolyzrange-withattr012
 * @recoil-artifact defines .text recoil:function:0x47e900: zClipRect::ClipPolyZRange_WithAttr012
 *
 *
 * Purpose: Clip the scratch polygon vertex, UV, and three-attribute streams against the Z range.
 */
int __fastcall ClipPolyZRange_WithAttr012(zClipRectPartial* clipRect, int* vertexCount)
{
    const int count = *vertexCount;
    const int flags = clipRect->flags;
    int allInsideNear;
    int i;
    zVec3 clippedVerts[kClipBufferCapacity] = { 0 };
    zClipUV clippedUvs[kClipBufferCapacity] = { 0 };
    float clippedAttr0[kClipBufferCapacity] = { 0 };
    float clippedAttr1[kClipBufferCapacity] = { 0 };
    float clippedAttr2[kClipBufferCapacity] = { 0 };
    int outputCount;

    if ((flags & 0x20) != 0) {
        int allBeyondFar = 1;
        for (i = 0; i < count && allBeyondFar != 0; ++i) {
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

    allInsideNear = 1;
    for (i = 0; i < count && allInsideNear != 0; ++i) {
        if (g_Clip_PolyVertsScratch[i].z < clipRect->zMin) {
            allInsideNear = 0;
        }
    }

    if (allInsideNear != 0) {
        return count >= 3 ? 1 : 0;
    }

    outputCount = 0;

    if (count > 0) {
        zVec3 prevVert = g_Clip_PolyVertsScratch[count - 1];
        zClipUV prevUv = g_Clip_PolyUvs[count - 1];
        float prevAttr0 = g_Clip_PolyAttr0[count - 1];
        float prevAttr1 = g_Clip_PolyAttr1[count - 1];
        float prevAttr2 = g_Clip_PolyAttr2[count - 1];
        int prevInside = prevVert.z >= clipRect->zMin;

        for (i = 0; i < count; ++i) {
            const zVec3 currVert = g_Clip_PolyVertsScratch[i];
            const zClipUV currUv = g_Clip_PolyUvs[i];
            const float currAttr0 = g_Clip_PolyAttr0[i];
            const float currAttr1 = g_Clip_PolyAttr1[i];
            const float currAttr2 = g_Clip_PolyAttr2[i];
            const int currInside = currVert.z >= clipRect->zMin;

            if (prevInside != currInside) {
                const float t = (clipRect->zMin - prevVert.z) / (currVert.z - prevVert.z);
                zVec3 intersection = { 0 };
                zClipUV intersectionUv = { 0 };
                intersection.x = prevVert.x + (currVert.x - prevVert.x) * t;
                intersection.y = prevVert.y + (currVert.y - prevVert.y) * t;
                intersection.z = clipRect->zMin;
                intersectionUv.u = prevUv.u + (currUv.u - prevUv.u) * t;
                intersectionUv.v = prevUv.v + (currUv.v - prevUv.v) * t;
                if (outputCount < kClipBufferCapacity) {
                    clippedVerts[outputCount] = intersection;
                    clippedUvs[outputCount] = intersectionUv;
                    clippedAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    clippedAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    clippedAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                }
            }

            if (currInside && outputCount < kClipBufferCapacity) {
                clippedVerts[outputCount] = currVert;
                clippedUvs[outputCount] = currUv;
                clippedAttr0[outputCount] = currAttr0;
                clippedAttr1[outputCount] = currAttr1;
                clippedAttr2[outputCount] = currAttr2;
                ++outputCount;
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

    memcpy(g_Clip_PolyVertsScratch, clippedVerts, (size_t)(outputCount) * sizeof(zVec3));
    memcpy(g_Clip_PolyUvs, clippedUvs, (size_t)(outputCount) * sizeof(zClipUV));
    memcpy(g_Clip_PolyAttr0, clippedAttr0, (size_t)(outputCount) * sizeof(float));
    memcpy(g_Clip_PolyAttr2, clippedAttr2, (size_t)(outputCount) * sizeof(float));
    memcpy(g_Clip_PolyAttr1, clippedAttr1, (size_t)(outputCount) * sizeof(float));
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-clippoly-withattr012
 * @recoil-artifact defines .text recoil:function:0x47efd0: zClipRect::ClipPoly_WithAttr012
 *
 *
 * Purpose: Clip active polygon vertex, UV, and three-attribute streams against enabled XY bounds.
 */
int __fastcall ClipPoly_WithAttr012(zClipRectPartial* clipRect, int* vertexCount)
{
    zClipVert scratchVerts[kClipBufferCapacity];
    zClipUV scratchUvs[kClipBufferCapacity];
    float scratchAttr0[kClipBufferCapacity];
    float scratchAttr1[kClipBufferCapacity];
    float scratchAttr2[kClipBufferCapacity];
    int outputCount = 0;
    int parity = 0;

    if ((clipRect->flags & 0x01) != 0) {
        int count;
        outputCount = 0;
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &g_Clip_PolyVerts[prevIndex];
                zClipVert* currVert = &g_Clip_PolyVerts[i];
                zClipUV* prevUv = &g_Clip_PolyUvs[prevIndex];
                zClipUV* currUv = &g_Clip_PolyUvs[i];
                float prevAttr0 = g_Clip_PolyAttr0[prevIndex];
                float prevAttr1 = g_Clip_PolyAttr1[prevIndex];
                float prevAttr2 = g_Clip_PolyAttr2[prevIndex];
                float currAttr0 = g_Clip_PolyAttr0[i];
                float currAttr1 = g_Clip_PolyAttr1[i];
                float currAttr2 = g_Clip_PolyAttr2[i];

                if (prevVert->x >= clipRect->xMin && currVert->x >= clipRect->xMin) {
                    scratchVerts[outputCount] = *currVert;
                    scratchUvs[outputCount] = *currUv;
                    scratchAttr0[outputCount] = currAttr0;
                    scratchAttr1[outputCount] = currAttr1;
                    scratchAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (prevVert->x >= clipRect->xMin && currVert->x < clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    scratchUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    scratchAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    scratchAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    scratchAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->x >= clipRect->xMin) {
                    const float t = (clipRect->xMin - prevVert->x) / (currVert->x - prevVert->x);
                    scratchVerts[outputCount].x = clipRect->xMin;
                    scratchVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    scratchVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    scratchUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    scratchUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    scratchAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    scratchAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    scratchAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        zClipUV* sourceUvs;
        zClipUV* destUvs;
        float* sourceAttr0;
        float* sourceAttr1;
        float* sourceAttr2;
        float* destAttr0;
        float* destAttr1;
        float* destAttr2;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                zClipUV* prevUv = &sourceUvs[prevIndex];
                zClipUV* currUv = &sourceUvs[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (prevVert->x < clipRect->xMaxAlt && currVert->x < clipRect->xMaxAlt) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (prevVert->x < clipRect->xMaxAlt && currVert->x >= clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->x < clipRect->xMaxAlt) {
                    const float t = (clipRect->xMaxAlt - prevVert->x) / (currVert->x - prevVert->x);
                    destVerts[outputCount].x = clipRect->xMaxAlt;
                    destVerts[outputCount].y = prevVert->y + (currVert->y - prevVert->y) * t;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        zClipUV* sourceUvs;
        zClipUV* destUvs;
        float* sourceAttr0;
        float* sourceAttr1;
        float* sourceAttr2;
        float* destAttr0;
        float* destAttr1;
        float* destAttr2;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                zClipUV* prevUv = &sourceUvs[prevIndex];
                zClipUV* currUv = &sourceUvs[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (prevVert->y >= clipRect->yMin && currVert->y >= clipRect->yMin) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (prevVert->y >= clipRect->yMin && currVert->y < clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->y >= clipRect->yMin) {
                    const float t = (clipRect->yMin - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMin;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
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
        zClipVert* sourceVerts;
        zClipVert* destVerts;
        zClipUV* sourceUvs;
        zClipUV* destUvs;
        float* sourceAttr0;
        float* sourceAttr1;
        float* sourceAttr2;
        float* destAttr0;
        float* destAttr1;
        float* destAttr2;
        int count;
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
        count = *vertexCount;
        if (count > 0) {
            int prevIndex = count - 1;
            int i;
            for (i = 0; i < count; ++i) {
                zClipVert* prevVert = &sourceVerts[prevIndex];
                zClipVert* currVert = &sourceVerts[i];
                zClipUV* prevUv = &sourceUvs[prevIndex];
                zClipUV* currUv = &sourceUvs[i];
                float prevAttr0 = sourceAttr0[prevIndex];
                float prevAttr1 = sourceAttr1[prevIndex];
                float prevAttr2 = sourceAttr2[prevIndex];
                float currAttr0 = sourceAttr0[i];
                float currAttr1 = sourceAttr1[i];
                float currAttr2 = sourceAttr2[i];

                if (prevVert->y < clipRect->yMaxAlt && currVert->y < clipRect->yMaxAlt) {
                    destVerts[outputCount] = *currVert;
                    destUvs[outputCount] = *currUv;
                    destAttr0[outputCount] = currAttr0;
                    destAttr1[outputCount] = currAttr1;
                    destAttr2[outputCount] = currAttr2;
                    ++outputCount;
                } else if (prevVert->y < clipRect->yMaxAlt && currVert->y >= clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
                    ++outputCount;
                } else if (currVert->y < clipRect->yMaxAlt) {
                    const float t = (clipRect->yMaxAlt - prevVert->y) / (currVert->y - prevVert->y);
                    destVerts[outputCount].x = prevVert->x + (currVert->x - prevVert->x) * t;
                    destVerts[outputCount].y = clipRect->yMaxAlt;
                    destVerts[outputCount].z = prevVert->z + (currVert->z - prevVert->z) * t;
                    destUvs[outputCount].u = prevUv->u + (currUv->u - prevUv->u) * t;
                    destUvs[outputCount].v = prevUv->v + (currUv->v - prevUv->v) * t;
                    destAttr0[outputCount] = prevAttr0 + (currAttr0 - prevAttr0) * t;
                    destAttr1[outputCount] = prevAttr1 + (currAttr1 - prevAttr1) * t;
                    destAttr2[outputCount] = prevAttr2 + (currAttr2 - prevAttr2) * t;
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
        memcpy(g_Clip_PolyVerts, scratchVerts, (size_t)(outputCount) * sizeof(zClipVert));
        memcpy(g_Clip_PolyUvs, scratchUvs, (size_t)(outputCount) * sizeof(zClipUV));
        memcpy(g_Clip_PolyAttr0, scratchAttr0, (size_t)(outputCount) * sizeof(float));
        memcpy(g_Clip_PolyAttr2, scratchAttr2, (size_t)(outputCount) * sizeof(float));
        memcpy(g_Clip_PolyAttr1, scratchAttr1, (size_t)(outputCount) * sizeof(float));
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zcliprect-trivialrejectpolyxy
 * @recoil-artifact defines .text recoil:function:0x4803b0: zClipRect::TrivialRejectPolyXY
 * @recoil-match byte
 *
 * Evidence: Current BN/status show this as a leaf zClipRect namespace helper over g_Clip_PolyVerts.
 * Purpose: Reject polygons whose active vertices all fall outside one enabled XY clip plane.
 */
int __fastcall TrivialRejectPolyXY(zClipRectPartial* clipRect, int vertexCount)
{
    const int flags = clipRect->flags;
    if (flags == 0) {
        return 1;
    }

    if ((flags & 0x01) != 0) {
        int allOutside = 1;
        int i;
        for (i = 0; i < vertexCount && allOutside != 0; ++i) {
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
        int i;
        for (i = 0; i < vertexCount && allOutside != 0; ++i) {
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
        int i;
        for (i = 0; i < vertexCount && allOutside != 0; ++i) {
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
        int i;
        for (i = 0; i < vertexCount && allOutside != 0; ++i) {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-updatesmallpolyrejectthresholds
 * @recoil-artifact defines .text recoil:function:0x4804c0: zModel::UpdateSmallPolyRejectThresholds
 * @recoil-match byte
 *
 * Purpose: cache the doubled and twenty-times small-polygon reject-area
 * thresholds used by projected model clipping.
 */
void __stdcall UpdateSmallPolyRejectThresholds(float baseRejectArea)
{
    gModel_SmallPolyRejectArea2x = baseRejectArea + baseRejectArea;
    gModel_SmallPolyRejectArea20x = gModel_SmallPolyRejectArea2x * 10.0f;
}
