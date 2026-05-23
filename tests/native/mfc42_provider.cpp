#include "Battlesport/Mfc42Abi.h"

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
