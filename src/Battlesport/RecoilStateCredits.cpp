#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zHud/zhud_ui.h"

struct RecoilStateCredits {
    RecoilPtr32 vftable;
    RecoilPtr32 dialog;

    RecoilStateCredits * Constructor();
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

namespace {
struct HudUiCreditsPanelVirtual {
    virtual void Update(float deltaSeconds) = 0;
    virtual void SetEnabled(int enabled) = 0;
    virtual HudUiCreditsPanelVirtual * ScalarDeletingDestructor(
        unsigned int flags
    ) = 0;
};

struct RecoilStateCredits_Vtbl {
    RecoilFn32 slots[10];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCredits_Vtbl) == sizeof(RecoilApp_IState_Vtbl));

RecoilStateCredits_Vtbl g_RecoilStateCredits_Vtbl = {0};

struct RecoilStateCreditsBaseVtableGuard {
    RecoilStateCredits *self;

    ~RecoilStateCreditsBaseVtableGuard() {
        self->vftable = RecoilSymbolPtr32(&g_RecoilStateBase_Vtbl);
    }
};
} // namespace

RecoilStateCredits g_RecoilStateCredits = {
    (RecoilPtr32)(unsigned int)&g_RecoilStateCredits_Vtbl,
    0,
};

// Reimplements 0x409950: RecoilStateCredits::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
void RecoilStateCredits::StaticInitAndRegisterAtExit() {
    g_RecoilStateCredits.Constructor();
    StaticInit();
}

// Reimplements 0x409970: RecoilStateCredits::StaticInit
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
void RecoilStateCredits::StaticInit() {
    atexit(RegisterAtExit);
}

// Reimplements 0x409980: RecoilStateCredits::RegisterAtExit
// Retail name is the registered at-exit callback; it destroys the global credits state.
void RecoilStateCredits::RegisterAtExit() {
    g_RecoilStateCredits.~RecoilStateCredits();
}

// Reimplements 0x409990: RecoilStateCredits::Constructor
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
RecoilStateCredits * RecoilStateCredits::Constructor() {
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateCredits_Vtbl;
    dialog = 0;
    return this;
}

// Reimplements 0x4099a0: RecoilStateCredits::OnWndActivate
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
void RecoilStateCredits::OnWndActivate(
    int activateCode
) {
    if (activateCode == 0) {
        return;
    }

    RecoilStateCredits *const state = this;
    HudUiCreditsPanel *const creditsPanel = (HudUiCreditsPanel *)(unsigned int)state->dialog;
    if (creditsPanel != 0) {
        ((HudUiDialogController *)creditsPanel)->BlitOwnedSurfaceToPrimary();
        ((HudUiContainer *)(unsigned int)state->dialog)->InvalidateChildren();
    }
}

// Reimplements 0x409a60: RecoilStateCredits::OnTryBecomeCurrent
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
int RecoilStateCredits::OnTryBecomeCurrent() {
    HudUiCreditsPanel *creditsPanel =
        (HudUiCreditsPanel *) ::operator new(sizeof(HudUiCreditsPanel));
    if (creditsPanel != 0) {
        creditsPanel = creditsPanel->Constructor();
    }
    dialog = (RecoilPtr32)(unsigned int)creditsPanel;

    ((HudUiCreditsPanelVirtual *)creditsPanel)->SetEnabled(1);
    return 1;
}

// Reimplements 0x409ad0: RecoilStateCredits::OnDeactivate
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
void RecoilStateCredits::OnDeactivate() {
    HudUiCreditsPanelVirtual *dialogView = (HudUiCreditsPanelVirtual *)dialog;
    if (dialogView == 0) {
        return;
    }

    dialogView->SetEnabled(0);
    ((HudUiDialogController *)(unsigned int)dialog)->BlitOwnedSurfaceToPrimary();

    dialogView = (HudUiCreditsPanelVirtual *)dialog;
    if (dialogView != 0) {
        dialogView->ScalarDeletingDestructor(1);
    }

    dialog = 0;
}

// Reimplements 0x4099f0: RecoilStateCredits::Destructor
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
RecoilStateCredits::~RecoilStateCredits() {
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateCredits_Vtbl;
    RecoilStateCreditsBaseVtableGuard baseVtableOnExit = {this};

    HudUiCreditsPanelVirtual *dialogView = (HudUiCreditsPanelVirtual *)dialog;
    if (dialogView != 0) {
        dialogView->SetEnabled(0);

        dialogView = (HudUiCreditsPanelVirtual *)dialog;
        if (dialogView != 0) {
            dialogView->ScalarDeletingDestructor(1);
        }

        dialog = 0;
    }
}

// Reimplements 0x409b00: RecoilStateCredits::QueuePush
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
void RecoilStateCredits::QueuePush() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilStateCredits,
        0
    );
}
