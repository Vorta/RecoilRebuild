#pragma once
#ifdef __cplusplus
#include "recoil/recoil_types.h"
#include <list>
#include <stddef.h>

#include "GameZRecoil/zReader/zreader.h"
#include "recoil/recoil_callconv.h"

typedef void* zZbdSectionCallback;
struct zZbdSectionCallbackCtx;

struct zZbdSectionHandler {
    const char* sectionName;
    zZbdSectionCallback onWrite;
    zZbdSectionCallback onDataReady;
    int sortOrder;
    void* userData;

    static bool __fastcall CompareSortOrderLessThan(const zZbdSectionHandler* nodeA, const zZbdSectionHandler* nodeB);
    int InvokeWriteCallback(zZbdSectionCallbackCtx* callbackCtx);
    void
    InvokeDataReady(zZbdSectionCallbackCtx* callbackCtx, const char* sectionToken, void* buffer, unsigned int size);

    bool operator<(const zZbdSectionHandler& other) const
    {
        return CompareSortOrderLessThan(this, &other);
    }
};

typedef std::list<zZbdSectionHandler> zZbdSectionHandlerList;

struct zZbdManager;

struct zZbdSectionCallbackCtx {
    zZbdManager* manager;
    zZbdSectionHandler* sectionHandler;
};

struct zZbdManager {
    zZbdSectionHandlerList sectionHandlers;
    zIndexArchive indexArchive;
    unsigned int tempBufferSize;
    void* tempBuffer;
    unsigned int unknown_2c;
    int stopRequested;

    zZbdManager()
    {
        tempBufferSize = 0;
        tempBuffer = 0;
    }
    ~zZbdManager();
    void RegisterSectionHandler(
        const char* sectionName,
        zZbdSectionCallback onWrite,
        zZbdSectionCallback onDataReady,
        int sortOrder,
        void* userData
    );
    int WriteZarFile(const char* filename);
    int LoadZarFile(const char* filepath);
    void RequestStop();
    int WriteSectionRecord(
        zZbdSectionCallbackCtx* callbackCtx,
        const char* sectionToken,
        const void* data,
        unsigned int dataSize
    );
    void
    FlushTempStreamToSectionRecord(FILE* tempStream, zZbdSectionCallbackCtx* callbackCtx, const char* sectionToken);
    FILE* CreateTempReadStreamFromBuffer(void* buffer, unsigned int size);
    void RemoveTempFiles(FILE* tempStream);
};

RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandler) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zZbdSectionHandlerList) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zZbdManager, indexArchive) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zZbdManager, stopRequested) == 0x30);
RECOIL_STATIC_ASSERT(sizeof(zZbdManager) == 0x34);

extern "C" {
extern zZbdManager* g_zUtil_ZbdManager;
}

namespace zUtil {
int __fastcall zZarWriteFileGlobal(const char* filename);
int __fastcall zZarLoadFileGlobal(const char* filepath);
void __cdecl zZarRequestStopGlobal();
int __cdecl ZBDInit();
void __cdecl ZBDDestroyGlobalManager();
} // namespace zUtil

namespace zUtil_ZAR {
extern "C" void __fastcall RegisterSectionHandler(
    const char* sectionName,
    zZbdSectionCallback onWrite,
    zZbdSectionCallback onDataReady,
    int sortOrder,
    void* userData
);
extern "C" int __fastcall WriteSectionBlob(
    zZbdSectionCallbackCtx* callbackCtx,
    const char* sectionToken,
    const void* data,
    unsigned int dataSize
);
} // namespace zUtil_ZAR

namespace zUtil_ZBD {
FILE* __cdecl OpenTempWriteStream();
FILE* __fastcall OpenTempReadStream(void* buffer, unsigned int size);
void __fastcall
FlushTempWriteStreamToSectionRecord(FILE* tempStream, zZbdSectionCallbackCtx* callbackCtx, const char* sectionToken);
void __fastcall CloseTempReadStream(FILE* tempStream);
} // namespace zUtil_ZBD
#else
/* C view of the zUtil ZBD entry points the zClass units call. */
#include "recoil/recoil_callconv.h"

typedef void* zZbdSectionCallback;

void __fastcall RegisterSectionHandler(
    const char* sectionName,
    zZbdSectionCallback onWrite,
    zZbdSectionCallback onDataReady,
    int sortOrder,
    void* userData
);
int __fastcall WriteSectionBlob(
    struct zZbdSectionCallbackCtx* callbackCtx,
    const char* sectionToken,
    const void* data,
    unsigned int dataSize
);
#endif
