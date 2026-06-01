#include "Battlesport/WestwoodOnlineUpgradeDialog.h"

#include "GameZRecoil/wwonline/upgrade_download.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <new>
#include <stdarg.h>
#include <stdio.h>

RECOIL_STATIC_ASSERT(sizeof(CWnd) == 0x40);
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);

// Access shim for imported MFC42 CDialog metadata; this does not reimplement
// CDialog behavior.
class WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *RECOIL_STDCALL GetMessageMap();
};

const RecoilNamedVtable kWestwoodOnlineUpgradeProgressDialog_Vtable = {
    "WestwoodOnlineUpgradeProgressDialog vtable"};

namespace {
const UINT kWestwoodOnlineUpgradeProgressDialogResourceId = 157;
const unsigned int kProgressStatusControlId = 1024;
const unsigned int kProgressStatusTextControlId = 1023;
const unsigned int kProgressTimerId = 1;
const unsigned int kProgressTimerMs = 50;
const unsigned int kDownloadPathBufferSize = 256;
const unsigned int kDownloadPromptBufferSize = 128;
const unsigned int kDownloadPromptMessageId = 0x3043;
const unsigned int kDownloadDialogResourceId = 162;
const char kDownloadSourcePathFormat[] = "%s\\%s";
const char kWestwoodOnlineUpgradeRegistryKey[] = "SOFTWARE\\Westwood\\Recoil";
} // namespace

extern "C" HINSTANCE g_RecoilApp_hInstance;
extern "C" HWND g_RecoilApp_hWndMain;
extern "C" char g_WestwoodOnlineUpgradeProgressStatusTextBuffer[1024] = "";

const AFX_MSGMAP *RECOIL_STDCALL
WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

const AFX_MSGMAP *RECOIL_STDCALL WestwoodOnlineUpgradeProgressDialog::GetBaseMessageMapForMfc() {
    return WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const WestwoodOnlineUpgradeProgressDialog::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP WestwoodOnlineUpgradeProgressDialog::messageMap = {
    &WestwoodOnlineUpgradeProgressDialog::GetBaseMessageMapForMfc,
    &WestwoodOnlineUpgradeProgressDialog::messageEntries[0],
};

// Reimplements 0x442260: WestwoodOnlineUpgradeProgressDialog::GetMessageMap
// (D:\Proj\GameZRecoil\westwoodonline\WolapiProgressDialog.cpp)
RECOIL_NOINLINE const AFX_MSGMAP *RECOIL_THISCALL
WestwoodOnlineUpgradeProgressDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeProgressDialog::messageMap;
}

// Reimplements 0x442270: WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt
// (D:\Proj\GameZRecoil\westwoodonline\WolapiProgressDialog.cpp)
RECOIL_NOINLINE BOOL RECOIL_CDECL WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(
    const char *format,
    ...
) {
    va_list args;
    va_start(
        args,
        format
    );
    vsprintf(
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer,
        format,
        args
    );
    va_end(args);

    return ::SetDlgItemTextA(
        g_hWestwoodOnlineUpgradeProgressDialog,
        kProgressStatusTextControlId,
        g_WestwoodOnlineUpgradeProgressStatusTextBuffer
    );
}

// Reimplements 0x442220: WestwoodOnlineUpgradeProgressDialog::Constructor
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeProgressDialog.cpp)
RECOIL_NOINLINE WestwoodOnlineUpgradeProgressDialog *RECOIL_THISCALL
WestwoodOnlineUpgradeProgressDialog::Constructor(
    CWnd *parentWnd
) {
    new ((CDialog *)this) CDialog(
        kWestwoodOnlineUpgradeProgressDialogResourceId,
        parentWnd
    );
    return this;
}

// Reimplements 0x43f440: WestwoodOnlineUpgradeProgressDialog::Destructor
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeDialog.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL WestwoodOnlineUpgradeProgressDialog::Destructor() {
    ((CDialog *)this)->CDialog::~CDialog();
}

// Reimplements 0x442240: WestwoodOnlineUpgradeProgressDialog::ScalarDeletingDestructor
// (D:\Proj\GameZRecoil\westwoodonline\WolapiProgressDialog.cpp)
RECOIL_NOINLINE WestwoodOnlineUpgradeProgressDialog *RECOIL_THISCALL
WestwoodOnlineUpgradeProgressDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    WestwoodOnlineUpgradeProgressDialog *const self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }
    return self;
}

// Reimplements 0x442320: WestwoodOnlineUpgradeProgressDialog::DlgProc
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeProgressDialog.cpp)
RECOIL_NOINLINE BOOL CALLBACK WestwoodOnlineUpgradeProgressDialog::DlgProc(
    HWND hWnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM
) {
    char sourcePath[kDownloadPathBufferSize];
    WestwoodOnlineUpgradeDownloadReadyEntry *entry;
    WestwoodOnlineUpgradeDownloadComObject *download;

    if (uMsg == WM_SETFONT) {
        return TRUE;
    }

    if (uMsg == WM_DESTROY) {
        ::KillTimer(
            hWnd,
            kProgressTimerId
        );
        WestwoodOnlineUpgradeDownload::UnadviseAndRelease();
        SetCurrentDirectoryA(g_WestwoodOnlineUpgradeDownloadRestoreCwd);
        ::EndDialog(
            hWnd,
            g_WestwoodOnlineUpgradeDownloadDialogResult
        );
        return TRUE;
    }

    if (uMsg == WM_INITDIALOG) {
        WestwoodOnlineUpgradeDownload::CreateInstanceAndAdvise();
        ::SetDlgItemTextA(
            hWnd,
            kProgressStatusControlId,
            g_WestwoodOnlineUpgradeDownloadReadyPromptText
        );
        GetCurrentDirectoryA(
            kDownloadPathBufferSize,
            g_WestwoodOnlineUpgradeDownloadRestoreCwd
        );

        entry = g_pWestwoodOnlineUpgradeDownloadReadyList;
        sprintf(
            sourcePath,
            kDownloadSourcePathFormat,
            entry->m_sourcePathBase,
            entry->m_fileName
        );
        if (SetCurrentDirectoryA(g_pWestwoodOnlineUpgradeDownloadReadyList->m_downloadDirectory) ==
            0) {
            CreateDirectoryA(
                g_pWestwoodOnlineUpgradeDownloadReadyList->m_downloadDirectory,
                0
            );
            SetCurrentDirectoryA(g_pWestwoodOnlineUpgradeDownloadReadyList->m_downloadDirectory);
        }

        ::SetDlgItemTextA(
            hWnd,
            kProgressStatusControlId,
            g_WestwoodOnlineUpgradeDownloadReadyPromptText
        );

        entry = g_pWestwoodOnlineUpgradeDownloadReadyList;
        download = (WestwoodOnlineUpgradeDownloadComObject *)g_pWestwoodOnlineUpgradeDownload;
        download->vftable->BeginDownload(
            (IUnknown *)download,
            entry->m_descriptor0,
            entry->m_descriptor1,
            entry->m_descriptor2,
            sourcePath,
            entry->m_fileName,
            kWestwoodOnlineUpgradeRegistryKey
        );
        g_hWestwoodOnlineUpgradeProgressDialog = hWnd;
        g_WestwoodOnlineUpgradeDownloadDialogResult = 0;
        ::SetTimer(
            hWnd,
            kProgressTimerId,
            kProgressTimerMs,
            0
        );
        return TRUE;
    }

    if (uMsg == WM_COMMAND) {
        if (LOWORD(wParam) == IDCANCEL) {
            download = (WestwoodOnlineUpgradeDownloadComObject *)g_pWestwoodOnlineUpgradeDownload;
            download->vftable->Abort((IUnknown *)download);
            ::DestroyWindow(g_hWestwoodOnlineUpgradeProgressDialog);
            return TRUE;
        }
        return FALSE;
    }

    if (uMsg == WM_TIMER) {
        if (g_WestwoodOnlineUpgradeDownloadDialogResult != 0) {
            ::DestroyWindow(hWnd);
            return TRUE;
        }

        download = (WestwoodOnlineUpgradeDownloadComObject *)g_pWestwoodOnlineUpgradeDownload;
        download->vftable->Pump((IUnknown *)download);
        return TRUE;
    }

    return FALSE;
}

// Reimplements 0x442530: WestwoodOnlineUpgradeDialog::ShowDownloadReadyList
// (D:\Proj\Battlesport\WestwoodOnlineUpgradeProgressDialog.cpp)
RECOIL_NOINLINE int RECOIL_FASTCALL WestwoodOnlineUpgradeDialog::ShowDownloadReadyList(
    WestwoodOnlineUpgradeDownloadReadyEntry *readyListHead
) {
    WestwoodOnlineUpgradeDownloadReadyEntry *currentEntry = readyListHead;
    WestwoodOnlineUpgradeDownloadReadyEntry *countCursor = currentEntry;
    int totalEntryCount = 0;
    int entryOrdinal = 0;

    if (currentEntry == 0) {
        return 1;
    }

    do {
        countCursor = countCursor->m_next;
        ++totalEntryCount;
    } while (countCursor != 0);

    do {
        ++entryOrdinal;
        g_pWestwoodOnlineUpgradeDownloadReadyList = currentEntry;
        zLoc::FormatMessage(
            g_WestwoodOnlineUpgradeDownloadReadyPromptText,
            kDownloadPromptBufferSize,
            kDownloadPromptMessageId,
            entryOrdinal,
            totalEntryCount
        );
        if (DialogBoxParamA(
                g_RecoilApp_hInstance,
                (LPCSTR)kDownloadDialogResourceId,
                g_RecoilApp_hWndMain,
                WestwoodOnlineUpgradeProgressDialog::DlgProc,
                0
            ) == -1) {
            return 0;
        }

        currentEntry = currentEntry->m_next;
    } while (currentEntry != 0);

    return 1;
}
