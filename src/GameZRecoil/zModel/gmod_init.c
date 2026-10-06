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

// Option names hud.cpp defines; retail reads these globals (0x4da834,
// 0x4da888), not literals.
extern "C" char g_zOpt_OptionName_GfxFlagsHw[];
extern "C" char g_zOpt_OptionName_GfxFlagsSw[];

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-globalstatestorage
 * @recoil-artifact defines .data recoil:data:0x576200: g_zModel_GlobalStateStorage.diPoolCapacity.
 * @recoil-artifact defines .data recoil:data:0x576204: g_zModel_GlobalStateStorage.diPoolBase.
 * @recoil-artifact defines .data recoil:data:0x576208: g_zModel_GlobalStateStorage.diPoolInUseCount.
 * @recoil-artifact defines .data recoil:data:0x57620c: g_zModel_GlobalStateStorage.diPoolFreeHeadIndex.
 * @recoil-artifact defines .data recoil:data:0x576210: g_zModel_GlobalStateStorage.renderMode.
 * @recoil-artifact defines .data recoil:data:0x576214: g_zModel_GlobalStateStorage.projectionViewContext.
 * @recoil-artifact defines .data recoil:data:0x576218: g_zModel_GlobalStateStorage.clipRectPrimary.
 * @recoil-artifact defines .data recoil:data:0x57623c: g_zModel_GlobalStateStorage.projectClipLeft.
 * @recoil-artifact defines .data recoil:data:0x576240: g_zModel_GlobalStateStorage.projectClipTop.
 * @recoil-artifact defines .data recoil:data:0x576244: g_zModel_GlobalStateStorage.projectClipRight.
 * @recoil-artifact defines .data recoil:data:0x576248: g_zModel_GlobalStateStorage.projectClipBottom.
 * @recoil-artifact defines .data recoil:data:0x57624c: g_zModel_GlobalStateStorage.smallPolyRejectArea2x.
 * @recoil-artifact defines .data recoil:data:0x576250: g_zModel_GlobalStateStorage.smallPolyRejectArea20x.
 * @recoil-artifact defines .data recoil:data:0x576254: g_zModel_GlobalStateStorage.altClipSourceRectValid.
 * @recoil-artifact defines .data recoil:data:0x576258: g_zModel_GlobalStateStorage.clipRectAlt.
 * @recoil-artifact defines .data recoil:data:0x57628c: g_zModel_GlobalStateStorage.altSourceLeft.
 * @recoil-artifact defines .data recoil:data:0x576290: g_zModel_GlobalStateStorage.altSourceTop.
 * @recoil-artifact defines .data recoil:data:0x576294: g_zModel_GlobalStateStorage.altSourceRight.
 * @recoil-artifact defines .data recoil:data:0x576298: g_zModel_GlobalStateStorage.altSourceBottom.
 * @recoil-artifact defines .data recoil:data:0x57629c: g_zModel_GlobalStateStorage.altSourceWidth.
 * @recoil-artifact defines .data recoil:data:0x5762a0: g_zModel_GlobalStateStorage.altSourceHeight.
 * @recoil-artifact defines .data recoil:data:0x5762a4: g_zModel_GlobalStateStorage.altRemapOffsetX.
 * @recoil-artifact defines .data recoil:data:0x5762a8: g_zModel_GlobalStateStorage.altRemapOffsetY.
 * @recoil-artifact defines .data recoil:data:0x5762ac: g_zModel_GlobalStateStorage.altRemapScaleX.
 * @recoil-artifact defines .data recoil:data:0x5762b0: g_zModel_GlobalStateStorage.altRemapScaleY.
 * @recoil-artifact defines .data recoil:data:0x5762b4: g_zModel_GlobalStateStorage.altRemapBiasX.
 * @recoil-artifact defines .data recoil:data:0x5762b8: g_zModel_GlobalStateStorage.altRemapBiasY.
 * @recoil-artifact defines .data recoil:data:0x5762bc: g_zModel_GlobalStateStorage.sharedVec3ScratchAStorage.
 * @recoil-artifact defines .data recoil:data:0x5792bc: g_zModel_GlobalStateStorage.sharedVec3ScratchBStorage.
 * @recoil-artifact defines .data recoil:data:0x57c2bc: g_zModel_GlobalStateStorage.transformedVerts.
 * @recoil-artifact defines .data recoil:data:0x57c2c0: g_zModel_GlobalStateStorage.transformedNormals.
 * @recoil-artifact defines .data recoil:data:0x57c2c4: g_zModel_GlobalStateStorage.diFaceVertexScratch.
 * @recoil-artifact defines .data recoil:data:0x57c5c4: g_zModel_GlobalStateStorage.clipPolyVertsScratch.
 * @recoil-artifact defines .data recoil:data:0x57c8c4: g_zModel_GlobalStateStorage.clipPolyVerts.
 * @recoil-artifact defines .data recoil:data:0x57cbc4: g_zModel_GlobalStateStorage.clipPolyUvsStorage.
 * @recoil-artifact defines .data recoil:data:0x57cdc4: g_zModel_GlobalStateStorage.clipPolyUvs.
 * @recoil-artifact defines .data recoil:data:0x57cdc8: g_zModel_GlobalStateStorage.currentPolyNormalsStorage.
 * @recoil-artifact defines .data recoil:data:0x57d0c8: g_zModel_GlobalStateStorage.currentPolyNormals.
 * @recoil-artifact defines .data recoil:data:0x57d0cc: g_zModel_GlobalStateStorage.clipPolyAttr0.
 * @recoil-artifact defines .data recoil:data:0x57d1cc: g_zModel_GlobalStateStorage.clipPolyAttr1.
 * @recoil-artifact defines .data recoil:data:0x57d2cc: g_zModel_GlobalStateStorage.clipPolyAttr2.
 * @recoil-artifact defines .data recoil:data:0x57d3cc: g_zModel_GlobalStateStorage.ambientColorRgb01.
 * @recoil-artifact defines .data recoil:data:0x57d3d8: g_zModel_GlobalStateStorage.fogBaseColorRgb01.
 * @recoil-artifact defines .data recoil:data:0x57d3e4: g_zModel_GlobalStateStorage.ambientIntensityFactor.
 * @recoil-artifact defines .data recoil:data:0x57d3e8: g_zModel_GlobalStateStorage.ambientScale.
 * @recoil-artifact defines .data recoil:data:0x57d3ec: g_zModel_GlobalStateStorage.specialLightPaletteRemapRecipe.
 * @recoil-artifact defines .data recoil:data:0x57d40c: g_zModel_GlobalStateStorage.vertexShadingEnabled.
 * @recoil-artifact defines .data recoil:data:0x57d410: g_zModel_GlobalStateStorage.lightInputNodeStates.
 * @recoil-artifact defines .data recoil:data:0x57d414: g_zModel_GlobalStateStorage.lightInputDataList.
 * @recoil-artifact defines .data recoil:data:0x57d418: g_zModel_GlobalStateStorage.hasActiveLights.
 * @recoil-artifact defines .data recoil:data:0x57d41c: g_zModel_GlobalStateStorage.lightInputCount.
 * @recoil-artifact defines .data recoil:data:0x57d420: g_zModel_GlobalStateStorage.activeLightCount.
 * @recoil-artifact defines .data recoil:data:0x57d424: g_zModel_GlobalStateStorage.activeLightSpecialIndex.
 * @recoil-artifact defines .data recoil:data:0x57d428: g_zModel_GlobalStateStorage.activeLights.
 * @recoil-artifact defines .data recoil:data:0x57d928: g_zModel_GlobalStateStorage.displayClearedWriteOnlyFlag.
 * @recoil-artifact defines .data recoil:data:0x57d92c: g_zModel_GlobalStateStorage.displayInitWriteOnlyFlag.
 * @recoil-artifact defines .data recoil:data:0x57d930: g_zModel_GlobalStateStorage.fogEnabled.
 * @recoil-artifact defines .data recoil:data:0x57d934: g_zModel_GlobalStateStorage.fogLinearModeEnabled.
 * @recoil-artifact defines .data recoil:data:0x57d938: g_zModel_GlobalStateStorage.fogColorRgb01.
 * @recoil-artifact defines .data recoil:data:0x57d944: g_zModel_GlobalStateStorage.fogDistanceStart.
 * @recoil-artifact defines .data recoil:data:0x57d948: g_zModel_GlobalStateStorage.fogDistanceEnd.
 * @recoil-artifact defines .data recoil:data:0x57d94c: g_zModel_GlobalStateStorage.fogDistanceInvRange.
 * @recoil-artifact defines .data recoil:data:0x57d950: g_zModel_GlobalStateStorage.fogHeightHigh.
 * @recoil-artifact defines .data recoil:data:0x57d954: g_zModel_GlobalStateStorage.fogHeightLow.
 * @recoil-artifact defines .data recoil:data:0x57d958: g_zModel_GlobalStateStorage.fogHeightInvRange.
 * @recoil-artifact defines .data recoil:data:0x57d95c: g_zModel_GlobalStateStorage.fogDensity.
 * @recoil-artifact defines .data recoil:data:0x57d960: g_zModel_GlobalStateStorage.renderVertexAlphaEnabled.
 * @recoil-artifact defines .data recoil:data:0x57d964: g_zModel_GlobalStateStorage.renderAlphaScaleCurrent.
 * @recoil-artifact defines .data recoil:data:0x57d968: g_zModel_GlobalStateStorage.fogTargetColorOverride.
 * @recoil-artifact defines .data recoil:data:0x57d978: g_zModel_GlobalStateStorage.inverseZTolerance.
 * @recoil-artifact defines .data recoil:data:0x57d97c: g_zModel_GlobalStateStorage.sharedVec3ScratchA.
 * @recoil-artifact defines .data recoil:data:0x57d980: g_zModel_GlobalStateStorage.sharedVec3ScratchB.
 * @recoil-artifact defines .data recoil:data:0x57d984: g_zModel_GlobalStateStorage.pointInPolygonVertices.
 * @recoil-artifact defines .data recoil:data:0x57d988: g_zModel_GlobalStateStorage.pointInPolygonEdgeNormals.
 * @recoil-artifact defines .data recoil:data:0x57d98c: g_zModel_GlobalStateStorage.pointInPolygonVertexCount.
 * @recoil-artifact defines .data recoil:data:0x57d990: g_zModel_GlobalStateStorage.textureWorldBaseU.
 * @recoil-artifact defines .data recoil:data:0x57d994: g_zModel_GlobalStateStorage.textureWorldBaseV.
 * @recoil-artifact defines .data recoil:data:0x57d998: g_zModel_GlobalStateStorage.textureWorldPerMeterU.
 * @recoil-artifact defines .data recoil:data:0x57d99c: g_zModel_GlobalStateStorage.textureWorldPerMeterV.
 * @recoil-artifact defines .data recoil:data:0x57d9a0: g_zModel_GlobalStateStorage.damageMaskEnabled.
 * @recoil-artifact defines .data recoil:data:0x57d9a4: g_zModel_GlobalStateStorage.damageMaskSlotIndex.
 * @recoil-artifact defines .data recoil:data:0x57d9a8: g_zModel_GlobalStateStorage.damageMaskHandles.
 * @recoil-artifact defines .data recoil:data:0x57d9b4: g_zModel_GlobalStateStorage.damageMaskPhaseU.
 * @recoil-artifact defines .data recoil:data:0x57d9b8: g_zModel_GlobalStateStorage.damageMaskPhaseV.
 * @recoil-artifact defines .data recoil:data:0x57d9bc: g_zModel_GlobalStateStorage.defaultGraphicsFlags.
 * @recoil-artifact defines .data recoil:data:0x57d9c0: g_zModel_GlobalStateStorage.pGraphicsFlags.
 * @recoil-artifact defines .data recoil:data:0x57d9c8: g_zModel_GlobalStateStorage.softwarePathActive.
 * @recoil-artifact defines .data recoil:data:0x57d9e0: g_zModel_GlobalStateStorage.renderFn.
 * @recoil-artifact defines .data recoil:data:0x57d9e4: g_zModel_GlobalStateStorage.clipMaskStack.
 * @recoil-artifact defines .data recoil:data:0x57da24: g_zModel_GlobalStateStorage.clipMaskStackTop.
 * @recoil-artifact defines .data recoil:data:0x57da28: g_zModel_GlobalStateStorage.variantCurrentTag.
 * @recoil-artifact defines .data recoil:data:0x57da2c: g_zModel_GlobalStateStorage.altClipPassEnabled.
 * Storage group: g_zModel_GlobalStateStorage.
 * Reconstructed zModel storage root over the adopted retail extent
 * [0x576200, 0x57da30). Shared-core containment [0x5762b8, 0x57d984) is high
 * confidence; the outer boundaries remain provisional. Retail addresses the
 * display-instance pool, clip rectangles and alternate-clip remap state, shared
 * Vec3 scratch, clip polygon scratch, lighting, fog, texture-world, damage-mask,
 * graphics-flag, render-dispatch and variant-tag state through this one root;
 * the compatibility field macros in gmod.h keep the recovered retail names.
 * The three unknown_* members are unrecovered byte spans.
 * Purpose: Owns the zero-filled zModel module state.
 */
zModel_GlobalState g_zModel_GlobalStateStorage = { 0 };
}

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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zmodel-bfetolerance
 * @recoil-artifact defines .data recoil:data:0x4e0fc0: Symbol.
 * Authored zModel display global.
 * Purpose: store the backface-elimination tolerance scalar used by display passes.
 */
float g_zModel_BFETolerance = 0.005f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-variant-filterenabled
 * @recoil-artifact defines .data recoil:data:0x4dd90c: Symbol.
 * Authored variant-filter global.
 * Purpose: gate whether variant tag comparisons filter model display entries.
 */
int g_Variant_FilterEnabled = 1;
zTag4Partial g_VariantTag_Current = { 0 };

extern "C" {
/**
 * Source owner evidence: zClipAlt is a namespace/data utility cluster over alternate clip rectangles,
 * remap globals, and typed zClipRect/zMath provider calls.
 * Evidence: BN facts for 0x476120, 0x479f90, 0x4766a0, and 0x47a1d0 show no constructor,
 * destructor, table write, or class-instance field access; the functions operate on file-scope
 * rectangle/remap state and passed camera/rect records.
 * Purpose: Keep the recovered alternate-clip state as typed source-level state rather than a
 * class/table scaffold; its rectangle and remap fields are zModel_GlobalState members.
 */

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-init-g-zclipalt-biasincludesprimaryorigin
 * @recoil-artifact defines .data recoil:data:0x5669e4: g_zClipAlt_BiasIncludesPrimaryOrigin.
 * Data owner: zClipAlt remap state.
 * Purpose: Select whether remap bias includes the primary clip origin.
 */
int g_zClipAlt_BiasIncludesPrimaryOrigin = 0;
}

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
 * @recoil-match byte
 *
 * Purpose: initialize zModel display globals, fog defaults, scratch buffers, and damage-mask state.
 */
int __cdecl zModelDisplayInit()
{
    gModel_DisplayInitWriteOnlyFlag = 1;
    gModel_FogEnabled = 1;
    gModel_FogLinearModeEnabled = 1;

    gModel_RenderMode = 2;
    gModel_RenderFn = zModel::RenderNodeSoftware;
    gAltClipPassEnabled = 0;
    gModel_ClipMaskStackTop = gModel_ClipMaskStack;
    g_zVideo_pActiveProjectionViewContext = 0;
    gClipRect_Primary.xMin = 0;
    gClipRect_Primary.xMax = 320.0f;
    gClipRect_Primary.yMin = 0;
    gClipRect_Primary.yMax = 200.0f;
    gClipRect_Primary.xMaxAlt = 319.0f;
    gClipRect_Primary.yMaxAlt = 199.0f;
    gModel_SmallPolyRejectArea2x = 4.0f;
    gModel_SmallPolyRejectArea20x = 40.0f;
    gAltClipSourceRectValid = 0;
    g_zModel_VertexShadingEnabled = 0;
    gModel_LightInputNodeStates = 0;
    gModel_LightInputDataList = 0;
    gModel_HasActiveLights = 0;
    gModel_LightInputCount = 0;
    gModel_ActiveLightCount = 0;
    gModel_DisplayClearedWriteOnlyFlag = 0;

    gModel_FogColorRgb01.red = 1.0f;
    gModel_FogColorRgb01.green = 0;
    gModel_FogColorRgb01.blue = 1.0f;
    gModel_FogDistanceStart = 500.0f;
    gModel_FogDistanceEnd = 700.0f;
    gModel_FogHeightHigh = 300.0f;
    gModel_FogHeightLow = 200.0f;
    gModel_FogDistanceInvRange = 0.005f;
    gModel_FogHeightInvRange = 0.01f;
    gModel_FogDensity = 2.0f;
    gModel_RenderVertexAlphaEnabled = 0;
    gModel_RenderAlphaScaleCurrent = 1.0f;

    if (g_zVideo_ActiveRendererPath != 0) {
        g_zRndr_InverseZTolerance = 0.02f;
        g_zVideo_InverseZTolerancePending = 0.02f;
    } else {
        g_zRndr_InverseZTolerance = 0.01f;
    }

    g_zModel_TransformedVerts = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_TransformedNormals = g_zModel_SharedVec3ScratchBStorage;
    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_SharedVec3ScratchB = g_zModel_SharedVec3ScratchBStorage;
    g_zModel_PointInPolygonVertices = g_zModel_SharedVec3ScratchAStorage;
    g_zModel_PointInPolygonEdgeNormals = g_zModel_SharedVec3ScratchBStorage;
    {
        for (int handleIndex = 0; handleIndex < 3; ++handleIndex) {
            g_OptCatalogDamageMaskHandles[handleIndex] = 0;
        }
    }
    g_OptCatalogDamageMaskPhaseU = 0;
    g_OptCatalogDamageMaskPhaseV = 0;
    g_zModel_PointInPolygonVertexCount = 0;
    g_zModel_TextureWorldPerMeterU = 0.2f;
    g_zModel_TextureWorldPerMeterV = 0.2f;
    g_Clip_PolyUvs = g_Clip_PolyUvsStorage;
    g_zModel_CurrentPolyNormals = 0;
    g_OptCatalogDamageMaskEnabled = 0;
    g_OptCatalogDamageMaskSlotIndex = 0;
    gModel_DefaultGraphicsFlags = -1;

    zOptionEntryPartial* graphicsFlagsOption = zGame::OptionsFindOption(
        g_zVideo_ActiveRendererPath != 0 ? g_zOpt_OptionName_GfxFlagsHw : g_zOpt_OptionName_GfxFlagsSw
    );
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
     * @recoil-match byte
     *
     * Purpose: cache the source rectangle extents used to remap alternate clipped
     * points into the active target rectangle.
     */
    void __fastcall SetSourceRect(const zClipAltFloatRect* rect)
    {
        g_zClipAlt_SourceRect = *rect;
        g_zClipAlt_SourceWidth = rect->right - rect->left;
        g_zClipAlt_SourceHeight = rect->bottom - rect->top;
        gAltClipSourceRectValid = 1;
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

/*
 * Layout check for zModel_GlobalState: every member offset and extent, including the
 * unrecovered byte spans, against the retail layout.
 */
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, diPoolCapacity) == 0x0 && sizeof(g_zModel_GlobalStateStorage.diPoolCapacity) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, diPoolBase) == 0x4 && sizeof(g_zModel_GlobalStateStorage.diPoolBase) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, diPoolInUseCount) == 0x8 && sizeof(g_zModel_GlobalStateStorage.diPoolInUseCount) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, diPoolFreeHeadIndex) == 0xc
    && sizeof(g_zModel_GlobalStateStorage.diPoolFreeHeadIndex) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, renderMode) == 0x10 && sizeof(g_zModel_GlobalStateStorage.renderMode) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, projectionViewContext) == 0x14
    && sizeof(g_zModel_GlobalStateStorage.projectionViewContext) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipRectPrimary) == 0x18 && sizeof(g_zModel_GlobalStateStorage.clipRectPrimary) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, projectClipLeft) == 0x3c && sizeof(g_zModel_GlobalStateStorage.projectClipLeft) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, projectClipTop) == 0x40 && sizeof(g_zModel_GlobalStateStorage.projectClipTop) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, projectClipRight) == 0x44
    && sizeof(g_zModel_GlobalStateStorage.projectClipRight) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, projectClipBottom) == 0x48
    && sizeof(g_zModel_GlobalStateStorage.projectClipBottom) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, smallPolyRejectArea2x) == 0x4c
    && sizeof(g_zModel_GlobalStateStorage.smallPolyRejectArea2x) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, smallPolyRejectArea20x) == 0x50
    && sizeof(g_zModel_GlobalStateStorage.smallPolyRejectArea20x) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altClipSourceRectValid) == 0x54
    && sizeof(g_zModel_GlobalStateStorage.altClipSourceRectValid) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipRectAlt) == 0x58 && sizeof(g_zModel_GlobalStateStorage.clipRectAlt) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, unknown_007c) == 0x7c && sizeof(g_zModel_GlobalStateStorage.unknown_007c) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altSourceRect.left) == 0x8c
    && sizeof(g_zModel_GlobalStateStorage.altSourceRect.left) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altSourceRect.top) == 0x90
    && sizeof(g_zModel_GlobalStateStorage.altSourceRect.top) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altSourceRect.right) == 0x94
    && sizeof(g_zModel_GlobalStateStorage.altSourceRect.right) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altSourceRect.bottom) == 0x98
    && sizeof(g_zModel_GlobalStateStorage.altSourceRect.bottom) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altSourceWidth) == 0x9c && sizeof(g_zModel_GlobalStateStorage.altSourceWidth) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altSourceHeight) == 0xa0 && sizeof(g_zModel_GlobalStateStorage.altSourceHeight) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altRemapOffsetX) == 0xa4 && sizeof(g_zModel_GlobalStateStorage.altRemapOffsetX) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altRemapOffsetY) == 0xa8 && sizeof(g_zModel_GlobalStateStorage.altRemapOffsetY) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altRemapScaleX) == 0xac && sizeof(g_zModel_GlobalStateStorage.altRemapScaleX) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altRemapScaleY) == 0xb0 && sizeof(g_zModel_GlobalStateStorage.altRemapScaleY) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altRemapBiasX) == 0xb4 && sizeof(g_zModel_GlobalStateStorage.altRemapBiasX) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altRemapBiasY) == 0xb8 && sizeof(g_zModel_GlobalStateStorage.altRemapBiasY) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, sharedVec3ScratchAStorage) == 0xbc
    && sizeof(g_zModel_GlobalStateStorage.sharedVec3ScratchAStorage) == 0x3000
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, sharedVec3ScratchBStorage) == 0x30bc
    && sizeof(g_zModel_GlobalStateStorage.sharedVec3ScratchBStorage) == 0x3000
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, transformedVerts) == 0x60bc
    && sizeof(g_zModel_GlobalStateStorage.transformedVerts) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, transformedNormals) == 0x60c0
    && sizeof(g_zModel_GlobalStateStorage.transformedNormals) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, diFaceVertexScratch) == 0x60c4
    && sizeof(g_zModel_GlobalStateStorage.diFaceVertexScratch) == 0x300
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipPolyVertsScratch) == 0x63c4
    && sizeof(g_zModel_GlobalStateStorage.clipPolyVertsScratch) == 0x300
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipPolyVerts) == 0x66c4 && sizeof(g_zModel_GlobalStateStorage.clipPolyVerts) == 0x300
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipPolyUvsStorage) == 0x69c4
    && sizeof(g_zModel_GlobalStateStorage.clipPolyUvsStorage) == 0x200
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipPolyUvs) == 0x6bc4 && sizeof(g_zModel_GlobalStateStorage.clipPolyUvs) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, currentPolyNormalsStorage) == 0x6bc8
    && sizeof(g_zModel_GlobalStateStorage.currentPolyNormalsStorage) == 0x300
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, currentPolyNormals) == 0x6ec8
    && sizeof(g_zModel_GlobalStateStorage.currentPolyNormals) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipPolyAttr0) == 0x6ecc && sizeof(g_zModel_GlobalStateStorage.clipPolyAttr0) == 0x100
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipPolyAttr1) == 0x6fcc && sizeof(g_zModel_GlobalStateStorage.clipPolyAttr1) == 0x100
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipPolyAttr2) == 0x70cc && sizeof(g_zModel_GlobalStateStorage.clipPolyAttr2) == 0x100
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, ambientColorRgb01) == 0x71cc
    && sizeof(g_zModel_GlobalStateStorage.ambientColorRgb01) == 0xc
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogBaseColorRgb01) == 0x71d8
    && sizeof(g_zModel_GlobalStateStorage.fogBaseColorRgb01) == 0xc
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, ambientIntensityFactor) == 0x71e4
    && sizeof(g_zModel_GlobalStateStorage.ambientIntensityFactor) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, ambientScale) == 0x71e8 && sizeof(g_zModel_GlobalStateStorage.ambientScale) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, specialLightPaletteRemapRecipe) == 0x71ec
    && sizeof(g_zModel_GlobalStateStorage.specialLightPaletteRemapRecipe) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, vertexShadingEnabled) == 0x720c
    && sizeof(g_zModel_GlobalStateStorage.vertexShadingEnabled) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, lightInputNodeStates) == 0x7210
    && sizeof(g_zModel_GlobalStateStorage.lightInputNodeStates) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, lightInputDataList) == 0x7214
    && sizeof(g_zModel_GlobalStateStorage.lightInputDataList) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, hasActiveLights) == 0x7218
    && sizeof(g_zModel_GlobalStateStorage.hasActiveLights) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, lightInputCount) == 0x721c
    && sizeof(g_zModel_GlobalStateStorage.lightInputCount) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, activeLightCount) == 0x7220
    && sizeof(g_zModel_GlobalStateStorage.activeLightCount) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, activeLightSpecialIndex) == 0x7224
    && sizeof(g_zModel_GlobalStateStorage.activeLightSpecialIndex) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, activeLights) == 0x7228 && sizeof(g_zModel_GlobalStateStorage.activeLights) == 0x500
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, displayClearedWriteOnlyFlag) == 0x7728
    && sizeof(g_zModel_GlobalStateStorage.displayClearedWriteOnlyFlag) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, displayInitWriteOnlyFlag) == 0x772c
    && sizeof(g_zModel_GlobalStateStorage.displayInitWriteOnlyFlag) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogEnabled) == 0x7730 && sizeof(g_zModel_GlobalStateStorage.fogEnabled) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogLinearModeEnabled) == 0x7734
    && sizeof(g_zModel_GlobalStateStorage.fogLinearModeEnabled) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogColorRgb01) == 0x7738 && sizeof(g_zModel_GlobalStateStorage.fogColorRgb01) == 0xc
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogDistanceStart) == 0x7744
    && sizeof(g_zModel_GlobalStateStorage.fogDistanceStart) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogDistanceEnd) == 0x7748 && sizeof(g_zModel_GlobalStateStorage.fogDistanceEnd) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogDistanceInvRange) == 0x774c
    && sizeof(g_zModel_GlobalStateStorage.fogDistanceInvRange) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogHeightHigh) == 0x7750 && sizeof(g_zModel_GlobalStateStorage.fogHeightHigh) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogHeightLow) == 0x7754 && sizeof(g_zModel_GlobalStateStorage.fogHeightLow) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogHeightInvRange) == 0x7758
    && sizeof(g_zModel_GlobalStateStorage.fogHeightInvRange) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogDensity) == 0x775c && sizeof(g_zModel_GlobalStateStorage.fogDensity) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, renderVertexAlphaEnabled) == 0x7760
    && sizeof(g_zModel_GlobalStateStorage.renderVertexAlphaEnabled) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, renderAlphaScaleCurrent) == 0x7764
    && sizeof(g_zModel_GlobalStateStorage.renderAlphaScaleCurrent) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, fogTargetColorOverride) == 0x7768
    && sizeof(g_zModel_GlobalStateStorage.fogTargetColorOverride) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, inverseZTolerance) == 0x7778
    && sizeof(g_zModel_GlobalStateStorage.inverseZTolerance) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, sharedVec3ScratchA) == 0x777c
    && sizeof(g_zModel_GlobalStateStorage.sharedVec3ScratchA) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, sharedVec3ScratchB) == 0x7780
    && sizeof(g_zModel_GlobalStateStorage.sharedVec3ScratchB) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, pointInPolygonVertices) == 0x7784
    && sizeof(g_zModel_GlobalStateStorage.pointInPolygonVertices) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, pointInPolygonEdgeNormals) == 0x7788
    && sizeof(g_zModel_GlobalStateStorage.pointInPolygonEdgeNormals) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, pointInPolygonVertexCount) == 0x778c
    && sizeof(g_zModel_GlobalStateStorage.pointInPolygonVertexCount) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, textureWorldBaseU) == 0x7790
    && sizeof(g_zModel_GlobalStateStorage.textureWorldBaseU) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, textureWorldBaseV) == 0x7794
    && sizeof(g_zModel_GlobalStateStorage.textureWorldBaseV) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, textureWorldPerMeterU) == 0x7798
    && sizeof(g_zModel_GlobalStateStorage.textureWorldPerMeterU) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, textureWorldPerMeterV) == 0x779c
    && sizeof(g_zModel_GlobalStateStorage.textureWorldPerMeterV) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, damageMaskEnabled) == 0x77a0
    && sizeof(g_zModel_GlobalStateStorage.damageMaskEnabled) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, damageMaskSlotIndex) == 0x77a4
    && sizeof(g_zModel_GlobalStateStorage.damageMaskSlotIndex) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, damageMaskHandles) == 0x77a8
    && sizeof(g_zModel_GlobalStateStorage.damageMaskHandles) == 0xc
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, damageMaskPhaseU) == 0x77b4
    && sizeof(g_zModel_GlobalStateStorage.damageMaskPhaseU) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, damageMaskPhaseV) == 0x77b8
    && sizeof(g_zModel_GlobalStateStorage.damageMaskPhaseV) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, defaultGraphicsFlags) == 0x77bc
    && sizeof(g_zModel_GlobalStateStorage.defaultGraphicsFlags) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, pGraphicsFlags) == 0x77c0 && sizeof(g_zModel_GlobalStateStorage.pGraphicsFlags) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, unknown_77c4) == 0x77c4 && sizeof(g_zModel_GlobalStateStorage.unknown_77c4) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, softwarePathActive) == 0x77c8
    && sizeof(g_zModel_GlobalStateStorage.softwarePathActive) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, unknown_77cc) == 0x77cc && sizeof(g_zModel_GlobalStateStorage.unknown_77cc) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, renderFn) == 0x77e0 && sizeof(g_zModel_GlobalStateStorage.renderFn) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipMaskStack) == 0x77e4 && sizeof(g_zModel_GlobalStateStorage.clipMaskStack) == 0x40
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, clipMaskStackTop) == 0x7824
    && sizeof(g_zModel_GlobalStateStorage.clipMaskStackTop) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, variantCurrentTag) == 0x7828
    && sizeof(g_zModel_GlobalStateStorage.variantCurrentTag) == 0x4
);
RECOIL_STATIC_ASSERT(
    offsetof(zModel_GlobalState, altClipPassEnabled) == 0x782c
    && sizeof(g_zModel_GlobalStateStorage.altClipPassEnabled) == 0x4
);
RECOIL_STATIC_ASSERT(sizeof(zModel_GlobalState) == 0x7830);
RECOIL_STATIC_ASSERT(sizeof(zVec3) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zClipVert) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zClipUV) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zClipRectPartial) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zColorRgb) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zVidPaletteRemapRecipe) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(zModel_ActiveLightEntryLive) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zModel_FogTargetColorOverride) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zTag4Partial) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(CZRenderFn) == 0x04);
