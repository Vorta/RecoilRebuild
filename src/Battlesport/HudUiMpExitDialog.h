#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zVideo.h"

/**
 * Original inline constructor evidence: BN 0x419740 installs the
 * HudUiMpExitDialog_NewGameButton dispatch table after the HudUiZrdWidget base
 * constructor, with no standalone retail constructor body.
 * Purpose: let VC5 emit the button subclass dispatch identity for the MpExit
 * dialog owner.
 */
struct HudUiMpExitDialog_NewGameButton : HudUiZrdWidget {
    /**
     * Original inline constructor evidence: no standalone retail function;
     * BN 0x419740 installs this subclass dispatch identity after the
     * HudUiZrdWidget base constructor.
     * Purpose: construct the multiplayer-exit new-game button with its
     * recovered C++ dispatch identity.
     */
    HudUiMpExitDialog_NewGameButton() : HudUiZrdWidget() {
    }

    void OnActivate();
};

/**
 * Original inline constructor evidence: BN 0x419740 installs the
 * HudUiMpExitDialog_ExitButton dispatch table after the HudUiZrdWidget base
 * constructor, with no standalone retail constructor body.
 * Purpose: let VC5 emit the button subclass dispatch identity for the MpExit
 * dialog owner.
 */
struct HudUiMpExitDialog_ExitButton : HudUiZrdWidget {
    /**
     * Original inline constructor evidence: no standalone retail function;
     * BN 0x419740 installs this subclass dispatch identity after the
     * HudUiZrdWidget base constructor.
     * Purpose: construct the multiplayer-exit leave button with its recovered
     * C++ dispatch identity.
     */
    HudUiMpExitDialog_ExitButton() : HudUiZrdWidget() {
    }

    void OnActivate();
};

/**
 * Ownership/evidence: BN 0x419740 constructs HudUiBackground, then the two
 * embedded HudUiZrdWidget buttons, and finally installs the HudUiMpExitDialog
 * dispatch identity before storing the singleton pointer.
 */
struct HudUiMpExitDialog : HudUiBackground {
    HudUiMpExitDialog_NewGameButton m_mpNewGameButton;
    HudUiMpExitDialog_ExitButton m_mpExitButton;
    zVidImagePartial *m_capturedBackgroundImage;
    float m_fadeElapsedSeconds;
    int m_mpNewGameButtonMode;

    /**
     * Original inline constructor evidence: no standalone retail function;
     * BN 0x419740 constructs the HudUiBackground base and embedded button
     * members before installing the HudUiMpExitDialog dispatch identity.
     * Purpose: construct the multiplayer-exit dialog owner with recovered
     * class and member dispatch identities.
     */
    HudUiMpExitDialog() : HudUiBackground(),
                          m_mpNewGameButton(),
                          m_mpExitButton() {
    }

    void UnloadLayout();
    void Update(float deltaSeconds);
    void LoadLayout();
    virtual HudUiBackground * ScalarDeletingDestructor(unsigned int flags);
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
