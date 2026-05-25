#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zWeapon/zWeapon.h"
#include "OptCatalog.h"

#include <cstdlib>
#include <cstring>

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

int g_TestDamageTimerCalls;
void *g_TestDamageTimerContext;
float g_TestDamageTimerDamage;

float RECOIL_FASTCALL TestDamageTimerCallback(void *context, float damageAmount) {
    ++g_TestDamageTimerCalls;
    g_TestDamageTimerContext = context;
    g_TestDamageTimerDamage = damageAmount;
    return damageAmount + *static_cast<float *>(context);
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

extern "C" int zweapon_optcatalog_find_entry_by_name_smoke(void) {
    const std::int32_t oldCount = g_OptCatalog_EntryCount;
    OptCatalogEntryDef *const oldTable = g_OptCatalog_EntryTable;

    OptCatalogEntryDef entries[4] = {};
    entries[0].keyName = const_cast<char *>("cannon");
    entries[1].keyName = nullptr;
    entries[2].keyName = const_cast<char *>("rocket");
    entries[3].keyName = const_cast<char *>("rocket");

    g_OptCatalog_EntryTable = entries;
    g_OptCatalog_EntryCount = 4;

    const bool foundFirst = OptCatalog::FindEntryByName("rocket") == &entries[2];
    const bool nullKeySkipped = OptCatalog::FindEntryByName("cannon") == &entries[0];
    const bool missing = OptCatalog::FindEntryByName("mine") == nullptr;

    g_OptCatalog_EntryCount = 0;
    const bool empty = OptCatalog::FindEntryByName("cannon") == nullptr;

    g_OptCatalog_EntryCount = oldCount;
    g_OptCatalog_EntryTable = oldTable;

    return foundFirst && nullKeySkipped && missing && empty ? 0 : 1;
}

extern "C" int zweapon_optcatalog_create_trail_segment_node_smoke(void) {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[2] = {};
    slots[0].freeTag = 1;
    slots[1].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial templateNode = {};
    zClass_NodePartial *const parent =
        OptCatalog::CreateTrailSegmentNodeFromTemplate(&templateNode);

    const bool attachedOk = parent == &slots[0].node && parent->classId == 5 &&
                            (parent->flags & 0x04) != 0 && parent->listCountB == 1 &&
                            parent->listB[0] == &templateNode && templateNode.listCountA == 1 &&
                            templateNode.listA[0] == parent;

    zClass_Object3D::DeleteNode(parent);
    zClass_TypeList::FreeAll();

    slots[0].freeTag = 0x00ffffff;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    zClass_NodePartial *const noTemplate =
        OptCatalog::CreateTrailSegmentNodeFromTemplate(nullptr);
    const bool noTemplateOk = noTemplate == &slots[0].node && noTemplate->classId == 5 &&
                              (noTemplate->flags & 0x04) != 0 &&
                              noTemplate->listCountB == 0;

    zClass_Object3D::DeleteNode(noTemplate);
    zClass_TypeList::FreeAll();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    return attachedOk && noTemplateOk ? 0 : 1;
}

extern "C" int zweapon_optcatalog_create_trail_runtime_state_smoke(void) {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[10] = {};
    for (int i = 0; i < 9; ++i) {
        slots[i].freeTag = i + 1;
    }
    slots[9].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial runtimeWorld = {};
    runtimeWorld.classId = 3;
    zClass_NodePartial templateNode = {};
    zClass_NodePartial projectileNode = {};
    zTag4Partial variantTag = {};
    zVec3 spawnPos = {1.0f, 2.0f, 3.0f};
    zVec3 spawnDir = {4.0f, 5.0f, 6.0f};
    OptCatalogEntryDef entry = {};
    entry.attachCloneTemplateNode = &templateNode;

    zClass_NodePartial *const oldRuntimeWorld = g_OptCatalogRuntimeWorld;
    g_OptCatalogRuntimeWorld = &runtimeWorld;

    OptCatalogTrailRuntimeState *const runtime = OptCatalog::CreateTrailRuntimeState(
        &entry, &projectileNode, &variantTag, nullptr, &spawnPos, &spawnDir, 10);
    const bool clampedOk =
        runtime != nullptr && runtime->ownerEntry == &entry &&
        runtime->projectileNode == &projectileNode && runtime->variantTagPtr == &variantTag &&
        runtime->spawnPos == &spawnPos && runtime->spawnDir == &spawnDir &&
        runtime->activeNodeSlotCount == 8 && runtime->activeNodeSlots[0].node != nullptr &&
        runtime->activeNodeSlots[7].node != nullptr &&
        std::strcmp(runtime->activeNodeSlots[0].node->name, "BeamReflect_0") == 0 &&
        std::strcmp(runtime->activeNodeSlots[7].node->name, "BeamReflect_7") == 0 &&
        (runtime->activeNodeSlots[0].node->flags & 0x04) == 0 &&
        runtimeWorld.listCountB == 8 && templateNode.listCountA == 8;

    std::free(runtime);
    std::free(runtimeWorld.listB);
    std::free(templateNode.listA);
    runtimeWorld = {};
    runtimeWorld.classId = 3;
    templateNode = {};

    entry.flags = 0x800;
    OptCatalogTrailRuntimeState *const single = OptCatalog::CreateTrailRuntimeState(
        &entry, &projectileNode, &variantTag, nullptr, &spawnPos, &spawnDir, 4);
    const bool singleSegmentOk =
        single != nullptr && single->activeNodeSlotCount == 1 &&
        single->activeNodeSlots[0].node != nullptr && runtimeWorld.listCountB == 1 &&
        std::strcmp(single->activeNodeSlots[0].node->name, "BeamReflect_0") == 0;

    std::free(single);
    std::free(runtimeWorld.listB);
    std::free(templateNode.listA);
    zClass_TypeList::FreeAll();
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeFreeHeadIndex = -1;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    if (!clampedOk) {
        return 1;
    }
    return singleSegmentOk ? 0 : 2;
}

extern "C" int zweapon_optcatalog_mine_iterator_smoke(void) {
    OptCatalogRuntimeInstanceStorage first = {};
    OptCatalogRuntimeInstanceStorage second = {};
    OptCatalogEntryDef entry = {};
    first.next = &second;
    entry.activeRuntimeListHead = &first;

    OptCatalogRuntimeInstanceStorage *const oldCursor = g_OptCatalog_MineIteratorCursor;
    g_OptCatalog_MineIteratorCursor = nullptr;

    const bool beginOk =
        OptCatalog::MineIterator_Begin(&entry) == &first &&
        g_OptCatalog_MineIteratorCursor == &first;
    const bool nextOk =
        OptCatalog::MineIterator_Next() == &second &&
        g_OptCatalog_MineIteratorCursor == &second;
    const bool endOk =
        OptCatalog::MineIterator_Next() == nullptr &&
        g_OptCatalog_MineIteratorCursor == nullptr &&
        OptCatalog::MineIterator_Next() == nullptr;

    g_OptCatalog_MineIteratorCursor = oldCursor;
    return beginOk && nextOk && endOk ? 0 : 1;
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

extern "C" int hitcontext_get_current_owner_smoke(void) {
    void *const oldOwnerOrCtx = g_OptCatalog_CurrentDamageOwnerOrCtx;
    int ownerSentinel = 0;

    g_OptCatalog_CurrentDamageOwnerOrCtx = &ownerSentinel;
    const bool ownerReturned = HitContext::GetCurrentOwnerOrCtx() == &ownerSentinel;

    g_OptCatalog_CurrentDamageOwnerOrCtx = nullptr;
    const bool nullReturned = HitContext::GetCurrentOwnerOrCtx() == nullptr;

    g_OptCatalog_CurrentDamageOwnerOrCtx = oldOwnerOrCtx;
    return ownerReturned && nullReturned ? 0 : 1;
}

extern "C" int optcatalog_set_damage_context_smoke(void) {
    const int oldContextKind = g_OptCatalog_DamageContextKind;
    void *const oldContextHitEvent = g_OptCatalog_DamageContextHitEvent;

    OptCatalogHitEventPartial hitEvent = {};
    zClass_NodePartial hitNode = {};
    int previous = 0;
    g_OptCatalog_DamageContextKind = 11;
    g_OptCatalog_DamageContextHitEvent = &previous;

    OptCatalog::SetDamageContext(3, nullptr);
    const bool nullEventDoesNotCapture =
        g_OptCatalog_DamageContextKind == 3 && g_OptCatalog_DamageContextHitEvent == &previous;

    OptCatalog::SetDamageContext(4, &hitEvent);
    const bool nullHitNodeDoesNotCapture =
        g_OptCatalog_DamageContextKind == 4 && g_OptCatalog_DamageContextHitEvent == &previous;

    hitEvent.hitNode = &hitNode;
    OptCatalog::SetDamageContext(5, &hitEvent);
    const bool hitNodeCaptures =
        g_OptCatalog_DamageContextKind == 5 && g_OptCatalog_DamageContextHitEvent == &hitEvent;

    g_OptCatalog_DamageContextKind = oldContextKind;
    g_OptCatalog_DamageContextHitEvent = oldContextHitEvent;
    return nullEventDoesNotCapture && nullHitNodeDoesNotCapture && hitNodeCaptures ? 0 : 1;
}

extern "C" int optcatalog_damage_feedback_leaf_helpers_smoke(void) {
    const float oldScalar = g_OptCatalogDamageFeedbackIntensityScalar;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;

    DamageFeedback::SetIntensityScalar(2.25f);
    const bool scalarOk = g_OptCatalogDamageFeedbackIntensityScalar == 2.25f;

    g_OptCatalog_CapturedDamageSourcePos = {1.0f, 2.0f, 3.0f};
    g_OptCatalog_CapturedDamageHitPos = {4.0f, 5.0f, 6.0f};
    zVec3 *const captured = OptCatalog::GetCapturedHitSourcePtr();
    const bool captureOk = captured == &g_OptCatalog_CapturedDamageSourcePos &&
                           captured[0].x == 1.0f && captured[0].y == 2.0f &&
                           captured[0].z == 3.0f && captured[1].x == 4.0f &&
                           captured[1].y == 5.0f && captured[1].z == 6.0f;

    g_OptCatalogDamageFeedbackIntensityScalar = oldScalar;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    return scalarOk && captureOk ? 0 : 1;
}

extern "C" int optcatalog_capture_hit_snapshot_and_invoke_damage_timer_callback_smoke(void) {
    const int oldCaptureEnabled = g_OptCatalog_CaptureHitSnapshotEnabled;
    const zVec3 oldSourcePos = g_OptCatalog_CapturedDamageSourcePos;
    const zVec3 oldHitPos = g_OptCatalog_CapturedDamageHitPos;

    zClass_NodeFreeListSlot hitNodeSlot = {};
    OptCatalogDamageHandlerPartial handler = {};
    OptCatalogHitEventPartial hitEvent = {};
    zVec3 sourcePos = {7.0f, 8.0f, 9.0f};
    float contextBonus = 4.0f;
    hitNodeSlot.damageHandler = &handler;
    handler.timerCallback = (void *)TestDamageTimerCallback;
    handler.timerContext = &contextBonus;
    hitEvent.hitNode = &hitNodeSlot.node;
    hitEvent.hitPos = {1.0f, 2.0f, 3.0f};
    g_TestDamageTimerCalls = 0;
    g_TestDamageTimerContext = nullptr;
    g_TestDamageTimerDamage = 0.0f;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;

    const float result =
        OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(&sourcePos, &hitEvent, 2.5f);
    const bool callbackOk = result == 6.5f && g_TestDamageTimerCalls == 1 &&
                            g_TestDamageTimerContext == &contextBonus &&
                            g_TestDamageTimerDamage == 2.5f;
    const bool captureOk =
        g_OptCatalog_CapturedDamageSourcePos.x == 7.0f &&
        g_OptCatalog_CapturedDamageSourcePos.y == 8.0f &&
        g_OptCatalog_CapturedDamageSourcePos.z == 9.0f &&
        g_OptCatalog_CapturedDamageHitPos.x == 1.0f &&
        g_OptCatalog_CapturedDamageHitPos.y == 2.0f &&
        g_OptCatalog_CapturedDamageHitPos.z == 3.0f;

    g_OptCatalog_CapturedDamageSourcePos = {11.0f, 12.0f, 13.0f};
    g_OptCatalog_CapturedDamageHitPos = {14.0f, 15.0f, 16.0f};
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;
    OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(&sourcePos, &hitEvent, 1.0f);
    const bool disabledCaptureOk =
        g_OptCatalog_CapturedDamageSourcePos.x == 11.0f &&
        g_OptCatalog_CapturedDamageSourcePos.y == 12.0f &&
        g_OptCatalog_CapturedDamageSourcePos.z == 13.0f &&
        g_OptCatalog_CapturedDamageHitPos.x == 14.0f &&
        g_OptCatalog_CapturedDamageHitPos.y == 15.0f &&
        g_OptCatalog_CapturedDamageHitPos.z == 16.0f;

    g_OptCatalog_CaptureHitSnapshotEnabled = oldCaptureEnabled;
    g_OptCatalog_CapturedDamageSourcePos = oldSourcePos;
    g_OptCatalog_CapturedDamageHitPos = oldHitPos;
    return callbackOk && captureOk && disabledCaptureOk ? 0 : 1;
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
    if (!clonedAttachOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 3;
    }

    zClass_NodePartial asyncRuntimeNode = {};
    zEffectAnimEntry asyncEffect = {};
    asyncEffect.activationState = 2;
    asyncEffect.runtimeNode = &asyncRuntimeNode;
    asyncEffect.triggerBaseValue = 1.0f;
    asyncEffect.triggerCurrentValue = 8.0f;
    runtimeB.asyncFxHandle = &asyncEffect;
    OptCatalog::RecycleAttachNodeClone(&entry, &runtimeB);
    if (runtimeB.attachCloneChild != nullptr) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 40;
    }
    if (entry.attachCloneChildFreeList != &cloneA || cloneA.callbackContext != &cloneB) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 41;
    }
    if (projectileB.listCountB != 0 || cloneA.listCountA != 0) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 42;
    }
    if (asyncEffect.triggerCurrentValue != 0.0f) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 43;
    }

    runtimeB.asyncFxHandle = reinterpret_cast<zEffectAnimEntry *>(&runtimeA);
    OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback(nullptr, &runtimeB, nullptr);
    const bool clearCallbackOk = runtimeB.asyncFxHandle == nullptr;
    if (!clearCallbackOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 5;
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
        return 6;
    }

    OptCatalogRuntimeInstanceStorage freeSentinel = {};
    OptCatalogRuntimeInstanceStorage runtimeD = {};
    zClass_NodePartial parentD = {};
    zClass_NodeFreeListSlot projectileSlotD = {};
    zClass_Object3DDataPartial projectileDataD = {};
    zClass_NodePartial templateChildD = {};
    zClass_NodePartial childD = {};
    projectileSlotD.node.classId = 5;
    projectileSlotD.node.classData = &projectileDataD;
    projectileSlotD.damageHandler = &entry;
    runtimeD.projectileNode = &projectileSlotD.node;
    runtimeD.lifetime = 0.0f;
    entry.attachCloneTemplateNode = &templateChildD;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinel;
    zClass_Class::AddChild(&parentD, &projectileSlotD.node);
    zClass_Object3D::gwObject3DAddChild(&projectileSlotD.node, &templateChildD);
    zClass_Object3D::gwObject3DAddChild(&projectileSlotD.node, &childD);

    OptCatalog::RecycleRuntimeInstanceStorage(&entry, &runtimeD);
    const bool recycleRuntimeOk =
        g_OptCatalogFreeRuntimeInstanceList == &runtimeD && runtimeD.next == &freeSentinel &&
        parentD.listCountB == 0 && projectileSlotD.node.listCountA == 0 &&
        projectileSlotD.node.listCountB == 0 && templateChildD.listCountA == 0 &&
        childD.listCountA == 0 && projectileSlotD.damageHandler == nullptr &&
        projectileDataD.scale.x == 1.0f && projectileDataD.scale.y == 1.0f &&
        projectileDataD.scale.z == 1.0f && projectileDataD.localMatrix[9] == 0.0f &&
        projectileDataD.localMatrix[10] == 0.0f && projectileDataD.localMatrix[11] == 0.0f;
    if (!recycleRuntimeOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 7;
    }

    OptCatalogRuntimeInstanceStorage freeSentinelE = {};
    OptCatalogRuntimeInstanceStorage runtimeE = {};
    zClass_NodePartial runtimeWorldE = {};
    zClass_NodeFreeListSlot projectileSlotE = {};
    zClass_Object3DDataPartial projectileDataE = {};
    runtimeWorldE.classId = 3;
    projectileSlotE.node.classId = 5;
    projectileSlotE.node.classData = &projectileDataE;
    projectileSlotE.damageHandler = &entry;
    runtimeE.projectileNode = &projectileSlotE.node;
    runtimeE.lifetime = 12.0f;
    runtimeE.flyoutAnimPrimary = nullptr;
    runtimeE.flyoutAnimSecondary = nullptr;
    g_OptCatalogRuntimeWorld = &runtimeWorldE;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinelE;
    zClass_Class::AddChild(&runtimeWorldE, &projectileSlotE.node);

    OptCatalog::RecycleRuntimeInstance(&entry, &runtimeE);
    const bool recycleInstanceOk =
        runtimeE.lifetime == 0.0f && runtimeWorldE.listCountB == 0 &&
        projectileSlotE.node.listCountA == 0 &&
        g_OptCatalogFreeRuntimeInstanceList == &runtimeE && runtimeE.next == &freeSentinelE &&
        projectileSlotE.damageHandler == nullptr && projectileDataE.scale.x == 1.0f &&
        projectileDataE.scale.y == 1.0f && projectileDataE.scale.z == 1.0f;
    if (!recycleInstanceOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 8;
    }

    OptCatalogRuntimeInstanceStorage runtimeF = {};
    OptCatalogRuntimeInstanceStorage runtimeG = {};
    OptCatalogRuntimeInstanceStorage freeSentinelF = {};
    zClass_NodeFreeListSlot projectileSlotF = {};
    zClass_Object3DDataPartial projectileDataF = {};
    zClass_NodeFreeListSlot projectileSlotG = {};
    zClass_Object3DDataPartial projectileDataG = {};
    zClass_NodePartial runtimeWorldF = {};
    runtimeWorldF.classId = 3;
    projectileSlotF.node.classId = 5;
    projectileSlotF.node.classData = &projectileDataF;
    projectileSlotG.node.classId = 5;
    projectileSlotG.node.classData = &projectileDataG;
    runtimeF.next = &runtimeG;
    runtimeF.projectileNode = &projectileSlotF.node;
    runtimeG.next = nullptr;
    runtimeG.projectileNode = &projectileSlotG.node;
    entry.activeRuntimeListHead = &runtimeF;
    g_OptCatalogRuntimeWorld = &runtimeWorldF;
    g_OptCatalogFreeRuntimeInstanceList = &freeSentinelF;
    zClass_Class::AddChild(&runtimeWorldF, &projectileSlotF.node);
    zClass_Class::AddChild(&runtimeWorldF, &projectileSlotG.node);

    OptCatalog::ClearRuntimeInstances(&entry);
    const bool clearInstancesOk =
        entry.activeRuntimeListHead == nullptr && runtimeWorldF.listCountB == 0 &&
        projectileSlotF.node.listCountA == 0 && projectileSlotG.node.listCountA == 0 &&
        g_OptCatalogFreeRuntimeInstanceList == &runtimeG && runtimeG.next == &runtimeF &&
        runtimeF.next == &freeSentinelF && projectileDataF.scale.x == 1.0f &&
        projectileDataG.scale.x == 1.0f;
    if (!clearInstancesOk) {
        g_OptCatalogFreeRuntimeInstanceList = oldFreeRuntimeList;
        g_OptCatalogRuntimeWorld = oldRuntimeWorld;
        g_OptCatalogNextSpawnScale = oldNextSpawnScale;
        return 9;
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
        status.lightNode != nullptr || status.nextUpdateTime != 0.0f ||
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
    const bool clearOk = status.runtimeFlags == 0 && status.lightNode == nullptr &&
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

    std::free(worldData.lightNodes);
    std::free(worldData.lightDataList);
    std::free(lightData.attachedWorlds);
    std::free(parent.listB);
    std::free(light.listA);
    g_OptCatalogThermalGlowFreeList = oldThermalGlowFreeList;
    g_OptCatalogRuntimeWorld = oldRuntimeWorld;
    return clampAndBandOk ? 0 : 5;
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

extern "C" int zweapon_optcatalog_deactivate_trail_runtime_state_smoke(void) {
    OptCatalogEntryDef owner = {};
    OptCatalogTrailRuntimeState previous = {};
    OptCatalogTrailRuntimeState runtime = {};
    OptCatalogTrailRuntimeState next = {};
    zClass_NodePartial node0 = {};
    zClass_NodePartial node2 = {};

    owner.activeTrailRuntime = &runtime;
    runtime.ownerEntry = &owner;
    runtime.prev = &previous;
    runtime.next = &next;
    previous.next = &runtime;
    next.prev = &runtime;
    runtime.activeNodeSlotCount = 3;
    runtime.activeNodeSlotCursor = 3;
    runtime.activeNodeSlots[0].node = &node0;
    runtime.activeNodeSlots[2].node = &node2;

    node0.classId = 5;
    node0.flags = 0x04;
    node2.classId = 5;
    node2.flags = 0x04;

    const int result = OptCatalog::DeactivateTrailRuntimeState(&runtime);

    const bool listOk = result == 0 && owner.activeTrailRuntime == &next &&
                        previous.next == &next && next.prev == &previous &&
                        runtime.next == nullptr && runtime.prev == nullptr;
    const bool slotsOk = runtime.activeNodeSlotCursor == 0 &&
                         (node0.flags & 0x04) == 0 && (node2.flags & 0x04) == 0;
    return listOk && slotsOk ? 0 : 1;
}
