#include "zSound.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zReader/zReader.h"

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
typedef int(__stdcall *BackendSetIntFn)(void *self, int value);
typedef int(__stdcall *BackendSetFloatFn)(void *self, float value);
typedef int(__stdcall *BackendSimpleFn)(void *self);

struct DirectSoundBufferVTable {
    void *slots00_38[15];
    BackendSetIntFn SetVolume;
};

struct DirectSoundBuffer {
    DirectSoundBufferVTable *vtable;
};

struct A3dSourceVTable {
    void *slots00_9c[40];
    BackendSetFloatFn SetGain;
};

struct A3dSource {
    A3dSourceVTable *vtable;
};

struct TickBackendDeviceVTable {
    void *slots00_2c[12];
    BackendSimpleFn Tick;
    BackendSimpleFn CommitDeferredSettings;
};

struct TickBackendDevice {
    TickBackendDeviceVTable *vtable;
};

zSndFadeList *ActiveFadeList() {
    return reinterpret_cast<zSndFadeList *>(&g_zSndFadeActiveListFlags);
}

void UnlinkAndDeleteFadeNode(zSndFadeListNode *node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
    ::operator delete(node);
}

void InitializeSentinel(zSndFadeListNode *&sentinel) {
    sentinel = static_cast<zSndFadeListNode *>(::operator new(sizeof(*sentinel)));
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    sentinel->fadeEntry = 0;
}

zReader::Node *ArrayBase(zReader::Node *node) {
    return node->value.nodes;
}

int ArrayCount(zReader::Node *node) {
    return ArrayBase(node)[0].value.i32;
}

void ApplyPresenceFlag(zSndSample *sample, zReader::Node *sampleNode, const char *name,
                       int bit) {
    if (zReader_GetNamedNode(sampleNode, name) != 0) {
        sample->replayFields.flags |= bit;
    } else {
        sample->replayFields.flags &= ~bit;
    }
}

void LoadQualityVariant(zSndQualityVariant *variant, zReader::Node *sampleNode, const char *name,
                        int defaultRate, int defaultBits,
                        int defaultChannels) {
    zReader::Node *variantNode = zReader_GetNamedNode(sampleNode, name);
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

void LoadRange(zSndSample *sample, zReader::Node *sampleNode) {
    zReader::Node *rangeNode = zReader_GetNamedNode(sampleNode, "RANGE");
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

void LoadSample(zSndSample *sample, zReader::Node *sampleNode) {
    sample->createGuard = 0;

    zReader::Node *fields = ArrayBase(sampleNode);
    sample->replayFields.sampleId = fields[1].value.str;
    sample->replayFields.resourceName = fields[2].value.str;

    ApplyPresenceFlag(sample, sampleNode, "3D", 0x04);
    ApplyPresenceFlag(sample, sampleNode, "LOOPED", 0x01);
    ApplyPresenceFlag(sample, sampleNode, "FREQUENCY", 0x20);
    ApplyPresenceFlag(sample, sampleNode, "HARDWARE", 0x40);
    ApplyPresenceFlag(sample, sampleNode, "PURGEABLE", 0x02);
    ApplyPresenceFlag(sample, sampleNode, "VOICE", 0x10);

    sample->replayFields.gain = 1.0f;
    zReader::ReadNamedFloat(sampleNode, "VOLUME", &sample->replayFields.gain);
    if (sample->replayFields.gain > 1.0f) {
        sample->replayFields.gain = 1.0f;
    } else if (!(sample->replayFields.gain >= 0.0f)) {
        sample->replayFields.gain = 0.0f;
    }

    sample->a3dDistanceScale = 1.0f;
    zReader::ReadNamedFloat(sampleNode, "A3DDIST", &sample->a3dDistanceScale);
    LoadRange(sample, sampleNode);

    LoadQualityVariant(&sample->highVariant, sampleNode, "HIGH", 44100, 16, 2);
    LoadQualityVariant(&sample->medVariant, sampleNode, "MED", 22050, 16, 1);
    LoadQualityVariant(&sample->lowVariant, sampleNode, "LOW", 11025, 8, 1);

    sample->playbackParam3 = 20000.0f;
    sample->playbackParam2 = 90000.0f;
}

void LoadSampleSet(const char *setName, zReader::Node *sampleListNode) {
    const int sampleCount = ArrayCount(sampleListNode) - 1;
    zSndSampleSet *sampleSet = static_cast<zSndSampleSet *>(::operator new(sizeof(zSndSampleSet)));
    if (sampleSet != 0) {
        sampleSet = sampleSet->RegistryAddEntry(setName, sampleCount);
    }

    zReader::Node *samples = ArrayBase(sampleListNode);
    for (int i = 0; i < sampleCount; ++i) {
        zSndSample *sample = sampleSet->GetSampleAt(i);
        LoadSample(sample, &samples[i + 1]);
    }
}

bool LegacyFlagIsTrue(zReader::Node *node) {
    return strcmp(node->value.str, "TRUE") == 0;
}

void LoadLegacySample(zSndSample *sample, zReader::Node *legacyEntryNode) {
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

void LoadLegacySampleSet(const char *setName, zReader::Node *sampleListNode) {
    const int sampleCount = ArrayCount(sampleListNode) - 1;
    zSndSampleSet *sampleSet = static_cast<zSndSampleSet *>(::operator new(sizeof(zSndSampleSet)));
    if (sampleSet != 0) {
        sampleSet = sampleSet->RegistryAddEntry(setName, sampleCount);
    }

    zReader::Node *entries = ArrayBase(sampleListNode);
    for (int i = 0; i < sampleCount; ++i) {
        zSndSample *sample = sampleSet->GetSampleAt(i);
        LoadLegacySample(sample, &entries[i + 1]);
    }
}
} // namespace

namespace zSndFadeLists {
RECOIL_NOINLINE void RECOIL_CDECL InitGlobals() {
    InitializeSentinel(g_zSndFadeActiveListSentinel);
    g_zSndFadeActiveListCount = 0;
    InitializeSentinel(g_zSndFadeDispatchListSentinel);
    g_zSndFadeDispatchListCount = 0;
}
} // namespace zSndFadeLists

namespace zSndFadeDispatchList {
// Reimplements 0x4a3a80: zSndFadeDispatchList::PushBack
RECOIL_NOINLINE void RECOIL_FASTCALL PushBack(zSndFadeEntry *fadeEntry) {
    zSndFadeListNode *const sentinel = g_zSndFadeDispatchListSentinel;
    zSndFadeListNode *const previous = sentinel->prev;
    zSndFadeListNode *const node = static_cast<zSndFadeListNode *>(::operator new(sizeof(*node)));

    node->next = sentinel != 0 ? sentinel : node;
    node->prev = previous != 0 ? previous : node;
    sentinel->prev = node;
    node->prev->next = node;
    node->fadeEntry = fadeEntry;
    ++g_zSndFadeDispatchListCount;
}
} // namespace zSndFadeDispatchList

// Reimplements 0x4a3ad0: zSndFadeEntry::TickAndMaybeDispatch
RECOIL_NOINLINE int RECOIL_THISCALL zSndFadeEntry::TickAndMaybeDispatch(float deltaTime) {
    const float direction = (targetValue - currentValue) < 0.0f ? -1.0f : 1.0f;
    currentValue += direction * deltaTime * 2500.0f;

    if (g_zSnd_ActiveBackend == 0) {
        if (currentValue >= 0.0f) {
            currentValue = 0.0f;
        } else if (currentValue < -10000.0f) {
            currentValue = -10000.0f;
        }

        DirectSoundBuffer *const buffer = reinterpret_cast<DirectSoundBuffer *>(handle->backendBuffer);
        buffer->vtable->SetVolume(buffer, static_cast<int>(currentValue));
    } else if (g_zSnd_ActiveBackend == 1) {
        if (currentValue > 1.0f) {
            currentValue = 1.0f;
        } else if (currentValue < 0.0f) {
            currentValue = 0.0f;
        }

        A3dSource *const source = reinterpret_cast<A3dSource *>(handle->backendBuffer);
        source->vtable->SetGain(source, zSndSample_PlaySimple(currentValue));
    }

    if (currentValue != targetValue) {
        return 0;
    }

    if (stopOnComplete != 0) {
        handle->StopIfActive();
    }

    zSndFadeDispatchList::PushBack(this);
    return 1;
}

// Reimplements 0x4a3c20: zSndFadeActiveList_TickAll
extern "C" RECOIL_NOINLINE void RECOIL_STDCALL zSndFadeActiveList_TickAll(float deltaTime) {
    if (g_zSndFadeActiveListCount == 0) {
        return;
    }

    zSndFadeListNode *const sentinel = g_zSndFadeActiveListSentinel;
    zSndFadeListNode *node = sentinel->next;
    while (node != sentinel && node->fadeEntry->TickAndMaybeDispatch(deltaTime) == 0) {
        node = node->next;
    }

    if (node != sentinel) {
        zSndFadeListNode *write = node;
        zSndFadeListNode *read = node->next;
        while (read != sentinel) {
            if (read->fadeEntry->TickAndMaybeDispatch(deltaTime) == 0) {
                write->fadeEntry = read->fadeEntry;
                write = write->next;
            }
            read = read->next;
        }
        node = write;
    }

    while (node != sentinel) {
        zSndFadeListNode *const next = node->next;
        UnlinkAndDeleteFadeNode(node);
        --g_zSndFadeActiveListCount;
        node = next;
    }
}

// Reimplements 0x4a3e50: zSndFadeList::DeleteNodeAndAdvanceCursor
RECOIL_NOINLINE void RECOIL_THISCALL
zSndFadeList::DeleteNodeAndAdvanceCursor(zSndFadeListNode *node, zSndFadeListNode **outCursor) {
    zSndFadeListNode *const next = node->next;
    UnlinkAndDeleteFadeNode(node);
    --count;
    *outCursor = next;
}

// Reimplements 0x4a3e90: zSndFadeListCursor::PopFrontCursor
RECOIL_NOINLINE zSndFadeListNode **RECOIL_THISCALL
zSndFadeListCursor::PopFrontCursor(zSndFadeListNode **outNode, int unused) {
    (void)unused;

    zSndFadeListNode *const current = node;
    node = current->next;
    *outNode = current;
    return outNode;
}

// Reimplements 0x49f620: zSnd_Tick
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zSnd_Tick(int skipA3dCommit) {
    if (g_zSnd_ActiveBackend == 1 && skipA3dCommit == 0) {
        TickBackendDevice *const device = reinterpret_cast<TickBackendDevice *>(g_zSnd_BackendDevice);
        device->vtable->CommitDeferredSettings(device);
        device->vtable->Tick(device);
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
    if (static_cast<unsigned int>(markerIndex) >=
        static_cast<unsigned int>(g_zSndLastVoice->markerCount)) {
        g_zSndLastVoiceMarkerIndex = 0;
        g_zSndLastVoice = 0;
        g_zSndLastVoiceStopMarkerIndex = 999;
        return;
    }

    if (static_cast<unsigned int>(markerIndex) >=
        static_cast<unsigned int>(g_zSndLastVoiceStopMarkerIndex)) {
        g_zSndLastVoiceHandle->StopIfActive();
        g_zSndLastVoiceStopMarkerIndex = 999;
        return;
    }

    g_zSndLastVoiceMarkerIndex = markerIndex + 1;
}

// Reimplements 0x49f614: zSnd_TickWrapper
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zSnd_TickWrapper(int skipA3dCommit) {
    zSnd_Tick(skipA3dCommit);
}

// Reimplements 0x4a1870: zSndSystem_InitNamedSetsSyntax
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSystem_InitNamedSetsSyntax(zReader::Node *configRootNode) {
    (void)configRootNode;

    zSndCd::Init(zReader_GetNamedNode(g_zSnd_ConfigRootNode, "CD_TRACKS"));

    const char *pathText = zReader::ReadNamedString(g_zSnd_ConfigRootNode, "SOUND_PATH");
    if (pathText != 0) {
        if (g_zSnd_SearchPathList == 0) {
            g_zSnd_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        } else {
            zUtil::ZRDR_AddSearchPaths(g_zSnd_SearchPathList, pathText);
        }
    }

    float speedOfSound = 0.0f;
    if (zReader::ReadNamedFloat(g_zSnd_ConfigRootNode, "SPEED_OF_SOUND", &speedOfSound) != 0) {
        zSnd::SetSpeedOfSoundMps(speedOfSound);
    }

    zReader::Node *setsNode = zReader_GetNamedNode(g_zSnd_ConfigRootNode, "SETS");
    zReader::Node *sets = ArrayBase(setsNode);
    const int setCount = (ArrayCount(setsNode) - 1) / 2;
    for (int i = 0; i < setCount; ++i) {
        zReader::Node *setNameNode = &sets[(i * 2) + 1];
        zReader::Node *sampleListNode = &sets[(i * 2) + 2];
        LoadSampleSet(setNameNode->value.str, sampleListNode);
    }

    zReader::Node *groupsNode = zReader_GetNamedNode(g_zSnd_ConfigRootNode, "SOUND_GROUPS");
    if (groupsNode != 0) {
        zSndGroup_QueuePendingLoadsFromConfigNode(groupsNode);
    }

    return 1;
}

namespace zSndFadeLists {
// Reimplements 0x4a3d20: zSndFadeLists::StopAllAndShutdown
RECOIL_NOINLINE void RECOIL_CDECL StopAllAndShutdown() {
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
        ActiveFadeList()->DeleteNodeAndAdvanceCursor(*cursor.PopFrontCursor(&outNode, 0),
                                                     &outCursor);
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
        UnlinkAndDeleteFadeNode(node);
        --g_zSndFadeDispatchListCount;
        node = next;
    }
}
} // namespace zSndFadeLists

namespace zSndSystem {
// Reimplements 0x4a13d0: zSndSystem::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
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
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSystem_InitLegacySetsSyntax(zReader::Node *configRootNode) {
    (void)configRootNode;

    zSndCd::Init(zReader_GetNamedNode(g_zSnd_ConfigRootNode, "CD_TRACKS"));

    const char *pathText = zReader::ReadNamedString(g_zSnd_ConfigRootNode, "SOUND_PATH");
    if (pathText != 0) {
        if (g_zSnd_SearchPathList == 0) {
            g_zSnd_SearchPathList = zUtil_ZRDR_CreateSearchPathList(pathText);
        } else {
            zUtil::ZRDR_AddSearchPaths(g_zSnd_SearchPathList, pathText);
        }
    }

    float speedOfSound = 0.0f;
    if (zReader::ReadNamedFloat(g_zSnd_ConfigRootNode, "SPEED_OF_SOUND", &speedOfSound) != 0) {
        zSnd::SetSpeedOfSoundMps(speedOfSound);
    }

    zReader::Node *setsNode = zReader_GetNamedNode(g_zSnd_ConfigRootNode, "SETS");
    zReader::Node *sets = ArrayBase(setsNode);
    const int setCount = (ArrayCount(setsNode) - 1) / 2;
    for (int i = 0; i < setCount; ++i) {
        zReader::Node *setNameNode = &sets[(i * 2) + 1];
        zReader::Node *sampleListNode = &sets[(i * 2) + 2];
        LoadLegacySampleSet(setNameNode->value.str, sampleListNode);
    }

    zReader::Node *groupsNode = zReader_GetNamedNode(g_zSnd_ConfigRootNode, "SOUND_GROUPS");
    if (groupsNode != 0) {
        zSndGroup_QueuePendingLoadsFromConfigNode(groupsNode);
    }

    return 1;
}
