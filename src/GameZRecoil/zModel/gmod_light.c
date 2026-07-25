#include "GameZRecoil/zModel/gmod.h"

#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <math.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-g-zmodel-sourcefile-gmodlightc
 * @recoil-artifact defines .data recoil:data:0x4e17f8: g_zModel_SourceFile_GmodLightC.
 * Data owner: geometry_model_assets.zmodel_gmod_light_diagnostics_data.
 * Purpose: store the writable gmod_light.c source-file path used by model
 * lighting diagnostics.
 *
 * Retail 0x4e17f8: initialized .data char[0x28] literal
 * "D:\\Proj\\GameZRecoil\\zModel\\gmod_light.c".
 */
char g_zModel_SourceFile_GmodLightC[0x28] =
    "D:\\Proj\\GameZRecoil\\zModel\\gmod_light.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-g-zmodel-maxlightsrequestfmt
 * @recoil-artifact defines .data recoil:data:0x4e1820: g_zModel_MaxLightsRequestFmt.
 * Data owner: geometry_model_assets.zmodel_gmod_light_diagnostics_data.
 * Purpose: store the writable active-light overflow diagnostic format.
 */
char g_zModel_MaxLightsRequestFmt[0x2c] =
    "Not enough MAX_LIGHTS: %d; requesting more.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-g-zmodel-nevergetheremsg
 * @recoil-artifact defines .data recoil:data:0x4e184c: g_zModel_NeverGetHereMsg.
 * Data owner: geometry_model_assets.zmodel_gmod_light_diagnostics_data.
 * Purpose: store the writable active-light unreachable-state diagnostic.
 */
char g_zModel_NeverGetHereMsg[0x10] = "Never get here?";
RECOIL_STATIC_ASSERT(sizeof(g_zModel_SourceFile_GmodLightC) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_MaxLightsRequestFmt) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(g_zModel_NeverGetHereMsg) == 0x10);

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-fogenabled
 * @recoil-artifact defines .data recoil:data:0x57d930: gModel_FogEnabled.
 * Authored zModel light/fog global.
 * Purpose: gate fog calculations during model lighting and vertex color setup.
 */
int gModel_FogEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-foglinearmodeenabled
 * @recoil-artifact defines .data recoil:data:0x57d934: gModel_FogLinearModeEnabled.
 * Authored zModel light/fog global.
 * Purpose: select the linear distance fog path for active model rendering.
 */
int gModel_FogLinearModeEnabled = 0;
zColorRgb gModel_FogColorRgb01 = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-fogdistancestart
 * @recoil-artifact defines .data recoil:data:0x57d944: gModel_FogDistanceStart.
 * Authored zModel light/fog global.
 * Purpose: store the near distance where linear fog begins.
 */
float gModel_FogDistanceStart = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-fogdistanceend
 * @recoil-artifact defines .data recoil:data:0x57d948: gModel_FogDistanceEnd.
 * Authored zModel light/fog global.
 * Purpose: store the far distance where linear fog reaches full strength.
 */
float gModel_FogDistanceEnd = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-fogdistanceinvrange
 * @recoil-artifact defines .data recoil:data:0x57d94c: gModel_FogDistanceInvRange.
 * Authored zModel light/fog global.
 * Purpose: cache the reciprocal distance-fog range used by fade calculations.
 */
float gModel_FogDistanceInvRange = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-fogheighthigh
 * @recoil-artifact defines .data recoil:data:0x57d950: gModel_FogHeightHigh.
 * Authored zModel light/fog global.
 * Purpose: store the upper height threshold for height fog.
 */
float gModel_FogHeightHigh = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-fogheightlow
 * @recoil-artifact defines .data recoil:data:0x57d954: gModel_FogHeightLow.
 * Authored zModel light/fog global.
 * Purpose: store the lower height threshold for height fog.
 */
float gModel_FogHeightLow = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-fogheightinvrange
 * @recoil-artifact defines .data recoil:data:0x57d958: gModel_FogHeightInvRange.
 * Authored zModel light/fog global.
 * Purpose: cache the reciprocal height-fog range used by fade calculations.
 */
float gModel_FogHeightInvRange = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-fogdensity
 * @recoil-artifact defines .data recoil:data:0x57d95c: gModel_FogDensity.
 * Authored zModel light/fog global.
 * Purpose: store the density scalar used by model fog shading.
 */
float gModel_FogDensity = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-rendervertexalphaenabled
 * @recoil-artifact defines .data recoil:data:0x57d960: gModel_RenderVertexAlphaEnabled.
 * Authored zModel light/fog global.
 * Purpose: gate per-vertex alpha output during model rendering.
 */
int gModel_RenderVertexAlphaEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-renderalphascalecurrent
 * @recoil-artifact defines .data recoil:data:0x57d964: gModel_RenderAlphaScaleCurrent.
 * Authored zModel light/fog global.
 * Purpose: store the current alpha scale applied to rendered vertices.
 */
float gModel_RenderAlphaScaleCurrent = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-hasactivelights
 * @recoil-artifact defines .data recoil:data:0x57d418: gModel_HasActiveLights.
 * Authored zModel active-light global.
 * Purpose: record whether the current light scan found any active lights.
 */
int gModel_HasActiveLights = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-activelightcount
 * @recoil-artifact defines .data recoil:data:0x57d420: gModel_ActiveLightCount.
 * Authored zModel active-light global.
 * Purpose: track the number of entries populated in the active-light array.
 */
int gModel_ActiveLightCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-activelightspecialindex
 * @recoil-artifact defines .data recoil:data:0x57d424: gModel_ActiveLightSpecialIndex.
 * Authored zModel active-light global.
 * Purpose: remember the special ambient-modulating active light index.
 */
int gModel_ActiveLightSpecialIndex = 0;
zModel_ActiveLightEntryLive gModel_ActiveLights[0x40] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-lightinputdatalist
 * @recoil-artifact defines .data recoil:data:0x57d414: gModel_LightInputDataList.
 * Authored zModel active-light global.
 * Purpose: point at the current caller-supplied light data pointer list.
 */
zClass_LightDataPartial **gModel_LightInputDataList = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-lightinputnodestates
 * @recoil-artifact defines .data recoil:data:0x57d410: gModel_LightInputNodeStates.
 * Authored zModel active-light global.
 * Purpose: point at the current caller-supplied light node-state list.
 */
zModel_LightStatePartial **gModel_LightInputNodeStates = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-lightinputcount
 * @recoil-artifact defines .data recoil:data:0x57d41c: gModel_LightInputCount.
 * Authored zModel active-light global.
 * Purpose: store the number of caller-supplied light inputs to scan.
 */
int gModel_LightInputCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-g-zmodel-softwarepathactive
 * @recoil-artifact defines .data recoil:data:0x57d9c8: g_zModel_SoftwarePathActive.
 * Authored zModel light/fog global.
 * Purpose: gate software-renderer lighting paths that need polygon normals.
 */
int g_zModel_SoftwarePathActive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-lightvertexdistancesqscratch
 * @recoil-artifact defines .data recoil:data:0x566a28: gModel_LightVertexDistanceSqScratch.
 * Data owner: engine.zmodel.light_vertex_distance_scratch.
 * Purpose: store per-light/per-vertex distance scratch values while building
 * model light weights.
 */
float gModel_LightVertexDistanceSqScratch[0x40][0x40] = {0};
RECOIL_STATIC_ASSERT(sizeof(gModel_LightVertexDistanceSqScratch) == 0x4000);
float g_Clip_PolyAttr0[0x40] = {0};
float g_Clip_PolyAttr1[0x40] = {0};
float g_Clip_PolyAttr2[0x40] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-g-zmodel-currentpolynormals
 * @recoil-artifact defines .data recoil:data:0x57d0c8: g_zModel_CurrentPolyNormals.
 * Authored zModel active-light global.
 * Purpose: point lighting code at the current polygon normal scratch buffer.
 */
zVec3 *g_zModel_CurrentPolyNormals = 0;
zVec3 g_zModel_CurrentPolyNormalsStorage[0x40] = {0};
zModel_FogTargetColorOverride g_zModel_FogTargetColorOverride = {0};
zColorRgb gModel_FogBaseColorRgb01 = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-ambientscale
 * @recoil-artifact defines .data recoil:data:0x57d3e8: gModel_AmbientScale.
 * Authored zModel active-light global.
 * Purpose: store the ambient scale used while applying active light results.
 */
float gModel_AmbientScale = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-ambientintensityfactor
 * @recoil-artifact defines .data recoil:data:0x57d3e4: gModel_AmbientIntensityFactor.
 * Authored zModel active-light global.
 * Purpose: store the active-light intensity factor used to adjust ambient light.
 */
float gModel_AmbientIntensityFactor = 0.0f;
zColorRgb gModel_AmbientColorRgb01 = {0};
zVidPaletteRemapRecipe gModel_SpecialLightPaletteRemapRecipe = {0};

namespace {


    /**
     * Original static helper observed in zModel light/fog callers
     * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
     * Purpose: produce the original fast square-root estimate from a float bit
     * pattern for distance and radius checks.
     */
    float ApproximateSqrtFromBits(float value) {
        int bits = 0;
        memcpy(
            &bits,
            &value,
            sizeof(bits)
        );
        bits = (bits >> 1) + 0x1fc00000;

        float result = 0.0f;
        memcpy(
            &result,
            &bits,
            sizeof(result)
        );
        return result;
    }

    /**
     * Original static helper observed in caller 0x489540
     * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
     * Purpose: evaluate vertical fog coverage for a sphere using the cached
     * projected Y coordinate and height-fog range.
     */
    float EvalHeightFogFade(
        const zVec3 *point,
        float radius
    ) {
        float projectedY = 0.0f;
        zMath::Vec3ArrayProjectToCachedY(
            point,
            &projectedY,
            1
        );

        const float bottom = projectedY - radius;
        if (bottom >= gModel_FogHeightHigh) {
            return 0.0f;
        }

        if (projectedY + radius <= gModel_FogHeightLow) {
            return 1.0f;
        }

        const float clampedBottom = bottom < gModel_FogHeightLow ? gModel_FogHeightLow : bottom;
        return (gModel_FogHeightHigh - clampedBottom) * gModel_FogHeightInvRange;
    }

    /**
     * Original static helper observed in zModel light-weight callers
     * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
     * Purpose: clamp a lighting or fog contribution weight to the unit interval.
     */
    float ClampWeight(float weight) {
        if (weight > 1.0f) {
            return 1.0f;
        }

        if (weight < 0.0f) {
            return 0.0f;
        }

        return weight;
    }

    /**
     * Original static helper observed in zModel light-weight callers
     * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
     * Purpose: compute the dot product used for angular light and normal
     * weighting.
     */
    float DotVec3(
        const zVec3 &a,
        const zVec3 &b
    ) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    /**
     * Original static helper observed in zModel light-weight callers
     * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
     * Purpose: subtract view-space vectors for light-to-vertex distance and
     * direction calculations.
     */
    zVec3 SubtractVec3(
        const zVec3 &a,
        const zVec3 &b
    ) {
        zVec3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
        return result;
    }

    /**
     * Original static helper observed in caller 0x487f10
     * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
     * Purpose: test whether a light or fog weight exceeds the visible one-byte
     * attribute threshold.
     */
    bool IsVisibleWeight(float weight) {
        return weight > (1.0f / 255.0f);
    }

    /**
     * Original static helper observed in caller 0x487f10
     * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
     * Purpose: clamp an attribute weight in place and return the clamped value.
     */
    float ClampWeightInPlace(float *weight) {
        *weight = ClampWeight(*weight);
        return *weight;
    }

    /**
     * Original static helper observed in callers 0x487f10 and 0x488d60
     * (D:\Proj\GameZRecoil\zModel\gmod_light.c).
     * Purpose: select either the fog target override color or an active light's
     * specular color for fog-target commits.
     */
    zColorRgb *SelectActiveLightColor(int lightIndex) {
        if (lightIndex < 0) {
            return &g_zModel_FogTargetColorOverride.colorRgb01;
        }

        return &gModel_ActiveLights[lightIndex].light->specularColor;
    }
}















/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-pointinpolygoninitxz
 * @recoil-artifact defines .text recoil:function:0x487a30: zModel_Light_PointInPolygonInitXZ
 * Purpose: seed active light inputs, filter enabled light states, track the
 * active point-light index, and initialize software-path ambient/remap state.
 */
void __fastcall zModel_Light_PointInPolygonInitXZ(
    zClass_LightDataPartial **lightDataList,
    zModel_LightStatePartial **lightNodeStates,
    int lightCount
) {
    gModel_LightInputDataList = lightDataList;
    gModel_LightInputNodeStates = lightNodeStates;
    gModel_LightInputCount = lightCount;
    gModel_ActiveLightCount = 0;
    gModel_ActiveLightSpecialIndex = -1;

    for (int i = 0; i < lightCount; ++i) {
        if ((lightNodeStates[i]->flags & 4) == 0) {
            continue;
        }

        if (gModel_ActiveLightCount == 0x40) {
            zError::ReportOld(
                0x200,
                g_zModel_SourceFile_GmodLightC,
                0x46,
                g_zModel_MaxLightsRequestFmt,
                0x40
            );
            break;
        }

        zModel_ActiveLightEntryLive &active = gModel_ActiveLights[gModel_ActiveLightCount];
        active.light = lightDataList[i];
        active.lightState = lightNodeStates[i];
        active.useFullWeight = 0;
        active.contributesToLighting = 0;
        active.reserved_10 = 0;
        if (lightDataList[i]->isPointMode != 0) {
            gModel_ActiveLightSpecialIndex = gModel_ActiveLightCount;
        }
        ++gModel_ActiveLightCount;
    }

    gModel_HasActiveLights = gModel_ActiveLightCount > 0 ? 1 : 0;
    if (g_zVideo_ActiveRendererPath != 0) {
        return;
    }

    gModel_FogBaseColorRgb01 = gModel_FogColorRgb01;
    gModel_AmbientScale = 1.0f;
    if (gModel_ActiveLightSpecialIndex >= 0) {
        zClass_LightDataPartial *light = gModel_ActiveLights[gModel_ActiveLightSpecialIndex].light;
        gModel_AmbientColorRgb01 = light->specularColor;
        gModel_AmbientIntensityFactor = 1.0f - light->intensityScale;
        gModel_SpecialLightPaletteRemapRecipe.color1R = light->specularColor.red;
        gModel_SpecialLightPaletteRemapRecipe.color1G = light->specularColor.green;
        gModel_SpecialLightPaletteRemapRecipe.color1B = light->specularColor.blue;
        gModel_SpecialLightPaletteRemapRecipe.color1Strength = 1.0f;
        gModel_SpecialLightPaletteRemapRecipe.color0R = light->specularColor.red;
        gModel_SpecialLightPaletteRemapRecipe.color0G = light->specularColor.green;
        gModel_SpecialLightPaletteRemapRecipe.color0B = light->specularColor.blue;
        gModel_SpecialLightPaletteRemapRecipe.color0Strength = 0.0f;
    } else {
        gModel_AmbientIntensityFactor = 0.0f;
        gModel_AmbientColorRgb01 = gModel_FogColorRgb01;
        gModel_SpecialLightPaletteRemapRecipe.color0R = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color0G = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color0B = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color0Strength = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color1R = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color1G = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color1B = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color1Strength = 1.0f;
    }
}

namespace zModel_Light {
    int __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-pointinpolygontestradiusxz
     * @recoil-artifact defines .text recoil:function:0x487c50: zModel_Light::PointInPolygonTestRadiusXZ
     * Purpose: evaluate active light contribution flags and per-light weights
     * for a bounding sphere in view-space XZ/radius terms.
     */
    PointInPolygonTestRadiusXZ(
        const zVec3 *sphereCenter,
        float radius
    ) {
        float lightDistances[0x40] = {0};
        int result = 0;
        int hasSoftwarePointLight = 0;

        for (int i = 0; i < gModel_ActiveLightCount; ++i) {
            zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[i];
            zClass_LightDataPartial *light = entry.light;
            entry.useFullWeight = 0;
            entry.contributesToLighting = 0;

            if ((entry.lightState->flags & 4) == 0) {
                zError::ReportOld(
                    0x200,
                    g_zModel_SourceFile_GmodLightC,
                    0xfa,
                    g_zModel_NeverGetHereMsg
                );
                continue;
            }

            if ((g_zVideo_ActiveRendererPath != 0 && light->isPointMode != 0) ||
                light->enabled == 0) {
                entry.useFullWeight = 1;
                entry.contributesToLighting = 1;
                ++result;
                continue;
            }

            if (light->lightSubMode == 0) {
                continue;
            }

            float distance = 0.0f;
            if (light->isPointMode != 0) {
                gModel_ActiveLightSpecialIndex = i;
                distance = sphereCenter->z;
            } else {
                const zVec3 delta = {light->viewPos.x - sphereCenter->x,
                    light->viewPos.y - sphereCenter->y,
                    light->viewPos.z - sphereCenter->z};
                const float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                distance = distSq == 0.0f ? 0.0f : ApproximateSqrtFromBits(distSq);
            }

            lightDistances[i] = distance - radius;
            const float farEdge = distance + radius;
            if (lightDistances[i] >= light->range2 && light->isPointMode == 0) {
                continue;
            }

            entry.contributesToLighting = 1;
            if (farEdge < light->range1) {
                entry.useFullWeight = 1;
                ++result;
                continue;
            }

            if (light->isPointMode != 0) {
                hasSoftwarePointLight = 1;
            }
            ++result;
        }

        if (result == 0) {
            return 0;
        }

        for (int i_647 = 0; i_647 < gModel_ActiveLightCount; ++i_647) {
            zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[i_647];
            if (entry.contributesToLighting == 0) {
                continue;
            }

            zClass_LightDataPartial *light = entry.light;
            if (hasSoftwarePointLight != 0 && g_zModel_SoftwarePathActive != 0 &&
                light->isPointMode == 0) {
                entry.contributesToLighting = 0;
                --result;
                continue;
            }

            float weight =
                entry.useFullWeight != 0 ? 1.0f : EvalDistanceWeight(
                    light,
                    lightDistances[i_647]
                );
            const float cap = light->falloff + light->intensityScale;
            if (cap < weight) {
                weight = cap;
            }
            weight = ClampWeight(weight);

            if (light->isPointMode != 0) {
                g_Clip_PolyAttr1[i_647] = weight;
            } else {
                g_Clip_PolyAttr0[i_647] = weight;
            }
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-setactivelights
     * @recoil-artifact defines .text recoil:function:0x487f10: zModel_Light::SetActiveLights
     * Purpose: build active-light vertex attributes for software and hardware
     * render paths, including fog target, point-light, attr1, and attr2 state.
     */
    int __fastcall SetActiveLights(
        zVec3 * surfaceNormal,
        int vertexCount,
        int *lightFlags,
        int *lightingMode,
        int usePaletteRemap
    ) {
        const float kVisibleWeight = 1.0f / 255.0f;
        const float kMinPointNormalWeight = 0.00402156916f;
        const float kMinIntensity = 9.99999975e-6f;

        const int initialLightingMode = *lightingMode;
        *lightingMode = 0;

        float scale255 = 0.0f;
        zFloat::Set255f(&scale255);

        bool hasAnyCandidate = false;
        bool valid[0x40][0x40] = {0};
        float distances[0x40][0x40] = {0};
        zVec3 lightToVertex[0x40][0x40] = {0};

        for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            const zVec3 vertex = {g_Clip_PolyVertsScratch[vertexIndex].x,
                g_Clip_PolyVertsScratch[vertexIndex].y,
                g_Clip_PolyVertsScratch[vertexIndex].z};

            for (int lightIndex = 0; lightIndex < gModel_ActiveLightCount; ++lightIndex) {
                zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[lightIndex];
                zClass_LightDataPartial *light = entry.light;

                if (*lightFlags == 1 && light->lightParam != 0 && light->isPointMode == 0) {
                    continue;
                }

                if (entry.contributesToLighting == 0) {
                    continue;
                }

                if (entry.useFullWeight != 0) {
                    valid[lightIndex][vertexIndex] = true;
                    hasAnyCandidate = true;
                    continue;
                }

                if (light->isPointMode != 0) {
                    distances[lightIndex][vertexIndex] = vertex.z;
                    if (vertex.z < light->range2) {
                        lightToVertex[lightIndex][vertexIndex] = vertex;
                        if (vertex.z != 0.0f) {
                            zMath_Vec3_DivScalar(
                                &lightToVertex[lightIndex][vertexIndex],
                                &lightToVertex[lightIndex][vertexIndex],
                                vertex.z
                            );
                        }
                        valid[lightIndex][vertexIndex] = true;
                        hasAnyCandidate = true;
                    }
                    continue;
                }

                zVec3 delta = SubtractVec3(
                    light->viewPos,
                    vertex
                );
                float distanceSq = DotVec3(
                    delta,
                    delta
                );
                distances[lightIndex][vertexIndex] = distanceSq;
                if (distanceSq >= light->range2Sq) {
                    continue;
                }

                if (distanceSq != 0.0f) {
                    distances[lightIndex][vertexIndex] = ApproximateSqrtFromBits(distanceSq);
                    zMath_Vec3_DivScalar(
                        &delta,
                        &delta,
                        distances[lightIndex][vertexIndex]
                    );
                }

                lightToVertex[lightIndex][vertexIndex] = delta;
                valid[lightIndex][vertexIndex] = true;
                hasAnyCandidate = true;
            }
        }

        if (!hasAnyCandidate && !IsVisibleWeight(g_zModel_FogTargetColorOverride.weight)) {
            return 0;
        }

        zMath::Vec3Normalize(surfaceNormal);

        float fogWeights[0x40] = {0};
        memset(
            g_Clip_PolyAttr1,
            0,
            (size_t)(vertexCount) * sizeof(float)
        );
        if (g_zVideo_ActiveRendererPath != 0 && (*lightFlags & 1) == 0) {
            memset(
                g_Clip_PolyAttr2,
                0,
                (size_t)(vertexCount) * sizeof(float)
            );
        }

        int fogContributorCount = 0;
        int pointContributorCount = 0;
        int selectedFogLightIndex = -1;
        int selectedPointLightIndex = -1;
        int hasAttr2Contribution = 0;

        for (int lightIndex = 0; lightIndex < gModel_ActiveLightCount; ++lightIndex) {
            zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[lightIndex];
            if (entry.contributesToLighting == 0) {
                continue;
            }

            zClass_LightDataPartial *light = entry.light;
            float pointSum = 0.0f;
            float attr2Sum = 0.0f;
            float fogSum = 0.0f;

            for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                if (!valid[lightIndex][vertexIndex]) {
                    continue;
                }

                float angularWeight = 1.0f;
                if (light->coneAngle != 0 || light->isPointMode != 0) {
                    if (light->isPointMode != 0 && g_zModel_CurrentPolyNormals != 0) {
                        angularWeight =
                            DotVec3(
                                g_zModel_CurrentPolyNormals[vertexIndex],
                                light->viewDir
                            );
                    } else if (light->isPointMode != 0) {
                        angularWeight =
                            DotVec3(
                                *surfaceNormal,
                                lightToVertex[lightIndex][vertexIndex]
                            );
                    } else {
                        angularWeight = DotVec3(
                            *surfaceNormal,
                            light->viewDir
                        );
                    }

                    if (light->isPointMode != 0 && angularWeight < kMinPointNormalWeight) {
                        angularWeight = kMinPointNormalWeight;
                    }

                    if (light->coneAngle != 0) {
                        float coneWeight =
                            DotVec3(
                                lightToVertex[lightIndex][vertexIndex],
                                light->viewDir
                            );
                        if (light->isPointMode != 0 && g_zModel_CurrentPolyNormals != 0) {
                            coneWeight =
                                DotVec3(
                                    g_zModel_CurrentPolyNormals[vertexIndex],
                                    light->viewDir
                                );
                            if (coneWeight < kMinPointNormalWeight) {
                                coneWeight = kMinPointNormalWeight;
                            }
                        }
                        angularWeight = coneWeight < kVisibleWeight ? 0.0f : coneWeight;
                    }
                }

                if (angularWeight <= kVisibleWeight && light->intensityScale <= kMinIntensity) {
                    continue;
                }

                float intensity = light->falloff * angularWeight + light->intensityScale;
                if (intensity > 1.0f) {
                    intensity = 1.0f;
                } else if (intensity < light->intensityScale) {
                    intensity = light->intensityScale;
                }

                float weight = light->isPointMode != 0 ? 1.0f - intensity : intensity;
                if (entry.useFullWeight == 0) {
                    const float distanceWeight =
                        EvalDistanceWeight(
                            light,
                            distances[lightIndex][vertexIndex]
                        );
                    if (light->isPointMode != 0) {
                        const float farWeight = 1.0f - light->intensityScale;
                        weight = (1.0f - distanceWeight) * (farWeight - weight) + weight;
                        if (weight > farWeight) {
                            weight = farWeight;
                        }
                    } else {
                        weight *= distanceWeight;
                    }
                }

                if (light->isPointMode != 0) {
                    g_Clip_PolyAttr1[vertexIndex] += weight;
                    pointSum += weight;
                } else if (g_zVideo_ActiveRendererPath != 0 && light->lightParam != 0) {
                    g_Clip_PolyAttr2[vertexIndex] += weight;
                    attr2Sum += weight;
                    hasAttr2Contribution = 1;
                } else {
                    fogWeights[vertexIndex] += weight;
                    fogSum += weight;
                }
            }

            if (attr2Sum + fogSum > kVisibleWeight) {
                selectedFogLightIndex = lightIndex;
                ++fogContributorCount;
            }

            if (pointSum > kVisibleWeight) {
                selectedPointLightIndex = lightIndex;
                ++pointContributorCount;
            }
        }

        if (IsVisibleWeight(g_zModel_FogTargetColorOverride.weight)) {
            ++fogContributorCount;
            selectedFogLightIndex = -1;
            if (g_zVideo_ActiveRendererPath == 0) {
                for (int i = 0; i < vertexCount; ++i) {
                    fogWeights[i] += g_zModel_FogTargetColorOverride.weight;
                }
            } else {
                for (int i = 0; i < vertexCount; ++i) {
                    g_Clip_PolyAttr2[i] += g_zModel_FogTargetColorOverride.weight;
                }
                hasAttr2Contribution = 1;
            }
        }

        if (fogContributorCount == 0 && pointContributorCount == 0) {
            return 0;
        }

        int pointAttrsVisible = 0;
        if (pointContributorCount > 0 || selectedPointLightIndex >= 0) {
            ClampWeightInPlace(&g_Clip_PolyAttr1[0]);
            pointAttrsVisible = IsVisibleWeight(g_Clip_PolyAttr1[0]) ? 1 : 0;
            int attr1Varies = 0;
            for (int i = 1; i < vertexCount; ++i) {
                ClampWeightInPlace(&g_Clip_PolyAttr1[i]);
                if (fabs(g_Clip_PolyAttr1[i] - g_Clip_PolyAttr1[0]) >= kVisibleWeight) {
                    attr1Varies = 1;
                }
                if (IsVisibleWeight(g_Clip_PolyAttr1[i])) {
                    pointAttrsVisible = 1;
                }
            }

            if (g_zVideo_ActiveRendererPath == 0 && g_zModel_SoftwarePathActive != 0 &&
                usePaletteRemap != 0) {
                if (attr1Varies != 0 && initialLightingMode != 0) {
                    for (int i = 0; i < vertexCount; ++i) {
                        g_Clip_PolyAttr0[i] = g_Clip_PolyAttr1[i] * scale255;
                    }
                    zRndr_SetPaletteShadeRecipeIndex(
                        &gModel_SpecialLightPaletteRemapRecipe
                    );
                    *lightingMode |= 1;
                    return 1;
                }

                if (pointAttrsVisible != 0) {
                    g_Clip_PolyAttr1[0] *= scale255;
                    zRndr_SetPaletteRemapKey(
                        &gModel_SpecialLightPaletteRemapRecipe,
                        g_Clip_PolyAttr1[0]
                    );
                } else if (IsVisibleWeight(g_zModel_FogTargetColorOverride.weight)) {
                    zRndr_SetPaletteRemapKeyFromRgb01(
                        0,
                        0.0f
                    );
                }
            } else if (pointAttrsVisible != 0) {
                *lightingMode |= 1;
            }
        }

        int resultFlags = 0;
        for (int i = 0; i < vertexCount; ++i) {
            ClampWeightInPlace(&fogWeights[i]);
            if (IsVisibleWeight(fogWeights[i])) {
                resultFlags = 1;
                g_Clip_PolyAttr0[i] +=
                    g_zVideo_ActiveRendererPath == 0 ? fogWeights[i] * scale255 : fogWeights[i];
            }
        }

        if (g_zVideo_ActiveRendererPath == 0) {
            if (resultFlags != 0) {
                if ((*lightFlags & 1) != 0 && fogContributorCount > 0) {
                    zRndr::CommitDirectFogParamsIfChanged();
                } else if (fogContributorCount > 1) {
                    zRndr::CommitDirectFogParamsIfChanged();
                } else if (fogContributorCount == 1) {
                    zRndr_FogTargetColorStaged_SetRgb01Clamped(
                        SelectActiveLightColor(selectedFogLightIndex)
                    );
                    zRndr::CommitStagedFogParamsIfChanged();
                }
            }
            return resultFlags;
        }

        if (hasAttr2Contribution != 0) {
            int attr2Visible = 0;
            for (int i = 0; i < vertexCount; ++i) {
                ClampWeightInPlace(&g_Clip_PolyAttr2[i]);
                if (IsVisibleWeight(g_Clip_PolyAttr2[i])) {
                    attr2Visible = 1;
                }
            }

            if (vertexCount > 1) {
                for (int i = 1; i < vertexCount && *lightingMode == 0; ++i) {
                    if (fabs(g_Clip_PolyAttr2[i] - g_Clip_PolyAttr2[0]) >= kVisibleWeight) {
                        *lightingMode = 2;
                    }
                }
            }

            if (attr2Visible != 0) {
                const int previousFlags = *lightFlags;
                resultFlags |= 8;
                *lightFlags |= 9;
                zColorRgb *color = SelectActiveLightColor(selectedFogLightIndex);
                if ((previousFlags & 1) != 0) {
                    zVideo_SetPendingFogTargetColorFromRgb01((zVideo_ColorRgbFloat *)(color));
                    zVideo::CommitFogTargetColorIfChanged();
                } else {
                    zVideo::SetFogColorFromRgb01((zVideo_ColorRgbFloat *)(color));
                    zVideo::CommitFogColorIfChanged();
                }
            }
        }

        if (selectedFogLightIndex >= 0 && resultFlags != 0) {
            *lightFlags |= 4;
            zVideo_SetPendingFogTargetColorFromRgb01((zVideo_ColorRgbFloat
                    *)(&gModel_ActiveLights[selectedFogLightIndex].light->specularColor));
        }

        return resultFlags | pointAttrsVisible;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-buildlightweights
 * @recoil-artifact defines .text recoil:function:0x488d60: zModel_Light::BuildLightWeights
 * Purpose: build software-path per-vertex light weights, choose and commit fog
 * target state, blend the packed fog color, and report whether lighting applied.
 */
int __fastcall zModel_Light_BuildLightWeights(
    zVec3 *surfaceNormal,
    int vertexCount,
    int *outPackedFogColor,
    float fogBlendScale
) {
    const float kVisibleWeight = 1.0f / 255.0f;
    const float kMinPointNormalWeight = 0.00402156916f;
    const float kMinIntensity = 9.99999975e-6f;

    bool hasAnyCandidate = false;
    bool valid[0x40][0x40] = {0};
    zVec3 lightToVertex[0x40][0x40] = {0};

    {
        for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
            const zVec3 vertex = {g_Clip_PolyVertsScratch[vertexIndex].x,
                g_Clip_PolyVertsScratch[vertexIndex].y,
                g_Clip_PolyVertsScratch[vertexIndex].z};

            {
                for (int lightIndex = 0; lightIndex < gModel_ActiveLightCount; ++lightIndex) {
                    zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[lightIndex];
                    if (entry.contributesToLighting == 0) {
                        continue;
                    }

                    if (entry.useFullWeight != 0) {
                        valid[lightIndex][vertexIndex] = true;
                        hasAnyCandidate = true;
                        continue;
                    }

                    zClass_LightDataPartial *light = entry.light;
                    zVec3 delta = SubtractVec3(
                        light->viewPos,
                        vertex
                    );
                    float distanceSq = DotVec3(
                        delta,
                        delta
                    );
                    gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex] = distanceSq;
                    if (distanceSq >= light->range2Sq) {
                        continue;
                    }

                    if (distanceSq != 0.0f) {
                        gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex] =
                            ApproximateSqrtFromBits(distanceSq);
                        zMath_Vec3_DivScalar(
                            &delta,
                            &delta,
                            gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex]
                        );
                    }

                    lightToVertex[lightIndex][vertexIndex] = delta;
                    valid[lightIndex][vertexIndex] = true;
                    hasAnyCandidate = true;
                }
            }
        }
    }

    if (!hasAnyCandidate && !(g_zModel_FogTargetColorOverride.weight > kVisibleWeight)) {
        return 0;
    }

    zMath::Vec3Normalize(surfaceNormal);

    float vertexWeights[0x40] = {0};
    float maxVertexWeight = 0.0f;
    int nonZeroLightCount = 0;
    int singleLightIndex = -1;

    {
        for (int lightIndex = 0; lightIndex < gModel_ActiveLightCount; ++lightIndex) {
            zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[lightIndex];
            if (entry.contributesToLighting == 0) {
                continue;
            }

            zClass_LightDataPartial *light = entry.light;
            float lightWeightSum = 0.0f;
            {
                for (int vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
                    if (!valid[lightIndex][vertexIndex]) {
                        continue;
                    }

                    float angularWeight = 1.0f;
                    if (light->coneAngle != 0 || light->isPointMode != 0) {
                        const zVec3 &direction = light->isPointMode != 0
                                                     ? lightToVertex[lightIndex][vertexIndex]
                                                     : light->viewDir;
                        angularWeight = DotVec3(
                            *surfaceNormal,
                            direction
                        );
                        if (light->isPointMode != 0 && angularWeight < kMinPointNormalWeight) {
                            angularWeight = kMinPointNormalWeight;
                        }
                        if (light->coneAngle != 0) {
                            const float coneDot = DotVec3(
                                direction,
                                light->viewDir
                            );
                            angularWeight = coneDot < kVisibleWeight ? 0.0f : coneDot;
                        }
                    }

                    if (angularWeight <= kVisibleWeight && light->intensityScale <= kMinIntensity) {
                        continue;
                    }

                    float intensity = light->falloff * angularWeight + light->intensityScale;
                    if (intensity > 1.0f) {
                        intensity = 1.0f;
                    } else if (intensity < light->intensityScale) {
                        intensity = light->intensityScale;
                    }

                    float baseWeight = light->isPointMode != 0 ? 1.0f - intensity : intensity;
                    if (entry.useFullWeight == 0) {
                        const float distanceWeight = zModel_Light::EvalDistanceWeight(
                            light,
                            gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex]
                        );
                        if (light->isPointMode != 0) {
                            const float farWeight = 1.0f - light->intensityScale;
                            baseWeight =
                                (1.0f - distanceWeight) * (farWeight - baseWeight) + baseWeight;
                            if (baseWeight > farWeight) {
                                baseWeight = farWeight;
                            }
                        } else {
                            baseWeight *= distanceWeight;
                        }
                    }

                    vertexWeights[vertexIndex] += baseWeight;
                    lightWeightSum += baseWeight;
                    if (baseWeight > maxVertexWeight) {
                        maxVertexWeight = baseWeight;
                    }
                }
            }

            if (lightWeightSum > kVisibleWeight) {
                singleLightIndex = lightIndex;
                ++nonZeroLightCount;
            }
        }
    }

    if (g_zModel_FogTargetColorOverride.weight > kVisibleWeight) {
        ++nonZeroLightCount;
        for (int i = 0; i < vertexCount; ++i) {
            vertexWeights[i] += g_zModel_FogTargetColorOverride.weight;
        }
    }

    if (nonZeroLightCount == 0) {
        return 0;
    }

    if (fogBlendScale > 0.0f && fogBlendScale >= maxVertexWeight && nonZeroLightCount > 0) {
        maxVertexWeight = fogBlendScale;
        zRndr::CommitDirectFogParamsIfChanged();
    } else if (nonZeroLightCount > 1) {
        zRndr::CommitDirectFogParamsIfChanged();
    } else if (nonZeroLightCount == 1) {
        zColorRgb *color = singleLightIndex >= 0
                               ? &gModel_ActiveLights[singleLightIndex].light->specularColor
                               : &g_zModel_FogTargetColorOverride.colorRgb01;
        zRndr_FogTargetColorStaged_SetRgb01Clamped(color);
        zRndr::CommitStagedFogParamsIfChanged();
    }

    maxVertexWeight = ClampWeight(maxVertexWeight);
    float scale = 0.0f;
    zFloat::Set255f(&scale);
    scale -= 1.0f;
    zRndr::BlendPackedColor565WithFogInPlace(
        outPackedFogColor,
        (int)(maxVertexWeight * scale)
    );
    return 1;
}

namespace zModel_Light {
    float __fastcall
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-evaldistanceweight
     * @recoil-artifact defines .text recoil:function:0x4894f0: zModel_Light::EvalDistanceWeight
     * Purpose: compute a light's range falloff as full, zero, or a linear blend
     * between the inner and outer range.
     */
    EvalDistanceWeight(
        const zClass_LightDataPartial *light,
        float distance
    ) {
        if (distance >= light->range2) {
            return 0.0f;
        }

        if (distance <= light->range1) {
            return 1.0f;
        }

        return (light->range2 - distance) * light->invRangeDelta;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-evalspherefogfade
     * @recoil-artifact defines .text recoil:function:0x489540: zModel_Light::EvalSphereFogFade
     * Purpose: combine distance and height fog coverage for a bounding sphere
     * and clamp the resulting fade.
     */
    float __fastcall EvalSphereFogFade(
        const zVec3 *point,
        float radius
    ) {
        const float distSqXZ = point->x * point->x + point->z * point->z;
        const float distanceXZ = ApproximateSqrtFromBits(distSqXZ);
        const float farEdge = distanceXZ + radius;
        if (farEdge <= gModel_FogDistanceStart) {
            return 0.0f;
        }

        float distanceFade = 1.0f;
        if (distanceXZ - radius < gModel_FogDistanceEnd) {
            const float clampedFarEdge =
                farEdge > gModel_FogDistanceEnd ? gModel_FogDistanceEnd : farEdge;
            distanceFade = (clampedFarEdge - gModel_FogDistanceStart) * gModel_FogDistanceInvRange;
        }

        const float heightFade = EvalHeightFogFade(
            point,
            radius
        );
        const float fade = heightFade * distanceFade;
        if (fade > 1.0f) {
            return 1.0f;
        }

        if (fade < 0.0f) {
            return 0.0f;
        }

        return fade;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-buildattr0depthfade
     * @recoil-artifact defines .text recoil:function:0x4896d0: zModel_Light::BuildAttr0DepthFade
     * Purpose: build per-vertex attr0 depth-fog weights from clip scratch
     * positions, distance fog, projected height fog, and 255-scale output.
     */
    int __fastcall BuildAttr0DepthFade(
        int vertexCount,
        int *outHasVariation
    ) {
        const float kVisibleAttrThreshold = 1.0f / 255.0f;

        float radialDistance[0x40] = {0};
        for (int i = 0; i < vertexCount; ++i) {
            const zClipVert &vert = g_Clip_PolyVertsScratch[i];
            radialDistance[i] = ApproximateSqrtFromBits(vert.x * vert.x + vert.z * vert.z);
        }

        float attrFade[0x40] = {0};
        float attrScale = 0.0f;
        zFloat::Set255f(&attrScale);

        int hasAnyFogCandidate = 0;
        for (int i_250 = 0; i_250 < vertexCount; ++i_250) {
            const float distance = radialDistance[i_250];
            if (distance < gModel_FogDistanceStart) {
                attrFade[i_250] = 0.0f;
                continue;
            }

            float fade = 1.0f;
            if (distance < gModel_FogDistanceEnd) {
                fade = (distance - gModel_FogDistanceStart) * gModel_FogDistanceInvRange;
            }

            float projectedY = 0.0f;
            zMath::Vec3ArrayProjectToCachedY(
                (const zVec3 *)(&g_Clip_PolyVertsScratch[i_250]),
                &projectedY,
                1
            );

            if (projectedY >= gModel_FogHeightHigh) {
                fade = 0.0f;
            } else if (projectedY > gModel_FogHeightLow) {
                fade *= (gModel_FogHeightHigh - projectedY) * gModel_FogHeightInvRange;
            }

            attrFade[i_250] = fade;
            hasAnyFogCandidate = 1;
        }

        if (hasAnyFogCandidate == 0) {
            return 0;
        }

        int result = 0;
        for (int i_281 = 0; i_281 < vertexCount; ++i_281) {
            if (attrFade[i_281] >= 1.0f) {
                attrFade[i_281] = 1.0f;
            } else if (attrFade[i_281] < 0.0f) {
                attrFade[i_281] = 0.0f;
            }

            if (attrFade[i_281] > kVisibleAttrThreshold) {
                result = 1;
            }

            if (fabs(attrFade[i_281] - attrFade[0]) > kVisibleAttrThreshold) {
                *outHasVariation = 1;
            }
        }

        if (result != 0) {
            for (int i = 0; i < vertexCount; ++i) {
                if (attrFade[i] > kVisibleAttrThreshold) {
                    g_Clip_PolyAttr0[i] = attrScale * attrFade[i];
                }
            }
        }

        if (result == 0) {
            *outHasVariation = 0;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-evalbatchspherefade
     * @recoil-artifact defines .text recoil:function:0x489920: zModel_Light::EvalBatchSphereFade
     * Purpose: evaluate depth and height fog for the current scratch vertex,
     * store the clamped fade, and report whether it is visible.
     */
    int __fastcall EvalBatchSphereFade(float *outFade) {
        const zClipVert &vert = g_Clip_PolyVertsScratch[0];
        const float distance = ApproximateSqrtFromBits(vert.x * vert.x + vert.z * vert.z);
        if (distance <= gModel_FogDistanceStart) {
            return 0;
        }

        float fade = 1.0f;
        if (distance < gModel_FogDistanceEnd) {
            fade = (distance - gModel_FogDistanceStart) * gModel_FogDistanceInvRange;
        }

        float projectedY = 0.0f;
        zMath::Vec3ArrayProjectToCachedY(
            (const zVec3 *)(&g_Clip_PolyVertsScratch[0]),
            &projectedY,
            1
        );

        if (projectedY >= gModel_FogHeightHigh) {
            fade = 0.0f;
        } else if (projectedY > gModel_FogHeightLow) {
            fade *= (gModel_FogHeightHigh - projectedY) * gModel_FogHeightInvRange;
        }

        if (fade >= 1.0f) {
            fade = 1.0f;
        } else if (fade < 0.0f) {
            fade = 0.0f;
        }

        *outFade = fade;
        return fade > 0.005f ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-buildattr1falloff
     * @recoil-artifact defines .text recoil:function:0x489a90: zModel_Light::BuildAttr1Falloff
     * Purpose: build per-vertex fog falloff weights in attr2, update lighting
     * variation flags, and commit the current fog color.
     */
    int __fastcall BuildAttr1Falloff(
        int vertexCount,
        int *pLightingFlags
    ) {
        const float kVisibleAttrThreshold = 1.0f / 255.0f;

        float radialDistance[0x40] = {0};
        for (int i = 0; i < vertexCount; ++i) {
            const zClipVert &vert = g_Clip_PolyVertsScratch[i];
            radialDistance[i] = ApproximateSqrtFromBits(vert.x * vert.x + vert.z * vert.z);
        }

        int hasFogContribution = 0;
        for (int fogIndex = 0; fogIndex < vertexCount; ++fogIndex) {
            const float distance = radialDistance[fogIndex];
            if (distance <= gModel_FogDistanceStart) {
                g_Clip_PolyAttr2[fogIndex] = 0.0f;
                continue;
            }

            float fade = 1.0f;
            if (distance < gModel_FogDistanceEnd) {
                fade = (distance - gModel_FogDistanceStart) * gModel_FogDistanceInvRange;
            }

            float projectedHeight = 0.0f;
            zMath::Vec3ArrayProjectToCachedY(
                (const zVec3 *)(&g_Clip_PolyVertsScratch[fogIndex]),
                &projectedHeight,
                1
            );

            if (projectedHeight >= gModel_FogHeightHigh) {
                fade = 0.0f;
            } else if (projectedHeight > gModel_FogHeightLow) {
                fade *= (gModel_FogHeightHigh - projectedHeight) * gModel_FogHeightInvRange;
            }

            g_Clip_PolyAttr2[fogIndex] = fade;
            hasFogContribution = 1;
        }

        if (hasFogContribution == 0) {
            *pLightingFlags &= ~2;
            return 0;
        }

        for (int clampIndex = 0; clampIndex < vertexCount; ++clampIndex) {
            g_Clip_PolyAttr2[clampIndex] = ClampWeight(g_Clip_PolyAttr2[clampIndex]);
        }

        int hasVisibleFog = 0;
        for (int visibleIndex = 0; visibleIndex < vertexCount; ++visibleIndex) {
            if (g_Clip_PolyAttr2[visibleIndex] > kVisibleAttrThreshold) {
                hasVisibleFog = 1;
                break;
            }
        }

        if (hasVisibleFog == 0) {
            *pLightingFlags &= ~2;
            return 0;
        }

        for (int varianceIndex = 1; varianceIndex < vertexCount; ++varianceIndex) {
            if (fabs(g_Clip_PolyAttr2[varianceIndex] - g_Clip_PolyAttr2[0]) >
                kVisibleAttrThreshold) {
                *pLightingFlags |= 2;
                break;
            }
        }

        zVideo::SetFogColorFromRgb01((zVideo_ColorRgbFloat *)(&gModel_FogColorRgb01));
        zVideo::CommitFogColorIfChanged();
        return hasVisibleFog;
    }
}
