#include "Battlesport/HudUiNetExitPanel.h"

#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/zInput/zInput.h"

#include <new>

extern "C" {
HudUiNetExitPanel *g_HudUiNetExitPanel = 0;
HudUiElement *g_HudUiNetExitPanel_SavedInputFocus = 0;
}

// Reimplements 0x41bd80: HudUiNetExitPanel::Constructor
// (D:\Proj\Battlesport\HudUi_NetExit.cpp)
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

// Reimplements 0x41beb0: HudUiNetExitPanel::Destructor
// (D:\Proj\Battlesport\HudUi_NetExit.cpp)
void HudUiNetExitPanel::Destructor() {
    exitWidget.DestructorCore();
    resumeWidget.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

void HudUiNetExitPanel::Update(
    float deltaSeconds
) {
    HudUiBackground::Update(deltaSeconds);
}

void HudUiNetExitPanel::SetEnabled(
    int enabled
) {
    HudUiBackground::SetEnabled(enabled);
}

// Reimplements 0x41be70: HudUiNetExitPanel_ExitButton::OnActivate
void HudUiNetExitPanel_ExitButton::OnActivate() {
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
}

// Reimplements 0x41bf10: HudUiNetExitPanel_ResumeWidget::OnActivate
void HudUiNetExitPanel_ResumeWidget::OnActivate() {
    HidePreview();
    g_HudUiNetExitPanel->SetEnabled(0);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x41bf40: HudUiNetExitPanel_ResumeWidget::OnShowPreview
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

// Reimplements 0x41bfa0: HudUiNetExitPanel_ResumeWidget::OnHidePreview
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

// Reimplements 0x41c000: HudUiNetExitPanel::CreateGlobal
// (D:\Proj\Battlesport\HudUi_NetExit.cpp)
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

// Reimplements 0x41c070: HudUiNetExitPanel::Show
void HudUiNetExitPanel::Show() {
    g_HudUiNetExitPanel->SetEnabled(1);
}

// Reimplements 0x41c080: HudUiNetExitPanel::Tick
int HudUiNetExitPanel::Tick() {
    g_HudUiNetExitPanel->Update(g_FrameDeltaTimeSec);
    return 0;
}

// Reimplements 0x41c0a0: HudUiNetExitPanel::DestroyGlobal
void HudUiNetExitPanel::DestroyGlobal() {
    HudUiNetExitPanel *const panel = g_HudUiNetExitPanel;
    if (panel != 0) {
        panel->Destructor();
        ::operator delete(panel);
        g_HudUiNetExitPanel = 0;
    }
}
