#include "OptCatalog.h"

#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zSound.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

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
void *g_OptCatalog_CurrentDamageOwnerOrCtx = 0;
void *g_OptCatalogDamageFeedbackCallback = 0;
float g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
int g_OptCatalog_DamageFeedbackHitCount = 0;
zClass_NodePartial *g_OptCatalogDamageFeedbackTrackedNode = 0;
float g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;
float g_OptCatalogNextSpawnScale = 0.0f;
zClass_NodePartial *g_OptCatalogThermalGlowFreeList = 0;
OptCatalogRuntimeInstanceStorage *g_OptCatalog_MineIteratorCursor = 0;
zReader::Node *g_OptCatalogLoadedTreeRoot = 0;
zSndSample *g_OptCatalogSndTriggerInactive = 0;
zSndSample *g_OptCatalogSndWeaponInactive = 0;
zSndSample *g_OptCatalogSndNoAmmoWarning = 0;
}

namespace {
    const unsigned int kOptCatalogFlagSingleTrailSegment = 0x800;
    const unsigned int kOptCatalogFlagSkipTrailSegmentLighting = 0x10000;

    OptCatalogDamageHandlerPartial *DamageHandlerForNode(zClass_NodePartial * node) {
        return static_cast<OptCatalogDamageHandlerPartial *>(
            ((zClass_NodeFreeListSlot *)(node))->damageHandler);
    }
}

namespace OptCatalog {
    // Reimplements 0x4ae3c0: OptCatalog::FindEntryByName
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogEntryDef *RECOIL_FASTCALL FindEntryByName(const char *name) {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef &entry = g_OptCatalog_EntryTable[i];
            if (entry.keyName != 0 && strcmp(name, entry.keyName) == 0) {
                return &g_OptCatalog_EntryTable[i];
            }
        }

        return 0;
    }

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

    // Reimplements 0x4b2130: OptCatalog::CreateTrailSegmentNodeFromTemplate
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE zClass_NodePartial *RECOIL_FASTCALL
    CreateTrailSegmentNodeFromTemplate(zClass_NodePartial *templateNode) {
        zClass_NodePartial *const parent = zClass_Object3D::gwObject3DInit();
        zClass_Class::gwNodeSetActive(parent, 1);
        if (templateNode != 0) {
            zClass_Object3D::gwObject3DAddChild(parent, templateNode);
        }

        return parent;
    }

    // Reimplements 0x4b1ec0: OptCatalog::CreateTrailRuntimeState
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogTrailRuntimeState *RECOIL_FASTCALL
    CreateTrailRuntimeState(OptCatalogEntryDef *entry, zClass_NodePartial *projectileNode,
                            zTag4Partial *variantTagPtr, void *reserved, zVec3 *spawnPos,
                            zVec3 *spawnDir, int segmentCount) {
        (void)reserved;

        OptCatalogTrailRuntimeState *const runtime =
            static_cast<OptCatalogTrailRuntimeState *>(
                calloc(1, sizeof(OptCatalogTrailRuntimeState)));
        runtime->ownerEntry = entry;
        runtime->projectileNode = projectileNode;
        runtime->variantTagPtr = variantTagPtr;
        runtime->spawnPos = spawnPos;
        runtime->spawnDir = spawnDir;

        int activeNodeSlotCount = segmentCount;
        if (activeNodeSlotCount > 8) {
            activeNodeSlotCount = 8;
        } else if ((entry->flags & kOptCatalogFlagSingleTrailSegment) != 0) {
            activeNodeSlotCount = 1;
        }

        runtime->activeNodeSlotCount = activeNodeSlotCount;
        for (int i = 0; i < activeNodeSlotCount; ++i) {
            zClass_NodePartial *const node =
                CreateTrailSegmentNodeFromTemplate(entry->attachCloneTemplateNode);
            runtime->activeNodeSlots[i].node = node;

            char nodeName[40];
            sprintf(nodeName, "BeamReflect_%d", i);
            zClass_Class::gwNodeSetName(node, nodeName);
            zClass_Class::gwNodeSetActive(node, 0);
            if ((entry->flags & kOptCatalogFlagSkipTrailSegmentLighting) == 0) {
                zClass_Object3D::gwObject3DSetLitFlag(node, 1);
            }
            zClass_Class::AddChild(g_OptCatalogRuntimeWorld, node);
        }

        return runtime;
    }

    // Reimplements 0x4b2930: OptCatalog_MineIterator::Begin
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_FASTCALL
    MineIterator_Begin(OptCatalogEntryDef *entry) {
        g_OptCatalog_MineIteratorCursor = entry->activeRuntimeListHead;
        return entry->activeRuntimeListHead;
    }

    // Reimplements 0x4b2940: OptCatalog_MineIterator::Next
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE OptCatalogRuntimeInstanceStorage *RECOIL_CDECL MineIterator_Next() {
        OptCatalogRuntimeInstanceStorage *result = g_OptCatalog_MineIteratorCursor;
        if (result != 0) {
            result = result->next;
            g_OptCatalog_MineIteratorCursor = result;
        }

        return result;
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

    // Reimplements 0x4ae4e0: OptCatalog::RecycleAttachNodeClone
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL RecycleAttachNodeClone(
        OptCatalogEntryDef * self, OptCatalogRuntimeInstanceStorage * runtimeInstance) {
        zEffectAnimEntry *const asyncFxHandle = runtimeInstance->asyncFxHandle;
        if (asyncFxHandle != 0) {
            zEffect_Anim::NodeActionCallback(asyncFxHandle, 0);
        }

        zClass_Object3D::RemoveChild(runtimeInstance->projectileNode,
                                     runtimeInstance->attachCloneChild);
        runtimeInstance->attachCloneChild->callbackContext = self->attachCloneChildFreeList;
        self->attachCloneChildFreeList = runtimeInstance->attachCloneChild;
        runtimeInstance->attachCloneChild = 0;
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
        ((zClass_NodeFreeListSlot *)(runtimeInstance->projectileNode))->damageHandler = (void *)(1);
        runtimeInstance->projectileNode->callbackContext =
            (zClass_NodePartial *)(runtimeInstance);
        runtimeInstance->projectileScale = self->flyoutHealth;

        zClass_Object3D::gwObject3DSetPosition(runtimeInstance->projectileNode, spawnPos->x,
                                               spawnPos->y, spawnPos->z);
        zClass_Class::AddChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);
        return runtimeInstance;
    }

    // Reimplements 0x4aeb50: OptCatalog::RecycleRuntimeInstance
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL RecycleRuntimeInstance(
        OptCatalogEntryDef * self, OptCatalogRuntimeInstanceStorage * runtimeInstance) {
        runtimeInstance->lifetime = 0.0f;

        zEffectAnimEntry *const flyoutAnimPrimary = runtimeInstance->flyoutAnimPrimary;
        if (flyoutAnimPrimary != 0) {
            zEffect_Anim::NodeActionCallback(flyoutAnimPrimary, 0);
            runtimeInstance->flyoutAnimPrimary = 0;
        }

        zEffectAnimEntry *const flyoutAnimSecondary = runtimeInstance->flyoutAnimSecondary;
        if (flyoutAnimSecondary != 0) {
            zEffect_Anim::NodeActionCallback(flyoutAnimSecondary, 0);
            runtimeInstance->flyoutAnimSecondary = 0;
        }

        if (runtimeInstance->attachCloneChild != 0) {
            RecycleAttachNodeClone(self, runtimeInstance);
        }

        zClass_Class::RemoveChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);
        RecycleRuntimeInstanceStorage(self, runtimeInstance);
    }

    // Reimplements 0x4aebc0: OptCatalog::ClearRuntimeInstances
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL ClearRuntimeInstances(OptCatalogEntryDef * self) {
        OptCatalogRuntimeInstanceStorage *runtimeInstance = self->activeRuntimeListHead;
        self->activeRuntimeListHead = 0;
        while (runtimeInstance != 0) {
            OptCatalogRuntimeInstanceStorage *const next = runtimeInstance->next;
            RecycleRuntimeInstance(self, runtimeInstance);
            runtimeInstance = next;
        }
    }

    // Reimplements 0x4ae590: OptCatalog::RecycleRuntimeInstanceStorage
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL RecycleRuntimeInstanceStorage(
        OptCatalogEntryDef * self, OptCatalogRuntimeInstanceStorage * runtimeInstance) {
        if (runtimeInstance->lifetime > 0.0f) {
            return;
        }

        zClass_NodePartial *const projectileNode = runtimeInstance->projectileNode;
        while (projectileNode->listCountA != 0) {
            zClass_Class::RemoveChild(projectileNode->listA[0], projectileNode);
        }

        zClass_NodePartial *const attachCloneTemplateNode = self->attachCloneTemplateNode;
        if (attachCloneTemplateNode != 0) {
            if (runtimeInstance->attachCloneChild != 0) {
                RecycleAttachNodeClone(self, runtimeInstance);
            } else {
                zClass_Class::RemoveChild(projectileNode, attachCloneTemplateNode);
            }
        }

        while (projectileNode->listCountB != 0) {
            zClass_Class::RemoveChild(projectileNode, projectileNode->listB[0]);
        }

        runtimeInstance->next =
            static_cast<OptCatalogRuntimeInstanceStorage *>(g_OptCatalogFreeRuntimeInstanceList);
        g_OptCatalogFreeRuntimeInstanceList = runtimeInstance;
        zClass_Object3D::gwObject3DSetScale(projectileNode, 1.0f, 1.0f, 1.0f);
        zClass_Object3D::gwObject3DSetRotation(projectileNode, 0.0f, 0.0f, 0.0f);
        zClass_Object3D::gwObject3DSetPosition(projectileNode, 0.0f, 0.0f, 0.0f);
        ((zClass_NodeFreeListSlot *)(projectileNode))->damageHandler = 0;
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

    // Reimplements 0x4aefb0: OptCatalog::DeactivateTrailRuntimeState
    // (D:\Proj\Battlesport\OptCatalog.cpp)
    RECOIL_NOINLINE int RECOIL_FASTCALL
    DeactivateTrailRuntimeState(OptCatalogTrailRuntimeState *trailRuntimeState) {
        zSndPlayHandle *const stopSoundHandle = trailRuntimeState->stopSoundHandle;
        OptCatalogEntryDef *const ownerEntry = trailRuntimeState->ownerEntry;

        if (stopSoundHandle != 0) {
            stopSoundHandle->StopIfActive();
        }

        zSndSample *const trailStopSample = ownerEntry->trailStopSample;
        if (trailStopSample != 0) {
            trailStopSample->PlayA3DSimple(1.0f);
        }

        zEffectAnimEntry *const trailEffectAnim = ownerEntry->trailEffectAnim;
        if (trailEffectAnim != 0) {
            zEffectAnim::Stop(trailEffectAnim);
            ownerEntry->trailEffectAnim = 0;
        }

        OptCatalogTrailRuntimeState *const next = trailRuntimeState->next;
        if (next != 0) {
            next->prev = trailRuntimeState->prev;
        }

        OptCatalogTrailRuntimeState *const prev = trailRuntimeState->prev;
        if (prev != 0) {
            prev->next = trailRuntimeState->next;
        }

        if (trailRuntimeState == ownerEntry->activeTrailRuntime) {
            ownerEntry->activeTrailRuntime = trailRuntimeState->next;
        }

        zClass_NodePartial *const lightNode = trailRuntimeState->lightNode;
        trailRuntimeState->prev = 0;
        trailRuntimeState->next = 0;
        if (lightNode != 0) {
            Light::ReturnToFreeList(lightNode);
        }

        for (int i = 0; i < trailRuntimeState->activeNodeSlotCount; ++i) {
            zClass_NodePartial *const node = trailRuntimeState->activeNodeSlots[i].node;
            if (node != 0) {
                zClass_Class::gwNodeSetActive(node, 0);
            }
        }

        trailRuntimeState->activeNodeSlotCursor = 0;
        return 0;
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

    // Reimplements 0x4b28e0: OptCatalog::SetDamageContext
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_FASTCALL SetDamageContext(
        int contextKind, OptCatalogHitEventPartial * contextHitEvent) {
        if (contextHitEvent != 0 && contextHitEvent->hitNode != 0) {
            g_OptCatalog_DamageContextHitEvent = contextHitEvent;
        }

        g_OptCatalog_DamageContextKind = contextKind;
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
            (OptCatalogDamageTimerCallback)(handler->timerCallback);
        return callback(handler->timerContext, damageAmount);
    }

    // Reimplements 0x4b2910: OptCatalog::GetCapturedHitSourcePtr
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE zVec3 *RECOIL_CDECL GetCapturedHitSourcePtr() {
        return &g_OptCatalog_CapturedDamageSourcePos;
    }
}

namespace DamageFeedback {
    // Reimplements 0x4b2900: DamageFeedback::SetIntensityScalar
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void RECOIL_STDCALL SetIntensityScalar(float scalar) {
        g_OptCatalogDamageFeedbackIntensityScalar = scalar;
    }
}

namespace HitContext {
    // Reimplements 0x4b2920: HitContext::GetCurrentOwnerOrCtx
    // (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
    RECOIL_NOINLINE void *RECOIL_CDECL GetCurrentOwnerOrCtx() {
        return g_OptCatalog_CurrentDamageOwnerOrCtx;
    }
}
