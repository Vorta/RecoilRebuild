#pragma once

#include "Battlesport/recoil_state_dialog_host.h"

struct HudUiCreditsPanel;

/**
 * @recoil-anchor recoil:anchor:battlesport.recoil-state-credits.type
 * @recoil-artifact emits .text recoil:function:0x4099d0: VC5 compiler-generated deleting-destructor contribution anchored to this complete type definition; not an authored body.
 * Purpose: Defines the credits state whose ordinary virtual lifetime causes
 * VC5 to emit the deleting-destructor contribution.
 */
struct RecoilStateCredits : RecoilStateDialogHost {
    RecoilStateCredits();
    static void __cdecl StaticInitAndRegisterAtExit();
    static RecoilStateCredits *StaticInit();
    static void RegisterAtExit();
    static void __cdecl AtExitDestructor();
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
