#include "Battlesport/ai_net.h"

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

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/time.h"
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
    char baseName[8];
    char nodeName[8];
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
    if (nameNode != 0) {
        strcpy(
            aiNet->name,
            nameNode->value.nodes[1].value.str
        );
    } else {
        strcpy(
            aiNet->name,
            baseName
        );
    }

    char token[0x18];
    zReader::Node *typeNode = zReader_GetNamedNode(root, g_AINet_TypeFieldName);
    if (typeNode != 0) {
        strcpy(
            token,
            typeNode->value.nodes[1].value.str
        );
        _strupr(token);

        if (strncmp(
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
    } else {
        aiNet->aiType = AINET_TYPE_ST;
    }

    zReader::Node *pathWidthNode = zReader_GetNamedNode(
        root,
        g_AINet_PathWidthFieldName
    );
    if (pathWidthNode != 0) {
        aiNet->pathWidth = pathWidthNode->value.nodes[1].value.f32;
    } else {
        aiNet->pathWidth = 10.0f;
    }

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
        aiNet->pursuitParam0 = pursuitNode->value.nodes[1].value.f32;
        aiNet->pursuitParam1 = pursuitNode->value.nodes[2].value.f32;
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
        aiNet->hideTime0 = hideTimesNode->value.nodes[1].value.f32;
        aiNet->hideTime1 = hideTimesNode->value.nodes[2].value.f32;
    } else {
        aiNet->hideTime0 = 8.0f;
        aiNet->hideTime1 = 4.0f;
    }

    zReader::Node *attackBuddyNode = zReader_GetNamedNode(
        root,
        g_AINet_AttackBuddyFieldName
    );
    if (attackBuddyNode != 0) {
        aiNet->attackBuddyNetId = attackBuddyNode->value.nodes[1].value.i32;
    } else {
        aiNet->attackBuddyNetId = 0;
    }

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
                memset(
                    *(AINetPathProbeFan **)(neighborIndexPtr + 3),
                    0,
                    sizeof(AINetPathProbeFan)
                );
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
 * Reimplements 0x403620: AINetPathProbeFan::InitFromSegment (Battlesport/ai_net.cpp).
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
    AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA_KEEP_PTR(
        delta,
        toPosition,
        fromPosition,
        v2
    );
#else
    AINET_PROBE_FAN_COMPUTE_SEGMENT_DELTA(
        delta,
        toPosition,
        fromPosition
    );
#endif

#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    float xzLength;
    AINET_PATH_PROBE_CLAMP_TRAVEL_VC5(
        v2,
        xzLength,
        pathWidth
    );
#else
    float xzLength;
    xzLength = sqrt(delta.x * delta.x + delta.z * delta.z);
    clampedTravel =
        (xzLength - pathWidth > pathWidth * g_AINetPathProbeHalfWidthScale)
            ? ((xzLength = sqrt(delta.x * delta.x + delta.z * delta.z)) - pathWidth)
            : (pathWidth * g_AINetPathProbeHalfWidthScale);
#endif

    zMath::Vec3NormalizeXZ(
        &delta,
        &delta
    );
    zVec3 *const perpendicularPtr = &perpendicular;
    zMath::Vec3PerpXZ(
        &delta,
        perpendicularPtr
    );
    zMath::Vec3RotateY(
        &probeDirPlus45,
        perpendicularPtr,
        45.0f
    );
    zMath::Vec3RotateY(
        &probeDirMinus45,
        perpendicularPtr,
        -45.0f
    );
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    /**
     * Reimplements 0x403620: AINetPathProbeFan::InitFromSegment path-width store.
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
 * Reimplements 0x4036f0: AINet::FindNearestNode (Battlesport/ai_net.cpp).
 * Purpose: Scans an AI node list and returns the node nearest to the query position.
 */
AINetNode *__fastcall AINet::FindNearestNode(
    const zVec3 *position,
    AINetNode *nodeListHead
) {
    AINetNode *nearest = 0;
    float bestDistanceSq = -1.0f;

    while (nodeListHead != 0) {
        const float distanceSq = zMath::Vec3DeltaLengthSq(
            position,
            &nodeListHead->position
        );
        if (distanceSq < bestDistanceSq || bestDistanceSq < 0.0f) {
            bestDistanceSq = distanceSq;
            nearest = nodeListHead;
        }
        nodeListHead = nodeListHead->next;
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
 * Reimplements 0x4037c0: AINetNode::Free (Battlesport/ai_net.cpp).
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
 * Reimplements 0x403800: AINet::Free (Battlesport/ai_net.cpp).
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
 * (Battlesport/ai_net.cpp).
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
 * Reimplements 0x403870: AINet::FreeAll (Battlesport/ai_net.cpp).
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
