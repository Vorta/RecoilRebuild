#include "recoil/Mfc42Abi.h"

#if defined(_M_IX86)
#pragma comment(linker, "/include:__imp_??0CWinApp@@QAE@PBD@Z")
#pragma comment(linker, "/include:__imp_??1CWinApp@@UAE@XZ")
#endif

extern "C" int recoil_mfc42_provider_smoke(void) {
    return 0;
}

extern "C" int mfc_cstring_default_ctor_provider_smoke(void) {
    CString value;
    const char *text = value;

    if (text == 0 || text[0] != '\0') {
        return 1;
    }

    return 0;
}

#if !defined(_MSC_VER) || _MSC_VER >= 1300
BOOL CWinThread::SetThreadPriority(int priority) {
    return ::SetThreadPriority(m_hThread, priority);
}

HGDIOBJ CGdiObject::GetSafeHandle() const {
    return this == 0 ? 0 : m_hObject;
}

HDC CDC::GetSafeHdc() const {
    return this == 0 ? 0 : m_hDC;
}
#endif
