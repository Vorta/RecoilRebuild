#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zvid.h"

/**
 * Reimplements logical authored override folded at 0x408f50:
 * RecoilStateControls::OnSuspend.
 *
 * Purpose: disable, blit, unlock, and present the hosted HUD dialog when
 * another app state is pushed on top of it.
 */
void RecoilStateControls::OnSuspend(
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
