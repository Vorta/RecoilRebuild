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

struct HudUiMainMenuDialog_CreditsButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMenuBackButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_SaveButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_LoadButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_NewGameButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_OptionsButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_QuitButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

struct HudUiMainMenuDialog_ControlsButton : HudUiZrdWidget {
    void RECOIL_THISCALL OnActivate();
};

template <typename Method> unsigned int MethodAddress(Method method)
{
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(unsigned int));
    unsigned int address = 0;
    memcpy(&address, &method, sizeof(method));
    return address;
}

template <typename Function, typename Method> Function MethodFunction(Method method)
{
    RECOIL_STATIC_ASSERT(sizeof(method) <= sizeof(Function));
    Function function = 0;
    memcpy(&function, &method, sizeof(method));
    return function;
}

RECOIL_NOINLINE void RECOIL_CDECL HudUiWidgetPostLoadNoOp()
{
}

// Main-menu button vtables differ in the activation callback at slot 12.
HudUiWidget_FTable MakeMainMenuButtonFTable(unsigned int activateCallback)
{
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

// Reimplements 0x414f40: HudUiMainMenuDialog_CreditsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_CreditsButton::OnActivate()
{
    RecoilStateCredits::QueuePush();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fa0: HudUiMenuBackButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMenuBackButton::OnActivate()
{
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

// Reimplements 0x414f60: HudUiMainMenuDialog_SaveButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_SaveButton::OnActivate()
{
    RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415140: HudUiMainMenuDialog_LoadButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_LoadButton::OnActivate()
{
    const RecoilSaveLoadTransitionMode transitionMode =
        g_RecoilState_MainMenuTransition.m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND
            ? RECOIL_SAVELOAD_MODE_FADE
            : RECOIL_SAVELOAD_MODE_STANDARD;
    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(transitionMode);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414f80: HudUiMainMenuDialog_NewGameButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_NewGameButton::OnActivate()
{
    HudUiNewGamePanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fc0: HudUiMainMenuDialog_OptionsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_OptionsButton::OnActivate()
{
    HudUiOptionsPanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x414fe0: HudUiMainMenuDialog_QuitButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_QuitButton::OnActivate()
{
    RecoilStateConfirmQuit::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415000: HudUiMainMenuDialog_ControlsButton::OnActivate
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog_ControlsButton::OnActivate()
{
    RecoilStateControls::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

const HudUiWidget_FTable g_HudUiMainMenu_CreditsButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMainMenuDialog_CreditsButton::OnActivate));
const HudUiWidget_FTable g_HudUiMainMenu_BackButton_FTable =
    MakeMainMenuButtonFTable(MethodAddress(&HudUiMenuBackButton::OnActivate));
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

HudUiMainMenuDialog_Vtbl MakeMainMenuDialogFTable()
{
    HudUiMainMenuDialog_Vtbl table = {0};
    table.UpdateAll = MethodFunction<void(RECOIL_THISCALL *)(HudUiMainMenuDialog *, float)>(
        &HudUiMainMenuDialog::Update);
    table.SetEnabled = MethodFunction<void(RECOIL_THISCALL *)(HudUiMainMenuDialog *, int)>(
        &HudUiMainMenuDialog::SetEnabled);
    table.ScalarDeletingDtor = MethodFunction<HudUiMainMenuDialog *(RECOIL_THISCALL *)(
        HudUiMainMenuDialog *, unsigned int)>(
        &HudUiMainMenuDialog::ScalarDeletingDestructor);
    return table;
}

const HudUiMainMenuDialog_Vtbl g_HudUiMainMenuDialog_FTable = MakeMainMenuDialogFTable();

RECOIL_FORCEINLINE int PlayerMenuSaveLoadBlocked(zUtil_PlayerStateStorage *playerState)
{
    const unsigned char *const playerBytes = playerState->bytes;
    return *(const int *)(playerBytes + kPlayerMenuSaveLoadBlockOffset);
}

RECOIL_FORCEINLINE void BindButton(HudUiMainMenuDialog *dialog, zReader::Node *loadedSection,
                                   HudUiZrdWidget *widget, const char *name)
{
    dialog->base.BindWidgetByName(loadedSection, &widget->base, name);
}

void BindNewLoadQuit(HudUiMainMenuDialog *dialog, zReader::Node *loadedSection)
{
    BindButton(dialog, loadedSection, &dialog->newGameButton, "NEWGAME");
    BindButton(dialog, loadedSection, &dialog->loadGameButton, "LOADGAME");
    BindButton(dialog, loadedSection, &dialog->quitButton, "QUIT");
}

void BindFrontendButtons(HudUiMainMenuDialog *dialog, zReader::Node *loadedSection)
{
    BindButton(dialog, loadedSection, &dialog->newGameButton, "NEWGAME");
    BindButton(dialog, loadedSection, &dialog->loadGameButton, "LOADGAME");
    BindButton(dialog, loadedSection, &dialog->optionsButton, "OPTIONS");
    BindButton(dialog, loadedSection, &dialog->controlsButton, "CONTROLS");
    BindButton(dialog, loadedSection, &dialog->creditsButton, "CREDITS");
    BindButton(dialog, loadedSection, &dialog->quitButton, "QUIT");
}

void BindFullInGameButtons(HudUiMainMenuDialog *dialog, zReader::Node *loadedSection)
{
    BindButton(dialog, loadedSection, &dialog->newGameButton, "NEWGAME");
    BindButton(dialog, loadedSection, &dialog->saveGameButton, "SAVEGAME");
    BindButton(dialog, loadedSection, &dialog->loadGameButton, "LOADGAME");
    BindButton(dialog, loadedSection, &dialog->optionsButton, "OPTIONS");
    BindButton(dialog, loadedSection, &dialog->controlsButton, "CONTROLS");
    BindButton(dialog, loadedSection, &dialog->creditsButton, "CREDITS");
    BindButton(dialog, loadedSection, &dialog->backButton, "BACK");
    BindButton(dialog, loadedSection, &dialog->quitButton, "QUIT");
}

void BindNetworkButtons(HudUiMainMenuDialog *dialog, zReader::Node *loadedSection)
{
    BindButton(dialog, loadedSection, &dialog->optionsButton, "OPTIONS");
    BindButton(dialog, loadedSection, &dialog->controlsButton, "CONTROLS");
    BindButton(dialog, loadedSection, &dialog->creditsButton, "CREDITS");
    BindButton(dialog, loadedSection, &dialog->backButton, "BACK");
    BindButton(dialog, loadedSection, &dialog->quitButton, "QUIT");
}
} // namespace

// Reimplements 0x414b60: HudUiMainMenuDialog::CanLoadGame
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
RECOIL_NOINLINE int RECOIL_CDECL HudUiMainMenuDialog::CanLoadGame()
{
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
RECOIL_NOINLINE int RECOIL_CDECL HudUiMainMenuDialog::CanSaveGame()
{
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

// Reimplements 0x414bc0: HudUiMainMenuDialog::Constructor
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
HudUiMainMenuDialog *RECOIL_THISCALL
HudUiMainMenuDialog::Constructor(RecoilMainMenuEntryRoute route)
{
    base.Constructor();

    creditsButton.Constructor();
    creditsButton.base.ftable = &g_HudUiMainMenu_CreditsButton_FTable;
    backButton.Constructor();
    backButton.base.ftable = &g_HudUiMainMenu_BackButton_FTable;
    saveGameButton.Constructor();
    saveGameButton.base.ftable = &g_HudUiMainMenu_SaveGameButton_FTable;
    loadGameButton.Constructor();
    loadGameButton.base.ftable = &g_HudUiMainMenu_LoadGameButton_FTable;
    newGameButton.Constructor();
    newGameButton.base.ftable = &g_HudUiMainMenu_NewGameButton_FTable;
    optionsButton.Constructor();
    optionsButton.base.ftable = &g_HudUiMainMenu_OptionsButton_FTable;
    quitButton.Constructor();
    quitButton.base.ftable = &g_HudUiMainMenu_QuitButton_FTable;
    controlsButton.Constructor();
    controlsButton.base.ftable = &g_HudUiMainMenu_ControlsButton_FTable;

    base.base.base.vptr = (const HudUiContainer_FTable *)&g_HudUiMainMenuDialog_FTable;

    if (zOpt::GetNetworkEnabled() != 0) {
        zReader::Node *const loadedSection = base.LoadFromZrd("dialog.zrd", "MAINMENU2", 0);
        if (loadedSection != 0) {
            BindButton(this, loadedSection, &optionsButton, "OPTIONS");
            BindButton(this, loadedSection, &controlsButton, "CONTROLS");
            BindButton(this, loadedSection, &creditsButton, "CREDITS");
            BindButton(this, loadedSection, &backButton, "BACK");
            BindButton(this, loadedSection, &quitButton, "QUIT");
            base.FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
        }
        return this;
    }

    if (route != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        zUtil_PlayerStateStorage *const playerState =
            (zUtil_PlayerStateStorage *)g_GameStateOrMapTable->playerState;
        if (playerState->lifecycleState == 4) {
            zReader::Node *const loadedSection = base.LoadFromZrd("dialog.zrd", "MAINMENU3", 0);
            if (loadedSection != 0) {
                BindButton(this, loadedSection, &newGameButton, "NEWGAME");
                BindButton(this, loadedSection, &loadGameButton, "LOADGAME");
                BindButton(this, loadedSection, &quitButton, "QUIT");
                base.FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
            }
        } else {
            zReader::Node *const loadedSection = base.LoadFromZrd("dialog.zrd", "MAINMENU1", 0);
            if (loadedSection != 0) {
                BindButton(this, loadedSection, &newGameButton, "NEWGAME");
                BindButton(this, loadedSection, &saveGameButton, "SAVEGAME");
                BindButton(this, loadedSection, &loadGameButton, "LOADGAME");
                BindButton(this, loadedSection, &optionsButton, "OPTIONS");
                BindButton(this, loadedSection, &controlsButton, "CONTROLS");
                BindButton(this, loadedSection, &creditsButton, "CREDITS");
                BindButton(this, loadedSection, &backButton, "BACK");
                BindButton(this, loadedSection, &quitButton, "QUIT");
                base.FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
            }
        }

        saveGameButton.modeOrEnabled = CanSaveGame();
        saveGameButton.RefreshState();
        loadGameButton.modeOrEnabled = CanLoadGame();
        loadGameButton.RefreshState();
        return this;
    }

    zReader::Node *const loadedSection = base.LoadFromZrd("dialog.zrd", "MAINMENU0", 0);
    if (loadedSection != 0) {
        BindButton(this, loadedSection, &newGameButton, "NEWGAME");
        BindButton(this, loadedSection, &loadGameButton, "LOADGAME");
        BindButton(this, loadedSection, &optionsButton, "OPTIONS");
        BindButton(this, loadedSection, &controlsButton, "CONTROLS");
        BindButton(this, loadedSection, &creditsButton, "CREDITS");
        BindButton(this, loadedSection, &quitButton, "QUIT");
        base.FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    loadGameButton.modeOrEnabled = CanLoadGame();
    loadGameButton.RefreshState();
    return this;
}

// Reimplements 0x415040: HudUiMainMenuDialog::Destructor
// (D:\Proj\Battlesport\HudUiMainMenuDialog.cpp)
void RECOIL_THISCALL HudUiMainMenuDialog::Destructor()
{
    controlsButton.DestructorCore();
    quitButton.DestructorCore();
    optionsButton.DestructorCore();
    newGameButton.DestructorCore();
    loadGameButton.DestructorCore();
    saveGameButton.DestructorCore();
    backButton.DestructorCore();
    creditsButton.DestructorCore();
    base.Destructor();
}

void RECOIL_THISCALL HudUiMainMenuDialog::Update(float deltaSeconds)
{
    base.Update(deltaSeconds);
}

void RECOIL_THISCALL HudUiMainMenuDialog::SetEnabled(int enabled)
{
    base.SetEnabled(enabled);
}

HudUiMainMenuDialog *RECOIL_THISCALL
HudUiMainMenuDialog::ScalarDeletingDestructor(unsigned int flags)
{
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}
