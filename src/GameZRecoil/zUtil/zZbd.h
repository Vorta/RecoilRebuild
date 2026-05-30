#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include "GameZRecoil/zReader/zReader.h"
#include "recoil/recoil_callconv.h"

typedef void * zZbdSectionCallback;
struct zZbdSectionCallbackCtx;

struct zZbdSectionHandler {
    const char *sectionName;
    zZbdSectionCallback onPreLoad;
    zZbdSectionCallback onDataReady;
    int sortOrder;
    void *userData;

    static RECOIL_NOINLINE bool RECOIL_FASTCALL
    CompareSortOrderLessThan(const zZbdSectionHandler *nodeA, const zZbdSectionHandler *nodeB);
    RECOIL_NOINLINE int RECOIL_THISCALL InvokePreLoad(zZbdSectionCallbackCtx *callbackCtx);
    RECOIL_NOINLINE void RECOIL_THISCALL InvokeDataReady(zZbdSectionCallbackCtx *callbackCtx,
                                                         const char *sectionToken, void *buffer,
                                                         unsigned int size);
};

struct zZbdSectionHandlerNode {
    zZbdSectionHandlerNode *next;
    zZbdSectionHandlerNode *prev;
    zZbdSectionHandler sectionHandler;
};

struct zZbdSectionHandlerList {
    unsigned char allocatorByte;
    unsigned char unknown_01[0x03];
    zZbdSectionHandlerNode *sentinel;
    int count;

    RECOIL_NOINLINE void RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE void RECOIL_THISCALL Front(zZbdSectionHandlerNode **outIter);
    RECOIL_NOINLINE void RECOIL_THISCALL Swap(zZbdSectionHandlerList *other);
    RECOIL_NOINLINE void RECOIL_THISCALL Merge(zZbdSectionHandlerList *source);
    RECOIL_NOINLINE void RECOIL_THISCALL
    SpliceThreeNodes(zZbdSectionHandlerNode *position, zZbdSectionHandlerList *source,
                     zZbdSectionHandlerNode *first, zZbdSectionHandlerNode *last);
};

struct zZbdManager;

struct zZbdSectionCallbackCtx {
    zZbdManager *manager;
    zZbdSectionHandler *sectionHandler;
};

struct zZbdManager {
    unsigned char allocatorByte;
    unsigned char unknown_01[0x03];
    zZbdSectionHandlerNode *sectionHandlerListSentinel;
    int sectionHandlerCount;
    zIndexArchive indexArchive;
    unsigned int tempBufferSize;
    void *tempBuffer;
    unsigned int unknown_2c;
    int stopRequested;

    RECOIL_NOINLINE void RECOIL_THISCALL RegisterSectionHandler(const char *sectionName,
                                                                zZbdSectionCallback onPreLoad,
                                                                zZbdSectionCallback onDataReady,
                                                                int sortOrder,
                                                                void *userData);
    RECOIL_NOINLINE int RECOIL_THISCALL LoadEntries(const char *filename);
    RECOIL_NOINLINE int RECOIL_THISCALL LoadZarFile(const char *filepath);
    RECOIL_NOINLINE void RECOIL_THISCALL RequestStop();
    RECOIL_NOINLINE int RECOIL_THISCALL
    WriteSectionRecord(zZbdSectionCallbackCtx *callbackCtx, const char *sectionToken,
                       const void *data, unsigned int dataSize);
    RECOIL_NOINLINE void RECOIL_THISCALL SortSectionHandlers();
    RECOIL_NOINLINE void RECOIL_THISCALL Destroy();
};

RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandler) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandlerNode) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zZbdSectionHandlerList, sentinel) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zZbdSectionHandlerList, count) == 0x08);
RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandlerList) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zZbdManager, sectionHandlerListSentinel) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zZbdManager, sectionHandlerCount) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zZbdManager, stopRequested) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zZbdManager) == 0x34);

extern "C" {
extern zZbdManager *g_zUtil_ZbdManager;
}

namespace zUtil {
RECOIL_NOINLINE int RECOIL_FASTCALL ZBD_LoadEntriesGlobal(const char *filename);
RECOIL_NOINLINE int RECOIL_FASTCALL ZAR_LoadFileGlobal(const char *filepath);
RECOIL_NOINLINE void RECOIL_CDECL ZAR_RequestStopGlobal();
RECOIL_NOINLINE int RECOIL_CDECL ZBD_Init();
RECOIL_NOINLINE void RECOIL_CDECL ZBD_DestroyGlobalManager();
} // namespace zUtil

namespace zUtil_ZAR {
RECOIL_NOINLINE void RECOIL_FASTCALL RegisterSectionHandler(const char *sectionName,
                                                            zZbdSectionCallback onPreLoad,
                                                            zZbdSectionCallback onDataReady,
                                                            int sortOrder, void *userData);
RECOIL_NOINLINE int RECOIL_FASTCALL WriteSectionBlob(zZbdSectionCallbackCtx *callbackCtx,
                                                              const char *sectionToken,
                                                              const void *data,
                                                              unsigned int dataSize);
} // namespace zUtil_ZAR
