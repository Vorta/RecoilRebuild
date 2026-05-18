#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "GameZRecoil/zGame/zGame.h"

// Reimplements 0x415370: RecoilStateMainMenuTransition::OnResume
void RECOIL_THISCALL RecoilStateMainMenuTransition::OnResume(int param) {
    if (m_mainMenuDialog == 0 || param != 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    HudUiMainMenuDialog *dialog = (HudUiMainMenuDialog *)m_mainMenuDialog;
    HudUiMainMenuDialog_Vtbl *dialogVtbl = (HudUiMainMenuDialog_Vtbl *)dialog->vftable;
    dialogVtbl->SetEnabled(dialog, 1);

    ((HudUiContainer *)m_mainMenuDialog)->InvalidateChildren();

    dialog = (HudUiMainMenuDialog *)m_mainMenuDialog;
    dialogVtbl = (HudUiMainMenuDialog_Vtbl *)dialog->vftable;
    dialogVtbl->UpdateAll(dialog, 0);

    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled((zVidRect32 *)zOpt::GetWindowSection(),
                                    (zVidRect32 *)zOpt::GetWindowSection(), 1, 1);
}
