#include "Battlesport/WestwoodOnlineUpgradeConfigDialog.h"

#include "Battlesport/WestwoodOnlineUpgradeApi.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <string.h>

// Provider-boundary accessor for imported MFC42 CDialog members; this does not reimplement
// provider behavior.
class CDialogProviderAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
    void CallOnOK();
};

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CEdit) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CComboBox) == 0x40);

struct IWestwoodOnlineUpgradeConfigProfileApi : IUnknown {
    virtual void STDMETHODCALLTYPE Reserved0c() = 0;
    virtual void STDMETHODCALLTYPE Reserved10() = 0;
    virtual void STDMETHODCALLTYPE Reserved14() = 0;
    virtual void STDMETHODCALLTYPE Reserved18() = 0;
    virtual void STDMETHODCALLTYPE Reserved1c() = 0;
    virtual void STDMETHODCALLTYPE Reserved20() = 0;
    virtual void STDMETHODCALLTYPE Reserved24() = 0;
    virtual void STDMETHODCALLTYPE Reserved28() = 0;
    virtual void STDMETHODCALLTYPE Reserved2c() = 0;
    virtual void STDMETHODCALLTYPE Reserved30() = 0;
    virtual void STDMETHODCALLTYPE Reserved34() = 0;
    virtual void STDMETHODCALLTYPE Reserved38() = 0;
    virtual void STDMETHODCALLTYPE Reserved3c() = 0;
    virtual void STDMETHODCALLTYPE Reserved40() = 0;
    virtual void STDMETHODCALLTYPE Reserved44() = 0;
    virtual void STDMETHODCALLTYPE Reserved48() = 0;
    virtual void STDMETHODCALLTYPE Reserved4c() = 0;
    virtual void STDMETHODCALLTYPE Reserved50() = 0;
    virtual void STDMETHODCALLTYPE Reserved54() = 0;
    virtual void STDMETHODCALLTYPE Reserved58() = 0;
    virtual void STDMETHODCALLTYPE Reserved5c() = 0;
    virtual void STDMETHODCALLTYPE Reserved60() = 0;
    virtual void STDMETHODCALLTYPE Reserved64() = 0;
    virtual void STDMETHODCALLTYPE Reserved68() = 0;
    virtual void STDMETHODCALLTYPE Reserved6c() = 0;
    virtual void STDMETHODCALLTYPE Reserved70() = 0;
    virtual void STDMETHODCALLTYPE Reserved74() = 0;
    virtual void STDMETHODCALLTYPE Reserved78() = 0;
    virtual void STDMETHODCALLTYPE Reserved7c() = 0;
    virtual void STDMETHODCALLTYPE Reserved80() = 0;
    virtual void STDMETHODCALLTYPE Reserved84() = 0;
    virtual void STDMETHODCALLTYPE Reserved88() = 0;
    virtual void STDMETHODCALLTYPE Reserved8c() = 0;
    virtual int STDMETHODCALLTYPE LoadConnectProfileStrings(
        int profileId,
        char **playerNameOut,
        char **connectStringOut
    ) = 0;
    virtual int STDMETHODCALLTYPE SaveConnectProfileStrings(
        int profileId,
        const char *playerName,
        const char *connectString,
        int connectStringMode
    ) = 0;
};

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
const unsigned int kWestwoodOnlineUpgradeConfigUnnamedProfileMessageId = 0x3044;
const int kSelectedProfileTextBufferLength = 32;
const int kDialogOkResult = 1;
const unsigned int kStackStorageUnitSize = sizeof(unsigned int);

void DestructConfigDialog(
    WestwoodOnlineUpgradeConfigDialog *dialog
) {
    dialog->Destructor();
}
} // namespace

const AFX_MSGMAP *__stdcall CDialogProviderAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

void CDialogProviderAccessor::CallOnOK() {
    CDialog::OnOK();
}

const AFX_MSGMAP *__stdcall WestwoodOnlineUpgradeConfigDialog::GetBaseMessageMapForMfc() {
    return CDialogProviderAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const WestwoodOnlineUpgradeConfigDialog::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP WestwoodOnlineUpgradeConfigDialog::messageMap = {
    &WestwoodOnlineUpgradeConfigDialog::GetBaseMessageMapForMfc,
    &WestwoodOnlineUpgradeConfigDialog::messageEntries[0],
};

// Reimplements 0x441a10: WestwoodOnlineUpgradeConfigDialog::GetMessageMap
// (WestwoodOnlineUpgradeConfigDialog.cpp)
const AFX_MSGMAP * WestwoodOnlineUpgradeConfigDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeConfigDialog::messageMap;
}

// Reimplements 0x441750: WestwoodOnlineUpgradeConfigDialog::Constructor
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp)
WestwoodOnlineUpgradeConfigDialog * WestwoodOnlineUpgradeConfigDialog::Constructor(
    CWnd *parentWnd
) {
    new ((CDialog *)this) CDialog(
        kWestwoodOnlineUpgradeConfigDialogResourceId,
        parentWnd
    );

    new (&m_profileCombo) CComboBox();
    new (&m_connectStringEdit) CEdit();
    new (&m_reservedString) CString();
    new (&m_connectStringEditText) CString();

    int index;
    for (index = 0; index < 2; ++index) {
        new (&m_savedPlayerNames[index]) CString();
    }
    for (index = 0; index < 2; ++index) {
        new (&m_savedConnectStrings[index]) CString();
    }
    for (index = 0; index < 2; ++index) {
        new (&m_profilePlayerNames[index]) CString();
    }
    for (index = 0; index < 2; ++index) {
        new (&m_profileConnectStrings[index]) CString();
    }

    m_connectStringEditText = kEmptyString;
    m_wolPasswordFlag = zOpt_GetWolPasswordFlagValue();
    return this;
}

// Reimplements 0x4418b0: WestwoodOnlineUpgradeConfigDialog::Destructor
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
void WestwoodOnlineUpgradeConfigDialog::Destructor() {
    int index;
    for (index = 1; index >= 0; --index) {
        m_profileConnectStrings[index].CString::~CString();
    }
    for (index = 1; index >= 0; --index) {
        m_profilePlayerNames[index].CString::~CString();
    }
    for (index = 1; index >= 0; --index) {
        m_savedConnectStrings[index].CString::~CString();
    }
    for (index = 1; index >= 0; --index) {
        m_savedPlayerNames[index].CString::~CString();
    }
    m_connectStringEditText.CString::~CString();
    m_reservedString.CString::~CString();
    ((CEdit *)&m_connectStringEdit)->CEdit::~CEdit();
    ((CComboBox *)&m_profileCombo)->CComboBox::~CComboBox();
    ((CDialog *)this)->CDialog::~CDialog();
}

// Reimplements 0x4419a0: WestwoodOnlineUpgradeConfigDialog::DoDataExchange
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
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

// Reimplements 0x441a20: WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
void WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditSetFocusClear() {
    ::SendMessageA(
        m_connectStringEdit.m_hWnd,
        EM_SETSEL,
        0,
        0
    );
}

// Reimplements 0x442100: WestwoodOnlineUpgradeConfigDialog::OnConnectStringEditKillFocus
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
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

// Reimplements 0x441a40: WestwoodOnlineUpgradeConfigDialog::OnInitDialog
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
BOOL WestwoodOnlineUpgradeConfigDialog::OnInitDialog() {
    ((CDialog *)this)->CDialog::OnInitDialog();

    IWestwoodOnlineUpgradeConfigProfileApi *const api =
        (IWestwoodOnlineUpgradeConfigProfileApi *)g_pWestwoodOnlineUpgradeApi;
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

// Reimplements 0x441f40: WestwoodOnlineUpgradeConfigDialog::OnOK
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
void WestwoodOnlineUpgradeConfigDialog::OnOK() {
    IWestwoodOnlineUpgradeConfigProfileApi *const api =
        (IWestwoodOnlineUpgradeConfigProfileApi *)g_pWestwoodOnlineUpgradeApi;

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

// Reimplements 0x442010: WestwoodOnlineUpgradeConfigDialog::OnProfileComboKillFocus
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
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

// Reimplements 0x442080: WestwoodOnlineUpgradeConfigDialog::OnProfileComboSelChange
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
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

// Reimplements 0x4420c0: WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboEditChange() {
    m_profileComboEditDirty = 1;
}

// Reimplements 0x4420d0: WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
void WestwoodOnlineUpgradeConfigDialog::OnProfileComboDropdown() {
    OnProfileComboKillFocus();
}

// Reimplements 0x4420e0: WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked
// (D:\Proj\GameZRecoil\westwoodonline\WolapiConfigDialog.cpp)
void WestwoodOnlineUpgradeConfigDialog::OnConnectStringModeClicked() {
    m_wolPasswordFlag = m_wolPasswordFlag == 0;
}

// Reimplements 0x441c60: WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp)
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

// Reimplements 0x441cb0: WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp)
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
