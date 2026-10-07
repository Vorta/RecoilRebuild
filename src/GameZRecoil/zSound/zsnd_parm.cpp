#include "zsnd.h"

#include "GameZRecoil/zSound/zsnd_a3d_provider.h"

#include <string.h>

namespace {

} // namespace

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndplayhandle-setfreqscaled
 * @recoil-artifact defines .text recoil:function:0x4a10e0: zSndPlayHandle::SetFreqScaled
 * @recoil-match byte
 *
 * Purpose: clamp and interpolate a playback-rate scale, then apply it to the
 * active DirectSound or A3D backend handle.
 */
int zSndPlayHandle::SetFreqScaled(float scale)
{
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    zSndSample* const sample = ownerSample;
    if (sample->createGuard != 0) {
        return -1;
    }

    // Original inline clamp observed in caller 0x4a10e0; keep normalized sound
    // pitch and frequency scales in the [0, 1] range before backend dispatch.
    if (scale > 1.0f) {
        scale = 1.0f;
    } else if (scale < 0.0f) {
        scale = 0.0f;
    }

    switch (g_zSnd_ActiveBackend) {
    case 1: {
        zA3dProviderSource* const source = (zA3dProviderSource*)(backendBuffer);
        if (source == 0) {
            return -1;
        }

        source->SetPitch(
            ((sample->playbackParam2 - sample->playbackParam3) * scale + sample->playbackParam3) / sample->sampleRate
        );
        return 1;
    }
    case 0: {
        LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(backendBuffer);
        if (buffer == 0) {
            return -1;
        }

        const int error = buffer->SetFrequency((int)((sample->playbackParam2 - sample->playbackParam3) * scale
            + sample->playbackParam3));
        if (error != 0) {
            return zSnd::ReportDirectSoundError(error, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_parm.cpp", 218);
        }
        break;
    }
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndplayhandle-setenablescale
 * @recoil-artifact defines .text recoil:function:0x4a11d0: zSndPlayHandle::SetEnableScale
 * @recoil-match byte
 *
 * Purpose: apply global volume scaling to the backend handle and refresh its
 * active 3D/backend state.
 */
int __fastcall zSndPlayHandle::SetEnableScale(float scale)
{
    int result = 0;
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return result;
    }

    switch (g_zSnd_ActiveBackend) {
    case 0: {
        const float volume = *(float*)(g_zSnd_GlobalVolumeScalePtr);
        gainScaled = zSnd::GainScaleToDirectSoundAttenuation(volume * scale);
        result = Update3DDispatch(0, 0, 0);
        break;
    }
    case 1: {
        // A3D keeps the raw float gain bits in the int-backed gain field.
        const float volume = *(float*)(g_zSnd_GlobalVolumeScalePtr);
        *(float*)&gainScaled = volume * scale;
        result = Update3DDispatch(0, 0, 0);
        break;
    }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndsample-setplaybackeventhandler
 * @recoil-artifact defines .text recoil:function:0x4a1240: zSndSample::SetPlaybackEventHandler
 * @recoil-match byte
 *
 * Purpose: install the playback event callback while the sample is not under
 * the creation guard.
 */
void __fastcall zSndSample::SetPlaybackEventHandler(void(__fastcall* callback)(int eventCode))
{
    if (createGuard == 0) {
        playbackEventHandler = callback;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndplayhandle-tryenablemanaged
 * @recoil-artifact defines .text recoil:function:0x4a1250: zSndPlayHandleTryEnableManaged
 * @recoil-match byte
 *
 * Purpose: mark a managed play handle active only when it exists and is not
 * already active.
 */
extern "C" int __fastcall zSndPlayHandleTryEnableManaged(zSndPlayHandle* handle)
{
    if (handle == 0 || handle->isActive != 0) {
        return 0;
    }

    handle->isActive = 1;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-parm.zsndplayhandle-trydisablemanaged
 * @recoil-artifact defines .text recoil:function:0x4a1270: zSndPlayHandleTryDisableManaged.
 * @recoil-match byte
 *
 * Purpose: clear a managed play handle's active flag only when it exists and
 * is currently active.
 */
extern "C" int __fastcall zSndPlayHandleTryDisableManaged(zSndPlayHandle* handle)
{
    if (handle == 0 || handle->isActive == 0) {
        return 0;
    }

    handle->isActive = 0;
    return 1;
}

namespace zSnd {
/**
 * @recoil-anchor recoil:anchor:zsound.zsnd-parm.z-snd-set-active-backend-pre-init
 * @recoil-artifact defines .text recoil:function:0x4a1290: zSnd::SetActiveBackendPreInit.
 * @recoil-match byte
 *
 * Purpose: Select the sound backend before the runtime is preinitialized.
 */
int __fastcall SetActiveBackendPreInit(int backend)
{
    if (g_zSnd_PreInitialized != 0) {
        return 0;
    }

    g_zSnd_ActiveBackend = backend;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:zsound.zsnd-parm.z-snd-get-active-backend
 * @recoil-artifact defines .text recoil:function:0x4a12b0: zSnd::GetActiveBackend.
 * @recoil-match byte
 *
 * Purpose: Return the currently selected sound backend id.
 */
int __cdecl GetActiveBackend()
{
    return g_zSnd_ActiveBackend;
}
} // namespace zSnd
