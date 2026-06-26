#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zVideo/zVideo.h"

/**
 * Reimplements 0x435e80: RecoilStateMainMenuTransition::OnUpdateShouldQuit.
 *
 * Purpose: update and present the active main-menu dialog each frame while the
 * transition state is current. Retail folds this byte-identical body with
 * RecoilStateSaveLoadTransition::OnUpdateShouldQuit at 0x435e80.
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
 * Original-source helper evidence: No standalone retail function exists for
 * this typed vtable wrapper; BN shows the retail main-menu transition
 * OnSuspend slot pointing at 0x408f50 RecoilStateDialogHost::OnWndActivate.
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
