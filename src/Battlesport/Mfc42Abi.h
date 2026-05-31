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
