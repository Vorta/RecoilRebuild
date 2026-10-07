#include "GameZRecoil/zSound/zsnd.h"

#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zSound/zsnd_a3d_provider.h"

#include <string.h>

extern "C" void* g_zSnd_BackendListenerHandle;

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
extern "C" zSndListenerState g_zSnd_ListenerState = { 0 };

/**
 * Purpose: Stores the cached DirectSound listener velocity used by Doppler
 * frequency scaling in software 3D playback.
 */
extern "C" zVec3 g_zSnd_ListenerVelocity = { 0 };

namespace {
} // namespace

/**
 * @recoil-anchor recoil:anchor:zsound.zsnd-3d.z-snd-update-listener-state
 * @recoil-artifact defines .text recoil:function:0x4a2950: zSndUpdateListenerState.
 * @recoil-match byte
 *
 * Purpose: update cached listener state or forward it to the A3D listener.
 */
extern "C" int __fastcall zSndUpdateListenerState(zSndListenerState* listenerState, zVec3* listenerVelocity)
{
    switch (g_zSnd_ActiveBackend) {
    case 1:
        if (g_zSnd_BackendListenerHandle == 0) {
            return -1;
        }

        if (listenerState != 0) {
            ((zA3dProviderListener*)(g_zSnd_BackendListenerHandle))
                ->SetPosition3f(listenerState->position.x, listenerState->position.y, listenerState->position.z);
            ((zA3dProviderListener*)(g_zSnd_BackendListenerHandle))
                ->SetOrientation6f(
                    -listenerState->forward.x,
                    -listenerState->forward.y,
                    -listenerState->forward.z,
                    listenerState->up.x,
                    listenerState->up.y,
                    listenerState->up.z
                );
        }

        if (listenerVelocity != 0) {
            ((zA3dProviderListener*)(g_zSnd_BackendListenerHandle))
                ->SetVelocity3f(listenerVelocity->x, listenerVelocity->y, listenerVelocity->z);
        }
        break;

    case 0:
        if (listenerState != 0) {
            memcpy(&g_zSnd_ListenerState, listenerState, sizeof(g_zSnd_ListenerState));
        }

        if (listenerVelocity != 0) {
            g_zSnd_ListenerVelocity = *listenerVelocity;
        }
        break;
    }

    g_zSnd_ListenerStateValid = 1;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:zsound.zsnd-3d.z-snd-play-handle-update3-ddispatch
 * @recoil-artifact defines .text recoil:function:0x4a2a30: zSndPlayHandle::Update3DDispatch.
 * @recoil-match byte
 *
 * Purpose: route play-handle 3D updates to the active sound backend.
 */
int __fastcall zSndPlayHandle::Update3DDispatch(zVec3* worldPos, zVec3* velocity, int velocityScaleMode)
{
    int result = 0;

    switch (g_zSnd_ActiveBackend) {
    case 1:
        result = Update3DA3D(worldPos, velocity, velocityScaleMode);
        break;

    case 0:
        result = Update3D(worldPos, velocity, velocityScaleMode);
        break;
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:zsound.zsnd-3d.z-snd-play-handle-update3-da3-d
 * @recoil-artifact defines .text recoil:function:0x4a2a70: zSndPlayHandle::Update3DA3D.
 * @recoil-match byte
 *
 * Purpose: update A3D provider position, velocity, gain, and Doppler state.
 */
int __fastcall zSndPlayHandle::Update3DA3D(zVec3* worldPos, zVec3* velocity, int velocityScaleMode)
{
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    if (worldPos != 0 && (ownerSample->replayFields.flags & 0x04) == 0) {
        return 1;
    }

    if (backendBuffer == 0) {
        return -1;
    }

    if (worldPos != 0) {
        ((zA3dProviderSource*)backendBuffer)->SetPosition3f(worldPos->x, worldPos->y, worldPos->z);
    }

    if (velocity != 0) {
        ((zA3dProviderSource*)backendBuffer)->SetVelocity3f(velocity->x, velocity->y, velocity->z);
    }

    if (zSnd::IsMuted() != 0) {
        ((zA3dProviderSource*)backendBuffer)->SetGain(0.0f);
    } else {
        ((zA3dProviderSource*)backendBuffer)->SetGain(zSndGainScaleIdentity(*(float*)&gainScaled));
    }

    ((zA3dProviderSource*)backendBuffer)->SetDopplerScale(velocityScaleMode != 0 ? 1.0 : 0.0);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-3d.play-handle-update3d
 * @recoil-artifact defines .text recoil:function:0x4a2b40: zSndPlayHandle::Update3D.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-sq
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 *
 *
 * Purpose: update DirectSound spatial pan, volume, and Doppler state.
 */
int __fastcall zSndPlayHandle::Update3D(zVec3* worldPos, zVec3* velocity, int velocityScaleMode)
{
    if (handleKind != ZSND_PLAYHANDLE_BACKEND) {
        return -1;
    }

    if (worldPos != 0 && (ownerSample->replayFields.flags & 0x04) == 0) {
        return 1;
    }

    if (backendBuffer == 0) {
        return -1;
    }

    int pan;
    int gain;
    if (g_zSnd_ListenerStateValid == 0 || (hasWorldPos == 0 && worldPos == 0)) {
        gain = -10000;
        if (zSnd::IsMuted() == 0) {
            gain = gainScaled;
        }
        pan = 0;
    } else {
        zSndSample* const sample = ownerSample;
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

        gain = -10000;
        if (zSnd::IsMuted() == 0) {
            gain = gainScaled;
        }

        zVec3 relativePos;
        zMath::Vec3Subtract(&this->worldPos, &g_zSnd_ListenerState.position, &relativePos);
        float distanceSquared;
        ZMTH_VECTOR_LENGTH_SQ(distanceSquared, &relativePos);

        float distance;
        float inverseDistance;
        if (distanceSquared == 0.0f) {
            pan = 0;
            distance = sample->rangeMin;
            inverseDistance = 1.0f / distance;
        } else {
            int distanceBits = *(int*)&distanceSquared;
            distanceBits = (distanceBits >> 1) + 0x1fc00000;
            distance = *(float*)&distanceBits;
            inverseDistance = 1.0f / distance;
            float panDot;
            ZMTH_VECTOR_DOT(panDot, &relativePos, &g_zSnd_ListenerState.right);
            pan = (int)(panDot * inverseDistance * 1600.0f);
        }

        if (distance > sample->rangeMax) {
            ((LPDIRECTSOUNDBUFFER)backendBuffer)->SetVolume(-10000);
            return 0;
        }

        if (distance >= sample->rangeMin) {
            gain += (int)(((distance / sample->rangeMin) - 1.0f) * -600.0f);
            if (gain < -10000) {
                gain = -10000;
            }
        }

        if (velocityScaleMode != 0) {
            zVec3 relativeVelocity;
            zMath::Vec3Subtract(&velocityOrDir, &g_zSnd_ListenerVelocity, &relativeVelocity);
            float dopplerDot;
            ZMTH_VECTOR_DOT(dopplerDot, &relativeVelocity, &relativePos);
            const float dopplerPitchScale = 1.0f - dopplerDot * inverseDistance * g_zSndInvSpeedOfSoundMps;

            unsigned int baseFrequency;
            ((LPDIRECTSOUNDBUFFER)backendBuffer)->GetFrequency((LPDWORD)&baseFrequency);
            const __int64 baseFrequencyWide = baseFrequency;
            const int scaledFrequency = (int)((float)(baseFrequencyWide)*dopplerPitchScale);
            ((LPDIRECTSOUNDBUFFER)backendBuffer)->SetFrequency(scaledFrequency);
        }
    }

    int error = ((LPDIRECTSOUNDBUFFER)backendBuffer)->SetPan(pan);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_3d.cpp", 0x160);
    }

    error = ((LPDIRECTSOUNDBUFFER)backendBuffer)->SetVolume(gain);
    if (error != 0) {
        return zSnd::ReportDirectSoundError(error, "D:\\Proj\\GameZRecoil\\zSound\\zsnd_3d.cpp", 0x164);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:zsound.zsnd-3d.z-snd-get-speed-of-sound-mps
 * @recoil-artifact defines .text recoil:function:0x4a2e70: zSndGetSpeedOfSoundMps.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zSound\zsnd_3d.cpp.
 * Purpose: return the current 3D-audio speed-of-sound setting.
 */
extern "C" float __cdecl zSndGetSpeedOfSoundMps()
{
    return g_zSndSpeedOfSoundMps;
}

/**
 * @recoil-anchor recoil:anchor:zsound.zsnd-3d.z-snd-set-speed-of-sound-mps
 * @recoil-artifact defines .text recoil:function:0x4a2e80: zSnd::SetSpeedOfSoundMps.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zSound\zsnd_3d.cpp.
 * Purpose: store the speed of sound and its reciprocal for 3D audio.
 */
void __fastcall zSnd::SetSpeedOfSoundMps(float speedOfSoundMps)
{
    g_zSndSpeedOfSoundMps = speedOfSoundMps;
    g_zSndInvSpeedOfSoundMps = 1.0f / speedOfSoundMps;
}
