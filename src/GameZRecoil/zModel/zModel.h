#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/include/zDi.h"
#include "recoil/recoil_callconv.h"

struct zGeometry_PlaneEquationPartial;

extern int gModel_FogEnabled;
extern int gModel_FogLinearModeEnabled;
extern float gModel_FogDistanceStart;
extern float gModel_FogDistanceEnd;
extern float gModel_FogDistanceInvRange;
extern float gModel_FogHeightHigh;
extern float gModel_FogHeightLow;
extern float gModel_FogHeightInvRange;
extern float gModel_FogDensity;
extern zColorRgb gModel_FogColorRgb01;
extern float gModel_SmallPolyRejectArea2x;
extern float gModel_SmallPolyRejectArea20x;
extern float g_OptCatalogDamageMaskPhaseU;
extern float g_OptCatalogDamageMaskPhaseV;
extern int g_OptCatalogDamageMaskEnabled;
extern int g_OptCatalogDamageMaskSlotIndex;
extern int gModel_DefaultGraphicsFlags;
extern int gModel_RenderVertexAlphaEnabled;
extern float gModel_RenderAlphaScaleCurrent;
extern void *g_zModel_GraphicsFlagsOption;
extern int g_Variant_FilterEnabled;
extern zTag4Partial g_VariantTag_Current;
extern zTag4Partial g_Variant_CurrentTag;
extern void *g_OptCatalogDamageMaskHandles[3];
extern int g_zModel_DisplayClipMode;
extern int g_zModel_DisplayClipX;
extern int g_zModel_DisplayClipY;
extern float g_zModel_InverseZTolerance;
extern float g_zModel_HardwareInverseZTolerance;
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

RECOIL_STATIC_ASSERT(offsetof(zModel_LightStatePartial, flags) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(zModel_ActiveLightEntryLive) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_ActiveLightEntryLive, useFullWeight) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zModel_ActiveLightEntryLive, contributesToLighting) == 0x0c);

struct zModel_FogTargetColorOverride {
    zColorRgb colorRgb01;
    float weight;
};

RECOIL_STATIC_ASSERT(offsetof(zModel_FogTargetColorOverride, weight) == 0x0c);
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

struct zModel_PaletteRemapRecipePartial {
    float color0R;
    float color0G;
    float color0B;
    float color1R;
    float color1G;
    float color1B;
    float color0Strength;
    float color1Strength;
};

RECOIL_STATIC_ASSERT(sizeof(zModel_PaletteRemapRecipePartial) == 0x20);

extern zModel_PaletteRemapRecipePartial gModel_SpecialLightPaletteRemapRecipe;

struct zModel_MaterialSlot {
    zModel_MaterialPartial material;
    short prevPoolIndex;
    short nextPoolIndex;
};

RECOIL_STATIC_ASSERT(sizeof(zModel_MaterialSlot) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialSlot, prevPoolIndex) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialSlot, nextPoolIndex) == 0x2a);

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
RECOIL_STATIC_ASSERT(offsetof(zModel_TextureScrollInfoPartial, wrapShiftU) == 0x0a);
RECOIL_STATIC_ASSERT(offsetof(zModel_TextureScrollInfoPartial, wrapShiftV) == 0x0b);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialTextureBindingPartial, flags) == 0x01);
RECOIL_STATIC_ASSERT(offsetof(zModel_MaterialTextureBindingPartial, textureRef) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(zModel_InstanceSurfaceEntryPartial) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstanceSurfaceEntryPartial, uvs) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstanceSurfaceEntryPartial, materialBinding) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstancePartial, surfaceEntryCount) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstancePartial, scrollRateU) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstancePartial, scrollingTextureFrameTick) == 0x2c);
RECOIL_STATIC_ASSERT(offsetof(zModel_InstancePartial, surfaceEntries) == 0x30);

RECOIL_NOINLINE int RECOIL_FASTCALL
zModel_Instance_UpdateScrollingTexturesIfNeeded(zModel_InstancePartial *instance);
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Instance_UpdateScrollingTextures(
    const zModel_TextureScrollInfoPartial *textureInfo, zModel_Uv *uvs, const float *scrollRates,
    int uvCount);
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_RenderPointQueueEntry(
    const zVec3 *pointPos, int packedColor16, zModel_PointEntryPartial *pointEntry);
RECOIL_NOINLINE int RECOIL_FASTCALL
zModel_Light_BuildLightWeights(zVec3 *surfaceNormal, int vertexCount,
                               int *outPackedFogColor, float fogBlendScale);
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Light_PointInPolygonInitXZ(
    zClass_LightDataPartial **lightDataList, zModel_LightStatePartial **lightNodeStates,
    int lightCount);

namespace zModel {
RECOIL_NOINLINE int RECOIL_CDECL Init();
RECOIL_NOINLINE void RECOIL_FASTCALL SetVertexShadingEnabled(int enabled);
RECOIL_NOINLINE void RECOIL_FASTCALL SetDisplayInstancePoolCapacity(int capacity);
RECOIL_NOINLINE void RECOIL_FASTCALL SetSoftwarePathActive(int active);
RECOIL_NOINLINE void RECOIL_STDCALL SetTextureWorldPerMeter(float worldPerMeterU,
                                                            float worldPerMeterV);
RECOIL_NOINLINE void RECOIL_STDCALL SetTextureWorldBase(float worldBaseU,
                                                        float worldBaseV);
RECOIL_NOINLINE int RECOIL_FASTCALL SetDiTextureWorldPerMeter(zDiPartial *di,
                                                              int worldSpaceEnabled,
                                                              float textureWorldPerMeter,
                                                              int textureWorldAxis);
RECOIL_NOINLINE void RECOIL_FASTCALL RenderNodeHardware(zClass_NodePartial *node, int clipMask);
RECOIL_NOINLINE void RECOIL_STDCALL SetBackfaceEliminationToleranceScalar(float scalar);
RECOIL_NOINLINE float RECOIL_CDECL GetBackfaceEliminationToleranceScalar();
RECOIL_NOINLINE void RECOIL_STDCALL UpdateSmallPolyRejectThresholds(float baseRejectArea);
} // namespace zModel

RECOIL_NOINLINE int RECOIL_CDECL zModel_Display_Init();
RECOIL_NOINLINE void RECOIL_STDCALL OptCatalog_SetDamageMaskUv(float u, float v);
RECOIL_NOINLINE int RECOIL_CDECL OptCatalog_IsDamageMaskEnabled();
RECOIL_NOINLINE void RECOIL_FASTCALL OptCatalog_SetDamageMaskEnabled(int enabled);
RECOIL_NOINLINE int RECOIL_FASTCALL
OptCatalog_IsDamageMaskSlotPtrRegistered(void *slotPtr);
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Fog_SetEnabled(int enabled);
RECOIL_NOINLINE int RECOIL_CDECL zModel_Fog_IsEnabled();
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetDistanceStart(float distanceStart);
RECOIL_NOINLINE float RECOIL_CDECL zModel_Fog_GetDistanceStart();
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetDistanceEnd(float distanceEnd);
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetHeightHigh(float heightHigh);
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetHeightLow(float heightLow);
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetDensity(float density);
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Fog_SetLinearModeEnabled(int enabled);
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Fog_SetColorRgb01(zColorRgb *rgb01);
RECOIL_NOINLINE void RECOIL_CDECL zModel_Fog_ApplyCurrentColor();

namespace zModel_Light {
RECOIL_NOINLINE float RECOIL_FASTCALL EvalDistanceWeight(const zClass_LightDataPartial *light,
                                                         float distance);
RECOIL_NOINLINE float RECOIL_FASTCALL EvalSphereFogFade(const zVec3 *point, float radius);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildAttr0DepthFade(int vertexCount,
                                                                 int *outHasVariation);
RECOIL_NOINLINE int RECOIL_FASTCALL BuildAttr1Falloff(int vertexCount,
                                                      int *pLightingFlags);
RECOIL_NOINLINE int RECOIL_FASTCALL EvalBatchSphereFade(float *outFade);
RECOIL_NOINLINE int RECOIL_FASTCALL PointInPolygonTestRadiusXZ(const zVec3 *sphereCenter,
                                                                        float radius);
RECOIL_NOINLINE int RECOIL_FASTCALL SetActiveLights(zVec3 *surfaceNormal, int vertexCount,
                                                    int *lightFlags, int *lightingMode,
                                                    int usePaletteRemap);
} // namespace zModel_Light

namespace zModel_DiPool {
RECOIL_NOINLINE int RECOIL_FASTCALL WriteToStream(void *stream);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadHeaderFromStream(void *stream,
                                                                  int *outCapacity,
                                                                  int *outInUseCount,
                                                                  int *outFreeHeadIndex);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadEntryDynamicDataFromStream(void *stream,
                                                                            zDiPartial *entry);
RECOIL_NOINLINE RECOIL_NO_GS zDiPartial *RECOIL_FASTCALL
ReadEntryByIndexFromStream(void *stream, int index);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadFromStream(void *stream);
RECOIL_NOINLINE zDiPartial *RECOIL_CDECL AllocFromFreeList();
RECOIL_NOINLINE int RECOIL_FASTCALL FreeIfUnreferenced(zDiPartial *di);
} // namespace zModel_DiPool

namespace zModel_Const {
RECOIL_NOINLINE float RECOIL_CDECL GetVertexMergeEpsilon();
RECOIL_NOINLINE void RECOIL_STDCALL SetVertexMergeEpsilon(float epsilon);
RECOIL_NOINLINE void RECOIL_STDCALL SetCoplanarTolerance(float tolerance);
RECOIL_NOINLINE void RECOIL_STDCALL SetColinearTolerance(float tolerance);
RECOIL_NOINLINE zVec3 *RECOIL_FASTCALL
SetNormalizedCrossFromVertexTriplet(zVec3 *vertex0, zVec3 *vertex1,
                                    zVec3 *outNormal, zVec3 *vertex2);
RECOIL_NOINLINE int RECOIL_FASTCALL
RemoveColinearVerticesInPlace(int *vertexCount, zVec3 *points, zClipUV *uvPairsA,
                              zVec3 *normalsB, zClipUV *uvPairsB);
RECOIL_NOINLINE zGeometry_PlaneEquationPartial *RECOIL_FASTCALL
ComputePolygonPlaneEquation(int vertexCount, zVec3 *vertices,
                            zGeometry_PlaneEquationPartial *outPlane);
RECOIL_NOINLINE int RECOIL_FASTCALL IsPolygonCoplanar(int vertexCount, zVec3 *vertices);
RECOIL_NOINLINE int RECOIL_FASTCALL AddOrMergeVertex(zDiPartial *self, zVec3 *point);
RECOIL_NOINLINE int RECOIL_FASTCALL
AddOrMergeVertexAndNormal(zDiPartial *self, zVec3 *point, zVec3 *normal);
RECOIL_NOINLINE int RECOIL_FASTCALL FindOrAppendNormalIndex(zDiPartial *self,
                                                            zVec3 *normal);
RECOIL_NOINLINE zClipUV RECOIL_STDCALL
SolveTriScalarGradient2D(float vertex0A, float vertex0B, float vertex1A,
                         float vertex1B, float vertex2A, float vertex2B,
                         float value0, float value1, float value2);
RECOIL_NOINLINE void RECOIL_FASTCALL QuantizeAndNormalizeUvPairs(int vertexCount,
                                                                 zClipUV *uvPairs);
RECOIL_NOINLINE void RECOIL_FASTCALL
SplitPolygonChunkedByVertexLimit(zDiPartial *self, int totalVertexCount, zVec3 *points,
                                 zVec3 *entryNormals, zClipUV *uvPairsA, zVec3 *normalsA,
                                 zVec3 *normalsBInput, zClipUV *uvPairsBInput,
                                 zModel_MaterialPartial *material, unsigned int drawFlags,
                                 int flagBit8, const int *userTag);
} // namespace zModel_Const

namespace zModel_MatlBuffer {
RECOIL_NOINLINE void RECOIL_FASTCALL SetArraySize(int count);
RECOIL_NOINLINE zModel_MaterialPartial *RECOIL_FASTCALL
CloneToActiveSlot(zModel_MaterialPartial *material);
RECOIL_NOINLINE int RECOIL_FASTCALL WriteGameZ(void *stream);
RECOIL_NOINLINE int RECOIL_FASTCALL ReadGameZ(void *stream);
RECOIL_NOINLINE int RECOIL_CDECL ReleaseAllActive();
RECOIL_NOINLINE void RECOIL_CDECL ReleaseTextureSurfaces();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
} // namespace zModel_MatlBuffer

namespace zModel_Matl {
RECOIL_NOINLINE int RECOIL_CDECL InitGlobals();
RECOIL_NOINLINE zModel_MaterialSlot *RECOIL_FASTCALL GetPoolEntry(int index);
}

namespace zModel_MatlSlot {
RECOIL_NOINLINE void RECOIL_FASTCALL Release(zModel_MaterialSlot *slot);
RECOIL_NOINLINE int RECOIL_FASTCALL IndexFromPtrOrMinus1(zModel_MaterialSlot *slot);
}

namespace zModel_Display {
RECOIL_NOINLINE int RECOIL_CDECL Reset();
RECOIL_NOINLINE int RECOIL_CDECL Shutdown();
RECOIL_NOINLINE int RECOIL_CDECL ShutdownThunk();
} // namespace zModel_Display

namespace zScene {
RECOIL_NOINLINE int RECOIL_FASTCALL TestProjectedSphereVisible(zVec3 *center,
                                                                        float radius);
}

RECOIL_NOINLINE void RECOIL_FASTCALL zModel_FogTargetColorOverride_SetCurrent(zColorRgb *colorRgb01,
                                                                              float weight);
RECOIL_NOINLINE void RECOIL_STDCALL zModel_RenderAlphaScale_SetCurrent(float scale);
RECOIL_NOINLINE void RECOIL_FASTCALL
zModel_RenderVertexAlphaEnabled_SetCurrent(int enabled);

namespace VariantTag {
RECOIL_NOINLINE int RECOIL_FASTCALL TagsOverlap(const zTag4Partial *tagA,
                                                         const zTag4Partial *tagB);
RECOIL_NOINLINE int RECOIL_FASTCALL CurrentAllowsId(int variantId);
}

namespace zDi {
RECOIL_NOINLINE void RECOIL_FASTCALL EvalBoundingSphereLightingFlags(
    zDiPartial *self, int *outDepthFade, int *outActiveLightState,
    int *outLensFlareVisible);
}
