#include "zSound.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zSound/zA3dProvider.h"

#include <stdlib.h>
#include <string.h>

/**
 * Reimplements data 0x56b368: g_zSnd_ConfigRootNode.
 * Data owner: namespace:zSound system configuration state.
 * Purpose: hold the loaded sound configuration tree until sound shutdown.
 */
extern "C" zReader::Node *g_zSnd_ConfigRootNode = 0;
/**
 * Reimplements data 0x56b364: g_zSnd_SearchPathList.
 * Data owner: namespace:zSound system configuration state.
 * Purpose: hold the sound resource search path list built from SOUND_PATH.
 */
extern "C" zArchiveList *g_zSnd_SearchPathList = 0;
/**
 * Data owner: namespace:zSound backend runtime state.
 * Purpose: reference the active A3D or DirectSound backend device.
 */
extern "C" void *g_zSnd_BackendDevice;
/**
 * Data owner: engine.zsound.fade_list_runtime_globals.
 * Purpose: store the authored flags word for the active fade intrusive list.
 */
extern "C" unsigned int g_zSndFadeActiveListFlags = 0;
/**
 * Data owner: engine.zsound.fade_list_runtime_globals.
 * Purpose: store the sentinel node for active fades.
 */
extern "C" zSndFadeListNode *g_zSndFadeActiveListSentinel = 0;
/**
 * Data owner: engine.zsound.fade_list_runtime_globals.
 * Purpose: count active fade-list nodes.
 */
extern "C" int g_zSndFadeActiveListCount = 0;
/**
 * Data owner: engine.zsound.fade_list_runtime_globals.
 * Purpose: store the authored flags word for the dispatch fade intrusive list.
 */
extern "C" unsigned int g_zSndFadeDispatchListFlags = 0;
/**
 * Data owner: engine.zsound.fade_list_runtime_globals.
 * Purpose: store the sentinel node for fade completion dispatch.
 */
extern "C" zSndFadeListNode *g_zSndFadeDispatchListSentinel = 0;
/**
 * Data owner: engine.zsound.fade_list_runtime_globals.
 * Purpose: count dispatch fade-list nodes.
 */
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

} // namespace

namespace zSndFadeLists {
/**
 * Reimplements 0x4a3940: zSndFadeLists::InitGlobals.
 * Purpose: allocate both fade-list sentinels and reset their entry counts.
 */
void InitGlobals() {
    g_zSndFadeActiveListSentinel =
        (zSndFadeListNode *)(::operator new(sizeof(*g_zSndFadeActiveListSentinel)));
    g_zSndFadeActiveListSentinel->next = g_zSndFadeActiveListSentinel;
    g_zSndFadeActiveListSentinel->prev = g_zSndFadeActiveListSentinel;
    g_zSndFadeActiveListCount = 0;
    g_zSndFadeDispatchListSentinel =
        (zSndFadeListNode *)(::operator new(sizeof(*g_zSndFadeDispatchListSentinel)));
    g_zSndFadeDispatchListSentinel->next = g_zSndFadeDispatchListSentinel;
    g_zSndFadeDispatchListSentinel->prev = g_zSndFadeDispatchListSentinel;
    g_zSndFadeDispatchListCount = 0;
}

/**
 * Reimplements 0x4a39b0: zSndFadeLists::ShutdownAtExit.
 * Purpose: release active and dispatch fade-list nodes during sound-system
 * shutdown.
 */
void ShutdownAtExit() {
    /**
     * Reimplements data 0x56b404: g_zSndFadeDispatchListSentinel.
     * Data: g_zSndFadeDispatchListSentinel.
     * Purpose: walk and release the dispatch fade-list sentinel ring.
     */
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

    /**
     * Reimplements data 0x56b3f4: g_zSndFadeActiveListSentinel.
     * Data: g_zSndFadeActiveListSentinel.
     * Purpose: walk and release the active fade-list sentinel ring.
     */
    zSndFadeListNode *activeSentinel = g_zSndFadeActiveListSentinel;
    node = activeSentinel->next;
    while (node != activeSentinel) {
        zSndFadeListNode *const next = node->next;
        UnlinkAndDeleteFadeNode(node);
        /**
         * Reimplements data 0x56b3f8: g_zSndFadeActiveListCount.
         * Data: g_zSndFadeActiveListCount.
         * Purpose: track active fade-list node removal.
         */
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
    /**
     * Reimplements data 0x56b404: g_zSndFadeDispatchListSentinel.
     * Data: g_zSndFadeDispatchListSentinel.
     * Purpose: append completion nodes to the dispatch fade-list ring.
     */
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
    /**
     * Reimplements data 0x56b408: g_zSndFadeDispatchListCount.
     * Data: g_zSndFadeDispatchListCount.
     * Purpose: track dispatch fade-list node insertion.
     */
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

        /**
         * Reimplements data 0x56b404: g_zSndFadeDispatchListSentinel.
         * Data: g_zSndFadeDispatchListSentinel.
         * Purpose: queue completed fade entries for dispatch callbacks.
         */
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
        /**
         * Reimplements data 0x56b408: g_zSndFadeDispatchListCount.
         * Data: g_zSndFadeDispatchListCount.
         * Purpose: track completion-node insertion from active fade ticking.
         */
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
        /**
         * Reimplements data 0x56b3f4: g_zSndFadeActiveListSentinel.
         * Data: g_zSndFadeActiveListSentinel.
         * Purpose: iterate and compact the active fade-list ring.
         */
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
            /**
             * Reimplements data 0x56b3f8: g_zSndFadeActiveListCount.
             * Data: g_zSndFadeActiveListCount.
             * Purpose: track active fade-list node deletion after compaction.
             */
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

    /**
     * Reimplements data 0x56b3d0: g_zSndLastVoice.
     * Data: g_zSndLastVoice.
     * Purpose: drive marker callback state for the currently tracked voice.
     */
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

    /**
     * Reimplements data 0x56b3d8: g_zSndLastVoiceMarkerIndex.
     * Data: g_zSndLastVoiceMarkerIndex.
     * Purpose: cache the current marker index before stop/reset decisions.
     */
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
 * Reimplements 0x49f614: zSnd::TickWrapper.
 * Evidence: BN shows a 12-byte NOP range that falls through into 0x49f620,
 * and HLIL renders the wrapper as a tailcall to zSnd::Tick.
 * Purpose: preserve the retail fallthrough wrapper entry while forwarding the
 * skip-A3D-commit flag into zSnd::Tick.
 */
extern "C" void __fastcall zSnd_TickWrapper(
    int skipA3dCommit
) {
    zSnd_Tick(skipA3dCommit);
}

/**
 * Reimplements 0x4a1870: zSndSystem_InitNamedSetsSyntax.
 * Evidence: BN 0x4a1870 expands the named SETS parser in this source file,
 * using g_zSnd_ConfigRootNode and g_zSnd_SearchPathList as namespace state.
 * Purpose: load named sound sets, optional CD tracks, search paths, groups,
 * and speed-of-sound settings from the sound config tree.
 */
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
    zReader::Node *sets = setsNode->value.nodes;
    const int setCount = (sets[0].value.i32 - 1) / 2;
    for (int i = 0; i < setCount; ++i) {
        zReader::Node *setNameNode = &sets[(i * 2) + 1];
        zReader::Node *sampleListNode = &sets[(i * 2) + 2];
        const int sampleCount = sampleListNode->value.nodes[0].value.i32 - 1;
        zSndSampleSet *sampleSet =
            (zSndSampleSet *)(::operator new(sizeof(zSndSampleSet)));
        if (sampleSet != 0) {
            sampleSet = sampleSet->RegistryAddEntry(
                setNameNode->value.str,
                sampleCount
            );
        }

        zReader::Node *samples = sampleListNode->value.nodes;
        for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
            zSndSample *sample = sampleSet->GetSampleAt(sampleIndex);
            zReader::Node *sampleNode = &samples[sampleIndex + 1];
            zReader::Node *sampleFields = sampleNode->value.nodes;

            sample->createGuard = 0;
            sample->replayFields.sampleId = sampleFields[1].value.str;
            sample->replayFields.resourceName = sampleFields[2].value.str;

            if (zReader_GetNamedNode(
                sampleNode,
                "3D"
            ) != 0) {
                sample->replayFields.flags |= 0x04;
            } else {
                sample->replayFields.flags &= ~0x04;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                "LOOPED"
            ) != 0) {
                sample->replayFields.flags |= 0x01;
            } else {
                sample->replayFields.flags &= ~0x01;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                "FREQUENCY"
            ) != 0) {
                sample->replayFields.flags |= 0x20;
            } else {
                sample->replayFields.flags &= ~0x20;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                "HARDWARE"
            ) != 0) {
                sample->replayFields.flags |= 0x40;
            } else {
                sample->replayFields.flags &= ~0x40;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                "PURGEABLE"
            ) != 0) {
                sample->replayFields.flags |= 0x02;
            } else {
                sample->replayFields.flags &= ~0x02;
            }

            if (zReader_GetNamedNode(
                sampleNode,
                "VOICE"
            ) != 0) {
                sample->replayFields.flags |= 0x10;
            } else {
                sample->replayFields.flags &= ~0x10;
            }

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

            zReader::Node *rangeNode = zReader_GetNamedNode(
                sampleNode,
                "RANGE"
            );
            if (rangeNode != 0) {
                sample->replayFields.flags |= 0x04;
                zReader::Node *range = rangeNode->value.nodes;
                sample->rangeMin = range[1].value.f32;
                sample->rangeMax = range[2].value.f32;
            } else {
                sample->rangeMin = 50.0f;
                sample->rangeMax = 400.0f;
            }

            zReader::Node *variantNode = zReader_GetNamedNode(
                sampleNode,
                "HIGH"
            );
            if (variantNode == 0) {
                sample->highVariant.sampleName = 0;
                sample->highVariant.samplesPerSec = 44100;
                sample->highVariant.bitsPerSample = 16;
                sample->highVariant.channelCount = 2;
            } else if (variantNode->type == zReader::ZRDR_NODE_STRING) {
                sample->highVariant.sampleName = _strdup(variantNode->value.str);
                sample->highVariant.samplesPerSec = 0;
                sample->highVariant.bitsPerSample = 0;
                sample->highVariant.channelCount = 0;
            } else {
                zReader::Node *format = variantNode->value.nodes;
                sample->highVariant.sampleName = 0;
                sample->highVariant.samplesPerSec = format[1].value.i32;
                sample->highVariant.bitsPerSample = format[2].value.i32;
                sample->highVariant.channelCount = format[3].value.i32;
            }

            variantNode = zReader_GetNamedNode(
                sampleNode,
                "MED"
            );
            if (variantNode == 0) {
                sample->medVariant.sampleName = 0;
                sample->medVariant.samplesPerSec = 22050;
                sample->medVariant.bitsPerSample = 16;
                sample->medVariant.channelCount = 1;
            } else if (variantNode->type == zReader::ZRDR_NODE_STRING) {
                sample->medVariant.sampleName = _strdup(variantNode->value.str);
                sample->medVariant.samplesPerSec = 0;
                sample->medVariant.bitsPerSample = 0;
                sample->medVariant.channelCount = 0;
            } else {
                zReader::Node *format = variantNode->value.nodes;
                sample->medVariant.sampleName = 0;
                sample->medVariant.samplesPerSec = format[1].value.i32;
                sample->medVariant.bitsPerSample = format[2].value.i32;
                sample->medVariant.channelCount = format[3].value.i32;
            }

            variantNode = zReader_GetNamedNode(
                sampleNode,
                "LOW"
            );
            if (variantNode == 0) {
                sample->lowVariant.sampleName = 0;
                sample->lowVariant.samplesPerSec = 11025;
                sample->lowVariant.bitsPerSample = 8;
                sample->lowVariant.channelCount = 1;
            } else if (variantNode->type == zReader::ZRDR_NODE_STRING) {
                sample->lowVariant.sampleName = _strdup(variantNode->value.str);
                sample->lowVariant.samplesPerSec = 0;
                sample->lowVariant.bitsPerSample = 0;
                sample->lowVariant.channelCount = 0;
            } else {
                zReader::Node *format = variantNode->value.nodes;
                sample->lowVariant.sampleName = 0;
                sample->lowVariant.samplesPerSec = format[1].value.i32;
                sample->lowVariant.bitsPerSample = format[2].value.i32;
                sample->lowVariant.channelCount = format[3].value.i32;
            }

            sample->playbackParam3 = 20000.0f;
            sample->playbackParam2 = 90000.0f;
        }
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
    /**
     * Reimplements data 0x56b3f4: g_zSndFadeActiveListSentinel.
     * Data: g_zSndFadeActiveListSentinel.
     * Purpose: stop active fade handles before draining the active fade list.
     */
    zSndFadeListNode *activeSentinel = g_zSndFadeActiveListSentinel;
    zSndFadeListNode *node = activeSentinel->next;
    while (node != activeSentinel) {
        zSndFadeEntry *const fadeEntry = node->fadeEntry;
        fadeEntry->handle->StopIfActive();
        zSndFadeDispatchList::PushBack(fadeEntry);
        node = node->next;
    }

    /**
     * Reimplements data 0x56b3f4: g_zSndFadeActiveListSentinel.
     * Data: g_zSndFadeActiveListSentinel.
     * Purpose: reset the active sentinel cursor for destructive drain.
     */
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

    /**
     * Reimplements data 0x56b404: g_zSndFadeDispatchListSentinel.
     * Data: g_zSndFadeDispatchListSentinel.
     * Purpose: release queued completion payloads and drain dispatch nodes.
     */
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
        /**
         * Reimplements data 0x56b408: g_zSndFadeDispatchListCount.
         * Data: g_zSndFadeDispatchListCount.
         * Purpose: track dispatch fade-list node deletion during shutdown.
         */
        --g_zSndFadeDispatchListCount;
        node = next;
    }
}
} // namespace zSndFadeLists

namespace zSndSystem {
/**
 * Reimplements 0x4a13d0: zSndSystem::Shutdown.
 * Evidence: BN 0x4a13d0 calls the sound subsystem shutdown routines, then
 * frees g_zSnd_ConfigRootNode and g_zSnd_SearchPathList when present.
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
        g_zSnd_SearchPathList = zUtil_ZRDR_FreeSearchPathList(g_zSnd_SearchPathList);
    }

    return 1;
}
} // namespace zSndSystem

/**
 * Reimplements 0x4a1510: zSndSystem_InitLegacySetsSyntax.
 * Evidence: BN 0x4a1510 expands the legacy positional SETS parser in this
 * source file, using the same namespace config/search-path state as 0x4a1870.
 * Purpose: load legacy sound sets, optional CD tracks, search paths, groups,
 * and speed-of-sound settings from the sound config tree.
 */
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
    zReader::Node *sets = setsNode->value.nodes;
    const int setCount = (sets[0].value.i32 - 1) / 2;
    for (int i = 0; i < setCount; ++i) {
        zReader::Node *setNameNode = &sets[(i * 2) + 1];
        zReader::Node *sampleListNode = &sets[(i * 2) + 2];
        const int sampleCount = sampleListNode->value.nodes[0].value.i32 - 1;
        zSndSampleSet *sampleSet =
            (zSndSampleSet *)(::operator new(sizeof(zSndSampleSet)));
        if (sampleSet != 0) {
            sampleSet = sampleSet->RegistryAddEntry(
                setNameNode->value.str,
                sampleCount
            );
        }

        zReader::Node *entries = sampleListNode->value.nodes;
        for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
            zSndSample *sample = sampleSet->GetSampleAt(sampleIndex);
            zReader::Node *entryNode = &entries[sampleIndex + 1];
            zReader::Node *entry = entryNode->value.nodes;

            sample->createGuard = 0;
            sample->replayFields.sampleId = entry[1].value.str;
            sample->replayFields.resourceName = entry[2].value.str;

            int tailIndex = 3;
            if (entry[tailIndex].type == zReader::ZRDR_NODE_FLOAT) {
                sample->replayFields.gain = entry[tailIndex].value.f32;
                ++tailIndex;
            } else {
                sample->replayFields.gain = 1.0f;
            }

            if (strcmp(
                entry[tailIndex].value.str,
                "TRUE"
            ) == 0) {
                sample->replayFields.flags |= 0x01;
            } else {
                sample->replayFields.flags &= ~0x01;
            }
            ++tailIndex;

            if (strcmp(
                entry[tailIndex].value.str,
                "TRUE"
            ) == 0) {
                sample->replayFields.flags |= 0x04;
            } else {
                sample->replayFields.flags &= ~0x04;
            }
            ++tailIndex;

            if (strcmp(
                entry[tailIndex].value.str,
                "TRUE"
            ) == 0) {
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
            sample->highVariant.samplesPerSec = 44100;
            sample->highVariant.bitsPerSample = 16;
            sample->highVariant.channelCount = 2;
            sample->medVariant.samplesPerSec = 22050;
            sample->medVariant.bitsPerSample = 16;
            sample->medVariant.channelCount = 1;
            sample->lowVariant.samplesPerSec = 11025;
            sample->lowVariant.bitsPerSample = 8;
            sample->lowVariant.channelCount = 1;
        }
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
