#include "Battlesport/wol_dialog.h"
#include "Battlesport/wol_api.h"
#include "Battlesport/wol_download.h"

#include <ocidl.h>
#include <new>
#include <string.h>

namespace {

struct ImportFunctionPatch {
    ULONG_PTR *slot;
    ULONG_PTR original;
};

struct FakeDownload;
struct FakeConnectionPointContainer;
struct FakeConnectionPoint;

FakeDownload *g_fakeDownloadObject;
FakeConnectionPointContainer *g_fakeConnectionPointContainer;
FakeConnectionPoint *g_fakeConnectionPoint;
int g_coCreateCalls;
bool g_coCreateArgsOk;
int g_findConnectionPointCalls;
bool g_connectionPointIidOk;
int g_adviseCalls;
IUnknown *g_adviseSink;
DWORD g_adviseCookie;
int g_unadviseCalls;
DWORD g_unadviseCookie;
int g_downloadBeginCalls;
IUnknown *g_downloadBeginSelf;
char g_downloadBeginDescriptor0[80];
char g_downloadBeginDescriptor1[80];
char g_downloadBeginDescriptor2[80];
char g_downloadBeginSourcePath[260];
char g_downloadBeginFileName[80];
char g_downloadBeginRegistryKey[80];
int g_downloadAbortCalls;
IUnknown *g_downloadAbortSelf;
int g_downloadPumpCalls;
IUnknown *g_downloadPumpSelf;
int g_downloadReleaseCalls;
int g_setDlgItemTextCalls;
HWND g_setDlgItemTextHwnd[4];
int g_setDlgItemTextControlId[4];
const char *g_setDlgItemTextValue[4];
int g_getCurrentDirectoryCalls;
char g_currentDirectory[260];
int g_setCurrentDirectoryCalls;
const char *g_setCurrentDirectoryPath[4];
BOOL g_setCurrentDirectoryResult[4];
int g_createDirectoryCalls;
const char *g_createDirectoryPath;
LPSECURITY_ATTRIBUTES g_createDirectorySecurity;
int g_setTimerCalls;
HWND g_setTimerHwnd;
UINT_PTR g_setTimerId;
UINT g_setTimerMs;
TIMERPROC g_setTimerProc;
int g_killTimerCalls;
HWND g_killTimerHwnd;
UINT_PTR g_killTimerId;
int g_destroyWindowCalls;
HWND g_destroyWindowHwnd[4];
int g_endDialogCalls;
HWND g_endDialogHwnd;
INT_PTR g_endDialogResult;
int g_modalDialogDtorCalls;
int g_modalDtorSequenceCount;
char g_modalDtorSequence[8];

struct FakeConnectionPoint : IConnectionPoint {
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **out) {
        if (IsEqualGUID(riid, IID_IUnknown) ||
            IsEqualGUID(riid, IID_IConnectionPoint)) {
            *out = this;
            return S_OK;
        }
        *out = 0;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() {
        return 2;
    }

    ULONG STDMETHODCALLTYPE Release() {
        return 1;
    }

    HRESULT STDMETHODCALLTYPE GetConnectionInterface(IID *) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE GetConnectionPointContainer(
        IConnectionPointContainer **
    ) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE Advise(IUnknown *sink, DWORD *cookie) {
        ++g_adviseCalls;
        g_adviseSink = sink;
        if (cookie != 0) {
            *cookie = 0x87654321;
            g_adviseCookie = *cookie;
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Unadvise(DWORD cookie) {
        ++g_unadviseCalls;
        g_unadviseCookie = cookie;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE EnumConnections(IEnumConnections **) {
        return E_NOTIMPL;
    }
};

struct FakeConnectionPointContainer : IConnectionPointContainer {
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **out) {
        if (IsEqualGUID(riid, IID_IUnknown) ||
            IsEqualGUID(riid, IID_IConnectionPointContainer)) {
            *out = this;
            return S_OK;
        }
        *out = 0;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() {
        return 2;
    }

    ULONG STDMETHODCALLTYPE Release() {
        return 1;
    }

    HRESULT STDMETHODCALLTYPE EnumConnectionPoints(IEnumConnectionPoints **) {
        return E_NOTIMPL;
    }

    HRESULT STDMETHODCALLTYPE FindConnectionPoint(
        REFIID riid,
        IConnectionPoint **out
    ) {
        ++g_findConnectionPointCalls;
        g_connectionPointIidOk =
            IsEqualGUID(riid, IID_WestwoodOnlineUpgradeDownloadEventSink) != 0;
        *out = g_fakeConnectionPoint;
        return S_OK;
    }
};

struct FakeDownload : IWestwoodOnlineUpgradeDownload {
    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **out) {
        if (IsEqualGUID(riid, IID_IUnknown) ||
            IsEqualGUID(riid, g_IID_WestwoodOnlineUpgradeDownload)) {
            *out = this;
            return S_OK;
        }
        if (IsEqualGUID(riid, IID_IConnectionPointContainer)) {
            *out = g_fakeConnectionPointContainer;
            return S_OK;
        }
        *out = 0;
        return E_NOINTERFACE;
    }

    ULONG STDMETHODCALLTYPE AddRef() {
        return 2;
    }

    ULONG STDMETHODCALLTYPE Release() {
        ++g_downloadReleaseCalls;
        return 1;
    }

    HRESULT STDMETHODCALLTYPE BeginDownload(
        const char *descriptor0,
        const char *descriptor1,
        const char *descriptor2,
        const char *sourcePath,
        const char *fileName,
        const char *registryKey
    ) {
        ++g_downloadBeginCalls;
        g_downloadBeginSelf = this;
        strcpy(g_downloadBeginDescriptor0, descriptor0);
        strcpy(g_downloadBeginDescriptor1, descriptor1);
        strcpy(g_downloadBeginDescriptor2, descriptor2);
        strcpy(g_downloadBeginSourcePath, sourcePath);
        strcpy(g_downloadBeginFileName, fileName);
        strcpy(g_downloadBeginRegistryKey, registryKey);
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Abort() {
        ++g_downloadAbortCalls;
        g_downloadAbortSelf = this;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE Pump() {
        ++g_downloadPumpCalls;
        g_downloadPumpSelf = this;
        return S_OK;
    }
};

FakeDownload g_fakeDownload;
FakeConnectionPointContainer g_fakeCpc;
FakeConnectionPoint g_fakeCp;

HRESULT WINAPI FakeCoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN outer,
    DWORD clsContext,
    REFIID riid,
    LPVOID *outObject
) {
    ++g_coCreateCalls;
    g_coCreateArgsOk =
        IsEqualGUID(rclsid, g_CLSID_WestwoodOnlineUpgradeDownload) != 0 &&
        outer == 0 &&
        clsContext == CLSCTX_INPROC_SERVER &&
        IsEqualGUID(riid, g_IID_WestwoodOnlineUpgradeDownload) != 0 &&
        outObject != 0;
    if (outObject != 0) {
        *outObject = g_fakeDownloadObject;
    }
    return S_OK;
}

BOOL WINAPI FakeSetDlgItemTextA(HWND hWnd, int controlId, LPCSTR text) {
    if (g_setDlgItemTextCalls < 4) {
        g_setDlgItemTextHwnd[g_setDlgItemTextCalls] = hWnd;
        g_setDlgItemTextControlId[g_setDlgItemTextCalls] = controlId;
        g_setDlgItemTextValue[g_setDlgItemTextCalls] = text;
    }
    ++g_setDlgItemTextCalls;
    return TRUE;
}

DWORD WINAPI FakeGetCurrentDirectoryA(DWORD bufferChars, LPSTR buffer) {
    ++g_getCurrentDirectoryCalls;
    if (bufferChars != 0) {
        lstrcpynA(buffer, g_currentDirectory, bufferChars);
    }
    return lstrlenA(g_currentDirectory);
}

BOOL WINAPI FakeSetCurrentDirectoryA(LPCSTR path) {
    const int index = g_setCurrentDirectoryCalls;
    if (index < 4) {
        g_setCurrentDirectoryPath[index] = path;
    }
    ++g_setCurrentDirectoryCalls;
    return index < 4 ? g_setCurrentDirectoryResult[index] : TRUE;
}

BOOL WINAPI FakeCreateDirectoryA(
    LPCSTR path,
    LPSECURITY_ATTRIBUTES security
) {
    ++g_createDirectoryCalls;
    g_createDirectoryPath = path;
    g_createDirectorySecurity = security;
    return TRUE;
}

UINT_PTR WINAPI FakeSetTimer(
    HWND hWnd,
    UINT_PTR timerId,
    UINT elapsedMs,
    TIMERPROC timerProc
) {
    ++g_setTimerCalls;
    g_setTimerHwnd = hWnd;
    g_setTimerId = timerId;
    g_setTimerMs = elapsedMs;
    g_setTimerProc = timerProc;
    return timerId;
}

BOOL WINAPI FakeKillTimer(HWND hWnd, UINT_PTR timerId) {
    ++g_killTimerCalls;
    g_killTimerHwnd = hWnd;
    g_killTimerId = timerId;
    return TRUE;
}

BOOL WINAPI FakeDestroyWindow(HWND hWnd) {
    if (g_destroyWindowCalls < 4) {
        g_destroyWindowHwnd[g_destroyWindowCalls] = hWnd;
    }
    ++g_destroyWindowCalls;
    return TRUE;
}

BOOL WINAPI FakeEndDialog(HWND hWnd, INT_PTR result) {
    ++g_endDialogCalls;
    g_endDialogHwnd = hWnd;
    g_endDialogResult = result;
    return TRUE;
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

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(imageBase + (
            descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                : descriptor->FirstThunk
        ));
        IMAGE_THUNK_DATA *addressThunk =
            (IMAGE_THUNK_DATA *)(imageBase + descriptor->FirstThunk);
        for (; nameThunk->u1.AddressOfData != 0; ++nameThunk, ++addressThunk) {
            if (IMAGE_SNAP_BY_ORDINAL(nameThunk->u1.Ordinal)) {
                continue;
            }

            IMAGE_IMPORT_BY_NAME *importName =
                (IMAGE_IMPORT_BY_NAME *)(imageBase + nameThunk->u1.AddressOfData);
            if (strcmp((const char *)importName->Name, functionName) != 0) {
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

        IMAGE_THUNK_DATA *nameThunk = (IMAGE_THUNK_DATA *)(imageBase + (
            descriptor->OriginalFirstThunk != 0 ? descriptor->OriginalFirstThunk
                                                : descriptor->FirstThunk
        ));
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

void RestoreImportPatch(ImportFunctionPatch &patch) {
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

void ResetProgressDtorProbe() {
    g_modalDialogDtorCalls = 0;
    g_modalDtorSequenceCount = 0;
    memset(g_modalDtorSequence, 0, sizeof(g_modalDtorSequence));
}

void RecordProgressDtorSequence(char code) {
    if (g_modalDtorSequenceCount < (int)sizeof(g_modalDtorSequence)) {
        g_modalDtorSequence[g_modalDtorSequenceCount] = code;
    }
    ++g_modalDtorSequenceCount;
}

void __fastcall FakeProgressDialogDtor(void *, void *) {
    ++g_modalDialogDtorCalls;
    RecordProgressDtorSequence('D');
}

void ResetDownloadDialogProbe() {
    g_fakeDownloadObject = &g_fakeDownload;
    g_fakeConnectionPointContainer = &g_fakeCpc;
    g_fakeConnectionPoint = &g_fakeCp;
    g_coCreateCalls = 0;
    g_coCreateArgsOk = false;
    g_findConnectionPointCalls = 0;
    g_connectionPointIidOk = false;
    g_adviseCalls = 0;
    g_adviseSink = 0;
    g_adviseCookie = 0;
    g_unadviseCalls = 0;
    g_unadviseCookie = 0;
    g_downloadBeginCalls = 0;
    g_downloadBeginSelf = 0;
    g_downloadBeginDescriptor0[0] = '\0';
    g_downloadBeginDescriptor1[0] = '\0';
    g_downloadBeginDescriptor2[0] = '\0';
    g_downloadBeginSourcePath[0] = '\0';
    g_downloadBeginFileName[0] = '\0';
    g_downloadBeginRegistryKey[0] = '\0';
    g_downloadAbortCalls = 0;
    g_downloadAbortSelf = 0;
    g_downloadPumpCalls = 0;
    g_downloadPumpSelf = 0;
    g_downloadReleaseCalls = 0;
    g_setDlgItemTextCalls = 0;
    memset(g_setDlgItemTextHwnd, 0, sizeof(g_setDlgItemTextHwnd));
    memset(g_setDlgItemTextControlId, 0, sizeof(g_setDlgItemTextControlId));
    memset(g_setDlgItemTextValue, 0, sizeof(g_setDlgItemTextValue));
    g_getCurrentDirectoryCalls = 0;
    strcpy(g_currentDirectory, "C:\\PreviousCwd");
    g_setCurrentDirectoryCalls = 0;
    memset(g_setCurrentDirectoryPath, 0, sizeof(g_setCurrentDirectoryPath));
    g_setCurrentDirectoryResult[0] = FALSE;
    g_setCurrentDirectoryResult[1] = TRUE;
    g_setCurrentDirectoryResult[2] = TRUE;
    g_setCurrentDirectoryResult[3] = TRUE;
    g_createDirectoryCalls = 0;
    g_createDirectoryPath = 0;
    g_createDirectorySecurity = 0;
    g_setTimerCalls = 0;
    g_setTimerHwnd = 0;
    g_setTimerId = 0;
    g_setTimerMs = 0;
    g_setTimerProc = 0;
    g_killTimerCalls = 0;
    g_killTimerHwnd = 0;
    g_killTimerId = 0;
    g_destroyWindowCalls = 0;
    memset(g_destroyWindowHwnd, 0, sizeof(g_destroyWindowHwnd));
    g_endDialogCalls = 0;
    g_endDialogHwnd = 0;
    g_endDialogResult = 0;
}

void DisposeCreatedDownloadSink(WestwoodOnlineUpgradeDownloadEventSink *oldSink) {
    if (g_pWestwoodOnlineUpgradeDownloadEventSink != 0 &&
        g_pWestwoodOnlineUpgradeDownloadEventSink != oldSink) {
        WestwoodOnlineUpgradeDownloadEventSink *const sink =
            g_pWestwoodOnlineUpgradeDownloadEventSink;
        sink->~WestwoodOnlineUpgradeDownloadEventSink();
        ::operator delete(sink);
    }
}

} // namespace

extern "C" int westwood_online_upgrade_progress_dialog_constructor_smoke(void) {
    alignas(WestwoodOnlineUpgradeProgressDialog)
        unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] = {};
    WestwoodOnlineUpgradeProgressDialog *const dialog =
        new (dialogStorage) WestwoodOnlineUpgradeProgressDialog(0);
    const bool constructed = dialog->GetMessageMap() != 0;
    dialog->~WestwoodOnlineUpgradeProgressDialog();
    return constructed ? 0 : 1;
}

extern "C" int westwood_online_upgrade_progress_dialog_get_message_map_smoke(void) {
    WestwoodOnlineUpgradeProgressDialog dialog(0);
    const AFX_MSGMAP *const messageMap =
        dialog.WestwoodOnlineUpgradeProgressDialog::GetMessageMap();
    return messageMap != 0 ? 0 : 1;
}

extern "C" int westwood_online_upgrade_progress_dialog_set_status_text_fmt_smoke(void) {
    ImportFunctionPatch setDlgItemTextImport = {};
    if (!PatchImportByName(
            "USER32.dll",
            "SetDlgItemTextA",
            (void *)&FakeSetDlgItemTextA,
            setDlgItemTextImport
        )) {
        return 1;
    }

    HWND const oldProgressHwnd = g_hWestwoodOnlineUpgradeProgressDialog;
    char oldStatusBuffer[sizeof(g_WestwoodOnlineUpgradeProgressStatusTextBuffer)];
    memcpy(
        oldStatusBuffer,
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        sizeof(oldStatusBuffer)
    );

    g_hWestwoodOnlineUpgradeProgressDialog = (HWND)0x13579bdf;
    g_setDlgItemTextCalls = 0;
    memset(g_setDlgItemTextHwnd, 0, sizeof(g_setDlgItemTextHwnd));
    memset(g_setDlgItemTextControlId, 0, sizeof(g_setDlgItemTextControlId));
    memset(g_setDlgItemTextValue, 0, sizeof(g_setDlgItemTextValue));
    g_WestwoodOnlineUpgradeProgressStatusTextBuffer[0] = '\0';

    BOOL const result = WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(
        "Downloaded %d of %s",
        7,
        "nine"
    );

    int failure = 0;
    if (result != TRUE ||
        g_setDlgItemTextCalls != 1 ||
        g_setDlgItemTextHwnd[0] != g_hWestwoodOnlineUpgradeProgressDialog ||
        g_setDlgItemTextControlId[0] != 1023 ||
        strcmp(g_setDlgItemTextValue[0], "Downloaded 7 of nine") != 0 ||
        strcmp(g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
               "Downloaded 7 of nine") != 0 ||
        g_setDlgItemTextValue[0] !=
            g_WestwoodOnlineUpgradeProgressStatusTextBuffer) {
        failure = 2;
    }

    memcpy(
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        oldStatusBuffer,
        sizeof(oldStatusBuffer)
    );
    g_hWestwoodOnlineUpgradeProgressDialog = oldProgressHwnd;
    RestoreImportPatch(setDlgItemTextImport);
    return failure;
}

extern "C" int westwood_online_upgrade_progress_dialog_destructor_smoke(void) {
    const WORD kMfc42CDialogDtorOrdinal = 641;
    ImportFunctionPatch import = {};
    if (!PatchImportByOrdinal(
            "MFC42.DLL",
            kMfc42CDialogDtorOrdinal,
            (void *)&FakeProgressDialogDtor,
            import
        )) {
        return 1;
    }

    alignas(WestwoodOnlineUpgradeProgressDialog)
        unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] = {};
    WestwoodOnlineUpgradeProgressDialog *const dialog =
        new (dialogStorage) WestwoodOnlineUpgradeProgressDialog(0);

    ResetProgressDtorProbe();
    dialog->~WestwoodOnlineUpgradeProgressDialog();
    const int result =
        g_modalDialogDtorCalls == 1 &&
                g_modalDtorSequenceCount == 1 &&
                strcmp(g_modalDtorSequence, "D") == 0
            ? 0
            : 2;

    RestoreImportPatch(import);
    return result;
}

extern "C" int westwood_online_upgrade_progress_dialog_scalar_dtor_smoke(void) {
    const WORD kMfc42CDialogDtorOrdinal = 641;
    ImportFunctionPatch import = {};
    if (!PatchImportByOrdinal(
            "MFC42.DLL",
            kMfc42CDialogDtorOrdinal,
            (void *)&FakeProgressDialogDtor,
            import
        )) {
        return 1;
    }

    alignas(WestwoodOnlineUpgradeProgressDialog)
        unsigned char dialogStorage[sizeof(WestwoodOnlineUpgradeProgressDialog)] = {};
    WestwoodOnlineUpgradeProgressDialog *const dialog =
        new (dialogStorage) WestwoodOnlineUpgradeProgressDialog(0);

    ResetProgressDtorProbe();
    dialog->~WestwoodOnlineUpgradeProgressDialog();
    int failure = 0;
    if (g_modalDialogDtorCalls != 1 ||
        g_modalDtorSequenceCount != 1 ||
        strcmp(g_modalDtorSequence, "D") != 0) {
        failure = 2;
    }

    ResetProgressDtorProbe();
    WestwoodOnlineUpgradeProgressDialog *const heapDialog =
        new WestwoodOnlineUpgradeProgressDialog(0);
    delete heapDialog;
    if (failure == 0 &&
        (g_modalDialogDtorCalls != 1 ||
            g_modalDtorSequenceCount != 1 ||
            strcmp(g_modalDtorSequence, "D") != 0)) {
        failure = 3;
    }

    RestoreImportPatch(import);
    return failure;
}

extern "C" int westwood_online_upgrade_progress_dialog_dlg_proc_smoke(void) {
    IWestwoodOnlineUpgradeDownload *const oldDownload =
        g_pWestwoodOnlineUpgradeDownload;
    WestwoodOnlineUpgradeDownloadEventSink *const oldSink =
        g_pWestwoodOnlineUpgradeDownloadEventSink;
    WestwoodOnlineUpgradeDownloadReadyEntry *const oldReadyList =
        g_pWestwoodOnlineUpgradeDownloadReadyList;
    const DWORD oldCookie = g_WestwoodOnlineUpgradeDownloadAdviseCookie;
    const LONG oldLiveCount = g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount;
    const int oldDialogResult = g_WestwoodOnlineUpgradeDownloadDialogResult;
    HWND const oldProgressHwnd = g_hWestwoodOnlineUpgradeProgressDialog;
    char oldPrompt[sizeof(g_WestwoodOnlineUpgradeDownloadReadyPromptText)];
    char oldRestoreCwd[sizeof(g_WestwoodOnlineUpgradeDownloadRestoreCwd)];
    ImportFunctionPatch imports[9] = {};

    memcpy(
        oldPrompt,
        g_WestwoodOnlineUpgradeDownloadReadyPromptText,
        sizeof(oldPrompt)
    );
    memcpy(
        oldRestoreCwd,
        g_WestwoodOnlineUpgradeDownloadRestoreCwd,
        sizeof(oldRestoreCwd)
    );

    int failure = 0;
    if (!PatchImportByName(
            "ole32.dll",
            "CoCreateInstance",
            (void *)&FakeCoCreateInstance,
            imports[0]
        ) ||
        !PatchImportByName(
            "USER32.dll",
            "SetDlgItemTextA",
            (void *)&FakeSetDlgItemTextA,
            imports[1]
        ) ||
        !PatchImportByName(
            "KERNEL32.dll",
            "GetCurrentDirectoryA",
            (void *)&FakeGetCurrentDirectoryA,
            imports[2]
        ) ||
        !PatchImportByName(
            "KERNEL32.dll",
            "SetCurrentDirectoryA",
            (void *)&FakeSetCurrentDirectoryA,
            imports[3]
        ) ||
        !PatchImportByName(
            "KERNEL32.dll",
            "CreateDirectoryA",
            (void *)&FakeCreateDirectoryA,
            imports[4]
        ) ||
        !PatchImportByName(
            "USER32.dll",
            "SetTimer",
            (void *)&FakeSetTimer,
            imports[5]
        ) ||
        !PatchImportByName(
            "USER32.dll",
            "KillTimer",
            (void *)&FakeKillTimer,
            imports[6]
        ) ||
        !PatchImportByName(
            "USER32.dll",
            "DestroyWindow",
            (void *)&FakeDestroyWindow,
            imports[7]
        ) ||
        !PatchImportByName(
            "USER32.dll",
            "EndDialog",
            (void *)&FakeEndDialog,
            imports[8]
        )) {
        failure = 90;
    }

    WestwoodOnlineUpgradeDownloadReadyEntry entry = {};
    entry.m_next = (WestwoodOnlineUpgradeDownloadReadyEntry *)0x12345678;
    strcpy(entry.m_descriptor0, "descriptor-zero");
    strcpy(entry.m_sourcePathBase, "C:\\PatchSource");
    strcpy(entry.m_fileName, "recoil11.exe");
    strcpy(entry.m_descriptor1, "descriptor-one");
    strcpy(entry.m_descriptor2, "descriptor-two");
    strcpy(entry.m_downloadDirectory, "C:\\PatchTarget");
    strcpy(g_WestwoodOnlineUpgradeDownloadReadyPromptText, "Download prompt");

    ResetDownloadDialogProbe();
    g_pWestwoodOnlineUpgradeDownload = 0;
    g_pWestwoodOnlineUpgradeDownloadEventSink = 0;
    g_pWestwoodOnlineUpgradeDownloadReadyList = &entry;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0;
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    g_WestwoodOnlineUpgradeDownloadDialogResult = 99;
    g_hWestwoodOnlineUpgradeProgressDialog = 0;

    HWND const dialogHwnd = (HWND)0x24681357;
    if (failure == 0) {
        BOOL result = WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd,
            WM_INITDIALOG,
            0,
            0
        );
        if (result != TRUE ||
            g_coCreateCalls != 1 ||
            !g_coCreateArgsOk ||
            g_pWestwoodOnlineUpgradeDownload != &g_fakeDownload ||
            g_pWestwoodOnlineUpgradeDownloadEventSink == 0 ||
            g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount != oldLiveCount + 1 ||
            g_findConnectionPointCalls != 1 ||
            !g_connectionPointIidOk ||
            g_adviseCalls != 1 ||
            g_adviseCookie != 0x87654321) {
            failure = 1;
        } else if (
            g_setDlgItemTextCalls != 2 ||
            g_setDlgItemTextHwnd[0] != dialogHwnd ||
            g_setDlgItemTextControlId[0] != 1024 ||
            strcmp(g_setDlgItemTextValue[0], "Download prompt") != 0 ||
            g_getCurrentDirectoryCalls != 1 ||
            strcmp(g_WestwoodOnlineUpgradeDownloadRestoreCwd,
                   "C:\\PreviousCwd") != 0) {
            failure = 2;
        } else if (
            g_setCurrentDirectoryCalls != 2 ||
            strcmp(g_setCurrentDirectoryPath[0], "C:\\PatchTarget") != 0 ||
            g_createDirectoryCalls != 1 ||
            strcmp(g_createDirectoryPath, "C:\\PatchTarget") != 0 ||
            g_createDirectorySecurity != 0) {
            failure = 3;
        } else if (
            g_downloadBeginCalls != 1 ||
            g_downloadBeginSelf != &g_fakeDownload ||
            strcmp(g_downloadBeginDescriptor0, "descriptor-zero") != 0 ||
            strcmp(g_downloadBeginDescriptor1, "descriptor-one") != 0 ||
            strcmp(g_downloadBeginDescriptor2, "descriptor-two") != 0 ||
            strcmp(g_downloadBeginSourcePath, "C:\\PatchSource\\recoil11.exe") != 0 ||
            strcmp(g_downloadBeginFileName, "recoil11.exe") != 0 ||
            strcmp(g_downloadBeginRegistryKey,
                   "SOFTWARE\\Westwood\\Recoil") != 0) {
            failure = 4;
        } else if (
            g_hWestwoodOnlineUpgradeProgressDialog != dialogHwnd ||
            g_WestwoodOnlineUpgradeDownloadDialogResult != 0 ||
            g_setTimerCalls != 1 ||
            g_setTimerHwnd != dialogHwnd ||
            g_setTimerId != 1 ||
            g_setTimerMs != 50 ||
            g_setTimerProc != 0) {
            failure = 5;
        }
    }

    if (failure == 0) {
        BOOL result = WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd,
            WM_TIMER,
            1,
            0
        );
        if (result != TRUE ||
            g_downloadPumpCalls != 1 ||
            g_downloadPumpSelf != &g_fakeDownload ||
            g_destroyWindowCalls != 0) {
            failure = 6;
        }
    }

    if (failure == 0) {
        g_WestwoodOnlineUpgradeDownloadDialogResult = 7;
        BOOL result = WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd,
            WM_TIMER,
            1,
            0
        );
        if (result != TRUE ||
            g_destroyWindowCalls != 1 ||
            g_destroyWindowHwnd[0] != dialogHwnd) {
            failure = 7;
        }
    }

    if (failure == 0) {
        g_hWestwoodOnlineUpgradeProgressDialog = (HWND)0x13572468;
        BOOL result = WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd,
            WM_COMMAND,
            IDCANCEL,
            0
        );
        if (result != TRUE ||
            g_downloadAbortCalls != 1 ||
            g_downloadAbortSelf != &g_fakeDownload ||
            g_destroyWindowCalls != 2 ||
            g_destroyWindowHwnd[1] != (HWND)0x13572468) {
            failure = 8;
        }
    }

    if (failure == 0) {
        g_WestwoodOnlineUpgradeDownloadDialogResult = 11;
        g_WestwoodOnlineUpgradeDownloadAdviseCookie = 0x456789ab;
        g_setCurrentDirectoryCalls = 0;
        g_killTimerCalls = 0;
        g_endDialogCalls = 0;
        g_unadviseCalls = 0;
        g_unadviseCookie = 0;
        g_downloadReleaseCalls = 0;
        BOOL result = WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd,
            WM_DESTROY,
            0,
            0
        );
        if (result != TRUE ||
            g_killTimerCalls != 1 ||
            g_killTimerHwnd != dialogHwnd ||
            g_killTimerId != 1 ||
            g_unadviseCalls != 1 ||
            g_unadviseCookie != 0x456789ab ||
            g_downloadReleaseCalls != 1) {
            failure = 9;
        } else if (
            g_setCurrentDirectoryCalls != 1 ||
            strcmp(g_setCurrentDirectoryPath[0], "C:\\PreviousCwd") != 0 ||
            g_endDialogCalls != 1 ||
            g_endDialogHwnd != dialogHwnd ||
            g_endDialogResult != 11) {
            failure = 10;
        }
    }

    if (failure == 0 &&
        WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd,
            WM_SETFONT,
            0,
            0
        ) != TRUE) {
        failure = 11;
    }
    if (failure == 0 &&
        WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd,
            WM_USER,
            0,
            0
        ) != FALSE) {
        failure = 12;
    }
    if (failure == 0 &&
        WestwoodOnlineUpgradeProgressDialog::DlgProc(
            dialogHwnd,
            WM_COMMAND,
            1,
            0
        ) != FALSE) {
        failure = 13;
    }

    DisposeCreatedDownloadSink(oldSink);
    g_pWestwoodOnlineUpgradeDownload = oldDownload;
    g_pWestwoodOnlineUpgradeDownloadEventSink = oldSink;
    g_pWestwoodOnlineUpgradeDownloadReadyList = oldReadyList;
    g_WestwoodOnlineUpgradeDownloadAdviseCookie = oldCookie;
    g_WestwoodOnlineUpgradeApiInitState.eventSinkLiveCount = oldLiveCount;
    g_WestwoodOnlineUpgradeDownloadDialogResult = oldDialogResult;
    g_hWestwoodOnlineUpgradeProgressDialog = oldProgressHwnd;
    memcpy(
        g_WestwoodOnlineUpgradeDownloadReadyPromptText,
        oldPrompt,
        sizeof(oldPrompt)
    );
    memcpy(
        g_WestwoodOnlineUpgradeDownloadRestoreCwd,
        oldRestoreCwd,
        sizeof(oldRestoreCwd)
    );
    for (int index = 8; index >= 0; --index) {
        RestoreImportPatch(imports[index]);
    }
    return failure;
}
