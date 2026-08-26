#pragma once

#include "recoil/recoil_types.h"
#include <time.h>

#if defined(_MSC_VER) && _MSC_VER < 1300
#ifndef _TIME_T_DEFINED
typedef long time_t;
#define _TIME_T_DEFINED
#endif
#ifndef _TM_DEFINED
struct tm {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
};
#define _TM_DEFINED
#endif
#endif

// Provider boundary for MFC42. These declarations come from the canonical
// VC5SP3 MFC42 headers and are not Recoil-authored class reimplementations.
// The retail game links against release MFC42.DLL; keep the provider headers in
// that shape even when the modern smoke target is a debug build.
#ifndef _AFXDLL
#define _AFXDLL
#endif
#if defined(_DEBUG)
#define RECOIL_MFC42_RESTORE_DEBUG
#undef _DEBUG
#endif
#include <afx.h>
#if !defined(_MSC_VER) || _MSC_VER >= 1300 || defined(RECOIL_MFC42_DELAY_AFXWIN_INLINES)
#undef _AFX_ENABLE_INLINES
#endif
#include <afxwin.h>
#undef _AFX_ENABLE_INLINES

// VC5SP3's common-control headers predate several declarations consumed by
// MFC42 afxcmn.h. Supply only the missing SDK shapes for local
// object-byte verification; modern builds already get these from Windows SDKs.
#if defined(_MSC_VER) && _MSC_VER <= 1200
#ifndef LVBKIF_SOURCE_NONE
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
#endif

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

#ifndef CCM_SETCOLORSCHEME
typedef struct tagCOLORSCHEME {
    DWORD dwSize;
    COLORREF clrBtnHighlight;
    COLORREF clrBtnShadow;
} COLORSCHEME, *LPCOLORSCHEME;
#endif

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

#if defined(_MSC_VER) && _MSC_VER < 1300
#ifndef RECOIL_MFC42_UINT_PTR_DEFINED
#define RECOIL_MFC42_UINT_PTR_DEFINED
typedef unsigned int UINT_PTR;
#endif
#endif

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
