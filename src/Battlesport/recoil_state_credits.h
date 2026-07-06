#pragma once

#include "GameZRecoil/RecoilApp/recoil_state_dialog_host.h"

struct HudUiCreditsPanel;

struct RecoilStateCredits : RecoilStateDialogHost {
    RecoilStateCredits();
    int OnTryBecomeCurrent();
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
