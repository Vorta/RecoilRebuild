#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"

#include <stddef.h>

struct HudUiNetExitPanel;
struct HudUiNetExitPanel_ExitButton;
struct HudUiNetExitPanel_ResumeWidget;

struct HudUiNetExitPanel_ExitButton : HudUiZrdWidget {
    int previewInputCaptureActive;

    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel_ExitButton) == 0x150);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetExitPanel_ExitButton,
        previewInputCaptureActive
    ) == 0x14c
);

struct HudUiNetExitPanel_ResumeWidget : HudUiZrdWidget {
    int previewInputCaptureActive;

    void OnActivate();
    void OnShowPreview();
    void OnHidePreview();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel_ResumeWidget) == 0x150);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetExitPanel_ResumeWidget,
        previewInputCaptureActive
    ) == 0x14c
);

struct HudUiNetExitPanel : HudUiBackground {
    HudUiNetExitPanel_ResumeWidget resumeWidget;
    HudUiNetExitPanel_ExitButton exitWidget;

    HudUiNetExitPanel * Constructor();
    virtual void SetEnabled(int enabled);
    static HudUiNetExitPanel *CreateGlobal();
    static void Show();
    static int Tick();
    static void DestroyGlobal();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel) == 0xabec);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetExitPanel,
        resumeWidget
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetExitPanel,
        exitWidget
    ) == 0xaa9c
);

extern "C" {
extern HudUiNetExitPanel *g_HudUiNetExitPanel;
extern HudUiElement *g_HudUiNetExitPanel_SavedInputFocus;
}
