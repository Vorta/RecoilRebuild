// Reimplements 0x40c370: zSys::ProbePlatformAndVideoCaps
RECOIL_NO_GS void RECOIL_FASTCALL zSys::ProbePlatformAndVideoCaps(
    zSysVideoCapsLevel *outVideoCaps, zSysPlatformCapsLevel *outPlatformCaps) {
    OSVERSIONINFOA osVer;
    LPDIRECTDRAW pDDraw = 0;
    LPDIRECTDRAW2 pDDraw2 = 0;
    LPDIRECTDRAWSURFACE pSurface = 0;
    LPDIRECTDRAWSURFACE3 pSurface3 = 0;
    LPDIRECTDRAWSURFACE4 pSurface4 = 0;

    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    RECOIL_STATIC_ASSERT(sizeof(OSVERSIONINFOA) == 0x94);

    if (GetVersionExA(&osVer) == 0) {
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
        return;
    }

    if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_NT4_PLUS;
        if (osVer.dwMajorVersion < 4) {
            *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
            return;
        }

        if (osVer.dwMajorVersion == 4) {
            *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW2;
            HMODULE dinputModule = LoadLibraryA(kDinputDll);
            if (dinputModule == 0) {
                OutputDebugStringA(kCouldNotLoadDinput);
                return;
            }

            FARPROC directInputCreate = GetProcAddress(dinputModule, kDirectInputCreateName);
            FreeLibrary(dinputModule);
            if (directInputCreate == 0) {
                OutputDebugStringA(kCouldNotGetDinputCreate);
                return;
            }

            *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW2_DINPUT;
            return;
        }
    } else {
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_NON_NT;
    }

    zLoadLibraryAFn loadLibrary = LoadLibraryA;
    HMODULE ddrawModule = loadLibrary(kDdrawDll);
    if (ddrawModule == 0) {
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
        FreeLibrary(ddrawModule);
        return;
    }

    zDirectDrawCreateFn directDrawCreate =
        (zDirectDrawCreateFn)GetProcAddress(ddrawModule, kDirectDrawCreateName);
    if (directDrawCreate == 0) {
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
        FreeLibrary(ddrawModule);
        OutputDebugStringA(kCouldNotLoadDDraw);
        return;
    }

    if (directDrawCreate(0, &pDDraw, 0) < 0) {
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
        FreeLibrary(ddrawModule);
        OutputDebugStringA(kCouldNotCreateDDraw);
        return;
    }

    *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW;
    if (IDirectDraw_QueryInterface(pDDraw, IID_IDirectDraw2, (void **)&pDDraw2) < 0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        OutputDebugStringA(kCouldNotQiDDraw2);
        return;
    }

    IDirectDraw2_Release(pDDraw2);
    *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW2;

    register HMODULE dinputModule = loadLibrary(kDinputDll);
    HMODULE dinputHandle = dinputModule;
    if (dinputHandle == 0) {
        OutputDebugStringA(kCouldNotLoadDinput);
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        return;
    }

    FARPROC directInputCreate = GetProcAddress(dinputHandle, kDirectInputCreateName);
    dinputModule = (HMODULE)directInputCreate;
    FreeLibrary(dinputHandle);
    if (dinputModule == 0) {
        FreeLibrary(ddrawModule);
        IDirectDraw_Release(pDDraw);
        OutputDebugStringA(kCouldNotGetDinputCreate);
        return;
    }

    *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW2_DINPUT;

    DDSURFACEDESC desc;
    memset(&desc, 0, sizeof(desc));
    RECOIL_STATIC_ASSERT(sizeof(DDSURFACEDESC) == 0x6c);
    desc.dwSize = sizeof(DDSURFACEDESC);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if (IDirectDraw_SetCooperativeLevel(pDDraw, 0, DDSCL_NORMAL) < 0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        OutputDebugStringA(kCouldNotSetCoopLevel);
        return;
    }

    if (IDirectDraw_CreateSurface(pDDraw, &desc, &pSurface, 0) < 0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        OutputDebugStringA(kCouldNotCreateSurface);
        return;
    }

    if (IDirectDrawSurface_QueryInterface(pSurface, IID_IDirectDrawSurface3,
                                          (void **)&pSurface3) < 0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        return;
    }

    *outVideoCaps = ZSYS_VIDEO_CAPS_SURFACE3;
    if (IDirectDrawSurface_QueryInterface(pSurface, IID_IDirectDrawSurface4,
                                          (void **)&pSurface4) < 0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        return;
    }

    *outVideoCaps = ZSYS_VIDEO_CAPS_SURFACE4;
    IDirectDrawSurface_Release(pSurface);
    IDirectDraw_Release(pDDraw);
    FreeLibrary(ddrawModule);
}
