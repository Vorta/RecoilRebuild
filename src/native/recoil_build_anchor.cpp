#include <windows.h>

struct zClass_NodePartial;

extern "C" int g_RecoilState_MainMenuSkipExitDelay = 0;

// Source-faithful helper recovered from address-backed callers in this source file.
extern "C" int recoil_native_build_anchor(
    void
) {
    return 0;
}

#if !defined(_MSC_VER)
// Source-faithful helper recovered from address-backed callers in this source file.
extern "C" void __cdecl __cpuid(
    int cpuInfo[4],
    int
) {
    cpuInfo[0] = 0;
    cpuInfo[1] = 0;
    cpuInfo[2] = 0;
    cpuInfo[3] = 0;
}
#endif
