#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "GameZRecoil/zGame/zGame.h"

#include <windows.h>

namespace zVideo {
int RECOIL_FASTCALL SetHalfResAdjustMode(int mode);
int RECOIL_FASTCALL Init_ApplyModeIndex(int modeIndex);
} // namespace zVideo

namespace HudUi {
void RECOIL_FASTCALL SetInvalidateMode(int mode);
}

namespace zVid {
int RECOIL_CDECL GetVideoModeIndexFromOptions();
void RECOIL_FASTCALL SetVideoModeIndex(int modeIndex);
} // namespace zVid

namespace {
enum zVideoHalfResAdjustMode {
    ZVIDEO_HALFRES_ADJUST_DISABLED = 0,
    ZVIDEO_HALFRES_ADJUST_ENABLED = 1,
};

enum zSndCdAudioOption {
    ZSND_CDAUDIO_DISABLED = 0,
};

extern "C" int g_RecoilState_MainMenuSkipExitDelay;

static RECOIL_FORCEINLINE void ApplyDeferredVideoMode(int targetMode,
                                                      zVideoHalfResAdjustMode halfResMode) {
    if (zVid::GetVideoModeIndexFromOptions() == targetMode) {
        return;
    }

    if (zVideo::Init_ApplyModeIndex(targetMode) != 0) {
        return;
    }

    zVid::SetVideoModeIndex(targetMode);
    zVideo::SetHalfResAdjustMode(halfResMode);
    HudUi::SetInvalidateMode(halfResMode);
}
} // namespace

namespace zOpt {
int RECOIL_FASTCALL SetHudTypeForCurrentHwMode(int hudType);
}

namespace HudUiMgr {
void RECOIL_CDECL TriggerCurrentLayoutOnActivated();
}

namespace zInput {
void RECOIL_CDECL Keyboard_ResetTransitionState();
}

namespace zSnd {
int RECOIL_CDECL GetCDAudioOption();
}

// Reimplements 0x4153d0: RecoilStateMainMenuTransition::OnDeactivate
void RECOIL_THISCALL RecoilStateMainMenuTransition::OnDeactivate() {
    int previousHudType;

    if (m_mainMenuDialog != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();

        HudUiMainMenuDialogVirtual *dialog =
            (HudUiMainMenuDialogVirtual *)(unsigned int)m_mainMenuDialog;
        dialog->SetEnabled(0);

        ((HudUiDialogController *)(unsigned int)m_mainMenuDialog)->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        dialog = (HudUiMainMenuDialogVirtual *)(unsigned int)m_mainMenuDialog;
        if (dialog != 0) {
            dialog->ScalarDeletingDestructor(1);
        }

        m_mainMenuDialog = 0;
    }

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        previousHudType = zOpt::SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_STANDARD);
    }

    switch (m_deferredVideoModeIndex) {
    case 5:
        ApplyDeferredVideoMode(5, ZVIDEO_HALFRES_ADJUST_ENABLED);
        break;
    case 3:
        ApplyDeferredVideoMode(3, ZVIDEO_HALFRES_ADJUST_DISABLED);
        break;
    case 4:
        ApplyDeferredVideoMode(4, ZVIDEO_HALFRES_ADJUST_ENABLED);
        break;
    case 2:
        ApplyDeferredVideoMode(2, ZVIDEO_HALFRES_ADJUST_DISABLED);
        break;
    case 6:
        ApplyDeferredVideoMode(6, ZVIDEO_HALFRES_ADJUST_ENABLED);
        break;
    case 7:
        ApplyDeferredVideoMode(7, ZVIDEO_HALFRES_ADJUST_ENABLED);
        break;
    default:
        zVideo::SetHalfResAdjustMode(m_savedHalfResAdjustMode);
        HudUi::SetInvalidateMode(m_savedHalfResAdjustMode);
        break;
    }

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        zOpt::SetHudTypeForCurrentHwMode(previousHudType);
    }

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        HudUiMgr::TriggerCurrentLayoutOnActivated();
    }

    zInput::Keyboard_ResetTransitionState();

    if (g_RecoilState_MainMenuSkipExitDelay == 0) {
        Sleep(0x3e8);
        zSndSampleSet_DestroyByName("DIALOG");

        zSndPlayHandleSnapshot *snapshot =
            (zSndPlayHandleSnapshot *)(unsigned int)m_pausedAudioSnapshot;
        if (snapshot != 0) {
            snapshot->RestoreAllWithGlobalVolumeDelta();
        }

        snapshot = (zSndPlayHandleSnapshot *)(unsigned int)m_pausedAudioSnapshot;
        if (snapshot != 0) {
            snapshot->Destroy();
            m_pausedAudioSnapshot = 0;
        }
    }

    if (zSnd::GetCDAudioOption() != ZSND_CDAUDIO_DISABLED) {
        zSndCd::Stop();
    }
}
