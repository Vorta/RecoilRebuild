#include "recoil/recoil_callconv.h"
#include "recoil/recoil_types.h"

#include "recoil/Mfc42Abi.h"

#include <string.h>

class MfcThreeFloatDialog : public CDialog {
  public:
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];

    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    const AFX_MSGMAP * GetMessageMap() const;

    void OnKillFocusValue0();
    void OnKillFocusValue1();
    void OnKillFocusValue2();
    void OnDeltaposSpinValue0(NMHDR *notify, long *result);
    void OnDeltaposSpinValue1(NMHDR *notify, long *result);
    void OnDeltaposSpinValue2(NMHDR *notify, long *result);
    void OnMove(int x, int y);
    int OnCreate(LPCREATESTRUCT createStruct);

    int unknown060;
    float value0;
    float value1;
    float value2;
};

RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(offsetof(MfcThreeFloatDialog, value0) == 0x64);
RECOIL_STATIC_ASSERT(offsetof(MfcThreeFloatDialog, value1) == 0x68);
RECOIL_STATIC_ASSERT(offsetof(MfcThreeFloatDialog, value2) == 0x6c);
RECOIL_STATIC_ASSERT(offsetof(NM_UPDOWN, iDelta) == 0x10);

struct CodeFunctionPatch
{
    unsigned char *address;
    unsigned char original[5];
};

struct ImportFunctionPatch
{
    ULONG_PTR *slot;
    ULONG_PTR original;
};

class MfcThreeFloatCWndAccess : public CWnd
{
  public:
    using CWnd::Default;
    using CWnd::UpdateData;
};

class MfcThreeFloatCDialogAccess : public CDialog
{
  public:
    using CDialog::OnOK;

    static const AFX_MSGMAP *GetBaseMessageMap()
    {
        return &CDialog::messageMap;
    }
};

int g_threeFloatUpdateDataCount;
int g_threeFloatUpdateDataSaveValue[8];
int g_threeFloatOnOkCount;
long g_threeFloatDefaultReturn;
int g_threeFloatDefaultCount;
float g_threeFloatUpdateReplacement[3];
int g_threeFloatUpdateReplacementIndex;

void ResetThreeFloatDialogLog()
{
    g_threeFloatUpdateDataCount = 0;
    g_threeFloatOnOkCount = 0;
    g_threeFloatDefaultCount = 0;
    g_threeFloatDefaultReturn = 0;
    g_threeFloatUpdateReplacement[0] = 0.0f;
    g_threeFloatUpdateReplacement[1] = 0.0f;
    g_threeFloatUpdateReplacement[2] = 0.0f;
    g_threeFloatUpdateReplacementIndex = -1;
    for (int index = 0; index < 8; ++index) {
        g_threeFloatUpdateDataSaveValue[index] = -1;
    }
}

template <typename Method> void *MethodAddress(Method method)
{
    union
    {
        Method method;
        void *address;
    } value = {method};
    return value.address;
}

void *CWndUpdateDataAddress()
{
    return MethodAddress(&MfcThreeFloatCWndAccess::UpdateData);
}

void *CWndDefaultAddress()
{
    return MethodAddress(&MfcThreeFloatCWndAccess::Default);
}

void *CDialogOnOKAddress()
{
    return MethodAddress(&MfcThreeFloatCDialogAccess::OnOK);
}

bool PatchFunctionJump(void *target, void *replacement, CodeFunctionPatch &patch)
{
    if (target == 0) {
        patch.address = 0;
        return false;
    }

    patch.address = (unsigned char *)target;
    memcpy(patch.original, patch.address, sizeof(patch.original));

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) == 0) {
        patch.address = 0;
        return false;
    }

    patch.address[0] = 0xe9;
    const intptr_t relativeOffset =
        (intptr_t)replacement - (intptr_t)(patch.address + sizeof(patch.original));
    *(int *)(patch.address + 1) = (int)relativeOffset;

    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    return true;
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

void RestoreFunctionPatch(CodeFunctionPatch &patch)
{
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(patch.address, sizeof(patch.original), PAGE_EXECUTE_READWRITE,
                       &oldProtect) != 0) {
        memcpy(patch.address, patch.original, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    }

    patch.address = 0;
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

int __fastcall FakeThreeFloatUpdateData(MfcThreeFloatDialog *dialog, void *,
                                             BOOL saveAndValidate)
{
    if (g_threeFloatUpdateDataCount < 8) {
        g_threeFloatUpdateDataSaveValue[g_threeFloatUpdateDataCount] = saveAndValidate;
    }
    ++g_threeFloatUpdateDataCount;

    if (saveAndValidate != 0 && g_threeFloatUpdateReplacementIndex >= 0) {
        if (g_threeFloatUpdateReplacementIndex == 0) {
            dialog->value0 = g_threeFloatUpdateReplacement[0];
        } else if (g_threeFloatUpdateReplacementIndex == 1) {
            dialog->value1 = g_threeFloatUpdateReplacement[1];
        } else {
            dialog->value2 = g_threeFloatUpdateReplacement[2];
        }
    }

    return 1;
}

void __fastcall FakeThreeFloatOnOK(CDialog *, void *)
{
    ++g_threeFloatOnOkCount;
}

long __fastcall FakeThreeFloatDefault(MfcThreeFloatDialog *, void *)
{
    ++g_threeFloatDefaultCount;
    return g_threeFloatDefaultReturn;
}

extern "C" int mfc_three_float_dialog_handlers_smoke(void)
{
    const WORD kMfc42CDialogOnOKOrdinal = 4853;
    CodeFunctionPatch updateDataPatch = {};
    CodeFunctionPatch defaultPatch = {};
    ImportFunctionPatch onOkPatch = {};
    if (!PatchFunctionJump(CWndUpdateDataAddress(), (void *)&FakeThreeFloatUpdateData,
                           updateDataPatch) ||
        !PatchImportByOrdinal("MFC42.DLL", kMfc42CDialogOnOKOrdinal,
                              (void *)&FakeThreeFloatOnOK, onOkPatch) ||
        !PatchFunctionJump(CWndDefaultAddress(), (void *)&FakeThreeFloatDefault, defaultPatch)) {
        RestoreFunctionPatch(defaultPatch);
        RestoreImportPatch(onOkPatch);
        RestoreFunctionPatch(updateDataPatch);
        return 1;
    }

    MfcThreeFloatDialog dialog = {};
    dialog.value0 = 1.0f;
    dialog.value1 = 2.0f;
    dialog.value2 = 3.0f;

    ResetThreeFloatDialogLog();
    g_threeFloatUpdateReplacementIndex = 0;
    g_threeFloatUpdateReplacement[0] = 1.0f;
    dialog.OnKillFocusValue0();
    const bool killNoChange =
        g_threeFloatUpdateDataCount == 1 && g_threeFloatUpdateDataSaveValue[0] == TRUE &&
        g_threeFloatOnOkCount == 0 && dialog.value0 == 1.0f;

    ResetThreeFloatDialogLog();
    g_threeFloatUpdateReplacementIndex = 1;
    g_threeFloatUpdateReplacement[1] = 4.5f;
    dialog.OnKillFocusValue1();
    const bool killChanged =
        g_threeFloatUpdateDataCount == 1 && g_threeFloatUpdateDataSaveValue[0] == TRUE &&
        g_threeFloatOnOkCount == 1 && dialog.value1 == 4.5f;

    NM_UPDOWN upDown = {};
    long result = -1;
    upDown.iDelta = 1;
    ResetThreeFloatDialogLog();
    dialog.OnDeltaposSpinValue0(&upDown.hdr, &result);
    const bool spinSubtract =
        dialog.value0 == 0.75f && result == 0 && g_threeFloatUpdateDataCount == 1 &&
        g_threeFloatUpdateDataSaveValue[0] == FALSE && g_threeFloatOnOkCount == 1;

    result = -1;
    upDown.iDelta = 2;
    ResetThreeFloatDialogLog();
    dialog.OnDeltaposSpinValue1(&upDown.hdr, &result);
    const bool spinValue1Subtract =
        dialog.value1 == 4.25f && result == 0 && g_threeFloatUpdateDataCount == 1 &&
        g_threeFloatUpdateDataSaveValue[0] == FALSE && g_threeFloatOnOkCount == 1;

    result = -1;
    upDown.iDelta = 0;
    ResetThreeFloatDialogLog();
    dialog.OnDeltaposSpinValue2(&upDown.hdr, &result);
    const bool spinAdd =
        dialog.value2 == 3.25f && result == 0 && g_threeFloatUpdateDataCount == 1 &&
        g_threeFloatUpdateDataSaveValue[0] == FALSE && g_threeFloatOnOkCount == 1;

    ResetThreeFloatDialogLog();
    dialog.OnMove(0x12, 0x34);
    const bool moveDefault = g_threeFloatDefaultCount == 1;

    ResetThreeFloatDialogLog();
    g_threeFloatDefaultReturn = -1;
    CREATESTRUCTA createStruct = {};
    const bool createDefaultMinusOne = dialog.OnCreate(&createStruct) == -1 &&
                                       g_threeFloatDefaultCount == 1;

    ResetThreeFloatDialogLog();
    g_threeFloatDefaultReturn = 7;
    const bool createDefaultOther = dialog.OnCreate(&createStruct) == 0 &&
                                    g_threeFloatDefaultCount == 1;

    const AFX_MSGMAP *const messageMap = dialog.GetMessageMap();
    const AFX_MSGMAP_ENTRY *const entries = messageMap->lpEntries;
    const bool mapHeader =
        messageMap == &MfcThreeFloatDialog::messageMap &&
        messageMap->pfnGetBaseMap == &MfcThreeFloatDialog::GetBaseMessageMapForMfc &&
        MfcThreeFloatDialog::GetBaseMessageMapForMfc() ==
            MfcThreeFloatCDialogAccess::GetBaseMessageMap() &&
        entries == MfcThreeFloatDialog::messageEntries;
    const bool killFocusEntries =
        entries[0].nMessage == WM_COMMAND && entries[0].nCode == EN_KILLFOCUS &&
        entries[0].nID == 0x3f1 && entries[0].nLastID == 0x3f1 &&
        entries[0].nSig == AfxSig_vv &&
        entries[0].pfn == (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue0 &&
        entries[1].nMessage == WM_COMMAND && entries[1].nCode == EN_KILLFOCUS &&
        entries[1].nID == 0x3f2 && entries[1].nLastID == 0x3f2 &&
        entries[1].nSig == AfxSig_vv &&
        entries[1].pfn == (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue1 &&
        entries[2].nMessage == WM_COMMAND && entries[2].nCode == EN_KILLFOCUS &&
        entries[2].nID == 0x3f3 && entries[2].nLastID == 0x3f3 &&
        entries[2].nSig == AfxSig_vv &&
        entries[2].pfn == (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue2;
    const UINT spinCode = (UINT)(WORD)(int)UDN_DELTAPOS;
    const bool spinEntries =
        entries[3].nMessage == WM_NOTIFY && entries[3].nCode == spinCode &&
        entries[3].nID == 0x42d && entries[3].nLastID == 0x42d &&
        entries[3].nSig == AfxSig_vNMHDRpl &&
        entries[3].pfn ==
            (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
                &MfcThreeFloatDialog::OnDeltaposSpinValue0 &&
        entries[4].nMessage == WM_NOTIFY && entries[4].nCode == spinCode &&
        entries[4].nID == 0x42e && entries[4].nLastID == 0x42e &&
        entries[4].nSig == AfxSig_vNMHDRpl &&
        entries[4].pfn ==
            (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
                &MfcThreeFloatDialog::OnDeltaposSpinValue1 &&
        entries[5].nMessage == WM_NOTIFY && entries[5].nCode == spinCode &&
        entries[5].nID == 0x42f && entries[5].nLastID == 0x42f &&
        entries[5].nSig == AfxSig_vNMHDRpl &&
        entries[5].pfn ==
            (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
                &MfcThreeFloatDialog::OnDeltaposSpinValue2;
    const bool windowEntries =
        entries[6].nMessage == WM_MOVE && entries[6].nCode == 0 &&
        entries[6].nID == 0 && entries[6].nLastID == 0 &&
        entries[6].nSig == AfxSig_vvii &&
        entries[6].pfn ==
            (AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(int, int))
                &MfcThreeFloatDialog::OnMove &&
        entries[7].nMessage == WM_CREATE && entries[7].nCode == 0 &&
        entries[7].nID == 0 && entries[7].nLastID == 0 &&
        entries[7].nSig == AfxSig_is &&
        entries[7].pfn ==
            (AFX_PMSG)(AFX_PMSGW)(int (AFX_MSG_CALL CWnd::*)(LPCREATESTRUCT))
                &MfcThreeFloatDialog::OnCreate;
    const bool sentinel =
        entries[8].nMessage == 0 && entries[8].nCode == 0 && entries[8].nID == 0 &&
        entries[8].nLastID == 0 && entries[8].nSig == AfxSig_end &&
        entries[8].pfn == 0;

    int failure = 0;
    if (!killNoChange) {
        failure = 2;
    } else if (!killChanged) {
        failure = 3;
    } else if (!spinSubtract) {
        failure = 4;
    } else if (!spinValue1Subtract) {
        failure = 5;
    } else if (!spinAdd) {
        failure = 6;
    } else if (!moveDefault) {
        failure = 7;
    } else if (!createDefaultMinusOne) {
        failure = 8;
    } else if (!createDefaultOther) {
        failure = 9;
    } else if (!mapHeader) {
        failure = 10;
    } else if (!killFocusEntries) {
        failure = 11;
    } else if (!spinEntries) {
        failure = 12;
    } else if (!windowEntries) {
        failure = 13;
    } else if (!sentinel) {
        failure = 14;
    }

    RestoreFunctionPatch(defaultPatch);
    RestoreImportPatch(onOkPatch);
    RestoreFunctionPatch(updateDataPatch);
    return failure;
}
