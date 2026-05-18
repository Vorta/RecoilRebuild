#include "Battlesport/RecoilApp.h"

struct RecoilStateCredits {
    RecoilPtr32 vftable;
    RecoilPtr32 dialog;

    RecoilStateCredits *RECOIL_THISCALL Constructor();
    static void RECOIL_CDECL QueuePush();
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCredits) == 0x08);

namespace {
struct RecoilStateCredits_Vtbl {
    RecoilFn32 slots[10];
};
RECOIL_STATIC_ASSERT(sizeof(RecoilStateCredits_Vtbl) == sizeof(RecoilApp_IState_Vtbl));

RecoilStateCredits_Vtbl g_RecoilStateCredits_Vtbl = {0};
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

// Reimplements 0x409b00: RecoilStateCredits::QueuePush
// (D:\Proj\Battlesport\RecoilStateCredits.cpp)
void RECOIL_CDECL RecoilStateCredits::QueuePush()
{
    g_RecoilApp.QueuePushState((RecoilApp_IState *)&g_RecoilStateCredits, 0);
}
