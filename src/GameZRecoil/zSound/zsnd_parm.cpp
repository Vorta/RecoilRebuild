#include "zSound.h"

#include <string.h>

namespace {
const char kZSndParmSourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_parm.cpp";

typedef int(__stdcall *BackendSetIntFn)(void *self, int value);
typedef int(__stdcall *BackendSetFloatFn)(void *self, float value);

struct DirectSoundBufferVTable {
    void *slots00_40[17];
    BackendSetIntFn SetFrequency;
};

struct DirectSoundBuffer {
    DirectSoundBufferVTable *vtable;
};

struct A3dSourceVTable {
    void *slots00_a4[42];
    BackendSetFloatFn SetPitchScaled;
};

struct A3dSource {
    A3dSourceVTable *vtable;
};

float Clamp01(float value) {
    if (value > 1.0f) {
        return 1.0f;
    }
    if (value < 0.0f) {
        return 0.0f;
    }
    return value;
}

int FloatToBits(float value) {
    int bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}
} // namespace

// Reimplements 0x4a10e0: zSndPlayHandle::SetFreqScaled
// (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp)
RECOIL_NOINLINE int RECOIL_THISCALL zSndPlayHandle::SetFreqScaled(float scale) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    zSndSample *const sample = ownerSample;
    if (sample->createGuard != 0) {
        return -1;
    }

    const float clampedScale = Clamp01(scale);
    const float playbackRate =
        (sample->playbackParam2 - sample->playbackParam3) * clampedScale +
        sample->playbackParam3;

    if (g_zSnd_ActiveBackend == 0) {
        DirectSoundBuffer *const buffer = (DirectSoundBuffer *)(backendBuffer);
        if (buffer == 0) {
            return -1;
        }

        const int error = buffer->vtable->SetFrequency(buffer, static_cast<int>(playbackRate));
        if (error != 0) {
            return zSnd::ReportDirectSoundError(error, kZSndParmSourceFile, 218);
        }
        return 1;
    }

    if (g_zSnd_ActiveBackend == 1) {
        A3dSource *const source = (A3dSource *)(backendBuffer);
        if (source == 0) {
            return -1;
        }

        source->vtable->SetPitchScaled(source, playbackRate / sample->sampleRate);
    }

    return 1;
}

// Reimplements 0x4a11d0: zSndPlayHandle::SetEnableScale
// (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp)
RECOIL_NOINLINE void RECOIL_THISCALL zSndPlayHandle::SetEnableScale(float scale) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return;
    }

    const float globalScale = *static_cast<float *>(g_zSnd_GlobalVolumeScalePtr);
    const float scaledGain = globalScale * scale;
    if (g_zSnd_ActiveBackend == 0) {
        gainScaled = zSnd::GainScaleToDirectSoundAttenuation(scaledGain);
        Update3DDispatch(0, 0, 0);
    } else if (g_zSnd_ActiveBackend == 1) {
        gainScaled = FloatToBits(scaledGain);
        Update3DDispatch(0, 0, 0);
    }
}

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
