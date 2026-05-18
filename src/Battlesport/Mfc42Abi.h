#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include <windows.h>

#include "recoil/recoil_callconv.h"

class CObject;

class CString {
  public:
    CString();
    CString(const char *text);
    ~CString();
    const CString &operator=(const char *text);
    void Empty();
    void Format(const char *format, ...);
    operator const char *() const {
        return m_pchData;
    }

    char *m_pchData;
};
RECOIL_STATIC_ASSERT(sizeof(CString) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(CString, m_pchData) == 0x00);

struct CRuntimeClass {
    LPCSTR m_lpszClassName;
    int m_nObjectSize;
    UINT m_wSchema;
    CObject *(RECOIL_STDCALL *m_pfnCreateObject)();
    CRuntimeClass *(RECOIL_STDCALL *m_pfnGetBaseClass)();
    CRuntimeClass *m_pNextClass;
};
RECOIL_STATIC_ASSERT(sizeof(CRuntimeClass) == 0x18);

struct AFX_MSGMAP_ENTRY {
    UINT nMessage;
    UINT nCode;
    UINT nID;
    UINT nLastID;
    UINT nSig;
    void *pfn;
};
RECOIL_STATIC_ASSERT(sizeof(AFX_MSGMAP_ENTRY) == 0x18);

struct AFX_MSGMAP {
    const AFX_MSGMAP *(RECOIL_STDCALL *pfnGetBaseMap)();
    const AFX_MSGMAP_ENTRY *lpEntries;
};
RECOIL_STATIC_ASSERT(sizeof(AFX_MSGMAP) == 0x08);

struct RecoilNamedVtable {
    const char *name;
};
