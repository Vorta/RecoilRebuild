#pragma once

#include "recoil/recoil_types.h"

#include <windows.h>

#include "recoil/recoil_callconv.h"

class CWnd;

class CDialog {
  public:
    CDialog(UINT resourceId, CWnd *parentWnd);
    virtual int DoModal();
    virtual ~CDialog();

    unsigned char reserved004[0x5c];
};

class CAboutDlg : public CDialog {
  public:
    CAboutDlg(CWnd *parentWnd = 0);
};

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(sizeof(CAboutDlg) == 0x60);
#endif
