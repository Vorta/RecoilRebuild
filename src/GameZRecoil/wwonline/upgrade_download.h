#pragma once

#include <stddef.h>

#include "Battlesport/WestwoodOnlineUpgradeRefCountAndLock.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <unknwn.h>
#include <windows.h>

struct WestwoodOnlineUpgradeDownload {
    RECOIL_NOINLINE static HRESULT RECOIL_CDECL CreateInstanceAndAdvise();
    RECOIL_NOINLINE static ULONG RECOIL_CDECL UnadviseAndRelease();
};

struct WestwoodOnlineUpgradeDownloadComVtable {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(
        IUnknown *self,
        REFIID iid,
        void **out
    );
    ULONG(STDMETHODCALLTYPE *AddRef)(IUnknown *self);
    ULONG(STDMETHODCALLTYPE *Release)(IUnknown *self);
    HRESULT(STDMETHODCALLTYPE *BeginDownload)(
        IUnknown *self,
        const char *descriptor0,
        const char *descriptor1,
        const char *descriptor2,
        const char *sourcePath,
        const char *fileName,
        const char *registryKey
    );
    HRESULT(STDMETHODCALLTYPE *Abort)(IUnknown *self);
    HRESULT(STDMETHODCALLTYPE *Pump)(IUnknown *self);
};

struct WestwoodOnlineUpgradeDownloadComObject {
    WestwoodOnlineUpgradeDownloadComVtable *vftable;
};

struct WestwoodOnlineUpgradeDownloadReadyEntry {
    unsigned char reserved000[0x0c];
    WestwoodOnlineUpgradeDownloadReadyEntry *m_next;
    char m_descriptor0[0x41];
    char m_sourcePathBase[0x100];
    char m_fileName[0x21];
    char m_descriptor1[0x21];
    char m_descriptor2[0x41];
    char m_downloadDirectory[0x100];
};

struct WestwoodOnlineUpgradeDownloadEventSinkVtable {
    void *slots[32];
};

enum WestwoodOnlineUpgradeDownloadState {
    WOL_DOWNLOAD_STATE_CONNECTING = 2,
    WOL_DOWNLOAD_STATE_FINDING_PATCH = 4,
    WOL_DOWNLOAD_STATE_DOWNLOADING_PATCH = 6
};

struct WestwoodOnlineUpgradeDownloadEventSink {
    WestwoodOnlineUpgradeDownloadEventSinkVtable *m_vftable;
    WestwoodOnlineUpgradeRefCountAndLock m_refCountAndLock;

    RECOIL_NOINLINE int RECOIL_THISCALL CallbackNoOp(void *arg);
    RECOIL_NOINLINE static HRESULT STDMETHODCALLTYPE OnDownloadFinished(IUnknown *self);
    RECOIL_NOINLINE static HRESULT STDMETHODCALLTYPE OnDownloadError(
        IUnknown *self,
        HRESULT result
    );
    RECOIL_NOINLINE static HRESULT STDMETHODCALLTYPE OnDownloadProgress(
        IUnknown *self,
        unsigned int bytesRead,
        unsigned int totalBytes,
        int unusedArg4,
        int secondsLeft
    );
    RECOIL_NOINLINE static HRESULT STDMETHODCALLTYPE OnStateChanged(
        IUnknown *self,
        WestwoodOnlineUpgradeDownloadState stateCode
    );
    RECOIL_NOINLINE static ULONG STDMETHODCALLTYPE AddRef(
        WestwoodOnlineUpgradeDownloadEventSink *self
    );
    RECOIL_NOINLINE static ULONG STDMETHODCALLTYPE Release(
        WestwoodOnlineUpgradeDownloadEventSink *self
    );
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE static HRESULT RECOIL_STDCALL QueryInterface(
        WestwoodOnlineUpgradeDownloadEventSink *self,
        REFIID iid,
        void **outInterface
    );
    RECOIL_NOINLINE static HRESULT RECOIL_STDCALL CreateInstance(
        WestwoodOnlineUpgradeDownloadEventSink **outSink
    );
};

extern WestwoodOnlineUpgradeDownloadEventSinkVtable g_WestwoodOnlineUpgradeDownloadEventSink_Vtbl;
extern "C" IUnknown *g_pWestwoodOnlineUpgradeDownload;
extern "C" void *g_pWestwoodOnlineUpgradeDownloadEventSink;
extern "C" DWORD g_WestwoodOnlineUpgradeDownloadAdviseCookie;
extern "C" int g_WestwoodOnlineUpgradeDownloadEventSinkConnectionOffset;
extern "C" LONG g_WestwoodOnlineUpgradeEventSinkLiveCount;
extern "C" WestwoodOnlineUpgradeDownloadReadyEntry *g_pWestwoodOnlineUpgradeDownloadReadyList;
extern "C" char g_WestwoodOnlineUpgradeDownloadReadyPromptText[0x80];
extern "C" char g_WestwoodOnlineUpgradeDownloadRestoreCwd[0x100];
extern "C" int g_WestwoodOnlineUpgradeDownloadDialogResult;
extern "C" HWND g_hWestwoodOnlineUpgradeProgressDialog;
extern const CLSID g_WestwoodOnlineUpgradeDownload_CLSID;
extern const IID g_WestwoodOnlineUpgradeDownload_IID;
extern const IID g_WestwoodOnlineUpgradeDownloadEventSink_IID;

RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadComVtable,
        BeginDownload
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadComVtable,
        Abort
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadComVtable,
        Pump
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadReadyEntry,
        m_next
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadReadyEntry,
        m_descriptor0
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadReadyEntry,
        m_sourcePathBase
    ) == 0x51
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadReadyEntry,
        m_fileName
    ) == 0x151
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadReadyEntry,
        m_descriptor1
    ) == 0x172
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadReadyEntry,
        m_descriptor2
    ) == 0x193
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadReadyEntry,
        m_downloadDirectory
    ) == 0x1d4
);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeDownloadEventSinkVtable) == 0x80);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeDownloadEventSink) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadEventSink,
        m_vftable
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadEventSink,
        m_refCountAndLock
    ) == 0x04
);
