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

struct zFMV_ActionVirtual {
    virtual zFMV_Action *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    virtual int RECOIL_THISCALL Update(double timeSec);
    virtual void RECOIL_THISCALL Begin(double timeSec);
    virtual void RECOIL_THISCALL End();
    virtual void RECOIL_THISCALL RunBlocking();
    void *reserved14;
};

struct zFMV_ActionBlurStack : zFMV_ActionBlur {
    zFMV_ActionBlurStack(int framesRemaining, int blurPassCount) {
        Constructor(framesRemaining, blurPassCount);
    }

    ~zFMV_ActionBlurStack() {
        vftable = &g_zFMV_ActionBase_Vtable;
    }
};
} // namespace

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
        zFMV_ActionBlurStack blurAction(4, 1);
        zFMV_ActionVirtual *const actionVirtual = (zFMV_ActionVirtual *)&blurAction;
        actionVirtual->Begin(0.0);
        while (actionVirtual->Update(0.0) != 0) {
        }
        actionVirtual->End();
    }

    zSndPlayHandleSnapshot *const audioSnapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
    audioSnapshot->StopAllIfPlaying();

    zSndSampleSet_InitByName("DIALOG");

    HudUiMainMenuDialog *const storage =
        (HudUiMainMenuDialog *)::operator new(sizeof(HudUiMainMenuDialog));
    HudUiMainMenuDialog *dialog = 0;
    if (storage != 0) {
        dialog = storage->Constructor(m_entryRoute);
    }

    m_mainMenuDialog = (RecoilPtr32)(unsigned int)dialog;

    HudUiMainMenuDialogVirtual *const dialogVirtual = (HudUiMainMenuDialogVirtual *)dialog;
    dialogVirtual->SetEnabled(1);

    if (zSnd::GetCDAudioOption() != ZSND_CDAUDIO_DISABLED) {
        zSndCd::PlayTrackWithMode(2, 5);
    }

    g_RecoilState_MainMenuSkipExitDelay = 0;
    return 1;
}
