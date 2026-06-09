#include "Battlesport/RecoilApp.h"

#include "Battlesport/Briefing.h"
#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/HudUiNetExitPanel.h"
#include "Battlesport/RecoilVersion.h"
#include "Battlesport/hud.h"
#include "Battlesport/player.h"
#include "GameZRecoil/RecoilApp/RecoilStateMainMenuTransition.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zNetwork/zNetwork.h"
#include "GameZRecoil/zReader/zReader.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zSys/zSys.h"
#include "GameZRecoil/zTurret/zTurret.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zUtil/zZbd.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "GameZRecoil/zWeapon/zWeapon.h"
#include "OptCatalog.h"
#include "pickup.h"
#include "zImage.h"

#include <new>

#ifndef SPI_SETSCREENSAVERRUNNING
#define SPI_SETSCREENSAVERRUNNING 0x0061
#endif

#ifdef FormatMessage
#undef FormatMessage
#endif

#include <direct.h>
#if defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
#ifndef __PLACEMENT_NEW_INLINE
#define __PLACEMENT_NEW_INLINE
#endif
#include <deque>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Provider-boundary accessor for imported MFC42 CWinApp protected members; this does not
// reimplement CWinApp behavior.
class RecoilMfcWinAppAccess : public CWinApp {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMapForRecoilApp();
};

// Source-faithful helper recovered from address-backed callers in this source file.
const AFX_MSGMAP *__stdcall RecoilMfcWinAppAccess::GetMessageMapForRecoilApp() {
    return &CWinApp::messageMap;
}

struct RecoilStateCredits {
    static void QueuePush();
};

AFX_MODULE_STATE *__stdcall AfxGetModuleState();
BOOL __stdcall AfxRegisterClass(WNDCLASSA *wndClass);
HINSTANCE __stdcall AfxFindResourceHandle(
    LPCSTR resourceName,
    LPCSTR resourceType
);

namespace {
enum zVideoRendererBackend {
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};

enum zVideoSoftwareModeHotkeyState {
    ZVIDEO_SOFTWARE_MODE_HOTKEY_DISABLED = 0,
    ZVIDEO_SOFTWARE_MODE_HOTKEY_ENABLED = 1,
};

enum zVideoClearScreenBufferState {
    ZVIDEO_CLEAR_SCREEN_BUFFER_ENABLED = 1,
};

const char k_SaveGameNameAllowedChars[] =
    "abcdefghijklmnopqrstuvwxyzABCDEFGHIKJKLMNOPQRSTUVWXYZ0123456789_ \x1b\r\x08\x7f\x02\x06";
RECOIL_STATIC_ASSERT(sizeof(k_SaveGameNameAllowedChars) == 0x48);

// Source-faithful helper recovered from address-backed callers in this source file.
zOpt_ViewRectSection *ViewRectFromPtr(
    void *ptr
) {
    return (zOpt_ViewRectSection *)ptr;
}

// Source-faithful helper recovered from address-backed callers in this source file.
LPCSTR IntResource(
    unsigned int value
) {
    return (LPCSTR)(value);
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline void ExtendPlayStateTransitionTimer(
    float seconds
) {
    if (g_RecoilApp.m_transitionFadeTimer > 0.0) {
        g_RecoilApp.m_transitionFadeTimer += seconds;
        return;
    }

    g_RecoilApp.m_transitionFadeTimer = seconds;
    zOpt::SetMuteSoundOption(1);
}

// Source-faithful helper recovered from address-backed callers in this source file.
void RunGrandPrizeBlurAction() {
    zFMV_ActionBlur blurAction;
    blurAction.Constructor(
        12,
        1
    );

    zFMV_Action *const action = &blurAction;
    action->Begin(0.0);
    while (action->Update(0.0) != 0) {
    }
    action->End();

    RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
    RecoilStateCredits::QueuePush();
}

// original-source inline helper: retail has no standalone function address, and
// BN callers 0x434fb0, 0x435160, and 0x4351b0 inline this same nullable
// HudUiSaveLoadEntries count expression.
inline int SaveLoadEntryCount(
    const HudUiSaveLoadDialog *dialog
) {
    return dialog->fileEntries.begin != 0
               ? (int)(dialog->fileEntries.end - dialog->fileEntries.begin)
               : 0;
}

} // namespace

RecoilApp g_RecoilApp;
RecoilStateSaveLoadTransition g_RecoilStateSaveLoadTransition;

extern "C" HWND g_RecoilApp_hWndMain;
extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" {
const char *g_RecoilApp_WndClassNamePtr = "RecoilClass";
int g_RecoilApp_WindowClassRegistered = 0;
int g_RecoilApp_AttractFmvReloadMode = 1;
}

/**
 * Reimplements 0x435a30: RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp.
 * Purpose: Initializes the save/load transition singleton and registers its exit cleanup.
 */
void RecoilStateSaveLoadTransition::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Reimplements 0x435a40: RecoilStateSaveLoadTransition::StaticInit.
 * Original source path: D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp.
 * Purpose: Constructs the global save/load transition object.
 */
RecoilStateSaveLoadTransition *RecoilStateSaveLoadTransition::StaticInit() {
    return g_RecoilStateSaveLoadTransition.Constructor();
}

/**
 * Reimplements 0x435a50: RecoilStateSaveLoadTransition::RegisterAtExit.
 * Original source path: D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp.
 * Purpose: Registers the save/load transition singleton destructor with atexit.
 */
void RecoilStateSaveLoadTransition::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x435a60: RecoilStateSaveLoadTransition::AtExitDestructor.
 * Original source path: D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp.
 * Purpose: Tears down the global save/load transition during process exit.
 */
void RecoilStateSaveLoadTransition::AtExitDestructor() {
    g_RecoilStateSaveLoadTransition.Destructor();
}

/**
 * Reimplements 0x435c80: RecoilStateSaveLoadTransition::Constructor.
 * Original source path: D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp.
 * Purpose: Initializes the save/load transition to the default save-dialog state.
 */
RecoilStateSaveLoadTransition * RecoilStateSaveLoadTransition::Constructor() {
    m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    m_dialog = 0;
    return this;
}

/**
 * Reimplements 0x435cc0: RecoilStateSaveLoadTransition::Destructor.
 * Original source path: D:\Proj\GameZRecoil\RecoilApp\RecoilStateSaveLoadTransition.cpp.
 * Purpose: Deletes the active save or load dialog owned by the transition.
 */
void RecoilStateSaveLoadTransition::Destructor() {
    HudUiSaveLoadDialog *dialog = (HudUiSaveLoadDialog *)m_dialog;
    if (dialog != 0) {
        if (m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE) {
            delete (HudUiSaveGameDialog *)dialog;
        } else {
            delete (HudUiLoadGameDialog *)dialog;
        }
        m_dialog = 0;
    }
}

/**
 * Reimplements 0x434660: operator<(HudUiSaveLoadEntry const &, HudUiSaveLoadEntry const &).
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: Orders save-game file entries by most recent write time.
 */
int __fastcall operator<(
    const HudUiSaveLoadEntry &lhs,
    const HudUiSaveLoadEntry &rhs
) {
    return CompareFileTime(
        &lhs.ftLastWriteTime,
        &rhs.ftLastWriteTime
    ) > 0 ? 1 : 0;
}

/**
 * Reimplements 0x434920: HudUiSaveLoadListItem::HudUiSaveLoadListItem.
 * Original source path: D:\Proj\Battlesport\hudui_saveload.cpp.
 * Purpose: Initializes a save/load list row panel and clears its entry index.
 */
HudUiSaveLoadListItem::HudUiSaveLoadListItem()
    : HudUiPanel(
          0,
          0,
          0
) {
    layoutY = 32767;
    layoutX = -1;
}

/**
 * Reimplements 0x434950: HudUiSaveLoadListItem::Draw.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Draws the list row panel and refreshes text bounds after rendering.
 */
void HudUiSaveLoadListItem::Draw() {
    HudUiPanel::Draw();
    UpdateTextBoundsFromContent();
}

/**
 * Reimplements 0x435a10: HudUiSaveLoadListItem::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Selects this row's save/load entry in its parent dialog.
 */
void HudUiSaveLoadListItem::OnActivate() {
    HudUiSaveLoadDialog *const owner = (HudUiSaveLoadDialog *)(parent);
    if (owner != 0) {
        owner->SetSelectedEntryIndex(layoutX);
    }
}

/**
 * Reimplements 0x434fb0: HudUiSaveLoadDialog::DeleteSaveFile.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Deletes the selected saved-game file and refreshes the dialog list.
 */
void HudUiSaveLoadDialog::DeleteSaveFile(
    int confirmDelete
) {
    char *const gameName = gameNameInput.GetBuffer();
    if (gameName == 0 || gameName[0] == '\0') {
        return;
    }

    _mkdir("SavedGames");

    char saveGamePath[MAX_PATH];
    sprintf(
        saveGamePath,
        "SavedGames\\%s",
        gameName
    );
    if (zReader::FileExists(saveGamePath) == 0) {
        return;
    }

    int shouldDelete = 1;
    if (confirmDelete != 0) {
        char titleText[128];
        char messageText[128];
        strcpy(
            titleText,
            zLoc::GetMessageString(138)
        );
        strcpy(
            messageText,
            zLoc::GetMessageString(139)
        );
        shouldDelete = HudUi::ShowMessageBox(
            messageText,
            titleText,
            (void *)1
        ) == 1 ? 1 : 0;
    }

    if (shouldDelete == 0) {
        return;
    }

    remove(saveGamePath);
    gameNameInput.Update("");
    RefreshSaveFileList();

    int selectedIndex = selectedEntryIndex;
    const int entryCount = SaveLoadEntryCount(this);
    if ((unsigned int)(selectedIndex) >= (unsigned int)(entryCount - 1)) {
        selectedIndex = entryCount - 1;
    }

    SetSelectedEntryIndex(selectedIndex);
}

/**
 * Reimplements 0x4348b0: HudUiSaveLoadGameNameInput::OnActivate.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Activates the save-game name input and moves the cursor to the end.
 */
void HudUiSaveLoadGameNameInput::OnActivate() {
    Update(GetBuffer());
    textInput.SetCursorPosition((int)(strlen(GetBuffer())));
    HudUiNumericTextInput::OnActivate();
}

/**
 * Reimplements 0x4348f0: HudUiSaveLoadGameNameInput::OnRawKeyboardEvent.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Filters raw key input to the save-game filename character set.
 */
int HudUiSaveLoadGameNameInput::OnRawKeyboardEvent(
    int key
) {
    if (strchr(
        k_SaveGameNameAllowedChars,
        key
    ) != 0) {
        textInput.DispatchKeyAction(key);
    }

    return 0;
}

/**
 * Reimplements 0x435140: HudUiSaveLoadDeleteButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Runs widget activation behavior and asks the dialog to delete the selected file.
 */
void HudUiSaveLoadDeleteButton::OnActivate() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();
    dialog->DeleteSaveFile(1);
}

/**
 * Reimplements 0x435160: HudUiSaveLoadNextButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Advances the selected save/load entry when another entry exists.
 */
void HudUiSaveLoadNextButton::OnActivate() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();

    const int nextEntryIndex = dialog->selectedEntryIndex + 1;
    if (nextEntryIndex >= 0 && nextEntryIndex < SaveLoadEntryCount(dialog)) {
        dialog->SetSelectedEntryIndex(nextEntryIndex);
    }
}

/**
 * Reimplements 0x4351b0: HudUiSaveLoadPrevButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Moves the selected save/load entry to the previous valid row.
 */
void HudUiSaveLoadPrevButton::OnActivate() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    HudUiZrdWidget::OnActivate();

    const int prevEntryIndex = dialog->selectedEntryIndex - 1;
    if (prevEntryIndex >= 0 && prevEntryIndex < SaveLoadEntryCount(dialog)) {
        dialog->SetSelectedEntryIndex(prevEntryIndex);
    }
}

/**
 * Reimplements 0x435220: HudUiSaveGamePrimaryActionButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Commits the save-game dialog result before running the widget activation path.
 */
void HudUiSaveGamePrimaryActionButton::OnActivate() {
    HudUiSaveLoadDialog *const dialog = (HudUiSaveLoadDialog *)(owner);
    if (dialog != 0) {
        dialog->ProcessDialogResult();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x435200: HudUiLoadGamePrimaryActionButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Commits the load-game dialog result before running the widget activation path.
 */
void HudUiLoadGamePrimaryActionButton::OnActivate() {
    HudUiLoadGameDialog *const dialog = (HudUiLoadGameDialog *)(owner);
    if (dialog != 0) {
        dialog->OnPrimaryAction();
    }

    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x436530: InsertEntryIntoSortedPrefix.
 * Original source path: D:\Proj\Battlesport\hudui_saveload.cpp.
 * Purpose: Inserts one save/load entry into the already sorted prefix before it.
 */
void __fastcall InsertEntryIntoSortedPrefix(
    HudUiSaveLoadEntry *entryPosition,
    HudUiSaveLoadEntry entry
) {
    HudUiSaveLoadEntry *writePosition = entryPosition;
    HudUiSaveLoadEntry *previous = entryPosition - 1;

    while (entry < *previous) {
        *writePosition = *previous;
        writePosition = previous;
        --previous;
    }

    *writePosition = entry;
}

/**
 * Reimplements 0x436580: PartitionEntriesByPivot.
 * Original source path: D:\Proj\Battlesport\hudui_saveload.cpp.
 * Purpose: Partitions a save/load entry range around the selected pivot entry.
 */
HudUiSaveLoadEntry *__fastcall PartitionEntriesByPivot(
    HudUiSaveLoadEntry *begin,
    HudUiSaveLoadEntry *end,
    HudUiSaveLoadEntry pivot
) {
    HudUiSaveLoadEntry *right = end;
    HudUiSaveLoadEntry *left = begin;

    for (;;) {
        while (*left < pivot) {
            ++left;
        }

        --right;
        while (pivot < *right) {
            --right;
        }

        if (right <= left) {
            break;
        }

        HudUiSaveLoadEntry temp = *left;
        *left = *right;
        ++left;
        *right = temp;
    }

    return left;
}

/**
 * Reimplements 0x4362f0: SortEntryRange.
 * Original source path: D:\Proj\Battlesport\hudui_saveload.cpp.
 * Purpose: Sorts a save/load entry range from newest to oldest using quicksort with insertion cleanup.
 */
void __fastcall SortEntryRange(
    HudUiSaveLoadEntry *begin,
    HudUiSaveLoadEntry *end,
    int unused
) {
    (void)unused;

    HudUiSaveLoadEntry *rangeBegin = begin;
    HudUiSaveLoadEntry *rangeEnd = end;
    int entryCount = rangeEnd - rangeBegin;
    if (entryCount <= 16) {
        return;
    }

    for (;;) {
        HudUiSaveLoadEntry lastEntry = *(rangeEnd - 1);
        HudUiSaveLoadEntry middleEntry = rangeBegin[entryCount / 2];
        HudUiSaveLoadEntry firstEntry = *rangeBegin;

        HudUiSaveLoadEntry *pivotSource;
        if (firstEntry < middleEntry) {
            if (middleEntry < lastEntry) {
                pivotSource = &middleEntry;
            } else if (firstEntry < lastEntry) {
                pivotSource = &lastEntry;
            } else {
                pivotSource = &firstEntry;
            }
        } else {
            if (firstEntry < lastEntry) {
                pivotSource = &firstEntry;
            } else if (middleEntry < lastEntry) {
                pivotSource = &lastEntry;
            } else {
                pivotSource = &middleEntry;
            }
        }

        HudUiSaveLoadEntry pivotStageCopy = *pivotSource;
        HudUiSaveLoadEntry pivotEntry = pivotStageCopy;
        HudUiSaveLoadEntry *left = rangeBegin;
        HudUiSaveLoadEntry *right = rangeEnd;

        for (;;) {
            while (*left < pivotEntry) {
                ++left;
            }

            --right;
            while (pivotEntry < *right) {
                --right;
            }

            if (right <= left) {
                break;
            }

            HudUiSaveLoadEntry swapTemp = *left;
            *left = *right;
            ++left;
            *right = swapTemp;
        }

        const int rightCount = rangeEnd - left;
        const int leftCount = left - rangeBegin;
        if (rightCount > leftCount) {
            SortEntryRange(
                rangeBegin,
                left,
                0
            );
            rangeBegin = left;
        } else {
            SortEntryRange(
                left,
                rangeEnd,
                0
            );
            rangeEnd = left;
        }

        entryCount = rangeEnd - rangeBegin;
        if (entryCount <= 16) {
            break;
        }
    }
}

/**
 * Reimplements 0x4355e0: HudUiSaveLoadDialog::RefreshSaveFileList.
 * Original source path: D:\Proj\Battlesport\hudui_saveload.cpp.
 * Purpose: Rebuilds and sorts the saved-game file entry vector from the SavedGames directory.
 */
void HudUiSaveLoadDialog::RefreshSaveFileList() {
    HudUiSaveLoadEntries *entries = &fileEntries;
    entries->EraseRangeNoDestroyInline(
        entries->begin,
        entries->end
    );

    HudUiSaveLoadEntry findData;
    HANDLE findHandle = FindFirstFileA(
        "SavedGames\\*.*",
        &findData
    );
    if (findHandle != INVALID_HANDLE_VALUE) {
        if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
            entries->InsertCopiesAt(
                entries->end,
                1,
                &findData
            );
        }

        while (FindNextFileA(
            findHandle,
            &findData
        ) != 0) {
            if ((findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                entries->InsertCopiesAt(
                    entries->end,
                    1,
                    &findData
                );
            }
        }
    }

    HudUiSaveLoadEntry *begin = entries->begin;
    HudUiSaveLoadEntry *end = entries->end;
    const int entryCount = end - begin;

    if (entryCount > 16) {
        HudUiSaveLoadEntry *rangeBegin = begin;
        HudUiSaveLoadEntry *rangeEnd = end;
        int rangeCount = entryCount;

        do {
            HudUiSaveLoadEntry lastEntry = *(rangeEnd - 1);
            HudUiSaveLoadEntry middleEntry = rangeBegin[rangeCount / 2];
            HudUiSaveLoadEntry firstEntry = *rangeBegin;

            HudUiSaveLoadEntry *pivotSource;
            if (firstEntry < middleEntry) {
                if (middleEntry < lastEntry) {
                    pivotSource = &middleEntry;
                } else if (firstEntry < lastEntry) {
                    pivotSource = &lastEntry;
                } else {
                    pivotSource = &firstEntry;
                }
            } else {
                if (firstEntry < lastEntry) {
                    pivotSource = &firstEntry;
                } else if (middleEntry < lastEntry) {
                    pivotSource = &lastEntry;
                } else {
                    pivotSource = &middleEntry;
                }
            }

            HudUiSaveLoadEntry pivotStageCopy = *pivotSource;
            HudUiSaveLoadEntry pivot = pivotStageCopy;
            HudUiSaveLoadEntry *split = PartitionEntriesByPivot(
                rangeBegin,
                rangeEnd,
                pivot
            );
            const int leftCount = split - rangeBegin;
            const int rightCount = rangeEnd - split;
            if (rightCount > leftCount) {
                SortEntryRange(
                    rangeBegin,
                    split,
                    0
                );
                rangeBegin = split;
            } else {
                SortEntryRange(
                    split,
                    rangeEnd,
                    0
                );
                rangeEnd = split;
            }

            rangeCount = rangeEnd - rangeBegin;
        } while (rangeCount > 16);
    }

    if (entryCount <= 16) {
        if (begin == end) {
            return;
        }

        HudUiSaveLoadEntry *entryPosition = begin + 1;
        if (entryPosition == end) {
            return;
        }

        do {
            HudUiSaveLoadEntry entry = *entryPosition;
            if (entry < *begin) {
                HudUiSaveLoadEntry *writePosition = entryPosition;
                while (writePosition != begin) {
                    *writePosition = *(writePosition - 1);
                    --writePosition;
                }
                *begin = entry;
            } else {
                InsertEntryIntoSortedPrefix(
                    entryPosition,
                    entry
                );
            }
            ++entryPosition;
        } while (entryPosition != end);
        return;
    }

    HudUiSaveLoadEntry *firstBlockEnd = begin + 16;
    if (begin != firstBlockEnd) {
        HudUiSaveLoadEntry *entryPosition = begin + 1;
        if (entryPosition != firstBlockEnd) {
            do {
                HudUiSaveLoadEntry entry = *entryPosition;
                if (entry < *begin) {
                    HudUiSaveLoadEntry *writePosition = entryPosition;
                    while (writePosition != begin) {
                        *writePosition = *(writePosition - 1);
                        --writePosition;
                    }
                    *begin = entry;
                } else {
                    InsertEntryIntoSortedPrefix(
                        entryPosition,
                        entry
                    );
                }
                ++entryPosition;
            } while (entryPosition != firstBlockEnd);
        }
    }

    for (HudUiSaveLoadEntry *entryPosition = firstBlockEnd; entryPosition != end; ++entryPosition) {
        HudUiSaveLoadEntry entry = *entryPosition;
        HudUiSaveLoadEntry *previous = entryPosition - 1;
        HudUiSaveLoadEntry *writePosition = entryPosition;
        if (entry < *previous) {
            do {
                *writePosition = *previous;
                writePosition = previous;
                --previous;
            } while (entry < *previous);
            *writePosition = entry;
        }
    }
}

/**
 * Reimplements 0x434ee0: HudUiSaveLoadDialog::InitializeFileEntries.
 * Original source path: D:\Proj\Battlesport\hudui_saveload.cpp.
 * Purpose: Seeds list-row layout metadata, loads saved-game entries, and binds visible rows.
 */
void HudUiSaveLoadDialog::InitializeFileEntries() {
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
    HudUiSaveLoadEntry *entry = fileEntries.begin;
    HudUiSaveLoadListItem *listItem = entryWidgets;
    while (entry != fileEntries.end && index < 9) {
        listItem->layoutX = index;
        listItem->SetTextFmt(
            "%s",
            entry->cFileName
        );
        listItem->SetVisible(
            1
        );

        ++entry;
        ++index;
        ++listItem;
    }
}

/**
 * Reimplements 0x4353f0: HudUiSaveLoadDialog::SetSelectedEntryIndex.
 * Original source path: D:\Proj\Battlesport\hudui_saveload.cpp.
 * Purpose: Updates the selected save/load entry and repopulates visible list rows around it.
 */
void HudUiSaveLoadDialog::SetSelectedEntryIndex(
    int selectedEntryIndexValue
) {
    selectedEntryIndex = selectedEntryIndexValue;

    for (int row = 0; row < 3; ++row) {
        const int entryIndex = selectedEntryIndexValue + row - 3;
        HudUiSaveLoadListItem *listItem = &entryWidgets[row];
        if (entryIndex >= 0) {
            unsigned int entryCount;
            if (fileEntries.begin == 0) {
                entryCount = 0;
            } else {
                entryCount = (unsigned int)(fileEntries.end - fileEntries.begin);
            }

            if ((unsigned int)entryIndex < entryCount) {
                listItem->layoutX = entryIndex;
                listItem->SetTextFmt(
                    "%s",
                    fileEntries.begin[entryIndex].cFileName
                );
                listItem->SetVisible(
                    1
                );
                listItem->Invalidate();
            } else {
                listItem->SetVisible(
                    0
                );
            }
        } else {
            listItem->SetVisible(
                0
            );
        }
    }

    if (selectedEntryIndexValue >= 0) {
        unsigned int selectedEntryCount;
        if (fileEntries.begin == 0) {
            selectedEntryCount = 0;
        } else {
            selectedEntryCount = (unsigned int)(fileEntries.end - fileEntries.begin);
        }

        if ((unsigned int)selectedEntryIndexValue < selectedEntryCount) {
            gameNameInput.Update(fileEntries.begin[selectedEntryIndexValue].cFileName);
        }
    }

    for (int lowerRow = 3; lowerRow < 9; ++lowerRow) {
        const int entryIndex = selectedEntryIndexValue + lowerRow - 2;
        HudUiSaveLoadListItem *listItem = &entryWidgets[lowerRow];
        if (entryIndex >= 0) {
            unsigned int entryCount;
            if (fileEntries.begin == 0) {
                entryCount = 0;
            } else {
                entryCount = (unsigned int)(fileEntries.end - fileEntries.begin);
            }

            if ((unsigned int)entryIndex < entryCount) {
                listItem->layoutX = entryIndex;
                listItem->SetTextFmt(
                    "%s",
                    fileEntries.begin[entryIndex].cFileName
                );
                listItem->SetVisible(
                    1
                );
                listItem->Invalidate();
            } else {
                listItem->SetVisible(
                    0
                );
            }
        } else {
            listItem->SetVisible(
                0
            );
        }
    }
}

/**
 * Reimplements 0x435a70: HudUiSaveLoadDialog::ProcessDialogResult.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Loads the selected saved game and queues the appropriate game-state transition.
 */
void HudUiSaveLoadDialog::ProcessDialogResult() {
    char *const gameName = gameNameInput.GetBuffer();
    char saveGamePath[MAX_PATH];
    saveGamePath[0] = '\0';

    if (gameName == 0 || gameName[0] == '\0') {
        return;
    }

    sprintf(
        saveGamePath,
        "SavedGames\\%s",
        gameName
    );
    if (zReader::FileExists(saveGamePath) == 0) {
        return;
    }

    if (zUtil::ZAR_LoadFileGlobal(saveGamePath) == 0) {
        return;
    }

    RecoilStateMainMenuTransition::ClearPausedAudioSnapshot();
    zSndPlayHandleSnapshot *const snapshot = (zSndPlayHandleSnapshot
            *)((unsigned int)(g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot));
    if (snapshot != 0) {
        snapshot->Destroy();
        g_RecoilStateSaveLoadTransition.m_pausedAudioSnapshot = 0;
    }

    zInp::SetJoystickOption(zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption()));
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
            g_RecoilApp.QueueSwitchCurrentState(
                &g_RecoilApp.m_missionFmvState,
                0
            );
        } else {
            g_RecoilApp.QueueExitCurrentState(0);
        }
        break;

    case RECOIL_SAVELOAD_MODE_FADE:
        ExtendPlayStateTransitionTimer(5.0f);
        g_RecoilApp.QueueExitCurrentState(1);
        g_RecoilApp.QueueExitCurrentState(1);
        break;

    case RECOIL_SAVELOAD_MODE_QUICKLOAD:
        ExtendPlayStateTransitionTimer(5.0f);
        g_RecoilApp.QueueExitCurrentState(0);
        break;
    }
}

/**
 * Reimplements 0x434dc0: HudUiLoadGameDialog::ProcessDialogResult.
 * Original source path: D:\Proj\Battlesport\HudUiLoadGameDialog.cpp.
 * Purpose: Uses the common save/load result handler for the load-game dialog.
 */
void HudUiLoadGameDialog::ProcessDialogResult() {
    HudUiSaveLoadDialog::ProcessDialogResult();
}

/**
 * Reimplements 0x434970: HudUiLoadGameDialog::OnPrimaryActionThunk.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Dispatches the load dialog primary action through the concrete dialog object.
 */
void HudUiLoadGameDialog::OnPrimaryActionThunk() {
    OnPrimaryAction();
}

/**
 * Reimplements 0x435240: HudUiLoadGameDialog::OnPrimaryAction.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Processes the selected file path through the global archive entry path and exits the dialog.
 */
void HudUiLoadGameDialog::OnPrimaryAction() {
    char *const gameName = gameNameInput.GetBuffer();
    if (gameName == 0 || gameName[0] == '\0') {
        g_RecoilApp.QueueExitCurrentState(0);
        return;
    }

    _mkdir("SavedGames");

    char saveGamePath[MAX_PATH];
    sprintf(
        saveGamePath,
        "SavedGames\\%s",
        gameName
    );
    if (zReader::FileExists(saveGamePath) != 0) {
        char titleText[128];
        char messageText[128];
        strcpy(
            titleText,
            zLoc::GetMessageString(136)
        );
        strcpy(
            messageText,
            zLoc::GetMessageString(137)
        );
        if (HudUi::ShowMessageBox(
            messageText,
            titleText,
            (void *)1
        ) == 2) {
            return;
        }
    }

    while (zUtil::ZBD_LoadEntriesGlobal(saveGamePath) == 0) {
        DeleteSaveFile(0);

        char titleText[128];
        char messageText[128];
        strcpy(
            titleText,
            zLoc::GetMessageString(136)
        );
        strcpy(
            messageText,
            zLoc::GetMessageString(140)
        );
        if (HudUi::ShowMessageBox(
            messageText,
            titleText,
            (void *)1
        ) == 2) {
            break;
        }
    }

    g_RecoilApp.QueueExitCurrentState(0);
}

/**
 * Reimplements 0x434680: HudUiSaveGameDialog::HudUiSaveGameDialog.
 * Original source path: D:\Proj\Battlesport\hudui_saveload.cpp.
 * Purpose: Builds the save-game dialog controls from dialog.zrd and initializes list contents.
 */
HudUiSaveGameDialog::HudUiSaveGameDialog() {
    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "SAVE_GAME_DIALOG",
        0
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &backButton,
            "BACK"
        );
        BindWidgetByName(
            loadedSection,
            &nextEntryButton,
            "NEXT_GAME_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &prevEntryButton,
            "PREV_GAME_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &deleteButton,
            "DELETE_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &primaryActionButton,
            "SAVE"
        );
        BindWidgetByName(
            loadedSection,
            &gameNameInput,
            "GAMENAME"
        );

        char listNodeName[32];
        for (int i = 0; i < 9; ++i) {
            sprintf(
                listNodeName,
                "LIST_%d",
                i
            );
            BindPrimitiveNodeToElement(
                loadedSection,
                &entryWidgets[i],
                listNodeName
            );
        }

        FreeLoadedTreeRoots((int)(unsigned int)(loadedSection));
    }

    InitializeFileEntries();
    SetSelectedEntryIndex(-1);
}

/**
 * Reimplements 0x434a80: HudUiSaveGameDialog::Destructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Tears down save-game dialog child widgets, entry storage, and background state.
 */
void HudUiSaveGameDialog::Destructor() {
    primaryActionButton.DestructorCore();

    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        entryWidgets[index - 1].HudUiPanel::~HudUiPanel();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x434b90: HudUiLoadGameDialog::HudUiLoadGameDialog.
 * Original source path: D:\Proj\Battlesport\HudUiSaveLoadDialog.cpp.
 * Purpose: Builds the load-game dialog controls from dialog.zrd and initializes list contents.
 */
HudUiLoadGameDialog::HudUiLoadGameDialog() {
    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        "LOAD_GAME_DIALOG",
        0
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &backButton,
            "BACK"
        );
        BindWidgetByName(
            loadedSection,
            &nextEntryButton,
            "NEXT_GAME_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &prevEntryButton,
            "PREV_GAME_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &deleteButton,
            "DELETE_BTN"
        );
        BindWidgetByName(
            loadedSection,
            &primaryActionButton,
            "LOAD"
        );
        BindWidgetByName(
            loadedSection,
            &gameNameInput,
            "GAMENAME"
        );

        char listNodeName[32];
        for (int i = 0; i < 9; ++i) {
            sprintf(
                listNodeName,
                "LIST_%d",
                i
            );
            BindPrimitiveNodeToElement(
                loadedSection,
                &entryWidgets[i],
                listNodeName
            );
        }

        FreeLoadedTreeRoots((int)(unsigned int)(loadedSection));
    }

    InitializeFileEntries();
    SetSelectedEntryIndex(0);
}

/**
 * Reimplements 0x4349a0: HudUiSaveLoadDialog::Destructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Tears down common save/load dialog child widgets, entry storage, and background state.
 */
void HudUiSaveLoadDialog::Destructor() {
    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        entryWidgets[index - 1].HudUiPanel::~HudUiPanel();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x434df0: HudUiLoadGameDialog::Destructor.
 * Original source path: D:\Proj\Battlesport\HudUiLoadGameDialog.cpp.
 * Purpose: Tears down load-game dialog child widgets, entry storage, and background state.
 */
void HudUiLoadGameDialog::Destructor() {
    primaryActionButton.DestructorCore();

    ::operator delete(fileEntries.begin);
    fileEntries.begin = 0;
    fileEntries.end = 0;
    fileEntries.capacityEnd = 0;

    for (int index = 9; index > 0; --index) {
        entryWidgets[index - 1].HudUiPanel::~HudUiPanel();
    }

    gameNameInput.Destructor();
    prevEntryButton.DestructorCore();
    nextEntryButton.DestructorCore();
    backButton.DestructorCore();
    deleteButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x435d20: RecoilStateSaveLoadTransition::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: Captures presentation/audio state and opens the requested save/load dialog.
 */
int RecoilStateSaveLoadTransition::OnTryBecomeCurrent() {
    if (m_capturePresentationMode != RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED) {
        if (g_zVideo_ActiveRendererPath != 0) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(
                0,
                0
            );
        }

        m_savedHalfResAdjustMode =
            (zVideoHalfResAdjustMode)zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
        HudUi::SetInvalidateMode(0);
        zSnd::ApplyMuteStateToActiveVoices(1);

        zSndPlayHandleSnapshot *const audioSnapshot =
            zSndPlayHandleSnapshot::CreateFromActiveSamples();
        m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
        audioSnapshot->StopAllIfPlaying();

        zFMV_ActionBlur blurAction;
        blurAction.Constructor(
            4,
            1
        );
        blurAction.Begin(0.0);
        while (blurAction.Update(0.0) != 0) {
        }
        blurAction.End();

        zSndSampleSet_InitByName("DIALOG");
    }

    HudUiSaveLoadDialog *dialog = 0;
    if (m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE) {
        HudUiSaveGameDialog *const storage =
            (HudUiSaveGameDialog *) ::operator new(sizeof(HudUiSaveGameDialog));
        if (storage != 0) {
            dialog = new (storage) HudUiSaveGameDialog;
        }
    } else {
        HudUiLoadGameDialog *const storage =
            (HudUiLoadGameDialog *) ::operator new(sizeof(HudUiLoadGameDialog));
        if (storage != 0) {
            dialog = new (storage) HudUiLoadGameDialog;
        }
    }

    m_dialog = (RecoilPtr32)(unsigned int)dialog;
    dialog->SetEnabled(1);
    return 1;
}

/**
 * Reimplements 0x435e80: RecoilStateSaveLoadTransition::OnUpdateShouldQuit.
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: Updates the active save/load dialog and reports whether the transition should quit.
 */
int RecoilStateSaveLoadTransition::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);

    if (m_dialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        ((HudUiSaveLoadDialog *)((unsigned int)m_dialog))->Update(g_FrameDeltaTimeSec);

        zVideo::Dispatch_UnlockPrimarySurfaceState();
    }

    zOpt_ViewRectSection *const dstRect = zOpt::GetWindowSection();
    zOpt_ViewRectSection *const srcRect = zOpt::GetWindowSection();
    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)srcRect,
        (zVidRect32 *)dstRect,
        1,
        1
    );
    return 0;
}

/**
 * Reimplements 0x435ed0: RecoilStateSaveLoadTransition::OnDeactivate.
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: Restores captured presentation/audio state and deletes the active save/load dialog.
 */
void RecoilStateSaveLoadTransition::OnDeactivate() {
    if (m_dialog != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();

        HudUiSaveLoadDialog *dialog = (HudUiSaveLoadDialog *)((unsigned int)m_dialog);
        dialog->SetEnabled(0);

        ((HudUiDialogController *)((unsigned int)m_dialog))->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        dialog = (HudUiSaveLoadDialog *)((unsigned int)m_dialog);
        if (dialog != 0) {
            if (m_dialogKind == RECOIL_SAVELOAD_DIALOG_SAVE) {
                delete (HudUiSaveGameDialog *)dialog;
            } else {
                delete (HudUiLoadGameDialog *)dialog;
            }
        }

        m_dialog = 0;
    }

    if (m_capturePresentationMode == RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED) {
        return;
    }

    zSndSampleSet_DestroyByName("DIALOG");

    zSndPlayHandleSnapshot *const audioSnapshot =
        (zSndPlayHandleSnapshot *)((unsigned int)m_pausedAudioSnapshot);
    if (audioSnapshot != 0) {
        audioSnapshot->RestoreAllWithGlobalVolumeDelta();
    }

    zSnd::ApplyMuteStateToActiveVoices(0);
    zVideo::SetHalfResAdjustMode(m_savedHalfResAdjustMode);
    HudUi::SetInvalidateMode(m_savedHalfResAdjustMode);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

/**
 * Reimplements 0x435f50: RecoilStateSaveLoadTransition::QueueOpenSaveDialog.
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: Configures and queues the save-dialog transition.
 */
void __fastcall RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
    RecoilSaveLoadPresentationCaptureMode capturePresentationMode
) {
    if (HudUiMainMenuDialog::CanSaveGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_capturePresentationMode = capturePresentationMode;
    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_SAVE;
    g_RecoilApp.QueuePushState(
        &g_RecoilStateSaveLoadTransition,
        0
    );
}

/**
 * Reimplements 0x435f80: RecoilStateSaveLoadTransition::QueueOpenLoadDialog.
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: Configures and queues the load-dialog transition.
 */
void __fastcall RecoilStateSaveLoadTransition::QueueOpenLoadDialog(
    RecoilSaveLoadTransitionMode transitionMode
) {
    if (HudUiMainMenuDialog::CanLoadGame() == 0) {
        return;
    }

    g_RecoilStateSaveLoadTransition.m_transitionMode = transitionMode;
    switch (transitionMode) {
    case RECOIL_SAVELOAD_MODE_STANDARD:
        break;
    case RECOIL_SAVELOAD_MODE_QUICKLOAD:
        g_RecoilStateSaveLoadTransition.m_capturePresentationMode =
            RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED;
        break;
    }

    g_RecoilStateSaveLoadTransition.m_dialogKind = RECOIL_SAVELOAD_DIALOG_LOAD;
    g_RecoilApp.QueuePushState(
        &g_RecoilStateSaveLoadTransition,
        0
    );
}

// Reimplements 0x430c90: RecoilApp::FatalErrorAndExit (D:\Proj\Battlesport\RecoilApp.cpp)
RECOIL_NO_GS void __fastcall RecoilApp::FatalErrorAndExit(
    int errorCode
) {
    if (errorCode != -1) {
        return;
    }

    char caption[0x80];
    char text[0x80];
    strcpy(
        caption,
        zLoc::GetMessageString(0x12)
    );
    strcpy(
        text,
        zLoc::GetMessageString(0x30)
    );

    Briefing::StopAndShutdownThread(0);
    zVideo_dd::FlipToGDIIfAttached();
    zSndSystem::Shutdown();
    zNetwork::ShutdownSessionRuntime();
    zVideo::ShutdownVideoSystem();
    printf(
        "%s: %s\n",
        caption,
        text
    );
    Sleep(1000);
    MessageBeep(MB_ICONHAND);
    MessageBoxA(
        g_RecoilApp_hWndMain,
        text,
        caption,
        MB_ICONHAND
    );
    zSys::ExitProcessWithCleanup(0);
}

// Reimplements 0x42e930: RecoilApp::ExitInstance
int RecoilApp::ExitInstance() {
    if (g_RecoilApp_WindowClassRegistered != 0) {
        HINSTANCE instanceHandle = AfxGetModuleState()->m_hCurrentInstanceHandle;
        UnregisterClassA(
            g_RecoilApp_WndClassNamePtr,
            instanceHandle
        );
        zGame::Options_SaveGameOptions();
        zGame::ReturnOnlyStub();
        zGame::Options_ShutdownRegistryContext();
        zUtil_ZRDR_Shutdown();
        zUtil_ZRDR_FreeNodePool();
        zUtil::ZBD_DestroyGlobalManager();
        zLoc::UnloadMessagesDll();
    }

    zInput::BindMapSystem_Shutdown();
    ((CWinApp *)(this))->CWinApp::ExitInstance();
    zSys::ExitProcessWithCleanup(0);
    return 0;
}

// Reimplements 0x42e520: RecoilApp::InitInstance
RECOIL_NO_GS int RecoilApp::InitInstance() {
    if (ActivateExistingInstance() == 0) {
        return 0;
    }

    WNDCLASSA wndClass = {0};
    wndClass.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS;
    wndClass.lpfnWndProc = DefWindowProcA;
    wndClass.hInstance = AfxGetModuleState()->m_hCurrentInstanceHandle;
    wndClass.hIcon =
        ::LoadIconA(
            AfxFindResourceHandle(
                IntResource(0x97),
                IntResource(0x0e)
            ),
            IntResource(0x97)
        );
    wndClass.hCursor = ::LoadCursorA(
        AfxFindResourceHandle(
            IntResource(0x7f00),
            IntResource(0x0c)
        ),
        IntResource(0x7f00)
    );
    wndClass.hbrBackground = CreateSolidBrush(0);
    wndClass.lpszMenuName = 0;
    wndClass.lpszClassName = g_RecoilApp_WndClassNamePtr;

    if (AfxRegisterClass(&wndClass) == 0) {
        return 0;
    }

    g_RecoilApp_WindowClassRegistered = 1;
    InitMainWindow();
    m_reserved148 = 0;
    m_pendingState = &m_introFmvState;

    char errorTextBuffer[0x400];
    char messageCaptionBuffer[0x100];
    char sharedTextBuffer[0x100];
    char registryCompanyNameBuffer[0x100];

    if (zLoc::LoadMessagesDll("MESSAGES.DLL") == 0) {
        char *systemErrorText = 0;
        sprintf(
            errorTextBuffer,
            "Exit at %s:%d\n",
            "D:\\Proj\\Battlesport\\RecoilApp.cpp",
            0x188
        );
        OutputDebugStringA(errorTextBuffer);
        FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            0,
            GetLastError(),
            0x400,
            (LPSTR)(&systemErrorText),
            0,
            0
        );
        strcpy(
            errorTextBuffer,
            systemErrorText
        );
        strcat(
            errorTextBuffer,
            "\n\n"
        );
        strcat(
            errorTextBuffer,
            "MESSAGES.DLL"
        );
        LocalFree(systemErrorText);
        zVideo_dd::FlipToGDIIfAttached();
        MessageBeep(MB_ICONASTERISK);
        MessageBoxA(
            0,
            errorTextBuffer,
            "",
            MB_ICONASTERISK
        );
        ExitProcess(0);
    }

    zLoc::FormatMessage(
        messageCaptionBuffer,
        0x100,
        0x83
    );
    while (zSys::FindFileOnDriveType(
        5,
        "video\\intro_01.avi",
        0
    ) == 0) {
        MessageBeep(MB_ICONEXCLAMATION);
        if (MessageBoxA(
                g_RecoilApp_hWndMain,
                messageCaptionBuffer,
                zLoc::GetMessageString(0x901),
                MB_OKCANCEL | MB_ICONEXCLAMATION
            ) != IDOK) {
            ExitProcess(0);
        }
    }

    zSysVideoCapsLevel videoCaps = ZSYS_VIDEO_CAPS_NONE;
    zSysPlatformCapsLevel platformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
    zSys::ProbePlatformAndVideoCaps(
        &videoCaps,
        &platformCaps
    );
    if ((unsigned int)(videoCaps) < (unsigned int)(ZSYS_VIDEO_CAPS_SURFACE4)) {
        zLoc::FormatMessage(
            messageCaptionBuffer,
            0x100,
            0x14
        );
        zLoc::FormatMessage(
            sharedTextBuffer,
            0x100,
            0x16
        );
        MessageBeep(MB_ICONHAND);
        MessageBoxA(
            g_RecoilApp_hWndMain,
            sharedTextBuffer,
            messageCaptionBuffer,
            MB_ICONHAND
        );
        ExitProcess(0);
    }

    zGame::ReturnOnlyStub();
    zUtil::ZBD_Init();
    zUtil::ZRDR_PreallocNodePool(0x200);
    zUtil::ZRDR_AddSearchPaths(
        0,
        "zbd"
    );
    zUtil::ZRDR_Init("..\\data\\common\\zrdr");

    strncpy(
        registryCompanyNameBuffer,
        zLoc::GetMessageString(0x900),
        sizeof(registryCompanyNameBuffer)
    );
    strncpy(
        sharedTextBuffer,
        zLoc::GetMessageString(0x901),
        sizeof(sharedTextBuffer)
    );
    zGame::Options_InitRegistryContext(
        registryCompanyNameBuffer,
        sharedTextBuffer,
        RecoilVersion::GetString()
    );
    zInput::BindMapSystem_Init(0x2f);

    if (zGame::Options_LoadGameOptions() == 0) {
        zArchive::MountIndexArchive(
            "zbd\\zrdr.zbd",
            1
        );
        if (zGame::Options_LoadGameOptions() == 0) {
            strcpy(
                sharedTextBuffer,
                zLoc::GetMessageString(0x901)
            );
            MessageBeep(MB_ICONHAND);
            MessageBoxA(
                g_RecoilApp_hWndMain,
                zLoc::GetMessageString(0x1e),
                sharedTextBuffer,
                MB_ICONHAND
            );
            ExitProcess(0);
        }
    }

    zVid::SetVideoModeIndex(zVid::GetVideoModeIndexFromOptions());
    CZRecoilFrame *const frame = (CZRecoilFrame *)((unsigned int)(GetMainWnd()));
    frame->ConfigureModeFeatureFlags();
    ((CZRecoilFrame *)((unsigned int)(GetMainWnd())))->InitStartupHwApiFromOptions();
    return 1;
}

// Reimplements 0x42e110: RecoilApp::CreateMainWnd
CZRecoilFrame * RecoilApp::CreateMainWnd() {
    CZRecoilFrame *frame = new CZRecoilFrame;
    if (frame == 0) {
        return 0;
    }

    return frame->Constructor();
}

// Reimplements 0x4429d0: RecoilApp::InitMainWindow
int RecoilApp::InitMainWindow() {
    Enable3dControls();

    m_pMainWnd = (CWnd *)CreateMainWnd();
    CZRecoilFrame *const mainWnd = GetMainWnd();
    mainWnd->m_app = this;
    m_pMainWnd->ShowWindow(SW_SHOW);
    UpdateWindow(m_pMainWnd->m_hWnd);
    return 1;
}

namespace {
const char kEngineInitFailed[] = "FAILED";
const char kEngineInitPassed[] = "PASSED";

// Source-faithful helper recovered from address-backed callers in this source file.
inline void PrintEngineInitZeroStatus(
    const char *format,
    int result
) {
    printf(
        format,
        result == 0 ? kEngineInitPassed : kEngineInitFailed
    );
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline void PrintEngineInitNonzeroStatus(
    const char *format,
    int result
) {
    printf(
        format,
        result != 0 ? kEngineInitPassed : kEngineInitFailed
    );
}

} // namespace

// Reimplements 0x442a50: RecoilApp::EngineInit
int RecoilApp::EngineInit(
    HWND hwnd
) {
    zUtil::ZRDR_PreallocNodePool(0);
    zUtil::ZRDR_Init(0);

    PrintEngineInitZeroStatus(
        "gModInit:  %s\n",
        zModel_Display_Init()
    );
    PrintEngineInitZeroStatus(
        "gClsInit:  %s\n",
        zVideo::ReturnSuccessStub()
    );
    PrintEngineInitZeroStatus(
        "zEffInit:  %s\n",
        zEffect::Init()
    );
    PrintEngineInitZeroStatus(
        "zRndrInit: %s\n",
        zRndr::InitGlobals()
    );
    PrintEngineInitNonzeroStatus(
        "zSndInit:  %s\n",
        zSnd_PreInitializeRuntimeState((RecoilPtr32)((unsigned int)hwnd))
    );
    PrintEngineInitZeroStatus(
        "zUtlInit:  %s\n",
        zVideo::ReturnSuccessStub()
    );
    PrintEngineInitZeroStatus(
        "zWepInit:  %s\n",
        zWepInit()
    );
    PrintEngineInitZeroStatus(
        "zImgInit:  %s\n",
        zImage_Init(0)
    );

    if (g_zVideo_ActiveRendererPath == 2) {
        zInput::Mouse_SetCooperativeLevelFlags(5);
    }

    PrintEngineInitZeroStatus(
        "zInInit:  %s\n",
        zInput::Init((HWND)((unsigned int)(hwnd)), (HINSTANCE)((unsigned int)(m_hInstance)))
    );
    Time::Reset();
    zVid::SetCachedClientRectUpdateMask(1);
    return 1;
}

// Reimplements 0x42e330: RecoilApp::InitializeDisplay
int __fastcall RecoilApp::InitializeDisplay(
    HWND hwnd
) {
    if (zVideo::InitVideoSystem(
            hwnd,
            zVid::GetHwApiOption(),
            zOpt::GetFullscreenOption(),
            zVid::GetVideoModeIndexFromOptions()
        ) != 0) {
        printf("Error opening video... ABORTING RUN\n");
        fflush(stdout);
        return 0;
    }

    if (zVid::GetAccelerationOption() == 0 &&
        zRndr::SpanOcclusionInit(zOpt::GetWindowSectionHeight()) != 0) {
        printf("Error opening HSE... ABORTING RUN\n");
        fflush(stdout);
        return 0;
    }

    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        zOpt::GetDisplaySection(),
        zOpt::GetDisplaySectionBitsPerPixel(),
        zVideo::GetPrimarySurfacePitch()
    );
    zRndr::SetVideoStrideMirrors(zOpt::GetVideoStrideValue());
    zVid::InitFrameScratchBuffers();

    const int oldClearState = zVideo::ExchangeClearScreenBufferEnabled(1);
    zVideo::CallClearSwSurfaceAndZBuffer(
        0,
        0
    );
    zVideo::CallClearPrimarySurfaceAndZBuffer(0);
    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    zVideo::CallClearPrimarySurfaceAndZBuffer(0);
    zVideo::AdjustSurfacesIfEnabled(
        0,
        0,
        1,
        1
    );
    zVideo::ExchangeClearScreenBufferEnabled(oldClearState);
    return 1;
}

// Reimplements 0x42e220: RecoilApp::StartEngine
RECOIL_NO_GS int RecoilApp::StartEngine(
    HWND hwnd
) {
    EngineInit(hwnd);
    PrintEngineInitZeroStatus(
        "turret:    %s\n",
        zTurret_System::ResetIterationState()
    );

    zSndSystem_Init(
        (RecoilPtr32)((unsigned int)hwnd),
        "sounds.zrd"
    );
    zSnd::SetAudioApiOption(zSnd::GetActiveBackend());

    if (InitializeDisplay(hwnd) == 0) {
        char caption[0x80];
        strcpy(
            caption,
            zLoc::GetMessageString(0x901)
        );
        MessageBoxExA(
            hwnd,
            zLoc::GetMessageString(0x1f),
            caption,
            MB_ICONHAND,
            0
        );
        return 0;
    }

    zInput::Init(
        hwnd,
        g_RecoilApp_hInstance
    );
    const int height = zOpt_DisplaySection_GetHeight();
    zInput::Mouse_SetClientSizeAndCenter(
        zOpt_DisplaySection_GetWidth(),
        height
    );
    zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption());

    zOpt_ViewRectSection *const windowSection = zOpt::GetWindowSection();
    HudUiMgr::InitHudLayouts(
        (const HudUiRect *)(zOpt::GetDisplaySection()),
        (const HudUiRect *)(windowSection)
    );
    return 1;
}

// Reimplements 0x42e990: RecoilApp::ActivateExistingInstance
int RecoilApp::ActivateExistingInstance() {
    CWnd *const existingWindow = CWnd::FromHandle(FindWindowA(
        g_RecoilApp_WndClassNamePtr,
        0
    ));
    if (existingWindow != 0) {
        CWnd *const popup = CWnd::FromHandle(GetLastActivePopup(existingWindow->m_hWnd));
        if (IsIconic(existingWindow->m_hWnd) != 0) {
            existingWindow->ShowWindow(SW_RESTORE);
        }

        SetForegroundWindow(popup->m_hWnd);
        return 0;
    }

    return 1;
}

// Reimplements 0x42e9f0: RecoilApp::PreTranslateMessage
int RecoilApp::PreTranslateMessage(
    tagMSG *msg
) {
    int handled = 0;
    if (zVid::GetAccelerationOption() != 0) {
        const UINT message = msg->message;
        if (message >= WM_SYSKEYDOWN && message <= WM_SYSKEYUP) {
            handled = 1;
        }
    }

    return handled;
}

namespace zSndCd {
void __fastcall OnMciNotify(
    unsigned int wParam,
    unsigned int lParam
);
}

namespace zDEClient {
int ShutdownGlobals();
}

// Reimplements 0x442bc0: RecoilApp::ShutdownSubsystems
void RecoilApp::ShutdownSubsystems() {
    zInput::Shutdown();
    zImage::ShutdownSubsystem();
    zUtil_ZRDR_ShutdownWildcardPath();
    zVid::ShutdownFrameScratchBuffers();
    zEffect::ShutdownAll();
    OptCatalog::Shutdown();
    zClass::Shutdown();
    zModel_Display::ShutdownThunk();
    zSndSystem::Shutdown();
    zUtil_ZRDR_Shutdown();
    zUtil_ZRDR_FreeNodePool();
}

// Reimplements 0x42e430: RecoilApp::ShutdownEngine
void RecoilApp::ShutdownEngine() {
    if (zSnd::GetCDAudioOption() != 0) {
        zSndCd::Stop();
    }

    zTurret_System::Shutdown();
    zDEClient::ShutdownGlobals();

    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionShutdown();
    }

    PickupTypeTable::FreeOptMeta();
    HudUiMgr::ShutdownResources();

    if (zOpt::GetNetworkEnabled() != 0) {
        zNetwork::ShutdownSessionRuntime();
    }

    ShutdownSubsystems();
    zVideo::ShutdownVideoSystem();
    zVideo::ReturnSuccessStub();
}

// Reimplements 0x42e490: RecoilApp::LoadZbdAndStartEngine
int RecoilApp::LoadZbdAndStartEngine() {
    if (g_HudSensorTracker.missionFlags != 0) {
        zArchive::MountIndexArchive(
            "zbd\\zrdr.zbd",
            1
        );
    }

    StartEngineAndQueueStartupState();
    g_HudSensorTracker.RegisterMissionSectionHandlers();
    return 1;
}

// Reimplements 0x42e4d0: RecoilApp::LoadZbdAndSetupSensorTracker
int RecoilApp::LoadZbdAndSetupSensorTracker(
    int missionId,
    const char *zbdPath,
    int skipIntroFmvMode,
    int missionFlags
) {
    LoadZbdAndStartEngine();
    m_skipIntroFmv = skipIntroFmvMode;
    if (zbdPath != 0) {
        g_HudSensorTracker.SetZbdPath(zbdPath);
        return 1;
    }

    g_HudSensorTracker.InitMissionIdAndFlags(
        missionId,
        missionFlags
    );
    return 1;
}

extern const AFX_MSGMAP_ENTRY g_RecoilApp_MessageEntries[1] = {
    {0, 0, 0, 0, 0, 0},
};

extern const AFX_MSGMAP g_RecoilApp_MessageMap = {
#if defined(_AFXDLL)
    &RecoilApp::GetBaseMessageMapForMfc,
#else
    RecoilMfcWinAppAccess::GetMessageMapForRecoilApp(),
#endif
    &g_RecoilApp_MessageEntries[0],
};

// Reimplements 0x4a5780: RecoilApp::InitStdLogFiles
RECOIL_NO_GS void __fastcall RecoilApp::InitStdLogFiles(
    const char *exePath
) {
    g_RecoilApp_hWndMain = 0;
    if (exePath == 0) {
        return;
    }

    char pathBuf[0x40];
    strcpy(
        pathBuf,
        exePath
    );
    strcat(
        pathBuf,
        ".err"
    );
    FILE *stream = freopen(
        pathBuf,
        "w",
        stderr
    );
    if (stream == 0 && GetTempPathA(
        sizeof(pathBuf),
        pathBuf
    ) != 0) {
        strcat(
            pathBuf,
            "gamez.err"
        );
        stream = freopen(
            pathBuf,
            "w",
            stderr
        );
    }
    if (stream != 0) {
        fprintf(
            stream,
            "File started\n---\n"
        );
        fflush(stream);
    }

    strcpy(
        pathBuf,
        exePath
    );
    strcat(
        pathBuf,
        ".out"
    );
    stream = freopen(
        pathBuf,
        "w",
        stdout
    );
    if (stream == 0 && GetTempPathA(
        sizeof(pathBuf),
        pathBuf
    ) != 0) {
        strcat(
            pathBuf,
            "gamez.out"
        );
        stream = freopen(
            pathBuf,
            "w",
            stdout
        );
    }
    if (stream != 0) {
        fprintf(
            stream,
            "File started\n---\n"
        );
        fflush(stream);
    }
}

/**
 * Reimplements 0x443700: RecoilApp_StateQueueBlock::InitFromCursor.
 * Purpose: Initializes one chunk cursor descriptor from a slot in the queue chunk map.
 */
RecoilApp_StateQueueBlock * RecoilApp_StateQueueBlock::InitFromCursor(
    RecoilApp_StateQueueItem **cursor,
    RecoilApp_StateQueueItem ***chunkBaseSlot
) {
    m_chunkBegin = *chunkBaseSlot;
    m_chunkEnd = *chunkBaseSlot + 1024;
    m_chunkBaseSlot = chunkBaseSlot;
    m_cursor = cursor;
    return this;
}

/**
 * Reimplements 0x443690: RecoilApp_StateQueue::GrowAndCenterChunkBaseList.
 * Purpose: Grows the chunk-map and recenters the active chunk-slot range in the new map.
 */
#if !(defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86))
RecoilApp_StateQueueItem *** RecoilApp_StateQueue::GrowAndCenterChunkBaseList(
    int newCapacity
) {
    int byteCount = newCapacity * (int)(sizeof(RecoilApp_StateQueueItem **));
    if (byteCount < 0) {
        byteCount = 0;
    }

    RecoilApp_StateQueueItem ***const newList =
        (RecoilApp_StateQueueItem ***)::operator new(byteCount);
    RecoilApp_StateQueueItem ***const centeredSlot =
        newList + (((unsigned int)newCapacity) >> 2);
    RecoilApp_StateQueueItem ***readSlot = m_readBlock.m_chunkBaseSlot;
    RecoilApp_StateQueueItem ***const stopSlot = m_writeBlock.m_chunkBaseSlot + 1;
    RecoilApp_StateQueueItem ***writeSlot = centeredSlot;

    while (readSlot != stopSlot) {
        *writeSlot = *readSlot;
        ++readSlot;
        ++writeSlot;
    }

    ::operator delete(m_chunkBaseList);
    m_chunkBaseList = newList;
    m_chunkBaseCapacity = newCapacity;
    return centeredSlot;
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline bool RecoilApp_StateQueue::Empty() const {
    return m_itemCount == 0;
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline RecoilApp_StateQueueItem *RecoilApp_StateQueue::Front() const {
    return *m_readBlock.m_cursor;
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline void RecoilAppQueueBlockAssignFromCursor(
    RecoilApp_StateQueueBlock *block,
    RecoilApp_StateQueueItem **cursor,
    RecoilApp_StateQueueItem ***chunkBaseSlot
) {
    block->m_chunkBegin = *chunkBaseSlot;
    block->m_chunkEnd = *chunkBaseSlot + 1024;
    block->m_cursor = cursor;
    block->m_chunkBaseSlot = chunkBaseSlot;
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline void RecoilApp_StateQueue::PopFront() {
    ++m_readBlock.m_cursor;
    --m_itemCount;

    if (m_itemCount != 0 && m_readBlock.m_cursor == m_readBlock.m_chunkEnd) {
        ++m_readBlock.m_chunkBaseSlot;
        m_readBlock.InitFromCursor(
            *m_readBlock.m_chunkBaseSlot,
            m_readBlock.m_chunkBaseSlot
        );
    }
}

// Source-faithful helper recovered from address-backed callers in this source file.
inline void RecoilApp_StateQueue::PushBack(
    RecoilApp_StateQueueItem *const &item
) {
    if (Empty() || m_writeBlock.m_cursor == m_writeBlock.m_chunkEnd) {
        RecoilApp_StateQueueItem **chunk =
            (RecoilApp_StateQueueItem **)::operator new(4096);

        if (Empty()) {
            m_chunkBaseCapacity = 2;
            m_chunkBaseList = (RecoilApp_StateQueueItem ***)::operator new(
                2 * (int)(sizeof(RecoilApp_StateQueueItem **))
            );
            m_chunkBaseList[1] = chunk;

            RecoilApp_StateQueueItem ***const chunkBaseSlot = m_chunkBaseList + 1;
            RecoilApp_StateQueueItem **const cursor = chunk + 512;
            RecoilAppQueueBlockAssignFromCursor(
                &m_readBlock,
                cursor,
                chunkBaseSlot
            );
            RecoilAppQueueBlockAssignFromCursor(
                &m_writeBlock,
                cursor,
                chunkBaseSlot
            );
        } else if (m_writeBlock.m_chunkBaseSlot <
                   m_chunkBaseList + m_chunkBaseCapacity - 1) {
            ++m_writeBlock.m_chunkBaseSlot;
            *m_writeBlock.m_chunkBaseSlot = chunk;
            RecoilAppQueueBlockAssignFromCursor(
                &m_writeBlock,
                chunk,
                m_writeBlock.m_chunkBaseSlot
            );
        } else {
            const int activeChunkCount =
                (int)(m_writeBlock.m_chunkBaseSlot - m_readBlock.m_chunkBaseSlot) + 1;
            RecoilApp_StateQueueItem **const oldReadCursor = m_readBlock.m_cursor;
            RecoilApp_StateQueueItem ***const centeredSlot =
                GrowAndCenterChunkBaseList(activeChunkCount * 2);
            RecoilApp_StateQueueItem ***const newWriteSlot =
                centeredSlot + activeChunkCount;
            *newWriteSlot = chunk;
            RecoilAppQueueBlockAssignFromCursor(
                &m_readBlock,
                oldReadCursor,
                centeredSlot
            );
            RecoilApp_StateQueueBlock writeBlock;
            writeBlock.InitFromCursor(
                chunk,
                newWriteSlot
            );
            m_writeBlock = writeBlock;
        }
    }

    RecoilApp_StateQueueItem **const slot = m_writeBlock.m_cursor;
    m_writeBlock.m_cursor = slot + 1;
    if (slot != 0) {
        *slot = item;
    }
    ++m_itemCount;
}
#else
/**
 * Original-source inline helper: VC5 owner verification uses the retail STL deque member.
 * Purpose: tests whether the recovered state queue has no pending transition items.
 */
inline bool RecoilApp_StateQueue::Empty() const {
    return empty();
}

/**
 * Original-source inline helper: VC5 owner verification uses the retail STL deque member.
 * Purpose: returns the pending transition item at the front of the queue.
 */
inline RecoilApp_StateQueueItem *RecoilApp_StateQueue::Front() const {
    return front();
}

/**
 * Original-source inline helper: VC5 owner verification uses the retail STL deque member.
 * Purpose: removes the pending transition item at the front of the queue.
 */
inline void RecoilApp_StateQueue::PopFront() {
    pop_front();
}

/**
 * Original-source inline helper: VC5 owner verification uses the retail STL deque member.
 * Purpose: appends one pending transition item to the queue.
 */
inline void RecoilApp_StateQueue::PushBack(
    RecoilApp_StateQueueItem *const &item
) {
    push_back(item);
}
#endif

/**
 * Reimplements 0x442c70: RecoilApp_MfcOleModule::RecoilApp_MfcOleModule.
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: constructs the MFC app subobject and initializes Recoil-owned state host fields.
 */
#if defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
RecoilApp_MfcOleModule::RecoilApp_MfcOleModule()
    : CWinApp(0)
#if !defined(_AFXDLL)
      , m_recoilPad(0)
#endif
{
    m_skipWait = 0;
    m_pendingState = 0;
    m_currentStateIndex = -1;
    memset(
        m_stateStack,
        0,
        sizeof(m_stateStack)
    );
}
#else
/**
 * Reimplements 0x442c70: RecoilApp_MfcOleModule::RecoilApp_MfcOleModule.
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: constructs the MFC app subobject and initializes Recoil-owned state host fields.
 */
RecoilApp_MfcOleModule::RecoilApp_MfcOleModule()
    : CWinApp(0)
#if !defined(_AFXDLL)
      , m_recoilPad(0)
#endif
      , m_pendingState(0),
      m_currentStateIndex(-1),
      m_stateHostReserved(0),
      m_skipWait(0),
      m_missionShutdownMode(RECOILAPP_MISSION_SHUTDOWN_ON_EXIT),
      m_stateQueue(),
      m_reserved148(0) {
    memset(
        m_stateStack,
        0,
        sizeof(m_stateStack)
    );
}
#endif

/**
 * Reimplements 0x4428b0: RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule.
 * Original source path: D:\Proj\Battlesport\RecoilApp.cpp.
 * Purpose: destroys the app state's chunked queue storage before chaining to the MFC base destructor.
 */
RecoilApp_MfcOleModule::~RecoilApp_MfcOleModule() {
#if defined(RECOILAPP_VC5_STL_STATE_QUEUE_MEMBER) && defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
    // VC5 emits the retail chunk-drain loop from the recovered deque member destructor.
#else
    if (m_stateQueue.m_chunkBaseList != 0) {
        RecoilApp_StateQueueItem ***slot = m_stateQueue.m_readBlock.m_chunkBaseSlot;
        RecoilApp_StateQueueItem ***const lastSlot = m_stateQueue.m_writeBlock.m_chunkBaseSlot;
        while (slot != 0 && slot <= lastSlot) {
            ::operator delete(*slot);
            ++slot;
        }

        ::operator delete(m_stateQueue.m_chunkBaseList);
        memset(
            &m_stateQueue,
            0,
            sizeof(m_stateQueue)
        );
    }
#endif
}

// Reimplements 0x42dfa0: RecoilApp::RecoilApp
RecoilApp::RecoilApp()
    : RecoilApp_MfcOleModule(),
      m_skipIntroFmv(0),
      m_transitionFadeTimer(0.0f) {
}

// Reimplements 0x42de60: RecoilApp::~RecoilApp
RecoilApp::~RecoilApp() {
}

// Source-faithful helper recovered from address-backed callers in this source file.
const AFX_MSGMAP *__stdcall RecoilApp::GetBaseMessageMapForMfc() {
    return RecoilMfcWinAppAccess::GetMessageMapForRecoilApp();
}

// Reimplements 0x42de10: RecoilApp::GetMessageMap
const AFX_MSGMAP * RecoilApp::GetMessageMap() const {
    return &g_RecoilApp_MessageMap;
}

// Reimplements 0x442c00: RecoilApp::GetMainWnd
CZRecoilFrame * RecoilApp::GetMainWnd() const {
    return (CZRecoilFrame *)m_pMainWnd;
}

// Reimplements 0x443140: RecoilApp::GetCurrentState
RecoilApp_IState * RecoilApp::GetCurrentState() const {
    if (m_currentStateIndex < 0) {
        return 0;
    }

    if (m_currentStateIndex >= (int)(sizeof(m_stateStack) / sizeof(m_stateStack[0]))) {
        return 0;
    }

    return m_stateStack[m_currentStateIndex];
}

/**
 * Reimplements 0x443160: RecoilApp::QueueSwitchCurrentState.
 * Purpose: enqueue a switch-current-state request and run the immediate exit/enter callbacks.
 */
RecoilApp_IState * RecoilApp::QueueSwitchCurrentState(
    RecoilApp_IState *state,
    int stateParam
) {
    RecoilApp_IState *const currentState = GetCurrentState();
    RecoilApp_StateQueueItem *item = new RecoilApp_StateQueueItem;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_SwitchCurrent;
        item->m_stateObj = state;
        item->m_param = stateParam;
    }
    m_stateQueue.PushBack(item);

    if (currentState != 0) {
        currentState->OnExit();
    }
    state->OnEnter();

    return currentState;
}

// Reimplements 0x443310: RecoilApp::QueuePushState
RecoilApp_IState * RecoilApp::QueuePushState(
    RecoilApp_IState *state,
    int suspendParam
) {
    RecoilApp_IState *const currentState = GetCurrentState();
    RecoilApp_StateQueueItem *item = new RecoilApp_StateQueueItem;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_PushState;
        item->m_stateObj = state;
        item->m_param = suspendParam;
    }
    m_stateQueue.PushBack(item);

    state->OnEnter();
    return currentState;
}

/**
 * Reimplements 0x4434b0: RecoilApp::QueueExitCurrentState.
 * Purpose: enqueue an exit-current-state request and run the current state's exit callback.
 */
RecoilApp_IState * RecoilApp::QueueExitCurrentState(
    int stateParam
) {
    RecoilApp_IState *const currentState = GetCurrentState();
    RecoilApp_StateQueueItem *item = new RecoilApp_StateQueueItem;
    if (item != 0) {
        item->m_type = 0;
        item->m_kind = RecoilApp_StateQueueKind_ExitCurrent;
        item->m_stateObj = 0;
        item->m_param = stateParam;
    }
    m_stateQueue.PushBack(item);

    if (currentState != 0) {
        currentState->OnExit();
    }

    return currentState;
}

// Reimplements 0x442c10: RecoilApp::StartEngineAndQueueStartupState
int RecoilApp::StartEngineAndQueueStartupState() {
    CZRecoilFrame *const mainWnd = GetMainWnd();

    if (StartEngine(mainWnd->m_hWnd) == 0) {
        ShutdownEngine();
        return ExitInstance();
    }

    m_skipWait = 1;
    m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_ON_EXIT;
    QueueSwitchCurrentState(
        m_pendingState,
        0
    );
    return 1;
}

// Reimplements 0x443650: RecoilApp::OnIdleOrDispatch
int RecoilApp::OnIdleOrDispatch(
    unsigned int wParam,
    unsigned int lParam
) {
    RecoilApp_IState *const currentState = GetCurrentState();
    zSndCd::OnMciNotify(
        wParam,
        lParam
    );
    if (currentState == 0) {
        return 0;
    }

    return currentState->OnIdleOrDispatch(
        wParam,
        lParam
    );
}

// Reimplements 0x442a10: RecoilApp::TakeSkipWaitMessage
int RecoilApp::TakeSkipWaitMessage() {
    const int wasSkipped = m_skipWait;
    m_skipWait = 0;
    return wasSkipped;
}

// Reimplements 0x442a30: RecoilApp::MarkSkipWaitMessage
int RecoilApp::MarkSkipWaitMessage() {
    const int wasSkipped = m_skipWait;
    m_skipWait = 1;
    return wasSkipped;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void RecoilApp::OnAppActivate() {
    MarkSkipWaitMessage();
}

// Source-faithful helper recovered from address-backed callers in this source file.
void RecoilApp::OnAppDeactivate() {
    TakeSkipWaitMessage();
}

// Reimplements 0x442d00: RecoilApp::Run
// (D:\Proj\Battlesport\RecoilApp.cpp)
int RecoilApp::Run() {
    CWinThread::SetThreadPriority(THREAD_PRIORITY_HIGHEST);

    for (;;) {
        while (PeekMessageA(
            &m_msgCur,
            0,
            0,
            0,
            PM_NOREMOVE
        ) != 0) {
            if (PumpMessage() == 0) {
                return ExitInstance();
            }
        }

        zNetworkDPlay::ReceivePendingMessages(-1);

        RecoilApp_IState *const currentState = GetCurrentState();
        if (m_skipWait == 0) {
            if (PeekMessageA(
                &m_msgCur,
                0,
                0,
                0,
                PM_NOREMOVE
            ) == 0) {
                WaitMessage();
            }
            continue;
        }

        if (!m_stateQueue.Empty()) {
            RecoilApp_StateQueueItem *const item = m_stateQueue.Front();
            m_stateQueue.PopFront();

            if (item->m_kind == RecoilApp_StateQueueKind_ExitCurrent) {
                if (currentState != 0) {
                    currentState->OnDeactivate();
                }

                m_stateStack[m_currentStateIndex] = 0;
                --m_currentStateIndex;
                if (m_currentStateIndex < 0) {
                    m_currentStateIndex = 0;
                }

                if (m_stateStack[m_currentStateIndex] != 0) {
                    m_stateStack[m_currentStateIndex]->OnResume(item->m_param);
                }
            } else if (item->m_kind == RecoilApp_StateQueueKind_PushState) {
                if (item->m_stateObj != 0) {
                    if (m_stateStack[m_currentStateIndex] != 0) {
                        m_stateStack[m_currentStateIndex]->OnSuspend(item->m_param);
                    }

                    if (item->m_stateObj->OnTryBecomeCurrent() != 0) {
                        ++m_currentStateIndex;
                        if (m_currentStateIndex >= 16) {
                            m_currentStateIndex = 15;
                        }

                        m_stateStack[m_currentStateIndex] = item->m_stateObj;
                    }
                }
            } else if (item->m_kind == RecoilApp_StateQueueKind_SwitchCurrent) {
                if (item->m_stateObj != 0) {
                    if (currentState != 0) {
                        currentState->OnDeactivate();
                    }

                    if (m_currentStateIndex < 0) {
                        m_currentStateIndex = 0;
                    }
                    if (m_currentStateIndex >= 16) {
                        m_currentStateIndex = 15;
                    }

                    if (item->m_stateObj->OnTryBecomeCurrent() != 0) {
                        m_stateStack[m_currentStateIndex] = item->m_stateObj;
                    } else if (currentState != 0) {
                        currentState->OnTryBecomeCurrent();
                    }
                }
            }

            delete item;
            continue;
        }

        if (currentState != 0 && currentState->OnUpdateShouldQuit() != 0) {
            OnAppDeactivate();
            PostQuitMessage(0);
        }
    }
}

// Reimplements 0x42eea0: RecoilApp_PlayState::RecoilApp_PlayState
RecoilApp_PlayState::RecoilApp_PlayState() {
    m_transitionScratch = 0;
    pPendingLoadGameStartPath = 0;
}

// Reimplements 0x42eec0: RecoilApp_PlayState::OnWndActivate
void RecoilApp_PlayState::OnWndActivate(
    int bActivate
) {
    if (bActivate != 0) {
        HudUiMgr::TriggerCurrentLayoutOnActivated();
    }
}

// Reimplements 0x42eed0: RecoilApp_PlayState::OnTryBecomeCurrent
// (D:\Proj\Battlesport\RecoilApp.cpp)
int RecoilApp_PlayState::OnTryBecomeCurrent() {
    const int completedObjectiveCount = g_HudSensorTracker.completedObjectiveCount;

    if (zVid::GetAccelerationOption() != 0) {
        BOOL screenSaverRunning = FALSE;
        SystemParametersInfoA(
            SPI_SETSCREENSAVERRUNNING,
            1,
            &screenSaverRunning,
            0
        );
    }

    Time::Reset();
    g_FrameDeltaTimeSec = 0.100000001f;

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiNetExitPanel::CreateGlobal();
    }

    int effectsLevel = zOpt::GetEffectsLevelForCurrentHwMode();
    if (zVid::GetAccelerationOption() == 0 && effectsLevel == 0) {
        effectsLevel = 1;
    }
    zOpt::SetEffectsLevelForCurrentHwMode(effectsLevel);

    HudUiMgr::EnsureHudLoaded("hud.zrd");
    HudUiLoadingCheckpoint::InitTable();
    HudUiLoadingCheckpoint::AdvanceAndLog("Loading common sounds");
    zSndSampleSet_InitByName("COMMON");

    Briefing::StartForMission(g_HudSensorTracker.GetMissionId());

    char loadingMessage[0x100];
    zLoc::FormatMessage(
        loadingMessage,
        sizeof(loadingMessage),
        3,
        RecoilVersion::GetString()
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    zLoc::FormatMessage(
        loadingMessage,
        sizeof(loadingMessage),
        5,
        zVid::GetSelectedHwApiDescriptionOrDefault()
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    zLoc::FormatMessage(
        loadingMessage,
        sizeof(loadingMessage),
        6,
        zVid::GetSelectedD3DDeviceNameOrDefault()
    );
    HudUiLoadingCheckpoint::AdvanceAndLog(loadingMessage);

    HudUiLoadingCheckpoint::AdvanceAndLog(zLoc::GetMessageString(0x10d));

    g_HudSensorTracker.LoadObjectivesFromPath("objectives.zrd");
    Player::ZAR_RegisterSections();
    Briefing::BuildObjectiveActionsGlobal(completedObjectiveCount);

    if (g_HudSensorTracker.LoadMissionCoreResources() == 0) {
        return 0;
    }

    g_HudSensorTracker.InitMissionGameplaySystems();
    Briefing::StopAndShutdownThread(1);
    HudUiMgr::ApplyHudModeSwitch(ZOPT_HUD_TYPE_STANDARD);

    const char *startAnimNodeName;
    if (pPendingLoadGameStartPath != 0) {
        ExtendPlayStateTransitionTimer(5.0f);

        char *const pendingLoadPath = pPendingLoadGameStartPath;
        zUtil::ZAR_LoadFileGlobal(pendingLoadPath);
        free(pendingLoadPath);
        pPendingLoadGameStartPath = 0;
        startAnimNodeName = "LOAD_GAME_START";
    } else {
        startAnimNodeName = "NEW_GAME_START";
    }

    g_HudSensorTracker.RunStartAnimsFromZrd(
        "StartAnims.zrd",
        startAnimNodeName
    );

    pRenderSection = zOpt::GetRenderSection();
    pDisplaySection = zOpt::GetDisplaySection();
    pWindowSection = zOpt::GetWindowSection();

    zInput::Keyboard_ResetTransitionState();
    zInput::Mouse_RecenterCursor();

    if (zVid::GetAccelerationOption() != 0) {
        zClass_Camera::SetActiveCamera(0);
        zClass_Camera::SetObjectHseTestEnabled(0);
    }

    ExtendPlayStateTransitionTimer(1.0f);

    TickAndRenderFrame(0);
    zInput::Keyboard_ResetTransitionState();
    zInput::Mouse_RecenterCursor();

    g_zVideo_FrameTick = 0;
    g_RecoilApp.m_reserved148 = 1;
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_ENABLED);
    g_HudSensorTracker.ResetHudForMissionStart();

    if (zInput::Mouse_IsInitialized() != 0) {
        g_zInput_MouseActive = 0;
        zInput::Mouse_UpdateAcquireState();
    }

    zInput::ResetAllTransitionState();

    zOpt::SetGraphicsFlagsForCurrentHwMode(zOpt::GetGraphicsFlagsForCurrentHwMode());
    zInp::SetJoystickOption(zInput::DI_SetJoystickEnabled(zInp::GetJoystickOption()));
    zOpt::SetCursorMode(zOpt::GetCursorMode());
    zOpt::SetCameraMode(zOpt::GetCameraModePlayerState());
    zOpt::SetThrottleMode(zOpt::GetThrottleMode());
    zOpt::SetSteeringMode(zOpt::GetSteeringMode());

    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        const int trackCount = zSndCd::GetTrackCount();
        zSndCd::PlayTrackWithMode(
            (missionId % (trackCount - 2)) + 2,
            5
        );
    }

    if (zOpt::GetNetworkEnabled() != 0) {
        if (zNetwork::IsHost() == 0) {
            ExtendPlayStateTransitionTimer(5.0f);
            HudUiMgr::EnableTopAndChatStacks();
            return 1;
        }

        HudUiMgr::EnableTopAndChatStacks();
    }

    return 1;
}

// Reimplements 0x42f5e0: RecoilApp_PlayState::OnUpdateShouldQuit
// (D:\Proj\Battlesport\RecoilApp.cpp)
int RecoilApp_PlayState::OnUpdateShouldQuit() {
    if (g_RecoilApp.m_transitionFadeTimer > 0.0f) {
        g_zVideo_SoftwareModeHotkeyEnabled = ZVIDEO_SOFTWARE_MODE_HOTKEY_DISABLED;
        TickAndRenderFrame(0);

        zOpt_ViewRectSection *const windowSection = ViewRectFromPtr(pWindowSection);
        if (g_RecoilApp.m_transitionFadeTimer >= 1.0f) {
            const int previousClearState =
                zVideo::ExchangeClearScreenBufferEnabled(ZVIDEO_CLEAR_SCREEN_BUFFER_ENABLED);
            ((zUtil_SaveGameState *)g_GameStateOrMapTable)
                ->playerState->transitionDamageSuppressed = 1;
            if (zVid::GetAccelerationOption() != 0) {
                zVideo::CallClearSwSurfaceAndZBuffer(
                    (zVidRect32 *)windowSection,
                    (zVidRect32 *)windowSection
                );
            } else {
                zVideo::CallClearPrimarySurfaceAndZBuffer((zVidRect32 *)windowSection);
            }
            zVideo::ExchangeClearScreenBufferEnabled(previousClearState);
        } else {
            const double overlayAlpha = g_RecoilApp.m_transitionFadeTimer > 0.0f
                                            ? (double)(g_RecoilApp.m_transitionFadeTimer)
                                            : 0.0;
            zRndr_OverlayRect_Submit(
                0,
                0,
                overlayAlpha
            );
        }

        zVideo::AdjustSurfacesIfEnabled(
            (zVidRect32 *)windowSection,
            (zVidRect32 *)windowSection,
            0,
            0
        );
        g_RecoilApp.m_transitionFadeTimer -= g_FrameDeltaTimeSec;

        if (g_RecoilApp.m_transitionFadeTimer <= 0.0f) {
            zOpt::SetMuteSoundOption(0);
            HudUiMgr::TriggerCurrentLayoutOnActivated();
            ((zUtil_SaveGameState *)g_GameStateOrMapTable)
                ->playerState->transitionDamageSuppressed = 0;
        }

        return 0;
    }

    if (g_RecoilApp_QuitAfterCredits != 0) {
        zSndPlayHandleSnapshot *const snapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
        snapshot->StopAllIfPlaying();
        zSndCd::Stop();

        zFMV_Script fmvScript;
        fmvScript.Init(
            "fmv.zrd",
            "GRANDPRIZE",
            0
        );
        fmvScript.RunBlocking(0);

        if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
            g_zVideo_pfnBltSwToPrimaryRectDirect(
                0,
                0
            );
        }

        zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
        HudUi::SetInvalidateMode(0);

        RunGrandPrizeBlurAction();
        fmvScript.Cleanup();
        return 0;
    }

    g_zVideo_SoftwareModeHotkeyEnabled = ZVIDEO_SOFTWARE_MODE_HOTKEY_ENABLED;
    if (TickAndRenderFrame(1) != 0) {
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiNetExitPanel::Show();
            return 0;
        }

        zRndr::SetActiveRegionSizeFromRect((HudUiRect *)ViewRectFromPtr(pWindowSection));
        if (g_RecoilApp_QuitAfterCredits == 0) {
            RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_INGAME);
        }
    }

    return 0;
}

// Reimplements 0x42f8a0: RecoilApp_PlayState::OnResume
void RecoilApp_PlayState::OnResume(
    int
) {
    if (zSnd::GetCDAudioOption() != 0) {
        const int missionId = g_HudSensorTracker.GetMissionId();
        const int trackCount = zSndCd::GetTrackCount();
        zSndCd::PlayTrackWithMode(
            (missionId % (trackCount - 2)) + 2,
            5
        );
    }
}

// Reimplements 0x42f8e0: RecoilApp_PlayState::OnDeactivate
// (D:\Proj\Battlesport\RecoilApp.cpp)
void RecoilApp_PlayState::OnDeactivate() {
    HudUiLoadingCheckpoint::AdvanceAndLog("Leaving Play State");

    if (zVid::GetAccelerationOption() != 0) {
        BOOL screenSaverRunning = FALSE;
        SystemParametersInfoA(
            SPI_SETSCREENSAVERRUNNING,
            0,
            &screenSaverRunning,
            0
        );
    }

    zSndCd::Stop();
    zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);

    if (zOpt::GetNetworkEnabled() != 0) {
        HudUiLoadingCheckpoint::AdvanceAndLog("Leaving Networking");
        HudUiNetExitPanel::DestroyGlobal();
    }

    if (zOpt::GetNetworkEnabled() == 0) {
        HudUiLoadingCheckpoint::AdvanceAndLog("Stop All Sounds");
        zSndPlayHandleSnapshot *const snapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
        snapshot->StopAllIfPlaying();
    }

    zFMV_Script fmvScript;
    fmvScript.Init(
        "fmv.zrd",
        "MISSIONOVER",
        0
    );
    fmvScript.RunBlocking(1);

    if (g_RecoilApp.m_missionShutdownMode == RECOILAPP_MISSION_SHUTDOWN_ON_EXIT) {
        g_HudSensorTracker.ShutdownMissionGameplaySystems();
    }

    zUtil_ZRDR_UnloadMountedArchives(0);
    fmvScript.Cleanup();
}

// Reimplements 0x42f9d0: RecoilApp_LeaveNetworkState::OnTryBecomeCurrent
int RecoilApp_LeaveNetworkState::OnTryBecomeCurrent() {
    zNetwork_DPlay_DestroyCachedLocalPlayer();
    g_RecoilApp.ShutdownEngine();
    zSndBackend::Shutdown();
    return 1;
}

// Reimplements 0x42eb70: RecoilApp_AttractFmvState::Constructor
// Actual C++ construction lets VC emit the one-state IState cleanup funclet.
RecoilApp_AttractFmvState::RecoilApp_AttractFmvState() {
}

// Source-faithful helper recovered from address-backed callers in this source file.
RecoilApp_IntroFmvState::RecoilApp_IntroFmvState() {
    m_stateData04 = 0;
}

// Reimplements 0x42ea20: RecoilApp_IntroFmvState::OnTryBecomeCurrent
int RecoilApp_IntroFmvState::OnTryBecomeCurrent() {
    zRndr::SetFrameBufferRegion(
        zVideo::GetPrimarySurfacePixels(),
        zOpt::GetWindowSection(),
        zOpt::GetDisplaySectionBitsPerPixel(),
        zVideo::GetPrimarySurfacePitch()
    );
    zRndr::SetVideoStrideMirrors(zOpt::GetVideoStrideValue());

    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
    zVid::SetCachedClientRectUpdateMask(1);

    if (g_RecoilApp.m_skipIntroFmv == 0) {
        zFMV_Script *const script = &m_fmv;
        if (g_RecoilApp_hWndMain != 0) {
            script->m_hWnd = g_RecoilApp_hWndMain;
        }

        if (script->LoadActionsFromZrd(
            "fmv.zrd",
            "INTRO"
        ) != -1) {
            script->BeginAtTime();
        }
    }

    return 1;
}

// Reimplements 0x42eac0: RecoilApp_IntroFmvState::OnUpdateShouldQuit
int RecoilApp_IntroFmvState::OnUpdateShouldQuit() {
    if (g_RecoilApp.m_skipIntroFmv != 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_missionFmvState,
            0
        );
        return 0;
    }

    zFMV_Script *const script = &m_fmv;
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_mainMenuPrepState,
            stateParam
        );
    }

    return 0;
}

// Reimplements 0x42eb00: RecoilApp_FmvState::OnIdleOrDispatch
int RecoilApp_FmvState::OnIdleOrDispatch(
    unsigned int,
    unsigned int
) {
    return 1;
}

// Reimplements 0x42eb10: RecoilApp_IntroFmvState::OnDeactivate
void RecoilApp_IntroFmvState::OnDeactivate() {
    m_fmv.BeginNow(1);
}

// Reimplements 0x42eb20: RecoilApp_MainMenuPrepState::OnTryBecomeCurrent
int RecoilApp_MainMenuPrepState::OnTryBecomeCurrent() {
    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );
    m_stateData04 = 0;
    return 1;
}

// Reimplements 0x42eb60: RecoilApp_MainMenuPrepState::OnUpdateShouldQuit
int RecoilApp_MainMenuPrepState::OnUpdateShouldQuit() {
    RecoilStateMainMenuTransition::QueueEnter(RECOIL_MAINMENU_ROUTE_FRONTEND);
    return 0;
}

// Reimplements 0x42ebf0: RecoilApp_AttractFmvState::OnTryBecomeCurrent
int RecoilApp_AttractFmvState::OnTryBecomeCurrent() {
    zVideo::Fx_SetSurfaceState(
        zVideo::GetPrimarySurfacePixels(),
        zVideo::GetPrimarySurfaceWidth(),
        zVideo::GetPrimarySurfaceHeight(),
        zVideo::GetPrimarySurfacePitch()
    );

    GetClientRect(
        g_RecoilApp_hWndMain,
        (RECT *)(m_clientRect)
    );

    if (g_RecoilApp_AttractFmvReloadMode != 0) {
        m_fmv.LoadActionsFromZrd(
            "fmv.zrd",
            "ATTRACT"
        );
        g_RecoilApp_AttractFmvReloadMode = 0;
    }

    zFMV_Script *const script = &m_fmv;
    if (g_RecoilApp_hWndMain != 0) {
        script->m_hWnd = g_RecoilApp_hWndMain;
    }

    if (script->LoadActionsFromZrd(
        "fmv.zrd",
        "ATTRACT"
    ) != -1) {
        script->BeginAtTime();
    }

    return 1;
}

// Reimplements 0x42ec80: RecoilApp_AttractFmvState::OnUpdateShouldQuit
int RecoilApp_AttractFmvState::OnUpdateShouldQuit() {
    zFMV_Script *const script = &m_fmv;
    const int stateParam = script->UpdateAtTime();
    if (stateParam == 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_mainMenuPrepState,
            stateParam
        );
    }

    return 0;
}

// Reimplements 0x42eca0: RecoilApp_AttractFmvState::OnDeactivate
void RecoilApp_AttractFmvState::OnDeactivate() {
    m_fmv.BeginNow(0);
}

// Reimplements 0x42ed30: RecoilApp_MissionFmvState::Constructor
// Actual C++ construction lets VC emit the one-state IState cleanup funclet.
RecoilApp_MissionFmvState::RecoilApp_MissionFmvState() {
    m_missionId = 0;
    m_skipMissionFmv = 0;
}

// Reimplements 0x42edb0: RecoilApp_MissionFmvState::OnTryBecomeCurrent
// (D:\Proj\GameZRecoil\recoilapp.cpp)
int RecoilApp_MissionFmvState::OnTryBecomeCurrent() {
    if (m_missionId == 0) {
        m_missionId = g_HudSensorTracker.GetMissionId();
    } else {
        g_HudSensorTracker.SetMissionId(m_missionId);
    }

    zUtil::SetMissionZrdrPathsAndMountZbd(m_missionId);

    char missionFmvTag[4];
    missionFmvTag[0] = 'M';
    missionFmvTag[1] = (char)(m_missionId + '0');
    missionFmvTag[2] = '\0';

    if (m_skipMissionFmv == 0) {
        zFMV_Script *const script = &m_fmv;
        if (g_RecoilApp_hWndMain != 0) {
            script->m_hWnd = g_RecoilApp_hWndMain;
        }

        if (script->LoadActionsFromZrd(
            "fmv.zrd",
            missionFmvTag
        ) != -1) {
            script->BeginAtTime();
        }
    }

    return 1;
}

// Reimplements 0x42ee50: RecoilApp_MissionFmvState::OnDeactivate
// (D:\Proj\GameZRecoil\recoilapp.cpp)
void RecoilApp_MissionFmvState::OnDeactivate() {
    const int skipMissionFmv = m_skipMissionFmv;
    m_missionId = 0;
    if (skipMissionFmv == 0) {
        m_fmv.BeginNow(1);
    }
}

// Reimplements 0x42ee70: RecoilApp_MissionFmvState::OnUpdateShouldQuit
// (D:\Proj\GameZRecoil\recoilapp.cpp)
int RecoilApp_MissionFmvState::OnUpdateShouldQuit() {
    if (m_skipMissionFmv != 0 || m_fmv.UpdateAtTime() == 0) {
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_playState,
            0
        );
    }

    return 0;
}

// Source-faithful helper recovered from address-backed callers in this source file.
RecoilApp_IState::~RecoilApp_IState() {
}

void RecoilApp_IState::OnWndActivate(
    int
) {}

// Source-faithful helper recovered from address-backed callers in this source file.
void RecoilApp_IState::OnEnter() {}

int RecoilApp_IState::OnTryBecomeCurrent() {
    return 1;
}

int RecoilApp_IState::OnUpdateShouldQuit() {
    return 0;
}

// Source-faithful helper recovered from address-backed callers in this source file.
void RecoilApp_IState::OnExit() {}

void RecoilApp_IState::OnDeactivate() {}

// Source-faithful helper recovered from address-backed callers in this source file.
void RecoilApp_IState::OnSuspend(
    int
) {}

void RecoilApp_IState::OnResume(
    int
) {}

int RecoilApp_IState::OnIdleOrDispatch(
    unsigned int,
    unsigned int
) {
    return 1;
}

// Reimplements 0x42df10: RecoilApp_AttractFmvState::~RecoilApp_AttractFmvState
RecoilApp_AttractFmvState::~RecoilApp_AttractFmvState() {
    m_fmv.Cleanup();
}

// Reimplements 0x42df50: RecoilApp_IntroFmvState::~RecoilApp_IntroFmvState
RecoilApp_IntroFmvState::~RecoilApp_IntroFmvState() {
    m_fmv.Cleanup();
}

// Reimplements 0x42e070: RecoilApp_MissionFmvState::~RecoilApp_MissionFmvState
RecoilApp_MissionFmvState::~RecoilApp_MissionFmvState() {
    m_fmv.Cleanup();
}
