#include "Battlesport/WestwoodOnlineUpgradeConfigDialog.h"

#include "Battlesport/WestwoodOnlineUpgradeApi.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <string.h>

/**
 * Provider-boundary accessor for imported MFC42 CDialog members; this does not reimplement
 * provider behavior.
 */
class CDialogProviderAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
    void CallOnOK();
};

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CEdit) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CComboBox) == 0x40);

void __stdcall DDX_Control(
    CDataExchange *dataExchange,
    int controlId,
    CWnd &control
);
void __stdcall DDX_Text(
    CDataExchange *dataExchange,
    int controlId,
    CString &value
);
void __stdcall DDX_Check(
    CDataExchange *dataExchange,
    int controlId,
    int &value
);

namespace {
const UINT kWestwoodOnlineUpgradeConfigDialogResourceId = 156;
const char kEmptyString[] = "";
const int kWestwoodOnlineUpgradeConfigProfileComboId = 1192;
const int kWestwoodOnlineUpgradeConfigConnectStringEditId = 1173;
const int kWestwoodOnlineUpgradeConfigRememberPasswordCheckId = 1182;
const UINT kMfcMessageMapSigVoid = 12;
const unsigned int kWestwoodOnlineUpgradeConfigUnnamedProfileMessageId = 0x3044;
const int kSelectedProfileTextBufferLength = 32;
const int kDialogOkResult = 1;
const unsigned int kStackStorageUnitSize = sizeof(unsigned int);

/**
 * Original helper evidence: no standalone retail function; observed at
 * callers 0x441cb0 and 0x441f40 source-cluster cleanup sites.
 * Purpose: centralizes config-dialog destructor dispatch for local stack objects.
 */
void DestructConfigDialog(
    WestwoodOnlineUpgradeConfigDialog *dialog
) {
    dialog->Destructor();
}
} // namespace

/**
 * Provider-boundary: imported MFC42 CDialog message map accessor.
 * Purpose: exposes the CDialog base message map for the recovered MFC map chain.
 */
const AFX_MSGMAP *__stdcall CDialogProviderAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Provider-boundary: imported MFC42 CDialog::OnOK member call.
 * Purpose: routes accepted dialog completion to the provider base class.
 */
void CDialogProviderAccessor::CallOnOK() {
    CDialog::OnOK();
}

/**
 * Original helper evidence: no standalone retail function; used by the MFC
 * message-map data for WestwoodOnlineUpgradeConfigDialog.
 * Purpose: returns the provider CDialog base message map.
 */
const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeConfigDialog::GetBaseMessageMapForMfc() {
    return CDialogProviderAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const WestwoodOnlineUpgradeConfigDialog::messageEntries[] = {
    {WM_COMMAND,
        EN_SETFOCUS,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear},
    {WM_COMMAND,
        CBN_KILLFOCUS,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus},
    {WM_COMMAND,
        CBN_SELCHANGE,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange},
    {WM_COMMAND,
        CBN_EDITCHANGE,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange},
    {WM_COMMAND,
        CBN_DROPDOWN,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown},
    {WM_COMMAND,
        BN_CLICKED,
        kWestwoodOnlineUpgradeConfigRememberPasswordCheckId,
        kWestwoodOnlineUpgradeConfigRememberPasswordCheckId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        kMfcMessageMapSigVoid,
        (AFX_PMSG)&WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus},
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP WestwoodOnlineUpgradeConfigDialog::messageMap = {
    &WestwoodOnlineUpgradeConfigDialog::GetBaseMessageMapForMfc,
    &WestwoodOnlineUpgradeConfigDialog::messageEntries[0],
};

/**
 * Reimplements 0x441a10: WestwoodOnlineUpgradeConfigDialog::GetMessageMap
 * Original file: WestwoodOnlineUpgradeConfigDialog.cpp.
 * Purpose: returns the recovered MFC command notification map for this dialog.
 */
const AFX_MSGMAP * WestwoodOnlineUpgradeConfigDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeConfigDialog::messageMap;
}

/**
 * Reimplements 0x441750: WestwoodOnlineUpgradeConfigDialog::WestwoodOnlineUpgradeConfigDialog
 * Original file: D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp.
 * Purpose: constructs the MFC dialog, child controls, CString profile arrays,
 * installs the derived dialog vftable, and seeds the WOL password flag.
 */
WestwoodOnlineUpgradeConfigDialog::WestwoodOnlineUpgradeConfigDialog(
    CWnd *parentWnd
) :
    CDialog(
        kWestwoodOnlineUpgradeConfigDialogResourceId,
        parentWnd
    ),
    m_profileCombo(),
    m_connectStringEdit(),
    m_reservedString(),
    m_connectStringEditText()
{
    m_connectStringEditText = kEmptyString;
    m_wolPasswordFlag = zOpt_GetWolPasswordFlagValue();
}

/**
 * Original helper evidence: no standalone retail function; reconstructed
 * callers still use the recovered Constructor helper spelling while the owner
 * model is the real C++ constructor above.
 * Purpose: placement-construct the config dialog and return self.
 */
WestwoodOnlineUpgradeConfigDialog * WestwoodOnlineUpgradeConfigDialog::Constructor(
    CWnd *parentWnd
) {
    new (this) WestwoodOnlineUpgradeConfigDialog(parentWnd);
    return this;
}

/**
 * Reimplements 0x4418b0: WestwoodOnlineUpgradeConfigDialog::~WestwoodOnlineUpgradeConfigDialog
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: tears down the profile CString arrays and embedded MFC controls in
 * the reverse order established by the constructor.
 */
WestwoodOnlineUpgradeConfigDialog::~WestwoodOnlineUpgradeConfigDialog() {
}

/**
 * Original helper evidence: no standalone retail function; reconstructed
 * callers still use the recovered destructor helper spelling while the owner
 * model is the real C++ destructor above.
 * Purpose: invoke the real config dialog destructor.
 */
void WestwoodOnlineUpgradeConfigDialog::Destructor() {
    this->WestwoodOnlineUpgradeConfigDialog::~WestwoodOnlineUpgradeConfigDialog();
}

/**
 * Reimplements 0x4419a0: WestwoodOnlineUpgradeConfigDialog::DoDataExchange
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: binds the profile combo, connect-string edit box, edit text, and
 * remember-password flag to the dialog controls.
 */
void WestwoodOnlineUpgradeConfigDialog::DoDataExchange(
    CDataExchange *dataExchange
) {
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeConfigProfileComboId,
        *((CWnd *)&m_profileCombo)
    );
    DDX_Control(
        dataExchange,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        *((CWnd *)&m_connectStringEdit)
    );
    DDX_Text(
        dataExchange,
        kWestwoodOnlineUpgradeConfigConnectStringEditId,
        m_connectStringEditText
    );
    DDX_Check(
        dataExchange,
        kWestwoodOnlineUpgradeConfigRememberPasswordCheckId,
        m_wolPasswordFlag
    );
}

/**
 * Reimplements 0x441a20: WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: clears the selection in the connect-string edit control on focus.
 */
void WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear() {
    ::SendMessageA(
        m_connectStringEdit.m_hWnd,
        EM_SETSEL,
        0,
        0
    );
}

/**
 * Reimplements 0x442100: WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: stores edited connect-string text and marks the profile as custom
 * when it differs from the saved value.
 */
void WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus() {
    const int selectedIndex = m_selectedProfileIndex;
    ((CWnd *)&m_connectStringEdit)->GetWindowTextA(m_profileConnectStrings[selectedIndex]);

    if (strcmp(
            (const char *)m_profileConnectStrings[selectedIndex],
            (const char *)m_savedConnectStrings[selectedIndex]
        ) != 0) {
        m_profileConnectStringModes[selectedIndex] = 0;
    }
}

/**
 * Reimplements 0x441a40: WestwoodOnlineUpgradeConfigDialog::OnInitDialog
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: loads both WOL profiles, fills the combo box, seeds connect-string
 * modes, and initializes the selected profile edit state.
 */
BOOL WestwoodOnlineUpgradeConfigDialog::OnInitDialog() {
    ((CDialog *)this)->CDialog::OnInitDialog();

    IWestwoodOnlineUpgradeProviderApi *const api =
        (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;
    char *playerName = 0;
    char *connectString = 0;

    if (api->LoadConnectProfileStrings(1, &playerName, &connectString) != 0) {
        playerName = (char *)kEmptyString;
        connectString = (char *)kEmptyString;
    }

    m_savedPlayerNames[0] = playerName;
    m_savedConnectStrings[0] = connectString;
    m_profilePlayerNames[0] = playerName;
    m_profileConnectStrings[0] = connectString;

    const char *displayName = playerName;
    if (displayName[0] == '\0') {
        displayName = zLoc::GetMessageString(kWestwoodOnlineUpgradeConfigUnnamedProfileMessageId);
    }
    LRESULT itemIndex =
        ::SendMessageA(
            m_profileCombo.m_hWnd,
            CB_INSERTSTRING,
            0,
            (LPARAM)displayName
        );
    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_SETITEMDATA,
        itemIndex,
        0
    );

    if (api->LoadConnectProfileStrings(2, &playerName, &connectString) != 0) {
        playerName = (char *)kEmptyString;
        connectString = (char *)kEmptyString;
    }

    m_savedPlayerNames[1] = playerName;
    m_savedConnectStrings[1] = connectString;
    m_profilePlayerNames[1] = playerName;
    m_profileConnectStrings[1] = connectString;

    displayName = playerName;
    if (displayName[0] == '\0') {
        displayName = zLoc::GetMessageString(kWestwoodOnlineUpgradeConfigUnnamedProfileMessageId);
    }
    itemIndex = ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_INSERTSTRING,
        1,
        (LPARAM)displayName
    );
    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_SETITEMDATA,
        itemIndex,
        1
    );

    m_profileConnectStringModes[0] = ((const char *)m_savedConnectStrings[0])[0] == '\0' ? 0 : 1;
    m_profileConnectStringModes[1] = ((const char *)m_savedConnectStrings[1])[0] == '\0' ? 0 : 1;
    m_selectedProfileIndex = 0;
    m_profileComboEditDirty = 0;
    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_SETCURSEL,
        0,
        0
    );
    ((CWnd *)&m_connectStringEdit)->SetWindowTextA((const char *)m_profileConnectStrings[0]);
    return TRUE;
}

/**
 * Reimplements 0x441f40: WestwoodOnlineUpgradeConfigDialog::OnOK
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: saves the selected WOL profile strings and password flag before
 * accepting the dialog through MFC.
 */
void WestwoodOnlineUpgradeConfigDialog::OnOK() {
    IWestwoodOnlineUpgradeProviderApi *const api =
        (IWestwoodOnlineUpgradeProviderApi *)g_pWestwoodOnlineUpgradeApi;

    if (m_wolPasswordFlag == 0) {
        api->SaveConnectProfileStrings(
            1,
            (const char *)m_profilePlayerNames[0],
            kEmptyString,
            0
        );
        api->SaveConnectProfileStrings(
            2,
            (const char *)m_profilePlayerNames[1],
            kEmptyString,
            0
        );
    } else {
        api->SaveConnectProfileStrings(
            1,
            (const char *)m_profilePlayerNames[0],
            (const char *)m_profileConnectStrings[0],
            m_profileConnectStringModes[0] == 0
        );
        api->SaveConnectProfileStrings(
            2,
            (const char *)m_profilePlayerNames[1],
            (const char *)m_profileConnectStrings[1],
            m_profileConnectStringModes[1] == 0
        );
    }

    zOpt::SetWolPasswordFlag(m_wolPasswordFlag);
    ((CDialogProviderAccessor *)this)->CallOnOK();
}

/**
 * Reimplements 0x442010: WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: commits edited profile text back into the combo box item.
 */
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus() {
    if (m_profileComboEditDirty == 0) {
        return;
    }

    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_DELETESTRING,
        m_selectedProfileIndex,
        0
    );
    ((CWnd *)&m_profileCombo)->GetWindowTextA(m_profilePlayerNames[m_selectedProfileIndex]);
    ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_INSERTSTRING,
        m_selectedProfileIndex,
        (LPARAM)(const char *)m_profilePlayerNames[m_selectedProfileIndex]
    );
    m_profileComboEditDirty = 0;
}

/**
 * Reimplements 0x442080: WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: tracks the selected profile and displays its connect string.
 */
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange() {
    m_selectedProfileIndex = (int) ::SendMessageA(
        m_profileCombo.m_hWnd,
        CB_GETCURSEL,
        0,
        0
    );
    ((CWnd *)&m_connectStringEdit)
        ->SetWindowTextA((const char *)m_profileConnectStrings[m_selectedProfileIndex]);
}

/**
 * Reimplements 0x4420c0: WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: records that the editable profile combo text has changed.
 */
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange() {
    m_profileComboEditDirty = 1;
}

/**
 * Reimplements 0x4420d0: WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: commits pending profile-combo edits before showing the drop-down.
 */
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown() {
    OnProfileComboKillFocus();
}

/**
 * Reimplements 0x4420e0: WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked
 * Original file: D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp.
 * Purpose: toggles whether saved WOL passwords/connect strings are retained.
 */
void WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked() {
    m_wolPasswordFlag = m_wolPasswordFlag == 0;
}

/**
 * Reimplements 0x441c60: WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues
 * Original file: D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp.
 * Purpose: exposes the selected profile name, connect string, and mode to the
 * parent upgrade dialog after the modal config dialog succeeds.
 */
void WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues(
    char **playerNameOut,
    char **connectStringOut,
    int *connectStringModeOut
) {
    const int selectedIndex = m_selectedProfileIndex;
    *playerNameOut =
        m_profilePlayerNames[selectedIndex].GetBuffer(kSelectedProfileTextBufferLength);
    *connectStringOut =
        m_profileConnectStrings[selectedIndex].GetBuffer(kSelectedProfileTextBufferLength);
    *connectStringModeOut = m_profileConnectStringModes[selectedIndex];
}

/**
 * Reimplements 0x441cb0: WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues
 * Original file: D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp.
 * Purpose: runs the stack-based config dialog and copies selected profile
 * values into the owning Westwood online upgrade dialog on OK.
 */
int WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues() {
    unsigned int dialogStorage
        [(sizeof(WestwoodOnlineUpgradeConfigDialog) + kStackStorageUnitSize - 1) /
            kStackStorageUnitSize];
    WestwoodOnlineUpgradeConfigDialog *const dialog =
        (WestwoodOnlineUpgradeConfigDialog *)dialogStorage;

    dialog->Constructor(0);
    if (((CDialog *)dialog)->CDialog::DoModal() != kDialogOkResult) {
        DestructConfigDialog(dialog);
        return 0;
    }

    char *playerName = 0;
    char *connectString = 0;
    int connectStringMode = 0;
    dialog->GetSelectedProfileValues(
        &playerName,
        &connectString,
        &connectStringMode
    );

    CString playerNameString(playerName);
    g_pWestwoodOnlineUpgradeDialog->SetSelectedProfilePlayerName(playerNameString);

    CString connectStringString(connectString);
    g_pWestwoodOnlineUpgradeDialog->SetSelectedProfileConnectString(connectStringString);
    g_pWestwoodOnlineUpgradeDialog->m_selectedProfileConnectStringMode = connectStringMode;

    DestructConfigDialog(dialog);
    return 1;
}
