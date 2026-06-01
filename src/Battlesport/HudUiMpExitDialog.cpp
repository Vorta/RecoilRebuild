#include "Battlesport/HudUiMpExitDialog.h"

#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>

extern "C" HWND g_RecoilApp_hWndMain;

namespace {
template <typename Method>
unsigned int MpExitMethodAddress(
    Method method
) {
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(
        &address,
        &method,
        sizeof(method)
    );
    return address;
}

RECOIL_NOINLINE void RECOIL_CDECL HudUiMpExitDialogPostLoadNoOp() {}

HudUiWidget_FTable MakeMpExitDialogButtonFTable(
    unsigned int activateCallback
) {
    HudUiWidget_FTable table = {0};
    table.slots[0] = MpExitMethodAddress(&HudUiZrdWidget::ScalarDeletingDestructorThunk);
    table.slots[1] = MpExitMethodAddress(&HudUiWidget::Draw);
    table.slots[3] = MpExitMethodAddress(&HudUiElement::SetPos);
    table.slots[4] = MpExitMethodAddress(&HudUiElement::SetX);
    table.slots[5] = MpExitMethodAddress(&HudUiElement::SetY);
    table.slots[6] = MpExitMethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = MpExitMethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = MpExitMethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = activateCallback;
    table.slots[15] = MpExitMethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = MpExitMethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = MpExitMethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = MpExitMethodAddress(&HudUiElement::GetX);
    table.slots[26] = MpExitMethodAddress(&HudUiElement::GetY);
    table.slots[30] = MpExitMethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = MpExitMethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = MpExitMethodAddress(&HudUiMpExitDialogPostLoadNoOp);
    return table;
}

HudUiMpExitDialog_Vtbl MakeMpExitDialogVtable() {
    HudUiMpExitDialog_Vtbl table = {0};
    table.slots[0] = MpExitMethodAddress(&HudUiMpExitDialog::Update);
    table.slots[1] = MpExitMethodAddress(&HudUiBackground::SetEnabled);
    table.slots[2] = MpExitMethodAddress(&HudUiMpExitDialog::ScalarDeletingDestructorThunk);
    return table;
}
} // namespace

const HudUiWidget_FTable g_HudUiZrdWidget_MpExitDialog_NewGameButton_Vtbl =
    MakeMpExitDialogButtonFTable(MpExitMethodAddress(&HudUiMpExitDialog_NewGameButton::OnActivate));
const HudUiWidget_FTable g_HudUiZrdWidget_MpExitDialog_ExitButton_Vtbl =
    MakeMpExitDialogButtonFTable(MpExitMethodAddress(&HudUiMpExitDialog_ExitButton::OnActivate));
const HudUiMpExitDialog_Vtbl g_HudUiMpExitDialog_Vtbl = MakeMpExitDialogVtable();
HudUiMpExitDialog *g_HudUiMpExitDialog = 0;

// Reimplements 0x419650: HudUiMpExitDialog::UnloadLayout
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiMpExitDialog::UnloadLayout() {
    base.SetEnabled(0);
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

// Reimplements 0x419690: HudUiMpExitDialog::Update
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiMpExitDialog::Update(
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
    base.Update(deltaSeconds);

    if (m_mpNewGameButtonMode >= 0) {
        HudScoreboard::DispatchSetScale(deltaSeconds);
    } else {
        g_HudUiTopMessageStack->base.UpdateAll(g_Time_UnscaledDeltaTimeSec);
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

// Reimplements 0x419500: HudUiMpExitDialog::LoadLayout
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiMpExitDialog::LoadLayout() {
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

    zReader::Node *const loadedSection = base.LoadFromZrd(
        "dialog.zrd",
        "MPEXIT",
        1
    );
    if (loadedSection != 0) {
        if (m_mpNewGameButtonMode >= 0) {
            base.BindWidgetByName(
                loadedSection,
                &m_mpNewGameButton.base,
                "MPNEWGAME"
            );
        }
        base.BindWidgetByName(
            loadedSection,
            &m_mpExitButton.base,
            "MPEXITBTN"
        );
        base.FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    base.base.base.SetChildFlags(0);
    if (m_mpNewGameButtonMode >= 0) {
        m_mpNewGameButton.modeOrEnabled = m_mpNewGameButtonMode;
        m_mpNewGameButton.RefreshState();
    } else {
        HudUiMgr::EnableTopAndChatStacks();
        g_HudUiTopMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
        if (zOpt::GetNetworkModemEnabled() == 0) {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x39),
                300.0f
            );
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x40),
                300.0f
            );
        } else {
            HudUi::ShowTopMessageLine(
                zLoc::GetMessageString(0x25),
                300.0f
            );
        }
    }

    m_fadeElapsedSeconds = 0.0f;
    base.SetEnabled(1);
}

// Reimplements 0x419800: HudUiMpExitDialog_MpNewGameButton::OnActivate
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
void RECOIL_THISCALL HudUiMpExitDialog_NewGameButton::OnActivate() {
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_introFmvState_1a0.base,
        0
    );
    HudUiNetGameSetupOverlayOwner::QueueEnterWithReconfigureFlag(1);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x419830: HudUiMpExitDialog_MpExitButton::OnActivate
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
void RECOIL_THISCALL HudUiMpExitDialog_ExitButton::OnActivate() {
    HudUiZrdWidget::OnActivate();
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState_1d0.base,
        0
    );
}

// Reimplements 0x419870: HudUiMpExitDialog::Destructor
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiMpExitDialog::Destructor() {
    m_mpExitButton.DestructorCore();
    m_mpNewGameButton.DestructorCore();
    base.Destructor();
}

// Reimplements 0x419850: HudUiMpExitDialog::ScalarDeletingDestructorThunk
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
HudUiMpExitDialog *RECOIL_THISCALL HudUiMpExitDialog::ScalarDeletingDestructorThunk(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x419740: RecoilApp_MpExitDialogState::OnEnter
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_MpExitDialogState::OnEnter() {
    if (g_HudUiMpExitDialog == 0) {
        HudUiMpExitDialog *dialog = (HudUiMpExitDialog *) ::operator new(sizeof(HudUiMpExitDialog));
        if (dialog != 0) {
            dialog->base.Constructor();
            dialog->m_mpNewGameButton.Constructor();
            dialog->m_mpNewGameButton.base.ftable =
                &g_HudUiZrdWidget_MpExitDialog_NewGameButton_Vtbl;
            dialog->m_mpExitButton.Constructor();
            dialog->m_mpExitButton.base.ftable = &g_HudUiZrdWidget_MpExitDialog_ExitButton_Vtbl;
            dialog->base.base.base.vptr = (const HudUiContainer_FTable *)&g_HudUiMpExitDialog_Vtbl;
        }

        g_HudUiMpExitDialog = dialog;
    }

    if (zVid::GetAccelerationOption() == 0) {
        g_HudUiMpExitDialog->LoadLayout();
    }
}

// Reimplements 0x4198d0: RecoilApp_MpExitDialogState::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_MpExitDialogState::OnTryBecomeCurrent() {
    zVideo::SetHalfResAdjustMode(0);
    HudUi::SetInvalidateMode(0);

    const int pitchBytes = zVideo::GetPrimarySurfacePitch();
    const int bitsPerPixel = zOpt::GetDisplaySectionBitsPerPixel();
    zOpt_ViewRectSection *const activeRegionRect = zOpt::GetWindowSection();
    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        activeRegionRect,
        bitsPerPixel,
        pitchBytes
    );

    zSndSampleSet_InitByName("DIALOG");
    zInput::BindMapContext_Push(0);
    zInput::BindMapCurrent_ResetAllBindings();

    if (zVid::GetAccelerationOption() != 0) {
        g_HudUiMpExitDialog->LoadLayout();
    }

    return 1;
}

// Reimplements 0x419940: RecoilApp_MpExitDialogState::OnDeactivate
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL RecoilApp_MpExitDialogState::OnDeactivate() {
    g_HudUiMpExitDialog->UnloadLayout();

    HudUiMpExitDialog *const dialog = g_HudUiMpExitDialog;
    if (dialog != 0) {
        typedef HudUiMpExitDialog *(RECOIL_THISCALL * ScalarDeletingDtorFn)(
            HudUiMpExitDialog * self,
            unsigned int flags
        );
        const HudUiMpExitDialog_Vtbl *const vtable =
            (const HudUiMpExitDialog_Vtbl *)dialog->base.base.base.vptr;
        ((ScalarDeletingDtorFn)vtable->slots[2])(
            dialog,
            1
        );
    }

    g_HudUiMpExitDialog = 0;
    zInput::BindMapContext_Pop();
    Sleep(1000);
    zSndSampleSet_DestroyByName("DIALOG");
    HudScoreboard::SetScaleAndRebuild(0.0f);
}

// Reimplements 0x419990: RecoilApp_MpExitDialogState::OnUpdateShouldQuit
// (D:\Proj\Battlesport\HudUiMpExitDialog.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL RecoilApp_MpExitDialogState::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);
    Time::Tick();

    HudUiMpExitDialog *const dialog = g_HudUiMpExitDialog;
    typedef void(RECOIL_THISCALL * UpdateFn)(
        HudUiMpExitDialog * self,
        float deltaSeconds
    );
    const HudUiMpExitDialog_Vtbl *const vtable =
        (const HudUiMpExitDialog_Vtbl *)dialog->base.base.base.vptr;
    ((UpdateFn)vtable->slots[0])(
        dialog,
        g_FrameDeltaTimeSec
    );

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
