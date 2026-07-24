#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zRender/zrndr.h"

extern "C" int zhud_cmd_dialog_state_lifecycle_smoke(void) {
    HudCmdDialogState state;
    return state.m_dialog == 0 ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_state_on_try_become_current_smoke(void) {
    const unsigned char oldKeyboardSuspend = g_zInput_KeyboardSuspendFlags;
    g_zInput_KeyboardSuspendFlags = (unsigned char)(oldKeyboardSuspend & ~2u);

    bool becameCurrent = false;
    {
        HudCmdDialogState state;
        const int became = state.OnTryBecomeCurrent();
        HudCmdDialog *const dialog = (HudCmdDialog *)state.m_dialog;
        becameCurrent =
            became == 1 &&
            dialog != 0 &&
            dialog->enabled == 1 &&
            (g_zInput_KeyboardSuspendFlags & 2u) != 0;
    }

    g_zInput_KeyboardSuspendFlags = oldKeyboardSuspend;
    return becameCurrent ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_state_on_deactivate_smoke(void) {
    const unsigned char oldKeyboardSuspend = g_zInput_KeyboardSuspendFlags;
    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    g_zInput_KeyboardSuspendFlags = 2;
    g_zInput_BindMap_Current = 0;

    HudCmdDialogState nullState;
    nullState.OnDeactivate();
    const bool nullPath =
        nullState.m_dialog == 0 && (g_zInput_KeyboardSuspendFlags & 2u) == 0;

    g_zInput_KeyboardSuspendFlags = oldKeyboardSuspend;
    g_zInput_BindMap_Current = oldCurrent;
    return nullPath ? 0 : 1;
}
