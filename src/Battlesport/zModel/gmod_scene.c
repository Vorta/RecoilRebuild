#include "GameZRecoil/zModel/zModel.h"

float gModel_SmallPolyRejectArea2x = 0.0f;
float gModel_SmallPolyRejectArea20x = 0.0f;

namespace zModel {
// Reimplements 0x4804c0: zModel::UpdateSmallPolyRejectThresholds
// (Battlesport/zModel/gmod_scene.c)
RECOIL_NOINLINE void RECOIL_STDCALL UpdateSmallPolyRejectThresholds(float baseRejectArea) {
    const float doubledArea = baseRejectArea + baseRejectArea;
    gModel_SmallPolyRejectArea2x = doubledArea;
    gModel_SmallPolyRejectArea20x = doubledArea * 10.0f;
}
} // namespace zModel
