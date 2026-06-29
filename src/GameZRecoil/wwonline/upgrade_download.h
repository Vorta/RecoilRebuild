#pragma once

#include <stddef.h>

#include "Battlesport/WestwoodOnlineUpgradeRefCountAndLock.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include <unknwn.h>
#include <windows.h>

struct WestwoodOnlineUpgradeDownload {
    static HRESULT CreateInstanceAndAdvise();
    static ULONG UnadviseAndRelease();
};

struct IWestwoodOnlineUpgradeDownload : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE BeginDownload(
        const char *descriptor0,
        const char *descriptor1,
        const char *descriptor2,
        const char *sourcePath,
        const char *fileName,
        const char *registryKey
    ) = 0;
    virtual HRESULT STDMETHODCALLTYPE Abort() = 0;
    virtual HRESULT STDMETHODCALLTYPE Pump() = 0;
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

enum WestwoodOnlineUpgradeDownloadState {
    WOL_DOWNLOAD_STATE_CONNECTING = 2,
    WOL_DOWNLOAD_STATE_FINDING_PATCH = 4,
    WOL_DOWNLOAD_STATE_DOWNLOADING_PATCH = 6
};

ULONG __stdcall WestwoodOnlineUpgradeSharedComAddRef(void *self);

struct WestwoodOnlineUpgradeDownloadEventSink : IUnknown {
    WestwoodOnlineUpgradeRefCountAndLock m_refCountAndLock;

    virtual HRESULT STDMETHODCALLTYPE OnDownloadFinished();
    virtual HRESULT STDMETHODCALLTYPE OnDownloadError(
        HRESULT result
    );
    virtual HRESULT STDMETHODCALLTYPE OnDownloadProgress(
        unsigned int bytesRead,
        unsigned int totalBytes,
        int unusedArg4,
        int secondsLeft
    );
    virtual int STDMETHODCALLTYPE CallbackNoOp(void *arg);
    virtual HRESULT STDMETHODCALLTYPE OnStateChanged(
        WestwoodOnlineUpgradeDownloadState stateCode
    );
    HRESULT STDMETHODCALLTYPE QueryInterface(
        REFIID iid,
        void **outInterface
    );
    ULONG STDMETHODCALLTYPE AddRef();
    ULONG STDMETHODCALLTYPE Release();
    ~WestwoodOnlineUpgradeDownloadEventSink();
    static HRESULT __stdcall CreateInstance(
        WestwoodOnlineUpgradeDownloadEventSink **outSink
    );
};

extern "C" IWestwoodOnlineUpgradeDownload *g_pWestwoodOnlineUpgradeDownload;
extern "C" WestwoodOnlineUpgradeDownloadEventSink *g_pWestwoodOnlineUpgradeDownloadEventSink;
extern "C" DWORD g_WestwoodOnlineUpgradeDownloadAdviseCookie;
extern "C" WestwoodOnlineUpgradeDownloadReadyEntry *g_pWestwoodOnlineUpgradeDownloadReadyList;
extern "C" char g_WestwoodOnlineUpgradeDownloadReadyPromptText[0x80];
extern "C" char g_WestwoodOnlineUpgradeDownloadRestoreCwd[0x100];
extern "C" int g_WestwoodOnlineUpgradeDownloadDialogResult;
extern "C" HWND g_hWestwoodOnlineUpgradeProgressDialog;
extern const CLSID g_CLSID_WestwoodOnlineUpgradeDownload;
extern const IID g_IID_WestwoodOnlineUpgradeDownload;
extern const IID IID_WestwoodOnlineUpgradeDownloadEventSink;

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
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeDownloadEventSink) == 0x20);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDownloadEventSink,
        m_refCountAndLock
    ) == 0x04
);
