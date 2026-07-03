#include "Battlesport/wol_api.h"

#include "Battlesport/recoil_app.h"
#include "Battlesport/wol_api_event_sink.h"
#include "Battlesport/wol_config_dialog.h"
#include "GameZRecoil/zCom/zCom.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <string.h>

/**
 * Reimplements data 0x4dd24c: g_WestwoodOnlineUpgradeAbortFlag.
 * Purpose: carries the upgrade dialog/provider disconnect abort state, starting
 * in the aborted state until the API init flow clears it.
 */
extern "C" int g_WestwoodOnlineUpgradeAbortFlag = 1;

// WestwoodOnline owns the WOL ActiveX API startup globals. Current BN evidence
// shows these as independent zero-initialized or immutable data symbols used by
// the API startup anchors; dialog/session-browser globals defined in this
// translation unit are tracked as a separate data-owner blocker.
// API startup wait/event state.
extern "C" HANDLE g_WestwoodOnlineUpgradeInitWaitEvents[3] = {0};
extern "C" HANDLE g_WestwoodOnlineUpgradeFailureEvent = 0;
extern "C" WestwoodOnlineUpgradeBootstrapServerRecord
    g_WestwoodOnlineUpgradeSelectedBootstrapServer = {0};

// Session-browser/dialog state: defined here by the recovered source file, but
// not part of the API startup data owner.
extern "C" int g_WestwoodOnlineUpgradeActiveListMode = 0;

// API callback pump state.
extern "C" int g_WestwoodOnlineUpgradeProcessCallbacksFlag = 0;

extern "C" WestwoodOnlineUpgradeApiInitState g_WestwoodOnlineUpgradeApiInitState = {0};
extern "C" int g_WestwoodOnlineUpgradeApiShutdownState = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeStatusTextEvent = 0;
extern "C" HANDLE g_WestwoodOnlineUpgradeBootstrapServerListEvent = 0;

// Session-browser cache state. 0x43d2e0 clears the current record at startup,
// but the broader browse/cache owner remains the dialog/event-sink slice.
extern "C" WestwoodOnlineUpgradeBrowseRecord g_WestwoodOnlineUpgradeCachedBrowseRecord = {0};
extern "C" WestwoodOnlineUpgradeBrowseRecord g_WestwoodOnlineUpgradeCachedBrowseRecordList[1024] = {
    {0}};

extern "C" int g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 0;
extern "C" void *g_pWestwoodOnlineUpgradeApiEventSink = 0;
extern "C" int g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
extern "C" DWORD g_WestwoodOnlineUpgradeApiAdviseCookie = 0;
extern "C" IUnknown *g_pWestwoodOnlineUpgradeApi = 0;

// More session-browser/dialog state.
extern "C" int g_WestwoodOnlineUpgradePendingSessionResultCount = 0;
extern "C" int g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = 0;
extern "C" int g_WestwoodOnlineUpgradeVisibleSessionResultCount = 0;
extern "C" int g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
extern "C" float g_WestwoodOnlineUpgradeNextAutoRefreshTime = 0.0f;

// BN observes the same COM identity bytes as g_CLSID_WestwoodOnlineUpgradeApi,
// g_IID_WestwoodOnlineUpgradeApi, and IID_WestwoodOnlineUpgradeApiEventSink.
// The source names below preserve the recovered API/event-sink naming used by
// this file and the event-sink interface map at 0x4d1ba0.
const CLSID g_WestwoodOnlineUpgradeApi_CLSID = {
    0x4dd3baf5,
    0x7579,
    0x11d1,
    {0xb1, 0xc6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56},
};
const IID g_WestwoodOnlineUpgradeApi_IID = {
    0x4dd3baf4,
    0x7579,
    0x11d1,
    {0xb1, 0xc6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56},
};
const IID g_WestwoodOnlineUpgradeApiEventSink_IID = {
    0x4dd3baf6,
    0x7579,
    0x11d1,
    {0xb1, 0xc6, 0x00, 0x60, 0x97, 0x17, 0x65, 0x56},
};

// BN 0x4d1ba0 owns the event-sink zCom interface map; 0x43d130 reads the
// first entry's offset field rather than a standalone source global.
extern const zCom::InterfaceMapEntry g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap[2];

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
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in callers 0x43d130 and 0x43d2e0 as inlined localized
 * failure-message copies.
 *
 * Purpose: copy a localized Westwood Online failure message into a stack buffer.
 */
void CopyFailureMessage(
    char *destination,
    const char *source
) {
    strcpy(
        destination,
        source
    );
}

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 and dependent WOL dialog/event-sink source as a
 * typed access to the API COM pointer.
 *
 * Purpose: return the global WOL ActiveX API pointer as the recovered provider
 * interface type.
 */
IWestwoodOnlineUpgradeProviderApi *GetApiComObject() {
    return (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
}

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 at the progress-dialog DestroyWindow dispatch.
 *
 * Purpose: destroy the modal Westwood Online progress dialog through its MFC
 * provider virtual slot.
 */
void DestroyProgressDialog() {
    CWnd *const progressWnd = (CWnd *)g_pWestwoodOnlineUpgradeProgressDialog;
    progressWnd->DestroyWindow();
}

/**
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 as the GetSystemDefaultLangID language switch.
 *
 * Purpose: choose the Westwood Online language id for German, French, or the
 * default localized startup path.
 */
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
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 as the initial WAIT_TIMEOUT callback pump.
 *
 * Purpose: process WOL API callbacks until the initial connect/status/failure
 * wait set leaves the timeout state.
 */
void PumpInitialCallbacksUntilEvent(
    DWORD *waitResult
) {
    while (*waitResult == WAIT_TIMEOUT) {
        if (g_WestwoodOnlineUpgradeProcessCallbacksFlag != 0 &&
            g_WestwoodOnlineUpgradeApiAsyncErrorFlag == 0) {
            IWestwoodOnlineUpgradeProviderApi *const api = GetApiComObject();
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
 * Original inline helper evidence: no standalone retail function exists.
 * Observed in caller 0x43d2e0 as the bootstrap-server wait loop.
 *
 * Purpose: process WOL API callbacks while waiting for the bootstrap server
 * list/status/failure events.
 */
DWORD PumpBootstrapCallbacksUntilEvent() {
    DWORD waitResult = WaitForMultipleObjects(
        3,
        g_WestwoodOnlineUpgradeInitWaitEvents,
        FALSE,
        kBootstrapWaitTimeoutMs
    );
    while (waitResult == WAIT_TIMEOUT) {
        IWestwoodOnlineUpgradeProviderApi *const api = GetApiComObject();
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
 * module handles, event-sink live count, and critical sections.
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

    self->eventSinkLiveCount = 0;
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

    CString connectString = g_pWestwoodOnlineUpgradeDialog->GetSelectedProfileConnectString();
    CString playerName = g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName();

    IWestwoodOnlineUpgradeProviderApi *apiCom = GetApiComObject();
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

    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 1;
    WestwoodOnlineUpgradeApiEventSink::CreateInstance(
        (WestwoodOnlineUpgradeApiEventSink **)&g_pWestwoodOnlineUpgradeApiEventSink
    );
    zCom::ConnectionPointContainer_Advise(
        g_pWestwoodOnlineUpgradeApi,
        (IUnknown *)((unsigned char *)g_pWestwoodOnlineUpgradeApiEventSink +
                     g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap[0].interfaceOffset),
        g_WestwoodOnlineUpgradeApiEventSink_IID,
        &g_WestwoodOnlineUpgradeApiAdviseCookie
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
        g_WestwoodOnlineUpgradeApiAdviseCookie
    );
    g_pWestwoodOnlineUpgradeApi->Release();
    g_pWestwoodOnlineUpgradeApi = 0;
    CloseHandle(g_WestwoodOnlineUpgradeBootstrapServerListEvent);
    CloseHandle(g_WestwoodOnlineUpgradeStatusTextEvent);
    CloseHandle(g_WestwoodOnlineUpgradeFailureEvent);
}
