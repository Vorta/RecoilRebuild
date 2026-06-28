#include "Battlesport/Recoil.h"

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does
 * not reimplement CDialog behavior.
 */
class CAboutDlgCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * MFC provider-boundary accessor for imported CDialog message-map metadata.
 * Purpose: Exposes CDialog::messageMap through the callback shape expected by
 * the derived CAboutDlg map.
 */
const AFX_MSGMAP *__stdcall CAboutDlgCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * MFC provider-boundary accessor for CAboutDlg's base message-map callback.
 * Purpose: Returns the provider-owned CDialog base message map for MFC dispatch
 * chaining.
 */
const AFX_MSGMAP *__stdcall CAboutDlg::GetBaseMessageMapForMfc() {
    return CAboutDlgCDialogMessageMapAccessor::GetMessageMap();
}

AFX_MSGMAP_ENTRY const CAboutDlg::messageEntries[] = {
    {0, 0, 0, 0, 0, 0},
};

const AFX_MSGMAP CAboutDlg::messageMap = {
    &CAboutDlg::GetBaseMessageMapForMfc,
    &CAboutDlg::messageEntries[0],
};

/**
 * MFC provider-boundary message-map accessor for CAboutDlg.
 * Purpose: Returns the authored empty dialog message-map table used by MFC
 * command routing.
 */
const AFX_MSGMAP * CAboutDlg::GetMessageMap() const {
    return &CAboutDlg::messageMap;
}

/**
 * Reimplements 0x401000: CAboutDlg::Constructor (D:\Proj\Battlesport\Recoil.cpp).
 *
 * Purpose: construct the authored About dialog over the MFC CDialog provider
 * base with the recovered dialog resource id and caller-supplied parent.
 */
CAboutDlg::CAboutDlg(
    CWnd *parentWnd
)
    : CDialog(
          0x67,
          parentWnd
      ) {}
