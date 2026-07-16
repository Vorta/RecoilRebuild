#include "Battlesport/about.h"

/**
 * Reimplements 0x401000: CAboutDlg::Constructor.
 *
 * Purpose: construct the authored About dialog over the MFC CDialog provider
 * base with the recovered dialog resource id and caller-supplied parent.
 */
CAboutDlg::CAboutDlg(
    CWnd *parentWnd
)
    : CDialog(
          CAboutDlg::IDD,
          parentWnd
      ) {}

/**
 * Reimplements 0x401030: CAboutDlg::GetMessageMap.
 *
 * Purpose: returns the authored empty About dialog message-map table used by
 * MFC command routing.
 *
 * Reimplements 0x401040: CWnd::BeginModalState.
 * Reimplements 0x401050: CWnd::EndModalState.
 *
 * Purpose: emits the adjacent MFC provider modal-state inline wrappers after
 * the About message-map COMDAT, matching the retail source/header timing.
 */
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()

/**
 * Compiler-emitted 0x401020: the current CObject::Serialize COFF candidate
 * supplies byte evidence for the shared retail ret-4 address group; this is
 * not proof of exact retail alias membership, unique semantic ownership,
 * provider acceptance, or authored About ownership.
 */
#define _AFXWIN_INLINE inline
#include <afxwin2.inl>
#undef _AFXWIN_INLINE
