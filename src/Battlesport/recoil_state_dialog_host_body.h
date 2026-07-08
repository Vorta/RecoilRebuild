#include "GameZRecoil/RecoilApp/recoil_state_dialog_host.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zVideo/zvid.h"

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
