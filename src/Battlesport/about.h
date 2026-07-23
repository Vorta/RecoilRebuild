#pragma once

#include "recoil/recoil_types.h"

#define RECOIL_MFC42_DELAY_AFXWIN_INLINES
#include "recoil/Mfc42Abi.h"
#undef RECOIL_MFC42_DELAY_AFXWIN_INLINES
#include "Battlesport/Resource.h"
#include "recoil/recoil_callconv.h"

/**
 * Authored Recoil About dialog reconstructed over imported MFC42 CDialog;
 * MFC base behavior is not reimplemented here.
 */
class CAboutDlg : public CDialog {
  public:
    enum { IDD = IDD_RECOIL_DIALOG_103 };

    CAboutDlg(CWnd *parentWnd = 0);

  protected:
    DECLARE_MESSAGE_MAP()
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CAboutDlg) == 0x60);
#endif
