#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "recoil/recoil_callconv.h"

struct zGeometry_PlaneEquationPartial;
struct zVidPaletteRemapRecipe;

extern int gModel_FogEnabled;
extern int gModel_FogLinearModeEnabled;
extern zColorRgb gModel_FogColorRgb01;
extern float gModel_FogDistanceStart;
extern float gModel_FogDistanceEnd;
extern float gModel_FogDistanceInvRange;
extern float gModel_FogHeightHigh;
extern float gModel_FogHeightLow;
extern float gModel_FogHeightInvRange;
extern float gModel_FogDensity;
extern float gModel_SmallPolyRejectArea2x;
extern float gModel_SmallPolyRejectArea20x;
extern float g_OptCatalogDamageMaskPhaseU;
extern float g_OptCatalogDamageMaskPhaseV;
extern int g_OptCatalogDamageMaskEnabled;
extern int g_OptCatalogDamageMaskSlotIndex;
extern int gModel_DefaultGraphicsFlags;
extern int *gModel_pGraphicsFlags;
extern int gModel_RenderVertexAlphaEnabled;
extern float gModel_RenderAlphaScaleCurrent;
extern int g_Variant_FilterEnabled;
extern zTag4Partial g_VariantTag_Current;
extern zTag4Partial g_Variant_CurrentTag;
extern void *g_OptCatalogDamageMaskHandles[3];
extern int g_zModel_DisplayClipMode;
extern int g_zModel_DisplayClipX;
extern int g_zModel_DisplayClipY;
extern float g_zModel_BFETolerance;
extern int g_zModel_VertexShadingEnabled;
extern float g_zModel_ConstVertexMergeEpsilon;
extern int g_zModel_MaxPolygonVertexCountBeforeSplit;
extern double g_zModel_ConstVertexWarnThreshold;
extern double g_zModel_NormalMergeEpsilon;
extern double g_zModel_CoplanarTolerance;
extern double g_zModel_ColinearTolerance;
extern float g_zModel_UvQuantizeBias;
extern float g_zModel_UvQuantizeScale;
extern float g_zModel_UvQuantizeInvScale;
extern zDiPartial *g_zModel_DiPoolBase;
extern int g_zModel_DiPoolCapacity;
extern int g_zModel_DiPoolInUseCount;
extern int g_zModel_DiPoolFreeHeadIndex;
extern int gModel_HasActiveLights;

struct zModel_LightStatePartial {
    unsigned char unknown_00[0x24];
    int flags;
};

struct zModel_ActiveLightEntryLive {
    zClass_LightDataPartial *light;
    zModel_LightStatePartial *lightState;
    int useFullWeight;
    int contributesToLighting;
    unsigned int reserved_10;
};

RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_LightStatePartial,
        flags
    ) == 0x24
);
RECOIL_STATIC_ASSERT(sizeof(zModel_ActiveLightEntryLive) == 0x14);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_ActiveLightEntryLive,
        useFullWeight
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_ActiveLightEntryLive,
        contributesToLighting
    ) == 0x0c
);

struct zModel_FogTargetColorOverride {
    zColorRgb colorRgb01;
    float weight;
};

RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_FogTargetColorOverride,
        weight
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(sizeof(zModel_FogTargetColorOverride) == 0x10);

extern int gModel_ActiveLightCount;
extern int gModel_ActiveLightSpecialIndex;
extern zModel_ActiveLightEntryLive gModel_ActiveLights[0x40];
extern zClass_LightDataPartial **gModel_LightInputDataList;
extern zModel_LightStatePartial **gModel_LightInputNodeStates;
extern int gModel_LightInputCount;
extern int g_zModel_SoftwarePathActive;
extern float g_Clip_PolyAttr0[0x40];
extern float g_Clip_PolyAttr1[0x40];
extern float g_Clip_PolyAttr2[0x40];
extern zVec3 *g_zModel_CurrentPolyNormals;
extern zVec3 g_zModel_CurrentPolyNormalsStorage[0x40];
extern zModel_FogTargetColorOverride g_zModel_FogTargetColorOverride;
extern zColorRgb gModel_FogBaseColorRgb01;
extern float gModel_AmbientScale;
extern float gModel_AmbientIntensityFactor;
extern zColorRgb gModel_AmbientColorRgb01;

extern zVidPaletteRemapRecipe gModel_SpecialLightPaletteRemapRecipe;

struct zModel_MaterialSlot {
    zModel_MaterialPartial material;
    short prevPoolIndex;
    short nextPoolIndex;
};

RECOIL_STATIC_ASSERT(sizeof(zModel_MaterialSlot) == 0x2c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialSlot,
        prevPoolIndex
    ) == 0x28
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialSlot,
        nextPoolIndex
    ) == 0x2a
);

extern zModel_MaterialSlot *g_zModel_MatlPool;
extern int g_zModel_MatlPoolCapacity;
extern int g_zModel_MatlPoolInUseCount;
extern int g_zModel_MatlFreeHeadIndex;
extern int g_zModel_MatlActiveHeadIndex;
extern zModel_MaterialPartial *g_zModel_MatlReuseCache;
extern zModel_MaterialPartial g_zModel_DefaultMaterial;
extern zVec3 g_zModel_SharedVec3ScratchAStorage[0x400];
extern zVec3 g_zModel_SharedVec3ScratchBStorage[0x400];
extern zVec3 *g_zModel_TransformedVerts;
extern zVec3 *g_zModel_TransformedNormals;
extern zVec3 *g_zModel_SharedVec3ScratchA;
extern zVec3 *g_zModel_SharedVec3ScratchB;
extern zVec3 *g_zModel_PointInPolygonVertices;
extern zVec3 *g_zModel_PointInPolygonEdgeNormals;
extern int g_zModel_PointInPolygonVertexCount;
extern float g_zModel_TextureWorldBaseU;
extern float g_zModel_TextureWorldBaseV;
extern float g_zModel_TextureWorldPerMeterU;
extern float g_zModel_TextureWorldPerMeterV;

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
    zModel_TextureScrollInfoPartial *textureInfo;
};

struct zModel_MaterialTextureBindingPartial {
    unsigned char unknown_00;
    unsigned char flags;
    unsigned char unknown_02[0x0e];
    zModel_TextureRefPartial *textureRef;
};

struct zModel_InstanceSurfaceEntryPartial {
    unsigned int vertexCountAndFlags;
    unsigned char unknown_04[0x0c];
    zModel_Uv *uvs;
    zModel_MaterialTextureBindingPartial *materialBinding;
    unsigned char unknown_18[0x04];
};

struct zModel_InstancePartial {
    unsigned char unknown_00[0x0c];
    int surfaceEntryCount;
    unsigned char unknown_10[0x14];
    float scrollRateU;
    float scrollRateV;
    int scrollingTextureFrameTick;
    zModel_InstanceSurfaceEntryPartial *surfaceEntries;
};

RECOIL_STATIC_ASSERT(sizeof(zModel_Uv) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_TextureScrollInfoPartial,
        wrapShiftU
    ) == 0x0a
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_TextureScrollInfoPartial,
        wrapShiftV
    ) == 0x0b
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialTextureBindingPartial,
        flags
    ) == 0x01
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_MaterialTextureBindingPartial,
        textureRef
    ) == 0x10
);
RECOIL_STATIC_ASSERT(sizeof(zModel_InstanceSurfaceEntryPartial) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_InstanceSurfaceEntryPartial,
        uvs
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_InstanceSurfaceEntryPartial,
        materialBinding
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_InstancePartial,
        surfaceEntryCount
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_InstancePartial,
        scrollRateU
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_InstancePartial,
        scrollingTextureFrameTick
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zModel_InstancePartial,
        surfaceEntries
    ) == 0x30
);

int __fastcall zModel_Instance_UpdateScrollingTexturesIfNeeded(
    zModel_InstancePartial *instance
);
void __fastcall zModel_Instance_UpdateScrollingTextures(
    const zModel_TextureScrollInfoPartial *textureInfo,
    zModel_Uv *uvs,
    const float *scrollRates,
    int uvCount
);
void __fastcall zModel_RenderPointQueueEntry(
    const zVec3 *pointPos,
    int packedColor16,
    zModel_PointEntryPartial *pointEntry
);
int __fastcall zModel_Light_BuildLightWeights(
    zVec3 *surfaceNormal,
    int vertexCount,
    int *outPackedFogColor,
    float fogBlendScale
);
void __fastcall zModel_Light_PointInPolygonInitXZ(
    zClass_LightDataPartial **lightDataList,
    zModel_LightStatePartial **lightNodeStates,
    int lightCount
);

namespace zModel {
int __cdecl Init();
void __fastcall SetVertexShadingEnabled(int enabled);
void __fastcall SetDisplayInstancePoolCapacity(int capacity);
void __fastcall SetSoftwarePathActive(int active);
void __stdcall SetTextureWorldPerMeter(
    float worldPerMeterU,
    float worldPerMeterV
);
void __stdcall SetTextureWorldBase(
    float worldBaseU,
    float worldBaseV
);
int __fastcall SetDiTextureWorldPerMeter(
    zDiPartial *di,
    int worldSpaceEnabled,
    float textureWorldPerMeter,
    int textureWorldAxis
);
void __fastcall RenderNodeHardware(
    zClass_NodePartial *node,
    int clipMask
);
void __fastcall RenderNodeSoftware(
    zClass_NodePartial *node,
    int clipMask
);
void __stdcall SetBackfaceEliminationToleranceScalar(float scalar);
float GetBackfaceEliminationToleranceScalar();
void __stdcall UpdateSmallPolyRejectThresholds(float baseRejectArea);
} // namespace zModel

int zModel_Display_Init();
void __stdcall OptCatalog_SetDamageMaskUv(
    float u,
    float v
);
int OptCatalog_IsDamageMaskEnabled();
void __fastcall OptCatalog_SetDamageMaskEnabled(int enabled);
int __fastcall OptCatalog_IsDamageMaskSlotPtrRegistered(void *slotPtr);
void __fastcall zModel_Fog_SetEnabled(int enabled);
int zModel_Fog_IsEnabled();
void __stdcall zModel_Fog_SetDistanceStart(float distanceStart);
float zModel_Fog_GetDistanceStart();
void __stdcall zModel_Fog_SetDistanceEnd(float distanceEnd);
void __stdcall zModel_Fog_SetHeightHigh(float heightHigh);
void __stdcall zModel_Fog_SetHeightLow(float heightLow);
void __stdcall zModel_Fog_SetDensity(float density);
void __fastcall zModel_Fog_SetLinearModeEnabled(int enabled);
void __fastcall zModel_Fog_SetColorRgb01(zColorRgb *rgb01);
void zModel_Fog_ApplyCurrentColor();

namespace zModel_Light {
float __fastcall EvalDistanceWeight(
    const zClass_LightDataPartial *light,
    float distance
);
float __fastcall EvalSphereFogFade(
    const zVec3 *point,
    float radius
);
int __fastcall BuildAttr0DepthFade(
    int vertexCount,
    int *outHasVariation
);
int __fastcall BuildAttr1Falloff(
    int vertexCount,
    int *pLightingFlags
);
int __fastcall EvalBatchSphereFade(float *outFade);
int __fastcall PointInPolygonTestRadiusXZ(
    const zVec3 *sphereCenter,
    float radius
);
int __fastcall SetActiveLights(
    zVec3 *surfaceNormal,
    int vertexCount,
    int *lightFlags,
    int *lightingMode,
    int usePaletteRemap
);
} // namespace zModel_Light

namespace zModel_DiPool {
int __fastcall WriteToStream(void *stream);
int __fastcall ReadHeaderFromStream(
    void *stream,
    int *outCapacity,
    int *outInUseCount,
    int *outFreeHeadIndex
);
int __fastcall ReadEntryDynamicDataFromStream(
    void *stream,
    zDiPartial *entry
);
RECOIL_NO_GS zDiPartial *__fastcall ReadEntryByIndexFromStream(
    void *stream,
    int index
);
int __fastcall ReadFromStream(void *stream);
zDiPartial *AllocFromFreeList();
int __fastcall FreeIfUnreferenced(zDiPartial *di);
} // namespace zModel_DiPool

namespace zModel_Const {
float GetVertexMergeEpsilon();
void __stdcall SetVertexMergeEpsilon(float epsilon);
void __stdcall SetCoplanarTolerance(float tolerance);
void __stdcall SetColinearTolerance(float tolerance);
zVec3 *__fastcall SetNormalizedCrossFromVertexTriplet(
    zVec3 *vertex0,
    zVec3 *vertex1,
    zVec3 *outNormal,
    zVec3 *vertex2
);
int __fastcall RemoveColinearVerticesInPlace(
    int *vertexCount,
    zVec3 *points,
    zClipUV *uvPairsA,
    zVec3 *normalsB,
    zClipUV *uvPairsB
);
zGeometry_PlaneEquationPartial *__fastcall ComputePolygonPlaneEquation(
    int vertexCount,
    zVec3 *vertices,
    zGeometry_PlaneEquationPartial *outPlane
);
int __fastcall IsPolygonCoplanar(
    int vertexCount,
    zVec3 *vertices
);
int __fastcall AddOrMergeVertex(
    zDiPartial *self,
    zVec3 *point
);
int __fastcall AddOrMergeVertexAndNormal(
    zDiPartial *self,
    zVec3 *point,
    zVec3 *normal
);
int __fastcall FindOrAppendNormalIndex(
    zDiPartial *self,
    zVec3 *normal
);
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
void __fastcall QuantizeAndNormalizeUvPairs(
    int vertexCount,
    zClipUV *uvPairs
);
void __fastcall SplitPolygonChunkedByVertexLimit(
    zDiPartial *self,
    int totalVertexCount,
    zVec3 *points,
    zVec3 *entryNormals,
    zClipUV *uvPairsA,
    zVec3 *normalsA,
    zVec3 *normalsBInput,
    zClipUV *uvPairsBInput,
    zModel_MaterialPartial *material,
    unsigned int drawFlags,
    int flagBit8,
    const int *userTag
);
} // namespace zModel_Const

namespace zModel_MatlBuffer {
void __fastcall SetArraySize(int count);
zModel_MaterialPartial *__fastcall CloneToActiveSlot(
    zModel_MaterialPartial *material
);
int __fastcall WriteGameZ(void *stream);
int __fastcall ReadGameZ(void *stream);
int ReleaseAllActive();
void __cdecl ReleaseTextureSurfaces();
int Shutdown();
} // namespace zModel_MatlBuffer

namespace zModel_Matl {
int InitGlobals();
zModel_MaterialSlot *__fastcall GetPoolEntry(int index);
} // namespace zModel_Matl

namespace zModel_MatlSlot {
void __fastcall Release(zModel_MaterialSlot *slot);
int __fastcall IndexFromPtrOrMinus1(zModel_MaterialSlot *slot);
} // namespace zModel_MatlSlot

namespace zModel_Display {
int Reset();
int __cdecl Shutdown();
int ShutdownThunk();
} // namespace zModel_Display

namespace zScene {
int __fastcall TestProjectedSphereVisible(
    zVec3 *center,
    float radius
);
}

void __fastcall zModel_FogTargetColorOverride_SetCurrent(
    zColorRgb *colorRgb01,
    float weight
);
void __stdcall zModel_RenderAlphaScale_SetCurrent(float scale);
void __fastcall zModel_RenderVertexAlphaEnabled_SetCurrent(int enabled);

namespace VariantTag {
int __fastcall TagsOverlap(
    const zTag4Partial *tagA,
    const zTag4Partial *tagB
);
int __fastcall CurrentAllowsId(int variantId);
} // namespace VariantTag

namespace zDi {
void __fastcall EvalBoundingSphereLightingFlags(
    zDiPartial *self,
    int *outDepthFade,
    int *outActiveLightState,
    int *outLensFlareVisible
);
}
