#include "Battlesport/WestwoodOnlineUpgradeConfigDialog.h"

#include "GameZRecoil/zGame/zGame.h"

#include <new>

// Access shim for imported MFC42 CDialog metadata; this does not reimplement
// CDialog behavior.
class CDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *RECOIL_STDCALL GetMessageMap();
};

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CEdit) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CComboBox) == 0x40);

const RecoilNamedVtable kWestwoodOnlineUpgradeConfigDialog_Vtable = {
    "WestwoodOnlineUpgradeConfigDialog vtable"};

namespace
{
const UINT kWestwoodOnlineUpgradeConfigDialogResourceId = 156;
const char kNoPasswordText[] = "No Password";
const int kSelectedProfileTextBufferLength = 32;
const int kDialogOkResult = 1;
const unsigned int kStackStorageUnitSize = sizeof(unsigned int);

void RECOIL_CDECL DestructConfigDialog(WestwoodOnlineUpgradeConfigDialog *dialog)
{
    for (int index = 1; index >= 0; --index)
    {
        dialog->m_profileConnectStrings[index].CString::~CString();
    }
    for (int index = 1; index >= 0; --index)
    {
        dialog->m_profilePlayerNames[index].CString::~CString();
    }
    for (int index = 1; index >= 0; --index)
    {
        dialog->m_profileDisplayNames[index].CString::~CString();
    }
    for (int index = 1; index >= 0; --index)
    {
        dialog->m_profileNames[index].CString::~CString();
    }
    dialog->m_noPasswordText.CString::~CString();
    dialog->m_passwordText.CString::~CString();
    ((CComboBox *)&dialog->m_profileCombo)->CComboBox::~CComboBox();
    ((CEdit *)&dialog->m_serverNameEdit)->CEdit::~CEdit();
    ((CDialog *)dialog)->CDialog::~CDialog();
}
} // namespace

const AFX_MSGMAP *RECOIL_STDCALL CDialogMessageMapAccessor::GetMessageMap()
{
    return &CDialog::messageMap;
}

const AFX_MSGMAP *RECOIL_STDCALL
WestwoodOnlineUpgradeConfigDialog::GetBaseMessageMapForMfc()
{
    return CDialogMessageMapAccessor::GetMessageMap();
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
RECOIL_WOL_CONFIG_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL
WestwoodOnlineUpgradeConfigDialog::GetMessageMap() const
{
    return &WestwoodOnlineUpgradeConfigDialog::messageMap;
}

// Reimplements 0x441750: WestwoodOnlineUpgradeConfigDialog::Constructor
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp)
RECOIL_WOL_CONFIG_NOINLINE WestwoodOnlineUpgradeConfigDialog *RECOIL_THISCALL
WestwoodOnlineUpgradeConfigDialog::Constructor(CWnd *parentWnd)
{
    new ((CDialog *)this)
        CDialog(kWestwoodOnlineUpgradeConfigDialogResourceId, parentWnd);

    new (&m_serverNameEdit) CEdit();
    new (&m_profileCombo) CComboBox();
    new (&m_passwordText) CString();
    new (&m_noPasswordText) CString();

    for (int index = 0; index < 2; ++index)
    {
        new (&m_profileNames[index]) CString();
    }
    for (int index = 0; index < 2; ++index)
    {
        new (&m_profileDisplayNames[index]) CString();
    }
    for (int index = 0; index < 2; ++index)
    {
        new (&m_profilePlayerNames[index]) CString();
    }
    for (int index = 0; index < 2; ++index)
    {
        new (&m_profileConnectStrings[index]) CString();
    }

    m_noPasswordText = kNoPasswordText;
    m_wolPasswordFlag = zOpt_GetWolPasswordFlagValue();
    return this;
}

// Reimplements 0x441c60: WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp)
RECOIL_WOL_CONFIG_NOINLINE void RECOIL_THISCALL
WestwoodOnlineUpgradeConfigDialog::GetSelectedProfileValues(
    char **playerNameOut,
    char **connectStringOut,
    int *connectStringModeOut)
{
    const int selectedIndex = m_selectedProfileIndex;
    *playerNameOut =
        m_profilePlayerNames[selectedIndex].GetBuffer(kSelectedProfileTextBufferLength);
    *connectStringOut =
        m_profileConnectStrings[selectedIndex].GetBuffer(kSelectedProfileTextBufferLength);
    *connectStringModeOut = m_profileConnectStringModes[selectedIndex];
}

// Reimplements 0x441cb0: WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeConfigDialog.cpp)
RECOIL_WOL_CONFIG_NOINLINE int RECOIL_CDECL
WestwoodOnlineUpgradeConfigDialog::ShowModalAndApplySelectedProfileValues()
{
    unsigned int dialogStorage[(sizeof(WestwoodOnlineUpgradeConfigDialog) +
                                kStackStorageUnitSize - 1) /
                               kStackStorageUnitSize];
    WestwoodOnlineUpgradeConfigDialog *const dialog =
        (WestwoodOnlineUpgradeConfigDialog *)dialogStorage;

    dialog->Constructor(0);
    if (((CDialog *)dialog)->CDialog::DoModal() != kDialogOkResult)
    {
        DestructConfigDialog(dialog);
        return 0;
    }

    char *playerName = 0;
    char *connectString = 0;
    int connectStringMode = 0;
    dialog->GetSelectedProfileValues(&playerName, &connectString, &connectStringMode);

    CString playerNameString(playerName);
    g_pWestwoodOnlineUpgradeDialog->SetSelectedProfilePlayerName(playerNameString);

    CString connectStringString(connectString);
    g_pWestwoodOnlineUpgradeDialog->SetSelectedProfileConnectString(connectStringString);
    g_pWestwoodOnlineUpgradeDialog->m_selectedProfileConnectStringMode = connectStringMode;

    DestructConfigDialog(dialog);
    return 1;
}
