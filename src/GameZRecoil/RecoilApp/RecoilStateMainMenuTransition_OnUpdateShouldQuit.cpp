#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zVideo/zVideo.h"

/**
 * Reimplements 0x435e80: RecoilStateSaveLoadTransition::OnUpdateShouldQuit
 * (BN canonical folded body).
 *
 * Source owner: app_shell.folded_dialog_update_should_quit. BN shows the
 * retail body shared by DialogHost, MainMenuTransition, SaveLoadTransition,
 * and other dialog-hosted state vtable slots; this definition preserves the
 * MainMenuTransition typed participant.
 *
 * Original-source function evidence: folded retail body 0x435e80.
 * Purpose: update and present the active main-menu dialog each frame while the
 * transition state is current.
 */
int RecoilStateMainMenuTransition::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);

    if (m_mainMenuDialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        ((HudUiContainer *)m_mainMenuDialog)->UpdateAll(g_FrameDeltaTimeSec);

        zVideo::Dispatch_UnlockPrimarySurfaceState();
    }

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
    return 0;
}

/**
 * Original-source function evidence: BN shows the retail main-menu transition OnSuspend slot
 * sharing the 0x408f50 RecoilStateDialogHost::OnSuspend body. This typed
 * definition preserves the MainMenuTransition source participant without
 * adding table or dispatch scaffolding.
 * Purpose: disable, blit, unlock, and present the hosted main-menu dialog when
 * a submenu state is pushed on top of it.
 */
void RecoilStateMainMenuTransition::OnSuspend(
    int param
) {
    (void)param;

    if (m_mainMenuDialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    m_mainMenuDialog->SetEnabled(0);
    ((HudUiDialogController *)m_mainMenuDialog)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
}
