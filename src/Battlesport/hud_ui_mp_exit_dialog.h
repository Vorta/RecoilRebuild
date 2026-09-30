#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zVideo/zvid.h"

/**
 * Original inline constructor evidence: BN 0x419740 installs the
 * CHudUiMpExitDialogNewGameButton dispatch table after the HudUiZrdWidget base
 * constructor, with no standalone retail constructor body.
 * Purpose: let VC5 emit the button subclass dispatch identity for the MpExit
 * dialog owner.
 */
struct CHudUiMpExitDialogNewGameButton : HudUiZrdWidget {
    /**
     * Original inline constructor evidence: no standalone retail function;
     * BN 0x419740 installs this subclass dispatch identity after the
     * HudUiZrdWidget base constructor.
     * Purpose: construct the multiplayer-exit new-game button with its
     * recovered C++ dispatch identity.
     */
    CHudUiMpExitDialogNewGameButton()
        : HudUiZrdWidget()
    {
    }

    void OnActivate();
};

/**
 * Original inline constructor evidence: BN 0x419740 installs the
 * CHudUiMpExitDialogExitButton dispatch table after the HudUiZrdWidget base
 * constructor, with no standalone retail constructor body.
 * Purpose: let VC5 emit the button subclass dispatch identity for the MpExit
 * dialog owner.
 */
struct CHudUiMpExitDialogExitButton : HudUiZrdWidget {
    /**
     * Original inline constructor evidence: no standalone retail function;
     * BN 0x419740 installs this subclass dispatch identity after the
     * HudUiZrdWidget base constructor.
     * Purpose: construct the multiplayer-exit leave button with its recovered
     * C++ dispatch identity.
     */
    CHudUiMpExitDialogExitButton()
        : HudUiZrdWidget()
    {
    }

    void OnActivate();
};

/**
 * @recoil-anchor recoil:anchor:battlesport.hud-ui-mp-exit-dialog.type
 * @recoil-artifact emits .text recoil:function:0x419870: HudUiMpExitDialog::~HudUiMpExitDialog (compiler-emitted implicit destructor).
 * Ownership/evidence: BN 0x419740 constructs HudUiBackground, then the two
 * embedded HudUiZrdWidget buttons, and finally installs the HudUiMpExitDialog
 * dispatch identity before storing the singleton pointer. Retail 0x419870 has
 * no derived vptr store, which VC5 emits only for the implicit destructor.
 * Purpose: define the multiplayer-exit dialog whose ordinary virtual lifetime
 * destroys the exit and new-game child widgets before the background base.
 */
struct HudUiMpExitDialog : HudUiBackground {
    CHudUiMpExitDialogNewGameButton m_mpNewGameButton;
    CHudUiMpExitDialogExitButton m_mpExitButton;
    zVidImagePartial* m_capturedBackgroundImage;
    float m_fadeElapsedSeconds;
    int m_mpNewGameButtonMode;

    /**
     * Original inline constructor evidence: no standalone retail function;
     * BN 0x419740 constructs the HudUiBackground base and embedded button
     * members before installing the HudUiMpExitDialog dispatch identity.
     * Purpose: construct the multiplayer-exit dialog owner with recovered
     * class and member dispatch identities.
     */
    HudUiMpExitDialog()
        : HudUiBackground()
        , m_mpNewGameButton()
        , m_mpExitButton()
    {
    }

    void UnloadLayout();
    virtual void Update(float deltaSeconds);
    void LoadLayout();
};

extern HudUiMpExitDialog* g_HudUiMpExitDialog;

RECOIL_STATIC_ASSERT(offsetof(HudUiMpExitDialog, m_mpNewGameButton) == 0xa94c);
RECOIL_STATIC_ASSERT(offsetof(HudUiMpExitDialog, m_mpExitButton) == 0xaa98);
RECOIL_STATIC_ASSERT(offsetof(HudUiMpExitDialog, m_capturedBackgroundImage) == 0xabe4);
RECOIL_STATIC_ASSERT(offsetof(HudUiMpExitDialog, m_fadeElapsedSeconds) == 0xabe8);
RECOIL_STATIC_ASSERT(offsetof(HudUiMpExitDialog, m_mpNewGameButtonMode) == 0xabec);
RECOIL_STATIC_ASSERT(sizeof(HudUiMpExitDialog) == 0xabf0);
