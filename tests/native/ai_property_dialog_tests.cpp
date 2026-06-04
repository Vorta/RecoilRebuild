#include "Battlesport/AiPropertyDlg.h"

#include <cstring>

namespace {
struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

int g_onDestroyCalls;
void *g_onDestroyThis;
int g_sendMessageStep;
HWND g_propertyComboHwnd;
HWND g_behaviorComboHwnd;
HWND g_sendMessageHwnd[8];
UINT g_sendMessageMsg[8];
WPARAM g_sendMessageWParam[8];
int g_showCursorCalls;
BOOL g_showCursorValue;
int g_setDlgItemTextCalls;
void *g_setDlgItemTextThis[8];
int g_setDlgItemTextId[8];
char g_setDlgItemTextValue[8][64];

bool PatchImportByOrdinal(
    const char *dllName,
    WORD ordinal,
    void *replacement,
    ImportFunctionPatch &patch
) {
    unsigned char *const imageBase = (unsigned char *)GetModuleHandleA(0);
    IMAGE_DOS_HEADER *const dos = (IMAGE_DOS_HEADER *)imageBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = (IMAGE_NT_HEADERS *)(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        (IMAGE_IMPORT_DESCRIPTOR *)(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll = (const char *)(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(
            imageBase + (descriptor->OriginalFirstThunk != 0
                             ? descriptor->OriginalFirstThunk
                             : descriptor->FirstThunk)
        );
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (!IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal) ||
                (WORD)(nameThunk->u1.Ordinal & 0xffff) != ordinal) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
                return false;
            }

            *patch.slot = (ULONG_PTR)replacement;
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

bool PatchImportByName(
    const char *dllName,
    const char *functionName,
    void *replacement,
    ImportFunctionPatch &patch
) {
    unsigned char *const imageBase = (unsigned char *)GetModuleHandleA(0);
    IMAGE_DOS_HEADER *const dos = (IMAGE_DOS_HEADER *)imageBase;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return false;
    }

    IMAGE_NT_HEADERS *const nt = (IMAGE_NT_HEADERS *)(imageBase + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return false;
    }

    const IMAGE_DATA_DIRECTORY &imports =
        nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (imports.VirtualAddress == 0) {
        return false;
    }

    IMAGE_IMPORT_DESCRIPTOR *descriptor =
        (IMAGE_IMPORT_DESCRIPTOR *)(imageBase + imports.VirtualAddress);
    for (; descriptor->Name != 0; ++descriptor) {
        const char *const importedDll = (const char *)(imageBase + descriptor->Name);
        if (_stricmp(importedDll, dllName) != 0) {
            continue;
        }

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(
            imageBase + (descriptor->OriginalFirstThunk != 0
                             ? descriptor->OriginalFirstThunk
                             : descriptor->FirstThunk)
        );
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)) {
                continue;
            }

            IMAGE_IMPORT_BY_NAME *importName =
                (IMAGE_IMPORT_BY_NAME *)(imageBase + nameThunk->u1.AddressOfData);
            if (std::strcmp((const char *)importName->Name, functionName) != 0) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0) {
                return false;
            }

            *patch.slot = (ULONG_PTR)replacement;
            DWORD ignored = 0;
            VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
            FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
            return true;
        }
    }

    return false;
}

void RestoreImportPatch(ImportFunctionPatch &patch)
{
    if (patch.slot == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.slot,
            sizeof(*patch.slot),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }

    patch.slot = 0;
    patch.original = 0;
}

void __fastcall FakeCWndOnDestroy(void *self, void *)
{
    ++g_onDestroyCalls;
    g_onDestroyThis = self;
}

LRESULT WINAPI FakeSendMessageA(HWND hwnd, UINT msg, WPARAM wParam, LPARAM)
{
    const int index = g_sendMessageStep++;
    if (index >= 0 && index < 8) {
        g_sendMessageHwnd[index] = hwnd;
        g_sendMessageMsg[index] = msg;
        g_sendMessageWParam[index] = wParam;
    }

    if (msg == CB_GETCURSEL && hwnd == g_propertyComboHwnd) {
        return 3;
    }
    if (msg == CB_GETITEMDATA && hwnd == g_propertyComboHwnd) {
        return 11;
    }
    if (msg == CB_GETCURSEL && hwnd == g_behaviorComboHwnd) {
        return 4;
    }
    if (msg == CB_GETITEMDATA && hwnd == g_behaviorComboHwnd) {
        return 2;
    }
    return 0;
}

int WINAPI FakeShowCursor(BOOL show)
{
    ++g_showCursorCalls;
    g_showCursorValue = show;
    return 0;
}

void __fastcall FakeSetDlgItemTextA(
    void *self,
    void *,
    int controlId,
    LPCSTR text
) {
    const int index = g_setDlgItemTextCalls++;
    if (index >= 0 && index < 8) {
        g_setDlgItemTextThis[index] = self;
        g_setDlgItemTextId[index] = controlId;
        std::strncpy(g_setDlgItemTextValue[index], text, 63);
        g_setDlgItemTextValue[index][63] = 0;
    }
}

void ResetDialog(AiPropertyDlg *dialog)
{
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->m_propertyCombo.m_hWnd = g_propertyComboHwnd;
    dialog->m_behaviorCombo.m_hWnd = g_behaviorComboHwnd;
}

bool LabelScenarioOk(AiPropertyDlg *dialog, LRESULT behaviorIndex, const char *first, const char *second)
{
    g_setDlgItemTextCalls = 0;
    std::memset(g_setDlgItemTextValue, 0, sizeof(g_setDlgItemTextValue));
    dialog->m_selectedBehaviorIndex = behaviorIndex;

    dialog->UpdatePropertyLabels();

    return g_setDlgItemTextCalls == 2 &&
           g_setDlgItemTextThis[0] == dialog &&
           g_setDlgItemTextThis[1] == dialog &&
           g_setDlgItemTextId[0] == 1107 &&
           g_setDlgItemTextId[1] == 1108 &&
           std::strcmp(g_setDlgItemTextValue[0], first) == 0 &&
           std::strcmp(g_setDlgItemTextValue[1], second) == 0;
}
}

extern "C" int ai_property_dialog_on_destroy_smoke(void)
{
    const WORD kMfc42CWndOnDestroyOrdinal = 6453;
    ImportFunctionPatch patches[3] = {};
    const bool installed =
        PatchImportByOrdinal(
            "MFC42.DLL",
            kMfc42CWndOnDestroyOrdinal,
            (void *)&FakeCWndOnDestroy,
            patches[0]
        ) &&
        PatchImportByName(
            "USER32.dll",
            "SendMessageA",
            (void *)&FakeSendMessageA,
            patches[1]
        ) &&
        PatchImportByName(
            "USER32.dll",
            "ShowCursor",
            (void *)&FakeShowCursor,
            patches[2]
        );

    unsigned char buffer[sizeof(AiPropertyDlg)];
    AiPropertyDlg *const dialog = (AiPropertyDlg *)buffer;
    g_propertyComboHwnd = (HWND)0x1234;
    g_behaviorComboHwnd = (HWND)0x5678;
    ResetDialog(dialog);

    g_onDestroyCalls = 0;
    g_onDestroyThis = 0;
    g_sendMessageStep = 0;
    g_showCursorCalls = 0;
    g_showCursorValue = TRUE;

    int result = 0;
    if (!installed) {
        result = 1;
    } else {
        dialog->OnDestroy();
        if (g_onDestroyCalls != 1 || g_onDestroyThis != dialog) {
            result = 2;
        } else if (
            g_sendMessageStep != 4 ||
            g_sendMessageHwnd[0] != g_propertyComboHwnd ||
            g_sendMessageMsg[0] != CB_GETCURSEL ||
            g_sendMessageHwnd[1] != g_propertyComboHwnd ||
            g_sendMessageMsg[1] != CB_GETITEMDATA ||
            g_sendMessageWParam[1] != 3 ||
            g_sendMessageHwnd[2] != g_behaviorComboHwnd ||
            g_sendMessageMsg[2] != CB_GETCURSEL ||
            g_sendMessageHwnd[3] != g_behaviorComboHwnd ||
            g_sendMessageMsg[3] != CB_GETITEMDATA ||
            g_sendMessageWParam[3] != 4
        ) {
            result = 3;
        } else if (
            dialog->m_selectedPropertyIndex != 11 ||
            dialog->m_selectedBehaviorIndex != 2 ||
            g_showCursorCalls != 1 ||
            g_showCursorValue != FALSE
        ) {
            result = 4;
        }
    }

    for (int index = 2; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    return result;
}

extern "C" int ai_property_dialog_on_sel_change_smoke(void)
{
    const WORD kMfc42CWndSetDlgItemTextAOrdinal = 5953;
    ImportFunctionPatch patches[2] = {};
    const bool installed =
        PatchImportByName(
            "USER32.dll",
            "SendMessageA",
            (void *)&FakeSendMessageA,
            patches[0]
        ) &&
        PatchImportByOrdinal(
            "MFC42.DLL",
            kMfc42CWndSetDlgItemTextAOrdinal,
            (void *)&FakeSetDlgItemTextA,
            patches[1]
        );

    unsigned char buffer[sizeof(AiPropertyDlg)];
    AiPropertyDlg *const dialog = (AiPropertyDlg *)buffer;
    g_propertyComboHwnd = (HWND)0x1234;
    g_behaviorComboHwnd = (HWND)0x5678;
    ResetDialog(dialog);

    g_sendMessageStep = 0;
    g_setDlgItemTextCalls = 0;

    int result = 0;
    if (!installed) {
        result = 1;
    } else {
        dialog->OnSelChange();
        if (
            g_sendMessageStep != 2 ||
            g_sendMessageHwnd[0] != g_behaviorComboHwnd ||
            g_sendMessageMsg[0] != CB_GETCURSEL ||
            g_sendMessageHwnd[1] != g_behaviorComboHwnd ||
            g_sendMessageMsg[1] != CB_GETITEMDATA ||
            g_sendMessageWParam[1] != 4 ||
            dialog->m_selectedBehaviorIndex != 2
        ) {
            result = 2;
        } else if (
            g_setDlgItemTextCalls != 2 ||
            std::strcmp(g_setDlgItemTextValue[0], "Attack Range") != 0 ||
            std::strcmp(g_setDlgItemTextValue[1], "Movement") != 0
        ) {
            result = 3;
        }
    }

    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    return result;
}

extern "C" int ai_property_dialog_update_property_labels_smoke(void)
{
    const WORD kMfc42CWndSetDlgItemTextAOrdinal = 5953;
    ImportFunctionPatch patch = {};
    const bool installed =
        PatchImportByOrdinal(
            "MFC42.DLL",
            kMfc42CWndSetDlgItemTextAOrdinal,
            (void *)&FakeSetDlgItemTextA,
            patch
        );

    unsigned char buffer[sizeof(AiPropertyDlg)];
    AiPropertyDlg *const dialog = (AiPropertyDlg *)buffer;
    g_propertyComboHwnd = (HWND)0x1234;
    g_behaviorComboHwnd = (HWND)0x5678;
    ResetDialog(dialog);

    int result = 0;
    if (!installed) {
        result = 1;
    } else if (!LabelScenarioOk(dialog, 0, "Min Pursuit Rng", "Max Pursuit Rng")) {
        result = 2;
    } else if (!LabelScenarioOk(dialog, 1, "Attack Range", "Unused")) {
        result = 3;
    } else if (!LabelScenarioOk(dialog, 2, "Attack Range", "Movement")) {
        result = 4;
    } else if (!LabelScenarioOk(dialog, 3, "Unused", "Unused")) {
        result = 5;
    } else if (!LabelScenarioOk(dialog, 6, "", "")) {
        result = 6;
    }

    RestoreImportPatch(patch);
    return result;
}
