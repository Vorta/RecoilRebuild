// Battlesport compilation unit between RecoilApp.cpp and RecoilFrame.cpp, inferred
// from the retail object boundary [0x42f9f0, 0x4301e0): its .rdata float
// constants [0x4d0bb8, 0x4d0bf0) follow RecoilApp.cpp's vtables and repeat
// values that RecoilApp.cpp (1.0f at 0x4d09cc) and RecoilNet.cpp (0.0f, -3.0f)
// pool separately. Original filename unresolved; RecoilForce.cpp is a
// provisional name (2026-10-02).

#include "Battlesport/CZRecoilFrame.h"
#include "Battlesport/about.h"
#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_exit_panel.h"
#include "Battlesport/net_ui.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/recoil_app.h"
#include "Battlesport/recoil_state_main_menu_transition.h"
#include "Battlesport/recoil_version.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_dialog.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/zDEClient/zdec.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zimage.h"

#include <algorithm>
#include <commdlg.h>
#include <math.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.z-input-ffeffect-set-z-input-ffeffect-set
 * @recoil-artifact defines .text recoil:function:0x42f9f0: zInput_FFEffectSet::zInput_FFEffectSet.
 * @recoil-match byte
 *
 * Purpose: create the seven gameplay force-feedback effects and start the
 * steady steer and pitch force effects when creation succeeds.
 */
zInput_FFEffectSet::zInput_FFEffectSet()
{
    PrimaryFire = zInputDICreateConstantForceEffectScaled(0.25f);
    AltFire = zInputDICreateConstantForceEffectScaled(0.5f);
    CollisionImpact = zInputDICreateConstantForceEffectScaled(0.5f);
    DamageHit = zInputDICreateConstantForceEffectScaled(0.5f);
    AmbientSine = zInputDICreateSineEffectScaled(0.05f);
    SteerForce = zInputDICreateConstantForceEffectWithDirection(0x6978);
    PitchForce = zInputDICreateConstantForceEffectWithDirection(0x4650);

    if (SteerForce != 0) {
        SteerForce->Start(1, 0);
    }
    if (PitchForce != 0) {
        PitchForce->Start(1, 0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilapp.zinput-di-is-force-feedback-enabled
 * @recoil-artifact defines .text recoil:function:0x42fa80: zInputDI::IsForceFeedbackEnabled.
 * @recoil-match byte
 *
 * Purpose: Reports whether joystick input and force feedback are both available.
 */
extern "C" int __fastcall zInputDIIsForceFeedbackEnabled(zInput_FFEffectSet* effectSet)
{
    if (zInp::GetJoystickOption() != 0 && zInputDIHasForceFeedback() != 0) {
        return 1;
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.z-input-ffeffect-set-restart-primary-fire-effect
 * @recoil-artifact defines .text recoil:function:0x42faa0: zInput_FFEffectSet::RestartPrimaryFireEffect.
 * @recoil-match byte
 *
 * Purpose: Stops and restarts the primary-fire force-feedback effect.
 */
void zInput_FFEffectSet::RestartPrimaryFireEffect()
{
    if (PrimaryFire == 0) {
        return;
    }
    PrimaryFire->Stop();
    PrimaryFire->Start(1, 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.ff-effect-set-play-alt-fire-effect
 * @recoil-artifact defines .text recoil:function:0x42fac0: zInput_FFEffectSet::PlayAltFireEffect.
 * @recoil-match byte
 *
 * Purpose: Applies the requested gain and starts the alternate-fire effect.
 */
void zInput_FFEffectSet::PlayAltFireEffect(float gain)
{
    if (AltFire == 0) {
        return;
    }
    RECOIL_STATIC_ASSERT(sizeof(DIEFFECT) == 0x34);
    RECOIL_STATIC_ASSERT(offsetof(DIEFFECT, dwGain) == 0x10);
    DIEFFECT desc;
    memset(&desc, 0, sizeof(desc));
    AltFire->Stop();
    if (gain > 1.0f) {
        gain = 1.0f;
    } else if (gain < 0.25f) {
        gain = 0.25f;
    }
    desc.dwSize = sizeof(desc);
    desc.dwGain = (DWORD)(gain * 10000.0f);
    AltFire->SetParameters(&desc, DIEP_GAIN);
    AltFire->Start(1, 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.z-input-ffeffect-set-play-collision-impact-effect
 * @recoil-artifact defines .text recoil:function:0x42fb50: zInput_FFEffectSet::PlayCollisionImpactEffect.
 * @recoil-match byte
 *
 * Purpose: Plays a collision impulse directed relative to the player.
 */
void zInput_FFEffectSet::PlayCollisionImpactEffect(const zVec3* impactWorldPosXZ, float gain)
{
    if (CollisionImpact == 0) {
        return;
    }
    CollisionImpact->Stop();
    int direction;
    {
        const float kPi = 3.14159274f;
        const float sourceBearing = (float)(atan2(-impactWorldPosXZ->x, -impactWorldPosXZ->z));
        const zInput_PlayerStatePartial* const playerState = g_GameStateOrMapTable->playerState;
        const float playerBearing = (float)(atan2(-playerState->cameraDirNextX, -playerState->cameraDirNextZ));
        float relativeBearing = kPi - (sourceBearing - playerBearing);
        {
            const float kTwoPi = 6.28318548f;
            if (relativeBearing < -kTwoPi) {
                relativeBearing += kTwoPi;
            } else if (relativeBearing > kTwoPi) {
                relativeBearing -= kTwoPi;
            }
        }
        {
            const double kRadToDeg = 57.295779513079999;
            direction = (int)(relativeBearing * kRadToDeg) * 100;
        }
    }
    {
        LONG polarDirection[2] = { direction, 0 };
        DIEFFECT desc;
        memset(&desc, 0, sizeof(desc));
        if (gain > 1.0f) {
            gain = 1.0f;
        } else if (gain < 0.2f) {
            gain = 0.2f;
        }
        desc.dwSize = sizeof(desc);
        desc.dwGain = (DWORD)(gain * 10000.0f);
        desc.dwFlags = 0x20;
        desc.cAxes = 2;
        desc.rglDirection = polarDirection;
        CollisionImpact->SetParameters(&desc, 0x44);
        CollisionImpact->Start(1, 0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.z-input-ffeffect-set-play-damage-hit-effect
 * @recoil-artifact defines .text recoil:function:0x42fc90: zInput_FFEffectSet::PlayDamageHitEffect.
 * @recoil-match byte
 *
 * Purpose: Plays a damage impulse directed from the hit source toward the player.
 */
void zInput_FFEffectSet::PlayDamageHitEffect(const zVec3* damageSourceWorldPosXZ, float gain)
{
    if (DamageHit == 0) {
        return;
    }
    DamageHit->Stop();
    int direction;
    {
        const float kPi = 3.14159274f;
        const float sourceBearing = (float)(atan2(damageSourceWorldPosXZ->x, damageSourceWorldPosXZ->z));
        const zInput_PlayerStatePartial* const playerState = g_GameStateOrMapTable->playerState;
        const float playerBearing = (float)(atan2(-playerState->cameraDirNextX, -playerState->cameraDirNextZ));
        float relativeBearing = kPi - (sourceBearing - playerBearing);
        {
            const float kTwoPi = 6.28318548f;
            if (relativeBearing < -kTwoPi) {
                relativeBearing += kTwoPi;
            } else if (relativeBearing > kTwoPi) {
                relativeBearing -= kTwoPi;
            }
        }
        {
            const double kRadToDeg = 57.295779513079999;
            direction = (int)(relativeBearing * kRadToDeg) * 100;
        }
    }
    {
        LONG polarDirection[2] = { direction, 0 };
        DIEFFECT desc;
        memset(&desc, 0, sizeof(desc));
        if (gain > 1.0f) {
            gain = 1.0f;
        } else if (gain < 0.25f) {
            gain = 0.25f;
        }
        desc.dwSize = sizeof(desc);
        desc.dwGain = (DWORD)(gain * 10000.0f);
        desc.dwFlags = 0x20;
        desc.cAxes = 2;
        desc.rglDirection = polarDirection;
        DamageHit->SetParameters(&desc, 0x44);
        DamageHit->Start(1, 0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.ff-effect-set-update-steer-and-pitch-force-effects
 * @recoil-artifact defines .text recoil:function:0x42fdc0: zInput_FFEffectSet::UpdateSteerAndPitchForceEffects.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-match byte
 *
 * Purpose: Updates continuous steering and pitch forces from current player motion.
 */
void zInput_FFEffectSet::UpdateSteerAndPitchForceEffects()
{
    zInput_PlayerStatePartial* const playerState = g_GameStateOrMapTable->playerState;
    LONG polarDirection[2] = { 0, 0 };
    DIEFFECT desc;
    if (SteerForce != 0) {
        float magnitude = playerState->angVelYaw / playerState->yawVelocityLimit;
        LONG direction;
        if (magnitude < 0.0f) {
            direction = 0x5a;
            magnitude = -magnitude;
        } else {
            direction = 0x6978;
        }
        polarDirection[0] = direction;
        memset(&desc, 0, sizeof(desc));
        if (magnitude > 0.75f) {
            magnitude = 0.75f;
        } else if (magnitude < 0.0f) {
            magnitude = 0.0f;
        }
        desc.dwSize = sizeof(desc);
        desc.dwGain = (DWORD)(magnitude * 10000.0f);
        desc.dwFlags = 0x20;
        desc.cAxes = 2;
        desc.rglDirection = polarDirection;
        SteerForce->SetParameters(&desc, 0x44);
        SteerForce->Start(1, 0);
    }
    if (PitchForce == 0) {
        return;
    }
    // Retail expands the inline zMath::FastExp helper here (EBP frame, MOV/ADD/MOV bridge).
    const float lowpassFactor = zMath::FastExp(g_Player_DeltaTime * -3.0f);
    g_zInput_DiPitchAngleLowpassRad
        = (g_zInput_DiPitchAngleLowpassRad * lowpassFactor) + ((1.0f - lowpassFactor) * playerState->pitchAngleRad);
    float residual = (playerState->pitchAngleRad - g_zInput_DiPitchAngleLowpassRad) * 8.0f;
    LONG direction;
    if (residual < 0.0f) {
        direction = 0;
        residual = -residual;
    } else {
        direction = 0x4650;
    }
    polarDirection[0] = direction;
    memset(&desc, 0, sizeof(desc));
    if (residual > 0.75f) {
        residual = 0.75f;
    } else if (residual < 0.0f) {
        residual = 0.0f;
    }
    desc.dwSize = sizeof(desc);
    desc.dwGain = (DWORD)(residual * 10000.0f);
    desc.dwFlags = 0x20;
    desc.cAxes = 2;
    desc.rglDirection = polarDirection;
    PitchForce->SetParameters(&desc, 0x44);
    PitchForce->Start(1, 0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.z-input-dicreate-constant-force-effect-scaled
 * @recoil-artifact defines .text recoil:function:0x42ffa0: zInputDICreateConstantForceEffectScaled.
 * @recoil-match byte
 *
 * Purpose: Creates a constant-force effect with a clamped gain.
 */
zInput_DiEffect* __stdcall zInputDICreateConstantForceEffectScaled(float gain)
{
    DWORD axes[2] = { 0, 4 };
    LONG direction[2] = { 0, 0 };
    if (gain > 1.0f) {
        gain = 1.0f;
    } else if (gain < 0.0f) {
        gain = 0.0f;
    }

    DICONSTANTFORCE constantForce;
    constantForce.lMagnitude = 10000;

    DIEFFECT effect;
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = 100000;
    effect.dwSamplePeriod = 0;
    effect.dwGain = (DWORD)(gain * 10000.0f);
    effect.dwTriggerButton = (DWORD)(-1);
    effect.dwTriggerRepeatInterval = 0;
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.lpEnvelope = 0;
    effect.cbTypeSpecificParams = sizeof(constantForce);
    effect.lpvTypeSpecificParams = &constantForce;
    return zInputDICreateForceFeedbackEffect(&GUID_ConstantForce, &effect);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.z-input-dicreate-constant-force-effect-with-direction
 * @recoil-artifact defines .text recoil:function:0x430070: zInputDICreateConstantForceEffectWithDirection.
 * @recoil-match byte
 *
 * Purpose: Creates a sustained constant-force effect in the requested direction.
 */
zInput_DiEffect* __fastcall zInputDICreateConstantForceEffectWithDirection(int directionValue)
{
    DWORD axes[2] = { 0, 4 };
    LONG direction[2] = { directionValue, 0 };

    DICONSTANTFORCE constantForce;
    constantForce.lMagnitude = 10000;

    DIEFFECT effect;
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = (DWORD)(-1);
    effect.dwSamplePeriod = 0;
    effect.dwGain = 0;
    effect.dwTriggerButton = (DWORD)(-1);
    effect.dwTriggerRepeatInterval = 0;
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.lpEnvelope = 0;
    effect.cbTypeSpecificParams = sizeof(constantForce);
    effect.lpvTypeSpecificParams = &constantForce;
    return zInputDICreateForceFeedbackEffect(&GUID_ConstantForce, &effect);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoilforce.z-input-dicreate-sine-effect-scaled
 * @recoil-artifact defines .text recoil:function:0x430100: zInputDICreateSineEffectScaled.
 * @recoil-match byte
 *
 * Purpose: Creates a periodic sine-force effect with a clamped gain.
 */
zInput_DiEffect* __stdcall zInputDICreateSineEffectScaled(float gain)
{
    DWORD axes[2] = { 0, 4 };
    LONG direction[2] = { 0, 0 };
    if (gain > 1.0f) {
        gain = 1.0f;
    } else if (gain < 0.0f) {
        gain = 0.0f;
    }

    DIPERIODIC periodic;
    periodic.dwMagnitude = (DWORD)(gain * 10000.0f);
    periodic.lOffset = 0;
    periodic.dwPhase = 0;
    periodic.dwPeriod = 20000;

    DIEFFECT effect;
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = (DWORD)(-1);
    effect.dwSamplePeriod = 0;
    effect.dwGain = 10000;
    effect.dwTriggerButton = (DWORD)(-1);
    effect.dwTriggerRepeatInterval = 0;
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.lpEnvelope = 0;
    effect.cbTypeSpecificParams = sizeof(periodic);
    effect.lpvTypeSpecificParams = &periodic;
    return zInputDICreateForceFeedbackEffect(&GUID_Sine, &effect);
}
