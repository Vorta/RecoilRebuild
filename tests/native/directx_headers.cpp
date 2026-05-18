#define DIRECTINPUT_VERSION 0x0500

// clang-format off
#include <windows.h>
#include <mmsystem.h>

#include <d3d.h>
#include <d3drm.h>
#include <ddraw.h>
#include <dinput.h>
#include <dplay.h>
#include <dplobby.h>
#include <dsound.h>
// clang-format on

extern "C" int recoil_legacy_directx_header_smoke(void) {
    return static_cast<int>(sizeof(DPSESSIONDESC2) + sizeof(DPCAPS) + sizeof(IDirectDraw *) +
                            sizeof(IDirect3D *) + sizeof(IDirect3DRM *) + sizeof(IDirectSound *) +
                            sizeof(IDirectInputA *));
}
