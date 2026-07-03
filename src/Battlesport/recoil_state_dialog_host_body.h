#include "GameZRecoil/RecoilApp/recoil_state_dialog_host.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zVideo/zvid.h"

/**
 * Reimplements 0x4099a0: RecoilStateDialogHost::OnWndActivate.
 *
 * Purpose: refresh the hosted HUD dialog surfaces when the application is
 * reactivated.
 */
void RecoilStateDialogHost::OnWndActivate(
    int activateCode
) {
    if (activateCode == 0) {
        return;
    }

    if (m_dialog == 0) {
        return;
    }

    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
    m_dialog->InvalidateChildren();
}

/**
 * Reimplements 0x435e80: RecoilStateSaveLoadTransition::OnUpdateShouldQuit
 * (BN canonical folded body).
 *
 * Source owner: app_shell.folded_dialog_update_should_quit. BN shows the
 * retail body shared by DialogHost, MainMenuTransition, SaveLoadTransition,
 * and other dialog-hosted state vtable slots; this definition preserves the
 * DialogHost typed participant.
 *
 * Original-source function evidence: folded retail body 0x435e80.
 * Purpose: update and present the hosted HUD dialog each frame while a dialog
 * app state is current.
 */
int RecoilStateDialogHost::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);

    if (m_dialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        m_dialog->UpdateAll(g_FrameDeltaTimeSec);

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
 * Reimplements 0x409ad0: RecoilStateDialogHost::OnDeactivate.
 *
 * Purpose: disable, repaint, destroy, and clear the active hosted HUD dialog.
 */
void RecoilStateDialogHost::OnDeactivate() {
    if (m_dialog == 0) {
        return;
    }

    m_dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();

    if (m_dialog != 0) {
        ((HudUiBackground *)m_dialog)->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

/**
 * Reimplements 0x408f50: RecoilStateDialogHost::OnSuspend.
 *
 * Purpose: disable, blit, unlock, and present the hosted HUD dialog when
 * another app state is pushed on top of it.
 */
void RecoilStateDialogHost::OnSuspend(
    int suspendParam
) {
    (void)suspendParam;

    if (m_dialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    m_dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
}
