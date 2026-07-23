#pragma once

#include <stddef.h>

#include "recoil/Mfc42Abi.h"
#include "Battlesport/wol_dialog.h"
#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

/**
 * Authored Recoil dialog reconstructed over imported MFC42 CDialog and
 * control classes. MFC control behavior is provided by MFC42, not reimplemented
 * in this source tree.
 */
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

    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    WestwoodOnlineUpgradeConfigDialog(CWnd *parentWnd);
    virtual ~WestwoodOnlineUpgradeConfigDialog();
    virtual const AFX_MSGMAP * GetMessageMap() const;
    WestwoodOnlineUpgradeConfigDialog * Constructor(
        CWnd *parentWnd
    );
    void Destructor();
    virtual void DoDataExchange(CDataExchange *dataExchange);
    void OnConnectStringEditSetFocusClear();
    void OnConnectStringEditKillFocus();
    virtual BOOL OnInitDialog();
    virtual void OnOK();
    void OnProfileComboKillFocus();
    void OnProfileComboSelChange();
    void OnProfileComboEditChange();
    void OnProfileComboDropdown();
    void OnConnectStringModeClicked();
    void GetSelectedProfileValues(
        char **playerNameOut,
        char **connectStringOut,
        int *connectStringModeOut
    );
    static int ShowModalAndApplySelectedProfileValues();
};

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
