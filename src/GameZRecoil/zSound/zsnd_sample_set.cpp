#include "zSound.h"

#include "GameZRecoil/zReader/zReader.h"

#include <stdlib.h>
#include <string.h>

extern "C" zSndSampleSetRegistry g_zSnd_SampleSetRegistry = {0};

namespace {
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x4a0900 and 0x4a08d0.
 * Purpose: Returns the active registry entry count from begin/end, or zero for an empty registry.
 */
int RegistrySize() {
    if (g_zSnd_SampleSetRegistry.begin == 0) {
        return 0;
    }

    return (int)(g_zSnd_SampleSetRegistry.end - g_zSnd_SampleSetRegistry.begin);
}

// Source-faithful helper recovered from address-backed callers in this source file.
int RegistryCapacity() {
    if (g_zSnd_SampleSetRegistry.begin == 0) {
        return 0;
    }

    return (int)(g_zSnd_SampleSetRegistry.capacityEnd - g_zSnd_SampleSetRegistry.begin);
}

// Restores likely inlined registry append helper observed in caller 0x4a09e0.
// Source-faithful helper recovered from address-backed callers in this source file.
inline void RegistryAppend(
    zSndSampleSet *set
) {
    const int size = RegistrySize();
    const int capacity = RegistryCapacity();
    if (capacity - size < 1) {
        int growBy = capacity;
        if (growBy <= 1) {
            growBy = 1;
        }

        const int newCapacity = size + growBy;
        zSndSampleSet **newBegin =
            (zSndSampleSet **)(::operator new((size_t)(newCapacity) * sizeof(zSndSampleSet *)));

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

} // namespace

// Reimplements 0x4a0810: zSnd_SetUseArchiveBanks
extern "C" void __fastcall zSnd_SetUseArchiveBanks(
    int enabled
) {
    // Binary Ninja shows 0x4a0810 writes only the selector byte before clearing the vector.
    unsigned char *const archiveBankFlag =
        (unsigned char *)(&g_zSnd_SampleSetRegistry.useArchiveBanksFlag);
    *archiveBankFlag = (unsigned char)(enabled);
    g_zSnd_SampleSetRegistry.begin = 0;
    g_zSnd_SampleSetRegistry.end = 0;
    g_zSnd_SampleSetRegistry.capacityEnd = 0;
}

// Reimplements 0x4a0840: zSndSampleSetRegistry_Shutdown
extern "C" void zSndSampleSetRegistry_Shutdown() {
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = 0;
    g_zSnd_SampleSetRegistry.end = 0;
    g_zSnd_SampleSetRegistry.capacityEnd = 0;
}

// Reimplements 0x4a0830: zSndSampleSetRegistry_RegisterAtExit
extern "C" void zSndSampleSetRegistry_RegisterAtExit() {
    atexit(zSndSampleSetRegistry_Shutdown);
}

// Reimplements 0x4a0800: zSnd_SetUseArchiveBanksAndRegisterAtExit
extern "C" void __fastcall zSnd_SetUseArchiveBanksAndRegisterAtExit(
    int enabled
) {
    zSnd_SetUseArchiveBanks(enabled);
    zSndSampleSetRegistry_RegisterAtExit();
}

/**
 * Reimplements 0x4a0900: zSndSampleSetRegistry_GetCount.
 * Purpose: Returns the number of active sample-set registry entries.
 */
extern "C" int zSndSampleSetRegistry_GetCount() {
    return RegistrySize();
}

/**
 * Reimplements 0x4a08d0: zSndSampleSetRegistry_GetByIndex.
 * Purpose: Returns the registry entry at a non-negative in-range index.
 */
extern "C" zSndSampleSet *__fastcall zSndSampleSetRegistry_GetByIndex(
    int index
) {
    if (index < 0) {
        return 0;
    }

    const int size = RegistrySize();
    if ((unsigned int)(index) < (unsigned int)(size)) {
        return g_zSnd_SampleSetRegistry.begin[index];
    }

    return 0;
}

/**
 * Reimplements 0x4a0920: zSndSampleSetRegistry_FindByName.
 * Purpose: return the registered sample set whose stored name exactly matches
 * the requested name.
 */
extern "C" zSndSampleSet *__fastcall zSndSampleSetRegistry_FindByName(
    const char *setName
) {
    zSndSampleSet **begin = g_zSnd_SampleSetRegistry.begin;
    zSndSampleSet **end = g_zSnd_SampleSetRegistry.end;
    for (zSndSampleSet **it = begin; it != end; ++it) {
        if (strcmp(
            (*it)->setName,
            setName
        ) == 0) {
            return *it;
        }
    }

    return 0;
}

/**
 * Reimplements 0x4a0870: zSndSampleSet_DestroyByName.
 * Purpose: find a registered sample set by name and dispatch its destroy routine.
 */
extern "C" int __fastcall zSndSampleSet_DestroyByName(
    const char *setName
) {
    return zSndSampleSetRegistry_FindByName(setName)->Destroy();
}

/**
 * Reimplements 0x4a0860: zSndSampleSet_InitByName.
 * Purpose: find a registered sample set by name and dispatch its
 * initialization routine.
 */
extern "C" int __fastcall zSndSampleSet_InitByName(
    const char *setName
) {
    return zSndSampleSetRegistry_FindByName(setName)->Init();
}

// Reimplements 0x4a09e0: zSndSampleSet::RegistryAddEntry
zSndSampleSet * zSndSampleSet::RegistryAddEntry(
    const char *name,
    int count
) {
    samples = (zSndSample *)(calloc(
        (size_t)(count),
        sizeof(zSndSample)
    ));
    sampleCount = count;
    resourcesLoaded = 0;

    if (name != 0) {
        setName = _strdup(name);
    }

    RegistryAppend(this);
    return this;
}

/**
 * Reimplements 0x4a0e90: zSndSampleSet::GetSampleAt.
 * Purpose: Returns the indexed sample pointer when the signed upper-bound check passes.
 */
zSndSample * zSndSampleSet::GetSampleAt(
    int index
) {
    if (this != 0 && index < sampleCount) {
        return &samples[index];
    }

    return 0;
}

/**
 * Reimplements 0x4a0ec0: zSndSampleSet::FindSampleByName.
 * Original file: GameZRecoil/zSound/zSound.cpp.
 * Purpose: find a loaded sample in this sample set by source sample id for the active backend.
 */
zSndSample * zSndSampleSet::FindSampleByName(
    const char *sampleName
) {
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

/**
 * Reimplements 0x4a0fb0: zSndSampleSet::LoadSamplesFromIndexArchive.
 * Purpose: load still-unloaded samples from the supplied index archive and
 * mirror each load result into the sample loaded flag.
 */
int zSndSampleSet::LoadSamplesFromIndexArchive(
    zIndexArchive *archive
) {
    zSndSample *sample = samples;
    int index = 0;
    if (sampleCount > 0) {
        do {
            zSndSampleReplayFields *replayFields = &sample->replayFields;
            if ((replayFields->flags & 0x08) == 0) {
                zSndWaveData *waveData = new zSndWaveData(
                    replayFields->resourceName,
                    0
                );

                waveData->LoadAndParseFromIndexArchiveIfNeeded(archive);

                if (waveData->parsedOk != 0) {
                    int initResult = sample->InitFromWaveData(waveData);
                    int flags = replayFields->flags;
                    initResult &= 1;
                    flags &= ~0x08;
                    initResult <<= 3;
                    flags |= initResult;
                    replayFields->flags = flags;
                } else {
                    replayFields->flags &= ~0x08;
                }

                if (waveData != 0) {
                    delete waveData;
                }
            }

            ++index;
            ++sample;
        } while (index < sampleCount);
    }

    return 1;
}

/**
 * Reimplements 0x4a0c40: zSndSampleSet::Init.
 * Purpose: initialize an unloaded sample set from archive banks first, then
 * from loose sample paths, and mark the set loaded.
 */
int zSndSampleSet::Init() {
    const char *const archiveNames[3] = {"soundsH.zbd", "soundsM.zbd", "soundsL.zbd"};
    int archiveBankIndex = 0;
    int archiveInitialized = 0;
    zIndexArchive archive;
    archive.Reset();

    if (this == 0 || resourcesLoaded != 0) {
        archive.Destroy();
        return 0;
    }

    if (g_zSnd_UseArchiveBanksFlag != 0) {
        if (g_zSnd_SoundLodValuePtr != 0) {
            const int soundLod = *(int *)(g_zSnd_SoundLodValuePtr);
            if (soundLod == 1) {
                archiveBankIndex = 1;
            } else if (soundLod == 2) {
                archiveBankIndex = 2;
            }
        }

        {
            for (int attempt = 0; attempt < 3 && archiveInitialized == 0; ++attempt) {
                const char *archivePath = archiveNames[archiveBankIndex];
                if (zReader::FileExists(archivePath) == 0) {
                    const char *resolvedPath =
                        zUtil_ZRDR_ResolvePathInSearchPathList(
                            g_zSnd_SearchPathList,
                            archivePath
                        );
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
            zSndSampleReplayFields *replayFields = &sample->replayFields;
            if ((replayFields->flags & 0x08) != 0) {
                continue;
            }

            const char *const path = zUtil_ZRDR_ResolvePathInSearchPathList(
                g_zSnd_SearchPathList,
                replayFields->resourceName
            );
            if (path != 0) {
                zSndWaveData *waveData = new zSndWaveData(
                    path,
                    1
                );

                if (waveData != 0 && waveData->parsedOk != 0) {
                    int initResult = sample->InitFromWaveData(waveData);
                    int flags = replayFields->flags;
                    initResult &= 1;
                    flags &= ~0x08;
                    initResult <<= 3;
                    flags |= initResult;
                    replayFields->flags = flags;
                } else {
                    replayFields->flags &= ~0x08;
                }

                if (waveData != 0) {
                    delete waveData;
                }
            }
        }
    }

    resourcesLoaded = 1;
    archive.Destroy();
    return 1;
}

/**
 * Reimplements 0x4a0e40: zSndSampleSet::Destroy.
 * Purpose: release loaded sample resources and clear the sample-set loaded flag.
 */
int zSndSampleSet::Destroy() {
    if (this == 0 || resourcesLoaded == 0) {
        return 0;
    }

    for (int i = 0; i < sampleCount; ++i) {
        samples[i].DestroyOwnedData();
    }
    resourcesLoaded = 0;
    return 1;
}

/**
 * Reimplements 0x4a0c00: zSndSampleSet::DestroyOwnedData.
 * Purpose: release owned sample storage and reset the sample count.
 */
void zSndSampleSet::DestroyOwnedData() {
    Destroy();
    if (samples != 0) {
        free(samples);
    }
    if (setName != 0) {
        free(setName);
    }
    sampleCount = 0;
}

/**
 * Reimplements 0x4a0880: zSndSampleSetRegistry_DestroyAll.
 * Purpose: destroy registered sample sets, clear their slots, and reset the active range.
 */
extern "C" void zSndSampleSetRegistry_DestroyAll() {
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

/**
 * Reimplements 0x4a0990: zSnd::FindSampleByName.
 * Original file: GameZRecoil/zSound/zSound.cpp.
 * Purpose: find a loaded sample by name across registered sample sets and pending stream groups.
 */
zSndSample *__fastcall zSnd::FindSampleByName(
    const char *sampleName
) {
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
