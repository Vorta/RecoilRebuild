#include "Battlesport/WestwoodOnlineUpgradeApi.h"

#include "Battlesport/RecoilApp.h"
#include "Battlesport/WestwoodOnlineUpgradeApiEventSink.h"
#include "Battlesport/WestwoodOnlineUpgradeConfigDialog.h"
#include "GameZRecoil/zCom/zCom.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <string.h>

struct IWestwoodOnlineUpgradeApi : IUnknown {
    virtual void STDMETHODCALLTYPE ProcessCallbacks() = 0;
    virtual void STDMETHODCALLTYPE BeginConnect(
        int languageId,
        int productId,
        const char *playerName,
        const char *connectString,
        int timeoutSeconds
    ) = 0;
    virtual void STDMETHODCALLTYPE RequestBootstrapServerList(
        WestwoodOnlineUpgradeBootstrapServerRecord *selectedBootstrapServer,
        int timeoutSeconds,
        int useAlternateConnectString
    ) = 0;
    virtual void STDMETHODCALLTYPE RequestListMode(
        int listMode,
        int enabled
    ) = 0;
};

extern "C" WestwoodOnlineUpgradeApiInitState g_WestwoodOnlineUpgradeApiInitState = {0};
extern "C" IUnknown *g_pWestwoodOnlineUpgradeApi = 0;
extern "C" void *g_pWestwoodOnlineUpgradeApiEventSink = 0;
extern "C" DWORD g_WestwoodOnlineUpgradeApiConnectionCookie = 0;
extern "C" int g_WestwoodOnlineUpgradeApiEventSinkConnectionOffset = 0;
extern "C" int g_WestwoodOnlineUpgradeApiReadyFlag = 0;
extern "C" int g_WestwoodOnlineUpgradeApiShutdownState = 0;
extern "C" int g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
extern "C" int g_WestwoodOnlineUpgradeAbortFlag = 0;
extern "C" int g_WestwoodOnlineUpgradeActiveListMode = 0;
extern "C" int g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
extern "C" int g_WestwoodOnlineUpgradePendingSessionResultCount = 0;
extern "C" int g_WestwoodOnlineUpgradeVisibleSessionResultCount = 0;
extern "C" int g_WestwoodOnlineUpgradeProcessCallbacksFlag = 0;
extern "C" float g_WestwoodOnlineUpgradeNextAutoRefreshTime = 0.0f;
extern "C" int g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeInitWaitEvents[3] = {0};
extern "C" HANDLE g_WestwoodOnlineUpgradeBootstrapServerListEvent = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeStatusTextEvent = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeFailureEvent = 0;
extern "C" WestwoodOnlineUpgradeBootstrapServerRecord
    g_WestwoodOnlineUpgradeSelectedBootstrapServer = {0};
extern "C" WestwoodOnlineUpgradeBrowseRecord g_WestwoodOnlineUpgradeCachedBrowseRecord = {0};
extern "C" WestwoodOnlineUpgradeBrowseRecord g_WestwoodOnlineUpgradeCachedBrowseRecordList[1024] = {
    {0}};
extern "C" int g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeCloseHandle0 = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeCloseHandle1 = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeCloseHandle2 = 0;

// BN observes these COM identity objects in the WOL ActiveX startup path.
// g_WestwoodOnlineUpgradeApiEventSink_IID is the recovered IID referenced by
// the event-sink interface map at 0x4d1ba0.
const CLSID g_WestwoodOnlineUpgradeApi_CLSID = {0};
const IID g_WestwoodOnlineUpgradeApi_IID = {0};
const IID g_WestwoodOnlineUpgradeApiEventSink_IID = {
    0x4dd3baf6,
    0x7579,
    0x11d1,
    {0xb1, 0xc6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56},
};

namespace {
const unsigned int kWestwoodOnlineUpgradeInitStateSize = sizeof(WestwoodOnlineUpgradeApiInitState);
const unsigned int kFailureMessageBufferSize = 128;
const unsigned int kWolApiFailureCaptionMessageId = 0x3030;
const unsigned int kWolApiFailureTextMessageId = 0x302f;
const UINT kWolApiFailureMessageBoxType = MB_ICONHAND;
const UINT kWestwoodOnlineUpgradeProgressDialogResourceId = 157;
const int kWestwoodOnlineUpgradeProgressStatusControlId = 1179;
const int kWestwoodOnlineUpgradeDialogServerControlId = 154;
const unsigned int kWolApiInitConnectingMessageId = 0x3033;
const unsigned int kWolApiInitReadyMessageId = 0x3034;
const unsigned int kWolApiInitFailureCaptionMessageId = 0x3032;
const unsigned int kWolApiInitFailureTextMessageId = 0x3035;
const unsigned int kWolApiFailureMessageBufferSize = 128;
const int kWolLanguageDefault = 0x1100;
const int kWolLanguageGerman = 0x1102;
const int kWolLanguageFrench = 0x1103;
const int kWolProductId = 0x10003;
const int kWolConnectTimeoutSeconds = 60;
const int kWolBootstrapTimeoutSeconds = 30;
const int kWolRequestListMode = 17;
const DWORD kInitialWaitTimeoutMs = 0;
const DWORD kCallbackSleepMs = 300;
const DWORD kBootstrapWaitTimeoutMs = 5;
const DWORD kFailureDisplaySleepMs = 1000;

/**
 * Local helper with no standalone retail function; observed in callers 0x43d130
 * and 0x43d2e0 as inlined localized failure-message copies.
 *
 * Purpose: copy a localized Westwood Online failure message into a stack buffer.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
void CopyFailureMessage(
    char *destination,
    const char *source
) {
    strcpy(
        destination,
        source
    );
}

// Source-faithful helper recovered from address-backed callers in this source file.
IWestwoodOnlineUpgradeApi *GetApiComObject() {
    return (IWestwoodOnlineUpgradeApi *)g_pWestwoodOnlineUpgradeApi;
}

/**
 * Local helper with no standalone retail function; observed in caller 0x43d2e0
 * at the progress-dialog DestroyWindow dispatch.
 *
 * Purpose: destroy the modal Westwood Online progress dialog through its MFC
 * provider virtual slot.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
void DestroyProgressDialog() {
    CWnd *const progressWnd = (CWnd *)g_pWestwoodOnlineUpgradeProgressDialog;
    progressWnd->DestroyWindow();
}

/**
 * Local helper with no standalone retail function; observed in caller 0x43d2e0
 * as the GetSystemDefaultLangID language switch.
 *
 * Purpose: choose the Westwood Online language id for German, French, or the
 * default localized startup path.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
int GetWolLanguageId() {
    const LANGID primaryLanguage = GetSystemDefaultLangID() & 0x3ff;
    if (primaryLanguage == LANG_GERMAN) {
        return kWolLanguageGerman;
    }
    if (primaryLanguage == LANG_FRENCH) {
        return kWolLanguageFrench;
    }
    return kWolLanguageDefault;
}

/**
 * Local helper with no standalone retail function; observed in caller 0x43d2e0
 * as the initial WAIT_TIMEOUT callback pump.
 *
 * Purpose: process WOL API callbacks until the initial connect/status/failure
 * wait set leaves the timeout state.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
void PumpInitialCallbacksUntilEvent(
    DWORD *waitResult
) {
    while (*waitResult == WAIT_TIMEOUT) {
        if (g_WestwoodOnlineUpgradeApiReadyFlag != 0 &&
            g_WestwoodOnlineUpgradeApiAsyncErrorFlag == 0) {
            IWestwoodOnlineUpgradeApi *const api = GetApiComObject();
            api->ProcessCallbacks();
        }

        Sleep(kCallbackSleepMs);
        if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0) {
            return;
        }

        *waitResult = WaitForMultipleObjects(
            3,
            g_WestwoodOnlineUpgradeInitWaitEvents,
            FALSE,
            kInitialWaitTimeoutMs
        );
    }
}

/**
 * Local helper with no standalone retail function; observed in caller 0x43d2e0
 * as the bootstrap-server wait loop.
 *
 * Purpose: process WOL API callbacks while waiting for the bootstrap server
 * list/status/failure events.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
DWORD PumpBootstrapCallbacksUntilEvent() {
    DWORD waitResult = WaitForMultipleObjects(
        3,
        g_WestwoodOnlineUpgradeInitWaitEvents,
        FALSE,
        kBootstrapWaitTimeoutMs
    );
    while (waitResult == WAIT_TIMEOUT) {
        IWestwoodOnlineUpgradeApi *const api = GetApiComObject();
        api->ProcessCallbacks();
        if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0) {
            break;
        }

        waitResult = WaitForMultipleObjects(
            3,
            g_WestwoodOnlineUpgradeInitWaitEvents,
            FALSE,
            kBootstrapWaitTimeoutMs
        );
    }
    return waitResult;
}
} // namespace

/**
 * Reimplements 0x42dda0: WestwoodOnlineUpgradeApiInitState::Init
 * (D:\Proj\Battlesport\WestwoodOnlineUpgradeApi.cpp).
 *
 * Purpose: validate and initialize the transient WOL bootstrap-state block,
 * module handles, events, and critical sections.
 */
HRESULT __stdcall WestwoodOnlineUpgradeApiInitState::Init(
    WestwoodOnlineUpgradeApiInitState *self,
    HANDLE bootstrapServerListEvent,
    HINSTANCE moduleHandle
) {
    if (self == 0) {
        return E_INVALIDARG;
    }

    if (self->structSize < sizeof(WestwoodOnlineUpgradeApiInitState)) {
        return E_INVALIDARG;
    }

    self->statusTextEvent = 0;
    self->failureEvent = 0;
    self->bootstrapServerListEvent = bootstrapServerListEvent;
    self->moduleHandleSecondary = moduleHandle;
    self->moduleHandleTertiary = moduleHandle;
    self->moduleHandlePrimary = moduleHandle;
    InitializeCriticalSection(&self->criticalSection0);
    InitializeCriticalSection(&self->criticalSection1);
    InitializeCriticalSection(&self->criticalSection2);
    return S_OK;
}

/**
 * Reimplements 0x43d2e0: WestwoodOnlineUpgradeApi::Init
 * (D:\Proj\Battlesport\WestwoodOnlineUpgradeApi.cpp).
 *
 * Purpose: create the WOL startup events and progress UI, begin the selected
 * profile connection, request bootstrap servers, and enter list mode on success.
 */
int WestwoodOnlineUpgradeApi::Init() {
    char failureCaption[kWolApiFailureMessageBufferSize];
    char failureText[kWolApiFailureMessageBufferSize];

    memset(
        &g_WestwoodOnlineUpgradeCachedBrowseRecord,
        0,
        sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
    );
    zGame::ReturnOnlyStub();

    WestwoodOnlineUpgradeApi api;
    if (api.CreateInstanceAndLoadConfig(g_hWestwoodOnlineUpgradeModuleInstance) == 0) {
        return 0;
    }

    g_WestwoodOnlineUpgradeBootstrapServerListEvent = CreateEventA(
        0,
        FALSE,
        FALSE,
        0
    );
    g_WestwoodOnlineUpgradeStatusTextEvent = CreateEventA(
        0,
        FALSE,
        FALSE,
        0
    );
    g_WestwoodOnlineUpgradeFailureEvent = CreateEventA(
        0,
        FALSE,
        FALSE,
        0
    );
    g_WestwoodOnlineUpgradeInitWaitEvents[0] = g_WestwoodOnlineUpgradeBootstrapServerListEvent;
    g_WestwoodOnlineUpgradeInitWaitEvents[1] = g_WestwoodOnlineUpgradeStatusTextEvent;
    g_WestwoodOnlineUpgradeInitWaitEvents[2] = g_WestwoodOnlineUpgradeFailureEvent;

    ((CDialog *)g_pWestwoodOnlineUpgradeProgressDialog)
        ->Create(
            (LPCSTR)kWestwoodOnlineUpgradeProgressDialogResourceId,
            0
        );
    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeProgressStatusControlId,
            zLoc::GetMessageString(kWolApiInitConnectingMessageId)
        );

    CString connectString;
    CString playerName;
    g_pWestwoodOnlineUpgradeDialog->GetSelectedProfileConnectString(&connectString);
    g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName(&playerName);

    IWestwoodOnlineUpgradeApi *apiCom = GetApiComObject();
    apiCom->BeginConnect(
        GetWolLanguageId(),
        kWolProductId,
        (const char *)playerName,
        (const char *)connectString,
        kWolConnectTimeoutSeconds
    );

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    DWORD waitResult = WaitForMultipleObjects(
        3,
        g_WestwoodOnlineUpgradeInitWaitEvents,
        FALSE,
        kInitialWaitTimeoutMs
    );
    PumpInitialCallbacksUntilEvent(&waitResult);
    if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0 || waitResult == WAIT_OBJECT_0 + 2) {
        ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)
            ->SetDlgItemTextA(
                kWestwoodOnlineUpgradeProgressStatusControlId,
                zLoc::GetMessageString(kWolApiInitFailureTextMessageId)
            );
        Sleep(kFailureDisplaySleepMs);
        DestroyProgressDialog();
        return 0;
    }

    g_WestwoodOnlineUpgradeAbortFlag = 0;
    ResetEvent(g_WestwoodOnlineUpgradeFailureEvent);
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;

    apiCom = GetApiComObject();
    apiCom->RequestBootstrapServerList(
        &g_WestwoodOnlineUpgradeSelectedBootstrapServer,
        kWolBootstrapTimeoutSeconds,
        g_pWestwoodOnlineUpgradeDialog->m_selectedProfileConnectStringMode == 0 ? 1 : 0
    );

    waitResult = PumpBootstrapCallbacksUntilEvent();
    DestroyProgressDialog();
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeDialogServerControlId,
            zLoc::GetMessageString(kWolApiInitReadyMessageId)
        );

    if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0) {
        return 0;
    }

    if (waitResult != WAIT_OBJECT_0 + 2) {
        apiCom = GetApiComObject();
        apiCom->RequestListMode(
            kWolRequestListMode,
            1
        );
        return 1;
    }

    CopyFailureMessage(
        failureCaption,
        zLoc::GetMessageString(kWolApiInitFailureCaptionMessageId)
    );
    CopyFailureMessage(
        failureText,
        zLoc::GetMessageString(kWolApiInitFailureTextMessageId)
    );
    ((CWnd *)((unsigned int)g_RecoilApp.m_pMainWnd))
        ->MessageBoxA(
            failureText,
            failureCaption,
            kWolApiFailureMessageBoxType
        );
    return 0;
}

/**
 * Reimplements 0x43d130: WestwoodOnlineUpgradeApi::CreateInstanceAndLoadConfig
 * (D:\Proj\Battlesport\WestwoodOnlineUpgradeApi.cpp).
 *
 * Purpose: initialize COM/MFC control hosting, create the WOL ActiveX API,
 * advise the event sink, and apply the selected upgrade profile.
 */
int WestwoodOnlineUpgradeApi::CreateInstanceAndLoadConfig(
    HANDLE bootstrapServerListEvent
) {
    char failureCaption[kFailureMessageBufferSize];
    char failureText[kFailureMessageBufferSize];

    CoInitialize(0);
    g_WestwoodOnlineUpgradeApiInitState.structSize = kWestwoodOnlineUpgradeInitStateSize;
    g_WestwoodOnlineUpgradeApiShutdownState = 0;
    WestwoodOnlineUpgradeApiInitState::Init(
        &g_WestwoodOnlineUpgradeApiInitState,
        bootstrapServerListEvent,
        0
    );
    AfxEnableControlContainer(0);
    CoCreateInstance(
        g_WestwoodOnlineUpgradeApi_CLSID,
        0,
        CLSCTX_INPROC_SERVER,
        g_WestwoodOnlineUpgradeApi_IID,
        (void **)&g_pWestwoodOnlineUpgradeApi
    );

    if (g_pWestwoodOnlineUpgradeApi == 0) {
        CopyFailureMessage(
            failureCaption,
            zLoc::GetMessageString(kWolApiFailureCaptionMessageId)
        );
        CopyFailureMessage(
            failureText,
            zLoc::GetMessageString(kWolApiFailureTextMessageId)
        );
        MessageBeep(kWolApiFailureMessageBoxType);
        ((CWnd *)((unsigned int)g_RecoilApp.GetMainWnd()))
            ->MessageBoxA(
                failureText,
                failureCaption,
                kWolApiFailureMessageBoxType
            );
        return 0;
    }

    g_WestwoodOnlineUpgradeApiReadyFlag = 1;
    WestwoodOnlineUpgradeApiEventSink::CreateInstance(
        (WestwoodOnlineUpgradeApiEventSink **)&g_pWestwoodOnlineUpgradeApiEventSink
    );
    zCom::ConnectionPointContainer_Advise(
        g_pWestwoodOnlineUpgradeApi,
        (IUnknown *)((unsigned char *)g_pWestwoodOnlineUpgradeApiEventSink +
                     g_WestwoodOnlineUpgradeApiEventSinkConnectionOffset),
        g_WestwoodOnlineUpgradeApiEventSink_IID,
        &g_WestwoodOnlineUpgradeApiConnectionCookie
    );

    if (WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues()) {
        return 1;
    }

    WestwoodOnlineUpgradeApi::Shutdown();
    return 0;
}

/**
 * Reimplements 0x43d280: WestwoodOnlineUpgradeApi::Shutdown
 * (D:\Proj\Battlesport\WestwoodOnlineUpgradeApi.cpp).
 *
 * Purpose: unadvise the WOL event sink, release the API COM object, and close
 * the startup wait handles.
 */
void WestwoodOnlineUpgradeApi::Shutdown() {
    if (g_pWestwoodOnlineUpgradeApi == 0) {
        return;
    }

    zCom::ConnectionPointContainer_Unadvise(
        g_pWestwoodOnlineUpgradeApi,
        g_WestwoodOnlineUpgradeApiEventSink_IID,
        g_WestwoodOnlineUpgradeApiConnectionCookie
    );
    g_pWestwoodOnlineUpgradeApi->Release();
    g_pWestwoodOnlineUpgradeApi = 0;
    CloseHandle(g_WestwoodOnlineUpgradeCloseHandle0);
    CloseHandle(g_WestwoodOnlineUpgradeCloseHandle1);
    CloseHandle(g_WestwoodOnlineUpgradeCloseHandle2);
}
