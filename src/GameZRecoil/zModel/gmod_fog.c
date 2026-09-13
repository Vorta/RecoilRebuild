#include "GameZRecoil/zModel/gmod.h"

#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <math.h>
#include <string.h>

/* Fog evaluator implementation; the original filename remains unresolved. */
namespace zModel_Light {
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-evalspherefogfade
     * @recoil-artifact defines .text recoil:function:0x489540: zModel_Light::EvalSphereFogFade
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmodel.sphere-fog-distance recoil:function:0x489540
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmodel.sphere-fog-distance
     * @recoil-match byte
     *
     * Purpose: combine sphere distance and height fog; the reviewed inline asm conversion
     * reproduces retail after failed VC5 C++ variants.
     */
    float __fastcall EvalSphereFogFade(const zVec3 *point, float radius) {
        const float distSqXZ = point->z * point->z + point->x * point->x;
        float distanceXZ;
        /**
         * Purpose: reproduce the retail distance estimate with inline asm and named locals after the VC5 C++ conversion variants failed.
         */
        __asm {
            mov eax, distSqXZ
            sar eax, 1
            add eax, 01fc00000h
            mov distanceXZ, eax
        }
        float farEdge = distanceXZ + radius;
        if (farEdge <= gModel_FogDistanceStart) {
            return 0.0f;
        }
        float projectedY;
        if (distanceXZ - radius >= gModel_FogDistanceEnd) {
            distanceXZ = 1.0f;
            zMath::Vec3ArrayProjectToCachedY(point, &projectedY, 1);
            float bottom = projectedY - radius;
            if (bottom >= gModel_FogHeightHigh) {
                distanceXZ = 0.0f;
            } else if (projectedY + radius > gModel_FogHeightLow) {
                if (bottom < gModel_FogHeightLow) {
                    bottom = gModel_FogHeightLow;
                }
                distanceXZ = (gModel_FogHeightHigh - bottom) * gModel_FogHeightInvRange;
            }
        } else {
            if (farEdge > gModel_FogDistanceEnd) {
                farEdge = gModel_FogDistanceEnd;
            }
            distanceXZ = (farEdge - gModel_FogDistanceStart) * gModel_FogDistanceInvRange;
            zMath::Vec3ArrayProjectToCachedY(point, &projectedY, 1);
            float bottom = projectedY - radius;
            if (bottom >= gModel_FogHeightHigh) {
                distanceXZ = 0.0f;
            } else if (projectedY + radius > gModel_FogHeightLow) {
                if (bottom < gModel_FogHeightLow) {
                    bottom = gModel_FogHeightLow;
                }
                float heightOnly; // Unused capture retained for proven VC5 byte output.
                distanceXZ = (heightOnly = (gModel_FogHeightHigh - bottom) * gModel_FogHeightInvRange) * distanceXZ;
            }
        }
        if (distanceXZ > 1.0f) {
            return 1.0f;
        }
        if (distanceXZ < 0.0f) {
            distanceXZ = 0.0f;
        }
        return distanceXZ;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-buildattr0depthfade
     * @recoil-artifact defines .text recoil:function:0x4896d0: zModel_Light::BuildAttr0DepthFade
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmodel.attr0-fog-distance recoil:function:0x4896d0
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmodel.attr0-fog-distance
     * @recoil-match byte
     *
     * Purpose: build per-vertex attr0 fog weights with 255-scale output; reviewed inline asm reproduces retail after failed VC5 C++ distance-estimate variants.
     */
    int __fastcall BuildAttr0DepthFade(int vertexCount, int *outHasVariation) {
        float projectedY;
        int hasAnyFogCandidate = 0;
        float attrScale;
        int result = 0;
        const double kVisibleAttrThreshold = 0.003921569;
        float radialDistance[0x40];
        for (int i = 0; i < vertexCount; ++i) {
            const zClipVert &vert = g_Clip_PolyVertsScratch[i];
            radialDistance[i] = vert.z * vert.z + vert.x * vert.x;
            const float distanceSq = radialDistance[i];
            float distance;
            /**
             * Purpose: reproduce the retail signed bit-pattern distance estimate through named locals after native VC5 forms failed.
             */
            __asm {
                mov eax, distanceSq
                sar eax, 1
                add eax, 01fc00000h
                mov distance, eax
            }
            radialDistance[i] = distance;
        }
        float attrFade[0x40];
        zFloat::Set255f(&attrScale);
        for (int i_250 = 0; i_250 < vertexCount; ++i_250) {
            const float &distance = radialDistance[i_250];
            if (distance < gModel_FogDistanceStart) {
                attrFade[i_250] = 0.0f;
                continue;
            }
            if (distance >= gModel_FogDistanceEnd) {
                attrFade[i_250] = 1.0f;
                zMath::Vec3ArrayProjectToCachedY((const zVec3 *)&g_Clip_PolyVertsScratch[i_250], &projectedY, 1);
                if (projectedY >= gModel_FogHeightHigh) {
                    attrFade[i_250] = 0.0f;
                } else if (projectedY > gModel_FogHeightLow) {
                    attrFade[i_250] = (gModel_FogHeightHigh - projectedY) * gModel_FogHeightInvRange;
                }
                hasAnyFogCandidate = 1;
            } else {
                attrFade[i_250] = (distance - gModel_FogDistanceStart) * gModel_FogDistanceInvRange;
                zMath::Vec3ArrayProjectToCachedY((const zVec3 *)&g_Clip_PolyVertsScratch[i_250], &projectedY, 1);
                if (projectedY >= gModel_FogHeightHigh) {
                    attrFade[i_250] = 0.0f;
                } else if (projectedY > gModel_FogHeightLow) {
                    attrFade[i_250] = (gModel_FogHeightHigh - projectedY) * gModel_FogHeightInvRange * attrFade[i_250];
                }
                hasAnyFogCandidate = 1;
            }
        }

        if (hasAnyFogCandidate == 0) {
            return 0;
        }
        for (int i_281 = 0; i_281 < vertexCount; ++i_281) {
            if (attrFade[i_281] > 1.0f) {
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
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmodel.vertex-fog-distance recoil:function:0x489920
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmodel.vertex-fog-distance
     * @recoil-match byte
     *
     * Purpose: evaluate current-vertex distance and height fog with the reviewed inline asm
     * conversion after failed VC5 C++ variants, and store/report a contributing fade.
     */
    int __fastcall EvalBatchSphereFade(float *outFade) {
        float projectedY;
        int hasFogContribution = 0;
        const float distanceSq = g_Clip_PolyVertsScratch[0].z * g_Clip_PolyVertsScratch[0].z +
                                 g_Clip_PolyVertsScratch[0].x * g_Clip_PolyVertsScratch[0].x;
        float distance;
        /**
         * Purpose: reproduce the retail distance estimate with inline asm and named locals after the VC5 C++ conversion variants failed.
         */
        __asm {
            mov eax, distanceSq
            sar eax, 1
            add eax, 01fc00000h
            mov distance, eax
        }
        if (distance <= gModel_FogDistanceStart) {
            distance = 0.0f;
        } else {
            if (distance >= gModel_FogDistanceEnd) {
                distance = 1.0f;
                zMath::Vec3ArrayProjectToCachedY((const zVec3 *)&g_Clip_PolyVertsScratch[0], &projectedY, 1);
                if (projectedY >= gModel_FogHeightHigh) {
                    distance = 0.0f;
                } else if (projectedY > gModel_FogHeightLow) {
                    distance = (gModel_FogHeightHigh - projectedY) * gModel_FogHeightInvRange;
                }
            } else {
                distance = (distance - gModel_FogDistanceStart) * gModel_FogDistanceInvRange;
                zMath::Vec3ArrayProjectToCachedY((const zVec3 *)&g_Clip_PolyVertsScratch[0], &projectedY, 1);
                if (projectedY >= gModel_FogHeightHigh) {
                    distance = 0.0f;
                } else if (projectedY > gModel_FogHeightLow) {
                    float heightOnly; // Unused capture retained for proven VC5 byte output.
                    distance = (heightOnly = (gModel_FogHeightHigh - projectedY) * gModel_FogHeightInvRange) * distance;
                }
            }
            hasFogContribution = 1;
        }
        if (hasFogContribution == 0) {
            return 0;
        }
        if (distance > 1.0f) {
            distance = 1.0f;
        } else if (distance < 0.0f) {
            distance = 0.0f;
        }
        *outFade = distance;
        return distance > 0.005f ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zmodel-gmod-light-zmodel-light-buildattr1falloff
     * @recoil-artifact defines .text recoil:function:0x489a90: zModel_Light::BuildAttr1Falloff
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmodel.attr2-fog-distance recoil:function:0x489a90
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zmodel.attr2-fog-distance
     * @recoil-match byte
     *
     * Purpose: build attr2 fog weights and commit the fog color; reviewed inline asm reproduces retail after failed VC5 C++ distance-estimate variants.
     */
    int __fastcall BuildAttr1Falloff(int vertexCount, int *pLightingFlags) {
        float projectedY;
        int hasFogContribution = 0;
        const double kVisibleAttrThreshold = 0.003921569;
        float radialDistance[0x40];
        for (int i = 0; i < vertexCount; ++i) {
            const zClipVert &vert = g_Clip_PolyVertsScratch[i];
            radialDistance[i] = vert.z * vert.z + vert.x * vert.x;
            const float distanceSq = radialDistance[i];
            float distance;
            /**
             * Purpose: reproduce the retail signed bit-pattern distance estimate through named locals after native VC5 forms failed.
             */
            __asm {
                mov eax, distanceSq
                sar eax, 1
                add eax, 01fc00000h
                mov distance, eax
            }
            radialDistance[i] = distance;
        }
        for (int fogIndex = 0; fogIndex < vertexCount; ++fogIndex) {
            const float &distance = radialDistance[fogIndex];
            if (distance <= gModel_FogDistanceStart) {
                g_Clip_PolyAttr2[fogIndex] = 0.0f;
                continue;
            }
            if (distance >= gModel_FogDistanceEnd) {
                g_Clip_PolyAttr2[fogIndex] = 1.0f;
                zMath::Vec3ArrayProjectToCachedY((const zVec3 *)&g_Clip_PolyVertsScratch[fogIndex], &projectedY, 1);
                if (projectedY >= gModel_FogHeightHigh) {
                    g_Clip_PolyAttr2[fogIndex] = 0.0f;
                } else if (projectedY > gModel_FogHeightLow) {
                    g_Clip_PolyAttr2[fogIndex] = (gModel_FogHeightHigh - projectedY) * gModel_FogHeightInvRange;
                }
                hasFogContribution = 1;
            } else {
                g_Clip_PolyAttr2[fogIndex] = (distance - gModel_FogDistanceStart) * gModel_FogDistanceInvRange;
                zMath::Vec3ArrayProjectToCachedY((const zVec3 *)&g_Clip_PolyVertsScratch[fogIndex], &projectedY, 1);
                if (projectedY >= gModel_FogHeightHigh) {
                    g_Clip_PolyAttr2[fogIndex] = 0.0f;
                } else if (projectedY > gModel_FogHeightLow) {
                    g_Clip_PolyAttr2[fogIndex] = (gModel_FogHeightHigh - projectedY) * gModel_FogHeightInvRange * g_Clip_PolyAttr2[fogIndex];
                }
                hasFogContribution = 1;
            }
        }
        if (hasFogContribution == 0) {
            *pLightingFlags &= ~2;
            return 0;
        }
        for (int clampIndex = 0; clampIndex < vertexCount; ++clampIndex) {
            if (g_Clip_PolyAttr2[clampIndex] > 1.0f)
                g_Clip_PolyAttr2[clampIndex] = 1.0f;
            else if (g_Clip_PolyAttr2[clampIndex] < 0.0f)
                g_Clip_PolyAttr2[clampIndex] = 0.0f;
        }
        int hasVisibleFog = 0;
        for (int visibleIndex = 0; visibleIndex < vertexCount && hasVisibleFog == 0; ++visibleIndex) {
            if (g_Clip_PolyAttr2[visibleIndex] > kVisibleAttrThreshold)
                hasVisibleFog = 1;
        }
        if (hasVisibleFog == 0) {
            *pLightingFlags &= ~2;
        } else {
            for (int varianceIndex = 1; varianceIndex < vertexCount; ++varianceIndex) {
                if (fabs(g_Clip_PolyAttr2[varianceIndex] - g_Clip_PolyAttr2[0]) > kVisibleAttrThreshold) {
                    *pLightingFlags |= 2;
                    break;
                }
            }
            zVideo::SetFogColorFromRgb01((zVideo_ColorRgbFloat *)(&gModel_FogColorRgb01));
            zVideo::CommitFogColorIfChanged();
        }
        return hasVisibleFog;
    }

}
