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

    if (dialog_04 == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    ((HudUiDialogControllerVirtual *)dialog_04)->SetEnabled(0);

    ((HudUiDialogController *)dialog_04)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
}
