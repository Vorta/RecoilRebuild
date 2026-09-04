/* Provisional physical carrier for the reviewed mixed weapon/player source migration. */
#include "recoil/Mfc42Abi.h"
#include "Battlesport/player.h"

#include "Battlesport/game_net.h"
#include "Battlesport/ai_net.h"
#include "Battlesport/pickup.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/zTime/time.h"
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
#include "Battlesport/turret.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "Battlesport/hud.h"

#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" char g_Player_KillVerbToken[0x0a];

namespace {

enum {
    kPlayerMasterTypeSub = 2,
    kPlayerCameraStateThirdPerson = 1,
    kPlayerCameraStateFirstPerson = 3,
    kPlayerCameraStateProjectileAttached = 7,
    kPlayerCameraStateRestorePrevious = 8
};

const int kPlayerAiMode2TopSteering = 1;
const int kPlayerNanitePanelDisabledSentinel = 123456789;
const float kPlayerAltAmmoDisabledSentinel = 123456792.0f;
const float kPlayerRecentHitAlertSec = 5.0f;
const unsigned int kPlayerGunControllerAvailableFlag = 0x04;
const unsigned int kPlayerGunControllerDualMountFlag = 0x02;
const unsigned int kPlayerGunControllerRecoilFlag = 0x01;
const unsigned int kPlayerOptCatalogFlagTetherGuided = 1u << 20;
const unsigned int kOptCatalogFlagReload = 1u << 18;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;
const unsigned int kPlayerTimedHitStatusActiveFlag = 0x01;
const unsigned int kOptCatalogFlagBypassDamageProtection = 0x200;
const unsigned int kOptCatalogFlagRecordsRecentHit = 0x1000;
const unsigned int kOptCatalogFlagAppliesTimedHitStatus = 0x200000;
const unsigned int kOptCatalogFlagBlockedInSub = 0x1000;
const unsigned int kOptCatalogFlagNoSubUse = 0x02;
const int kPlayerTickCameraStateProjectileAttached = 7;
const int kPlayerTickCameraStateRestorePrevious = 8;
const zVec3 kPlayerDefaultAltGunAimOrigin = {0.0f, 0.0f, -1.0f};

struct HitOwnerSaveStateLinkPartial {
    unsigned char unknown_00[0x04];
    zUtil_SaveGameState *ownerSaveState;
};

struct HitOwnerOrContextPartial {
    unsigned char unknown_00[0x40];
    HitOwnerSaveStateLinkPartial *ownerLink;
};

/**
 * Original inline helper; no standalone retail function exists. Observed in address-backed callers 0x4386c0, 0x4289f0, 0x42c0d0, 0x42c2e0, 0x427440, 0x427ec0, 0x43a600, and 0x43a900 as a VC5-era int-bits smoothing idiom.
 * Purpose: reinterpret an IEEE-754 bit pattern as float.
 */
float PlayerFloatFromBits(
    int bits
) {
    float value = 0.0f;
    memcpy(
        &value,
        &bits,
        sizeof(value)
    );
    return value;
}
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x4024a0 AINet::SolveAltGunLeadTargetPoint, 0x43b500 Player::ApplyAimPitchToDirection, 0x43a4f0 Player::UpdateGunAndTurretAimNodes.
 * Purpose: provide the recovered player fast sqrt estimate helper for
 * the Player/Pickup gameplay source cluster.
 */
float PlayerFastSqrtEstimate(
    float value
) {
    int bits = 0;
    memcpy(
        &bits,
        &value,
        sizeof(bits)
    );
    bits = (bits >> 1) + 0x1fc00000;
    memcpy(
        &value,
        &bits,
        sizeof(value)
    );
    return value;
}
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x426770 Player::UpdateMasterTypeTrack, 0x427440 Player::UpdateMasterTypeHover_FromModalProbe, 0x43a600 Player::UpdateAltGunAimDirection.
 * Purpose: provide the recovered transform world vector to local helper for
 * the Player/Pickup gameplay source cluster.
 */
zVec3 TransformWorldVectorToLocal(
    const zVec3 &vec,
    const zMat4x3 &matrix
) {
    zVec3 out = {0};
    out.x = vec.x * matrix.xx + vec.y * matrix.xy + vec.z * matrix.xz;
    out.y = vec.x * matrix.yx + vec.y * matrix.yy + vec.z * matrix.yz;
    out.z = vec.x * matrix.zx + vec.y * matrix.zy + vec.z * matrix.zz;
    return out;
}
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x428520 Player::UpdateMasterTypeSub, 0x426770 Player::UpdateMasterTypeTrack, 0x427440 Player::UpdateMasterTypeHover_FromModalProbe, 0x427140 Player::UpdateMasterTypeHover.
 * Purpose: provide the recovered transform local vector to world helper for
 * the Player/Pickup gameplay source cluster.
 */
zVec3 TransformLocalVectorToWorld(
    const zVec3 &vec,
    const zMat4x3 &matrix
) {
    zVec3 out = {0};
    out.x = vec.x * matrix.xx + vec.y * matrix.yx + vec.z * matrix.zx;
    out.y = vec.x * matrix.xy + vec.y * matrix.yy + vec.z * matrix.zy;
    out.z = vec.x * matrix.xz + vec.y * matrix.yz + vec.z * matrix.zz;
    return out;
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed owner 0x439ba0 through the alt-gun transition
 * state code in this source file.
 * Evidence basis: repeated typed access to the transition animation scale field
 * in the lower/raise transition fragments rather than a retail helper call.
 * Purpose: name the player-state transition animation scale field used while
 * lowering and raising single-mount alt guns.
 */
float &PlayerAltGunTransitionAnimScale(
    zUtil_PlayerStateStorage *playerState
) {
    return playerState->altGunTransitionAnimScale;
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 through
 * TickAltGunTransitionAnimation's state 2 path.
 * Evidence basis: the delay/timer fragment is part of the recovered transition
 * state machine in this source file, with no separate retail callee.
 * Purpose: wait for the retract delay before starting the alt-gun lower
 * transition and master-type loop SFX.
 */
void TickAltGunRetractDelay(
    zUtil_SaveGameState *saveState,
    zUtil_PlayerStateStorage *playerState
) {
    playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
    if (playerState->altGunTransitionTimerA > 0.300000012f) {
        playerState->altGunTransitionTimerA = 0.0f;
        playerState->altGunTransitionState = 4;
        saveState->StartMasterTypeLoopSfxHandle(
            0,
            1.0f
        );
    }
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 through
 * TickAltGunTransitionAnimation's state 4 path.
 * Evidence basis: recovered inline transition math, node position, scale, and
 * rotation updates use the same player/controller fields as the surrounding
 * alt-gun runtime state machine.
 * Purpose: lower the previous single-mount alt gun toward the door before the
 * door-open transition.
 * Original-source helper.
 */
void TickAltGunLowerTransition(
    zUtil_PlayerStateStorage *playerState,
    PlayerGunFireController *controller
) {
    playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
    const float progress = playerState->altGunTransitionTimerA * 4.0f;
    const float targetY = controller->attachPosY - 0.400000006f;
    const float animScale = progress * 0.400000006f;
    PlayerAltGunTransitionAnimScale(playerState) = animScale;

    const float y = controller->attachPosY - animScale;
    if (y <= targetY) {
        zClass_Object3D::gwObject3DSetPosition(
            controller->attachNodePrimary,
            controller->attachPosX,
            targetY,
            controller->attachPosZ
        );
        zClass_Object3D::gwObject3DSetScale(
            controller->attachNodePrimary,
            1.0f,
            0.600000024f,
            0.600000024f
        );
        playerState->altGunTransitionState = 8;
        playerState->altGunTransitionTimerA = 0.0f;
        return;
    }

    zClass_Object3D::gwObject3DSetPosition(
        controller->attachNodePrimary,
        controller->attachPosX,
        y,
        controller->attachPosZ
    );
    const float scale = 1.0f - progress * 0.399999976f;
    zClass_Object3D::gwObject3DSetScale(
        controller->attachNodePrimary,
        1.0f,
        scale,
        scale
    );
    zClass_Object3D::gwObject3DSetRotation(
        controller->attachNodePrimary,
        0.0f,
        0.0f,
        0.0f
    );
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 through
 * TickAltGunTransitionAnimation's state 8 path.
 * Evidence basis: door node scale animation is a local fragment of the
 * transition state switch, not a standalone retail function.
 * Purpose: open the left/right door nodes and advance to alt-gun activation.
 */
void TickAltGunDoorOpen(
    zUtil_PlayerStateStorage *playerState
) {
    playerState->altGunTransitionTimerB += g_FrameDeltaTimeSec;
    float xScale = playerState->altGunTransitionTimerB * 4.0f;
    if (xScale >= 1.0f) {
        xScale = 1.0f;
        playerState->altGunTransitionState = 16;
        playerState->altGunTransitionTimerB = 0.0f;
    }

    zClass_Object3D::gwObject3DSetScale(
        playerState->doorLeftNode,
        xScale,
        1.0f,
        1.0f
    );
    zClass_Object3D::gwObject3DSetScale(
        playerState->doorRightNode,
        xScale,
        1.0f,
        1.0f
    );
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 through the state 16 dual-mount
 * activation path.
 * Evidence basis: opposite controller node deactivation is repeated typed
 * controller/bank field access inside the recovered alt-gun activation
 * fragment, with no separate retail callee.
 * Purpose: hide the inactive side's alt-gun mount nodes before activating a
 * dual-mount weapon.
 * Original-source helper.
 */
void DeactivateOppositeAltGunControllerNodes(
    zUtil_PlayerStateStorage *playerState,
    PlayerGunFireController *activeController
) {
    PlayerAltWeaponBank *const bank =
        &playerState->altWeaponBanks[activeController->weaponBankIndex];
    PlayerGunFireController *const oppositeController =
        activeController->weaponSideIndex == 0 ? &bank->controllerB : &bank->controllerA;

    if (oppositeController->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(
            oppositeController->attachNodePrimary,
            0
        );
    }
    if (oppositeController->attachNodeSecondary != 0) {
        zClass_Class::gwNodeSetActive(
            oppositeController->attachNodeSecondary,
            0
        );
    }
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 through
 * TickAltGunTransitionAnimation's state 16 path.
 * Evidence basis: activation, attach-node clone ownership, door state, and
 * SFX updates are one switch fragment in the recovered owner state machine.
 * Purpose: remove the outgoing alt gun, activate the selected controller, and
 * prepare the close/raise transition for single-mount weapons.
 */
void TickAltGunActivateTransition(
    zUtil_SaveGameState *saveState,
    zUtil_PlayerStateStorage *playerState
) {
    PlayerGunFireController *const transitionController = playerState->altGunTransitionController;
    if (transitionController != 0 && transitionController->attachNodePrimary != 0 &&
        (transitionController->flags & kPlayerGunControllerDualMountFlag) == 0) {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)transitionController->attachState;
        if (attachState != 0) {
            zClass_Class::RemoveChild(
                transitionController->attachNodePrimary,
                attachState->projectileNode
            );
            OptCatalog::RecycleRuntimeInstanceStorage(
                transitionController->optCatalogEntry,
                attachState
            );
            transitionController->attachState = 0;
        }

        zClass_Class::gwNodeSetActive(
            transitionController->attachNodePrimary,
            0
        );
        zClass_Object3D::gwObject3DSetPosition(
            transitionController->attachNodePrimary,
            transitionController->attachPosX,
            transitionController->attachPosY,
            transitionController->attachPosZ
        );
    }

    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    if (activeController == 0 || activeController->attachNodePrimary == 0) {
        playerState->altGunTransitionState = 1;
        return;
    }

    if ((activeController->flags & kPlayerGunControllerDualMountFlag) != 0) {
        saveState->StartMasterTypeLoopSfxHandle(
            2,
            1.0f
        );
        DeactivateOppositeAltGunControllerNodes(
            playerState,
            activeController
        );
        if (activeController->attachNodePrimary != 0) {
            zClass_Class::gwNodeSetActive(
                activeController->attachNodePrimary,
                1
            );
        }
        if (activeController->attachNodeSecondary != 0) {
            zClass_Class::gwNodeSetActive(
                activeController->attachNodeSecondary,
                1
            );
        }
        playerState->altGunTransitionState = 1;
        return;
    }

    OptCatalogEntryDef *const entry = activeController->optCatalogEntry;
    if ((entry->flags & kOptCatalogFlagReload) != 0 && activeController->ammoOrCharge > 0.0f) {
        activeController->attachState = OptCatalog::AllocOrReuseAttachNodeClone(entry);
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)activeController->attachState;
        zClass_Class::AddChild(
            activeController->attachNodePrimary,
            attachState->projectileNode
        );
        attachState->ownerNode = playerState->rootNode;
    }

    zClass_Class::gwNodeSetActive(
        activeController->attachNodePrimary,
        1
    );
    zClass_Object3D::gwObject3DSetPosition(
        activeController->attachNodePrimary,
        activeController->attachPosX,
        activeController->attachPosY - 0.400000006f,
        activeController->attachPosZ
    );
    zClass_Object3D::gwObject3DSetScale(
        activeController->attachNodePrimary,
        1.0f,
        0.600000024f,
        0.600000024f
    );
    playerState->altGunTransitionState = 32;
    saveState->StartMasterTypeLoopSfxHandle(
        0,
        1.0f
    );
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 through
 * TickAltGunTransitionAnimation's state 32 path.
 * Evidence basis: door close scale animation is a local state-machine fragment
 * using the same timer field and door nodes as the open path.
 * Purpose: close the left/right door nodes and advance to the alt-gun raise
 * transition.
 */
void TickAltGunDoorClose(
    zUtil_PlayerStateStorage *playerState
) {
    playerState->altGunTransitionTimerB += g_FrameDeltaTimeSec;
    float xScale = 1.0f - playerState->altGunTransitionTimerB * 4.0f;
    if (xScale <= 0.00100000005f) {
        xScale = 0.00100000005f;
        playerState->altGunTransitionState = 64;
        playerState->altGunTransitionTimerB = 0.0f;
    }

    zClass_Object3D::gwObject3DSetScale(
        playerState->doorLeftNode,
        xScale,
        1.0f,
        1.0f
    );
    zClass_Object3D::gwObject3DSetScale(
        playerState->doorRightNode,
        xScale,
        1.0f,
        1.0f
    );
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 through
 * TickAltGunTransitionAnimation's state 64 path.
 * Evidence basis: recovered inline transition math updates the same attach node
 * position/scale and transition animation scale field as the lower path.
 * Purpose: raise the active single-mount alt gun back to its resting mount.
 */
void TickAltGunRaiseTransition(
    zUtil_PlayerStateStorage *playerState,
    PlayerGunFireController *controller
) {
    playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
    const float progress = playerState->altGunTransitionTimerA * 4.0f;
    const float animScale = progress * 0.400000006f;
    PlayerAltGunTransitionAnimScale(playerState) = animScale;
    const float y = animScale + controller->attachPosY - 0.400000006f;
    if (y >= controller->attachPosY) {
        PlayerAltGunTransitionAnimScale(playerState) = 0.400000006f;
        playerState->altGunTransitionState = 1;
        playerState->altGunTransitionTimerA = 0.0f;
        zClass_Object3D::gwObject3DSetPosition(
            controller->attachNodePrimary,
            controller->attachPosX,
            controller->attachPosY,
            controller->attachPosZ
        );
        zClass_Object3D::gwObject3DSetScale(
            controller->attachNodePrimary,
            1.0f,
            1.0f,
            1.0f
        );
        return;
    }

    zClass_Object3D::gwObject3DSetPosition(
        controller->attachNodePrimary,
        controller->attachPosX,
        y,
        controller->attachPosZ
    );
    const float scale = progress * 0.399999976f + 0.600000024f;
    zClass_Object3D::gwObject3DSetScale(
        controller->attachNodePrimary,
        1.0f,
        scale,
        scale
    );
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 when the alt-gun transition state
 * is not idle or projectile-camera tethered.
 * Evidence basis: the switch-lowered state dispatch is local to the recovered
 * runtime tick owner; 0x43a3a0 and 0x43a3bc are compiler switch-lowering
 * artifacts, not standalone authored helpers.
 * Purpose: dispatch the active alt-gun transition animation state.
 */
void TickAltGunTransitionAnimation(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const transitionController = playerState->altGunTransitionController;

    switch (playerState->altGunTransitionState) {
    case 2:
        TickAltGunRetractDelay(
            saveState,
            playerState
        );
        break;
    case 4:
        TickAltGunLowerTransition(
            playerState,
            transitionController
        );
        break;
    case 8:
        TickAltGunDoorOpen(playerState);
        break;
    case 16:
        TickAltGunActivateTransition(
            saveState,
            playerState
        );
        break;
    case 32:
        TickAltGunDoorClose(playerState);
        break;
    case 64:
        TickAltGunRaiseTransition(
            playerState,
            playerState->activeAltGunController
        );
        break;
    }
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 when transition bits 0x180 are
 * set.
 * Evidence basis: projectile tether cleanup and camera toggle handling are
 * typed player/controller state fragments in the owner tick, with no retail
 * helper callee.
 * Purpose: clean up or toggle the projectile-camera alt-gun tether state.
 */
void TickAltGunTetherCleanup(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    OptCatalogRuntimeInstanceStorage *const attachState =
        (OptCatalogRuntimeInstanceStorage *)activeController->attachState;

    if (attachState->ownerNode == 0) {
        if (playerState->cameraState == kPlayerTickCameraStateProjectileAttached) {
            HudUiMgr::EnableHud();
            Player::ApplyCameraState(kPlayerTickCameraStateRestorePrevious);
        }
        playerState->pendingAltCameraToggle = 0;
        OptCatalog::RecycleRuntimeInstanceStorage(
            activeController->optCatalogEntry,
            attachState
        );
        activeController->attachState = 0;
        playerState->altGunTransitionState = activeController->ammoOrCharge > 0.0f ? 4 : 1;
        return;
    }

    if (playerState->pendingAltCameraToggle != 0) {
        if (playerState->cameraState == kPlayerTickCameraStateProjectileAttached) {
            HudUiMgr::EnableHud();
            Player::ApplyCameraState(kPlayerTickCameraStateRestorePrevious);
            playerState->altGunTransitionState = 256;
        } else {
            HudUiMgr::DisableHud();
            Player::ApplyCameraState(kPlayerTickCameraStateProjectileAttached);
            playerState->altGunTransitionState = 128;
        }
        playerState->pendingAltCameraToggle = 0;
    }
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 after dispatch/tether handling.
 * Evidence basis: trigger-process cleanup, HUD message formatting, and runtime
 * instance removal are a local owner tick fragment using fixed player state
 * banks and source-file local OptCatalog/HUD calls.
 * Purpose: consume the alt-gun trigger process flag and report removed weapon
 * runtime instances.
 */
void TickAltGunTriggerProcessCleanup(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->altGunTriggerProcessFlag == 0) {
        return;
    }

    char message[0x50] = {0};
    int removedA = 0;
    int removedB = 0;
    OptCatalogEntryDef *const entryA = playerState->altWeaponBanks[5].controllerA.optCatalogEntry;
    if (entryA != 0) {
        removedA = OptCatalog::RemoveRuntimeInstance(
            entryA,
            0,
            playerState->rootNode
        );
        if (removedA != 0) {
            zLoc::FormatMessage(
                message,
                sizeof(message),
                0x248,
                removedA
            );
            HudUi::ShowTopMessageLine(
                message,
                5.0f
            );
        }
    }

    OptCatalogEntryDef *const entryB = playerState->altWeaponBanks[5].controllerB.optCatalogEntry;
    if (entryB != 0) {
        removedB = OptCatalog::RemoveRuntimeInstance(
            entryB,
            0,
            playerState->rootNode
        );
        if (removedB != 0) {
            zLoc::FormatMessage(
                message,
                sizeof(message),
                0x249,
                removedB
            );
            HudUi::ShowTopMessageLine(
                message,
                5.0f
            );
        }
    }

    if (removedA == 0 && removedB == 0) {
        OptCatalog::PlayWeaponInactiveWarning();
    }
    playerState->altGunTriggerProcessFlag = 0;
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 after the local-player gate.
 * Evidence basis: ammo drain, trail deactivation, HUD value update, and
 * auto-switch state changes are a single local ammo-state fragment, with no
 * standalone retail callee.
 * Purpose: update active alt-gun charge/ammo and decide whether the remaining
 * local tick work may continue.
 */
int TickAltGunLocalAmmoState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    OptCatalogEntryDef *const entry = activeController->optCatalogEntry;

    if (playerState->altGunFireHeldFlag != 0 &&
        activeController->ammoOrCharge != kPlayerAltAmmoDisabledSentinel) {
        activeController->ammoOrCharge -= g_FrameDeltaTimeSec / entry->fireRateInterval;
        if (activeController->ammoOrCharge < 0.0f) {
            activeController->ammoOrCharge = 0.0f;
        }
        activeController->trailRuntimeState->ammoOrChargeMirror = activeController->ammoOrCharge;
    }

    if (activeController->ammoOrCharge <= 0.0f) {
        if ((entry->flags & kOptCatalogFlagReload) != 0 &&
            playerState->altGunTransitionState != 1) {
            return 0;
        }

        activeController->ammoOrCharge = 0.0f;
        if (playerState->altGunFireHeldFlag != 0) {
            activeController->trailRuntimeState->ammoOrChargeMirror = 0.0f;
            playerState->altGunFireHeldFlag = 0;
            playerState->altGunDispatchRequested = 0;
            OptCatalog::DeactivateTrailRuntimeState(activeController->trailRuntimeState);
        }

        HudUiMessage::SetValueIfOwnerMatches(
            activeController->weaponBankIndex,
            activeController->weaponSideIndex,
            0.0f
        );
        Player::AutoSwitchToNextUsableAltWeapon(saveState);
        return 1;
    }

    if ((entry->flags & kOptCatalogFlagReload) != 0 && playerState->altGunTransitionState == 1 &&
        activeController->attachState == 0) {
        playerState->altGunTransitionState = 2;
    }
    return 1;
}
/**
 * Original-source helper evidence: no standalone retail function exists;
 * observed in address-backed caller 0x439ba0 after the local ammo-state update,
 * and it may call address-backed 0x43a400 for the primary-gun tick.
 * Evidence basis: alt-fire slot recoil decay and primary-gun tick dispatch are
 * the final local-player fragment of the recovered owner tick.
 * Purpose: update alt-fire slot offsets and run the primary-gun dispatch tick
 * when a primary controller is active.
 */
void TickAltGunLocalSlotAndPrimaryState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->gunNode != 0) {
        if (playerState->altFireSlotLeft.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotLeft,
                playerState->altFireSlotLeft.attachNode,
                playerState->gunFireDir.y,
                1
            );
        }
        if (playerState->altFireSlotRight.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotRight,
                playerState->altFireSlotRight.attachNode,
                playerState->gunFireDir.y,
                1
            );
        }
        if (playerState->altFireSlotCenter.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotCenter,
                playerState->altFireSlotCenter.attachNode,
                playerState->gunFireDir.y,
                0
            );
        }
    }

    if (playerState->activePrimaryGunController != 0) {
        Player::ProcessPrimaryGunDispatchTick(saveState);
    }
}

} // namespace

namespace Player {
zVec3 TransformPointByMatrix(
    const zVec3 &point,
    const zMat4x3 &matrix
);
static int IsUsableAltWeaponController(
    zUtil_SaveGameState *saveState,
    PlayerGunFireController *controller
);

/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-freealtweapontrailruntimestates
 * @recoil-artifact defines .text recoil:function:0x438b60: Player::FreeAltWeaponTrailRuntimeStates
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: release existing trail runtime state storage before rebuilding
 * alternate weapon banks.
 */
void __fastcall FreeAltWeaponTrailRuntimeStates(
    zUtil_SaveGameState *saveState
) {
    PlayerAltWeaponBank *bank = &saveState->playerState->altWeaponBanks[1];
    for (int i = 0; i < 9; ++i, ++bank) {
        OptCatalogTrailRuntimeState *const controllerATrail = bank->controllerA.trailRuntimeState;
        if (controllerATrail != 0) {
            OptCatalog::FreeTrailRuntimeStateStorage(controllerATrail);
        }

        OptCatalogTrailRuntimeState *const controllerBTrail = bank->controllerB.trailRuntimeState;
        if (controllerBTrail != 0) {
            OptCatalog::FreeTrailRuntimeStateStorage(controllerBTrail);
        }
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-loadweaponbanksandselectdefaults
 * @recoil-artifact defines .text recoil:function:0x438ba0: Player::LoadWeaponBanksAndSelectDefaults
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: rebuild weapon-bank controller state from master weapon specs,
 * bind weapon mount nodes/trails, select default controllers, and refresh
 * cached selection/timed-hit state.
 */
void __fastcall LoadWeaponBanksAndSelectDefaults(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    FreeAltWeaponTrailRuntimeStates(saveState);

    const float resetAmmoOrCharge = playerState->lifecycleState == kPlayerLifecycleRemote
                                        ? kPlayerAltAmmoDisabledSentinel
                                        : 0.0f;
    for (int bankIndex = 0; bankIndex < 10; ++bankIndex) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
        bank.selectedSide = 0;
        for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
            PlayerGunFireController *const controller = sideIndex == 0
                                                        ? &bank.controllerA
                                                        : &bank.controllerB;
            controller->weaponBankIndex = bankIndex;
            controller->weaponSideIndex = sideIndex;
            controller->flags &= ~kPlayerGunControllerAvailableFlag;
            controller->ammoOrCharge = resetAmmoOrCharge;
            controller->attachNodePrimary = 0;
            controller->trailRuntimeState = 0;
        }
    }

    int trailSegmentCount = 1;
    if (playerState->playerOrdinal == 1 || strstr(
        playerState->rootNode->name,
        "net"
    ) != 0) {
        trailSegmentCount = 8;
    }

    if (masterCommonData->weaponNodeCount > 0 && masterCommonData->weaponSpecHead != 0) {
        int altDefaultSelected = 0;
        int primaryDefaultSelected = 0;
        PlayerMasterWeaponSpec *weaponSpec = masterCommonData->weaponSpecHead;
        while (weaponSpec != 0) {
            char optCatalogName[0x50];
            strcpy(
                optCatalogName,
                weaponSpec->optCatalogName
            );

            const int bankIndex = optCatalogName[4] - '0';
            const int sideIndex = optCatalogName[6] - '0';
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            PlayerGunFireController *const controller =
                sideIndex == 0 ? &bank.controllerA : &bank.controllerB;

            controller->optCatalogEntry = OptCatalog::FindEntryByName(optCatalogName);

            int available = 0;
            const int packedWeaponSlotId = (bankIndex << 4) | sideIndex;
            CheckMissionWeaponAvailability(
                saveState,
                weaponSpec->missionRequirementOrGateId,
                packedWeaponSlotId,
                &available
            );

            controller->flags = (controller->flags & ~kPlayerGunControllerDualMountFlag) |
                                ((weaponSpec->mountLayoutFlags & 1) << 1);
            if (available != 0) {
                if (controller->optCatalogEntry != 0) {
                    controller->flags |= kPlayerGunControllerAvailableFlag;
                } else {
                    controller->flags &= ~kPlayerGunControllerAvailableFlag;
                }
            }
            controller->ammoOrCharge = (controller->flags & kPlayerGunControllerAvailableFlag) != 0
                                           ? weaponSpec->startAmmoOrCharge
                                           : 0.0f;
            controller->nextDispatchTime = 0.0f;
            controller->dispatchRepeatDelay = weaponSpec->dispatchRepeatDelay;
            controller->aiAttackRangeMin = weaponSpec->aiAttackRangeMin;
            controller->aiAttackRangeMax = weaponSpec->aiAttackRangeMax;
            controller->flags = (controller->flags & ~kPlayerGunControllerRecoilFlag) |
                                (weaponSpec->fireSlotRecoilFlags & kPlayerGunControllerRecoilFlag);
            controller->initialHardpointSelectState = weaponSpec->initialHardpointSelectState;

            if (playerState->gunNode != 0) {
                if ((controller->flags & kPlayerGunControllerDualMountFlag) != 0) {
                    char mountName[0x50];
                    char scrollName[0x50];
                    sprintf(
                        mountName,
                        "%s_L",
                        controller->optCatalogEntry->displayName
                    );
                    controller->attachNodePrimary =
                        zClass_Class::FindNodeRecursiveByName(
                            playerState->gunNode,
                            mountName
                        );
                    if (controller->attachNodePrimary != 0) {
                        zClass_Class::gwNodeSetActive(
                            controller->attachNodePrimary,
                            0
                        );
                    }
                    sprintf(
                        scrollName,
                        "%sSCROLL",
                        mountName
                    );
                    zClass_NodePartial *scrollNode =
                        zClass_Class::FindNodeRecursiveByName(
                            controller->attachNodePrimary,
                            scrollName
                        );
                    controller->scrollTextureModelA = 0;
                    if (scrollNode != 0) {
                        unsigned int userData = 0;
                        zClass_Class::gwNodeGetUserData(
                            scrollNode,
                            &userData
                        );
                        controller->scrollTextureModelA = (zDiPartial *)userData;
                        zModel::SetDiTextureWorldPerMeter(
                            controller->scrollTextureModelA,
                            1,
                            0.0f,
                            2
                        );
                    }

                    sprintf(
                        mountName,
                        "%s_R",
                        controller->optCatalogEntry->displayName
                    );
                    controller->attachNodeSecondary =
                        zClass_Class::FindNodeRecursiveByName(
                            playerState->gunNode,
                            mountName
                        );
                    if (controller->attachNodeSecondary != 0) {
                        zClass_Class::gwNodeSetActive(
                            controller->attachNodeSecondary,
                            0
                        );
                    }
                    sprintf(
                        scrollName,
                        "%sSCROLL",
                        mountName
                    );
                    scrollNode = zClass_Class::FindNodeRecursiveByName(
                        controller->attachNodeSecondary,
                        scrollName
                    );
                    controller->scrollTextureModelB = 0;
                    if (scrollNode != 0) {
                        unsigned int userData = 0;
                        zClass_Class::gwNodeGetUserData(
                            scrollNode,
                            &userData
                        );
                        controller->scrollTextureModelB = (zDiPartial *)userData;
                        zModel::SetDiTextureWorldPerMeter(
                            controller->scrollTextureModelB,
                            1,
                            0.0f,
                            2
                        );
                    }
                } else {
                    controller->attachNodePrimary =
                        zClass_Class::FindNodeRecursiveByName(
                            playerState->gunNode,
                            controller->optCatalogEntry->displayName
                        );
                    if (controller->attachNodePrimary != 0) {
                        zClass_Class::gwNodeSetActive(
                            controller->attachNodePrimary,
                            0
                        );
                    }
                    zClass_Object3D::gwObject3DGetPosition(
                        controller->attachNodePrimary,
                        &controller->attachPosX,
                        &controller->attachPosY,
                        &controller->attachPosZ
                    );
                }
            }

            if ((controller->optCatalogEntry->flags & kOptCatalogFlagCreateTrail) != 0) {
                controller->trailRuntimeState = OptCatalog::CreateTrailRuntimeState(
                    controller->optCatalogEntry,
                    playerState->rootNode,
                    &playerState->variantTag,
                    controller->attachNodePrimary,
                    &playerState->altFireOrigin,
                    &playerState->gunFireDir,
                    trailSegmentCount
                );
            }

            if (zOpt::GetNetworkEnabled() == 0 && available != 0) {
                if (altDefaultSelected == 0) {
                    ApplyAltWeaponSwitch(
                        saveState,
                        0,
                        controller
                    );
                    altDefaultSelected = 1;
                } else if (primaryDefaultSelected == 0) {
                    ApplyPrimaryWeaponSwitch(
                        saveState,
                        0,
                        controller
                    );
                    primaryDefaultSelected = 1;
                }
            }

            weaponSpec = weaponSpec->next;
        }
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        int selected = 0;
        for (int bankIndex = 2; selected == 0 && bankIndex < 10; ++bankIndex) {
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            if ((bank.controllerA.flags & kPlayerGunControllerAvailableFlag) != 0) {
                ApplyAltWeaponSwitch(
                    saveState,
                    0,
                    &bank.controllerA
                );
                selected = 1;
            } else if ((bank.controllerB.flags & kPlayerGunControllerAvailableFlag) != 0) {
                ApplyAltWeaponSwitch(
                    saveState,
                    0,
                    &bank.controllerB
                );
                selected = 1;
            }
        }
    }

    if (playerState->activeAltGunController == 0) {
        ApplyAltWeaponSwitch(
            saveState,
            0,
            &playerState->altWeaponBanks[1].controllerA
        );
    }

    PlayerGunFireController *primaryController = &playerState->altWeaponBanks[1].controllerA;
    if ((playerState->altWeaponBanks[1].controllerB.flags & kPlayerGunControllerAvailableFlag) !=
        0) {
        primaryController = &playerState->altWeaponBanks[1].controllerB;
    }
    ApplyPrimaryWeaponSwitch(
        saveState,
        0,
        primaryController
    );

    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;
    if (activeAltGunController->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(
            activeAltGunController->attachNodePrimary,
            1
        );
    }

    playerState->altHardpointSelectState =
        activeAltGunController->initialHardpointSelectState == 2 ? 2 : 0;
    playerState->cachedAltSelectionCode =
        activeAltGunController->weaponBankIndex * 100 + activeAltGunController->weaponSideIndex;

    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;
    if (activePrimaryGunController != 0) {
        playerState->cachedPrimarySelectionCode =
            activePrimaryGunController->weaponBankIndex * 100 +
            activePrimaryGunController->weaponSideIndex;
    }

    playerState->pendingAltCameraToggle = 0;
    playerState->timedHitStatus.lightParentNode = playerState->rootNode;
    playerState->timedHitStatus.ResetFields();

    zUtil_ZAR::RegisterSectionHandler(
        "Mines",
        (zZbdSectionCallback)(&WriteMinesZarSection),
        (zZbdSectionCallback)(&Mines_ZAR_ReadEntryOrReset),
        1000,
        0
    );
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-cachegunhardpointsanddetachdisplays
 * @recoil-artifact defines .text recoil:function:0x4390d0: Player::CacheGunHardpointsAndDetachDisplays
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: cache the gun node and its fpnt_c/fpnt_l/fpnt_r hardpoint
 * positions, detaching display instances during bootstrap when requested.
 */
void __fastcall CacheGunHardpointsAndDetachDisplays(
    zUtil_SaveGameState *saveState,
    int detachDisplays
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    playerState->gunNode = zClass_Class::FindSubNodeByName(
        playerState->rootNode,
        "gun"
    );
    if (playerState->gunNode != 0) {
        float *const gunMatrix = zClass_Object3D::gwObject3DGetMatrixPtr(playerState->gunNode);
        playerState->gunNodeMatrixPos.x = gunMatrix[9];
        playerState->gunNodeMatrixPos.y = gunMatrix[10];
        playerState->gunNodeMatrixPos.z = gunMatrix[11];
    }

    if (playerState->gunNode == 0) {
        return;
    }

    zClass_NodePartial *hardpointNode = zClass_Class::FindNodeRecursiveByName(
        playerState->gunNode,
        "fpnt_c"
    );
    if (hardpointNode != 0) {
        zClass_Object3D::gwObject3DGetPosition(
            hardpointNode,
            &playerState->firePointCenter.x,
            &playerState->firePointCenter.y,
            &playerState->firePointCenter.z
        );
        if (detachDisplays != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(hardpointNode, &displayInstanceValue);
            zClass_Class::gwNodeSetDisplayInstance(hardpointNode, 0);
            zDiPartial *const displayInstance = (zDiPartial *)displayInstanceValue;
            if (displayInstance != 0 && displayInstance->refCount != 0) {
                zDi::Release(displayInstance);
                zModel_DiPool::FreeIfUnreferenced(displayInstance);
            }
        }
    }

    hardpointNode = zClass_Class::FindNodeRecursiveByName(
        playerState->gunNode,
        "fpnt_l"
    );
    if (hardpointNode != 0) {
        zClass_Object3D::gwObject3DGetPosition(
            hardpointNode,
            &playerState->firePointLeft.x,
            &playerState->firePointLeft.y,
            &playerState->firePointLeft.z
        );
        if (detachDisplays != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(hardpointNode, &displayInstanceValue);
            zClass_Class::gwNodeSetDisplayInstance(hardpointNode, 0);
            zDiPartial *const displayInstance = (zDiPartial *)displayInstanceValue;
            if (displayInstance != 0 && displayInstance->refCount != 0) {
                zDi::Release(displayInstance);
                zModel_DiPool::FreeIfUnreferenced(displayInstance);
            }
        }
    }

    hardpointNode = zClass_Class::FindNodeRecursiveByName(
        playerState->gunNode,
        "fpnt_r"
    );
    if (hardpointNode != 0) {
        zClass_Object3D::gwObject3DGetPosition(
            hardpointNode,
            &playerState->firePointRight.x,
            &playerState->firePointRight.y,
            &playerState->firePointRight.z
        );
        if (detachDisplays != 0) {
            unsigned int displayInstanceValue = 0;
            zClass_Class::gwNodeGetUserData(hardpointNode, &displayInstanceValue);
            zClass_Class::gwNodeSetDisplayInstance(hardpointNode, 0);
            zDiPartial *const displayInstance = (zDiPartial *)displayInstanceValue;
            if (displayInstance != 0 && displayInstance->refCount != 0) {
                zDi::Release(displayInstance);
                zModel_DiPool::FreeIfUnreferenced(displayInstance);
            }
        }
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-handlealtweaponbankselectinput
 * @recoil-artifact defines .text recoil:function:0x439260: Player::HandleAltWeaponBankSelectInput.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\zWeapon.cpp.
 * Purpose: reimplement Player::HandleAltWeaponBankSelectInput from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall HandleAltWeaponBankSelectInput(
    int inputCode
) {
    zUtil_SaveGameState *const saveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    PlayerGunFireController *const previousController = playerState->activeAltGunController;

    if (playerState->altGunTransitionState != 1) {
        return;
    }

    int bankIndex = inputCode - 14;
    if (inputCode < 14 || inputCode > 23) {
        bankIndex = playerState->activeAltBankIndex;
    }

    PlayerAltWeaponBank *const bank = &playerState->altWeaponBanks[bankIndex];
    PlayerGunFireController *newController = 0;
    PlayerGunFireController *failedController = 0;
    int switchAccepted = 0;

    if (bankIndex == playerState->activeAltBankIndex) {
        if (bank->selectedSide == 0) {
            newController = &bank->controllerB;
        } else {
            newController = &bank->controllerA;
        }
        failedController = newController;

        if (newController->optCatalogEntry != 0 && (newController->flags & 4) != 0 &&
            newController->ammoOrCharge != 0.0f) {
            switchAccepted = 1;
        }
    } else {
        const int selectedSide = bank->selectedSide;
        newController = selectedSide == 0 ? &bank->controllerA : &bank->controllerB;
        failedController = newController;

        if (newController->optCatalogEntry != 0 && (newController->flags & 4) != 0 &&
            newController->ammoOrCharge != 0.0f) {
            switchAccepted = 1;
        } else if (newController->optCatalogEntry != 0) {
            bank->selectedSide = selectedSide == 0 ? 1 : 0;
        }
    }

    if (switchAccepted != 0) {
        if (masterModalData->masterType == 2 &&
            (newController->optCatalogEntry->flags & 0x1000) != 0) {
            char message[64];
            OptCatalog::PlayNoAmmoWarning();
            zLoc::FormatMessage(
                message,
                sizeof(message),
                0x24b,
                newController->optCatalogEntry->description
            );
            HudUi::ShowTopMessageLine(
                message,
                5.0f
            );
            return;
        }

        HudUi::ShowTopMessageLine(
            newController->optCatalogEntry->description,
            5.0f
        );
        HudUiMessage::UpdateSelectedWeaponDisplay(
            newController->weaponBankIndex,
            newController->weaponSideIndex,
            newController->ammoOrCharge
        );
        ApplyAltWeaponSwitch(
            saveState,
            previousController,
            newController
        );
    } else if (failedController->optCatalogEntry != 0) {
        if ((failedController->flags & 4) == 0) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x916),
                5.0f
            );
        } else if (failedController->ammoOrCharge == 0.0f) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x917),
                5.0f
            );
        }
        HudUi::ShowTopMessageLine(
            failedController->optCatalogEntry->description,
            5.0f
        );
    }

    zUtil_PlayerStateStorage *const displayPlayerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    PlayerGunFireController *const activeController = displayPlayerState->activeAltGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activeController->weaponBankIndex,
        activeController->weaponSideIndex,
        activeController->ammoOrCharge
    );
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-handleprimaryweaponvarianttoggleinput
 * @recoil-artifact defines .text recoil:function:0x439460: Player::HandlePrimaryWeaponVariantToggleInput.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::HandlePrimaryWeaponVariantToggleInput from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall HandlePrimaryWeaponVariantToggleInput(
    int keyCode
) {
    (void)keyCode;

    zUtil_SaveGameState *const saveState = g_LocalPlayerSaveState;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const previousController = playerState->activePrimaryGunController;
    PlayerGunFireController *newController = 0;

    if (previousController->weaponSideIndex == 0) {
        newController = &playerState->altWeaponBanks[1].controllerB;

        if ((newController->flags & 4) == 0) {
            HudUi::ShowTopMessageLine(
                newController->optCatalogEntry->description,
                5.0f
            );
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x916),
                5.0f
            );
            return;
        }

        if (newController->ammoOrCharge <= 0.0f) {
            HudUi::ShowTopMessageLine(
                newController->optCatalogEntry->description,
                5.0f
            );
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x917),
                5.0f
            );
            return;
        }
    } else {
        newController = &playerState->altWeaponBanks[1].controllerA;
    }

    saveState->StartMasterTypeLoopSfxHandle(
        2,
        1.0f
    );
    ApplyPrimaryWeaponSwitch(
        saveState,
        previousController,
        newController
    );
    HudUi::ShowTopMessageLine(
        newController->optCatalogEntry->description,
        5.0f
    );

    zUtil_PlayerStateStorage *const displayPlayerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    PlayerGunFireController *const activeController =
        displayPlayerState->activePrimaryGunController;
    HudUiMessage::UpdateSelectedWeaponDisplay(
        activeController->weaponBankIndex,
        activeController->weaponSideIndex,
        activeController->ammoOrCharge
    );
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-applyaltweaponswitch
 * @recoil-artifact defines .text recoil:function:0x439540: Player::ApplyAltWeaponSwitch
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: install the selected alternate weapon controller, start the
 * transition state, stop any held trail fire, and cache the bank/side code.
 */
void __fastcall ApplyAltWeaponSwitch(
    zUtil_SaveGameState *saveState,
    PlayerGunFireController *previousController,
    PlayerGunFireController *newController
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->activeAltGunController = newController;

    const int weaponBankIndex = newController->weaponBankIndex;
    playerState->activeAltBankIndex = weaponBankIndex;
    playerState->altWeaponBanks[weaponBankIndex].selectedSide = newController->weaponSideIndex;
    playerState->altHardpointSelectState = 0;
    playerState->altGunTransitionTimerA = 0.0f;
    playerState->altGunTransitionTimerB = 0.0f;

    if (previousController != 0) {
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            saveState->StartMasterTypeLoopSfxHandle(
                0,
                1.0f
            );
        }

        playerState->altGunTransitionState = 4;
        playerState->altGunTransitionController = previousController;
    } else {
        playerState->altGunTransitionState = 16;
        playerState->altGunTransitionController = newController;
    }

    if (playerState->altGunFireHeldFlag != 0) {
        playerState->altGunFireHeldFlag = 0;
        OptCatalog::DeactivateTrailRuntimeState(previousController->trailRuntimeState);
    }

    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    playerState->cachedAltSelectionCode =
        activeController->weaponSideIndex + activeController->weaponBankIndex * 100;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-applyprimaryweaponswitch
 * @recoil-artifact defines .text recoil:function:0x439600: Player::ApplyPrimaryWeaponSwitch
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: activate the selected primary weapon controller, toggle previous
 * and new mount nodes, and cache the selected bank/side display code.
 */
void __fastcall ApplyPrimaryWeaponSwitch(
    zUtil_SaveGameState *saveState,
    PlayerGunFireController *previousController,
    PlayerGunFireController *newController
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->activePrimaryGunController = newController;
    playerState->primaryHardpointSelectState = 2;

    if (previousController != 0 && previousController->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(
            previousController->attachNodePrimary,
            0
        );
    }
    if (previousController != 0 && previousController->attachNodeSecondary != 0) {
        zClass_Class::gwNodeSetActive(
            previousController->attachNodeSecondary,
            0
        );
    }
    if (newController != 0 && newController->attachNodePrimary != 0) {
        zClass_Class::gwNodeSetActive(
            newController->attachNodePrimary,
            1
        );
    }
    if (newController != 0 && newController->attachNodeSecondary != 0) {
        zClass_Class::gwNodeSetActive(
            newController->attachNodeSecondary,
            1
        );
    }

    PlayerGunFireController *const activeController = playerState->activePrimaryGunController;
    playerState->cachedPrimarySelectionCode =
        activeController->weaponSideIndex + activeController->weaponBankIndex * 100;
}
} // namespace Player

namespace HudUiMgrSensor {
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-huduimgrsensor-updatemarkersandprogressfromvarianttag
 * @recoil-artifact defines .text recoil:function:0x439690: HudUiMgrSensor::UpdateMarkersAndProgressFromVariantTag.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMgrSensor.cpp.
 * BN/source evidence ties this to the sensor-target runtime owner: the track
 * list stores discriminated player/turret payloads, candidate filtering uses
 * variant tags and scene-path projection visibility, and marker creation feeds
 * the typed HudUiSlot placement/update path rather than raw HUD offsets.
 * Purpose: refresh candidate sensor targets for the requested variant tag,
 * place visible markers, and update the selected target progress slots.
 */
void __fastcall UpdateMarkersAndProgressFromVariantTag(
    const zTag4Partial *requiredVariantTag
) {
    HudUiMgrSensorTrackNode *trackNode = g_HudUiMgrSensor_TrackList.head;
    zUtil_PlayerStateStorage *const localPlayerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    HudUiMgrSensorTrackNode *candidateTrackNodes[0x64];
    int candidateCount = 0;
    while (trackNode != 0) {
        if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
            zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(trackNode->payload);
            zUtil_PlayerStateStorage *const playerState = saveState->playerState;

            if (playerState->recentHitFlag != 0 &&
                !(g_Time_AccumulatedTimeSec < playerState->recentHitExpireTime)) {
                playerState->recentHitFlag = 0;
            }

            if (playerState->lifecycleState != 1 && playerState->lifecycleState != 4 &&
                VariantTag::TagsOverlap(
                    &playerState->variantTag,
                    requiredVariantTag
                ) != 0) {
                const float distXZ =
                    fabs(playerState->fxOffsetWorld.x - localPlayerState->worldPos.x) +
                    fabs(playerState->fxOffsetWorld.z - localPlayerState->worldPos.z);
                if (distXZ < 650.0f && candidateCount < 0x63) {
                    candidateTrackNodes[candidateCount++] = trackNode;
                }
            }
        } else {
            trackNode->trackKind = HUD_SENSOR_TRACK_KIND_TURRET;
            zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
            if (turretRuntime->HasActiveNode() != 0 &&
                VariantTag::CurrentAllowsId(turretRuntime->turretNode->nodeType) != 0) {
                const float distXZ = fabs(turretRuntime->firePos.z - localPlayerState->worldPos.z) +
                                     fabs(turretRuntime->firePos.x - localPlayerState->worldPos.x);
                if (distXZ < 650.0f && candidateCount < 0x63) {
                    candidateTrackNodes[candidateCount++] = trackNode;
                }
            }
        }

        trackNode = trackNode->next;
    }

    if (candidateCount != 0) {
        int selectedIndex = g_HudUiMgrSensor_RoundRobinTrackIndex + 1;
        g_HudUiMgrSensor_RoundRobinTrackIndex = selectedIndex;
        if (selectedIndex >= candidateCount) {
            selectedIndex = 0;
            g_HudUiMgrSensor_RoundRobinTrackIndex = 0;
        }

        HudUiMgrSensorTrackNode *const selectedTrackNode = candidateTrackNodes[selectedIndex];
        if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
            zUtil_SaveGameState *const saveState =
                (zUtil_SaveGameState *)(selectedTrackNode->payload);
            zUtil_PlayerStateStorage *const playerState = saveState->playerState;
            zVec3 point = playerState->fxOffsetWorld;
            point.y += 3.0f;

            const int visible =
                AINet::HasLineOfSightFromCameraTarget(
                    playerState->rootNode,
                    &point,
                    1
                );
            playerState->spawnStateInitialized = visible;
            if (visible != 0 && playerState->recentHitMarkerHandle != 0) {
                playerState->recentHitFlag = 1;
                playerState->recentHitExpireTime = g_Time_AccumulatedTimeSec + 3.0f;
            }
        } else if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
            zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(selectedTrackNode->payload);
            turretRuntime->scenePathVisible = AINet::HasLineOfSightFromCameraTarget(
                turretRuntime->turretNode,
                &turretRuntime->firePos,
                2
            );
        }

        {
            for (int index = 0; index < candidateCount; ++index) {
                HudUiMgrSensorTrackNode *const candidate = candidateTrackNodes[index];
                if (candidate->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
                    zUtil_SaveGameState *const saveState =
                        (zUtil_SaveGameState *)(candidate->payload);
                    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
                    if ((playerState->spawnStateInitialized & 1) != 0) {
                        playerState->recentHitMarkerHandle =
                            HudUiMgrSensor::PlaceTrackCounterWidget(
                                candidate,
                                &playerState->fxOffsetWorld
                            );
                    }
                } else if (candidate->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
                    zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(candidate->payload);
                    if ((turretRuntime->scenePathVisible & 1) != 0) {
                        HudUiMgrSensor::PlaceTrackCounterWidget(
                            candidate,
                            &turretRuntime->firePos
                        );
                    }
                }
            }
        }
    }

    PlayerGunFireController *const activeAltGunController =
        localPlayerState->activeAltGunController;
    const unsigned int optEntryFlags = activeAltGunController->optCatalogEntry->flags;
    if (((optEntryFlags >> 20) & 1u) != 0) {
        HudUiMgr::CopyReticleProjection(&localPlayerState->autoTurnTargetWorldPos.x);
        localPlayerState->progressTargetCount = 1;
        localPlayerState->progressTargetSlots[0].targetPos =
            &localPlayerState->autoTurnTargetWorldPos;
        localPlayerState->progressTargetSlots[0].targetVelocity = 0;
        HudUiMgrTarget::UpdateSelectedProgressMeter(0);
        return;
    }

    if (activeAltGunController->ammoOrCharge != 0.0f) {
        int markerMode = 0;
        if (((optEntryFlags >> 16) & 1u) != 0) {
            markerMode = 2;
        } else if ((optEntryFlags & 0x4000u) != 0) {
            markerMode = 1;
        }

        localPlayerState->progressTargetCount =
            HudUiMgrSensor::PlaceTrackMarker(
                markerMode,
                localPlayerState->progressTargetSlots
            );
    }

    HudUiMgrTarget::UpdateSelectedProgressMeter(0);
}
} // namespace HudUiMgrSensor

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-resetdamagestateandtimedhitstatus
 * @recoil-artifact defines .text recoil:function:0x439990: Player::ResetDamageStateAndTimedHitStatus
 *
 * Purpose: reload damage material state, clear damage flags, and clear any
 * attached timed-hit status light.
 */
void __fastcall ResetDamageStateAndTimedHitStatus(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_Node::LoadFlagBit8MaterialImagesAndTexturePack(playerState->rootNode);
    playerState->queuedFixedDamageFlag = 0;
    playerState->damageProtectionActive = 0;
    playerState->damageVisualFlag = 0;
    playerState->timedHitStatus.ClearLightAndReset();
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-resetdamagevisualsandtimedstatus
 * @recoil-artifact defines .text recoil:function:0x4399c0: Player::ResetDamageVisualsAndTimedStatus
 * Purpose: Clears damage flash state and timed hit status before damage processing.
 */
void __fastcall ResetDamageVisualsAndTimedStatus(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    if (playerState->statusMeterValue <= 0.0f) {
        if (playerState->airborneFlag == 0) {
            EnterLocalInactiveDestroyedLifecycle(saveState);
        }
        return;
    }

    if ((playerState->timedHitStatus.runtimeFlags & kPlayerTimedHitStatusActiveFlag) != 0) {
        const int timedResult =
            playerState->timedHitStatus.TickAndUpdateLight(playerState->rootNode->cachedBounds[0]);
        playerState->damageProtectionActive = timedResult == 2;
    }

    if (playerState->recentHitValid != 0) {
        if (g_Time_AccumulatedTimeSec < playerState->recentHitFxExpireTime) {
            if (playerState->lifecycleState != kPlayerLifecycleRemote) {
                const float damage = playerState->recentHitDamage * g_FrameDeltaTimeSec;
                if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
                    EnterDestroyedState(
                        saveState,
                        0,
                        0,
                        damage
                    );
                } else {
                    HitCallback_RecordContextAndTimedStatus(
                        saveState,
                        0,
                        0,
                        damage
                    );
                }
            }
        } else {
            zEffectAnim::Stop(playerState->recentHitLightHandle);
            playerState->recentHitLightHandle = 0;
            playerState->recentHitValid = 0;
        }
    }

    if (playerState->queuedFixedDamageFlag != 0) {
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            EnterDestroyedState(
                saveState,
                0,
                0,
                masterCommonData->maxHealth
            );
        } else {
            HitCallback_RecordContextAndTimedStatus(
                saveState,
                0,
                0,
                masterCommonData->maxHealth
            );
        }
        playerState->queuedFixedDamageFlag = 0;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        g_PlayerStatusMeterRatio < 0.25f) {
        HudLowMeterLoopSound::SetLoopActive(0);
        if (g_Time_AccumulatedTimeSec > g_Hud_LowMeterNextBeepTime) {
            g_Hud_LowMeterBeepSample->PlayA3DSimple(1.0f);
            g_Hud_LowMeterNextBeepTime = g_Hud_LowMeterBeepInterval + g_Time_AccumulatedTimeSec;
        }
    }
}
} // namespace Player

/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-g-hud-lowmeterbeepsample
 * @recoil-artifact defines .data recoil:data:0x4f3748: g_Hud_LowMeterBeepSample.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Holds the one-shot low-meter warning sample loaded from player.zrd.
 */
zSndSample *g_Hud_LowMeterBeepSample = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-g-hud-lowmeterloopsample
 * @recoil-artifact defines .data recoil:data:0x4f374c: g_Hud_LowMeterLoopSample.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Holds the looped low-meter warning sample loaded from player.zrd.
 */
zSndSample *g_Hud_LowMeterLoopSample = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-g-hud-lowmeterloopactive
 * @recoil-artifact defines .data recoil:data:0x4f3750: g_Hud_LowMeterLoopActive.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Tracks whether the low-meter loop sample has been started.
 */
int g_Hud_LowMeterLoopActive = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-g-hud-lowmeterbeepinterval
 * @recoil-artifact defines .data recoil:data:0x4f3758: g_Hud_LowMeterBeepInterval.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Stores the low-meter one-shot beep interval from player.zrd.
 */
float g_Hud_LowMeterBeepInterval = 0.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-g-hud-lowmeternextbeeptime
 * @recoil-artifact defines .data recoil:data:0x4f375c: g_Hud_LowMeterNextBeepTime.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Stores the next absolute mission time for a low-meter one-shot beep.
 */
float g_Hud_LowMeterNextBeepTime = 0.0f;

namespace HudLowMeterLoopSound {

/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-hudlowmeterloopsound-setloopactive
 * @recoil-artifact defines .text recoil:function:0x439b20: HudLowMeterLoopSound::SetLoopActive.
 * Original source filename remains unresolved in the mixed later Player/combat shelf.
 * Purpose: Starts or stops the low-meter loop sample on active-state changes.
 */
void __fastcall SetLoopActive(
    int enabled
) {
    const int wasActive = g_Hud_LowMeterLoopActive;
    if (enabled != 0) {
        if (wasActive == 0) {
            g_Hud_LowMeterLoopSample->PlayA3DSimple(1.0f);
            g_Hud_LowMeterLoopActive = 1;
        }
        return;
    }

    if (wasActive != 0) {
        g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
        g_Hud_LowMeterLoopActive = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-hudlowmeterloopsound-disable
 * @recoil-artifact defines .text recoil:function:0x439b70: HudLowMeterLoopSound::Disable.
 * Original source filename remains unresolved in the mixed later Player/combat shelf.
 * Purpose: Stops both low-meter warning samples and clears the loop-active flag.
 */
void __cdecl Disable() {
    g_Hud_LowMeterBeepSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopActive = 0;
}

} // namespace HudLowMeterLoopSound

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-tickaltgunruntimestate
 * @recoil-artifact defines .text recoil:function:0x439ba0: Player::TickAltGunRuntimeState.
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Source model: Player source-file runtime tick owner for active alt-gun state;
 * transition fragments are recovered as original-source helpers with no
 * standalone retail functions where noted above.
 * Touched data: updates typed zUtil_PlayerStateStorage and
 * PlayerGunFireController runtime state, installs OptCatalog pending spawn
 * target overrides, reads g_GameStateOrMapTable for local-player-only work,
 * uses accepted g_FrameDeltaTimeSec through transition/ammo helpers, and may
 * reach g_HudSensorTracker through 0x43a400. BN .rdata literals
 * 0x4d17a8..0x4d17c0 match the source literals in this owner; 0x43a3a0 and
 * 0x43a3bc are compiler switch-lowering artifacts.
 * Purpose: tick active alt-gun dispatch, transition, tether, ammo, slot recoil,
 * and linked primary-gun runtime state for the current save game.
 */
void __fastcall TickAltGunRuntimeState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;

    OptCatalog::SetPendingSpawnTargetOverrides(
        &playerState->progressTargetCount,
        playerState->progressTargetSlots
    );

    if (playerState->altGunTransitionState == 1 ||
        (playerState->altGunTransitionState & 0x180) != 0) {
        if (playerState->altGunDispatchRequested != 0) {
            ProcessAltGunDispatchRequest(saveState);
        } else if (playerState->altGunFireHeldFlag != 0) {
            playerState->altGunFireHeldFlag = 0;
            OptCatalog::DeactivateTrailRuntimeState(activeAltGunController->trailRuntimeState);
        }

        if ((playerState->altGunTransitionState & 0x180) != 0) {
            OptCatalogRuntimeInstanceStorage *const attachState =
                (OptCatalogRuntimeInstanceStorage *)activeAltGunController->attachState;

            if (attachState->ownerNode == 0) {
                if (playerState->cameraState == kPlayerTickCameraStateProjectileAttached) {
                    HudUiMgr::EnableHud();
                    Player::ApplyCameraState(kPlayerTickCameraStateRestorePrevious);
                }
                playerState->pendingAltCameraToggle = 0;
                OptCatalog::RecycleRuntimeInstanceStorage(
                    activeAltGunController->optCatalogEntry,
                    attachState
                );
                activeAltGunController->attachState = 0;
                playerState->altGunTransitionState =
                    activeAltGunController->ammoOrCharge > 0.0f ? 4 : 1;
            } else if (playerState->pendingAltCameraToggle != 0) {
                if (playerState->cameraState != kPlayerTickCameraStateProjectileAttached) {
                    HudUiMgr::DisableHud();
                    Player::ApplyCameraState(kPlayerTickCameraStateProjectileAttached);
                    playerState->altGunTransitionState = 128;
                } else {
                    HudUiMgr::EnableHud();
                    Player::ApplyCameraState(kPlayerTickCameraStateRestorePrevious);
                    playerState->altGunTransitionState = 256;
                }
                playerState->pendingAltCameraToggle = 0;
            }
        }

        if (playerState->altGunTriggerProcessFlag != 0) {
            char message[0x50] = {0};
            int removedA = 0;
            int removedB = 0;
            OptCatalogEntryDef *const entryA =
                playerState->altWeaponBanks[5].controllerA.optCatalogEntry;
            if (entryA != 0) {
                removedA = OptCatalog::RemoveRuntimeInstance(
                    entryA,
                    0,
                    playerState->rootNode
                );
                if (removedA != 0) {
                    zLoc::FormatMessage(
                        message,
                        sizeof(message),
                        0x248,
                        removedA
                    );
                    HudUi::ShowTopMessageLine(
                        message,
                        5.0f
                    );
                }
            }

            OptCatalogEntryDef *const entryB =
                playerState->altWeaponBanks[5].controllerB.optCatalogEntry;
            if (entryB != 0) {
                removedB = OptCatalog::RemoveRuntimeInstance(
                    entryB,
                    0,
                    playerState->rootNode
                );
                if (removedB != 0) {
                    zLoc::FormatMessage(
                        message,
                        sizeof(message),
                        0x249,
                        removedB
                    );
                    HudUi::ShowTopMessageLine(
                        message,
                        5.0f
                    );
                }
            }

            if (removedA == 0 && removedB == 0) {
                OptCatalog::PlayWeaponInactiveWarning();
            }
            playerState->altGunTriggerProcessFlag = 0;
        }
    } else {
        PlayerGunFireController *const transitionController =
            playerState->altGunTransitionController;

        switch (playerState->altGunTransitionState) {
        case 2:
            playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
            if (playerState->altGunTransitionTimerA > 0.300000012f) {
                playerState->altGunTransitionTimerA = 0.0f;
                playerState->altGunTransitionState = 4;
                saveState->StartMasterTypeLoopSfxHandle(
                    0,
                    1.0f
                );
            }
            break;

        case 4: {
            playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
            const float progress = playerState->altGunTransitionTimerA * 4.0f;
            const float targetY = transitionController->attachPosY - 0.400000006f;
            const float animScale = progress * 0.400000006f;
            playerState->altGunTransitionAnimScale = animScale;

            const float y = transitionController->attachPosY - animScale;
            if (y <= targetY) {
                zClass_Object3D::gwObject3DSetPosition(
                    transitionController->attachNodePrimary,
                    transitionController->attachPosX,
                    targetY,
                    transitionController->attachPosZ
                );
                zClass_Object3D::gwObject3DSetScale(
                    transitionController->attachNodePrimary,
                    1.0f,
                    0.600000024f,
                    0.600000024f
                );
                playerState->altGunTransitionState = 8;
                playerState->altGunTransitionTimerA = 0.0f;
                break;
            }

            zClass_Object3D::gwObject3DSetPosition(
                transitionController->attachNodePrimary,
                transitionController->attachPosX,
                y,
                transitionController->attachPosZ
            );
            const float scale = 1.0f - progress * 0.399999976f;
            zClass_Object3D::gwObject3DSetScale(
                transitionController->attachNodePrimary,
                1.0f,
                scale,
                scale
            );
            zClass_Object3D::gwObject3DSetRotation(
                transitionController->attachNodePrimary,
                0.0f,
                0.0f,
                0.0f
            );
            break;
        }

        case 8: {
            playerState->altGunTransitionTimerB += g_FrameDeltaTimeSec;
            float xScale = playerState->altGunTransitionTimerB * 4.0f;
            if (xScale >= 1.0f) {
                xScale = 1.0f;
                playerState->altGunTransitionState = 16;
                playerState->altGunTransitionTimerB = 0.0f;
            }

            zClass_Object3D::gwObject3DSetScale(
                playerState->doorLeftNode,
                xScale,
                1.0f,
                1.0f
            );
            zClass_Object3D::gwObject3DSetScale(
                playerState->doorRightNode,
                xScale,
                1.0f,
                1.0f
            );
            break;
        }

        case 16: {
            if (transitionController != 0 && transitionController->attachNodePrimary != 0 &&
                (transitionController->flags & kPlayerGunControllerDualMountFlag) == 0) {
                OptCatalogRuntimeInstanceStorage *const attachState =
                    (OptCatalogRuntimeInstanceStorage *)transitionController->attachState;
                if (attachState != 0) {
                    zClass_Class::RemoveChild(
                        transitionController->attachNodePrimary,
                        attachState->projectileNode
                    );
                    OptCatalog::RecycleRuntimeInstanceStorage(
                        transitionController->optCatalogEntry,
                        attachState
                    );
                    transitionController->attachState = 0;
                }

                zClass_Class::gwNodeSetActive(
                    transitionController->attachNodePrimary,
                    0
                );
                zClass_Object3D::gwObject3DSetPosition(
                    transitionController->attachNodePrimary,
                    transitionController->attachPosX,
                    transitionController->attachPosY,
                    transitionController->attachPosZ
                );
            }

            PlayerGunFireController *const activeController =
                playerState->activeAltGunController;
            if (activeController == 0 || activeController->attachNodePrimary == 0) {
                playerState->altGunTransitionState = 1;
                break;
            }

            if ((activeController->flags & kPlayerGunControllerDualMountFlag) != 0) {
                saveState->StartMasterTypeLoopSfxHandle(
                    2,
                    1.0f
                );
                PlayerAltWeaponBank *const bank =
                    &playerState->altWeaponBanks[activeController->weaponBankIndex];
                PlayerGunFireController *const oppositeController =
                    activeController->weaponSideIndex == 0
                        ? &bank->controllerB
                        : &bank->controllerA;
                if (oppositeController->attachNodePrimary != 0) {
                    zClass_Class::gwNodeSetActive(
                        oppositeController->attachNodePrimary,
                        0
                    );
                }
                if (oppositeController->attachNodeSecondary != 0) {
                    zClass_Class::gwNodeSetActive(
                        oppositeController->attachNodeSecondary,
                        0
                    );
                }
                if (activeController->attachNodePrimary != 0) {
                    zClass_Class::gwNodeSetActive(
                        activeController->attachNodePrimary,
                        1
                    );
                }
                if (activeController->attachNodeSecondary != 0) {
                    zClass_Class::gwNodeSetActive(
                        activeController->attachNodeSecondary,
                        1
                    );
                }
                playerState->altGunTransitionState = 1;
                break;
            }

            OptCatalogEntryDef *const entry = activeController->optCatalogEntry;
            if ((entry->flags & kOptCatalogFlagReload) != 0 &&
                activeController->ammoOrCharge > 0.0f) {
                activeController->attachState =
                    OptCatalog::AllocOrReuseAttachNodeClone(entry);
                OptCatalogRuntimeInstanceStorage *const attachState =
                    (OptCatalogRuntimeInstanceStorage *)activeController->attachState;
                zClass_Class::AddChild(
                    activeController->attachNodePrimary,
                    attachState->projectileNode
                );
                attachState->ownerNode = playerState->rootNode;
            }

            zClass_Class::gwNodeSetActive(
                activeController->attachNodePrimary,
                1
            );
            zClass_Object3D::gwObject3DSetPosition(
                activeController->attachNodePrimary,
                activeController->attachPosX,
                activeController->attachPosY - 0.400000006f,
                activeController->attachPosZ
            );
            zClass_Object3D::gwObject3DSetScale(
                activeController->attachNodePrimary,
                1.0f,
                0.600000024f,
                0.600000024f
            );
            playerState->altGunTransitionState = 32;
            saveState->StartMasterTypeLoopSfxHandle(
                0,
                1.0f
            );
            break;
        }

        case 32: {
            playerState->altGunTransitionTimerB += g_FrameDeltaTimeSec;
            float xScale = 1.0f - playerState->altGunTransitionTimerB * 4.0f;
            if (xScale <= 0.00100000005f) {
                xScale = 0.00100000005f;
                playerState->altGunTransitionState = 64;
                playerState->altGunTransitionTimerB = 0.0f;
            }

            zClass_Object3D::gwObject3DSetScale(
                playerState->doorLeftNode,
                xScale,
                1.0f,
                1.0f
            );
            zClass_Object3D::gwObject3DSetScale(
                playerState->doorRightNode,
                xScale,
                1.0f,
                1.0f
            );
            break;
        }

        case 64: {
            PlayerGunFireController *const activeController =
                playerState->activeAltGunController;
            playerState->altGunTransitionTimerA += g_FrameDeltaTimeSec;
            const float progress = playerState->altGunTransitionTimerA * 4.0f;
            const float animScale = progress * 0.400000006f;
            playerState->altGunTransitionAnimScale = animScale;
            const float y =
                animScale + activeController->attachPosY - 0.400000006f;
            if (y >= activeController->attachPosY) {
                playerState->altGunTransitionAnimScale = 0.400000006f;
                playerState->altGunTransitionState = 1;
                playerState->altGunTransitionTimerA = 0.0f;
                zClass_Object3D::gwObject3DSetPosition(
                    activeController->attachNodePrimary,
                    activeController->attachPosX,
                    activeController->attachPosY,
                    activeController->attachPosZ
                );
                zClass_Object3D::gwObject3DSetScale(
                    activeController->attachNodePrimary,
                    1.0f,
                    1.0f,
                    1.0f
                );
                break;
            }

            zClass_Object3D::gwObject3DSetPosition(
                activeController->attachNodePrimary,
                activeController->attachPosX,
                y,
                activeController->attachPosZ
            );
            const float scale = progress * 0.399999976f + 0.600000024f;
            zClass_Object3D::gwObject3DSetScale(
                activeController->attachNodePrimary,
                1.0f,
                scale,
                scale
            );
            break;
        }
        }
    }

    OptCatalog::SetPendingSpawnTargetOverrides(
        0,
        0
    );

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        return;
    }

    OptCatalogEntryDef *const activeEntry =
        activeAltGunController->optCatalogEntry;
    if (playerState->altGunFireHeldFlag != 0 &&
        activeAltGunController->ammoOrCharge != kPlayerAltAmmoDisabledSentinel) {
        activeAltGunController->ammoOrCharge -=
            g_FrameDeltaTimeSec / activeEntry->fireRateInterval;
        if (activeAltGunController->ammoOrCharge < 0.0f) {
            activeAltGunController->ammoOrCharge = 0.0f;
        }
        activeAltGunController->trailRuntimeState->ammoOrChargeMirror =
            activeAltGunController->ammoOrCharge;
    }

    if (activeAltGunController->ammoOrCharge <= 0.0f) {
        if ((activeEntry->flags & kOptCatalogFlagReload) != 0 &&
            playerState->altGunTransitionState != 1) {
            return;
        }

        activeAltGunController->ammoOrCharge = 0.0f;
        if (playerState->altGunFireHeldFlag != 0) {
            activeAltGunController->trailRuntimeState->ammoOrChargeMirror = 0.0f;
            playerState->altGunFireHeldFlag = 0;
            playerState->altGunDispatchRequested = 0;
            OptCatalog::DeactivateTrailRuntimeState(
                activeAltGunController->trailRuntimeState
            );
        }

        HudUiMessage::SetValueIfOwnerMatches(
            activeAltGunController->weaponBankIndex,
            activeAltGunController->weaponSideIndex,
            0.0f
        );
        Player::AutoSwitchToNextUsableAltWeapon(saveState);
    } else if ((activeEntry->flags & kOptCatalogFlagReload) != 0 &&
               playerState->altGunTransitionState == 1 &&
               activeAltGunController->attachState == 0) {
        playerState->altGunTransitionState = 2;
    }

    if (playerState->gunNode != 0) {
        if (playerState->altFireSlotLeft.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotLeft,
                playerState->altFireSlotLeft.attachNode,
                playerState->gunFireDir.y,
                1
            );
        }
        if (playerState->altFireSlotRight.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotRight,
                playerState->altFireSlotRight.attachNode,
                playerState->gunFireDir.y,
                1
            );
        }
        if (playerState->altFireSlotCenter.offset != 0.0f) {
            Player::DecayAndApplyAltFireSlotOffsetToNode(
                &playerState->altFireSlotCenter,
                playerState->altFireSlotCenter.attachNode,
                playerState->gunFireDir.y,
                0
            );
        }
    }

    if (playerState->activePrimaryGunController != 0) {
        Player::ProcessPrimaryGunDispatchTick(saveState);
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-processprimarygundispatchtick
 * @recoil-artifact defines .text recoil:function:0x43a400: Player::ProcessPrimaryGunDispatchTick.
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Source model: Player source-file runtime tick helper for the active primary
 * gun controller; preserves the typed controller/player-state source shape
 * used by the local alt-gun tick owner.
 * Touched data: updates player-state primary dispatch fields, primary fire
 * slot/controller ammo and scroll texture state, compares g_GameStateOrMapTable,
 * and increments g_HudSensorTracker.primaryGunDispatchCount for the active
 * local game state. Shared 0.0/1.0 literals are compiler-pooled constants, not
 * exclusive authored globals.
 * Purpose: process one pending primary-gun dispatch request and route empty
 * primary weapons through the variant-toggle handler.
 */
void __fastcall ProcessPrimaryGunDispatchTick(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;

    if (activePrimaryGunController->scrollTextureModelA != 0) {
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)activePrimaryGunController->scrollTextureModelA
        );
    }
    if (activePrimaryGunController->scrollTextureModelB != 0) {
        zModel_Instance_UpdateScrollingTexturesIfNeeded(
            (zModel_InstancePartial *)activePrimaryGunController->scrollTextureModelB
        );
    }

    if (playerState->primaryGunDispatchRequested == 0) {
        return;
    }

    PlayerGunFireSlot *activeFireSlot = 0;
    SelectPrimaryGunFirePointAndSlot(
        saveState,
        &activeFireSlot
    );
    playerState->primaryGunDispatchRequested = 0;

    if (activePrimaryGunController->ammoOrCharge > 0.0f) {
        if (activePrimaryGunController->ammoOrCharge != kPlayerAltAmmoDisabledSentinel) {
            activePrimaryGunController->ammoOrCharge -= 1.0f;
            if (activePrimaryGunController->ammoOrCharge < 0.0f) {
                activePrimaryGunController->ammoOrCharge = 0.0f;
            }
        }

        EnsureGunAuxEffectActive(
            saveState,
            activePrimaryGunController,
            &playerState->primaryFireOrigin
        );

        if ((activePrimaryGunController->flags & 1) != 0 &&
            activePrimaryGunController->attachNodePrimary != 0 &&
            saveState->primaryModalState->masterModalData->masterType != kPlayerMasterTypeSub) {
            activeFireSlot->offset = 1.5f;
        }

        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            ++g_HudSensorTracker.primaryGunDispatchCount;
        }
    }

    if (activePrimaryGunController->ammoOrCharge == 0.0f) {
        HandlePrimaryWeaponVariantToggleInput(0x31);
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-updategunandturretaimnodes
 * @recoil-artifact defines .text recoil:function:0x43a4f0: Player::UpdateGunAndTurretAimNodes
 * Purpose: apply the alternate gun aim vector to the gun pitch and turret yaw
 * node matrices.
 */
void __fastcall UpdateGunAndTurretAimNodes(
    const zVec3 *aimDirection,
    zClass_NodePartial *gunNode,
    zClass_NodePartial *turretNode
) {
    if (gunNode == 0 || turretNode == 0 || aimDirection == 0) {
        return;
    }

    float horizontalLength =
        aimDirection->x * aimDirection->x + aimDirection->z * aimDirection->z;
    int horizontalLengthBits = 0;
    memcpy(
        &horizontalLengthBits,
        &horizontalLength,
        sizeof(horizontalLengthBits)
    );
    horizontalLengthBits = (horizontalLengthBits >> 1) + 0x1fc00000;
    memcpy(
        &horizontalLength,
        &horizontalLengthBits,
        sizeof(horizontalLength)
    );

    zMat4x3 *const gunMatrix = (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(gunNode);
    gunMatrix->xx = 1.0f;
    gunMatrix->xy = 0.0f;
    gunMatrix->xz = 0.0f;
    gunMatrix->yx = 0.0f;
    gunMatrix->yy = horizontalLength;
    gunMatrix->yz = aimDirection->y;
    gunMatrix->zx = 0.0f;
    gunMatrix->zy = -aimDirection->y;
    gunMatrix->zz = horizontalLength;
    zClass_Object3D::gwObject3DSetMatrix(
        gunNode,
        (float *)gunMatrix
    );

    float yawForward = 1.0f;
    float yawSide = 0.0f;
    if (horizontalLength != 0.0f) {
        const float invHorizontalLength = 1.0f / horizontalLength;
        yawForward = -(aimDirection->z * invHorizontalLength);
        yawSide = -(aimDirection->x * invHorizontalLength);
    }

    zMat4x3 *const turretMatrix = (zMat4x3 *)zClass_Object3D::gwObject3DGetMatrixPtr(turretNode);
    turretMatrix->xx = yawForward;
    turretMatrix->xy = 0.0f;
    turretMatrix->xz = -yawSide;
    turretMatrix->yx = 0.0f;
    turretMatrix->yy = 1.0f;
    turretMatrix->yz = 0.0f;
    turretMatrix->zx = yawSide;
    turretMatrix->zy = 0.0f;
    turretMatrix->zz = yawForward;
    zClass_Object3D::gwObject3DSetMatrix(
        turretNode,
        (float *)turretMatrix
    );
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-updatealtgunaimdirection
 * @recoil-artifact defines .text recoil:function:0x43a600: Player::UpdateAltGunAimDirection
 * Purpose: update the smoothed alternate gun aim direction and final gun-fire
 * vector from the current target and aim basis.
 */
void __fastcall UpdateAltGunAimDirection(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->gunNode == 0 || playerState->turretNode == 0) {
        return;
    }

    BuildGunFireTransform(saveState);
    UpdateAltGunAimBasisOrigin(
        saveState,
        &playerState->aimBasisOrigin
    );

    zVec3 aimDirection = {0};
    aimDirection.x = playerState->storedTargetPos.x - playerState->aimBasisOrigin.x;
    aimDirection.y = playerState->storedTargetPos.y - playerState->aimBasisOrigin.y;
    aimDirection.z = playerState->storedTargetPos.z - playerState->aimBasisOrigin.z;

    const float aimLength = (float)(sqrt(
        aimDirection.x * aimDirection.x + aimDirection.y * aimDirection.y +
        aimDirection.z * aimDirection.z
    ));
    const float invAimLength = 1.0f / aimLength;
    aimDirection.x *= invAimLength;
    aimDirection.y *= invAimLength;
    aimDirection.z *= invAimLength;

    const float pitchY = OptCatalog::ComputeAimPitchForTarget(
        playerState->activeAltGunController->optCatalogEntry,
        &playerState->aimBasisOrigin,
        &playerState->gunFireDir,
        &playerState->storedTargetPos,
        &playerState->aimTargetDistanceApprox
    );
    playerState->aimPitchResult = pitchY;
    if (pitchY != -1.0f && playerState->altGunTransitionState == 1) {
        ApplyAimPitchToDirection(
            &aimDirection,
            pitchY
        );
    }

    if (playerState->cameraTickEnabled != 0 &&
        (playerState->cameraState == kPlayerCameraStateThirdPerson ||
            playerState->cameraState == kPlayerCameraStateFirstPerson)) {
        const float cameraDot = aimDirection.x * playerState->cameraDirNext.x +
                                aimDirection.y * playerState->cameraDirNext.y +
                                aimDirection.z * playerState->cameraDirNext.z;
        const float targetDistanceSq =
            zMath::Vec3DeltaLengthSq(
                &playerState->storedTargetPos,
                &playerState->worldPos
            );
        if (cameraDot < 0.0f || targetDistanceSq < 9.0f) {
            aimDirection = playerState->gunFireDir;
            playerState->usePresetGunFireDir = 1;
        }
    }

    const zVec3 worldAimDirection = aimDirection;
    const zMat4x3 &gunFireTransform = playerState->gunFireTransform;
    aimDirection.x = worldAimDirection.x * gunFireTransform.xx +
                     worldAimDirection.y * gunFireTransform.xy +
                     worldAimDirection.z * gunFireTransform.xz;
    aimDirection.y = worldAimDirection.x * gunFireTransform.yx +
                     worldAimDirection.y * gunFireTransform.yy +
                     worldAimDirection.z * gunFireTransform.yz;
    aimDirection.z = worldAimDirection.x * gunFireTransform.zx +
                     worldAimDirection.y * gunFireTransform.zy +
                     worldAimDirection.z * gunFireTransform.zz;
    if (aimDirection.y > masterModalData->gunPitchRate) {
        ApplyAimPitchToDirection(
            &aimDirection,
            masterModalData->gunPitchRate
        );
    }
    if (aimDirection.y < masterModalData->gunPitchMin) {
        ApplyAimPitchToDirection(
            &aimDirection,
            masterModalData->gunPitchMin
        );
    }

    const int smoothingBits =
        (int)(g_FrameDeltaTimeSec * -8.0f * 12102200.0f) + 0x3f800000;
    float smoothingFactor = 0.0f;
    memcpy(
        &smoothingFactor,
        &smoothingBits,
        sizeof(smoothingFactor)
    );
    zMath::Vec3LerpNormalize(
        &playerState->altGunAimOrigin,
        &aimDirection,
        smoothingFactor
    );
    aimDirection = playerState->altGunAimOrigin;

    UpdateGunAndTurretAimNodes(
        &aimDirection,
        playerState->gunNode,
        playerState->turretNode
    );
    playerState->gunFireDir.x = aimDirection.x * gunFireTransform.xx +
                                aimDirection.y * gunFireTransform.yx +
                                aimDirection.z * gunFireTransform.zx;
    playerState->gunFireDir.y = aimDirection.x * gunFireTransform.xy +
                                aimDirection.y * gunFireTransform.yy +
                                aimDirection.z * gunFireTransform.zy;
    playerState->gunFireDir.z = aimDirection.x * gunFireTransform.xz +
                                aimDirection.y * gunFireTransform.yz +
                                aimDirection.z * gunFireTransform.zz;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-decayandapplyaltfireslotoffsettonode
 * @recoil-artifact defines .text recoil:function:0x43a900: Player::DecayAndApplyAltFireSlotOffsetToNode.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::DecayAndApplyAltFireSlotOffsetToNode from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall DecayAndApplyAltFireSlotOffsetToNode(
    PlayerGunFireSlot *slot,
    zClass_NodePartial *slotNode,
    float slotAimY,
    int applyMatrix
) {
    const int dampingBits = (int)(g_FrameDeltaTimeSec * -8.09f * 12102200.0f) + 0x3f800000;
    float dampingFactor = 0.0f;
    memcpy(
        &dampingFactor,
        &dampingBits,
        sizeof(dampingFactor)
    );
    slot->offset *= dampingFactor;
    if (slot->offset > -0.01f && slot->offset < 0.01f) {
        slot->offset = 0.0f;
    }

    float *const matrix = zClass_Object3D::gwObject3DGetMatrixPtr(slotNode);
    matrix[10] = -(slotAimY * slot->offset);
    matrix[11] = slot->offset;
    if (applyMatrix != 0) {
        zClass_Object3D::gwObject3DSetMatrix(
            slotNode,
            matrix
        );
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-applygunfireslotoffsettonode
 * @recoil-artifact defines .text recoil:function:0x43a980: Player::ApplyGunFireSlotOffsetToNode.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ApplyGunFireSlotOffsetToNode from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ApplyGunFireSlotOffsetToNode(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->gunNode == 0) {
        return;
    }

    if (playerState->altFireSlotLeft.offset != 0.0f) {
        playerState->altFireSlotLeft.offset = 0.0f;
        DecayAndApplyAltFireSlotOffsetToNode(
            &playerState->altFireSlotLeft,
            playerState->altFireSlotLeft.attachNode,
            playerState->gunFireDir.y,
            1
        );
    }
    if (playerState->altFireSlotRight.offset != 0.0f) {
        playerState->altFireSlotRight.offset = 0.0f;
        DecayAndApplyAltFireSlotOffsetToNode(
            &playerState->altFireSlotRight,
            playerState->altFireSlotRight.attachNode,
            playerState->gunFireDir.y,
            1
        );
    }
    if (playerState->altFireSlotCenter.offset != 0.0f) {
        playerState->altFireSlotCenter.offset = 0.0f;
        DecayAndApplyAltFireSlotOffsetToNode(
            &playerState->altFireSlotCenter,
            playerState->altFireSlotCenter.attachNode,
            playerState->gunFireDir.y,
            0
        );
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-selectaltgunfirepointandslot
 * @recoil-artifact defines .text recoil:function:0x43aa30: Player::SelectAltGunFirePointAndSlot
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: choose the alternate-gun fire origin and slot for the active
 * controller.
 */
void __fastcall SelectAltGunFirePointAndSlot(
    zUtil_SaveGameState *saveState,
    PlayerGunFireSlot **outActiveFireSlotPtr
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;

    if (playerState->gunNode == 0 || playerState->turretNode == 0) {
        const float y = playerState->worldPos.y + 1.0f;
        playerState->altFireOrigin.x = playerState->worldPos.x;
        playerState->altFireOrigin.y = y;
        playerState->altFireOrigin.z = playerState->worldPos.z;
        playerState->aimBasisOrigin.x = playerState->worldPos.x;
        playerState->aimBasisOrigin.y = y;
        playerState->aimBasisOrigin.z = playerState->worldPos.z;
        playerState->gunFireDir = playerState->steerBasisRaw;
        return;
    }

    zMat4x3 aimBasisWorldMatrix = {0};
    ComposeAimBasisWorldMatrix(
        saveState,
        &aimBasisWorldMatrix
    );

    switch (playerState->altHardpointSelectState) {
    case 0:
        if (activeAltGunController->attachState != 0) {
            playerState->altFireOrigin.x = aimBasisWorldMatrix.posX;
            playerState->altFireOrigin.y = aimBasisWorldMatrix.posY;
            playerState->altFireOrigin.z = aimBasisWorldMatrix.posZ;
        } else {
            playerState->altFireOrigin.x =
                playerState->firePointCenter.x * aimBasisWorldMatrix.xx +
                playerState->firePointCenter.y * aimBasisWorldMatrix.yx +
                playerState->firePointCenter.z * aimBasisWorldMatrix.zx +
                aimBasisWorldMatrix.posX;
            playerState->altFireOrigin.y =
                playerState->firePointCenter.x * aimBasisWorldMatrix.xy +
                playerState->firePointCenter.y * aimBasisWorldMatrix.yy +
                playerState->firePointCenter.z * aimBasisWorldMatrix.zy +
                aimBasisWorldMatrix.posY;
            playerState->altFireOrigin.z =
                playerState->firePointCenter.x * aimBasisWorldMatrix.xz +
                playerState->firePointCenter.y * aimBasisWorldMatrix.yz +
                playerState->firePointCenter.z * aimBasisWorldMatrix.zz +
                aimBasisWorldMatrix.posZ;
        }
        playerState->altFireSlotCenter.attachNode = activeAltGunController->attachNodePrimary;
        *outActiveFireSlotPtr = &playerState->altFireSlotCenter;
        return;

    case 1:
        playerState->altFireOrigin.x =
            playerState->firePointRight.x * aimBasisWorldMatrix.xx +
            playerState->firePointRight.y * aimBasisWorldMatrix.yx +
            playerState->firePointRight.z * aimBasisWorldMatrix.zx +
            aimBasisWorldMatrix.posX;
        playerState->altFireOrigin.y =
            playerState->firePointRight.x * aimBasisWorldMatrix.xy +
            playerState->firePointRight.y * aimBasisWorldMatrix.yy +
            playerState->firePointRight.z * aimBasisWorldMatrix.zy +
            aimBasisWorldMatrix.posY;
        playerState->altFireOrigin.z =
            playerState->firePointRight.x * aimBasisWorldMatrix.xz +
            playerState->firePointRight.y * aimBasisWorldMatrix.yz +
            playerState->firePointRight.z * aimBasisWorldMatrix.zz +
            aimBasisWorldMatrix.posZ;
        playerState->altFireSlotRight.attachNode = activeAltGunController->attachNodeSecondary;
        *outActiveFireSlotPtr = &playerState->altFireSlotRight;
        playerState->altHardpointSelectState = 2;
        return;

    case 2:
        playerState->altFireOrigin.x =
            playerState->firePointLeft.x * aimBasisWorldMatrix.xx +
            playerState->firePointLeft.y * aimBasisWorldMatrix.yx +
            playerState->firePointLeft.z * aimBasisWorldMatrix.zx +
            aimBasisWorldMatrix.posX;
        playerState->altFireOrigin.y =
            playerState->firePointLeft.x * aimBasisWorldMatrix.xy +
            playerState->firePointLeft.y * aimBasisWorldMatrix.yy +
            playerState->firePointLeft.z * aimBasisWorldMatrix.zy +
            aimBasisWorldMatrix.posY;
        playerState->altFireOrigin.z =
            playerState->firePointLeft.x * aimBasisWorldMatrix.xz +
            playerState->firePointLeft.y * aimBasisWorldMatrix.yz +
            playerState->firePointLeft.z * aimBasisWorldMatrix.zz +
            aimBasisWorldMatrix.posZ;
        playerState->altFireSlotLeft.attachNode = activeAltGunController->attachNodePrimary;
        *outActiveFireSlotPtr = &playerState->altFireSlotLeft;
        playerState->altHardpointSelectState = 1;
        return;
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-selectprimarygunfirepointandslot
 * @recoil-artifact defines .text recoil:function:0x43acf0: Player::SelectPrimaryGunFirePointAndSlot.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SelectPrimaryGunFirePointAndSlot from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall SelectPrimaryGunFirePointAndSlot(
    zUtil_SaveGameState *saveState,
    PlayerGunFireSlot **outActiveFireSlotPtr
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activePrimaryGunController =
        playerState->activePrimaryGunController;

    if (playerState->gunNode == 0 || playerState->turretNode == 0) {
        const float y = playerState->worldPos.y + 1.0f;
        playerState->primaryFireOrigin.x = playerState->worldPos.x;
        playerState->primaryFireOrigin.y = y;
        playerState->primaryFireOrigin.z = playerState->worldPos.z;
        playerState->aimBasisOrigin.x = playerState->worldPos.x;
        playerState->aimBasisOrigin.y = y;
        playerState->aimBasisOrigin.z = playerState->worldPos.z;
        playerState->gunFireDir = playerState->steerBasisRaw;
        return;
    }

    if (playerState->damageVisualFlag != 0) {
        CacheGunHardpointsAndDetachDisplays(
            saveState,
            0
        );
        playerState->damageVisualFlag = 0;
    }

    zMat4x3 aimBasisWorldMatrix = {0};
    ComposeAimBasisWorldMatrix(
        saveState,
        &aimBasisWorldMatrix
    );

    switch (playerState->primaryHardpointSelectState) {
    case 0:
        if (activePrimaryGunController->attachState != 0) {
            playerState->primaryFireOrigin.x = aimBasisWorldMatrix.posX;
            playerState->primaryFireOrigin.y = aimBasisWorldMatrix.posY;
            playerState->primaryFireOrigin.z = aimBasisWorldMatrix.posZ;
        } else {
            playerState->primaryFireOrigin.x =
                playerState->firePointCenter.x * aimBasisWorldMatrix.xx +
                playerState->firePointCenter.y * aimBasisWorldMatrix.yx +
                playerState->firePointCenter.z * aimBasisWorldMatrix.zx +
                aimBasisWorldMatrix.posX;
            playerState->primaryFireOrigin.y =
                playerState->firePointCenter.x * aimBasisWorldMatrix.xy +
                playerState->firePointCenter.y * aimBasisWorldMatrix.yy +
                playerState->firePointCenter.z * aimBasisWorldMatrix.zy +
                aimBasisWorldMatrix.posY;
            playerState->primaryFireOrigin.z =
                playerState->firePointCenter.x * aimBasisWorldMatrix.xz +
                playerState->firePointCenter.y * aimBasisWorldMatrix.yz +
                playerState->firePointCenter.z * aimBasisWorldMatrix.zz +
                aimBasisWorldMatrix.posZ;
        }
        playerState->altFireSlotCenter.attachNode = activePrimaryGunController->attachNodePrimary;
        *outActiveFireSlotPtr = &playerState->altFireSlotCenter;
        return;

    case 1:
        playerState->primaryFireOrigin.x =
            playerState->firePointRight.x * aimBasisWorldMatrix.xx +
            playerState->firePointRight.y * aimBasisWorldMatrix.yx +
            playerState->firePointRight.z * aimBasisWorldMatrix.zx +
            aimBasisWorldMatrix.posX;
        playerState->primaryFireOrigin.y =
            playerState->firePointRight.x * aimBasisWorldMatrix.xy +
            playerState->firePointRight.y * aimBasisWorldMatrix.yy +
            playerState->firePointRight.z * aimBasisWorldMatrix.zy +
            aimBasisWorldMatrix.posY;
        playerState->primaryFireOrigin.z =
            playerState->firePointRight.x * aimBasisWorldMatrix.xz +
            playerState->firePointRight.y * aimBasisWorldMatrix.yz +
            playerState->firePointRight.z * aimBasisWorldMatrix.zz +
            aimBasisWorldMatrix.posZ;
        playerState->altFireSlotRight.attachNode = activePrimaryGunController->attachNodeSecondary;
        *outActiveFireSlotPtr = &playerState->altFireSlotRight;
        playerState->primaryHardpointSelectState = 2;
        return;

    case 2:
        playerState->primaryFireOrigin.x =
            playerState->firePointLeft.x * aimBasisWorldMatrix.xx +
            playerState->firePointLeft.y * aimBasisWorldMatrix.yx +
            playerState->firePointLeft.z * aimBasisWorldMatrix.zx +
            aimBasisWorldMatrix.posX;
        playerState->primaryFireOrigin.y =
            playerState->firePointLeft.x * aimBasisWorldMatrix.xy +
            playerState->firePointLeft.y * aimBasisWorldMatrix.yy +
            playerState->firePointLeft.z * aimBasisWorldMatrix.zy +
            aimBasisWorldMatrix.posY;
        playerState->primaryFireOrigin.z =
            playerState->firePointLeft.x * aimBasisWorldMatrix.xz +
            playerState->firePointLeft.y * aimBasisWorldMatrix.yz +
            playerState->firePointLeft.z * aimBasisWorldMatrix.zz +
            aimBasisWorldMatrix.posZ;
        playerState->altFireSlotLeft.attachNode = activePrimaryGunController->attachNodePrimary;
        *outActiveFireSlotPtr = &playerState->altFireSlotLeft;
        playerState->primaryHardpointSelectState = 1;
        return;
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-composeaimbasisworldmatrix
 * @recoil-artifact defines .text recoil:function:0x43afd0: Player::ComposeAimBasisWorldMatrix
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: compose the current player aim basis into a world-space transform.
 */
void __fastcall ComposeAimBasisWorldMatrix(
    zUtil_SaveGameState *saveState,
    zMat4x3 *outMatrix34
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zMat4x3 gunMatrix = {0};
    memcpy(
        &gunMatrix,
        zClass_Object3D::gwObject3DGetMatrixPtr(playerState->gunNode),
        sizeof(gunMatrix)
    );

    zMat4x3 turretMatrix = {0};
    memcpy(
        &turretMatrix,
        zClass_Object3D::gwObject3DGetMatrixPtr(playerState->turretNode),
        sizeof(turretMatrix)
    );

    zMat4x3 gunFireTransform = {0};
    memcpy(
        &gunFireTransform,
        &playerState->gunFireTransform,
        sizeof(gunFireTransform)
    );

    outMatrix34->xx = turretMatrix.xx * gunFireTransform.xx + turretMatrix.xz * gunFireTransform.zx;
    outMatrix34->xy = turretMatrix.xx * gunFireTransform.xy + turretMatrix.xz * gunFireTransform.zy;
    outMatrix34->xz = turretMatrix.xx * gunFireTransform.xz + turretMatrix.xz * gunFireTransform.zz;

    const float yawX =
        turretMatrix.zx * gunFireTransform.xx + turretMatrix.zz * gunFireTransform.zx;
    const float yawY =
        turretMatrix.zx * gunFireTransform.xy + turretMatrix.zz * gunFireTransform.zy;
    const float yawZ =
        turretMatrix.zx * gunFireTransform.xz + turretMatrix.zz * gunFireTransform.zz;

    outMatrix34->yx = gunFireTransform.yx * gunMatrix.yy + gunMatrix.yz * yawX;
    outMatrix34->yy = gunFireTransform.yy * gunMatrix.yy + gunMatrix.yz * yawY;
    outMatrix34->yz = gunFireTransform.yz * gunMatrix.yy + gunMatrix.yz * yawZ;

    outMatrix34->zx = gunFireTransform.yx * gunMatrix.zy + gunMatrix.zz * yawX;
    outMatrix34->zy = gunFireTransform.yy * gunMatrix.zy + gunMatrix.zz * yawY;
    outMatrix34->zz = gunFireTransform.yz * gunMatrix.zy + gunMatrix.zz * yawZ;

    outMatrix34->posX = playerState->aimBasisOrigin.x;
    outMatrix34->posY = playerState->aimBasisOrigin.y;
    outMatrix34->posZ = playerState->aimBasisOrigin.z;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-buildgunfiretransform
 * @recoil-artifact defines .text recoil:function:0x43b1b0: Player::BuildGunFireTransform
 * Purpose: build the player gun-fire transform from the root and active modal
 * node matrices.
 */
void __fastcall BuildGunFireTransform(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerModalState *const primaryModalState = saveState->primaryModalState;

    zMat4x3 rootMatrix = {0};
    memcpy(
        &rootMatrix,
        zClass_Object3D::gwObject3DGetMatrixPtr(playerState->rootNode),
        sizeof(rootMatrix)
    );

    if (primaryModalState->modalNode == 0) {
        memcpy(
            &playerState->gunFireTransform,
            &rootMatrix,
            sizeof(rootMatrix)
        );
        return;
    }

    zMat4x3 modalMatrix = {0};
    memcpy(
        &modalMatrix,
        zClass_Object3D::gwObject3DGetMatrixPtr(primaryModalState->modalNode),
        sizeof(modalMatrix)
    );

    playerState->gunFireTransform.xx = modalMatrix.xx * rootMatrix.xx +
                                       modalMatrix.xy * rootMatrix.yx +
                                       modalMatrix.xz * rootMatrix.zx;
    playerState->gunFireTransform.xy = modalMatrix.xx * rootMatrix.xy +
                                       modalMatrix.xy * rootMatrix.yy +
                                       modalMatrix.xz * rootMatrix.zy;
    playerState->gunFireTransform.xz = modalMatrix.xx * rootMatrix.xz +
                                       modalMatrix.xy * rootMatrix.yz +
                                       modalMatrix.xz * rootMatrix.zz;
    playerState->gunFireTransform.yx = modalMatrix.yx * rootMatrix.xx +
                                       modalMatrix.yy * rootMatrix.yx +
                                       modalMatrix.yz * rootMatrix.zx;
    playerState->gunFireTransform.yy = modalMatrix.yx * rootMatrix.xy +
                                       modalMatrix.yy * rootMatrix.yy +
                                       modalMatrix.yz * rootMatrix.zy;
    playerState->gunFireTransform.yz = modalMatrix.yx * rootMatrix.xz +
                                       modalMatrix.yy * rootMatrix.yz +
                                       modalMatrix.yz * rootMatrix.zz;
    playerState->gunFireTransform.zx =
        modalMatrix.zy * rootMatrix.yx + modalMatrix.zz * rootMatrix.zx;
    playerState->gunFireTransform.zy =
        modalMatrix.zy * rootMatrix.yy + modalMatrix.zz * rootMatrix.zy;
    playerState->gunFireTransform.zz =
        modalMatrix.zy * rootMatrix.yz + modalMatrix.zz * rootMatrix.zz;
    playerState->gunFireTransform.posX =
        modalMatrix.posY * rootMatrix.yx + modalMatrix.posZ * rootMatrix.zx + rootMatrix.posX;
    playerState->gunFireTransform.posY =
        modalMatrix.posY * rootMatrix.yy + modalMatrix.posZ * rootMatrix.zy + rootMatrix.posY;
    playerState->gunFireTransform.posZ =
        modalMatrix.posY * rootMatrix.yz + modalMatrix.posZ * rootMatrix.zz + rootMatrix.posZ;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-updatealtgunaimbasisorigin
 * @recoil-artifact defines .text recoil:function:0x43b3e0: Player::UpdateAltGunAimBasisOrigin
 * Purpose: compute the world-space origin used as the alternate gun aim basis.
 */
void __fastcall UpdateAltGunAimBasisOrigin(
    zUtil_SaveGameState *saveState,
    zVec3 *outBasisOrigin
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zMat4x3 gunMatrix = {0};
    memcpy(
        &gunMatrix,
        zClass_Object3D::gwObject3DGetMatrixPtr(playerState->gunNode),
        sizeof(gunMatrix)
    );

    zMat4x3 turretMatrix = {0};
    memcpy(
        &turretMatrix,
        zClass_Object3D::gwObject3DGetMatrixPtr(playerState->turretNode),
        sizeof(turretMatrix)
    );

    zMat4x3 gunFireTransform = {0};
    memcpy(
        &gunFireTransform,
        &playerState->gunFireTransform,
        sizeof(gunFireTransform)
    );

    const float localAimX = turretMatrix.zx * gunMatrix.posZ;
    const float localAimY = turretMatrix.posY + gunMatrix.posY;
    const float localAimZ = turretMatrix.zz * gunMatrix.posZ + turretMatrix.posZ;

    outBasisOrigin->x = gunFireTransform.xx * localAimX + gunFireTransform.yx * localAimY +
                        gunFireTransform.zx * localAimZ + gunFireTransform.posX;
    outBasisOrigin->y = gunFireTransform.xy * localAimX + gunFireTransform.yy * localAimY +
                        gunFireTransform.zy * localAimZ + gunFireTransform.posY;
    outBasisOrigin->z = gunFireTransform.xz * localAimX + gunFireTransform.yz * localAimY +
                        gunFireTransform.zz * localAimZ + gunFireTransform.posZ;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-applyaimpitchtodirection
 * @recoil-artifact defines .text recoil:function:0x43b500: Player::ApplyAimPitchToDirection
 * Purpose: adjust an aim direction to the requested pitch while preserving
 * horizontal heading when possible.
 */
void __fastcall ApplyAimPitchToDirection(
    zVec3 *direction,
    float pitchY
) {
    const float horizontalLenSq = direction->x * direction->x + direction->z * direction->z;
    if (horizontalLenSq == 0.0f) {
        if (pitchY == 0.0f) {
            *direction = kPlayerDefaultAltGunAimOrigin;
            return;
        }

        float diagonal = (1.0f - pitchY * pitchY) * 0.5f;
        int diagonalBits = 0;
        memcpy(
            &diagonalBits,
            &diagonal,
            sizeof(diagonalBits)
        );
        diagonalBits = (diagonalBits >> 1) + 0x1fc00000;
        memcpy(
            &diagonal,
            &diagonalBits,
            sizeof(diagonal)
        );
        direction->x = diagonal;
        direction->y = pitchY;
        direction->z = diagonal;
        return;
    }

    float scale = (1.0f - pitchY * pitchY) / horizontalLenSq;
    int scaleBits = 0;
    memcpy(
        &scaleBits,
        &scale,
        sizeof(scaleBits)
    );
    scaleBits = (scaleBits >> 1) + 0x1fc00000;
    memcpy(
        &scale,
        &scaleBits,
        sizeof(scale)
    );
    direction->x *= scale;
    direction->y = pitchY;
    direction->z *= scale;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-applystatusmeterchange
 * @recoil-artifact defines .text recoil:function:0x43b5d0: Player::ApplyStatusMeterChange.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_status.cpp.
 * Purpose: apply an absolute or relative status-meter change, clamp it to the
 * player's health range, publish the ratio, and refresh the shield HUD meter.
 * ABI/source shape: __fastcall free function in the Player status-meter source
 * slice; uses typed zUtil_SaveGameState, zUtil_PlayerStateStorage, and
 * PlayerMasterCommonData fields rather than raw runtime offsets. The broader
 * Player source owner remains parent-pending in the plan.
 * Touched data owner: g_PlayerStatusMeterRatio is covered by accepted
 * battlesport_gameplay.player_damage_runtime_globals.
 * Dependencies: HudUiMgrSensor::SetShieldMessageRatio and
 * PlayerMasterCommonData::maxHealth/invMaxHealth provide the HUD and clamp
 * contracts used by callers.
 */
void __fastcall ApplyStatusMeterChange(
    zUtil_SaveGameState *saveState,
    int mode,
    float delta
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    if (mode != 0) {
        playerState->statusMeterValue += delta;
    } else {
        playerState->statusMeterValue = delta;
    }

    if (!(playerState->statusMeterValue <= masterCommonData->maxHealth)) {
        playerState->statusMeterValue = masterCommonData->maxHealth;
    } else if (playerState->statusMeterValue < 0.0f) {
        playerState->statusMeterValue = 0.0f;
    }

    g_PlayerStatusMeterRatio = masterCommonData->invMaxHealth * playerState->statusMeterValue;
    HudUiMgrSensor::SetShieldMessageRatio(g_PlayerStatusMeterRatio);
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-updatestatusmeter
 * @recoil-artifact defines .text recoil:function:0x43b660: Player::UpdateStatusMeter.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_status.cpp.
 * Purpose: process status-meter restore/gain updates, show localized HUD
 * feedback, trigger the restore visual path, and reset damage state when the
 * meter is restored absolutely.
 * ABI/source shape: __fastcall free function in the Player status-meter source
 * slice; keeps the original helper dependency on ApplyStatusMeterChange and
 * typed zUtil_SaveGameState/zUtil_PlayerStateStorage access. The broader Player
 * source owner remains parent-pending in the plan.
 * Touched data owner: reads g_PlayerStatusMeterRatio through accepted
 * battlesport_gameplay.player_damage_runtime_globals.
 * Dependencies: zLoc/HudUi message formatting, zEffectAnim restore velocity,
 * and ResetDamageStateAndTimedHitStatus are required side-effect contracts.
 */
int __fastcall UpdateStatusMeter(
    zUtil_SaveGameState *saveState,
    int mode,
    float delta
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (mode == 0) {
        ApplyStatusMeterChange(
            saveState,
            mode,
            playerState->masterCommonData->maxHealth
        );
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x902),
            5.0f
        );
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x246),
            5.0f
        );
        zEffectAnim::SetVelocity_Thunk(
            playerState->regenSkinFxEntry,
            0,
            0.0f,
            0.0f,
            0.0f
        );
        ResetDamageStateAndTimedHitStatus(saveState);
        return 1;
    }

    const float oldStatusMeterRatio = g_PlayerStatusMeterRatio;
    ApplyStatusMeterChange(
        saveState,
        1,
        delta
    );

    char message[64];
    const int percentGain = (int)((g_PlayerStatusMeterRatio - oldStatusMeterRatio) * 100.0f);
    zLoc::FormatMessage(
        message,
        sizeof(message),
        0x903,
        percentGain
    );
    HudUi::ShowTopMessageLine(
        message,
        5.0f
    );
    return 1;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-recordrecenthitfeedback
 * @recoil-artifact defines .text recoil:function:0x43b730: Player::RecordRecentHitFeedback
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: cache the latest hit source/context and restart the recent-hit
 * feedback light effect for later damage and kill attribution.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: source-owner packet groups this callback with the damage-hit and
 * destroyed-state handlers; body updates recent-hit storage, stops any prior
 * light effect, and starts the recovered recent-hit effect handle.
 */
void __fastcall RecordRecentHitFeedback(
    zUtil_SaveGameState *saveState,
    OptCatalogEntryDef *hitSource,
    float damage
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->recentHitValid = 1;
    playerState->lastHitOwnerOrCtx = HitContext::GetCurrentOwnerOrCtx();
    playerState->recentHitSource = hitSource;
    playerState->recentHitDamage = damage;
    playerState->recentHitFxExpireTime = g_Time_AccumulatedTimeSec + 4.0f;

    zEffectAnimEntry *const recentHitLightHandle = playerState->recentHitLightHandle;
    if (recentHitLightHandle != 0) {
        zEffectAnim::Stop(recentHitLightHandle);
    }

    playerState->recentHitLightHandle = zEffectAnim::SetPositionRefAndVelocity_Thunk(
        g_PlayerRecentHitFxAnimEntry,
        0,
        playerState->rootNode,
        0,
        0
    );
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-updatetimedhitstatusfromhitsource
 * @recoil-artifact defines .text recoil:function:0x43b790: Player::UpdateTimedHitStatusFromHitSource
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: update the player's timed-hit status contribution from a hit source
 * and return the remaining damage that should be applied.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: body applies the special max-health status path for timed-status
 * sources, otherwise scales damage by inverse max health and suppresses damage
 * only when the timed status update reports a completed status event.
 */
float __fastcall UpdateTimedHitStatusFromHitSource(
    zUtil_SaveGameState *saveState,
    OptCatalogEntryDef *hitSource,
    float damage
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    if ((hitSource->flags & 0x800u) != 0) {
        HitSource::UpdateTimedStatus(
            hitSource,
            &playerState->timedHitStatus,
            masterCommonData->maxHealth
        );
        return damage;
    }

    const float contribution = masterCommonData->invMaxHealth * damage;
    if (HitSource::UpdateTimedStatus(
        hitSource,
        &playerState->timedHitStatus,
        contribution
    ) == 1) {
        return 0.0f;
    }
    return damage;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-cleardestroyedrespawneffecthandlecallback
 * @recoil-artifact defines .text recoil:function:0x43b800: Player::ClearDestroyedRespawnEffectHandleCallback
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: clear the async destroyed-respawn effect handle when the effect
 * callback completes.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: callback signature matches the effect-animation completion ABI and
 * the body only clears the save-state player's destroyed respawn async handle.
 */
void __fastcall ClearDestroyedRespawnEffectHandleCallback(
    zEffectAnimEntry *,
    zUtil_SaveGameState *saveState,
    int
) {
    saveState->playerState->destroyedRespawnAsyncHandle = 0;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-hitcallback-recordnetcontextandtimedstatus
 * @recoil-artifact defines .text recoil:function:0x43b810: Player::HitCallback_RecordNetContextAndTimedStatus
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: network hit callback that records recent-hit context and timed-hit
 * status without applying local damage.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: callback exits for inactive lifecycle state, conditionally records
 * recent-hit feedback and timed-hit status from hit-source flags, and returns
 * the recent-hit validity state used by the damage callback contract.
 */
int __fastcall HitCallback_RecordNetContextAndTimedStatus(
    zUtil_SaveGameState *saveState,
    OptCatalogEntryDef *hitSource,
    void *,
    float damage
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        return 0;
    }

    if (hitSource != 0) {
        if ((hitSource->flags & kOptCatalogFlagRecordsRecentHit) != 0) {
            RecordRecentHitFeedback(
                saveState,
                hitSource,
                damage
            );
        }
        if ((hitSource->flags & kOptCatalogFlagAppliesTimedHitStatus) != 0) {
            UpdateTimedHitStatusFromHitSource(
                saveState,
                hitSource,
                damage
            );
        }
    }

    return playerState->recentHitValid;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-hitcallback-recordcontextandtimedstatus
 * @recoil-artifact defines .text recoil:function:0x43b870: Player::HitCallback_RecordContextAndTimedStatus
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: gameplay hit callback that records hit context, applies damage,
 * enters destroyed-state side effects, and awards kill rewards.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: body shares the recent-hit/timed-status helpers with the other hit
 * callbacks, rejects self-owned damage, updates status meter scaling, starts
 * destroyed vehicle effects through the recovered callback, clears alt-gun
 * runtime, records damage context, and preserves nanite/reward side effects.
 */
int __fastcall HitCallback_RecordContextAndTimedStatus(
    zUtil_SaveGameState *saveState,
    OptCatalogEntryDef *hitSource,
    void *hitRenderPointEntry,
    float damage
) {
    OptCatalogEntryDef *killEventContext = hitSource;
    int pickupRewardMultiplier = 1;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;

    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        return 0;
    }

    void *ownerOrCtx = 0;
    if (hitSource != 0) {
        ownerOrCtx = HitContext::GetCurrentOwnerOrCtx();
    } else if (playerState->recentHitValid != 0) {
        killEventContext = playerState->recentHitSource;
        hitSource = killEventContext;
        ownerOrCtx = playerState->lastHitOwnerOrCtx;
    }

    if (ownerOrCtx != 0) {
        HitOwnerOrContextPartial *const hitOwner = (HitOwnerOrContextPartial *)(ownerOrCtx);
        HitOwnerSaveStateLinkPartial *const ownerLink = hitOwner->ownerLink;
        if (ownerLink != 0 && ownerLink->ownerSaveState == saveState) {
            return 0;
        }
    }

    if (hitSource != 0) {
        if ((hitSource->flags & kOptCatalogFlagRecordsRecentHit) != 0) {
            RecordRecentHitFeedback(
                saveState,
                hitSource,
                damage
            );
        }
        if ((hitSource->flags & kOptCatalogFlagAppliesTimedHitStatus) != 0) {
            damage = UpdateTimedHitStatusFromHitSource(
                saveState,
                hitSource,
                damage
            );
        }
    }

    if (damage != 0.0f) {
        if (playerState->damageProtectionActive != 0 &&
            (hitSource == 0 || (hitSource->flags & kOptCatalogFlagBypassDamageProtection) == 0)) {
            playerState->statusMeterValue = 0.0f;
            pickupRewardMultiplier = 2;
        } else {
            playerState->statusMeterValue -= damage;
        }
    }
    playerState->statusMeterScaled = masterCommonData->invMaxHealth * playerState->statusMeterValue;

    if (playerState->statusMeterValue <= 0.0f) {
        playerState->lifecycleState = kPlayerLifecycleInactive;
        playerState->statusMeterValue = 0.0f;

        if (zSnd::GetAudioApiOption() == 1) {
            saveState->UpdateModalLoopSfx(0);
        }
        AINet::AiDiscardNegativeBranchPathNodes(saveState);
        StartDestroyedStateVehicleEffect(
            saveState,
            (void *)ClearDestroyedRespawnEffectHandleCallback
        );
        ResetAltGunRuntimeState(saveState);

        if (killEventContext != 0 && hitRenderPointEntry != 0) {
            OptCatalog::SetDamageContext(
                1,
                (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample)
            );
        }
        AddScaledHudCounterValue(masterCommonData->maxHealth);

        int spawnedNaniteReward = 0;
        zUtil_SaveGameState *const localSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
        if (localSaveState->playerState->nanitePanelLevel != kPlayerNanitePanelDisabledSentinel &&
            masterCommonData->naniteBuildRate != 0) {
            ++masterCommonData->naniteSpawnCounter;
            if (masterCommonData->naniteSpawnCounter >= masterCommonData->naniteBuildRate) {
                zVec3 spawnPos = playerState->worldPos;
                spawnPos.y -= masterModalData->modeAltTransitionTime;
                zClass_cls_di::SnapProbePointYToBestCandidate(&spawnPos);
                Pickup::SpawnAt(
                    34,
                    masterCommonData->naniteMaxLevel,
                    &spawnPos,
                    0,
                    0
                );
                masterCommonData->naniteSpawnCounter = 0;
                spawnedNaniteReward = 1;
            }
        }

        if (localSaveState->playerState->activeAltGunController->ammoOrCharge !=
                kPlayerAltAmmoDisabledSentinel &&
            spawnedNaniteReward == 0) {
            zVec3 spawnPos = playerState->worldPos;
            spawnPos.y -= masterModalData->modeAltTransitionTime;
            zClass_cls_di::SnapProbePointYToBestCandidate(&spawnPos);
            Pickup::SpawnAt(
                masterCommonData->pickupType,
                masterCommonData->pickupCapacity * pickupRewardMultiplier,
                &spawnPos,
                0,
                0
            );
        }

        ++g_HudSensorTracker.missionStat0;
        return 0;
    }

    DamageFeedback::SetIntensityScalar(
        masterCommonData->invMaxHealth * playerState->statusMeterValue
    );

    if (playerState->lifecycleState == kPlayerLifecycleAi &&
        playerState->aiTopLevelState != kPlayerAiMode2TopSteering) {
        AINet::AiEnterMode2SteeringPursuit(saveState);
        if (playerState->aiRuntime != 0 && playerState->aiRuntime->attackBuddyNetId != 0) {
            AINet::AiAlertAttackBuddies(saveState);
        }
        playerState->recentHitFlag = 1;
        playerState->recentHitExpireTime = g_Time_AccumulatedTimeSec + kPlayerRecentHitAlertSec;
    }

    if (killEventContext != 0) {
        OptCatalog::SetDamageContext(
            0,
            (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample)
        );
        if (damage > 5.0f &&
            (killEventContext->flags & kOptCatalogFlagAppliesTimedHitStatus) == 0 &&
            (killEventContext->flags & kOptCatalogFlagNoSubUse) == 0) {
            const float impulseBase = masterModalData->invMass * damage;
            const float angleScale = impulseBase * 0.0250000004f;
            const float velocityScale = impulseBase * 1.66700006f;
            const zVec3 *const sourcePos = OptCatalog::GetCapturedHitSourcePtr();
            const zVec3 *const hitPos = &g_OptCatalog_CapturedDamageHitPos;
            zVec3 direction = {sourcePos->x - hitPos->x,
                sourcePos->y - hitPos->y,
                sourcePos->z - hitPos->z};
            const float length = sqrt(
                direction.x * direction.x + direction.y * direction.y + direction.z * direction.z
            );
            const float invLength = 1.0f / length;
            direction.x *= invLength;
            direction.y *= invLength;
            direction.z *= invLength;
            ApplyPitchRollVelocityImpulseFromDirection(
                saveState,
                &direction,
                angleScale,
                velocityScale
            );
        }
    }

    return playerState->recentHitValid;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-enterlocalinactivedestroyedlifecycle
 * @recoil-artifact defines .text recoil:function:0x43bc40: Player::EnterLocalInactiveDestroyedLifecycle
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: put the local player into the inactive destroyed lifecycle and, for
 * network games, attach the destroyed reset callback to the respawn effect.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: body is gated to the active local save state, updates lifecycle and
 * alt-gun transition fields, starts the destroyed respawn effect, stops BFT
 * bubble FX, and installs the reset callback only when networking is enabled.
 */
void __fastcall EnterLocalInactiveDestroyedLifecycle(
    zUtil_SaveGameState *saveState
) {
    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->lifecycleState = kPlayerLifecycleInactive;
    playerState->altGunTransitionState = 1;
    playerState->altGunTransitionController = 0;
    playerState->altGunTransitionTimerA = 0.0f;

    zEffectAnimEntry *const destroyedRespawnHandle = zEffectAnim::SetVelocity_Thunk(
        playerState->destroyedRespawnFxEntry,
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    StopBftBubbleFxHandle(saveState);

    if (zOpt::GetNetworkEnabled() != 0) {
        playerState->cameraTransitionTimer = 1;
        zEffectAnimEntry::SetOnStateDoneCallback(
            destroyedRespawnHandle,
            (void *)(&DestroyedStateResetCallback),
            saveState
        );
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-enterdestroyedstate
 * @recoil-artifact defines .text recoil:function:0x43bcc0: Player::EnterDestroyedState
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: apply local damage, transition the local player into destroyed or
 * inactive lifecycle state, and emit hit feedback, network kill, and impact
 * side effects.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: source-owner packet groups this with the hit callbacks and
 * destroyed-state helpers; body preserves the damage suppression gates,
 * recent-hit/timed-status updates, status meter and nanite handling,
 * camera/steering reset, local lifecycle transition, network kill attribution,
 * damage context, impulse, force-feedback, and hit stamp behavior.
 */
int __fastcall EnterDestroyedState(
    zUtil_SaveGameState *saveState,
    OptCatalogEntryDef *hitSource,
    OptCatalogHitEventPartial *hitRenderPoint,
    float damage
) {
    OptCatalogEntryDef *killEventContext = hitSource;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->transitionDamageSuppressed != 0 ||
        playerState->lifecycleState == kPlayerLifecycleInactive ||
        playerState->lifecycleState == kPlayerLifecycleDestroyed) {
        return 0;
    }

    if (hitSource != 0) {
        if ((hitSource->flags & kOptCatalogFlagRecordsRecentHit) != 0) {
            RecordRecentHitFeedback(
                saveState,
                hitSource,
                damage
            );
        }
        if ((hitSource->flags & kOptCatalogFlagAppliesTimedHitStatus) != 0) {
            damage = UpdateTimedHitStatusFromHitSource(
                saveState,
                hitSource,
                damage
            );
        }
    }

    if (damage != 0.0f) {
        if (playerState->damageProtectionActive != 0 &&
            (hitSource == 0 || (hitSource->flags & kOptCatalogFlagBypassDamageProtection) == 0)) {
            damage = masterCommonData->maxHealth;
        }

        ApplyStatusMeterChange(
            saveState,
            1,
            -damage
        );
        if (g_PlayerStatusMeterRatio <= 0.0f) {
            const int nanitePanelLevel = playerState->nanitePanelLevel;
            if (nanitePanelLevel != 0 && nanitePanelLevel != kPlayerNanitePanelDisabledSentinel) {
                playerState->nanitePanelLevel = nanitePanelLevel - 1;
                HudUiMgr::SetNanitePanelCount(nanitePanelLevel - 1);
            }
            UpdateStatusMeter(
                saveState,
                0,
                0.0f
            );
        }
    }

    playerState->statusMeterScaled = 1.0f;
    if (playerState->statusMeterValue <= 0.0f) {
        if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            return 0;
        }

        saveState->UpdateModalLoopSfx(0);
        if (playerState->cameraState == kPlayerCameraStateProjectileAttached) {
            ApplyCameraState(kPlayerCameraStateRestorePrevious);
        }
        g_PlayerPrevSteeringMode = zOpt::GetSteeringMode();
        g_PlayerPrevCameraState = playerState->cameraState;
        zOpt::SetSteeringMode(kPlayerCameraStateThirdPerson);
        ApplyCameraState(kPlayerCameraStateThirdPerson);
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x23e),
            5.0f
        );
        playerState->statusMeterValue = 0.0f;
        g_HudSensorTracker.menuTransitionDelaySec = g_Time_AccumulatedTimeSec;
        ResetAltGunRuntimeState(saveState);

        if (hitSource != 0 && hitRenderPoint != 0) {
            OptCatalog::SetDamageContext(
                1,
                (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample)
            );
        }

        if (playerState->airborneFlag != 0) {
            playerState->lifecycleState = kPlayerLifecycleDestroyed;
        } else {
            EnterLocalInactiveDestroyedLifecycle(saveState);
        }

        if (zOpt::GetNetworkEnabled() != 0) {
            void *ownerOrCtx = 0;
            if (hitRenderPoint != 0) {
                ownerOrCtx = HitContext::GetCurrentOwnerOrCtx();
            } else if (playerState->recentHitValid != 0) {
                ownerOrCtx = playerState->lastHitOwnerOrCtx;
                killEventContext = playerState->recentHitSource;
            }

            HitOwnerOrContextPartial *const hitOwner = (HitOwnerOrContextPartial *)(ownerOrCtx);
            if (hitOwner != 0 && hitOwner->ownerLink != 0 &&
                hitOwner->ownerLink->ownerSaveState != 0) {
                GameNet::SendPkt08_PlayerKillEvent(
                    hitOwner->ownerLink->ownerSaveState,
                    (short)(killEventContext->ordinalIndex)
                );
            } else {
                GameNet::SendPkt08_PlayerKillEvent(
                    saveState,
                    0
                );
            }
        }

        HudLowMeterLoopSound::Disable();
        return 0;
    }

    if (hitSource != 0) {
        OptCatalog::SetDamageContext(
            0,
            (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample)
        );
        const unsigned int flags = hitSource->flags;
        if ((flags & kOptCatalogFlagAppliesTimedHitStatus) == 0) {
            if ((flags & kOptCatalogFlagNoSubUse) == 0) {
                const zVec3 *const sourcePos = OptCatalog::GetCapturedHitSourcePtr();
                const zVec3 *const hitPos = &g_OptCatalog_CapturedDamageHitPos;
                zVec3 direction = {sourcePos->x - hitPos->x,
                    sourcePos->y - hitPos->y,
                    sourcePos->z - hitPos->z};
                const float length = sqrt(
                    direction.x * direction.x + direction.y * direction.y +
                    direction.z * direction.z
                );
                const float invLength = 1.0f / length;
                direction.x *= invLength;
                direction.y *= invLength;
                direction.z *= invLength;

                if (damage > 5.0f) {
                    const float impulseBase = masterModalData->invMass * damage;
                    ApplyPitchRollVelocityImpulseFromDirection(
                        saveState,
                        &direction,
                        impulseBase * 0.00499999989f,
                        impulseBase * 0.333000004f
                    );
                }

                if (zInput_DI_IsForceFeedbackEnabled() != 0) {
                    g_zInputFfEffectSet->PlayDamageHitEffect(
                        &direction,
                        damage * 0.0500000007f
                    );
                }
            }

            if (hitRenderPoint != 0) {
                OptCatalog::ApplyDamageMaskStampOnHit(hitRenderPoint);
            }
        }
    }

    return playerState->recentHitValid;
}
/**
 * Source placement note: Player::ApplyDamageLocal is provisionally located here.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: finish local damage processing by updating feedback while health
 * remains, or by starting destroyed-respawn FX, clearing recent-hit feedback,
 * resetting alt-gun runtime state, and hiding the tracked HUD progress meter.
 * Source shape: Player damage-local subsystem helper linked to the damage-hit
 * and destroyed-state callback slice; this is not a whole Player class owner
 * and is not standalone.
 * Data: reads and writes the supplied save-state player's damage, respawn,
 * recent-hit, and selected-probe fields; delegates shared damage feedback,
 * damage context, alt-gun reset, and HUD progress state to their owners.
 */
int __fastcall ApplyDamageLocal(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (playerState->statusMeterValue > 0.0f) {
        DamageFeedback::SetIntensityScalar(
            playerState->masterCommonData->invMaxHealth * playerState->statusMeterValue
        );
        return 0;
    }

    zEffectAnimEntry *const destroyedRespawnHandle = zEffectAnim::SetVelocity_Thunk(
        playerState->destroyedRespawnFxEntry,
        playerState->rootNode,
        0.0f,
        0.0f,
        0.0f
    );
    playerState->destroyedRespawnAsyncHandle = destroyedRespawnHandle;
    zEffectAnimEntry::SetOnStateDoneCallback(
        destroyedRespawnHandle,
        (void *)(&DestroyedStateRespawnCallback),
        saveState
    );

    if (playerState->recentHitValid != 0) {
        zEffect_Anim::NodeActionCallback(
            playerState->recentHitLightHandle,
            0
        );
        playerState->recentHitLightHandle = 0;
        playerState->recentHitValid = 0;
    }

    ResetAltGunRuntimeState(saveState);
    OptCatalog::SetDamageContext(
        1,
        (OptCatalogHitEventPartial *)(void *)(&playerState->selectedProbeSample)
    );
    HudUiMgr::HideTrackedProgressMeterIfOwnerMatches(saveState);
    return 1;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-startdestroyedstatevehicleeffect
 * @recoil-artifact defines .text recoil:function:0x43c0c0: Player::StartDestroyedStateVehicleEffect
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: choose and start the destroyed-state vehicle effect, clear recent
 * hit feedback, and optionally install the respawn completion callback.
 * Source owner: Player damage-hit and destroyed-state callback subsystem, not
 * a C++ Player class owner.
 * Evidence: body selects the effect from fixed damage, damage protection,
 * recent-hit, AI, and default destroyed-state conditions; stores the async
 * handle, clears recent-hit light state, installs the provided callback, and
 * hides the tracked HUD progress meter for the save state.
 */
void __fastcall StartDestroyedStateVehicleEffect(
    zUtil_SaveGameState *saveState,
    void *respawnCallback
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zEffectAnimEntry *vehicleEffect;
    zClass_NodePartial *rootNode;

    playerState->destroyedRespawnAsyncHandle = 0;
    if (playerState->queuedFixedDamageFlag != 0) {
        vehicleEffect = playerState->shockVehicleFxEntry;
        rootNode = 0;
    } else if (playerState->damageProtectionActive != 0) {
        vehicleEffect = playerState->shatterVehicleFxEntry;
        rootNode = playerState->rootNode;
    } else if (playerState->recentHitValid != 0) {
        vehicleEffect = playerState->napalmVehicleFxEntry;
        rootNode = playerState->rootNode;
    } else if (playerState->aiMode != 0) {
        vehicleEffect = playerState->subTransitionFxEntry;
        rootNode = playerState->rootNode;
    } else {
        vehicleEffect = playerState->destroyedRespawnFxEntry;
        rootNode = playerState->rootNode;
    }

    zEffectAnimEntry *const asyncHandle =
        zEffectAnim::SetVelocity_Thunk(
            vehicleEffect,
            rootNode,
            0.0f,
            0.0f,
            0.0f
        );
    playerState->destroyedRespawnAsyncHandle = asyncHandle;

    if (playerState->recentHitValid != 0) {
        zEffect_Anim::NodeActionCallback(
            playerState->recentHitLightHandle,
            0
        );
        playerState->recentHitLightHandle = 0;
        playerState->recentHitValid = 0;
    }

    if (respawnCallback != 0) {
        zEffectAnimEntry::SetOnStateDoneCallback(
            asyncHandle,
            respawnCallback,
            saveState
        );
    }

    HudUiMgr::HideTrackedProgressMeterIfOwnerMatches(saveState);
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-processaltgundispatchrequest
 * @recoil-artifact defines .text recoil:function:0x43c190: Player::ProcessAltGunDispatchRequest
 * BN source path: D:\Proj\GameZRecoil\zWeapon.cpp.
 * Purpose: dispatch an alternate-gun fire request through effect, trail, or
 * projectile handling.
 */
void __fastcall ProcessAltGunDispatchRequest(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;
    PlayerGunFireSlot *activeFireSlot = 0;
    SelectAltGunFirePointAndSlot(
        saveState,
        &activeFireSlot
    );

    if (playerState->altGunFireHeldFlag != 0) {
        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            ++g_HudSensorTracker.primaryGunDispatchCount;
        }
        return;
    }

    playerState->altGunDispatchRequested = 0;
    if (activeAltGunController->ammoOrCharge > 0.0f) {
        int didFire = 0;
        if (playerState->activeAltBankIndex == 1) {
            didFire = EnsureGunAuxEffectActive(
                saveState,
                activeAltGunController,
                &playerState->altFireOrigin
            );
        } else if ((activeAltGunController->optCatalogEntry->flags & kOptCatalogFlagCreateTrail) != 0) {
            UpdateContinuousAltGunFireController(saveState);
            didFire = activeFireSlot != 0;
        } else {
            if (activeAltGunController->attachState != 0) {
                didFire = AltGunLaunchProjectile(saveState);
            } else {
                didFire = AltGunFireSimpleProjectile(saveState);
            }

            if (didFire != 0 && saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
                zInput_DI_IsForceFeedbackEnabled() != 0) {
                zInput_DI_PlayAltFireEffect(
                    g_zInputFfEffectSet,
                    activeAltGunController->optCatalogEntry->damage * 0.0151515156f
                );
            }
        }

        if (didFire == 0) {
            return;
        }

        if ((activeAltGunController->flags & 1) != 0 &&
            activeAltGunController->attachNodePrimary != 0) {
            activeFireSlot->offset = 1.5f;
        }

        if (activeAltGunController->ammoOrCharge != kPlayerAltAmmoDisabledSentinel) {
            activeAltGunController->ammoOrCharge -= 1.0f;
            if (activeAltGunController->ammoOrCharge < 0.0f) {
                activeAltGunController->ammoOrCharge = 0.0f;
            }
        }

        if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
            ++g_HudSensorTracker.primaryGunDispatchCount;
        }
        return;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        OptCatalog::PlayTriggerInactiveWarning();
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-updatecontinuousaltgunfirecontroller
 * @recoil-artifact defines .text recoil:function:0x43c2d0: Player::UpdateContinuousAltGunFireController
 * BN source path: D:\Proj\GameZRecoil\zWeapon.cpp.
 * Purpose: tick continuous alternate-gun trail state for the active
 * controller.
 */
void __fastcall UpdateContinuousAltGunFireController(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;

    if (saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub) {
        playerState->queuedFixedDamageFlag = 1;
        return;
    }

    if (playerState->altGunFireHeldFlag == 0) {
        const int playerOrdinal = playerState->playerOrdinal;
        playerState->altGunFireHeldFlag = 1;
        OptCatalog::ActivateTrailRuntimeState(
            activeAltGunController->trailRuntimeState,
            playerOrdinal
        );
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        ++g_HudSensorTracker.primaryGunDispatchCount;
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-ensuregunauxeffectactive
 * @recoil-artifact defines .text recoil:function:0x43c330: Player::EnsureGunAuxEffectActive
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: ensure an auxiliary muzzle effect exists and is positioned for
 * the selected gun controller.
 */
int __fastcall EnsureGunAuxEffectActive(
    zUtil_SaveGameState *saveState,
    PlayerGunFireController *gunController,
    zVec3 *effectPos
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    zVec3 spawnDir = {0};
    if (playerState->usePresetGunFireDir != 0) {
        spawnDir = playerState->gunFireDir;
    } else {
        spawnDir.x = playerState->storedTargetPos.x - effectPos->x;
        spawnDir.y = playerState->storedTargetPos.y - effectPos->y;
        spawnDir.z = playerState->storedTargetPos.z - effectPos->z;

        const float length = (float)(sqrt(
            spawnDir.x * spawnDir.x + spawnDir.y * spawnDir.y + spawnDir.z * spawnDir.z
        ));
        const float invLength = 1.0f / length;
        spawnDir.x *= invLength;
        spawnDir.y *= invLength;
        spawnDir.z *= invLength;
    }

    if (OptCatalog::AllocRuntimeInstance(
            gunController->optCatalogEntry,
            playerState->rootNode,
            &playerState->variantTag,
            effectPos,
            &spawnDir,
            &playerState->projectileSpawnVel,
            saveState,
            0
        ) == 0) {
        return 0;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        zInput_DI_IsForceFeedbackEnabled() != 0) {
        zInput_DI_RestartPrimaryFireEffect(g_zInputFfEffectSet);
    }

    return 1;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-altgunlaunchprojectile
 * @recoil-artifact defines .text recoil:function:0x43c430: Player::AltGunLaunchProjectile
 * BN source path: D:\Proj\GameZRecoil\zWeapon.cpp.
 * Purpose: launch an attached alternate-gun projectile from the active
 * controller.
 */
int __fastcall AltGunLaunchProjectile(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;

    if (saveState != (zUtil_SaveGameState *)g_GameStateOrMapTable &&
        (playerState->altGunTransitionState == 0x180 ||
            playerState->altGunTransitionState == 0x100)) {
        OptCatalog::RemoveRuntimeInstance(
            playerState->altWeaponBanks[8].controllerB.optCatalogEntry,
            0,
            playerState->rootNode
        );
        OptCatalog::RemoveRuntimeInstance(
            playerState->altWeaponBanks[9].controllerB.optCatalogEntry,
            0,
            playerState->rootNode
        );
        activeAltGunController->attachState =
            OptCatalog::AllocOrReuseAttachNodeClone(activeAltGunController->optCatalogEntry);
    } else {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)activeAltGunController->attachState;
        zClass_Class::RemoveChild(
            activeAltGunController->attachNodePrimary,
            attachState->projectileNode
        );
    }

    if (OptCatalog::AllocRuntimeInstance(
            activeAltGunController->optCatalogEntry,
            playerState->rootNode,
            &playerState->variantTag,
            &playerState->altFireOrigin,
            &playerState->gunFireDir,
            &playerState->projectileSpawnVel,
            saveState,
            (OptCatalogRuntimeInstanceStorage *)activeAltGunController->attachState
        ) == 0) {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)activeAltGunController->attachState;
        zClass_Class::AddChild(
            activeAltGunController->attachNodePrimary,
            attachState->projectileNode
        );
        return 0;
    }

    if ((activeAltGunController->optCatalogEntry->flags & kPlayerOptCatalogFlagTetherGuided) == 0) {
        activeAltGunController->attachState = 0;
        if (activeAltGunController->ammoOrCharge > 1.0f) {
            playerState->altGunTransitionState = 2;
        }
        playerState->altGunTransitionController = activeAltGunController;
        return 1;
    }

    if (saveState == (zUtil_SaveGameState *)g_GameStateOrMapTable) {
        playerState->pendingAltCameraToggle = 1;
    }
    playerState->altGunTransitionState = 0x100;
    playerState->altGunTransitionController = activeAltGunController;
    return 1;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-altgunfiresimpleprojectile
 * @recoil-artifact defines .text recoil:function:0x43c550: Player::AltGunFireSimpleProjectile
 * BN source path: D:\Proj\GameZRecoil\zWeapon.cpp.
 * Purpose: fire a simple alternate-gun projectile from the active fire
 * origin.
 */
int __fastcall AltGunFireSimpleProjectile(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;

    zVec3 spawnDir = {0};
    if (activeAltGunController->optCatalogEntry->gravity == 0.0f) {
        spawnDir.x = playerState->storedTargetPos.x - playerState->altFireOrigin.x;
        spawnDir.y = playerState->storedTargetPos.y - playerState->altFireOrigin.y;
        spawnDir.z = playerState->storedTargetPos.z - playerState->altFireOrigin.z;

        const float length = (float)(sqrt(
            spawnDir.x * spawnDir.x + spawnDir.y * spawnDir.y + spawnDir.z * spawnDir.z
        ));
        const float invLength = 1.0f / length;
        spawnDir.x *= invLength;
        spawnDir.y *= invLength;
        spawnDir.z *= invLength;
    } else {
        spawnDir = playerState->gunFireDir;
    }

    return OptCatalog::AllocRuntimeInstance(
               activeAltGunController->optCatalogEntry,
               playerState->rootNode,
               &playerState->variantTag,
               &playerState->altFireOrigin,
               &spawnDir,
               &playerState->projectileSpawnVel,
               saveState,
               0
           ) != 0;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-isaltweaponallowedincurrentmastermode
 * @recoil-artifact defines .text recoil:function:0x43c630: Player::IsAltWeaponAllowedInCurrentMasterMode.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::IsAltWeaponAllowedInCurrentMasterMode from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall IsAltWeaponAllowedInCurrentMasterMode(
    zUtil_SaveGameState *saveState,
    OptCatalogEntryDef *entry
) {
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        const unsigned int flags = entry->flags;
        if ((flags & kOptCatalogFlagBlockedInSub) != 0 || (flags & kOptCatalogFlagNoSubUse) != 0) {
            return 0;
        }
    }

    return 1;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-autoswitchtonextusablealtweapon
 * @recoil-artifact defines .text recoil:function:0x43c660: Player::AutoSwitchToNextUsableAltWeapon.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::AutoSwitchToNextUsableAltWeapon from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall AutoSwitchToNextUsableAltWeapon(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeController = playerState->activeAltGunController;
    int activeBankIndex = activeController->weaponBankIndex;
    if (activeBankIndex == 0) {
        return;
    }

    PlayerAltWeaponBank *bank = &playerState->altWeaponBanks[activeBankIndex];
    const int oppositeSideIndex = activeController->weaponSideIndex == 0;
    PlayerGunFireController *candidate = &bank->controllerA + oppositeSideIndex;
    if ((candidate->flags & 4) != 0 &&
        IsAltWeaponAllowedInCurrentMasterMode(
            saveState,
            candidate->optCatalogEntry
        ) != 0 &&
        candidate->ammoOrCharge > 0.0f) {
        HandleAltWeaponBankSelectInput(activeBankIndex + 14);
        return;
    }

    for (--activeBankIndex; activeBankIndex > 1; --activeBankIndex) {
        bank = &playerState->altWeaponBanks[activeBankIndex];
        if (((bank->controllerA.flags & 4) != 0 &&
             IsAltWeaponAllowedInCurrentMasterMode(
                 saveState,
                 bank->controllerA.optCatalogEntry
             ) != 0 &&
             bank->controllerA.ammoOrCharge > 0.0f) ||
            ((bank->controllerB.flags & 4) != 0 &&
             IsAltWeaponAllowedInCurrentMasterMode(
                 saveState,
                 bank->controllerB.optCatalogEntry
             ) != 0 &&
             bank->controllerB.ammoOrCharge > 0.0f)) {
            HandleAltWeaponBankSelectInput(activeBankIndex + 14);
            return;
        }
    }

    for (int nextBankIndex = activeController->weaponBankIndex + 1;
         nextBankIndex < 10;
         ++nextBankIndex) {
        bank = &playerState->altWeaponBanks[nextBankIndex];
        if (((bank->controllerA.flags & 4) != 0 &&
             IsAltWeaponAllowedInCurrentMasterMode(
                 saveState,
                 bank->controllerA.optCatalogEntry
             ) != 0 &&
             bank->controllerA.ammoOrCharge > 0.0f) ||
            ((bank->controllerB.flags & 4) != 0 &&
             IsAltWeaponAllowedInCurrentMasterMode(
                 saveState,
                 bank->controllerB.optCatalogEntry
             ) != 0 &&
             bank->controllerB.ammoOrCharge > 0.0f)) {
            HandleAltWeaponBankSelectInput(nextBankIndex + 14);
            return;
        }
    }
}
/**
 * Source placement note: Player::ResetAltGunDoorAnimationState is provisionally located here.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\player.cpp.
 * Purpose: reset the alternate-gun door animation timer and restore the
 * left/right door-node scale before runtime state reset.
 * Source shape: Player alt-gun runtime/reset dispatch subsystem member;
 * shares typed save-state/player-state field access with the accepted
 * alt-gun runtime functions and is not a standalone owner.
 * Data: updates only the supplied save-state player's transition timer and
 * door-node object scales; no authored globals are touched.
 */
void __fastcall ResetAltGunDoorAnimationState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->altGunTransitionTimerB = 0.0f;

    zClass_NodePartial *const doorLeftNode = playerState->doorLeftNode;
    if (doorLeftNode != 0) {
        zClass_Object3D::gwObject3DSetScale(
            doorLeftNode,
            1.0f,
            1.0f,
            1.0f
        );
    }

    zClass_NodePartial *const doorRightNode = playerState->doorRightNode;
    if (doorRightNode != 0) {
        zClass_Object3D::gwObject3DSetScale(
            doorRightNode,
            1.0f,
            1.0f,
            1.0f
        );
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-resetaltgunruntimestate
 * @recoil-artifact defines .text recoil:function:0x43c850: Player::ResetAltGunRuntimeState
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: clear active alternate-gun firing, attachment, door, and transition
 * runtime state before resetting the alternate weapon bank attachment nodes.
 */
void __fastcall ResetAltGunRuntimeState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;

    if (playerState->altGunFireHeldFlag != 0) {
        playerState->altGunFireHeldFlag = 0;
        OptCatalog::DeactivateTrailRuntimeState(activeAltGunController->trailRuntimeState);
    }

    OptCatalogRuntimeInstanceStorage *const attachState =
        (OptCatalogRuntimeInstanceStorage *)(activeAltGunController->attachState);
    if (attachState != 0) {
        zClass_Class::RemoveChild(
            activeAltGunController->attachNodePrimary,
            attachState->projectileNode
        );
        OptCatalog::RecycleRuntimeInstanceStorage(
            activeAltGunController->optCatalogEntry,
            attachState
        );
        activeAltGunController->attachState = 0;
    }

    ResetAltGunDoorAnimationState(saveState);
    playerState->timedHitStatus.ClearLightAndReset();
    playerState->altGunTransitionState = 1;
    playerState->altGunTransitionController = 0;
    playerState->altGunTransitionTimerA = 0.0f;

    PlayerAltWeaponBank *bank = &playerState->altWeaponBanks[2];
    for (int i = 0; i < 8; ++i, ++bank) {
        PlayerGunFireController *controller = &bank->controllerA;
        zClass_NodePartial *attachNode = controller->attachNodePrimary;
        if (attachNode != 0) {
            zClass_Class::gwNodeSetActive(attachNode, 0);
            zClass_Object3D::gwObject3DSetPosition(
                attachNode,
                controller->attachPosX,
                controller->attachPosY,
                controller->attachPosZ
            );
            zClass_Object3D::gwObject3DSetScale(
                attachNode,
                1.0f,
                1.0f,
                1.0f
            );
        }

        controller = &bank->controllerB;
        attachNode = controller->attachNodePrimary;
        if (attachNode != 0) {
            zClass_Class::gwNodeSetActive(attachNode, 0);
            zClass_Object3D::gwObject3DSetPosition(
                attachNode,
                controller->attachPosX,
                controller->attachPosY,
                controller->attachPosZ
            );
            zClass_Object3D::gwObject3DSetScale(
                attachNode,
                1.0f,
                1.0f,
                1.0f
            );
        }
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-removealldeployedmines
 * @recoil-artifact defines .text recoil:function:0x43c950: Player::RemoveAllDeployedMines
 * BN source path: D:\Proj\Battlesport\player.cpp.
 * Purpose: remove deployed mine runtime instances from banks 4/5 controller
 * A/B using the player root node.
 */
void __fastcall RemoveAllDeployedMines(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_NodePartial *const ownerNode = playerState->rootNode;

    OptCatalogEntryDef *entry = playerState->altWeaponBanks[4].controllerA.optCatalogEntry;
    if (entry != 0) {
        OptCatalog::RemoveRuntimeInstance(
            entry,
            0,
            ownerNode
        );
    }

    entry = playerState->altWeaponBanks[4].controllerB.optCatalogEntry;
    if (entry != 0) {
        OptCatalog::RemoveRuntimeInstance(
            entry,
            0,
            ownerNode
        );
    }

    entry = playerState->altWeaponBanks[5].controllerA.optCatalogEntry;
    if (entry != 0) {
        OptCatalog::RemoveRuntimeInstance(
            entry,
            0,
            ownerNode
        );
    }

    entry = playerState->altWeaponBanks[5].controllerB.optCatalogEntry;
    if (entry != 0) {
        OptCatalog::RemoveRuntimeInstance(
            entry,
            0,
            ownerNode
        );
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-findaltgunfirecontrollerforweaponid
 * @recoil-artifact defines .text recoil:function:0x43c9c0: Player::FindAltGunFireControllerForWeaponId
 * BN source path: D:\Proj\GameZRecoil\Player\player_weapon.c.
 * Purpose: select the alternate-gun fire controller matching the requested
 * weapon id.
 */
PlayerGunFireController *__fastcall FindAltGunFireControllerForWeaponId(
    zUtil_SaveGameState *saveState,
    int weaponId
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    OptCatalogEntryDef *const entry = OptCatalog::FindEntryById(weaponId);

    for (int i = 2; i < 10; ++i) {
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[i];
        if (bank.controllerA.optCatalogEntry == entry) {
            return &bank.controllerA;
        }

        if (bank.controllerB.optCatalogEntry == entry) {
            return &bank.controllerB;
        }
    }

    return &playerState->altWeaponBanks[1].controllerA;
}
} // namespace Player

namespace zWeapon_OptCatalog {
enum {
    kOptCatalogKillVerbStringBytes = 20,
    kOptCatalogKillVerbStringCopyLimit = kOptCatalogKillVerbStringBytes - 1
};
    /**
     * @recoil-anchor recoil:anchor:battlesport-weapon-zweapon-optcatalog-loadkillverbstring
     * @recoil-artifact defines .text recoil:function:0x43ca20: zWeapon_OptCatalog::LoadKillVerbString
     * Purpose: Allocate and populate the entry kill-verb string from the
     * optional KILL_VERB catalog node or default localized message.
     */
    void __fastcall LoadKillVerbString(
        zReader::Node * entryNode,
        OptCatalogEntryDef * entry
    ) {
        char *const killVerbString = (char *)(calloc(
            1,
            kOptCatalogKillVerbStringBytes
        ));
        entry->killVerbString = killVerbString;

        zReader::Node *const killVerbNode = zReader_GetNamedNode(
            entryNode,
            g_Player_KillVerbToken
        );
        const char *sourceText = 0;
        if (killVerbNode != 0) {
            sourceText = zLoc::ResolveMessageKeyOrFallback(
                killVerbNode->value.nodes[1].value.str
            );
        } else {
            sourceText = zLoc::GetMessageString(0x250);
        }

        strncpy(
            killVerbString,
            sourceText,
            kOptCatalogKillVerbStringCopyLimit
        );
    }
} // namespace zWeapon_OptCatalog

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-checkmissionweaponavailability
 * @recoil-artifact defines .text recoil:function:0x43ca90: Player::CheckMissionWeaponAvailability
 * BN source path: D:\Proj\GameZRecoil\Player\player_weapon.c.
 * Purpose: decide whether the current mission/network rules allow one packed
 * weapon bank/side slot, using the stack-local multiplayer whitelist.
 */
void __fastcall CheckMissionWeaponAvailability(
    zUtil_SaveGameState *saveState,
    int missionThreshold,
    int packedWeaponSlotId,
    int *availableOut
) {
    (void)saveState;

    const int currentMissionId = g_HudSensorTracker.GetMissionId();
    if (zOpt::GetNetworkEnabled() == 0) {
        *availableOut = missionThreshold != 0 && missionThreshold <= currentMissionId ? 1 : 0;
        return;
    }

    const int networkWhitelist[13][4] = {
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0, 0, 0, 0},
        {0x10, 0x11, 0x20, 0},
        {0x10, 0x11, 0x61, 0},
        {0x10, 0x11, 0x31, 0},
        {0x10, 0x11, 0x20, 0},
        {0x10, 0x11, 0x80, 0},
        {0x10, 0x11, 0x20, 0},
        {0x10, 0x11, 0x31, 0},
    };

    *availableOut = 0;
    const int *const row = networkWhitelist[currentMissionId - 1];
    for (int i = 0; i < 4; ++i) {
        if (packedWeaponSlotId == row[i]) {
            *availableOut = 1;
            return;
        }
    }
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-writemineszarsection
 * @recoil-artifact defines .text recoil:function:0x43cc70: Player::WriteMinesZarSection
 * BN source path: D:\Proj\GameZRecoil\Player\player_weapon.c.
 * Purpose: serialize deployed mine runtime instances for banks 4 and 5 into
 * the Mines ZAR section after an initial sentinel blob.
 */
int __fastcall WriteMinesZarSection(
    zZbdSectionCallbackCtx *writer,
    void *userData
) {
    (void)userData;

    PlayerMineSaveEntry data = {0};
    data.resetMarker = 1;
    strncpy(
        data.ownerNodeName,
        "Dummy",
        0x24
    );

    int writeOk = zUtil_ZAR::WriteSectionBlob(
        writer,
        "DummyMineData",
        &data,
        0x60
    );
    int mineCount = 0;
    for (int bankIndex = 4; writeOk != 0 && bankIndex < 6; ++bankIndex) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
        PlayerGunFireController *controllers[2] = {&bank.controllerA, &bank.controllerB};

        for (int sideIndex = 0; writeOk != 0 && sideIndex < 2; ++sideIndex) {
            OptCatalogEntryDef *const entry = controllers[sideIndex]->optCatalogEntry;
            if (entry == 0) {
                continue;
            }

            strncpy(
                data.optCatalogName,
                entry->keyName,
                0x20
            );
            OptCatalogRuntimeInstanceStorage *runtime = OptCatalog_MineIterator::Begin(entry);
            while (runtime != 0) {
                data.resetMarker = 0;
                data.spawnPos = runtime->pos;
                zClass_Object3D::gwObject3DGetScale(
                    runtime->projectileNode,
                    &data.scale.x,
                    &data.scale.y,
                    &data.scale.z
                );
                strncpy(
                    data.ownerNodeName,
                    zClass_Class::gwNodeGetName(runtime->ownerNode),
                    0x24
                );

                char blobToken[0x14];
                sprintf(
                    blobToken,
                    "MineData%03d",
                    mineCount
                );
                ++mineCount;
                writeOk = zUtil_ZAR::WriteSectionBlob(
                    writer,
                    blobToken,
                    &data,
                    0x60
                );
                runtime = OptCatalog_MineIterator::Next();
            }
        }
    }

    return writeOk;
}
/**
 * @recoil-anchor recoil:anchor:battlesport-weapon-player-mines-zar-readentryorreset
 * @recoil-artifact defines .text recoil:function:0x43cdf0: Player::Mines_ZAR_ReadEntryOrReset
 * BN source path: D:\Proj\GameZRecoil\Player\player_weapon.c.
 * Purpose: handle Mines ZAR blobs by clearing live mine runtimes on the
 * sentinel record or respawning one saved mine at its stored owner node.
 */
void __fastcall Mines_ZAR_ReadEntryOrReset(
    zZbdSectionCallbackCtx *,
    const char *,
    PlayerMineSaveEntry *mineData,
    unsigned int,
    void *
) {
    if (mineData->resetMarker != 0) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        for (int bankIndex = 4; bankIndex < 6; ++bankIndex) {
            PlayerAltWeaponBank &bank = playerState->altWeaponBanks[bankIndex];
            PlayerGunFireController *controllers[2] = {&bank.controllerA, &bank.controllerB};
            for (int sideIndex = 0; sideIndex < 2; ++sideIndex) {
                OptCatalogEntryDef *const entry = controllers[sideIndex]->optCatalogEntry;
                if (entry != 0) {
                    OptCatalog::ClearRuntimeInstances(entry);
                }
            }
        }
        return;
    }

    OptCatalogEntryDef *const entry = OptCatalog::FindEntryByName(mineData->optCatalogName);
    zClass_NodePartial *const ownerNode = zClass::FindByTypeAndName(
        6,
        mineData->ownerNodeName
    );
    if (entry != 0 && ownerNode != 0) {
        OptCatalogRuntimeInstanceStorage *const runtime =
            OptCatalog::SpawnRuntimeInstanceAt(
                entry,
                &mineData->spawnPos,
                ownerNode
            );
        zClass_Object3D::gwObject3DSetScale(
            runtime->projectileNode,
            mineData->scale.x,
            mineData->scale.y,
            mineData->scale.z
        );
    }
}

/* Deliberately after all selected rows: raw-extra diagnostic only. */
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x43c660 Player::AutoSwitchToNextUsableAltWeapon.
 * Purpose: provide the recovered is usable alt weapon controller helper for
 * the Player/Pickup gameplay source cluster.
 */
static int IsUsableAltWeaponController(
    zUtil_SaveGameState *saveState,
    PlayerGunFireController *controller
) {
    return (controller->flags & 4) != 0 &&
           IsAltWeaponAllowedInCurrentMasterMode(
               saveState,
               controller->optCatalogEntry
           ) != 0 &&
           controller->ammoOrCharge > 0.0f;
}
} // namespace Player
