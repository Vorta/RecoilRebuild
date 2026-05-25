#include "zWeapon.h"

#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "OptCatalog.h"

extern "C" {
int g_zWeapon_ZarHandlerRegistered = 1;
char g_zWeapon_ArchiveName[8] = "Weapons";
float g_zWeapon_MaxTetherAltitude = 0.0f;
}

namespace {
void *const kOnWeaponsSectionPreLoad = (void *)(0x004b1140);
void *const kOnWeaponsSectionDataReady = (void *)(0x004b1160);
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
        zUtil_ZAR::RegisterSectionHandler(g_zWeapon_ArchiveName, kOnWeaponsSectionPreLoad,
                                          kOnWeaponsSectionDataReady, 0x3e8, 0);
    }

    return 0;
}

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
