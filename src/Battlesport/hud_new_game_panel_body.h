#pragma once

#include "Battlesport/hud.h"
#include "GameZRecoil/include/opt_catalog.h"

/* Complete HudUiNewGamePanel family body for the later mission.cpp host. */
/* Include exactly once after AiPropertyDlg and before NetSessionConfigDialog. */

/**
 * Reimplements 0x41c270: HudUiNewGamePanel_StartButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Start the new game through the owning panel before normal widget activation.
 */
void HudUiNewGamePanel_StartButton::OnActivate() {
    HudUiNewGamePanel *const panel = (HudUiNewGamePanel *)(owner);
    if (panel != 0) {
        panel->StartGameFromFields();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x41c290: HudUiNewGamePanel::HudUiNewGamePanel.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Construct the panel, bind its ZRD widgets, and load the player name.
 */
HudUiNewGamePanel::HudUiNewGamePanel()
    : HudUiBackground() {
    zReader::Node *const loadedSection =
        HudUiBackground::LoadFromZrd(
            "dialog.zrd",
            "NEWGAMEPANEL",
            0
        );
    if (loadedSection != 0) {
        HudUiBackground::BindWidgetByName(loadedSection, &backWidget, "BACK");
        HudUiBackground::BindWidgetByName(loadedSection, &startWidget, "START");
        HudUiBackground::BindWidgetByName(loadedSection, &nameInput, "NAME");
        HudUiBackground::BindWidgetByName(loadedSection, &intensity, "INTENSITY");
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    nameInput.Update(zOpt_GetPlayerName());
}

/**
 * Reimplements 0x41c3b0: HudUiNewGamePanel_NameInput::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Refresh and activate the player-name input with raw keyboard capture.
 */
void HudUiNewGamePanel_NameInput::OnActivate() {
    textInput.AllocTextBuffer(21);
    HudUiNumericTextInput::Update(zOpt_GetPlayerName());
    HudUiNumericTextInput::OnActivate();
    HudUiNumericTextInput::SetRawKeyboardCapture(1);
}

/**
 * Reimplements 0x41c400: HudUiNewGamePanel::~HudUiNewGamePanel.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Tear down the panel through ordinary reverse C++ member/base cleanup.
 */
HudUiNewGamePanel::~HudUiNewGamePanel() {
}

/**
 * Reimplements 0x41c4e0: HudUiNewGamePanel::SyncIntensityFromDifficulty.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Reflect the saved game difficulty in the panel selector.
 */
void HudUiNewGamePanel::SyncIntensityFromDifficulty() {
    intensity.SetSelectedIndex(zOpt::GetGameDifficultyMode());
}

/**
 * Reimplements 0x41c500: HudUiNewGamePanel::StartGameFromFields.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Commit new-game options and queue mission FMV startup.
 */
void HudUiNewGamePanel::StartGameFromFields() {
    HudCheat::ClearNanitePanelCheatSentinel();
    zOpt::SetPlayerName(nameInput.GetBuffer());
    zOpt::SetGameDifficultyMode(intensity.selectedIndex);
    ((HudUiBackgroundContainer *)(&g_RecoilApp.m_missionFmvState))
        ->HudUiBackgroundContainer::SetEnabled(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_missionFmvState, 0);
}

/**
 * Reimplements 0x41c560: HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Create, enable, and retain the new-game panel for the overlay state.
 */
int HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent() {
    HudUiNewGamePanel *const panel = new HudUiNewGamePanel;
    m_dialog = panel;
    panel->SyncIntensityFromDifficulty();
    panel->SetEnabled(1);
    return 1;
}

/**
 * Reimplements data 0x4f32c8: g_HudUiNewGamePanelOverlayOwner.
 * Purpose: Own the ordinary static-storage new-game overlay state object.
 */
HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;

/**
 * Reimplements 0x41c630: HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Disable and destroy the active panel owned by this app state.
 */
HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner() {
    HudUiNewGamePanel *panel = (HudUiNewGamePanel *)m_dialog;
    if (panel != 0) {
        panel->SetEnabled(0);

        panel = (HudUiNewGamePanel *)m_dialog;
        if (panel != 0) {
            delete panel;
        }

        m_dialog = 0;
    }
}

/**
 * Reimplements 0x41c6c0: HudUiNewGamePanelOverlayOwner::QueueEnter.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Queue the global overlay owner as the next app state.
 */
void HudUiNewGamePanelOverlayOwner::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudUiNewGamePanelOverlayOwner,
        0
    );
}
