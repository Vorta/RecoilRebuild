#pragma once

#include "Battlesport/RecoilApp.h"

struct HudUiCreditsPanel;

struct RecoilStateCredits : RecoilApp_IState {
    HudUiCreditsPanel *m_dialog;

    RecoilStateCredits();
    static void StaticInitAndRegisterAtExit();
    static void StaticInit();
    static void RegisterAtExit();
    void OnWndActivate(int activateCode);
    int OnTryBecomeCurrent();
    void OnDeactivate();
    ~RecoilStateCredits();
    static void QueuePush();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCredits) == 0x08);
RECOIL_STATIC_ASSERT(
    offsetof(
        RecoilStateCredits,
        m_dialog
    ) == 0x04
);

extern RecoilStateCredits g_RecoilStateCredits;
