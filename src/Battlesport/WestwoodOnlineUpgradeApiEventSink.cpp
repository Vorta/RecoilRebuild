#include "Battlesport/WestwoodOnlineUpgradeApiEventSink.h"

#include "Battlesport/RecoilApp.h"
#include "Battlesport/WestwoodOnlineUpgradeApi.h"
#include "Battlesport/WestwoodOnlineUpgradeDialog.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/wwonline/upgrade_download.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <stdlib.h>
#include <string.h>

namespace
{
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
const char kSpaceDelimiter[] = " ";
const char kBrowseSessionResolvedStatusCodeFmt[] = "%s %x";

// Provider ABI shim for the unavailable Westwood Online upgrade API COM object.
// Only consumed slots are named; offset asserts keep the imported contract fixed.
struct WestwoodOnlineUpgradeApiCallbackVtable
{
    void *reserved000[6];
    void(STDMETHODCALLTYPE *RequestListMode)(
        IUnknown *self,
        int listMode,
        int enabled
    );
    void *reserved01c[3];
    void(STDMETHODCALLTYPE *CancelPendingSessionFlow)(IUnknown *self);
    void *reserved02c[11];
    void(STDMETHODCALLTYPE *RequestSessionDetails)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *sessionRequest
    );
    void *reserved05c[6];
    void(STDMETHODCALLTYPE *SetQueryMode)(IUnknown *self, int listMode);
    void *reserved078[8];
    void(STDMETHODCALLTYPE *GetQueryResultCount)(IUnknown *self, int *outCount);
};

struct WestwoodOnlineUpgradeApiCallbackComObject
{
    WestwoodOnlineUpgradeApiCallbackVtable *vftable;
};

RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiCallbackVtable,
                              RequestListMode) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiCallbackVtable,
                              CancelPendingSessionFlow) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiCallbackVtable,
                              RequestSessionDetails) == 0x58);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiCallbackVtable,
                              SetQueryMode) == 0x74);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeApiCallbackVtable,
                              GetQueryResultCount) == 0x98);

WestwoodOnlineUpgradeApiCallbackComObject *RECOIL_CDECL GetCallbackApiComObject()
{
    return (WestwoodOnlineUpgradeApiCallbackComObject *)g_pWestwoodOnlineUpgradeApi;
}
}

extern "C" LONG g_WestwoodOnlineUpgradeEventSinkLiveCount = 0;

// Vtable identity installed by 0x43f610. The individual COM sink slots remain
// reconstruction targets; this table intentionally carries no retail addresses.
WestwoodOnlineUpgradeApiEventSinkVtable g_WestwoodOnlineUpgradeApiEventSink_Vtbl = {0};

// Reimplements 0x43f610: WestwoodOnlineUpgradeApiEventSink::CreateInstance
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE HRESULT RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::CreateInstance(
    WestwoodOnlineUpgradeApiEventSink **outSink)
{
    HRESULT result = E_OUTOFMEMORY;
    WestwoodOnlineUpgradeApiEventSink *eventSink =
        (WestwoodOnlineUpgradeApiEventSink *)(::operator new(
            sizeof(WestwoodOnlineUpgradeApiEventSink)));

    if (eventSink != 0)
    {
        eventSink->m_refCountAndLock.Init();
        eventSink->m_vftable = &g_WestwoodOnlineUpgradeApiEventSink_Vtbl;
        InterlockedIncrement(&g_WestwoodOnlineUpgradeEventSinkLiveCount);
        result = S_OK;
    }

    *outSink = eventSink;
    return result;
}

// Reimplements 0x43f830: WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult(
    void *,
    int resultCode,
    WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList
)
{
    char dialogCaption[128];
    char dialogMessage[128];

    if (g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0)
    {
        return 0;
    }

    if (resultCode < 0)
    {
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
    }

    if (downloadReadyList == 0)
    {
        return 0;
    }

    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 0;
    strcpy(dialogCaption, zLoc::GetMessageString(0x3001));
    strcpy(dialogMessage, zLoc::GetMessageString(0x3002));
    if (((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->MessageBoxA(dialogMessage,
                          dialogCaption,
                          MB_YESNO | MB_ICONQUESTION) != IDYES)
    {
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
        g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    }
    else if (WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(downloadReadyList) != 0)
    {
        strcpy(dialogCaption, zLoc::GetMessageString(0x3001));
        strcpy(dialogMessage, zLoc::GetMessageString(0x3046));
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->MessageBoxA(dialogMessage, dialogCaption, MB_ICONEXCLAMATION);
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
        g_pWestwoodOnlineUpgradeDialog->OnDestroy();
        ExitProcess(0);
    }

    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 1;
    return 0;
}

// Reimplements 0x43fa70: WestwoodOnlineUpgradeApiEventSink::OnServerError
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnServerError(
    void *,
    int,
    const char *errorText
)
{
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
        ->MessageBoxA(errorText,
                      kWestwoodOnlineUpgradeServerErrorTitle,
                      MB_ICONHAND);
    return 0;
}

// Reimplements 0x43fa90: WestwoodOnlineUpgradeApiEventSink::OnApiStatus
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnApiStatus(
    void *,
    int statusCode,
    const char *statusText
)
{
    int resultCount;
    char failureCaption[kApiStatusTextBufferSize];
    char failureMessage[kApiStatusTextBufferSize];
    char resultCountStatusText[kApiStatusTextBufferSize];
    int failureMessageId;
    UINT messageBoxFlags;
    char *statusLine;
    WestwoodOnlineUpgradeApiCallbackComObject *api;

    Time::Reset();

    if (statusCode == 0)
    {
        statusLine = strtok(_strdup(statusText), kApiStatusLineDelimiter);
        while (statusLine != 0)
        {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusLine);
            statusLine = strtok(0, kApiStatusLineDelimiter);
        }

        SetEvent(g_WestwoodOnlineUpgradeStatusTextEvent);
        // Retail frees the final strtok result, which is NULL here, leaking the
        // duplicated status text buffer.
        free(statusLine);

        api = GetCallbackApiComObject();
        api->vftable->SetQueryMode((IUnknown *)api, kApiStatusActiveListMode);
        g_WestwoodOnlineUpgradeActiveListMode = kApiStatusActiveListMode;
        api->vftable->GetQueryResultCount((IUnknown *)api, &resultCount);

        if (resultCount == 0)
        {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kApiStatusNoResultsMessageId));
        }
        else if (resultCount == 1)
        {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kApiStatusOneResultMessageId));
        }
        else
        {
            zLoc::FormatMessage(resultCountStatusText,
                                kApiStatusTextBufferSize,
                                kApiStatusMultipleResultsMessageId,
                                resultCount);
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                resultCountStatusText);
        }

        return 0;
    }

    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)->DestroyWindow();
    strcpy(failureCaption,
           zLoc::GetMessageString(kApiStatusFailureCaptionMessageId));
    messageBoxFlags = MB_ICONEXCLAMATION;

    if (statusCode == kApiStatusFailure64)
    {
        failureMessageId = kApiStatusFailure64MessageId;
    }
    else if (statusCode == kApiStatusFailure65)
    {
        failureMessageId = kApiStatusFailure65MessageId;
    }
    else if (statusCode == kApiStatusFailure6a)
    {
        failureMessageId = kApiStatusFailure6aMessageId;
    }
    else if (statusCode == kApiStatusFailure72)
    {
        failureMessageId = kApiStatusFailure72MessageId;
    }
    else
    {
        failureMessageId = kApiStatusFailureDefaultMessageId;
        messageBoxFlags = MB_ICONHAND;
    }

    strcpy(failureMessage, zLoc::GetMessageString(failureMessageId));
    ((CWnd *)((unsigned int)g_RecoilApp.m_pMainWnd))
        ->MessageBoxA(failureMessage, failureCaption, messageBoxFlags);
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
    return 0;
}

// Reimplements 0x43fde0:
// WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived(
    void *,
    int status,
    const char *statusText
)
{
    char *statusLine;

    if (status < 0)
    {
        return 0;
    }

    // Retail duplicates statusText for strtok and never frees the duplicate.
    statusLine = strtok(_strdup(statusText), kApiStatusLineDelimiter);
    while (statusLine != 0)
    {
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusLine);
        statusLine = strtok(0, kApiStatusLineDelimiter);
    }

    SetEvent(g_WestwoodOnlineUpgradeStatusTextEvent);
    return 0;
}

// Reimplements 0x43fe50:
// WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord
)
{
    char statusText[kBrowseRecordAddedStatusBufferSize];
    WestwoodOnlineUpgradeApiCallbackComObject *api;

    if (status < 0)
    {
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
            zLoc::GetMessageString(kBrowseRecordAddedFailureMessageId));
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
        api = GetCallbackApiComObject();
        api->vftable->RequestListMode(
            (IUnknown *)api,
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        return 0;
    }

    const unsigned int messageId = browseRecord->m_recordFlags != 0 ?
        kBrowseRecordAddedClosedMessageId :
        kBrowseRecordAddedOpenMessageId;
    zLoc::FormatMessage(
        statusText,
        kBrowseRecordAddedStatusBufferSize,
        messageId,
        browseRecord->m_sessionName
    );
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);

    if (browseRecord->m_recordFlags != 0)
    {
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

// Reimplements 0x43ff80:
// WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
)
{
    char statusText[kBrowseSessionResolvedStatusBufferSize];
    unsigned int failureMessageId;
    WestwoodOnlineUpgradeApiCallbackComObject *api;

    if (status < 0)
    {
        strcpy(
            statusText,
            zLoc::GetMessageString(kBrowseSessionResolvedFailurePrefixMessageId)
        );
        strcat(statusText, kSpaceDelimiter);

        if (status == kBrowseSessionResolvedFailure6c)
        {
            failureMessageId = kBrowseSessionResolvedFailure6cMessageId;
        }
        else if (status == kBrowseSessionResolvedFailure70)
        {
            failureMessageId = kBrowseSessionResolvedFailure70MessageId;
        }
        else if (status == kBrowseSessionResolvedFailure72)
        {
            failureMessageId = kBrowseSessionResolvedFailure72MessageId;
        }
        else if (status == kBrowseSessionResolvedFailure71)
        {
            failureMessageId = kBrowseSessionResolvedFailure71MessageId;
        }
        else if (status == kBrowseSessionResolvedFailure6e)
        {
            failureMessageId = kBrowseSessionResolvedFailure6eMessageId;
        }
        else
        {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                kBrowseSessionResolvedStatusCodeFmt,
                zLoc::GetMessageString(
                    kBrowseSessionResolvedFailurePrefixMessageId),
                status
            );
            g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
            g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
            api = GetCallbackApiComObject();
            api->vftable->RequestListMode(
                (IUnknown *)api,
                g_WestwoodOnlineUpgradeActiveListMode,
                1
            );
            return 0;
        }

        strcat(statusText, zLoc::GetMessageString(failureMessageId));
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = '\0';
        api = GetCallbackApiComObject();
        api->vftable->RequestListMode(
            (IUnknown *)api,
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        return 0;
    }

    if ((sessionRequest->m_rowFlags & kSessionRequestRefreshCacheFlag) != 0)
    {
        g_WestwoodOnlineUpgradeCachedBrowseRecord = *browseRecord;
        g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
    }

    if ((sessionRequest->m_rowFlags & kSessionRequestSkipDetailsFlag) == 0)
    {
        api = GetCallbackApiComObject();
        api->vftable->RequestListMode(
            (IUnknown *)api,
            g_WestwoodOnlineUpgradeActiveListMode,
            1
        );
        if (browseRecord->m_recordFlags != 0 &&
            g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag == 1)
        {
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
        api->vftable->RequestSessionDetails((IUnknown *)api, sessionRequest);
    }

    return 0;
}

// Reimplements 0x4401d0:
// WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
)
{
    char statusText[kBrowseSessionResolvedStatusBufferSize];
    LRESULT sessionIndex;
    WestwoodOnlineUpgradeApiCallbackComObject *api;

    if (status < 0)
    {
        api = GetCallbackApiComObject();
        api->vftable->RequestListMode(
            (IUnknown *)api,
            kApiStatusActiveListMode,
            1
        );
        api = GetCallbackApiComObject();
        api->vftable->CancelPendingSessionFlow((IUnknown *)api);
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

    sessionIndex =
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(
                kWestwoodOnlineUpgradeSessionResultsListId,
                LB_FINDSTRINGEXACT,
                (WPARAM)-1,
                (LPARAM)sessionRequest->m_sessionName
            );
    if (sessionIndex != LB_ERR)
    {
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
        (sessionRequest->m_rowFlags & kSessionRequestRefreshCacheFlag) == 0)
    {
        g_pWestwoodOnlineUpgradeDialog->AppendConnectStatusAndRefreshList(
            browseRecord->m_sessionName);
    }

    api = GetCallbackApiComObject();
    api->vftable->RequestListMode(
        (IUnknown *)api,
        g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
    return 0;
}

// Reimplements 0x4402c0:
// WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated(
    void *,
    int status,
    WestwoodOnlineUpgradeBrowseRecord *browseRecord,
    WestwoodOnlineUpgradeSessionRequest *sessionList
)
{
    int deferAutoConnect;
    int rowFlags;
    char sessionResultText[kSessionListEnumeratedResultTextBufferSize];
    LRESULT sessionIndex;
    WestwoodOnlineUpgradeSessionRequest *sessionNode;

    if (status < 0)
    {
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

    while (sessionNode != 0)
    {
        rowFlags = sessionNode->m_rowFlags;
        ++g_WestwoodOnlineUpgradePendingSessionResultCount;
        strcpy(sessionResultText, sessionNode->m_sessionName);

        if ((rowFlags & kSessionRequestRefreshCacheFlag) != 0)
        {
            if ((rowFlags & kSessionRequestSkipDetailsFlag) != 0)
            {
                strcat(
                    sessionResultText,
                    zLoc::GetMessageString(
                        kSessionListEnumeratedReadyMessageId)
                );
                deferAutoConnect = 1;
                if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag == 0)
                {
                    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
                    g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(1);
                }
            }
            else
            {
                strcat(
                    sessionResultText,
                    zLoc::GetMessageString(
                        kSessionListEnumeratedClosedMessageId)
                );
                g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
                g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(1);
            }
        }
        else if ((rowFlags & kSessionRequestSkipDetailsFlag) != 0)
        {
            strcat(
                sessionResultText,
                zLoc::GetMessageString(
                    kSessionListEnumeratedPendingMessageId)
            );
            deferAutoConnect = 1;
        }

        sessionIndex =
            ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
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

    if (deferAutoConnect == 0)
    {
        g_pWestwoodOnlineUpgradeDialog->AppendConnectStatusAndRefreshList(
            browseRecord->m_sessionName);
    }

    return 0;
}

// Reimplements 0x43f9d0:
// WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeApiEventSink.cpp)
RECOIL_NOINLINE int RECOIL_STDCALL
WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved(
    void *,
    int status,
    WestwoodOnlineUpgradeSessionRequest *sessionRequest
)
{
    char statusText[kPendingSessionRemovedStatusBufferSize];
    LRESULT sessionIndex;

    if (status < 0)
    {
        return 0;
    }

    zLoc::FormatMessage(statusText,
                        kPendingSessionRemovedStatusBufferSize,
                        kPendingSessionRemovedStatusMessageId,
                        sessionRequest->m_sessionName);
    g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(statusText);

    sessionIndex =
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(kWestwoodOnlineUpgradeSessionResultsListId,
                                  LB_FINDSTRINGEXACT,
                                  (WPARAM)-1,
                                  (LPARAM)sessionRequest->m_sessionName);
    if (sessionIndex != LB_ERR)
    {
        ((CWnd *)g_pWestwoodOnlineUpgradeDialog)
            ->SendDlgItemMessageA(kWestwoodOnlineUpgradeSessionResultsListId,
                                  LB_DELETESTRING,
                                  (WPARAM)sessionIndex,
                                  0);
        --g_WestwoodOnlineUpgradePendingSessionResultCount;
    }

    return 0;
}
