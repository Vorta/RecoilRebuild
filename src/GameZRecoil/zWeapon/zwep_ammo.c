// zWeapon compilation unit before zwep_init.c, inferred from the retail object
// boundary [0x4ae380, 0x4b1090): its .rdata pooled constants [0x4d3388,
// 0x4d33f4) repeat 0.0f and 1.0f that zwep_init.c pools again at [0x4d33f4,
// 0x4d3404), and its .bss statics occupy [0x56bc9c, 0x56bcb0). Ending the
// object before zWepInit is a placement heuristic. Retail .data also begins
// with two floats read only by ProcessRuntimeInstances (0x4e42e4, 0x4e42e8)
// that are not modeled. Original filename unresolved; zwep_ammo.c is a
// provisional name after the same-engine zwep_ammo.cpp (2026-10-02).

#include "opt_catalog.h"
#include "zwep.h"

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-allocruntimegatecallback
 * @recoil-artifact defines .data recoil:data:0x56bc9c: g_OptCatalog_AllocRuntimeGateCallback.
 * BN xrefs: GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks installs
 * the callback; OptCatalog::AllocRuntimeInstance calls it when network gate
 * processing is enabled.
 * Purpose: optional allocation gate for networked OptCatalog runtime
 * instances.
 */
OptCatalogAllocRuntimeGateCallback g_OptCatalog_AllocRuntimeGateCallback = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-altgundispatchnoopcallback
 * @recoil-artifact defines .data recoil:data:0x56bca0: g_OptCatalog_AltGunDispatchNoOpCallback.
 * BN xrefs: GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks installs
 * the alternate-gun no-op dispatch callback.
 * Purpose: callback slot paired with the runtime allocation gate for
 * alternate-gun dispatch processing.
 */
OptCatalogAllocRuntimeGateCallback g_OptCatalog_AltGunDispatchNoOpCallback = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-removeruntimerelaycallback
 * @recoil-artifact defines .data recoil:data:0x56bca4: g_OptCatalog_RemoveRuntimeRelayCallback.
 * BN xrefs: GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks installs
 * the callback; OptCatalog::RemoveRuntimeInstance invokes it after removal.
 * Purpose: optional network relay hook for removed OptCatalog runtime
 * instances.
 */
OptCatalogRemoveRuntimeRelayCallback g_OptCatalog_RemoveRuntimeRelayCallback = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-f-0x56bca8
 * @recoil-artifact defines .data recoil:data:0x56bca8: g_OptCatalogRuntimeDeltaTime.
 * Purpose: current unscaled frame delta consumed by OptCatalog projectile and
 * trail runtime processing.
 */
float g_OptCatalogRuntimeDeltaTime = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-f-0x56bcac
 * @recoil-artifact defines .data recoil:data:0x56bcac: g_OptCatalogRuntimeNowSec.
 * Purpose: current unscaled time used by OptCatalog runtime updates and
 * warning-sound gates.
 */
float g_OptCatalogRuntimeNowSec = 0.0f;
}

namespace
{
    struct OptCatalogQueuedImpactRecord {
        OptCatalogEntryDef* entry;
        CZNodePartial* ownerNode;
        zVec3 sourcePos;
        OptCatalogRaycastHitEntry hit;
        float damageAmount;
        unsigned char unknown_40[4];
    };

    RECOIL_STATIC_ASSERT(sizeof(OptCatalogQueuedImpactRecord) == 68);

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogqueuedimpacts
     * @recoil-artifact defines .data recoil:data:0x778970: g_OptCatalogQueuedImpactRecords.
     * BN data shape: OptCatalogQueuedImpactRecord[64], 4352 bytes, zero-filled
     * BSS. Paired with g_OptCatalogQueuedImpactCount at 0x77896c.
     * Purpose: deferred impact callback queue drained by
     * OptCatalog::ProcessRuntimeInstances.
     */
    OptCatalogQueuedImpactRecord g_OptCatalogQueuedImpacts[64] = { 0 };
} // namespace

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogprocessruntimerelayenabled
 * @recoil-artifact defines .data recoil:data:0x4dcf7c: g_OptCatalogProcessRuntimeRelayEnabled.
 * BN initial bytes are 01 00 00 00. BN xrefs:
 * OptCatalog::SendPkt0ARemoveRuntimeRelay reads the gate and
 * OptCatalog::HandlePkt0ARemoveRuntimeRelay clears/restores it around local
 * relay processing.
 * Purpose: suppresses recursive runtime-removal relay while a network packet
 * is being handled.
 */
int g_OptCatalogProcessRuntimeRelayEnabled = 1;
}

namespace
{
    const unsigned int kOptCatalogFlagImmediateProbeImpact = 1u << 12;
    const unsigned int kOptCatalogFlagFullProbeDamage = 1u << 13;
    const unsigned int kOptCatalogFlagCraterImpact = 0x08;
    const unsigned int kOptCatalogFlagQuickSandImpact = 0x20000;
    const unsigned int kOptCatalogFlagAlwaysPlayImpactFx = 4194304;
    const unsigned int kOptCatalogFlagTrailRuntime = 2;
    const unsigned int kOptCatalogFlagImpactWhenScaleExpired = 4;

    const unsigned int kOptCatalogFlagAllowOutOfRangeAimPitch = 0x2000;

    const unsigned int kOptCatalogFlagForceSpawnVelocity = 0x400;
    const unsigned int kOptCatalogFlagRelativeSpeed = 0x800000;
    const unsigned int kOptCatalogFlagFlyoutSkipRotation = 0x2000;
    const unsigned int kOptCatalogFlagFlyoutModelRotation = 0x100;
    const unsigned int kOptCatalogFlagUsePendingSpawnTarget = 1u << 22;
    const unsigned int kOptCatalogFlagTrailUsePendingSpawnTargets = 1u << 14;
    const unsigned int kOptCatalogFlagTrailStartMutedAndLight = 1u << 11;
    const unsigned int kOptCatalogFlagExpires = 1u << 6;
    const unsigned int kOptCatalogFlagFixedRotate = 1u << 7;
    const unsigned int kOptCatalogFlagInstant = 1u << 10;
    const unsigned int kOptCatalogFlagLockOn = 1u << 14;
    const unsigned int kOptCatalogFlagLockOnLead = 1u << 15;

    const unsigned int kOptCatalogFlagRemoteDetonate = 1u << 19;
    const unsigned int kOptCatalogFlagTetherGuided = 1u << 20;

    const unsigned int kOptCatalogNodeFlagAcceptsTerrainDeformation = 0x10000;
    const unsigned int kOptCatalogFastSqrtBias = 0x1fc00000;

    const int kMaxQueuedImpacts = 64;
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-f-0x4d33ec
     * @recoil-artifact defines .rdata recoil:data:0x4d33ec: kOptCatalogAimPitchRangeScale.
     * Purpose: scales OptCatalog aim pitch range values loaded from weapon
     * catalog data.
     */
    const float kOptCatalogAimPitchRangeScale = -0.239999995f;
    const float kOptCatalogTrailDamageBlendLimit = 0.25f;
    const double kOptCatalogPi = 3.14159265359;

    typedef void(__fastcall * OptCatalogRuntimeUpdateCallback)(OptCatalogRuntimeInstanceStorage * runtimeInstance);

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in OptCatalog aim and trail math callsites in this source file.
     * Purpose: approximate square root through the recovered bit-bias idiom.
     */
    float FastSqrtApprox(float value)
    {
        unsigned int bits = 0;
        memcpy(&bits, &value, sizeof(bits));
        bits = (bits >> 1) + kOptCatalogFastSqrtBias;
        memcpy(&value, &bits, sizeof(value));
        return value;
    }
} // namespace

namespace OptCatalog
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-blenddirectiontowardtarget
     * @recoil-artifact defines .text recoil:function:0x4ae380: OptCatalog::BlendDirectionTowardTarget
     * @recoil-match byte
     *
     * Purpose: blend an active direction vector toward a target direction
     * using per-axis weights, then renormalize the result.
     */
    void __fastcall BlendDirectionTowardTarget(
        zVec3 * direction,
        const zVec3* targetDirection,
        float xWeight,
        float yWeight,
        float zWeight
    )
    {
        direction->x += (targetDirection->x - direction->x) * xWeight;
        direction->y += (targetDirection->y - direction->y) * yWeight;
        direction->z += (targetDirection->z - direction->z) * zWeight;
        zMath::Vec3Normalize(direction);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-findentrybyname
     * @recoil-artifact defines .text recoil:function:0x4ae3c0: OptCatalog::FindEntryByName
     * @recoil-match byte
     *
     * Purpose: return the first loaded OptCatalog entry whose keyName matches
     * the requested catalog name.
     */
    OptCatalogEntryDef* __fastcall FindEntryByName(const char* name)
    {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef& entry = g_OptCatalog_EntryTable[i];
            if (entry.keyName != 0 && strcmp(name, entry.keyName) == 0) {
                return &g_OptCatalog_EntryTable[i];
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-findentrybyid
     * @recoil-artifact defines .text recoil:function:0x4ae450: OptCatalog::FindEntryById
     * @recoil-match byte
     *
     * Purpose: return the first loaded OptCatalog entry whose ordinalIndex
     * matches the requested catalog id.
     */
    OptCatalogEntryDef* __fastcall FindEntryById(int entryId)
    {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef& entry = g_OptCatalog_EntryTable[i];
            if (entry.keyName != 0 && entryId == entry.ordinalIndex) {
                return &g_OptCatalog_EntryTable[i];
            }
        }

        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-setpendingspawntargetoverrides
     * @recoil-artifact defines .text recoil:function:0x4ae4a0: OptCatalog::SetPendingSpawnTargetOverrides
     * @recoil-match byte
     *
     * Purpose: install the pending-spawn target count and list pointers used
     * by OptCatalog runtime spawn setup.
     */
    void __fastcall SetPendingSpawnTargetOverrides(void* pendingSpawnTargetCountPtr, void* pendingSpawnTargetListPtr)
    {
        g_OptCatalogPendingSpawnTargetCountPtr = (int*)(pendingSpawnTargetCountPtr);
        g_OptCatalogPendingSpawnTargetListPtr = (PlayerProgressTargetSlotRuntime*)(pendingSpawnTargetListPtr);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-allocorreuseattachnodechildclone
     * @recoil-artifact defines .text recoil:function:0x4ae4b0: OptCatalog::AllocOrReuseAttachNodeChildClone
     * @recoil-match byte
     *
     * Purpose: reuse an attach-clone child from the entry free list, or clone
     * the template node when none are available.
     */
    CZNodePartial* __fastcall AllocOrReuseAttachNodeChildClone(OptCatalogEntryDef * self)
    {
        CZNodePartial* const clone = self->attachCloneChildFreeList;
        if (clone != 0) {
            self->attachCloneChildFreeList = clone->callbackContext;
            clone->callbackContext = 0;
            return clone;
        }

        return CZUtil::CopyNodeWithCloneOptions(self->attachCloneTemplateNode, 0, 1);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-recycleattachnodeclone
     * @recoil-artifact defines .text recoil:function:0x4ae4e0: OptCatalog::RecycleAttachNodeClone
     * @recoil-match byte
     *
     * Purpose: stop pending attach animation work, detach the child clone,
     * and return it to the entry clone free list.
     */
    void __fastcall RecycleAttachNodeClone(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    )
    {
        zEffectAnimEntry* const asyncFxHandle = runtimeInstance->asyncFxHandle;
        if (asyncFxHandle != 0) {
            zEffect_Anim::NodeActionCallback(asyncFxHandle, 0);
        }

        CZObject3D::RemoveChild(runtimeInstance->projectileNode, runtimeInstance->attachCloneChild);
        runtimeInstance->attachCloneChild->callbackContext = self->attachCloneChildFreeList;
        self->attachCloneChildFreeList = runtimeInstance->attachCloneChild;
        runtimeInstance->attachCloneChild = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-clearruntimeinstanceasyncfxhandlecallback
     * @recoil-artifact defines .text recoil:function:0x4ae520: OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback
     * @recoil-match byte
     *
     * Purpose: clear the runtime instance async FX handle after the attached
     * model animation completes.
     */
    void __fastcall ClearRuntimeInstanceAsyncFxHandleCallback(
        void*,
        OptCatalogRuntimeInstanceStorage* runtimeInstance,
        void*
    )
    {
        runtimeInstance->asyncFxHandle = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-allocorreuseattachnodeclone
     * @recoil-artifact defines .text recoil:function:0x4ae530: OptCatalog::AllocOrReuseAttachNodeClone
     * @recoil-match byte
     *
     * Purpose: take a runtime instance from the free list, attach any flyout
     * child clone, and reset per-spawn lifetime state.
     */
    OptCatalogRuntimeInstanceStorage* __fastcall AllocOrReuseAttachNodeClone(OptCatalogEntryDef * self)
    {
        OptCatalogRuntimeInstanceStorage* const runtimeInstance = g_OptCatalogFreeRuntimeInstanceList;
        if (runtimeInstance == 0) {
            return 0;
        }

        g_OptCatalogFreeRuntimeInstanceList = runtimeInstance->next;

        CZNodePartial* attachChildNode = self->attachCloneTemplateNode;
        if (attachChildNode != 0) {
            if (self->flyoutModelAnimationEntry != 0) {
                CZNodePartial* const clonedAttachChildNode = AllocOrReuseAttachNodeChildClone(self);
                runtimeInstance->attachCloneChild = clonedAttachChildNode;
                attachChildNode = clonedAttachChildNode;
            }

            CZObject3D::gwObject3DAddChild(runtimeInstance->projectileNode, attachChildNode);
        }

        runtimeInstance->lifetime = 0.0f;
        runtimeInstance->updateCallback = 0;
        return runtimeInstance;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-recycleruntimeinstancestorage
     * @recoil-artifact defines .text recoil:function:0x4ae590: OptCatalog::RecycleRuntimeInstanceStorage
     * @recoil-match byte
     *
     * Purpose: detach projectile children, restore transform and collision
     * state, and push the runtime storage back onto the free list.
     */
    void __fastcall RecycleRuntimeInstanceStorage(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    )
    {
        if (runtimeInstance->lifetime > 0.0f) {
            return;
        }

        while (runtimeInstance->projectileNode->listCountA != 0) {
            CZClass::RemoveChild(runtimeInstance->projectileNode->listA[0], runtimeInstance->projectileNode);
        }

        if (self->attachCloneTemplateNode != 0) {
            if (runtimeInstance->attachCloneChild == 0) {
                CZClass::RemoveChild(runtimeInstance->projectileNode, self->attachCloneTemplateNode);
            } else {
                RecycleAttachNodeClone(self, runtimeInstance);
            }
        }

        while (runtimeInstance->projectileNode->listCountB != 0) {
            CZClass::RemoveChild(runtimeInstance->projectileNode, runtimeInstance->projectileNode->listB[0]);
        }

        runtimeInstance->next = g_OptCatalogFreeRuntimeInstanceList;
        g_OptCatalogFreeRuntimeInstanceList = runtimeInstance;
        CZObject3D::gwObject3DSetScale(runtimeInstance->projectileNode, 1.0f, 1.0f, 1.0f);
        CZObject3D::gwObject3DSetRotation(runtimeInstance->projectileNode, 0.0f, 0.0f, 0.0f);
        CZObject3D::gwObject3DSetPosition(runtimeInstance->projectileNode, 0.0f, 0.0f, 0.0f);
        ((CZNodeFreeListSlot*)(runtimeInstance->projectileNode))->damageHandler = 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-allocruntimeinstance
     * @recoil-artifact defines .text recoil:function:0x4ae660: OptCatalog::AllocRuntimeInstance
     *
     *
     * Purpose: allocate or reuse a projectile runtime instance, link it active,
     * initialize motion, FX, target, and collision state for the spawn.
     */
    OptCatalogRuntimeInstanceStorage* __fastcall AllocRuntimeInstance(
        OptCatalogEntryDef * self,
        CZNodePartial * ownerNode,
        zTag4Partial * variantTagOrNull,
        zVec3 * spawnPos,
        zVec3 * spawnDir,
        zVec3 * spawnVelocity,
        void* saveState,
        OptCatalogRuntimeInstanceStorage* runtimeInstanceOrNull
    )
    {
        if (g_OptCatalogNetworkOptionState != 0 && g_OptCatalog_AllocRuntimeGateCallback != 0
            && g_OptCatalog_AllocRuntimeGateCallback(self, &saveState) == 0) {
            return 0;
        }

        OptCatalogRuntimeInstanceStorage* runtimeInstance = runtimeInstanceOrNull;
        if (runtimeInstance == 0) {
            runtimeInstance = AllocOrReuseAttachNodeClone(self);
            if (runtimeInstance == 0) {
                return 0;
            }
        }

        runtimeInstance->next = self->activeRuntimeListHead;
        self->activeRuntimeListHead = runtimeInstance;
        CZClass::AddChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);

        runtimeInstance->origin = *spawnPos;
        runtimeInstance->pos = *spawnPos;
        runtimeInstance->dir = *spawnDir;
        runtimeInstance->ownerNode = ownerNode;
        runtimeInstance->rangeProgress = 0.0f;
        runtimeInstance->scaleFade = 0.0f;
        runtimeInstance->saveState = saveState;
        if (variantTagOrNull != 0) {
            memcpy(&runtimeInstance->variantTag, variantTagOrNull, sizeof(runtimeInstance->variantTag));
        } else {
            runtimeInstance->variantTag = 4;
        }
        runtimeInstance->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        runtimeInstance->speed = self->velocity;
        if (self->acceleration == 0.0f && (self->flags & kOptCatalogFlagForceSpawnVelocity) == 0) {
            runtimeInstance->lifetime = self->velocity;
            zMath::Vec3ScaleAdd(spawnVelocity, spawnDir, self->velocity, &runtimeInstance->velocity);
        } else {
            runtimeInstance->lifetime = 0.0000999999975f;
            runtimeInstance->velocity = *spawnVelocity;
            if ((self->flags & kOptCatalogFlagRelativeSpeed) != 0) {
                const float relativeSpeed = (float)(sqrt(
                    (spawnVelocity->x * spawnVelocity->x) + (spawnVelocity->y * spawnVelocity->y)
                    + (spawnVelocity->z * spawnVelocity->z)
                ));
                runtimeInstance->speed += relativeSpeed;
                runtimeInstance->lifetime += relativeSpeed;
                runtimeInstance->velocity.x -= spawnDir->x * relativeSpeed;
                runtimeInstance->velocity.y -= spawnDir->y * relativeSpeed;
                runtimeInstance->velocity.z -= spawnDir->z * relativeSpeed;
            }
        }

        if (self->fireFxSelectedSoundIndex != -1) {
            self->fireFxSoundSamples[self->fireFxSelectedSoundIndex]->PlayA3D(1.0f, &runtimeInstance->pos, 0);
        }

        if (self->fireFxEffectTemplateIndex != 0) {
            zEffect::SpawnRuntimeInstanceAt(self->fireFxEffectTemplateIndex, &runtimeInstance->pos);
        } else if (self->fireFxSelectedEffectIndex != -1) {
            zEffectAnimEntry* const fireAnim = self->fireFxAnimationEntries[self->fireFxSelectedEffectIndex];
            if (fireAnim != 0) {
                float randomRoll = 0.0f;
                if ((self->fireFxFlags & 1u) != 0) {
                    randomRoll = (((float)(rand()) * 0.0000305185094f) - 0.5f) * (float)(kOptCatalogPi);
                }

                // Retail null-checks the selected entry but always animates entry 0.
                zEffectAnim::SetTransformRotAndVelocityThunk(
                    self->fireFxAnimationEntries[0],
                    0,
                    runtimeInstance->pos.x,
                    runtimeInstance->pos.y,
                    runtimeInstance->pos.z,
                    (float)asin((double)spawnDir->y),
                    (float)(atan2(-spawnDir->x, -spawnDir->z)),
                    randomRoll,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        }

        if ((self->flags & kOptCatalogFlagFlyoutSkipRotation) == 0
            && (((self->flags & kOptCatalogFlagFlyoutModelRotation) != 0 && self->attachCloneTemplateNode != 0)
                || (self->flyoutAnimationEntry != 0 && self->attachCloneTemplateNode == 0))) {
            CZObject3D::gwObject3DSetRotation(
                runtimeInstance->projectileNode,
                (float)asin((double)spawnDir->y),
                (float)(atan2(-spawnDir->x, -spawnDir->z)),
                0.0f
            );
        }

        CZObject3D::gwObject3DSetPosition(
            runtimeInstance->projectileNode,
            runtimeInstance->pos.x,
            runtimeInstance->pos.y,
            runtimeInstance->pos.z
        );

        if (self->flyoutSelectedEffectIndex != -1) {
            if (self->flyoutAnimationEntry != 0) {
                runtimeInstance->flyoutAnimPrimary = zEffectAnim::SetTransformRefsThunk(
                    self->flyoutAnimationEntry,
                    0,
                    runtimeInstance->projectileNode,
                    0,
                    runtimeInstance->projectileNode,
                    0
                );
            }
            if (self->flyoutAttachedAnimationEntry != 0) {
                runtimeInstance->flyoutAnimSecondary = zEffectAnim::SetPositionRefAndVelocityThunk(
                    self->flyoutAttachedAnimationEntry,
                    0,
                    runtimeInstance->projectileNode,
                    0,
                    0
                );
            }
            if (self->flyoutModelAnimationEntry != 0) {
                zEffectAnimEntry* const asyncFxHandle = zEffectAnim::SetVelocityThunk(
                    self->flyoutModelAnimationEntry,
                    runtimeInstance->attachCloneChild,
                    0.0f,
                    0.0f,
                    0.0f
                );
                runtimeInstance->asyncFxHandle = asyncFxHandle;
                zEffectAnimEntry::SetOnStateDoneCallback(
                    asyncFxHandle,
                    (void*)(&ClearRuntimeInstanceAsyncFxHandleCallback),
                    runtimeInstance
                );
            }
        }

        runtimeInstance->aux = zMath::g_zMath_Vec3Zero;
        runtimeInstance->spawnGateAccum = 0.0f;
        runtimeInstance->pendingTargetA = 0;
        runtimeInstance->pendingTargetB = 0;
        if ((self->flags & kOptCatalogFlagLockOn) != 0 && g_OptCatalogPendingSpawnTargetListPtr != 0) {
            runtimeInstance->aux = *spawnVelocity;
            int* const pendingTargetCount = g_OptCatalogPendingSpawnTargetCountPtr;
            if (pendingTargetCount != 0 && *pendingTargetCount > 0) {
                PlayerProgressTargetSlotRuntime* const targetList = g_OptCatalogPendingSpawnTargetListPtr;
                runtimeInstance->pendingTargetA = targetList[0].targetPos;
                runtimeInstance->pendingTargetB = targetList[0].targetVelocity;
            }
            g_OptCatalogPendingSpawnTargetCountPtr = 0;
        }

        if ((self->flags & kOptCatalogFlagImpactWhenScaleExpired) != 0) {
            CZClass::gwNodeSetRaycastable(runtimeInstance->projectileNode, 1);
            runtimeInstance->projectileNode->flags |= 0x08000000;
            ((CZNodeFreeListSlot*)(runtimeInstance->projectileNode))->damageHandler = (void*)(1);
            runtimeInstance->projectileNode->callbackContext = (CZNodePartial*)(runtimeInstance);
            runtimeInstance->projectileScale = self->flyoutHealth;
        } else {
            CZClass::gwNodeSetRaycastable(runtimeInstance->projectileNode, 0);
            runtimeInstance->projectileNode->flags &= ~0x08000000u;
        }

        self->fireFxSelectedSoundIndex = 0;
        self->fireFxSelectedEffectIndex = 0;
        self->flyoutSelectedEffectIndex = 0;
        return runtimeInstance;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-spawnruntimeinstanceat
     * @recoil-artifact defines .text recoil:function:0x4aeaa0: OptCatalog::SpawnRuntimeInstanceAt
     * @recoil-match byte
     *
     * Purpose: spawn a positioned impact-scale runtime instance and attach
     * its projectile node to the OptCatalog runtime world.
     */
    OptCatalogRuntimeInstanceStorage* __fastcall SpawnRuntimeInstanceAt(
        OptCatalogEntryDef * self,
        zVec3 * spawnPos,
        CZNodePartial * ownerNode
    )
    {
        OptCatalogRuntimeInstanceStorage* const runtimeInstance = AllocOrReuseAttachNodeClone(self);

        runtimeInstance->next = self->activeRuntimeListHead;
        self->activeRuntimeListHead = runtimeInstance;
        runtimeInstance->pos = *spawnPos;
        runtimeInstance->lifetime = 0.0f;
        runtimeInstance->ownerNode = ownerNode;
        runtimeInstance->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        CZClass::gwNodeSetRaycastable(runtimeInstance->projectileNode, 1);
        runtimeInstance->projectileNode->flags |= 0x08000000;
        ((CZNodeFreeListSlot*)(runtimeInstance->projectileNode))->damageHandler = (void*)(1);
        runtimeInstance->projectileNode->callbackContext = (CZNodePartial*)(runtimeInstance);
        runtimeInstance->projectileScale = self->flyoutHealth;

        CZObject3D::gwObject3DSetPosition(runtimeInstance->projectileNode, spawnPos->x, spawnPos->y, spawnPos->z);
        CZClass::AddChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);
        return runtimeInstance;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-recycleruntimeinstance
     * @recoil-artifact defines .text recoil:function:0x4aeb50: OptCatalog::RecycleRuntimeInstance
     * @recoil-match byte
     *
     * Purpose: stop runtime FX, recycle any attach clone, detach the projectile
     * node from the runtime world, and return storage to the free list.
     */
    void __fastcall RecycleRuntimeInstance(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    )
    {
        runtimeInstance->lifetime = 0.0f;

        zEffectAnimEntry* const flyoutAnimPrimary = runtimeInstance->flyoutAnimPrimary;
        if (flyoutAnimPrimary != 0) {
            zEffect_Anim::NodeActionCallback(flyoutAnimPrimary, 0);
            runtimeInstance->flyoutAnimPrimary = 0;
        }

        zEffectAnimEntry* const flyoutAnimSecondary = runtimeInstance->flyoutAnimSecondary;
        if (flyoutAnimSecondary != 0) {
            zEffect_Anim::NodeActionCallback(flyoutAnimSecondary, 0);
            runtimeInstance->flyoutAnimSecondary = 0;
        }

        if (runtimeInstance->attachCloneChild != 0) {
            RecycleAttachNodeClone(self, runtimeInstance);
        }

        CZClass::RemoveChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);
        RecycleRuntimeInstanceStorage(self, runtimeInstance);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-clearruntimeinstances
     * @recoil-artifact defines .text recoil:function:0x4aebc0: OptCatalog::ClearRuntimeInstances
     * @recoil-match byte
     *
     * Purpose: unlink and recycle every active runtime instance owned by the
     * catalog entry.
     */
    void __fastcall ClearRuntimeInstances(OptCatalogEntryDef * self)
    {
        OptCatalogRuntimeInstanceStorage* runtimeInstance = self->activeRuntimeListHead;
        self->activeRuntimeListHead = 0;
        while (runtimeInstance != 0) {
            OptCatalogRuntimeInstanceStorage* const current = runtimeInstance;
            runtimeInstance = runtimeInstance->next;
            RecycleRuntimeInstance(self, current);
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-removeruntimeinstance
     * @recoil-artifact defines .text recoil:function:0x4aebf0: OptCatalog::RemoveRuntimeInstance
     * @recoil-match byte
     *
     * Purpose: process and recycle matching active runtime instances, or probe
     * a supplied point, then notify the remove-runtime relay callback.
     */
    int __fastcall RemoveRuntimeInstance(OptCatalogEntryDef * self, zVec3 * pointOrVec3, CZNodePartial * ownerNode)
    {
        int result = 0;

        if (pointOrVec3 == 0) {
            OptCatalogRuntimeInstanceStorage* next = self->activeRuntimeListHead;
            OptCatalogRuntimeInstanceStorage** link = &self->activeRuntimeListHead;
            while (next != 0) {
                OptCatalogRuntimeInstanceStorage* const runtimeInstance = next;
                next = runtimeInstance->next;
                if ((self->flags & (1u << 20)) != 0
                    || (runtimeInstance->lifetime == 0.0f
                        && (ownerNode == 0 || runtimeInstance->ownerNode == ownerNode))) {
                    *link = next;
                    result += ProcessRuntimeInstance(self, runtimeInstance);
                    RecycleRuntimeInstance(self, runtimeInstance);
                } else {
                    link = &runtimeInstance->next;
                }
            }
        } else {
            OptCatalogRuntimeInstanceStorage runtimeInstance = { 0 };
            runtimeInstance.pos = *pointOrVec3;
            runtimeInstance.ownerNode = ownerNode;
            runtimeInstance.spawnScale = 1.0f;
            result = ProcessRuntimeInstance(self, &runtimeInstance);
        }

        if (result != 0 && g_OptCatalog_RemoveRuntimeRelayCallback != 0) {
            g_OptCatalog_RemoveRuntimeRelayCallback(self, pointOrVec3, ownerNode);
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-processruntimeinstance
     * @recoil-artifact defines .text recoil:function:0x4aed00: OptCatalog::ProcessRuntimeInstance
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef* and EDX is
     * OptCatalogRuntimeInstanceStorage*. Builds a vertical probe from runtime
     * position, masks and restores projectile active state for closest-hit
     * raycast against g_OptCatalogRuntimeWorld, dispatches direct hits through
     * HandleImpactEvent, then optionally runs the fallback impact probe using
     * BuildImpactHitList and HandleImpactFromRuntimeProbe.
     * Purpose: advance one runtime projectile through direct and fallback impact checks.
     */
    int __fastcall ProcessRuntimeInstance(OptCatalogEntryDef * self, OptCatalogRuntimeInstanceStorage * runtimeInstance)
    {
        CZNodePartial* const projectileNode = runtimeInstance->projectileNode;
        zVec3 startPoint = runtimeInstance->pos;
        zVec3 endPoint = runtimeInstance->pos;
        startPoint.y += 1.0f;
        endPoint.y -= self->impactProximity * 0.1f;

        int result = 0;
        int restoreProjectileActive = 0;
        if (projectileNode != 0 && (projectileNode->flags & 0x04) != 0) {
            restoreProjectileActive = 1;
            CZClass::gwNodeSetActive(projectileNode, 0);
        }

        PlayerProbeSampleCandidateBuffer rayData;
        if (CZDisplayInstance::RaycastSelectClosestHitBetweenPoints(
                g_OptCatalogRuntimeWorld,
                &startPoint,
                &endPoint,
                &rayData
            )
            == 0) {
            OptCatalogHitEventPartial* const hitEvent
                = (OptCatalogHitEventPartial*)(void*)(&rayData.entries[rayData.candidateCount]);
            HandleImpactEvent(self, hitEvent, runtimeInstance);
            result = 1;
        }

        if (restoreProjectileActive != 0) {
            CZClass::gwNodeSetActive(projectileNode, 1);
        }

        if (g_OptCatalog_FallbackImpactProbeEnabled != 0 && self->impactProximity > 0.0f) {
            OptCatalogRaycastHitList fallbackHits;
            if (BuildImpactHitList(self, runtimeInstance, 1, &fallbackHits) != 0) {
                runtimeInstance->pos = startPoint;
                HandleImpactFromRuntimeProbe(self, runtimeInstance, &fallbackHits, 0);
            }
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-activatetrailruntimestate
     * @recoil-artifact defines .text recoil:function:0x4aee40: OptCatalog::ActivateTrailRuntimeState
     *
     *
     * BN source path: src/Battlesport/zWeapon.cpp.
     * BN behavior: ECX is OptCatalogTrailRuntimeState*, EDX carries
     * playerOrdinal but is not consumed. Starts trail stop/loop audio,
     * optionally mutes the loop, spawns the fire effect or trail animation,
     * resets trail timers, consumes g_OptCatalogNextSpawnScale, captures
     * pending spawn targets, optionally allocates a glow light, and links the
     * state at owner->activeTrailRuntime.
     * Data touch: reads/writes g_OptCatalogNextSpawnScale at 0x779aac and
     * reads/clears g_OptCatalogPendingSpawnTargetCountPtr at 0x77895c when
     * pending trail targets are enabled.
     * Purpose: activate a prebuilt trail runtime state for a weapon owner.
     */
    int __fastcall ActivateTrailRuntimeState(OptCatalogTrailRuntimeState * trailRuntimeState, int playerOrdinal)
    {
        (void)playerOrdinal;

        OptCatalogEntryDef* const ownerEntry = trailRuntimeState->ownerEntry;
        ownerEntry->trailStopSample->PlayA3DSimple(1.0f);
        zSndPlayHandle* const loopHandle = ownerEntry->trailLoopSample->PlayA3DSimple(1.0f);
        trailRuntimeState->stopSoundHandle = loopHandle;
        if ((ownerEntry->flags & kOptCatalogFlagTrailStartMutedAndLight) != 0) {
            loopHandle->SetFreqScaled(0.0f);
        }

        if (ownerEntry->fireFxEffectTemplateIndex != 0) {
            zEffect::SpawnRuntimeInstanceAt(ownerEntry->fireFxEffectTemplateIndex, trailRuntimeState->spawnPos);
        } else if (ownerEntry->fireFxAnimationEntries[0] != 0) {
            const zVec3* const spawnPos = trailRuntimeState->spawnPos;
            const zVec3* const spawnDir = trailRuntimeState->spawnDir;
            float randomRoll;
            if ((ownerEntry->fireFxFlags & 1u) != 0) {
                randomRoll = (((float)(rand()) * 0.0000305185094f) - 0.5f) * 3.14159265f;
            } else {
                randomRoll = 0.0f;
            }

            ownerEntry->trailEffectAnim = zEffectAnim::SetTransformRotAndVelocityThunk(
                ownerEntry->fireFxAnimationEntries[0],
                0,
                spawnPos->x,
                spawnPos->y,
                spawnPos->z,
                (float)asin((double)spawnDir->y),
                (float)(atan2(-spawnDir->x, -spawnDir->z)),
                randomRoll,
                0.0f,
                0.0f,
                0.0f
            );
        } else {
            ownerEntry->trailEffectAnim = 0;
        }

        trailRuntimeState->trailDistance = 0.0f;
        trailRuntimeState->volumeFadeTimer = 0.0f;
        trailRuntimeState->alphaPulsePhase = 0.0f;
        trailRuntimeState->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        if ((ownerEntry->flags & kOptCatalogFlagTrailUsePendingSpawnTargets) != 0) {
            trailRuntimeState->pendingSpawnTargetCountPtr = g_OptCatalogPendingSpawnTargetCountPtr;
            trailRuntimeState->pendingSpawnTargetListPtr = g_OptCatalogPendingSpawnTargetListPtr;
            g_OptCatalogPendingSpawnTargetCountPtr = 0;
        }

        if ((ownerEntry->flags & kOptCatalogFlagTrailStartMutedAndLight) != 0) {
            trailRuntimeState->lightNode
                = CZLight::AllocFromFreeListAndAttach(&ownerEntry->timedStatusLightSpecularColor);
            CZLight::gwLightSetRange(
                trailRuntimeState->lightNode,
                ownerEntry->timedStatusLightRangeMin,
                ownerEntry->timedStatusLightRangeMax
            );
            CZClass::gwNodeSetActive(trailRuntimeState->lightNode, 0);
        }

        if (ownerEntry->activeTrailRuntime != 0) {
            ownerEntry->activeTrailRuntime->prev = trailRuntimeState;
        }
        trailRuntimeState->prev = 0;
        trailRuntimeState->next = ownerEntry->activeTrailRuntime;
        ownerEntry->activeTrailRuntime = trailRuntimeState;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-deactivatetrailruntimestate
     * @recoil-artifact defines .text recoil:function:0x4aefb0: OptCatalog::DeactivateTrailRuntimeState
     * @recoil-match byte
     *
     * Purpose: stop trail runtime resources, unlink the active trail state,
     * return any glow light, and deactivate live trail segment nodes.
     */
    int __fastcall DeactivateTrailRuntimeState(OptCatalogTrailRuntimeState * trailRuntimeState)
    {
        zSndPlayHandle* const stopSoundHandle = trailRuntimeState->stopSoundHandle;
        OptCatalogEntryDef* const ownerEntry = trailRuntimeState->ownerEntry;

        if (stopSoundHandle != 0) {
            stopSoundHandle->StopIfActive();
        }

        zSndSample* const trailStopSample = ownerEntry->trailStopSample;
        if (trailStopSample != 0) {
            trailStopSample->PlayA3DSimple(1.0f);
        }

        zEffectAnimEntry* const trailEffectAnim = ownerEntry->trailEffectAnim;
        if (trailEffectAnim != 0) {
            zEffectAnim::Stop(trailEffectAnim);
            ownerEntry->trailEffectAnim = 0;
        }

        OptCatalogTrailRuntimeState* const next = trailRuntimeState->next;
        if (next != 0) {
            next->prev = trailRuntimeState->prev;
        }

        OptCatalogTrailRuntimeState* const prev = trailRuntimeState->prev;
        if (prev != 0) {
            prev->next = trailRuntimeState->next;
        }

        if (trailRuntimeState == ownerEntry->activeTrailRuntime) {
            ownerEntry->activeTrailRuntime = trailRuntimeState->next;
        }

        CZNodePartial* const lightNode = trailRuntimeState->lightNode;
        trailRuntimeState->prev = 0;
        trailRuntimeState->next = 0;
        if (lightNode != 0) {
            CZLight::ReturnToFreeList(lightNode);
        }

        for (int i = 0; i < trailRuntimeState->activeNodeSlotCount; ++i) {
            CZNodePartial* const node = trailRuntimeState->activeNodeSlots[i].node;
            if (node != 0) {
                CZClass::gwNodeSetActive(node, 0);
            }
        }

        trailRuntimeState->activeNodeSlotCursor = 0;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-processruntimeinstances
     * @recoil-artifact defines .text recoil:function:0x4af060: OptCatalog::ProcessRuntimeInstances
     *
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: drains queued impact callbacks, stores unscaled delta/time,
     * walks every loaded OptCatalog entry, updates trail-runtime segment
     * visuals and projectile runtime instances, recycles expired instances,
     * handles lock-on warning audio, and restores the packed variant tag.
     * Data touch: reads/writes g_OptCatalogQueuedImpactCount at 0x77896c,
     * g_OptCatalogRuntimeDeltaTime at 0x56bca8, g_OptCatalogRuntimeNowSec at
     * 0x56bcac, and lock-on warning gate state.
     * Purpose: frame-update all active OptCatalog runtime state.
     */
    void __cdecl ProcessRuntimeInstances()
    {
        unsigned int savedPackedVariantTag = 0;
        memcpy(&savedPackedVariantTag, &g_Variant_CurrentTag, sizeof(savedPackedVariantTag));
        float nearestLockOnDistance = (float)(_HUGE);

        g_OptCatalogRuntimeDeltaTime = g_Time_UnscaledDeltaTimeSec;
        g_OptCatalogRuntimeNowSec = g_Time_UnscaledAccumulatedTimeSec;

        while (g_OptCatalogQueuedImpactCount != 0) {
            --g_OptCatalogQueuedImpactCount;
            OptCatalogQueuedImpactRecord* const record = &g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount];
            InvokeDamageFeedbackAndHitCallback(
                record->entry,
                record->ownerNode,
                &record->sourcePos,
                (OptCatalogHitEventPartial*)(void*)(&record->hit),
                record->damageAmount
            );
        }

        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef* const entry = &g_OptCatalog_EntryTable[i];
            if (entry->keyName == 0) {
                continue;
            }

            if ((entry->flags & kOptCatalogFlagTrailRuntime) == 0) {
                OptCatalogRuntimeInstanceStorage** link = &entry->activeRuntimeListHead;
                OptCatalogRuntimeInstanceStorage* runtimeInstance = entry->activeRuntimeListHead;
                while (runtimeInstance != 0) {
                    OptCatalogRuntimeInstanceStorage* const nextRuntime = runtimeInstance->next;
                    int updateState = 1;
                    zVec3 movementDelta = { 0 };
                    zVec3 endPoint = runtimeInstance->pos;

                    if (runtimeInstance->updateCallback != 0) {
                        OptCatalogRuntimeUpdateCallback callback
                            = (OptCatalogRuntimeUpdateCallback)(runtimeInstance->updateCallback);
                        callback(runtimeInstance);
                    }

                    if ((unsigned char)(runtimeInstance->variantTag & 0xffu) == 4) {
                        memcpy(&g_Variant_CurrentTag, &savedPackedVariantTag, sizeof(g_Variant_CurrentTag));
                    } else {
                        memcpy(&g_Variant_CurrentTag, &runtimeInstance->variantTag, sizeof(g_Variant_CurrentTag));
                    }

                    if ((entry->flags & kOptCatalogFlagImpactWhenScaleExpired) != 0
                        && runtimeInstance->projectileScale <= 0.0f) {
                        HandleImpactEventFromRuntimeState(entry, runtimeInstance);
                        updateState = 0;
                    } else {
                        if (runtimeInstance->lifetime != 0.0f) {
                            if ((entry->flags & kOptCatalogFlagLockOn) != 0 && runtimeInstance->pendingTargetA != 0) {
                                zVec3 targetDirection;
                                const float targetDistance = zMath::Vec3DirectionTo(
                                    &runtimeInstance->pos,
                                    (zVec3*)(runtimeInstance->pendingTargetA),
                                    &targetDirection
                                );
                                if ((entry->flags & kOptCatalogFlagLockOnLead) != 0
                                    && runtimeInstance->pendingTargetB != 0) {
                                    zMath::LineVsSphereHit(
                                        &runtimeInstance->pos,
                                        (zVec3*)(runtimeInstance->pendingTargetA),
                                        runtimeInstance->lifetime,
                                        (zVec3*)(runtimeInstance->pendingTargetB),
                                        &targetDirection
                                    );
                                }

                                float turnBlend;
                                if (runtimeInstance->spawnGateAccum < entry->turnSuspendTime) {
                                    turnBlend = 0.0f;
                                } else if (entry->lockOnTime + entry->turnSuspendTime <= entry->lockOnTime) {
                                    turnBlend = 1.0f;
                                } else {
                                    turnBlend = (runtimeInstance->spawnGateAccum - entry->turnSuspendTime)
                                        / entry->lockOnTime;
                                }

                                if ((entry->flags & kOptCatalogFlagTetherGuided) != 0) {
                                    PlayerProbeSampleCandidateBuffer groundPick;
                                    CZDisplayInstance::FindBestPickCandidateBelowPoint(
                                        g_OptCatalogRuntimeWorld,
                                        (zVec3*)(runtimeInstance->pendingTargetA),
                                        &groundPick
                                    );
                                    if (groundPick.candidateCount == 1) {
                                        const float tetherHeight
                                            = runtimeInstance->pos.y - groundPick.entries[0].hitPos.y;
                                        if (tetherHeight > g_zWeapon_MaxTetherAltitude - 5.0f
                                            && targetDirection.y > 0.0f) {
                                            targetDirection.y = 0.0f;
                                        }
                                        BlendDirectionTowardTarget(
                                            &runtimeInstance->dir,
                                            &targetDirection,
                                            entry->turnRate * turnBlend * g_OptCatalogRuntimeDeltaTime,
                                            entry->turnRate * turnBlend * g_OptCatalogRuntimeDeltaTime,
                                            entry->pitchRate * turnBlend * g_OptCatalogRuntimeDeltaTime
                                        );
                                    } else {
                                        runtimeInstance->rangeProgress = entry->range;
                                    }
                                } else {
                                    const float turnStep = entry->turnRate * turnBlend * g_OptCatalogRuntimeDeltaTime;
                                    const float directionDot = runtimeInstance->dir.x * targetDirection.x
                                        + runtimeInstance->dir.y * targetDirection.y
                                        + runtimeInstance->dir.z * targetDirection.z;
                                    float turnAngle;
                                    if (directionDot >= 1.0f) {
                                        turnAngle = 0.0f;
                                    } else if (directionDot <= -1.0f) {
                                        turnAngle = (float)(kOptCatalogPi);
                                    } else {
                                        turnAngle = (float)(acos(directionDot));
                                        while (turnAngle < 0.0f) {
                                            turnAngle += 6.28318548f;
                                        }
                                        while (turnAngle >= 6.28318548f) {
                                            turnAngle -= 6.28318548f;
                                        }
                                        if (turnAngle > (float)(kOptCatalogPi)) {
                                            turnAngle = 6.28318548f - turnAngle;
                                        }
                                    }

                                    if (turnAngle > 0.0f) {
                                        float slerpAmount = turnStep;
                                        if (slerpAmount > turnAngle) {
                                            slerpAmount = turnAngle;
                                        }
                                        zMath::Vec3Slerp(
                                            &runtimeInstance->dir,
                                            &targetDirection,
                                            slerpAmount / turnAngle,
                                            &runtimeInstance->dir
                                        );
                                        zMath::Vec3Normalize(&runtimeInstance->dir);
                                    }
                                }

                                if (runtimeInstance->spawnGateAccum > entry->lockOnTime) {
                                    runtimeInstance->velocity.x = runtimeInstance->dir.x * runtimeInstance->lifetime;
                                    runtimeInstance->velocity.y = runtimeInstance->dir.y * runtimeInstance->lifetime;
                                    runtimeInstance->velocity.z = runtimeInstance->dir.z * runtimeInstance->lifetime;
                                } else {
                                    const float retainedVelocity
                                        = (entry->lockOnTime - runtimeInstance->spawnGateAccum) / entry->lockOnTime;
                                    runtimeInstance->spawnGateAccum += g_OptCatalogRuntimeDeltaTime;
                                    runtimeInstance->velocity.x = runtimeInstance->dir.x * runtimeInstance->lifetime
                                        + runtimeInstance->aux.x * retainedVelocity;
                                    runtimeInstance->velocity.y = runtimeInstance->dir.y * runtimeInstance->lifetime
                                        + runtimeInstance->aux.y * retainedVelocity;
                                    runtimeInstance->velocity.z = runtimeInstance->dir.z * runtimeInstance->lifetime
                                        + runtimeInstance->aux.z * retainedVelocity;
                                }

                                CZObject3D::gwObject3DSetRotation(
                                    runtimeInstance->projectileNode,
                                    (float)(asin(runtimeInstance->dir.y)),
                                    (float)(atan2(-runtimeInstance->dir.x, -runtimeInstance->dir.z)),
                                    0.0f
                                );
                                if (targetDistance < nearestLockOnDistance) {
                                    nearestLockOnDistance = targetDistance;
                                }
                            }

                            int accelerated = 0;
                            if ((entry->flags & kOptCatalogFlagFullProbeDamage) == 0
                                && runtimeInstance->lifetime < runtimeInstance->speed) {
                                runtimeInstance->lifetime += entry->acceleration * g_OptCatalogRuntimeDeltaTime;
                                if (runtimeInstance->lifetime > runtimeInstance->speed) {
                                    runtimeInstance->lifetime = runtimeInstance->speed;
                                }
                                accelerated = 1;
                            }
                            if (entry->gravity != 0.0f) {
                                runtimeInstance->velocity.y -= entry->gravity * g_OptCatalogRuntimeDeltaTime;
                            }
                            if (accelerated != 0) {
                                zMath::Vec3ScaleAdd(
                                    &runtimeInstance->velocity,
                                    &runtimeInstance->dir,
                                    runtimeInstance->lifetime,
                                    &runtimeInstance->velocity
                                );
                            }

                            if ((entry->flags & kOptCatalogFlagInstant) != 0) {
                                movementDelta.x = runtimeInstance->dir.x * entry->range;
                                movementDelta.y = runtimeInstance->dir.y * entry->range;
                                movementDelta.z = runtimeInstance->dir.z * entry->range;
                            } else {
                                movementDelta.x = runtimeInstance->velocity.x * g_OptCatalogRuntimeDeltaTime;
                                movementDelta.y = runtimeInstance->velocity.y * g_OptCatalogRuntimeDeltaTime;
                                movementDelta.z = runtimeInstance->velocity.z * g_OptCatalogRuntimeDeltaTime;
                                const float movementLength = (float)(sqrt(
                                    movementDelta.x * movementDelta.x + movementDelta.y * movementDelta.y
                                    + movementDelta.z * movementDelta.z
                                ));
                                runtimeInstance->rangeProgress += movementLength;
                                if ((entry->flags & kOptCatalogFlagFullProbeDamage) != 0) {
                                    runtimeInstance->rangeProgress *= 0.5f;
                                }
                                if (runtimeInstance->rangeProgress >= entry->range) {
                                    if ((entry->flags & kOptCatalogFlagLockOn) != 0
                                        && (entry->flags & kOptCatalogFlagExpires) == 0) {
                                        HandleImpactEventFromRuntimeState(entry, runtimeInstance);
                                    }
                                    updateState = 0;
                                } else if ((entry->flags & kOptCatalogFlagLockOn) != 0
                                    && runtimeInstance->pendingTargetA != 0 && entry->detonationDistSq != 0.0f
                                    && zMath::Vec3DeltaLengthSq(
                                           &runtimeInstance->pos,
                                           (zVec3*)(runtimeInstance->pendingTargetA)
                                       ) <= entry->detonationDistSq) {
                                    updateState = 0;
                                    g_OptCatalog_FallbackImpactProbeEnabled = 0;
                                    RemoveRuntimeInstance(entry, &runtimeInstance->pos, 0);
                                    g_OptCatalog_FallbackImpactProbeEnabled = 1;
                                }
                            }
                        } else if ((entry->flags & kOptCatalogFlagFullProbeDamage) != 0) {
                            OptCatalogRaycastHitList mineHits;
                            int foundMineImpact = 0;
                            updateState = 2;
                            if (g_OptCatalogNetworkOptionState != 0
                                && g_OptCatalogRuntimeNowSec > runtimeInstance->spawnGateAccum) {
                                BuildImpactHitList(entry, runtimeInstance, 0, &mineHits);
                                foundMineImpact = 1;
                            } else if ((entry->flags & kOptCatalogFlagRemoteDetonate) == 0
                                && BuildImpactHitList(entry, runtimeInstance, 0, &mineHits) != 0) {
                                foundMineImpact = 1;
                            }
                            if (foundMineImpact != 0) {
                                HandleImpactFromRuntimeProbe(entry, runtimeInstance, &mineHits, 0);
                                updateState = 0;
                                g_OptCatalog_FallbackImpactProbeEnabled = 0;
                                ProcessRuntimeInstance(entry, runtimeInstance);
                                g_OptCatalog_FallbackImpactProbeEnabled = 1;
                            } else {
                                CZObject3D::gwObject3DTranslateRotation(
                                    runtimeInstance->projectileNode,
                                    0.0f,
                                    3.4906585f * g_OptCatalogRuntimeDeltaTime,
                                    0.0f
                                );
                            }
                        }
                    }

                    if (updateState == 1) {
                        endPoint.x = runtimeInstance->pos.x + movementDelta.x;
                        endPoint.y = runtimeInstance->pos.y + movementDelta.y;
                        endPoint.z = runtimeInstance->pos.z + movementDelta.z;
                        SetDamageMaskSlotIndex(entry->damageMaskSlotIndex);
                        CZClass::gwNodeSetRaycastable(runtimeInstance->ownerNode, 0);
                        if ((entry->flags & kOptCatalogFlagImpactWhenScaleExpired) != 0) {
                            CZClass::gwNodeSetRaycastable(runtimeInstance->projectileNode, 0);
                        }
                        CZDisplayInstance::SetStopAfterFirstHit(0x40000);
                        PlayerProbeSampleCandidateBuffer segmentHits;
                        if (CZDisplayInstance::RaycastSelectClosestHitBetweenPoints(
                                g_OptCatalogRuntimeWorld,
                                &runtimeInstance->pos,
                                &endPoint,
                                &segmentHits
                            )
                            == 0) {
                            zClassDiPickCandidateEntry* candidate = &segmentHits.entries[segmentHits.candidateCount];
                            OptCatalogHitEventPartial* hitEvent = (OptCatalogHitEventPartial*)(void*)(candidate);
                            OptCatalogRaycastHitEntry* rayHit = (OptCatalogRaycastHitEntry*)(void*)(candidate);
                            void* excludedDamageHandler = ((CZNodeFreeListSlot*)(candidate->node))->damageHandler;
                            if ((entry->flags & kOptCatalogFlagFullProbeDamage) == 0) {
                                if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
                                    g_OptCatalog_CapturedDamageSourcePos = runtimeInstance->pos;
                                    g_OptCatalog_CapturedDamageHitPos = candidate->hitPos;
                                }
                                runtimeInstance->pos = candidate->hitPos;
                                updateState = 0;
                                g_OptCatalog_CaptureHitSnapshotEnabled = 0;
                                HandleImpactEvent(entry, hitEvent, runtimeInstance);
                                if (entry->impactProximity > 0.0f) {
                                    OptCatalogRaycastHitList proximityHits;
                                    if (BuildImpactHitList(entry, runtimeInstance, 1, &proximityHits) != 0) {
                                        HandleImpactFromRuntimeProbe(
                                            entry,
                                            runtimeInstance,
                                            &proximityHits,
                                            excludedDamageHandler
                                        );
                                    }
                                }
                                g_OptCatalog_CaptureHitSnapshotEnabled = 1;
                            } else {
                                for (;;) {
                                    int impactSlot = 0;
                                    if (rayHit->surfaceRef != 0) {
                                        impactSlot = rayHit->surfaceRef->impactSlot;
                                    }
                                    float rayLength = 0.0f;
                                    float reflectedLength = 0.0f;
                                    const int response = CanSpawnThroughRay(
                                        entry,
                                        rayHit,
                                        &runtimeInstance->pos,
                                        &endPoint,
                                        &rayLength,
                                        &reflectedLength,
                                        &runtimeInstance->dir
                                    );
                                    if (response != 0 && runtimeInstance->lifetime > 0.2f) {
                                        const float oldMagnitude = runtimeInstance->lifetime;
                                        runtimeInstance->lifetime *= 0.25f;
                                        reflectedLength *= 0.5f;
                                        runtimeInstance->velocity.x
                                            = runtimeInstance->dir.x * runtimeInstance->lifetime;
                                        runtimeInstance->velocity.y
                                            = runtimeInstance->dir.y * runtimeInstance->lifetime;
                                        runtimeInstance->velocity.z
                                            = runtimeInstance->dir.z * runtimeInstance->lifetime;
                                        zMath::Vec3ScaleAdd(
                                            &rayHit->pos,
                                            &runtimeInstance->dir,
                                            reflectedLength,
                                            &endPoint
                                        );
                                        PlayBounceSound(
                                            entry,
                                            rayHit,
                                            impactSlot,
                                            (oldMagnitude / runtimeInstance->speed + 1.0f) * 0.5f
                                        );
                                        SetDamageMaskSlotIndex(entry->damageMaskSlotIndex);
                                        CZDisplayInstance::SetStopAfterFirstHit(0x40000);
                                        PlayerProbeSampleCandidateBuffer reflectedHits;
                                        if (CZDisplayInstance::RaycastSelectClosestHitBetweenPoints(
                                                g_OptCatalogRuntimeWorld,
                                                &rayHit->pos,
                                                &endPoint,
                                                &reflectedHits
                                            )
                                            != 0) {
                                            break;
                                        }
                                        runtimeInstance->pos = rayHit->pos;
                                        candidate = &reflectedHits.entries[reflectedHits.candidateCount];
                                        hitEvent = (OptCatalogHitEventPartial*)(void*)(candidate);
                                        rayHit = (OptCatalogRaycastHitEntry*)(void*)(candidate);
                                        continue;
                                    }

                                    if (response == 0) {
                                        updateState = 0;
                                        HandleImpactEvent(entry, hitEvent, runtimeInstance);
                                    } else {
                                        runtimeInstance->lifetime = 0.0f;
                                        runtimeInstance->rangeProgress = entry->range;
                                        endPoint = rayHit->pos;
                                        updateState = 2;
                                        CZObject3D::gwObject3DTranslateRotation(
                                            runtimeInstance->projectileNode,
                                            0.0f,
                                            3.4906585f * g_OptCatalogRuntimeDeltaTime,
                                            0.0f
                                        );
                                        CZObject3D::gwObject3DSetScale(
                                            runtimeInstance->projectileNode,
                                            5.0f,
                                            5.0f,
                                            5.0f
                                        );
                                    }
                                    break;
                                }
                            }
                        }
                        if ((entry->flags & kOptCatalogFlagImpactWhenScaleExpired) != 0) {
                            CZClass::gwNodeSetRaycastable(runtimeInstance->projectileNode, 1);
                        }
                        CZClass::gwNodeSetRaycastable(runtimeInstance->ownerNode, 1);
                    }

                    if ((entry->flags & kOptCatalogFlagInstant) != 0) {
                        updateState = 0;
                    }

                    if (updateState != 0) {
                        if (updateState == 1) {
                            if ((entry->flags & kOptCatalogFlagFullProbeDamage) == 0) {
                                if (entry->gravity != 0.0f && (entry->flags & kOptCatalogFlagFixedRotate) == 0) {
                                    zVec3 direction = runtimeInstance->velocity;
                                    zMath::Vec3Normalize(&direction);
                                    CZObject3D::gwObject3DSetRotation(
                                        runtimeInstance->projectileNode,
                                        (float)(asin(direction.y)),
                                        (float)(atan2(-direction.x, -direction.z)),
                                        0.0f
                                    );
                                }
                            } else {
                                if (runtimeInstance->scaleFade <= 0.0f) {
                                    CZObject3D::gwObject3DTranslateRotation(
                                        runtimeInstance->projectileNode,
                                        0.0f,
                                        3.4906585f * g_OptCatalogRuntimeDeltaTime,
                                        0.0f
                                    );
                                }
                                if (runtimeInstance->scaleFade < 1.0f) {
                                    const float scale = 1.0f - runtimeInstance->scaleFade * -4.0f;
                                    CZObject3D::gwObject3DSetScale(
                                        runtimeInstance->projectileNode,
                                        scale,
                                        scale,
                                        scale
                                    );
                                    runtimeInstance->scaleFade += g_OptCatalogRuntimeDeltaTime;
                                }
                            }
                            runtimeInstance->pos = endPoint;
                            CZObject3D::gwObject3DSetPosition(
                                runtimeInstance->projectileNode,
                                runtimeInstance->pos.x,
                                runtimeInstance->pos.y,
                                runtimeInstance->pos.z
                            );
                        }
                        link = &runtimeInstance->next;
                    } else {
                        *link = nextRuntime;
                        runtimeInstance->lifetime = 0.0f;
                        if (runtimeInstance->flyoutAnimPrimary != 0) {
                            zEffect_Anim::NodeActionCallback(runtimeInstance->flyoutAnimPrimary, 0);
                            runtimeInstance->flyoutAnimPrimary = 0;
                        }
                        if (runtimeInstance->flyoutAnimSecondary != 0) {
                            zEffect_Anim::NodeActionCallback(runtimeInstance->flyoutAnimSecondary, 0);
                            runtimeInstance->flyoutAnimSecondary = 0;
                        }
                        CZClass::RemoveChild(g_OptCatalogRuntimeWorld, runtimeInstance->projectileNode);
                        if ((entry->flags & kOptCatalogFlagTetherGuided) == 0) {
                            RecycleRuntimeInstanceStorage(entry, runtimeInstance);
                        } else {
                            runtimeInstance->ownerNode = 0;
                        }
                    }

                    runtimeInstance = *link;
                }
            } else {
                OptCatalogTrailRuntimeState* trailRuntime = entry->activeTrailRuntime;
                while (trailRuntime != 0) {
                    OptCatalogTrailRuntimeState* const nextTrailRuntime = trailRuntime->next;
                    if (trailRuntime->variantTagPtr != 0) {
                        g_Variant_CurrentTag = *trailRuntime->variantTagPtr;
                    } else {
                        memcpy(&g_Variant_CurrentTag, &savedPackedVariantTag, sizeof(g_Variant_CurrentTag));
                    }

                    OptCatalogTrailNodeSlot* const segments = trailRuntime->activeNodeSlots;
                    int visibleSegmentCount = 0;
                    int targetCount = 0;
                    if (trailRuntime->pendingSpawnTargetCountPtr != 0) {
                        targetCount = *trailRuntime->pendingSpawnTargetCountPtr;
                    }
                    if (targetCount > 8) {
                        targetCount = 8;
                    }

                    if (trailRuntime->spawnPos != 0 && trailRuntime->spawnDir != 0
                        && trailRuntime->activeNodeSlotCount > 0) {
                        segments[0].pos = *trailRuntime->spawnPos;
                        segments[0].dir = *trailRuntime->spawnDir;

                        if ((entry->flags & 0x10000u) != 0 && trailRuntime->pendingSpawnTargetListPtr != 0) {
                            if (targetCount > 1) {
                                float targetProjectionScratch[8];
                                zVec3 sortedDirection;
                                ReflectAndSortImpactTraceList(trailRuntime, targetProjectionScratch, &sortedDirection);
                                if (targetCount > 4) {
                                    targetCount = 4;
                                }

                                zVec3 cursor = *trailRuntime->spawnPos;
                                int targetIndex;
                                for (targetIndex = 0; targetIndex < targetCount; ++targetIndex) {
                                    targetProjectionScratch[targetIndex] *= 0.4f;
                                    segments[targetIndex].pos = cursor;
                                    zMath::Vec3ScaleAdd(
                                        &segments[targetIndex].pos,
                                        &sortedDirection,
                                        targetProjectionScratch[targetIndex],
                                        &cursor
                                    );
                                }

                                const float jitter = targetProjectionScratch[targetCount - 1] * 0.1f;
                                for (targetIndex = 1; targetIndex < targetCount; ++targetIndex) {
                                    segments[targetIndex].pos.x += ((float)(rand()) * 0.0000305185094f - 0.5f) * jitter;
                                    segments[targetIndex].pos.z += ((float)(rand()) * 0.0000305185094f - 0.5f) * jitter;
                                }

                                int stopped = 0;
                                for (targetIndex = 0; targetIndex < targetCount - 1; ++targetIndex) {
                                    OptCatalogTrailNodeSlot* const segment = &segments[targetIndex];
                                    segment->scale = zMath::Vec3DirectionTo(
                                        &segment->pos,
                                        &segments[targetIndex + 1].pos,
                                        &segment->dir
                                    );
                                    stopped = ComputeTrailImpactResponse(
                                        entry,
                                        trailRuntime,
                                        segment,
                                        &segments[targetIndex + 1].pos
                                    );
                                    UpdateTrailSegmentVisual(segment);
                                    ++visibleSegmentCount;
                                    if (stopped != 0) {
                                        break;
                                    }
                                }

                                if (stopped == 0) {
                                    OptCatalogTrailNodeSlot* const segment = &segments[visibleSegmentCount];
                                    zVec3* const penultimateTarget
                                        = trailRuntime->pendingSpawnTargetListPtr[visibleSegmentCount].targetPos;
                                    segment->scale
                                        = zMath::Vec3DirectionTo(&segment->pos, penultimateTarget, &segment->dir);
                                    zVec3* const finalTarget
                                        = trailRuntime->pendingSpawnTargetListPtr[targetCount - 1].targetPos;
                                    segment->scale = zMath::Vec3DirectionTo(&segment->pos, finalTarget, &segment->dir);
                                    stopped = ComputeTrailImpactResponse(entry, trailRuntime, segment, finalTarget);
                                    UpdateTrailSegmentVisual(segment);
                                    ++visibleSegmentCount;
                                }

                                if (stopped == 0 && visibleSegmentCount < targetCount) {
                                    OptCatalogTrailNodeSlot* const segment = &segments[visibleSegmentCount];
                                    zVec3* const finalTarget
                                        = trailRuntime->pendingSpawnTargetListPtr[targetCount - 1].targetPos;
                                    segment->scale = zMath::Vec3DirectionTo(&segment->pos, finalTarget, &segment->dir);
                                    ComputeTrailImpactResponse(entry, trailRuntime, segment, finalTarget);
                                    UpdateTrailSegmentVisual(segment);
                                    ++visibleSegmentCount;
                                }
                            } else if (targetCount == 1) {
                                zVec3* const targetPos = trailRuntime->pendingSpawnTargetListPtr[0].targetPos;
                                segments[0].scale
                                    = 0.5f * zMath::Vec3DirectionTo(&segments[0].pos, targetPos, &segments[0].dir);
                                zMath::Vec3ScaleAdd(
                                    &segments[0].pos,
                                    &segments[0].dir,
                                    segments[0].scale,
                                    &segments[1].pos
                                );
                                const float jitter = segments[0].scale * 0.2f;
                                segments[1].pos.x += ((float)(rand()) * 0.0000305185094f - 0.5f) * jitter;
                                segments[1].pos.z += ((float)(rand()) * 0.0000305185094f - 0.5f) * jitter;
                                segments[0].scale
                                    = zMath::Vec3DirectionTo(&segments[0].pos, &segments[1].pos, &segments[0].dir);
                                const int stopped
                                    = ComputeTrailImpactResponse(entry, trailRuntime, &segments[0], &segments[1].pos);
                                UpdateTrailSegmentVisual(&segments[0]);
                                visibleSegmentCount = 1;
                                if (stopped == 0) {
                                    segments[1].scale
                                        = zMath::Vec3DirectionTo(&segments[1].pos, targetPos, &segments[1].dir);
                                    ComputeTrailImpactResponse(entry, trailRuntime, &segments[1], targetPos);
                                    UpdateTrailSegmentVisual(&segments[1]);
                                    visibleSegmentCount = 2;
                                }
                            } else {
                                zVec3 endPoint;
                                segments[0].scale = entry->range;
                                zMath::Vec3ScaleAdd(&segments[0].pos, &segments[0].dir, segments[0].scale, &endPoint);
                                ComputeTrailImpactResponse(entry, trailRuntime, &segments[0], &endPoint);
                                UpdateTrailSegmentVisual(&segments[0]);
                                visibleSegmentCount = 1;
                            }
                        } else {
                            if (trailRuntime->trailDistance != entry->range) {
                                const float remainingDistance = entry->range - trailRuntime->trailDistance;
                                if (remainingDistance > 0.1) {
                                    trailRuntime->trailDistance
                                        += entry->velocity * remainingDistance * g_OptCatalogRuntimeDeltaTime;
                                } else {
                                    trailRuntime->trailDistance = entry->range;
                                }
                            }
                            trailRuntime->alphaPulsePhase += g_OptCatalogRuntimeDeltaTime * 15.707963f;

                            zVec3* rayStart = trailRuntime->spawnPos;
                            zVec3* rayDirection = trailRuntime->spawnDir;
                            zVec3 reflectedDirection;
                            float travelledDistance = 0.0f;
                            int continueReflection;
                            PlayerProbeSampleCandidateBuffer trailRayHits;
                            do {
                                OptCatalogTrailNodeSlot* const segment = &segments[visibleSegmentCount];
                                CZClass::gwNodeSetActive(segment->node, 1);
                                CZObject3D::gwObject3DSetPosition(segment->node, rayStart->x, rayStart->y, rayStart->z);
                                CZObject3D::gwObject3DSetRotation(
                                    segment->node,
                                    (float)(asin(rayDirection->y)),
                                    (float)(atan2(-rayDirection->x, -rayDirection->z)),
                                    0.0f
                                );
                                CZObject3D::gwObject3DSetAlphaScale(
                                    segment->node,
                                    1.0f - (float)(sin(trailRuntime->alphaPulsePhase)) * -0.25f
                                );

                                float rayLength = trailRuntime->trailDistance - travelledDistance;
                                zMath::Vec3ScaleAdd(rayStart, rayDirection, rayLength, &segment->pos);
                                SetDamageMaskSlotIndex(entry->damageMaskSlotIndex);
                                CZDisplayInstance::SetStopAfterFirstHit(0x40000);
                                if (visibleSegmentCount == 1) {
                                    CZClass::gwNodeSetRaycastable(trailRuntime->projectileNode, 0);
                                }
                                const int rayResult = CZDisplayInstance::RaycastSelectClosestHitBetweenPoints(
                                    g_OptCatalogRuntimeWorld,
                                    rayStart,
                                    &segment->pos,
                                    &trailRayHits
                                );
                                if (visibleSegmentCount == 1) {
                                    CZClass::gwNodeSetRaycastable(trailRuntime->projectileNode, 1);
                                }

                                continueReflection = 0;
                                if (rayResult == 0) {
                                    zClassDiPickCandidateEntry* const candidate
                                        = &trailRayHits.entries[trailRayHits.candidateCount];
                                    OptCatalogRaycastHitEntry* const hit
                                        = (OptCatalogRaycastHitEntry*)(void*)(candidate);
                                    float reflectedLength = 0.0f;
                                    const int spawnResult = CanSpawnThroughRay(
                                        entry,
                                        hit,
                                        rayStart,
                                        &segment->pos,
                                        &rayLength,
                                        &reflectedLength,
                                        &reflectedDirection
                                    );
                                    travelledDistance += rayLength;
                                    if (spawnResult == 0) {
                                        if ((entry->flags & 0x800u) != 0) {
                                            trailRuntime->volumeFadeTimer += g_OptCatalogRuntimeDeltaTime;
                                            if (trailRuntime->volumeFadeTimer < entry->detonationDistSq) {
                                                trailRuntime->stopSoundHandle->SetFreqScaled(
                                                    trailRuntime->volumeFadeTimer / entry->detonationDistSq
                                                );
                                                InvokeDamageFeedbackAndHitCallback(
                                                    entry,
                                                    trailRuntime->projectileNode,
                                                    rayStart,
                                                    (OptCatalogHitEventPartial*)(void*)(hit),
                                                    0.0f
                                                );
                                                CZClass::gwNodeSetActive(trailRuntime->lightNode, 1);
                                                CZLight::gwLightSetPosition(
                                                    trailRuntime->lightNode,
                                                    hit->pos.x,
                                                    hit->pos.y,
                                                    hit->pos.z
                                                );
                                            } else {
                                                OptCatalogRuntimeInstanceStorage impactRuntime = { 0 };
                                                impactRuntime.ownerNode = trailRuntime->projectileNode;
                                                impactRuntime.pos = *trailRuntime->spawnPos;
                                                impactRuntime.spawnScale = trailRuntime->spawnScale;
                                                HandleImpactEvent(
                                                    entry,
                                                    (OptCatalogHitEventPartial*)(void*)(hit),
                                                    &impactRuntime
                                                );
                                                trailRuntime->volumeFadeTimer = 0.0f;
                                                trailRuntime->stopSoundHandle->SetFreqScaled(0.0f);
                                            }
                                        } else {
                                            trailRuntime->trailBlend = ((float)(cos(
                                                                            (double)(trailRuntime->trailDistance)
                                                                            * kOptCatalogPi / entry->range
                                                                        )) + 1.0f)
                                                * 0.5f;
                                            if (trailRuntime->trailBlend < kOptCatalogTrailDamageBlendLimit) {
                                                trailRuntime->trailBlend = kOptCatalogTrailDamageBlendLimit;
                                            }
                                            InvokeDamageFeedbackAndHitCallback(
                                                entry,
                                                trailRuntime->projectileNode,
                                                rayStart,
                                                (OptCatalogHitEventPartial*)(void*)(hit),
                                                entry->damage * g_OptCatalogRuntimeDeltaTime * trailRuntime->trailBlend
                                            );
                                            trailRuntime->trailDistance = travelledDistance;
                                        }
                                    } else {
                                        if ((entry->flags & 0x800u) != 0) {
                                            trailRuntime->volumeFadeTimer -= g_OptCatalogRuntimeDeltaTime;
                                            float frequencyScale = 0.0f;
                                            if (trailRuntime->volumeFadeTimer >= 0.0f) {
                                                frequencyScale
                                                    = trailRuntime->volumeFadeTimer / entry->detonationDistSq;
                                            } else {
                                                trailRuntime->volumeFadeTimer = 0.0f;
                                            }
                                            trailRuntime->stopSoundHandle->SetFreqScaled(frequencyScale);
                                        }
                                        segment->pos = hit->pos;
                                        if (spawnResult == 1) {
                                            rayStart = &segment->pos;
                                            rayDirection = &reflectedDirection;
                                            continueReflection = 1;
                                        }
                                    }
                                } else if ((entry->flags & 0x800u) != 0) {
                                    trailRuntime->volumeFadeTimer = 0.0f;
                                    trailRuntime->stopSoundHandle->SetFreqScaled(0.0f);
                                    CZClass::gwNodeSetActive(trailRuntime->lightNode, 0);
                                }

                                CZObject3D::gwObject3DSetScale(segment->node, 1.0f, 1.0f, rayLength);
                                ++visibleSegmentCount;
                            } while (
                                continueReflection != 0 && visibleSegmentCount < trailRuntime->activeNodeSlotCount);
                        }
                    }

                    for (int segmentIndex = visibleSegmentCount; segmentIndex < trailRuntime->activeNodeSlotCursor;
                        ++segmentIndex) {
                        CZClass::gwNodeSetActive(trailRuntime->activeNodeSlots[segmentIndex].node, 0);
                    }
                    trailRuntime->activeNodeSlotCursor = visibleSegmentCount;

                    trailRuntime = nextTrailRuntime;
                }
            }
        }

        if (nearestLockOnDistance < (float)(_HUGE)
            && g_OptCatalogRuntimeNowSec >= g_OptCatalogLockOnWarningGateTimeSec) {
            g_OptCatalogSndLockOnWarning->PlayA3DSimple(1.0f);
            g_OptCatalogLockOnWarningGateTimeSec = g_OptCatalogRuntimeNowSec + 5.0f;
        }

        memcpy(&g_Variant_CurrentTag, &savedPackedVariantTag, sizeof(g_Variant_CurrentTag));
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-computeaimpitchfortarget
     * @recoil-artifact defines .text recoil:function:0x4b0530: OptCatalog::ComputeAimPitchForTarget
     *
     *
     * Purpose: Computes launch pitch to hit a target and writes the approximated target distance.
     */
    float __fastcall ComputeAimPitchForTarget(
        OptCatalogEntryDef * self,
        const zVec3* origin,
        const zVec3* unusedDirection,
        const zVec3* target,
        float* distanceApproxOut
    )
    {
        (void)unusedDirection;

        zVec3 delta;
        delta.x = target->x - origin->x;
        delta.y = target->y - origin->y;
        delta.z = target->z - origin->z;

        const float distanceSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        int distanceBits;
        memcpy(&distanceBits, &distanceSq, sizeof(distanceBits));
        distanceBits = (distanceBits >> 1) + (int)(kOptCatalogFastSqrtBias);

        float distanceApprox;
        memcpy(&distanceApprox, &distanceBits, sizeof(distanceApprox));
        *distanceApproxOut = distanceApprox;

        if (self->gravity == 0.0f) {
            return -1.0f;
        }

        const float verticalSlope = delta.y / distanceApprox;
        if (distanceApprox < self->range) {
            return verticalSlope - (distanceApprox / self->range) * kOptCatalogAimPitchRangeScale;
        }

        if ((self->flags & kOptCatalogFlagAllowOutOfRangeAimPitch) != 0) {
            return verticalSlope - kOptCatalogAimPitchRangeScale;
        }

        return -1.0f;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-playtriggerinactivewarning
     * @recoil-artifact defines .text recoil:function:0x4b0600: OptCatalog::PlayTriggerInactiveWarning
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: play the trigger-inactive warning sound at full gain.
     */
    void __cdecl PlayTriggerInactiveWarning()
    {
        g_OptCatalogSndTriggerInactive->PlayA3DSimple(1.0f);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-playweaponinactivewarning
     * @recoil-artifact defines .text recoil:function:0x4b0620: OptCatalog::PlayWeaponInactiveWarning
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: play the weapon-inactive warning sound at full gain.
     */
    void __cdecl PlayWeaponInactiveWarning()
    {
        g_OptCatalogSndWeaponInactive->PlayA3DSimple(1.0f);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-playnoammowarning
     * @recoil-artifact defines .text recoil:function:0x4b0640: OptCatalog::PlayNoAmmoWarning
     * @recoil-match byte
     *
     * BN source path: D:\Proj\Battlesport\OptCatalog.cpp.
     * Purpose: play the no-ammo warning sound at full gain.
     */
    void __cdecl PlayNoAmmoWarning()
    {
        g_OptCatalogSndNoAmmoWarning->PlayA3DSimple(1.0f);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-emitqsandimpactevent
     * @recoil-artifact defines .text recoil:function:0x4b0660: OptCatalog::EmitQSandImpactEvent
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: if the hit node accepts terrain deformation, builds a
     * quicksand event at the hit position, selects randomized or clamped
     * radius, and dispatches it through the quicksand net relay.
     * Data touch: reads g_OptCatalogMaxCraterRadius at 0x779a7c.
     * Purpose: emit a quicksand terrain-deformation event for an OptCatalog hit.
     */
    void __fastcall EmitQSandImpactEvent(
        OptCatalogEntryDef * self,
        OptCatalogHitEventPartial * hitEvent,
        CZNodePartial * unusedOwnerNode,
        CZNodePartial * damageOwnerNode
    )
    {
        (void)unusedOwnerNode;

        if ((hitEvent->hitNode->flags & kOptCatalogNodeFlagAcceptsTerrainDeformation) == 0) {
            return;
        }

        zDEClient_QSandEventTemplate eventTemplate;
        zDEClient::CopyQSandEventTemplateDefaults(&eventTemplate);

        eventTemplate.center = hitEvent->hitPos;
        if (self->craterRadiusRandomRange != 0) {
            const unsigned int radius = (unsigned int)(self->craterRadiusBase
                + ((unsigned int)(rand() * self->craterRadiusRandomRange) >> 15));
            eventTemplate.radius = (float)(radius);
        } else {
            const float radius = self->impactProximity * 0.5f;
            if (g_OptCatalogMaxCraterRadius < radius) {
                eventTemplate.radius = g_OptCatalogMaxCraterRadius;
            } else {
                eventTemplate.radius = radius;
            }
        }

        eventTemplate.damageOwnerNode = damageOwnerNode;
        zDEClient_QSand::InstanceEventMaybeRelay(&eventTemplate);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-emitcraterimpactevent
     * @recoil-artifact defines .text recoil:function:0x4b0710: OptCatalog::EmitCraterImpactEvent
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: if the hit node accepts terrain deformation, builds a
     * crater event at the hit position, selects randomized or clamped radius,
     * invokes the crater net relay, and returns 1 only when the relay does
     * not consume the impact.
     * Data touch: reads g_OptCatalogMaxCraterRadius at 0x779a7c.
     * Purpose: emit a crater terrain-deformation event for an OptCatalog hit.
     */
    int __fastcall EmitCraterImpactEvent(
        OptCatalogEntryDef * self,
        OptCatalogHitEventPartial * hitEvent,
        CZNodePartial * unusedOwnerNode,
        CZNodePartial * damageOwnerNode
    )
    {
        (void)unusedOwnerNode;

        int result = 0;
        if ((hitEvent->hitNode->flags & kOptCatalogNodeFlagAcceptsTerrainDeformation) != 0) {
            zDEClient_CraterEventTemplate eventTemplate;
            zDEClient_Crater::InitEventTemplateDefaults(&eventTemplate);

            eventTemplate.craterMaterialSlot = (zModel_MaterialSlot*)(hitEvent->surfaceRef);
            eventTemplate.center = hitEvent->hitPos;
            if (self->craterRadiusRandomRange != 0) {
                const unsigned int radius = (unsigned int)(self->craterRadiusBase
                    + ((unsigned int)(rand() * self->craterRadiusRandomRange) >> 15));
                eventTemplate.radius = (float)(radius);
            } else {
                const float radius = self->impactProximity * 0.5f;
                if (g_OptCatalogMaxCraterRadius < radius) {
                    eventTemplate.radius = g_OptCatalogMaxCraterRadius;
                } else {
                    eventTemplate.radius = radius;
                }
            }

            eventTemplate.damageOwnerNode = damageOwnerNode;
            result = zDEClient_Crater::InstanceEventMaybeRelay(&eventTemplate) == 0 ? 1 : 0;
        }

        return result;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-handleimpactevent
     * @recoil-artifact defines .text recoil:function:0x4b07d0: OptCatalog::HandleImpactEvent
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef*, EDX is
     * OptCatalogHitEventPartial*, and the runtime instance is passed on the
     * stack. Reads the impact slot from the surface reference, invokes the
     * optional impact callback, scales damage by runtime spawnScale, dispatches
     * damage feedback, terrain impact events, impact sound, and fallback
     * animation/effect spawning according to entry flags and damage-context
     * state.
     * Purpose: apply all direct impact feedback for a runtime projectile hit.
     */
    void __fastcall HandleImpactEvent(
        OptCatalogEntryDef * self,
        OptCatalogHitEventPartial * hitEvent,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    )
    {
        int suppressFallbackFx = 0;
        int impactSlot;
        if (hitEvent->surfaceRef != 0) {
            impactSlot = hitEvent->surfaceRef->impactSlot;
        } else {
            impactSlot = 0;
        }

        if (self->impactCallback != 0) {
            self->impactCallback(self, hitEvent, runtimeInstance);
        }

        const float damageAmount = runtimeInstance->spawnScale * self->damage;
        int damageHandled = InvokeDamageFeedbackAndHitCallback(
            self,
            runtimeInstance->ownerNode,
            &runtimeInstance->pos,
            hitEvent,
            damageAmount
        );

        if ((self->flags & kOptCatalogFlagCraterImpact) != 0) {
            if ((self->flags & kOptCatalogFlagAlwaysPlayImpactFx) == 0) {
                suppressFallbackFx = 1;
            }
            suppressFallbackFx
                &= EmitCraterImpactEvent(self, hitEvent, (CZNodePartial*)(impactSlot), runtimeInstance->ownerNode);
        } else if ((self->flags & kOptCatalogFlagQuickSandImpact) != 0) {
            OptCatalogHitEventPartial* const contextHitEvent
                = (OptCatalogHitEventPartial*)(g_OptCatalog_DamageContextHitEvent);
            if (contextHitEvent != 0) {
                EmitQSandImpactEvent(
                    self,
                    contextHitEvent,
                    contextHitEvent->surfaceRef != 0 ? contextHitEvent->surfaceRef->impactOwnerNode : 0,
                    runtimeInstance->ownerNode
                );
            } else {
                EmitQSandImpactEvent(self, hitEvent, (CZNodePartial*)(impactSlot), runtimeInstance->ownerNode);
            }
        }

        if (g_OptCatalog_DamageContextKind != 0 && (self->flags & kOptCatalogFlagCraterImpact) != 0
            && g_OptCatalog_DamageContextHitEvent != 0) {
            OptCatalogHitEventPartial* const contextHitEvent
                = (OptCatalogHitEventPartial*)(g_OptCatalog_DamageContextHitEvent);
            EmitCraterImpactEvent(
                self,
                contextHitEvent,
                contextHitEvent->surfaceRef != 0 ? contextHitEvent->surfaceRef->impactOwnerNode : 0,
                runtimeInstance->ownerNode
            );
        }

        if ((self->flags & kOptCatalogFlagQuickSandImpact) != 0 && g_OptCatalog_DamageContextHitEvent != 0) {
            OptCatalogHitEventPartial* const contextHitEvent
                = (OptCatalogHitEventPartial*)(g_OptCatalog_DamageContextHitEvent);
            EmitQSandImpactEvent(
                self,
                contextHitEvent,
                contextHitEvent->surfaceRef != 0 ? contextHitEvent->surfaceRef->impactOwnerNode : 0,
                runtimeInstance->ownerNode
            );
            return;
        }

        PlayImpactSound(self, hitEvent, impactSlot, 1.0f);
        if (suppressFallbackFx == 0 && damageHandled == 0) {
            zEffectAnimEntry* const animationEntry = self->impactFxTable[impactSlot].animationEntry;
            if (animationEntry != 0) {
                zEffectAnim::SetTransformRotAndVelocityThunk(
                    animationEntry,
                    0,
                    hitEvent->hitPos.x,
                    hitEvent->hitPos.y,
                    hitEvent->hitPos.z,
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        }

        if (self->impactFxTable[impactSlot].effectTemplateIndex != 0) {
            zEffect::SpawnRuntimeInstanceAt(self->impactFxTable[impactSlot].effectTemplateIndex, &hitEvent->hitPos);
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-handleimpacteventfromruntimestate
     * @recoil-artifact defines .text recoil:function:0x4b0980: OptCatalog::HandleImpactEventFromRuntimeState
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef* and EDX is
     * OptCatalogRuntimeInstanceStorage*. Builds a stack hit event from
     * runtimeInstance->pos, a zero-slot surface-material reference, and
     * runtimeInstance->projectileNode, then forwards to HandleImpactEvent with
     * the original runtime instance.
     * Purpose: synthesize a simple hit event from runtime state and dispatch it.
     */
    void __fastcall HandleImpactEventFromRuntimeState(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    )
    {
        OptCatalogHitEventPartial hitEvent;
        OptCatalogSurfaceMaterialRef surfaceRef;

        surfaceRef.flags &= 0xfeff;
        surfaceRef.impactSlot = 0;
        hitEvent.hitPos = runtimeInstance->pos;
        hitEvent.surfaceRef = &surfaceRef;
        hitEvent.hitNode = runtimeInstance->projectileNode;

        HandleImpactEvent(self, &hitEvent, runtimeInstance);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-buildimpacthitlist
     * @recoil-artifact defines .text recoil:function:0x4b09d0: OptCatalog::BuildImpactHitList
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef*, EDX is
     * OptCatalogRuntimeInstanceStorage*, with allowOwnerOnlyHit and outHitList
     * on the stack. Temporarily clears projectile raycastability, filters
     * g_OptCatalogRuntimeWorld against a sphere at runtimeInstance->pos using
     * impactProximity, restores raycastability, rejects owner-only hits when
     * requested, and returns success for an accepted hit list.
     * Purpose: collect nearby impact candidates for runtime-probe handling.
     */
    int __fastcall BuildImpactHitList(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance,
        int allowOwnerOnlyHit,
        OptCatalogRaycastHitList* outHitList
    )
    {
        int restoreRaycastable = 0;
        if (runtimeInstance->projectileNode != 0 && (runtimeInstance->projectileNode->flags & 0x10) != 0) {
            restoreRaycastable = 1;
            CZClass::gwNodeSetRaycastable(runtimeInstance->projectileNode, 0);
        }

        int result = CZDisplayInstance::FilterRegionsAgainstSphere(
            g_OptCatalogRuntimeWorld,
            &runtimeInstance->pos,
            0,
            self->impactProximity,
            1,
            1,
            outHitList
        );

        if (restoreRaycastable != 0) {
            CZClass::gwNodeSetRaycastable(runtimeInstance->projectileNode, 1);
        }

        if (allowOwnerOnlyHit == 0 && outHitList->hitCount == 1
            && outHitList->hits[0].hitNode == runtimeInstance->ownerNode) {
            result = 1;
        }

        return result == 0 ? 1 : 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-handleimpactfromruntimeprobe
     * @recoil-artifact defines .text recoil:function:0x4b0a50: OptCatalog::HandleImpactFromRuntimeProbe
     *
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef*, EDX is
     * OptCatalogRuntimeInstanceStorage*, with hitList and excludedDamageHandler
     * on the stack. Walks probe hits, skips the excluded damage handler,
     * computes full or distance-scaled damage multiplied by spawnScale, then
     * either dispatches damage feedback immediately or queues a
     * OptCatalogQueuedImpactRecord; returns nonzero when any hit was processed.
     * Purpose: process or queue damage feedback for fallback probe hits.
     */
    int __fastcall HandleImpactFromRuntimeProbe(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance,
        OptCatalogRaycastHitList * hitList,
        void* excludedDamageHandler
    )
    {
        int processedAny = 0;
        for (int i = 0; i < hitList->hitCount; ++i) {
            OptCatalogRaycastHitEntry* hit = &hitList->hits[i];
            CZNodeFreeListSlot* hitSlot = (CZNodeFreeListSlot*)(hit->hitNode);
            if (hitSlot->damageHandler == excludedDamageHandler) {
                continue;
            }

            float damageAmount;
            if ((self->flags & kOptCatalogFlagFullProbeDamage) != 0) {
                damageAmount = self->damage;
            } else {
                damageAmount = (1.0f - hit->distance / self->damageFalloffRange) * self->damage;
            }
            damageAmount *= runtimeInstance->spawnScale;

            if ((self->flags & kOptCatalogFlagImmediateProbeImpact) != 0
                || g_OptCatalogQueuedImpactCount >= kMaxQueuedImpacts) {
                OptCatalogHitEventPartial* hitEvent = (OptCatalogHitEventPartial*)(void*)(hit);
                InvokeDamageFeedbackAndHitCallback(
                    self,
                    runtimeInstance->ownerNode,
                    &runtimeInstance->pos,
                    hitEvent,
                    damageAmount
                );
            } else {
                g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount].entry = self;
                g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount].ownerNode = runtimeInstance->ownerNode;
                g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount].sourcePos = runtimeInstance->pos;
                g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount].hit = *hit;
                g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount].damageAmount = damageAmount;
                ++g_OptCatalogQueuedImpactCount;
            }

            processedAny = 1;
        }

        return processedAny;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-canspawnthroughray
     * @recoil-artifact defines .text recoil:function:0x4b0ba0: OptCatalog::CanSpawnThroughRay
     *
     *
     * Purpose: test whether a trail segment can continue through a ray hit and
     * compute reflected distance/direction outputs.
     */
    int __fastcall CanSpawnThroughRay(
        OptCatalogEntryDef * self,
        OptCatalogRaycastHitEntry * hit,
        const zVec3* rayStart,
        const zVec3* rayEnd,
        float* rayLengthOut,
        float* reflectedLengthOut,
        zVec3* reflectedDirOut
    )
    {
        const float rayLength = zMath::Vec3DeltaLength(&hit->pos, rayStart);
        *rayLengthOut = rayLength;
        if (rayLength == 0.0f) {
            return 2;
        }

        const unsigned int flags = self->flags;
        if ((flags & (1u << 19)) == 0) {
            CZNodeFreeListSlot* const hitSlot = (CZNodeFreeListSlot*)(hit->hitNode);
            if (hitSlot->damageHandler != 0) {
                if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
                    g_OptCatalog_CapturedDamageSourcePos = *rayStart;
                    g_OptCatalog_CapturedDamageHitPos = *rayEnd;
                }

                return 0;
            }
        }

        if ((flags & 1u) == 0) {
            return 2;
        }

        zVec3 incident;
        incident.x = rayEnd->x - rayStart->x;
        incident.y = rayEnd->y - rayStart->y;
        incident.z = rayEnd->z - rayStart->z;
        zMath::Vec3Reflect((zVec3*)(void*)(hit), &incident, reflectedDirOut);
        *reflectedLengthOut = zMath::Vec3Normalize(reflectedDirOut);
        return 1;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-reflectandsortimpacttracelist
     * @recoil-artifact defines .text recoil:function:0x4b0ca0: OptCatalog::ReflectAndSortImpactTraceList
     *
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: choose the farthest pending trail target direction and sort
     * pending target slots by projection along that direction.
     */
    void __fastcall ReflectAndSortImpactTraceList(
        OptCatalogTrailRuntimeState * runtime,
        float* targetProjectionScratch,
        zVec3* directionOut
    )
    {
        zVec3* farthestTarget = directionOut;
        float farthestDistance = 0.0f;
        for (int projectionIndex = 0; projectionIndex < *runtime->pendingSpawnTargetCountPtr; ++projectionIndex) {
            zVec3* const targetPos = runtime->pendingSpawnTargetListPtr[projectionIndex].targetPos;
            const float distance = zMath::Vec3DeltaLength(runtime->spawnPos, targetPos);
            if (distance > farthestDistance) {
                farthestDistance = distance;
                farthestTarget = targetPos;
            }
        }

        zMath::Vec3DirectionTo(runtime->spawnPos, farthestTarget, directionOut);

        for (int targetProjectionIndex = 0; targetProjectionIndex < *runtime->pendingSpawnTargetCountPtr;
            ++targetProjectionIndex) {
            zVec3* const targetPos = runtime->pendingSpawnTargetListPtr[targetProjectionIndex].targetPos;
            zVec3 delta;
            delta.x = targetPos->x - runtime->spawnPos->x;
            delta.y = targetPos->y - runtime->spawnPos->y;
            delta.z = targetPos->z - runtime->spawnPos->z;
            targetProjectionScratch[targetProjectionIndex]
                = directionOut->x * delta.x + directionOut->y * delta.y + directionOut->z * delta.z;
        }

        int swapped;
        do {
            swapped = 0;
            for (int i = 0; i < *runtime->pendingSpawnTargetCountPtr - 1; ++i) {
                if (targetProjectionScratch[i] > targetProjectionScratch[i + 1]) {
                    PlayerProgressTargetSlotRuntime targetSwap = runtime->pendingSpawnTargetListPtr[i];
                    runtime->pendingSpawnTargetListPtr[i] = runtime->pendingSpawnTargetListPtr[i + 1];
                    runtime->pendingSpawnTargetListPtr[i + 1] = targetSwap;

                    const float projectionSwap = targetProjectionScratch[i];
                    targetProjectionScratch[i] = targetProjectionScratch[i + 1];
                    targetProjectionScratch[i + 1] = projectionSwap;
                    swapped = 1;
                }
            }
        } while (swapped != 0);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-computetrailimpactresponse
     * @recoil-artifact defines .text recoil:function:0x4b0e20: OptCatalog::ComputeTrailImpactResponse
     *
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: raycast a trail segment against the runtime world, apply
     * damage feedback on hits, play impact audio, and trim segment length to
     * the selected hit.
     */
    int __fastcall ComputeTrailImpactResponse(
        OptCatalogEntryDef * self,
        OptCatalogTrailRuntimeState * trailRuntime,
        OptCatalogTrailNodeSlot * segment,
        const zVec3* targetPos
    )
    {
        SetDamageMaskSlotIndex(self->damageMaskSlotIndex);
        CZDisplayInstance::SetStopAfterFirstHit(0x40000);
        CZClass::gwNodeSetRaycastable(trailRuntime->projectileNode, 0);

        PlayerProbeSampleCandidateBuffer rayData;
        const int raycastResult = CZDisplayInstance::RaycastSelectClosestHitBetweenPoints(
            g_OptCatalogRuntimeWorld,
            &segment->pos,
            targetPos,
            &rayData
        );

        CZClass::gwNodeSetRaycastable(trailRuntime->projectileNode, 1);

        if (raycastResult == 0) {
            zClassDiPickCandidateEntry* const selectedHit = &rayData.entries[rayData.candidateCount];
            OptCatalogHitEventPartial* const hitEvent = (OptCatalogHitEventPartial*)(void*)(selectedHit);
            CZNodeFreeListSlot* const hitSlot = (CZNodeFreeListSlot*)(selectedHit->node);

            if (hitSlot->damageHandler != 0) {
                trailRuntime->trailBlend
                    = ((float)cos((trailRuntime->trailDistance * kOptCatalogPi) / self->range) + 1.0f) * 0.5f;
                if (0.25f < trailRuntime->trailBlend) {
                    trailRuntime->trailBlend = 0.25f;
                }

                InvokeDamageFeedbackAndHitCallback(
                    self,
                    trailRuntime->projectileNode,
                    &segment->pos,
                    hitEvent,
                    trailRuntime->spawnScale * self->damage * g_OptCatalogRuntimeDeltaTime * trailRuntime->trailBlend
                );

                int impactSlot;
                if (hitEvent->surfaceRef != 0) {
                    impactSlot = hitEvent->surfaceRef->impactSlot;
                } else {
                    impactSlot = 0;
                }
                PlayImpactSound(self, hitEvent, impactSlot, 1.0f);
            }

            segment->scale = zMath::Vec3DeltaLength(&segment->pos, &selectedHit->hitPos);
            return 1;
        }

        segment->scale = zMath::Vec3DeltaLength(&segment->pos, targetPos);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-updatetrailsegmentvisual
     * @recoil-artifact defines .text recoil:function:0x4b0f70: OptCatalog::UpdateTrailSegmentVisual
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: activate and transform a trail segment node from its recovered
     * position, direction, and scale state.
     */
    void __fastcall UpdateTrailSegmentVisual(OptCatalogTrailNodeSlot * segment)
    {
        CZClass::gwNodeSetActive(segment->node, 1);
        CZObject3D::gwObject3DSetPosition(segment->node, segment->pos.x, segment->pos.y, segment->pos.z);

        const float yaw = (float)(atan2(-segment->dir.x, -segment->dir.z));
        const float pitch = (float)(asin(segment->dir.y));
        CZObject3D::gwObject3DSetRotation(segment->node, pitch, yaw, 0.0f);
        CZObject3D::gwObject3DSetScale(segment->node, 1.0f, 1.0f, segment->scale);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-playimpactsound
     * @recoil-artifact defines .text recoil:function:0x4b0fd0: OptCatalog::PlayImpactSound
     * @recoil-match byte
     *
     * Purpose: choose and play an impact sound sample at the hit position.
     */
    void __fastcall
    PlayImpactSound(OptCatalogEntryDef * self, OptCatalogHitEventPartial * hitEvent, int impactSlot, float gainScale)
    {
        if (self->impactFxTable[impactSlot].soundCount == 0) {
            return;
        }

        const int soundIndex = (unsigned int)(rand() * self->impactFxTable[impactSlot].soundCount) >> 15;
        self->impactFxTable[impactSlot].soundSamples[soundIndex]->PlayA3D(gainScale, &hitEvent->hitPos, 0);
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-playbouncesound
     * @recoil-artifact defines .text recoil:function:0x4b1030: OptCatalog::PlayBounceSound
     * @recoil-match byte
     *
     * Purpose: choose and play a bounce sound sample at the raycast hit.
     */
    void __fastcall
    PlayBounceSound(OptCatalogEntryDef * self, OptCatalogRaycastHitEntry * hitEvent, int impactSlot, float gainScale)
    {
        if (self->impactFxTable[impactSlot].bounceSoundCount == 0) {
            return;
        }

        const int soundIndex = (unsigned int)(rand() * self->impactFxTable[impactSlot].bounceSoundCount) >> 15;
        self->impactFxTable[impactSlot].bounceSoundSamples[soundIndex]->PlayA3D(gainScale, &hitEvent->pos, 0);
    }
} // namespace OptCatalog
