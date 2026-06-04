#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "GameZRecoil/zReader/zReader.h"
#include "recoil/recoil_callconv.h"

typedef void *zZbdSectionCallback;
struct zZbdSectionCallbackCtx;

struct zZbdSectionHandler {
    const char *sectionName;
    zZbdSectionCallback onPreLoad;
    zZbdSectionCallback onDataReady;
    int sortOrder;
    void *userData;

    static bool __fastcall CompareSortOrderLessThan(
        const zZbdSectionHandler *nodeA,
        const zZbdSectionHandler *nodeB
    );
    int InvokePreLoad(zZbdSectionCallbackCtx *callbackCtx);
    void InvokeDataReady(
        zZbdSectionCallbackCtx *callbackCtx,
        const char *sectionToken,
        void *buffer,
        unsigned int size
    );
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

    void Constructor();
    void Front(zZbdSectionHandlerNode **outIter);
    void Swap(zZbdSectionHandlerList *other);
    void Merge(zZbdSectionHandlerList *source);
    void SpliceThreeNodes(
        zZbdSectionHandlerNode *position,
        zZbdSectionHandlerList *source,
        zZbdSectionHandlerNode *first,
        zZbdSectionHandlerNode *last
    );
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

    void RegisterSectionHandler(
        const char *sectionName,
        zZbdSectionCallback onPreLoad,
        zZbdSectionCallback onDataReady,
        int sortOrder,
        void *userData
    );
    int LoadEntries(const char *filename);
    int LoadZarFile(const char *filepath);
    void RequestStop();
    int WriteSectionRecord(
        zZbdSectionCallbackCtx *callbackCtx,
        const char *sectionToken,
        const void *data,
        unsigned int dataSize
    );
    void FlushTempStreamToSectionRecord(
        FILE *tempStream,
        zZbdSectionCallbackCtx *callbackCtx,
        const char *sectionToken
    );
    FILE * CreateTempReadStreamFromBuffer(
        void *buffer,
        unsigned int size
    );
    void RemoveTempFiles(FILE *tempStream);
    void SortSectionHandlers();
    void Destroy();
};

RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandler) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandlerNode) == 0x1c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zZbdSectionHandlerList,
        sentinel
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zZbdSectionHandlerList,
        count
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandlerList) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zZbdManager,
        sectionHandlerListSentinel
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zZbdManager,
        sectionHandlerCount
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zZbdManager,
        stopRequested
    ) == 0x30
);
RECOIL_STATIC_ASSERT(sizeof(zZbdManager) == 0x34);

extern "C" {
extern zZbdManager *g_zUtil_ZbdManager;
}

namespace zUtil {
int __fastcall ZBD_LoadEntriesGlobal(const char *filename);
int __fastcall ZAR_LoadFileGlobal(const char *filepath);
void ZAR_RequestStopGlobal();
int ZBD_Init();
void ZBD_DestroyGlobalManager();
} // namespace zUtil

namespace zUtil_ZAR {
void __fastcall RegisterSectionHandler(
    const char *sectionName,
    zZbdSectionCallback onPreLoad,
    zZbdSectionCallback onDataReady,
    int sortOrder,
    void *userData
);
int __fastcall WriteSectionBlob(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken,
    const void *data,
    unsigned int dataSize
);
} // namespace zUtil_ZAR

namespace zUtil_ZBD {
FILE *OpenTempWriteStream();
FILE *__fastcall OpenTempReadStream(
    void *buffer,
    unsigned int size
);
void __fastcall FlushTempWriteStreamToSectionRecord(
    FILE *tempStream,
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken
);
void __fastcall CloseTempReadStream(FILE *tempStream);
} // namespace zUtil_ZBD
