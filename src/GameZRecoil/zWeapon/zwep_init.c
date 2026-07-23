#include "opt_catalog.h"
#include "zwep.h"

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * Reimplements data 0x56bc9c: g_OptCatalog_AllocRuntimeGateCallback.
 * BN xrefs: GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks installs
 * the callback; OptCatalog::AllocRuntimeInstance calls it when network gate
 * processing is enabled.
 * Purpose: optional allocation gate for networked OptCatalog runtime
 * instances.
 */
OptCatalogAllocRuntimeGateCallback g_OptCatalog_AllocRuntimeGateCallback = 0;
/**
 * Reimplements data 0x56bca0: g_OptCatalog_AltGunDispatchNoOpCallback.
 * BN xrefs: GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks installs
 * the alternate-gun no-op dispatch callback.
 * Purpose: callback slot paired with the runtime allocation gate for
 * alternate-gun dispatch processing.
 */
OptCatalogAllocRuntimeGateCallback g_OptCatalog_AltGunDispatchNoOpCallback = 0;
/**
 * Reimplements data 0x56bca4: g_OptCatalog_RemoveRuntimeRelayCallback.
 * BN xrefs: GameNet::RegisterGameplayHandlersAndOptCatalogCallbacks installs
 * the callback; OptCatalog::RemoveRuntimeInstance invokes it after removal.
 * Purpose: optional network relay hook for removed OptCatalog runtime
 * instances.
 */
OptCatalogRemoveRuntimeRelayCallback g_OptCatalog_RemoveRuntimeRelayCallback = 0;
/**
 * Reimplements data 0x56bca8: g_OptCatalogRuntimeDeltaTime.
 * Purpose: current unscaled frame delta consumed by OptCatalog projectile and
 * trail runtime processing.
 */
float g_OptCatalogRuntimeDeltaTime = 0.0f;
/**
 * Reimplements data 0x56bcac: g_OptCatalogRuntimeNowSec.
 * Purpose: current unscaled time used by OptCatalog runtime updates and
 * warning-sound gates.
 */
float g_OptCatalogRuntimeNowSec = 0.0f;
/**
 * Reimplements data 0x56bcb0: g_OptCatalog_MineIteratorCursor.
 * BN xrefs: OptCatalog_MineIterator::Begin and
 * OptCatalog_MineIterator::Next.
 * Purpose: cursor for MineIterator_Begin/Next traversal of an entry's active
 * runtime-instance list.
 */
OptCatalogRuntimeInstanceStorage *g_OptCatalog_MineIteratorCursor = 0;
/**
 * Reimplements data 0x778920: g_OptCatalogRuntimeWorld.
 * BN xrefs include runtime allocation/recycling, projectile raycasts, trail
 * impact probes, zWeapon load/init/shutdown, and thermal glow light attach.
 * Purpose: active world node used by OptCatalog runtime projectiles, trail
 * probes, and glow-light attachment.
 */
zClass_NodePartial *g_OptCatalogRuntimeWorld = 0;
/**
 * Reimplements data 0x778924: g_OptCatalog_EntryCount.
 * BN xrefs include OptCatalog lookup helpers, ProcessRuntimeInstances,
 * zWeapon::Init, zWeapon::LoadOptCatalogFromPath, and ShutdownCore.
 * Purpose: number of loaded OptCatalog entries in the runtime catalog table.
 */
int g_OptCatalog_EntryCount = 0;
/**
 * Reimplements data 0x778928: g_OptCatalog_EntryTable.
 * BN xrefs include OptCatalog lookup helpers, ProcessRuntimeInstances,
 * zWeapon::Init, zWeapon::LoadOptCatalogFromPath, and ShutdownCore.
 * Purpose: owning pointer for the loaded OptCatalog entry array walked by
 * runtime processing and lookup helpers.
 */
OptCatalogEntryDef *g_OptCatalog_EntryTable = 0;
/**
 * Reimplements data 0x77892c: g_OptCatalogRuntimeInstanceCount.
 * Purpose: stores the configured runtime projectile pool count loaded with
 * the OptCatalog.
 */
int g_OptCatalogRuntimeInstanceCount = 0;
/**
 * Reimplements data 0x778930: g_OptCatalogRuntimeInstancePool.
 * Purpose: owns the allocated runtime projectile pool backing the free list
 * and active per-entry runtime lists.
 */
void *g_OptCatalogRuntimeInstancePool = 0;
/**
 * Reimplements data 0x778934: g_OptCatalogFreeRuntimeInstanceList.
 * Purpose: head of the free runtime projectile instance list shared by
 * allocation, recycling, and shutdown.
 */
OptCatalogRuntimeInstanceStorage *g_OptCatalogFreeRuntimeInstanceList = 0;
/**
 * Reimplements data 0x778938: g_OptCatalogThermalGlowFreeList.
 * Purpose: stores the head of the pooled thermal glow light free list shared
 * by OptCatalog runtime effects and the Light lifecycle functions.
 */
zClass_NodePartial *g_OptCatalogThermalGlowFreeList = 0;
/**
 * Reimplements data 0x77893c: g_OptCatalogNetworkOptionState.
 * BN xrefs: zWeapon::LoadOptCatalogFromPath initializes the state;
 * OptCatalog::AllocRuntimeInstance and ProcessRuntimeInstances read it for
 * network-runtime behavior.
 * Purpose: active OptCatalog network option state loaded with the catalog.
 */
int g_OptCatalogNetworkOptionState = 0;
/**
 * Reimplements data 0x778940: g_OptCatalog_CapturedDamageSourcePos.
 * Purpose: Stores g OptCatalog CapturedDamageSourcePos data used by effects_weapons.optcatalog_damage_feedback_data.
 */
zVec3 g_OptCatalog_CapturedDamageSourcePos = {0};
/**
 * Reimplements data 0x77894c: g_OptCatalog_CapturedDamageHitPos.
 * Purpose: Stores g OptCatalog CapturedDamageHitPos data used by effects_weapons.optcatalog_damage_feedback_data.
 */
zVec3 g_OptCatalog_CapturedDamageHitPos = {0};
/**
 * Reimplements data 0x778958: g_OptCatalog_CurrentDamageOwnerOrCtx.
 * Purpose: Stores g OptCatalog CurrentDamageOwnerOrCtx data used by effects_weapons.optcatalog_damage_feedback_data.
 */
void *g_OptCatalog_CurrentDamageOwnerOrCtx = 0;
/**
 * Reimplements data 0x77895c: g_OptCatalogPendingSpawnTargetCountPtr.
 * Purpose: transient pointer to the pending target count consumed by
 * OptCatalog runtime spawn and trail activation.
 */
int *g_OptCatalogPendingSpawnTargetCountPtr = 0;
/**
 * Reimplements data 0x778960: g_OptCatalogPendingSpawnTargetListPtr.
 * Purpose: transient pointer to pending target slots consumed by OptCatalog
 * runtime spawn and trail activation.
 */
PlayerProgressTargetSlotRuntime *g_OptCatalogPendingSpawnTargetListPtr = 0;
/**
 * Reimplements data 0x778964: g_OptCatalog_FallbackImpactProbeEnabled.
 * BN xrefs: OptCatalog::ProcessRuntimeInstance, ProcessRuntimeInstances,
 * zWeapon::Init, and OptCatalog::ShutdownCore.
 * Purpose: enables deferred fallback impact probes for runtime projectile
 * processing.
 */
int g_OptCatalog_FallbackImpactProbeEnabled = 0;
/**
 * Reimplements data 0x778968: g_OptCatalog_CaptureHitSnapshotEnabled.
 * Purpose: Stores g OptCatalog CaptureHitSnapshotEnabled data used by effects_weapons.optcatalog_damage_feedback_data.
 */
int g_OptCatalog_CaptureHitSnapshotEnabled = 0;
/**
 * Reimplements data 0x77896c: g_OptCatalogQueuedImpactCount.
 * Purpose: counts deferred OptCatalog impact records drained by
 * ProcessRuntimeInstances.
 */
int g_OptCatalogQueuedImpactCount = 0;
}

namespace {
    struct OptCatalogQueuedImpactRecord {
        OptCatalogEntryDef *entry;
        zClass_NodePartial *ownerNode;
        zVec3 sourcePos;
        OptCatalogRaycastHitEntry hit;
        float damageAmount;
        unsigned char unknown_40[4];
    };

    RECOIL_STATIC_ASSERT(sizeof(OptCatalogQueuedImpactRecord) == 68);

    /**
     * Reimplements data 0x778970: g_OptCatalogQueuedImpactRecords.
     * BN data shape: OptCatalogQueuedImpactRecord[64], 4352 bytes, zero-filled
     * BSS. Paired with g_OptCatalogQueuedImpactCount at 0x77896c.
     * Purpose: deferred impact callback queue drained by
     * OptCatalog::ProcessRuntimeInstances.
     */
    OptCatalogQueuedImpactRecord g_OptCatalogQueuedImpacts[64] = {0};
}

extern "C" {
/**
 * Reimplements data 0x779a70: g_OptCatalogLoadedTreeRoot.
 * BN xrefs: zWeapon::LoadOptCatalogFromPath stores the loaded root;
 * OptCatalog::ShutdownCore frees it through zReader::FreeLoadedTree and
 * clears the pointer.
 * Purpose: owning pointer for the currently loaded OptCatalog zReader tree.
 */
zReader::Node *g_OptCatalogLoadedTreeRoot = 0;
/**
 * Reimplements data 0x779a74: g_OptCatalogSndLockOnWarning.
 * BN xrefs: OptCatalog::ProcessRuntimeInstances and
 * zWeapon::LoadOptCatalogFromPath. BN currently types the data symbol as
 * int32_t, but all use sites consume it as a zSndSample pointer.
 * Purpose: lock-on warning sample played by the runtime tick gate.
 */
zSndSample *g_OptCatalogSndLockOnWarning = 0;
/**
 * Reimplements data 0x779a78: g_OptCatalogLockOnWarningGateTimeSec.
 * BN xrefs: OptCatalog::ProcessRuntimeInstances, zWeapon::Init, and
 * zWeapon::OnWeaponsSectionDataReady.
 * Purpose: throttles lock-on warning playback during OptCatalog runtime ticks.
 */
float g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
/**
 * Reimplements data 0x779a7c: g_OptCatalogMaxCraterRadius.
 * Purpose: clamps crater and quicksand terrain-deformation event radii.
 */
float g_OptCatalogMaxCraterRadius = 0.0f;
/**
 * Reimplements data 0x779a80: g_OptCatalog_DamageContextKind.
 * Purpose: Stores g OptCatalog DamageContextKind data used by effects_weapons.optcatalog_damage_feedback_data.
 */
int g_OptCatalog_DamageContextKind = 0;
/**
 * Reimplements data 0x779a84: g_OptCatalog_DamageFeedbackScale.
 * BN xrefs: DamageFeedback::SetIntensityScalar stores this scalar and
 * OptCatalog::InvokeDamageFeedbackAndHitCallback consumes it when selecting
 * damage-feedback effects. Source currently names the variable
 * g_OptCatalogDamageFeedbackIntensityScalar.
 * Purpose: per-hit damage feedback intensity scalar.
 */
float g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;
/**
 * Reimplements data 0x779a88: g_OptCatalog_DamageContextHitEvent.
 * Purpose: Stores g OptCatalog DamageContextHitEvent data used by effects_weapons.optcatalog_damage_feedback_data.
 */
void *g_OptCatalog_DamageContextHitEvent = 0;
/**
 * Reimplements data 0x779a8c: g_OptCatalogSndTriggerInactive.
 * BN xrefs: OptCatalog::PlayTriggerInactiveWarning and
 * zWeapon::LoadOptCatalogFromPath.
 * Purpose: trigger-inactive warning sample loaded with the OptCatalog.
 */
zSndSample *g_OptCatalogSndTriggerInactive = 0;
/**
 * Reimplements data 0x779a90: g_OptCatalogSndWeaponInactive.
 * BN xrefs: OptCatalog::PlayWeaponInactiveWarning and
 * zWeapon::LoadOptCatalogFromPath.
 * Purpose: weapon-inactive warning sample loaded with the OptCatalog.
 */
zSndSample *g_OptCatalogSndWeaponInactive = 0;
/**
 * Reimplements data 0x779a94: g_OptCatalogSndNoAmmoWarning.
 * BN xrefs: OptCatalog::PlayNoAmmoWarning and
 * zWeapon::LoadOptCatalogFromPath.
 * Purpose: no-ammo warning sample loaded with the OptCatalog.
 */
zSndSample *g_OptCatalogSndNoAmmoWarning = 0;
/**
 * Reimplements data 0x779a9c: g_OptCatalogDamageFeedbackCallback.
 * Purpose: Stores g OptCatalogDamageFeedbackCallback data used by effects_weapons.optcatalog_damage_feedback_data.
 */
void *g_OptCatalogDamageFeedbackCallback = 0;
/**
 * Reimplements data 0x779aa0: g_OptCatalog_DamageFeedbackHitCount.
 * Purpose: Stores g OptCatalog DamageFeedbackHitCount data used by effects_weapons.optcatalog_damage_feedback_data.
 */
int g_OptCatalog_DamageFeedbackHitCount = 0;
/**
 * Reimplements data 0x779aa4: g_OptCatalogDamageFeedbackTrackedNode.
 * Purpose: Stores g OptCatalogDamageFeedbackTrackedNode data used by effects_weapons.optcatalog_damage_feedback_data.
 */
zClass_NodePartial *g_OptCatalogDamageFeedbackTrackedNode = 0;
/**
 * Reimplements data 0x779aac: g_OptCatalogNextSpawnScale.
 * Purpose: one-shot spawn scale transferred into projectile or trail runtime
 * state, then reset to 1.0f.
 */
float g_OptCatalogNextSpawnScale = 0.0f;
/**
 * Reimplements data 0x4dcf7c: g_OptCatalogProcessRuntimeRelayEnabled.
 * BN initial bytes are 01 00 00 00. BN xrefs:
 * OptCatalog::SendPkt0A_RemoveRuntimeRelay reads the gate and
 * OptCatalog::HandlePkt0A_RemoveRuntimeRelay clears/restores it around local
 * relay processing.
 * Purpose: suppresses recursive runtime-removal relay while a network packet
 * is being handled.
 */
int g_OptCatalogProcessRuntimeRelayEnabled = 1;
/**
 * Reimplements data 0x4df804: g_zEffectAnim_TokenRange.
 * BN data shape: char[0x6] "RANGE"; xrefs from
 * zSndSystem::InitNamedSetsSyntax and zWeapon::LoadOptCatalogFromPath.
 * Purpose: names the RANGE parser field shared by sound sample ranges and
 * OptCatalog projectile ranges.
 */
char g_zEffectAnim_TokenRange[0x6] = "RANGE";
/**
 * Reimplements data 0x4df80c: g_zEffectAnim_TokenBounceSound.
 * BN data shape: char[0x0d] "BOUNCE_SOUND"; xref from
 * OptCatalog::LoadFxSpecFromReaderNode.
 * Purpose: names the optional bounce-sound sample list in OptCatalog effect
 * specs.
 */
char g_zEffectAnim_TokenBounceSound[0x0d] = "BOUNCE_SOUND";
/**
 * Reimplements data 0x4dd218: g_Player_KillVerbToken.
 * BN data shape: char[0x0a] "KILL_VERB"; xref only from
 * zWeapon_OptCatalog::LoadKillVerbString at 0x43ca20.
 * Purpose: names the optional kill-verb parser field in OptCatalog entry
 * records.
 */
char g_Player_KillVerbToken[0x0a] = "KILL_VERB";
}

namespace {
    const unsigned int kOptCatalogFlagImmediateProbeImpact = 1u << 12;
    const unsigned int kOptCatalogFlagFullProbeDamage = 1u << 13;
    const unsigned int kOptCatalogFlagCraterImpact = 0x08;
    const unsigned int kOptCatalogFlagQuickSandImpact = 0x20000;
    const unsigned int kOptCatalogFlagAlwaysPlayImpactFx = 4194304;
    const unsigned int kOptCatalogFlagTrailRuntime = 2;
    const unsigned int kOptCatalogFlagImpactWhenScaleExpired = 4;
    const unsigned int kOptCatalogFlagSingleTrailSegment = 0x800;
    const unsigned int kOptCatalogFlagAllowOutOfRangeAimPitch = 0x2000;
    const unsigned int kOptCatalogFlagSkipTrailSegmentLighting = 0x10000;
    const unsigned int kOptCatalogFlagSkipDamageMaskStamp = 0x200000;
    const unsigned int kOptCatalogFlagForceSpawnVelocity = 0x400;
    const unsigned int kOptCatalogFlagRelativeSpeed = 0x800000;
    const unsigned int kOptCatalogFlagFlyoutSkipRotation = 0x2000;
    const unsigned int kOptCatalogFlagFlyoutModelRotation = 0x100;
    const unsigned int kOptCatalogFlagUsePendingSpawnTarget = 1u << 22;
    const unsigned int kOptCatalogFlagTrailUsePendingSpawnTargets = 1u << 14;
    const unsigned int kOptCatalogFlagTrailStartMutedAndLight = 1u << 19;
    const unsigned int kOptCatalogFlagExpires = 1u << 6;
    const unsigned int kOptCatalogFlagFixedRotate = 1u << 7;
    const unsigned int kOptCatalogFlagInstant = 1u << 10;
    const unsigned int kOptCatalogFlagLockOn = 1u << 14;
    const unsigned int kOptCatalogFlagLockOnLead = 1u << 15;
    const unsigned int kOptCatalogFlagMultiTarget = 1u << 16;
    const unsigned int kOptCatalogFlagReload = 1u << 18;
    const unsigned int kOptCatalogFlagRemoteDetonate = 1u << 19;
    const unsigned int kOptCatalogFlagTetherGuided = 1u << 20;
    const unsigned int kOptCatalogFlagAppliesTimedHitStatus = 1u << 21;
    const unsigned int kOptCatalogFlagTimedStatusSubtractive = 1u << 9;
    const unsigned int kOptCatalogFlagHeatTimedStatus = 1u << 5;
    const unsigned int kOptCatalogNodeFlagAcceptsTerrainDeformation = 0x10000;
    const unsigned int kOptCatalogFastSqrtBias = 0x1fc00000;
    const int kOptCatalogRequiredVersion = 2;
    const int kMaxQueuedImpacts = 64;
    /**
     * Reimplements data 0x4d33ec: kOptCatalogAimPitchRangeScale.
     * Purpose: scales OptCatalog aim pitch range values loaded from weapon
     * catalog data.
     */
    const float kOptCatalogAimPitchRangeScale = -0.239999995f;
    const float kOptCatalogTrailDamageBlendLimit = 0.25f;
    const double kOptCatalogPi = 3.14159265358979323846;
    /**
     * Reimplements data 0x4e4600: g_zWeapon_BeamReflectNameFmt.
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN data shape: char[15] "BeamReflect_%d"; xref only from
     * OptCatalog::CreateTrailRuntimeState at 0x4b1ec0.
     * Purpose: format the inactive BeamReflect segment node names created
     * for OptCatalog trail runtime state.
     */
    const char g_zWeapon_BeamReflectNameFmt[15] = "BeamReflect_%d";

    typedef void( * OptCatalogRuntimeUpdateCallback)(
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    );

    struct OptCatalogDamageHealthOverlay {
        unsigned char unknown_00[0x7c];
        float health;
    };

    struct OptCatalogRuntimeInstancePoolSlot {
        OptCatalogRuntimeInstanceStorage runtime;
        unsigned char padding[4];
    };

    RECOIL_STATIC_ASSERT(sizeof(OptCatalogRuntimeInstancePoolSlot) == 0x90);

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath and its local loader
     * helpers.
     * Purpose: return the element count stored in a zReader array node.
     */
    int zReaderArrayCount(zReader::Node * node) {
        return node->value.nodes[0].value.i32;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath and OptCatalog loader
     * helpers.
     * Purpose: return a string element from a zReader array node.
     */
    const char *zReaderArrayString(
        zReader::Node * node,
        int index
    ) {
        return node->value.nodes[index].value.str;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath and OptCatalog loader
     * helpers.
     * Purpose: return an integer element from a zReader array node.
     */
    int zReaderArrayInt(
        zReader::Node * node,
        int index
    ) {
        return node->value.nodes[index].value.i32;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath and OptCatalog loader
     * helpers.
     * Purpose: read an int-or-float zReader array element as a float.
     */
    float zReaderArrayFloat(
        zReader::Node * node,
        int index
    ) {
        zReader::Node *const valueNode = &node->value.nodes[index];
        if (valueNode->type == zReader::ZRDR_NODE_INT) {
            return (float)(valueNode->value.i32);
        }

        return valueNode->value.f32;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath loader branches.
     * Purpose: set or clear an OptCatalog flag from a parsed boolean value.
     */
    void SetFlagFromBool(
        unsigned int &flags,
        unsigned int flag,
        int value
    ) {
        if (value != 0) {
            flags |= flag;
        } else {
            flags &= ~flag;
        }
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath callback table setup.
     * Purpose: convert a typed callback pointer to the generic action payload.
     */
    template<typename T> void *ActionCallbackPtr(T callback) {
        RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(void *));
        union {
            T typed;
            void *raw;
        } ptr = {callback};
        return ptr.raw;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in OptCatalog aim and trail math callsites in this source file.
     * Purpose: approximate square root through the recovered bit-bias idiom.
     */
    float FastSqrtApprox(float value) {
        unsigned int bits = 0;
        memcpy(
            &bits,
            &value,
            sizeof(bits)
        );
        bits = (bits >> 1) + kOptCatalogFastSqrtBias;
        memcpy(
            &value,
            &bits,
            sizeof(value)
        );
        return value;
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath loader branches.
     * Purpose: fetch an optional named zReader array string by index.
     */
    const char *ReadNamedArrayString(
        zReader::Node * parentNode,
        const char *name,
        int index
    ) {
        zReader::Node *const node = zReader_GetNamedNode(
            parentNode,
            name
        );
        if (node == 0 || node->type != zReader::ZRDR_NODE_ARRAY ||
            zReaderArrayCount(node) <= index) {
            return 0;
        }

        return zReaderArrayString(
            node,
            index
        );
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath warning and trail sound
     * loaders.
     * Purpose: resolve an optional named sound sample into an output slot.
     */
    void LoadNamedSoundSample(
        zReader::Node * parentNode,
        const char *name,
        zSndSample **outSample
    ) {
        const char *const sampleName = ReadNamedArrayString(
            parentNode,
            name,
            1
        );
        if (sampleName != 0) {
            *outSample = zSnd::FindSampleByName(sampleName);
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath flag loader branches.
     * Purpose: load a boolean OptCatalog flag from a named zReader array.
     */
    void LoadNamedBoolFlag(
        zReader::Node * entryNode,
        const char *name,
        OptCatalogEntryDef *entry,
        unsigned int flag
    ) {
        zReader::Node *const node = zReader_GetNamedNode(
            entryNode,
            name
        );
        if (node != 0 && node->type == zReader::ZRDR_NODE_ARRAY && zReaderArrayCount(node) > 1) {
            SetFlagFromBool(
                entry->flags,
                flag,
                zReaderArrayInt(node, 1)
            );
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath impact loader branches.
     * Purpose: parse crater radius base and randomized range metadata.
     */
    void LoadRadiusRange(
        zReader::Node * node,
        OptCatalogEntryDef * entry
    ) {
        const int count = zReaderArrayCount(node);
        if (count > 1 && zReaderArrayInt(
            node,
            1
        ) != 0) {
            entry->flags |= kOptCatalogFlagCraterImpact;
            if (count > 2) {
                const int minRadius = (int)(zReaderArrayFloat(
                    node,
                    1
                ));
                entry->craterRadiusBase = minRadius;
                entry->craterRadiusRandomRange = (int)(zReaderArrayFloat(
                    node,
                    2
                )) - minRadius;
            }
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath timed-status loader
     * branches.
     * Purpose: load timed-hit light range, delay, and color metadata.
     */
    void LoadTimedStatusBlock(
        zReader::Node * node,
        OptCatalogEntryDef * entry
    ) {
        if (zReaderArrayCount(node) <= 6) {
            return;
        }

        entry->timedStatusLightRangeMin = zReaderArrayFloat(
            node,
            1
        );
        entry->timedStatusLightRangeMax = zReaderArrayFloat(
            node,
            2
        );
        entry->timedStatusUpdateDelay = zReaderArrayFloat(
            node,
            3
        );
        entry->timedStatusLightSpecularColor.red = zReaderArrayFloat(
            node,
            4
        );
        entry->timedStatusLightSpecularColor.green = zReaderArrayFloat(
            node,
            5
        );
        entry->timedStatusLightSpecularColor.blue = zReaderArrayFloat(
            node,
            6
        );
        entry->flags |= kOptCatalogFlagAppliesTimedHitStatus;
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath designate-status loader
     * branches.
     * Purpose: load remote-detonation designate status metadata.
     */
    void LoadDesignateStatusBlock(
        zReader::Node * node,
        OptCatalogEntryDef * entry
    ) {
        if (zReaderArrayCount(node) <= 6) {
            return;
        }

        entry->timedStatusLightRangeMin = zReaderArrayFloat(
            node,
            1
        );
        entry->timedStatusLightRangeMax = zReaderArrayFloat(
            node,
            2
        );
        entry->timedStatusUpdateDelay = 0.0f;
        entry->timedStatusLightSpecularColor.red = zReaderArrayFloat(
            node,
            3
        );
        entry->timedStatusLightSpecularColor.green = zReaderArrayFloat(
            node,
            4
        );
        entry->timedStatusLightSpecularColor.blue = zReaderArrayFloat(
            node,
            5
        );
        entry->detonationDistSq = zReaderArrayFloat(
            node,
            6
        );
        entry->flags &= ~(kOptCatalogFlagAppliesTimedHitStatus | kOptCatalogFlagHeatTimedStatus);
        entry->flags |= kOptCatalogFlagRemoteDetonate;
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath damage-feedback loader
     * branches.
     * Purpose: load health-scaled damage-feedback effect variants.
     */
    void LoadDamageFeedbackOnHealth(
        zReader::Node * node,
        OptCatalogEntryDef * entry
    ) {
        const int count = zReaderArrayCount(node) - 1;
        entry->damageFeedbackVariantCount = count > 4 ? 4 : count;
        for (int i = 0; i < entry->damageFeedbackVariantCount; ++i) {
            zReader::Node *const variantNode = &node->value.nodes[i + 1];
            if (variantNode->type == zReader::ZRDR_NODE_ARRAY &&
                zReaderArrayCount(variantNode) > 2) {
                entry->damageFeedbackVariants[i].minFeedbackScale =
                    zReaderArrayFloat(
                        variantNode,
                        1
                    );
                entry->damageFeedbackVariants[i].effect =
                    zEffectAnim::FindEntryByName(zReaderArrayString(
                        variantNode,
                        2
                    ));
            }
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath impact-effect loader
     * branches.
     * Purpose: load per-material impact effect specs and fallback entries.
     */
    void LoadImpactFxTable(
        zReader::Node * impactNode,
        OptCatalogEntryDef * entry
    ) {
        if (g_zRndr_GlobalStringCount <= 0) {
            return;
        }

        OptCatalog::LoadFxSpecFromReaderNode(
            impactNode,
            &entry->impactFxTable[0],
            g_zRndr_GlobalStringTable[0]
        );
        for (int i = 1; i < g_zRndr_GlobalStringCount; ++i) {
            if (zReader_GetNamedNode(
                impactNode,
                g_zRndr_GlobalStringTable[i]
            ) != 0) {
                OptCatalog::LoadFxSpecFromReaderNode(
                    impactNode,
                    &entry->impactFxTable[i],
                    g_zRndr_GlobalStringTable[i]
                );
            } else {
                entry->impactFxTable[i] = entry->impactFxTable[0];
            }
        }

        zReader::Node *const animationAlwaysNode =
            zReader_GetNamedNode(
                impactNode,
                "ANIMATION_ALWAYS"
            );
        if (animationAlwaysNode != 0) {
            entry->flags |= kOptCatalogFlagAlwaysPlayImpactFx;
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath after catalog count load.
     * Purpose: allocate and initialize the OptCatalog runtime projectile pool.
     */
    void SetupRuntimeInstancePool() {
        if (g_OptCatalogRuntimeInstanceCount <= 0) {
            return;
        }

        OptCatalogRuntimeInstancePoolSlot *const slots =
            (OptCatalogRuntimeInstancePoolSlot *)(calloc(
                g_OptCatalogRuntimeInstanceCount,
                sizeof(OptCatalogRuntimeInstancePoolSlot)
            ));
        g_OptCatalogRuntimeInstancePool = slots;
        if (slots == 0) {
            return;
        }

        for (int i = 0; i < g_OptCatalogRuntimeInstanceCount; ++i) {
            OptCatalogRuntimeInstanceStorage *const runtime = &slots[i].runtime;
            runtime->projectileNode = zClass_Object3D::gwObject3DInit();
            if (runtime->projectileNode != 0) {
                char name[40];
                sprintf(
                    name,
                    "Projectile_%d",
                    i
                );
                zClass_Class::gwNodeSetName(
                    runtime->projectileNode,
                    name
                );
                zClass_Class::gwNodeSetRaycastable(
                    runtime->projectileNode,
                    0
                );
                zClass_Class::gwNodeSetCellPickable(
                    runtime->projectileNode,
                    0
                );
                zClass_Class::gwNodeSetPickable(
                    runtime->projectileNode,
                    1
                );
            }

            runtime->flyoutAnimPrimary = 0;
            runtime->flyoutAnimSecondary = 0;
            runtime->asyncFxHandle = 0;
            runtime->next = g_OptCatalogFreeRuntimeInstanceList;
            g_OptCatalogFreeRuntimeInstanceList = runtime;
        }
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in OptCatalog damage-feedback callback paths.
     * Purpose: read the damage-handler pointer stored on a zClass node slot.
     */
    OptCatalogDamageHandlerPartial *DamageHandlerForNode(zClass_NodePartial * node) {
        return (OptCatalogDamageHandlerPartial *)(((zClass_NodeFreeListSlot *)(node))
                ->damageHandler);
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in OptCatalog damage-feedback callback paths.
     * Purpose: spawn a feedback animation at the hit position.
     */
    void ActivateDamageFeedbackEffect(
        zEffectAnimEntry * effect,
        OptCatalogHitEventPartial * hitEvent
    ) {
        zEffectAnim::SetTransformRotAndVelocity_Thunk(
            effect,
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

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in OptCatalog damage-context effect paths.
     * Purpose: resolve the impact owner node from the current damage context.
     */
    zClass_NodePartial *ImpactOwnerNodeFromDamageContext() {
        OptCatalogHitEventPartial *const contextHitEvent =
            (OptCatalogHitEventPartial *)(g_OptCatalog_DamageContextHitEvent);
        if (contextHitEvent == 0 || contextHitEvent->surfaceRef == 0) {
            return 0;
        }

        return contextHitEvent->surfaceRef->impactOwnerNode;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in OptCatalog::ProcessRuntimeInstances variant-save logic.
     * Purpose: pack the active four-byte variant tag into an integer.
     */
    unsigned int PackVariantTag(const zTag4Partial *tag) {
        unsigned int packed = 0;
        memcpy(
            &packed,
            tag,
            sizeof(packed)
        );
        return packed;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in OptCatalog::ProcessRuntimeInstances variant-restore logic.
     * Purpose: unpack an integer into the current four-byte variant tag.
     */
    void SetCurrentVariantTagFromPacked(unsigned int packedTag) {
        memcpy(
            &g_Variant_CurrentTag,
            &packedTag,
            sizeof(g_Variant_CurrentTag)
        );
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in OptCatalog::ProcessRuntimeInstances runtime-list walks.
     * Purpose: select either a runtime variant tag or the saved caller tag.
     */
    void SetCurrentVariantForRuntime(
        unsigned int packedRuntimeTag,
        unsigned int savedPackedVariantTag
    ) {
        const unsigned char runtimeTagCount = (unsigned char)(packedRuntimeTag & 0xffu);
        if (runtimeTagCount == 4) {
            SetCurrentVariantTagFromPacked(savedPackedVariantTag);
        } else {
            SetCurrentVariantTagFromPacked(packedRuntimeTag);
        }
    }
}

namespace OptCatalog {








#if defined(RECOILAPP_LINK_SPLIT_EARLY_SHARD)
    /**
     * Reimplements 0x4340c0: OptCatalog::AltGunDispatchAllocRuntimeGateCallback
     * (D:\Proj\Battlesport\ai_net.cpp)
     * Purpose: gate pkt07 alt-gun runtime allocation and launch-time callback
     * dispatch for local map-owned rows.
     */
    int __fastcall AltGunDispatchAllocRuntimeGateCallback(
        OptCatalogEntryDef * self,
        void **saveStateSlot
    ) {
        const int ordinalIndex = self->ordinalIndex;
        if (ordinalIndex == 0 || ordinalIndex == 1) {
            return 1;
        }

        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(*saveStateSlot);
        if (saveState == 0) {
            return 0;
        }

        if (saveState == (zUtil_SaveGameState *)(g_GameStateOrMapTable)) {
            *saveStateSlot = (void *)(zVideo::ReturnSuccessStub());
            GameNet::SendPkt07_AltGunDispatch(
                (short)(ordinalIndex),
                (unsigned int)(*saveStateSlot)
            );
            *saveStateSlot = (void *)((unsigned int)(*saveStateSlot) | 0x01000000u);
            return 1;
        }

        const unsigned int dispatchFlags =
            (unsigned int)(saveState->playerState->altGunDispatchFlags);
        if ((dispatchFlags & 0x02000000u) == 0) {
            return 0;
        }

        *saveStateSlot = (void *)(dispatchFlags);
        return 1;
    }

    /**
     * Reimplements 0x434240: OptCatalog::SendPkt0A_RemoveRuntimeRelay
     * (D:\Proj\GameZRecoil\GameNet.cpp)
     * Purpose: send pkt0A removal relay packets for authored runtime
     * instances when recursive relay processing is enabled.
     */
    void __fastcall SendPkt0A_RemoveRuntimeRelay(
        OptCatalogEntryDef * self,
        zVec3 * pointOrVec3,
        zClass_NodePartial * ownerNode
    ) {
        if (g_OptCatalogProcessRuntimeRelayEnabled == 0 || ownerNode == 0) {
            return;
        }

        HudUiMgrSensorTrackNode *const ownerTrackContext =
            (HudUiMgrSensorTrackNode *)(ownerNode->callbackContext);
        if (ownerTrackContext == 0) {
            return;
        }

        zUtil_SaveGameState *const ownerSaveState =
            (zUtil_SaveGameState *)(ownerTrackContext->payload);
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header.payloadDword0 =
            zNetwork_GetLocalPlayerKey();
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.optCatalogEntryId =
            (short)(self->ordinalIndex);
        if (pointOrVec3 != 0) {
            g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3 = *pointOrVec3;
        } else {
            g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3.x = 0.0f;
            g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3.y = 0.0f;
            g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.pointOrVec3.z = 0.0f;
        }
        g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.ownerPlayerKey =
            ownerSaveState->netPlayerRow->playerKey;
        zNetwork_SendPacketReliable(&g_NetPkt0A_OptCatalogProcessRuntimeRelayBuf.header);
    }

    /**
     * Reimplements 0x4342d0: OptCatalog::HandlePkt0A_RemoveRuntimeRelay
     * (D:\Proj\GameZRecoil\GameNet.cpp)
     * Purpose: handle pkt0A removal relay packets by resolving the
     * OptCatalog entry and player row while suppressing echo relay sends.
     */
    int __fastcall HandlePkt0A_RemoveRuntimeRelay(
        int,
        NetPkt0A_RemoveRuntimeRelay *packet
    ) {
        OptCatalogEntryDef *const entry =
            OptCatalog::FindEntryById((int)(packet->optCatalogEntryId));

        zVec3 relayPointScratch;
        zVec3 *pointOrVec3 = &relayPointScratch;
        if (packet->pointOrVec3.x == 0.0f && packet->pointOrVec3.y == 0.0f &&
            packet->pointOrVec3.z == 0.0f) {
            pointOrVec3 = 0;
        }

        GameNetPlayerRow *const row = GameNet::FindPlayerRowByKey(packet->ownerPlayerKey);
        if (row == 0) {
            return 0;
        }

        zUtil_SaveGameState *const ownerSaveState = (zUtil_SaveGameState *)row->saveState;
        if (entry != 0 && ownerSaveState != 0) {
            g_OptCatalogProcessRuntimeRelayEnabled = 0;
            OptCatalog::RemoveRuntimeInstance(
                entry,
                pointOrVec3,
                ownerSaveState->playerState->rootNode
            );
            g_OptCatalogProcessRuntimeRelayEnabled = 1;
        }

        return 1;
    }
#endif






































}

#include "GameZRecoil/zUtil/zbd.h"

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

RECOIL_STATIC_ASSERT(sizeof(PlayerTimedHitStatus) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, runtimeFlags) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, hitSource) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, currentLevel) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, targetLevel) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, lightNode) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, nextUpdateTime) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(PlayerTimedHitStatus, lightParentNode) == 0x18);

namespace {
    /**
     * Reimplements data 0x4e4658: g_zWeapon_ThermalGlowLabel.
     * Purpose: stores the fixed node name assigned to pooled thermal glow
     * lights during initialization.
     */
    const char g_zWeapon_ThermalGlowLabel[] = "Thermal glow";
} // namespace

namespace OptCatalog {
/**
     * Reimplements 0x4ae380: OptCatalog::BlendDirectionTowardTarget
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: blend an active direction vector toward a target direction
     * using per-axis weights, then renormalize the result.
     */
    void __fastcall BlendDirectionTowardTarget(
        zVec3 * direction,
        const zVec3 *targetDirection,
        float xWeight,
        float yWeight,
        float zWeight
    ) {
        direction->x += (targetDirection->x - direction->x) * xWeight;
        direction->y += (targetDirection->y - direction->y) * yWeight;
        direction->z += (targetDirection->z - direction->z) * zWeight;
        zMath::Vec3Normalize(direction);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae3c0: OptCatalog::FindEntryByName
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: return the first loaded OptCatalog entry whose keyName matches
     * the requested catalog name.
     */
    OptCatalogEntryDef *__fastcall FindEntryByName(const char *name) {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef &entry = g_OptCatalog_EntryTable[i];
            if (entry.keyName != 0 && strcmp(
                name,
                entry.keyName
            ) == 0) {
                return &g_OptCatalog_EntryTable[i];
            }
        }

        return 0;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae450: OptCatalog::FindEntryById
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: return the first loaded OptCatalog entry whose ordinalIndex
     * matches the requested catalog id.
     */
    OptCatalogEntryDef *__fastcall FindEntryById(int entryId) {
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef &entry = g_OptCatalog_EntryTable[i];
            if (entry.keyName != 0 && entry.ordinalIndex == entryId) {
                return &g_OptCatalog_EntryTable[i];
            }
        }

        return 0;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae4a0: OptCatalog::SetPendingSpawnTargetOverrides
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: install the pending-spawn target count and list pointers used
     * by OptCatalog runtime spawn setup.
     */
    void __fastcall SetPendingSpawnTargetOverrides(
        void *pendingSpawnTargetCountPtr,
        void *pendingSpawnTargetListPtr
    ) {
        g_OptCatalogPendingSpawnTargetCountPtr = (int *)(pendingSpawnTargetCountPtr);
        g_OptCatalogPendingSpawnTargetListPtr =
            (PlayerProgressTargetSlotRuntime *)(pendingSpawnTargetListPtr);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae4b0: OptCatalog::AllocOrReuseAttachNodeChildClone
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: reuse an attach-clone child from the entry free list, or clone
     * the template node when none are available.
     */
    zClass_NodePartial *__fastcall AllocOrReuseAttachNodeChildClone(
        OptCatalogEntryDef * self
    ) {
        zClass_NodePartial *const clone = self->attachCloneChildFreeList;
        if (clone != 0) {
            self->attachCloneChildFreeList = clone->callbackContext;
            clone->callbackContext = 0;
            return clone;
        }

        return zClass_cls_util::CopyNodeWithCloneOptions(
            self->attachCloneTemplateNode,
            0,
            1
        );
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae4e0: OptCatalog::RecycleAttachNodeClone
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: stop pending attach animation work, detach the child clone,
     * and return it to the entry clone free list.
     */
    void __fastcall RecycleAttachNodeClone(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    ) {
        zEffectAnimEntry *const asyncFxHandle = runtimeInstance->asyncFxHandle;
        if (asyncFxHandle != 0) {
            zEffect_Anim::NodeActionCallback(
                asyncFxHandle,
                0
            );
        }

        zClass_Object3D::RemoveChild(
            runtimeInstance->projectileNode,
            runtimeInstance->attachCloneChild
        );
        runtimeInstance->attachCloneChild->callbackContext = self->attachCloneChildFreeList;
        self->attachCloneChildFreeList = runtimeInstance->attachCloneChild;
        runtimeInstance->attachCloneChild = 0;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae520: OptCatalog::ClearRuntimeInstanceAsyncFxHandleCallback
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: clear the runtime instance async FX handle after the attached
     * model animation completes.
     */
    void __fastcall ClearRuntimeInstanceAsyncFxHandleCallback(
        void *,
        OptCatalogRuntimeInstanceStorage *runtimeInstance,
        void *
    ) {
        runtimeInstance->asyncFxHandle = 0;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae530: OptCatalog::AllocOrReuseAttachNodeClone
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: take a runtime instance from the free list, attach any flyout
     * child clone, and reset per-spawn lifetime state.
     */
    OptCatalogRuntimeInstanceStorage *__fastcall AllocOrReuseAttachNodeClone(
        OptCatalogEntryDef * self
    ) {
        OptCatalogRuntimeInstanceStorage *const runtimeInstance =
            g_OptCatalogFreeRuntimeInstanceList;
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

            zClass_Object3D::gwObject3DAddChild(
                runtimeInstance->projectileNode,
                attachChildNode
            );
        }

        runtimeInstance->lifetime = 0.0f;
        runtimeInstance->updateCallback = 0;
        return runtimeInstance;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae590: OptCatalog::RecycleRuntimeInstanceStorage
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: detach projectile children, restore transform and collision
     * state, and push the runtime storage back onto the free list.
     */
    void __fastcall RecycleRuntimeInstanceStorage(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    ) {
        if (runtimeInstance->lifetime > 0.0f) {
            return;
        }

        zClass_NodePartial *const projectileNode = runtimeInstance->projectileNode;
        while (projectileNode->listCountA != 0) {
            zClass_Class::RemoveChild(
                projectileNode->listA[0],
                projectileNode
            );
        }

        zClass_NodePartial *const attachCloneTemplateNode = self->attachCloneTemplateNode;
        if (attachCloneTemplateNode != 0) {
            if (runtimeInstance->attachCloneChild != 0) {
                RecycleAttachNodeClone(
                    self,
                    runtimeInstance
                );
            } else {
                zClass_Class::RemoveChild(
                    projectileNode,
                    attachCloneTemplateNode
                );
            }
        }

        while (projectileNode->listCountB != 0) {
            zClass_Class::RemoveChild(
                projectileNode,
                projectileNode->listB[0]
            );
        }

        runtimeInstance->next = g_OptCatalogFreeRuntimeInstanceList;
        g_OptCatalogFreeRuntimeInstanceList = runtimeInstance;
        zClass_Object3D::gwObject3DSetScale(
            projectileNode,
            1.0f,
            1.0f,
            1.0f
        );
        zClass_Object3D::gwObject3DSetRotation(
            projectileNode,
            0.0f,
            0.0f,
            0.0f
        );
        zClass_Object3D::gwObject3DSetPosition(
            projectileNode,
            0.0f,
            0.0f,
            0.0f
        );
        ((zClass_NodeFreeListSlot *)(projectileNode))->damageHandler = 0;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4ae660: OptCatalog::AllocRuntimeInstance
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: allocate or reuse a projectile runtime instance, link it active,
     * initialize motion, FX, target, and collision state for the spawn.
     */
    OptCatalogRuntimeInstanceStorage *__fastcall AllocRuntimeInstance(
        OptCatalogEntryDef * self,
        zClass_NodePartial * ownerNode,
        zTag4Partial * variantTagOrNull,
        zVec3 * spawnPos,
        zVec3 * spawnDir,
        zVec3 * spawnVelocity,
        void *saveState,
        OptCatalogRuntimeInstanceStorage *runtimeInstanceOrNull
    ) {
        if (g_OptCatalogNetworkOptionState != 0 && g_OptCatalog_AllocRuntimeGateCallback != 0 &&
            g_OptCatalog_AllocRuntimeGateCallback(
                self,
                &saveState
            ) == 0) {
            return 0;
        }

        OptCatalogRuntimeInstanceStorage *runtimeInstance = runtimeInstanceOrNull;
        if (runtimeInstance == 0) {
            runtimeInstance = AllocOrReuseAttachNodeClone(self);
            if (runtimeInstance == 0) {
                return 0;
            }
        }

        runtimeInstance->next = self->activeRuntimeListHead;
        self->activeRuntimeListHead = runtimeInstance;
        zClass_Class::AddChild(
            g_OptCatalogRuntimeWorld,
            runtimeInstance->projectileNode
        );

        runtimeInstance->origin = *spawnPos;
        runtimeInstance->pos = *spawnPos;
        runtimeInstance->dir = *spawnDir;
        runtimeInstance->ownerNode = ownerNode;
        runtimeInstance->rangeProgress = 0.0f;
        runtimeInstance->scaleFade = 0.0f;
        runtimeInstance->saveState = saveState;
        runtimeInstance->variantTag = variantTagOrNull != 0 ? PackVariantTag(variantTagOrNull) : 4;
        runtimeInstance->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        runtimeInstance->speed = self->velocity;
        if (self->acceleration == 0.0f && (self->flags & kOptCatalogFlagForceSpawnVelocity) == 0) {
            runtimeInstance->lifetime = self->velocity;
            zMath::Vec3ScaleAdd(
                spawnVelocity,
                spawnDir,
                self->velocity,
                &runtimeInstance->velocity
            );
        } else {
            runtimeInstance->lifetime = 0.0000999999975f;
            runtimeInstance->velocity = *spawnVelocity;
            if ((self->flags & kOptCatalogFlagRelativeSpeed) != 0) {
                const float relativeSpeed = sqrtf(
                    (spawnVelocity->x * spawnVelocity->x) + (spawnVelocity->y * spawnVelocity->y) +
                    (spawnVelocity->z * spawnVelocity->z)
                );
                runtimeInstance->speed += relativeSpeed;
                runtimeInstance->lifetime += relativeSpeed;
                runtimeInstance->velocity.x -= spawnDir->x * relativeSpeed;
                runtimeInstance->velocity.y -= spawnDir->y * relativeSpeed;
                runtimeInstance->velocity.z -= spawnDir->z * relativeSpeed;
            }
        }

        if (self->fireFxSelectedSoundIndex != -1) {
            self->fireFxSoundSamples[self->fireFxSelectedSoundIndex]
                ->PlayA3D(
                    &runtimeInstance->pos,
                    1.0f,
                    0
                );
        }

        if (self->fireFxEffectTemplateIndex != 0) {
            zEffect::SpawnRuntimeInstanceAt(
                self->fireFxEffectTemplateIndex,
                &runtimeInstance->pos
            );
        } else if (self->fireFxSelectedEffectIndex != -1) {
            zEffectAnimEntry *const fireAnim =
                self->fireFxAnimationEntries[self->fireFxSelectedEffectIndex];
            if (fireAnim != 0) {
                float randomRoll = 0.0f;
                if ((self->fireFxFlags & 1u) != 0) {
                    randomRoll =
                        (((float)(rand()) * 0.0000305185094f) - 0.5f) * (float)(kOptCatalogPi);
                }

                zEffectAnim::SetTransformRotAndVelocity_Thunk(
                    fireAnim,
                    0,
                    runtimeInstance->pos.x,
                    runtimeInstance->pos.y,
                    runtimeInstance->pos.z,
                    asinf(spawnDir->y),
                    (float)(atan2(
                        -spawnDir->z,
                        -spawnDir->x
                    )),
                    randomRoll,
                    0.0f,
                    0.0f,
                    0.0f
                );
            }
        }

        if ((self->flags & kOptCatalogFlagFlyoutSkipRotation) == 0 &&
            (((self->flags & kOptCatalogFlagFlyoutModelRotation) != 0 &&
                 self->attachCloneTemplateNode != 0) ||
                (self->flyoutAnimationEntry != 0 && self->attachCloneTemplateNode == 0))) {
            zClass_Object3D::gwObject3DSetRotation(
                runtimeInstance->projectileNode,
                asinf(spawnDir->y),
                (float)(atan2(
                    -spawnDir->z,
                    -spawnDir->x
                )),
                0.0f
            );
        }

        zClass_Object3D::gwObject3DSetPosition(
            runtimeInstance->projectileNode,
            runtimeInstance->pos.x,
            runtimeInstance->pos.y,
            runtimeInstance->pos.z
        );

        if (self->flyoutSelectedEffectIndex != -1) {
            if (self->flyoutAnimationEntry != 0) {
                runtimeInstance->flyoutAnimPrimary = zEffectAnim::SetTransformRefs_Thunk(
                    self->flyoutAnimationEntry,
                    0,
                    runtimeInstance->projectileNode,
                    0,
                    runtimeInstance->projectileNode,
                    0
                );
            }
            if (self->flyoutAttachedAnimationEntry != 0) {
                runtimeInstance->flyoutAnimSecondary = zEffectAnim::SetPositionRefAndVelocity_Thunk(
                    self->flyoutAttachedAnimationEntry,
                    0,
                    runtimeInstance->projectileNode,
                    0,
                    0
                );
            }
            if (self->flyoutModelAnimationEntry != 0) {
                zEffectAnimEntry *const asyncFxHandle = zEffectAnim::SetVelocity_Thunk(
                    self->flyoutModelAnimationEntry,
                    runtimeInstance->attachCloneChild,
                    0.0f,
                    0.0f,
                    0.0f
                );
                runtimeInstance->asyncFxHandle = asyncFxHandle;
                zEffectAnimEntry::SetOnStateDoneCallback(
                    asyncFxHandle,
                    (void *)(&ClearRuntimeInstanceAsyncFxHandleCallback),
                    runtimeInstance
                );
            }
        }

        runtimeInstance->aux = zMath::g_zMath_Vec3Zero;
        runtimeInstance->spawnGateAccum = 0.0f;
        runtimeInstance->pendingTargetA = 0;
        runtimeInstance->pendingTargetB = 0;
        if ((self->flags & kOptCatalogFlagUsePendingSpawnTarget) != 0 &&
            g_OptCatalogPendingSpawnTargetListPtr != 0) {
            runtimeInstance->aux = *spawnVelocity;
            int *const pendingTargetCount = g_OptCatalogPendingSpawnTargetCountPtr;
            if (pendingTargetCount != 0 && *pendingTargetCount > 0) {
                PlayerProgressTargetSlotRuntime *const targetList =
                    g_OptCatalogPendingSpawnTargetListPtr;
                runtimeInstance->pendingTargetA = targetList[0].targetPos;
                runtimeInstance->pendingTargetB = targetList[0].targetVelocity;
            }
            g_OptCatalogPendingSpawnTargetCountPtr = 0;
        }

        if ((self->flags & kOptCatalogFlagImpactWhenScaleExpired) != 0) {
            zClass_Class::gwNodeSetRaycastable(
                runtimeInstance->projectileNode,
                1
            );
            runtimeInstance->projectileNode->flags |= 0x08000000;
            ((zClass_NodeFreeListSlot *)(runtimeInstance->projectileNode))->damageHandler =
                (void *)(1);
            runtimeInstance->projectileNode->callbackContext =
                (zClass_NodePartial *)(runtimeInstance);
            runtimeInstance->projectileScale = self->flyoutHealth;
        } else {
            zClass_Class::gwNodeSetRaycastable(
                runtimeInstance->projectileNode,
                0
            );
            runtimeInstance->projectileNode->flags &= ~0x08000000u;
        }

        self->fireFxSelectedSoundIndex = 0;
        self->fireFxSelectedEffectIndex = 0;
        self->flyoutSelectedEffectIndex = 0;
        return runtimeInstance;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4aeaa0: OptCatalog::SpawnRuntimeInstanceAt
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: spawn a positioned impact-scale runtime instance and attach
     * its projectile node to the OptCatalog runtime world.
     */
    OptCatalogRuntimeInstanceStorage *__fastcall SpawnRuntimeInstanceAt(
        OptCatalogEntryDef * self,
        zVec3 * spawnPos,
        zClass_NodePartial * ownerNode
    ) {
        OptCatalogRuntimeInstanceStorage *const runtimeInstance = AllocOrReuseAttachNodeClone(self);

        runtimeInstance->next = self->activeRuntimeListHead;
        self->activeRuntimeListHead = runtimeInstance;
        runtimeInstance->pos = *spawnPos;
        runtimeInstance->lifetime = 0.0f;
        runtimeInstance->ownerNode = ownerNode;
        runtimeInstance->spawnScale = g_OptCatalogNextSpawnScale;
        g_OptCatalogNextSpawnScale = 1.0f;

        zClass_Class::gwNodeSetRaycastable(
            runtimeInstance->projectileNode,
            1
        );
        runtimeInstance->projectileNode->flags |= 0x08000000;
        ((zClass_NodeFreeListSlot *)(runtimeInstance->projectileNode))->damageHandler = (void *)(1);
        runtimeInstance->projectileNode->callbackContext = (zClass_NodePartial *)(runtimeInstance);
        runtimeInstance->projectileScale = self->flyoutHealth;

        zClass_Object3D::gwObject3DSetPosition(
            runtimeInstance->projectileNode,
            spawnPos->x,
            spawnPos->y,
            spawnPos->z
        );
        zClass_Class::AddChild(
            g_OptCatalogRuntimeWorld,
            runtimeInstance->projectileNode
        );
        return runtimeInstance;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4aeb50: OptCatalog::RecycleRuntimeInstance
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: stop runtime FX, recycle any attach clone, detach the projectile
     * node from the runtime world, and return storage to the free list.
     */
    void __fastcall RecycleRuntimeInstance(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    ) {
        runtimeInstance->lifetime = 0.0f;

        zEffectAnimEntry *const flyoutAnimPrimary = runtimeInstance->flyoutAnimPrimary;
        if (flyoutAnimPrimary != 0) {
            zEffect_Anim::NodeActionCallback(
                flyoutAnimPrimary,
                0
            );
            runtimeInstance->flyoutAnimPrimary = 0;
        }

        zEffectAnimEntry *const flyoutAnimSecondary = runtimeInstance->flyoutAnimSecondary;
        if (flyoutAnimSecondary != 0) {
            zEffect_Anim::NodeActionCallback(
                flyoutAnimSecondary,
                0
            );
            runtimeInstance->flyoutAnimSecondary = 0;
        }

        if (runtimeInstance->attachCloneChild != 0) {
            RecycleAttachNodeClone(
                self,
                runtimeInstance
            );
        }

        zClass_Class::RemoveChild(
            g_OptCatalogRuntimeWorld,
            runtimeInstance->projectileNode
        );
        RecycleRuntimeInstanceStorage(
            self,
            runtimeInstance
        );
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4aebc0: OptCatalog::ClearRuntimeInstances
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: unlink and recycle every active runtime instance owned by the
     * catalog entry.
     */
    void __fastcall ClearRuntimeInstances(OptCatalogEntryDef * self) {
        OptCatalogRuntimeInstanceStorage *runtimeInstance = self->activeRuntimeListHead;
        self->activeRuntimeListHead = 0;
        while (runtimeInstance != 0) {
            OptCatalogRuntimeInstanceStorage *const next = runtimeInstance->next;
            RecycleRuntimeInstance(
                self,
                runtimeInstance
            );
            runtimeInstance = next;
        }
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4aebf0: OptCatalog::RemoveRuntimeInstance
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: process and recycle matching active runtime instances, or probe
     * a supplied point, then notify the remove-runtime relay callback.
     */
    int __fastcall RemoveRuntimeInstance(
        OptCatalogEntryDef * self,
        zVec3 * pointOrVec3,
        zClass_NodePartial * ownerNode
    ) {
        int result = 0;

        if (pointOrVec3 != 0) {
            OptCatalogRuntimeInstanceStorage runtimeInstance = {0};
            runtimeInstance.ownerNode = ownerNode;
            runtimeInstance.pos = *pointOrVec3;
            runtimeInstance.spawnScale = 1.0f;
            result = ProcessRuntimeInstance(
                self,
                &runtimeInstance
            );
        } else {
            OptCatalogRuntimeInstanceStorage *runtimeInstance = self->activeRuntimeListHead;
            OptCatalogRuntimeInstanceStorage **link = &self->activeRuntimeListHead;
            while (runtimeInstance != 0) {
                OptCatalogRuntimeInstanceStorage *const next = runtimeInstance->next;
                if ((self->flags & (1u << 20)) != 0 ||
                    (runtimeInstance->lifetime == 0.0f &&
                        (ownerNode == 0 || runtimeInstance->ownerNode == ownerNode))) {
                    *link = next;
                    result += ProcessRuntimeInstance(
                        self,
                        runtimeInstance
                    );
                    RecycleRuntimeInstance(
                        self,
                        runtimeInstance
                    );
                } else {
                    link = &runtimeInstance->next;
                }

                runtimeInstance = next;
            }
        }

        if (result != 0 && g_OptCatalog_RemoveRuntimeRelayCallback != 0) {
            g_OptCatalog_RemoveRuntimeRelayCallback(
                self,
                pointOrVec3,
                ownerNode
            );
        }

        return result;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4aed00: OptCatalog::ProcessRuntimeInstance
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef* and EDX is
     * OptCatalogRuntimeInstanceStorage*. Builds a vertical probe from runtime
     * position, masks and restores projectile active state for closest-hit
     * raycast against g_OptCatalogRuntimeWorld, dispatches direct hits through
     * HandleImpactEvent, then optionally runs the fallback impact probe using
     * BuildImpactHitList and HandleImpactFromRuntimeProbe.
     * Purpose: advance one runtime projectile through direct and fallback impact checks.
     */
    int __fastcall ProcessRuntimeInstance(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance
    ) {
        zClass_NodePartial *const projectileNode = runtimeInstance->projectileNode;
        zVec3 startPoint = runtimeInstance->pos;
        zVec3 endPoint = runtimeInstance->pos;
        startPoint.y += 1.0f;
        endPoint.y -= self->impactProximity * 0.1f;

        int result = 0;
        int restoreProjectileActive = 0;
        if (projectileNode != 0 && (projectileNode->flags & 0x04) != 0) {
            restoreProjectileActive = 1;
            zClass_Class::gwNodeSetActive(
                projectileNode,
                0
            );
        }

        PlayerProbeSampleCandidateBuffer rayData = {0};
        if (zClass_cls_di::RaycastSelectClosestHitBetweenPoints(
                g_OptCatalogRuntimeWorld,
                &startPoint,
                &endPoint,
                &rayData
            ) == 0) {
            OptCatalogHitEventPartial *const hitEvent =
                (OptCatalogHitEventPartial *)(void *)(&rayData.entries[rayData.candidateCount]);
            HandleImpactEvent(
                self,
                hitEvent,
                runtimeInstance
            );
            result = 1;
        }

        if (restoreProjectileActive != 0) {
            zClass_Class::gwNodeSetActive(
                projectileNode,
                1
            );
        }

        if (g_OptCatalog_FallbackImpactProbeEnabled != 0 && self->impactProximity > 0.0f) {
            OptCatalogRaycastHitList fallbackHits = {0};
            if (BuildImpactHitList(
                self,
                runtimeInstance,
                1,
                &fallbackHits
            ) != 0) {
                runtimeInstance->pos = startPoint;
                HandleImpactFromRuntimeProbe(
                    self,
                    runtimeInstance,
                    &fallbackHits,
                    0
                );
            }
        }

        return result;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4aee40: OptCatalog::ActivateTrailRuntimeState
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
    void __fastcall ActivateTrailRuntimeState(
        OptCatalogTrailRuntimeState * trailRuntimeState,
        int playerOrdinal
    ) {
        (void)playerOrdinal;

        OptCatalogEntryDef *const ownerEntry = trailRuntimeState->ownerEntry;
        ownerEntry->trailStopSample->PlayA3DSimple(1.0f);
        zSndPlayHandle *const loopHandle = ownerEntry->trailLoopSample->PlayA3DSimple(1.0f);
        trailRuntimeState->stopSoundHandle = loopHandle;
        if ((ownerEntry->flags & kOptCatalogFlagTrailStartMutedAndLight) != 0) {
            loopHandle->SetFreqScaled(0.0f);
        }

        if (ownerEntry->fireFxEffectTemplateIndex != 0) {
            zEffect::SpawnRuntimeInstanceAt(
                ownerEntry->fireFxEffectTemplateIndex,
                trailRuntimeState->spawnPos
            );
        } else if (ownerEntry->fireFxAnimationEntries[0] != 0) {
            float randomRoll = 0.0f;
            if ((ownerEntry->fireFxFlags & 1u) != 0) {
                randomRoll = (((float)(rand()) * 0.0000305185094f) - 0.5f) * (float)(kOptCatalogPi);
            }

            ownerEntry->trailEffectAnim = zEffectAnim::SetTransformRotAndVelocity_Thunk(
                ownerEntry->fireFxAnimationEntries[0],
                0,
                trailRuntimeState->spawnPos->x,
                trailRuntimeState->spawnPos->y,
                trailRuntimeState->spawnPos->z,
                asinf(trailRuntimeState->spawnDir->y),
                (float)(atan2(
                    -trailRuntimeState->spawnDir->z,
                    -trailRuntimeState->spawnDir->x
                )),
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
            trailRuntimeState->pendingSpawnTargetCountPtr =
                g_OptCatalogPendingSpawnTargetCountPtr;
            trailRuntimeState->pendingSpawnTargetListPtr =
                g_OptCatalogPendingSpawnTargetListPtr;
            g_OptCatalogPendingSpawnTargetCountPtr = 0;
        }

        if ((ownerEntry->flags & kOptCatalogFlagTrailStartMutedAndLight) != 0) {
            zClass_NodePartial *const light =
                Light::AllocFromFreeListAndAttach(&ownerEntry->timedStatusLightSpecularColor);
            trailRuntimeState->lightNode = light;
            zClass_Light::gwLightSetRange(
                light,
                ownerEntry->timedStatusLightRangeMin,
                ownerEntry->timedStatusLightRangeMax
            );
            zClass_Class::gwNodeSetActive(
                light,
                0
            );
        }

        OptCatalogTrailRuntimeState *const activeRuntime = ownerEntry->activeTrailRuntime;
        if (activeRuntime != 0) {
            activeRuntime->prev = trailRuntimeState;
        }
        trailRuntimeState->prev = 0;
        trailRuntimeState->next = activeRuntime;
        ownerEntry->activeTrailRuntime = trailRuntimeState;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4aefb0: OptCatalog::DeactivateTrailRuntimeState
     * (D:\Proj\Battlesport\OptCatalog.cpp).
     * Purpose: stop trail runtime resources, unlink the active trail state,
     * return any glow light, and deactivate live trail segment nodes.
     */
    int __fastcall DeactivateTrailRuntimeState(
        OptCatalogTrailRuntimeState * trailRuntimeState
    ) {
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
                zClass_Class::gwNodeSetActive(
                    node,
                    0
                );
            }
        }

        trailRuntimeState->activeNodeSlotCursor = 0;
        return 0;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4af060: OptCatalog::ProcessRuntimeInstances
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
    void ProcessRuntimeInstances() {
        const unsigned int savedPackedVariantTag = PackVariantTag(&g_Variant_CurrentTag);
        float nearestLockOnDistance = (float)(_HUGE);

        g_OptCatalogRuntimeDeltaTime = g_Time_UnscaledDeltaTimeSec;
        g_OptCatalogRuntimeNowSec = g_Time_UnscaledAccumulatedTimeSec;

        while (g_OptCatalogQueuedImpactCount != 0) {
            --g_OptCatalogQueuedImpactCount;
            OptCatalogQueuedImpactRecord *const record =
                &g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount];
            InvokeDamageFeedbackAndHitCallback(
                record->entry,
                record->ownerNode,
                &record->sourcePos,
                (OptCatalogHitEventPartial *)(void *)(&record->hit),
                record->damageAmount
            );
        }

        for (int i = 0; i < g_OptCatalog_EntryCount; ++i) {
            OptCatalogEntryDef *const entry = &g_OptCatalog_EntryTable[i];
            if (entry->keyName == 0) {
                continue;
            }

            if ((entry->flags & kOptCatalogFlagTrailRuntime) != 0) {
                OptCatalogTrailRuntimeState *trailRuntime = entry->activeTrailRuntime;
                while (trailRuntime != 0) {
                    OptCatalogTrailRuntimeState *const nextTrailRuntime = trailRuntime->next;
                    if (trailRuntime->variantTagPtr != 0) {
                        g_Variant_CurrentTag = *trailRuntime->variantTagPtr;
                    } else {
                        SetCurrentVariantTagFromPacked(savedPackedVariantTag);
                    }

                    OptCatalogTrailNodeSlot *segment = trailRuntime->activeNodeSlots;
                    if (trailRuntime->spawnPos != 0 && trailRuntime->spawnDir != 0 &&
                        trailRuntime->activeNodeSlotCount > 0) {
                        segment->pos = *trailRuntime->spawnPos;
                        segment->dir = *trailRuntime->spawnDir;
                        trailRuntime->trailDistance +=
                            entry->damageFalloffRange * g_OptCatalogRuntimeDeltaTime;
                        trailRuntime->alphaPulsePhase += g_OptCatalogRuntimeDeltaTime * 10.0f;

                        int visibleSegmentCount = 1;
                        if (trailRuntime->pendingSpawnTargetCountPtr != 0 &&
                            trailRuntime->pendingSpawnTargetListPtr != 0 &&
                            *trailRuntime->pendingSpawnTargetCountPtr > 1) {
                            float targetProjectionScratch[8] = {0};
                            zVec3 sortedDirection = {0};
                            ReflectAndSortImpactTraceList(
                                trailRuntime,
                                targetProjectionScratch,
                                &sortedDirection
                            );

                            visibleSegmentCount = *trailRuntime->pendingSpawnTargetCountPtr;
                            if (visibleSegmentCount > 4) {
                                visibleSegmentCount = 4;
                            }
                            if (visibleSegmentCount > trailRuntime->activeNodeSlotCount) {
                                visibleSegmentCount = trailRuntime->activeNodeSlotCount;
                            }

                            zVec3 cursor = *trailRuntime->spawnPos;
                            for (int segmentIndex = 0; segmentIndex < visibleSegmentCount;
                                ++segmentIndex) {
                                OptCatalogTrailNodeSlot *const currentSegment =
                                    &trailRuntime->activeNodeSlots[segmentIndex];
                                currentSegment->pos = cursor;

                                zVec3 *const targetPos =
                                    trailRuntime->pendingSpawnTargetListPtr[segmentIndex].targetPos;
                                zMath::Vec3DirectionTo(
                                    &currentSegment->pos,
                                    targetPos,
                                    &currentSegment->dir
                                );
                                currentSegment->scale =
                                    zMath::Vec3DeltaLength(
                                        &currentSegment->pos,
                                        targetPos
                                    );
                                ComputeTrailImpactResponse(
                                    entry,
                                    trailRuntime,
                                    currentSegment,
                                    targetPos
                                );
                                UpdateTrailSegmentVisual(currentSegment);
                                zMath::Vec3ScaleAdd(
                                    &currentSegment->pos,
                                    &currentSegment->dir,
                                    currentSegment->scale,
                                    &cursor
                                );
                            }
                        } else {
                            segment->scale = entry->range;
                            ComputeTrailImpactResponse(
                                entry,
                                trailRuntime,
                                segment,
                                trailRuntime->spawnPos
                            );
                            UpdateTrailSegmentVisual(segment);
                        }

                        for (int segmentIndex = visibleSegmentCount;
                            segmentIndex < trailRuntime->activeNodeSlotCursor;
                            ++segmentIndex) {
                            zClass_NodePartial *const node =
                                trailRuntime->activeNodeSlots[segmentIndex].node;
                            if (node != 0) {
                                zClass_Class::gwNodeSetActive(
                                    node,
                                    0
                                );
                            }
                        }
                        trailRuntime->activeNodeSlotCursor = visibleSegmentCount;
                    }

                    trailRuntime = nextTrailRuntime;
                }
                continue;
            }

            OptCatalogRuntimeInstanceStorage **link = &entry->activeRuntimeListHead;
            OptCatalogRuntimeInstanceStorage *runtimeInstance = entry->activeRuntimeListHead;
            while (runtimeInstance != 0) {
                OptCatalogRuntimeInstanceStorage *const nextRuntime = runtimeInstance->next;
                int recycleRuntime = 0;

                if (runtimeInstance->updateCallback != 0) {
                    OptCatalogRuntimeUpdateCallback callback =
                        (OptCatalogRuntimeUpdateCallback)(runtimeInstance->updateCallback);
                    callback(runtimeInstance);
                }

                SetCurrentVariantForRuntime(
                    runtimeInstance->variantTag,
                    savedPackedVariantTag
                );

                if ((entry->flags & kOptCatalogFlagImpactWhenScaleExpired) != 0 &&
                    runtimeInstance->projectileScale <= 0.0f) {
                    HandleImpactEventFromRuntimeState(
                        entry,
                        runtimeInstance
                    );
                    recycleRuntime = 1;
                } else {
                    if (runtimeInstance->speed != 0.0f) {
                        if ((entry->flags & kOptCatalogFlagLockOn) != 0 &&
                            runtimeInstance->pendingTargetA != 0) {
                            zVec3 targetDirection;
                            zMath::Vec3DirectionTo(
                                &runtimeInstance->pos,
                                (zVec3 *)(runtimeInstance->pendingTargetA),
                                &targetDirection
                            );

                            float turnBlend;
                            if (runtimeInstance->spawnGateAccum < entry->turnSuspendTime) {
                                turnBlend = 0.0f;
                            } else if (entry->lockOnTime + entry->turnSuspendTime <=
                                entry->turnSuspendTime) {
                                turnBlend = 1.0f;
                            } else {
                                turnBlend =
                                    (runtimeInstance->spawnGateAccum - entry->turnSuspendTime) /
                                    entry->lockOnTime;
                            }

                            if ((entry->flags & kOptCatalogFlagTetherGuided) == 0) {
                                const float turnStep =
                                    entry->turnRate * turnBlend * g_OptCatalogRuntimeDeltaTime;
                                const float directionDot =
                                    runtimeInstance->dir.x * targetDirection.x +
                                    runtimeInstance->dir.y * targetDirection.y +
                                    runtimeInstance->dir.z * targetDirection.z;
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
                        }

                        runtimeInstance->velocity.x =
                            runtimeInstance->dir.x * runtimeInstance->speed;
                        runtimeInstance->velocity.y =
                            runtimeInstance->dir.y * runtimeInstance->speed;
                        runtimeInstance->velocity.z =
                            runtimeInstance->dir.z * runtimeInstance->speed;
                    }

                    runtimeInstance->pos.x +=
                        runtimeInstance->velocity.x * g_OptCatalogRuntimeDeltaTime;
                    runtimeInstance->pos.y +=
                        runtimeInstance->velocity.y * g_OptCatalogRuntimeDeltaTime;
                    runtimeInstance->pos.z +=
                        runtimeInstance->velocity.z * g_OptCatalogRuntimeDeltaTime;
                    runtimeInstance->lifetime += g_OptCatalogRuntimeDeltaTime;

                    if (runtimeInstance->projectileNode != 0) {
                        zClass_Object3D::gwObject3DSetPosition(
                            runtimeInstance->projectileNode,
                            runtimeInstance->pos.x,
                            runtimeInstance->pos.y,
                            runtimeInstance->pos.z
                        );

                        zVec3 direction = runtimeInstance->dir;
                        if (zMath::Vec3Normalize(&direction) != 0.0f) {
                            const float yaw = (float)(atan2(
                                -direction.x,
                                -direction.z
                            ));
                            const float pitch = (float)(asin(direction.y));
                            zClass_Object3D::gwObject3DSetRotation(
                                runtimeInstance->projectileNode,
                                pitch,
                                yaw,
                                0.0f
                            );
                        }
                    }

                    if (entry->impactProximity > 0.0f &&
                        ProcessRuntimeInstance(
                            entry,
                            runtimeInstance
                        ) != 0) {
                        recycleRuntime = 1;
                    } else if (entry->range > 0.0f && runtimeInstance->lifetime >= entry->range) {
                        recycleRuntime = 1;
                    }
                }

                if (recycleRuntime != 0) {
                    *link = nextRuntime;
                    RecycleRuntimeInstance(
                        entry,
                        runtimeInstance
                    );
                } else {
                    link = &runtimeInstance->next;

                    if (runtimeInstance->pendingTargetA != 0) {
                        zVec3 *const targetPos = (zVec3 *)(runtimeInstance->pendingTargetA);
                        const float lockOnDistance =
                            zMath::Vec3DeltaLength(
                                &runtimeInstance->pos,
                                targetPos
                            );
                        if (lockOnDistance < nearestLockOnDistance) {
                            nearestLockOnDistance = lockOnDistance;
                        }
                    }
                }

                runtimeInstance = nextRuntime;
            }
        }

        if (nearestLockOnDistance < (float)(_HUGE) &&
            g_OptCatalogRuntimeNowSec >= g_OptCatalogLockOnWarningGateTimeSec) {
            if (g_OptCatalogSndLockOnWarning != 0) {
                g_OptCatalogSndLockOnWarning->PlayA3DSimple(1.0f);
            }
            g_OptCatalogLockOnWarningGateTimeSec = g_OptCatalogRuntimeNowSec - 0.5f;
        }

        SetCurrentVariantTagFromPacked(savedPackedVariantTag);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0530: OptCatalog::ComputeAimPitchForTarget
     * Purpose: Computes launch pitch to hit a target and writes the approximated target distance.
     */
    float __fastcall ComputeAimPitchForTarget(
        OptCatalogEntryDef * self,
        const zVec3 *origin,
        const zVec3 *unusedDirection,
        const zVec3 *target,
        float *distanceApproxOut
    ) {
        (void)unusedDirection;

        zVec3 delta;
        delta.x = target->x - origin->x;
        delta.y = target->y - origin->y;
        delta.z = target->z - origin->z;

        const float distanceSq = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        int distanceBits;
        memcpy(
            &distanceBits,
            &distanceSq,
            sizeof(distanceBits)
        );
        distanceBits = (distanceBits >> 1) + (int)(kOptCatalogFastSqrtBias);

        float distanceApprox;
        memcpy(
            &distanceApprox,
            &distanceBits,
            sizeof(distanceApprox)
        );
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
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0600: OptCatalog::PlayTriggerInactiveWarning
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: play the trigger-inactive warning sound at full gain.
     */
    void PlayTriggerInactiveWarning() {
        g_OptCatalogSndTriggerInactive->PlayA3DSimple(1.0f);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0620: OptCatalog::PlayWeaponInactiveWarning
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: play the weapon-inactive warning sound at full gain.
     */
    void PlayWeaponInactiveWarning() {
        g_OptCatalogSndWeaponInactive->PlayA3DSimple(1.0f);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0640: OptCatalog::PlayNoAmmoWarning
     * BN source path: D:\Proj\Battlesport\OptCatalog.cpp.
     * Purpose: play the no-ammo warning sound at full gain.
     */
    void PlayNoAmmoWarning() {
        g_OptCatalogSndNoAmmoWarning->PlayA3DSimple(1.0f);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0660: OptCatalog::EmitQSandImpactEvent
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
        zClass_NodePartial * unusedOwnerNode,
        zClass_NodePartial * damageOwnerNode
    ) {
        (void)unusedOwnerNode;

        if ((hitEvent->hitNode->flags & kOptCatalogNodeFlagAcceptsTerrainDeformation) == 0) {
            return;
        }

        zDEClient_QSandEventTemplate eventTemplate;
        zDEClient::CopyQSandEventTemplateDefaults(&eventTemplate);

        eventTemplate.center = hitEvent->hitPos;
        if (self->craterRadiusRandomRange != 0) {
            const unsigned int radius =
                (unsigned int)(self->craterRadiusBase +
                               ((rand() * self->craterRadiusRandomRange) >> 15));
            eventTemplate.radius = (float)(radius);
        } else {
            eventTemplate.radius = self->impactProximity * 0.5f;
            if (g_OptCatalogMaxCraterRadius < eventTemplate.radius) {
                eventTemplate.radius = g_OptCatalogMaxCraterRadius;
            }
        }

        eventTemplate.damageOwnerNode = damageOwnerNode;
        zDEClient_QSand::InstanceEventMaybeRelay(&eventTemplate);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0710: OptCatalog::EmitCraterImpactEvent
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
        zClass_NodePartial * unusedOwnerNode,
        zClass_NodePartial * damageOwnerNode
    ) {
        (void)unusedOwnerNode;

        if ((hitEvent->hitNode->flags & kOptCatalogNodeFlagAcceptsTerrainDeformation) == 0) {
            return 0;
        }

        zDEClient_CraterEventTemplate eventTemplate;
        zDEClient_Crater::InitEventTemplateDefaults(&eventTemplate);

        eventTemplate.craterMaterialSlot = (zModel_MaterialSlot *)(hitEvent->surfaceRef);
        eventTemplate.center = hitEvent->hitPos;
        if (self->craterRadiusRandomRange != 0) {
            const unsigned int radius =
                (unsigned int)(self->craterRadiusBase +
                               ((rand() * self->craterRadiusRandomRange) >> 15));
            eventTemplate.radius = (float)(radius);
        } else {
            eventTemplate.radius = self->impactProximity * 0.5f;
            if (g_OptCatalogMaxCraterRadius < eventTemplate.radius) {
                eventTemplate.radius = g_OptCatalogMaxCraterRadius;
            }
        }

        eventTemplate.damageOwnerNode = damageOwnerNode;
        return zDEClient_Crater::InstanceEventMaybeRelay(&eventTemplate) == 0 ? 1 : 0;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b07d0: OptCatalog::HandleImpactEvent
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
    ) {
        int impactSlot = 0;
        if (hitEvent->surfaceRef != 0) {
            impactSlot = hitEvent->surfaceRef->impactSlot;
        }

        if (self->impactCallback != 0) {
            self->impactCallback(
                self,
                hitEvent,
                runtimeInstance
            );
        }

        const float damageAmount = runtimeInstance->spawnScale * self->damage;
        int damageHandled = InvokeDamageFeedbackAndHitCallback(
            self,
            runtimeInstance->ownerNode,
            &runtimeInstance->pos,
            hitEvent,
            damageAmount
        );

        int suppressFallbackFx = 0;
        if ((self->flags & kOptCatalogFlagCraterImpact) != 0) {
            if ((self->flags & kOptCatalogFlagAlwaysPlayImpactFx) == 0) {
                suppressFallbackFx = 1;
            }
            suppressFallbackFx &= EmitCraterImpactEvent(
                self,
                hitEvent,
                hitEvent->surfaceRef != 0 ? hitEvent->surfaceRef->impactOwnerNode : 0,
                runtimeInstance->ownerNode
            );
        } else if ((self->flags & kOptCatalogFlagQuickSandImpact) != 0) {
            zClass_NodePartial *contextOwnerNode = 0;
            if (g_OptCatalog_DamageContextHitEvent != 0) {
                contextOwnerNode = ImpactOwnerNodeFromDamageContext();
            } else if (hitEvent->surfaceRef != 0) {
                contextOwnerNode = hitEvent->surfaceRef->impactOwnerNode;
            }

            EmitQSandImpactEvent(
                self,
                hitEvent,
                contextOwnerNode,
                runtimeInstance->ownerNode
            );
        }

        if (g_OptCatalog_DamageContextKind != 0 &&
            (self->flags & kOptCatalogFlagCraterImpact) != 0) {
            EmitCraterImpactEvent(
                self,
                hitEvent,
                ImpactOwnerNodeFromDamageContext(),
                runtimeInstance->ownerNode
            );
        }

        if ((self->flags & kOptCatalogFlagQuickSandImpact) != 0 &&
            g_OptCatalog_DamageContextHitEvent != 0) {
            EmitQSandImpactEvent(
                self,
                hitEvent,
                ImpactOwnerNodeFromDamageContext(),
                runtimeInstance->ownerNode
            );
            return;
        }

        PlayImpactSound(
            self,
            hitEvent,
            impactSlot,
            1.0f
        );
        if (suppressFallbackFx == 0 && damageHandled == 0) {
            OptCatalogFxSpec *const impactSpec = &self->impactFxTable[impactSlot];
            zEffectAnimEntry *const animationEntry = impactSpec->animationEntry;
            if (animationEntry != 0) {
                zEffectAnim::SetTransformRotAndVelocity_Thunk(
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

            if (impactSpec->effectTemplateIndex != 0) {
                zEffect::SpawnRuntimeInstanceAt(
                    impactSpec->effectTemplateIndex,
                    &hitEvent->hitPos
                );
            }
        }
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0980: OptCatalog::HandleImpactEventFromRuntimeState
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
    ) {
        OptCatalogHitEventPartial hitEvent = {0};
        OptCatalogSurfaceMaterialRef surfaceRef = {0};

        surfaceRef.flags &= 0xfeff;
        surfaceRef.impactSlot = 0;
        hitEvent.hitPos = runtimeInstance->pos;
        hitEvent.surfaceRef = &surfaceRef;
        hitEvent.hitNode = runtimeInstance->projectileNode;

        HandleImpactEvent(
            self,
            &hitEvent,
            runtimeInstance
        );
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b09d0: OptCatalog::BuildImpactHitList
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef*, EDX is
     * OptCatalogRuntimeInstanceStorage*, with allowOwnerOnlyHit and outHitList
     * on the stack. Temporarily clears projectile raycastability, filters
     * g_Player_RuntimeDiScene against a sphere at runtimeInstance->pos using
     * impactProximity, restores raycastability, rejects owner-only hits when
     * requested, and returns success for an accepted hit list.
     * Purpose: collect nearby impact candidates for runtime-probe handling.
     */
    int __fastcall BuildImpactHitList(
        OptCatalogEntryDef * self,
        OptCatalogRuntimeInstanceStorage * runtimeInstance,
        int allowOwnerOnlyHit,
        OptCatalogRaycastHitList *outHitList
    ) {
        zClass_NodePartial *projectileNode = runtimeInstance->projectileNode;
        int restoreRaycastable = 0;
        if (projectileNode != 0 && (projectileNode->flags & 0x10) != 0) {
            restoreRaycastable = 1;
            zClass_Class::gwNodeSetRaycastable(
                projectileNode,
                0
            );
        }

        int result = zClass_cls_di::FilterRegionsAgainstSphere(
            g_Player_RuntimeDiScene,
            &runtimeInstance->pos,
            0,
            self->impactProximity,
            1,
            1,
            outHitList
        );

        if (restoreRaycastable != 0) {
            zClass_Class::gwNodeSetRaycastable(
                projectileNode,
                1
            );
        }

        if (allowOwnerOnlyHit == 0 && outHitList->hitCount == 1 &&
            outHitList->hits[0].hitNode == runtimeInstance->ownerNode) {
            result = 1;
        }

        return result == 0 ? 1 : 0;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0a50: OptCatalog::HandleImpactFromRuntimeProbe
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
        void *excludedDamageHandler
    ) {
        int processedAny = 0;
        for (int i = 0; i < hitList->hitCount; ++i) {
            OptCatalogRaycastHitEntry *hit = &hitList->hits[i];
            zClass_NodeFreeListSlot *hitSlot = (zClass_NodeFreeListSlot *)(hit->hitNode);
            if (hitSlot->damageHandler == excludedDamageHandler) {
                continue;
            }

            float damageAmount = self->damage;
            if ((self->flags & kOptCatalogFlagFullProbeDamage) == 0) {
                damageAmount = (1.0f - hit->distance / self->damageFalloffRange) * self->damage;
            }
            damageAmount *= runtimeInstance->spawnScale;

            if ((self->flags & kOptCatalogFlagImmediateProbeImpact) != 0 ||
                g_OptCatalogQueuedImpactCount >= kMaxQueuedImpacts) {
                OptCatalogHitEventPartial *hitEvent = (OptCatalogHitEventPartial *)(void *)(hit);
                InvokeDamageFeedbackAndHitCallback(
                    self,
                    runtimeInstance->ownerNode,
                    &runtimeInstance->pos,
                    hitEvent,
                    damageAmount
                );
            } else {
                OptCatalogQueuedImpactRecord *record =
                    &g_OptCatalogQueuedImpacts[g_OptCatalogQueuedImpactCount];
                record->entry = self;
                record->ownerNode = runtimeInstance->ownerNode;
                record->sourcePos = runtimeInstance->pos;
                record->hit = *hit;
                record->damageAmount = damageAmount;
                ++g_OptCatalogQueuedImpactCount;
            }

            processedAny = 1;
        }

        return processedAny;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0ba0: OptCatalog::CanSpawnThroughRay
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
     * Purpose: test whether a trail segment can continue through a ray hit and
     * compute reflected distance/direction outputs.
     */
    int __fastcall CanSpawnThroughRay(
        OptCatalogEntryDef * self,
        OptCatalogRaycastHitEntry * hit,
        const zVec3 *rayStart,
        const zVec3 *rayEnd,
        float *rayLengthOut,
        float *reflectedLengthOut,
        zVec3 *reflectedDirOut
    ) {
        const float rayLength = zMath::Vec3DeltaLength(
            &hit->pos,
            rayStart
        );
        *rayLengthOut = rayLength;
        if (rayLength == 0.0f) {
            return 2;
        }

        const unsigned int flags = self->flags;
        if ((flags & (1u << 19)) == 0) {
            zClass_NodeFreeListSlot *const hitSlot = (zClass_NodeFreeListSlot *)(hit->hitNode);
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
        zMath::Vec3Reflect(
            (zVec3 *)(void *)(hit),
            &incident,
            reflectedDirOut
        );
        *reflectedLengthOut = zMath::Vec3Normalize(reflectedDirOut);
        return 1;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0ca0: OptCatalog::ReflectAndSortImpactTraceList
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: choose the farthest pending trail target direction and sort
     * pending target slots by projection along that direction.
     */
    void __fastcall ReflectAndSortImpactTraceList(
        OptCatalogTrailRuntimeState * runtime,
        float *targetProjectionScratch,
        zVec3 *directionOut
    ) {
        zVec3 *farthestTarget = directionOut;
        float farthestDistance = 0.0f;
        for (int projectionIndex = 0; projectionIndex < *runtime->pendingSpawnTargetCountPtr;
            ++projectionIndex) {
            zVec3 *const targetPos = runtime->pendingSpawnTargetListPtr[projectionIndex].targetPos;
            const float distance = zMath::Vec3DeltaLength(
                runtime->spawnPos,
                targetPos
            );
            if (distance > farthestDistance) {
                farthestDistance = distance;
                farthestTarget = targetPos;
            }
        }

        zMath::Vec3DirectionTo(
            runtime->spawnPos,
            farthestTarget,
            directionOut
        );

        for (int targetProjectionIndex = 0;
            targetProjectionIndex < *runtime->pendingSpawnTargetCountPtr;
            ++targetProjectionIndex) {
            zVec3 *const targetPos =
                runtime->pendingSpawnTargetListPtr[targetProjectionIndex].targetPos;
            zVec3 delta;
            delta.x = targetPos->x - runtime->spawnPos->x;
            delta.y = targetPos->y - runtime->spawnPos->y;
            delta.z = targetPos->z - runtime->spawnPos->z;
            targetProjectionScratch[targetProjectionIndex] =
                directionOut->x * delta.x + directionOut->y * delta.y + directionOut->z * delta.z;
        }

        int swapped;
        do {
            swapped = 0;
            for (int i = 0; i < *runtime->pendingSpawnTargetCountPtr - 1; ++i) {
                if (targetProjectionScratch[i] > targetProjectionScratch[i + 1]) {
                    PlayerProgressTargetSlotRuntime targetSwap =
                        runtime->pendingSpawnTargetListPtr[i];
                    runtime->pendingSpawnTargetListPtr[i] =
                        runtime->pendingSpawnTargetListPtr[i + 1];
                    runtime->pendingSpawnTargetListPtr[i + 1] = targetSwap;

                    const float projectionSwap = targetProjectionScratch[i];
                    targetProjectionScratch[i] = targetProjectionScratch[i + 1];
                    targetProjectionScratch[i + 1] = projectionSwap;
                    swapped = 1;
                }
            }
        } while (swapped != 0);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0e20: OptCatalog::ComputeTrailImpactResponse
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: raycast a trail segment against the runtime world, apply
     * damage feedback on hits, play impact audio, and trim segment length to
     * the selected hit.
     */
    int __fastcall ComputeTrailImpactResponse(
        OptCatalogEntryDef * self,
        OptCatalogTrailRuntimeState * trailRuntime,
        OptCatalogTrailNodeSlot * segment,
        const zVec3 *targetPos
    ) {
        SetDamageMaskSlotIndex(self->damageMaskSlotIndex);
        zClass_cls_di::SetStopAfterFirstHit(0x40000);
        zClass_Class::gwNodeSetRaycastable(
            trailRuntime->projectileNode,
            0
        );

        PlayerProbeSampleCandidateBuffer rayData = {0};
        const int raycastResult = zClass_cls_di::RaycastSelectClosestHitBetweenPoints(
            g_OptCatalogRuntimeWorld,
            &segment->pos,
            targetPos,
            &rayData
        );

        zClass_Class::gwNodeSetRaycastable(
            trailRuntime->projectileNode,
            1
        );

        if (raycastResult != 0) {
            segment->scale = zMath::Vec3DeltaLength(
                &segment->pos,
                targetPos
            );
            return 0;
        }

        zClassDiPickCandidateEntry *const selectedHit = &rayData.entries[rayData.candidateCount];
        OptCatalogHitEventPartial *const hitEvent =
            (OptCatalogHitEventPartial *)(void *)(selectedHit);
        zClass_NodeFreeListSlot *const hitSlot = (zClass_NodeFreeListSlot *)(selectedHit->node);

        if (hitSlot->damageHandler != 0) {
            const double phase = (trailRuntime->trailDistance * kOptCatalogPi) / self->range;
            const float computedBlend = (float)((cos(phase) + 1.0) * 0.5);
            trailRuntime->trailBlend = computedBlend;
            if (computedBlend > kOptCatalogTrailDamageBlendLimit) {
                trailRuntime->trailBlend = kOptCatalogTrailDamageBlendLimit;
            }

            const float damageAmount = trailRuntime->spawnScale * self->damage *
                                       g_OptCatalogRuntimeDeltaTime * trailRuntime->trailBlend;
            InvokeDamageFeedbackAndHitCallback(
                self,
                trailRuntime->projectileNode,
                &segment->pos,
                hitEvent,
                damageAmount
            );

            int impactSlot = 0;
            if (hitEvent->surfaceRef != 0) {
                impactSlot = hitEvent->surfaceRef->impactSlot;
            }
            PlayImpactSound(
                self,
                hitEvent,
                impactSlot,
                1.0f
            );
        }

        segment->scale = zMath::Vec3DeltaLength(
            &segment->pos,
            &selectedHit->hitPos
        );
        return 1;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0f70: OptCatalog::UpdateTrailSegmentVisual
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: activate and transform a trail segment node from its recovered
     * position, direction, and scale state.
     */
    void __fastcall UpdateTrailSegmentVisual(
        OptCatalogTrailNodeSlot * segment
    ) {
        zClass_Class::gwNodeSetActive(
            segment->node,
            1
        );
        zClass_Object3D::gwObject3DSetPosition(
            segment->node,
            segment->pos.x,
            segment->pos.y,
            segment->pos.z
        );

        const float yaw = (float)(atan2(
            -segment->dir.x,
            -segment->dir.z
        ));
        const float pitch = (float)(asin(segment->dir.y));
        zClass_Object3D::gwObject3DSetRotation(
            segment->node,
            pitch,
            yaw,
            0.0f
        );
        zClass_Object3D::gwObject3DSetScale(
            segment->node,
            1.0f,
            1.0f,
            segment->scale
        );
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b0fd0: OptCatalog::PlayImpactSound
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
     * Purpose: choose and play an impact sound sample at the hit position.
     */
    void __fastcall PlayImpactSound(
        OptCatalogEntryDef * self,
        OptCatalogHitEventPartial * hitEvent,
        int impactSlot,
        float gainScale
    ) {
        OptCatalogFxSpec *const impactSpec = &self->impactFxTable[impactSlot];
        const int soundCount = impactSpec->soundCount;
        if (soundCount == 0) {
            return;
        }

        const int soundIndex = (rand() * soundCount) >> 15;
        zSndSample *const sample = impactSpec->soundSamples[soundIndex];
        sample->PlayA3D(
            &hitEvent->hitPos,
            gainScale,
            0
        );
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b1030: OptCatalog::PlayBounceSound
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
     * Purpose: choose and play a bounce sound sample at the raycast hit.
     */
    void __fastcall PlayBounceSound(
        OptCatalogEntryDef * self,
        OptCatalogRaycastHitEntry * hitEvent,
        int impactSlot,
        float gainScale
    ) {
        OptCatalogFxSpec *const impactSpec = &self->impactFxTable[impactSlot];
        const int soundCount = impactSpec->bounceSoundCount;
        if (soundCount == 0) {
            return;
        }

        const int soundIndex = (rand() * soundCount) >> 15;
        zSndSample *const sample = impactSpec->bounceSoundSamples[soundIndex];
        sample->PlayA3D(
            &hitEvent->pos,
            gainScale,
            0
        );
    }
} // namespace OptCatalog
namespace zWeapon {
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
            (zZbdSectionCallback)(&zWeapon::OnWeaponsSectionPreLoad),
            (zZbdSectionCallback)(&zWeapon::OnWeaponsSectionDataReady),
            0x3e8,
            0
        );
    }

    return 0;
}
} // namespace zWeapon
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
} // namespace zWeapon
namespace zWeapon {
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
} // namespace zWeapon
namespace OptCatalog {
/**
     * Reimplements 0x4b1180: OptCatalog::Shutdown
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
     * Purpose: public shutdown wrapper for OptCatalog runtime cleanup.
     */
    int Shutdown() {
        ShutdownCore();
        return 0;
    }
} // namespace OptCatalog
namespace zWeapon {
/**
     * Reimplements 0x4b1190: zWeapon::LoadOptCatalogFromPath
     * (D:\Proj\GameZRecoil\zWeapon\zwep_init.c).
     * Purpose: load weapons.zrd, build the OptCatalog entry table, initialize
     * runtime storage, and publish the loaded runtime globals.
     */
    int __fastcall LoadOptCatalogFromPath(
        zClass_NodePartial * worldNode,
        const char *path,
        int networkState,
        zWeaponOptCatalogEntryCallback entryCallback
    ) {
        g_OptCatalogRuntimeWorld = worldNode;
        Light::InitThermalGlowPool();

        zReader::Node *const rootNode = zReader::LoadNodeFromPath(
            path,
            0,
            0
        );
        g_OptCatalogLoadedTreeRoot = rootNode;
        if (rootNode == 0) {
            zError::ReportOld(
                0x200,
                "D:\\Proj\\GameZRecoil\\zWeapon\\zwep_init.c",
                0xc6,
                g_HudSensorTracker_ReadFileFailedFmt,
                path
            );
            return -1;
        }

        zReader::Node *const versionNode = zReader_GetNamedNode(
            rootNode,
            "VERSION"
        );
        if (versionNode == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zWeapon\\zwep_init.c",
                0xdb,
                "No ZWEP version found"
            );
            return -1;
        }

        int version = 0;
        zReader::ReadNamedInt(
            rootNode,
            "VERSION",
            &version
        );
        if (version != kOptCatalogRequiredVersion) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zWeapon\\zwep_init.c",
                0xd3,
                "Incorrect ZWEP version (found %d, wanted %d)",
                version,
                kOptCatalogRequiredVersion
            );
            return -1;
        }

        LoadNamedSoundSample(
            rootNode,
            "LOCK_ON_WARNING",
            &g_OptCatalogSndLockOnWarning
        );
        LoadNamedSoundSample(
            rootNode,
            "NO_AMMO_WARNING",
            &g_OptCatalogSndTriggerInactive
        );
        LoadNamedSoundSample(
            rootNode,
            "TRIGGER_INACTIVE",
            &g_OptCatalogSndWeaponInactive
        );
        LoadNamedSoundSample(
            rootNode,
            "WEAPON_INACTIVE",
            &g_OptCatalogSndNoAmmoWarning
        );
        zReader::ReadNamedFloat(
            rootNode,
            "MAX_CRATER_RADIUS",
            &g_OptCatalogMaxCraterRadius
        );

        zReader::Node *const ballisticsNode = zReader_GetNamedNode(
            rootNode,
            "BALLISTICS"
        );
        if (ballisticsNode != 0 && ballisticsNode->type == zReader::ZRDR_NODE_ARRAY) {
            const int ballisticsCount = zReaderArrayCount(ballisticsNode);
            g_OptCatalog_EntryCount = (ballisticsCount - 1) / 2;
            g_OptCatalog_EntryTable =
                (OptCatalogEntryDef *)(calloc(
                    g_OptCatalog_EntryCount,
                    sizeof(OptCatalogEntryDef)
                ));

            for (int itemIndex = 1, entryIndex = 0; itemIndex < ballisticsCount;
                itemIndex += 2, ++entryIndex) {
                OptCatalogEntryDef *const entry = &g_OptCatalog_EntryTable[entryIndex];
                const char *const keyName = zReaderArrayString(
                    ballisticsNode,
                    itemIndex
                );
                entry->keyName = (char *)(keyName);
                entry->displayName = entry->keyName;
                entry->description = _strdup(keyName);
                entry->militaryName = _strdup(keyName);
                entry->ordinalIndex = entryIndex;
                entry->ammoOrChargeMax = 50.0f;
                entry->range = 500.0f;
                entry->rangeSq = entry->range * entry->range;
                entry->velocity = 120.0f;
                entry->damage = 0.100000001f;
                entry->timedStatusInterpRate = 1.0f;
                entry->impactFxTable = (OptCatalogFxSpec *)(calloc(
                    g_zRndr_GlobalStringCount,
                    sizeof(OptCatalogFxSpec)
                ));

                zReader::Node *const entryNode = zReader_GetNamedNode(
                    rootNode,
                    keyName
                );
                if (entryNode != 0) {
                    const char *stringValue = zReader::ReadNamedString(
                        entryNode,
                        "NAME"
                    );
                    if (stringValue != 0) {
                        entry->displayName = (char *)(stringValue);
                    }

                    stringValue = zReader::ReadNamedString(
                        entryNode,
                        "DESC"
                    );
                    if (stringValue != 0) {
                        free(entry->description);
                        entry->description =
                            _strdup(zLoc::ResolveMessageKeyOrFallback(stringValue));
                    }

                    stringValue = zReader::ReadNamedString(
                        entryNode,
                        "MILITARY_NAME"
                    );
                    if (stringValue != 0) {
                        free(entry->militaryName);
                        entry->militaryName =
                            _strdup(zLoc::ResolveMessageKeyOrFallback(stringValue));
                    }

                    zReader::ReadNamedFloat(
                        entryNode,
                        "ACCELERATION",
                        &entry->acceleration
                    );

                    int intValue = 0;
                    if (zReader::ReadNamedInt(
                        entryNode,
                        "AMMO_LIMIT",
                        &intValue
                    ) != 0) {
                        entry->ammoOrChargeMax = (float)(intValue);
                    }

                    zReader::Node *fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "BEAM"
                    );
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        entry->flags |= kOptCatalogFlagTrailRuntime;
                        if (zReaderArrayCount(fieldNode) > 1) {
                            entry->velocity = 1.0f / zReaderArrayFloat(
                                fieldNode,
                                1
                            );
                        }
                        if (zReaderArrayCount(fieldNode) > 2) {
                            entry->timedStatusInterpRate = zReaderArrayFloat(
                                fieldNode,
                                2
                            );
                        }
                        if (zReaderArrayCount(fieldNode) > 3) {
                            SetFlagFromBool(
                                entry->flags,
                                1u,
                                zReaderArrayInt(fieldNode, 3)
                            );
                        }
                    }

                    LoadNamedBoolFlag(
                        entryNode,
                        "CATCHES_FIRE",
                        entry,
                        kOptCatalogFlagImmediateProbeImpact
                    );

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "CRATER"
                    );
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadRadiusRange(
                            fieldNode,
                            entry
                        );
                    }

                    zReader::ReadNamedFloat(
                        entryNode,
                        "DAMAGE",
                        &entry->damage
                    );
                    float floatValue = 0.0f;
                    if (zReader::ReadNamedFloat(entryNode, "DETONATION_DISTANCE", &floatValue) !=
                        0) {
                        entry->detonationDistSq = floatValue * floatValue;
                    }

                    LoadNamedBoolFlag(
                        entryNode,
                        "EXPIRES",
                        entry,
                        kOptCatalogFlagExpires
                    );
                    if (zReader::ReadNamedFloat(entryNode, "FIRE_RATE", &floatValue) != 0 &&
                        floatValue != 0.0f) {
                        entry->fireRateInterval = 1.0f / floatValue;
                    }
                    LoadNamedBoolFlag(
                        entryNode,
                        "FIXED_ROTATE",
                        entry,
                        kOptCatalogFlagFixedRotate
                    );

                    if ((entry->flags & kOptCatalogFlagLockOn) == 0) {
                        zReader::ReadNamedFloat(
                            entryNode,
                            "GRAVITY",
                            &entry->gravity
                        );
                    }

                    if (zReader::ReadNamedFloat(
                        entryNode,
                        "IMPACT_PROXIMITY",
                        &floatValue
                    ) != 0) {
                        entry->impactProximity = floatValue;
                        entry->damageFalloffRange = floatValue * floatValue;
                    }

                    zReader::ReadNamedInt(
                        entryNode,
                        "IMPACT_TYPE",
                        &entry->damageMaskSlotIndex
                    );
                    LoadNamedBoolFlag(
                        entryNode,
                        "INSTANT",
                        entry,
                        kOptCatalogFlagInstant
                    );

                    if (zReader::ReadNamedFloat(
                        entryNode,
                        "LOCK_ON",
                        &entry->lockOnTime
                    ) != 0) {
                        entry->flags |= kOptCatalogFlagLockOn;
                    }
                    LoadNamedBoolFlag(
                        entryNode,
                        "LOCK_ON_LEAD",
                        entry,
                        kOptCatalogFlagLockOnLead
                    );

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "MINE"
                    );
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY &&
                        zReaderArrayCount(fieldNode) > 1) {
                        SetFlagFromBool(
                            entry->flags,
                            kOptCatalogFlagFullProbeDamage,
                            zReaderArrayInt(fieldNode, 1)
                        );
                        if (zReaderArrayInt(
                            fieldNode,
                            1
                        ) != 0) {
                            entry->flags |= 1u;
                        }
                    }

                    LoadNamedBoolFlag(
                        entryNode,
                        "MULTI_TARGET",
                        entry,
                        kOptCatalogFlagMultiTarget
                    );

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "QUICKSAND"
                    );
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY &&
                        zReaderArrayCount(fieldNode) > 1 && zReaderArrayInt(
                            fieldNode,
                            1
                        ) != 0) {
                        entry->flags |= kOptCatalogFlagQuickSandImpact;
                        LoadRadiusRange(
                            fieldNode,
                            entry
                        );
                    }

                    if (zReader::ReadNamedFloat(
                        entryNode,
                        g_zEffectAnim_TokenRange,
                        &entry->range
                    ) != 0) {
                        entry->rangeSq = entry->range * entry->range;
                    }
                    LoadNamedBoolFlag(
                        entryNode,
                        "RELATIVE_SPEED",
                        entry,
                        kOptCatalogFlagRelativeSpeed
                    );
                    LoadNamedBoolFlag(
                        entryNode,
                        "REMOTE_DETONATE",
                        entry,
                        kOptCatalogFlagRemoteDetonate
                    );

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "TETHER_GUIDED"
                    );
                    if (fieldNode != 0) {
                        entry->flags |= kOptCatalogFlagTetherGuided;
                    }

                    entry->turnRate = 0.159999996f;
                    zReader::ReadNamedFloat(
                        entryNode,
                        "TURN_RATE",
                        &entry->turnRate
                    );
                    zReader::ReadNamedFloat(
                        entryNode,
                        "TURN_SUSPEND_TIME",
                        &entry->turnSuspendTime
                    );
                    entry->pitchRate = 0.159999996f;
                    zReader::ReadNamedFloat(
                        entryNode,
                        "PITCH_RATE",
                        &entry->pitchRate
                    );

                    if ((entry->flags & kOptCatalogFlagTrailRuntime) == 0) {
                        zReader::ReadNamedFloat(
                            entryNode,
                            "VELOCITY",
                            &entry->velocity
                        );
                    }
                    LoadNamedBoolFlag(
                        entryNode,
                        "RELOAD",
                        entry,
                        kOptCatalogFlagReload
                    );
                    entry->killVerbString = 0;

                    if (entryCallback != 0) {
                        entryCallback(
                            entryNode,
                            entry
                        );
                    }

                    OptCatalog::LoadFxSpecFromReaderNode(
                        entryNode,
                        &entry->fireFxSpec,
                        "FIRE"
                    );
                    OptCatalog::LoadFxSpecFromReaderNode(
                        entryNode,
                        &entry->flyoutFxSpec,
                        "FLYOUT"
                    );

                    if (zReader::ReadNamedInt(
                        entryNode,
                        "FLYOUT_HEALTH",
                        &intValue
                    ) != 0) {
                        entry->flags |= kOptCatalogFlagImpactWhenScaleExpired;
                        entry->flyoutHealth = (float)(intValue);
                        if (entry->attachCloneTemplateNode != 0) {
                            zClass_Class::gwNodeSetRaycastable(
                                entry->attachCloneTemplateNode,
                                0
                            );
                        }
                    }

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "IMPACT"
                    );
                    if (fieldNode != 0 && entry->impactFxTable != 0) {
                        LoadImpactFxTable(
                            fieldNode,
                            entry
                        );
                    }

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "FREEZE"
                    );
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadTimedStatusBlock(
                            fieldNode,
                            entry
                        );
                        entry->flags |= kOptCatalogFlagTimedStatusSubtractive;
                    }

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "HEAT"
                    );
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadTimedStatusBlock(
                            fieldNode,
                            entry
                        );
                        entry->flags |= kOptCatalogFlagHeatTimedStatus;
                    }

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "DESIGNATE"
                    );
                    if (fieldNode != 0 && fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadDesignateStatusBlock(
                            fieldNode,
                            entry
                        );
                    }

                    stringValue = ReadNamedArrayString(
                        entryNode,
                        "KILL_ANIMATION",
                        1
                    );
                    if (stringValue != 0) {
                        entry->damageContextEffect = zEffectAnim::FindEntryByName(stringValue);
                    }

                    stringValue = ReadNamedArrayString(
                        entryNode,
                        "DAMAGE_ANIMATION",
                        1
                    );
                    if (stringValue != 0) {
                        entry->damageFeedbackVariantCount = 1;
                        entry->damageFeedbackVariants[0].minFeedbackScale = 1.0f;
                        entry->damageFeedbackVariants[0].effect =
                            zEffectAnim::FindEntryByName(stringValue);
                    }

                    fieldNode = zReader_GetNamedNode(
                        entryNode,
                        "DAMAGE_ANIM_ON_HEALTH"
                    );
                    if (g_zVideo_ActiveRendererPath != 0 && fieldNode != 0 &&
                        fieldNode->type == zReader::ZRDR_NODE_ARRAY) {
                        LoadDamageFeedbackOnHealth(
                            fieldNode,
                            entry
                        );
                    }
                }

                if (entry->gravity != 0.0f) {
                    entry->trailSegmentTimeSec =
                        (entry->velocity * entry->velocity) / (entry->gravity * 2.0f);
                    entry->velocity = FastSqrtApprox(entry->gravity * entry->range * 2.0f);
                }

                if (entry->attachCloneTemplateNode != 0) {
                    zClass_Class::gwNodeSetActive(
                        entry->attachCloneTemplateNode,
                        1
                    );
                    if (zClass::AnyNodeMatchesPredicateRecursive(
                            entry->attachCloneTemplateNode,
                            zClass_Node::HasRenderableDiPredicate
                        ) == 0) {
                        entry->flags |= kOptCatalogFlagSkipTrailSegmentLighting;
                    } else {
                        entry->flags &= ~kOptCatalogFlagSkipTrailSegmentLighting;
                    }
                }

                if ((entry->flags & kOptCatalogFlagTrailRuntime) == 0 && entry->velocity != 0.0f &&
                    entry->fireRateInterval != 0.0f) {
                    g_OptCatalogRuntimeInstanceCount +=
                        (int)(floor(entry->range / entry->velocity / entry->fireRateInterval)) + 1;
                }
            }
        }

        SetupRuntimeInstancePool();

        zClass_NodePartial *const callbackNode = zClass_Object3D::gwObject3DInit();
        if (callbackNode == 0) {
            zError::ReportOld(
                0x400,
                "D:\\Proj\\GameZRecoil\\zWeapon\\zwep_init.c",
                0x2d9,
                "Error allocating weapon_tick callback"
            );
            g_OptCatalogNetworkOptionState = networkState;
            return 0;
        }

        zClass_Class::gwNodeSetPriority(
            callbackNode,
            3
        );
        zClass_Class::gwNodeSetActionCallback(
            callbackNode,
            ActionCallbackPtr(&OptCatalog::ProcessRuntimeInstances)
        );
        g_OptCatalogNetworkOptionState = networkState;
        return 0;
    }
} // namespace zWeapon
namespace zWeapon {
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
namespace OptCatalog {
/**
     * Reimplements 0x4b1d90: OptCatalog::ShutdownCore.
     * Purpose: release loaded OptCatalog entries, runtime pools, reader tree,
     * and reset runtime globals to initialization defaults.
     */
    int ShutdownCore() {
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
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b1ec0: OptCatalog::CreateTrailRuntimeState
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: allocate trail runtime state, create inactive BeamReflect
     * segment nodes, and attach them to the OptCatalog runtime world.
     */
    OptCatalogTrailRuntimeState *__fastcall CreateTrailRuntimeState(
        OptCatalogEntryDef * entry,
        zClass_NodePartial * projectileNode,
        zTag4Partial * variantTagPtr,
        void *reserved,
        zVec3 *spawnPos,
        zVec3 *spawnDir,
        int segmentCount
    ) {
        (void)reserved;

        OptCatalogTrailRuntimeState *const runtime =
            (OptCatalogTrailRuntimeState *)(calloc(
                1,
                sizeof(OptCatalogTrailRuntimeState)
            ));
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
            sprintf(
                nodeName,
                g_zWeapon_BeamReflectNameFmt,
                i
            );
            zClass_Class::gwNodeSetName(
                node,
                nodeName
            );
            zClass_Class::gwNodeSetActive(
                node,
                0
            );
            if ((entry->flags & kOptCatalogFlagSkipTrailSegmentLighting) == 0) {
                zClass_Object3D::gwObject3DSetLitFlag(
                    node,
                    1
                );
            }
            zClass_Class::AddChild(
                g_OptCatalogRuntimeWorld,
                node
            );
        }

        return runtime;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b1f90: OptCatalog::FreeTrailRuntimeStateStorage
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: release trail runtime-state storage owned by player and
     * turret cleanup paths.
     */
    void __fastcall FreeTrailRuntimeStateStorage(void *trailRuntimeState) {
        free(trailRuntimeState);
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b1fa0: OptCatalog::LoadFxSpecFromReaderNode
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp).
     * Purpose: load one named impact effect spec from a zReader node.
     */
    void __fastcall LoadFxSpecFromReaderNode(
        zReader::Node * parentNode,
        OptCatalogFxSpec * spec,
        const char *childName
    ) {
        zReader::Node *const specNode = zReader_GetNamedNode(
            parentNode,
            childName
        );
        if (specNode == 0) {
            return;
        }

        zReader::Node *fieldNode = zReader_GetNamedNode(
            specNode,
            "EFFECT"
        );
        if (fieldNode != 0) {
            if (zReaderArrayCount(fieldNode) > 1) {
                spec->effectTemplateIndex =
                    zEffect::FindTemplateIndexByName(zReaderArrayString(
                        fieldNode,
                        1
                    ));
            }
        } else {
            fieldNode = zReader_GetNamedNode(
                specNode,
                "MODEL"
            );
            if (fieldNode != 0 && zReaderArrayCount(fieldNode) > 1) {
                spec->modelNode = zClass::FindByTypeAndName(
                    6,
                    zReaderArrayString(fieldNode, 1)
                );
            }
        }

        fieldNode = zReader_GetNamedNode(
            specNode,
            "ANIMATION_ATTACHED"
        );
        if (fieldNode != 0 && zReaderArrayCount(fieldNode) > 1) {
            spec->attachedAnimationEntry =
                zEffectAnim::FindEntryByName(zReaderArrayString(
                    fieldNode,
                    1
                ));
        }

        fieldNode = zReader_GetNamedNode(
            specNode,
            "MODEL_ANIMATION"
        );
        if (fieldNode != 0 && zReaderArrayCount(fieldNode) > 1) {
            spec->modelAnimationEntry =
                zEffectAnim::FindEntryByName(zReaderArrayString(
                    fieldNode,
                    1
                ));
        }

        fieldNode = zReader_GetNamedNode(
            specNode,
            "ANIMATION"
        );
        if (fieldNode != 0 && zReaderArrayCount(fieldNode) > 1) {
            spec->animationEntry = zEffectAnim::FindEntryByName(zReaderArrayString(
                fieldNode,
                1
            ));
        }

        fieldNode = zReader_GetNamedNode(
            specNode,
            "RANDOM_ROTATE"
        );
        if (fieldNode != 0) {
            spec->flags =
                (((unsigned int)(zReaderArrayInt(
                    fieldNode,
                    1
                )) ^ spec->flags) & 1) ^ spec->flags;
        }

        fieldNode = zReader_GetNamedNode(
            specNode,
            g_HudZrd_Key_Sound
        );
        if (fieldNode != 0) {
            const int count = zReaderArrayCount(fieldNode);
            spec->soundCount = count - 1;
            if (count > 1) {
                zSndSample **sample = spec->soundSamples;
                for (int i = 1; i < count; ++i, ++sample) {
                    *sample = zSnd::FindSampleByName(zReaderArrayString(
                        fieldNode,
                        i
                    ));
                }
            }
        }

        fieldNode = zReader_GetNamedNode(
            specNode,
            g_zEffectAnim_TokenBounceSound
        );
        if (fieldNode != 0) {
            const int count = zReaderArrayCount(fieldNode);
            spec->bounceSoundCount = count - 1;
            if (count > 1) {
                zSndSample **sample = spec->bounceSoundSamples;
                for (int i = 1; i < count; ++i, ++sample) {
                    *sample = zSnd::FindSampleByName(zReaderArrayString(
                        fieldNode,
                        i
                    ));
                }
            }
        }
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b2130: OptCatalog::CreateTrailSegmentNodeFromTemplate
     * (D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp)
     * Purpose: allocate an active Object3D segment node and attach an optional
     * template child to it.
     */
    zClass_NodePartial *__fastcall CreateTrailSegmentNodeFromTemplate(
        zClass_NodePartial * templateNode
    ) {
        zClass_NodePartial *const parent = zClass_Object3D::gwObject3DInit();
        zClass_Class::gwNodeSetActive(
            parent,
            1
        );
        if (templateNode != 0) {
            zClass_Object3D::gwObject3DAddChild(
                parent,
                templateNode
            );
        }

        return parent;
    }
} // namespace OptCatalog
namespace Light {
/**
     * Reimplements 0x4b2160: Light::InitThermalGlowPool
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: allocate the fixed eight-node thermal glow light pool, initialize
     * names, positions, and ranges, then link every node onto the free list.
     */
    int InitThermalGlowPool() {
        for (int i = 0; i < 8; ++i) {
            zClass_NodePartial *const light = zClass_Light::gwLightNew();
            zClass_Class::gwNodeSetName(
                light,
                g_zWeapon_ThermalGlowLabel
            );
            zClass_Light::gwLightSetPosition(
                light,
                0.0f,
                0.0f,
                0.0f
            );
            zClass_Light::gwLightSetRange(
                light,
                0.1f,
                0.2f
            );
            light->callbackContext = g_OptCatalogThermalGlowFreeList;
            g_OptCatalogThermalGlowFreeList = light;
        }

        return 1;
    }
} // namespace Light
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
namespace Light {
/**
     * Reimplements 0x4b21e0: Light::DestroyThermalGlowPool
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: delete every thermal glow light still on the free list and clear
     * the pool head.
     */
    int DestroyThermalGlowPool() {
        zClass_NodePartial *node = g_OptCatalogThermalGlowFreeList;
        while (node != 0) {
            zClass_NodePartial *next = node->callbackContext;
            node->callbackContext = 0;
            zClass_Class::DeleteNodeByType(node);
            node = next;
        }

        g_OptCatalogThermalGlowFreeList = 0;
        return 1;
    }
} // namespace Light
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
namespace Light {
/**
     * Reimplements 0x4b2520: Light::AllocFromFreeListAndAttach
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: pop a thermal glow light from the free list, reset its range and
     * specular color, and attach it to the active runtime world.
     */
    zClass_NodePartial *__fastcall AllocFromFreeListAndAttach(
        zColorRgb * specularColor
    ) {
        zClass_NodePartial *const light = g_OptCatalogThermalGlowFreeList;
        if (light == 0) {
            return 0;
        }

        g_OptCatalogThermalGlowFreeList = light->callbackContext;
        zClass_Light::gwLightSetRange(
            light,
            0.1f,
            0.2f
        );
        zClass_Light::gwLightSetSpecularColor(
            light,
            specularColor->red,
            specularColor->green,
            specularColor->blue
        );
        zClass_World::AddLight(
            g_OptCatalogRuntimeWorld,
            light
        );
        return light;
    }
} // namespace Light
namespace Light {
/**
     * Reimplements 0x4b2570: Light::ReturnToFreeList
     * (D:\Proj\GameZRecoil\zClass\Light.c).
     * Purpose: reset a thermal glow light's range, detach it from the runtime
     * world, and push it back onto the thermal glow free list.
     */
    void __fastcall ReturnToFreeList(zClass_NodePartial * lightNode) {
        zClass_Light::gwLightSetRange(
            lightNode,
            0.1f,
            0.2f
        );
        zClass_World::RemoveLight(
            g_OptCatalogRuntimeWorld,
            lightNode
        );
        lightNode->callbackContext = g_OptCatalogThermalGlowFreeList;
        g_OptCatalogThermalGlowFreeList = lightNode;
    }
} // namespace Light
namespace zClass_Node {
/**
     * Reimplements 0x4b25a0: zClass_Node::SetDamageHitCallback
     * Source: D:\Proj\GameZRecoil\zWeapon\OptCatalog.c
     * Purpose: create or reuse a damage handler, install its hit callback, and
     * propagate the handler through the node subtree.
     */
    int __fastcall SetDamageHitCallback(
        void *context,
        zClass_NodePartial *node,
        void *callback
    ) {
        OptCatalogDamageHandlerPartial *handler =
            (OptCatalogDamageHandlerPartial *)(((zClass_NodeFreeListSlot *)(node))
                ->damageHandler);
        if (handler == 0) {
            handler = (OptCatalogDamageHandlerPartial *)(calloc(
                1,
                sizeof(OptCatalogDamageHandlerPartial)
            ));
        } else if (handler->hitContext != 0) {
            return 0;
        }

        handler->hitCallback = context;
        handler->hitContext = callback;
        AssignDamageHandlerRecursiveIfMissing(
            node,
            handler
        );
        zClass_Class::gwNodeSetHasHitCallback(
            node,
            1
        );
        return 0;
    }
} // namespace zClass_Node
namespace zClass_Node {
/**
     * Reimplements 0x4b25f0: zClass_Node::AssignDamageHandlerRecursiveIfMissing
     * Source: D:\Proj\GameZRecoil\zWeapon\OptCatalog.c
     * Purpose: assign a shared damage handler to nodes in a child-list subtree
     * that do not already own one.
     */
    void __fastcall AssignDamageHandlerRecursiveIfMissing(
        zClass_NodePartial * node,
        OptCatalogDamageHandlerPartial * handler
    ) {
        if (((zClass_NodeFreeListSlot *)(node))->damageHandler != 0) {
            return;
        }

        if (node->listCountB != 0) {
            for (int i = 0; i < node->listCountB; ++i) {
                AssignDamageHandlerRecursiveIfMissing(
                    node->listB[i],
                    handler
                );
            }
        }

        ((zClass_NodeFreeListSlot *)(node))->damageHandler = handler;
    }
} // namespace zClass_Node
namespace zClass_Node {
/**
     * Reimplements 0x4b2630: zClass_Node::ClearDamageHandler
     * Source: D:\Proj\GameZRecoil\zWeapon\OptCatalog.c
     * Purpose: detach and free a node subtree's shared damage handler.
     */
    int __fastcall ClearDamageHandler(zClass_NodePartial * node) {
        if (node == 0) {
            return 0;
        }

        OptCatalogDamageHandlerPartial *handler =
            (OptCatalogDamageHandlerPartial *)(((zClass_NodeFreeListSlot *)(node))
                ->damageHandler);
        if (handler != 0) {
            ClearDamageHandlerRecursive(
                node,
                handler
            );
            if (handler->hitContext != 0) {
                zClass_Class::gwNodeSetHasHitCallback(
                    node,
                    0
                );
            }
            free(handler);
        }

        return 0;
    }
} // namespace zClass_Node
namespace zClass_Node {
/**
     * Reimplements 0x4b2670: zClass_Node::ClearDamageHandlerRecursive
     * Source: D:\Proj\GameZRecoil\zWeapon\OptCatalog.c
     * Purpose: clear a matching shared damage handler through a node subtree.
     */
    void __fastcall ClearDamageHandlerRecursive(
        zClass_NodePartial * node,
        OptCatalogDamageHandlerPartial * handler
    ) {
        if (node->listCountB != 0) {
            for (int i = 0; i < node->listCountB; ++i) {
                ClearDamageHandlerRecursive(
                    node->listB[i],
                    handler
                );
            }
        }

        if (((zClass_NodeFreeListSlot *)(node))->damageHandler == handler) {
            ((zClass_NodeFreeListSlot *)(node))->damageHandler = 0;
        }
    }
} // namespace zClass_Node
namespace zClass_Node {
/**
     * Reimplements 0x4b26b0: zClass_Node::SetDamageTimerCallback
     * Source: D:\Proj\GameZRecoil\zWeapon\OptCatalog.c
     * Purpose: create or reuse a damage handler, install its timer callback,
     * and propagate the handler through the node subtree.
     */
    int __fastcall SetDamageTimerCallback(
        void *callback,
        zClass_NodePartial *node,
        void *context
    ) {
        OptCatalogDamageHandlerPartial *handler =
            (OptCatalogDamageHandlerPartial *)(((zClass_NodeFreeListSlot *)(node))
                ->damageHandler);
        if (handler == 0) {
            handler = (OptCatalogDamageHandlerPartial *)(calloc(
                1,
                sizeof(OptCatalogDamageHandlerPartial)
            ));
        }

        handler->timerContext = context;
        handler->timerCallback = callback;
        AssignDamageHandlerRecursiveIfMissing(
            node,
            handler
        );
        return 0;
    }
} // namespace zClass_Node
namespace OptCatalog {
/**
     * Reimplements 0x4b26f0: OptCatalog::InvokeDamageFeedbackAndHitCallback
     * Source path: D:\Proj\GameZRecoil\zWeapon\OptCatalog.c
     * Purpose: apply per-hit damage feedback and handler callback state.
     * Behavior: clears current damage context, optionally stamps the damage
     * mask, dispatches health or handler callbacks, captures hit snapshots,
     * selects feedback effects, and counts hits for the tracked owner node.
     */
    int __fastcall InvokeDamageFeedbackAndHitCallback(
        OptCatalogEntryDef * self,
        zClass_NodePartial * damageOwnerNode,
        zVec3 * sourcePos,
        OptCatalogHitEventPartial * hitEvent,
        float damageAmount
    ) {
        int result = 0;
        g_OptCatalog_DamageContextKind = 0;
        g_OptCatalog_DamageContextHitEvent = 0;

        if ((self->flags & kOptCatalogFlagSkipDamageMaskStamp) == 0) {
            ApplyDamageMaskStampOnHit(hitEvent);
        }

        OptCatalogDamageHandlerPartial *const handler = DamageHandlerForNode(hitEvent->hitNode);
        if (handler == 0) {
            return 0;
        }

        if (handler == (OptCatalogDamageHandlerPartial *)(1)) {
            OptCatalogDamageHealthOverlay *const healthOverlay =
                (OptCatalogDamageHealthOverlay *)(hitEvent->hitNode->callbackContext);
            healthOverlay->health -= damageAmount;
        } else if (handler->hitContext != 0) {
            if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
                g_OptCatalog_CapturedDamageSourcePos = *sourcePos;
                g_OptCatalog_CapturedDamageHitPos = hitEvent->hitPos;
            }

            g_OptCatalog_CurrentDamageOwnerOrCtx = damageOwnerNode;
            g_OptCatalogDamageFeedbackIntensityScalar = 1.0f;

            OptCatalogDamageFeedbackCallback feedbackCallback =
                (OptCatalogDamageFeedbackCallback)(g_OptCatalogDamageFeedbackCallback);
            if (feedbackCallback != 0) {
                feedbackCallback(
                    handler,
                    hitEvent->hitNode,
                    damageAmount
                );
            }

            OptCatalogHitCallback hitCallback = (OptCatalogHitCallback)(handler->hitContext);
            result = hitCallback(
                handler->hitCallback,
                self,
                hitEvent,
                damageAmount
            );

            if (g_OptCatalog_DamageContextKind != 0) {
                if (self->damageContextEffect != 0) {
                    ActivateDamageFeedbackEffect(
                        self->damageContextEffect,
                        hitEvent
                    );
                }
            } else if (self->damageFeedbackVariantCount != 0) {
                for (int i = 0; i < self->damageFeedbackVariantCount; ++i) {
                    if (g_OptCatalogDamageFeedbackIntensityScalar <=
                        self->damageFeedbackVariants[i].minFeedbackScale) {
                        ActivateDamageFeedbackEffect(
                            self->damageFeedbackVariants[i].effect,
                            hitEvent
                        );
                        break;
                    }
                }
            }
        }

        if (g_OptCatalogDamageFeedbackTrackedNode == damageOwnerNode) {
            ++g_OptCatalog_DamageFeedbackHitCount;
        }

        return result;
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b2880: OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback
     * Source path: src/GameZRecoil/zWeapon/OptCatalog.c
     * Purpose: capture hit positions and forward damage to the timer callback.
     * Behavior: looks up the hit node damage handler, optionally copies source
     * and hit positions to the captured globals, invokes the timer callback,
     * and returns the callback float result.
     */
    float __fastcall CaptureHitSnapshotAndInvokeDamageTimerCallback(
        zVec3 * sourcePos,
        OptCatalogHitEventPartial * hitEvent,
        float damageAmount
    ) {
        OptCatalogDamageHandlerPartial *handler = DamageHandlerForNode(hitEvent->hitNode);

        if (g_OptCatalog_CaptureHitSnapshotEnabled == 1) {
            g_OptCatalog_CapturedDamageSourcePos = *sourcePos;
            g_OptCatalog_CapturedDamageHitPos = hitEvent->hitPos;
        }

        OptCatalogDamageTimerCallback callback =
            (OptCatalogDamageTimerCallback)(handler->timerCallback);
        return callback(
            handler->timerContext,
            damageAmount
        );
    }
} // namespace OptCatalog
namespace OptCatalog {
/**
     * Reimplements 0x4b28e0: OptCatalog::SetDamageContext
     * Source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp
     * Purpose: publish the active damage-context kind and optional hit event.
     * Behavior: stores the damage-context kind and captures the hit event only
     * when the event and its hit node are non-null.
     */
    void __fastcall SetDamageContext(
        int contextKind,
        OptCatalogHitEventPartial *contextHitEvent
    ) {
        if (contextHitEvent != 0 && contextHitEvent->hitNode != 0) {
            g_OptCatalog_DamageContextHitEvent = contextHitEvent;
        }

        g_OptCatalog_DamageContextKind = contextKind;
    }
} // namespace OptCatalog
namespace DamageFeedback {
/**
     * Reimplements 0x4b2900: DamageFeedback::SetIntensityScalar
     * Source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp
     * Purpose: update the active damage-feedback intensity scalar.
     * Behavior: stores the per-hit damage-feedback intensity scalar used by
     * OptCatalog feedback variant selection.
     */
    void __stdcall SetIntensityScalar(float scalar) {
        g_OptCatalogDamageFeedbackIntensityScalar = scalar;
    }
} // namespace DamageFeedback
namespace OptCatalog {
/**
     * Reimplements 0x4b2910: OptCatalog::GetCapturedHitSourcePtr
     * Source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp
     * Purpose: expose the captured damage source vector buffer.
     * Behavior: returns the captured damage source-position global; callers
     * consume the adjacent captured hit-position vector.
     */
    zVec3 *GetCapturedHitSourcePtr() {
        return &g_OptCatalog_CapturedDamageSourcePos;
    }
} // namespace OptCatalog
namespace HitContext {
/**
     * Reimplements 0x4b2920: HitContext::GetCurrentOwnerOrCtx
     * Source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp
     * Purpose: expose the current OptCatalog damage owner/context pointer.
     * Behavior: returns the current OptCatalog damage owner/context pointer.
     */
    void *GetCurrentOwnerOrCtx() {
        return g_OptCatalog_CurrentDamageOwnerOrCtx;
    }
} // namespace HitContext
namespace OptCatalog_MineIterator {
/**
     * Reimplements 0x4b2930: OptCatalog_MineIterator::Begin
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: ECX is OptCatalogEntryDef*, load activeRuntimeListHead,
     * store it to g_OptCatalog_MineIteratorCursor, and return the same
     * runtime-instance pointer.
     * Data touch: writes the BSS global g_OptCatalog_MineIteratorCursor
     * at 0x56bcb0.
     * Purpose: start iterating the active runtime-instance list for a mine
     * OptCatalog entry.
     */
    OptCatalogRuntimeInstanceStorage *__fastcall Begin(
        OptCatalogEntryDef * entry
    ) {
        g_OptCatalog_MineIteratorCursor = entry->activeRuntimeListHead;
        return entry->activeRuntimeListHead;
    }
} // namespace OptCatalog_MineIterator
namespace OptCatalog_MineIterator {
/**
     * Reimplements 0x4b2940: OptCatalog_MineIterator::Next
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN behavior: read g_OptCatalog_MineIteratorCursor; when non-null,
     * advance through OptCatalogRuntimeInstanceStorage::next, write the new
     * cursor back, and return it; when null, return null without changing the
     * global.
     * Data touch: reads and conditionally writes the BSS global
     * g_OptCatalog_MineIteratorCursor at 0x56bcb0.
     * Purpose: advance the current mine runtime-instance iterator cursor.
     */
    OptCatalogRuntimeInstanceStorage *Next() {
        OptCatalogRuntimeInstanceStorage *result = g_OptCatalog_MineIteratorCursor;
        if (result != 0) {
            result = result->next;
            g_OptCatalog_MineIteratorCursor = result;
        }

        return result;
    }
} // namespace OptCatalog_MineIterator
