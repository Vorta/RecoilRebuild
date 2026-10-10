#include "GameZRecoil/zClass/cls_api.h"
#include "GameZRecoil/zEffect/zeff.h"

#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zutil.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* zReader and zUtil ZBD entry points; zreader.h and zbd.h declare them for the C++ units only. */
int __fastcall zArchiveListAddTail(zArchiveList* list, void* payload);
int __fastcall zArchiveListRemove(zArchiveList* list, void* payload);
void* __fastcall zArchiveListFindKey(zArchiveList* list, unsigned int value);
FILE* __cdecl OpenTempWriteStream(void);
FILE* __fastcall OpenTempReadStream(void* buffer, unsigned int size);
void __fastcall
FlushTempWriteStreamToSectionRecord(FILE* tempStream, zZbdSectionCallbackCtx* callbackCtx, const char* sectionToken);
void __fastcall CloseTempReadStream(FILE* tempStream);

enum {
    kInitialActivationRecordCapacity = 1000,
    kActivationRecordNoQueueDispatchFlag = 0x00000100u,
    kActivationRecordQueueOverrideFlag = 0x00001000u,
    kActivationRecordQueueOverrideValue = 0x00002000u,
    kActivationRecordDispatchOverrideFlag = 0x00000400u,
    kActivationRecordDispatchOverrideValue = 0x00000800u
};

typedef struct zEffectAnimTrackedNodeSaveRecord {
    int nodeIndex;
    int activeFlag;
    int usesCachedMatrix;
    float transform[12];
    int diFlagBits;
    int diUserValue;
} zEffectAnimTrackedNodeSaveRecord;

typedef struct zEffectAnimActivationSaveRecord {
    zEffectAnimActivationRecord base;
    unsigned char unknown_50[4];
    unsigned char savedActivationState;
    unsigned char trackedNodeCount;
    unsigned char unknown_56[2];
    zEffectAnimTrackedNodeSaveRecord trackedNodes[1];
} zEffectAnimActivationSaveRecord;

typedef struct zEffectAnimSaveHeader {
    zEffectAnimActivationRecord base;
    int entryTableIndex;
    unsigned char savedActivationState;
    unsigned char trackedNodeCount;
    unsigned char unknown_56[2];
} zEffectAnimSaveHeader;

enum { kMaxEffectAnimTrackedNodeSaveCount = 256 };

typedef struct zEffectAnimSaveRecord {
    zEffectAnimSaveHeader header;
    zEffectAnimTrackedNodeSaveRecord trackedNodes[kMaxEffectAnimTrackedNodeSaveCount];
} zEffectAnimSaveRecord;

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimTrackedNodeSaveRecord) == 0x44);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationSaveRecord, savedActivationState) == 0x54);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationSaveRecord, trackedNodeCount) == 0x55);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimActivationSaveRecord, trackedNodes) == 0x58);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSaveHeader) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSaveRecord, trackedNodes) == 0x58);
enum {
    kMaxActivationSaveRecordSize = offsetof(zEffectAnimActivationSaveRecord, trackedNodes)
    + sizeof(zEffectAnimTrackedNodeSaveRecord) * kMaxEffectAnimTrackedNodeSaveCount
};

typedef struct zEffectAnimRunningSaveHeader {
    int entryTableIndex;
    int matchSavedRootNode;
    char entryName[0x20];
    int rootNodeIndex;
    int nodeRefAIndex;
    zVec3 refVecA;
    int nodeRefBIndex;
    zVec3 refVecB;
    unsigned char activationState;
    unsigned char unknown_4d[3];
    float triggerCurrentValue;
    float activationCountdown;
    zVec3 velocity;
    unsigned char runtimeSurfaceCount;
    unsigned char lightRefCount;
    unsigned char soundRefCount;
    unsigned char reserved;
} zEffectAnimRunningSaveHeader;

typedef struct zEffectAnimRuntimeNodeSaveRecord {
    char name[0x24];
    int isAttached;
    float posX;
    float posY;
    float posZ;
    int parentNodeIndex;
} zEffectAnimRuntimeNodeSaveRecord;

typedef struct zEffectAnimSoundNodeSaveRecord {
    char name[0x24];
    int isAttached;
    int hasPosition;
    float posX;
    float posY;
    float posZ;
    int parentNodeIndex;
} zEffectAnimSoundNodeSaveRecord;

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRunningSaveHeader) == 0x68);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRuntimeNodeSaveRecord) == 0x38);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSoundNodeSaveRecord) == 0x3c);

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.clearactivationrecords
 * @recoil-artifact defines .text recoil:function:0x4603d0: zEffect_Anim::ClearActivationRecords.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: release the queued activation-record table and reset the record count.
 */
void __fastcall ClearActivationRecords(void)
{
    if (g_zEffectAnim_ActivationRecordTable != 0) {
        free(g_zEffectAnim_ActivationRecordTable);
        g_zEffectAnim_ActivationRecordTable = 0;
    }

    g_zEffectAnim_ActivationRecordCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.hasactivationrecord
 * @recoil-artifact defines .text recoil:function:0x460400: zEffect_Anim::HasActivationRecord.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: report whether a queued activation record already targets the same
 * animation name and node token.
 */
int __fastcall HasActivationRecord(zEffectAnimActivationRecord* record)
{
    int i;
    for (i = 0; i < g_zEffectAnim_ActivationRecordCount; ++i) {
        zEffectAnimActivationRecord* const queuedRecord = &g_zEffectAnim_ActivationRecordTable[i];
        if (queuedRecord->nodeToken == record->nodeToken
            && strncmp(queuedRecord->animName, record->animName, sizeof(record->animName)) == 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.getactivationrecordcount
 * @recoil-artifact defines .text recoil:function:0x460470: zEffect_Anim::GetActivationRecordCount.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: return the current number of queued activation records.
 */
int __cdecl GetActivationRecordCount(void)
{
    return g_zEffectAnim_ActivationRecordCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.getactivationrecordat
 * @recoil-artifact defines .text recoil:function:0x460480: zEffect_Anim::GetActivationRecordAt.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: return the queued activation record at the requested table index.
 */
zEffectAnimActivationRecord* __fastcall GetActivationRecordAt(int index)
{
    return &g_zEffectAnim_ActivationRecordTable[index];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.saveactivationrecords
 * @recoil-artifact defines .text recoil:function:0x460490: zEffect_Anim::SaveActivationRecords.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: serialize queued activation records with tracked-node state into
 * numbered ZAR activation sections.
 */
int __fastcall SaveActivationRecords(zZbdSectionCallbackCtx* callbackCtx)
{
    int result = 1;
    int i;
    for (i = 0; i < g_zEffectAnim_ActivationRecordCount && result != 0; ++i) {
        zEffectAnimEntry* const entry = FindEntryByName(g_zEffectAnim_ActivationRecordTable[i].animName);
        zEffectAnimActivationRecord* const sourceRecord = &g_zEffectAnim_ActivationRecordTable[i];
        unsigned char saveRecordStorage[kMaxActivationSaveRecordSize];
        zEffectAnimActivationSaveRecord* const saveRecord = (zEffectAnimActivationSaveRecord*)(saveRecordStorage);
        char sectionName[0x14];

        memcpy(&saveRecord->base, sourceRecord, sizeof(*sourceRecord));
        if (entry != 0) {
            CZNodePartial* const rootNode = NodeIndexToPtr(sourceRecord->nodeToken);
            saveRecord->savedActivationState = entry->activationState;
            RebindEntryToNode(entry, rootNode);
            if (entry->trackedNodeList != 0) {
                int j;
                saveRecord->trackedNodeCount = entry->trackedNodeCount;
                for (j = 0; j < entry->trackedNodeCount; ++j) {
                    CZNodePartial* const node = entry->trackedNodeList[j].trackedNode;
                    zEffectAnimTrackedNodeSaveRecord* const savedTracked = &saveRecord->trackedNodes[j];
                    CZObject3DDataPartial* objectData;
                    if (node == 0 || node->classId != 5) {
                        savedTracked->nodeIndex = -1;
                        continue;
                    }

                    objectData = (CZObject3DDataPartial*)(node->classData);
                    savedTracked->nodeIndex = NodePtrToValidatedIndex(node);
                    savedTracked->activeFlag = ((unsigned int)(node->flags) >> 2) & 1;
                    savedTracked->usesCachedMatrix = ((unsigned int)(objectData->flags) >> 4) & 1;
                    if (savedTracked->usesCachedMatrix != 0) {
                        memcpy(savedTracked->transform, gwObject3DGetMatrixPtr(node), sizeof(savedTracked->transform));
                    } else {
                        gwObject3DGetPosition(
                            node,
                            &savedTracked->transform[0],
                            &savedTracked->transform[1],
                            &savedTracked->transform[2]
                        );
                        gwObject3DGetRotation(
                            node,
                            &savedTracked->transform[3],
                            &savedTracked->transform[4],
                            &savedTracked->transform[5]
                        );
                        gwObject3DGetScale(
                            node,
                            &savedTracked->transform[6],
                            &savedTracked->transform[7],
                            &savedTracked->transform[8]
                        );
                    }

                    if (node->userDataOrDiRef != 0) {
                        savedTracked->diFlagBits = (savedTracked->diFlagBits & ~1)
                            | ((((unsigned int*)(node->userDataOrDiRef))[1] >> 3) & 1);
                        savedTracked->diUserValue = (int)(((unsigned int*)(node->userDataOrDiRef))[8]);
                    } else {
                        savedTracked->diFlagBits &= ~1;
                    }
                }
            } else {
                saveRecord->trackedNodeCount = 0;
            }
        } else {
            saveRecord->savedActivationState = 0;
            saveRecord->trackedNodeCount = 0;
        }

        sprintf(sectionName, g_zEffectAnim_ActivationSectionNameFmt, i);
        result = WriteSectionBlob(
            callbackCtx,
            sectionName,
            saveRecord,
            offsetof(zEffectAnimActivationSaveRecord, trackedNodes)
                + sizeof(zEffectAnimTrackedNodeSaveRecord) * saveRecord->trackedNodeCount
        );
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.loadactivationrecords
 * @recoil-artifact defines .text recoil:function:0x4606d0: zEffect_Anim::LoadActivationRecords.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: restore queued activation records, activation states, tracked-node
 * transforms, and deferred record queue state from ZAR activation sections.
 */
void __fastcall LoadActivationRecords(void* unused, const char* sectionToken, void* data, int dataSize, void* extraCtx)
{
    zEffectAnimActivationSaveRecord* const record = (zEffectAnimActivationSaveRecord*)(data);
    zEffectAnimEntry* entry = FindEntryByName(record->base.animName);
    int i;

    if (strcmp(sectionToken, g_zEffectAnim_ActivationSectionName0) == 0) {
        int i_2482;
        for (i = 0; i < GetActivationRecordCount(); ++i) {
            zEffectAnimActivationRecord* const queued = GetActivationRecordAt(i);
            zEffResetActivationRecord(queued);
            ReportOld(
                0x100,
                g_zEffect_SourceFile_ZeffAnimSaveC,
                0x190,
                g_zEffectAnim_ResetActivationRecordFmt,
                queued->animName
            );
        }

        ClearActivationRecords();
        for (i_2482 = 1; i_2482 < g_zEffectAnim_State.entryCount; ++i_2482) {
            zEffectAnimEntry* cursor = &g_zEffectAnim_State.entryList[i_2482];
            while (cursor != 0) {
                cursor->flags &= ~0x4000u;
                cursor = cursor->runtimeSibling;
            }
        }
    }

    if (record->base.nodeToken >= 0) {
        CZNodePartial* const rootNode = NodeIndexToPtr(record->base.nodeToken);
        while (entry->runtimeSibling != 0 && entry->boundNode != rootNode) {
            entry = entry->runtimeSibling;
        }

        zEffAnimReset(entry, rootNode);
        entry = zEffProcessActivationRecord(&record->base);
    }

    if (entry == 0) {
        return;
    }

    entry->flags |= 0x4000u;
    if (record->savedActivationState == 2) {
        if (entry->activationState != 2) {
            zEffAnimReset(entry, NodeIndexToPtr(record->base.nodeToken));
            entry = zEffProcessActivationRecord(&record->base);
            ReportOld(0x100, g_zEffect_SourceFile_ZeffAnimSaveC, 0x1b6, g_zEffectAnim_ProcessActivationRecordName);
        } else if (record->base.nodeToken == -1) {
            memcpy(AllocActivationRecord(), &record->base, sizeof(zEffectAnimActivationRecord));
        }
    }

    if (record->savedActivationState == 3) {
        if (entry->activationState != 3) {
            Stop(entry);
            entry->activationState = 3;
        }
        if (record->base.nodeToken == -1) {
            memcpy(AllocActivationRecord(), &record->base, sizeof(zEffectAnimActivationRecord));
        }
        ReportOld(0x100, g_zEffect_SourceFile_ZeffAnimSaveC, 0x1c9, g_zEffectAnim_StateExecutedMsg);
    }

    if (record->savedActivationState == 1) {
        if (entry->activationState != 1) {
            CZNodePartial* const node = NodeIndexToPtr(record->base.nodeToken);
            if (entry->activationState == 4) {
                entry->activationState = 3;
            }
            zEffAnimReset(entry, node);
            ReportOld(0x100, g_zEffect_SourceFile_ZeffAnimSaveC, 0x1d3, g_zEffectAnim_ResetFunctionName);
        }
        if (record->base.nodeToken == -1) {
            memcpy(AllocActivationRecord(), &record->base, sizeof(zEffectAnimActivationRecord));
        }
    }

    if (record->savedActivationState == 6) {
        if (entry->activationState != 6) {
            CZNodePartial* const node = NodeIndexToPtr(record->base.nodeToken);
            if (entry->activationState == 4) {
                entry->activationState = 3;
                zEffAnimReset(entry, node);
                entry = zEffProcessActivationRecord(&record->base);
                ReportOld(0x100, g_zEffect_SourceFile_ZeffAnimSaveC, 0x1e4, g_zEffectAnim_ProcessActivationRecordName);
            } else if (record->base.nodeToken == -1) {
                memcpy(AllocActivationRecord(), &record->base, sizeof(zEffectAnimActivationRecord));
            }
        } else if (record->base.nodeToken == -1) {
            memcpy(AllocActivationRecord(), &record->base, sizeof(zEffectAnimActivationRecord));
        }
    }

    if (record->savedActivationState == 4) {
        if (entry->activationState != 4) {
            Stop(entry);
            entry->activationState = 4;
            ReportOld(0x100, g_zEffect_SourceFile_ZeffAnimSaveC, 0x1f5, g_zEffectAnim_StateInvalidMsg);
        }
        if (record->base.nodeToken == -1) {
            memcpy(AllocActivationRecord(), &record->base, sizeof(zEffectAnimActivationRecord));
        }
    }

    for (i = 0; i < record->trackedNodeCount; ++i) {
        zEffectAnimTrackedNodeSaveRecord* const tracked = &record->trackedNodes[i];
        CZNodePartial* const node = NodeIndexToPtr(tracked->nodeIndex);
        if (node == 0 || node->classId != 5) {
            continue;
        }

        ReportOld(
            0x100,
            g_zEffect_SourceFile_ZeffAnimSaveC,
            0x201,
            g_zEffectAnim_RestoreNodeFmt,
            node,
            tracked->activeFlag
        );
        gwNodeSetActive(node, tracked->activeFlag != 0 ? 1 : 0);
        if (tracked->activeFlag != 0) {
            if (tracked->usesCachedMatrix != 0) {
                gwObject3DSetMatrix(node, tracked->transform);
            } else {
                gwObject3DSetPosition(node, tracked->transform[0], tracked->transform[1], tracked->transform[2]);
                gwObject3DSetRotation(node, tracked->transform[3], tracked->transform[4], tracked->transform[5]);
                gwObject3DSetScale(node, tracked->transform[6], tracked->transform[7], tracked->transform[8]);
            }
        }

        if (node->userDataOrDiRef != 0) {
            const int diFlag = tracked->diFlagBits & 1;
            if (diFlag != 0) {
                unsigned int* const di = (unsigned int*)(node->userDataOrDiRef);
                di[1] = (di[1] & ~0x08u) | (unsigned int)(diFlag << 3);
                ((unsigned int*)(node->userDataOrDiRef))[8] = (unsigned int)(tracked->diUserValue);
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.allocactivationrecord
 * @recoil-artifact defines .text recoil:function:0x460ae0: zEffect_Anim::AllocActivationRecord.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: allocate or grow the activation-record queue and return the next slot.
 */
zEffectAnimActivationRecord* __cdecl AllocActivationRecord(void)
{
    if (g_zEffectAnim_ActivationRecordTable == 0) {
        g_zEffectAnim_ActivationRecordTable = (zEffectAnimActivationRecord*)(malloc(
            sizeof(zEffectAnimActivationRecord) * kInitialActivationRecordCapacity
        ));
        g_zEffectAnim_ActivationRecordCapacity = kInitialActivationRecordCapacity;
        g_zEffectAnim_ActivationRecordCount = 0;
    }

    if (g_zEffectAnim_ActivationRecordCount >= g_zEffectAnim_ActivationRecordCapacity) {
        zEffectAnimActivationRecord* const oldTable = g_zEffectAnim_ActivationRecordTable;
        g_zEffectAnim_ActivationRecordTable = (zEffectAnimActivationRecord*)(malloc(
            sizeof(zEffectAnimActivationRecord) * g_zEffectAnim_ActivationRecordCapacity * 2
        ));
        memcpy(
            g_zEffectAnim_ActivationRecordTable,
            oldTable,
            sizeof(zEffectAnimActivationRecord) * g_zEffectAnim_ActivationRecordCapacity
        );
        g_zEffectAnim_ActivationRecordCapacity *= 2;
        free(oldTable);
    }

    g_zEffectAnim_ActivationRecordTable[g_zEffectAnim_ActivationRecordCount].recordId
        = (g_zEffectAnim_ActivationDispatchTagHigh & (unsigned int)(g_zEffectAnim_ActivationRecordCount)) & 0x00ffffffu;
    return &g_zEffectAnim_ActivationRecordTable[g_zEffectAnim_ActivationRecordCount++];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.saverunninganimrecord
 * @recoil-artifact defines .text recoil:function:0x460bc0: zEffect_Anim::SaveRunningAnimRecord.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: write one running animation entry, runtime sequence state, and
 * attached light/sound refs into a temporary ZBD section stream.
 */
int __fastcall SaveRunningAnimRecord(
    zZbdSectionCallbackCtx* callbackCtx,
    zEffectAnimEntry* entry,
    int runningIndex,
    int includePrimaryEntry
)
{
    zEffectAnimRunningSaveHeader header;
    char sectionName[0x14];
    int result;
    FILE* tempStream;
    header.entryTableIndex = runningIndex;
    header.matchSavedRootNode = includePrimaryEntry;
    strncpy(header.entryName, entry->name, sizeof(header.entryName));
    header.rootNodeIndex = NodePtrToValidatedIndex(entry->boundNode);
    header.nodeRefAIndex = NodePtrToValidatedIndex(entry->refNodeA);
    header.refVecA = entry->refPointA;
    header.nodeRefBIndex = NodePtrToValidatedIndex(entry->refNodeB);
    header.refVecB = entry->refPointB;
    header.activationState = entry->activationState;
    header.triggerCurrentValue = entry->triggerCurrentValue;
    header.activationCountdown = entry->activationCountdown;
    header.velocity = entry->velocity;
    header.runtimeSurfaceCount = entry->runtimeSequenceCount;
    header.lightRefCount = entry->lightRefCount;
    header.soundRefCount = entry->soundRefCount;

    sprintf(sectionName, g_zEffectAnim_RunningSectionNameFmt, runningIndex);

    tempStream = OpenTempWriteStream();
    if (tempStream != 0) {
        int i_2646;
        int i_2660;
        int i_2679;
        result = fwrite(&header, sizeof(header), 1, tempStream) == 1;
        for (i_2646 = 0; i_2646 < entry->runtimeSequenceCount && result != 0; ++i_2646) {
            zEffectAnimSurfaceRuntime* const runtime = &entry->runtimeList[i_2646];
            zEffectAnimSurfaceRuntime runtimeCopy = *runtime;
            runtimeCopy.currentEvent
                = (void*)((unsigned char*)(runtime->currentEvent) - (unsigned char*)(runtime->eventStream));
            result = fwrite(&runtimeCopy, sizeof(runtimeCopy), 1, tempStream) == 1;
            if (runtime->eventStreamSize > 0) {
                result = fwrite(runtime->eventStream, runtime->eventStreamSize, 1, tempStream) == 1;
            }
        }

        for (i_2660 = 0; i_2660 < entry->lightRefCount && result != 0; ++i_2660) {
            zEffectAnimRuntimeNodeSaveRecord record;
            zEffectAnimRuntimeNodeRef* const lightRef = &entry->lightRefList[i_2660];
            CZNodePartial* const node = lightRef->runtimeNode;
            strncpy(record.name, lightRef->name.text, sizeof(record.name));
            record.isAttached = lightRef->isAttached;
            if (node != 0) {
                // Original 0x460bc0 uses this shared position helper for saved light refs too.
                gwSoundGetPosition(node, &record.posX, &record.posY, &record.posZ);
                record.parentNodeIndex = node->listCountA > 0 ? NodePtrToValidatedIndex(node->listA[0]) : -1;
            }
            result = fwrite(&record, sizeof(record), 1, tempStream) == 1;
        }

        for (i_2679 = 0; i_2679 < entry->soundRefCount && result != 0; ++i_2679) {
            zEffectAnimSoundNodeSaveRecord record;
            zEffectAnimRuntimeNodeRef* const soundRef = &entry->soundRefList[i_2679];
            CZNodePartial* const node = soundRef->runtimeNode;
            strncpy(record.name, soundRef->name.text, sizeof(record.name));
            record.isAttached = soundRef->isAttached;
            if (node != 0) {
                CZSoundDataPartial* const soundData = (CZSoundDataPartial*)(node->classData);
                record.hasPosition = (soundData->runtimeFlags >> 1) & 1;
                gwSoundGetPosition(node, &record.posX, &record.posY, &record.posZ);
                record.parentNodeIndex = node->listCountA > 0 ? NodePtrToValidatedIndex(node->listA[0]) : -1;
            }
            result = fwrite(&record, sizeof(record), 1, tempStream) == 1;
        }

        FlushTempWriteStreamToSectionRecord(tempStream, callbackCtx, sectionName);
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.saverunninganimrecords
 * @recoil-artifact defines .text recoil:function:0x460f80: zEffect_Anim::SaveRunningAnimRecords.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: enumerate active animation entries and cloned siblings that need
 * running-state persistence.
 */
int __fastcall SaveRunningAnimRecords(zZbdSectionCallbackCtx* callbackCtx)
{
    int result = 1;
    int i;
    for (i = 1; i < g_zEffectAnim_State.entryCount && result != 0; ++i) {
        zEffectAnimEntry* const entry = &g_zEffectAnim_State.entryList[i];
        if (entry != 0 && (entry->activationState == 2 || entry->activationState == 6)) {
            if (((entry->flags & 0x1000) == 0 || (entry->flags & 0x2000) != 0)
                && g_zEffectAnim_RecordQueueEnabled != 0) {
                zEffectAnimEntry* sibling;
                result = SaveRunningAnimRecord(callbackCtx, entry, i, 1);
                sibling = entry->runtimeSibling;
                while (sibling != 0 && result != 0) {
                    if (sibling->activationState == 2) {
                        result = SaveRunningAnimRecord(callbackCtx, sibling, i, 0);
                    }
                    sibling = sibling->runtimeSibling;
                }
            }
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.loadrunninganimrecords
 * @recoil-artifact defines .text recoil:function:0x461040: zEffect_Anim::LoadRunningAnimRecords.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: restore one running animation entry, sequence event streams, reset
 * scratch refs, and attached light/sound refs from a ZAR section.
 */
void __fastcall LoadRunningAnimRecords(void* unused, const char* sectionToken, void* data, int dataSize, void* extraCtx)
{
    FILE* tempStream;
    zEffectAnimRunningSaveHeader header;
    zEffectAnimEntry* entry;
    CZNodePartial* rootNode;
    int i;
    int i_2831;
    int i_2861;
    (void)unused;
    (void)sectionToken;
    (void)extraCtx;

    tempStream = OpenTempReadStream(data, dataSize);
    if (tempStream == 0) {
        return;
    }

    fread(&header, sizeof(header), 1, tempStream);

    entry = &g_zEffectAnim_State.entryList[header.entryTableIndex];
    rootNode = NodeIndexToPtr(header.rootNodeIndex);

    if (header.matchSavedRootNode != 0) {
        while (entry->runtimeSibling != 0 && entry->boundNode != rootNode) {
            entry = entry->runtimeSibling;
        }

        if (entry->boundNode != rootNode) {
            while (entry->runtimeSibling != 0 && entry->activationState == 2) {
                entry = entry->runtimeSibling;
            }

            if (entry->activationState == 2) {
                entry->runtimeSibling = CloneEntryForNode(entry, rootNode);
            }
        }
    }

    entry->flags |= 0x4000u;
    entry->refNodeA = NodeIndexToPtr(header.nodeRefAIndex);
    entry->refPointA = header.refVecA;
    entry->refNodeB = NodeIndexToPtr(header.nodeRefBIndex);
    entry->refPointB = header.refVecB;
    entry->activationState = header.activationState;
    entry->triggerCurrentValue = header.triggerCurrentValue;
    entry->activationCountdown = header.activationCountdown;
    entry->velocity = header.velocity;
    entry->runtimeSequenceCount = header.runtimeSurfaceCount;

    for (i = 0; i < entry->runtimeSequenceCount; ++i) {
        zEffectAnimSurfaceRuntime* runtime;
        if (entry->runtimeList[i].eventStream != 0) {
            free(entry->runtimeList[i].eventStream);
            entry->runtimeList[i].eventStream = 0;
        }

        runtime = &entry->runtimeList[i];
        fread(runtime, sizeof(*runtime), 1, tempStream);

        if (runtime->eventStreamSize > 0) {
            void* const eventStream = malloc(runtime->eventStreamSize);
            runtime->currentEvent = (unsigned char*)(eventStream) + (unsigned int)(runtime->currentEvent);
            runtime->eventStream = eventStream;
            fread(runtime->eventStream, runtime->eventStreamSize, 1, tempStream);
        }
    }

    for (i_2831 = 0; i_2831 < header.lightRefCount; ++i_2831) {
        zEffectAnimRuntimeNodeSaveRecord record;
        int lightIndex;
        zEffectAnimRuntimeNodeRef* lightRef;
        CZNodePartial* lightNode;
        fread(&record, sizeof(record), 1, tempStream);

        if (record.isAttached == 0) {
            continue;
        }

        lightIndex = FindOrCreateLightRef(entry, record.name);
        if (lightIndex < 0) {
            continue;
        }

        lightRef = &entry->lightRefList[lightIndex];
        lightNode = lightRef->runtimeNode;
        if (lightRef->isAttached != 0 || lightNode == 0) {
            continue;
        }

        gwNodeSetActive(lightNode, 1);
        if (record.parentNodeIndex >= 0 && lightNode->listCountA == 0) {
            CZNodePartial* const parentNode = NodeIndexToPtr(record.parentNodeIndex);
            AddChild(parentNode, lightNode);
        }
        gwLightSetPosition(lightNode, record.posX, record.posY, record.posZ);
        AddLight(g_zEffectAnim_State.worldNode, lightNode);
        lightRef->isAttached = 1;
    }

    for (i_2861 = 0; i_2861 < header.soundRefCount; ++i_2861) {
        zEffectAnimSoundNodeSaveRecord record;
        int soundIndex;
        zEffectAnimRuntimeNodeRef* soundRef;
        CZNodePartial* soundNode;
        fread(&record, sizeof(record), 1, tempStream);

        if (record.isAttached == 0) {
            continue;
        }

        soundIndex = FindOrCreateSoundRef(entry, record.name);
        if (soundIndex < 0) {
            continue;
        }

        soundRef = &entry->soundRefList[soundIndex];
        soundNode = soundRef->runtimeNode;
        if (soundRef->isAttached != 0 || soundNode == 0) {
            continue;
        }

        gwNodeSetActive(soundNode, 1);
        if (record.parentNodeIndex >= 0 && soundNode->listCountA == 0) {
            CZNodePartial* const parentNode = NodeIndexToPtr(record.parentNodeIndex);
            AddChild(parentNode, soundNode);
        }
        if (record.hasPosition != 0) {
            gwSoundSetPosition(soundNode, record.posX, record.posY, record.posZ);
        }
        AddSound(g_zEffectAnim_State.worldNode, soundNode);
        soundRef->isAttached = 1;
    }

    entry->runtimeNode->callbackContext = (CZNodePartial*)(entry);
    gwNodeSetActionCallbackTail(entry->runtimeNode, (void*)(&RunSequence));
    CloseTempReadStream(tempStream);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.saveanimrecords
 * @recoil-artifact defines .text recoil:function:0x461430: zEffect_Anim::SaveAnimRecords.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: serialize non-running animation activation state and tracked-node
 * transforms into numbered Anim sections.
 */
int __fastcall SaveAnimRecords(zZbdSectionCallbackCtx* callbackCtx)
{
    int result = 1;
    int i;
    for (i = 1; i < g_zEffectAnim_State.entryCount && result != 0; ++i) {
        zEffectAnimEntry* const entry = &g_zEffectAnim_State.entryList[i];

        zEffectAnimSaveRecord saveRecord;
        zEffectAnimSaveHeader* const header = &saveRecord.header;
        char sectionName[0x14];
        header->base.commandType = 0;
        if (entry != 0) {
            if (entry->activationState == 5) {
                continue;
            }

            if (((entry->flags & 0x1000) != 0 && (entry->flags & 0x2000) == 0)
                || g_zEffectAnim_RecordQueueEnabled == 0) {
                continue;
            }

            header->entryTableIndex = i;
            strncpy(header->base.animName, entry->name, sizeof(header->base.animName));
            if (entry->trackedNodeList != 0) {
                int childIndex;
                header->savedActivationState = entry->activationState;
                header->trackedNodeCount = entry->trackedNodeCount;
                for (childIndex = 0; childIndex < entry->trackedNodeCount; ++childIndex) {
                    CZNodePartial* const node = entry->trackedNodeList[childIndex].trackedNode;
                    zEffectAnimTrackedNodeSaveRecord* const record = &saveRecord.trackedNodes[childIndex];
                    CZObject3DDataPartial* objectData;
                    if (node == 0 || node->classId != 5) {
                        record->nodeIndex = -1;
                        continue;
                    }

                    objectData = (CZObject3DDataPartial*)(node->classData);
                    record->nodeIndex = NodePtrToValidatedIndex(node);
                    record->activeFlag = ((unsigned int)(node->flags) >> 2) & 1;
                    record->usesCachedMatrix = ((unsigned int)(objectData->flags) >> 4) & 1;
                    if (record->usesCachedMatrix != 0) {
                        memcpy(record->transform, gwObject3DGetMatrixPtr(node), sizeof(record->transform));
                    } else {
                        gwObject3DGetPosition(
                            node,
                            &record->transform[0],
                            &record->transform[1],
                            &record->transform[2]
                        );
                        gwObject3DGetRotation(
                            node,
                            &record->transform[3],
                            &record->transform[4],
                            &record->transform[5]
                        );
                        gwObject3DGetScale(node, &record->transform[6], &record->transform[7], &record->transform[8]);
                    }
                }
            } else {
                header->trackedNodeCount = 0;
            }
        } else {
            strncpy(header->base.animName, g_zEffect_StringNone, sizeof(header->base.animName));
            header->savedActivationState = 0;
            header->trackedNodeCount = 0;
        }

        sprintf(sectionName, g_zEffectAnim_AnimSectionNameFmt, i + g_zEffectAnim_ActivationRecordCount);
        result = WriteSectionBlob(
            callbackCtx,
            sectionName,
            &saveRecord,
            sizeof(zEffectAnimSaveHeader) + sizeof(zEffectAnimTrackedNodeSaveRecord) * header->trackedNodeCount
        );
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.loadanimrecords
 * @recoil-artifact defines .text recoil:function:0x461670: zEffect_Anim::LoadAnimRecords.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: restore non-running animation activation state and tracked-node
 * transforms from a saved Anim section.
 */
void __fastcall LoadAnimRecords(void* unused, const char* sectionToken, void* data, int dataSize, void* extraCtx)
{
    zEffectAnimSaveHeader* header;
    zEffectAnimEntry* entry;
    zEffectAnimEntry* cursor;
    zEffectAnimTrackedNodeSaveRecord* records;
    int i;
    (void)unused;
    (void)sectionToken;
    (void)dataSize;
    (void)extraCtx;

    header = (zEffectAnimSaveHeader*)(data);
    entry = &g_zEffectAnim_State.entryList[header->entryTableIndex];
    if (entry == 0 || (entry->flags & 0x4000u) != 0) {
        return;
    }

    if (header->savedActivationState == 1 && entry->activationState != 1) {
        if (entry->activationState == 4) {
            entry->activationState = 3;
        }
        zEffAnimReset(entry, entry->boundNode);
        ReportOld(0x100, g_zEffect_SourceFile_ZeffAnimSaveC, 0x419, g_zEffectAnim_ResetTraceFmt, entry);
    }

    for (cursor = entry; cursor != 0; cursor = cursor->runtimeSibling) {
        if ((cursor->flags & 0x4000u) == 0 && cursor->activationState == 2) {
            zEffAnimReset(cursor, cursor->boundNode);
            ReportOld(0x100, g_zEffect_SourceFile_ZeffAnimSaveC, 0x41f, g_zEffectAnim_ResetTraceFmt, cursor);
        }
    }

    records = (zEffectAnimTrackedNodeSaveRecord*)((unsigned char*)(data) + sizeof(*header));
    for (i = 0; i < header->trackedNodeCount; ++i) {
        zEffectAnimTrackedNodeSaveRecord* const record = &records[i];
        CZNodePartial* const node = NodeIndexToPtr(record->nodeIndex);
        if (node == 0 || node->classId != 5) {
            continue;
        }

        ReportOld(
            0x100,
            g_zEffect_SourceFile_ZeffAnimSaveC,
            0x426,
            g_zEffectAnim_RestoreNodeFmt,
            node,
            record->activeFlag
        );
        gwNodeSetActive(node, record->activeFlag != 0 ? 1 : 0);
        if (record->activeFlag == 0) {
            continue;
        }

        if (record->usesCachedMatrix != 0) {
            gwObject3DSetMatrix(node, record->transform);
        } else {
            gwObject3DSetPosition(node, record->transform[0], record->transform[1], record->transform[2]);
            gwObject3DSetRotation(node, record->transform[3], record->transform[4], record->transform[5]);
            gwObject3DSetScale(node, record->transform[6], record->transform[7], record->transform[8]);
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.getactivationrecordpackedsize
 * @recoil-artifact defines .text recoil:function:0x461800: zEffect_Anim::GetActivationRecordPackedSize.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: return the serialized byte count for an activation record command type.
 */
int __fastcall GetActivationRecordPackedSize(zEffectAnimActivationRecord* record)
{
    switch (record->commandType) {
    case 2:
        return 0x38;
    case 3:
        return 0x48;
    case 4:
        return 0x4c;
    case 1:
        return 0x50;
    }

    return 0x50;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.resetfromactivationrecord
 * @recoil-artifact defines .text recoil:function:0x461840: zEffect_Anim::zEffResetActivationRecord.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: restart the named animation against the node stored in an activation record.
 */
void __fastcall zEffResetActivationRecord(zEffectAnimActivationRecord* record)
{
    zEffectAnimEntry* const entry = FindEntryByName(record->animName);
    CZNodePartial* const node = NodeIndexToPtr(record->nodeToken);
    zEffAnimReset(entry, node);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.processactivationrecord
 * @recoil-artifact defines .text recoil:function:0x461870: zEffect_Anim::zEffProcessActivationRecord.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: dispatch a queued activation record to the matching animation
 * activation command.
 */
zEffectAnimEntry* __fastcall zEffProcessActivationRecord(zEffectAnimActivationRecord* record)
{
    zEffectAnimEntry* result = 0;
    zEffectAnimEntry* const entry = FindEntryByName(record->animName);
    if (entry == 0) {
        return 0;
    }

    switch (record->commandType) {
    case 1: {
        CZNodePartial* const rootNode = NodeIndexToPtr(record->nodeToken);
        result = SetTransformRotAndVelocityThunk(
            entry,
            rootNode,
            record->params[0].f32,
            record->params[1].f32,
            record->params[2].f32,
            record->params[3].f32,
            record->params[4].f32,
            record->params[5].f32,
            record->params[6].f32,
            record->params[7].f32,
            record->params[8].f32
        );
        break;
    }

    case 2: {
        CZNodePartial* const rootNode = NodeIndexToPtr(record->nodeToken);
        result = SetVelocityThunk(entry, rootNode, record->params[0].f32, record->params[1].f32, record->params[2].f32);
        break;
    }

    case 3: {
        CZNodePartial* const rootNode = NodeIndexToPtr(record->nodeToken);
        CZNodePartial* const refNode = NodeIndexToPtr(record->params[0].i32);
        result = SetPositionRefAndVelocityThunk(
            entry,
            rootNode,
            refNode,
            (const zVec3*)(&record->params[1]),
            (const zVec3*)(&record->params[4])
        );
        break;
    }

    case 4: {
        CZNodePartial* const rootNode = NodeIndexToPtr(record->nodeToken);
        CZNodePartial* const refNodeA = NodeIndexToPtr(record->params[0].i32);
        CZNodePartial* const refNodeB = NodeIndexToPtr(record->params[4].i32);
        result = SetTransformRefsThunk(
            entry,
            rootNode,
            refNodeA,
            (const zVec3*)(&record->params[1]),
            refNodeB,
            (const zVec3*)(&record->params[5])
        );
        break;
    }

    default:
        break;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.queuecmdtype1transformrotvelocity
 * @recoil-artifact defines .text recoil:function:0x461970: zEffectAnim::QueueCmdType1TransformRotVelocity.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim_activation.c.
 * Purpose: build the command type 1 activation record for transform, rotation,
 * and velocity activation and apply the queue/dispatch gates.
 */
zEffectAnimActivationRecord* __fastcall QueueCmdType1TransformRotVelocity(
    zEffectAnimEntry* self,
    CZNodePartial* boundNode,
    float posX,
    float posY,
    float posZ,
    float rotX,
    float rotY,
    float rotZ,
    float velocityX,
    float velocityY,
    float velocityZ
)
{
    zEffectAnimActivationRecord* result = 0;
    const int boundNodeToken = NodePtrToValidatedIndex(boundNode);
    const unsigned int flags = self->flags;

    int recordQueueRequested;
    int dispatchRequested;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 && (flags & kActivationRecordNoQueueDispatchFlag) == 0
        && self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0)) {
        recordQueueRequested = 1;
    } else {
        recordQueueRequested = 0;
    }

    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 && (flags & kActivationRecordNoQueueDispatchFlag) == 0
        && self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0)) {
        dispatchRequested = 1;
    } else {
        dispatchRequested = 0;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord* const record = AllocActivationRecord();
        record->commandType = 1;
        strncpy(record->animName, self->name, sizeof(record->animName));
        record->nodeToken = boundNodeToken;
        record->params[0].f32 = posX;
        record->params[1].f32 = posY;
        record->params[2].f32 = posZ;
        record->params[3].f32 = rotX;
        record->params[4].f32 = rotY;
        record->params[5].f32 = rotZ;
        record->params[6].f32 = velocityX;
        record->params[7].f32 = velocityY;
        record->params[8].f32 = velocityZ;

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            DiscardLastActivationRecord();
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.discardlastactivationrecord
 * @recoil-artifact defines .text recoil:function:0x461a90: zEffect_Anim::DiscardLastActivationRecord.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: remove the most recently allocated activation record from the queue.
 */
void __cdecl DiscardLastActivationRecord(void)
{
    --g_zEffectAnim_ActivationRecordCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.queuecmdtype2velocity
 * @recoil-artifact defines .text recoil:function:0x461aa0: zEffectAnim::QueueCmdType2Velocity.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim_activation.c.
 * Purpose: build the command type 2 activation record for velocity-only
 * activation and apply the queue/dispatch gates.
 */
zEffectAnimActivationRecord* __fastcall QueueCmdType2Velocity(
    zEffectAnimEntry* self,
    CZNodePartial* boundNode,
    float velocityX,
    float velocityY,
    float velocityZ
)
{
    zEffectAnimActivationRecord* result = 0;
    const int boundNodeToken = NodePtrToValidatedIndex(boundNode);
    const unsigned int flags = self->flags;

    int recordQueueRequested;
    int dispatchRequested;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 && (flags & kActivationRecordNoQueueDispatchFlag) == 0
        && self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0)) {
        recordQueueRequested = 1;
    } else {
        recordQueueRequested = 0;
    }

    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 && (flags & kActivationRecordNoQueueDispatchFlag) == 0
        && self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0)) {
        dispatchRequested = 1;
    } else {
        dispatchRequested = 0;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord* const record = AllocActivationRecord();
        record->commandType = 2;
        strncpy(record->animName, self->name, sizeof(record->animName));
        record->nodeToken = boundNodeToken;
        record->params[0].f32 = velocityX;
        record->params[1].f32 = velocityY;
        record->params[2].f32 = velocityZ;

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            DiscardLastActivationRecord();
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.queuecmdtype3positionrefandvelocity
 * @recoil-artifact defines .text recoil:function:0x461ba0: zEffectAnim::QueueCmdType3PositionRefAndVelocity.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim_activation.c.
 * Purpose: build the command type 3 activation record for a position reference
 * plus velocity and apply the queue/dispatch gates.
 */
zEffectAnimActivationRecord* __fastcall QueueCmdType3PositionRefAndVelocity(
    zEffectAnimEntry* self,
    CZNodePartial* boundNode,
    CZNodePartial* refNode,
    const zVec3* refVec,
    const zVec3* velocityVec
)
{
    zEffectAnimActivationRecord* result = 0;
    const int boundNodeToken = NodePtrToValidatedIndex(boundNode);
    const int refNodeToken = NodePtrToValidatedIndex(refNode);
    const unsigned int flags = self->flags;
    int recordQueueRequested;
    int dispatchRequested;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if ((flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0'
        && (boundNode == 0 || boundNodeToken > 0) && (refNode == 0 || refNodeToken > 0)) {
        recordQueueRequested = 1;
    } else {
        recordQueueRequested = 0;
    }

    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if ((flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0'
        && (boundNode == 0 || boundNodeToken > 0) && (refNode == 0 || refNodeToken > 0)) {
        dispatchRequested = 1;
    } else {
        dispatchRequested = 0;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord* const record = AllocActivationRecord();
        record->commandType = 3;
        strncpy(record->animName, self->name, sizeof(record->animName));
        record->nodeToken = boundNodeToken;
        record->params[0].i32 = refNodeToken;
        if (refVec != 0) {
            *(zVec3*)(&record->params[1]) = *refVec;
        } else {
            record->params[1].u32 = record->params[2].u32 = record->params[3].u32 = 0;
        }

        if (velocityVec != 0) {
            *(zVec3*)(&record->params[4]) = *velocityVec;
        } else {
            record->params[4].u32 = record->params[5].u32 = record->params[6].u32 = 0;
        }

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            DiscardLastActivationRecord();
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.queuecmdtype4transformrefs
 * @recoil-artifact defines .text recoil:function:0x461d00: zEffectAnim::QueueCmdType4TransformRefs.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim_activation.c.
 * Purpose: build the command type 4 activation record for two transform
 * references and apply the queue/dispatch gates.
 */
zEffectAnimActivationRecord* __fastcall QueueCmdType4TransformRefs(
    zEffectAnimEntry* self,
    CZNodePartial* boundNode,
    CZNodePartial* refNodeA,
    const zVec3* refVecA,
    CZNodePartial* refNodeB,
    const zVec3* refVecB
)
{
    zEffectAnimActivationRecord* result = 0;
    const int boundNodeToken = NodePtrToValidatedIndex(boundNode);
    const int refNodeAToken = NodePtrToValidatedIndex(refNodeA);
    const int refNodeBToken = NodePtrToValidatedIndex(refNodeB);
    const unsigned int flags = self->flags;
    int recordQueueRequested;
    int dispatchRequested;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 && (flags & kActivationRecordNoQueueDispatchFlag) == 0
        && self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0) && (refNodeA == 0 || refNodeAToken > 0)
        && (refNodeB == 0 || refNodeBToken > 0)) {
        recordQueueRequested = 1;
    } else {
        recordQueueRequested = 0;
    }

    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 && (flags & kActivationRecordNoQueueDispatchFlag) == 0
        && self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0) && (refNodeA == 0 || refNodeAToken > 0)
        && (refNodeB == 0 || refNodeBToken > 0)) {
        dispatchRequested = 1;
    } else {
        dispatchRequested = 0;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord* const record = AllocActivationRecord();
        record->commandType = 4;
        strncpy(record->animName, self->name, sizeof(record->animName));
        record->nodeToken = boundNodeToken;
        record->params[0].i32 = refNodeAToken;
        if (refVecA != 0) {
            *(zVec3*)(&record->params[1]) = *refVecA;
        } else {
            record->params[1].u32 = record->params[2].u32 = record->params[3].u32 = 0;
        }

        record->params[4].i32 = refNodeBToken;
        if (refVecB != 0) {
            *(zVec3*)(&record->params[5]) = *refVecB;
        } else {
            record->params[5].u32 = record->params[6].u32 = record->params[7].u32 = 0;
        }

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            DiscardLastActivationRecord();
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.setactivationdispatchcontext
 * @recoil-artifact defines .text recoil:function:0x461eb0: zEffect_Anim::SetActivationDispatchContext.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zEffect.cpp.
 * Purpose: store the activation-dispatch callback and high-byte context tag.
 */
void __fastcall
SetActivationDispatchContext(void(__fastcall* callback)(zEffectAnimActivationRecord* record), int context)
{
    g_zEffectAnim_ActivationDispatchCallback = callback;
    g_zEffectAnim_ActivationDispatchTagHigh = (unsigned int)(context) << 24;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.findnodeuserdatarecursive
 * @recoil-artifact defines .text recoil:function:0x461ec0: zEffect::FindNodeUserDataRecursive.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: find the first non-null user-data value in a root-first node tree
 * traversal.
 */
void* __fastcall FindNodeUserDataRecursive(CZNodePartial* node)
{
    unsigned int userDataValue;
    int i;
    gwNodeGetUserData(node, &userDataValue);
    if (userDataValue != 0) {
        return (void*)(userDataValue);
    }

    for (i = 0; i < node->listCountB; ++i) {
        userDataValue = (unsigned int)(FindNodeUserDataRecursive(node->listB[i]));
        if (userDataValue != 0) {
            return (void*)(userDataValue);
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.spawnruntimeinstanceat
 * @recoil-artifact defines .text recoil:function:0x461f00: zEffect::SpawnRuntimeInstanceAt.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: acquire and activate a runtime effect entry at a world position,
 * then install its node action callback.
 */
void __fastcall SpawnRuntimeInstanceAt(int effectIndex, const zVec3* worldPos)
{
    if (g_zEffect_RuntimeManager.initialized != 0 && effectIndex != -1) {
        zEffect_RuntimeEntry* const entry = AcquireRuntimeEntryByIndex(effectIndex);
        if (entry != 0) {
            ActivateRuntimeEntryAtPosition(entry, worldPos);
            gwNodeSetActive(entry->effectNode, 1);
            entry->effectNode->callbackContext = (CZNodePartial*)(entry);
            gwNodeSetActionCallback(entry->effectNode, (void*)(&RuntimeNodeActionCallback));
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.activateruntimeentryatposition
 * @recoil-artifact defines .text recoil:function:0x461f50: zEffect::ActivateRuntimeEntryAtPosition.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: initialize runtime scale, lifetime, fade, position, variant, and
 * parent attachment for one active effect instance.
 */
int __fastcall ActivateRuntimeEntryAtPosition(zEffect_RuntimeEntry* runtimeEntry, const zVec3* worldPos)
{
    float distanceSq;
    float currentScale;
    runtimeEntry->elapsedSec = 0.0f;
    runtimeEntry->currentScale = 5.0f;
    runtimeEntry->initialScale = 1.4f;
    runtimeEntry->nearCullDistSq = 56.25f;
    runtimeEntry->farFadeDistSq = 1600.0f;
    runtimeEntry->fadeInTimeSec = 0.3f;
    runtimeEntry->fadeInScaleRate = 6.0f;
    runtimeEntry->baseScale = 6.0f;
    runtimeEntry->lifeTimeSec = 30.0f;
    runtimeEntry->fadeOutScaleRate = 3.75f;
    runtimeEntry->fadeOutStartScale = 3.0f;
    runtimeEntry->fadeOutEndScale = 10.5f;
    runtimeEntry->fadeOutStartTimeSec = 1.0f;

    distanceSq = ComputeDistanceSqToListener(worldPos);
    if (distanceSq < runtimeEntry->nearCullDistSq) {
        runtimeEntry->currentScale = 0.0f;
        runtimeEntry->fadeInTimeSec = 0.0f;
        runtimeEntry->fadeOutStartTimeSec = 0.0f;
        return 0;
    }

    if (distanceSq < runtimeEntry->farFadeDistSq) {
        const float scaleFactor = distanceSq / runtimeEntry->farFadeDistSq;
        runtimeEntry->currentScale *= scaleFactor;
        runtimeEntry->fadeInScaleRate *= scaleFactor;
    }

    currentScale = runtimeEntry->currentScale;
    gwObject3DSetScale(runtimeEntry->effectNode, currentScale, currentScale, currentScale);
    gwObject3DSetPosition(runtimeEntry->effectNode, worldPos->x, worldPos->y, worldPos->z);
    ResetCurrentVariant((zDiPartial*)(runtimeEntry->effectGfxData));
    AddChild(g_zEffect_RuntimeManager.parentNode, runtimeEntry->effectNode);
    ++g_zEffect_RuntimeManager.activatedCount;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.computedistancesqtolistener
 * @recoil-artifact defines .text recoil:function:0x462050: zEffect::ComputeDistanceSqToListener.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-sq
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: compute squared distance from the runtime listener node to a world
 * position.
 */
float __fastcall ComputeDistanceSqToListener(const zVec3* worldPos)
{
    zVec3 listenerPosition;
    float distanceSq;
    GetWorldPosition(g_zEffect_RuntimeManager.listenerNode, &listenerPosition);
    Vec3Subtract(&listenerPosition, worldPos, &listenerPosition);
    ZMTH_VECTOR_LENGTH_SQ(distanceSq, &listenerPosition);
    return distanceSq;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.acquireruntimeentrybyindex
 * @recoil-artifact defines .text recoil:function:0x4620d0: zEffect::AcquireRuntimeEntryByIndex.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: reuse a free runtime effect entry for an index or clone a fresh
 * entry from its template.
 */
zEffect_RuntimeEntry* __fastcall AcquireRuntimeEntryByIndex(int effectIndex)
{
    zEffect_RuntimeEntry* payload;
    if (effectIndex == -1) {
        return 0;
    }

    payload
        = (zEffect_RuntimeEntry*)(zArchiveListFindKey(g_zEffect_RuntimeManager.freeList, (unsigned int)(effectIndex)));
    if (payload != 0) {
        ++g_zEffect_RuntimeManager.recycleCount;
        zArchiveListRemove(g_zEffect_RuntimeManager.freeList, payload);
    } else {
        ++g_zEffect_RuntimeManager.freshAllocCount;
        payload = CloneRuntimeEntryFromTemplate(effectIndex);
    }

    return payload;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.cloneruntimeentryfromtemplate
 * @recoil-artifact defines .text recoil:function:0x462130: zEffect::CloneRuntimeEntryFromTemplate.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: allocate a runtime effect entry and copy its template node tree and
 * graphics data reference.
 */
zEffect_RuntimeEntry* __fastcall CloneRuntimeEntryFromTemplate(int effectIndex)
{
    zEffect_RuntimeEntry* clone;
    CZNodePartial* node;
    if (effectIndex == -1) {
        return 0;
    }

    if (g_zEffect_RuntimeManager.templates[effectIndex].effectNode == 0) {
        return 0;
    }

    clone = (zEffect_RuntimeEntry*)(malloc(sizeof(zEffect_RuntimeEntry)));
    memcpy(clone, &g_zEffect_RuntimeManager.templates[effectIndex], sizeof(zEffect_RuntimeEntry));

    node = CopyNodeWithCloneOptions(clone->effectNode, g_zEffect_CloneCopyMode, g_zEffect_CloneCopyChildrenMode);
    clone->effectNode = node;
    if (node == 0) {
        return 0;
    }

    clone->effectGfxData = FindNodeUserDataRecursive(node);
    return clone->effectGfxData != 0 ? clone : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.runtimenodeactioncallback
 * @recoil-artifact defines .text recoil:function:0x4621b0: zEffect::RuntimeNodeActionCallback.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: advance runtime effect fade timing and recycle the effect entry
 * after the instance has completed.
 */
void __fastcall RuntimeNodeActionCallback(CZNodePartial* node)
{
    zEffect_RuntimeEntry* runtimeEntry;
    if ((node->flags & 0x04) == 0) {
        return;
    }

    runtimeEntry = (zEffect_RuntimeEntry*)(node->callbackContext);
    runtimeEntry->elapsedSec += g_FrameDeltaTimeSec;

    if (runtimeEntry->elapsedSec < runtimeEntry->fadeInTimeSec) {
        const float currentScale = runtimeEntry->currentScale + runtimeEntry->fadeInScaleRate * g_FrameDeltaTimeSec;
        runtimeEntry->currentScale = currentScale;
        gwObject3DSetScale(node, currentScale, currentScale, currentScale);
        return;
    }

    if (runtimeEntry->elapsedSec < runtimeEntry->fadeOutStartTimeSec) {
        float currentScale;
        runtimeEntry->currentScale -= runtimeEntry->fadeOutScaleRate * g_FrameDeltaTimeSec;
        if (runtimeEntry->currentScale < 0.01) {
            runtimeEntry->currentScale = 0.01f;
        }

        currentScale = runtimeEntry->currentScale;
        gwObject3DSetScale(node, currentScale, currentScale, currentScale);
        return;
    }

    node->callbackContext = 0;
    zArchiveListAddTail(g_zEffect_RuntimeManager.freeList, runtimeEntry);
    gwNodeSetActionCallback(node, 0);
    gwNodeSetActive(node, 0);
    if (node->listCountA != 0) {
        RemoveChild(g_zEffect_RuntimeManager.parentNode, node);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.findtemplateindexbyname
 * @recoil-artifact defines .text recoil:function:0x462280: zEffect::FindTemplateIndexByName.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\Effect.c.
 * Purpose: return the runtime template index whose effect name matches.
 */
int __fastcall FindTemplateIndexByName(const char* name)
{
    int i;
    for (i = 0; i < g_zEffect_RuntimeManager.templateCount; ++i) {
        if (strcmp(name, g_zEffect_RuntimeManager.templates[i].effectName) == 0) {
            return i;
        }
    }

    return -1;
}
