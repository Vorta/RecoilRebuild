#include "GameZRecoil/RecoilApp/recoil_state_main_menu_transition.h"

#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zVideo/zvid.h"

extern char g_HudUiDialogSampleSetName[0x7];

namespace {
struct zFMV_ActionBlurStack : zFMV_ActionBlur {
    /**
     * Original inline helper observed in caller 0x415220.
     *
     * Purpose: construct the temporary blur action used while entering the
     * main-menu transition from gameplay.
     */
    zFMV_ActionBlurStack(
        int framesRemaining,
        int blurPassCount
    ) : zFMV_ActionBlur(
            framesRemaining,
            blurPassCount
        ) {}

};
} // namespace

namespace zVideo {
int __fastcall SetHalfResAdjustMode(int mode);
}

namespace HudUi {
void __fastcall SetInvalidateMode(int mode);
}

namespace zSnd {
int GetCDAudioOption();
}

namespace zSndCd {
int __fastcall PlayTrackWithMode(
    int track,
    int mode
);
}

/**
 * Reimplements 0x415220: RecoilStateMainMenuTransition::OnTryBecomeCurrent.
 *
 * Purpose: enter the main-menu transition by preparing video/HUD state,
 * pausing active sounds, loading dialog audio, constructing the menu dialog,
 * and starting CD audio when enabled.
 */
RECOIL_NO_GS int RecoilStateMainMenuTransition::OnTryBecomeCurrent() {
    if (g_zVideo_ActiveRendererPath != 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            0,
            0
        );
    }

    m_savedHalfResAdjustMode = zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        zFMV_ActionBlurStack blurAction(
            4,
            1
        );
        blurAction.Begin(0.0);
        while (blurAction.Update(0.0) != 0) {
        }
        blurAction.End();
    }

    zSndPlayHandleSnapshot *const audioSnapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
    audioSnapshot->StopAllIfPlaying();

    zSndSampleSet_InitByName(g_HudUiDialogSampleSetName);

    HudUiMainMenuDialog *const dialog = new HudUiMainMenuDialog(m_entryRoute);

    m_mainMenuDialog = dialog;

    dialog->SetEnabled(1);

    if (zSnd::GetCDAudioOption() != 0) {
        zSndCd::PlayTrackWithMode(
            2,
            5
        );
    }

    g_RecoilState_MainMenuSkipExitDelay = 0;
    return 1;
}
