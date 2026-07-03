#include "zsnd.h"

#include "GameZRecoil/zReader/zreader.h"

#include <stdlib.h>
#include <string.h>

extern "C" zSndSampleSetRegistry g_zSnd_SampleSetRegistry = {0};

/**
 * Reimplements data 0x4e2238: g_zSndBankArchiveNameLow.
 * Owner data: audio_fmv archive-bank name buffer; adjacent archive-bank flag
 * at 0x4e2234 is separately owned.
 * Purpose: provide the writable low-quality sound archive bank name.
 */
char g_zSndBankArchiveNameLow[0x0c] = "soundsL.zbd";
/**
 * Reimplements data 0x4e2244: g_zSndBankArchiveNameMedium.
 * Owner data: audio_fmv archive-bank name buffer.
 * Purpose: provide the writable medium-quality sound archive bank name.
 */
char g_zSndBankArchiveNameMedium[0x0c] = "soundsM.zbd";
/**
 * Reimplements data 0x4e2250: g_zSndBankArchiveNameHigh.
 * Owner data: audio_fmv archive-bank name buffer.
 * Purpose: provide the writable high-quality sound archive bank name.
 */
char g_zSndBankArchiveNameHigh[0x0c] = "soundsH.zbd";

namespace {
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in callers 0x4a0900 and 0x4a08d0.
 * Evidence: both callers inline the same null-begin guard followed by end-minus-begin pointer count.
 * Purpose: Returns the active registry entry count from begin/end, or zero for an empty registry.
 */
int RegistrySize() {
    if (g_zSnd_SampleSetRegistry.begin == 0) {
        return 0;
    }

    return (int)(g_zSnd_SampleSetRegistry.end - g_zSnd_SampleSetRegistry.begin);
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in caller 0x4a09e0.
 * Evidence: 0x4a09e0 inlines the same null-begin guard followed by capacityEnd-minus-begin pointer count.
 * Purpose: Returns the active registry pointer capacity from begin/capacityEnd.
 */
int RegistryCapacity() {
    if (g_zSnd_SampleSetRegistry.begin == 0) {
        return 0;
    }

    return (int)(g_zSnd_SampleSetRegistry.capacityEnd - g_zSnd_SampleSetRegistry.begin);
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed in caller 0x4a09e0.
 * Evidence: BN shows the registry growth path as std::vector-style zSndSampleSet* insertion over
 * the same begin/end/capacityEnd triplet, with self stored as the inserted value.
 * Purpose: Appends a sample-set entry, growing the registry pointer array when full.
 */
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

/**
 * Reimplements 0x4a0810: zSnd_SetUseArchiveBanks.
 * Purpose: Stores the archive-bank selector flag and clears the sample-set registry range.
 */
extern "C" void __fastcall zSnd_SetUseArchiveBanks(
    unsigned char enabled
) {
    g_zSnd_SampleSetRegistry.useArchiveBanksFlag = enabled;
    g_zSnd_SampleSetRegistry.begin = 0;
    g_zSnd_SampleSetRegistry.end = 0;
    g_zSnd_SampleSetRegistry.capacityEnd = 0;
}

/**
 * Reimplements 0x4a0840: zSndSampleSetRegistry_Shutdown.
 * Purpose: Releases the sample-set registry storage and clears its pointer range.
 */
extern "C" void zSndSampleSetRegistry_Shutdown() {
    ::operator delete(g_zSnd_SampleSetRegistry.begin);
    g_zSnd_SampleSetRegistry.begin = 0;
    g_zSnd_SampleSetRegistry.end = 0;
    g_zSnd_SampleSetRegistry.capacityEnd = 0;
}

/**
 * Reimplements 0x4a0830: zSndSampleSetRegistry_RegisterAtExit.
 * Purpose: Registers the sample-set registry shutdown routine with the CRT exit list.
 */
extern "C" void zSndSampleSetRegistry_RegisterAtExit() {
    atexit(zSndSampleSetRegistry_Shutdown);
}

/**
 * Reimplements 0x4a0800: zSnd_SetUseArchiveBanksAndRegisterAtExit.
 * Purpose: Applies the archive-bank setting and registers sample-set registry cleanup.
 */
extern "C" void __fastcall zSnd_SetUseArchiveBanksAndRegisterAtExit(
    unsigned char enabled
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

/**
 * Reimplements 0x4a09e0: zSndSampleSet::RegistryAddEntry.
 * Purpose: Allocates sample entries, stores the set name, and appends this set to the registry.
 */
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
 * Original file: GameZRecoil/zSound/zsnd.cpp.
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
    const char *const archiveNames[3] = {
        g_zSndBankArchiveNameHigh,
        g_zSndBankArchiveNameMedium,
        g_zSndBankArchiveNameLow
    };
    int archiveBankIndex = 0;
    int archiveInitialized = 0;
    zIndexArchive archive;
    archive.Reset();

    if (this == 0 || resourcesLoaded != 0) {
        archive.Destroy();
        return 0;
    }

    if (g_zSnd_UseArchiveBanksFlag != 0) {
        const int soundLod = *(int *)(g_zSnd_SoundLodValuePtr);
        if (soundLod == 1) {
            archiveBankIndex = 1;
        } else if (soundLod == 2) {
            archiveBankIndex = 2;
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
        zSndSample *sample = samples;
        int index = 0;
        if (sampleCount > 0) {
            do {
                zSndSampleReplayFields *replayFields = &sample->replayFields;
                if ((replayFields->flags & 0x08) == 0) {
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

                ++index;
                ++sample;
            } while (index < sampleCount);
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
 * Original file: GameZRecoil/zSound/zsnd.cpp.
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
