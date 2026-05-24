#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zWeapon/zWeapon.h"
#include "OptCatalog.h"

#include <cstdlib>

namespace {
zZbdManager MakeManager(zZbdSectionHandlerNode &sentinel) {
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
    sentinel.sectionHandler = {};

    zZbdManager manager = {};
    manager.sectionHandlerListSentinel = &sentinel;
    return manager;
}

void ClearRegisteredHandlers(zZbdSectionHandlerNode &sentinel) {
    zZbdSectionHandlerNode *node = sentinel.next;
    while (node != &sentinel) {
        zZbdSectionHandlerNode *next = node->next;
        delete node;
        node = next;
    }
    sentinel.next = &sentinel;
    sentinel.prev = &sentinel;
}
} // namespace

extern "C" int zweapon_init_smoke(void) {
    zZbdSectionHandlerNode sentinel = {};
    zZbdManager manager = MakeManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    g_zWeapon_ZarHandlerRegistered = 1;

    g_OptCatalog_FallbackImpactProbeEnabled = 0;
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;
    g_OptCatalog_EntryCount = 9;
    g_OptCatalog_EntryTable = reinterpret_cast<OptCatalogEntryDef *>(0x1111);
    g_OptCatalogRuntimeInstanceCount = 8;
    g_OptCatalogRuntimeInstancePool = reinterpret_cast<void *>(0x2222);
    g_OptCatalogFreeRuntimeInstanceList = reinterpret_cast<void *>(0x3333);
    g_OptCatalogRuntimeWorld = reinterpret_cast<zClass_NodePartial *>(0x4444);
    g_OptCatalogPendingSpawnTargetCountPtr = reinterpret_cast<void *>(0x5555);
    g_OptCatalogPendingSpawnTargetListPtr = reinterpret_cast<void *>(0x6666);
    g_OptCatalogQueuedImpactCount = 7;
    g_OptCatalog_DamageContextKind = 6;
    g_OptCatalog_DamageContextHitEvent = reinterpret_cast<void *>(0x7777);
    g_OptCatalogDamageFeedbackCallback = reinterpret_cast<void *>(0x8888);
    g_OptCatalog_DamageFeedbackHitCount = 5;
    g_OptCatalogDamageFeedbackTrackedNode = reinterpret_cast<zClass_NodePartial *>(0x9999);

    if (zWepInit() != 0) {
        return 1;
    }

    const bool resetOk =
        g_OptCatalog_FallbackImpactProbeEnabled == 1 &&
        g_OptCatalog_CaptureHitSnapshotEnabled == 1 && g_OptCatalog_EntryCount == 0 &&
        g_OptCatalog_EntryTable == nullptr && g_OptCatalogRuntimeInstanceCount == 0 &&
        g_OptCatalogRuntimeInstancePool == nullptr &&
        g_OptCatalogFreeRuntimeInstanceList == nullptr && g_OptCatalogRuntimeWorld == nullptr &&
        g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
        g_OptCatalogPendingSpawnTargetListPtr == nullptr && g_OptCatalogMaxCraterRadius == 30.0f &&
        g_OptCatalogQueuedImpactCount == 0 && g_OptCatalog_DamageContextKind == 0 &&
        g_OptCatalog_DamageContextHitEvent == nullptr && g_zWeapon_MaxTetherAltitude == 30.0f &&
        g_OptCatalogDamageFeedbackCallback == nullptr &&
        g_OptCatalogLockOnWarningGateTimeSec == 0.0f && g_OptCatalog_DamageFeedbackHitCount == 0 &&
        g_OptCatalogDamageFeedbackTrackedNode == nullptr && g_OptCatalogNextSpawnScale == 1.0f;

    const bool registerOk =
        manager.sectionHandlerCount == 1 && sentinel.next != &sentinel &&
        sentinel.next->sectionHandler.sectionName == g_zWeapon_ArchiveName &&
        sentinel.next->sectionHandler.onPreLoad == reinterpret_cast<void *>(0x004b1140) &&
        sentinel.next->sectionHandler.onDataReady == reinterpret_cast<void *>(0x004b1160) &&
        sentinel.next->sectionHandler.sortOrder == 0x3e8 &&
        sentinel.next->sectionHandler.userData == nullptr;

    ClearRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    if (!resetOk || !registerOk) {
        return 2;
    }

    g_zWeapon_ZarHandlerRegistered = 0;
    manager = MakeManager(sentinel);
    g_zUtil_ZbdManager = &manager;
    zWepInit();
    const bool skipOk = manager.sectionHandlerCount == 0;
    ClearRegisteredHandlers(sentinel);
    g_zUtil_ZbdManager = nullptr;
    g_zWeapon_ZarHandlerRegistered = 1;
    return skipOk ? 0 : 3;
}

extern "C" int zweapon_optcatalog_shutdown_smoke(void) {
    g_OptCatalog_EntryCount = 1;
    g_OptCatalog_EntryTable =
        static_cast<OptCatalogEntryDef *>(std::calloc(1, sizeof(OptCatalogEntryDef)));
    g_OptCatalog_EntryTable[0].description = static_cast<char *>(std::malloc(4));
    g_OptCatalog_EntryTable[0].militaryName = static_cast<char *>(std::malloc(4));
    g_OptCatalog_EntryTable[0].impactFxTable = std::malloc(4);
    g_OptCatalog_EntryTable[0].killVerbString = static_cast<char *>(std::malloc(4));
    g_OptCatalogRuntimeInstanceCount = 3;
    g_OptCatalogRuntimeInstancePool = std::malloc(4);
    g_OptCatalogFreeRuntimeInstanceList = reinterpret_cast<void *>(0x1234);
    g_OptCatalogRuntimeWorld = reinterpret_cast<zClass_NodePartial *>(0x5678);
    g_OptCatalogPendingSpawnTargetCountPtr = reinterpret_cast<void *>(0x9abc);
    g_OptCatalogPendingSpawnTargetListPtr = reinterpret_cast<void *>(0xdef0);
    g_OptCatalog_FallbackImpactProbeEnabled = 0;
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;
    g_OptCatalogQueuedImpactCount = 9;
    g_OptCatalog_DamageFeedbackHitCount = 8;
    g_OptCatalogThermalGlowFreeList = nullptr;
    g_OptCatalogLoadedTreeRoot = nullptr;

    if (OptCatalog::Shutdown() != 0) {
        return 1;
    }

    return g_OptCatalog_EntryCount == 0 && g_OptCatalog_EntryTable == nullptr &&
                   g_OptCatalogRuntimeInstanceCount == 0 &&
                   g_OptCatalogRuntimeInstancePool == nullptr &&
                   g_OptCatalogFreeRuntimeInstanceList == nullptr &&
                   g_OptCatalogRuntimeWorld == nullptr &&
                   g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
                   g_OptCatalogPendingSpawnTargetListPtr == nullptr &&
                   g_OptCatalog_FallbackImpactProbeEnabled == 1 &&
                   g_OptCatalog_CaptureHitSnapshotEnabled == 1 &&
                   g_OptCatalogQueuedImpactCount == 0 && g_OptCatalog_DamageFeedbackHitCount == 0
               ? 0
               : 2;
}

extern "C" int zweapon_optcatalog_find_entry_by_id_smoke(void) {
    const std::int32_t oldCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldTable = g_OptCatalog_EntryTable;

    OptCatalogEntryDef entries[3] = {};
    entries[0].keyName = const_cast<char *>("cannon");
    entries[0].ordinalIndex = 11;
    entries[1].keyName = nullptr;
    entries[1].ordinalIndex = 12;
    entries[2].keyName = const_cast<char *>("rocket");
    entries[2].ordinalIndex = 13;

    g_OptCatalog_EntryTable = entries;
    g_OptCatalog_EntryCount = 3;

    const bool found = OptCatalog::FindEntryById(13) == &entries[2];
    const bool nullKeySkipped = OptCatalog::FindEntryById(12) == nullptr;
    const bool missing = OptCatalog::FindEntryById(99) == nullptr;

    g_OptCatalog_EntryCount = 0;
    const bool empty = OptCatalog::FindEntryById(11) == nullptr;

    g_OptCatalog_EntryCount = oldCount;
    g_OptCatalog_EntryTable = oldTable;

    return found && nullKeySkipped && missing && empty ? 0 : 1;
}

extern "C" int zweapon_optcatalog_pending_spawn_override_smoke(void) {
    std::int32_t pendingCount = 3;
    void *pendingList = &pendingCount;

    OptCatalog::SetPendingSpawnTargetOverrides(&pendingCount, &pendingList);
    const bool setOk = g_OptCatalogPendingSpawnTargetCountPtr == &pendingCount &&
                       g_OptCatalogPendingSpawnTargetListPtr == &pendingList;

    OptCatalog::SetPendingSpawnTargetOverrides(nullptr, nullptr);
    const bool resetOk = g_OptCatalogPendingSpawnTargetCountPtr == nullptr &&
                         g_OptCatalogPendingSpawnTargetListPtr == nullptr;

    return setOk && resetOk ? 0 : 1;
}

extern "C" int zweapon_optcatalog_runtime_free_list_helpers_smoke(void) {
    void *const oldFreeRuntimeList = g_OptCatalogFreeRuntimeInstanceList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    const float oldNextSpawnScale = g_OptCatalogNextSpawnScale;

    OptCatalogEntryDef entry = {};
    zClass_NodePartial cloneA = {};
    zClass_NodePartial cloneB = {};
    cloneA.callbackContext = &cloneB;
    entry.attachCloneChildFreeList = &cloneA;

    zClass_NodePartial *const reusedClone = OptCatalog::AllocOrReuseAttachNodeChildClone(&entry);
    if (reusedClone != &cloneA || entry.attachCloneChildFreeList != &cloneB ||
        cloneA.callbackContext != nullptr) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 1;
    }

    OptCatalogRuntimeInstanceStorage runtimeA = {};
    OptCatalogRuntimeInstanceStorage runtimeB = {};
    zClass_NodePartial projectileA = {};
    zClass_Object3DDataPartial projectileDataA = {};
    projectileA.classId = 5;
    projectileA.classData = &projectileDataA;
    zClass_NodePartial attachNode = {};
    runtimeA.next = &runtimeB;
    runtimeA.projectileNode = &projectileA;
    runtimeA.lifetime = 9.0f;
    runtimeA.updateCallback = &entry;
    entry.attachCloneTemplateNode = &attachNode;
    entry.flyoutModelAnimationEntry = nullptr;
    g_OptCatalogFreeRuntimeInstanceList = &runtimeA;

    OptCatalogRuntimeInstanceStorage *const directRuntime =
        OptCatalog::AllocOrReuseAttachNodeClone(&entry);
    const bool directAttachOk = directRuntime == &runtimeA &&
                                g_OptCatalogFreeRuntimeInstanceList == &runtimeB &&
                                runtimeA.attachCloneChild == nullptr && runtimeA.lifetime == 0.0f &&
                                runtimeA.updateCallback == nullptr && projectileA.listCountB == 1 &&
                                projectileA.listB[0] == &attachNode && attachNode.listCountA == 1 &&
                                attachNode.listA[0] == &projectileA;
    zClass_Object3D::RemoveChild(&projectileA, &attachNode);
    if (!directAttachOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 2;
    }

    zClass_NodePartial projectileB = {};
    zClass_Object3DDataPartial projectileDataB = {};
    projectileB.classId = 5;
    projectileB.classData = &projectileDataB;
    runtimeB.next = nullptr;
    runtimeB.projectileNode = &projectileB;
    runtimeB.attachCloneChild = nullptr;
    runtimeB.lifetime = 4.0f;
    runtimeB.updateCallback = &runtimeA;
    cloneA.callbackContext = &cloneB;
    entry.attachCloneChildFreeList = &cloneA;
    entry.flyoutModelAnimationEntry = reinterpret_cast<zEffectAnimEntry *>(&entry);
    g_OptCatalogFreeRuntimeInstanceList = &runtimeB;

    OptCatalogRuntimeInstanceStorage *const clonedRuntime =
        OptCatalog::AllocOrReuseAttachNodeClone(&entry);
    const bool clonedAttachOk =
        clonedRuntime == &runtimeB && g_OptCatalogFreeRuntimeInstanceList == nullptr &&
        runtimeB.attachCloneChild == &cloneA && entry.attachCloneChildFreeList == &cloneB &&
        cloneA.callbackContext == nullptr && projectileB.listCountB == 1 &&
        projectileB.listB[0] == &cloneA && cloneA.listCountA == 1 &&
        cloneA.listA[0] == &projectileB && runtimeB.lifetime == 0.0f &&
        runtimeB.updateCallback == nullptr;
    zClass_Object3D::RemoveChild(&projectileB, &cloneA);
    if (!clonedAttachOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 3;
    }

    runtimeB.asyncFxHandle = reinterpret_cast<zEffectAnimEntry *>(&runtimeA);
    OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback(nullptr, &runtimeB, nullptr);
    const bool clearCallbackOk = runtimeB.asyncFxHandle == nullptr;
    if (!clearCallbackOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 4;
    }

    OptCatalogRuntimeInstanceStorage runtimeC = {};
    zClass_NodeFreeListSlot projectileSlot = {};
    zClass_Object3DDataPartial projectileDataC = {};
    projectileSlot.node.classId = 5;
    projectileSlot.node.classData = &projectileDataC;
    runtimeC.projectileNode = &projectileSlot.node;
    runtimeC.lifetime = 3.0f;
    runtimeC.next = nullptr;
    g_OptCatalogFreeRuntimeInstanceList = &runtimeC;

    zClass_NodePartial runtimeWorld = {};
    runtimeWorld.classId = 3;
    g_OptCatalogRuntimeWorld = &runtimeWorld;
    g_OptCatalogNextSpawnScale = 2.5f;
    entry.attachCloneTemplateNode = nullptr;
    entry.flyoutModelAnimationEntry = nullptr;
    entry.activeRuntimeListHead = &runtimeA;
    entry.flyoutHealth = 7.5f;
    zClass_NodePartial ownerNode = {};
    zVec3 spawnPos = {11.0f, 12.0f, 13.0f};

    OptCatalogRuntimeInstanceStorage *const spawnedRuntime =
        OptCatalog::SpawnRuntimeInstanceAt(&entry, &spawnPos, &ownerNode);
    const bool spawnOk =
        spawnedRuntime == &runtimeC && entry.activeRuntimeListHead == &runtimeC &&
        runtimeC.next == &runtimeA && runtimeC.pos.x == 11.0f && runtimeC.pos.y == 12.0f &&
        runtimeC.pos.z == 13.0f && runtimeC.lifetime == 0.0f && runtimeC.ownerNode == &ownerNode &&
        runtimeC.spawnScale == 2.5f && g_OptCatalogNextSpawnScale == 1.0f &&
        (projectileSlot.node.flags & 0x08000000) != 0 &&
        projectileSlot.damageHandler == reinterpret_cast<void *>(1) &&
        projectileSlot.node.callbackContext == reinterpret_cast<zClass_NodePartial *>(&runtimeC) &&
        runtimeC.projectileScale == 7.5f && projectileDataC.localMatrix[9] == 11.0f &&
        projectileDataC.localMatrix[10] == 12.0f && projectileDataC.localMatrix[11] == 13.0f &&
        runtimeWorld.listCountB == 1 && runtimeWorld.listB[0] == &projectileSlot.node;
    zClass_Class::RemoveChild(&runtimeWorld, &projectileSlot.node);
    if (!spawnOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 5;
    }

    g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    g_OptCatalogNextSpawnScale = oldNextSpawnScale;
    return 0;
}

extern "C" int light_alloc_from_free_list_and_attach_smoke(void) {
    zClass_NodePartial *const oldThermalGlowFreeList = g_OptCatalogThermalGlowFreeList;
    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;

    g_OptCatalogThermalGlowFreeList = nullptr;
    zColorRgb color = {0.25f, 0.5f, 0.75f};
    if (Light::AllocFromFreeListAndAttach(&color) != nullptr) {
        g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        return 1;
    }

    zClass_NodePartial light = {};
    zClass_NodePartial nextLight = {};
    zClass_LightDataPartial lightData = {};
    light.classData = &lightData;
    light.callbackContext = &nextLight;

    zClass_WorldDataPartial worldData = {};
    zClass_NodePartial world = {};
    world.classData = &worldData;

    g_OptCatalogThermalGlowFreeList = &light;
    g_OptCatalogRuntimeWorld = &world;

    zClass_NodePartial *const allocated = Light::AllocFromFreeListAndAttach(&color);
    const bool ok = allocated == &light && g_OptCatalogThermalGlowFreeList == &nextLight &&
                    lightData.range1 == 0.1f && lightData.range2 == 0.2f &&
                    lightData.specularColor.red == 0.25f && lightData.specularColor.green == 0.5f &&
                    lightData.specularColor.blue == 0.75f && worldData.lightCount == 1 &&
                    worldData.lightNodes[0] == &light && worldData.lightDataList[0] == &lightData &&
                    lightData.attachedWorldCount == 1 && lightData.attachedWorlds[0] == &world;

    Light::ReturnToFreeList(&light);
    const bool returnedOk = g_OptCatalogThermalGlowFreeList == &light &&
                            light.callbackContext == &nextLight && worldData.lightCount == 0 &&
                            lightData.attachedWorldCount == 0 && lightData.range1 == 0.1f &&
                            lightData.range2 == 0.2f;

    std::free(worldData.lightNodes);
    std::free(worldData.lightDataList);
    std::free(lightData.attachedWorlds);
    g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    return ok && returnedOk ? 0 : 2;
}

extern "C" int zweapon_optcatalog_warning_samples_smoke(void) {
    zSndSample trigger = {};
    zSndSample weapon = {};
    zSndSample noAmmo = {};

    g_OptCatalogSndTriggerInactive = &trigger;
    g_OptCatalogSndWeaponInactive = &weapon;
    g_OptCatalogSndNoAmmoWarning = &noAmmo;
    g_zSnd_IsInitialized = 0;
    g_zSnd_PreInitialized = 0;

    OptCatalog::PlayTriggerInactiveWarning();
    OptCatalog::PlayWeaponInactiveWarning();
    OptCatalog::PlayNoAmmoWarning();

    const bool samplesKept = g_OptCatalogSndTriggerInactive == &trigger &&
                             g_OptCatalogSndWeaponInactive == &weapon &&
                             g_OptCatalogSndNoAmmoWarning == &noAmmo;

    g_OptCatalogSndTriggerInactive = nullptr;
    g_OptCatalogSndWeaponInactive = nullptr;
    g_OptCatalogSndNoAmmoWarning = nullptr;

    return samplesKept ? 0 : 1;
}
