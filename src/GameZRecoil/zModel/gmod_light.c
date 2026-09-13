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
zClass_NodePartial **gModel_LightInputNodeStates = 0;
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-pointinpolygoninitxz
 * @recoil-artifact defines .text recoil:function:0x487a30: zModelLightPointInPolygonInitXZ
 *
 *
 * Purpose: select active lights and initialize ambient colour and palette remapping.
 */
void __fastcall zModelLightPointInPolygonInitXZ(
    zClass_NodePartial **lightNodes,
    zClass_LightDataPartial **lightDataList,
    int lightCount
) {
    gModel_LightInputNodeStates = lightNodes;
    gModel_LightInputDataList = lightDataList;
    gModel_LightInputCount = lightCount;
    // Keep the coupled empty-list initialization for the retail VC5 output.
    gModel_ActiveLightSpecialIndex = (gModel_ActiveLightCount = 0) - 1;
    for (int i = 0; i < gModel_LightInputCount; ++i) {
        if ((gModel_LightInputNodeStates[i]->flags & 4) == 0) {
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

        zModel_ActiveLightEntryLive *active = &gModel_ActiveLights[gModel_ActiveLightCount];
        active->light = gModel_LightInputDataList[i];
        active = &gModel_ActiveLights[gModel_ActiveLightCount];
        active->lightNode = gModel_LightInputNodeStates[i];
        if (gModel_LightInputDataList[i]->isDirectedSource != 0) {
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
        zModel_ActiveLightEntryLive *active = &gModel_ActiveLights[gModel_ActiveLightSpecialIndex];
        gModel_AmbientColorRgb01 = active->light->specularColor;
        active = &gModel_ActiveLights[gModel_ActiveLightSpecialIndex];
        gModel_AmbientIntensityFactor = 1.0f - active->light->intensityScale;
    } else {
        gModel_AmbientIntensityFactor = 0.0f;
        gModel_AmbientColorRgb01 = gModel_FogColorRgb01;
    }

    gModel_SpecialLightPaletteRemapRecipe.color1Strength = 1.0f;
    if (gModel_ActiveLightSpecialIndex >= 0) {
        zModel_ActiveLightEntryLive *active = &gModel_ActiveLights[gModel_ActiveLightSpecialIndex];
        gModel_SpecialLightPaletteRemapRecipe.color1 = active->light->specularColor;
        active = &gModel_ActiveLights[gModel_ActiveLightSpecialIndex];
        gModel_SpecialLightPaletteRemapRecipe.color0 = active->light->specularColor;
        gModel_SpecialLightPaletteRemapRecipe.color0Strength = 0.0f;
    } else {
        gModel_SpecialLightPaletteRemapRecipe.color0.red =
            gModel_SpecialLightPaletteRemapRecipe.color0.green =
            gModel_SpecialLightPaletteRemapRecipe.color0.blue = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color1.red =
            gModel_SpecialLightPaletteRemapRecipe.color1.green =
            gModel_SpecialLightPaletteRemapRecipe.color1.blue = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color0Strength = 0.0f;
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

            if ((entry.lightNode->flags & 4) == 0) {
                zError::ReportOld(
                    0x200,
                    g_zModel_SourceFile_GmodLightC,
                    0xfa,
                    g_zModel_NeverGetHereMsg
                );
                continue;
            }

            if ((g_zVideo_ActiveRendererPath != 0 && light->isDirectedSource != 0) ||
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
            if (light->isDirectedSource != 0) {
                gModel_ActiveLightSpecialIndex = i;
                distance = sphereCenter->z;
            } else {
                const zVec3 delta = {light->viewPos.x - sphereCenter->x,
                    light->viewPos.y - sphereCenter->y,
                    light->viewPos.z - sphereCenter->z};
                const float distSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
                if (distSq == 0.0f) {
                    distance = 0.0f;
                } else {
                    int distanceBits = *(const int *)&distSq;
                    distanceBits = (distanceBits >> 1) + 0x1fc00000;
                    distance = *(float *)&distanceBits;
                }
            }

            lightDistances[i] = distance - radius;
            const float farEdge = distance + radius;
            if (lightDistances[i] >= light->range2 && light->isDirectedSource == 0) {
                continue;
            }

            entry.contributesToLighting = 1;
            if (farEdge < light->range1) {
                entry.useFullWeight = 1;
                ++result;
                continue;
            }

            if (light->isDirectedSource != 0) {
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
                light->isDirectedSource == 0) {
                entry.contributesToLighting = 0;
                --result;
                continue;
            }

            float weight =
                entry.useFullWeight != 0 ? 1.0f : EvalDistanceWeight(light, lightDistances[i_647]);
            const float cap = light->falloff + light->intensityScale;
            if (cap < weight) {
                weight = cap;
            }
            if (weight > 1.0f) {
                weight = 1.0f;
            } else if (weight < 0.0f) {
                weight = 0.0f;
            }

            if (light->isDirectedSource != 0) {
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
        const double kVisibleWeight = 0.003921569;
        const double kMinPointNormalWeight = kVisibleWeight + 0.0001f;
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

                if (*lightFlags == 1 && light->lightParam != 0 && light->isDirectedSource == 0) {
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

                if (light->isDirectedSource != 0) {
                    distances[lightIndex][vertexIndex] = vertex.z;
                    if (vertex.z < light->range2) {
                        lightToVertex[lightIndex][vertexIndex] = vertex;
                        if (vertex.z != 0.0f) {
                            zMathVec3DivScalar(
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

                zVec3 difference;
                difference.x = light->viewPos.x - vertex.x;
                difference.y = light->viewPos.y - vertex.y;
                difference.z = light->viewPos.z - vertex.z;
                zVec3 delta = difference;
                float distanceSq =
                    delta.x * difference.x +
                    delta.y * difference.y +
                    delta.z * difference.z;
                distances[lightIndex][vertexIndex] = distanceSq;
                if (distanceSq >= light->range2Sq) {
                    continue;
                }

                if (distanceSq != 0.0f) {
                    int distanceBits = 0;
                    memcpy(&distanceBits, &distanceSq, sizeof(distanceBits));
                    distanceBits = (distanceBits >> 1) + 0x1fc00000;
                    float distance = 0.0f;
                    memcpy(&distance, &distanceBits, sizeof(distance));
                    distances[lightIndex][vertexIndex] = distance;
                    zMathVec3DivScalar(&delta, &delta, distance);
                }

                lightToVertex[lightIndex][vertexIndex] = delta;
                valid[lightIndex][vertexIndex] = true;
                hasAnyCandidate = true;
            }
        }

        if (!hasAnyCandidate &&
            !(g_zModel_FogTargetColorOverride.weight > kVisibleWeight)) {
            return 0;
        }

        zMath::Vec3Normalize(surfaceNormal);

        float fogWeights[0x40] = {0};
        memset(g_Clip_PolyAttr1, 0, (size_t)(vertexCount) * sizeof(float));
        if (g_zVideo_ActiveRendererPath != 0 && (*lightFlags & 1) == 0) {
            memset(g_Clip_PolyAttr2, 0, (size_t)(vertexCount) * sizeof(float));
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
                if (light->isDirectional != 0 || light->isDirectedSource != 0) {
                    float dotProduct = 0.0f;
                    if (light->isDirectedSource != 0 && g_zModel_CurrentPolyNormals != 0) {
                        const zVec3 &polyNormal =
                            g_zModel_CurrentPolyNormals[vertexIndex];
                        dotProduct =
                            polyNormal.x * light->viewDir.x +
                            polyNormal.y * light->viewDir.y +
                            polyNormal.z * light->viewDir.z;
                    } else if (light->isDirectedSource != 0) {
                        const zVec3 &direction =
                            lightToVertex[lightIndex][vertexIndex];
                        dotProduct =
                            surfaceNormal->x * direction.x +
                            surfaceNormal->y * direction.y +
                            surfaceNormal->z * direction.z;
                    } else {
                        dotProduct =
                            surfaceNormal->x * light->viewDir.x +
                            surfaceNormal->y * light->viewDir.y +
                            surfaceNormal->z * light->viewDir.z;
                    }
                    angularWeight = dotProduct;

                    if (light->isDirectedSource != 0 && angularWeight < kMinPointNormalWeight) {
                        angularWeight = kMinPointNormalWeight;
                    }

                    if (light->isDirectional != 0) {
                        const zVec3 &direction =
                            lightToVertex[lightIndex][vertexIndex];
                        float coneWeight =
                            direction.x * light->viewDir.x +
                            direction.y * light->viewDir.y +
                            direction.z * light->viewDir.z;
                        if (light->isDirectedSource != 0 && g_zModel_CurrentPolyNormals != 0) {
                            const zVec3 &polyNormal =
                                g_zModel_CurrentPolyNormals[vertexIndex];
                            coneWeight =
                                polyNormal.x * light->viewDir.x +
                                polyNormal.y * light->viewDir.y +
                                polyNormal.z * light->viewDir.z;
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

                float weight = light->isDirectedSource != 0 ? 1.0f - intensity : intensity;
                if (g_zVideo_ActiveRendererPath == 0) {
                    if (light->isDirectedSource != 0) {
                        if (entry.useFullWeight == 0) {
                            const float distanceWeight = EvalDistanceWeight(
                                light,
                                distances[lightIndex][vertexIndex]
                            );
                            const float farWeight = 1.0f - light->intensityScale;
                            weight =
                                (1.0f - distanceWeight) * (farWeight - weight) + weight;
                            if (weight > farWeight) {
                                weight = farWeight;
                            }
                        }
                        g_Clip_PolyAttr1[vertexIndex] += weight;
                        pointSum += weight;
                    } else {
                        if (entry.useFullWeight == 0) {
                            weight *= EvalDistanceWeight(light, distances[lightIndex][vertexIndex]);
                        }
                        fogWeights[vertexIndex] += weight;
                        fogSum += weight;
                    }
                } else if (light->isDirectedSource != 0) {
                    if (entry.useFullWeight == 0) {
                        const float distanceWeight = EvalDistanceWeight(
                            light,
                            distances[lightIndex][vertexIndex]
                        );
                        const float farWeight = 1.0f - light->intensityScale;
                        weight = (1.0f - distanceWeight) * (farWeight - weight) + weight;
                        if (weight > farWeight) {
                            weight = farWeight;
                        }
                    }
                    g_Clip_PolyAttr1[vertexIndex] += weight;
                    pointSum += weight;
                } else {
                    if (entry.useFullWeight == 0) {
                        weight *= EvalDistanceWeight(light, distances[lightIndex][vertexIndex]);
                    }
                    if (light->lightParam != 0) {
                        g_Clip_PolyAttr2[vertexIndex] += weight;
                        attr2Sum += weight;
                        hasAttr2Contribution = 1;
                    } else {
                        fogWeights[vertexIndex] += weight;
                        fogSum += weight;
                    }
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

        if (g_zModel_FogTargetColorOverride.weight > kVisibleWeight) {
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
            if (g_Clip_PolyAttr1[0] > 1.0f) {
                g_Clip_PolyAttr1[0] = 1.0f;
            } else if (g_Clip_PolyAttr1[0] < 0.0f) {
                g_Clip_PolyAttr1[0] = 0.0f;
            }
            pointAttrsVisible = g_Clip_PolyAttr1[0] > kVisibleWeight ? 1 : 0;
            int attr1Varies = 0;
            for (int i = 1; i < vertexCount; ++i) {
                if (g_Clip_PolyAttr1[i] > 1.0f) {
                    g_Clip_PolyAttr1[i] = 1.0f;
                } else if (g_Clip_PolyAttr1[i] < 0.0f) {
                    g_Clip_PolyAttr1[i] = 0.0f;
                }
                if (fabs(g_Clip_PolyAttr1[i] - g_Clip_PolyAttr1[0]) >= kVisibleWeight) {
                    attr1Varies = 1;
                }
                if (g_Clip_PolyAttr1[i] > kVisibleWeight) {
                    pointAttrsVisible = 1;
                }
            }

            if (g_zVideo_ActiveRendererPath == 0 && g_zModel_SoftwarePathActive != 0 &&
                usePaletteRemap != 0) {
                if (attr1Varies != 0 && initialLightingMode != 0) {
                    for (int i = 0; i < vertexCount; ++i) {
                        g_Clip_PolyAttr0[i] = g_Clip_PolyAttr1[i] * scale255;
                    }
                    zRndrSetPaletteShadeRecipeIndex(&gModel_SpecialLightPaletteRemapRecipe);
                    *lightingMode |= 1;
                    return 1;
                }

                if (pointAttrsVisible != 0) {
                    g_Clip_PolyAttr1[0] *= scale255;
                    zRndrSetPaletteRemapKey(
                        &gModel_SpecialLightPaletteRemapRecipe,
                        g_Clip_PolyAttr1[0]
                    );
                }
                if (pointAttrsVisible == 0 &&
                    g_zModel_FogTargetColorOverride.weight > kVisibleWeight) {
                    zRndrSetPaletteRemapKeyFromRgb01(0, 0.0f);
                }
            } else if (pointAttrsVisible != 0) {
                *lightingMode |= 1;
            }
        }

        int resultFlags = 0;
        for (int i = 0; i < vertexCount; ++i) {
            if (fogWeights[i] > 1.0f) {
                fogWeights[i] = 1.0f;
            } else if (fogWeights[i] < 0.0f) {
                fogWeights[i] = 0.0f;
            }
            if (fogWeights[i] > kVisibleWeight) {
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
                    zColorRgb *color = selectedFogLightIndex < 0
                        ? &g_zModel_FogTargetColorOverride.colorRgb01
                        : &gModel_ActiveLights[selectedFogLightIndex].light->specularColor;
                    zRndrFogTargetColorStagedSetRgb01Clamped(color);
                    zRndr::CommitStagedFogParamsIfChanged();
                }
            }
            return resultFlags;
        }

        if (hasAttr2Contribution != 0) {
            int attr2Visible = 0;
            for (int i = 0; i < vertexCount; ++i) {
                if (g_Clip_PolyAttr2[i] > 1.0f) {
                    g_Clip_PolyAttr2[i] = 1.0f;
                } else if (g_Clip_PolyAttr2[i] < 0.0f) {
                    g_Clip_PolyAttr2[i] = 0.0f;
                }
                if (g_Clip_PolyAttr2[i] > kVisibleWeight) {
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
                zColorRgb *color = selectedFogLightIndex < 0
                    ? &g_zModel_FogTargetColorOverride.colorRgb01
                    : &gModel_ActiveLights[selectedFogLightIndex].light->specularColor;
                if ((previousFlags & 1) != 0) {
                    zVideoSetPendingFogTargetColorFromRgb01((zVideo_ColorRgbFloat *)(color));
                    zVideo::CommitFogTargetColorIfChanged();
                } else {
                    zVideo::SetFogColorFromRgb01((zVideo_ColorRgbFloat *)(color));
                    zVideo::CommitFogColorIfChanged();
                }
            }
        }

        if (selectedFogLightIndex >= 0 && resultFlags != 0) {
            *lightFlags |= 4;
            zVideoSetPendingFogTargetColorFromRgb01((zVideo_ColorRgbFloat
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
int __fastcall zModelLightBuildLightWeights(
    zVec3 *surfaceNormal,
    int vertexCount,
    int *outPackedFogColor,
    float fogBlendScale
) {
    const double kVisibleWeight = 0.003921569;
    const double kMinPointNormalWeight = kVisibleWeight + 0.0001f;
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
                    zVec3 difference;
                    difference.x = light->viewPos.x - vertex.x;
                    difference.y = light->viewPos.y - vertex.y;
                    difference.z = light->viewPos.z - vertex.z;
                    zVec3 delta = difference;
                    float distanceSq =
                        delta.x * difference.x +
                        delta.y * difference.y +
                        delta.z * difference.z;
                    gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex] = distanceSq;
                    if (distanceSq >= light->range2Sq) {
                        continue;
                    }

                    if (distanceSq != 0.0f) {
                        int distanceBits = 0;
                        memcpy(&distanceBits, &distanceSq, sizeof(distanceBits));
                        distanceBits = (distanceBits >> 1) + 0x1fc00000;
                        float distance = 0.0f;
                        memcpy(&distance, &distanceBits, sizeof(distance));
                        zMathVec3DivScalar(&delta, &delta, distance);
                        gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex] = distance;
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
                    if (light->isDirectional != 0 || light->isDirectedSource != 0) {
                        const zVec3 &direction = light->isDirectedSource != 0
                                                     ? lightToVertex[lightIndex][vertexIndex]
                                                     : light->viewDir;
                        angularWeight =
                            surfaceNormal->x * direction.x +
                            surfaceNormal->y * direction.y +
                            surfaceNormal->z * direction.z;
                        if (light->isDirectedSource != 0 && angularWeight < kMinPointNormalWeight) {
                            angularWeight = kMinPointNormalWeight;
                        }
                        if (light->isDirectional != 0) {
                            const float coneDot =
                                direction.x * light->viewDir.x +
                                direction.y * light->viewDir.y +
                                direction.z * light->viewDir.z;
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

                    float baseWeight = light->isDirectedSource != 0 ? 1.0f - intensity : intensity;
                    if (entry.useFullWeight == 0) {
                        if (g_zVideo_ActiveRendererPath == 0) {
                            if (light->isDirectedSource != 0) {
                                const float distanceWeight =
                                    zModel_Light::EvalDistanceWeight(
                                        light,
                                        gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex]
                                    );
                                const float farWeight = 1.0f - light->intensityScale;
                                baseWeight = (1.0f - distanceWeight) *
                                                 (farWeight - baseWeight) +
                                             baseWeight;
                                if (baseWeight > farWeight) {
                                    baseWeight = farWeight;
                                }
                                vertexWeights[vertexIndex] += baseWeight;
                                lightWeightSum += baseWeight;
                                if (baseWeight > maxVertexWeight) {
                                    maxVertexWeight = baseWeight;
                                }
                                continue;
                            } else {
                                baseWeight *= zModel_Light::EvalDistanceWeight(
                                    light,
                                    gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex]
                                );
                                lightWeightSum += baseWeight;
                                vertexWeights[vertexIndex] += baseWeight;
                                if (baseWeight > maxVertexWeight) {
                                    maxVertexWeight = baseWeight;
                                }
                                continue;
                            }
                        } else if (light->isDirectedSource != 0) {
                            const float distanceWeight = zModel_Light::EvalDistanceWeight(
                                light,
                                gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex]
                            );
                            const float farWeight = 1.0f - light->intensityScale;
                            baseWeight =
                                (1.0f - distanceWeight) * (farWeight - baseWeight) + baseWeight;
                            if (baseWeight > farWeight) {
                                baseWeight = farWeight;
                            }
                            vertexWeights[vertexIndex] += baseWeight;
                            lightWeightSum += baseWeight;
                            if (baseWeight > maxVertexWeight) {
                                maxVertexWeight = baseWeight;
                            }
                            continue;
                        } else {
                            baseWeight *= zModel_Light::EvalDistanceWeight(
                                light,
                                gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex]
                            );
                            vertexWeights[vertexIndex] += baseWeight;
                            lightWeightSum += baseWeight;
                            if (baseWeight > maxVertexWeight) {
                                maxVertexWeight = baseWeight;
                            }
                            continue;
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
        zRndrFogTargetColorStagedSetRgb01Clamped(color);
        zRndr::CommitStagedFogParamsIfChanged();
    }

    if (maxVertexWeight > 1.0f) {
        maxVertexWeight = 1.0f;
    } else if (maxVertexWeight < 0.0f) {
        maxVertexWeight = 0.0f;
    }
    float scale = 0.0f;
    zFloat::Set255f(&scale);
    scale -= 1.0f;
    zRndr::BlendPackedColor565WithFogInPlace(outPackedFogColor, (int)(maxVertexWeight * scale));
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

}
