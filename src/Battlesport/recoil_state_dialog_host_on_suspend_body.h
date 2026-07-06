#include "GameZRecoil/RecoilApp/recoil_state_dialog_host.h"

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zvid.h"

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
