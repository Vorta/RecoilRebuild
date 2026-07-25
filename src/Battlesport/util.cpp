#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zUtil/zsave_game.h"

#include "Battlesport/ai_net.h"
#include "Battlesport/game_net.h"

#include <stdlib.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:battlesport-util-g-huduimessageboxdialog-sectionname
 * @recoil-artifact defines .data recoil:data:0x4dd1c8: g_HudUiMessageBoxDialog_SectionName.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: local MESSAGEBOX ZRD section-name data for the
 * HudUi::ShowMessageBox entrypoint wrapper; exact .data extent is the
 * writable char[11] bytes "MESSAGEBOX\0" with the sole xref in 0x438350.
 * Purpose: name the dialog.zrd section loaded by the modal message-box
 * wrapper.
 */
char g_HudUiMessageBoxDialog_SectionName[11] = "MESSAGEBOX";
/**
 * @recoil-anchor recoil:anchor:battlesport-util-k-msgboxwidgetname-message
 * @recoil-artifact defines .data recoil:data:0x4e489c: k_msgBoxWidgetName_Message.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the message text primitive from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Message[8] = "MESSAGE";
/**
 * @recoil-anchor recoil:anchor:battlesport-util-k-msgboxwidgetname-title
 * @recoil-artifact defines .data recoil:data:0x4e48a4: k_msgBoxWidgetName_Title.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the title primitive from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Title[6] = "TITLE";
/**
 * @recoil-anchor recoil:anchor:battlesport-util-k-msgboxwidgetname-cancel
 * @recoil-artifact defines .data recoil:data:0x4e48ac: k_msgBoxWidgetName_Cancel.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the cancel button from a loaded message-box layout.
 */
char k_msgBoxWidgetName_Cancel[10] = "MB_CANCEL";
/**
 * @recoil-anchor recoil:anchor:battlesport-util-k-msgboxwidgetname-ok
 * @recoil-artifact defines .data recoil:data:0x4e48b8: k_msgBoxWidgetName_OK.
 * Source model: writable ZRD widget-name literal used only by
 * HudUiMessageBoxDialog::Constructor.
 * Purpose: bind the OK button from a loaded message-box layout.
 */
char k_msgBoxWidgetName_OK[6] = "MB_OK";

namespace HudUi {
/**
 * @recoil-anchor recoil:anchor:battlesport-util-hudui-showmessagebox
 * @recoil-artifact defines .text recoil:function:0x438350: HudUi::ShowMessageBox.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * BN source path: D:\Proj\Battlesport\HudUiMessageBoxDialog.cpp.
 * Source model: HudUiMessageBoxDialog.cpp entrypoint wrapper that constructs
 * the stack HudUiMessageBoxDialog, not a broad HudUi owner or table scaffold.
 * Purpose: load the MESSAGEBOX section from dialog.zrd, run the dialog modally
 * with the caller strings/context and infinite timeout, then destroy it.
 * Touched data: g_HudUiMessageBoxDialog_SectionName at 0x4dd1c8 is the local
 * writable char[11] MESSAGEBOX section-name data; dialog.zrd is the accepted
 * shared dialog path literal.
 * Source placement note: this definition was provisionally moved from
 * HudUiMessageBoxDialog.cpp.
 */
int __fastcall ShowMessageBox(
    const char *messageText,
    const char *titleText,
    void *modalContext
) {
    HudUiMessageBoxDialog dialog;
    dialog.Constructor(
        "dialog.zrd",
        g_HudUiMessageBoxDialog_SectionName
    );
    const int result = dialog.RunModal(
        messageText,
        titleText,
        modalContext,
        -1.0f
    );
    dialog.Destructor();
    return result;
}
} // namespace HudUi

/**
 * @recoil-anchor recoil:anchor:battlesport-util-zutil-savegamestatelist-init
 * @recoil-artifact defines .text recoil:function:0x4383e0: zUtil_SaveGameStateList_Init.
 *
 * Purpose: initialize a save-state list sentinel and allocate zeroed player
 * state storage for the owning save-game state.
 */
zUtil_SaveGameState *__fastcall zUtil_SaveGameStateList_Init(
    zUtil_SaveGameState *self
) {
    self->unknown_10 = 0;
    self->saveStateListTail = 0;
    self->saveStateListHead = 0;
    self->saveStateCount = 0;
    self->next = 0;
    self->firstSaveState = 0;

    self->playerState = (zUtil_PlayerStateStorage *)(malloc(sizeof(zUtil_PlayerStateStorage)));
    memset(
        self->playerState,
        0,
        sizeof(zUtil_PlayerStateStorage)
    );

    self->unknown_0c = 0;
    self->unknown_24 = 0;
    self->modeLoopBlend = 0.0f;
    return self;
}

/**
 * @recoil-anchor recoil:anchor:battlesport-util-zutil-savegamestate-freeownedresources
 * @recoil-artifact defines .text recoil:function:0x438430: zUtil_SaveGameState::FreeOwnedResources
 *
 * Purpose: detach save-state back-references, free modal-state nodes, and
 * release the owned player-state storage.
 */
void zUtil_SaveGameState::FreeOwnedResources() {
    if (playerState->lifecycleState == 2) {
        AINet::AiDiscardNegativeBranchPathNodes(this);
    }

    if (netPlayerRow != 0) {
        netPlayerRow->saveState = 0;
    }

    PlayerModalState *modalState = modalStateListHead;
    while (modalState != 0) {
        PlayerModalState *const nextModalState = modalState->next;
        if (modalStateCount != 0) {
            if (modalState == modalStateListHead) {
                --modalStateCount;
                modalStateListHead = modalState->next;
                if (modalStateListHead == 0) {
                    modalStateListAux = 0;
                    modalStateListTail = 0;
                }
            } else {
                PlayerModalState *cursor = modalStateListHead;
                while (cursor != 0) {
                    PlayerModalState *const cursorNext = cursor->next;
                    if (cursorNext == modalState) {
                        --modalStateCount;
                        cursor->next = modalState->next;
                        if (modalStateListTail == modalState) {
                            modalStateListTail = cursor;
                        }
                        break;
                    }

                    cursor = cursorNext;
                }
            }
        }

        free(modalState);
        modalState = nextModalState;
    }

    primaryModalState = 0;
    free(playerState);
}

/**
 * @recoil-anchor recoil:anchor:battlesport-util-zutil-savegamestatelist-allocappend
 * @recoil-artifact defines .text recoil:function:0x4384e0: zUtil_SaveGameStateList_AllocAppend.
 *
 * Purpose: allocate a zeroed save-state node and append it to the tracked
 * save-state list.
 */
zUtil_SaveGameState *__fastcall zUtil_SaveGameStateList_AllocAppend(
    zUtil_SaveGameState *self
) {
    zUtil_SaveGameState *const saveState =
        (zUtil_SaveGameState *)(malloc(sizeof(zUtil_SaveGameState)));
    memset(
        saveState,
        0,
        sizeof(zUtil_SaveGameState)
    );

    if (self->firstSaveState == 0) {
        self->firstSaveState = saveState;
    }

    if (saveState != 0) {
        saveState->next = 0;
        if (self->saveStateCount == 0) {
            self->saveStateListHead = saveState;
        } else {
            self->saveStateListTail->next = saveState;
        }

        self->saveStateListTail = saveState;
        saveState->next = 0;
        ++self->saveStateCount;
    }

    return saveState;
}
