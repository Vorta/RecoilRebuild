#include "zSound.h"

// Reimplements 0x4a1240: zSndSample::SetPlaybackEventHandler
// (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp)
RECOIL_NOINLINE void RECOIL_FASTCALL
zSndSample::SetPlaybackEventHandler(void(RECOIL_FASTCALL *callback)(int eventCode)) {
    if (createGuard == 0) {
        playbackEventHandler = callback;
    }
}

// Reimplements 0x4a1250: zSndPlayHandle_TryEnableManaged
// (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp)
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPlayHandle_TryEnableManaged(zSndPlayHandle *handle) {
    if (handle == 0 || handle->isActive != 0) {
        return 0;
    }

    handle->isActive = 1;
    return 1;
}

// Reimplements 0x4a1270: zSndPlayHandle_TryDisableManaged
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zSndPlayHandle_TryDisableManaged(zSndPlayHandle *handle) {
    if (handle == 0 || handle->isActive == 0) {
        return 0;
    }

    handle->isActive = 0;
    return 1;
}
