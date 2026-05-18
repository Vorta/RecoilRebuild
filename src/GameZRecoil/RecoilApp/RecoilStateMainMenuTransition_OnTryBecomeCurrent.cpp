#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"


namespace {
const RecoilPtr32 kZfmvActionBase_VtableAddress = 0x004cee50;
const unsigned int kHudUiMainMenuDialogAllocationSize = 0xb3ac;

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
} // namespace

struct zFMV_ActionBlur;

struct zFMV_ActionBlur_Vtbl {
    RecoilFn32 Destroy;
    int(RECOIL_THISCALL *pfnUpdate)(zFMV_ActionBlur *self, int, int);
    void(RECOIL_THISCALL *pfnBegin)(zFMV_ActionBlur *self, int, int);
    void(RECOIL_THISCALL *pfnEnd)(zFMV_ActionBlur *self);
};

struct zFMV_ActionBlur {
    RecoilPtr32 vftable; // zFMV_ActionBlur_Vtbl*
    unsigned char storage[0x2c];

    zFMV_ActionBlur *RECOIL_THISCALL Constructor(int transitionKind,
                                                 int capturePrimary);
};
RECOIL_STATIC_ASSERT(sizeof(zFMV_ActionBlur) == 0x30);

namespace zVideo {
int RECOIL_FASTCALL SetHalfResAdjustMode(int mode);
}

namespace HudUi {
void RECOIL_FASTCALL SetInvalidateMode(int mode);
}

namespace zSnd {
int RECOIL_CDECL GetCDAudioOption();
}

namespace zSndCd {
int RECOIL_FASTCALL PlayTrackWithMode(int track, int mode);
}

// Reimplements 0x415220: RecoilStateMainMenuTransition::OnTryBecomeCurrent
RECOIL_NO_GS int RECOIL_THISCALL RecoilStateMainMenuTransition::OnTryBecomeCurrent() {
    if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(0, 0);
    }

    m_savedHalfResAdjustMode = zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        zFMV_ActionBlur blurAction;
        blurAction.Constructor(4, 1);

        zFMV_ActionBlur_Vtbl *blurVtbl =
            (zFMV_ActionBlur_Vtbl *)(unsigned int)blurAction.vftable;
        blurVtbl->pfnBegin(&blurAction, 0, 0);

        while (blurVtbl->pfnUpdate(&blurAction, 0, 0) != 0) {
        }

        blurVtbl->pfnEnd(&blurAction);
        blurAction.vftable = kZfmvActionBase_VtableAddress;
    }

    zSndPlayHandleSnapshot *const audioSnapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
    audioSnapshot->StopAllIfPlaying();

    zSndSampleSet_InitByName("DIALOG");

    HudUiMainMenuDialog *const storage =
        (HudUiMainMenuDialog *)::operator new(kHudUiMainMenuDialogAllocationSize);
    HudUiMainMenuDialog *dialog = 0;
    if (storage != 0) {
        dialog = storage->Constructor(m_entryRoute);
    }

    m_mainMenuDialog = (RecoilPtr32)(unsigned int)dialog;

    HudUiMainMenuDialog_Vtbl *const dialogVtbl =
        (HudUiMainMenuDialog_Vtbl *)(unsigned int)dialog->vftable;
    dialogVtbl->SetEnabled(dialog, 1);

    if (zSnd::GetCDAudioOption() != ZSND_CDAUDIO_DISABLED) {
        zSndCd::PlayTrackWithMode(2, 5);
    }

    g_RecoilState_MainMenuSkipExitDelay = 0;
    return 1;
}
