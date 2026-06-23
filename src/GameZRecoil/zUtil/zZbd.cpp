#include "zZbd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * Reimplements data 0x56bf70: g_zUtil_ZbdManager.
 * BN types this zero-initialized 4-byte .data slot as the process-wide
 * zZbdManager pointer; ZBD_Init/ZBD_DestroyGlobalManager own its lifecycle
 * and the ZAR/ZBD wrapper helpers null-check it before forwarding work.
 * Purpose: store the active ZBD archive manager singleton.
 */
zZbdManager *g_zUtil_ZbdManager = 0;
/**
 * Reimplements data 0x4e3010: g_zUtil_SourceFile_ZutlZarCpp.
 * BN stores this initialized .data literal immediately before the
 * GetLastError format used by the ZAR archive-open diagnostic path.
 * Purpose: name the original zutl_zar.cpp source file in error reports.
 */
char g_zUtil_SourceFile_ZutlZarCpp[0x27] =
    "D:\\Proj\\GameZRecoil\\zUtil\\zutl_zar.cpp";
/**
 * Reimplements data 0x4e3038: g_zUtil_GetLastErrorFmt.
 * BN xrefs this adjacent literal from zIndexArchive::Init's CreateFileA
 * failure path while reporting ZAR load errors.
 * Purpose: format Win32 GetLastError diagnostics for failed archive opens.
 */
char g_zUtil_GetLastErrorFmt[0x19] = "GetLastError(0x%08x): %s";
/**
 * Reimplements data 0x4e48e8: g_zUtil_ZbdSectionRecordFmt.
 * BN stores this writable .data format literal and xrefs it only from
 * zZbdManager::WriteSectionRecord's section/token path formatting.
 * Purpose: format ZBD section record paths as "section/token".
 */
char g_zUtil_ZbdSectionRecordFmt[0x6] = "%s/%s";
}

namespace zUtil {
/**
 * Reimplements 0x4c0030: zUtil::ZBD_LoadEntriesGlobal
 * (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp).
 * Purpose: load ZBD entries through the active global ZBD manager when present.
 */
int __fastcall ZBD_LoadEntriesGlobal(
    const char *filename
) {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager == 0) {
        return 0;
    }

    return manager->LoadEntries(filename);
}

/**
 * Reimplements 0x4c0050: zUtil::ZAR_LoadFileGlobal
 * (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp).
 * Purpose: load a ZAR file through the active global ZBD manager when present.
 */
int __fastcall ZAR_LoadFileGlobal(
    const char *filepath
) {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager == 0) {
        return 0;
    }

    return manager->LoadZarFile(filepath);
}

/**
 * Reimplements 0x4c0070: zUtil::ZAR_RequestStopGlobal
 * (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp).
 * Purpose: request cooperative ZAR loading stop through the active manager.
 */
void ZAR_RequestStopGlobal() {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager != 0) {
        manager->RequestStop();
    }
}

/**
 * Reimplements 0x4c0100: zUtil::ZBD_Init
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: allocate and initialize the global ZBD manager and handler sentinel.
 */
int ZBD_Init() {
    zZbdManager *manager = new zZbdManager;
    if (manager != 0) {
        manager->allocatorByte = 0;
        zZbdSectionHandlerNode *sentinel = new zZbdSectionHandlerNode;
        sentinel->next = sentinel;
        sentinel->prev = sentinel;

        manager->sectionHandlerListSentinel = sentinel;
        manager->sectionHandlerCount = 0;
        manager->indexArchive.Reset();
        manager->tempBufferSize = 0;
        manager->tempBuffer = 0;
        manager->unknown_2c = 0;
        manager->stopRequested = 0;
    }

    g_zUtil_ZbdManager = manager;
    return 0;
}

/**
 * Reimplements 0x4c0180: zUtil::ZBD_DestroyGlobalManager
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: destroy and clear the active global ZBD manager.
 */
void ZBD_DestroyGlobalManager() {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager == 0) {
        return;
    }

    manager->Destroy();
    ::operator delete(manager);
    g_zUtil_ZbdManager = 0;
}
} // namespace zUtil

/**
 * Reimplements 0x4c0260: zZbdSectionHandler::CompareSortOrderLessThan
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: compare section handlers by ascending sort order.
 */
bool __fastcall zZbdSectionHandler::CompareSortOrderLessThan(
    const zZbdSectionHandler *nodeA,
    const zZbdSectionHandler *nodeB
) {
    return nodeA->sortOrder < nodeB->sortOrder;
}

/**
 * Reimplements 0x4c0b70: zZbdSectionHandlerList::Constructor
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support).
 * Purpose: initialize an empty section-handler list with a sentinel node.
 */
void zZbdSectionHandlerList::Constructor() {
    allocatorByte = 0;
    sentinel = new zZbdSectionHandlerNode;
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    count = 0;
}

/**
 * Reimplements 0x4c0b60: zZbdSectionHandlerList::Front
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support).
 * Purpose: return an iterator to the first section-handler list node.
 */
void zZbdSectionHandlerList::Front(
    zZbdSectionHandlerNode **outIter
) {
    *outIter = sentinel->next;
}

/**
 * Reimplements 0x4c0ba0: zZbdSectionHandlerList::Swap
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support).
 * Purpose: exchange sentinels and counts for two handler lists.
 */
void zZbdSectionHandlerList::Swap(
    zZbdSectionHandlerList *other
) {
    zZbdSectionHandlerNode *const oldSentinel = sentinel;
    sentinel = other->sentinel;
    other->sentinel = oldSentinel;

    const int oldCount = count;
    count = other->count;
    other->count = oldCount;
}

/**
 * Reimplements 0x4c0ce0: zZbdSectionHandlerList::SpliceThreeNodes
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support).
 * Purpose: splice a node range before the requested list position.
 */
void zZbdSectionHandlerList::SpliceThreeNodes(
    zZbdSectionHandlerNode *position,
    zZbdSectionHandlerList *source,
    zZbdSectionHandlerNode *first,
    zZbdSectionHandlerNode *last
) {
    (void)source;

    zZbdSectionHandlerNode *const lastPrev = last->prev;
    zZbdSectionHandlerNode *const firstPrev = first->prev;
    zZbdSectionHandlerNode *const positionPrev = position->prev;

    lastPrev->next = position;
    firstPrev->next = last;
    positionPrev->next = first;

    first->prev = positionPrev;
    last->prev = firstPrev;
    position->prev = lastPrev;
}

/**
 * Reimplements 0x4c0bd0: zZbdSectionHandlerList::Merge
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support).
 * Purpose: merge another sorted handler list into this list.
 */
void zZbdSectionHandlerList::Merge(
    zZbdSectionHandlerList *source
) {
    if (source == this) {
        return;
    }

    zZbdSectionHandlerNode *destNode = sentinel->next;
    zZbdSectionHandlerNode *sourceNode = source->sentinel->next;
    while (destNode != sentinel && sourceNode != source->sentinel) {
        if (zZbdSectionHandler::CompareSortOrderLessThan(
                &sourceNode->sectionHandler,
                &destNode->sectionHandler
            )) {
            zZbdSectionHandlerNode *const nextSourceNode = sourceNode->next;
            SpliceThreeNodes(
                destNode,
                source,
                sourceNode,
                nextSourceNode
            );
            sourceNode = nextSourceNode;
        } else {
            destNode = destNode->next;
        }
    }

    if (sourceNode != source->sentinel) {
        SpliceThreeNodes(
            sentinel,
            source,
            sourceNode,
            source->sentinel
        );
    }

    count += source->count;
    source->count = 0;
}

/**
 * Reimplements 0x4c01b0: zZbdManager::Destroy
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: release manager buffers, archive records, handler nodes, and sentinel.
 */
void zZbdManager::Destroy() {
    if (tempBuffer != 0) {
        ::operator delete(tempBuffer);
        tempBuffer = 0;
    }

    indexArchive.Destroy();

    zZbdSectionHandlerNode *const sentinel = sectionHandlerListSentinel;
    zZbdSectionHandlerNode *node = sentinel->next;
    while (node != sentinel) {
        zZbdSectionHandlerNode *const next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;
        ::operator delete(node);
        --sectionHandlerCount;
        node = next;
    }

    ::operator delete(sectionHandlerListSentinel);
    sectionHandlerListSentinel = 0;
    sectionHandlerCount = 0;
}

/**
 * Reimplements 0x4c0280: zZbdManager::RegisterSectionHandler
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: add a unique section handler node to the manager's handler list.
 */
void zZbdManager::RegisterSectionHandler(
    const char *sectionName,
    zZbdSectionCallback onPreLoad,
    zZbdSectionCallback onDataReady,
    int sortOrder,
    void *userData
) {
    zZbdSectionHandlerNode *sentinel = sectionHandlerListSentinel;
    zZbdSectionHandlerNode *node = sentinel->next;

    while (node != sentinel) {
        if (strcmp(
            node->sectionHandler.sectionName,
            sectionName
        ) == 0) {
            return;
        }
        node = node->next;
    }

    zZbdSectionHandlerNode *previous = sentinel->prev;
    zZbdSectionHandlerNode *newNode = new zZbdSectionHandlerNode;

    newNode->next = sentinel != 0 ? sentinel : newNode;
    newNode->prev = previous != 0 ? previous : newNode;
    sentinel->prev = newNode;
    newNode->prev->next = newNode;

    newNode->sectionHandler.sectionName = sectionName;
    newNode->sectionHandler.onPreLoad = onPreLoad;
    newNode->sectionHandler.onDataReady = onDataReady;
    newNode->sectionHandler.sortOrder = sortOrder;
    newNode->sectionHandler.userData = userData;
    ++sectionHandlerCount;
}

/**
 * Reimplements 0x4c0370: zZbdManager::LoadEntries
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: create a write archive and invoke registered pre-load handlers.
 */
int zZbdManager::LoadEntries(
    const char *filename
) {
    int result = 0;
    if (indexArchive.OpenCreateWrite(filename) != 0) {
        result = 1;
        SortSectionHandlers();

        zZbdSectionHandlerNode *const sentinel = sectionHandlerListSentinel;
        zZbdSectionHandlerNode *node = sentinel->next;
        while (node != sentinel && result != 0) {
            zZbdSectionCallbackCtx callbackCtx = {this, &node->sectionHandler};
            result = node->sectionHandler.InvokePreLoad(&callbackCtx);
            node = node->next;
        }

        indexArchive.CloseAndFreeRecords();
    }

    return result;
}

/**
 * Reimplements 0x4c0400: zZbdManager::LoadZarFile
 * (D:\Proj\GameZRecoil\zZbd\zzbd.c).
 * Purpose: load a ZAR archive and dispatch matching section records.
 */
int zZbdManager::LoadZarFile(
    const char *filepath
) {
    if (indexArchive.Init(filepath) == 0) {
        return 0;
    }

    stopRequested = 0;
    for (unsigned int i = 0; i < indexArchive.recordCount; ++i) {
        const char *recordName = indexArchive.records[i].name;
        char recordPath[0x50] = {0};
        char sectionName[0x50] = {0};
        char sectionToken[0x50] = {0};

        strncpy(
            recordPath,
            recordName,
            sizeof(recordPath)
        );
        strncpy(
            sectionName,
            strtok(
                recordPath,
                "/"
            ),
            sizeof(sectionName)
        );
        strncpy(
            sectionToken,
            strtok(
                0,
                " "
            ),
            sizeof(sectionToken)
        );
        strncpy(
            recordPath,
            recordName,
            sizeof(recordPath)
        );

        zZbdSectionHandlerNode *const sentinel = sectionHandlerListSentinel;
        zZbdSectionHandlerNode *node = sentinel->next;
        while (node != sentinel && strcmp(
            sectionName,
            node->sectionHandler.sectionName
        ) != 0) {
            node = node->next;
        }

        if (node != sentinel) {
            zZbdSectionCallbackCtx callbackCtx = {this, &node->sectionHandler};
            unsigned int bufferSize = 0;
            indexArchive.ReadFileByName(
                recordPath,
                0,
                &bufferSize
            );
            if (bufferSize > tempBufferSize) {
                if (tempBuffer != 0) {
                    ::operator delete(tempBuffer);
                }
                tempBuffer = ::operator new(bufferSize);
                tempBufferSize = bufferSize;
            }

            indexArchive.ReadFileByName(
                recordPath,
                tempBuffer,
                &bufferSize
            );
            node->sectionHandler
                .InvokeDataReady(
                    &callbackCtx,
                    sectionToken,
                    tempBuffer,
                    bufferSize
                );

            if (stopRequested != 0) {
                break;
            }
        }
    }

    indexArchive.CloseAndFreeRecords();
    return 1;
}

/**
 * Reimplements 0x4c0620: zZbdManager::RequestStop
 * (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp).
 * Purpose: set the cooperative stop flag for archive loading.
 */
void zZbdManager::RequestStop() {
    stopRequested = 1;
}

/**
 * Reimplements 0x4c0630: zZbdManager::WriteSectionRecord
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: format a section/token record path and append the payload.
 */
int zZbdManager::WriteSectionRecord(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken,
    const void *data,
    unsigned int dataSize
) {
    char recordPath[0x50] = {0};
    sprintf(
        recordPath,
        g_zUtil_ZbdSectionRecordFmt,
        callbackCtx->sectionHandler->sectionName,
        sectionToken
    );
    return indexArchive.AddFileRecord(
        recordPath,
        data,
        dataSize,
        0,
        0
    );
}

/**
 * Reimplements 0x4c0700: zZbdManager::FlushTempStreamToSectionRecord
 * (D:\Proj\GameZRecoil\zUtil\zzbd.c).
 * Purpose: copy a temp stream into a section record and remove temp files.
 */
void zZbdManager::FlushTempStreamToSectionRecord(
    FILE *tempStream,
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken
) {
    fflush(tempStream);
    fseek(
        tempStream,
        0,
        SEEK_END
    );
    const int dataSize = ftell(tempStream);
    rewind(tempStream);

    void *sectionData = malloc(dataSize);
    if (sectionData != 0) {
        fread(
            sectionData,
            dataSize,
            1,
            tempStream
        );
        WriteSectionRecord(
            callbackCtx,
            sectionToken,
            sectionData,
            dataSize
        );
        free(sectionData);
    }

    _rmtmp();
}

/**
 * Reimplements 0x4c0780: zZbdManager::CreateTempReadStreamFromBuffer
 * (D:\Proj\GameZRecoil\zUtil\zzbd.c).
 * Purpose: create a rewound temp stream containing the supplied buffer.
 */
FILE * zZbdManager::CreateTempReadStreamFromBuffer(
    void *buffer,
    unsigned int size
) {
    FILE *const tempStream = tmpfile();
    fwrite(
        buffer,
        size,
        1,
        tempStream
    );
    fflush(tempStream);
    rewind(tempStream);
    return tempStream;
}

/**
 * Reimplements 0x4c07c0: zZbdManager::RemoveTempFiles
 * (D:\Proj\GameZRecoil\zUtil\zzbd.c).
 * Purpose: remove CRT temp files associated with ZBD streaming.
 */
void zZbdManager::RemoveTempFiles(
    FILE *tempStream
) {
    (void)tempStream;

    _rmtmp();
}

/**
 * Reimplements 0x4c06a0: zZbdSectionHandler::InvokePreLoad
 * (D:\Proj\GameZRecoil\zZbd\zzbd.c).
 * Purpose: invoke an optional pre-load section callback with user data.
 */
int zZbdSectionHandler::InvokePreLoad(
    zZbdSectionCallbackCtx *callbackCtx
) {
    if (onPreLoad == 0) {
        return 1;
    }

    typedef int(__fastcall * PreLoadCallback)(
        zZbdSectionCallbackCtx *,
        void *
    );
    return ((PreLoadCallback)(onPreLoad))(
        callbackCtx,
        userData
    );
}

/**
 * Reimplements 0x4c06c0: zZbdSectionHandler::InvokeDataReady
 * (D:\Proj\GameZRecoil\zZbd\zzbd.c).
 * Purpose: invoke an optional section data-ready callback with payload data.
 */
void zZbdSectionHandler::InvokeDataReady(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken,
    void *buffer,
    unsigned int size
) {
    if (onDataReady != 0) {
        typedef void(__fastcall * DataReadyCallback)(
            zZbdSectionCallbackCtx *,
            const char *,
            void *,
            unsigned int,
            void *
        );
        ((DataReadyCallback)(onDataReady))(
            callbackCtx,
            sectionToken,
            buffer,
            size,
            userData
        );
    }
}

/**
 * Reimplements 0x4c07d0: zZbdManager::SortSectionHandlers
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: run the MSVC STL list-sort cascade over registered handlers.
 */
void zZbdManager::SortSectionHandlers() {
    if (sectionHandlerCount < 2) {
        return;
    }

    zZbdSectionHandlerList *const sectionHandlers =
        (zZbdSectionHandlerList *)this;
    zZbdSectionHandlerList carry;
    carry.allocatorByte = allocatorByte;
    carry.sentinel = new zZbdSectionHandlerNode;
    carry.sentinel->next = carry.sentinel;
    carry.sentinel->prev = carry.sentinel;
    carry.count = 0;

    zZbdSectionHandlerList bins[16];
    int i;
    for (i = 0; i < 16; ++i) {
        bins[i].Constructor();
    }

    int filledBins = 0;
    while (sectionHandlers->count != 0) {
        zZbdSectionHandlerNode *const first =
            sectionHandlers->sentinel->next;
        zZbdSectionHandlerNode *const last = first->next;

        carry.SpliceThreeNodes(
            carry.sentinel->next,
            sectionHandlers,
            first,
            last
        );
        ++carry.count;
        --sectionHandlers->count;

        i = 0;
        while (i < filledBins && i < 15 && bins[i].count != 0) {
            bins[i].Merge(&carry);
            bins[i].Swap(&carry);
            ++i;
        }

        if (i == 15) {
            bins[15].Merge(&carry);
            filledBins = 16;
        } else {
            bins[i].Swap(&carry);
            if (i == filledBins) {
                ++filledBins;
            }
        }
    }

    while (filledBins != 0) {
        --filledBins;
        sectionHandlers->Merge(&bins[filledBins]);
    }

    for (i = 15; i >= 0; --i) {
        ::operator delete(bins[i].sentinel);
    }
    ::operator delete(carry.sentinel);
}

namespace zUtil_ZAR {
/**
 * Reimplements 0x4bffe0: zUtil_ZAR::RegisterSectionHandler
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: forward a section handler registration to the active ZBD manager.
 */
void __fastcall RegisterSectionHandler(
    const char *sectionName,
    zZbdSectionCallback onPreLoad,
    zZbdSectionCallback onDataReady,
    int sortOrder,
    void *userData
) {
    zZbdManager *manager = g_zUtil_ZbdManager;
    if (manager != 0) {
        manager->RegisterSectionHandler(
            sectionName,
            onPreLoad,
            onDataReady,
            sortOrder,
            userData
        );
    }
}

/**
 * Reimplements 0x4c0010: zUtil_ZAR::WriteSectionBlob
 * (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp).
 * Purpose: write a ZAR section blob through the callback manager context.
 */
int __fastcall WriteSectionBlob(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken,
    const void *data,
    unsigned int dataSize
) {
    return callbackCtx->manager->WriteSectionRecord(
        callbackCtx,
        sectionToken,
        data,
        dataSize
    );
}
} // namespace zUtil_ZAR

namespace zUtil_ZBD {
/**
 * Reimplements 0x4c0080: zUtil_ZBD::OpenTempWriteStream
 * (D:\Proj\GameZRecoil\zUtil\zbd_save.c).
 * Purpose: open a temp write stream when a global ZBD manager is active.
 */
FILE *OpenTempWriteStream() {
    if (g_zUtil_ZbdManager == 0) {
        return 0;
    }

    return tmpfile();
}

/**
 * Reimplements 0x4c00c0: zUtil_ZBD::OpenTempReadStream
 * (D:\Proj\GameZRecoil\zUtil\zbd_save.c).
 * Purpose: create a temp read stream from a buffer through the active manager.
 */
FILE *__fastcall OpenTempReadStream(
    void *buffer,
    unsigned int size
) {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager == 0) {
        return 0;
    }

    return manager->CreateTempReadStreamFromBuffer(
        buffer,
        size
    );
}

/**
 * Reimplements 0x4c00a0: zUtil_ZBD::FlushTempWriteStreamToSectionRecord
 * (D:\Proj\GameZRecoil\zUtil\zbd_save.c).
 * Purpose: flush a temp write stream into a named section record.
 */
void __fastcall FlushTempWriteStreamToSectionRecord(
    FILE *tempStream,
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken
) {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager != 0) {
        manager->FlushTempStreamToSectionRecord(
            tempStream,
            callbackCtx,
            sectionToken
        );
    }
}

/**
 * Reimplements 0x4c00e0: zUtil_ZBD::CloseTempReadStream
 * (D:\Proj\GameZRecoil\zUtil\zbd_save.c).
 * Purpose: close ZBD temp read-stream state through the active manager.
 */
void __fastcall CloseTempReadStream(
    FILE *tempStream
) {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager != 0) {
        manager->RemoveTempFiles(tempStream);
    }
}
} // namespace zUtil_ZBD
