#include "OptCatalog.h"

#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"

#include <stdlib.h>

extern "C" {
int g_OptCatalog_CaptureHitSnapshotEnabled = 0;
int g_OptCatalog_FallbackImpactProbeEnabled = 0;
zVec3 g_OptCatalog_CapturedDamageSourcePos = {0};
zVec3 g_OptCatalog_CapturedDamageHitPos = {0};
int g_OptCatalog_EntryCount = 0;
OptCatalogEntryDef *g_OptCatalog_EntryTable = 0;
int g_OptCatalogRuntimeInstanceCount = 0;
void *g_OptCatalogRuntimeInstancePool = 0;
void *g_OptCatalogFreeRuntimeInstanceList = 0;
zClass_NodePartial *g_OptCatalogRuntimeWorld = 0;
void *g_OptCatalogPendingSpawnTargetCountPtr = 0;
void *g_OptCatalogPendingSpawnTargetListPtr = 0;
float g_OptCatalogMaxCraterRadius = 0.0f;
int g_OptCatalogQueuedImpactCount = 0;
int g_OptCatalog_DamageContextKind = 0;
void *g_OptCatalog_DamageContextHitEvent = 0;
void *g_OptCatalogDamageFeedbackCallback = 0;
float g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
int g_OptCatalog_DamageFeedbackHitCount = 0;
zClass_NodePartial *g_OptCatalogDamageFeedbackTrackedNode = 0;
float g_OptCatalogNextSpawnScale = 0.0f;
zClass_NodePartial *g_OptCatalogThermalGlowFreeList = 0;
zReader::Node *g_OptCatalogLoadedTreeRoot = 0;
zSndSample *g_OptCatalogSndTriggerInactive = 0;
zSndSample *g_OptCatalogSndWeaponInactive = 0;
zSndSample *g_OptCatalogSndNoAmmoWarning = 0;
}

namespace {
    OptCatalogDamageHandlerPartial *DamageHandlerForNode(zClass_NodePartial * node) {
        return static_cast<OptCatalogDamageHandlerPartial *>(
            reinterpret_cast<zClass_NodeFreeListSlot *>(node)->damageHandler);
    }
}

namespace OptCatalog {
    // Reimplements 0x4ae450: OptCatalog::FindEntryById (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogEntryDef *RECOIL_FASTCALL FindEntryById(int entryId) {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef &entry = g_OptCatalog_EntryTable[i];
            if (entry.keyName != 0 && entry.ordinalIndex == entryId) {
                return &g_OptCatalog_EntryTable[i];
            }
        }

        return 0;
    }

    // Reimplements 0x4ae4a0: OptCatalog::SetPendingSpawnTargetOverrides
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL SetPendingSpawnTargetOverrides(
        void *pendingSpawnTargetCountPtr, void *pendingSpawnTargetListPtr) {
        g_OptCatalogPendingSpawnTargetCountPtr = pendingSpawnTargetCountPtr;
        g_OptCatalogPendingSpawnTargetListPtr = pendingSpawnTargetListPtr;
    }

    // Reimplements 0x4ae4b0: OptCatalog::AllocOrReuseAttachNodeChildClone
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL AllocOrReuseAttachNodeChildClone(
        OptCatalogEntryDef * self) {
        zClass_NodePartial *const clone = self->attachCloneChildFreeList;
        if (clone != 0) {
            self->attachCloneChildFreeList = clone->callbackContext;
            clone->callbackContext = 0;
            return clone;
        }

        return zClass_cls_util::CopyNodeWithCloneOptions(self->attachCloneTemplateNode, 0, 1);
    }

    // Reimplements 0x4ae520: OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL ClearRuntimeInstanceAsyncFxHandleCallback(
        void *, OptCatalogRuntimeInstanceStorage *runtimeInstance, void *) {
        runtimeInstance->asyncFxHandle = 0;
    }

    // Reimplements 0x4ae530: OptCatalog::AllocOrReuseAttachNodeClone
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL AllocOrReuseAttachNodeClone(
        OptCatalogEntryDef * self) {
        OptCatalogRuntimeInstanceStorage *const runtimeInstance =
            static_cast<OptCatalogRuntimeInstanceStorage *>(g_OptCatalogFreeRuntimeInstanceList);
        if (runtimeInstance == 0) {
            return 0;
        }

        g_OptCatalogFreeRuntimeInstanceList = runtimeInstance->next;

        zClass_NodePartial *attachChildNode = self->attachCloneTemplateNode;
        if (attachChildNode != 0) {
            if (self->flyoutModelAnimationEntry != 0) {
                zClass_NodePartial *const clonedAttachChildNode =
                    AllocOrReuseAttachNodeChildClone(self);
                runtimeInstance->attachCloneChild = clonedAttachChildNode;
                attachChildNode = clonedAttachChildNode;
            }

            zClass_Object3D::gwObject3DAddChild(runtimeInstance->projectileNode, attachChildNode);
        }

        runtimeInstance->lifetime = 0.0f;
        runtimeInstance->updateCallback = 0;
        return runtimeInstance;
    }

    // Reimplements 0x4aeaa0: OptCatalog::SpawnRuntimeInstanceAt
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL SpawnRuntimeInstanceAt(
        OptCatalogEntryDef * self, zVec3 * spawnPos, zClass_NodePartial * ownerNode) {
        OptCatalogRuntimeInstanceStorage *const runtimeInstance = AllocOrReuseAttachNodeClone(self);

        runtimeInstance->next = self->activeRuntimeListHead;
        self->activeRuntimeListHead = runtimeInstance;
        runtimeInstance->pos = *spawnPos;
        runtimeInstance->lifetime = 0.0f;
        runtimeInstance->ownerNode = ownerNode;
        runtimeInstance->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        zClass_Class::gwNodeSetRaycastable(runtimeInstance->projectileNode, 1);
        runtimeInstance->projectileNode->flags |= 0x08000000;
        reinterpret_cast<zClass_NodeFreeListSlot *>(runtimeInstance->projectileNode)
            ->damageHandler = reinterpret_cast<void *>(1);
        runtimeInstance->projectileNode->callbackContext =
            reinterpret_cast<zClass_NodePartial *>(runtimeInstance);
        runtimeInstance->projectileScale = self->flyoutHealth;

        zClass_Object3D::gwObject3DSetPosition(runtimeInstance->projectileNode, spawnPos->x,
                                               spawnPos->y, spawnPos->z);
        zClass_Class::AddChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);
        return runtimeInstance;
    }

    // Reimplements 0x4b1d90: OptCatalog::ShutdownCore
    RECOIL_NOINLINE int RECOIL_CDECL ShutdownCore() {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef &entry = g_OptCatalog_EntryTable[i];
            if (entry.impactFxTable != 0) {
                free(entry.impactFxTable);
                entry.impactFxTable = 0;
            }
            if (entry.killVerbString != 0) {
                free(entry.killVerbString);
                entry.killVerbString = 0;
            }
            if (entry.description != 0) {
                free(entry.description);
                entry.description = 0;
            }
            if (entry.militaryName != 0) {
                free(entry.militaryName);
                entry.militaryName = 0;
            }

            zClass_NodePartial *impactNode = entry.impactNodeListHead;
            while (impactNode != 0) {
                zClass_NodePartial *const next = impactNode->callbackContext;
                entry.impactNodeListHead = next;
                zClass_Util::DestroyNodeRecursive(impactNode);
                impactNode = entry.impactNodeListHead;
            }
        }

        if (g_OptCatalog_EntryTable != 0) {
            free(g_OptCatalog_EntryTable);
            g_OptCatalog_EntryTable = 0;
        }
        if (g_OptCatalogRuntimeInstancePool != 0) {
            free(g_OptCatalogRuntimeInstancePool);
            g_OptCatalogRuntimeInstancePool = 0;
        }
        Light::DestroyThermalGlowPool();
        g_OptCatalogRuntimeWorld = 0;
        zReader::FreeLoadedTree(g_OptCatalogLoadedTreeRoot);
        g_OptCatalogLoadedTreeRoot = 0;

        g_OptCatalog_EntryCount = 0;
        g_OptCatalog_EntryTable = 0;
        g_OptCatalogRuntimeInstanceCount = 0;
        g_OptCatalogRuntimeInstancePool = 0;
        g_OptCatalogFreeRuntimeInstanceList = 0;
        g_OptCatalogRuntimeWorld = 0;
        g_OptCatalogPendingSpawnTargetCountPtr = 0;
        g_OptCatalogPendingSpawnTargetListPtr = 0;
        g_OptCatalog_FallbackImpactProbeEnabled = 1;
        g_OptCatalog_CaptureHitSnapshotEnabled = 1;
        g_OptCatalogQueuedImpactCount = 0;
        g_OptCatalog_DamageFeedbackHitCount = 0;
        return 0;
    }

    // Reimplements 0x4b1180: OptCatalog::Shutdown (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
        ShutdownCore();
        return 0;
    }

    // Reimplements 0x4b1f90: OptCatalog::FreeTrailRuntimeStateStorage
    RECOIL_NOINLINE void RECOIL_FASTCALL FreeTrailRuntimeStateStorage(void *trailRuntimeState) {
        free(trailRuntimeState);
    }

    // Reimplements 0x4b0600: OptCatalog::PlayTriggerInactiveWarning
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_CDECL PlayTriggerInactiveWarning() {
        g_OptCatalogSndTriggerInactive->PlayA3DSimple(1.0f);
    }

    // Reimplements 0x4b0620: OptCatalog::PlayWeaponInactiveWarning
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_CDECL PlayWeaponInactiveWarning() {
        g_OptCatalogSndWeaponInactive->PlayA3DSimple(1.0f);
    }

    // Reimplements 0x4b0640: OptCatalog::PlayNoAmmoWarning (D:\Proj\Battlesport\OptCatalog.cpp)
    RECOIL_NOINLINE void RECOIL_CDECL PlayNoAmmoWarning() {
        g_OptCatalogSndNoAmmoWarning->PlayA3DSimple(1.0f);
    }

    // Reimplements 0x4b2880: OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback
    RECOIL_NOINLINE float RECOIL_FASTCALL CaptureHitSnapshotAndInvokeDamageTimerCallback(
        zVec3 * sourcePos, OptCatalogHitEventPartial * hitEvent, float damageAmount) {
        OptCatalogDamageHandlerPartial *handler = DamageHandlerForNode(hitEvent->hitNode);

        if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
            g_OptCatalog_CapturedDamageSourcePos = *sourcePos;
            g_OptCatalog_CapturedDamageHitPos = hitEvent->hitPos;
        }

        OptCatalogDamageTimerCallback callback =
            reinterpret_cast<OptCatalogDamageTimerCallback>(handler->timerCallback);
        return callback(handler->timerContext, damageAmount);
    }
}
