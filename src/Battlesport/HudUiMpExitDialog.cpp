#include "Battlesport/RecoilApp.h"
#include "Battlesport/HudUiMpExitDialog.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"

#include <new>
#include <stdio.h>
#include <string.h>
#include <windows.h>

extern "C" HWND g_RecoilApp_hWndMain;

HudUiMpExitDialog *g_HudUiMpExitDialog = 0;

/**
 * Reimplements 0x419650: HudUiMpExitDialog::UnloadLayout.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: disable and unload the multiplayer exit dialog presentation state and release its captured background image.
 */
void HudUiMpExitDialog::UnloadLayout() {
    SetEnabled(0);
    Update(0.0f);
    HudScoreboard::SetScaleAndRebuild(0.0f);
    g_HudUiTopMessageStack->Clear();
    if (m_capturedBackgroundImage != 0) {
        m_capturedBackgroundImage =
            (zVidImagePartial *)(unsigned int)zVid_Image::ReleaseIfNotDefault(
                m_capturedBackgroundImage
            );
    }
}

/**
 * Reimplements 0x419690: HudUiMpExitDialog::Update.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: advance the multiplayer exit fade/update path and restore the captured background through the video postprocess pass.
 */
void HudUiMpExitDialog::Update(
    float deltaSeconds
) {
    if (m_mpNewGameButtonMode >= 0) {
        const float fadeElapsedSeconds = m_fadeElapsedSeconds + deltaSeconds;
        m_fadeElapsedSeconds = fadeElapsedSeconds;
        HudScoreboard::SetScaleAndRebuild(fadeElapsedSeconds < 1.0f ? fadeElapsedSeconds : 1.0f);
    }

    zVideo::RunPostprocessOnPrimaryBuffer();
    zVid_Image::BlitToActiveTarget(
        m_capturedBackgroundImage,
        0,
        0,
        0,
        0
    );
    HudUiBackground::Update(deltaSeconds);

    if (m_mpNewGameButtonMode >= 0) {
        HudScoreboard::DispatchSetScale(deltaSeconds);
    } else {
        g_HudUiTopMessageStack->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    }

    zVideo::Dispatch_UnlockPrimarySurfaceState();
    zOpt_ViewRectSection *const dstRect = zOpt::GetWindowSection();
    zOpt_ViewRectSection *const srcRect = zOpt::GetWindowSection();
    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)srcRect,
        (zVidRect32 *)dstRect,
        0,
        1
    );
}

/**
 * Reimplements 0x419500: HudUiMpExitDialog::LoadLayout.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: capture and blur the current surface, load the MPEXIT dialog layout, and configure button or network-message state.
 */
void HudUiMpExitDialog::LoadLayout() {
    m_mpNewGameButtonMode = HudUiMgr::IsLocalPlayerFirstInStatsList();

    zVidImagePartial *const image = zVideo_buff_CaptureSurfaceToImage(1);
    m_capturedBackgroundImage = image;
    const int imageWidth = image->width;
    zVideo::Fx_SetSurfaceState(
        image->pixels,
        imageWidth,
        image->height,
        imageWidth * 2
    );
    zVideo::buff_BlurRegionByMode(
        0,
        3
    );
    zVideo::buff_BlurRegionByMode(
        0,
        3
    );
    zVideo::buff_BlurRegionByMode(
        0,
        3
    );

    HudScoreboard::SetScaleAndRebuild(0.0f);

    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "MPEXIT",
        1
    );
    if (loadedSection != 0) {
        if (m_mpNewGameButtonMode >= 0) {
            BindWidgetByName(
                loadedSection,
                &m_mpNewGameButton,
                "MPNEWGAME"
            );
        }
        BindWidgetByName(
            loadedSection,
            &m_mpExitButton,
            "MPEXITBTN"
        );
        FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    SetChildFlags(0);
    if (m_mpNewGameButtonMode >= 0) {
        HudUiZrdWidget *const newGameButton = &m_mpNewGameButton;
        newGameButton->modeOrEnabled = m_mpNewGameButtonMode;
        newGameButton->RefreshState();
    } else {
        HudUiMgr::EnableTopAndChatStacks();
        g_HudUiTopMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
        if (zOpt::GetNetworkModemEnabled() != 0) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x25),
                300.0f
            );
        } else {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x39),
                300.0f
            );
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x40),
                300.0f
            );
        }
    }

    m_fadeElapsedSeconds = 0.0f;
    SetEnabled(1);
}

/**
 * Reimplements 0x419800: HudUiMpExitDialog_MpNewGameButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: queue the intro FMV and multiplayer setup reconfiguration when the new-game button is activated.
 */
void HudUiMpExitDialog_NewGameButton::OnActivate() {
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_introFmvState,
        0
    );
    HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(1);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x419830: HudUiMpExitDialog_MpExitButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: run the base widget activation and queue the leave-network state.
 */
void HudUiMpExitDialog_ExitButton::OnActivate() {
    HudUiZrdWidget::OnActivate();
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
}

/**
 * Reimplements 0x419870: HudUiMpExitDialog::Destructor.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: destroy the exit and new-game child widgets before tearing down the background base.
 */
void HudUiMpExitDialog::Destructor() {
    m_mpExitButton.DestructorCore();
    m_mpNewGameButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x419850: HudUiMpExitDialog::ScalarDeletingDestructorThunk.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: run dialog destruction and optionally release the dialog storage.
 */
HudUiBackground * HudUiMpExitDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Reimplements 0x419740: RecoilApp_MpExitDialogState::OnEnter.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: lazily construct the multiplayer exit dialog singleton and load its layout in software video mode.
 */
void RecoilApp_MpExitDialogState::OnEnter() {
    if (g_HudUiMpExitDialog == 0) {
        HudUiMpExitDialog *dialog = (HudUiMpExitDialog *) ::operator new(sizeof(HudUiMpExitDialog));
        if (dialog != 0) {
            new (dialog) HudUiMpExitDialog;
        }

        g_HudUiMpExitDialog = dialog;
    }

    if (zVid::GetAccelerationOption() == 0) {
        g_HudUiMpExitDialog->LoadLayout();
    }
}

/**
 * Reimplements 0x4198d0: RecoilApp_MpExitDialogState::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: configure render, sound, and input state before entering the multiplayer exit dialog.
 */
int RecoilApp_MpExitDialogState::OnTryBecomeCurrent() {
    zVideo::SetHalfResAdjustMode(0);
    HudUi::SetInvalidateMode(0);

    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        zOpt::GetWindowSection(),
        zOpt::GetDisplaySectionBitsPerPixel(),
        zVideo::GetPrimarySurfacePitch()
    );

    zSndSampleSet_InitByName("DIALOG");
    zInput::BindMapContext_Push(0);
    zInput::BindMapCurrent_ResetAllBindings();

    if (zVid::GetAccelerationOption() != 0) {
        g_HudUiMpExitDialog->LoadLayout();
    }

    return 1;
}

/**
 * Reimplements 0x419940: RecoilApp_MpExitDialogState::OnDeactivate.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: unload, destroy, and clear the multiplayer exit dialog and restore input, sound, and scoreboard state.
 */
void RecoilApp_MpExitDialogState::OnDeactivate() {
    g_HudUiMpExitDialog->UnloadLayout();

    HudUiMpExitDialog *const dialog = g_HudUiMpExitDialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
    }

    g_HudUiMpExitDialog = 0;
    zInput::BindMapContext_Pop();
    Sleep(1000);
    zSndSampleSet_DestroyByName("DIALOG");
    HudScoreboard::SetScaleAndRebuild(0.0f);
}

/**
 * Reimplements 0x419990: RecoilApp_MpExitDialogState::OnUpdateShouldQuit.
 * Original source path: D:\Proj\Battlesport\HudUiMpExitDialog.cpp.
 * Purpose: poll input, tick/update the dialog, and run the fatal timeout shutdown path after a long stalled fade.
 */
int RecoilApp_MpExitDialogState::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);
    Time::Tick();

    HudUiMpExitDialog *const dialog = g_HudUiMpExitDialog;
    ((HudUiContainer *)dialog)->UpdateAll(g_FrameDeltaTimeSec);

    if (g_HudUiMpExitDialog->m_fadeElapsedSeconds > 600.0f) {
        char caption[128];
        char text[128];

        strcpy(
            caption,
            zLoc::GetMessageString(28)
        );
        strcpy(
            text,
            zLoc::GetMessageString(29)
        );
        zVideo_dd::FlipToGDIIfAttached();
        zSndSystem::Shutdown();
        zNetwork::ShutdownSessionRuntime();
        zVideo::ShutdownVideoSystem();
        printf(
            "%s: %s\n",
            caption,
            text
        );
        Sleep(1000);
        MessageBeep(MB_ICONHAND);
        MessageBoxA(
            g_RecoilApp_hWndMain,
            text,
            caption,
            MB_ICONHAND
        );
        zSys::ExitProcessWithCleanup(0);
    }

    return 0;
}
