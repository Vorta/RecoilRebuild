#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "Battlesport/hud.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <new>

struct RecoilStateCredits {
    static void QueuePush();
};

/**
 * Reimplements 0x414f40: HudUiMainMenuDialog_CreditsButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Queue the credits state and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_CreditsButton::OnActivate() {
    RecoilStateCredits::QueuePush();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x414fa0: HudUiMenuBackButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Exit the current state and refresh the active HUD layout.
 */
void HudUiMenuBackButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

/**
 * Reimplements 0x414f60: HudUiMainMenuDialog_SaveButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Open the save dialog and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_SaveButton::OnActivate() {
    RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED
    );
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x415140: HudUiMainMenuDialog_LoadButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Open the load dialog using the frontend or in-game transition mode.
 */
void HudUiMainMenuDialog_LoadButton::OnActivate() {
    if (g_RecoilState_MainMenuTransition.m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_FADE);
        HudUiZrdWidget::OnActivate();
        return;
    }

    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_STANDARD);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x414f80: HudUiMainMenuDialog_NewGameButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Enter the new-game overlay and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_NewGameButton::OnActivate() {
    HudUiNewGamePanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x414fc0: HudUiMainMenuDialog_OptionsButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Enter the options overlay and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_OptionsButton::OnActivate() {
    HudUiOptionsPanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x414fe0: HudUiMainMenuDialog_QuitButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Enter the quit confirmation state and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_QuitButton::OnActivate() {
    RecoilStateConfirmQuit::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x415000: HudUiMainMenuDialog_ControlsButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Enter the controls state and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_ControlsButton::OnActivate() {
    RecoilStateControls::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

namespace {
/**
 * Recovered original inline source helper: no standalone retail function.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Evidence: repeated caller bodies at 0x414b60 and 0x414b90 read
 * zUtil_PlayerStateStorage::environmentAttachmentActive at offset 0x25c.
 * Purpose: Report whether the current player state blocks save/load menu actions.
 */
inline int PlayerMenuSaveLoadBlocked(
    zUtil_PlayerStateStorage *playerState
) {
    return playerState->environmentAttachmentActive;
}

} // namespace

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the background
 * member with a HudUiBackground base subobject.
 * Purpose: Initialize the dialog background member.
 */
inline HudUiMainMenuDialogBackground::HudUiMainMenuDialogBackground() : HudUiBackground() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the background
 * member without additional body work.
 * Purpose: Tear down the dialog background member.
 */
inline HudUiMainMenuDialogBackground::~HudUiMainMenuDialogBackground() {
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the credits
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the credits button member.
 */
inline HudUiMainMenuDialog_CreditsButton::HudUiMainMenuDialog_CreditsButton() :
    HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the credits
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the credits button member.
 */
inline HudUiMainMenuDialog_CreditsButton::~HudUiMainMenuDialog_CreditsButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the save
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the save button member.
 */
inline HudUiMainMenuDialog_SaveButton::HudUiMainMenuDialog_SaveButton() : HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the save
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the save button member.
 */
inline HudUiMainMenuDialog_SaveButton::~HudUiMainMenuDialog_SaveButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the load
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the load button member.
 */
inline HudUiMainMenuDialog_LoadButton::HudUiMainMenuDialog_LoadButton() : HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the load
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the load button member.
 */
inline HudUiMainMenuDialog_LoadButton::~HudUiMainMenuDialog_LoadButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the new-game
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the new-game button member.
 */
inline HudUiMainMenuDialog_NewGameButton::HudUiMainMenuDialog_NewGameButton() :
    HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the new-game
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the new-game button member.
 */
inline HudUiMainMenuDialog_NewGameButton::~HudUiMainMenuDialog_NewGameButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the options
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the options button member.
 */
inline HudUiMainMenuDialog_OptionsButton::HudUiMainMenuDialog_OptionsButton() :
    HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the options
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the options button member.
 */
inline HudUiMainMenuDialog_OptionsButton::~HudUiMainMenuDialog_OptionsButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the quit
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the quit button member.
 */
inline HudUiMainMenuDialog_QuitButton::HudUiMainMenuDialog_QuitButton() : HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the quit
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the quit button member.
 */
inline HudUiMainMenuDialog_QuitButton::~HudUiMainMenuDialog_QuitButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the controls
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the controls button member.
 */
inline HudUiMainMenuDialog_ControlsButton::HudUiMainMenuDialog_ControlsButton() :
    HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the controls
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the controls button member.
 */
inline HudUiMainMenuDialog_ControlsButton::~HudUiMainMenuDialog_ControlsButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Reimplements 0x414b60: HudUiMainMenuDialog::CanLoadGame.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Allow load-game navigation unless the active player state is blocked.
 */
int HudUiMainMenuDialog::CanLoadGame() {
    zUtil_PlayerStateStorage *playerState;
    zInput_GameStateOrMapTablePartial *const gameState = g_GameStateOrMapTable;
    if (gameState == 0) {
        goto canLoad;
    }

    playerState = (zUtil_PlayerStateStorage *)gameState->playerState;
    if (playerState == 0) {
        goto canLoad;
    }

    if (PlayerMenuSaveLoadBlocked(playerState) != 0) {
        return 0;
    }

canLoad:
    return 1;
}

/**
 * Reimplements 0x414b90: HudUiMainMenuDialog::CanSaveGame.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Allow save-game navigation only when active game state is present and not blocked.
 */
int HudUiMainMenuDialog::CanSaveGame() {
    zInput_GameStateOrMapTablePartial *const gameState = g_GameStateOrMapTable;
    if (gameState == 0) {
        return (int)gameState;
    }

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)gameState->playerState;
    if (playerState == 0) {
        goto canSave;
    }

    if (PlayerMenuSaveLoadBlocked(playerState) != 0) {
        return 0;
    }

canSave:
    return 1;
}

/**
 * Reimplements 0x414bc0: HudUiMainMenuDialog::HudUiMainMenuDialog.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Load the route-specific main-menu layout and bind its child buttons.
 */
HudUiMainMenuDialog::HudUiMainMenuDialog(
    RecoilMainMenuEntryRoute route
) {
    // Preserves the VC5SP3 register lifetime observed in BN 0x414bc0 for the
    // repeatedly bound save, load, and quit button subobjects.
    HudUiMainMenuDialog_SaveButton *const saveButton = &saveGameButton;
    HudUiMainMenuDialog_LoadButton *const loadButton = &loadGameButton;
    HudUiMainMenuDialog_QuitButton *const quitButtonPtr = &quitButton;

    if (zOpt::GetNetworkEnabled() != 0) {
        zReader::Node *const loadedSection = LoadFromZrd(
            "dialog.zrd",
            "MAINMENU2",
            0
        );
        if (loadedSection != 0) {
            BindWidgetByName(
                loadedSection,
                &optionsButton,
                "OPTIONS"
            );
            BindWidgetByName(
                loadedSection,
                &controlsButton,
                "CONTROLS"
            );
            BindWidgetByName(
                loadedSection,
                &creditsButton,
                "CREDITS"
            );
            BindWidgetByName(
                loadedSection,
                &backButton,
                "BACK"
            );
            BindWidgetByName(
                loadedSection,
                quitButtonPtr,
                "QUIT"
            );
            FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
        }
        return;
    }

    if (route != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        zInput_GameStateOrMapTablePartial *const gameState = g_GameStateOrMapTable;
        zUtil_PlayerStateStorage *const playerState =
            (zUtil_PlayerStateStorage *)gameState->playerState;
        if (playerState->lifecycleState == 4) {
            zReader::Node *const loadedSection = LoadFromZrd(
                "dialog.zrd",
                "MAINMENU3",
                0
            );
            if (loadedSection != 0) {
                BindWidgetByName(
                    loadedSection,
                    &newGameButton,
                    "NEWGAME"
                );
                BindWidgetByName(
                    loadedSection,
                    loadButton,
                    "LOADGAME"
                );
                BindWidgetByName(
                    loadedSection,
                    quitButtonPtr,
                    "QUIT"
                );
                FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
            }
        } else {
            zReader::Node *const loadedSection = LoadFromZrd(
                "dialog.zrd",
                "MAINMENU1",
                0
            );
            if (loadedSection != 0) {
                BindWidgetByName(
                    loadedSection,
                    &newGameButton,
                    "NEWGAME"
                );
                BindWidgetByName(
                    loadedSection,
                    saveButton,
                    "SAVEGAME"
                );
                BindWidgetByName(
                    loadedSection,
                    loadButton,
                    "LOADGAME"
                );
                BindWidgetByName(
                    loadedSection,
                    &optionsButton,
                    "OPTIONS"
                );
                BindWidgetByName(
                    loadedSection,
                    &controlsButton,
                    "CONTROLS"
                );
                BindWidgetByName(
                    loadedSection,
                    &creditsButton,
                    "CREDITS"
                );
                BindWidgetByName(
                    loadedSection,
                    &backButton,
                    "BACK"
                );
                BindWidgetByName(
                    loadedSection,
                    quitButtonPtr,
                    "QUIT"
                );
                FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
            }
        }

        saveButton->modeOrEnabled = CanSaveGame();
        saveButton->RefreshState();
        loadButton->modeOrEnabled = CanLoadGame();
        loadButton->RefreshState();
        return;
    }

    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "MAINMENU0",
        0
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &newGameButton,
            "NEWGAME"
        );
        BindWidgetByName(
            loadedSection,
            loadButton,
            "LOADGAME"
        );
        BindWidgetByName(
            loadedSection,
            &optionsButton,
            "OPTIONS"
        );
        BindWidgetByName(
            loadedSection,
            &controlsButton,
            "CONTROLS"
        );
        BindWidgetByName(
            loadedSection,
            &creditsButton,
            "CREDITS"
        );
        BindWidgetByName(
            loadedSection,
            quitButtonPtr,
            "QUIT"
        );
        FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    loadButton->modeOrEnabled = CanLoadGame();
    loadButton->RefreshState();
}

/**
 * Reimplements 0x415040: HudUiMainMenuDialog::~HudUiMainMenuDialog.
 * Original source path: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Provide the owner-level destructor body for member teardown.
 */
HudUiMainMenuDialog::~HudUiMainMenuDialog() {}
