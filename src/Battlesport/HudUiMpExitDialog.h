#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zVideo.h"

struct HudUiMpExitDialog_NewGameButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMpExitDialog_ExitButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMpExitDialog_Vtbl {
    unsigned int slots[3];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiMpExitDialog_Vtbl) == 0x0c);

struct HudUiMpExitDialog {
    HudUiBackground base;
    HudUiMpExitDialog_NewGameButton m_mpNewGameButton;
    HudUiMpExitDialog_ExitButton m_mpExitButton;
    zVidImagePartial *m_capturedBackgroundImage;
    float m_fadeElapsedSeconds;
    int m_mpNewGameButtonMode;

    RECOIL_NOINLINE void RECOIL_THISCALL UnloadLayout();
    RECOIL_NOINLINE void RECOIL_THISCALL Update(float deltaSeconds);
    RECOIL_NOINLINE void RECOIL_THISCALL LoadLayout();
    RECOIL_NOINLINE void RECOIL_THISCALL Destructor();
    HudUiMpExitDialog *RECOIL_THISCALL ScalarDeletingDestructorThunk(unsigned int flags);
};

extern const HudUiWidget_FTable g_HudUiZrdWidget_MpExitDialog_NewGameButton_Vtbl;
extern const HudUiWidget_FTable g_HudUiZrdWidget_MpExitDialog_ExitButton_Vtbl;
extern const HudUiMpExitDialog_Vtbl g_HudUiMpExitDialog_Vtbl;
extern HudUiMpExitDialog *g_HudUiMpExitDialog;

RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMpExitDialog,
        m_mpNewGameButton
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMpExitDialog,
        m_mpExitButton
    ) == 0xaa98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMpExitDialog,
        m_capturedBackgroundImage
    ) == 0xabe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMpExitDialog,
        m_fadeElapsedSeconds
    ) == 0xabe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiMpExitDialog,
        m_mpNewGameButtonMode
    ) == 0xabec
);
RECOIL_STATIC_ASSERT(sizeof(HudUiMpExitDialog) == 0xabf0);
