#include "Battlesport/WestwoodOnlineUpgradeConfigDialog.h"

extern "C" int westwood_online_upgrade_config_get_message_map_smoke(void)
{
    WestwoodOnlineUpgradeConfigDialog dialog;
    const AFX_MSGMAP *const messageMap = dialog.GetMessageMap();
    if (messageMap != &WestwoodOnlineUpgradeConfigDialog::messageMap) {
        return 1;
    }

    if (messageMap->pfnGetBaseMap == 0 ||
        messageMap->lpEntries != &WestwoodOnlineUpgradeConfigDialog::messageEntries[0]) {
        return 2;
    }

    const AFX_MSGMAP_ENTRY &sentinel =
        WestwoodOnlineUpgradeConfigDialog::messageEntries[0];
    return sentinel.nMessage == 0 && sentinel.nCode == 0 && sentinel.nID == 0 &&
                   sentinel.nLastID == 0 && sentinel.nSig == 0 && sentinel.pfn == 0
               ? 0
               : 3;
}
