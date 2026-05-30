#include "zZbd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
zZbdManager *g_zUtil_ZbdManager = 0;
}

namespace zUtil {
// Reimplements 0x4c0030: zUtil::ZBD_LoadEntriesGlobal
// (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ZBD_LoadEntriesGlobal(const char *filename) {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager == 0) {
        return 0;
    }

    return manager->LoadEntries(filename);
}

// Reimplements 0x4c0050: zUtil::ZAR_LoadFileGlobal
// (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL ZAR_LoadFileGlobal(const char *filepath) {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager == 0) {
        return 0;
    }

    return manager->LoadZarFile(filepath);
}

// Reimplements 0x4c0070: zUtil::ZAR_RequestStopGlobal
// (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp)
RECOIL_NOINLINE void RECOIL_CDECL ZAR_RequestStopGlobal() {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager != 0) {
        manager->RequestStop();
    }
}

// Reimplements 0x4c0100: zUtil::ZBD_Init
RECOIL_NOINLINE int RECOIL_CDECL ZBD_Init() {
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

// Reimplements 0x4c0180: zUtil::ZBD_DestroyGlobalManager
RECOIL_NOINLINE void RECOIL_CDECL ZBD_DestroyGlobalManager() {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager == 0) {
        return;
    }

    manager->Destroy();
    ::operator delete(manager);
    g_zUtil_ZbdManager = 0;
}
} // namespace zUtil

// Reimplements 0x4c0260: zZbdSectionHandler::CompareSortOrderLessThan
// (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp)
RECOIL_NOINLINE bool RECOIL_FASTCALL zZbdSectionHandler::CompareSortOrderLessThan(
    const zZbdSectionHandler *nodeA, const zZbdSectionHandler *nodeB) {
    return nodeA->sortOrder < nodeB->sortOrder;
}

// Reimplements 0x4c0b70: zZbdSectionHandlerList::Constructor
// (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support)
RECOIL_NOINLINE void RECOIL_THISCALL zZbdSectionHandlerList::Constructor() {
    allocatorByte = 0;
    sentinel = new zZbdSectionHandlerNode;
    sentinel->next = sentinel;
    sentinel->prev = sentinel;
    count = 0;
}

// Reimplements 0x4c0b60: zZbdSectionHandlerList::Front
// (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support)
RECOIL_NOINLINE void RECOIL_THISCALL
zZbdSectionHandlerList::Front(zZbdSectionHandlerNode **outIter) {
    *outIter = sentinel->next;
}

// Reimplements 0x4c0ba0: zZbdSectionHandlerList::Swap
// (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support)
RECOIL_NOINLINE void RECOIL_THISCALL
zZbdSectionHandlerList::Swap(zZbdSectionHandlerList *other) {
    zZbdSectionHandlerNode *const oldSentinel = sentinel;
    sentinel = other->sentinel;
    other->sentinel = oldSentinel;

    const int oldCount = count;
    count = other->count;
    other->count = oldCount;
}

// Reimplements 0x4c0ce0: zZbdSectionHandlerList::SpliceThreeNodes
// (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support)
RECOIL_NOINLINE void RECOIL_THISCALL
zZbdSectionHandlerList::SpliceThreeNodes(zZbdSectionHandlerNode *position,
                                         zZbdSectionHandlerList *source,
                                         zZbdSectionHandlerNode *first,
                                         zZbdSectionHandlerNode *last) {
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

// Reimplements 0x4c0bd0: zZbdSectionHandlerList::Merge
// (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp; MSVC 5.0 STL list support)
RECOIL_NOINLINE void RECOIL_THISCALL
zZbdSectionHandlerList::Merge(zZbdSectionHandlerList *source) {
    if (source == this) {
        return;
    }

    zZbdSectionHandlerNode *destNode = sentinel->next;
    zZbdSectionHandlerNode *sourceNode = source->sentinel->next;
    while (destNode != sentinel && sourceNode != source->sentinel) {
        if (zZbdSectionHandler::CompareSortOrderLessThan(&sourceNode->sectionHandler,
                                                         &destNode->sectionHandler)) {
            zZbdSectionHandlerNode *const nextSourceNode = sourceNode->next;
            SpliceThreeNodes(destNode, source, sourceNode, nextSourceNode);
            sourceNode = nextSourceNode;
        } else {
            destNode = destNode->next;
        }
    }

    if (sourceNode != source->sentinel) {
        SpliceThreeNodes(sentinel, source, sourceNode, source->sentinel);
    }

    count += source->count;
    source->count = 0;
}

// Reimplements 0x4c01b0: zZbdManager::Destroy
RECOIL_NOINLINE void RECOIL_THISCALL zZbdManager::Destroy() {
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

// Reimplements 0x4c0280: zZbdManager::RegisterSectionHandler
RECOIL_NOINLINE void RECOIL_THISCALL zZbdManager::RegisterSectionHandler(
    const char *sectionName, zZbdSectionCallback onPreLoad, zZbdSectionCallback onDataReady,
    int sortOrder, void *userData) {
    zZbdSectionHandlerNode *sentinel = sectionHandlerListSentinel;
    zZbdSectionHandlerNode *node = sentinel->next;

    while (node != sentinel) {
        if (strcmp(node->sectionHandler.sectionName, sectionName) == 0) {
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

// Reimplements 0x4c0370: zZbdManager::LoadEntries
// (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zZbdManager::LoadEntries(const char *filename) {
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

// Reimplements 0x4c0400: zZbdManager::LoadZarFile
// (D:\Proj\GameZRecoil\zZbd\zzbd.c)
RECOIL_NOINLINE int RECOIL_THISCALL zZbdManager::LoadZarFile(const char *filepath) {
    if (indexArchive.Init(filepath) == 0) {
        return 0;
    }

    stopRequested = 0;
    for (unsigned int i = 0; i < indexArchive.recordCount; ++i) {
        const char *recordName = indexArchive.records[i].name;
        char recordPath[0x50] = {0};
        char sectionName[0x50] = {0};
        char sectionToken[0x50] = {0};

        strncpy(recordPath, recordName, sizeof(recordPath));
        strncpy(sectionName, strtok(recordPath, "/"), sizeof(sectionName));
        strncpy(sectionToken, strtok(0, " "), sizeof(sectionToken));
        strncpy(recordPath, recordName, sizeof(recordPath));

        zZbdSectionHandlerNode *const sentinel = sectionHandlerListSentinel;
        zZbdSectionHandlerNode *node = sentinel->next;
        while (node != sentinel &&
               strcmp(sectionName, node->sectionHandler.sectionName) != 0) {
            node = node->next;
        }

        if (node != sentinel) {
            zZbdSectionCallbackCtx callbackCtx = {this, &node->sectionHandler};
            unsigned int bufferSize = 0;
            indexArchive.ReadFileByName(recordPath, 0, &bufferSize);
            if (bufferSize > tempBufferSize) {
                if (tempBuffer != 0) {
                    ::operator delete(tempBuffer);
                }
                tempBuffer = ::operator new(bufferSize);
                tempBufferSize = bufferSize;
            }

            indexArchive.ReadFileByName(recordPath, tempBuffer, &bufferSize);
            node->sectionHandler.InvokeDataReady(&callbackCtx, sectionToken, tempBuffer,
                                                 bufferSize);

            if (stopRequested != 0) {
                break;
            }
        }
    }

    indexArchive.CloseAndFreeRecords();
    return 1;
}

// Reimplements 0x4c0620: zZbdManager::RequestStop
// (D:\Proj\GameZRecoil\zUtil\zutl_zar.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zZbdManager::RequestStop() {
    stopRequested = 1;
}

// Reimplements 0x4c0630: zZbdManager::WriteSectionRecord
RECOIL_NOINLINE int RECOIL_THISCALL
zZbdManager::WriteSectionRecord(zZbdSectionCallbackCtx *callbackCtx, const char *sectionToken,
                                const void *data, unsigned int dataSize) {
    char recordPath[0x50] = {0};
    sprintf(recordPath, "%s/%s", callbackCtx->sectionHandler->sectionName, sectionToken);
    return indexArchive.AddFileRecord(recordPath, data, dataSize, 0, 0);
}

// Reimplements 0x4c06a0: zZbdSectionHandler::InvokePreLoad
// (D:\Proj\GameZRecoil\zZbd\zzbd.c)
RECOIL_NOINLINE int RECOIL_THISCALL
zZbdSectionHandler::InvokePreLoad(zZbdSectionCallbackCtx *callbackCtx) {
    if (onPreLoad == 0) {
        return 1;
    }

    typedef int (RECOIL_FASTCALL *PreLoadCallback)(zZbdSectionCallbackCtx *, void *);
    return ((PreLoadCallback)(onPreLoad))(callbackCtx, userData);
}

// Reimplements 0x4c06c0: zZbdSectionHandler::InvokeDataReady
// (D:\Proj\GameZRecoil\zZbd\zzbd.c)
RECOIL_NOINLINE void RECOIL_THISCALL
zZbdSectionHandler::InvokeDataReady(zZbdSectionCallbackCtx *callbackCtx, const char *sectionToken,
                                    void *buffer, unsigned int size) {
    if (onDataReady != 0) {
        typedef void (RECOIL_FASTCALL *DataReadyCallback)(zZbdSectionCallbackCtx *, const char *,
                                                          void *, unsigned int, void *);
        ((DataReadyCallback)(onDataReady))(callbackCtx, sectionToken, buffer, size,
                                                         userData);
    }
}

// Reimplements 0x4c07d0: zZbdManager::SortSectionHandlers
// (D:\Proj\GameZRecoil\zUtil\zUtil_ZBD.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zZbdManager::SortSectionHandlers() {
    if (sectionHandlerCount < 2) {
        return;
    }

    zZbdSectionHandlerNode *const sentinel = sectionHandlerListSentinel;
    zZbdSectionHandlerNode sorted;
    sorted.next = &sorted;
    sorted.prev = &sorted;

    zZbdSectionHandlerNode *node = sentinel->next;
    while (node != sentinel) {
        zZbdSectionHandlerNode *const next = node->next;
        node->prev->next = node->next;
        node->next->prev = node->prev;

        zZbdSectionHandlerNode *insertBefore = sorted.next;
        while (insertBefore != &sorted &&
               !zZbdSectionHandler::CompareSortOrderLessThan(&node->sectionHandler,
                                                             &insertBefore->sectionHandler)) {
            insertBefore = insertBefore->next;
        }

        zZbdSectionHandlerNode *const insertAfter = insertBefore->prev;
        node->next = insertBefore;
        node->prev = insertAfter;
        insertAfter->next = node;
        insertBefore->prev = node;
        node = next;
    }

    sentinel->next = sorted.next;
    sentinel->prev = sorted.prev;
    sorted.next->prev = sentinel;
    sorted.prev->next = sentinel;
}

namespace zUtil_ZAR {
// Reimplements 0x4bffe0: zUtil_ZAR::RegisterSectionHandler
RECOIL_NOINLINE void RECOIL_FASTCALL RegisterSectionHandler(const char *sectionName,
                                                            zZbdSectionCallback onPreLoad,
                                                            zZbdSectionCallback onDataReady,
                                                            int sortOrder,
                                                            void *userData) {
    zZbdManager *manager = g_zUtil_ZbdManager;
    if (manager != 0) {
        manager->RegisterSectionHandler(sectionName, onPreLoad, onDataReady, sortOrder, userData);
    }
}

// Reimplements 0x4c0010: zUtil_ZAR::WriteSectionBlob
RECOIL_NOINLINE int RECOIL_FASTCALL WriteSectionBlob(zZbdSectionCallbackCtx *callbackCtx,
                                                              const char *sectionToken,
                                                              const void *data,
                                                              unsigned int dataSize) {
    return callbackCtx->manager->WriteSectionRecord(callbackCtx, sectionToken, data, dataSize);
}
} // namespace zUtil_ZAR
