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
 * Purpose: Activates the save-game name input and moves the cursor to the end.
 */
void HudUiSaveLoadGameNameInput::OnActivate()
{
    Update(GetBuffer());
    textInput.SetCursorPosition((int)(strlen(GetBuffer())));
    HudUiNumericTextInput::OnActivate();
}

/**
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
 * Purpose: Initializes a save/load list row panel and clears its entry index.
 */
HudUiSaveLoadListItem::HudUiSaveLoadListItem()
    : HudUiPanel(0, 0, 0)
{
    layoutY = 32767;
    layoutX = -1;
}

/**
 * Purpose: Draws the list row panel and refreshes text bounds after rendering.
 */
void HudUiSaveLoadListItem::Draw()
{
    HudUiPanel::Draw();
    UpdateTextBoundsFromContent();
}

/**
 * Purpose: Dispatches the save dialog primary action to its nonvirtual result handler.
 */
void HudUiSaveGameDialog::OnPrimaryActionThunk()
{
    ProcessDialogResult();
}

/**
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
 * Purpose: Dispatches the load dialog primary action through the concrete dialog object.
 */
void HudUiLoadGameDialog::OnPrimaryActionThunk()
{
    ProcessDialogResult();
}

/**
 * Purpose: Seeds list-row layout metadata, loads saved-game entries, and binds visible rows.
 */
void HudUiSaveLoadDialog::InitializeFileEntries()
{
    entryWidgets[0].layoutY = 0x2666;
    entryWidgets[1].layoutY = 0x3fff;
    entryWidgets[2].layoutY = 0x7fff;
    entryWidgets[3].layoutY = 0x7fff;
    entryWidgets[4].layoutY = 0x7fff;
    entryWidgets[5].layoutY = 29490;
    entryWidgets[6].layoutY = 22936;
    entryWidgets[7].layoutY = 0x3fff;
    entryWidgets[8].layoutY = 0x2666;

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
 * Purpose: Runs widget activation behavior and asks the dialog to delete the selected file.
 */
void HudUiSaveLoadDeleteButton::OnActivate()
{
    HudUiSaveLoadDialog* const dialog = (HudUiSaveLoadDialog*)(owner);
    HudUiZrdWidget::OnActivate();
    dialog->DeleteSaveFile(1);
}

/**
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

    while (zUtil::ZBDLoadEntriesGlobal(saveGamePath) == 0) {
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
            unsigned int entryCount;
            if (fileEntries.begin() == 0) {
                entryCount = 0;
            } else {
                entryCount = (unsigned int)(fileEntries.end() - fileEntries.begin());
            }

            if ((unsigned int)entryIndex < entryCount) {
                listItem->layoutX = entryIndex;
                listItem->SetTextFmt("%s", fileEntries.begin()[entryIndex].cFileName);
                listItem->SetVisible(1);
                listItem->Invalidate();
                continue;
            }
        }

        listItem->SetVisible(0);
    }

    if (selectedEntryIndexValue >= 0) {
        unsigned int selectedEntryCount;
        if (fileEntries.begin() == 0) {
            selectedEntryCount = 0;
        } else {
            selectedEntryCount = (unsigned int)(fileEntries.end() - fileEntries.begin());
        }

        if ((unsigned int)selectedEntryIndexValue < selectedEntryCount) {
            gameNameInput.Update(fileEntries.begin()[selectedEntryIndexValue].cFileName);
        }
    }

    for (int lowerRow = 3; lowerRow < 9; ++lowerRow) {
        const int entryIndex = selectedEntryIndexValue + lowerRow - 2;
        HudUiSaveLoadListItem* listItem = &entryWidgets[lowerRow];
        if (entryIndex >= 0) {
            unsigned int entryCount;
            if (fileEntries.begin() == 0) {
                entryCount = 0;
            } else {
                entryCount = (unsigned int)(fileEntries.end() - fileEntries.begin());
            }

            if ((unsigned int)entryIndex < entryCount) {
                listItem->layoutX = entryIndex;
                listItem->SetTextFmt("%s", fileEntries.begin()[entryIndex].cFileName);
                listItem->SetVisible(1);
                listItem->Invalidate();
                continue;
            }
        }

        listItem->SetVisible(0);
    }
}

/**
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
 * Purpose: Tears down the global save/load transition during process exit.
 */
void __cdecl RecoilStateSaveLoadTransition::AtExitDestructor()
{
    g_RecoilStateSaveLoadTransition.RecoilStateSaveLoadTransition::~RecoilStateSaveLoadTransition();
}

/**
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
    zSndPlayHandleSnapshot* const snapshot
        = (zSndPlayHandleSnapshot*)((unsigned int)(g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot));
    if (snapshot != 0) {
        snapshot->Destroy();
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

        zSndPlayHandleSnapshot* const audioSnapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
        m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
        audioSnapshot->StopAllIfPlaying();

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

    zSndPlayHandleSnapshot* const audioSnapshot = (zSndPlayHandleSnapshot*)((unsigned int)m_pausedAudioSnapshot);
    if (audioSnapshot != 0) {
        audioSnapshot->RestoreAllWithGlobalVolumeDelta();
    }

    zSnd::ApplyMuteStateToActiveVoices(0);
    zVideo::SetHalfResAdjustMode(m_savedHalfResAdjustMode);
    HudUi::SetInvalidateMode(m_savedHalfResAdjustMode);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

/**
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
