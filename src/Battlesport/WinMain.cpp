#include <windows.h>

extern int WINAPI AfxWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                             LPSTR lpCmdLine, int nCmdShow);

#if defined(_MSC_VER)
// VC5SP3 needs this local shape to emit the original direct stack-argument pushes.
#pragma optimize("t", off)
#endif
// Reimplements 0x4c81c0: WinMain (WinMain.cpp)
extern "C" int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                              LPSTR lpCmdLine, int nCmdShow) {
    return AfxWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
#if defined(_MSC_VER)
#pragma optimize("", on)
#endif
