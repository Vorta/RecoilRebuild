#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/include/zClipAlt.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include "recoil/recoil_types.h"
#include <stdlib.h>
#include <string.h>

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
 * Reimplements data 0x566a1c: Symbol.
 * Authored zModel material-pool global.
 * Purpose: point at the allocated material-slot pool storage.
 */
zModel_MaterialSlot *g_zModel_MatlPool = 0;
/**
 * Reimplements data 0x566a18: Symbol.
 * Authored zModel material-pool global.
 * Purpose: record the configured material-slot pool capacity.
 */
int g_zModel_MatlPoolCapacity = 0;
/**
 * Reimplements data 0x566a20: Symbol.
 * Authored zModel material-pool global.
 * Purpose: count material-slot pool entries currently allocated.
 */
int g_zModel_MatlPoolInUseCount = 0;
/**
 * Reimplements data 0x4e1160: Symbol.
 * Authored zModel material-pool global.
 * Purpose: hold the head index of the material-slot free list.
 */
int g_zModel_MatlFreeHeadIndex = -1;
/**
 * Reimplements data 0x4e1164: Symbol.
 * Authored zModel material-pool global.
 * Purpose: hold the head index of the active material-slot list.
 */
int g_zModel_MatlActiveHeadIndex = -1;
/**
 * Reimplements data 0x566a24: Symbol.
 * Authored zModel material-pool global.
 * Purpose: cache a reusable material record for display material allocation.
 */
zModel_MaterialPartial *g_zModel_MatlReuseCache = 0;
zModel_MaterialPartial g_zModel_DefaultMaterial = {0};
namespace {
extern char g_ZrdrGlobalString_Default[];
extern char g_ZrdrGlobalString_Water[];
extern char g_ZrdrGlobalString_Seafloor[];
extern char g_ZrdrGlobalString_Quicksand[];
extern char g_ZrdrGlobalString_Lava[];
extern char g_ZrdrGlobalString_Fire[];
} // namespace
/**
 * Reimplements data 0x4e0fc8: Symbol.
 * Authored renderer global-string table data.
 * Purpose: track the active count in the fixed-prefix plus dynamic string table.
 */
int g_zRndr_GlobalStringCount = 6;
/**
 * Reimplements data 0x4e0fd0: Symbol.
 * Authored renderer global-string table data.
 * Purpose: store the six built-in renderer prefixes followed by dynamic entries loaded at runtime.
 */
char *g_zRndr_GlobalStringTable[100] = {
    g_ZrdrGlobalString_Default,
    g_ZrdrGlobalString_Water,
    g_ZrdrGlobalString_Seafloor,
    g_ZrdrGlobalString_Quicksand,
    g_ZrdrGlobalString_Lava,
    g_ZrdrGlobalString_Fire
};

namespace {
/**
 * Reimplements data 0x4e1168: g_ZrdrGlobalString_Fire.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "fire" renderer prefix.
 */
char g_ZrdrGlobalString_Fire[] = "fire";
/**
 * Reimplements data 0x4e1170: g_ZrdrGlobalString_Lava.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "lava" renderer prefix.
 */
char g_ZrdrGlobalString_Lava[] = "lava";
/**
 * Reimplements data 0x4e1178: g_ZrdrGlobalString_Quicksand.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "quicksand" renderer prefix.
 */
char g_ZrdrGlobalString_Quicksand[] = "quicksand";
/**
 * Reimplements data 0x4e1184: g_ZrdrGlobalString_Seafloor.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "seafloor" renderer prefix.
 */
char g_ZrdrGlobalString_Seafloor[] = "seafloor";
/**
 * Reimplements data 0x4e1190: g_ZrdrGlobalString_Water.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "water" renderer prefix.
 */
char g_ZrdrGlobalString_Water[] = "water";
/**
 * Reimplements data 0x4e1198: g_ZrdrGlobalString_Default.
 * Authored renderer global-string table data.
 * Purpose: provide the built-in "default" renderer prefix.
 */
char g_ZrdrGlobalString_Default[] = "default";
} // namespace

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

/**
 * Reimplements 0x476470: zModel::GetBackfaceEliminationToleranceScalar
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: return the current global backface-elimination tolerance scalar.
 */
float GetBackfaceEliminationToleranceScalar() {
    return g_zModel_BFETolerance;
}
} // namespace zModel

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
 * Reimplements 0x479c80: OptCatalog_IsDamageMaskEnabled
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: report whether OptCatalog damage-mask stamping is currently enabled.
 */
int OptCatalog_IsDamageMaskEnabled() {
    return g_OptCatalogDamageMaskEnabled;
}

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

namespace zRndr {
/**
 * Reimplements 0x480ec0: zRndr::GlobalStringTable_ReleaseDynamicEntries
 * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
 * Purpose: release dynamically loaded renderer global-string entries and restore the fixed prefix count.
 */
void GlobalStringTable_ReleaseDynamicEntries() {
    for (int i = 6; i < g_zRndr_GlobalStringCount; ++i) {
        free(g_zRndr_GlobalStringTable[i]);
        g_zRndr_GlobalStringTable[i] = 0;
    }

    g_zRndr_GlobalStringCount = 6;
}
} // namespace zRndr

namespace zRndr_GlobalStringTable {
/**
 * Reimplements 0x481460: zRndr_GlobalStringTable::LoadDynamicEntriesFromPath
 * (D:\Proj\GameZRecoil\zRndr\zRndr_GlobalStringTable.cpp).
 * Purpose: append non-prefix dynamic global-string entries loaded from a zReader node tree.
 */
void __fastcall LoadDynamicEntriesFromPath(
    char *path
) {
    if (path == 0) {
        return;
    }

    zReader::Node *const root = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    if (root == 0) {
        return;
    }

    zReader::Node *const stringList = root->value.nodes[1].value.nodes;
    if (stringList == 0) {
        return;
    }

    const int stringCount = stringList[0].value.i32;
    for (int index = 1; index < stringCount; ++index) {
        char *const entry = stringList[index].value.str;
        if (zReader::FindGlobalStringPrefixIndex(entry) != -1) {
            continue;
        }

        if (g_zRndr_GlobalStringCount >= 100) {
            break;
        }

        const size_t byteCount = strlen(entry) + 1;
        char *const copy = (char *)(malloc(byteCount));
        g_zRndr_GlobalStringTable[g_zRndr_GlobalStringCount] = copy;
        if (copy != 0) {
            ++g_zRndr_GlobalStringCount;
            memcpy(
                copy,
                entry,
                byteCount
            );
        }
    }

    zReader::FreeLoadedTree(root);
}
} // namespace zRndr_GlobalStringTable

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

namespace zModel_MatlSlot {
/**
 * Reimplements 0x480dc0: zModel_MatlSlot::Release
 * (D:\Proj\GameZRecoil\zModel\zModel_Matl.cpp).
 * Purpose: release a material slot, free cycle data, and return the slot to the material free list.
 */
void __fastcall Release(
    zModel_MaterialSlot *slot
) {
    if (slot == 0) {
        return;
    }

    if ((slot->material.flags & 0x0400) != 0 && slot->material.cycle != 0) {
        if (slot->material.cycle->frameTable != 0) {
            free(slot->material.cycle->frameTable);
            slot->material.cycle->frameTable = 0;
        }
        free(slot->material.cycle);
        slot->material.cycle = 0;
    }

    memset(
        &slot->material,
        0,
        sizeof(slot->material)
    );

    const int slotIndex = zModel_MatlSlot::IndexFromPtrOrMinus1(slot);
    const short prevIndex = slot->prevPoolIndex;
    const short nextIndex = slot->nextPoolIndex;

    if (prevIndex >= 0) {
        g_zModel_MatlPool[prevIndex].nextPoolIndex = nextIndex;
    }
    if (nextIndex >= 0) {
        g_zModel_MatlPool[nextIndex].prevPoolIndex = prevIndex;
    }
    if (g_zModel_MatlActiveHeadIndex == slotIndex) {
        g_zModel_MatlActiveHeadIndex = nextIndex;
    }

    slot->prevPoolIndex = -1;
    slot->nextPoolIndex = (short)(g_zModel_MatlFreeHeadIndex);
    if (g_zModel_MatlFreeHeadIndex >= 0) {
        g_zModel_MatlPool[g_zModel_MatlFreeHeadIndex].prevPoolIndex = (short)(slotIndex);
    }

    g_zModel_MatlFreeHeadIndex = slotIndex;
    --g_zModel_MatlPoolInUseCount;
}
} // namespace zModel_MatlSlot

namespace zModel_MatlBuffer {
enum {
    kMaterialHasTextureUploadSurface = 0x0100,
    kMaterialTextureSurfacePinned = 0x0200,
    kRendererBackend3dfx = 2,
};

/**
 * Reimplements 0x480d80: zModel_MatlBuffer::ReleaseAllActive
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: release every active material slot and clear the material reuse cache.
 */
int ReleaseAllActive() {
    while (g_zModel_MatlActiveHeadIndex >= 0) {
        if (g_zModel_MatlPool == 0 || g_zModel_MatlActiveHeadIndex >= g_zModel_MatlPoolCapacity) {
            g_zModel_MatlActiveHeadIndex = -1;
            break;
        }
        zModel_MatlSlot::Release(&g_zModel_MatlPool[g_zModel_MatlActiveHeadIndex]);
    }

    g_zModel_MatlReuseCache = 0;
    return 0;
}

/**
 * Reimplements 0x480fd0: zModel_MatlBuffer::ReleaseTextureSurfaces
 * (D:\Proj\GameZRecoil\zModel\zmodel.cpp).
 * Purpose: release upload-surface references for active, unpinned texture materials.
 */
void ReleaseTextureSurfaces() {
    int slotIndex = g_zModel_MatlActiveHeadIndex;
    while (slotIndex >= 0) {
        zModel_MaterialSlot *const slot = &g_zModel_MatlPool[slotIndex];
        zModel_MaterialPartial *const material = &slot->material;

        if ((material->flags & kMaterialHasTextureUploadSurface) != 0 &&
            (material->flags & kMaterialTextureSurfacePinned) == 0) {
            zImage_TexDirEntryPartial *const texDirEntry = material->currentTextureDirectoryEntry;

            if (texDirEntry != 0 && texDirEntry->texture != 0) {
                if (g_zVideo_ActiveRendererPath == kRendererBackend3dfx) {
                    zVid_Image::ReleaseOwnedBuffers(texDirEntry->image);
                }

                ((
                    zVideo_TextureRecordReleaseUploadSurfaceRefProc
                )g_zVideo_pfnTextureRecordReleaseUploadSurfaceRef)(texDirEntry->texture);
            }
        }

        slotIndex = slot->nextPoolIndex;
    }
}

/**
 * Reimplements 0x480f10: zModel_MatlBuffer::Shutdown
 * (D:\Proj\GameZRecoil\zModel\gmod_matl.c).
 * Purpose: shut down the material pool and release dynamic renderer string entries.
 */
int Shutdown() {
    ReleaseAllActive();
    if (g_zModel_MatlPool != 0) {
        free(g_zModel_MatlPool);
        g_zModel_MatlPool = 0;
    }

    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    zRndr::GlobalStringTable_ReleaseDynamicEntries();
    g_zModel_MatlReuseCache = 0;
    return 0;
}
} // namespace zModel_MatlBuffer

namespace zModel_Matl {
/**
 * Reimplements 0x480ae0: zModel_Matl::InitGlobals
 * (GameZRecoil/zModel/zModel_Matl.cpp).
 * Purpose: allocate and initialize the global material-slot pool and default material.
 */
int InitGlobals() {
    if (g_zModel_MatlPoolCapacity == 0) {
        g_zModel_MatlPoolCapacity = 2500;
    }

    const size_t poolBytes = (size_t)(g_zModel_MatlPoolCapacity) * sizeof(zModel_MaterialSlot);
    g_zModel_MatlPool = (zModel_MaterialSlot *)(malloc(poolBytes));
    memset(
        g_zModel_MatlPool,
        0,
        poolBytes
    );

    g_zModel_MatlFreeHeadIndex = 0;
    if (g_zModel_MatlPoolCapacity > 0) {
        for (int i = 0; i < g_zModel_MatlPoolCapacity; ++i) {
            g_zModel_MatlPool[i].prevPoolIndex = (short)(i == 0 ? -1 : i - 1);
            g_zModel_MatlPool[i].nextPoolIndex =
                (short)(i == g_zModel_MatlPoolCapacity - 1 ? -1 : i + 1);
        }
    }

    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlPoolInUseCount = 0;
    zModel_Material::ResetDefaults(&g_zModel_DefaultMaterial);
    return 0;
}

/**
 * Reimplements 0x4805e0: zModel_Matl::GetPoolEntry
 * (D:\Proj\GameZRecoil\zModel\zModel_Display.cpp).
 * Purpose: return the material-slot pool entry for a non-negative index.
 */
zModel_MaterialSlot *__fastcall GetPoolEntry(
    int index
) {
    if (index < 0) {
        return 0;
    }

    return &g_zModel_MatlPool[index];
}
} // namespace zModel_Matl

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
