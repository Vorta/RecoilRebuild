#include "Battlesport/WestwoodOnlineUpgradeConfigDialog.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/WestwoodOnlineUpgradeApi.h"
#include "Battlesport/WestwoodOnlineUpgradeApiEventSink.h"
#include "Battlesport/WestwoodOnlineUpgradeDialog.h"
#include "Battlesport/WestwoodOnlineUpgradeRefCountAndLock.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/wwonline/upgrade_download.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <ocidl.h>
#include <stdarg.h>
#include <string.h>

extern int g_threeFloatDefaultCount;
extern long g_threeFloatDefaultReturn;
extern int g_threeFloatUpdateDataCount;
extern int g_threeFloatUpdateDataSaveValue[8];
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" HWND g_RecoilApp_hWndMain;

namespace
{
RecoilNamedVtable *TestObjectVtable(void *object)
{
    return *(RecoilNamedVtable **)object;
}

bool TestMfcWindowConstructed(CWnd &wnd)
{
    return *(void **)&wnd != 0 && wnd.m_hWnd == 0;
}

struct ImportFunctionPatch
{
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct CodeFunctionPatch
{
    unsigned char *address;
    unsigned char original[5];
};

struct ShutdownFakeUnknownVtable
{
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IUnknown *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IUnknown *);
    ULONG(STDMETHODCALLTYPE *Release)(IUnknown *);
};

struct ShutdownFakeConnectionPointContainerVtable
{
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IConnectionPointContainer *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IConnectionPointContainer *);
    ULONG(STDMETHODCALLTYPE *Release)(IConnectionPointContainer *);
    HRESULT(STDMETHODCALLTYPE *EnumConnectionPoints)(IConnectionPointContainer *, void **);
    HRESULT(STDMETHODCALLTYPE *FindConnectionPoint)(IConnectionPointContainer *, REFIID,
                                                    IConnectionPoint **);
};

struct ShutdownFakeConnectionPointVtable
{
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IConnectionPoint *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IConnectionPoint *);
    ULONG(STDMETHODCALLTYPE *Release)(IConnectionPoint *);
    HRESULT(STDMETHODCALLTYPE *GetConnectionInterface)(IConnectionPoint *, IID *);
    HRESULT(STDMETHODCALLTYPE *GetConnectionPointContainer)(IConnectionPoint *,
                                                           IConnectionPointContainer **);
    HRESULT(STDMETHODCALLTYPE *Advise)(IConnectionPoint *, IUnknown *, DWORD *);
    HRESULT(STDMETHODCALLTYPE *Unadvise)(IConnectionPoint *, DWORD);
    HRESULT(STDMETHODCALLTYPE *EnumConnections)(IConnectionPoint *, void **);
};

struct ShutdownFakeUnknown
{
    ShutdownFakeUnknownVtable *vftable;
};

struct ShutdownFakeConnectionPointContainer
{
    ShutdownFakeConnectionPointContainerVtable *vftable;
};

struct ShutdownFakeConnectionPoint
{
    ShutdownFakeConnectionPointVtable *vftable;
};

struct InitFakeApiVtable
{
    void *QueryInterface;
    void *AddRef;
    void *Release;
    void(STDMETHODCALLTYPE *ProcessCallbacks)(IUnknown *self);
    void(STDMETHODCALLTYPE *BeginConnect)(IUnknown *self, int languageId, int productId,
                                          const char *playerName,
                                          const char *connectString,
                                          int timeoutSeconds);
    void(STDMETHODCALLTYPE *RequestBootstrapServerList)(
        IUnknown *self,
        WestwoodOnlineUpgradeBootstrapServerRecord *selectedBootstrapServer,
        int timeoutSeconds,
        int useAlternateConnectString);
    void(STDMETHODCALLTYPE *RequestListMode)(IUnknown *self, int listMode, int enabled);
    int(STDMETHODCALLTYPE *SubmitQueryRequest)(
        IUnknown *self,
        WestwoodOnlineUpgradeQueryRequest *request);
    int(STDMETHODCALLTYPE *LoadBrowseRecord)(
        IUnknown *self,
        WestwoodOnlineUpgradeBrowseRecord *record);
    void(STDMETHODCALLTYPE *ResetQueryState)(IUnknown *self);
    void(STDMETHODCALLTYPE *CancelPendingSessionFlow)(IUnknown *self);
    void(STDMETHODCALLTYPE *SubmitStatusText)(IUnknown *self, const char *statusText);
    void(STDMETHODCALLTYPE *SubmitSessionRequestListAndStatusText)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *sessionRequestList,
        const char *statusText);
    void(STDMETHODCALLTYPE *Disconnect)(IUnknown *self);
    void *reserved038;
    void(STDMETHODCALLTYPE *SubmitEncodedQueryString)(
        IUnknown *self,
        const char *encodedQuery);
    void *reserved040;
    void *reserved044;
    void(STDMETHODCALLTYPE *SubmitPendingSessionList)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *sessionRequestList);
    void *reserved04c;
    void *reserved050;
    void(STDMETHODCALLTYPE *QueueSessionRequest)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *request);
    void(STDMETHODCALLTYPE *RequestSessionDetails)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *request);
    void *reserved05c;
    int(STDMETHODCALLTYPE *RequestUpgradeDownloadReadyResult)(
        IUnknown *self,
        WestwoodOnlineUpgradeConnectContext *context);
    int(STDMETHODCALLTYPE *QueryStatusWithTokenAndServer)(
        IUnknown *self,
        WestwoodOnlineUpgradeConnectContext *context,
        const char *serverText);
    void *reserved068;
    void(STDMETHODCALLTYPE *BeginConnectWithPreparedContext)(
        IUnknown *self,
        WestwoodOnlineUpgradeConnectContext *context,
        int mode);
    int(STDMETHODCALLTYPE *PrepareConnectContextAndMode)(
        IUnknown *self,
        WestwoodOnlineUpgradeConnectContext *context);
    void(STDMETHODCALLTYPE *SetQueryMode)(IUnknown *self, int listMode);
    void *reserved078;
    void *reserved07c;
    void(STDMETHODCALLTYPE *LookupBrowseRecordBySessionName)(
        IUnknown *self,
        const char *sessionName,
        int lookupMode);
    void *reserved084[5];
    void(STDMETHODCALLTYPE *GetQueryResultCount)(IUnknown *self, int *outCount);
};

struct InitFakeApiObject
{
    InitFakeApiVtable *vftable;
};

struct InitFakeProgressVtable
{
    void *reserved000[24];
    int(RECOIL_FASTCALL *DestroyWindow)(CWnd *self, void *edx);
};

int g_modalSelectedMissionIndex;
int g_modalMenuStep;
int g_modalMenuVisible[2];
void *g_modalMenuThis[2];
int g_modalDoModalCalls;
int g_modalDialogDtorCalls;
int g_modalCStringDtorCalls;
int g_modalListDtorCalls;
int g_modalEditDtorCalls;
int g_modalComboDtorCalls;
int g_modalButtonDtorCalls;
bool g_modalArgsOk;
int g_modalDtorSequenceCount;
char g_modalDtorSequence[32];
int g_modalDdxStep;
CDataExchange *g_modalDdxContext[24];
int g_modalDdxKind[24];
int g_modalDdxControlId[24];
void *g_modalDdxValue[24];
int g_modalTimeTickCalls;
int g_configDoModalCalls;
int g_configDoModalResult;
WestwoodOnlineUpgradeConfigDialog *g_configDoModalThis;
ShutdownFakeUnknown g_shutdownUnknown;
ShutdownFakeConnectionPointContainer g_shutdownCpc;
ShutdownFakeConnectionPoint g_shutdownConnectionPoint;
int g_shutdownSourceReleaseCalls;
int g_shutdownCpcReleaseCalls;
int g_shutdownConnectionPointReleaseCalls;
int g_shutdownFindConnectionPointCalls;
int g_shutdownUnadviseCalls;
DWORD g_shutdownUnadviseCookie;
bool g_shutdownIidOk;
int g_shutdownAdviseCalls;
IUnknown *g_shutdownAdviseSink;
DWORD g_shutdownAdviseCookie;
int g_apiCreateCoInitializeCalls;
void *g_apiCreateCoInitializeReserved;
int g_apiCreateCoCreateCalls;
bool g_apiCreateCoCreateArgsOk;
HRESULT g_apiCreateCoCreateResult;
IUnknown *g_apiCreateCoCreateObject;
int g_apiCreateShowModalCalls;
int g_apiCreateShowModalResult;
int g_downloadCreateCoCreateCalls;
bool g_downloadCreateCoCreateArgsOk;
HRESULT g_downloadCreateCoCreateResult;
IUnknown *g_downloadCreateCoCreateObject;
InitFakeApiObject g_initFakeApi;
InitFakeApiVtable g_initFakeApiVtable;
InitFakeProgressVtable g_initProgressVtable;
WestwoodOnlineUpgradeDownloadComObject g_downloadDlgFakeObject;
int g_initCreateInstanceCalls;
HANDLE g_initCreateInstanceBootstrap;
int g_initCreateEventCalls;
HANDLE g_initCreatedEvents[3];
int g_initCreateProgressCalls;
LPCSTR g_initCreateProgressResource;
CWnd *g_initCreateProgressParent;
int g_initSetDlgItemTextCalls;
void *g_initSetDlgItemTextThis[4];
int g_initSetDlgItemTextControlId[4];
const char *g_initSetDlgItemTextValue[4];
unsigned int g_initMessageIds[16];
int g_initMessageIdCalls;
int g_initDialogBaseOnInitCalls;
CDialog *g_initDialogBaseOnInitThis;
int g_initDialogApiInitCalls;
int g_initDialogApiInitResult;
int g_initDialogOnDestroyCalls;
WestwoodOnlineUpgradeDialog *g_initDialogOnDestroyThis;
int g_initDialogSetAbortCalls;
WestwoodOnlineUpgradeDialog *g_initDialogSetAbortThis;
int g_initDialogSetTimerCalls;
HWND g_initDialogSetTimerHwnd;
UINT_PTR g_initDialogSetTimerId;
UINT g_initDialogSetTimerMs;
TIMERPROC g_initDialogSetTimerProc;
int g_initDialogSendMessageCalls;
HWND g_initDialogSendMessageHwnd[24];
UINT g_initDialogSendMessageMsg[24];
WPARAM g_initDialogSendMessageWParam[24];
LPARAM g_initDialogSendMessageLParam[24];
int g_initDialogComboAddCalls;
int g_initBeginConnectCalls;
int g_initBeginConnectLanguageId;
int g_initBeginConnectProductId;
const char *g_initBeginConnectPlayerName;
const char *g_initBeginConnectConnectString;
int g_initBeginConnectTimeoutSeconds;
int g_initProcessCallbacksCalls;
int g_initRequestBootstrapCalls;
WestwoodOnlineUpgradeBootstrapServerRecord *g_initRequestBootstrapServer;
int g_initRequestBootstrapTimeoutSeconds;
int g_initRequestBootstrapUseAlternate;
int g_initRequestListModeCalls;
int g_initRequestListMode;
int g_initRequestListModeEnabled;
int g_initDisconnectCalls;
int g_beginConnectGetWindowTextCalls;
void *g_beginConnectGetWindowTextThis;
char *g_beginConnectGetWindowTextBuffer;
int g_beginConnectGetWindowTextMaxCount;
int g_beginConnectPrepareCalls;
WestwoodOnlineUpgradeConnectContext *g_beginConnectPrepareContext;
int g_beginConnectPrepareResult;
int g_beginConnectPreparedCalls;
WestwoodOnlineUpgradeConnectContext *g_beginConnectPreparedContext;
int g_beginConnectPreparedMode;
int g_checkAndApplyUpgradeCalls;
WestwoodOnlineUpgradeConnectContext *g_checkAndApplyUpgradeContext;
int g_checkAndApplyUpgradeResult;
char g_queryStatusTokenInput[20];
char g_queryStatusServerInput[80];
int g_queryStatusGetWindowTextCalls;
void *g_queryStatusGetWindowTextThis[2];
int g_queryStatusGetWindowTextMaxCount[2];
int g_queryStatusSetWindowTextCalls;
void *g_queryStatusSetWindowTextThis[4];
char g_queryStatusSetWindowTextValue[4][8];
int g_queryStatusProviderCalls;
WestwoodOnlineUpgradeConnectContext *g_queryStatusProviderContext;
const char *g_queryStatusProviderServerText;
int g_queryStatusProviderResult;
int g_submitEncodedQueryCalls;
IUnknown *g_submitEncodedQuerySelf;
char g_submitEncodedQueryText[64];
int g_submitPendingSessionListCalls;
IUnknown *g_submitPendingSessionListSelf;
int g_submitPendingSessionListCount;
char g_submitPendingSessionListNames[8][0x34];
int g_queryStatusMessageBoxCalls;
void *g_queryStatusMessageBoxThis;
char g_queryStatusMessageBoxText[64];
char g_queryStatusMessageBoxCaption[64];
unsigned int g_queryStatusMessageBoxType;
int g_queryStatusMessageBoxResult;
char g_refreshCurrentQuerySessionNameInput[64];
int g_refreshCurrentQueryGetWindowTextCalls;
void *g_refreshCurrentQueryGetWindowTextThis;
int g_refreshCurrentQuerySubmitCalls;
WestwoodOnlineUpgradeQueryRequest g_refreshCurrentQuerySubmitRequest;
int g_refreshCurrentQuerySubmitResult;
int g_queueSessionRequestCalls;
WestwoodOnlineUpgradeSessionRequest g_queueSessionRequestCopies[8];
IUnknown *g_queueSessionRequestSelf[8];
char g_submitVisibleStatusInput[64];
int g_submitVisibleGetWindowTextCalls;
void *g_submitVisibleGetWindowTextThis;
int g_submitVisibleSubmitStatusCalls;
IUnknown *g_submitVisibleSubmitStatusSelf;
char g_submitVisibleSubmitStatusText[64];
int g_submitVisibleSubmitListCalls;
IUnknown *g_submitVisibleSubmitListSelf;
int g_submitVisibleSubmitListCount;
char g_submitVisibleSubmitListStatusText[64];
char g_submitVisibleSubmitListNames[8][0x34];
int g_submitVisibleFormatCalls;
char *g_submitVisibleFormatBuffer;
int g_submitVisibleFormatMaxChars;
unsigned int g_submitVisibleFormatMessageId;
const char *g_submitVisibleFormatSessionName;
const char *g_submitVisibleFormatStatusText;
int g_submitVisibleAppendCalls;
WestwoodOnlineUpgradeDialog *g_submitVisibleAppendThis;
char g_submitVisibleAppendFormat[64];
char g_submitVisibleAppendArg0[64];
char g_submitVisibleAppendArg1[64];
int g_lookupBrowseRecordCalls;
IUnknown *g_lookupBrowseRecordSelf[8];
char g_lookupBrowseRecordSessionName[8][0x34];
int g_lookupBrowseRecordMode[8];
int g_loadBrowseRecordCalls;
IUnknown *g_loadBrowseRecordSelf;
WestwoodOnlineUpgradeBrowseRecord g_loadBrowseRecordCopy;
WestwoodOnlineUpgradeBrowseRecord *g_loadBrowseRecordRecord;
int g_loadBrowseRecordResult;
int g_initDestroyProgressCalls;
CWnd *g_initDestroyedProgress;
int g_destroyBeginDisconnectCalls;
WestwoodOnlineUpgradeDialog *g_destroyBeginDisconnectThis;
int g_destroyShutdownCalls;
int g_destroyKillTimerCalls;
HWND g_destroyKillTimerHwnd;
UINT_PTR g_destroyKillTimerId;
DWORD g_initWaitResults[4];
int g_initWaitResultCount;
int g_initWaitCalls;
HANDLE *g_initWaitHandles;
DWORD g_initWaitTimeouts[4];
int g_initResetEventCalls;
HANDLE g_initResetEventHandle;
int g_initSleepCalls;
DWORD g_initSleepDurations[4];
int g_bootstrapSetEventCalls;
HANDLE g_bootstrapSetEventHandles[4];
int g_enableWindowCalls;
void *g_enableWindowThis[12];
int g_enableWindowEnable[12];
int g_resetQueryStateCalls;
int g_resetSendMessageCalls;
HWND g_resetSendMessageHwnd;
UINT g_resetSendMessageMsg;
WPARAM g_resetSendMessageWParam;
LPARAM g_resetSendMessageLParam;
int g_appendConnectFormatCalls;
char *g_appendConnectFormatBuffer;
int g_appendConnectFormatMaxChars;
unsigned int g_appendConnectFormatMessageId;
const char *g_appendConnectFormatSessionName;
int g_appendConnectAppendCalls;
WestwoodOnlineUpgradeDialog *g_appendConnectAppendThis;
const char *g_appendConnectAppendText;
int g_appendConnectResetCalls;
WestwoodOnlineUpgradeDialog *g_appendConnectResetThis;
int g_abortOnCancelCalls;
CDialog *g_abortOnCancelThis;
int g_downloadDlgBeginCalls;
IUnknown *g_downloadDlgBeginSelf;
char g_downloadDlgBeginDescriptor0[80];
char g_downloadDlgBeginDescriptor1[80];
char g_downloadDlgBeginDescriptor2[80];
char g_downloadDlgBeginSourcePath[260];
char g_downloadDlgBeginFileName[80];
char g_downloadDlgBeginRegistryKey[80];
int g_downloadDlgAbortCalls;
IUnknown *g_downloadDlgAbortSelf;
int g_downloadDlgPumpCalls;
IUnknown *g_downloadDlgPumpSelf;
int g_downloadDlgReleaseCalls;
int g_downloadDlgSetDlgItemTextCalls;
HWND g_downloadDlgSetDlgItemTextHwnd[4];
int g_downloadDlgSetDlgItemTextControlId[4];
const char *g_downloadDlgSetDlgItemTextValue[4];
int g_downloadDlgGetCurrentDirectoryCalls;
char g_downloadDlgCurrentDirectory[260];
int g_downloadDlgSetCurrentDirectoryCalls;
const char *g_downloadDlgSetCurrentDirectoryPath[4];
BOOL g_downloadDlgSetCurrentDirectoryResult[4];
int g_downloadDlgCreateDirectoryCalls;
const char *g_downloadDlgCreateDirectoryPath;
LPSECURITY_ATTRIBUTES g_downloadDlgCreateDirectorySecurity;
int g_downloadDlgSetTimerCalls;
HWND g_downloadDlgSetTimerHwnd;
UINT_PTR g_downloadDlgSetTimerId;
UINT g_downloadDlgSetTimerMs;
TIMERPROC g_downloadDlgSetTimerProc;
int g_downloadDlgKillTimerCalls;
HWND g_downloadDlgKillTimerHwnd;
UINT_PTR g_downloadDlgKillTimerId;
int g_downloadDlgDestroyWindowCalls;
HWND g_downloadDlgDestroyWindowHwnd[4];
int g_downloadDlgEndDialogCalls;
HWND g_downloadDlgEndDialogHwnd;
INT_PTR g_downloadDlgEndDialogResult;
int g_downloadReadyFormatCalls;
char *g_downloadReadyFormatBuffer[8];
int g_downloadReadyFormatMaxChars[8];
unsigned int g_downloadReadyFormatMessageId[8];
int g_downloadReadyFormatOrdinal[8];
int g_downloadReadyFormatTotal[8];
int g_downloadReadyDialogCalls;
HINSTANCE g_downloadReadyDialogInstance[8];
LPCSTR g_downloadReadyDialogTemplate[8];
HWND g_downloadReadyDialogParent[8];
DLGPROC g_downloadReadyDialogProc[8];
LPARAM g_downloadReadyDialogParam[8];
INT_PTR g_downloadReadyDialogResult[8];
int g_downloadReadyCallbackMessageBoxCalls;
void *g_downloadReadyCallbackMessageBoxThis;
char g_downloadReadyCallbackMessageBoxText[128];
char g_downloadReadyCallbackMessageBoxCaption[128];
UINT g_downloadReadyCallbackMessageBoxType;
int g_downloadReadyCallbackMessageBoxResult[4];
int g_downloadReadyCallbackSetEventCalls;
HANDLE g_downloadReadyCallbackSetEventHandle[4];
int g_downloadReadyCallbackShowListCalls;
WestwoodOnlineUpgradeDownloadReadyEntry *g_downloadReadyCallbackShowList;
int g_downloadReadyCallbackShowListResult;
int g_pendingRemovedFormatCalls;
char *g_pendingRemovedFormatBuffer;
int g_pendingRemovedFormatMaxChars;
unsigned int g_pendingRemovedFormatMessageId;
const char *g_pendingRemovedFormatSessionName;
int g_pendingRemovedAppendCalls;
WestwoodOnlineUpgradeDialog *g_pendingRemovedAppendThis;
char g_pendingRemovedAppendText[128];
int g_pendingRemovedSendMessageCalls;
CWnd *g_pendingRemovedSendMessageThis[12];
int g_pendingRemovedSendMessageControlId[12];
UINT g_pendingRemovedSendMessageMessage[12];
WPARAM g_pendingRemovedSendMessageWParam[12];
LPARAM g_pendingRemovedSendMessageLParam[12];
LRESULT g_pendingRemovedSendMessageResult[12];
char g_pendingRemovedSendMessageText[12][260];
int g_serverErrorMessageBoxCalls;
CWnd *g_serverErrorMessageBoxThis;
char g_serverErrorMessageBoxText[128];
char g_serverErrorMessageBoxCaption[64];
UINT g_serverErrorMessageBoxType;
int g_apiStatusTimeResetCalls;
int g_apiStatusAppendCalls;
WestwoodOnlineUpgradeDialog *g_apiStatusAppendThis[8];
char g_apiStatusAppendText[8][128];
int g_apiStatusSetQueryModeCalls;
IUnknown *g_apiStatusSetQueryModeSelf;
int g_apiStatusSetQueryModeValue;
int g_apiStatusGetQueryResultCountCalls;
IUnknown *g_apiStatusGetQueryResultCountSelf;
int *g_apiStatusGetQueryResultCountOut;
int g_apiStatusQueryResultCount;
int g_apiStatusFormatCalls;
char *g_apiStatusFormatBuffer;
int g_apiStatusFormatMaxChars;
unsigned int g_apiStatusFormatMessageId;
int g_apiStatusFormatResultCount;
int g_browseRecordAddedFormatCalls;
char *g_browseRecordAddedFormatBuffer;
int g_browseRecordAddedFormatMaxChars;
unsigned int g_browseRecordAddedFormatMessageId;
const char *g_browseRecordAddedFormatSessionName;
int g_browseRecordAddedEnableCalls;
WestwoodOnlineUpgradeDialog *g_browseRecordAddedEnableThis[4];
int g_browseRecordAddedEnableValue[4];
int g_browseResolvedAppendCalls;
WestwoodOnlineUpgradeDialog *g_browseResolvedAppendThis[8];
char g_browseResolvedAppendFormat[8][128];
const char *g_browseResolvedAppendArgText[8];
int g_browseResolvedAppendArgStatus[8];
int g_browseResolvedFormatCalls;
char *g_browseResolvedFormatBuffer;
int g_browseResolvedFormatMaxChars;
unsigned int g_browseResolvedFormatMessageId;
const char *g_browseResolvedFormatSessionName;
const char *g_browseResolvedFormatBrowseName;
int g_browseResolvedUpdateCalls;
WestwoodOnlineUpgradeDialog *g_browseResolvedUpdateThis;
int g_browseResolvedConnectCalls;
WestwoodOnlineUpgradeDialog *g_browseResolvedConnectThis[4];
int g_browseResolvedConnectValue[4];
int g_browseResolvedRequestDetailsCalls;
IUnknown *g_browseResolvedRequestDetailsSelf;
WestwoodOnlineUpgradeSessionRequest *g_browseResolvedRequestDetailsRequest;
int g_sessionFinishedCancelCalls;
IUnknown *g_sessionFinishedCancelSelf;
int g_sessionFinishedAppendConnectCalls;
WestwoodOnlineUpgradeDialog *g_sessionFinishedAppendConnectThis;
const char *g_sessionFinishedAppendConnectSessionName;

template <typename Method> void *MethodAddress(Method method)
{
    union
    {
        Method method;
        void *address;
    } value = {method};
    return value.address;
}

class WestwoodCWndAccess : public CWnd
{
  public:
    using CWnd::Default;
    using CWnd::SetWindowText;
    using CWnd::UpdateData;
};

void *CWndDefaultAddress()
{
    return MethodAddress(&WestwoodCWndAccess::Default);
}

void *CWndSetWindowTextAAddress()
{
    return MethodAddress(&WestwoodCWndAccess::SetWindowText);
}

void *CWndUpdateDataAddress()
{
    return MethodAddress(&WestwoodCWndAccess::UpdateData);
}

template <typename Method> AFX_PMSG MessageHandler(Method method)
{
    union
    {
        Method method;
        AFX_PMSG handler;
    } value = {method};
    return value.handler;
}

HRESULT STDMETHODCALLTYPE ShutdownUnknownQueryInterface(IUnknown *, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, IID_IConnectionPointContainer))
    {
        *out = &g_shutdownCpc;
        return S_OK;
    }

    *out = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE ShutdownUnknownAddRef(IUnknown *)
{
    return 2;
}

ULONG STDMETHODCALLTYPE ShutdownUnknownRelease(IUnknown *)
{
    ++g_shutdownSourceReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE ShutdownCpcQueryInterface(IConnectionPointContainer *, REFIID,
                                                    void **out)
{
    *out = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE ShutdownCpcAddRef(IConnectionPointContainer *)
{
    return 2;
}

ULONG STDMETHODCALLTYPE ShutdownCpcRelease(IConnectionPointContainer *)
{
    ++g_shutdownCpcReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE ShutdownEnumConnectionPoints(IConnectionPointContainer *, void **)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ShutdownFindConnectionPoint(IConnectionPointContainer *, REFIID iid,
                                                      IConnectionPoint **out)
{
    ++g_shutdownFindConnectionPointCalls;
    g_shutdownIidOk =
        IsEqualGUID(iid, g_WestwoodOnlineUpgradeApiEventSink_IID) ||
        IsEqualGUID(iid, g_WestwoodOnlineUpgradeDownloadEventSink_IID)
            ? true
            : false;
    *out = (IConnectionPoint *)&g_shutdownConnectionPoint;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ShutdownCpQueryInterface(IConnectionPoint *, REFIID, void **out)
{
    *out = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE ShutdownCpAddRef(IConnectionPoint *)
{
    return 2;
}

ULONG STDMETHODCALLTYPE ShutdownCpRelease(IConnectionPoint *)
{
    ++g_shutdownConnectionPointReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE ShutdownGetConnectionInterface(IConnectionPoint *, IID *)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ShutdownGetConnectionPointContainer(IConnectionPoint *,
                                                             IConnectionPointContainer **)
{
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE ShutdownAdvise(IConnectionPoint *, IUnknown *sink, DWORD *cookie)
{
    ++g_shutdownAdviseCalls;
    g_shutdownAdviseSink = sink;
    if (cookie != 0)
    {
        *cookie = 0x87654321;
        g_shutdownAdviseCookie = *cookie;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ShutdownUnadvise(IConnectionPoint *, DWORD cookie)
{
    ++g_shutdownUnadviseCalls;
    g_shutdownUnadviseCookie = cookie;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE ShutdownEnumConnections(IConnectionPoint *, void **)
{
    return E_NOTIMPL;
}

HRESULT WINAPI FakeApiCreateCoInitialize(void *reserved)
{
    ++g_apiCreateCoInitializeCalls;
    g_apiCreateCoInitializeReserved = reserved;
    return S_OK;
}

HRESULT WINAPI FakeApiCreateCoCreateInstance(REFCLSID rclsid, LPUNKNOWN outer,
                                             DWORD clsContext, REFIID riid,
                                             LPVOID *outObject)
{
    ++g_apiCreateCoCreateCalls;
    g_apiCreateCoCreateArgsOk =
        IsEqualGUID(rclsid, g_WestwoodOnlineUpgradeApi_CLSID) != 0 &&
        outer == 0 &&
        clsContext == CLSCTX_INPROC_SERVER &&
        IsEqualGUID(riid, g_WestwoodOnlineUpgradeApi_IID) != 0 &&
        outObject != 0;
    if (outObject != 0)
    {
        *outObject = g_apiCreateCoCreateObject;
    }
    return g_apiCreateCoCreateResult;
}

HRESULT WINAPI FakeDownloadCreateCoCreateInstance(REFCLSID rclsid, LPUNKNOWN outer,
                                                  DWORD clsContext, REFIID riid,
                                                  LPVOID *outObject)
{
    ++g_downloadCreateCoCreateCalls;
    g_downloadCreateCoCreateArgsOk =
        IsEqualGUID(rclsid, g_WestwoodOnlineUpgradeDownload_CLSID) != 0 &&
        outer == 0 &&
        clsContext == CLSCTX_INPROC_SERVER &&
        IsEqualGUID(riid, g_WestwoodOnlineUpgradeDownload_IID) != 0 &&
        outObject != 0;
    if (outObject != 0)
    {
        *outObject = g_downloadCreateCoCreateObject;
    }
    return g_downloadCreateCoCreateResult;
}

HRESULT STDMETHODCALLTYPE DownloadDlgQueryInterface(IUnknown *, REFIID iid, void **out)
{
    if (IsEqualGUID(iid, IID_IConnectionPointContainer))
    {
        *out = &g_shutdownCpc;
        return S_OK;
    }

    *out = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE DownloadDlgAddRef(IUnknown *)
{
    return 2;
}

ULONG STDMETHODCALLTYPE DownloadDlgRelease(IUnknown *)
{
    ++g_downloadDlgReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE DownloadDlgBeginDownload(IUnknown *self,
                                                   const char *descriptor0,
                                                   const char *descriptor1,
                                                   const char *descriptor2,
                                                   const char *sourcePath,
                                                   const char *fileName,
                                                   const char *registryKey)
{
    ++g_downloadDlgBeginCalls;
    g_downloadDlgBeginSelf = self;
    strcpy(g_downloadDlgBeginDescriptor0, descriptor0);
    strcpy(g_downloadDlgBeginDescriptor1, descriptor1);
    strcpy(g_downloadDlgBeginDescriptor2, descriptor2);
    strcpy(g_downloadDlgBeginSourcePath, sourcePath);
    strcpy(g_downloadDlgBeginFileName, fileName);
    strcpy(g_downloadDlgBeginRegistryKey, registryKey);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DownloadDlgAbort(IUnknown *self)
{
    ++g_downloadDlgAbortCalls;
    g_downloadDlgAbortSelf = self;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE DownloadDlgPump(IUnknown *self)
{
    ++g_downloadDlgPumpCalls;
    g_downloadDlgPumpSelf = self;
    return S_OK;
}

BOOL WINAPI FakeDownloadDlgSetDlgItemTextA(HWND hWnd, int controlId, LPCSTR text)
{
    if (g_downloadDlgSetDlgItemTextCalls < 4)
    {
        g_downloadDlgSetDlgItemTextHwnd[g_downloadDlgSetDlgItemTextCalls] = hWnd;
        g_downloadDlgSetDlgItemTextControlId[g_downloadDlgSetDlgItemTextCalls] =
            controlId;
        g_downloadDlgSetDlgItemTextValue[g_downloadDlgSetDlgItemTextCalls] = text;
    }
    ++g_downloadDlgSetDlgItemTextCalls;
    return TRUE;
}

DWORD WINAPI FakeDownloadDlgGetCurrentDirectoryA(DWORD bufferChars, LPSTR buffer)
{
    ++g_downloadDlgGetCurrentDirectoryCalls;
    if (bufferChars != 0)
    {
        lstrcpynA(buffer, g_downloadDlgCurrentDirectory, bufferChars);
    }
    return lstrlenA(g_downloadDlgCurrentDirectory);
}

BOOL WINAPI FakeDownloadDlgSetCurrentDirectoryA(LPCSTR path)
{
    const int index = g_downloadDlgSetCurrentDirectoryCalls;
    if (index < 4)
    {
        g_downloadDlgSetCurrentDirectoryPath[index] = path;
    }
    ++g_downloadDlgSetCurrentDirectoryCalls;
    if (index < 4)
    {
        return g_downloadDlgSetCurrentDirectoryResult[index];
    }
    return TRUE;
}

BOOL WINAPI FakeDownloadDlgCreateDirectoryA(LPCSTR path,
                                            LPSECURITY_ATTRIBUTES security)
{
    ++g_downloadDlgCreateDirectoryCalls;
    g_downloadDlgCreateDirectoryPath = path;
    g_downloadDlgCreateDirectorySecurity = security;
    return TRUE;
}

UINT_PTR WINAPI FakeDownloadDlgSetTimer(HWND hWnd,
                                        UINT_PTR timerId,
                                        UINT elapsedMs,
                                        TIMERPROC timerProc)
{
    ++g_downloadDlgSetTimerCalls;
    g_downloadDlgSetTimerHwnd = hWnd;
    g_downloadDlgSetTimerId = timerId;
    g_downloadDlgSetTimerMs = elapsedMs;
    g_downloadDlgSetTimerProc = timerProc;
    return timerId;
}

BOOL WINAPI FakeDownloadDlgKillTimer(HWND hWnd, UINT_PTR timerId)
{
    ++g_downloadDlgKillTimerCalls;
    g_downloadDlgKillTimerHwnd = hWnd;
    g_downloadDlgKillTimerId = timerId;
    return TRUE;
}

BOOL WINAPI FakeDownloadDlgDestroyWindow(HWND hWnd)
{
    if (g_downloadDlgDestroyWindowCalls < 4)
    {
        g_downloadDlgDestroyWindowHwnd[g_downloadDlgDestroyWindowCalls] = hWnd;
    }
    ++g_downloadDlgDestroyWindowCalls;
    return TRUE;
}

BOOL WINAPI FakeDownloadDlgEndDialog(HWND hWnd, INT_PTR result)
{
    ++g_downloadDlgEndDialogCalls;
    g_downloadDlgEndDialogHwnd = hWnd;
    g_downloadDlgEndDialogResult = result;
    return TRUE;
}

unsigned int RECOIL_CDECL FakeDownloadReadyFormatMessage(char *outBuffer,
                                                         int maxChars,
                                                         unsigned int messageId,
                                                         int ordinal,
                                                         int total)
{
    if (g_downloadReadyFormatCalls < 8)
    {
        g_downloadReadyFormatBuffer[g_downloadReadyFormatCalls] = outBuffer;
        g_downloadReadyFormatMaxChars[g_downloadReadyFormatCalls] = maxChars;
        g_downloadReadyFormatMessageId[g_downloadReadyFormatCalls] = messageId;
        g_downloadReadyFormatOrdinal[g_downloadReadyFormatCalls] = ordinal;
        g_downloadReadyFormatTotal[g_downloadReadyFormatCalls] = total;
    }
    ++g_downloadReadyFormatCalls;
    wsprintfA(outBuffer, "download %d of %d", ordinal, total);
    return lstrlenA(outBuffer);
}

INT_PTR WINAPI FakeDownloadReadyDialogBoxParamA(HINSTANCE instance,
                                                LPCSTR templateName,
                                                HWND parent,
                                                DLGPROC dialogProc,
                                                LPARAM initParam)
{
    const int index = g_downloadReadyDialogCalls;
    if (index < 8)
    {
        g_downloadReadyDialogInstance[index] = instance;
        g_downloadReadyDialogTemplate[index] = templateName;
        g_downloadReadyDialogParent[index] = parent;
        g_downloadReadyDialogProc[index] = dialogProc;
        g_downloadReadyDialogParam[index] = initParam;
    }
    ++g_downloadReadyDialogCalls;
    if (index < 8)
    {
        return g_downloadReadyDialogResult[index];
    }
    return 1;
}

int RECOIL_FASTCALL FakeDownloadReadyCallbackMessageBoxA(CWnd *self,
                                                         void *,
                                                         LPCSTR text,
                                                         LPCSTR caption,
                                                         UINT type)
{
    const int index = g_downloadReadyCallbackMessageBoxCalls;
    ++g_downloadReadyCallbackMessageBoxCalls;
    g_downloadReadyCallbackMessageBoxThis = self;
    strcpy(g_downloadReadyCallbackMessageBoxText, text);
    strcpy(g_downloadReadyCallbackMessageBoxCaption, caption);
    g_downloadReadyCallbackMessageBoxType = type;
    if (index < 4)
    {
        return g_downloadReadyCallbackMessageBoxResult[index];
    }
    return IDYES;
}

BOOL WINAPI FakeDownloadReadyCallbackSetEvent(HANDLE eventHandle)
{
    if (g_downloadReadyCallbackSetEventCalls < 4)
    {
        g_downloadReadyCallbackSetEventHandle
            [g_downloadReadyCallbackSetEventCalls] = eventHandle;
    }
    ++g_downloadReadyCallbackSetEventCalls;
    return TRUE;
}

int RECOIL_FASTCALL FakeDownloadReadyCallbackShowList(
    WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList)
{
    ++g_downloadReadyCallbackShowListCalls;
    g_downloadReadyCallbackShowList = downloadReadyList;
    return g_downloadReadyCallbackShowListResult;
}

void ResetPendingSessionRemovedFakes(void)
{
    g_pendingRemovedFormatCalls = 0;
    g_pendingRemovedFormatBuffer = 0;
    g_pendingRemovedFormatMaxChars = 0;
    g_pendingRemovedFormatMessageId = 0;
    g_pendingRemovedFormatSessionName = 0;
    g_pendingRemovedAppendCalls = 0;
    g_pendingRemovedAppendThis = 0;
    g_pendingRemovedAppendText[0] = '\0';
    g_pendingRemovedSendMessageCalls = 0;
    for (int index = 0; index < 12; ++index)
    {
        g_pendingRemovedSendMessageThis[index] = 0;
        g_pendingRemovedSendMessageControlId[index] = 0;
        g_pendingRemovedSendMessageMessage[index] = 0;
        g_pendingRemovedSendMessageWParam[index] = 0;
        g_pendingRemovedSendMessageLParam[index] = 0;
        g_pendingRemovedSendMessageResult[index] = LB_ERR;
        g_pendingRemovedSendMessageText[index][0] = '\0';
    }
}

unsigned int RECOIL_CDECL FakePendingSessionRemovedFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName)
{
    ++g_pendingRemovedFormatCalls;
    g_pendingRemovedFormatBuffer = outBuffer;
    g_pendingRemovedFormatMaxChars = maxChars;
    g_pendingRemovedFormatMessageId = messageId;
    g_pendingRemovedFormatSessionName = sessionName;
    strcpy(outBuffer, "pending removed status");
    return lstrlenA(outBuffer);
}

int RECOIL_CDECL FakePendingSessionRemovedAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *format,
    ...)
{
    ++g_pendingRemovedAppendCalls;
    g_pendingRemovedAppendThis = self;
    strcpy(g_pendingRemovedAppendText, format);
    return 1;
}

LRESULT RECOIL_FASTCALL FakePendingSessionRemovedSendDlgItemMessageA(
    CWnd *self,
    void *,
    int controlId,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    const int index = g_pendingRemovedSendMessageCalls;
    if (index < 12)
    {
        g_pendingRemovedSendMessageThis[index] = self;
        g_pendingRemovedSendMessageControlId[index] = controlId;
        g_pendingRemovedSendMessageMessage[index] = message;
        g_pendingRemovedSendMessageWParam[index] = wParam;
        g_pendingRemovedSendMessageLParam[index] = lParam;
        if (lParam != 0 &&
            (message == LB_ADDSTRING ||
             message == LB_INSERTSTRING ||
             message == LB_FINDSTRINGEXACT))
        {
            strcpy(g_pendingRemovedSendMessageText[index], (const char *)lParam);
        }
    }
    ++g_pendingRemovedSendMessageCalls;
    if (index < 12)
    {
        return g_pendingRemovedSendMessageResult[index];
    }
    return LB_ERR;
}

int RECOIL_FASTCALL FakeServerErrorMessageBoxA(CWnd *self,
                                               void *,
                                               LPCSTR text,
                                               LPCSTR caption,
                                               UINT type)
{
    ++g_serverErrorMessageBoxCalls;
    g_serverErrorMessageBoxThis = self;
    strcpy(g_serverErrorMessageBoxText, text);
    strcpy(g_serverErrorMessageBoxCaption, caption);
    g_serverErrorMessageBoxType = type;
    return IDOK;
}

void ResetApiStatusFakes(void)
{
    g_apiStatusTimeResetCalls = 0;
    g_apiStatusAppendCalls = 0;
    for (int index = 0; index < 8; ++index)
    {
        g_apiStatusAppendThis[index] = 0;
        g_apiStatusAppendText[index][0] = '\0';
    }
    g_apiStatusSetQueryModeCalls = 0;
    g_apiStatusSetQueryModeSelf = 0;
    g_apiStatusSetQueryModeValue = 0;
    g_apiStatusGetQueryResultCountCalls = 0;
    g_apiStatusGetQueryResultCountSelf = 0;
    g_apiStatusGetQueryResultCountOut = 0;
    g_apiStatusFormatCalls = 0;
    g_apiStatusFormatBuffer = 0;
    g_apiStatusFormatMaxChars = 0;
    g_apiStatusFormatMessageId = 0;
    g_apiStatusFormatResultCount = 0;
    g_initMessageIdCalls = 0;
    g_downloadReadyCallbackSetEventCalls = 0;
    g_initDestroyProgressCalls = 0;
    g_initDestroyedProgress = 0;
    g_serverErrorMessageBoxCalls = 0;
    g_serverErrorMessageBoxThis = 0;
    g_serverErrorMessageBoxText[0] = '\0';
    g_serverErrorMessageBoxCaption[0] = '\0';
    g_serverErrorMessageBoxType = 0;
}

void ResetBrowseRecordAddedFakes(void)
{
    g_initMessageIdCalls = 0;
    g_apiStatusAppendCalls = 0;
    for (int index = 0; index < 8; ++index)
    {
        g_apiStatusAppendThis[index] = 0;
        g_apiStatusAppendText[index][0] = '\0';
    }
    g_browseRecordAddedFormatCalls = 0;
    g_browseRecordAddedFormatBuffer = 0;
    g_browseRecordAddedFormatMaxChars = 0;
    g_browseRecordAddedFormatMessageId = 0;
    g_browseRecordAddedFormatSessionName = 0;
    g_browseRecordAddedEnableCalls = 0;
    for (int index = 0; index < 4; ++index)
    {
        g_browseRecordAddedEnableThis[index] = 0;
        g_browseRecordAddedEnableValue[index] = 0;
    }
    g_initRequestListModeCalls = 0;
    g_initRequestListMode = 0;
    g_initRequestListModeEnabled = 0;
    g_initSetDlgItemTextCalls = 0;
    for (int index = 0; index < 4; ++index)
    {
        g_initSetDlgItemTextThis[index] = 0;
        g_initSetDlgItemTextControlId[index] = 0;
        g_initSetDlgItemTextValue[index] = 0;
    }
    ResetPendingSessionRemovedFakes();
}

void ResetBrowseResolvedFakes(void)
{
    ResetBrowseRecordAddedFakes();
    g_browseResolvedAppendCalls = 0;
    for (int index = 0; index < 8; ++index)
    {
        g_browseResolvedAppendThis[index] = 0;
        g_browseResolvedAppendFormat[index][0] = '\0';
        g_browseResolvedAppendArgText[index] = 0;
        g_browseResolvedAppendArgStatus[index] = 0;
    }
    g_browseResolvedFormatCalls = 0;
    g_browseResolvedFormatBuffer = 0;
    g_browseResolvedFormatMaxChars = 0;
    g_browseResolvedFormatMessageId = 0;
    g_browseResolvedFormatSessionName = 0;
    g_browseResolvedFormatBrowseName = 0;
    g_browseResolvedUpdateCalls = 0;
    g_browseResolvedUpdateThis = 0;
    g_browseResolvedConnectCalls = 0;
    for (int index = 0; index < 4; ++index)
    {
        g_browseResolvedConnectThis[index] = 0;
        g_browseResolvedConnectValue[index] = 0;
    }
    g_browseResolvedRequestDetailsCalls = 0;
    g_browseResolvedRequestDetailsSelf = 0;
    g_browseResolvedRequestDetailsRequest = 0;
    g_sessionFinishedCancelCalls = 0;
    g_sessionFinishedCancelSelf = 0;
    g_sessionFinishedAppendConnectCalls = 0;
    g_sessionFinishedAppendConnectThis = 0;
    g_sessionFinishedAppendConnectSessionName = 0;
}

void RECOIL_CDECL FakeApiStatusTimeReset(void)
{
    ++g_apiStatusTimeResetCalls;
}

int RECOIL_CDECL FakeApiStatusAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *format,
    ...)
{
    const int index = g_apiStatusAppendCalls;
    if (index < 8)
    {
        g_apiStatusAppendThis[index] = self;
        strcpy(g_apiStatusAppendText[index], format);
    }
    ++g_apiStatusAppendCalls;
    return 1;
}

unsigned int RECOIL_CDECL FakeApiStatusFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    int resultCount)
{
    ++g_apiStatusFormatCalls;
    g_apiStatusFormatBuffer = outBuffer;
    g_apiStatusFormatMaxChars = maxChars;
    g_apiStatusFormatMessageId = messageId;
    g_apiStatusFormatResultCount = resultCount;
    wsprintfA(outBuffer, "result count %d", resultCount);
    return lstrlenA(outBuffer);
}

unsigned int RECOIL_CDECL FakeBrowseRecordAddedFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName)
{
    ++g_browseRecordAddedFormatCalls;
    g_browseRecordAddedFormatBuffer = outBuffer;
    g_browseRecordAddedFormatMaxChars = maxChars;
    g_browseRecordAddedFormatMessageId = messageId;
    g_browseRecordAddedFormatSessionName = sessionName;
    wsprintfA(outBuffer, "browse %04x %s", messageId, sessionName);
    return lstrlenA(outBuffer);
}

void RECOIL_FASTCALL FakeBrowseRecordAddedEnableQueryControls(
    WestwoodOnlineUpgradeDialog *self,
    void *,
    int enable)
{
    const int index = g_browseRecordAddedEnableCalls;
    if (index < 4)
    {
        g_browseRecordAddedEnableThis[index] = self;
        g_browseRecordAddedEnableValue[index] = enable;
    }
    ++g_browseRecordAddedEnableCalls;
}

int RECOIL_CDECL FakeBrowseResolvedAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *format,
    ...)
{
    const int index = g_browseResolvedAppendCalls;
    if (index < 8)
    {
        g_browseResolvedAppendThis[index] = self;
        strcpy(g_browseResolvedAppendFormat[index], format);
        if (strcmp(format, "%s %x") == 0)
        {
            va_list args;
            va_start(args, format);
            g_browseResolvedAppendArgText[index] = va_arg(args, const char *);
            g_browseResolvedAppendArgStatus[index] = va_arg(args, int);
            va_end(args);
        }
    }
    ++g_browseResolvedAppendCalls;
    return 1;
}

unsigned int RECOIL_CDECL FakeBrowseResolvedFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName,
    const char *browseName)
{
    ++g_browseResolvedFormatCalls;
    g_browseResolvedFormatBuffer = outBuffer;
    g_browseResolvedFormatMaxChars = maxChars;
    g_browseResolvedFormatMessageId = messageId;
    g_browseResolvedFormatSessionName = sessionName;
    g_browseResolvedFormatBrowseName = browseName;
    wsprintfA(outBuffer, "resolved %04x %s %s", messageId, sessionName,
              browseName);
    return lstrlenA(outBuffer);
}

void RECOIL_FASTCALL FakeBrowseResolvedUpdateSessionListQueryFromControls(
    WestwoodOnlineUpgradeDialog *self,
    void *)
{
    ++g_browseResolvedUpdateCalls;
    g_browseResolvedUpdateThis = self;
}

void RECOIL_FASTCALL FakeBrowseResolvedEnableConnectButton(
    WestwoodOnlineUpgradeDialog *self,
    void *,
    int enable)
{
    const int index = g_browseResolvedConnectCalls;
    if (index < 4)
    {
        g_browseResolvedConnectThis[index] = self;
        g_browseResolvedConnectValue[index] = enable;
    }
    ++g_browseResolvedConnectCalls;
}

void STDMETHODCALLTYPE FakeBrowseResolvedRequestSessionDetails(
    IUnknown *self,
    WestwoodOnlineUpgradeSessionRequest *request)
{
    ++g_browseResolvedRequestDetailsCalls;
    g_browseResolvedRequestDetailsSelf = self;
    g_browseResolvedRequestDetailsRequest = request;
}

void STDMETHODCALLTYPE FakeSessionFinishedCancelPendingSessionFlow(IUnknown *self)
{
    ++g_sessionFinishedCancelCalls;
    g_sessionFinishedCancelSelf = self;
}

void RECOIL_FASTCALL FakeSessionFinishedAppendConnectStatusAndRefreshList(
    WestwoodOnlineUpgradeDialog *self,
    void *,
    const char *sessionName)
{
    ++g_sessionFinishedAppendConnectCalls;
    g_sessionFinishedAppendConnectThis = self;
    g_sessionFinishedAppendConnectSessionName = sessionName;
}

void STDMETHODCALLTYPE FakeApiStatusSetQueryMode(IUnknown *self, int listMode)
{
    ++g_apiStatusSetQueryModeCalls;
    g_apiStatusSetQueryModeSelf = self;
    g_apiStatusSetQueryModeValue = listMode;
}

void STDMETHODCALLTYPE FakeApiStatusGetQueryResultCount(IUnknown *self,
                                                        int *outCount)
{
    ++g_apiStatusGetQueryResultCountCalls;
    g_apiStatusGetQueryResultCountSelf = self;
    g_apiStatusGetQueryResultCountOut = outCount;
    *outCount = g_apiStatusQueryResultCount;
}

int RECOIL_CDECL FakeApiCreateShowModalAndApplySelectedProfileValues()
{
    ++g_apiCreateShowModalCalls;
    return g_apiCreateShowModalResult;
}

int RECOIL_FASTCALL FakeInitCreateInstanceAndLoadConfig(
    WestwoodOnlineUpgradeApi *,
    void *,
    HANDLE bootstrapServerListEvent)
{
    ++g_initCreateInstanceCalls;
    g_initCreateInstanceBootstrap = bootstrapServerListEvent;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;
    return 1;
}

HANDLE WINAPI FakeInitCreateEventA(LPSECURITY_ATTRIBUTES, BOOL, BOOL, LPCSTR)
{
    HANDLE result = 0;
    if (g_initCreateEventCalls < 3)
    {
        result = g_initCreatedEvents[g_initCreateEventCalls];
    }
    ++g_initCreateEventCalls;
    return result;
}

BOOL RECOIL_FASTCALL FakeInitCreateProgress(CDialog *, void *, LPCSTR resourceName,
                                            CWnd *parentWnd)
{
    ++g_initCreateProgressCalls;
    g_initCreateProgressResource = resourceName;
    g_initCreateProgressParent = parentWnd;
    return TRUE;
}

void RECOIL_FASTCALL FakeInitSetDlgItemTextA(CWnd *self, void *, int controlId,
                                             LPCSTR text)
{
    if (g_initSetDlgItemTextCalls < 4)
    {
        g_initSetDlgItemTextThis[g_initSetDlgItemTextCalls] = self;
        g_initSetDlgItemTextControlId[g_initSetDlgItemTextCalls] = controlId;
        g_initSetDlgItemTextValue[g_initSetDlgItemTextCalls] = text;
    }
    ++g_initSetDlgItemTextCalls;
}

char *RECOIL_FASTCALL FakeInitGetMessageString(unsigned int messageId)
{
    static char messages[16][32];
    if (g_initMessageIdCalls < 16)
    {
        g_initMessageIds[g_initMessageIdCalls] = messageId;
        wsprintfA(messages[g_initMessageIdCalls], "msg-%04x", messageId);
        return messages[g_initMessageIdCalls++];
    }

    return (char *)"msg-overflow";
}

int RECOIL_FASTCALL FakeInitDialogBaseOnInitDialog(CDialog *self, void *)
{
    ++g_initDialogBaseOnInitCalls;
    g_initDialogBaseOnInitThis = self;
    return 1;
}

int RECOIL_CDECL FakeInitDialogApiInit()
{
    ++g_initDialogApiInitCalls;
    return g_initDialogApiInitResult;
}

void RECOIL_FASTCALL FakeInitDialogOnDestroy(
    WestwoodOnlineUpgradeDialog *self,
    void *)
{
    ++g_initDialogOnDestroyCalls;
    g_initDialogOnDestroyThis = self;
}

void RECOIL_FASTCALL FakeInitDialogSetAbortAndClose(
    WestwoodOnlineUpgradeDialog *self,
    void *)
{
    ++g_initDialogSetAbortCalls;
    g_initDialogSetAbortThis = self;
}

UINT_PTR WINAPI FakeInitDialogSetTimer(HWND hwnd,
                                       UINT_PTR timerId,
                                       UINT elapsedMs,
                                       TIMERPROC timerProc)
{
    ++g_initDialogSetTimerCalls;
    g_initDialogSetTimerHwnd = hwnd;
    g_initDialogSetTimerId = timerId;
    g_initDialogSetTimerMs = elapsedMs;
    g_initDialogSetTimerProc = timerProc;
    return timerId;
}

LRESULT WINAPI FakeInitDialogSendMessageA(HWND hwnd,
                                          UINT msg,
                                          WPARAM wParam,
                                          LPARAM lParam)
{
    int const index = g_initDialogSendMessageCalls;
    if (index < 24)
    {
        g_initDialogSendMessageHwnd[index] = hwnd;
        g_initDialogSendMessageMsg[index] = msg;
        g_initDialogSendMessageWParam[index] = wParam;
        g_initDialogSendMessageLParam[index] = lParam;
    }
    ++g_initDialogSendMessageCalls;

    if (msg == CB_ADDSTRING)
    {
        return g_initDialogComboAddCalls++;
    }
    return 0;
}

void STDMETHODCALLTYPE FakeInitProcessCallbacks(IUnknown *)
{
    ++g_initProcessCallbacksCalls;
}

void STDMETHODCALLTYPE FakeInitBeginConnect(IUnknown *, int languageId, int productId,
                                            const char *playerName,
                                            const char *connectString,
                                            int timeoutSeconds)
{
    ++g_initBeginConnectCalls;
    g_initBeginConnectLanguageId = languageId;
    g_initBeginConnectProductId = productId;
    g_initBeginConnectPlayerName = playerName;
    g_initBeginConnectConnectString = connectString;
    g_initBeginConnectTimeoutSeconds = timeoutSeconds;
}

void STDMETHODCALLTYPE FakeInitRequestBootstrapServerList(
    IUnknown *,
    WestwoodOnlineUpgradeBootstrapServerRecord *selectedBootstrapServer,
    int timeoutSeconds,
    int useAlternateConnectString)
{
    ++g_initRequestBootstrapCalls;
    g_initRequestBootstrapServer = selectedBootstrapServer;
    g_initRequestBootstrapTimeoutSeconds = timeoutSeconds;
    g_initRequestBootstrapUseAlternate = useAlternateConnectString;
}

void STDMETHODCALLTYPE FakeInitRequestListMode(IUnknown *, int listMode, int enabled)
{
    ++g_initRequestListModeCalls;
    g_initRequestListMode = listMode;
    g_initRequestListModeEnabled = enabled;
}

void STDMETHODCALLTYPE FakeResetQueryState(IUnknown *)
{
    ++g_resetQueryStateCalls;
}

void STDMETHODCALLTYPE FakeInitDisconnect(IUnknown *)
{
    ++g_initDisconnectCalls;
}

int RECOIL_FASTCALL FakeBeginConnectGetWindowTextA(CWnd *self, void *,
                                                   char *buffer, int maxCount)
{
    ++g_beginConnectGetWindowTextCalls;
    g_beginConnectGetWindowTextThis = self;
    g_beginConnectGetWindowTextBuffer = buffer;
    g_beginConnectGetWindowTextMaxCount = maxCount;
    strcpy(buffer, "SrvMode");
    return 7;
}

int STDMETHODCALLTYPE FakeBeginConnectPrepareContext(
    IUnknown *,
    WestwoodOnlineUpgradeConnectContext *context)
{
    ++g_beginConnectPrepareCalls;
    g_beginConnectPrepareContext = context;
    return g_beginConnectPrepareResult;
}

void STDMETHODCALLTYPE FakeBeginConnectWithPreparedContext(
    IUnknown *,
    WestwoodOnlineUpgradeConnectContext *context,
    int mode)
{
    ++g_beginConnectPreparedCalls;
    g_beginConnectPreparedContext = context;
    g_beginConnectPreparedMode = mode;
}

int STDMETHODCALLTYPE FakeCheckAndApplyUpgradeResult(
    IUnknown *,
    WestwoodOnlineUpgradeConnectContext *context)
{
    ++g_checkAndApplyUpgradeCalls;
    g_checkAndApplyUpgradeContext = context;
    return g_checkAndApplyUpgradeResult;
}

int RECOIL_FASTCALL FakeQueryStatusGetWindowTextA(CWnd *self, void *,
                                                  char *buffer, int maxCount)
{
    int const index = g_queryStatusGetWindowTextCalls;
    if (index < 2)
    {
        g_queryStatusGetWindowTextThis[index] = self;
        g_queryStatusGetWindowTextMaxCount[index] = maxCount;
    }
    ++g_queryStatusGetWindowTextCalls;

    if (maxCount == 19)
    {
        strcpy(buffer, g_queryStatusTokenInput);
    }
    else
    {
        strcpy(buffer, g_queryStatusServerInput);
    }

    return (int)strlen(buffer);
}

int RECOIL_FASTCALL FakeWestwoodUpdateData(CWnd *, void *, BOOL saveAndValidate)
{
    if (g_threeFloatUpdateDataCount < 8)
    {
        g_threeFloatUpdateDataSaveValue[g_threeFloatUpdateDataCount] =
            saveAndValidate;
    }
    ++g_threeFloatUpdateDataCount;
    return 1;
}

long RECOIL_FASTCALL FakeWestwoodDefault(CWnd *, void *)
{
    ++g_threeFloatDefaultCount;
    return g_threeFloatDefaultReturn;
}

void RECOIL_FASTCALL FakeQueryStatusSetWindowTextA(CWnd *self, void *,
                                                   const char *text)
{
    int const index = g_queryStatusSetWindowTextCalls;
    if (index < 4)
    {
        g_queryStatusSetWindowTextThis[index] = (void *)self->m_hWnd;
        strcpy(g_queryStatusSetWindowTextValue[index], text);
    }
    ++g_queryStatusSetWindowTextCalls;
}

int STDMETHODCALLTYPE FakeQueryStatusWithTokenAndServer(
    IUnknown *,
    WestwoodOnlineUpgradeConnectContext *context,
    const char *serverText)
{
    ++g_queryStatusProviderCalls;
    g_queryStatusProviderContext = context;
    g_queryStatusProviderServerText = serverText;
    return g_queryStatusProviderResult;
}

int RECOIL_FASTCALL FakeQueryStatusMessageBoxA(CWnd *self, void *,
                                               const char *text,
                                               const char *caption,
                                               unsigned int type)
{
    ++g_queryStatusMessageBoxCalls;
    g_queryStatusMessageBoxThis = self;
    strcpy(g_queryStatusMessageBoxText, text);
    strcpy(g_queryStatusMessageBoxCaption, caption);
    g_queryStatusMessageBoxType = type;
    return g_queryStatusMessageBoxResult;
}

void RECOIL_FASTCALL FakeRefreshCurrentQueryGetWindowTextA(CWnd *self, void *,
                                                           CString *text)
{
    ++g_refreshCurrentQueryGetWindowTextCalls;
    g_refreshCurrentQueryGetWindowTextThis = self;
    *text = g_refreshCurrentQuerySessionNameInput;
}

int STDMETHODCALLTYPE FakeRefreshCurrentQuerySubmit(
    IUnknown *,
    WestwoodOnlineUpgradeQueryRequest *request)
{
    ++g_refreshCurrentQuerySubmitCalls;
    g_refreshCurrentQuerySubmitRequest = *request;
    return g_refreshCurrentQuerySubmitResult;
}

void STDMETHODCALLTYPE FakeSubmitEncodedQueryString(
    IUnknown *self,
    const char *encodedQuery)
{
    ++g_submitEncodedQueryCalls;
    g_submitEncodedQuerySelf = self;
    strcpy(g_submitEncodedQueryText, encodedQuery);
}

void STDMETHODCALLTYPE FakeSubmitPendingSessionList(
    IUnknown *self,
    WestwoodOnlineUpgradeSessionRequest *sessionRequestList)
{
    ++g_submitPendingSessionListCalls;
    g_submitPendingSessionListSelf = self;
    g_submitPendingSessionListCount = 0;
    while (sessionRequestList != 0 &&
           g_submitPendingSessionListCount < 8)
    {
        strcpy(g_submitPendingSessionListNames[g_submitPendingSessionListCount],
               sessionRequestList->m_sessionName);
        sessionRequestList = sessionRequestList->m_next;
        ++g_submitPendingSessionListCount;
    }
}

void RECOIL_FASTCALL FakeSubmitVisibleGetWindowTextA(CWnd *self, void *,
                                                     CString *text)
{
    ++g_submitVisibleGetWindowTextCalls;
    g_submitVisibleGetWindowTextThis = self;
    *text = g_submitVisibleStatusInput;
}

void STDMETHODCALLTYPE FakeSubmitVisibleSubmitStatusText(
    IUnknown *self,
    const char *statusText)
{
    ++g_submitVisibleSubmitStatusCalls;
    g_submitVisibleSubmitStatusSelf = self;
    strcpy(g_submitVisibleSubmitStatusText, statusText);
}

void STDMETHODCALLTYPE FakeSubmitVisibleSubmitSessionRequestListAndStatusText(
    IUnknown *self,
    WestwoodOnlineUpgradeSessionRequest *sessionRequestList,
    const char *statusText)
{
    ++g_submitVisibleSubmitListCalls;
    g_submitVisibleSubmitListSelf = self;
    strcpy(g_submitVisibleSubmitListStatusText, statusText);
    g_submitVisibleSubmitListCount = 0;
    while (sessionRequestList != 0 &&
           g_submitVisibleSubmitListCount < 8)
    {
        strcpy(g_submitVisibleSubmitListNames[g_submitVisibleSubmitListCount],
               sessionRequestList->m_sessionName);
        sessionRequestList = sessionRequestList->m_next;
        ++g_submitVisibleSubmitListCount;
    }
}

unsigned int RECOIL_CDECL FakeSubmitVisibleFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName,
    const char *statusText)
{
    ++g_submitVisibleFormatCalls;
    g_submitVisibleFormatBuffer = outBuffer;
    g_submitVisibleFormatMaxChars = maxChars;
    g_submitVisibleFormatMessageId = messageId;
    g_submitVisibleFormatSessionName = sessionName;
    g_submitVisibleFormatStatusText = statusText;
    strcpy(outBuffer, "formatted visible status");
    return 24;
}

int RECOIL_CDECL FakeSubmitVisibleAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *format,
    ...)
{
    ++g_submitVisibleAppendCalls;
    g_submitVisibleAppendThis = self;
    strcpy(g_submitVisibleAppendFormat, format);
    g_submitVisibleAppendArg0[0] = '\0';
    g_submitVisibleAppendArg1[0] = '\0';

    va_list args;
    va_start(args, format);
    if (strcmp(format, "{ %s } %s") == 0)
    {
        strcpy(g_submitVisibleAppendArg0, va_arg(args, const char *));
        strcpy(g_submitVisibleAppendArg1, va_arg(args, const char *));
    }
    va_end(args);
    return 1;
}

void STDMETHODCALLTYPE FakeQueueSessionRequest(
    IUnknown *self,
    WestwoodOnlineUpgradeSessionRequest *request)
{
    if (g_queueSessionRequestCalls < 8)
    {
        g_queueSessionRequestSelf[g_queueSessionRequestCalls] = self;
        g_queueSessionRequestCopies[g_queueSessionRequestCalls] = *request;
    }
    ++g_queueSessionRequestCalls;
}

void STDMETHODCALLTYPE FakeLookupBrowseRecordBySessionName(
    IUnknown *self,
    const char *sessionName,
    int lookupMode)
{
    if (g_lookupBrowseRecordCalls < 8)
    {
        g_lookupBrowseRecordSelf[g_lookupBrowseRecordCalls] = self;
        strcpy(g_lookupBrowseRecordSessionName[g_lookupBrowseRecordCalls],
               sessionName);
        g_lookupBrowseRecordMode[g_lookupBrowseRecordCalls] = lookupMode;
    }
    ++g_lookupBrowseRecordCalls;
}

int STDMETHODCALLTYPE FakeLoadBrowseRecord(
    IUnknown *self,
    WestwoodOnlineUpgradeBrowseRecord *record)
{
    ++g_loadBrowseRecordCalls;
    g_loadBrowseRecordSelf = self;
    g_loadBrowseRecordRecord = record;
    g_loadBrowseRecordCopy = *record;
    return g_loadBrowseRecordResult;
}

void STDMETHODCALLTYPE FakeDestroyProcessCallbacks(IUnknown *)
{
    ++g_initProcessCallbacksCalls;
    if (g_initProcessCallbacksCalls >= 2)
    {
        g_WestwoodOnlineUpgradeAbortFlag = 1;
    }
}

int RECOIL_FASTCALL FakeInitDestroyProgress(CWnd *self, void *)
{
    ++g_initDestroyProgressCalls;
    g_initDestroyedProgress = self;
    return 1;
}

void RECOIL_FASTCALL FakeDestroyBeginDisconnect(WestwoodOnlineUpgradeDialog *self,
                                                void *)
{
    ++g_destroyBeginDisconnectCalls;
    g_destroyBeginDisconnectThis = self;
}

void RECOIL_CDECL FakeDestroyApiShutdown()
{
    ++g_destroyShutdownCalls;
}

BOOL WINAPI FakeDestroyKillTimer(HWND hwnd, UINT_PTR timerId)
{
    ++g_destroyKillTimerCalls;
    g_destroyKillTimerHwnd = hwnd;
    g_destroyKillTimerId = timerId;
    return TRUE;
}

DWORD WINAPI FakeInitWaitForMultipleObjects(DWORD count, const HANDLE *handles,
                                            BOOL waitAll, DWORD timeout)
{
    if (count != 3 || waitAll != FALSE)
    {
        return WAIT_FAILED;
    }

    g_initWaitHandles = (HANDLE *)handles;
    if (g_initWaitCalls < 4)
    {
        g_initWaitTimeouts[g_initWaitCalls] = timeout;
    }

    if (g_initWaitCalls < g_initWaitResultCount)
    {
        return g_initWaitResults[g_initWaitCalls++];
    }

    ++g_initWaitCalls;
    return WAIT_OBJECT_0;
}

BOOL WINAPI FakeInitResetEvent(HANDLE eventHandle)
{
    ++g_initResetEventCalls;
    g_initResetEventHandle = eventHandle;
    return TRUE;
}

BOOL WINAPI FakeBootstrapSetEvent(HANDLE eventHandle)
{
    if (g_bootstrapSetEventCalls < 4)
    {
        g_bootstrapSetEventHandles[g_bootstrapSetEventCalls] = eventHandle;
    }
    ++g_bootstrapSetEventCalls;
    return TRUE;
}

VOID WINAPI FakeInitSleep(DWORD duration)
{
    if (g_initSleepCalls < 4)
    {
        g_initSleepDurations[g_initSleepCalls] = duration;
    }
    ++g_initSleepCalls;
}

LANGID WINAPI FakeInitGetSystemDefaultLangID()
{
    return MAKELANGID(LANG_GERMAN, SUBLANG_DEFAULT);
}

int RECOIL_FASTCALL FakeWolEnableWindow(CWnd *self, void *, int enable)
{
    if (g_enableWindowCalls < 12)
    {
        g_enableWindowThis[g_enableWindowCalls] = self;
        g_enableWindowEnable[g_enableWindowCalls] = enable;
    }
    ++g_enableWindowCalls;
    return enable == 0 ? 0 : 1;
}

LRESULT WINAPI FakeResetSendMessageA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    ++g_resetSendMessageCalls;
    g_resetSendMessageHwnd = hwnd;
    g_resetSendMessageMsg = msg;
    g_resetSendMessageWParam = wParam;
    g_resetSendMessageLParam = lParam;
    return 0;
}

unsigned int RECOIL_CDECL FakeAppendConnectFormatMessage(char *outBuffer, int maxChars,
                                                         unsigned int messageId,
                                                         const char *sessionName)
{
    ++g_appendConnectFormatCalls;
    g_appendConnectFormatBuffer = outBuffer;
    g_appendConnectFormatMaxChars = maxChars;
    g_appendConnectFormatMessageId = messageId;
    g_appendConnectFormatSessionName = sessionName;
    strcpy(outBuffer, "formatted connect status");
    return 24;
}

int RECOIL_CDECL FakeAppendConnectAppendStatusTextFmt(WestwoodOnlineUpgradeDialog *self,
                                                      const char *text)
{
    ++g_appendConnectAppendCalls;
    g_appendConnectAppendThis = self;
    g_appendConnectAppendText = text;
    return 1;
}

void RECOIL_FASTCALL FakeAppendConnectResetSelectedBrowseRecord(
    WestwoodOnlineUpgradeDialog *self,
    void *)
{
    ++g_appendConnectResetCalls;
    g_appendConnectResetThis = self;
}

void RECOIL_FASTCALL FakeAbortOnCancel(CDialog *self, void *)
{
    ++g_abortOnCancelCalls;
    g_abortOnCancelThis = self;
}

ShutdownFakeUnknownVtable g_shutdownUnknownVtable = {
    ShutdownUnknownQueryInterface,
    ShutdownUnknownAddRef,
    ShutdownUnknownRelease,
};

ShutdownFakeConnectionPointContainerVtable g_shutdownCpcVtable = {
    ShutdownCpcQueryInterface,
    ShutdownCpcAddRef,
    ShutdownCpcRelease,
    ShutdownEnumConnectionPoints,
    ShutdownFindConnectionPoint,
};

ShutdownFakeConnectionPointVtable g_shutdownConnectionPointVtable = {
    ShutdownCpQueryInterface,
    ShutdownCpAddRef,
    ShutdownCpRelease,
    ShutdownGetConnectionInterface,
    ShutdownGetConnectionPointContainer,
    ShutdownAdvise,
    ShutdownUnadvise,
    ShutdownEnumConnections,
};

WestwoodOnlineUpgradeDownloadComVtable g_downloadDlgFakeVtable = {
    DownloadDlgQueryInterface,
    DownloadDlgAddRef,
    DownloadDlgRelease,
    DownloadDlgBeginDownload,
    DownloadDlgAbort,
    DownloadDlgPump,
};

bool PatchImportByOrdinal(const char *dllName, WORD ordinal, void *replacement,
                          ImportFunctionPatch &patch)
{
    unsigned char *const imageBase = (unsigned char *)GetModuleHandleA(0);
    IMAGE_DOS_HEADER *const dos = (IMAGE_DOS_HEADER *)imageBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = (IMAGE_NT_HEADERS *)(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        (IMAGE_IMPORT_DESCRIPTOR *)(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll = (const char *)(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(
            imageBase + (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                             : descriptor->FirstThunk));
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (!IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal) ||
                (WORD)(nameThunk->u1.Ordinal & 0xffff) != ordinal) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                return false;
            }

            *patch.slot = (ULONG_PTR)(unsigned long long)replacement;
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

bool PatchImportByName(const char *dllName, const char *functionName, void *replacement,
                       ImportFunctionPatch &patch)
{
    unsigned char *const imageBase = (unsigned char *)GetModuleHandleA(0);
    IMAGE_DOS_HEADER *const dos = (IMAGE_DOS_HEADER *)imageBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = (IMAGE_NT_HEADERS *)(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        (IMAGE_IMPORT_DESCRIPTOR *)(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll = (const char *)(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(imageBase +
            (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                 : descriptor->FirstThunk));
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)) {
                continue;
            }

            IMAGE_IMPORT_BY_NAME *importName =
                (IMAGE_IMPORT_BY_NAME *)(imageBase + nameThunk->u1.AddressOfData);
            if (strcmp((const char *)importName->Name, functionName) != 0) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                               &oldProtect) == 0) {
                return false;
            }

            *patch.slot = (ULONG_PTR)replacement;
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

void RestoreImportPatch(ImportFunctionPatch &patch)
{
    if (patch.slot == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.slot, sizeof(*patch.slot), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }

    patch.slot = 0;
    patch.original = 0;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch)
{
    if (target == 0) {
        patch.address = 0;
        return false;
    }

    patch.address = (unsigned char *)target;
    memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        patch.address = 0;
        return false;
    }

    patch.address[0] = 0xe9;
    const intptr_t relativeOffset =
        (intptr_t)replacement - (intptr_t)(patch.address + sizeof(patch.original));
    *(int *)(patch.address + 1) = (int)relativeOffset;

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch)
{
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = 0;
}

void ResetModalProbe()
{
    g_modalSelectedMissionIndex = -1;
    g_modalMenuStep = 0;
    g_modalMenuVisible[0] = -1;
    g_modalMenuVisible[1] = -1;
    g_modalMenuThis[0] = 0;
    g_modalMenuThis[1] = 0;
    g_modalDoModalCalls = 0;
    g_modalDialogDtorCalls = 0;
    g_modalCStringDtorCalls = 0;
    g_modalListDtorCalls = 0;
    g_modalEditDtorCalls = 0;
    g_modalComboDtorCalls = 0;
    g_modalButtonDtorCalls = 0;
    g_modalArgsOk = true;
    g_modalDtorSequenceCount = 0;
    memset(g_modalDtorSequence, 0, sizeof(g_modalDtorSequence));
}

void RecordModalDtorSequence(char code)
{
    if (g_modalDtorSequenceCount < (int)sizeof(g_modalDtorSequence))
    {
        g_modalDtorSequence[g_modalDtorSequenceCount] = code;
    }
    ++g_modalDtorSequenceCount;
}

void ResetModalDdxProbe()
{
    g_modalDdxStep = 0;
    memset(g_modalDdxContext, 0, sizeof(g_modalDdxContext));
    memset(g_modalDdxKind, 0, sizeof(g_modalDdxKind));
    memset(g_modalDdxControlId, 0, sizeof(g_modalDdxControlId));
    memset(g_modalDdxValue, 0, sizeof(g_modalDdxValue));
}

void RecordModalDdx(CDataExchange *dataExchange, int kind, int controlId,
                    void *value)
{
    const int index = g_modalDdxStep;
    if (index < (int)(sizeof(g_modalDdxKind) / sizeof(g_modalDdxKind[0])))
    {
        g_modalDdxContext[index] = dataExchange;
        g_modalDdxKind[index] = kind;
        g_modalDdxControlId[index] = controlId;
        g_modalDdxValue[index] = value;
    }
    ++g_modalDdxStep;
}

void RECOIL_FASTCALL FakeModalSetMenuBarVisibility(CZRecoilFrame *self, void *, int visible)
{
    if (g_modalMenuStep < 2) {
        g_modalMenuThis[g_modalMenuStep] = self;
        g_modalMenuVisible[g_modalMenuStep] = visible;
    }
    ++g_modalMenuStep;
}

int RECOIL_FASTCALL FakeModalDoModal(void *self, void *)
{
    ++g_modalDoModalCalls;
    g_modalArgsOk =
        g_modalArgsOk && self == g_pWestwoodOnlineUpgradeDialog &&
        g_pWestwoodOnlineUpgradeDialog != 0 &&
        g_pWestwoodOnlineUpgradeProgressDialog != 0 &&
        TestObjectVtable(g_pWestwoodOnlineUpgradeDialog) != 0 &&
        TestObjectVtable(g_pWestwoodOnlineUpgradeProgressDialog) != 0 &&
        g_WestwoodOnlineUpgradeSelectedMissionIndex == -1;
    g_WestwoodOnlineUpgradeSelectedMissionIndex = g_modalSelectedMissionIndex;
    return 1;
}

int RECOIL_FASTCALL FakeConfigDoModal(void *self, void *)
{
    ++g_configDoModalCalls;
    g_configDoModalThis = (WestwoodOnlineUpgradeConfigDialog *)self;
    g_configDoModalThis->m_selectedProfileIndex = 1;
    g_configDoModalThis->m_profilePlayerNames[1] = "DialogPilot";
    g_configDoModalThis->m_profileConnectStrings[1] = "DialogConnect";
    g_configDoModalThis->m_profileConnectStringModes[1] = 21;
    return g_configDoModalResult;
}

void RECOIL_FASTCALL FakeModalDialogDtor(void *, void *)
{
    ++g_modalDialogDtorCalls;
    RecordModalDtorSequence('D');
}

void RECOIL_FASTCALL FakeModalCStringDtor(void *, void *)
{
    ++g_modalCStringDtorCalls;
    RecordModalDtorSequence('S');
}

void RECOIL_FASTCALL FakeModalListDtor(void *, void *)
{
    ++g_modalListDtorCalls;
    RecordModalDtorSequence('L');
}

void RECOIL_FASTCALL FakeModalEditDtor(void *, void *)
{
    ++g_modalEditDtorCalls;
    RecordModalDtorSequence('E');
}

void RECOIL_FASTCALL FakeModalComboDtor(void *, void *)
{
    ++g_modalComboDtorCalls;
    RecordModalDtorSequence('C');
}

void RECOIL_FASTCALL FakeModalButtonDtor(void *, void *)
{
    ++g_modalButtonDtorCalls;
    RecordModalDtorSequence('B');
}

void RECOIL_STDCALL FakeModalDDXControl(CDataExchange *dataExchange,
                                        int controlId, void *control)
{
    RecordModalDdx(dataExchange, 1, controlId, control);
}

void RECOIL_STDCALL FakeModalDDXTextUInt(CDataExchange *dataExchange,
                                         int controlId, unsigned int *value)
{
    RecordModalDdx(dataExchange, 2, controlId, value);
}

void RECOIL_STDCALL FakeModalDDXCheck(CDataExchange *dataExchange,
                                      int controlId, int *value)
{
    RecordModalDdx(dataExchange, 3, controlId, value);
}

void RECOIL_CDECL FakeModalTimeTick()
{
    ++g_modalTimeTickCalls;
}

bool InstallModalPatches(ImportFunctionPatch *imports, CodeFunctionPatch &menuPatch)
{
    const WORD kMfc42CDialogDoModalOrdinal = 2514;
    const WORD kMfc42CStringDtorOrdinal = 800;
    const WORD kMfc42CComboBoxDtorOrdinal = 616;
    const WORD kMfc42CListBoxDtorOrdinal = 692;
    const WORD kMfc42CButtonDtorOrdinal = 609;
    const WORD kMfc42CEditDtorOrdinal = 656;
    const WORD kMfc42CDialogDtorOrdinal = 641;

    return PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDoModalOrdinal,
                                (void *)&FakeModalDoModal, imports[0]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42CStringDtorOrdinal,
                                (void *)&FakeModalCStringDtor, imports[1]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42CComboBoxDtorOrdinal,
                                (void *)&FakeModalComboDtor, imports[2]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42CListBoxDtorOrdinal,
                                (void *)&FakeModalListDtor, imports[3]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42CButtonDtorOrdinal,
                                (void *)&FakeModalButtonDtor, imports[4]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42CEditDtorOrdinal,
                                (void *)&FakeModalEditDtor, imports[5]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDtorOrdinal,
                                (void *)&FakeModalDialogDtor, imports[6]) &&
           PatchFunctionJump(MethodAddress(&CZRecoilFrame::SetMenuBarVisibility),
                             (void *)&FakeModalSetMenuBarVisibility, menuPatch);
}

bool InstallModalDdxPatches(ImportFunctionPatch *imports)
{
    const WORD kMfc42DDXCheckOrdinal = 2301;
    const WORD kMfc42DDXControlOrdinal = 2302;
    const WORD kMfc42DDXTextUIntOrdinal = 2363;

    return PatchImportByOrdinal("MFC42.DLL", kMfc42DDXControlOrdinal,
                                (void *)&FakeModalDDXControl, imports[0]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42DDXTextUIntOrdinal,
                                (void *)&FakeModalDDXTextUInt, imports[1]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42DDXCheckOrdinal,
                                (void *)&FakeModalDDXCheck, imports[2]);
}

void RestoreModalDdxPatches(ImportFunctionPatch *imports)
{
    for (int index = 2; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
}

void RestoreModalPatches(ImportFunctionPatch *imports, CodeFunctionPatch &menuPatch)
{
    RestoreFunctionPatch(menuPatch);
    for (int i = 6; i >= 0; --i) {
        RestoreImportPatch(imports[i]);
    }
}
} // namespace

extern "C" int westwood_online_upgrade_config_get_message_map_smoke(void)
{
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeConfigDialog)] = {0};
    WestwoodOnlineUpgradeConfigDialog &dialog = *(WestwoodOnlineUpgradeConfigDialog *)dialogStorage;
    const AFX_MSGMAP *const messageMap =
        dialog.WestwoodOnlineUpgradeConfigDialog::GetMessageMap();
    if (messageMap != &WestwoodOnlineUpgradeConfigDialog::messageMap) {
        return 1;
    }

    if (messageMap->pfnGetBaseMap == 0 ||
        messageMap->lpEntries != &WestwoodOnlineUpgradeConfigDialog::messageEntries[0]) {
        return 2;
    }

    const AFX_MSGMAP_ENTRY &sentinel =
        WestwoodOnlineUpgradeConfigDialog::messageEntries[0];
    return sentinel.nMessage == 0 && sentinel.nCode == 0 && sentinel.nID == 0 &&
                   sentinel.nLastID == 0 && sentinel.nSig == 0 && sentinel.pfn == 0
               ? 0
               : 3;
}

extern "C" int westwood_online_upgrade_dialog_get_message_map_smoke(void)
{
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    const AFX_MSGMAP *const messageMap =
        dialog.WestwoodOnlineUpgradeDialog::GetMessageMap();
    if (messageMap != &WestwoodOnlineUpgradeDialog::messageMap)
    {
        return 1;
    }

    if (messageMap->pfnGetBaseMap == 0 ||
        messageMap->lpEntries != &WestwoodOnlineUpgradeDialog::messageEntries[0])
    {
        return 2;
    }

    const AFX_MSGMAP_ENTRY *const entries =
        WestwoodOnlineUpgradeDialog::messageEntries;
    if (entries[0].nMessage != WM_TIMER ||
        entries[0].nCode != 0 ||
        entries[0].nID != 0 ||
        entries[0].nLastID != 0 ||
        entries[0].nSig != 13 ||
        entries[0].pfn !=
            MessageHandler(&WestwoodOnlineUpgradeDialog::OnRefreshListTimer))
    {
        return 3;
    }

    if (entries[1].nMessage != WM_DESTROY ||
        entries[1].nSig != 12 ||
        entries[1].pfn != MessageHandler(&WestwoodOnlineUpgradeDialog::OnDestroy))
    {
        return 4;
    }

    if (entries[21].nMessage != WM_COMMAND ||
        entries[21].nCode != LBN_DBLCLK ||
        entries[21].nID != 1136 ||
        entries[21].nLastID != 1136 ||
        entries[21].nSig != 12 ||
        entries[21].pfn !=
            MessageHandler(&WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk))
    {
        return 5;
    }

    if (entries[22].nMessage != WM_COMMAND ||
        entries[22].nCode != EN_KILLFOCUS ||
        entries[22].nID != 1170 ||
        entries[22].pfn !=
            MessageHandler(&WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus))
    {
        return 6;
    }

    if (entries[23].nMessage != WM_COMMAND ||
        entries[23].nCode != EN_KILLFOCUS ||
        entries[23].nID != 1169 ||
        entries[23].pfn !=
            MessageHandler(&WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus))
    {
        return 7;
    }

    if (entries[24].nMessage != WM_COMMAND ||
        entries[24].nCode != EN_KILLFOCUS ||
        entries[24].nID != 1168 ||
        entries[24].pfn !=
            MessageHandler(&WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus))
    {
        return 8;
    }

    const AFX_MSGMAP_ENTRY &sentinel = entries[25];
    return sentinel.nMessage == 0 && sentinel.nCode == 0 && sentinel.nID == 0 &&
                   sentinel.nLastID == 0 && sentinel.nSig == 0 && sentinel.pfn == 0
               ? 0
               : 9;
}

extern "C" int westwood_online_upgrade_dialog_on_init_bootstrap_smoke(void)
{
    const WORD kMfc42CDialogOnInitDialogOrdinal = 4710;
    const WORD kMfc42CWndSetDlgItemTextAOrdinal = 5953;
    const WORD kMfc42CWndEnableWindowOrdinal = 2642;
    ImportFunctionPatch imports[5] = {};
    CodeFunctionPatch patches[5] = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogOnInitDialogOrdinal,
                              (void *)&FakeInitDialogBaseOnInitDialog,
                              imports[0]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetDlgItemTextAOrdinal,
                              (void *)&FakeInitSetDlgItemTextA, imports[1]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndEnableWindowOrdinal,
                              (void *)&FakeWolEnableWindow, imports[2]) ||
        !PatchImportByName("USER32.dll", "SendMessageA",
                           (void *)&FakeInitDialogSendMessageA, imports[3]) ||
        !PatchImportByName("USER32.dll", "SetTimer",
                           (void *)&FakeInitDialogSetTimer, imports[4]) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString, patches[0]) ||
        !PatchFunctionJump((void *)&WestwoodOnlineUpgradeApi::Init,
                           (void *)&FakeInitDialogApiInit, patches[1]) ||
        !PatchFunctionJump(MethodAddress(&WestwoodOnlineUpgradeDialog::OnDestroy),
                           (void *)&FakeInitDialogOnDestroy, patches[2]) ||
        !PatchFunctionJump(MethodAddress(&WestwoodOnlineUpgradeDialog::SetAbortAndClose),
                           (void *)&FakeInitDialogSetAbortAndClose, patches[3]) ||
        !PatchFunctionJump(CWndUpdateDataAddress(), (void *)&FakeWestwoodUpdateData,
                           patches[4]))
    {
        for (int index = 4; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        for (int index = 4; index >= 0; --index)
        {
            RestoreImportPatch(imports[index]);
        }
        return 1;
    }

    const int oldDisconnectInFlightFlag =
        g_WestwoodOnlineUpgradeDisconnectInFlightFlag;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    new (&dialog.m_sessionName) CString();
    dialog.m_hWnd = (HWND)0x12340100;
    dialog.m_sessionModeCombo.m_hWnd = (HWND)0x12340101;
    dialog.m_sessionNameEdit.m_hWnd = (HWND)0x12340102;
    dialog.m_serverAddressEdit.m_hWnd = (HWND)0x12340103;
    dialog.m_statusServerEdit.m_hWnd = (HWND)0x12340104;
    dialog.m_statusTokenEdit.m_hWnd = (HWND)0x12340105;
    dialog.m_statusList.m_hWnd = (HWND)0x12340106;

    g_initDialogBaseOnInitCalls = 0;
    g_initDialogBaseOnInitThis = 0;
    g_initDialogApiInitCalls = 0;
    g_initDialogApiInitResult = 1;
    g_initDialogOnDestroyCalls = 0;
    g_initDialogOnDestroyThis = 0;
    g_initDialogSetAbortCalls = 0;
    g_initDialogSetAbortThis = 0;
    g_initDialogSetTimerCalls = 0;
    g_initDialogSetTimerHwnd = 0;
    g_initDialogSetTimerId = 0;
    g_initDialogSetTimerMs = 0;
    g_initDialogSetTimerProc = 0;
    g_initDialogSendMessageCalls = 0;
    g_initDialogComboAddCalls = 0;
    memset(g_initDialogSendMessageHwnd, 0, sizeof(g_initDialogSendMessageHwnd));
    memset(g_initDialogSendMessageMsg, 0, sizeof(g_initDialogSendMessageMsg));
    memset(g_initDialogSendMessageWParam, 0,
           sizeof(g_initDialogSendMessageWParam));
    memset(g_initDialogSendMessageLParam, 0,
           sizeof(g_initDialogSendMessageLParam));
    g_initSetDlgItemTextCalls = 0;
    g_initMessageIdCalls = 0;
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatUpdateDataSaveValue[0] = -1;
    g_enableWindowCalls = 0;
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 77;

    int result = dialog.OnInitDialogBootstrap();
    int failure = 0;
    if (result != 1 ||
        g_initDialogBaseOnInitCalls != 1 ||
        g_initDialogBaseOnInitThis != (CDialog *)&dialog ||
        g_initMessageIdCalls != 9 ||
        g_initMessageIds[0] != 0x2036 ||
        g_initMessageIds[6] != 0x203c ||
        g_initMessageIds[7] != 0x3034 ||
        g_initMessageIds[8] != 0x3037 ||
        strcmp((const char *)dialog.m_sessionName, "msg-3034") != 0)
    {
        failure = 2;
    }
    else if (g_initDialogSendMessageCalls != 20 ||
             g_initDialogComboAddCalls != 7 ||
             g_initDialogSendMessageMsg[0] != CB_ADDSTRING ||
             strcmp((const char *)g_initDialogSendMessageLParam[0],
                    "msg-2036") != 0 ||
             g_initDialogSendMessageMsg[1] != CB_SETITEMDATA ||
             g_initDialogSendMessageWParam[1] != 0 ||
             g_initDialogSendMessageLParam[1] != 0 ||
             g_initDialogSendMessageMsg[13] != CB_SETITEMDATA ||
             g_initDialogSendMessageWParam[13] != 6 ||
             g_initDialogSendMessageLParam[13] != 6 ||
             g_initDialogSendMessageMsg[14] != CB_SETCURSEL ||
             g_initDialogSendMessageHwnd[14] != dialog.m_sessionModeCombo.m_hWnd)
    {
        failure = 3;
    }
    else if (g_initSetDlgItemTextCalls != 1 ||
             g_initSetDlgItemTextThis[0] != &dialog ||
             g_initSetDlgItemTextControlId[0] != 1125 ||
             strcmp(g_initSetDlgItemTextValue[0], "msg-3037") != 0 ||
             dialog.m_queryValueOrTime != 15 ||
             dialog.m_queryAuxParam != 10 ||
             dialog.m_queryMaxPlayers != 4 ||
             dialog.m_queryStatusFlagBit0 != 1 ||
             dialog.m_queryStatusFlagBit1 != 0 ||
             g_threeFloatUpdateDataCount != 1 ||
             g_threeFloatUpdateDataSaveValue[0] != 0 ||
             dialog.m_statusLineCount != 0 ||
             g_enableWindowCalls != 7 ||
             g_initDialogApiInitCalls != 1)
    {
        failure = 4;
    }
    else if (g_initDialogSetTimerCalls != 1 ||
             g_initDialogSetTimerHwnd != dialog.m_hWnd ||
             g_initDialogSetTimerId != 4 ||
             g_initDialogSetTimerMs != 100 ||
             g_initDialogSetTimerProc != 0 ||
             g_initDialogSendMessageHwnd[15] != dialog.m_sessionNameEdit.m_hWnd ||
             g_initDialogSendMessageMsg[15] != EM_LIMITTEXT ||
             g_initDialogSendMessageWParam[15] != 17 ||
             g_initDialogSendMessageHwnd[16] !=
                 dialog.m_serverAddressEdit.m_hWnd ||
             g_initDialogSendMessageWParam[16] != 9 ||
             g_initDialogSendMessageHwnd[17] != dialog.m_statusServerEdit.m_hWnd ||
             g_initDialogSendMessageWParam[17] != 100 ||
             g_initDialogSendMessageHwnd[18] != dialog.m_statusTokenEdit.m_hWnd ||
             g_initDialogSendMessageWParam[18] != 10 ||
             g_initDialogSendMessageHwnd[19] != dialog.m_statusList.m_hWnd ||
             g_initDialogSendMessageMsg[19] != LB_SETHORIZONTALEXTENT ||
             g_initDialogSendMessageWParam[19] != 1500 ||
             g_WestwoodOnlineUpgradeDisconnectInFlightFlag != 0 ||
             g_initDialogOnDestroyCalls != 0 ||
             g_initDialogSetAbortCalls != 0)
    {
        failure = 5;
    }

    dialog.m_sessionName.~CString();

    unsigned char failingDialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &failingDialog = *(WestwoodOnlineUpgradeDialog *)failingDialogStorage;
    new (&failingDialog.m_sessionName) CString();
    failingDialog.m_sessionModeCombo.m_hWnd = (HWND)0x12340201;
    g_initDialogApiInitResult = 0;
    g_initDialogBaseOnInitCalls = 0;
    g_initDialogApiInitCalls = 0;
    g_initDialogOnDestroyCalls = 0;
    g_initDialogOnDestroyThis = 0;
    g_initDialogSetAbortCalls = 0;
    g_initDialogSetAbortThis = 0;
    g_initDialogSetTimerCalls = 0;
    g_initDialogSendMessageCalls = 0;
    g_initDialogComboAddCalls = 0;
    g_initMessageIdCalls = 0;
    g_enableWindowCalls = 0;
    g_threeFloatUpdateDataCount = 0;
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 55;

    result = failingDialog.OnInitDialogBootstrap();
    if (failure == 0 &&
        (result != 0 ||
         g_initDialogBaseOnInitCalls != 1 ||
         g_initDialogApiInitCalls != 1 ||
         g_initDialogOnDestroyCalls != 1 ||
         g_initDialogOnDestroyThis != &failingDialog ||
         g_initDialogSetAbortCalls != 1 ||
         g_initDialogSetAbortThis != &failingDialog ||
         g_initDialogSetTimerCalls != 0 ||
         g_initDialogSendMessageCalls != 15 ||
         g_enableWindowCalls != 7 ||
         g_WestwoodOnlineUpgradeDisconnectInFlightFlag != 55))
    {
        failure = 6;
    }
    failingDialog.m_sessionName.~CString();

    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = oldDisconnectInFlightFlag;
    for (int index = 4; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    for (int index = 4; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    return failure;
}

extern "C" int westwood_online_upgrade_config_dialog_constructor_smoke(void)
{
    int wolPasswordFlag = 73;
    int *const oldWolPasswordFlagOption = g_zOpt_WolPasswordFlagOption;
    g_zOpt_WolPasswordFlagOption = &wolPasswordFlag;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeConfigDialog)] = {0};

    WestwoodOnlineUpgradeConfigDialog &dialog = *(WestwoodOnlineUpgradeConfigDialog *)dialogStorage;
    memset(&dialog, 0x5a, sizeof(dialog));

    WestwoodOnlineUpgradeConfigDialog *const result = dialog.Constructor(0);
    int failure = 0;
    if (result != &dialog ||
        TestObjectVtable(&dialog) == 0)
    {
        failure = 1;
    }
    else if (!TestMfcWindowConstructed(dialog.m_serverNameEdit) ||
             !TestMfcWindowConstructed(dialog.m_profileCombo))
    {
        failure = 2;
    }
    else if (strcmp((const char *)dialog.m_noPasswordText, "No Password") != 0)
    {
        failure = 3;
    }
    else if (dialog.m_wolPasswordFlag != wolPasswordFlag)
    {
        failure = 4;
    }

    g_zOpt_WolPasswordFlagOption = oldWolPasswordFlagOption;
    return failure;
}

extern "C" int westwood_online_upgrade_config_get_selected_profile_values_smoke(void)
{
    int wolPasswordFlag = 0;
    int *const oldWolPasswordFlagOption = g_zOpt_WolPasswordFlagOption;
    g_zOpt_WolPasswordFlagOption = &wolPasswordFlag;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeConfigDialog)] = {0};

    WestwoodOnlineUpgradeConfigDialog &dialog = *(WestwoodOnlineUpgradeConfigDialog *)dialogStorage;
    dialog.Constructor(0);
    dialog.m_selectedProfileIndex = 1;
    dialog.m_profilePlayerNames[1] = "PilotTwo";
    dialog.m_profileConnectStrings[1] = "connect-two";
    dialog.m_profileConnectStringModes[1] = 12;

    char *playerName = 0;
    char *connectString = 0;
    int connectStringMode = 0;
    dialog.GetSelectedProfileValues(&playerName, &connectString, &connectStringMode);

    const int result = strcmp(playerName, "PilotTwo") == 0 &&
                               strcmp(connectString, "connect-two") == 0 &&
                               connectStringMode == 12
                           ? 0
                           : 1;

    g_zOpt_WolPasswordFlagOption = oldWolPasswordFlagOption;
    return result;
}

extern "C" int westwood_online_upgrade_config_show_modal_apply_smoke(void)
{
    const WORD kMfc42CDialogDoModalOrdinal = 2514;
    const WORD kMfc42CStringDtorOrdinal = 800;
    const WORD kMfc42CComboBoxDtorOrdinal = 616;
    const WORD kMfc42CEditDtorOrdinal = 656;
    const WORD kMfc42CDialogDtorOrdinal = 641;
    ImportFunctionPatch imports[5] = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDoModalOrdinal,
                              (void *)&FakeConfigDoModal, imports[0]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CStringDtorOrdinal,
                              (void *)&FakeModalCStringDtor, imports[1]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CComboBoxDtorOrdinal,
                              (void *)&FakeModalComboDtor, imports[2]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CEditDtorOrdinal,
                              (void *)&FakeModalEditDtor, imports[3]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDtorOrdinal,
                              (void *)&FakeModalDialogDtor, imports[4]))
    {
        for (int index = 4; index >= 0; --index)
        {
            RestoreImportPatch(imports[index]);
        }
        return 1;
    }

    int wolPasswordFlag = 0;
    int *const oldWolPasswordFlagOption = g_zOpt_WolPasswordFlagOption;
    WestwoodOnlineUpgradeDialog *const oldUpgradeDialog = g_pWestwoodOnlineUpgradeDialog;
    g_zOpt_WolPasswordFlagOption = &wolPasswordFlag;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    memset(&dialog, 0x5a, sizeof(dialog));
    dialog.Constructor(0);
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    g_configDoModalCalls = 0;
    g_configDoModalResult = IDOK;
    g_configDoModalThis = 0;
    int result = WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues();
    int failure = 0;
    if (result != 1 || g_configDoModalCalls != 1 || g_configDoModalThis == 0)
    {
        failure = 2;
    }
    else if (strcmp((const char *)dialog.m_selectedProfilePlayerName, "DialogPilot") != 0 ||
             strcmp((const char *)dialog.m_selectedProfileConnectString,
                    "DialogConnect") != 0 ||
             dialog.m_selectedProfileConnectStringMode != 21)
    {
        failure = 3;
    }

    CString oldPlayer("OldPilot");
    CString oldConnect("OldConnect");
    dialog.SetSelectedProfilePlayerName(oldPlayer);
    dialog.SetSelectedProfileConnectString(oldConnect);
    dialog.m_selectedProfileConnectStringMode = 99;
    g_configDoModalCalls = 0;
    g_configDoModalResult = IDCANCEL;
    result = WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues();
    if (failure == 0 &&
        (result != 0 || g_configDoModalCalls != 1 ||
         strcmp((const char *)dialog.m_selectedProfilePlayerName, "OldPilot") != 0 ||
         strcmp((const char *)dialog.m_selectedProfileConnectString, "OldConnect") != 0 ||
         dialog.m_selectedProfileConnectStringMode != 99))
    {
        failure = 4;
    }

    g_pWestwoodOnlineUpgradeDialog = oldUpgradeDialog;
    g_zOpt_WolPasswordFlagOption = oldWolPasswordFlagOption;
    for (int index = 4; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    return failure;
}

extern "C" int westwood_online_upgrade_download_callback_no_op_smoke(void)
{
    WestwoodOnlineUpgradeDownloadEventSink sink = {};
    WestwoodOnlineUpgradeDownloadEventSinkVtable markerVtable = {};

    sink.m_vftable = &markerVtable;
    sink.m_refCountAndLock.refCount = 7;

    const int result = sink.CallbackNoOp(&sink.m_refCountAndLock);
    return result == 0 && sink.m_vftable == &markerVtable &&
                   sink.m_refCountAndLock.refCount == 7
               ? 0
               : 1;
}

extern "C" int westwood_online_upgrade_download_event_sink_create_instance_smoke(void)
{
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeEventSinkLiveCount;
    WestwoodOnlineUpgradeDownloadEventSink *eventSink = 0;

    HRESULT result = WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(&eventSink);
    int failure = 0;
    if (result != S_OK || eventSink == 0)
    {
        failure = 1;
    }
    else if (eventSink->m_vftable != &g_WestwoodOnlineUpgradeDownloadEventSink_Vtbl)
    {
        failure = 2;
    }
    else if (eventSink->m_refCountAndLock.refCount != 0 ||
             g_WestwoodOnlineUpgradeEventSinkLiveCount != oldLiveCount + 1)
    {
        failure = 3;
    }
    else if (TryEnterCriticalSection(&eventSink->m_refCountAndLock.lock) == 0)
    {
        failure = 4;
    }
    else
    {
        LeaveCriticalSection(&eventSink->m_refCountAndLock.lock);
    }

    if (eventSink != 0)
    {
        DeleteCriticalSection(&eventSink->m_refCountAndLock.lock);
        ::operator delete(eventSink);
    }
    g_WestwoodOnlineUpgradeEventSinkLiveCount = oldLiveCount;
    return failure;
}

extern "C" int westwood_online_upgrade_api_event_sink_create_instance_smoke(void)
{
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeEventSinkLiveCount;
    WestwoodOnlineUpgradeApiEventSink *eventSink = 0;

    HRESULT result = WestwoodOnlineUpgradeApiEventSink::CreateInstance(&eventSink);
    int failure = 0;
    if (result != S_OK || eventSink == 0)
    {
        failure = 1;
    }
    else if (eventSink->m_vftable != &g_WestwoodOnlineUpgradeApiEventSink_Vtbl)
    {
        failure = 2;
    }
    else if (eventSink->m_refCountAndLock.refCount != 0 ||
             g_WestwoodOnlineUpgradeEventSinkLiveCount != oldLiveCount + 1)
    {
        failure = 3;
    }
    else if (TryEnterCriticalSection(&eventSink->m_refCountAndLock.lock) == 0)
    {
        failure = 4;
    }
    else
    {
        LeaveCriticalSection(&eventSink->m_refCountAndLock.lock);
    }

    if (eventSink != 0)
    {
        DeleteCriticalSection(&eventSink->m_refCountAndLock.lock);
        ::operator delete(eventSink);
    }
    g_WestwoodOnlineUpgradeEventSinkLiveCount = oldLiveCount;
    return failure;
}

extern "C" int westwood_online_upgrade_download_create_instance_advise_smoke(void)
{
    IUnknown *const oldDownload = g_pWestwoodOnlineUpgradeDownload;
    void *const oldSink = g_pWestwoodOnlineUpgradeDownloadEventSink;
    const DWORD oldCookie = g_WestwoodOnlineUpgradeDownloadAdviseCookie;
    const int oldSinkOffset =
        g_WestwoodOnlineUpgradeDownloadEventSinkConnectionOffset;
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeEventSinkLiveCount;
    ImportFunctionPatch import = {};

    g_shutdownUnknown.vftable = &g_shutdownUnknownVtable;
    g_shutdownCpc.vftable = &g_shutdownCpcVtable;
    g_shutdownConnectionPoint.vftable = &g_shutdownConnectionPointVtable;
    g_shutdownSourceReleaseCalls = 0;
    g_shutdownCpcReleaseCalls = 0;
    g_shutdownConnectionPointReleaseCalls = 0;
    g_shutdownFindConnectionPointCalls = 0;
    g_shutdownAdviseCalls = 0;
    g_shutdownAdviseSink = 0;
    g_shutdownAdviseCookie = 0;
    g_shutdownIidOk = false;
    g_downloadCreateCoCreateCalls = 0;
    g_downloadCreateCoCreateArgsOk = false;
    g_downloadCreateCoCreateResult = S_OK;
    g_downloadCreateCoCreateObject = (IUnknown *)&g_shutdownUnknown;
    g_pWestwoodOnlineUpgradeDownload = 0;
    g_pWestwoodOnlineUpgradeDownloadEventSink = 0;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0;
    g_WestwoodOnlineUpgradeDownloadEventSinkConnectionOffset = 0;

    int failure = 0;
    if (!PatchImportByName("ole32.dll", "CoCreateInstance",
                           (void *)&FakeDownloadCreateCoCreateInstance, import))
    {
        failure = 90;
    }
    else
    {
        HRESULT result = WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise();
        if (result != S_OK || g_downloadCreateCoCreateCalls != 1 ||
            !g_downloadCreateCoCreateArgsOk)
        {
            failure = 1;
        }
        else if (g_pWestwoodOnlineUpgradeDownload !=
                     (IUnknown *)&g_shutdownUnknown ||
                 g_pWestwoodOnlineUpgradeDownloadEventSink == 0 ||
                 g_WestwoodOnlineUpgradeEventSinkLiveCount != oldLiveCount + 1)
        {
            failure = 2;
        }
        else if (g_shutdownFindConnectionPointCalls != 1 ||
                 g_shutdownAdviseCalls != 1 || !g_shutdownIidOk ||
                 g_shutdownAdviseSink !=
                     (IUnknown *)g_pWestwoodOnlineUpgradeDownloadEventSink ||
                 g_WestwoodOnlineUpgradeDownloadAdviseCookie != 0x87654321)
        {
            failure = 3;
        }
        else if (g_shutdownConnectionPointReleaseCalls != 1 ||
                 g_shutdownCpcReleaseCalls != 1 ||
                 g_shutdownSourceReleaseCalls != 0)
        {
            failure = 4;
        }
    }

    if (g_pWestwoodOnlineUpgradeDownloadEventSink != 0)
    {
        WestwoodOnlineUpgradeDownloadEventSink *const sink =
            (WestwoodOnlineUpgradeDownloadEventSink *)
                g_pWestwoodOnlineUpgradeDownloadEventSink;
        DeleteCriticalSection(&sink->m_refCountAndLock.lock);
        ::operator delete(sink);
    }
    g_pWestwoodOnlineUpgradeDownload = oldDownload;
    g_pWestwoodOnlineUpgradeDownloadEventSink = oldSink;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = oldCookie;
    g_WestwoodOnlineUpgradeDownloadEventSinkConnectionOffset = oldSinkOffset;
    g_WestwoodOnlineUpgradeEventSinkLiveCount = oldLiveCount;
    RestoreImportPatch(import);
    return failure;
}

extern "C" int westwood_online_upgrade_download_unadvise_release_smoke(void)
{
    IUnknown *const oldDownload = g_pWestwoodOnlineUpgradeDownload;
    const DWORD oldCookie = g_WestwoodOnlineUpgradeDownloadAdviseCookie;

    g_shutdownUnknown.vftable = &g_shutdownUnknownVtable;
    g_shutdownCpc.vftable = &g_shutdownCpcVtable;
    g_shutdownConnectionPoint.vftable = &g_shutdownConnectionPointVtable;
    g_shutdownSourceReleaseCalls = 0;
    g_shutdownCpcReleaseCalls = 0;
    g_shutdownConnectionPointReleaseCalls = 0;
    g_shutdownFindConnectionPointCalls = 0;
    g_shutdownUnadviseCalls = 0;
    g_shutdownUnadviseCookie = 0;
    g_shutdownIidOk = false;
    g_pWestwoodOnlineUpgradeDownload = (IUnknown *)&g_shutdownUnknown;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0x13572468;

    const ULONG result = WestwoodOnlineUpgradeDownload::UnadviseAndRelease();
    int failure = 0;
    if (result != 1 || g_shutdownFindConnectionPointCalls != 1 ||
        g_shutdownUnadviseCalls != 1 || !g_shutdownIidOk ||
        g_shutdownUnadviseCookie != 0x13572468)
    {
        failure = 1;
    }
    else if (g_shutdownConnectionPointReleaseCalls != 1 ||
             g_shutdownCpcReleaseCalls != 1 ||
             g_shutdownSourceReleaseCalls != 1)
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeDownload = oldDownload;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = oldCookie;
    return failure;
}

extern "C" int westwood_online_upgrade_progress_dialog_dlg_proc_smoke(void)
{
    IUnknown *const oldDownload = g_pWestwoodOnlineUpgradeDownload;
    void *const oldSink = g_pWestwoodOnlineUpgradeDownloadEventSink;
    WestwoodOnlineUpgradeDownloadReadyEntry *const oldReadyList =
        g_pWestwoodOnlineUpgradeDownloadReadyList;
    const DWORD oldCookie = g_WestwoodOnlineUpgradeDownloadAdviseCookie;
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeEventSinkLiveCount;
    const int oldDialogResult = g_WestwoodOnlineUpgradeDownloadDialogResult;
    HWND const oldProgressHwnd = g_hWestwoodOnlineUpgradeProgressDialog;
    char oldPrompt[sizeof(g_WestwoodOnlineUpgradeDownloadReadyPromptText)];
    char oldRestoreCwd[sizeof(g_WestwoodOnlineUpgradeDownloadRestoreCwd)];
    ImportFunctionPatch imports[9] = {};

    memcpy(oldPrompt,
           g_WestwoodOnlineUpgradeDownloadReadyPromptText,
           sizeof(oldPrompt));
    memcpy(oldRestoreCwd,
           g_WestwoodOnlineUpgradeDownloadRestoreCwd,
           sizeof(oldRestoreCwd));

    int failure = 0;
    if (!PatchImportByName("ole32.dll", "CoCreateInstance",
                           (void *)&FakeDownloadCreateCoCreateInstance,
                           imports[0]) ||
        !PatchImportByName("USER32.dll", "SetDlgItemTextA",
                           (void *)&FakeDownloadDlgSetDlgItemTextA,
                           imports[1]) ||
        !PatchImportByName("KERNEL32.dll", "GetCurrentDirectoryA",
                           (void *)&FakeDownloadDlgGetCurrentDirectoryA,
                           imports[2]) ||
        !PatchImportByName("KERNEL32.dll", "SetCurrentDirectoryA",
                           (void *)&FakeDownloadDlgSetCurrentDirectoryA,
                           imports[3]) ||
        !PatchImportByName("KERNEL32.dll", "CreateDirectoryA",
                           (void *)&FakeDownloadDlgCreateDirectoryA,
                           imports[4]) ||
        !PatchImportByName("USER32.dll", "SetTimer",
                           (void *)&FakeDownloadDlgSetTimer,
                           imports[5]) ||
        !PatchImportByName("USER32.dll", "KillTimer",
                           (void *)&FakeDownloadDlgKillTimer,
                           imports[6]) ||
        !PatchImportByName("USER32.dll", "DestroyWindow",
                           (void *)&FakeDownloadDlgDestroyWindow,
                           imports[7]) ||
        !PatchImportByName("USER32.dll", "EndDialog",
                           (void *)&FakeDownloadDlgEndDialog,
                           imports[8]))
    {
        failure = 90;
    }

    WestwoodOnlineUpgradeDownloadReadyEntry entry = {};
    entry.m_next = (WestwoodOnlineUpgradeDownloadReadyEntry *)0x12345678;
    strcpy(entry.m_descriptor0, "descriptor-zero");
    strcpy(entry.m_sourcePathBase, "C:\\PatchSource");
    strcpy(entry.m_fileName, "recoil11.exe");
    strcpy(entry.m_descriptor1, "descriptor-one");
    strcpy(entry.m_descriptor2, "descriptor-two");
    strcpy(entry.m_downloadDirectory, "C:\\PatchTarget");
    strcpy(g_WestwoodOnlineUpgradeDownloadReadyPromptText, "Download prompt");

    g_downloadDlgFakeObject.vftable = &g_downloadDlgFakeVtable;
    g_shutdownCpc.vftable = &g_shutdownCpcVtable;
    g_shutdownConnectionPoint.vftable = &g_shutdownConnectionPointVtable;
    g_shutdownCpcReleaseCalls = 0;
    g_shutdownConnectionPointReleaseCalls = 0;
    g_shutdownFindConnectionPointCalls = 0;
    g_shutdownAdviseCalls = 0;
    g_shutdownUnadviseCalls = 0;
    g_shutdownIidOk = false;
    g_downloadCreateCoCreateCalls = 0;
    g_downloadCreateCoCreateArgsOk = false;
    g_downloadCreateCoCreateResult = S_OK;
    g_downloadCreateCoCreateObject = (IUnknown *)&g_downloadDlgFakeObject;
    g_downloadDlgBeginCalls = 0;
    g_downloadDlgBeginSelf = 0;
    g_downloadDlgAbortCalls = 0;
    g_downloadDlgAbortSelf = 0;
    g_downloadDlgPumpCalls = 0;
    g_downloadDlgPumpSelf = 0;
    g_downloadDlgReleaseCalls = 0;
    g_downloadDlgSetDlgItemTextCalls = 0;
    g_downloadDlgGetCurrentDirectoryCalls = 0;
    strcpy(g_downloadDlgCurrentDirectory, "C:\\PreviousCwd");
    g_downloadDlgSetCurrentDirectoryCalls = 0;
    g_downloadDlgSetCurrentDirectoryResult[0] = FALSE;
    g_downloadDlgSetCurrentDirectoryResult[1] = TRUE;
    g_downloadDlgSetCurrentDirectoryResult[2] = TRUE;
    g_downloadDlgSetCurrentDirectoryResult[3] = TRUE;
    g_downloadDlgCreateDirectoryCalls = 0;
    g_downloadDlgCreateDirectoryPath = 0;
    g_downloadDlgCreateDirectorySecurity = 0;
    g_downloadDlgSetTimerCalls = 0;
    g_downloadDlgKillTimerCalls = 0;
    g_downloadDlgDestroyWindowCalls = 0;
    g_downloadDlgEndDialogCalls = 0;
    g_pWestwoodOnlineUpgradeDownload = 0;
    g_pWestwoodOnlineUpgradeDownloadEventSink = 0;
    g_pWestwoodOnlineUpgradeDownloadReadyList = &entry;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0;
    g_WestwoodOnlineUpgradeEventSinkLiveCount = oldLiveCount;
    g_WestwoodOnlineUpgradeDownloadDialogResult = 99;
    g_hWestwoodOnlineUpgradeProgressDialog = 0;

    HWND const dialogHwnd = (HWND)0x24681357;
    if (failure == 0)
    {
        BOOL result = WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd, WM_INITDIALOG, 0, 0);
        if (result != TRUE || g_downloadCreateCoCreateCalls != 1 ||
            !g_downloadCreateCoCreateArgsOk ||
            g_pWestwoodOnlineUpgradeDownload !=
                (IUnknown *)&g_downloadDlgFakeObject ||
            g_pWestwoodOnlineUpgradeDownloadEventSink == 0 ||
            g_WestwoodOnlineUpgradeEventSinkLiveCount != oldLiveCount + 1 ||
            g_shutdownAdviseCalls != 1)
        {
            failure = 1;
        }
        else if (g_downloadDlgSetDlgItemTextCalls != 2 ||
                 g_downloadDlgSetDlgItemTextHwnd[0] != dialogHwnd ||
                 g_downloadDlgSetDlgItemTextControlId[0] != 1024 ||
                 strcmp(g_downloadDlgSetDlgItemTextValue[0],
                        "Download prompt") != 0 ||
                 g_downloadDlgGetCurrentDirectoryCalls != 1 ||
                 strcmp(g_WestwoodOnlineUpgradeDownloadRestoreCwd,
                        "C:\\PreviousCwd") != 0)
        {
            failure = 2;
        }
        else if (g_downloadDlgSetCurrentDirectoryCalls != 2 ||
                 strcmp(g_downloadDlgSetCurrentDirectoryPath[0],
                        "C:\\PatchTarget") != 0 ||
                 g_downloadDlgCreateDirectoryCalls != 1 ||
                 strcmp(g_downloadDlgCreateDirectoryPath, "C:\\PatchTarget") !=
                     0 ||
                 g_downloadDlgCreateDirectorySecurity != 0)
        {
            failure = 3;
        }
        else if (g_downloadDlgBeginCalls != 1 ||
                 g_downloadDlgBeginSelf != (IUnknown *)&g_downloadDlgFakeObject ||
                 strcmp(g_downloadDlgBeginDescriptor0, "descriptor-zero") != 0 ||
                 strcmp(g_downloadDlgBeginDescriptor1, "descriptor-one") != 0 ||
                 strcmp(g_downloadDlgBeginDescriptor2, "descriptor-two") != 0 ||
                 strcmp(g_downloadDlgBeginSourcePath,
                        "C:\\PatchSource\\recoil11.exe") != 0 ||
                 strcmp(g_downloadDlgBeginFileName, "recoil11.exe") != 0 ||
                 strcmp(g_downloadDlgBeginRegistryKey,
                        "SOFTWARE\\Westwood\\Recoil") != 0)
        {
            failure = 4;
        }
        else if (g_hWestwoodOnlineUpgradeProgressDialog != dialogHwnd ||
                 g_WestwoodOnlineUpgradeDownloadDialogResult != 0 ||
                 g_downloadDlgSetTimerCalls != 1 ||
                 g_downloadDlgSetTimerHwnd != dialogHwnd ||
                 g_downloadDlgSetTimerId != 1 ||
                 g_downloadDlgSetTimerMs != 50 ||
                 g_downloadDlgSetTimerProc != 0)
        {
            failure = 5;
        }
    }

    if (failure == 0)
    {
        BOOL result =
            WestwoodOnlineUpgradeProgressDialog::DlgProc(dialogHwnd,
                                                        WM_TIMER,
                                                        1,
                                                        0);
        if (result != TRUE || g_downloadDlgPumpCalls != 1 ||
            g_downloadDlgPumpSelf != (IUnknown *)&g_downloadDlgFakeObject ||
            g_downloadDlgDestroyWindowCalls != 0)
        {
            failure = 6;
        }
    }

    if (failure == 0)
    {
        g_WestwoodOnlineUpgradeDownloadDialogResult = 7;
        BOOL result =
            WestwoodOnlineUpgradeProgressDialog::DlgProc(dialogHwnd,
                                                        WM_TIMER,
                                                        1,
                                                        0);
        if (result != TRUE || g_downloadDlgDestroyWindowCalls != 1 ||
            g_downloadDlgDestroyWindowHwnd[0] != dialogHwnd)
        {
            failure = 7;
        }
    }

    if (failure == 0)
    {
        g_hWestwoodOnlineUpgradeProgressDialog = (HWND)0x13572468;
        BOOL result = WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd, WM_COMMAND, IDCANCEL, 0);
        if (result != TRUE || g_downloadDlgAbortCalls != 1 ||
            g_downloadDlgAbortSelf != (IUnknown *)&g_downloadDlgFakeObject ||
            g_downloadDlgDestroyWindowCalls != 2 ||
            g_downloadDlgDestroyWindowHwnd[1] != (HWND)0x13572468)
        {
            failure = 8;
        }
    }

    if (failure == 0)
    {
        g_WestwoodOnlineUpgradeDownloadDialogResult = 11;
        g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0x456789ab;
        g_downloadDlgSetCurrentDirectoryCalls = 0;
        g_downloadDlgKillTimerCalls = 0;
        g_downloadDlgEndDialogCalls = 0;
        g_shutdownUnadviseCalls = 0;
        g_shutdownUnadviseCookie = 0;
        g_downloadDlgReleaseCalls = 0;
        BOOL result =
            WestwoodOnlineUpgradeProgressDialog::DlgProc(dialogHwnd,
                                                        WM_DESTROY,
                                                        0,
                                                        0);
        if (result != TRUE || g_downloadDlgKillTimerCalls != 1 ||
            g_downloadDlgKillTimerHwnd != dialogHwnd ||
            g_downloadDlgKillTimerId != 1 || g_shutdownUnadviseCalls != 1 ||
            g_shutdownUnadviseCookie != 0x456789ab ||
            g_downloadDlgReleaseCalls != 1)
        {
            failure = 9;
        }
        else if (g_downloadDlgSetCurrentDirectoryCalls != 1 ||
                 strcmp(g_downloadDlgSetCurrentDirectoryPath[0],
                        "C:\\PreviousCwd") != 0 ||
                 g_downloadDlgEndDialogCalls != 1 ||
                 g_downloadDlgEndDialogHwnd != dialogHwnd ||
                 g_downloadDlgEndDialogResult != 11)
        {
            failure = 10;
        }
    }

    if (failure == 0 &&
        WestwoodOnlineUpgradeProgressDialog::DlgProc(dialogHwnd,
                                                     WM_SETFONT,
                                                     0,
                                                     0) != TRUE)
    {
        failure = 11;
    }
    if (failure == 0 &&
        WestwoodOnlineUpgradeProgressDialog::DlgProc(dialogHwnd,
                                                     WM_USER,
                                                     0,
                                                     0) != FALSE)
    {
        failure = 12;
    }
    if (failure == 0 &&
        WestwoodOnlineUpgradeProgressDialog::DlgProc(dialogHwnd,
                                                     WM_COMMAND,
                                                     1,
                                                     0) != FALSE)
    {
        failure = 13;
    }

    if (g_pWestwoodOnlineUpgradeDownloadEventSink != 0)
    {
        WestwoodOnlineUpgradeDownloadEventSink *const sink =
            (WestwoodOnlineUpgradeDownloadEventSink *)
                g_pWestwoodOnlineUpgradeDownloadEventSink;
        DeleteCriticalSection(&sink->m_refCountAndLock.lock);
        ::operator delete(sink);
    }
    g_pWestwoodOnlineUpgradeDownload = oldDownload;
    g_pWestwoodOnlineUpgradeDownloadEventSink = oldSink;
    g_pWestwoodOnlineUpgradeDownloadReadyList = oldReadyList;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = oldCookie;
    g_WestwoodOnlineUpgradeEventSinkLiveCount = oldLiveCount;
    g_WestwoodOnlineUpgradeDownloadDialogResult = oldDialogResult;
    g_hWestwoodOnlineUpgradeProgressDialog = oldProgressHwnd;
    memcpy(g_WestwoodOnlineUpgradeDownloadReadyPromptText,
           oldPrompt,
           sizeof(oldPrompt));
    memcpy(g_WestwoodOnlineUpgradeDownloadRestoreCwd,
           oldRestoreCwd,
           sizeof(oldRestoreCwd));

    for (int index = 8; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_show_download_ready_list_smoke(void)
{
    WestwoodOnlineUpgradeDownloadReadyEntry *const oldReadyList =
        g_pWestwoodOnlineUpgradeDownloadReadyList;
    HINSTANCE const oldInstance = g_RecoilApp_hInstance;
    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    char oldPrompt[sizeof(g_WestwoodOnlineUpgradeDownloadReadyPromptText)];
    ImportFunctionPatch import = {};
    CodeFunctionPatch formatPatch = {};

    memcpy(oldPrompt,
           g_WestwoodOnlineUpgradeDownloadReadyPromptText,
           sizeof(oldPrompt));

    int failure = 0;
    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeDownloadReadyFormatMessage,
                           formatPatch) ||
        !PatchImportByName("USER32.dll", "DialogBoxParamA",
                           (void *)&FakeDownloadReadyDialogBoxParamA,
                           import))
    {
        failure = 90;
    }

    WestwoodOnlineUpgradeDownloadReadyEntry entries[3] = {};
    entries[0].m_next = &entries[1];
    entries[1].m_next = &entries[2];
    entries[2].m_next = 0;
    g_RecoilApp_hInstance = (HINSTANCE)0x10203040;
    g_RecoilApp_hWndMain = (HWND)0x50607080;
    g_pWestwoodOnlineUpgradeDownloadReadyList = 0;
    g_downloadReadyFormatCalls = 0;
    g_downloadReadyDialogCalls = 0;
    g_downloadReadyDialogResult[0] = 101;
    g_downloadReadyDialogResult[1] = 102;
    g_downloadReadyDialogResult[2] = 103;

    if (failure == 0)
    {
        int result = WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(&entries[0]);
        if (result != 1 || g_downloadReadyFormatCalls != 3 ||
            g_downloadReadyDialogCalls != 3 ||
            g_pWestwoodOnlineUpgradeDownloadReadyList != &entries[2])
        {
            failure = 1;
        }
        for (int index = 0; failure == 0 && index < 3; ++index)
        {
            if (g_downloadReadyFormatBuffer[index] !=
                    g_WestwoodOnlineUpgradeDownloadReadyPromptText ||
                g_downloadReadyFormatMaxChars[index] != 128 ||
                g_downloadReadyFormatMessageId[index] != 0x3043 ||
                g_downloadReadyFormatOrdinal[index] != index + 1 ||
                g_downloadReadyFormatTotal[index] != 3 ||
                g_downloadReadyDialogInstance[index] != g_RecoilApp_hInstance ||
                g_downloadReadyDialogTemplate[index] != (LPCSTR)162 ||
                g_downloadReadyDialogParent[index] != g_RecoilApp_hWndMain ||
                g_downloadReadyDialogProc[index] !=
                    WestwoodOnlineUpgradeProgressDialog::DlgProc ||
                g_downloadReadyDialogParam[index] != 0)
            {
                failure = 2 + index;
            }
        }
        if (failure == 0 &&
            strcmp(g_WestwoodOnlineUpgradeDownloadReadyPromptText,
                   "download 3 of 3") != 0)
        {
            failure = 5;
        }
    }

    if (failure == 0)
    {
        g_downloadReadyFormatCalls = 0;
        g_downloadReadyDialogCalls = 0;
        int result = WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(0);
        if (result != 1 || g_downloadReadyFormatCalls != 0 ||
            g_downloadReadyDialogCalls != 0)
        {
            failure = 6;
        }
    }

    if (failure == 0)
    {
        entries[0].m_next = &entries[1];
        entries[1].m_next = 0;
        g_downloadReadyFormatCalls = 0;
        g_downloadReadyDialogCalls = 0;
        g_downloadReadyDialogResult[0] = 201;
        g_downloadReadyDialogResult[1] = -1;
        int result = WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(&entries[0]);
        if (result != 0 || g_downloadReadyFormatCalls != 2 ||
            g_downloadReadyDialogCalls != 2 ||
            g_pWestwoodOnlineUpgradeDownloadReadyList != &entries[1] ||
            g_downloadReadyFormatOrdinal[1] != 2 ||
            g_downloadReadyFormatTotal[1] != 2)
        {
            failure = 7;
        }
    }

    g_pWestwoodOnlineUpgradeDownloadReadyList = oldReadyList;
    g_RecoilApp_hInstance = oldInstance;
    g_RecoilApp_hWndMain = oldMainHwnd;
    memcpy(g_WestwoodOnlineUpgradeDownloadReadyPromptText,
           oldPrompt,
           sizeof(oldPrompt));
    RestoreImportPatch(import);
    RestoreFunctionPatch(formatPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_api_event_sink_on_download_ready_smoke(void)
{
    const int oldAsyncError = g_WestwoodOnlineUpgradeApiAsyncErrorFlag;
    const int oldProcessCallbacks = g_WestwoodOnlineUpgradeProcessCallbacksFlag;
    HANDLE const oldFailureEvent = g_WestwoodOnlineUpgradeFailureEvent;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};
    ImportFunctionPatch imports[2] = {};

    int failure = 0;
    if (!PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[0]))
    {
        failure = 91;
    }
    else if (!PatchFunctionJump(
                 (void *)&WestwoodOnlineUpgradeDialog::ShowDownloadReadyList,
                 (void *)&FakeDownloadReadyCallbackShowList,
                 patches[1]))
    {
        failure = 92;
    }
    else if (!PatchImportByOrdinal("MFC42.DLL",
                                   4224,
                                   (void *)&FakeDownloadReadyCallbackMessageBoxA,
                                   imports[0]))
    {
        failure = 93;
    }
    else if (!PatchImportByName("KERNEL32.dll",
                                "SetEvent",
                                (void *)&FakeDownloadReadyCallbackSetEvent,
                                imports[1]))
    {
        failure = 94;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_hWnd = (HWND)0x11112222;
    WestwoodOnlineUpgradeDownloadReadyEntry entry = {};
    HANDLE const failureEvent = (HANDLE)0x76543210;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_WestwoodOnlineUpgradeFailureEvent = failureEvent;

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 1;
    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 77;
    g_downloadReadyCallbackSetEventCalls = 0;
    g_downloadReadyCallbackMessageBoxCalls = 0;
    g_downloadReadyCallbackShowListCalls = 0;
    int result = WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult(
        0, 0, &entry);
    if (failure == 0 &&
        (result != 0 || g_WestwoodOnlineUpgradeProcessCallbacksFlag != 77 ||
         g_downloadReadyCallbackSetEventCalls != 0 ||
         g_downloadReadyCallbackMessageBoxCalls != 0 ||
         g_downloadReadyCallbackShowListCalls != 0))
    {
        failure = 1;
    }

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    g_downloadReadyCallbackSetEventCalls = 0;
    result = WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult(
        0, -5, 0);
    if (failure == 0 &&
        (result != 0 || g_downloadReadyCallbackSetEventCalls != 1 ||
         g_downloadReadyCallbackSetEventHandle[0] != failureEvent ||
         g_downloadReadyCallbackMessageBoxCalls != 0))
    {
        failure = 2;
    }

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 99;
    g_downloadReadyCallbackSetEventCalls = 0;
    g_downloadReadyCallbackMessageBoxCalls = 0;
    g_downloadReadyCallbackMessageBoxResult[0] = IDCANCEL;
    g_downloadReadyCallbackShowListCalls = 0;
    result = WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult(
        0, 0, &entry);
    if (failure == 0 &&
        (result != 0 || g_WestwoodOnlineUpgradeProcessCallbacksFlag != 1 ||
         g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 1 ||
         g_downloadReadyCallbackSetEventCalls != 1 ||
         g_downloadReadyCallbackSetEventHandle[0] != failureEvent ||
         g_downloadReadyCallbackMessageBoxCalls != 1 ||
         g_downloadReadyCallbackShowListCalls != 0))
    {
        failure = 3;
    }
    else if (failure == 0 &&
             (g_downloadReadyCallbackMessageBoxThis != &dialog ||
              strcmp(g_downloadReadyCallbackMessageBoxCaption, "msg-3001") != 0 ||
              strcmp(g_downloadReadyCallbackMessageBoxText, "msg-3002") != 0 ||
              g_downloadReadyCallbackMessageBoxType !=
                  (MB_YESNO | MB_ICONQUESTION)))
    {
        failure = 4;
    }

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 99;
    g_downloadReadyCallbackSetEventCalls = 0;
    g_downloadReadyCallbackMessageBoxCalls = 0;
    g_downloadReadyCallbackMessageBoxResult[0] = IDYES;
    g_downloadReadyCallbackShowListCalls = 0;
    g_downloadReadyCallbackShowList = 0;
    g_downloadReadyCallbackShowListResult = 0;
    result = WestwoodOnlineUpgradeApiEventSink::OnDownloadReadyResult(
        0, 0, &entry);
    if (failure == 0 &&
        (result != 0 || g_WestwoodOnlineUpgradeProcessCallbacksFlag != 1 ||
         g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0 ||
         g_downloadReadyCallbackSetEventCalls != 0 ||
         g_downloadReadyCallbackMessageBoxCalls != 1 ||
         g_downloadReadyCallbackShowListCalls != 1 ||
         g_downloadReadyCallbackShowList != &entry))
    {
        failure = 5;
    }

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = oldAsyncError;
    g_WestwoodOnlineUpgradeProcessCallbacksFlag = oldProcessCallbacks;
    g_WestwoodOnlineUpgradeFailureEvent = oldFailureEvent;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    for (int index = 1; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    for (int index = 1; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}

extern "C" int
westwood_online_upgrade_api_event_sink_on_pending_session_removed_smoke(void)
{
    const WORD kMfc42CWndSendDlgItemMessageAOrdinal = 5802;
    const int oldPendingSessionResultCount =
        g_WestwoodOnlineUpgradePendingSessionResultCount;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};
    ImportFunctionPatch import = {};

    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakePendingSessionRemovedFormatMessage,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakePendingSessionRemovedAppendStatusTextFmt,
            patches[1]) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndSendDlgItemMessageAOrdinal,
                              (void *)&FakePendingSessionRemovedSendDlgItemMessageA,
                              import))
    {
        RestoreImportPatch(import);
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    WestwoodOnlineUpgradeSessionRequest request = {};
    strcpy(request.m_sessionName, "Alpha Session");

    int failure = 0;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 7;
    ResetPendingSessionRemovedFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved(
            0, -1, &request);
    if (result != 0 || g_WestwoodOnlineUpgradePendingSessionResultCount != 7 ||
        g_pendingRemovedFormatCalls != 0 ||
        g_pendingRemovedAppendCalls != 0 ||
        g_pendingRemovedSendMessageCalls != 0)
    {
        failure = 1;
    }

    g_WestwoodOnlineUpgradePendingSessionResultCount = 7;
    ResetPendingSessionRemovedFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved(
        0, 0, &request);
    if (failure == 0 &&
        (result != 0 || g_WestwoodOnlineUpgradePendingSessionResultCount != 7 ||
         g_pendingRemovedFormatCalls != 1 ||
         g_pendingRemovedFormatBuffer == 0 ||
         g_pendingRemovedFormatMaxChars != 128 ||
         g_pendingRemovedFormatMessageId != 0x3004 ||
         g_pendingRemovedFormatSessionName != request.m_sessionName ||
         g_pendingRemovedAppendCalls != 1 ||
         g_pendingRemovedAppendThis != &dialog ||
         strcmp(g_pendingRemovedAppendText, "pending removed status") != 0 ||
         g_pendingRemovedSendMessageCalls != 1))
    {
        failure = 2;
    }
    else if (failure == 0 &&
             (g_pendingRemovedSendMessageThis[0] != (CWnd *)&dialog ||
              g_pendingRemovedSendMessageControlId[0] != 1137 ||
              g_pendingRemovedSendMessageMessage[0] != LB_FINDSTRINGEXACT ||
              g_pendingRemovedSendMessageWParam[0] != (WPARAM)-1 ||
              g_pendingRemovedSendMessageLParam[0] !=
                  (LPARAM)request.m_sessionName))
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradePendingSessionResultCount = 7;
    ResetPendingSessionRemovedFakes();
    g_pendingRemovedSendMessageResult[0] = 2;
    result = WestwoodOnlineUpgradeApiEventSink::OnPendingSessionRequestRemoved(
        0, 0, &request);
    if (failure == 0 &&
        (result != 0 || g_WestwoodOnlineUpgradePendingSessionResultCount != 6 ||
         g_pendingRemovedSendMessageCalls != 2))
    {
        failure = 4;
    }
    else if (failure == 0 &&
             (g_pendingRemovedSendMessageThis[1] != (CWnd *)&dialog ||
              g_pendingRemovedSendMessageControlId[1] != 1137 ||
              g_pendingRemovedSendMessageMessage[1] != LB_DELETESTRING ||
              g_pendingRemovedSendMessageWParam[1] != 2 ||
              g_pendingRemovedSendMessageLParam[1] != 0))
    {
        failure = 5;
    }

    g_WestwoodOnlineUpgradePendingSessionResultCount =
        oldPendingSessionResultCount;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreImportPatch(import);
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}

extern "C" int westwood_online_upgrade_api_event_sink_on_server_error_smoke(void)
{
    const WORD kMfc42CWndMessageBoxAOrdinal = 4224;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    ImportFunctionPatch import = {};

    if (!PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndMessageBoxAOrdinal,
                              (void *)&FakeServerErrorMessageBoxA,
                              import))
    {
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    g_serverErrorMessageBoxCalls = 0;
    g_serverErrorMessageBoxThis = 0;
    g_serverErrorMessageBoxText[0] = '\0';
    g_serverErrorMessageBoxCaption[0] = '\0';
    g_serverErrorMessageBoxType = 0;
    int result = WestwoodOnlineUpgradeApiEventSink::OnServerError(
        0, -123, "Server refused the request");

    int failure = 0;
    if (result != 0 ||
        g_serverErrorMessageBoxCalls != 1 ||
        g_serverErrorMessageBoxThis != (CWnd *)&dialog ||
        strcmp(g_serverErrorMessageBoxText, "Server refused the request") != 0 ||
        strcmp(g_serverErrorMessageBoxCaption, "ServerError") != 0 ||
        g_serverErrorMessageBoxType != MB_ICONHAND)
    {
        failure = 1;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreImportPatch(import);
    return failure;
}

extern "C" int westwood_online_upgrade_api_event_sink_on_api_status_smoke(void)
{
    const WORD kMfc42CWndMessageBoxAOrdinal = 4224;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const int oldAsyncError = g_WestwoodOnlineUpgradeApiAsyncErrorFlag;
    HANDLE const oldStatusEvent = g_WestwoodOnlineUpgradeStatusTextEvent;
    HANDLE const oldFailureEvent = g_WestwoodOnlineUpgradeFailureEvent;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    WestwoodOnlineUpgradeProgressDialog *const oldProgressDialog =
        g_pWestwoodOnlineUpgradeProgressDialog;
    RecoilPtr32 const oldMainWnd = g_RecoilApp.m_pMainWnd;
    CodeFunctionPatch patches[3] = {};
    ImportFunctionPatch imports[2] = {};

    if (!PatchFunctionJump((void *)&Time::Reset,
                           (void *)&FakeApiStatusTimeReset,
                           patches[0]) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[1]) ||
        !PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeApiStatusFormatMessage,
                           patches[2]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeApiStatusAppendStatusTextFmt,
            patches[3]) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndMessageBoxAOrdinal,
                              (void *)&FakeServerErrorMessageBoxA,
                              imports[0]) ||
        !PatchImportByName("KERNEL32.dll",
                           "SetEvent",
                           (void *)&FakeDownloadReadyCallbackSetEvent,
                           imports[1]))
    {
        for (int index = 1; index >= 0; --index)
        {
            RestoreImportPatch(imports[index]);
        }
        for (int index = 3; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    unsigned char progressStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] =
        {0};
    WestwoodOnlineUpgradeProgressDialog &progressDialog =
        *(WestwoodOnlineUpgradeProgressDialog *)progressStorage;
    unsigned char mainWndStorage[sizeof(CWnd)] = {0};
    CWnd &mainWnd = *(CWnd *)mainWndStorage;

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.SetQueryMode = FakeApiStatusSetQueryMode;
    g_initFakeApiVtable.GetQueryResultCount = FakeApiStatusGetQueryResultCount;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_initProgressVtable.DestroyWindow = FakeInitDestroyProgress;
    *(void **)&progressDialog = &g_initProgressVtable;
    g_pWestwoodOnlineUpgradeProgressDialog = &progressDialog;
    g_RecoilApp.m_pMainWnd = (RecoilPtr32)((unsigned int)&mainWnd);
    g_WestwoodOnlineUpgradeStatusTextEvent = (HANDLE)0x12345670;
    g_WestwoodOnlineUpgradeFailureEvent = (HANDLE)0x12345671;

    int failure = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 0;
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 77;
    g_apiStatusQueryResultCount = 0;
    ResetApiStatusFakes();
    int result = WestwoodOnlineUpgradeApiEventSink::OnApiStatus(
        0, 0, "Alpha\rBeta");
    if (result != 0 ||
        g_apiStatusTimeResetCalls != 1 ||
        g_apiStatusAppendCalls != 3 ||
        strcmp(g_apiStatusAppendText[0], "Alpha") != 0 ||
        strcmp(g_apiStatusAppendText[1], "Beta") != 0 ||
        strcmp(g_apiStatusAppendText[2], "msg-3005") != 0 ||
        g_apiStatusAppendThis[0] != &dialog ||
        g_downloadReadyCallbackSetEventCalls != 1 ||
        g_downloadReadyCallbackSetEventHandle[0] !=
            g_WestwoodOnlineUpgradeStatusTextEvent ||
        g_apiStatusSetQueryModeCalls != 1 ||
        g_apiStatusSetQueryModeSelf != (IUnknown *)&g_initFakeApi ||
        g_apiStatusSetQueryModeValue != 17 ||
        g_WestwoodOnlineUpgradeActiveListMode != 17 ||
        g_apiStatusGetQueryResultCountCalls != 1 ||
        g_apiStatusGetQueryResultCountSelf != (IUnknown *)&g_initFakeApi ||
        g_apiStatusFormatCalls != 0 ||
        g_initDestroyProgressCalls != 0 ||
        g_serverErrorMessageBoxCalls != 0 ||
        g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 77)
    {
        failure = 1;
    }

    g_apiStatusQueryResultCount = 1;
    ResetApiStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnApiStatus(0, 0, "Solo");
    if (failure == 0 &&
        (result != 0 || g_apiStatusAppendCalls != 2 ||
         strcmp(g_apiStatusAppendText[0], "Solo") != 0 ||
         strcmp(g_apiStatusAppendText[1], "msg-3006") != 0 ||
         g_apiStatusFormatCalls != 0))
    {
        failure = 2;
    }

    g_apiStatusQueryResultCount = 3;
    ResetApiStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnApiStatus(0, 0, "");
    if (failure == 0 &&
        (result != 0 || g_apiStatusAppendCalls != 1 ||
         strcmp(g_apiStatusAppendText[0], "result count 3") != 0 ||
         g_apiStatusFormatCalls != 1 ||
         g_apiStatusFormatBuffer == 0 ||
         g_apiStatusFormatMaxChars != 128 ||
         g_apiStatusFormatMessageId != 0x3007 ||
         g_apiStatusFormatResultCount != 3))
    {
        failure = 3;
    }

    const int statusCodes[5] = {
        (int)0x80040064,
        (int)0x80040065,
        (int)0x8004006a,
        (int)0x80040072,
        -5};
    const unsigned int messageIds[5] = {
        0x3008,
        0x3009,
        0x300b,
        0x300a,
        0x300c};
    const UINT messageBoxTypes[5] = {
        MB_ICONEXCLAMATION,
        MB_ICONEXCLAMATION,
        MB_ICONEXCLAMATION,
        MB_ICONEXCLAMATION,
        MB_ICONHAND};

    for (int index = 0; failure == 0 && index < 5; ++index)
    {
        g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
        ResetApiStatusFakes();
        result = WestwoodOnlineUpgradeApiEventSink::OnApiStatus(
            0, statusCodes[index], "ignored");
        char expectedMessage[32];
        wsprintfA(expectedMessage, "msg-%04x", messageIds[index]);
        if (result != 0 ||
            g_apiStatusTimeResetCalls != 1 ||
            g_initDestroyProgressCalls != 1 ||
            g_initDestroyedProgress != (CWnd *)&progressDialog ||
            g_initMessageIdCalls != 2 ||
            g_initMessageIds[0] != 0x3003 ||
            g_initMessageIds[1] != messageIds[index] ||
            g_serverErrorMessageBoxCalls != 1 ||
            g_serverErrorMessageBoxThis != &mainWnd ||
            strcmp(g_serverErrorMessageBoxCaption, "msg-3003") != 0 ||
            strcmp(g_serverErrorMessageBoxText, expectedMessage) != 0 ||
            g_serverErrorMessageBoxType != messageBoxTypes[index] ||
            g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 1 ||
            g_downloadReadyCallbackSetEventCalls != 1 ||
            g_downloadReadyCallbackSetEventHandle[0] !=
                g_WestwoodOnlineUpgradeFailureEvent ||
            g_apiStatusAppendCalls != 0 ||
            g_apiStatusSetQueryModeCalls != 0)
        {
            failure = 4 + index;
        }
    }

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = oldAsyncError;
    g_WestwoodOnlineUpgradeStatusTextEvent = oldStatusEvent;
    g_WestwoodOnlineUpgradeFailureEvent = oldFailureEvent;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeProgressDialog = oldProgressDialog;
    g_RecoilApp.m_pMainWnd = oldMainWnd;
    for (int index = 1; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    for (int index = 3; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}

extern "C" int
westwood_online_upgrade_api_event_sink_on_status_text_received_smoke(void)
{
    const HANDLE oldStatusEvent = g_WestwoodOnlineUpgradeStatusTextEvent;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch appendPatch = {};
    ImportFunctionPatch setEventImport = {};

    if (!PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeApiStatusAppendStatusTextFmt,
            appendPatch) ||
        !PatchImportByName("KERNEL32.dll",
                           "SetEvent",
                           (void *)&FakeDownloadReadyCallbackSetEvent,
                           setEventImport))
    {
        RestoreImportPatch(setEventImport);
        RestoreFunctionPatch(appendPatch);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_WestwoodOnlineUpgradeStatusTextEvent = (HANDLE)0x34567891;

    int failure = 0;
    ResetApiStatusFakes();
    int result = WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived(
        0, -1, "Alpha\rBeta");
    if (result != 0 ||
        g_apiStatusAppendCalls != 0 ||
        g_downloadReadyCallbackSetEventCalls != 0)
    {
        failure = 1;
    }

    ResetApiStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived(
        0, 0, "Alpha\rBeta\rGamma");
    if (failure == 0 &&
        (result != 0 ||
         g_apiStatusAppendCalls != 3 ||
         g_apiStatusAppendThis[0] != &dialog ||
         strcmp(g_apiStatusAppendText[0], "Alpha") != 0 ||
         strcmp(g_apiStatusAppendText[1], "Beta") != 0 ||
         strcmp(g_apiStatusAppendText[2], "Gamma") != 0 ||
         g_downloadReadyCallbackSetEventCalls != 1 ||
         g_downloadReadyCallbackSetEventHandle[0] !=
             g_WestwoodOnlineUpgradeStatusTextEvent))
    {
        failure = 2;
    }

    ResetApiStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnStatusTextReceived(
        0, 0, "");
    if (failure == 0 &&
        (result != 0 ||
         g_apiStatusAppendCalls != 0 ||
         g_downloadReadyCallbackSetEventCalls != 1 ||
         g_downloadReadyCallbackSetEventHandle[0] !=
             g_WestwoodOnlineUpgradeStatusTextEvent))
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradeStatusTextEvent = oldStatusEvent;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreImportPatch(setEventImport);
    RestoreFunctionPatch(appendPatch);
    return failure;
}

extern "C" int
westwood_online_upgrade_api_event_sink_on_browse_record_added_smoke(void)
{
    const WORD kMfc42CWndSetDlgItemTextAOrdinal = 5953;
    const WORD kMfc42CWndSendDlgItemMessageAOrdinal = 5802;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const int oldCreateFromQuery =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    WestwoodOnlineUpgradeBrowseRecord oldCachedRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;
    CodeFunctionPatch patches[4] = {};
    ImportFunctionPatch imports[2] = {};

    if (!PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[0]) ||
        !PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeBrowseRecordAddedFormatMessage,
                           patches[1]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeApiStatusAppendStatusTextFmt,
            patches[2]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::EnableQueryControls),
            (void *)&FakeBrowseRecordAddedEnableQueryControls,
            patches[3]) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndSetDlgItemTextAOrdinal,
                              (void *)&FakeInitSetDlgItemTextA,
                              imports[0]) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndSendDlgItemMessageAOrdinal,
                              (void *)&FakePendingSessionRemovedSendDlgItemMessageA,
                              imports[1]))
    {
        for (int index = 1; index >= 0; --index)
        {
            RestoreImportPatch(imports[index]);
        }
        for (int index = 3; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    WestwoodOnlineUpgradeBrowseRecord record = {};
    strcpy(record.m_sessionName, "Alpha");
    strcpy(record.m_serverAddress, "alpha.example");

    int failure = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 11;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 77;
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName, "cached");
    ResetBrowseRecordAddedFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded(0, -1, &record);
    if (result != 0 ||
        g_initMessageIdCalls != 1 ||
        g_initMessageIds[0] != 0x300d ||
        g_apiStatusAppendCalls != 1 ||
        strcmp(g_apiStatusAppendText[0], "msg-300d") != 0 ||
        g_browseRecordAddedEnableCalls != 1 ||
        g_browseRecordAddedEnableThis[0] != &dialog ||
        g_browseRecordAddedEnableValue[0] != 0 ||
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0' ||
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0 ||
        g_initRequestListModeCalls != 1 ||
        g_initRequestListMode != 11 ||
        g_initRequestListModeEnabled != 1 ||
        g_browseRecordAddedFormatCalls != 0 ||
        g_initSetDlgItemTextCalls != 0 ||
        g_pendingRemovedSendMessageCalls != 0)
    {
        failure = 1;
    }

    record.m_recordFlags = 0;
    strcpy(record.m_sessionName, "Alpha");
    strcpy(record.m_serverAddress, "alpha.example");
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    ResetBrowseRecordAddedFakes();
    result =
        WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded(0, 0, &record);
    if (failure == 0 &&
        (result != 0 ||
         g_browseRecordAddedFormatCalls != 1 ||
         g_browseRecordAddedFormatBuffer == 0 ||
         g_browseRecordAddedFormatMaxChars != 128 ||
         g_browseRecordAddedFormatMessageId != 0x300e ||
         g_browseRecordAddedFormatSessionName != record.m_sessionName ||
         g_apiStatusAppendCalls != 1 ||
         strcmp(g_apiStatusAppendText[0], "browse 300e Alpha") != 0 ||
         g_browseRecordAddedEnableCalls != 0 ||
         g_initSetDlgItemTextCalls != 0 ||
         g_pendingRemovedSendMessageCalls != 1 ||
         g_pendingRemovedSendMessageThis[0] != (CWnd *)&dialog ||
         g_pendingRemovedSendMessageControlId[0] != 1136 ||
         g_pendingRemovedSendMessageMessage[0] != LB_INSERTSTRING ||
         g_pendingRemovedSendMessageWParam[0] != (WPARAM)-1 ||
         g_pendingRemovedSendMessageLParam[0] !=
             (LPARAM)record.m_sessionName ||
         g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1 ||
         memcmp(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
                &record,
                sizeof(record)) != 0 ||
         g_initRequestListModeCalls != 0))
    {
        failure = 2;
    }

    record.m_recordFlags = 1;
    strcpy(record.m_sessionName, "Beta");
    strcpy(record.m_serverAddress, "beta.example");
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    ResetBrowseRecordAddedFakes();
    result =
        WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAdded(0, 0, &record);
    if (failure == 0 &&
        (result != 0 ||
         g_browseRecordAddedFormatCalls != 1 ||
         g_browseRecordAddedFormatMessageId != 0x300f ||
         g_browseRecordAddedFormatSessionName != record.m_sessionName ||
         g_apiStatusAppendCalls != 1 ||
         strcmp(g_apiStatusAppendText[0], "browse 300f Beta") != 0 ||
         g_browseRecordAddedEnableCalls != 1 ||
         g_browseRecordAddedEnableThis[0] != &dialog ||
         g_browseRecordAddedEnableValue[0] != 1 ||
         g_initSetDlgItemTextCalls != 1 ||
         g_initSetDlgItemTextThis[0] != &dialog ||
         g_initSetDlgItemTextControlId[0] != 1174 ||
         strcmp(g_initSetDlgItemTextValue[0], "Beta") != 0 ||
         g_pendingRemovedSendMessageCalls != 1 ||
         g_pendingRemovedSendMessageControlId[0] != 1136 ||
         g_pendingRemovedSendMessageMessage[0] != LB_INSERTSTRING ||
         g_pendingRemovedSendMessageLParam[0] !=
             (LPARAM)record.m_sessionName ||
         g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1 ||
         memcmp(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
                &record,
                sizeof(record)) != 0))
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFromQuery;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedRecord;
    for (int index = 1; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    for (int index = 3; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}

extern "C" int
westwood_online_upgrade_api_event_sink_on_browse_record_and_session_resolved_smoke(void)
{
    const WORD kMfc42CWndSetDlgItemTextAOrdinal = 5953;
    const WORD kMfc42CWndSendDlgItemMessageAOrdinal = 5802;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const int oldCreateFromQuery =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldPendingSessionCount =
        g_WestwoodOnlineUpgradePendingSessionResultCount;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    WestwoodOnlineUpgradeBrowseRecord oldCachedRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;
    CodeFunctionPatch patches[6] = {};
    ImportFunctionPatch imports[2] = {};

    if (!PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[0]) ||
        !PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeBrowseResolvedFormatMessage,
                           patches[1]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeBrowseResolvedAppendStatusTextFmt,
            patches[2]) ||
        !PatchFunctionJump(
            MethodAddress(
                &WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls),
            (void *)&FakeBrowseResolvedUpdateSessionListQueryFromControls,
            patches[3]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::EnableQueryControls),
            (void *)&FakeBrowseRecordAddedEnableQueryControls,
            patches[4]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::EnableConnectButton),
            (void *)&FakeBrowseResolvedEnableConnectButton,
            patches[5]) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndSetDlgItemTextAOrdinal,
                              (void *)&FakeInitSetDlgItemTextA,
                              imports[0]) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndSendDlgItemMessageAOrdinal,
                              (void *)&FakePendingSessionRemovedSendDlgItemMessageA,
                              imports[1]))
    {
        for (int index = 1; index >= 0; --index)
        {
            RestoreImportPatch(imports[index]);
        }
        for (int index = 5; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_initFakeApiVtable.RequestSessionDetails =
        FakeBrowseResolvedRequestSessionDetails;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    WestwoodOnlineUpgradeBrowseRecord browseRecord = {};
    strcpy(browseRecord.m_sessionName, "Browse");
    strcpy(browseRecord.m_serverAddress, "browse.example");
    WestwoodOnlineUpgradeSessionRequest sessionRequest = {};
    strcpy(sessionRequest.m_sessionName, "Session");

    int failure = 0;
    const int failureStatuses[6] = {
        (int)0x8004006c,
        (int)0x80040070,
        (int)0x80040072,
        (int)0x80040071,
        (int)0x8004006e,
        -5};
    const char *const expectedFailureText[6] = {
        "msg-3011 msg-3012",
        "msg-3011 msg-3013",
        "msg-3011 msg-300a",
        "msg-3011 msg-3014",
        "msg-3011 msg-3010",
        "%s %x"};

    for (int index = 0; failure == 0 && index < 6; ++index)
    {
        g_WestwoodOnlineUpgradeActiveListMode = 23 + index;
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 77;
        strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName,
               "cached");
        ResetBrowseResolvedFakes();
        const int result =
            WestwoodOnlineUpgradeApiEventSink::
                OnBrowseRecordAndSessionResolved(
                    0,
                    failureStatuses[index],
                    &browseRecord,
                    &sessionRequest
                );
        if (result != 0 ||
            g_browseResolvedAppendCalls != 1 ||
            g_browseResolvedAppendThis[0] != &dialog ||
            strcmp(g_browseResolvedAppendFormat[0],
                   expectedFailureText[index]) != 0 ||
            g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0 ||
            g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] !=
                '\0' ||
            g_initRequestListModeCalls != 1 ||
            g_initRequestListMode != 23 + index ||
            g_initRequestListModeEnabled != 1 ||
            g_browseRecordAddedEnableCalls != 0 ||
            g_browseResolvedFormatCalls != 0 ||
            g_pendingRemovedSendMessageCalls != 0 ||
            g_browseResolvedRequestDetailsCalls != 0)
        {
            failure = 1 + index;
        }
        else if (index == 5 &&
                 (g_browseResolvedAppendArgText[0] == 0 ||
                  strcmp(g_browseResolvedAppendArgText[0], "msg-3011") != 0 ||
                  g_browseResolvedAppendArgStatus[0] != failureStatuses[index]))
        {
            failure = 7;
        }
    }

    browseRecord.m_recordFlags = 7;
    sessionRequest.m_rowFlags = 0x8001;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 4;
    ResetBrowseResolvedFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved(
            0,
            0,
            &browseRecord,
            &sessionRequest
        );
    if (failure == 0 &&
        (result != 0 ||
         memcmp(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
                &browseRecord,
                sizeof(browseRecord)) != 0 ||
         g_browseRecordAddedEnableCalls != 1 ||
         g_browseRecordAddedEnableThis[0] != &dialog ||
         g_browseRecordAddedEnableValue[0] != 0 ||
         g_initRequestListModeCalls != 0 ||
         g_browseResolvedRequestDetailsCalls != 0 ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 4))
    {
        failure = 8;
    }

    browseRecord.m_recordFlags = 0;
    sessionRequest.m_rowFlags = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 17;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 4;
    ResetBrowseResolvedFakes();
    result =
        WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved(
            0,
            0,
            &browseRecord,
            &sessionRequest
        );
    if (failure == 0 &&
        (result != 0 ||
         g_initRequestListModeCalls != 1 ||
         g_initRequestListMode != 17 ||
         g_initRequestListModeEnabled != 1 ||
         g_browseResolvedUpdateCalls != 0 ||
         g_initSetDlgItemTextCalls != 1 ||
         g_initSetDlgItemTextThis[0] != &dialog ||
         g_initSetDlgItemTextControlId[0] != 1174 ||
         strcmp(g_initSetDlgItemTextValue[0], "Browse") != 0 ||
         g_browseResolvedFormatCalls != 1 ||
         g_browseResolvedFormatMaxChars != 128 ||
         g_browseResolvedFormatMessageId != 0x3015 ||
         g_browseResolvedFormatSessionName != sessionRequest.m_sessionName ||
         g_browseResolvedFormatBrowseName != browseRecord.m_sessionName ||
         g_browseResolvedAppendCalls != 1 ||
         strcmp(g_browseResolvedAppendFormat[0],
                "resolved 3015 Session Browse") != 0 ||
         g_pendingRemovedSendMessageCalls != 1 ||
         g_pendingRemovedSendMessageControlId[0] != 1137 ||
         g_pendingRemovedSendMessageMessage[0] != LB_ADDSTRING ||
         g_pendingRemovedSendMessageWParam[0] != 0 ||
         g_pendingRemovedSendMessageLParam[0] !=
             (LPARAM)sessionRequest.m_sessionName ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 5 ||
         g_browseResolvedConnectCalls != 1 ||
         g_browseResolvedConnectThis[0] != &dialog ||
         g_browseResolvedConnectValue[0] != 1 ||
         g_browseResolvedRequestDetailsCalls != 1 ||
         g_browseResolvedRequestDetailsSelf != (IUnknown *)&g_initFakeApi ||
         g_browseResolvedRequestDetailsRequest != &sessionRequest))
    {
        failure = 9;
    }

    browseRecord.m_recordFlags = 1;
    sessionRequest.m_rowFlags = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 18;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 9;
    ResetBrowseResolvedFakes();
    result =
        WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordAndSessionResolved(
            0,
            0,
            &browseRecord,
            &sessionRequest
        );
    if (failure == 0 &&
        (result != 0 ||
         g_browseResolvedUpdateCalls != 1 ||
         g_browseResolvedUpdateThis != &dialog ||
         g_initRequestListModeCalls != 1 ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 10 ||
         g_browseResolvedRequestDetailsCalls != 1))
    {
        failure = 10;
    }

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFromQuery;
    g_WestwoodOnlineUpgradePendingSessionResultCount = oldPendingSessionCount;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedRecord;
    for (int index = 1; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    for (int index = 5; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}

extern "C" int
westwood_online_upgrade_api_event_sink_on_session_query_finished_smoke(void)
{
    const WORD kMfc42CWndSendDlgItemMessageAOrdinal = 5802;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const int oldPendingSessionCount =
        g_WestwoodOnlineUpgradePendingSessionResultCount;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[4] = {};
    ImportFunctionPatch import = {};

    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeBrowseResolvedFormatMessage,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeBrowseResolvedAppendStatusTextFmt,
            patches[1]) ||
        !PatchFunctionJump(
            MethodAddress(
                &WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList),
            (void *)&FakeSessionFinishedAppendConnectStatusAndRefreshList,
            patches[2]) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndSendDlgItemMessageAOrdinal,
                              (void *)&FakePendingSessionRemovedSendDlgItemMessageA,
                              import))
    {
        RestoreImportPatch(import);
        for (int index = 2; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_initFakeApiVtable.CancelPendingSessionFlow =
        FakeSessionFinishedCancelPendingSessionFlow;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    WestwoodOnlineUpgradeBrowseRecord browseRecord = {};
    strcpy(browseRecord.m_sessionName, "Browse");
    WestwoodOnlineUpgradeSessionRequest sessionRequest = {};
    strcpy(sessionRequest.m_sessionName, "Session");

    int failure = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 33;
    ResetBrowseResolvedFakes();
    int result = WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished(
        0,
        -1,
        &browseRecord,
        &sessionRequest
    );
    if (result != 0 ||
        g_initRequestListModeCalls != 1 ||
        g_initRequestListMode != 17 ||
        g_initRequestListModeEnabled != 1 ||
        g_sessionFinishedCancelCalls != 1 ||
        g_sessionFinishedCancelSelf != (IUnknown *)&g_initFakeApi ||
        g_browseResolvedFormatCalls != 0 ||
        g_browseResolvedAppendCalls != 0 ||
        g_pendingRemovedSendMessageCalls != 0)
    {
        failure = 1;
    }

    sessionRequest.m_rowFlags = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 21;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 4;
    ResetBrowseResolvedFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished(
        0,
        0,
        &browseRecord,
        &sessionRequest
    );
    if (failure == 0 &&
        (result != 0 ||
         g_browseResolvedFormatCalls != 1 ||
         g_browseResolvedFormatMaxChars != 128 ||
         g_browseResolvedFormatMessageId != 0x3016 ||
         g_browseResolvedFormatSessionName != sessionRequest.m_sessionName ||
         g_browseResolvedFormatBrowseName != browseRecord.m_sessionName ||
         g_browseResolvedAppendCalls != 1 ||
         strcmp(g_browseResolvedAppendFormat[0],
                "resolved 3016 Session Browse") != 0 ||
         g_pendingRemovedSendMessageCalls != 1 ||
         g_pendingRemovedSendMessageControlId[0] != 1137 ||
         g_pendingRemovedSendMessageMessage[0] != LB_FINDSTRINGEXACT ||
         g_pendingRemovedSendMessageWParam[0] != (WPARAM)-1 ||
         g_pendingRemovedSendMessageLParam[0] !=
             (LPARAM)sessionRequest.m_sessionName ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 4 ||
         g_sessionFinishedAppendConnectCalls != 0 ||
         g_initRequestListModeCalls != 1 ||
         g_initRequestListMode != 21 ||
         g_initRequestListModeEnabled != 1))
    {
        failure = 2;
    }

    sessionRequest.m_rowFlags = 1;
    g_WestwoodOnlineUpgradeActiveListMode = 22;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 7;
    ResetBrowseResolvedFakes();
    g_pendingRemovedSendMessageResult[0] = 3;
    result = WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished(
        0,
        0,
        &browseRecord,
        &sessionRequest
    );
    if (failure == 0 &&
        (result != 0 ||
         g_pendingRemovedSendMessageCalls != 2 ||
         g_pendingRemovedSendMessageMessage[1] != LB_DELETESTRING ||
         g_pendingRemovedSendMessageWParam[1] != 3 ||
         g_pendingRemovedSendMessageLParam[1] != 0 ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 6 ||
         g_sessionFinishedAppendConnectCalls != 1 ||
         g_sessionFinishedAppendConnectThis != &dialog ||
         g_sessionFinishedAppendConnectSessionName !=
             browseRecord.m_sessionName ||
         g_initRequestListModeCalls != 1 ||
         g_initRequestListMode != 22))
    {
        failure = 3;
    }

    sessionRequest.m_rowFlags = 0x8001;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 7;
    ResetBrowseResolvedFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnSessionQueryFinished(
        0,
        0,
        &browseRecord,
        &sessionRequest
    );
    if (failure == 0 &&
        (result != 0 ||
         g_sessionFinishedAppendConnectCalls != 0 ||
         g_initRequestListModeCalls != 1))
    {
        failure = 4;
    }

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradePendingSessionResultCount = oldPendingSessionCount;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreImportPatch(import);
    for (int index = 2; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}

extern "C" int
westwood_online_upgrade_api_event_sink_on_session_list_enumerated_smoke(void)
{
    const WORD kMfc42CWndSendDlgItemMessageAOrdinal = 5802;
    const int oldCreateFromQuery =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldPendingSessionCount =
        g_WestwoodOnlineUpgradePendingSessionResultCount;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[4] = {};
    ImportFunctionPatch import = {};

    if (!PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::EnableQueryControls),
            (void *)&FakeBrowseRecordAddedEnableQueryControls,
            patches[1]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::EnableConnectButton),
            (void *)&FakeBrowseResolvedEnableConnectButton,
            patches[2]) ||
        !PatchFunctionJump(
            MethodAddress(
                &WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList),
            (void *)&FakeSessionFinishedAppendConnectStatusAndRefreshList,
            patches[3]) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndSendDlgItemMessageAOrdinal,
                              (void *)&FakePendingSessionRemovedSendDlgItemMessageA,
                              import))
    {
        RestoreImportPatch(import);
        for (int index = 3; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    WestwoodOnlineUpgradeBrowseRecord browseRecord = {};
    strcpy(browseRecord.m_sessionName, "Browse");
    WestwoodOnlineUpgradeSessionRequest nodeA = {};
    WestwoodOnlineUpgradeSessionRequest nodeB = {};
    WestwoodOnlineUpgradeSessionRequest nodeC = {};
    strcpy(nodeA.m_sessionName, "Alpha");
    strcpy(nodeB.m_sessionName, "Beta");
    strcpy(nodeC.m_sessionName, "Gamma");
    nodeA.m_next = &nodeB;
    nodeB.m_next = &nodeC;
    nodeC.m_next = 0;

    int failure = 0;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 6;
    ResetBrowseResolvedFakes();
    int result = WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated(
        0,
        -1,
        &browseRecord,
        &nodeA
    );
    if (result != 0 ||
        g_pendingRemovedSendMessageCalls != 0 ||
        g_WestwoodOnlineUpgradePendingSessionResultCount != 6 ||
        g_sessionFinishedAppendConnectCalls != 0)
    {
        failure = 1;
    }

    g_WestwoodOnlineUpgradePendingSessionResultCount = 6;
    ResetBrowseResolvedFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated(
        0,
        0,
        &browseRecord,
        0
    );
    if (failure == 0 &&
        (result != 0 ||
         g_pendingRemovedSendMessageCalls != 1 ||
         g_pendingRemovedSendMessageControlId[0] != 1137 ||
         g_pendingRemovedSendMessageMessage[0] != LB_RESETCONTENT ||
         g_pendingRemovedSendMessageWParam[0] != 0 ||
         g_pendingRemovedSendMessageLParam[0] != 0 ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 0 ||
         g_sessionFinishedAppendConnectCalls != 1 ||
         g_sessionFinishedAppendConnectThis != &dialog ||
         g_sessionFinishedAppendConnectSessionName !=
             browseRecord.m_sessionName))
    {
        failure = 2;
    }

    nodeA.m_rowFlags = 0;
    nodeA.m_next = 0;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 6;
    ResetBrowseResolvedFakes();
    g_pendingRemovedSendMessageResult[1] = 5;
    result = WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated(
        0,
        0,
        &browseRecord,
        &nodeA
    );
    if (failure == 0 &&
        (result != 0 ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 1 ||
         g_pendingRemovedSendMessageCalls != 3 ||
         g_pendingRemovedSendMessageMessage[1] != LB_ADDSTRING ||
         strcmp(g_pendingRemovedSendMessageText[1], "Alpha") != 0 ||
         g_pendingRemovedSendMessageMessage[2] != LB_SETITEMDATA ||
         g_pendingRemovedSendMessageWParam[2] != 5 ||
         g_pendingRemovedSendMessageLParam[2] != 0 ||
         g_sessionFinishedAppendConnectCalls != 1 ||
         g_browseRecordAddedEnableCalls != 0 ||
         g_browseResolvedConnectCalls != 0 ||
         g_initMessageIdCalls != 0))
    {
        failure = 3;
    }

    nodeA.m_rowFlags = 0x8001;
    nodeB.m_rowFlags = 0x8000;
    nodeC.m_rowFlags = 1;
    nodeA.m_next = &nodeB;
    nodeB.m_next = &nodeC;
    nodeC.m_next = 0;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 9;
    ResetBrowseResolvedFakes();
    g_pendingRemovedSendMessageResult[1] = 7;
    g_pendingRemovedSendMessageResult[3] = 8;
    g_pendingRemovedSendMessageResult[5] = 9;
    result = WestwoodOnlineUpgradeApiEventSink::OnSessionListEnumerated(
        0,
        0,
        &browseRecord,
        &nodeA
    );
    if (failure == 0 &&
        (result != 0 ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 3 ||
         g_pendingRemovedSendMessageCalls != 7 ||
         g_initMessageIdCalls != 3 ||
         g_initMessageIds[0] != 0x3017 ||
         g_initMessageIds[1] != 0x3018 ||
         g_initMessageIds[2] != 0x3019 ||
         strcmp(g_pendingRemovedSendMessageText[1], "Alphamsg-3017") != 0 ||
         g_pendingRemovedSendMessageWParam[2] != 7 ||
         g_pendingRemovedSendMessageLParam[2] != 0x8001 ||
         strcmp(g_pendingRemovedSendMessageText[3], "Betamsg-3018") != 0 ||
         g_pendingRemovedSendMessageWParam[4] != 8 ||
         g_pendingRemovedSendMessageLParam[4] != 0x8000 ||
         strcmp(g_pendingRemovedSendMessageText[5], "Gammamsg-3019") != 0 ||
         g_pendingRemovedSendMessageWParam[6] != 9 ||
         g_pendingRemovedSendMessageLParam[6] != 1 ||
         g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1 ||
         g_browseRecordAddedEnableCalls != 2 ||
         g_browseRecordAddedEnableThis[0] != &dialog ||
         g_browseRecordAddedEnableValue[0] != 1 ||
         g_browseRecordAddedEnableValue[1] != 0 ||
         g_browseResolvedConnectCalls != 1 ||
         g_browseResolvedConnectThis[0] != &dialog ||
         g_browseResolvedConnectValue[0] != 1 ||
         g_sessionFinishedAppendConnectCalls != 0))
    {
        failure = 4;
    }

    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFromQuery;
    g_WestwoodOnlineUpgradePendingSessionResultCount = oldPendingSessionCount;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreImportPatch(import);
    for (int index = 3; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_constructor_smoke(void)
{
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    memset(&dialog, 0x5a, sizeof(dialog));

    WestwoodOnlineUpgradeDialog *const result = dialog.Constructor(0);
    if (result != &dialog || TestObjectVtable(&dialog) == 0) {
        return 1;
    }

    if (!TestMfcWindowConstructed(dialog.m_serverAddressEdit) ||
        !TestMfcWindowConstructed(dialog.m_statusTokenEdit) ||
        !TestMfcWindowConstructed(dialog.m_queryValueOrTimeEdit) ||
        !TestMfcWindowConstructed(dialog.m_queryMaxPlayersEdit) ||
        !TestMfcWindowConstructed(dialog.m_queryAuxParamEdit) ||
        !TestMfcWindowConstructed(dialog.m_statusServerEdit) ||
        !TestMfcWindowConstructed(dialog.m_sessionNameEdit)) {
        return 2;
    }

    if (!TestMfcWindowConstructed(dialog.m_queryStatusFlag1Check) ||
        !TestMfcWindowConstructed(dialog.m_queryStatusFlag0Check) ||
        !TestMfcWindowConstructed(dialog.m_submitPendingSessionListButton) ||
        !TestMfcWindowConstructed(dialog.m_connectButton) ||
        !TestMfcWindowConstructed(dialog.m_querySessionsByNameButton) ||
        !TestMfcWindowConstructed(dialog.m_queueVisibleSessionRequestsButton)) {
        return 3;
    }

    if (!TestMfcWindowConstructed(dialog.m_statusList) ||
        !TestMfcWindowConstructed(dialog.m_sessionResultsList) ||
        !TestMfcWindowConstructed(dialog.m_browseRecordList) ||
        !TestMfcWindowConstructed(dialog.m_sessionModeCombo)) {
        return 4;
    }

    if (dialog.m_queryAuxParam != 0 || dialog.m_queryMaxPlayers != 0 ||
        dialog.m_queryValueOrTime != 0 || dialog.m_queryStatusFlagBit0 != 0 ||
        dialog.m_queryStatusFlagBit1 != 0 ||
        dialog.m_selectedProfileConnectStringMode != 0) {
        return 5;
    }

    return dialog.m_querySessionModeKind == 0x5a5a5a5a &&
                   dialog.m_statusLineCount == 0x5a5a5a5a
               ? 0
               : 6;
}

extern "C" int westwood_online_upgrade_progress_dialog_constructor_smoke(void)
{
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] = {0};
    WestwoodOnlineUpgradeProgressDialog &dialog = *(WestwoodOnlineUpgradeProgressDialog *)dialogStorage;
    memset(&dialog, 0x5a, sizeof(dialog));

    WestwoodOnlineUpgradeProgressDialog *const result = dialog.Constructor(0);
    return result == &dialog &&
                   TestObjectVtable(&dialog) != 0
               ? 0
               : 1;
}

extern "C" int westwood_online_upgrade_progress_dialog_destructor_smoke(void)
{
    const WORD kMfc42CDialogDtorOrdinal = 641;
    ImportFunctionPatch import = {};
    if (!PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CDialogDtorOrdinal,
                              (void *)&FakeModalDialogDtor,
                              import))
    {
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] = {0};

    WestwoodOnlineUpgradeProgressDialog &dialog = *(WestwoodOnlineUpgradeProgressDialog *)dialogStorage;
    ResetModalProbe();
    dialog.Destructor();
    const int result =
        g_modalDialogDtorCalls == 1 &&
                g_modalDtorSequenceCount == 1 &&
                strcmp(g_modalDtorSequence, "D") == 0
            ? 0
            : 2;

    RestoreImportPatch(import);
    return result;
}

extern "C" int westwood_online_upgrade_dialog_show_modal_smoke(void)
{
    ImportFunctionPatch imports[7] = {};
    CodeFunctionPatch menuPatch = {};
    if (!InstallModalPatches(imports, menuPatch)) {
        RestoreModalPatches(imports, menuPatch);
        return 10;
    }

    const RecoilApp oldApp = g_RecoilApp;
    HINSTANCE const oldModuleInstance = g_hWestwoodOnlineUpgradeModuleInstance;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    WestwoodOnlineUpgradeProgressDialog *const oldProgress =
        g_pWestwoodOnlineUpgradeProgressDialog;
    const int oldSelected = g_WestwoodOnlineUpgradeSelectedMissionIndex;

    g_RecoilApp.m_pMainWnd = 0x12345678;
    g_RecoilApp.m_hInstance_6c = 0x2468ace0;

    ResetModalProbe();
    g_modalSelectedMissionIndex = 8;
    int selectedMissionIndex = -55;
    int result =
        WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(&selectedMissionIndex);
    int failure = 0;
    if (result != 1 || selectedMissionIndex != 8) {
        failure = 1;
    } else if (!g_modalArgsOk) {
        failure = 2;
    } else if (g_hWestwoodOnlineUpgradeModuleInstance != (HINSTANCE)0x2468ace0) {
        failure = 3;
    } else if (g_modalMenuStep != 2 || g_modalMenuThis[0] != (void *)0x12345678 ||
               g_modalMenuThis[1] != (void *)0x12345678 || g_modalMenuVisible[0] != 0 ||
               g_modalMenuVisible[1] != 1) {
        failure = 4;
    } else if (g_modalDoModalCalls != 1) {
        failure = 50 + g_modalDoModalCalls;
    } else if (g_modalDialogDtorCalls != 2) {
        failure = 60 + g_modalDialogDtorCalls;
    } else if (g_modalCStringDtorCalls != 3) {
        failure = 70 + g_modalCStringDtorCalls;
    } else if (g_modalListDtorCalls != 3) {
        failure = 80 + g_modalListDtorCalls;
    } else if (g_modalEditDtorCalls != 7) {
        failure = 90 + g_modalEditDtorCalls;
    } else if (g_modalComboDtorCalls != 1) {
        failure = 100 + g_modalComboDtorCalls;
    } else if (g_modalButtonDtorCalls != 6) {
        failure = 110 + g_modalButtonDtorCalls;
    }

    ResetModalProbe();
    g_modalSelectedMissionIndex = -1;
    selectedMissionIndex = -77;
    result =
        WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(&selectedMissionIndex);
    if (failure == 0) {
        if (result != 0 || selectedMissionIndex != -77) {
            failure = 6;
        } else if (!g_modalArgsOk || g_modalMenuStep != 2 || g_modalMenuVisible[0] != 0 ||
                   g_modalMenuVisible[1] != 1 || g_modalDoModalCalls != 1) {
            failure = 7;
        } else if (g_modalDialogDtorCalls != 2 || g_modalCStringDtorCalls != 3 ||
                   g_modalListDtorCalls != 3 || g_modalEditDtorCalls != 7 ||
                   g_modalComboDtorCalls != 1 || g_modalButtonDtorCalls != 6) {
            failure = 8;
        }
    }

    g_RecoilApp = oldApp;
    g_hWestwoodOnlineUpgradeModuleInstance = oldModuleInstance;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeProgressDialog = oldProgress;
    g_WestwoodOnlineUpgradeSelectedMissionIndex = oldSelected;

    RestoreModalPatches(imports, menuPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_destructor_smoke(void)
{
    ImportFunctionPatch imports[7] = {};
    CodeFunctionPatch menuPatch = {};
    if (!InstallModalPatches(imports, menuPatch))
    {
        RestoreModalPatches(imports, menuPatch);
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    ResetModalProbe();
    dialog.Destructor();
    int failure = 0;
    if (g_modalDtorSequenceCount != 21 ||
        strcmp(g_modalDtorSequence, "SSSLEELCLBBBBBBEEEEED") != 0)
    {
        failure = 2;
    }
    else if (g_modalCStringDtorCalls != 3 ||
             g_modalListDtorCalls != 3 ||
             g_modalEditDtorCalls != 7 ||
             g_modalComboDtorCalls != 1 ||
             g_modalButtonDtorCalls != 6 ||
             g_modalDialogDtorCalls != 1)
    {
        failure = 3;
    }

    ResetModalProbe();
    WestwoodOnlineUpgradeDialog *const scalarResult =
        dialog.ScalarDeletingDestructor(0);
    if (failure == 0 &&
        (scalarResult != &dialog ||
         g_modalDtorSequenceCount != 21 ||
         strcmp(g_modalDtorSequence, "SSSLEELCLBBBBBBEEEEED") != 0))
    {
        failure = 4;
    }

    ResetModalProbe();
    WestwoodOnlineUpgradeDialog *const heapDialog =
        (WestwoodOnlineUpgradeDialog *)::operator new(sizeof(WestwoodOnlineUpgradeDialog));
    WestwoodOnlineUpgradeDialog *const deletedResult =
        heapDialog->ScalarDeletingDestructor(1);
    if (failure == 0 &&
        (deletedResult != heapDialog ||
         g_modalDtorSequenceCount != 21 ||
         strcmp(g_modalDtorSequence, "SSSLEELCLBBBBBBEEEEED") != 0))
    {
        failure = 5;
    }

    RestoreModalPatches(imports, menuPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_do_data_exchange_smoke(void)
{
    ImportFunctionPatch imports[3] = {};
    if (!InstallModalDdxPatches(imports))
    {
        RestoreModalDdxPatches(imports);
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    unsigned char dataExchangeStorage[16] = {};
    CDataExchange *const dataExchange = (CDataExchange *)dataExchangeStorage;

    ResetModalDdxProbe();
    dialog.WestwoodOnlineUpgradeDialog::DoDataExchange(dataExchange);

    const int expectedKind[22] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 3, 3};
    const int expectedControlId[22] = {
        1176, 1139, 1168, 1170, 1169, 1123, 1122, 1151, 1150, 1160, 1051,
        1141, 1171, 1137, 1138, 1148, 1136, 1169, 1170, 1168, 1122, 1123};
    void *expectedValue[22] = {
        &dialog.m_serverAddressEdit,
        &dialog.m_statusTokenEdit,
        &dialog.m_queryValueOrTimeEdit,
        &dialog.m_queryMaxPlayersEdit,
        &dialog.m_queryAuxParamEdit,
        &dialog.m_queryStatusFlag1Check,
        &dialog.m_queryStatusFlag0Check,
        &dialog.m_submitPendingSessionListButton,
        &dialog.m_connectButton,
        &dialog.m_querySessionsByNameButton,
        &dialog.m_queueVisibleSessionRequestsButton,
        &dialog.m_statusList,
        &dialog.m_sessionModeCombo,
        &dialog.m_sessionResultsList,
        &dialog.m_statusServerEdit,
        &dialog.m_sessionNameEdit,
        &dialog.m_browseRecordList,
        &dialog.m_queryAuxParam,
        &dialog.m_queryMaxPlayers,
        &dialog.m_queryValueOrTime,
        &dialog.m_queryStatusFlagBit0,
        &dialog.m_queryStatusFlagBit1};

    int failure = 0;
    if (g_modalDdxStep != 22)
    {
        failure = 2;
    }
    else
    {
        for (int index = 0; index < 22; ++index)
        {
            if (g_modalDdxContext[index] != dataExchange ||
                g_modalDdxKind[index] != expectedKind[index] ||
                g_modalDdxControlId[index] != expectedControlId[index] ||
                g_modalDdxValue[index] != expectedValue[index])
            {
                failure = 3;
                break;
            }
        }
    }

    RestoreModalDdxPatches(imports);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_refresh_list_timer_smoke(void)
{
    CodeFunctionPatch timePatch = {};
    CodeFunctionPatch defaultPatch = {};
    if (!PatchFunctionJump((void *)&Time::Tick, (void *)&FakeModalTimeTick,
                           timePatch) ||
        !PatchFunctionJump(CWndDefaultAddress(), (void *)&FakeWestwoodDefault,
                           defaultPatch))
    {
        RestoreFunctionPatch(defaultPatch);
        RestoreFunctionPatch(timePatch);
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldProcessCallbacksFlag = g_WestwoodOnlineUpgradeProcessCallbacksFlag;
    const float oldNextAutoRefreshTime = g_WestwoodOnlineUpgradeNextAutoRefreshTime;
    const float oldUnscaledTime = g_Time_UnscaledAccumulatedTimeSec;
    const int oldDefaultCount = g_threeFloatDefaultCount;
    const long oldDefaultReturn = g_threeFloatDefaultReturn;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.ProcessCallbacks = FakeInitProcessCallbacks;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    int failure = 0;
    g_threeFloatDefaultReturn = 77;

    g_modalTimeTickCalls = 0;
    g_threeFloatDefaultCount = 0;
    g_initProcessCallbacksCalls = 0;
    g_initRequestListModeCalls = 0;
    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 0;
    g_WestwoodOnlineUpgradeNextAutoRefreshTime = 10.0f;
    g_Time_UnscaledAccumulatedTimeSec = 100.0f;
    dialog.OnRefreshListTimer(2);
    if (g_modalTimeTickCalls != 1)
    {
        failure = 30;
    }
    else if (g_threeFloatDefaultCount != 1)
    {
        failure = 31;
    }
    else if (g_initProcessCallbacksCalls != 0)
    {
        failure = 33;
    }
    else if (g_initRequestListModeCalls != 0)
    {
        failure = 34;
    }
    else if (g_WestwoodOnlineUpgradeNextAutoRefreshTime != 10.0f)
    {
        failure = 35;
    }

    g_modalTimeTickCalls = 0;
    g_threeFloatDefaultCount = 0;
    g_initProcessCallbacksCalls = 0;
    g_initRequestListModeCalls = 0;
    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 1;
    g_WestwoodOnlineUpgradeNextAutoRefreshTime = 25.0f;
    g_Time_UnscaledAccumulatedTimeSec = 20.0f;
    dialog.OnRefreshListTimer(2);
    if (failure == 0 &&
        (g_modalTimeTickCalls != 1 ||
         g_threeFloatDefaultCount != 1 ||
         g_initProcessCallbacksCalls != 1 ||
         g_initRequestListModeCalls != 0 ||
         g_WestwoodOnlineUpgradeNextAutoRefreshTime != 25.0f))
    {
        failure = 4;
    }

    g_modalTimeTickCalls = 0;
    g_threeFloatDefaultCount = 0;
    g_initProcessCallbacksCalls = 0;
    g_initRequestListModeCalls = 0;
    g_initRequestListMode = 0;
    g_initRequestListModeEnabled = 0;
    g_WestwoodOnlineUpgradeProcessCallbacksFlag = 1;
    g_WestwoodOnlineUpgradeNextAutoRefreshTime = 39.0f;
    g_Time_UnscaledAccumulatedTimeSec = 40.0f;
    dialog.OnRefreshListTimer(2);
    if (failure == 0 &&
        (g_modalTimeTickCalls != 1 ||
         g_threeFloatDefaultCount != 1 ||
         g_initProcessCallbacksCalls != 1 ||
         g_initRequestListModeCalls != 1 ||
         g_initRequestListMode != 17 ||
         g_initRequestListModeEnabled != 1 ||
         g_WestwoodOnlineUpgradeNextAutoRefreshTime != 100.0f))
    {
        failure = 5;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_WestwoodOnlineUpgradeProcessCallbacksFlag = oldProcessCallbacksFlag;
    g_WestwoodOnlineUpgradeNextAutoRefreshTime = oldNextAutoRefreshTime;
    g_Time_UnscaledAccumulatedTimeSec = oldUnscaledTime;
    g_threeFloatDefaultCount = oldDefaultCount;
    g_threeFloatDefaultReturn = oldDefaultReturn;

    RestoreFunctionPatch(defaultPatch);
    RestoreFunctionPatch(timePatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_begin_disconnect_smoke(void)
{
    const WORD kMfc42CDialogCreateOrdinal = 2086;
    const WORD kMfc42CWndSetDlgItemTextAOrdinal = 5953;
    ImportFunctionPatch imports[2] = {};
    CodeFunctionPatch zlocPatch = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogCreateOrdinal,
                              (void *)&FakeInitCreateProgress, imports[0]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetDlgItemTextAOrdinal,
                              (void *)&FakeInitSetDlgItemTextA, imports[1]) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString, zlocPatch))
    {
        RestoreFunctionPatch(zlocPatch);
        RestoreImportPatch(imports[1]);
        RestoreImportPatch(imports[0]);
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeProgressDialog *const oldProgressDialog =
        g_pWestwoodOnlineUpgradeProgressDialog;
    const int oldDisconnectInFlight =
        g_WestwoodOnlineUpgradeDisconnectInFlightFlag;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    unsigned char progressDialogStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] = {0};
    WestwoodOnlineUpgradeProgressDialog &progressDialog = *(WestwoodOnlineUpgradeProgressDialog *)progressDialogStorage;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.Disconnect = FakeInitDisconnect;
    g_pWestwoodOnlineUpgradeProgressDialog = &progressDialog;

    int failure = 0;

    g_pWestwoodOnlineUpgradeApi = 0;
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 0;
    g_initCreateProgressCalls = 0;
    g_initSetDlgItemTextCalls = 0;
    g_initMessageIdCalls = 0;
    g_initDisconnectCalls = 0;
    dialog.BeginDisconnectAndShowProgress();
    if (g_initCreateProgressCalls != 0 ||
        g_initSetDlgItemTextCalls != 0 ||
        g_initMessageIdCalls != 0 ||
        g_initDisconnectCalls != 0 ||
        g_WestwoodOnlineUpgradeDisconnectInFlightFlag != 0)
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 1;
    g_initCreateProgressCalls = 0;
    g_initSetDlgItemTextCalls = 0;
    g_initMessageIdCalls = 0;
    g_initDisconnectCalls = 0;
    dialog.BeginDisconnectAndShowProgress();
    if (failure == 0 &&
        (g_initCreateProgressCalls != 0 ||
         g_initSetDlgItemTextCalls != 0 ||
         g_initMessageIdCalls != 0 ||
         g_initDisconnectCalls != 0 ||
         g_WestwoodOnlineUpgradeDisconnectInFlightFlag != 1))
    {
        failure = 3;
    }

    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 0;
    g_initCreateProgressCalls = 0;
    g_initCreateProgressResource = 0;
    g_initCreateProgressParent = (CWnd *)0x11111111;
    g_initSetDlgItemTextCalls = 0;
    g_initMessageIdCalls = 0;
    g_initDisconnectCalls = 0;
    dialog.BeginDisconnectAndShowProgress();
    if (failure == 0 &&
        (g_initCreateProgressCalls != 1 ||
         g_initCreateProgressResource != (LPCSTR)157 ||
         g_initCreateProgressParent != 0 ||
         g_initMessageIdCalls != 1 ||
         g_initMessageIds[0] != 0x301a ||
         g_initSetDlgItemTextCalls != 1 ||
         g_initSetDlgItemTextThis[0] != (void *)&progressDialog ||
         g_initSetDlgItemTextControlId[0] != 1179 ||
         strcmp(g_initSetDlgItemTextValue[0], "msg-301a") != 0 ||
         g_initDisconnectCalls != 1 ||
         g_WestwoodOnlineUpgradeDisconnectInFlightFlag != 1))
    {
        failure = 4;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeProgressDialog = oldProgressDialog;
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = oldDisconnectInFlight;

    RestoreFunctionPatch(zlocPatch);
    RestoreImportPatch(imports[1]);
    RestoreImportPatch(imports[0]);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_begin_connect_smoke(void)
{
    const WORD kMfc42CWndGetWindowTextAOrdinal = 3873;
    ImportFunctionPatch getWindowTextPatch = {};
    CodeFunctionPatch patches[2] = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndGetWindowTextAOrdinal,
                              (void *)&FakeBeginConnectGetWindowTextA,
                              getWindowTextPatch) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString, patches[0]) ||
        !PatchFunctionJump(MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
                           (void *)&FakeAppendConnectAppendStatusTextFmt,
                           patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        RestoreImportPatch(getWindowTextPatch);
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.PrepareConnectContextAndMode =
        FakeBeginConnectPrepareContext;
    g_initFakeApiVtable.BeginConnectWithPreparedContext =
        FakeBeginConnectWithPreparedContext;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    int failure = 0;

    g_beginConnectPrepareResult = 0;
    g_beginConnectGetWindowTextCalls = 0;
    g_beginConnectGetWindowTextThis = 0;
    g_beginConnectGetWindowTextBuffer = 0;
    g_beginConnectGetWindowTextMaxCount = 0;
    g_beginConnectPrepareCalls = 0;
    g_beginConnectPrepareContext = 0;
    g_beginConnectPreparedCalls = 0;
    g_beginConnectPreparedContext = 0;
    g_beginConnectPreparedMode = -1;
    g_initMessageIdCalls = 0;
    g_appendConnectAppendCalls = 0;
    g_appendConnectAppendThis = 0;
    g_appendConnectAppendText = 0;
    dialog.BeginConnect();
    if (g_beginConnectGetWindowTextCalls != 1 ||
        g_beginConnectGetWindowTextThis != &dialog.m_statusServerEdit ||
        g_beginConnectGetWindowTextMaxCount != 9 ||
        g_beginConnectPrepareCalls != 1 ||
        g_beginConnectPreparedCalls != 1 ||
        g_beginConnectPrepareContext != g_beginConnectPreparedContext ||
        strcmp(g_beginConnectPrepareContext->m_requestText, "SrvMode") != 0 ||
        g_beginConnectPreparedMode != 0 ||
        g_initMessageIdCalls != 1 ||
        g_initMessageIds[0] != 0x303b ||
        g_appendConnectAppendCalls != 1 ||
        g_appendConnectAppendThis != &dialog ||
        strcmp(g_appendConnectAppendText, "msg-303b") != 0)
    {
        failure = 2;
    }

    g_beginConnectPrepareResult = 1;
    g_beginConnectGetWindowTextCalls = 0;
    g_beginConnectGetWindowTextThis = 0;
    g_beginConnectGetWindowTextBuffer = 0;
    g_beginConnectGetWindowTextMaxCount = 0;
    g_beginConnectPrepareCalls = 0;
    g_beginConnectPrepareContext = 0;
    g_beginConnectPreparedCalls = 0;
    g_beginConnectPreparedContext = 0;
    g_beginConnectPreparedMode = -1;
    g_initMessageIdCalls = 0;
    g_appendConnectAppendCalls = 0;
    g_appendConnectAppendThis = 0;
    g_appendConnectAppendText = 0;
    dialog.BeginConnect();
    if (failure == 0 &&
        (g_beginConnectGetWindowTextCalls != 1 ||
         g_beginConnectGetWindowTextThis != &dialog.m_statusServerEdit ||
         g_beginConnectGetWindowTextMaxCount != 9 ||
         g_beginConnectPrepareCalls != 1 ||
         g_beginConnectPreparedCalls != 1 ||
         g_beginConnectPrepareContext != g_beginConnectPreparedContext ||
         strcmp(g_beginConnectPrepareContext->m_requestText, "SrvMode") != 0 ||
         g_beginConnectPreparedMode != 1 ||
         g_initMessageIdCalls != 1 ||
         g_initMessageIds[0] != 0x303c ||
         g_appendConnectAppendCalls != 1 ||
         g_appendConnectAppendThis != &dialog ||
         strcmp(g_appendConnectAppendText, "msg-303c") != 0))
    {
        failure = 3;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    RestoreImportPatch(getWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_check_and_apply_upgrade_smoke(void)
{
    const WORD kMfc42CWndGetWindowTextAOrdinal = 3873;
    ImportFunctionPatch getWindowTextPatch = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndGetWindowTextAOrdinal,
                              (void *)&FakeBeginConnectGetWindowTextA,
                              getWindowTextPatch))
    {
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.RequestUpgradeDownloadReadyResult =
        FakeCheckAndApplyUpgradeResult;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    g_beginConnectGetWindowTextCalls = 0;
    g_beginConnectGetWindowTextThis = 0;
    g_beginConnectGetWindowTextBuffer = 0;
    g_beginConnectGetWindowTextMaxCount = 0;
    g_checkAndApplyUpgradeCalls = 0;
    g_checkAndApplyUpgradeContext = 0;
    g_checkAndApplyUpgradeResult = 17;

    int failure = 0;
    int const result = dialog.CheckAndApplyUpgrade();
    if (result != 17 ||
        g_beginConnectGetWindowTextCalls != 1 ||
        g_beginConnectGetWindowTextThis != &dialog.m_statusServerEdit ||
        g_beginConnectGetWindowTextMaxCount != 9 ||
        g_checkAndApplyUpgradeCalls != 1 ||
        g_checkAndApplyUpgradeContext !=
            (WestwoodOnlineUpgradeConnectContext *)
                (g_beginConnectGetWindowTextBuffer - 0x24) ||
        strcmp(g_checkAndApplyUpgradeContext->m_requestText, "SrvMode") != 0)
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    RestoreImportPatch(getWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_query_status_smoke(void)
{
    const WORD kMfc42CWndGetWindowTextAOrdinal = 3873;
    const WORD kMfc42CWndMessageBoxAOrdinal = 4224;
    ImportFunctionPatch getWindowTextPatch = {};
    ImportFunctionPatch messageBoxPatch = {};
    CodeFunctionPatch setWindowTextPatch = {};
    CodeFunctionPatch zlocPatch = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndGetWindowTextAOrdinal,
                              (void *)&FakeQueryStatusGetWindowTextA,
                              getWindowTextPatch) ||
        !PatchFunctionJump(CWndSetWindowTextAAddress(),
                           (void *)&FakeQueryStatusSetWindowTextA,
                           setWindowTextPatch) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndMessageBoxAOrdinal,
                              (void *)&FakeQueryStatusMessageBoxA,
                              messageBoxPatch) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           zlocPatch))
    {
        RestoreFunctionPatch(zlocPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreImportPatch(messageBoxPatch);
        RestoreImportPatch(getWindowTextPatch);
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.QueryStatusWithTokenAndServer =
        FakeQueryStatusWithTokenAndServer;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    strcpy(g_queryStatusTokenInput, "token42 tail");
    strcpy(g_queryStatusServerInput, "server.example");
    g_queryStatusGetWindowTextCalls = 0;
    g_queryStatusSetWindowTextCalls = 0;
    g_queryStatusProviderCalls = 0;
    g_queryStatusProviderContext = 0;
    g_queryStatusProviderServerText = 0;
    g_queryStatusProviderResult = 23;
    g_queryStatusMessageBoxCalls = 0;
    g_initMessageIdCalls = 0;

    int failure = 0;
    int result = dialog.QueryStatus();
    if (result != 23 ||
        g_queryStatusGetWindowTextCalls != 2 ||
        g_queryStatusGetWindowTextThis[0] != &dialog.m_statusTokenEdit ||
        g_queryStatusGetWindowTextMaxCount[0] != 19 ||
        g_queryStatusGetWindowTextThis[1] != &dialog.m_statusServerEdit ||
        g_queryStatusGetWindowTextMaxCount[1] != 79 ||
        g_queryStatusSetWindowTextCalls != 2 ||
        strcmp(g_queryStatusSetWindowTextValue[0], "") != 0 ||
        strcmp(g_queryStatusSetWindowTextValue[1], "") != 0 ||
        g_queryStatusProviderCalls != 1 ||
        strcmp(g_queryStatusProviderContext->m_requestText, "token42") != 0 ||
        strcmp(g_queryStatusProviderServerText, "server.example") != 0 ||
        g_queryStatusMessageBoxCalls != 0 ||
        g_initMessageIdCalls != 0)
    {
        failure = 2;
    }

    strcpy(g_queryStatusTokenInput, "   ");
    strcpy(g_queryStatusServerInput, "unused");
    g_queryStatusGetWindowTextCalls = 0;
    g_queryStatusSetWindowTextCalls = 0;
    g_queryStatusProviderCalls = 0;
    g_queryStatusProviderContext = 0;
    g_queryStatusProviderServerText = 0;
    g_queryStatusMessageBoxCalls = 0;
    g_queryStatusMessageBoxThis = 0;
    g_queryStatusMessageBoxText[0] = '\0';
    g_queryStatusMessageBoxCaption[0] = '\0';
    g_queryStatusMessageBoxType = 0;
    g_queryStatusMessageBoxResult = 31;
    g_initMessageIdCalls = 0;

    result = dialog.QueryStatus();
    if (failure == 0 &&
        (result != 31 ||
         g_queryStatusGetWindowTextCalls != 1 ||
         g_queryStatusGetWindowTextThis[0] != &dialog.m_statusTokenEdit ||
         g_queryStatusGetWindowTextMaxCount[0] != 19 ||
         g_queryStatusSetWindowTextCalls != 1 ||
         strcmp(g_queryStatusSetWindowTextValue[0], "") != 0 ||
         g_queryStatusProviderCalls != 0 ||
         g_queryStatusMessageBoxCalls != 1 ||
         g_queryStatusMessageBoxThis != &dialog ||
         strcmp(g_queryStatusMessageBoxText, "msg-303d") != 0 ||
         strcmp(g_queryStatusMessageBoxCaption, "msg-3003") != 0 ||
         g_queryStatusMessageBoxType != MB_ICONHAND ||
         g_initMessageIdCalls != 2 ||
         g_initMessageIds[0] != 0x3003 ||
         g_initMessageIds[1] != 0x303d))
    {
        failure = 3;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    RestoreFunctionPatch(zlocPatch);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreImportPatch(messageBoxPatch);
    RestoreImportPatch(getWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_request_active_list_mode_smoke(void)
{
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;
    g_WestwoodOnlineUpgradeActiveListMode = 29;
    g_initRequestListModeCalls = 0;
    g_initRequestListMode = 0;
    g_initRequestListModeEnabled = 0;

    dialog.RequestActiveListMode();
    int const failure =
        g_initRequestListModeCalls == 1 && g_initRequestListMode == 29 &&
                g_initRequestListModeEnabled == 1
            ? 0
            : 1;

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_update_session_list_query_smoke(void)
{
    CodeFunctionPatch updateDataPatch = {};
    if (!PatchFunctionJump(CWndUpdateDataAddress(), (void *)&FakeWestwoodUpdateData,
                           updateDataPatch))
    {
        return 1;
    }

    HWND comboBox = CreateWindowExA(0,
                                    "COMBOBOX",
                                    "",
                                    WS_POPUP | CBS_DROPDOWNLIST,
                                    0,
                                    0,
                                    200,
                                    200,
                                    0,
                                    0,
                                    GetModuleHandleA(0),
                                    0);
    if (comboBox == 0)
    {
        RestoreFunctionPatch(updateDataPatch);
        return 1;
    }

    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"zero");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"one");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"two");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"three");
    SendMessageA(comboBox, CB_SETCURSEL, 3, 0);

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.SubmitEncodedQueryString = FakeSubmitEncodedQueryString;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionModeCombo.m_hWnd = comboBox;
    dialog.m_queryValueOrTime = 5;
    dialog.m_queryAuxParam = 6;
    dialog.m_queryMaxPlayers = 7;
    dialog.m_queryStatusFlagBit0 = 1;
    dialog.m_queryStatusFlagBit1 = 0;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatUpdateDataSaveValue[0] = -1;
    g_submitEncodedQueryCalls = 0;
    g_submitEncodedQuerySelf = 0;
    g_submitEncodedQueryText[0] = '\0';

    dialog.UpdateSessionListQueryFromControls();

    char expected[64];
    wsprintfA(expected, "%1d%4d%4d%1d%1d%1d", 3, 5, 6, 7, 1, 0);
    int failure = 0;
    if (g_threeFloatUpdateDataCount != 1 ||
        g_threeFloatUpdateDataSaveValue[0] != 1 ||
        g_submitEncodedQueryCalls != 1 ||
        g_submitEncodedQuerySelf != (IUnknown *)&g_initFakeApi ||
        strcmp(g_submitEncodedQueryText, expected) != 0)
    {
        failure = 2;
    }

    g_threeFloatUpdateDataCount = 0;
    g_threeFloatUpdateDataSaveValue[0] = -1;
    g_submitEncodedQueryCalls = 0;
    g_submitEncodedQuerySelf = 0;
    g_submitEncodedQueryText[0] = '\0';
    dialog.OnQueryControlsChanged();
    if (failure == 0 &&
        (g_threeFloatUpdateDataCount != 1 ||
         g_threeFloatUpdateDataSaveValue[0] != 1 ||
         g_submitEncodedQueryCalls != 1 ||
         g_submitEncodedQuerySelf != (IUnknown *)&g_initFakeApi ||
         strcmp(g_submitEncodedQueryText, expected) != 0))
    {
        failure = 3;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    DestroyWindow(comboBox);
    RestoreFunctionPatch(updateDataPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_refresh_current_query_smoke(void)
{
    const WORD kMfc42CWndGetWindowTextACStringOrdinal = 3874;
    const WORD kMfc42CWndMessageBoxAOrdinal = 4224;
    ImportFunctionPatch getWindowTextPatch = {};
    ImportFunctionPatch messageBoxPatch = {};
    CodeFunctionPatch setWindowTextPatch = {};
    CodeFunctionPatch patches[3] = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndGetWindowTextACStringOrdinal,
                              (void *)&FakeRefreshCurrentQueryGetWindowTextA,
                              getWindowTextPatch) ||
        !PatchFunctionJump(CWndSetWindowTextAAddress(),
                           (void *)&FakeQueryStatusSetWindowTextA,
                           setWindowTextPatch) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndMessageBoxAOrdinal,
                              (void *)&FakeQueryStatusMessageBoxA,
                              messageBoxPatch) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(
                &WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList),
            (void *)&FakeAppendConnectResetSelectedBrowseRecord,
            patches[1]) ||
        !PatchFunctionJump(MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
                           (void *)&FakeAppendConnectAppendStatusTextFmt,
                           patches[2]))
    {
        RestoreFunctionPatch(patches[2]);
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreImportPatch(messageBoxPatch);
        RestoreImportPatch(getWindowTextPatch);
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    const int oldCreateSessionFromQueryFlag =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_queryMaxPlayers = 14;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.ResetQueryState = FakeResetQueryState;
    g_initFakeApiVtable.SubmitQueryRequest = FakeRefreshCurrentQuerySubmit;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    strcpy(g_refreshCurrentQuerySessionNameInput, "  Alpha  ");
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradeActiveListMode = 7;
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = 'S';
    g_refreshCurrentQueryGetWindowTextCalls = 0;
    g_refreshCurrentQueryGetWindowTextThis = 0;
    g_refreshCurrentQuerySubmitCalls = 0;
    memset(&g_refreshCurrentQuerySubmitRequest,
           0x5a,
           sizeof(g_refreshCurrentQuerySubmitRequest));
    g_refreshCurrentQuerySubmitResult = 0;
    g_appendConnectResetCalls = 0;
    g_appendConnectResetThis = 0;
    g_resetQueryStateCalls = 0;
    g_queryStatusSetWindowTextCalls = 0;
    g_queryStatusMessageBoxCalls = 0;
    g_appendConnectAppendCalls = 0;
    g_initMessageIdCalls = 0;

    int failure = 0;
    dialog.OnRefreshCurrentQuery();
    if (g_appendConnectResetCalls != 1 ||
        g_appendConnectResetThis != &dialog ||
        g_refreshCurrentQueryGetWindowTextCalls != 1 ||
        g_refreshCurrentQueryGetWindowTextThis != &dialog.m_sessionNameEdit ||
        g_queryStatusSetWindowTextCalls != 1 ||
        strcmp(g_queryStatusSetWindowTextValue[0], "") != 0 ||
        g_resetQueryStateCalls != 1 ||
        g_refreshCurrentQuerySubmitCalls != 1 ||
        g_refreshCurrentQuerySubmitRequest.m_listMode != 0 ||
        g_refreshCurrentQuerySubmitRequest.m_queryVariant != 2 ||
        g_refreshCurrentQuerySubmitRequest.m_queryMaxPlayers != 14 ||
        g_refreshCurrentQuerySubmitRequest.m_queryFlags != 0 ||
        strcmp(g_refreshCurrentQuerySubmitRequest.m_sessionName, "Alpha") != 0 ||
        g_WestwoodOnlineUpgradeActiveListMode != 0 ||
        g_queryStatusMessageBoxCalls != 0 ||
        g_appendConnectAppendCalls != 0 ||
        g_initMessageIdCalls != 0)
    {
        failure = 2;
    }

    strcpy(g_refreshCurrentQuerySessionNameInput, "   ");
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 7;
    g_refreshCurrentQueryGetWindowTextCalls = 0;
    g_refreshCurrentQuerySubmitCalls = 0;
    g_queryStatusSetWindowTextCalls = 0;
    g_queryStatusMessageBoxCalls = 0;
    g_queryStatusMessageBoxResult = 0;
    g_queryStatusMessageBoxText[0] = '\0';
    g_queryStatusMessageBoxCaption[0] = '\0';
    g_initMessageIdCalls = 0;
    dialog.OnRefreshCurrentQuery();
    if (failure == 0 &&
        (g_refreshCurrentQuerySubmitCalls != 0 ||
         g_queryStatusSetWindowTextCalls != 0 ||
         g_queryStatusMessageBoxCalls != 1 ||
         strcmp(g_queryStatusMessageBoxText, "msg-303f") != 0 ||
         strcmp(g_queryStatusMessageBoxCaption, "msg-3003") != 0 ||
         g_initMessageIdCalls != 2 ||
         g_initMessageIds[0] != 0x3003 ||
         g_initMessageIds[1] != 0x303f ||
         g_WestwoodOnlineUpgradeActiveListMode != 7))
    {
        failure = 3;
    }

    strcpy(g_refreshCurrentQuerySessionNameInput, "Alpha Beta");
    g_refreshCurrentQuerySubmitCalls = 0;
    g_queryStatusMessageBoxCalls = 0;
    g_queryStatusMessageBoxText[0] = '\0';
    g_queryStatusMessageBoxCaption[0] = '\0';
    g_initMessageIdCalls = 0;
    dialog.OnRefreshCurrentQuery();
    if (failure == 0 &&
        (g_refreshCurrentQuerySubmitCalls != 0 ||
         g_queryStatusMessageBoxCalls != 1 ||
         strcmp(g_queryStatusMessageBoxText, "msg-303e") != 0 ||
         strcmp(g_queryStatusMessageBoxCaption, "msg-3003") != 0 ||
         g_initMessageIds[0] != 0x3003 ||
         g_initMessageIds[1] != 0x303e))
    {
        failure = 4;
    }

    strcpy(g_refreshCurrentQuerySessionNameInput, "Gamma");
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_WestwoodOnlineUpgradeActiveListMode = 7;
    g_refreshCurrentQuerySubmitCalls = 0;
    g_refreshCurrentQuerySubmitResult = 0x800401f7;
    g_queryStatusSetWindowTextCalls = 0;
    g_queryStatusMessageBoxCalls = 0;
    g_appendConnectAppendCalls = 0;
    g_appendConnectAppendThis = 0;
    g_appendConnectAppendText = 0;
    g_initMessageIdCalls = 0;
    dialog.OnRefreshCurrentQuery();
    if (failure == 0 &&
        (g_refreshCurrentQuerySubmitCalls != 1 ||
         g_appendConnectAppendCalls != 1 ||
         g_appendConnectAppendThis != &dialog ||
         strcmp(g_appendConnectAppendText, "msg-3042") != 0 ||
         g_initMessageIds[0] != 0x3042 ||
         g_WestwoodOnlineUpgradeActiveListMode != 0))
    {
        failure = 5;
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag =
        oldCreateSessionFromQueryFlag;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    RestoreFunctionPatch(patches[2]);
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreImportPatch(messageBoxPatch);
    RestoreImportPatch(getWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_on_destroy_smoke(void)
{
    ImportFunctionPatch imports[2] = {};
    CodeFunctionPatch patches[2] = {};
    if (!PatchImportByName("KERNEL32.dll", "Sleep",
                           (void *)&FakeInitSleep, imports[0]) ||
        !PatchImportByName("USER32.dll", "KillTimer",
                           (void *)&FakeDestroyKillTimer, imports[1]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress),
            (void *)&FakeDestroyBeginDisconnect, patches[0]) ||
        !PatchFunctionJump((void *)&WestwoodOnlineUpgradeApi::Shutdown,
                           (void *)&FakeDestroyApiShutdown, patches[1]))
    {
        for (int index = 1; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
            RestoreImportPatch(imports[index]);
        }
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeProgressDialog *const oldProgressDialog =
        g_pWestwoodOnlineUpgradeProgressDialog;
    const int oldAbortFlag = g_WestwoodOnlineUpgradeAbortFlag;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_hWnd = (HWND)0x24681357;
    unsigned char progressDialogStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] = {0};
    WestwoodOnlineUpgradeProgressDialog &progressDialog = *(WestwoodOnlineUpgradeProgressDialog *)progressDialogStorage;
    *(RecoilNamedVtable **)&progressDialog =
        (RecoilNamedVtable *)&g_initProgressVtable;
    g_initProgressVtable.DestroyWindow = FakeInitDestroyProgress;
    g_pWestwoodOnlineUpgradeProgressDialog = &progressDialog;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.ProcessCallbacks = FakeDestroyProcessCallbacks;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    int failure = 0;

    g_WestwoodOnlineUpgradeAbortFlag = 1;
    g_destroyBeginDisconnectCalls = 0;
    g_destroyBeginDisconnectThis = 0;
    g_initProcessCallbacksCalls = 0;
    g_initSleepCalls = 0;
    g_destroyKillTimerCalls = 0;
    g_destroyKillTimerHwnd = 0;
    g_destroyKillTimerId = 0;
    g_destroyShutdownCalls = 0;
    g_initDestroyProgressCalls = 0;
    g_initDestroyedProgress = 0;
    dialog.OnDestroy();
    if (g_destroyBeginDisconnectCalls != 0 ||
        g_initProcessCallbacksCalls != 0 ||
        g_initSleepCalls != 0 ||
        g_destroyKillTimerCalls != 1 ||
        g_destroyKillTimerHwnd != (HWND)0x24681357 ||
        g_destroyKillTimerId != 4 ||
        g_destroyShutdownCalls != 1 ||
        g_initDestroyProgressCalls != 1 ||
        g_initDestroyedProgress != (CWnd *)&progressDialog)
    {
        failure = 2;
    }

    g_WestwoodOnlineUpgradeAbortFlag = 0;
    g_destroyBeginDisconnectCalls = 0;
    g_destroyBeginDisconnectThis = 0;
    g_initProcessCallbacksCalls = 0;
    g_initSleepCalls = 0;
    g_destroyKillTimerCalls = 0;
    g_destroyShutdownCalls = 0;
    g_initDestroyProgressCalls = 0;
    g_initDestroyedProgress = 0;
    dialog.OnDestroy();
    if (failure == 0 &&
        (g_destroyBeginDisconnectCalls != 1 ||
         g_destroyBeginDisconnectThis != &dialog ||
         g_initProcessCallbacksCalls != 2 ||
         g_initSleepCalls != 2 ||
         g_initSleepDurations[0] != 200 ||
         g_initSleepDurations[1] != 200 ||
         g_destroyKillTimerCalls != 1 ||
         g_destroyKillTimerHwnd != (HWND)0x24681357 ||
         g_destroyKillTimerId != 4 ||
         g_destroyShutdownCalls != 1 ||
         g_initDestroyProgressCalls != 1 ||
         g_initDestroyedProgress != (CWnd *)&progressDialog ||
         g_WestwoodOnlineUpgradeAbortFlag != 1))
    {
        failure = 3;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeProgressDialog = oldProgressDialog;
    g_WestwoodOnlineUpgradeAbortFlag = oldAbortFlag;

    for (int index = 1; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
        RestoreImportPatch(imports[index]);
    }
    return failure;
}

extern "C" int westwood_online_upgrade_api_init_state_smoke(void)
{
    const HANDLE bootstrapEvent = (HANDLE)0x13572468;
    const HINSTANCE moduleHandle = (HINSTANCE)0x24681357;
    WestwoodOnlineUpgradeApiInitState state = {};
    state.structSize = sizeof(state);
    state.statusTextEvent = (HANDLE)0x11111111;
    state.failureEvent = (HANDLE)0x22222222;

    if (WestwoodOnlineUpgradeApiInitState::Init(0, bootstrapEvent, moduleHandle) !=
        E_INVALIDARG)
    {
        return 1;
    }

    WestwoodOnlineUpgradeApiInitState smallState = {};
    smallState.structSize = sizeof(smallState) - 1;
    smallState.moduleHandlePrimary = (HINSTANCE)0x33333333;
    if (WestwoodOnlineUpgradeApiInitState::Init(&smallState, bootstrapEvent, moduleHandle) !=
            E_INVALIDARG ||
        smallState.moduleHandlePrimary != (HINSTANCE)0x33333333)
    {
        return 2;
    }

    if (WestwoodOnlineUpgradeApiInitState::Init(&state, bootstrapEvent, moduleHandle) != S_OK)
    {
        return 3;
    }

    int failure = 0;
    if (state.moduleHandlePrimary != moduleHandle ||
        state.moduleHandleSecondary != moduleHandle ||
        state.moduleHandleTertiary != moduleHandle ||
        state.bootstrapServerListEvent != bootstrapEvent ||
        state.statusTextEvent != 0 ||
        state.failureEvent != 0)
    {
        failure = 4;
    }
    else if (TryEnterCriticalSection(&state.criticalSection0) == 0)
    {
        failure = 5;
    }
    else
    {
        LeaveCriticalSection(&state.criticalSection0);
    }

    if (failure == 0 && TryEnterCriticalSection(&state.criticalSection1) == 0)
    {
        failure = 6;
    }
    else if (failure == 0)
    {
        LeaveCriticalSection(&state.criticalSection1);
    }

    if (failure == 0 && TryEnterCriticalSection(&state.criticalSection2) == 0)
    {
        failure = 7;
    }
    else if (failure == 0)
    {
        LeaveCriticalSection(&state.criticalSection2);
    }

    DeleteCriticalSection(&state.criticalSection2);
    DeleteCriticalSection(&state.criticalSection1);
    DeleteCriticalSection(&state.criticalSection0);
    return failure;
}

extern "C" int westwood_online_upgrade_api_shutdown_smoke(void)
{
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const DWORD oldCookie = g_WestwoodOnlineUpgradeApiConnectionCookie;
    HANDLE const oldHandle0 = g_WestwoodOnlineUpgradeCloseHandle0;
    HANDLE const oldHandle1 = g_WestwoodOnlineUpgradeCloseHandle1;
    HANDLE const oldHandle2 = g_WestwoodOnlineUpgradeCloseHandle2;

    g_shutdownUnknown.vftable = &g_shutdownUnknownVtable;
    g_shutdownCpc.vftable = &g_shutdownCpcVtable;
    g_shutdownConnectionPoint.vftable = &g_shutdownConnectionPointVtable;
    g_shutdownSourceReleaseCalls = 0;
    g_shutdownCpcReleaseCalls = 0;
    g_shutdownConnectionPointReleaseCalls = 0;
    g_shutdownFindConnectionPointCalls = 0;
    g_shutdownUnadviseCalls = 0;
    g_shutdownUnadviseCookie = 0;
    g_shutdownIidOk = false;
    g_shutdownAdviseCalls = 0;
    g_shutdownAdviseSink = 0;
    g_shutdownAdviseCookie = 0;

    g_pWestwoodOnlineUpgradeApi = 0;
    WestwoodOnlineUpgradeApi::Shutdown();
    if (g_shutdownSourceReleaseCalls != 0 || g_shutdownUnadviseCalls != 0)
    {
        return 1;
    }

    HANDLE handle0 = CreateEventA(0, TRUE, FALSE, 0);
    HANDLE handle1 = CreateEventA(0, TRUE, FALSE, 0);
    HANDLE handle2 = CreateEventA(0, TRUE, FALSE, 0);
    if (handle0 == 0 || handle1 == 0 || handle2 == 0)
    {
        if (handle0 != 0)
        {
            CloseHandle(handle0);
        }
        if (handle1 != 0)
        {
            CloseHandle(handle1);
        }
        if (handle2 != 0)
        {
            CloseHandle(handle2);
        }
        return 2;
    }

    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_shutdownUnknown;
    g_WestwoodOnlineUpgradeApiConnectionCookie = 0x12345678;
    g_WestwoodOnlineUpgradeCloseHandle0 = handle0;
    g_WestwoodOnlineUpgradeCloseHandle1 = handle1;
    g_WestwoodOnlineUpgradeCloseHandle2 = handle2;
    WestwoodOnlineUpgradeApi::Shutdown();

    int failure = 0;
    if (g_pWestwoodOnlineUpgradeApi != 0)
    {
        failure = 3;
    }
    else if (g_shutdownFindConnectionPointCalls != 1 || !g_shutdownIidOk ||
             g_shutdownUnadviseCalls != 1 ||
             g_shutdownUnadviseCookie != 0x12345678)
    {
        failure = 4;
    }
    else if (g_shutdownConnectionPointReleaseCalls != 1 ||
             g_shutdownCpcReleaseCalls != 1 ||
             g_shutdownSourceReleaseCalls != 1)
    {
        failure = 5;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_WestwoodOnlineUpgradeApiConnectionCookie = oldCookie;
    g_WestwoodOnlineUpgradeCloseHandle0 = oldHandle0;
    g_WestwoodOnlineUpgradeCloseHandle1 = oldHandle1;
    g_WestwoodOnlineUpgradeCloseHandle2 = oldHandle2;
    return failure;
}

extern "C" int westwood_online_upgrade_api_create_instance_load_config_smoke(void)
{
    ImportFunctionPatch imports[2] = {};
    CodeFunctionPatch showModalPatch = {};
    if (!PatchImportByName("ole32.dll", "CoInitialize",
                           (void *)&FakeApiCreateCoInitialize, imports[0]) ||
        !PatchImportByName("ole32.dll", "CoCreateInstance",
                           (void *)&FakeApiCreateCoCreateInstance, imports[1]) ||
        !PatchFunctionJump(
            MethodAddress(
                &WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues),
            (void *)&FakeApiCreateShowModalAndApplySelectedProfileValues,
            showModalPatch))
    {
        RestoreFunctionPatch(showModalPatch);
        RestoreImportPatch(imports[1]);
        RestoreImportPatch(imports[0]);
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    void *const oldSink = g_pWestwoodOnlineUpgradeApiEventSink;
    const DWORD oldCookie = g_WestwoodOnlineUpgradeApiConnectionCookie;
    const int oldReadyFlag = g_WestwoodOnlineUpgradeApiReadyFlag;
    const int oldShutdownState = g_WestwoodOnlineUpgradeApiShutdownState;
    const int oldSinkOffset = g_WestwoodOnlineUpgradeApiEventSinkConnectionOffset;
    WestwoodOnlineUpgradeApiInitState const oldInitState =
        g_WestwoodOnlineUpgradeApiInitState;
    HANDLE const oldHandle0 = g_WestwoodOnlineUpgradeCloseHandle0;
    HANDLE const oldHandle1 = g_WestwoodOnlineUpgradeCloseHandle1;
    HANDLE const oldHandle2 = g_WestwoodOnlineUpgradeCloseHandle2;

    g_shutdownUnknown.vftable = &g_shutdownUnknownVtable;
    g_shutdownCpc.vftable = &g_shutdownCpcVtable;
    g_shutdownConnectionPoint.vftable = &g_shutdownConnectionPointVtable;
    g_apiCreateCoInitializeCalls = 0;
    g_apiCreateCoInitializeReserved = 0;
    g_apiCreateCoCreateCalls = 0;
    g_apiCreateCoCreateArgsOk = false;
    g_apiCreateCoCreateResult = S_OK;
    g_apiCreateCoCreateObject = (IUnknown *)&g_shutdownUnknown;
    g_apiCreateShowModalCalls = 0;
    g_apiCreateShowModalResult = 1;
    g_shutdownFindConnectionPointCalls = 0;
    g_shutdownAdviseCalls = 0;
    g_shutdownAdviseSink = 0;
    g_shutdownAdviseCookie = 0;
    g_shutdownConnectionPointReleaseCalls = 0;
    g_shutdownCpcReleaseCalls = 0;
    g_shutdownSourceReleaseCalls = 0;
    g_shutdownIidOk = false;
    g_pWestwoodOnlineUpgradeApi = 0;
    g_pWestwoodOnlineUpgradeApiEventSink = 0;
    g_WestwoodOnlineUpgradeApiConnectionCookie = 0;
    g_WestwoodOnlineUpgradeApiReadyFlag = 0;
    g_WestwoodOnlineUpgradeApiShutdownState = 77;
    g_WestwoodOnlineUpgradeApiEventSinkConnectionOffset = 0;

    WestwoodOnlineUpgradeApi api;
    const HANDLE bootstrapEvent = (HANDLE)0x31415926;
    int result = api.CreateInstanceAndLoadConfig(bootstrapEvent);
    int failure = 0;
    if (result != 1 || g_apiCreateCoInitializeCalls != 1 ||
        g_apiCreateCoInitializeReserved != 0 || g_apiCreateCoCreateCalls != 1 ||
        !g_apiCreateCoCreateArgsOk)
    {
        failure = 2;
    }
    else if (g_WestwoodOnlineUpgradeApiInitState.structSize !=
                 sizeof(g_WestwoodOnlineUpgradeApiInitState) ||
             g_WestwoodOnlineUpgradeApiInitState.bootstrapServerListEvent !=
                 bootstrapEvent ||
             g_WestwoodOnlineUpgradeApiShutdownState != 0 ||
             g_WestwoodOnlineUpgradeApiReadyFlag != 1)
    {
        failure = 3;
    }
    else if (g_pWestwoodOnlineUpgradeApi != (IUnknown *)&g_shutdownUnknown ||
             g_pWestwoodOnlineUpgradeApiEventSink == 0 ||
             g_shutdownFindConnectionPointCalls != 1 ||
             g_shutdownAdviseCalls != 1 ||
             g_shutdownAdviseSink !=
                 (IUnknown *)g_pWestwoodOnlineUpgradeApiEventSink ||
             g_WestwoodOnlineUpgradeApiConnectionCookie != 0x87654321 ||
             !g_shutdownIidOk)
    {
        failure = 4;
    }
    else if (g_apiCreateShowModalCalls != 1 ||
             g_shutdownSourceReleaseCalls != 0 ||
             g_shutdownConnectionPointReleaseCalls != 1 ||
             g_shutdownCpcReleaseCalls != 1)
    {
        failure = 5;
    }

    if (g_pWestwoodOnlineUpgradeApiEventSink != 0)
    {
        WestwoodOnlineUpgradeApiEventSink *const sink =
            (WestwoodOnlineUpgradeApiEventSink *)g_pWestwoodOnlineUpgradeApiEventSink;
        DeleteCriticalSection(&sink->m_refCountAndLock.lock);
        ::operator delete(sink);
        g_pWestwoodOnlineUpgradeApiEventSink = 0;
    }
    DeleteCriticalSection(&g_WestwoodOnlineUpgradeApiInitState.criticalSection2);
    DeleteCriticalSection(&g_WestwoodOnlineUpgradeApiInitState.criticalSection1);
    DeleteCriticalSection(&g_WestwoodOnlineUpgradeApiInitState.criticalSection0);
    g_WestwoodOnlineUpgradeApiInitState = oldInitState;

    g_apiCreateCoCreateObject = (IUnknown *)&g_shutdownUnknown;
    g_apiCreateShowModalCalls = 0;
    g_apiCreateShowModalResult = 0;
    g_shutdownFindConnectionPointCalls = 0;
    g_shutdownAdviseCalls = 0;
    g_shutdownUnadviseCalls = 0;
    g_shutdownSourceReleaseCalls = 0;
    g_shutdownConnectionPointReleaseCalls = 0;
    g_shutdownCpcReleaseCalls = 0;
    g_pWestwoodOnlineUpgradeApi = 0;
    g_WestwoodOnlineUpgradeApiConnectionCookie = 0;
    result = api.CreateInstanceAndLoadConfig(bootstrapEvent);
    if (failure == 0 &&
        (result != 0 || g_apiCreateShowModalCalls != 1 ||
         g_shutdownAdviseCalls != 1 || g_shutdownUnadviseCalls != 1 ||
         g_shutdownSourceReleaseCalls != 1 || g_pWestwoodOnlineUpgradeApi != 0))
    {
        failure = 6;
    }

    if (g_pWestwoodOnlineUpgradeApiEventSink != 0)
    {
        WestwoodOnlineUpgradeApiEventSink *const sink =
            (WestwoodOnlineUpgradeApiEventSink *)g_pWestwoodOnlineUpgradeApiEventSink;
        DeleteCriticalSection(&sink->m_refCountAndLock.lock);
        ::operator delete(sink);
    }
    DeleteCriticalSection(&g_WestwoodOnlineUpgradeApiInitState.criticalSection2);
    DeleteCriticalSection(&g_WestwoodOnlineUpgradeApiInitState.criticalSection1);
    DeleteCriticalSection(&g_WestwoodOnlineUpgradeApiInitState.criticalSection0);

    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeApiEventSink = oldSink;
    g_WestwoodOnlineUpgradeApiConnectionCookie = oldCookie;
    g_WestwoodOnlineUpgradeApiReadyFlag = oldReadyFlag;
    g_WestwoodOnlineUpgradeApiShutdownState = oldShutdownState;
    g_WestwoodOnlineUpgradeApiEventSinkConnectionOffset = oldSinkOffset;
    g_WestwoodOnlineUpgradeApiInitState = oldInitState;
    g_WestwoodOnlineUpgradeCloseHandle0 = oldHandle0;
    g_WestwoodOnlineUpgradeCloseHandle1 = oldHandle1;
    g_WestwoodOnlineUpgradeCloseHandle2 = oldHandle2;

    RestoreFunctionPatch(showModalPatch);
    RestoreImportPatch(imports[1]);
    RestoreImportPatch(imports[0]);
    return failure;
}

extern "C" int westwood_online_upgrade_api_init_smoke(void)
{
    const WORD kMfc42CDialogCreateOrdinal = 2086;
    const WORD kMfc42CWndSetDlgItemTextAOrdinal = 5953;
    ImportFunctionPatch imports[7] = {};
    CodeFunctionPatch patches[2] = {};
    if (!PatchImportByName("KERNEL32.dll", "CreateEventA",
                           (void *)&FakeInitCreateEventA, imports[0]) ||
        !PatchImportByName("KERNEL32.dll", "WaitForMultipleObjects",
                           (void *)&FakeInitWaitForMultipleObjects, imports[1]) ||
        !PatchImportByName("KERNEL32.dll", "ResetEvent",
                           (void *)&FakeInitResetEvent, imports[2]) ||
        !PatchImportByName("KERNEL32.dll", "Sleep",
                           (void *)&FakeInitSleep, imports[3]) ||
        !PatchImportByName("KERNEL32.dll", "GetSystemDefaultLangID",
                           (void *)&FakeInitGetSystemDefaultLangID, imports[4]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogCreateOrdinal,
                              (void *)&FakeInitCreateProgress, imports[5]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetDlgItemTextAOrdinal,
                              (void *)&FakeInitSetDlgItemTextA, imports[6]) ||
        !PatchFunctionJump(MethodAddress(
                               &WestwoodOnlineUpgradeApi::CreateInstanceAndLoadConfig),
                           (void *)&FakeInitCreateInstanceAndLoadConfig, patches[0]) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString, patches[1]))
    {
        for (int index = 1; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        for (int index = 6; index >= 0; --index)
        {
            RestoreImportPatch(imports[index]);
        }
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    WestwoodOnlineUpgradeProgressDialog *const oldProgressDialog =
        g_pWestwoodOnlineUpgradeProgressDialog;
    HINSTANCE const oldModule = g_hWestwoodOnlineUpgradeModuleInstance;
    const int oldAsyncError = g_WestwoodOnlineUpgradeApiAsyncErrorFlag;
    const int oldReadyFlag = g_WestwoodOnlineUpgradeApiReadyFlag;
    const int oldAbortFlag = g_WestwoodOnlineUpgradeAbortFlag;
    HANDLE const oldWaitEvent0 = g_WestwoodOnlineUpgradeInitWaitEvents[0];
    HANDLE const oldWaitEvent1 = g_WestwoodOnlineUpgradeInitWaitEvents[1];
    HANDLE const oldWaitEvent2 = g_WestwoodOnlineUpgradeInitWaitEvents[2];
    HANDLE const oldBootstrapEvent = g_WestwoodOnlineUpgradeBootstrapServerListEvent;
    HANDLE const oldStatusEvent = g_WestwoodOnlineUpgradeStatusTextEvent;
    HANDLE const oldFailureEvent = g_WestwoodOnlineUpgradeFailureEvent;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    unsigned char progressDialogStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] = {0};
    WestwoodOnlineUpgradeProgressDialog &progressDialog = *(WestwoodOnlineUpgradeProgressDialog *)progressDialogStorage;
    dialog.m_selectedProfileConnectStringMode = 0;
    new (&dialog.m_selectedProfilePlayerName) CString("InitPilot");
    new (&dialog.m_selectedProfileConnectString) CString("InitConnect");
    *(RecoilNamedVtable **)&progressDialog =
        (RecoilNamedVtable *)&g_initProgressVtable;

    g_initFakeApiVtable.QueryInterface = 0;
    g_initFakeApiVtable.AddRef = 0;
    g_initFakeApiVtable.Release = 0;
    g_initFakeApiVtable.ProcessCallbacks = FakeInitProcessCallbacks;
    g_initFakeApiVtable.BeginConnect = FakeInitBeginConnect;
    g_initFakeApiVtable.RequestBootstrapServerList =
        FakeInitRequestBootstrapServerList;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_initFakeApiVtable.SubmitQueryRequest = 0;
    g_initFakeApiVtable.LoadBrowseRecord = 0;
    g_initFakeApiVtable.ResetQueryState = FakeResetQueryState;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initProgressVtable.DestroyWindow = FakeInitDestroyProgress;

    g_initCreateInstanceCalls = 0;
    g_initCreateInstanceBootstrap = 0;
    g_initCreateEventCalls = 0;
    g_initCreatedEvents[0] = (HANDLE)0x11110001;
    g_initCreatedEvents[1] = (HANDLE)0x11110002;
    g_initCreatedEvents[2] = (HANDLE)0x11110003;
    g_initCreateProgressCalls = 0;
    g_initCreateProgressResource = 0;
    g_initCreateProgressParent = 0;
    g_initSetDlgItemTextCalls = 0;
    g_initMessageIdCalls = 0;
    g_initBeginConnectCalls = 0;
    g_initBeginConnectLanguageId = 0;
    g_initBeginConnectProductId = 0;
    g_initBeginConnectPlayerName = 0;
    g_initBeginConnectConnectString = 0;
    g_initBeginConnectTimeoutSeconds = 0;
    g_initProcessCallbacksCalls = 0;
    g_initRequestBootstrapCalls = 0;
    g_initRequestBootstrapServer = 0;
    g_initRequestBootstrapTimeoutSeconds = 0;
    g_initRequestBootstrapUseAlternate = 0;
    g_initRequestListModeCalls = 0;
    g_initRequestListMode = 0;
    g_initRequestListModeEnabled = 0;
    g_initDestroyProgressCalls = 0;
    g_initDestroyedProgress = 0;
    g_initWaitResults[0] = WAIT_OBJECT_0;
    g_initWaitResults[1] = WAIT_OBJECT_0;
    g_initWaitResultCount = 2;
    g_initWaitCalls = 0;
    g_initWaitHandles = 0;
    g_initResetEventCalls = 0;
    g_initResetEventHandle = 0;
    g_initSleepCalls = 0;

    g_hWestwoodOnlineUpgradeModuleInstance = (HINSTANCE)0x22223333;
    g_pWestwoodOnlineUpgradeApi = 0;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_pWestwoodOnlineUpgradeProgressDialog = &progressDialog;
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 99;
    g_WestwoodOnlineUpgradeApiReadyFlag = 1;
    g_WestwoodOnlineUpgradeAbortFlag = 77;
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0x5a,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));

    const int result = WestwoodOnlineUpgradeApi::Init();
    int failure = 0;
    if (result != 1 || g_initCreateInstanceCalls != 1 ||
        g_initCreateInstanceBootstrap != (HANDLE)0x22223333 ||
        g_initCreateEventCalls != 3)
    {
        failure = 2;
    }
    else if (g_WestwoodOnlineUpgradeInitWaitEvents[0] != g_initCreatedEvents[0] ||
             g_WestwoodOnlineUpgradeInitWaitEvents[1] != g_initCreatedEvents[1] ||
             g_WestwoodOnlineUpgradeInitWaitEvents[2] != g_initCreatedEvents[2] ||
             g_WestwoodOnlineUpgradeFailureEvent != g_initCreatedEvents[2])
    {
        failure = 3;
    }
    else if (g_initCreateProgressCalls != 1 ||
             g_initCreateProgressResource != (LPCSTR)157 ||
             g_initSetDlgItemTextCalls != 2 ||
             g_initSetDlgItemTextThis[0] != &progressDialog ||
             g_initSetDlgItemTextControlId[0] != 1179 ||
             g_initSetDlgItemTextThis[1] != &dialog ||
             g_initSetDlgItemTextControlId[1] != 154)
    {
        failure = 4;
    }
    else if (g_initMessageIdCalls != 2 ||
             g_initMessageIds[0] != 0x3033 ||
             g_initMessageIds[1] != 0x3034)
    {
        failure = 5;
    }
    else if (g_initBeginConnectCalls != 1 ||
             g_initBeginConnectLanguageId != 0x1102 ||
             g_initBeginConnectProductId != 0x10003 ||
             strcmp(g_initBeginConnectPlayerName, "InitPilot") != 0 ||
             strcmp(g_initBeginConnectConnectString, "InitConnect") != 0 ||
             g_initBeginConnectTimeoutSeconds != 60)
    {
        failure = 6;
    }
    else if (g_initWaitCalls != 2 ||
             g_initWaitHandles != &g_WestwoodOnlineUpgradeInitWaitEvents[0] ||
             g_initWaitTimeouts[0] != 0 ||
             g_initWaitTimeouts[1] != 5 ||
             g_initResetEventCalls != 1 ||
             g_initResetEventHandle != g_initCreatedEvents[2] ||
             g_WestwoodOnlineUpgradeAbortFlag != 0 ||
             g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0)
    {
        failure = 7;
    }
    else if (g_initRequestBootstrapCalls != 1 ||
             g_initRequestBootstrapServer !=
                 &g_WestwoodOnlineUpgradeSelectedBootstrapServer ||
             g_initRequestBootstrapTimeoutSeconds != 30 ||
             g_initRequestBootstrapUseAlternate != 1 ||
             g_initDestroyProgressCalls != 1 ||
             g_initDestroyedProgress != (CWnd *)&progressDialog)
    {
        failure = 8;
    }
    else if (g_initRequestListModeCalls != 1 ||
             g_initRequestListMode != 17 ||
             g_initRequestListModeEnabled != 1 ||
             g_initSleepCalls != 0 ||
             ((unsigned char *)&g_WestwoodOnlineUpgradeCachedBrowseRecord)[0] != 0)
    {
        failure = 9;
    }

    dialog.m_selectedProfileConnectString.CString::~CString();
    dialog.m_selectedProfilePlayerName.CString::~CString();
    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeProgressDialog = oldProgressDialog;
    g_hWestwoodOnlineUpgradeModuleInstance = oldModule;
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = oldAsyncError;
    g_WestwoodOnlineUpgradeApiReadyFlag = oldReadyFlag;
    g_WestwoodOnlineUpgradeAbortFlag = oldAbortFlag;
    g_WestwoodOnlineUpgradeInitWaitEvents[0] = oldWaitEvent0;
    g_WestwoodOnlineUpgradeInitWaitEvents[1] = oldWaitEvent1;
    g_WestwoodOnlineUpgradeInitWaitEvents[2] = oldWaitEvent2;
    g_WestwoodOnlineUpgradeBootstrapServerListEvent = oldBootstrapEvent;
    g_WestwoodOnlineUpgradeStatusTextEvent = oldStatusEvent;
    g_WestwoodOnlineUpgradeFailureEvent = oldFailureEvent;
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;

    for (int index = 1; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    for (int index = 6; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_on_bootstrap_server_list_smoke(void)
{
    ImportFunctionPatch setEventPatch = {};
    if (!PatchImportByName("KERNEL32.dll",
                           "SetEvent",
                           (void *)&FakeBootstrapSetEvent,
                           setEventPatch))
    {
        return 1;
    }

    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    HANDLE const oldFailureEvent = g_WestwoodOnlineUpgradeFailureEvent;
    HANDLE const oldInitWaitEvent0 = g_WestwoodOnlineUpgradeInitWaitEvents[0];
    WestwoodOnlineUpgradeBootstrapServerRecord const oldSelected =
        g_WestwoodOnlineUpgradeSelectedBootstrapServer;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    new (&dialog.m_selectedProfilePlayerName) CString("Pilot");
    new (&dialog.m_selectedProfileConnectString) CString("Conn");
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_WestwoodOnlineUpgradeFailureEvent = (HANDLE)0x11110000;
    g_WestwoodOnlineUpgradeInitWaitEvents[0] = (HANDLE)0x22220000;

    WestwoodOnlineUpgradeBootstrapServerRecord serverA = {};
    WestwoodOnlineUpgradeBootstrapServerRecord serverB = {};
    WestwoodOnlineUpgradeBootstrapServerRecord serverC = {};
    serverA.m_gameType = 1;
    serverA.m_next = &serverB;
    strcpy(serverA.m_serverName, "Alpha");
    strcpy(serverA.m_serverType, "CHAT");
    strcpy(serverA.m_connectData, "alpha-data");
    serverB.m_gameType = 2;
    serverB.m_next = &serverC;
    strcpy(serverB.m_serverName, "Beta");
    strcpy(serverB.m_serverType, "IRC");
    strcpy(serverB.m_connectData, "beta-data");
    strcpy(serverB.m_playerName, "OldP");
    strcpy(serverB.m_connectString, "OldC");
    serverB.reserved0f8[0] = 0x7c;
    serverC.m_gameType = 3;
    strcpy(serverC.m_serverName, "Gamma");
    strcpy(serverC.m_serverType, "IRC");
    strcpy(serverC.m_connectData, "gamma-data");

    int failure = 0;
    memset(&g_WestwoodOnlineUpgradeSelectedBootstrapServer,
           0x5a,
           sizeof(g_WestwoodOnlineUpgradeSelectedBootstrapServer));
    g_bootstrapSetEventCalls = 0;
    int result =
        WestwoodOnlineUpgradeDialog::OnBootstrapServerList(0, -5, &serverA);
    if (result != 0 ||
        g_bootstrapSetEventCalls != 1 ||
        g_bootstrapSetEventHandles[0] != (HANDLE)0x11110000 ||
        ((unsigned char *)&g_WestwoodOnlineUpgradeSelectedBootstrapServer)[0] !=
            0x5a)
    {
        failure = 2;
    }

    memset(&g_WestwoodOnlineUpgradeSelectedBootstrapServer,
           0,
           sizeof(g_WestwoodOnlineUpgradeSelectedBootstrapServer));
    g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_gameType = 99;
    g_bootstrapSetEventCalls = 0;
    result = WestwoodOnlineUpgradeDialog::OnBootstrapServerList(0, 0, 0);
    if (failure == 0 &&
        (result != 0 ||
         g_bootstrapSetEventCalls != 1 ||
         g_bootstrapSetEventHandles[0] != (HANDLE)0x22220000 ||
         g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_gameType != 99 ||
         strcmp(g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_playerName,
                "Pilot") != 0 ||
         strcmp(g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_connectString,
                "Conn") != 0))
    {
        failure = 3;
    }

    memset(&g_WestwoodOnlineUpgradeSelectedBootstrapServer,
           0,
           sizeof(g_WestwoodOnlineUpgradeSelectedBootstrapServer));
    g_WestwoodOnlineUpgradeSelectedBootstrapServer.reserved0f8[0] = 0x6b;
    g_bootstrapSetEventCalls = 0;
    result = WestwoodOnlineUpgradeDialog::OnBootstrapServerList(0, 0, &serverA);
    if (failure == 0 &&
        (result != 0 ||
         g_bootstrapSetEventCalls != 1 ||
         g_bootstrapSetEventHandles[0] != (HANDLE)0x22220000 ||
         g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_gameType != 2 ||
         g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_next != &serverC ||
         strcmp(g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_serverName,
                "Beta") != 0 ||
         strcmp(g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_serverType,
                "IRC") != 0 ||
         strcmp(g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_connectData,
                "beta-data") != 0 ||
         strcmp(g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_playerName,
                "Pilot") != 0 ||
         strcmp(g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_connectString,
                "Conn") != 0 ||
         g_WestwoodOnlineUpgradeSelectedBootstrapServer.reserved0f8[0] != 0x6b))
    {
        failure = 4;
    }

    dialog.m_selectedProfileConnectString.CString::~CString();
    dialog.m_selectedProfilePlayerName.CString::~CString();
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_WestwoodOnlineUpgradeFailureEvent = oldFailureEvent;
    g_WestwoodOnlineUpgradeInitWaitEvents[0] = oldInitWaitEvent0;
    g_WestwoodOnlineUpgradeSelectedBootstrapServer = oldSelected;
    RestoreImportPatch(setEventPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_append_status_text_smoke(void)
{
    HWND listBox = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_NOTIFY, 0, 0, 200,
                                  10, 0, 0, GetModuleHandleA(0), 0);
    if (listBox == 0)
    {
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_statusList.m_hWnd = listBox;
    int failure = 0;

    dialog.AppendStatusTextFmt("\nstatus %d\n", 7);
    char text[64] = {};
    SendMessageA(listBox, LB_GETTEXT, 0, (LPARAM)text);
    if (dialog.m_statusLineCount != 1 ||
        SendMessageA(listBox, LB_GETCOUNT, 0, 0) != 1 ||
        strcmp(text, "status 7") != 0)
    {
        failure = 2;
    }

    if (failure == 0)
    {
        SendMessageA(listBox, LB_RESETCONTENT, 0, 0);
        dialog.m_statusLineCount = 0;
        dialog.AppendStatusTextFmt("\n");
        text[0] = 'x';
        SendMessageA(listBox, LB_GETTEXT, 0, (LPARAM)text);
        if (dialog.m_statusLineCount != 1 ||
            SendMessageA(listBox, LB_GETCOUNT, 0, 0) != 1 ||
            text[0] != '\0')
        {
            failure = 3;
        }
    }

    if (failure == 0)
    {
        SendMessageA(listBox, LB_RESETCONTENT, 0, 0);
        dialog.m_statusLineCount = 0;
        for (int index = 0; index < 105; ++index)
        {
            dialog.AppendStatusTextFmt("line %d", index);
        }

        SendMessageA(listBox, LB_GETTEXT, 0, (LPARAM)text);
        if (dialog.m_statusLineCount != 100)
        {
            failure = 4;
        }
        else if (SendMessageA(listBox, LB_GETCOUNT, 0, 0) != 100)
        {
            failure = 5;
        }
        else if (strcmp(text, "line 5") != 0)
        {
            failure = 6;
        }
    }

    DestroyWindow(listBox);
    return failure;
}

extern "C" int westwood_online_upgrade_truncate_string_at_first_space_smoke(void)
{
    char emptyText[] = "";
    char noSpaceText[] = "Alpha";
    char leadingSpaceText[] = " Alpha";
    char firstSpaceText[] = "Alpha Beta Gamma";

    WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(emptyText);
    WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(noSpaceText);
    WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(leadingSpaceText);
    WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(firstSpaceText);

    if (strcmp(emptyText, "") != 0 ||
        strcmp(noSpaceText, "Alpha") != 0 ||
        strcmp(leadingSpaceText, "") != 0 ||
        strcmp(firstSpaceText, "Alpha") != 0)
    {
        return 1;
    }
    return 0;
}

extern "C" int westwood_online_upgrade_dialog_query_sessions_by_name_smoke(void)
{
    const WORD kMfc42CWndGetWindowTextACStringOrdinal = 3874;
    const WORD kMfc42CWndGetWindowTextAOrdinal = 3873;
    const WORD kMfc42CWndMessageBoxAOrdinal = 4224;
    const WORD kMfc42CWndEnableWindowOrdinal = 2642;
    ImportFunctionPatch imports[5] = {};
    CodeFunctionPatch setWindowTextPatch = {};
    CodeFunctionPatch updateDataPatch = {};
    CodeFunctionPatch zlocPatch = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndGetWindowTextACStringOrdinal,
                              (void *)&FakeRefreshCurrentQueryGetWindowTextA,
                              imports[0]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndGetWindowTextAOrdinal,
                              (void *)&FakeQueryStatusGetWindowTextA,
                              imports[1]) ||
        !PatchFunctionJump(CWndSetWindowTextAAddress(),
                           (void *)&FakeQueryStatusSetWindowTextA,
                           setWindowTextPatch) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndMessageBoxAOrdinal,
                              (void *)&FakeQueryStatusMessageBoxA,
                              imports[3]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndEnableWindowOrdinal,
                              (void *)&FakeWolEnableWindow,
                              imports[4]) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           zlocPatch) ||
        !PatchFunctionJump(CWndUpdateDataAddress(), (void *)&FakeWestwoodUpdateData,
                           updateDataPatch))
    {
        RestoreFunctionPatch(updateDataPatch);
        RestoreFunctionPatch(zlocPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        for (int index = 4; index >= 0; --index)
        {
            RestoreImportPatch(imports[index]);
        }
        return 1;
    }

    HWND listBox = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_NOTIFY, 0, 0, 200,
                                  10, 0, 0, GetModuleHandleA(0), 0);
    if (listBox == 0)
    {
        RestoreFunctionPatch(updateDataPatch);
        RestoreFunctionPatch(zlocPatch);
        RestoreFunctionPatch(setWindowTextPatch);
        for (int index = 4; index >= 0; --index)
        {
            RestoreImportPatch(imports[index]);
        }
        return 2;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    new (&dialog.m_sessionName) CString();
    dialog.m_queryMaxPlayers = 14;
    dialog.m_statusList.m_hWnd = listBox;
    dialog.m_statusLineCount = 2;
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"first");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"second");
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.ResetQueryState = FakeResetQueryState;
    g_initFakeApiVtable.SubmitQueryRequest = FakeRefreshCurrentQuerySubmit;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    strcpy(g_refreshCurrentQuerySessionNameInput, "  Alpha  ");
    strcpy(g_queryStatusServerInput, "Srv123");
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = 'S';
    g_WestwoodOnlineUpgradeActiveListMode = 0;
    g_refreshCurrentQueryGetWindowTextCalls = 0;
    g_queryStatusGetWindowTextCalls = 0;
    g_queryStatusSetWindowTextCalls = 0;
    g_refreshCurrentQuerySubmitCalls = 0;
    g_refreshCurrentQuerySubmitResult = 0;
    g_resetQueryStateCalls = 0;
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatUpdateDataSaveValue[0] = -1;
    g_enableWindowCalls = 0;
    g_queryStatusMessageBoxCalls = 0;
    g_initMessageIdCalls = 0;

    int failure = 0;
    dialog.OnQuerySessionsByName();
    if (g_refreshCurrentQueryGetWindowTextCalls != 1 ||
        g_refreshCurrentQueryGetWindowTextThis != &dialog.m_sessionNameEdit ||
        g_queryStatusGetWindowTextCalls != 1 ||
        g_queryStatusGetWindowTextThis[0] != &dialog.m_serverAddressEdit ||
        g_queryStatusGetWindowTextMaxCount[0] != 8 ||
        g_queryStatusSetWindowTextCalls != 2 ||
        strcmp(g_queryStatusSetWindowTextValue[0], "Srv123") != 0 ||
        strcmp(g_queryStatusSetWindowTextValue[1], "") != 0 ||
        g_resetQueryStateCalls != 1 ||
        g_refreshCurrentQuerySubmitCalls != 1 ||
        g_refreshCurrentQuerySubmitRequest.m_listMode != 17 ||
        g_refreshCurrentQuerySubmitRequest.m_queryVariant != 2 ||
        g_refreshCurrentQuerySubmitRequest.m_queryMaxPlayers != 14 ||
        g_refreshCurrentQuerySubmitRequest.m_queryExtraParam != 0 ||
        strcmp(g_refreshCurrentQuerySubmitRequest.m_sessionName, "Alpha") != 0 ||
        strcmp(g_refreshCurrentQuerySubmitRequest.m_serverAddress, "Srv123") != 0 ||
        strcmp((const char *)dialog.m_sessionName, "Alpha") != 0 ||
        g_threeFloatUpdateDataCount != 1 ||
        g_threeFloatUpdateDataSaveValue[0] != 0 ||
        dialog.m_statusLineCount != 0 ||
        SendMessageA(listBox, LB_GETCOUNT, 0, 0) != 0 ||
        g_WestwoodOnlineUpgradeActiveListMode != 17 ||
        g_enableWindowCalls != 2 ||
        g_enableWindowThis[0] != &dialog.m_querySessionsByNameButton ||
        g_enableWindowThis[1] != &dialog.m_queueVisibleSessionRequestsButton ||
        g_enableWindowEnable[0] != 1 ||
        g_enableWindowEnable[1] != 1 ||
        g_queryStatusMessageBoxCalls != 0 ||
        g_initMessageIdCalls != 0)
    {
        failure = 3;
    }

    strcpy(g_refreshCurrentQuerySessionNameInput, "   ");
    g_refreshCurrentQuerySubmitCalls = 0;
    g_queryStatusSetWindowTextCalls = 0;
    g_queryStatusMessageBoxCalls = 0;
    g_queryStatusMessageBoxText[0] = '\0';
    g_queryStatusMessageBoxCaption[0] = '\0';
    g_initMessageIdCalls = 0;
    dialog.OnQuerySessionsByName();
    if (failure == 0 &&
        (g_refreshCurrentQuerySubmitCalls != 0 ||
         g_queryStatusSetWindowTextCalls != 0 ||
         g_queryStatusMessageBoxCalls != 1 ||
         strcmp(g_queryStatusMessageBoxText, "msg-303f") != 0 ||
         strcmp(g_queryStatusMessageBoxCaption, "msg-3003") != 0 ||
         g_initMessageIds[0] != 0x3003 ||
         g_initMessageIds[1] != 0x303f))
    {
        failure = 4;
    }

    strcpy(g_refreshCurrentQuerySessionNameInput, "Alpha Beta");
    g_queryStatusMessageBoxCalls = 0;
    g_queryStatusMessageBoxText[0] = '\0';
    g_queryStatusMessageBoxCaption[0] = '\0';
    g_initMessageIdCalls = 0;
    dialog.OnQuerySessionsByName();
    if (failure == 0 &&
        (g_refreshCurrentQuerySubmitCalls != 0 ||
         g_queryStatusMessageBoxCalls != 1 ||
         strcmp(g_queryStatusMessageBoxText, "msg-303e") != 0 ||
         strcmp(g_queryStatusMessageBoxCaption, "msg-3003") != 0 ||
         g_initMessageIds[0] != 0x3003 ||
         g_initMessageIds[1] != 0x303e))
    {
        failure = 5;
    }

    strcpy(g_refreshCurrentQuerySessionNameInput, "Gamma");
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_refreshCurrentQuerySubmitCalls = 0;
    g_refreshCurrentQuerySubmitResult = 0x800401f7;
    g_queryStatusMessageBoxCalls = 0;
    g_queryStatusMessageBoxText[0] = '\0';
    g_queryStatusMessageBoxCaption[0] = '\0';
    g_initMessageIdCalls = 0;
    dialog.OnQuerySessionsByName();
    if (failure == 0 &&
        (g_refreshCurrentQuerySubmitCalls != 1 ||
         g_queryStatusMessageBoxCalls != 1 ||
         strcmp(g_queryStatusMessageBoxText, "msg-3042") != 0 ||
         strcmp(g_queryStatusMessageBoxCaption, "msg-3003") != 0 ||
         g_initMessageIds[0] != 0x3003 ||
         g_initMessageIds[1] != 0x3042))
    {
        failure = 6;
    }

    strcpy(g_refreshCurrentQuerySessionNameInput, "Delta");
    g_refreshCurrentQuerySubmitCalls = 0;
    g_refreshCurrentQuerySubmitResult = 0x800401f8;
    g_queryStatusMessageBoxCalls = 0;
    g_queryStatusMessageBoxText[0] = '\0';
    g_initMessageIdCalls = 0;
    dialog.OnQuerySessionsByName();
    if (failure == 0 &&
        (g_refreshCurrentQuerySubmitCalls != 1 ||
         g_queryStatusMessageBoxCalls != 1 ||
         strcmp(g_queryStatusMessageBoxText, "msg-303e") != 0 ||
         g_initMessageIds[0] != 0x3003 ||
         g_initMessageIds[1] != 0x303e))
    {
        failure = 7;
    }

    strcpy(g_refreshCurrentQuerySessionNameInput, "Epsilon");
    g_refreshCurrentQuerySubmitCalls = 0;
    g_refreshCurrentQuerySubmitResult = -9;
    g_queryStatusMessageBoxCalls = 0;
    g_queryStatusMessageBoxText[0] = '\0';
    g_initMessageIdCalls = 0;
    dialog.OnQuerySessionsByName();
    if (failure == 0 &&
        (g_refreshCurrentQuerySubmitCalls != 1 ||
         g_queryStatusMessageBoxCalls != 1 ||
         strcmp(g_queryStatusMessageBoxText, "msg-3003") != 0 ||
         g_initMessageIds[0] != 0x3003 ||
         g_initMessageIds[1] != 0x3003))
    {
        failure = 8;
    }

    dialog.m_sessionName.~CString();
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    DestroyWindow(listBox);
    RestoreFunctionPatch(updateDataPatch);
    RestoreFunctionPatch(zlocPatch);
    RestoreFunctionPatch(setWindowTextPatch);
    for (int index = 4; index >= 0; --index)
    {
        RestoreImportPatch(imports[index]);
    }
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_queue_visible_session_requests_smoke(void)
{
    HWND listBox = CreateWindowExA(0,
                                   "LISTBOX",
                                   "",
                                   WS_POPUP | LBS_MULTIPLESEL,
                                   0,
                                   0,
                                   200,
                                   10,
                                   0,
                                   0,
                                   GetModuleHandleA(0),
                                   0);
    if (listBox == 0)
    {
        return 1;
    }

    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Self Server");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Alpha Server");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Beta");
    SendMessageA(listBox, LB_SETSEL, TRUE, 0);
    SendMessageA(listBox, LB_SETSEL, TRUE, 1);
    SendMessageA(listBox, LB_SETSEL, TRUE, 2);

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldCreateSessionFromQueryFlag =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldVisibleSessionResultCount =
        g_WestwoodOnlineUpgradeVisibleSessionResultCount;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionResultsList.m_hWnd = listBox;
    new (&dialog.m_selectedProfilePlayerName) CString();
    dialog.m_selectedProfilePlayerName = "Self";

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.QueueSessionRequest = FakeQueueSessionRequest;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    int failure = 0;
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradeVisibleSessionResultCount = 123;
    g_queueSessionRequestCalls = 0;
    dialog.QueueVisibleSessionRequests();
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 123 ||
        g_queueSessionRequestCalls != 0)
    {
        failure = 2;
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = 'S';
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    g_WestwoodOnlineUpgradeVisibleSessionResultCount = 0;
    g_queueSessionRequestCalls = 0;
    dialog.QueueVisibleSessionRequests();
    if (failure == 0 &&
        (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 3 ||
         g_queueSessionRequestCalls != 0))
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_queueSessionRequestCalls = 0;
    memset(g_queueSessionRequestCopies, 0, sizeof(g_queueSessionRequestCopies));
    dialog.QueueVisibleSessionRequests();
    if (failure == 0 &&
        (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 3 ||
         g_queueSessionRequestCalls != 2 ||
         g_queueSessionRequestSelf[0] != (IUnknown *)&g_initFakeApi ||
         g_queueSessionRequestSelf[1] != (IUnknown *)&g_initFakeApi ||
         strcmp(g_queueSessionRequestCopies[0].m_sessionName, "Alpha") != 0 ||
         strcmp(g_queueSessionRequestCopies[1].m_sessionName, "Beta") != 0))
    {
        failure = 4;
    }

    SendMessageA(listBox, LB_SETSEL, FALSE, -1);
    g_queueSessionRequestCalls = 0;
    dialog.QueueVisibleSessionRequests();
    if (failure == 0 &&
        (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 0 ||
         g_queueSessionRequestCalls != 0))
    {
        failure = 5;
    }

    dialog.m_selectedProfilePlayerName.~CString();
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag =
        oldCreateSessionFromQueryFlag;
    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        oldVisibleSessionResultCount;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    DestroyWindow(listBox);
    return failure;
}

extern "C" int
westwood_online_upgrade_dialog_queue_visible_session_requests_lookup_smoke(void)
{
    HWND listBox = CreateWindowExA(0,
                                   "LISTBOX",
                                   "",
                                   WS_POPUP | LBS_MULTIPLESEL,
                                   0,
                                   0,
                                   200,
                                   10,
                                   0,
                                   0,
                                   GetModuleHandleA(0),
                                   0);
    if (listBox == 0)
    {
        return 1;
    }

    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Self Server");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Alpha Server");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Beta");
    SendMessageA(listBox, LB_SETSEL, TRUE, 0);
    SendMessageA(listBox, LB_SETSEL, TRUE, 1);
    SendMessageA(listBox, LB_SETSEL, TRUE, 2);

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldCreateSessionFromQueryFlag =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldVisibleSessionResultCount =
        g_WestwoodOnlineUpgradeVisibleSessionResultCount;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionResultsList.m_hWnd = listBox;
    new (&dialog.m_selectedProfilePlayerName) CString();
    dialog.m_selectedProfilePlayerName = "Self";

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.QueueSessionRequest = FakeQueueSessionRequest;
    g_initFakeApiVtable.LookupBrowseRecordBySessionName =
        FakeLookupBrowseRecordBySessionName;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = 'S';
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_queueSessionRequestCalls = 0;
    g_lookupBrowseRecordCalls = 0;
    memset(g_queueSessionRequestCopies, 0, sizeof(g_queueSessionRequestCopies));
    memset(g_lookupBrowseRecordSessionName,
           0,
           sizeof(g_lookupBrowseRecordSessionName));

    int failure = 0;
    dialog.QueueVisibleSessionRequestsAndLookupBrowseRecords();
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 3 ||
        g_lookupBrowseRecordCalls != 2 ||
        g_queueSessionRequestCalls != 2 ||
        g_lookupBrowseRecordSelf[0] != (IUnknown *)&g_initFakeApi ||
        g_lookupBrowseRecordSelf[1] != (IUnknown *)&g_initFakeApi ||
        strcmp(g_lookupBrowseRecordSessionName[0], "Alpha") != 0 ||
        strcmp(g_lookupBrowseRecordSessionName[1], "Beta") != 0 ||
        g_lookupBrowseRecordMode[0] != 1 ||
        g_lookupBrowseRecordMode[1] != 1 ||
        strcmp(g_queueSessionRequestCopies[0].m_sessionName, "Alpha") != 0 ||
        strcmp(g_queueSessionRequestCopies[1].m_sessionName, "Beta") != 0)
    {
        failure = 2;
    }

    SendMessageA(listBox, LB_SETSEL, FALSE, -1);
    g_queueSessionRequestCalls = 0;
    g_lookupBrowseRecordCalls = 0;
    dialog.QueueVisibleSessionRequestsAndLookupBrowseRecords();
    if (failure == 0 &&
        (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 0 ||
         g_lookupBrowseRecordCalls != 0 ||
         g_queueSessionRequestCalls != 0))
    {
        failure = 3;
    }

    dialog.m_selectedProfilePlayerName.~CString();
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag =
        oldCreateSessionFromQueryFlag;
    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        oldVisibleSessionResultCount;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    DestroyWindow(listBox);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_browse_record_dblclk_smoke(void)
{
    const WORD kMfc42CWndGetWindowTextAOrdinal = 3873;
    ImportFunctionPatch getWindowTextPatch = {};
    CodeFunctionPatch setWindowTextPatch = {};
    CodeFunctionPatch patches[4] = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndGetWindowTextAOrdinal,
                              (void *)&FakeQueryStatusGetWindowTextA,
                              getWindowTextPatch) ||
        !PatchFunctionJump(CWndSetWindowTextAAddress(),
                           (void *)&FakeQueryStatusSetWindowTextA,
                           setWindowTextPatch) ||
        !PatchFunctionJump(
            MethodAddress(
                &WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList),
            (void *)&FakeAppendConnectResetSelectedBrowseRecord,
            patches[0]) ||
        !PatchFunctionJump(MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
                           (void *)&FakeAppendConnectAppendStatusTextFmt,
                           patches[1]) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[2]) ||
        !PatchFunctionJump(CWndUpdateDataAddress(), (void *)&FakeWestwoodUpdateData,
                           patches[3]))
    {
        for (int index = 3; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreImportPatch(getWindowTextPatch);
        return 1;
    }

    HWND browseList = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_NOTIFY,
                                      0, 0, 200, 10, 0, 0,
                                      GetModuleHandleA(0), 0);
    HWND statusList = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_NOTIFY,
                                      0, 0, 200, 10, 0, 0,
                                      GetModuleHandleA(0), 0);
    if (browseList == 0 || statusList == 0)
    {
        if (statusList != 0)
        {
            DestroyWindow(statusList);
        }
        if (browseList != 0)
        {
            DestroyWindow(browseList);
        }
        for (int index = 3; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreImportPatch(getWindowTextPatch);
        return 2;
    }

    SendMessageA(browseList, LB_ADDSTRING, 0, (LPARAM)"Alpha");
    SendMessageA(browseList, LB_SETCURSEL, 0, 0);

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;
    WestwoodOnlineUpgradeBrowseRecord const oldListRecord0 =
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[0];

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.LoadBrowseRecord = FakeLoadBrowseRecord;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    new (&dialog.m_sessionName) CString();
    dialog.m_browseRecordList.m_hWnd = browseList;
    dialog.m_statusList.m_hWnd = statusList;
    dialog.m_serverAddressEdit.m_hWnd = (HWND)0x12340005;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecordList[0],
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecordList[0]));
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName, "Alpha");
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecordList[0].m_sessionName,
           "Alpha");
    g_loadBrowseRecordCalls = 0;
    g_appendConnectResetCalls = 0;

    int failure = 0;
    dialog.OnBrowseRecordListDblClk();
    if (g_loadBrowseRecordCalls != 0 || g_appendConnectResetCalls != 0)
    {
        failure = 3;
    }

    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName, "Old");
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecordList[0].m_sessionName,
           "Alpha");
    g_WestwoodOnlineUpgradeCachedBrowseRecordList[0].m_recordFlags = 1;
    strcpy(g_queryStatusServerInput, "Srv123");
    SendMessageA(statusList, LB_ADDSTRING, 0, (LPARAM)"old-status");
    dialog.m_statusLineCount = 1;
    g_loadBrowseRecordCalls = 0;
    g_loadBrowseRecordSelf = 0;
    g_loadBrowseRecordRecord = 0;
    memset(&g_loadBrowseRecordCopy, 0, sizeof(g_loadBrowseRecordCopy));
    g_loadBrowseRecordResult = 0;
    g_queryStatusGetWindowTextCalls = 0;
    g_queryStatusSetWindowTextCalls = 0;
    g_appendConnectResetCalls = 0;
    g_appendConnectResetThis = 0;
    g_appendConnectAppendCalls = 0;
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatUpdateDataSaveValue[0] = -1;

    dialog.OnBrowseRecordListDblClk();
    if (failure == 0 &&
        (g_appendConnectResetCalls != 1 ||
         g_appendConnectResetThis != &dialog ||
         g_queryStatusGetWindowTextCalls != 1 ||
         g_queryStatusGetWindowTextThis[0] != &dialog.m_serverAddressEdit ||
         g_queryStatusGetWindowTextMaxCount[0] != 8 ||
         g_queryStatusSetWindowTextCalls != 1 ||
         g_queryStatusSetWindowTextThis[0] != (void *)dialog.m_serverAddressEdit.m_hWnd ||
         strcmp(g_queryStatusSetWindowTextValue[0], "Srv123") != 0 ||
         g_loadBrowseRecordCalls != 1 ||
         g_loadBrowseRecordSelf != (IUnknown *)&g_initFakeApi ||
         g_loadBrowseRecordRecord != &g_WestwoodOnlineUpgradeCachedBrowseRecordList[0] ||
         strcmp(g_loadBrowseRecordCopy.m_sessionName, "Alpha") != 0 ||
         strcmp(g_loadBrowseRecordCopy.m_serverAddress, "Srv123") != 0 ||
         strcmp(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName,
                "Alpha") != 0 ||
         strcmp(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_serverAddress,
                "Srv123") != 0 ||
         strcmp((const char *)dialog.m_sessionName, "Alpha") != 0 ||
         g_threeFloatUpdateDataCount != 1 ||
         g_threeFloatUpdateDataSaveValue[0] != 0 ||
         dialog.m_statusLineCount != 0 ||
         SendMessageA(statusList, LB_GETCOUNT, 0, 0) != 0 ||
         g_appendConnectAppendCalls != 0))
    {
        failure = 4;
    }

    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecordList[0],
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecordList[0]));
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecordList[0].m_sessionName,
           "Beta");
    strcpy(g_queryStatusServerInput, "DupSrv");
    SendMessageA(statusList, LB_ADDSTRING, 0, (LPARAM)"keep-status");
    dialog.m_statusLineCount = 1;
    g_loadBrowseRecordCalls = 0;
    g_loadBrowseRecordResult = 0x800401f7;
    g_appendConnectAppendCalls = 0;
    g_appendConnectAppendThis = 0;
    g_appendConnectAppendText = 0;
    g_initMessageIdCalls = 0;
    g_threeFloatUpdateDataCount = 0;
    g_queryStatusSetWindowTextCalls = 0;

    dialog.OnBrowseRecordListDblClk();
    if (failure == 0 &&
        (g_loadBrowseRecordCalls != 1 ||
         g_appendConnectAppendCalls != 1 ||
         g_appendConnectAppendThis != &dialog ||
         strcmp(g_appendConnectAppendText, "msg-3042") != 0 ||
         g_initMessageIdCalls != 1 ||
         g_initMessageIds[0] != 0x3042 ||
         g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0' ||
         g_threeFloatUpdateDataCount != 0 ||
         dialog.m_statusLineCount != 1 ||
         SendMessageA(statusList, LB_GETCOUNT, 0, 0) != 1))
    {
        failure = 5;
    }

    dialog.m_sessionName.~CString();
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    g_WestwoodOnlineUpgradeCachedBrowseRecordList[0] = oldListRecord0;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    DestroyWindow(statusList);
    DestroyWindow(browseList);
    for (int index = 3; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreImportPatch(getWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_request_list_modes_smoke(void)
{
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_WestwoodOnlineUpgradeActiveListMode = 77;
    g_initRequestListModeCalls = 0;
    g_initRequestListMode = -1;
    g_initRequestListModeEnabled = -1;
    dialog.RequestListMode0();

    int failure = 0;
    if (g_WestwoodOnlineUpgradeActiveListMode != 0 ||
        g_initRequestListModeCalls != 1 ||
        g_initRequestListMode != 0 ||
        g_initRequestListModeEnabled != 0)
    {
        failure = 1;
    }

    g_WestwoodOnlineUpgradeActiveListMode = 77;
    g_initRequestListModeCalls = 0;
    g_initRequestListMode = -1;
    g_initRequestListModeEnabled = -1;
    dialog.RequestListMode11();
    if (failure == 0 &&
        (g_WestwoodOnlineUpgradeActiveListMode != 17 ||
         g_initRequestListModeCalls != 1 ||
         g_initRequestListMode != 17 ||
         g_initRequestListModeEnabled != 1))
    {
        failure = 2;
    }

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_session_mode_sel_change_smoke(void)
{
    const WORD kMfc42CWndSetDlgItemTextAOrdinal = 5953;
    const WORD kMfc42CWndEnableWindowOrdinal = 2642;
    ImportFunctionPatch imports[2] = {};
    CodeFunctionPatch updateDataPatch = {};
    CodeFunctionPatch zlocPatch = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetDlgItemTextAOrdinal,
                              (void *)&FakeInitSetDlgItemTextA, imports[0]) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CWndEnableWindowOrdinal,
                              (void *)&FakeWolEnableWindow, imports[1]) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           zlocPatch) ||
        !PatchFunctionJump(CWndUpdateDataAddress(), (void *)&FakeWestwoodUpdateData,
                           updateDataPatch))
    {
        RestoreFunctionPatch(updateDataPatch);
        RestoreFunctionPatch(zlocPatch);
        RestoreImportPatch(imports[1]);
        RestoreImportPatch(imports[0]);
        return 1;
    }

    HWND comboBox = CreateWindowExA(0,
                                    "COMBOBOX",
                                    "",
                                    WS_POPUP | CBS_DROPDOWNLIST,
                                    0,
                                    0,
                                    200,
                                    200,
                                    0,
                                    0,
                                    GetModuleHandleA(0),
                                    0);
    if (comboBox == 0)
    {
        RestoreFunctionPatch(updateDataPatch);
        RestoreFunctionPatch(zlocPatch);
        RestoreImportPatch(imports[1]);
        RestoreImportPatch(imports[0]);
        return 2;
    }

    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"value");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"time");
    SendMessageA(comboBox, CB_SETITEMDATA, 0, 3);
    SendMessageA(comboBox, CB_SETITEMDATA, 1, 2);

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.SubmitEncodedQueryString = FakeSubmitEncodedQueryString;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionModeCombo.m_hWnd = comboBox;
    dialog.m_queryValueOrTime = 5;
    dialog.m_queryAuxParam = 6;
    dialog.m_queryMaxPlayers = 7;
    dialog.m_queryStatusFlagBit0 = 1;
    dialog.m_queryStatusFlagBit1 = 0;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    SendMessageA(comboBox, CB_SETCURSEL, 1, 0);
    g_initSetDlgItemTextCalls = 0;
    g_enableWindowCalls = 0;
    g_initMessageIdCalls = 0;
    g_submitEncodedQueryCalls = 0;
    g_threeFloatUpdateDataCount = 0;
    dialog.OnSessionModeComboSelChange();

    int failure = 0;
    if (dialog.m_querySessionModeKind != 2 ||
        g_initSetDlgItemTextCalls != 1 ||
        g_initSetDlgItemTextThis[0] != &dialog ||
        g_initSetDlgItemTextControlId[0] != 1167 ||
        strcmp(g_initSetDlgItemTextValue[0], "msg-3040") != 0 ||
        g_enableWindowCalls != 1 ||
        g_enableWindowThis[0] != &dialog.m_queryValueOrTimeEdit ||
        g_enableWindowEnable[0] != 0 ||
        g_initMessageIds[0] != 0x3040 ||
        g_submitEncodedQueryCalls != 1 ||
        g_threeFloatUpdateDataCount != 1)
    {
        failure = 3;
    }

    SendMessageA(comboBox, CB_SETCURSEL, 0, 0);
    g_initSetDlgItemTextCalls = 0;
    g_enableWindowCalls = 0;
    g_initMessageIdCalls = 0;
    g_submitEncodedQueryCalls = 0;
    g_threeFloatUpdateDataCount = 0;
    dialog.OnSessionModeComboSelChange();
    if (failure == 0 &&
        (dialog.m_querySessionModeKind != 3 ||
         g_initSetDlgItemTextCalls != 1 ||
         g_initSetDlgItemTextThis[0] != &dialog ||
         g_initSetDlgItemTextControlId[0] != 1167 ||
         strcmp(g_initSetDlgItemTextValue[0], "msg-3041") != 0 ||
         g_enableWindowCalls != 1 ||
         g_enableWindowThis[0] != &dialog.m_queryValueOrTimeEdit ||
         g_enableWindowEnable[0] != 1 ||
         g_initMessageIds[0] != 0x3041 ||
         g_submitEncodedQueryCalls != 1 ||
         g_threeFloatUpdateDataCount != 1))
    {
        failure = 4;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    DestroyWindow(comboBox);
    RestoreFunctionPatch(updateDataPatch);
    RestoreFunctionPatch(zlocPatch);
    RestoreImportPatch(imports[1]);
    RestoreImportPatch(imports[0]);
    return failure;
}

extern "C" int
westwood_online_upgrade_dialog_submit_visible_session_requests_status_smoke(void)
{
    const WORD kMfc42CWndGetWindowTextACStringOrdinal = 3874;
    ImportFunctionPatch imports[1] = {};
    CodeFunctionPatch setWindowTextPatch = {};
    CodeFunctionPatch patches[2] = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndGetWindowTextACStringOrdinal,
                              (void *)&FakeSubmitVisibleGetWindowTextA,
                              imports[0]) ||
        !PatchFunctionJump(CWndSetWindowTextAAddress(),
                           (void *)&FakeQueryStatusSetWindowTextA,
                           setWindowTextPatch) ||
        !PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeSubmitVisibleFormatMessage,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeSubmitVisibleAppendStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreImportPatch(imports[0]);
        return 1;
    }

    HWND listBox = CreateWindowExA(0,
                                   "LISTBOX",
                                   "",
                                   WS_POPUP | LBS_MULTIPLESEL,
                                   0,
                                   0,
                                   200,
                                   10,
                                   0,
                                   0,
                                   GetModuleHandleA(0),
                                   0);
    if (listBox == 0)
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreImportPatch(imports[0]);
        return 2;
    }

    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Alpha One");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Beta Two");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Gamma");

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldVisibleSessionResultCount =
        g_WestwoodOnlineUpgradeVisibleSessionResultCount;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.SubmitStatusText = FakeSubmitVisibleSubmitStatusText;
    g_initFakeApiVtable.SubmitSessionRequestListAndStatusText =
        FakeSubmitVisibleSubmitSessionRequestListAndStatusText;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionResultsList.m_hWnd = listBox;
    dialog.m_statusServerEdit.m_hWnd = (HWND)0x1234;
    new (&dialog.m_selectedProfilePlayerName) CString();
    dialog.m_selectedProfilePlayerName = "Player";

    int failure = 0;
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    strcpy(g_submitVisibleStatusInput, "Ignored");
    g_submitVisibleGetWindowTextCalls = 0;
    g_WestwoodOnlineUpgradeVisibleSessionResultCount = 77;
    dialog.SubmitVisibleSessionRequestsAndStatusText();
    if (g_submitVisibleGetWindowTextCalls != 0 ||
        g_WestwoodOnlineUpgradeVisibleSessionResultCount != 77)
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = 'S';
    strcpy(g_submitVisibleStatusInput, "");
    g_submitVisibleGetWindowTextCalls = 0;
    g_queryStatusSetWindowTextCalls = 0;
    g_submitVisibleSubmitStatusCalls = 0;
    g_submitVisibleSubmitListCalls = 0;
    g_submitVisibleAppendCalls = 0;
    dialog.SubmitVisibleSessionRequestsAndStatusText();
    if (failure == 0 &&
        (g_submitVisibleGetWindowTextCalls != 1 ||
         g_submitVisibleGetWindowTextThis != &dialog.m_statusServerEdit ||
         g_queryStatusSetWindowTextCalls != 0 ||
         g_submitVisibleSubmitStatusCalls != 0 ||
         g_submitVisibleSubmitListCalls != 0 ||
         g_submitVisibleAppendCalls != 0))
    {
        failure = 4;
    }

    strcpy(g_submitVisibleStatusInput, "Status text");
    SendMessageA(listBox, LB_SETSEL, FALSE, -1);
    g_queryStatusSetWindowTextCalls = 0;
    g_submitVisibleSubmitStatusCalls = 0;
    g_submitVisibleSubmitListCalls = 0;
    g_submitVisibleAppendCalls = 0;
    dialog.SubmitVisibleSessionRequestsAndStatusText();
    if (failure == 0 &&
        (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 0 ||
         g_queryStatusSetWindowTextCalls != 1 ||
         strcmp(g_queryStatusSetWindowTextValue[0], "") != 0 ||
         g_submitVisibleSubmitStatusCalls != 1 ||
         g_submitVisibleSubmitStatusSelf != (IUnknown *)&g_initFakeApi ||
         strcmp(g_submitVisibleSubmitStatusText, "Status text") != 0 ||
         g_submitVisibleSubmitListCalls != 0 ||
         g_submitVisibleAppendCalls != 1 ||
         strcmp(g_submitVisibleAppendFormat, "{ %s } %s") != 0 ||
         strcmp(g_submitVisibleAppendArg0, "Player") != 0 ||
         strcmp(g_submitVisibleAppendArg1, "Status text") != 0))
    {
        failure = 5;
    }

    SendMessageA(listBox, LB_SETSEL, TRUE, 0);
    SendMessageA(listBox, LB_SETSEL, TRUE, 1);
    SendMessageA(listBox, LB_SETSEL, TRUE, 2);
    strcpy(g_submitVisibleStatusInput, "Batch status");
    g_submitVisibleFormatCalls = 0;
    g_submitVisibleAppendCalls = 0;
    g_submitVisibleSubmitStatusCalls = 0;
    g_submitVisibleSubmitListCalls = 0;
    memset(g_submitVisibleSubmitListNames,
           0,
           sizeof(g_submitVisibleSubmitListNames));
    dialog.SubmitVisibleSessionRequestsAndStatusText();
    if (failure == 0 &&
        (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 3 ||
         g_submitVisibleFormatCalls != 1 ||
         g_submitVisibleFormatMaxChars != 128 ||
         g_submitVisibleFormatMessageId != 0x3038 ||
         strcmp(g_submitVisibleFormatSessionName, "Gamma") != 0 ||
         strcmp(g_submitVisibleFormatStatusText, "Batch status") != 0 ||
         g_submitVisibleAppendCalls != 1 ||
         strcmp(g_submitVisibleAppendFormat, "formatted visible status") != 0 ||
         g_submitVisibleSubmitStatusCalls != 0 ||
         g_submitVisibleSubmitListCalls != 1 ||
         g_submitVisibleSubmitListSelf != (IUnknown *)&g_initFakeApi ||
         g_submitVisibleSubmitListCount != 3 ||
         strcmp(g_submitVisibleSubmitListStatusText, "Batch status") != 0 ||
         strcmp(g_submitVisibleSubmitListNames[0], "Gamma") != 0 ||
         strcmp(g_submitVisibleSubmitListNames[1], "Beta") != 0 ||
         strcmp(g_submitVisibleSubmitListNames[2], "Alpha") != 0))
    {
        failure = 6;
    }

    dialog.m_selectedProfilePlayerName.~CString();
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        oldVisibleSessionResultCount;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    DestroyWindow(listBox);
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    RestoreFunctionPatch(setWindowTextPatch);
    RestoreImportPatch(imports[0]);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_submit_pending_session_list_smoke(void)
{
    HWND listBox = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_NOTIFY, 0, 0, 200,
                                  10, 0, 0, GetModuleHandleA(0), 0);
    if (listBox == 0)
    {
        return 1;
    }

    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Alpha One");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Beta Two");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"Gamma");

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    const int oldCreateSessionFromQueryFlag =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldPendingSessionResultCount =
        g_WestwoodOnlineUpgradePendingSessionResultCount;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.SubmitPendingSessionList = FakeSubmitPendingSessionList;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionResultsList.m_hWnd = listBox;

    int failure = 0;
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 3;
    g_submitPendingSessionListCalls = 0;
    dialog.SubmitPendingSessionListFromResults();
    if (g_submitPendingSessionListCalls != 0)
    {
        failure = 2;
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = 'S';
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    dialog.SubmitPendingSessionListFromResults();
    if (failure == 0 && g_submitPendingSessionListCalls != 0)
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 0;
    g_submitPendingSessionListCalls = 0;
    g_submitPendingSessionListCount = -1;
    dialog.SubmitPendingSessionListFromResults();
    if (failure == 0 &&
        (g_submitPendingSessionListCalls != 1 ||
         g_submitPendingSessionListSelf != (IUnknown *)&g_initFakeApi ||
         g_submitPendingSessionListCount != 0))
    {
        failure = 4;
    }

    g_WestwoodOnlineUpgradePendingSessionResultCount = 4;
    g_submitPendingSessionListCalls = 0;
    memset(g_submitPendingSessionListNames,
           0,
           sizeof(g_submitPendingSessionListNames));
    dialog.SubmitPendingSessionListFromResults();
    if (failure == 0 &&
        (g_submitPendingSessionListCalls != 1 ||
         g_submitPendingSessionListCount != 3 ||
         strcmp(g_submitPendingSessionListNames[0], "Gamma") != 0 ||
         strcmp(g_submitPendingSessionListNames[1], "Beta") != 0 ||
         strcmp(g_submitPendingSessionListNames[2], "Alpha") != 0))
    {
        failure = 5;
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag =
        oldCreateSessionFromQueryFlag;
    g_WestwoodOnlineUpgradePendingSessionResultCount =
        oldPendingSessionResultCount;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    DestroyWindow(listBox);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_max_players_edit_change_smoke(void)
{
    CodeFunctionPatch updateDataPatch = {};
    if (!PatchFunctionJump(CWndUpdateDataAddress(), (void *)&FakeWestwoodUpdateData,
                           updateDataPatch))
    {
        return 1;
    }

    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatUpdateDataSaveValue[0] = -1;

    dialog.OnMaxPlayersEditChange();
    int const failure =
        g_threeFloatUpdateDataCount == 1 && g_threeFloatUpdateDataSaveValue[0] == 1
            ? 0
            : 1;

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(updateDataPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_max_players_edit_kill_focus_smoke(void)
{
    CodeFunctionPatch setWindowTextPatch = {};
    if (!PatchFunctionJump(CWndSetWindowTextAAddress(),
                           (void *)&FakeQueryStatusSetWindowTextA,
                           setWindowTextPatch))
    {
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_queryMaxPlayersEdit.m_hWnd = (HWND)0x12340006;

    dialog.m_queryMaxPlayers = 1;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnMaxPlayersEditKillFocus();
    int failure = 0;
    if (g_queryStatusSetWindowTextCalls != 1 ||
        g_queryStatusSetWindowTextThis[0] !=
            (void *)dialog.m_queryMaxPlayersEdit.m_hWnd ||
        strcmp(g_queryStatusSetWindowTextValue[0], "2") != 0 ||
        dialog.m_queryMaxPlayers != 2)
    {
        failure = 2;
    }

    dialog.m_queryMaxPlayers = 5;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnMaxPlayersEditKillFocus();
    if (failure == 0 &&
        (g_queryStatusSetWindowTextCalls != 1 ||
         strcmp(g_queryStatusSetWindowTextValue[0], "4") != 0 ||
         dialog.m_queryMaxPlayers != 4))
    {
        failure = 3;
    }

    dialog.m_queryMaxPlayers = 3;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnMaxPlayersEditKillFocus();
    if (failure == 0 &&
        (g_queryStatusSetWindowTextCalls != 1 ||
         strcmp(g_queryStatusSetWindowTextValue[0], "3") != 0 ||
         dialog.m_queryMaxPlayers != 3))
    {
        failure = 4;
    }

    RestoreFunctionPatch(setWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_aux_param_edit_kill_focus_smoke(void)
{
    CodeFunctionPatch setWindowTextPatch = {};
    if (!PatchFunctionJump(CWndSetWindowTextAAddress(),
                           (void *)&FakeQueryStatusSetWindowTextA,
                           setWindowTextPatch))
    {
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_queryAuxParamEdit.m_hWnd = (HWND)0x12340007;

    dialog.m_queryAuxParam = 0;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnAuxParamEditKillFocus();
    int failure = 0;
    if (g_queryStatusSetWindowTextCalls != 1 ||
        g_queryStatusSetWindowTextThis[0] !=
            (void *)dialog.m_queryAuxParamEdit.m_hWnd ||
        strcmp(g_queryStatusSetWindowTextValue[0], "1") != 0 ||
        dialog.m_queryAuxParam != 1)
    {
        failure = 2;
    }

    dialog.m_queryAuxParam = 1001;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnAuxParamEditKillFocus();
    if (failure == 0 &&
        (g_queryStatusSetWindowTextCalls != 1 ||
         strcmp(g_queryStatusSetWindowTextValue[0], "1000") != 0 ||
         dialog.m_queryAuxParam != 1000))
    {
        failure = 3;
    }

    dialog.m_queryAuxParam = 27;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnAuxParamEditKillFocus();
    if (failure == 0 &&
        (g_queryStatusSetWindowTextCalls != 1 ||
         strcmp(g_queryStatusSetWindowTextValue[0], "27") != 0 ||
         dialog.m_queryAuxParam != 27))
    {
        failure = 4;
    }

    RestoreFunctionPatch(setWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_value_or_time_edit_kill_focus_smoke(void)
{
    CodeFunctionPatch setWindowTextPatch = {};
    if (!PatchFunctionJump(CWndSetWindowTextAAddress(),
                           (void *)&FakeQueryStatusSetWindowTextA,
                           setWindowTextPatch))
    {
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_queryValueOrTimeEdit.m_hWnd = (HWND)0x12340008;

    dialog.m_queryValueOrTime = 1;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnValueOrTimeEditKillFocus();
    int failure = 0;
    if (g_queryStatusSetWindowTextCalls != 1 ||
        g_queryStatusSetWindowTextThis[0] !=
            (void *)dialog.m_queryValueOrTimeEdit.m_hWnd ||
        strcmp(g_queryStatusSetWindowTextValue[0], "2") != 0 ||
        dialog.m_queryValueOrTime != 2)
    {
        failure = 2;
    }

    dialog.m_queryValueOrTime = 2001;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnValueOrTimeEditKillFocus();
    if (failure == 0 &&
        (g_queryStatusSetWindowTextCalls != 1 ||
         strcmp(g_queryStatusSetWindowTextValue[0], "2000") != 0 ||
         dialog.m_queryValueOrTime != 2000))
    {
        failure = 3;
    }

    dialog.m_queryValueOrTime = 45;
    g_queryStatusSetWindowTextCalls = 0;
    dialog.OnValueOrTimeEditKillFocus();
    if (failure == 0 &&
        (g_queryStatusSetWindowTextCalls != 1 ||
         strcmp(g_queryStatusSetWindowTextValue[0], "45") != 0 ||
         dialog.m_queryValueOrTime != 45))
    {
        failure = 4;
    }

    RestoreFunctionPatch(setWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_clear_status_list_smoke(void)
{
    HWND listBox = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_NOTIFY, 0, 0, 200,
                                  10, 0, 0, GetModuleHandleA(0), 0);
    if (listBox == 0)
    {
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog *const dialog =
        (WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog->m_statusList.m_hWnd = listBox;
    dialog->m_statusLineCount = 2;
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"first");
    SendMessageA(listBox, LB_ADDSTRING, 0, (LPARAM)"second");

    dialog->ClearStatusList();
    int const failure =
        dialog->m_statusLineCount == 0 &&
                SendMessageA(listBox, LB_GETCOUNT, 0, 0) == 0
            ? 0
            : 2;

    DestroyWindow(listBox);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_set_selected_profile_values_smoke(void)
{
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    memset(&dialog, 0x5a, sizeof(dialog));
    dialog.Constructor(0);

    CString playerName("Pilot One");
    CString connectString("connect-one");
    dialog.SetSelectedProfilePlayerName(playerName);
    dialog.SetSelectedProfileConnectString(connectString);

    CString copiedPlayerName;
    CString copiedConnectString;
    CString *const playerNameResult = dialog.GetSelectedProfilePlayerName(&copiedPlayerName);
    CString *const connectStringResult =
        dialog.GetSelectedProfileConnectString(&copiedConnectString);

    int failure = 0;
    if (strcmp((const char *)dialog.m_selectedProfilePlayerName, "Pilot One") != 0 ||
        strcmp((const char *)dialog.m_selectedProfileConnectString, "connect-one") != 0)
    {
        failure = 1;
    }
    else if (playerNameResult != &copiedPlayerName ||
             connectStringResult != &copiedConnectString ||
             strcmp((const char *)copiedPlayerName, "Pilot One") != 0 ||
             strcmp((const char *)copiedConnectString, "connect-one") != 0)
    {
        failure = 2;
    }

    return failure;
}

extern "C" int westwood_online_upgrade_dialog_enable_controls_smoke(void)
{
    const WORD kMfc42CWndEnableWindowOrdinal = 2642;
    ImportFunctionPatch enablePatch = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndEnableWindowOrdinal,
                              (void *)&FakeWolEnableWindow, enablePatch))
    {
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_enableWindowCalls = 0;
    dialog.EnableQueryControls(0);
    int failure = 0;
    if (g_enableWindowCalls != 7 ||
        g_enableWindowThis[0] != &dialog.m_sessionModeCombo ||
        g_enableWindowThis[1] != &dialog.m_queryStatusFlag0Check ||
        g_enableWindowThis[2] != &dialog.m_queryStatusFlag1Check ||
        g_enableWindowThis[3] != &dialog.m_queryAuxParamEdit ||
        g_enableWindowThis[4] != &dialog.m_queryValueOrTimeEdit ||
        g_enableWindowThis[5] != &dialog.m_submitPendingSessionListButton ||
        g_enableWindowThis[6] != &dialog.m_connectButton)
    {
        failure = 2;
    }
    else
    {
        for (int index = 0; index < 7; ++index)
        {
            if (g_enableWindowEnable[index] != 0)
            {
                failure = 3;
                break;
            }
        }
    }

    g_enableWindowCalls = 0;
    dialog.EnableConnectButton(1);
    if (failure == 0 &&
        (g_enableWindowCalls != 1 ||
         g_enableWindowThis[0] != &dialog.m_connectButton ||
         g_enableWindowEnable[0] != 1))
    {
        failure = 4;
    }

    RestoreImportPatch(enablePatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_reset_selected_browse_record_smoke(void)
{
    const WORD kMfc42CWndEnableWindowOrdinal = 2642;
    ImportFunctionPatch enablePatch = {};
    ImportFunctionPatch sendMessagePatch = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CWndEnableWindowOrdinal,
                              (void *)&FakeWolEnableWindow, enablePatch) ||
        !PatchImportByName("USER32.dll", "SendMessageA",
                           (void *)&FakeResetSendMessageA, sendMessagePatch))
    {
        RestoreImportPatch(sendMessagePatch);
        RestoreImportPatch(enablePatch);
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    const int oldCreateSessionFromQueryFlag =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    WestwoodOnlineUpgradeBrowseRecord const oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionResultsList.m_hWnd = (HWND)0x12345678;
    g_initFakeApiVtable.QueryInterface = 0;
    g_initFakeApiVtable.AddRef = 0;
    g_initFakeApiVtable.Release = 0;
    g_initFakeApiVtable.ProcessCallbacks = 0;
    g_initFakeApiVtable.BeginConnect = 0;
    g_initFakeApiVtable.RequestBootstrapServerList = 0;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_initFakeApiVtable.SubmitQueryRequest = 0;
    g_initFakeApiVtable.LoadBrowseRecord = 0;
    g_initFakeApiVtable.ResetQueryState = FakeResetQueryState;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 55;
    g_WestwoodOnlineUpgradeActiveListMode = 11;
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord,
           0x5a,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = 'S';
    g_resetQueryStateCalls = 0;
    g_resetSendMessageCalls = 0;
    g_enableWindowCalls = 0;
    g_initRequestListModeCalls = 0;
    g_initRequestListMode = 0;
    g_initRequestListModeEnabled = 0;

    dialog.ResetSelectedBrowseRecordAndRefreshList();
    int failure = 0;
    if (g_resetQueryStateCalls != 1 ||
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0')
    {
        failure = 2;
    }
    else if (g_resetSendMessageCalls != 1 ||
             g_resetSendMessageHwnd != (HWND)0x12345678 ||
             g_resetSendMessageMsg != LB_RESETCONTENT ||
             g_resetSendMessageWParam != 0 ||
             g_resetSendMessageLParam != 0)
    {
        failure = 3;
    }
    else if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0 ||
             g_enableWindowCalls != 10)
    {
        failure = 4;
    }
    else if (g_enableWindowThis[0] != &dialog.m_sessionModeCombo ||
             g_enableWindowThis[7] != &dialog.m_connectButton ||
             g_enableWindowThis[8] != &dialog.m_querySessionsByNameButton ||
             g_enableWindowThis[9] != &dialog.m_queueVisibleSessionRequestsButton)
    {
        failure = 5;
    }
    else
    {
        for (int index = 0; index < 10; ++index)
        {
            if (g_enableWindowEnable[index] != 0)
            {
                failure = 6;
                break;
            }
        }
    }

    if (failure == 0 &&
        (g_initRequestListModeCalls != 1 ||
         g_initRequestListMode != 11 ||
         g_initRequestListModeEnabled != 1))
    {
        failure = 7;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateSessionFromQueryFlag;
    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    RestoreImportPatch(sendMessagePatch);
    RestoreImportPatch(enablePatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_append_connect_status_smoke(void)
{
    CodeFunctionPatch patches[3] = {};
    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeAppendConnectFormatMessage, patches[0]) ||
        !PatchFunctionJump(MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
                           (void *)&FakeAppendConnectAppendStatusTextFmt, patches[1]) ||
        !PatchFunctionJump(
            MethodAddress(
                &WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList),
            (void *)&FakeAppendConnectResetSelectedBrowseRecord, patches[2]))
    {
        for (int index = 2; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        return 1;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_appendConnectFormatCalls = 0;
    g_appendConnectFormatBuffer = 0;
    g_appendConnectFormatMaxChars = 0;
    g_appendConnectFormatMessageId = 0;
    g_appendConnectFormatSessionName = 0;
    g_appendConnectAppendCalls = 0;
    g_appendConnectAppendThis = 0;
    g_appendConnectAppendText = 0;
    g_appendConnectResetCalls = 0;
    g_appendConnectResetThis = 0;

    dialog.AppendConnectStatusAndRefreshList("SessionAlpha");
    int failure = 0;
    if (g_appendConnectFormatCalls != 1 ||
        g_appendConnectFormatBuffer == 0 ||
        g_appendConnectFormatMaxChars != 128 ||
        g_appendConnectFormatMessageId != 0x3036 ||
        strcmp(g_appendConnectFormatSessionName, "SessionAlpha") != 0)
    {
        failure = 2;
    }
    else if (g_appendConnectAppendCalls != 1 ||
             g_appendConnectAppendThis != &dialog ||
             strcmp(g_appendConnectAppendText, "formatted connect status") != 0 ||
             g_appendConnectAppendText != g_appendConnectFormatBuffer)
    {
        failure = 3;
    }
    else if (g_appendConnectResetCalls != 1 ||
             g_appendConnectResetThis != &dialog)
    {
        failure = 4;
    }

    for (int index = 2; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_set_abort_and_close_smoke(void)
{
    const WORD kMfc42CDialogOnCancelOrdinal = 4376;
    ImportFunctionPatch cancelPatch = {};
    if (!PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogOnCancelOrdinal,
                              (void *)&FakeAbortOnCancel, cancelPatch))
    {
        return 1;
    }

    const int oldAbortFlag = g_WestwoodOnlineUpgradeAbortFlag;
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_WestwoodOnlineUpgradeAbortFlag = 0;
    g_abortOnCancelCalls = 0;
    g_abortOnCancelThis = 0;

    dialog.SetAbortAndClose();
    const int failure =
        g_WestwoodOnlineUpgradeAbortFlag == 1 &&
                g_abortOnCancelCalls == 1 &&
                g_abortOnCancelThis == (CDialog *)&dialog
            ? 0
            : 2;

    g_WestwoodOnlineUpgradeAbortFlag = oldAbortFlag;
    RestoreImportPatch(cancelPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_ref_count_and_lock_init_smoke(void)
{
    WestwoodOnlineUpgradeRefCountAndLock refCountAndLock = {};
    refCountAndLock.refCount = 17;
    WestwoodOnlineUpgradeRefCountAndLock *const result = refCountAndLock.Init();
    int failure = 0;
    if (result != &refCountAndLock || refCountAndLock.refCount != 0)
    {
        failure = 1;
    }
    else if (TryEnterCriticalSection(&refCountAndLock.lock) == 0)
    {
        failure = 2;
    }
    else
    {
        LeaveCriticalSection(&refCountAndLock.lock);
    }

    DeleteCriticalSection(&refCountAndLock.lock);
    return failure;
}
