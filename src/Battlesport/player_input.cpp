// Battlesport compilation unit between player_contact.cpp and player_move.cpp,
// inferred from the retail object boundary [0x425920, 0x426350): its .rdata float
// constants [0x4d0758, 0x4d0770) repeat 0.0f, 1.0f, -1.0f and 10.0f that the
// neighbouring objects pool separately. Its first and last functions
// (0x425920, 0x426150, 0x426330) read no pooled constant and are placed with the
// input code by subject. Original filename unresolved; player_input.cpp is a
// provisional name (2026-10-02).

#include "recoil/Mfc42Abi.h"
#include "player.h"

#include "Battlesport/ai_net.h"
#include "Battlesport/game_net.h"
#include "Battlesport/pickup.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "hud.h"
#include "hud_sensor_tracker.h"
#include "opt_catalog.h"

#include <algorithm>
#include <atlbase.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern char g_HudUiCounterText_PlayerLabel[];

extern "C" {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-g-player-gameplayinputstepscale
 * @recoil-artifact defines .data recoil:data:0x4dc970: g_Player_GameplayInputStepScale.
 * BN types this as an initialized .data float read by local mouse/cursor
 * steering when cursor mode uses mouse deltas.
 * Purpose: Scales mouse delta input into player steering command steps.
 */
float g_Player_GameplayInputStepScale = 0.03f;
} // extern "C"
namespace {
/**
 * Original inline helper; no standalone retail function exists. Observed in address-backed callers 0x4386c0, 0x4289f0,
 * 0x42c0d0, 0x42c2e0, 0x427440, 0x427ec0, 0x43a600, and 0x43a900 as a VC5-era int-bits smoothing idiom. Purpose:
 * reinterpret an IEEE-754 bit pattern as float.
 */
#define PLAYER_FLOAT_FROM_BITS(bits) (*(const float*)&(bits))
/**
 * Original inline helper; no standalone retail function exists.
 * Observed in address-backed callers 0x425a20 Player::TickLocalPlayerControls, 0x428520 Player::UpdateMasterTypeSub,
 * 0x426770 Player::UpdateMasterTypeTrack, 0x427440 Player::UpdateMasterTypeHoverFromModalProbe. Purpose: clamp a
 * caller-owned scalar without emitting an out-of-line helper under the retail translation unit's /Ob0 compile mode.
 */
#define PLAYER_CLAMP_SIGNED(value, limit)                                                                              \
    do {                                                                                                               \
        if ((value) > (limit)) {                                                                                       \
            (value) = (limit);                                                                                         \
        } else if ((value) < -(limit)) {                                                                               \
            (value) = -(limit);                                                                                        \
        }                                                                                                              \
    } while (0)
enum PlayerMasterTypeId {
    kPlayerMasterTypeFly = 1,
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3,
    kPlayerMasterTypeHover = 4,
    kPlayerMasterTypeAmphib = 5
};
} // namespace
namespace Player {
enum { kPlayerMasterTypeSub = 2, kPlayerMaxModalProbePoints = 4, kPlayerEnvProbeBasePointOffset = 15 };

enum PlayerCameraState {
    kPlayerCameraStateToggleRequest = 0,
    kPlayerCameraStateThirdPerson = 1,
    kPlayerCameraStateClearScreen = 2,
    kPlayerCameraStateFirstPerson = 3,
    kPlayerCameraStateTargeting = 4,
    kPlayerCameraStateProjectileAttached = 7,
    kPlayerCameraStateRestorePrevious = 8
};
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-registergameplaycommandcallbacksandcreateffeffects
 * @recoil-artifact defines .text recoil:function:0x425920: Player::RegisterGameplayCommandCallbacksAndCreateFfEffects.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\Battlesport\player.cpp.
 * Purpose: reimplement Player::RegisterGameplayCommandCallbacksAndCreateFfEffects from the recovered
 * Battlesport gameplay source file.
 */
void __cdecl RegisterGameplayCommandCallbacksAndCreateFfEffects()
{
    // zInput's keyboard bridge tail-jumps to these handlers with commandId in ECX.
    zInputCommandCallbackFn hudHotkeyCallback = (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand);
    zInput::BindMapCurrentSetCommandCallback(30, hudHotkeyCallback);
    zInput::BindMapCurrentSetCommandCallback(9, hudHotkeyCallback);
    zInput::BindMapCurrentSetCommandCallback(32, hudHotkeyCallback);
    zInput::BindMapCurrentSetCommandCallback(33, hudHotkeyCallback);

    if (zVid::GetAccelerationOption() == 0) {
        zInput::BindMapCurrentSetCommandCallback(
            34,
            (zInputCommandCallbackFn)(zVideo::HandleSoftwareModeHotkeyCommand)
        );
    }

    zInput::BindMapCurrentSetCommandCallback(35, hudHotkeyCallback);
    zInput::BindMapCurrentSetCommandCallback(42, hudHotkeyCallback);
    zInput::BindMapCurrentSetCommandCallback(43, hudHotkeyCallback);
    zInput::BindMapCurrentSetCommandCallback(44, hudHotkeyCallback);
    zInput::BindMapCurrentSetCommandCallback(45, hudHotkeyCallback);

    g_zInputFfEffectSet = new zInput_FFEffectSet;
}
} // namespace Player
/**
 * Retail inline-expansion evidence: 0x425a20 scales autoTurnTargetDir by the
 * negated camera back offset with the Vec3ScaleTo shape (fld st0; fmul [x];
 * fld [y]; fmul st1) and keeps the y store that the following cameraLerpEnd.y
 * copy overwrites; no standalone retail function exists. Reconstruction model
 * (original-source helper status inferred): the same TU-resident inline helper as player_move.cpp, player_contact.cpp
 * and Camera.c; its historical declaration location is not established. Purpose: scale a vector by a scalar into an
 * output vector.
 */
inline void Vec3ScaleTo(const zVec3* vec, float scale, zVec3* out)
{
    out->x = vec->x * scale;
    out->y = vec->y * scale;
    out->z = vec->z * scale;
}

namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-ticklocalplayercontrols
 * @recoil-artifact defines .text recoil:function:0x425a20: Player::TickLocalPlayerControls.
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.fast-exp-bits
 * @recoil-raw-consumer recoil:raw-asm:gamezrecoil.zmath.vector-subtract
 * @recoil-match byte
 *
 * Raw assembly: reviewed (Pro batch Z, run 96d501c4): the zMath::FastExp integer
 * bridge [0x425aca,0x425ad5) supplying cursorBlend and the zMath::Vec3Subtract
 * expansion [0x4260af,0x4260d2) writing cameraLerpStart (requires player_input.cpp /Ob1).
 *
 * Purpose: advance local player control input, camera, movement, weapon, and
 * HUD interaction state for the current frame.
 *
 */
void __fastcall TickLocalPlayerControls(zUtil_SaveGameState* saveState)
{
    if (g_Player_LocalControlEnabled == 0) {
        return;
    }

    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    PlayerMasterModalData* const masterModalData = saveState->primaryModalState->masterModalData;

    // Retail does not initialise the snapshot; only the mouse-snapshot path fills it.
    // The cursor-mode steering read of mouseState.deltaX below does not re-check that
    // path, so with the joystick off and runtime input flag 2 clear it reads an
    // uninitialised value: retained retail behaviour outside the supported input state.
    zInput::MouseStateSnapshot mouseState;
    if (zInp::GetJoystickOption() != 0) {
        DIJOYSTATE2* const joyState = zInput::DIGetCurrentState();
        if (playerState->cameraState == kPlayerCameraStateProjectileAttached) {
            const float joyCursorY = (float)(-joyState->lY) * g_zInput_JoystickAxisConfig_Gameplay.axes[1].normScale;
            const float joyCursorX = (float)(joyState->lX) * g_zInput_JoystickAxisConfig_Gameplay.axes[0].normScale;
            playerState->cursorDeltaX = 0.0f;
            playerState->cursorDeltaY = 0.0f;
            playerState->cursorNormX = joyCursorX;
            playerState->cursorNormY = joyCursorY;
        } else {
            playerState->cursorNormX = 0.0f;
            const float joyCursorY = -((float)(joyState->lY) * g_zInput_JoystickAxisConfig_Gameplay.axes[1].normScale);
            const float cursorBlend = zMath::FastExp(g_Player_DeltaTime * -3.2f);
            playerState->cursorNormY = cursorBlend * playerState->cursorNormY + (1.0f - cursorBlend) * joyCursorY;
            playerState->steeringInput
                = (float)(-joyState->lX) * g_zInput_JoystickAxisConfig_Gameplay.axes[0].normScale;
            playerState->throttleInput
                = (float)(-joyState->lZ) * g_zInput_JoystickAxisConfig_Gameplay.axes[2].normScale;
            playerState->joyCameraYawInput
                = (float)(joyState->lRz) * g_zInput_JoystickAxisConfig_Gameplay.axes[3].normScale;
        }
    } else if ((g_Player_RuntimeInputFlags & 2) != 0) {
        zInput::MouseGetStateSnapshot(&mouseState);
        playerState->cursorDeltaX = mouseState.cursorNormX - playerState->cursorNormX;
        playerState->cursorDeltaY = mouseState.cursorNormY - playerState->cursorNormY;
        playerState->cursorNormX = mouseState.cursorNormX;
        playerState->cursorNormY = mouseState.cursorNormY;
    }

    if ((zInput::BindMapCurrentReadCommandInputState(4) & 3) != 0) {
        if (zOpt::GetThrottleMode() != 0) {
            playerState->throttleInput += g_FrameDeltaTimeSec;
        } else {
            playerState->throttleInput = 1.0f;
        }
    } else if ((zInput::BindMapCurrentReadCommandInputState(1) & 3) != 0) {
        if (zOpt::GetThrottleMode() != 0) {
            playerState->throttleInput -= g_FrameDeltaTimeSec;
        } else {
            playerState->throttleInput = -1.0f;
        }
    } else if (zOpt::GetThrottleMode() == 0 && zInp::GetJoystickOption() == 0) {
        playerState->throttleInput = 0.0f;
    }

    if ((zInput::BindMapCurrentReadCommandInputState(2) & 3) != 0) {
        playerState->steeringInput = 1.0f;
    } else if ((zInput::BindMapCurrentReadCommandInputState(3) & 3) != 0) {
        playerState->steeringInput = -1.0f;
    } else if (zInp::GetJoystickOption() == 0) {
        playerState->steeringInput = 0.0f;
    }

    if (zOpt::GetSteeringMode() == 0 && playerState->steeringInput == 0.0f && zInp::GetJoystickOption() == 0) {
        if (zOpt::GetCursorMode() != 0) {
            if (playerState->cursorDeltaX == 0.0f && mouseState.deltaX != 0) {
                playerState->steeringInput = (float)(-mouseState.deltaX) * g_Player_GameplayInputStepScale;
            }
        } else {
            const float cameraZone = g_Player_CameraZone;
            const float zoneScale = -g_Player_CameraZoneInvRange;
            if (playerState->cursorNormX > cameraZone) {
                playerState->steeringInput = (playerState->cursorNormX - cameraZone) * zoneScale;
            } else if (playerState->cursorNormX < -cameraZone) {
                playerState->steeringInput = (cameraZone + playerState->cursorNormX) * zoneScale;
            }
        }
    }

    if ((zInput::BindMapCurrentReadCommandInputState(5) & 3) != 0) {
        playerState->subVerticalInput = 1.0f;
    } else if ((zInput::BindMapCurrentReadCommandInputState(6) & 3) != 0) {
        playerState->subVerticalInput = -1.0f;
    } else {
        playerState->subVerticalInput = 0.0f;
    }

    const float pitchZone = g_Player_CameraZone;
    const float pitchScale = -g_Player_CameraZoneInvRange;
    playerState->subPitchInput = 0.0f;
    // Retail 0x425cfe compares fabs(localVel.z) with 10.0f using a strict greater-than test (test ah,0x41).
    if (masterModalData->masterType == kPlayerMasterTypeSub && (float)(fabs(playerState->localVel.z)) > 10.0f) {
        if (playerState->cursorNormY > pitchZone) {
            playerState->subPitchInput = (playerState->cursorNormY - pitchZone) * pitchScale;
        } else if (playerState->cursorNormY < -pitchZone) {
            playerState->subPitchInput = (pitchZone + playerState->cursorNormY) * pitchScale;
        }
    }

    PLAYER_CLAMP_SIGNED(playerState->subVerticalInput, 1.0f);
    PLAYER_CLAMP_SIGNED(playerState->throttleInput, 1.0f);
    PLAYER_CLAMP_SIGNED(playerState->steeringInput, 1.0f);
    PLAYER_CLAMP_SIGNED(playerState->subPitchInput, 1.0f);

    playerState->throttleInputCopy = playerState->throttleInput;
    playerState->steeringInputCopy = playerState->steeringInput;
    playerState->subVerticalInputCopy = playerState->subVerticalInput;
    playerState->subPitchInputCopy = playerState->subPitchInput;
    HudUiMgr::UpdateTargetReticleFromCursor(
        2,
        playerState->cursorNormX,
        playerState->cursorNormY,
        &playerState->storedTargetPos
    );

    const int altFireState = zInput::BindMapCurrentReadCommandInputState(12);
    if ((altFireState & 3) != 0) {
        PlayerGunFireController* const activeAltGun = playerState->activeAltGunController;
        if ((activeAltGun->optCatalogEntry->flags & 2u) == 0) {
            if ((playerState->altGunTransitionState & 0x180) != 0) {
                if ((altFireState & 1) != 0) {
                    playerState->pendingAltCameraToggle = 1;
                }
            } else if (g_Player_TotalTimeSecScaled >= activeAltGun->nextDispatchTime && playerState->playerOrdinal != 0
                && activeAltGun != &playerState->altWeaponBanks[1].controllerA) {
                playerState->altGunDispatchRequested = 1;
                activeAltGun->nextDispatchTime = activeAltGun->dispatchRepeatDelay + g_Player_TotalTimeSecScaled;
            }
        } else if (activeAltGun->ammoOrCharge > 0.0f) {
            playerState->altGunDispatchRequested = 1;
        } else if (altFireState == 1) {
            playerState->altGunDispatchRequested = altFireState;
        }
    } else {
        playerState->altGunDispatchRequested = 0;
    }

    const int primaryFireState = zInput::BindMapCurrentReadCommandInputState(11);
    playerState->usePresetGunFireDir = 0;
    if ((primaryFireState & 3) != 0) {
        PlayerGunFireController* const activePrimaryGun = playerState->activePrimaryGunController;
        if ((playerState->altGunTransitionState & 0x180) != 0) {
            playerState->usePresetGunFireDir = 1;
        } else if (g_Player_TotalTimeSecScaled >= activePrimaryGun->nextDispatchTime && playerState->playerOrdinal != 0
            && g_Time_AccumulatedTimeSec >= playerState->primaryGunGateUntilTime) {
            playerState->primaryGunDispatchRequested = 1;
            activePrimaryGun->nextDispatchTime = activePrimaryGun->dispatchRepeatDelay + g_Player_TotalTimeSecScaled;
        }
    } else {
        playerState->primaryGunDispatchRequested = 0;
    }

    if (zInput::BindMapCurrentReadCommandInputState(13) == 1) {
        playerState->altGunTriggerProcessFlag = 1;
        if (zOpt::GetNetworkEnabled() != 0 && g_HudSensorTracker.raceCheckpointMode != 0) {
            g_HudTimerPanelNetState.tenSecondWarningsEnabled = 1;
        }
    } else {
        playerState->altGunTriggerProcessFlag = 0;
    }

    if (zInput::BindMapCurrentReadCommandInputState(7) == 1) {
        ResetMouseControlStateAndRecenterCursor(saveState);
    }

    if ((zInput::BindMapCurrentReadCommandInputState(37) & 3) != 0
        && masterModalData->masterType == kPlayerMasterTypeHover && playerState->autoTurnSign == 0) {
        TransitionToMasterTypeTrack(g_LocalPlayerSaveState, 0);
    }

    // Retail 0x425fe6 tests offset 0x34 as an int (mov eax,[esi+0x34]; test eax,eax).
    if ((zInput::BindMapCurrentReadCommandInputState(38) & 3) != 0
        && masterModalData->masterType == kPlayerMasterTypeHover && playerState->probeImpactSlot1SeenFlag != 0) {
        TransitionToMasterTypeAmphib(g_LocalPlayerSaveState, 1, 0);
    }

    if ((zInput::BindMapCurrentReadCommandInputState(39) & 3) != 0
        && (masterModalData->masterType == kPlayerMasterTypeTrack
            || masterModalData->masterType == kPlayerMasterTypeAmphib)) {
        TransitionToMasterTypeHover(g_LocalPlayerSaveState, 0);
    }

    if ((zInput::BindMapCurrentReadCommandInputState(40) & 3) != 0
        && masterModalData->masterType == kPlayerMasterTypeAmphib) {
        TransitionToMasterTypeSub(g_LocalPlayerSaveState, 0);
    }

    if (zInput::BindMapCurrentReadCommandInputState(8) != 1) {
        return;
    }

    playerState->autoTurnTargetWorldPos = playerState->storedTargetPos;
    SetAutoTurnTargetDirFromWorldPoint(saveState, &playerState->autoTurnTargetWorldPos);

    // Retail passes the uninitialised target straight to gwCameraGetTarget.
    zVec3 cameraTarget;
    CZCamera::gwCameraGetTarget(g_MainCamera, &cameraTarget.x, &cameraTarget.y, &cameraTarget.z);
    zMath::Vec3Subtract(&cameraTarget, &playerState->worldPos, &playerState->cameraLerpStart);

    Vec3ScaleTo(&playerState->autoTurnTargetDir, -playerState->cameraBackOffset.z, &playerState->cameraLerpEnd);
    playerState->cameraLerpEnd.y = playerState->cameraLerpStart.y;
    ApplyCameraState(kPlayerCameraStateTargeting);
}
} // namespace Player
namespace HudUi {
/**
 * @recoil-anchor recoil:anchor:battlesport.player-input.hud-ui-handle-hotkey-command
 * @recoil-artifact defines .text recoil:function:0x426150: HudUi::HandleHotkeyCommand.
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hudui.cpp.
 * Purpose: Dispatch gameplay hotkeys to camera, HUD, cheat, chat, aux overlay, throttle, and save/load commands.
 */
void __fastcall HandleHotkeyCommand(int commandId)
{
    switch (commandId) {
    case 30:
        Player::ApplyCameraState(0);
        return;
    case 9:
        Player::ToggleSteeringModeAndResetMouseLook();
        return;
    case 31:
        Player::ApplyCameraState(2);
        return;
    case 35:
        if (zOpt::GetNetworkEnabled() == 0) {
            HudUiCallback::QueueCheatCodeState();
        }
        zInput::KeyboardResetTransitionState();
        return;
    case 33:
        zOpt::ToggleHudTypeForCurrentHwMode();
        return;
    case 32:
        HudUiMgr::ToggleHud();
        return;
    case 42:
        GameNet::BeginChatCompose();
        return;
    case 43:
        if (zOpt::GetThrottleMode() != 0) {
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x24d), 5.0f);
            zOpt::SetThrottleMode(0);
        } else {
            HudUi::ShowTopMessageLine(zLoc::GetMessageString(0x24c), 5.0f);
            zOpt::SetThrottleMode(1);
        }
        return;
    case 36:
        if (g_HudUi_AuxOverlayEnabled != 0) {
            g_HudUi_AuxOverlayEnabled = 0;
            HudUiMgr::SetFloatTimerVisible(0);
            HudUiMgr::SetAuxOverlayVisible(0);
        } else {
            g_HudUi_AuxOverlayEnabled = 1;
            HudUiMgr::SetFloatTimerVisible(1);
            HudUiMgr::SetAuxOverlayVisible(1);
        }
        return;
    case 44:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(0, g_HudUiMessage_NodeName, zLoc::GetMessageString(0x86), 2.0f);
        } else if (HudUiMainMenuDialog::CanLoadGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_QUICKLOAD);
        } else {
            HudUiMgrObjective::Show(0, g_HudUiMessage_NodeName, zLoc::GetMessageString(0x87), 2.0f);
        }
        return;
    case 45:
        if (zOpt::GetNetworkEnabled() != 0) {
            HudUiMgrObjective::Show(0, g_HudUiMessage_NodeName, zLoc::GetMessageString(0x85), 2.0f);
        } else if (HudUiMainMenuDialog::CanSaveGame() != 0) {
            RecoilStateSaveLoadTransition::QueueOpenSaveDialog(RECOIL_SAVELOAD_CAPTURE_PRESENTATION_ENABLED);
        } else {
            HudUiMgrObjective::Show(0, g_HudUiMessage_NodeName, zLoc::GetMessageString(0x82), 2.0f);
        }
        return;
    default:
        return;
    }
}
} // namespace HudUi
namespace Player {
/**
 * @recoil-anchor recoil:anchor:battlesport-player-player-resetmousecontrolstateandrecentercursor
 * @recoil-artifact defines .text recoil:function:0x426330: Player::ResetMouseControlStateAndRecenterCursor
 * @recoil-match byte
 *
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\Player\Player_Camera.cpp.
 * Purpose: Reset a save state's mouse-look offsets and recenter the mouse
 * cursor.
 * Source owner: battlesport_gameplay.player_camera_control_state_bridge,
 * not a C++ Player class and not the accepted player_camera.c source-file
 * owner.
 */
void __fastcall ResetMouseControlStateAndRecenterCursor(zUtil_SaveGameState* saveState)
{
    zUtil_PlayerStateStorage* const playerState = saveState->playerState;
    playerState->thirdPersonYawOffset = 0.0f;
    playerState->cameraElevationOffset = 0.0f;
    zInput::MouseRecenterCursor();
}
} // namespace Player
