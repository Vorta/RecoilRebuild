#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zclip_alt.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/include/zdi.h"
#include "recoil/recoil_callconv.h"

struct zGeometry_PlaneEquationPartial;

extern int g_Variant_FilterEnabled;
extern zTag4Partial g_VariantTag_Current;
extern float g_zModel_BFETolerance;
extern float g_zModel_ConstVertexMergeEpsilon;
extern int g_zModel_MaxPolygonVertexCountBeforeSplit;
extern double g_zModel_CoplanarTolerance;
extern double g_zModel_ColinearTolerance;
extern float g_zModel_UvQuantizeBias;
extern float g_zModel_UvQuantizeScale;
extern float g_zModel_UvQuantizeInvScale;

/*
 * Active lights retain their scene node alongside the light class data.
 * Node flags govern participation; class data supplies lighting parameters.
 */

struct zModel_ActiveLightEntryLive {
    CZLightDataPartial* light;
    CZNodePartial* lightNode;
    int useFullWeight;
    int contributesToLighting;
    unsigned int reserved_10;
};

RECOIL_STATIC_ASSERT(offsetof(CZNodePartial, flags) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zModel_ActiveLightEntryLive) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_ActiveLightEntryLive, useFullWeight) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zModel_ActiveLightEntryLive, contributesToLighting) == 0x0c);

struct zModel_FogTargetColorOverride {
    zColorRgb colorRgb01;
    float weight;
};

RECOIL_STATIC_ASSERT(offsetof(zModel_FogTargetColorOverride, weight) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(zModel_FogTargetColorOverride) == 0x10);

struct zModel_MaterialSlot {
    zModel_MaterialPartial material;
    short prevPoolIndex;
    short nextPoolIndex;
};

RECOIL_STATIC_ASSERT(sizeof(zModel_MaterialSlot) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialSlot, prevPoolIndex) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialSlot, nextPoolIndex) == 0x2a);

extern zModel_MaterialSlot* g_zModel_MatlPool;
extern int g_zModel_MatlPoolCapacity;
extern int g_zModel_MatlPoolInUseCount;
extern int g_zModel_MatlFreeHeadIndex;
extern int g_zModel_MatlActiveHeadIndex;
extern zModel_MaterialPartial* g_zModel_MatlReuseCache;
extern zModel_MaterialPartial g_zModel_DefaultMaterial;

struct zModel_Uv {
    float u;
    float v;
};

struct zModel_TextureScrollInfoPartial {
    unsigned char unknown_00[0x0a];
    unsigned char wrapShiftU;
    unsigned char wrapShiftV;
};

struct zModel_TextureRefPartial {
    zModel_TextureScrollInfoPartial* textureInfo;
};

struct zModel_MaterialTextureBindingPartial {
    unsigned short flags;
    unsigned char unknown_02[0x0e];
    zModel_TextureRefPartial* textureRef;
};

struct zModel_InstanceSurfaceEntryPartial {
    unsigned int vertexCountAndFlags;
    unsigned char unknown_04[0x0c];
    zModel_Uv* uvs;
    zModel_MaterialTextureBindingPartial* materialBinding;
    unsigned char unknown_18[0x04];
};

struct zModel_InstancePartial {
    unsigned char unknown_00[0x0c];
    int surfaceEntryCount;
    unsigned char unknown_10[0x14];
    float scrollRateU;
    float scrollRateV;
    int scrollingTextureFrameTick;
    zModel_InstanceSurfaceEntryPartial* surfaceEntries;
};

RECOIL_STATIC_ASSERT(sizeof(zModel_Uv) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zModel_TextureScrollInfoPartial, wrapShiftU) == 0x0a);
RECOIL_STATIC_ASSERT(offsetof(zModel_TextureScrollInfoPartial, wrapShiftV) == 0x0b);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialTextureBindingPartial, flags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialTextureBindingPartial, textureRef) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zModel_InstanceSurfaceEntryPartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstanceSurfaceEntryPartial, uvs) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstanceSurfaceEntryPartial, materialBinding) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstancePartial, surfaceEntryCount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstancePartial, scrollRateU) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstancePartial, scrollingTextureFrameTick) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstancePartial, surfaceEntries) == 0x30);

int __fastcall zModelInstanceUpdateScrollingTexturesIfNeeded(zModel_InstancePartial* instance);
void __fastcall zModelInstanceUpdateScrollingTextures(
    const zModel_TextureScrollInfoPartial* textureInfo,
    zModel_Uv* uvs,
    const float* scrollRates,
    int uvCount
);
void __fastcall
zModelRenderPointQueueEntry(const zVec3* pointPos, unsigned short packedColor16, zModel_PointEntryPartial* pointEntry);
int __fastcall
zModelLightBuildLightWeights(zVec3* surfaceNormal, int vertexCount, int* outPackedFogColor, float fogBlendScale);
void __fastcall
zModelLightPointInPolygonInitXZ(CZNodePartial** lightNodes, CZLightDataPartial** lightDataList, int lightCount);

namespace zModel {
int __cdecl Init();
void __fastcall SetVertexShadingEnabled(int enabled);
void __fastcall SetDisplayInstancePoolCapacity(int capacity);
void __fastcall SetSoftwarePathActive(int active);
void __stdcall SetTextureWorldPerMeter(float worldPerMeterU, float worldPerMeterV);
void __stdcall SetTextureWorldBase(float worldBaseU, float worldBaseV);
int __fastcall SetDiTextureWorldPerMeter(zDiPartial* di, int worldSpaceEnabled, float scrollRateU, float scrollRateV);
/** Both render-node entries clear eax before returning (retail 0x476f14/0x477b23, 0x477d55/0x478c5b). */
int __fastcall RenderNodeHardware(CZNodePartial* node, int clipMask);
int __fastcall RenderNodeSoftware(CZNodePartial* node, int clipMask);
void __stdcall SetBackfaceEliminationToleranceScalar(float scalar);
float __cdecl GetBackfaceEliminationToleranceScalar();
void __stdcall UpdateSmallPolyRejectThresholds(float baseRejectArea);
} // namespace zModel

int __cdecl zModelDisplayInit();
void __stdcall OptCatalogSetDamageMaskUv(float u, float v);
int __cdecl OptCatalogIsDamageMaskEnabled();
void __fastcall OptCatalogSetDamageMaskEnabled(int enabled);
int __fastcall OptCatalogIsDamageMaskSlotPtrRegistered(void* slotPtr);
void __fastcall zModelFogSetEnabled(int enabled);
int __cdecl zModelFogIsEnabled();
void __stdcall zModelFogSetDistanceStart(float distanceStart);
float __cdecl zModelFogGetDistanceStart();
void __stdcall zModelFogSetDistanceEnd(float distanceEnd);
void __stdcall zModelFogSetHeightHigh(float heightHigh);
void __stdcall zModelFogSetHeightLow(float heightLow);
void __stdcall zModelFogSetDensity(float density);
void __fastcall zModelFogSetLinearModeEnabled(int enabled);
void __fastcall zModelFogSetColorRgb01(zColorRgb* rgb01);
void __cdecl zModelFogApplyCurrentColor();

namespace zModel_Light {
float __fastcall EvalDistanceWeight(float distance, const CZLightDataPartial* light);
float __fastcall EvalSphereFogFade(const zVec3* point, float radius);
int __fastcall BuildAttr0DepthFade(int vertexCount, int* outHasVariation);
int __fastcall BuildAttr1Falloff(int vertexCount, int* pLightingFlags);
int __fastcall EvalBatchSphereFade(float* outFade);
int __fastcall PointInPolygonTestRadiusXZ(const zVec3* sphereCenter, float radius);
int __fastcall
SetActiveLights(zVec3* surfaceNormal, int vertexCount, int* lightFlags, int* lightingMode, int usePaletteRemap);
} // namespace zModel_Light

namespace zModel_DiPool {
int __fastcall WriteToStream(void* stream);
int __fastcall ReadHeaderFromStream(void* stream, int* outCapacity, int* outInUseCount, int* outFreeHeadIndex);
int __fastcall ReadEntryDynamicDataFromStream(void* stream, zDiPartial* entry);
RECOIL_NO_GS zDiPartial* __fastcall ReadEntryByIndexFromStream(void* stream, int index);
int __fastcall ReadFromStream(void* stream);
zDiPartial* __cdecl AllocFromFreeList();
int __fastcall FreeIfUnreferenced(zDiPartial* di);
} // namespace zModel_DiPool

namespace zModel_Const {
float __cdecl GetVertexMergeEpsilon();
void __stdcall SetVertexMergeEpsilon(float epsilon);
void __stdcall SetCoplanarTolerance(float tolerance);
void __stdcall SetColinearTolerance(float tolerance);
zVec3 __fastcall SetNormalizedCrossFromVertexTriplet(zVec3* vertex0, zVec3* vertex1, zVec3* vertex2);
int __fastcall
check_colinearity(int* vertexCount, zVec3* points, zClipUV* uvPairsA, zVec3* normalsB, zClipUV* uvPairsB);
zGeometry_PlaneEquationPartial* __fastcall
ComputePolygonPlaneEquation(int vertexCount, zVec3* vertices, zGeometry_PlaneEquationPartial* outPlane);
int __fastcall IsPolygonCoplanar(int vertexCount, zVec3* vertices);
int __fastcall AddOrMergeVertex(zDiPartial* self, zVec3* point);
int __fastcall AddOrMergeVertexAndNormal(zDiPartial* self, zVec3* point, zVec3* normal);
int __fastcall FindOrAppendNormalIndex(zDiPartial* self, zVec3* normal);
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
);
void __fastcall QuantizeAndNormalizeUvPairs(int vertexCount, zClipUV* uvPairs);
void __fastcall SplitPolygonChunkedByVertexLimit(
    zDiPartial* self,
    int totalVertexCount,
    zVec3* points,
    zVec3* entryNormals,
    zClipUV* uvPairsA,
    zVec3* normalsA,
    zVec3* normalsBInput,
    zClipUV* uvPairsBInput,
    zModel_MaterialPartial* material,
    unsigned int drawFlags,
    int flagBit8,
    const int* userTag
);
} // namespace zModel_Const

namespace zModel_MatlBuffer {
void __fastcall SetArraySize(int count);
zModel_MaterialPartial* __fastcall CloneToActiveSlot(zModel_MaterialPartial* material);
int __fastcall WriteGameZ(void* stream);
int __fastcall ReadGameZ(void* stream);
int __cdecl ReleaseAllActive();
void __cdecl ReleaseTextureSurfaces();
int __cdecl Shutdown();
} // namespace zModel_MatlBuffer

namespace zModel_Matl {
int __cdecl InitGlobals();
zModel_MaterialSlot* __fastcall GetPoolEntry(int index);
} // namespace zModel_Matl

namespace zModel_MatlSlot {
void __fastcall Release(zModel_MaterialSlot* slot);
int __fastcall IndexFromPtrOrMinus1(zModel_MaterialSlot* slot);
} // namespace zModel_MatlSlot

namespace zModel_Display {
int __cdecl Reset();
int __cdecl Shutdown();
int __cdecl ShutdownThunk();
} // namespace zModel_Display

namespace zScene {
int __fastcall TestProjectedSphereVisible(zVec3* center, float radius);
}

void __fastcall zModelFogTargetColorOverrideSetCurrent(zColorRgb* colorRgb01, float weight);
void __stdcall zModelRenderAlphaScaleSetCurrent(float scale);
void __fastcall zModelRenderVertexAlphaEnabledSetCurrent(int enabled);

namespace VariantTag {
int __fastcall TagsOverlap(const zTag4Partial* tagA, const zTag4Partial* tagB);
int __fastcall CurrentAllowsId(int variantId);
} // namespace VariantTag

namespace zDi {
void __fastcall EvalBoundingSphereLightingFlags(
    zDiPartial* self,
    int* outDepthFade,
    int* outActiveLightState,
    int* outLensFlareVisible
);
}

/*
 * Reconstructed zModel storage root.
 * Adopted retail extent: [0x576200, 0x57da30). Shared-core containment
 * ([0x5762b8, 0x57d984)) is high confidence; exact outer boundaries remain
 * provisional. Names and opaque-span representations are reconstruction
 * choices: the unknown_* members are unrecovered byte spans of the adopted
 * layout, not claimed padding, reserved fields or particular scalars.
 * The compatibility macros below expose the recovered retail names as members
 * of the single definition in gmod_init.c; offsets are checked there.
 */
struct zModel_GlobalState {
    int diPoolCapacity; /* +0x0000 0x576200 */
    zDiPartial* diPoolBase; /* +0x0004 0x576204 */
    int diPoolInUseCount; /* +0x0008 0x576208 */
    int diPoolFreeHeadIndex; /* +0x000c 0x57620c */
    int renderMode; /* +0x0010 0x576210 */
    CZCameraDataPartial* projectionViewContext; /* +0x0014 0x576214 */
    zClipRectPartial clipRectPrimary; /* +0x0018 0x576218 */
    float projectClipLeft; /* +0x003c 0x57623c */
    float projectClipTop; /* +0x0040 0x576240 */
    float projectClipRight; /* +0x0044 0x576244 */
    float projectClipBottom; /* +0x0048 0x576248 */
    float smallPolyRejectArea2x; /* +0x004c 0x57624c */
    float smallPolyRejectArea20x; /* +0x0050 0x576250 */
    int altClipSourceRectValid; /* +0x0054 0x576254 */
    zClipRectPartial clipRectAlt; /* +0x0058 0x576258 */
    unsigned char unknown_007c[0x10]; /* +0x007c 0x57627c unrecovered byte span */
    zClipAltFloatRect altSourceRect; /* +0x008c 0x57628c */
    float altSourceWidth; /* +0x009c 0x57629c */
    float altSourceHeight; /* +0x00a0 0x5762a0 */
    float altRemapOffsetX; /* +0x00a4 0x5762a4 */
    float altRemapOffsetY; /* +0x00a8 0x5762a8 */
    float altRemapScaleX; /* +0x00ac 0x5762ac */
    float altRemapScaleY; /* +0x00b0 0x5762b0 */
    float altRemapBiasX; /* +0x00b4 0x5762b4 */
    float altRemapBiasY; /* +0x00b8 0x5762b8 */
    zVec3 sharedVec3ScratchAStorage[0x400]; /* +0x00bc 0x5762bc */
    zVec3 sharedVec3ScratchBStorage[0x400]; /* +0x30bc 0x5792bc */
    zVec3* transformedVerts; /* +0x60bc 0x57c2bc */
    zVec3* transformedNormals; /* +0x60c0 0x57c2c0 */
    /*
     * Compatibility name g_CZClass_DiFaceVertexScratch4: the 4 suffix reflects
     * the earlier reconstruction; the adopted extent is zVec3[0x40], bounded by
     * the next member at +0x300.
     */
    zVec3 diFaceVertexScratch[0x40]; /* +0x60c4 0x57c2c4 */
    // Transformed zVec3 records: the renderers gather them with whole-record copies (0x477211, 0x47812c).
    zVec3 clipPolyVertsScratch[0x40]; /* +0x63c4 0x57c5c4 */
    zClipVert clipPolyVerts[0x40]; /* +0x66c4 0x57c8c4 */
    zClipUV clipPolyUvsStorage[0x40]; /* +0x69c4 0x57cbc4 */
    zClipUV* clipPolyUvs; /* +0x6bc4 0x57cdc4 */
    zVec3 currentPolyNormalsStorage[0x40]; /* +0x6bc8 0x57cdc8 */
    zVec3* currentPolyNormals; /* +0x6ec8 0x57d0c8 */
    float clipPolyAttr0[0x40]; /* +0x6ecc 0x57d0cc */
    float clipPolyAttr1[0x40]; /* +0x6fcc 0x57d1cc */
    float clipPolyAttr2[0x40]; /* +0x70cc 0x57d2cc */
    /*
     * Ambient/fog palette remap recipe: color0 = ambient colour, color1 = fog
     * base colour, color0Strength = ambient intensity, color1Strength = scale.
     * Retail 0x4773f9/0x477406 pass 0x57d3cc to the recipe consumers.
     */
    zVidPaletteRemapRecipe ambientPaletteRemapRecipe; /* +0x71cc 0x57d3cc */
    zVidPaletteRemapRecipe specialLightPaletteRemapRecipe; /* +0x71ec 0x57d3ec */
    int vertexShadingEnabled; /* +0x720c 0x57d40c */
    CZNodePartial** lightInputNodeStates; /* +0x7210 0x57d410 */
    CZLightDataPartial** lightInputDataList; /* +0x7214 0x57d414 */
    int hasActiveLights; /* +0x7218 0x57d418 */
    int lightInputCount; /* +0x721c 0x57d41c */
    int activeLightCount; /* +0x7220 0x57d420 */
    int activeLightSpecialIndex; /* +0x7224 0x57d424 */
    zModel_ActiveLightEntryLive activeLights[0x40]; /* +0x7228 0x57d428 */
    int displayClearedWriteOnlyFlag; /* +0x7728 0x57d928 */
    int displayInitWriteOnlyFlag; /* +0x772c 0x57d92c */
    int fogEnabled; /* +0x7730 0x57d930 */
    int fogLinearModeEnabled; /* +0x7734 0x57d934 */
    zColorRgb fogColorRgb01; /* +0x7738 0x57d938 */
    float fogDistanceStart; /* +0x7744 0x57d944 */
    float fogDistanceEnd; /* +0x7748 0x57d948 */
    float fogDistanceInvRange; /* +0x774c 0x57d94c */
    float fogHeightHigh; /* +0x7750 0x57d950 */
    float fogHeightLow; /* +0x7754 0x57d954 */
    float fogHeightInvRange; /* +0x7758 0x57d958 */
    float fogDensity; /* +0x775c 0x57d95c */
    int renderVertexAlphaEnabled; /* +0x7760 0x57d960 */
    float renderAlphaScaleCurrent; /* +0x7764 0x57d964 */
    zModel_FogTargetColorOverride fogTargetColorOverride; /* +0x7768 0x57d968 */
    float inverseZTolerance; /* +0x7778 0x57d978 */
    zVec3* sharedVec3ScratchA; /* +0x777c 0x57d97c */
    zVec3* sharedVec3ScratchB; /* +0x7780 0x57d980 */
    zVec3* pointInPolygonVertices; /* +0x7784 0x57d984 */
    zVec3* pointInPolygonEdgeNormals; /* +0x7788 0x57d988 */
    int pointInPolygonVertexCount; /* +0x778c 0x57d98c */
    float textureWorldBaseU; /* +0x7790 0x57d990 */
    float textureWorldBaseV; /* +0x7794 0x57d994 */
    float textureWorldPerMeterU; /* +0x7798 0x57d998 */
    float textureWorldPerMeterV; /* +0x779c 0x57d99c */
    int damageMaskEnabled; /* +0x77a0 0x57d9a0 */
    int damageMaskSlotIndex; /* +0x77a4 0x57d9a4 */
    void* damageMaskHandles[3]; /* +0x77a8 0x57d9a8 */
    float damageMaskPhaseU; /* +0x77b4 0x57d9b4 */
    float damageMaskPhaseV; /* +0x77b8 0x57d9b8 */
    int defaultGraphicsFlags; /* +0x77bc 0x57d9bc */
    int* pGraphicsFlags; /* +0x77c0 0x57d9c0 */
    unsigned char unknown_77c4[4]; /* +0x77c4 0x57d9c4 unrecovered byte span */
    int softwarePathActive; /* +0x77c8 0x57d9c8 */
    unsigned char unknown_77cc[0x14]; /* +0x77cc 0x57d9cc unrecovered byte span */
    CZRenderFn renderFn; /* +0x77e0 0x57d9e0 */
    int clipMaskStack[0x10]; /* +0x77e4 0x57d9e4 */
    int* clipMaskStackTop; /* +0x7824 0x57da24 */
    zTag4Partial variantCurrentTag; /* +0x7828 0x57da28 */
    int altClipPassEnabled; /* +0x782c 0x57da2c */
};

extern "C" {
extern zModel_GlobalState g_zModel_GlobalStateStorage;
}

#define g_zModel_DiPoolCapacity (g_zModel_GlobalStateStorage.diPoolCapacity)
#define g_zModel_DiPoolBase (g_zModel_GlobalStateStorage.diPoolBase)
#define g_zModel_DiPoolInUseCount (g_zModel_GlobalStateStorage.diPoolInUseCount)
#define g_zModel_DiPoolFreeHeadIndex (g_zModel_GlobalStateStorage.diPoolFreeHeadIndex)
#define gModel_RenderMode (g_zModel_GlobalStateStorage.renderMode)
#define g_zVideo_pActiveProjectionViewContext (g_zModel_GlobalStateStorage.projectionViewContext)
#define gClipRect_Primary (g_zModel_GlobalStateStorage.clipRectPrimary)
#define g_zVideo_ProjectClipLeft (g_zModel_GlobalStateStorage.projectClipLeft)
#define g_zVideo_ProjectClipTop (g_zModel_GlobalStateStorage.projectClipTop)
#define g_zVideo_ProjectClipRight (g_zModel_GlobalStateStorage.projectClipRight)
#define g_zVideo_ProjectClipBottom (g_zModel_GlobalStateStorage.projectClipBottom)
#define gModel_SmallPolyRejectArea2x (g_zModel_GlobalStateStorage.smallPolyRejectArea2x)
#define gModel_SmallPolyRejectArea20x (g_zModel_GlobalStateStorage.smallPolyRejectArea20x)
#define gAltClipSourceRectValid (g_zModel_GlobalStateStorage.altClipSourceRectValid)
#define gClipRect_Alt (g_zModel_GlobalStateStorage.clipRectAlt)
#define g_zClipAlt_SourceRect (g_zModel_GlobalStateStorage.altSourceRect)
#define g_zClipAlt_SourceLeft (g_zModel_GlobalStateStorage.altSourceRect.left)
#define g_zClipAlt_SourceTop (g_zModel_GlobalStateStorage.altSourceRect.top)
#define g_zClipAlt_SourceRight (g_zModel_GlobalStateStorage.altSourceRect.right)
#define g_zClipAlt_SourceBottom (g_zModel_GlobalStateStorage.altSourceRect.bottom)
#define g_zClipAlt_SourceWidth (g_zModel_GlobalStateStorage.altSourceWidth)
#define g_zClipAlt_SourceHeight (g_zModel_GlobalStateStorage.altSourceHeight)
#define g_zClipAlt_RemapOffsetX (g_zModel_GlobalStateStorage.altRemapOffsetX)
#define g_zClipAlt_RemapOffsetY (g_zModel_GlobalStateStorage.altRemapOffsetY)
#define g_zClipAlt_RemapScaleX (g_zModel_GlobalStateStorage.altRemapScaleX)
#define g_zClipAlt_RemapScaleY (g_zModel_GlobalStateStorage.altRemapScaleY)
#define g_zClipAlt_RemapBiasX (g_zModel_GlobalStateStorage.altRemapBiasX)
#define g_zClipAlt_RemapBiasY (g_zModel_GlobalStateStorage.altRemapBiasY)
#define g_zModel_SharedVec3ScratchAStorage (g_zModel_GlobalStateStorage.sharedVec3ScratchAStorage)
#define g_zModel_SharedVec3ScratchBStorage (g_zModel_GlobalStateStorage.sharedVec3ScratchBStorage)
#define g_zModel_TransformedVerts (g_zModel_GlobalStateStorage.transformedVerts)
#define g_zModel_TransformedNormals (g_zModel_GlobalStateStorage.transformedNormals)
#define g_CZClass_DiFaceVertexScratch4 (g_zModel_GlobalStateStorage.diFaceVertexScratch)
#define g_Clip_PolyVertsScratch (g_zModel_GlobalStateStorage.clipPolyVertsScratch)
#define g_Clip_PolyVerts (g_zModel_GlobalStateStorage.clipPolyVerts)
#define g_Clip_PolyUvsStorage (g_zModel_GlobalStateStorage.clipPolyUvsStorage)
#define g_Clip_PolyUvs (g_zModel_GlobalStateStorage.clipPolyUvs)
#define g_zModel_CurrentPolyNormalsStorage (g_zModel_GlobalStateStorage.currentPolyNormalsStorage)
#define g_zModel_CurrentPolyNormals (g_zModel_GlobalStateStorage.currentPolyNormals)
#define g_Clip_PolyAttr0 (g_zModel_GlobalStateStorage.clipPolyAttr0)
#define g_Clip_PolyAttr1 (g_zModel_GlobalStateStorage.clipPolyAttr1)
#define g_Clip_PolyAttr2 (g_zModel_GlobalStateStorage.clipPolyAttr2)
#define gModel_AmbientPaletteRemapRecipe (g_zModel_GlobalStateStorage.ambientPaletteRemapRecipe)
#define gModel_AmbientColorRgb01 (g_zModel_GlobalStateStorage.ambientPaletteRemapRecipe.color0)
#define gModel_FogBaseColorRgb01 (g_zModel_GlobalStateStorage.ambientPaletteRemapRecipe.color1)
#define gModel_AmbientIntensityFactor (g_zModel_GlobalStateStorage.ambientPaletteRemapRecipe.color0Strength)
#define gModel_AmbientScale (g_zModel_GlobalStateStorage.ambientPaletteRemapRecipe.color1Strength)
#define gModel_SpecialLightPaletteRemapRecipe (g_zModel_GlobalStateStorage.specialLightPaletteRemapRecipe)
#define g_zModel_VertexShadingEnabled (g_zModel_GlobalStateStorage.vertexShadingEnabled)
#define gModel_LightInputNodeStates (g_zModel_GlobalStateStorage.lightInputNodeStates)
#define gModel_LightInputDataList (g_zModel_GlobalStateStorage.lightInputDataList)
#define gModel_HasActiveLights (g_zModel_GlobalStateStorage.hasActiveLights)
#define gModel_LightInputCount (g_zModel_GlobalStateStorage.lightInputCount)
#define gModel_ActiveLightCount (g_zModel_GlobalStateStorage.activeLightCount)
#define gModel_ActiveLightSpecialIndex (g_zModel_GlobalStateStorage.activeLightSpecialIndex)
#define gModel_ActiveLights (g_zModel_GlobalStateStorage.activeLights)
#define gModel_DisplayClearedWriteOnlyFlag (g_zModel_GlobalStateStorage.displayClearedWriteOnlyFlag)
#define gModel_DisplayInitWriteOnlyFlag (g_zModel_GlobalStateStorage.displayInitWriteOnlyFlag)
#define gModel_FogEnabled (g_zModel_GlobalStateStorage.fogEnabled)
#define gModel_FogLinearModeEnabled (g_zModel_GlobalStateStorage.fogLinearModeEnabled)
#define gModel_FogColorRgb01 (g_zModel_GlobalStateStorage.fogColorRgb01)
#define gModel_FogDistanceStart (g_zModel_GlobalStateStorage.fogDistanceStart)
#define gModel_FogDistanceEnd (g_zModel_GlobalStateStorage.fogDistanceEnd)
#define gModel_FogDistanceInvRange (g_zModel_GlobalStateStorage.fogDistanceInvRange)
#define gModel_FogHeightHigh (g_zModel_GlobalStateStorage.fogHeightHigh)
#define gModel_FogHeightLow (g_zModel_GlobalStateStorage.fogHeightLow)
#define gModel_FogHeightInvRange (g_zModel_GlobalStateStorage.fogHeightInvRange)
#define gModel_FogDensity (g_zModel_GlobalStateStorage.fogDensity)
#define gModel_RenderVertexAlphaEnabled (g_zModel_GlobalStateStorage.renderVertexAlphaEnabled)
#define gModel_RenderAlphaScaleCurrent (g_zModel_GlobalStateStorage.renderAlphaScaleCurrent)
#define g_zModel_FogTargetColorOverride (g_zModel_GlobalStateStorage.fogTargetColorOverride)
#define g_zRndr_InverseZTolerance (g_zModel_GlobalStateStorage.inverseZTolerance)
#define g_zModel_SharedVec3ScratchA (g_zModel_GlobalStateStorage.sharedVec3ScratchA)
#define g_zModel_SharedVec3ScratchB (g_zModel_GlobalStateStorage.sharedVec3ScratchB)
#define g_zModel_PointInPolygonVertices (g_zModel_GlobalStateStorage.pointInPolygonVertices)
#define g_zModel_PointInPolygonEdgeNormals (g_zModel_GlobalStateStorage.pointInPolygonEdgeNormals)
#define g_zModel_PointInPolygonVertexCount (g_zModel_GlobalStateStorage.pointInPolygonVertexCount)
#define g_zModel_TextureWorldBaseU (g_zModel_GlobalStateStorage.textureWorldBaseU)
#define g_zModel_TextureWorldBaseV (g_zModel_GlobalStateStorage.textureWorldBaseV)
#define g_zModel_TextureWorldPerMeterU (g_zModel_GlobalStateStorage.textureWorldPerMeterU)
#define g_zModel_TextureWorldPerMeterV (g_zModel_GlobalStateStorage.textureWorldPerMeterV)
#define g_OptCatalogDamageMaskEnabled (g_zModel_GlobalStateStorage.damageMaskEnabled)
#define g_OptCatalogDamageMaskSlotIndex (g_zModel_GlobalStateStorage.damageMaskSlotIndex)
#define g_OptCatalogDamageMaskHandles (g_zModel_GlobalStateStorage.damageMaskHandles)
#define g_OptCatalogDamageMaskPhaseU (g_zModel_GlobalStateStorage.damageMaskPhaseU)
#define g_OptCatalogDamageMaskPhaseV (g_zModel_GlobalStateStorage.damageMaskPhaseV)
#define gModel_DefaultGraphicsFlags (g_zModel_GlobalStateStorage.defaultGraphicsFlags)
#define gModel_pGraphicsFlags (g_zModel_GlobalStateStorage.pGraphicsFlags)
#define g_zModel_SoftwarePathActive (g_zModel_GlobalStateStorage.softwarePathActive)
#define gModel_RenderFn (g_zModel_GlobalStateStorage.renderFn)
#define gModel_ClipMaskStack (g_zModel_GlobalStateStorage.clipMaskStack)
#define gModel_ClipMaskStackTop (g_zModel_GlobalStateStorage.clipMaskStackTop)
#define g_Variant_CurrentTag (g_zModel_GlobalStateStorage.variantCurrentTag)
#define gAltClipPassEnabled (g_zModel_GlobalStateStorage.altClipPassEnabled)
