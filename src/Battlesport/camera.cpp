// Player camera compilation unit, inferred from the retail code and literal
// contribution boundary. Original filename/path unresolved. See
// docs/reconstruction/audits/camera_byte_match_2026-09-10.md.

#include "recoil/Mfc42Abi.h"
#include "Battlesport/hud.h"

#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/zstr.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zFMV/fmv.h"

#include "GameZRecoil/zInterp/zInterp.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "Battlesport/wol_download.h"

#include <math.h>
#include <new>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


namespace Player {

enum HudPhysicalPlayerMasterTypeId {
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3
};

enum HudPhysicalPlayerCameraState {
    kPlayerCameraStateToggleRequest = 0,
    kPlayerCameraStateThirdPerson = 1,
    kPlayerCameraStateClearScreen = 2,
    kPlayerCameraStateFirstPerson = 3,
    kPlayerCameraStateTargeting = 4,
    kPlayerCameraStateProjectileAttached = 7,
    kPlayerCameraStateRestorePrevious = 8
};
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.tickactivecamerastate
 * @recoil-artifact defines .text recoil:function:0x404e90: Player::TickActiveCameraState.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-direction
 *
 * Purpose: Update the active player camera and its cached direction.
 */
void __fastcall TickActiveCameraState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    g_Player_CameraVariantUpdatedThisTick = 0;

    if (g_Player_RebuildCameraDirFlatFromCurrentTarget != 0) {
        zVec3 targetWorldPos = playerState->worldPos;
        zVec3 activeCameraTarget;
        CZCamera::gwCameraGetTarget(
            g_MainCamera,
            &activeCameraTarget.x,
            &activeCameraTarget.y,
            &activeCameraTarget.z
        );

        playerState->cameraTargetDistance =
            zMath::Vec3DeltaLength(&activeCameraTarget, &targetWorldPos);

        targetWorldPos.y += playerState->cameraYOffset;
        ZMTH_VECTOR_DIRECTION(&playerState->cameraDirFlat, &activeCameraTarget, &targetWorldPos);
        g_Player_RebuildCameraDirFlatFromCurrentTarget = 0;
    }

    switch (playerState->cameraState) {
    case 1:
        UpdateChaseCameraFromInput(saveState);
        break;
    case kPlayerCameraStateClearScreen:
        UpdateTopDownCameraState(saveState);
        break;
    case kPlayerCameraStateFirstPerson:
        UpdateFirstPersonCameraFromInput(saveState);
        break;
    case kPlayerCameraStateTargeting:
        UpdateThirdPersonCamera(saveState);
        break;
    case 5:
        UpdateCameraState5(saveState);
        break;
    case 6:
        UpdateCameraFromStoredTargetTowardPlayer(saveState);
        break;
    case kPlayerCameraStateProjectileAttached:
        RestoreThirdPersonCameraFromObstructionState(saveState);
        break;
    }

    if (g_Player_CameraVariantUpdatedThisTick == 0) {
        UpdateCameraVariantFromCameraPos(saveState, &playerState->cameraTarget);
    }

    UpdateCameraWeatherFxEmitterVisibility();

    if (playerState->cameraState != kPlayerCameraStateClearScreen) {
        playerState->cameraBasisCache = playerState->cameraDirNext;
    } else {
        playerState->cameraBasisCache = playerState->steerBasisNorm;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatechasecamerafrominput
 * @recoil-artifact defines .text recoil:function:0x405040: Player::UpdateChaseCameraFromInput.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-direction
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-xz
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot-xz
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-match byte
 *
 * Purpose: Update the player chase camera from controls, motion, and obstructions.
 * Shared camera scalars require the complete camera consumer population.
 */
void __fastcall UpdateChaseCameraFromInput(zUtil_SaveGameState *saveState) {
    const double kVerticalSpeedCameraInputCutoff = 11.0;
    const float kCameraElevationInputScale = -8.0f;
    const float kCameraVelocitySwingScale = -0.0900000036f;
    const float kCameraElevationBaseClearance = 0.5f;
    const float kCameraHeadingDotEpsilon = 0.0000999999975f;
    const float kCameraDistanceDampingRate = -6.0f;
    const float kTrackYOffsetDampingRate = 5.0f;
    const float kNonTrackYOffsetDampingRate = 3.0f;
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    const float maxCamYawRate = g_Player_MaxCamYawRate;
    const float cameraZone = g_Player_CameraZone;
    const float cameraZoneInvRange = g_Player_CameraZoneInvRange;
    zInput::MouseStateSnapshot mouseState;
    if (zOpt::GetCursorMode() != 0) {
        memcpy(&mouseState, zInput::MouseGetStateSnapshotPtr(), sizeof(mouseState));
    }
    float cameraAdjustment = 0.0f;
    if (playerState->joyCameraYawInput != 0.0f) {
        cameraAdjustment = maxCamYawRate * g_FrameDeltaTimeSec * playerState->joyCameraYawInput;
    } else if (zOpt::GetSteeringMode() != 0) {
        if (zOpt::GetCursorMode() != 0) {
            if (playerState->cursorDeltaX == 0.0f && mouseState.deltaX != 0) {
                cameraAdjustment = (float)(mouseState.deltaX) * g_Player_MousePushX;
            }
        } else if (playerState->cursorNormX > cameraZone) {
            cameraAdjustment = (playerState->cursorNormX - cameraZone) * cameraZoneInvRange *
                       maxCamYawRate * g_FrameDeltaTimeSec;
        } else if (playerState->cursorNormX < -cameraZone) {
            cameraAdjustment = (cameraZone + playerState->cursorNormX) * cameraZoneInvRange *
                       maxCamYawRate * g_FrameDeltaTimeSec;
        }
    }
    playerState->thirdPersonYawOffset += cameraAdjustment;
    const float verticalCameraZoneInvRange = -cameraZoneInvRange;
    if (fabs(playerState->localVel.z) < kVerticalSpeedCameraInputCutoff) {
        if (zOpt::GetCursorMode() != 0) {
            if (playerState->cursorDeltaY == 0.0f && mouseState.deltaY != 0) {
                playerState->cameraElevationOffset -= (float)(mouseState.deltaY) * g_Player_MousePushY;
            }
        } else if (playerState->cursorNormY > cameraZone) {
            playerState->cameraElevationOffset -= (playerState->cursorNormY - cameraZone) * verticalCameraZoneInvRange *
                             g_FrameDeltaTimeSec * kCameraElevationInputScale;
        } else if (playerState->cursorNormY < -cameraZone) {
            playerState->cameraElevationOffset -= (cameraZone + playerState->cursorNormY) * verticalCameraZoneInvRange *
                             g_FrameDeltaTimeSec * kCameraElevationInputScale;
        }
    }

    const zVec3 cameraBackOffset = playerState->cameraBackOffset;

    float horizontalProjectileSpeed;
    ZMTH_VECTOR_LENGTH_XZ(horizontalProjectileSpeed, &playerState->projectileSpawnVel);
    float speedSwingFactor;
    speedSwingFactor = zMath::FastExp(horizontalProjectileSpeed * kCameraVelocitySwingScale);
    const float maxElevationOffset = masterCommonData->cameraUdSwing[0] * speedSwingFactor;
    const float baseElevationLimit = cameraBackOffset.y - kCameraElevationBaseClearance;
    const double upperElevationLimit =
        baseElevationLimit < maxElevationOffset ? baseElevationLimit : maxElevationOffset;

    if (playerState->cameraElevationOffset > upperElevationLimit) {
        playerState->cameraElevationOffset = upperElevationLimit;
    } else {
        const double lowerElevationLimit = -maxElevationOffset;
        if (playerState->cameraElevationOffset < lowerElevationLimit) {
            playerState->cameraElevationOffset = (float)lowerElevationLimit;
        }
    }

    const float headingLerpRate =
        (playerState->slipSfxActive != 0 ? g_Player_CameraHeadingLerpBaseWhenFlagSet
                                       : g_Player_CameraHeadingLerpBaseWhenFlagClear) +
        1.0f / (g_Player_CameraHeadingDotAbs + kCameraHeadingDotEpsilon);
    zVec3 cameraScratch = playerState->steerBasisNorm;
    float headingBlend;
    headingBlend = zMath::FastExp(-headingLerpRate * g_FrameDeltaTimeSec);
    zMath::Vec3LerpNormalize(&playerState->cameraDirFlat, &cameraScratch, headingBlend);
    float headingDot;
    ZMTH_VECTOR_DOT_XZ(headingDot, &cameraScratch, &playerState->cameraDirFlat);
    g_Player_CameraHeadingDotAbs = (float)fabs(headingDot);
    zVec3 cameraDirection;
    if (playerState->thirdPersonYawOffset != 0.0f) {
        float yawSin, yawCos;
        float unscaledCos, unscaledSin; // Unused saved values; these assignments reproduce the retail bytes.
        zMath::SinCos(playerState->thirdPersonYawOffset, &yawSin, &yawCos);
        cameraDirection.x = yawCos * playerState->cameraDirFlat.x - yawSin * playerState->cameraDirFlat.z;
        cameraDirection.z = (unscaledCos = yawCos) * playerState->cameraDirFlat.z + (unscaledSin = yawSin) * playerState->cameraDirFlat.x;
    } else {
        cameraDirection = playerState->cameraDirFlat;
    }

    if (playerState->slipSfxActive == 0) {
        cameraAdjustment = cameraBackOffset.z - g_Player_CameraElastic * playerState->localVel.z;
    } else {
        cameraAdjustment = playerState->cameraTargetDistance;
    }

    float distanceBlend;
    distanceBlend = zMath::FastExp(g_FrameDeltaTimeSec * kCameraDistanceDampingRate);
    const float distanceInvBlend = 1.0f - distanceBlend;
    cameraAdjustment *= distanceInvBlend;
    playerState->cameraTargetDistance = cameraAdjustment + distanceBlend * playerState->cameraTargetDistance;

    zVec3 cameraOffset;
    cameraOffset.x =
        -cameraDirection.z * cameraBackOffset.x - cameraDirection.x * playerState->cameraTargetDistance;
    cameraOffset.z =
        cameraDirection.x * cameraBackOffset.x - cameraDirection.z * playerState->cameraTargetDistance;

    const float yOffsetRate = saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeTrack
                                  ? kTrackYOffsetDampingRate
                                  : kNonTrackYOffsetDampingRate;
    float yOffsetBlend;
    yOffsetBlend = zMath::FastExp(-(yOffsetRate * g_FrameDeltaTimeSec));
    const double yOffsetInvBlend = 1.0f - yOffsetBlend;
    float targetYOffset = (cameraOffset.x * playerState->steerBasisNorm.x +
                              playerState->steerBasisNorm.z * cameraOffset.z) *
                          playerState->steerBasisRaw.y;
    if (saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeTrack && targetYOffset <= 0.0f) {
        targetYOffset = 0.0f;
    }
    playerState->thirdPersonPositionYOffset =
        yOffsetInvBlend * targetYOffset + yOffsetBlend * playerState->thirdPersonPositionYOffset;

    zVec3 cameraPos;
    cameraPos.x = playerState->worldPos.x + cameraOffset.x;
    cameraPos.y = playerState->worldPos.y + cameraBackOffset.y +
                  playerState->thirdPersonPositionYOffset - playerState->cameraElevationOffset;
    cameraPos.z = playerState->worldPos.z + cameraOffset.z;

    cameraScratch = playerState->worldPos;
    cameraScratch.y += playerState->cameraYOffset;
    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        AdjustSubCameraFocusForObstruction(saveState, &cameraScratch);
    }

    ZMTH_VECTOR_DIRECTION(&playerState->cameraDirNext, &cameraPos, &cameraScratch);

    AdjustThirdPersonCameraBySideProbes(
        saveState,
        &cameraPos,
        &cameraScratch,
        &playerState->cameraDirNext
    );

    CZCamera::gwCameraSetTarget(g_MainCamera, cameraPos.x, cameraPos.y, cameraPos.z);
    const zVec3 cameraOrientation =
        zMath::Vec3DirectionAnglesBetweenPoints(&cameraPos, &cameraScratch);
    CZCamera::gwCameraSetPosition(
        g_MainCamera,
        cameraOrientation.x,
        cameraOrientation.y,
        cameraOrientation.z
    );

    playerState->cameraTarget = cameraPos;
    playerState->cameraDir = playerState->cameraDirNext;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatethirdpersoncamera
 * @recoil-artifact defines .text recoil:function:0x405650: Player::UpdateThirdPersonCamera
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-direction
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Purpose: update the third-person camera target, camera orientation, horizon
 * node, and cached direction vectors from the active player state.
 */
void __fastcall UpdateThirdPersonCamera(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 cameraTarget;
    zMath::Vec3Add(&playerState->worldPos, &playerState->cameraLerpStart, &cameraTarget);

    CZCamera::gwCameraSetTarget(g_MainCamera, cameraTarget.x, cameraTarget.y, cameraTarget.z);
    if (g_Player_HorizonNode != 0) {
        CZObject3D::gwObject3DSetPosition(
            g_Player_HorizonNode,
            cameraTarget.x,
            cameraTarget.y,
            cameraTarget.z
        );
    }

    zVec3 cameraLookAt = playerState->worldPos;
    cameraLookAt.y += playerState->cameraYOffset;

    const zVec3 cameraAngles =
        zMath::Vec3DirectionAnglesBetweenPoints(&cameraTarget, &cameraLookAt);
    CZCamera::gwCameraSetPosition(
        g_MainCamera,
        cameraAngles.x,
        cameraAngles.y,
        cameraAngles.z
    );

    ZMTH_VECTOR_DIRECTION(&playerState->cameraDirNext, &cameraTarget, &playerState->autoTurnTargetWorldPos);

    playerState->cameraTarget = cameraTarget;
    playerState->cameraDir = playerState->cameraDirNext;
    playerState->cameraDirFlat = playerState->cameraDirNext;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->cameraDirFlat, &playerState->cameraDirFlat);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.player.camera-state5
 * @recoil-artifact defines .text recoil:logical-function:0x4076f0:player-camera-state5: Inferred camera handler.
 *
 * Purpose: Preserve the empty update hook for player camera state 5.
 * The name is descriptive reconstruction spelling. This camera-cluster
 * implementation placement is provisional; the original TU, authored position,
 * and winner of the folded RET group are unknown.
 */
void __fastcall UpdateCameraState5(zUtil_SaveGameState *) {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatetopdowncamerastate
 * @recoil-artifact defines .text recoil:function:0x4057d0: Player::UpdateTopDownCameraState.
 * @recoil-match byte
 *
 * Purpose: Update the top-down camera target, fixed orientation and direction.
 * Initialize each coordinate from world position before adding its offset,
 * preserving the retail x87 operand order in the governed VC5 build.
 */
void __fastcall UpdateTopDownCameraState(zUtil_SaveGameState *saveState) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    float x = playerState->worldPos.x;
    x += playerState->cameraState2TargetOffset.x;
    playerState->cameraTarget.x = x;
    float y = playerState->worldPos.y;
    y += playerState->cameraState2TargetOffset.y;
    playerState->cameraTarget.y = y;
    float z = playerState->worldPos.z;
    z += playerState->cameraState2TargetOffset.z;
    playerState->cameraTarget.z = z;

    CZCamera::gwCameraSetTarget(
        g_MainCamera,
        playerState->cameraTarget.x,
        playerState->cameraTarget.y,
        playerState->cameraTarget.z
    );
    CZCamera::gwCameraSetPosition(g_MainCamera, -1.54999995f, 0.0f, 0.0f);
    playerState->cameraDir.x = 0.0f;
    playerState->cameraDir.y = -1.0f;
    playerState->cameraDir.z = 0.0f;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatecamerafromstoredtargettowardplayer
 * @recoil-artifact defines .text recoil:function:0x405870: Player::UpdateCameraFromStoredTargetTowardPlayer.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-direction
 * @recoil-match byte
 *
 * Purpose: implement Player::UpdateCameraFromStoredTargetTowardPlayer in the Battlesport camera subsystem.
 */
void __fastcall UpdateCameraFromStoredTargetTowardPlayer(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 lookAt = playerState->worldPos;
    lookAt.y += playerState->cameraState == kPlayerCameraStateThirdPerson
                    ? playerState->cameraYOffset
                    : playerState->cameraState6YOffset;

    zVec3 cameraTarget = playerState->cameraTarget;
    ZMTH_VECTOR_DIRECTION(&playerState->cameraDirNext, &cameraTarget, &lookAt);

    const zVec3 cameraAngles =
        zMath::Vec3DirectionAnglesBetweenPoints(&cameraTarget, &lookAt);
    CZCamera::gwCameraSetPosition(
        g_MainCamera,
        cameraAngles.x,
        cameraAngles.y,
        cameraAngles.z
    );

    playerState->cameraDir = playerState->cameraDirNext;
    playerState->cameraDirFlat = playerState->cameraDirNext;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->cameraDirFlat, &playerState->cameraDirFlat);
}
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatefirstpersoncamerafrominput
 * @recoil-artifact defines .text recoil:function:0x4059a0: Player::UpdateFirstPersonCameraFromInput.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-xz
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-direction
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Purpose: Update first-person camera elevation, target and direction from input.
 */
void __fastcall UpdateFirstPersonCameraFromInput(zUtil_SaveGameState *saveState) {
    const float kForwardSpeedClampThreshold = 10.0f;
    const float kForwardSpeedClampScale = -0.0153999999f;
    const float kElevationCameraPosScale = -0.349999994f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float cameraZone = g_Player_CameraZone;
    const float invertedCameraZoneInvRange = -g_Player_CameraZoneInvRange;

    if (zOpt::GetCursorMode() != 0) {
        zInput::MouseStateSnapshot mouseState = *zInput::MouseGetStateSnapshotPtr();
        if (playerState->cursorDeltaY == 0.0f && mouseState.deltaY != 0) {
            playerState->cameraElevationOffset -= (float)(mouseState.deltaY) * g_Player_MousePushY;
        }
    } else if (playerState->cursorNormY > cameraZone) {
        float elevationDelta = (playerState->cursorNormY - cameraZone) *
                               invertedCameraZoneInvRange * g_Player_FpCamElevationRate;
        playerState->cameraElevationOffset += elevationDelta * g_FrameDeltaTimeSec;
    } else if (playerState->cursorNormY < -cameraZone) {
        float elevationDelta = (cameraZone + playerState->cursorNormY) *
                               invertedCameraZoneInvRange * g_Player_FpCamElevationRate;
        playerState->cameraElevationOffset += elevationDelta * g_FrameDeltaTimeSec;
    }

    float elevationMin = g_Player_FpCamElevationMin;
    float elevationMax = g_Player_FpCamElevationMax;
    float forwardSpeed;
    ZMTH_VECTOR_LENGTH_XZ(forwardSpeed, &playerState->projectileSpawnVel);
    const float speedOverThreshold = forwardSpeed - kForwardSpeedClampThreshold;
    if (speedOverThreshold > 0.0f) {
        const float elevationScale = zMath::FastExp(speedOverThreshold * kForwardSpeedClampScale);
        float unscaledMin, unscaledMax; // Unused saved values; these assignments reproduce the retail bytes.
        elevationMin = (unscaledMin = elevationMin) * elevationScale;
        elevationMax = (unscaledMax = elevationMax) * elevationScale;
    }
    const float elevation = playerState->cameraElevationOffset;
    if (elevation > elevationMax) {
        playerState->cameraElevationOffset = elevationMax;
    } else if (playerState->cameraElevationOffset < elevationMin) {
        playerState->cameraElevationOffset = elevationMin;
    }

    const zMat4x3 &motionBasis = playerState->motionBasis;
    const zVec3 &localOffset = playerState->cameraState6LocalOffset;
    zVec3 cameraPoint = playerState->worldPos;
    zVec3 cameraLocalOffsetWorld;
    ZMTH_VECTOR_TRANSFORM_DIRECTION(&motionBasis, &cameraLocalOffsetWorld, &localOffset);
    zMath::Vec3Add(&cameraPoint, &cameraLocalOffsetWorld, &cameraPoint);

    CZCamera::gwCameraSetTarget(g_MainCamera, cameraPoint.x, cameraPoint.y, cameraPoint.z);
    playerState->cameraTarget = cameraPoint;

    cameraPoint = playerState->cameraState6BasePos;
    cameraPoint.x -= playerState->cameraElevationOffset * kElevationCameraPosScale;
    CZCamera::gwCameraSetPosition(
        g_MainCamera,
        cameraPoint.x,
        cameraPoint.y,
        cameraPoint.z
    );

    playerState->cameraDirNext = playerState->steerBasisRaw;
    playerState->cameraDirFlat = playerState->steerBasisRaw;
    playerState->cameraDir = playerState->steerBasisRaw;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.applycamerastate
 * @recoil-artifact defines .text recoil:function:0x405c90: Player::ApplyCameraState
 * @recoil-match byte
 *
 * Purpose: apply a requested player camera state while preserving previous
 * state and option flags for first-person, third-person, clear-screen, and
 * projectile views.
 * Read the active state again after callbacks before saving the prior mode.
 */
void __fastcall ApplyCameraState(int newState) {
    zUtil_SaveGameState *const saveState = g_CurrentPlayerSaveState;
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->cameraState == newState ||
        (playerState->cameraState == kPlayerCameraStateClearScreen &&
         newState == kPlayerCameraStateProjectileAttached)) {
        return;
    }

    if (playerState->cameraState == kPlayerCameraStateProjectileAttached &&
        newState != kPlayerCameraStateRestorePrevious) {
        ApplyCameraState(kPlayerCameraStateRestorePrevious);
    }

    if (newState == kPlayerCameraStateToggleRequest) {
        switch (playerState->cameraState) {
        case kPlayerCameraStateThirdPerson:
            newState = kPlayerCameraStateFirstPerson;
            break;
        case kPlayerCameraStateClearScreen:
            newState = kPlayerCameraStateRestorePrevious;
            break;
        case kPlayerCameraStateFirstPerson:
            newState = kPlayerCameraStateThirdPerson;
            break;
        default:
            break;
        }
    }

    switch (newState) {
    case kPlayerCameraStateThirdPerson:
        playerState->cameraLerpActive = 0;
        playerState->cameraTargetDistance = playerState->cameraBackOffset.z;
        if (playerState->cameraState == kPlayerCameraStateFirstPerson) {
            zOpt::SetSteeringMode(g_Player_SavedSteeringMode);
        }
        break;

    case kPlayerCameraStateClearScreen:
        zVideo::ExchangeClearScreenBufferEnabled(1);
        break;

    case kPlayerCameraStateFirstPerson:
        g_Player_SavedSteeringMode = zOpt::GetSteeringMode();
        zOpt::SetSteeringMode(0);
        playerState->cameraElevationOffset = 0.0f;
        break;

    case kPlayerCameraStateProjectileAttached: {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)(playerState->activeAltGunController->attachState);
        CZNodePartial *const projectileNode = attachState->projectileNode;
        CZCamera::gwCameraSetTarget(g_MainCamera, 0.0f, 1.0f, 1.0f);
        CZCamera::gwCameraSetPosition(g_MainCamera, 0.0f, 0.0f, 0.0f);
        CZClass::AddChild(projectileNode, g_MainCamera);
        CZObject3D::gwObject3DSetAlphaScale(projectileNode, 0.5f);
        CZObject3D::gwObject3DSetLitFlag(projectileNode, 1);
        ((HudUiElement *)(&g_Player_State7FxPass3Ui))->SetVisible(1);
        break;
    }

    case kPlayerCameraStateRestorePrevious:
        if (playerState->cameraState == kPlayerCameraStateProjectileAttached) {
            newState = playerState->previousCameraState;
            OptCatalogRuntimeInstanceStorage *const attachState =
                (OptCatalogRuntimeInstanceStorage *)(playerState->activeAltGunController
                        ->attachState);
            CZNodePartial *const projectileNode = attachState->projectileNode;
            CZClass::RemoveChild(projectileNode, g_MainCamera);
            CZObject3D::gwObject3DSetAlphaScale(projectileNode, 1.0f);
            CZObject3D::gwObject3DSetLitFlag(projectileNode, 0);
            UpdateThirdPersonCamera(g_CurrentPlayerSaveState);
            ((HudUiElement *)(&g_Player_State7FxPass3Ui))->SetVisible(0);
            zTag4::Clear(&g_VariantTag_Current);
            g_Variant_CurrentTag = g_VariantTag_Current;
        } else if (playerState->cameraState == kPlayerCameraStateClearScreen) {
            newState = playerState->previousCameraState;
            zVideo::ExchangeClearScreenBufferEnabled(0);
            UpdateThirdPersonCamera(g_CurrentPlayerSaveState);
        }
        break;

    default:
        break;
    }

    playerState->previousCameraState = playerState->cameraState;
    playerState->cameraState = newState;

    if (newState == kPlayerCameraStateThirdPerson) {
        zOpt::SetCameraMode(1);
    } else if (newState == kPlayerCameraStateFirstPerson) {
        zOpt::SetCameraMode(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.togglesteeringmodeandresetmouselook
 * @recoil-artifact defines .text recoil:function:0x405ec0: Player::ToggleSteeringModeAndResetMouseLook
 *
 * Purpose: Reset active mouse-look state and toggle the steering-mode option.
 * Source owner: battlesport_gameplay.player_camera_control_state_bridge,
 * not a C++ Player class and not the accepted player_camera.c source-file
 * owner.
 */
void ToggleSteeringModeAndResetMouseLook() {
    ResetMouseControlStateAndRecenterCursor((zUtil_SaveGameState *)g_GameStateOrMapTable);
    zOpt::SetSteeringMode(zOpt::GetSteeringMode() == 0 ? 1 : 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.adjustthirdpersoncamerabyoffsetprobes
 * @recoil-artifact defines .text recoil:function:0x405ee0: Player::AdjustThirdPersonCameraByOffsetProbes.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-match byte
 *
 * Purpose: implement Player::AdjustThirdPersonCameraByOffsetProbes in the Battlesport camera subsystem.
 */
int __fastcall AdjustThirdPersonCameraByOffsetProbes(
    zUtil_SaveGameState *saveState,
    zVec3 *cameraPos,
    const zVec3 *sideDir
) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kCameraSideProbeDistance = 2.0f;
    const float kSubVerticalProbeDistance = -2.0f;

    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    CZNodePartial *const rootNode = saveState->playerState->rootNode;

    int result = 0;
    zVec3 perpDir;
    zMath::Vec3PerpXZ(sideDir, &perpDir);
    zVec3 normalizedPerp;
    zMath::Vec3NormalizeXZ(&perpDir, &normalizedPerp);
    normalizedPerp.y = 0.0f;

    const zVec3 sideOffset = {
        normalizedPerp.x * kCameraSideProbeDistance,
        0.0f,
        normalizedPerp.z * kCameraSideProbeDistance,
    };

    CZDisplayInstanceSegmentEndpoints segmentPairs[3];
    segmentPairs[1].start = *cameraPos;
    segmentPairs[0].start = *cameraPos;
    zMath::Vec3Add(cameraPos, &sideOffset, &segmentPairs[0].end);
    zMath::Vec3Subtract(cameraPos, &sideOffset, &segmentPairs[1].end);

    int endpointCount;
    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        endpointCount = 6;
        segmentPairs[2].end = *cameraPos;
        segmentPairs[2].start = *cameraPos;
        segmentPairs[2].end.y -= kSubVerticalProbeDistance;
    } else {
        endpointCount = 4;
    }

    CZDisplayInstance::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);
    CZClass::gwNodeSetRaycastable(rootNode, 0);

    PlayerProbeSampleCandidateBuffer probeBatches[3];
    CZDisplayInstance::BuildProbeHitBatchesForSegments(
        g_Player_RuntimeDiScene,
        segmentPairs,
        endpointCount,
        probeBatches
    );

    CZClass::gwNodeSetRaycastable(rootNode, 1);
    FilterCameraProbeBlockingHits(probeBatches, endpointCount >> 1);

    zVec3 outHitPos;
    if (FindNearestThirdPersonCameraProbePoint(probeBatches, 1, cameraPos, &outHitPos) != 0) {
        result = 1;
        cameraPos->x += outHitPos.x - segmentPairs[0].end.x;
        cameraPos->z += outHitPos.z - segmentPairs[0].end.z;
    } else if (FindNearestThirdPersonCameraProbePoint(&probeBatches[1], 1, cameraPos, &outHitPos) !=
               0) {
        result = 1;
        cameraPos->x += outHitPos.x - segmentPairs[1].end.x;
        cameraPos->z += outHitPos.z - segmentPairs[1].end.z;
    }

    if (masterModalData->masterType == kPlayerMasterTypeSub &&
        FindNearestThirdPersonCameraProbePoint(&probeBatches[2], 1, cameraPos, &outHitPos) != 0) {
        result |= 1;
        cameraPos->y += outHitPos.y - segmentPairs[2].end.y;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.adjustthirdpersoncamerabysideprobes
 * @recoil-artifact defines .text recoil:function:0x406110: Player::AdjustThirdPersonCameraBySideProbes.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-direction
 *
 *
 * Purpose: implement Player::AdjustThirdPersonCameraBySideProbes in the Battlesport camera subsystem.
 */
int __fastcall AdjustThirdPersonCameraBySideProbes(
    zUtil_SaveGameState *saveState,
    zVec3 *cameraPos,
    const zVec3 *focusPos,
    zVec3 *cameraDirNext
) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kSubCameraProbeHeightOffset = 2.20000005f;
    const float kCameraPickMaxY = 500.0f;
    const float kCameraPickRiseWindow = 0.00100000005f;
    const float kCameraFloorOffset = -0.5f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    CZNodePartial *const rootNode = playerState->rootNode;
    const zTag4Partial savedVariantTag = g_Variant_CurrentTag;
    int cameraAdjusted = 0;

    zTag4::Clear(&g_Variant_CurrentTag);

    float negativeScale;
    zVec3 sideProbeOffset = {
        (negativeScale = -g_Player_ThirdPersonCameraSideProbeOffsetScale) * cameraDirNext->x,
        negativeScale * cameraDirNext->y,
        negativeScale * cameraDirNext->z,
    };
    const zVec3 sideProbeEndpoint = {
        cameraPos->x + sideProbeOffset.x,
        cameraPos->y + sideProbeOffset.y,
        cameraPos->z + sideProbeOffset.z,
    };

    CZDisplayInstanceSegmentEndpoints segmentPairs[2];
    segmentPairs[1].end = sideProbeEndpoint;
    segmentPairs[0].start = sideProbeEndpoint;
    segmentPairs[1].start = *focusPos;
    segmentPairs[0].end = *focusPos;

    CZClass::gwNodeSetRaycastable(rootNode, 0);
    CZDisplayInstance::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);

    PlayerProbeSampleCandidateBuffer probeBatches[2];
    CZDisplayInstance::BuildProbeHitBatchesForSegments(
        g_Player_RuntimeDiScene, segmentPairs, 4, probeBatches);

    CZClass::gwNodeSetRaycastable(rootNode, 1);
    FilterCameraProbeBlockingHits(probeBatches, 2);

    zVec3 hitPos;

    if (FindNearestThirdPersonCameraProbePoint(probeBatches, 2, focusPos, &hitPos) != 0) {
        sideProbeOffset.x = cameraDirNext->x * g_Player_ThirdPersonCameraSideProbeOffsetScale;
        sideProbeOffset.y = cameraDirNext->y * g_Player_ThirdPersonCameraSideProbeOffsetScale;
        sideProbeOffset.z = cameraDirNext->z * g_Player_ThirdPersonCameraSideProbeOffsetScale;
        cameraPos->x = hitPos.x + sideProbeOffset.x;
        cameraPos->y = hitPos.y + sideProbeOffset.y;
        cameraPos->z = hitPos.z + sideProbeOffset.z;
        cameraAdjusted = 1;
    }

    cameraAdjusted |= AdjustThirdPersonCameraByOffsetProbes(saveState, cameraPos, cameraDirNext);

    int preferAttachmentSlot1 = 0;
    if (saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub) {
        preferAttachmentSlot1 = 1;
        const float subClampY = playerState->subModeProbeBestHeight - kSubCameraProbeHeightOffset;
        if (!(subClampY > cameraPos->y)) {
            cameraPos->y = subClampY;
        }
    }

    g_Variant_CurrentTag = savedVariantTag;
    CZClass::gwNodeSetCellPickable(rootNode, 0);
    const int pickResult = CZDisplayInstance::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene,
        probeBatches,
        cameraPos->x,
        kCameraPickMaxY,
        cameraPos->z
    );
    CZClass::gwNodeSetCellPickable(rootNode, 1);
    if (pickResult != 0) {
        return cameraAdjusted;
    }

    int selectedCandidateIndex;
    int selectedImpactSlot;
    float taggedHeight;
    const float selectedHeight = SelectProbeSampleHeightFromCandidates(
        probeBatches,
        &selectedCandidateIndex,
        cameraPos->y,
        kCameraPickRiseWindow,
        preferAttachmentSlot1,
        &selectedImpactSlot,
        &taggedHeight
    );
    UpdateCameraVariantFromAnchor(probeBatches, cameraPos, selectedCandidateIndex);

    const float targetY = selectedHeight - kCameraFloorOffset;
    g_Player_CameraVariantUpdatedThisTick = 1;
    if (targetY > cameraPos->y) {
        cameraPos->y = targetY;

        ZMTH_VECTOR_DIRECTION(cameraDirNext, cameraPos, focusPos);
        cameraAdjusted = 1;
    }

    return cameraAdjusted;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.restorethirdpersoncamerafromobstructionstate
 * @recoil-artifact defines .text recoil:function:0x4063f0: Player::RestoreThirdPersonCameraFromObstructionState.
 * @recoil-match byte
 *
 * Purpose: implement Player::RestoreThirdPersonCameraFromObstructionState in the Battlesport camera subsystem.
 */
void __fastcall RestoreThirdPersonCameraFromObstructionState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    CZNode::GetWorldPosition(g_MainCamera, &playerState->cameraTarget);
    playerState->cameraDir = playerState->cameraObstructionDir;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.unbindcurrentsavestateifsingleplayer
 * @recoil-artifact defines .text recoil:function:0x406430: Player::UnbindCurrentSaveStateIfSinglePlayer
 *
 * Purpose: Clear the current save-state binding when the mission is not in
 * network play.
 * Source owner: battlesport_gameplay.player_camera_control_state_bridge,
 * not a C++ Player class and not the accepted player_camera.c source-file
 * owner.
 */
void UnbindCurrentSaveStateIfSinglePlayer() {
    if (zOpt::GetNetworkEnabled() == 0) {
        g_CurrentPlayerSaveState->playerState->currentSaveStateBound = 0;
        g_CurrentPlayerSaveState = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.bindactivegamestateascurrentsavestate
 * @recoil-artifact defines .text recoil:function:0x406450: Player::BindActiveGameStateAsCurrentSaveState
 *
 * Purpose: Bind the active local game-state record as the current save state
 * for camera/control paths.
 * Source owner: battlesport_gameplay.player_camera_control_state_bridge,
 * not a C++ Player class and not the accepted player_camera.c source-file
 * owner.
 */
void BindActiveGameStateAsCurrentSaveState() {
    zUtil_SaveGameState *const activeSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    activeSaveState->playerState->currentSaveStateBound = 1;
    g_CurrentPlayerSaveState = activeSaveState;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatecameravariantfromcamerapos
 * @recoil-artifact defines .text recoil:function:0x406470: Player::UpdateCameraVariantFromCameraPos.
 * @recoil-match byte
 *
 * Purpose: implement Player::UpdateCameraVariantFromCameraPos in the Battlesport camera subsystem.
 */
void __fastcall UpdateCameraVariantFromCameraPos(
    zUtil_SaveGameState *saveState,
    zVec3 *cameraPos
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerProbeSampleCandidateBuffer candidateBuffers[2];

    CZClass::gwNodeSetCellPickable(playerState->rootNode, 0);
    const int pickResult = CZDisplayInstance::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene,
        candidateBuffers,
        cameraPos->x,
        500.0f,
        cameraPos->z
    );
    CZClass::gwNodeSetCellPickable(playerState->rootNode, 1);

    if (pickResult == 0) {
        int selectedCandidateIndex;
        int selectedImpactSlot;
        float taggedHeight;
        SelectProbeSampleHeightFromCandidates(
            candidateBuffers,
            &selectedCandidateIndex,
            cameraPos->y,
            0.00100000005f,
            pickResult,
            &selectedImpactSlot,
            &taggedHeight
        );
        UpdateCameraVariantFromAnchor(candidateBuffers, cameraPos, selectedCandidateIndex);
    }

    g_Player_CameraVariantUpdatedThisTick = 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatecameravariantfromanchor
 * @recoil-artifact defines .text recoil:function:0x406510: Player::UpdateCameraVariantFromAnchor.
 * @recoil-match byte
 *
 * Purpose: implement Player::UpdateCameraVariantFromAnchor in the Battlesport camera subsystem.
 */
void __fastcall UpdateCameraVariantFromAnchor(
    PlayerProbeSampleCandidateBuffer *candidates,
    zVec3 *cameraPos,
    int selectedCandidateIndex
) {
    (void)cameraPos;

    zUtil_PlayerStateStorage *const playerState =
        g_CurrentPlayerSaveState->playerState;
    const zTag4Partial playerVariantTag = playerState->variantTag;

    zTag4Partial finalVariantTag;
    if (candidates->candidateCount > 0 &&
        candidates->entries[selectedCandidateIndex].variantTag.count > 0) {
        g_VariantTag_Current = candidates->entries[selectedCandidateIndex].variantTag;

        for (int playerTagIndex = 0; playerTagIndex < playerVariantTag.count; ++playerTagIndex) {
            const unsigned char playerTag = playerVariantTag.tags[playerTagIndex];
            int tagAlreadyPresent = 0;
            for (int tagIndex = 0; tagIndex < g_VariantTag_Current.count; ++tagIndex) {
                if (playerTag == g_VariantTag_Current.tags[tagIndex]) {
                    tagAlreadyPresent = 1;
                    break;
                }
            }

            if (tagAlreadyPresent == 0 && g_VariantTag_Current.count < 3) {
                g_VariantTag_Current.tags[g_VariantTag_Current.count] = playerTag;
                ++g_VariantTag_Current.count;
            }
        }

        int tagIsComplete = 1;
        for (int tagIndex = 0; tagIndex < g_VariantTag_Current.count; ++tagIndex) {
            if (g_VariantTag_Current.tags[tagIndex] == 0xff) {
                tagIsComplete = 0;
            }
        }

        if (tagIsComplete != 0) {
            finalVariantTag = g_VariantTag_Current;
            g_Player_LastValidCameraVariantTag = finalVariantTag;
        } else {
            finalVariantTag = g_Player_LastValidCameraVariantTag;
        }
    } else {
        finalVariantTag = g_Player_LastValidCameraVariantTag;
    }

    g_VariantTag_Current = finalVariantTag;
    g_Variant_CurrentTag = finalVariantTag;
    CZCamera::gwCameraSetVariantTagOverride(g_MainCamera, &g_VariantTag_Current);
    zEffect::SetVariantOverridePackedIdsIfComplete(&g_VariantTag_Current);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatecameraweatherfxemittervisibility
 * @recoil-artifact defines .text recoil:function:0x406610: Player::UpdateCameraWeatherFxEmitterVisibility.
 * @recoil-match byte
 *
 * Purpose: implement Player::UpdateCameraWeatherFxEmitterVisibility in the Battlesport camera subsystem.
 */
void UpdateCameraWeatherFxEmitterVisibility() {
    const float kVerticalProbeOffset = -50.0f;
    HudUiElement *const fxElement = g_HudSensorTracker.fxPass3Obj;
    if (fxElement == 0) {
        return;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    const int isSubMode =
        saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub;
    if (isSubMode != 0) {
        if ((~fxElement->flags & 0x10) != 0) {
            fxElement->SetVisible(0);
        }
    } else {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        CZNodePartial *const rootNode = playerState->rootNode;
        zVec3 cameraTarget;
        CZCamera::gwCameraGetTarget(
            g_MainCamera,
            &cameraTarget.x,
            &cameraTarget.y,
            &cameraTarget.z
        );
        CZClass::gwNodeSetRaycastable(rootNode, 0);
        CZDisplayInstance::SetStopAfterFirstHit(0x40000);
        CZDisplayInstance::SetBreakOnFirstCandidate(1);

        PlayerProbeSampleCandidateBuffer raycastCandidates;
        const int raycastResult = CZDisplayInstance::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            &raycastCandidates,
            cameraTarget.x,
            cameraTarget.y,
            cameraTarget.z,
            cameraTarget.x,
            cameraTarget.y - kVerticalProbeOffset,
            cameraTarget.z
        );

        CZDisplayInstance::SetBreakOnFirstCandidate(0);
        CZClass::gwNodeSetRaycastable(rootNode, 1);

        if (raycastResult == 0 && raycastCandidates.candidateCount > 0) {
            if ((~fxElement->flags & 0x10) != 0) {
                fxElement->SetVisible(0);
            }
        } else if ((~fxElement->flags & 0x10) == 0) {
            fxElement->SetVisible(1);
        }
    }

    if ((~fxElement->flags & 0x10) == 0) {
        return;
    }

    HudWeatherFx *const weatherFx = (HudWeatherFx *)(fxElement);
    CZNodePartial *const camera = g_MainCamera;
    weatherFx->camera = camera;
    weatherFx->activeParticleCount = zOpt::GetReplicateMode() == 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.filtercameraprobeblockinghits
 * @recoil-artifact defines .text recoil:function:0x406730: Player::FilterCameraProbeBlockingHits.
 * @recoil-match byte
 *
 * Purpose: implement Player::FilterCameraProbeBlockingHits in the Battlesport camera subsystem.
 */
void __fastcall FilterCameraProbeBlockingHits(
    PlayerProbeSampleCandidateBuffer *batches,
    int batchCount
) {
    const int kIgnoredCameraProbeNodeFlag = 0x8000000;
    const int kCallbackContextPresentFlag = 0x100000;
    const int kPlayerCollisionContextKind = 2;

    if (batchCount <= 0) {
        return;
    }

    PlayerProbeSampleCandidateBuffer *batch = batches;
    for (int batchesRemaining = batchCount; batchesRemaining > 0; --batchesRemaining) {
        for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
            zClassDiPickCandidateEntry *const candidate = &batch->entries[hitIndex];
            CZNodePartial *const node = candidate->node;
            const int flags = node->flags;

            if ((flags & kIgnoredCameraProbeNodeFlag) != 0) {
                candidate->node = 0;
                continue;
            }

            if ((flags & kCallbackContextPresentFlag) != 0 && node->callbackContext != 0) {
                int *const contextKind = (int *)(node->callbackContext);
                if (*contextKind == kPlayerCollisionContextKind) {
                    candidate->node = 0;
                }
            } else if (g_HudSensorTracker.raceCheckpointMode != 0 &&
                       HudSensorTracker::ParseCheckpointNumberFromNode(node) != 0) {
                candidate->node = 0;
            }
        }

        ++batch;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.adjustsubcamerafocusforobstruction
 * @recoil-artifact defines .text recoil:function:0x4067a0: Player::AdjustSubCameraFocusForObstruction.
 * @recoil-match byte
 *
 * Purpose: implement Player::AdjustSubCameraFocusForObstruction in the Battlesport camera subsystem.
 */
int __fastcall AdjustSubCameraFocusForObstruction(
    zUtil_SaveGameState *saveState,
    zVec3 *focusPos
) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kSubCameraFocusObstructionYOffset = 0.200000003f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    CZNodePartial *const rootNode = playerState->rootNode;
    zVec3 *const playerPos = &playerState->worldPos;
    int result = 0;
    CZDisplayInstanceSegmentEndpoints segmentPairs[2];
    segmentPairs[1].end = *playerPos;
    segmentPairs[0].start = *playerPos;
    segmentPairs[1].start = *focusPos;
    segmentPairs[0].end = *focusPos;

    CZClass::gwNodeSetRaycastable(rootNode, 0);
    CZDisplayInstance::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);

    PlayerProbeSampleCandidateBuffer probeBatches[2];
    CZDisplayInstance::BuildProbeHitBatchesForSegments(
        g_Player_RuntimeDiScene,
        segmentPairs,
        4,
        probeBatches
    );

    CZClass::gwNodeSetRaycastable(rootNode, 1);
    FilterCameraProbeBlockingHits(probeBatches, 2);

    zVec3 hitPos;
    if (FindNearestThirdPersonCameraProbePoint(probeBatches, 2, playerPos, &hitPos) !=
        0) {
        focusPos->y -= kSubCameraFocusObstructionYOffset;
        result = 1;
    }

    return result;
}

} // namespace Player
