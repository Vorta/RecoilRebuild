#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

RecoilStateMainMenuTransition g_RecoilState_MainMenuTransition;

// Reimplements 0x415170: RecoilStateMainMenuTransition::Constructor
RecoilStateMainMenuTransition *RECOIL_THISCALL RecoilStateMainMenuTransition::Constructor() {
    vftable = kRecoilStateMainMenuTransition_VtblAddress;
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
        self->vftable = kRecoilStateBase_VtblAddress;
    }
};
} // namespace

// Reimplements 0x4151b0: RecoilStateMainMenuTransition::Destructor
RECOIL_NOINLINE RECOIL_NO_GS void RECOIL_THISCALL RecoilStateMainMenuTransition::Destructor() {
    vftable = kRecoilStateMainMenuTransition_VtblAddress;
    RecoilStateMainMenuTransitionBaseVtableGuard baseVtableOnExit = {this};

    HudUiMainMenuDialog *dialog = (HudUiMainMenuDialog *)m_mainMenuDialog;
    if (dialog != 0) {
        HudUiMainMenuDialog_Vtbl *dialogVtbl = (HudUiMainMenuDialog_Vtbl *)dialog->vftable;
        dialogVtbl->SetEnabled(dialog, 0);

        dialog = (HudUiMainMenuDialog *)m_mainMenuDialog;
        if (dialog != 0) {
            dialogVtbl = (HudUiMainMenuDialog_Vtbl *)dialog->vftable;
            dialogVtbl->ScalarDeletingDtor(dialog, 1);
        }

        m_mainMenuDialog = 0;
    }
}

// Reimplements 0x415190: RecoilStateMainMenuTransition::ScalarDeletingDestructor
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateMainMenuTransition.cpp)
RECOIL_NOINLINE RecoilStateMainMenuTransition *RECOIL_THISCALL
RecoilStateMainMenuTransition::ScalarDeletingDestructor(unsigned int flags) {
    Destructor();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}
