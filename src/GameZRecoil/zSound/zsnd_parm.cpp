#include "zsnd.h"

#include "GameZRecoil/zSound/zsnd_a3d_provider.h"

#include <string.h>

namespace {
const char kZSndParmSourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_parm.cpp";

} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndplayhandle-setfreqscaled
 * @recoil-artifact defines .text recoil:function:0x4a10e0: zSndPlayHandle::SetFreqScaled
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

    // Original inline clamp observed in caller 0x4a10e0; keep normalized sound
    // pitch and frequency scales in the [0, 1] range before backend dispatch.
    float clampedScale = scale;
    if (clampedScale > 1.0f) {
        clampedScale = 1.0f;
    } else if (clampedScale < 0.0f) {
        clampedScale = 0.0f;
    }
    const float playbackRate =
        (sample->playbackParam2 - sample->playbackParam3) * clampedScale + sample->playbackParam3;

    if (g_zSnd_ActiveBackend == 1) {
        zA3dProviderSource *const source = (zA3dProviderSource *)(backendBuffer);
        if (source == 0) {
            return -1;
        }

        source->SetPitch(
            playbackRate / sample->sampleRate
        );
    } else if (g_zSnd_ActiveBackend == 0) {
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

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndplayhandle-setenablescale
 * @recoil-artifact defines .text recoil:function:0x4a11d0: zSndPlayHandle::SetEnableScale
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
    if (g_zSnd_ActiveBackend == 1) {
        // BN stores the x87 product directly into this int-backed gain field
        // for A3D, preserving the raw float bits for later replay.
        memcpy(
            &gainScaled,
            &scaledGain,
            sizeof(gainScaled)
        );
        Update3DDispatch(
            0,
            0,
            0
        );
    } else if (g_zSnd_ActiveBackend == 0) {
        gainScaled = zSnd::GainScaleToDirectSoundAttenuation(scaledGain);
        Update3DDispatch(
            0,
            0,
            0
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndsample-setplaybackeventhandler
 * @recoil-artifact defines .text recoil:function:0x4a1240: zSndSample::SetPlaybackEventHandler
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndplayhandle-tryenablemanaged
 * @recoil-artifact defines .text recoil:function:0x4a1250: zSndPlayHandle_TryEnableManaged
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
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndplayhandle-trydisablemanaged
 * @recoil-artifact defines .text recoil:function:0x4a1270: zSndPlayHandle_TryDisableManaged.
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

namespace zSnd {
/**
 * Purpose: Select the sound backend before the runtime is preinitialized.
 */
int __fastcall SetActiveBackendPreInit(
    int backend
) {
    if (g_zSnd_PreInitialized != 0) {
        return 0;
    }

    g_zSnd_ActiveBackend = backend;
    return 1;
}

/**
 * Purpose: Return the currently selected sound backend id.
 */
int __cdecl GetActiveBackend() {
    return g_zSnd_ActiveBackend;
}
} // namespace zSnd
