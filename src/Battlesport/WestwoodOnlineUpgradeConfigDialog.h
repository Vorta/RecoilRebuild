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
    CEdit m_serverNameEdit;
    CComboBox m_profileCombo;
    CString m_passwordText;
    CString m_noPasswordText;
    int m_wolPasswordFlag;
    CString m_profileNames[2];
    CString m_profileDisplayNames[2];
    CString m_profilePlayerNames[2];
    CString m_profileConnectStrings[2];
    int m_profileConnectStringModes[2];
    int m_selectedProfileIndex;

    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static const AFX_MSGMAP *RECOIL_STDCALL GetBaseMessageMapForMfc();
    RECOIL_WOL_CONFIG_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL GetMessageMap() const;
    RECOIL_WOL_CONFIG_NOINLINE WestwoodOnlineUpgradeConfigDialog *RECOIL_THISCALL
    Constructor(CWnd *parentWnd);
    RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL
    GetSelectedProfileValues(char **playerNameOut,
                             char **connectStringOut,
                             int *connectStringModeOut);
    RECOIL_WOL_CONFIG_NOINLINE static int RECOIL_CDECL
    ShowModalAndApplySelectedProfileValues();
};

extern const RecoilNamedVtable kWestwoodOnlineUpgradeConfigDialog_Vtable;

RECOIL_STATIC_ASSERT(sizeof(WestwoodOnlineUpgradeConfigDialog) == 0x118);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_serverNameEdit) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_profileCombo) == 0xa0);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_passwordText) == 0xe0);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_noPasswordText) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_wolPasswordFlag) == 0xe8);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_profileNames) == 0xec);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_profileDisplayNames) == 0xf4);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_profilePlayerNames) == 0xfc);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_profileConnectStrings) == 0x104);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_profileConnectStringModes) == 0x10c);
RECOIL_STATIC_ASSERT(offsetof(WestwoodOnlineUpgradeConfigDialog, m_selectedProfileIndex) == 0x114);
