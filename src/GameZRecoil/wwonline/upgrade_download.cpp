#include "Battlesport/Mfc42Abi.h"
#include "GameZRecoil/wwonline/upgrade_download.h"

#include "Battlesport/WestwoodOnlineUpgradeDialog.h"
#include "GameZRecoil/zCom/zCom.h"

#include <commctrl.h>

extern "C" IUnknown *g_pWestwoodOnlineUpgradeDownload = 0;
extern "C" void *g_pWestwoodOnlineUpgradeDownloadEventSink = 0;
extern "C" DWORD g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0;
extern "C" int g_WestwoodOnlineUpgradeDownloadEventSinkConnectionOffset = 0;
extern "C" WestwoodOnlineUpgradeDownloadReadyEntry *g_pWestwoodOnlineUpgradeDownloadReadyList = 0;
extern "C" char g_WestwoodOnlineUpgradeDownloadReadyPromptText[0x80] = {0};
extern "C" char g_WestwoodOnlineUpgradeDownloadRestoreCwd[0x100] = {0};
extern "C" int g_WestwoodOnlineUpgradeDownloadDialogResult = 0;
extern "C" HWND g_hWestwoodOnlineUpgradeProgressDialog = 0;

// Vtable identity installed by 0x4425c0. Slot bodies are separate
// reconstruction targets; this table intentionally carries no retail addresses.
WestwoodOnlineUpgradeDownloadEventSinkVtable g_WestwoodOnlineUpgradeDownloadEventSink_Vtbl = {0};

// BN observes these COM identity objects in the WOL download ActiveX path.
// Exact GUID contents remain future provider-interface recovery evidence.
const CLSID g_WestwoodOnlineUpgradeDownload_CLSID = {0};
const IID g_WestwoodOnlineUpgradeDownload_IID = {0};
const IID g_WestwoodOnlineUpgradeDownloadEventSink_IID = {0};

// Recovered interface map used by 0x4427d0. BN data at 0x4d1fc8 contains
// {IID_WestwoodOnlineUpgradeDownloadEventSink, offset 0, direct} followed by end.
const zCom::InterfaceMapEntry g_WestwoodOnlineUpgradeDownloadEventSink_InterfaceMap[2] = {
    {&g_WestwoodOnlineUpgradeDownloadEventSink_IID, 0, zCom::ZCOM_INTERFACE_MAP_DIRECT},
    {0, 0, zCom::ZCOM_INTERFACE_MAP_END},
};

namespace {
const char kDownloadFinishedStatusText[] = "Finished!";
const char kDownloadErrorStatusText[] = "ERROR";
const char kDownloadProgressStatusText[] = "Bytes read: %d / %d";
const char kDownloadProgressWithTimeStatusText[] = "Bytes read: %d / %d.    Time left: %d seconds";
const char kDownloadStateConnectingText[] = "Connecting...";
const char kDownloadStateFindingPatchText[] = "Finding patch...";
const char kDownloadStateDownloadingPatchText[] = "Downloading patch...";
const int kDownloadProgressControlId = 1021;
const unsigned int kDownloadProgressPercentScale = 100;
const DWORD kDownloadErrorStatusSleepMs = 1000;
} // namespace

// Reimplements 0x414b50: WestwoodOnlineUpgradeDownloadEventSink::CallbackNoOp
// (D:\Proj\GameZRecoil\wwonline\upgrade_download.cpp)
int WestwoodOnlineUpgradeDownloadEventSink::CallbackNoOp(
    void *
) {
    return 0;
}

// Reimplements 0x442660: WestwoodOnlineUpgradeDownloadEventSink::OnDownloadFinished
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
HRESULT STDMETHODCALLTYPE
WestwoodOnlineUpgradeDownloadEventSink::OnDownloadFinished(
    IUnknown *
) {
    WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadFinishedStatusText);
    g_WestwoodOnlineUpgradeDownloadDialogResult = 1;
    return S_OK;
}

// Reimplements 0x442680: WestwoodOnlineUpgradeDownloadEventSink::OnDownloadError
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::OnDownloadError(
    IUnknown *,
    HRESULT
) {
    WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadErrorStatusText);
    Sleep(kDownloadErrorStatusSleepMs);
    g_WestwoodOnlineUpgradeDownloadDialogResult = -1;
    return S_OK;
}

// Reimplements 0x4426b0: WestwoodOnlineUpgradeDownloadEventSink::OnDownloadProgress
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
HRESULT STDMETHODCALLTYPE
WestwoodOnlineUpgradeDownloadEventSink::OnDownloadProgress(
    IUnknown *,
    unsigned int bytesRead,
    unsigned int totalBytes,
    int,
    int secondsLeft
) {
    SendDlgItemMessageA(
        g_hWestwoodOnlineUpgradeProgressDialog,
        kDownloadProgressControlId,
        PBM_SETPOS,
        (bytesRead * kDownloadProgressPercentScale) / totalBytes,
        0
    );
    if (secondsLeft > 0) {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(
            kDownloadProgressWithTimeStatusText,
            bytesRead,
            totalBytes,
            secondsLeft
        );
    } else {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(
            kDownloadProgressStatusText,
            bytesRead,
            totalBytes
        );
    }
    return S_OK;
}

// Reimplements 0x442720: WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged(
    IUnknown *,
    WestwoodOnlineUpgradeDownloadState stateCode
) {
    if (stateCode == WOL_DOWNLOAD_STATE_CONNECTING) {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadStateConnectingText);
    } else if (stateCode == WOL_DOWNLOAD_STATE_FINDING_PATCH) {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadStateFindingPatchText);
    } else if (stateCode == WOL_DOWNLOAD_STATE_DOWNLOADING_PATCH) {
        WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(kDownloadStateDownloadingPatchText);
    }
    return S_OK;
}

// Reimplements 0x442770: WestwoodOnlineUpgradeDownloadEventSink::AddRef
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
ULONG STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::AddRef(
    WestwoodOnlineUpgradeDownloadEventSink *self
) {
    return (ULONG)InterlockedIncrement(&self->m_refCountAndLock.refCount);
}

// Reimplements 0x442790: WestwoodOnlineUpgradeDownloadEventSink::Release
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
ULONG STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::Release(
    WestwoodOnlineUpgradeDownloadEventSink *self
) {
    ULONG refCount;

    refCount = (ULONG)InterlockedDecrement(&self->m_refCountAndLock.refCount);
    if (refCount == 0 && self != 0) {
        self->Destructor();
        delete self;
    }

    return refCount;
}

// Reimplements 0x4427d0: WestwoodOnlineUpgradeDownloadEventSink::QueryInterface
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
HRESULT __stdcall WestwoodOnlineUpgradeDownloadEventSink::QueryInterface(
    WestwoodOnlineUpgradeDownloadEventSink *self,
    REFIID iid,
    void **outInterface
) {
    return zCom::QueryInterfaceFromInterfaceMap(
        self,
        g_WestwoodOnlineUpgradeDownloadEventSink_InterfaceMap,
        &iid,
        outInterface
    );
}

// Reimplements 0x4427f0: WestwoodOnlineUpgradeDownloadEventSink::Destructor
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
void WestwoodOnlineUpgradeDownloadEventSink::Destructor() {
    m_vftable = &g_WestwoodOnlineUpgradeDownloadEventSink_Vtbl;
    m_refCountAndLock.refCount = 1;
    InterlockedDecrement(&g_WestwoodOnlineUpgradeEventSinkLiveCount);
    DeleteCriticalSection(&m_refCountAndLock.lock);
}

// Reimplements 0x4425c0: WestwoodOnlineUpgradeDownloadEventSink::CreateInstance
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
HRESULT __stdcall WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(
    WestwoodOnlineUpgradeDownloadEventSink **outSink
) {
    HRESULT result = E_OUTOFMEMORY;
    WestwoodOnlineUpgradeDownloadEventSink *eventSink =
        (WestwoodOnlineUpgradeDownloadEventSink *)(::operator new(
            sizeof(WestwoodOnlineUpgradeDownloadEventSink)
        ));

    if (eventSink != 0) {
        eventSink->m_refCountAndLock.Init();
        eventSink->m_vftable = &g_WestwoodOnlineUpgradeDownloadEventSink_Vtbl;
        InterlockedIncrement(&g_WestwoodOnlineUpgradeEventSinkLiveCount);
        result = S_OK;
    }

    *outSink = eventSink;
    return result;
}

// Reimplements 0x4422a0: WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownload.cpp)
HRESULT WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise() {
    CoCreateInstance(
        g_WestwoodOnlineUpgradeDownload_CLSID,
        0,
        CLSCTX_INPROC_SERVER,
        g_WestwoodOnlineUpgradeDownload_IID,
        (void **)&g_pWestwoodOnlineUpgradeDownload
    );
    WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(
        (WestwoodOnlineUpgradeDownloadEventSink **)&g_pWestwoodOnlineUpgradeDownloadEventSink
    );
    return zCom::ConnectionPointContainer_Advise(
        g_pWestwoodOnlineUpgradeDownload,
        (IUnknown *)((unsigned char *)g_pWestwoodOnlineUpgradeDownloadEventSink +
                     g_WestwoodOnlineUpgradeDownloadEventSinkConnectionOffset),
        g_WestwoodOnlineUpgradeDownloadEventSink_IID,
        &g_WestwoodOnlineUpgradeDownloadAdviseCookie
    );
}

// Reimplements 0x4422f0: WestwoodOnlineUpgradeDownload::UnadviseAndRelease
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownload.cpp)
ULONG WestwoodOnlineUpgradeDownload::UnadviseAndRelease() {
    zCom::ConnectionPointContainer_Unadvise(
        g_pWestwoodOnlineUpgradeDownload,
        g_WestwoodOnlineUpgradeDownloadEventSink_IID,
        g_WestwoodOnlineUpgradeDownloadAdviseCookie
    );
    return g_pWestwoodOnlineUpgradeDownload->Release();
}
