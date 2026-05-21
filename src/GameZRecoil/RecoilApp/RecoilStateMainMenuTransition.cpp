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

// Reimplements 0x4151b0: RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition
RECOIL_NOINLINE RECOIL_NO_GS RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition() {
    vftable = kRecoilStateMainMenuTransition_VtblAddress;
    RecoilStateMainMenuTransitionBaseVtableGuard baseVtableOnExit = {this};

    HudUiMainMenuDialogVirtual *dialog = (HudUiMainMenuDialogVirtual *)m_mainMenuDialog;
    if (dialog != 0) {
        dialog->SetEnabled(0);

        dialog = (HudUiMainMenuDialogVirtual *)m_mainMenuDialog;
        if (dialog != 0) {
            dialog->ScalarDeletingDestructor(1);
        }

        m_mainMenuDialog = 0;
    }
}

// Reimplements 0x415190: RecoilStateMainMenuTransition::ScalarDeletingDestructor
// (D:\Proj\GameZRecoil\RecoilApp\RecoilStateMainMenuTransition.cpp)
RECOIL_NOINLINE RecoilStateMainMenuTransition *RECOIL_THISCALL
RecoilStateMainMenuTransition::ScalarDeletingDestructor(unsigned int flags) {
    this->~RecoilStateMainMenuTransition();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}
