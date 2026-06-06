#include "Battlesport/hud.h"

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {
struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

template <typename Method> void *MethodAddress(Method method) {
    union MethodToFunction {
        Method method;
        void *function;
    };

    MethodToFunction thunk{};
    thunk.method = method;
    return thunk.function;
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(
        patch.original,
        patch.address,
        sizeof(patch.original)
    );

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        patch.address = nullptr;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(
        patch.address,
        sizeof(patch.original),
        oldProtect,
        &ignored
    );
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == nullptr) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(
            patch.address,
            patch.original,
            sizeof(patch.original)
        );
        DWORD ignored = 0;
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            oldProtect,
            &ignored
        );
        FlushInstructionCache(
            GetCurrentProcess(),
            patch.address,
            sizeof(patch.original)
        );
    }

    patch.address = nullptr;
}

int g_saveGameInitLoadCalls;
bool g_saveGameInitLoadArgsOk;

struct SaveGameInitLoadProbe {
    zReader::Node * LoadFromZrd(
        const char *zrdPath,
        const char *sectionName,
        int capturePrimary
    );
};

zReader::Node * SaveGameInitLoadProbe::LoadFromZrd(
    const char *zrdPath,
    const char *sectionName,
    int capturePrimary
) {
    ++g_saveGameInitLoadCalls;
    g_saveGameInitLoadArgsOk =
        this != nullptr &&
        zrdPath != nullptr &&
        std::strcmp(
            zrdPath,
            "dialog.zrd"
        ) == 0 &&
        sectionName != nullptr &&
        std::strcmp(
            sectionName,
            "SAVE_GAME_DIALOG"
        ) == 0 &&
        capturePrimary == 0;
    return nullptr;
}

void *HudUiBackgroundLoadFromZrdAddress() {
    return MethodAddress(&HudUiBackground::LoadFromZrd);
}

void *SaveGameInitLoadProbeAddress() {
    return MethodAddress(&SaveGameInitLoadProbe::LoadFromZrd);
}

void SetWriteTime(
    HudUiSaveLoadEntry *entry,
    DWORD lowPart
) {
    std::memset(
        entry,
        0,
        sizeof(*entry)
    );
    entry->ftLastWriteTime.dwLowDateTime = lowPart;
    entry->ftLastWriteTime.dwHighDateTime = 0;
}

} // namespace

extern "C" int hud_ui_save_load_entry_is_newer_than_smoke(void) {
    HudUiSaveLoadEntry older;
    HudUiSaveLoadEntry same;
    HudUiSaveLoadEntry newer;

    SetWriteTime(
        &older,
        100
    );
    same = older;
    SetWriteTime(
        &newer,
        101
    );

    return newer.IsNewerThan(&older) == 1 && older.IsNewerThan(&newer) == 0 &&
                   older.IsNewerThan(&same) == 0
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_list_item_constructor_smoke(void) {
    HudUiSaveLoadListItem item{};

    HudUiSaveLoadListItem *const result = item.Constructor();

    return result == &item && item.layoutY == 32767 && item.layoutX == -1 &&
                   item.parent == 0 && item.next == 0 && item.textPick == 0
               ? 0
               : 1;
}

extern "C" int hud_ui_container_constructor_smoke(void) {
    HudUiContainer container;

    return container.enabled == 0 && container.childHead == nullptr &&
                   container.childTail == nullptr
               ? 0
               : 1;
}

extern "C" int hud_ui_container_set_enabled_smoke(void) {
    HudUiContainer container;

    container.SetEnabled(1);
    const bool enabled = container.enabled == 1;

    container.SetEnabled(0);
    return enabled && container.enabled == 0 ? 0 : 1;
}

extern "C" int hud_ui_background_container_constructor_smoke(void) {
    HudUiBackgroundContainer container(3);

    return container.enabled == 0 && container.childHead == nullptr &&
                   container.childTail == nullptr &&
                   container.inputFocusElement == nullptr &&
                   container.captureTransitionMask == 3
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_insert_entry_sorted_prefix_smoke(void) {
    HudUiSaveLoadEntry entries[4];
    SetWriteTime(
        &entries[0],
        400
    );
    SetWriteTime(
        &entries[1],
        100
    );

    HudUiSaveLoadEntry middle;
    SetWriteTime(
        &middle,
        250
    );
    HudUiSaveLoadDialog::InsertEntryIntoSortedPrefix(
        &entries[2],
        middle
    );

    const bool insertedMiddle = entries[0].ftLastWriteTime.dwLowDateTime == 400 &&
                                entries[1].ftLastWriteTime.dwLowDateTime == 250 &&
                                entries[2].ftLastWriteTime.dwLowDateTime == 100;

    HudUiSaveLoadEntry oldest;
    SetWriteTime(
        &oldest,
        50
    );
    HudUiSaveLoadDialog::InsertEntryIntoSortedPrefix(
        &entries[3],
        oldest
    );

    return insertedMiddle && entries[0].ftLastWriteTime.dwLowDateTime == 400 &&
                   entries[1].ftLastWriteTime.dwLowDateTime == 250 &&
                   entries[2].ftLastWriteTime.dwLowDateTime == 100 &&
                   entries[3].ftLastWriteTime.dwLowDateTime == 50
               ? 0
               : 1;
}

extern "C" int hud_ui_save_load_partition_entries_by_pivot_smoke(void) {
    HudUiSaveLoadEntry entries[5];
    SetWriteTime(
        &entries[0],
        300
    );
    SetWriteTime(
        &entries[1],
        100
    );
    SetWriteTime(
        &entries[2],
        500
    );
    SetWriteTime(
        &entries[3],
        200
    );
    SetWriteTime(
        &entries[4],
        400
    );

    HudUiSaveLoadEntry pivot;
    SetWriteTime(
        &pivot,
        300
    );

    HudUiSaveLoadEntry *const split = HudUiSaveLoadDialog::PartitionEntriesByPivot(
        entries,
        entries + 5,
        pivot
    );
    if (split < entries || split > entries + 5) {
        return 1;
    }

    for (HudUiSaveLoadEntry *entry = entries; entry != split; ++entry) {
        if (pivot.IsNewerThan(entry) != 0) {
            return 1;
        }
    }

    for (HudUiSaveLoadEntry *entry = split; entry != entries + 5; ++entry) {
        if (entry->IsNewerThan(&pivot) != 0) {
            return 1;
        }
    }

    return 0;
}

extern "C" int hud_ui_save_load_sort_entry_range_smoke(void) {
    HudUiSaveLoadEntry smallEntries[16];
    for (int i = 0; i < 16; ++i) {
        SetWriteTime(
            &smallEntries[i],
            (DWORD)(i + 1)
        );
    }

    HudUiSaveLoadDialog::SortEntryRange(
        smallEntries,
        smallEntries + 16,
        0
    );
    for (int i = 0; i < 16; ++i) {
        if (smallEntries[i].ftLastWriteTime.dwLowDateTime != (DWORD)(i + 1)) {
            return 1;
        }
    }

    HudUiSaveLoadEntry largeEntries[17];
    for (int i = 0; i < 17; ++i) {
        SetWriteTime(
            &largeEntries[i],
            (DWORD)((i + 1) * 100)
        );
    }

    HudUiSaveLoadDialog::SortEntryRange(
        largeEntries,
        largeEntries + 17,
        0
    );
    for (int i = 0; i < 17; ++i) {
        const DWORD expectedTime = (DWORD)((17 - i) * 100);
        if (largeEntries[i].ftLastWriteTime.dwLowDateTime != expectedTime) {
            return 1;
        }
    }

    return 0;
}

extern "C" int hud_ui_save_game_dialog_init_layout_smoke(void) {
    void *const storage = ::operator new(sizeof(HudUiSaveGameDialog));
    std::memset(
        storage,
        0,
        sizeof(HudUiSaveGameDialog)
    );
    HudUiSaveGameDialog *const dialog = static_cast<HudUiSaveGameDialog *>(storage);

    CodeFunctionPatch loadPatch{};
    if (!PatchFunctionJump(
            HudUiBackgroundLoadFromZrdAddress(),
            SaveGameInitLoadProbeAddress(),
            loadPatch
        )) {
        ::operator delete(storage);
        return 1;
    }

    g_saveGameInitLoadCalls = 0;
    g_saveGameInitLoadArgsOk = false;
    HudUiSaveGameDialog *const result = dialog->InitLayout();
    RestoreFunctionPatch(loadPatch);

    int checkResult = 0;
    if (result != dialog ||
        g_saveGameInitLoadCalls != 1 ||
        !g_saveGameInitLoadArgsOk ||
        dialog->gameNameInput.textInput.buffer == nullptr ||
        dialog->gameNameInput.textInput.capacity != 20 ||
        std::strcmp(
            dialog->gameNameInput.textInput.buffer,
            ""
        ) != 0 ||
        dialog->gameNameInput.sliderBorder.inputActive != 1 ||
        dialog->selectedEntryIndex != -1) {
        checkResult = 2;
    }

    const int expectedLayoutY[9] = {
        9830,
        16383,
        32767,
        32767,
        32767,
        29490,
        22936,
        16383,
        9830
    };
    for (int i = 0; i < 9; ++i) {
        if (dialog->entryWidgets[i].layoutY != expectedLayoutY[i] ||
            (dialog->entryWidgets[i].flags & 0x10u) == 0) {
            checkResult = 3;
        }
    }

    dialog->~HudUiSaveGameDialog();
    ::operator delete(storage);
    return checkResult;
}

extern "C" int hud_ui_save_load_set_selected_entry_index_smoke(void) {
    HudUiSaveLoadDialog dialog{};
    HudUiSaveLoadEntry entries[5];

    std::memset(
        entries,
        0,
        sizeof(entries)
    );

    dialog.gameNameInput.BaseConstructor();
    dialog.gameNameInput.AllocTextBuffer(32);
    for (int i = 0; i < 9; ++i) {
        dialog.entryWidgets[i].Constructor();
    }
    for (int i = 0; i < 5; ++i) {
        std::sprintf(
            entries[i].cFileName,
            "entry%d.sav",
            i
        );
    }

    dialog.fileEntries.begin = entries;
    dialog.fileEntries.end = entries + 5;
    dialog.fileEntries.capacityEnd = entries + 5;

    dialog.SetSelectedEntryIndex(2);

    int result = 0;
    if (dialog.selectedEntryIndex != 2 ||
        std::strcmp(
            dialog.gameNameInput.GetBuffer(),
            "entry2.sav"
        ) != 0) {
        result = 1;
    }

    const int expectedLayout[9] = {-1, 0, 1, 3, 4, -1, -1, -1, -1};
    const int expectedHidden[9] = {1, 0, 0, 0, 0, 1, 1, 1, 1};
    const char *const expectedText[9] = {
        "",
        "entry0.sav",
        "entry1.sav",
        "entry3.sav",
        "entry4.sav",
        "",
        "",
        "",
        ""
    };

    for (int i = 0; i < 9; ++i) {
        HudUiSaveLoadListItem *const item = &dialog.entryWidgets[i];
        if (item->layoutX != expectedLayout[i]) {
            result = 1;
        }
        if (((item->flags & 0x10u) != 0 ? 1 : 0) != expectedHidden[i]) {
            result = 1;
        }
        if (std::strcmp(
                item->GetLastTextPtr(),
                expectedText[i]
            ) != 0) {
            result = 1;
        }
    }

    dialog.gameNameInput.Destructor();
    for (int i = 8; i >= 0; --i) {
        dialog.entryWidgets[i].HudUiPanel::Destructor();
    }

    return result;
}
