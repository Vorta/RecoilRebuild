#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "recoil/recoil_callconv.h"

#include <stddef.h>
#include "recoil/recoil_types.h"

struct HudUiNetExitPanel;
struct HudUiNetExitPanel_ExitButton;
struct HudUiNetExitPanel_ResumeWidget;

struct HudUiNetExitPanel_FTable {
    void(RECOIL_THISCALL *updateAll)(HudUiNetExitPanel *self, float deltaSeconds);
    int(RECOIL_THISCALL *setEnabled)(HudUiNetExitPanel *self, int enabled);
    HudUiNetExitPanel *(RECOIL_THISCALL *scalarDeletingDtor)(HudUiNetExitPanel *self,
                                                             unsigned int flags);
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel_FTable) == 0x0c);

struct HudUiNetExitPanel_ExitButton {
    HudUiZrdWidget base;
    int previewInputCaptureActive;

    void RECOIL_THISCALL OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel_ExitButton) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(HudUiNetExitPanel_ExitButton, previewInputCaptureActive) == 0x14c);

struct HudUiNetExitPanel_ResumeWidget {
    HudUiZrdWidget base;
    int previewInputCaptureActive;

    void RECOIL_THISCALL OnActivate();
    void RECOIL_THISCALL OnShowPreview();
    void RECOIL_THISCALL OnHidePreview();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel_ResumeWidget) == 0x150);
RECOIL_STATIC_ASSERT(offsetof(HudUiNetExitPanel_ResumeWidget, previewInputCaptureActive) == 0x14c);

struct HudUiNetExitPanel {
    HudUiBackground base;
    HudUiNetExitPanel_ResumeWidget resumeWidget;
    HudUiNetExitPanel_ExitButton exitWidget;

    HudUiNetExitPanel *RECOIL_THISCALL Constructor();
    void RECOIL_THISCALL Destructor();
    void RECOIL_THISCALL Update(float deltaSeconds);
    int RECOIL_THISCALL SetEnabled(int enabled);
    HudUiNetExitPanel *RECOIL_THISCALL ScalarDeletingDestructor(unsigned int flags);
    RECOIL_NOINLINE static HudUiNetExitPanel *RECOIL_CDECL CreateGlobal();
    RECOIL_NOINLINE static void RECOIL_CDECL Show();
    RECOIL_NOINLINE static int RECOIL_CDECL Tick();
    RECOIL_NOINLINE static void RECOIL_CDECL DestroyGlobal();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel) == 0xabec);
RECOIL_STATIC_ASSERT(offsetof(HudUiNetExitPanel, resumeWidget) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiNetExitPanel, exitWidget) == 0xaa9c);

extern "C" {
extern HudUiNetExitPanel *g_HudUiNetExitPanel;
extern HudUiElement *g_HudUiNetExitPanel_SavedInputFocus;
}
