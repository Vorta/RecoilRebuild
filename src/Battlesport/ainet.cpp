#include "Battlesport/ainet.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zReader/zReader.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
AINet *g_AINetListHead = 0;
AINet *g_AINetListTail = 0;
}

namespace {
const char kAINetSourceFile[] = "D:\\Proj\\Battlesport\\ai_net.cpp";

zReader::Node *AINetZrdPayload(zReader::Node *node) {
    return node->value.nodes;
}

int AINetZrdFirstInt(zReader::Node *node) {
    return AINetZrdPayload(node)[1].value.i32;
}

float AINetZrdFirstFloat(zReader::Node *node) {
    return AINetZrdPayload(node)[1].value.f32;
}

const char *AINetZrdFirstString(zReader::Node *node) {
    return AINetZrdPayload(node)[1].value.str;
}

void AINetCopyAndUpper(char *buffer, const char *text) {
    strcpy(buffer, text);
    _strupr(buffer);
}
} // namespace

// Reimplements 0x402fd0: AINet::LoadAllFromZrd (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE void RECOIL_CDECL AINet::LoadAllFromZrd() {
    for (int netId = 1; netId < 100; ++netId) {
        AINet::LoadFromZrd(netId);
    }
}

// Reimplements 0x403040: AINet::LoadFromZrd (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE AINet *RECOIL_FASTCALL AINet::LoadFromZrd(int netId) {
    char baseName[0x10];
    sprintf(baseName, "net_%02d", netId);

    char path[0x104];
    sprintf(path, "%s.zrd", baseName);

    zReader::Node *const root = zReader::LoadNodeFromPath(path, 0, 0);
    if (root == 0) {
        return 0;
    }

    zReader::Node *versionNode = zReader_GetNamedNode(root, "version");
    if (versionNode != 0 && AINetZrdFirstInt(versionNode) != 105) {
        zError::ReportOld(0x200, kAINetSourceFile, 0x8c,
                          "Wrong ai_paths.zrd version number!");
        return 0;
    }

    AINet *const aiNet = AINet::Alloc();
    aiNet->netId = netId;

    zReader::Node *nameNode = zReader_GetNamedNode(root, "name");
    strcpy(aiNet->name, nameNode != 0 ? AINetZrdFirstString(nameNode) : baseName);

    char token[0x18];
    zReader::Node *typeNode = zReader_GetNamedNode(root, "type");
    if (typeNode != 0) {
        AINetCopyAndUpper(token, AINetZrdFirstString(typeNode));
    }

    if (typeNode == 0 || strncmp(token, "ST", 2) == 0) {
        aiNet->aiType = AINET_TYPE_ST;
    } else if (strncmp(token, "HI", 2) == 0) {
        aiNet->aiType = AINET_TYPE_HI;
    } else if (strncmp(token, "FI", 2) == 0) {
        aiNet->aiType = AINET_TYPE_FI;
    } else if (strncmp(token, "DE", 2) == 0) {
        aiNet->aiType = AINET_TYPE_DE;
    }

    zReader::Node *pathWidthNode = zReader_GetNamedNode(root, "path_width");
    aiNet->pathWidth = pathWidthNode != 0 ? AINetZrdFirstFloat(pathWidthNode) : 10.0f;

    zReader::Node *activateRadiusNode = zReader_GetNamedNode(root, "activate_rad");
    if (activateRadiusNode != 0) {
        aiNet->activateRadius = AINetZrdFirstFloat(activateRadiusNode);
    }

    zReader::Node *attackRadiusNode = zReader_GetNamedNode(root, "attack_rad");
    if (attackRadiusNode != 0) {
        aiNet->attackRadius = AINetZrdFirstFloat(attackRadiusNode);
    }

    zReader::Node *attackDwellNode = zReader_GetNamedNode(root, "attack_dwell");
    if (attackDwellNode != 0) {
        aiNet->attackDwell = AINetZrdFirstFloat(attackDwellNode);
    }

    zReader::Node *pursuitNode = zReader_GetNamedNode(root, "pursuit_params");
    if (pursuitNode == 0) {
        pursuitNode = zReader_GetNamedNode(root, "pursuit_range");
    }
    if (pursuitNode != 0) {
        zReader::Node *const payload = AINetZrdPayload(pursuitNode);
        aiNet->pursuitParam0 = payload[1].value.f32;
        aiNet->pursuitParam1 = payload[2].value.f32;
    }

    zReader::Node *notPursuitDwellNode = zReader_GetNamedNode(root, "not_pursuit_dwell");
    if (notPursuitDwellNode != 0) {
        aiNet->notPursuitDwell = AINetZrdFirstFloat(notPursuitDwellNode);
    }

    zReader::Node *returnRangeNode = zReader_GetNamedNode(root, "return_range");
    if (returnRangeNode != 0) {
        aiNet->returnRange = AINetZrdFirstFloat(returnRangeNode);
    }

    zReader::Node *hideTimesNode = zReader_GetNamedNode(root, "hide_times");
    if (hideTimesNode != 0) {
        zReader::Node *const payload = AINetZrdPayload(hideTimesNode);
        aiNet->hideTime0 = payload[1].value.f32;
        aiNet->hideTime1 = payload[2].value.f32;
    } else {
        aiNet->hideTime0 = 8.0f;
        aiNet->hideTime1 = 4.0f;
    }

    zReader::Node *attackBuddyNode = zReader_GetNamedNode(root, "attack_buddy");
    aiNet->attackBuddyNetId = attackBuddyNode != 0 ? AINetZrdFirstInt(attackBuddyNode) : 0;

    zReader::Node *activateBuddyNode = zReader_GetNamedNode(root, "activate_buddy");
    if (activateBuddyNode != 0) {
        aiNet->activateBuddyNetId = AINetZrdFirstInt(activateBuddyNode);
    } else {
        aiNet->attackBuddyNetId = 0;
    }

    zReader::Node *attackStrategyNode = zReader_GetNamedNode(root, "attack_strategy");
    if (attackStrategyNode != 0) {
        AINetCopyAndUpper(token, AINetZrdFirstString(attackStrategyNode));
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
        char nodeName[0x10];
        sprintf(nodeName, "node_%02d", nodeIndex);

        zReader::Node *node = zReader_GetNamedNode(root, nodeName);
        if (node == 0) {
            continue;
        }

        AINetNode *const aiNode = (AINetNode *)(malloc(sizeof(AINetNode)));
        memset(aiNode, 0, sizeof(AINetNode));

        if (tail != 0) {
            tail->next = aiNode;
        } else {
            aiNet->nodeListHead = aiNode;
        }
        tail = aiNode;

        zReader::Node *const payload = AINetZrdPayload(node);
        zReader::Node *const position = AINetZrdPayload(&payload[2]);
        zReader::Node *const neighbors = AINetZrdPayload(&payload[3]);

        aiNode->nodeIndex = nodeIndex;
        aiNode->costOrType = payload[1].value.i32;
        aiNode->position.x = position[1].value.f32;
        aiNode->position.y = position[2].value.f32;
        aiNode->position.z = position[3].value.f32;
        aiNode->neighborIndices[0] = neighbors[1].value.i32;
        aiNode->neighborIndices[1] = neighbors[2].value.i32;
        aiNode->neighborIndices[2] = neighbors[3].value.i32;
    }

    AINet::ResolveNeighborLinksAndBuildProbeFans(aiNet->nodeListHead, aiNet->pathWidth);
    zReader::FreeLoadedTree(root);
    return aiNet;
}

// Reimplements 0x402ff0: AINet::Alloc (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE AINet *RECOIL_CDECL AINet::Alloc() {
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

// Reimplements 0x403510: AINet::FindByNetId (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE AINet *RECOIL_FASTCALL AINet::FindByNetId(int netId) {
    AINet *aiNet = g_AINetListHead;
    while (aiNet != 0) {
        if (aiNet->netId == netId) {
            return aiNet;
        }
        aiNet = aiNet->next;
    }

    return 0;
}

// Reimplements 0x4036f0: AINet::FindNearestNode (D:\Proj\Battlesport\ainet.cpp)
RECOIL_NOINLINE AINetNode *RECOIL_FASTCALL AINet::FindNearestNode(const zVec3 *position,
                                                                  AINetNode *nodeListHead) {
    AINetNode *nearest = 0;
    float bestDistanceSq = -1.0f;

    AINetNode *node = nodeListHead;
    while (node != 0) {
        const float distanceSq = zMath::Vec3DeltaLengthSq(position, &node->position);
        if (distanceSq < bestDistanceSq || bestDistanceSq < 0.0f) {
            bestDistanceSq = distanceSq;
            nearest = node;
        }
        node = node->next;
    }

    return nearest;
}

// Reimplements 0x403530: AINet::FindNodeByIndex (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE AINetNode *RECOIL_FASTCALL AINet::FindNodeByIndex(int nodeIndex,
                                                                  AINetNode *nodeListHead) {
    AINetNode *node = nodeListHead;
    while (node != 0) {
        if (node->nodeIndex == nodeIndex) {
            return node;
        }
        node = node->next;
    }

    return 0;
}

// Reimplements 0x403620: AINetPathProbeFan::InitFromSegment
// (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL
AINetPathProbeFan::InitFromSegment(zVec3 fromPosition, zVec3 toPosition, float pathWidth) {
    delta.x = toPosition.x - fromPosition.x;
    delta.y = toPosition.y - fromPosition.y;
    delta.z = toPosition.z - fromPosition.z;

    const float xzLength = sqrt(delta.x * delta.x + delta.z * delta.z);
    const float minimumTravel = pathWidth * 0.5f;
    const float availableTravel = xzLength - pathWidth;
    clampedTravel = availableTravel < minimumTravel ? minimumTravel : availableTravel;

    zMath::Vec3NormalizeXZ(&delta, &delta);
    zMath::Vec3PerpXZ(&delta, &perpendicular);
    zMath::Vec3RotateY(&probeDirPlus45, &perpendicular, 45.0f);
    zMath::Vec3RotateY(&probeDirMinus45, &perpendicular, -45.0f);
    this->pathWidth = pathWidth;
}

// Reimplements 0x403550: AINet::ResolveNeighborLinksAndBuildProbeFans
// (D:\Proj\Battlesport\ai_net.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
AINet::ResolveNeighborLinksAndBuildProbeFans(AINetNode *nodeListHead, float pathWidth) {
    AINetNode *node = nodeListHead;
    while (node != 0) {
        for (int slot = 0; slot < 3; ++slot) {
            const int neighborIndex = node->neighborIndices[slot];
            if (neighborIndex < 0) {
                node->neighborNodes[slot] = 0;
                node->probeFans[slot] = 0;
            } else {
                AINetNode *const neighbor = AINet::FindNodeByIndex(neighborIndex, nodeListHead);
                node->neighborNodes[slot] = neighbor;

                AINetPathProbeFan *const probeFan =
                    (AINetPathProbeFan *)(malloc(sizeof(AINetPathProbeFan)));
                node->probeFans[slot] = probeFan;
                memset(probeFan, 0, sizeof(AINetPathProbeFan));
                probeFan->InitFromSegment(node->position, neighbor->position, pathWidth);
            }
        }

        node = node->next;
    }
}

// Reimplements 0x4037c0: AINetNode::Free (src/Battlesport/ainet.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL AINetNode::Free() {
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

// Reimplements 0x403800: AINet::Free (src/Battlesport/ainet.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL AINet::Free() {
    if (this == 0) {
        return;
    }

    AINetNode *node = nodeListHead;
    while (node != 0) {
        AINetNode *const next = node->next;
        node->Free();
        node = next;
    }

    free(this);
}

// Reimplements 0x403870: AINet::FreeAll (src/Battlesport/ainet.cpp)
RECOIL_NOINLINE void RECOIL_CDECL AINet::FreeAll() {
    AINet *aiNet = g_AINetListHead;
    while (aiNet != 0) {
        g_AINetListHead = aiNet->next;
        aiNet->Free();
        aiNet = g_AINetListHead;
    }
}
