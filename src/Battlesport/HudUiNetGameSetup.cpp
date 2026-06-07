#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/HudUiNetGameSetup.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/GameNet.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/RecoilApp.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"

#include <new>
#include <stdio.h>
#include <string.h>

namespace {
// Source-faithful helper recovered from address-backed callers in this source file.
int ClampInt(
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

// Source-faithful helper recovered from address-backed callers in this source file.
void SetZrdWidgetEnabled(
    HudUiZrdWidget *widget,
    int enabled
) {
    widget->modeOrEnabled = enabled;
    widget->RefreshState();
}

// Source-faithful helper recovered from address-backed callers in this source file.
void InitClampedInput(
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

// Source-faithful helper recovered from address-backed callers in this source file.
void ConfigureStepButton(
    HudUiClampedIntStepButton *button,
    HudUiClampedIntTextInput *targetInput,
    int stepDelta
) {
    button->targetInput = targetInput;
    button->stepDelta = stepDelta;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void SetWidgetVisible(
    HudUiWidget *widget,
    int visible
) {
    ((HudUiElement *)(widget))->SetVisible(visible);
}

// Source-faithful helper recovered from address-backed callers in this source file.
void ApplyWorldSelectionSideEffects(
    HudUiNetGameSetupPanel *panel,
    int selectedIndex
) {
    panel->worldSelector.SetIndexClamped(selectedIndex);

    if (panel->worldSelector.selectedIndex == 2) {
        SetWidgetVisible(
            &panel->killsSwitch,
            0
        );
        SetWidgetVisible(
            &panel->lapsSwitch,
            1
        );

        if (panel->killsInput.CommitAndGetValue() == 1) {
            panel->killsInput.Update("2");
        }
        panel->killsInput.minValue = 2;
        panel->killsInput.maxValue = 99;

        SetZrdWidgetEnabled(
            &panel->incTimeLimitButton,
            0
        );
        SetZrdWidgetEnabled(
            &panel->decTimeLimitButton,
            0
        );
    } else {
        SetWidgetVisible(
            &panel->killsSwitch,
            1
        );
        SetWidgetVisible(
            &panel->lapsSwitch,
            0
        );
        panel->killsInput.minValue = 1;
        panel->killsInput.maxValue = 99;

        SetZrdWidgetEnabled(
            &panel->timeLimitInput,
            1
        );
        SetZrdWidgetEnabled(
            &panel->incTimeLimitButton,
            1
        );
        SetZrdWidgetEnabled(
            &panel->decTimeLimitButton,
            1
        );
    }

    panel->killsInput.Invalidate();
    panel->incKillsButton.Invalidate();
    panel->decKillsButton.Invalidate();
}
} // namespace

// Reimplements 0x419aa0: HudUiNetGameSetupPanel::Constructor
// (D:\Proj\Battlesport\HudUiNetGameSetup.cpp)
HudUiNetGameSetupPanel * HudUiNetGameSetupPanel::Constructor(
    int reconfigureExistingSessionValue
) {
    new ((HudUiBackground *)this) HudUiBackground;

    playButton.Constructor();
    cancelButton.Constructor();
    gameNameInput.Constructor(21);
    worldSelector.Constructor();
    nextWorldButton.Constructor();
    prevWorldButton.Constructor();
    new (&timeLimitInput) HudUiClampedIntTextInput(4);
    incTimeLimitButton.Constructor();
    incTimeLimitButton.targetInput = 0;
    incTimeLimitButton.stepDelta = 1;
    decTimeLimitButton.Constructor();
    decTimeLimitButton.targetInput = 0;
    decTimeLimitButton.stepDelta = 1;
    new (&killsInput) HudUiClampedIntTextInput(2);
    incKillsButton.Constructor();
    incKillsButton.targetInput = 0;
    incKillsButton.stepDelta = 1;
    decKillsButton.Constructor();
    decKillsButton.targetInput = 0;
    decKillsButton.stepDelta = 1;
    new (&maxPlayersInput) HudUiClampedIntTextInput(2);
    incMaxPlayersButton.Constructor();
    incMaxPlayersButton.targetInput = 0;
    incMaxPlayersButton.stepDelta = 1;
    decMaxPlayersButton.Constructor();
    decMaxPlayersButton.targetInput = 0;
    decMaxPlayersButton.stepDelta = 1;
    allowMapsToggle.Constructor();
    nameTagsToggle.Constructor();
    killsSwitch.Constructor(0);
    lapsSwitch.Constructor(0);

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
    return this;
}

// Reimplements 0x41a400: HudUiNetGameSetupPanel::Destructor
// (D:\Proj\Battlesport\HudUiNetGameSetup.cpp)
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

// Source-faithful helper recovered from address-backed callers in this source file.
HudUiNetGameSetupPanel * HudUiNetGameSetupPanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41a160: HudUiNetGameSetupPanel_CancelButton::OnActivate
void HudUiNetGameSetupPanel_CancelButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x41a5b0: HudUiNetGameSetupPanel_LaunchButton::OnActivate
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

// Reimplements 0x41a820: HudUiNetGameSetupPanel_NextWorldButton::OnActivate
void HudUiNetGameSetupPanel_NextWorldButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    ApplyWorldSelectionSideEffects(
        ownerPanel,
        ownerPanel->worldSelector.selectedIndex + 1
    );
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x41a9c0: HudUiNetGameSetupPanel_PrevWorldButton::OnActivate
void HudUiNetGameSetupPanel_PrevWorldButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    ApplyWorldSelectionSideEffects(
        ownerPanel,
        ownerPanel->worldSelector.selectedIndex - 1
    );
    HudUiZrdWidget::OnActivate();
}
