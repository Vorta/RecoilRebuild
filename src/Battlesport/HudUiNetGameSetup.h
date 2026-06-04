#pragma once

#include "GameZRecoil/zHud/zhud_ui.h"

#include <stddef.h>

struct HudUiNetGameSetupPanel;

struct HudUiNetGameSetupPanel_FTable {
    unsigned int slots[3];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_FTable) == 0x0c);

struct HudUiNetGameSetupPanel_LaunchButton : HudUiZrdWidget {
    void OnActivate();
};
RECOIL_STATIC_ASSERT(sizeof(HudUiNetGameSetupPanel_LaunchButton) == 0x14c);

struct HudUiNetGameSetupPanel_CancelButton : HudUiZrdWidget {
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

struct HudUiNetGameSetupPanel : HudUiBackground {
    HudUiNumericTextInput *currentFocusWidget;
    HudUiNetGameSetupPanel_LaunchButton playButton;
    HudUiNetGameSetupPanel_CancelButton cancelButton;
    HudUiNetGameSetupTextInput gameNameInput;
    HudUiCycleSelectorWidget worldSelector;
    HudUiNetGameSetupPanel_NextWorldButton nextWorldButton;
    HudUiNetGameSetupPanel_PrevWorldButton prevWorldButton;
    HudUiClampedIntTextInput timeLimitInput;
    HudUiClampedIntStepButton incTimeLimitButton;
    HudUiClampedIntStepButton decTimeLimitButton;
    HudUiClampedIntTextInput killsInput;
    HudUiClampedIntStepButton incKillsButton;
    HudUiClampedIntStepButton decKillsButton;
    HudUiClampedIntTextInput maxPlayersInput;
    HudUiClampedIntStepButton incMaxPlayersButton;
    HudUiClampedIntStepButton decMaxPlayersButton;
    HudUiCheckToggleWidget allowMapsToggle;
    HudUiCheckToggleWidget nameTagsToggle;
    HudUiWidget killsSwitch;
    HudUiWidget lapsSwitch;
    int reconfigureExistingSession;

    HudUiNetGameSetupPanel * Constructor(int reconfigureExistingSessionValue);
    void Destructor();
    HudUiNetGameSetupPanel * ScalarDeletingDestructor(
        unsigned int flags
    );
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

extern const HudUiNetGameSetupPanel_FTable g_HudUiNetGameSetupPanel_FTable;
extern const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_PlayButton_FTable;
extern const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_CancelButton_FTable;
extern const HudUiNumericTextInput_Base_FTable g_HudUiNetGameSetupPanel_GameNameInput_FTable;
extern const HudUiCycleSelectorWidget_FTable g_HudUiNetGameSetupPanel_WorldSelector_FTable;
extern const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_NextWorldButton_FTable;
extern const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_PrevWorldButton_FTable;
extern const HudUiClampedIntTextInput_FTable g_HudUiNetGameSetupPanel_ClampedInput_FTable;
extern const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_StepButton_FTable;
extern const HudUiCheckToggleWidget_FTable g_HudUiNetGameSetupPanel_CheckToggle_FTable;
