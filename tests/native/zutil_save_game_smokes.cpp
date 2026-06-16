#include "GameZRecoil/zUtil/zSaveGame.h"

#include <cstdint>
#include <cstdlib>

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
