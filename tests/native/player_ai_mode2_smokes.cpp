// Checked-in focused native smoke translation unit, formerly extracted from player_tests.cpp.
// Emits selected player smoke functions to avoid duplicate smoke definitions.

#include "Battlesport/game_net.h"
#include "Battlesport/briefing.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/ai_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <windows.h>

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>


namespace {
template <typename T> T *AllocZeroedMalloc() {
    void *const mem = std::calloc(1, sizeof(T));
    return static_cast<T *>(mem);
}

template <typename T> T *AllocZeroedNew() {
    T *const value = static_cast<T *>(::operator new(sizeof(T)));
    std::memset(value, 0, sizeof(T));
    return value;
}

bool Vec3Equals(const zVec3 &value, const zVec3 &expected) {
    return value.x == expected.x && value.y == expected.y && value.z == expected.z;
}

bool FloatNear(float actual, float expected) {
    return std::fabs(actual - expected) < 0.0001f;
}

PlayerPendingContact *MakePendingContactChain(int count) {
    PlayerPendingContact *head = nullptr;
    PlayerPendingContact *tail = nullptr;
    for (int i = 0; i < count; ++i) {
        PlayerPendingContact *const contact = AllocZeroedNew<PlayerPendingContact>();
        if (head == nullptr) {
            head = contact;
        } else {
            tail->next = contact;
        }
        tail = contact;
    }
    return head;
}

void FillPendingContactQueue(PlayerPendingContactQueue *queue, int count) {
    queue->listAux = 0x12340000 + count;
    queue->head = MakePendingContactChain(count);
    queue->tail = queue->head;
    while (queue->tail != nullptr && queue->tail->next != nullptr) {
        queue->tail = queue->tail->next;
    }
    queue->count = count;
}

bool PendingContactQueueCleared(const PlayerPendingContactQueue &queue) {
    return queue.listAux == 0 && queue.head == nullptr && queue.tail == nullptr &&
           queue.count == 0;
}

bool PendingContactPayloadMatches(const PlayerPendingContact *contact,
                                  const zClassDiPickCandidateEntry &candidate,
                                  const zVec3 &segmentStart, const zVec3 &segmentEnd,
                                  int segmentTag) {
    return contact != nullptr && contact->hit.node == candidate.node &&
           contact->hit.scenePayload == candidate.scenePayload &&
           contact->hit.surfaceNormal.y == candidate.surfaceNormal.y &&
           Vec3Equals(contact->sweepStart, segmentStart) &&
           Vec3Equals(contact->sweepEnd, segmentEnd) && contact->segmentTag == segmentTag;
}

void InitObjectPositionNode(zClass_NodePartial *node, zClass_Object3DDataPartial *data,
                            float x, float y, float z) {
    std::memset(node, 0, sizeof(*node));
    std::memset(data, 0, sizeof(*data));
    node->classId = 5;
    node->classData = data;
    node->flags = 1;
    data->localMatrix[9] = x;
    data->localMatrix[10] = y;
    data->localMatrix[11] = z;
}
} // namespace

extern "C" int player_tick_ai_mode2_top_level_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldFinalized = g_Player_AiMode2State1Finalized;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState localGameState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    AINetNode currentPathNode = {};
    AINetNode nextPathNode = {};
    saveState.playerState = &playerState;
    localGameState.playerState = &localPlayerState;
    currentPathNode.neighborNodes[0] = &nextPathNode;
    playerState.aiCurrentPathNode = &currentPathNode;
    playerState.aiCurrentPathNeighborIndex = 0;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    localPlayerState.fxOffsetWorld = {12.0f, 13.0f, 14.0f};
    localPlayerState.worldPos = {10.0f, 0.0f, 0.0f};
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localGameState;
    g_Player_AiMode2State1Finalized = 1;

    playerState.aiTopLevelState = 6;
    playerState.storedTargetPos = {};
    AINet::TickAiMode2TopLevel(&saveState);
    const bool snapshotDefaultOk =
        Vec3Equals(playerState.storedTargetPos, {12.0f, 13.0f, 14.0f}) &&
        playerState.aiTopLevelState == 6;

    playerState.aiTopLevelState = 2;
    playerState.throttleInput = 0.75f;
    playerState.throttleInputCopy = 0.75f;
    playerState.steeringInput = 0.5f;
    playerState.steeringInputCopy = 0.5f;
    AINet::TickAiMode2TopLevel(&saveState);
    const bool turnTowardOk =
        playerState.aiTopLevelState == 2 && playerState.throttleInput == 0.0f &&
        playerState.throttleInputCopy == 0.0f &&
        FloatNear(playerState.steeringInput, 0.0f) &&
        FloatNear(playerState.steeringInputCopy, 0.0f);

    playerState.aiTopLevelState = 5;
    playerState.aiReturnTopLevelState = 4;
    playerState.autoTurnActive = 0;
    playerState.autoTurnSign = -1;
    AINet::TickAiMode2TopLevel(&saveState);
    const bool autoTurnRestoreOk =
        playerState.aiTopLevelState == 4 && playerState.autoTurnSign == 0;

    playerState.aiTopLevelState = 5;
    playerState.aiReturnTopLevelState = 3;
    playerState.autoTurnActive = 1;
    playerState.autoTurnSign = 1;
    AINet::TickAiMode2TopLevel(&saveState);
    const bool autoTurnActiveOk =
        playerState.aiTopLevelState == 5 && playerState.autoTurnSign == 0;

    g_Player_AiMode2State1Finalized = oldFinalized;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!snapshotDefaultOk) {
        return 1;
    }
    if (!turnTowardOk) {
        return 2;
    }
    if (!autoTurnRestoreOk) {
        return 3;
    }
    return autoTurnActiveOk ? 0 : 4;
}

extern "C" int player_build_ai_peer_rings_by_ai_net_id_smoke(void) {
    zUtil_SaveGameState *const oldHead = g_PlayerSaveStateListHead;
    zUtil_SaveGameState *const oldTail = g_PlayerSaveStateListTail;
    const int oldCount = g_PlayerSaveStateCount;

    zUtil_SaveGameState saves[5] = {};
    zUtil_PlayerStateStorage playerStates[5] = {};
    for (int i = 0; i < 5; ++i) {
        saves[i].playerState = &playerStates[i];
        saves[i].aiPeerRingNext = &saves[i];
        if (i != 4) {
            saves[i].next = &saves[i + 1];
        }
    }

    playerStates[0].aiNetId = 7;
    playerStates[0].lifecycleState = 2;
    playerStates[1].aiNetId = 7;
    playerStates[1].lifecycleState = 2;
    playerStates[2].aiNetId = 9;
    playerStates[2].lifecycleState = 2;
    playerStates[3].aiNetId = 7;
    playerStates[3].lifecycleState = 4;
    playerStates[4].aiNetId = 7;
    playerStates[4].lifecycleState = 2;

    g_PlayerSaveStateListHead = &saves[0];
    g_PlayerSaveStateListTail = &saves[4];
    g_PlayerSaveStateCount = 5;

    AINet::BuildAiPeerRingsByAiNetId();

    const bool firstPassOk =
        saves[0].aiPeerRingNext == &saves[4] && saves[4].aiPeerRingNext == &saves[1] &&
        saves[1].aiPeerRingNext == &saves[0] && saves[2].aiPeerRingNext == &saves[2] &&
        saves[3].aiPeerRingNext == &saves[3];

    AINet::BuildAiPeerRingsByAiNetId();

    const bool stableOk =
        saves[0].aiPeerRingNext == &saves[4] && saves[4].aiPeerRingNext == &saves[1] &&
        saves[1].aiPeerRingNext == &saves[0] && saves[2].aiPeerRingNext == &saves[2] &&
        saves[3].aiPeerRingNext == &saves[3];

    g_PlayerSaveStateListHead = oldHead;
    g_PlayerSaveStateListTail = oldTail;
    g_PlayerSaveStateCount = oldCount;
    return firstPassOk && stableOk ? 0 : 1;
}

extern "C" int player_tick_ai_mode2_path_follow_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    AINetNode currentNode = {};
    AINetNode nextNode = {};
    AINetNode autoTargetNode = {};
    AINetPathProbeFan nextFan = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};
    currentNode.nodeIndex = 1;
    nextNode.nodeIndex = 2;
    autoTargetNode.nodeIndex = 3;
    currentNode.neighborNodes[0] = &nextNode;
    nextNode.neighborNodes[0] = &autoTargetNode;
    nextNode.probeFans[0] = &nextFan;
    nextNode.position = {0.0f, 0.0f, 20.0f};
    autoTargetNode.position = {3.0f, 0.0f, 4.0f};
    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiCurrentPathNeighborIndex = 0;
    playerState.aiTopLevelState = 7;
    playerState.playerCollisionResolved = 1;
    g_Player_RuntimeDiScene = &emptyWorld;

    AINet::TickAiMode2PathFollow(&saveState);
    const bool autoTurnOk =
        playerState.aiReturnTopLevelState == 7 && playerState.aiTopLevelState == 5 &&
        playerState.autoTurnActive == 1 &&
        FloatNear(playerState.autoTurnTargetDir.x, 0.6f) &&
        FloatNear(playerState.autoTurnTargetDir.z, 0.8f) &&
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 0.0f;

    playerState = {};
    modalState = {};
    modalData = {};
    currentNode = {};
    nextNode = {};
    rootNode = {};
    emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;
    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    playerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};
    currentNode.nodeIndex = 10;
    nextNode.nodeIndex = 11;
    nextNode.position = {0.0f, 0.0f, 20.0f};
    currentNode.neighborNodes[0] = &nextNode;
    currentNode.probeFans[0] = &nextFan;
    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiCurrentPathNeighborIndex = 0;

    AINet::TickAiMode2PathFollow(&saveState);
    const bool followOk =
        playerState.aiCurrentPathNode == &currentNode &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        playerState.aiPathCursorAdvanceRequested == 1 &&
        FloatNear(playerState.throttleInput, 1.0f) &&
        FloatNear(playerState.throttleInputCopy, 1.0f) &&
        playerState.steeringInput == 0.0f &&
        playerState.steeringInputCopy == 0.0f;

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    if (!autoTurnOk) {
        return 1;
    }
    return followOk ? 0 : 2;
}

extern "C" int player_ai_mode2_forward_probe_requires_auto_turn_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const zTag4Partial oldVariantCurrent = g_Variant_CurrentTag;
    const zTag4Partial oldVariantTagCurrent = g_VariantTag_Current;

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};
    playerState.projectileSpawnVel = {3.0f, 0.0f, 4.0f};
    playerState.variantTag.count = 1;
    playerState.variantTag.tags[0] = 0x33;
    playerState.variantTag.tags[1] = 0xff;
    playerState.variantTag.tags[2] = 0xff;
    modalData.probePoints[1].y = 2.5f;
    modalData.probePoints[1].z = -6.0f;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_VariantTag_Current.count = 0;
    g_VariantTag_Current.tags[0] = 0xff;
    g_VariantTag_Current.tags[1] = 0xff;
    g_VariantTag_Current.tags[2] = 0xff;
    g_Variant_CurrentTag.count = 2;
    g_Variant_CurrentTag.tags[0] = 1;
    g_Variant_CurrentTag.tags[1] = 2;

    playerState.playerCollisionResolved = 1;
    playerState.aiMode2SteeringRetryCount = 4;
    const int earlyResult = AINet::AiMode2ForwardProbeRequiresAutoTurn(&saveState);
    const bool earlyOk = earlyResult == 1 && playerState.aiMode2SteeringRetryCount == 5;

    playerState.playerCollisionResolved = 0;
    playerState.preferredCollisionResolved = 0;
    playerState.aiMode2SteeringRetryCount = 9;
    const int clearResult = AINet::AiMode2ForwardProbeRequiresAutoTurn(&saveState);
    const bool clearOk =
        clearResult == 0 && playerState.aiMode2SteeringRetryCount == 9 &&
        PendingContactQueueCleared(playerState.preferredCollisionQueue) &&
        PendingContactQueueCleared(playerState.playerCollisionQueue) &&
        PendingContactQueueCleared(playerState.worldCollisionQueue) &&
        (rootNode.flags & 0x10) != 0 &&
        g_Variant_CurrentTag.count == g_VariantTag_Current.count &&
        g_Variant_CurrentTag.tags[0] == g_VariantTag_Current.tags[0];

    FillPendingContactQueue(&playerState.preferredCollisionQueue, 1);
    const int queuedResult = AINet::AiMode2ForwardProbeRequiresAutoTurn(&saveState);
    const bool queuedOk =
        queuedResult == 1 &&
        PendingContactQueueCleared(playerState.preferredCollisionQueue);

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_Variant_CurrentTag = oldVariantCurrent;
    g_VariantTag_Current = oldVariantTagCurrent;

    if (!earlyOk) {
        return 1;
    }
    if (!clearOk) {
        return 2;
    }
    return queuedOk ? 0 : 3;
}

extern "C" int player_ai_advance_path_cursor_and_compute_target_vec_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    saveState.playerState = &playerState;
    playerState.worldPos = {10.0f, 20.0f, 30.0f};

    auto runPositiveCase = [&](int caseIndex) {
        AINetNode previousNode = {};
        AINetNode nextNode = {};
        AINetNode branch0 = {};
        AINetNode branch1 = {};
        AINetPathProbeFan probeFans[3] = {};
        previousNode.nodeIndex = 10;
        nextNode.nodeIndex = 20;
        branch0.nodeIndex = 30;
        branch1.nodeIndex = 31;
        previousNode.neighborNodes[1] = &nextNode;
        nextNode.position = {
            1.0f + (float)caseIndex,
            2.0f + (float)caseIndex,
            3.0f + (float)caseIndex
        };
        nextNode.probeFans[0] = &probeFans[0];
        nextNode.probeFans[1] = &probeFans[1];
        nextNode.probeFans[2] = &probeFans[2];

        int expectedBranchIndex;
        switch (caseIndex) {
        case 0:
            nextNode.neighborNodes[0] = &previousNode;
            nextNode.neighborNodes[1] = &branch0;
            expectedBranchIndex = 1;
            break;
        case 1:
            nextNode.neighborNodes[0] = &branch0;
            nextNode.neighborNodes[1] = &previousNode;
            expectedBranchIndex = 0;
            break;
        case 2:
            nextNode.neighborNodes[0] = &branch0;
            nextNode.neighborNodes[2] = &previousNode;
            expectedBranchIndex = 0;
            break;
        case 3:
            nextNode.neighborNodes[0] = nullptr;
            nextNode.neighborNodes[1] = &previousNode;
            nextNode.neighborNodes[2] = &branch0;
            expectedBranchIndex = 0;
            break;
        default:
            nextNode.neighborNodes[0] = &branch0;
            nextNode.neighborNodes[1] = &branch1;
            expectedBranchIndex = 0;
            break;
        }

        AINetNode *currentNode = &previousNode;
        playerState.aiCurrentPathNode = nullptr;
        playerState.aiCurrentPathNeighborIndex = 1;
        AINetPathProbeFan *outFan = nullptr;
        zVec3 targetVec = {};

        AINet::AiAdvancePathCursorAndComputeTargetVec(
            &saveState,
            &currentNode,
            &outFan,
            &targetVec
        );

        const zVec3 expectedTarget = {
            playerState.worldPos.x - nextNode.position.x,
            playerState.worldPos.y - nextNode.position.y,
            playerState.worldPos.z - nextNode.position.z
        };
        return currentNode == &nextNode &&
               playerState.aiCurrentPathNode == &nextNode &&
               playerState.aiCurrentPathNeighborIndex == expectedBranchIndex &&
               outFan == &probeFans[expectedBranchIndex] &&
               Vec3Equals(targetVec, expectedTarget);
    };

    for (int caseIndex = 0; caseIndex < 5; ++caseIndex) {
        if (!runPositiveCase(caseIndex)) {
            return 1 + caseIndex;
        }
    }

    AINetNode *negativeNode = AllocZeroedMalloc<AINetNode>();
    AINetNode positiveNode = {};
    AINetNode positiveForward = {};
    AINetPathProbeFan positiveFan = {};
    AINet aiNet = {};
    aiNet.aiType = AINET_TYPE_HI;
    negativeNode->nodeIndex = -1;
    negativeNode->neighborNodes[0] = &positiveNode;
    positiveNode.nodeIndex = 5;
    positiveForward.nodeIndex = 6;
    positiveNode.neighborNodes[0] = &positiveForward;
    positiveNode.probeFans[0] = &positiveFan;
    positiveNode.position = {4.0f, 6.0f, 8.0f};
    playerState.aiNet = &aiNet;
    playerState.aiTopLevelState = 0;
    playerState.aiCurrentPathNeighborIndex = 0;
    AINetNode *currentNode = negativeNode;
    AINetPathProbeFan *outFan = nullptr;
    zVec3 targetVec = {};

    AINet::AiAdvancePathCursorAndComputeTargetVec(&saveState, &currentNode, &outFan,
                                                   &targetVec);
    const bool negativeOk =
        currentNode == &positiveNode && playerState.aiCurrentPathNode == &positiveNode &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        playerState.aiTopLevelState == 2 && outFan == &positiveFan &&
        Vec3Equals(targetVec, {6.0f, 14.0f, 22.0f});

    if (!negativeOk) {
        return 6;
    }

    AINetNode *negativeCurrent = AllocZeroedMalloc<AINetNode>();
    AINetNode negativeReplacement = {};
    AINetPathProbeFan negativeReplacementFan = {};
    negativeCurrent->nodeIndex = -10;
    negativeCurrent->neighborNodes[0] = &negativeReplacement;
    negativeReplacement.nodeIndex = -11;
    negativeReplacement.probeFans[0] = &negativeReplacementFan;
    negativeReplacement.position = {7.0f, 8.0f, 9.0f};
    currentNode = negativeCurrent;
    playerState.aiCurrentPathNode = negativeCurrent;
    playerState.aiCurrentPathNeighborIndex = 0;
    playerState.aiTopLevelState = 7;
    outFan = nullptr;
    targetVec = {};

    AINet::AiAdvancePathCursorAndComputeTargetVec(
        &saveState,
        &currentNode,
        &outFan,
        &targetVec
    );
    const bool negativeReplacementOk =
        currentNode == &negativeReplacement &&
        playerState.aiCurrentPathNode == &negativeReplacement &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        playerState.aiTopLevelState == 7 &&
        negativeReplacement.nodeIndex == -11 &&
        outFan == &negativeReplacementFan &&
        Vec3Equals(targetVec, {3.0f, 12.0f, 21.0f});

    return negativeReplacementOk ? 0 : 7;
}

extern "C" int player_ai_choose_next_path_branch_index_smoke(void) {
    zUtil_SaveGameState saveState = {};
    AINetNode currentNode = {};
    AINetNode branch0 = {};
    AINetNode branch1 = {};
    AINetNode branch2 = {};
    AINetNode *currentNodePtr = &currentNode;
    int outBranchIndex = 99;

    currentNode.neighborNodes[0] = &branch0;
    int result = AINet::AiChooseNextPathBranchIndex(&saveState, &currentNodePtr,
                                                     &outBranchIndex, -1);
    if (result != 1 || outBranchIndex != 0) {
        return 1;
    }

    currentNode.neighborNodes[1] = &branch1;
    outBranchIndex = 99;
    result = AINet::AiChooseNextPathBranchIndex(&saveState, &currentNodePtr,
                                                 &outBranchIndex, 0);
    if (result != 1 || outBranchIndex != 1) {
        return 2;
    }

    currentNode.neighborNodes[2] = &branch2;
    outBranchIndex = 99;
    result = AINet::AiChooseNextPathBranchIndex(&saveState, &currentNodePtr,
                                                 &outBranchIndex, -1);
    return result == 1 && outBranchIndex >= 0 && outBranchIndex < 3 ? 0 : 3;
}

extern "C" int player_tick_ai_mode2_steering_substate_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    AINet aiNet = {};
    PlayerGunFireController activeAltGunController = {};
    OptCatalogEntryDef optCatalogEntry = {};

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    targetState.playerState = &targetPlayerState;
    modalState.masterModalData = &modalData;
    playerState.aiNet = &aiNet;
    playerState.activeAltGunController = &activeAltGunController;
    activeAltGunController.optCatalogEntry = &optCatalogEntry;
    activeAltGunController.nextDispatchTime = 200.0f;
    activeAltGunController.dispatchRepeatDelay = 1.0f;
    optCatalogEntry.velocity = 10.0f;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));
    g_Player_TotalTimeSecScaled = 100.0f;

    modalData.masterType = 2;
    aiNet.pursuitParam0 = 1.0f;
    aiNet.pursuitParam1 = 10.0f;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.vehiclePitchRad = 0.1f;
    playerState.aiNextPathRebuildTime = 200.0f;
    playerState.aiCurrentSteeringSubstate = 0;
    playerState.aiRestoreTarget = {0.0f, 0.0f, 0.0f};
    playerState.aiRestoreDistanceSq = 1000.0f;
    playerState.aiTopLevelState = 5;
    playerState.aiSavedTopLevelState = 8;
    playerState.aiNotPursuitDwell = 2.0f;
    targetPlayerState.worldPos = {3.0f, 4.0f, 0.0f};
    targetPlayerState.lifecycleState = 1;

    AINet::TickAiMode2SteeringSubstate(&saveState);
    const float verticalScale = 4.0f / 3.0f;
    const float expectedPitch =
        (g_Player_AiMode2_SteeringPitchInputScale * verticalScale - 0.1f) *
        g_Player_AiMode2_SteeringPitchTurnGain;
    const bool directSubOk =
        playerState.throttleInput == 0.0f && playerState.steeringInput == 0.0f &&
        FloatNear(playerState.subPitchInput, expectedPitch) &&
        FloatNear(playerState.subPitchInputCopy, expectedPitch) &&
        FloatNear(playerState.subVerticalInput, 0.4f) &&
        FloatNear(playerState.subVerticalInputCopy, 0.4f) &&
        playerState.aiTopLevelState == 5;

    targetPlayerState.lifecycleState = 4;
    playerState.aiTopLevelState = 5;
    playerState.aiSavedTopLevelState = 8;
    AINet::TickAiMode2SteeringSubstate(&saveState);
    const bool restoreOk =
        playerState.aiTopLevelState == 8 && FloatNear(playerState.aiStateUntilTime, 102.0f);

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!directSubOk) {
        return 1;
    }
    return restoreOk ? 0 : 2;
}

extern "C" int player_update_ai_mode2_move_and_turn_toward_target_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINet aiNet = {};
    saveState.playerState = &playerState;
    playerState.aiNet = &aiNet;
    aiNet.pursuitParam0 = 5.0f;
    aiNet.pursuitParam1 = 12.0f;

    AINet::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, -0.25f, -0.1f, 20.0f);
    const bool backwardLeftOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    AINet::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, 0.0f, 0.0f, 20.0f);
    const bool zeroForwardRightOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    AINet::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, 0.5f, -0.25f, 14.0f);
    const bool farOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        FloatNear(playerState.steeringInput, -0.25f) &&
        FloatNear(playerState.steeringInputCopy, -0.25f);

    AINet::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, 0.5f, 0.75f, 3.0f);
    const bool nearOk =
        playerState.throttleInput == -1.0f && playerState.throttleInputCopy == -1.0f &&
        FloatNear(playerState.steeringInput, 0.75f) &&
        FloatNear(playerState.steeringInputCopy, 0.75f);

    AINet::UpdateAiMode2MoveAndTurnTowardTarget(&saveState, 0.5f, 0.25f, 8.0f);
    const bool bandOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        FloatNear(playerState.steeringInput, 0.25f) &&
        FloatNear(playerState.steeringInputCopy, 0.25f);

    if (!backwardLeftOk) {
        return 1;
    }
    if (!zeroForwardRightOk) {
        return 2;
    }
    if (!farOk) {
        return 3;
    }
    if (!nearOk) {
        return 4;
    }
    return bandOk ? 0 : 5;
}

extern "C" int player_tick_ai_mode2_offset_target_steering_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    AINet aiNet = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    targetState.playerState = &targetPlayerState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.aiNet = &aiNet;
    playerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};
    modalData.probePoints[1].z = -1.0f;
    aiNet.pursuitParam0 = 0.0f;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {10.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    AINet::TickAiMode2OffsetTargetSteering(&saveState, 0.0f, 0.0f, 0.0f);
    const bool offsetBranchOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.aiCurrentSteeringSubstate == 0;

    playerState.playerCollisionResolved = 1;
    playerState.aiCurrentSteeringSubstate = 2;
    playerState.aiReturnSteeringSubstate = 99;
    playerState.throttleInput = 3.0f;
    playerState.steeringInput = 4.0f;
    playerState.throttleInputCopy = 5.0f;
    playerState.steeringInputCopy = 6.0f;
    playerState.aiMode2SteeringRetryCount = 7;
    AINet::TickAiMode2OffsetTargetSteering(&saveState, 0.0f, 0.0f, 0.0f);
    const bool autoTurnBranchOk =
        playerState.throttleInput == 0.0f && playerState.steeringInput == 0.0f &&
        playerState.throttleInputCopy == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.aiReturnSteeringSubstate == 2 &&
        playerState.aiCurrentSteeringSubstate == 6 &&
        playerState.aiMode2SteeringRetryCount == 8 &&
        playerState.autoTurnActive == 1 &&
        FloatNear(playerState.autoTurnTargetDir.x, 1.0f) &&
        FloatNear(playerState.autoTurnTargetDir.y, 0.0f) &&
        FloatNear(playerState.autoTurnTargetDir.z, 0.0f);

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!offsetBranchOk) {
        return 1;
    }
    return autoTurnBranchOk ? 0 : 2;
}

extern "C" int player_tick_ai_mode2_dynamic_offset_target_steering_smoke(void) {
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    static std::int32_t matrixFlags[8];
    static float *matrixSlots[8];
    static zMat4x3 matrix;
    matrixFlags[0] = 1;
    matrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
              0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    PlayerModalState modalState = {};
    PlayerMasterModalData modalData = {};
    AINet aiNet = {};
    zClass_NodePartial rootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};
    emptyWorld.classData = &emptyWorldData;

    saveState.playerState = &playerState;
    saveState.primaryModalState = &modalState;
    targetState.playerState = &targetPlayerState;
    modalState.masterModalData = &modalData;
    playerState.rootNode = &rootNode;
    playerState.aiNet = &aiNet;
    playerState.projectileSpawnVel = {1.0f, 0.0f, 0.0f};
    playerState.aiDynamicOffsetDir = {1.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    modalData.probePoints[1].z = -1.0f;
    aiNet.pursuitParam0 = 10.0f;
    aiNet.pursuitParam1 = 2.0f;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));

    AINet::TickAiMode2DynamicOffsetTargetSteering(&saveState, 0.0f, 0.0f, 25.0f);
    const bool dynamicBranchOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.aiCurrentSteeringSubstate == 0;

    playerState.playerCollisionResolved = 1;
    playerState.aiCurrentSteeringSubstate = 2;
    playerState.aiReturnSteeringSubstate = 99;
    playerState.throttleInput = 3.0f;
    playerState.steeringInput = 4.0f;
    playerState.throttleInputCopy = 5.0f;
    playerState.steeringInputCopy = 6.0f;
    playerState.aiMode2SteeringRetryCount = 7;
    targetPlayerState.worldPos = {10.0f, 0.0f, 0.0f};
    AINet::TickAiMode2DynamicOffsetTargetSteering(&saveState, 0.0f, 0.0f, 15.0f);
    const bool autoTurnBranchOk =
        playerState.throttleInput == 0.0f && playerState.steeringInput == 0.0f &&
        playerState.throttleInputCopy == 0.0f && playerState.steeringInputCopy == 0.0f &&
        playerState.aiReturnSteeringSubstate == 2 &&
        playerState.aiCurrentSteeringSubstate == 6 &&
        playerState.aiMode2SteeringRetryCount == 8 &&
        playerState.autoTurnActive == 1 &&
        FloatNear(playerState.autoTurnTargetDir.x, 1.0f) &&
        FloatNear(playerState.autoTurnTargetDir.y, 0.0f) &&
        FloatNear(playerState.autoTurnTargetDir.z, 0.0f);

    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!dynamicBranchOk) {
        return 1;
    }
    return autoTurnBranchOk ? 0 : 2;
}

extern "C" int player_ai_try_enter_mode2_attack_pursuit_los_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    zClass_NodePartial *const oldRuntimeScene = g_Player_RuntimeDiScene;
    const int oldFinalized = g_Player_AiMode2State1Finalized;
    const float oldTotalTime = g_Player_TotalTimeSecScaled;
    const float oldAccumulatedTime = g_Time_AccumulatedTimeSec;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState buddySave = {};
    zUtil_SaveGameState localGameState = {};
    zUtil_PlayerStateStorage aiState = {};
    zUtil_PlayerStateStorage buddyState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    AINet aiNet = {};
    AINetNode currentPathNode = {};
    AINetNode restoreNode = {};
    zClass_NodePartial aiRootNode = {};
    zClass_NodePartial localRootNode = {};
    zClass_WorldDataPartial emptyWorldData = {};
    zClass_NodePartial emptyWorld = {};

    saveState.playerState = &aiState;
    saveState.aiPeerRingNext = &buddySave;
    buddySave.playerState = &buddyState;
    buddySave.aiPeerRingNext = &saveState;
    localGameState.playerState = &localPlayerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localGameState;
    emptyWorld.classData = &emptyWorldData;
    g_Player_RuntimeDiScene = &emptyWorld;
    g_Player_AiMode2State1Finalized = 0;
    g_Player_TotalTimeSecScaled = 12.0f;
    g_Time_AccumulatedTimeSec = 3.0f;

    aiState.rootNode = &aiRootNode;
    aiState.aiNet = &aiNet;
    aiState.aiCurrentPathNode = &currentPathNode;
    aiState.aiCurrentPathNeighborIndex = 0;
    aiState.aiTopLevelState = 7;
    aiState.aiStateUntilTime = 5.0f;
    aiState.aiAttackRadiusSq = 100.0f;
    aiState.fxOffsetWorld = {3.0f, 4.0f, 0.0f};
    aiState.aiMode2AttackDwell = 2.0f;
    aiState.aiMode2SteeringRetryCount = 6;
    aiState.aiTargetLineOfSightClear = 0;
    aiNet.attackBuddyNetId = 0;
    currentPathNode.neighborNodes[0] = &restoreNode;
    restoreNode.position = {8.0f, 9.0f, 10.0f};
    localPlayerState.rootNode = &localRootNode;
    localPlayerState.fxOffsetWorld = {0.0f, 0.0f, 0.0f};

    int result = AINet::AiTryEnterMode2AttackPursuitIfLineOfSight(&saveState);
    const bool successOk =
        result == 1 && aiState.aiTopLevelState == 1 && aiState.aiSavedTopLevelState == 7 &&
        aiState.aiTargetLineOfSightClear == 1 && aiState.aiMode2SteeringRetryCount == 0 &&
        Vec3Equals(aiState.aiRestoreTarget, {8.0f, 9.0f, 10.0f}) &&
        FloatNear(g_DiPickQueryPoint.x, 0.0f) && FloatNear(g_DiPickQueryPoint.y, 0.0f) &&
        FloatNear(g_DiSegmentEnd.x, 3.0f) && FloatNear(g_DiSegmentEnd.y, 5.5f);

    aiState.aiTopLevelState = 9;
    aiState.aiSavedTopLevelState = 0;
    aiState.aiTargetLineOfSightClear = 5;
    aiState.aiMode2SteeringRetryCount = 4;
    g_Player_TotalTimeSecScaled = 4.0f;
    result = AINet::AiTryEnterMode2AttackPursuitIfLineOfSight(&saveState);
    const bool timeGateOk =
        result == 0 && aiState.aiTopLevelState == 9 &&
        aiState.aiTargetLineOfSightClear == 5 && aiState.aiMode2SteeringRetryCount == 4;

    g_Player_TotalTimeSecScaled = 12.0f;
    aiState.aiAttackRadiusSq = 1.0f;
    result = AINet::AiTryEnterMode2AttackPursuitIfLineOfSight(&saveState);
    const bool rangeGateOk =
        result == 0 && aiState.aiTopLevelState == 9 &&
        aiState.aiTargetLineOfSightClear == 5;

    aiState.aiAttackRadiusSq = 100.0f;
    g_Player_AiMode2State1Finalized = 1;
    result = AINet::AiTryEnterMode2AttackPursuitIfLineOfSight(&saveState);
    const bool finalizedGateOk =
        result == 0 && aiState.aiTopLevelState == 9 &&
        aiState.aiTargetLineOfSightClear == 5;

    g_Player_TotalTimeSecScaled = oldTotalTime;
    g_Time_AccumulatedTimeSec = oldAccumulatedTime;
    g_Player_AiMode2State1Finalized = oldFinalized;
    g_Player_RuntimeDiScene = oldRuntimeScene;
    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!successOk) {
        return 1;
    }
    if (!timeGateOk) {
        return 2;
    }
    if (!rangeGateOk) {
        return 3;
    }
    return finalizedGateOk ? 0 : 4;
}

extern "C" int player_ai_alert_attack_buddies_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldFinalized = g_Player_AiMode2State1Finalized;
    const float oldTime = g_Time_AccumulatedTimeSec;
    const float oldTotalTime = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState selfSave = {};
    zUtil_SaveGameState buddySave = {};
    zUtil_SaveGameState steeringBuddySave = {};
    zUtil_SaveGameState localGameState = {};
    zUtil_PlayerStateStorage selfState = {};
    zUtil_PlayerStateStorage buddyState = {};
    zUtil_PlayerStateStorage steeringBuddyState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    AINetNode currentPathNode = {};
    AINetNode restoreNode = {};

    selfSave.playerState = &selfState;
    buddySave.playerState = &buddyState;
    steeringBuddySave.playerState = &steeringBuddyState;
    localGameState.playerState = &localPlayerState;
    selfSave.aiPeerRingNext = &buddySave;
    buddySave.aiPeerRingNext = &steeringBuddySave;
    steeringBuddySave.aiPeerRingNext = &selfSave;

    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localGameState;
    g_Player_AiMode2State1Finalized = 0;
    g_Time_AccumulatedTimeSec = 4.5f;
    g_Player_TotalTimeSecScaled = 20.0f;

    buddyState.aiTopLevelState = 7;
    buddyState.aiCurrentPathNode = &currentPathNode;
    buddyState.aiCurrentPathNeighborIndex = 0;
    buddyState.aiMode2AttackDwell = 1.5f;
    buddyState.aiCurrentSteeringSubstate = 0;
    currentPathNode.neighborNodes[0] = &restoreNode;
    restoreNode.position = {2.0f, 3.0f, 4.0f};

    steeringBuddyState.aiTopLevelState = 1;
    steeringBuddyState.recentHitFlag = 0;
    steeringBuddyState.recentHitExpireTime = 0.0f;

    AINet::AiAlertAttackBuddies(&selfSave);
    const bool alertedBuddy =
        buddyState.aiTopLevelState == 1 && buddyState.aiSavedTopLevelState == 7 &&
        buddyState.recentHitFlag == 1 && FloatNear(buddyState.recentHitExpireTime, 14.5f) &&
        Vec3Equals(buddyState.aiRestoreTarget, {2.0f, 3.0f, 4.0f});
    const bool skippedSteeringBuddy =
        steeringBuddyState.recentHitFlag == 0 &&
        FloatNear(steeringBuddyState.recentHitExpireTime, 0.0f);

    buddyState.aiTopLevelState = 9;
    buddyState.recentHitFlag = 0;
    buddyState.recentHitExpireTime = 0.0f;
    g_Player_AiMode2State1Finalized = 1;
    AINet::AiAlertAttackBuddies(&selfSave);
    const bool finalizedGateOk =
        buddyState.aiTopLevelState == 9 && buddyState.recentHitFlag == 0 &&
        FloatNear(buddyState.recentHitExpireTime, 0.0f);

    g_Player_TotalTimeSecScaled = oldTotalTime;
    g_Time_AccumulatedTimeSec = oldTime;
    g_Player_AiMode2State1Finalized = oldFinalized;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return alertedBuddy && skippedSteeringBuddy && finalizedGateOk ? 0 : 1;
}

extern "C" int player_ai_enter_mode2_steering_pursuit_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;
    const int oldFinalized = g_Player_AiMode2State1Finalized;
    const float oldTime = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState localGameState = {};
    zUtil_PlayerStateStorage aiState = {};
    zUtil_PlayerStateStorage localPlayerState = {};
    AINetNode currentPathNode = {};
    AINetNode restoreNode = {};

    saveState.playerState = &aiState;
    localGameState.playerState = &localPlayerState;
    g_GameStateOrMapTable = (zInput_GameStateOrMapTablePartial *)&localGameState;

    aiState.aiTopLevelState = 7;
    aiState.aiSavedTopLevelState = -1;
    aiState.aiCurrentPathNode = &currentPathNode;
    aiState.aiCurrentPathNeighborIndex = 1;
    aiState.aiMode2AttackDwell = 2.5f;
    aiState.aiCurrentSteeringSubstate = 2;
    aiState.worldPos = {13.0f, 7.0f, 4.0f};
    localPlayerState.worldPos = {10.0f, 99.0f, 0.0f};
    restoreNode.position = {21.0f, 22.0f, 23.0f};
    currentPathNode.neighborNodes[1] = &restoreNode;

    g_Player_AiMode2State1Finalized = 1;
    g_Player_TotalTimeSecScaled = 12.25f;
    AINet::AiEnterMode2SteeringPursuit(&saveState);
    const bool finalizedGateOk =
        aiState.aiTopLevelState == 7 && aiState.aiSavedTopLevelState == -1 &&
        aiState.aiStateStartTime == 0.0f;

    g_Player_AiMode2State1Finalized = 0;
    AINet::AiEnterMode2SteeringPursuit(&saveState);
    const bool dynamicOffsetOk =
        aiState.aiSavedTopLevelState == 7 && aiState.aiTopLevelState == 1 &&
        FloatNear(aiState.aiStateStartTime, 12.25f) &&
        FloatNear(aiState.aiStateEndTime, 14.75f) &&
        Vec3Equals(aiState.aiRestoreTarget, {21.0f, 22.0f, 23.0f}) &&
        FloatNear(aiState.aiDynamicOffsetDir.x, 0.6f) &&
        FloatNear(aiState.aiDynamicOffsetDir.y, 0.0f) &&
        FloatNear(aiState.aiDynamicOffsetDir.z, 0.8f);

    aiState.aiTopLevelState = 1;
    aiState.aiCurrentSteeringSubstate = 0;
    aiState.aiDynamicOffsetDir = {9.0f, 8.0f, 7.0f};
    AINet::AiEnterMode2SteeringPursuit(&saveState);
    const bool nonDynamicOk =
        aiState.aiSavedTopLevelState == 1 && aiState.aiTopLevelState == 1 &&
        Vec3Equals(aiState.aiDynamicOffsetDir, {9.0f, 8.0f, 7.0f});

    g_Player_TotalTimeSecScaled = oldTime;
    g_Player_AiMode2State1Finalized = oldFinalized;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    return finalizedGateOk && dynamicOffsetOk && nonDynamicOk ? 0 : 1;
}

extern "C" int player_ai_rebuild_synthetic_path_to_node_if_far_smoke(void) {
    const float oldTotalTime = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode currentPathNode = {};
    AINetNode targetNode = {};
    saveState.playerState = &playerState;

    playerState.worldPos = {3.0f, 0.0f, 4.0f};
    currentPathNode.position = {0.0f, 0.0f, 0.0f};
    playerState.aiCurrentPathNode = &currentPathNode;
    playerState.aiCurrentPathNeighborIndex = 2;
    playerState.aiNextPathRebuildTime = 99.0f;
    AINet::AiRebuildSyntheticPathToNodeIfFar(&saveState, &targetNode);
    const bool nearOk =
        playerState.aiCurrentPathNode == &currentPathNode &&
        playerState.aiCurrentPathNeighborIndex == 2 &&
        FloatNear(playerState.aiNextPathRebuildTime, 99.0f);

    playerState.worldPos = {30.0f, 0.0f, 40.0f};
    playerState.aiCurrentPathNeighborIndex = 2;
    g_Player_TotalTimeSecScaled = 7.5f;
    AINet::AiRebuildSyntheticPathToNodeIfFar(&saveState, &targetNode);
    AINetNode *const syntheticNode = playerState.aiCurrentPathNode;
    AINetPathProbeFan *const fan =
        syntheticNode != nullptr ? syntheticNode->probeFans[0] : nullptr;
    const bool farOk =
        syntheticNode != nullptr && syntheticNode != &currentPathNode &&
        syntheticNode->nodeIndex == -1 &&
        Vec3Equals(syntheticNode->position, {30.0f, 0.0f, 40.0f}) &&
        syntheticNode->neighborNodes[0] == &targetNode && fan != nullptr &&
        FloatNear(fan->pathWidth, 10.0f) &&
        playerState.aiCurrentPathNeighborIndex == 0 &&
        FloatNear(playerState.aiNextPathRebuildTime, 8.5f);

    if (syntheticNode != nullptr && syntheticNode != &currentPathNode) {
        syntheticNode->Free();
    }
    g_Player_TotalTimeSecScaled = oldTotalTime;

    if (!nearOk) {
        return 1;
    }
    return farOk ? 0 : 2;
}

extern "C" int player_update_ai_mode2_turn_toward_player_no_throttle_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.throttleInput = 9.0f;
    playerState.throttleInputCopy = 8.0f;

    targetPlayerState.worldPos = {10.0f, 0.0f, 10.0f};
    AINet::UpdateAiMode2TurnTowardPlayerNoThrottle(&saveState);
    const float diagonal = static_cast<float>(std::sqrt(0.5f));
    const bool aheadOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        FloatNear(playerState.steeringInput, -diagonal) &&
        FloatNear(playerState.steeringInputCopy, -diagonal);

    targetPlayerState.worldPos = {-10.0f, 0.0f, 10.0f};
    AINet::UpdateAiMode2TurnTowardPlayerNoThrottle(&saveState);
    const bool behindLeftOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    targetPlayerState.worldPos = {-10.0f, 0.0f, -10.0f};
    AINet::UpdateAiMode2TurnTowardPlayerNoThrottle(&saveState);
    const bool behindRightOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!aheadOk) {
        return 1;
    }
    if (!behindLeftOk) {
        return 2;
    }
    return behindRightOk ? 0 : 3;
}

extern "C" int player_update_ai_mode2_turn_in_place_toward_player_smoke(void) {
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    g_GameStateOrMapTable =
        static_cast<zInput_GameStateOrMapTablePartial *>(static_cast<void *>(&targetState));

    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.throttleInput = 9.0f;
    playerState.throttleInputCopy = 8.0f;

    targetPlayerState.worldPos = {10.0f, 0.0f, 10.0f};
    AINet::UpdateAiMode2TurnInPlaceTowardPlayer(&saveState);
    const float diagonal = static_cast<float>(std::sqrt(0.5f));
    const bool aheadOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        FloatNear(playerState.steeringInput, -diagonal) &&
        FloatNear(playerState.steeringInputCopy, -diagonal);

    targetPlayerState.worldPos = {-10.0f, 0.0f, 10.0f};
    AINet::UpdateAiMode2TurnInPlaceTowardPlayer(&saveState);
    const bool behindLeftOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    targetPlayerState.worldPos = {-10.0f, 0.0f, -10.0f};
    AINet::UpdateAiMode2TurnInPlaceTowardPlayer(&saveState);
    const bool behindRightOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    g_GameStateOrMapTable = oldGameStateOrMapTable;

    if (!aheadOk) {
        return 1;
    }
    if (!behindLeftOk) {
        return 2;
    }
    return behindRightOk ? 0 : 3;
}

extern "C" int player_update_ai_mode2_move_and_turn_toward_offset_target_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    AINet aiNet = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    playerState.aiNet = &aiNet;

    aiNet.pursuitParam0 = 0.0f;
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {10.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget(&saveState, &targetState);
    const bool directAheadOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f;

    targetPlayerState.worldPos = {0.0f, 0.0f, 10.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget(&saveState, &targetState);
    const bool sideClampOk =
        playerState.throttleInput == 0.25f && playerState.throttleInputCopy == 0.25f &&
        playerState.steeringInput == -1.0f && playerState.steeringInputCopy == -1.0f;

    playerState.worldPos = {1.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget(&saveState, &targetState);
    const bool behindOk =
        playerState.throttleInput == 0.0f && playerState.throttleInputCopy == 0.0f &&
        playerState.steeringInput == 1.0f && playerState.steeringInputCopy == 1.0f;

    aiNet.pursuitParam0 = 10.0f;
    playerState.worldPos = {10.0f, 0.0f, 0.0f};
    targetPlayerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {0.0f, 0.0f, 1.0f};
    AINet::UpdateAiMode2MoveAndTurnTowardOffsetTarget(&saveState, &targetState);

    const float expectedX = 10.0f * g_Player_AiMode2_OffsetTargetRotateCos15Deg - 10.0f;
    const float expectedZ = 10.0f * g_Player_AiMode2_OffsetTargetRotateSin15Deg;
    const float expectedLen =
        static_cast<float>(std::sqrt(expectedX * expectedX + expectedZ * expectedZ));
    const float expectedSteer = expectedX / expectedLen;
    const float expectedThrottle = 1.0f - static_cast<float>(std::fabs(expectedSteer));
    const bool rotatedOffsetOk =
        FloatNear(playerState.throttleInput, expectedThrottle) &&
        FloatNear(playerState.throttleInputCopy, expectedThrottle) &&
        FloatNear(playerState.steeringInput, expectedSteer) &&
        FloatNear(playerState.steeringInputCopy, expectedSteer);

    if (!directAheadOk) {
        return 1;
    }
    if (!sideClampOk) {
        return 2;
    }
    if (!behindOk) {
        return 3;
    }
    return rotatedOffsetOk ? 0 : 4;
}

extern "C" int player_update_ai_mode2_move_and_turn_toward_dynamic_offset_target_smoke(void) {
    zUtil_SaveGameState saveState = {};
    zUtil_SaveGameState targetState = {};
    zUtil_PlayerStateStorage playerState = {};
    zUtil_PlayerStateStorage targetPlayerState = {};
    AINet aiNet = {};
    saveState.playerState = &playerState;
    targetState.playerState = &targetPlayerState;
    playerState.aiNet = &aiNet;
    aiNet.pursuitParam0 = 10.0f;
    aiNet.pursuitParam1 = 2.0f;
    targetPlayerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.aiDynamicOffsetDir = {1.0f, 0.0f, 0.0f};

    playerState.localVel.z = 0.0f;
    AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(&saveState, &targetState, 25.0f);
    const bool forwardOk =
        playerState.throttleInput == 1.0f && playerState.throttleInputCopy == 1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f;

    playerState.localVel.z = 3.0f;
    AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(&saveState, &targetState, 15.0f);
    const float reverseTargetX = 10.0f;
    const float reverseTargetZ = 1.0f;
    const float reverseLen = static_cast<float>(
        std::sqrt(reverseTargetX * reverseTargetX + reverseTargetZ * reverseTargetZ));
    const float reverseSteer = reverseTargetZ / reverseLen;
    const float reverseThrottle = -(1.0f - static_cast<float>(std::fabs(reverseSteer)));
    const bool reverseOk =
        FloatNear(playerState.throttleInput, reverseThrottle) &&
        FloatNear(playerState.throttleInputCopy, reverseThrottle) &&
        FloatNear(playerState.steeringInput, reverseSteer) &&
        FloatNear(playerState.steeringInputCopy, reverseSteer);

    aiNet.pursuitParam0 = 1.0f;
    aiNet.pursuitParam1 = 1.0f;
    playerState.localVel.z = 0.0f;
    playerState.aiDynamicOffsetDir = {-1.0f, 0.0f, 0.0f};
    AINet::UpdateAiMode2MoveAndTurnTowardDynamicOffsetTarget(&saveState, &targetState, 3.0f);
    const bool backupOk =
        playerState.throttleInput == -1.0f && playerState.throttleInputCopy == -1.0f &&
        playerState.steeringInput == 0.0f && playerState.steeringInputCopy == 0.0f;

    if (!forwardOk) {
        return 1;
    }
    if (!reverseOk) {
        return 2;
    }
    return backupOk ? 0 : 3;
}

extern "C" int player_tick_ai_mode2_timed_path_steering_smoke(void) {
    const float oldTotalTimeSecScaled = g_Player_TotalTimeSecScaled;

    zUtil_SaveGameState saveState = {};
    zUtil_PlayerStateStorage playerState = {};
    AINetNode currentNode = {};
    AINetNode anchorNode = {};
    AINetNode forwardNode = {};
    saveState.playerState = &playerState;
    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiHomePathNode = &anchorNode;
    playerState.steerBasisNorm = {1.0f, 0.0f, 0.0f};
    playerState.worldPos = {0.0f, 0.0f, 0.0f};
    currentNode.neighborNodes[0] = &forwardNode;
    forwardNode.position = {10.0f, 0.0f, 0.0f};

    g_Player_TotalTimeSecScaled = 10.0f;
    playerState.unknown_0fa4 = 20.0f;
    playerState.throttleInput = 0.75f;
    playerState.throttleInputCopy = 0.5f;
    playerState.steeringInput = -0.25f;
    playerState.steeringInputCopy = -0.5f;
    playerState.recentHitFlag = 0;
    AINet::TickAiMode2TimedPathSteering(&saveState);
    const bool timeGateOk =
        FloatNear(playerState.throttleInput, 0.75f) &&
        FloatNear(playerState.throttleInputCopy, 0.5f) &&
        FloatNear(playerState.steeringInput, -0.25f) &&
        FloatNear(playerState.steeringInputCopy, -0.5f) &&
        playerState.recentHitFlag == 1;

    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiHomePathNode = &currentNode;
    currentNode.neighborNodes[0] = &forwardNode;
    forwardNode.position = {10.0f, 0.0f, 0.0f};
    playerState.throttleInput = 0.0f;
    playerState.throttleInputCopy = 0.0f;
    playerState.steeringInput = 0.0f;
    playerState.steeringInputCopy = 0.0f;
    playerState.recentHitFlag = 0;
    playerState.unknown_0fa4 = 5.0f;
    AINet::TickAiMode2TimedPathSteering(&saveState);
    const bool forwardBranchOk =
        FloatNear(playerState.throttleInput, 1.0f) &&
        FloatNear(playerState.throttleInputCopy, 1.0f) &&
        FloatNear(playerState.steeringInput, 0.0f) &&
        FloatNear(playerState.steeringInputCopy, 0.0f) &&
        playerState.recentHitFlag == 1;

    playerState.aiCurrentPathNode = &currentNode;
    playerState.aiHomePathNode = &anchorNode;
    currentNode.neighborNodes[0] = &anchorNode;
    currentNode.nodeIndex = 7;
    anchorNode.position = {-10.0f, 0.0f, 0.0f};
    playerState.throttleInput = 0.0f;
    playerState.throttleInputCopy = 0.0f;
    playerState.steeringInput = 0.0f;
    playerState.steeringInputCopy = 0.0f;
    playerState.recentHitFlag = 0;
    playerState.unknown_0fa4 = 5.0f;
    AINet::TickAiMode2TimedPathSteering(&saveState);
    const bool reverseBranchOk =
        FloatNear(playerState.throttleInput, -1.0f) &&
        FloatNear(playerState.throttleInputCopy, -1.0f) &&
        FloatNear(playerState.steeringInput, 0.0f) &&
        FloatNear(playerState.steeringInputCopy, 0.0f) &&
        playerState.recentHitFlag == 1;

    g_Player_TotalTimeSecScaled = oldTotalTimeSecScaled;

    if (!timeGateOk) {
        return 1;
    }
    if (!forwardBranchOk) {
        return 2;
    }
    return reverseBranchOk ? 0 : 3;
}

extern "C" int player_tick_remote_network_player_smoke(void) {
    const int oldNameTags = g_GameNetStatus_NameTags;
    const float oldDamageFeedbackScalar = g_OptCatalogDamageFeedbackIntensityScalar;
    zInput_GameStateOrMapTablePartial *const oldGameStateOrMapTable = g_GameStateOrMapTable;

    g_GameNetStatus_NameTags = 0;
    g_GameStateOrMapTable = nullptr;

    zUtil_SaveGameState transitionSaveState = {};
    zUtil_PlayerStateStorage transitionPlayerState = {};
    zClass_NodePartial transitionRootNode = {};
    zClass_Object3DDataPartial transitionRootData = {};
    GameNetPlayerRow *const transitionRow = AllocZeroedMalloc<GameNetPlayerRow>();
    transitionSaveState.playerState = &transitionPlayerState;
    transitionSaveState.netPlayerRow = transitionRow;
    transitionPlayerState.rootNode = &transitionRootNode;
    transitionPlayerState.worldPos = {10.0f, 20.0f, 30.0f};
    transitionPlayerState.fxOffsetLocal = {1.0f, 2.0f, 3.0f};
    transitionPlayerState.netReceivedPos = {40.0f, 50.0f, 60.0f};
    transitionPlayerState.netReceivedAngles = {0.25f, 0.5f, 0.75f};
    transitionPlayerState.cameraTransitionTimer = 5;
    InitObjectPositionNode(&transitionRootNode, &transitionRootData, 0.0f, 0.0f, 0.0f);

    Player::TickRemoteNetworkPlayer(&transitionSaveState);

    const bool transitionOk =
        Vec3Equals(transitionPlayerState.fxOffsetWorld, {11.0f, 22.0f, 33.0f}) &&
        Vec3Equals(transitionPlayerState.worldPos, transitionPlayerState.netReceivedPos) &&
        FloatNear(transitionPlayerState.vehiclePitchRad, 0.25f) &&
        FloatNear(transitionPlayerState.restartYawRad, 0.5f) &&
        FloatNear(transitionPlayerState.vehicleRollRad, 0.75f) &&
        FloatNear(transitionRootData.localMatrix[9], 40.0f) &&
        FloatNear(transitionRootData.localMatrix[10], 50.0f) &&
        FloatNear(transitionRootData.localMatrix[11], 60.0f);

    zUtil_SaveGameState lerpSaveState = {};
    zUtil_PlayerStateStorage lerpPlayerState = {};
    PlayerMasterCommonData lerpCommonData = {};
    zClass_NodePartial lerpRootNode = {};
    zClass_Object3DDataPartial lerpRootData = {};
    GameNetPlayerRow *const lerpRow = AllocZeroedMalloc<GameNetPlayerRow>();
    lerpSaveState.playerState = &lerpPlayerState;
    lerpSaveState.netPlayerRow = lerpRow;
    lerpPlayerState.rootNode = &lerpRootNode;
    lerpPlayerState.masterCommonData = &lerpCommonData;
    lerpCommonData.invMaxHealth = 0.01f;
    lerpPlayerState.worldPos = {0.0f, 10.0f, 20.0f};
    lerpPlayerState.fxOffsetLocal = {2.0f, 3.0f, 4.0f};
    lerpPlayerState.netReceivedPos = {10.0f, 20.0f, 40.0f};
    lerpPlayerState.netReceivedAngles = {1.0f, 1.5f, 2.0f};
    lerpPlayerState.cameraTransitionTimer = 0;
    lerpPlayerState.lifecycleState = 5;
    lerpPlayerState.statusMeterValue = 25.0f;
    g_OptCatalogDamageFeedbackIntensityScalar = 0.0f;
    InitObjectPositionNode(&lerpRootNode, &lerpRootData, 0.0f, 0.0f, 0.0f);

    Player::TickRemoteNetworkPlayer(&lerpSaveState);

    const bool lerpOk =
        Vec3Equals(lerpPlayerState.fxOffsetWorld, {2.0f, 13.0f, 24.0f}) &&
        FloatNear(lerpPlayerState.worldPos.x, 3.5f) &&
        FloatNear(lerpPlayerState.worldPos.y, 13.5f) &&
        FloatNear(lerpPlayerState.worldPos.z, 27.0f) &&
        FloatNear(lerpPlayerState.vehiclePitchRad, 1.0f) &&
        FloatNear(lerpPlayerState.restartYawRad, 1.5f) &&
        FloatNear(lerpPlayerState.vehicleRollRad, 2.0f) &&
        lerpPlayerState.cameraTransitionTimer == 0 &&
        FloatNear(g_OptCatalogDamageFeedbackIntensityScalar, 0.25f) &&
        FloatNear(lerpRootData.localMatrix[9], 3.5f) &&
        FloatNear(lerpRootData.localMatrix[10], 13.5f) &&
        FloatNear(lerpRootData.localMatrix[11], 27.0f);

    g_GameNetStatus_NameTags = oldNameTags;
    g_OptCatalogDamageFeedbackIntensityScalar = oldDamageFeedbackScalar;
    g_GameStateOrMapTable = oldGameStateOrMapTable;
    std::free(transitionRow);
    std::free(lerpRow);
    if (!transitionOk) {
        return 1;
    }
    return lerpOk ? 0 : 2;
}
