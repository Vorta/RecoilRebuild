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
 * Purpose: Caches the DirectSound listener transform as four zVec3 rows:
 * right, up, forward, position.
 *
 * Reconstruction: Inferred array data model; the original declaration
 * and source spelling are unproved.
 *
 * Evidence: The supplied Camera.c reconstruction passes its float[12]
 * world-transform storage to zSndUpdateListenerState. The DirectSound
 * setter copies the complete 0x30-byte representation into this cache;
 * Update3D is its only reader in the supplied tree inventory.
 *
 * Under the supplied VC5SP3 C++ TU profile, the tested declared-array
 * form reproduces S1's row-3 argument home (cache base + 0x24), D1's
 * immediate row-0 binding (cache base + 0), and S2's unchanged immediate
 * listener-velocity binding. The tested structure-member forms do not
 * reproduce S1's home.
 *
 * These are site- and profile-specific observations, not a universal
 * VC5 binding rule or proof of the original source declaration.
 */
extern "C" zVec3 g_zSnd_ListenerState[4] = { 0 };

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
    case ZSND_AUDIO_API_A3D:
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

    case ZSND_AUDIO_API_DIRECTSOUND:
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
 * @recoil-artifact defines .text recoil:function:0x4a2a30: zSndPlayHandleUpdate3DDispatch.
 * @recoil-match byte
 *
 * Purpose: route play-handle 3D updates to the active sound backend.
 */
extern "C" int __fastcall
zSndPlayHandleUpdate3DDispatch(zSndPlayHandle* playHandle, zVec3* worldPos, zVec3* velocity, int velocityScaleMode)
{
    int result = 0;

    switch (g_zSnd_ActiveBackend) {
    case ZSND_AUDIO_API_A3D:
        result = playHandle->Update3DA3D(worldPos, velocity, velocityScaleMode);
        break;

    case ZSND_AUDIO_API_DIRECTSOUND:
        result = playHandle->Update3D(worldPos, velocity, velocityScaleMode);
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

namespace zMath {
/**
 * Purpose: Inline-function spelling of the reviewed vector-dot island for
 * Update3D's two dot products (D1 [0x4a2cb1,0x4a2cd2), D2 [0x4a2d8a,0x4a2da9)):
 * D1 binds &relativePos through an inline-parameter home and the listener
 * right row &g_zSnd_ListenerState[0] as an immediate (retail 0x4a2cb4 mov edx,
 * 0x56b370); D2 binds both local addresses through inline-parameter homes. The
 * capturing ZMTH_VECTOR_DOT does not reproduce those bindings; original spelling
 * and header ownership are unproved (TU-resident model, as zmth_main.c).
 * Original inline helper evidence: no standalone retail function; observed at
 * retail 0x4a2b40, whose two dot-product sites contain no call.
 */
inline float Vec3Dot(const zVec3* left, const zVec3* right)
{
    float result;
    ZMTH_VECTOR_DOT_BOUND(result, left, right);
    return result;
}

/**
 * @recoil-raw-asm recoil:raw-asm:gamezrecoil.zsound.zsnd-3d.fast-sqrt-estimate
 *
 * Purpose: reproduce the legacy binary32 square-root estimate by reading the
 * input representation, arithmetic-shifting it right by one, adding
 * 0x1fc00000, and writing the resulting representation to the named result.
 * Reconstruction: zsnd_3d.cpp-resident copy of the reviewed Camera.c
 * zMath::FastSqrt helper (same body); original spelling and declaration
 * location unproved.
 * Raw assembly: inline __asm island; the native-C fallback compiled under
 * VC5SP3 does not reproduce the consumer (failed control below).
 * Island contract: named 32-bit input/output; clobbers EAX and arithmetic
 * flags, with final flags from ADD. No x87 instructions; depth 0/0/0.
 * No special-value correction or zero-input special case is performed.
 * Sole authorized consumer/range: 0x4a2b40 [0x4a2c8c,0x4a2c99). The compiler
 * owns argument preparation, homes, frame, register saves and result
 * transport; fresh governed object, relocation and linked-body proof remains
 * required. Failed control: the consumer with this fallback compiled natively
 * differs by 397 bytes over the whole 816-byte function (not an island count).
 * Fallback (non-VC5 builds): byte-copies the representation through a signed
 * int with memcpy (zwep_init.c form); no pointer pun. It assumes binary32
 * float, a 32-bit two's-complement int of the same size and an arithmetic
 * right shift of negative signed values.
 * Original inline helper evidence: no standalone retail function; observed at
 * retail 0x4a2b40 [0x4a2c8c,0x4a2c99).
 */
inline float FastSqrt(float value)
{
    float result;
#if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER == 1100
    __asm {
        mov eax, value
        sar eax, 1
        add eax, 01fc00000h
        mov result, eax
    }
#else
    int bits;
    memcpy(&bits, &value, sizeof bits);
    bits = (bits >> 1) + 0x1fc00000;
    memcpy(&result, &bits, sizeof result);
#endif
    return result;
}
} // namespace zMath

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zsound.zsnd-3d.play-handle-update3d
 * @recoil-artifact defines .text recoil:function:0x4a2b40: zSndPlayHandle::Update3D.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-length-sq
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-dot
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zsound.zsnd-3d.fast-sqrt-estimate
 * @recoil-match byte
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
        if (zSnd::IsMuted() != 0) {
            gain = -10000;
        } else {
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

        if (zSnd::IsMuted() != 0) {
            gain = -10000;
        } else {
            gain = gainScaled;
        }

        zVec3 relativePos;
        zMath::Vec3Subtract(&this->worldPos, &g_zSnd_ListenerState[3], &relativePos);
        float distanceSquared;
        ZMTH_VECTOR_LENGTH_SQ(distanceSquared, &relativePos);

        // The fast estimate below replaces the squared length in place.
        float distance = distanceSquared;
        float inverseDistance;
        if (distanceSquared == 0.0f) {
            pan = 0;
            distance = sample->rangeMin;
            inverseDistance = 1.0f / distance;
        } else {
            distance = zMath::FastSqrt(distance);
            inverseDistance = 1.0f / distance;
            const float panDot = zMath::Vec3Dot(&relativePos, &g_zSnd_ListenerState[0]);
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
            const float dopplerDot = zMath::Vec3Dot(&relativeVelocity, &relativePos);
            const float dopplerPitchScale = 1.0f - dopplerDot * inverseDistance * g_zSndInvSpeedOfSoundMps;

            // Retail scales the GetFrequency result in its own home and passes
            // that DWORD back to SetFrequency.
            DWORD frequency;
            ((LPDIRECTSOUNDBUFFER)backendBuffer)->GetFrequency(&frequency);
            frequency = (DWORD)((float)frequency * dopplerPitchScale);
            ((LPDIRECTSOUNDBUFFER)backendBuffer)->SetFrequency(frequency);
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
