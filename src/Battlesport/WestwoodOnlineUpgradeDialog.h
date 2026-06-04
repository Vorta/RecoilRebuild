#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

struct WestwoodOnlineUpgradeBootstrapServerRecord;
struct WestwoodOnlineUpgradeDownloadReadyEntry;

// Authored Recoil dialog reconstructed over imported MFC42 CDialog. MFC
// dialog behavior is provided by MFC42, not reimplemented in this source tree.
struct WestwoodOnlineUpgradeProgressDialog : CDialog {
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    const AFX_MSGMAP * GetMessageMap() const;
    static BOOL SetStatusTextFmt(
        const char *format,
        ...
    );
    WestwoodOnlineUpgradeProgressDialog * Constructor(
        CWnd *parentWnd
    );
    void Destructor();
    WestwoodOnlineUpgradeProgressDialog * ScalarDeletingDestructor(
        unsigned int flags
    );
    static BOOL CALLBACK DlgProc(
        HWND hWnd,
        UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
    );
};

// Authored Recoil dialog reconstructed over imported MFC42 CDialog and
// control classes. MFC control behavior is provided by MFC42, not reimplemented
// in this source tree.
struct WestwoodOnlineUpgradeDialog : CDialog {
    CEdit m_serverAddressEdit;
    CEdit m_statusTokenEdit;
    CEdit m_queryValueOrTimeEdit;
    CEdit m_queryMaxPlayersEdit;
    CEdit m_queryAuxParamEdit;
    CButton m_queryStatusFlag1Check;
    CButton m_queryStatusFlag0Check;
    CButton m_submitPendingSessionListButton;
    CButton m_connectButton;
    CButton m_querySessionsByNameButton;
    CButton m_queueVisibleSessionRequestsButton;
    CListBox m_statusList;
    CComboBox m_sessionModeCombo;
    CListBox m_sessionResultsList;
    CEdit m_statusServerEdit;
    CEdit m_sessionNameEdit;
    CListBox m_browseRecordList;
    unsigned int m_queryAuxParam;
    unsigned int m_queryMaxPlayers;
    unsigned int m_queryValueOrTime;
    unsigned int m_queryStatusFlagBit0;
    unsigned int m_queryStatusFlagBit1;
    unsigned int m_querySessionModeKind;
    CString m_selectedProfilePlayerName;
    CString m_selectedProfileConnectString;
    CString m_sessionName;
    unsigned int m_selectedProfileConnectStringMode;
    unsigned int m_statusLineCount;

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    const AFX_MSGMAP * GetMessageMap() const;
    int OnInitDialogBootstrap();
    WestwoodOnlineUpgradeDialog * Constructor(CWnd *parentWnd);
    void Destructor();
    WestwoodOnlineUpgradeDialog * ScalarDeletingDestructor(
        unsigned int flags
    );
    void DoDataExchange(CDataExchange *dataExchange);
    int AppendStatusTextFmt(
        const char *format,
        ...
    );
    void SetSelectedProfilePlayerName(CString playerName);
    void SetSelectedProfileConnectString(CString connectString);
    CString * GetSelectedProfilePlayerName(CString *outName);
    CString * GetSelectedProfileConnectString(
        CString *outConnectString
    );
    void OnRefreshListTimer(UINT_PTR timerId);
    void BeginDisconnectAndShowProgress();
    void BeginConnect();
    int CheckAndApplyUpgrade();
    int QueryStatus();
    void UpdateSessionListQueryFromControls();
    void RequestActiveListMode();
    void RequestListMode0();
    void RequestListMode11();
    void OnRefreshCurrentQuery();
    void OnQuerySessionsByName();
    void SubmitVisibleSessionRequestsAndStatusText();
    void QueueVisibleSessionRequests();
    void QueueVisibleSessionRequestsAndLookupBrowseRecords();
    void OnBrowseRecordListDblClk();
    void OnSessionModeComboSelChange();
    void SubmitPendingSessionListFromResults();
    void OnQueryControlsChanged();
    void OnMaxPlayersEditChange();
    void OnMaxPlayersEditKillFocus();
    void OnAuxParamEditKillFocus();
    void OnValueOrTimeEditKillFocus();
    void OnDestroy();
    void EnableQueryControls(int enable);
    void EnableConnectButton(int enable);
    void ResetSelectedBrowseRecordAndRefreshList();
    void ClearStatusList();
    void AppendConnectStatusAndRefreshList(const char *sessionName);
    void SetAbortAndClose();
    static int __fastcall ShowModalAndGetSelectedMissionIndex(
        int *selectedMissionIndexOut
    );
    static int __fastcall ShowDownloadReadyList(
        WestwoodOnlineUpgradeDownloadReadyEntry *readyListHead
    );
    static int __stdcall OnBootstrapServerList(
        void *callbackContext,
        int resultCode,
        WestwoodOnlineUpgradeBootstrapServerRecord *serverList
    );
};

extern const RecoilNamedVtable kWestwoodOnlineUpgradeDialog_Vtable;
extern const RecoilNamedVtable kWestwoodOnlineUpgradeProgressDialog_Vtable;

extern "C" HINSTANCE g_hWestwoodOnlineUpgradeModuleInstance;
extern "C" WestwoodOnlineUpgradeProgressDialog *g_pWestwoodOnlineUpgradeProgressDialog;
extern "C" WestwoodOnlineUpgradeDialog *g_pWestwoodOnlineUpgradeDialog;
extern "C" char g_WestwoodOnlineUpgradeProgressStatusTextBuffer[1024];
extern "C" int g_WestwoodOnlineUpgradeSelectedMissionIndex;
extern "C" char g_WestwoodOnlineUpgradeStatusAppendBuffer[1024];

namespace WestwoodOnlineUpgrade {
void __fastcall TruncateStringAtFirstSpace(char *text);
}

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeProgressDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeDialog) == 0x4cc);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_serverAddressEdit
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_statusTokenEdit
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_queryValueOrTimeEdit
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_queryMaxPlayersEdit
    ) == 0x120
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_queryAuxParamEdit
    ) == 0x160
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_queryStatusFlag1Check
    ) == 0x1a0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_queryStatusFlag0Check
    ) == 0x1e0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_submitPendingSessionListButton
    ) == 0x220
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_connectButton
    ) == 0x260
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_querySessionsByNameButton
    ) == 0x2a0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_queueVisibleSessionRequestsButton
    ) == 0x2e0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_statusList
    ) == 0x320
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_sessionModeCombo
    ) == 0x360
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_sessionResultsList
    ) == 0x3a0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_statusServerEdit
    ) == 0x3e0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_sessionNameEdit
    ) == 0x420
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_browseRecordList
    ) == 0x460
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_queryAuxParam
    ) == 0x4a0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_querySessionModeKind
    ) == 0x4b4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_selectedProfilePlayerName
    ) == 0x4b8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_selectedProfileConnectString
    ) == 0x4bc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_sessionName
    ) == 0x4c0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_selectedProfileConnectStringMode
    ) == 0x4c4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeDialog,
        m_statusLineCount
    ) == 0x4c8
);
