// Battlesport compilation unit between RecoilNet.cpp and turret.cpp, inferred
// from the retail object boundary [0x434660, 0x436630): its .rdata constants
// [0x4d1710, 0x4d1728) repeat the double 0.0 and -5.0f that RecoilApp.cpp pools
// at 0x4d09b8 and 0x4d09c0, its sort-helper instantiations close the object at
// [0x435fd0, 0x436630), and its .data [0x4dcfe4, 0x4dd080), .bss
// [0x4f3fb0, 0x4f3fcc) and CRT initializer 0x435a30 are its own. Original
// filename unresolved; saveload.cpp is a provisional name (2026-10-02).

#include "Battlesport/recoil_app.h"

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/briefing.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "Battlesport/recoil_version.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "opt_catalog.h"
#include "pickup.h"
#include "zimage.h"

#include <new>

#ifndef SPI_SETSCREENSAVERRUNNING
#define SPI_SETSCREENSAVERRUNNING 0x0061
#endif

#ifdef FormatMessage
#undef FormatMessage
#endif

#include <direct.h>
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#endif
#include <deque>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace {
/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.k-savegamenameallowedchars
 * @recoil-artifact defines .rdata recoil:data:0x4d1598: k_SaveGameNameAllowedChars.
 * Purpose: save-game name raw-key allowlist consumed by HudUiSaveLoadGameNameInput::OnRawKeyboardEvent.
 */
const char k_SaveGameNameAllowedChars[]
    = "abcdefghijklmnopqrstuvwxyzABCDEFGHIKJKLMNOPQRSTUVWXYZ0123456789_ \x1b\r\x08\x7f\x02\x06";
RECOIL_STATIC_ASSERT(sizeof(k_SaveGameNameAllowedChars) == 0x48);

/**
 * Original helper: source-local with no standalone retail function address.
 * Purpose: casts an option payload pointer to the view-rectangle section type.
 */
inline zOpt_ViewRectSection* ViewRectFromPtr(void* ptr)
{
    return (zOpt_ViewRectSection*)ptr;
}

/**
 * Evidence: this source-local Win32 resource conversion emits no standalone retail function.
 * Purpose: forms a Win32 integer resource pointer from a numeric identifier.
 */
inline LPCSTR IntResource(unsigned int value)
{
    return (LPCSTR)(value);
}

/**
 * Original inline helper with no standalone retail function address.
 * Purpose: returns the nullable save/load entry count for dialog navigation.
 */
inline int SaveLoadEntryCount(const HudUiSaveLoadDialog* dialog)
{
    return (int)dialog->fileEntries.size();
}

} // namespace

/**
 *
 * Purpose: stores the zero-initialized singleton save/load app-state
 * transition object; retail evidence models this as the complete 0x1c-byte
 * owner data object for RecoilStateSaveLoadTransition. Explicit storage keeps
 * the original symbol and leaves construction to the recovered lifecycle
 * helpers instead of compiler-generated automatic startup thunks.
 */
#undef g_RecoilStateSaveLoadTransition
RecoilStateSaveLoadTransitionStorage g_RecoilStateSaveLoadTransition = { 0 };
#define g_RecoilStateSaveLoadTransition (*(RecoilStateSaveLoadTransition*)&g_RecoilStateSaveLoadTransition)

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void(__cdecl* RecoilStateSaveLoadTransitionCrtInitializerFn)();
#pragma data_seg(".CRT$XCU")
static RecoilStateSaveLoadTransitionCrtInitializerFn s_RecoilStateSaveLoadTransitionCrtInit
    = RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * operator<(HudUiSaveLoadEntry const &, HudUiSaveLoadEntry const &).
 * Purpose: Orders save-game file entries by most recent write time.
 */
int __fastcall operator<(const HudUiSaveLoadEntry& lhs, const HudUiSaveLoadEntry& rhs)
{
    return CompareFileTime(&lhs.ftLastWriteTime, &rhs.ftLastWriteTime) > 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-game-dialog-hud-ui-save-game-dialog
 * @recoil-artifact defines .text recoil:function:0x434680: HudUiSaveGameDialog::HudUiSaveGameDialog.
 * @recoil-match byte
 *
 * Purpose: Builds the save-game dialog controls from dialog.zrd and initializes list contents.
 */
HudUiSaveGameDialog::HudUiSaveGameDialog()
{
    zReader::Node* const loadedSection = LoadFromZrd("dialog.zrd", "SAVE_GAME_DIALOG", 0);
    if (loadedSection != 0) {
        BindWidgetByName(loadedSection, &backButton, "BACK");
        BindWidgetByName(loadedSection, &nextEntryButton, "NEXT_GAME_BTN");
        BindWidgetByName(loadedSection, &prevEntryButton, "PREV_GAME_BTN");
        BindWidgetByName(loadedSection, &deleteButton, "DELETE_BTN");
        BindWidgetByName(loadedSection, &primaryActionButton, "SAVE");
        BindWidgetByName(loadedSection, &gameNameInput, "GAMENAME");

        char listNodeName[32];
        for (int i = 0; i < 9; ++i) {
            sprintf(listNodeName, "LIST_%d", i);
            BindPrimitiveNodeToElement(loadedSection, &entryWidgets[i], listNodeName);
        }

        FreeLoadedTreeRoots((int)(unsigned int)(loadedSection));
    }

    InitializeFileEntries();
    SetSelectedEntryIndex(-1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-game-name-input-on-activate
 * @recoil-artifact defines .text recoil:function:0x4348b0: HudUiSaveLoadGameNameInput::OnActivate.
 * @recoil-match byte
 *
 * Purpose: Activates the save-game name input and moves the cursor to the end.
 */
void HudUiSaveLoadGameNameInput::OnActivate()
{
    Update(GetBuffer());
    textInput.SetCursorPosition((int)(strlen(GetBuffer())));
    HudUiNumericTextInput::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-game-name-input-on-raw-keyboard-char
 * @recoil-artifact defines .text recoil:function:0x4348f0: HudUiSaveLoadGameNameInput::OnRawKeyboardChar.
 * @recoil-match byte
 *
 * Purpose: Filters raw key input to the save-game filename character set.
 */
int HudUiSaveLoadGameNameInput::OnRawKeyboardChar(int key)
{
    if (strchr(k_SaveGameNameAllowedChars, key) != 0) {
        textInput.DispatchKeyAction(key);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-list-item-hud-ui-save-load-list-item
 * @recoil-artifact defines .text recoil:function:0x434920: HudUiSaveLoadListItem::HudUiSaveLoadListItem.
 * @recoil-match byte
 *
 * Purpose: Initializes a save/load list row panel and clears its entry index.
 */
HudUiSaveLoadListItem::HudUiSaveLoadListItem()
    : HudUiPanel(0, 0, 0)
{
    layoutY = 32767;
    layoutX = -1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-list-item-draw
 * @recoil-artifact defines .text recoil:function:0x434950: HudUiSaveLoadListItem::Draw.
 * @recoil-match byte
 *
 * Purpose: Draws the list row panel and refreshes text bounds after rendering.
 */
void HudUiSaveLoadListItem::Draw()
{
    HudUiPanel::Draw();
    UpdateTextBoundsFromContent();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-game-dialog-on-primary-action-thunk
 * @recoil-artifact defines .text recoil:function:0x434970: HudUiSaveGameDialog::OnPrimaryActionThunk.
 * @recoil-match byte
 *
 * Purpose: Dispatches the save dialog primary action to its nonvirtual result handler.
 */
void HudUiSaveGameDialog::OnPrimaryActionThunk()
{
    ProcessDialogResult();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-load-game-dialog-hud-ui-load-game-dialog
 * @recoil-artifact defines .text recoil:function:0x434b90: HudUiLoadGameDialog::HudUiLoadGameDialog.
 * @recoil-match byte
 *
 * Purpose: Builds the load-game dialog controls from dialog.zrd and initializes list contents.
 */
HudUiLoadGameDialog::HudUiLoadGameDialog()
{
    zReader::Node* const loadedSection = LoadFromZrd("dialog.zrd", "LOAD_GAME_DIALOG", 0);
    if (loadedSection != 0) {
        BindWidgetByName(loadedSection, &backButton, "BACK");
        BindWidgetByName(loadedSection, &nextEntryButton, "NEXT_GAME_BTN");
        BindWidgetByName(loadedSection, &prevEntryButton, "PREV_GAME_BTN");
        BindWidgetByName(loadedSection, &deleteButton, "DELETE_BTN");
        BindWidgetByName(loadedSection, &primaryActionButton, "LOAD");
        BindWidgetByName(loadedSection, &gameNameInput, "GAMENAME");

        char listNodeName[32];
        for (int i = 0; i < 9; ++i) {
            sprintf(listNodeName, "LIST_%d", i);
            BindPrimitiveNodeToElement(loadedSection, &entryWidgets[i], listNodeName);
        }

        FreeLoadedTreeRoots((int)(unsigned int)(loadedSection));
    }

    InitializeFileEntries();
    SetSelectedEntryIndex(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-load-game-dialog-on-primary-action-thunk
 * @recoil-artifact defines .text recoil:function:0x434dc0: HudUiLoadGameDialog::OnPrimaryActionThunk.
 * @recoil-match byte
 *
 * Purpose: Dispatches the load dialog primary action through the concrete dialog object.
 */
void HudUiLoadGameDialog::OnPrimaryActionThunk()
{
    ProcessDialogResult();
}

namespace {
/**
 * Reconstruction model: a TU-local inline row setter. Retail 0x434ee0 keeps the
 * nine layoutY stores in row order (0x434ef6..0x434f30), holding the shared 0x3fff
 * value in ecx until the eighth store; VC5 keeps that order only when each store goes
 * through an inline-expanded row pointer (direct member stores hoist both 0x3fff
 * stores). Original-source helper status is inferred; helper spelling and placement are not established; no standalone
 * retail function exists. Purpose: set one save/load list row's layout Y value.
 */
inline void SetRowLayoutY(HudUiSaveLoadListItem* item, int layoutY)
{
    item->layoutY = layoutY;
}
} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-dialog-initialize-file-entries
 * @recoil-artifact defines .text recoil:function:0x434ee0: HudUiSaveLoadDialog::InitializeFileEntries.
 * @recoil-match byte
 *
 * Purpose: Seeds list-row layout metadata, loads saved-game entries, and binds visible rows.
 */
void HudUiSaveLoadDialog::InitializeFileEntries()
{
    SetRowLayoutY(&entryWidgets[0], (int)(0.3 * 0x7fff));
    SetRowLayoutY(&entryWidgets[1], (int)(0.5 * 0x7fff));
    SetRowLayoutY(&entryWidgets[2], (int)(1.0 * 0x7fff));
    SetRowLayoutY(&entryWidgets[3], (int)(1.0 * 0x7fff));
    SetRowLayoutY(&entryWidgets[4], (int)(1.0 * 0x7fff));
    SetRowLayoutY(&entryWidgets[5], (int)(0.9 * 0x7fff));
    SetRowLayoutY(&entryWidgets[6], (int)(0.7 * 0x7fff));
    SetRowLayoutY(&entryWidgets[7], (int)(0.5 * 0x7fff));
    SetRowLayoutY(&entryWidgets[8], (int)(0.3 * 0x7fff));

    RefreshSaveFileList();

    int index = 0;
    HudUiSaveLoadEntry* entry = fileEntries.begin();
    while (entry != fileEntries.end() && index < 9) {
        entryWidgets[index].layoutX = index;
        entryWidgets[index].SetTextFmt("%s", entry->cFileName);
        entryWidgets[index].SetVisible(1);

        ++entry;
        ++index;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-dialog-delete-save-file
 * @recoil-artifact defines .text recoil:function:0x434fb0: HudUiSaveLoadDialog::DeleteSaveFile.
 * @recoil-match byte
 *
 * Purpose: Deletes the selected saved-game file and refreshes the dialog list.
 */
void HudUiSaveLoadDialog::DeleteSaveFile(int confirmDelete)
{
    int shouldDelete = 1;
    char* const gameName = gameNameInput.GetBuffer();
    if (gameName == 0 || gameName[0] == '\0') {
        return;
    }

    _mkdir("SavedGames");

    char saveGamePath[MAX_PATH];
    sprintf(saveGamePath, "SavedGames\\%s", gameName);
    if (zReader::FileExists(saveGamePath) == 0) {
        return;
    }

    if (confirmDelete != 0) {
        char titleText[128];
        char messageText[128];
        strcpy(titleText, zLoc::GetMessageString(138));
        strcpy(messageText, zLoc::GetMessageString(139));
        shouldDelete = HudUi::ShowMessageBox(messageText, titleText, (void*)1) == 1 ? 1 : 0;
    }

    if (shouldDelete == 0) {
        return;
    }

    remove(saveGamePath);
    gameNameInput.Update("");
    RefreshSaveFileList();

    SetSelectedEntryIndex(
        (unsigned int)(selectedEntryIndex) < (unsigned int)(SaveLoadEntryCount(this) - 1) ? selectedEntryIndex
                                                                                          : SaveLoadEntryCount(this) - 1
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-delete-button-on-activate
 * @recoil-artifact defines .text recoil:function:0x435140: HudUiSaveLoadDeleteButton::OnActivate.
 * @recoil-match byte
 *
 * Purpose: Runs widget activation behavior and asks the dialog to delete the selected file.
 */
void HudUiSaveLoadDeleteButton::OnActivate()
{
    HudUiSaveLoadDialog* const dialog = (HudUiSaveLoadDialog*)(owner);
    HudUiZrdWidget::OnActivate();
    dialog->DeleteSaveFile(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-next-button-on-activate
 * @recoil-artifact defines .text recoil:function:0x435160: HudUiSaveLoadNextButton::OnActivate.
 * @recoil-match byte
 *
 * Purpose: Advances the selected save/load entry when another entry exists.
 */
void HudUiSaveLoadNextButton::OnActivate()
{
    HudUiSaveLoadDialog* const dialog = (HudUiSaveLoadDialog*)(owner);
    HudUiZrdWidget::OnActivate();

    const int entryCount = SaveLoadEntryCount(dialog);
    const int nextEntryIndex = dialog->selectedEntryIndex + 1;
    if (nextEntryIndex >= 0 && nextEntryIndex < entryCount) {
        dialog->SetSelectedEntryIndex(nextEntryIndex);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-prev-button-on-activate
 * @recoil-artifact defines .text recoil:function:0x4351b0: HudUiSaveLoadPrevButton::OnActivate.
 * @recoil-match byte
 *
 * Purpose: Moves the selected save/load entry to the previous valid row.
 */
void HudUiSaveLoadPrevButton::OnActivate()
{
    HudUiSaveLoadDialog* const dialog = (HudUiSaveLoadDialog*)(owner);
    HudUiZrdWidget::OnActivate();

    const int entryCount = SaveLoadEntryCount(dialog);
    const int prevEntryIndex = dialog->selectedEntryIndex - 1;
    if (prevEntryIndex >= 0 && prevEntryIndex < entryCount) {
        dialog->SetSelectedEntryIndex(prevEntryIndex);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-game-primary-action-button-on-activate
 * @recoil-artifact defines .text recoil:function:0x435200: HudUiSaveGamePrimaryActionButton::OnActivate.
 * @recoil-match byte
 *
 * Purpose: Commits the save-game dialog result before running the widget activation path.
 */
void HudUiSaveGamePrimaryActionButton::OnActivate()
{
    HudUiSaveGameDialog* const dialog = (HudUiSaveGameDialog*)(owner);
    if (dialog != 0) {
        dialog->ProcessDialogResult();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-load-game-primary-action-button-on-activate
 * @recoil-artifact defines .text recoil:function:0x435220: HudUiLoadGamePrimaryActionButton::OnActivate.
 * @recoil-match byte
 *
 * Purpose: Commits the load-game dialog result before running the widget activation path.
 */
void HudUiLoadGamePrimaryActionButton::OnActivate()
{
    HudUiLoadGameDialog* const dialog = (HudUiLoadGameDialog*)(owner);
    if (dialog != 0) {
        dialog->ProcessDialogResult();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-game-dialog-process-dialog-result
 * @recoil-artifact defines .text recoil:function:0x435240: HudUiSaveGameDialog::ProcessDialogResult.
 * @recoil-match byte
 *
 * Purpose: Saves to the selected file path through the global archive entry path and exits the dialog.
 */
void HudUiSaveGameDialog::ProcessDialogResult()
{
    char* const gameName = gameNameInput.GetBuffer();
    if (gameName == 0 || gameName[0] == '\0') {
        g_RecoilApp.QueueExitCurrentState(0);
        return;
    }

    _mkdir("SavedGames");

    char saveGamePath[MAX_PATH];
    char titleText[128];
    char messageText[128];
    sprintf(saveGamePath, "SavedGames\\%s", gameName);
    if (zReader::FileExists(saveGamePath) != 0) {
        strcpy(titleText, zLoc::GetMessageString(136));
        strcpy(messageText, zLoc::GetMessageString(137));
        if (HudUi::ShowMessageBox(messageText, titleText, (void*)1) == 2) {
            return;
        }
    }

    while (zUtil::zZarWriteFileGlobal(saveGamePath) == 0) {
        DeleteSaveFile(0);

        strcpy(titleText, zLoc::GetMessageString(136));
        strcpy(messageText, zLoc::GetMessageString(140));
        if (HudUi::ShowMessageBox(messageText, titleText, (void*)1) == 2) {
            break;
        }
    }

    g_RecoilApp.QueueExitCurrentState(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-game-name-input-on-accept
 * @recoil-artifact defines .text recoil:function:0x4353e0: HudUiSaveLoadGameNameInput::OnAccept.
 * @recoil-match byte
 *
 * Purpose: commit the current save-game name through the owning dialog.
 */
void HudUiSaveLoadGameNameInput::OnAccept()
{
    HudUiSaveLoadDialog* const dialog = (HudUiSaveLoadDialog*)(owner);
    dialog->OnPrimaryActionThunk();
}

/**
 * Purpose: Updates the selected save/load entry and repopulates visible list rows around it.
 */
void HudUiSaveLoadDialog::SetSelectedEntryIndex(int selectedEntryIndexValue)
{
    selectedEntryIndex = selectedEntryIndexValue;

    for (int row = 0; row < 3; ++row) {
        const int entryIndex = selectedEntryIndexValue + row - 3;
        HudUiSaveLoadListItem* listItem = &entryWidgets[row];
        if (entryIndex >= 0) {
            if ((unsigned int)entryIndex < fileEntries.size()) {
                HudUiSaveLoadEntry& entry = fileEntries[entryIndex];
                entryWidgets[row].layoutX = entryIndex;
                listItem->SetTextFmt("%s", entry.cFileName);
                listItem->SetVisible(1);
                listItem->Invalidate();
                continue;
            }
        }

        listItem->SetVisible(0);
    }

    if (selectedEntryIndexValue >= 0) {
        if ((unsigned int)selectedEntryIndexValue < fileEntries.size()) {
            gameNameInput.Update(fileEntries[selectedEntryIndexValue].cFileName);
        }
    }

    for (int lowerRow = 3; lowerRow < 9; ++lowerRow) {
        const int entryIndex = selectedEntryIndexValue + lowerRow - 2;
        HudUiSaveLoadListItem* listItem = &entryWidgets[lowerRow];
        if (entryIndex >= 0) {
            if ((unsigned int)entryIndex < fileEntries.size()) {
                HudUiSaveLoadEntry& entry = fileEntries[entryIndex];
                entryWidgets[lowerRow].layoutX = entryIndex;
                listItem->SetTextFmt("%s", entry.cFileName);
                listItem->SetVisible(1);
                listItem->Invalidate();
                continue;
            }
        }

        listItem->SetVisible(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-dialog-refresh-save-file-list
 * @recoil-artifact defines .text recoil:function:0x4355e0: HudUiSaveLoadDialog::RefreshSaveFileList.
 * @recoil-match source
 *
 * Purpose: Rebuilds and sorts the saved-game file entry vector from the SavedGames directory.
 */
void HudUiSaveLoadDialog::RefreshSaveFileList()
{
    HudUiSaveLoadEntries* entries = &fileEntries;
    entries->clear();

    HudUiSaveLoadEntry findData;
    HANDLE findHandle = FindFirstFileA("SavedGames\\*.*", &findData);
    if (findHandle != INVALID_HANDLE_VALUE) {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            entries->push_back(findData);
        }

        while (FindNextFileA(findHandle, &findData) != 0) {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                entries->push_back(findData);
            }
        }
    }

    std::sort(fileEntries.begin(), fileEntries.end());
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-save-load-list-item-on-activate
 * @recoil-artifact defines .text recoil:function:0x435a10: HudUiSaveLoadListItem::OnActivate.
 * @recoil-match byte
 *
 * Purpose: Selects this row's save/load entry in its parent dialog.
 */
void HudUiSaveLoadListItem::OnActivate()
{
    HudUiSaveLoadDialog* const owner = (HudUiSaveLoadDialog*)(parent);
    if (owner != 0) {
        owner->SetSelectedEntryIndex(layoutX);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.recoil-state-save-load-transition-static-init-and-register-at-exit
 * @recoil-artifact defines .text recoil:function:0x435a30: RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit.
 * @recoil-match byte
 *
 * Purpose: Initializes the save/load transition singleton and registers its exit cleanup.
 */
void __cdecl RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit()
{
    StaticInit();
    RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-state-save-load-transition-static-init
 * @recoil-artifact defines .text recoil:function:0x435a40: RecoilStateSaveLoadTransition::StaticInit.
 * @recoil-match byte
 *
 * Purpose: Constructs the global save/load transition object.
 */
void __cdecl RecoilStateSaveLoadTransition::StaticInit()
{
    g_RecoilStateSaveLoadTransition.RecoilStateSaveLoadTransition::RecoilStateSaveLoadTransition();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.recoil-state-save-load-transition-register-at-exit
 * @recoil-artifact defines .text recoil:function:0x435a50: RecoilStateSaveLoadTransition::RegisterAtExit.
 * @recoil-match byte
 *
 * Purpose: Registers the save/load transition singleton destructor with atexit.
 */
void __cdecl RecoilStateSaveLoadTransition::RegisterAtExit()
{
    atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.recoil-state-save-load-transition-at-exit-destructor
 * @recoil-artifact defines .text recoil:function:0x435a60: RecoilStateSaveLoadTransition::AtExitDestructor.
 * @recoil-match byte
 *
 * Purpose: Tears down the global save/load transition during process exit.
 */
void __cdecl RecoilStateSaveLoadTransition::AtExitDestructor()
{
    g_RecoilStateSaveLoadTransition.RecoilStateSaveLoadTransition::~RecoilStateSaveLoadTransition();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.hud-ui-load-game-dialog-process-dialog-result
 * @recoil-artifact defines .text recoil:function:0x435a70: HudUiLoadGameDialog::ProcessDialogResult.
 * @recoil-match byte
 *
 * Purpose: Loads the selected saved game and queues the appropriate game-state transition.
 */
void HudUiLoadGameDialog::ProcessDialogResult()
{
    char* const gameName = gameNameInput.GetBuffer();
    char saveGamePath[MAX_PATH];
    saveGamePath[0] = '\0';

    if (gameName == 0 || gameName[0] == '\0') {
        return;
    }

    sprintf(saveGamePath, "SavedGames\\%s", gameName);
    if (zReader::FileExists(saveGamePath) == 0) {
        return;
    }

    if (zUtil::zZarLoadFileGlobal(saveGamePath) == 0) {
        return;
    }

    RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();
    zSndPlayHandleSnapshotList* const snapshot
        = (zSndPlayHandleSnapshotList*)((unsigned int)(g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot));
    if (snapshot != 0) {
        zSnd::DestroySnapshot(snapshot);
        g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot = 0;
    }

    zInp::SetJoystickOption(zInput::DISetJoystickEnabled(zInp::GetJoystickOption()));
    zOpt::SetCursorMode(zOpt::GetCursorMode());
    zOpt::SetCameraMode(zOpt::GetCameraModePlayerState());
    zOpt::SetThrottleMode(zOpt::GetThrottleMode());
    zOpt::SetSteeringMode(zOpt::GetSteeringMode());

    switch (g_RecoilStateSaveLoadTransition.m_transitionMode) {
    case RECOIL_SAVELOAD_MODE_STANDARD:
        if (saveGamePath[0] != '\0') {
            g_RecoilApp.m_playState.pPendingLoadGameStartPath = _strdup(saveGamePath);
            g_RecoilApp.m_missionFmvState.m_skipMissionFmv = 1;
            g_RecoilApp.QueueExitCurrentState(1);
            g_RecoilApp.QueueSwitchCurrentState(&g_RecoilApp.m_missionFmvState, 0);
        } else {
            g_RecoilApp.QueueExitCurrentState(0);
        }
        break;

    case RECOIL_SAVELOAD_MODE_FADE:
        if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
            g_RecoilApp.m_transitionFadeTimer += 5.0f;
        } else {
            g_RecoilApp.m_transitionFadeTimer = 5.0f;
            zOpt::SetMuteSoundOption(1);
        }
        g_RecoilApp.QueueExitCurrentState(1);
        g_RecoilApp.QueueExitCurrentState(1);
        break;

    case RECOIL_SAVELOAD_MODE_QUICKLOAD:
        if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
            g_RecoilApp.m_transitionFadeTimer += 5.0f;
        } else {
            g_RecoilApp.m_transitionFadeTimer = 5.0f;
            zOpt::SetMuteSoundOption(1);
        }
        g_RecoilApp.QueueExitCurrentState(0);
        break;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.saveload-transition-constructor
 * @recoil-artifact defines .text recoil:function:0x435c80: Save/load state construction.
 * @recoil-artifact emits .rdata recoil:data:0x4d1728: Compiler-generated state dispatch table.
 * @recoil-match byte
 *
 * Purpose: Construct the complete polymorphic save/load state, including its
 * dispatch table, before the application can queue its entry callback.
 */
RecoilStateSaveLoadTransition::RecoilStateSaveLoadTransition()
{
    m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    m_dialog = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.saveload-transition-destructor
 * @recoil-artifact defines .text recoil:function:0x435cc0: Save/load state destruction.
 * @recoil-match byte
 *
 * Purpose: Delete the active save or load dialog before inherited state cleanup.
 */
RecoilStateSaveLoadTransition::~RecoilStateSaveLoadTransition()
{
    HudUiSaveLoadDialog* dialog = (HudUiSaveLoadDialog*)m_dialog;
    if (dialog != 0) {
        delete dialog;
        m_dialog = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.recoil-state-save-load-transition-on-try-become-current
 * @recoil-artifact defines .text recoil:function:0x435d20: RecoilStateSaveLoadTransition::OnTryBecomeCurrent.
 * @recoil-match byte
 *
 * Purpose: Captures presentation/audio state and opens the requested save/load dialog.
 */
int RecoilStateSaveLoadTransition::OnTryBecomeCurrent()
{
    if (m_capturePresentationMode != RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED) {
        if (g_zVideo_ActiveRendererPath != 0) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(0, 0);
        }

        m_savedHalfResAdjustMode
            = (zVideoHalfResAdjustMode)zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
        HudUi::SetInvalidateMode(0);
        zSnd::ApplyMuteStateToActiveVoices(1);

        zSndPlayHandleSnapshotList* const audioSnapshot = zSnd::CreateSnapshotFromActiveSamples();
        m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
        zSnd::StopSnapshotVoicesIfPlaying(audioSnapshot);

        zFMV_ActionBlur blurAction(4, 1);
        zFMV_Action* const action = &blurAction;
        action->Begin(0.0);
        while (action->Update(0.0) != 0) { }
        action->End();

        zSndSampleSetInitByName("DIALOG");
    }

    if (m_dialogKind != RECOIL_SAVELOAD_DIALOG_LOAD) {
        m_dialog = new HudUiSaveGameDialog;
    } else {
        m_dialog = new HudUiLoadGameDialog;
    }

    m_dialog->SetEnabled(1);
    return 1;
}

#include "Battlesport/recoil_state_main_menu_transition.h"

#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid.h"

/**
 * (BN canonical folded body).
 *
 * Source owner: app_shell.folded_dialog_update_should_quit. BN shows the
 * retail body shared by DialogHost, MainMenuTransition, SaveLoadTransition,
 * and other dialog-hosted state vtable slots; this definition preserves the
 * MainMenuTransition typed participant.
 *
 * Original-source function evidence: folded retail body 0x435e80.
 * Purpose: update and present the active main-menu dialog each frame while the
 * transition state is current.
 */
int RecoilStateMainMenuTransition::OnUpdateShouldQuit()
{
    zInput::PollActiveDevices(0);

    if (m_mainMenuDialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        ((HudUiContainer*)m_mainMenuDialog)->UpdateAll(g_FrameDeltaTimeSec);

        zVideo::DispatchUnlockPrimarySurfaceState();
    }

    zVideo::AdjustSurfacesIfEnabled((zVidRect32*)zOpt::GetWindowSection(), (zVidRect32*)zOpt::GetWindowSection(), 1, 1);
    return 0;
}

/**
 * (BN canonical folded body).
 * BN source-owner evidence for the SaveLoadTransition participant shows the
 * retail body shared by DialogHost, MainMenuTransition, SaveLoadTransition,
 * and other dialog-hosted state vtable slots; this definition preserves the
 * SaveLoadTransition typed participant.
 * The original-source function evidence is the folded retail body at 0x435e80.
 * Purpose: Updates the active save/load dialog and reports whether the transition should quit.
 */
int RecoilStateSaveLoadTransition::OnUpdateShouldQuit()
{
    zInput::PollActiveDevices(0);

    if (m_dialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        ((HudUiSaveLoadDialog*)((unsigned int)m_dialog))->UpdateAll(g_FrameDeltaTimeSec);

        zVideo::DispatchUnlockPrimarySurfaceState();
    }

    zOpt_ViewRectSection* const dstRect = zOpt::GetWindowSection();
    zOpt_ViewRectSection* const srcRect = zOpt::GetWindowSection();
    zVideo::AdjustSurfacesIfEnabled((zVidRect32*)srcRect, (zVidRect32*)dstRect, 1, 1);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.recoil-state-save-load-transition-on-deactivate
 * @recoil-artifact defines .text recoil:function:0x435ed0: RecoilStateSaveLoadTransition::OnDeactivate.
 * @recoil-match byte
 *
 * Purpose: Restores captured presentation/audio state and deletes the active save/load dialog.
 */
void RecoilStateSaveLoadTransition::OnDeactivate()
{
    if (m_dialog != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();

        HudUiSaveLoadDialog* dialog = (HudUiSaveLoadDialog*)((unsigned int)m_dialog);
        dialog->SetEnabled(0);

        ((HudUiDialogController*)((unsigned int)m_dialog))->BlitOwnedSurfaceToPrimary();
        zVideo::DispatchUnlockPrimarySurfaceState();

        dialog = (HudUiSaveLoadDialog*)((unsigned int)m_dialog);
        delete dialog;

        m_dialog = 0;
    }

    if (m_capturePresentationMode == RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED) {
        return;
    }

    zSndSampleSetDestroyByName("DIALOG");

    zSndPlayHandleSnapshotList* const audioSnapshot
        = (zSndPlayHandleSnapshotList*)((unsigned int)m_pausedAudioSnapshot);
    if (audioSnapshot != 0) {
        zSnd::RestoreSnapshotWithGlobalVolumeDelta(audioSnapshot);
    }

    zSnd::ApplyMuteStateToActiveVoices(0);
    zVideo::SetHalfResAdjustMode(m_savedHalfResAdjustMode);
    HudUi::SetInvalidateMode(m_savedHalfResAdjustMode);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.recoil-state-save-load-transition-queue-open-save-dialog
 * @recoil-artifact defines .text recoil:function:0x435f50: RecoilStateSaveLoadTransition::QueueOpenSaveDialog.
 * @recoil-match byte
 *
 * Purpose: Configures and queues the save-dialog transition.
 */
void __fastcall RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
    RecoilSaveLoadPresentationCaptureMode capturePresentationMode
)
{
    if (HudUiMainMenuDialog::CanSaveGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_capturePresentationMode = capturePresentationMode;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    g_RecoilApp.QueuePushState(&g_RecoilStateSaveLoadTransition, 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.saveload.recoil-state-save-load-transition-queue-open-load-dialog
 * @recoil-artifact defines .text recoil:function:0x435f80: RecoilStateSaveLoadTransition::QueueOpenLoadDialog.
 * @recoil-match byte
 *
 * Purpose: Configures and queues the load-dialog transition.
 */
void __fastcall RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RecoilSaveLoadTransitionMode transitionMode)
{
    if (HudUiMainMenuDialog::CanLoadGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_transitionMode = transitionMode;
    switch (transitionMode) {
    case RECOIL_SAVELOAD_MODE_STANDARD:
        break;
    case RECOIL_SAVELOAD_MODE_QUICKLOAD:
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode = RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED;
        break;
    }

    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    g_RecoilApp.QueuePushState(&g_RecoilStateSaveLoadTransition, 0);
}

/**
 * VC5 C1 draws declarations, labels and temporaries from one translation-unit
 * ID counter, and HudUiSaveLoadDialog::RefreshSaveFileList (0x4355e0) orders
 * code by that counter's parity at code generation. This declaration is never
 * referenced and emits no code, data or symbols. User-authorized exception:
 * match-proofs.md "Per-TU VC5 ID-counter parity exception".
 * Purpose: keep this file's ID-counter parity after shared-header changes.
 */
extern int g_SaveLoadIdCounterAlignment0;
