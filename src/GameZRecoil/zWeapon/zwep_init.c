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

struct OptCatalogQueuedImpactRecord {
    OptCatalogEntryDef* entry;
    CZNodePartial* ownerNode;
    zVec3 sourcePos;
    OptCatalogRaycastHitEntry hit;
    float damageAmount;
    unsigned char unknown_40[4];
};

/**
 * Deferred OptCatalog impact queue. Retail keeps the count and the 64 records
 * in one object (0x77896c..0x779a70): HandleImpactFromRuntimeProbe re-reads
 * the count after every record store, as VC5 does for stores into one aggregate.
 */
struct OptCatalogQueuedImpactQueue {
    int count;
    OptCatalogQueuedImpactRecord records[64];
};

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogruntimeworld
 * @recoil-artifact defines .data recoil:data:0x778920: g_OptCatalogRuntimeWorld.
 * BN xrefs include runtime allocation/recycling, projectile raycasts, trail
 * impact probes, zWeapon load/init/shutdown, and thermal glow light attach.
 * Purpose: active world node used by OptCatalog runtime projectiles, trail
 * probes, and glow-light attachment.
 */
CZNodePartial* g_OptCatalogRuntimeWorld = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-entrycount
 * @recoil-artifact defines .data recoil:data:0x778924: g_OptCatalog_EntryCount.
 * BN xrefs include OptCatalog lookup helpers, ProcessRuntimeInstances,
 * zWeapon::Init, zWeapon::LoadOptCatalogFromPath, and ShutdownCore.
 * Purpose: number of loaded OptCatalog entries in the runtime catalog table.
 */
int g_OptCatalog_EntryCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-entrytable
 * @recoil-artifact defines .data recoil:data:0x778928: g_OptCatalog_EntryTable.
 * BN xrefs include OptCatalog lookup helpers, ProcessRuntimeInstances,
 * zWeapon::Init, zWeapon::LoadOptCatalogFromPath, and ShutdownCore.
 * Purpose: owning pointer for the loaded OptCatalog entry array walked by
 * runtime processing and lookup helpers.
 */
OptCatalogEntryDef* g_OptCatalog_EntryTable = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogruntimeinstancecount
 * @recoil-artifact defines .data recoil:data:0x77892c: g_OptCatalogRuntimeInstanceCount.
 * Purpose: stores the configured runtime projectile pool count loaded with
 * the OptCatalog.
 */
int g_OptCatalogRuntimeInstanceCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogruntimeinstancepool
 * @recoil-artifact defines .data recoil:data:0x778930: g_OptCatalogRuntimeInstancePool.
 * Purpose: owns the allocated runtime projectile pool backing the free list
 * and active per-entry runtime lists.
 */
void* g_OptCatalogRuntimeInstancePool = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogfreeruntimeinstancelist
 * @recoil-artifact defines .data recoil:data:0x778934: g_OptCatalogFreeRuntimeInstanceList.
 * Purpose: head of the free runtime projectile instance list shared by
 * allocation, recycling, and shutdown.
 */
OptCatalogRuntimeInstanceStorage* g_OptCatalogFreeRuntimeInstanceList = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogthermalglowfreelist
 * @recoil-artifact defines .data recoil:data:0x778938: g_OptCatalogThermalGlowFreeList.
 * Purpose: stores the head of the pooled thermal glow light free list shared
 * by OptCatalog runtime effects and the Light lifecycle functions.
 */
CZNodePartial* g_OptCatalogThermalGlowFreeList = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalognetworkoptionstate
 * @recoil-artifact defines .data recoil:data:0x77893c: g_OptCatalogNetworkOptionState.
 * BN xrefs: zWeapon::LoadOptCatalogFromPath initializes the state;
 * OptCatalog::AllocRuntimeInstance and ProcessRuntimeInstances read it for
 * network-runtime behavior.
 * Purpose: active OptCatalog network option state loaded with the catalog.
 */
int g_OptCatalogNetworkOptionState = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-captureddamagesourcepos
 * @recoil-artifact defines .data recoil:data:0x778940: g_OptCatalog_CapturedDamageSourcePos.
 * Purpose: Stores g OptCatalog CapturedDamageSourcePos data used by effects_weapons.optcatalog_damage_feedback_data.
 */
zVec3 g_OptCatalog_CapturedDamageSourcePos = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-captureddamagehitpos
 * @recoil-artifact defines .data recoil:data:0x77894c: g_OptCatalog_CapturedDamageHitPos.
 * Purpose: Stores g OptCatalog CapturedDamageHitPos data used by effects_weapons.optcatalog_damage_feedback_data.
 */
zVec3 g_OptCatalog_CapturedDamageHitPos = { 0 };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-currentdamageownerorctx
 * @recoil-artifact defines .data recoil:data:0x778958: g_OptCatalog_CurrentDamageOwnerOrCtx.
 * Purpose: Stores g OptCatalog CurrentDamageOwnerOrCtx data used by effects_weapons.optcatalog_damage_feedback_data.
 */
void* g_OptCatalog_CurrentDamageOwnerOrCtx = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogpendingspawntargetcountptr
 * @recoil-artifact defines .data recoil:data:0x77895c: g_OptCatalogPendingSpawnTargetCountPtr.
 * Purpose: transient pointer to the pending target count consumed by
 * OptCatalog runtime spawn and trail activation.
 */
int* g_OptCatalogPendingSpawnTargetCountPtr = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogpendingspawntargetlistptr
 * @recoil-artifact defines .data recoil:data:0x778960: g_OptCatalogPendingSpawnTargetListPtr.
 * Purpose: transient pointer to pending target slots consumed by OptCatalog
 * runtime spawn and trail activation.
 */
PlayerProgressTargetSlotRuntime* g_OptCatalogPendingSpawnTargetListPtr = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-fallbackimpactprobeenabled
 * @recoil-artifact defines .data recoil:data:0x778964: g_OptCatalog_FallbackImpactProbeEnabled.
 * BN xrefs: OptCatalog::ProcessRuntimeInstance, ProcessRuntimeInstances,
 * zWeapon::Init, and OptCatalog::ShutdownCore.
 * Purpose: enables deferred fallback impact probes for runtime projectile
 * processing.
 */
int g_OptCatalog_FallbackImpactProbeEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-capturehitsnapshotenabled
 * @recoil-artifact defines .data recoil:data:0x778968: g_OptCatalog_CaptureHitSnapshotEnabled.
 * Purpose: Stores g OptCatalog CaptureHitSnapshotEnabled data used by effects_weapons.optcatalog_damage_feedback_data.
 */
int g_OptCatalog_CaptureHitSnapshotEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogqueuedimpactcount
 * @recoil-artifact defines .data recoil:data:0x77896c: g_OptCatalogQueuedImpactQueue.
 * Purpose: deferred OptCatalog impact queue (count at +0, 64 records at +4)
 * drained by ProcessRuntimeInstances.
 */
OptCatalogQueuedImpactQueue g_OptCatalogQueuedImpactQueue = { 0 };

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogloadedtreeroot
 * @recoil-artifact defines .data recoil:data:0x779a70: g_OptCatalogLoadedTreeRoot.
 * BN xrefs: zWeapon::LoadOptCatalogFromPath stores the loaded root;
 * OptCatalog::ShutdownCore frees it through zReader::Free and
 * clears the pointer.
 * Purpose: owning pointer for the currently loaded OptCatalog zReader tree.
 */
zReader::Node* g_OptCatalogLoadedTreeRoot = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogsndlockonwarning
 * @recoil-artifact defines .data recoil:data:0x779a74: g_OptCatalogSndLockOnWarning.
 * BN xrefs: OptCatalog::ProcessRuntimeInstances and
 * zWeapon::LoadOptCatalogFromPath. BN currently types the data symbol as
 * int32_t, but all use sites consume it as a zSndSample pointer.
 * Purpose: lock-on warning sample played by the runtime tick gate.
 */
zSndSample* g_OptCatalogSndLockOnWarning = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-f-0x779a78
 * @recoil-artifact defines .data recoil:data:0x779a78: g_OptCatalogLockOnWarningGateTimeSec.
 * BN xrefs: OptCatalog::ProcessRuntimeInstances, zWeapon::Init, and
 * zWeapon::OnWeaponsSectionDataReady.
 * Purpose: throttles lock-on warning playback during OptCatalog runtime ticks.
 */
float g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-f-0x779a7c
 * @recoil-artifact defines .data recoil:data:0x779a7c: g_OptCatalogMaxCraterRadius.
 * Purpose: clamps crater and quicksand terrain-deformation event radii.
 */
float g_OptCatalogMaxCraterRadius = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-damagecontextkind
 * @recoil-artifact defines .data recoil:data:0x779a80: g_OptCatalog_DamageContextKind.
 * Purpose: Stores g OptCatalog DamageContextKind data used by effects_weapons.optcatalog_damage_feedback_data.
 */
int g_OptCatalog_DamageContextKind = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-f-0x779a84
 * @recoil-artifact defines .data recoil:data:0x779a84: g_OptCatalog_DamageFeedbackScale.
 * BN xrefs: DamageFeedback::SetIntensityScalar stores this scalar and
 * OptCatalog::InvokeDamageFeedbackAndHitCallback consumes it when selecting
 * damage-feedback effects. Source currently names the variable
 * g_OptCatalogDamageFeedbackIntensityScalar.
 * Purpose: per-hit damage feedback intensity scalar.
 */
float g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-damagecontexthitevent
 * @recoil-artifact defines .data recoil:data:0x779a88: g_OptCatalog_DamageContextHitEvent.
 * Purpose: Stores g OptCatalog DamageContextHitEvent data used by effects_weapons.optcatalog_damage_feedback_data.
 */
void* g_OptCatalog_DamageContextHitEvent = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogsndtriggerinactive
 * @recoil-artifact defines .data recoil:data:0x779a8c: g_OptCatalogSndTriggerInactive.
 * BN xrefs: OptCatalog::PlayTriggerInactiveWarning and
 * zWeapon::LoadOptCatalogFromPath.
 * Purpose: trigger-inactive warning sample loaded with the OptCatalog.
 */
zSndSample* g_OptCatalogSndTriggerInactive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogsndweaponinactive
 * @recoil-artifact defines .data recoil:data:0x779a90: g_OptCatalogSndWeaponInactive.
 * BN xrefs: OptCatalog::PlayWeaponInactiveWarning and
 * zWeapon::LoadOptCatalogFromPath.
 * Purpose: weapon-inactive warning sample loaded with the OptCatalog.
 */
zSndSample* g_OptCatalogSndWeaponInactive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogsndnoammowarning
 * @recoil-artifact defines .data recoil:data:0x779a94: g_OptCatalogSndNoAmmoWarning.
 * BN xrefs: OptCatalog::PlayNoAmmoWarning and
 * zWeapon::LoadOptCatalogFromPath.
 * Purpose: no-ammo warning sample loaded with the OptCatalog.
 */
zSndSample* g_OptCatalogSndNoAmmoWarning = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogdamagefeedbackcallback
 * @recoil-artifact defines .data recoil:data:0x779a9c: g_OptCatalogDamageFeedbackCallback.
 * Purpose: Stores g OptCatalogDamageFeedbackCallback data used by effects_weapons.optcatalog_damage_feedback_data.
 */
void* g_OptCatalogDamageFeedbackCallback = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalog-damagefeedbackhitcount
 * @recoil-artifact defines .data recoil:data:0x779aa0: g_OptCatalog_DamageFeedbackHitCount.
 * Purpose: Stores g OptCatalog DamageFeedbackHitCount data used by effects_weapons.optcatalog_damage_feedback_data.
 */
int g_OptCatalog_DamageFeedbackHitCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-optcatalogdamagefeedbacktrackednode
 * @recoil-artifact defines .data recoil:data:0x779aa4: g_OptCatalogDamageFeedbackTrackedNode.
 * Purpose: Stores g OptCatalogDamageFeedbackTrackedNode data used by effects_weapons.optcatalog_damage_feedback_data.
 */
CZNodePartial* g_OptCatalogDamageFeedbackTrackedNode = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-f-0x779aac
 * @recoil-artifact defines .data recoil:data:0x779aac: g_OptCatalogNextSpawnScale.
 * Purpose: one-shot spawn scale transferred into projectile or trail runtime
 * state, then reset to 1.0f.
 */
float g_OptCatalogNextSpawnScale = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-x6
 * @recoil-artifact defines .data recoil:data:0x4df804: g_zEffectAnim_TokenRange.
 * BN data shape: char[0x6] "RANGE"; xrefs from
 * zSndSystem::InitNamedSetsSyntax and zWeapon::LoadOptCatalogFromPath.
 * Purpose: names the RANGE parser field shared by sound sample ranges and
 * OptCatalog projectile ranges.
 */
char g_zEffectAnim_TokenRange[0x6] = "RANGE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-x0d
 * @recoil-artifact defines .data recoil:data:0x4df80c: g_zEffectAnim_TokenBounceSound.
 * BN data shape: char[0x0d] "BOUNCE_SOUND"; xref from
 * OptCatalog::LoadFxSpecFromReaderNode.
 * Purpose: names the optional bounce-sound sample list in OptCatalog effect
 * specs.
 */
char g_zEffectAnim_TokenBounceSound[0x0d] = "BOUNCE_SOUND";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-x0a
 * @recoil-artifact defines .data recoil:data:0x4dd218: g_Player_KillVerbToken.
 * BN data shape: char[0x0a] "KILL_VERB"; xref only from
 * zWeapon_OptCatalog::LoadKillVerbString at 0x43ca20.
 * Purpose: names the optional kill-verb parser field in OptCatalog entry
 * records.
 */
char g_Player_KillVerbToken[0x0a] = "KILL_VERB";
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
    const unsigned int kOptCatalogFlagSingleTrailSegment = 0x800;

    const unsigned int kOptCatalogFlagSkipTrailSegmentLighting = 0x10000;
    const unsigned int kOptCatalogFlagNoRenderableAttachment = 1u << 8;

    const unsigned int kOptCatalogFlagRelativeSpeed = 0x800000;

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

    const unsigned int kOptCatalogFastSqrtBias = 0x1fc00000;
    const int kOptCatalogRequiredVersion = 2;

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-zweapon-beamreflectnamefmt
     * @recoil-artifact defines .data recoil:data:0x4e4600: g_zWeapon_BeamReflectNameFmt.
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * BN data shape: char[15] "BeamReflect_%d"; xref only from
     * OptCatalog::CreateTrailRuntimeState at 0x4b1ec0.
     * Purpose: format the inactive BeamReflect segment node names created
     * for OptCatalog trail runtime state.
     * Retail keeps it in writable .data, where VC5 places only non-const
     * arrays, so it is not declared const.
     */
    char g_zWeapon_BeamReflectNameFmt[15] = "BeamReflect_%d";

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath and its local loader
     * helpers.
     * Purpose: return the element count stored in a zReader array node.
     */
    int zReaderArrayCount(zReader::Node * node)
    {
        return node->value.nodes[0].value.i32;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath and OptCatalog loader
     * helpers.
     * Purpose: return an integer element from a zReader array node.
     */
    int zReaderArrayInt(zReader::Node * node, int index)
    {
        return node->value.nodes[index].value.i32;
    }

    /**
     * Original inline helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath and OptCatalog loader
     * helpers.
     * Purpose: read an int-or-float zReader array element as a float.
     */
    float zReaderArrayFloat(zReader::Node * node, int index)
    {
        zReader::Node* const valueNode = &node->value.nodes[index];
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
    void SetFlagFromBool(unsigned int& flags, unsigned int flag, int value)
    {
        if (value != 0) {
            flags |= flag;
        } else {
            flags &= ~flag;
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath flag loader branches.
     * Purpose: load a boolean OptCatalog flag from a named zReader array.
     */
    void LoadNamedBoolFlag(zReader::Node * entryNode, const char* name, OptCatalogEntryDef* entry, unsigned int flag)
    {
        zReader::Node* const node = zRdrGetNode(entryNode, name);
        if (node != 0 && node->type == zReader::ZRDR_NODE_ARRAY && zReaderArrayCount(node) > 1) {
            SetFlagFromBool(entry->flags, flag, zReaderArrayInt(node, 1));
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath impact loader branches.
     * Purpose: parse crater radius base and randomized range metadata.
     */
    void LoadRadiusRange(zReader::Node * node, OptCatalogEntryDef * entry)
    {
        const int count = zReaderArrayCount(node);
        if (count > 1 && zReaderArrayInt(node, 1) != 0) {
            entry->flags |= kOptCatalogFlagCraterImpact;
            if (count > 2) {
                const int minRadius = (int)(zReaderArrayFloat(node, 1));
                entry->craterRadiusBase = minRadius;
                entry->craterRadiusRandomRange = (int)(zReaderArrayFloat(node, 2)) - minRadius;
            }
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath timed-status loader
     * branches.
     * Purpose: load timed-hit light range, delay, and color metadata.
     */
    void LoadTimedStatusBlock(zReader::Node * node, OptCatalogEntryDef * entry)
    {
        if (zReaderArrayCount(node) <= 6) {
            return;
        }

        entry->timedStatusLightRangeMin = zReaderArrayFloat(node, 1);
        entry->timedStatusLightRangeMax = zReaderArrayFloat(node, 2);
        entry->timedStatusUpdateDelay = zReaderArrayFloat(node, 3);
        entry->timedStatusLightSpecularColor.red = zReaderArrayFloat(node, 4);
        entry->timedStatusLightSpecularColor.green = zReaderArrayFloat(node, 5);
        entry->timedStatusLightSpecularColor.blue = zReaderArrayFloat(node, 6);
        entry->flags |= kOptCatalogFlagAppliesTimedHitStatus;
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath designate-status loader
     * branches.
     * Purpose: load remote-detonation designate status metadata.
     */
    void LoadDesignateStatusBlock(zReader::Node * node, OptCatalogEntryDef * entry)
    {
        if (zReaderArrayCount(node) <= 6) {
            return;
        }

        entry->timedStatusLightRangeMin = zReaderArrayFloat(node, 1);
        entry->timedStatusLightRangeMax = zReaderArrayFloat(node, 2);
        entry->timedStatusUpdateDelay = 0.0f;
        entry->timedStatusLightSpecularColor.red = zReaderArrayFloat(node, 3);
        entry->timedStatusLightSpecularColor.green = zReaderArrayFloat(node, 4);
        entry->timedStatusLightSpecularColor.blue = zReaderArrayFloat(node, 5);
        entry->detonationDistSq = zReaderArrayFloat(node, 6);
        entry->flags &= ~(kOptCatalogFlagAppliesTimedHitStatus | kOptCatalogFlagHeatTimedStatus);
        entry->flags |= kOptCatalogFlagRemoteDetonate;
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath damage-feedback loader
     * branches.
     * Purpose: load health-scaled damage-feedback effect variants.
     */
    void LoadDamageFeedbackOnHealth(zReader::Node * node, OptCatalogEntryDef * entry)
    {
        const int count = zReaderArrayCount(node) - 1;
        entry->damageFeedbackVariantCount = count > 4 ? 4 : count;
        for (int i = 0; i < entry->damageFeedbackVariantCount; ++i) {
            zReader::Node* const variantNode = &node->value.nodes[i + 1];
            if (variantNode->type == zReader::ZRDR_NODE_ARRAY && zReaderArrayCount(variantNode) > 2) {
                entry->damageFeedbackVariants[i].minFeedbackScale = zReaderArrayFloat(variantNode, 1);
                entry->damageFeedbackVariants[i].effect
                    = zEffectAnim::FindEntryByName(variantNode->value.nodes[2].value.str);
            }
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath impact-effect loader
     * branches.
     * Purpose: load per-material impact effect specs and fallback entries.
     */
    void LoadImpactFxTable(zReader::Node * impactNode, OptCatalogEntryDef * entry)
    {
        if (g_zRndr_GlobalStringCount <= 0) {
            return;
        }

        OptCatalog::LoadFxSpecFromReaderNode(impactNode, &entry->impactFxTable[0], g_zRndr_GlobalStringTable[0]);
        for (int i = 1; i < g_zRndr_GlobalStringCount; ++i) {
            if (zRdrGetNode(impactNode, g_zRndr_GlobalStringTable[i]) != 0) {
                OptCatalog::LoadFxSpecFromReaderNode(
                    impactNode,
                    &entry->impactFxTable[i],
                    g_zRndr_GlobalStringTable[i]
                );
            } else {
                entry->impactFxTable[i] = entry->impactFxTable[0];
            }
        }

        zReader::Node* const animationAlwaysNode = zRdrGetNode(impactNode, "ANIMATION_ALWAYS");
        if (animationAlwaysNode != 0) {
            entry->flags |= kOptCatalogFlagAlwaysPlayImpactFx;
        }
    }

    /**
     * Original static helper evidence: no standalone retail function.
     * Observed in zWeapon::LoadOptCatalogFromPath after catalog count load.
     * Purpose: allocate and initialize the OptCatalog runtime projectile pool.
     */
    void SetupRuntimeInstancePool()
    {
        if (g_OptCatalogRuntimeInstanceCount <= 0) {
            return;
        }

        OptCatalogRuntimeInstanceStorage* const slots = (OptCatalogRuntimeInstanceStorage*)(calloc(
            g_OptCatalogRuntimeInstanceCount,
            sizeof(OptCatalogRuntimeInstanceStorage)
        ));
        g_OptCatalogRuntimeInstancePool = slots;
        if (slots == 0) {
            return;
        }

        for (int i = 0; i < g_OptCatalogRuntimeInstanceCount; ++i) {
            OptCatalogRuntimeInstanceStorage* const runtime = &slots[i];
            runtime->projectileNode = CZObject3D::gwObject3DInit();
            if (runtime->projectileNode != 0) {
                char name[40];
                sprintf(name, "Projectile_%d", i);
                CZClass::gwNodeSetName(runtime->projectileNode, name);
                CZClass::gwNodeSetRaycastable(runtime->projectileNode, 0);
                CZClass::gwNodeSetCellPickable(runtime->projectileNode, 0);
                CZClass::gwNodeSetPickable(runtime->projectileNode, 1);
            }

            runtime->flyoutAnimPrimary = 0;
            runtime->flyoutAnimSecondary = 0;
            runtime->asyncFxHandle = 0;
            runtime->next = g_OptCatalogFreeRuntimeInstanceList;
            g_OptCatalogFreeRuntimeInstanceList = runtime;
        }
    }
} // namespace

#include "GameZRecoil/zUtil/zbd.h"

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-zweapon-zarhandlerregistered
 * @recoil-artifact defines .data recoil:data:0x4e42ec: g_zWeapon_ZarHandlerRegistered.
 * BN xrefs: zWepInit gates Weapons ZAR section callback registration.
 * Purpose: one-time startup flag controlling whether zWeapon registers the
 * Weapons archive callbacks during initialization.
 */
int g_zWeapon_ZarHandlerRegistered = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-g-zweapon-archivename
 * @recoil-artifact defines .data recoil:data:0x4e42f0: g_zWeapon_ArchiveName.
 * BN xrefs: zWepInit passes this string to zUtil_ZAR::RegisterSectionHandler.
 * Purpose: archive section name used when registering zWeapon save callbacks.
 */
char g_zWeapon_ArchiveName[8] = "Weapons";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-f-0x779a98
 * @recoil-artifact defines .data recoil:data:0x779a98: g_zWeapon_MaxTetherAltitude.
 * BN xrefs: zWepInit restores the startup default and tether checks consume
 * the configured altitude cap.
 * Purpose: runtime maximum tether altitude loaded from weapon configuration.
 */
float g_zWeapon_MaxTetherAltitude = 0.0f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-zwepinit
 * @recoil-artifact defines .text recoil:function:0x4b1090: zWepInit.
 * @recoil-match byte
 *
 * Purpose: reset weapon and OptCatalog runtime globals, restore weapon
 * defaults, and optionally register the Weapons ZAR section callbacks.
 */
extern "C" int __cdecl zWepInit()
{
    // Clearing EntryCount first keeps 0 in EAX and 1 in ECX, as in retail.
    g_OptCatalog_EntryCount = 0;
    g_OptCatalog_FallbackImpactProbeEnabled = 1;
    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    g_OptCatalog_EntryTable = 0;
    g_OptCatalogRuntimeInstanceCount = 0;
    g_OptCatalogRuntimeInstancePool = 0;
    g_OptCatalogFreeRuntimeInstanceList = 0;
    g_OptCatalogRuntimeWorld = 0;
    g_OptCatalogPendingSpawnTargetCountPtr = 0;
    g_OptCatalogPendingSpawnTargetListPtr = 0;
    g_OptCatalogMaxCraterRadius = 30.0f;
    g_OptCatalogQueuedImpactQueue.count = 0;
    g_OptCatalog_DamageContextKind = 0;
    g_OptCatalog_DamageContextHitEvent = 0;
    g_zWeapon_MaxTetherAltitude = 30.0f;
    g_OptCatalogDamageFeedbackCallback = 0;
    g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
    g_OptCatalog_DamageFeedbackHitCount = 0;
    g_OptCatalogDamageFeedbackTrackedNode = 0;
    g_OptCatalogNextSpawnScale = 1.0f;

    if (g_zWeapon_ZarHandlerRegistered != 0) {
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

namespace zWeapon
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-onweaponssectionpreload
     * @recoil-artifact defines .text recoil:function:0x4b1140: zWeapon::OnWeaponsSectionPreLoad
     * @recoil-match byte
     *
     * Purpose: write the current weapon damage-feedback hit count into the
     * WeaponData section blob before the Weapons archive section is saved.
     */
    int __fastcall OnWeaponsSectionPreLoad(zZbdSectionCallbackCtx * callbackCtx, void*)
    {
        int weaponDataHitCount = g_OptCatalog_DamageFeedbackHitCount;
        return zUtil_ZAR::WriteSectionBlob(callbackCtx, "WeaponData", &weaponDataHitCount, sizeof(weaponDataHitCount));
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-onweaponssectiondataready
     * @recoil-artifact defines .text recoil:function:0x4b1160: zWeapon::OnWeaponsSectionDataReady
     * @recoil-match byte
     *
     * Purpose: restore the weapon damage-feedback hit count from the WeaponData
     * section blob and reset the lock-on warning gate.
     */
    void __fastcall
    OnWeaponsSectionDataReady(zZbdSectionCallbackCtx*, const char*, void* weaponData, unsigned int, void*)
    {
        g_OptCatalog_DamageFeedbackHitCount = *(int*)(weaponData);
        g_OptCatalogLockOnWarningGateTimeSec = 0.0f;
    }
} // namespace zWeapon

namespace OptCatalog
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-shutdown
     * @recoil-artifact defines .text recoil:function:0x4b1180: OptCatalog::Shutdown
     * @recoil-match byte
     *
     * Purpose: public shutdown wrapper for OptCatalog runtime cleanup.
     */
    int __cdecl Shutdown()
    {
        ShutdownCore();
        return 0;
    }
} // namespace OptCatalog

namespace zWeapon
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-loadoptcatalogfrompath
     * @recoil-artifact defines .text recoil:function:0x4b1190: zWeapon::LoadOptCatalogFromPath
     * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zweapon.load-opt-catalog.fast-sqrt-estimate recoil:function:0x4b1190
     * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zweapon.load-opt-catalog.fast-sqrt-estimate
     *
     *
     * Raw assembly: one in-body fast-sqrt estimate at retail [0x4b1bc5,0x4b1bd2),
     * transforming the stored bits of velocityProduct into velocityEstimate through
     * EAX (Pro batch Y, run 0f1cfa97). The block clobbers EAX and the arithmetic
     * flags; it does not use or alter the x87 stack or control word.
     *
     * Purpose: load weapons.zrd, build the OptCatalog entry table, initialize
     * runtime storage, and publish the loaded runtime globals.
     */
    int __fastcall LoadOptCatalogFromPath(
        CZNodePartial * worldNode,
        const char* path,
        int networkState,
        zWeaponOptCatalogEntryCallback entryCallback
    )
    {
        // Retail keeps the last parsed RANGE in one function-scope float: an entry
        // without a RANGE node reuses a previously parsed value in the instance
        // count below. It is not entry->range; before any RANGE has been parsed it
        // is uninitialized, as in retail.
        float range;
        short entryIndex = 0;

        g_OptCatalogRuntimeWorld = worldNode;
        CZLight::InitThermalGlowPool();

        zReader::Node* const rootNode = zReader::Load(path, 0, 0);
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

        zReader::Node* const versionNode = zRdrGetNode(rootNode, "VERSION");
        if (versionNode != 0) {
            const int version = versionNode->value.nodes[1].value.i32;
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

            zReader::Node* node = zRdrGetNode(rootNode, "LOCK_ON_WARNING");
            if (node != 0) {
                g_OptCatalogSndLockOnWarning = zSnd::FindSampleByName(node->value.nodes[1].value.str);
            }

            node = zRdrGetNode(rootNode, "NO_AMMO_WARNING");
            if (node != 0) {
                g_OptCatalogSndTriggerInactive = zSnd::FindSampleByName(node->value.nodes[1].value.str);
            }

            node = zRdrGetNode(rootNode, "TRIGGER_INACTIVE");
            if (node != 0) {
                g_OptCatalogSndWeaponInactive = zSnd::FindSampleByName(node->value.nodes[1].value.str);
            }

            node = zRdrGetNode(rootNode, "WEAPON_INACTIVE");
            if (node != 0) {
                g_OptCatalogSndNoAmmoWarning = zSnd::FindSampleByName(node->value.nodes[1].value.str);
            }

            node = zRdrGetNode(rootNode, "MAX_CRATER_RADIUS");
            if (node != 0) {
                g_OptCatalogMaxCraterRadius = node->value.nodes[1].value.f32;
            }

            zReader::Node* const ballisticsNode = zRdrGetNode(rootNode, "BALLISTICS");
            if (ballisticsNode != 0) {
                g_OptCatalog_EntryCount = (ballisticsNode->value.nodes[0].value.i32 - 1) / 2;
                g_OptCatalog_EntryTable
                    = (OptCatalogEntryDef*)(calloc(1, g_OptCatalog_EntryCount * sizeof(OptCatalogEntryDef)));

                OptCatalogEntryDef* entry = g_OptCatalog_EntryTable;
                for (unsigned int itemIndex = 1; itemIndex < (unsigned int)(ballisticsNode->value.nodes[0].value.i32);
                    itemIndex += 2, ++entry) {
                    entry->ammoOrChargeMax = 50.0f;
                    entry->range = 500.0f;
                    entry->velocity = 120.0f;
                    entry->damage = 0.100000001f;
                    entry->timedStatusInterpRate = 1.0f;
                    entry->ordinalIndex = entryIndex++;
                    entry->impactFxTable
                        = (OptCatalogFxSpec*)(calloc(1, g_zRndr_GlobalStringCount * sizeof(OptCatalogFxSpec)));

                    // Each weapon block follows its name inside the BALLISTICS array.
                    zReader::Node* const entryNode
                        = zRdrGetNode(ballisticsNode, ballisticsNode->value.nodes[itemIndex].value.str);
                    if (entryNode != 0) {
                        entry->keyName = (char*)(ballisticsNode->value.nodes[itemIndex].value.str);

                        zReader::Node* fieldNode = zRdrGetNode(entryNode, "NAME");
                        if (fieldNode != 0) {
                            entry->displayName = (char*)(fieldNode->value.nodes[1].value.str);
                        } else {
                            entry->displayName = entry->keyName;
                        }

                        fieldNode = zRdrGetNode(entryNode, "DESC");
                        entry->description = _strdup(
                            fieldNode != 0 ? zLoc::ResolveMessageKeyOrFallback(fieldNode->value.nodes[1].value.str)
                                           : entry->keyName
                        );

                        fieldNode = zRdrGetNode(entryNode, "MILITARY_NAME");
                        entry->militaryName = _strdup(
                            fieldNode != 0 ? zLoc::ResolveMessageKeyOrFallback(fieldNode->value.nodes[1].value.str)
                                           : entry->keyName
                        );

                        fieldNode = zRdrGetNode(entryNode, "ACCELERATION");
                        if (fieldNode != 0) {
                            entry->acceleration = fieldNode->value.nodes[1].value.f32;
                        }

                        fieldNode = zRdrGetNode(entryNode, "AMMO_LIMIT");
                        if (fieldNode != 0) {
                            entry->ammoOrChargeMax = (float)(fieldNode->value.nodes[1].value.i32);
                        }

                        fieldNode = zRdrGetNode(entryNode, "BEAM");
                        if (fieldNode != 0) {
                            entry->flags |= kOptCatalogFlagTrailRuntime;
                            entry->velocity = 1.0f / fieldNode->value.nodes[1].value.f32;
                            entry->timedStatusInterpRate = fieldNode->value.nodes[2].value.f32;
                            entry->flags ^= (fieldNode->value.nodes[3].value.i32 ^ entry->flags) & 1u;
                        }

                        fieldNode = zRdrGetNode(entryNode, "CATCHES_FIRE");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagImmediateProbeImpact)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 12);
                        }

                        fieldNode = zRdrGetNode(entryNode, "CRATER");
                        if (fieldNode != 0) {
                            if (fieldNode->value.nodes[1].value.i32 != 0) {
                                entry->flags |= kOptCatalogFlagCraterImpact;
                            }
                            if (fieldNode->value.nodes[0].value.i32 > 2) {
                                entry->craterRadiusBase = fieldNode->value.nodes[1].value.i32;
                                entry->craterRadiusRandomRange
                                    = fieldNode->value.nodes[2].value.i32 - fieldNode->value.nodes[1].value.i32;
                            } else {
                                entry->craterRadiusRandomRange = 0;
                            }
                        }

                        fieldNode = zRdrGetNode(entryNode, "DAMAGE");
                        if (fieldNode != 0) {
                            entry->damage = fieldNode->value.nodes[1].value.f32;
                        }

                        float floatValue;
                        fieldNode = zRdrGetNode(entryNode, "DETONATION_DISTANCE");
                        if (fieldNode != 0) {
                            floatValue = fieldNode->value.nodes[1].value.f32;
                            entry->detonationDistSq = floatValue * floatValue;
                        }

                        fieldNode = zRdrGetNode(entryNode, "EXPIRES");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagExpires)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 6);
                        }

                        fieldNode = zRdrGetNode(entryNode, "FIRE_RATE");
                        if (fieldNode != 0) {
                            entry->fireRateInterval = 1.0f / fieldNode->value.nodes[1].value.f32;
                        }

                        fieldNode = zRdrGetNode(entryNode, "FIXED_ROTATE");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagFixedRotate)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 7);
                        }

                        fieldNode = zRdrGetNode(entryNode, "GRAVITY");
                        if (fieldNode != 0 && (entry->flags & kOptCatalogFlagLockOn) == 0) {
                            entry->gravity = fieldNode->value.nodes[1].value.f32;
                        }

                        fieldNode = zRdrGetNode(entryNode, "IMPACT_PROXIMITY");
                        if (fieldNode != 0) {
                            entry->impactProximity = fieldNode->value.nodes[1].value.f32;
                            floatValue = fieldNode->value.nodes[1].value.f32;
                            entry->damageFalloffRange = floatValue * floatValue;
                        }

                        fieldNode = zRdrGetNode(entryNode, "IMPACT_TYPE");
                        if (fieldNode != 0) {
                            entry->damageMaskSlotIndex = fieldNode->value.nodes[1].value.i32;
                        }

                        fieldNode = zRdrGetNode(entryNode, "INSTANT");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagInstant)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 10);
                        }

                        fieldNode = zRdrGetNode(entryNode, "LOCK_ON");
                        if (fieldNode != 0) {
                            entry->lockOnTime = fieldNode->value.nodes[1].value.f32;
                            entry->flags |= kOptCatalogFlagLockOn;
                        }

                        fieldNode = zRdrGetNode(entryNode, "LOCK_ON_LEAD");
                        if (fieldNode != 0) {
                            entry->flags |= kOptCatalogFlagLockOnLead;
                        }

                        fieldNode = zRdrGetNode(entryNode, "MINE");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagFullProbeDamage)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 13) | 1u;
                        }

                        fieldNode = zRdrGetNode(entryNode, "MULTI_TARGET");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagMultiTarget)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 16);
                        }

                        fieldNode = zRdrGetNode(entryNode, "QUICKSAND");
                        if (fieldNode != 0) {
                            if (fieldNode->value.nodes[1].value.i32 != 0) {
                                entry->flags |= kOptCatalogFlagQuickSandImpact;
                            }
                            if (fieldNode->value.nodes[0].value.i32 > 2) {
                                entry->craterRadiusBase = fieldNode->value.nodes[1].value.i32;
                                entry->craterRadiusRandomRange
                                    = fieldNode->value.nodes[2].value.i32 - fieldNode->value.nodes[1].value.i32;
                            } else {
                                entry->craterRadiusRandomRange = 0;
                            }
                        }

                        fieldNode = zRdrGetNode(entryNode, g_zEffectAnim_TokenRange);
                        if (fieldNode != 0) {
                            range = fieldNode->value.nodes[1].value.f32;
                            entry->range = range;
                            entry->rangeSq = range * range;
                        }

                        fieldNode = zRdrGetNode(entryNode, "RELATIVE_SPEED");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagRelativeSpeed)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 23);
                        }

                        fieldNode = zRdrGetNode(entryNode, "REMOTE_DETONATE");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagRemoteDetonate)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 19);
                        }

                        fieldNode = zRdrGetNode(entryNode, "TETHER_GUIDED");
                        if (fieldNode != 0) {
                            entry->flags |= kOptCatalogFlagTetherGuided;
                        }

                        fieldNode = zRdrGetNode(entryNode, "TURN_RATE");
                        if (fieldNode != 0) {
                            entry->turnRate = fieldNode->value.nodes[1].value.f32;
                        } else {
                            entry->turnRate = 0.159999996f;
                        }

                        fieldNode = zRdrGetNode(entryNode, "TURN_SUSPEND_TIME");
                        if (fieldNode != 0) {
                            entry->turnSuspendTime = fieldNode->value.nodes[1].value.f32;
                        }

                        fieldNode = zRdrGetNode(entryNode, "PITCH_RATE");
                        if (fieldNode != 0) {
                            entry->pitchRate = fieldNode->value.nodes[1].value.f32;
                        } else {
                            entry->pitchRate = 0.159999996f;
                        }

                        fieldNode = zRdrGetNode(entryNode, "VELOCITY");
                        if (fieldNode != 0 && (entry->flags & kOptCatalogFlagTrailRuntime) == 0) {
                            entry->velocity = fieldNode->value.nodes[1].value.f32;
                        }

                        fieldNode = zRdrGetNode(entryNode, "RELOAD");
                        if (fieldNode != 0) {
                            entry->flags = (entry->flags & ~kOptCatalogFlagReload)
                                | ((fieldNode->value.nodes[1].value.i32 & 1) << 18);
                        }
                        entry->killVerbString = 0;

                        if (entryCallback != 0) {
                            entryCallback(entryNode, entry);
                        }

                        OptCatalog::LoadFxSpecFromReaderNode(entryNode, &entry->fireFxSpec, "FIRE");
                        OptCatalog::LoadFxSpecFromReaderNode(entryNode, &entry->flyoutFxSpec, "FLYOUT");

                        fieldNode = zRdrGetNode(entryNode, "FLYOUT_HEALTH");
                        if (fieldNode != 0) {
                            entry->flags |= kOptCatalogFlagImpactWhenScaleExpired;
                            entry->flyoutHealth = (float)(fieldNode->value.nodes[1].value.i32);
                            if (entry->attachCloneTemplateNode != 0) {
                                CZClass::gwNodeSetRaycastable(entry->attachCloneTemplateNode, 0);
                            }
                        }

                        zReader::Node* const impactNode = zRdrGetNode(entryNode, "IMPACT");
                        if (impactNode != 0) {
                            OptCatalog::LoadFxSpecFromReaderNode(
                                impactNode,
                                &entry->impactFxTable[0],
                                g_zRndr_GlobalStringTable[0]
                            );
                            for (int materialIndex = 1; materialIndex < g_zRndr_GlobalStringCount; ++materialIndex) {
                                if (zRdrGetNode(impactNode, g_zRndr_GlobalStringTable[materialIndex]) != 0) {
                                    OptCatalog::LoadFxSpecFromReaderNode(
                                        impactNode,
                                        &entry->impactFxTable[materialIndex],
                                        g_zRndr_GlobalStringTable[materialIndex]
                                    );
                                } else {
                                    entry->impactFxTable[materialIndex] = entry->impactFxTable[0];
                                }
                            }

                            if (zRdrGetNode(impactNode, "ANIMATION_ALWAYS") != 0) {
                                entry->flags |= kOptCatalogFlagAlwaysPlayImpactFx;
                            }

                            fieldNode = zRdrGetNode(impactNode, "FREEZE");
                            if (fieldNode != 0 && fieldNode->value.nodes[0].value.i32 > 6) {
                                entry->timedStatusLightRangeMin = fieldNode->value.nodes[1].value.f32;
                                entry->timedStatusLightRangeMax = fieldNode->value.nodes[2].value.f32;
                                entry->timedStatusUpdateDelay = fieldNode->value.nodes[3].value.f32;
                                entry->timedStatusLightSpecularColor.red = fieldNode->value.nodes[4].value.f32;
                                entry->timedStatusLightSpecularColor.green = fieldNode->value.nodes[5].value.f32;
                                entry->timedStatusLightSpecularColor.blue = fieldNode->value.nodes[6].value.f32;
                                entry->flags
                                    |= kOptCatalogFlagTimedStatusSubtractive | kOptCatalogFlagAppliesTimedHitStatus;
                            }

                            fieldNode = zRdrGetNode(impactNode, "HEAT");
                            if (fieldNode != 0 && fieldNode->value.nodes[0].value.i32 > 6) {
                                entry->timedStatusLightRangeMin = fieldNode->value.nodes[1].value.f32;
                                entry->timedStatusLightRangeMax = fieldNode->value.nodes[2].value.f32;
                                entry->timedStatusUpdateDelay = fieldNode->value.nodes[3].value.f32;
                                entry->timedStatusLightSpecularColor.red = fieldNode->value.nodes[4].value.f32;
                                entry->timedStatusLightSpecularColor.green = fieldNode->value.nodes[5].value.f32;
                                entry->timedStatusLightSpecularColor.blue = fieldNode->value.nodes[6].value.f32;
                                entry->flags |= kOptCatalogFlagAppliesTimedHitStatus | kOptCatalogFlagHeatTimedStatus;
                            }

                            fieldNode = zRdrGetNode(impactNode, "DESIGNATE");
                            if (fieldNode != 0 && fieldNode->value.nodes[0].value.i32 > 6) {
                                entry->timedStatusLightRangeMin = fieldNode->value.nodes[1].value.f32;
                                entry->timedStatusLightRangeMax = fieldNode->value.nodes[2].value.f32;
                                entry->timedStatusUpdateDelay = 0.0f;
                                entry->timedStatusLightSpecularColor.red = fieldNode->value.nodes[3].value.f32;
                                entry->timedStatusLightSpecularColor.green = fieldNode->value.nodes[4].value.f32;
                                entry->timedStatusLightSpecularColor.blue = fieldNode->value.nodes[5].value.f32;
                                entry->flags
                                    &= ~(kOptCatalogFlagAppliesTimedHitStatus | kOptCatalogFlagHeatTimedStatus);
                                entry->flags |= kOptCatalogFlagSingleTrailSegment;
                                entry->detonationDistSq = fieldNode->value.nodes[6].value.f32;
                            }

                            fieldNode = zRdrGetNode(impactNode, "KILL_ANIMATION");
                            if (fieldNode != 0) {
                                entry->damageContextEffect
                                    = zEffectAnim::FindEntryByName(fieldNode->value.nodes[1].value.str);
                            }

                            fieldNode = zRdrGetNode(impactNode, "DAMAGE_ANIMATION");
                            if (fieldNode != 0) {
                                entry->damageFeedbackVariantCount = 1;
                                entry->damageFeedbackVariants[0].minFeedbackScale = 1.0f;
                                entry->damageFeedbackVariants[0].effect
                                    = zEffectAnim::FindEntryByName(fieldNode->value.nodes[1].value.str);
                            }

                            if (g_zVideo_ActiveRendererPath != 0) {
                                fieldNode = zRdrGetNode(impactNode, "DAMAGE_ANIM_ON_HEALTH");
                                if (fieldNode != 0) {
                                    entry->damageFeedbackVariantCount = fieldNode->value.nodes[0].value.i32 - 1;
                                    for (unsigned int feedbackIndex = 0;
                                        feedbackIndex < (unsigned int)(entry->damageFeedbackVariantCount);
                                        ++feedbackIndex) {
                                        zReader::Node* const feedbackNode
                                            = fieldNode->value.nodes[feedbackIndex + 1].value.nodes;
                                        entry->damageFeedbackVariants[feedbackIndex].minFeedbackScale
                                            = feedbackNode[1].value.f32;
                                        entry->damageFeedbackVariants[feedbackIndex].effect
                                            = zEffectAnim::FindEntryByName(feedbackNode[2].value.str);
                                    }
                                }
                            }
                        }
                    }

                    if (entry->gravity != 0.0f) {
                        const float speed = entry->velocity;
                        entry->trailSegmentTimeSec = speed * speed / (entry->gravity * 2.0f);
                        float velocityProduct = entry->gravity * entry->range * 2.0f;
                        float velocityEstimate;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
                        __asm {
                            mov eax, velocityProduct
                            sar eax, 1
                            add eax, 01fc00000h
                            mov velocityEstimate, eax
                        }
#else
                        {
                            int estimateBits;
                            memcpy(&estimateBits, &velocityProduct, sizeof estimateBits);
                            estimateBits = (estimateBits >> 1) + 0x1fc00000;
                            memcpy(&velocityEstimate, &estimateBits, sizeof velocityEstimate);
                        }
#endif
                        entry->velocity = velocityEstimate;
                    }

                    CZNodePartial* const attachNode = entry->attachCloneTemplateNode;
                    if (attachNode != 0) {
                        CZClass::gwNodeSetActive(attachNode, 1);
                        // Retail stores the "no renderable DI" result in bit 8.
                        const int hasRenderable
                            = CZClass::AnyNodeMatchesPredicateRecursive(attachNode, CZNode::HasRenderableDiPredicate);
                        entry->flags = (entry->flags & ~kOptCatalogFlagNoRenderableAttachment)
                            | (((hasRenderable == 0) & 1) << 8);
                    }

                    if ((entry->flags & kOptCatalogFlagTrailRuntime) == 0) {
                        g_OptCatalogRuntimeInstanceCount
                            += (int)(range / entry->velocity / entry->fireRateInterval) + 1;
                    }
                }
            }

            OptCatalogRuntimeInstanceStorage* runtime = (OptCatalogRuntimeInstanceStorage*)(calloc(
                g_OptCatalogRuntimeInstanceCount,
                sizeof(OptCatalogRuntimeInstanceStorage)
            ));
            g_OptCatalogRuntimeInstancePool = runtime;
            g_OptCatalogFreeRuntimeInstanceList = 0;
            for (unsigned int runtimeIndex = 0; runtimeIndex < (unsigned int)(g_OptCatalogRuntimeInstanceCount);
                ++runtimeIndex, ++runtime) {
                runtime->projectileNode = CZObject3D::gwObject3DInit();

                char projectileName[32];
                sprintf(projectileName, "Projectile_%d", runtimeIndex);
                CZClass::gwNodeSetName(runtime->projectileNode, projectileName);

                runtime->flyoutAnimPrimary = 0;
                runtime->flyoutAnimSecondary = 0;
                runtime->asyncFxHandle = 0;
                runtime->next = g_OptCatalogFreeRuntimeInstanceList;
                g_OptCatalogFreeRuntimeInstanceList = runtime;

                CZClass::gwNodeSetRaycastable(runtime->projectileNode, 0);
                CZClass::gwNodeSetCellPickable(runtime->projectileNode, 0);
                CZClass::gwNodeSetPickable(runtime->projectileNode, 1);
            }

            CZNodePartial* const callbackNode = CZObject3D::gwObject3DInit();
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

            CZClass::gwNodeSetPriority(callbackNode, 3);
            CZClass::gwNodeSetActionCallback(callbackNode, (void*)(&OptCatalog::ProcessRuntimeInstances));
            g_OptCatalogNetworkOptionState = networkState;
            return 0;
        }

        zError::ReportOld(0x400, "D:\\Proj\\GameZRecoil\\zWeapon\\zwep_init.c", 0xdb, "No ZWEP version found");
        return -1;
    }
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-setmaxtetheraltitude
     * @recoil-artifact defines .text recoil:function:0x4b1d80: zWeapon::SetMaxTetherAltitude
     * @recoil-match byte
     *
     * Purpose: store the maximum tether altitude used by weapon script commands.
     */
    void __stdcall SetMaxTetherAltitude(float altitude)
    {
        g_zWeapon_MaxTetherAltitude = altitude;
    }
} // namespace zWeapon

namespace OptCatalog
{
    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-shutdowncore
     * @recoil-artifact defines .text recoil:function:0x4b1d90: OptCatalog::ShutdownCore.
     * @recoil-match byte
     *
     * Purpose: release loaded OptCatalog entries, runtime pools, reader tree,
     * and reset runtime globals to initialization defaults.
     */
    int __fastcall ShutdownCore()
    {
        OptCatalogEntryDef* entryPtr = g_OptCatalog_EntryTable;
        for (int i = 0; i < g_OptCatalog_EntryCount; ++i, ++entryPtr) {
            OptCatalogEntryDef& entry = *entryPtr;
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

            CZNodePartial* impactNode = entry.impactNodeListHead;
            while (impactNode != 0) {
                CZNodePartial* const next = impactNode->callbackContext;
                entry.impactNodeListHead = next;
                CZUtil::DestroyNodeRecursive(impactNode);
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
        CZLight::DestroyThermalGlowPool();
        g_OptCatalogRuntimeWorld = 0;
        zReader::Free(g_OptCatalogLoadedTreeRoot);
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
        g_OptCatalogQueuedImpactQueue.count = 0;
        g_OptCatalog_DamageFeedbackHitCount = 0;
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-createtrailruntimestate
     * @recoil-artifact defines .text recoil:function:0x4b1ec0: OptCatalog::CreateTrailRuntimeState
     * @recoil-match byte
     *
     * Purpose: allocate trail runtime state, create inactive BeamReflect
     * segment nodes, and attach them to the OptCatalog runtime world.
     */
    OptCatalogTrailRuntimeState* __fastcall CreateTrailRuntimeState(
        OptCatalogEntryDef * entry,
        CZNodePartial * projectileNode,
        zTag4Partial * variantTagPtr,
        void* reserved,
        zVec3* spawnPos,
        zVec3* spawnDir,
        int segmentCount
    )
    {
        (void)reserved;

        OptCatalogTrailRuntimeState* const runtime
            = (OptCatalogTrailRuntimeState*)(calloc(1, sizeof(OptCatalogTrailRuntimeState)));
        runtime->ownerEntry = entry;
        runtime->projectileNode = projectileNode;
        runtime->spawnPos = spawnPos;
        runtime->variantTagPtr = variantTagPtr;
        runtime->spawnDir = spawnDir;

        if (segmentCount > 8) {
            segmentCount = 8;
        } else if ((entry->flags & kOptCatalogFlagSingleTrailSegment) != 0) {
            segmentCount = 1;
        }

        runtime->activeNodeSlotCount = segmentCount;
        for (int i = 0; i < segmentCount; ++i) {
            runtime->activeNodeSlots[i].node = CreateTrailSegmentNodeFromTemplate(entry->attachCloneTemplateNode);

            char nodeName[40];
            sprintf(nodeName, g_zWeapon_BeamReflectNameFmt, i);
            CZClass::gwNodeSetName(runtime->activeNodeSlots[i].node, nodeName);
            CZClass::gwNodeSetActive(runtime->activeNodeSlots[i].node, 0);
            if ((entry->flags & kOptCatalogFlagSkipTrailSegmentLighting) == 0) {
                CZObject3D::gwObject3DSetLitFlag(runtime->activeNodeSlots[i].node, 1);
            }
            CZClass::AddChild(g_OptCatalogRuntimeWorld, runtime->activeNodeSlots[i].node);
        }

        return runtime;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-freetrailruntimestatestorage
     * @recoil-artifact defines .text recoil:function:0x4b1f90: OptCatalog::FreeTrailRuntimeStateStorage
     * @recoil-match byte
     *
     * BN source path: D:\Proj\GameZRecoil\zWeapon\zWeapon.cpp.
     * Purpose: release trail runtime-state storage and return zero status.
     */
    int __fastcall FreeTrailRuntimeStateStorage(void* trailRuntimeState)
    {
        free(trailRuntimeState);
        return 0;
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-loadfxspecfromreadernode
     * @recoil-artifact defines .text recoil:function:0x4b1fa0: OptCatalog::LoadFxSpecFromReaderNode
     * @recoil-match byte
     *
     * Purpose: load one named impact effect spec from a zReader node.
     */
    void __fastcall LoadFxSpecFromReaderNode(zReader::Node * parentNode, OptCatalogFxSpec * spec, const char* childName)
    {
        zReader::Node* const specNode = zRdrGetNode(parentNode, childName);
        if (specNode == 0) {
            return;
        }

        zReader::Node* fieldNode = zRdrGetNode(specNode, "EFFECT");
        if (fieldNode != 0) {
            if (fieldNode->value.nodes[0].value.i32 > 1) {
                spec->effectTemplateIndex = zEffect::FindTemplateIndexByName(fieldNode->value.nodes[1].value.str);
            }
        } else {
            fieldNode = zRdrGetNode(specNode, "MODEL");
            if (fieldNode != 0 && fieldNode->value.nodes[0].value.i32 > 1) {
                spec->modelNode = CZClass::FindByTypeAndName(6, fieldNode->value.nodes[1].value.str);
            }
        }

        fieldNode = zRdrGetNode(specNode, "ANIMATION_ATTACHED");
        if (fieldNode != 0 && fieldNode->value.nodes[0].value.i32 > 1) {
            spec->attachedAnimationEntry = zEffectAnim::FindEntryByName(fieldNode->value.nodes[1].value.str);
        }

        fieldNode = zRdrGetNode(specNode, "MODEL_ANIMATION");
        if (fieldNode != 0 && fieldNode->value.nodes[0].value.i32 > 1) {
            spec->modelAnimationEntry = zEffectAnim::FindEntryByName(fieldNode->value.nodes[1].value.str);
        }

        fieldNode = zRdrGetNode(specNode, "ANIMATION");
        if (fieldNode != 0 && fieldNode->value.nodes[0].value.i32 > 1) {
            spec->animationEntry = zEffectAnim::FindEntryByName(fieldNode->value.nodes[1].value.str);
        }

        fieldNode = zRdrGetNode(specNode, "RANDOM_ROTATE");
        if (fieldNode != 0) {
            spec->flags = (((unsigned int)(fieldNode->value.nodes[1].value.i32) ^ spec->flags) & 1) ^ spec->flags;
        }

        fieldNode = zRdrGetNode(specNode, g_HudZrd_Key_Sound);
        if (fieldNode != 0) {
            spec->soundCount = fieldNode->value.nodes[0].value.i32 - 1;
            for (int i = 1; i < fieldNode->value.nodes[0].value.i32; ++i) {
                spec->soundSamples[i - 1] = zSnd::FindSampleByName(fieldNode->value.nodes[i].value.str);
            }
        }

        fieldNode = zRdrGetNode(specNode, g_zEffectAnim_TokenBounceSound);
        if (fieldNode != 0) {
            spec->bounceSoundCount = fieldNode->value.nodes[0].value.i32 - 1;
            for (int i = 1; i < fieldNode->value.nodes[0].value.i32; ++i) {
                spec->bounceSoundSamples[i - 1] = zSnd::FindSampleByName(fieldNode->value.nodes[i].value.str);
            }
        }
    }

    /**
     * @recoil-anchor recoil:anchor:gamezrecoil-zweapon-zwep-init-createtrailsegmentnodefromtemplate
     * @recoil-artifact defines .text recoil:function:0x4b2130: OptCatalog::CreateTrailSegmentNodeFromTemplate
     * @recoil-match byte
     *
     * Purpose: allocate an active Object3D segment node and attach an optional
     * template child to it.
     */
    CZNodePartial* __fastcall CreateTrailSegmentNodeFromTemplate(CZNodePartial * templateNode)
    {
        CZNodePartial* const parent = CZObject3D::gwObject3DInit();
        CZClass::gwNodeSetActive(parent, 1);
        if (templateNode != 0) {
            CZObject3D::gwObject3DAddChild(parent, templateNode);
        }

        return parent;
    }
} // namespace OptCatalog
