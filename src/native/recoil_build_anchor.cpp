#include <windows.h>

struct zClass_NodePartial;

extern "C" int g_RecoilState_MainMenuSkipExitDelay = 0;

extern "C" int recoil_native_build_anchor(
    void
) {
    return 0;
}

#if defined(_MSC_VER) && _MSC_VER < 1300 && defined(_M_IX86)
extern "C" void __cdecl __cpuid(
    int cpuInfo[4],
    int functionId
) {
    int eaxOut;
    int ebxOut;
    int ecxOut;
    int edxOut;
    // Narrow x86 provider shim for VC6, which predates the __cpuid intrinsic.
    __asm {
        mov eax, dword ptr [functionId]
        cpuid
        mov dword ptr [eaxOut], eax
        mov dword ptr [ebxOut], ebx
        mov dword ptr [ecxOut], ecx
        mov dword ptr [edxOut], edx
    }
    cpuInfo[0] = eaxOut;
    cpuInfo[1] = ebxOut;
    cpuInfo[2] = ecxOut;
    cpuInfo[3] = edxOut;
}
#elif !defined(_MSC_VER)
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
