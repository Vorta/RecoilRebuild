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

    static const AFX_MSGMAP *RECOIL_STDCALL GetBaseMessageMapForMfc();
    RECOIL_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL GetMessageMap() const;
    RECOIL_NOINLINE static BOOL RECOIL_CDECL SetStatusTextFmt(
        const char *format,
        ...
    );
    RECOIL_NOINLINE WestwoodOnlineUpgradeProgressDialog *RECOIL_THISCALL Constructor(
        CWnd *parentWnd
    );
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE WestwoodOnlineUpgradeProgressDialog *RECOIL_THISCALL ScalarDeletingDestructor(
        unsigned int flags
    );
    RECOIL_NOINLINE static BOOL CALLBACK DlgProc(
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

    static const AFX_MSGMAP *RECOIL_STDCALL GetBaseMessageMapForMfc();
    RECOIL_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL GetMessageMap() const;
    RECOIL_NOINLINE int RECOIL_THISCALL OnInitDialogBootstrap();
    RECOIL_NOINLINE WestwoodOnlineUpgradeDialog *RECOIL_THISCALL Constructor(CWnd *parentWnd);
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_NOINLINE WestwoodOnlineUpgradeDialog *RECOIL_THISCALL ScalarDeletingDestructor(
        unsigned int flags
    );
    RECOIL_NOINLINE void RECOIL_THISCALL DoDataExchange(CDataExchange *dataExchange);
    RECOIL_NOINLINE int RECOIL_CDECL AppendStatusTextFmt(
        const char *format,
        ...
    );
    RECOIL_NOINLINE void RECOIL_THISCALL SetSelectedProfilePlayerName(CString playerName);
    RECOIL_NOINLINE void RECOIL_THISCALL SetSelectedProfileConnectString(CString connectString);
    RECOIL_NOINLINE CString *RECOIL_THISCALL GetSelectedProfilePlayerName(CString *outName);
    RECOIL_NOINLINE CString *RECOIL_THISCALL GetSelectedProfileConnectString(
        CString *outConnectString
    );
    RECOIL_NOINLINE void RECOIL_THISCALL OnRefreshListTimer(UINT_PTR timerId);
    RECOIL_NOINLINE void RECOIL_THISCALL BeginDisconnectAndShowProgress();
    RECOIL_NOINLINE void RECOIL_THISCALL BeginConnect();
    RECOIL_NOINLINE int RECOIL_THISCALL CheckAndApplyUpgrade();
    RECOIL_NOINLINE int RECOIL_THISCALL QueryStatus();
    RECOIL_NOINLINE void RECOIL_THISCALL UpdateSessionListQueryFromControls();
    RECOIL_NOINLINE void RECOIL_THISCALL RequestActiveListMode();
    RECOIL_NOINLINE void RECOIL_THISCALL RequestListMode0();
    RECOIL_NOINLINE void RECOIL_THISCALL RequestListMode11();
    RECOIL_NOINLINE void RECOIL_THISCALL OnRefreshCurrentQuery();
    RECOIL_NOINLINE void RECOIL_THISCALL OnQuerySessionsByName();
    RECOIL_NOINLINE void RECOIL_THISCALL SubmitVisibleSessionRequestsAndStatusText();
    RECOIL_NOINLINE void RECOIL_THISCALL QueueVisibleSessionRequests();
    RECOIL_NOINLINE void RECOIL_THISCALL QueueVisibleSessionRequestsAndLookupBrowseRecords();
    RECOIL_NOINLINE void RECOIL_THISCALL OnBrowseRecordListDblClk();
    RECOIL_NOINLINE void RECOIL_THISCALL OnSessionModeComboSelChange();
    RECOIL_NOINLINE void RECOIL_THISCALL SubmitPendingSessionListFromResults();
    RECOIL_NOINLINE void RECOIL_THISCALL OnQueryControlsChanged();
    RECOIL_NOINLINE void RECOIL_THISCALL OnMaxPlayersEditChange();
    RECOIL_NOINLINE void RECOIL_THISCALL OnMaxPlayersEditKillFocus();
    RECOIL_NOINLINE void RECOIL_THISCALL OnAuxParamEditKillFocus();
    RECOIL_NOINLINE void RECOIL_THISCALL OnValueOrTimeEditKillFocus();
    RECOIL_NOINLINE void RECOIL_THISCALL OnDestroy();
    RECOIL_NOINLINE void RECOIL_THISCALL EnableQueryControls(int enable);
    RECOIL_NOINLINE void RECOIL_THISCALL EnableConnectButton(int enable);
    RECOIL_NOINLINE void RECOIL_THISCALL ResetSelectedBrowseRecordAndRefreshList();
    RECOIL_NOINLINE void RECOIL_THISCALL ClearStatusList();
    RECOIL_NOINLINE void RECOIL_THISCALL AppendConnectStatusAndRefreshList(const char *sessionName);
    RECOIL_NOINLINE void RECOIL_THISCALL SetAbortAndClose();
    RECOIL_NOINLINE static int RECOIL_FASTCALL ShowModalAndGetSelectedMissionIndex(
        int *selectedMissionIndexOut
    );
    RECOIL_NOINLINE static int RECOIL_FASTCALL ShowDownloadReadyList(
        WestwoodOnlineUpgradeDownloadReadyEntry *readyListHead
    );
    RECOIL_NOINLINE static int RECOIL_STDCALL OnBootstrapServerList(
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
RECOIL_NOINLINE void RECOIL_FASTCALL TruncateStringAtFirstSpace(char *text);
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
