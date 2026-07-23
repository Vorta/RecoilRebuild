// Tests-only WOL API event-sink smokes ported from westwood_online_upgrade_tests.cpp.
// Keep this file focused on the compiled functional smoke registrations.

#include "Battlesport/wol_config_dialog.h"
#include "Battlesport/cz_recoil_frame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/wol_api.h"
#include "Battlesport/wol_api_event_sink.h"
#include "Battlesport/wol_dialog.h"
#include "Battlesport/wol_ref_count_and_lock.h"
#include "GameZRecoil/Time/time.h"
#include "Battlesport/wol_download.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zNetwork/znet.h"

#include <ocidl.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#define WOL_API_EVENT_SINK_TEST_WRAPPER(name, params, forwarded) \
    int __stdcall WestwoodOnlineUpgradeApiEventSink::name params \
    { \
        return ((WestwoodOnlineUpgradeApiEventSink *)callbackContext) \
            ->WestwoodOnlineUpgradeApiEventSink::name forwarded; \
    }

WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnDownloadReadyResult,
    (void *callbackContext, int resultCode,
     WestwoodOnlineUpgradeDownloadReadyEntry *downloadReadyList),
    (resultCode, downloadReadyList))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnPendingSessionRequestRemoved,
    (void *callbackContext, int status, WestwoodOnlineUpgradeSessionRequest *sessionRequest),
    (status, sessionRequest))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnBootstrapServerList,
    (void *callbackContext, int resultCode,
     WestwoodOnlineUpgradeBootstrapServerRecord *serverList),
    (resultCode, serverList))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnServerError,
    (void *callbackContext, int status, const char *errorText),
    (status, errorText))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnApiStatus,
    (void *callbackContext, int statusCode, const char *statusText),
    (statusCode, statusText))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnStatusTextReceived,
    (void *callbackContext, int status, const char *statusText),
    (status, statusText))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnBrowseRecordAdded,
    (void *callbackContext, int status, WestwoodOnlineUpgradeBrowseRecord *browseRecord),
    (status, browseRecord))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnBrowseRecordAndSessionResolved,
    (void *callbackContext, int status, WestwoodOnlineUpgradeBrowseRecord *browseRecord,
     WestwoodOnlineUpgradeSessionRequest *sessionRequest),
    (status, browseRecord, sessionRequest))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnSessionQueryFinished,
    (void *callbackContext, int status, WestwoodOnlineUpgradeBrowseRecord *browseRecord,
     WestwoodOnlineUpgradeSessionRequest *sessionRequest),
    (status, browseRecord, sessionRequest))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnSessionLaunchResult,
    (void *callbackContext, int status, WestwoodOnlineUpgradeBrowseRecord *browseRecord,
     WestwoodOnlineUpgradeSessionRequest *sessionNode,
     WestwoodOnlineUpgradeSessionRequest *sessionRequest),
    (status, browseRecord, sessionNode, sessionRequest))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnSessionListEnumerated,
    (void *callbackContext, int status, WestwoodOnlineUpgradeBrowseRecord *browseRecord,
     WestwoodOnlineUpgradeSessionRequest *sessionList),
    (status, browseRecord, sessionList))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    LaunchSelectedSession,
    (void *callbackContext, int status, int reserved,
     WestwoodOnlineUpgradeSessionRequest *selectedSessionList, int reserved2),
    (status, reserved, selectedSessionList, reserved2))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    ApplyEncodedQueryString0,
    (void *callbackContext, int status, int reserved, char *encodedQuery),
    (status, reserved, encodedQuery))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    ApplyEncodedQueryString1,
    (void *callbackContext, int status, int reserved, int reserved2, char *encodedQuery),
    (status, reserved, reserved2, encodedQuery))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendSessionRequestStatus301B,
    (void *callbackContext, int status, int reserved,
     WestwoodOnlineUpgradeSessionRequest *sessionRequest, const char *statusText),
    (status, reserved, sessionRequest, statusText))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendSessionRequestStatus301C,
    (void *callbackContext, int status, WestwoodOnlineUpgradeSessionRequest *sessionRequest,
     const char *statusText),
    (status, sessionRequest, statusText))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendSessionRequestStatus301C_Alt0,
    (void *callbackContext, int status, WestwoodOnlineUpgradeSessionRequest *sessionRequest,
     int value),
    (status, sessionRequest, value))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendSessionRequestStatus301C_Alt1,
    (void *callbackContext, int status, int reserved,
     WestwoodOnlineUpgradeSessionRequest *sessionRequest, int value),
    (status, reserved, sessionRequest, value))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    CallbackNoOp0,
    (void *callbackContext, int reserved0, int reserved1, int reserved2),
    (reserved0, reserved1, reserved2))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    CallbackNoOp1,
    (void *callbackContext, int reserved0, int reserved1),
    (reserved0, reserved1))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendTimeStatus302A,
    (void *callbackContext, int status, long unixTime),
    (status, unixTime))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendValueStatus302B_302C,
    (void *callbackContext, int reserved, int value, int usePrimaryMessage),
    (reserved, value, usePrimaryMessage))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    UpdateSessionResultItemFlags,
    (void *callbackContext, int status, const char *sessionName, int flags, int reserved),
    (status, sessionName, flags, reserved))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendSessionRequestStatus301D,
    (void *callbackContext, int status, WestwoodOnlineUpgradeSessionRequest *sessionRequest,
     const char *statusText),
    (status, sessionRequest, statusText))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendConnectStatus301E_3021,
    (void *callbackContext, int connectionStatusCode),
    (connectionStatusCode))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendBrowseRecordStatus3022_3025,
    (void *callbackContext, int status, WestwoodOnlineUpgradeBrowseRecord *browseRecord),
    (status, browseRecord))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    AppendValueStatus3026,
    (void *callbackContext, int status, int value),
    (status, value))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnNetworkStatusChanged,
    (void *callbackContext, int connectionStatusCode),
    (connectionStatusCode))
WOL_API_EVENT_SINK_TEST_WRAPPER(
    OnBrowseRecordListReceived,
    (void *callbackContext, int status, WestwoodOnlineUpgradeBrowseRecord *browseRecordList),
    (status, browseRecordList))

#undef WOL_API_EVENT_SINK_TEST_WRAPPER

extern int g_threeFloatDefaultCount;
extern long g_threeFloatDefaultReturn;
extern int g_threeFloatUpdateDataCount;
extern int g_threeFloatUpdateDataSaveValue[8];
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" HWND g_RecoilApp_hWndMain;

namespace
{
void *TestObjectVtable(void *object)
{
    return *(void **)object;
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

struct ApiEventSinkSmokeVtable
{
    void *slots[3];
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
    void *reserved084[3];
    int(STDMETHODCALLTYPE *LoadConnectProfileStrings)(
        IUnknown *self,
        int profileId,
        char **playerNameOut,
        char **connectStringOut);
    int(STDMETHODCALLTYPE *SaveConnectProfileStrings)(
        IUnknown *self,
        int profileId,
        const char *playerName,
        const char *connectString,
        int connectStringMode);
    void(STDMETHODCALLTYPE *GetQueryResultCount)(IUnknown *self, int *outCount);
};

struct InitFakeApiObject
{
    InitFakeApiVtable *vftable;
};

struct InitFakeProgressVtable
{
    void *reserved000[24];
    int(__fastcall *DestroyWindow)(CWnd *self, void *edx);
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
int g_configFocusSendMessageCalls;
HWND g_configFocusSendMessageHwnd;
UINT g_configFocusSendMessageMsg;
WPARAM g_configFocusSendMessageWParam;
LPARAM g_configFocusSendMessageLParam;
int g_configInitLoadProfileCalls;
int g_configInitLoadProfileIds[4];
int g_configInitLoadProfileResults[4];
const char *g_configInitLoadProfilePlayers[4];
const char *g_configInitLoadProfileConnectStrings[4];
int g_configInitSetWindowTextCalls;
CWnd *g_configInitSetWindowTextThis;
const char *g_configInitSetWindowTextValue;
int g_configOnOkSaveProfileCalls;
int g_configOnOkSaveProfileIds[4];
const char *g_configOnOkSaveProfilePlayers[4];
const char *g_configOnOkSaveProfileConnectStrings[4];
int g_configOnOkSaveProfileModes[4];
int g_configOnOkBaseOnOkCalls;
CDialog *g_configOnOkBaseOnOkThis;
int g_configComboKillFocusGetWindowTextCalls;
CWnd *g_configComboKillFocusGetWindowTextThis;
const char *g_configComboKillFocusText;
int g_configSelChangeSendMessageCalls;
HWND g_configSelChangeSendMessageHwnd;
UINT g_configSelChangeSendMessageMsg;
WPARAM g_configSelChangeSendMessageWParam;
LPARAM g_configSelChangeSendMessageLParam;
LRESULT g_configSelChangeSendMessageResult;
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
int g_appendStatus301BFormatCalls;
char *g_appendStatus301BFormatBuffer;
int g_appendStatus301BFormatMaxChars;
unsigned int g_appendStatus301BFormatMessageId;
const char *g_appendStatus301BFormatSessionName;
const char *g_appendStatus301BFormatStatusText;
int g_appendStatus301BAppendCalls;
WestwoodOnlineUpgradeDialog *g_appendStatus301BAppendThis;
char g_appendStatus301BAppendText[64];
int g_appendStatus301CFormatCalls;
char *g_appendStatus301CFormatBuffer;
int g_appendStatus301CFormatMaxChars;
unsigned int g_appendStatus301CFormatMessageId;
const char *g_appendStatus301CFormatSessionName;
const char *g_appendStatus301CFormatStatusText;
int g_appendStatus301CAppendCalls;
WestwoodOnlineUpgradeDialog *g_appendStatus301CAppendThis;
char g_appendStatus301CAppendText[64];
int g_appendStatus301CAltAppendCalls;
WestwoodOnlineUpgradeDialog *g_appendStatus301CAltAppendThis;
const char *g_appendStatus301CAltAppendFormat;
const char *g_appendStatus301CAltAppendSessionName;
int g_appendStatus301CAltAppendValue;
int g_appendStatus301DFormatCalls;
char *g_appendStatus301DFormatBuffer;
int g_appendStatus301DFormatMaxChars;
unsigned int g_appendStatus301DFormatMessageId;
const char *g_appendStatus301DFormatSessionName;
const char *g_appendStatus301DFormatStatusText;
int g_appendStatus301DAppendCalls;
WestwoodOnlineUpgradeDialog *g_appendStatus301DAppendThis;
char g_appendStatus301DAppendText[64];
int g_appendConnectStatusCalls;
WestwoodOnlineUpgradeDialog *g_appendConnectStatusThis[8];
char g_appendConnectStatusText[8][64];
int g_appendBrowseRecordStatusFormatCalls;
char *g_appendBrowseRecordStatusFormatBuffer;
int g_appendBrowseRecordStatusFormatMaxChars;
unsigned int g_appendBrowseRecordStatusFormatMessageId;
const char *g_appendBrowseRecordStatusFormatSessionName;
int g_appendValueStatusFormatCalls;
char *g_appendValueStatusFormatBuffer;
int g_appendValueStatusFormatMaxChars;
unsigned int g_appendValueStatusFormatMessageId;
int g_appendValueStatusFormatValue;
int g_appendTimeStatusFormatCalls;
char *g_appendTimeStatusFormatBuffer;
int g_appendTimeStatusFormatMaxChars;
unsigned int g_appendTimeStatusFormatMessageId;
char g_appendTimeStatusFormatTimeText[64];
int g_browseRecordListFormatCalls;
char *g_browseRecordListFormatBuffer[12];
int g_browseRecordListFormatMaxChars[12];
unsigned int g_browseRecordListFormatMessageId[12];
const char *g_browseRecordListFormatSessionName[12];
int g_browseRecordListFormatMetric0[12];
int g_browseRecordListFormatMetric1[12];
const char *g_browseRecordListFormatLatencyText[12];
int g_networkStatusReturnOnlyCalls;
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
int g_downloadDlgSendDlgItemMessageCalls;
HWND g_downloadDlgSendDlgItemMessageHwnd[4];
int g_downloadDlgSendDlgItemMessageControlId[4];
UINT g_downloadDlgSendDlgItemMessageMessage[4];
WPARAM g_downloadDlgSendDlgItemMessageWParam[4];
LPARAM g_downloadDlgSendDlgItemMessageLParam[4];
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
int g_sessionLaunchFormatCalls;
char *g_sessionLaunchFormatBuffer;
int g_sessionLaunchFormatMaxChars;
unsigned int g_sessionLaunchFormatMessageId;
const char *g_sessionLaunchFormatArg0;
const char *g_sessionLaunchFormatArg1;
const char *g_sessionLaunchFormatArg2;
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
int g_launchInitSessionCalls;
GUID *g_launchInitSessionGuid;
int g_launchFormatIpv4Calls;
unsigned int g_launchFormatIpv4Packed;
char g_launchFormattedHost[20];
int g_launchSelectTcpCalls;
char g_launchSelectTcpAddress[20];
int g_launchSelectTcpSkip;
int g_launchSelectTcpResult;
int g_launchCreateSessionCalls;
zNetworkSessionDescStatusFields g_launchCreateSessionFields;
int g_launchCreateSessionResult;
int g_launchSetNetworkEnabledCalls;
int g_launchSetNetworkEnabledValue;
int g_launchGetPlayerNameCalls;
WestwoodOnlineUpgradeDialog *g_launchGetPlayerNameThis[4];
char g_launchPlayerName[64];
int g_launchCreateLocalPlayerCalls;
char g_launchCreateLocalPlayerName[4][64];
int g_launchOpenSelectedCalls;
zNetworkSessionDescStatusFields g_launchOpenSelectedInputFields;
zNetworkSessionDescStatusFields g_launchOpenSelectedOutputFields;
int g_launchOpenSelectedResult;
int g_launchSetPlayerNameCalls;
char g_launchSetPlayerName[64];
int g_launchSetStatusBitsCalls;
unsigned int g_launchStatusBits;
int g_launchTimerCalls;
HudSensorTracker *g_launchTimerThis;
int g_launchTimerSecondsRaw;
int g_launchTimerGoalValue;
int g_launchSendMessageCalls;
HWND g_launchSendMessageHwnd;
UINT g_launchSendMessageMsg;
WPARAM g_launchSendMessageWParam;
LPARAM g_launchSendMessageLParam;
LRESULT g_launchSendMessageResult;
int g_apiEventSinkAddRefCalls;
IUnknown *g_apiEventSinkAddRefSelf;

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
        IsEqualGUID(iid, IID_WestwoodOnlineUpgradeDownloadEventSink)
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
        IsEqualGUID(rclsid, g_CLSID_WestwoodOnlineUpgradeDownload) != 0 &&
        outer == 0 &&
        clsContext == CLSCTX_INPROC_SERVER &&
        IsEqualGUID(riid, g_IID_WestwoodOnlineUpgradeDownload) != 0 &&
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

LRESULT WINAPI FakeDownloadDlgSendDlgItemMessageA(
    HWND hWnd,
    int controlId,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    const int index = g_downloadDlgSendDlgItemMessageCalls;
    if (index < 4)
    {
        g_downloadDlgSendDlgItemMessageHwnd[index] = hWnd;
        g_downloadDlgSendDlgItemMessageControlId[index] = controlId;
        g_downloadDlgSendDlgItemMessageMessage[index] = message;
        g_downloadDlgSendDlgItemMessageWParam[index] = wParam;
        g_downloadDlgSendDlgItemMessageLParam[index] = lParam;
    }
    ++g_downloadDlgSendDlgItemMessageCalls;
    return 1;
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

unsigned int FakeDownloadReadyFormatMessage(char *outBuffer,
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

int __fastcall FakeDownloadReadyCallbackMessageBoxA(CWnd *self,
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

int __fastcall FakeDownloadReadyCallbackShowList(
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

unsigned int FakePendingSessionRemovedFormatMessage(
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

int FakePendingSessionRemovedAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *format,
    ...)
{
    ++g_pendingRemovedAppendCalls;
    g_pendingRemovedAppendThis = self;
    strcpy(g_pendingRemovedAppendText, format);
    return 1;
}

LRESULT __fastcall FakePendingSessionRemovedSendDlgItemMessageA(
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
             message == LB_FINDSTRING ||
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

int __fastcall FakeServerErrorMessageBoxA(CWnd *self,
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
    g_sessionLaunchFormatCalls = 0;
    g_sessionLaunchFormatBuffer = 0;
    g_sessionLaunchFormatMaxChars = 0;
    g_sessionLaunchFormatMessageId = 0;
    g_sessionLaunchFormatArg0 = 0;
    g_sessionLaunchFormatArg1 = 0;
    g_sessionLaunchFormatArg2 = 0;
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

void ResetLaunchSelectedSessionFakes(void)
{
    g_threeFloatUpdateDataCount = 0;
    for (int index = 0; index < 8; ++index)
    {
        g_threeFloatUpdateDataSaveValue[index] = 0;
    }
    g_launchInitSessionCalls = 0;
    g_launchInitSessionGuid = 0;
    g_launchFormatIpv4Calls = 0;
    g_launchFormatIpv4Packed = 0;
    strcpy(g_launchFormattedHost, "4.3.2.1");
    g_launchSelectTcpCalls = 0;
    g_launchSelectTcpAddress[0] = '\0';
    g_launchSelectTcpSkip = 0;
    g_launchSelectTcpResult = 1;
    g_launchCreateSessionCalls = 0;
    memset(&g_launchCreateSessionFields, 0, sizeof(g_launchCreateSessionFields));
    g_launchCreateSessionResult = 1;
    g_launchSetNetworkEnabledCalls = 0;
    g_launchSetNetworkEnabledValue = 0;
    g_launchGetPlayerNameCalls = 0;
    for (int index2 = 0; index2 < 4; ++index2)
    {
        g_launchGetPlayerNameThis[index2] = 0;
        g_launchCreateLocalPlayerName[index2][0] = '\0';
    }
    strcpy(g_launchPlayerName, "PlayerOne");
    g_launchCreateLocalPlayerCalls = 0;
    g_launchOpenSelectedCalls = 0;
    memset(&g_launchOpenSelectedInputFields, 0,
           sizeof(g_launchOpenSelectedInputFields));
    memset(&g_launchOpenSelectedOutputFields, 0,
           sizeof(g_launchOpenSelectedOutputFields));
    g_launchOpenSelectedResult = 1;
    g_launchSetPlayerNameCalls = 0;
    g_launchSetPlayerName[0] = '\0';
    g_launchSetStatusBitsCalls = 0;
    g_launchStatusBits = 0;
    g_launchTimerCalls = 0;
    g_launchTimerThis = 0;
    g_launchTimerSecondsRaw = 0;
    g_launchTimerGoalValue = 0;
    g_launchSendMessageCalls = 0;
    g_launchSendMessageHwnd = 0;
    g_launchSendMessageMsg = 0;
    g_launchSendMessageWParam = 0;
    g_launchSendMessageLParam = 0;
    g_launchSendMessageResult = 2;
    g_initDisconnectCalls = 0;
}

void FakeApiStatusTimeReset(void)
{
    ++g_apiStatusTimeResetCalls;
}

int FakeApiStatusAppendStatusTextFmt(
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

unsigned int FakeApiStatusFormatMessage(
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

unsigned int FakeBrowseRecordAddedFormatMessage(
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

void __fastcall FakeBrowseRecordAddedEnableQueryControls(
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

int FakeBrowseResolvedAppendStatusTextFmt(
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

unsigned int FakeBrowseResolvedFormatMessage(
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

unsigned int FakeSessionLaunchFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    ...)
{
    va_list args;

    ++g_sessionLaunchFormatCalls;
    g_sessionLaunchFormatBuffer = outBuffer;
    g_sessionLaunchFormatMaxChars = maxChars;
    g_sessionLaunchFormatMessageId = messageId;
    va_start(args, messageId);
    g_sessionLaunchFormatArg0 = va_arg(args, const char *);
    g_sessionLaunchFormatArg1 = va_arg(args, const char *);
    if (messageId == 0x302e)
    {
        g_sessionLaunchFormatArg2 = va_arg(args, const char *);
        wsprintfA(outBuffer,
                  "launch %04x %s %s %s",
                  messageId,
                  g_sessionLaunchFormatArg0,
                  g_sessionLaunchFormatArg1,
                  g_sessionLaunchFormatArg2);
    }
    else
    {
        g_sessionLaunchFormatArg2 = 0;
        wsprintfA(outBuffer,
                  "launch %04x %s %s",
                  messageId,
                  g_sessionLaunchFormatArg0,
                  g_sessionLaunchFormatArg1);
    }
    va_end(args);
    return lstrlenA(outBuffer);
}

void __fastcall FakeBrowseResolvedUpdateSessionListQueryFromControls(
    WestwoodOnlineUpgradeDialog *self,
    void *)
{
    ++g_browseResolvedUpdateCalls;
    g_browseResolvedUpdateThis = self;
}

void __fastcall FakeBrowseResolvedEnableConnectButton(
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

void __fastcall FakeSessionFinishedAppendConnectStatusAndRefreshList(
    WestwoodOnlineUpgradeDialog *self,
    void *,
    const char *sessionName)
{
    ++g_sessionFinishedAppendConnectCalls;
    g_sessionFinishedAppendConnectThis = self;
    g_sessionFinishedAppendConnectSessionName = sessionName;
}

int __fastcall FakeLaunchInitSessionRuntime(GUID *appGuid)
{
    ++g_launchInitSessionCalls;
    g_launchInitSessionGuid = appGuid;
    return 0;
}

void __fastcall FakeLaunchFormatIpv4Address(char *outText,
                                                 unsigned int ipAddress)
{
    ++g_launchFormatIpv4Calls;
    g_launchFormatIpv4Packed = ipAddress;
    strcpy(outText, g_launchFormattedHost);
}

int __fastcall FakeLaunchSelectTcpIpProviderAndEnumSessions(
    char *addressString,
    int skipSessionEnumeration)
{
    ++g_launchSelectTcpCalls;
    strcpy(g_launchSelectTcpAddress, addressString);
    g_launchSelectTcpSkip = skipSessionEnumeration;
    return g_launchSelectTcpResult;
}

int __fastcall FakeLaunchCreateSessionFromStatusFields(
    zNetworkSessionDescStatusFields *statusFields)
{
    ++g_launchCreateSessionCalls;
    g_launchCreateSessionFields = *statusFields;
    return g_launchCreateSessionResult;
}

void __fastcall FakeLaunchSetNetworkEnabled(int value)
{
    ++g_launchSetNetworkEnabledCalls;
    g_launchSetNetworkEnabledValue = value;
}

class LaunchDialogPatchOps
{
  public:
    CString * GetSelectedProfilePlayerName(CString *outName)
    {
        const int index = g_launchGetPlayerNameCalls;
        if (index < 4)
        {
            g_launchGetPlayerNameThis[index] =
                (WestwoodOnlineUpgradeDialog *)this;
        }
        ++g_launchGetPlayerNameCalls;
        outName->CString::CString(g_launchPlayerName);
        return outName;
    }
};

int __fastcall FakeLaunchCreateLocalPlayerRecordAndRegister(
    char *playerName)
{
    const int index = g_launchCreateLocalPlayerCalls;
    if (index < 4)
    {
        strcpy(g_launchCreateLocalPlayerName[index], playerName);
    }
    ++g_launchCreateLocalPlayerCalls;
    return 1;
}

int __fastcall FakeLaunchOpenSelectedSessionAndReadStatusFields(
    zNetworkSessionDescStatusFields *statusFields)
{
    ++g_launchOpenSelectedCalls;
    g_launchOpenSelectedInputFields = *statusFields;
    if (g_launchOpenSelectedResult != 0)
    {
        *statusFields = g_launchOpenSelectedOutputFields;
    }
    return g_launchOpenSelectedResult;
}

void __fastcall FakeLaunchSetPlayerName(const char *name)
{
    ++g_launchSetPlayerNameCalls;
    strcpy(g_launchSetPlayerName, name);
}

void __fastcall FakeLaunchSetStatusBitsFromFlags(unsigned int statusFlags)
{
    ++g_launchSetStatusBitsCalls;
    g_launchStatusBits = statusFlags;
}

class LaunchHudSensorTrackerPatchOps
{
  public:
    void SetRuntimeTimerSecAndGoalValue(int timerSecRaw,
                                                        int goalValue)
    {
        ++g_launchTimerCalls;
        g_launchTimerThis = (HudSensorTracker *)this;
        g_launchTimerSecondsRaw = timerSecRaw;
        g_launchTimerGoalValue = goalValue;
    }
};

LRESULT WINAPI FakeLaunchSendMessageA(HWND hWnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam)
{
    ++g_launchSendMessageCalls;
    g_launchSendMessageHwnd = hWnd;
    g_launchSendMessageMsg = msg;
    g_launchSendMessageWParam = wParam;
    g_launchSendMessageLParam = lParam;
    return g_launchSendMessageResult;
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

int FakeApiCreateShowModalAndApplySelectedProfileValues()
{
    ++g_apiCreateShowModalCalls;
    return g_apiCreateShowModalResult;
}

int __fastcall FakeInitCreateInstanceAndLoadConfig(
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

BOOL __fastcall FakeInitCreateProgress(CDialog *, void *, LPCSTR resourceName,
                                            CWnd *parentWnd)
{
    ++g_initCreateProgressCalls;
    g_initCreateProgressResource = resourceName;
    g_initCreateProgressParent = parentWnd;
    return TRUE;
}

void __fastcall FakeInitSetDlgItemTextA(CWnd *self, void *, int controlId,
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

char *__fastcall FakeInitGetMessageString(unsigned int messageId)
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

int __fastcall FakeInitDialogBaseOnInitDialog(CDialog *self, void *)
{
    ++g_initDialogBaseOnInitCalls;
    g_initDialogBaseOnInitThis = self;
    return 1;
}

int FakeInitDialogApiInit()
{
    ++g_initDialogApiInitCalls;
    return g_initDialogApiInitResult;
}

void __fastcall FakeInitDialogOnDestroy(
    WestwoodOnlineUpgradeDialog *self,
    void *)
{
    ++g_initDialogOnDestroyCalls;
    g_initDialogOnDestroyThis = self;
}

void __fastcall FakeInitDialogSetAbortAndClose(
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

    if (msg == CB_ADDSTRING || msg == CB_INSERTSTRING)
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

ULONG STDMETHODCALLTYPE FakeApiEventSinkAddRef(IUnknown *self)
{
    ++g_apiEventSinkAddRefCalls;
    g_apiEventSinkAddRefSelf = self;
    return 2;
}

void STDMETHODCALLTYPE FakeResetQueryState(IUnknown *)
{
    ++g_resetQueryStateCalls;
}

void STDMETHODCALLTYPE FakeInitDisconnect(IUnknown *)
{
    ++g_initDisconnectCalls;
}

int __fastcall FakeBeginConnectGetWindowTextA(CWnd *self, void *,
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

int __fastcall FakeQueryStatusGetWindowTextA(CWnd *self, void *,
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

int __fastcall FakeWestwoodUpdateData(CWnd *, void *, BOOL saveAndValidate)
{
    if (g_threeFloatUpdateDataCount < 8)
    {
        g_threeFloatUpdateDataSaveValue[g_threeFloatUpdateDataCount] =
            saveAndValidate;
    }
    ++g_threeFloatUpdateDataCount;
    return 1;
}

long __fastcall FakeWestwoodDefault(CWnd *, void *)
{
    ++g_threeFloatDefaultCount;
    return g_threeFloatDefaultReturn;
}

void __fastcall FakeQueryStatusSetWindowTextA(CWnd *self, void *,
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

int __fastcall FakeQueryStatusMessageBoxA(CWnd *self, void *,
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

void __fastcall FakeRefreshCurrentQueryGetWindowTextA(CWnd *self, void *,
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

void __fastcall FakeSubmitVisibleGetWindowTextA(CWnd *self, void *,
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

unsigned int FakeSubmitVisibleFormatMessage(
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

int FakeSubmitVisibleAppendStatusTextFmt(
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

void ResetAppendSessionRequestStatus301BFakes(void)
{
    g_appendStatus301BFormatCalls = 0;
    g_appendStatus301BFormatBuffer = 0;
    g_appendStatus301BFormatMaxChars = 0;
    g_appendStatus301BFormatMessageId = 0;
    g_appendStatus301BFormatSessionName = 0;
    g_appendStatus301BFormatStatusText = 0;
    g_appendStatus301BAppendCalls = 0;
    g_appendStatus301BAppendThis = 0;
    g_appendStatus301BAppendText[0] = '\0';
}

unsigned int FakeAppendSessionRequestStatus301BFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName,
    const char *statusText)
{
    ++g_appendStatus301BFormatCalls;
    g_appendStatus301BFormatBuffer = outBuffer;
    g_appendStatus301BFormatMaxChars = maxChars;
    g_appendStatus301BFormatMessageId = messageId;
    g_appendStatus301BFormatSessionName = sessionName;
    g_appendStatus301BFormatStatusText = statusText;
    strcpy(outBuffer, "formatted 301b status");
    return lstrlenA(outBuffer);
}

int FakeAppendSessionRequestStatus301BAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *text)
{
    ++g_appendStatus301BAppendCalls;
    g_appendStatus301BAppendThis = self;
    strcpy(g_appendStatus301BAppendText, text);
    return 1;
}

void ResetAppendSessionRequestStatus301CFakes(void)
{
    g_appendStatus301CFormatCalls = 0;
    g_appendStatus301CFormatBuffer = 0;
    g_appendStatus301CFormatMaxChars = 0;
    g_appendStatus301CFormatMessageId = 0;
    g_appendStatus301CFormatSessionName = 0;
    g_appendStatus301CFormatStatusText = 0;
    g_appendStatus301CAppendCalls = 0;
    g_appendStatus301CAppendThis = 0;
    g_appendStatus301CAppendText[0] = '\0';
}

unsigned int FakeAppendSessionRequestStatus301CFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName,
    const char *statusText)
{
    ++g_appendStatus301CFormatCalls;
    g_appendStatus301CFormatBuffer = outBuffer;
    g_appendStatus301CFormatMaxChars = maxChars;
    g_appendStatus301CFormatMessageId = messageId;
    g_appendStatus301CFormatSessionName = sessionName;
    g_appendStatus301CFormatStatusText = statusText;
    strcpy(outBuffer, "formatted 301c status");
    return lstrlenA(outBuffer);
}

int FakeAppendSessionRequestStatus301CAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *text)
{
    ++g_appendStatus301CAppendCalls;
    g_appendStatus301CAppendThis = self;
    strcpy(g_appendStatus301CAppendText, text);
    return 1;
}

void ResetAppendSessionRequestStatus301CAltFakes(void)
{
    g_initMessageIdCalls = 0;
    g_appendStatus301CAltAppendCalls = 0;
    g_appendStatus301CAltAppendThis = 0;
    g_appendStatus301CAltAppendFormat = 0;
    g_appendStatus301CAltAppendSessionName = 0;
    g_appendStatus301CAltAppendValue = 0;
}

int FakeAppendSessionRequestStatus301CAltAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *format,
    ...)
{
    ++g_appendStatus301CAltAppendCalls;
    g_appendStatus301CAltAppendThis = self;
    g_appendStatus301CAltAppendFormat = format;

    va_list args;
    va_start(args, format);
    g_appendStatus301CAltAppendSessionName = va_arg(args, const char *);
    g_appendStatus301CAltAppendValue = va_arg(args, int);
    va_end(args);
    return 1;
}

void ResetAppendSessionRequestStatus301DFakes(void)
{
    g_appendStatus301DFormatCalls = 0;
    g_appendStatus301DFormatBuffer = 0;
    g_appendStatus301DFormatMaxChars = 0;
    g_appendStatus301DFormatMessageId = 0;
    g_appendStatus301DFormatSessionName = 0;
    g_appendStatus301DFormatStatusText = 0;
    g_appendStatus301DAppendCalls = 0;
    g_appendStatus301DAppendThis = 0;
    g_appendStatus301DAppendText[0] = '\0';
}

unsigned int FakeAppendSessionRequestStatus301DFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName,
    const char *statusText)
{
    ++g_appendStatus301DFormatCalls;
    g_appendStatus301DFormatBuffer = outBuffer;
    g_appendStatus301DFormatMaxChars = maxChars;
    g_appendStatus301DFormatMessageId = messageId;
    g_appendStatus301DFormatSessionName = sessionName;
    g_appendStatus301DFormatStatusText = statusText;
    strcpy(outBuffer, "formatted 301d status");
    return lstrlenA(outBuffer);
}

int FakeAppendSessionRequestStatus301DAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *text)
{
    ++g_appendStatus301DAppendCalls;
    g_appendStatus301DAppendThis = self;
    strcpy(g_appendStatus301DAppendText, text);
    return 1;
}

void ResetAppendConnectStatusFakes(void)
{
    g_initMessageIdCalls = 0;
    g_appendConnectStatusCalls = 0;
    for (int index = 0; index < 8; ++index)
    {
        g_appendConnectStatusThis[index] = 0;
        g_appendConnectStatusText[index][0] = '\0';
    }
}

int FakeAppendConnectStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *text)
{
    const int index = g_appendConnectStatusCalls;
    if (index < 8)
    {
        g_appendConnectStatusThis[index] = self;
        strcpy(g_appendConnectStatusText[index], text);
    }
    ++g_appendConnectStatusCalls;
    return 1;
}

void ResetAppendBrowseRecordStatusFakes(void)
{
    ResetAppendConnectStatusFakes();
    g_appendBrowseRecordStatusFormatCalls = 0;
    g_appendBrowseRecordStatusFormatBuffer = 0;
    g_appendBrowseRecordStatusFormatMaxChars = 0;
    g_appendBrowseRecordStatusFormatMessageId = 0;
    g_appendBrowseRecordStatusFormatSessionName = 0;
}

unsigned int FakeAppendBrowseRecordStatusFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName)
{
    ++g_appendBrowseRecordStatusFormatCalls;
    g_appendBrowseRecordStatusFormatBuffer = outBuffer;
    g_appendBrowseRecordStatusFormatMaxChars = maxChars;
    g_appendBrowseRecordStatusFormatMessageId = messageId;
    g_appendBrowseRecordStatusFormatSessionName = sessionName;
    strcpy(outBuffer, "formatted browse record status");
    return lstrlenA(outBuffer);
}

void ResetAppendValueStatusFakes(void)
{
    ResetAppendConnectStatusFakes();
    g_appendValueStatusFormatCalls = 0;
    g_appendValueStatusFormatBuffer = 0;
    g_appendValueStatusFormatMaxChars = 0;
    g_appendValueStatusFormatMessageId = 0;
    g_appendValueStatusFormatValue = 0;
}

unsigned int FakeAppendValueStatusFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    int value)
{
    ++g_appendValueStatusFormatCalls;
    g_appendValueStatusFormatBuffer = outBuffer;
    g_appendValueStatusFormatMaxChars = maxChars;
    g_appendValueStatusFormatMessageId = messageId;
    g_appendValueStatusFormatValue = value;
    strcpy(outBuffer, "formatted value status");
    return lstrlenA(outBuffer);
}

void ResetAppendTimeStatusFakes(void)
{
    ResetAppendConnectStatusFakes();
    g_appendTimeStatusFormatCalls = 0;
    g_appendTimeStatusFormatBuffer = 0;
    g_appendTimeStatusFormatMaxChars = 0;
    g_appendTimeStatusFormatMessageId = 0;
    g_appendTimeStatusFormatTimeText[0] = '\0';
}

unsigned int FakeAppendTimeStatusFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *timeText)
{
    ++g_appendTimeStatusFormatCalls;
    g_appendTimeStatusFormatBuffer = outBuffer;
    g_appendTimeStatusFormatMaxChars = maxChars;
    g_appendTimeStatusFormatMessageId = messageId;
    strcpy(g_appendTimeStatusFormatTimeText, timeText);
    strcpy(outBuffer, "formatted time status");
    return lstrlenA(outBuffer);
}

void ResetBrowseRecordListFakes(void)
{
    ResetPendingSessionRemovedFakes();
    g_browseRecordListFormatCalls = 0;
    for (int index = 0; index < 12; ++index)
    {
        g_browseRecordListFormatBuffer[index] = 0;
        g_browseRecordListFormatMaxChars[index] = 0;
        g_browseRecordListFormatMessageId[index] = 0;
        g_browseRecordListFormatSessionName[index] = 0;
        g_browseRecordListFormatMetric0[index] = 0;
        g_browseRecordListFormatMetric1[index] = 0;
        g_browseRecordListFormatLatencyText[index] = 0;
    }
}

unsigned int FakeBrowseRecordListFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName,
    int displayMetric1,
    ...)
{
    const int index = g_browseRecordListFormatCalls;
    int displayMetric0 = 0;
    const char *latencyText = "";

    va_list args;
    va_start(args, displayMetric1);
    if (messageId == 0x3028)
    {
        displayMetric0 = va_arg(args, int);
        latencyText = va_arg(args, const char *);
    }
    va_end(args);

    if (index < 12)
    {
        g_browseRecordListFormatBuffer[index] = outBuffer;
        g_browseRecordListFormatMaxChars[index] = maxChars;
        g_browseRecordListFormatMessageId[index] = messageId;
        g_browseRecordListFormatSessionName[index] = sessionName;
        g_browseRecordListFormatMetric0[index] = displayMetric0;
        g_browseRecordListFormatMetric1[index] = displayMetric1;
        g_browseRecordListFormatLatencyText[index] = latencyText;
    }
    ++g_browseRecordListFormatCalls;

    if (messageId == 0x3028)
    {
        wsprintfA(outBuffer,
                  "%s:%04x:%d:%d:%s",
                  sessionName,
                  messageId,
                  displayMetric1,
                  displayMetric0,
                  latencyText);
    }
    else
    {
        wsprintfA(outBuffer,
                  "%s:%04x:%d",
                  sessionName,
                  messageId,
                  displayMetric1);
    }
    return lstrlenA(outBuffer);
}

void FakeNetworkStatusReturnOnlyStub(void)
{
    ++g_networkStatusReturnOnlyCalls;
}

void ResetNetworkStatusFakes(void)
{
    g_networkStatusReturnOnlyCalls = 0;
    g_initDialogSetAbortCalls = 0;
    g_initDialogSetAbortThis = 0;
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

int __fastcall FakeInitDestroyProgress(CWnd *self, void *)
{
    ++g_initDestroyProgressCalls;
    g_initDestroyedProgress = self;
    return 1;
}

void __fastcall FakeDestroyBeginDisconnect(WestwoodOnlineUpgradeDialog *self,
                                                void *)
{
    ++g_destroyBeginDisconnectCalls;
    g_destroyBeginDisconnectThis = self;
}

void FakeDestroyApiShutdown()
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

int __fastcall FakeWolEnableWindow(CWnd *self, void *, int enable)
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

unsigned int FakeAppendConnectFormatMessage(char *outBuffer, int maxChars,
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

int FakeAppendConnectAppendStatusTextFmt(WestwoodOnlineUpgradeDialog *self,
                                                      const char *text)
{
    ++g_appendConnectAppendCalls;
    g_appendConnectAppendThis = self;
    g_appendConnectAppendText = text;
    return 1;
}

void __fastcall FakeAppendConnectResetSelectedBrowseRecord(
    WestwoodOnlineUpgradeDialog *self,
    void *)
{
    ++g_appendConnectResetCalls;
    g_appendConnectResetThis = self;
}

void __fastcall FakeAbortOnCancel(CDialog *self, void *)
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

void __fastcall FakeModalSetMenuBarVisibility(CZRecoilFrame *self, void *, int visible)
{
    if (g_modalMenuStep < 2) {
        g_modalMenuThis[g_modalMenuStep] = self;
        g_modalMenuVisible[g_modalMenuStep] = visible;
    }
    ++g_modalMenuStep;
}

int __fastcall FakeModalDoModal(void *self, void *)
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

int __fastcall FakeConfigDoModal(void *self, void *)
{
    ++g_configDoModalCalls;
    g_configDoModalThis = (WestwoodOnlineUpgradeConfigDialog *)self;
    g_configDoModalThis->m_selectedProfileIndex = 1;
    g_configDoModalThis->m_profilePlayerNames[1] = "DialogPilot";
    g_configDoModalThis->m_profileConnectStrings[1] = "DialogConnect";
    g_configDoModalThis->m_profileConnectStringModes[1] = 21;
    return g_configDoModalResult;
}

void __fastcall FakeModalDialogDtor(void *, void *)
{
    ++g_modalDialogDtorCalls;
    RecordModalDtorSequence('D');
}

void __fastcall FakeModalCStringDtor(void *, void *)
{
    ++g_modalCStringDtorCalls;
    RecordModalDtorSequence('S');
}

void __fastcall FakeModalListDtor(void *, void *)
{
    ++g_modalListDtorCalls;
    RecordModalDtorSequence('L');
}

void __fastcall FakeModalEditDtor(void *, void *)
{
    ++g_modalEditDtorCalls;
    RecordModalDtorSequence('E');
}

void __fastcall FakeModalComboDtor(void *, void *)
{
    ++g_modalComboDtorCalls;
    RecordModalDtorSequence('C');
}

void __fastcall FakeModalButtonDtor(void *, void *)
{
    ++g_modalButtonDtorCalls;
    RecordModalDtorSequence('B');
}

void __stdcall FakeModalDDXControl(CDataExchange *dataExchange,
                                        int controlId, void *control)
{
    RecordModalDdx(dataExchange, 1, controlId, control);
}

void __stdcall FakeModalDDXTextUInt(CDataExchange *dataExchange,
                                         int controlId, unsigned int *value)
{
    RecordModalDdx(dataExchange, 2, controlId, value);
}

void __stdcall FakeModalDDXTextCString(CDataExchange *dataExchange,
                                            int controlId, CString *value)
{
    RecordModalDdx(dataExchange, 2, controlId, value);
}

void __stdcall FakeModalDDXCheck(CDataExchange *dataExchange,
                                      int controlId, int *value)
{
    RecordModalDdx(dataExchange, 3, controlId, value);
}

LRESULT WINAPI FakeConfigFocusSendMessageA(HWND hwnd,
                                           UINT msg,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    ++g_configFocusSendMessageCalls;
    g_configFocusSendMessageHwnd = hwnd;
    g_configFocusSendMessageMsg = msg;
    g_configFocusSendMessageWParam = wParam;
    g_configFocusSendMessageLParam = lParam;
    return 0;
}

int STDMETHODCALLTYPE FakeConfigInitLoadConnectProfileStrings(
    IUnknown *,
    int profileId,
    char **playerNameOut,
    char **connectStringOut)
{
    const int index = g_configInitLoadProfileCalls;
    if (index < 4)
    {
        g_configInitLoadProfileIds[index] = profileId;
        *playerNameOut = (char *)g_configInitLoadProfilePlayers[index];
        *connectStringOut = (char *)g_configInitLoadProfileConnectStrings[index];
    }
    ++g_configInitLoadProfileCalls;
    return index < 4 ? g_configInitLoadProfileResults[index] : 1;
}

void ConstructConfigDialogStrings(WestwoodOnlineUpgradeConfigDialog &dialog)
{
    new (&dialog.m_reservedString) CString();
    new (&dialog.m_connectStringEditText) CString();
    for (int index = 0; index < 2; ++index)
    {
        new (&dialog.m_savedPlayerNames[index]) CString();
        new (&dialog.m_savedConnectStrings[index]) CString();
        new (&dialog.m_profilePlayerNames[index]) CString();
        new (&dialog.m_profileConnectStrings[index]) CString();
    }
}

void DestructConfigDialogStrings(WestwoodOnlineUpgradeConfigDialog &dialog)
{
    for (int index = 1; index >= 0; --index)
    {
        dialog.m_profileConnectStrings[index].CString::~CString();
        dialog.m_profilePlayerNames[index].CString::~CString();
        dialog.m_savedConnectStrings[index].CString::~CString();
        dialog.m_savedPlayerNames[index].CString::~CString();
    }
    dialog.m_connectStringEditText.CString::~CString();
    dialog.m_reservedString.CString::~CString();
}

void __fastcall FakeConfigInitSetWindowTextA(
    CWnd *self,
    void *,
    const char *text)
{
    ++g_configInitSetWindowTextCalls;
    g_configInitSetWindowTextThis = self;
    g_configInitSetWindowTextValue = text;
}

int STDMETHODCALLTYPE FakeConfigOnOkSaveConnectProfileStrings(
    IUnknown *,
    int profileId,
    const char *playerName,
    const char *connectString,
    int connectStringMode)
{
    const int index = g_configOnOkSaveProfileCalls;
    if (index < 4)
    {
        g_configOnOkSaveProfileIds[index] = profileId;
        g_configOnOkSaveProfilePlayers[index] = playerName;
        g_configOnOkSaveProfileConnectStrings[index] = connectString;
        g_configOnOkSaveProfileModes[index] = connectStringMode;
    }
    ++g_configOnOkSaveProfileCalls;
    return 0;
}

void __fastcall FakeConfigOnOkBaseOnOK(CDialog *self, void *)
{
    ++g_configOnOkBaseOnOkCalls;
    g_configOnOkBaseOnOkThis = self;
}

void __fastcall FakeConfigComboKillFocusGetWindowTextA(
    CWnd *self,
    void *,
    CString *text)
{
    ++g_configComboKillFocusGetWindowTextCalls;
    g_configComboKillFocusGetWindowTextThis = self;
    *text = g_configComboKillFocusText;
}

LRESULT WINAPI FakeConfigSelChangeSendMessageA(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    ++g_configSelChangeSendMessageCalls;
    g_configSelChangeSendMessageHwnd = hwnd;
    g_configSelChangeSendMessageMsg = msg;
    g_configSelChangeSendMessageWParam = wParam;
    g_configSelChangeSendMessageLParam = lParam;
    return g_configSelChangeSendMessageResult;
}

void FakeModalTimeTick()
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
    const WORD kMfc42DDXTextCStringOrdinal = 2370;

    return PatchImportByOrdinal("MFC42.DLL", kMfc42DDXControlOrdinal,
                                (void *)&FakeModalDDXControl, imports[0]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42DDXTextUIntOrdinal,
                                (void *)&FakeModalDDXTextUInt, imports[1]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42DDXCheckOrdinal,
                                (void *)&FakeModalDDXCheck, imports[2]) &&
           PatchImportByOrdinal("MFC42.DLL", kMfc42DDXTextCStringOrdinal,
                                (void *)&FakeModalDDXTextCString, imports[3]);
}

void RestoreModalDdxPatches(ImportFunctionPatch *imports)
{
    for (int index = 3; index >= 0; --index)
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

extern "C" int westwood_online_upgrade_api_event_sink_create_instance_smoke(void)
{
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    WestwoodOnlineUpgradeApiEventSink *eventSink = 0;

    HRESULT result = WestwoodOnlineUpgradeApiEventSink::CreateInstance(&eventSink);
    int failure = 0;
    if (result != S_OK || eventSink == 0)
    {
        failure = 1;
    }
    else if (TestObjectVtable(eventSink) == 0)
    {
        failure = 2;
    }
    else if (eventSink->m_refCountAndLock.refCount != 0 ||
             g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount != oldLiveCount + 1)
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
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    return failure;
}


extern "C" int westwood_online_upgrade_api_event_sink_query_interface_smoke(void)
{
    ApiEventSinkSmokeVtable vtable = {};
    WestwoodOnlineUpgradeApiEventSink sink = {};
    GUID otherIid = g_WestwoodOnlineUpgradeApiEventSink_IID;
    void *outInterface;
    HRESULT result;

    vtable.slots[1] = (void *)&FakeApiEventSinkAddRef;
    *(void **)&sink = &vtable;

    g_apiEventSinkAddRefCalls = 0;
    g_apiEventSinkAddRefSelf = 0;
    outInterface = (void *)0xcccccccc;
    result = WestwoodOnlineUpgradeApiEventSink::QueryInterface(
        &sink,
        g_WestwoodOnlineUpgradeApiEventSink_IID,
        &outInterface
    );
    if (result != S_OK ||
        outInterface != &sink ||
        g_apiEventSinkAddRefCalls != 1 ||
        g_apiEventSinkAddRefSelf != (IUnknown *)&sink)
    {
        return 1;
    }

    outInterface = (void *)0xcccccccc;
    result = WestwoodOnlineUpgradeApiEventSink::QueryInterface(
        &sink,
        IID_IUnknown,
        &outInterface
    );
    if (result != S_OK ||
        outInterface != &sink ||
        g_apiEventSinkAddRefCalls != 2 ||
        g_apiEventSinkAddRefSelf != (IUnknown *)&sink)
    {
        return 2;
    }

    otherIid.Data1 ^= 1;
    outInterface = (void *)0xcccccccc;
    result = WestwoodOnlineUpgradeApiEventSink::QueryInterface(
        &sink,
        otherIid,
        &outInterface
    );
    if (result != E_NOINTERFACE ||
        outInterface != 0 ||
        g_apiEventSinkAddRefCalls != 2)
    {
        return 3;
    }

    result = WestwoodOnlineUpgradeApiEventSink::QueryInterface(
        &sink,
        g_WestwoodOnlineUpgradeApiEventSink_IID,
        0
    );
    return result == E_POINTER ? 0 : 4;
}


extern "C" int westwood_online_upgrade_api_event_sink_lifetime_smoke(void)
{
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    ApiEventSinkSmokeVtable otherVtable = {};
    int failure = 0;

    WestwoodOnlineUpgradeApiEventSink stackSink = {};
    stackSink.m_refCountAndLock.Init();
    *(void **)&stackSink = &otherVtable;
    stackSink.m_refCountAndLock.refCount = 5;
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = 10;
    stackSink.Destructor();
    if (TestObjectVtable(&stackSink) != &otherVtable ||
        stackSink.m_refCountAndLock.refCount != 1 ||
        g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount != 9)
    {
        failure = 1;
    }

    WestwoodOnlineUpgradeApiEventSink *eventSink =
        (WestwoodOnlineUpgradeApiEventSink *)::operator new(
            sizeof(WestwoodOnlineUpgradeApiEventSink));
    eventSink->m_refCountAndLock.Init();
    *(void **)eventSink = &otherVtable;
    eventSink->m_refCountAndLock.refCount = 2;
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = 20;
    ULONG refCount = WestwoodOnlineUpgradeApiEventSink::Release(eventSink);
    if (failure == 0 &&
        (refCount != 1 ||
         eventSink->m_refCountAndLock.refCount != 1 ||
         TestObjectVtable(eventSink) != &otherVtable ||
         g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount != 20))
    {
        failure = 2;
    }
    DeleteCriticalSection(&eventSink->m_refCountAndLock.lock);
    ::operator delete(eventSink);

    eventSink = (WestwoodOnlineUpgradeApiEventSink *)::operator new(
        sizeof(WestwoodOnlineUpgradeApiEventSink));
    eventSink->m_refCountAndLock.Init();
    *(void **)eventSink = &otherVtable;
    eventSink->m_refCountAndLock.refCount = 1;
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = 20;
    refCount = WestwoodOnlineUpgradeApiEventSink::Release(eventSink);
    if (failure == 0 &&
        (refCount != 0 ||
         g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount != 19))
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
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
    CWnd *const oldMainWnd = g_RecoilApp.m_pMainWnd;
    CodeFunctionPatch patches[4] = {};
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
    g_RecoilApp.m_pMainWnd = &mainWnd;
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
westwood_online_upgrade_api_event_sink_on_session_launch_result_smoke(void)
{
    const WORD kMfc42CWndSendDlgItemMessageAOrdinal = 5802;
    const int oldActiveListMode = g_WestwoodOnlineUpgradeActiveListMode;
    const int oldCreateFromQuery =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldPendingSessionCount =
        g_WestwoodOnlineUpgradePendingSessionResultCount;
    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeBrowseRecord oldCachedBrowseRecord =
        g_WestwoodOnlineUpgradeCachedBrowseRecord;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[4] = {};
    ImportFunctionPatch import = {};

    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeSessionLaunchFormatMessage,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeBrowseResolvedAppendStatusTextFmt,
            patches[1]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::EnableQueryControls),
            (void *)&FakeBrowseRecordAddedEnableQueryControls,
            patches[2]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::EnableConnectButton),
            (void *)&FakeBrowseResolvedEnableConnectButton,
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
    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.RequestListMode = FakeInitRequestListMode;
    g_initFakeApiVtable.CancelPendingSessionFlow =
        FakeSessionFinishedCancelPendingSessionFlow;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    WestwoodOnlineUpgradeBrowseRecord browseRecord = {};
    strcpy(browseRecord.m_sessionName, "Browse");
    WestwoodOnlineUpgradeSessionRequest sessionNode = {};
    strcpy(sessionNode.m_sessionName, "Node");
    WestwoodOnlineUpgradeSessionRequest sessionRequest = {};
    strcpy(sessionRequest.m_sessionName, "Request");

    int failure = 0;
    g_WestwoodOnlineUpgradeActiveListMode = 44;
    ResetBrowseResolvedFakes();
    int result = WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult(
        0,
        -1,
        &browseRecord,
        &sessionNode,
        &sessionRequest
    );
    if (result != 0 ||
        g_sessionFinishedCancelCalls != 1 ||
        g_sessionFinishedCancelSelf != (IUnknown *)&g_initFakeApi ||
        g_sessionLaunchFormatCalls != 0 ||
        g_browseResolvedAppendCalls != 0 ||
        g_pendingRemovedSendMessageCalls != 0 ||
        g_initRequestListModeCalls != 0)
    {
        failure = 1;
    }

    sessionNode.m_rowFlags = 0x8000;
    g_WestwoodOnlineUpgradeActiveListMode = 55;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 7;
    strcpy(g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName, "Cached");
    ResetBrowseResolvedFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult(
        0,
        0,
        &browseRecord,
        &sessionNode,
        &sessionRequest
    );
    if (failure == 0 &&
        (result != 0 ||
         g_sessionLaunchFormatCalls != 1 ||
         g_sessionLaunchFormatMaxChars != 128 ||
         g_sessionLaunchFormatMessageId != 0x302d ||
         g_sessionLaunchFormatArg0 != browseRecord.m_sessionName ||
         g_sessionLaunchFormatArg1 != sessionRequest.m_sessionName ||
         g_sessionLaunchFormatArg2 != 0 ||
         g_browseResolvedAppendCalls != 1 ||
         strcmp(g_browseResolvedAppendFormat[0],
                "launch 302d Browse Request") != 0 ||
         g_initRequestListModeCalls != 1 ||
         g_initRequestListMode != 55 ||
         g_initRequestListModeEnabled != 1 ||
         g_pendingRemovedSendMessageCalls != 1 ||
         g_pendingRemovedSendMessageControlId[0] != 1137 ||
         g_pendingRemovedSendMessageMessage[0] != LB_RESETCONTENT ||
         g_pendingRemovedSendMessageWParam[0] != 0 ||
         g_pendingRemovedSendMessageLParam[0] != 0 ||
         g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0 ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 0 ||
         g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0' ||
         g_browseRecordAddedEnableCalls != 1 ||
         g_browseRecordAddedEnableThis[0] != &dialog ||
         g_browseRecordAddedEnableValue[0] != 0 ||
         g_browseResolvedConnectCalls != 1 ||
         g_browseResolvedConnectThis[0] != &dialog ||
         g_browseResolvedConnectValue[0] != 0))
    {
        failure = 2;
    }

    sessionNode.m_rowFlags = 0;
    g_WestwoodOnlineUpgradePendingSessionResultCount = 6;
    ResetBrowseResolvedFakes();
    g_pendingRemovedSendMessageResult[0] = 4;
    result = WestwoodOnlineUpgradeApiEventSink::OnSessionLaunchResult(
        0,
        0,
        &browseRecord,
        &sessionNode,
        &sessionRequest
    );
    if (failure == 0 &&
        (result != 0 ||
         g_sessionLaunchFormatCalls != 1 ||
         g_sessionLaunchFormatMaxChars != 128 ||
         g_sessionLaunchFormatMessageId != 0x302e ||
         g_sessionLaunchFormatArg0 != sessionNode.m_sessionName ||
         g_sessionLaunchFormatArg1 != browseRecord.m_sessionName ||
         g_sessionLaunchFormatArg2 != sessionRequest.m_sessionName ||
         strcmp(g_browseResolvedAppendFormat[0],
                "launch 302e Node Browse Request") != 0 ||
         g_pendingRemovedSendMessageCalls != 2 ||
         g_pendingRemovedSendMessageMessage[0] != LB_FINDSTRINGEXACT ||
         g_pendingRemovedSendMessageWParam[0] != (WPARAM)-1 ||
         strcmp(g_pendingRemovedSendMessageText[0], "Node") != 0 ||
         g_pendingRemovedSendMessageMessage[1] != LB_DELETESTRING ||
         g_pendingRemovedSendMessageWParam[1] != 4 ||
         g_pendingRemovedSendMessageLParam[1] != 0 ||
         g_WestwoodOnlineUpgradePendingSessionResultCount != 5 ||
         g_initRequestListModeCalls != 0 ||
         g_browseRecordAddedEnableCalls != 0 ||
         g_browseResolvedConnectCalls != 0))
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradeActiveListMode = oldActiveListMode;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFromQuery;
    g_WestwoodOnlineUpgradePendingSessionResultCount = oldPendingSessionCount;
    g_WestwoodOnlineUpgradeCachedBrowseRecord = oldCachedBrowseRecord;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreImportPatch(import);
    for (int index = 3; index >= 0; --index)
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

int LaunchFloatBits(float value)
{
    union
    {
        float value;
        int raw;
    } bits = {value};
    return bits.raw;
}

extern "C" int
westwood_online_upgrade_api_event_sink_launch_selected_session_smoke(void)
{
    int failure = 0;
    int result;

    g_initDisconnectCalls = 0;
    result = WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession(
        0,
        -1,
        0,
        0,
        0
    );
    if (result != 0 || g_initDisconnectCalls != 0)
    {
        return 1;
    }

    CodeFunctionPatch patches[12] = {};
    ImportFunctionPatch sendMessagePatch = {};
    int patchCount = 0;
    bool installed = true;
    installed = installed &&
                PatchFunctionJump(CWndUpdateDataAddress(),
                                  (void *)&FakeWestwoodUpdateData,
                                  patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump((void *)&zNetwork::InitSessionRuntime,
                                  (void *)&FakeLaunchInitSessionRuntime,
                                  patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump((void *)&Net::FormatIpv4Address,
                                  (void *)&FakeLaunchFormatIpv4Address,
                                  patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump(
                    (void *)&zNetworkDPlay::SelectTcpIpProviderAndEnumSessions,
                    (void *)&FakeLaunchSelectTcpIpProviderAndEnumSessions,
                    patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump(
                    (void *)&zNetwork_DPlay::CreateSessionFromStatusFields,
                    (void *)&FakeLaunchCreateSessionFromStatusFields,
                    patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump((void *)&zOpt::SetNetworkEnabled,
                                  (void *)&FakeLaunchSetNetworkEnabled,
                                  patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump(
                    MethodAddress(
                        &WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName),
                    MethodAddress(
                        &LaunchDialogPatchOps::GetSelectedProfilePlayerName),
                    patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump(
                    (void *)&zNetwork_DPlay::CreateLocalPlayerRecordAndRegister,
                    (void *)&FakeLaunchCreateLocalPlayerRecordAndRegister,
                    patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump(
                    (void *)&zNetworkDPlay::OpenSelectedSessionAndReadStatusFields,
                    (void *)&FakeLaunchOpenSelectedSessionAndReadStatusFields,
                    patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump((void *)&zOpt::SetPlayerName,
                                  (void *)&FakeLaunchSetPlayerName,
                                  patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump((void *)&GameNet::SetStatusBitsFromFlags,
                                  (void *)&FakeLaunchSetStatusBitsFromFlags,
                                  patches[patchCount++]);
    installed = installed &&
                PatchFunctionJump(
                    MethodAddress(
                        &HudSensorTracker::SetRuntimeTimerSecAndGoalValue),
                    MethodAddress(
                        &LaunchHudSensorTrackerPatchOps::
                            SetRuntimeTimerSecAndGoalValue),
                    patches[patchCount++]);
    installed = installed &&
                PatchImportByName("USER32.dll",
                                  "SendMessageA",
                                  (void *)&FakeLaunchSendMessageA,
                                  sendMessagePatch);
    if (!installed)
    {
        RestoreImportPatch(sendMessagePatch);
        for (int index = patchCount - 1; index >= 0; --index)
        {
            RestoreFunctionPatch(patches[index]);
        }
        return 90;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeDialog *const oldDialog =
        g_pWestwoodOnlineUpgradeDialog;
    const int oldCreateFromQuery =
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag;
    const int oldMissionIndex = g_WestwoodOnlineUpgradeSelectedMissionIndex;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    new (&dialog.m_sessionName) CString("Arena");
    dialog.m_sessionModeCombo.m_hWnd = (HWND)0x12345678;
    dialog.m_queryStatusFlagBit0 = 1;
    dialog.m_queryStatusFlagBit1 = 1;
    dialog.m_queryValueOrTime = 5;
    dialog.m_queryAuxParam = 6;
    dialog.m_queryMaxPlayers = 7;

    g_initFakeApiVtable.Disconnect = FakeInitDisconnect;
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    WestwoodOnlineUpgradeSessionRequest skipped = {};
    WestwoodOnlineUpgradeSessionRequest selected = {};
    skipped.m_rowFlags = 0;
    skipped.m_next = &selected;
    selected.m_rowFlags = 1;
    selected.m_hostIpv4Packed = 0x01020304;
    selected.m_next = 0;

    ResetLaunchSelectedSessionFakes();
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_WestwoodOnlineUpgradeSelectedMissionIndex = -1;
    result = WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession(
        0,
        0,
        0,
        &skipped,
        0
    );
    if (result != 0 ||
        g_threeFloatUpdateDataCount != 1 ||
        g_threeFloatUpdateDataSaveValue[0] != 1 ||
        g_launchInitSessionCalls != 1 ||
        g_launchInitSessionGuid != &g_zNetwork_WestwoodOnlineAppGuid ||
        g_launchFormatIpv4Calls != 1 ||
        g_launchFormatIpv4Packed != 0x01020304 ||
        g_launchSelectTcpCalls != 1 ||
        strcmp(g_launchSelectTcpAddress, "4.3.2.1") != 0 ||
        g_launchSelectTcpSkip != 1 ||
        g_launchSendMessageCalls != 1 ||
        g_launchSendMessageHwnd != (HWND)0x12345678 ||
        g_launchSendMessageMsg != CB_GETCURSEL ||
        g_launchCreateSessionCalls != 1 ||
        g_launchCreateSessionFields.statusFlags != 3 ||
        g_launchCreateSessionFields.eventCode != 3 ||
        g_launchCreateSessionFields.valueOrTime != 5 ||
        g_launchCreateSessionFields.auxParam != 6 ||
        g_launchCreateSessionFields.maxPlayers != 7 ||
        strcmp(g_launchCreateSessionFields.sessionNameBuf, "Arena") != 0 ||
        g_launchSetNetworkEnabledCalls != 1 ||
        g_launchSetNetworkEnabledValue != 1 ||
        g_launchGetPlayerNameCalls != 1 ||
        g_launchGetPlayerNameThis[0] != &dialog ||
        g_launchCreateLocalPlayerCalls != 1 ||
        strcmp(g_launchCreateLocalPlayerName[0], "PlayerOne") != 0 ||
        g_launchSetPlayerNameCalls != 0 ||
        g_launchSetStatusBitsCalls != 1 ||
        g_launchStatusBits != 3 ||
        g_launchTimerCalls != 1 ||
        g_launchTimerThis != &g_HudSensorTracker ||
        g_launchTimerSecondsRaw != LaunchFloatBits(300.0f) ||
        g_launchTimerGoalValue != 6 ||
        g_WestwoodOnlineUpgradeSelectedMissionIndex != 3 ||
        g_initDisconnectCalls != 1)
    {
        failure = 2;
    }

    ResetLaunchSelectedSessionFakes();
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    g_WestwoodOnlineUpgradeSelectedMissionIndex = -1;
    g_launchOpenSelectedOutputFields.statusFlags = 2;
    g_launchOpenSelectedOutputFields.eventCode = 9;
    g_launchOpenSelectedOutputFields.valueOrTime = 2;
    g_launchOpenSelectedOutputFields.auxParam = 11;
    g_launchOpenSelectedOutputFields.maxPlayers = 8;
    strcpy(g_launchOpenSelectedOutputFields.sessionNameBuf, "Joined");
    result = WestwoodOnlineUpgradeApiEventSink::LaunchSelectedSession(
        0,
        0,
        0,
        &selected,
        0
    );
    if (failure == 0 &&
        (result != 0 ||
         g_launchSelectTcpCalls != 1 ||
         g_launchSelectTcpSkip != 0 ||
         g_launchCreateSessionCalls != 0 ||
         g_launchSendMessageCalls != 0 ||
         g_launchOpenSelectedCalls != 1 ||
         g_launchOpenSelectedInputFields.selectedSessionIndex != 0 ||
         g_launchSetNetworkEnabledCalls != 1 ||
         g_launchCreateLocalPlayerCalls != 1 ||
         strcmp(g_launchCreateLocalPlayerName[0], "PlayerOne") != 0 ||
         g_launchSetPlayerNameCalls != 1 ||
         strcmp(g_launchSetPlayerName, "PlayerOne") != 0 ||
         g_launchGetPlayerNameCalls != 2 ||
         g_launchSetStatusBitsCalls != 1 ||
         g_launchStatusBits != 2 ||
         g_launchTimerCalls != 1 ||
         g_launchTimerSecondsRaw != LaunchFloatBits(120.0f) ||
         g_launchTimerGoalValue != 11 ||
         g_WestwoodOnlineUpgradeSelectedMissionIndex != 9 ||
         g_initDisconnectCalls != 1))
    {
        failure = 3;
    }

    dialog.m_sessionName.CString::~CString();
    g_pWestwoodOnlineUpgradeApi = oldApi;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = oldCreateFromQuery;
    g_WestwoodOnlineUpgradeSelectedMissionIndex = oldMissionIndex;
    RestoreImportPatch(sendMessagePatch);
    for (int index = patchCount - 1; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_apply_encoded_query_string0_smoke(void)
{
    CodeFunctionPatch updateDataPatch = {};
    if (!PatchFunctionJump(CWndUpdateDataAddress(),
                           (void *)&FakeWestwoodUpdateData,
                           updateDataPatch))
    {
        return 1;
    }

    g_threeFloatUpdateDataCount = 0;
    int result = WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0(
        0,
        -1,
        0,
        (char *)"ignored"
    );
    if (result != 0 || g_threeFloatUpdateDataCount != 0)
    {
        RestoreFunctionPatch(updateDataPatch);
        return 2;
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
        return 3;
    }

    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"zero");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"one");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"two");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"three");
    SendMessageA(comboBox, CB_SETCURSEL, 0, 0);

    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionModeCombo.m_hWnd = comboBox;
    dialog.m_queryValueOrTime = 0;
    dialog.m_queryAuxParam = 0;
    dialog.m_queryMaxPlayers = 0;
    dialog.m_queryStatusFlagBit0 = 0;
    dialog.m_queryStatusFlagBit1 = 0;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatUpdateDataSaveValue[0] = -1;

    char encodedQuery[64];
    wsprintfA(encodedQuery, "%1d%4d%4d%1d%1d%1d", 3, 42, 321, 5, 0, 1);
    result = WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0(
        0,
        0,
        0,
        encodedQuery
    );

    int failure = 0;
    if (result != 0 ||
        SendMessageA(comboBox, CB_GETCURSEL, 0, 0) != 3 ||
        dialog.m_queryValueOrTime != 42 ||
        dialog.m_queryAuxParam != 321 ||
        dialog.m_queryMaxPlayers != 5 ||
        dialog.m_queryStatusFlagBit0 != 0 ||
        dialog.m_queryStatusFlagBit1 != 1 ||
        g_threeFloatUpdateDataCount != 1 ||
        g_threeFloatUpdateDataSaveValue[0] != 0)
    {
        failure = 4;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    DestroyWindow(comboBox);
    RestoreFunctionPatch(updateDataPatch);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_apply_encoded_query_string1_smoke(void)
{
    CodeFunctionPatch updateDataPatch = {};
    if (!PatchFunctionJump(CWndUpdateDataAddress(),
                           (void *)&FakeWestwoodUpdateData,
                           updateDataPatch))
    {
        return 1;
    }

    g_threeFloatUpdateDataCount = 0;
    int result = WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1(
        0,
        -1,
        0,
        0,
        (char *)"ignored"
    );
    if (result != 0 || g_threeFloatUpdateDataCount != 0)
    {
        RestoreFunctionPatch(updateDataPatch);
        return 2;
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
        return 3;
    }

    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"zero");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"one");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"two");
    SendMessageA(comboBox, CB_ADDSTRING, 0, (LPARAM)"three");
    SendMessageA(comboBox, CB_SETCURSEL, 0, 0);

    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    dialog.m_sessionModeCombo.m_hWnd = comboBox;
    dialog.m_queryValueOrTime = 0;
    dialog.m_queryAuxParam = 0;
    dialog.m_queryMaxPlayers = 0;
    dialog.m_queryStatusFlagBit0 = 0;
    dialog.m_queryStatusFlagBit1 = 0;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatUpdateDataSaveValue[0] = -1;

    char encodedQuery[64];
    wsprintfA(encodedQuery, "%1d%4d%4d%1d%1d%1d", 2, 25, 123, 4, 1, 0);
    result = WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1(
        0,
        0,
        0,
        0,
        encodedQuery
    );

    int failure = 0;
    if (result != 0 ||
        SendMessageA(comboBox, CB_GETCURSEL, 0, 0) != 2 ||
        dialog.m_queryValueOrTime != 25 ||
        dialog.m_queryAuxParam != 123 ||
        dialog.m_queryMaxPlayers != 4 ||
        dialog.m_queryStatusFlagBit0 != 1 ||
        dialog.m_queryStatusFlagBit1 != 0 ||
        g_threeFloatUpdateDataCount != 1 ||
        g_threeFloatUpdateDataSaveValue[0] != 0)
    {
        failure = 4;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    DestroyWindow(comboBox);
    RestoreFunctionPatch(updateDataPatch);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_session_request_status301b_smoke(
    void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump(
            (void *)&zLoc::FormatMessage,
            (void *)&FakeAppendSessionRequestStatus301BFormatMessage,
            patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendSessionRequestStatus301BAppendStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    WestwoodOnlineUpgradeSessionRequest request = {};
    strcpy(request.m_sessionName, "Bravo Session");

    ResetAppendSessionRequestStatus301BFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301B(
            0,
            -1,
            0,
            &request,
            "ignored"
        );
    int failure = 0;
    if (result != 0 ||
        g_appendStatus301BFormatCalls != 0 ||
        g_appendStatus301BAppendCalls != 0)
    {
        failure = 1;
    }

    ResetAppendSessionRequestStatus301BFakes();
    result = WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301B(
        0,
        0,
        0,
        &request,
        "joined"
    );
    if (failure == 0 &&
        (result != 0 ||
         g_appendStatus301BFormatCalls != 1 ||
         g_appendStatus301BFormatBuffer == 0 ||
         g_appendStatus301BFormatMaxChars != 128 ||
         g_appendStatus301BFormatMessageId != 0x301b ||
         g_appendStatus301BFormatSessionName != request.m_sessionName ||
         strcmp(g_appendStatus301BFormatStatusText, "joined") != 0 ||
         g_appendStatus301BAppendCalls != 1 ||
         g_appendStatus301BAppendThis != &dialog ||
         strcmp(g_appendStatus301BAppendText, "formatted 301b status") != 0))
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_session_request_status301c_smoke(
    void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump(
            (void *)&zLoc::FormatMessage,
            (void *)&FakeAppendSessionRequestStatus301CFormatMessage,
            patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendSessionRequestStatus301CAppendStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    WestwoodOnlineUpgradeSessionRequest request = {};
    strcpy(request.m_sessionName, "Charlie Session");

    ResetAppendSessionRequestStatus301CFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C(
            0,
            -1,
            &request,
            "ignored"
        );
    int failure = 0;
    if (result != 0 ||
        g_appendStatus301CFormatCalls != 0 ||
        g_appendStatus301CAppendCalls != 0)
    {
        failure = 1;
    }

    ResetAppendSessionRequestStatus301CFakes();
    result = WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C(
        0,
        0,
        &request,
        "ready"
    );
    if (failure == 0 &&
        (result != 0 ||
         g_appendStatus301CFormatCalls != 1 ||
         g_appendStatus301CFormatBuffer == 0 ||
         g_appendStatus301CFormatMaxChars != 128 ||
         g_appendStatus301CFormatMessageId != 0x301c ||
         g_appendStatus301CFormatSessionName != request.m_sessionName ||
         strcmp(g_appendStatus301CFormatStatusText, "ready") != 0 ||
         g_appendStatus301CAppendCalls != 1 ||
         g_appendStatus301CAppendThis != &dialog ||
         strcmp(g_appendStatus301CAppendText, "formatted 301c status") != 0))
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_session_request_status301c_alt0_smoke(
    void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendSessionRequestStatus301CAltAppendStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    WestwoodOnlineUpgradeSessionRequest sessionRequest = {};
    strcpy(sessionRequest.m_sessionName, "Alt301C");
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    ResetAppendSessionRequestStatus301CAltFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt0(
            0,
            -1,
            &sessionRequest,
            42
        );
    int failure = 0;
    if (result != 0 ||
        g_initMessageIdCalls != 0 ||
        g_appendStatus301CAltAppendCalls != 0)
    {
        failure = 1;
    }

    ResetAppendSessionRequestStatus301CAltFakes();
    result =
        WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt0(
            0,
            0,
            &sessionRequest,
            42
        );
    if (failure == 0 &&
        (result != 0 ||
         g_initMessageIdCalls != 1 ||
         g_initMessageIds[0] != 0x301c ||
         g_appendStatus301CAltAppendCalls != 1 ||
         g_appendStatus301CAltAppendThis != &dialog ||
         strcmp(g_appendStatus301CAltAppendFormat, "msg-301c") != 0 ||
         g_appendStatus301CAltAppendSessionName != sessionRequest.m_sessionName ||
         g_appendStatus301CAltAppendValue != 42))
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_session_request_status301c_alt1_smoke(
    void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendSessionRequestStatus301CAltAppendStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    WestwoodOnlineUpgradeSessionRequest sessionRequest = {};
    strcpy(sessionRequest.m_sessionName, "Alt301C1");
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    ResetAppendSessionRequestStatus301CAltFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt1(
            0,
            -1,
            77,
            &sessionRequest,
            84
        );
    int failure = 0;
    if (result != 0 ||
        g_initMessageIdCalls != 0 ||
        g_appendStatus301CAltAppendCalls != 0)
    {
        failure = 1;
    }

    ResetAppendSessionRequestStatus301CAltFakes();
    result =
        WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301C_Alt1(
            0,
            0,
            77,
            &sessionRequest,
            84
        );
    if (failure == 0 &&
        (result != 0 ||
         g_initMessageIdCalls != 1 ||
         g_initMessageIds[0] != 0x301c ||
         g_appendStatus301CAltAppendCalls != 1 ||
         g_appendStatus301CAltAppendThis != &dialog ||
         strcmp(g_appendStatus301CAltAppendFormat, "msg-301c") != 0 ||
         g_appendStatus301CAltAppendSessionName != sessionRequest.m_sessionName ||
         g_appendStatus301CAltAppendValue != 84))
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_callback_no_op0_smoke(void)
{
    return WestwoodOnlineUpgradeApiEventSink::CallbackNoOp0(
        (void *)0x1234,
        -1,
        23,
        45
    );
}


extern "C" int
westwood_online_upgrade_api_event_sink_callback_no_op1_smoke(void)
{
    return WestwoodOnlineUpgradeApiEventSink::CallbackNoOp1(
        (void *)0x5678,
        -1,
        23
    );
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_time_status302a_smoke(void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeAppendTimeStatusFormatMessage,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendConnectStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    const long unixTime = 60 * 60 * 24;
    const time_t expectedUnixTime = (time_t)unixTime;
    const char *const expectedTimeText = ctime(&expectedUnixTime);

    ResetAppendTimeStatusFakes();
    int result = WestwoodOnlineUpgradeApiEventSink::AppendTimeStatus302A(
        0,
        -1,
        unixTime
    );
    int failure = 0;
    if (result != 0 ||
        g_appendTimeStatusFormatCalls != 0 ||
        g_appendConnectStatusCalls != 0)
    {
        failure = 1;
    }

    ResetAppendTimeStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::AppendTimeStatus302A(
        0,
        0,
        unixTime
    );
    if (failure == 0 &&
        (result != 0 ||
         g_appendTimeStatusFormatCalls != 1 ||
         g_appendTimeStatusFormatBuffer == 0 ||
         g_appendTimeStatusFormatMaxChars != 128 ||
         g_appendTimeStatusFormatMessageId != 0x302a ||
         strcmp(g_appendTimeStatusFormatTimeText, expectedTimeText) != 0 ||
         g_appendConnectStatusCalls != 1 ||
         g_appendConnectStatusThis[0] != &dialog ||
         strcmp(g_appendConnectStatusText[0], "formatted time status") != 0))
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_value_status302b_302c_smoke(void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeAppendValueStatusFormatMessage,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendConnectStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    const int usePrimaryValues[2] = {1, 0};
    const unsigned int expectedMessageIds[2] = {0x302b, 0x302c};
    int failure = 0;

    for (int index = 0; index < 2; ++index)
    {
        ResetAppendValueStatusFakes();
        int result =
            WestwoodOnlineUpgradeApiEventSink::AppendValueStatus302B_302C(
                0,
                -1,
                77 + index,
                usePrimaryValues[index]
            );
        if (result != 0 ||
            g_appendValueStatusFormatCalls != 1 ||
            g_appendValueStatusFormatBuffer == 0 ||
            g_appendValueStatusFormatMaxChars != 128 ||
            g_appendValueStatusFormatMessageId != expectedMessageIds[index] ||
            g_appendValueStatusFormatValue != 77 + index ||
            g_appendConnectStatusCalls != 1 ||
            g_appendConnectStatusThis[0] != &dialog ||
            strcmp(g_appendConnectStatusText[0], "formatted value status") != 0)
        {
            failure = index + 1;
            break;
        }
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_update_session_result_item_flags_smoke(
    void)
{
    const WORD kMfc42CWndSendDlgItemMessageAOrdinal = 5802;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    ImportFunctionPatch import = {};

    if (!PatchImportByOrdinal("MFC42.DLL",
                              kMfc42CWndSendDlgItemMessageAOrdinal,
                              (void *)&FakePendingSessionRemovedSendDlgItemMessageA,
                              import))
    {
        RestoreImportPatch(import);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    int failure = 0;

    ResetPendingSessionRemovedFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags(
            0,
            -1,
            "Missing",
            7,
            99
        );
    if (result != 0 || g_pendingRemovedSendMessageCalls != 1 ||
        g_pendingRemovedSendMessageThis[0] != (CWnd *)&dialog ||
        g_pendingRemovedSendMessageControlId[0] != 1137 ||
        g_pendingRemovedSendMessageMessage[0] != LB_FINDSTRING ||
        g_pendingRemovedSendMessageWParam[0] != (WPARAM)-1 ||
        strcmp(g_pendingRemovedSendMessageText[0], "Missing") != 0)
    {
        failure = 1;
    }

    ResetPendingSessionRemovedFakes();
    g_pendingRemovedSendMessageResult[0] = 3;
    result = WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags(
        0,
        0,
        "Alpha",
        1,
        123
    );
    if (failure == 0 &&
        (result != 0 || g_pendingRemovedSendMessageCalls != 5 ||
         g_pendingRemovedSendMessageMessage[0] != LB_FINDSTRING ||
         g_pendingRemovedSendMessageWParam[0] != (WPARAM)-1 ||
         strcmp(g_pendingRemovedSendMessageText[0], "Alpha") != 0 ||
         g_pendingRemovedSendMessageMessage[1] != LB_GETITEMDATA ||
         g_pendingRemovedSendMessageWParam[1] != 3 ||
         g_pendingRemovedSendMessageLParam[1] != 0 ||
         g_pendingRemovedSendMessageMessage[2] != LB_SETITEMDATA ||
         g_pendingRemovedSendMessageWParam[2] != 3 ||
         g_pendingRemovedSendMessageLParam[2] != 1 ||
         g_pendingRemovedSendMessageMessage[3] != LB_DELETESTRING ||
         g_pendingRemovedSendMessageWParam[3] != 3 ||
         g_pendingRemovedSendMessageLParam[3] != 0 ||
         g_pendingRemovedSendMessageMessage[4] != LB_ADDSTRING ||
         g_pendingRemovedSendMessageWParam[4] != 0 ||
         strcmp(g_pendingRemovedSendMessageText[4], "Alpha + *") != 0))
    {
        failure = 2;
    }

    ResetPendingSessionRemovedFakes();
    g_pendingRemovedSendMessageResult[0] = 2;
    result = WestwoodOnlineUpgradeApiEventSink::UpdateSessionResultItemFlags(
        0,
        0,
        "Beta",
        2,
        456
    );
    if (failure == 0 &&
        (result != 0 || g_pendingRemovedSendMessageCalls != 5 ||
         g_pendingRemovedSendMessageMessage[2] != LB_SETITEMDATA ||
         g_pendingRemovedSendMessageWParam[2] != 2 ||
         g_pendingRemovedSendMessageLParam[2] != 2 ||
         strcmp(g_pendingRemovedSendMessageText[4], "Beta *") != 0))
    {
        failure = 3;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreImportPatch(import);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_session_request_status301d_smoke(
    void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump(
            (void *)&zLoc::FormatMessage,
            (void *)&FakeAppendSessionRequestStatus301DFormatMessage,
            patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendSessionRequestStatus301DAppendStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    WestwoodOnlineUpgradeSessionRequest request = {};
    strcpy(request.m_sessionName, "Delta Session");

    ResetAppendSessionRequestStatus301DFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::AppendSessionRequestStatus301D(
            0,
            -1,
            &request,
            "launched"
        );
    int failure = 0;
    if (result != 0 ||
        g_appendStatus301DFormatCalls != 1 ||
        g_appendStatus301DFormatBuffer == 0 ||
        g_appendStatus301DFormatMaxChars != 128 ||
        g_appendStatus301DFormatMessageId != 0x301d ||
        g_appendStatus301DFormatSessionName != request.m_sessionName ||
        strcmp(g_appendStatus301DFormatStatusText, "launched") != 0 ||
        g_appendStatus301DAppendCalls != 1 ||
        g_appendStatus301DAppendThis != &dialog ||
        strcmp(g_appendStatus301DAppendText, "formatted 301d status") != 0)
    {
        failure = 1;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_connect_status301e_3021_smoke(
    void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendConnectStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    const int statusCodes[4] = {0, 0x40134, 0x40133, -1};
    const unsigned int expectedMessageIds[4] = {0x301e, 0x301f, 0x3020, 0x3021};
    int failure = 0;

    for (int index = 0; index < 4; ++index)
    {
        ResetAppendConnectStatusFakes();
        int result =
            WestwoodOnlineUpgradeApiEventSink::AppendConnectStatus301E_3021(
                0,
                statusCodes[index]
            );
        char expectedText[64];
        wsprintfA(expectedText, "msg-%04x", expectedMessageIds[index]);
        if (result != 0 ||
            g_initMessageIdCalls != 1 ||
            g_initMessageIds[0] != expectedMessageIds[index] ||
            g_appendConnectStatusCalls != 1 ||
            g_appendConnectStatusThis[0] != &dialog ||
            strcmp(g_appendConnectStatusText[0], expectedText) != 0)
        {
            failure = index + 1;
            break;
        }
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_browse_record_status3022_3025_smoke(
    void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[3] = {};

    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeAppendBrowseRecordStatusFormatMessage,
                           patches[0]) ||
        !PatchFunctionJump((void *)&zLoc::GetMessageString,
                           (void *)&FakeInitGetMessageString,
                           patches[1]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendConnectStatusTextFmt,
            patches[2]))
    {
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

    WestwoodOnlineUpgradeBrowseRecord browseRecord = {};
    strcpy(browseRecord.m_sessionName, "Echo Session");

    ResetAppendBrowseRecordStatusFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::AppendBrowseRecordStatus3022_3025(
            0,
            -1,
            &browseRecord
        );
    int failure = 0;
    if (result != 0 ||
        g_appendBrowseRecordStatusFormatCalls != 1 ||
        g_appendBrowseRecordStatusFormatBuffer == 0 ||
        g_appendBrowseRecordStatusFormatMaxChars != 128 ||
        g_appendBrowseRecordStatusFormatMessageId != 0x3022 ||
        g_appendBrowseRecordStatusFormatSessionName !=
            browseRecord.m_sessionName ||
        g_initMessageIdCalls != 0 ||
        g_appendConnectStatusCalls != 1 ||
        g_appendConnectStatusThis[0] != &dialog ||
        strcmp(g_appendConnectStatusText[0],
               "formatted browse record status") != 0)
    {
        failure = 1;
    }

    const int statusCodes[4] = {0x40131, 0x40130, 0x40132, -1};
    const unsigned int expectedMessageIds[4] = {0x3023, 0x3020, 0x3024, 0x3025};
    for (int index = 0; failure == 0 && index < 4; ++index)
    {
        ResetAppendBrowseRecordStatusFakes();
        result =
            WestwoodOnlineUpgradeApiEventSink::AppendBrowseRecordStatus3022_3025(
                0,
                statusCodes[index],
                0
            );
        char expectedText[64];
        wsprintfA(expectedText, "msg-%04x", expectedMessageIds[index]);
        if (result != 0 ||
            g_appendBrowseRecordStatusFormatCalls != 0 ||
            g_initMessageIdCalls != 1 ||
            g_initMessageIds[0] != expectedMessageIds[index] ||
            g_appendConnectStatusCalls != 1 ||
            g_appendConnectStatusThis[0] != &dialog ||
            strcmp(g_appendConnectStatusText[0], expectedText) != 0)
        {
            failure = index + 2;
        }
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    for (int index = 2; index >= 0; --index)
    {
        RestoreFunctionPatch(patches[index]);
    }
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_append_value_status3026_smoke(void)
{
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeAppendValueStatusFormatMessage,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendConnectStatusTextFmt,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    ResetAppendValueStatusFakes();
    int result = WestwoodOnlineUpgradeApiEventSink::AppendValueStatus3026(
        0,
        -1,
        77
    );
    int failure = 0;
    if (result != 0 ||
        g_appendValueStatusFormatCalls != 0 ||
        g_appendConnectStatusCalls != 0)
    {
        failure = 1;
    }

    ResetAppendValueStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::AppendValueStatus3026(
        0,
        0,
        77
    );
    if (failure == 0 &&
        (result != 0 ||
         g_appendValueStatusFormatCalls != 1 ||
         g_appendValueStatusFormatBuffer == 0 ||
         g_appendValueStatusFormatMaxChars != 128 ||
         g_appendValueStatusFormatMessageId != 0x3026 ||
         g_appendValueStatusFormatValue != 77 ||
         g_appendConnectStatusCalls != 1 ||
         g_appendConnectStatusThis[0] != &dialog ||
         strcmp(g_appendConnectStatusText[0], "formatted value status") != 0))
    {
        failure = 2;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_on_network_status_changed_smoke(void)
{
    const int oldAsyncError = g_WestwoodOnlineUpgradeApiAsyncErrorFlag;
    const int oldAbortFlag = g_WestwoodOnlineUpgradeAbortFlag;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch patches[2] = {};

    if (!PatchFunctionJump((void *)&zGame::ReturnOnlyStub,
                           (void *)&FakeNetworkStatusReturnOnlyStub,
                           patches[0]) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::SetAbortAndClose),
            (void *)&FakeInitDialogSetAbortAndClose,
            patches[1]))
    {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    int failure = 0;

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    g_WestwoodOnlineUpgradeAbortFlag = 0;
    ResetNetworkStatusFakes();
    int result = WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged(
        0,
        (int)0x80040069
    );
    if (result != 0 ||
        g_networkStatusReturnOnlyCalls != 1 ||
        g_initDialogSetAbortCalls != 0 ||
        g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 1)
    {
        failure = 1;
    }

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    g_WestwoodOnlineUpgradeAbortFlag = 0;
    ResetNetworkStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged(
        0,
        (int)0x80040068
    );
    if (failure == 0 &&
        (result != 0 ||
         g_networkStatusReturnOnlyCalls != 1 ||
         g_initDialogSetAbortCalls != 0 ||
         g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0))
    {
        failure = 2;
    }

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    g_WestwoodOnlineUpgradeAbortFlag = 0;
    ResetNetworkStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged(
        0,
        0x4012f
    );
    if (failure == 0 &&
        (result != 0 ||
         g_networkStatusReturnOnlyCalls != 1 ||
         g_initDialogSetAbortCalls != 1 ||
         g_initDialogSetAbortThis != &dialog ||
         g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0))
    {
        failure = 3;
    }

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    g_WestwoodOnlineUpgradeAbortFlag = 1;
    ResetNetworkStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged(
        0,
        0x4012f
    );
    if (failure == 0 &&
        (result != 0 ||
         g_networkStatusReturnOnlyCalls != 1 ||
         g_initDialogSetAbortCalls != 0 ||
         g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0))
    {
        failure = 4;
    }

    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = 0;
    g_WestwoodOnlineUpgradeAbortFlag = 0;
    ResetNetworkStatusFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnNetworkStatusChanged(
        0,
        0x4012d
    );
    if (failure == 0 &&
        (result != 0 ||
         g_networkStatusReturnOnlyCalls != 1 ||
         g_initDialogSetAbortCalls != 0 ||
         g_WestwoodOnlineUpgradeApiAsyncErrorFlag != 0))
    {
        failure = 5;
    }

    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_WestwoodOnlineUpgradeApiAsyncErrorFlag = oldAsyncError;
    g_WestwoodOnlineUpgradeAbortFlag = oldAbortFlag;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_on_bootstrap_server_list_smoke(void)
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
    int const oldSelectedMissionIndex =
        g_WestwoodOnlineUpgradeSelectedMissionIndex;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};

    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
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
        WestwoodOnlineUpgradeApiEventSink::OnBootstrapServerList(0, -5, &serverA);
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
    result = WestwoodOnlineUpgradeApiEventSink::OnBootstrapServerList(0, 0, 0);
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
    g_WestwoodOnlineUpgradeSelectedMissionIndex = 0x12345678;
    g_bootstrapSetEventCalls = 0;
    result =
        WestwoodOnlineUpgradeApiEventSink::OnBootstrapServerList(0, 0, &serverA);
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
         g_WestwoodOnlineUpgradeSelectedMissionIndex != 0x12345678))
    {
        failure = 4;
    }

    dialog.m_selectedProfileConnectString.CString::~CString();
    dialog.m_selectedProfilePlayerName.CString::~CString();
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_WestwoodOnlineUpgradeFailureEvent = oldFailureEvent;
    g_WestwoodOnlineUpgradeInitWaitEvents[0] = oldInitWaitEvent0;
    g_WestwoodOnlineUpgradeSelectedBootstrapServer = oldSelected;
    g_WestwoodOnlineUpgradeSelectedMissionIndex = oldSelectedMissionIndex;
    RestoreImportPatch(setEventPatch);
    return failure;
}


extern "C" int
westwood_online_upgrade_api_event_sink_on_browse_record_list_received_smoke(
    void)
{
    const int oldCachedBrowseRecordListCount =
        g_WestwoodOnlineUpgradeCachedBrowseRecordListCount;
    WestwoodOnlineUpgradeBrowseRecord oldListRecords[10];
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    CodeFunctionPatch formatPatch = {};
    ImportFunctionPatch sendMessagePatch = {};

    for (int index = 0; index < 10; ++index)
    {
        oldListRecords[index] = g_WestwoodOnlineUpgradeCachedBrowseRecordList[index];
    }

    if (!PatchFunctionJump((void *)&zLoc::FormatMessage,
                           (void *)&FakeBrowseRecordListFormatMessage,
                           formatPatch) ||
        !PatchImportByOrdinal("MFC42.DLL",
                              5802,
                              (void *)&FakePendingSessionRemovedSendDlgItemMessageA,
                              sendMessagePatch))
    {
        RestoreImportPatch(sendMessagePatch);
        RestoreFunctionPatch(formatPatch);
        return 90;
    }

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog =
        *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    g_pWestwoodOnlineUpgradeDialog = &dialog;

    WestwoodOnlineUpgradeBrowseRecord records[10] = {};
    const char *recordNames[10] = {"Open",
                                   "Unknown",
                                   "L250",
                                   "L500",
                                   "L750",
                                   "L1000",
                                   "L1250",
                                   "L1500",
                                   "L1750",
                                   "L1751"};
    const int latencies[9] = {-1, 250, 251, 501, 751, 1001, 1251, 1501, 1751};
    const char *expectedLatencyText[9] = {"??",
                                          " | ",
                                          " ||| ",
                                          " ||||| ",
                                          " ||||||| ",
                                          " ||||||||| ",
                                          " ||||||||||| ",
                                          " ||||||||||||| ",
                                          " ||||||||||||||| "};

    records[0].m_recordFlags = 0;
    records[0].m_displayMetric0 = 20;
    records[0].m_displayMetric1 = 10;
    strcpy(records[0].m_sessionName, recordNames[0]);
    records[0].m_next = &records[1];
    for (int index = 1; index < 10; ++index)
    {
        records[index].m_recordFlags = 1;
        records[index].m_displayMetric0 = 200 + index;
        records[index].m_displayMetric1 = 100 + index;
        records[index].m_latencyMs = latencies[index - 1];
        strcpy(records[index].m_sessionName, recordNames[index]);
        records[index].m_next = index == 9 ? 0 : &records[index + 1];
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = 7;
    ResetBrowseRecordListFakes();
    int result =
        WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordListReceived(
            0,
            -1,
            &records[0]
        );
    int failure = 0;
    if (result != 0 ||
        g_WestwoodOnlineUpgradeCachedBrowseRecordListCount != 0 ||
        g_browseRecordListFormatCalls != 0 ||
        g_pendingRemovedSendMessageCalls != 0)
    {
        failure = 1;
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecordListCount = 55;
    ResetBrowseRecordListFakes();
    result = WestwoodOnlineUpgradeApiEventSink::OnBrowseRecordListReceived(
        0,
        0,
        &records[0]
    );
    if (failure == 0 &&
        (result != 0 ||
         g_browseRecordListFormatCalls != 10 ||
         g_pendingRemovedSendMessageCalls != 11 ||
         g_WestwoodOnlineUpgradeCachedBrowseRecordListCount != 10))
    {
        failure = 2;
    }

    if (failure == 0 &&
        (g_pendingRemovedSendMessageThis[0] != (CWnd *)&dialog ||
         g_pendingRemovedSendMessageControlId[0] != 1136 ||
         g_pendingRemovedSendMessageMessage[0] != LB_RESETCONTENT ||
         g_pendingRemovedSendMessageWParam[0] != 0 ||
         g_pendingRemovedSendMessageLParam[0] != 0))
    {
        failure = 3;
    }

    if (failure == 0 &&
        (g_browseRecordListFormatMaxChars[0] != 256 ||
         g_browseRecordListFormatMessageId[0] != 0x3027 ||
         g_browseRecordListFormatSessionName[0] != records[0].m_sessionName ||
         g_browseRecordListFormatMetric1[0] != 10 ||
         strcmp(g_pendingRemovedSendMessageText[1], "Open:3027:10") != 0))
    {
        failure = 4;
    }

    for (int index = 1; failure == 0 && index < 10; ++index)
    {
        char expectedRow[128];
        wsprintfA(expectedRow,
                  "%s:%04x:%d:%d:%s",
                  recordNames[index],
                  0x3028,
                  100 + index,
                  200 + index,
                  expectedLatencyText[index - 1]);
        if (g_browseRecordListFormatBuffer[index] == 0 ||
            g_browseRecordListFormatMaxChars[index] != 256 ||
            g_browseRecordListFormatMessageId[index] != 0x3028 ||
            g_browseRecordListFormatSessionName[index] !=
                records[index].m_sessionName ||
            g_browseRecordListFormatMetric0[index] != 200 + index ||
            g_browseRecordListFormatMetric1[index] != 100 + index ||
            strcmp(g_browseRecordListFormatLatencyText[index],
                   expectedLatencyText[index - 1]) != 0 ||
            g_pendingRemovedSendMessageControlId[index + 1] != 1136 ||
            g_pendingRemovedSendMessageMessage[index + 1] != LB_INSERTSTRING ||
            g_pendingRemovedSendMessageWParam[index + 1] != (WPARAM)-1 ||
            strcmp(g_pendingRemovedSendMessageText[index + 1],
                   expectedRow) != 0)
        {
            failure = index + 10;
        }
    }

    for (int index = 0; failure == 0 && index < 10; ++index)
    {
        if (strcmp(g_WestwoodOnlineUpgradeCachedBrowseRecordList[index]
                       .m_sessionName,
                   recordNames[index]) != 0 ||
            g_WestwoodOnlineUpgradeCachedBrowseRecordList[index]
                    .m_recordFlags != records[index].m_recordFlags ||
            g_WestwoodOnlineUpgradeCachedBrowseRecordList[index]
                    .m_displayMetric0 != records[index].m_displayMetric0 ||
            g_WestwoodOnlineUpgradeCachedBrowseRecordList[index]
                    .m_displayMetric1 != records[index].m_displayMetric1 ||
            g_WestwoodOnlineUpgradeCachedBrowseRecordList[index]
                    .m_latencyMs != records[index].m_latencyMs ||
            g_WestwoodOnlineUpgradeCachedBrowseRecordList[index].m_next != 0)
        {
            failure = index + 30;
        }
    }

    for (int index = 0; index < 10; ++index)
    {
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[index] =
            oldListRecords[index];
    }
    g_WestwoodOnlineUpgradeCachedBrowseRecordListCount =
        oldCachedBrowseRecordListCount;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    RestoreImportPatch(sendMessagePatch);
    RestoreFunctionPatch(formatPatch);
    return failure;
}

