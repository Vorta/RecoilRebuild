#include "zWeapon.h"

#include "GameZRecoil/zUtil/zZbd.h"
#include "OptCatalog.h"

extern "C" {
int g_zWeapon_ZarHandlerRegistered = 1;
char g_zWeapon_ArchiveName[8] = "Weapons";
float g_zWeapon_MaxTetherAltitude = 0.0f;
}

namespace {
void *const kOnWeaponsSectionPreLoad = reinterpret_cast<void *>(0x004b1140);
void *const kOnWeaponsSectionDataReady = reinterpret_cast<void *>(0x004b1160);
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
