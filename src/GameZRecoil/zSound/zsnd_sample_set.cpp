#include "zSound.h"

#include "GameZRecoil/zReader/zReader.h"

#include <stdlib.h>
#include <string.h>

extern "C" zSndSampleSetRegistry g_zSnd_SampleSetRegistry = {0};

namespace {
int RegistrySize() {
    if (g_zSnd_SampleSetRegistry.begin == 0) {
        return 0;
    }

    return static_cast<int>(g_zSnd_SampleSetRegistry.end - g_zSnd_SampleSetRegistry.begin);
}

int RegistryCapacity() {
    if (g_zSnd_SampleSetRegistry.begin == 0) {
        return 0;
    }

    return static_cast<int>(g_zSnd_SampleSetRegistry.capacityEnd -
                                     g_zSnd_SampleSetRegistry.begin);
}

void RegistryAppend(zSndSampleSet *set) {
    const int size = RegistrySize();
    const int capacity = RegistryCapacity();
    if (capacity - size < 1) {
        int growBy = capacity;
        if (growBy <= 1) {
            growBy = 1;
        }

        const int newCapacity = size + growBy;
        zSndSampleSet **newBegin = static_cast<zSndSampleSet **>(
            ::operator new(static_cast<size_t>(newCapacity) * sizeof(zSndSampleSet *)));

        for (int i = 0; i < size; ++i) {
            newBegin[i] = g_zSnd_SampleSetRegistry.begin[i];
        }

        ::operator delete(g_zSnd_SampleSetRegistry.begin);
        g_zSnd_SampleSetRegistry.begin = newBegin;
        g_zSnd_SampleSetRegistry.end = &newBegin[size];
        g_zSnd_SampleSetRegistry.capacityEnd = &newBegin[newCapacity];
    }

    *g_zSnd_SampleSetRegistry.end = set;
    ++g_zSnd_SampleSetRegistry.end;
}

void UpdateSampleLoadedFlag(zSndSample *sample, int initResult) {
    sample->replayFields.flags = (sample->replayFields.flags & ~0x08) | ((initResult & 1) << 3);
}

void ClearSampleLoadedFlag(zSndSample *sample) {
    sample->replayFields.flags &= ~0x08;
}

void LoadSampleFromWavePath(zSndSample *sample, const char *path) {
    zSndWaveData *waveData = static_cast<zSndWaveData *>(::operator new(sizeof(zSndWaveData)));
    waveData = waveData != 0 ? waveData->ConstructorFromPath(path, 1) : 0;

    if (waveData != 0 && waveData->parsedOk != 0) {
        UpdateSampleLoadedFlag(sample, sample->InitFromWaveData(waveData));
    } else {
        ClearSampleLoadedFlag(sample);
    }

    if (waveData != 0) {
        waveData->Destructor();
        ::operator delete(waveData);
    }
}
} // namespace

// Reimplements 0x4a0810: zSnd_SetUseArchiveBanks
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL zSnd_SetUseArchiveBanks(int enabled) {
    g_zSnd_SampleSetRegistry.useArchiveBanksFlag = enabled;
    g_zSnd_SampleSetRegistry.begin = 0;
    g_zSnd_SampleSetRegistry.end = 0;
    g_zSnd_SampleSetRegistry.capacityEnd = 0;
}

// Reimplements 0x4a0840: zSndSampleSetRegistry_Shutdown
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zSndSampleSetRegistry_Shutdown() {
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = 0;
    g_zSnd_SampleSetRegistry.end = 0;
    g_zSnd_SampleSetRegistry.capacityEnd = 0;
}

// Reimplements 0x4a0830: zSndSampleSetRegistry_RegisterAtExit
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zSndSampleSetRegistry_RegisterAtExit() {
    atexit(zSndSampleSetRegistry_Shutdown);
}

// Reimplements 0x4a0800: zSnd_SetUseArchiveBanksAndRegisterAtExit
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL
zSnd_SetUseArchiveBanksAndRegisterAtExit(int enabled) {
    zSnd_SetUseArchiveBanks(enabled);
    zSndSampleSetRegistry_RegisterAtExit();
}

// Reimplements 0x4a0900: zSndSampleSetRegistry_GetCount
extern "C" RECOIL_NOINLINE int RECOIL_CDECL zSndSampleSetRegistry_GetCount() {
    return RegistrySize();
}

// Reimplements 0x4a08d0: zSndSampleSetRegistry_GetByIndex
extern "C" RECOIL_NOINLINE zSndSampleSet *RECOIL_FASTCALL
zSndSampleSetRegistry_GetByIndex(int index) {
    if (index < 0) {
        return 0;
    }

    const int size = RegistrySize();
    if (static_cast<unsigned int>(index) < static_cast<unsigned int>(size)) {
        return g_zSnd_SampleSetRegistry.begin[index];
    }

    return 0;
}

// Reimplements 0x4a0920: zSndSampleSetRegistry_FindByName
extern "C" RECOIL_NOINLINE zSndSampleSet *RECOIL_FASTCALL
zSndSampleSetRegistry_FindByName(const char *setName) {
    for (zSndSampleSet **it = g_zSnd_SampleSetRegistry.begin; it != g_zSnd_SampleSetRegistry.end;
         ++it) {
        if (strcmp((*it)->setName, setName) == 0) {
            return *it;
        }
    }

    return 0;
}

// Reimplements 0x4a0870: zSndSampleSet_DestroyByName
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSampleSet_DestroyByName(const char *setName) {
    return zSndSampleSetRegistry_FindByName(setName)->Destroy();
}

// Reimplements 0x4a0860: zSndSampleSet_InitByName
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndSampleSet_InitByName(const char *setName) {
    return zSndSampleSetRegistry_FindByName(setName)->Init();
}

// Reimplements 0x4a09e0: zSndSampleSet::RegistryAddEntry
RECOIL_NOINLINE zSndSampleSet *RECOIL_THISCALL zSndSampleSet::RegistryAddEntry(const char *name,
                                                                               int count) {
    samples =
        static_cast<zSndSample *>(calloc(static_cast<size_t>(count), sizeof(zSndSample)));
    sampleCount = count;
    resourcesLoaded = 0;

    if (name != 0) {
        setName = _strdup(name);
    }

    RegistryAppend(this);
    return this;
}

// Reimplements 0x4a0e90: zSndSampleSet::GetSampleAt
RECOIL_NOINLINE zSndSample *RECOIL_THISCALL zSndSampleSet::GetSampleAt(int index) {
    if (this != 0 && index < sampleCount) {
        return &samples[index];
    }

    return 0;
}

// Reimplements 0x4a0ec0: zSndSampleSet::FindSampleByName
RECOIL_NOINLINE zSndSample *RECOIL_THISCALL
zSndSampleSet::FindSampleByName(const char *sampleName) {
    if (this == 0 || (g_zSnd_ActiveBackend != 0 && g_zSnd_ActiveBackend != 1)) {
        return 0;
    }

    {
    for (int index = 0; index < sampleCount; ++index) {
        zSndSample *const sample = &samples[index];
        if (strcmp(sampleName, sample->replayFields.sampleId) == 0 &&
            sample->primaryVoice.backendBuffer != 0) {
            return sample;
        }
    }
    }

    return 0;
}

// Reimplements 0x4a0fb0: zSndSampleSet::LoadSamplesFromIndexArchive
RECOIL_NOINLINE int RECOIL_THISCALL
zSndSampleSet::LoadSamplesFromIndexArchive(zIndexArchive *archive) {
    {
    for (int index = 0; index < sampleCount; ++index) {
        zSndSample *const sample = &samples[index];
        if ((sample->replayFields.flags & 0x08) != 0) {
            continue;
        }

        zSndWaveData *waveData = static_cast<zSndWaveData *>(::operator new(sizeof(zSndWaveData)));
        waveData = waveData != 0
                       ? waveData->ConstructorFromPath(sample->replayFields.resourceName, 0)
                       : 0;

        if (waveData != 0) {
            waveData->LoadAndParseFromIndexArchiveIfNeeded(archive);
        }

        if (waveData != 0 && waveData->parsedOk != 0) {
            UpdateSampleLoadedFlag(sample, sample->InitFromWaveData(waveData));
        } else {
            ClearSampleLoadedFlag(sample);
        }

        if (waveData != 0) {
            waveData->Destructor();
            ::operator delete(waveData);
        }
    }
    }

    return 1;
}

// Reimplements 0x4a0c40: zSndSampleSet::Init
RECOIL_NOINLINE int RECOIL_THISCALL zSndSampleSet::Init() {
    zIndexArchive archive;
    archive.Reset();

    if (this == 0 || resourcesLoaded != 0) {
        archive.Destroy();
        return 0;
    }

    if (g_zSnd_UseArchiveBanksFlag != 0) {
        const char *const archiveNames[3] = {"soundsH.zbd", "soundsM.zbd", "soundsL.zbd"};
        int archiveBankIndex = 0;
        if (g_zSnd_SoundLodValuePtr != 0) {
            const int soundLod = *static_cast<int *>(g_zSnd_SoundLodValuePtr);
            if (soundLod == 1) {
                archiveBankIndex = 1;
            } else if (soundLod == 2) {
                archiveBankIndex = 2;
            }
        }

        int archiveInitialized = 0;
        {
        for (int attempt = 0; attempt < 3 && archiveInitialized == 0; ++attempt) {
            const char *archivePath = archiveNames[archiveBankIndex];
            if (zReader::FileExists(archivePath) == 0) {
                const char *resolvedPath =
                    zUtil_ZRDR_ResolvePathInSearchPathList(g_zSnd_SearchPathList, archivePath);
                if (resolvedPath != 0) {
                    archivePath = resolvedPath;
                } else {
                    archivePath = 0;
                }
            }

            if (archivePath != 0) {
                archiveInitialized = archive.Init(archivePath);
            }

            if (archiveInitialized == 0) {
                ++archiveBankIndex;
                if (archiveBankIndex >= 3) {
                    archiveBankIndex = 0;
                }
            }
        }
        }

        if (archiveInitialized != 0) {
            LoadSamplesFromIndexArchive(&archive);
            archive.CloseAndFreeRecords();
        }
    }

    {
    for (int index = 0; index < sampleCount; ++index) {
        zSndSample *const sample = &samples[index];
        if ((sample->replayFields.flags & 0x08) != 0) {
            continue;
        }

        const char *const path = zUtil_ZRDR_ResolvePathInSearchPathList(
            g_zSnd_SearchPathList, sample->replayFields.resourceName);
        if (path != 0) {
            LoadSampleFromWavePath(sample, path);
        }
    }
    }

    resourcesLoaded = 1;
    archive.Destroy();
    return 1;
}

// Reimplements 0x4a0e40: zSndSampleSet::Destroy
RECOIL_NOINLINE int RECOIL_THISCALL zSndSampleSet::Destroy() {
    if (this == 0 || resourcesLoaded == 0) {
        return 0;
    }

    for (int i = 0; i < sampleCount; ++i) {
        samples[i].DestroyOwnedData();
    }
    resourcesLoaded = 0;
    return 1;
}

// Reimplements 0x4a0c00: zSndSampleSet::DestroyOwnedData
RECOIL_NOINLINE void RECOIL_THISCALL zSndSampleSet::DestroyOwnedData() {
    Destroy();
    if (samples != 0) {
        free(samples);
    }
    if (setName != 0) {
        free(setName);
    }
    sampleCount = 0;
}

// Reimplements 0x4a0880: zSndSampleSetRegistry_DestroyAll
extern "C" RECOIL_NOINLINE void RECOIL_CDECL zSndSampleSetRegistry_DestroyAll() {
    for (zSndSampleSet **it = g_zSnd_SampleSetRegistry.begin; it != g_zSnd_SampleSetRegistry.end;
         ++it) {
        zSndSampleSet *set = *it;
        if (set != 0) {
            set->DestroyOwnedData();
            ::operator delete(set);
            *it = 0;
        }
    }

    g_zSnd_SampleSetRegistry.end = g_zSnd_SampleSetRegistry.begin;
}

// Reimplements 0x4a0990: zSnd::FindSampleByName
RECOIL_NOINLINE zSndSample *RECOIL_FASTCALL zSnd::FindSampleByName(const char *sampleName) {
    if (g_zSnd_IsInitialized == 0 || sampleName == 0) {
        return 0;
    }

    for (zSndSampleSet **it = g_zSnd_SampleSetRegistry.begin; it != g_zSnd_SampleSetRegistry.end;
         ++it) {
        zSndSample *const sample = (*it)->FindSampleByName(sampleName);
        if (sample != 0) {
            return sample;
        }
    }

    return zSndPendingList_FindByName(sampleName);
}
