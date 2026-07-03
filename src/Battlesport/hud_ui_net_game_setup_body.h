#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/hud_ui_net_game_setup.h"

#include "Battlesport/cz_recoil_frame.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/recoil_app.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"

#include <new>
#include <stdio.h>
#include <string.h>

namespace {
/**
 * Original helper evidence: no standalone retail function; repeated inlined
 * min/max clamp sequence in 0x419aa0 and shared world-button callers
 * 0x41a820/0x41a9c0 immediately before "%d" formatting.
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Clamp integer setup values to the active input bounds.
 */
inline int ClampInt(
    int value,
    int minValue,
    int maxValue
) {
    if (value < minValue) {
        value = minValue;
    }
    if (value > maxValue) {
        value = maxValue;
    }
    return value;
}

/**
 * Original helper evidence: no standalone retail function; repeated store to
 * modeOrEnabled followed by the ftable slot 0x78 RefreshState dispatch in
 * 0x419aa0 and world-button side-effect callers 0x41a820/0x41a9c0.
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Store the enabled flag and refresh the ZRD widget state.
 */
inline void SetZrdWidgetEnabled(
    HudUiZrdWidget *widget,
    int enabled
) {
    widget->modeOrEnabled = enabled;
    widget->RefreshState();
}

/**
 * Original helper evidence: no standalone retail function; repeated
 * constructor-lowered pattern in 0x419aa0 for time, kills, and max players:
 * min/max stores, clamped value, sprintf("%d"), then
 * HudUiNumericTextInput::Update.
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Initialize clamped integer text input bounds and visible text.
 */
inline void InitClampedInput(
    HudUiClampedIntTextInput *input,
    int minValue,
    int maxValue,
    int value
) {
    input->minValue = minValue;
    input->maxValue = maxValue;

    char valueText[20];
    sprintf(
        valueText,
        "%d",
        ClampInt(value, minValue, maxValue)
    );
    input->Update(valueText);
}

/**
 * Original helper evidence: no standalone retail function; repeated
 * constructor-local targetInput and stepDelta stores in 0x419aa0 for the
 * increment/decrement time, kills, and max players buttons.
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Bind a step button to its target clamped integer input.
 */
inline void ConfigureStepButton(
    HudUiClampedIntStepButton *button,
    HudUiClampedIntTextInput *targetInput,
    int stepDelta
) {
    button->targetInput = targetInput;
    button->stepDelta = stepDelta;
}

/**
 * Original helper evidence: no standalone retail function; repeated indirect
 * ftable slot 0x60 visibility dispatch in 0x419aa0 and world-button callers
 * 0x41a820/0x41a9c0.
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Dispatch a widget visibility change through its installed table.
 */
inline void SetWidgetVisible(
    HudUiWidget *widget,
    int visible
) {
    ((HudUiElement *)(widget))->SetVisible(visible);
}

} // namespace

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this concrete member
 * widget table.
 * Purpose: construct the launch button through its ZRD widget base.
 */
HudUiNetGameSetupPanel_LaunchButton::HudUiNetGameSetupPanel_LaunchButton()
    : HudUiZrdWidget() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this concrete member
 * widget table.
 * Purpose: construct the cancel button through its ZRD widget base.
 */
HudUiNetGameSetupPanel_CancelButton::HudUiNetGameSetupPanel_CancelButton()
    : HudUiZrdWidget() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this concrete member
 * widget table.
 * Purpose: construct the next-world button through its ZRD widget base.
 */
HudUiNetGameSetupPanel_NextWorldButton::HudUiNetGameSetupPanel_NextWorldButton()
    : HudUiZrdWidget() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this concrete member
 * widget table.
 * Purpose: construct the previous-world button through its ZRD widget base.
 */
HudUiNetGameSetupPanel_PrevWorldButton::HudUiNetGameSetupPanel_PrevWorldButton()
    : HudUiZrdWidget() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing the world selector
 * member table at panel offset 0xaf5c.
 * Purpose: construct the world selector through its cycle-selector base.
 */
HudUiNetGameSetupPanel_WorldSelector::HudUiNetGameSetupPanel_WorldSelector()
    : HudUiCycleSelectorWidget() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing the time-limit input
 * member table at panel offset 0xb3fc.
 * Purpose: construct the time-limit input with the four-digit clamp buffer.
 */
HudUiNetGameSetupPanel_TimeLimitInput::HudUiNetGameSetupPanel_TimeLimitInput()
    : HudUiClampedIntTextInput(4) {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing the kills input member
 * table at panel offset 0xba20.
 * Purpose: construct the kills input with the two-digit clamp buffer.
 */
HudUiNetGameSetupPanel_KillsInput::HudUiNetGameSetupPanel_KillsInput()
    : HudUiClampedIntTextInput(2) {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing the max-players input
 * member table at panel offset 0xc044.
 * Purpose: construct the max-players input with the two-digit clamp buffer.
 */
HudUiNetGameSetupPanel_MaxPlayersInput::HudUiNetGameSetupPanel_MaxPlayersInput()
    : HudUiClampedIntTextInput(2) {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this increment button
 * table.
 * Purpose: construct the time-limit increment button through its step-button base.
 */
HudUiNetGameSetupPanel_IncTimeLimitButton::HudUiNetGameSetupPanel_IncTimeLimitButton()
    : HudUiClampedIntStepButton() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this decrement button
 * table.
 * Purpose: construct the time-limit decrement button through its step-button base.
 */
HudUiNetGameSetupPanel_DecTimeLimitButton::HudUiNetGameSetupPanel_DecTimeLimitButton()
    : HudUiClampedIntStepButton() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this increment button
 * table.
 * Purpose: construct the kills increment button through its step-button base.
 */
HudUiNetGameSetupPanel_IncKillsButton::HudUiNetGameSetupPanel_IncKillsButton()
    : HudUiClampedIntStepButton() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this decrement button
 * table.
 * Purpose: construct the kills decrement button through its step-button base.
 */
HudUiNetGameSetupPanel_DecKillsButton::HudUiNetGameSetupPanel_DecKillsButton()
    : HudUiClampedIntStepButton() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this increment button
 * table.
 * Purpose: construct the max-players increment button through its step-button base.
 */
HudUiNetGameSetupPanel_IncMaxPlayersButton::HudUiNetGameSetupPanel_IncMaxPlayersButton()
    : HudUiClampedIntStepButton() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing this decrement button
 * table.
 * Purpose: construct the max-players decrement button through its step-button base.
 */
HudUiNetGameSetupPanel_DecMaxPlayersButton::HudUiNetGameSetupPanel_DecMaxPlayersButton()
    : HudUiClampedIntStepButton() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing the allow-maps toggle
 * table at panel offset 0xc668.
 * Purpose: construct the allow-maps toggle through its check-toggle base.
 */
HudUiNetGameSetupPanel_AllowMapsToggle::HudUiNetGameSetupPanel_AllowMapsToggle()
    : HudUiCheckToggleWidget() {
}

/**
 * Original helper evidence: no standalone retail function; observed in caller
 * 0x419aa0 as compiler-emitted construction installing the name-tags toggle
 * table at panel offset 0xc7cc.
 * Purpose: construct the name-tags toggle through its check-toggle base.
 */
HudUiNetGameSetupPanel_NameTagsToggle::HudUiNetGameSetupPanel_NameTagsToggle()
    : HudUiCheckToggleWidget() {
}

/**
 * Reimplements 0x419aa0: HudUiNetGameSetupPanel::Constructor
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Initialize the network game setup panel controls and default session options.
 */
HudUiNetGameSetupPanel::HudUiNetGameSetupPanel(
    int reconfigureExistingSessionValue
) : HudUiBackground(),
    playButton(),
    cancelButton(),
    gameNameInput(),
    worldSelector(),
    nextWorldButton(),
    prevWorldButton(),
    timeLimitInput(),
    incTimeLimitButton(),
    decTimeLimitButton(),
    killsInput(),
    incKillsButton(),
    decKillsButton(),
    maxPlayersInput(),
    incMaxPlayersButton(),
    decMaxPlayersButton(),
    allowMapsToggle(),
    nameTagsToggle(),
    killsSwitch(),
    lapsSwitch() {
    gameNameInput.AllocTextBuffer(21);
    gameNameInput.Update("");
    gameNameInput.SetInputActive(0);
    incTimeLimitButton.targetInput = 0;
    incTimeLimitButton.stepDelta = 1;
    decTimeLimitButton.targetInput = 0;
    decTimeLimitButton.stepDelta = 1;
    incKillsButton.targetInput = 0;
    incKillsButton.stepDelta = 1;
    decKillsButton.targetInput = 0;
    decKillsButton.stepDelta = 1;
    incMaxPlayersButton.targetInput = 0;
    incMaxPlayersButton.stepDelta = 1;
    decMaxPlayersButton.targetInput = 0;
    decMaxPlayersButton.stepDelta = 1;

    reconfigureExistingSession = reconfigureExistingSessionValue;

    zReader::Node *const loadedSection =
        HudUiBackground::LoadFromZrd(
            "dialog.zrd",
            "MP_NEW_GAME",
            0
        );
    if (loadedSection != 0) {
        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&killsSwitch),
            "KILLS_SWITCH"
        );
        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&lapsSwitch),
            "LAPS_SWITCH"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &playButton,
            "PLAY"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &cancelButton,
            "CANCEL"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &gameNameInput,
            "GAME_NAME"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &worldSelector,
            "WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &nextWorldButton,
            "INC_WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &prevWorldButton,
            "DEC_WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &timeLimitInput,
            "TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incTimeLimitButton,
            "INC_TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decTimeLimitButton,
            "DEC_TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &killsInput,
            "KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incKillsButton,
            "INC_KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decKillsButton,
            "DEC_KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &maxPlayersInput,
            "MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incMaxPlayersButton,
            "INC_MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decMaxPlayersButton,
            "DEC_MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &allowMapsToggle,
            "ALLOW_MAPS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &nameTagsToggle,
            "NAME_TAGS"
        );
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    SetWidgetVisible(
        &killsSwitch,
        1
    );
    SetWidgetVisible(
        &lapsSwitch,
        0
    );
    worldSelector.SetIndexClamped(0);
    currentFocusWidget = 0;

    char playerNameText[24];
    sprintf(
        playerNameText,
        "%.21s",
        zOpt_GetPlayerName()
    );
    gameNameInput.Update(playerNameText);
    gameNameInput.AllocTextBuffer(22);

    const int enabledForNewSession = reconfigureExistingSession == 0 ? 1 : 0;
    SetZrdWidgetEnabled(
        &gameNameInput,
        enabledForNewSession
    );

    InitClampedInput(
        &timeLimitInput,
        5,
        360,
        15
    );
    ConfigureStepButton(
        &incTimeLimitButton,
        &timeLimitInput,
        1
    );
    ConfigureStepButton(
        &decTimeLimitButton,
        &timeLimitInput,
        -1
    );

    InitClampedInput(
        &killsInput,
        1,
        99,
        10
    );
    ConfigureStepButton(
        &incKillsButton,
        &killsInput,
        1
    );
    ConfigureStepButton(
        &decKillsButton,
        &killsInput,
        -1
    );

    if (zOpt::GetNetworkModemEnabled() != 0) {
        SetZrdWidgetEnabled(
            &maxPlayersInput,
            0
        );
        SetZrdWidgetEnabled(
            &incMaxPlayersButton,
            0
        );
        SetZrdWidgetEnabled(
            &decMaxPlayersButton,
            0
        );
    } else {
        InitClampedInput(
            &maxPlayersInput,
            2,
            8,
            8
        );
        SetZrdWidgetEnabled(
            &maxPlayersInput,
            enabledForNewSession
        );
        ConfigureStepButton(
            &incMaxPlayersButton,
            &maxPlayersInput,
            1
        );
        SetZrdWidgetEnabled(
            &incMaxPlayersButton,
            enabledForNewSession
        );
        ConfigureStepButton(
            &decMaxPlayersButton,
            &maxPlayersInput,
            -1
        );
        SetZrdWidgetEnabled(
            &decMaxPlayersButton,
            enabledForNewSession
        );
    }

    allowMapsToggle.SetChecked(1);
    nameTagsToggle.SetChecked(0);
    SetChildFlags(0);
}

/**
 * Reimplements 0x41a400: HudUiNetGameSetupPanel::Destructor
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Tear down the panel-owned controls before destroying the background base.
 */
void HudUiNetGameSetupPanel::Destructor() {
    lapsSwitch.DestructorCore();
    killsSwitch.DestructorCore();
    nameTagsToggle.DestructorCore();
    allowMapsToggle.DestructorCore();
    decMaxPlayersButton.DestructorCore();
    incMaxPlayersButton.DestructorCore();
    maxPlayersInput.Destructor();
    decKillsButton.DestructorCore();
    incKillsButton.DestructorCore();
    killsInput.Destructor();
    decTimeLimitButton.DestructorCore();
    incTimeLimitButton.DestructorCore();
    timeLimitInput.Destructor();
    prevWorldButton.DestructorCore();
    nextWorldButton.DestructorCore();
    worldSelector.DestructorCore();
    gameNameInput.Destructor();
    cancelButton.DestructorCore();
    playButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Original helper evidence: no standalone retail function; observed in
 * the HudUiNetGameSetupOverlayOwner panel delete path.
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Run the panel destructor and optionally free the panel storage.
 */
HudUiBackground * HudUiNetGameSetupPanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Reimplements 0x41a160: HudUiNetGameSetupPanel_CancelButton::OnActivate
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Leave the network setup state when the cancel button is activated.
 */
void HudUiNetGameSetupPanel_CancelButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x41a5b0: HudUiNetGameSetupPanel_LaunchButton::OnActivate
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Commit setup values and start or reconfigure the network game session.
 */
void HudUiNetGameSetupPanel_LaunchButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    int statusFlags = 0;
    HudUiZrdWidget::OnActivate();

    if (ownerPanel->allowMapsToggle.checked != 0) {
        statusFlags = 1;
    }
    if (ownerPanel->nameTagsToggle.checked != 0) {
        statusFlags |= 2;
    }

    HudUiClampedIntTextInput *const killsInput = &ownerPanel->killsInput;
    if (zOpt::GetNetworkModemEnabled() == 0 && ownerPanel->reconfigureExistingSession == 0) {
        zNetworkSessionDescStatusFields statusFields;
        statusFields.eventCode = ownerPanel->worldSelector.selectedIndex + 1;
        statusFields.statusFlags = statusFlags;
        statusFields.valueOrTime = ownerPanel->timeLimitInput.CommitAndGetValue();
        statusFields.auxParam = killsInput->CommitAndGetValue();
        statusFields.maxPlayers = ownerPanel->maxPlayersInput.CommitAndGetValue();
        strcpy(
            statusFields.sessionNameBuf,
            ownerPanel->gameNameInput.GetBuffer()
        );

        if (zNetwork_DPlay::CreateSessionFromStatusFields(&statusFields) != 0) {
            zOpt::SetNetworkEnabled(1);
            zNetwork_DPlay::CreateLocalPlayerRecordAndRegister(zOpt_GetPlayerName());
        }
    } else {
        const int auxParam = killsInput->CommitAndGetValue();
        const int valueOrTime = ownerPanel->timeLimitInput.CommitAndGetValue();
        GameNet::SendPkt14_HudTimerAndFlagsSync(
            ownerPanel->worldSelector.selectedIndex + 1,
            statusFlags,
            valueOrTime,
            auxParam
        );
        if (zNetwork::IsHost() != 0) {
            GameNet::HostUpdateSessionDescStatusFields(
                ownerPanel->worldSelector.selectedIndex + 1,
                killsInput->CommitAndGetValue(),
                ownerPanel->timeLimitInput.CommitAndGetValue(),
                statusFlags
            );
        }

        GameNet::UnregisterGameplayPacketHandlers();
        GameNet::ResetRemotePlayersAndSpawnLists();
    }

    g_RecoilApp.m_skipIntroFmv = 1;
    GameNet::SetStatusBitsFromFlags(statusFlags);

    const int goalValue = killsInput->CommitAndGetValue();
    const int timeLimitMinutes = ownerPanel->timeLimitInput.CommitAndGetValue();
    union TimerSecondsRaw {
        float seconds;
        int raw;
    } timerSeconds = {(float)(timeLimitMinutes) * 60.0f};
    g_HudSensorTracker.SetRuntimeTimerSecAndGoalValue(
        timerSeconds.raw,
        goalValue
    );

    CZRecoilFrame *const mainWnd = (CZRecoilFrame *)((unsigned int)(g_RecoilApp.GetMainWnd()));
    g_HudSensorTracker.InitMissionIdAndFlags(
        ownerPanel->worldSelector.selectedIndex + 7,
        mainWnd->m_useArchiveBanks
    );
    g_RecoilApp.QueueExitCurrentState(0);
}

/**
 * Reimplements 0x41a820: HudUiNetGameSetupPanel_NextWorldButton::OnActivate
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Advance the selected world and apply the related setup side effects.
 */
void HudUiNetGameSetupPanel_NextWorldButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    ownerPanel->worldSelector.SetIndexClamped(ownerPanel->worldSelector.selectedIndex + 1);

    HudUiClampedIntTextInput *killsInput;
    if (ownerPanel->worldSelector.selectedIndex == 2) {
        SetWidgetVisible(
            &ownerPanel->killsSwitch,
            0
        );
        SetWidgetVisible(
            &ownerPanel->lapsSwitch,
            1
        );

        killsInput = &ownerPanel->killsInput;
        if (killsInput->CommitAndGetValue() == 1) {
            char valueText[20];
            int clampedValue = 2;
            if (killsInput->minValue > clampedValue) {
                clampedValue = killsInput->minValue;
            }
            if (clampedValue > killsInput->maxValue) {
                clampedValue = killsInput->maxValue;
            }
            sprintf(
                valueText,
                "%d",
                clampedValue
            );
            killsInput->Update(valueText);
        }
        killsInput->minValue = 2;
        killsInput->maxValue = 99;

        SetZrdWidgetEnabled(
            &ownerPanel->incTimeLimitButton,
            0
        );
        SetZrdWidgetEnabled(
            &ownerPanel->decTimeLimitButton,
            0
        );
    } else {
        SetWidgetVisible(
            &ownerPanel->killsSwitch,
            1
        );
        SetWidgetVisible(
            &ownerPanel->lapsSwitch,
            0
        );
        killsInput = &ownerPanel->killsInput;
        killsInput->minValue = 1;
        killsInput->maxValue = 99;

        SetZrdWidgetEnabled(
            &ownerPanel->timeLimitInput,
            1
        );
        SetZrdWidgetEnabled(
            &ownerPanel->incTimeLimitButton,
            1
        );
        SetZrdWidgetEnabled(
            &ownerPanel->decTimeLimitButton,
            1
        );
    }

    killsInput->Invalidate();
    ownerPanel->incKillsButton.Invalidate();
    ownerPanel->decKillsButton.Invalidate();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x41a9c0: HudUiNetGameSetupPanel_PrevWorldButton::OnActivate
 * Source: D:\Proj\Battlesport\HudUiNetGameSetup.cpp
 * Purpose: Move to the previous world and apply the related setup side effects.
 */
void HudUiNetGameSetupPanel_PrevWorldButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    ownerPanel->worldSelector.SetIndexClamped(ownerPanel->worldSelector.selectedIndex - 1);

    HudUiClampedIntTextInput *killsInput;
    if (ownerPanel->worldSelector.selectedIndex == 2) {
        SetWidgetVisible(
            &ownerPanel->killsSwitch,
            0
        );
        SetWidgetVisible(
            &ownerPanel->lapsSwitch,
            1
        );

        killsInput = &ownerPanel->killsInput;
        if (killsInput->CommitAndGetValue() == 1) {
            char valueText[20];
            int clampedValue = 2;
            if (killsInput->minValue > clampedValue) {
                clampedValue = killsInput->minValue;
            }
            if (clampedValue > killsInput->maxValue) {
                clampedValue = killsInput->maxValue;
            }
            sprintf(
                valueText,
                "%d",
                clampedValue
            );
            killsInput->Update(valueText);
        }
        killsInput->minValue = 2;
        killsInput->maxValue = 99;

        SetZrdWidgetEnabled(
            &ownerPanel->incTimeLimitButton,
            0
        );
        SetZrdWidgetEnabled(
            &ownerPanel->decTimeLimitButton,
            0
        );
    } else {
        SetWidgetVisible(
            &ownerPanel->killsSwitch,
            1
        );
        SetWidgetVisible(
            &ownerPanel->lapsSwitch,
            0
        );
        killsInput = &ownerPanel->killsInput;
        killsInput->minValue = 1;
        killsInput->maxValue = 99;

        SetZrdWidgetEnabled(
            &ownerPanel->timeLimitInput,
            1
        );
        SetZrdWidgetEnabled(
            &ownerPanel->incTimeLimitButton,
            1
        );
        SetZrdWidgetEnabled(
            &ownerPanel->decTimeLimitButton,
            1
        );
    }

    killsInput->Invalidate();
    ownerPanel->incKillsButton.Invalidate();
    ownerPanel->decKillsButton.Invalidate();
    HudUiZrdWidget::OnActivate();
}
