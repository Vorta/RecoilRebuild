#pragma once

#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/WestwoodOnlineUpgradeDialog.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#if defined(_MSC_VER) && _MSC_VER >= 1300
#define RECOIL_WOL_CONFIG_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define RECOIL_WOL_CONFIG_NOINLINE __attribute__((noinline))
#else
#define RECOIL_WOL_CONFIG_NOINLINE
#endif

// Authored Recoil dialog reconstructed over imported MFC42 CDialog and
// control classes. MFC control behavior is provided by MFC42, not reimplemented
// in this source tree.
struct WestwoodOnlineUpgradeConfigDialog : CDialog {
  public:
    CComboBox m_profileCombo;
    CEdit m_connectStringEdit;
    CString m_reservedString;
    CString m_connectStringEditText;
    int m_wolPasswordFlag;
    CString m_savedPlayerNames[2];
    CString m_savedConnectStrings[2];
    CString m_profilePlayerNames[2];
    CString m_profileConnectStrings[2];
    int m_profileConnectStringModes[2];
    int m_selectedProfileIndex;
    int m_profileComboEditDirty;

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static const AFX_MSGMAP *RECOIL_STDCALL GetBaseMessageMapForMfc();
    RECOIL_WOL_CONFIG_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL GetMessageMap() const;
    RECOIL_WOL_CONFIG_NOINLINE WestwoodOnlineUpgradeConfigDialog *RECOIL_THISCALL Constructor(
        CWnd *parentWnd
    );
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL Destructor();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL DoDataExchange(CDataExchange *dataExchange);
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL OnConnectStringEditSetFocusClear();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL OnConnectStringEditKillFocus();
    RECOIL_WOL_CONFIG_NOINLINE BOOL RECOIL_THISCALL OnInitDialog();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL OnOK();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL OnProfileComboKillFocus();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL OnProfileComboSelChange();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL OnProfileComboEditChange();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL OnProfileComboDropdown();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL OnConnectStringModeClicked();
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL GetSelectedProfileValues(
        char **playerNameOut,
        char **connectStringOut,
        int *connectStringModeOut
    );
    RECOIL_WOL_CONFIG_NOINLINE static int RECOIL_CDECL ShowModalAndApplySelectedProfileValues();
};

extern const RecoilNamedVtable kWestwoodOnlineUpgradeConfigDialog_Vtable;

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeConfigDialog) == 0x11c);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_profileCombo
    ) == 0x60
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_connectStringEdit
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_reservedString
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_connectStringEditText
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_wolPasswordFlag
    ) == 0xe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_savedPlayerNames
    ) == 0xec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_savedConnectStrings
    ) == 0xf4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_profilePlayerNames
    ) == 0xfc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_profileConnectStrings
    ) == 0x104
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_profileConnectStringModes
    ) == 0x10c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_selectedProfileIndex
    ) == 0x114
);
RECOIL_STATIC_ASSERT(
    offsetof(
        WestwoodOnlineUpgradeConfigDialog,
        m_profileComboEditDirty
    ) == 0x118
);
