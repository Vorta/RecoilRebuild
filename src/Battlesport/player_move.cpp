// Battlesport compilation unit between player_input.cpp and player_remap.cpp,
// inferred from the retail object boundary [0x426350, 0x429f10): its .rdata
// constants [0x4d0770, 0x4d0848) repeat 0.0f, 1.0f, -1.0f and 0.01f that the
// neighbouring objects pool separately; player_terrain.cpp reads its -300.0f
// (0x4d0798). Original filename unresolved; player_move.cpp is a provisional
// name (2026-10-02).

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
zVec3 g_Player_AmphibBasisUpRef = { 0.0f, 1.0f, 0.0f };
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-amphibsteerbasislerprate
 * @recoil-artifact defines .data recoil:data:0x4dc9a4: g_Player_AmphibSteerBasisLerpRate.
 * Purpose: stores the plan-tracked g_Player_AmphibSteerBasisLerpRate gameplay data symbol.
 */
float g_Player_AmphibSteerBasisLerpRate = 3.0f;
} // extern "C"

// Shared zero vector in this object's read-only data (0x4d0788); retail
// player.cpp, player_contact.cpp, pickup and opt-catalog code read the same object.
extern const zVec3 g_Player_ConstZeroVec3 = { 0.0f, 0.0f, 0.0f };
// Default alternate-gun aim origin (retail 0x4dc998, inside this object's .data
// run); player.cpp and weapon.cpp's aim-pitch helper read the same retail object.
extern const zVec3 kPlayerDefaultAltGunAimOrigin = { 0.0f, 0.0f, -1.0f };
// No-hit probe height in this object's read-only data (0x4d0798); player_terrain.cpp's
// BuildEnvironmentProbeResult reads the same object.
extern const float kPlayerProbeNoHitHeight = -300.0f;
namespace {
/**
 * Original inline helper; no standalone retail function exists. Observed in address-backed callers 0x4386c0, 0x4289f0,
 * 0x42c0d0, 0x42c2e0, 0x427440, 0x427ec0, 0x43a600, and 0x43a900 as a VC5-era int-bits smoothing idiom. Purpose:
 * reinterpret an IEEE-754 bit pattern as float.
 */
#define PLAYER_FLOAT_FROM_BITS(bits) (*(const float*)&(bits))

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in address-backed callers 0x428520 Player::UpdateMasterTypeSub, 0x426770 Player::UpdateMasterTypeTrack,
 * 0x4279f0 Player::UpdateMasterTypeAmphib, 0x427140 Player::UpdateMasterTypeHover. Purpose: wrap an accumulated signed
 * angle by one turn without emitting an out-of-line helper under the retail translation unit's /Ob0 compile mode.
 */
#define PLAYER_WRAP_SIGNED_TWO_PI(angle)                                                                               \
    do {                                                                                                               \
        if ((angle) < -6.28318548f) {                                                                                  \
            (angle) += 6.28318548f;                                                                                    \
        } else if ((angle) > 6.28318548f) {                                                                            \
            (angle) -= 6.28318548f;                                                                                    \
        }                                                                                                              \
    } while (0)
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in address-backed callers 0x425a20 Player::TickLocalPlayerControls, 0x428520 Player::UpdateMasterTypeSub,
 * 0x426770 Player::UpdateMasterTypeTrack, 0x427440 Player::UpdateMasterTypeHoverFromModalProbe. Purpose: clamp a
 * caller-owned scalar without emitting an out-of-line helper under the retail translation unit's /Ob0 compile mode.
 */
#define PLAYER_CLAMP_SIGNED(value, limit)                                                                              \
    do {                                                                                                               \
        if ((value) > (limit)) {                                                                                       \
            (value) = (limit);                                                                                         \
        } else if ((value) < -(limit)) {                                                                               \
            (value) = -(limit);                                                                                        \
        }                                                                                                              \
    } while (0)
/**
 * Original inline helper; no standalone retail function exists.
 * Observed expanded inline at 0x426770 Player::UpdateMasterTypeTrack,
 * 0x427440 Player::UpdateMasterTypeHoverFromModalProbe, and
 * 0x43a600 Player::UpdateAltGunAimDirection.
 * Purpose: transform a world-space vector into master-local space without
 * emitting an out-of-line helper under the retail translation unit's /Ob0
 * compile mode.
 */
#define PLAYER_TRANSFORM_WORLD_VECTOR_TO_LOCAL(out, vec, matrix)                                                       \
    do {                                                                                                               \
        (out).x = (vec).x * (matrix).xx + (vec).y * (matrix).xy + (vec).z * (matrix).xz;                               \
        (out).y = (vec).x * (matrix).yx + (vec).y * (matrix).yy + (vec).z * (matrix).yz;                               \
        (out).z = (vec).x * (matrix).zx + (vec).y * (matrix).zy + (vec).z * (matrix).zz;                               \
    } while (0)

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in address-backed callers 0x428520 Player::UpdateMasterTypeSub, 0x426770 Player::UpdateMasterTypeTrack,
 * 0x427440 Player::UpdateMasterTypeHoverFromModalProbe, 0x427140 Player::UpdateMasterTypeHover. Purpose: transform a
 * local vector without emitting an out-of-line helper under the retail translation unit's /Ob0 compile mode.
 */
#define PLAYER_TRANSFORM_LOCAL_VECTOR_TO_WORLD(out, vec, matrix)                                                       \
    do {                                                                                                               \
        (out).x = (vec).x * (matrix).xx + (vec).y * (matrix).yx + (vec).z * (matrix).zx;                               \
        (out).y = (vec).x * (matrix).xy + (vec).y * (matrix).yy + (vec).z * (matrix).zy;                               \
        (out).z = (vec).x * (matrix).xz + (vec).y * (matrix).yz + (vec).z * (matrix).zz;                               \
    } while (0)
enum PlayerMasterTypeId {
    kPlayerMasterTypeFly = 1,
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3,
    kPlayerMasterTypeHover = 4,
    kPlayerMasterTypeAmphib = 5
};
const int kPlayerPerFrameGeneralFlag = 2;
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x426770 Player::UpdateMasterTypeTrack, 0x42d5c0
 * Player::ApplyEnvironmentProbeResult. Purpose: provide the recovered cache attachment local offset helper for the
 * Player/Pickup gameplay source cluster.
 */
#define PLAYER_CACHE_ATTACHMENT_LOCAL_OFFSET(playerState)                                                              \
    do {                                                                                                               \
        const float playerAttachmentDx = (playerState)->worldPos.x - (playerState)->environmentAttachmentMatrix.posX;  \
        const float playerAttachmentDy = (playerState)->worldPos.y - (playerState)->environmentAttachmentMatrix.posY;  \
        const float playerAttachmentDz = (playerState)->worldPos.z - (playerState)->environmentAttachmentMatrix.posZ;  \
        const zMat4x3* const playerAttachmentMatrix = &(playerState)->environmentAttachmentMatrix;                     \
        (playerState)->environmentAttachmentLocalOffset.x = playerAttachmentDx * playerAttachmentMatrix->xx            \
            + playerAttachmentDy * playerAttachmentMatrix->xy + playerAttachmentDz * playerAttachmentMatrix->xz;       \
        (playerState)->environmentAttachmentLocalOffset.y = playerAttachmentDx * playerAttachmentMatrix->yx            \
            + playerAttachmentDy * playerAttachmentMatrix->yy + playerAttachmentDz * playerAttachmentMatrix->yz;       \
        (playerState)->environmentAttachmentLocalOffset.z = playerAttachmentDx * playerAttachmentMatrix->zx            \
            + playerAttachmentDy * playerAttachmentMatrix->zy + playerAttachmentDz * playerAttachmentMatrix->zz;       \
    } while (0)
} // namespace
namespace Player {
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in address-backed callers 0x424010 PlayerPendingContact::SelectPreferred, 0x4251f0
 * Player::CollectPendingCollisionContactsForQuadProbe, 0x426770 Player::UpdateMasterTypeTrack, 0x428d60
 * Player::ProbeModalSampleHeights. Purpose: transform a point without emitting an out-of-line helper under the retail
 * translation unit's /Ob0 compile mode.
 */
#define PLAYER_TRANSFORM_POINT_BY_MATRIX(result, point, matrix)                                                        \
    do {                                                                                                               \
        (result).x = (point).x * (matrix).xx + (point).y * (matrix).yx + (point).z * (matrix).zx + (matrix).posX;      \
        (result).y = (point).x * (matrix).xy + (point).y * (matrix).yy + (point).z * (matrix).zy + (matrix).posY;      \
        (result).z = (point).x * (matrix).xz + (point).y * (matrix).yz + (point).z * (matrix).zz + (matrix).posZ;      \
    } while (0)
enum { kPlayerMasterTypeSub = 2, kPlayerMaxModalProbePoints = 4, kPlayerEnvProbeBasePointOffset = 15 };

#define PLAYER_MAX_MODAL_PROBE_POINTS 4
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-floatsign
 * @recoil-artifact defines .text recoil:function:0x426350: Player::FloatSign.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::FloatSign from the recovered
 * Battlesport gameplay source file.
 */
int __stdcall FloatSign(float value)
{
    if (value == 0.0f) {
        return 0;
    }

    if (value < 0.0f) {
        return -1;
    }

    return 1;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport.player-move.player-tick-all-players
 * @recoil-artifact defines .text recoil:function:0x426390: Player::TickAllPlayers.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: GameZRecoil/player.cpp.
 * Purpose: reimplement PlayerMgr::TickAllPlayers from the recovered
 * Battlesport gameplay source file.
 */
void __cdecl TickAllPlayers()
{
    g_Player_DeltaTime = 0.00499999989f > g_FrameDeltaTimeSec ? 0.00499999989f : g_FrameDeltaTimeSec;
    g_Player_InvDeltaTime = 1.0f / g_Player_DeltaTime;
    g_Player_TotalTimeSecScaled = g_Time_AccumulatedTimeSec;
    g_Player_DeltaTimeScaled001 = g_Player_DeltaTime * 0.00999999978f;

    int activeMode2Count = 0;
    int totalMode2Count = 0;
    zUtil_SaveGameState* saveState;
    for (saveState = g_PlayerSaveStateList.head; saveState != 0; saveState = saveState != 0 ? saveState->next : 0) {
        zUtil_PlayerStateStorage* const playerState = saveState->playerState;
        const int lifecycleState = playerState->lifecycleState;
        playerState->generalFlags |= kPlayerPerFrameGeneralFlag;

        if (lifecycleState == kPlayerLifecycleInactive || lifecycleState == kPlayerLifecycleState6Inactive) {
            if (playerState->altGunFireHeldFlag != 0) {
                PlayerGunFireController* const activeAltGunController = playerState->activeAltGunController;
                playerState->altGunFireHeldFlag = 0;
                OptCatalog::DeactivateTrailRuntimeState(activeAltGunController->trailRuntimeState);
            }

            if (saveState == g_LocalPlayerSaveState) {
                TickLocalPlayerControls(saveState);
            }

            if (playerState->cameraTickEnabled != 0) {
                TickActiveCameraState(saveState);
            }

            if (zOpt::GetNetworkEnabled() != 0 && saveState != (zUtil_SaveGameState*)g_GameStateOrMapTable
                && saveState != g_Player2SaveState) {
                CZClass::gwNodeSetActive(playerState->rootNode, 0);
            }
        } else if (lifecycleState == kPlayerLifecycleRemote) {
            TickRemoteNetworkPlayer(saveState);
        } else {
            if (saveState == g_LocalPlayerSaveState) {
                TickLocalPlayerControls(saveState);
            } else if (lifecycleState == kPlayerLifecycleAi) {
                ++totalMode2Count;
                if (VariantTag::TagsOverlap(&playerState->variantTag, &g_VariantTag_Current) != 0
                    && ((playerState->targetDistanceSq = zMath::Vec3DistSqXZ(
                             &playerState->worldPos,
                             &((zUtil_SaveGameState*)g_GameStateOrMapTable)->playerState->worldPos
                         )) <= playerState->aiActivationRadiusSq
                        || playerState->recentHitFlag != 0)
                    && playerState->aiTickSuppressed == 0) {
                    playerState->aiActive = 1;
                    ++activeMode2Count;
                    AINet::TickAiMode2TopLevel(saveState);
                } else {
                    if (playerState->cameraTickEnabled != 0) {
                        TickActiveCameraState(saveState);
                    }
                    if (zSnd::GetAudioApiOption() == 1) {
                        saveState->UpdateModalLoopSfx(0);
                    }

                    const int altGunFireHeldFlag = playerState->altGunFireHeldFlag;
                    playerState->aiActive = 0;
                    if (altGunFireHeldFlag != 0) {
                        PlayerGunFireController* const activeAltGunController = playerState->activeAltGunController;
                        playerState->altGunFireHeldFlag = 0;
                        OptCatalog::DeactivateTrailRuntimeState(activeAltGunController->trailRuntimeState);
                    }
                    continue;
                }
            }

            const int postTickLifecycleState = playerState->lifecycleState;
            if (postTickLifecycleState == kPlayerLifecycleLocal || postTickLifecycleState == 0
                || VariantTag::TagsOverlap(&playerState->variantTag, &g_VariantTag_Current) != 0) {
                TickMasterTypeAndForceFeedback(saveState);

                if (playerState->masterType != kPlayerMasterTypeAmphib) {
                    if (g_Player_LocalControlEnabled != 0) {
                        UpdateAltGunAimDirection(saveState);
                    }

                    playerState->netInputBit16Latch = playerState->altGunDispatchRequested != 0
                            && playerState->activeAltGunController->ammoOrCharge > 0.0f
                        ? 1
                        : 0;
                    playerState->netInputBit17Latch = playerState->primaryGunDispatchRequested != 0
                            && playerState->activePrimaryGunController->ammoOrCharge > 0.0f
                        ? 1
                        : 0;

                    TickAltGunRuntimeState(saveState);
                }

                ResetDamageVisualsAndTimedStatus(saveState);
            }

            if (playerState->cameraTickEnabled != 0) {
                TickActiveCameraState(saveState);
            }

            if (zSnd::GetAudioApiOption() == 1) {
                saveState->UpdateModalLoopSfx(1);
            }
        }
    }

    if (zSnd::GetAudioApiOption() != 1) {
        zUtil_SaveGameState* const localSaveState = (zUtil_SaveGameState*)g_GameStateOrMapTable;
        if (localSaveState->playerState->lifecycleState != kPlayerLifecycleInactive) {
            localSaveState->UpdateModalLoopSfx(1);
        }
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        GameNet::TickLocalPlayerPkt06ReplicationAndHudTimer((zUtil_SaveGameState*)g_GameStateOrMapTable);
    }

    UpdateDebugOverlayHud(g_CurrentPlayerSaveState, activeMode2Count, totalMode2Count);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-tickmastertypeandforcefeedback
 * @recoil-artifact defines .text recoil:function:0x4266b0: Player::TickMasterTypeAndForceFeedback.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::TickMasterTypeAndForceFeedback from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall TickMasterTypeAndForceFeedback(zUtil_SaveGameState* saveState)
{
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    if (playerState->lifecycleState == kPlayerLifecycleInactive) {
        return;
    }

    if (playerState->damageProtectionActive != 0) {
        playerState->subPitchInput = 0.0f;
        playerState->subVerticalInput = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = 0.0f;
    }

    switch (masterModalData->masterType) {
    case 0:
        UpdateMasterTypeBasic(saveState);
        break;
    case kPlayerMasterTypeTrack:
        UpdateMasterTypeTrack(saveState);
        break;
    case kPlayerMasterTypeHover:
        UpdateMasterTypeHover(saveState);
        break;
    case kPlayerMasterTypeAmphib:
        UpdateMasterTypeAmphib(saveState);
        break;
    case kPlayerMasterTypeSub:
        UpdateMasterTypeSub(saveState);
        break;
    default:
        break;
    }

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        zEffect::SetConditionalRefPos(&playerState->worldPos);
        if (zInputDIIsForceFeedbackEnabled(g_zInputFfEffectSet) != 0) {
            g_zInputFfEffectSet->UpdateSteerAndPitchForceEffects();
        }
    }
}
} // namespace Player
/**
 * Original inline helper; no standalone retail function exists.
 * Evidence: retail 0x426770 and 0x427c64 scale projectileSpawnVel by
 * g_Player_InvDeltaTime through a separate output pointer after the
 * Vec3Subtract island (same helper shape as Camera.c Vec3ScaleTo).
 * Purpose: scale a vector by a scalar into an output vector.
 */
inline void Vec3ScaleTo(const zVec3* vec, float scale, zVec3* out)
{
    out->x = vec->x * scale;
    out->y = vec->y * scale;
    out->z = vec->z * scale;
}

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypetrack
 * @recoil-artifact defines .text recoil:function:0x426770: Player::UpdateMasterTypeTrack.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-rotate-rows-in-place
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-direction
 *
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeTrack from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeTrack(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    RebuildSteerBasisFromMotionAxes(saveState);

    if (playerState->airborneFlag != 0) {
        playerState->angVelYaw *= zMath::FastExp(g_Player_DeltaTime * -1.0f);
        if (playerState->slipSfxActive != 0) {
            StopSlipSfx(saveState);
        }
    } else {
        UpdateAutoTurnAndSteerFromTarget(saveState);
    }

    const float yawDelta = g_Player_DeltaTime * playerState->angVelYaw;
    if (playerState->environmentAttachmentActive != 0) {
        playerState->poseCache.y += yawDelta;
        PLAYER_WRAP_SIGNED_TWO_PI(playerState->poseCache.y);
        zMath::MatStackPushPtr((float*)&playerState->environmentAttachmentMatrix);
        zMath::MatLoadIdentity();
        CZNode::gwNodeBuildNodeToAncestorMatrix(playerState->environmentAttachmentNode, 3);
        zMath::MatStackPopPtr();
        playerState->restartYawRad
            = (float)(atan2(playerState->environmentAttachmentMatrix.zx, playerState->environmentAttachmentMatrix.zz))
            + playerState->poseCache.y;
    } else {
        playerState->restartYawRad += yawDelta;
        PLAYER_WRAP_SIGNED_TWO_PI(playerState->restartYawRad);
        playerState->poseCache = playerState->vehicleRotationAngles;
    }

    zMath::MatBuildEulerRotation3x3(
        playerState->vehicleRotationAngles.x,
        playerState->restartYawRad,
        playerState->vehicleRollRad,
        &playerState->motionBasis
    );
    RebuildSteerBasisFromMotionBasis(saveState);
    if (playerState->airborneFlag == 0) {
        UpdateYawVelocityFromSteerInput(saveState);
    }

    if (playerState->environmentAttachmentActive != 0) {
        zMath::Vec3RotateY(&playerState->yawRotatedLocalVel, &playerState->localVel, playerState->poseCache.y);
        const float offsetDx = g_Player_DeltaTime * playerState->yawRotatedLocalVel.x;
        const float offsetDz = playerState->yawRotatedLocalVel.z * g_Player_DeltaTime;
        playerState->environmentAttachmentLocalOffset.x += offsetDx;
        playerState->environmentAttachmentLocalOffset.z += offsetDz;

        zVec3 attachedWorld;
        ZMTH_VECTOR_TRANSFORM_POINT(
            &playerState->environmentAttachmentMatrix,
            &attachedWorld,
            &playerState->environmentAttachmentLocalOffset
        );
        zMath::Vec3Subtract(&attachedWorld, &playerState->worldPos, &playerState->projectileSpawnVel);
        Vec3ScaleTo(&playerState->projectileSpawnVel, g_Player_InvDeltaTime, &playerState->projectileSpawnVel);
        playerState->worldPos = attachedWorld;
    } else {
        if (playerState->airborneFlag != 0) {
            playerState->projectileSpawnVel.x *= zMath::FastExp(g_Player_DeltaTime * -0.200000003f);
            playerState->projectileSpawnVel.z *= zMath::FastExp(g_Player_DeltaTime * -0.200000003f);
            playerState->localVel = playerState->projectileSpawnVel;
            ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE(&playerState->motionBasis, &playerState->localVel);
        } else {
            ZMTH_VECTOR_TRANSFORM_DIRECTION(
                &playerState->motionBasis,
                &playerState->projectileSpawnVel,
                &playerState->localVel
            );
        }

        playerState->worldPos.x += g_Player_DeltaTime * playerState->projectileSpawnVel.x;
        playerState->yawRotatedLocalVel = playerState->projectileSpawnVel;
        playerState->worldPos.z += playerState->projectileSpawnVel.z * g_Player_DeltaTime;
    }

    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    switch (saveState->primaryModalState->masterModalData->masterType) {
    case kPlayerMasterTypeTrack:
        if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
            UpdatePostMoveEnvironment(saveState, 7);
        } else {
            UpdatePostMoveEnvironment(saveState, 4);
        }
        break;
    case 0:
        UpdateMasterTypeBasicOrTrackFromModalProbe(saveState);
        playerState->airborneFlag = 0;
        break;
    }

    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;
    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    if (playerState->airborneFlag != 0) {
        if (playerState->airborneFlagPrev == 0) {
            CZClass::gwNodeSetActive(playerState->modeVariantNode, 0);
        }
    } else if (playerState->airborneFlagPrev != 0) {
        CZClass::gwNodeSetActive(playerState->modeVariantNode, 1);
    }
    playerState->airborneFlagPrev = playerState->airborneFlag;

    if (playerState->environmentAttachmentActive != 0) {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
        // Reviewed subtract island over the matrix translation components (posX/posY/posZ).
        zMath::Vec3Subtract(
            &playerState->worldPos,
            (const zVec3*)&playerState->environmentAttachmentMatrix.posX,
            &playerState->environmentAttachmentLocalOffset
        );
#else
        playerState->environmentAttachmentLocalOffset.x
            = playerState->worldPos.x - playerState->environmentAttachmentMatrix.posX;
        playerState->environmentAttachmentLocalOffset.y
            = playerState->worldPos.y - playerState->environmentAttachmentMatrix.posY;
        playerState->environmentAttachmentLocalOffset.z
            = playerState->worldPos.z - playerState->environmentAttachmentMatrix.posZ;
#endif
        ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE(
            &playerState->environmentAttachmentMatrix,
            &playerState->environmentAttachmentLocalOffset
        );
    }

    CZObject3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehicleRotationAngles.x,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    CZObject3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    if (primaryModalState->modalNode != 0 && masterModalData->masterType == kPlayerMasterTypeTrack) {
        const float dampingWeight = zMath::FastExp(-(masterModalData->chassisSmoothFactor * g_Player_DeltaTime));
        const float newWeight = 1.0f - dampingWeight;
        float pitchTarget = masterModalData->chassisPitchRate * playerState->angVelPitch
            + masterModalData->chassisPitchMax * playerState->localVel.z;
        const float rollScale = masterModalData->chassisRollMax;
        float rollTarget = 0.0f;
        const float pitchFiltered
            = dampingWeight * primaryModalState->chassisPitchFilterState + pitchTarget * newWeight;
        primaryModalState->chassisPitchFilterState = pitchFiltered;
        const float rollFiltered = dampingWeight * primaryModalState->chassisRollFilterState + rollTarget * newWeight;
        primaryModalState->chassisRollFilterState = rollFiltered;

        pitchTarget -= pitchFiltered;
        primaryModalState->chassisPitchAngleRad = pitchTarget;
        primaryModalState->chassisRollAngleRad
            = rollScale * playerState->angVelYaw * playerState->localVel.z + (rollTarget - rollFiltered);
        if (masterModalData->chassisPitchDamping < pitchTarget) {
            primaryModalState->chassisPitchAngleRad = masterModalData->chassisPitchDamping;
        } else if (pitchTarget < -masterModalData->chassisPitchDamping) {
            primaryModalState->chassisPitchAngleRad = -masterModalData->chassisPitchDamping;
        }
        if (primaryModalState->chassisRollAngleRad > (double)masterModalData->chassisRollDamping) {
            primaryModalState->chassisRollAngleRad = masterModalData->chassisRollDamping;
        } else if (primaryModalState->chassisRollAngleRad < -masterModalData->chassisRollDamping) {
            primaryModalState->chassisRollAngleRad = -masterModalData->chassisRollDamping;
        }
        CZObject3D::gwObject3DSetRotation(
            primaryModalState->modalNode,
            primaryModalState->chassisPitchAngleRad,
            0.0f,
            primaryModalState->chassisRollAngleRad
        );
    }

    CZClass::gwNodeUpdate(playerState->rootNode);
    if (primaryModalState->modalNode != 0) {
        CZClass::gwNodeUpdate(primaryModalState->modalNode);
    }
    memcpy(
        &playerState->previousTransform,
        CZObject3D::gwObject3DGetMatrixPtr(playerState->rootNode),
        sizeof(playerState->previousTransform)
    );
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;

    if (primaryModalState->nodeRTracks != 0) {
        float trackSpeed = -playerState->localVel.z - playerState->angVelYaw * -2.25f;
        const float trackSpeedAbs = (float)(fabs(trackSpeed));
        const PlayerMasterCommonData* const masterCommonData = saveState->playerState->masterCommonData;
        int variantIndex;
        if (trackSpeedAbs >= masterCommonData->trackSwitchDist2) {
            variantIndex = 3;
        } else if (trackSpeedAbs >= masterCommonData->trackSwitchDist1) {
            variantIndex = 2;
        } else if (trackSpeedAbs >= masterCommonData->trackSwitchDist0) {
            variantIndex = 1;
        } else {
            variantIndex = 0;
        }

        unsigned int trackDisplay;
        CZClass::gwNodeGetUserData(primaryModalState->nodeRTracks, &trackDisplay);
        zDi::SetCurrentVariant((zDiPartial*)trackDisplay, variantIndex);

        unsigned int trackInstance;
        CZClass::gwNodeGetUserData(primaryModalState->nodeRTracks, &trackInstance);
        zModel::SetDiTextureWorldPerMeter((zDiPartial*)trackInstance, 1, 0.0f, trackSpeed * 1.72000003f);
        zModelInstanceUpdateScrollingTexturesIfNeeded((zModel_InstancePartial*)trackInstance);

        trackSpeed = -playerState->localVel.z - playerState->angVelYaw * 2.25f;
        CZClass::gwNodeGetUserData(primaryModalState->nodeLTracks, &trackInstance);
        zModel::SetDiTextureWorldPerMeter((zDiPartial*)trackInstance, 1, 0.0f, trackSpeed * 1.72000003f);
        zModelInstanceUpdateScrollingTexturesIfNeeded((zModel_InstancePartial*)trackInstance);
    }

    if (playerState->airborneFlag == 0) {
        if (primaryModalState->nodeDustL != 0 && primaryModalState->nodeDustR != 0) {
            const float dustScale = (float)(fabs(playerState->localVel.z)) / playerState->axisClampRuntime;
            CZObject3D::gwObject3DSetScale(primaryModalState->nodeDustL, dustScale, dustScale, dustScale);
            CZObject3D::gwObject3DSetScale(primaryModalState->nodeDustR, dustScale, dustScale, dustScale);
        }
    } else if (primaryModalState->nodeDustL != 0 && primaryModalState->nodeDustR != 0) {
        CZObject3D::gwObject3DSetScale(primaryModalState->nodeDustL, 0.0f, 0.0f, 0.0f);
        CZObject3D::gwObject3DSetScale(primaryModalState->nodeDustR, 0.0f, 0.0f, 0.0f);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypehover
 * @recoil-artifact defines .text recoil:function:0x427140: Player::UpdateMasterTypeHover.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-direction
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeHover from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeHover(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    playerState->restartYawRad += playerState->angVelYaw * g_Player_DeltaTime;
    PLAYER_WRAP_SIGNED_TWO_PI(playerState->restartYawRad);

    zMath::MatBuildEulerRotation3x3(
        playerState->vehicleRotationAngles.x,
        playerState->restartYawRad,
        playerState->vehicleRollRad,
        &playerState->motionBasis
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);

    ZMTH_VECTOR_TRANSFORM_DIRECTION(
        &playerState->motionBasis,
        &playerState->projectileSpawnVel,
        &playerState->localVel
    );

    playerState->worldPos.x += g_Player_DeltaTime * playerState->projectileSpawnVel.x;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->worldPos.z += g_Player_DeltaTime * playerState->projectileSpawnVel.z;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->worldPos.y += g_Player_DeltaTime * playerState->projectileSpawnVel.y;
    playerState->motionBasis.posY = playerState->worldPos.y;

    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    if (playerState->slipSfxActive != 0
        && (playerState->playerCollisionResolved != 0 || playerState->worldCollisionResolved != 0
            || playerState->preferredCollisionResolved != 0)) {
        StopSlipSfx(saveState);
    }

    UpdateMasterTypeHoverFromModalProbe(saveState);

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    CZObject3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehicleRotationAngles.x,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    CZObject3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );

    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    CZClass::gwNodeUpdate(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        CZObject3D::gwObject3DGetMatrixPtr(playerState->rootNode),
        sizeof(playerState->previousTransform)
    );

    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypehover-frommodalprobe
 * @recoil-artifact defines .text recoil:function:0x427440: Player::UpdateMasterTypeHoverFromModalProbe.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-rotate-rows-in-place
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-direction
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeHoverFromModalProbe from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeHoverFromModalProbe(zUtil_SaveGameState* saveState)
{
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[PLAYER_MAX_MODAL_PROBE_POINTS];
    float outBestHeight;
    PlayerProbeTypeHistogram outTypeHistogram;
    int outAttachmentCandidateCount;
    CZNodePartial* outAttachmentNode;
    float lowestProbeHeight = 5000.0f;
    ProbeModalSampleHeights(
        saveState,
        probeHeightByPoint,
        &outBestHeight,
        0,
        &outTypeHistogram,
        &outAttachmentCandidateCount,
        &outAttachmentNode
    );

    playerState->yawVelocityLimit = masterModalData->yawRateMax;

    // Retail leaves the lowest index unset when no probe is below 5000.
    int lowestProbeIndex;
    const int probePointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < probePointCount; ++i) {
        if (probeHeightByPoint[i] < lowestProbeHeight) {
            lowestProbeHeight = probeHeightByPoint[i];
            lowestProbeIndex = i;
        }
    }

    int supportPointIndex[PLAYER_MAX_MODAL_PROBE_POINTS];
    int supportCount = 0;
    for (int supportIndex = 0; supportIndex < probePointCount; ++supportIndex) {
        if (supportIndex != lowestProbeIndex) {
            supportPointIndex[supportCount++] = supportIndex;
        }
    }

    zVec3 p0 = primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[0]];
    p0.y = probeHeightByPoint[supportPointIndex[0]];
    zVec3 p1 = primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[1]];
    p1.y = probeHeightByPoint[supportPointIndex[1]];
    zVec3 p2 = primaryModalState->transformedProbePointWorldByIndex[supportPointIndex[2]];
    p2.y = probeHeightByPoint[supportPointIndex[2]];

    zVec3 probePlaneNormal;
    zMathVec3TriangleNormal(&p0, &p2, &p1, &probePlaneNormal);

    float gravityScale = playerState->gravityAccel;
    if (probePlaneNormal.y < g_Player_MaxSlope) {
        gravityScale *= 12.0f;
    }
    zVec3 slopeImpulse;
    slopeImpulse.x = probePlaneNormal.x * (g_Player_DeltaTime * gravityScale);
    slopeImpulse.z = probePlaneNormal.z * (g_Player_DeltaTime * gravityScale);
    slopeImpulse.y = ((probePlaneNormal.y - 1.0f) * g_Player_DeltaTime) * gravityScale;
    zMath::Vec3Add(&playerState->projectileSpawnVel, &slopeImpulse, &playerState->projectileSpawnVel);

    playerState->localVel = playerState->projectileSpawnVel;
    ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE(&playerState->motionBasis, &playerState->localVel);

    zMath::Vec3LerpNormalize(
        &playerState->steerBasisRef,
        &probePlaneNormal,
        zMath::FastExp(masterModalData->hoverNormalLerpRate * g_FrameDeltaTimeSec)
    );
    RebuildSteerBasisRawFromRef(saveState);
    RebuildMotionBasisFromSteerBasis(saveState);

    float minHoverClearance = 1000.0f;
    for (int clearanceIndex = 0; clearanceIndex < primaryModalState->modalStateCode; ++clearanceIndex) {
        ZMTH_VECTOR_TRANSFORM_POINT(
            &playerState->motionBasis,
            &primaryModalState->transformedProbePointWorldByIndex[clearanceIndex],
            &masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + clearanceIndex]
        );

        const float clearance = primaryModalState->transformedProbePointWorldByIndex[clearanceIndex].y
            - probeHeightByPoint[clearanceIndex];
        if (clearance < minHoverClearance) {
            minHoverClearance = clearance;
        }
    }

    const float hoverLiftError = minHoverClearance - masterModalData->modeAltTransitionTime;
    if (playerState->modeVariantNode != 0) {
        CZClass::gwNodeSetActive(playerState->modeVariantNode, hoverLiftError <= 2.0f ? 1 : 0);
    }

    if (hoverLiftError > 2.0f && playerState->localVel.y > 0.0f) {
        playerState->localVel.y = 0.0f;
    }

    const float liftDamping = zMath::FastExp(masterModalData->hoverLiftDampingRate * g_Player_DeltaTime);
    playerState->localVel.y = liftDamping * playerState->localVel.y
        - (1.0f - liftDamping) * masterModalData->hoverLiftScale * hoverLiftError;

    if (minHoverClearance < 0.0) {
        playerState->worldPos.y -= minHoverClearance - 0.5f;
        playerState->motionBasis.posY = playerState->worldPos.y;
        if (probePlaneNormal.y > g_Player_MaxSlope) {
            playerState->localVel.y -= -5.0f;
        }
    }

    if (playerState->slipSfxActive != 0) {
        ZMTH_VECTOR_TRANSFORM_DIRECTION(
            &playerState->motionBasis,
            &playerState->projectileSpawnVel,
            &playerState->localVel
        );
    }

    zMath::Vec3RotateY(&probePlaneNormal, &playerState->steerBasisRef, -playerState->restartYawRad);
    playerState->vehiclePitchRad = (float)(asin(probePlaneNormal.z));
    playerState->vehicleRollRad = (float)(asin(-probePlaneNormal.x));

    const float speedAbs = (float)(fabs(playerState->localVel.z));
    const float pitchWaveArg
        = (masterModalData->hoverPitchWaveSpeedRate * speedAbs + masterModalData->hoverPitchWaveBaseRate)
        * g_Time_AccumulatedTimeSec;
    const float rollWaveArg
        = (masterModalData->hoverRollWaveSpeedRate * speedAbs + masterModalData->hoverRollWaveBaseRate)
        * g_Time_AccumulatedTimeSec;
    const float pitchWave = (float)(sin(pitchWaveArg)) * masterModalData->hoverPitchWaveAmplitude;
    const float rollWave = (float)(sin(rollWaveArg)) * masterModalData->hoverRollWaveAmplitude
        + masterModalData->hoverRollYawCoupleScale * playerState->angVelYaw * playerState->localVel.z;
    playerState->vehiclePitchRad += g_Player_DeltaTime * pitchWave;
    playerState->vehicleRollRad += g_Player_DeltaTime * rollWave;

    PLAYER_CLAMP_SIGNED(playerState->vehiclePitchRad, 0.523599982f);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypeamphib
 * @recoil-artifact defines .text recoil:function:0x4279f0: Player::UpdateMasterTypeAmphib.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeAmphib from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeAmphib(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    const float yawDelta = playerState->angVelYaw * g_Player_DeltaTime;
    if (playerState->environmentAttachmentActive != 0) {
        playerState->poseCache.y += yawDelta;
        PLAYER_WRAP_SIGNED_TWO_PI(playerState->poseCache.y);
        zMath::MatStackPushPtr((float*)&playerState->environmentAttachmentMatrix);
        zMath::MatLoadIdentity();
        CZNode::gwNodeBuildNodeToAncestorMatrix(playerState->environmentAttachmentNode, 3);
        zMath::MatStackPopPtr();
        playerState->restartYawRad
            = (float)(atan2(playerState->environmentAttachmentMatrix.zx, playerState->environmentAttachmentMatrix.zz))
            + playerState->poseCache.y;
    } else {
        playerState->restartYawRad += yawDelta;
        PLAYER_WRAP_SIGNED_TWO_PI(playerState->restartYawRad);
        playerState->poseCache = playerState->vehicleRotationAngles;
    }

    zMath::MatBuildEulerRotation3x3(
        playerState->vehicleRotationAngles.x,
        playerState->restartYawRad,
        playerState->vehicleRollRad,
        &playerState->motionBasis
    );
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);

    if (playerState->environmentAttachmentActive != 0) {
        zMath::Vec3RotateY(&playerState->yawRotatedLocalVel, &playerState->localVel, playerState->poseCache.y);
        const float offsetDx = g_Player_DeltaTime * playerState->yawRotatedLocalVel.x;
        const float offsetDz = playerState->yawRotatedLocalVel.z * g_Player_DeltaTime;
        playerState->environmentAttachmentLocalOffset.x += offsetDx;
        playerState->environmentAttachmentLocalOffset.z += offsetDz;
        // Retail stores the zero y offset after both += updates (+0x1b2).
        playerState->environmentAttachmentLocalOffset.y = 0.0f;

        zVec3 attachedWorld;
        ZMTH_VECTOR_TRANSFORM_POINT(
            &playerState->environmentAttachmentMatrix,
            &attachedWorld,
            &playerState->environmentAttachmentLocalOffset
        );
        zMath::Vec3Subtract(&attachedWorld, &playerState->worldPos, &playerState->projectileSpawnVel);
        Vec3ScaleTo(&playerState->projectileSpawnVel, g_Player_InvDeltaTime, &playerState->projectileSpawnVel);
        playerState->worldPos = attachedWorld;
    } else {
        const float negSteerZ = -playerState->steerBasisNorm.z;
        const float negSteerX = -playerState->steerBasisNorm.x;
        playerState->projectileSpawnVel.x = negSteerX * playerState->localVel.z + negSteerZ * playerState->localVel.x;
        playerState->projectileSpawnVel.y = playerState->localVel.y;
        playerState->projectileSpawnVel.z = negSteerZ * playerState->localVel.z - negSteerX * playerState->localVel.x;
        playerState->worldPos.x += playerState->projectileSpawnVel.x * g_Player_DeltaTime;
        playerState->worldPos.z += playerState->projectileSpawnVel.z * g_Player_DeltaTime;
        playerState->yawRotatedLocalVel = playerState->projectileSpawnVel;
    }

    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    if (playerState->lifecycleState != 0) {
        ProcessPendingContactQueues(saveState);
    }

    UpdateMasterTypeAmphibFromModalProbe(saveState);

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    CZObject3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehicleRotationAngles.x,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    CZObject3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    CZClass::gwNodeUpdate(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        CZObject3D::gwObject3DGetMatrixPtr(playerState->rootNode),
        sizeof(playerState->previousTransform)
    );
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;

    if (primaryModalState->nodeWake != 0) {
        const float wakeScale = (float)(fabs(playerState->localVel.z)) / playerState->axisClampRuntime;
        CZObject3D::gwObject3DSetScale(primaryModalState->nodeWake, wakeScale, wakeScale, wakeScale);
        CZObject3D::gwObject3DSetScale(primaryModalState->nodeSplashL, wakeScale, wakeScale, wakeScale);
        CZObject3D::gwObject3DSetScale(primaryModalState->nodeSplashR, wakeScale, wakeScale, wakeScale);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypeamphib-frommodalprobe
 * @recoil-artifact defines .text recoil:function:0x427ec0: Player::UpdateMasterTypeAmphibFromModalProbe.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-match source
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeAmphibFromModalProbe from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeAmphibFromModalProbe(zUtil_SaveGameState* saveState)
{
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[PLAYER_MAX_MODAL_PROBE_POINTS];
    float outBestHeight;
    PlayerProbeTypeHistogram outTypeHistogram;
    int outAttachmentCandidateCount;
    CZNodePartial* outAttachmentNode;
    ProbeModalSampleHeights(
        saveState,
        probeHeightByPoint,
        &outBestHeight,
        0,
        &outTypeHistogram,
        &outAttachmentCandidateCount,
        &outAttachmentNode
    );

    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    if (outTypeHistogram.countByImpactSlot[1] < primaryModalState->modalStateCode) {
        if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
            playerState->amphibProbeCoverageFailed = 1;
            TransitionToMasterTypeTrack(saveState, 0);
        } else {
            playerState->projectileSpawnVel.x = 0.0f;
            playerState->projectileSpawnVel.z = 0.0f;
            playerState->localVel.x = 0.0f;
            playerState->localVel.z = 0.0f;
            playerState->aiTopLevelState = 0;
            playerState->aiStateUntilTime = g_Time_AccumulatedTimeSec + 8.0f;
        }
    } else {
        playerState->amphibProbeCoverageFailed = 0;
    }

    float maxSampleHeight = outBestHeight;
    for (int i = 0; i < primaryModalState->modalStateCode; ++i) {
        if (probeHeightByPoint[i] >= (double)maxSampleHeight) {
            maxSampleHeight = probeHeightByPoint[i];
        }
    }

    const float oldWorldY = playerState->worldPos.y;
    playerState->worldPos.y = maxSampleHeight + masterModalData->modeAltTransitionTime;
    const float verticalVelocity = (playerState->worldPos.y - oldWorldY) * g_Player_InvDeltaTime;
    playerState->localVel.y = verticalVelocity;
    playerState->projectileSpawnVel.y = verticalVelocity;

    zVec3 amphibUpVector = g_Player_AmphibBasisUpRef;
    ApplyAmphibSpeedOscillation(saveState, &amphibUpVector, 1);

    zMath::Vec3LerpNormalize(
        &playerState->steerBasisRef,
        &amphibUpVector,
        zMath::FastExp(-(g_FrameDeltaTimeSec * g_Player_AmphibSteerBasisLerpRate))
    );
    if (playerState->steerBasisRef.y == 0.0) {
        playerState->steerBasisRef.y = 0.00100000005f;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y
        = -((rawBasis.x * playerState->steerBasisRef.x + rawBasis.z * playerState->steerBasisRef.z)
            / playerState->steerBasisRef.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;
    RebuildMotionBasisFromSteerBasis(saveState);

    zMath::Vec3RotateY(&amphibUpVector, &playerState->steerBasisRef, -playerState->restartYawRad);
    playerState->vehiclePitchRad = (float)(asin(amphibUpVector.z));
    playerState->vehicleRollRad = (float)(asin(-amphibUpVector.x));
    PLAYER_CLAMP_SIGNED(playerState->vehiclePitchRad, 0.523599982f);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypebasic
 * @recoil-artifact defines .text recoil:function:0x428120: Player::UpdateMasterTypeBasic.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeBasic from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeBasic(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    float savedLocalVelX;
    if (playerState->cameraState == 2) {
        UpdateBankVelocityFromSteerInput(saveState);
        savedLocalVelX = playerState->localVel.x;
    } else {
        IntegrateYawAndWrapFromYawVelocity(saveState);
    }

    zMath::MatBuildEulerRotation3x3(
        playerState->vehicleRotationAngles.x,
        playerState->vehicleRotationAngles.y,
        playerState->vehicleRotationAngles.z,
        &playerState->motionBasis
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    RebuildSteerBasisFromMotionBasis(saveState);

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    UpdateYawVelocityFromSteerInput(saveState);
    if (playerState->cameraState == 2) {
        playerState->localVel.x = savedLocalVelX;
    }

    const float negSteerZ = -playerState->steerBasisNorm.z;
    const float negSteerX = -playerState->steerBasisNorm.x;
    playerState->projectileSpawnVel.y = playerState->localVel.y;
    playerState->projectileSpawnVel.x = negSteerX * playerState->localVel.z + negSteerZ * playerState->localVel.x;
    playerState->projectileSpawnVel.z = negSteerZ * playerState->localVel.z - negSteerX * playerState->localVel.x;

    playerState->worldPos.x += playerState->projectileSpawnVel.x * g_Player_DeltaTime;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->worldPos.z += playerState->projectileSpawnVel.z * g_Player_DeltaTime;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    UpdateMasterTypeBasicOrTrackFromModalProbe(saveState);

    CZObject3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehicleRotationAngles.x,
        playerState->vehicleRotationAngles.y,
        playerState->vehicleRotationAngles.z
    );
    CZObject3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );

    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;

    CZClass::gwNodeUpdate(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        CZObject3D::gwObject3DGetMatrixPtr(playerState->rootNode),
        sizeof(playerState->previousTransform)
    );

    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypebasicortrack-frommodalprobe
 * @recoil-artifact defines .text recoil:function:0x428350: Player::UpdateMasterTypeBasicOrTrackFromModalProbe.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeBasicOrTrackFromModalProbe from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeBasicOrTrackFromModalProbe(zUtil_SaveGameState* saveState)
{
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    // This caller consumes the heights; the helper also requires these outputs.
    float sampleHeights[PLAYER_MAX_MODAL_PROBE_POINTS];
    CZNodePartial* unusedAttachmentNode;
    float unusedBestHeight;
    int unusedAttachmentCandidateCount;
    PlayerProbeTypeHistogram unusedHistogram;
    ProbeModalSampleHeights(
        saveState,
        sampleHeights,
        &unusedBestHeight,
        0,
        &unusedHistogram,
        &unusedAttachmentCandidateCount,
        &unusedAttachmentNode
    );

    playerState->yawVelocityLimit = masterModalData->yawRateMax;

    float maxSampleHeight;
    const int probePointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < probePointCount; ++i) {
        if (i == 0) {
            maxSampleHeight = sampleHeights[0];
        } else {
            maxSampleHeight = __max(sampleHeights[i], maxSampleHeight);
        }
    }

    playerState->vehiclePitchRad = 0.0f;
    playerState->vehicleRollRad = 0.0f;
    playerState->worldPos.y = masterModalData->modeAltTransitionTime + maxSampleHeight;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatebankvelocityfromsteerinput
 * @recoil-artifact defines .text recoil:function:0x4283f0: Player::UpdateBankVelocityFromSteerInput.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\player.cpp.
 * Purpose: reimplement Player::UpdateBankVelocityFromSteerInput from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateBankVelocityFromSteerInput(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    playerState->restartYawRad = 0.0f;
    if (playerState->steeringInput != 0.0f) {
        if (playerState->steeringInputCopy > 0.0f && playerState->localVel.x > 0.0f) {
            playerState->localVel.x = 0.0f;
        } else if (playerState->steeringInputCopy < 0.0f && playerState->localVel.x < 0.0f) {
            playerState->localVel.x = 0.0f;
        }

        playerState->localVel.x -= masterModalData->accelRate * g_Player_DeltaTime * playerState->steeringInputCopy;
    } else {
        playerState->localVel.x = 0.0f;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-integrateyawandwrapfromyawvelocity
 * @recoil-artifact defines .text recoil:function:0x428490: Player::IntegrateYawAndWrapFromYawVelocity.
 * @recoil-match byte
 *
 * Purpose: Updates auto-turn steering, integrates yaw and wraps it by one full turn.
 */
void __fastcall IntegrateYawAndWrapFromYawVelocity(zUtil_SaveGameState* saveState)
{
    const float kTwoPi = 6.28318548f;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    if (playerState->autoTurnActive != 0) {
        playerState->restartYawRad
            = (float)(atan2(-playerState->autoTurnTargetDir.z, -playerState->autoTurnTargetDir.x));
        playerState->steeringInput = 0.0f;
        playerState->angVelYaw = 0.0f;
        playerState->autoTurnActive = 0;
    }

    UpdateAutoTurnAndSteerFromTarget(saveState);

    float yaw = playerState->restartYawRad + playerState->angVelYaw * g_Player_DeltaTime;
    playerState->restartYawRad = yaw;
    if (yaw < -kTwoPi) {
        playerState->restartYawRad = yaw + kTwoPi;
    } else if (yaw > kTwoPi) {
        playerState->restartYawRad = yaw - kTwoPi;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatemastertypesub
 * @recoil-artifact defines .text recoil:function:0x428520: Player::UpdateMasterTypeSub.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-direction
 * @recoil-match source
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateMasterTypeSub from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateMasterTypeSub(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    CacheDisableCopterSndNodesAndStopSample();
    RebuildSteerBasisFromMotionAxes(saveState);
    UpdateAutoTurnAndSteerFromTarget(saveState);

    if (playerState->subPitchInput != 0.0f) {
        if ((playerState->subPitchInputCopy > 0.0f && playerState->angVelPitch < 0.0f)
            || (playerState->subPitchInputCopy < 0.0f && playerState->angVelPitch > 0.0f)) {
            playerState->angVelPitch = 0.0f;
        }

        const float angVelPitch = playerState->angVelPitch
            + masterModalData->yawAccel * g_Player_DeltaTime * playerState->subPitchInputCopy * 0.5f;
        playerState->angVelPitch = angVelPitch;
        // Reconstruction spelling that reproduces retail's register comparison
        // (fld limit; fxch; fcompp) under this profile.
        if ((double)masterModalData->yawRateMax < angVelPitch) {
            playerState->angVelPitch = masterModalData->yawRateMax;
        } else if (angVelPitch < -masterModalData->yawRateMax) {
            playerState->angVelPitch = -masterModalData->yawRateMax;
        }
    } else {
        playerState->angVelPitch = 0.0f;
        playerState->vehicleRotationAngles.x *= zMath::FastExp(g_Player_DeltaTime * -7.0f);
    }

    playerState->vehicleRotationAngles.x += g_Player_DeltaTime * playerState->angVelPitch;
    playerState->restartYawRad += g_Player_DeltaTime * playerState->angVelYaw;
    playerState->vehicleRollRad = playerState->angVelRoll * g_Player_DeltaTime + playerState->vehicleRollRad
        - (masterModalData->hoverRollYawCoupleScale * playerState->angVelYaw) * playerState->localVel.z;
    PLAYER_CLAMP_SIGNED(playerState->vehicleRotationAngles.x, 0.5f);
    PLAYER_WRAP_SIGNED_TWO_PI(playerState->restartYawRad);
    PLAYER_CLAMP_SIGNED(playerState->vehicleRollRad, 0.349999994f);

    zMath::MatBuildEulerRotation3x3(
        playerState->vehicleRotationAngles.x,
        playerState->restartYawRad,
        playerState->vehicleRollRad,
        &playerState->motionBasis
    );
    RebuildSteerBasisFromMotionBasis(saveState);
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
    playerState->axisClampRuntime = masterModalData->maxSpeed;

    UpdateYawVelocityFromSteerInput(saveState);
    UpdateSubVerticalDamping(saveState);

    ZMTH_VECTOR_TRANSFORM_DIRECTION(
        &playerState->motionBasis,
        &playerState->projectileSpawnVel,
        &playerState->localVel
    );
    playerState->worldPos.x += g_Player_DeltaTime * playerState->projectileSpawnVel.x;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->worldPos.y += g_Player_DeltaTime * playerState->projectileSpawnVel.y;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->worldPos.z += g_Player_DeltaTime * playerState->projectileSpawnVel.z;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    ProcessPendingContactQueues(saveState);
    UpdateSubModeWaterProbeState(saveState);
    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        ProcessPendingContactQueues(saveState);
        if (CollectPendingCollisionContactsForQuadProbe(saveState, 0.0f) != 0
            || CollectPendingCollisionContactsForQuadProbe(saveState, 1.25f) != 0) {
            ApplyPendingCollisionProbeVelocity(saveState);
            playerState->collisionProbeResolved = 1;
        } else {
            playerState->collisionProbeResolved = 0;
        }
    }

    CZObject3D::gwObject3DSetRotation(
        playerState->rootNode,
        playerState->vehicleRotationAngles.x,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    CZObject3D::gwObject3DSetPosition(
        playerState->rootNode,
        playerState->worldPos.x,
        playerState->worldPos.y,
        playerState->worldPos.z
    );
    playerState->fxOffsetWorld.x = playerState->fxOffsetLocal.x + playerState->worldPos.x;
    playerState->fxOffsetWorld.y = playerState->fxOffsetLocal.y + playerState->worldPos.y;
    playerState->fxOffsetWorld.z = playerState->fxOffsetLocal.z + playerState->worldPos.z;
    CZClass::gwNodeUpdate(playerState->rootNode);
    memcpy(
        &playerState->previousTransform,
        CZObject3D::gwObject3DGetMatrixPtr(playerState->rootNode),
        sizeof(playerState->previousTransform)
    );
    playerState->bankBasis = playerState->steerBasisNorm;
    playerState->cachedVehicleRotationAngles = playerState->vehicleRotationAngles;

    CZNodePartial* const nodeProps = primaryModalState->nodeProps;
    if (nodeProps != 0) {
        const float cycleSpeed = 6.0f - playerState->localVel.z * 0.5f;
        unsigned int displayInstanceValue;
        CZClass::gwNodeGetUserData(nodeProps, &displayInstanceValue);
        zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial*)displayInstanceValue, cycleSpeed);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatesubmodewaterprobestate
 * @recoil-artifact defines .text recoil:function:0x4289f0: Player::UpdateSubModeWaterProbeState.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateSubModeWaterProbeState from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateSubModeWaterProbeState(zUtil_SaveGameState* saveState)
{
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    float probeHeightByPoint[PLAYER_MAX_MODAL_PROBE_POINTS];
    float outBestHeight;
    PlayerProbeTypeHistogram outTypeHistogram;
    int outAttachmentCandidateCount;
    CZNodePartial* outAttachmentNode;
    ProbeModalSampleHeights(
        saveState,
        probeHeightByPoint,
        &outBestHeight,
        1,
        &outTypeHistogram,
        &outAttachmentCandidateCount,
        &outAttachmentNode
    );

    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    if (outBestHeight == kPlayerProbeNoHitHeight) {
        outBestHeight = 1000.0f;
    }
    playerState->subModeProbeBestHeight = outBestHeight;

    float deepestSubmergedSampleHeight = kPlayerProbeNoHitHeight;
    int deepestSubmergedSampleIndex;
    for (int i = 0; i < primaryModalState->modalStateCode; ++i) {
        if (probeHeightByPoint[i] < outBestHeight && deepestSubmergedSampleHeight < probeHeightByPoint[i]) {
            deepestSubmergedSampleHeight = probeHeightByPoint[i];
            deepestSubmergedSampleIndex = i;
        }
    }

    if (playerState->worldCollisionResolved != 1) {
        float resolvedY = playerState->worldPos.y;
        const float surfaceY = masterModalData->modeAltTransitionTime + outBestHeight;
        if (resolvedY > surfaceY) {
            resolvedY = surfaceY;
        } else {
            deepestSubmergedSampleHeight -= masterModalData->probePoints[15 + deepestSubmergedSampleIndex].y;
            if (resolvedY < deepestSubmergedSampleHeight) {
                resolvedY = deepestSubmergedSampleHeight;
            }
        }

        playerState->worldPos.y = resolvedY;
        playerState->motionBasis.posY = resolvedY;
    }

    const float rollDampingFactor = zMath::FastExp(-g_Player_DeltaTime);
    playerState->angVelRoll = -(rollDampingFactor * playerState->vehicleRollRad);

    const float speedAbs = (float)(fabs(playerState->localVel.z));
    const float pitchWaveRate
        = speedAbs * masterModalData->hoverPitchWaveSpeedRate + masterModalData->hoverPitchWaveBaseRate;
    const float rollWaveRate
        = speedAbs * masterModalData->hoverRollWaveSpeedRate + masterModalData->hoverRollWaveBaseRate;
    const float pitchBobDelta
        = sinf(pitchWaveRate * g_Time_AccumulatedTimeSec) * masterModalData->hoverPitchWaveAmplitude;
    const float rollBobDelta = sinf(rollWaveRate * g_Time_AccumulatedTimeSec) * masterModalData->hoverRollWaveAmplitude;

    playerState->vehiclePitchRad += g_Player_DeltaTime * pitchBobDelta;
    playerState->vehicleRollRad += g_Player_DeltaTime * rollBobDelta;

    if (playerState->underwaterFxEnabled != 0 && playerState->cameraTarget.y < outBestHeight) {
        ((HudUiElement*)(&g_Player_UnderwaterFxPass3Ui))->SetVisible(1);
        g_Player_HorizonNodeFollowCameraEnabled = 0;

        CZNodePartial* const nodeCaustic1 = primaryModalState->nodeCaustic1;
        if (nodeCaustic1 != 0) {
            unsigned int displayInstanceValue;
            CZClass::gwNodeGetUserData(nodeCaustic1, &displayInstanceValue);
            zDi::SetCurrentVariantCycleTextureSpeed((zDiPartial*)displayInstanceValue, 12.0f);
        }
    }

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable
        && playerState->worldPos.y + 2.20000005f > outBestHeight) {
        TransitionToMasterTypeAmphib(saveState, 0, 0);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatesubverticaldamping
 * @recoil-artifact defines .text recoil:function:0x428c20: Player::UpdateSubVerticalDamping.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-match source
 *
 * Source model: bounded Player namespace subsystem helper, not a C++ Player class member.
 * Purpose: Apply submarine vertical input acceleration, velocity clamp, and neutral-input vertical damping.
 */
void __fastcall UpdateSubVerticalDamping(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->subVerticalInput != 0.0f) {
        if ((playerState->subVerticalInputCopy > 0.0f && playerState->localVel.y < 0.0f)
            || (playerState->subVerticalInputCopy < 0.0f && playerState->localVel.y > 0.0f)) {
            playerState->localVel.y = 0.0f;
        }

        const float localY = masterModalData->accelRate * g_Player_DeltaTime * playerState->subVerticalInputCopy
            + playerState->localVel.y;
        playerState->localVel.y = localY;
        if (localY > 20.0f) {
            playerState->localVel.y = 20.0f;
        } else if (localY < -20.0f) {
            playerState->localVel.y = -20.0f;
        }
        return;
    }

    if (playerState->throttleInputCopy == 0.0f) {
        playerState->localVel.y *= zMath::FastExp(
            -(g_Player_DeltaTime * (g_Time_AccumulatedTimeSec < playerState->primaryGunGateUntilTime ? 2.0f : 10.0f))
        );
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-probemodalsampleheights
 * @recoil-artifact defines .text recoil:function:0x428d60: Player::ProbeModalSampleHeights.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-transform-point
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Source model: bounded Player modal-probe subsystem helper over zUtil_SaveGameState,
 * PlayerModalState, PlayerMasterModalData, accepted zClass/zDI dependencies, and
 * accepted Player/frame/variant/zInput runtime globals; no Player C++ class object or
 * table ownership is required by current BN evidence.
 * Purpose: transform active modal probe points, build scene height candidates, select
 * per-sample impact heights, and publish histogram and attachment outputs.
 */
void __fastcall ProbeModalSampleHeights(
    zUtil_SaveGameState* saveState,
    float* outSampleHeightByPoint,
    float* outBestHeight,
    int preferAttachmentSlot1,
    PlayerProbeTypeHistogram* outTypeHistogram,
    int* outAttachmentCandidateCount,
    CZNodePartial** outAttachmentNode
)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;
    int bestCandidateIndex;
    int selectedImpactSlot;
    float taggedHeight;
    PlayerProbeSampleCandidateBuffer candidateBuffers[PLAYER_MAX_MODAL_PROBE_POINTS];

    memset(outTypeHistogram, 0, sizeof(*outTypeHistogram));
    CZClass::gwNodeSetCellPickable(playerState->rootNode, 0);
    CZClass::gwNodeSetCellPickable(
        ((zUtil_PlayerStateStorage*)((void*)(g_GameStateOrMapTable->playerState)))->rootNode,
        0
    );

    const float probeYAdvance = playerState->projectileSpawnVel.y * g_Player_DeltaTime;
    for (int i = 0; i < primaryModalState->modalStateCode; ++i) {
        ZMTH_VECTOR_TRANSFORM_POINT(
            &playerState->motionBasis,
            &primaryModalState->transformedProbePointWorldByIndex[i],
            &masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + i]
        );
        if (masterModalData->masterType != kPlayerMasterTypeSub) {
            primaryModalState->transformedProbePointWorldByIndex[i].y += probeYAdvance;
        }
    }

    // Retail keeps the rise window at least 4.0 (fld 4.0; fcomp st(1); store 4.0 when larger).
    const float maxRiseWindow = __max(4.0f, 1.0f - probeYAdvance);

    CZClass::gwNodeSetCellPickable(playerState->rootNode, 0);
    CZClass::gwNodeSetCellPickable(
        ((zUtil_PlayerStateStorage*)((void*)(g_GameStateOrMapTable->playerState)))->rootNode,
        0
    );
    g_Variant_CurrentTag = playerState->variantTag;
    CZDisplayInstance::BuildPickCandidatesForPointBatch(
        g_Player_RuntimeDiScene,
        primaryModalState->transformedProbePointWorldByIndex,
        primaryModalState->modalStateCode,
        500.0f,
        candidateBuffers
    );
    g_Variant_CurrentTag = g_VariantTag_Current;
    CZClass::gwNodeSetCellPickable(playerState->rootNode, 1);
    CZClass::gwNodeSetCellPickable(
        ((zUtil_PlayerStateStorage*)((void*)(g_GameStateOrMapTable->playerState)))->rootNode,
        1
    );

    *outBestHeight = kPlayerProbeNoHitHeight;
    *outAttachmentCandidateCount = 0;

    for (int sampleIndex = 0; sampleIndex < primaryModalState->modalStateCode; ++sampleIndex) {
        outSampleHeightByPoint[sampleIndex] = SelectProbeSampleHeightFromCandidates(
            &candidateBuffers[sampleIndex],
            primaryModalState->transformedProbePointWorldByIndex[sampleIndex].y,
            &bestCandidateIndex,
            maxRiseWindow,
            preferAttachmentSlot1,
            &selectedImpactSlot,
            &taggedHeight
        );
        *outBestHeight = __max(*outBestHeight, taggedHeight);

        if (sampleIndex == 0) {
            if (candidateBuffers[0].candidateCount > 0) {
                playerState->selectedProbeSample = candidateBuffers[0].entries[bestCandidateIndex];
                playerState->selectedProbeSample.hitPos.x = primaryModalState->transformedProbePointWorldByIndex[0].x;
                playerState->selectedProbeSample.hitPos.z = primaryModalState->transformedProbePointWorldByIndex[0].z;
                playerState->variantTag = candidateBuffers[0].entries[bestCandidateIndex].variantTag;

                CZNodePartial* const worldChild
                    = CZClass::gwNodeGetWorldChild(candidateBuffers[0].entries[bestCandidateIndex].node);
                if (worldChild != 0) {
                    CZClass::gwNodeSetNodeType(playerState->rootNode, worldChild->nodeType);
                } else {
                    const zTag4Partial candidateTag = candidateBuffers[0].entries[bestCandidateIndex].variantTag;
                    CZClass::gwNodeSetNodeType(playerState->rootNode, candidateTag.tags[0]);
                }
            } else {
                CZClass::gwNodeSetNodeType(playerState->rootNode, 0xff);
            }
        }

        outTypeHistogram->countByImpactSlot[selectedImpactSlot] += 1;

        // Retail reads the best candidate's node without a null test.
        if (candidateBuffers[sampleIndex].candidateCount != 0
            && candidateBuffers[sampleIndex].entries[bestCandidateIndex].node->auxFlags != 0) {
            *outAttachmentCandidateCount += 1;
            *outAttachmentNode
                = (CZNodePartial*)(candidateBuffers[sampleIndex].entries[bestCandidateIndex].node->callbackContext);
        }
    }

    playerState->probeImpactSlot1SeenFlag = outTypeHistogram->countByImpactSlot[1] > 0 ? 1 : 0;
    playerState->probeImpactSlot4SeenFlag = outTypeHistogram->countByImpactSlot[4] > 0 ? 1 : 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-selectprobesampleheightfromcandidates
 * @recoil-artifact defines .text recoil:function:0x4290f0: Player::SelectProbeSampleHeightFromCandidates.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SelectProbeSampleHeightFromCandidates from the recovered
 * Battlesport gameplay source file.
 */
float __fastcall SelectProbeSampleHeightFromCandidates(
    PlayerProbeSampleCandidateBuffer* candidateBuffer,
    float sampleHeight,
    int* outBestCandidateIndex,
    float maxRiseWindow,
    int preferAttachmentSlot1,
    int* outSelectedImpactSlot,
    float* outTaggedHeight
)
{
    float selectedHeight = -250.0f;
    float nearestFallbackHeight = kPlayerProbeNoHitHeight;
    int selectedImpactSlot;

    *outBestCandidateIndex = 0;
    *outSelectedImpactSlot = selectedImpactSlot = 0;
    *outTaggedHeight = kPlayerProbeNoHitHeight;

    float bestAbsDelta = 10000.9f;
    const int candidateCount = candidateBuffer->candidateCount;
    if (candidateCount > 0) {
        for (int i = 0; i < candidateBuffer->candidateCount; ++i) {
            zClassDiPickCandidateEntry* const candidate = &candidateBuffer->entries[i];
            const float candidateHeight = candidate->hitPos.y;
            const int impactSlot
                = candidate->scenePayload != 0 ? ((zModel_MaterialPartial*)candidate->scenePayload)->userTag : 0;

            if (impactSlot != 0) {
                *outTaggedHeight = candidateHeight;
                selectedImpactSlot = impactSlot;
                if (preferAttachmentSlot1 != 0 && impactSlot == 1) {
                    continue;
                }
            }

            const float absDelta = (float)(fabs(candidateHeight - sampleHeight));
            if (absDelta < bestAbsDelta) {
                bestAbsDelta = absDelta;
                nearestFallbackHeight = candidateHeight;
            }

            if (candidateHeight > selectedHeight && candidateHeight - maxRiseWindow <= sampleHeight) {
                selectedHeight = candidateHeight;
                *outBestCandidateIndex = i;
            }
        }

        const float taggedHeight = *outTaggedHeight;
        if (taggedHeight + maxRiseWindow >= sampleHeight) {
            *outSelectedImpactSlot = selectedImpactSlot;
        }

        if (selectedHeight == -250.0f && nearestFallbackHeight != kPlayerProbeNoHitHeight) {
            return nearestFallbackHeight;
        }
        if (selectedHeight <= -250.0f) {
            return -250.0f;
        }
        return selectedHeight;
    }

    return sampleHeight;
}
} // namespace Player
namespace zMath {
/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.player-move.vector-transform-direction-in-place
 *
 * Purpose: Transform a direction in place by the matrix's 3x3 part, without
 * translation; in the raw arm all reads precede the z/y/x binary32 stores.
 * Reconstruction: player_move.cpp-resident copy of the reviewed gmod_pick.c
 * in-place helper (same body as the zmth_main.c copy; retail family 0x4293da,
 * 0x473f6d, 0x47460f, 0x485315); original spelling and declaration location unproved.
 * Raw assembly: identical body to the reviewed gmod_pick.c island.
 * Island contract: EAX/EBX hold vector/matrix from compiler-owned parameter
 * homes and are clobbered; integer flags and the x87 control word unchanged;
 * x87 entry/peak/exit depth 0/6/0 on normal completion; x87 status and
 * exceptions are not preserved. The vector must not overlap the matrix.
 * Consumers are scoped by the raw-assembly allowlist.
 * Retail inline-expansion evidence: the listed consumer contains the operand reloads, arithmetic
 * sequence and result stores without a call at that site; the original inline helper's header
 * ownership and declaration placement are not established (TU-resident reconstruction model).
 * Original inline helper evidence: no standalone retail function; observed at
 * retail 0x4293d4 in 0x429240.
 */
inline void Vec3TransformDirectionInPlace(const zMat4x3* matrix, zVec3* vector)
{
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov eax, vector
        mov ebx, matrix
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xx
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xy
        fld dword ptr [eax]zVec3.x
        fmul dword ptr [ebx]zMat4x3.xz
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yx
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yy
        fld dword ptr [eax]zVec3.y
        fmul dword ptr [ebx]zMat4x3.yz
        fxch st(2)
        faddp st(5), st
        faddp st(3), st
        faddp st(1), st
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zx
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zy
        fld dword ptr [eax]zVec3.z
        fmul dword ptr [ebx]zMat4x3.zz
        fxch st(2)
        faddp st(5), st
        faddp st(3), st
        faddp st(1), st
        fstp dword ptr [eax]zVec3.z
        fstp dword ptr [eax]zVec3.y
        fstp dword ptr [eax]zVec3.x
    }
#else
    const zVec3 source = *vector;
    vector->x = source.x * matrix->xx + source.y * matrix->yx + source.z * matrix->zx;
    vector->y = source.x * matrix->xy + source.y * matrix->yy + source.z * matrix->zy;
    vector->z = source.x * matrix->xz + source.y * matrix->yz + source.z * matrix->zz;
#endif
}
} // namespace zMath

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applyamphibspeedoscillation
 * @recoil-artifact defines .text recoil:function:0x429240: Player::ApplyAmphibSpeedOscillation.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.sin-cos
 * @recoil-raw-consumer recoil:raw-asm:battlesport.player-move.vector-transform-direction-in-place
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ApplyAmphibSpeedOscillation from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall
ApplyAmphibSpeedOscillation(zUtil_SaveGameState* saveState, zVec3* inOutUpVector, int includeYawCoupling)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    const float pitchArg = (masterModalData->hoverPitchWaveSpeedRate * fabs(playerState->localVel.z)
                               + masterModalData->hoverPitchWaveBaseRate)
        * g_Time_AccumulatedTimeSec;
    const float rollArg = (masterModalData->hoverRollWaveSpeedRate * fabs(playerState->localVel.z)
                              + masterModalData->hoverRollWaveBaseRate)
        * g_Time_AccumulatedTimeSec;

    const float pitchAngle = (float)(sin(pitchArg)) * masterModalData->hoverPitchWaveAmplitude;
    float rollAngle = (float)(sin(rollArg)) * masterModalData->hoverRollWaveAmplitude;
    if (includeYawCoupling != 0) {
        rollAngle += playerState->angVelYaw * masterModalData->hoverRollYawCoupleScale * playerState->localVel.z;
    }

    const float yawSin = -playerState->steerBasisNorm.x;
    const float yawCos = -playerState->steerBasisNorm.z;
    float pitchSin;
    float pitchCos;
    zMath::SinCos(pitchAngle, &pitchSin, &pitchCos);
    float rollSin;
    float rollCos;
    zMath::SinCos(rollAngle, &rollSin, &rollCos);

    zMat4x3 oscillationBasis;
    oscillationBasis.xx = yawSin * pitchSin * rollSin + rollCos * yawCos;
    oscillationBasis.xy = rollSin * pitchCos;
    oscillationBasis.xz = rollSin * yawCos * pitchSin - rollCos * yawSin;
    oscillationBasis.yx = yawSin * pitchSin * rollCos - rollSin * yawCos;
    oscillationBasis.yy = rollCos * pitchCos;
    oscillationBasis.yz = rollCos * yawCos * pitchSin + rollSin * yawSin;
    oscillationBasis.zx = yawSin * pitchCos;
    oscillationBasis.zy = -pitchSin;
    oscillationBasis.zz = yawCos * pitchCos;

    zMath::Vec3TransformDirectionInPlace(&oscillationBasis, inOutUpVector);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applypitchrollvelocityimpulsefromdirection
 * @recoil-artifact defines .text recoil:function:0x429430: Player::ApplyPitchRollVelocityImpulseFromDirection
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: transform an incoming hit direction into player-local space and apply the matching pitch/roll and local X/Z
 * velocity impulse. Source owner: Player damage-hit and destroyed-state callback subsystem, not a standalone C++ Player
 * class owner. Evidence: status names this address-backed helper; body loads the root-node 3x3 rotation, transforms one
 * direction vector, then applies the scaled local X/Z components to vehicle pitch, roll, and local velocity.
 */
void __fastcall ApplyPitchRollVelocityImpulseFromDirection(
    zUtil_SaveGameState* saveState,
    const zVec3* direction,
    float angleScale,
    float velocityScale
)
{
    zVec3 localDirection = *direction;
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadRotationFrom3x3((const zMat4x3*)(CZObject3D::gwObject3DGetMatrixPtr(playerState->rootNode)));
    zMath::Vec3ArrayTransformDirection(&localDirection, 1);
    zMath::MatStackPopPtr();

    playerState->vehiclePitchRad -= localDirection.z * angleScale;
    playerState->vehicleRollRad += localDirection.x * angleScale;
    playerState->localVel.x -= localDirection.x * velocityScale;
    playerState->localVel.z -= localDirection.z * velocityScale;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildsteerbasisfrommotionbasis
 * @recoil-artifact defines .text recoil:function:0x4294d0: Player::RebuildSteerBasisFromMotionBasis.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::RebuildSteerBasisFromMotionBasis from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildSteerBasisFromMotionBasis(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;

    playerState->steerBasisRaw.x = -playerState->motionBasis.zx;
    playerState->steerBasisRaw.y = -playerState->motionBasis.zy;
    playerState->steerBasisRaw.z = -playerState->motionBasis.zz;

    playerState->steerBasisRef.x = playerState->motionBasis.yx;
    playerState->steerBasisRef.y = playerState->motionBasis.yy;
    playerState->steerBasisRef.z = playerState->motionBasis.yz;

    playerState->steerBasisNorm = playerState->steerBasisRaw;
    playerState->steerBasisNorm.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->steerBasisNorm, &playerState->steerBasisNorm);
}
} // namespace Player
/**
 * Reconstruction model: inline cross-product Y component. Retail 0x4295be..0x4295d6
 * loads steer.z and target.z and multiplies them by target.x and steer.x memory
 * operands; VC5 orders those operands this way only through the inline-expanded
 * pointer parameters. Original-source helper status is inferred; helper spelling and placement are not established; no
 * standalone retail function exists.
 * Purpose: return the y component of the cross product a x b.
 */
inline float Vec3CrossY(const zVec3* a, const zVec3* b)
{
    return a->z * b->x - a->x * b->z;
}

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildsteerbasisfrommotionaxes
 * @recoil-artifact defines .text recoil:function:0x429560: Player::RebuildSteerBasisFromMotionAxes.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 *
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::RebuildSteerBasisFromMotionAxes from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildSteerBasisFromMotionAxes(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->autoTurnActive == 0) {
        return;
    }

    if (playerState->steeringInput != 0.0f) {
        playerState->autoTurnActive = 0;
        if (saveState == g_LocalPlayerSaveState) {
            ApplyCameraState(playerState->previousCameraState);
        }
    }

    if (playerState->autoTurnActive == 0) {
        return;
    }

    const float cross = Vec3CrossY(&playerState->steerBasisNorm, &playerState->autoTurnTargetDir);
    const float dot = playerState->steerBasisNorm.x * playerState->autoTurnTargetDir.x
        + playerState->steerBasisNorm.z * playerState->autoTurnTargetDir.z;
    if (dot < (float)(cos(g_Player_DeltaTime * masterModalData->yawRateMax))) {
        const int turnSign = cross < 0.0f ? -1 : 1;
        const float turnSignFloat = (float)(turnSign);
        playerState->steeringInput = turnSignFloat;
        playerState->steeringInputCopy = turnSignFloat;
        playerState->angVelYaw = turnSignFloat * masterModalData->yawRateMax;

        if (saveState == g_LocalPlayerSaveState && playerState->lifecycleState != 2) {
            zVec3 normalizedCursor;
            HudUiMgr::ProjectPointToNormalizedClamped(&playerState->autoTurnTargetWorldPos, &normalizedCursor);
            playerState->autoTurnCursorNormX = normalizedCursor.x;
            playerState->autoTurnCursorNormY = normalizedCursor.y;
            zInput::MouseSetNormalizedCursorPos(normalizedCursor.x, normalizedCursor.y);

            zMath::Vec3Lerp(
                &playerState->cameraLerpStart,
                &playerState->cameraLerpEnd,
                zMath::FastExp(g_FrameDeltaTimeSec * -2.0f)
            );
        }
        return;
    }

    playerState->thirdPersonYawOffset = 0.0f;
    playerState->cameraDirFlat = playerState->cameraDir;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(&playerState->cameraDirFlat, &playerState->cameraDirFlat);

    if (saveState == (zUtil_SaveGameState*)(g_GameStateOrMapTable) && saveState == g_LocalPlayerSaveState) {
        ApplyCameraState(playerState->previousCameraState);
        zInput::MouseRecenterCursorX();
    }

    // Retail passes -x as atan2's y operand (fpatan with -x in st(1)).
    playerState->restartYawRad = (float)(atan2(-playerState->autoTurnTargetDir.x, -playerState->autoTurnTargetDir.z));
    playerState->autoTurnActive = 0;
    playerState->steeringInputCopy = 0.0f;
    playerState->angVelYaw = 0.0f;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updateautoturnandsteerfromtarget
 * @recoil-artifact defines .text recoil:function:0x429750: Player::UpdateAutoTurnAndSteerFromTarget
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-match source
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: damp yaw angular velocity when steering is neutral, otherwise apply
 * steering yaw acceleration and clamp it to the active yaw velocity limit.
 * Source owner: proposed Player auto-turn yaw steering helper; owner/data gates
 * are still pending outside this docblock-only edit.
 * Evidence: status names this address-backed helper; body branches on steering
 * input, builds the recovered yaw-damping scale, zeroes opposing yaw velocity,
 * accumulates yaw acceleration from steering input, and clamps angVelYaw.
 */
void __fastcall UpdateAutoTurnAndSteerFromTarget(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    if (playerState->steeringInput != 0.0f) {
        if ((playerState->steeringInputCopy > 0.0f && playerState->angVelYaw < 0.0f)
            || (playerState->steeringInputCopy < 0.0f && playerState->angVelYaw > 0.0f)) {
            playerState->angVelYaw = 0.0f;
        }

        const float newYawVelocity
            = masterModalData->yawAccel * g_Player_DeltaTime * playerState->steeringInputCopy + playerState->angVelYaw;
        playerState->angVelYaw = newYawVelocity;

        const float yawVelocityLimit = playerState->yawVelocityLimit;
        if (newYawVelocity > yawVelocityLimit) {
            playerState->angVelYaw = yawVelocityLimit;
        } else if (newYawVelocity < -yawVelocityLimit) {
            playerState->angVelYaw = -yawVelocityLimit;
        }
    } else {
        playerState->angVelYaw *= zMath::FastExp(-(masterModalData->yawDamping * g_Player_DeltaTime));
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updateyawvelocityfromsteerinput
 * @recoil-artifact defines .text recoil:function:0x429870: Player::UpdateYawVelocityFromSteerInput.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-match source
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::UpdateYawVelocityFromSteerInput from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateYawVelocityFromSteerInput(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    // Retail compares the float magnitude against the scaled threshold (fcomp m32).
    if ((float)(fabs(playerState->localVel.x)) < g_Player_DeltaTimeScaled001) {
        playerState->localVel.x = 0.0f;
    }

    if ((float)(fabs(playerState->localVel.z)) < g_Player_DeltaTimeScaled001) {
        playerState->localVel.z = 0.0f;
    }

    if (playerState->slipSfxActive != 0) {
        ComputeTurnSlipDelta(saveState);
        return;
    }

    if (playerState->throttleInput != 0.0f) {
        if (playerState->throttleInputCopy > 0.0f && playerState->localVel.z > 0.0f) {
            playerState->localVel.z *= zMath::FastExp(-(masterModalData->rateDampingDecel * g_Player_DeltaTime));
        } else if (playerState->throttleInputCopy < 0.0f && playerState->localVel.z < 0.0f) {
            playerState->localVel.z *= zMath::FastExp(-(masterModalData->rateDampingDecel * g_Player_DeltaTime));
        }

        const float localZ = playerState->localVel.z
            - masterModalData->accelRate * g_Player_DeltaTime * playerState->throttleInputCopy;
        playerState->localVel.z = localZ;
        const float velocityLimit = (float)(fabs(playerState->throttleInputCopy)) * playerState->axisClampRuntime;
        if (localZ > velocityLimit) {
            playerState->localVel.z = velocityLimit;
        } else if (localZ < -velocityLimit) {
            playerState->localVel.z = -velocityLimit;
        }
    } else {
        playerState->localVel.z *= zMath::FastExp(-(masterModalData->rateDampingDecel * g_Player_DeltaTime));
    }

    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        float residual;
        if ((residual = UpdateBankAndTurnDynamics(saveState)) != 0.0f) {
            float localX = residual * g_Player_DeltaTime + playerState->localVel.x;
            // Reconstruction spelling that reproduces retail's register comparison
            // (fld limit; fld st(1); fcompp) under this profile.
            if ((double)playerState->axisClampRuntime < localX) {
                localX = playerState->axisClampRuntime;
            } else if (localX < -playerState->axisClampRuntime) {
                localX = -playerState->axisClampRuntime;
            }

            if (playerState->localVel.x != 0.0f) {
                const int oldLocalXSign = playerState->localVel.x < 0.0f ? -1 : 1;
                const int localXSign = localX < 0.0f ? -1 : 1;
                if (oldLocalXSign != localXSign) {
                    localX = 0.0f;
                }
            }

            playerState->localVel.x = localX;
            return;
        }
    }

    if (playerState->localVel.x != 0.0f) {
        playerState->localVel.x *= zMath::FastExp(-(masterModalData->rateDampingAccel * g_Player_DeltaTime));
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatebankandturndynamics
 * @recoil-artifact defines .text recoil:function:0x429b40: Player::UpdateBankAndTurnDynamics.
 *
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::UpdateBankAndTurnDynamics from the recovered
 * Battlesport gameplay source file.
 */
float __fastcall UpdateBankAndTurnDynamics(zUtil_SaveGameState* saveState)
{
    if (g_Player_DeltaTime < 0.0000001) {
        return 0.0f;
    }

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    const float crossYaw = playerState->steerBasisNorm.x * playerState->bankBasis.z
        - playerState->steerBasisNorm.z * playerState->bankBasis.x;
    const float slipDelta
        = crossYaw * -playerState->localVel.z * g_Player_InvDeltaTime + playerState->motionBasis.xy * -28.0f;

    // Retail falls through to the shared return when the static slip is within
    // friction (+0xe4 jne 0x1e3) and duplicates the epilogue for the sign path.
    float residual = 0.0f;
    if (playerState->localVel.x == 0.0f) {
        if (fabs(slipDelta) > masterModalData->frictionStatic) {
            const int sign = slipDelta < 0.0f ? -1 : 1;
            residual = slipDelta - (float)(sign)*masterModalData->frictionStatic;
            StartSlipSfx(saveState);
        }
    } else {
        residual = slipDelta - (float)(FloatSign(playerState->localVel.x)) * masterModalData->frictionDynamic;

        if (playerState->throttleInputCopy != 0.0f
            && FloatSign(playerState->steeringInputCopy) == FloatSign(playerState->restartYawRad)) {
            const int residualSign = residual < 0.0f ? -1 : 1;
            const int velocitySign = playerState->localVel.x < 0.0f ? -1 : 1;
            if (residualSign != velocitySign) {
                residual = 0.0f;
            }
        }

        if (playerState->slipSfxActive == 0 && fabs(slipDelta) > masterModalData->frictionStatic) {
            StartSlipSfx(saveState);
        }
    }

    return residual;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computeturnslipdelta
 * @recoil-artifact defines .text recoil:function:0x429d30: Player::ComputeTurnSlipDelta.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-rotate-rows-in-place
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: src/Battlesport/player.cpp.
 * Purpose: reimplement Player::ComputeTurnSlipDelta from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeTurnSlipDelta(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    playerState->localVel = playerState->projectileSpawnVel;
    ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE(&playerState->motionBasis, &playerState->localVel);

    const float localZ
        = playerState->localVel.z - masterModalData->accelRate * playerState->throttleInputCopy * g_Player_DeltaTime;
    const float axisClampRuntime = playerState->axisClampRuntime;
    playerState->localVel.z = localZ;
    if (localZ > axisClampRuntime) {
        playerState->localVel.z = axisClampRuntime;
    } else if (localZ < -axisClampRuntime) {
        playerState->localVel.z = -axisClampRuntime;
    }

    float localX = playerState->localVel.x + UpdateBankAndTurnDynamics(saveState) * g_Player_DeltaTime;
    if (playerState->localVel.x != 0.0f) {
        const int oldSign = playerState->localVel.x < 0.0f ? -1 : 1;
        const int newSign = localX < 0.0f ? -1 : 1;
        if (oldSign != newSign) {
            localX = 0.0f;
            StopSlipSfx(saveState);
        }
    }

    if ((playerState->localVel.x = localX) > axisClampRuntime) {
        playerState->localVel.x = axisClampRuntime;
    } else if (localX < -axisClampRuntime) {
        playerState->localVel.x = -axisClampRuntime;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-startslipsfx
 * @recoil-artifact defines .text recoil:function:0x429ed0: Player::StartSlipSfx.
 * @recoil-match byte
 *
 * Purpose: Start the player slip sound and update its active flag.
 */
void __fastcall StartSlipSfx(zUtil_SaveGameState* saveState)
{
    saveState->playerState->slipSfxActive = 1;
    saveState->StartModalLoopSfxHandle(3, 1.0f);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-stopslipsfx
 * @recoil-artifact defines .text recoil:function:0x429ef0: Player::StopSlipSfx.
 * @recoil-match byte
 *
 * Purpose: Stop the player slip sound and update its active flag.
 */
void __fastcall StopSlipSfx(zUtil_SaveGameState* saveState)
{
    saveState->playerState->slipSfxActive = 0;
    saveState->StopModalLoopSfxHandle(3);
}
} // namespace Player
