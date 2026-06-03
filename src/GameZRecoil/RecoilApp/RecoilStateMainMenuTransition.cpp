#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include <stdlib.h>
#include <string.h>

namespace {
template <typename Method>
unsigned int MainMenuTransitionMethodAddress(
    Method method
) {
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(
        &address,
        &method,
        sizeof(method)
    );
    return address;
}

RecoilApp_IState_Vtbl MakeMainMenuTransitionVtable() {
    RecoilApp_IState_Vtbl table = {0};
    table.ScalarDeletingDtor =
        MainMenuTransitionMethodAddress(&RecoilStateMainMenuTransition::ScalarDeletingDestructor);
    table.OnCanBecomeCurrent =
        MainMenuTransitionMethodAddress(&RecoilStateMainMenuTransition::OnTryBecomeCurrent);
    table.OnDeactivate =
        MainMenuTransitionMethodAddress(&RecoilStateMainMenuTransition::OnDeactivate);
    table.OnResume = MainMenuTransitionMethodAddress(&RecoilStateMainMenuTransition::OnResume);
    return table;
}
} // namespace

RecoilStateMainMenuTransition g_RecoilState_MainMenuTransition;
RecoilApp_IState_Vtbl g_RecoilStateMainMenuTransition_Vtbl = MakeMainMenuTransitionVtable();

// Reimplements 0x415100: RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit
void RECOIL_CDECL RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x415110: RecoilStateMainMenuTransition::StaticInit
RecoilStateMainMenuTransition *RECOIL_CDECL RecoilStateMainMenuTransition::StaticInit() {
    return g_RecoilState_MainMenuTransition.Constructor();
}

// Reimplements 0x415120: RecoilStateMainMenuTransition::RegisterAtExit
void RECOIL_CDECL RecoilStateMainMenuTransition::RegisterAtExit() {
    atexit(RecoilStateMainMenuTransition::AtExitDestructor);
}

// Reimplements 0x415130: RecoilStateMainMenuTransition::AtExitDestructor
void RECOIL_CDECL RecoilStateMainMenuTransition::AtExitDestructor() {
    g_RecoilState_MainMenuTransition.~RecoilStateMainMenuTransition();
}

// Reimplements 0x415170: RecoilStateMainMenuTransition::Constructor
RecoilStateMainMenuTransition *RECOIL_THISCALL RecoilStateMainMenuTransition::Constructor() {
    vftable = RecoilSymbolPtr32(&g_RecoilStateMainMenuTransition_Vtbl);
    m_mainMenuDialog = 0;
    m_entryRoute = RECOIL_MAINMENU_ROUTE_FRONTEND;
    m_deferredVideoModeIndex = ZVID_MODE_INVALID_COMPLEMENT;
    m_pausedAudioSnapshot = 0;
    return this;
}

namespace {
struct RecoilStateMainMenuTransitionBaseVtableGuard {
    RecoilStateMainMenuTransition *self;

    ~RecoilStateMainMenuTransitionBaseVtableGuard() {
        self->vftable = RecoilSymbolPtr32(&g_RecoilStateBase_Vtbl);
    }
};
} // namespace

// Reimplements 0x4151b0: RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition
RECOIL_NOINLINE RECOIL_NO_GS RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition() {
    vftable = RecoilSymbolPtr32(&g_RecoilStateMainMenuTransition_Vtbl);
    RecoilStateMainMenuTransitionBaseVtableGuard baseVtableOnExit = {this};

    HudUiMainMenuDialog *dialog = (HudUiMainMenuDialog *)m_mainMenuDialog;
    if (dialog != 0) {
        dialog->SetEnabled(0);

        dialog = (HudUiMainMenuDialog *)m_mainMenuDialog;
        if (dialog != 0) {
            delete dialog;
        }

        m_mainMenuDialog = 0;
    }
}

// Reimplements 0x415190: RecoilStateMainMenuTransition::ScalarDeletingDestructor
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateMainMenuTransition.cpp)
RECOIL_NOINLINE RecoilStateMainMenuTransition *RECOIL_THISCALL
RecoilStateMainMenuTransition::ScalarDeletingDestructor(
    unsigned int flags
) {
    this->~RecoilStateMainMenuTransition();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}
