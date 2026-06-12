#include "zSound.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zA3dProvider.h"

#include <stdlib.h>
#include <string.h>

extern "C" zReader::Node *g_zSnd_ConfigRootNode = 0;
extern "C" zArchiveList *g_zSnd_SearchPathList = 0;
extern "C" void *g_zSnd_BackendDevice;
extern "C" unsigned int g_zSndFadeActiveListFlags = 0;
extern "C" zSndFadeListNode *g_zSndFadeActiveListSentinel = 0;
extern "C" int g_zSndFadeActiveListCount = 0;
extern "C" unsigned int g_zSndFadeDispatchListFlags = 0;
extern "C" zSndFadeListNode *g_zSndFadeDispatchListSentinel = 0;
extern "C" int g_zSndFadeDispatchListCount = 0;

namespace {
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in 0x4a39b0 and 0x4a3d20 zSnd fade-list callers.
 * Purpose: recover the authored active-fade list record from the adjacent BN
 * globals for flags, sentinel, and count.
 */
zSndFadeList *ActiveFadeList() {
    return (zSndFadeList *)(&g_zSndFadeActiveListFlags);
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in 0x4a39b0 and 0x4a3a80 zSnd fade-list callers.
 * Purpose: recover the authored dispatch-fade list record from the adjacent BN
 * globals for flags, sentinel, and count.
 */
zSndFadeList *DispatchFadeList() {
    return (zSndFadeList *)(&g_zSndFadeDispatchListFlags);
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in 0x4a39b0, 0x4a3c20, 0x4a3d20, and 0x4a3e50 fade-list cleanup
 * callers.
 * Purpose: unlink one fade-list node from its intrusive list and release its
 * storage.
 */
void UnlinkAndDeleteFadeNode(
    zSndFadeListNode *node
) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    ::operator delete(node);
}

// Source-faithful helper recovered from address-backed callers in this source file.
void InitializeSentinel(
    zSndFadeListNode *&sentinel
) {
    sentinel = (zSndFadeListNode *)(::operator new(sizeof(*sentinel)));
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
}

// Source-faithful helper recovered from address-backed callers in this source file.
zReader::Node *ArrayBase(
    zReader::Node *node
) {
    return node->value.nodes;
}

// Source-faithful helper recovered from address-backed callers in this source file.
int ArrayCount(
    zReader::Node *node
) {
    return ArrayBase(node)[0].value.i32;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void ApplyPresenceFlag(
    zSndSample *sample,
    zReader::Node *sampleNode,
    const char *name,
    int bit
) {
    if (zReader_GetNamedNode(
        sampleNode,
        name
    ) != 0) {
        sample->replayFields.flags |= bit;
    } else {
        sample->replayFields.flags &= ~bit;
    }
}

// Source-faithful helper recovered from address-backed callers in this source file.
void LoadQualityVariant(
    zSndQualityVariant *variant,
    zReader::Node *sampleNode,
    const char *name,
    int defaultRate,
    int defaultBits,
    int defaultChannels
) {
    zReader::Node *variantNode = zReader_GetNamedNode(
        sampleNode,
        name
    );
    if (variantNode == 0) {
        variant->sampleName = 0;
        variant->samplesPerSec = defaultRate;
        variant->bitsPerSample = defaultBits;
        variant->channelCount = defaultChannels;
        return;
    }

    if (variantNode->type == zReader::ZRDR_NODE_STRING) {
        variant->sampleName = _strdup(variantNode->value.str);
        variant->samplesPerSec = 0;
        variant->bitsPerSample = 0;
        variant->channelCount = 0;
        return;
    }

    zReader::Node *format = ArrayBase(variantNode);
    variant->sampleName = 0;
    variant->samplesPerSec = format[1].value.i32;
    variant->bitsPerSample = format[2].value.i32;
    variant->channelCount = format[3].value.i32;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void LoadRange(
    zSndSample *sample,
    zReader::Node *sampleNode
) {
    zReader::Node *rangeNode = zReader_GetNamedNode(
        sampleNode,
        "RANGE"
    );
    if (rangeNode == 0) {
        sample->rangeMin = 50.0f;
        sample->rangeMax = 400.0f;
        return;
    }

    sample->replayFields.flags |= 0x04;
    zReader::Node *range = ArrayBase(rangeNode);
    sample->rangeMin = range[1].value.f32;
    sample->rangeMax = range[2].value.f32;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void LoadSample(
    zSndSample *sample,
    zReader::Node *sampleNode
) {
    sample->createGuard = 0;

    zReader::Node *fields = ArrayBase(sampleNode);
    sample->replayFields.sampleId = fields[1].value.str;
    sample->replayFields.resourceName = fields[2].value.str;

    ApplyPresenceFlag(
        sample,
        sampleNode,
        "3D",
        0x04
    );
    ApplyPresenceFlag(
        sample,
        sampleNode,
        "LOOPED",
        0x01
    );
    ApplyPresenceFlag(
        sample,
        sampleNode,
        "FREQUENCY",
        0x20
    );
    ApplyPresenceFlag(
        sample,
        sampleNode,
        "HARDWARE",
        0x40
    );
    ApplyPresenceFlag(
        sample,
        sampleNode,
        "PURGEABLE",
        0x02
    );
    ApplyPresenceFlag(
        sample,
        sampleNode,
        "VOICE",
        0x10
    );

    sample->replayFields.gain = 1.0f;
    zReader::ReadNamedFloat(
        sampleNode,
        "VOLUME",
        &sample->replayFields.gain
    );
    if (sample->replayFields.gain > 1.0f) {
        sample->replayFields.gain = 1.0f;
    } else if (!(sample->replayFields.gain >= 0.0f)) {
        sample->replayFields.gain = 0.0f;
    }

    sample->a3dDistanceScale = 1.0f;
    zReader::ReadNamedFloat(
        sampleNode,
        "A3DDIST",
        &sample->a3dDistanceScale
    );
    LoadRange(
        sample,
        sampleNode
    );

    LoadQualityVariant(
        &sample->highVariant,
        sampleNode,
        "HIGH",
        44100,
        16,
        2
    );
    LoadQualityVariant(
        &sample->medVariant,
        sampleNode,
        "MED",
        22050,
        16,
        1
    );
    LoadQualityVariant(
        &sample->lowVariant,
        sampleNode,
        "LOW",
        11025,
        8,
        1
    );

    sample->playbackParam3 = 20000.0f;
    sample->playbackParam2 = 90000.0f;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void LoadSampleSet(
    const char *setName,
    zReader::Node *sampleListNode
) {
    const int sampleCount = ArrayCount(sampleListNode) - 1;
    zSndSampleSet *sampleSet = (zSndSampleSet *)(::operator new(sizeof(zSndSampleSet)));
    if (sampleSet != 0) {
        sampleSet = sampleSet->RegistryAddEntry(
            setName,
            sampleCount
        );
    }

    zReader::Node *samples = ArrayBase(sampleListNode);
    for (int i = 0; i < sampleCount; ++i) {
        zSndSample *sample = sampleSet->GetSampleAt(i);
        LoadSample(
            sample,
            &samples[i + 1]
        );
    }
}

// Source-faithful helper recovered from address-backed callers in this source file.
bool LegacyFlagIsTrue(
    zReader::Node *node
) {
    return strcmp(
        node->value.str,
        "TRUE"
    ) == 0;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void LoadLegacySample(
    zSndSample *sample,
    zReader::Node *legacyEntryNode
) {
    sample->createGuard = 0;

    zReader::Node *entry = ArrayBase(legacyEntryNode);
    sample->replayFields.sampleId = entry[1].value.str;
    sample->replayFields.resourceName = entry[2].value.str;

    int tailIndex = 3;
    if (entry[tailIndex].type == zReader::ZRDR_NODE_FLOAT) {
        sample->replayFields.gain = entry[tailIndex].value.f32;
        ++tailIndex;
    } else {
        sample->replayFields.gain = 1.0f;
    }

    if (LegacyFlagIsTrue(&entry[tailIndex])) {
        sample->replayFields.flags |= 0x01;
    } else {
        sample->replayFields.flags &= ~0x01;
    }
    ++tailIndex;

    if (LegacyFlagIsTrue(&entry[tailIndex])) {
        sample->replayFields.flags |= 0x04;
    } else {
        sample->replayFields.flags &= ~0x04;
    }
    ++tailIndex;

    if (LegacyFlagIsTrue(&entry[tailIndex])) {
        sample->replayFields.flags |= 0x02;
    } else {
        sample->replayFields.flags &= ~0x02;
    }
    ++tailIndex;

    if (entry[0].value.i32 == tailIndex + 2) {
        sample->rangeMin = entry[tailIndex].value.f32;
        sample->rangeMax = entry[tailIndex + 1].value.f32;
    } else {
        sample->rangeMin = 50.0f;
        sample->rangeMax = 400.0f;
    }

    sample->playbackParam3 = 20000.0f;
    sample->playbackParam2 = 90000.0f;
    sample->highVariant.sampleName = 0;
    sample->highVariant.samplesPerSec = 44100;
    sample->highVariant.bitsPerSample = 16;
    sample->highVariant.channelCount = 2;
    sample->medVariant.sampleName = 0;
    sample->medVariant.samplesPerSec = 22050;
    sample->medVariant.bitsPerSample = 16;
    sample->medVariant.channelCount = 1;
    sample->lowVariant.sampleName = 0;
    sample->lowVariant.samplesPerSec = 11025;
    sample->lowVariant.bitsPerSample = 8;
    sample->lowVariant.channelCount = 1;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void LoadLegacySampleSet(
    const char *setName,
    zReader::Node *sampleListNode
) {
    const int sampleCount = ArrayCount(sampleListNode) - 1;
    zSndSampleSet *sampleSet = (zSndSampleSet *)(::operator new(sizeof(zSndSampleSet)));
    if (sampleSet != 0) {
        sampleSet = sampleSet->RegistryAddEntry(
            setName,
            sampleCount
        );
    }

    zReader::Node *entries = ArrayBase(sampleListNode);
    for (int i = 0; i < sampleCount; ++i) {
        zSndSample *sample = sampleSet->GetSampleAt(i);
        LoadLegacySample(
            sample,
            &entries[i + 1]
        );
    }
}
} // namespace

namespace zSndFadeLists {
/**
 * Reimplements 0x4a3940: zSndFadeLists::InitGlobals.
 * Purpose: allocate both fade-list sentinels and reset their entry counts.
 */
void InitGlobals() {
    InitializeSentinel(g_zSndFadeActiveListSentinel);
    g_zSndFadeActiveListCount = 0;
    InitializeSentinel(g_zSndFadeDispatchListSentinel);
    g_zSndFadeDispatchListCount = 0;
}

/**
 * Reimplements 0x4a39b0: zSndFadeLists::ShutdownAtExit.
 * Purpose: release active and dispatch fade-list nodes during sound-system
 * shutdown.
 */
void ShutdownAtExit() {
    zSndFadeListNode *dispatchSentinel = g_zSndFadeDispatchListSentinel;
    zSndFadeListNode *node = dispatchSentinel->next;
    while (node != dispatchSentinel) {
        zSndFadeListNode *outCursor = 0;
        DispatchFadeList()->DeleteNodeAndAdvanceCursor(
            node,
            &outCursor
        );
        node = outCursor;
    }

    ::operator delete(dispatchSentinel);
    g_zSndFadeDispatchListSentinel = 0;
    g_zSndFadeDispatchListCount = 0;

    zSndFadeListNode *activeSentinel = g_zSndFadeActiveListSentinel;
    node = activeSentinel->next;
    while (node != activeSentinel) {
        zSndFadeListNode *const next = node->next;
        UnlinkAndDeleteFadeNode(node);
        --g_zSndFadeActiveListCount;
        node = next;
    }

    ::operator delete(activeSentinel);
    g_zSndFadeActiveListSentinel = 0;
    g_zSndFadeActiveListCount = 0;
}

/**
 * Reimplements 0x4a39a0: zSndFadeLists::RegisterShutdownAtExit.
 * Purpose: register the fade-list shutdown callback with the CRT atexit list.
 */
void RegisterShutdownAtExit() {
    atexit(ShutdownAtExit);
}

/**
 * Reimplements 0x4a3930: zSndFadeLists::Init.
 * Purpose: initialize fade-list globals and arrange their process-exit cleanup.
 */
void Init() {
    InitGlobals();
    RegisterShutdownAtExit();
}
} // namespace zSndFadeLists

namespace zSndFadeDispatchList {
/**
 * Reimplements 0x4a3a80: zSndFadeDispatchList::PushBack.
 * Purpose: append a completed fade entry to the dispatch list for completion
 * handling.
 */
void __fastcall PushBack(
    zSndFadeEntry *fadeEntry
) {
    zSndFadeListNode *const sentinel = g_zSndFadeDispatchListSentinel;
    zSndFadeListNode *const previous = sentinel->prev;
    zSndFadeListNode *const node = (zSndFadeListNode *)(::operator new(sizeof(*node)));

    node->next = sentinel != 0 ? sentinel : node;
    node->prev = previous != 0 ? previous : node;
    sentinel->prev = node;
    node->prev->next = node;
    zSndFadeEntry **const payloadSlot = &node->fadeEntry;
    if (payloadSlot != 0) {
        *payloadSlot = fadeEntry;
    }
    ++g_zSndFadeDispatchListCount;
}
} // namespace zSndFadeDispatchList

/**
 * Reimplements 0x4a3ad0: zSndFadeEntry::UpdateAndQueueCompletion.
 * Purpose: advance one fade entry toward its target, apply the backend
 * volume/gain value, and queue completed entries for dispatch.
 */
int zSndFadeEntry::TickAndMaybeDispatch(
    float deltaTime
) {
    const float direction = (targetValue - currentValue) < 0.0 ? -1.0f : 1.0f;
    const float step = direction * deltaTime * 2500.0f;
    currentValue = currentValue + step;

    switch (g_zSnd_ActiveBackend) {
    case 0: {
        if (currentValue > 0.0f) {
            currentValue = 0.0f;
        } else if (currentValue < -10000.0f) {
            currentValue = -10000.0f;
        }

        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(handle->backendBuffer);
        buffer->SetVolume((int)(currentValue));
        break;
    }
    case 1: {
        if (currentValue > 1.0) {
            currentValue = 1.0f;
        } else if (currentValue < 0.0) {
            currentValue = 0.0f;
        }

        ((zA3dProviderSource *)(handle->backendBuffer))->SetGain(
            zSndSample_PlaySimple(currentValue)
        );
        break;
    }
    }

    if (currentValue == targetValue) {
        if (stopOnComplete != 0) {
            handle->StopIfActive();
        }

        zSndFadeListNode *const sentinel = g_zSndFadeDispatchListSentinel;
        zSndFadeListNode *const previous = sentinel->prev;
        zSndFadeListNode *const node = (zSndFadeListNode *)(::operator new(sizeof(*node)));

        node->next = sentinel != 0 ? sentinel : node;
        node->prev = previous != 0 ? previous : node;
        sentinel->prev = node;
        node->prev->next = node;
        zSndFadeEntry **const payloadSlot = &node->fadeEntry;
        if (payloadSlot != 0) {
            *payloadSlot = this;
        }
        ++g_zSndFadeDispatchListCount;
        return 1;
    }
    return 0;
}

/**
 * Reimplements 0x4a3c20: zSndFadeActiveList::TickAll.
 * Purpose: tick active fades, compact unfinished entries, and delete completed
 * fade-list nodes.
 */
extern "C" void __stdcall zSndFadeActiveList_TickAll(
    float deltaTime
) {
    bool listIsEmpty = (g_zSndFadeActiveListCount == 0);
    if (!listIsEmpty) {
        zSndFadeListNode *const sentinel = g_zSndFadeActiveListSentinel;
        zSndFadeListNode *node = sentinel->next;
        int nodeIsActive = (node == sentinel);
        nodeIsActive = !nodeIsActive;
        while (nodeIsActive != 0) {
            if (node->fadeEntry->TickAndMaybeDispatch(deltaTime) != 0) {
                break;
            }
            node = node->next;
            nodeIsActive = (node == sentinel);
            nodeIsActive = !nodeIsActive;
        }

        bool nodeIsSentinel = (node == sentinel);
        if (!nodeIsSentinel) {
            zSndFadeListNode *write = node;
            zSndFadeListNode *read = node->next;
            int readIsActive = (read == sentinel);
            readIsActive = !readIsActive;
            while (readIsActive != 0) {
                if (read->fadeEntry->TickAndMaybeDispatch(deltaTime) == 0) {
                    zSndFadeEntry *const fadeEntry = read->fadeEntry;
                    zSndFadeListNode *const savedWrite = write;
                    write = write->next;
                    savedWrite->fadeEntry = fadeEntry;
                }
                read = read->next;
                readIsActive = (read == sentinel);
                readIsActive = !readIsActive;
            }
            node = write;
        }

        int deleteNode = (node == sentinel);
        deleteNode = !deleteNode;
        while (deleteNode != 0) {
            zSndFadeListNode *const deadNode = node;
            node = node->next;
            deadNode->prev->next = deadNode->next;
            deadNode->next->prev = deadNode->prev;
            ::operator delete(deadNode);
            --g_zSndFadeActiveListCount;
            deleteNode = (node == sentinel);
            deleteNode = !deleteNode;
        }
    }
}

/**
 * Reimplements 0x4a3e50: zSndFadeList::DeleteNodeAndAdvanceCursor.
 * Purpose: remove the current fade-list node, release its storage, and advance
 * the caller's cursor to the next node.
 */
void zSndFadeList::DeleteNodeAndAdvanceCursor(
    zSndFadeListNode *node,
    zSndFadeListNode **outCursor
) {
    zSndFadeListNode *const next = node->next;
    UnlinkAndDeleteFadeNode(node);
    --count;
    *outCursor = next;
}

/**
 * Reimplements 0x4a3e90: zSndFadeListCursor::PopFrontCursor.
 * Purpose: return the current cursor node and advance the cursor to the next
 * intrusive-list node.
 */
zSndFadeListNode ** zSndFadeListCursor::PopFrontCursor(
    zSndFadeListNode **outNode,
    int unused
) {
    (void)unused;

    zSndFadeListNode *const current = node;
    node = current->next;
    *outNode = current;
    return outNode;
}

/**
 * Reimplements 0x49f620: zSnd::Tick.
 * Purpose: advance backend deferred work, active fades, and the last-voice
 * marker callback timeline.
 */
extern "C" void __fastcall zSnd_Tick(
    int skipA3dCommit
) {
    if (g_zSnd_ActiveBackend == 1 && skipA3dCommit == 0) {
        ((zA3dProviderDevice *)(g_zSnd_BackendDevice))->Flush();
        ((zA3dProviderDevice *)(g_zSnd_BackendDevice))->Clear();
    }

    zSndFadeActiveList_TickAll(g_FrameDeltaTimeSec);

    zSndSample *const sample = g_zSndLastVoice;
    if (sample == 0) {
        return;
    }

    float *const markerValues = sample->markerValues;
    if (markerValues == 0) {
        g_zSndLastVoiceMarkerIndex = 0;
        g_zSndLastVoice = 0;
        g_zSndLastVoiceStopMarkerIndex = 999;
        return;
    }

    if (g_Time_UnscaledAccumulatedTimeSec < markerValues[g_zSndLastVoiceMarkerIndex]) {
        return;
    }

    sample->playbackEventHandler(g_zSndLastVoiceMarkerIndex);

    const int markerIndex = g_zSndLastVoiceMarkerIndex;
    if ((unsigned int)(markerIndex) >= (unsigned int)(g_zSndLastVoice->markerCount)) {
        g_zSndLastVoiceMarkerIndex = 0;
        g_zSndLastVoice = 0;
        g_zSndLastVoiceStopMarkerIndex = 999;
        return;
    }

    if ((unsigned int)(markerIndex) >= (unsigned int)(g_zSndLastVoiceStopMarkerIndex)) {
        g_zSndLastVoiceHandle->StopIfActive();
        g_zSndLastVoiceStopMarkerIndex = 999;
        return;
    }

    g_zSndLastVoiceMarkerIndex = markerIndex + 1;
}

/**
 * Reimplements 0x49f614: zSnd_TickWrapper.
 * Purpose: forward the skip-A3D-commit flag into zSnd::Tick through the
 * retail fallthrough wrapper entry.
 */
extern "C" void __fastcall zSnd_TickWrapper(
    int skipA3dCommit
) {
    zSnd_Tick(skipA3dCommit);
}

// Reimplements 0x4a1870: zSndSystem_InitNamedSetsSyntax
extern "C" int __fastcall zSndSystem_InitNamedSetsSyntax(
    zReader::Node *configRootNode
) {
    (void)configRootNode;

    zSndCd::Init(zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        "CD_TRACKS"
    ));

    const char *pathText = zReader::ReadNamedString(
        g_zSnd_ConfigRootNode,
        "SOUND_PATH"
    );
    if (pathText != 0) {
        if (g_zSnd_SearchPathList == 0) {
            g_zSnd_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        } else {
            zUtil::ZRDR_AddSearchPaths(
                g_zSnd_SearchPathList,
                pathText
            );
        }
    }

    float speedOfSound = 0.0f;
    if (zReader::ReadNamedFloat(
        g_zSnd_ConfigRootNode,
        "SPEED_OF_SOUND",
        &speedOfSound
    ) != 0) {
        zSnd::SetSpeedOfSoundMps(speedOfSound);
    }

    zReader::Node *setsNode = zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        "SETS"
    );
    zReader::Node *sets = ArrayBase(setsNode);
    const int setCount = (ArrayCount(setsNode) - 1) / 2;
    for (int i = 0; i < setCount; ++i) {
        zReader::Node *setNameNode = &sets[(i * 2) + 1];
        zReader::Node *sampleListNode = &sets[(i * 2) + 2];
        LoadSampleSet(
            setNameNode->value.str,
            sampleListNode
        );
    }

    zReader::Node *groupsNode = zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        "SOUND_GROUPS"
    );
    if (groupsNode != 0) {
        zSndGroup_QueuePendingLoadsFromConfigNode(groupsNode);
    }

    return 1;
}

namespace zSndFadeLists {
/**
 * Reimplements 0x4a3d20: zSndFadeLists::StopAllAndShutdown.
 * Purpose: stop active fade handles and drain both recovered fade-list records
 * during sound-system shutdown.
 */
void StopAllAndShutdown() {
    zSndFadeListNode *activeSentinel = g_zSndFadeActiveListSentinel;
    zSndFadeListNode *node = activeSentinel->next;
    while (node != activeSentinel) {
        zSndFadeEntry *const fadeEntry = node->fadeEntry;
        fadeEntry->handle->StopIfActive();
        zSndFadeDispatchList::PushBack(fadeEntry);
        node = node->next;
    }

    activeSentinel = g_zSndFadeActiveListSentinel;
    zSndFadeListCursor cursor = {activeSentinel->next};
    while (cursor.node != activeSentinel) {
        zSndFadeListNode *outNode = 0;
        zSndFadeListNode *outCursor = 0;
        ((zSndFadeList *)(&g_zSndFadeActiveListFlags))->DeleteNodeAndAdvanceCursor(
            *cursor.PopFrontCursor(
                &outNode,
                0
            ),
            &outCursor
        );
        cursor.node = outCursor;
    }

    zSndFadeListNode *const dispatchSentinel = g_zSndFadeDispatchListSentinel;
    node = dispatchSentinel->next;
    while (node != dispatchSentinel) {
        ::operator delete(node->fadeEntry);
        node->fadeEntry = 0;
        node = node->next;
    }

    node = dispatchSentinel->next;
    while (node != dispatchSentinel) {
        zSndFadeListNode *const next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        ::operator delete(node);
        --g_zSndFadeDispatchListCount;
        node = next;
    }
}
} // namespace zSndFadeLists

namespace zSndSystem {
/**
 * Reimplements 0x4a13d0: zSndSystem::Shutdown.
 * Purpose: shut down sound subsystems and release sound config/search-path
 * resources.
 */
int Shutdown() {
    zSndStreamMgr::Shutdown();
    zSndBackend::Shutdown();
    zSndCd::Shutdown();
    zSndSampleSetRegistry_DestroyAll();
    zSndFadeLists::StopAllAndShutdown();

    if (g_zSnd_ConfigRootNode != 0) {
        zReader::FreeLoadedTree(g_zSnd_ConfigRootNode);
        g_zSnd_ConfigRootNode = 0;
    }

    if (g_zSnd_SearchPathList != 0) {
        zUtil_ZRDR_FreeSearchPathList(g_zSnd_SearchPathList);
        g_zSnd_SearchPathList = 0;
    }

    return 1;
}
} // namespace zSndSystem

// Reimplements 0x4a1510: zSndSystem_InitLegacySetsSyntax
extern "C" int __fastcall zSndSystem_InitLegacySetsSyntax(
    zReader::Node *configRootNode
) {
    (void)configRootNode;

    zSndCd::Init(zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        "CD_TRACKS"
    ));

    const char *pathText = zReader::ReadNamedString(
        g_zSnd_ConfigRootNode,
        "SOUND_PATH"
    );
    if (pathText != 0) {
        if (g_zSnd_SearchPathList == 0) {
            g_zSnd_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        } else {
            zUtil::ZRDR_AddSearchPaths(
                g_zSnd_SearchPathList,
                pathText
            );
        }
    }

    float speedOfSound = 0.0f;
    if (zReader::ReadNamedFloat(
        g_zSnd_ConfigRootNode,
        "SPEED_OF_SOUND",
        &speedOfSound
    ) != 0) {
        zSnd::SetSpeedOfSoundMps(speedOfSound);
    }

    zReader::Node *setsNode = zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        "SETS"
    );
    zReader::Node *sets = ArrayBase(setsNode);
    const int setCount = (ArrayCount(setsNode) - 1) / 2;
    for (int i = 0; i < setCount; ++i) {
        zReader::Node *setNameNode = &sets[(i * 2) + 1];
        zReader::Node *sampleListNode = &sets[(i * 2) + 2];
        LoadLegacySampleSet(
            setNameNode->value.str,
            sampleListNode
        );
    }

    zReader::Node *groupsNode = zReader_GetNamedNode(
        g_zSnd_ConfigRootNode,
        "SOUND_GROUPS"
    );
    if (groupsNode != 0) {
        zSndGroup_QueuePendingLoadsFromConfigNode(groupsNode);
    }

    return 1;
}
