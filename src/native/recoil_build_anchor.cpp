#include <windows.h>

struct zClass_NodePartial;

extern "C" int g_RecoilState_MainMenuSkipExitDelay = 0;

extern "C" int recoil_native_build_anchor(void) {
    return 0;
}

#if !defined(_MSC_VER)
extern "C" void __cdecl __cpuid(int cpuInfo[4], int) {
    cpuInfo[0] = 0;
    cpuInfo[1] = 0;
    cpuInfo[2] = 0;
    cpuInfo[3] = 0;
}
#endif
