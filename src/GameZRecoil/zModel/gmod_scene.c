#include "GameZRecoil/zModel/zModel.h"

/**
 * Reimplements data 0x57624c: gModel_SmallPolyRejectArea2x.
 * Purpose: cache the doubled small-polygon reject-area threshold.
 */
float gModel_SmallPolyRejectArea2x = 0.0f;
/**
 * Reimplements data 0x576250: gModel_SmallPolyRejectArea20x.
 * Purpose: cache the twenty-times small-polygon reject-area threshold.
 */
float gModel_SmallPolyRejectArea20x = 0.0f;

namespace zModel {
    /**
     * Reimplements 0x4804c0: zModel::UpdateSmallPolyRejectThresholds
     * (Battlesport/zModel/gmod_scene.c).
     *
     * Purpose: cache the doubled and twenty-times small-polygon reject-area
     * thresholds used by projected model clipping.
     */
    void __stdcall UpdateSmallPolyRejectThresholds(float baseRejectArea) {
        const float doubledArea = baseRejectArea + baseRejectArea;
        gModel_SmallPolyRejectArea2x = doubledArea;
        gModel_SmallPolyRejectArea20x = doubledArea * 10.0f;
    }
} // namespace zModel
