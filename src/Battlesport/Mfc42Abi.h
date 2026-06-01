#pragma once

#include "recoil/recoil_types.h"

// Provider boundary for MFC42. These declarations come from the vendored MFC42
// SDK and are not Recoil-authored class reimplementations.
// The retail game links against release MFC42.DLL; keep the provider headers in
// that shape even when the modern smoke target is a debug build.
#if defined(_DEBUG)
#define RECOIL_MFC42_RESTORE_DEBUG
#undef _DEBUG
#endif
#include <afx.h>
#undef _AFX_ENABLE_INLINES
#include <afxwin.h>

// VC5SP3's common-control headers predate several declarations consumed by
// the vendored MFC42 afxcmn.h. Supply only the missing SDK shapes for local
// object-byte verification; modern builds already get these from Windows SDKs.
#if defined(_MSC_VER) && _MSC_VER <= 1200
typedef struct tagLVBKIMAGEA {
    ULONG ulFlags;
    HBITMAP hbm;
    LPSTR pszImage;
    UINT cchImageMax;
    int xOffsetPercent;
    int yOffsetPercent;
} LVBKIMAGEA, *LPLVBKIMAGEA;
typedef LVBKIMAGEA LVBKIMAGE;
typedef LPLVBKIMAGEA LPLVBKIMAGE;

#ifndef TBIF_IMAGE
typedef struct tagTBBUTTONINFOA {
    UINT cbSize;
    DWORD dwMask;
    int idCommand;
    int iImage;
    BYTE fsState;
    BYTE fsStyle;
    WORD cx;
    DWORD lParam;
    LPSTR pszText;
    int cchText;
} TBBUTTONINFOA, *LPTBBUTTONINFOA;
typedef TBBUTTONINFOA TBBUTTONINFO;
typedef LPTBBUTTONINFOA LPTBBUTTONINFO;
#endif

#ifndef TBIMHT_AFTER
typedef struct tagTBINSERTMARK {
    int iButton;
    DWORD dwFlags;
} TBINSERTMARK, *LPTBINSERTMARK;
#endif

#ifndef RBBS_GRIPPERALWAYS
#define RBBS_GRIPPERALWAYS 0x00000080
#endif

typedef struct tagCOLORSCHEME {
    DWORD dwSize;
    COLORREF clrBtnHighlight;
    COLORREF clrBtnShadow;
} COLORSCHEME, *LPCOLORSCHEME;

#ifndef RBHT_NOWHERE
typedef struct tagRBHITTESTINFO {
    POINT pt;
    UINT flags;
    int iBand;
} RBHITTESTINFO, *LPRBHITTESTINFO;
#endif
#endif

#include <afxcmn.h>
#include <afxext.h>
#if defined(RECOIL_MFC42_RESTORE_DEBUG)
#define _DEBUG
#undef RECOIL_MFC42_RESTORE_DEBUG
#endif

#include "recoil/recoil_callconv.h"

#if defined(_M_IX86) || defined(__i386__)
RECOIL_STATIC_ASSERT(sizeof(AFX_PMSG) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(CString) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(CRuntimeClass) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(AFX_MSGMAP_ENTRY) == 0x18);
RECOIL_STATIC_ASSERT(sizeof(AFX_MSGMAP) == 0x08);
#endif

struct RecoilNamedVtable {
    const char *name;
};
