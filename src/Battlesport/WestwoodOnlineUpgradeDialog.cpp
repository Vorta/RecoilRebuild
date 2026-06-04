#include "Battlesport/WestwoodOnlineUpgradeDialog.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/RecoilApp.h"
#include "Battlesport/WestwoodOnlineUpgradeApi.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Access shim for imported MFC42 CDialog::OnCancel; this does not reimplement
// CDialog behavior.
class CDialogCancelAccessor : public CDialog {
  public:
    void CallBaseOnCancel();
};

// Access shim for imported MFC42 CDialog metadata; this does not reimplement
// CDialog behavior.
class WestwoodOnlineUpgradeCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

// Access shim for imported MFC42 CWnd::Default; this does not reimplement
// CWnd behavior.
class WestwoodOnlineUpgradeCWndAccess : public CWnd {
  public:
    long CallDefault();
};

void __stdcall DDX_Control(
    CDataExchange *dataExchange,
    int controlId,
    CWnd &control
);
void __stdcall DDX_Text(
    CDataExchange *dataExchange,
    int controlId,
    unsigned int &value
);
void __stdcall DDX_Check(
    CDataExchange *dataExchange,
    int controlId,
    int &value
);

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CEdit) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CButton) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CListBox) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CComboBox) == 0x40);

void CDialogCancelAccessor::CallBaseOnCancel() {
    CDialog::OnCancel();
}

const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

long WestwoodOnlineUpgradeCWndAccess::CallDefault() {
    return CWnd::Default();
}

namespace {
struct WestwoodOnlineUpgradeApiDialogVtable {
    void *QueryInterface;
    void *AddRef;
    void *Release;
    void(STDMETHODCALLTYPE *ProcessCallbacks)(IUnknown *self);
    void *BeginConnect;
    void *RequestBootstrapServerList;
    void(STDMETHODCALLTYPE *RequestListMode)(
        IUnknown *self,
        int listMode,
        int enabled
    );
    int(STDMETHODCALLTYPE *SubmitQueryRequest)(
        IUnknown *self,
        WestwoodOnlineUpgradeQueryRequest *request
    );
    int(STDMETHODCALLTYPE *LoadBrowseRecord)(
        IUnknown *self,
        WestwoodOnlineUpgradeBrowseRecord *record
    );
    void(STDMETHODCALLTYPE *ResetQueryState)(IUnknown *self);
    void *reserved028;
    void(STDMETHODCALLTYPE *SubmitStatusText)(
        IUnknown *self,
        const char *statusText
    );
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
    void *reserved058[2];
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
    void *reserved074;
    void *reserved078;
    void *reserved07c;
    void(STDMETHODCALLTYPE *LookupBrowseRecordBySessionName)(
        IUnknown *self,
        const char *sessionName,
        int lookupMode
    );
};

struct WestwoodOnlineUpgradeApiDialogComObject {
    WestwoodOnlineUpgradeApiDialogVtable *vftable;
};

struct WestwoodOnlineUpgradeProgressWndVtable {
    void *reserved000[24];
    int(__fastcall *DestroyWindow)(
        CWnd *self,
        void *edx
    );
};

RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        RequestListMode
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        SubmitQueryRequest
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        LoadBrowseRecord
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        ResetQueryState
    ) == 0x24
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        Disconnect
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        SubmitEncodedQueryString
    ) == 0x3c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        SubmitPendingSessionList
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        QueueSessionRequest
    ) == 0x54
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        RequestUpgradeDownloadReadyResult
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        QueryStatusWithTokenAndServer
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        BeginConnectWithPreparedContext
    ) == 0x6c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        PrepareConnectContextAndMode
    ) == 0x70
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeApiDialogVtable,
        LookupBrowseRecordBySessionName
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeProgressWndVtable,
        DestroyWindow
    ) == 0x60
);

WestwoodOnlineUpgradeApiDialogComObject *GetDialogApiComObject() {
    return (WestwoodOnlineUpgradeApiDialogComObject *)g_pWestwoodOnlineUpgradeApi;
}
} // namespace

const RecoilNamedVtable kWestwoodOnlineUpgradeDialog_Vtable = {
    "WestwoodOnlineUpgradeDialog vtable"};

extern "C" {
HINSTANCE g_hWestwoodOnlineUpgradeModuleInstance = 0;
WestwoodOnlineUpgradeProgressDialog *g_pWestwoodOnlineUpgradeProgressDialog = 0;
WestwoodOnlineUpgradeDialog *g_pWestwoodOnlineUpgradeDialog = 0;
int g_WestwoodOnlineUpgradeSelectedMissionIndex = 0;
char g_WestwoodOnlineUpgradeStatusAppendBuffer[1024] = "";
}

namespace {
const unsigned int kStatusAppendBufferSize = 1024;
const unsigned int kStatusListMaxLineCount = 100;
const unsigned int kStatusListScrollThreshold = 9;
const unsigned int kStatusListScrollBackCount = 8;
const unsigned int kConnectStatusBufferSize = 128;
const unsigned int kConnectStatusMessageId = 0x3036;
const UINT kWestwoodOnlineUpgradeDialogResourceId = 154;
const UINT kWestwoodOnlineUpgradeProgressDialogResourceId = 157;
const int kWestwoodOnlineUpgradeProgressStatusControlId = 1179;
const int kWestwoodOnlineUpgradeServerAddressEditId = 1176;
const int kWestwoodOnlineUpgradeStatusTokenEditId = 1139;
const int kWestwoodOnlineUpgradeQueryValueOrTimeEditId = 1168;
const int kWestwoodOnlineUpgradeQueryMaxPlayersEditId = 1170;
const int kWestwoodOnlineUpgradeQueryAuxParamEditId = 1169;
const int kWestwoodOnlineUpgradeQueryStatusFlag1CheckId = 1123;
const int kWestwoodOnlineUpgradeQueryStatusFlag0CheckId = 1122;
const int kWestwoodOnlineUpgradeSubmitPendingSessionListButtonId = 1151;
const int kWestwoodOnlineUpgradeConnectButtonId = 1150;
const int kWestwoodOnlineUpgradeQueryValueOrTimeLabelId = 1167;
const int kWestwoodOnlineUpgradeQuerySessionsByNameButtonId = 1160;
const int kWestwoodOnlineUpgradeQueueVisibleSessionRequestsButtonId = 1051;
const int kWestwoodOnlineUpgradeStatusListId = 1141;
const int kWestwoodOnlineUpgradeSessionModeComboId = 1171;
const int kWestwoodOnlineUpgradeSessionResultsListId = 1137;
const int kWestwoodOnlineUpgradeStatusServerEditId = 1138;
const int kWestwoodOnlineUpgradeSessionNameEditId = 1148;
const int kWestwoodOnlineUpgradeBrowseRecordListId = 1136;
const int kWestwoodOnlineUpgradeResetBrowseRecordButtonId = 1150;
const int kWestwoodOnlineUpgradeDisconnectButtonId = 1166;
const int kWestwoodOnlineUpgradeBeginConnectButtonId = 1163;
const int kWestwoodOnlineUpgradeCheckUpgradeButtonId = 1162;
const int kWestwoodOnlineUpgradeQueryStatusButtonId = 1161;
const int kWestwoodOnlineUpgradeActiveListModeButtonId = 1144;
const int kWestwoodOnlineUpgradeRefreshCurrentQueryButtonId = 1143;
const int kWestwoodOnlineUpgradeQuerySessionsByNameCommandId = 1145;
const int kWestwoodOnlineUpgradeQueueVisibleSessionRequestsCommandId = 1160;
const int kWestwoodOnlineUpgradeQueueVisibleLookupCommandId = 1051;
const int kWestwoodOnlineUpgradeListMode0ButtonId = 1154;
const int kWestwoodOnlineUpgradeListMode11ButtonId = 1155;
const int kWestwoodOnlineUpgradeStatusCaptionLabelId = 1125;
const int kWestwoodOnlineUpgradeAutoRefreshListMode = 17;
const float kWestwoodOnlineUpgradeAutoRefreshIntervalSec = 60.0f;
const unsigned int kWolDisconnectStatusMessageId = 0x301a;
const DWORD kWolDestroyCallbackSleepMs = 200;
const UINT_PTR kWolRefreshListTimerId = 4;
const UINT kWolRefreshListTimerMs = 100;
const int kWolConnectRequestTextMaxChars = 9;
const WPARAM kWolSessionNameEditLimit = 17;
const WPARAM kWolServerAddressEditLimit = 9;
const WPARAM kWolStatusServerEditLimit = 100;
const WPARAM kWolStatusTokenEditLimit = 10;
const WPARAM kWolStatusListHorizontalExtent = 1500;
const int kWolQueryStatusTokenTextBufferSize = 20;
const int kWolQueryStatusTokenTextMaxChars = 19;
const int kWolQueryStatusServerTextBufferSize = 80;
const int kWolQueryStatusServerTextMaxChars = 79;
const int kWolQuerySessionsByNameServerTextMaxChars = 8;
const int kWolVisibleSessionSelectionLimit = 1024;
const int kWolQueryStatusMessageBoxTextBufferSize = 128;
const int kWolQueryStatusMessageBoxTitleBufferSize = 128;
const char kWolQueryStatusTokenDelimiter[] = " ";
const char kWolEncodedSessionListQueryFormat[] = "%1d%4d%4d%1d%1d%1d";
const char kWolSubmitStatusAppendFormat[] = "{ %s } %s";
const char kWolBootstrapServerTypeIrc[] = "IRC";
const int kWolQuerySessionNameCopyMaxChars = 16;
const int kWolQuerySessionsByNameListMode = 17;
const int kWolSessionModeCount = 7;
const unsigned int kWolSessionModeFirstMessageId = 0x2036;
const unsigned int kWolDialogDefaultSessionNameMessageId = 0x3034;
const unsigned int kWolStatusCaptionMessageId = 0x3037;
const int kWolMinPlayersPerSession = 2;
const int kWolMaxPlayersPerSession = 4;
const int kWolMinQueryAuxParam = 1;
const int kWolMaxQueryAuxParam = 1000;
const int kWolMinQueryValueOrTime = 2;
const int kWolMaxQueryValueOrTime = 2000;
const int kWolEditTextRadix = 10;
const unsigned int kWolEditTextBufferSize = 16;
const unsigned int kWolBeginConnectMode0MessageId = 0x303b;
const unsigned int kWolBeginConnectMode1MessageId = 0x303c;
const unsigned int kWolQueryStatusErrorTitleMessageId = 0x3003;
const unsigned int kWolQueryStatusMissingTokenMessageId = 0x303d;
const unsigned int kWolQuerySessionSubmitFailedMessageId = 0x303e;
const unsigned int kWolQuerySessionNameRequiredMessageId = 0x303f;
const unsigned int kWolQuerySessionModeTimeLabelMessageId = 0x3040;
const unsigned int kWolQuerySessionModeValueLabelMessageId = 0x3041;
const unsigned int kWolQuerySessionDuplicateMessageId = 0x3042;
const unsigned int kWolSubmitSessionRequestStatusMessageId = 0x3038;
const HRESULT kWolQueryDuplicateNameResult = 0x800401f7;
const HRESULT kWolQuerySubmitFailedResult = 0x800401f8;
const unsigned int kWolBootstrapServerCopiedBytes = 0xf8;
const unsigned int kStackStorageUnitSize = sizeof(unsigned int);
const UINT kMfcMessageMapSigVoid = 12;
const UINT kMfcMessageMapSigVoidUInt = 13;

RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeStatusAppendBuffer) == kStatusAppendBufferSize);

void DestructProgressDialog(
    WestwoodOnlineUpgradeProgressDialog *dialog
) {
    ((CDialog *)dialog)->CDialog::~CDialog();
}

void DestructMainDialog(
    WestwoodOnlineUpgradeDialog *dialog
) {
    dialog->Destructor();
}
} // namespace

const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeDialog::GetBaseMessageMapForMfc() {
    return WestwoodOnlineUpgradeCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const WestwoodOnlineUpgradeDialog::messageEntries[] = {
    {WM_TIMER,
        0,
        0,
        0,
        kMfcMessageMapSigVoidUInt,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnRefreshListTimer},
    {WM_DESTROY, 0, 0, 0, kMfcMessageMapSigVoid, (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnDestroy},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeResetBrowseRecordButtonId,
        kWestwoodOnlineUpgradeResetBrowseRecordButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeDisconnectButtonId,
        kWestwoodOnlineUpgradeDisconnectButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeBeginConnectButtonId,
        kWestwoodOnlineUpgradeBeginConnectButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::BeginConnect},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeCheckUpgradeButtonId,
        kWestwoodOnlineUpgradeCheckUpgradeButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueryStatusButtonId,
        kWestwoodOnlineUpgradeQueryStatusButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::QueryStatus},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeActiveListModeButtonId,
        kWestwoodOnlineUpgradeActiveListModeButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::RequestActiveListMode},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeRefreshCurrentQueryButtonId,
        kWestwoodOnlineUpgradeRefreshCurrentQueryButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQuerySessionsByNameCommandId,
        kWestwoodOnlineUpgradeQuerySessionsByNameCommandId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQuerySessionsByName},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueueVisibleSessionRequestsCommandId,
        kWestwoodOnlineUpgradeQueueVisibleSessionRequestsCommandId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueueVisibleLookupCommandId,
        kWestwoodOnlineUpgradeQueueVisibleLookupCommandId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeListMode0ButtonId,
        kWestwoodOnlineUpgradeListMode0ButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::RequestListMode0},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeListMode11ButtonId,
        kWestwoodOnlineUpgradeListMode11ButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::RequestListMode11},
    {WM_COMMAND,
        CBN_SELCHANGE,
        kWestwoodOnlineUpgradeSessionModeComboId,
        kWestwoodOnlineUpgradeSessionModeComboId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueryStatusFlag0CheckId,
        kWestwoodOnlineUpgradeQueryStatusFlag0CheckId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQueryControlsChanged},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeQueryStatusFlag1CheckId,
        kWestwoodOnlineUpgradeQueryStatusFlag1CheckId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQueryControlsChanged},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeSubmitPendingSessionListButtonId,
        kWestwoodOnlineUpgradeSubmitPendingSessionListButtonId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults},
    {WM_COMMAND,
        EN_CHANGE,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQueryControlsChanged},
    {WM_COMMAND,
        EN_CHANGE,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnQueryControlsChanged},
    {WM_COMMAND,
        EN_CHANGE,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange},
    {WM_COMMAND,
        LBN_DBLCLK,
        kWestwoodOnlineUpgradeBrowseRecordListId,
        kWestwoodOnlineUpgradeBrowseRecordListId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP WestwoodOnlineUpgradeDialog::messageMap = {
    &WestwoodOnlineUpgradeDialog::GetBaseMessageMapForMfc,
    &WestwoodOnlineUpgradeDialog::messageEntries[0],
};

// Reimplements 0x43dcc0: WestwoodOnlineUpgradeDialog::GetMessageMap
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
const AFX_MSGMAP * WestwoodOnlineUpgradeDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeDialog::messageMap;
}

// Reimplements 0x43dcd0: WestwoodOnlineUpgradeDialog::OnInitDialogBootstrap
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
int WestwoodOnlineUpgradeDialog::OnInitDialogBootstrap() {
    ((CDialog *)this)->CDialog::OnInitDialog();

    CString sessionModeNames[kWolSessionModeCount] = {
        zLoc::GetMessageString(kWolSessionModeFirstMessageId),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 1),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 2),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 3),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 4),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 5),
        zLoc::GetMessageString(kWolSessionModeFirstMessageId + 6)};

    m_sessionName.Format(zLoc::GetMessageString(kWolDialogDefaultSessionNameMessageId));

    for (int index = 0; index < kWolSessionModeCount; ++index) {
        LRESULT const itemIndex = ::SendMessageA(
            m_sessionModeCombo.m_hWnd,
            CB_ADDSTRING,
            0,
            (LPARAM)(const char *)sessionModeNames[index]
        );
        ::SendMessageA(
            m_sessionModeCombo.m_hWnd,
            CB_SETITEMDATA,
            itemIndex,
            index
        );
    }
    ::SendMessageA(
        m_sessionModeCombo.m_hWnd,
        CB_SETCURSEL,
        0,
        0
    );

    ((CWnd *)this)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeStatusCaptionLabelId,
            zLoc::GetMessageString(kWolStatusCaptionMessageId)
        );
    m_queryValueOrTime = 15;
    m_queryAuxParam = 10;
    m_queryMaxPlayers = 4;
    m_queryStatusFlagBit0 = 1;
    m_queryStatusFlagBit1 = 0;
    ((CWnd *)this)->UpdateData(0);

    m_statusLineCount = 0;
    EnableQueryControls(0);

    if (WestwoodOnlineUpgradeApi::Init() == 0) {
        OnDestroy();
        SetAbortAndClose();
        return 0;
    }

    ::SetTimer(
        m_hWnd,
        kWolRefreshListTimerId,
        kWolRefreshListTimerMs,
        0
    );
    ::SendMessageA(
        m_sessionNameEdit.m_hWnd,
        EM_LIMITTEXT,
        kWolSessionNameEditLimit,
        0
    );
    ::SendMessageA(
        m_serverAddressEdit.m_hWnd,
        EM_LIMITTEXT,
        kWolServerAddressEditLimit,
        0
    );
    ::SendMessageA(
        m_statusServerEdit.m_hWnd,
        EM_LIMITTEXT,
        kWolStatusServerEditLimit,
        0
    );
    ::SendMessageA(
        m_statusTokenEdit.m_hWnd,
        EM_LIMITTEXT,
        kWolStatusTokenEditLimit,
        0
    );
    ::SendMessageA(
        m_statusList.m_hWnd,
        LB_SETHORIZONTALEXTENT,
        kWolStatusListHorizontalExtent,
        0
    );
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 0;
    return 1;
}

// Reimplements 0x43f5d0: WestwoodOnlineUpgrade::TruncateStringAtFirstSpace
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void __fastcall WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(
    char *text
) {
    unsigned int index = 0;
    while (text[index] != '\0') {
        if (text[index] == ' ') {
            text[index] = '\0';
            return;
        }
        ++index;
    }
}

// Reimplements 0x43d740: WestwoodOnlineUpgradeDialog::Constructor
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
WestwoodOnlineUpgradeDialog * WestwoodOnlineUpgradeDialog::Constructor(
    CWnd *parentWnd
) {
    new ((CDialog *)this) CDialog(
        kWestwoodOnlineUpgradeDialogResourceId,
        parentWnd
    );

    new (&m_serverAddressEdit) CEdit();
    new (&m_statusTokenEdit) CEdit();
    new (&m_queryValueOrTimeEdit) CEdit();
    new (&m_queryMaxPlayersEdit) CEdit();
    new (&m_queryAuxParamEdit) CEdit();
    new (&m_queryStatusFlag1Check) CButton();
    new (&m_queryStatusFlag0Check) CButton();
    new (&m_submitPendingSessionListButton) CButton();
    new (&m_connectButton) CButton();
    new (&m_querySessionsByNameButton) CButton();
    new (&m_queueVisibleSessionRequestsButton) CButton();
    new (&m_statusList) CListBox();
    new (&m_sessionModeCombo) CComboBox();
    new (&m_sessionResultsList) CListBox();
    new (&m_statusServerEdit) CEdit();
    new (&m_sessionNameEdit) CEdit();
    new (&m_browseRecordList) CListBox();

    new (&m_selectedProfilePlayerName) CString();
    new (&m_selectedProfileConnectString) CString();
    new (&m_sessionName) CString();

    m_queryAuxParam = 0;
    m_queryMaxPlayers = 0;
    m_queryValueOrTime = 0;
    m_queryStatusFlagBit0 = 0;
    m_queryStatusFlagBit1 = 0;
    m_selectedProfileConnectStringMode = 0;
    return this;
}

// Reimplements 0x43d9a0: WestwoodOnlineUpgradeDialog::Destructor
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::Destructor() {
    m_sessionName.CString::~CString();
    m_selectedProfileConnectString.CString::~CString();
    m_selectedProfilePlayerName.CString::~CString();
    ((CListBox *)&m_browseRecordList)->CListBox::~CListBox();
    ((CEdit *)&m_sessionNameEdit)->CEdit::~CEdit();
    ((CEdit *)&m_statusServerEdit)->CEdit::~CEdit();
    ((CListBox *)&m_sessionResultsList)->CListBox::~CListBox();
    ((CComboBox *)&m_sessionModeCombo)->CComboBox::~CComboBox();
    ((CListBox *)&m_statusList)->CListBox::~CListBox();
    ((CButton *)&m_queueVisibleSessionRequestsButton)->CButton::~CButton();
    ((CButton *)&m_querySessionsByNameButton)->CButton::~CButton();
    ((CButton *)&m_connectButton)->CButton::~CButton();
    ((CButton *)&m_submitPendingSessionListButton)->CButton::~CButton();
    ((CButton *)&m_queryStatusFlag0Check)->CButton::~CButton();
    ((CButton *)&m_queryStatusFlag1Check)->CButton::~CButton();
    ((CEdit *)&m_queryAuxParamEdit)->CEdit::~CEdit();
    ((CEdit *)&m_queryMaxPlayersEdit)->CEdit::~CEdit();
    ((CEdit *)&m_queryValueOrTimeEdit)->CEdit::~CEdit();
    ((CEdit *)&m_statusTokenEdit)->CEdit::~CEdit();
    ((CEdit *)&m_serverAddressEdit)->CEdit::~CEdit();
    ((CDialog *)this)->CDialog::~CDialog();
}

// Reimplements 0x43d980: WestwoodOnlineUpgradeDialog::ScalarDeletingDestructor
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
WestwoodOnlineUpgradeDialog * WestwoodOnlineUpgradeDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    WestwoodOnlineUpgradeDialog *const self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }
    return self;
}

// Reimplements 0x43db20: WestwoodOnlineUpgradeDialog::DoDataExchange
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::DoDataExchange(
    CDataExchange *dataExchange
) {
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeServerAddressEditId,
        *((CWnd *)&m_serverAddressEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeStatusTokenEditId,
        *((CWnd *)&m_statusTokenEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        *((CWnd *)&m_queryValueOrTimeEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        *((CWnd *)&m_queryMaxPlayersEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        *((CWnd *)&m_queryAuxParamEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryStatusFlag1CheckId,
        *((CWnd *)&m_queryStatusFlag1Check)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueryStatusFlag0CheckId,
        *((CWnd *)&m_queryStatusFlag0Check)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeSubmitPendingSessionListButtonId,
        *((CWnd *)&m_submitPendingSessionListButton)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeConnectButtonId,
        *((CWnd *)&m_connectButton)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQuerySessionsByNameButtonId,
        *((CWnd *)&m_querySessionsByNameButton)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeQueueVisibleSessionRequestsButtonId,
        *((CWnd *)&m_queueVisibleSessionRequestsButton)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeStatusListId,
        *((CWnd *)&m_statusList)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeSessionModeComboId,
        *((CWnd *)&m_sessionModeCombo)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeSessionResultsListId,
        *((CWnd *)&m_sessionResultsList)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeStatusServerEditId,
        *((CWnd *)&m_statusServerEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeSessionNameEditId,
        *((CWnd *)&m_sessionNameEdit)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeBrowseRecordListId,
        *((CWnd *)&m_browseRecordList)
    );
    DDX_Text(
        dataExchange,
        kWestwoodOnlineUpgradeQueryAuxParamEditId,
        m_queryAuxParam
    );
    DDX_Text(
        dataExchange,
        kWestwoodOnlineUpgradeQueryMaxPlayersEditId,
        m_queryMaxPlayers
    );
    DDX_Text(
        dataExchange,
        kWestwoodOnlineUpgradeQueryValueOrTimeEditId,
        m_queryValueOrTime
    );
    DDX_Check(
        dataExchange,
        kWestwoodOnlineUpgradeQueryStatusFlag0CheckId,
        (int &)m_queryStatusFlagBit0
    );
    DDX_Check(
        dataExchange,
        kWestwoodOnlineUpgradeQueryStatusFlag1CheckId,
        (int &)m_queryStatusFlagBit1
    );
}

// Reimplements 0x43d060: WestwoodOnlineUpgradeDialog::AppendStatusTextFmt
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
int WestwoodOnlineUpgradeDialog::AppendStatusTextFmt(
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    vsprintf(
        g_WestwoodOnlineUpgradeStatusAppendBuffer,
        format,
        args
    );
    va_end(args);

    const unsigned int formattedLength =
        (unsigned int)strlen(g_WestwoodOnlineUpgradeStatusAppendBuffer);
    char *appendText = g_WestwoodOnlineUpgradeStatusAppendBuffer;
    if (g_WestwoodOnlineUpgradeStatusAppendBuffer[0] == '\n') {
        if (formattedLength == 1) {
            g_WestwoodOnlineUpgradeStatusAppendBuffer[0] = '\0';
        } else {
            appendText = &g_WestwoodOnlineUpgradeStatusAppendBuffer[1];
        }
    }

    if (formattedLength != 0 &&
        g_WestwoodOnlineUpgradeStatusAppendBuffer[formattedLength - 1] == '\n') {
        g_WestwoodOnlineUpgradeStatusAppendBuffer[formattedLength - 1] = '\0';
    }

    ::SendMessageA(
        m_statusList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)appendText
    );
    ++m_statusLineCount;
    if (m_statusLineCount > kStatusListMaxLineCount) {
        --m_statusLineCount;
        ::SendMessageA(
            m_statusList.m_hWnd,
            LB_DELETESTRING,
            0,
            0
        );
    }

    if (m_statusLineCount > kStatusListScrollThreshold) {
        return (int) ::SendMessageA(
            m_statusList.m_hWnd,
            LB_SETTOPINDEX,
            m_statusLineCount - kStatusListScrollBackCount,
            0
        );
    }

    return (int)m_statusLineCount;
}

// Reimplements 0x43ebd0: WestwoodOnlineUpgradeDialog::ClearStatusList
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::ClearStatusList() {
    ::SendMessageA(
        m_statusList.m_hWnd,
        LB_RESETCONTENT,
        0,
        0
    );
    m_statusLineCount = 0;
}

// Reimplements 0x442180: WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName(
    CString playerName
) {
    m_selectedProfilePlayerName = playerName;
}

// Reimplements 0x4421d0: WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString(
    CString connectString
) {
    m_selectedProfileConnectString = connectString;
}

// Reimplements 0x4416f0: WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
CString * WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName(
    CString *outName
) {
    outName->CString::CString(m_selectedProfilePlayerName);
    return outName;
}

// Reimplements 0x441720: WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
CString * WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString(
    CString *outConnectString
) {
    outConnectString->CString::CString(m_selectedProfileConnectString);
    return outConnectString;
}

// Reimplements 0x43dfe0: WestwoodOnlineUpgradeDialog::OnRefreshListTimer
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnRefreshListTimer(
    UINT_PTR
) {
    Time::Tick();
    if (g_WestwoodOnlineUpgradeProcessCallbacksFlag != 0) {
        WestwoodOnlineUpgradeApiDialogComObject *api = GetDialogApiComObject();
        api->vftable->ProcessCallbacks((IUnknown *)api);
        if (g_Time_UnscaledAccumulatedTimeSec >= g_WestwoodOnlineUpgradeNextAutoRefreshTime) {
            api = GetDialogApiComObject();
            g_WestwoodOnlineUpgradeNextAutoRefreshTime =
                g_Time_UnscaledAccumulatedTimeSec + kWestwoodOnlineUpgradeAutoRefreshIntervalSec;
            api->vftable
                ->RequestListMode(
                    (IUnknown *)api,
                    kWestwoodOnlineUpgradeAutoRefreshListMode,
                    1
                );
        }
    }

    ((WestwoodOnlineUpgradeCWndAccess *)this)->CallDefault();
}

// Reimplements 0x43e450: WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress() {
    if (g_pWestwoodOnlineUpgradeApi == 0 || g_WestwoodOnlineUpgradeDisconnectInFlightFlag != 0) {
        return;
    }

    ((CDialog *)g_pWestwoodOnlineUpgradeProgressDialog)
        ->Create(
            (LPCSTR)kWestwoodOnlineUpgradeProgressDialogResourceId,
            0
        );
    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeProgressStatusControlId,
            zLoc::GetMessageString(kWolDisconnectStatusMessageId)
        );

    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    api->vftable->Disconnect((IUnknown *)api);
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 1;
}

// Reimplements 0x43e4b0: WestwoodOnlineUpgradeDialog::BeginConnect
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::BeginConnect() {
    WestwoodOnlineUpgradeConnectContext context;
    ((CWnd *)&m_statusServerEdit)
        ->GetWindowTextA(
            context.m_requestText,
            kWolConnectRequestTextMaxChars
        );

    WestwoodOnlineUpgradeApiDialogComObject *api = GetDialogApiComObject();
    int mode = 0;
    unsigned int messageId = kWolBeginConnectMode0MessageId;
    if (api->vftable->PrepareConnectContextAndMode(
        (IUnknown *)api,
        &context
    ) != 0) {
        mode = 1;
        messageId = kWolBeginConnectMode1MessageId;
    }

    AppendStatusTextFmt(zLoc::GetMessageString(messageId));

    api = GetDialogApiComObject();
    api->vftable->BeginConnectWithPreparedContext(
        (IUnknown *)api,
        &context,
        mode
    );
}

// Reimplements 0x43e520: WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
int WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade() {
    WestwoodOnlineUpgradeConnectContext context;
    ((CWnd *)&m_statusServerEdit)
        ->GetWindowTextA(
            context.m_requestText,
            kWolConnectRequestTextMaxChars
        );

    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    return api->vftable->RequestUpgradeDownloadReadyResult(
        (IUnknown *)api,
        &context
    );
}

// Reimplements 0x43e550: WestwoodOnlineUpgradeDialog::QueryStatus
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
int WestwoodOnlineUpgradeDialog::QueryStatus() {
    char tokenInputText[kWolQueryStatusTokenTextBufferSize];
    ((CWnd *)&m_statusTokenEdit)->GetWindowTextA(
        tokenInputText,
        kWolQueryStatusTokenTextMaxChars
    );
    ((CWnd *)&m_statusTokenEdit)->SetWindowTextA("");

    char *token = strtok(
        tokenInputText,
        kWolQueryStatusTokenDelimiter
    );
    if (token != 0) {
        WestwoodOnlineUpgradeConnectContext tokenContext;
        strcpy(
            tokenContext.m_requestText,
            token
        );

        char serverText[kWolQueryStatusServerTextBufferSize];
        ((CWnd *)&m_statusServerEdit)
            ->GetWindowTextA(
                serverText,
                kWolQueryStatusServerTextMaxChars
            );
        ((CWnd *)&m_statusServerEdit)->SetWindowTextA("");

        WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
        return api->vftable
            ->QueryStatusWithTokenAndServer(
                (IUnknown *)api,
                &tokenContext,
                serverText
            );
    }

    char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
    strcpy(
        messageBoxTitle,
        zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
    );

    char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
    strcpy(
        messageBoxText,
        zLoc::GetMessageString(kWolQueryStatusMissingTokenMessageId)
    );

    return ((CWnd *)this)->MessageBoxA(
        messageBoxText,
        messageBoxTitle,
        MB_ICONHAND
    );
}

// Reimplements 0x43cf90: WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls() {
    CString encodedQuery;
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)->UpdateData(1);
    encodedQuery.Empty();

    LRESULT const selectedMode = ::SendMessageA(
        g_pWestwoodOnlineUpgradeDialog->m_sessionModeCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    WestwoodOnlineUpgradeDialog *const dialog = g_pWestwoodOnlineUpgradeDialog;
    encodedQuery.Format(
        kWolEncodedSessionListQueryFormat,
        selectedMode,
        dialog->m_queryValueOrTime,
        dialog->m_queryAuxParam,
        dialog->m_queryMaxPlayers,
        dialog->m_queryStatusFlagBit0,
        dialog->m_queryStatusFlagBit1
    );
    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    api->vftable->SubmitEncodedQueryString(
        (IUnknown *)api,
        encodedQuery
    );
}

// Reimplements 0x43e680: WestwoodOnlineUpgradeDialog::RequestActiveListMode
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::RequestActiveListMode() {
    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    api->vftable->RequestListMode(
        (IUnknown *)api,
        g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
}

// Reimplements 0x43e6a0: WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery() {
    if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0) {
        ResetSelectedBrowseRecordAndRefreshList();
    }

    CString sessionNameText;
    ((CWnd *)&m_sessionNameEdit)->GetWindowTextA(sessionNameText);
    sessionNameText.TrimLeft();
    sessionNameText.TrimRight();

    char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
    char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
    if (((const char *)sessionNameText)[0] == '\0') {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQuerySessionNameRequiredMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxText,
            messageBoxTitle,
            0
        );
        return;
    }

    if (strstr(
        sessionNameText,
        kWolQueryStatusTokenDelimiter
    ) != 0) {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQuerySessionSubmitFailedMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxText,
            messageBoxTitle,
            0
        );
        return;
    }

    ((CWnd *)&m_sessionNameEdit)->SetWindowTextA("");

    WestwoodOnlineUpgradeQueryRequest request;
    memset(
        &request,
        0,
        sizeof(request)
    );
    strncpy(
        request.m_sessionName,
        sessionNameText,
        kWolQuerySessionNameCopyMaxChars
    );
    request.m_listMode = 0;
    request.m_queryVariant = 2;
    request.m_queryMaxPlayers = m_queryMaxPlayers;
    request.m_queryFlags = 0;

    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
        api->vftable->ResetQueryState((IUnknown *)api);
    }

    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    int const result = api->vftable->SubmitQueryRequest(
        (IUnknown *)api,
        &request
    );
    if (result < 0) {
        unsigned int messageId = kWolQuerySessionSubmitFailedMessageId;
        if (result == kWolQueryDuplicateNameResult) {
            messageId = kWolQuerySessionDuplicateMessageId;
        }
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(zLoc::GetMessageString(messageId));
    }

    g_WestwoodOnlineUpgradeActiveListMode = 0;
}

// Reimplements 0x43e900: WestwoodOnlineUpgradeDialog::OnQuerySessionsByName
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnQuerySessionsByName() {
    CString sessionNameText;
    ((CWnd *)&m_sessionNameEdit)->GetWindowTextA(sessionNameText);
    sessionNameText.TrimLeft();
    sessionNameText.TrimRight();

    char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
    char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
    if (sessionNameText.IsEmpty()) {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQuerySessionNameRequiredMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxText,
            messageBoxTitle,
            0
        );
        return;
    }

    if (strstr(
        sessionNameText,
        kWolQueryStatusTokenDelimiter
    ) != 0) {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQuerySessionSubmitFailedMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxText,
            messageBoxTitle,
            0
        );
        return;
    }

    WestwoodOnlineUpgradeQueryRequest request;
    memset(
        &request,
        0,
        sizeof(request)
    );
    strncpy(
        request.m_sessionName,
        sessionNameText,
        kWolQuerySessionNameCopyMaxChars
    );
    request.m_listMode = kWolQuerySessionsByNameListMode;
    request.m_queryVariant = 2;
    request.m_queryMaxPlayers = m_queryMaxPlayers;
    request.m_queryExtraParam = 0;
    ((CWnd *)&m_serverAddressEdit)
        ->GetWindowTextA(
            request.m_serverAddress,
            kWolQuerySessionsByNameServerTextMaxChars
        );
    ((CWnd *)&m_serverAddressEdit)->SetWindowTextA(request.m_serverAddress);

    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
        api->vftable->ResetQueryState((IUnknown *)api);
    }

    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    int const result = api->vftable->SubmitQueryRequest(
        (IUnknown *)api,
        &request
    );
    if (result < 0) {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        unsigned int messageId = kWolQueryStatusErrorTitleMessageId;
        if (result == kWolQueryDuplicateNameResult) {
            messageId = kWolQuerySessionDuplicateMessageId;
        } else if (result == kWolQuerySubmitFailedResult) {
            messageId = kWolQuerySessionSubmitFailedMessageId;
        }
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(messageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxText,
            messageBoxTitle,
            0
        );
        return;
    }

    ((CWnd *)&m_sessionNameEdit)->SetWindowTextA("");
    m_sessionName = sessionNameText;
    ((CWnd *)this)->UpdateData(0);
    ClearStatusList();
    g_WestwoodOnlineUpgradeActiveListMode = kWolQuerySessionsByNameListMode;
    ((CWnd *)&m_querySessionsByNameButton)->EnableWindow(1);
    ((CWnd *)&m_queueVisibleSessionRequestsButton)->EnableWindow(1);
}

// Reimplements 0x43e1c0:
// WestwoodOnlineUpgradeDialog::SubmitVisibleSessionRequestsAndStatusText
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::SubmitVisibleSessionRequestsAndStatusText() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0') {
        return;
    }

    static CString pendingStatusText("");
    ((CWnd *)&m_statusServerEdit)->GetWindowTextA(pendingStatusText);
    if (pendingStatusText.IsEmpty()) {
        return;
    }

    ((CWnd *)&m_statusServerEdit)->SetWindowTextA("");

    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        (int) ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETSELCOUNT,
            0,
            0
        );
    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount == 0) {
        api->vftable->SubmitStatusText(
            (IUnknown *)api,
            pendingStatusText
        );
        AppendStatusTextFmt(
            kWolSubmitStatusAppendFormat,
            (const char *)m_selectedProfilePlayerName,
            (const char *)pendingStatusText
        );
        return;
    }

    LPARAM selectedRowIndices[kWolVisibleSessionSelectionLimit];
    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_GETSELITEMS,
        kWolVisibleSessionSelectionLimit,
        (LPARAM)selectedRowIndices
    );

    WestwoodOnlineUpgradeSessionRequest *sessionRequestList = 0;
    WestwoodOnlineUpgradeSessionRequest *sessionRequest = 0;
    for (int rowIndex = 0; rowIndex < g_WestwoodOnlineUpgradeVisibleSessionResultCount;
        ++rowIndex) {
        sessionRequest = new WestwoodOnlineUpgradeSessionRequest;
        memset(
            sessionRequest,
            0,
            sizeof(WestwoodOnlineUpgradeSessionRequest)
        );
        ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETTEXT,
            selectedRowIndices[rowIndex],
            (LPARAM)sessionRequest->m_sessionName
        );
        WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(sessionRequest->m_sessionName);
        sessionRequest->m_next = sessionRequestList;
        sessionRequestList = sessionRequest;
    }

    if (sessionRequest != 0) {
        char statusFormatBuffer[kConnectStatusBufferSize];
        zLoc::FormatMessage(
            statusFormatBuffer,
            kConnectStatusBufferSize,
            kWolSubmitSessionRequestStatusMessageId,
            sessionRequest->m_sessionName,
            (const char *)pendingStatusText
        );
        AppendStatusTextFmt(statusFormatBuffer);
    }

    api->vftable->SubmitSessionRequestListAndStatusText(
        (IUnknown *)api,
        sessionRequestList,
        pendingStatusText
    );

    while (sessionRequestList != 0) {
        WestwoodOnlineUpgradeSessionRequest *const nextSessionRequest = sessionRequestList->m_next;
        delete sessionRequestList;
        sessionRequestList = nextSessionRequest;
    }
}

// Reimplements 0x43f6b0: WestwoodOnlineUpgradeDialog::OnBootstrapServerList
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
int __stdcall WestwoodOnlineUpgradeDialog::OnBootstrapServerList(
    void *,
    int resultCode,
    WestwoodOnlineUpgradeBootstrapServerRecord *serverList
) {
    char debugText[4096];
    sprintf(
        debugText,
        "\nOnServerList:\n\tResult Code: %d\n",
        resultCode
    );
    zGame::ReturnOnlyStub();

    if (resultCode < 0) {
        SetEvent(g_WestwoodOnlineUpgradeFailureEvent);
        return 0;
    }

    int foundIrcServer = 0;
    WestwoodOnlineUpgradeBootstrapServerRecord *server = serverList;
    while (server != 0) {
        if (strcmp(
            server->m_serverType,
            kWolBootstrapServerTypeIrc
        ) == 0 && foundIrcServer == 0) {
            memcpy(
                &g_WestwoodOnlineUpgradeSelectedBootstrapServer,
                server,
                kWolBootstrapServerCopiedBytes
            );
            foundIrcServer = 1;
        }

        sprintf(
            debugText,
            "\n\tServer:\n\t\tName: %s\n\t\tType: %s\n\t\tConndata: %s\n\t\tGameType: %d\n",
            server->m_serverName,
            server->m_serverType,
            server->m_connectData,
            server->m_gameType
        );
        zGame::ReturnOnlyStub();
        server = server->m_next;
    }

    {
        CString playerName;
        g_pWestwoodOnlineUpgradeDialog->GetSelectedProfilePlayerName(&playerName);
        strcpy(
            g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_playerName,
            (const char *)playerName
        );
    }

    {
        CString connectString;
        g_pWestwoodOnlineUpgradeDialog->GetSelectedProfileConnectString(&connectString);
        strcpy(
            g_WestwoodOnlineUpgradeSelectedBootstrapServer.m_connectString,
            (const char *)connectString
        );
    }

    SetEvent(g_WestwoodOnlineUpgradeInitWaitEvents[0]);
    return 0;
}

// Reimplements 0x43ec00: WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0') {
        return;
    }

    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        (int) ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETSELCOUNT,
            0,
            0
        );
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount == 0 ||
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1) {
        return;
    }

    LPARAM selectedRowIndices[kWolVisibleSessionSelectionLimit];
    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_GETSELITEMS,
        kWolVisibleSessionSelectionLimit,
        (LPARAM)selectedRowIndices
    );

    WestwoodOnlineUpgradeSessionRequest *sessionRequest = new WestwoodOnlineUpgradeSessionRequest;
    for (int rowIndex = 0; rowIndex < g_WestwoodOnlineUpgradeVisibleSessionResultCount;
        ++rowIndex) {
        memset(
            sessionRequest,
            0,
            sizeof(WestwoodOnlineUpgradeSessionRequest)
        );
        ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETTEXT,
            selectedRowIndices[rowIndex],
            (LPARAM)sessionRequest->m_sessionName
        );
        WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(sessionRequest->m_sessionName);
        if (strstr(
            sessionRequest->m_sessionName,
            (const char *)m_selectedProfilePlayerName
        ) == 0) {
            WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
            api->vftable->QueueSessionRequest(
                (IUnknown *)api,
                sessionRequest
            );
        }
    }
    delete sessionRequest;
}

// Reimplements 0x43ed10:
// WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0') {
        return;
    }

    g_WestwoodOnlineUpgradeVisibleSessionResultCount =
        (int) ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETSELCOUNT,
            0,
            0
        );
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount == 0 ||
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1) {
        return;
    }

    LPARAM selectedRowIndices[kWolVisibleSessionSelectionLimit];
    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_GETSELITEMS,
        kWolVisibleSessionSelectionLimit,
        (LPARAM)selectedRowIndices
    );

    WestwoodOnlineUpgradeSessionRequest *sessionRequest = new WestwoodOnlineUpgradeSessionRequest;
    for (int rowIndex = 0; rowIndex < g_WestwoodOnlineUpgradeVisibleSessionResultCount;
        ++rowIndex) {
        memset(
            sessionRequest,
            0,
            sizeof(WestwoodOnlineUpgradeSessionRequest)
        );
        ::SendMessageA(
            m_sessionResultsList.m_hWnd,
            LB_GETTEXT,
            selectedRowIndices[rowIndex],
            (LPARAM)sessionRequest->m_sessionName
        );
        WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(sessionRequest->m_sessionName);
        if (strstr(
            sessionRequest->m_sessionName,
            (const char *)m_selectedProfilePlayerName
        ) == 0) {
            WestwoodOnlineUpgradeApiDialogComObject *api = GetDialogApiComObject();
            api->vftable->LookupBrowseRecordBySessionName(
                (IUnknown *)api,
                sessionRequest->m_sessionName,
                1
            );
            api = GetDialogApiComObject();
            api->vftable->QueueSessionRequest(
                (IUnknown *)api,
                sessionRequest
            );
        }
    }
    delete sessionRequest;
}

// Reimplements 0x43e040: WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk() {
    LRESULT const selectedIndex = ::SendMessageA(
        m_browseRecordList.m_hWnd,
        LB_GETCURSEL,
        0,
        0
    );
    if (selectedIndex != LB_ERR &&
        g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        if (strcmp(
                g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName,
                g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex].m_sessionName
            ) == 0) {
            return;
        }
        ResetSelectedBrowseRecordAndRefreshList();
    }

    WestwoodOnlineUpgradeBrowseRecord *const selectedRecord =
        &g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex];
    ((CWnd *)&m_serverAddressEdit)
        ->GetWindowTextA(
            selectedRecord->m_serverAddress,
            kWolQuerySessionsByNameServerTextMaxChars
        );
    ((CWnd *)&m_serverAddressEdit)->SetWindowTextA(selectedRecord->m_serverAddress);

    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    int const result = api->vftable->LoadBrowseRecord(
        (IUnknown *)api,
        selectedRecord
    );
    if (result == kWolQueryDuplicateNameResult) {
        g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
            zLoc::GetMessageString(kWolQuerySessionDuplicateMessageId)
        );
        return;
    }

    g_WestwoodOnlineUpgradeCachedBrowseRecord = *selectedRecord;
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_recordFlags != 0) {
        m_sessionName = g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName;
        ((CWnd *)this)->UpdateData(0);
    }
    ClearStatusList();
}

// Reimplements 0x43ee40: WestwoodOnlineUpgradeDialog::RequestListMode0
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::RequestListMode0() {
    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    g_WestwoodOnlineUpgradeActiveListMode = 0;
    api->vftable->RequestListMode(
        (IUnknown *)api,
        0,
        0
    );
}

// Reimplements 0x43ee60: WestwoodOnlineUpgradeDialog::RequestListMode11
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::RequestListMode11() {
    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    g_WestwoodOnlineUpgradeActiveListMode = kWestwoodOnlineUpgradeAutoRefreshListMode;
    api->vftable->RequestListMode(
        (IUnknown *)api,
        kWestwoodOnlineUpgradeAutoRefreshListMode,
        1
    );
}

// Reimplements 0x43ee80: WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange() {
    LRESULT const selectedIndex = ::SendMessageA(
        m_sessionModeCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    LRESULT const selectedModeKind =
        ::SendMessageA(
            m_sessionModeCombo.m_hWnd,
            CB_GETITEMDATA,
            selectedIndex,
            0
        );
    m_querySessionModeKind = selectedModeKind;
    if (selectedModeKind == 2) {
        ((CWnd *)this)
            ->SetDlgItemTextA(
                kWestwoodOnlineUpgradeQueryValueOrTimeLabelId,
                zLoc::GetMessageString(kWolQuerySessionModeTimeLabelMessageId)
            );
        ((CWnd *)&m_queryValueOrTimeEdit)->EnableWindow(0);
        UpdateSessionListQueryFromControls();
        return;
    }

    ((CWnd *)this)
        ->SetDlgItemTextA(
            kWestwoodOnlineUpgradeQueryValueOrTimeLabelId,
            zLoc::GetMessageString(kWolQuerySessionModeValueLabelMessageId)
        );
    ((CWnd *)&m_queryValueOrTimeEdit)->EnableWindow(1);
    UpdateSessionListQueryFromControls();
}

// Reimplements 0x43ef10: WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] == '\0' ||
        g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 1) {
        return;
    }

    WestwoodOnlineUpgradeSessionRequest *sessionRequestList = 0;
    for (int rowIndex = 0; rowIndex < g_WestwoodOnlineUpgradePendingSessionResultCount;
        ++rowIndex) {
        WestwoodOnlineUpgradeSessionRequest *sessionRequest =
            new WestwoodOnlineUpgradeSessionRequest;
        memset(
            sessionRequest,
            0,
            sizeof(WestwoodOnlineUpgradeSessionRequest)
        );
        if (::SendMessageA(
                m_sessionResultsList.m_hWnd,
                LB_GETTEXT,
                rowIndex,
                (LPARAM)sessionRequest->m_sessionName
            ) == LB_ERR) {
            delete sessionRequest;
        } else {
            WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(sessionRequest->m_sessionName);
            sessionRequest->m_next = sessionRequestList;
            sessionRequestList = sessionRequest;
        }
    }

    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    api->vftable->SubmitPendingSessionList(
        (IUnknown *)api,
        sessionRequestList
    );

    while (sessionRequestList != 0) {
        WestwoodOnlineUpgradeSessionRequest *const nextSessionRequest = sessionRequestList->m_next;
        delete sessionRequestList;
        sessionRequestList = nextSessionRequest;
    }
}

// Reimplements 0x43efc0: WestwoodOnlineUpgradeDialog::OnQueryControlsChanged
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnQueryControlsChanged() {
    UpdateSessionListQueryFromControls();
}

// Reimplements 0x43efd0: WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange() {
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)->UpdateData(1);
}

// Reimplements 0x43f450: WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus() {
    if ((int)m_queryMaxPlayers < kWolMinPlayersPerSession) {
        ((CWnd *)&m_queryMaxPlayersEdit)->SetWindowTextA("2");
        m_queryMaxPlayers = kWolMinPlayersPerSession;
        return;
    }

    if ((int)m_queryMaxPlayers > kWolMaxPlayersPerSession) {
        ((CWnd *)&m_queryMaxPlayersEdit)->SetWindowTextA("4");
        m_queryMaxPlayers = kWolMaxPlayersPerSession;
        return;
    }

    char valueText[kWolEditTextBufferSize];
    _ltoa(
        (long)m_queryMaxPlayers,
        valueText,
        kWolEditTextRadix
    );
    ((CWnd *)&m_queryMaxPlayersEdit)->SetWindowTextA(valueText);
}

// Reimplements 0x43f4d0: WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus() {
    if ((int)m_queryAuxParam < kWolMinQueryAuxParam) {
        ((CWnd *)&m_queryAuxParamEdit)->SetWindowTextA("1");
        m_queryAuxParam = kWolMinQueryAuxParam;
        return;
    }

    if ((int)m_queryAuxParam > kWolMaxQueryAuxParam) {
        ((CWnd *)&m_queryAuxParamEdit)->SetWindowTextA("1000");
        m_queryAuxParam = kWolMaxQueryAuxParam;
        return;
    }

    char valueText[kWolEditTextBufferSize];
    _ltoa(
        (long)m_queryAuxParam,
        valueText,
        kWolEditTextRadix
    );
    ((CWnd *)&m_queryAuxParamEdit)->SetWindowTextA(valueText);
}

// Reimplements 0x43f550: WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus() {
    if ((int)m_queryValueOrTime < kWolMinQueryValueOrTime) {
        ((CWnd *)&m_queryValueOrTimeEdit)->SetWindowTextA("2");
        m_queryValueOrTime = kWolMinQueryValueOrTime;
        return;
    }

    if ((int)m_queryValueOrTime > kWolMaxQueryValueOrTime) {
        ((CWnd *)&m_queryValueOrTimeEdit)->SetWindowTextA("2000");
        m_queryValueOrTime = kWolMaxQueryValueOrTime;
        return;
    }

    char valueText[kWolEditTextBufferSize];
    _ltoa(
        (long)m_queryValueOrTime,
        valueText,
        kWolEditTextRadix
    );
    ((CWnd *)&m_queryValueOrTimeEdit)->SetWindowTextA(valueText);
}

// Reimplements 0x43e160: WestwoodOnlineUpgradeDialog::OnDestroy
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::OnDestroy() {
    if (g_WestwoodOnlineUpgradeAbortFlag == 0) {
        BeginDisconnectAndShowProgress();
        while (g_WestwoodOnlineUpgradeAbortFlag == 0) {
            WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
            api->vftable->ProcessCallbacks((IUnknown *)api);
            Sleep(kWolDestroyCallbackSleepMs);
        }
    }

    ::KillTimer(
        m_hWnd,
        kWolRefreshListTimerId
    );
    WestwoodOnlineUpgradeApi::Shutdown();

    WestwoodOnlineUpgradeProgressWndVtable *const progressVtable =
        (WestwoodOnlineUpgradeProgressWndVtable *)(*(
            void **
        )g_pWestwoodOnlineUpgradeProgressDialog);
    progressVtable->DestroyWindow(
        (CWnd *)g_pWestwoodOnlineUpgradeProgressDialog,
        0
    );
}

// Reimplements 0x43d6b0: WestwoodOnlineUpgradeDialog::EnableQueryControls
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::EnableQueryControls(
    int enable
) {
    ((CWnd *)&m_sessionModeCombo)->EnableWindow(enable);
    ((CWnd *)&m_queryStatusFlag0Check)->EnableWindow(enable);
    ((CWnd *)&m_queryStatusFlag1Check)->EnableWindow(enable);
    ((CWnd *)&m_queryAuxParamEdit)->EnableWindow(enable);
    ((CWnd *)&m_queryValueOrTimeEdit)->EnableWindow(enable);
    ((CWnd *)&m_submitPendingSessionListButton)->EnableWindow(enable);
    ((CWnd *)&m_connectButton)->EnableWindow(enable);
}

// Reimplements 0x43d720: WestwoodOnlineUpgradeDialog::EnableConnectButton
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::EnableConnectButton(
    int enable
) {
    ((CWnd *)&m_connectButton)->EnableWindow(enable);
}

// Reimplements 0x43e3b0: WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
        api->vftable->ResetQueryState((IUnknown *)api);
        memset(
            &g_WestwoodOnlineUpgradeCachedBrowseRecord,
            0,
            sizeof(g_WestwoodOnlineUpgradeCachedBrowseRecord)
        );
    }

    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_RESETCONTENT,
        0,
        0
    );
    g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag = 0;
    g_pWestwoodOnlineUpgradeDialog->EnableQueryControls(0);
    g_pWestwoodOnlineUpgradeDialog->EnableConnectButton(0);
    ((CWnd *)&m_querySessionsByNameButton)->EnableWindow(0);
    ((CWnd *)&m_queueVisibleSessionRequestsButton)->EnableWindow(0);

    WestwoodOnlineUpgradeApiDialogComObject *const api = GetDialogApiComObject();
    api->vftable->RequestListMode(
        (IUnknown *)api,
        g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
}

// Reimplements 0x43d650: WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList(
    const char *sessionName
) {
    char statusText[kConnectStatusBufferSize];
    zLoc::FormatMessage(
        statusText,
        kConnectStatusBufferSize,
        kConnectStatusMessageId,
        sessionName
    );
    AppendStatusTextFmt(statusText);
    ResetSelectedBrowseRecordAndRefreshList();
}

// Reimplements 0x43d6a0: WestwoodOnlineUpgradeDialog::SetAbortAndClose
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
void WestwoodOnlineUpgradeDialog::SetAbortAndClose() {
    g_WestwoodOnlineUpgradeAbortFlag = 1;
    ((CDialogCancelAccessor *)this)->CallBaseOnCancel();
}

// Reimplements 0x43efe0: WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
int __fastcall
WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(
    int *selectedMissionIndexOut
) {
    unsigned int dialogStorage
        [(sizeof(WestwoodOnlineUpgradeDialog) + kStackStorageUnitSize - 1) / kStackStorageUnitSize];
    unsigned int progressDialogStorage
        [(sizeof(WestwoodOnlineUpgradeProgressDialog) + kStackStorageUnitSize - 1) /
            kStackStorageUnitSize];
    WestwoodOnlineUpgradeDialog *const dialog = (WestwoodOnlineUpgradeDialog *)dialogStorage;
    WestwoodOnlineUpgradeProgressDialog *const progressDialog =
        (WestwoodOnlineUpgradeProgressDialog *)progressDialogStorage;

    g_hWestwoodOnlineUpgradeModuleInstance =
        (HINSTANCE)((unsigned int)(g_RecoilApp.m_hInstance));
    dialog->Constructor(0);
    progressDialog->Constructor(0);

    g_pWestwoodOnlineUpgradeProgressDialog = progressDialog;
    g_pWestwoodOnlineUpgradeDialog = dialog;
    ((CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd())))->SetMenuBarVisibility(0);

    g_WestwoodOnlineUpgradeSelectedMissionIndex = -1;
    ((CDialog *)dialog)->CDialog::DoModal();

    ((CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd())))->SetMenuBarVisibility(1);

    const int selectedMissionIndex = g_WestwoodOnlineUpgradeSelectedMissionIndex;
    if (selectedMissionIndex == -1) {
        DestructProgressDialog(progressDialog);
        DestructMainDialog(dialog);
        return 0;
    }

    *selectedMissionIndexOut = selectedMissionIndex;
    DestructProgressDialog(progressDialog);
    DestructMainDialog(dialog);
    return 1;
}
