#include "GameZRecoil/wwonline/upgrade_download.h"

#include "GameZRecoil/zCom/zCom.h"

extern "C" IUnknown *g_pWestwoodOnlineUpgradeDownload = 0;
extern "C" void *g_pWestwoodOnlineUpgradeDownloadEventSink = 0;
extern "C" DWORD g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0;
extern "C" int g_WestwoodOnlineUpgradeDownloadEventSinkConnectionOffset = 0;
extern "C" WestwoodOnlineUpgradeDownloadReadyEntry
    *g_pWestwoodOnlineUpgradeDownloadReadyList = 0;
extern "C" char g_WestwoodOnlineUpgradeDownloadReadyPromptText[0x80] = {0};
extern "C" char g_WestwoodOnlineUpgradeDownloadRestoreCwd[0x100] = {0};
extern "C" int g_WestwoodOnlineUpgradeDownloadDialogResult = 0;
extern "C" HWND g_hWestwoodOnlineUpgradeProgressDialog = 0;

// Vtable identity installed by 0x4425c0. Slot bodies are separate
// reconstruction targets; this table intentionally carries no retail addresses.
WestwoodOnlineUpgradeDownloadEventSinkVtable
    g_WestwoodOnlineUpgradeDownloadEventSink_Vtbl = {0};

// BN observes these COM identity objects in the WOL download ActiveX path.
// Exact GUID contents remain future provider-interface recovery evidence.
const CLSID g_WestwoodOnlineUpgradeDownload_CLSID = {0};
const IID g_WestwoodOnlineUpgradeDownload_IID = {0};
const IID g_WestwoodOnlineUpgradeDownloadEventSink_IID = {0};

// Reimplements 0x414b50: WestwoodOnlineUpgradeDownloadEventSink::CallbackNoOp
// (D:\Proj\GameZRecoil\wwonline\upgrade_download.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL
WestwoodOnlineUpgradeDownloadEventSink::CallbackNoOp(void *)
{
    return 0;
}

// Reimplements 0x4425c0: WestwoodOnlineUpgradeDownloadEventSink::CreateInstance
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownloadEventSink.cpp)
RECOIL_NOINLINE HRESULT RECOIL_STDCALL
WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(
    WestwoodOnlineUpgradeDownloadEventSink **outSink)
{
    HRESULT result = E_OUTOFMEMORY;
    WestwoodOnlineUpgradeDownloadEventSink *eventSink =
        (WestwoodOnlineUpgradeDownloadEventSink *)(::operator new(
            sizeof(WestwoodOnlineUpgradeDownloadEventSink)));

    if (eventSink != 0)
    {
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
RECOIL_NOINLINE HRESULT RECOIL_CDECL
WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise()
{
    CoCreateInstance(g_WestwoodOnlineUpgradeDownload_CLSID,
                     0,
                     CLSCTX_INPROC_SERVER,
                     g_WestwoodOnlineUpgradeDownload_IID,
                     (void **)&g_pWestwoodOnlineUpgradeDownload);
    WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(
        (WestwoodOnlineUpgradeDownloadEventSink **)
            &g_pWestwoodOnlineUpgradeDownloadEventSink);
    return zCom::ConnectionPointContainer_Advise(
        g_pWestwoodOnlineUpgradeDownload,
        (IUnknown *)((unsigned char *)g_pWestwoodOnlineUpgradeDownloadEventSink +
                     g_WestwoodOnlineUpgradeDownloadEventSinkConnectionOffset),
        g_WestwoodOnlineUpgradeDownloadEventSink_IID,
        &g_WestwoodOnlineUpgradeDownloadAdviseCookie);
}

// Reimplements 0x4422f0: WestwoodOnlineUpgradeDownload::UnadviseAndRelease
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDownload.cpp)
RECOIL_NOINLINE ULONG RECOIL_CDECL
WestwoodOnlineUpgradeDownload::UnadviseAndRelease()
{
    zCom::ConnectionPointContainer_Unadvise(
        g_pWestwoodOnlineUpgradeDownload,
        g_WestwoodOnlineUpgradeDownloadEventSink_IID,
        g_WestwoodOnlineUpgradeDownloadAdviseCookie);
    return g_pWestwoodOnlineUpgradeDownload->Release();
}
