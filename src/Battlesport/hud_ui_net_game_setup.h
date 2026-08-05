#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"

#include <stddef.h>

struct HudUiNetGameSetupPanel;

struct HudUiNetGameSetupPanel_LaunchButton : HudUiZrdWidget {
    HudUiNetGameSetupPanel_LaunchButton();
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_LaunchButton) == 0x14c);

struct HudUiNetGameSetupPanel_CancelButton : HudUiZrdWidget {
    HudUiNetGameSetupPanel_CancelButton();
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_CancelButton) == 0x14c);

struct HudUiNetGameSetupPanel_NextWorldButton : HudUiZrdWidget {
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_NextWorldButton) == 0x14c);

struct HudUiNetGameSetupPanel_PrevWorldButton : HudUiZrdWidget {
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_PrevWorldButton) == 0x14c);

struct HudUiNetGameSetupPanel_WorldSelector : HudUiCycleSelectorWidget {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_WorldSelector) == 0x208);

struct HudUiNetGameSetupPanel_TimeLimitInput : HudUiClampedIntTextInput {
    HudUiNetGameSetupPanel_TimeLimitInput()
        : HudUiClampedIntTextInput(4) {
    }
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_TimeLimitInput) == 0x37c);

struct HudUiNetGameSetupPanel_KillsInput : HudUiClampedIntTextInput {
    HudUiNetGameSetupPanel_KillsInput()
        : HudUiClampedIntTextInput(2) {
    }
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_KillsInput) == 0x37c);

struct HudUiNetGameSetupPanel_MaxPlayersInput : HudUiClampedIntTextInput {
    HudUiNetGameSetupPanel_MaxPlayersInput()
        : HudUiClampedIntTextInput(2) {
    }
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_MaxPlayersInput) == 0x37c);

struct HudUiNetGameSetupPanel_IncTimeLimitButton : HudUiClampedIntStepButton {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_IncTimeLimitButton) == 0x154);

struct HudUiNetGameSetupPanel_DecTimeLimitButton : HudUiClampedIntStepButton {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_DecTimeLimitButton) == 0x154);

struct HudUiNetGameSetupPanel_IncKillsButton : HudUiClampedIntStepButton {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_IncKillsButton) == 0x154);

struct HudUiNetGameSetupPanel_DecKillsButton : HudUiClampedIntStepButton {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_DecKillsButton) == 0x154);

struct HudUiNetGameSetupPanel_IncMaxPlayersButton : HudUiClampedIntStepButton {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_IncMaxPlayersButton) == 0x154);

struct HudUiNetGameSetupPanel_DecMaxPlayersButton : HudUiClampedIntStepButton {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_DecMaxPlayersButton) == 0x154);

struct HudUiNetGameSetupPanel_AllowMapsToggle : HudUiCheckToggleWidget {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_AllowMapsToggle) == 0x164);

struct HudUiNetGameSetupPanel_NameTagsToggle : HudUiCheckToggleWidget {
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_NameTagsToggle) == 0x164);

struct HudUiNetGameSetupPanel : HudUiBackground {
    HudUiNumericTextInput *currentFocusWidget;
    HudUiNetGameSetupPanel_LaunchButton playButton;
    HudUiNetGameSetupPanel_CancelButton cancelButton;
    HudUiNetGameSetupTextInput gameNameInput;
    HudUiNetGameSetupPanel_WorldSelector worldSelector;
    HudUiNetGameSetupPanel_NextWorldButton nextWorldButton;
    HudUiNetGameSetupPanel_PrevWorldButton prevWorldButton;
    HudUiNetGameSetupPanel_TimeLimitInput timeLimitInput;
    HudUiNetGameSetupPanel_IncTimeLimitButton incTimeLimitButton;
    HudUiNetGameSetupPanel_DecTimeLimitButton decTimeLimitButton;
    HudUiNetGameSetupPanel_KillsInput killsInput;
    HudUiNetGameSetupPanel_IncKillsButton incKillsButton;
    HudUiNetGameSetupPanel_DecKillsButton decKillsButton;
    HudUiNetGameSetupPanel_MaxPlayersInput maxPlayersInput;
    HudUiNetGameSetupPanel_IncMaxPlayersButton incMaxPlayersButton;
    HudUiNetGameSetupPanel_DecMaxPlayersButton decMaxPlayersButton;
    HudUiNetGameSetupPanel_AllowMapsToggle allowMapsToggle;
    HudUiNetGameSetupPanel_NameTagsToggle nameTagsToggle;
    HudUiWidget killsSwitch;
    HudUiWidget lapsSwitch;
    int reconfigureExistingSession;

    HudUiNetGameSetupPanel(int reconfigureExistingSessionValue);
    ~HudUiNetGameSetupPanel();
    /**
     * Original inline constructor evidence: no standalone retail function;
     * local reconstructed callers need the previous constructor-shaped entry,
     * while 0x419aa0 is now the compiler-emitted C++ constructor body.
     * Purpose: construct the setup panel in caller-provided storage.
     */
    HudUiNetGameSetupPanel * Constructor(int reconfigureExistingSessionValue) {
        return new (this) HudUiNetGameSetupPanel(reconfigureExistingSessionValue);
    }
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel) == 0xcaac);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        currentFocusWidget
    ) == 0xa94c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        playButton
    ) == 0xa950
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        cancelButton
    ) == 0xaa9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        gameNameInput
    ) == 0xabe8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        worldSelector
    ) == 0xaf5c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        nextWorldButton
    ) == 0xb164
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        prevWorldButton
    ) == 0xb2b0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        timeLimitInput
    ) == 0xb3fc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        incTimeLimitButton
    ) == 0xb778
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        decTimeLimitButton
    ) == 0xb8cc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        killsInput
    ) == 0xba20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        incKillsButton
    ) == 0xbd9c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        decKillsButton
    ) == 0xbef0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        maxPlayersInput
    ) == 0xc044
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        incMaxPlayersButton
    ) == 0xc3c0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        decMaxPlayersButton
    ) == 0xc514
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        allowMapsToggle
    ) == 0xc668
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        nameTagsToggle
    ) == 0xc7cc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        killsSwitch
    ) == 0xc930
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        lapsSwitch
    ) == 0xc9ec
);
RECOIL_STATIC_ASSERT(
    offsetof(
        HudUiNetGameSetupPanel,
        reconfigureExistingSession
    ) == 0xcaa8
);
