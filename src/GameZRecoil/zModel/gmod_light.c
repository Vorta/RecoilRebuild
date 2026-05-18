#include "GameZRecoil/zModel/zModel.h"

#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

#include <math.h>
#include <string.h>

int gModel_FogEnabled = 0;
int gModel_FogLinearModeEnabled = 0;
float gModel_FogDistanceStart = 0.0f;
float gModel_FogDistanceEnd = 0.0f;
float gModel_FogDistanceInvRange = 0.0f;
float gModel_FogHeightHigh = 0.0f;
float gModel_FogHeightLow = 0.0f;
float gModel_FogHeightInvRange = 0.0f;
float gModel_FogDensity = 0.0f;
zColorRgb gModel_FogColorRgb01 = {0};
int gModel_RenderVertexAlphaEnabled = 0;
float gModel_RenderAlphaScaleCurrent = 1.0f;
int gModel_HasActiveLights = 0;
int gModel_ActiveLightCount = 0;
int gModel_ActiveLightSpecialIndex = 0;
zModel_ActiveLightEntryLive gModel_ActiveLights[0x40] = {0};
zClass_LightDataPartial **gModel_LightInputDataList = 0;
zModel_LightStatePartial **gModel_LightInputNodeStates = 0;
int gModel_LightInputCount = 0;
int g_zModel_SoftwarePathActive = 0;
float g_Clip_PolyAttr0[0x40] = {0};
float g_Clip_PolyAttr1[0x40] = {0};
float g_Clip_PolyAttr2[0x40] = {0};
zModel_FogTargetColorOverride g_zModel_FogTargetColorOverride = {0};
zColorRgb gModel_FogBaseColorRgb01 = {0};
float gModel_AmbientScale = 0.0f;
float gModel_AmbientIntensityFactor = 0.0f;
zColorRgb gModel_AmbientColorRgb01 = {0};
zModel_PaletteRemapRecipePartial gModel_SpecialLightPaletteRemapRecipe = {0};

namespace {
    void UpdateDistanceInvRange(float range) {
        if (range != 0.0f) {
            gModel_FogDistanceInvRange = 1.0f / range;
        }
    }

    void UpdateHeightInvRange(float range) {
        if (range != 0.0f) {
            gModel_FogHeightInvRange = 1.0f / range;
        }
    }

    float ApproximateSqrtFromBits(float value) {
        int bits = 0;
        memcpy(&bits, &value, sizeof(bits));
        bits = (bits >> 1) + 0x1fc00000;

        float result = 0.0f;
        memcpy(&result, &bits, sizeof(result));
        return result;
    }

    float EvalHeightFogFade(const zVec3 *point, float radius) {
        float projectedY = 0.0f;
        zMath::Vec3ArrayProjectToCachedY(point, &projectedY, 1);

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

    float ClampWeight(float weight) {
        if (weight > 1.0f) {
            return 1.0f;
        }

        if (weight < 0.0f) {
            return 0.0f;
        }

        return weight;
    }

    float DotVec3(const zVec3 &a, const zVec3 &b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    zVec3 SubtractVec3(const zVec3 &a, const zVec3 &b) {
        zVec3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
        return result;
    }
}

// Reimplements 0x476170: zModel_Fog_SetEnabled
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Fog_SetEnabled(int enabled) {
    gModel_FogEnabled = enabled;
}

// Reimplements 0x476180: zModel_Fog_IsEnabled (GameZRecoil/zModel/zmodel.cpp)
RECOIL_NOINLINE int RECOIL_CDECL zModel_Fog_IsEnabled() {
    return gModel_FogEnabled;
}

// Reimplements 0x476190: zModel_Fog_SetDistanceStart
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetDistanceStart(float distanceStart) {
    const float range = gModel_FogDistanceEnd - distanceStart;
    gModel_FogDistanceStart = distanceStart;
    UpdateDistanceInvRange(range);
}

// Reimplements 0x4761d0: zModel_Fog_GetDistanceStart
// (GameZRecoil/zModel/zModel_Display.cpp)
RECOIL_NOINLINE float RECOIL_CDECL zModel_Fog_GetDistanceStart() {
    return gModel_FogDistanceStart;
}

// Reimplements 0x4761e0: zModel_Fog_SetDistanceEnd
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetDistanceEnd(float distanceEnd) {
    const float range = distanceEnd - gModel_FogDistanceStart;
    gModel_FogDistanceEnd = distanceEnd;
    UpdateDistanceInvRange(range);
}

// Reimplements 0x476220: zModel_Fog_SetHeightHigh
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetHeightHigh(float heightHigh) {
    const float range = heightHigh - gModel_FogHeightLow;
    gModel_FogHeightHigh = heightHigh;
    UpdateHeightInvRange(range);
}

// Reimplements 0x476260: zModel_Fog_SetHeightLow
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetHeightLow(float heightLow) {
    const float range = gModel_FogHeightHigh - heightLow;
    gModel_FogHeightLow = heightLow;
    UpdateHeightInvRange(range);
}

// Reimplements 0x4762a0: zModel_Fog_SetDensity
RECOIL_NOINLINE void RECOIL_STDCALL zModel_Fog_SetDensity(float density) {
    gModel_FogDensity = density;
}

// Reimplements 0x4762b0: zModel_Fog_SetLinearModeEnabled
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Fog_SetLinearModeEnabled(int enabled) {
    gModel_FogLinearModeEnabled = enabled;
}

// Reimplements 0x4762c0: zModel_Fog_SetColorRgb01
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Fog_SetColorRgb01(zColorRgb *rgb01) {
    memcpy(&gModel_FogColorRgb01, rgb01, sizeof(gModel_FogColorRgb01));
    if (g_zVideo_ActiveRendererPath != 0) {
        zVideo::SetFogColorFromRgb01(reinterpret_cast<zVideo_ColorRgbFloat *>(rgb01));
    }
}

// Reimplements 0x4762f0: zModel_Fog_ApplyCurrentColor
RECOIL_NOINLINE void RECOIL_CDECL zModel_Fog_ApplyCurrentColor() {
    zRndr::FogColor_SetRgb01Clamped(&gModel_FogColorRgb01);
}

// Reimplements 0x476040: zModel_FogTargetColorOverride_SetCurrent
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_FogTargetColorOverride_SetCurrent(
    zColorRgb *colorRgb01, float weight) {
    if (colorRgb01 != 0) {
        g_zModel_FogTargetColorOverride.colorRgb01 = *colorRgb01;
    }
    g_zModel_FogTargetColorOverride.weight = weight;
}

// Reimplements 0x476070: zModel_RenderAlphaScale_SetCurrent
RECOIL_NOINLINE void RECOIL_STDCALL zModel_RenderAlphaScale_SetCurrent(float scale) {
    gModel_RenderAlphaScaleCurrent = scale;
}

// Reimplements 0x476080: zModel_RenderVertexAlphaEnabled_SetCurrent
RECOIL_NOINLINE void RECOIL_FASTCALL
zModel_RenderVertexAlphaEnabled_SetCurrent(int enabled) {
    gModel_RenderVertexAlphaEnabled = enabled;
}

namespace zModel_Light {
    // Reimplements 0x4894f0: zModel_Light::EvalDistanceWeight
    RECOIL_NOINLINE float RECOIL_FASTCALL EvalDistanceWeight(const zClass_LightDataPartial *light,
                                                             float distance) {
        if (distance >= light->range2) {
            return 0.0f;
        }

        if (distance <= light->range1) {
            return 1.0f;
        }

        return (light->range2 - distance) * light->invRangeDelta;
    }

    // Reimplements 0x489540: zModel_Light::EvalSphereFogFade
    RECOIL_NOINLINE float RECOIL_FASTCALL EvalSphereFogFade(const zVec3 *point, float radius) {
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

        const float heightFade = EvalHeightFogFade(point, radius);
        const float fade = heightFade * distanceFade;
        if (fade > 1.0f) {
            return 1.0f;
        }

        if (fade < 0.0f) {
            return 0.0f;
        }

        return fade;
    }

    // Reimplements 0x4896d0: zModel_Light::BuildAttr0DepthFade
    RECOIL_NOINLINE int RECOIL_FASTCALL BuildAttr0DepthFade(
        int vertexCount, int *outHasVariation) {
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
                reinterpret_cast<const zVec3 *>(&g_Clip_PolyVertsScratch[i_250]), &projectedY, 1);

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

    // Reimplements 0x489920: zModel_Light::EvalBatchSphereFade
    RECOIL_NOINLINE int RECOIL_FASTCALL EvalBatchSphereFade(float *outFade) {
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
            reinterpret_cast<const zVec3 *>(&g_Clip_PolyVertsScratch[0]), &projectedY, 1);

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
}

RECOIL_NOINLINE int RECOIL_FASTCALL
zModel_Light_BuildLightWeights(zVec3 *surfaceNormal, int vertexCount,
                               int *outPackedFogColor, float fogBlendScale) {
    const float kVisibleWeight = 1.0f / 255.0f;
    const float kMinPointNormalWeight = 0.00402156916f;
    const float kMinIntensity = 9.99999975e-6f;

    bool hasAnyCandidate = false;
    bool valid[0x40][0x40] = {0};
    float distances[0x40][0x40] = {0};
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
            zVec3 delta = SubtractVec3(light->viewPos, vertex);
            float distanceSq = DotVec3(delta, delta);
            distances[lightIndex][vertexIndex] = distanceSq;
            if (distanceSq >= light->range2Sq) {
                continue;
            }

            if (distanceSq != 0.0f) {
                distances[lightIndex][vertexIndex] = ApproximateSqrtFromBits(distanceSq);
                zMath_Vec3_DivScalar(&delta, &delta, distances[lightIndex][vertexIndex]);
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
                angularWeight = DotVec3(*surfaceNormal, direction);
                if (light->isPointMode != 0 && angularWeight < kMinPointNormalWeight) {
                    angularWeight = kMinPointNormalWeight;
                }
                if (light->coneAngle != 0) {
                    const float coneDot = DotVec3(direction, light->viewDir);
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
                const float distanceWeight =
                    zModel_Light::EvalDistanceWeight(light, distances[lightIndex][vertexIndex]);
                if (light->isPointMode != 0) {
                    const float farWeight = 1.0f - light->intensityScale;
                    baseWeight = (1.0f - distanceWeight) * (farWeight - baseWeight) + baseWeight;
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
    zRndr::BlendPackedColor565WithFogInPlace(outPackedFogColor,
                                             static_cast<int>(maxVertexWeight * scale));
    return 1;
}

// Reimplements 0x487a30: zModel_Light_PointInPolygonInitXZ
RECOIL_NOINLINE void RECOIL_FASTCALL zModel_Light_PointInPolygonInitXZ(
    zClass_LightDataPartial **lightDataList, zModel_LightStatePartial **lightNodeStates,
    int lightCount) {
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
            zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zModel\\gmod_light.c", 0x46,
                              "Not enough MAX_LIGHTS: %d; requesting more.", 0x40);
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
    // Reimplements 0x487c50: zModel_Light::PointInPolygonTestRadiusXZ
    RECOIL_NOINLINE int RECOIL_FASTCALL PointInPolygonTestRadiusXZ(
        const zVec3 *sphereCenter, float radius) {
        float lightDistances[0x40] = {0};
        int result = 0;
        int hasSoftwarePointLight = 0;

        for (int i = 0; i < gModel_ActiveLightCount; ++i) {
            zModel_ActiveLightEntryLive &entry = gModel_ActiveLights[i];
            zClass_LightDataPartial *light = entry.light;
            entry.useFullWeight = 0;
            entry.contributesToLighting = 0;

            if ((entry.lightState->flags & 4) == 0) {
                zError::ReportOld(0x200, "D:\\Proj\\GameZRecoil\\zModel\\gmod_light.c", 0xfa,
                                  "Never get here?");
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
                entry.useFullWeight != 0 ? 1.0f : EvalDistanceWeight(light, lightDistances[i_647]);
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
}
