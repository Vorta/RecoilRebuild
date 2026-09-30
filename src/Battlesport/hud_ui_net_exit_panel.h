#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"

#include <stddef.h>

struct HudUiNetExitPanel;
struct HudUiNetExitPanel_ExitButton;
struct CHudUiNetExitPanelResumeWidget;

struct HudUiNetExitPanel_ExitButton : HudUiZrdWidget {
    int previewInputCaptureActive;

    HudUiNetExitPanel_ExitButton()
        : previewInputCaptureActive(0)
    {
    }
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel_ExitButton) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(HudUiNetExitPanel_ExitButton, previewInputCaptureActive) == 0x14c);

struct CHudUiNetExitPanelResumeWidget : HudUiZrdWidget {
    int previewInputCaptureActive;

    CHudUiNetExitPanelResumeWidget()
        : previewInputCaptureActive(0)
    {
    }
    void OnActivate();
    virtual void ShowPreview();
    virtual void HidePreview();
};
RECOIL_STATIC_ASSERT(sizeof(CHudUiNetExitPanelResumeWidget) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(CHudUiNetExitPanelResumeWidget, previewInputCaptureActive) == 0x14c);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud-ui-net-exit-panel.type
 * @recoil-artifact emits .text recoil:function:0x41beb0: HudUiNetExitPanel::~HudUiNetExitPanel (compiler-emitted implicit destructor).
 * Retail 0x41beb0 has no derived vptr store, which VC5 emits only for the
 * implicit destructor.
 * Purpose: define the network-exit panel whose ordinary virtual lifetime
 * destroys its two embedded widgets before the background base.
 */
struct HudUiNetExitPanel : HudUiBackground {
    CHudUiNetExitPanelResumeWidget resumeWidget;
    HudUiNetExitPanel_ExitButton exitWidget;

    HudUiNetExitPanel();
    virtual void SetEnabled(int enabled);
    static HudUiNetExitPanel* __cdecl CreateGlobal();
    static void __cdecl Show();
    static int __cdecl Tick();
    static void __cdecl DestroyGlobal();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel) == 0xabec);
RECOIL_STATIC_ASSERT(offsetof(HudUiNetExitPanel, resumeWidget) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiNetExitPanel, exitWidget) == 0xaa9c);

extern "C" {
extern HudUiNetExitPanel* g_HudUiNetExitPanel;
extern HudUiElement* g_HudUiNetExitPanel_SavedInputFocus;
}
