#include "Battlesport/hud_ui_net_exit_panel.h"

#include "Battlesport/recoil_app.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zInput/zinput.h"

#include <new>

extern "C" {
/**
 * Reimplements data 0x4f32c0: g_HudUiNetExitPanel.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: hold the process-global network exit panel singleton.
 */
HudUiNetExitPanel *g_HudUiNetExitPanel = 0;
/**
 * Reimplements data 0x4f32bc: g_HudUiNetExitPanel_SavedInputFocus.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: preserve the prior HUD input focus while the network exit panel owns input capture.
 */
HudUiElement *g_HudUiNetExitPanel_SavedInputFocus = 0;
}

/**
 * Reimplements 0x41bd80: HudUiNetExitPanel::Constructor.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: initialize the network exit panel, bind its exit and resume widgets, and capture input focus state.
 */
HudUiNetExitPanel * HudUiNetExitPanel::Constructor() {
    new ((HudUiBackground *)this) HudUiBackground;

    resumeWidget.Constructor();
    resumeWidget.previewInputCaptureActive = 0;

    exitWidget.Constructor();
    exitWidget.previewInputCaptureActive = 0;

    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "NETEXIT",
        1
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &exitWidget,
            "EXIT"
        );
        BindWidgetByName(
            loadedSection,
            &resumeWidget,
            "RESUME"
        );
        FreeLoadedTreeRoots((int)((unsigned int)(loadedSection)));
    }

    if (zInp::GetJoystickOption() == 0) {
        g_HudUiNetExitPanel_SavedInputFocus = GetInputFocus();
        SetInputFocus(0);
    }

    SetChildFlags(0);
    SetEnabled(0);
    return this;
}

/**
 * Original helper evidence: no standalone retail function; recovered from address-backed callers in this source file.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: forward panel enabled-state changes through the HudUiBackground base implementation.
 */
void HudUiNetExitPanel::SetEnabled(
    int enabled
) {
    HudUiBackground::SetEnabled(enabled);
}

/**
 * Reimplements 0x41be70: HudUiNetExitPanel_ExitButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: queue the leave-network app state when the exit button is activated.
 */
void HudUiNetExitPanel_ExitButton::OnActivate() {
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
}

/**
 * Reimplements 0x41bf10: HudUiNetExitPanel_ResumeWidget::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: close the preview, hide the network exit panel, and dispatch normal ZRD activation.
 */
void HudUiNetExitPanel_ResumeWidget::OnActivate() {
    HidePreview();
    g_HudUiNetExitPanel->SetEnabled(0);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x41bf40: HudUiNetExitPanel_ResumeWidget::OnShowPreview.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: push preview input capture, restore saved focus for mouse mode, and show the resume preview.
 */
void HudUiNetExitPanel_ResumeWidget::OnShowPreview() {
    if (previewInputCaptureActive == 0) {
        zInput::BindMapContext_Push(0);
        zInput::BindMapCurrent_SetMouseBinding(
            1,
            0
        );

        if (zInp::GetJoystickOption() == 0) {
            HudUiMgr::UpdateTargetReticleFromCursor(
                0,
                0,
                0.0f,
                0.0f
            );

            HudUiElement *const focus = g_HudUiNetExitPanel_SavedInputFocus;
            if (focus != 0) {
                ((HudUiBackgroundContainer *)(owner))->SetInputFocus(focus);
            }
        }

        previewInputCaptureActive = 1;
    }

    ShowPreview();
}

/**
 * Reimplements 0x41bfa0: HudUiNetExitPanel_ResumeWidget::OnHidePreview.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: pop preview input capture, save current focus for mouse mode, and hide the resume preview.
 */
void HudUiNetExitPanel_ResumeWidget::OnHidePreview() {
    if (previewInputCaptureActive != 0) {
        zInput::BindMapContext_Pop();

        if (zInp::GetJoystickOption() == 0) {
            HudUiMgr::UpdateTargetReticleFromCursor(
                1,
                0,
                0.0f,
                0.0f
            );
            HudUiBackgroundContainer *const backgroundOwner = (HudUiBackgroundContainer *)(owner);
            g_HudUiNetExitPanel_SavedInputFocus = backgroundOwner->GetInputFocus();
            backgroundOwner->SetInputFocus(0);
        }

        previewInputCaptureActive = 0;
    }

    HidePreview();
}

/**
 * Reimplements 0x41c000: HudUiNetExitPanel::CreateGlobal.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: allocate and construct the process-global network exit panel singleton.
 */
HudUiNetExitPanel *HudUiNetExitPanel::CreateGlobal() {
    HudUiNetExitPanel *const panel =
        (HudUiNetExitPanel *)(::operator new(sizeof(HudUiNetExitPanel)));
    if (panel == 0) {
        g_HudUiNetExitPanel = 0;
        return 0;
    }

    g_HudUiNetExitPanel = panel->Constructor();
    return g_HudUiNetExitPanel;
}

/**
 * Reimplements 0x41c070: HudUiNetExitPanel::Show.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: enable the process-global network exit panel.
 */
void HudUiNetExitPanel::Show() {
    g_HudUiNetExitPanel->SetEnabled(1);
}

/**
 * Reimplements 0x41c080: HudUiNetExitPanel::Tick.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: tick the process-global network exit panel with the frame delta.
 */
int HudUiNetExitPanel::Tick() {
    g_HudUiNetExitPanel->UpdateAll(g_FrameDeltaTimeSec);
    return 0;
}

/**
 * Reimplements 0x41c0a0: HudUiNetExitPanel::DestroyGlobal.
 * Original source path: D:\Proj\Battlesport\HudUi_NetExit.cpp.
 * Purpose: destroy and release the process-global network exit panel singleton.
 */
void HudUiNetExitPanel::DestroyGlobal() {
    HudUiNetExitPanel *const panel = g_HudUiNetExitPanel;
    if (panel != 0) {
        delete panel;
        g_HudUiNetExitPanel = 0;
    }
}
