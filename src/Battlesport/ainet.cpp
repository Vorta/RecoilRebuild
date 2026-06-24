#include "Battlesport/ainet.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zReader/zReader.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
