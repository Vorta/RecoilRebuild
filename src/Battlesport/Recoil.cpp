#include "Battlesport/Recoil.h"

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
