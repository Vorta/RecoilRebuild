// Battlesport compilation unit between player_state.cpp and Recoil.cpp, inferred
// from the retail object boundary [0x42bf90, 0x42db50): its probe-mask table and
// float constants [0x4d0870, 0x4d08f0) repeat 0.0f, 1.0f, -1.0f, -5.0f and 5.0f
// that the neighbouring objects pool separately. Original filename unresolved;
// player_terrain.cpp is a provisional name (2026-10-02).

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

/**
 * VC5 C1 draws declarations, labels and temporaries from one translation-unit
 * ID counter, and 0x42cbd0 order(s) code by that counter's
 * parity at parse time. These declarations are never referenced and emit no code,
 * data or symbols. User-authorized exception: match-proofs.md "Per-TU VC5
 * ID-counter parity exception".
 * Purpose: keep this file's ID-counter parity after shared-header changes.
 */
extern int g_PlayerTerrainIdCounterAlignment0;
extern int g_PlayerTerrainIdCounterAlignment1;
extern int g_PlayerTerrainIdCounterAlignment2;
extern int g_PlayerTerrainIdCounterAlignment3;
extern char g_HudUiCounterText_PlayerLabel[];

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerenvprobesamplecount
 * @recoil-artifact defines .data recoil:data:0x4f3bc8: g_PlayerEnvProbeSampleCount.
 * Data owner 0x4f3bc8..0x4f3c8f: zero-initialized Player post-move environment probe globals.
 * BN exposes seven live world-point samples; the remaining zero bytes in this owner are
 * bounded padding.
 * Purpose: stores the plan-tracked g_PlayerEnvProbeSampleCount gameplay data symbol.
 */
int g_PlayerEnvProbeSampleCount = 0;
unsigned char g_PlayerEnvProbeSampleCountPadding[4] = { 0 };
int g_PlayerEnvProbe_AboveGroundFlags[10] = { 0 };
int g_PlayerEnvProbe_AboveGroundIndices[10] = { 0 };
zVec3 g_PlayerEnvProbeWorldPoints[7] = { 0 };
unsigned char g_PlayerEnvProbeWorldPointsTailPadding[24] = { 0 };
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-playerenvprobe-abovegroundcount
 * @recoil-artifact defines .data recoil:data:0x4f3c8c: g_PlayerEnvProbe_AboveGroundCount.
 * Purpose: stores the plan-tracked g_PlayerEnvProbe_AboveGroundCount gameplay data symbol.
 */
int g_PlayerEnvProbe_AboveGroundCount = 0;
} // extern "C"
namespace {
/**
 * Original inline helper; no standalone retail function exists. Observed in address-backed callers 0x4386c0, 0x4289f0,
 * 0x42c0d0, 0x42c2e0, 0x427440, 0x427ec0, 0x43a600, and 0x43a900 as a VC5-era int-bits smoothing idiom. Purpose:
 * reinterpret an IEEE-754 bit pattern as float.
 */
#define PLAYER_FLOAT_FROM_BITS(bits) (*(const float*)&(bits))
enum PlayerMasterTypeId {
    kPlayerMasterTypeFly = 1,
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3,
    kPlayerMasterTypeHover = 4,
    kPlayerMasterTypeAmphib = 5
};
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed callers 0x426770 Player::UpdateMasterTypeTrack, 0x42d5c0
 * Player::ApplyEnvironmentProbeResult. Purpose: provide the recovered cache attachment local offset helper for the
 * Player/Pickup gameplay source cluster.
 */
#define PLAYER_CACHE_ATTACHMENT_LOCAL_OFFSET(playerState)                                                              \
    do {                                                                                                               \
        zMath::Vec3Subtract(                                                                                           \
            &(playerState)->worldPos,                                                                                  \
            (const zVec3*)(&(playerState)->environmentAttachmentMatrix.posX),                                          \
            &(playerState)->environmentAttachmentLocalOffset                                                           \
        );                                                                                                             \
        ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE(                                                                              \
            &(playerState)->environmentAttachmentMatrix,                                                               \
            &(playerState)->environmentAttachmentLocalOffset                                                           \
        );                                                                                                             \
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

const int g_PlayerEnvProbeSampleMaskTable[8] = { 0x89, 0x43, 0x86, 0x4c, 0x28, 0x22, 0xf0, 0x00 };
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updatepostmoveenvironment
 * @recoil-artifact defines .text recoil:function:0x42bf90: Player::UpdatePostMoveEnvironment.
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdatePostMoveEnvironment from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdatePostMoveEnvironment(zUtil_SaveGameState* saveState, int probeSampleCount)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    g_PlayerEnvProbeSampleCount = probeSampleCount;

    PlayerEnvProbeResult probeResult;
    memset(&probeResult, 0, sizeof(probeResult));
    probeResult.minProbeDepth = 4.0f;
    probeResult.preferAttachmentSlot1 = playerState->amphibUnlocked == 0 ? 1 : 0;

    const float restartYawRad = playerState->restartYawRad;
    playerState->vehiclePitchRad += playerState->angVelPitch * g_Player_DeltaTime;
    playerState->vehicleRollRad += playerState->angVelRoll * g_Player_DeltaTime;
    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posZ = playerState->worldPos.z;

    RebuildSteerBasisFromMotionBasis(saveState);

    const float verticalVelocityAfterGravity
        = playerState->projectileSpawnVel.y - playerState->gravityAccel * g_Player_DeltaTime;
    playerState->projectileSpawnVel.y = verticalVelocityAfterGravity;
    const float advancedWorldPosY = playerState->worldPos.y + verticalVelocityAfterGravity * g_Player_DeltaTime;
    playerState->worldPos.y = advancedWorldPosY;
    playerState->motionBasis.posY = advancedWorldPosY;

    BuildEnvironmentProbeResult(saveState, &probeResult);
    if (ApplyEnvironmentProbeResult(saveState, &probeResult) == 0) {
        return;
    }

    ProcessEnvProbeResults(saveState, &probeResult);
    RebuildOrientationFromNormal(saveState);
    if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable && playerState->airborneFlag == 0) {
        FindThirdProbeAndComputeNormal(saveState, &probeResult);
    }
    UpdateVerticalVelocityAndTransform(saveState, &probeResult);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-processenvproberesults
 * @recoil-artifact defines .text recoil:function:0x42c0d0: Player::ProcessEnvProbeResults.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ProcessEnvProbeResults from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ProcessEnvProbeResults(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* probeResult)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    const float supportDepthThreshold = g_Player_DeltaTime * 5.0f;
    g_PlayerEnvProbe_AboveGroundCount = 0;

    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        const int impactSlot = probeResult->impactSlotBySample[sampleIndex];
        if (impactSlot == 3) {
            probeResult->candidateScoreBySample[sampleIndex] -= g_Player_QuicksandSinkRate;
        } else if (impactSlot == 4) {
            probeResult->candidateScoreBySample[sampleIndex] -= g_Player_LavaSinkRate;
        }

        // A sample supports the vehicle when its ground height reaches the probe's support depth.
        if (g_PlayerEnvProbeWorldPoints[sampleIndex].y - supportDepthThreshold
            <= probeResult->candidateScoreBySample[sampleIndex]) {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 1;
            g_PlayerEnvProbe_AboveGroundIndices[g_PlayerEnvProbe_AboveGroundCount] = sampleIndex;
            ++g_PlayerEnvProbe_AboveGroundCount;
        } else {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 0;
        }
    }

    if (g_PlayerEnvProbe_AboveGroundCount == 0) {
        playerState->airborneFlag = 1;
        const float unclampedPitchRecoveryVel = (playerState->vehiclePitchRad - -0.523599982f) * -0.699999988f;
        const float targetPitchRecoveryVel
            = unclampedPitchRecoveryVel > -0.699999988f ? unclampedPitchRecoveryVel : -0.699999988f;
        const float targetRollRecoveryVel = playerState->vehicleRollRad * -0.699999988f;
        const float previousAngularVelocityBlendWeight
            = zMath::FastExp(-(saveState->primaryModalState->masterModalData->aDamping * g_Player_DeltaTime));
        const float newAngularVelocityBlendWeight = 1.0f - previousAngularVelocityBlendWeight;
        playerState->angVelPitch = previousAngularVelocityBlendWeight * playerState->angVelPitch
            + newAngularVelocityBlendWeight * targetPitchRecoveryVel;
        playerState->angVelRoll = previousAngularVelocityBlendWeight * playerState->angVelRoll
            + newAngularVelocityBlendWeight * targetRollRecoveryVel;
        return;
    }

    if (g_PlayerEnvProbe_AboveGroundCount == 1) {
        ComputeSurfaceFrom1Probe(saveState, probeResult);
    } else if (g_PlayerEnvProbe_AboveGroundCount == 2) {
        ComputeSurfaceFrom2Probes(saveState, probeResult);
    } else if (g_PlayerEnvProbe_AboveGroundCount == 3) {
        if (CheckProbeSampleMaskOverlap(
                g_PlayerEnvProbe_AboveGroundIndices[0],
                g_PlayerEnvProbe_AboveGroundIndices[1],
                g_PlayerEnvProbe_AboveGroundIndices[2]
            )
            != 0) {
            ComputeSurfaceFrom2Probes(saveState, probeResult);
        } else {
            ComputeSurfaceFrom3Probes(saveState, probeResult);
        }
    } else {
        SelectBestProbesByDotProduct(&playerState->steerBasisRef, probeResult);
        ComputeSurfaceFrom3Probes(saveState, probeResult);
    }
    playerState->airborneFlag = 0;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-updateverticalvelocityandtransform
 * @recoil-artifact defines .text recoil:function:0x42c2e0: Player::UpdateVerticalVelocityAndTransform.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-rotate-rows-in-place
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::UpdateVerticalVelocityAndTransform from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateVerticalVelocityAndTransform(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* probeResult)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    const float measuredFrameDeltaY
        = (playerState->worldPos.y - playerState->previousTransform.posY) * g_Player_InvDeltaTime;
    if (g_PlayerEnvProbe_AboveGroundCount < 3) {
        const float previousVerticalVelocityBlendWeight = zMath::FastExp(g_Player_DeltaTime * -5.0f);
        playerState->projectileSpawnVel.y = previousVerticalVelocityBlendWeight * playerState->projectileSpawnVel.y
            + (1.0f - previousVerticalVelocityBlendWeight) * measuredFrameDeltaY;
    } else {
        playerState->projectileSpawnVel.y = measuredFrameDeltaY;
    }

    AccumulateSlopeForces(saveState, probeResult);
    if (playerState->projectileSpawnVel.y > 55.0f) {
        playerState->projectileSpawnVel.y = 0.0f;
    }

    if (playerState->environmentAttachmentActive != 0) {
        return;
    }

    playerState->localVel = playerState->projectileSpawnVel;
    ZMTH_VECTOR_ROTATE_ROWS_IN_PLACE(&playerState->motionBasis, &playerState->localVel);
    if (g_PlayerEnvProbe_AboveGroundCount >= 3) {
        playerState->localVel.y = 0.0f;
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-accumulateslopeforces
 * @recoil-artifact defines .text recoil:function:0x42c420: Player::AccumulateSlopeForces.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Purpose: add downhill force from up to three selected terrain-contact normals.
 */
void __fastcall AccumulateSlopeForces(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* probeResult)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    int clampedAboveGroundCount = g_PlayerEnvProbe_AboveGroundCount;
    if (clampedAboveGroundCount >= 3) {
        clampedAboveGroundCount = 3;
    }
    for (int i = 0; i < clampedAboveGroundCount; ++i) {
        const int sampleIndex = g_PlayerEnvProbe_AboveGroundIndices[i];
        const int bestCandidateIndex = probeResult->bestIndexBySample[sampleIndex];
        const zVec3 surfaceNormal
            = probeResult->candidateBuffers[sampleIndex].entries[bestCandidateIndex].surfaceNormal;
        if (!(surfaceNormal.y >= g_Player_MaxSlope)) {
            zVec3 force;
            force.x = surfaceNormal.x * (g_Player_DeltaTime * playerState->gravityAccel) * 5.0f;
            force.z = surfaceNormal.z * (g_Player_DeltaTime * playerState->gravityAccel) * 5.0f;
            force.y = (surfaceNormal.y - 1.0f) * g_Player_DeltaTime * playerState->gravityAccel * 5.0f;
            /**
             * Purpose: preserve the reviewed vector-add operation after storing the force.
             */
            zMath::Vec3Add(&playerState->projectileSpawnVel, &force, &playerState->projectileSpawnVel);
        }
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computesurfacefrom1probe
 * @recoil-artifact defines .text recoil:function:0x42c520: Player::ComputeSurfaceFrom1Probe.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ComputeSurfaceFrom1Probe from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeSurfaceFrom1Probe(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* probeResult)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zVec3 samplePoint = g_PlayerEnvProbeWorldPoints[g_PlayerEnvProbe_AboveGroundIndices[0]];
    samplePoint.y = probeResult->candidateScoreBySample[g_PlayerEnvProbe_AboveGroundIndices[0]];

    float supportPlaneDot;
    ZMTH_VECTOR_DOT(supportPlaneDot, &playerState->steerBasisRef, &samplePoint);
    playerState->worldPos.y = SolveHeightOnSurface(saveState, supportPlaneDot);

    zVec3 sampleOffsetFromPlayer;
    zMath::Vec3Subtract(&samplePoint, &playerState->worldPos, &sampleOffsetFromPlayer);
    zVec3 tiltVector;
    ZMTH_VECTOR_CROSS(&sampleOffsetFromPlayer, &playerState->steerBasisRef, &tiltVector);
    ApplyTerrainTilt(saveState, &tiltVector, 1.0f);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computesurfacefrom2probes
 * @recoil-artifact defines .text recoil:function:0x42c640: Player::ComputeSurfaceFrom2Probes.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ComputeSurfaceFrom2Probes from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeSurfaceFrom2Probes(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* probeResult)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    const int sampleIndexB = g_PlayerEnvProbe_AboveGroundIndices[1];
    const int sampleIndexA = g_PlayerEnvProbe_AboveGroundIndices[0];

    zVec3 pointA = g_PlayerEnvProbeWorldPoints[sampleIndexA];
    pointA.y = probeResult->candidateScoreBySample[sampleIndexA];
    float pointASupportDot;
    ZMTH_VECTOR_DOT(pointASupportDot, &playerState->steerBasisRef, &pointA);

    zVec3 pointB = g_PlayerEnvProbeWorldPoints[sampleIndexB];
    pointB.y = SolveHeightOnSurface(saveState, pointASupportDot);

    zVec3 supportEdge;
    zMath::Vec3Subtract(&pointB, &pointA, &supportEdge);
    zVec3 perpOffset;
    ZMTH_VECTOR_CROSS(&playerState->steerBasisRef, &supportEdge, &perpOffset);
    zVec3 pointC;
    zMath::Vec3Add(&pointA, &perpOffset, &pointC);

    pointB.y = probeResult->candidateScoreBySample[sampleIndexB];
    ComputeTriangleNormal(saveState, &pointA, &pointB, &pointC);

    float surfaceDot;
    ZMTH_VECTOR_DOT(surfaceDot, &playerState->steerBasisRef, &pointA);
    playerState->worldPos.y = SolveHeightOnSurface(saveState, surfaceDot);

    zMath::Vec3Normalize(&supportEdge);
    zMath::Vec3Subtract(&pointA, &playerState->worldPos, &pointA);
    ZMTH_VECTOR_CROSS(&playerState->steerBasisRef, &supportEdge, &pointC);
    float tiltScale;
    ZMTH_VECTOR_DOT(tiltScale, &pointA, &pointC);
    ApplyTerrainTilt(saveState, &supportEdge, tiltScale);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applyterraintilt
 * @recoil-artifact defines .text recoil:function:0x42c8d0: Player::ApplyTerrainTilt.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ApplyTerrainTilt from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ApplyTerrainTilt(zUtil_SaveGameState* saveState, const zVec3* tiltVector, float tiltScale)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    const float tiltFactor = (g_Player_NominalGravity / playerState->gravityAccel) * tiltScale;
    zVec3 rotatedTilt;
    rotatedTilt.x = tiltVector->x * tiltFactor;
    rotatedTilt.y = tiltVector->y * tiltFactor;
    rotatedTilt.z = tiltVector->z * tiltFactor;
    zMath::Vec3RotateY(&rotatedTilt, &rotatedTilt, -playerState->restartYawRad);

    if (playerState->airborneFlag != 0) {
        ResetTerrainContactImpulsesAndPlayImpactSfx(saveState);
    }

    playerState->angVelPitch += rotatedTilt.x;
    playerState->angVelRoll += rotatedTilt.z;
    if (playerState->angVelPitch > 1.20000005f) {
        playerState->angVelPitch = 1.20000005f;
    } else if (playerState->angVelPitch < -1.20000005f) {
        playerState->angVelPitch = -1.20000005f;
    }
    if (playerState->angVelRoll > 1.20000005f) {
        playerState->angVelRoll = 1.20000005f;
    } else if (playerState->angVelRoll < -1.20000005f) {
        playerState->angVelRoll = -1.20000005f;
    }

    // The rotated tilt's storage is reused for the downhill velocity impulse.
    rotatedTilt = playerState->steerBasisRef;
    rotatedTilt.y = 0.0f;
    const float velocityScale = g_Player_DeltaTime * playerState->gravityAccel * 5.0f;
    rotatedTilt.x *= velocityScale;
    rotatedTilt.y *= velocityScale;
    rotatedTilt.z *= velocityScale;
    zMath::Vec3Add(&playerState->projectileSpawnVel, &rotatedTilt, &playerState->projectileSpawnVel);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computesurfacefrom3probes
 * @recoil-artifact defines .text recoil:function:0x42ca40: Player::ComputeSurfaceFrom3Probes.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ComputeSurfaceFrom3Probes from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ComputeSurfaceFrom3Probes(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* probeResult)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    if (playerState->airborneFlag != 0) {
        ResetTerrainContactImpulsesAndPlayImpactSfx(saveState);
    }

    const int sampleIndexA = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int sampleIndexB = g_PlayerEnvProbe_AboveGroundIndices[1];
    const int sampleIndexC = g_PlayerEnvProbe_AboveGroundIndices[2];
    zVec3 probePointA = g_PlayerEnvProbeWorldPoints[sampleIndexA];
    probePointA.y = probeResult->candidateScoreBySample[sampleIndexA];
    zVec3 probePointB = g_PlayerEnvProbeWorldPoints[sampleIndexB];
    probePointB.y = probeResult->candidateScoreBySample[sampleIndexB];
    zVec3 probePointC = g_PlayerEnvProbeWorldPoints[sampleIndexC];
    probePointC.y = probeResult->candidateScoreBySample[sampleIndexC];

    ComputeTriangleNormal(saveState, &probePointA, &probePointB, &probePointC);
    float surfaceDot;
    ZMTH_VECTOR_DOT(surfaceDot, &playerState->steerBasisRef, &probePointA);
    playerState->worldPos.y = SolveHeightOnSurface(saveState, surfaceDot);
    playerState->angVelPitch = 0.0f;
    playerState->angVelRoll = 0.0f;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resetterraincontactimpulsesandplayimpactsfx
 * @recoil-artifact defines .text recoil:function:0x42cb50: Player::ResetTerrainContactImpulsesAndPlayImpactSfx.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ResetTerrainContactImpulsesAndPlayImpactSfx from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall ResetTerrainContactImpulsesAndPlayImpactSfx(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    playerState->angVelRoll = 0.0f;
    playerState->angVelPitch = 0.0f;

    if (saveState != (zUtil_SaveGameState*)g_GameStateOrMapTable) {
        return;
    }

    float sfxVolume = (float)(fabs(playerState->projectileSpawnVel.y * 0.100000001f));
    if (sfxVolume > 1.0f) {
        sfxVolume = 1.0f;
    } else if (sfxVolume < 0.0f) {
        sfxVolume = 0.0f;
    }
    saveState->StartModalLoopSfxHandle(5, sfxVolume);
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-checkprobesamplemaskoverlap
 * @recoil-artifact defines .text recoil:function:0x42cbd0: Player::CheckProbeSampleMaskOverlap.
 * @recoil-match byte
 *
 * Purpose: Returns the shared mask bits of three environment probe samples.
 */
int __fastcall CheckProbeSampleMaskOverlap(int sampleIndexA, int sampleIndexB, int sampleIndexC)
{
    const int indexC = sampleIndexC;
    const int maskB = g_PlayerEnvProbeSampleMaskTable[sampleIndexB];
    const int maskA = g_PlayerEnvProbeSampleMaskTable[sampleIndexA];
    return g_PlayerEnvProbeSampleMaskTable[indexC] & maskB & maskA;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-selectbestprobesbydotproduct
 * @recoil-artifact defines .text recoil:function:0x42cc00: Player::SelectBestProbesByDotProduct.
 * @recoil-raw-asm recoil:raw-asm:battlesport.player.probe-dot-xyz
 * @recoil-raw-consumer recoil:raw-asm:battlesport.player.probe-dot-xyz
 * @recoil-match byte
 *
 * Purpose: Rank probes by rounded XYZ dot; reviewed raw assembly preserves
 * retail's complete schedule and x87 boundary. Original spelling is inferred.
 */
void __fastcall SelectBestProbesByDotProduct(const zVec3* referenceNormal, PlayerEnvProbeResult* probeResult)
{
    int sampleIndexA, sampleIndexB, sampleIndexC, sampleIndexD;
    float scoreA, scoreB, scoreC, scoreD;
    scoreA = scoreB = scoreC = scoreD = -100000000.0f;
    sampleIndexA = sampleIndexB = sampleIndexC = sampleIndexD = -1;

    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        if (g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] == 0) {
            continue;
        }
        zVec3 candidatePoint = g_PlayerEnvProbeWorldPoints[sampleIndex];
        candidatePoint.y = probeResult->candidateScoreBySample[sampleIndex];
        float score;
        const zVec3* const candidatePointForDot = &candidatePoint;
        /**
         * Purpose: Compute the grouped XYZ dot and store score as binary32.
         * Raw assembly: native forms miss retail's complete instruction/state boundary.
         * Some native controls round score. ECX/EDX clobbered; normal x87 depths 0/3/0.
         */
        __asm {
            mov ecx, referenceNormal
            mov edx, candidatePointForDot
            fld dword ptr [ecx]zVec3.x
            fmul dword ptr [edx]zVec3.x
            fld dword ptr [ecx]zVec3.y
            fmul dword ptr [edx]zVec3.y
            fld dword ptr [ecx]zVec3.z
            fmul dword ptr [edx]zVec3.z
            fxch st(1)
            faddp st(2), st(0)
            faddp st(1), st(0)
            fstp score
        }

        if (score > scoreA)
        {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            scoreD = scoreC;
            sampleIndexD = sampleIndexC;
            sampleIndexC = sampleIndexB;
            scoreC = scoreB;
            scoreB = scoreA;
            sampleIndexB = sampleIndexA;
            scoreA = score;
            sampleIndexA = sampleIndex;
        }
        else if (score > scoreB)
        {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            sampleIndexD = sampleIndexC;
            sampleIndexC = sampleIndexB;
            scoreD = scoreC;
            scoreC = scoreB;
            scoreB = score;
            sampleIndexB = sampleIndex;
        }
        else if (score > scoreC)
        {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            sampleIndexD = sampleIndexC;
            scoreD = scoreC;
            scoreC = score;
            sampleIndexC = sampleIndex;
        }
        else if (score > scoreD)
        {
            if (sampleIndexD > -1) {
                g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
            }
            scoreD = score;
            sampleIndexD = sampleIndex;
        }
        else
        {
            g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] = 0;
        }
    }
    if (CheckProbeSampleMaskOverlap(sampleIndexA, sampleIndexB, sampleIndexC)) {
        g_PlayerEnvProbe_AboveGroundFlags[sampleIndexC] = 0;
    } else {
        g_PlayerEnvProbe_AboveGroundFlags[sampleIndexD] = 0;
    }
    RebuildAboveGroundIndices();
}

} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-solveheightonsurface
 * @recoil-artifact defines .text recoil:function:0x42cde0: Player::SolveHeightOnSurface.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::SolveHeightOnSurface from the recovered
 * Battlesport gameplay source file.
 */
float __fastcall SolveHeightOnSurface(zUtil_SaveGameState* saveState, float supportPlaneDot)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zVec3 steerBasisRef = playerState->steerBasisRef;
    if (steerBasisRef.y == 0.0f) {
        steerBasisRef.y = 0.0000999999975f;
    }

    return (supportPlaneDot - playerState->worldPos.x * steerBasisRef.x - playerState->worldPos.z * steerBasisRef.z)
        / steerBasisRef.y;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-computetrianglenormal
 * @recoil-artifact defines .text recoil:function:0x42ce50: Player::ComputeTriangleNormal.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-cross
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ComputeTriangleNormal from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall
ComputeTriangleNormal(zUtil_SaveGameState* saveState, const zVec3* pointA, const zVec3* pointB, const zVec3* pointC)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    zVec3 sideAB;
    zMath::Vec3Subtract(pointB, pointA, &sideAB);
    zVec3 sideAC;
    zMath::Vec3Subtract(pointC, pointA, &sideAC);
    zVec3 normal;
    ZMTH_VECTOR_CROSS(&sideAB, &sideAC, &normal);
    zMath::Vec3Normalize(&normal);
    if (normal.y < 0.0f) {
        normal.x *= -1.0f;
        normal.y *= -1.0f;
        normal.z *= -1.0f;
    }
    playerState->steerBasisRef = normal;
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildabovegroundindices
 * @recoil-artifact defines .text recoil:function:0x42cf60: Player::RebuildAboveGroundIndices.
 * @recoil-match byte
 *
 * Purpose: reimplement Player::RebuildAboveGroundIndices from the recovered Battlesport gameplay source file.
 */
void __fastcall RebuildAboveGroundIndices()
{
    int sampleIndex = 0;
    const int sampleCount = g_PlayerEnvProbeSampleCount;
    if (sampleCount > 0) {
        int* aboveGroundIndexCursor = g_PlayerEnvProbe_AboveGroundIndices;
        do {
            if (g_PlayerEnvProbe_AboveGroundFlags[sampleIndex] != 0) {
                *aboveGroundIndexCursor = sampleIndex;
                ++aboveGroundIndexCursor;
            }
            ++sampleIndex;
        } while (sampleIndex < sampleCount);
    }
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-buildenvironmentproberesult
 * @recoil-artifact defines .text recoil:function:0x42cf90: Player::BuildEnvironmentProbeResult.
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::BuildEnvironmentProbeResult from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall BuildEnvironmentProbeResult(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* outProbe)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerModalState* const primaryModalState = saveState->primaryModalState;
    PlayerMasterModalData* const masterModalData = primaryModalState->masterModalData;

    const int modalPointCount = primaryModalState->modalStateCode;
    for (int i = 0; i < modalPointCount; ++i) {
        zVec3 transformed;
        PLAYER_TRANSFORM_POINT_BY_MATRIX(
            transformed,
            masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + i],
            playerState->motionBasis
        );
        primaryModalState->transformedProbePointWorldByIndex[i] = transformed;
        g_PlayerEnvProbeWorldPoints[i] = transformed;
    }

    if (g_PlayerEnvProbeSampleCount > 4) {
        zMath::Vec3Midpoint(
            &g_PlayerEnvProbeWorldPoints[0],
            &g_PlayerEnvProbeWorldPoints[3],
            &g_PlayerEnvProbeWorldPoints[4]
        );
        zMath::Vec3Midpoint(
            &g_PlayerEnvProbeWorldPoints[1],
            &g_PlayerEnvProbeWorldPoints[2],
            &g_PlayerEnvProbeWorldPoints[5]
        );
    }

    if (g_PlayerEnvProbeSampleCount > 6) {
        g_PlayerEnvProbeWorldPoints[6] = playerState->worldPos;
    }

    zUtil_PlayerStateStorage* const globalPlayerState
        = (zUtil_PlayerStateStorage*)((void*)(g_GameStateOrMapTable->playerState));
    CZClass::gwNodeSetCellPickable(playerState->rootNode, 0);
    CZClass::gwNodeSetCellPickable(globalPlayerState->rootNode, 0);

    g_Variant_CurrentTag = playerState->variantTag;
    CZDisplayInstance::BuildPickCandidatesForPointBatch(
        g_Player_RuntimeDiScene,
        g_PlayerEnvProbeWorldPoints,
        g_PlayerEnvProbeSampleCount,
        500.0f,
        outProbe->candidateBuffers
    );
    g_Variant_CurrentTag = g_VariantTag_Current;

    outProbe->highestSelectedHitY = -300.0f;
    outProbe->attachmentCandidateCount = 0;

    float maxRiseWindow = -(playerState->projectileSpawnVel.y * g_Player_DeltaTime);
    if (outProbe->minProbeDepth > maxRiseWindow) {
        maxRiseWindow = outProbe->minProbeDepth;
    }

    for (int sampleIndex = 0; sampleIndex < g_PlayerEnvProbeSampleCount; ++sampleIndex) {
        int bestCandidateIndex = 0;
        int selectedImpactSlot = 0;
        float taggedHeight = -300.0f;
        PlayerProbeSampleCandidateBuffer* const candidateBuffer = &outProbe->candidateBuffers[sampleIndex];

        outProbe->candidateScoreBySample[sampleIndex] = SelectProbeSampleHeightFromCandidates(
            candidateBuffer,
            &bestCandidateIndex,
            g_PlayerEnvProbeWorldPoints[sampleIndex].y,
            maxRiseWindow,
            outProbe->preferAttachmentSlot1,
            &selectedImpactSlot,
            &taggedHeight
        );
        outProbe->bestIndexBySample[sampleIndex] = bestCandidateIndex;
        outProbe->impactSlotBySample[sampleIndex] = selectedImpactSlot;

        if (outProbe->highestSelectedHitY < taggedHeight) {
            outProbe->highestSelectedHitY = taggedHeight;
        }

        if (sampleIndex < 4) {
            if (sampleIndex == 0) {
                if (outProbe->candidateBuffers[0].candidateCount > 0) {
                    const zClassDiPickCandidateEntry* const selectedCandidate
                        = &outProbe->candidateBuffers[0].entries[outProbe->bestIndexBySample[0]];
                    playerState->selectedProbeSample = *selectedCandidate;
                    playerState->selectedProbeSample.hitPos.x
                        = primaryModalState->transformedProbePointWorldByIndex[0].x;
                    playerState->selectedProbeSample.hitPos.z
                        = primaryModalState->transformedProbePointWorldByIndex[0].z;
                    playerState->variantTag = selectedCandidate->variantTag;

                    CZNodePartial* const worldChild = CZClass::gwNodeGetWorldChild(selectedCandidate->node);
                    const int nodeType = worldChild != 0 ? worldChild->nodeType : selectedCandidate->variantTag.tags[0];
                    CZClass::gwNodeSetNodeType(playerState->rootNode, nodeType);
                } else {
                    CZClass::gwNodeSetNodeType(playerState->rootNode, 0xff);
                }
            }

            // Only the four primary samples feed the surface histogram and attachment vote.
            outProbe->hitHistogram.countByImpactSlot[selectedImpactSlot] += 1;

            if (candidateBuffer->candidateCount != 0) {
                CZNodePartial* const candidateNode = candidateBuffer->entries[bestCandidateIndex].node;
                if (candidateNode != 0 && candidateNode->auxFlags != 0) {
                    outProbe->attachmentCandidateCount += 1;
                    outProbe->attachmentNode = (CZNodePartial*)(candidateNode->callbackContext);
                }
            }
        } else if ((g_PlayerEnvProbeSampleMaskTable[sampleIndex] & 0x0a) == 0) {
            outProbe->candidateScoreBySample[sampleIndex] -= 0.2f;
        }
    }

    playerState->probeImpactSlot1SeenFlag = outProbe->hitHistogram.countByImpactSlot[1];
}
} // namespace Player
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-findthirdprobeandcomputenormal
 * @recoil-artifact defines .text recoil:function:0x42d320: Player::FindThirdProbeAndComputeNormal.
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::FindThirdProbeAndComputeNormal from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall FindThirdProbeAndComputeNormal(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* probeResult)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    int thirdProbeCandidateScanCount = 4;
    if (g_PlayerEnvProbeSampleCount <= 4) {
        thirdProbeCandidateScanCount = g_PlayerEnvProbeSampleCount;
    }

    const int firstAboveGroundSampleIndex = g_PlayerEnvProbe_AboveGroundIndices[0];
    const int secondAboveGroundSampleIndex = g_PlayerEnvProbe_AboveGroundIndices[1];
    int bestThirdProbeSampleIndex = 0;
    float bestThirdProbeHeightDelta = 0.0f;
    for (int candidateProbeSampleIndex = 0; candidateProbeSampleIndex < thirdProbeCandidateScanCount;
        ++candidateProbeSampleIndex) {
        if (candidateProbeSampleIndex == firstAboveGroundSampleIndex
            || candidateProbeSampleIndex == secondAboveGroundSampleIndex) {
            continue;
        }

        zVec3 transformedCandidateProbePoint;
        PLAYER_TRANSFORM_POINT_BY_MATRIX(
            transformedCandidateProbePoint,
            masterModalData->probePoints[kPlayerEnvProbeBasePointOffset + candidateProbeSampleIndex],
            playerState->motionBasis
        );
        const float candidateHeightDelta
            = probeResult->candidateScoreBySample[candidateProbeSampleIndex] - transformedCandidateProbePoint.y;
        if (candidateHeightDelta > bestThirdProbeHeightDelta
            && CheckProbeSampleMaskOverlap(
                   firstAboveGroundSampleIndex,
                   secondAboveGroundSampleIndex,
                   candidateProbeSampleIndex
               ) == 0) {
            bestThirdProbeSampleIndex = candidateProbeSampleIndex;
            bestThirdProbeHeightDelta = candidateHeightDelta;
        }
    }

    if (bestThirdProbeHeightDelta <= g_Player_DeltaTime) {
        return;
    }

    zVec3 firstSynthSupportPoint = g_PlayerEnvProbeWorldPoints[firstAboveGroundSampleIndex];
    zVec3 secondSynthSupportPoint = g_PlayerEnvProbeWorldPoints[secondAboveGroundSampleIndex];
    zVec3 thirdSynthSupportPoint = g_PlayerEnvProbeWorldPoints[bestThirdProbeSampleIndex];
    firstSynthSupportPoint.y = probeResult->candidateScoreBySample[firstAboveGroundSampleIndex];
    secondSynthSupportPoint.y = probeResult->candidateScoreBySample[secondAboveGroundSampleIndex];
    thirdSynthSupportPoint.y = probeResult->candidateScoreBySample[bestThirdProbeSampleIndex];

    ComputeTriangleNormal(saveState, &firstSynthSupportPoint, &secondSynthSupportPoint, &thirdSynthSupportPoint);
    const float surfaceDot = playerState->steerBasisRef.x * firstSynthSupportPoint.x
        + playerState->steerBasisRef.y * firstSynthSupportPoint.y
        + playerState->steerBasisRef.z * firstSynthSupportPoint.z;
    playerState->worldPos.y = SolveHeightOnSurface(saveState, surfaceDot);
    RebuildOrientationFromNormal(saveState);
}
} // namespace Player

namespace zMath {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-zmath-vec3midpoint
 * @recoil-artifact defines .text recoil:function:0x42d560: zMath::Vec3Midpoint.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-add
 * @recoil-match byte
 *
 * Purpose: Writes the component-wise midpoint and returns the output pointer.
 * Data: Uses the shared midpoint half scalar at 0x4d08d4.
 * The reviewed addition stores all three sums before scaling.
 * The compiler owns scalar arithmetic, calling convention and return.
 */
zVec3* __fastcall Vec3Midpoint(const zVec3* a, const zVec3* b, zVec3* outMidpoint)
{
    Vec3Add(a, b, outMidpoint);

    // Unused snapshots preserve the retail x87 load order, proven by byte matching.
    float unscaledX, unscaledY, unscaledZ;
    outMidpoint->x = (unscaledX = outMidpoint->x) * g_zMath_MidpointHalf;
    outMidpoint->y = (unscaledY = outMidpoint->y) * g_zMath_MidpointHalf;
    outMidpoint->z = (unscaledZ = outMidpoint->z) * g_zMath_MidpointHalf;
    return outMidpoint;
}
} // namespace zMath

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-applyenvironmentproberesult
 * @recoil-artifact defines .text recoil:function:0x42d5c0: Player::ApplyEnvironmentProbeResult.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-rotate-rows-in-place
 * @recoil-source previously-byte-matched
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::ApplyEnvironmentProbeResult from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall ApplyEnvironmentProbeResult(zUtil_SaveGameState* saveState, PlayerEnvProbeResult* envProbe)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;
    if (envProbe->attachmentCandidateCount > 3) {
        if (playerState->environmentAttachmentActive == 0) {
            playerState->environmentAttachmentActive = 1;
            playerState->environmentAttachmentNode = envProbe->attachmentNode;
            CZObject3DDataPartial* const objectData = (CZObject3DDataPartial*)(envProbe->attachmentNode->classData);
            memcpy(
                &playerState->environmentAttachmentMatrix,
                objectData->cachedWorldMatrix,
                sizeof(playerState->environmentAttachmentMatrix)
            );
            playerState->poseCache.y = playerState->restartYawRad
                - (float)(atan2(
                    playerState->environmentAttachmentMatrix.zx,
                    playerState->environmentAttachmentMatrix.zz
                ));
            PLAYER_CACHE_ATTACHMENT_LOCAL_OFFSET(playerState);
        }
    } else if (playerState->environmentAttachmentActive != 0) {
        CZObject3DDataPartial* const objectData
            = (CZObject3DDataPartial*)(playerState->environmentAttachmentNode->classData);
        memcpy(
            &playerState->environmentAttachmentMatrix,
            objectData->cachedWorldMatrix,
            sizeof(playerState->environmentAttachmentMatrix)
        );
        playerState->restartYawRad
            = (float)(atan2(playerState->environmentAttachmentMatrix.zx, playerState->environmentAttachmentMatrix.zz))
            + playerState->poseCache.y;
        playerState->poseCache = playerState->vehicleRotationAngles;
        playerState->environmentAttachmentActive = 0;
        playerState->environmentAttachmentNode = 0;
    }

    if (playerState->amphibUnlocked != 0 && envProbe->hitHistogram.countByImpactSlot[1] > 1
        && TransitionToMasterTypeAmphib(saveState, 0, 0) != 0) {
        playerState->currentMasterType = masterModalData->masterType;
        if (playerState->projectileSpawnVel.y < -10.0f) {
            zEffectAnim::SetTransformRotAndVelocityThunk(
                g_Player_BftSplashAnimEntry,
                0,
                playerState->worldPos.x,
                envProbe->highestSelectedHitY,
                playerState->worldPos.z,
                0.0f,
                playerState->restartYawRad,
                0.0f,
                0.0f,
                0.0f,
                0.0f
            );
            playerState->projectileSpawnVel.z = 0.0f;
            playerState->projectileSpawnVel.x = 0.0f;
        }
        return 0;
    }

    playerState->gravityAccel = g_Player_NominalGravity;
    if (envProbe->highestSelectedHitY - playerState->worldPos.y > 1.0f
        && envProbe->hitHistogram.countByImpactSlot[1] > 1) {
        const int wasUnderwater = playerState->aiMode;
        playerState->gravityAccel = g_Player_WaterGravity;
        if (wasUnderwater == 0) {
            playerState->aiMode = 1;
            if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
                HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x909), 5.0f);
                HudLowMeterLoopSound::SetLoopActive(1);
            }
        }

        const float damage = g_Player_DeltaTime * 8.0f;
        if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
            EnterDestroyedState(saveState, 0, 0, damage);
            if (playerState->cameraTarget.y < envProbe->highestSelectedHitY) {
                ((HudUiElement*)(&g_Player_UnderwaterFxPass3Ui))->SetVisible(1);
            } else {
                ((HudUiElement*)(&g_Player_UnderwaterFxPass3Ui))->SetVisible(0);
            }
        } else {
            HitCallbackRecordContextAndTimedStatus(saveState, 0, 0, damage);
        }
    } else {
        if (playerState->aiMode != 0) {
            playerState->aiMode = 0;
            if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
                ((HudUiElement*)(&g_Player_UnderwaterFxPass3Ui))->SetVisible(0);
                HudLowMeterLoopSound::SetLoopActive(0);
            }
        }
    }

    if (envProbe->hitHistogram.countByImpactSlot[3] > 1) {
        playerState->axisClampRuntime = masterModalData->maxSpeed * masterModalData->quicksandSlowdown;
        playerState->yawVelocityLimit = masterModalData->yawRateMax * masterModalData->quicksandSlowdown;
        playerState->gravityAccel = g_Player_QuicksandGravity;
        return 1;
    }

    if (envProbe->hitHistogram.countByImpactSlot[4] > 1) {
        if (playerState->hoverUnlocked != 0 && TransitionToMasterTypeHover(saveState, 0) != 0) {
            return 0;
        }

        if (playerState->motionInput == 0) {
            playerState->motionInput = 1;
            if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
                HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x910), 5.0f);
                HudLowMeterLoopSound::SetLoopActive(1);
            }
        }

        playerState->axisClampRuntime = masterModalData->maxSpeed * masterModalData->lavaSlowdown;
        playerState->yawVelocityLimit = masterModalData->yawRateMax * masterModalData->lavaSlowdown;
        const float damage = (float)(envProbe->hitHistogram.countByImpactSlot[4]) * g_Player_DeltaTime * 12.0f;
        if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
            EnterDestroyedState(saveState, g_Player_MakeHotOptEntry, 0, damage);
        } else {
            HitCallbackRecordContextAndTimedStatus(saveState, g_Player_MakeHotOptEntry, 0, damage);
        }
        return 1;
    }

    if (playerState->motionInput != 0) {
        playerState->motionInput = 0;
        if (saveState == (zUtil_SaveGameState*)g_GameStateOrMapTable) {
            HudLowMeterLoopSound::SetLoopActive(0);
        }
    }

    playerState->axisClampRuntime = masterModalData->maxSpeed;
    playerState->yawVelocityLimit = masterModalData->yawRateMax;
    return 1;
}
} // namespace Player

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-rebuildorientationfromnormal
 * @recoil-artifact defines .text recoil:function:0x42da40: Player::RebuildOrientationFromNormal.
 *
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::RebuildOrientationFromNormal from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RebuildOrientationFromNormal(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    if (playerState->steerBasisRef.y == 0.0) {
        playerState->steerBasisRef.y = 0.00100000005f;
    }

    zVec3 rawBasis = playerState->steerBasisNorm;
    rawBasis.y
        = -((rawBasis.x * playerState->steerBasisRef.x + playerState->steerBasisRef.z * rawBasis.z)
            / playerState->steerBasisRef.y);
    zMath::Vec3Normalize(&rawBasis);
    playerState->steerBasisRaw = rawBasis;

    zVec3 yawRelativeNormal;
    zMath::Vec3RotateY(&yawRelativeNormal, &playerState->steerBasisRef, -playerState->restartYawRad);
    playerState->vehiclePitchRad = (float)(asin(yawRelativeNormal.z));
    playerState->vehicleRollRad = (float)(asin(-yawRelativeNormal.x));
    zMath::MatBuildEulerRotation3x3(
        &playerState->motionBasis,
        playerState->vehiclePitchRad,
        playerState->restartYawRad,
        playerState->vehicleRollRad
    );
    playerState->motionBasis.posX = playerState->worldPos.x;
    playerState->motionBasis.posY = playerState->worldPos.y;
    playerState->motionBasis.posZ = playerState->worldPos.z;
}
} // namespace Player
