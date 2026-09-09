#include "Battlesport/ai_net.h"

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-ainet-path-probe-half-width-scale
 * @recoil-artifact defines .rdata recoil:data:0x4cc850: Read-only half-width scale used by the path-probe travel clamp.
 * Retail data evidence: the path-probe travel clamp reads this 0.5 scalar when
 * selecting the minimum half-width travel.
 * Purpose: Stores the read-only half-width scale used for minimum probe-fan travel.
 */
extern const float g_AINetPathProbeHalfWidthScale = 0.5f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-ainetlisthead
 * @recoil-artifact defines .data recoil:data:0x4e5c58: g_AINetListHead.
 * Purpose: Stores the zero-initialized head pointer for the loaded AINet global list.
 */
AINet *g_AINetListHead = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-ainetlisttail
 * @recoil-artifact defines .data recoil:data:0x4e5c5c: g_AINetListTail.
 * Purpose: Stores the zero-initialized tail pointer maintained with the loaded AINet global list.
 */
AINet *g_AINetListTail = 0;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-pathfollowpitchinputscale
 * @recoil-artifact defines .data recoil:data:0x4da0c0: g_Player_AiMode2_PathFollowPitchInputScale.
 * BN types this as an initialized .data float used by the Mode2 AI
 * path-follow pitch steering input.
 * Purpose: Scales path-follow vertical steering error into pitch input.
 */
float g_Player_AiMode2_PathFollowPitchInputScale = 0.0174499992f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-pathfollowpitchturngain
 * @recoil-artifact defines .data recoil:data:0x4da0c4: g_Player_AiMode2_PathFollowPitchTurnGain.
 * BN types this as an initialized .data float paired with the Mode2 AI
 * path-follow pitch input scale.
 * Purpose: Scales path-follow pitch input into turn correction.
 */
float g_Player_AiMode2_PathFollowPitchTurnGain = 5.69999981f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-steeringpitchinputscale
 * @recoil-artifact defines .data recoil:data:0x4da0c8: g_Player_AiMode2_SteeringPitchInputScale.
 * BN types this as an initialized .data float used by the Mode2 AI steering
 * substates.
 * Purpose: Scales steering vertical distance into pitch input.
 */
float g_Player_AiMode2_SteeringPitchInputScale = 0.800000012f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-steeringpitchturngain
 * @recoil-artifact defines .data recoil:data:0x4da0cc: g_Player_AiMode2_SteeringPitchTurnGain.
 * BN types this as an initialized .data float paired with the Mode2 AI
 * steering pitch input scale.
 * Purpose: Scales steering pitch input into turn correction.
 */
float g_Player_AiMode2_SteeringPitchTurnGain = 5.69999981f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-steeringverticalerrorscale
 * @recoil-artifact defines .data recoil:data:0x4da0d0: g_Player_AiMode2_SteeringVerticalErrorScale.
 * BN types this as an initialized .data float read by the Mode2 AI steering
 * substates.
 * Purpose: Scales steering vertical error before pitch correction.
 */
float g_Player_AiMode2_SteeringVerticalErrorScale = 0.100000001f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-tuningscalar55a
 * @recoil-artifact defines .data recoil:data:0x4da0d4: g_Player_AiMode2_TuningScalar55A.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the first Mode2 AI 55.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar55A = 55.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-tuningscalar55b
 * @recoil-artifact defines .data recoil:data:0x4da0d8: g_Player_AiMode2_TuningScalar55B.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the second Mode2 AI 55.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar55B = 55.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-tuningscalar5
 * @recoil-artifact defines .data recoil:data:0x4da0dc: g_Player_AiMode2_TuningScalar5.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the Mode2 AI 5.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar5 = 5.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-tuningscalar10
 * @recoil-artifact defines .data recoil:data:0x4da0e0: g_Player_AiMode2_TuningScalar10.
 * BN types this as an initialized .data float in the contiguous Mode2 AI
 * tuning scalar range.
 * Purpose: Stores the Mode2 AI 10.0 tuning scalar.
 */
float g_Player_AiMode2_TuningScalar10 = 10.0f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-offsettargetrotatecos15deg
 * @recoil-artifact defines .data recoil:data:0x4da0e4: g_Player_AiMode2_OffsetTargetRotateCos15Deg.
 * BN types this as an initialized .data float used by the Mode2 AI offset
 * target steering rotation.
 * Inferred declaration qualifier: consistent volatile float declarations
 * reproduce both retail coefficient reads under canonical VC5SP3. The
 * original cv-qualification and its purpose remain unresolved; no external
 * writer or synchronization contract is inferred. Pro review:
 * 2026-09-08T16-25-03-957Z. Qualifier controls affect only this scalar's reads.
 * Purpose: Stores the retail cosine scalar for offset-target rotation.
 */
volatile float g_Player_AiMode2_OffsetTargetRotateCos15Deg = 0.965900004f;
/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.g-player-aimode2-offsettargetrotatesin15deg
 * @recoil-artifact defines .data recoil:data:0x4da0e8: g_Player_AiMode2_OffsetTargetRotateSin15Deg.
 * BN types this as an initialized .data float used by the Mode2 AI offset
 * target steering rotation.
 * Inferred declaration qualifier: consistent volatile float declarations
 * reproduce both retail coefficient reads under canonical VC5SP3. The
 * original cv-qualification and its purpose remain unresolved; no external
 * writer or synchronization contract is inferred. Pro review:
 * 2026-09-08T16-25-03-957Z. Qualifier controls affect only this scalar's reads.
 * Purpose: Stores the retail sine scalar for offset-target rotation.
 */
volatile float g_Player_AiMode2_OffsetTargetRotateSin15Deg = 0.25879999995f;

}

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth_decls.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BATTLESPORT_AI_NET_EMIT_HEADER_BODIES
#include "Battlesport/ai_net.h"
#undef BATTLESPORT_AI_NET_EMIT_HEADER_BODIES

#include "GameZRecoil/zMath/zmth.h"
/**
 * Purpose: Loads every numbered AI path network definition from the mission
 * ZRD set. Its retail source relationship remains unresolved.
 */
void AINet::LoadAllFromZrd() {
    for (int netId = 1; netId < 100; ++netId) {
        AINet::LoadFromZrd(netId);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-alloc
 * @recoil-artifact defines .text recoil:function:0x402ff0: AINet::Alloc (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Allocates a zeroed AI network record and appends it to the global AI network list.
 */
AINet *AINet::Alloc() {
    AINet *const aiNet = (AINet *)(malloc(sizeof(AINet)));
    memset(aiNet, 0, sizeof(AINet));

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
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x403040: AINet::LoadFromZrd (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Parses one AI path network ZRD, builds its node list, resolves links, and returns the loaded network.
 */
AINet *__fastcall AINet::LoadFromZrd(
    int netId
) {
    char baseName[8];
    char nodeName[8];
    sprintf(baseName, "net_%02d", netId);

    char path[0x104];
    sprintf(path, "%s.zrd", baseName);

    zReader::Node *const root = zReader::Load(path, 0, 0);
    if (root == 0) {
        return 0;
    }

    zReader::Node *versionNode = zRdrGetNode(root, "version");
    if (versionNode != 0 && versionNode->value.nodes[1].value.i32 != 105) {
        zError::ReportOld(
            0x200,
            "D:\\Proj\\Battlesport\\ai_net.cpp",
            0x8c,
            "Wrong ai_paths.zrd version number!"
        );
        return 0;
    }

    AINet *const aiNet = AINet::Alloc();
    aiNet->netId = netId;

    zReader::Node *nameNode = zRdrGetNode(root, "name");
    if (nameNode != 0) {
        strcpy(aiNet->name, nameNode->value.nodes[1].value.str);
    } else {
        strcpy(aiNet->name, baseName);
    }

    char token[0x18];
    zReader::Node *typeNode = zRdrGetNode(root, "type");
    if (typeNode != 0) {
        strcpy(token, typeNode->value.nodes[1].value.str);
        _strupr(token);

        if (strncmp(token, "ST", 2) == 0) {
            aiNet->aiType = AINET_TYPE_ST;
        } else if (strncmp(token, "HI", 2) == 0) {
            aiNet->aiType = AINET_TYPE_HI;
        } else if (strncmp(token, "FI", 2) == 0) {
            aiNet->aiType = AINET_TYPE_FI;
        } else if (strncmp(token, "DE", 2) == 0) {
            aiNet->aiType = AINET_TYPE_DE;
        }
    } else {
        aiNet->aiType = AINET_TYPE_ST;
    }

    zReader::Node *pathWidthNode = zRdrGetNode(root, "path_width");
    if (pathWidthNode != 0) {
        aiNet->pathWidth = pathWidthNode->value.nodes[1].value.f32;
    } else {
        aiNet->pathWidth = 10.0f;
    }

    zReader::Node *activateRadiusNode = zRdrGetNode(root, "activate_rad");
    if (activateRadiusNode != 0) {
        aiNet->activateRadius = activateRadiusNode->value.nodes[1].value.f32;
    }

    zReader::Node *attackRadiusNode = zRdrGetNode(root, "attack_rad");
    if (attackRadiusNode != 0) {
        aiNet->attackRadius = attackRadiusNode->value.nodes[1].value.f32;
    }

    zReader::Node *attackDwellNode = zRdrGetNode(root, "attack_dwell");
    if (attackDwellNode != 0) {
        aiNet->attackDwell = attackDwellNode->value.nodes[1].value.f32;
    }

    zReader::Node *pursuitNode = zRdrGetNode(root, "pursuit_params");
    if (pursuitNode == 0) {
        pursuitNode = zRdrGetNode(root, "pursuit_range");
    }
    if (pursuitNode != 0) {
        aiNet->pursuitParam0 = pursuitNode->value.nodes[1].value.f32;
        aiNet->pursuitParam1 = pursuitNode->value.nodes[2].value.f32;
    }

    zReader::Node *notPursuitDwellNode = zRdrGetNode(root, "not_pursuit_dwell");
    if (notPursuitDwellNode != 0) {
        aiNet->notPursuitDwell = notPursuitDwellNode->value.nodes[1].value.f32;
    }

    zReader::Node *returnRangeNode = zRdrGetNode(root, "return_range");
    if (returnRangeNode != 0) {
        aiNet->returnRange = returnRangeNode->value.nodes[1].value.f32;
    }

    zReader::Node *hideTimesNode = zRdrGetNode(root, "hide_times");
    if (hideTimesNode != 0) {
        aiNet->hideTime0 = hideTimesNode->value.nodes[1].value.f32;
        aiNet->hideTime1 = hideTimesNode->value.nodes[2].value.f32;
    } else {
        aiNet->hideTime0 = 8.0f;
        aiNet->hideTime1 = 4.0f;
    }

    zReader::Node *attackBuddyNode = zRdrGetNode(root, "attack_buddy");
    if (attackBuddyNode != 0) {
        aiNet->attackBuddyNetId = attackBuddyNode->value.nodes[1].value.i32;
    } else {
        aiNet->attackBuddyNetId = 0;
    }

    zReader::Node *activateBuddyNode = zRdrGetNode(root, "activate_buddy");
    if (activateBuddyNode != 0) {
        aiNet->activateBuddyNetId = activateBuddyNode->value.nodes[1].value.i32;
    } else {
        aiNet->attackBuddyNetId = 0;
    }

    zReader::Node *attackStrategyNode = zRdrGetNode(root, "attack_strategy");
    if (attackStrategyNode != 0) {
        strcpy(token, attackStrategyNode->value.nodes[1].value.str);
        _strupr(token);
        if (strncmp(token, "FOL", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_FOL;
        } else if (strncmp(token, "CIR", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_CIR;
        } else if (strncmp(token, "HEA", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_HEA;
        } else if (strncmp(token, "BAC", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_BAC;
        } else if (strncmp(token, "ZIG", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_ZIG;
        } else if (strncmp(token, "SIT", 3) == 0) {
            aiNet->attackStrategy = AINET_STRAT_SIT;
        }
    } else {
        aiNet->attackStrategy = AINET_STRAT_HEA;
    }

    AINetNode *tail = 0;
    for (int nodeIndex = 0; nodeIndex < 99; ++nodeIndex) {
        sprintf(nodeName, "node_%02d", nodeIndex);

        zReader::Node *node = zRdrGetNode(root, nodeName);
        if (node == 0) {
            continue;
        }

        AINetNode *const aiNode = (AINetNode *)(malloc(sizeof(AINetNode)));
        memset(aiNode, 0, sizeof(AINetNode));

        if (tail == 0) {
            aiNet->nodeListHead = aiNode;
        } else {
            tail->next = aiNode;
        }
        tail = aiNode;

        aiNode->nodeIndex = nodeIndex;
        aiNode->costOrType = node->value.nodes[1].value.i32;
        aiNode->position.x = node->value.nodes[2].value.nodes[1].value.f32;
        aiNode->position.y = node->value.nodes[2].value.nodes[2].value.f32;
        aiNode->position.z = node->value.nodes[2].value.nodes[3].value.f32;
        aiNode->neighborIndices[0] = node->value.nodes[3].value.nodes[1].value.i32;
        aiNode->neighborIndices[1] = node->value.nodes[3].value.nodes[2].value.i32;
        aiNode->neighborIndices[2] = node->value.nodes[3].value.nodes[3].value.i32;
    }

    AINet::ResolveNeighborLinksAndBuildProbeFans(aiNet->nodeListHead, aiNet->pathWidth);
    zReader::Free(root);
    return aiNet;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-findbynetid
 * @recoil-artifact defines .text recoil:function:0x403510: AINet::FindByNetId (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
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
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-findnodebyindex
 * @recoil-artifact defines .text recoil:function:0x403530: AINet::FindNodeByIndex (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
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
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-resolveneighborlinksandbuildprobefans
 * @recoil-artifact defines .text recoil:function:0x403550: AINet::ResolveNeighborLinksAndBuildProbeFans (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Resolves neighbor indices into node pointers and allocates probe fans for valid AI path links.
 */
void __fastcall AINet::ResolveNeighborLinksAndBuildProbeFans(
    AINetNode *nodeListHead,
    float pathWidth
) {
    AINetNode *node = nodeListHead;
    while (node != 0) {
        const zVec3 fromPosition = node->position;
        int *neighborIndexPtr = node->neighborIndices;
        for (int slotCount = 3; slotCount != 0; --slotCount) {
            const int neighborIndex = *neighborIndexPtr;
            if (neighborIndex >= 0) {
                *(AINetNode **)neighborIndexPtr = AINet::FindNodeByIndex(
                    neighborIndex,
                    nodeListHead
                );

                *(AINetPathProbeFan **)(neighborIndexPtr + 3) =
                    (AINetPathProbeFan *)(malloc(sizeof(AINetPathProbeFan)));
                memset(*(AINetPathProbeFan **)(neighborIndexPtr + 3), 0, sizeof(AINetPathProbeFan));
                (*(AINetPathProbeFan **)(neighborIndexPtr + 3))->InitFromSegment(
                    fromPosition,
                    (*(AINetNode **)neighborIndexPtr)->position,
                    pathWidth
                );
            } else {
                *(AINetNode **)neighborIndexPtr = 0;
                *(AINetPathProbeFan **)(neighborIndexPtr + 3) = 0;
            }
            ++neighborIndexPtr;
        }

        node = node->next;
    }
}

/**
 * @recoil-raw-asm recoil:raw-asm:battlesport.ai-net.ainet-path-probe-fan.init-from-segment.path-width-store
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.ainet-path-probe-fan.init-from-segment.path-width-store recoil:function:0x403620
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.vector-subtract recoil:function:0x403620
 * @recoil-raw-consumer recoil:raw-asm:battlesport.ai-net.path-probe-clamp-travel-vc5 recoil:function:0x403620
 * Original function evidence: retail 0x403620 contains these three approved local and
 * header-expanded raw-assembly regions.
 * Raw-assembly evidence: VC5 C++ variants did not preserve the retail
 * fixed-register path-width store, shared vector subtraction, or x87 clamp
 * ordering; each approved island remains scoped to this exact consumer.
 * Purpose: Builds the normalized path-probe fan basis and travel clamp for one AI navigation segment.
 */
void AINetPathProbeFan::InitFromSegment(
    zVec3 fromPosition,
    zVec3 toPosition,
    float pathWidth
) {
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    zVec3 *v2;
#else
    zVec3 *v0;
    zVec3 *v1;
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA_KEEP_PTR(delta, toPosition, fromPosition, v2);
#else
    AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA(delta, toPosition, fromPosition);
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    float xzLength;
    AINET_PATH_PROBE_CLAMP_TRAVEL_VC5(v2, xzLength, pathWidth);
#else
    float xzLength;
    xzLength = sqrt(delta.x * delta.x + delta.z * delta.z);
    clampedTravel =
        (xzLength - pathWidth > pathWidth * g_AINetPathProbeHalfWidthScale)
            ? ((xzLength = sqrt(delta.x * delta.x + delta.z * delta.z)) - pathWidth)
            : (pathWidth * g_AINetPathProbeHalfWidthScale);
#endif

    zMath::Vec3NormalizeXZ(&delta, &delta);
    zVec3 *const perpendicularPtr = &perpendicular;
    zMath::Vec3PerpXZ(&delta, perpendicularPtr);
    zMath::Vec3RotateY(45.0f, &probeDirPlus45, perpendicularPtr);
    zMath::Vec3RotateY(-45.0f, &probeDirMinus45, perpendicularPtr);
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    /**
     * AINetPathProbeFan::InitFromSegment path-width store.
     * Raw assembly: preserves the VC5 register/store shape for the final
     * pathWidth assignment after C++ assignment variants kept rotating the
     * remaining byte diff in this function.
     * Purpose: Stores the probe fan path width in the retail register order.
     */
    __asm mov edx, pathWidth
    __asm mov dword ptr [esi+34h], edx
#else
    this->pathWidth = pathWidth;
#endif
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-findnearestnode
 * @recoil-artifact defines .text recoil:function:0x4036f0: AINet::FindNearestNode (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
 * Purpose: Scans an AI node list and returns the node nearest to the query position.
 */
AINetNode *__fastcall AINet::FindNearestNode(
    const zVec3 *position,
    AINetNode *nodeListHead
) {
    /**
     * @recoil-anchor recoil:anchor:battlesport.ai-net.nearest-node-minimum-distance-sq
     * @recoil-artifact defines .rdata recoil:data:0x4cc858: Nearest-node zero comparison scalar.
     * Purpose: Supplies the separate read-only zero boundary used to recognize
     * the negative distance sentinel. Retail 0x40371e reads the four-byte
     * positive-zero range at 0x4cc858, distinct from the header-body zero.
     * This auxiliary constant represents that storage dependency; the original
     * identifier, lexical scope, and containing-object extent remain unresolved.
     */
    static const float minimumDistanceSq = 0.0f;
    AINetNode *nearest = 0;
    float bestDistanceSq = -1.0f;

    while (nodeListHead != 0) {
        const float distanceSq = zMath::Vec3DeltaLengthSq(position, &nodeListHead->position);
        if (distanceSq < bestDistanceSq || bestDistanceSq < minimumDistanceSq) {
            bestDistanceSq = distanceSq;
            nearest = nodeListHead;
        }
        nodeListHead = nodeListHead->next;
    }

    return nearest;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-buildaipeerringsbyainetid
 * @recoil-artifact defines .text recoil:function:0x403750: AINet::BuildAiPeerRingsByAiNetId.
 * @recoil-match byte
 *
 * Source placement audit: BN file-literal order makes accepted player.cpp
 * ownership invalid; this body remains here only until the AINet remap can
 * pull the required save-state declarations with it.
 * Purpose: link active save states sharing one AI network id into peer rings
 * used by AI bootstrap while leaving inactive/unlinked records untouched.
 * Source owner: pending battlesport_ai.ainet_peer_ring_build audit.
 */
void AINet::BuildAiPeerRingsByAiNetId() {
    zUtil_SaveGameState *saveState = g_PlayerSaveStateList.head;
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
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainetnode-free
 * @recoil-artifact defines .text recoil:function:0x4037c0: AINetNode::Free (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
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
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-free
 * @recoil-artifact defines .text recoil:function:0x403800: AINet::Free (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
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
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-aidiscardnegativebranchpathnodes
 * @recoil-artifact defines .text recoil:function:0x403830: AINet::AiDiscardNegativeBranchPathNodes
 * @recoil-match byte
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
 * @recoil-anchor recoil:anchor:battlesport.ai-net.ainet-freeall
 * @recoil-artifact defines .text recoil:function:0x403870: AINet::FreeAll (Battlesport/ai_net.cpp).
 * @recoil-match byte
 *
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
