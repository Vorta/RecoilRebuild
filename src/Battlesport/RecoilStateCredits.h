#pragma once

#include "GameZRecoil/RecoilApp/RecoilStateDialogHost.h"

struct HudUiCreditsPanel;

struct RecoilStateCredits : RecoilStateDialogHost {
    RecoilStateCredits();
    static void StaticInitAndRegisterAtExit();
    static void StaticInit();
    static void RegisterAtExit();
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

union RecoilStateCreditsStorage {
    unsigned long align;
    unsigned char bytes[sizeof(RecoilStateCredits)];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCreditsStorage) == 0x08);

extern RecoilStateCreditsStorage g_RecoilStateCredits;
#define g_RecoilStateCredits \
    (*(RecoilStateCredits *)&g_RecoilStateCredits)
