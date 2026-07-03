#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "opt_catalog.h"

#include <cstdlib>

namespace {
bool TestFloatNear(float actual, float expected) {
    float delta = actual - expected;
    if (delta < 0.0f) {
        delta = -delta;
    }
    return delta <= 0.0001f;
}
}

extern "C" int player_timed_hit_status_smoke(void) {
    zClass_NodePartial parent = {};
    zClass_NodePartial oldHitSourceNode = {};
    OptCatalogEntryDef oldHitSource = {};
    PlayerTimedHitStatus status = {};
    status.runtimeFlags = 0xff;
    status.hitSource = &oldHitSource;
    status.currentLevel = 0.5f;
    status.targetLevel = -0.5f;
    status.lightNode = &oldHitSourceNode;
    status.nextUpdateTime = 10.0f;
    status.lightParentNode = &parent;

    status.ResetFields();
    if (status.runtimeFlags != 0xfcu || status.hitSource != &oldHitSource ||
        status.currentLevel != 0.0f || status.targetLevel != 0.0f ||
        status.lightNode != 0 || status.nextUpdateTime != 0.0f ||
        status.lightParentNode != &parent) {
        return 1;
    }

    status.runtimeFlags = 3;
    status.currentLevel = 0.25f;
    status.targetLevel = 0.75f;
    status.nextUpdateTime = 1.0f;
    status.ClearLightAndReset();
    if (status.runtimeFlags != 3 || status.currentLevel != 0.25f ||
        status.targetLevel != 0.75f || status.nextUpdateTime != 1.0f) {
        return 2;
    }

    zClass_NodePartial *const oldThermalGlowFreeList = g_OptCatalogThermalGlowFreeList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;

    zClass_NodePartial light = {};
    zClass_NodePartial nextLight = {};
    zClass_LightDataPartial lightData = {};
    light.classData = &lightData;
    light.callbackContext = &nextLight;

    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial world = {};
    world.classData = &worldData;
    parent.classId = 3;

    OptCatalogEntryDef source = {};
    source.timedStatusLightSpecularColor = {0.2f, 0.4f, 0.6f};
    status = {};
    status.currentLevel = -0.75f;
    status.targetLevel = 0.9f;
    status.lightParentNode = &parent;
    g_OptCatalogThermalGlowFreeList = &light;
    g_OptCatalogRuntimeWorld = &world;

    const int lowBand = HitSource::UpdateTimedStatus(&source, &status, 0.4f);
    const bool allocOk =
        lowBand == 2 && status.runtimeFlags == 3 && status.hitSource == &source &&
        status.targetLevel == 1.0f && status.lightNode == &light &&
        g_OptCatalogThermalGlowFreeList == &nextLight && lightData.range1 == 0.1f &&
        lightData.range2 == 0.2f && lightData.specularColor.red == 0.2f &&
        lightData.specularColor.green == 0.4f && lightData.specularColor.blue == 0.6f &&
        worldData.lightCount == 1 && parent.listCountB == 1 && parent.listB[0] == &light;
    if (!allocOk) {
        std::free(worldData.lightNodes);
        std::free(worldData.lightDataList);
        std::free(lightData.attachedWorlds);
        std::free(parent.listB);
        std::free(light.listA);
        g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        return 3;
    }

    status.ClearLightAndReset();
    const bool clearOk = status.runtimeFlags == 0 && status.lightNode == 0 &&
                         status.currentLevel == 0.0f && status.targetLevel == 0.0f &&
                         status.nextUpdateTime == 0.0f &&
                         g_OptCatalogThermalGlowFreeList == &light &&
                         worldData.lightCount == 0 && parent.listCountB == 0;
    if (!clearOk) {
        std::free(worldData.lightNodes);
        std::free(worldData.lightDataList);
        std::free(lightData.attachedWorlds);
        std::free(parent.listB);
        std::free(light.listA);
        g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        return 4;
    }

    source.flags = 0x200;
    status = {};
    status.currentLevel = 0.25f;
    status.targetLevel = -0.8f;
    status.lightNode = &nextLight;
    status.lightParentNode = &parent;
    const int highBand = HitSource::UpdateTimedStatus(&source, &status, 0.5f);
    status.currentLevel = 0.0f;
    const int middleBand = HitSource::UpdateTimedStatus(&source, &status, 0.1f);
    const bool clampAndBandOk = highBand == 0 && middleBand == 1 &&
                                status.targetLevel == -1.0f && status.lightNode == &nextLight;
    if (!clampAndBandOk) {
        std::free(worldData.lightNodes);
        std::free(worldData.lightDataList);
        std::free(lightData.attachedWorlds);
        std::free(parent.listB);
        std::free(light.listA);
        g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        return 5;
    }

    const float oldFrameDeltaTimeSec = g_FrameDeltaTimeSec;
    const float oldAccumulatedTimeSec = g_Time_AccumulatedTimeSec;
    const int oldApproxExpNegDirty = g_zMath_ApproxExpNegDirty;

    zClass_NodePartial tickLight = {};
    zClass_LightDataPartial tickLightData = {};
    tickLight.classData = &tickLightData;
    source.timedStatusInterpRate = 1.0f;
    source.timedStatusLightRangeMin = 2.0f;
    source.timedStatusLightRangeMax = 6.0f;
    source.timedStatusUpdateDelay = 1.5f;
    source.timedStatusLightSpecularColor = {0.3f, 0.5f, 0.7f};
    status = {};
    status.runtimeFlags = 2;
    status.hitSource = &source;
    status.currentLevel = 0.25f;
    status.targetLevel = -0.75f;
    status.lightNode = &tickLight;
    status.nextUpdateTime = 0.25f;
    g_FrameDeltaTimeSec = 0.5f;
    g_Time_AccumulatedTimeSec = 4.0f;

    const int interpolatingBand = status.TickAndUpdateLight(2.0f);
    const bool interpolationOk =
        interpolatingBand == 1 && status.runtimeFlags == 2 &&
        TestFloatNear(status.currentLevel, -0.25f) &&
        TestFloatNear(status.nextUpdateTime, 5.5f) &&
        TestFloatNear(tickLightData.range1, 1.0f) &&
        TestFloatNear(tickLightData.range2, 3.0f) &&
        TestFloatNear(tickLightData.specularColor.red, 0.3f) &&
        TestFloatNear(tickLightData.specularColor.green, 0.5f) &&
        TestFloatNear(tickLightData.specularColor.blue, 0.7f);

    zClass_NodePartial decayLight = {};
    zClass_LightDataPartial decayLightData = {};
    decayLight.classData = &decayLightData;
    status = {};
    status.hitSource = &source;
    status.currentLevel = 0.5f;
    status.targetLevel = 0.9f;
    status.lightNode = &decayLight;
    status.nextUpdateTime = 3.0f;
    g_FrameDeltaTimeSec = 0.5f;
    g_Time_AccumulatedTimeSec = 4.0f;

    const int decayBand = status.TickAndUpdateLight(1.5f);
    const float expectedFade = g_zMath_ApproxExpNegTable[19] * 0.5f;
    const float expectedScale = 1.5f * expectedFade;
    const bool decayOk = decayBand == 0 && TestFloatNear(status.currentLevel, expectedFade) &&
                         TestFloatNear(status.targetLevel, expectedFade) &&
                         TestFloatNear(decayLightData.range1, 2.0f * expectedScale) &&
                         TestFloatNear(decayLightData.range2, 6.0f * expectedScale);

    status = {};
    status.hitSource = &source;
    status.currentLevel = -0.25f;
    status.targetLevel = -0.25f;
    status.nextUpdateTime = 10.0f;
    g_Time_AccumulatedTimeSec = 4.0f;
    const int waitingBand = status.TickAndUpdateLight(1.0f);
    const bool waitingOk = waitingBand == 1 && status.currentLevel == -0.25f &&
                           status.targetLevel == -0.25f;

    g_FrameDeltaTimeSec = oldFrameDeltaTimeSec;
    g_Time_AccumulatedTimeSec = oldAccumulatedTimeSec;
    g_zMath_ApproxExpNegDirty = oldApproxExpNegDirty;

    std::free(worldData.lightNodes);
    std::free(worldData.lightDataList);
    std::free(lightData.attachedWorlds);
    std::free(parent.listB);
    std::free(light.listA);
    g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    if (!interpolationOk) {
        return 6;
    }
    if (!decayOk) {
        return 7;
    }
    return waitingOk ? 0 : 8;
}
