#include "Battlesport/ainet.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zReader/zReader.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const int kPlayerAiMode2TopSteering = 1;
const int kPlayerAiMode2SteerDirectTarget = 0;
const int kPlayerAiMode2SteerOffsetTarget = 1;
const int kPlayerAiMode2SteerDynamicOffsetTarget = 2;
const int kPlayerAiMode2SteerPathFollow = 3;
const int kPlayerAiMode2SteerTurnInPlace = 5;
const int kPlayerAiMode2SteerAutoTurn = 6;
const float kPlayerAiAltGunAttackForwardMin = 0.75f;
const float kPlayerAiAltGunStatusMinScale = 0.5f;
const int kPlayerAiTopPathFollow = 0;
const int kPlayerAiTopTurnTowardTarget = 2;
const int kPlayerAiTopTurnOnlyTowardTarget = 3;
const int kPlayerAiTopPathSteering = 4;
const int kPlayerAiTopAutoTurn = 5;
const int kPlayerMasterTypeSub = 2;
const int kPlayerLifecycleInactive = 4;
const float kPlayerAiPathFollowMinThrottle = 0.25f;
const float kPlayerAiPathFollowAdvanceDistance = 10.0f;
const float kPlayerAiForwardPathAdvanceDistance = 5.0f;
const float kPlayerAiSyntheticPathRebuildDistanceSq = 400.0f;
const float kPlayerAiSyntheticPathWidth = 10.0f;
const float kPlayerAiSyntheticPathRebuildDelaySec = 1.0f;
const float kPlayerAiAttackLosTargetYOffset = 1.5f;
const float kPlayerAiDynamicOffsetBackUpDistance = 10.0f;
const unsigned int kOptCatalogFlagLockOnTargetRef = 0x4000;
const unsigned int kOptCatalogFlagCreateTrail = 0x02;

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in address-backed caller 0x4024a0 as the same fast square-root
 * estimate pattern used by later Player source-file helpers.
 * Purpose: provide the recovered ai_net.cpp-local fast sqrt estimate helper.
 */
static float PlayerFastSqrtEstimate(
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

extern "C" {
/**
 * Reimplements data 0x4cc850: g_AINetPathProbeHalfWidthScale.
 * Purpose: Stores the read-only half-width scale used for minimum probe-fan travel.
 */
extern const float g_AINetPathProbeHalfWidthScale = 0.5f;
/**
 * Reimplements data 0x4e5c58: g_AINetListHead.
 * Purpose: Stores the zero-initialized head pointer for the loaded AINet global list.
 */
AINet *g_AINetListHead = 0;
/**
 * Reimplements data 0x4e5c5c: g_AINetListTail.
 * Purpose: Stores the zero-initialized tail pointer maintained with the loaded AINet global list.
 */
AINet *g_AINetListTail = 0;
/**
 * Reimplements data 0x4da0c0: g_Player_AiMode2_PathFollowPitchInputScale.
 * BN types this as an initialized .data float used by the Mode2 AI
 * path-follow pitch steering input.
 * Purpose: Scales path-follow vertical steering error into pitch input.
 */
float g_Player_AiMode2_PathFollowPitchInputScale = 0.0174499992f;
/**
 * Reimplements data 0x4da0c4: g_Player_AiMode2_PathFollowPitchTurnGain.
 * BN types this as an initialized .data float paired with the Mode2 AI
 * path-follow pitch input scale.
 * Purpose: Scales path-follow pitch input into turn correction.
 */
float g_Player_AiMode2_PathFollowPitchTurnGain = 5.69999981f;
/**
 * Reimplements data 0x4da0c8: g_Player_AiMode2_SteeringPitchInputScale.
 * BN types this as an initialized .data float used by the Mode2 AI steering
 * substates.
 * Purpose: Scales steering vertical distance into pitch input.
 */
float g_Player_AiMode2_SteeringPitchInputScale = 0.800000012f;
/**
 * Reimplements data 0x4da0cc: g_Player_AiMode2_SteeringPitchTurnGain.
 * BN types this as an initialized .data float paired with the Mode2 AI
 * steering pitch input scale.
 * Purpose: Scales steering pitch input into turn correction.
 */
float g_Player_AiMode2_SteeringPitchTurnGain = 5.69999981f;
/**
 * Reimplements data 0x4da0d0: g_Player_AiMode2_SteeringVerticalErrorScale.
 * BN types this as an initialized .data float read by the Mode2 AI steering
 * substates.
 * Purpose: Scales steering vertical error before pitch correction.
 */
float g_Player_AiMode2_SteeringVerticalErrorScale = 0.100000001f;
/**
 * Reimplements data 0x4da0d4: g_Player_AiMode2_TuningScalar55A.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the first Mode2 AI 55.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar55A = 55.0f;
/**
 * Reimplements data 0x4da0d8: g_Player_AiMode2_TuningScalar55B.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the second Mode2 AI 55.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar55B = 55.0f;
/**
 * Reimplements data 0x4da0dc: g_Player_AiMode2_TuningScalar5.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the Mode2 AI 5.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar5 = 5.0f;
/**
 * Reimplements data 0x4da0e0: g_Player_AiMode2_TuningScalar10.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the Mode2 AI 10.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar10 = 10.0f;
/**
 * Reimplements data 0x4da0e4: g_Player_AiMode2_OffsetTargetRotateCos15Deg.
 * BN types this as an initialized .data float used by the Mode2 AI offset
 * target steering rotation.
 * Purpose: Stores the retail cosine scalar for offset-target rotation.
 */
float g_Player_AiMode2_OffsetTargetRotateCos15Deg = 0.965900004f;
/**
 * Reimplements data 0x4da0e8: g_Player_AiMode2_OffsetTargetRotateSin15Deg.
 * BN types this as an initialized .data float used by the Mode2 AI offset
 * target steering rotation.
 * Purpose: Stores the retail sine scalar for offset-target rotation.
 */
float g_Player_AiMode2_OffsetTargetRotateSin15Deg = 0.25879999995f;
/**
 * Reimplements data 0x4da0ec: g_AINet_NodeNameFormat.
 * Purpose: Stores the writable node field-name format used while parsing AI path nodes.
 */
char g_AINet_NodeNameFormat[] = "node_%02d";
/**
 * Reimplements data 0x4da0f8: g_AINet_AttackStrategyTokenSit.
 * Purpose: Stores the writable attack strategy token for stationary behavior.
 */
char g_AINet_AttackStrategyTokenSit[] = "SIT";
/**
 * Reimplements data 0x4da0fc: g_AINet_AttackStrategyTokenZig.
 * Purpose: Stores the writable attack strategy token for zig-zag behavior.
 */
char g_AINet_AttackStrategyTokenZig[] = "ZIG";
/**
 * Reimplements data 0x4da100: g_AINet_AttackStrategyTokenBack.
 * Purpose: Stores the writable attack strategy token for backing-away behavior.
 */
char g_AINet_AttackStrategyTokenBack[] = "BAC";
/**
 * Reimplements data 0x4da104: g_AINet_AttackStrategyTokenHeadOn.
 * Purpose: Stores the writable attack strategy token for head-on behavior.
 */
char g_AINet_AttackStrategyTokenHeadOn[] = "HEA";
/**
 * Reimplements data 0x4da108: g_AINet_AttackStrategyTokenCircle.
 * Purpose: Stores the writable attack strategy token for circling behavior.
 */
char g_AINet_AttackStrategyTokenCircle[] = "CIR";
/**
 * Reimplements data 0x4da10c: g_AINet_AttackStrategyTokenFollow.
 * Purpose: Stores the writable attack strategy token for follow behavior.
 */
char g_AINet_AttackStrategyTokenFollow[] = "FOL";
/**
 * Reimplements data 0x4da110: g_AINet_AttackStrategyFieldName.
 * Purpose: Stores the writable attack strategy field name used by the ZRD parser.
 */
char g_AINet_AttackStrategyFieldName[] = "attack_strategy";
/**
 * Reimplements data 0x4da120: g_AINet_ActivateBuddyFieldName.
 * Purpose: Stores the writable activate-buddy field name used by the ZRD parser.
 */
char g_AINet_ActivateBuddyFieldName[] = "activate_buddy";
/**
 * Reimplements data 0x4da130: g_AINet_AttackBuddyFieldName.
 * Purpose: Stores the writable attack-buddy field name used by the ZRD parser.
 */
char g_AINet_AttackBuddyFieldName[] = "attack_buddy";
/**
 * Reimplements data 0x4da140: g_AINet_HideTimesFieldName.
 * Purpose: Stores the writable hide-times field name used by the ZRD parser.
 */
char g_AINet_HideTimesFieldName[] = "hide_times";
/**
 * Reimplements data 0x4da14c: g_AINet_ReturnRangeFieldName.
 * Purpose: Stores the writable return-range field name used by the ZRD parser.
 */
char g_AINet_ReturnRangeFieldName[] = "return_range";
/**
 * Reimplements data 0x4da15c: g_AINet_NotPursuitDwellFieldName.
 * Purpose: Stores the writable non-pursuit dwell field name used by the ZRD parser.
 */
char g_AINet_NotPursuitDwellFieldName[] = "not_pursuit_dwell";
/**
 * Reimplements data 0x4da170: g_AINet_PursuitRangeFieldName.
 * Purpose: Stores the writable pursuit range field name accepted by older ZRD data.
 */
char g_AINet_PursuitRangeFieldName[] = "pursuit_range";
/**
 * Reimplements data 0x4da180: g_AINet_PursuitParamsFieldName.
 * Purpose: Stores the writable pursuit parameters field name used by the ZRD parser.
 */
char g_AINet_PursuitParamsFieldName[] = "pursuit_params";
/**
 * Reimplements data 0x4da190: g_AINet_AttackDwellFieldName.
 * Purpose: Stores the writable attack dwell field name used by the ZRD parser.
 */
char g_AINet_AttackDwellFieldName[] = "attack_dwell";
/**
 * Reimplements data 0x4da1a0: g_AINet_AttackRadiusFieldName.
 * Purpose: Stores the writable attack radius field name used by the ZRD parser.
 */
char g_AINet_AttackRadiusFieldName[] = "attack_rad";
/**
 * Reimplements data 0x4da1ac: g_AINet_ActivateRadiusFieldName.
 * Purpose: Stores the writable activation radius field name used by the ZRD parser.
 */
char g_AINet_ActivateRadiusFieldName[] = "activate_rad";
/**
 * Reimplements data 0x4da1bc: g_AINet_PathWidthFieldName.
 * Purpose: Stores the writable path width field name used by the ZRD parser.
 */
char g_AINet_PathWidthFieldName[] = "path_width";
/**
 * Reimplements data 0x4da1c8: g_AINet_TypeTokenDe.
 * Purpose: Stores the writable AI type token for defensive networks.
 */
char g_AINet_TypeTokenDe[] = "DE";
/**
 * Reimplements data 0x4da1cc: g_AINet_TypeTokenFi.
 * Purpose: Stores the writable AI type token for fighter networks.
 */
char g_AINet_TypeTokenFi[] = "FI";
/**
 * Reimplements data 0x4da1d0: g_AINet_TypeTokenHi.
 * Purpose: Stores the writable AI type token for hidden networks.
 */
char g_AINet_TypeTokenHi[] = "HI";
/**
 * Reimplements data 0x4da1d4: g_AINet_TypeTokenSt.
 * Purpose: Stores the writable AI type token for standard networks.
 */
char g_AINet_TypeTokenSt[] = "ST";
/**
 * Reimplements data 0x4da1d8: g_AINet_TypeFieldName.
 * Purpose: Stores the writable type field name used by the ZRD parser.
 */
char g_AINet_TypeFieldName[] = "type";
/**
 * Reimplements data 0x4da1e0: g_AINet_NameFieldName.
 * Purpose: Stores the writable name field used by the ZRD parser.
 */
char g_AINet_NameFieldName[] = "name";
/**
 * Reimplements data 0x4da1e8: g_AINet_SourceFilePath.
 * Purpose: Stores the writable original source path used for AI ZRD version errors.
 */
char g_AINet_SourceFilePath[] = "D:\\Proj\\Battlesport\\ai_net.cpp";
/**
 * Reimplements data 0x4da208: g_AINet_WrongVersionMessage.
 * Purpose: Stores the writable diagnostic emitted when an AI paths ZRD has the wrong version.
 */
char g_AINet_WrongVersionMessage[] = "Wrong ai_paths.zrd version number!";
/**
 * Reimplements data 0x4da22c: g_AINet_VersionFieldName.
 * Purpose: Stores the writable version field name used by the ZRD parser.
 */
char g_AINet_VersionFieldName[] = "version";
/**
 * Reimplements data 0x4da234: g_AINet_ZrdNameFormat.
 * Purpose: Stores the writable ZRD path format used for AI network files.
 */
char g_AINet_ZrdNameFormat[] = "%s.zrd";
/**
 * Reimplements data 0x4da23c: g_AINet_NetNameFormat.
 * Purpose: Stores the writable numbered AI network base-name format.
 */
char g_AINet_NetNameFormat[] = "net_%02d";
}

/** Reimplements 0x401060: AINet::TickAiMode2TopLevel (Battlesport/ai_net.cpp).
 * Purpose: Dispatches the active mode-2 top-level state and attack-pursuit transitions. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::TickAiMode2TopLevel(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const localPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
    playerState->storedTargetPos = localPlayerState->fxOffsetWorld;

    switch (playerState->aiTopLevelState) {
    case kPlayerAiTopPathFollow:
        TickAiMode2PathFollow(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState) != 0) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;

    case kPlayerAiMode2TopSteering:
        TickAiMode2SteeringSubstate(saveState);
        return;

    case kPlayerAiTopTurnTowardTarget:
        UpdateAiMode2TurnTowardPlayerNoThrottle(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState) != 0) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;

    case kPlayerAiTopTurnOnlyTowardTarget:
        UpdateAiMode2TurnInPlaceTowardPlayer(saveState);
        AiTryEnterMode2AttackPursuitIfLineOfSight(saveState);
        return;

    case kPlayerAiTopPathSteering:
        TickAiMode2TimedPathSteering(saveState);
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState) != 0) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;

    case kPlayerAiTopAutoTurn: {
        const int autoTurnActive = playerState->autoTurnActive;
        playerState->autoTurnSign = 0;
        if (autoTurnActive == 0) {
            playerState->aiTopLevelState = playerState->aiReturnTopLevelState;
        }
        if (AiTryEnterMode2AttackPursuitIfLineOfSight(saveState) != 0) {
            AiRebuildSyntheticPathToNodeIfFar(
                saveState,
                playerState->aiCurrentPathNode
                    ->neighborNodes[playerState->aiCurrentPathNeighborIndex]
            );
        }
        return;
    }

    default:
        return;
    }
}

/** Reimplements 0x401180: AINet::TickAiMode2PathFollow (Battlesport/ai_net.cpp).
 * Purpose: Steers toward the current AI path edge, advances the cursor, or arms auto-turn. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::TickAiMode2PathFollow(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    AINetNode *currentNode = playerState->aiCurrentPathNode;
    AINetPathProbeFan *edgeProbeFan =
        currentNode->probeFans[playerState->aiCurrentPathNeighborIndex];
    AINetNode *targetPathNode = currentNode->neighborNodes[playerState->aiCurrentPathNeighborIndex];

    zVec3 targetDelta = {0};
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) != 0) {
        AiAdvancePathCursorAndComputeTargetVec(
            saveState,
            &currentNode,
            &edgeProbeFan,
            &targetDelta
        );
        targetPathNode = currentNode->neighborNodes[playerState->aiCurrentPathNeighborIndex];
        playerState->aiReturnTopLevelState = playerState->aiTopLevelState;
        playerState->aiTopLevelState = kPlayerAiTopAutoTurn;
        playerState->autoTurnActive = 1;

        zVec3 autoTurnTargetDelta = {
            targetPathNode->position.x - playerState->worldPos.x,
            targetPathNode->position.y - playerState->worldPos.y,
            targetPathNode->position.z - playerState->worldPos.z,
        };
        autoTurnTargetDelta.y = 0.0f;
        zMath::Vec3NormalizeXZ(
            &autoTurnTargetDelta,
            &playerState->autoTurnTargetDir
        );
        playerState->throttleInput = 0.0f;
        playerState->throttleInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        return;
    }

    targetDelta.x = targetPathNode->position.x - playerState->worldPos.x;
    targetDelta.y = targetPathNode->position.y - playerState->worldPos.y;
    targetDelta.z = targetPathNode->position.z - playerState->worldPos.z;
    targetDelta.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDelta);

    zVec3 steerBasis = playerState->steerBasisNorm;
    const float steerDotXZ = steerBasis.x * targetDelta.x + steerBasis.z * targetDelta.z;
    const float steerCrossXZ = steerBasis.z * targetDelta.x - steerBasis.x * targetDelta.z;

    if (steerDotXZ < 0.0f) {
        if (playerState->aiPathCursorAdvanceRequested != 0) {
            AiAdvancePathCursorAndComputeTargetVec(
                saveState,
                &currentNode,
                &edgeProbeFan,
                &targetDelta
            );
            playerState->aiPathCursorAdvanceRequested = 0;
            TickAiMode2PathFollow(saveState);
            return;
        }

        playerState->throttleInput = 0.0f;
        playerState->steeringInput = steerCrossXZ < 0.0f ? -1.0f : 1.0f;
    } else {
        float throttle = 1.0f - (float)(fabs(steerCrossXZ));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->aiPathCursorAdvanceRequested = 1;
        playerState->throttleInput = throttle;
        playerState->steeringInput = steerCrossXZ;
    }

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;

    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        const float pitchInput = ((targetPathNode->position.y - playerState->worldPos.y +
                                      masterModalData->modeAltTransitionTime) *
                                         g_Player_AiMode2_PathFollowPitchInputScale -
                                     playerState->vehiclePitchRad) *
                                 g_Player_AiMode2_PathFollowPitchTurnGain;
        playerState->subPitchInput = pitchInput;
        playerState->subPitchInputCopy = pitchInput;
    }

    if (targetDistance < kPlayerAiPathFollowAdvanceDistance) {
        AiAdvancePathCursorAndComputeTargetVec(
            saveState,
            &currentNode,
            &edgeProbeFan,
            &targetDelta
        );
        playerState->aiPathCursorAdvanceRequested = 0;
    }
}

/** Reimplements 0x401420: AINet::AiMode2ForwardProbeRequiresAutoTurn (Battlesport/ai_net.cpp).
 * Purpose: Checks forward probe queues and requests auto-turn recovery when blocked. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
int __fastcall AINet::AiMode2ForwardProbeRequiresAutoTurn(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (playerState->playerCollisionResolved != 0 || playerState->preferredCollisionResolved != 0) {
        ++playerState->aiMode2SteeringRetryCount;
        return 1;
    }

    const PlayerMasterModalData *const masterModalData =
        saveState->primaryModalState->masterModalData;

    zClass_DiSegmentEndpoints segmentPairs[1];
    segmentPairs[0].start = playerState->worldPos;
    segmentPairs[0].start.y += masterModalData->probePoints[1].y;

    zVec3 forwardDir = playerState->projectileSpawnVel;
    float forwardProbeLength = zMath::Vec3Normalize(&forwardDir);
    if (forwardProbeLength < 1.0f) {
        forwardProbeLength = 1.0f;
    } else {
        forwardProbeLength = zMath::Vec3Normalize(&forwardDir);
    }

    const float forwardProbeOffset = forwardProbeLength * 0.5f - masterModalData->probePoints[1].z;
    segmentPairs[0].end.x = playerState->worldPos.x + forwardProbeOffset * forwardDir.x;
    segmentPairs[0].end.y = playerState->worldPos.y + forwardProbeOffset * forwardDir.y;
    segmentPairs[0].end.z = playerState->worldPos.z + forwardProbeOffset * forwardDir.z;

    int segmentTags[2] = {-1, -1};
    Player::CollectPendingContactsForSegments(
        saveState,
        segmentPairs,
        2,
        segmentTags
    );

    const int hasBlockingContacts = playerState->preferredCollisionQueue.count != 0 ||
                                    playerState->playerCollisionQueue.count != 0;
    Player::ClearPendingContactQueues(saveState);
    return hasBlockingContacts != 0 ? 1 : 0;
}

/** Reimplements 0x401580: AINet::AiAdvancePathCursorAndComputeTargetVec (Battlesport/ai_net.cpp).
 * Purpose: Advances the AI path cursor and returns the target vector and probe fan. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::AiAdvancePathCursorAndComputeTargetVec(
    zUtil_SaveGameState *saveState,
    AINetNode **currentNodeInOut,
    AINetPathProbeFan **outProbeFan,
    zVec3 *outTargetVec
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const previousNode = *currentNodeInOut;
    AINetNode *const nextNode =
        previousNode->neighborNodes[playerState->aiCurrentPathNeighborIndex];
    playerState->aiCurrentPathNode = nextNode;

    const int previousNodeIndex = previousNode->nodeIndex;
    if (previousNodeIndex < 0) {
        previousNode->Free();
        *currentNodeInOut = playerState->aiCurrentPathNode;

        if ((*currentNodeInOut)->nodeIndex < 0) {
            playerState->aiCurrentPathNeighborIndex = 0;
        } else {
            int nextBranchIndex = 0;
            AiChooseNextPathBranchIndex(
                saveState,
                currentNodeInOut,
                &nextBranchIndex,
                -1
            );
            playerState->aiCurrentPathNeighborIndex = nextBranchIndex;
            if (playerState->aiNet->aiType == AINET_TYPE_HI) {
                playerState->aiTopLevelState = kPlayerAiTopTurnTowardTarget;
            }
        }
    } else {
        *currentNodeInOut = nextNode;

        int excludedBranchIndex = 4;
        for (int branchIndex = 0; branchIndex < 3; ++branchIndex) {
            AINetNode *const reverseNode = nextNode->neighborNodes[branchIndex];
            if (reverseNode != 0 && reverseNode->nodeIndex == previousNodeIndex) {
                excludedBranchIndex = branchIndex;
                break;
            }
        }

        int nextBranchIndex = 0;
        AiChooseNextPathBranchIndex(
            saveState,
            currentNodeInOut,
            &nextBranchIndex,
            excludedBranchIndex
        );
        playerState->aiCurrentPathNeighborIndex = nextBranchIndex;
    }

    AINetNode *const selectedNode = *currentNodeInOut;
    *outProbeFan = selectedNode->probeFans[playerState->aiCurrentPathNeighborIndex];
    outTargetVec->x = playerState->worldPos.x - selectedNode->position.x;
    outTargetVec->y = playerState->worldPos.y - selectedNode->position.y;
    outTargetVec->z = playerState->worldPos.z - selectedNode->position.z;
}

/** Reimplements 0x4016a0: AINet::AiChooseNextPathBranchIndex (Battlesport/ai_net.cpp).
 * Purpose: Selects the next non-excluded AI path branch for mode-2 steering. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
int __fastcall AINet::AiChooseNextPathBranchIndex(
    zUtil_SaveGameState *saveState,
    AINetNode **currentNodeInOut,
    int *outBranchIndex,
    int excludedBranchIndex
) {
    (void)saveState;

    AINetNode *currentNode = *currentNodeInOut;
    int branchCount = 0;
    AINetNode **neighborSlot = currentNode->neighborNodes;
    for (int branchIndex = 0; branchIndex < 3; ++branchIndex) {
        if (neighborSlot[branchIndex] != 0) {
            ++branchCount;
        }
    }

    if (branchCount == 1) {
        *outBranchIndex = 0;
        return 1;
    }

    if (branchCount == 2) {
        *outBranchIndex = 0;
    } else {
        *outBranchIndex = rand() % branchCount;
    }

    if (*outBranchIndex == excludedBranchIndex) {
        *outBranchIndex = (*outBranchIndex + 1) % branchCount;
    }

    return 1;
}

/** Reimplements 0x401710: AINet::TickAiMode2SteeringSubstate (Battlesport/ai_net.cpp).
 * Purpose: Runs pursuit steering, submarine vertical controls, and pursuit exit checks. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::TickAiMode2SteeringSubstate(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    zUtil_SaveGameState *const targetSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;
    const zVec3 targetWorldSnapshot = targetPlayerState->worldPos;

    if (g_Player_TotalTimeSecScaled >= playerState->aiNextPathRebuildTime &&
        playerState->aiCurrentSteeringSubstate != kPlayerAiMode2SteerPathFollow) {
        AiRebuildSyntheticPathToNodeIfFar(
            saveState,
            playerState->aiCurrentPathNode
        );
    }

    zVec3 targetDelta = {
        targetWorldSnapshot.x - playerState->worldPos.x,
        targetWorldSnapshot.y - playerState->worldPos.y,
        targetWorldSnapshot.z - playerState->worldPos.z,
    };
    const float targetVerticalDelta = targetDelta.y;
    targetDelta.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDelta);
    const float verticalDistanceScale =
        targetDistance != 0.0f ? targetVerticalDelta / targetDistance : 0.0f;

    const float lateralDot = playerState->steerBasisNorm.z * targetDelta.x -
                             playerState->steerBasisNorm.x * targetDelta.z;
    float forwardDot = playerState->steerBasisNorm.x * targetDelta.x +
                       playerState->steerBasisNorm.z * targetDelta.z;

    if (playerState->aiMode2SteeringRetryCount > 6) {
        playerState->aiCurrentSteeringSubstate = kPlayerAiMode2SteerTurnInPlace;
    }

    switch (playerState->aiCurrentSteeringSubstate) {
    case kPlayerAiMode2SteerDirectTarget:
        UpdateAiMode2MoveAndTurnTowardTarget(
            saveState,
            forwardDot,
            lateralDot,
            targetDistance
        );
        break;
    case kPlayerAiMode2SteerOffsetTarget:
        TickAiMode2OffsetTargetSteering(
            saveState,
            forwardDot,
            lateralDot,
            targetDistance
        );
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerDynamicOffsetTarget:
        TickAiMode2DynamicOffsetTargetSteering(
            saveState,
            forwardDot,
            lateralDot,
            targetDistance
        );
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerPathFollow:
        TickAiMode2PathFollow(saveState);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerTurnInPlace:
        UpdateAiMode2TurnInPlaceTowardPlayer(saveState);
        forwardDot = 1.0f;
        break;
    case kPlayerAiMode2SteerAutoTurn:
        if (playerState->autoTurnActive == 0) {
            playerState->aiCurrentSteeringSubstate = playerState->aiReturnSteeringSubstate;
        }
        forwardDot = 1.0f;
        break;
    default:
        break;
    }

    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        const float pitchInput = (g_Player_AiMode2_SteeringPitchInputScale * verticalDistanceScale -
                                     playerState->vehiclePitchRad) *
                                 g_Player_AiMode2_SteeringPitchTurnGain;
        playerState->subPitchInput = pitchInput;
        playerState->subPitchInputCopy = pitchInput;

        const float verticalInput = (targetWorldSnapshot.y - playerState->worldPos.y) *
                                    g_Player_AiMode2_SteeringVerticalErrorScale;
        playerState->subVerticalInput = verticalInput;
        playerState->subVerticalInputCopy = verticalInput;
    }

    TickAiMode2AltGunAttackWindow(
        saveState,
        targetDistance,
        forwardDot
    );

    if (targetPlayerState->lifecycleState == kPlayerLifecycleInactive ||
        zMath::Vec3DeltaLengthSq(
            &playerState->worldPos,
            &playerState->aiRestoreTarget
        ) >
            playerState->aiRestoreDistanceSq) {
        AiRestoreSavedTopLevelState(saveState);
        playerState->aiStateUntilTime =
            g_Player_TotalTimeSecScaled + playerState->aiNotPursuitDwell;
    }
}

/** Reimplements 0x401970: AINet::UpdateAiMode2MoveAndTurnTowardTarget (Battlesport/ai_net.cpp).
 * Purpose: Converts target alignment and pursuit distance into throttle and steering input. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardTarget(
    zUtil_SaveGameState *saveState,
    float forwardDot,
    float lateralDot,
    float targetDistance
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (forwardDot <= 0.0f) {
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = lateralDot < 0.0f ? -1.0f : 1.0f;
    } else {
        AINet *const aiNet = playerState->aiNet;
        playerState->steeringInput = lateralDot;
        if (targetDistance > aiNet->pursuitParam1) {
            playerState->throttleInput = 1.0f;
        } else if (targetDistance < aiNet->pursuitParam0) {
            playerState->throttleInput = -1.0f;
        } else {
            playerState->throttleInput = 0.0f;
        }
    }

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

/** Reimplements 0x401a40: AINet::TickAiMode2OffsetTargetSteering (Battlesport/ai_net.cpp).
 * Purpose: Runs offset-target pursuit or switches to auto-turn recovery when blocked. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::TickAiMode2OffsetTargetSteering(
    zUtil_SaveGameState *saveState,
    float unusedForwardDot,
    float unusedLateralDot,
    float unusedTargetDistance
) {
    (void)unusedForwardDot;
    (void)unusedLateralDot;
    (void)unusedTargetDistance;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) == 0) {
        UpdateAiMode2MoveAndTurnTowardOffsetTarget(
            saveState,
            (zUtil_SaveGameState *)g_GameStateOrMapTable
        );
        return;
    }

    zUtil_SaveGameState *const targetSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    Player::SetAutoTurnTargetDirFromWorldPoint(
        saveState,
        &targetSaveState->playerState->worldPos
    );

    int *const currentSteeringSubstatePtr = &playerState->aiCurrentSteeringSubstate;
    int currentSteeringSubstate = *currentSteeringSubstatePtr;
    playerState->steeringInputCopy = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->aiReturnSteeringSubstate = currentSteeringSubstate;
    *currentSteeringSubstatePtr = kPlayerAiMode2SteerAutoTurn;
}

/** Reimplements 0x401ab0: AINet::TickAiMode2DynamicOffsetTargetSteering (Battlesport/ai_net.cpp).
 * Purpose: Runs dynamic-offset pursuit or switches to auto-turn recovery when blocked. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::TickAiMode2DynamicOffsetTargetSteering(
    zUtil_SaveGameState *saveState,
    float unusedForwardDot,
    float unusedLateralDot,
    float targetDistance
) {
    (void)unusedForwardDot;
    (void)unusedLateralDot;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    if (AiMode2ForwardProbeRequiresAutoTurn(saveState) == 0) {
        UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
            saveState,
            (zUtil_SaveGameState *)g_GameStateOrMapTable,
            targetDistance
        );
        return;
    }

    Player::SetAutoTurnTargetDirFromWorldPoint(
        saveState,
        &((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->worldPos
    );

    int *const currentSteeringSubstatePtr = &playerState->aiCurrentSteeringSubstate;
    int currentSteeringSubstate = *currentSteeringSubstatePtr;
    playerState->steeringInputCopy = 0.0f;
    playerState->steeringInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->throttleInput = 0.0f;
    playerState->aiReturnSteeringSubstate = currentSteeringSubstate;
    *currentSteeringSubstatePtr = kPlayerAiMode2SteerAutoTurn;
}

/** Reimplements 0x401b20: AINet::AiTryEnterMode2AttackPursuitIfLineOfSight (Battlesport/ai_net.cpp).
 * Purpose: Tests attack range and local-player line of sight before steering pursuit. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
int __fastcall AINet::AiTryEnterMode2AttackPursuitIfLineOfSight(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const aiState = saveState->playerState;
    if (g_Player_AiMode2State1Finalized != 0) {
        return 0;
    }

    if (g_Player_TotalTimeSecScaled <= aiState->aiStateUntilTime) {
        return 0;
    }

    zUtil_PlayerStateStorage *const localPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
    const float targetDistSq =
        zMath::Vec3DeltaLengthSq(
            &localPlayerState->fxOffsetWorld,
            &aiState->fxOffsetWorld
        );
    if (targetDistSq >= aiState->aiAttackRadiusSq) {
        return 0;
    }

    zVec3 lineOfSightPoint = aiState->fxOffsetWorld;
    lineOfSightPoint.y += kPlayerAiAttackLosTargetYOffset;
    if (HasLineOfSightFromLocalPlayerFxOffset(
        aiState->rootNode,
        &lineOfSightPoint,
        1
    ) == 0) {
        aiState->aiTargetLineOfSightClear = 0;
        return 0;
    }

    AiEnterMode2SteeringPursuit(saveState);
    aiState->aiTargetLineOfSightClear = 1;
    if (aiState->aiNet->attackBuddyNetId != 0) {
        AiAlertAttackBuddies(saveState);
    }
    aiState->aiMode2SteeringRetryCount = 0;
    return 1;
}

/** Reimplements 0x401c00: AINet::AiAlertAttackBuddies (Battlesport/ai_net.cpp).
 * Purpose: Propagates an attack-pursuit alert around the AI peer ring. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::AiAlertAttackBuddies(
    zUtil_SaveGameState *saveState
) {
    zUtil_SaveGameState *buddySaveState = saveState->aiPeerRingNext;
    if (g_Player_AiMode2State1Finalized != 0 || buddySaveState == saveState) {
        return;
    }

    do {
        zUtil_PlayerStateStorage *const buddyState = buddySaveState->playerState;
        if (buddyState->aiTopLevelState != kPlayerAiMode2TopSteering) {
            AiEnterMode2SteeringPursuit(buddySaveState);
            buddyState->recentHitFlag = 1;
            buddyState->recentHitExpireTime = g_Time_AccumulatedTimeSec + 10.0f;
        }
        buddySaveState = buddySaveState->aiPeerRingNext;
    } while (buddySaveState != saveState);
}

/** Reimplements 0x401c60: AINet::AiEnterMode2SteeringPursuit (Battlesport/ai_net.cpp).
 * Purpose: Saves the prior top-level state and enters steering pursuit for the attack window. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::AiEnterMode2SteeringPursuit(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const aiState = saveState->playerState;
    zUtil_PlayerStateStorage *const localPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
    if (g_Player_AiMode2State1Finalized != 0) {
        return;
    }

    const int previousTopLevelState = aiState->aiTopLevelState;
    aiState->aiSavedTopLevelState = previousTopLevelState;
    aiState->aiStateStartTime = g_Player_TotalTimeSecScaled;
    aiState->aiStateEndTime = aiState->aiMode2AttackDwell + g_Player_TotalTimeSecScaled;
    if (previousTopLevelState != kPlayerAiMode2TopSteering) {
        aiState->aiTopLevelState = kPlayerAiMode2TopSteering;
    }

    AINetNode *const restorePathNode =
        aiState->aiCurrentPathNode->neighborNodes[aiState->aiCurrentPathNeighborIndex];
    aiState->aiRestoreTarget = restorePathNode->position;

    if (aiState->aiCurrentSteeringSubstate != kPlayerAiMode2SteerDynamicOffsetTarget) {
        return;
    }

    aiState->aiDynamicOffsetDir.x = aiState->worldPos.x - localPlayerState->worldPos.x;
    aiState->aiDynamicOffsetDir.y = 0.0f;
    aiState->aiDynamicOffsetDir.z = aiState->worldPos.z - localPlayerState->worldPos.z;
    zMath::Vec3Normalize(&aiState->aiDynamicOffsetDir);
}

/**
 * Reimplements 0x401d50: AINet::HasLineOfSightFromLocalPlayerFxOffset
 * (Battlesport/ai_net.cpp).
 * Purpose: tests whether the active local player fx-offset position has an
 * unobstructed ray path to the supplied point while temporarily excluding the
 * tested node and local player root from raycast candidates.
 */
int __fastcall AINet::HasLineOfSightFromLocalPlayerFxOffset(
    zClass_NodePartial *node,
    const zVec3 *point,
    int directionMode
) {
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetRaycastable(
        node,
        0
    );
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    zClass_cls_di::SetBreakOnFirstCandidate(1);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);

    zVec3 startPoint = {0};
    zVec3 endPoint = {0};
    if (directionMode == 1) {
        startPoint = playerState->fxOffsetWorld;
        endPoint = *point;
    } else {
        startPoint = *point;
        endPoint = playerState->fxOffsetWorld;
    }

    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int raycastResult = zClass_cls_di::RaycastFindClosest(
        g_Player_RuntimeDiScene,
        &rayData,
        startPoint.x,
        startPoint.y,
        startPoint.z,
        endPoint.x,
        endPoint.y,
        endPoint.z
    );

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        1
    );
    zClass_Class::gwNodeSetRaycastable(
        node,
        1
    );

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}

/**
 * Reimplements 0x401e50: AINet::HasLineOfSightFromCameraTarget
 * (Battlesport/ai_net.cpp).
 * Purpose: tests whether the active camera target has an unobstructed ray path
 * to the supplied point while temporarily excluding the tested node and local
 * player root from raycast candidates.
 */
int __fastcall AINet::HasLineOfSightFromCameraTarget(
    zClass_NodePartial *node,
    const zVec3 *point,
    int directionMode
) {
    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);

    zVec3 cameraTarget = {0};
    zClass_Camera::gwCameraGetTarget(
        g_MainCamera,
        &cameraTarget.x,
        &cameraTarget.y,
        &cameraTarget.z
    );

    g_Variant_CurrentTag = playerState->variantTag;
    zClass_Class::gwNodeSetRaycastable(
        node,
        0
    );
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    zClass_cls_di::SetBreakOnFirstCandidate(1);
    zClass_cls_di::SetStopAfterFirstHit(0x40000);

    zVec3 startPoint = {0};
    zVec3 endPoint = {0};
    if (directionMode == 1) {
        startPoint = cameraTarget;
        endPoint = *point;
    } else {
        startPoint = *point;
        endPoint = cameraTarget;
    }

    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int raycastResult = zClass_cls_di::RaycastFindClosest(
        g_Player_RuntimeDiScene,
        &rayData,
        startPoint.x,
        startPoint.y,
        startPoint.z,
        endPoint.x,
        endPoint.y,
        endPoint.z
    );

    zClass_cls_di::SetBreakOnFirstCandidate(0);
    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        1
    );
    zClass_Class::gwNodeSetRaycastable(
        node,
        1
    );

    return raycastResult == 0 && rayData.candidateCount != 0 ? 0 : 1;
}

/** Reimplements 0x401f60: AINet::AiRebuildSyntheticPathToNodeIfFar (Battlesport/ai_net.cpp).
 * Purpose: Builds a temporary synthetic AI path node back to the requested target. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::AiRebuildSyntheticPathToNodeIfFar(
    zUtil_SaveGameState *saveState,
    AINetNode *targetNode
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const currentPathNode = playerState->aiCurrentPathNode;

    if (zMath::Vec3DeltaLengthSq(&playerState->worldPos, &currentPathNode->position) <
        kPlayerAiSyntheticPathRebuildDistanceSq) {
        return;
    }

    AINetNode *const syntheticNode = (AINetNode *)(malloc(sizeof(AINetNode)));
    memset(
        syntheticNode,
        0,
        sizeof(*syntheticNode)
    );
    syntheticNode->neighborNodes[0] = targetNode;
    syntheticNode->position = playerState->worldPos;
    syntheticNode->nodeIndex = -1;

    AINetPathProbeFan *const fan = (AINetPathProbeFan *)(malloc(sizeof(AINetPathProbeFan)));
    memset(
        fan,
        0,
        sizeof(*fan)
    );
    syntheticNode->probeFans[0] = fan;
    fan->InitFromSegment(
        syntheticNode->position,
        currentPathNode->position,
        kPlayerAiSyntheticPathWidth
    );

    playerState->aiCurrentPathNode = syntheticNode;
    playerState->aiCurrentPathNeighborIndex = 0;
    playerState->aiNextPathRebuildTime =
        g_Player_TotalTimeSecScaled + kPlayerAiSyntheticPathRebuildDelaySec;
}

/**
 * Reimplements 0x402080: AINet::AiRestoreSavedTopLevelState.
 * BN shows a fastcall leaf that copies playerState->aiSavedTopLevelState to
 * playerState->aiTopLevelState through the save-state's playerState pointer.
 * Purpose: Restores a saved AI top-level state for one player save-state node.
 */
void __fastcall AINet::AiRestoreSavedTopLevelState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->aiTopLevelState = playerState->aiSavedTopLevelState;
}

/** Reimplements 0x402090: AINet::UpdateAiMode2TurnTowardPlayerNoThrottle (Battlesport/ai_net.cpp).
 * Purpose: Turns toward the local player while holding throttle at zero. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::UpdateAiMode2TurnTowardPlayerNoThrottle(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;

    zVec3 targetDelta = {
        targetPlayerState->worldPos.x - playerState->worldPos.x,
        targetPlayerState->worldPos.y - playerState->worldPos.y,
        targetPlayerState->worldPos.z - playerState->worldPos.z,
    };
    targetDelta.y = 0.0f;
    zMath::Vec3Normalize(&targetDelta);

    const float turnCross = playerState->steerBasisNorm.z * targetDelta.x -
                            playerState->steerBasisNorm.x * targetDelta.z;
    const float forwardDot = playerState->steerBasisNorm.x * targetDelta.x +
                             playerState->steerBasisNorm.z * targetDelta.z;
    if (forwardDot <= 0.0f) {
        playerState->steeringInput = turnCross < 0.0f ? -1.0f : 1.0f;
    } else {
        playerState->steeringInput = turnCross;
    }

    playerState->throttleInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
    playerState->steeringInputCopy = playerState->steeringInput;
}

/** Reimplements 0x402170: AINet::UpdateAiMode2TurnInPlaceTowardPlayer (Battlesport/ai_net.cpp).
 * Purpose: Turns in place toward the local player without changing throttle. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::UpdateAiMode2TurnInPlaceTowardPlayer(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState =
        ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;

    zVec3 targetDelta = {
        targetPlayerState->worldPos.x - playerState->worldPos.x,
        targetPlayerState->worldPos.y - playerState->worldPos.y,
        targetPlayerState->worldPos.z - playerState->worldPos.z,
    };
    targetDelta.y = 0.0f;
    zMath::Vec3Normalize(&targetDelta);

    const float turnCross = playerState->steerBasisNorm.z * targetDelta.x -
                            playerState->steerBasisNorm.x * targetDelta.z;
    const float forwardDot = playerState->steerBasisNorm.x * targetDelta.x +
                             playerState->steerBasisNorm.z * targetDelta.z;
    if (forwardDot <= 0.0f) {
        playerState->steeringInput = turnCross < 0.0f ? -1.0f : 1.0f;
    } else {
        playerState->steeringInput = turnCross;
    }

    playerState->steeringInputCopy = playerState->steeringInput;
    playerState->throttleInput = 0.0f;
    playerState->throttleInputCopy = 0.0f;
}

/**
 * Reimplements 0x402250: AINet::TickAiMode2AltGunAttackWindow.
 * Original source path: Battlesport/ai_net.cpp.
 * Purpose: reimplement AINet::TickAiMode2AltGunAttackWindow from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::TickAiMode2AltGunAttackWindow(
    zUtil_SaveGameState *saveState,
    float targetDistance,
    float forwardDot
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (g_Player_TotalTimeSecScaled > playerState->aiStateEndTime) {
        const float startTime = g_Player_TotalTimeSecScaled + playerState->aiNotPursuitDwell;
        playerState->aiStateStartTime = startTime;
        playerState->aiStateEndTime = startTime + playerState->aiMode2AttackDwell;
    }

    PlayerGunFireController *const activeAltGunController = playerState->activeAltGunController;
    zUtil_SaveGameState *const targetSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;

    if (playerState->altGunFireHeldFlag != 0) {
        if (g_Player_TotalTimeSecScaled <= activeAltGunController->nextDispatchTime &&
            forwardDot >= kPlayerAiAltGunAttackForwardMin &&
            targetDistance <= activeAltGunController->aiAttackRangeMax &&
            targetPlayerState->lifecycleState != kPlayerLifecycleInactive) {
            playerState->storedTargetPos = targetPlayerState->fxOffsetWorld;
            return;
        }

        playerState->altGunDispatchRequested = 0;
        activeAltGunController->nextDispatchTime =
            g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
        return;
    }

    if (g_Player_TotalTimeSecScaled <= activeAltGunController->nextDispatchTime ||
        g_Player_TotalTimeSecScaled <= playerState->aiStateStartTime ||
        playerState->damageProtectionActive != 0 || forwardDot <= kPlayerAiAltGunAttackForwardMin ||
        targetDistance >= activeAltGunController->aiAttackRangeMax ||
        targetDistance <= activeAltGunController->aiAttackRangeMin ||
        HasLineOfSightFromLocalPlayerFxOffset(
            playerState->rootNode,
            &playerState->fxOffsetWorld,
            1
        ) == 0 ||
        targetPlayerState->lifecycleState == kPlayerLifecycleInactive) {
        return;
    }

    playerState->altGunDispatchRequested = 1;

    float statusScale = playerState->statusMeterScaled;
    if (statusScale <= kPlayerAiAltGunStatusMinScale) {
        statusScale = kPlayerAiAltGunStatusMinScale;
    }

    activeAltGunController->nextDispatchTime =
        g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay / statusScale;

    OptCatalogEntryDef *const optCatalogEntry = activeAltGunController->optCatalogEntry;
    const unsigned int flags = optCatalogEntry->flags;
    if ((flags & kOptCatalogFlagCreateTrail) != 0) {
        playerState->altGunFireHeldFlag = 1;
        OptCatalog::ActivateTrailRuntimeState(
            activeAltGunController->trailRuntimeState,
            playerState->playerOrdinal
        );
        activeAltGunController->nextDispatchTime =
            g_Player_TotalTimeSecScaled + activeAltGunController->dispatchRepeatDelay;
        return;
    }

    if ((flags & kOptCatalogFlagLockOnTargetRef) != 0) {
        playerState->progressTargetCount = 1;
        playerState->progressTargetSlots[0].targetPos = &targetPlayerState->fxOffsetWorld;
        playerState->progressTargetSlots[0].targetVelocity = &targetPlayerState->projectileSpawnVel;
        HudUi::ShowTopMessageLine(
            zLoc::GetMessageString(0x908),
            5.0f
        );
        return;
    }

    playerState->progressTargetCount = 0;
    playerState->progressTargetSlots[0].targetPos = 0;
    playerState->progressTargetSlots[0].targetVelocity = 0;
    SolveAltGunLeadTargetPoint(
        saveState,
        targetSaveState,
        &playerState->storedTargetPos
    );
}

/**
 * Reimplements 0x4024a0: AINet::SolveAltGunLeadTargetPoint.
 * Original source path: Battlesport/ai_net.cpp.
 * Purpose: reimplement AINet::SolveAltGunLeadTargetPoint from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::SolveAltGunLeadTargetPoint(
    zUtil_SaveGameState *saveState,
    zUtil_SaveGameState *targetSaveState,
    zVec3 *outTargetPos
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetSaveState->playerState;
    const float inverseProjectileVelocity =
        1.0f / playerState->activeAltGunController->optCatalogEntry->velocity;

    zVec3 scaledTargetDelta = {
        (targetPlayerState->worldPos.x - playerState->worldPos.x) * inverseProjectileVelocity,
        (targetPlayerState->worldPos.y - playerState->worldPos.y) * inverseProjectileVelocity,
        (targetPlayerState->worldPos.z - playerState->worldPos.z) * inverseProjectileVelocity,
    };
    zVec3 relativeVelocity = {
        targetPlayerState->projectileSpawnVel.x - playerState->projectileSpawnVel.x,
        targetPlayerState->projectileSpawnVel.y - playerState->projectileSpawnVel.y,
        targetPlayerState->projectileSpawnVel.z - playerState->projectileSpawnVel.z,
    };
    zVec3 scaledRelativeVelocity = {
        relativeVelocity.x * inverseProjectileVelocity,
        relativeVelocity.y * inverseProjectileVelocity,
        relativeVelocity.z * inverseProjectileVelocity,
    };

    const float relativeSpeedSq = scaledRelativeVelocity.x * scaledRelativeVelocity.x +
                                  scaledRelativeVelocity.y * scaledRelativeVelocity.y +
                                  scaledRelativeVelocity.z * scaledRelativeVelocity.z;
    const float quadraticA = 1.0f - relativeSpeedSq;
    if (quadraticA <= 0.0f) {
        *outTargetPos = targetPlayerState->worldPos;
        return;
    }

    const float quadraticB = scaledRelativeVelocity.x * scaledTargetDelta.x +
                             scaledRelativeVelocity.y * scaledTargetDelta.y +
                             scaledRelativeVelocity.z * scaledTargetDelta.z;
    const float targetDistanceSq = scaledTargetDelta.x * scaledTargetDelta.x +
                                   scaledTargetDelta.y * scaledTargetDelta.y +
                                   scaledTargetDelta.z * scaledTargetDelta.z;
    const float discriminant = quadraticA * targetDistanceSq + quadraticB * quadraticB;
    const float leadScale = (PlayerFastSqrtEstimate(discriminant) + quadraticB) / quadraticA;

    scaledRelativeVelocity.x = relativeVelocity.x * leadScale;
    scaledRelativeVelocity.y = relativeVelocity.y * leadScale;
    scaledRelativeVelocity.z = relativeVelocity.z * leadScale;
    outTargetPos->x = targetPlayerState->fxOffsetWorld.x + scaledRelativeVelocity.x;
    outTargetPos->y = targetPlayerState->fxOffsetWorld.y + scaledRelativeVelocity.y;
    outTargetPos->z = targetPlayerState->fxOffsetWorld.z + scaledRelativeVelocity.z;
    outTargetPos->y -= ((float)(rand()) * 3.05185094e-05f - 0.5f) * -2.0f;
}

/** Reimplements 0x4026d0: AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget (Battlesport/ai_net.cpp).
 * Purpose: Rotates the target-to-AI vector by accepted tuning globals and steers to the offset point. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget(
    zUtil_SaveGameState *saveState,
    zUtil_SaveGameState *targetState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetState->playerState;
    const float offsetDistance = playerState->aiNet->pursuitParam0;

    zVec3 targetToPlayerDir = {
        playerState->worldPos.x - targetPlayerState->worldPos.x,
        playerState->worldPos.y - targetPlayerState->worldPos.y,
        playerState->worldPos.z - targetPlayerState->worldPos.z,
    };
    targetToPlayerDir.y = 0.0f;
    zMath::Vec3Normalize(&targetToPlayerDir);

    zVec3 steerOffsetDir = {0};
    steerOffsetDir.y = 0.0f;
    steerOffsetDir.x =
        offsetDistance * (g_Player_AiMode2_OffsetTargetRotateCos15Deg * targetToPlayerDir.x -
                             g_Player_AiMode2_OffsetTargetRotateSin15Deg * targetToPlayerDir.z);
    steerOffsetDir.z =
        offsetDistance * (g_Player_AiMode2_OffsetTargetRotateCos15Deg * targetToPlayerDir.z +
                             g_Player_AiMode2_OffsetTargetRotateSin15Deg * targetToPlayerDir.x);

    zVec3 offsetTarget = {
        targetPlayerState->worldPos.x + steerOffsetDir.x,
        targetPlayerState->worldPos.y + steerOffsetDir.y,
        targetPlayerState->worldPos.z + steerOffsetDir.z,
    };

    zVec3 targetDir = {
        offsetTarget.x - playerState->worldPos.x,
        offsetTarget.y - playerState->worldPos.y,
        offsetTarget.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    zMath::Vec3Normalize(&targetDir);

    const float forwardDot =
        playerState->steerBasisNorm.x * targetDir.x + playerState->steerBasisNorm.z * targetDir.z;
    const float turnCross =
        playerState->steerBasisNorm.z * targetDir.x - playerState->steerBasisNorm.x * targetDir.z;

    if (forwardDot < 0.0f) {
        playerState->throttleInput = 0.0f;
        playerState->steeringInput = turnCross < 0.0f ? -1.0f : 1.0f;
    } else {
        float throttle = 1.0f - (float)(fabs(turnCross));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->throttleInput = throttle;
        playerState->steeringInput = turnCross;
    }

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

/** Reimplements 0x4028c0: AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget (Battlesport/ai_net.cpp).
 * Purpose: Blends dynamic pursuit and side-offset steering based on distance to the local player. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(
    zUtil_SaveGameState *saveState,
    zUtil_SaveGameState *targetState,
    float targetDistance
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zUtil_PlayerStateStorage *const targetPlayerState = targetState->playerState;
    AINet *const aiNet = playerState->aiNet;
    const float pursuitDistance = aiNet->pursuitParam0;
    const float sideOffsetScale = aiNet->pursuitParam1;
    const float doublePursuitDistance = pursuitDistance + pursuitDistance;

    zVec3 dynamicOffsetDir = playerState->aiDynamicOffsetDir;
    zVec3 targetPoint = {
        targetPlayerState->worldPos.x + pursuitDistance * dynamicOffsetDir.x,
        targetPlayerState->worldPos.y + pursuitDistance * dynamicOffsetDir.y,
        targetPlayerState->worldPos.z + pursuitDistance * dynamicOffsetDir.z,
    };

    float blend = (doublePursuitDistance - targetDistance) / pursuitDistance;
    if (blend > 1.0f) {
        blend = 1.0f;
    } else if (blend < 0.0f) {
        blend = 0.0f;
    }

    int reverseSideOffset = 0;
    float signedSideScale = blend * sideOffsetScale;
    if (playerState->localVel.z > 0.0f && targetDistance < doublePursuitDistance) {
        reverseSideOffset = 1;
        signedSideScale = -signedSideScale;
    }

    zVec3 sideOffset = {
        dynamicOffsetDir.z * signedSideScale,
        targetPoint.y * signedSideScale,
        -dynamicOffsetDir.x * signedSideScale,
    };
    targetPoint.x += sideOffset.x;
    targetPoint.y += sideOffset.y;
    targetPoint.z += sideOffset.z;

    zVec3 targetDir = {
        targetPoint.x - playerState->worldPos.x,
        targetPoint.y - playerState->worldPos.y,
        targetPoint.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    const float targetDirDistance = zMath::Vec3Normalize(&targetDir);

    zVec3 steerBasis = playerState->steerBasisNorm;
    if (reverseSideOffset != 0) {
        steerBasis.x = -steerBasis.x;
        steerBasis.z = -steerBasis.z;
    }

    const float forwardDot = steerBasis.x * targetDir.x + steerBasis.z * targetDir.z;
    const float turnCross = steerBasis.z * targetDir.x - steerBasis.x * targetDir.z;

    if (forwardDot < 0.0f && targetDirDistance < kPlayerAiDynamicOffsetBackUpDistance) {
        playerState->throttleInput = -1.0f;
        playerState->steeringInput = 0.0f;
    } else {
        float throttle = 1.0f - (float)(fabs(turnCross));
        if (throttle <= kPlayerAiPathFollowMinThrottle) {
            throttle = kPlayerAiPathFollowMinThrottle;
        }
        playerState->throttleInput = throttle;
        playerState->steeringInput = turnCross;
    }

    if (reverseSideOffset != 0) {
        playerState->throttleInput = -playerState->throttleInput;
    }
    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
}

/** Reimplements 0x402b70: AINet::TickAiMode2TimedPathSteering (Battlesport/ai_net.cpp).
 * Purpose: Alternates timed forward and reverse path-node steering around the AI home path node. Source model: AINet source-file contribution over save-state/playerState, not a Player class. */
void __fastcall AINet::TickAiMode2TimedPathSteering(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;

    if (g_Player_TotalTimeSecScaled > playerState->unknown_0fa4) {
        AINetNode *const currentPathNode = playerState->aiCurrentPathNode;
        AINetNode *const pathAnchorNode = playerState->aiHomePathNode;

        if (currentPathNode == pathAnchorNode) {
            AiSteerTowardPathNodeForward(saveState);
        } else if (currentPathNode->neighborNodes[0] == pathAnchorNode &&
                   currentPathNode->nodeIndex != -1) {
            AiSteerTowardPathNodeReverse(saveState);
        } else {
            TickAiMode2PathFollow(saveState);
        }
    }

    playerState->recentHitFlag = 1;
}

/**
 * Reimplements 0x402be0: AINet::AiSteerTowardPathNodeForward.
 * Original source path: Battlesport/ai_net.cpp.
 * Purpose: reimplement AINet::AiSteerTowardPathNodeForward from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::AiSteerTowardPathNodeForward(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const forwardNode = playerState->aiCurrentPathNode->neighborNodes[0];

    zVec3 targetDir = {
        forwardNode->position.x - playerState->worldPos.x,
        forwardNode->position.y - playerState->worldPos.y,
        forwardNode->position.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDir);

    if (targetDistance < kPlayerAiForwardPathAdvanceDistance) {
        playerState->aiCurrentPathNode = forwardNode;
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        playerState->unknown_0fa4 = g_Player_TotalTimeSecScaled + 4.0f;
        return;
    }

    const float forwardDot =
        playerState->steerBasisNorm.x * targetDir.x + playerState->steerBasisNorm.z * targetDir.z;
    const float turnCross =
        playerState->steerBasisNorm.z * targetDir.x - playerState->steerBasisNorm.x * targetDir.z;

    if (forwardDot < 0.0f) {
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = turnCross < 0.0f ? -1.0f : 1.0f;
        playerState->steeringInput = playerState->steeringInputCopy;
        return;
    }

    float throttle = 1.0f - (float)(fabs(turnCross));
    if (throttle <= kPlayerAiPathFollowMinThrottle) {
        throttle = kPlayerAiPathFollowMinThrottle;
    }
    playerState->throttleInputCopy = throttle;
    playerState->throttleInput = throttle;
    playerState->steeringInputCopy = turnCross;
    playerState->steeringInput = turnCross;
}

/**
 * Reimplements 0x402d60: AINet::AiSteerTowardPathNodeReverse.
 * Original source path: Battlesport/ai_net.cpp.
 * Purpose: reimplement AINet::AiSteerTowardPathNodeReverse from the recovered
 * Battlesport ai_net.cpp source-file contribution.
 */
void __fastcall AINet::AiSteerTowardPathNodeReverse(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    AINetNode *const forwardNode = playerState->aiCurrentPathNode->neighborNodes[0];

    zVec3 targetDir = {
        forwardNode->position.x - playerState->worldPos.x,
        forwardNode->position.y - playerState->worldPos.y,
        forwardNode->position.z - playerState->worldPos.z,
    };
    targetDir.y = 0.0f;
    const float targetDistance = zMath::Vec3Normalize(&targetDir);

    if (targetDistance < kPlayerAiForwardPathAdvanceDistance) {
        playerState->aiCurrentPathNode = forwardNode;
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = 0.0f;
        playerState->steeringInput = 0.0f;
        playerState->unknown_0fa4 = g_Player_TotalTimeSecScaled + 14.0f;
        return;
    }

    const zVec3 reverseSteerBasis = {
        -playerState->steerBasisNorm.x,
        playerState->steerBasisNorm.y,
        -playerState->steerBasisNorm.z,
    };
    const float forwardDot = reverseSteerBasis.x * targetDir.x + reverseSteerBasis.z * targetDir.z;
    const float turnCross = reverseSteerBasis.z * targetDir.x - reverseSteerBasis.x * targetDir.z;

    if (forwardDot < 0.0f) {
        playerState->throttleInputCopy = 0.0f;
        playerState->throttleInput = 0.0f;
        playerState->steeringInputCopy = turnCross < 0.0f ? -1.0f : 1.0f;
        playerState->steeringInput = playerState->steeringInputCopy;
        return;
    }

    float throttle = 1.0f - (float)(fabs(turnCross));
    if (throttle <= kPlayerAiPathFollowMinThrottle) {
        throttle = kPlayerAiPathFollowMinThrottle;
    }
    throttle = -throttle;
    playerState->throttleInputCopy = throttle;
    playerState->throttleInput = throttle;
    playerState->steeringInputCopy = turnCross;
    playerState->steeringInput = turnCross;
}

/**
 * Reimplements 0x402f10: AINet::AiFinalizeMode2State1ForAllPlayers.
 * BN shows traversal from g_PlayerSaveStateListHead, filtering
 * lifecycleState == 2 and aiTopLevelState == 1, restoring matching nodes, and
 * setting g_Player_AiMode2State1Finalized to 1 after the pass.
 * Purpose: Finalizes AI Mode2 State1 by restoring saved top-level state for
 * active AI players and setting the global finalization latch.
 */
void AINet::AiFinalizeMode2State1ForAllPlayers() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (playerState->lifecycleState == 2 && playerState->aiTopLevelState == 1) {
            AiRestoreSavedTopLevelState(saveState);
        }

        saveState = saveState != 0 ? saveState->next : 0;
    }

    g_Player_AiMode2State1Finalized = 1;
}

#include "GameZRecoil/zMath/zMath_vec3_normalize.inl"

/**
 * Reimplements 0x402fd0: AINet::LoadAllFromZrd (Battlesport/ai_net.cpp).
 * Purpose: Loads every numbered AI path network definition from the mission ZRD set.
 */
void AINet::LoadAllFromZrd() {
    for (int netId = 1; netId < 100; ++netId) {
        AINet::LoadFromZrd(netId);
    }
}

/**
 * Reimplements 0x402ff0: AINet::Alloc (Battlesport/ai_net.cpp).
 * Purpose: Allocates a zeroed AI network record and appends it to the global AI network list.
 */
AINet *AINet::Alloc() {
    AINet *const aiNet = (AINet *)(malloc(sizeof(AINet)));
    memset(
        aiNet,
        0,
        sizeof(AINet)
    );

    if (g_AINetListHead != 0) {
        g_AINetListTail->next = aiNet;
        g_AINetListTail = aiNet;
        return aiNet;
    }

    g_AINetListHead = aiNet;
    g_AINetListTail = aiNet;
    return aiNet;
}

/**
 * Reimplements 0x403040: AINet::LoadFromZrd (Battlesport/ai_net.cpp).
 * Purpose: Parses one AI path network ZRD, builds its node list, resolves links, and returns the loaded network.
 */
AINet *__fastcall AINet::LoadFromZrd(
    int netId
) {
    char baseName[0x10];
    sprintf(
        baseName,
        g_AINet_NetNameFormat,
        netId
    );

    char path[0x104];
    sprintf(
        path,
        g_AINet_ZrdNameFormat,
        baseName
    );

    zReader::Node *const root = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    if (root == 0) {
        return 0;
    }

    zReader::Node *versionNode = zReader_GetNamedNode(
        root,
        g_AINet_VersionFieldName
    );
    if (versionNode != 0 && versionNode->value.nodes[1].value.i32 != 105) {
        zError::ReportOld(
            0x200,
            g_AINet_SourceFilePath,
            0x8c,
            g_AINet_WrongVersionMessage
        );
        return 0;
    }

    AINet *const aiNet = AINet::Alloc();
    aiNet->netId = netId;

    zReader::Node *nameNode = zReader_GetNamedNode(
        root,
        g_AINet_NameFieldName
    );
    strcpy(
        aiNet->name,
        nameNode != 0 ? nameNode->value.nodes[1].value.str : baseName
    );

    char token[0x18];
    zReader::Node *typeNode = zReader_GetNamedNode(
        root,
        g_AINet_TypeFieldName
    );
    if (typeNode != 0) {
        strcpy(
            token,
            typeNode->value.nodes[1].value.str
        );
        _strupr(token);
    }

    if (typeNode == 0 || strncmp(
        token,
        g_AINet_TypeTokenSt,
        2
    ) == 0) {
        aiNet->aiType = AINET_TYPE_ST;
    } else if (strncmp(
        token,
        g_AINet_TypeTokenHi,
        2
    ) == 0) {
        aiNet->aiType = AINET_TYPE_HI;
    } else if (strncmp(
        token,
        g_AINet_TypeTokenFi,
        2
    ) == 0) {
        aiNet->aiType = AINET_TYPE_FI;
    } else if (strncmp(
        token,
        g_AINet_TypeTokenDe,
        2
    ) == 0) {
        aiNet->aiType = AINET_TYPE_DE;
    }

    zReader::Node *pathWidthNode = zReader_GetNamedNode(
        root,
        g_AINet_PathWidthFieldName
    );
    aiNet->pathWidth = pathWidthNode != 0 ? pathWidthNode->value.nodes[1].value.f32 : 10.0f;

    zReader::Node *activateRadiusNode = zReader_GetNamedNode(
        root,
        g_AINet_ActivateRadiusFieldName
    );
    if (activateRadiusNode != 0) {
        aiNet->activateRadius = activateRadiusNode->value.nodes[1].value.f32;
    }

    zReader::Node *attackRadiusNode = zReader_GetNamedNode(
        root,
        g_AINet_AttackRadiusFieldName
    );
    if (attackRadiusNode != 0) {
        aiNet->attackRadius = attackRadiusNode->value.nodes[1].value.f32;
    }

    zReader::Node *attackDwellNode = zReader_GetNamedNode(
        root,
        g_AINet_AttackDwellFieldName
    );
    if (attackDwellNode != 0) {
        aiNet->attackDwell = attackDwellNode->value.nodes[1].value.f32;
    }

    zReader::Node *pursuitNode = zReader_GetNamedNode(
        root,
        g_AINet_PursuitParamsFieldName
    );
    if (pursuitNode == 0) {
        pursuitNode = zReader_GetNamedNode(
            root,
            g_AINet_PursuitRangeFieldName
        );
    }
    if (pursuitNode != 0) {
        zReader::Node *const payload = pursuitNode->value.nodes;
        aiNet->pursuitParam0 = payload[1].value.f32;
        aiNet->pursuitParam1 = payload[2].value.f32;
    }

    zReader::Node *notPursuitDwellNode = zReader_GetNamedNode(
        root,
        g_AINet_NotPursuitDwellFieldName
    );
    if (notPursuitDwellNode != 0) {
        aiNet->notPursuitDwell = notPursuitDwellNode->value.nodes[1].value.f32;
    }

    zReader::Node *returnRangeNode = zReader_GetNamedNode(
        root,
        g_AINet_ReturnRangeFieldName
    );
    if (returnRangeNode != 0) {
        aiNet->returnRange = returnRangeNode->value.nodes[1].value.f32;
    }

    zReader::Node *hideTimesNode = zReader_GetNamedNode(
        root,
        g_AINet_HideTimesFieldName
    );
    if (hideTimesNode != 0) {
        zReader::Node *const payload = hideTimesNode->value.nodes;
        aiNet->hideTime0 = payload[1].value.f32;
        aiNet->hideTime1 = payload[2].value.f32;
    } else {
        aiNet->hideTime0 = 8.0f;
        aiNet->hideTime1 = 4.0f;
    }

    zReader::Node *attackBuddyNode = zReader_GetNamedNode(
        root,
        g_AINet_AttackBuddyFieldName
    );
    aiNet->attackBuddyNetId = attackBuddyNode != 0 ? attackBuddyNode->value.nodes[1].value.i32 : 0;

    zReader::Node *activateBuddyNode = zReader_GetNamedNode(
        root,
        g_AINet_ActivateBuddyFieldName
    );
    if (activateBuddyNode != 0) {
        aiNet->activateBuddyNetId = activateBuddyNode->value.nodes[1].value.i32;
    } else {
        aiNet->attackBuddyNetId = 0;
    }

    zReader::Node *attackStrategyNode = zReader_GetNamedNode(
        root,
        g_AINet_AttackStrategyFieldName
    );
    if (attackStrategyNode != 0) {
        strcpy(
            token,
            attackStrategyNode->value.nodes[1].value.str
        );
        _strupr(token);
        if (strncmp(
            token,
            g_AINet_AttackStrategyTokenFollow,
            3
        ) == 0) {
            aiNet->attackStrategy = AINET_STRAT_FOL;
        } else if (strncmp(
            token,
            g_AINet_AttackStrategyTokenCircle,
            3
        ) == 0) {
            aiNet->attackStrategy = AINET_STRAT_CIR;
        } else if (strncmp(
            token,
            g_AINet_AttackStrategyTokenHeadOn,
            3
        ) == 0) {
            aiNet->attackStrategy = AINET_STRAT_HEA;
        } else if (strncmp(
            token,
            g_AINet_AttackStrategyTokenBack,
            3
        ) == 0) {
            aiNet->attackStrategy = AINET_STRAT_BAC;
        } else if (strncmp(
            token,
            g_AINet_AttackStrategyTokenZig,
            3
        ) == 0) {
            aiNet->attackStrategy = AINET_STRAT_ZIG;
        } else if (strncmp(
            token,
            g_AINet_AttackStrategyTokenSit,
            3
        ) == 0) {
            aiNet->attackStrategy = AINET_STRAT_SIT;
        }
    } else {
        aiNet->attackStrategy = AINET_STRAT_HEA;
    }

    AINetNode *tail = 0;
    for (int nodeIndex = 0; nodeIndex < 99; ++nodeIndex) {
        char nodeName[0x10];
        sprintf(
            nodeName,
            g_AINet_NodeNameFormat,
            nodeIndex
        );

        zReader::Node *node = zReader_GetNamedNode(
            root,
            nodeName
        );
        if (node == 0) {
            continue;
        }

        AINetNode *const aiNode = (AINetNode *)(malloc(sizeof(AINetNode)));
        memset(
            aiNode,
            0,
            sizeof(AINetNode)
        );

        if (tail != 0) {
            tail->next = aiNode;
        } else {
            aiNet->nodeListHead = aiNode;
        }
        tail = aiNode;

        zReader::Node *const payload = node->value.nodes;
        zReader::Node *const position = payload[2].value.nodes;
        zReader::Node *const neighbors = payload[3].value.nodes;

        aiNode->nodeIndex = nodeIndex;
        aiNode->costOrType = payload[1].value.i32;
        aiNode->position.x = position[1].value.f32;
        aiNode->position.y = position[2].value.f32;
        aiNode->position.z = position[3].value.f32;
        aiNode->neighborIndices[0] = neighbors[1].value.i32;
        aiNode->neighborIndices[1] = neighbors[2].value.i32;
        aiNode->neighborIndices[2] = neighbors[3].value.i32;
    }

    AINet::ResolveNeighborLinksAndBuildProbeFans(
        aiNet->nodeListHead,
        aiNet->pathWidth
    );
    zReader::FreeLoadedTree(root);
    return aiNet;
}

/**
 * Reimplements 0x403510: AINet::FindByNetId (Battlesport/ai_net.cpp).
 * Purpose: Finds the first loaded AI network with the requested network id.
 */
AINet *__fastcall AINet::FindByNetId(
    int netId
) {
    AINet *aiNet = g_AINetListHead;
    while (aiNet != 0) {
        if (aiNet->netId == netId) {
            return aiNet;
        }
        aiNet = aiNet->next;
    }

    return 0;
}

/**
 * Reimplements 0x403530: AINet::FindNodeByIndex (Battlesport/ai_net.cpp).
 * Purpose: Finds the first AI path node with the requested parsed node index.
 */
AINetNode *__fastcall AINet::FindNodeByIndex(
    int nodeIndex,
    AINetNode *nodeListHead
) {
    AINetNode *node = nodeListHead;
    while (node != 0) {
        if (node->nodeIndex == nodeIndex) {
            return node;
        }
        node = node->next;
    }

    return 0;
}

/**
 * Reimplements 0x403550: AINet::ResolveNeighborLinksAndBuildProbeFans (Battlesport/ai_net.cpp).
 * Purpose: Resolves neighbor indices into node pointers and allocates probe fans for valid AI path links.
 */
void __fastcall AINet::ResolveNeighborLinksAndBuildProbeFans(
    AINetNode *nodeListHead,
    float pathWidth
) {
    AINetNode *node = nodeListHead;
    while (node != 0) {
        for (int slot = 0; slot < 3; ++slot) {
            const int neighborIndex = node->neighborIndices[slot];
            if (neighborIndex < 0) {
                node->neighborNodes[slot] = 0;
                node->probeFans[slot] = 0;
            } else {
                AINetNode *const neighbor = AINet::FindNodeByIndex(
                    neighborIndex,
                    nodeListHead
                );
                node->neighborNodes[slot] = neighbor;

                AINetPathProbeFan *const probeFan =
                    (AINetPathProbeFan *)(malloc(sizeof(AINetPathProbeFan)));
                node->probeFans[slot] = probeFan;
                memset(
                    probeFan,
                    0,
                    sizeof(AINetPathProbeFan)
                );
                probeFan->InitFromSegment(
                    node->position,
                    neighbor->position,
                    pathWidth
                );
            }
        }

        node = node->next;
    }
}

/**
 * Reimplements 0x403620: AINetPathProbeFan::InitFromSegment (Battlesport/ai_net.cpp).
 * Purpose: Builds the normalized path-probe fan basis and travel clamp for one AI navigation segment.
 */
void AINetPathProbeFan::InitFromSegment(
    zVec3 fromPosition,
    zVec3 toPosition,
    float pathWidth
) {
    delta.x = toPosition.x - fromPosition.x;
    delta.y = toPosition.y - fromPosition.y;
    delta.z = toPosition.z - fromPosition.z;

    const float xzLength = sqrt(delta.x * delta.x + delta.z * delta.z);
    const float minimumTravel = pathWidth * g_AINetPathProbeHalfWidthScale;
    const float availableTravel = xzLength - pathWidth;
    clampedTravel = availableTravel < minimumTravel ? minimumTravel : availableTravel;

    zMath::Vec3NormalizeXZ(
        &delta,
        &delta
    );
    zMath::Vec3PerpXZ(
        &delta,
        &perpendicular
    );
    zMath::Vec3RotateY(
        &probeDirPlus45,
        &perpendicular,
        45.0f
    );
    zMath::Vec3RotateY(
        &probeDirMinus45,
        &perpendicular,
        -45.0f
    );
    this->pathWidth = pathWidth;
}

/**
 * Reimplements 0x4036f0: AINet::FindNearestNode (Battlesport/ainet.cpp).
 * Purpose: Scans an AI node list and returns the node nearest to the query position.
 */
AINetNode *__fastcall AINet::FindNearestNode(
    const zVec3 *position,
    AINetNode *nodeListHead
) {
    AINetNode *node = nodeListHead;
    AINetNode *nearest = 0;
    float bestDistanceSq = -1.0f;

    while (node != 0) {
        const float distanceSq = zMath::Vec3DeltaLengthSq(
            position,
            &node->position
        );
        if (distanceSq < bestDistanceSq || bestDistanceSq < 0.0f) {
            bestDistanceSq = distanceSq;
            nearest = node;
        }
        node = node->next;
    }

    return nearest;
}

/**
 * Reimplements 0x403750: AINet::BuildAiPeerRingsByAiNetId.
 * Source placement audit: BN file-literal order makes accepted player.cpp
 * ownership invalid; this body remains here only until the AINet remap can
 * pull the required save-state declarations with it.
 * Purpose: link active save states sharing one AI network id into peer rings
 * used by AI bootstrap while leaving inactive/unlinked records untouched.
 * Source owner: pending battlesport_ai.ainet_peer_ring_build audit.
 */
void AINet::BuildAiPeerRingsByAiNetId() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateListHead;
    while (saveState != 0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        const int aiNetId = playerState->aiNetId;
        zUtil_SaveGameState *candidate = saveState != 0 ? saveState->next : 0;
        while (candidate != 0) {
            zUtil_PlayerStateStorage *const candidatePlayerState = candidate->playerState;
            if (candidatePlayerState->aiNetId == aiNetId &&
                candidatePlayerState->lifecycleState != kPlayerLifecycleInactive &&
                candidate->aiPeerRingNext == candidate) {
                candidate->aiPeerRingNext = saveState->aiPeerRingNext;
                saveState->aiPeerRingNext = candidate;
            }
            candidate = candidate != 0 ? candidate->next : 0;
        }
        saveState = saveState != 0 ? saveState->next : 0;
    }
}

/**
 * Reimplements 0x4037c0: AINetNode::Free (Battlesport/ainet.cpp).
 * Purpose: Frees a path node and any probe-fan records allocated for its neighbor links.
 */
void AINetNode::Free() {
    if (this == 0) {
        return;
    }

    {
        for (int index = 0; index < 3; ++index) {
            AINetPathProbeFan *probeFan = probeFans[index];
            if (probeFan != 0) {
                free(probeFan);
            }
        }
    }

    free(this);
}

/**
 * Reimplements 0x403800: AINet::Free (Battlesport/ainet.cpp).
 * Purpose: Frees every node owned by this AI network and then releases the network record.
 */
void AINet::Free() {
    if (this == 0) {
        return;
    }

    AINetNode *node = nodeListHead;
    while (node != 0) {
        AINetNode *const current = node;
        node = node->next;
        current->Free();
    }

    free(this);
}

/**
 * Reimplements 0x403830: AINet::AiDiscardNegativeBranchPathNodes
 * (Battlesport/ainet.cpp).
 *
 * Purpose: discard temporary negative-index AI path nodes before the saved
 * player state releases or resumes its current path cursor.
 */
void __fastcall AINet::AiDiscardNegativeBranchPathNodes(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *playerState = saveState->playerState;
    AINetNode *aiCurrentPathNode = playerState->aiCurrentPathNode;
    if (aiCurrentPathNode == 0 || aiCurrentPathNode->nodeIndex >= 0) {
        return;
    }

    do {
        playerState->aiCurrentPathNode = aiCurrentPathNode->neighborNodes[0];
        aiCurrentPathNode->Free();
        aiCurrentPathNode = playerState->aiCurrentPathNode;
    } while (aiCurrentPathNode->nodeIndex < 0);
}

/**
 * Reimplements 0x403870: AINet::FreeAll (Battlesport/ainet.cpp).
 * Purpose: Walks the global AI network list and frees every loaded network.
 */
void AINet::FreeAll() {
    AINet *aiNet = g_AINetListHead;
    while (aiNet != 0) {
        g_AINetListHead = aiNet->next;
        aiNet->Free();
        aiNet = g_AINetListHead;
    }
}
