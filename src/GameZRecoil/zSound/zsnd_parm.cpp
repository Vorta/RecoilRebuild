#include "zSound.h"

#include "GameZRecoil/zSound/zA3dProvider.h"

#include <string.h>

namespace {
const char kZSndParmSourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_parm.cpp";

/**
 * Original inline helper observed in caller 0x4a10e0.
 *
 * Purpose: clamp normalized sound pitch and frequency scales to the accepted
 * [0, 1] range before backend dispatch.
 */
float Clamp01(
    float value
) {
    if (value > 1.0f) {
        return 1.0f;
    }
    if (value < 0.0f) {
        return 0.0f;
    }
    return value;
}

/**
 * Original static helper observed in caller 0x4a11d0.
 *
 * Purpose: copy a float's stored bit pattern into an integer field used by
 * the A3D backend path.
 */
int FloatToBits(
    float value
) {
    int bits = 0;
    memcpy(
        &bits,
        &value,
        sizeof(bits)
    );
    return bits;
}
} // namespace

/**
 * Reimplements 0x4a10e0: zSndPlayHandle::SetFreqScaled
 * (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp).
 *
 * Purpose: clamp and interpolate a playback-rate scale, then apply it to the
 * active DirectSound or A3D backend handle.
 */
int zSndPlayHandle::SetFreqScaled(
    float scale
) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    zSndSample *const sample = ownerSample;
    if (sample->createGuard != 0) {
        return -1;
    }

    const float clampedScale = Clamp01(scale);
    const float playbackRate =
        (sample->playbackParam2 - sample->playbackParam3) * clampedScale + sample->playbackParam3;

    if (g_zSnd_ActiveBackend == 0) {
        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(backendBuffer);
        if (buffer == 0) {
            return -1;
        }

        const int error = buffer->SetFrequency((int)(playbackRate));
        if (error != 0) {
            return zSnd::ReportDirectSoundError(
                error,
                kZSndParmSourceFile,
                218
            );
        }
        return 1;
    }

    if (g_zSnd_ActiveBackend == 1) {
        zA3dProviderSource *const source = (zA3dProviderSource *)(backendBuffer);
        if (source == 0) {
            return -1;
        }

        source->SetPitch(
            playbackRate / sample->sampleRate
        );
    }

    return 1;
}

/**
 * Reimplements 0x4a11d0: zSndPlayHandle::SetEnableScale
 * (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp).
 *
 * Purpose: apply global volume scaling to the backend handle and refresh its
 * active 3D/backend state.
 */
void zSndPlayHandle::SetEnableScale(
    float scale
) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return;
    }

    const float globalScale = *(float *)(g_zSnd_GlobalVolumeScalePtr);
    const float scaledGain = globalScale * scale;
    if (g_zSnd_ActiveBackend == 0) {
        gainScaled = zSnd::GainScaleToDirectSoundAttenuation(scaledGain);
        Update3DDispatch(
            0,
            0,
            0
        );
    } else if (g_zSnd_ActiveBackend == 1) {
        gainScaled = FloatToBits(scaledGain);
        Update3DDispatch(
            0,
            0,
            0
        );
    }
}

/**
 * Reimplements 0x4a1240: zSndSample::SetPlaybackEventHandler
 * (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp).
 *
 * Purpose: install the playback event callback while the sample is not under
 * the creation guard.
 */
void __fastcall zSndSample::SetPlaybackEventHandler(
    void(__fastcall *callback)(int eventCode)
) {
    if (createGuard == 0) {
        playbackEventHandler = callback;
    }
}

/**
 * Reimplements 0x4a1250: zSndPlayHandle_TryEnableManaged
 * (D:\Proj\GameZRecoil\zSound\zsnd_parm.cpp).
 *
 * Purpose: mark a managed play handle active only when it exists and is not
 * already active.
 */
extern "C" int __fastcall zSndPlayHandle_TryEnableManaged(
    zSndPlayHandle *handle
) {
    if (handle == 0 || handle->isActive != 0) {
        return 0;
    }

    handle->isActive = 1;
    return 1;
}

/**
 * Reimplements 0x4a1270: zSndPlayHandle_TryDisableManaged.
 *
 * Purpose: clear a managed play handle's active flag only when it exists and
 * is currently active.
 */
extern "C" int __fastcall zSndPlayHandle_TryDisableManaged(
    zSndPlayHandle *handle
) {
    if (handle == 0 || handle->isActive == 0) {
        return 0;
    }

    handle->isActive = 0;
    return 1;
}
