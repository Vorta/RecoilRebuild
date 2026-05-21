#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "GameZRecoil/zGame/zGame.h"

// Reimplements 0x415370: RecoilStateMainMenuTransition::OnResume
void RECOIL_THISCALL RecoilStateMainMenuTransition::OnResume(int param) {
    if (m_mainMenuDialog == 0 || param != 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    HudUiMainMenuDialogVirtual *dialog =
        (HudUiMainMenuDialogVirtual *)m_mainMenuDialog;
    dialog->SetEnabled(1);

    ((HudUiContainer *)m_mainMenuDialog)->InvalidateChildren();

    dialog = (HudUiMainMenuDialogVirtual *)m_mainMenuDialog;
    dialog->Update(0.0f);

    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled((zVidRect32 *)zOpt::GetWindowSection(),
                                    (zVidRect32 *)zOpt::GetWindowSection(), 1, 1);
}
