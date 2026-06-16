#include "Battlesport/WestwoodOnlineUpgradeApiEventSink.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/WestwoodOnlineUpgradeApi.h"
#include "Battlesport/WestwoodOnlineUpgradeDialog.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/wwonline/upgrade_download.h"
#include "GameZRecoil/zCom/zCom.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zNetwork/zNetwork.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

namespace {
const int kPendingSessionRemovedStatusBufferSize = 128;
const unsigned int kPendingSessionRemovedStatusMessageId = 0x3004;
const int kWestwoodOnlineUpgradeSessionResultsListId = 1137;
const char kWestwoodOnlineUpgradeServerErrorTitle[] = "ServerError";
const int kApiStatusTextBufferSize = 128;
const char kApiStatusLineDelimiter[] = "\r";
const int kApiStatusActiveListMode = 17;
const int kApiStatusNoResultsMessageId = 0x3005;
const int kApiStatusOneResultMessageId = 0x3006;
const int kApiStatusMultipleResultsMessageId = 0x3007;
const int kApiStatusFailureCaptionMessageId = 0x3003;
const int kApiStatusFailureDefaultMessageId = 0x300c;
const int kApiStatusFailure64MessageId = 0x3008;
const int kApiStatusFailure65MessageId = 0x3009;
const int kApiStatusFailure6aMessageId = 0x300b;
const int kApiStatusFailure72MessageId = 0x300a;
const int kApiStatusFailure64 = (int)0x80040064;
const int kApiStatusFailure65 = (int)0x80040065;
const int kApiStatusFailure6a = (int)0x8004006a;
const int kApiStatusFailure72 = (int)0x80040072;
const int kBrowseRecordAddedStatusBufferSize = 128;
const unsigned int kBrowseRecordAddedFailureMessageId = 0x300d;
const unsigned int kBrowseRecordAddedOpenMessageId = 0x300e;
const unsigned int kBrowseRecordAddedClosedMessageId = 0x300f;
const int kWestwoodOnlineUpgradeBrowseRecordListId = 1136;
const int kWestwoodOnlineUpgradeGameButtonId = 1174;
const int kBrowseSessionResolvedStatusBufferSize = 128;
const unsigned int kBrowseSessionResolvedFailurePrefixMessageId = 0x3011;
const unsigned int kBrowseSessionResolvedFailure6cMessageId = 0x3012;
const unsigned int kBrowseSessionResolvedFailure70MessageId = 0x3013;
const unsigned int kBrowseSessionResolvedFailure72MessageId = 0x300a;
const unsigned int kBrowseSessionResolvedFailure71MessageId = 0x3014;
const unsigned int kBrowseSessionResolvedFailure6eMessageId = 0x3010;
const unsigned int kBrowseSessionResolvedStatusMessageId = 0x3015;
const unsigned int kSessionQueryFinishedStatusMessageId = 0x3016;
const unsigned int kSessionListEnumeratedReadyMessageId = 0x3017;
const unsigned int kSessionListEnumeratedClosedMessageId = 0x3018;
const unsigned int kSessionListEnumeratedPendingMessageId = 0x3019;
const int kBrowseSessionResolvedFailure6c = (int)0x8004006c;
const int kBrowseSessionResolvedFailure70 = (int)0x80040070;
const int kBrowseSessionResolvedFailure72 = (int)0x80040072;
const int kBrowseSessionResolvedFailure71 = (int)0x80040071;
const int kBrowseSessionResolvedFailure6e = (int)0x8004006e;
const int kSessionRequestRefreshCacheFlag = 0x8000;
const int kSessionRequestSkipDetailsFlag = 1;
const int kSessionListEnumeratedResultTextBufferSize = 256;
const int kSessionRequestStatusBufferSize = 128;
const unsigned int kSessionRequestStatus301BMessageId = 0x301b;
const unsigned int kSessionRequestStatus301CMessageId = 0x301c;
const unsigned int kSessionRequestStatus301DMessageId = 0x301d;
const unsigned int kConnectStatusDefaultMessageId = 0x301e;
const unsigned int kConnectStatusCode40134MessageId = 0x301f;
const unsigned int kConnectStatusCode40133MessageId = 0x3020;
const unsigned int kConnectStatusFallbackMessageId = 0x3021;
const int kConnectStatusCode40134 = 0x40134;
const int kConnectStatusCode40133 = 0x40133;
const unsigned int kBrowseRecordStatusNamedMessageId = 0x3022;
const unsigned int kBrowseRecordStatusCode40131MessageId = 0x3023;
const unsigned int kBrowseRecordStatusCode40130MessageId = 0x3020;
const unsigned int kBrowseRecordStatusCode40132MessageId = 0x3024;
const unsigned int kBrowseRecordStatusFallbackMessageId = 0x3025;
const int kBrowseRecordStatusCode40131 = 0x40131;
const int kBrowseRecordStatusCode40130 = 0x40130;
const int kBrowseRecordStatusCode40132 = 0x40132;
const unsigned int kValueStatus3026MessageId = 0x3026;
const unsigned int kTimeStatus302AMessageId = 0x302a;
const unsigned int kValueStatus302BMessageId = 0x302b;
const unsigned int kValueStatus302CMessageId = 0x302c;
const int kSessionResultRowTextBufferSize = 40;
const char kSessionResultReadySuffix[] = " +";
const char kSessionResultPendingSuffix[] = " *";
const unsigned int kSessionLaunchResultResetMessageId = 0x302d;
const unsigned int kSessionLaunchResultRemoveMessageId = 0x302e;
const int kNetworkStatusConnectError = (int)0x80040069;
const int kNetworkStatusNetworkDown = (int)0x80040067;
const int kNetworkStatusLookupFailed = (int)0x80040068;
const int kNetworkStatusTimeout = (int)0x8004006a;
const int kNetworkStatusConnecting = 0x4012c;
const int kNetworkStatusConnected = 0x4012d;
const int kNetworkStatusDisconnecting = 0x4012e;
const int kNetworkStatusDisconnected = 0x4012f;
const char kNetworkStatusDebugFormat[] = "Net Status = %s (%d)\n";
const char kNetworkStatusConnectErrorText[] = "CHAT_E_CON_ERROR";
const char kNetworkStatusNetworkDownText[] = "CHAT_E_CON_NETDOWN";
const char kNetworkStatusLookupFailedText[] = "CHAT_E_CON_LOOKUP_FAILED";
const char kNetworkStatusTimeoutText[] = "CHAT_E_TIMEOUT";
const char kNetworkStatusConnectingText[] = "CHAT_S_CON_CONNECTING";
const char kNetworkStatusConnectedText[] = "CHAT_S_CON_CONNECTED";
const char kNetworkStatusDisconnectingText[] = "CHAT_S_CON_DISCONNECTING";
const char kNetworkStatusDisconnectedText[] = "CHAT_S_CON_DISCONNECTED";
const char kNetworkStatusUnknownText[] = "CHAT_S_CON_UNKNOWN";
const char kSpaceDelimiter[] = " ";
const char kBrowseSessionResolvedStatusCodeFmt[] = "%s %x";
const int kBrowseRecordListRowTextBufferSize = 256;
const unsigned int kBrowseRecordListOpenMessageId = 0x3027;
const unsigned int kBrowseRecordListClosedMessageId = 0x3028;
const int kBrowseRecordLatencyUnknown = -1;
const int kBrowseRecordLatencyStep0Max = 250;
const int kBrowseRecordLatencyStep1Max = 500;
const int kBrowseRecordLatencyStep2Max = 750;
const int kBrowseRecordLatencyStep3Max = 1000;
const int kBrowseRecordLatencyStep4Max = 1250;
const int kBrowseRecordLatencyStep5Max = 1500;
const int kBrowseRecordLatencyStep6Max = 1750;
const char kBrowseRecordLatencyUnknownText[] = "??";
const char kBrowseRecordLatencyStep0Text[] = " | ";
const char kBrowseRecordLatencyStep1Text[] = " ||| ";
const char kBrowseRecordLatencyStep2Text[] = " ||||| ";
const char kBrowseRecordLatencyStep3Text[] = " ||||||| ";
const char kBrowseRecordLatencyStep4Text[] = " ||||||||| ";
const char kBrowseRecordLatencyStep5Text[] = " ||||||||||| ";
const char kBrowseRecordLatencyStep6Text[] = " ||||||||||||| ";
const char kBrowseRecordLatencyStep7Text[] = " ||||||||||||||| ";

struct IWestwoodOnlineUpgradeApiCallbacks : IUnknown {
    virtual void STDMETHODCALLTYPE Reserved0c() = 0;
    virtual void STDMETHODCALLTYPE Reserved10() = 0;
    virtual void STDMETHODCALLTYPE Reserved14() = 0;
    virtual void STDMETHODCALLTYPE RequestListMode(
        int listMode,
        int enabled
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved1c() = 0;
    virtual void STDMETHODCALLTYPE Reserved20() = 0;
    virtual void STDMETHODCALLTYPE Reserved24() = 0;
    virtual void STDMETHODCALLTYPE CancelPendingSessionFlow() = 0;
    virtual void STDMETHODCALLTYPE Reserved2c() = 0;
    virtual void STDMETHODCALLTYPE Reserved30() = 0;
    virtual void STDMETHODCALLTYPE Disconnect() = 0;
    virtual void STDMETHODCALLTYPE Reserved38() = 0;
    virtual void STDMETHODCALLTYPE Reserved3c() = 0;
    virtual void STDMETHODCALLTYPE Reserved40() = 0;
    virtual void STDMETHODCALLTYPE Reserved44() = 0;
    virtual void STDMETHODCALLTYPE Reserved48() = 0;
    virtual void STDMETHODCALLTYPE Reserved4c() = 0;
    virtual void STDMETHODCALLTYPE Reserved50() = 0;
    virtual void STDMETHODCALLTYPE Reserved54() = 0;
    virtual void STDMETHODCALLTYPE RequestSessionDetails(
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved5c() = 0;
    virtual void STDMETHODCALLTYPE Reserved60() = 0;
    virtual void STDMETHODCALLTYPE Reserved64() = 0;
    virtual void STDMETHODCALLTYPE Reserved68() = 0;
    virtual void STDMETHODCALLTYPE Reserved6c() = 0;
    virtual void STDMETHODCALLTYPE Reserved70() = 0;
    virtual void STDMETHODCALLTYPE SetQueryMode(
        int listMode
    ) = 0;
    virtual void STDMETHODCALLTYPE Reserved78() = 0;
    virtual void STDMETHODCALLTYPE Reserved7c() = 0;
    virtual void STDMETHODCALLTYPE Reserved80() = 0;
    virtual void STDMETHODCALLTYPE Reserved84() = 0;
    virtual void STDMETHODCALLTYPE Reserved88() = 0;
    virtual void STDMETHODCALLTYPE Reserved8c() = 0;
    virtual void STDMETHODCALLTYPE Reserved90() = 0;
    virtual void STDMETHODCALLTYPE Reserved94() = 0;
    virtual void STDMETHODCALLTYPE GetQueryResultCount(
        int *outCount
    ) = 0;
};

// Source-faithful helper recovered from address-backed callers in this source file.
IWestwoodOnlineUpgradeApiCallbacks *GetCallbackApiComObject() {
    return (IWestwoodOnlineUpgradeApiCallbacks *)g_pWestwoodOnlineUpgradeApi;
}
} // namespace

extern "C" LONG g_WestwoodOnlineUpgradeEventSinkLiveCount = 0;

// Recovered interface map used by 0x441660. BN data at 0x4d1ba0 contains
// {IID_WestwoodOnlineUpgradeApiEventSink, offset 0, direct} followed by end.
const zCom::InterfaceMapEntry g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap[2] = {
    {&g_WestwoodOnlineUpgradeApiEventSink_IID, 0, zCom::ZCOM_INTERFACE_MAP_DIRECT},
    {0, 0, zCom::ZCOM_INTERFACE_MAP_END},
};

/**
 * Reimplements 0x43f610: WestwoodOnlineUpgradeApiEventSink::CreateInstance.
 * Purpose: Allocates and initializes the Westwood API event sink for COM callbacks.
 */
HRESULT __stdcall WestwoodOnlineUpgradeApiEventSink::CreateInstance(
    WestwoodOnlineUpgradeApiEventSink **outSink
) {
    HRESULT result = E_OUTOFMEMORY;
    WestwoodOnlineUpgradeApiEventSink *eventSink = new WestwoodOnlineUpgradeApiEventSink;

    if (eventSink != 0) {
        eventSink->m_refCountAndLock.Init();
        InterlockedIncrement(&g_WestwoodOnlineUpgradeEventSinkLiveCount);
        result = S_OK;
    }

    *outSink = eventSink;
    return result;
}

/**
 * Reimplements 0x441660: WestwoodOnlineUpgradeApiEventSink::QueryInterface.
 * Purpose: Resolves the API event sink interfaces through its recovered interface map.
 */
HRESULT STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::QueryInterface(
    REFIID iid,
    void **outInterface
) {
    return WestwoodOnlineUpgradeApiEventSink::QueryInterface(
        this,
        iid,
        outInterface
    );
}

// Source-faithful helper recovered from address-backed callers in this source file.
HRESULT __stdcall WestwoodOnlineUpgradeApiEventSink::QueryInterface(
    WestwoodOnlineUpgradeApiEventSink *self,
    REFIID iid,
    void **outInterface
) {
    return zCom::QueryInterfaceFromInterfaceMap(
        self,
        g_WestwoodOnlineUpgradeApiEventSink_InterfaceMap,
        &iid,
        outInterface
    );
}

// Source-faithful helper recovered from address-backed callers in this source file.
ULONG STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::AddRef() {
    return (ULONG)InterlockedIncrement(&m_refCountAndLock.refCount);
}

// Source-faithful helper recovered from address-backed callers in this source file.
ULONG STDMETHODCALLTYPE WestwoodOnlineUpgradeApiEventSink::Release() {
    return WestwoodOnlineUpgradeApiEventSink::Release(this);
}

/**
 * Reimplements 0x441620: WestwoodOnlineUpgradeApiEventSink::Release.
 * Purpose: Decrements the COM reference count and destroys the API sink on final release.
 */
ULONG __stdcall WestwoodOnlineUpgradeApiEventSink::Release(
    WestwoodOnlineUpgradeApiEventSink *self
) {
    ULONG refCount;

    refCount = (ULONG)InterlockedDecrement(&self->m_refCountAndLock.refCount);
    if (refCount == 0 && self != 0) {
        self->Destructor();
        delete self;
    }

    return refCount;
}

/**
 * Reimplements 0x441680: WestwoodOnlineUpgradeApiEventSink::Destructor.
 * Purpose: Tears down the embedded lock and decrements the live Westwood event-sink count.
 */
void WestwoodOnlineUpgradeApiEventSink::Destructor() {
    m_refCountAndLock.refCount = 1;
    InterlockedDecrement(&g_WestwoodOnlineUpgradeEventSinkLiveCount);
    DeleteCriticalSection(&m_refCountAndLock.lock);
}

/**
 * Reimplements 0x43f830: WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult.
 * Purpose: Handles patch-download readiness results and opens the download-ready dialog.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult(
    void *,
    int resultCode,
    WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList
) {
    char dialogCaption[128];
    char dialogMessage[128];

    if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0) {
        return 0;
    }

    if (resultCode < 0) {
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
    }

    if (downloadReadyList == 0) {
        return 0;
    }

    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 0;
    strcpy(
        dialogCaption,
        zLoc::GetMessageString(0x3001)
    );
    strcpy(
        dialogMessage,
        zLoc::GetMessageString(0x3002)
    );
    if (((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->MessageBoxA(
                dialogMessage,
                dialogCaption,
                MB_YESNO | MB_ICONQUESTION
            ) != IDYES) {
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
        g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    } else if (WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(downloadReadyList) != 0) {
        strcpy(
            dialogCaption,
            zLoc::GetMessageString(0x3001)
        );
        strcpy(
            dialogMessage,
            zLoc::GetMessageString(0x3046)
        );
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->MessageBoxA(
                dialogMessage,
                dialogCaption,
                MB_ICONEXCLAMATION
            );
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
        g_pWestwoodOnlineUpgradeDialog->OnDestroy();
        ExitProcess(0);
    }

    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 1;
    return 0;
}

/**
 * Reimplements 0x43fa70: WestwoodOnlineUpgradeApiEventSink::OnServerError.
 * Purpose: Displays a Westwood Online server-error dialog for a received error string.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnServerError(
    void *,
    int,
    const char *errorText
) {
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->MessageBoxA(
            errorText,
            kWestwoodOnlineUpgradeServerErrorTitle,
            MB_ICONHAND
        );
    return 0;
}

/**
 * Reimplements 0x43fa90: WestwoodOnlineUpgradeApiEventSink::OnApiStatus.
 * Purpose: Formats API status results and appends them to the upgrade dialog result list.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnApiStatus(
    void *,
    int statusCode,
    const char *statusText
) {
    int resultCount;
    char failureCaption[kApiStatusTextBufferSize];
    char failureMessage[kApiStatusTextBufferSize];
    char resultCountStatusText[kApiStatusTextBufferSize];
    int failureMessageId;
    UINT messageBoxFlags;
    char *statusLine;
    IWestwoodOnlineUpgradeApiCallbacks *api;

    Time::Reset();

    if (statusCode == 0) {
        statusLine = strtok(
            _strdup(statusText),
            kApiStatusLineDelimiter
        );
        while (statusLine != 0) {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusLine);
            statusLine = strtok(
                0,
                kApiStatusLineDelimiter
            );
        }

        SetEvent(g_WestwoodOnlineUpgradeStatusTextEvent);
        // Retail frees the final strtok result, which is NULL here, leaking the
        // duplicated status text buffer.
        free(statusLine);

        api = GetCallbackApiComObject();
        api->SetQueryMode(kApiStatusActiveListMode);
        g_WestwoodOnlineUpgradeActiveListMode = kApiStatusActiveListMode;
        api->GetQueryResultCount(&resultCount);

        if (resultCount == 0) {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kApiStatusNoResultsMessageId)
            );
        } else if (resultCount == 1) {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kApiStatusOneResultMessageId)
            );
        } else {
            zLoc::FormatMessage(
                resultCountStatusText,
                kApiStatusTextBufferSize,
                kApiStatusMultipleResultsMessageId,
                resultCount
            );
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(resultCountStatusText);
        }

        return 0;
    }

    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)->DestroyWindow();
    strcpy(
        failureCaption,
        zLoc::GetMessageString(kApiStatusFailureCaptionMessageId)
    );
    messageBoxFlags = MB_ICONEXCLAMATION;

    if (statusCode == kApiStatusFailure64) {
        failureMessageId = kApiStatusFailure64MessageId;
    } else if (statusCode == kApiStatusFailure65) {
        failureMessageId = kApiStatusFailure65MessageId;
    } else if (statusCode == kApiStatusFailure6a) {
        failureMessageId = kApiStatusFailure6aMessageId;
    } else if (statusCode == kApiStatusFailure72) {
        failureMessageId = kApiStatusFailure72MessageId;
    } else {
        failureMessageId = kApiStatusFailureDefaultMessageId;
        messageBoxFlags = MB_ICONHAND;
    }

    strcpy(
        failureMessage,
        zLoc::GetMessageString(failureMessageId)
    );
    ((CWnd *)((unsigned int)g_RecoilApp.m_pMainWnd))
        ->MessageBoxA(
            failureMessage,
            failureCaption,
            messageBoxFlags
        );
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
    return 0;
}

/**
 * Reimplements 0x43fde0: WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived.
 * Purpose: Writes received status text into the Westwood Online status display.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived(
    void *,
    int status,
    const char *statusText
) {
    char *statusLine;

    if (status < 0) {
        return 0;
    }

    // Retail duplicates statusText for strtok and never frees the duplicate.
    statusLine = strtok(
        _strdup(statusText),
        kApiStatusLineDelimiter
    );
    while (statusLine != 0) {
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusLine);
        statusLine = strtok(
            0,
            kApiStatusLineDelimiter
        );
    }

    SetEvent(g_WestwoodOnlineUpgradeStatusTextEvent);
    return 0;
}

/**
 * Reimplements 0x43fe50: WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded.
 * Purpose: Appends a browse-record row and enables the game button when appropriate.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord
) {
    char statusText[kBrowseRecordAddedStatusBufferSize];
    IWestwoodOnlineUpgradeApiCallbacks *api;

    if (status < 0) {
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
            zLoc::GetMessageString(kBrowseRecordAddedFailureMessageId)
        );
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
        api = GetCallbackApiComObject();
        api->RequestListMode(
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        return 0;
    }

    const unsigned int messageId = browseRecord->m_recordFlags != 0
                                       ? kBrowseRecordAddedClosedMessageId
                                       : kBrowseRecordAddedOpenMessageId;
    zLoc::FormatMessage(
        statusText,
        kBrowseRecordAddedStatusBufferSize,
        messageId,
        browseRecord->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);

    if (browseRecord->m_recordFlags != 0) {
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(1);
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SetDlgItemTextA(
                kWestwoodOnlineUpgradeGameButtonId,
                browseRecord->m_sessionName
            );
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord = *browseRecord;
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->SendDlgItemMessageA(
            kWestwoodOnlineUpgradeBrowseRecordListId,
            LB_INSERTSTRING,
            (WPARAM)-1,
            (LPARAM)browseRecord->m_sessionName
        );
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    return 0;
}

/**
 * Reimplements 0x43ff80: WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved.
 * Purpose: Resolves browse-record/session details and updates session row status text.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
) {
    char statusText[kBrowseSessionResolvedStatusBufferSize];
    unsigned int failureMessageId;
    IWestwoodOnlineUpgradeApiCallbacks *api;

    if (status < 0) {
        strcpy(
            statusText,
            zLoc::GetMessageString(kBrowseSessionResolvedFailurePrefixMessageId)
        );
        strcat(
            statusText,
            kSpaceDelimiter
        );

        if (status == kBrowseSessionResolvedFailure6c) {
            failureMessageId = kBrowseSessionResolvedFailure6cMessageId;
        } else if (status == kBrowseSessionResolvedFailure70) {
            failureMessageId = kBrowseSessionResolvedFailure70MessageId;
        } else if (status == kBrowseSessionResolvedFailure72) {
            failureMessageId = kBrowseSessionResolvedFailure72MessageId;
        } else if (status == kBrowseSessionResolvedFailure71) {
            failureMessageId = kBrowseSessionResolvedFailure71MessageId;
        } else if (status == kBrowseSessionResolvedFailure6e) {
            failureMessageId = kBrowseSessionResolvedFailure6eMessageId;
        } else {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                kBrowseSessionResolvedStatusCodeFmt,
                zLoc::GetMessageString(kBrowseSessionResolvedFailurePrefixMessageId),
                status
            );
            g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
            g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
            api = GetCallbackApiComObject();
            api->RequestListMode(
                g_WestwoodOnlineUpgradeActiveListMode,
                1
            );
            return 0;
        }

        strcat(
            statusText,
            zLoc::GetMessageString(failureMessageId)
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
        api = GetCallbackApiComObject();
        api->RequestListMode(
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        return 0;
    }

    if ((sessionRequest->m_rowFlags & kSessionRequestRefreshCacheFlag) != 0) {
        g_WestwoodOnlineUpgradeCachedBrowseRecord = *browseRecord;
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
    }

    if ((sessionRequest->m_rowFlags & kSessionRequestSkipDetailsFlag) == 0) {
        api = GetCallbackApiComObject();
        api->RequestListMode(
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        if (browseRecord->m_recordFlags != 0 &&
            g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag == 1) {
            g_pWestwoodOnlineUpgradeDialog->UpdateSessionListQueryFromControls();
        }

        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SetDlgItemTextA(
                kWestwoodOnlineUpgradeGameButtonId,
                browseRecord->m_sessionName
            );
        zLoc::FormatMessage(
            statusText,
            kBrowseSessionResolvedStatusBufferSize,
            kBrowseSessionResolvedStatusMessageId,
            sessionRequest->m_sessionName,
            browseRecord->m_sessionName
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeSessionResultsListId,
                LB_ADDSTRING,
                0,
                (LPARAM)sessionRequest->m_sessionName
            );
        ++g_WestwoodOnlineUpgradePendingSessionResultCount;
        g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(1);
        api = GetCallbackApiComObject();
        api->RequestSessionDetails(
            sessionRequest
        );
    }

    return 0;
}

/**
 * Reimplements 0x4401d0: WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished.
 * Purpose: Finalizes session-query state and requests detail data for pending sessions.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
) {
    char statusText[kBrowseSessionResolvedStatusBufferSize];
    LRESULT sessionIndex;
    IWestwoodOnlineUpgradeApiCallbacks *api;

    if (status < 0) {
        api = GetCallbackApiComObject();
        api->RequestListMode(
            kApiStatusActiveListMode,
            1
        );
        api = GetCallbackApiComObject();
        api->CancelPendingSessionFlow();
        return 0;
    }

    zLoc::FormatMessage(
        statusText,
        kBrowseSessionResolvedStatusBufferSize,
        kSessionQueryFinishedStatusMessageId,
        sessionRequest->m_sessionName,
        browseRecord->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);

    sessionIndex = ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
                       ->SendDlgItemMessageA(
                           kWestwoodOnlineUpgradeSessionResultsListId,
                           LB_FINDSTRINGEXACT,
                           (WPARAM)-1,
                           (LPARAM)sessionRequest->m_sessionName
                       );
    if (sessionIndex != LB_ERR) {
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeSessionResultsListId,
                LB_DELETESTRING,
                (WPARAM)sessionIndex,
                0
            );
        --g_WestwoodOnlineUpgradePendingSessionResultCount;
    }

    if ((sessionRequest->m_rowFlags & kSessionRequestSkipDetailsFlag) != 0 &&
        (sessionRequest->m_rowFlags & kSessionRequestRefreshCacheFlag) == 0) {
        g_pWestwoodOnlineUpgradeDialog->AppendConnectStatusAndRefreshList(
            browseRecord->m_sessionName
        );
    }

    api = GetCallbackApiComObject();
    api->RequestListMode(
        g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
    return 0;
}

/**
 * Reimplements 0x441480: WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult.
 * Purpose: Handles launch success/failure state for a selected Westwood session.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionNode,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
) {
    char statusText[kSessionRequestStatusBufferSize];
    LRESULT sessionIndex;
    IWestwoodOnlineUpgradeApiCallbacks *api;

    if (status < 0) {
        api = GetCallbackApiComObject();
        api->CancelPendingSessionFlow();
        return 0;
    }

    if ((sessionNode->m_rowFlags & kSessionRequestRefreshCacheFlag) != 0) {
        zLoc::FormatMessage(
            statusText,
            kSessionRequestStatusBufferSize,
            kSessionLaunchResultResetMessageId,
            browseRecord->m_sessionName,
            sessionRequest->m_sessionName
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
        api = GetCallbackApiComObject();
        g_WestwoodOnlineUpgradePendingSessionResultCount = 0;
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
        api->RequestListMode(
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeSessionResultsListId,
                LB_RESETCONTENT,
                0,
                0
            );
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
        g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(0);
        return 0;
    }

    zLoc::FormatMessage(
        statusText,
        kSessionRequestStatusBufferSize,
        kSessionLaunchResultRemoveMessageId,
        sessionNode->m_sessionName,
        browseRecord->m_sessionName,
        sessionRequest->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
    sessionIndex = ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
                       ->SendDlgItemMessageA(
                           kWestwoodOnlineUpgradeSessionResultsListId,
                           LB_FINDSTRINGEXACT,
                           (WPARAM)-1,
                           (LPARAM)sessionNode->m_sessionName
                       );
    if (sessionIndex != LB_ERR) {
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeSessionResultsListId,
                LB_DELETESTRING,
                (WPARAM)sessionIndex,
                0
            );
        --g_WestwoodOnlineUpgradePendingSessionResultCount;
    }

    return 0;
}

/**
 * Reimplements 0x4402c0: WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated.
 * Purpose: Enumerates session records and appends visible rows for the upgrade dialog.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionList
) {
    int deferAutoConnect;
    int rowFlags;
    char sessionResultText[kSessionListEnumeratedResultTextBufferSize];
    LRESULT sessionIndex;
    WestwoodOnlineUpgradeSessionRequest *sessionNode;

    if (status < 0) {
        return 0;
    }

    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->SendDlgItemMessageA(
            kWestwoodOnlineUpgradeSessionResultsListId,
            LB_RESETCONTENT,
            0,
            0
        );
    g_WestwoodOnlineUpgradePendingSessionResultCount = 0;
    deferAutoConnect = 0;
    sessionNode = sessionList;

    while (sessionNode != 0) {
        rowFlags = sessionNode->m_rowFlags;
        ++g_WestwoodOnlineUpgradePendingSessionResultCount;
        strcpy(
            sessionResultText,
            sessionNode->m_sessionName
        );

        if ((rowFlags & kSessionRequestRefreshCacheFlag) != 0) {
            if ((rowFlags & kSessionRequestSkipDetailsFlag) != 0) {
                strcat(
                    sessionResultText,
                    zLoc::GetMessageString(kSessionListEnumeratedReadyMessageId)
                );
                deferAutoConnect = 1;
                if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag == 0) {
                    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
                    g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(1);
                }
            } else {
                strcat(
                    sessionResultText,
                    zLoc::GetMessageString(kSessionListEnumeratedClosedMessageId)
                );
                g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
                g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(1);
            }
        } else if ((rowFlags & kSessionRequestSkipDetailsFlag) != 0) {
            strcat(
                sessionResultText,
                zLoc::GetMessageString(kSessionListEnumeratedPendingMessageId)
            );
            deferAutoConnect = 1;
        }

        sessionIndex = ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
                           ->SendDlgItemMessageA(
                               kWestwoodOnlineUpgradeSessionResultsListId,
                               LB_ADDSTRING,
                               0,
                               (LPARAM)sessionResultText
                           );
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeSessionResultsListId,
                LB_SETITEMDATA,
                (WPARAM)sessionIndex,
                rowFlags
            );
        sessionNode = sessionNode->m_next;
    }

    if (deferAutoConnect == 0) {
        g_pWestwoodOnlineUpgradeDialog->AppendConnectStatusAndRefreshList(
            browseRecord->m_sessionName
        );
    }

    return 0;
}

/**
 * Reimplements 0x4404c0: WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession.
 * Purpose: Applies selected-session connection details and starts the network launch path.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession(
    void *,
    int status,
    int,
    WestwoodOnlineUpgradeSessionRequest *selectedSessionList,
    int
) {
    int launched;
    int showConnectionFailureMessage;
    char hostAddressText[20];
    zNetworkSessionDescStatusFields statusFields;
    char failureCaptionText[128];
    char failureMessageText[128];
    WestwoodOnlineUpgradeSessionRequest *selectedSessionNode;
    WestwoodOnlineUpgradeDialog *dialog;
    IWestwoodOnlineUpgradeApiCallbacks *api;

    if (status < 0) {
        return 0;
    }

    launched = 0;
    showConnectionFailureMessage = 0;
    selectedSessionNode = selectedSessionList;

    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)->UpdateData(TRUE);
    while (selectedSessionNode != 0 &&
           (selectedSessionNode->m_rowFlags & kSessionRequestSkipDetailsFlag) == 0) {
        selectedSessionNode = selectedSessionNode->m_next;
    }

    if (selectedSessionNode != 0) {
        zNetwork::InitSessionRuntime(&g_zNetwork_RecoilAppGuid);
        Net::FormatIpv4Address(
            hostAddressText,
            selectedSessionNode->m_hostIpv4Packed
        );

        if (zNetworkDPlay::SelectTcpIpProviderAndEnumSessions(
                hostAddressText,
                g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag
            ) == 0) {
            showConnectionFailureMessage = 1;
        } else if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0) {
            dialog = g_pWestwoodOnlineUpgradeDialog;
            statusFields.statusFlags = 0;
            if (dialog->m_queryStatusFlagBit0 != 0) {
                statusFields.statusFlags = 1;
            }
            if (dialog->m_queryStatusFlagBit1 != 0) {
                statusFields.statusFlags |= 2;
            }

            statusFields.eventCode =
                (int)SendMessageA(
                    dialog->m_sessionModeCombo.m_hWnd,
                    CB_GETCURSEL,
                    0,
                    0
                ) + 1;
            statusFields.valueOrTime = dialog->m_queryValueOrTime;
            statusFields.auxParam = dialog->m_queryAuxParam;
            statusFields.maxPlayers = dialog->m_queryMaxPlayers;
            strcpy(
                statusFields.sessionNameBuf,
                (const char *)dialog->m_sessionName
            );

            if (zNetwork_DPlay::CreateSessionFromStatusFields(&statusFields) != 0) {
                zOpt::SetNetworkEnabled(1);
                CString selectedPlayerName;
                g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName(&selectedPlayerName);
                zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(
                    (char *)(const char *)selectedPlayerName
                );
                launched = 1;
            }
        } else {
            statusFields.selectedSessionIndex = 0;
            if (zNetworkDPlay::OpenSelectedSessionAndReadStatusFields(&statusFields) == 0) {
                showConnectionFailureMessage = 1;
            } else {
                zOpt::SetNetworkEnabled(1);
                {
                    CString selectedPlayerName;
                    g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName(
                        &selectedPlayerName
                    );
                    zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(
                        (char *)(const char *)selectedPlayerName
                    );
                }

                {
                    CString selectedPlayerName;
                    g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName(
                        &selectedPlayerName
                    );
                    zOpt::SetPlayerName((const char *)selectedPlayerName);
                }
                launched = 1;
            }
        }

        if (launched != 0) {
            union {
                float value;
                int raw;
            } timerSeconds = {(float)statusFields.valueOrTime * 60.0f};
            GameNet::SetStatusBitsFromFlags(statusFields.statusFlags);
            g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
                timerSeconds.raw,
                statusFields.auxParam
            );
            g_WestwoodOnlineUpgradeSelectedMissionIndex = statusFields.eventCode;
        }
    }

    if (showConnectionFailureMessage != 0) {
        strcpy(
            failureCaptionText,
            zLoc::GetMessageString(kApiStatusFailureCaptionMessageId)
        );
        strcpy(
            failureMessageText,
            zLoc::GetMessageString(kApiStatusFailureDefaultMessageId)
        );
        MessageBeep(MB_ICONHAND);
        ((CWnd *)g_RecoilApp.m_pMainWnd)
            ->MessageBoxA(
                failureMessageText,
                failureCaptionText,
                MB_ICONHAND
            );
    }

    api = GetCallbackApiComObject();
    api->Disconnect();
    return 0;
}

/**
 * Reimplements 0x440a30: WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0.
 * Purpose: Parses encoded query data for one Westwood Online session result variant.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0(
    void *,
    int status,
    int,
    char *encodedQuery
) {
    WestwoodOnlineUpgradeDialog *dialog;

    if (status < 0) {
        return 0;
    }

    CString encodedQueryText(encodedQuery);
    CString queryFieldText;
    dialog = g_pWestwoodOnlineUpgradeDialog;
    queryFieldText = encodedQueryText.Mid(
        0,
        1
    );
    SendMessageA(
        dialog->m_sessionModeCombo.m_hWnd,
        CB_SETCURSEL,
        atoi((const char *)queryFieldText),
        0
    );

    queryFieldText = encodedQueryText.Mid(
        1,
        4
    );
    dialog->m_queryValueOrTime = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        5,
        4
    );
    dialog->m_queryAuxParam = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        9,
        1
    );
    dialog->m_queryMaxPlayers = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        10,
        1
    );
    dialog->m_queryStatusFlagBit0 = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        11,
        1
    );
    dialog->m_queryStatusFlagBit1 = atoi((const char *)queryFieldText);

    ((CWnd *)dialog)->UpdateData(FALSE);
    return 0;
}

/**
 * Reimplements 0x4407e0: WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1.
 * Purpose: Parses encoded query data for the alternate session result callback shape.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1(
    void *,
    int status,
    int,
    int,
    char *encodedQuery
) {
    WestwoodOnlineUpgradeDialog *dialog;

    if (status < 0) {
        return 0;
    }

    CString encodedQueryText(encodedQuery);
    CString queryFieldText;
    dialog = g_pWestwoodOnlineUpgradeDialog;
    queryFieldText = encodedQueryText.Mid(
        0,
        1
    );
    SendMessageA(
        dialog->m_sessionModeCombo.m_hWnd,
        CB_SETCURSEL,
        atoi((const char *)queryFieldText),
        0
    );

    queryFieldText = encodedQueryText.Mid(
        1,
        4
    );
    dialog->m_queryValueOrTime = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        5,
        4
    );
    dialog->m_queryAuxParam = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        9,
        1
    );
    dialog->m_queryMaxPlayers = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        10,
        1
    );
    dialog->m_queryStatusFlagBit0 = atoi((const char *)queryFieldText);

    queryFieldText = encodedQueryText.Mid(
        11,
        1
    );
    dialog->m_queryStatusFlagBit1 = atoi((const char *)queryFieldText);

    ((CWnd *)dialog)->UpdateData(FALSE);
    return 0;
}

/**
 * Reimplements 0x440c80: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301B.
 * Purpose: Appends localized 0x301b status text for a session request.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301B(
    void *,
    int status,
    int,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    const char *statusText
) {
    char statusMessageText[kSessionRequestStatusBufferSize];

    if (status < 0) {
        return 0;
    }

    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        kSessionRequestStatus301BMessageId,
        sessionRequest->m_sessionName,
        statusText
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x440ce0: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C.
 * Purpose: Appends localized 0x301c status text for a session request.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C(
    void *,
    int status,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    const char *statusText
) {
    char statusMessageText[kSessionRequestStatusBufferSize];

    if (status < 0) {
        return 0;
    }

    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        kSessionRequestStatus301CMessageId,
        sessionRequest->m_sessionName,
        statusText
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x4411c0: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt0.
 * Purpose: Appends the first numeric 0x301c status variant for a session request.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt0(
    void *,
    int status,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    int value
) {
    if (status < 0) {
        return 0;
    }

    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
        zLoc::GetMessageString(kSessionRequestStatus301CMessageId),
        sessionRequest->m_sessionName,
        value
    );
    return 0;
}

/**
 * Reimplements 0x441200: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt1.
 * Purpose: Appends the second numeric 0x301c status variant for a session request.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt1(
    void *,
    int status,
    int,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    int value
) {
    if (status < 0) {
        return 0;
    }

    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
        zLoc::GetMessageString(kSessionRequestStatus301CMessageId),
        sessionRequest->m_sessionName,
        value
    );
    return 0;
}

/**
 * Reimplements 0x441240: WestwoodOnlineUpgradeApiEventSink::CallbackNoOp0.
 * Purpose: Handles an unused four-argument API event callback with a zero result.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::CallbackNoOp0(
    void *,
    int,
    int,
    int
) {
    return 0;
}

/**
 * Reimplements 0x441250: WestwoodOnlineUpgradeApiEventSink::CallbackNoOp1.
 * Purpose: Handles an unused three-argument API event callback with a zero result.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::CallbackNoOp1(
    void *,
    int,
    int
) {
    return 0;
}

/**
 * Reimplements 0x441260: WestwoodOnlineUpgradeApiEventSink::AppendTimeStatus302A.
 * Purpose: Converts a session timestamp to localized 0x302a status text.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendTimeStatus302A(
    void *,
    int status,
    long unixTime
) {
    char statusMessageText[kSessionRequestStatusBufferSize];
    time_t timeValue;

    if (status == 0) {
        timeValue = (time_t)unixTime;
        zLoc::FormatMessage(
            statusMessageText,
            kSessionRequestStatusBufferSize,
            kTimeStatus302AMessageId,
            ctime(&timeValue)
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    }

    return 0;
}

/**
 * Reimplements 0x4412c0: WestwoodOnlineUpgradeApiEventSink::AppendValueStatus302B_302C.
 * Purpose: Appends one of the localized value status messages for a numeric field.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendValueStatus302B_302C(
    void *,
    int,
    int value,
    int usePrimaryMessage
) {
    char statusMessageText[kSessionRequestStatusBufferSize];
    unsigned int messageId;

    messageId = usePrimaryMessage != 0 ? kValueStatus302BMessageId : kValueStatus302CMessageId;
    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        messageId,
        value
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x441350: WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags.
 * Purpose: Updates the visible suffix flags for a matching session result row.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags(
    void *,
    int,
    const char *sessionName,
    int flags,
    int
) {
    char rowText[kSessionResultRowTextBufferSize];
    LRESULT sessionIndex;

    sessionIndex = g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeSessionResultsListId,
        LB_FINDSTRING,
        (WPARAM)-1,
        (LPARAM)sessionName
    );
    if (sessionIndex == LB_ERR) {
        return 0;
    }

    g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeSessionResultsListId,
        LB_GETITEMDATA,
        (WPARAM)sessionIndex,
        0
    );
    g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeSessionResultsListId,
        LB_SETITEMDATA,
        (WPARAM)sessionIndex,
        flags
    );
    g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeSessionResultsListId,
        LB_DELETESTRING,
        (WPARAM)sessionIndex,
        0
    );

    strcpy(
        rowText,
        sessionName
    );
    if ((flags & 1) != 0) {
        strcat(
            rowText,
            kSessionResultReadySuffix
        );
    }
    strcat(
        rowText,
        kSessionResultPendingSuffix
    );
    g_pWestwoodOnlineUpgradeDialog->SendDlgItemMessageA(
        kWestwoodOnlineUpgradeSessionResultsListId,
        LB_ADDSTRING,
        0,
        (LPARAM)rowText
    );

    return 0;
}

/**
 * Reimplements 0x440d40: WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301D.
 * Purpose: Appends localized 0x301d status text for a session request.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301D(
    void *,
    int,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest,
    const char *statusText
) {
    char statusMessageText[kSessionRequestStatusBufferSize];

    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        kSessionRequestStatus301DMessageId,
        sessionRequest->m_sessionName,
        statusText
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x440d90: WestwoodOnlineUpgradeApiEventSink::AppendConnectStatus301E_3021.
 * Purpose: Maps connection status codes to localized connect-status text.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendConnectStatus301E_3021(
    void *,
    int connectionStatusCode
) {
    unsigned int messageId;

    if (connectionStatusCode == 0) {
        messageId = kConnectStatusDefaultMessageId;
    } else if (connectionStatusCode == kConnectStatusCode40134) {
        messageId = kConnectStatusCode40134MessageId;
    } else if (connectionStatusCode == kConnectStatusCode40133) {
        messageId = kConnectStatusCode40133MessageId;
    } else {
        messageId = kConnectStatusFallbackMessageId;
    }

    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(zLoc::GetMessageString(messageId));
    return 0;
}

/**
 * Reimplements 0x440e10: WestwoodOnlineUpgradeApiEventSink::AppendBrowseRecordStatus3022_3025.
 * Purpose: Maps browse-record status codes to localized browse-status text.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendBrowseRecordStatus3022_3025(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord
) {
    unsigned int messageId;
    char statusMessageText[kSessionRequestStatusBufferSize];

    if (browseRecord != 0) {
        zLoc::FormatMessage(
            statusMessageText,
            kSessionRequestStatusBufferSize,
            kBrowseRecordStatusNamedMessageId,
            browseRecord->m_sessionName
        );
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
        return 0;
    }

    if (status == kBrowseRecordStatusCode40131) {
        messageId = kBrowseRecordStatusCode40131MessageId;
    } else if (status == kBrowseRecordStatusCode40130) {
        messageId = kBrowseRecordStatusCode40130MessageId;
    } else if (status == kBrowseRecordStatusCode40132) {
        messageId = kBrowseRecordStatusCode40132MessageId;
    } else {
        messageId = kBrowseRecordStatusFallbackMessageId;
    }

    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(zLoc::GetMessageString(messageId));
    return 0;
}

/**
 * Reimplements 0x440ef0: WestwoodOnlineUpgradeApiEventSink::AppendValueStatus3026.
 * Purpose: Appends localized 0x3026 value text for a session status field.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::AppendValueStatus3026(
    void *,
    int status,
    int value
) {
    char statusMessageText[kSessionRequestStatusBufferSize];

    if (status < 0) {
        return 0;
    }

    zLoc::FormatMessage(
        statusMessageText,
        kSessionRequestStatusBufferSize,
        kValueStatus3026MessageId,
        value
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusMessageText);
    return 0;
}

/**
 * Reimplements 0x440f40: WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged.
 * Purpose: Updates network status text and side effects for Westwood connection changes.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged(
    void *,
    int connectionStatusCode
) {
    const char *statusName;
    char debugStatusText[kSessionRequestStatusBufferSize];

    if (connectionStatusCode == kNetworkStatusConnectError) {
        statusName = kNetworkStatusConnectErrorText;
    } else if (connectionStatusCode == kNetworkStatusNetworkDown) {
        statusName = kNetworkStatusNetworkDownText;
    } else if (connectionStatusCode == kNetworkStatusLookupFailed) {
        statusName = kNetworkStatusLookupFailedText;
    } else if (connectionStatusCode == kNetworkStatusTimeout) {
        statusName = kNetworkStatusTimeoutText;
    } else if (connectionStatusCode == kNetworkStatusConnecting) {
        statusName = kNetworkStatusConnectingText;
    } else if (connectionStatusCode == kNetworkStatusConnected) {
        statusName = kNetworkStatusConnectedText;
    } else if (connectionStatusCode == kNetworkStatusDisconnecting) {
        statusName = kNetworkStatusDisconnectingText;
    } else if (connectionStatusCode == kNetworkStatusDisconnected) {
        statusName = kNetworkStatusDisconnectedText;
    } else {
        statusName = kNetworkStatusUnknownText;
    }

    sprintf(
        debugStatusText,
        kNetworkStatusDebugFormat,
        statusName,
        connectionStatusCode
    );
    zGame::ReturnOnlyStub();

    if (connectionStatusCode == kNetworkStatusDisconnected &&
        g_WestwoodOnlineUpgradeAbortFlag == 0) {
        g_pWestwoodOnlineUpgradeDialog->SetAbortAndClose();
        return 0;
    }

    if (connectionStatusCode == kNetworkStatusConnectError ||
        connectionStatusCode == kNetworkStatusNetworkDown ||
        connectionStatusCode == kNetworkStatusTimeout) {
        g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    }

    return 0;
}

/**
 * Reimplements 0x441040: WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordListReceived.
 * Purpose: Rebuilds the browse-record list display from a received record list.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordListReceived(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecordList
) {
    WestwoodOnlineUpgradeBrowseRecord *currentRecord;
    const char *latencyBarText;
    char rowText[kBrowseRecordListRowTextBufferSize];
    int latencyMs;
    int cacheIndex;

    g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = 0;
    if (status < 0) {
        return 0;
    }

    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->SendDlgItemMessageA(
            kWestwoodOnlineUpgradeBrowseRecordListId,
            LB_RESETCONTENT,
            0,
            0
        );

    currentRecord = browseRecordList;
    while (currentRecord != 0) {
        if (currentRecord->m_recordFlags == 0) {
            zLoc::FormatMessage(
                rowText,
                kBrowseRecordListRowTextBufferSize,
                kBrowseRecordListOpenMessageId,
                currentRecord->m_sessionName,
                currentRecord->m_displayMetric1
            );
        } else {
            latencyMs = currentRecord->m_latencyMs;
            if (latencyMs == kBrowseRecordLatencyUnknown) {
                latencyBarText = kBrowseRecordLatencyUnknownText;
            } else if (latencyMs <= kBrowseRecordLatencyStep0Max) {
                latencyBarText = kBrowseRecordLatencyStep0Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep1Max) {
                latencyBarText = kBrowseRecordLatencyStep1Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep2Max) {
                latencyBarText = kBrowseRecordLatencyStep2Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep3Max) {
                latencyBarText = kBrowseRecordLatencyStep3Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep4Max) {
                latencyBarText = kBrowseRecordLatencyStep4Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep5Max) {
                latencyBarText = kBrowseRecordLatencyStep5Text;
            } else if (latencyMs <= kBrowseRecordLatencyStep6Max) {
                latencyBarText = kBrowseRecordLatencyStep6Text;
            } else {
                latencyBarText = kBrowseRecordLatencyStep7Text;
            }

            zLoc::FormatMessage(
                rowText,
                kBrowseRecordListRowTextBufferSize,
                kBrowseRecordListClosedMessageId,
                currentRecord->m_sessionName,
                currentRecord->m_displayMetric1,
                currentRecord->m_displayMetric0,
                latencyBarText
            );
        }

        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeBrowseRecordListId,
                LB_INSERTSTRING,
                (WPARAM)-1,
                (LPARAM)rowText
            );
        cacheIndex = g_WestwoodOnlineUpgradeCachedBrowseRecordListCount;
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[cacheIndex] = *currentRecord;
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[cacheIndex].m_next = 0;
        g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = cacheIndex + 1;
        currentRecord = currentRecord->m_next;
    }

    return 0;
}

/**
 * Reimplements 0x43f9d0: WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved.
 * Purpose: Removes or updates pending-session UI state after a session request is removed.
 */
int __stdcall WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved(
    void *,
    int status,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
) {
    char statusText[kPendingSessionRemovedStatusBufferSize];
    LRESULT sessionIndex;

    if (status < 0) {
        return 0;
    }

    zLoc::FormatMessage(
        statusText,
        kPendingSessionRemovedStatusBufferSize,
        kPendingSessionRemovedStatusMessageId,
        sessionRequest->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);

    sessionIndex = ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
                       ->SendDlgItemMessageA(
                           kWestwoodOnlineUpgradeSessionResultsListId,
                           LB_FINDSTRINGEXACT,
                           (WPARAM)-1,
                           (LPARAM)sessionRequest->m_sessionName
                       );
    if (sessionIndex != LB_ERR) {
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeSessionResultsListId,
                LB_DELETESTRING,
                (WPARAM)sessionIndex,
                0
            );
        --g_WestwoodOnlineUpgradePendingSessionResultCount;
    }

    return 0;
}
