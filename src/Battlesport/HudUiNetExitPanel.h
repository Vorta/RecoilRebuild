#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "recoil/recoil_callconv.h"

#include "recoil/recoil_types.h"
#include <stddef.h>

struct HudUiNetExitPanel;
struct HudUiNetExitPanel_ExitButton;
struct HudUiNetExitPanel_ResumeWidget;

struct HudUiNetExitPanel_FTable {
    void( *updateAll)(
        HudUiNetExitPanel *self,
        float deltaSeconds
    );
    int( *setEnabled)(
        HudUiNetExitPanel *self,
        int enabled
    );
    HudUiNetExitPanel *( *scalarDeletingDtor)(
        HudUiNetExitPanel *self,
        unsigned int flags
    );
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetExitPanel_FTable) == 0x0c);

struct HudUiNetExitPanel_ExitButton {
    HudUiZrdWidget base;
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

struct HudUiNetExitPanel_ResumeWidget {
    HudUiZrdWidget base;
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

struct HudUiNetExitPanel {
    HudUiBackground base;
    HudUiNetExitPanel_ResumeWidget resumeWidget;
    HudUiNetExitPanel_ExitButton exitWidget;

    HudUiNetExitPanel * Constructor();
    void Destructor();
    void Update(float deltaSeconds);
    int SetEnabled(int enabled);
    HudUiNetExitPanel * ScalarDeletingDestructor(unsigned int flags);
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
