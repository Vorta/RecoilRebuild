#include "Battlesport/Mfc42Abi.h"
#include "GameZRecoil/zUtil/zsave_game.h"

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"

#include <stdlib.h>
#include <string.h>

/**
 * Reimplements 0x4383e0: zUtil_SaveGameStateList_Init.
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
 * Reimplements 0x4384e0: zUtil_SaveGameStateList_AllocAppend.
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

/**
 * Reimplements 0x438430: zUtil_SaveGameState::FreeOwnedResources
 * (D:\Proj\GameZRecoil\zUtil\zUtil.cpp).
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
