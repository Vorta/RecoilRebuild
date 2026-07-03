#include "Battlesport/wol_config_dialog.h"
#include "Battlesport/wol_api_event_sink.h"
#include "Battlesport/wol_api.h"
#include "Battlesport/cz_recoil_frame.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/wol_dialog.h"
#include "Battlesport/wol_ref_count_and_lock.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/wwonline/wol_download.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <ocidl.h>
#include <stdarg.h>
#include <string.h>

extern int g_threeFloatDefaultCount;
extern long g_threeFloatDefaultReturn;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" HWND g_RecoilApp_hWndMain;

namespace {

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

struct InitFakeApiVtable {
    void *QueryInterface;
    void *AddRef;
    void *Release;
    void(STDMETHODCALLTYPE *ProcessCallbacks)(IUnknown *self);
    void(STDMETHODCALLTYPE *BeginConnect)(
        IUnknown *self,
        int languageId,
        int productId,
        const char *playerName,
        const char *connectString,
        int timeoutSeconds
    );
    void(STDMETHODCALLTYPE *RequestBootstrapServerList)(
        IUnknown *self,
        WestwoodOnlineUpgradeBootstrapServerRecord *selectedBootstrapServer,
        int timeoutSeconds,
        int useAlternateConnectString
    );
    void(STDMETHODCALLTYPE *RequestListMode)(IUnknown *self, int listMode, int enabled);
    int(STDMETHODCALLTYPE *SubmitQueryRequest)(
        IUnknown *self,
        WestwoodOnlineUpgradeQueryRequest *request
    );
    int(STDMETHODCALLTYPE *LoadBrowseRecord)(
        IUnknown *self,
        WestwoodOnlineUpgradeBrowseRecord *record
    );
    void(STDMETHODCALLTYPE *ResetQueryState)(IUnknown *self);
    void(STDMETHODCALLTYPE *CancelPendingSessionFlow)(IUnknown *self);
    void(STDMETHODCALLTYPE *SubmitStatusText)(IUnknown *self, const char *statusText);
    void(STDMETHODCALLTYPE *SubmitSessionRequestListAndStatusText)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *sessionRequestList,
        const char *statusText
    );
    void(STDMETHODCALLTYPE *Disconnect)(IUnknown *self);
    void *reserved038;
    void(STDMETHODCALLTYPE *SubmitEncodedQueryString)(
        IUnknown *self,
        const char *encodedQuery
    );
    void *reserved040;
    void *reserved044;
    void(STDMETHODCALLTYPE *SubmitPendingSessionList)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *sessionRequestList
    );
    void *reserved04c;
    void *reserved050;
    void(STDMETHODCALLTYPE *QueueSessionRequest)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *request
    );
    void(STDMETHODCALLTYPE *RequestSessionDetails)(
        IUnknown *self,
        WestwoodOnlineUpgradeSessionRequest *request
    );
    void *reserved05c;
    int(STDMETHODCALLTYPE *RequestUpgradeDownloadReadyResult)(
        IUnknown *self,
        WestwoodOnlineUpgradeConnectContext *context
    );
    int(STDMETHODCALLTYPE *QueryStatusWithTokenAndServer)(
        IUnknown *self,
        WestwoodOnlineUpgradeConnectContext *context,
        const char *serverText
    );
    void *reserved068;
    void(STDMETHODCALLTYPE *BeginConnectWithPreparedContext)(
        IUnknown *self,
        WestwoodOnlineUpgradeConnectContext *context,
        int mode
    );
    int(STDMETHODCALLTYPE *PrepareConnectContextAndMode)(
        IUnknown *self,
        WestwoodOnlineUpgradeConnectContext *context
    );
    void(STDMETHODCALLTYPE *SetQueryMode)(IUnknown *self, int listMode);
    void *reserved078;
    void *reserved07c;
    void(STDMETHODCALLTYPE *LookupBrowseRecordBySessionName)(
        IUnknown *self,
        const char *sessionName,
        int lookupMode
    );
    void *reserved084[3];
    int(STDMETHODCALLTYPE *LoadConnectProfileStrings)(
        IUnknown *self,
        int profileId,
        char **playerNameOut,
        char **connectStringOut
    );
    int(STDMETHODCALLTYPE *SaveConnectProfileStrings)(
        IUnknown *self,
        int profileId,
        const char *playerName,
        const char *connectString,
        int connectStringMode
    );
    void(STDMETHODCALLTYPE *GetQueryResultCount)(IUnknown *self, int *outCount);
};

struct InitFakeApiObject {
    InitFakeApiVtable *vftable;
};

struct FakeDownloadUnknownVtable {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IUnknown *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IUnknown *);
    ULONG(STDMETHODCALLTYPE *Release)(IUnknown *);
};

struct FakeConnectionPointContainerVtable {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IConnectionPointContainer *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IConnectionPointContainer *);
    ULONG(STDMETHODCALLTYPE *Release)(IConnectionPointContainer *);
    HRESULT(STDMETHODCALLTYPE *EnumConnectionPoints)(IConnectionPointContainer *, void **);
    HRESULT(STDMETHODCALLTYPE *FindConnectionPoint)(
        IConnectionPointContainer *,
        REFIID,
        IConnectionPoint **
    );
};

struct FakeConnectionPointVtable {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IConnectionPoint *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IConnectionPoint *);
    ULONG(STDMETHODCALLTYPE *Release)(IConnectionPoint *);
    HRESULT(STDMETHODCALLTYPE *GetConnectionInterface)(IConnectionPoint *, IID *);
    HRESULT(STDMETHODCALLTYPE *GetConnectionPointContainer)(
        IConnectionPoint *,
        IConnectionPointContainer **
    );
    HRESULT(STDMETHODCALLTYPE *Advise)(IConnectionPoint *, IUnknown *, DWORD *);
    HRESULT(STDMETHODCALLTYPE *Unadvise)(IConnectionPoint *, DWORD);
    HRESULT(STDMETHODCALLTYPE *EnumConnections)(IConnectionPoint *, void **);
};

struct FakeDownloadUnknown {
    FakeDownloadUnknownVtable *vftable;
};

struct FakeConnectionPointContainer {
    FakeConnectionPointContainerVtable *vftable;
};

struct FakeConnectionPoint {
    FakeConnectionPointVtable *vftable;
};

FakeConnectionPointContainer g_fakeDownloadCpc;
FakeConnectionPoint g_fakeDownloadConnectionPoint;
FakeDownloadUnknown g_fakeDownloadUnknown;
InitFakeApiObject g_initFakeApi;
InitFakeApiVtable g_initFakeApiVtable;
int g_fakeDownloadCoCreateCalls;
bool g_fakeDownloadCoCreateArgsOk;
HRESULT g_fakeDownloadCoCreateResult;
IUnknown *g_fakeDownloadCoCreateObject;
int g_fakeDownloadFindConnectionPointCalls;
bool g_fakeDownloadIidOk;
int g_fakeDownloadAdviseCalls;
IUnknown *g_fakeDownloadAdviseSink;
DWORD g_fakeDownloadAdviseCookie;
int g_fakeDownloadConnectionPointReleaseCalls;
int g_fakeDownloadCpcReleaseCalls;
int g_fakeDownloadSourceReleaseCalls;
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
int g_initSleepCalls;
DWORD g_initSleepDurations[4];
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
unsigned int g_initMessageIds[16];
int g_initMessageIdCalls;
int g_initDialogBaseOnInitCalls;
CDialog *g_initDialogBaseOnInitThis;
int g_initDialogSendMessageCalls;
HWND g_initDialogSendMessageHwnd[24];
UINT g_initDialogSendMessageMsg[24];
WPARAM g_initDialogSendMessageWParam[24];
LPARAM g_initDialogSendMessageLParam[24];
int g_initDialogComboAddCalls;
int g_initProcessCallbacksCalls;
int g_initRequestListModeCalls;
int g_initRequestListMode;
int g_initRequestListModeEnabled;
int g_showModalSelectedMissionIndex;
int g_showModalMenuStep;
int g_showModalMenuVisible[2];
void *g_showModalMenuThis[2];
int g_showModalDoModalCalls;
int g_showModalDialogDtorCalls;
int g_showModalCStringDtorCalls;
int g_showModalListDtorCalls;
int g_showModalEditDtorCalls;
int g_showModalComboDtorCalls;
int g_showModalButtonDtorCalls;
bool g_showModalArgsOk;
int g_modalTimeTickCalls;
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
int g_appendConnectAppendCalls;
WestwoodOnlineUpgradeDialog *g_appendConnectAppendThis;
const char *g_appendConnectAppendText;
int g_queryStatusSetWindowTextCalls;
void *g_queryStatusSetWindowTextThis[4];
char g_queryStatusSetWindowTextValue[4][8];
int g_queueSessionRequestCalls;
WestwoodOnlineUpgradeSessionRequest g_queueSessionRequestCopies[8];
IUnknown *g_queueSessionRequestSelf[8];
int g_lookupBrowseRecordCalls;
IUnknown *g_lookupBrowseRecordSelf[8];
char g_lookupBrowseRecordSessionName[8][0x34];
int g_lookupBrowseRecordMode[8];
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

void *TestObjectVtable(void *object) {
    return *(void **)object;
}

bool TestMfcWindowConstructed(CWnd &wnd) {
    return *(void **)&wnd != 0 && wnd.m_hWnd == 0;
}

template <typename Method>
ULONG_PTR MemberPointerBits(Method method) {
    ULONG_PTR bits = 0;
    memcpy(&bits, &method, sizeof(method));
    return bits;
}

ULONG_PTR MsgMapEntryHandlerBits(const AFX_MSGMAP_ENTRY &entry) {
    ULONG_PTR bits = 0;
    memcpy(&bits, &entry.pfn, sizeof(entry.pfn));
    return bits;
}

template <typename Method>
void *MethodAddress(Method method) {
    union {
        Method method;
        void *address;
    } value = {method};
    return value.address;
}

class WestwoodCWndAccess : public CWnd {
  public:
    using CWnd::Default;
    using CWnd::SetWindowText;
};

void *CWndDefaultAddress() {
    return MethodAddress(&WestwoodCWndAccess::Default);
}

void *CWndSetWindowTextAAddress() {
    return MethodAddress(&WestwoodCWndAccess::SetWindowText);
}

HRESULT STDMETHODCALLTYPE FakeDownloadUnknownQueryInterface(
    IUnknown *,
    REFIID iid,
    void **out
) {
    if (IsEqualGUID(iid, IID_IConnectionPointContainer)) {
        *out = &g_fakeDownloadCpc;
        return S_OK;
    }

    *out = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FakeDownloadUnknownAddRef(IUnknown *) {
    return 2;
}

ULONG STDMETHODCALLTYPE FakeDownloadUnknownRelease(IUnknown *) {
    ++g_fakeDownloadSourceReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE FakeDownloadCpcQueryInterface(
    IConnectionPointContainer *,
    REFIID,
    void **out
) {
    *out = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FakeDownloadCpcAddRef(IConnectionPointContainer *) {
    return 2;
}

ULONG STDMETHODCALLTYPE FakeDownloadCpcRelease(IConnectionPointContainer *) {
    ++g_fakeDownloadCpcReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE FakeDownloadEnumConnectionPoints(
    IConnectionPointContainer *,
    void **
) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE FakeDownloadFindConnectionPoint(
    IConnectionPointContainer *,
    REFIID iid,
    IConnectionPoint **out
) {
    ++g_fakeDownloadFindConnectionPointCalls;
    g_fakeDownloadIidOk = IsEqualGUID(iid, IID_WestwoodOnlineUpgradeDownloadEventSink) != 0;
    *out = (IConnectionPoint *)&g_fakeDownloadConnectionPoint;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FakeDownloadCpQueryInterface(
    IConnectionPoint *,
    REFIID,
    void **out
) {
    *out = 0;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FakeDownloadCpAddRef(IConnectionPoint *) {
    return 2;
}

ULONG STDMETHODCALLTYPE FakeDownloadCpRelease(IConnectionPoint *) {
    ++g_fakeDownloadConnectionPointReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE FakeDownloadGetConnectionInterface(IConnectionPoint *, IID *) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE FakeDownloadGetConnectionPointContainer(
    IConnectionPoint *,
    IConnectionPointContainer **
) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE FakeDownloadAdvise(
    IConnectionPoint *,
    IUnknown *sink,
    DWORD *cookie
) {
    ++g_fakeDownloadAdviseCalls;
    g_fakeDownloadAdviseSink = sink;
    if (cookie != 0) {
        *cookie = 0x87654321;
        g_fakeDownloadAdviseCookie = *cookie;
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FakeDownloadUnadvise(IConnectionPoint *, DWORD) {
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FakeDownloadEnumConnections(IConnectionPoint *, void **) {
    return E_NOTIMPL;
}

HRESULT WINAPI FakeDownloadCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN outer,
    DWORD clsContext,
    REFIID riid,
    LPVOID *outObject
) {
    ++g_fakeDownloadCoCreateCalls;
    g_fakeDownloadCoCreateArgsOk =
        IsEqualGUID(rclsid, g_CLSID_WestwoodOnlineUpgradeDownload) != 0 &&
        outer == 0 &&
        clsContext == CLSCTX_INPROC_SERVER &&
        IsEqualGUID(riid, g_IID_WestwoodOnlineUpgradeDownload) != 0 &&
        outObject != 0;
    if (outObject != 0) {
        *outObject = g_fakeDownloadCoCreateObject;
    }
    return g_fakeDownloadCoCreateResult;
}

BOOL WINAPI FakeDownloadDlgSetDlgItemTextA(HWND hWnd, int controlId, LPCSTR text) {
    if (g_downloadDlgSetDlgItemTextCalls < 4) {
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
    LPARAM lParam
) {
    const int index = g_downloadDlgSendDlgItemMessageCalls;
    if (index < 4) {
        g_downloadDlgSendDlgItemMessageHwnd[index] = hWnd;
        g_downloadDlgSendDlgItemMessageControlId[index] = controlId;
        g_downloadDlgSendDlgItemMessageMessage[index] = message;
        g_downloadDlgSendDlgItemMessageWParam[index] = wParam;
        g_downloadDlgSendDlgItemMessageLParam[index] = lParam;
    }
    ++g_downloadDlgSendDlgItemMessageCalls;
    return 0;
}

VOID WINAPI FakeInitSleep(DWORD duration) {
    if (g_initSleepCalls < 4) {
        g_initSleepDurations[g_initSleepCalls] = duration;
    }
    ++g_initSleepCalls;
}

char *__fastcall FakeInitGetMessageString(unsigned int messageId) {
    static char messages[16][32];
    if (g_initMessageIdCalls < 16) {
        g_initMessageIds[g_initMessageIdCalls] = messageId;
        wsprintfA(messages[g_initMessageIdCalls], "msg-%04x", messageId);
        return messages[g_initMessageIdCalls++];
    }

    return (char *)"msg-overflow";
}

int __fastcall FakeInitDialogBaseOnInitDialog(CDialog *self, void *) {
    ++g_initDialogBaseOnInitCalls;
    g_initDialogBaseOnInitThis = self;
    return 1;
}

LRESULT WINAPI FakeInitDialogSendMessageA(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
) {
    const int index = g_initDialogSendMessageCalls;
    if (index < 24) {
        g_initDialogSendMessageHwnd[index] = hwnd;
        g_initDialogSendMessageMsg[index] = msg;
        g_initDialogSendMessageWParam[index] = wParam;
        g_initDialogSendMessageLParam[index] = lParam;
    }
    ++g_initDialogSendMessageCalls;

    if (msg == CB_ADDSTRING || msg == CB_INSERTSTRING) {
        return g_initDialogComboAddCalls++;
    }
    return 0;
}

int STDMETHODCALLTYPE FakeConfigInitLoadConnectProfileStrings(
    IUnknown *,
    int profileId,
    char **playerNameOut,
    char **connectStringOut
) {
    const int index = g_configInitLoadProfileCalls;
    if (index < 4) {
        g_configInitLoadProfileIds[index] = profileId;
        *playerNameOut = (char *)g_configInitLoadProfilePlayers[index];
        *connectStringOut = (char *)g_configInitLoadProfileConnectStrings[index];
    }
    ++g_configInitLoadProfileCalls;
    return index < 4 ? g_configInitLoadProfileResults[index] : 1;
}

void ConstructConfigDialogStrings(WestwoodOnlineUpgradeConfigDialog &dialog) {
    new (&dialog.m_reservedString) CString();
    new (&dialog.m_connectStringEditText) CString();
    for (int index = 0; index < 2; ++index) {
        new (&dialog.m_savedPlayerNames[index]) CString();
        new (&dialog.m_savedConnectStrings[index]) CString();
        new (&dialog.m_profilePlayerNames[index]) CString();
        new (&dialog.m_profileConnectStrings[index]) CString();
    }
}

void DestructConfigDialogStrings(WestwoodOnlineUpgradeConfigDialog &dialog) {
    for (int index = 1; index >= 0; --index) {
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
    const char *text
) {
    ++g_configInitSetWindowTextCalls;
    g_configInitSetWindowTextThis = self;
    g_configInitSetWindowTextValue = text;
}

int STDMETHODCALLTYPE FakeConfigOnOkSaveConnectProfileStrings(
    IUnknown *,
    int profileId,
    const char *playerName,
    const char *connectString,
    int connectStringMode
) {
    const int index = g_configOnOkSaveProfileCalls;
    if (index < 4) {
        g_configOnOkSaveProfileIds[index] = profileId;
        g_configOnOkSaveProfilePlayers[index] = playerName;
        g_configOnOkSaveProfileConnectStrings[index] = connectString;
        g_configOnOkSaveProfileModes[index] = connectStringMode;
    }
    ++g_configOnOkSaveProfileCalls;
    return 0;
}

void __fastcall FakeConfigOnOkBaseOnOK(CDialog *self, void *) {
    ++g_configOnOkBaseOnOkCalls;
    g_configOnOkBaseOnOkThis = self;
}

void FakeModalTimeTick() {
    ++g_modalTimeTickCalls;
}

void ResetShowModalProbe() {
    g_showModalSelectedMissionIndex = -1;
    g_showModalMenuStep = 0;
    g_showModalMenuVisible[0] = -1;
    g_showModalMenuVisible[1] = -1;
    g_showModalMenuThis[0] = 0;
    g_showModalMenuThis[1] = 0;
    g_showModalDoModalCalls = 0;
    g_showModalDialogDtorCalls = 0;
    g_showModalCStringDtorCalls = 0;
    g_showModalListDtorCalls = 0;
    g_showModalEditDtorCalls = 0;
    g_showModalComboDtorCalls = 0;
    g_showModalButtonDtorCalls = 0;
    g_showModalArgsOk = true;
}

void __fastcall FakeShowModalSetMenuBarVisibility(CZRecoilFrame *self, void *, int visible) {
    if (g_showModalMenuStep < 2) {
        g_showModalMenuThis[g_showModalMenuStep] = self;
        g_showModalMenuVisible[g_showModalMenuStep] = visible;
    }
    ++g_showModalMenuStep;
}

int __fastcall FakeShowModalDoModal(void *self, void *) {
    ++g_showModalDoModalCalls;
    g_showModalArgsOk =
        g_showModalArgsOk &&
        self == g_pWestwoodOnlineUpgradeDialog &&
        g_pWestwoodOnlineUpgradeDialog != 0 &&
        g_pWestwoodOnlineUpgradeProgressDialog != 0 &&
        TestObjectVtable(g_pWestwoodOnlineUpgradeDialog) != 0 &&
        TestObjectVtable(g_pWestwoodOnlineUpgradeProgressDialog) != 0 &&
        g_WestwoodOnlineUpgradeSelectedMissionIndex == -1;
    g_WestwoodOnlineUpgradeSelectedMissionIndex = g_showModalSelectedMissionIndex;
    return 1;
}

void __fastcall FakeShowModalDialogDtor(void *, void *) {
    ++g_showModalDialogDtorCalls;
}

void __fastcall FakeShowModalCStringDtor(void *, void *) {
    ++g_showModalCStringDtorCalls;
}

void __fastcall FakeShowModalListDtor(void *, void *) {
    ++g_showModalListDtorCalls;
}

void __fastcall FakeShowModalEditDtor(void *, void *) {
    ++g_showModalEditDtorCalls;
}

void __fastcall FakeShowModalComboDtor(void *, void *) {
    ++g_showModalComboDtorCalls;
}

void __fastcall FakeShowModalButtonDtor(void *, void *) {
    ++g_showModalButtonDtorCalls;
}

long __fastcall FakeWestwoodDefault(CWnd *, void *) {
    ++g_threeFloatDefaultCount;
    return g_threeFloatDefaultReturn;
}

void STDMETHODCALLTYPE FakeInitProcessCallbacks(IUnknown *) {
    ++g_initProcessCallbacksCalls;
}

void STDMETHODCALLTYPE FakeInitRequestListMode(IUnknown *, int listMode, int enabled) {
    ++g_initRequestListModeCalls;
    g_initRequestListMode = listMode;
    g_initRequestListModeEnabled = enabled;
}

int __fastcall FakeBeginConnectGetWindowTextA(
    CWnd *self,
    void *,
    char *buffer,
    int maxCount
) {
    ++g_beginConnectGetWindowTextCalls;
    g_beginConnectGetWindowTextThis = self;
    g_beginConnectGetWindowTextBuffer = buffer;
    g_beginConnectGetWindowTextMaxCount = maxCount;
    strcpy(buffer, "SrvMode");
    return 7;
}

int STDMETHODCALLTYPE FakeBeginConnectPrepareContext(
    IUnknown *,
    WestwoodOnlineUpgradeConnectContext *context
) {
    ++g_beginConnectPrepareCalls;
    g_beginConnectPrepareContext = context;
    return g_beginConnectPrepareResult;
}

void STDMETHODCALLTYPE FakeBeginConnectWithPreparedContext(
    IUnknown *,
    WestwoodOnlineUpgradeConnectContext *context,
    int mode
) {
    ++g_beginConnectPreparedCalls;
    g_beginConnectPreparedContext = context;
    g_beginConnectPreparedMode = mode;
}

int FakeAppendConnectAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *text
) {
    ++g_appendConnectAppendCalls;
    g_appendConnectAppendThis = self;
    g_appendConnectAppendText = text;
    return 1;
}

void __fastcall FakeQueryStatusSetWindowTextA(
    CWnd *self,
    void *,
    const char *text
) {
    const int index = g_queryStatusSetWindowTextCalls;
    if (index < 4) {
        g_queryStatusSetWindowTextThis[index] = (void *)self->m_hWnd;
        strcpy(g_queryStatusSetWindowTextValue[index], text);
    }
    ++g_queryStatusSetWindowTextCalls;
}

void STDMETHODCALLTYPE FakeQueueSessionRequest(
    IUnknown *self,
    WestwoodOnlineUpgradeSessionRequest *request
) {
    if (g_queueSessionRequestCalls < 8) {
        g_queueSessionRequestSelf[g_queueSessionRequestCalls] = self;
        g_queueSessionRequestCopies[g_queueSessionRequestCalls] = *request;
    }
    ++g_queueSessionRequestCalls;
}

void STDMETHODCALLTYPE FakeLookupBrowseRecordBySessionName(
    IUnknown *self,
    const char *sessionName,
    int lookupMode
) {
    if (g_lookupBrowseRecordCalls < 8) {
        g_lookupBrowseRecordSelf[g_lookupBrowseRecordCalls] = self;
        strcpy(g_lookupBrowseRecordSessionName[g_lookupBrowseRecordCalls], sessionName);
        g_lookupBrowseRecordMode[g_lookupBrowseRecordCalls] = lookupMode;
    }
    ++g_lookupBrowseRecordCalls;
}

void __fastcall FakeSubmitVisibleGetWindowTextA(
    CWnd *self,
    void *,
    CString *text
) {
    ++g_submitVisibleGetWindowTextCalls;
    g_submitVisibleGetWindowTextThis = self;
    *text = g_submitVisibleStatusInput;
}

void STDMETHODCALLTYPE FakeSubmitVisibleSubmitStatusText(
    IUnknown *self,
    const char *statusText
) {
    ++g_submitVisibleSubmitStatusCalls;
    g_submitVisibleSubmitStatusSelf = self;
    strcpy(g_submitVisibleSubmitStatusText, statusText);
}

void STDMETHODCALLTYPE FakeSubmitVisibleSubmitSessionRequestListAndStatusText(
    IUnknown *self,
    WestwoodOnlineUpgradeSessionRequest *sessionRequestList,
    const char *statusText
) {
    ++g_submitVisibleSubmitListCalls;
    g_submitVisibleSubmitListSelf = self;
    strcpy(g_submitVisibleSubmitListStatusText, statusText);
    g_submitVisibleSubmitListCount = 0;
    while (sessionRequestList != 0 && g_submitVisibleSubmitListCount < 8) {
        strcpy(
            g_submitVisibleSubmitListNames[g_submitVisibleSubmitListCount],
            sessionRequestList->m_sessionName
        );
        sessionRequestList = sessionRequestList->m_next;
        ++g_submitVisibleSubmitListCount;
    }
}

unsigned int FakeSubmitVisibleFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    const char *sessionName,
    const char *statusText
) {
    ++g_submitVisibleFormatCalls;
    g_submitVisibleFormatBuffer = outBuffer;
    g_submitVisibleFormatMaxChars = maxChars;
    g_submitVisibleFormatMessageId = messageId;
    g_submitVisibleFormatSessionName = sessionName;
    g_submitVisibleFormatStatusText = statusText;
    strcpy(outBuffer, "formatted visible status");
    return 24;
}

unsigned int FakeDownloadReadyFormatMessage(
    char *outBuffer,
    int maxChars,
    unsigned int messageId,
    int ordinal,
    int total
) {
    if (g_downloadReadyFormatCalls < 8) {
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

INT_PTR WINAPI FakeDownloadReadyDialogBoxParamA(
    HINSTANCE instance,
    LPCSTR templateName,
    HWND parent,
    DLGPROC dialogProc,
    LPARAM initParam
) {
    const int index = g_downloadReadyDialogCalls;
    if (index < 8) {
        g_downloadReadyDialogInstance[index] = instance;
        g_downloadReadyDialogTemplate[index] = templateName;
        g_downloadReadyDialogParent[index] = parent;
        g_downloadReadyDialogProc[index] = dialogProc;
        g_downloadReadyDialogParam[index] = initParam;
    }
    ++g_downloadReadyDialogCalls;
    return index < 8 ? g_downloadReadyDialogResult[index] : 1;
}

int FakeSubmitVisibleAppendStatusTextFmt(
    WestwoodOnlineUpgradeDialog *self,
    const char *format,
    ...
) {
    ++g_submitVisibleAppendCalls;
    g_submitVisibleAppendThis = self;
    strcpy(g_submitVisibleAppendFormat, format);
    g_submitVisibleAppendArg0[0] = '\0';
    g_submitVisibleAppendArg1[0] = '\0';

    va_list args;
    va_start(args, format);
    if (strcmp(format, "{ %s } %s") == 0) {
        strcpy(g_submitVisibleAppendArg0, va_arg(args, const char *));
        strcpy(g_submitVisibleAppendArg1, va_arg(args, const char *));
    }
    va_end(args);
    return 1;
}

FakeDownloadUnknownVtable g_fakeDownloadUnknownVtable = {
    FakeDownloadUnknownQueryInterface,
    FakeDownloadUnknownAddRef,
    FakeDownloadUnknownRelease,
};

FakeConnectionPointContainerVtable g_fakeDownloadCpcVtable = {
    FakeDownloadCpcQueryInterface,
    FakeDownloadCpcAddRef,
    FakeDownloadCpcRelease,
    FakeDownloadEnumConnectionPoints,
    FakeDownloadFindConnectionPoint,
};

FakeConnectionPointVtable g_fakeDownloadConnectionPointVtable = {
    FakeDownloadCpQueryInterface,
    FakeDownloadCpAddRef,
    FakeDownloadCpRelease,
    FakeDownloadGetConnectionInterface,
    FakeDownloadGetConnectionPointContainer,
    FakeDownloadAdvise,
    FakeDownloadUnadvise,
    FakeDownloadEnumConnections,
};

bool PatchImportByOrdinal(
    const char *dllName,
    WORD ordinal,
    void *replacement,
    ImportFunctionPatch &patch
) {
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

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(imageBase + (
            descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                : descriptor->FirstThunk
        ));
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (
                !IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal) ||
                (WORD)(nameThunk->u1.Ordinal & 0xffff) != ordinal
            ) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
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

bool PatchImportByName(
    const char *dllName,
    const char *functionName,
    void *replacement,
    ImportFunctionPatch &patch
) {
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

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(imageBase + (
            descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                : descriptor->FirstThunk
        ));
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
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
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

void RestoreImportPatch(ImportFunctionPatch &patch) {
    if (patch.slot == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.slot,
            sizeof(*patch.slot),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }

    patch.slot = 0;
    patch.original = 0;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    if (target == 0) {
        patch.address = 0;
        return false;
    }

    patch.address = (unsigned char *)target;
    memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
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

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = 0;
}

bool InstallShowModalPatches(ImportFunctionPatch *imports, CodeFunctionPatch &menuPatch) {
    const WORD kMfc42CDialogDoModalOrdinal = 2514;
    const WORD kMfc42CStringDtorOrdinal = 800;
    const WORD kMfc42CComboBoxDtorOrdinal = 616;
    const WORD kMfc42CListBoxDtorOrdinal = 692;
    const WORD kMfc42CButtonDtorOrdinal = 609;
    const WORD kMfc42CEditDtorOrdinal = 656;
    const WORD kMfc42CDialogDtorOrdinal = 641;

    return PatchImportByOrdinal(
               "MFC42.DLL",
               kMfc42CDialogDoModalOrdinal,
               (void *)&FakeShowModalDoModal,
               imports[0]
           ) &&
           PatchImportByOrdinal(
               "MFC42.DLL",
               kMfc42CStringDtorOrdinal,
               (void *)&FakeShowModalCStringDtor,
               imports[1]
           ) &&
           PatchImportByOrdinal(
               "MFC42.DLL",
               kMfc42CComboBoxDtorOrdinal,
               (void *)&FakeShowModalComboDtor,
               imports[2]
           ) &&
           PatchImportByOrdinal(
               "MFC42.DLL",
               kMfc42CListBoxDtorOrdinal,
               (void *)&FakeShowModalListDtor,
               imports[3]
           ) &&
           PatchImportByOrdinal(
               "MFC42.DLL",
               kMfc42CButtonDtorOrdinal,
               (void *)&FakeShowModalButtonDtor,
               imports[4]
           ) &&
           PatchImportByOrdinal(
               "MFC42.DLL",
               kMfc42CEditDtorOrdinal,
               (void *)&FakeShowModalEditDtor,
               imports[5]
           ) &&
           PatchImportByOrdinal(
               "MFC42.DLL",
               kMfc42CDialogDtorOrdinal,
               (void *)&FakeShowModalDialogDtor,
               imports[6]
           ) &&
           PatchFunctionJump(
               MethodAddress(&CZRecoilFrame::SetMenuBarVisibility),
               (void *)&FakeShowModalSetMenuBarVisibility,
               menuPatch
           );
}

void RestoreShowModalPatches(ImportFunctionPatch *imports, CodeFunctionPatch &menuPatch) {
    RestoreFunctionPatch(menuPatch);
    for (int index = 6; index >= 0; --index) {
        RestoreImportPatch(imports[index]);
    }
}

} // namespace

extern "C" int westwood_online_upgrade_config_dialog_constructor_smoke(void) {
    int wolPasswordFlag = 73;
    int *const oldWolPasswordFlagOption = g_zOpt_WolPasswordFlagOption;
    g_zOpt_WolPasswordFlagOption = &wolPasswordFlag;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeConfigDialog)] = {0};
    WestwoodOnlineUpgradeConfigDialog &dialog =
        *(WestwoodOnlineUpgradeConfigDialog *)dialogStorage;
    memset(&dialog, 0x5a, sizeof(dialog));

    WestwoodOnlineUpgradeConfigDialog *const result = dialog.Constructor(0);
    int failure = 0;
    if (result != &dialog || TestObjectVtable(&dialog) == 0) {
        failure = 1;
    } else if (!TestMfcWindowConstructed(dialog.m_profileCombo) ||
               !TestMfcWindowConstructed(dialog.m_connectStringEdit)) {
        failure = 2;
    } else if (strcmp((const char *)dialog.m_connectStringEditText, "") != 0) {
        failure = 3;
    } else if (dialog.m_wolPasswordFlag != wolPasswordFlag) {
        failure = 4;
    }

    g_zOpt_WolPasswordFlagOption = oldWolPasswordFlagOption;
    return failure;
}

extern "C" int westwood_online_upgrade_config_dialog_on_init_smoke(void) {
    const WORD kMfc42CDialogOnInitDialogOrdinal = 4710;
    ImportFunctionPatch imports[2] = {};
    CodeFunctionPatch patches[2] = {};
    if (
        !PatchImportByOrdinal(
            "MFC42.DLL",
            kMfc42CDialogOnInitDialogOrdinal,
            (void *)&FakeInitDialogBaseOnInitDialog,
            imports[0]
        ) ||
        !PatchImportByName(
            "USER32.dll",
            "SendMessageA",
            (void *)&FakeInitDialogSendMessageA,
            imports[1]
        ) ||
        !PatchFunctionJump(
            (void *)&zLoc::GetMessageString,
            (void *)&FakeInitGetMessageString,
            patches[0]
        ) ||
        !PatchFunctionJump(
            CWndSetWindowTextAAddress(),
            (void *)&FakeConfigInitSetWindowTextA,
            patches[1]
        )
    ) {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        RestoreImportPatch(imports[1]);
        RestoreImportPatch(imports[0]);
        return 90;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.LoadConnectProfileStrings =
        FakeConfigInitLoadConnectProfileStrings;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;

    g_configInitLoadProfileCalls = 0;
    memset(g_configInitLoadProfileIds, 0, sizeof(g_configInitLoadProfileIds));
    g_configInitLoadProfileResults[0] = 0;
    g_configInitLoadProfileResults[1] = 0;
    g_configInitLoadProfilePlayers[0] = "PilotOne";
    g_configInitLoadProfilePlayers[1] = "";
    g_configInitLoadProfileConnectStrings[0] = "ConnectOne";
    g_configInitLoadProfileConnectStrings[1] = "";
    g_initDialogBaseOnInitCalls = 0;
    g_initDialogBaseOnInitThis = 0;
    g_initDialogSendMessageCalls = 0;
    g_initDialogComboAddCalls = 0;
    memset(g_initDialogSendMessageHwnd, 0, sizeof(g_initDialogSendMessageHwnd));
    memset(g_initDialogSendMessageMsg, 0, sizeof(g_initDialogSendMessageMsg));
    memset(g_initDialogSendMessageWParam, 0, sizeof(g_initDialogSendMessageWParam));
    memset(g_initDialogSendMessageLParam, 0, sizeof(g_initDialogSendMessageLParam));
    g_initMessageIdCalls = 0;
    g_configInitSetWindowTextCalls = 0;
    g_configInitSetWindowTextThis = 0;
    g_configInitSetWindowTextValue = 0;

    int failure = 0;
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeConfigDialog)] = {0};
    WestwoodOnlineUpgradeConfigDialog &dialog =
        *(WestwoodOnlineUpgradeConfigDialog *)dialogStorage;
    ConstructConfigDialogStrings(dialog);
    dialog.m_profileCombo.m_hWnd = (HWND)0x12340500;
    dialog.m_connectStringEdit.m_hWnd = (HWND)0x12340501;

    const BOOL result = dialog.WestwoodOnlineUpgradeConfigDialog::OnInitDialog();

    if (
        result != TRUE ||
        g_initDialogBaseOnInitCalls != 1 ||
        g_initDialogBaseOnInitThis != (CDialog *)&dialog ||
        g_configInitLoadProfileCalls != 2 ||
        g_configInitLoadProfileIds[0] != 1 ||
        g_configInitLoadProfileIds[1] != 2
    ) {
        failure = 1;
    } else if (
        strcmp((const char *)dialog.m_savedPlayerNames[0], "PilotOne") != 0 ||
        strcmp((const char *)dialog.m_savedConnectStrings[0], "ConnectOne") != 0 ||
        strcmp((const char *)dialog.m_profilePlayerNames[0], "PilotOne") != 0 ||
        strcmp((const char *)dialog.m_profileConnectStrings[0], "ConnectOne") != 0 ||
        strcmp((const char *)dialog.m_savedPlayerNames[1], "") != 0 ||
        strcmp((const char *)dialog.m_savedConnectStrings[1], "") != 0 ||
        strcmp((const char *)dialog.m_profilePlayerNames[1], "") != 0 ||
        strcmp((const char *)dialog.m_profileConnectStrings[1], "") != 0
    ) {
        failure = 2;
    } else if (
        g_initMessageIdCalls != 1 ||
        g_initMessageIds[0] != 0x3044 ||
        g_initDialogSendMessageCalls != 5 ||
        g_initDialogComboAddCalls != 2 ||
        g_initDialogSendMessageHwnd[0] != dialog.m_profileCombo.m_hWnd ||
        g_initDialogSendMessageMsg[0] != CB_INSERTSTRING ||
        g_initDialogSendMessageWParam[0] != 0 ||
        strcmp((const char *)g_initDialogSendMessageLParam[0], "PilotOne") != 0 ||
        g_initDialogSendMessageMsg[1] != CB_SETITEMDATA ||
        g_initDialogSendMessageWParam[1] != 0 ||
        g_initDialogSendMessageLParam[1] != 0 ||
        g_initDialogSendMessageMsg[2] != CB_INSERTSTRING ||
        g_initDialogSendMessageWParam[2] != 1 ||
        strcmp((const char *)g_initDialogSendMessageLParam[2], "msg-3044") != 0 ||
        g_initDialogSendMessageMsg[3] != CB_SETITEMDATA ||
        g_initDialogSendMessageWParam[3] != 1 ||
        g_initDialogSendMessageLParam[3] != 1 ||
        g_initDialogSendMessageMsg[4] != CB_SETCURSEL ||
        g_initDialogSendMessageWParam[4] != 0 ||
        g_initDialogSendMessageLParam[4] != 0
    ) {
        failure = 3;
    } else if (
        dialog.m_profileConnectStringModes[0] != 1 ||
        dialog.m_profileConnectStringModes[1] != 0 ||
        dialog.m_selectedProfileIndex != 0 ||
        dialog.m_profileComboEditDirty != 0 ||
        g_configInitSetWindowTextCalls != 1 ||
        g_configInitSetWindowTextThis != (CWnd *)&dialog.m_connectStringEdit ||
        strcmp(g_configInitSetWindowTextValue, "ConnectOne") != 0
    ) {
        failure = 4;
    }

    DestructConfigDialogStrings(dialog);

    g_pWestwoodOnlineUpgradeApi = oldApi;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    RestoreImportPatch(imports[1]);
    RestoreImportPatch(imports[0]);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_show_modal_smoke(void) {
    ImportFunctionPatch imports[7] = {};
    CodeFunctionPatch menuPatch = {};
    if (!InstallShowModalPatches(imports, menuPatch)) {
        RestoreShowModalPatches(imports, menuPatch);
        return 10;
    }

    CWnd *const oldMainWnd = g_RecoilApp.m_pMainWnd;
    HINSTANCE const oldInstance = g_RecoilApp.m_hInstance;
    HINSTANCE const oldModuleInstance = g_hWestwoodOnlineUpgradeModuleInstance;
    WestwoodOnlineUpgradeDialog *const oldDialog = g_pWestwoodOnlineUpgradeDialog;
    WestwoodOnlineUpgradeProgressDialog *const oldProgress =
        g_pWestwoodOnlineUpgradeProgressDialog;
    const int oldSelected = g_WestwoodOnlineUpgradeSelectedMissionIndex;

    g_RecoilApp.m_pMainWnd = (CWnd *)0x12345678;
    g_RecoilApp.m_hInstance = (HINSTANCE)0x2468ace0;

    ResetShowModalProbe();
    g_showModalSelectedMissionIndex = 8;
    int selectedMissionIndex = -55;
    int result =
        WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(&selectedMissionIndex);
    int failure = 0;
    if (result != 1 || selectedMissionIndex != 8) {
        failure = 1;
    } else if (!g_showModalArgsOk) {
        failure = 2;
    } else if (g_hWestwoodOnlineUpgradeModuleInstance != (HINSTANCE)0x2468ace0) {
        failure = 3;
    } else if (
        g_showModalMenuStep != 2 ||
        g_showModalMenuThis[0] != (void *)0x12345678 ||
        g_showModalMenuThis[1] != (void *)0x12345678 ||
        g_showModalMenuVisible[0] != 0 ||
        g_showModalMenuVisible[1] != 1
    ) {
        failure = 4;
    } else if (g_showModalDoModalCalls != 1) {
        failure = 50 + g_showModalDoModalCalls;
    } else if (g_showModalDialogDtorCalls != 2) {
        failure = 60 + g_showModalDialogDtorCalls;
    } else if (g_showModalCStringDtorCalls != 3) {
        failure = 70 + g_showModalCStringDtorCalls;
    } else if (g_showModalListDtorCalls != 3) {
        failure = 80 + g_showModalListDtorCalls;
    } else if (g_showModalEditDtorCalls != 7) {
        failure = 90 + g_showModalEditDtorCalls;
    } else if (g_showModalComboDtorCalls != 1) {
        failure = 100 + g_showModalComboDtorCalls;
    } else if (g_showModalButtonDtorCalls != 6) {
        failure = 110 + g_showModalButtonDtorCalls;
    }

    ResetShowModalProbe();
    g_showModalSelectedMissionIndex = -1;
    selectedMissionIndex = -77;
    result =
        WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(&selectedMissionIndex);
    if (failure == 0) {
        if (result != 0 || selectedMissionIndex != -77) {
            failure = 6;
        } else if (
            !g_showModalArgsOk ||
            g_showModalMenuStep != 2 ||
            g_showModalMenuVisible[0] != 0 ||
            g_showModalMenuVisible[1] != 1 ||
            g_showModalDoModalCalls != 1
        ) {
            failure = 7;
        } else if (
            g_showModalDialogDtorCalls != 2 ||
            g_showModalCStringDtorCalls != 3 ||
            g_showModalListDtorCalls != 3 ||
            g_showModalEditDtorCalls != 7 ||
            g_showModalComboDtorCalls != 1 ||
            g_showModalButtonDtorCalls != 6
        ) {
            failure = 8;
        }
    }

    g_RecoilApp.m_pMainWnd = oldMainWnd;
    g_RecoilApp.m_hInstance = oldInstance;
    g_hWestwoodOnlineUpgradeModuleInstance = oldModuleInstance;
    g_pWestwoodOnlineUpgradeDialog = oldDialog;
    g_pWestwoodOnlineUpgradeProgressDialog = oldProgress;
    g_WestwoodOnlineUpgradeSelectedMissionIndex = oldSelected;

    RestoreShowModalPatches(imports, menuPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_show_download_ready_list_smoke(void) {
    WestwoodOnlineUpgradeDownloadReadyEntry *const oldReadyList =
        g_pWestwoodOnlineUpgradeDownloadReadyList;
    HINSTANCE const oldInstance = g_RecoilApp_hInstance;
    HWND const oldMainHwnd = g_RecoilApp_hWndMain;
    char oldPrompt[sizeof(g_WestwoodOnlineUpgradeDownloadReadyPromptText)];
    ImportFunctionPatch import = {};
    CodeFunctionPatch formatPatch = {};

    memcpy(
        oldPrompt,
        g_WestwoodOnlineUpgradeDownloadReadyPromptText,
        sizeof(oldPrompt)
    );

    int failure = 0;
    if (!PatchFunctionJump(
            (void *)&zLoc::FormatMessage,
            (void *)&FakeDownloadReadyFormatMessage,
            formatPatch
        ) ||
        !PatchImportByName(
            "USER32.dll",
            "DialogBoxParamA",
            (void *)&FakeDownloadReadyDialogBoxParamA,
            import
        )) {
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

    if (failure == 0) {
        int result = WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(&entries[0]);
        if (result != 1 || g_downloadReadyFormatCalls != 3 ||
            g_downloadReadyDialogCalls != 3 ||
            g_pWestwoodOnlineUpgradeDownloadReadyList != &entries[2]) {
            failure = 1;
        }
        for (int index = 0; failure == 0 && index < 3; ++index) {
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
                g_downloadReadyDialogParam[index] != 0) {
                failure = 2 + index;
            }
        }
        if (failure == 0 &&
            strcmp(
                g_WestwoodOnlineUpgradeDownloadReadyPromptText,
                "download 3 of 3"
            ) != 0) {
            failure = 5;
        }
    }

    if (failure == 0) {
        g_downloadReadyFormatCalls = 0;
        g_downloadReadyDialogCalls = 0;
        int result = WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(0);
        if (result != 1 || g_downloadReadyFormatCalls != 0 ||
            g_downloadReadyDialogCalls != 0) {
            failure = 6;
        }
    }

    if (failure == 0) {
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
            g_downloadReadyFormatTotal[1] != 2) {
            failure = 7;
        }
    }

    g_pWestwoodOnlineUpgradeDownloadReadyList = oldReadyList;
    g_RecoilApp_hInstance = oldInstance;
    g_RecoilApp_hWndMain = oldMainHwnd;
    memcpy(
        g_WestwoodOnlineUpgradeDownloadReadyPromptText,
        oldPrompt,
        sizeof(oldPrompt)
    );
    RestoreImportPatch(import);
    RestoreFunctionPatch(formatPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_ref_count_and_lock_init_smoke(void) {
    WestwoodOnlineUpgradeRefCountAndLock refCountAndLock = {};
    refCountAndLock.refCount = 17;
    WestwoodOnlineUpgradeRefCountAndLock *const result = refCountAndLock.Init();
    int failure = 0;
    if (result != &refCountAndLock || refCountAndLock.refCount != 0) {
        failure = 1;
    } else if (TryEnterCriticalSection(&refCountAndLock.lock) == 0) {
        failure = 2;
    } else {
        LeaveCriticalSection(&refCountAndLock.lock);
    }

    DeleteCriticalSection(&refCountAndLock.lock);
    return failure;
}

extern "C" int westwood_online_upgrade_config_dialog_on_ok_smoke(void) {
    const WORD kMfc42CDialogOnOKOrdinal = 4853;
    ImportFunctionPatch onOkImport = {};
    if (!PatchImportByOrdinal(
            "MFC42.DLL",
            kMfc42CDialogOnOKOrdinal,
            (void *)&FakeConfigOnOkBaseOnOK,
            onOkImport
        )) {
        return 90;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    int wolPasswordFlag = 77;
    int *const oldWolPasswordFlagOption = g_zOpt_WolPasswordFlagOption;

    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
    g_initFakeApi.vftable = &g_initFakeApiVtable;
    g_initFakeApiVtable.SaveConnectProfileStrings =
        FakeConfigOnOkSaveConnectProfileStrings;
    g_pWestwoodOnlineUpgradeApi = (IUnknown *)&g_initFakeApi;
    g_zOpt_WolPasswordFlagOption = &wolPasswordFlag;

    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeConfigDialog)] = {0};
    WestwoodOnlineUpgradeConfigDialog &dialog =
        *(WestwoodOnlineUpgradeConfigDialog *)dialogStorage;
    ConstructConfigDialogStrings(dialog);

    g_configOnOkSaveProfileCalls = 0;
    memset(g_configOnOkSaveProfileIds, 0, sizeof(g_configOnOkSaveProfileIds));
    memset(g_configOnOkSaveProfilePlayers, 0, sizeof(g_configOnOkSaveProfilePlayers));
    memset(
        g_configOnOkSaveProfileConnectStrings,
        0,
        sizeof(g_configOnOkSaveProfileConnectStrings)
    );
    memset(g_configOnOkSaveProfileModes, 0, sizeof(g_configOnOkSaveProfileModes));
    g_configOnOkBaseOnOkCalls = 0;
    g_configOnOkBaseOnOkThis = 0;

    dialog.m_wolPasswordFlag = 0;
    dialog.m_profilePlayerNames[0] = "NoSaveOne";
    dialog.m_profilePlayerNames[1] = "NoSaveTwo";
    dialog.m_profileConnectStrings[0] = "HiddenOne";
    dialog.m_profileConnectStrings[1] = "HiddenTwo";
    dialog.m_profileConnectStringModes[0] = 8;
    dialog.m_profileConnectStringModes[1] = 9;

    dialog.WestwoodOnlineUpgradeConfigDialog::OnOK();

    int failure = 0;
    if (
        g_configOnOkSaveProfileCalls != 2 ||
        g_configOnOkSaveProfileIds[0] != 1 ||
        strcmp(g_configOnOkSaveProfilePlayers[0], "NoSaveOne") != 0 ||
        strcmp(g_configOnOkSaveProfileConnectStrings[0], "") != 0 ||
        g_configOnOkSaveProfileModes[0] != 0 ||
        g_configOnOkSaveProfileIds[1] != 2 ||
        strcmp(g_configOnOkSaveProfilePlayers[1], "NoSaveTwo") != 0 ||
        strcmp(g_configOnOkSaveProfileConnectStrings[1], "") != 0 ||
        g_configOnOkSaveProfileModes[1] != 0 ||
        wolPasswordFlag != 0 ||
        g_configOnOkBaseOnOkCalls != 1 ||
        g_configOnOkBaseOnOkThis != (CDialog *)&dialog
    ) {
        failure = 1;
    }

    g_configOnOkSaveProfileCalls = 0;
    memset(g_configOnOkSaveProfileIds, 0, sizeof(g_configOnOkSaveProfileIds));
    memset(g_configOnOkSaveProfilePlayers, 0, sizeof(g_configOnOkSaveProfilePlayers));
    memset(
        g_configOnOkSaveProfileConnectStrings,
        0,
        sizeof(g_configOnOkSaveProfileConnectStrings)
    );
    memset(g_configOnOkSaveProfileModes, 0, sizeof(g_configOnOkSaveProfileModes));
    g_configOnOkBaseOnOkCalls = 0;
    g_configOnOkBaseOnOkThis = 0;
    wolPasswordFlag = 77;

    dialog.m_wolPasswordFlag = 1;
    dialog.m_profilePlayerNames[0] = "SaveOne";
    dialog.m_profilePlayerNames[1] = "SaveTwo";
    dialog.m_profileConnectStrings[0] = "ConnectOne";
    dialog.m_profileConnectStrings[1] = "ConnectTwo";
    dialog.m_profileConnectStringModes[0] = 0;
    dialog.m_profileConnectStringModes[1] = 6;

    dialog.WestwoodOnlineUpgradeConfigDialog::OnOK();

    if (
        failure == 0 &&
        (
            g_configOnOkSaveProfileCalls != 2 ||
            g_configOnOkSaveProfileIds[0] != 1 ||
            strcmp(g_configOnOkSaveProfilePlayers[0], "SaveOne") != 0 ||
            strcmp(g_configOnOkSaveProfileConnectStrings[0], "ConnectOne") != 0 ||
            g_configOnOkSaveProfileModes[0] != 1 ||
            g_configOnOkSaveProfileIds[1] != 2 ||
            strcmp(g_configOnOkSaveProfilePlayers[1], "SaveTwo") != 0 ||
            strcmp(g_configOnOkSaveProfileConnectStrings[1], "ConnectTwo") != 0 ||
            g_configOnOkSaveProfileModes[1] != 0 ||
            wolPasswordFlag != 1 ||
            g_configOnOkBaseOnOkCalls != 1 ||
            g_configOnOkBaseOnOkThis != (CDialog *)&dialog
        )
    ) {
        failure = 2;
    }

    DestructConfigDialogStrings(dialog);
    g_zOpt_WolPasswordFlagOption = oldWolPasswordFlagOption;
    g_pWestwoodOnlineUpgradeApi = oldApi;
    RestoreImportPatch(onOkImport);
    return failure;
}

extern "C" int westwood_online_upgrade_config_get_message_map_smoke(void) {
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeConfigDialog)] = {0};
    WestwoodOnlineUpgradeConfigDialog &dialog =
        *(WestwoodOnlineUpgradeConfigDialog *)dialogStorage;
    const AFX_MSGMAP *const messageMap =
        dialog.WestwoodOnlineUpgradeConfigDialog::GetMessageMap();
    if (
        messageMap != &WestwoodOnlineUpgradeConfigDialog::messageMap ||
        messageMap->pfnGetBaseMap == 0 ||
        messageMap->pfnGetBaseMap() == 0 ||
        messageMap->lpEntries != &WestwoodOnlineUpgradeConfigDialog::messageEntries[0]
    ) {
        return 10;
    }

    const AFX_MSGMAP_ENTRY *const entries =
        WestwoodOnlineUpgradeConfigDialog::messageEntries;
    const bool editSetFocusOk =
        entries[0].nMessage == WM_COMMAND &&
        entries[0].nCode == EN_SETFOCUS &&
        entries[0].nID == 0x495 &&
        entries[0].nLastID == 0x495 &&
        entries[0].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[0]) ==
            MemberPointerBits(&WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear);
    const bool comboKillFocusOk =
        entries[1].nMessage == WM_COMMAND &&
        entries[1].nCode == CBN_KILLFOCUS &&
        entries[1].nID == 0x4a8 &&
        entries[1].nLastID == 0x4a8 &&
        entries[1].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[1]) ==
            MemberPointerBits(&WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus);
    const bool comboSelChangeOk =
        entries[2].nMessage == WM_COMMAND &&
        entries[2].nCode == CBN_SELCHANGE &&
        entries[2].nID == 0x4a8 &&
        entries[2].nLastID == 0x4a8 &&
        entries[2].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[2]) ==
            MemberPointerBits(&WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange);
    const bool comboEditChangeOk =
        entries[3].nMessage == WM_COMMAND &&
        entries[3].nCode == CBN_EDITCHANGE &&
        entries[3].nID == 0x4a8 &&
        entries[3].nLastID == 0x4a8 &&
        entries[3].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[3]) ==
            MemberPointerBits(&WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange);
    const bool comboDropdownOk =
        entries[4].nMessage == WM_COMMAND &&
        entries[4].nCode == CBN_DROPDOWN &&
        entries[4].nID == 0x4a8 &&
        entries[4].nLastID == 0x4a8 &&
        entries[4].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[4]) ==
            MemberPointerBits(&WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown);
    const bool connectStringModeClickedOk =
        entries[5].nMessage == WM_COMMAND &&
        entries[5].nCode == BN_CLICKED &&
        entries[5].nID == 0x49e &&
        entries[5].nLastID == 0x49e &&
        entries[5].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[5]) ==
            MemberPointerBits(&WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked);
    const bool editKillFocusOk =
        entries[6].nMessage == WM_COMMAND &&
        entries[6].nCode == EN_KILLFOCUS &&
        entries[6].nID == 0x495 &&
        entries[6].nLastID == 0x495 &&
        entries[6].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[6]) ==
            MemberPointerBits(&WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus);
    const bool sentinelOk =
        entries[7].nMessage == 0 &&
        entries[7].nCode == 0 &&
        entries[7].nID == 0 &&
        entries[7].nLastID == 0 &&
        entries[7].nSig == 0 &&
        MsgMapEntryHandlerBits(entries[7]) == 0;

    return editSetFocusOk &&
                   comboKillFocusOk &&
                   comboSelChangeOk &&
                   comboEditChangeOk &&
                   comboDropdownOk &&
                   connectStringModeClickedOk &&
                   editKillFocusOk &&
                   sentinelOk
               ? 0
               : 11;
}

extern "C" int westwood_online_upgrade_download_callback_no_op_smoke(void) {
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    WestwoodOnlineUpgradeDownloadEventSink *sink = 0;
    HRESULT createResult = WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(&sink);
    if (createResult != S_OK || sink == 0) {
        g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
        return 2;
    }

    void *const vtable = TestObjectVtable(sink);
    sink->m_refCountAndLock.refCount = 7;

    const int result = sink->CallbackNoOp(&sink->m_refCountAndLock);
    const int failure =
        result == 0 && TestObjectVtable(sink) == vtable &&
                sink->m_refCountAndLock.refCount == 7
            ? 0
            : 1;
    sink->~WestwoodOnlineUpgradeDownloadEventSink();
    ::operator delete(sink);
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    return failure;
}

extern "C" int westwood_online_upgrade_download_event_sink_create_instance_smoke(void) {
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    WestwoodOnlineUpgradeDownloadEventSink *eventSink = 0;

    HRESULT result = WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(&eventSink);
    int failure = 0;
    if (result != S_OK || eventSink == 0) {
        failure = 1;
    } else if (TestObjectVtable(eventSink) == 0) {
        failure = 2;
    } else if (eventSink->m_refCountAndLock.refCount != 0 ||
               g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount != oldLiveCount + 1) {
        failure = 3;
    } else if (TryEnterCriticalSection(&eventSink->m_refCountAndLock.lock) == 0) {
        failure = 4;
    } else {
        LeaveCriticalSection(&eventSink->m_refCountAndLock.lock);
    }

    if (eventSink != 0) {
        eventSink->~WestwoodOnlineUpgradeDownloadEventSink();
        ::operator delete(eventSink);
    }
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    return failure;
}

extern "C" int westwood_online_upgrade_download_event_sink_finished_smoke(void) {
    ImportFunctionPatch setDlgItemTextImport = {};
    if (!PatchImportByName(
            "USER32.dll",
            "SetDlgItemTextA",
            (void *)&FakeDownloadDlgSetDlgItemTextA,
            setDlgItemTextImport
        )) {
        return 1;
    }

    HWND const oldProgressHwnd = g_hWestwoodOnlineUpgradeProgressDialog;
    const int oldDialogResult = g_WestwoodOnlineUpgradeDownloadDialogResult;
    char oldStatusBuffer[sizeof(g_WestwoodOnlineUpgradeProgressStatusTextBuffer)];
    memcpy(
        oldStatusBuffer,
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        sizeof(oldStatusBuffer)
    );

    g_hWestwoodOnlineUpgradeProgressDialog = (HWND)0x24681357;
    g_WestwoodOnlineUpgradeDownloadDialogResult = 0;
    g_WestwoodOnlineUpgradeProgressStatusTextBuffer[0] = '\0';
    g_downloadDlgSetDlgItemTextCalls = 0;
    memset(g_downloadDlgSetDlgItemTextHwnd, 0, sizeof(g_downloadDlgSetDlgItemTextHwnd));
    memset(
        g_downloadDlgSetDlgItemTextControlId,
        0,
        sizeof(g_downloadDlgSetDlgItemTextControlId)
    );
    memset(g_downloadDlgSetDlgItemTextValue, 0, sizeof(g_downloadDlgSetDlgItemTextValue));

    HRESULT const result =
        ((WestwoodOnlineUpgradeDownloadEventSink *)0x12345678)
            ->WestwoodOnlineUpgradeDownloadEventSink::OnDownloadFinished();

    int failure = 0;
    if (result != S_OK ||
        g_WestwoodOnlineUpgradeDownloadDialogResult != 1 ||
        g_downloadDlgSetDlgItemTextCalls != 1 ||
        g_downloadDlgSetDlgItemTextHwnd[0] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSetDlgItemTextControlId[0] != 1023 ||
        strcmp(g_downloadDlgSetDlgItemTextValue[0], "Finished!") != 0 ||
        strcmp(g_WestwoodOnlineUpgradeProgressStatusTextBuffer, "Finished!") != 0 ||
        g_downloadDlgSetDlgItemTextValue[0] !=
            g_WestwoodOnlineUpgradeProgressStatusTextBuffer) {
        failure = 2;
    }

    memcpy(
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        oldStatusBuffer,
        sizeof(oldStatusBuffer)
    );
    g_WestwoodOnlineUpgradeDownloadDialogResult = oldDialogResult;
    g_hWestwoodOnlineUpgradeProgressDialog = oldProgressHwnd;
    RestoreImportPatch(setDlgItemTextImport);
    return failure;
}

extern "C" int westwood_online_upgrade_download_event_sink_error_smoke(void) {
    ImportFunctionPatch imports[2] = {};
    if (!PatchImportByName(
            "USER32.dll",
            "SetDlgItemTextA",
            (void *)&FakeDownloadDlgSetDlgItemTextA,
            imports[0]
        ) ||
        !PatchImportByName(
            "KERNEL32.dll",
            "Sleep",
            (void *)&FakeInitSleep,
            imports[1]
        )) {
        RestoreImportPatch(imports[1]);
        RestoreImportPatch(imports[0]);
        return 1;
    }

    HWND const oldProgressHwnd = g_hWestwoodOnlineUpgradeProgressDialog;
    const int oldDialogResult = g_WestwoodOnlineUpgradeDownloadDialogResult;
    char oldStatusBuffer[sizeof(g_WestwoodOnlineUpgradeProgressStatusTextBuffer)];
    memcpy(
        oldStatusBuffer,
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        sizeof(oldStatusBuffer)
    );

    g_hWestwoodOnlineUpgradeProgressDialog = (HWND)0x24681358;
    g_WestwoodOnlineUpgradeDownloadDialogResult = 0;
    g_WestwoodOnlineUpgradeProgressStatusTextBuffer[0] = '\0';
    g_downloadDlgSetDlgItemTextCalls = 0;
    memset(g_downloadDlgSetDlgItemTextHwnd, 0, sizeof(g_downloadDlgSetDlgItemTextHwnd));
    memset(
        g_downloadDlgSetDlgItemTextControlId,
        0,
        sizeof(g_downloadDlgSetDlgItemTextControlId)
    );
    memset(g_downloadDlgSetDlgItemTextValue, 0, sizeof(g_downloadDlgSetDlgItemTextValue));
    g_initSleepCalls = 0;
    memset(g_initSleepDurations, 0, sizeof(g_initSleepDurations));

    HRESULT const result =
        ((WestwoodOnlineUpgradeDownloadEventSink *)0x12345678)
            ->WestwoodOnlineUpgradeDownloadEventSink::OnDownloadError(E_FAIL);

    int failure = 0;
    if (result != S_OK ||
        g_WestwoodOnlineUpgradeDownloadDialogResult != -1 ||
        g_downloadDlgSetDlgItemTextCalls != 1 ||
        g_downloadDlgSetDlgItemTextHwnd[0] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSetDlgItemTextControlId[0] != 1023 ||
        strcmp(g_downloadDlgSetDlgItemTextValue[0], "ERROR") != 0 ||
        strcmp(g_WestwoodOnlineUpgradeProgressStatusTextBuffer, "ERROR") != 0 ||
        g_downloadDlgSetDlgItemTextValue[0] !=
            g_WestwoodOnlineUpgradeProgressStatusTextBuffer ||
        g_initSleepCalls != 1 ||
        g_initSleepDurations[0] != 1000) {
        failure = 2;
    }

    memcpy(
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        oldStatusBuffer,
        sizeof(oldStatusBuffer)
    );
    g_WestwoodOnlineUpgradeDownloadDialogResult = oldDialogResult;
    g_hWestwoodOnlineUpgradeProgressDialog = oldProgressHwnd;
    RestoreImportPatch(imports[1]);
    RestoreImportPatch(imports[0]);
    return failure;
}

extern "C" int westwood_online_upgrade_download_event_sink_progress_smoke(void) {
    ImportFunctionPatch imports[2] = {};
    if (!PatchImportByName(
            "USER32.dll",
            "SetDlgItemTextA",
            (void *)&FakeDownloadDlgSetDlgItemTextA,
            imports[0]
        ) ||
        !PatchImportByName(
            "USER32.dll",
            "SendDlgItemMessageA",
            (void *)&FakeDownloadDlgSendDlgItemMessageA,
            imports[1]
        )) {
        RestoreImportPatch(imports[1]);
        RestoreImportPatch(imports[0]);
        return 1;
    }

    HWND const oldProgressHwnd = g_hWestwoodOnlineUpgradeProgressDialog;
    char oldStatusBuffer[sizeof(g_WestwoodOnlineUpgradeProgressStatusTextBuffer)];
    memcpy(
        oldStatusBuffer,
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        sizeof(oldStatusBuffer)
    );

    g_hWestwoodOnlineUpgradeProgressDialog = (HWND)0x24681359;
    g_WestwoodOnlineUpgradeProgressStatusTextBuffer[0] = '\0';
    g_downloadDlgSetDlgItemTextCalls = 0;
    memset(g_downloadDlgSetDlgItemTextHwnd, 0, sizeof(g_downloadDlgSetDlgItemTextHwnd));
    memset(
        g_downloadDlgSetDlgItemTextControlId,
        0,
        sizeof(g_downloadDlgSetDlgItemTextControlId)
    );
    memset(g_downloadDlgSetDlgItemTextValue, 0, sizeof(g_downloadDlgSetDlgItemTextValue));
    g_downloadDlgSendDlgItemMessageCalls = 0;
    memset(
        g_downloadDlgSendDlgItemMessageHwnd,
        0,
        sizeof(g_downloadDlgSendDlgItemMessageHwnd)
    );
    memset(
        g_downloadDlgSendDlgItemMessageControlId,
        0,
        sizeof(g_downloadDlgSendDlgItemMessageControlId)
    );
    memset(
        g_downloadDlgSendDlgItemMessageMessage,
        0,
        sizeof(g_downloadDlgSendDlgItemMessageMessage)
    );
    memset(
        g_downloadDlgSendDlgItemMessageWParam,
        0,
        sizeof(g_downloadDlgSendDlgItemMessageWParam)
    );
    memset(
        g_downloadDlgSendDlgItemMessageLParam,
        0,
        sizeof(g_downloadDlgSendDlgItemMessageLParam)
    );

    HRESULT const withTimeResult =
        ((WestwoodOnlineUpgradeDownloadEventSink *)0x12345678)
            ->WestwoodOnlineUpgradeDownloadEventSink::OnDownloadProgress(
                125,
                500,
                77,
                9
            );
    char withTimeStatus[128];
    strcpy(withTimeStatus, g_WestwoodOnlineUpgradeProgressStatusTextBuffer);
    HRESULT const withoutTimeResult =
        ((WestwoodOnlineUpgradeDownloadEventSink *)0x12345678)
            ->WestwoodOnlineUpgradeDownloadEventSink::OnDownloadProgress(
                500,
                800,
                88,
                0
            );

    int failure = 0;
    if (withTimeResult != S_OK || withoutTimeResult != S_OK ||
        g_downloadDlgSendDlgItemMessageCalls != 2 ||
        g_downloadDlgSendDlgItemMessageHwnd[0] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSendDlgItemMessageControlId[0] != 1021 ||
        g_downloadDlgSendDlgItemMessageMessage[0] != 1026 ||
        g_downloadDlgSendDlgItemMessageWParam[0] != 25 ||
        g_downloadDlgSendDlgItemMessageLParam[0] != 0 ||
        g_downloadDlgSendDlgItemMessageHwnd[1] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSendDlgItemMessageControlId[1] != 1021 ||
        g_downloadDlgSendDlgItemMessageMessage[1] != 1026 ||
        g_downloadDlgSendDlgItemMessageWParam[1] != 62 ||
        g_downloadDlgSendDlgItemMessageLParam[1] != 0 ||
        g_downloadDlgSetDlgItemTextCalls != 2 ||
        g_downloadDlgSetDlgItemTextHwnd[0] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSetDlgItemTextControlId[0] != 1023 ||
        g_downloadDlgSetDlgItemTextValue[0] !=
            g_WestwoodOnlineUpgradeProgressStatusTextBuffer ||
        strcmp(withTimeStatus, "Bytes read: 125 / 500.    Time left: 9 seconds") != 0 ||
        g_downloadDlgSetDlgItemTextHwnd[1] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSetDlgItemTextControlId[1] != 1023 ||
        strcmp(g_downloadDlgSetDlgItemTextValue[1], "Bytes read: 500 / 800") != 0 ||
        strcmp(g_WestwoodOnlineUpgradeProgressStatusTextBuffer, "Bytes read: 500 / 800") != 0 ||
        g_downloadDlgSetDlgItemTextValue[1] !=
            g_WestwoodOnlineUpgradeProgressStatusTextBuffer) {
        failure = 2;
    }

    memcpy(
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        oldStatusBuffer,
        sizeof(oldStatusBuffer)
    );
    g_hWestwoodOnlineUpgradeProgressDialog = oldProgressHwnd;
    RestoreImportPatch(imports[1]);
    RestoreImportPatch(imports[0]);
    return failure;
}

extern "C" int westwood_online_upgrade_download_event_sink_state_changed_smoke(void) {
    ImportFunctionPatch setDlgItemTextImport = {};
    if (!PatchImportByName(
            "USER32.dll",
            "SetDlgItemTextA",
            (void *)&FakeDownloadDlgSetDlgItemTextA,
            setDlgItemTextImport
        )) {
        return 1;
    }

    HWND const oldProgressHwnd = g_hWestwoodOnlineUpgradeProgressDialog;
    char oldStatusBuffer[sizeof(g_WestwoodOnlineUpgradeProgressStatusTextBuffer)];
    memcpy(
        oldStatusBuffer,
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        sizeof(oldStatusBuffer)
    );

    g_hWestwoodOnlineUpgradeProgressDialog = (HWND)0x2468135a;
    g_WestwoodOnlineUpgradeProgressStatusTextBuffer[0] = '\0';
    g_downloadDlgSetDlgItemTextCalls = 0;
    memset(g_downloadDlgSetDlgItemTextHwnd, 0, sizeof(g_downloadDlgSetDlgItemTextHwnd));
    memset(
        g_downloadDlgSetDlgItemTextControlId,
        0,
        sizeof(g_downloadDlgSetDlgItemTextControlId)
    );
    memset(g_downloadDlgSetDlgItemTextValue, 0, sizeof(g_downloadDlgSetDlgItemTextValue));

    HRESULT const connectingResult =
        ((WestwoodOnlineUpgradeDownloadEventSink *)0x12345678)
            ->WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged(
                WOL_DOWNLOAD_STATE_CONNECTING
            );
    char connectingStatus[64];
    strcpy(connectingStatus, g_WestwoodOnlineUpgradeProgressStatusTextBuffer);
    HRESULT const findingResult =
        ((WestwoodOnlineUpgradeDownloadEventSink *)0x12345678)
            ->WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged(
                WOL_DOWNLOAD_STATE_FINDING_PATCH
            );
    char findingStatus[64];
    strcpy(findingStatus, g_WestwoodOnlineUpgradeProgressStatusTextBuffer);
    HRESULT const downloadingResult =
        ((WestwoodOnlineUpgradeDownloadEventSink *)0x12345678)
            ->WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged(
                WOL_DOWNLOAD_STATE_DOWNLOADING_PATCH
            );
    char downloadingStatus[64];
    strcpy(downloadingStatus, g_WestwoodOnlineUpgradeProgressStatusTextBuffer);
    HRESULT const ignoredResult =
        ((WestwoodOnlineUpgradeDownloadEventSink *)0x12345678)
            ->WestwoodOnlineUpgradeDownloadEventSink::OnStateChanged(
                (WestwoodOnlineUpgradeDownloadState)5
            );

    int failure = 0;
    if (connectingResult != S_OK || findingResult != S_OK ||
        downloadingResult != S_OK || ignoredResult != S_OK ||
        g_downloadDlgSetDlgItemTextCalls != 3 ||
        g_downloadDlgSetDlgItemTextHwnd[0] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSetDlgItemTextControlId[0] != 1023 ||
        g_downloadDlgSetDlgItemTextValue[0] !=
            g_WestwoodOnlineUpgradeProgressStatusTextBuffer ||
        strcmp(connectingStatus, "Connecting...") != 0 ||
        g_downloadDlgSetDlgItemTextHwnd[1] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSetDlgItemTextControlId[1] != 1023 ||
        strcmp(findingStatus, "Finding patch...") != 0 ||
        g_downloadDlgSetDlgItemTextHwnd[2] !=
            g_hWestwoodOnlineUpgradeProgressDialog ||
        g_downloadDlgSetDlgItemTextControlId[2] != 1023 ||
        strcmp(downloadingStatus, "Downloading patch...") != 0 ||
        strcmp(g_WestwoodOnlineUpgradeProgressStatusTextBuffer, "Downloading patch...") != 0 ||
        g_downloadDlgSetDlgItemTextValue[2] !=
            g_WestwoodOnlineUpgradeProgressStatusTextBuffer) {
        failure = 2;
    }

    memcpy(
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        oldStatusBuffer,
        sizeof(oldStatusBuffer)
    );
    g_hWestwoodOnlineUpgradeProgressDialog = oldProgressHwnd;
    RestoreImportPatch(setDlgItemTextImport);
    return failure;
}

extern "C" int westwood_online_upgrade_download_event_sink_query_interface_smoke(void) {
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    WestwoodOnlineUpgradeDownloadEventSink *sink = 0;
    GUID otherIid = IID_WestwoodOnlineUpgradeDownloadEventSink;
    void *outInterface;
    HRESULT result;

    result = WestwoodOnlineUpgradeDownloadEventSink::CreateInstance(&sink);
    if (result != S_OK || sink == 0) {
        g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
        return 5;
    }

    sink->m_refCountAndLock.refCount = 0;
    outInterface = (void *)0xcccccccc;
    result = sink->QueryInterface(
        IID_WestwoodOnlineUpgradeDownloadEventSink,
        &outInterface
    );
    if (result != S_OK ||
        outInterface != sink ||
        sink->m_refCountAndLock.refCount != 1) {
        sink->~WestwoodOnlineUpgradeDownloadEventSink();
        ::operator delete(sink);
        g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
        return 1;
    }

    outInterface = (void *)0xcccccccc;
    result = sink->QueryInterface(
        IID_IUnknown,
        &outInterface
    );
    if (result != S_OK ||
        outInterface != sink ||
        sink->m_refCountAndLock.refCount != 2) {
        sink->~WestwoodOnlineUpgradeDownloadEventSink();
        ::operator delete(sink);
        g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
        return 2;
    }

    otherIid.Data1 ^= 1;
    outInterface = (void *)0xcccccccc;
    result = sink->QueryInterface(
        otherIid,
        &outInterface
    );
    if (result != E_NOINTERFACE ||
        outInterface != 0 ||
        sink->m_refCountAndLock.refCount != 2) {
        sink->~WestwoodOnlineUpgradeDownloadEventSink();
        ::operator delete(sink);
        g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
        return 3;
    }

    result = sink->QueryInterface(
        IID_WestwoodOnlineUpgradeDownloadEventSink,
        0
    );
    const int failure = result == E_POINTER ? 0 : 4;
    sink->~WestwoodOnlineUpgradeDownloadEventSink();
    ::operator delete(sink);
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    return failure;
}

extern "C" int westwood_online_upgrade_download_event_sink_destructor_smoke(void) {
    LONG const oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    WestwoodOnlineUpgradeDownloadEventSink *sink =
        new WestwoodOnlineUpgradeDownloadEventSink;

    sink->m_refCountAndLock.Init();
    sink->m_refCountAndLock.refCount = 9;
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = 5;
    void *const vtable = TestObjectVtable(sink);

    sink->~WestwoodOnlineUpgradeDownloadEventSink();

    const int failure =
        sink->m_refCountAndLock.refCount == 1 &&
                g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount == 4 &&
                TestObjectVtable(sink) == vtable
            ? 0
            : 1;

    ::operator delete(sink);
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    return failure;
}

extern "C" int westwood_online_upgrade_download_event_sink_release_smoke(void) {
    LONG const oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    WestwoodOnlineUpgradeDownloadEventSink *stackSink =
        new WestwoodOnlineUpgradeDownloadEventSink;
    stackSink->m_refCountAndLock.Init();
    stackSink->m_refCountAndLock.refCount = 2;
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = 7;

    ULONG refCount = stackSink->Release();
    int failure = refCount == 1 &&
                          stackSink->m_refCountAndLock.refCount == 1 &&
                          g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount == 7
                      ? 0
                      : 1;
    stackSink->~WestwoodOnlineUpgradeDownloadEventSink();
    ::operator delete(stackSink);

    if (failure == 0) {
        WestwoodOnlineUpgradeDownloadEventSink *const heapSink =
            new WestwoodOnlineUpgradeDownloadEventSink;
        heapSink->m_refCountAndLock.Init();
        heapSink->m_refCountAndLock.refCount = 1;
        g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = 3;

        refCount = heapSink->Release();
        if (refCount != 0 || g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount != 2) {
            failure = 2;
        }
    }

    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    return failure;
}

extern "C" int westwood_online_upgrade_download_create_instance_advise_smoke(void) {
    IWestwoodOnlineUpgradeDownload *const oldDownload = g_pWestwoodOnlineUpgradeDownload;
    WestwoodOnlineUpgradeDownloadEventSink *const oldSink =
        g_pWestwoodOnlineUpgradeDownloadEventSink;
    const DWORD oldCookie = g_WestwoodOnlineUpgradeDownloadAdviseCookie;
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    ImportFunctionPatch import = {};

    g_fakeDownloadUnknown.vftable = &g_fakeDownloadUnknownVtable;
    g_fakeDownloadCpc.vftable = &g_fakeDownloadCpcVtable;
    g_fakeDownloadConnectionPoint.vftable = &g_fakeDownloadConnectionPointVtable;
    g_fakeDownloadSourceReleaseCalls = 0;
    g_fakeDownloadCpcReleaseCalls = 0;
    g_fakeDownloadConnectionPointReleaseCalls = 0;
    g_fakeDownloadFindConnectionPointCalls = 0;
    g_fakeDownloadAdviseCalls = 0;
    g_fakeDownloadAdviseSink = 0;
    g_fakeDownloadAdviseCookie = 0;
    g_fakeDownloadIidOk = false;
    g_fakeDownloadCoCreateCalls = 0;
    g_fakeDownloadCoCreateArgsOk = false;
    g_fakeDownloadCoCreateResult = S_OK;
    g_fakeDownloadCoCreateObject = (IUnknown *)&g_fakeDownloadUnknown;
    g_pWestwoodOnlineUpgradeDownload = 0;
    g_pWestwoodOnlineUpgradeDownloadEventSink = 0;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0;

    int failure = 0;
    if (!PatchImportByName(
            "ole32.dll",
            "CoCreateInstance",
            (void *)&FakeDownloadCoCreateInstance,
            import
        )) {
        failure = 90;
    } else {
        HRESULT result = WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise();
        if (result != S_OK || g_fakeDownloadCoCreateCalls != 1 ||
            !g_fakeDownloadCoCreateArgsOk) {
            failure = 1;
        } else if (g_pWestwoodOnlineUpgradeDownload !=
                       (IWestwoodOnlineUpgradeDownload *)&g_fakeDownloadUnknown ||
                   g_pWestwoodOnlineUpgradeDownloadEventSink == 0 ||
                   g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount != oldLiveCount + 1) {
            failure = 2;
        } else if (g_fakeDownloadFindConnectionPointCalls != 1 ||
                   g_fakeDownloadAdviseCalls != 1 ||
                   !g_fakeDownloadIidOk ||
                   g_fakeDownloadAdviseSink !=
                       (IUnknown *)g_pWestwoodOnlineUpgradeDownloadEventSink ||
                   g_WestwoodOnlineUpgradeDownloadAdviseCookie != 0x87654321) {
            failure = 3;
        } else if (g_fakeDownloadConnectionPointReleaseCalls != 1 ||
                   g_fakeDownloadCpcReleaseCalls != 1 ||
                   g_fakeDownloadSourceReleaseCalls != 0) {
            failure = 4;
        }
    }

    if (g_pWestwoodOnlineUpgradeDownloadEventSink != 0) {
        WestwoodOnlineUpgradeDownloadEventSink *const sink =
            g_pWestwoodOnlineUpgradeDownloadEventSink;
        sink->~WestwoodOnlineUpgradeDownloadEventSink();
        ::operator delete(sink);
    }
    g_pWestwoodOnlineUpgradeDownload = oldDownload;
    g_pWestwoodOnlineUpgradeDownloadEventSink = oldSink;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = oldCookie;
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    RestoreImportPatch(import);
    return failure;
}

extern "C" int westwood_online_upgrade_shared_com_add_ref_smoke(void) {
    WestwoodOnlineUpgradeApiEventSink apiSink = {};
    struct DownloadRefCountOwner {
        void *vftable;
        WestwoodOnlineUpgradeRefCountAndLock m_refCountAndLock;
    } downloadSink = {};
    apiSink.m_refCountAndLock.refCount = 3;
    downloadSink.m_refCountAndLock.refCount = 8;

    const ULONG apiResult = WestwoodOnlineUpgradeSharedComAddRef(&apiSink);
    const ULONG downloadResult = WestwoodOnlineUpgradeSharedComAddRef(&downloadSink);
    return apiResult == 4 &&
                   apiSink.m_refCountAndLock.refCount == 4 &&
                   downloadResult == 9 &&
                   downloadSink.m_refCountAndLock.refCount == 9
               ? 0
               : 1;
}

extern "C" int westwood_online_upgrade_dialog_refresh_list_timer_smoke(void) {
    CodeFunctionPatch timePatch = {};
    CodeFunctionPatch defaultPatch = {};
    if (
        !PatchFunctionJump((void *)&Time::Tick, (void *)&FakeModalTimeTick, timePatch) ||
        !PatchFunctionJump(
            CWndDefaultAddress(),
            (void *)&FakeWestwoodDefault,
            defaultPatch
        )
    ) {
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
    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
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
    if (g_modalTimeTickCalls != 1) {
        failure = 30;
    } else if (g_threeFloatDefaultCount != 1) {
        failure = 31;
    } else if (g_initProcessCallbacksCalls != 0) {
        failure = 33;
    } else if (g_initRequestListModeCalls != 0) {
        failure = 34;
    } else if (g_WestwoodOnlineUpgradeNextAutoRefreshTime != 10.0f) {
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
    if (
        failure == 0 &&
        (
            g_modalTimeTickCalls != 1 ||
            g_threeFloatDefaultCount != 1 ||
            g_initProcessCallbacksCalls != 1 ||
            g_initRequestListModeCalls != 0 ||
            g_WestwoodOnlineUpgradeNextAutoRefreshTime != 25.0f
        )
    ) {
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
    if (
        failure == 0 &&
        (
            g_modalTimeTickCalls != 1 ||
            g_threeFloatDefaultCount != 1 ||
            g_initProcessCallbacksCalls != 1 ||
            g_initRequestListModeCalls != 1 ||
            g_initRequestListMode != 17 ||
            g_initRequestListModeEnabled != 1 ||
            g_WestwoodOnlineUpgradeNextAutoRefreshTime != 100.0f
        )
    ) {
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

extern "C" int westwood_online_upgrade_dialog_begin_connect_smoke(void) {
    const WORD kMfc42CWndGetWindowTextAOrdinal = 3873;
    ImportFunctionPatch getWindowTextPatch = {};
    CodeFunctionPatch patches[2] = {};
    if (
        !PatchImportByOrdinal(
            "MFC42.DLL",
            kMfc42CWndGetWindowTextAOrdinal,
            (void *)&FakeBeginConnectGetWindowTextA,
            getWindowTextPatch
        ) ||
        !PatchFunctionJump(
            (void *)&zLoc::GetMessageString,
            (void *)&FakeInitGetMessageString,
            patches[0]
        ) ||
        !PatchFunctionJump(
            MethodAddress(&WestwoodOnlineUpgradeDialog::AppendStatusTextFmt),
            (void *)&FakeAppendConnectAppendStatusTextFmt,
            patches[1]
        )
    ) {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        RestoreImportPatch(getWindowTextPatch);
        return 1;
    }

    IUnknown *const oldApi = g_pWestwoodOnlineUpgradeApi;
    unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeDialog)] = {0};
    WestwoodOnlineUpgradeDialog &dialog = *(WestwoodOnlineUpgradeDialog *)dialogStorage;
    memset(&g_initFakeApiVtable, 0, sizeof(g_initFakeApiVtable));
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
    if (
        g_beginConnectGetWindowTextCalls != 1 ||
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
        strcmp(g_appendConnectAppendText, "msg-303b") != 0
    ) {
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
    if (
        failure == 0 &&
        (
            g_beginConnectGetWindowTextCalls != 1 ||
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
            strcmp(g_appendConnectAppendText, "msg-303c") != 0
        )
    ) {
        failure = 3;
    }

    g_pWestwoodOnlineUpgradeApi = oldApi;
    RestoreFunctionPatch(patches[1]);
    RestoreFunctionPatch(patches[0]);
    RestoreImportPatch(getWindowTextPatch);
    return failure;
}

extern "C" int westwood_online_upgrade_dialog_append_status_text_smoke(void) {
    HWND listBox = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_NOTIFY, 0, 0, 200,
                                  10, 0, 0, GetModuleHandleA(0), 0);
    if (listBox == 0) {
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
        strcmp(text, "status 7") != 0) {
        failure = 2;
    }

    if (failure == 0) {
        SendMessageA(listBox, LB_RESETCONTENT, 0, 0);
        dialog.m_statusLineCount = 0;
        dialog.AppendStatusTextFmt("\n");
        text[0] = 'x';
        SendMessageA(listBox, LB_GETTEXT, 0, (LPARAM)text);
        if (dialog.m_statusLineCount != 1 ||
            SendMessageA(listBox, LB_GETCOUNT, 0, 0) != 1 ||
            text[0] != '\0') {
            failure = 3;
        }
    }

    if (failure == 0) {
        SendMessageA(listBox, LB_RESETCONTENT, 0, 0);
        dialog.m_statusLineCount = 0;
        for (int index = 0; index < 105; ++index) {
            dialog.AppendStatusTextFmt("line %d", index);
        }

        SendMessageA(listBox, LB_GETTEXT, 0, (LPARAM)text);
        if (dialog.m_statusLineCount != 100) {
            failure = 4;
        } else if (SendMessageA(listBox, LB_GETCOUNT, 0, 0) != 100) {
            failure = 5;
        } else if (strcmp(text, "line 5") != 0) {
            failure = 6;
        }
    }

    DestroyWindow(listBox);
    return failure;
}

extern "C" int
westwood_online_upgrade_dialog_queue_visible_session_requests_lookup_smoke(void) {
    HWND listBox = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_MULTIPLESEL,
                                  0, 0, 200, 10, 0, 0, GetModuleHandleA(0), 0);
    if (listBox == 0) {
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

    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord, 0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] = 'S';
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 1;
    g_queueSessionRequestCalls = 0;
    g_lookupBrowseRecordCalls = 0;
    memset(g_queueSessionRequestCopies, 0, sizeof(g_queueSessionRequestCopies));
    memset(g_lookupBrowseRecordSessionName, 0, sizeof(g_lookupBrowseRecordSessionName));

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
        strcmp(g_queueSessionRequestCopies[1].m_sessionName, "Beta") != 0) {
        failure = 2;
    }

    SendMessageA(listBox, LB_SETSEL, FALSE, -1);
    g_queueSessionRequestCalls = 0;
    g_lookupBrowseRecordCalls = 0;
    dialog.QueueVisibleSessionRequestsAndLookupBrowseRecords();
    if (failure == 0 &&
        (g_WestwoodOnlineUpgradeVisibleSessionResultCount != 0 ||
         g_lookupBrowseRecordCalls != 0 ||
         g_queueSessionRequestCalls != 0)) {
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

extern "C" int
westwood_online_upgrade_dialog_submit_visible_session_requests_status_smoke(void) {
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
            patches[1])) {
        RestoreFunctionPatch(patches[1]);
        RestoreFunctionPatch(patches[0]);
        RestoreFunctionPatch(setWindowTextPatch);
        RestoreImportPatch(imports[0]);
        return 1;
    }

    HWND listBox = CreateWindowExA(0, "LISTBOX", "", WS_POPUP | LBS_MULTIPLESEL,
                                  0, 0, 200, 10, 0, 0, GetModuleHandleA(0), 0);
    if (listBox == 0) {
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
    memset(&g_WestwoodOnlineUpgradeCachedBrowseRecord, 0,
           sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord));
    strcpy(g_submitVisibleStatusInput, "Ignored");
    g_submitVisibleGetWindowTextCalls = 0;
    g_WestwoodOnlineUpgradeVisibleSessionResultCount = 77;
    dialog.SubmitVisibleSessionRequestsAndStatusText();
    if (g_submitVisibleGetWindowTextCalls != 0 ||
        g_WestwoodOnlineUpgradeVisibleSessionResultCount != 77) {
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
         g_submitVisibleAppendCalls != 0)) {
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
         strcmp(g_submitVisibleAppendArg1, "Status text") != 0)) {
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
    memset(g_submitVisibleSubmitListNames, 0, sizeof(g_submitVisibleSubmitListNames));
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
         strcmp(g_submitVisibleSubmitListNames[2], "Alpha") != 0)) {
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
