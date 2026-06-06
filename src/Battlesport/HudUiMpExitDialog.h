#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zVideo.h"

struct HudUiMpExitDialog_NewGameButton : HudUiZrdWidget {
    void OnActivate();
};

struct HudUiMpExitDialog_ExitButton : HudUiZrdWidget {
    void OnActivate();
};

struct HudUiMpExitDialog : HudUiBackground {
    HudUiMpExitDialog_NewGameButton m_mpNewGameButton;
    HudUiMpExitDialog_ExitButton m_mpExitButton;
    zVidImagePartial *m_capturedBackgroundImage;
    float m_fadeElapsedSeconds;
    int m_mpNewGameButtonMode;

    void UnloadLayout();
    void Update(float deltaSeconds);
    void LoadLayout();
    void Destructor();
};

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
