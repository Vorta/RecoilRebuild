#include "Battlesport/wol_dialog.h"

#include "Battlesport/cz_recoil_frame.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Provider-boundary accessor for imported MFC42 CDialog::OnCancel; this does not reimplement
 * CDialog behavior.
 */
class CDialogCancelAccessor : public CDialog {
  public:
    void CallBaseOnCancel();
};

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does not reimplement
 * CDialog behavior.
 */
class WestwoodOnlineUpgradeCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * Provider-boundary accessor for imported MFC42 CWnd::Default; this does not reimplement
 * CWnd behavior.
 */
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
    int &value
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

/**
 * Original helper evidence: no standalone retail function; observed in
 * WestwoodOnlineUpgradeDialog::SetAbortAndClose.
 * Purpose: call imported MFC42 CDialog::OnCancel for the dialog wrapper.
 */
void CDialogCancelAccessor::CallBaseOnCancel() {
    CDialog::OnCancel();
}

/**
 * Original helper evidence: no standalone retail function; observed in the
 * WestwoodOnlineUpgradeDialog MFC message-map chain.
 * Purpose: expose imported MFC42 CDialog message-map metadata to the dialog.
 */
const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; observed in
 * WestwoodOnlineUpgradeDialog::OnRefreshListTimer.
 * Purpose: call imported MFC42 CWnd::Default for timer message fallthrough.
 */
long WestwoodOnlineUpgradeCWndAccess::CallDefault() {
    return CWnd::Default();
}

extern "C" {
/**
 * Reimplements data 0x4f5238: g_hWestwoodOnlineUpgradeModuleInstance.
 * Purpose: stores the module instance passed to
 * WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex for the
 * stack-owned upgrade dialog lifetime.
 */
HINSTANCE g_hWestwoodOnlineUpgradeModuleInstance = 0;
/**
 * Reimplements data 0x4f4230: g_pWestwoodOnlineUpgradeProgressDialog.
 * Purpose: stores the active stack-owned progress dialog while the
 * Westwood online upgrade modal flow is running.
 */
WestwoodOnlineUpgradeProgressDialog *g_pWestwoodOnlineUpgradeProgressDialog = 0;
/**
 * Reimplements data 0x538568: g_pWestwoodOnlineUpgradeDialog.
 * Purpose: stores the active stack-owned main upgrade dialog used by static
 * callbacks and member routines during the modal flow.
 */
WestwoodOnlineUpgradeDialog *g_pWestwoodOnlineUpgradeDialog = 0;
/**
 * Reimplements data 0x4f53c0: g_WestwoodOnlineUpgradeSelectedMissionIndex.
 * Purpose: stores the mission index selected by the Westwood online upgrade
 * dialog before ShowModalAndGetSelectedMissionIndex returns it.
 */
int g_WestwoodOnlineUpgradeSelectedMissionIndex = 0;
/**
 * Reimplements data 0x538990: g_WestwoodOnlineUpgradeStatusAppendBuffer.
 * Purpose: holds the reusable formatted status text buffer consumed by
 * WestwoodOnlineUpgradeDialog::AppendStatusTextFmt.
 */
char g_WestwoodOnlineUpgradeStatusAppendBuffer[1024] = "";
}

/**
 * Reimplements data 0x4dd25c: g_WestwoodOnlineUpgradeSessionQueryPayloadFmt.
 * Purpose: CString::Format payload used by
 * WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls to encode
 * the provider session-list query fields.
 */
char g_WestwoodOnlineUpgradeSessionQueryPayloadFmt[] = "%1d%4d%4d%1d%1d%1d";
/**
 * Reimplements data 0x4dd270: g_WestwoodOnlineUpgradeSessionQueryDisplayFmt.
 * Purpose: status-list format used when submitting typed status text without
 * selected session rows.
 */
char g_WestwoodOnlineUpgradeSessionQueryDisplayFmt[] = "{ %s } %s";
/**
 * Reimplements data 0x4dd27c: g_WestwoodOnlineUpgradeSingleDigitFieldMaxText_4.
 * Purpose: normalized max-player edit text for the upper single-digit query
 * bound.
 */
char g_WestwoodOnlineUpgradeSingleDigitFieldMaxText_4[] = "4";
/**
 * Reimplements data 0x4dd280: g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2.
 * Purpose: normalized edit text for the lower single-digit query bound shared
 * by max-player and value/time controls.
 */
char g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2[] = "2";
/**
 * Reimplements data 0x4dd284: g_WestwoodOnlineUpgradeMaxPlayersMaxText_1000.
 * Purpose: normalized auxiliary-parameter edit text for the 1000 upper bound.
 */
char g_WestwoodOnlineUpgradeMaxPlayersMaxText_1000[] = "1000";
/**
 * Reimplements data 0x4dd28c: g_WestwoodOnlineUpgradeMaxPlayersMinText_1.
 * Purpose: normalized auxiliary-parameter edit text for the lower query bound.
 */
char g_WestwoodOnlineUpgradeMaxPlayersMinText_1[] = "1";
/**
 * Reimplements data 0x4dd290: g_WestwoodOnlineUpgradeAuxParamMaxText_2000.
 * Purpose: normalized value/time edit text for the 2000 upper bound.
 */
char g_WestwoodOnlineUpgradeAuxParamMaxText_2000[] = "2000";

namespace {
const unsigned int kStatusAppendBufferSize = 1024;
const int kStatusListMaxLineCount = 100;
const int kStatusListScrollThreshold = 9;
const int kStatusListScrollBackCount = 8;
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
const UINT kMfcMessageMapSigVoid = 12;
const UINT kMfcMessageMapSigVoidUInt = 13;

RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeStatusAppendBuffer) == kStatusAppendBufferSize);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeSessionQueryPayloadFmt) == 0x13);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeSessionQueryDisplayFmt) == 0x0a);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeSingleDigitFieldMaxText_4) == 0x02);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2) == 0x02);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeMaxPlayersMaxText_1000) == 0x05);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeMaxPlayersMinText_1) == 0x02);
RECOIL_STATIC_ASSERT(sizeof(g_WestwoodOnlineUpgradeAuxParamMaxText_2000) == 0x05);

} // namespace

/**
 * Original helper evidence: no standalone retail function; observed in the
 * WestwoodOnlineUpgradeDialog MFC message-map chain.
 * Purpose: return the imported CDialog base message map for MFC dispatch.
 */
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

/**
 * Reimplements 0x43dcc0: WestwoodOnlineUpgradeDialog::GetMessageMap
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: return the recovered MFC message map for the upgrade dialog.
 */
const AFX_MSGMAP * WestwoodOnlineUpgradeDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeDialog::messageMap;
}

/**
 * Reimplements 0x43dcd0: WestwoodOnlineUpgradeDialog::OnInitDialog
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: initialize dialog controls, defaults, provider state, and refresh
 * timer.
 */
BOOL WestwoodOnlineUpgradeDialog::OnInitDialog() {
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

/**
 * Reimplements 0x43f5d0: WestwoodOnlineUpgrade::TruncateStringAtFirstSpace
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: trim list-box session text down to the first token before provider
 * requests.
 */
void __fastcall WestwoodOnlineUpgrade::TruncateStringAtFirstSpace(
    char *text
) {
    unsigned int index = 0;
    while (index < strlen(text)) {
        if (text[index] == ' ') {
            text[index] = '\0';
            return;
        }
        ++index;
    }
}

/**
 * Reimplements 0x43d740: WestwoodOnlineUpgradeDialog::WestwoodOnlineUpgradeDialog
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: construct the CDialog-derived upgrade dialog and its embedded MFC
 * controls.
 */
WestwoodOnlineUpgradeDialog::WestwoodOnlineUpgradeDialog(
    CWnd *parentWnd
) :
    CDialog(
        kWestwoodOnlineUpgradeDialogResourceId,
        parentWnd
    ),
    m_serverAddressEdit(),
    m_statusTokenEdit(),
    m_queryValueOrTimeEdit(),
    m_queryMaxPlayersEdit(),
    m_queryAuxParamEdit(),
    m_queryStatusFlag1Check(),
    m_queryStatusFlag0Check(),
    m_submitPendingSessionListButton(),
    m_connectButton(),
    m_querySessionsByNameButton(),
    m_queueVisibleSessionRequestsButton(),
    m_statusList(),
    m_sessionModeCombo(),
    m_sessionResultsList(),
    m_statusServerEdit(),
    m_sessionNameEdit(),
    m_browseRecordList(),
    m_selectedProfilePlayerName(),
    m_selectedProfileConnectString(),
    m_sessionName()
{
    m_queryAuxParam = 0;
    m_queryMaxPlayers = 0;
    m_queryValueOrTime = 0;
    m_queryStatusFlagBit0 = 0;
    m_queryStatusFlagBit1 = 0;
    m_selectedProfileConnectStringMode = 0;
}

/**
 * Original helper evidence: no standalone retail function; recovered callers
 * still use this placement-constructor helper while the owner model is the
 * real C++ constructor above.
 * Purpose: placement-construct the main WOL upgrade dialog and return self.
 */
WestwoodOnlineUpgradeDialog * WestwoodOnlineUpgradeDialog::Constructor(
    CWnd *parentWnd
) {
    new (this) WestwoodOnlineUpgradeDialog(parentWnd);
    return this;
}

/**
 * Reimplements 0x43d9a0: WestwoodOnlineUpgradeDialog::Destructor
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Evidence: retail matches VC5's implicit derived destructor; declaring a
 * source-level ~WestwoodOnlineUpgradeDialog body adds a non-retail derived
 * vftable reset before member cleanup.
 * Purpose: invoke the implicit dialog destructor for recovered teardown callers.
 */
void WestwoodOnlineUpgradeDialog::Destructor() {
    this->WestwoodOnlineUpgradeDialog::~WestwoodOnlineUpgradeDialog();
}

/**
 * Reimplements 0x43d980: WestwoodOnlineUpgradeDialog::ScalarDeletingDestructor
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: run the dialog destructor and optionally free scalar-delete storage.
 */
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

/**
 * Reimplements 0x43db20: WestwoodOnlineUpgradeDialog::DoDataExchange
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: bind MFC controls and synchronize query option fields.
 */
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

/**
 * Reimplements 0x43d060: WestwoodOnlineUpgradeDialog::AppendStatusTextFmt
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: format and append a status line while keeping the status list
 * bounded and scrolled.
 */
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

    if (g_WestwoodOnlineUpgradeStatusAppendBuffer[formattedLength - 1] == '\n') {
        g_WestwoodOnlineUpgradeStatusAppendBuffer[formattedLength - 1] = '\0';
    }

    ::SendMessageA(
        m_statusList.m_hWnd,
        LB_ADDSTRING,
        0,
        (LPARAM)appendText
    );
    ++m_statusLineCount;
    if ((int)m_statusLineCount > kStatusListMaxLineCount) {
        --m_statusLineCount;
        ::SendMessageA(
            m_statusList.m_hWnd,
            LB_DELETESTRING,
            0,
            0
        );
    }

    if ((int)m_statusLineCount > kStatusListScrollThreshold) {
        return (int) ::SendMessageA(
            m_statusList.m_hWnd,
            LB_SETTOPINDEX,
            m_statusLineCount - kStatusListScrollBackCount,
            0
        );
    }

    return (int)m_statusLineCount;
}

/**
 * Reimplements 0x43ebd0: WestwoodOnlineUpgradeDialog::ClearStatusList
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clear the visible status list and reset the dialog line counter.
 */
void WestwoodOnlineUpgradeDialog::ClearStatusList() {
    ::SendMessageA(
        m_statusList.m_hWnd,
        LB_RESETCONTENT,
        0,
        0
    );
    m_statusLineCount = 0;
}

/**
 * Reimplements 0x442180: WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: store the selected Westwood Online profile player name.
 */
void WestwoodOnlineUpgradeDialog::SetSelectedProfilePlayerName(
    CString playerName
) {
    m_selectedProfilePlayerName = playerName;
}

/**
 * Reimplements 0x4421d0: WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: store the selected Westwood Online profile connection string.
 */
void WestwoodOnlineUpgradeDialog::SetSelectedProfileConnectString(
    CString connectString
) {
    m_selectedProfileConnectString = connectString;
}

/**
 * Reimplements 0x4416f0: WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: return the selected profile player name for callback use.
 */
CString WestwoodOnlineUpgradeDialog::GetSelectedProfilePlayerName() {
    return m_selectedProfilePlayerName;
}

/**
 * Reimplements 0x441720: WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: return the selected profile connection string for callback use.
 */
CString WestwoodOnlineUpgradeDialog::GetSelectedProfileConnectString() {
    return m_selectedProfileConnectString;
}

/**
 * Reimplements 0x43dfe0: WestwoodOnlineUpgradeDialog::OnRefreshListTimer
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: tick time, pump provider callbacks, and request periodic list
 * refreshes.
 */
void WestwoodOnlineUpgradeDialog::OnRefreshListTimer(
    UINT_PTR
) {
    Time::Tick();
    if (g_WestwoodOnlineUpgradeProcessCallbacksFlag != 0) {
        IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        api->ProcessCallbacks();
        if (g_Time_UnscaledAccumulatedTimeSec > g_WestwoodOnlineUpgradeNextAutoRefreshTime) {
            api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            g_WestwoodOnlineUpgradeNextAutoRefreshTime =
                g_Time_UnscaledAccumulatedTimeSec - (-60.0);
            api->RequestListMode(kWestwoodOnlineUpgradeAutoRefreshListMode,
                    1
                );
        }
    }

    ((WestwoodOnlineUpgradeCWndAccess *)this)->CallDefault();
}

/**
 * Reimplements 0x43e450: WestwoodOnlineUpgradeDialog::BeginDisconnectAndShowProgress
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: show the progress dialog and start provider disconnect processing.
 */
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

    IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->Disconnect();
    g_WestwoodOnlineUpgradeDisconnectInFlightFlag = 1;
}

/**
 * Reimplements 0x43e4b0: WestwoodOnlineUpgradeDialog::BeginConnect
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: gather connection text, update status, and start provider connect.
 */
void WestwoodOnlineUpgradeDialog::BeginConnect() {
    WestwoodOnlineUpgradeConnectContext context;
    ((CWnd *)&m_statusServerEdit)
        ->GetWindowTextA(
            context.m_requestText,
            kWolConnectRequestTextMaxChars
        );

    IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    int mode;
    if (api->PrepareConnectContextAndMode(&context
    ) == 0) {
        mode = 0;
        AppendStatusTextFmt(zLoc::GetMessageString(kWolBeginConnectMode0MessageId));
    } else {
        mode = 1;
        AppendStatusTextFmt(zLoc::GetMessageString(kWolBeginConnectMode1MessageId));
    }

    api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->BeginConnectWithPreparedContext(&context,
        mode
    );
}

/**
 * Reimplements 0x43e520: WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: ask the provider whether the requested upgrade download is ready.
 */
int WestwoodOnlineUpgradeDialog::CheckAndApplyUpgrade() {
    WestwoodOnlineUpgradeConnectContext context;
    ((CWnd *)&m_statusServerEdit)
        ->GetWindowTextA(
            context.m_requestText,
            kWolConnectRequestTextMaxChars
        );

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    return api->RequestUpgradeDownloadReadyResult(&context
    );
}

/**
 * Reimplements 0x43e550: WestwoodOnlineUpgradeDialog::QueryStatus
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: submit a status-token query or display the localized missing-token
 * error.
 */
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

        IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        return api->QueryStatusWithTokenAndServer(&tokenContext,
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

/**
 * Reimplements 0x43cf90: WestwoodOnlineUpgradeDialog::UpdateSessionListQueryFromControls
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: encode the global dialog's current query controls and submit the
 * query string to the provider.
 */
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
        g_WestwoodOnlineUpgradeSessionQueryPayloadFmt,
        selectedMode,
        dialog->m_queryValueOrTime,
        dialog->m_queryAuxParam,
        dialog->m_queryMaxPlayers,
        dialog->m_queryStatusFlagBit0,
        dialog->m_queryStatusFlagBit1
    );
    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->SubmitEncodedQueryString(encodedQuery
    );
}

/**
 * Reimplements 0x43e680: WestwoodOnlineUpgradeDialog::RequestActiveListMode
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: refresh the provider list using the dialog's active list mode.
 */
void WestwoodOnlineUpgradeDialog::RequestActiveListMode() {
    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->RequestListMode(g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
}

/**
 * Reimplements 0x43e6a0: WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: validate the current session query and submit it to the provider.
 */
void WestwoodOnlineUpgradeDialog::OnRefreshCurrentQuery() {
    if (g_WestwoodOnlineUpgradeCreateSessionFromQueryFlag != 0) {
        ResetSelectedBrowseRecordAndRefreshList();
    }

    CString sessionNameText;
    ((CWnd *)&m_sessionNameEdit)->GetWindowTextA(sessionNameText);
    sessionNameText.TrimLeft();
    sessionNameText.TrimRight();

    if (((const char *)sessionNameText)[0] == '\0') {
        char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
        char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQuerySessionNameRequiredMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxTitle,
            messageBoxText,
            0
        );
        return;
    }

    if (strstr(
        sessionNameText,
        kWolQueryStatusTokenDelimiter
    ) != 0) {
        char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
        char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
        strcpy(
            messageBoxText,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQuerySessionSubmitFailedMessageId)
        );
        ((CWnd *)this)->MessageBoxA(
            messageBoxTitle,
            messageBoxText,
            0
        );
        return;
    }

    m_sessionNameEdit.SetWindowTextA("");

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
        IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        api->ResetQueryState();
    }

    IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    int result = api->SubmitQueryRequest(&request
    );
    if (result < 0) {
        if (result != kWolQueryDuplicateNameResult) {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kWolQuerySessionSubmitFailedMessageId)
            );
        } else {
            g_pWestwoodOnlineUpgradeDialog->AppendStatusTextFmt(
                zLoc::GetMessageString(kWolQuerySessionDuplicateMessageId)
            );
        }
    }

    g_WestwoodOnlineUpgradeActiveListMode = 0;
}

/**
 * Reimplements 0x43e900: WestwoodOnlineUpgradeDialog::OnQuerySessionsByName
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: validate a named session query, submit it, and update dialog state.
 */
void WestwoodOnlineUpgradeDialog::OnQuerySessionsByName() {
    CString sessionNameText;
    m_sessionNameEdit.GetWindowTextA(sessionNameText);
    sessionNameText.TrimLeft();
    sessionNameText.TrimRight();
    CWnd *const serverAddressEdit = &m_serverAddressEdit;
    char messageBoxText[kWolQueryStatusMessageBoxTextBufferSize];
    char messageBoxTitle[kWolQueryStatusMessageBoxTitleBufferSize];
    if (sessionNameText.GetLength() == 0) {
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
    serverAddressEdit->GetWindowTextA(
        request.m_serverAddress,
        kWolQuerySessionsByNameServerTextMaxChars
    );
    serverAddressEdit->SetWindowTextA(request.m_serverAddress);

    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        api->ResetQueryState();
    }

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    int result = api->SubmitQueryRequest(&request
    );
    if (result < 0) {
        strcpy(
            messageBoxTitle,
            zLoc::GetMessageString(kWolQueryStatusErrorTitleMessageId)
        );
        unsigned int messageId = kWolQueryStatusErrorTitleMessageId;
        if (result != kWolQueryDuplicateNameResult) {
            if (result == kWolQuerySubmitFailedResult) {
                messageId = kWolQuerySessionSubmitFailedMessageId;
            }
        } else {
            messageId = kWolQuerySessionDuplicateMessageId;
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

    m_sessionNameEdit.SetWindowTextA("");
    m_sessionName = sessionNameText;
    ((CWnd *)this)->UpdateData(0);
    ClearStatusList();
    g_WestwoodOnlineUpgradeActiveListMode = kWolQuerySessionsByNameListMode;
    m_querySessionsByNameButton.EnableWindow(1);
    m_queueVisibleSessionRequestsButton.EnableWindow(1);
}

/**
 * Reimplements 0x43e1c0:
 *     WestwoodOnlineUpgradeDialog::SubmitVisibleSessionRequestsAndStatusText
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: submit visible session requests with pending status text.
 */
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
    if (g_WestwoodOnlineUpgradeVisibleSessionResultCount == 0) {
        ((IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi)->SubmitStatusText(
            pendingStatusText
        );
        AppendStatusTextFmt(
            g_WestwoodOnlineUpgradeSessionQueryDisplayFmt,
            (const char *)m_selectedProfilePlayerName,
            (const char *)pendingStatusText
        );
        return;
    }

    LPARAM selectedRowIndices[kWolVisibleSessionSelectionLimit];
    WestwoodOnlineUpgradeSessionRequest *sessionRequestList = 0;
    WestwoodOnlineUpgradeSessionRequest *sessionRequest;
    ::SendMessageA(
        m_sessionResultsList.m_hWnd,
        LB_GETSELITEMS,
        kWolVisibleSessionSelectionLimit,
        (LPARAM)selectedRowIndices
    );

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

    char statusFormatBuffer[kConnectStatusBufferSize];
    zLoc::FormatMessage(
        statusFormatBuffer,
        kConnectStatusBufferSize,
        kWolSubmitSessionRequestStatusMessageId,
        sessionRequest->m_sessionName,
        (const char *)pendingStatusText
    );
    AppendStatusTextFmt(statusFormatBuffer);

    ((IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi)->SubmitSessionRequestListAndStatusText(
        sessionRequest,
        pendingStatusText
    );

    while (sessionRequestList != 0) {
        WestwoodOnlineUpgradeSessionRequest *const nextSessionRequest = sessionRequestList->m_next;
        delete sessionRequestList;
        sessionRequestList = nextSessionRequest;
    }
}

/**
 * Reimplements 0x43ec00: WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: queue selected visible session names with the provider.
 */
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
            IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            api->QueueSessionRequest(sessionRequest
            );
        }
    }
    delete sessionRequest;
}

/**
 * Reimplements 0x43ed10:
 *     WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequestsAndLookupBrowseRecords
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: look up selected browse records and queue matching session requests.
 */
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
            IWestwoodOnlineUpgradeProviderApi *api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            api->LookupBrowseRecordBySessionName(sessionRequest->m_sessionName,
                1
            );
            api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            api->QueueSessionRequest(sessionRequest
            );
        }
    }
    delete sessionRequest;
}

/**
 * Reimplements 0x43e040: WestwoodOnlineUpgradeDialog::OnBrowseRecordListDblClk
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: load a selected browse record and update the dialog selection.
 */
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

    ((CWnd *)&m_serverAddressEdit)
        ->GetWindowTextA(
            g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex].m_serverAddress,
            kWolQuerySessionsByNameServerTextMaxChars
        );
    ((CWnd *)&m_serverAddressEdit)->SetWindowTextA(
        g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex].m_serverAddress
    );

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    WestwoodOnlineUpgradeBrowseRecord *const selectedRecord =
        &g_WestwoodOnlineUpgradeCachedBrowseRecordList[selectedIndex];
    int const result = api->LoadBrowseRecord(selectedRecord
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

/**
 * Reimplements 0x43ee40: WestwoodOnlineUpgradeDialog::RequestListMode0
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: switch to list mode 0 and request provider refresh.
 */
void WestwoodOnlineUpgradeDialog::RequestListMode0() {
    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    g_WestwoodOnlineUpgradeActiveListMode = 0;
    api->RequestListMode(0,
        0
    );
}

/**
 * Reimplements 0x43ee60: WestwoodOnlineUpgradeDialog::RequestListMode11
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: switch to the auto-refresh list mode and request provider refresh.
 */
void WestwoodOnlineUpgradeDialog::RequestListMode11() {
    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    g_WestwoodOnlineUpgradeActiveListMode = kWestwoodOnlineUpgradeAutoRefreshListMode;
    api->RequestListMode(kWestwoodOnlineUpgradeAutoRefreshListMode,
        1
    );
}

/**
 * Reimplements 0x43ee80: WestwoodOnlineUpgradeDialog::OnSessionModeComboSelChange
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: update query label/edit state after the session-mode selection
 * changes.
 */
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

/**
 * Reimplements 0x43ef10: WestwoodOnlineUpgradeDialog::SubmitPendingSessionListFromResults
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: build and submit the pending session request list from result rows.
 */
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

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->SubmitPendingSessionList(sessionRequestList
    );

    while (sessionRequestList != 0) {
        WestwoodOnlineUpgradeSessionRequest *const nextSessionRequest = sessionRequestList->m_next;
        delete sessionRequestList;
        sessionRequestList = nextSessionRequest;
    }
}

/**
 * Reimplements 0x43efc0: WestwoodOnlineUpgradeDialog::OnQueryControlsChanged
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: refresh the encoded provider query after control changes.
 */
void WestwoodOnlineUpgradeDialog::OnQueryControlsChanged() {
    UpdateSessionListQueryFromControls();
}

/**
 * Reimplements 0x43efd0: WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: synchronize max-player edit text into dialog data.
 */
void WestwoodOnlineUpgradeDialog::OnMaxPlayersEditChange() {
    ((CWnd *)g_pWestwoodOnlineUpgradeDialog)->UpdateData(1);
}

/**
 * Reimplements 0x43f450: WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clamp and normalize the max-player query edit value.
 */
void WestwoodOnlineUpgradeDialog::OnMaxPlayersEditKillFocus() {
    if ((int)m_queryMaxPlayers < kWolMinPlayersPerSession) {
        ((CWnd *)&m_queryMaxPlayersEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2);
        m_queryMaxPlayers = kWolMinPlayersPerSession;
        return;
    }

    if ((int)m_queryMaxPlayers > kWolMaxPlayersPerSession) {
        ((CWnd *)&m_queryMaxPlayersEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeSingleDigitFieldMaxText_4);
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

/**
 * Reimplements 0x43f4d0: WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clamp and normalize the auxiliary query parameter edit value.
 */
void WestwoodOnlineUpgradeDialog::OnAuxParamEditKillFocus() {
    if ((int)m_queryAuxParam < kWolMinQueryAuxParam) {
        ((CWnd *)&m_queryAuxParamEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeMaxPlayersMinText_1);
        m_queryAuxParam = kWolMinQueryAuxParam;
        return;
    }

    if ((int)m_queryAuxParam > kWolMaxQueryAuxParam) {
        ((CWnd *)&m_queryAuxParamEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeMaxPlayersMaxText_1000);
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

/**
 * Reimplements 0x43f550: WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clamp and normalize the value-or-time query edit value.
 */
void WestwoodOnlineUpgradeDialog::OnValueOrTimeEditKillFocus() {
    if ((int)m_queryValueOrTime < kWolMinQueryValueOrTime) {
        ((CWnd *)&m_queryValueOrTimeEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeSingleDigitFieldMinText_2);
        m_queryValueOrTime = kWolMinQueryValueOrTime;
        return;
    }

    if ((int)m_queryValueOrTime > kWolMaxQueryValueOrTime) {
        ((CWnd *)&m_queryValueOrTimeEdit)
            ->SetWindowTextA(g_WestwoodOnlineUpgradeAuxParamMaxText_2000);
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

/**
 * Reimplements 0x43e160: WestwoodOnlineUpgradeDialog::OnDestroy
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: disconnect, stop refresh timing, and shut down the provider API.
 */
void WestwoodOnlineUpgradeDialog::OnDestroy() {
    if (g_WestwoodOnlineUpgradeAbortFlag == 0) {
        BeginDisconnectAndShowProgress();
        while (g_WestwoodOnlineUpgradeAbortFlag == 0) {
            IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
            api->ProcessCallbacks();
            Sleep(kWolDestroyCallbackSleepMs);
        }
    }

    ::KillTimer(
        m_hWnd,
        kWolRefreshListTimerId
    );
    WestwoodOnlineUpgradeApi::Shutdown();

    ((CWnd *)g_pWestwoodOnlineUpgradeProgressDialog)->DestroyWindow();
}

/**
 * Reimplements 0x43d6b0: WestwoodOnlineUpgradeDialog::EnableQueryControls
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: enable or disable the dialog controls used to build session
 * queries.
 */
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

/**
 * Reimplements 0x43d720: WestwoodOnlineUpgradeDialog::EnableConnectButton
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: enable or disable the provider connect button.
 */
void WestwoodOnlineUpgradeDialog::EnableConnectButton(
    int enable
) {
    ((CWnd *)&m_connectButton)->EnableWindow(enable);
}

/**
 * Reimplements 0x43e3b0: WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: clear the selected browse record and request a refreshed list.
 */
void WestwoodOnlineUpgradeDialog::ResetSelectedBrowseRecordAndRefreshList() {
    if (g_WestwoodOnlineUpgradeCachedBrowseRecord.m_sessionName[0] != '\0') {
        IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
        api->ResetQueryState();
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

    IWestwoodOnlineUpgradeProviderApi *const api = (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    api->RequestListMode(g_WestwoodOnlineUpgradeActiveListMode,
        1
    );
}

/**
 * Reimplements 0x43d650: WestwoodOnlineUpgradeDialog::AppendConnectStatusAndRefreshList
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: append connection status text and refresh the browse list.
 */
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

/**
 * Reimplements 0x43d6a0: WestwoodOnlineUpgradeDialog::SetAbortAndClose
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: mark the upgrade flow aborted and close through MFC cancel.
 */
void WestwoodOnlineUpgradeDialog::SetAbortAndClose() {
    g_WestwoodOnlineUpgradeAbortFlag = 1;
    ((CDialogCancelAccessor *)this)->CallBaseOnCancel();
}

/**
 * Reimplements 0x43efe0: WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex
 * Source: D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp
 * Purpose: run the modal upgrade dialog and return the selected mission index.
 */
int __fastcall WestwoodOnlineUpgradeDialog::ShowModalAndGetSelectedMissionIndex(
    int *selectedMissionIndexOut
) {
    g_hWestwoodOnlineUpgradeModuleInstance =
        (HINSTANCE)((unsigned int)(g_RecoilApp.m_hInstance));
    WestwoodOnlineUpgradeDialog dialog(0);
    WestwoodOnlineUpgradeProgressDialog progressDialog(0);

    g_pWestwoodOnlineUpgradeProgressDialog = &progressDialog;
    g_pWestwoodOnlineUpgradeDialog = &dialog;
    ((CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd())))->SetMenuBarVisibility(0);

    g_WestwoodOnlineUpgradeSelectedMissionIndex = -1;
    g_pWestwoodOnlineUpgradeDialog->DoModal();

    ((CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd())))->SetMenuBarVisibility(1);

    const int selectedMissionIndex = g_WestwoodOnlineUpgradeSelectedMissionIndex;
    if (selectedMissionIndex != -1) {
        *selectedMissionIndexOut = selectedMissionIndex;
        return 1;
    }

    return 0;
}
