#include "Battlesport/WestwoodOnlineUpgradeConfigDialog.h"

class CDialog {
  protected:
    __declspec(dllimport) static const AFX_MSGMAP messageMap;
};

class CDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *RECOIL_STDCALL GetMessageMap();
};

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
