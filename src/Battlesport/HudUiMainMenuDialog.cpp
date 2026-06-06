#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "Battlesport/hud.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <new>

struct RecoilStateCredits {
    static void QueuePush();
};

// Reimplements 0x414f40: HudUiMainMenuDialog_CreditsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void HudUiMainMenuDialog_CreditsButton::OnActivate() {
    RecoilStateCredits::QueuePush();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fa0: HudUiMenuBackButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void HudUiMenuBackButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

// Reimplements 0x414f60: HudUiMainMenuDialog_SaveButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void HudUiMainMenuDialog_SaveButton::OnActivate() {
    RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED
    );
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415140: HudUiMainMenuDialog_LoadButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void HudUiMainMenuDialog_LoadButton::OnActivate() {
    if (g_RecoilState_MainMenuTransition.m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_FADE);
        HudUiZrdWidget::OnActivate();
        return;
    }

    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_STANDARD);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414f80: HudUiMainMenuDialog_NewGameButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void HudUiMainMenuDialog_NewGameButton::OnActivate() {
    HudUiNewGamePanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fc0: HudUiMainMenuDialog_OptionsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void HudUiMainMenuDialog_OptionsButton::OnActivate() {
    HudUiOptionsPanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fe0: HudUiMainMenuDialog_QuitButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void HudUiMainMenuDialog_QuitButton::OnActivate() {
    RecoilStateConfirmQuit::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415000: HudUiMainMenuDialog_ControlsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void HudUiMainMenuDialog_ControlsButton::OnActivate() {
    RecoilStateControls::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

namespace {
inline int PlayerMenuSaveLoadBlocked(
    zUtil_PlayerStateStorage *playerState
) {
    return playerState->environmentAttachmentActive;
}

inline void BindButton(
    HudUiMainMenuDialog *dialog,
    zReader::Node *loadedSection,
    HudUiZrdWidget *widget,
    const char *name
) {
    dialog->BindWidgetByName(
        loadedSection,
        widget,
        name
    );
}

void BindNewLoadQuit(
    HudUiMainMenuDialog *dialog,
    zReader::Node *loadedSection
) {
    BindButton(
        dialog,
        loadedSection,
        &dialog->newGameButton,
        "NEWGAME"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->loadGameButton,
        "LOADGAME"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->quitButton,
        "QUIT"
    );
}

void BindFrontendButtons(
    HudUiMainMenuDialog *dialog,
    zReader::Node *loadedSection
) {
    BindButton(
        dialog,
        loadedSection,
        &dialog->newGameButton,
        "NEWGAME"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->loadGameButton,
        "LOADGAME"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->optionsButton,
        "OPTIONS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->controlsButton,
        "CONTROLS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->creditsButton,
        "CREDITS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->quitButton,
        "QUIT"
    );
}

void BindFullInGameButtons(
    HudUiMainMenuDialog *dialog,
    zReader::Node *loadedSection
) {
    BindButton(
        dialog,
        loadedSection,
        &dialog->newGameButton,
        "NEWGAME"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->saveGameButton,
        "SAVEGAME"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->loadGameButton,
        "LOADGAME"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->optionsButton,
        "OPTIONS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->controlsButton,
        "CONTROLS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->creditsButton,
        "CREDITS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->backButton,
        "BACK"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->quitButton,
        "QUIT"
    );
}

void BindNetworkButtons(
    HudUiMainMenuDialog *dialog,
    zReader::Node *loadedSection
) {
    BindButton(
        dialog,
        loadedSection,
        &dialog->optionsButton,
        "OPTIONS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->controlsButton,
        "CONTROLS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->creditsButton,
        "CREDITS"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->backButton,
        "BACK"
    );
    BindButton(
        dialog,
        loadedSection,
        &dialog->quitButton,
        "QUIT"
    );
}
} // namespace

inline HudUiMainMenuDialogBackground::HudUiMainMenuDialogBackground() : HudUiBackground() {
}

inline HudUiMainMenuDialogBackground::~HudUiMainMenuDialogBackground() {
}

inline HudUiMainMenuDialog_CreditsButton::HudUiMainMenuDialog_CreditsButton() :
    HudUiZrdWidget() {
}

inline HudUiMainMenuDialog_CreditsButton::~HudUiMainMenuDialog_CreditsButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiMenuBackButton::HudUiMenuBackButton() : HudUiZrdWidget() {
}

inline HudUiMenuBackButton::~HudUiMenuBackButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiMainMenuDialog_SaveButton::HudUiMainMenuDialog_SaveButton() : HudUiZrdWidget() {
}

inline HudUiMainMenuDialog_SaveButton::~HudUiMainMenuDialog_SaveButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiMainMenuDialog_LoadButton::HudUiMainMenuDialog_LoadButton() : HudUiZrdWidget() {
}

inline HudUiMainMenuDialog_LoadButton::~HudUiMainMenuDialog_LoadButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiMainMenuDialog_NewGameButton::HudUiMainMenuDialog_NewGameButton() :
    HudUiZrdWidget() {
}

inline HudUiMainMenuDialog_NewGameButton::~HudUiMainMenuDialog_NewGameButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiMainMenuDialog_OptionsButton::HudUiMainMenuDialog_OptionsButton() :
    HudUiZrdWidget() {
}

inline HudUiMainMenuDialog_OptionsButton::~HudUiMainMenuDialog_OptionsButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiMainMenuDialog_QuitButton::HudUiMainMenuDialog_QuitButton() : HudUiZrdWidget() {
}

inline HudUiMainMenuDialog_QuitButton::~HudUiMainMenuDialog_QuitButton() {
    HudUiZrdWidget::DestructorCore();
}

inline HudUiMainMenuDialog_ControlsButton::HudUiMainMenuDialog_ControlsButton() :
    HudUiZrdWidget() {
}

inline HudUiMainMenuDialog_ControlsButton::~HudUiMainMenuDialog_ControlsButton() {
    HudUiZrdWidget::DestructorCore();
}

// Reimplements 0x414b60: HudUiMainMenuDialog::CanLoadGame
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
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

// Reimplements 0x414b90: HudUiMainMenuDialog::CanSaveGame
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
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

// Reimplements 0x414bc0: HudUiMainMenuDialog::HudUiMainMenuDialog
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
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
            BindButton(
                this,
                loadedSection,
                &optionsButton,
                "OPTIONS"
            );
            BindButton(
                this,
                loadedSection,
                &controlsButton,
                "CONTROLS"
            );
            BindButton(
                this,
                loadedSection,
                &creditsButton,
                "CREDITS"
            );
            BindButton(
                this,
                loadedSection,
                &backButton,
                "BACK"
            );
            BindButton(
                this,
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
                BindButton(
                    this,
                    loadedSection,
                    &newGameButton,
                    "NEWGAME"
                );
                BindButton(
                    this,
                    loadedSection,
                    loadButton,
                    "LOADGAME"
                );
                BindButton(
                    this,
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
                BindButton(
                    this,
                    loadedSection,
                    &newGameButton,
                    "NEWGAME"
                );
                BindButton(
                    this,
                    loadedSection,
                    saveButton,
                    "SAVEGAME"
                );
                BindButton(
                    this,
                    loadedSection,
                    loadButton,
                    "LOADGAME"
                );
                BindButton(
                    this,
                    loadedSection,
                    &optionsButton,
                    "OPTIONS"
                );
                BindButton(
                    this,
                    loadedSection,
                    &controlsButton,
                    "CONTROLS"
                );
                BindButton(
                    this,
                    loadedSection,
                    &creditsButton,
                    "CREDITS"
                );
                BindButton(
                    this,
                    loadedSection,
                    &backButton,
                    "BACK"
                );
                BindButton(
                    this,
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
        BindButton(
            this,
            loadedSection,
            &newGameButton,
            "NEWGAME"
        );
        BindButton(
            this,
            loadedSection,
            loadButton,
            "LOADGAME"
        );
        BindButton(
            this,
            loadedSection,
            &optionsButton,
            "OPTIONS"
        );
        BindButton(
            this,
            loadedSection,
            &controlsButton,
            "CONTROLS"
        );
        BindButton(
            this,
            loadedSection,
            &creditsButton,
            "CREDITS"
        );
        BindButton(
            this,
            loadedSection,
            quitButtonPtr,
            "QUIT"
        );
        FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    loadButton->modeOrEnabled = CanLoadGame();
    loadButton->RefreshState();
}

// Reimplements 0x415040: HudUiMainMenuDialog::~HudUiMainMenuDialog
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
HudUiMainMenuDialog::~HudUiMainMenuDialog() {}
