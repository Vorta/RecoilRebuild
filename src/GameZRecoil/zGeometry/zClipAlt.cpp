#include "zClipAlt.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zMath/zMath.h"

extern "C" {
float g_zClipAlt_SourceLeft = 0.0f;
float g_zClipAlt_SourceTop = 0.0f;
float g_zClipAlt_SourceRight = 0.0f;
float g_zClipAlt_SourceBottom = 0.0f;
float g_zClipAlt_SourceWidth = 0.0f;
float g_zClipAlt_SourceHeight = 0.0f;
zClipRectPartial gClipRect_Alt = {0};
float g_zClipAlt_RemapOffsetX = 0.0f;
float g_zClipAlt_RemapOffsetY = 0.0f;
float g_zClipAlt_RemapScaleX = 0.0f;
float g_zClipAlt_RemapScaleY = 0.0f;
float g_zClipAlt_RemapBiasX = 0.0f;
float g_zClipAlt_RemapBiasY = 0.0f;
int g_zClipAlt_BiasIncludesPrimaryOrigin = 0;
int gAltClipSourceRectValid = 0;
int gAltClipPassEnabled = 0;
}

// Reimplements 0x47a1d0: zClipAlt_BuildFrustumPlanes
// (GameZRecoil/zModel/zmodel.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL zClipAlt_BuildFrustumPlanes(zClass_CameraDataPartial *cameraData) {
    zMath::MatStackPushPtr(cameraData->worldTransform);
    zMath_Mat_TransformNormalBatch(&cameraData->localFrustumLeftNormal, cameraData->worldFrustumNormals, 6);
    zMath::MatStackPopPtr();
}

namespace zClipAlt {
// Reimplements 0x476120: zClipAlt::SetSourceRect
void RECOIL_FASTCALL SetSourceRect(const zClipAltFloatRect *rect) {
    g_zClipAlt_SourceLeft = rect->left;
    g_zClipAlt_SourceTop = rect->top;
    g_zClipAlt_SourceRight = rect->right;
    g_zClipAlt_SourceBottom = rect->bottom;
    g_zClipAlt_SourceWidth = rect->right - rect->left;
    gAltClipSourceRectValid = 1;
    g_zClipAlt_SourceHeight = rect->bottom - rect->top;
}

// Reimplements 0x479f90: zClipAlt::SetTargetRect (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp)
void RECOIL_FASTCALL SetTargetRect(const zClipAltFloatRect *rect, int replicate) {
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

// Reimplements 0x4766a0: zClipAlt::RemapPointXYInPlace
// (D:\Proj\Battlesport\zClip.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL RemapPointXYInPlace(float *point) {
    g_Clip_PolyVerts[0].x = point[0];
    g_Clip_PolyVerts[0].y = point[1];
    if (zClipRect::TrivialRejectPolyXY(&gClipRect_Alt, 1) == 0) {
        return 0;
    }

    point[0] = g_zClipAlt_RemapScaleX * point[0] + g_zClipAlt_RemapBiasX;
    point[1] = g_zClipAlt_RemapScaleY * point[1] + g_zClipAlt_RemapBiasY;
    return 1;
}
} // namespace zClipAlt
