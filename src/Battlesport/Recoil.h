#pragma once

#include "recoil/recoil_types.h"

#include "Battlesport/Mfc42Abi.h"
#include "recoil/recoil_callconv.h"

/**
 * Authored Recoil About dialog reconstructed over imported MFC42 CDialog;
 * MFC base behavior is not reimplemented here.
 */
class CAboutDlg : public CDialog {
  public:
    CAboutDlg(CWnd *parentWnd = 0);
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CAboutDlg) == 0x60);
#endif
