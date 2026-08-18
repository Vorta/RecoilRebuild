#include "zsnd.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zReader/zreader.h"

#include <stdlib.h>
#include <string.h>

extern "C" zArchiveList *g_zSndStream_PendingList = 0;
extern "C" zArchiveList *g_zSndStream_ActiveList = 0;
extern "C" zArchiveList *g_zSndStream_FreeList = 0;
extern "C" zSndStreamRequest *g_zSndStream_MatchedRequest = 0;
extern "C" int g_zSndStream_MatchedRequestCount = 0;
extern "C" zClass_NodePartial *g_zSndStream_RootNode = 0;

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgrouprepeatloaderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e2db8: g_zSnd_SoundGroupRepeatLoadErrorFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable REPEAT parser diagnostic format.
 */
char g_zSnd_SoundGroupRepeatLoadErrorFmt[0x2a] =
    "Error loading REPEAT for sound group (%s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgrouprepeatkey
 * @recoil-artifact defines .data recoil:data:0x4e2de4: g_zSnd_SoundGroupRepeatKey.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable REPEAT parser key.
 */
char g_zSnd_SoundGroupRepeatKey[0x7] = "REPEAT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupplaysolokey
 * @recoil-artifact defines .data recoil:data:0x4e2dec: g_zSnd_SoundGroupPlaySoloKey.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable PLAY_SOLO parser key.
 */
char g_zSnd_SoundGroupPlaySoloKey[0xa] = "PLAY_SOLO";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-sourcefile-zsndgrpcpp
 * @recoil-artifact defines .data recoil:data:0x4e2df8: g_zSnd_SourceFile_ZsndGrpCpp.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable source-file path used by zSnd group diagnostics.
 */
char g_zSnd_SourceFile_ZsndGrpCpp[0x28] =
    "D:\\Proj\\GameZRecoil\\zSound\\zsnd_grp.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupdynamicweightsloaderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e2e20: g_zSnd_SoundGroupDynamicWeightsLoadErrorFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable DYNAMIC_WEIGHTS parser diagnostic format.
 */
char g_zSnd_SoundGroupDynamicWeightsLoadErrorFmt[0x33] =
    "Error loading DYNAMIC_WEIGHTS for sound group (%s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupdynamicweightskey
 * @recoil-artifact defines .data recoil:data:0x4e2e54: g_zSnd_SoundGroupDynamicWeightsKey.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable DYNAMIC_WEIGHTS parser key.
 */
char g_zSnd_SoundGroupDynamicWeightsKey[0x10] = "DYNAMIC_WEIGHTS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupdelayterminationloaderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e2e64: g_zSnd_SoundGroupDelayTerminationLoadErrorFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable DELAY_TERMINATION parser diagnostic format.
 */
char g_zSnd_SoundGroupDelayTerminationLoadErrorFmt[0x35] =
    "Error loading DELAY_TERMINATION for sound group (%s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupdelayterminationkey
 * @recoil-artifact defines .data recoil:data:0x4e2e9c: g_zSnd_SoundGroupDelayTerminationKey.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable DELAY_TERMINATION parser key.
 */
char g_zSnd_SoundGroupDelayTerminationKey[0x12] = "DELAY_TERMINATION";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupdelayrepeatloaderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e2eb0: g_zSnd_SoundGroupDelayRepeatLoadErrorFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable DELAY_REPEAT parser diagnostic format.
 */
char g_zSnd_SoundGroupDelayRepeatLoadErrorFmt[0x30] =
    "Error loading DELAY_REPEAT for sound group (%s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupdelayrepeatkey
 * @recoil-artifact defines .data recoil:data:0x4e2ee0: g_zSnd_SoundGroupDelayRepeatKey.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable DELAY_REPEAT parser key.
 */
char g_zSnd_SoundGroupDelayRepeatKey[0xd] = "DELAY_REPEAT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupweightloaderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e2ef0: g_zSnd_SoundGroupWeightLoadErrorFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable WEIGHT parser diagnostic format.
 */
char g_zSnd_SoundGroupWeightLoadErrorFmt[0x2a] =
    "Error loading WEIGHT for sound group (%s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupweightkey
 * @recoil-artifact defines .data recoil:data:0x4e2f1c: g_zSnd_SoundGroupWeightKey.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable WEIGHT parser key.
 */
char g_zSnd_SoundGroupWeightKey[0x7] = "WEIGHT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupplaycountloaderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e2f24: g_zSnd_SoundGroupPlayCountLoadErrorFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable PLAY_COUNT parser diagnostic format.
 */
char g_zSnd_SoundGroupPlayCountLoadErrorFmt[0x2e] =
    "Error loading PLAY_COUNT for sound group (%s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupplaycountkey
 * @recoil-artifact defines .data recoil:data:0x4e2f54: g_zSnd_SoundGroupPlayCountKey.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable PLAY_COUNT parser key.
 */
char g_zSnd_SoundGroupPlayCountKey[0xb] = "PLAY_COUNT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupdelayplayloaderrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e2f60: g_zSnd_SoundGroupDelayPlayLoadErrorFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable DELAY_PLAY parser diagnostic format.
 */
char g_zSnd_SoundGroupDelayPlayLoadErrorFmt[0x2e] =
    "Error loading DELAY_PLAY for sound group (%s)";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-soundgroupdelayplaykey
 * @recoil-artifact defines .data recoil:data:0x4e2f90: g_zSnd_SoundGroupDelayPlayKey.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable DELAY_PLAY parser key.
 */
char g_zSnd_SoundGroupDelayPlayKey[0xb] = "DELAY_PLAY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-g-zsnd-nulltoken
 * @recoil-artifact defines .data recoil:data:0x4e2f9c: g_zSnd_NullToken.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the writable stream-sample NULL token.
 */
char g_zSnd_NullToken[0x5] = "NULL";
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndpendinglist-findbyname
 * @recoil-artifact defines .text recoil:function:0x4a44c0: zSndPendingList_FindByName.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zSound\zsnd_grp.cpp.
 * Purpose: search the pending stream group list for a group with the requested sample name.
 */
extern "C" zSndSample *__fastcall zSndPendingList_FindByName(
    const char *sampleName
) {
    if (g_zSndStream_PendingList == 0) {
        return 0;
    }

    return (zSndSample *)(zArchiveList_FindPayloadByPredicate(
        g_zSndStream_PendingList,
        &zSndPendingList_MatchNamePredicate,
        (char *)(sampleName)
    ));
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndpendinglist-matchnamepredicate
 * @recoil-artifact defines .text recoil:function:0x4a44e0: zSndPendingList_MatchNamePredicate.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zSound\zsnd_grp.cpp.
 * Purpose: compare a pending sound group name with the requested sample name.
 */
extern "C" int __fastcall zSndPendingList_MatchNamePredicate(
    void *payload,
    void *sampleName
) {
    return strcmp(
        ((zSndGroup *)(payload))->groupName,
        (const char *)(sampleName)
    ) != 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndgroup-queuependingloadsfromconfignode
 * @recoil-artifact defines .text recoil:function:0x4a4530: zSndGroup_QueuePendingLoadsFromConfigNode.
 * Purpose: queue every parsed sound group from a top-level config array for
 * deferred stream loading.
 */
extern "C" int __fastcall zSndGroup_QueuePendingLoadsFromConfigNode(
    zReader::Node *readerNode
) {
    if (readerNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    if (g_zSndStream_PendingList == 0) {
        g_zSndStream_PendingList = zArchiveList_CreateEmpty();
        if (g_zSndStream_PendingList == 0) {
            return 0;
        }
    }

    zReader::Node *nodeArray = readerNode->value.nodes;
    for (int i = 1; i < nodeArray[0].value.i32; ++i) {
        zSndGroup *payload = zSndGroup_LoadFromConfigNode(&nodeArray[i]);
        if (payload != 0) {
            zArchiveList_PushFrontPayload(
                g_zSndStream_PendingList,
                payload
            );
        }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndgroup-loadfromconfignode
 * @recoil-artifact defines .text recoil:function:0x4a4590: zSndGroup_LoadFromConfigNode.
 * Purpose: allocate and populate one sound group from a zReader array node.
 */
extern "C" zSndGroup *__fastcall zSndGroup_LoadFromConfigNode(
    zReader::Node *readerNode
) {
    if (readerNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zSndGroup *result = (zSndGroup *)(calloc(
        1,
        sizeof(zSndGroup)
    ));
    if (result == 0) {
        return 0;
    }

    result->createGuard = 1;
    zReader::Node *nodeArray = readerNode->value.nodes;
    {
        for (int childIndex = 1; childIndex < nodeArray[0].value.i32; ++childIndex) {
            zReader::Node *childNode = &nodeArray[childIndex];
            if (childNode->type == zReader::ZRDR_NODE_ARRAY) {
                zSndGroupConfigBlock *blocks = (zSndGroupConfigBlock *)(realloc(
                    result->configBlocks,
                    (size_t)(result->configBlockCount + 1) * sizeof(zSndGroupConfigBlock)
                ));
                result->configBlocks = blocks;
                if (blocks != 0) {
                    zSndGroupConfigBlock *block = &blocks[result->configBlockCount];
                    memset(
                        block,
                        0,
                        sizeof(*block)
                    );
                    zSndGroup_LoadConfigBlock(
                        childNode,
                        (zSndGroupRuntimeFields *)(&result->groupName),
                        block
                    );
                    ++result->configBlockCount;
                }
                continue;
            }

            if (childNode->type != zReader::ZRDR_NODE_STRING) {
                continue;
            }

            const char *key = childNode->value.str;
            zReader::Node *valueNode = &nodeArray[childIndex + 1];
            if (strcmp(
                key,
                g_zSnd_SoundGroupDelayRepeatKey
            ) == 0) {
                if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                    result->delayRepeatSec = valueNode->value.f32;
                } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                    result->delayRepeatSec = (float)(valueNode->value.i32);
                } else {
                    zError::ReportOld(
                        0x200,
                        g_zSnd_SourceFile_ZsndGrpCpp,
                        0x141,
                        g_zSnd_SoundGroupDelayRepeatLoadErrorFmt,
                        result->groupName
                    );
                }
                ++childIndex;
            } else if (strcmp(
                key,
                g_zSnd_SoundGroupDelayTerminationKey
            ) == 0) {
                if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                    result->delayTerminationSec = valueNode->value.f32;
                } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                    result->delayTerminationSec = (float)(valueNode->value.i32);
                } else {
                    zError::ReportOld(
                        0x200,
                        g_zSnd_SourceFile_ZsndGrpCpp,
                        0x14f,
                        g_zSnd_SoundGroupDelayTerminationLoadErrorFmt,
                        result->groupName
                    );
                }
                ++childIndex;
            } else if (strcmp(
                key,
                g_zSnd_SoundGroupDynamicWeightsKey
            ) == 0) {
                result->dynamicWeightsEnabled = 1;
                if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                    result->dynamicWeightScale = valueNode->value.f32;
                } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                    result->dynamicWeightScale = (float)(valueNode->value.i32);
                } else {
                    zError::ReportOld(
                        0x200,
                        g_zSnd_SourceFile_ZsndGrpCpp,
                        0x15f,
                        g_zSnd_SoundGroupDynamicWeightsLoadErrorFmt,
                        result->groupName
                    );
                }

                if (result->dynamicWeightScale <= 0.0f) {
                    result->dynamicWeightScale = 0.0f;
                } else if (result->dynamicWeightScale >= 1.0f) {
                    result->dynamicWeightScale = 1.0f;
                }
                ++childIndex;
            } else if (strcmp(
                key,
                g_zSnd_SoundGroupPlaySoloKey
            ) == 0) {
                result->playSolo = 1;
            } else if (strcmp(
                key,
                g_zSnd_SoundGroupRepeatKey
            ) == 0) {
                if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                    result->repeatCount = (unsigned short)(valueNode->value.f32);
                } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                    result->repeatCount = (unsigned short)(valueNode->value.i32);
                } else {
                    zError::ReportOld(
                        0x200,
                        g_zSnd_SourceFile_ZsndGrpCpp,
                        0x174,
                        g_zSnd_SoundGroupRepeatLoadErrorFmt,
                        result->groupName
                    );
                }
                ++childIndex;
            } else {
                result->groupName = key;
            }
        }
    }

    int needsDefaultWeights = 0;
    for (int i = 0; i < result->configBlockCount; ++i) {
        if (result->configBlocks[i].weight < 0.0001f) {
            needsDefaultWeights = 1;
            break;
        }
    }

    if (needsDefaultWeights != 0 && result->configBlockCount > 0) {
        const float defaultWeight = 100.0f / (float)(result->configBlockCount);
        for (int defaultIndex = 0; defaultIndex < result->configBlockCount; ++defaultIndex) {
            result->configBlocks[defaultIndex].weight = defaultWeight;
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndgroup-loadconfigblock
 * @recoil-artifact defines .text recoil:function:0x4a49b0: zSndGroup_LoadConfigBlock.
 * Purpose: parse one sound-group config block, including nested blocks and
 * per-entry playback controls.
 */
extern "C" int __fastcall zSndGroup_LoadConfigBlock(
    zReader::Node *readerNode,
    zSndGroupRuntimeFields *groupFields,
    zSndGroupConfigBlock *outConfigBlock
) {
    if (outConfigBlock->maxPlayCount == 0) {
        outConfigBlock->maxPlayCount = 0xffff;
    }
    outConfigBlock->currentPlayCount = outConfigBlock->maxPlayCount;

    zReader::Node *nodeArray = readerNode->value.nodes;
    {
        for (int childIndex = 1; childIndex < nodeArray[0].value.i32; ++childIndex) {
            zReader::Node *childNode = &nodeArray[childIndex];
            if (childNode->type == zReader::ZRDR_NODE_ARRAY) {
                if (childIndex == 1) {
                    zSndGroup_LoadConfigBlock(
                        childNode,
                        groupFields,
                        outConfigBlock
                    );
                } else {
                    zSndGroupConfigBlock *nested =
                        (zSndGroupConfigBlock *)(calloc(
                            1,
                            sizeof(zSndGroupConfigBlock)
                        ));
                    if (nested != 0) {
                        outConfigBlock->child = nested;
                        zSndGroup_LoadConfigBlock(
                            childNode,
                            groupFields,
                            nested
                        );
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
            if (strcmp(
                key,
                g_zSnd_SoundGroupDelayPlayKey
            ) == 0) {
                if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                    outConfigBlock->delayPlaySec = valueNode->value.f32;
                    ++childIndex;
                } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                    outConfigBlock->delayPlaySec = (float)(valueNode->value.i32);
                    ++childIndex;
                } else {
                    zError::ReportOld(
                        0x200,
                        g_zSnd_SourceFile_ZsndGrpCpp,
                        0xb1,
                        g_zSnd_SoundGroupDelayPlayLoadErrorFmt,
                        groupFields->groupName
                    );
                    ++childIndex;
                }
            } else if (strcmp(
                key,
                g_zSnd_SoundGroupPlayCountKey
            ) == 0) {
                if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                    outConfigBlock->maxPlayCount =
                        (unsigned short)(valueNode->value.f32 + 0.5f);
                    outConfigBlock->currentPlayCount = outConfigBlock->maxPlayCount;
                    ++childIndex;
                } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                    outConfigBlock->maxPlayCount =
                        (unsigned short)(valueNode->value.i32);
                    outConfigBlock->currentPlayCount = outConfigBlock->maxPlayCount;
                    ++childIndex;
                } else {
                    zError::ReportOld(
                        0x200,
                        g_zSnd_SourceFile_ZsndGrpCpp,
                        0xbf,
                        g_zSnd_SoundGroupPlayCountLoadErrorFmt,
                        groupFields->groupName
                    );
                    ++childIndex;
                    outConfigBlock->currentPlayCount = outConfigBlock->maxPlayCount;
                }
            } else if (strcmp(
                key,
                g_zSnd_SoundGroupWeightKey
            ) == 0) {
                if (valueNode->type == zReader::ZRDR_NODE_FLOAT) {
                    outConfigBlock->weight = valueNode->value.f32;
                    ++childIndex;
                } else if (valueNode->type == zReader::ZRDR_NODE_INT) {
                    outConfigBlock->weight = (float)(valueNode->value.i32);
                    ++childIndex;
                } else {
                    zError::ReportOld(
                        0x200,
                        g_zSnd_SourceFile_ZsndGrpCpp,
                        0xcf,
                        g_zSnd_SoundGroupWeightLoadErrorFmt,
                        groupFields->groupName
                    );
                    ++childIndex;
                }
            } else {
                outConfigBlock->streamName = key;
            }
        }
    }

    return 1;
}

namespace zSndStreamMgr {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreammgr-updateactiverequestpredicate
 * @recoil-artifact defines .text recoil:function:0x4a4c40: zSndStreamMgr::UpdateActiveRequestPredicate.
 * Purpose: advance one active stream request and record finished requests for
 * recycling.
 */
int __fastcall UpdateActiveRequestPredicate(
    void *payload,
    void *
) {
    zSndStreamRequest *request = (zSndStreamRequest *)(payload);
    switch (request->streamState) {
    case 0:
        request->StateBeginGroup();
        break;
    case 1:
        request->StatePlayCurrentEntry();
        break;
    case 2:
        request->StateWaitRepeatDelay();
        break;
    case 3:
        request->StateWaitTerminationDelay();
        break;
    case 4:
        if (g_zSndStream_MatchedRequest == 0) {
            g_zSndStream_MatchedRequest = request;
        }
        ++g_zSndStream_MatchedRequestCount;
        break;
    default:
        break;
    }
    return 1;
}
} // namespace zSndStreamMgr

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreamrequest-statebegingroup
 * @recoil-artifact defines .text recoil:function:0x4a4cb0: zSndStreamRequest::StateBeginGroup.
 * Purpose: initialize stream-request playback state and select the first
 * playable group entry.
 */
int zSndStreamRequest::StateBeginGroup() {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndgroup-selectweightedentry
 * @recoil-artifact defines .text recoil:function:0x4a4d10: zSndGroup::SelectWeightedEntry.
 * Purpose: choose a playable config block using remaining play count and
 * weighted random selection.
 */
zSndGroupConfigBlock * zSndGroup::SelectWeightedEntry() {
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
    const float selection = (float)(rand()) * 3.05185094e-05f * totalWeight;
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreamrequest-stateplaycurrententry
 * @recoil-artifact defines .text recoil:function:0x4a4ea0: zSndStreamRequest::StatePlayCurrentEntry.
 * Purpose: play due stream entries, advance child entries, and transition to
 * repeat or termination delay.
 *
 * Uses signed play-count decrement so 0xffff remains the original infinite-play
 * sentinel.
 */
void zSndStreamRequest::StatePlayCurrentEntry() {
    elapsedSec = elapsedSec + g_FrameDeltaTimeSec;

    while (currentEntry != 0) {
        zSndGroupConfigBlock *entry = currentEntry;
        if (elapsedSec < entry->delayPlaySec) {
            break;
        }

        if (entry->currentPlayCount != 0) {
            if (entry->cachedSample == 0) {
                const char *sampleName = entry->streamName;
                if (strcmp(
                        sampleName,
                        g_zSnd_NullToken
                    ) != 0) {
                    entry->cachedSample = zSnd::FindSampleByName(sampleName);
                }
            }

            zSndSample *sample = entry->cachedSample;
            if (sample != 0) {
                if (hasWorldPos != 0) {
                    sample->PlayA3D(
                        &worldPos,
                        gain,
                        &velocity
                    );
                } else {
                    sample->PlayA3DSimple(gain);
                }
            }

            if ((short)(entry->currentPlayCount) > 0) {
                --entry->currentPlayCount;
            }
        }

        elapsedSec = 0.0f;
        currentEntry = entry->child;
    }

    if (currentEntry != 0) {
        return;
    }

    const unsigned short repeatCount = group->repeatCount;
    if (playIndex == (int)((short)(repeatCount))) {
        if (group->delayTerminationSec > 0.0f) {
            elapsedSec = 0.0f;
            streamState = 3;
        } else {
            streamState = 4;
        }
    } else {
        if (repeatCount != 0xffff) {
            ++playIndex;
        }
        streamState = 2;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreamrequest-statewaitrepeatdelay
 * @recoil-artifact defines .text recoil:function:0x4a4fd0: zSndStreamRequest::StateWaitRepeatDelay.
 * Purpose: wait for the repeat delay before selecting the next playable group
 * entry.
 */
void zSndStreamRequest::StateWaitRepeatDelay() {
    elapsedSec = elapsedSec + g_FrameDeltaTimeSec;
    if (elapsedSec < group->delayRepeatSec) {
        return;
    }

    elapsedSec = 0.0f;
    currentEntry = group->SelectWeightedEntry();
    streamState = currentEntry != 0 ? 1 : 4;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreamrequest-statewaitterminationdelay
 * @recoil-artifact defines .text recoil:function:0x4a5020: zSndStreamRequest::StateWaitTerminationDelay.
 * Purpose: wait for the termination delay before marking a stream request
 * finished.
 */
void zSndStreamRequest::StateWaitTerminationDelay() {
    elapsedSec = elapsedSec + g_FrameDeltaTimeSec;
    if (elapsedSec < group->delayTerminationSec) {
        return;
    }

    elapsedSec = 0.0f;
    streamState = 4;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreammgr-recyclefinishedrequest
 * @recoil-artifact defines .text recoil:function:0x4a5050: zSndStreamMgr::RecycleFinishedRequest.
 * Purpose: run active stream-request updates and recycle the first finished
 * request back to the free list.
 */
extern "C" void zSndStreamMgr_RecycleFinishedRequest() {
    zArchiveList_FindPayloadByPredicate(
        g_zSndStream_ActiveList,
        &zSndStreamMgr::UpdateActiveRequestPredicate,
        0
    );

    if (g_zSndStream_MatchedRequest == 0) {
        return;
    }

    zArchiveList_RemovePayload(
        g_zSndStream_ActiveList,
        g_zSndStream_MatchedRequest
    );
    zArchiveList_PushFrontPayload(
        g_zSndStream_FreeList,
        g_zSndStream_MatchedRequest
    );
    g_zSndStream_MatchedRequest = 0;
    --g_zSndStream_MatchedRequestCount;
}

namespace zSndStreamMgr {
namespace {
/**
 * Source-faithful helper recovered from address-backed callers in this source file.
 * Original helper evidence: no standalone retail function; recovered from
 * stream-manager shutdown cleanup paths.
 * Purpose: release every request payload in a stream-manager archive list.
 */
void FreeRequestList(
    zArchiveList *&list
) {
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

/**
 * Source-faithful helper recovered from address-backed callers in this source file.
 * Original helper evidence: no standalone retail function; recovered from
 * pending sound-group cleanup paths.
 * Purpose: release one pending group config and its nested child blocks.
 */
void FreePendingGroupConfig(
    zSndGroup *pendingConfig
) {
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

/**
 * Source-faithful helper recovered from address-backed callers in this source file.
 * Original helper evidence: no standalone retail function; recovered from
 * stream-manager shutdown cleanup paths.
 * Purpose: release every pending group config in a stream-manager archive list.
 */
void FreePendingList(
    zArchiveList *&list
) {
    if (list == 0) {
        return;
    }

    zSndGroup *pendingConfig = (zSndGroup *)(zArchiveList_PopFrontPayload(list));
    while (pendingConfig != 0) {
        FreePendingGroupConfig(pendingConfig);
        pendingConfig = (zSndGroup *)(zArchiveList_PopFrontPayload(list));
    }

    zArchiveList_Destroy(list);
    list = 0;
}
} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreammgr-shutdown
 * @recoil-artifact defines .text recoil:function:0x4a50a0: zSndStreamMgr::Shutdown.
 * Purpose: drain stream-manager lists, release pending stream configs, clear
 * stream-manager root/list globals, and return success.
 */
int __cdecl Shutdown() {
    if (g_zSndStream_RootNode != 0 && zClass::IsInitialized() != 0) {
        zClass_Class::gwNodeSetActionCallback(
            g_zSndStream_RootNode,
            0
        );
        zClass_Object3D::DeleteNode(g_zSndStream_RootNode);
    }
    g_zSndStream_RootNode = 0;

    FreeRequestList(g_zSndStream_ActiveList);
    FreeRequestList(g_zSndStream_FreeList);
    FreePendingList(g_zSndStream_PendingList);
    return 1;
}
} // namespace zSndStreamMgr

namespace {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreamrequest-matchrequestpredicate
 * @recoil-artifact defines .text recoil:function:0x4a51e0: zSndStreamRequest::MatchRequestPredicate.
 * Purpose: compare an active stream-list payload against the requested play
 * handle and return zero only for a match.
 */
int __fastcall MatchStreamRequestPredicate(
    void *payload,
    void *userData
) {
    return payload != userData ? 1 : 0;
}

} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreamrequest-stopifactive
 * @recoil-artifact defines .text recoil:function:0x4a51f0: zSndStreamRequest::StopIfActive.
 * Purpose: find an active stream request matching the play handle and move it
 * into the stop state.
 */
extern "C" int __fastcall zSndStreamRequest_StopIfActive(
    zSndPlayHandle *request
) {
    void *const found = zArchiveList_FindPayloadByPredicate(
        g_zSndStream_ActiveList,
        &MatchStreamRequestPredicate,
        request
    );
    if (found != 0) {
        ((zSndStreamRequest *)(request))->streamState = 4;
        return 1;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreamrequest-matchgrouppredicate
 * @recoil-artifact defines .text recoil:function:0x4a5220: zSndStreamRequest_MatchGroupPredicate.
 * Purpose: compare a queued stream request with a sound group while searching
 * active request lists.
 */
extern "C" int __fastcall zSndStreamRequest_MatchGroupPredicate(
    void *payload,
    void *group
) {
    return ((zSndStreamRequest *)(payload))->group != group ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndgroup-queuestreamrequestsimple
 * @recoil-artifact defines .text recoil:function:0x4a5230: zSndGroup::QueueStreamRequestSimple.
 * Purpose: queue a non-positional stream request for this sound group.
 */
zSndPlayHandle * zSndGroup::QueueStreamRequestSimple(
    float gain
) {
    return QueueStreamRequest(
        0,
        gain,
        0,
        0
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndgroup-queuestreamrequest
 * @recoil-artifact defines .text recoil:function:0x4a5250: zSndGroup::QueueStreamRequest.
 * Purpose: allocate or recycle a stream request, fill its group playback state,
 * and begin queued stream playback.
 */
zSndPlayHandle *__fastcall zSndGroup::QueueStreamRequest(
    int hasWorldPos,
    float gain,
    zVec3 *worldPos,
    zVec3 *velocity
) {
    if (g_zSndStream_RootNode == 0) {
        zSndStreamMgr_EnsureInit();
    }

    if (playSolo != 0 && zArchiveList_FindPayloadByPredicate(
                             g_zSndStream_ActiveList,
                             &zSndStreamRequest_MatchGroupPredicate,
                             this
                         ) != 0) {
        return 0;
    }

    zSndStreamRequest *request =
        (zSndStreamRequest *)(zArchiveList_PopFrontPayload(g_zSndStream_FreeList));
    if (request == 0) {
        request = (zSndStreamRequest *)(malloc(sizeof(zSndStreamRequest)));
        if (request == 0) {
            return 0;
        }
    }

    memset(
        request,
        0,
        sizeof(*request)
    );
    zArchiveList_PushFrontPayload(
        g_zSndStream_ActiveList,
        request
    );

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
            memset(
                &request->velocity,
                0,
                sizeof(request->velocity)
            );
        }
    } else {
        request->hasWorldPos = 0;
    }

    return request->StateBeginGroup() != 0 ? (zSndPlayHandle *)(request) : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndstreammgr-ensureinit
 * @recoil-artifact defines .text recoil:function:0x4a5350: zSndStreamMgr_EnsureInit.
 * Purpose: lazily create the stream-manager root node and request lists.
 */
extern "C" int zSndStreamMgr_EnsureInit() {
    if (g_zSndStream_RootNode == 0) {
        g_zSndStream_RootNode = zClass_Object3D::gwObject3DInit();
        if (g_zSndStream_RootNode == 0) {
            return 0;
        }

        zClass_Class::gwNodeSetActionCallbackTail(
            g_zSndStream_RootNode,
            (void *)(&zSndStreamMgr_RecycleFinishedRequest)
        );
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zsound-zsnd-grp-zsndgroup-queuestreamrequestwithworldpos
 * @recoil-artifact defines .text recoil:function:0x4a53d0: zSndGroup::QueueStreamRequestWithWorldPos.
 * Purpose: queue a positional stream request for this sound group.
 */
zSndPlayHandle *__fastcall zSndGroup::QueueStreamRequestWithWorldPos(
    zVec3 *worldPos,
    float gain,
    zVec3 *velocity
) {
    return QueueStreamRequest(
        1,
        gain,
        worldPos,
        velocity
    );
}
