#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zVideo/zVideo.h"

namespace {
enum zVideoHalfResAdjustMode {
    ZVIDEO_HALFRES_ADJUST_DISABLED = 0,
};

enum zVideoRendererBackend {
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};

enum zSndCdAudioOption {
    ZSND_CDAUDIO_DISABLED = 0,
};

extern "C" int g_RecoilState_MainMenuSkipExitDelay;

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
    ) {
        Constructor(
            framesRemaining,
            blurPassCount
        );
    }

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
    if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
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

    zSndSampleSet_InitByName("DIALOG");

    HudUiMainMenuDialog *const dialog = new HudUiMainMenuDialog(m_entryRoute);

    m_mainMenuDialog = dialog;

    dialog->SetEnabled(1);

    if (zSnd::GetCDAudioOption() != ZSND_CDAUDIO_DISABLED) {
        zSndCd::PlayTrackWithMode(
            2,
            5
        );
    }

    g_RecoilState_MainMenuSkipExitDelay = 0;
    return 1;
}
