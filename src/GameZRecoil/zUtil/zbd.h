#pragma once

#include "recoil/recoil_types.h"
#include <list>
#include <stddef.h>

#include "GameZRecoil/zReader/zreader.h"
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

    bool operator<(const zZbdSectionHandler &other) const {
        return CompareSortOrderLessThan(this, &other);
    }
};

typedef std::list<zZbdSectionHandler> zZbdSectionHandlerList;

struct zZbdManager;

struct zZbdSectionCallbackCtx {
    zZbdManager *manager;
    zZbdSectionHandler *sectionHandler;
};

struct zZbdManager {
    zZbdSectionHandlerList sectionHandlers;
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
    void Destroy();
};

RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandler) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandlerList) == 0x0c);
RECOIL_STATIC_ASSERT(
    offsetof(
        zZbdManager,
        indexArchive
    ) == 0x0c
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
void __cdecl ZAR_RequestStopGlobal();
int __cdecl ZBD_Init();
void __cdecl ZBD_DestroyGlobalManager();
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
FILE *__cdecl OpenTempWriteStream();
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
