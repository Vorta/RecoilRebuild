#include "GameZRecoil/zSound/zsnd.h"

#include "GameZRecoil/zSound/zsnd_a3d_provider.h"

#include <string.h>

extern "C" void *g_zSnd_BackendListenerHandle;

/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zSound\zsnd_3d.cpp.
 * Purpose: Stores the configured speed of sound in meters per second for
 * 3D audio listener and Doppler calculations.
 */
extern "C" float g_zSndSpeedOfSoundMps = 345.0f;

/**
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zSound\zsnd_3d.cpp.
 * Purpose: Caches the reciprocal speed-of-sound scale used by 3D audio
 * Doppler pitch updates.
 */
extern "C" float g_zSndInvSpeedOfSoundMps = 1.0f / 345.0f;

/**
 * Purpose: Stores whether the cached DirectSound listener transform can be
 * used by zSound 3D playback update paths.
 */
extern "C" int g_zSnd_ListenerStateValid = 0;

/**
 * Purpose: Stores the cached DirectSound listener position and basis vectors
 * used when software 3D playback gain and pan are recomputed.
 */
extern "C" zSndListenerState g_zSnd_ListenerState = {0};

/**
 * Purpose: Stores the cached DirectSound listener velocity used by Doppler
 * frequency scaling in software 3D playback.
 */
extern "C" zVec3 g_zSnd_ListenerVelocity = {0};

namespace {
const char kZSnd3dSourceFile[] = "D:\\Proj\\GameZRecoil\\zSound\\zsnd_3d.cpp";
} // namespace

/**
 * Purpose: update cached listener state or forward it to the A3D listener.
 */
extern "C" int __fastcall zSnd_UpdateListenerState(
    zSndListenerState *listenerState,
    zVec3 *listenerVelocity
) {
    if (g_zSnd_ActiveBackend == 0) {
        if (listenerState != 0) {
            memcpy(
                &g_zSnd_ListenerState,
                listenerState,
                sizeof(g_zSnd_ListenerState)
            );
        }

        if (listenerVelocity != 0) {
            g_zSnd_ListenerVelocity = *listenerVelocity;
        }

        g_zSnd_ListenerStateValid = 1;
        return 1;
    }

    if (g_zSnd_ActiveBackend == 1) {
        zA3dProviderListener *const listener =
            (zA3dProviderListener *)(g_zSnd_BackendListenerHandle);
        if (listener == 0) {
            return -1;
        }

        if (listenerState != 0) {
            listener->SetPosition3f(
                listenerState->position.x,
                listenerState->position.y,
                listenerState->position.z
            );
            listener->SetOrientation6f(
                -listenerState->forward.x,
                -listenerState->forward.y,
                -listenerState->forward.z,
                listenerState->up.x,
                listenerState->up.y,
                listenerState->up.z
            );
        }

        if (listenerVelocity != 0) {
            listener->SetVelocity3f(
                listenerVelocity->x,
                listenerVelocity->y,
                listenerVelocity->z
            );
        }
    }

    g_zSnd_ListenerStateValid = 1;
    return 1;
}

/**
 * Purpose: route play-handle 3D updates to the active sound backend.
 */
int __fastcall zSndPlayHandle::Update3DDispatch(
    zVec3 *worldPos,
    zVec3 *velocity,
    int velocityScaleMode
) {
    if (g_zSnd_ActiveBackend == 1) {
        return Update3D_A3D(
            worldPos,
            velocity,
            velocityScaleMode
        );
    }

    if (g_zSnd_ActiveBackend == 0) {
        return Update3D(
            worldPos,
            velocity,
            velocityScaleMode
        );
    }

    return 0;
}

/**
 * Purpose: update A3D provider position, velocity, gain, and Doppler state.
 */
int __fastcall zSndPlayHandle::Update3D_A3D(
    zVec3 *worldPos,
    zVec3 *velocity,
    int velocityScaleMode
) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    if (worldPos != 0 && (ownerSample->replayFields.flags & 0x04) == 0) {
        return 1;
    }

    int const sourceGainScaled = gainScaled;
    zA3dProviderSource *const source = (zA3dProviderSource *)(backendBuffer);
    if (source == 0) {
        return -1;
    }

    if (worldPos != 0) {
        source->SetPosition3f(
            worldPos->x,
            worldPos->y,
            worldPos->z
        );
    }

    if (velocity != 0) {
        ((zA3dProviderSource *)backendBuffer)->SetVelocity3f(
            velocity->x,
            velocity->y,
            velocity->z
        );
    }

    if (zSnd::IsMuted() != 0) {
        source->SetGain(0.0f);
    } else {
        source->SetGain(
            zSndSample_PlaySimple(*(float *)&sourceGainScaled)
        );
    }

    source->SetDopplerScale(
        velocityScaleMode != 0 ? 1.0f : 0.0f
    );
    return 1;
}

/**
 * Purpose: update DirectSound spatial pan, volume, and Doppler state.
 */
int __fastcall zSndPlayHandle::Update3D(
    zVec3 *worldPos,
    zVec3 *velocity,
    int velocityScaleMode
) {
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    if (worldPos != 0 && (ownerSample->replayFields.flags & 0x04) == 0) {
        return 1;
    }

    LPDIRECTSOUNDBUFFER const buffer = (LPDIRECTSOUNDBUFFER)(backendBuffer);
    if (buffer == 0) {
        return -1;
    }

    int pan = 0;
    int gain = -10000;
    if (g_zSnd_ListenerStateValid == 0 || (hasWorldPos == 0 && worldPos == 0)) {
        if (zSnd::IsMuted() == 0) {
            gain = gainScaled;
        }
    } else {
        zSndSample *const sample = ownerSample;
        if (sample->createGuard != 0) {
            return -1;
        }

        if (worldPos != 0) {
            hasWorldPos = 1;
            this->worldPos = *worldPos;
        }

        if (velocity != 0) {
            velocityOrDir = *velocity;
        }

        if (zSnd::IsMuted() == 0) {
            gain = gainScaled;
        }

        const zVec3 relativePos = {this->worldPos.x - g_zSnd_ListenerState.position.x,
            this->worldPos.y - g_zSnd_ListenerState.position.y,
            this->worldPos.z - g_zSnd_ListenerState.position.z};
        const float distanceSquared =
            relativePos.x * relativePos.x +
            relativePos.y * relativePos.y +
            relativePos.z * relativePos.z;

        float distance = sample->rangeMin;
        float inverseDistance = 1.0f / sample->rangeMin;
        if (distanceSquared != 0.0f) {
            int distanceBits = *(int *)&distanceSquared;
            distanceBits = (distanceBits >> 1) + 0x1fc00000;
            distance = *(float *)&distanceBits;
            inverseDistance = 1.0f / distance;
            const float panDot =
                relativePos.x * g_zSnd_ListenerState.right.x +
                relativePos.y * g_zSnd_ListenerState.right.y +
                relativePos.z * g_zSnd_ListenerState.right.z;
            pan = (int)(panDot * inverseDistance * 1600.0f);
        }

        if (distance > sample->rangeMax) {
            buffer->SetVolume(-10000);
            return 0;
        }

        if (distance >= sample->rangeMin) {
            gain += (int)(((distance / sample->rangeMin) - 1.0f) * -600.0f);
            if (gain < -10000) {
                gain = -10000;
            }
        }

        if (velocityScaleMode != 0) {
            const zVec3 relativeVelocity = {velocityOrDir.x - g_zSnd_ListenerVelocity.x,
                velocityOrDir.y - g_zSnd_ListenerVelocity.y,
                velocityOrDir.z - g_zSnd_ListenerVelocity.z};
            const float dopplerDot =
                relativeVelocity.x * relativePos.x +
                relativeVelocity.y * relativePos.y +
                relativeVelocity.z * relativePos.z;
            const float dopplerPitchScale =
                1.0f - dopplerDot * inverseDistance * g_zSndInvSpeedOfSoundMps;

            unsigned int baseFrequency = 0;
            buffer->GetFrequency((LPDWORD)&baseFrequency);
            const __int64 baseFrequencyWide = baseFrequency;
            const int scaledFrequency =
                (int)((float)(baseFrequencyWide)*dopplerPitchScale);
            ((LPDIRECTSOUNDBUFFER)backendBuffer)->SetFrequency(scaledFrequency);
        }
    }

    int error = buffer->SetPan(pan);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(
            error,
            kZSnd3dSourceFile,
            0x160
        );
    }

    error = buffer->SetVolume(gain);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(
            error,
            kZSnd3dSourceFile,
            0x164
        );
    }

    return 1;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zSound\zsnd_3d.cpp.
 * Purpose: return the current 3D-audio speed-of-sound setting.
 */
extern "C" float __cdecl zSnd_GetSpeedOfSoundMps() {
    return g_zSndSpeedOfSoundMps;
}

/**
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zSound\zsnd_3d.cpp.
 * Purpose: store the speed of sound and its reciprocal for 3D audio.
 */
void __fastcall zSnd::SetSpeedOfSoundMps(
    float speedOfSoundMps
) {
    g_zSndSpeedOfSoundMps = speedOfSoundMps;
    g_zSndInvSpeedOfSoundMps = 1.0f / speedOfSoundMps;
}
