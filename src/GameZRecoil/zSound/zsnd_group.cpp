#include "zSound.h"

#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zReader/zReader.h"

#include <stdlib.h>
#include <string.h>

extern "C" zArchiveList *g_zSndStream_PendingList = 0;
extern "C" zArchiveList *g_zSndStream_ActiveList = 0;
extern "C" zArchiveList *g_zSndStream_FreeList = 0;
extern "C" zSndStreamRequest *g_zSndStream_MatchedRequest = 0;
extern "C" int g_zSndStream_MatchedRequestCount = 0;
extern "C" zClass_NodePartial *g_zSndStream_RootNode = 0;

namespace {
const char *kZSndGroupSourceFile = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_grp.cpp";

// Reimplements 0x4a51e0: MatchStreamRequestPredicate
int RECOIL_FASTCALL MatchStreamRequestPredicate(void *payload, void *userData) {
    return payload != userData ? 1 : 0;
}

zArchiveListNode *FindNodeByPayload(zArchiveList *list, void *payload) {
    if (list == 0 || list->count == 0) {
        return 0;
    }

    zArchiveListNode *node = list->head;
    for (int i = 0; i < list->count; ++i) {
        if (node->payload == payload) {
            return node;
        }
        node = node->next;
    }

    return 0;
}

void *RemovePayloadFromList(zArchiveList *list, void *payload) {
    zArchiveListNode *node = FindNodeByPayload(list, payload);
    if (node == 0) {
        return 0;
    }

    if (list->count == 1) {
        list->head = 0;
    } else {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        if (list->head == node) {
            list->head = node->next;
        }
    }

    --list->count;
    return zArchiveList_FreeNode(node);
}

int RECOIL_FASTCALL MatchFinishedRequestPredicate(void *payload, void *) {
    zSndStreamRequest * request = static_cast<zSndStreamRequest *>(payload);
    if (request->streamState == 4) {
        if (g_zSndStream_MatchedRequest == 0) {
            g_zSndStream_MatchedRequest = request;
        }
        ++g_zSndStream_MatchedRequestCount;
    }
    return 1;
}

zReader::Node *ArrayBase(zReader::Node *node) {
    return node->value.nodes;
}

int ArrayCount(zReader::Node *node) {
    return ArrayBase(node)[0].value.i32;
}

void ReportConfigError(int line, const char *message,
                       const zSndGroupRuntimeFields *groupFields) {
    zError::ReportOld(0x200, kZSndGroupSourceFile, line, message, groupFields->groupName);
}

void StorePlayCount(zSndGroupConfigBlock *block, unsigned short count) {
    block->maxPlayCount = count;
    block->currentPlayCount = block->maxPlayCount;
}

bool StoreFloatField(zReader::Node *valueNode, float *outValue) {
    if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
        *outValue = valueNode->value.f32;
        return true;
    }

    if (valueNode->type == zReader::ZRDR_NODE_INT) {
        *outValue = static_cast<float>(valueNode->value.i32);
        return true;
    }

    return false;
}

bool StoreRepeatCount(zReader::Node *valueNode, unsigned short *outValue) {
    if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
        *outValue = static_cast<unsigned short>(valueNode->value.f32);
        return true;
    }

    if (valueNode->type == zReader::ZRDR_NODE_INT) {
        *outValue = static_cast<unsigned short>(valueNode->value.i32);
        return true;
    }

    return false;
}

void NormalizeDefaultWeights(zSndGroup *group) {
    bool needsDefaultWeights = false;
    for (int i = 0; i < group->configBlockCount; ++i) {
        if (group->configBlocks[i].weight < 0.0001f) {
            needsDefaultWeights = true;
            break;
        }
    }

    if (!needsDefaultWeights || group->configBlockCount <= 0) {
        return;
    }

    const float defaultWeight = 100.0f / static_cast<float>(group->configBlockCount);
    {
    for (int defaultIndex = 0; defaultIndex < group->configBlockCount; ++defaultIndex) {
        group->configBlocks[defaultIndex].weight = defaultWeight;
    }
    }
}
} // namespace

// Reimplements 0x4a51f0: zSndStreamRequest_StopIfActive
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndStreamRequest_StopIfActive(zSndPlayHandle *request) {
    void *const found = zArchiveList_FindPayloadByPredicate(g_zSndStream_ActiveList,
                                                            &MatchStreamRequestPredicate, request);
    if (found == 0) {
        return 0;
    }

    ((zSndStreamRequest *)(request))->streamState = 4;
    return 1;
}

// Reimplements 0x4a5220: zSndStreamRequest_MatchGroupPredicate
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndStreamRequest_MatchGroupPredicate(void *payload, void *group) {
    return static_cast<zSndStreamRequest *>(payload)->group != group ? 1 : 0;
}

// Reimplements 0x4a44e0: zSndPendingList_MatchNamePredicate
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPendingList_MatchNamePredicate(void *payload, void *sampleName) {
    return strcmp(static_cast<zSndGroup *>(payload)->groupName,
                       static_cast<const char *>(sampleName)) != 0
               ? 1
               : 0;
}

// Reimplements 0x4a44c0: zSndPendingList_FindByName
extern "C" RECOIL_NOINLINE zSndSample *RECOIL_FASTCALL
zSndPendingList_FindByName(const char *sampleName) {
    if (g_zSndStream_PendingList == 0) {
        return 0;
    }

    return (zSndSample *)(zArchiveList_FindPayloadByPredicate(
        g_zSndStream_PendingList, &zSndPendingList_MatchNamePredicate,
        const_cast<char *>(sampleName)));
}

// Reimplements 0x4a4d10: zSndGroup::SelectWeightedEntry
RECOIL_NOINLINE zSndGroupConfigBlock *RECOIL_THISCALL zSndGroup::SelectWeightedEntry() {
    if (configBlockCount == 1) {
        return configBlocks[0].maxPlayCount != 0 ? configBlocks : 0;
    }

    float totalWeight = 0.0f;
    for (int i = 0; i < configBlockCount; ++i) {
        if (configBlocks[i].maxPlayCount != 0) {
            totalWeight += configBlocks[i].weight;
        }
    }

    zSndGroupConfigBlock *result = 0;
    int selectedIndex = 0;
    const float selection = static_cast<float>(rand()) * 3.05185094e-05f * totalWeight;
    const float selectSlop = totalWeight * 0.00100000005f;
    float cumulativeWeight = 0.0f;
    for (; selectedIndex < configBlockCount; ++selectedIndex) {
        zSndGroupConfigBlock &entry = configBlocks[selectedIndex];
        if (entry.maxPlayCount != 0) {
            cumulativeWeight += entry.weight;
            if (cumulativeWeight + selectSlop >= selection) {
                result = &entry;
                if (dynamicWeightsEnabled != 0) {
                    entry.weight *= dynamicWeightScale;
                }
                break;
            }
        }
    }

    if (dynamicWeightsEnabled != 0) {
        float renormalizeTotal = 0.0f;
        for (int i = 0; i < configBlockCount; ++i) {
            zSndGroupConfigBlock &entry = configBlocks[i];
            if (entry.maxPlayCount != 0) {
                if (i != selectedIndex && entry.weight < 0.00100000005f) {
                    entry.weight = 0.00100000005f;
                }
                renormalizeTotal += entry.weight;
            }
        }

        if (renormalizeTotal != 0.0f) {
            const float scale = 100.0f / renormalizeTotal;
            for (int i = 0; i < configBlockCount; ++i) {
                if (configBlocks[i].maxPlayCount != 0) {
                    configBlocks[i].weight *= scale;
                }
            }
        }
    }

    return result;
}

// Reimplements 0x4a4cb0: zSndStreamRequest::StateBeginGroup
RECOIL_NOINLINE int RECOIL_THISCALL zSndStreamRequest::StateBeginGroup() {
    elapsedSec = 0.0f;
    playIndex = 0;
    currentEntry = 0;

    if (group->configBlockCount <= 0) {
        currentEntry = 0;
        streamState = 4;
        return 1;
    }

    currentEntry = group->SelectWeightedEntry();
    streamState = currentEntry != 0 ? 1 : 4;
    return 1;
}

// Reimplements 0x4a5350: zSndStreamMgr_EnsureInit
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zSndStreamMgr_EnsureInit() {
    if (g_zSndStream_RootNode == 0) {
        g_zSndStream_RootNode = zClass_Object3D::gwObject3DInit();
        if (g_zSndStream_RootNode == 0) {
            return 0;
        }

        zClass_Class::gwNodeSetActionCallbackTail(
            g_zSndStream_RootNode, (void *)(&zSndStreamMgr_RecycleFinishedRequest));
    }

    if (g_zSndStream_PendingList == 0) {
        g_zSndStream_PendingList = zArchiveList_CreateEmpty();
        if (g_zSndStream_PendingList == 0) {
            return 0;
        }
    }

    if (g_zSndStream_ActiveList == 0) {
        g_zSndStream_ActiveList = zArchiveList_CreateEmpty();
        if (g_zSndStream_ActiveList == 0) {
            return 0;
        }
    }

    if (g_zSndStream_FreeList == 0) {
        g_zSndStream_FreeList = zArchiveList_CreateEmpty();
        if (g_zSndStream_FreeList == 0) {
            return 0;
        }
    }

    return 1;
}

namespace zSndStreamMgr {
namespace {
void FreeRequestList(zArchiveList *&list) {
    if (list == 0) {
        return;
    }

    void *payload = zArchiveList_PopFrontPayload(list);
    while (payload != 0) {
        free(payload);
        payload = zArchiveList_PopFrontPayload(list);
    }
    zArchiveList_Destroy(list);
    list = 0;
}

void FreePendingGroupConfig(zSndGroup *pendingConfig) {
    if (pendingConfig->createGuard != 1) {
        return;
    }

    for (int i = 0; i < pendingConfig->configBlockCount; ++i) {
        zSndGroupConfigBlock *child = pendingConfig->configBlocks[i].child;
        while (child != 0) {
            zSndGroupConfigBlock *const next = child->child;
            free(child);
            child = next;
        }
    }

    free(pendingConfig->configBlocks);
    free(pendingConfig);
}

void FreePendingList(zArchiveList *&list) {
    if (list == 0) {
        return;
    }

    zSndGroup * pendingConfig = static_cast<zSndGroup *>(zArchiveList_PopFrontPayload(list));
    while (pendingConfig != 0) {
        FreePendingGroupConfig(pendingConfig);
        pendingConfig = static_cast<zSndGroup *>(zArchiveList_PopFrontPayload(list));
    }

    zArchiveList_Destroy(list);
    list = 0;
}
} // namespace

// Reimplements 0x4a50a0: zSndStreamMgr::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    if (g_zSndStream_RootNode != 0 && zClass::IsInitialized() != 0) {
        zClass_Class::gwNodeSetActionCallback(g_zSndStream_RootNode, 0);
        zClass_Object3D::DeleteNode(g_zSndStream_RootNode);
    }
    g_zSndStream_RootNode = 0;

    FreeRequestList(g_zSndStream_ActiveList);
    FreeRequestList(g_zSndStream_FreeList);
    FreePendingList(g_zSndStream_PendingList);
    g_zSndStream_MatchedRequest = 0;
    g_zSndStream_MatchedRequestCount = 0;
    return 1;
}
} // namespace zSndStreamMgr

// Reimplements 0x4a5250: zSndGroup::QueueStreamRequest
RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL zSndGroup::QueueStreamRequest(
    int hasWorldPos, float gain, zVec3 *worldPos, zVec3 *velocity) {
    if (g_zSndStream_RootNode == 0) {
        zSndStreamMgr_EnsureInit();
    }

    if (playSolo != 0 && zArchiveList_FindPayloadByPredicate(g_zSndStream_ActiveList,
                                                             &zSndStreamRequest_MatchGroupPredicate,
                                                             this) != 0) {
        return 0;
    }

    zSndStreamRequest * request = static_cast<zSndStreamRequest *>(zArchiveList_PopFrontPayload(g_zSndStream_FreeList));
    if (request == 0) {
        request = static_cast<zSndStreamRequest *>(malloc(sizeof(zSndStreamRequest)));
        if (request == 0) {
            return 0;
        }
    }

    memset(request, 0, sizeof(*request));
    zArchiveList_PushFrontPayload(g_zSndStream_ActiveList, request);

    request->gain = gain;
    request->handleKind = ZSND_PLAYHANDLE_STREAM_REQUEST;
    request->group = this;
    request->streamState = 0;
    if (hasWorldPos != 0 && worldPos != 0) {
        request->hasWorldPos = 1;
        request->worldPos = *worldPos;
        if (velocity != 0) {
            request->velocity = *velocity;
        } else {
            memset(&request->velocity, 0, sizeof(request->velocity));
        }
    } else {
        request->hasWorldPos = 0;
    }

    return request->StateBeginGroup() != 0 ? (zSndPlayHandle *)(request) : 0;
}

// Reimplements 0x4a5230: zSndGroup::QueueStreamRequestSimple
RECOIL_NOINLINE zSndPlayHandle *RECOIL_THISCALL zSndGroup::QueueStreamRequestSimple(float gain) {
    return QueueStreamRequest(0, gain, 0, 0);
}

// Reimplements 0x4a53d0: zSndGroup::QueueStreamRequestWithWorldPos
RECOIL_NOINLINE zSndPlayHandle *RECOIL_FASTCALL
zSndGroup::QueueStreamRequestWithWorldPos(zVec3 *worldPos, float gain, zVec3 *velocity) {
    return QueueStreamRequest(1, gain, worldPos, velocity);
}

extern "C" RECOIL_NOINLINE void RECOIL_CDECL zSndStreamMgr_RecycleFinishedRequest() {
    zArchiveList_FindPayloadByPredicate(g_zSndStream_ActiveList, &MatchFinishedRequestPredicate,
                                        0);

    if (g_zSndStream_MatchedRequest == 0) {
        return;
    }

    RemovePayloadFromList(g_zSndStream_ActiveList, g_zSndStream_MatchedRequest);
    zArchiveList_PushFrontPayload(g_zSndStream_FreeList, g_zSndStream_MatchedRequest);
    g_zSndStream_MatchedRequest = 0;
    --g_zSndStream_MatchedRequestCount;
}

// Reimplements 0x4a49b0: zSndGroup_LoadConfigBlock
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndGroup_LoadConfigBlock(zReader::Node *readerNode, zSndGroupRuntimeFields *groupFields,
                          zSndGroupConfigBlock *outConfigBlock) {
    if (outConfigBlock->maxPlayCount == 0) {
        outConfigBlock->maxPlayCount = 0xffff;
    }
    outConfigBlock->currentPlayCount = outConfigBlock->maxPlayCount;

    zReader::Node *nodeArray = ArrayBase(readerNode);
    {
    for (int childIndex = 1; childIndex < ArrayCount(readerNode); ++childIndex) {
        zReader::Node *childNode = &nodeArray[childIndex];
        if (childNode->type == zReader::ZRDR_NODE_ARRAY) {
            if (childIndex == 1) {
                zSndGroup_LoadConfigBlock(childNode, groupFields, outConfigBlock);
            } else {
                zSndGroupConfigBlock *nested = static_cast<zSndGroupConfigBlock *>(
                    calloc(1, sizeof(zSndGroupConfigBlock)));
                if (nested != 0) {
                    outConfigBlock->child = nested;
                    zSndGroup_LoadConfigBlock(childNode, groupFields, nested);
                    outConfigBlock = nested;
                }
            }
            continue;
        }

        if (childNode->type != zReader::ZRDR_NODE_STRING) {
            continue;
        }

        const char *key = childNode->value.str;
        zReader::Node *valueNode = &nodeArray[childIndex + 1];
        if (strcmp(key, "DELAY_PLAY") == 0) {
            if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                outConfigBlock->delayPlaySec = valueNode->value.f32;
                ++childIndex;
            } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                outConfigBlock->delayPlaySec = static_cast<float>(valueNode->value.i32);
                ++childIndex;
            } else {
                ReportConfigError(0xb1, "Error loading DELAY_PLAY for sound group (%s)",
                                  groupFields);
                ++childIndex;
            }
        } else if (strcmp(key, "PLAY_COUNT") == 0) {
            if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                StorePlayCount(outConfigBlock,
                               static_cast<unsigned short>(valueNode->value.f32 + 0.5f));
                ++childIndex;
            } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                StorePlayCount(outConfigBlock, static_cast<unsigned short>(valueNode->value.i32));
                ++childIndex;
            } else {
                ReportConfigError(0xbf, "Error loading PLAY_COUNT for sound group (%s)",
                                  groupFields);
                ++childIndex;
                outConfigBlock->currentPlayCount = outConfigBlock->maxPlayCount;
            }
        } else if (strcmp(key, "WEIGHT") == 0) {
            if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                outConfigBlock->weight = valueNode->value.f32;
                ++childIndex;
            } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                outConfigBlock->weight = static_cast<float>(valueNode->value.i32);
                ++childIndex;
            } else {
                ReportConfigError(0xcf, "Error loading WEIGHT for sound group (%s)", groupFields);
                ++childIndex;
            }
        } else {
            outConfigBlock->streamName = key;
        }
    }
    }

    return 1;
}

// Reimplements 0x4a4590: zSndGroup_LoadFromConfigNode
extern "C" RECOIL_NOINLINE zSndGroup *RECOIL_FASTCALL
zSndGroup_LoadFromConfigNode(zReader::Node *readerNode) {
    if (readerNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zSndGroup *result = static_cast<zSndGroup *>(calloc(1, sizeof(zSndGroup)));
    if (result == 0) {
        return 0;
    }

    result->createGuard = 1;
    zReader::Node *nodeArray = ArrayBase(readerNode);
    {
    for (int childIndex = 1; childIndex < ArrayCount(readerNode); ++childIndex) {
        zReader::Node *childNode = &nodeArray[childIndex];
        if (childNode->type == zReader::ZRDR_NODE_ARRAY) {
            zSndGroupConfigBlock *blocks = static_cast<zSndGroupConfigBlock *>(realloc(
                result->configBlocks, static_cast<size_t>(result->configBlockCount + 1) *
                                          sizeof(zSndGroupConfigBlock)));
            result->configBlocks = blocks;
            if (blocks != 0) {
                zSndGroupConfigBlock *block = &blocks[result->configBlockCount];
                memset(block, 0, sizeof(*block));
                zSndGroup_LoadConfigBlock(
                    childNode, (zSndGroupRuntimeFields *)(&result->groupName),
                    block);
                ++result->configBlockCount;
            }
            continue;
        }

        if (childNode->type != zReader::ZRDR_NODE_STRING) {
            continue;
        }

        const char *key = childNode->value.str;
        zReader::Node *valueNode = &nodeArray[childIndex + 1];
        if (strcmp(key, "DELAY_REPEAT") == 0) {
            if (!StoreFloatField(valueNode, &result->delayRepeatSec)) {
                ReportConfigError(0x141, "Error loading DELAY_REPEAT for sound group (%s)",
                                  (zSndGroupRuntimeFields *)(&result->groupName));
            }
            ++childIndex;
        } else if (strcmp(key, "DELAY_TERMINATION") == 0) {
            if (!StoreFloatField(valueNode, &result->delayTerminationSec)) {
                ReportConfigError(0x14f, "Error loading DELAY_TERMINATION for sound group (%s)",
                                  (zSndGroupRuntimeFields *)(&result->groupName));
            }
            ++childIndex;
        } else if (strcmp(key, "DYNAMIC_WEIGHTS") == 0) {
            result->dynamicWeightsEnabled = 1;
            if (!StoreFloatField(valueNode, &result->dynamicWeightScale)) {
                ReportConfigError(0x15f, "Error loading DYNAMIC_WEIGHTS for sound group (%s)",
                                  (zSndGroupRuntimeFields *)(&result->groupName));
            }

            if (result->dynamicWeightScale <= 0.0f) {
                result->dynamicWeightScale = 0.0f;
            } else if (result->dynamicWeightScale >= 1.0f) {
                result->dynamicWeightScale = 1.0f;
            }
            ++childIndex;
        } else if (strcmp(key, "PLAY_SOLO") == 0) {
            result->playSolo = 1;
        } else if (strcmp(key, "REPEAT") == 0) {
            if (!StoreRepeatCount(valueNode, &result->repeatCount)) {
                ReportConfigError(0x174, "Error loading REPEAT for sound group (%s)",
                                  (zSndGroupRuntimeFields *)(&result->groupName));
            }
            ++childIndex;
        } else {
            result->groupName = key;
        }
    }
    }

    NormalizeDefaultWeights(result);
    return result;
}

// Reimplements 0x4a4530: zSndGroup_QueuePendingLoadsFromConfigNode
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndGroup_QueuePendingLoadsFromConfigNode(zReader::Node *readerNode) {
    if (readerNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    if (g_zSndStream_PendingList == 0) {
        g_zSndStream_PendingList = zArchiveList_CreateEmpty();
        if (g_zSndStream_PendingList == 0) {
            return 0;
        }
    }

    zReader::Node *nodeArray = ArrayBase(readerNode);
    for (int i = 1; i < ArrayCount(readerNode); ++i) {
        zSndGroup *payload = zSndGroup_LoadFromConfigNode(&nodeArray[i]);
        if (payload != 0) {
            zArchiveList_PushFrontPayload(g_zSndStream_PendingList, payload);
        }
    }

    return 1;
}
