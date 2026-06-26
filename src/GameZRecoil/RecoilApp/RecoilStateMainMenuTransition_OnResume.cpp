#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "GameZRecoil/zGame/zGame.h"

/**
 * Reimplements 0x415370: RecoilStateMainMenuTransition::OnResume.
 *
 * Purpose: re-enable and refresh the main-menu dialog after a child state
 * resumes back into the menu transition state.
 */
void RecoilStateMainMenuTransition::OnResume(
    int param
) {
    if (m_mainMenuDialog == 0 || param != 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    m_mainMenuDialog->SetEnabled(1);
    ((HudUiContainer *)m_mainMenuDialog)->InvalidateChildren();
    ((HudUiContainer *)m_mainMenuDialog)->UpdateAll(0.0f);

    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
}
