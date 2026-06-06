#include "GameZRecoil/RecoilApp/RecoilStateDialogHost.h"

/**
 * Reimplements 0x408f50: RecoilStateDialogHost::OnWndActivate.
 *
 * Purpose: redraw and re-present the hosted HUD dialog when the application
 * receives a window activation notification.
 */
void RecoilStateDialogHost::OnWndActivate(
    int activateCode
) {
    (void)activateCode;

    if (m_dialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    m_dialog->SetEnabled(0);

    m_dialog->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
}
