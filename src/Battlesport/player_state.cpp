// Battlesport compilation unit between player_remap.cpp and player_terrain.cpp,
// inferred from the retail object boundary [0x42a9f0, 0x42bf90): its .rdata
// constants [0x4d0848, 0x4d0870) repeat 0.0f, 1.0f, -1.0f and -5.0f that the
// neighbouring objects pool separately, and its .data strings
// [0x4dc9a8, 0x4dcaac) follow player_move.cpp's. Original filename unresolved;
// player_state.cpp is a provisional name (2026-10-02).

#include "recoil/Mfc42Abi.h"
#include "player.h"

#include "Battlesport/ai_net.h"
#include "Battlesport/game_net.h"
#include "Battlesport/pickup.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "hud.h"
#include "hud_sensor_tracker.h"
#include "opt_catalog.h"

#include <algorithm>
#include <atlbase.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern char g_HudUiCounterText_PlayerLabel[];

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-posyaw
 * @recoil-artifact defines .data recoil:data:0x4dc9a8: g_Player_HudReadoutFmt_PosYaw.
 * BN types this as a writable .data char[0x14] read by
 * Player::UpdateDebugOverlayHud for the position/yaw debug overlay line.
 * Purpose: Formats the debug HUD position and yaw readout.
 */
char g_Player_HudReadoutFmt_PosYaw[0x14] = "POS %d %d %d YAW %d";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_PosYaw) == 0x14);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-dynamics
 * @recoil-artifact defines .data recoil:data:0x4dc9bc: g_Player_HudReadoutFmt_Dynamics.
 * BN types this as a writable .data char[0x15] read by
 * Player::UpdateDebugOverlayHud for the normal dynamics debug overlay line.
 * Purpose: Formats the debug HUD player dynamics readout.
 */
char g_Player_HudReadoutFmt_Dynamics[0x15] = "%s using %s dynamics";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_Dynamics) == 0x15);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-dynamicss
 * @recoil-artifact defines .data recoil:data:0x4dc9d4: g_Player_HudReadoutFmt_DynamicsS.
 * BN types this as a writable .data char[0x19] read by
 * Player::UpdateDebugOverlayHud for the slipping dynamics debug overlay line.
 * Purpose: Formats the debug HUD player dynamics readout while slipping.
 */
char g_Player_HudReadoutFmt_DynamicsS[0x19] = "%s using %s dynamics - S";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_DynamicsS) == 0x19);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-dynamicsa
 * @recoil-artifact defines .data recoil:data:0x4dc9f0: g_Player_HudReadoutFmt_DynamicsA.
 * BN types this as a writable .data char[0x19] read by
 * Player::UpdateDebugOverlayHud for the airborne dynamics debug overlay line.
 * Purpose: Formats the debug HUD player dynamics readout while airborne.
 */
char g_Player_HudReadoutFmt_DynamicsA[0x19] = "%s using %s dynamics - A";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_DynamicsA) == 0x19);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-dead
 * @recoil-artifact defines .data recoil:data:0x4dca0c: g_Player_HudReadoutFmt_Dead.
 * BN types this as a writable .data char[0x0c] read by
 * Player::UpdateDebugOverlayHud for the inactive-player debug overlay line.
 * Purpose: Formats the debug HUD inactive player readout.
 */
char g_Player_HudReadoutFmt_Dead[0x0c] = "%s is DEAD!";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_Dead) == 0x0c);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-hudreadoutfmt-modegoalnode
 * @recoil-artifact defines .data recoil:data:0x4dca18: g_Player_HudReadoutFmt_ModeGoalNode.
 * BN types this as a writable .data char[0x26] read by
 * Player::UpdateDebugOverlayHud for the AI mode/goal-node debug overlay line.
 * Purpose: Formats the debug HUD AI mode and goal-node readout.
 */
char g_Player_HudReadoutFmt_ModeGoalNode[0x26] = "%s is in mode %d and had goal node %d";
RECOIL_STATIC_ASSERT(sizeof(g_Player_HudReadoutFmt_ModeGoalNode) == 0x26);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-unknown
 * @recoil-artifact defines .data recoil:data:0x4dca40: g_Player_MasterTypeName_Unknown.
 * BN types this as a writable .data char[0x08] shared by
 * Player::UpdateDebugOverlayHud, zNetworkDPlayReportError, and zSnd error
 * reporters for fallback "UNKNOWN" diagnostics.
 * Purpose: Names the shared fallback diagnostic/master-type token.
 */
char g_Player_MasterTypeName_Unknown[0x08] = "UNKNOWN";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Unknown) == 0x08);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-fly
 * @recoil-artifact defines .data recoil:data:0x4dca48: g_Player_MasterTypeName_Fly.
 * BN types this as a writable .data char[0x04] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the fly master type in the debug HUD.
 */
char g_Player_MasterTypeName_Fly[0x04] = "FLY";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Fly) == 0x04);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-sub
 * @recoil-artifact defines .data recoil:data:0x4dca4c: g_Player_MasterTypeName_Sub.
 * BN types this as a writable .data char[0x04] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the sub master type in the debug HUD.
 */
char g_Player_MasterTypeName_Sub[0x04] = "SUB";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Sub) == 0x04);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-amphib
 * @recoil-artifact defines .data recoil:data:0x4dca50: g_Player_MasterTypeName_Amphib.
 * BN types this as a writable .data char[0x07] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the amphib master type in the debug HUD.
 */
char g_Player_MasterTypeName_Amphib[0x07] = "AMPHIB";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Amphib) == 0x07);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-hover
 * @recoil-artifact defines .data recoil:data:0x4dca58: g_Player_MasterTypeName_Hover.
 * BN types this as a writable .data char[0x06] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the hover master type in the debug HUD.
 */
char g_Player_MasterTypeName_Hover[0x06] = "HOVER";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Hover) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-track
 * @recoil-artifact defines .data recoil:data:0x4dca60: g_Player_MasterTypeName_Track.
 * BN types this as a writable .data char[0x06] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the track master type in the debug HUD.
 */
char g_Player_MasterTypeName_Track[0x06] = "TRACK";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Track) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-mastertypename-basic
 * @recoil-artifact defines .data recoil:data:0x4dca68: g_Player_MasterTypeName_Basic.
 * BN types this as a writable .data char[0x06] read by
 * Player::UpdateDebugOverlayHud's inlined master-type-name switch.
 * Purpose: Names the basic master type in the debug HUD.
 */
char g_Player_MasterTypeName_Basic[0x06] = "BASIC";
RECOIL_STATIC_ASSERT(sizeof(g_Player_MasterTypeName_Basic) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptertypename02
 * @recoil-artifact defines .data recoil:data:0x4dca70: g_Player_CopterTypeName02.
 * BN types this as a writable .data char[0x09] used by the copter sound-node
 * cache when binding the second copter actor by type/name.
 * Purpose: Names the second copter object for copter sound-node caching.
 */
char g_Player_CopterTypeName02[0x09] = "copter02";
RECOIL_STATIC_ASSERT(sizeof(g_Player_CopterTypeName02) == 0x09);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-coptertypename01
 * @recoil-artifact defines .data recoil:data:0x4dca7c: g_Player_CopterTypeName01.
 * BN types this as a writable .data char[0x09] used by the copter sound-node
 * cache when binding the first copter actor by type/name.
 * Purpose: Names the first copter object for copter sound-node caching.
 */
char g_Player_CopterTypeName01[0x09] = "copter01";
RECOIL_STATIC_ASSERT(sizeof(g_Player_CopterTypeName01) == 0x09);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-pickupoptkey-drop
 * @recoil-artifact defines .data recoil:data:0x4dca88: g_PickupOptKey_Drop.
 * BN types this as a writable .data char[0x05] used by Player async command
 * callback case 914 when spawning a carrier-node pickup.
 * Purpose: Names the drop pickup option key used by async debug commands.
 */
char g_PickupOptKey_Drop[0x05] = "drop";
RECOIL_STATIC_ASSERT(sizeof(g_PickupOptKey_Drop) == 0x05);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-pickupoptkey-crbox
 * @recoil-artifact defines .data recoil:data:0x4dca90: g_PickupOptKey_Crbox.
 * BN types this as a writable .data char[0x06] used by Player async command
 * callback case 913 when spawning a carrier-node pickup.
 * Purpose: Names the crbox pickup option key used by async debug commands.
 */
char g_PickupOptKey_Crbox[0x06] = "crbox";
RECOIL_STATIC_ASSERT(sizeof(g_PickupOptKey_Crbox) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-pickupoptkey-vwbus
 * @recoil-artifact defines .data recoil:data:0x4dca98: g_PickupOptKey_Vwbus.
 * BN types this as a writable .data char[0x06] used by Player async command
 * callback case 912 when spawning a carrier-node pickup.
 * Purpose: Names the vwbus pickup option key used by async debug commands.
 */
char g_PickupOptKey_Vwbus[0x06] = "vwbus";
RECOIL_STATIC_ASSERT(sizeof(g_PickupOptKey_Vwbus) == 0x06);
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-copterhealthynode1
 * @recoil-artifact defines .data recoil:data:0x4f3bbc: g_Player_CopterHealthyNode1.
 * Data owner 0x4f36c4/0x4f36c8/0x4f36cc and 0x4f3bbc/0x4f3bc0: zero-initialized copter
 * sound-node cache used by the player.cpp copter sound helpers. Mission init seeds the
 * sample/cache, 0x42b630 lazily binds the copter nodes, and 0x42b5a0 reactivates sound nodes
 * while healthy.
 * Purpose: stores the plan-tracked g_Player_CopterHealthyNode1 gameplay data symbol.
 */
CZNodePartial* g_Player_CopterHealthyNode1 = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-copterhealthynode2
 * @recoil-artifact defines .data recoil:data:0x4f3bc0: g_Player_CopterHealthyNode2.
 * Purpose: stores the plan-tracked g_Player_CopterHealthyNode2 gameplay data symbol.
 */
CZNodePartial* g_Player_CopterHealthyNode2 = 0;
} // extern "C"
namespace {
enum PlayerMasterTypeId {
    kPlayerMasterTypeFly = 1,
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3,
    kPlayerMasterTypeHover = 4,
    kPlayerMasterTypeAmphib = 5
};
/**
 * @recoil-anchor recoil:anchor:battlesport-player-master-type-track-cooldown-negative
 * @recoil-artifact defines .rdata recoil:data:0x4d0860: Shared negative unit interval.
 * @recoil-artifact defines .rdata recoil:data:0x4d0868: Negative fly cooldown interval.
 *
 * Purpose: retain the distinct retail cooldown scalars; original names remain inferred.
 */
const float kPlayerMasterTypeTrackCooldownNegSec = -1.0f;
const float kPlayerMasterTypeFlyCooldownNegSec = -5.0f;

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x42ac90 Player::TransitionToMasterTypeTrack, 0x42aeb0
 * Player::TransitionToMasterTypeAmphib, 0x42b2a0 Player::TransitionToMasterTypeSub, 0x42b0f0
 * Player::TransitionToMasterTypeHover. Purpose: provide the recovered trigger zero velocity fx list helper for the
 * Player/Pickup gameplay source cluster.
 */
#define PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(entries, rootNode, flags)                                                 \
    do {                                                                                                               \
        for (int playerFxIndex = 0; playerFxIndex < 2; ++playerFxIndex) {                                              \
            zEffectAnimEntry* const playerFxEntry = (entries)[playerFxIndex];                                          \
            if (playerFxEntry != 0 && (flags) == 0) {                                                                  \
                zEffectAnim::SetVelocityThunk(playerFxEntry, (rootNode), 0.0f, 0.0f, 0.0f);                            \
            }                                                                                                          \
        }                                                                                                              \
    } while (0)
} // namespace
namespace Player {
enum { kPlayerMasterTypeSub = 2, kPlayerMaxModalProbePoints = 4, kPlayerEnvProbeBasePointOffset = 15 };
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-addscaledhudcountervalue
 * @recoil-artifact defines .text recoil:function:0x42a9f0: Player::AddScaledHudCounterValue.
 * @recoil-artifact emits .rdata recoil:data:0x4d0848: Unit scale.
 * @recoil-artifact emits .rdata recoil:data:0x4d084c: Counter units.
 * @recoil-match byte
 *
 * Purpose: Scale HUD contributions by dispatch count; constant names/storage are inferred.
 */
void __fastcall AddScaledHudCounterValue(float value)
{
    static const float unitScale = 1.0f;
    static const float counterUnits = 1000.0f;
    double scale;
    if (g_HudSensorTracker.primaryGunDispatchCount > 0) {
        scale = (float)g_OptCatalog_DamageFeedbackHitCount / g_HudSensorTracker.primaryGunDispatchCount;
    } else {
        scale = unitScale;
    }
    // This VC5 double-scale/float-storage model retains the x87 scale
    // across _ftol and reproduces retail's native post-call discard.
    g_Player_HudCounterValue = g_Player_HudCounterValue + (int)((value * scale) * counterUnits);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-getsavestatelisthead
 * @recoil-artifact defines .text recoil:function:0x42aa40: Player::GetSaveStateListHead
 * @recoil-match byte
 *
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: return the global head of the player save-state list.
 * Source owner: Player save-state/bootstrap record-global subsystem, not a
 * C++ Player class.
 */
zUtil_SaveGameState* __cdecl GetSaveStateListHead()
{
    return g_PlayerSaveStateList.head;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatedebugoverlayhud
 * @recoil-artifact defines .text recoil:function:0x42aa50: Player::UpdateDebugOverlayHud.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: refresh weapon HUD values, objective counter text, and the debug
 * overlay lines for the current player save state.
 */
void __fastcall
UpdateDebugOverlayHud(zUtil_SaveGameState* saveState, int unusedActiveMode2Count, int unusedTotalMode2Count)
{
    (void)unusedActiveMode2Count;
    (void)unusedTotalMode2Count;

    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    HudUiMgr::SetReticleMode(
        playerState->activeAltGunController->optCatalogEntry->range > playerState->aimTargetDistanceApprox
                && playerState->activeAltGunController->ammoOrCharge != 0.0f
            ? 1
            : 0
    );

    HudUiMessage::SetValueIfOwnerMatches(
        playerState->activeAltGunController->weaponBankIndex,
        playerState->activeAltGunController->weaponSideIndex,
        playerState->activeAltGunController->ammoOrCharge
    );

    PlayerGunFireController* const primaryController = playerState->activePrimaryGunController;
    if (primaryController != 0) {
        HudUiMessage::SetValueIfOwnerMatches(
            primaryController->weaponBankIndex,
            primaryController->weaponSideIndex,
            primaryController->ammoOrCharge
        );
    }

    HudUiMgrObjective::RefreshCounterText(g_Player_HudCounterValue);

    char masterTypeName[12];
    switch (saveState->primaryModalState->masterModalData->masterType) {
    case 0:
        strcpy(masterTypeName, g_Player_MasterTypeName_Basic);
        break;
    case kPlayerMasterTypeTrack:
        strcpy(masterTypeName, g_Player_MasterTypeName_Track);
        break;
    case kPlayerMasterTypeHover:
        strcpy(masterTypeName, g_Player_MasterTypeName_Hover);
        break;
    case kPlayerMasterTypeAmphib:
        strcpy(masterTypeName, g_Player_MasterTypeName_Amphib);
        break;
    case kPlayerMasterTypeSub:
        strcpy(masterTypeName, g_Player_MasterTypeName_Sub);
        break;
    case kPlayerMasterTypeFly:
        strcpy(masterTypeName, g_Player_MasterTypeName_Fly);
        break;
    default:
        strcpy(masterTypeName, g_Player_MasterTypeName_Unknown);
        break;
    }

    char debugLine[256];
    const int lifecycleState = playerState->lifecycleState;
    if (lifecycleState == kPlayerLifecycleLocal || lifecycleState == 0) {
        if (playerState->airborneFlag != 0) {
            sprintf(debugLine, g_Player_HudReadoutFmt_DynamicsA, playerState->rootNode->name, masterTypeName);
        } else if (playerState->slipSfxActive != 0) {
            sprintf(debugLine, g_Player_HudReadoutFmt_DynamicsS, playerState->rootNode->name, masterTypeName);
        } else {
            sprintf(debugLine, g_Player_HudReadoutFmt_Dynamics, playerState->rootNode->name, masterTypeName);
        }
    } else if (lifecycleState == kPlayerLifecycleAi) {
        sprintf(
            debugLine,
            g_Player_HudReadoutFmt_ModeGoalNode,
            playerState->rootNode->name,
            playerState->aiTopLevelState,
            playerState->aiCurrentPathNode->nodeIndex
        );
    } else if (lifecycleState == kPlayerLifecycleInactive) {
        sprintf(debugLine, g_Player_HudReadoutFmt_Dead, playerState->rootNode->name);
    }

    HudUiAuxOverlay::UpdateTextLine(2, 1, debugLine);

    sprintf(
        debugLine,
        g_Player_HudReadoutFmt_PosYaw,
        (int)(playerState->worldPos.x),
        (int)(playerState->worldPos.y),
        (int)(playerState->worldPos.z),
        (int)((double)(playerState->restartYawRad) * 57.29577951308)
    );
    HudUiAuxOverlay::UpdateTextLine(2, 2, debugLine);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypetrack
 * @recoil-artifact defines .text recoil:function:0x42ac90: Player::TransitionToMasterTypeTrack
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: enter track mode after cooldown and source-mode transition rules
 * allow it.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation matches the known Player modal/state
 * model with SUB/HOVER/AMPHIB source gates, underwater HUD and copter sound
 * cleanup, source FX dispatch, mode variant activation, HUD counter update,
 * stale amphib light stop, track node action, and transition light handle
 * creation.
 */
int __fastcall TransitionToMasterTypeTrack(zUtil_SaveGameState* saveState, int flags)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }

    switch (masterModalData->masterType) {
    case kPlayerMasterTypeHover:
        if (playerState->autoTurnSign != 0) {
            return 0;
        }

        PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(masterModalData->fxList_fromHoverToTrack, playerState->rootNode, flags);
        break;
    case kPlayerMasterTypeAmphib:
        PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(masterModalData->fxList_fromAmphibToTrack, playerState->rootNode, flags);
        break;
    case kPlayerMasterTypeSub: {
        if (flags == 0) {
            return 0;
        }

        CZObject3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerA.attachNodePrimary,
            0.0f,
            0.0f,
            0.0f
        );
        CZObject3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerA.attachNodeSecondary,
            0.0f,
            0.0f,
            0.0f
        );
        CZObject3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerB.attachNodePrimary,
            0.0f,
            0.0f,
            0.0f
        );
        CZObject3D::gwObject3DSetPosition(
            playerState->altWeaponBanks[1].controllerB.attachNodeSecondary,
            0.0f,
            0.0f,
            0.0f
        );
        ((HudUiElement*)(&g_Player_UnderwaterFxPass3Ui))->SetVisible(0);
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        CZNodePartial* const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue;
            CZClass::gwNodeGetUserData(nodeCaustic1, &displayInstanceValue);
            zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial*)displayInstanceValue, 0.0f);
        }

        playerState->damageVisualFlag = 1;
        break;
    }
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeTrack);
    playerState->masterTypeTransitionCooldownUntilTime
        = g_Time_AccumulatedTimeSec - kPlayerMasterTypeTrackCooldownNegSec;
    CZClass::gwNodeSetActive(playerState->modeVariantNode, 1);

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x238), 5.0f);
        HudUiMgr::SetModeCounterState(0, 2);
    }

    zEffectAnimEntry* const toAmphibLightHandle = playerState->masterTypeTransitionToAmphibLightHandle;
    if (toAmphibLightHandle != 0) {
        zEffectAnim::Stop(toAmphibLightHandle);
        playerState->masterTypeTransitionToAmphibLightHandle = 0;
    }

    zEffect_Anim::zEffAnimReset(playerState->masterTypeTransitionToTrackNodeAction, playerState->rootNode);
    playerState->masterTypeTransitionToTrackLightHandle = zEffectAnim::SetVelocityThunk(
        playerState->masterTypeTransitionToTrackNodeAction,
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypeamphib
 * @recoil-artifact defines .text recoil:function:0x42aeb0: Player::TransitionToMasterTypeAmphib
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: enter amphib mode when unlocked, off cooldown, and accepted by the
 * source-mode transition rules.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation preserves the fastcall-plus-stack source
 * shape for transition and extra flags, amphib unlock/cooldown guards, SUB
 * cleanup and FX path, TRACK/HOVER source FX paths, modal selection, pitch/roll
 * reset, HUD counter update, stale track light stop, and amphib light start.
 */
int __fastcall TransitionToMasterTypeAmphib(zUtil_SaveGameState* saveState, int transitionFlags, int extraFlags)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->amphibUnlocked == 0) {
        return 0;
    }

    switch (masterModalData->masterType) {
    case kPlayerMasterTypeHover:
        PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(
            masterModalData->fxList_fromHoverToAmphib,
            playerState->rootNode,
            extraFlags
        );
        break;
    case kPlayerMasterTypeTrack: {
        playerState->airborneFlag = 0;
        CZNodePartial* const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            CZObject3D::gwObject3DSetRotation(modalNode, 0.0f, 0.0f, 0.0f);
        }
        CZClass::gwNodeSetActive(playerState->modeVariantNode, 1);
        PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(
            masterModalData->fxList_fromTrackToAmphib,
            playerState->rootNode,
            extraFlags
        );
        break;
    }
    case kPlayerMasterTypeSub: {
        if (transitionFlags != 0) {
            return 0;
        }

        ((HudUiElement*)(&g_Player_UnderwaterFxPass3Ui))->SetVisible(0);
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        CZNodePartial* const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue;
            CZClass::gwNodeGetUserData(nodeCaustic1, &displayInstanceValue);
            zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial*)displayInstanceValue, 0.0f);
        }

        StopBftBubbleFxHandle(saveState);
        PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(
            masterModalData->fxList_fromSubToAmphib,
            playerState->rootNode,
            extraFlags
        );
        playerState->damageVisualFlag = 1;
        break;
    }
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeAmphib);
    playerState->masterTypeTransitionCooldownUntilTime
        = g_Time_AccumulatedTimeSec - kPlayerMasterTypeTrackCooldownNegSec;

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x239), 5.0f);
        HudUiMgr::SetModeCounterState(1, 2);
    }

    playerState->vehicleRollRad = 0.0f;
    playerState->vehiclePitchRad = 0.0f;

    zEffectAnimEntry* const toTrackLightHandle = playerState->masterTypeTransitionToTrackLightHandle;
    if (toTrackLightHandle != 0) {
        zEffectAnim::Stop(toTrackLightHandle);
        playerState->masterTypeTransitionToTrackLightHandle = 0;
    }

    zEffect_Anim::zEffAnimReset(playerState->masterTypeTransitionToAmphibNodeAction, playerState->rootNode);
    playerState->masterTypeTransitionToAmphibLightHandle = zEffectAnim::SetVelocityThunk(
        playerState->masterTypeTransitionToAmphibNodeAction,
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypehover
 * @recoil-artifact defines .text recoil:function:0x42b0f0: Player::TransitionToMasterTypeHover
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: enter hover mode when unlocked, off cooldown, and accepted by the
 * source-mode transition rules.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation follows the Player modal/state source
 * model with hover unlock/cooldown guards, SUB cleanup and damage visual latch,
 * TRACK rotation and airborne reset, AMPHIB/HOVER FX paths, modal selection,
 * one-second cooldown update, and local HUD counter update.
 */
int __fastcall TransitionToMasterTypeHover(zUtil_SaveGameState* saveState, int flags)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->hoverUnlocked == 0) {
        return 0;
    }

    switch (masterModalData->masterType) {
    case kPlayerMasterTypeAmphib:
        PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(masterModalData->fxList_fromAmphibToHover, playerState->rootNode, flags);
        break;
    case kPlayerMasterTypeTrack: {
        playerState->airborneFlag = 0;
        CZNodePartial* const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            CZObject3D::gwObject3DSetRotation(modalNode, 0.0f, 0.0f, 0.0f);
        }
        CZClass::gwNodeSetActive(playerState->modeVariantNode, 1);
        PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(masterModalData->fxList_fromTrackToHover, playerState->rootNode, flags);
        break;
    }
    case kPlayerMasterTypeSub: {
        if (flags == 0) {
            return 0;
        }

        ((HudUiElement*)(&g_Player_UnderwaterFxPass3Ui))->SetVisible(0);
        g_Player_HorizonNodeFollowCameraEnabled = 1;
        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        ReactivateCopterSndNodesIfHealthy();

        CZNodePartial* const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue;
            CZClass::gwNodeGetUserData(nodeCaustic1, &displayInstanceValue);
            zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial*)displayInstanceValue, 0.0f);
        }

        playerState->damageVisualFlag = 1;
        break;
    }
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeHover);
    playerState->masterTypeTransitionCooldownUntilTime
        = g_Time_AccumulatedTimeSec - kPlayerMasterTypeTrackCooldownNegSec;

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x23a), 5.0f);
        HudUiMgr::SetModeCounterState(2, 2);
    }

    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypesub
 * @recoil-artifact defines .text recoil:function:0x42b2a0: Player::TransitionToMasterTypeSub
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: enter sub mode after applying gun-slot offsets, transition gates,
 * source-mode cleanup, modal selection, alternate-weapon validation, and FX
 * updates.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation matches the known save-state and player
 * state model with damage visual latching before gates, cooldown/sub unlock
 * exits, SUB/TRACK/AMPHIB source rules, forced descent nudge, alt-weapon
 * fallback, loop SFX and copter sound handling, stale light stops, and sub
 * transition light creation.
 */
int __fastcall TransitionToMasterTypeSub(zUtil_SaveGameState* saveState, int flags)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    playerState->damageVisualFlag = 1;
    ApplyGunFireSlotOffsetToNode(saveState);

    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }
    if (playerState->subUnlocked == 0) {
        return 0;
    }

    switch (masterModalData->masterType) {
    case kPlayerMasterTypeTrack: {
        if (flags == 0) {
            return 0;
        }

        playerState->airborneFlag = 0;
        CZNodePartial* const modalNode = primaryModalState->modalNode;
        if (modalNode != 0) {
            CZObject3D::gwObject3DSetRotation(modalNode, 0.0f, 0.0f, 0.0f);
        }

        // Retail falls through into the amphib transition; with flags set its checks and FX triggers are no-ops.
    }
    case kPlayerMasterTypeAmphib:
        if (playerState->bankInput != 0 && flags == 0) {
            return 0;
        }

        PLAYER_TRIGGER_ZERO_VELOCITY_FX_LIST(masterModalData->fxList_fromAmphibToSub, playerState->rootNode, flags);
        playerState->localVel.y = -3.0f;
        playerState->worldPos.y -= 4.0999999f;
        break;
    case kPlayerMasterTypeSub:
        if (flags == 0) {
            return 1;
        }

        saveState->StopMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack);
        break;
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeSub);

    if (Player::IsAltWeaponAllowedInCurrentMasterMode(saveState, playerState->activeAltGunController->optCatalogEntry)
        == 0) {
        Player::AutoSwitchToNextUsableAltWeapon(saveState);
    }

    playerState->masterTypeTransitionCooldownUntilTime
        = g_Time_AccumulatedTimeSec - kPlayerMasterTypeTrackCooldownNegSec;

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x23b), 5.0f);
        HudUiMgr::SetModeCounterState(3, 2);
        saveState->EnsureMasterTypeLoopSfxHandle(kPlayerMasterTypeTrack, 0.5f);
        CacheDisableCopterSndNodesAndStopSample();
    }

    zEffectAnimEntry* const toTrackLightHandle = playerState->masterTypeTransitionToTrackLightHandle;
    if (toTrackLightHandle != 0) {
        zEffectAnim::Stop(toTrackLightHandle);
        playerState->masterTypeTransitionToTrackLightHandle = 0;
    }

    zEffectAnimEntry* const toAmphibLightHandle = playerState->masterTypeTransitionToAmphibLightHandle;
    if (toAmphibLightHandle != 0) {
        zEffectAnim::Stop(toAmphibLightHandle);
        playerState->masterTypeTransitionToAmphibLightHandle = 0;
    }

    zEffect_Anim::zEffAnimReset(playerState->masterTypeTransitionToSubNodeAction, playerState->rootNode);
    playerState->masterTypeTransitionToSubLightHandle = zEffectAnim::SetVelocityThunk(
        playerState->masterTypeTransitionToSubNodeAction,
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-stopbftbubblefxhandle
 * @recoil-artifact defines .text recoil:function:0x42b4a0: Player::StopBftBubbleFxHandle.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::StopBftBubbleFxHandle from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall StopBftBubbleFxHandle(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zEffectAnimEntry* const handle = playerState->masterTypeTransitionToSubLightHandle;
    if (handle != 0) {
        zEffectAnim::Stop(handle);
        playerState->masterTypeTransitionToSubLightHandle = 0;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-transitiontomastertypefly
 * @recoil-artifact defines .text recoil:function:0x42b4c0: Player::TransitionToMasterTypeFly.
 * @recoil-match byte
 *
 * Purpose: select the fly modal state when the transition cooldown allows it.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation follows the reviewed Player save-state
 * model: cooldown guard, SUB-source damage visual latch, source master-type
 * capture, fly modal selection, five-second cooldown update, and integer
 * success/failure return.
 */
int __fastcall TransitionToMasterTypeFly(zUtil_SaveGameState* saveState, int flags)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    if (g_Time_AccumulatedTimeSec < playerState->masterTypeTransitionCooldownUntilTime) {
        return 0;
    }

    switch (masterModalData->masterType) {
    case kPlayerMasterTypeSub:
        playerState->damageVisualFlag = 1;
        break;
    }

    playerState->currentMasterType = masterModalData->masterType;
    saveState->SelectModalStateByMasterType(kPlayerMasterTypeFly);
    playerState->masterTypeTransitionCooldownUntilTime = g_Time_AccumulatedTimeSec - kPlayerMasterTypeFlyCooldownNegSec;
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applymastertypetransition
 * @recoil-artifact defines .text recoil:function:0x42b520: Player::ApplyMasterTypeTransition
 * @recoil-match byte
 *
 * Purpose: reset the primary-gun gate timestamp and dispatch a requested master type to the concrete transition helper.
 * Source owner: Player master-type transition cluster.
 * Evidence: existing implementation preserves the dispatcher source shape:
 * writes primaryGunGateUntilTime from accumulated time, maps FLY/SUB/TRACK/
 * HOVER/AMPHIB cases to the reviewed transition helpers, passes AMPHIB
 * transitionFlags as zero with caller flags as extraFlags, and returns
 * masterType - 1 for unsupported requests.
 */
int __fastcall ApplyMasterTypeTransition(zUtil_SaveGameState* saveState, int masterType, int flags)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;

    switch (masterType) {
    case kPlayerMasterTypeTrack:
        return TransitionToMasterTypeTrack(saveState, flags);
    case kPlayerMasterTypeAmphib:
        return TransitionToMasterTypeAmphib(saveState, 0, flags);
    case kPlayerMasterTypeHover:
        return TransitionToMasterTypeHover(saveState, flags);
    case kPlayerMasterTypeSub:
        return TransitionToMasterTypeSub(saveState, flags);
    case kPlayerMasterTypeFly:
        return TransitionToMasterTypeFly(saveState, flags);
    default:
        return masterType - 1;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-reactivatecoptersndnodesifhealthy
 * @recoil-artifact defines .text recoil:function:0x42b5a0: Player::ReactivateCopterSndNodesIfHealthy
 * @recoil-match byte
 *
 * Purpose: reactivate each cached copter sound node whose healthy node remains
 * active, then restart the cached chopper sample through the node play handle.
 */
void __cdecl ReactivateCopterSndNodesIfHealthy()
{
    if (g_Player_CopterHealthyNode1 != 0 && (g_Player_CopterHealthyNode1->flags & 0x04) != 0
        && g_Player_CopterSndNode1 != 0) {
        CZClass::gwNodeSetActive(g_Player_CopterSndNode1, 1);
        CZSoundDataPartial* const soundData1 = (CZSoundDataPartial*)(g_Player_CopterSndNode1->classData);
        if (soundData1 != 0) {
            zSndPlayHandle* const playHandle1 = soundData1->playHandle;
            if (playHandle1 != 0) {
                zSndPlayHandle::PlayWithDeltaBackendDispatch(g_Player_CopterSndSample, playHandle1, 0, 0.0f);
            }
        }
    }

    if (g_Player_CopterHealthyNode2 != 0 && (g_Player_CopterHealthyNode2->flags & 0x04) != 0
        && g_Player_CopterSndNode2 != 0) {
        CZClass::gwNodeSetActive(g_Player_CopterSndNode2, 1);
        CZSoundDataPartial* const soundData2 = (CZSoundDataPartial*)(g_Player_CopterSndNode2->classData);
        if (soundData2 != 0) {
            zSndPlayHandle* const playHandle2 = soundData2->playHandle;
            if (playHandle2 != 0) {
                zSndPlayHandle::PlayWithDeltaBackendDispatch(g_Player_CopterSndSample, playHandle2, 0, 0.0f);
            }
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-cachedisablecoptersndnodesandstopsample
 * @recoil-artifact defines .text recoil:function:0x42b630: Player::CacheDisableCopterSndNodesAndStopSample
 * @recoil-match byte
 *
 * Purpose: lazily cache the two copter healthy/sound scene nodes, disable the
 * sound nodes, and stop active chopper sample voices.
 */
void __cdecl CacheDisableCopterSndNodesAndStopSample()
{
    if (g_Player_CopterSndNode1 == 0) {
        CZNodePartial* const copterRoot = CZClass::FindByTypeAndName(6, g_Player_CopterTypeName01);
        if (copterRoot != 0) {
            g_Player_CopterHealthyNode1 = CZClass::FindSubNodeByName(copterRoot, g_Player_HealthySubNodeName);
            g_Player_CopterSndNode1 = CZClass::FindSubNodeByName(copterRoot, "snd_chopper");
        }
    }

    if (g_Player_CopterSndNode2 == 0) {
        CZNodePartial* const copterRoot = CZClass::FindByTypeAndName(6, g_Player_CopterTypeName02);
        if (copterRoot != 0) {
            g_Player_CopterHealthyNode2 = CZClass::FindSubNodeByName(copterRoot, g_Player_HealthySubNodeName);
            g_Player_CopterSndNode2 = CZClass::FindSubNodeByName(copterRoot, "snd_chopper");
        }
    }

    if (g_Player_CopterSndNode1 != 0) {
        CZClass::gwNodeSetActive(g_Player_CopterSndNode1, 0);
    }
    if (g_Player_CopterSndNode2 != 0) {
        CZClass::gwNodeSetActive(g_Player_CopterSndNode2, 0);
    }

    g_Player_CopterSndSample->StopActiveVoicesIfPlaying();
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-findnearestthirdpersoncameraprobepoint
 * @recoil-artifact defines .text recoil:function:0x42b6e0: Player::FindNearestThirdPersonCameraProbePoint.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::FindNearestThirdPersonCameraProbePoint from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall FindNearestThirdPersonCameraProbePoint(
    PlayerProbeSampleCandidateBuffer* batches,
    int batchCount,
    const zVec3* referencePos,
    zVec3* outHitPos
)
{
    int found = 0;
    int bestBatchIndex;
    int bestEntryIndex;

    for (int batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        PlayerProbeSampleCandidateBuffer* const batch = &batches[batchIndex];
        if (batch->candidateCount != 0) {
            for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
                if (batch->entries[hitIndex].node != 0) {
                    bestBatchIndex = batchIndex;
                    found = 1;
                    bestEntryIndex = hitIndex;
                    batchIndex = batchCount;
                    break;
                }
            }
        }
    }

    if (found == 0) {
        return 0;
    }

    PlayerProbeSampleCandidateBuffer* const bestBatch = &batches[bestBatchIndex];
    zVec3* const bestHitPos = &bestBatch->entries[bestEntryIndex].hitPos;
    float bestDistSq = zMath::Vec3DeltaLengthSq(bestHitPos, referencePos);

    for (int searchBatchIndex = 0; searchBatchIndex < batchCount; ++searchBatchIndex) {
        PlayerProbeSampleCandidateBuffer* const batch = &batches[searchBatchIndex];
        for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
            zClassDiPickCandidateEntry* const candidate = &batch->entries[hitIndex];
            if (candidate->node != 0) {
                const float distSq = zMath::Vec3DeltaLengthSq(&candidate->hitPos, referencePos);
                if (distSq < bestDistSq) {
                    bestDistSq = distSq;
                    bestEntryIndex = hitIndex;
                    bestBatchIndex = searchBatchIndex;
                }
            }
        }
    }

    *outHitPos = batches[bestBatchIndex].entries[bestEntryIndex].hitPos;
    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-synclocalposefromrootnode
 * @recoil-artifact defines .text recoil:function:0x42b810: Player::SyncLocalPoseFromRootNode.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SyncLocalPoseFromRootNode from the recovered
 * Battlesport gameplay source file.
 */
void __cdecl SyncLocalPoseFromRootNode()
{
    zInput_GameStateOrMapTablePartial* const gameState = g_GameStateOrMapTable;
    zUtil_SaveGameState* const saveState = (zUtil_SaveGameState*)gameState;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    CZObject3D::gwObject3DGetPosition(
        playerState->rootNode,
        &playerState->worldPos.x,
        &playerState->worldPos.y,
        &playerState->worldPos.z
    );
    CZObject3D::gwObject3DGetRotation(
        playerState->rootNode,
        &playerState->vehiclePitchRad,
        &playerState->restartYawRad,
        &playerState->vehicleRollRad
    );
    zMath::MatBuildEulerRotation3x3(
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad,
        &playerState->motionBasis
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->previousTransform = playerState->motionBasis;
    playerState->lifecycleState = 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildsteerbasisrawfromref
 * @recoil-artifact defines .text recoil:function:0x42b8c0: Player::RebuildSteerBasisRawFromRef.
 * @recoil-match byte
 *
 * Purpose: normalize the steering direction projected onto the reference plane.
 */
void __fastcall RebuildSteerBasisRawFromRef(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    const zVec3 normal = playerState->steerBasisNorm;
    const zVec3 reference = playerState->steerBasisRef;

    if (reference.y == 0.0f) {
        return;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y = -((reference.x * normal.x + reference.z * normal.z) / reference.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildmotionbasisfromsteerbasis
 * @recoil-artifact defines .text recoil:function:0x42b970: Player::RebuildMotionBasisFromSteerBasis.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-match byte
 *
 * Purpose: Build the motion matrix from the steering axes and world position.
 * The cross product writes the side axis before native C++ fills the matrix.
 * Pro-reviewed raw scope is only [0x42b994,0x42b9d5), including pointer reloads.
 * C++ owns parameter homes, frame, saves and the complete matrix copy.
 * The capturing macro spelling and historical header placement remain inferred.
 */
void __fastcall RebuildMotionBasisFromSteerBasis(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    zVec3 basisSide;
    ZMTH_VECTOR_CROSS(&playerState->steerBasisRaw, &playerState->steerBasisRef, &basisSide);

    zMat4x3 motionBasis;
    motionBasis.xx = basisSide.x;
    motionBasis.xy = basisSide.y;
    motionBasis.xz = basisSide.z;
    motionBasis.yx = playerState->steerBasisRef.x;
    motionBasis.yy = playerState->steerBasisRef.y;
    motionBasis.yz = playerState->steerBasisRef.z;
    motionBasis.zx = -playerState->steerBasisRaw.x;
    motionBasis.zy = -playerState->steerBasisRaw.y;
    motionBasis.zz = -playerState->steerBasisRaw.z;
    motionBasis.posX = playerState->worldPos.x;
    motionBasis.posY = playerState->worldPos.y;
    motionBasis.posZ = playerState->worldPos.z;

    playerState->motionBasis = motionBasis;
}
} // namespace Player
namespace CZDisplayInstance {
/**
 * @recoil-anchor recoil:anchor:battlesport.player-state.czdisplay-instance-snap-probe-point-yto-best-candidate
 * @recoil-artifact defines .text recoil:function:0x42ba50: CZDisplayInstance::SnapProbePointYToBestCandidate.
 * @recoil-match byte
 *
 * Provenance: address-backed cls_di.c reconstruction from current Binary Ninja
 * behavior/global evidence; native smoke coverage exercises the owner slice.
 * Purpose: preserve the recovered cls_di raycast/filter runtime behavior.
 */
int __fastcall SnapProbePointYToBestCandidate(zVec3* point)
{
    PlayerProbeSampleCandidateBuffer candidateBuffer;
    const int result
        = BuildPickCandidateListBelowPoint(g_Player_RuntimeDiScene, point->x, 500.0f, point->z, &candidateBuffer);
    int selectedImpactSlot;
    int bestCandidateIndex;
    float taggedHeight;
    point->y = Player::SelectProbeSampleHeightFromCandidates(
        &candidateBuffer,
        point->y,
        &bestCandidateIndex,
        2.0f,
        0,
        &selectedImpactSlot,
        &taggedHeight
    );
    return result;
}
} // namespace CZDisplayInstance
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-setautoturntargetdirfromworldpoint
 * @recoil-artifact defines .text recoil:function:0x42bab0: Player::SetAutoTurnTargetDirFromWorldPoint.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SetAutoTurnTargetDirFromWorldPoint from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall SetAutoTurnTargetDirFromWorldPoint(zUtil_SaveGameState* saveState, const zVec3* worldPoint)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zVec3 targetDir;
    zMath::Vec3Subtract(worldPoint, &playerState->worldPos, &targetDir);
    targetDir.y = 0.0f;

    // Retail leaves the output Y slot untouched, then copies the whole vector.
    zVec3 normalizedTargetDir;
    zMath::Vec3NormalizeXZ(&targetDir, &normalizedTargetDir);
    playerState->autoTurnTargetDir = normalizedTargetDir;
    playerState->autoTurnActive = 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-asynccommandcallback
 * @recoil-artifact defines .text recoil:function:0x42bb30: Player::AsyncCommandCallback
 * @recoil-match byte
 *
 * Purpose: Dispatches script async command events that toggle HUD/gameplay
 * state, apply debug damage, and spawn debug pickup carrier nodes.
 */
void __fastcall AsyncCommandCallback(zEffectAnimEntry* animEntry, void*, int eventCode)
{
    switch (eventCode) {
    case 0:
        if (animEntry == g_Player_ActiveDebugScriptAsyncEntry) {
            g_Player_ActiveDebugScriptAsyncEntry = 0;
        }
        return;

    case 1:
        g_Player_RebuildCameraDirFlatFromCurrentTarget = 1;
        BindActiveGameStateAsCurrentSaveState();
        return;

    case 2:
        UnbindCurrentSaveStateIfSinglePlayer();
        HudUiMgr::DisableHud();
        HudUiMgr::UpdateTargetReticleFromCursor(0, 0.0f, 0.0f, 0);
        HudUiMgr::DisableTopAndChatStacks();
        return;

    case 10:
        SyncLocalPoseFromRootNode();
        HudUiMgr::EnableTopAndChatStacks();
        return;

    case 11:
        if (zOpt::GetNetworkEnabled() == 0) {
            ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->lifecycleState = kPlayerLifecycleState6Inactive;
            ((zUtil_SaveGameState*)g_GameStateOrMapTable)->UpdateModalLoopSfx(0);
        }
        return;

    case 14:
        if (zOpt::GetNetworkEnabled() == 0) {
            g_Player_LocalControlEnabled = 0;
            HudUiMgr::DisableHud();
            HudUiMgr::UpdateTargetReticleFromCursor(0, 0.0f, 0.0f, 0);
            HudUiTimerPanel::SetRunning(0);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
        }
        zTurret_System::DisableTickCallback();
        return;

    case 15:
        if (zOpt::GetNetworkEnabled() == 0) {
            g_Player_LocalControlEnabled = 1;
            if (zOpt::GetHudVisibilityOption() != 0) {
                HudUiMgr::ApplyHudModeSwitch(zOpt::GetHudTypeForCurrentHwMode());
                HudUiMgr::EnableHud();
            }
            HudUiMgr::UpdateTargetReticleFromCursor(1, 0.5f, 0.5f, 0);
            HudUi::ShowTopMessageLine(
                ((zUtil_SaveGameState*)g_GameStateOrMapTable)
                    ->playerState->activeAltGunController->optCatalogEntry->description,
                5.0f
            );
            HudUiTimerPanel::SetRunning(1);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
            if (g_HudSensorTracker.GetMissionId() == 1 && g_HudSensorTracker.firstIncompleteObjectiveIndex == 0
                && g_HudSensorTracker.primaryGunDispatchCount == 0) {
                HudUi::PlayPowerupSfx(1);
            }
        }
        zTurret_System::EnableTickCallback();
        return;

    case 16:
        ResetMotionTransientState(((zUtil_SaveGameState*)g_GameStateOrMapTable));
        return;

    case 17:
        CaptureCurrentObjectPoseAsRestartAnchor(((zUtil_SaveGameState*)g_GameStateOrMapTable));
        return;

    case 20:
        g_Player_ActiveDebugScriptAsyncEntry = animEntry;
        return;

    case 25:
        ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->nanitePanelLevel = 0;
        HudUiMgr::SetNanitePanelCount(0);
        // The retail switch falls through to the shared destruction event.
    case 26:
        EnterDestroyedState(
            ((zUtil_SaveGameState*)g_GameStateOrMapTable),
            0,
            0,
            ((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->statusMeterValue
                - kPlayerMasterTypeTrackCooldownNegSec
        );
        return;

    case 27:
        EnterDestroyedState(((zUtil_SaveGameState*)g_GameStateOrMapTable), 0, 0, 10.0f);
        return;

    case 99:
        g_HudSensorTracker.SaveAndQueueMissionState();
        return;

    case 911:
        PickupAirdropSpawnRef::TrySpawnRandomPickupFromGlobal();
        return;

    case 912:
        Pickup::SpawnAtCarrierNodeByName(g_PickupOptKey_Vwbus, 32, 1);
        return;

    case 913:
        Pickup::SpawnAtCarrierNodeByName(g_PickupOptKey_Crbox, 36, 1);
        return;

    case 914:
        Pickup::SpawnAtCarrierNodeByName(g_PickupOptKey_Drop, 30, 1);
        return;

    default:
        return;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-setworldposeandrestartanchor
 * @recoil-artifact defines .text recoil:function:0x42be00: Player::SetWorldPoseAndRestartAnchor.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SetWorldPoseAndRestartAnchor from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall SetWorldPoseAndRestartAnchor(zUtil_SaveGameState* saveState, const zVec3* position, float yawRad)
{
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    playerState->worldPos.x = position->x;
    playerState->worldPos.y = position->y;
    playerState->worldPos.z = position->z;
    playerState->restartYawRad = yawRad;
    playerState->previousTransform.posX = position->x;
    playerState->previousTransform.posY = position->y;
    playerState->previousTransform.posZ = position->z;
    zTag4::Clear(&g_VariantTag_Current);
    g_Variant_CurrentTag = g_VariantTag_Current;
    playerState->variantTag = g_VariantTag_Current;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-capturecurrentobjectposeasrestartanchor
 * @recoil-artifact defines .text recoil:function:0x42be70: Player::CaptureCurrentObjectPoseAsRestartAnchor.
 * @recoil-match byte
 *
 * Purpose: reimplement Player::CaptureCurrentObjectPoseAsRestartAnchor from the recovered Battlesport gameplay source
 * file.
 */
void __fastcall CaptureCurrentObjectPoseAsRestartAnchor(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = g_LocalPlayerSaveState->playerState;

    zVec3 worldPos;
    CZObject3D::gwObject3DGetPosition(playerState->rootNode, &worldPos.x, &worldPos.y, &worldPos.z);

    zVec3 rotation;
    CZObject3D::gwObject3DGetRotation(playerState->rootNode, &rotation.x, &rotation.y, &rotation.z);

    SetWorldPoseAndRestartAnchor(saveState, &worldPos, rotation.y);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resetmotiontransientstate
 * @recoil-artifact defines .text recoil:function:0x42bed0: Player::ResetMotionTransientState.
 * @recoil-match byte
 *
 * Purpose: reimplement Player::ResetMotionTransientState from the recovered Battlesport gameplay source file.
 */
void __fastcall ResetMotionTransientState(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    playerState->localVel.x = playerState->localVel.y = playerState->localVel.z = 0.0f;
    playerState->projectileSpawnVel.x = playerState->projectileSpawnVel.y = playerState->projectileSpawnVel.z = 0.0f;
    playerState->yawRotatedLocalVel.x = playerState->yawRotatedLocalVel.y = playerState->yawRotatedLocalVel.z = 0.0f;
    playerState->angVelPitch = playerState->angVelYaw = playerState->angVelRoll = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->subVerticalInput = 0.0f;
    playerState->subVerticalInputCopy = 0.0f;
    playerState->steeringInputCopy = 0.0f;
    playerState->throttleInputCopy = 0.0f;
}
} // namespace Player
namespace HudUi {
/**
 * @recoil-anchor recoil:anchor:battlesport.player-state.hud-ui-play-powerup-sfx
 * @recoil-artifact defines .text recoil:function:0x42bf40: HudUi::PlayPowerupSfx.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud.cpp.
 * Purpose: lazily resolve the powerup sound sample and play or stop its active voices.
 */
void __fastcall PlayPowerupSfx(int shouldPlay)
{
    static zSndSample* powerupSample = zSnd::FindSampleByName("snd_powerup");

    if (shouldPlay != 0) {
        powerupSample->PlayA3DSimple(1.0f);
        return;
    }

    powerupSample->StopActiveVoicesIfPlaying();
}
} // namespace HudUi
