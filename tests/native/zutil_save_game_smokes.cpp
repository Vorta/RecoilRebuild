#include "Battlesport/GameNet.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>

extern "C" int zutil_save_game_state_list_smoke(void) {
    zUtil_SaveGameState list = {};
    list.next = (zUtil_SaveGameState *)(0x1);
    list.firstSaveState = (zUtil_SaveGameState *)(0x2);
    list.saveStateListHead = (zUtil_SaveGameState *)(0x3);
    list.saveStateListTail = (zUtil_SaveGameState *)(0x4);
    list.saveStateCount = 5;

    if (zUtil_SaveGameStateList_Init(&list) != &list ||
        list.next != 0 ||
        list.firstSaveState != 0 ||
        list.saveStateListHead != 0 ||
        list.saveStateListTail != 0 ||
        list.saveStateCount != 0 ||
        list.playerState == 0) {
        return 1;
    }

    for (std::uint8_t value : list.playerState->bytes) {
        if (value != 0) {
            std::free(list.playerState);
            return 2;
        }
    }

    zUtil_SaveGameState *const first = zUtil_SaveGameStateList_AllocAppend(&list);
    zUtil_SaveGameState *const second = zUtil_SaveGameStateList_AllocAppend(&list);
    const bool linked =
        first != 0 &&
        second != 0 &&
        first->next == second &&
        second->next == 0 &&
        list.firstSaveState == first &&
        list.saveStateListHead == first &&
        list.saveStateListTail == second &&
        list.saveStateCount == 2;

    std::free(first);
    std::free(second);
    std::free(list.playerState);
    return linked ? 0 : 3;
}

extern "C" int zutil_save_game_state_free_owned_resources_smoke(void) {
    zUtil_SaveGameState saveState = {};
    saveState.playerState =
        (zUtil_PlayerStateStorage *)(std::malloc(sizeof(zUtil_PlayerStateStorage)));
    if (saveState.playerState == 0) {
        return 1;
    }

    GameNetPlayerRow *const row =
        (GameNetPlayerRow *)(std::malloc(sizeof(GameNetPlayerRow)));
    if (row == 0) {
        std::free(saveState.playerState);
        return 2;
    }

    memset(row, 0, sizeof(GameNetPlayerRow));
    row->saveState = (GameNetPlayerSaveState *)(&saveState);
    saveState.netPlayerRow = row;

    PlayerModalState *const firstModal =
        (PlayerModalState *)(std::malloc(sizeof(PlayerModalState)));
    PlayerModalState *const secondModal =
        (PlayerModalState *)(std::malloc(sizeof(PlayerModalState)));
    if (firstModal == 0 || secondModal == 0) {
        std::free(firstModal);
        std::free(secondModal);
        std::free(row);
        std::free(saveState.playerState);
        return 2;
    }

    memset(saveState.playerState, 0, sizeof(zUtil_PlayerStateStorage));
    memset(firstModal, 0, sizeof(PlayerModalState));
    memset(secondModal, 0, sizeof(PlayerModalState));

    firstModal->next = secondModal;
    saveState.primaryModalState = firstModal;
    saveState.modalStateListHead = firstModal;
    saveState.modalStateListTail = secondModal;
    saveState.modalStateCount = 2;
    saveState.modalStateListAux = 1;

    saveState.FreeOwnedResources();

    const int result =
        row->saveState == 0 &&
                   saveState.primaryModalState == 0 &&
                   saveState.modalStateListHead == 0 &&
                   saveState.modalStateListTail == 0 &&
                   saveState.modalStateCount == 0 &&
                   saveState.modalStateListAux == 0
               ? 0
               : 3;
    std::free(row);
    return result;
}
