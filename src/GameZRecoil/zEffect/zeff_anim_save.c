#include "GameZRecoil/zEffect/zeff.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
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

namespace {
const int kInitialActivationRecordCapacity = 1000;
const unsigned int kActivationRecordNoQueueDispatchFlag = 0x00000100u;
const unsigned int kActivationRecordQueueOverrideFlag = 0x00001000u;
const unsigned int kActivationRecordQueueOverrideValue = 0x00002000u;
const unsigned int kActivationRecordDispatchOverrideFlag = 0x00000400u;
const unsigned int kActivationRecordDispatchOverrideValue = 0x00000800u;

struct zEffectAnimTrackedNodeSaveRecord {
    int nodeIndex;
    int activeFlag;
    int usesCachedMatrix;
    float transform[12];
    int diFlagBits;
    int diUserValue;
};

struct zEffectAnimActivationSaveRecord {
    zEffectAnimActivationRecord base;
    unsigned char unknown_50[4];
    unsigned char savedActivationState;
    unsigned char trackedNodeCount;
    unsigned char unknown_56[2];
    zEffectAnimTrackedNodeSaveRecord trackedNodes[1];
};

struct zEffectAnimSaveHeader {
    zEffectAnimActivationRecord base;
    int entryTableIndex;
    unsigned char savedActivationState;
    unsigned char trackedNodeCount;
    unsigned char unknown_56[2];
};

const int kMaxEffectAnimTrackedNodeSaveCount = 256;

struct zEffectAnimSaveRecord {
    zEffectAnimSaveHeader header;
    zEffectAnimTrackedNodeSaveRecord trackedNodes[kMaxEffectAnimTrackedNodeSaveCount];
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimTrackedNodeSaveRecord) == 0x44);
RECOIL_STATIC_ASSERT(
    offsetof(
        zEffectAnimActivationSaveRecord,
        savedActivationState
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zEffectAnimActivationSaveRecord,
        trackedNodeCount
    ) == 0x55
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zEffectAnimActivationSaveRecord,
        trackedNodes
    ) == 0x58
);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSaveHeader) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(zEffectAnimSaveRecord, trackedNodes) == 0x58);
const unsigned int kMaxActivationSaveRecordSize =
    offsetof(zEffectAnimActivationSaveRecord, trackedNodes) +
    sizeof(zEffectAnimTrackedNodeSaveRecord) * kMaxEffectAnimTrackedNodeSaveCount;

struct zEffectAnimRunningSaveHeader {
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
    float velocityX;
    float velocityY;
    float velocityZ;
    unsigned char runtimeSurfaceCount;
    unsigned char lightRefCount;
    unsigned char soundRefCount;
    unsigned char reserved;
};

struct zEffectAnimRuntimeNodeSaveRecord {
    char name[0x24];
    int isAttached;
    float posX;
    float posY;
    float posZ;
    int parentNodeIndex;
};

struct zEffectAnimSoundNodeSaveRecord {
    char name[0x24];
    int isAttached;
    int hasPosition;
    float posX;
    float posY;
    float posZ;
    int parentNodeIndex;
};

RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRunningSaveHeader) == 0x68);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimRuntimeNodeSaveRecord) == 0x38);
RECOIL_STATIC_ASSERT(sizeof(zEffectAnimSoundNodeSaveRecord) == 0x3c);
} // namespace

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.clearactivationrecords
 * @recoil-artifact defines .text recoil:function:0x4603d0: zEffect_Anim::ClearActivationRecords.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: release the queued activation-record table and reset the record count.
 */
void ClearActivationRecords() {
    if (g_zEffectAnim_ActivationRecordTable != 0) {
        free(g_zEffectAnim_ActivationRecordTable);
        g_zEffectAnim_ActivationRecordTable = 0;
    }

    g_zEffectAnim_ActivationRecordCount = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.hasactivationrecord
 * @recoil-artifact defines .text recoil:function:0x460400: zEffect_Anim::HasActivationRecord.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: report whether a queued activation record already targets the same
 * animation name and node token.
 */
int __fastcall HasActivationRecord(
    zEffectAnimActivationRecord *record
) {
    for (int i = 0; i < g_zEffectAnim_ActivationRecordCount; ++i) {
        zEffectAnimActivationRecord *const queuedRecord = &g_zEffectAnim_ActivationRecordTable[i];
        if (queuedRecord->nodeToken == record->nodeToken &&
            strncmp(
                queuedRecord->animName,
                record->animName,
                sizeof(record->animName)
            ) == 0) {
            return 1;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.getactivationrecordcount
 * @recoil-artifact defines .text recoil:function:0x460470: zEffect_Anim::GetActivationRecordCount.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: return the current number of queued activation records.
 */
int GetActivationRecordCount() {
    return g_zEffectAnim_ActivationRecordCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.getactivationrecordat
 * @recoil-artifact defines .text recoil:function:0x460480: zEffect_Anim::GetActivationRecordAt.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: return the queued activation record at the requested table index.
 */
zEffectAnimActivationRecord *__fastcall GetActivationRecordAt(
    int index
) {
    return &g_zEffectAnim_ActivationRecordTable[index];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.saveactivationrecords
 * @recoil-artifact defines .text recoil:function:0x460490: zEffect_Anim::SaveActivationRecords.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: serialize queued activation records with tracked-node state into
 * numbered ZAR activation sections.
 */
int __fastcall SaveActivationRecords(
    zZbdSectionCallbackCtx *callbackCtx
) {
    int result = 1;
    for (int i = 0; result != 0 && i < g_zEffectAnim_ActivationRecordCount; ++i) {
        zEffectAnimActivationRecord *const sourceRecord = &g_zEffectAnim_ActivationRecordTable[i];
        zEffectAnimEntry *const entry = zEffectAnim::FindEntryByName(sourceRecord->animName);
        unsigned char trackedNodeCount = 0;
        if (entry != 0 && entry->trackedNodeList != 0) {
            trackedNodeCount = entry->trackedNodeCount;
        }

        const unsigned int recordSize = offsetof(zEffectAnimActivationSaveRecord, trackedNodes) +
                                        sizeof(zEffectAnimTrackedNodeSaveRecord) * trackedNodeCount;
        unsigned char saveRecordStorage[kMaxActivationSaveRecordSize];
        zEffectAnimActivationSaveRecord *const saveRecord =
            (zEffectAnimActivationSaveRecord *)(saveRecordStorage);

        memcpy(
            &saveRecord->base,
            sourceRecord,
            sizeof(*sourceRecord)
        );
        if (entry != 0) {
            zClass_NodePartial *const rootNode = GameZ_ZBD::NodeIndexToPtr(sourceRecord->nodeToken);
            saveRecord->savedActivationState = entry->activationState;
            zEffectAnim::RebindEntryToNode(
                entry,
                rootNode
            );
            saveRecord->trackedNodeCount = trackedNodeCount;

            for (int j = 0; j < trackedNodeCount; ++j) {
                zEffectAnimTrackedNode *const tracked = &entry->trackedNodeList[j];
                zEffectAnimTrackedNodeSaveRecord *const savedTracked = &saveRecord->trackedNodes[j];
                zClass_NodePartial *const node = tracked->trackedNode;
                if (node == 0 || node->classId != 5) {
                    savedTracked->nodeIndex = -1;
                    continue;
                }

                savedTracked->nodeIndex = zClass::NodePtrToValidatedIndex(node);
                savedTracked->activeFlag = (node->flags >> 2) & 1;

                zClass_Object3DDataPartial *const objectData =
                    (zClass_Object3DDataPartial *)(node->classData);
                savedTracked->usesCachedMatrix = (objectData->flags >> 4) & 1;
                if (savedTracked->usesCachedMatrix != 0) {
                    memcpy(
                        savedTracked->transform,
                        zClass_Object3D::gwObject3DGetMatrixPtr(node),
                        sizeof(savedTracked->transform)
                    );
                } else {
                    zClass_Object3D::gwObject3DGetPosition(
                        node,
                        &savedTracked->transform[0],
                        &savedTracked->transform[1],
                        &savedTracked->transform[2]
                    );
                    zClass_Object3D::gwObject3DGetRotation(
                        node,
                        &savedTracked->transform[3],
                        &savedTracked->transform[4],
                        &savedTracked->transform[5]
                    );
                    zClass_Object3D::gwObject3DGetScale(
                        node,
                        &savedTracked->transform[6],
                        &savedTracked->transform[7],
                        &savedTracked->transform[8]
                    );
                }

                if (node->userDataOrDiRef != 0) {
                    unsigned int *const di = (unsigned int *)(node->userDataOrDiRef);
                    savedTracked->diFlagBits = (savedTracked->diFlagBits & ~1) | ((di[1] >> 3) & 1);
                    savedTracked->diUserValue = (int)(di[8]);
                } else {
                    savedTracked->diFlagBits &= ~1;
                }
            }
        } else {
            saveRecord->savedActivationState = 0;
            saveRecord->trackedNodeCount = 0;
        }

        char sectionName[0x14];
        sprintf(
            sectionName,
            g_zEffectAnim_ActivationSectionNameFmt,
            i
        );
        result = zUtil_ZAR::WriteSectionBlob(
            callbackCtx,
            sectionName,
            saveRecord,
            recordSize
        );
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.loadactivationrecords
 * @recoil-artifact defines .text recoil:function:0x4606d0: zEffect_Anim::LoadActivationRecords.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: restore queued activation records, activation states, tracked-node
 * transforms, and deferred record queue state from ZAR activation sections.
 */
void __fastcall LoadActivationRecords(
    void *,
    const char *sectionToken,
    void *data,
    int,
    void *
) {
    zEffectAnimActivationSaveRecord *const record = (zEffectAnimActivationSaveRecord *)(data);
    zEffectAnimEntry *entry = zEffectAnim::FindEntryByName(record->base.animName);

    if (strcmp(
        sectionToken,
        g_zEffectAnim_ActivationSectionName0
    ) == 0) {
        for (int i = 0; i < GetActivationRecordCount(); ++i) {
            zEffectAnimActivationRecord *const queued = GetActivationRecordAt(i);
            ResetFromActivationRecord(queued);
            zError::ReportOld(
                0x100,
                g_zEffect_SourceFile_ZeffAnimSaveC,
                0x190,
                g_zEffectAnim_ResetActivationRecordFmt,
                queued->animName
            );
        }

        ClearActivationRecords();
        for (int i_2482 = 1; i_2482 < g_zEffectAnim_EntryCount; ++i_2482) {
            zEffectAnimEntry *cursor = &g_zEffectAnim_EntryList[i_2482];
            while (cursor != 0) {
                cursor->flags &= ~0x4000u;
                cursor = cursor->runtimeSibling;
            }
        }
    }

    if (record->base.nodeToken >= 0 && entry != 0) {
        zClass_NodePartial *const rootNode = GameZ_ZBD::NodeIndexToPtr(record->base.nodeToken);
        zEffectAnimEntry *sibling = entry->runtimeSibling;
        while (sibling != 0 && entry->boundNode != rootNode) {
            entry = sibling;
            sibling = entry->runtimeSibling;
        }

        NodeActionCallback(
            entry,
            rootNode
        );
        entry = ProcessActivationRecord(&record->base);
    }

    if (entry == 0) {
        return;
    }

    entry->flags |= 0x4000u;
    const unsigned char savedState = record->savedActivationState;
    if (savedState == 2) {
        if (entry->activationState != 2) {
            NodeActionCallback(
                entry,
                GameZ_ZBD::NodeIndexToPtr(record->base.nodeToken)
            );
            entry = ProcessActivationRecord(&record->base);
            zError::ReportOld(
                0x100,
                g_zEffect_SourceFile_ZeffAnimSaveC,
                0x1b6,
                g_zEffectAnim_ProcessActivationRecordName
            );
        }
        if (record->base.nodeToken == -1) {
            memcpy(
                AllocActivationRecord(),
                &record->base,
                sizeof(zEffectAnimActivationRecord)
            );
        }
    }

    if (savedState == 3) {
        if (entry->activationState != 3) {
            zEffectAnim::Stop(entry);
            entry->activationState = 3;
        }
        if (record->base.nodeToken == -1) {
            memcpy(
                AllocActivationRecord(),
                &record->base,
                sizeof(zEffectAnimActivationRecord)
            );
        }
        zError::ReportOld(
            0x100,
            g_zEffect_SourceFile_ZeffAnimSaveC,
            0x1c9,
            g_zEffectAnim_StateExecutedMsg
        );
    }

    if (savedState == 1) {
        if (entry->activationState != 1) {
            if (entry->activationState == 4) {
                entry->activationState = 3;
            }
            NodeActionCallback(
                entry,
                GameZ_ZBD::NodeIndexToPtr(record->base.nodeToken)
            );
            zError::ReportOld(
                0x100,
                g_zEffect_SourceFile_ZeffAnimSaveC,
                0x1d3,
                g_zEffectAnim_ResetFunctionName
            );
        }
        if (record->base.nodeToken == -1) {
            memcpy(
                AllocActivationRecord(),
                &record->base,
                sizeof(zEffectAnimActivationRecord)
            );
        }
    }

    if (savedState == 6) {
        if (entry->activationState != 6) {
            if (entry->activationState == 4) {
                entry->activationState = 3;
            }
            NodeActionCallback(
                entry,
                GameZ_ZBD::NodeIndexToPtr(record->base.nodeToken)
            );
            entry = ProcessActivationRecord(&record->base);
            zError::ReportOld(
                0x100,
                g_zEffect_SourceFile_ZeffAnimSaveC,
                0x1e4,
                g_zEffectAnim_ProcessActivationRecordName
            );
        }
        if (record->base.nodeToken == -1) {
            memcpy(
                AllocActivationRecord(),
                &record->base,
                sizeof(zEffectAnimActivationRecord)
            );
        }
    }

    if (savedState == 4) {
        if (entry->activationState != 4) {
            zEffectAnim::Stop(entry);
            entry->activationState = 4;
            zError::ReportOld(
                0x100,
                g_zEffect_SourceFile_ZeffAnimSaveC,
                0x1f5,
                g_zEffectAnim_StateInvalidMsg
            );
        }
        if (record->base.nodeToken == -1) {
            memcpy(
                AllocActivationRecord(),
                &record->base,
                sizeof(zEffectAnimActivationRecord)
            );
        }
    }

    for (int i = 0; i < record->trackedNodeCount; ++i) {
        zEffectAnimTrackedNodeSaveRecord *const tracked = &record->trackedNodes[i];
        zClass_NodePartial *const node = GameZ_ZBD::NodeIndexToPtr(tracked->nodeIndex);
        if (node == 0 || node->classId != 5) {
            continue;
        }

        zError::ReportOld(
            0x100,
            g_zEffect_SourceFile_ZeffAnimSaveC,
            0x201,
            g_zEffectAnim_RestoreNodeFmt,
            node,
            tracked->activeFlag
        );
        zClass_Class::gwNodeSetActive(
            node,
            tracked->activeFlag != 0 ? 1 : 0
        );
        if (tracked->activeFlag != 0) {
            if (tracked->usesCachedMatrix != 0) {
                zClass_Object3D::gwObject3DSetMatrix(
                    node,
                    tracked->transform
                );
            } else {
                zClass_Object3D::gwObject3DSetPosition(
                    node,
                    tracked->transform[0],
                    tracked->transform[1],
                    tracked->transform[2]
                );
                zClass_Object3D::gwObject3DSetRotation(
                    node,
                    tracked->transform[3],
                    tracked->transform[4],
                    tracked->transform[5]
                );
                zClass_Object3D::gwObject3DSetScale(
                    node,
                    tracked->transform[6],
                    tracked->transform[7],
                    tracked->transform[8]
                );
            }
        }

        if (node->userDataOrDiRef != 0 && (tracked->diFlagBits & 1) != 0) {
            unsigned int *const di = (unsigned int *)(node->userDataOrDiRef);
            di[1] = (di[1] & ~0x08u) | 0x08u;
            di[8] = (unsigned int)(tracked->diUserValue);
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.allocactivationrecord
 * @recoil-artifact defines .text recoil:function:0x460ae0: zEffect_Anim::AllocActivationRecord.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: allocate or grow the activation-record queue and return the next slot.
 */
zEffectAnimActivationRecord *AllocActivationRecord() {
    zEffectAnimActivationRecord *recordTable = g_zEffectAnim_ActivationRecordTable;
    int recordCount = 0;
    int recordCapacity = 0;

    if (recordTable == 0) {
        recordTable = (zEffectAnimActivationRecord *)(malloc(
            sizeof(zEffectAnimActivationRecord) * kInitialActivationRecordCapacity
        ));
        recordCapacity = kInitialActivationRecordCapacity;
        recordCount = 0;
        g_zEffectAnim_ActivationRecordTable = recordTable;
        g_zEffectAnim_ActivationRecordCapacity = recordCapacity;
        g_zEffectAnim_ActivationRecordCount = 0;
    } else {
        recordCount = g_zEffectAnim_ActivationRecordCount;
        recordCapacity = g_zEffectAnim_ActivationRecordCapacity;
    }

    if (recordCount >= recordCapacity) {
        zEffectAnimActivationRecord *const oldTable = recordTable;
        recordTable = (zEffectAnimActivationRecord *)(malloc(
            sizeof(zEffectAnimActivationRecord) * recordCapacity * 2
        ));
        g_zEffectAnim_ActivationRecordTable = recordTable;
        memcpy(
            recordTable,
            oldTable,
            sizeof(zEffectAnimActivationRecord) * recordCapacity
        );
        g_zEffectAnim_ActivationRecordCapacity = recordCapacity * 2;
        free(oldTable);
        recordCount = g_zEffectAnim_ActivationRecordCount;
    }

    zEffectAnimActivationRecord *const record = &recordTable[recordCount];
    record->recordId =
        (g_zEffectAnim_ActivationDispatchTagHigh & (unsigned int)(recordCount)) & 0x00ffffffu;
    g_zEffectAnim_ActivationRecordCount = recordCount + 1;
    return record;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.saverunninganimrecord
 * @recoil-artifact defines .text recoil:function:0x460bc0: zEffect_Anim::SaveRunningAnimRecord.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: write one running animation entry, runtime sequence state, and
 * attached light/sound refs into a temporary ZBD section stream.
 */
int __fastcall SaveRunningAnimRecord(
    zZbdSectionCallbackCtx *callbackCtx,
    zEffectAnimEntry *entry,
    int runningIndex,
    int includePrimaryEntry
) {
    zEffectAnimRunningSaveHeader header = {0};
    header.entryTableIndex = runningIndex;
    header.matchSavedRootNode = includePrimaryEntry;
    strncpy(
        header.entryName,
        entry->name,
        sizeof(header.entryName)
    );
    header.rootNodeIndex = zClass::NodePtrToValidatedIndex(entry->boundNode);
    header.nodeRefAIndex = zClass::NodePtrToValidatedIndex(
        (zClass_NodePartial *)((unsigned int)(entry->resetScratch[0]))
    );
    memcpy(
        &header.refVecA.x,
        &entry->resetScratch[1],
        sizeof(header.refVecA.x)
    );
    memcpy(
        &header.refVecA.y,
        &entry->resetScratch[2],
        sizeof(header.refVecA.y)
    );
    memcpy(
        &header.refVecA.z,
        &entry->resetScratch[3],
        sizeof(header.refVecA.z)
    );
    header.nodeRefBIndex = zClass::NodePtrToValidatedIndex(
        (zClass_NodePartial *)((unsigned int)(entry->resetScratch[4]))
    );
    memcpy(
        &header.refVecB.x,
        &entry->resetScratch[5],
        sizeof(header.refVecB.x)
    );
    memcpy(
        &header.refVecB.y,
        &entry->resetScratch[6],
        sizeof(header.refVecB.y)
    );
    memcpy(
        &header.refVecB.z,
        &entry->resetScratch[7],
        sizeof(header.refVecB.z)
    );
    header.activationState = entry->activationState;
    header.triggerCurrentValue = entry->triggerCurrentValue;
    header.activationCountdown = entry->activationCountdown;
    header.velocityX = entry->velocityX;
    header.velocityY = entry->velocityY;
    header.velocityZ = entry->velocityZ;
    header.runtimeSurfaceCount = entry->runtimeSequenceCount;
    header.lightRefCount = entry->lightRefCount;
    header.soundRefCount = entry->soundRefCount;

    char sectionName[0x14];
    sprintf(
        sectionName,
        g_zEffectAnim_RunningSectionNameFmt,
        runningIndex
    );

    FILE *const tempStream = zUtil_ZBD::OpenTempWriteStream();
    if (tempStream == 0) {
        return (int)(callbackCtx);
    }

    int result = fwrite(
        &header,
        sizeof(header),
        1,
        tempStream
    ) == 1 ? 1 : 0;
    for (int i_2646 = 0; result != 0 && i_2646 < entry->runtimeSequenceCount; ++i_2646) {
        zEffectAnimSurfaceRuntime runtimeCopy = entry->runtimeList[i_2646];
        runtimeCopy.currentEvent = (void *)((unsigned char *)(runtimeCopy.currentEvent) -
                                            (unsigned char *)(runtimeCopy.eventStream));
        result = fwrite(
            &runtimeCopy,
            sizeof(runtimeCopy),
            1,
            tempStream
        ) == 1 ? 1 : 0;

        const int eventStreamSize = entry->runtimeList[i_2646].eventStreamSize;
        if (eventStreamSize > 0) {
            result =
                fwrite(
                    entry->runtimeList[i_2646].eventStream,
                    eventStreamSize,
                    1,
                    tempStream
                ) == 1
                    ? 1
                    : 0;
        }
    }

    for (int i_2660 = 0; result != 0 && i_2660 < entry->lightRefCount; ++i_2660) {
        zEffectAnimRuntimeNodeSaveRecord record = {0};
        zEffectAnimRuntimeNodeRef *const lightRef = &entry->lightRefList[i_2660];
        strncpy(
            record.name,
            lightRef->name.text,
            sizeof(record.name)
        );
        record.isAttached = lightRef->isAttached;
        zClass_NodePartial *const node = lightRef->runtimeNode;
        if (node != 0) {
            // Original 0x460bc0 uses this shared position helper for saved light refs too.
            zClass_Sound::gwSoundGetPosition(
                node,
                &record.posX,
                &record.posY,
                &record.posZ
            );
            record.parentNodeIndex =
                node->listCountA > 0 ? zClass::NodePtrToValidatedIndex(node->listA[0]) : -1;
        }
        result = fwrite(
            &record,
            sizeof(record),
            1,
            tempStream
        ) == 1 ? 1 : 0;
    }

    for (int i_2679 = 0; result != 0 && i_2679 < entry->soundRefCount; ++i_2679) {
        zEffectAnimSoundNodeSaveRecord record = {0};
        zEffectAnimRuntimeNodeRef *const soundRef = &entry->soundRefList[i_2679];
        strncpy(
            record.name,
            soundRef->name.text,
            sizeof(record.name)
        );
        record.isAttached = soundRef->isAttached;
        zClass_NodePartial *const node = soundRef->runtimeNode;
        if (node != 0) {
            zClass_SoundDataPartial *const soundData = (zClass_SoundDataPartial *)(node->classData);
            record.hasPosition = (soundData->runtimeFlags >> 1) & 1;
            zClass_Sound::gwSoundGetPosition(
                node,
                &record.posX,
                &record.posY,
                &record.posZ
            );
            record.parentNodeIndex =
                node->listCountA > 0 ? zClass::NodePtrToValidatedIndex(node->listA[0]) : -1;
        }
        result = fwrite(
            &record,
            sizeof(record),
            1,
            tempStream
        ) == 1 ? 1 : 0;
    }

    zUtil_ZBD::FlushTempWriteStreamToSectionRecord(
        tempStream,
        callbackCtx,
        sectionName
    );
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.saverunninganimrecords
 * @recoil-artifact defines .text recoil:function:0x460f80: zEffect_Anim::SaveRunningAnimRecords.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: enumerate active animation entries and cloned siblings that need
 * running-state persistence.
 */
int __fastcall SaveRunningAnimRecords(
    zZbdSectionCallbackCtx *callbackCtx
) {
    int result = 1;
    for (int i = 1; result != 0 && i < g_zEffectAnim_EntryCount; ++i) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i];
        if (entry != 0 && (entry->activationState == 2 || entry->activationState == 6)) {
            const unsigned short flags = (unsigned short)(entry->flags);
            if (((flags & 0x1000) == 0 || (flags & 0x2000) != 0) &&
                g_zEffectAnim_RecordQueueEnabled != 0) {
                result = SaveRunningAnimRecord(
                    callbackCtx,
                    entry,
                    i,
                    1
                );
                zEffectAnimEntry *sibling = entry->runtimeSibling;
                while (result != 0 && sibling != 0) {
                    if (sibling->activationState == 2) {
                        result = SaveRunningAnimRecord(
                            callbackCtx,
                            sibling,
                            i,
                            0
                        );
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
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: restore one running animation entry, sequence event streams, reset
 * scratch refs, and attached light/sound refs from a ZAR section.
 */
void __fastcall LoadRunningAnimRecords(
    void *unused,
    const char *sectionToken,
    void *data,
    int dataSize,
    void *extraCtx
) {
    (void)unused;
    (void)sectionToken;
    (void)extraCtx;

    FILE *const tempStream = zUtil_ZBD::OpenTempReadStream(
        data,
        dataSize
    );
    if (tempStream == 0) {
        return;
    }

    zEffectAnimRunningSaveHeader header = {0};
    fread(
        &header,
        sizeof(header),
        1,
        tempStream
    );

    zEffectAnimEntry *entry = &g_zEffectAnim_EntryList[header.entryTableIndex];
    zClass_NodePartial *const rootNode = GameZ_ZBD::NodeIndexToPtr(header.rootNodeIndex);

    if (header.matchSavedRootNode != 0) {
        zEffectAnimEntry *sibling = entry->runtimeSibling;
        if (sibling != 0) {
            while (entry->boundNode != rootNode) {
                entry = sibling;
                sibling = entry->runtimeSibling;
                if (sibling == 0) {
                    break;
                }
            }
        }

        if (entry->boundNode != rootNode) {
            sibling = entry->runtimeSibling;
            if (sibling != 0) {
                while (entry->activationState == 2) {
                    entry = sibling;
                    sibling = entry->runtimeSibling;
                    if (sibling == 0) {
                        break;
                    }
                }
            }

            if (entry->activationState == 2) {
                entry->runtimeSibling = zEffectAnim::CloneEntryForNode(
                    entry,
                    rootNode
                );
            }
        }
    }

    entry->flags |= 0x4000u;
    entry->resetScratch[0] =
        (unsigned int)((unsigned int)(GameZ_ZBD::NodeIndexToPtr(header.nodeRefAIndex)));
    memcpy(
        &entry->resetScratch[1],
        &header.refVecA.x,
        sizeof(header.refVecA.x)
    );
    memcpy(
        &entry->resetScratch[2],
        &header.refVecA.y,
        sizeof(header.refVecA.y)
    );
    memcpy(
        &entry->resetScratch[3],
        &header.refVecA.z,
        sizeof(header.refVecA.z)
    );
    entry->resetScratch[4] =
        (unsigned int)((unsigned int)(GameZ_ZBD::NodeIndexToPtr(header.nodeRefBIndex)));
    memcpy(
        &entry->resetScratch[5],
        &header.refVecB.x,
        sizeof(header.refVecB.x)
    );
    memcpy(
        &entry->resetScratch[6],
        &header.refVecB.y,
        sizeof(header.refVecB.y)
    );
    memcpy(
        &entry->resetScratch[7],
        &header.refVecB.z,
        sizeof(header.refVecB.z)
    );
    entry->activationState = header.activationState;
    entry->triggerCurrentValue = header.triggerCurrentValue;
    entry->activationCountdown = header.activationCountdown;
    entry->velocityX = header.velocityX;
    entry->velocityY = header.velocityY;
    entry->velocityZ = header.velocityZ;
    entry->runtimeSequenceCount = header.runtimeSurfaceCount;

    for (int i = 0; i < entry->runtimeSequenceCount; ++i) {
        zEffectAnimSurfaceRuntime *const runtime = &entry->runtimeList[i];
        if (runtime->eventStream != 0) {
            free(runtime->eventStream);
            runtime->eventStream = 0;
        }

        fread(
            runtime,
            sizeof(*runtime),
            1,
            tempStream
        );

        if (runtime->eventStreamSize > 0) {
            const unsigned int currentEventOffset = (unsigned int)(runtime->currentEvent);
            void *const eventStream = malloc(runtime->eventStreamSize);
            runtime->currentEvent = (unsigned char *)(eventStream) + currentEventOffset;
            runtime->eventStream = eventStream;
            fread(
                runtime->eventStream,
                runtime->eventStreamSize,
                1,
                tempStream
            );
        }
    }

    for (int i_2831 = 0; i_2831 < header.lightRefCount; ++i_2831) {
        zEffectAnimRuntimeNodeSaveRecord record = {0};
        fread(
            &record,
            sizeof(record),
            1,
            tempStream
        );

        if (record.isAttached == 0) {
            continue;
        }

        const int lightIndex = zEffectAnim::FindOrCreateLightRef(
            entry,
            record.name
        );
        if (lightIndex < 0) {
            continue;
        }

        zEffectAnimRuntimeNodeRef *const lightRef = &entry->lightRefList[lightIndex];
        zClass_NodePartial *const lightNode = lightRef->runtimeNode;
        if (lightRef->isAttached != 0 || lightNode == 0) {
            continue;
        }

        zClass_Class::gwNodeSetActive(
            lightNode,
            1
        );
        if (record.parentNodeIndex >= 0 && lightNode->listCountA == 0) {
            zClass_Class::AddChild(
                GameZ_ZBD::NodeIndexToPtr(record.parentNodeIndex),
                lightNode
            );
        }
        zClass_Light::gwLightSetPosition(
            lightNode,
            record.posX,
            record.posY,
            record.posZ
        );
        zClass_World::AddLight(
            g_zEffect_World,
            lightNode
        );
        lightRef->isAttached = 1;
    }

    for (int i_2861 = 0; i_2861 < header.soundRefCount; ++i_2861) {
        zEffectAnimSoundNodeSaveRecord record = {0};
        fread(
            &record,
            sizeof(record),
            1,
            tempStream
        );

        if (record.isAttached == 0) {
            continue;
        }

        const int soundIndex = zEffectAnim::FindOrCreateSoundRef(
            entry,
            record.name
        );
        if (soundIndex < 0) {
            continue;
        }

        zEffectAnimRuntimeNodeRef *const soundRef = &entry->soundRefList[soundIndex];
        zClass_NodePartial *const soundNode = soundRef->runtimeNode;
        if (soundRef->isAttached != 0 || soundNode == 0) {
            continue;
        }

        zClass_Class::gwNodeSetActive(
            soundNode,
            1
        );
        if (record.parentNodeIndex >= 0 && soundNode->listCountA == 0) {
            zClass_Class::AddChild(
                GameZ_ZBD::NodeIndexToPtr(record.parentNodeIndex),
                soundNode
            );
        }
        if (record.hasPosition != 0) {
            zClass_Sound::gwSoundSetPosition(
                soundNode,
                record.posX,
                record.posY,
                record.posZ
            );
        }
        zClass_World::AddSound(
            g_zEffect_World,
            soundNode
        );
        soundRef->isAttached = 1;
    }

    entry->runtimeNode->callbackContext = (zClass_NodePartial *)(entry);
    zClass_Class::gwNodeSetActionCallbackTail(
        entry->runtimeNode,
        (void *)(&zEffect_Anim::RunSequence)
    );
    zUtil_ZBD::CloseTempReadStream(tempStream);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.saveanimrecords
 * @recoil-artifact defines .text recoil:function:0x461430: zEffect_Anim::SaveAnimRecords.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: serialize non-running animation activation state and tracked-node
 * transforms into numbered Anim sections.
 */
int __fastcall SaveAnimRecords(
    zZbdSectionCallbackCtx *callbackCtx
) {
    int result = 1;
    for (int i = 1; result != 0 && i < g_zEffectAnim_EntryCount; ++i) {
        zEffectAnimEntry *const entry = &g_zEffectAnim_EntryList[i];

        zEffectAnimSaveRecord saveRecord = {0};
        zEffectAnimSaveHeader *const header = &saveRecord.header;
        unsigned char trackedNodeCount = 0;
        if (entry == 0) {
            strncpy(
                header->base.animName,
                g_zEffect_StringNone,
                sizeof(header->base.animName)
            );
        } else {
            if (entry->activationState == 5) {
                continue;
            }

            const unsigned short flags = (unsigned short)(entry->flags);
            if (((flags & 0x1000) != 0 && (flags & 0x2000) == 0) ||
                g_zEffectAnim_RecordQueueEnabled == 0) {
                continue;
            }

            if (entry->trackedNodeList != 0) {
                trackedNodeCount = entry->trackedNodeCount;
            }

            strncpy(
                header->base.animName,
                entry->name,
                sizeof(header->base.animName)
            );
            header->entryTableIndex = i;
            header->savedActivationState = entry->activationState;
            header->trackedNodeCount = trackedNodeCount;
        }

        zEffectAnimTrackedNodeSaveRecord *const records = saveRecord.trackedNodes;
        {
            for (int childIndex = 0; childIndex < trackedNodeCount; ++childIndex) {
                zEffectAnimTrackedNodeSaveRecord *const record = &records[childIndex];
                zClass_NodePartial *const node = entry->trackedNodeList[childIndex].trackedNode;
                if (node == 0 || node->classId != 5) {
                    record->nodeIndex = -1;
                    continue;
                }

                zClass_Object3DDataPartial *const objectData =
                    (zClass_Object3DDataPartial *)(node->classData);
                record->nodeIndex = zClass::NodePtrToValidatedIndex(node);
                record->activeFlag = (node->flags >> 2) & 1;
                record->usesCachedMatrix = (objectData->flags >> 4) & 1;
                if (record->usesCachedMatrix != 0) {
                    memcpy(
                        record->transform,
                        zClass_Object3D::gwObject3DGetMatrixPtr(node),
                        sizeof(record->transform)
                    );
                } else {
                    zClass_Object3D::gwObject3DGetPosition(
                        node,
                        &record->transform[0],
                        &record->transform[1],
                        &record->transform[2]
                    );
                    zClass_Object3D::gwObject3DGetRotation(
                        node,
                        &record->transform[3],
                        &record->transform[4],
                        &record->transform[5]
                    );
                    zClass_Object3D::gwObject3DGetScale(
                        node,
                        &record->transform[6],
                        &record->transform[7],
                        &record->transform[8]
                    );
                }
            }
        }

        const unsigned int payloadSize =
            sizeof(zEffectAnimSaveHeader) +
            sizeof(zEffectAnimTrackedNodeSaveRecord) * trackedNodeCount;
        char sectionName[0x14];
        sprintf(
            sectionName,
            g_zEffectAnim_AnimSectionNameFmt,
            i + g_zEffectAnim_ActivationRecordCount
        );
        result = zUtil_ZAR::WriteSectionBlob(
            callbackCtx,
            sectionName,
            &saveRecord,
            payloadSize
        );
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.loadanimrecords
 * @recoil-artifact defines .text recoil:function:0x461670: zEffect_Anim::LoadAnimRecords.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: restore non-running animation activation state and tracked-node
 * transforms from a saved Anim section.
 */
void __fastcall LoadAnimRecords(
    void *unused,
    const char *sectionToken,
    void *data,
    int dataSize,
    void *extraCtx
) {
    (void)unused;
    (void)sectionToken;
    (void)dataSize;
    (void)extraCtx;

    zEffectAnimSaveHeader *const header = (zEffectAnimSaveHeader *)(data);
    zEffectAnimEntry *entry = &g_zEffectAnim_EntryList[header->entryTableIndex];
    if (entry == 0 || (entry->flags & 0x4000u) != 0) {
        return;
    }

    if (header->savedActivationState == 1 && entry->activationState != 1) {
        if (entry->activationState == 4) {
            entry->activationState = 3;
        }
        NodeActionCallback(
            entry,
            entry->boundNode
        );
        zError::ReportOld(
            0x100,
            g_zEffect_SourceFile_ZeffAnimSaveC,
            0x419,
            g_zEffectAnim_ResetTraceFmt,
            entry
        );
    }

    for (zEffectAnimEntry *cursor = entry; cursor != 0; cursor = cursor->runtimeSibling) {
        if ((cursor->flags & 0x4000u) == 0 && cursor->activationState == 2) {
            NodeActionCallback(
                cursor,
                cursor->boundNode
            );
            zError::ReportOld(
                0x100,
                g_zEffect_SourceFile_ZeffAnimSaveC,
                0x41f,
                g_zEffectAnim_ResetTraceFmt,
                cursor
            );
        }
    }

    zEffectAnimTrackedNodeSaveRecord *records =
        (zEffectAnimTrackedNodeSaveRecord *)((unsigned char *)(data) + sizeof(*header));
    for (int i = 0; i < header->trackedNodeCount; ++i) {
        zEffectAnimTrackedNodeSaveRecord *const record = &records[i];
        zClass_NodePartial *const node = GameZ_ZBD::NodeIndexToPtr(record->nodeIndex);
        if (node == 0 || node->classId != 5) {
            continue;
        }

        zError::ReportOld(
            0x100,
            g_zEffect_SourceFile_ZeffAnimSaveC,
            0x426,
            g_zEffectAnim_RestoreNodeFmt,
            node,
            record->activeFlag
        );
        zClass_Class::gwNodeSetActive(
            node,
            record->activeFlag != 0 ? 1 : 0
        );
        if (record->activeFlag == 0) {
            continue;
        }

        if (record->usesCachedMatrix != 0) {
            zClass_Object3D::gwObject3DSetMatrix(
                node,
                record->transform
            );
        } else {
            zClass_Object3D::gwObject3DSetPosition(
                node,
                record->transform[0],
                record->transform[1],
                record->transform[2]
            );
            zClass_Object3D::gwObject3DSetRotation(
                node,
                record->transform[3],
                record->transform[4],
                record->transform[5]
            );
            zClass_Object3D::gwObject3DSetScale(
                node,
                record->transform[6],
                record->transform[7],
                record->transform[8]
            );
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.getactivationrecordpackedsize
 * @recoil-artifact defines .text recoil:function:0x461800: zEffect_Anim::GetActivationRecordPackedSize.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: return the serialized byte count for an activation record command type.
 */
int __fastcall GetActivationRecordPackedSize(
    zEffectAnimActivationRecord *record
) {
    switch (record->commandType) {
    case 1:
        return 0x38;
    case 2:
        return 0x48;
    case 3:
        return 0x4c;
    default:
        return 0x50;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.resetfromactivationrecord
 * @recoil-artifact defines .text recoil:function:0x461840: zEffect_Anim::ResetFromActivationRecord.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: restart the named animation against the node stored in an activation record.
 */
void __fastcall ResetFromActivationRecord(
    zEffectAnimActivationRecord *record
) {
    NodeActionCallback(
        zEffectAnim::FindEntryByName(record->animName),
        GameZ_ZBD::NodeIndexToPtr(record->nodeToken)
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.processactivationrecord
 * @recoil-artifact defines .text recoil:function:0x461870: zEffect_Anim::ProcessActivationRecord.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: dispatch a queued activation record to the matching animation
 * activation command.
 */
zEffectAnimEntry *__fastcall ProcessActivationRecord(
    zEffectAnimActivationRecord *record
) {
    zEffectAnimEntry *const entry = zEffectAnim::FindEntryByName(record->animName);
    if (entry == 0) {
        return 0;
    }

    zClass_NodePartial *const rootNode = GameZ_ZBD::NodeIndexToPtr(record->nodeToken);
    switch (record->commandType) {
    case 1:
        return zEffectAnim::SetTransformRotAndVelocity_Thunk(
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

    case 2:
        return zEffectAnim::SetVelocity_Thunk(
            entry,
            rootNode,
            record->params[0].f32,
            record->params[1].f32,
            record->params[2].f32
        );

    case 3:
        return zEffectAnim::SetPositionRefAndVelocity_Thunk(
            entry,
            rootNode,
            GameZ_ZBD::NodeIndexToPtr(record->params[0].i32),
            (const zVec3 *)(&record->params[1]),
            (const zVec3 *)(&record->params[4])
        );

    case 4:
        return zEffectAnim::SetTransformRefs_Thunk(
            entry,
            rootNode,
            GameZ_ZBD::NodeIndexToPtr(record->params[0].i32),
            (const zVec3 *)(&record->params[1]),
            GameZ_ZBD::NodeIndexToPtr(record->params[4].i32),
            (const zVec3 *)(&record->params[5])
        );

    default:
        return 0;
    }
}

} // namespace zEffect_Anim

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.queuecmdtype1transformrotvelocity
 * @recoil-artifact defines .text recoil:function:0x461970: zEffectAnim::QueueCmdType1TransformRotVelocity.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim_activation.c.
 * Purpose: build the command type 1 activation record for transform, rotation,
 * and velocity activation and apply the queue/dispatch gates.
 */
zEffectAnimActivationRecord *__fastcall QueueCmdType1TransformRotVelocity(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    float posX,
    float posY,
    float posZ,
    float rotX,
    float rotY,
    float rotZ,
    float velocityX,
    float velocityY,
    float velocityZ
) {
    zEffectAnimActivationRecord *result = 0;
    const int boundNodeToken = zClass::NodePtrToValidatedIndex(boundNode);
    const unsigned int flags = self->flags;

    int recordQueueRequested = 0;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 &&
               (flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0' &&
               (boundNode == 0 || boundNodeToken > 0)) {
        recordQueueRequested = 1;
    }

    int dispatchRequested = 0;
    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 &&
               (flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0' &&
               (boundNode == 0 || boundNodeToken > 0)) {
        dispatchRequested = 1;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord *const record = zEffect_Anim::AllocActivationRecord();
        record->commandType = 1;
        strncpy(
            record->animName,
            self->name,
            sizeof(record->animName)
        );
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
            zEffect_Anim::DiscardLastActivationRecord();
        }
    }

    return result;
}

} // namespace zEffectAnim

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.discardlastactivationrecord
 * @recoil-artifact defines .text recoil:function:0x461a90: zEffect_Anim::DiscardLastActivationRecord.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_anim_save.c.
 * Purpose: remove the most recently allocated activation record from the queue.
 */
void DiscardLastActivationRecord() {
    --g_zEffectAnim_ActivationRecordCount;
}

} // namespace zEffect_Anim

namespace zEffectAnim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.queuecmdtype2velocity
 * @recoil-artifact defines .text recoil:function:0x461aa0: zEffectAnim::QueueCmdType2Velocity.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim_activation.c.
 * Purpose: build the command type 2 activation record for velocity-only
 * activation and apply the queue/dispatch gates.
 */
zEffectAnimActivationRecord *__fastcall QueueCmdType2Velocity(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    float velocityX,
    float velocityY,
    float velocityZ
) {
    zEffectAnimActivationRecord *result = 0;
    const int boundNodeToken = zClass::NodePtrToValidatedIndex(boundNode);
    const unsigned int flags = self->flags;

    int recordQueueRequested = 0;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 &&
               (flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0' &&
               (boundNode == 0 || boundNodeToken > 0)) {
        recordQueueRequested = 1;
    }

    int dispatchRequested = 0;
    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 &&
               (flags & kActivationRecordNoQueueDispatchFlag) == 0 && self->name[0] != '\0' &&
               (boundNode == 0 || boundNodeToken > 0)) {
        dispatchRequested = 1;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord *const record = zEffect_Anim::AllocActivationRecord();
        record->commandType = 2;
        strncpy(
            record->animName,
            self->name,
            sizeof(record->animName)
        );
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
            zEffect_Anim::DiscardLastActivationRecord();
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.queuecmdtype3positionrefandvelocity
 * @recoil-artifact defines .text recoil:function:0x461ba0: zEffectAnim::QueueCmdType3PositionRefAndVelocity.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim_activation.c.
 * Purpose: build the command type 3 activation record for a position reference
 * plus velocity and apply the queue/dispatch gates.
 */
zEffectAnimActivationRecord *__fastcall QueueCmdType3PositionRefAndVelocity(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    zClass_NodePartial *refNode,
    const zVec3 *refVec,
    const zVec3 *velocityVec
) {
    zEffectAnimActivationRecord *result = 0;
    const int boundNodeToken = zClass::NodePtrToValidatedIndex(boundNode);
    const int refNodeToken = zClass::NodePtrToValidatedIndex(refNode);
    const unsigned int flags = self->flags;
    const bool refsValid = (flags & kActivationRecordNoQueueDispatchFlag) == 0 &&
                           self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0) &&
                           (refNode == 0 || refNodeToken > 0);

    int recordQueueRequested = 0;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (refsValid) {
        recordQueueRequested = 1;
    }

    int dispatchRequested = 0;
    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (refsValid) {
        dispatchRequested = 1;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord *const record = zEffect_Anim::AllocActivationRecord();
        record->commandType = 3;
        strncpy(
            record->animName,
            self->name,
            sizeof(record->animName)
        );
        record->nodeToken = boundNodeToken;
        record->params[0].i32 = refNodeToken;
        if (refVec != 0) {
            record->params[1].f32 = refVec->x;
            record->params[2].f32 = refVec->y;
            record->params[3].f32 = refVec->z;
        } else {
            record->params[1].u32 = 0;
            record->params[2].u32 = 0;
            record->params[3].u32 = 0;
        }

        if (velocityVec != 0) {
            record->params[4].f32 = velocityVec->x;
            record->params[5].f32 = velocityVec->y;
            record->params[6].f32 = velocityVec->z;
        } else {
            record->params[4].u32 = 0;
            record->params[5].u32 = 0;
            record->params[6].u32 = 0;
        }

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            zEffect_Anim::DiscardLastActivationRecord();
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.queuecmdtype4transformrefs
 * @recoil-artifact defines .text recoil:function:0x461d00: zEffectAnim::QueueCmdType4TransformRefs.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_anim_activation.c.
 * Purpose: build the command type 4 activation record for two transform
 * references and apply the queue/dispatch gates.
 */
zEffectAnimActivationRecord *__fastcall QueueCmdType4TransformRefs(
    zEffectAnimEntry *self,
    zClass_NodePartial *boundNode,
    zClass_NodePartial *refNodeA,
    const zVec3 *refVecA,
    zClass_NodePartial *refNodeB,
    const zVec3 *refVecB
) {
    zEffectAnimActivationRecord *result = 0;
    const int boundNodeToken = zClass::NodePtrToValidatedIndex(boundNode);
    const int refNodeAToken = zClass::NodePtrToValidatedIndex(refNodeA);
    const int refNodeBToken = zClass::NodePtrToValidatedIndex(refNodeB);
    const unsigned int flags = self->flags;
    const bool refsValid = (flags & kActivationRecordNoQueueDispatchFlag) == 0 &&
                           self->name[0] != '\0' && (boundNode == 0 || boundNodeToken > 0) &&
                           (refNodeA == 0 || refNodeAToken > 0) &&
                           (refNodeB == 0 || refNodeBToken > 0);

    int recordQueueRequested = 0;
    if ((flags & kActivationRecordQueueOverrideFlag) != 0) {
        recordQueueRequested = (flags & kActivationRecordQueueOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_RecordQueueEnabled != 0 && refsValid) {
        recordQueueRequested = 1;
    }

    int dispatchRequested = 0;
    if ((flags & kActivationRecordDispatchOverrideFlag) != 0) {
        dispatchRequested = (flags & kActivationRecordDispatchOverrideValue) != 0 ? 1 : 0;
    } else if (g_zEffectAnim_DispatchEnabled != 0 && refsValid) {
        dispatchRequested = 1;
    }

    if (recordQueueRequested != 0 || dispatchRequested != 0) {
        zEffectAnimActivationRecord *const record = zEffect_Anim::AllocActivationRecord();
        record->commandType = 4;
        strncpy(
            record->animName,
            self->name,
            sizeof(record->animName)
        );
        record->nodeToken = boundNodeToken;
        record->params[0].i32 = refNodeAToken;
        if (refVecA != 0) {
            record->params[1].f32 = refVecA->x;
            record->params[2].f32 = refVecA->y;
            record->params[3].f32 = refVecA->z;
        } else {
            record->params[1].u32 = 0;
            record->params[2].u32 = 0;
            record->params[3].u32 = 0;
        }

        record->params[4].i32 = refNodeBToken;
        if (refVecB != 0) {
            record->params[5].f32 = refVecB->x;
            record->params[6].f32 = refVecB->y;
            record->params[7].f32 = refVecB->z;
        } else {
            record->params[5].u32 = 0;
            record->params[6].u32 = 0;
            record->params[7].u32 = 0;
        }

        if (dispatchRequested != 0) {
            result = record;
            if (g_zEffectAnim_ActivationDispatchCallback != 0) {
                g_zEffectAnim_ActivationDispatchCallback(record);
            }
        }

        if (recordQueueRequested == 0) {
            zEffect_Anim::DiscardLastActivationRecord();
        }
    }

    return result;
}

} // namespace zEffectAnim

namespace zEffect_Anim {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.setactivationdispatchcontext
 * @recoil-artifact defines .text recoil:function:0x461eb0: zEffect_Anim::SetActivationDispatchContext.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zEffect.cpp.
 * Purpose: store the activation-dispatch callback and high-byte context tag.
 */
void __fastcall SetActivationDispatchContext(
    void(__fastcall *callback)(zEffectAnimActivationRecord *record),
    int context
) {
    g_zEffectAnim_ActivationDispatchCallback = callback;
    g_zEffectAnim_ActivationDispatchTagHigh = (unsigned int)(context) << 24;
}

} // namespace zEffect_Anim

namespace zEffect {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.findnodeuserdatarecursive
 * @recoil-artifact defines .text recoil:function:0x461ec0: zEffect::FindNodeUserDataRecursive.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: find the first non-null user-data value in a root-first node tree
 * traversal.
 */
void *__fastcall FindNodeUserDataRecursive(
    zClass_NodePartial *node
) {
    unsigned int userDataValue = 0;
    zClass_Class::gwNodeGetUserData(
        node,
        &userDataValue
    );
    if (userDataValue != 0) {
        return (void *)((unsigned int)(userDataValue));
    }

    for (int i = 0; i < node->listCountB; ++i) {
        void *const result = FindNodeUserDataRecursive(node->listB[i]);
        if (result != 0) {
            return result;
        }
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.spawnruntimeinstanceat
 * @recoil-artifact defines .text recoil:function:0x461f00: zEffect::SpawnRuntimeInstanceAt.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: acquire and activate a runtime effect entry at a world position,
 * then install its node action callback.
 */
int __fastcall SpawnRuntimeInstanceAt(
    int effectIndex,
    const zVec3 *worldPos
) {
    const int initialized = g_zEffect_RuntimeManager.initialized;
    if (initialized != 0 && effectIndex != -1) {
        zEffect_RuntimeEntry *const entry = AcquireRuntimeEntryByIndex(effectIndex);
        if (entry != 0) {
            ActivateRuntimeEntryAtPosition(
                entry,
                worldPos
            );
            zClass_Class::gwNodeSetActive(
                entry->effectNode,
                1
            );
            entry->effectNode->callbackContext = (zClass_NodePartial *)(entry);
            zClass_Class::gwNodeSetActionCallback(
                entry->effectNode,
                (void *)(&RuntimeNodeActionCallback)
            );
        }
    }

    return initialized;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.activateruntimeentryatposition
 * @recoil-artifact defines .text recoil:function:0x461f50: zEffect::ActivateRuntimeEntryAtPosition.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: initialize runtime scale, lifetime, fade, position, variant, and
 * parent attachment for one active effect instance.
 */
int __fastcall ActivateRuntimeEntryAtPosition(
    zEffect_RuntimeEntry *runtimeEntry,
    const zVec3 *worldPos
) {
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

    const float distanceSq = ComputeDistanceSqToListener(worldPos);
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

    const float currentScale = runtimeEntry->currentScale;
    zClass_Object3D::gwObject3DSetScale(
        runtimeEntry->effectNode,
        currentScale,
        currentScale,
        currentScale
    );
    zClass_Object3D::gwObject3DSetPosition(
        runtimeEntry->effectNode,
        worldPos->x,
        worldPos->y,
        worldPos->z
    );
    zDi::ResetCurrentVariant((zDiPartial *)(runtimeEntry->effectGfxData));
    zClass_Class::AddChild(
        g_zEffect_RuntimeManager.parentNode,
        runtimeEntry->effectNode
    );
    ++g_zEffect_RuntimeManager.activatedCount;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.computedistancesqtolistener
 * @recoil-artifact defines .text recoil:function:0x462050: zEffect::ComputeDistanceSqToListener.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: compute squared distance from the runtime listener node to a world
 * position.
 */
float __fastcall ComputeDistanceSqToListener(
    const zVec3 *worldPos
) {
    zVec3 listenerPosition = {0};
    gwNode::GetWorldPosition(
        g_zEffect_RuntimeManager.listenerNode,
        &listenerPosition
    );
    listenerPosition.x -= worldPos->x;
    listenerPosition.y -= worldPos->y;
    listenerPosition.z -= worldPos->z;
    return listenerPosition.x * listenerPosition.x + listenerPosition.y * listenerPosition.y +
           listenerPosition.z * listenerPosition.z;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.acquireruntimeentrybyindex
 * @recoil-artifact defines .text recoil:function:0x4620d0: zEffect::AcquireRuntimeEntryByIndex.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: reuse a free runtime effect entry for an index or clone a fresh
 * entry from its template.
 */
zEffect_RuntimeEntry *__fastcall AcquireRuntimeEntryByIndex(
    int effectIndex
) {
    if (effectIndex == -1) {
        return 0;
    }

    zEffect_RuntimeEntry *const payload = (zEffect_RuntimeEntry *)(zArchiveList_FindPayloadByValue(
        g_zEffect_RuntimeManager.freeList,
        (unsigned int)(effectIndex)
    ));
    if (payload != 0) {
        ++g_zEffect_RuntimeManager.recycleCount;
        zArchiveList_RemovePayload(
            g_zEffect_RuntimeManager.freeList,
            payload
        );
        return payload;
    }

    ++g_zEffect_RuntimeManager.freshAllocCount;
    return CloneRuntimeEntryFromTemplate(effectIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.cloneruntimeentryfromtemplate
 * @recoil-artifact defines .text recoil:function:0x462130: zEffect::CloneRuntimeEntryFromTemplate.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: allocate a runtime effect entry and copy its template node tree and
 * graphics data reference.
 */
zEffect_RuntimeEntry *__fastcall CloneRuntimeEntryFromTemplate(
    int effectIndex
) {
    if (effectIndex == -1) {
        return 0;
    }

    zEffect_RuntimeEntry *const templateEntry = &g_zEffect_RuntimeManager.templates[effectIndex];
    if (templateEntry->effectNode == 0) {
        return 0;
    }

    zEffect_RuntimeEntry *const clone =
        (zEffect_RuntimeEntry *)(malloc(sizeof(zEffect_RuntimeEntry)));
    memcpy(
        clone,
        templateEntry,
        sizeof(zEffect_RuntimeEntry)
    );

    zClass_NodePartial *const node = zClass_cls_util::CopyNodeWithCloneOptions(
        clone->effectNode,
        g_zEffect_CloneCopyMode,
        g_zEffect_CloneCopyChildrenMode
    );
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
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\eff_runtime.c.
 * Purpose: advance runtime effect fade timing and recycle the effect entry
 * after the instance has completed.
 */
int __fastcall RuntimeNodeActionCallback(
    zClass_NodePartial *node
) {
    if ((node->flags & 0x04) == 0) {
        return 0;
    }

    zEffect_RuntimeEntry *const runtimeEntry = (zEffect_RuntimeEntry *)(node->callbackContext);
    runtimeEntry->elapsedSec += g_FrameDeltaTimeSec;

    if (runtimeEntry->elapsedSec < runtimeEntry->fadeInTimeSec) {
        runtimeEntry->currentScale += runtimeEntry->fadeInScaleRate * g_FrameDeltaTimeSec;
        const float currentScale = runtimeEntry->currentScale;
        return zClass_Object3D::gwObject3DSetScale(
            node,
            currentScale,
            currentScale,
            currentScale
        );
    }

    if (runtimeEntry->elapsedSec < runtimeEntry->fadeOutStartTimeSec) {
        runtimeEntry->currentScale -= runtimeEntry->fadeOutScaleRate * g_FrameDeltaTimeSec;
        if (runtimeEntry->currentScale < 0.01f) {
            runtimeEntry->currentScale = 0.01f;
        }

        const float currentScale = runtimeEntry->currentScale;
        return zClass_Object3D::gwObject3DSetScale(
            node,
            currentScale,
            currentScale,
            currentScale
        );
    }

    node->callbackContext = 0;
    zArchiveList_PushBackPayload(
        g_zEffect_RuntimeManager.freeList,
        runtimeEntry
    );
    zClass_Class::gwNodeSetActionCallback(
        node,
        0
    );
    zClass_Class::gwNodeSetActive(
        node,
        0
    );
    const int parentCount = node->listCountA;
    if (parentCount != 0) {
        return zClass_Class::RemoveChild(
            g_zEffect_RuntimeManager.parentNode,
            node
        );
    }

    return parentCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-anim-save.findtemplateindexbyname
 * @recoil-artifact defines .text recoil:function:0x462280: zEffect::FindTemplateIndexByName.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zEffect\Effect.c.
 * Purpose: return the runtime template index whose effect name matches.
 */
int __fastcall FindTemplateIndexByName(
    const char *name
) {
    for (int i = 0; i < g_zEffect_RuntimeManager.templateCount; ++i) {
        if (strcmp(
            name,
            g_zEffect_RuntimeManager.templates[i].effectName
        ) == 0) {
            return i;
        }
    }

    return -1;
}

} // namespace zEffect
