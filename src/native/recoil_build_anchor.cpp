#include <windows.h>

struct zClass_NodePartial;

extern "C" int g_RecoilState_MainMenuSkipExitDelay = 0;

extern "C" int recoil_native_build_anchor(void) {
    return 0;
}

extern "C" void __cdecl __cpuid(int cpuInfo[4], int) {
    cpuInfo[0] = 0;
    cpuInfo[1] = 0;
    cpuInfo[2] = 0;
    cpuInfo[3] = 0;
}

extern "C" const GUID IID_IDirectDrawSurface4 = {
    0x0b2b8630,
    0xad35,
    0x11d0,
    {0x8e, 0xa6, 0x00, 0x60, 0x97, 0x97, 0xea, 0x5b}};

int __fastcall zVideo_sw_RenderFrame(zClass_NodePartial *, int) {
    return 0;
}

extern "C" int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {
    return 0;
}
