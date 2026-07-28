#include "Battlesport/wol_dialog.h"

#include <stdarg.h>
#include <stdio.h>

namespace {
const UINT kWestwoodOnlineUpgradeProgressDialog_ResourceId = 157;
const unsigned int kProgressStatusTextControlId = 1023;
}

extern "C" HWND g_hWestwoodOnlineUpgradeProgressDialog;

/**
 * @recoil-anchor recoil:anchor:battlesport.wol.westwoodonlineupgradeprogressdialog-constructor
 * @recoil-artifact defines .text recoil:function:0x442220: WestwoodOnlineUpgradeProgressDialog::WestwoodOnlineUpgradeProgressDialog (D:\Proj\GameZRecoil\westwoodonline\WolapiProgressDialog.cpp).
 * Purpose: initialize the standalone WOL download progress dialog with its
 * MFC dialog resource and optional parent window. The implicit virtual
 * destructor and scalar deleting-destructor are compiler-generated lifecycle
 * contributions, not authored wrapper methods.
 */
WestwoodOnlineUpgradeProgressDialog::WestwoodOnlineUpgradeProgressDialog(
    CWnd *parentWnd
) :
    CDialog(
        kWestwoodOnlineUpgradeProgressDialog_ResourceId,
        parentWnd
    )
{
}

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does
 * not reimplement CDialog behavior.
 */
class WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

const AFX_MSGMAP *__stdcall
WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; used by the
 * progress-dialog MFC message-map data.
 * Purpose: returns the provider CDialog base map.
 */
const AFX_MSGMAP *__stdcall
WestwoodOnlineUpgradeProgressDialog::GetBaseMessageMapForMfc() {
    return WestwoodOnlineUpgradeProgressCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const WestwoodOnlineUpgradeProgressDialog::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP WestwoodOnlineUpgradeProgressDialog::messageMap = {
    &WestwoodOnlineUpgradeProgressDialog::GetBaseMessageMapForMfc,
    &WestwoodOnlineUpgradeProgressDialog::messageEntries[0],
};

/**
 * @recoil-anchor recoil:anchor:battlesport.wol.westwoodonlineupgradeprogressdialog-getmessagemap
 * @recoil-artifact defines .text recoil:function:0x442260: WestwoodOnlineUpgradeProgressDialog::GetMessageMap (D:\Proj\GameZRecoil\westwoodonline\WolapiProgressDialog.cpp).
 * Purpose: returns the sentinel-only MFC message-map record for the raw dialog
 * proc.
 */
const AFX_MSGMAP * WestwoodOnlineUpgradeProgressDialog::GetMessageMap() const {
    return &WestwoodOnlineUpgradeProgressDialog::messageMap;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.wol.westwoodonlineupgradeprogressdialog-setstatustextfmt
 * @recoil-artifact defines .text recoil:function:0x442270: WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt (D:\Proj\GameZRecoil\westwoodonline\WolapiProgressDialog.cpp).
 * Purpose: formats text into the recovered 0x40-byte global buffer and writes
 * the progress status control.
 */
BOOL WestwoodOnlineUpgradeProgressDialog::SetStatusTextFmt(
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
