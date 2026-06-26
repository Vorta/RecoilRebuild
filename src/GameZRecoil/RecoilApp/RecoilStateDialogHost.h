#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/RecoilApp.h"

struct HudUiContainer;

struct RecoilStateDialogHost : RecoilApp_IState {
    HudUiContainer *m_dialog;

    void OnWndActivate(int activateCode);
    int OnUpdateShouldQuit();
    void OnDeactivate();
    void OnSuspend(int suspendParam);
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateDialogHost) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateDialogHost,
        m_dialog
    ) == 0x04
);
