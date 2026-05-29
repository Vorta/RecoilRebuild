#include "zWeapon.h"

#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zMath/zMath.h"
#include "OptCatalog.h"

#include <math.h>

extern "C" {
int g_zWeapon_ZarHandlerRegistered = 1;
char g_zWeapon_ArchiveName[8] = "Weapons";
float g_zWeapon_MaxTetherAltitude = 0.0f;
}

namespace {
template <typename T> zZbdSectionCallback ZbdCallbackPtr(T callback) {
    RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T typed;
        zZbdSectionCallback raw;
    } ptr = {callback};
    return ptr.raw;
}
} // namespace

// Reimplements 0x4b1090: zWepInit
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zWepInit() {
    g_OptCatalog_FallbackImpactProbeEnabled = 1;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;

    const int shouldRegisterZarHandler = g_zWeapon_ZarHandlerRegistered;

    g_OptCatalog_EntryCount = 0;
    g_OptCatalog_EntryTable = 0;
    g_OptCatalogRuntimeInstanceCount = 0;
    g_OptCatalogRuntimeInstancePool = 0;
    g_OptCatalogFreeRuntimeInstanceList = 0;
    g_OptCatalogRuntimeWorld = 0;
    g_OptCatalogPendingSpawnTargetCountPtr = 0;
    g_OptCatalogPendingSpawnTargetListPtr = 0;
    g_OptCatalogMaxCraterRadius = 30.0f;
    g_OptCatalogQueuedImpactCount = 0;
    g_OptCatalog_DamageContextKind = 0;
    g_OptCatalog_DamageContextHitEvent = 0;
    g_zWeapon_MaxTetherAltitude = 30.0f;
    g_OptCatalogDamageFeedbackCallback = 0;
    g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
    g_OptCatalog_DamageFeedbackHitCount = 0;
    g_OptCatalogDamageFeedbackTrackedNode = 0;
    g_OptCatalogNextSpawnScale = 1.0f;

    if (shouldRegisterZarHandler != 0) {
        zUtil_ZAR::RegisterSectionHandler(
            g_zWeapon_ArchiveName, ZbdCallbackPtr(&zWeapon::OnWeaponsSectionPreLoad),
            ZbdCallbackPtr(&zWeapon::OnWeaponsSectionDataReady), 0x3e8, 0);
    }

    return 0;
}

namespace zWeapon {
// Reimplements 0x4b1140: zWeapon::OnWeaponsSectionPreLoad
// (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL OnWeaponsSectionPreLoad(zZbdSectionCallbackCtx *callbackCtx,
                                                            void *) {
    int weaponDataHitCount = g_OptCatalog_DamageFeedbackHitCount;
    return zUtil_ZAR::WriteSectionBlob(callbackCtx, "WeaponData", &weaponDataHitCount,
                                       sizeof(weaponDataHitCount));
}

// Reimplements 0x4b1160: zWeapon::OnWeaponsSectionDataReady
// (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
OnWeaponsSectionDataReady(zZbdSectionCallbackCtx *, const char *, void *weaponData,
                          unsigned int, void *) {
    g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
    g_OptCatalog_DamageFeedbackHitCount = *(int *)(weaponData);
}

// Reimplements 0x4b1d80: zWeapon::SetMaxTetherAltitude
// (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
RECOIL_NOINLINE void RECOIL_STDCALL SetMaxTetherAltitude(float altitude) {
    g_zWeapon_MaxTetherAltitude = altitude;
}
} // namespace zWeapon

// Reimplements 0x4b21c0: PlayerTimedHitStatus::ResetFields
// (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL PlayerTimedHitStatus::ResetFields() {
    runtimeFlags &= ~3u;
    lightNode = 0;
    currentLevel = 0.0f;
    targetLevel = 0.0f;
    nextUpdateTime = 0.0f;
}

// Reimplements 0x4b22d0: PlayerTimedHitStatus::ClearLightAndReset
// (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL PlayerTimedHitStatus::ClearLightAndReset() {
    if (lightNode != 0) {
        zClass_Class::RemoveChild(lightParentNode, lightNode);
        Light::ReturnToFreeList(lightNode);
        ResetFields();
    }
}

// Reimplements 0x4b2300: PlayerTimedHitStatus::TickAndUpdateLight
// (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL PlayerTimedHitStatus::TickAndUpdateLight(float hitStatus) {
    OptCatalogEntryDef *const source = hitSource;

    if ((runtimeFlags & 2u) != 0) {
        const float previousLevel = currentLevel;
        const float delta = targetLevel - currentLevel;
        if (fabsf(delta) <= 0.001f) {
            runtimeFlags &= ~2u;
            currentLevel = targetLevel;
        } else {
            float step = source->timedStatusInterpRate * g_FrameDeltaTimeSec;
            if (step > 1.0f) {
                step = 1.0f;
            }

            currentLevel += delta * step;
            if (currentLevel > 1.0f) {
                currentLevel = 1.0f;
            } else if (currentLevel < -1.0f) {
                currentLevel = -1.0f;
            }
        }

        nextUpdateTime = source->timedStatusUpdateDelay + g_Time_AccumulatedTimeSec;

        if (lightNode != 0) {
            const float lightScale = fabsf(hitStatus * currentLevel);
            zClass_Light::gwLightSetRange(lightNode,
                                          source->timedStatusLightRangeMin * lightScale,
                                          source->timedStatusLightRangeMax * lightScale);

            if ((previousLevel > 0.0f && currentLevel < 0.0f) ||
                (previousLevel < 0.0f && currentLevel > 0.0f)) {
                zClass_Light::gwLightSetSpecularColor(lightNode,
                                                       source->timedStatusLightSpecularColor.red,
                                                       source->timedStatusLightSpecularColor.green,
                                                       source->timedStatusLightSpecularColor.blue);
            }
        }
    } else if (g_Time_AccumulatedTimeSec >= nextUpdateTime) {
        const float fadedLevel = zMath::ApproxExpNeg(g_FrameDeltaTimeSec * 0.75f) * currentLevel;
        currentLevel = fadedLevel;
        targetLevel = fadedLevel;

        if (fabsf(fadedLevel) < 0.001f) {
            ClearLightAndReset();
        } else if (lightNode != 0) {
            const float lightScale = fabsf(hitStatus * fadedLevel);
            zClass_Light::gwLightSetRange(lightNode,
                                          source->timedStatusLightRangeMin * lightScale,
                                          source->timedStatusLightRangeMax * lightScale);
        }
    }

    if (currentLevel < -0.5f) {
        return 2;
    }
    if (currentLevel < 0.0f) {
        return 1;
    }
    return 0;
}

namespace HitSource {
// Reimplements 0x4b2210: HitSource::UpdateTimedStatus
// (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL
UpdateTimedStatus(OptCatalogEntryDef *self, PlayerTimedHitStatus *status, float amount) {
    status->hitSource = self;
    status->runtimeFlags |= 3u;

    if ((self->flags & 0x200u) != 0) {
        status->targetLevel -= amount;
    } else {
        status->targetLevel += amount;
    }

    if (status->targetLevel > 1.0f) {
        status->targetLevel = 1.0f;
    } else if (status->targetLevel < -1.0f) {
        status->targetLevel = -1.0f;
    }

    if (status->lightNode == 0) {
        zClass_NodePartial *const light =
            Light::AllocFromFreeListAndAttach(&self->timedStatusLightSpecularColor);
        status->lightNode = light;
        if (light != 0) {
            zClass_Class::AddChild(status->lightParentNode, light);
        }
    }

    if (status->currentLevel < -0.5f) {
        return 2;
    }
    if (status->currentLevel <= 0.0f) {
        return 1;
    }
    return 0;
}
} // namespace HitSource
