#include "zbd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-g-zutil-zbdmanager
 * @recoil-artifact defines .data recoil:data:0x56bf70: g_zUtil_ZbdManager.
 * BN types this zero-initialized 4-byte .data slot as the process-wide
 * zZbdManager pointer; ZBD_Init/ZBD_DestroyGlobalManager own its lifecycle
 * and the ZAR/ZBD wrapper helpers null-check it before forwarding work.
 * Purpose: store the active ZBD archive manager singleton.
 */
zZbdManager *g_zUtil_ZbdManager = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-x27
 * @recoil-artifact defines .data recoil:data:0x4e3010: g_zUtil_SourceFile_ZutlZarCpp.
 * BN stores this initialized .data literal immediately before the
 * GetLastError format used by the ZAR archive-open diagnostic path.
 * Purpose: name the original zutl_zar.cpp source file in error reports.
 */
char g_zUtil_SourceFile_ZutlZarCpp[0x27] =
    "D:\\Proj\\GameZRecoil\\zUtil\\zutl_zar.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-x19
 * @recoil-artifact defines .data recoil:data:0x4e3038: g_zUtil_GetLastErrorFmt.
 * BN xrefs this adjacent literal from zIndexArchive::Init's CreateFileA
 * failure path while reporting ZAR load errors.
 * Purpose: format Win32 GetLastError diagnostics for failed archive opens.
 */
char g_zUtil_GetLastErrorFmt[0x19] = "GetLastError(0x%08x): %s";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-x2
 * @recoil-artifact defines .data recoil:data:0x4e48e4: k_zar_StrTokSlash.
 * BN stores this writable two-byte .data delimiter immediately before
 * g_zUtil_ZbdSectionRecordFmt and xrefs it from zZbdManager::LoadZarFile's
 * first strtok call.
 * Purpose: split ZAR record paths at the section/token slash delimiter.
 */
char k_zar_StrTokSlash[0x2] = "/";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-x6
 * @recoil-artifact defines .data recoil:data:0x4e48e8: g_zUtil_ZbdSectionRecordFmt.
 * BN stores this writable .data format literal and xrefs it only from
 * zZbdManager::WriteSectionRecord's section/token path formatting.
 * Purpose: format ZBD section record paths as "section/token".
 */
char g_zUtil_ZbdSectionRecordFmt[0x6] = "%s/%s";
}

namespace zUtil_ZAR {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-registersectionhandler
 * @recoil-artifact defines .text recoil:function:0x4bffe0: zUtil_ZAR::RegisterSectionHandler
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-writesectionblob
 * @recoil-artifact defines .text recoil:function:0x4c0010: zUtil_ZAR::WriteSectionBlob
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

namespace zUtil {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zbd-loadentriesglobal
 * @recoil-artifact defines .text recoil:function:0x4c0030: zUtil::ZBD_LoadEntriesGlobal
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zar-loadfileglobal
 * @recoil-artifact defines .text recoil:function:0x4c0050: zUtil::ZAR_LoadFileGlobal
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zar-requeststopglobal
 * @recoil-artifact defines .text recoil:function:0x4c0070: zUtil::ZAR_RequestStopGlobal
 * Purpose: request cooperative ZAR loading stop through the active manager.
 */
void __cdecl ZAR_RequestStopGlobal() {
    zZbdManager *const manager = g_zUtil_ZbdManager;
    if (manager != 0) {
        manager->RequestStop();
    }
}
} // namespace zUtil

namespace zUtil_ZBD {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-opentempwritestream
 * @recoil-artifact defines .text recoil:function:0x4c0080: zUtil_ZBD::OpenTempWriteStream
 * Purpose: open a temp write stream when a global ZBD manager is active.
 */
FILE *__cdecl OpenTempWriteStream() {
    FILE *tempStream = 0;
    if (g_zUtil_ZbdManager != 0) {
        tempStream = tmpfile();
    }

    return tempStream;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-flushtempwritestreamtosectionrecord
 * @recoil-artifact defines .text recoil:function:0x4c00a0: zUtil_ZBD::FlushTempWriteStreamToSectionRecord
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-opentempreadstream
 * @recoil-artifact defines .text recoil:function:0x4c00c0: zUtil_ZBD::OpenTempReadStream
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-closetempreadstream
 * @recoil-artifact defines .text recoil:function:0x4c00e0: zUtil_ZBD::CloseTempReadStream
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

namespace zUtil {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zbd-init
 * @recoil-artifact defines .text recoil:function:0x4c0100: zUtil::ZBD_Init
 * Purpose: allocate and initialize the global ZBD manager and handler sentinel.
 */
int __cdecl ZBD_Init() {
    zZbdManager *manager = new zZbdManager;
    if (manager != 0) {
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zbd-destroyglobalmanager
 * @recoil-artifact defines .text recoil:function:0x4c0180: zUtil::ZBD_DestroyGlobalManager
 * Purpose: destroy and clear the active global ZBD manager.
 */
void __cdecl ZBD_DestroyGlobalManager() {
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-destroy
 * @recoil-artifact defines .text recoil:function:0x4c01b0: zZbdManager::Destroy
 * Purpose: release manager buffers, archive records, handler nodes, and sentinel.
 */
void zZbdManager::Destroy() {
    if (tempBuffer != 0) {
        ::operator delete(tempBuffer);
        tempBuffer = 0;
    }

    indexArchive.Destroy();
    sectionHandlers.~zZbdSectionHandlerList();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdsectionhandler-comparesortorderlessthan
 * @recoil-artifact defines .text recoil:function:0x4c0260: zZbdSectionHandler::CompareSortOrderLessThan
 * Purpose: compare section handlers by ascending sort order.
 */
bool __fastcall zZbdSectionHandler::CompareSortOrderLessThan(
    const zZbdSectionHandler *nodeA,
    const zZbdSectionHandler *nodeB
) {
    return nodeA->sortOrder < nodeB->sortOrder;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-registersectionhandler
 * @recoil-artifact defines .text recoil:function:0x4c0280: zZbdManager::RegisterSectionHandler
 * Purpose: add a unique section handler node to the manager's handler list.
 */
void zZbdManager::RegisterSectionHandler(
    const char *sectionName,
    zZbdSectionCallback onPreLoad,
    zZbdSectionCallback onDataReady,
    int sortOrder,
    void *userData
) {
    zZbdSectionHandlerList::iterator node = sectionHandlers.begin();
    while (node != sectionHandlers.end()) {
        if (strcmp(
            node->sectionName,
            sectionName
        ) == 0) {
            return;
        }
        ++node;
    }

    zZbdSectionHandler sectionHandler;
    sectionHandler.sectionName = sectionName;
    sectionHandler.onPreLoad = onPreLoad;
    sectionHandler.onDataReady = onDataReady;
    sectionHandler.sortOrder = sortOrder;
    sectionHandler.userData = userData;
    sectionHandlers.push_back(sectionHandler);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-loadentries
 * @recoil-artifact defines .text recoil:function:0x4c0370: zZbdManager::LoadEntries
 * Purpose: create a write archive and invoke registered pre-load handlers.
 */
int zZbdManager::LoadEntries(
    const char *filename
) {
    int result = 0;
    if (indexArchive.OpenCreateWrite(filename) != 0) {
        result = 1;
        sectionHandlers.sort();

        zZbdSectionHandlerList::iterator node = sectionHandlers.begin();
        while (node != sectionHandlers.end() && result != 0) {
            zZbdSectionCallbackCtx callbackCtx = {this, &*node};
            result = node->InvokePreLoad(&callbackCtx);
            ++node;
        }

        indexArchive.CloseAndFreeRecords();
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-loadzarfile
 * @recoil-artifact defines .text recoil:function:0x4c0400: zZbdManager::LoadZarFile
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
                k_zar_StrTokSlash
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

        zZbdSectionHandlerList::iterator node = sectionHandlers.begin();
        while (node != sectionHandlers.end() && strcmp(
            sectionName,
            node->sectionName
        ) != 0) {
            ++node;
        }

        if (node != sectionHandlers.end()) {
            zZbdSectionCallbackCtx callbackCtx = {this, &*node};
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
            node->InvokeDataReady(
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-requeststop
 * @recoil-artifact defines .text recoil:function:0x4c0620: zZbdManager::RequestStop
 * Purpose: set the cooperative stop flag for archive loading.
 */
void zZbdManager::RequestStop() {
    stopRequested = 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-writesectionrecord
 * @recoil-artifact defines .text recoil:function:0x4c0630: zZbdManager::WriteSectionRecord
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdsectionhandler-invokepreload
 * @recoil-artifact defines .text recoil:function:0x4c06a0: zZbdSectionHandler::InvokePreLoad
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdsectionhandler-invokedataready
 * @recoil-artifact defines .text recoil:function:0x4c06c0: zZbdSectionHandler::InvokeDataReady
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-flushtempstreamtosectionrecord
 * @recoil-artifact defines .text recoil:function:0x4c0700: zZbdManager::FlushTempStreamToSectionRecord
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-createtempreadstreamfrombuffer
 * @recoil-artifact defines .text recoil:function:0x4c0780: zZbdManager::CreateTempReadStreamFromBuffer
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zutil-zutl-zbd-zzbdmanager-removetempfiles
 * @recoil-artifact defines .text recoil:function:0x4c07c0: zZbdManager::RemoveTempFiles
 * Purpose: remove CRT temp files associated with ZBD streaming.
 */
void zZbdManager::RemoveTempFiles(
    FILE *tempStream
) {
    (void)tempStream;

    _rmtmp();
}
