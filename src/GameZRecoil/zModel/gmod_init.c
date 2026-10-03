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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-vertexshadingenabled
 * @recoil-artifact defines .data recoil:data:0x57d40c: g_zModel_VertexShadingEnabled.
 * Purpose: gate vertex-shading behavior for zModel render paths.
 */
int g_zModel_VertexShadingEnabled = 0;
/*
 * BN identifies the gmod_init.c diagnostics as three initialized .data char
 * arrays in this order, including the VC alignment padding between rows.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-sourcefile-gmodinitc
 * @recoil-artifact defines .data recoil:data:0x4e0f28: g_zModel_SourceFile_GmodInitC.
 * Purpose: store the writable source-file path passed to gmod_init diagnostics.
 */
char g_zModel_SourceFile_GmodInitC[0x27] = "D:\\Proj\\GameZRecoil\\zModel\\gmod_init.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-setmodel3darraysizealreadysetfmt
 * @recoil-artifact defines .data recoil:data:0x4e0f50: g_zModel_SetModel3dArraySizeAlreadySetFmt.
 * Purpose: store the writable display-instance pool capacity diagnostic format.
 */
char g_zModel_SetModel3dArraySizeAlreadySetFmt[0x3a] = "Error setting model3d array size; size already set to %d.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-texturescrollnullptrerrormsg
 * @recoil-artifact defines .data recoil:data:0x4e0f8c: g_zModel_TextureScrollNullPtrErrorMsg.
 * Purpose: store the writable null display-instance texture-world diagnostic.
 */
char g_zModel_TextureScrollNullPtrErrorMsg[0x33] = "ERROR setting model texture scroll data; Null ptr.";
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SourceFile_GmodInitC) == 0x27);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SetModel3dArraySizeAlreadySetFmt) == 0x3a);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_TextureScrollNullPtrErrorMsg) == 0x33);

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-displayinitwriteonlyflag
 * @recoil-artifact defines .data recoil:data:0x57d92c: gModel_DisplayInitWriteOnlyFlag.
 * Authored zModel display-init lifecycle global.
 * Purpose: record that display initialization has run.
 */
int gModel_DisplayInitWriteOnlyFlag = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-rendermode
 * @recoil-artifact defines .data recoil:data:0x576210: gModel_RenderMode.
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
void* g_zModel_SpanOcclusionProc = 0;
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-displayclearedwriteonlyflag
 * @recoil-artifact defines .data recoil:data:0x57d928: gModel_DisplayClearedWriteOnlyFlag.
 * Authored zModel display-init lifecycle global.
 * Purpose: clear the display lifecycle write-only state before fog defaults are installed.
 */
int gModel_DisplayClearedWriteOnlyFlag = 0;
int g_zModel_FogReserved = 0;
float g_zModel_FogScale = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-bfetolerance
 * @recoil-artifact defines .data recoil:data:0x4e0fc0: Symbol.
 * Authored zModel display global.
 * Purpose: store the backface-elimination tolerance scalar used by display passes.
 */
float g_zModel_BFETolerance = 0.005f;
zVec3 g_zModel_SharedVec3ScratchAStorage[0x400] = { 0 };
zVec3 g_zModel_SharedVec3ScratchBStorage[0x400] = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-transformedverts
 * @recoil-artifact defines .data recoil:data:0x57c2bc: g_zModel_TransformedVerts.
 * Authored zModel display scratch pointer global.
 * Purpose: point transformed-vertex passes at the primary shared Vec3 scratch buffer.
 */
zVec3* g_zModel_TransformedVerts = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-transformednormals
 * @recoil-artifact defines .data recoil:data:0x57c2c0: g_zModel_TransformedNormals.
 * Authored zModel display scratch pointer global.
 * Purpose: point transformed-normal passes at the secondary shared Vec3 scratch buffer.
 */
zVec3* g_zModel_TransformedNormals = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-sharedvec3scratcha
 * @recoil-artifact defines .data recoil:data:0x57d97c: Symbol.
 * Authored zModel display global.
 * Purpose: point scratch users at the primary shared transformed-vector buffer.
 */
zVec3* g_zModel_SharedVec3ScratchA = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-sharedvec3scratchb
 * @recoil-artifact defines .data recoil:data:0x57d980: Symbol.
 * Authored zModel display global.
 * Purpose: point scratch users at the secondary shared transformed-vector buffer.
 */
zVec3* g_zModel_SharedVec3ScratchB = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-pointinpolygonvertices
 * @recoil-artifact defines .data recoil:data:0x57d984: Symbol.
 * Authored zModel display global.
 * Purpose: alias point-in-polygon vertices to the current primary scratch buffer.
 */
zVec3* g_zModel_PointInPolygonVertices = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-pointinpolygonedgenormals
 * @recoil-artifact defines .data recoil:data:0x57d988: Symbol.
 * Authored zModel display global.
 * Purpose: alias point-in-polygon edge normals to the current secondary scratch buffer.
 */
zVec3* g_zModel_PointInPolygonEdgeNormals = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-pointinpolygonvertexcount
 * @recoil-artifact defines .data recoil:data:0x57d98c: Symbol.
 * Authored zModel display global.
 * Purpose: track the number of points in the current point-in-polygon scratch set.
 */
int g_zModel_PointInPolygonVertexCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-textureworldbaseu
 * @recoil-artifact defines .data recoil:data:0x57d990: Symbol.
 * Authored zModel display global.
 * Purpose: store the world-space texture U origin used by model display setup.
 */
float g_zModel_TextureWorldBaseU = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-textureworldbasev
 * @recoil-artifact defines .data recoil:data:0x57d994: Symbol.
 * Authored zModel display global.
 * Purpose: store the world-space texture V origin used by model display setup.
 */
float g_zModel_TextureWorldBaseV = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-textureworldpermeteru
 * @recoil-artifact defines .data recoil:data:0x57d998: Symbol.
 * Authored zModel display global.
 * Purpose: store the world-space texture U scale used by model display setup.
 */
float g_zModel_TextureWorldPerMeterU = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-textureworldpermeterv
 * @recoil-artifact defines .data recoil:data:0x57d99c: Symbol.
 * Authored zModel display global.
 * Purpose: store the world-space texture V scale used by model display setup.
 */
float g_zModel_TextureWorldPerMeterV = 0.0f;
int g_zModel_ScratchCounters[8] = { 0 };
float g_zModel_PointInPolyTolX = 0.0f;
float g_zModel_PointInPolyTolY = 0.0f;
unsigned char g_zModel_DamageMaskStorage[0x200] = { 0 };
void* g_zModel_DamageMaskCurrent = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-optcatalogdamagemaskenabled
 * @recoil-artifact defines .data recoil:data:0x57d9a0: Symbol.
 * Authored OptCatalog damage-mask global.
 * Purpose: gate whether damage-mask stamping is active for hit surfaces.
 */
int g_OptCatalogDamageMaskEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-optcatalogdamagemaskslotindex
 * @recoil-artifact defines .data recoil:data:0x57d9a4: Symbol.
 * Authored OptCatalog damage-mask global.
 * Purpose: select which registered damage-mask handle slot is active.
 */
int g_OptCatalogDamageMaskSlotIndex = 0;
void* g_OptCatalogDamageMaskHandles[3] = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-optcatalogdamagemaskphaseu
 * @recoil-artifact defines .data recoil:data:0x57d9b4: Symbol.
 * Authored OptCatalog damage-mask global.
 * Purpose: store the current damage-mask U phase before stamp wrapping.
 */
float g_OptCatalogDamageMaskPhaseU = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-optcatalogdamagemaskphasev
 * @recoil-artifact defines .data recoil:data:0x57d9b8: Symbol.
 * Authored OptCatalog damage-mask global.
 * Purpose: store the current damage-mask V phase before stamp wrapping.
 */
float g_OptCatalogDamageMaskPhaseV = 0.0f;
int g_zModel_OptCatalogAux0 = 0;
int g_zModel_OptCatalogAux1 = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-defaultgraphicsflags
 * @recoil-artifact defines .data recoil:data:0x57d9bc: Symbol.
 * Authored zModel display global.
 * Purpose: provide the fallback graphics-flags storage when the options catalog has no entry.
 */
int gModel_DefaultGraphicsFlags = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-pgraphicsflags
 * @recoil-artifact defines .data recoil:data:0x57d9c0: gModel_pGraphicsFlags.
 * Authored zModel display global.
 * Purpose: point model display code at the active graphics-flags integer value.
 */
int* gModel_pGraphicsFlags = 0;

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-renderfn
 * @recoil-artifact defines .data recoil:data:0x57d9e0: gModel_RenderFn.
 * Authored zModel display global.
 * Purpose: dispatch visible model nodes to the active renderer path.
 */
CZRenderFn gModel_RenderFn = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-clipmaskstack
 * @recoil-artifact defines .data recoil:data:0x57d9e4: gModel_ClipMaskStack.
 * Authored zModel display global.
 * Purpose: store nested model clip masks for zClass render traversal.
 */
int gModel_ClipMaskStack[0x10] = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-clipmaskstacktop
 * @recoil-artifact defines .data recoil:data:0x57da24: gModel_ClipMaskStackTop.
 * Authored zModel display global.
 * Purpose: track the current entry in the model clip-mask stack.
 */
int* gModel_ClipMaskStackTop = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-variant-filterenabled
 * @recoil-artifact defines .data recoil:data:0x4dd90c: Symbol.
 * Authored variant-filter global.
 * Purpose: gate whether variant tag comparisons filter model display entries.
 */
int g_Variant_FilterEnabled = 1;
zTag4Partial g_VariantTag_Current = { 0 };
zTag4Partial g_Variant_CurrentTag = { 0 };

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-smallpolyrejectarea2x
 * @recoil-artifact defines .data recoil:data:0x57624c: gModel_SmallPolyRejectArea2x.
 * Purpose: cache the doubled small-polygon reject-area threshold.
 */
float gModel_SmallPolyRejectArea2x = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-gmodel-smallpolyrejectarea20x
 * @recoil-artifact defines .data recoil:data:0x576250: gModel_SmallPolyRejectArea20x.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-sourceleft
 * @recoil-artifact defines .data recoil:data:0x57628c: g_zClipAlt_SourceLeft.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Hold the source rectangle left edge for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceLeft = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-sourcetop
 * @recoil-artifact defines .data recoil:data:0x576290: g_zClipAlt_SourceTop.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Hold the source rectangle top edge for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceTop = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-sourceright
 * @recoil-artifact defines .data recoil:data:0x576294: g_zClipAlt_SourceRight.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Hold the source rectangle right edge for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceRight = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-sourcebottom
 * @recoil-artifact defines .data recoil:data:0x576298: g_zClipAlt_SourceBottom.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Hold the source rectangle bottom edge for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceBottom = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-sourcewidth
 * @recoil-artifact defines .data recoil:data:0x57629c: g_zClipAlt_SourceWidth.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Cache the source rectangle width for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceWidth = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-sourceheight
 * @recoil-artifact defines .data recoil:data:0x5762a0: g_zClipAlt_SourceHeight.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Cache the source rectangle height for alternate-clip coordinate remapping.
 */
float g_zClipAlt_SourceHeight = 0.0f;

/**
 * Data owner: zClipAlt target clipping rectangle.
 * Purpose: Hold the alternate clipping bounds used by zClipRect rejection and clipping routines.
 */
zClipRectPartial gClipRect_Alt = { 0 };

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-remapoffsetx
 * @recoil-artifact defines .data recoil:data:0x5762a4: g_zClipAlt_RemapOffsetX.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the source-to-target X offset for alternate clipped points.
 */
float g_zClipAlt_RemapOffsetX = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-remapoffsety
 * @recoil-artifact defines .data recoil:data:0x5762a8: g_zClipAlt_RemapOffsetY.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the source-to-target Y offset for alternate clipped points.
 */
float g_zClipAlt_RemapOffsetY = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-remapscalex
 * @recoil-artifact defines .data recoil:data:0x5762ac: g_zClipAlt_RemapScaleX.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the X scale used to remap alternate clipped points.
 */
float g_zClipAlt_RemapScaleX = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-remapscaley
 * @recoil-artifact defines .data recoil:data:0x5762b0: g_zClipAlt_RemapScaleY.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the Y scale used to remap alternate clipped points.
 */
float g_zClipAlt_RemapScaleY = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-remapbiasx
 * @recoil-artifact defines .data recoil:data:0x5762b4: g_zClipAlt_RemapBiasX.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the X bias used to remap alternate clipped points.
 */
float g_zClipAlt_RemapBiasX = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-remapbiasy
 * @recoil-artifact defines .data recoil:data:0x5762b8: g_zClipAlt_RemapBiasY.
 * Data owner: zClipAlt remap state.
 * Purpose: Cache the Y bias used to remap alternate clipped points.
 */
float g_zClipAlt_RemapBiasY = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-biasincludesprimaryorigin
 * @recoil-artifact defines .data recoil:data:0x5669e4: g_zClipAlt_BiasIncludesPrimaryOrigin.
 * Data owner: zClipAlt remap state.
 * Purpose: Select whether remap bias includes the primary clip origin.
 */
int g_zClipAlt_BiasIncludesPrimaryOrigin = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-galtclipsourcerectvalid
 * @recoil-artifact defines .data recoil:data:0x576254: gAltClipSourceRectValid.
 * Data owner: zClipAlt source rectangle state.
 * Purpose: Record whether the alternate clipping source rectangle has been configured.
 */
int gAltClipSourceRectValid = 0;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-galtclippassenabled
 * @recoil-artifact defines .data recoil:data:0x57da2c: gAltClipPassEnabled.
 * Data owner: zClipAlt pass state.
 * Purpose: Record whether the alternate clipping pass is enabled.
 */
int gAltClipPassEnabled = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-clip-polyverts
 * @recoil-artifact defines .data recoil:data:0x57c8c4: g_Clip_PolyVerts.
 * Data owner: zClipRect polygon clipping scratch vertices.
 * Purpose: Hold the active polygon vertex stream for XY clipping and rejection.
 */
zClipVert g_Clip_PolyVerts[0x40] = { 0 };

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-clip-polyvertsscratch
 * @recoil-artifact defines .data recoil:data:0x57c5c4: g_Clip_PolyVertsScratch.
 * Data owner: zClipRect polygon clipping scratch vertices.
 * Purpose: Hold the alternate polygon vertex stream for Z-range clipping passes.
 */
zClipVert g_Clip_PolyVertsScratch[0x40] = { 0 };

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-clip-polyuvsstorage
 * @recoil-artifact defines .data recoil:data:0x57cbc4: g_Clip_PolyUvsStorage.
 * Data owner: zClipRect polygon clipping scratch UV storage.
 * Purpose: Provide default UV storage for clipping passes that preserve texture coordinates.
 */
zClipUV g_Clip_PolyUvsStorage[0x40] = { 0 };

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-clip-polyuvs
 * @recoil-artifact defines .data recoil:data:0x57cdc4: g_Clip_PolyUvs.
 * Data owner: zClipRect polygon clipping scratch UV cursor.
 * Purpose: Select the active UV stream used by polygon clipping passes.
 */
zClipUV* g_Clip_PolyUvs = 0;

/**
 * Data owner: zClipRect primary clipping rectangle.
 * Purpose: Hold the primary screen clip bounds used by model and alternate clipping callers.
 */
zClipRectPartial gClipRect_Primary = { 0 };

/**
 * Original source helper expression observed in callers 0x476190 and 0x4761e0
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: cache the reciprocal distance-fog range when the range is
 * nonzero.
 */
#define UpdateDistanceInvRange(range)                                                                                  \
    do {                                                                                                               \
        if ((range) != 0.0f) {                                                                                         \
            gModel_FogDistanceInvRange = 1.0f / (range);                                                               \
        }                                                                                                              \
    } while (0)

/**
 * Original source helper expression observed in callers 0x476220 and 0x476260
 * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
 * Purpose: cache the reciprocal height-fog range when the range is nonzero.
 */
#define UpdateHeightInvRange(range)                                                                                    \
    do {                                                                                                               \
        if ((range) != 0.0f) {                                                                                         \
            gModel_FogHeightInvRange = 1.0f / (range);                                                                 \
        }                                                                                                              \
    } while (0)

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-display-init
 * @recoil-artifact defines .text recoil:function:0x475c40: zModelDisplayInit
 *
 *
 * Purpose: initialize zModel display globals, fog defaults, scratch buffers, and damage-mask state.
 */
int __cdecl zModelDisplayInit()
{
    gModel_DisplayInitWriteOnlyFlag = 1;

    gModel_RenderMode = 2;
    g_zModel_DisplayClipMode = 2;
    g_zModel_SpanOcclusionProc = (void*)(&zModel::RenderNodeSoftware);
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

    zOptionEntryPartial* graphicsFlagsOption
        = zGame::OptionsFindOption(g_zVideo_ActiveRendererPath != 0 ? "GfxFlags_HW" : "GfxFlags_SW");
    gModel_pGraphicsFlags
        = graphicsFlagsOption != 0 ? &graphicsFlagsOption->payloadOrBuffer : &gModel_DefaultGraphicsFlags;

    zTag4::Clear(&g_Variant_CurrentTag);
    return 0;
}

namespace zModel_Display
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-display-shutdownthunk
     * @recoil-artifact defines .text recoil:function:0x475e60: zModel_Display::ShutdownThunk
     * @recoil-match byte
     *
     * Purpose: registration thunk that invokes zModel_Display::Shutdown.
     */
    int __cdecl ShutdownThunk()
    {
        Shutdown();
        return 0;
    }
} // namespace zModel_Display

namespace zModel
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-init
     * @recoil-artifact defines .text recoil:function:0x475e70: zModel::Init
     * @recoil-match byte
     *
     * Purpose: initialize zModel material and display-instance pools and choose the render path.
     */
    int __cdecl Init()
    {
        zModel_Matl::InitGlobals();

        if (g_zVideo_ActiveRendererPath != 0) {
            gModel_RenderFn = zModel::RenderNodeHardware;
            g_zModel_SoftwarePathActive = 0;
        } else {
            g_zModel_SoftwarePathActive = 1;
        }

        gModel_ClipMaskStackTop = gModel_ClipMaskStack;

        if (g_zModel_DiPoolCapacity == 0) {
            g_zModel_DiPoolCapacity = 1750;
        }

        g_zModel_DiPoolBase = (zDiPartial*)(malloc(g_zModel_DiPoolCapacity * sizeof(zDiPartial)));
        memset(g_zModel_DiPoolBase, 0, g_zModel_DiPoolCapacity * sizeof(zDiPartial));
        g_zModel_DiPoolFreeHeadIndex = 0;
        if (g_zModel_DiPoolCapacity > 0) {
            for (int i = 0; i < g_zModel_DiPoolCapacity - 1; ++i) {
                g_zModel_DiPoolBase[i].nextFreeIndex = i + 1;
            }

            g_zModel_DiPoolBase[g_zModel_DiPoolCapacity - 1].nextFreeIndex = -1;
        }

        g_zModel_DiPoolInUseCount = 0;
        return 0;
    }
} // namespace zModel

namespace zModel_Display
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-display-reset
     * @recoil-artifact defines .text recoil:function:0x475f60: zModel_Display::Reset
     * @recoil-match byte
     *
     * Purpose: free all currently in-use display-instance pool entries.
     */
    int __cdecl Reset()
    {
        if (g_zModel_DiPoolCapacity > 0) {
            for (int i = 0; i < g_zModel_DiPoolInUseCount; ++i) {
                zModel_DiPool::FreeIfUnreferenced(&g_zModel_DiPoolBase[i]);
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-display-shutdown
     * @recoil-artifact defines .text recoil:function:0x475fa0: zModel_Display::Shutdown
     * @recoil-match byte
     *
     * Purpose: shut down display materials and release the display-instance pool.
     */
    int __cdecl Shutdown()
    {
        zModel_MatlBuffer::Shutdown();
        if (g_zModel_DiPoolCapacity > 0) {
            Reset();
            free(g_zModel_DiPoolBase);
            g_zModel_DiPoolBase = 0;
            g_zModel_DiPoolCapacity = 0;
            g_zModel_DiPoolInUseCount = 0;
            g_zModel_DiPoolFreeHeadIndex = -1;
        }

        return 0;
    }
} // namespace zModel_Display

namespace zModel
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-setdisplayinstancepoolcapacity
     * @recoil-artifact defines .text recoil:function:0x475ff0: zModel::SetDisplayInstancePoolCapacity
     * @recoil-match byte
     *
     * Purpose: set the display-instance pool capacity before zModel initialization.
     */
    void __fastcall SetDisplayInstancePoolCapacity(int capacity)
    {
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

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-setsoftwarepathactive
     * @recoil-artifact defines .text recoil:function:0x476020: zModel::SetSoftwarePathActive
     * @recoil-match byte
     *
     * Purpose: update the software render path flag when no hardware renderer is active.
     */
    void __fastcall SetSoftwarePathActive(int active)
    {
        if (g_zVideo_ActiveRendererPath == 0) {
            g_zModel_SoftwarePathActive = active;
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-setvertexshadingenabled
     * @recoil-artifact defines .text recoil:function:0x476030: zModel::SetVertexShadingEnabled
     * @recoil-match byte
     *
     * Purpose: set the global vertex-shading enable flag.
     */
    void __fastcall SetVertexShadingEnabled(int enabled)
    {
        g_zModel_VertexShadingEnabled = enabled;
    }
} // namespace zModel

/**
 * Purpose: optionally copy a fog-target override color and always store its
 * blend weight.
 */
void __fastcall zModelFogTargetColorOverrideSetCurrent(zColorRgb* colorRgb01, float weight)
{
    if (colorRgb01 != 0) {
        g_zModel_FogTargetColorOverride.colorRgb01 = *colorRgb01;
    }
    g_zModel_FogTargetColorOverride.weight = weight;
}

/**
 * Purpose: store the current render alpha-scale value.
 */
void __stdcall zModelRenderAlphaScaleSetCurrent(float scale)
{
    gModel_RenderAlphaScaleCurrent = scale;
}

/**
 * Purpose: store the current vertex-alpha enabled flag.
 */
void __fastcall zModelRenderVertexAlphaEnabledSetCurrent(int enabled)
{
    gModel_RenderVertexAlphaEnabled = enabled;
}

namespace zModel
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-settextureworldpermeter
     * @recoil-artifact defines .text recoil:function:0x476090: zModel::SetTextureWorldPerMeter
     * @recoil-match byte
     *
     * Purpose: set global texture-world scale per meter.
     */
    void __stdcall SetTextureWorldPerMeter(float worldPerMeterU, float worldPerMeterV)
    {
        g_zModel_TextureWorldPerMeterU = worldPerMeterU;
        g_zModel_TextureWorldPerMeterV = worldPerMeterV;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-settextureworldbase
     * @recoil-artifact defines .text recoil:function:0x4760b0: zModel::SetTextureWorldBase
     * @recoil-match byte
     *
     * Purpose: set global texture-world base coordinates.
     */
    void __stdcall SetTextureWorldBase(float worldBaseU, float worldBaseV)
    {
        g_zModel_TextureWorldBaseU = worldBaseU;
        g_zModel_TextureWorldBaseV = worldBaseV;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zmodel-setditextureworldpermeter
     * @recoil-artifact defines .text recoil:function:0x4760d0: zModel::SetDiTextureWorldPerMeter
     * @recoil-match byte
     *
     * Purpose: enable display-instance texture scrolling and store its U/V rates.
     */
    int __fastcall
    SetDiTextureWorldPerMeter(zDiPartial * di, int worldSpaceEnabled, float scrollRateU, float scrollRateV)
    {
        if (di == 0) {
            zError::ReportOld(0x200, g_zModel_SourceFile_GmodInitC, 0x285, g_zModel_TextureScrollNullPtrErrorMsg);
            return 1;
        }

        di->flags = (di->flags & ~0x20) | ((worldSpaceEnabled & 1) << 5);
        di->scrollRateU = scrollRateU;
        di->scrollRateV = scrollRateV;
        return 0;
    }
} // namespace zModel

namespace zClipAlt
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zclipalt-setsourcerect
     * @recoil-artifact defines .text recoil:function:0x476120: zClipAlt::SetSourceRect.
     *
     *
     * Purpose: cache the source rectangle extents used to remap alternate clipped
     * points into the active target rectangle.
     */
    void __fastcall SetSourceRect(const zClipAltFloatRect* rect)
    {
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
 * Purpose: store the current fog-enabled flag.
 */
void __fastcall zModelFogSetEnabled(int enabled)
{
    gModel_FogEnabled = enabled;
}

/**
 * Purpose: return the current fog-enabled flag.
 */
int __cdecl zModelFogIsEnabled()
{
    return gModel_FogEnabled;
}

/**
 * Purpose: store the distance-fog start value and refresh the cached inverse
 * range against the current end value.
 */
void __stdcall zModelFogSetDistanceStart(float distanceStart)
{
    const float range = gModel_FogDistanceEnd - distanceStart;
    gModel_FogDistanceStart = distanceStart;
    UpdateDistanceInvRange(range);
}

/**
 * Purpose: return the current distance-fog start value.
 */
float __cdecl zModelFogGetDistanceStart()
{
    return gModel_FogDistanceStart;
}

/**
 * Purpose: store the distance-fog end value and refresh the cached inverse
 * range against the current start value.
 */
void __stdcall zModelFogSetDistanceEnd(float distanceEnd)
{
    const float range = distanceEnd - gModel_FogDistanceStart;
    gModel_FogDistanceEnd = distanceEnd;
    UpdateDistanceInvRange(range);
}

/**
 * Purpose: store the high height-fog bound and refresh the cached inverse
 * vertical range.
 */
void __stdcall zModelFogSetHeightHigh(float heightHigh)
{
    const float range = heightHigh - gModel_FogHeightLow;
    gModel_FogHeightHigh = heightHigh;
    UpdateHeightInvRange(range);
}

/**
 * Purpose: store the low height-fog bound and refresh the cached inverse
 * vertical range.
 */
void __stdcall zModelFogSetHeightLow(float heightLow)
{
    const float range = gModel_FogHeightHigh - heightLow;
    gModel_FogHeightLow = heightLow;
    UpdateHeightInvRange(range);
}

/**
 * Purpose: store the current fog density scalar.
 */
void __stdcall zModelFogSetDensity(float density)
{
    gModel_FogDensity = density;
}

/**
 * Purpose: store the linear fog mode enabled flag.
 */
void __fastcall zModelFogSetLinearModeEnabled(int enabled)
{
    gModel_FogLinearModeEnabled = enabled;
}

/**
 * Purpose: copy the fog RGB color and update hardware renderer fog color when
 * the active renderer path requires it.
 */
void __fastcall zModelFogSetColorRgb01(zColorRgb* rgb01)
{
    memcpy(&gModel_FogColorRgb01, rgb01, sizeof(gModel_FogColorRgb01));
    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo::SetFogColorFromRgb01((zVideo_ColorRgbFloat*)(rgb01));
    }
}

/**
 * Purpose: apply the current fog color through the renderer's clamped RGB path.
 */
void __cdecl zModelFogApplyCurrentColor()
{
    zRndr::FogColorSetRgb01Clamped(&gModel_FogColorRgb01);
}

namespace zRndr
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-zrndr-setinverseztolerance
     * @recoil-artifact defines .text recoil:function:0x476300: zRndr::SetInverseZTolerance
     * @recoil-match byte
     *
     * Purpose: update the software inverse-Z tolerance and mirror it to the active renderer path.
     */
    void __stdcall SetInverseZTolerance(float inverseZTolerance)
    {
        g_zRndr_InverseZTolerance = inverseZTolerance;
        if (g_zVideo_ActiveRendererPath != 0) {
            g_zVideo_InverseZTolerancePending = inverseZTolerance;
        }
    }
} // namespace zRndr
