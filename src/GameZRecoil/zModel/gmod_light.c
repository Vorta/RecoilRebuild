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
char g_zModel_SourceFile_GmodLightC[0x28] = "D:\\Proj\\GameZRecoil\\zModel\\gmod_light.c";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-g-zmodel-maxlightsrequestfmt
 * @recoil-artifact defines .data recoil:data:0x4e1820: g_zModel_MaxLightsRequestFmt.
 * Data owner: geometry_model_assets.zmodel_gmod_light_diagnostics_data.
 * Purpose: store the writable active-light overflow diagnostic format.
 */
char g_zModel_MaxLightsRequestFmt[0x2c] = "Not enough MAX_LIGHTS: %d; requesting more.";
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-gmodel-lightvertexdistancesqscratch
 * @recoil-artifact defines .data recoil:data:0x566a28: gModel_LightVertexDistanceSqScratch.
 * Data owner: engine.zmodel.light_vertex_distance_scratch.
 * Purpose: store per-light/per-vertex distance scratch values while building
 * model light weights.
 */
float gModel_LightVertexDistanceSqScratch[0x40][0x40] = { 0 };
RECOIL_STATIC_ASSERT(sizeof(gModel_LightVertexDistanceSqScratch) == 0x4000);

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-pointinpolygoninitxz
 * @recoil-artifact defines .text recoil:function:0x487a30: zModelLightPointInPolygonInitXZ
 * @recoil-match byte
 *
 * Purpose: select active lights and initialize ambient colour and palette remapping.
 */
void __fastcall
zModelLightPointInPolygonInitXZ(CZNodePartial** lightNodes, CZLightDataPartial** lightDataList, int lightCount)
{
    int i;
    gModel_LightInputNodeStates = lightNodes;
    gModel_LightInputDataList = lightDataList;
    gModel_LightInputCount = lightCount;
    // Keep the coupled empty-list initialization for the retail VC5 output.
    gModel_ActiveLightSpecialIndex = (gModel_ActiveLightCount = 0) - 1;
    for (i = 0; i < gModel_LightInputCount; ++i) {
        zModel_ActiveLightEntryLive* active;
        if ((gModel_LightInputNodeStates[i]->flags & 4) == 0) {
            continue;
        }

        if (gModel_ActiveLightCount == 0x40) {
            ReportOld(0x200, g_zModel_SourceFile_GmodLightC, 0x46, g_zModel_MaxLightsRequestFmt, 0x40);
            break;
        }

        active = &gModel_ActiveLights[gModel_ActiveLightCount];
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
        zModel_ActiveLightEntryLive* active = &gModel_ActiveLights[gModel_ActiveLightSpecialIndex];
        gModel_AmbientColorRgb01 = active->light->specularColor;
        active = &gModel_ActiveLights[gModel_ActiveLightSpecialIndex];
        gModel_AmbientIntensityFactor = 1.0f - active->light->intensityScale;
    } else {
        gModel_AmbientIntensityFactor = 0.0f;
        gModel_AmbientColorRgb01 = gModel_FogColorRgb01;
    }

    gModel_SpecialLightPaletteRemapRecipe.color1Strength = 1.0f;
    if (gModel_ActiveLightSpecialIndex >= 0) {
        zModel_ActiveLightEntryLive* active = &gModel_ActiveLights[gModel_ActiveLightSpecialIndex];
        gModel_SpecialLightPaletteRemapRecipe.color1 = active->light->specularColor;
        active = &gModel_ActiveLights[gModel_ActiveLightSpecialIndex];
        gModel_SpecialLightPaletteRemapRecipe.color0 = active->light->specularColor;
        gModel_SpecialLightPaletteRemapRecipe.color0Strength = 0.0f;
    } else {
        gModel_SpecialLightPaletteRemapRecipe.color0.red = gModel_SpecialLightPaletteRemapRecipe.color0.green
            = gModel_SpecialLightPaletteRemapRecipe.color0.blue = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color1.red = gModel_SpecialLightPaletteRemapRecipe.color1.green
            = gModel_SpecialLightPaletteRemapRecipe.color1.blue = 0.0f;
        gModel_SpecialLightPaletteRemapRecipe.color0Strength = 0.0f;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-pointinpolygontestradiusxz
 * @recoil-artifact defines .text recoil:function:0x487c50: zModel_Light::PointInPolygonTestRadiusXZ
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmodel.point-in-polygon-test-radius-xz.fast-sqrt-estimate recoil:function:0x487c50
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmodel.point-in-polygon-test-radius-xz.fast-sqrt-estimate
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-match byte
 *
 * Raw assembly: one in-body 13-byte fast-sqrt estimate island at retail
 * [0x487d4b,0x487d58).
 *
 *
 * Purpose: evaluate active light contribution flags and per-light weights
 * for a bounding sphere in view-space XZ/radius terms.
 */
int __fastcall PointInPolygonTestRadiusXZ(const zVec3* sphereCenter, float radius)
{
    zVec3 lightDeltas[0x40];
    float lightDistances[0x40];
    int hasSoftwarePointLight = 0;
    int result = 0;

    zModel_ActiveLightEntryLive* entry;
    int i;
    for (i = 0, entry = &gModel_ActiveLights[0]; i < gModel_ActiveLightCount; ++i, ++entry) {
        entry->useFullWeight = 0;
        entry->contributesToLighting = 0;

        if ((entry->lightNode->flags & 4) != 0) {
            if ((g_zVideo_ActiveRendererPath == 0 || entry->light->isDirectedSource == 0)
                && entry->light->enabled != 0) {
                float farEdge;
                if (entry->light->lightSubMode == 0) {
                    continue;
                }

                if (entry->light->isDirectedSource != 0) {
                    gModel_ActiveLightSpecialIndex = i;
                    lightDistances[i] = sphereCenter->z;
                } else {
                    Vec3Subtract(&entry->light->viewPos, sphereCenter, &lightDeltas[i]);
                    lightDistances[i] = lightDeltas[i].y * lightDeltas[i].y + lightDeltas[i].z * lightDeltas[i].z
                        + lightDeltas[i].x * lightDeltas[i].x;
                    if (lightDistances[i] != 0.0f) {
                        float distanceSq = lightDistances[i];
                        float distance;
                        // Raw-assembly fast square-root estimate: retail transforms the named distanceSq
                        // bits through EAX ((bits >> 1) + 0x1fc00000) into the named result local.
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
                        __asm {
                            mov eax, distanceSq
                            sar eax, 1
                            add eax, 01fc00000h
                            mov distance, eax
                        }
#else
                        {
                            int estimateBits;
                            memcpy(&estimateBits, &distanceSq, sizeof estimateBits);
                            estimateBits = (estimateBits >> 1) + 0x1fc00000;
                            memcpy(&distance, &estimateBits, sizeof distance);
                        }
#endif
                        lightDistances[i] = distance;
                    }
                }

                farEdge = radius + lightDistances[i];
                lightDistances[i] -= radius;
                if (lightDistances[i] >= entry->light->range2 && entry->light->isDirectedSource == 0) {
                    continue;
                }

                entry->contributesToLighting = 1;
                if (farEdge < entry->light->range1) {
                    entry->useFullWeight = 1;
                    ++result;
                    continue;
                }

                if (entry->light->isDirectedSource != 0) {
                    hasSoftwarePointLight = 1;
                }
                ++result;
            } else {
                entry->useFullWeight = 1;
                entry->contributesToLighting = 1;
                ++result;
            }
        } else {
            ReportOld(0x200, g_zModel_SourceFile_GmodLightC, 0xfa, g_zModel_NeverGetHereMsg);
        }
    }

    if (result == 0) {
        return 0;
    }

    for (i = 0, entry = &gModel_ActiveLights[0]; i < gModel_ActiveLightCount; ++i, ++entry) {
        float weight;
        float cap;
        if (entry->contributesToLighting == 0) {
            continue;
        }

        if (hasSoftwarePointLight != 0 && g_zModel_SoftwarePathActive != 0 && entry->light->isDirectedSource == 0) {
            entry->contributesToLighting = 0;
            --result;
            continue;
        }

        if (entry->useFullWeight != 0) {
            weight = 1.0f;
        } else {
            weight = EvalDistanceWeight(lightDistances[i], entry->light);
        }
        cap = entry->light->falloff + entry->light->intensityScale;
        if (cap < weight) {
            weight = cap;
        }
        if (weight > 1.0f) {
            weight = 1.0f;
        } else if (weight < 0.0f) {
            weight = 0.0f;
        }

        if (entry->light->isDirectedSource != 0) {
            g_Clip_PolyAttr1[i] = weight;
        } else {
            g_Clip_PolyAttr0[i] = weight;
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-setactivelights
 * @recoil-artifact defines .text recoil:function:0x487f10: zModel_Light::SetActiveLights
 *
 *
 * Purpose: build active-light vertex attributes for software and hardware
 * render paths, including fog target, point-light, attr1, and attr2 state.
 */
int __fastcall
SetActiveLights(zVec3* surfaceNormal, int vertexCount, int* lightFlags, int* lightingMode, int usePaletteRemap)
{
    const double kVisibleWeight = 0.003921569;
    const double kMinPointNormalWeight = kVisibleWeight + 0.0001f;
    const float kMinIntensity = 9.99999975e-6f;

    const int hardwarePath = g_zVideo_ActiveRendererPath;
    const int initialLightingMode = *lightingMode;
    int pointAttrsVarying = 0;
    int hasAnyCandidate = 0;
    int resultFlags = 0;
    int hasAttr2Contribution = 0;
    float scale255;
    int valid[0x40][0x40];
    float distances[0x40][0x40];
    zVec3 lightToVertex[0x40][0x40];
    int vertexIndex;
    float fogWeights[0x40];
    int i;
    int fogContributorCount;
    int pointContributorCount;
    int selectedFogLightIndex;
    int selectedPointLightIndex;
    int lightIndex;
    int pointAttrsVisible;
    *lightingMode = 0;

    Set255f(&scale255);

    // Retail leaves these uninitialized: pass 2 reads stale entries for lights skipped above
    // and for lightToVertex[l][0] of full-weight directional lights.

    for (vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
        const zVec3* const vertex = &g_Clip_PolyVertsScratch[vertexIndex];
        for (lightIndex = 0; lightIndex < gModel_ActiveLightCount; ++lightIndex) {
            zModel_ActiveLightEntryLive* entry = &gModel_ActiveLights[lightIndex];
            CZLightDataPartial* const light = entry->light;
            zVec3* delta;
            float distanceSq;
            if (*lightFlags == 1 && light->lightParam != 0 && light->isDirectedSource == 0) {
                continue;
            }

            valid[lightIndex][vertexIndex] = 0;
            if (entry->contributesToLighting == 0) {
                continue;
            }

            if (entry->useFullWeight != 0) {
                valid[lightIndex][vertexIndex] = 1;
                hasAnyCandidate = 1;
                continue;
            }

            if (light->isDirectedSource != 0) {
                const float depth = vertex->z;
                distances[lightIndex][vertexIndex] = depth;
                if (depth < light->range2) {
                    zMathVec3DivScalar(vertex, &lightToVertex[lightIndex][vertexIndex], depth);
                }
                valid[lightIndex][vertexIndex] = 1;
                hasAnyCandidate = 1;
                continue;
            }

            delta = &lightToVertex[lightIndex][vertexIndex];
            delta->x = light->viewPos.x - vertex->x;
            delta->y = light->viewPos.y - vertex->y;
            delta->z = light->viewPos.z - vertex->z;
            distanceSq = delta->x * delta->x + delta->y * delta->y + delta->z * delta->z;
            distances[lightIndex][vertexIndex] = distanceSq;
            if (distanceSq >= light->range2Sq) {
                continue;
            }

            if (distanceSq != 0.0f) {
                int distanceBits = *(const int*)&distanceSq;
                distanceBits = (distanceBits >> 1) + 0x1fc00000;
                distances[lightIndex][vertexIndex] = *(float*)&distanceBits;
            }
            zMathVec3DivScalar(delta, delta, distances[lightIndex][vertexIndex]);
            valid[lightIndex][vertexIndex] = 1;
            hasAnyCandidate = 1;
        }
    }

    if (g_zModel_FogTargetColorOverride.weight > kVisibleWeight) {
        hasAnyCandidate = 1;
    }
    if (hasAnyCandidate == 0) {
        return 0;
    }

    Vec3Normalize(surfaceNormal);

    for (i = 0; i < vertexCount; ++i) {
        fogWeights[i] = 0.0f;
        g_Clip_PolyAttr1[i] = 0.0f;
    }
    if (hardwarePath != 0 && (*lightFlags & 1) == 0) {
        for (i = 0; i < vertexCount; ++i) {
            g_Clip_PolyAttr2[i] = 0.0f;
        }
    }

    fogContributorCount = 0;
    pointContributorCount = 0;
    selectedFogLightIndex = -1;

    for (lightIndex = 0; lightIndex < gModel_ActiveLightCount; ++lightIndex) {
        zModel_ActiveLightEntryLive* entry = &gModel_ActiveLights[lightIndex];
        CZLightDataPartial* light;
        float attr2Sum;
        float fogSum;
        float pointSum;
        float surfaceWeight;
        float angularWeight;
        float intensity;
        if (entry->contributesToLighting == 0) {
            continue;
        }

        light = entry->light;
        attr2Sum = 0.0f;
        fogSum = 0.0f;
        pointSum = 0.0f;
        if (light->isDirectional == 0 && light->isDirectedSource == 0) {
            surfaceWeight = 1.0f;
        } else {
            const float surfaceDot = surfaceNormal->x * light->viewDir.x + surfaceNormal->y * light->viewDir.y
                + surfaceNormal->z * light->viewDir.z;
            surfaceWeight = surfaceDot;
            if (light->isDirectedSource != 0 && surfaceDot < kMinPointNormalWeight) {
                surfaceWeight = (float)kMinPointNormalWeight;
            }
        }

        // The first vertex also establishes the light base intensity.
        angularWeight = surfaceWeight;
        if (surfaceWeight > kVisibleWeight) {
            if (light->isDirectional != 0) {
                const zVec3* direction = &lightToVertex[lightIndex][0];
                const float coneDot = direction->x * light->viewDir.x + direction->y * light->viewDir.y
                    + direction->z * light->viewDir.z;
                angularWeight = coneDot < kVisibleWeight ? 0.0f : coneDot;
            } else if (light->isDirectedSource != 0 && g_zModel_CurrentPolyNormals != 0) {
                const zVec3* polyNormal = &g_zModel_CurrentPolyNormals[0];
                const float normalDot = polyNormal->x * light->viewDir.x + polyNormal->y * light->viewDir.y
                    + polyNormal->z * light->viewDir.z;
                angularWeight = normalDot < kMinPointNormalWeight ? (float)kMinPointNormalWeight : normalDot;
            }
        } else {
            surfaceWeight = 0.0f;
            angularWeight = 0.0f;
        }

        if (surfaceWeight <= kVisibleWeight && light->intensityScale <= kMinIntensity) {
            continue;
        }

        intensity = light->falloff * angularWeight + light->intensityScale;
        if (intensity > 1.0f) {
            intensity = 1.0f;
        } else if (light->intensityScale > intensity) {
            intensity = light->intensityScale;
        }

        if (valid[lightIndex][0]) {
            float weight = light->isDirectedSource != 0 ? 1.0f - intensity : intensity;
            if (entry->useFullWeight == 0) {
                if (light->isDirectedSource != 0) {
                    const float farWeight = 1.0f - light->intensityScale;
                    const float distanceWeight = EvalDistanceWeight(distances[lightIndex][0], light);
                    weight = (1.0f - distanceWeight) * (farWeight - weight) + weight;
                    if (weight > farWeight) {
                        weight = farWeight;
                    }
                } else {
                    weight *= EvalDistanceWeight(distances[lightIndex][0], light);
                }
            }

            if (light->isDirectedSource != 0) {
                g_Clip_PolyAttr1[0] += weight;
                pointSum += weight;
            } else if (hardwarePath != 0 && light->lightParam != 0) {
                g_Clip_PolyAttr2[0] += weight;
                hasAttr2Contribution = 1;
                attr2Sum += weight;
            } else {
                fogWeights[0] += weight;
                fogSum += weight;
            }
        }

        for (vertexIndex = 1; vertexIndex < vertexCount; ++vertexIndex) {
            float weight;
            if (!valid[lightIndex][vertexIndex]) {
                continue;
            }

            angularWeight = surfaceWeight;
            if (surfaceWeight > kVisibleWeight) {
                if (light->isDirectional != 0) {
                    zVec3* const direction = &lightToVertex[lightIndex][vertexIndex];
                    float coneDot;
                    if (entry->useFullWeight != 0) {
                        const zVec3* const vertex = &g_Clip_PolyVertsScratch[vertexIndex];
                        direction->x = light->viewPos.x - vertex->x;
                        direction->y = light->viewPos.y - vertex->y;
                        direction->z = light->viewPos.z - vertex->z;
                    }
                    coneDot = direction->x * light->viewDir.x + direction->y * light->viewDir.y
                        + direction->z * light->viewDir.z;
                    angularWeight = coneDot < kVisibleWeight ? 0.0f : coneDot;
                } else if (light->isDirectedSource != 0 && g_zModel_CurrentPolyNormals != 0) {
                    const zVec3* polyNormal = &g_zModel_CurrentPolyNormals[vertexIndex];
                    const float normalDot = polyNormal->x * light->viewDir.x + polyNormal->y * light->viewDir.y
                        + polyNormal->z * light->viewDir.z;
                    angularWeight = normalDot < kMinPointNormalWeight ? (float)kMinPointNormalWeight : normalDot;
                }
            } else {
                angularWeight = 0.0f;
            }

            if (angularWeight <= kVisibleWeight && light->intensityScale <= kMinIntensity) {
                continue;
            }

            if (light->isDirectional != 0 || (light->isDirectedSource != 0 && g_zModel_CurrentPolyNormals != 0)) {
                intensity = light->falloff * angularWeight + light->intensityScale;
                if (intensity > 1.0f) {
                    intensity = 1.0f;
                } else if (light->intensityScale > intensity) {
                    intensity = light->intensityScale;
                }
            }

            weight = light->isDirectedSource != 0 ? 1.0f - intensity : intensity;
            if (entry->useFullWeight == 0) {
                if (light->isDirectedSource != 0) {
                    const float farWeight = 1.0f - light->intensityScale;
                    const float distanceWeight = EvalDistanceWeight(distances[lightIndex][vertexIndex], light);
                    weight = (1.0f - distanceWeight) * (farWeight - weight) + weight;
                    if (weight > farWeight) {
                        weight = farWeight;
                    }
                } else {
                    weight *= EvalDistanceWeight(distances[lightIndex][vertexIndex], light);
                }
            }

            if (light->isDirectedSource != 0) {
                g_Clip_PolyAttr1[vertexIndex] += weight;
                pointSum += weight;
            } else if (hardwarePath != 0 && light->lightParam != 0) {
                g_Clip_PolyAttr2[vertexIndex] += weight;
                hasAttr2Contribution = 1;
                attr2Sum += weight;
            } else {
                fogWeights[vertexIndex] += weight;
                fogSum += weight;
            }
        }

        if (fogSum + attr2Sum > kVisibleWeight) {
            selectedFogLightIndex = lightIndex;
            ++fogContributorCount;
        } else if (pointSum > kVisibleWeight) {
            selectedPointLightIndex = lightIndex;
            ++pointContributorCount;
        }
    }

    if (g_zModel_FogTargetColorOverride.weight > kVisibleWeight) {
        ++fogContributorCount;
        if (hardwarePath != 0) {
            for (i = 0; i < vertexCount; ++i) {
                g_Clip_PolyAttr2[i] += g_zModel_FogTargetColorOverride.weight;
            }
            hasAttr2Contribution = 1;
        } else {
            for (i = 0; i < vertexCount; ++i) {
                fogWeights[i] += g_zModel_FogTargetColorOverride.weight;
            }
        }
    }

    if (fogContributorCount == 0 && pointContributorCount == 0) {
        return 0;
    }

    if (pointContributorCount > 0) {
        if (g_Clip_PolyAttr1[0] > 1.0f) {
            g_Clip_PolyAttr1[0] = 1.0f;
        } else if (g_Clip_PolyAttr1[0] < 0.0f) {
            g_Clip_PolyAttr1[0] = 0.0f;
        }

        if (hardwarePath == 0) {
            for (i = 1; i < vertexCount; ++i) {
                if (g_Clip_PolyAttr1[i] > 1.0f) {
                    g_Clip_PolyAttr1[i] = 1.0f;
                } else if (g_Clip_PolyAttr1[i] < 0.0f) {
                    g_Clip_PolyAttr1[i] = 0.0f;
                }
                if (fabs(g_Clip_PolyAttr1[i] - g_Clip_PolyAttr1[0]) > kVisibleWeight) {
                    pointAttrsVarying = 1;
                }
            }

            if (g_zModel_SoftwarePathActive != 0 && usePaletteRemap != 0) {
                if (pointAttrsVarying != 0 && initialLightingMode != 0) {
                    for (i = 0; i < vertexCount; ++i) {
                        g_Clip_PolyAttr0[i] = g_Clip_PolyAttr1[i] * scale255;
                    }
                    zRndrSetPaletteShadeRecipeIndex(&gModel_SpecialLightPaletteRemapRecipe);
                    *lightingMode |= 1;
                    return 1;
                }

                g_Clip_PolyAttr1[0] *= scale255;
                zRndrSetPaletteRemapKey(&gModel_SpecialLightPaletteRemapRecipe, g_Clip_PolyAttr1[0]);
            } else if (g_zModel_FogTargetColorOverride.weight == 0.0f) {
                ++fogContributorCount;
                for (i = 0; i < vertexCount; ++i) {
                    fogWeights[i] += g_Clip_PolyAttr1[i];
                }
                zRndrSetPaletteRemapKeyFromRgb01(0, 0.0f);
                if (selectedFogLightIndex < 0) {
                    selectedFogLightIndex = selectedPointLightIndex;
                }
            }
        }
    }

    if (fogContributorCount > 0) {
        for (i = 1; i < vertexCount && *lightingMode == 0; ++i) {
            if (fabs(fogWeights[i] - fogWeights[0]) > kVisibleWeight) {
                *lightingMode = 1;
            }
        }
    } else if (pointContributorCount == 0) {
        return 0;
    }

    if (hardwarePath == 0) {
        for (i = 0; i < vertexCount; ++i) {
            if (fogWeights[i] > 1.0f) {
                fogWeights[i] = 1.0f;
            } else if (fogWeights[i] < 0.0f) {
                fogWeights[i] = 0.0f;
            }
            if (fogWeights[i] > kVisibleWeight) {
                resultFlags = 1;
                g_Clip_PolyAttr0[i] += scale255 * fogWeights[i];
            }
        }

        if (resultFlags != 0) {
            if ((*lightFlags & 1) != 0 && fogContributorCount > 0) {
                CommitDirectFogParamsIfChanged();
            } else if (fogContributorCount > 1) {
                CommitDirectFogParamsIfChanged();
            } else if (fogContributorCount == 1) {
                zColorRgb* color = selectedFogLightIndex < 0
                    ? &g_zModel_FogTargetColorOverride.colorRgb01
                    : &gModel_ActiveLights[selectedFogLightIndex].light->specularColor;
                zRndrFogTargetColorStagedSetRgb01Clamped(color);
                CommitStagedFogParamsIfChanged();
            }
        }
        return resultFlags;
    }

    pointAttrsVisible = 0;
    for (i = 0; i < vertexCount; ++i) {
        if (g_Clip_PolyAttr1[i] > 1.0f) {
            g_Clip_PolyAttr1[i] = 1.0f;
        } else if (g_Clip_PolyAttr1[i] < 0.0f) {
            g_Clip_PolyAttr1[i] = 0.0f;
        }
        if (g_Clip_PolyAttr1[i] > kVisibleWeight) {
            pointAttrsVisible = 1;
        }
    }
    if (pointAttrsVisible != 0) {
        *lightingMode |= 1;
    }

    for (i = 0; i < vertexCount; ++i) {
        if (fogWeights[i] > 1.0f) {
            fogWeights[i] = 1.0f;
        } else if (fogWeights[i] < 0.0f) {
            fogWeights[i] = 0.0f;
        }
        if (fogWeights[i] > kVisibleWeight) {
            g_Clip_PolyAttr0[i] += fogWeights[i];
            resultFlags = 1;
        }
    }

    if (hasAttr2Contribution != 0) {
        int attr2Visible = 0;
        for (i = 0; i < vertexCount; ++i) {
            if (g_Clip_PolyAttr2[i] > 1.0f) {
                g_Clip_PolyAttr2[i] = 1.0f;
            } else if (g_Clip_PolyAttr2[i] < 0.0f) {
                g_Clip_PolyAttr2[i] = 0.0f;
            }
            if (g_Clip_PolyAttr2[i] > kVisibleWeight) {
                attr2Visible = 1;
            }
        }

        for (i = 1; i < vertexCount && *lightingMode == 0; ++i) {
            if (fabs(g_Clip_PolyAttr2[i] - g_Clip_PolyAttr2[0]) > kVisibleWeight) {
                *lightingMode = 2;
            }
        }

        if (attr2Visible != 0) {
            int previousFlags;
            zColorRgb* color;
            resultFlags |= 8;
            previousFlags = *lightFlags;
            *lightFlags |= 9;
            color = selectedFogLightIndex < 0 ? &g_zModel_FogTargetColorOverride.colorRgb01
                                              : &gModel_ActiveLights[selectedFogLightIndex].light->specularColor;
            if ((previousFlags & 1) != 0) {
                zVideoSetPendingFogTargetColorFromRgb01((zVideo_ColorRgbFloat*)(color));
                CommitFogTargetColorIfChanged();
            } else {
                SetFogColorFromRgb01((zVideo_ColorRgbFloat*)(color));
                CommitFogColorIfChanged();
            }
        }
    }

    if (selectedFogLightIndex >= 0 && resultFlags != 0) {
        *lightFlags |= 4;
        zVideoSetPendingFogTargetColorFromRgb01(
            (zVideo_ColorRgbFloat*)(&gModel_ActiveLights[selectedFogLightIndex].light->specularColor)
        );
    }

    return resultFlags | pointAttrsVisible;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-buildlightweights
 * @recoil-artifact defines .text recoil:function:0x488d60: zModel_Light::BuildLightWeights
 *
 *
 * Purpose: build software-path per-vertex light weights, choose and commit fog
 * target state, blend the packed fog color, and report whether lighting applied.
 */
int __fastcall
zModelLightBuildLightWeights(zVec3* surfaceNormal, int vertexCount, int* outPackedFogColor, float fogBlendScale)
{
    const double kVisibleWeight = 0.003921569;
    const double kMinPointNormalWeight = kVisibleWeight + 0.0001f;
    const float kMinIntensity = 9.99999975e-6f;

    int hasAnyCandidate = 0;
    // Retail leaves these uninitialized: pass 2 reads a stale lightToVertex[l][0] for
    // full-weight directional lights.
    int valid[0x40][0x40];
    zVec3 lightToVertex[0x40][0x40];
    int vertexIndex;
    float vertexWeights[0x40];
    int i;
    float maxVertexWeight;
    int nonZeroLightCount;
    int singleLightIndex;
    int lightIndex;
    float scale;

    for (vertexIndex = 0; vertexIndex < vertexCount; ++vertexIndex) {
        const zVec3* const vertex = &g_Clip_PolyVertsScratch[vertexIndex];
        for (lightIndex = 0; lightIndex < gModel_ActiveLightCount; ++lightIndex) {
            zModel_ActiveLightEntryLive* entry = &gModel_ActiveLights[lightIndex];
            valid[lightIndex][vertexIndex] = 0;
            if (entry->contributesToLighting == 0) {
                continue;
            }

            if (entry->useFullWeight == 0) {
                CZLightDataPartial* const light = entry->light;
                zVec3* const delta = &lightToVertex[lightIndex][vertexIndex];
                float distanceSq;
                delta->x = light->viewPos.x - vertex->x;
                delta->y = light->viewPos.y - vertex->y;
                delta->z = light->viewPos.z - vertex->z;
                distanceSq = delta->x * delta->x + delta->y * delta->y + delta->z * delta->z;
                gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex] = distanceSq;
                if (distanceSq >= light->range2Sq) {
                    continue;
                }

                if (distanceSq != 0.0f) {
                    int distanceBits = *(const int*)&distanceSq;
                    distanceBits = (distanceBits >> 1) + 0x1fc00000;
                    gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex] = *(float*)&distanceBits;
                }
                zMathVec3DivScalar(delta, delta, gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex]);
            }

            valid[lightIndex][vertexIndex] = 1;
            hasAnyCandidate = 1;
        }
    }

    if (g_zModel_FogTargetColorOverride.weight > kVisibleWeight) {
        hasAnyCandidate = 1;
    }
    if (hasAnyCandidate == 0) {
        return 0;
    }

    Vec3Normalize(surfaceNormal);

    for (i = 0; i < vertexCount; ++i) {
        vertexWeights[i] = 0.0f;
    }

    maxVertexWeight = 0.0f;
    nonZeroLightCount = 0;
    singleLightIndex = -1;

    for (lightIndex = 0; lightIndex < gModel_ActiveLightCount; ++lightIndex) {
        zModel_ActiveLightEntryLive* entry = &gModel_ActiveLights[lightIndex];
        CZLightDataPartial* light;
        float lightWeightSum;
        float surfaceWeight;
        float angularWeight;
        float intensity;
        if (entry->contributesToLighting == 0) {
            continue;
        }

        light = entry->light;
        lightWeightSum = 0.0f;
        if (light->isDirectional == 0 && light->isDirectedSource == 0) {
            surfaceWeight = 1.0f;
        } else {
            const float surfaceDot = surfaceNormal->x * light->viewDir.x + surfaceNormal->y * light->viewDir.y
                + surfaceNormal->z * light->viewDir.z;
            surfaceWeight = surfaceDot;
            if (light->isDirectedSource != 0 && surfaceDot < kMinPointNormalWeight) {
                surfaceWeight = (float)kMinPointNormalWeight;
            }
        }

        // The first vertex also establishes the light base intensity.
        angularWeight = surfaceWeight;
        if (surfaceWeight > kVisibleWeight) {
            if (light->isDirectional != 0) {
                const zVec3* direction = &lightToVertex[lightIndex][0];
                const float coneDot = direction->x * light->viewDir.x + direction->y * light->viewDir.y
                    + direction->z * light->viewDir.z;
                angularWeight = coneDot < kVisibleWeight ? 0.0f : coneDot;
            }
        } else {
            surfaceWeight = 0.0f;
            angularWeight = 0.0f;
        }

        if (surfaceWeight <= kVisibleWeight && light->intensityScale <= kMinIntensity) {
            continue;
        }

        intensity = light->falloff * angularWeight + light->intensityScale;
        if (intensity > 1.0f) {
            intensity = 1.0f;
        } else if (light->intensityScale > intensity) {
            intensity = light->intensityScale;
        }

        if (valid[lightIndex][0]) {
            float weight = light->isDirectedSource != 0 ? 1.0f - intensity : intensity;
            if (entry->useFullWeight == 0) {
                if (light->isDirectedSource != 0) {
                    const float farWeight = 1.0f - light->intensityScale;
                    const float distanceWeight
                        = EvalDistanceWeight(gModel_LightVertexDistanceSqScratch[lightIndex][0], light);
                    weight = (1.0f - distanceWeight) * (farWeight - weight) + weight;
                    if (weight > farWeight) {
                        weight = farWeight;
                    }
                } else {
                    weight *= EvalDistanceWeight(gModel_LightVertexDistanceSqScratch[lightIndex][0], light);
                }
            }
            vertexWeights[0] += weight;
            lightWeightSum += vertexWeights[0];
        }

        for (vertexIndex = 1; vertexIndex < vertexCount; ++vertexIndex) {
            float weight;
            if (!valid[lightIndex][vertexIndex]) {
                continue;
            }

            angularWeight = surfaceWeight;
            if (surfaceWeight > kVisibleWeight) {
                if (light->isDirectional != 0) {
                    zVec3* const direction = &lightToVertex[lightIndex][vertexIndex];
                    float coneDot;
                    if (entry->useFullWeight != 0) {
                        const zVec3* const vertex = &g_Clip_PolyVertsScratch[vertexIndex];
                        direction->x = light->viewPos.x - vertex->x;
                        direction->y = light->viewPos.y - vertex->y;
                        direction->z = light->viewPos.z - vertex->z;
                    }
                    coneDot = direction->x * light->viewDir.x + direction->y * light->viewDir.y
                        + direction->z * light->viewDir.z;
                    angularWeight = coneDot < kVisibleWeight ? 0.0f : coneDot;
                }
            } else {
                angularWeight = 0.0f;
            }

            if (angularWeight <= kVisibleWeight && light->intensityScale <= kMinIntensity) {
                continue;
            }

            if (light->isDirectional != 0) {
                intensity = light->falloff * angularWeight + light->intensityScale;
                if (intensity > 1.0f) {
                    intensity = 1.0f;
                } else if (light->intensityScale > intensity) {
                    intensity = light->intensityScale;
                }
            }

            weight = light->isDirectedSource != 0 ? 1.0f - intensity : intensity;
            if (entry->useFullWeight == 0) {
                if (light->isDirectedSource != 0) {
                    const float farWeight = 1.0f - light->intensityScale;
                    const float distanceWeight
                        = EvalDistanceWeight(gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex], light);
                    weight = (1.0f - distanceWeight) * (farWeight - weight) + weight;
                    if (weight > farWeight) {
                        weight = farWeight;
                    }
                } else {
                    weight *= EvalDistanceWeight(gModel_LightVertexDistanceSqScratch[lightIndex][vertexIndex], light);
                }
            }
            vertexWeights[vertexIndex] += weight;
            lightWeightSum += vertexWeights[vertexIndex];
            if (vertexWeights[vertexIndex] > maxVertexWeight) {
                maxVertexWeight = vertexWeights[vertexIndex];
            }
        }

        if (lightWeightSum > kVisibleWeight) {
            singleLightIndex = lightIndex;
            ++nonZeroLightCount;
        }
    }

    if (g_zModel_FogTargetColorOverride.weight > kVisibleWeight) {
        ++nonZeroLightCount;
        for (i = 0; i < vertexCount; ++i) {
            vertexWeights[i] += g_zModel_FogTargetColorOverride.weight;
        }
    }

    if (nonZeroLightCount == 0) {
        return 0;
    }

    if (fogBlendScale > 0.0f && nonZeroLightCount > 0) {
        if (fogBlendScale > maxVertexWeight) {
            maxVertexWeight = fogBlendScale;
        }
        CommitDirectFogParamsIfChanged();
    } else if (nonZeroLightCount > 1) {
        CommitDirectFogParamsIfChanged();
    } else if (nonZeroLightCount == 1) {
        zColorRgb* color = singleLightIndex >= 0 ? &gModel_ActiveLights[singleLightIndex].light->specularColor
                                                 : &g_zModel_FogTargetColorOverride.colorRgb01;
        zRndrFogTargetColorStagedSetRgb01Clamped(color);
        CommitStagedFogParamsIfChanged();
    }

    if (maxVertexWeight > 1.0f) {
        maxVertexWeight = 1.0f;
    } else if (maxVertexWeight < 0.0f) {
        maxVertexWeight = 0.0f;
    }
    scale = 0.0f;
    Set255f(&scale);
    scale -= 1.0f;
    BlendPackedColor565WithFogInPlace(outPackedFogColor, (int)(maxVertexWeight * scale));
    return 1;
}

float __fastcall
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-evaldistanceweight
 * @recoil-artifact defines .text recoil:function:0x4894f0: zModel_Light::EvalDistanceWeight
 * @recoil-match byte
 *
 * Purpose: compute a light's range falloff as full, zero, or a linear blend
 * between the inner and outer range.
 * Distance-first parameter order reproduces the retail callers' preparation
 * order (distance loaded before ECX = light, then pushed); not uniquely proved.
 */
EvalDistanceWeight(float distance, const CZLightDataPartial* light)
{
    if (distance >= light->range2) {
        return 0.0f;
    }

    if (distance <= light->range1) {
        return 1.0f;
    }

    return (light->range2 - distance) * light->invRangeDelta;
}
