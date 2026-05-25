#include "GameZRecoil/zUtil/zSaveGame.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/player.h"

#include <stdlib.h>
#include <string.h>

// Reimplements 0x4383e0: zUtil_SaveGameStateList_Init
RECOIL_NOINLINE zUtil_SaveGameState *RECOIL_FASTCALL
zUtil_SaveGameStateList_Init(zUtil_SaveGameState *self) {
    self->unknown_10 = 0;
    self->saveStateListTail = 0;
    self->saveStateListHead = 0;
    self->saveStateCount = 0;
    self->next = 0;
    self->firstSaveState = 0;

    self->playerState =
        static_cast<zUtil_PlayerStateStorage *>(malloc(sizeof(zUtil_PlayerStateStorage)));
    memset(self->playerState, 0, sizeof(zUtil_PlayerStateStorage));

    self->unknown_0c = 0;
    self->unknown_24 = 0;
    self->modeLoopBlend = 0.0f;
    return self;
}

// Reimplements 0x4384e0: zUtil_SaveGameStateList_AllocAppend
RECOIL_NOINLINE zUtil_SaveGameState *RECOIL_FASTCALL
zUtil_SaveGameStateList_AllocAppend(zUtil_SaveGameState *self) {
    zUtil_SaveGameState *const saveState =
        static_cast<zUtil_SaveGameState *>(malloc(sizeof(zUtil_SaveGameState)));
    memset(saveState, 0, sizeof(zUtil_SaveGameState));

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

// Reimplements 0x438430: zUtil_SaveGameState::FreeOwnedResources
// (D:\Proj\GameZRecoil\zUtil\zUtil.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zUtil_SaveGameState::FreeOwnedResources() {
    if (playerState->lifecycleState == 2) {
        Player::AiDiscardNegativeBranchPathNodes(this);
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
