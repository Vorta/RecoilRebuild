#include "zWeapon.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "OptCatalog.h"

#include <math.h>

extern "C" {
/**
 * Reimplements data 0x4e42ec: g_zWeapon_ZarHandlerRegistered.
 * BN xrefs: zWepInit gates Weapons ZAR section callback registration.
 * Purpose: one-time startup flag controlling whether zWeapon registers the
 * Weapons archive callbacks during initialization.
 */
int g_zWeapon_ZarHandlerRegistered = 1;
/**
 * Reimplements data 0x4e42f0: g_zWeapon_ArchiveName.
 * BN xrefs: zWepInit passes this string to zUtil_ZAR::RegisterSectionHandler.
 * Purpose: archive section name used when registering zWeapon save callbacks.
 */
char g_zWeapon_ArchiveName[8] = "Weapons";
/**
 * Reimplements data 0x779a98: g_zWeapon_MaxTetherAltitude.
 * BN xrefs: zWepInit restores the startup default and tether checks consume
 * the configured altitude cap.
 * Purpose: runtime maximum tether altitude loaded from weapon configuration.
 */
float g_zWeapon_MaxTetherAltitude = 0.0f;
}

namespace {
template <typename T>
/**
 * Original helper evidence: no standalone retail function; observed in
 * caller 0x4b1090.
 *
 * Purpose: preserve the typed callback declaration at each registration site
 * while passing the raw ZAR section-callback pointer expected by zUtil_ZAR.
 */
zZbdSectionCallback ZbdCallbackPtr(
    T callback
) {
    RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T typed;
        zZbdSectionCallback raw;
    } ptr = {callback};
    return ptr.raw;
}
} // namespace

/**
 * Reimplements 0x4b1090: zWepInit.
 *
 * Purpose: reset weapon and OptCatalog runtime globals, restore weapon
 * defaults, and optionally register the Weapons ZAR section callbacks.
 */
extern "C" int zWepInit() {
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
            g_zWeapon_ArchiveName,
            ZbdCallbackPtr(&zWeapon::OnWeaponsSectionPreLoad),
            ZbdCallbackPtr(&zWeapon::OnWeaponsSectionDataReady),
            0x3e8,
            0
        );
    }

    return 0;
}

namespace zWeapon {
/**
 * Reimplements 0x4b1140: zWeapon::OnWeaponsSectionPreLoad
 * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
 *
 * Purpose: write the current weapon damage-feedback hit count into the
 * WeaponData section blob before the Weapons archive section is saved.
 */
int __fastcall OnWeaponsSectionPreLoad(
    zZbdSectionCallbackCtx *callbackCtx,
    void *
) {
    int weaponDataHitCount = g_OptCatalog_DamageFeedbackHitCount;
    return zUtil_ZAR::WriteSectionBlob(
        callbackCtx,
        "WeaponData",
        &weaponDataHitCount,
        sizeof(weaponDataHitCount)
    );
}

/**
 * Reimplements 0x4b1160: zWeapon::OnWeaponsSectionDataReady
 * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
 *
 * Purpose: restore the weapon damage-feedback hit count from the WeaponData
 * section blob and reset the lock-on warning gate.
 */
void __fastcall OnWeaponsSectionDataReady(
    zZbdSectionCallbackCtx *,
    const char *,
    void *weaponData,
    unsigned int,
    void *
) {
    g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
    g_OptCatalog_DamageFeedbackHitCount = *(int *)(weaponData);
}

/**
 * Reimplements 0x4b1d80: zWeapon::SetMaxTetherAltitude
 * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
 *
 * Purpose: store the maximum tether altitude used by weapon script commands.
 */
void __stdcall SetMaxTetherAltitude(
    float altitude
) {
    g_zWeapon_MaxTetherAltitude = altitude;
}
} // namespace zWeapon

RECOIL_STATIC_ASSERT(sizeof(PlayerTimedHitStatus) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, runtimeFlags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, hitSource) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, currentLevel) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, targetLevel) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, lightNode) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, nextUpdateTime) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, lightParentNode) == 0x18);

/**
 * Reimplements 0x4b21c0: PlayerTimedHitStatus::ResetFields
 * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
 *
 * Purpose: clear the active and interpolation flags and reset the timed-hit
 * light, level, and update timer fields.
 */
void PlayerTimedHitStatus::ResetFields() {
    runtimeFlags &= ~3u;
    lightNode = 0;
    currentLevel = 0.0f;
    targetLevel = 0.0f;
    nextUpdateTime = 0.0f;
}

/**
 * Reimplements 0x4b22d0: PlayerTimedHitStatus::ClearLightAndReset
 * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
 *
 * Purpose: detach and recycle the active timed-hit light, then reset the
 * status fields.
 */
void PlayerTimedHitStatus::ClearLightAndReset() {
    if (lightNode != 0) {
        zClass_Class::RemoveChild(
            lightParentNode,
            lightNode
        );
        Light::ReturnToFreeList(lightNode);
        ResetFields();
    }
}

/**
 * Reimplements 0x4b2300: PlayerTimedHitStatus::TickAndUpdateLight
 * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
 *
 * Purpose: advance timed-hit interpolation or decay, update the status light,
 * and return the current damage band.
 */
int PlayerTimedHitStatus::TickAndUpdateLight(
    float hitStatus
) {
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
            zClass_Light::gwLightSetRange(
                lightNode,
                source->timedStatusLightRangeMin * lightScale,
                source->timedStatusLightRangeMax * lightScale
            );

            if ((previousLevel > 0.0f && currentLevel < 0.0f) ||
                (previousLevel < 0.0f && currentLevel > 0.0f)) {
                zClass_Light::gwLightSetSpecularColor(
                    lightNode,
                    source->timedStatusLightSpecularColor.red,
                    source->timedStatusLightSpecularColor.green,
                    source->timedStatusLightSpecularColor.blue
                );
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
            zClass_Light::gwLightSetRange(
                lightNode,
                source->timedStatusLightRangeMin * lightScale,
                source->timedStatusLightRangeMax * lightScale
            );
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
/**
 * Reimplements 0x4b2210: HitSource::UpdateTimedStatus
 * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
 *
 * Purpose: apply a hit source's timed-status contribution, allocate its
 * status light when needed, and report the current damage band.
 */
int __fastcall UpdateTimedStatus(
    OptCatalogEntryDef *self,
    PlayerTimedHitStatus *status,
    float amount
) {
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
            zClass_Class::AddChild(
                status->lightParentNode,
                light
            );
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
