#include "Battlesport/game_net.h"
#include "Battlesport/mission.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zLoc/zloc.h"

#include <cstring>

namespace {
int g_dtorStep;
int g_dtorOrder[7];
void *g_dtorThis[7];
int g_ddxStep;
int g_ddxKind[13];
int g_ddxIdOrLimit[13];
void *g_ddxContext[13];
void *g_ddxValue[13];
unsigned int g_ddxMin[13];
unsigned int g_ddxMax[13];
int g_onDestroyStep;
void *g_onDestroyThis;
int g_sendMessageStep;
HWND g_sendMessageHwnd;
HWND g_sendMessageHwndByCall[20];
UINT g_sendMessageMsgByCall[20];
WPARAM g_sendMessageWParamByCall[20];
LPARAM g_sendMessageLParamByCall[20];
int g_addStringCalls;
LRESULT g_selectedComboIndex;
LRESULT g_selectedItemData;
WPARAM g_itemDataWParam;
int g_onInitBaseCalls;
void *g_onInitBaseThis;
int g_updateDataCalls;
BOOL g_updateDataSave;
int g_getNetworkModemEnabledCalls;
int g_getNetworkModemEnabledResult;
int g_setDlgItemTextCalls;
void *g_setDlgItemTextThis;
int g_setDlgItemTextId;
const char *g_setDlgItemTextValue;
int g_atexitCalls;
void(*g_atexitCallback)(void);
int g_atexitResult;

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

bool TestMfcWindowConstructed(CWnd &wnd) {
    return *(void **)&wnd != 0 && wnd.m_hWnd == 0;
}

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

        IMAGE_THUNK_DATA *nameThunk =
            (IMAGE_THUNK_DATA *)(imageBase +
                (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                     : descriptor->FirstThunk));
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (
                !IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)
                || (WORD)(nameThunk->u1.Ordinal & 0xffff) != ordinal
            ) {
                continue;
            }

            DWORD oldProtect = 0;
            patch.slot = &addressThunk->u1.Function;
            patch.original = addressThunk->u1.Function;
            if (
                VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0
            ) {
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

        IMAGE_THUNK_DATA *nameThunk =
            (IMAGE_THUNK_DATA *)(imageBase +
                (descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                     : descriptor->FirstThunk));
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
            if (
                VirtualProtect(
                    patch.slot,
                    sizeof(*patch.slot),
                    PAGE_EXECUTE_READWRITE,
                    &oldProtect
                ) == 0
            ) {
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

void RestoreImportPatch(ImportFunctionPatch &patch) {
    if (patch.slot == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (
        VirtualProtect(
            patch.slot,
            sizeof(*patch.slot),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0
    ) {
        *patch.slot = patch.original;
        DWORD ignored = 0;
        VirtualProtect(patch.slot, sizeof(*patch.slot), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.slot, sizeof(*patch.slot));
    }

    patch.slot = 0;
    patch.original = 0;
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch) {
    if (target == 0) {
        patch.address = 0;
        return false;
    }

    patch.address = (unsigned char *)target;
    std::memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0
    ) {
        patch.address = 0;
        return false;
    }

    patch.address[0] = 0xe9;
    const LONG relativeOffset =
        (LONG)((unsigned char *)replacement - (patch.address + sizeof(patch.original)));
    std::memcpy(patch.address + 1, &relativeOffset, sizeof(relativeOffset));

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0
    ) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = 0;
}

void *CWndUpdateDataAddress() {
    int (CWnd::*method)(BOOL) = &CWnd::UpdateData;
    void *address = 0;
    std::memcpy(&address, &method, sizeof(address));
    return address;
}

template <typename Method>
ULONG_PTR MemberPointerBits(Method method) {
    ULONG_PTR bits = 0;
    std::memcpy(&bits, &method, sizeof(method));
    return bits;
}

ULONG_PTR MsgMapEntryHandlerBits(const AFX_MSGMAP_ENTRY &entry) {
    ULONG_PTR bits = 0;
    std::memcpy(&bits, &entry.pfn, sizeof(entry.pfn));
    return bits;
}

void RecordDtor(void *self, int stepId) {
    const int index = g_dtorStep;
    if (index < 7) {
        g_dtorOrder[index] = stepId;
        g_dtorThis[index] = self;
    }
    ++g_dtorStep;
}

void __fastcall FakeCStringDtor(void *self) {
    RecordDtor(self, 1);
}

void __fastcall FakeComboDtor(void *self) {
    RecordDtor(self, 2);
}

void __fastcall FakeSpinDtor(void *self) {
    RecordDtor(self, 3);
}

void __fastcall FakeDialogDtor(void *self) {
    RecordDtor(self, 7);
}

int FakeAtexit(void(*callback)(void)) {
    ++g_atexitCalls;
    g_atexitCallback = callback;
    return g_atexitResult;
}

BOOL __fastcall FakeOnInitBase(void *self) {
    ++g_onInitBaseCalls;
    g_onInitBaseThis = self;
    return TRUE;
}

int __fastcall FakeUpdateData(void *, void *, BOOL saveAndValidate) {
    ++g_updateDataCalls;
    g_updateDataSave = saveAndValidate;
    return 1;
}

int FakeGetNetworkModemEnabled() {
    ++g_getNetworkModemEnabledCalls;
    return g_getNetworkModemEnabledResult;
}

void RecordDdx(CDataExchange *dataExchange, int kind, int idOrLimit, void *value) {
    const int index = g_ddxStep;
    if (index < 13) {
        g_ddxKind[index] = kind;
        g_ddxIdOrLimit[index] = idOrLimit;
        g_ddxContext[index] = dataExchange;
        g_ddxValue[index] = value;
        g_ddxMin[index] = 0;
        g_ddxMax[index] = 0;
    }
    ++g_ddxStep;
}

void __stdcall FakeDDXControl(CDataExchange *dataExchange, int controlId, void *control) {
    RecordDdx(dataExchange, 1, controlId, control);
}

void __stdcall FakeDDXTextCString(CDataExchange *dataExchange, int controlId, CString *value) {
    RecordDdx(dataExchange, 2, controlId, value);
}

void __stdcall FakeDDVMaxChars(CDataExchange *dataExchange, CString *value, int maxChars) {
    RecordDdx(dataExchange, 3, maxChars, value);
}

void __stdcall FakeDDXTextUInt(
    CDataExchange *dataExchange,
    int controlId,
    unsigned int *value
) {
    RecordDdx(dataExchange, 4, controlId, value);
}

void __stdcall FakeDDVMinMaxUInt(
    CDataExchange *dataExchange,
    unsigned int value,
    unsigned int minValue,
    unsigned int maxValue
) {
    RecordDdx(dataExchange, 5, (int)value, 0);
    const int index = g_ddxStep - 1;
    if (index >= 0 && index < 13) {
        g_ddxMin[index] = minValue;
        g_ddxMax[index] = maxValue;
    }
}

void __stdcall FakeDDXCheck(CDataExchange *dataExchange, int controlId, int *value) {
    RecordDdx(dataExchange, 6, controlId, value);
}

void __fastcall FakeCWndOnDestroy(void *self, void *) {
    ++g_onDestroyStep;
    g_onDestroyThis = self;
}

LRESULT WINAPI FakeSendMessageA(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    const int callIndex = g_sendMessageStep;
    if (callIndex < 20) {
        g_sendMessageHwndByCall[callIndex] = hwnd;
        g_sendMessageMsgByCall[callIndex] = message;
        g_sendMessageWParamByCall[callIndex] = wParam;
        g_sendMessageLParamByCall[callIndex] = lParam;
    }
    ++g_sendMessageStep;
    g_sendMessageHwnd = hwnd;
    if (message == CB_ADDSTRING) {
        return g_addStringCalls++;
    }
    if (message == CB_GETCURSEL && wParam == 0 && lParam == 0) {
        return g_selectedComboIndex;
    }
    if (message == CB_GETITEMDATA && lParam == 0) {
        g_itemDataWParam = wParam;
        return g_selectedItemData;
    }
    return 0;
}

void ResetOnInitLog(int modemEnabled) {
    g_sendMessageStep = 0;
    g_sendMessageHwnd = 0;
    std::memset(g_sendMessageHwndByCall, 0, sizeof(g_sendMessageHwndByCall));
    std::memset(g_sendMessageMsgByCall, 0, sizeof(g_sendMessageMsgByCall));
    std::memset(g_sendMessageWParamByCall, 0, sizeof(g_sendMessageWParamByCall));
    std::memset(g_sendMessageLParamByCall, 0, sizeof(g_sendMessageLParamByCall));
    g_addStringCalls = 0;
    g_onInitBaseCalls = 0;
    g_onInitBaseThis = 0;
    g_updateDataCalls = 0;
    g_updateDataSave = TRUE;
    g_getNetworkModemEnabledCalls = 0;
    g_getNetworkModemEnabledResult = modemEnabled;
    g_setDlgItemTextCalls = 0;
    g_setDlgItemTextThis = 0;
    g_setDlgItemTextId = 0;
    g_setDlgItemTextValue = 0;
}

char *__fastcall FakeGetMessageString(unsigned int messageId) {
    switch (messageId) {
    case 12352:
        return (char *)"Max teams";
    case 12353:
        return (char *)"Max players";
    default:
        return (char *)"";
    }
}

void __fastcall FakeSetDlgItemTextA(void *self, void *, int controlId, LPCSTR text) {
    ++g_setDlgItemTextCalls;
    g_setDlgItemTextThis = self;
    g_setDlgItemTextId = controlId;
    g_setDlgItemTextValue = text;
}
} // namespace

extern "C" int net_session_config_dialog_get_message_map_smoke(void) {
    unsigned char dialogStorage[sizeof(NetSessionConfigDialog)] = {0};
    NetSessionConfigDialog &dialog = *(NetSessionConfigDialog *)dialogStorage;
    const AFX_MSGMAP *const messageMap = dialog.NetSessionConfigDialog::GetMessageMap();
    if (
        messageMap != &NetSessionConfigDialog::messageMap
        || messageMap->pfnGetBaseMap == 0
        || messageMap->pfnGetBaseMap() == 0
        || messageMap->lpEntries != &NetSessionConfigDialog::messageEntries[0]
    ) {
        return 10;
    }

    const AFX_MSGMAP_ENTRY *const entries = messageMap->lpEntries;
    const bool destroyEntryOk =
        entries[0].nMessage == WM_DESTROY &&
        entries[0].nCode == 0 &&
        entries[0].nID == 0 &&
        entries[0].nLastID == 0 &&
        entries[0].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[0]) == MemberPointerBits(&NetSessionConfigDialog::OnDestroy);
    const bool mapChangedEntryOk =
        entries[1].nMessage == WM_COMMAND &&
        entries[1].nCode == CBN_SELCHANGE &&
        entries[1].nID == 1116 &&
        entries[1].nLastID == 1116 &&
        entries[1].nSig == 12 &&
        MsgMapEntryHandlerBits(entries[1]) == MemberPointerBits(&NetSessionConfigDialog::OnMapChanged);
    const bool sentinelOk =
        entries[2].nMessage == 0 &&
        entries[2].nCode == 0 &&
        entries[2].nID == 0 &&
        entries[2].nLastID == 0 &&
        entries[2].nSig == 0 &&
        MsgMapEntryHandlerBits(entries[2]) == 0;

    return destroyEntryOk && mapChangedEntryOk && sentinelOk ? 0 : 11;
}

extern "C" int net_session_config_dialog_init_map_name_strings_smoke(void) {
    const char *const expectedNames[7] = {
        "RiverWorks",
        "Crater Chaos",
        "Beach Rally",
        "Clone City",
        "Frozen Tundra",
        "Poison Valley",
        "New Clone City"
    };

    NetSessionConfigDialog::InitMapNameStrings();

    int result = 0;
    for (int index = 0; index < 7; ++index) {
        if (
            (const char *)g_NetSessionConfigDialog_MapNameStrings[index] == 0
            || std::strcmp(
                (const char *)g_NetSessionConfigDialog_MapNameStrings[index],
                expectedNames[index]
            ) != 0
        ) {
            result = index + 1;
            break;
        }
    }

    for (int index = 6; index >= 0; --index) {
        g_NetSessionConfigDialog_MapNameStrings[index].~CString();
    }
    return result;
}

extern "C" int net_session_config_dialog_register_map_name_cleanup_smoke(void) {
    CodeFunctionPatch patch = {};
    g_atexitCalls = 0;
    g_atexitCallback = 0;
    g_atexitResult = 17;

    if (!PatchFunctionJump((void *)&atexit, (void *)&FakeAtexit, patch)) {
        return 1;
    }

    NetSessionConfigDialog::RegisterMapNameCleanup();
    RestoreFunctionPatch(patch);

    if (g_atexitCalls != 1) {
        return 2;
    }
    if (g_atexitCallback != &NetSessionConfigDialog::CleanupMapNameStringsOnExit) {
        return 3;
    }
    return 0;
}

extern "C" int mission_register_multiplayer_maps_smoke(void) {
    CodeFunctionPatch patch = {};
    g_atexitCalls = 0;
    g_atexitCallback = 0;
    g_atexitResult = 17;

    if (!PatchFunctionJump((void *)&atexit, (void *)&FakeAtexit, patch)) {
        return 1;
    }

    Mission::RegisterMultiplayerMaps();
    RestoreFunctionPatch(patch);

    const bool ok =
        g_atexitCalls == 1 &&
        g_atexitCallback == &NetSessionConfigDialog::CleanupMapNameStringsOnExit &&
        std::strcmp((const char *)g_NetSessionConfigDialog_MapNameStrings[0], "RiverWorks") == 0 &&
        std::strcmp((const char *)g_NetSessionConfigDialog_MapNameStrings[6], "New Clone City") == 0;
    for (int index = 6; index >= 0; --index) {
        g_NetSessionConfigDialog_MapNameStrings[index].~CString();
    }
    return ok ? 0 : 2;
}

extern "C" int net_session_config_dialog_cleanup_map_name_strings_on_exit_smoke(void) {
    NetSessionConfigDialog::InitMapNameStrings();
    NetSessionConfigDialog::CleanupMapNameStringsOnExit();
    return 0;
}

int RunOnInitScenario(int modemEnabled, LPARAM expectedMaxRange) {
    const char *const expectedNames[7] = {
        "RiverWorks",
        "Crater Chaos",
        "Beach Rally",
        "Clone City",
        "Frozen Tundra",
        "Poison Valley",
        "New Clone City"
    };

    NetSessionConfigDialog *const dialog =
        (NetSessionConfigDialog *)::operator new(sizeof(NetSessionConfigDialog));
    std::memset(dialog, 0, sizeof(*dialog));
    dialog->Constructor(0);
    dialog->m_defaultExerciseOrdinal = 7;
    dialog->m_mapCombo.m_hWnd = (HWND)0x3101;
    dialog->m_timeLimitSpin.m_hWnd = (HWND)0x3102;
    dialog->m_valueLimitSpin.m_hWnd = (HWND)0x3103;
    dialog->m_maxPlayersSpin.m_hWnd = (HWND)0x3104;

    ResetOnInitLog(modemEnabled);
    const BOOL initResult = dialog->NetSessionConfigDialog::OnInitDialog();

    int result = 0;
    if (initResult != TRUE || g_onInitBaseCalls != 1 || g_onInitBaseThis != dialog) {
        result = 1;
    } else if (std::strcmp((const char *)dialog->m_sessionName, "Exercise 007") != 0) {
        result = 2;
    } else if (g_sendMessageStep != 18 || g_addStringCalls != 7) {
        result = 3;
    } else {
        for (int index = 0; index < 7; ++index) {
            const int addCall = index * 2;
            const int dataCall = addCall + 1;
            if (
                g_sendMessageHwndByCall[addCall] != dialog->m_mapCombo.m_hWnd
                || g_sendMessageMsgByCall[addCall] != CB_ADDSTRING
                || std::strcmp((const char *)g_sendMessageLParamByCall[addCall], expectedNames[index]) != 0
                || g_sendMessageHwndByCall[dataCall] != dialog->m_mapCombo.m_hWnd
                || g_sendMessageMsgByCall[dataCall] != CB_SETITEMDATA
                || g_sendMessageWParamByCall[dataCall] != (WPARAM)index
                || g_sendMessageLParamByCall[dataCall] != index
            ) {
                result = 4 + index;
                break;
            }
        }
    }

    if (result == 0) {
        const bool tailMessagesOk =
            g_sendMessageMsgByCall[14] == CB_SETCURSEL &&
            g_sendMessageMsgByCall[15] == 1125 &&
            g_sendMessageLParamByCall[15] == MAKELPARAM(360, 0) &&
            g_sendMessageMsgByCall[16] == 1125 &&
            g_sendMessageLParamByCall[16] == MAKELPARAM(100, 0) &&
            g_sendMessageMsgByCall[17] == 1125 &&
            g_sendMessageLParamByCall[17] == expectedMaxRange;
        const bool stateOk =
            dialog->m_valueLimit == 5 &&
            dialog->m_timeLimitMinutes == 10 &&
            dialog->m_maxPlayers == 8 &&
            dialog->m_unusedCheckboxEnabled == 1 &&
            g_updateDataCalls == 1 &&
            g_updateDataSave == FALSE &&
            g_getNetworkModemEnabledCalls == 1 &&
            g_setDlgItemTextCalls == 1 &&
            g_setDlgItemTextThis == dialog &&
            g_setDlgItemTextId == 1125 &&
            std::strcmp(g_setDlgItemTextValue, "Max players") == 0;
        result = tailMessagesOk && stateOk ? 0 : 20;
    }

    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_config_dialog_on_init_dialog_smoke(void) {
    const WORD kMfc42CDialogOnInitDialogOrdinal = 4710;
    const WORD kMfc42CWndSetDlgItemTextOrdinal = 5953;
    ImportFunctionPatch importPatches[3] = {};
    CodeFunctionPatch functionPatches[3] = {};
    const bool installed =
        PatchImportByName("USER32.dll", "SendMessageA", (void *)&FakeSendMessageA, importPatches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogOnInitDialogOrdinal, (void *)&FakeOnInitBase, importPatches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetDlgItemTextOrdinal, (void *)&FakeSetDlgItemTextA, importPatches[2]) &&
        PatchFunctionJump(CWndUpdateDataAddress(), (void *)&FakeUpdateData, functionPatches[0]) &&
        PatchFunctionJump((void *)&zOpt::GetNetworkModemEnabled, (void *)&FakeGetNetworkModemEnabled, functionPatches[1]) &&
        PatchFunctionJump((void *)&zLoc::GetMessageString, (void *)&FakeGetMessageString, functionPatches[2]);

    NetSessionConfigDialog::InitMapNameStrings();
    int result = 0;
    if (!installed) {
        result = 1;
    } else {
        result = RunOnInitScenario(0, MAKELPARAM(8, 2));
        if (result == 0) {
            const int modemResult = RunOnInitScenario(1, MAKELPARAM(2, 2));
            if (modemResult != 0) {
                result = 100 + modemResult;
            }
        }
    }
    NetSessionConfigDialog::CleanupMapNameStringsOnExit();

    for (int index = 2; index >= 0; --index) {
        RestoreFunctionPatch(functionPatches[index]);
    }
    for (int index = 2; index >= 0; --index) {
        RestoreImportPatch(importPatches[index]);
    }
    return result;
}

extern "C" int net_session_config_dialog_constructor_smoke(void) {
    NetSessionConfigDialog *const dialog =
        (NetSessionConfigDialog *)::operator new(sizeof(NetSessionConfigDialog));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(0);
    const bool ok =
        returned == dialog &&
        *(void **)dialog != 0 &&
        TestMfcWindowConstructed(dialog->m_maxPlayersSpin) &&
        TestMfcWindowConstructed(dialog->m_valueLimitSpin) &&
        TestMfcWindowConstructed(dialog->m_timeLimitSpin) &&
        TestMfcWindowConstructed(dialog->m_mapCombo) &&
        (const char *)dialog->m_sessionName != 0 &&
        std::strcmp((const char *)dialog->m_sessionName, "") == 0 &&
        dialog->m_valueLimit == 0 &&
        dialog->m_timeLimitMinutes == 0 &&
        dialog->m_maxPlayers == 0 &&
        dialog->m_unusedCheckboxEnabled == 0 &&
        offsetof(NetSessionConfigDialog, m_maxPlayersSpin) == 0x68 &&
        offsetof(NetSessionConfigDialog, m_sessionName) == 0x168 &&
        sizeof(NetSessionConfigDialog) == 0x17c;

    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return ok ? 0 : 1;
}

extern "C" int net_session_config_dialog_destructor_smoke(void) {
    const WORD kMfc42CStringDtorOrdinal = 800;
    const WORD kMfc42CComboBoxDtorOrdinal = 616;
    const WORD kMfc42CSpinButtonCtrlDtorOrdinal = 793;
    const WORD kMfc42CDialogDtorOrdinal = 641;
    ImportFunctionPatch patches[4] = {};
    bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42CStringDtorOrdinal, (void *)&FakeCStringDtor, patches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CComboBoxDtorOrdinal, (void *)&FakeComboDtor, patches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CSpinButtonCtrlDtorOrdinal, (void *)&FakeSpinDtor, patches[2]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogDtorOrdinal, (void *)&FakeDialogDtor, patches[3]);

    NetSessionConfigDialog *const dialog =
        (NetSessionConfigDialog *)::operator new(sizeof(NetSessionConfigDialog));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(0);
    dialog->m_sessionName = "session";
    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else {
        g_dtorStep = 0;
        std::memset(g_dtorOrder, 0, sizeof(g_dtorOrder));
        std::memset(g_dtorThis, 0, sizeof(g_dtorThis));

        dialog->Destructor();

        if (g_dtorStep != 6) {
            result = 6;
        } else if (
            g_dtorOrder[0] != 1
            || g_dtorOrder[1] != 2
            || g_dtorOrder[2] != 3
            || g_dtorOrder[3] != 3
            || g_dtorOrder[4] != 3
            || g_dtorOrder[5] != 7
        ) {
            result = 7;
        }
    }

    for (int index = 3; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_config_dialog_do_data_exchange_smoke(void) {
    const WORD kMfc42DDVMaxCharsOrdinal = 2289;
    const WORD kMfc42DDVMinMaxUIntOrdinal = 2297;
    const WORD kMfc42DDXCheckOrdinal = 2301;
    const WORD kMfc42DDXControlOrdinal = 2302;
    const WORD kMfc42DDXTextUIntOrdinal = 2363;
    const WORD kMfc42DDXTextCStringOrdinal = 2370;
    ImportFunctionPatch patches[6] = {};
    bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXControlOrdinal, (void *)&FakeDDXControl, patches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXTextCStringOrdinal, (void *)&FakeDDXTextCString, patches[1]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDVMaxCharsOrdinal, (void *)&FakeDDVMaxChars, patches[2]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXTextUIntOrdinal, (void *)&FakeDDXTextUInt, patches[3]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDVMinMaxUIntOrdinal, (void *)&FakeDDVMinMaxUInt, patches[4]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42DDXCheckOrdinal, (void *)&FakeDDXCheck, patches[5]);

    NetSessionConfigDialog *const dialog =
        (NetSessionConfigDialog *)::operator new(sizeof(NetSessionConfigDialog));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(0);
    dialog->m_valueLimit = 123;
    dialog->m_timeLimitMinutes = 456;
    dialog->m_maxPlayers = 6;
    unsigned char dataExchangeStorage[16] = {};
    CDataExchange *const dataExchange = (CDataExchange *)dataExchangeStorage;
    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else {
        g_ddxStep = 0;
        std::memset(g_ddxKind, 0, sizeof(g_ddxKind));
        std::memset(g_ddxIdOrLimit, 0, sizeof(g_ddxIdOrLimit));
        std::memset(g_ddxContext, 0, sizeof(g_ddxContext));
        std::memset(g_ddxValue, 0, sizeof(g_ddxValue));
        std::memset(g_ddxMin, 0, sizeof(g_ddxMin));
        std::memset(g_ddxMax, 0, sizeof(g_ddxMax));

        dialog->NetSessionConfigDialog::DoDataExchange(dataExchange);

        if (g_ddxStep != 13) {
            result = 4;
        } else if (
            g_ddxKind[0] != 1 || g_ddxIdOrLimit[0] != 1072 || g_ddxValue[0] != &dialog->m_maxPlayersSpin ||
            g_ddxKind[1] != 1 || g_ddxIdOrLimit[1] != 1121 || g_ddxValue[1] != &dialog->m_valueLimitSpin ||
            g_ddxKind[2] != 1 || g_ddxIdOrLimit[2] != 1120 || g_ddxValue[2] != &dialog->m_timeLimitSpin ||
            g_ddxKind[3] != 1 || g_ddxIdOrLimit[3] != 1116 || g_ddxValue[3] != &dialog->m_mapCombo ||
            g_ddxKind[4] != 2 || g_ddxIdOrLimit[4] != 1115 || g_ddxValue[4] != &dialog->m_sessionName ||
            g_ddxKind[5] != 3 || g_ddxIdOrLimit[5] != 80 || g_ddxValue[5] != &dialog->m_sessionName ||
            g_ddxKind[6] != 4 || g_ddxIdOrLimit[6] != 1117 || g_ddxValue[6] != &dialog->m_valueLimit ||
            g_ddxKind[7] != 5 || g_ddxIdOrLimit[7] != 123 || g_ddxMin[7] != 0 || g_ddxMax[7] != 10000 ||
            g_ddxKind[8] != 4 || g_ddxIdOrLimit[8] != 1118 || g_ddxValue[8] != &dialog->m_timeLimitMinutes ||
            g_ddxKind[9] != 5 || g_ddxIdOrLimit[9] != 456 || g_ddxMin[9] != 0 || g_ddxMax[9] != 10000 ||
            g_ddxKind[10] != 4 || g_ddxIdOrLimit[10] != 1119 || g_ddxValue[10] != &dialog->m_maxPlayers ||
            g_ddxKind[11] != 5 || g_ddxIdOrLimit[11] != 6 || g_ddxMin[11] != 2 || g_ddxMax[11] != 8 ||
            g_ddxKind[12] != 6 || g_ddxIdOrLimit[12] != 1122 || g_ddxValue[12] != &dialog->m_unusedCheckboxEnabled
        ) {
            result = 5;
        }
    }

    for (int index = 5; index >= 0; --index) {
        RestoreImportPatch(patches[index]);
    }

    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return result;
}

extern "C" int net_session_config_dialog_on_destroy_smoke(void) {
    const WORD kMfc42CWndOnDestroyOrdinal = 6453;
    ImportFunctionPatch importPatches[2] = {};
    bool installed =
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndOnDestroyOrdinal, (void *)&FakeCWndOnDestroy, importPatches[0]) &&
        PatchImportByName("USER32.dll", "SendMessageA", (void *)&FakeSendMessageA, importPatches[1]);

    NetSessionConfigDialog *const dialog =
        (NetSessionConfigDialog *)::operator new(sizeof(NetSessionConfigDialog));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(0);
    dialog->m_mapCombo.m_hWnd = (HWND)0x1234;
    g_onDestroyStep = 0;
    g_onDestroyThis = 0;
    g_sendMessageStep = 0;
    g_sendMessageHwnd = 0;
    g_selectedComboIndex = 4;
    g_selectedItemData = 9;
    g_itemDataWParam = 0;

    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else {
        dialog->OnDestroy();
        if (g_onDestroyStep != 1 || g_onDestroyThis != dialog) {
            result = 4;
        } else if (
            g_sendMessageStep != 2
            || g_sendMessageHwnd != dialog->m_mapCombo.m_hWnd
            || g_itemDataWParam != 4
            || dialog->m_selectedMapIndex != 9
        ) {
            result = 5;
        }
    }

    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(importPatches[index]);
    }
    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return result;
}

bool RunMapChangedScenario(
    NetSessionConfigDialog *dialog,
    LRESULT selectedMapIndex,
    const char *expectedText
) {
    g_sendMessageStep = 0;
    g_sendMessageHwnd = 0;
    g_selectedComboIndex = 3;
    g_selectedItemData = selectedMapIndex;
    g_itemDataWParam = 0;
    g_setDlgItemTextCalls = 0;
    g_setDlgItemTextThis = 0;
    g_setDlgItemTextId = 0;
    g_setDlgItemTextValue = 0;

    dialog->OnMapChanged();

    return g_sendMessageStep == 2 &&
           g_sendMessageHwnd == dialog->m_mapCombo.m_hWnd &&
           g_itemDataWParam == 3 &&
           dialog->m_selectedMapIndex == selectedMapIndex &&
           g_setDlgItemTextCalls == 1 &&
           g_setDlgItemTextThis == dialog &&
           g_setDlgItemTextId == 1125 &&
           g_setDlgItemTextValue == expectedText;
}

extern "C" int net_session_config_dialog_on_map_changed_smoke(void) {
    const WORD kMfc42CWndSetDlgItemTextOrdinal = 5953;
    ImportFunctionPatch importPatches[2] = {};
    CodeFunctionPatch functionPatch = {};
    bool installed =
        PatchImportByName("USER32.dll", "SendMessageA", (void *)&FakeSendMessageA, importPatches[0]) &&
        PatchImportByOrdinal("MFC42.DLL", kMfc42CWndSetDlgItemTextOrdinal, (void *)&FakeSetDlgItemTextA, importPatches[1]) &&
        PatchFunctionJump((void *)&zLoc::GetMessageString, (void *)&FakeGetMessageString, functionPatch);

    NetSessionConfigDialog *const dialog =
        (NetSessionConfigDialog *)::operator new(sizeof(NetSessionConfigDialog));
    std::memset(dialog, 0xcc, sizeof(*dialog));

    NetSessionConfigDialog *const returned = dialog->Constructor(0);
    dialog->m_mapCombo.m_hWnd = (HWND)0x5678;
    int result = 0;
    if (!installed) {
        result = 2;
    } else if (returned != dialog) {
        result = 3;
    } else if (!RunMapChangedScenario(dialog, 2, FakeGetMessageString(12352))) {
        result = 4;
    } else if (!RunMapChangedScenario(dialog, 5, FakeGetMessageString(12353))) {
        result = 5;
    }

    RestoreFunctionPatch(functionPatch);
    for (int index = 1; index >= 0; --index) {
        RestoreImportPatch(importPatches[index]);
    }
    dialog->m_sessionName.~CString();
    ::operator delete(dialog);
    return result;
}
