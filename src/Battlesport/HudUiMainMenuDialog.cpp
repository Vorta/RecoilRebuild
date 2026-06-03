#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"

#include "Battlesport/hud.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <string.h>

struct RecoilStateCredits {
    static void RECOIL_CDECL QueuePush();
};

namespace {
const size_t kPlayerMenuSaveLoadBlockOffset = 0x25c;

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

template <
    typename Slot,
    typename Method>
void AssignMethodSlot(
    Slot &slot,
    Method method
) {
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(slot));
    memset(
        &slot,
        0,
        sizeof(slot)
    );
    memcpy(
        &slot,
        &method,
        sizeof(method)
    );
}

RECOIL_NOINLINE void RECOIL_CDECL HudUiWidgetPostLoadNoOp() {}

// Main-menu button vtables differ in the activation callback at slot 12.
HudUiWidget_FTable MakeMainMenuButtonFTable(
    unsigned int activateCallback
) {
    HudUiWidget_FTable table = {0};
    table.slots[0] = MethodAddress(&HudUiZrdWidget::ScalarDeletingDestructor);
    table.slots[1] = MethodAddress(&HudUiWidget::Draw);
    table.slots[3] = MethodAddress(&HudUiElement::SetPos);
    table.slots[4] = MethodAddress(&HudUiElement::SetX);
    table.slots[5] = MethodAddress(&HudUiElement::SetY);
    table.slots[6] = MethodAddress(&HudUiElement::SetBltSourceAndClipRect);
    table.slots[7] = MethodAddress(&HudUiElement::SetClipRect);
    table.slots[8] = MethodAddress(&HudUiZrdWidget::Invalidate);
    table.slots[12] = activateCallback;
    table.slots[15] = MethodAddress(&HudUiZrdWidget::ShowPreview);
    table.slots[16] = MethodAddress(&HudUiZrdWidget::HidePreview);
    table.slots[24] = MethodAddress(&HudUiElement::SetVisible);
    table.slots[25] = MethodAddress(&HudUiElement::GetX);
    table.slots[26] = MethodAddress(&HudUiElement::GetY);
    table.slots[30] = MethodAddress(&HudUiZrdWidget::RefreshState);
    table.slots[31] = MethodAddress(&HudUiZrdWidget::LoadFromZrd);
    table.slots[32] = MethodAddress(&HudUiWidgetPostLoadNoOp);
    return table;
}
} // namespace

// Reimplements 0x414f40: HudUiMainMenuDialog_CreditsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_CreditsButton::OnActivate() {
    RecoilStateCredits::QueuePush();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fa0: HudUiMenuBackButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMenuBackButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

// Reimplements 0x414f60: HudUiMainMenuDialog_SaveButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_SaveButton::OnActivate() {
    RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED
    );
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415140: HudUiMainMenuDialog_LoadButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_LoadButton::OnActivate() {
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
void RECOIL_THISCALL HudUiMainMenuDialog_NewGameButton::OnActivate() {
    HudUiNewGamePanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fc0: HudUiMainMenuDialog_OptionsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_OptionsButton::OnActivate() {
    HudUiOptionsPanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fe0: HudUiMainMenuDialog_QuitButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_QuitButton::OnActivate() {
    RecoilStateConfirmQuit::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415000: HudUiMainMenuDialog_ControlsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_ControlsButton::OnActivate() {
    RecoilStateControls::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

namespace {
const HudUiWidget_FTable g_HudUiMainMenu_CreditsButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMainMenuDialog_CreditsButton::OnActivate));
const HudUiWidget_FTable g_HudUiMainMenu_SaveGameButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMainMenuDialog_SaveButton::OnActivate));
const HudUiWidget_FTable g_HudUiMainMenu_LoadGameButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMainMenuDialog_LoadButton::OnActivate));
const HudUiWidget_FTable g_HudUiMainMenu_NewGameButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMainMenuDialog_NewGameButton::OnActivate));
const HudUiWidget_FTable g_HudUiMainMenu_OptionsButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMainMenuDialog_OptionsButton::OnActivate));
const HudUiWidget_FTable g_HudUiMainMenu_QuitButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMainMenuDialog_QuitButton::OnActivate));
const HudUiWidget_FTable g_HudUiMainMenu_ControlsButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMainMenuDialog_ControlsButton::OnActivate));

struct HudUiMainMenuDialog_FTable {
    HudUiContainerUpdateAllFn updateAll;
    HudUiContainerSetEnabledFn setEnabled;
    unsigned int scalarDeletingDestructor;
};

HudUiMainMenuDialog_FTable MakeMainMenuDialogFTable() {
    HudUiMainMenuDialog_FTable table = {0};
    AssignMethodSlot(
        table.updateAll,
        &HudUiBackground::Update
    );
    AssignMethodSlot(
        table.setEnabled,
        &HudUiBackground::SetEnabled
    );
    return table;
}

const HudUiMainMenuDialog_FTable g_HudUiMainMenuDialog_FTable = MakeMainMenuDialogFTable();

RECOIL_FORCEINLINE void InstallMainMenuDialogFTable(
    HudUiMainMenuDialog *dialog
) {
    dialog->base.base.vptr = (const HudUiContainer_FTable *)(&g_HudUiMainMenuDialog_FTable);
}

RECOIL_FORCEINLINE int PlayerMenuSaveLoadBlocked(
    zUtil_PlayerStateStorage *playerState
) {
    const unsigned char *const playerBytes = playerState->bytes;
    return *(const int *)(playerBytes + kPlayerMenuSaveLoadBlockOffset);
}

RECOIL_FORCEINLINE void BindButton(
    HudUiMainMenuDialog *dialog,
    zReader::Node *loadedSection,
    HudUiZrdWidget *widget,
    const char *name
) {
    dialog->BindWidgetByName(
        loadedSection,
        &widget->base,
        name
    );
}

typedef void (HudUiZrdWidget::*HudUiMainMenuRefreshStateMethod)();

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

RECOIL_FORCEINLINE HudUiMainMenuDialogBackground::HudUiMainMenuDialogBackground() {
    HudUiBackground::Constructor();
}

RECOIL_FORCEINLINE HudUiMainMenuDialogBackground::~HudUiMainMenuDialogBackground() {
    HudUiBackground::Destructor();
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_CreditsButton::HudUiMainMenuDialog_CreditsButton() {
    HudUiZrdWidget::Constructor();
    base.ftable = &g_HudUiMainMenu_CreditsButton_FTable;
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_CreditsButton::~HudUiMainMenuDialog_CreditsButton() {
    HudUiZrdWidget::DestructorCore();
}

RECOIL_FORCEINLINE HudUiMenuBackButton::HudUiMenuBackButton() {
    HudUiZrdWidget::Constructor();
    base.ftable = &g_HudUiMainMenu_BackButton_FTable;
}

RECOIL_FORCEINLINE HudUiMenuBackButton::~HudUiMenuBackButton() {
    HudUiZrdWidget::DestructorCore();
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_SaveButton::HudUiMainMenuDialog_SaveButton() {
    HudUiZrdWidget::Constructor();
    base.ftable = &g_HudUiMainMenu_SaveGameButton_FTable;
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_SaveButton::~HudUiMainMenuDialog_SaveButton() {
    HudUiZrdWidget::DestructorCore();
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_LoadButton::HudUiMainMenuDialog_LoadButton() {
    HudUiZrdWidget::Constructor();
    base.ftable = &g_HudUiMainMenu_LoadGameButton_FTable;
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_LoadButton::~HudUiMainMenuDialog_LoadButton() {
    HudUiZrdWidget::DestructorCore();
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_NewGameButton::HudUiMainMenuDialog_NewGameButton() {
    HudUiZrdWidget::Constructor();
    base.ftable = &g_HudUiMainMenu_NewGameButton_FTable;
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_NewGameButton::~HudUiMainMenuDialog_NewGameButton() {
    HudUiZrdWidget::DestructorCore();
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_OptionsButton::HudUiMainMenuDialog_OptionsButton() {
    HudUiZrdWidget::Constructor();
    base.ftable = &g_HudUiMainMenu_OptionsButton_FTable;
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_OptionsButton::~HudUiMainMenuDialog_OptionsButton() {
    HudUiZrdWidget::DestructorCore();
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_QuitButton::HudUiMainMenuDialog_QuitButton() {
    HudUiZrdWidget::Constructor();
    base.ftable = &g_HudUiMainMenu_QuitButton_FTable;
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_QuitButton::~HudUiMainMenuDialog_QuitButton() {
    HudUiZrdWidget::DestructorCore();
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_ControlsButton::HudUiMainMenuDialog_ControlsButton() {
    HudUiZrdWidget::Constructor();
    base.ftable = &g_HudUiMainMenu_ControlsButton_FTable;
}

RECOIL_FORCEINLINE HudUiMainMenuDialog_ControlsButton::~HudUiMainMenuDialog_ControlsButton() {
    HudUiZrdWidget::DestructorCore();
}

extern const HudUiWidget_FTable g_HudUiMainMenu_BackButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMenuBackButton::OnActivate));

// Reimplements 0x414b60: HudUiMainMenuDialog::CanLoadGame
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
RECOIL_NOINLINE int RECOIL_CDECL HudUiMainMenuDialog::CanLoadGame() {
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
RECOIL_NOINLINE int RECOIL_CDECL HudUiMainMenuDialog::CanSaveGame() {
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
    InstallMainMenuDialogFTable(this);

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
        (saveButton->*((HudUiMainMenuRefreshStateMethod *)(&saveButton->base.ftable->slots[30]))[0])();
        loadButton->modeOrEnabled = CanLoadGame();
        (loadButton->*((HudUiMainMenuRefreshStateMethod *)(&loadButton->base.ftable->slots[30]))[0])();
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
    (loadButton->*((HudUiMainMenuRefreshStateMethod *)(&loadButton->base.ftable->slots[30]))[0])();
}

// Reimplements 0x415040: HudUiMainMenuDialog::~HudUiMainMenuDialog
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
HudUiMainMenuDialog::~HudUiMainMenuDialog() {}
