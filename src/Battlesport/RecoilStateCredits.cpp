#include "Battlesport/RecoilApp.h"

struct RecoilStateCredits {
    RecoilPtr32 vftable;
    RecoilPtr32 dialog;

    RecoilStateCredits *RECOIL_THISCALL Constructor();
    RECOIL_NOINLINE ~RecoilStateCredits();
    static void RECOIL_CDECL QueuePush();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCredits) == 0x08);

namespace {
struct HudUiCreditsPanelVirtual {
    virtual void RECOIL_THISCALL Update(float deltaSeconds) = 0;
    virtual void RECOIL_THISCALL SetEnabled(int enabled) = 0;
    virtual HudUiCreditsPanelVirtual *RECOIL_THISCALL
    ScalarDeletingDestructor(unsigned int flags) = 0;
};

struct RecoilStateCredits_Vtbl {
    RecoilFn32 slots[10];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCredits_Vtbl) == sizeof(RecoilApp_IState_Vtbl));

RecoilStateCredits_Vtbl g_RecoilStateCredits_Vtbl = {0};

struct RecoilStateCreditsBaseVtableGuard {
    RecoilStateCredits *self;

    ~RecoilStateCreditsBaseVtableGuard()
    {
        self->vftable = kRecoilStateBase_VtblAddress;
    }
};
} // namespace

RecoilStateCredits g_RecoilStateCredits = {
    (RecoilPtr32)(unsigned int)&g_RecoilStateCredits_Vtbl,
    0,
};

// Reimplements 0x409990: RecoilStateCredits::Constructor
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
RecoilStateCredits *RECOIL_THISCALL RecoilStateCredits::Constructor()
{
    vftable = (RecoilPtr32)(unsigned int)&g_RecoilStateCredits_Vtbl;
    dialog = 0;
    return this;
}

// Reimplements 0x4099f0: RecoilStateCredits::Destructor
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
RECOIL_NOINLINE RecoilStateCredits::~RecoilStateCredits()
{
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
void RECOIL_CDECL RecoilStateCredits::QueuePush()
{
    g_RecoilApp.QueuePushState((RecoilApp_IState *)&g_RecoilStateCredits, 0);
}
