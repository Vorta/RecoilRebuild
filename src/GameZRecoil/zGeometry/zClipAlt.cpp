#include "zClipAlt.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zMath/zMath.h"

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

namespace zClipAlt {
/**
 * Source owner evidence: the zClipAlt namespace functions share typed alternate-clip rectangle and
 * remap state, with no BN evidence for a C++ object owner.
 * Purpose: Group the recovered zClipAlt namespace routines that configure and apply alternate
 * clipping without introducing a production dispatch-table or class scaffold.
 */

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
