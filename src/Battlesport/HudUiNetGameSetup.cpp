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

#include <stdio.h>
#include <string.h>

namespace {
template <typename Method>
unsigned int MethodAddress(
    Method method
) {
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(
        &address,
        &method,
        sizeof(method)
    );
    return address;
}

RECOIL_NOINLINE void RECOIL_CDECL HudUiNetGameSetupPostLoadNoOp() {}

template <typename FTable>
FTable MakeHudUiNetGameSetupZrdFTable(
    unsigned int activateSlot
) {
    FTable table = {0};
    table.slots[0] = MethodAddress(&HudUiZrdWidget::ScalarDeletingDestructor);
    table.slots[1] = MethodAddress(&HudUiWidget::Draw);
    table.slots[3] = MethodAddress(&HudUiElement::SetPos);
    table.slots[4] = MethodAddress(&HudUiElement::SetX);
    table.slots[5] = MethodAddress(&HudUiElement::SetY);
    table.slots[6] = MethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = MethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = MethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = activateSlot;
    table.slots[15] = MethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = MethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = MethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = MethodAddress(&HudUiElement::GetX);
    table.slots[26] = MethodAddress(&HudUiElement::GetY);
    table.slots[30] = MethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = MethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = (unsigned int)(&HudUiNetGameSetupPostLoadNoOp);
    return table;
}

HudUiNetGameSetupPanel_FTable MakeHudUiNetGameSetupPanelFTable() {
    HudUiNetGameSetupPanel_FTable table = {0};
    table.slots[0] = MethodAddress(&HudUiBackground::Update);
    table.slots[1] = MethodAddress(&HudUiBackground::SetEnabled);
    table.slots[2] = MethodAddress(&HudUiNetGameSetupPanel::ScalarDeletingDestructor);
    return table;
}

HudUiZrdWidget_FTable MakeHudUiNetGameSetupPanelPlayButtonFTable() {
    return MakeHudUiNetGameSetupZrdFTable<HudUiZrdWidget_FTable>(
        MethodAddress(&HudUiNetGameSetupPanel_LaunchButton::OnActivate)
    );
}

HudUiNumericTextInput_Base_FTable MakeHudUiNetGameSetupPanelGameNameInputFTable() {
    HudUiNumericTextInput_Base_FTable table =
        MakeHudUiNetGameSetupZrdFTable<HudUiNumericTextInput_Base_FTable>(
            MethodAddress(&HudUiNetGameSetupTextInput::OnActivateFocusAndCursor)
        );
    table.slots[0] = MethodAddress(&HudUiNumericTextInput::ScalarDeletingDestructorThunk);
    table.slots[2] = MethodAddress(&HudUiNumericTextInput::UpdateCaptureUiAndClip);
    table.slots[34] = MethodAddress(&HudUiNumericTextInput::OnRawKeyboardChar);
    table.slots[35] = MethodAddress(&zStub::ReturnZeroNoArgs);
    return table;
}

HudUiClampedIntTextInput_FTable MakeHudUiNetGameSetupPanelClampedInputFTable() {
    HudUiClampedIntTextInput_FTable table =
        MakeHudUiNetGameSetupZrdFTable<HudUiClampedIntTextInput_FTable>(
            MethodAddress(&HudUiNetGameSetupTextInput::OnActivateFocusAndCursor)
        );
    table.slots[0] = MethodAddress(&HudUiNumericTextInput::ScalarDeletingDestructorThunk);
    table.slots[2] = MethodAddress(&HudUiNumericTextInput::UpdateCaptureUiAndClip);
    table.slots[33] = MethodAddress(&HudUiClampedIntTextInput::OnRawKeyboardDigitOnly);
    table.slots[34] = MethodAddress(&HudUiNumericTextInput::OnAcceptForwardToCommit);
    table.slots[35] = MethodAddress(&HudUiClampedIntTextInput::CommitAndGetValue);
    return table;
}

HudUiCycleSelectorWidget_FTable MakeHudUiNetGameSetupPanelWorldSelectorFTable() {
    HudUiCycleSelectorWidget_FTable table =
        MakeHudUiNetGameSetupZrdFTable<HudUiCycleSelectorWidget_FTable>(
            MethodAddress(&HudUiZrdWidget::OnActivate)
        );
    table.slots[0] = MethodAddress(&HudUiCycleSelectorWidget::ScalarDeletingDestructor);
    table.slots[9] = MethodAddress(&HudUiCycleSelectorWidget::Update);
    table.slots[31] = MethodAddress(&HudUiCycleSelectorWidget::LoadFromZrd);
    return table;
}

HudUiCheckToggleWidget_FTable MakeHudUiNetGameSetupPanelCheckToggleFTable() {
    HudUiCheckToggleWidget_FTable table =
        MakeHudUiNetGameSetupZrdFTable<HudUiCheckToggleWidget_FTable>(
            MethodAddress(&HudUiCheckToggleWidget::OnActivate)
        );
    table.slots[0] = MethodAddress(&HudUiCheckToggleWidget::ScalarDeletingDestructor);
    table.slots[31] = MethodAddress(&HudUiCheckToggleWidget::LoadFromZrd);
    return table;
}

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

void SetZrdWidgetEnabled(
    HudUiZrdWidget *widget,
    int enabled
) {
    widget->modeOrEnabled = enabled;
    widget->RefreshState();
}

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

void ConfigureStepButton(
    HudUiClampedIntStepButton *button,
    HudUiClampedIntTextInput *targetInput,
    int stepDelta
) {
    button->targetInput = targetInput;
    button->stepDelta = stepDelta;
}

void SetWidgetVisible(
    HudUiWidget *widget,
    int visible
) {
    ((HudUiElement *)(widget))->SetVisible(visible);
}

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
            &panel->incTimeLimitButton.base,
            0
        );
        SetZrdWidgetEnabled(
            &panel->decTimeLimitButton.base,
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
            &panel->timeLimitInput.base,
            1
        );
        SetZrdWidgetEnabled(
            &panel->incTimeLimitButton.base,
            1
        );
        SetZrdWidgetEnabled(
            &panel->decTimeLimitButton.base,
            1
        );
    }

    panel->killsInput.base.Invalidate();
    panel->incKillsButton.base.Invalidate();
    panel->decKillsButton.base.Invalidate();
}
} // namespace

const HudUiNetGameSetupPanel_FTable g_HudUiNetGameSetupPanel_FTable =
    MakeHudUiNetGameSetupPanelFTable();
const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_PlayButton_FTable =
    MakeHudUiNetGameSetupPanelPlayButtonFTable();
const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_CancelButton_FTable =
    MakeHudUiNetGameSetupZrdFTable<HudUiZrdWidget_FTable>(
        MethodAddress(&HudUiNetGameSetupPanel_CancelButton::OnActivate)
    );
const HudUiNumericTextInput_Base_FTable g_HudUiNetGameSetupPanel_GameNameInput_FTable =
    MakeHudUiNetGameSetupPanelGameNameInputFTable();
const HudUiCycleSelectorWidget_FTable g_HudUiNetGameSetupPanel_WorldSelector_FTable =
    MakeHudUiNetGameSetupPanelWorldSelectorFTable();
const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_NextWorldButton_FTable =
    MakeHudUiNetGameSetupZrdFTable<HudUiZrdWidget_FTable>(
        MethodAddress(&HudUiNetGameSetupPanel_NextWorldButton::OnActivate)
    );
const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_PrevWorldButton_FTable =
    MakeHudUiNetGameSetupZrdFTable<HudUiZrdWidget_FTable>(
        MethodAddress(&HudUiNetGameSetupPanel_PrevWorldButton::OnActivate)
    );
const HudUiClampedIntTextInput_FTable g_HudUiNetGameSetupPanel_ClampedInput_FTable =
    MakeHudUiNetGameSetupPanelClampedInputFTable();
const HudUiZrdWidget_FTable g_HudUiNetGameSetupPanel_StepButton_FTable =
    MakeHudUiNetGameSetupZrdFTable<HudUiZrdWidget_FTable>(
        MethodAddress(&HudUiClampedIntStepButton::OnActivate)
    );
const HudUiCheckToggleWidget_FTable g_HudUiNetGameSetupPanel_CheckToggle_FTable =
    MakeHudUiNetGameSetupPanelCheckToggleFTable();

// Reimplements 0x419aa0: HudUiNetGameSetupPanel::Constructor
// (D:\Proj\Battlesport\HudUiNetGameSetup.cpp)
HudUiNetGameSetupPanel *RECOIL_THISCALL HudUiNetGameSetupPanel::Constructor(
    int reconfigureExistingSessionValue
) {
    HudUiBackground::Constructor();

    playButton.Constructor();
    playButton.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_PlayButton_FTable);
    cancelButton.Constructor();
    cancelButton.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_CancelButton_FTable);
    gameNameInput.Constructor(21);
    gameNameInput.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_GameNameInput_FTable);
    worldSelector.Constructor();
    worldSelector.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_WorldSelector_FTable);
    nextWorldButton.Constructor();
    nextWorldButton.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_NextWorldButton_FTable);
    prevWorldButton.Constructor();
    prevWorldButton.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_PrevWorldButton_FTable);
    timeLimitInput.HudUiNumericTextInput::Constructor(4);
    timeLimitInput.minValue = -2147483647 - 1;
    timeLimitInput.maxValue = 2147483647;
    timeLimitInput.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_ClampedInput_FTable);
    incTimeLimitButton.base.Constructor();
    incTimeLimitButton.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_StepButton_FTable);
    incTimeLimitButton.targetInput = 0;
    incTimeLimitButton.stepDelta = 1;
    decTimeLimitButton.base.Constructor();
    decTimeLimitButton.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_StepButton_FTable);
    decTimeLimitButton.targetInput = 0;
    decTimeLimitButton.stepDelta = 1;
    killsInput.Constructor(2);
    killsInput.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_ClampedInput_FTable);
    incKillsButton.base.Constructor();
    incKillsButton.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_StepButton_FTable);
    incKillsButton.targetInput = 0;
    incKillsButton.stepDelta = 1;
    decKillsButton.base.Constructor();
    decKillsButton.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_StepButton_FTable);
    decKillsButton.targetInput = 0;
    decKillsButton.stepDelta = 1;
    maxPlayersInput.Constructor(2);
    maxPlayersInput.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_ClampedInput_FTable);
    incMaxPlayersButton.base.Constructor();
    incMaxPlayersButton.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_StepButton_FTable);
    incMaxPlayersButton.targetInput = 0;
    incMaxPlayersButton.stepDelta = 1;
    decMaxPlayersButton.base.Constructor();
    decMaxPlayersButton.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_StepButton_FTable);
    decMaxPlayersButton.targetInput = 0;
    decMaxPlayersButton.stepDelta = 1;
    allowMapsToggle.Constructor();
    allowMapsToggle.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_CheckToggle_FTable);
    nameTagsToggle.Constructor();
    nameTagsToggle.base.base.ftable =
        (const HudUiWidget_FTable *)(&g_HudUiNetGameSetupPanel_CheckToggle_FTable);
    killsSwitch.Constructor(0);
    lapsSwitch.Constructor(0);

    base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiNetGameSetupPanel_FTable);
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
            &playButton.base,
            "PLAY"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &cancelButton.base,
            "CANCEL"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &gameNameInput.base.base,
            "GAME_NAME"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &worldSelector.base.base,
            "WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &nextWorldButton.base,
            "INC_WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &prevWorldButton.base,
            "DEC_WORLD"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &timeLimitInput.base.base,
            "TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incTimeLimitButton.base.base,
            "INC_TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decTimeLimitButton.base.base,
            "DEC_TIME_LIMIT"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &killsInput.base.base,
            "KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incKillsButton.base.base,
            "INC_KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decKillsButton.base.base,
            "DEC_KILLS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &maxPlayersInput.base.base,
            "MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &incMaxPlayersButton.base.base,
            "INC_MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &decMaxPlayersButton.base.base,
            "DEC_MAX_PLAYERS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &allowMapsToggle.base.base,
            "ALLOW_MAPS"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &nameTagsToggle.base.base,
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
        &gameNameInput.base,
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
            &maxPlayersInput.base,
            0
        );
        SetZrdWidgetEnabled(
            &incMaxPlayersButton.base,
            0
        );
        SetZrdWidgetEnabled(
            &decMaxPlayersButton.base,
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
            &maxPlayersInput.base,
            enabledForNewSession
        );
        ConfigureStepButton(
            &incMaxPlayersButton,
            &maxPlayersInput,
            1
        );
        SetZrdWidgetEnabled(
            &incMaxPlayersButton.base,
            enabledForNewSession
        );
        ConfigureStepButton(
            &decMaxPlayersButton,
            &maxPlayersInput,
            -1
        );
        SetZrdWidgetEnabled(
            &decMaxPlayersButton.base,
            enabledForNewSession
        );
    }

    allowMapsToggle.SetChecked(1);
    nameTagsToggle.SetChecked(0);
    base.base.SetChildFlags(0);
    return this;
}

// Reimplements 0x41a400: HudUiNetGameSetupPanel::Destructor
// (D:\Proj\Battlesport\HudUiNetGameSetup.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL HudUiNetGameSetupPanel::Destructor() {
    lapsSwitch.DestructorCore();
    killsSwitch.DestructorCore();
    nameTagsToggle.DestructorCore();
    allowMapsToggle.DestructorCore();
    decMaxPlayersButton.base.DestructorCore();
    incMaxPlayersButton.base.DestructorCore();
    maxPlayersInput.Destructor();
    decKillsButton.base.DestructorCore();
    incKillsButton.base.DestructorCore();
    killsInput.Destructor();
    decTimeLimitButton.base.DestructorCore();
    incTimeLimitButton.base.DestructorCore();
    timeLimitInput.Destructor();
    prevWorldButton.DestructorCore();
    nextWorldButton.DestructorCore();
    worldSelector.DestructorCore();
    gameNameInput.Destructor();
    cancelButton.DestructorCore();
    playButton.DestructorCore();
    HudUiBackground::Destructor();
}

RECOIL_NOINLINE HudUiNetGameSetupPanel *RECOIL_THISCALL
HudUiNetGameSetupPanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41a160: HudUiNetGameSetupPanel_CancelButton::OnActivate
void RECOIL_THISCALL HudUiNetGameSetupPanel_CancelButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x41a5b0: HudUiNetGameSetupPanel_LaunchButton::OnActivate
void RECOIL_THISCALL HudUiNetGameSetupPanel_LaunchButton::OnActivate() {
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
void RECOIL_THISCALL HudUiNetGameSetupPanel_NextWorldButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    ApplyWorldSelectionSideEffects(
        ownerPanel,
        ownerPanel->worldSelector.selectedIndex + 1
    );
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x41a9c0: HudUiNetGameSetupPanel_PrevWorldButton::OnActivate
void RECOIL_THISCALL HudUiNetGameSetupPanel_PrevWorldButton::OnActivate() {
    HudUiNetGameSetupPanel *const ownerPanel = (HudUiNetGameSetupPanel *)(owner);
    ApplyWorldSelectionSideEffects(
        ownerPanel,
        ownerPanel->worldSelector.selectedIndex - 1
    );
    HudUiZrdWidget::OnActivate();
}
