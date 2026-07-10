#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/hud.h"

#include "Battlesport/briefing.h"
#include "Battlesport/game_net.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/zstr.h"
#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/include/zclip_rect.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRndr/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zInterp/zInterp.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "GameZRecoil/zTurret/zTurret.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "GameZRecoil/wwonline/wol_download.h"

#include <math.h>
#include <new>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/**
 * Reimplements data 0x4f32c8: g_HudUiNewGamePanelOverlayOwner.
 *
 * Purpose: own the zero-initialized new-game panel overlay singleton storage.
 */
#undef g_HudUiNewGamePanelOverlayOwner
HudUiNewGamePanelOverlayOwnerStorage g_HudUiNewGamePanelOverlayOwner = {0};
/**
 * Reimplements data 0x4e5e08: g_HudUiOptionsPanelOverlayOwner.
 *
 * Source owner: legacy.hud_ui.class_huduioptionspaneloverlayowner.
 * Purpose: own the zero-initialized options-panel overlay singleton storage.
 */
#undef g_HudUiOptionsPanelOverlayOwner
HudUiOptionsPanelOverlayOwnerStorage g_HudUiOptionsPanelOverlayOwner = {0};
/**
 * Reimplements data 0x4edc48: g_RecoilState_ConfirmQuit.
 *
 * Purpose: own the zero-initialized confirm-quit app-state singleton storage.
 */
#undef g_RecoilState_ConfirmQuit
RecoilStateConfirmQuitStorage g_RecoilState_ConfirmQuit = {0};
extern "C" int g_RecoilState_MainMenuSkipExitDelay = 0;
/**
 * Reimplements data 0x4e5dd0: g_RecoilStateControls.
 *
 * Source owner: legacy.app_shell.class_recoilstatecontrols.
 * Purpose: own the zero-initialized controls app-state singleton storage.
 */
#undef g_RecoilStateControls
RecoilStateControlsStorage g_RecoilStateControls = {0};
/**
 * Reimplements data 0x4e5ce8: g_RecoilStateCheatCode.
 *
 * Source owner: legacy.app_shell.class_recoilstatecheatcode.
 * Purpose: own the zero-initialized cheat-code app-state singleton storage.
 */
#undef g_RecoilStateCheatCode
RecoilStateCheatCodeStorage g_RecoilStateCheatCode = {0};
#define g_HudUiNewGamePanelOverlayOwner \
    (*(HudUiNewGamePanelOverlayOwner *)&g_HudUiNewGamePanelOverlayOwner)
#define g_HudUiOptionsPanelOverlayOwner \
    (*(HudUiOptionsPanelOverlayOwner *)&g_HudUiOptionsPanelOverlayOwner)
#define g_RecoilState_ConfirmQuit \
    (*(RecoilStateConfirmQuit *)&g_RecoilState_ConfirmQuit)
#define g_RecoilStateControls \
    (*(RecoilStateControls *)&g_RecoilStateControls)
#define g_RecoilStateCheatCode \
    (*(RecoilStateCheatCode *)&g_RecoilStateCheatCode)
/**
 * Reimplements data 0x4f3748: g_Hud_LowMeterBeepSample.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Holds the one-shot low-meter warning sample loaded from player.zrd.
 */
zSndSample *g_Hud_LowMeterBeepSample = 0;
/**
 * Reimplements data 0x4f374c: g_Hud_LowMeterLoopSample.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Holds the looped low-meter warning sample loaded from player.zrd.
 */
zSndSample *g_Hud_LowMeterLoopSample = 0;
/**
 * Reimplements data 0x4f3750: g_Hud_LowMeterLoopActive.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Tracks whether the low-meter loop sample has been started.
 */
int g_Hud_LowMeterLoopActive = 0;
/**
 * Reimplements data 0x4f3758: g_Hud_LowMeterBeepInterval.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Stores the low-meter one-shot beep interval from player.zrd.
 */
float g_Hud_LowMeterBeepInterval = 0.0f;
/**
 * Reimplements data 0x4f375c: g_Hud_LowMeterNextBeepTime.
 * Source owner: hud_ui.hud_low_meter_loop_sound_globals.
 * Purpose: Stores the next absolute mission time for a low-meter one-shot beep.
 */
float g_Hud_LowMeterNextBeepTime = 0.0f;

/**
 * Reimplements data 0x4da3d8: g_HudUiDialogSampleSetName.
 * Source owner: hud_ui.shared_dialog_sample_set_name.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: names the shared dialog sample set loaded by HUD/menu dialog states.
 */
char g_HudUiDialogSampleSetName[0x7] = "DIALOG";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiDialogSampleSetName) == 0x7);

/**
 * Reimplements data 0x4da8d8: g_HudUiControlsDialog_CameraModeSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD camera-mode selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_CameraModeSelectorNodeName[] = "CAMERA_MODE";
/**
 * Reimplements data 0x4da8e4: g_HudUiControlsDialog_CursorModeSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD cursor-mode selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_CursorModeSelectorNodeName[] = "CURSOR_MODE";
/**
 * Reimplements data 0x4da8f0: g_HudUiControlsDialog_SteeringModeSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD steering-mode selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_SteeringModeSelectorNodeName[] = "STEERING_MODE";
/**
 * Reimplements data 0x4da900: g_HudUiControlsDialog_ThrottleModeSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD throttle-mode selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_ThrottleModeSelectorNodeName[] = "THROTTLE_MODE";
/**
 * Reimplements data 0x4da910: g_HudUiControlsDialog_MouseOrJoystickSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD mouse-or-joystick selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_MouseOrJoystickSelectorNodeName[] = "MOUSE_OR_JOYSTICK";
/**
 * Reimplements data 0x4da924: g_HudUiControlsDialog_CommandsButtonNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD commands button node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_CommandsButtonNodeName[] = "COMMANDS_BTN";
/**
 * Reimplements data 0x4da934: g_HudUiResumeButtonNodeName.
 * Source owner: hud_ui.shared_resume_button_node_name_string.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the shared resume button node bound by Controls and NetExit HUD UI dialogs.
 */
char g_HudUiResumeButtonNodeName[] = "RESUME";
/**
 * Reimplements data 0x4da93c: g_HudUiControlsDialogSectionName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD controls dialog section loaded by HudUiControlsDialog.
 */
char g_HudUiControlsDialogSectionName[] = "CONTROLS_DIALOG";

/**
 * Reimplements data 0x4daedc: g_HudUiBackgroundConfirmQuit_CancelButtonNodeName.
 * Source owner: hud_ui.confirm_quit_dialog_literal_strings.
 * Purpose: name the ZRD cancel button node bound by HudUiBackgroundConfirmQuit.
 */
char g_HudUiBackgroundConfirmQuit_CancelButtonNodeName[0xc] = "CANCEL_QUIT";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiBackgroundConfirmQuit_CancelButtonNodeName) == 0xc);
/**
 * Reimplements data 0x4daee8: g_HudUiBackgroundConfirmQuit_OkButtonNodeName.
 * Source owner: hud_ui.confirm_quit_dialog_literal_strings.
 * Purpose: name the ZRD OK button node bound by HudUiBackgroundConfirmQuit.
 */
char g_HudUiBackgroundConfirmQuit_OkButtonNodeName[0xb] = "OK_TO_QUIT";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiBackgroundConfirmQuit_OkButtonNodeName) == 0xb);
/**
 * Reimplements data 0x4daef4: g_HudUiBackgroundConfirmQuit_SectionName.
 * Source owner: hud_ui.confirm_quit_dialog_literal_strings.
 * Purpose: name the confirm-quit ZRD section loaded by HudUiBackgroundConfirmQuit.
 */
char g_HudUiBackgroundConfirmQuit_SectionName[0xd] = "CONFIRM_QUIT";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiBackgroundConfirmQuit_SectionName) == 0xd);

const int ZOPT_GRAPHICS_PERSPECTIVE = 8;
const int ZOPT_GRAPHICS_GLOBAL_LIGHT = 0x10;
const int ZVID_HW_MODE_SOFTWARE = 0;
const float ZSND_CD_VOLUME_TO_NORMALIZED = 1.52590219e-05f;
const float ZSND_CD_NORMALIZED_TO_VOLUME = 65535.0f;

namespace {
const int kHudWeatherFxRainSlantDelta = 1;
const int kHudWeatherFxSnowTextureWidth = 16;
const int kHudWeatherFxSnowTextureHeight = 8;
const int kHudWeatherFxSnowTextureTexels =
    kHudWeatherFxSnowTextureWidth * kHudWeatherFxSnowTextureHeight;

/**
 * Original inline helper; no standalone retail function exists.
 * Observed callers: 0x4be2f0 HudWeatherFxSnow::Update and 0x4be880 HudWeatherFxRain::Update.
 * Purpose: Compute a weather particle velocity vector's squared length before normalization.
 */
inline float HudWeatherFxVec3LengthSq(
    const zVec3 *value
) {
    return value->x * value->x + value->y * value->y + value->z * value->z;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed callers: 0x4be2f0 HudWeatherFxSnow::Update.
 * Purpose: Decide whether a snow particle left the visible weather cone and must respawn.
 */
inline int HudWeatherFxSnowNeedsReset(
    const zVec3 *position
) {
    const float absZ = (float)(fabs(position->z));
    if ((float)(fabs(position->y)) > absZ) {
        return 1;
    }
    if ((float)(fabs(position->x)) > absZ) {
        return 1;
    }
    if (position->z > 1.0) {
        return 1;
    }
    if (position->z < 0.5) {
        return 1;
    }
    return 0;
}

enum zVideoRendererBackend {
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};

} // namespace

/**
 * Reimplements data 0x56bf48: g_HudWeatherFxSnow_LastCameraTarget.
 * Purpose: Retain the previous snow camera target coordinates for frame-to-frame drift.
 */
HudWeatherFxCameraTargetHistory g_HudWeatherFxSnow_LastCameraTarget = {
    0.0f,
    0.0f,
    0.0f,
    0.0f
};
/**
 * Reimplements data 0x56bf58: g_HudWeatherFxRain_LastCameraTarget.
 * Purpose: Retain the previous rain camera target coordinates for frame-to-frame drift.
 */
HudWeatherFxCameraTargetHistory g_HudWeatherFxRain_LastCameraTarget = {
    0.0f,
    0.0f,
    0.0f,
    0.0f
};
/**
 * Reimplements data 0x56bf68: g_HudWeatherFxSnow_TimeAccumulator.
 * Purpose: Accumulate elapsed snow update time.
 */
float g_HudWeatherFxSnow_TimeAccumulator = 0.0f;
/**
 * Reimplements data 0x56bf6c: g_HudWeatherFxRain_TimeAccumulator.
 * Purpose: Accumulate elapsed rain update time.
 */
float g_HudWeatherFxRain_TimeAccumulator = 0.0f;

/**
 * Reimplements 0x404ca0: HudUiElement::Draw.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::Draw.
 */
void HudUiElement::Draw() {
    DrawBase();
}

/**
 * Reimplements 0x404cb0: HudUiElement::DrawBase.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: blit the element's attached image at its current position using its clip rect.
 */
void HudUiElement::DrawBase() {
    if (bltSource != 0) {
        zVid_Image::BlitToActiveTarget(
            (zVidImagePartial *)(bltSource),
            x,
            y,
            0,
            (zVidRect32 *)(&clipRect)
        );
    }
}

/**
 * Reimplements 0x404cd0: HudUiElement::SetPos.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update both element position coordinates and invalidate the element.
 */
void HudUiElement::SetPos(
    int newX,
    int newY
) {
    x = newX;
    y = newY;
    Invalidate();
}

/**
 * Reimplements 0x404cf0: HudUiElement::SetX.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update the horizontal element position and invalidate the element.
 */
void HudUiElement::SetX(
    int newX
) {
    x = newX;
    Invalidate();
}

/**
 * Reimplements 0x404d00: HudUiElement::SetY.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update the vertical element position and invalidate the element.
 */
void HudUiElement::SetY(
    int newY
) {
    y = newY;
    Invalidate();
}

/**
 * Reimplements 0x404d10: HudUiElement::HitTestTrue.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: accept all coordinates for default HUD elements.
 */
unsigned char HudUiElement::HitTestTrue(
    int px,
    int py
) {
    (void)px;
    (void)py;
    return 1;
}

/**
 * Reimplements 0x404d20: HudUiElement::SetVisible.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update the hidden flag for one HUD element and invalidate it.
 */
void HudUiElement::SetVisible(
    int visible
) {
    if (visible != 0) {
        flags &= 0xffffffefu;
    } else {
        flags |= 0x10u;
    }

    Invalidate();
}

/**
 * Reimplements 0x404d50: HudUiElement::GetX.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return the base element x-coordinate from the recovered center-position virtual slot.
 */
int HudUiElement::GetCenterX() {
    return x;
}

/**
 * Reimplements 0x404d60: HudUiElement::GetY.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return the base element y-coordinate from the recovered center-position virtual slot.
 */
int HudUiElement::GetCenterY() {
    return y;
}

/**
 * Reimplements 0x404d70: HudUiElement::ScalarDeletingDestructor.
 * Provider-boundary: VC5 scalar-deleting destructor physical emission; this is
 * not authored HudUiElement owner evidence.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the emitted scalar-deleting-destructor thunk shape in the
 * hud.cpp physical order target.
 */
HudUiElement * HudUiElement::ScalarDeletingDestructor(
    unsigned int flags
) {
    this->~HudUiElement();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Reimplements 0x404d90: HudUiWidget::GetCenterX.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return x directly or the aligned image center x when widget alignment is active.
 */
int HudUiWidget::GetCenterX() {
    if (alignFlags != 0) {
        const int width = image != 0 ? image->width : 0;
        return x + (width / 2);
    }

    return x;
}

/**
 * Reimplements 0x404dd0: HudUiWidget::GetCenterY.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return y directly or the aligned image center y when widget alignment is active.
 */
int HudUiWidget::GetCenterY() {
    if (alignFlags != 0) {
        const int height = image != 0 ? image->height : 0;
        return y + (height / 2);
    }

    return y;
}

/**
 * Reimplements 0x404e10: HudUiWidget::RebuildBltRectFromImage.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: rebuild a widget blit rectangle from the current image dimensions.
 */
RECOIL_NO_GS void HudUiWidget::RebuildBltRectFromImage() {
    zVidImagePartial *const sourceImage = image;
    int right = x;
    int bottom = y;
    HudUiRect rect;

    rect.left = right;
    rect.top = bottom;
    right += sourceImage != 0 ? sourceImage->width : 0;
    rect.right = right;
    bottom += sourceImage != 0 ? sourceImage->height : 0;
    rect.bottom = bottom;

    SetClipRect(&rect);
}

/**
 * Reimplements 0x404e60: HudUiCircle::HitTest.
 * Original file: D:\Proj\Battlesport\hud.cpp.
 * Purpose: normalize circle hit-test membership to an integer result.
 */
int HudUiCircle::HitTest(
    int px,
    int py
) {
    return HitTestCore(
        px,
        py
    ) != 0 ? 1 : 0;
}

namespace zError {
/**
 * Reimplements 0x404e80: zError::ReportOldNoOp.
 * Purpose: Preserves the stripped retail legacy-report call ABI without producing output.
 */
void ReportOld(
    int,
    const char *,
    int,
    const char *,
    ...
) {}

} // namespace zError

namespace Player {

enum HudPhysicalPlayerMasterTypeId {
    kPlayerMasterTypeSub = 2,
    kPlayerMasterTypeTrack = 3
};

enum HudPhysicalPlayerCameraState {
    kPlayerCameraStateToggleRequest = 0,
    kPlayerCameraStateThirdPerson = 1,
    kPlayerCameraStateClearScreen = 2,
    kPlayerCameraStateFirstPerson = 3,
    kPlayerCameraStateTargeting = 4,
    kPlayerCameraStateProjectileAttached = 7,
    kPlayerCameraStateRestorePrevious = 8
};
/**
 * Reimplements 0x404e90: Player::TickActiveCameraState.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::TickActiveCameraState from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall TickActiveCameraState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    g_Player_CameraVariantUpdatedThisTick = 0;

    if (g_Player_RebuildCameraDirFlatFromCurrentTarget != 0) {
        zVec3 targetWorldPos = playerState->worldPos;
        zVec3 activeCameraTarget = {0};
        zClass_Camera::gwCameraGetTarget(
            g_MainCamera,
            &activeCameraTarget.x,
            &activeCameraTarget.y,
            &activeCameraTarget.z
        );

        playerState->cameraTargetDistance =
            zMath::Vec3DeltaLength(
                &activeCameraTarget,
                &targetWorldPos
            );

        targetWorldPos.y += playerState->cameraYOffset;
        const float dirX = targetWorldPos.x - activeCameraTarget.x;
        const float dirY = targetWorldPos.y - activeCameraTarget.y;
        const float dirZ = targetWorldPos.z - activeCameraTarget.z;
        const float invLength = 1.0f / (float)(sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ));
        playerState->cameraDirFlat.z = dirZ * invLength;
        playerState->cameraDirFlat.x = dirX * invLength;
        playerState->cameraDirFlat.y = dirY * invLength;
        g_Player_RebuildCameraDirFlatFromCurrentTarget = 0;
    }

    switch (playerState->cameraState) {
    case 1:
        UpdateChaseCameraFromInput(saveState);
        break;
    case kPlayerCameraStateClearScreen:
        UpdateTopDownCameraState(saveState);
        break;
    case kPlayerCameraStateFirstPerson:
        UpdateFirstPersonCameraFromInput(saveState);
        break;
    case kPlayerCameraStateTargeting:
        UpdateThirdPersonCamera(saveState);
        break;
    case 5:
        zGame::ReturnOnlyStub();
        break;
    case 6:
        UpdateCameraFromStoredTargetTowardPlayer(saveState);
        break;
    case kPlayerCameraStateProjectileAttached:
        RestoreThirdPersonCameraFromObstructionState(saveState);
        break;
    }

    if (g_Player_CameraVariantUpdatedThisTick == 0) {
        UpdateCameraVariantFromCameraPos(
            saveState,
            &playerState->cameraTarget
        );
    }

    UpdateCameraWeatherFxEmitterVisibility();

    if (playerState->cameraState == kPlayerCameraStateClearScreen) {
        playerState->cameraBasisCache = playerState->steerBasisNorm;
    } else {
        playerState->cameraBasisCache = playerState->cameraDirNext;
    }
}

/**
 * Reimplements 0x405040: Player::UpdateChaseCameraFromInput.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::UpdateChaseCameraFromInput from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateChaseCameraFromInput(
    zUtil_SaveGameState *saveState
) {
    const float kVerticalSpeedCameraInputCutoff = 11.0f;
    const float kCameraElevationInputScale = -8.0f;
    const float kCameraVelocitySwingScale = -0.0900000036f;
    const float kCameraElevationBaseClearance = 0.5f;
    const float kCameraHeadingDotEpsilon = 0.0000999999975f;
    const float kCameraDistanceDampingRate = -6.0f;
    const float kTrackYOffsetDampingRate = 5.0f;
    const float kNonTrackYOffsetDampingRate = 3.0f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerMasterCommonData *const masterCommonData = playerState->masterCommonData;
    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    const float cameraZone = g_Player_CameraZone;
    const float cameraZoneInvRange = g_Player_CameraZoneInvRange;
    const float maxCamYawRate = g_Player_MaxCamYawRate;

    zInput::MouseStateSnapshot mouseState = {0};
    if (zOpt::GetCursorMode() != 0) {
        memcpy(
            &mouseState,
            zInput::Mouse_GetStateSnapshotPtr(),
            sizeof(mouseState)
        );
    }

    float yawDelta = 0.0f;
    if (playerState->joyCameraYawInput != 0.0f) {
        yawDelta = maxCamYawRate * g_FrameDeltaTimeSec * playerState->joyCameraYawInput;
    } else if (zOpt::GetSteeringMode() != 0) {
        if (zOpt::GetCursorMode() != 0) {
            if (playerState->cursorDeltaX == 0.0f && mouseState.deltaX != 0) {
                yawDelta = (float)(mouseState.deltaX) * g_Player_MousePushX;
            }
        } else if (playerState->cursorNormX > cameraZone) {
            yawDelta = (playerState->cursorNormX - cameraZone) * cameraZoneInvRange *
                       maxCamYawRate * g_FrameDeltaTimeSec;
        } else if (playerState->cursorNormX < -cameraZone) {
            yawDelta = (cameraZone + playerState->cursorNormX) * cameraZoneInvRange *
                       maxCamYawRate * g_FrameDeltaTimeSec;
        }
    }
    playerState->thirdPersonYawOffset += yawDelta;

    const float invertedCameraZoneInvRange = -cameraZoneInvRange;
    if ((float)(fabs(playerState->localVel.z)) < kVerticalSpeedCameraInputCutoff) {
        float elevationDelta = 0.0f;
        if (zOpt::GetCursorMode() != 0) {
            if (playerState->cursorDeltaY == 0.0f && mouseState.deltaY != 0) {
                elevationDelta = (float)(mouseState.deltaY) * g_Player_MousePushY;
            }
        } else if (playerState->cursorNormY > cameraZone) {
            elevationDelta = (playerState->cursorNormY - cameraZone) * invertedCameraZoneInvRange *
                             g_FrameDeltaTimeSec * kCameraElevationInputScale;
        } else if (playerState->cursorNormY < -cameraZone) {
            elevationDelta = (cameraZone + playerState->cursorNormY) * invertedCameraZoneInvRange *
                             g_FrameDeltaTimeSec * kCameraElevationInputScale;
        }
        playerState->cameraElevationOffset -= elevationDelta;
    }

    const float thirdPersonSideOffset = playerState->thirdPersonSideOffset;
    const float thirdPersonBaseYOffset = playerState->thirdPersonBaseYOffset;
    const float cameraDistance = playerState->cameraDistance;

    const float horizontalProjectileSpeed = (float)(sqrt(
        playerState->projectileSpawnVel.x * playerState->projectileSpawnVel.x +
        playerState->projectileSpawnVel.z * playerState->projectileSpawnVel.z
    ));
    union {
        int bits;
        float value;
    } speedSwingBits;
    speedSwingBits.bits =
        (int)(horizontalProjectileSpeed * kCameraVelocitySwingScale * 12102200.0f) + 0x3f800000;
    const float speedSwingFactor = speedSwingBits.value;
    float maxElevationOffset = masterCommonData->cameraUdSwing[0] * speedSwingFactor;
    const float baseElevationLimit = thirdPersonBaseYOffset - kCameraElevationBaseClearance;
    if (baseElevationLimit < maxElevationOffset) {
        maxElevationOffset = baseElevationLimit;
    }

    if (playerState->cameraElevationOffset > maxElevationOffset) {
        playerState->cameraElevationOffset = maxElevationOffset;
    } else if (playerState->cameraElevationOffset < -maxElevationOffset) {
        playerState->cameraElevationOffset = -maxElevationOffset;
    }

    const float headingLerpBase = playerState->slipSfxActive != 0
                                      ? g_Player_CameraHeadingLerpBaseWhenFlagSet
                                      : g_Player_CameraHeadingLerpBaseWhenFlagClear;
    union {
        int bits;
        float value;
    } headingBlendBits;
    headingBlendBits.bits =
        (int)(-(headingLerpBase +
                  1.0f / (g_Player_CameraHeadingDotAbs + kCameraHeadingDotEpsilon)) *
              g_FrameDeltaTimeSec * 12102200.0f) +
        0x3f800000;
    const float headingBlend = headingBlendBits.value;
    zVec3 flatSteerBasis = playerState->steerBasisNorm;
    zMath::Vec3LerpNormalize(
        &playerState->cameraDirFlat,
        &flatSteerBasis,
        headingBlend
    );
    g_Player_CameraHeadingDotAbs = (float)(fabs(
        playerState->steerBasisNorm.x * playerState->cameraDirFlat.x +
        playerState->steerBasisNorm.z * playerState->cameraDirFlat.z
    ));

    float cameraDirX = playerState->cameraDirFlat.x;
    float cameraDirZ = playerState->cameraDirFlat.z;
    if (playerState->thirdPersonYawOffset != 0.0f) {
        const float yawSin = (float)(sin(playerState->thirdPersonYawOffset));
        const float yawCos = (float)(cos(playerState->thirdPersonYawOffset));
        cameraDirX = yawCos * playerState->cameraDirFlat.x - yawSin * playerState->cameraDirFlat.z;
        cameraDirZ = yawCos * playerState->cameraDirFlat.z + yawSin * playerState->cameraDirFlat.x;
    }

    float targetDistance = playerState->cameraTargetDistance;
    if (playerState->slipSfxActive == 0) {
        targetDistance = cameraDistance - g_Player_CameraElastic * playerState->localVel.z;
    }

    union {
        int bits;
        float value;
    } distanceBlendBits;
    distanceBlendBits.bits =
        (int)(g_FrameDeltaTimeSec * kCameraDistanceDampingRate * 12102200.0f) + 0x3f800000;
    const float distanceBlend = distanceBlendBits.value;
    playerState->cameraTargetDistance =
        (1.0f - distanceBlend) * targetDistance + distanceBlend * playerState->cameraTargetDistance;

    zVec3 cameraOffset = {0};
    cameraOffset.x =
        -cameraDirZ * thirdPersonSideOffset - cameraDirX * playerState->cameraTargetDistance;
    cameraOffset.z =
        cameraDirX * thirdPersonSideOffset - cameraDirZ * playerState->cameraTargetDistance;

    const float yOffsetRate = masterModalData->masterType == kPlayerMasterTypeTrack
                                  ? kTrackYOffsetDampingRate
                                  : kNonTrackYOffsetDampingRate;
    union {
        int bits;
        float value;
    } yOffsetBlendBits;
    yOffsetBlendBits.bits =
        (int)(-yOffsetRate * g_FrameDeltaTimeSec * 12102200.0f) + 0x3f800000;
    const float yOffsetBlend = yOffsetBlendBits.value;
    const float yOffsetInvBlend = 1.0f - yOffsetBlend;
    float targetYOffset = (cameraOffset.x * playerState->steerBasisNorm.x +
                              playerState->steerBasisNorm.z * cameraOffset.z) *
                          playerState->steerBasisRaw.y;
    if (masterModalData->masterType == kPlayerMasterTypeTrack && targetYOffset <= 0.0f) {
        targetYOffset = 0.0f;
    }
    playerState->thirdPersonPositionYOffset =
        yOffsetBlend * playerState->thirdPersonPositionYOffset + targetYOffset * yOffsetInvBlend;

    zVec3 cameraPos = {0};
    cameraPos.x = playerState->worldPos.x + cameraOffset.x;
    cameraPos.y = playerState->worldPos.y + thirdPersonBaseYOffset +
                  playerState->thirdPersonPositionYOffset - playerState->cameraElevationOffset;
    cameraPos.z = playerState->worldPos.z + cameraOffset.z;

    zVec3 focusPos = playerState->worldPos;
    focusPos.y += playerState->cameraYOffset;
    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        AdjustSubCameraFocusForObstruction(
            saveState,
            &focusPos
        );
    }

    const float dirX = focusPos.x - cameraPos.x;
    const float dirY = focusPos.y - cameraPos.y;
    const float dirZ = focusPos.z - cameraPos.z;
    const float invDirLength = 1.0f / (float)(sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ));
    playerState->cameraDirNext.z = dirZ * invDirLength;
    playerState->cameraDirNext.x = dirX * invDirLength;
    playerState->cameraDirNext.y = dirY * invDirLength;

    AdjustThirdPersonCameraBySideProbes(
        saveState,
        &cameraPos,
        &focusPos,
        &playerState->cameraDirNext
    );

    zClass_Camera::gwCameraSetTarget(
        g_MainCamera,
        cameraPos.x,
        cameraPos.y,
        cameraPos.z
    );
    zVec3 cameraAngles = {0};
    zVec3 *const cameraAnglesPtr =
        zMath::Vec3DirectionAnglesBetweenPoints(
            &cameraPos,
            &focusPos,
            &cameraAngles
        );
    zClass_Camera::gwCameraSetPosition(
        g_MainCamera,
        cameraAnglesPtr->x,
        cameraAnglesPtr->y,
        cameraAnglesPtr->z
    );

    playerState->cameraTarget = cameraPos;
    playerState->cameraDir = playerState->cameraDirNext;
}

/**
 * Reimplements 0x405650: Player::UpdateThirdPersonCamera
 * BN source path: D:\Proj\GameZRecoil\zGame\Player\Player_Camera.cpp.
 * Purpose: update the third-person camera target, camera orientation, horizon
 * node, and cached direction vectors from the active player state.
 */
void __fastcall UpdateThirdPersonCamera(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 cameraTarget = {
        playerState->worldPos.x + playerState->cameraLerpStart.x,
        playerState->worldPos.y + playerState->cameraLerpStart.y,
        playerState->worldPos.z + playerState->cameraLerpStart.z,
    };

    zClass_Camera::gwCameraSetTarget(
        g_MainCamera,
        cameraTarget.x,
        cameraTarget.y,
        cameraTarget.z
    );
    if (g_Player_HorizonNode != 0) {
        zClass_Object3D::gwObject3DSetPosition(
            g_Player_HorizonNode,
            cameraTarget.x,
            cameraTarget.y,
            cameraTarget.z
        );
    }

    zVec3 cameraLookAt = playerState->worldPos;
    cameraLookAt.y += playerState->cameraYOffset;

    zVec3 cameraAngles = {0};
    zMath::Vec3DirectionAnglesBetweenPoints(
        &cameraTarget,
        &cameraLookAt,
        &cameraAngles
    );
    zClass_Camera::gwCameraSetPosition(
        g_MainCamera,
        cameraAngles.x,
        cameraAngles.y,
        cameraAngles.z
    );

    zVec3 dir = {
        playerState->autoTurnTargetWorldPos.x - cameraTarget.x,
        playerState->autoTurnTargetWorldPos.y - cameraTarget.y,
        playerState->autoTurnTargetWorldPos.z - cameraTarget.z,
    };
    const float invLength = 1.0f / (float)(sqrt(dir.x * dir.x + dir.y * dir.y + dir.z * dir.z));
    playerState->cameraDirNext.x = dir.x * invLength;
    playerState->cameraDirNext.y = dir.y * invLength;
    playerState->cameraDirNext.z = dir.z * invLength;

    playerState->cameraTarget = cameraTarget;
    playerState->cameraDir = playerState->cameraDirNext;
    playerState->cameraDirFlat = playerState->cameraDirNext;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(
        &playerState->cameraDirFlat,
        &playerState->cameraDirFlat
    );
}

/**
 * Reimplements 0x4057d0: Player::UpdateTopDownCameraState.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::UpdateTopDownCameraState from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateTopDownCameraState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    playerState->cameraTarget.x = playerState->worldPos.x + playerState->cameraState2TargetOffset.x;
    playerState->cameraTarget.y = playerState->worldPos.y + playerState->cameraState2TargetOffset.y;
    playerState->cameraTarget.z = playerState->worldPos.z + playerState->cameraState2TargetOffset.z;

    zClass_Camera::gwCameraSetTarget(
        g_MainCamera,
        playerState->cameraTarget.x,
        playerState->cameraTarget.y,
        playerState->cameraTarget.z
    );
    zClass_Camera::gwCameraSetPosition(
        g_MainCamera,
        -1.54999995f,
        0.0f,
        0.0f
    );
    playerState->cameraDir.x = 0.0f;
    playerState->cameraDir.y = -1.0f;
    playerState->cameraDir.z = 0.0f;
}

/**
 * Reimplements 0x405870: Player::UpdateCameraFromStoredTargetTowardPlayer.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::UpdateCameraFromStoredTargetTowardPlayer from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateCameraFromStoredTargetTowardPlayer(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zVec3 lookAt = playerState->worldPos;
    lookAt.y += playerState->cameraState == kPlayerCameraStateThirdPerson
                    ? playerState->cameraYOffset
                    : playerState->cameraState6YOffset;

    zVec3 cameraTarget = playerState->cameraTarget;
    const float dirX = lookAt.x - cameraTarget.x;
    const float dirY = lookAt.y - cameraTarget.y;
    const float dirZ = lookAt.z - cameraTarget.z;
    const float invDirLength = 1.0f / (float)(sqrt(dirX * dirX + dirY * dirY + dirZ * dirZ));
    playerState->cameraDirNext.z = dirZ * invDirLength;
    playerState->cameraDirNext.x = dirX * invDirLength;
    playerState->cameraDirNext.y = dirY * invDirLength;

    zVec3 cameraAngles = {0};
    zVec3 *const cameraAnglesPtr =
        zMath::Vec3DirectionAnglesBetweenPoints(
            &cameraTarget,
            &lookAt,
            &cameraAngles
        );
    zClass_Camera::gwCameraSetPosition(
        g_MainCamera,
        cameraAnglesPtr->x,
        cameraAnglesPtr->y,
        cameraAnglesPtr->z
    );

    playerState->cameraDir = playerState->cameraDirNext;
    playerState->cameraDirFlat = playerState->cameraDirNext;
    playerState->cameraDirFlat.y = 0.0f;
    zMath::Vec3NormalizeXZ(
        &playerState->cameraDirFlat,
        &playerState->cameraDirFlat
    );
}

/**
 * Reimplements 0x4059a0: Player::UpdateFirstPersonCameraFromInput.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::UpdateFirstPersonCameraFromInput from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateFirstPersonCameraFromInput(
    zUtil_SaveGameState *saveState
) {
    const float kForwardSpeedClampThreshold = 10.0f;
    const float kForwardSpeedClampScale = -0.0153999999f;
    const float kElevationCameraPosScale = -0.349999994f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    const float cameraZone = g_Player_CameraZone;
    const float invertedCameraZoneInvRange = -g_Player_CameraZoneInvRange;

    if (zOpt::GetCursorMode() != 0) {
        zInput::MouseStateSnapshot mouseState = *zInput::Mouse_GetStateSnapshotPtr();
        if (playerState->cursorDeltaY == 0.0f && mouseState.deltaY != 0) {
            playerState->cameraElevationOffset -= (float)(mouseState.deltaY) * g_Player_MousePushY;
        }
    } else if (playerState->cursorNormY > cameraZone) {
        playerState->cameraElevationOffset += (playerState->cursorNormY - cameraZone) *
                                              invertedCameraZoneInvRange *
                                              g_Player_FpCamElevationRate * g_FrameDeltaTimeSec;
    } else if (playerState->cursorNormY < -cameraZone) {
        playerState->cameraElevationOffset += (cameraZone + playerState->cursorNormY) *
                                              invertedCameraZoneInvRange *
                                              g_Player_FpCamElevationRate * g_FrameDeltaTimeSec;
    }

    float elevationMin = g_Player_FpCamElevationMin;
    float elevationMax = g_Player_FpCamElevationMax;
    const float forwardSpeed = (float)(sqrt(
        playerState->projectileSpawnVel.x * playerState->projectileSpawnVel.x +
        playerState->projectileSpawnVel.z * playerState->projectileSpawnVel.z
    ));
    const float speedOverThreshold = forwardSpeed - kForwardSpeedClampThreshold;
    if (speedOverThreshold > 0.0f) {
        union {
            int bits;
            float value;
        } elevationScaleBits;
        elevationScaleBits.bits =
            (int)(speedOverThreshold * kForwardSpeedClampScale * 12102200.0f) + 0x3f800000;
        const float elevationScale = elevationScaleBits.value;
        elevationMin *= elevationScale;
        elevationMax *= elevationScale;
    }

    if (playerState->cameraElevationOffset > elevationMax) {
        playerState->cameraElevationOffset = elevationMax;
    } else if (playerState->cameraElevationOffset < elevationMin) {
        playerState->cameraElevationOffset = elevationMin;
    }

    const zMat4x3 &motionBasis = playerState->motionBasis;
    const zVec3 &localOffset = playerState->cameraState6LocalOffset;
    zVec3 cameraPoint = playerState->worldPos;
    zVec3 cameraLocalOffsetWorld = {0};
    cameraLocalOffsetWorld.x = localOffset.x * motionBasis.xx + localOffset.y * motionBasis.yx +
                               localOffset.z * motionBasis.zx;
    cameraLocalOffsetWorld.y = localOffset.x * motionBasis.xy + localOffset.y * motionBasis.yy +
                               localOffset.z * motionBasis.zy;
    cameraLocalOffsetWorld.z = localOffset.x * motionBasis.xz + localOffset.y * motionBasis.yz +
                               localOffset.z * motionBasis.zz;
    cameraPoint.x += cameraLocalOffsetWorld.x;
    cameraPoint.y += cameraLocalOffsetWorld.y;
    cameraPoint.z += cameraLocalOffsetWorld.z;

    zClass_Camera::gwCameraSetTarget(
        g_MainCamera,
        cameraPoint.x,
        cameraPoint.y,
        cameraPoint.z
    );
    playerState->cameraTarget = cameraPoint;

    zVec3 cameraPosition = playerState->cameraState6BasePos;
    cameraPosition.x -= playerState->cameraElevationOffset * kElevationCameraPosScale;
    zClass_Camera::gwCameraSetPosition(
        g_MainCamera,
        cameraPosition.x,
        cameraPosition.y,
        cameraPosition.z
    );

    playerState->cameraDirNext = playerState->steerBasisRaw;
    playerState->cameraDirFlat = playerState->steerBasisRaw;
    playerState->cameraDir = playerState->steerBasisRaw;
}

/**
 * Reimplements 0x405c90: Player::ApplyCameraState
 * BN source path: D:\Proj\GameZRecoil\zGame\Player\Player_Camera.cpp.
 * Purpose: apply a requested player camera state while preserving previous
 * state and option flags for first-person, third-person, clear-screen, and
 * projectile views.
 */
void __fastcall ApplyCameraState(
    int newState
) {
    zUtil_SaveGameState *const saveState = g_CurrentPlayerSaveState;
    if (saveState == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    int currentState = playerState->cameraState;
    if (newState == currentState || (currentState == kPlayerCameraStateClearScreen &&
                                        newState == kPlayerCameraStateProjectileAttached)) {
        return;
    }

    if (currentState == kPlayerCameraStateProjectileAttached &&
        newState != kPlayerCameraStateRestorePrevious) {
        ApplyCameraState(kPlayerCameraStateRestorePrevious);
        currentState = playerState->cameraState;
    }

    if (newState == kPlayerCameraStateToggleRequest) {
        switch (currentState) {
        case kPlayerCameraStateThirdPerson:
            newState = kPlayerCameraStateFirstPerson;
            break;
        case kPlayerCameraStateClearScreen:
            newState = kPlayerCameraStateRestorePrevious;
            break;
        case kPlayerCameraStateFirstPerson:
            newState = kPlayerCameraStateThirdPerson;
            break;
        default:
            break;
        }
    }

    int resolvedState = newState;
    switch (newState) {
    case kPlayerCameraStateThirdPerson:
        playerState->cameraLerpActive = 0;
        playerState->cameraTargetDistance = playerState->cameraDistance;
        if (currentState == kPlayerCameraStateFirstPerson) {
            zOpt::SetSteeringMode(g_Player_SavedSteeringMode);
        }
        break;

    case kPlayerCameraStateClearScreen:
        zVideo::ExchangeClearScreenBufferEnabled(1);
        break;

    case kPlayerCameraStateFirstPerson:
        g_Player_SavedSteeringMode = zOpt::GetSteeringMode();
        zOpt::SetSteeringMode(0);
        playerState->cameraElevationOffset = 0.0f;
        break;

    case kPlayerCameraStateProjectileAttached: {
        OptCatalogRuntimeInstanceStorage *const attachState =
            (OptCatalogRuntimeInstanceStorage *)(playerState->activeAltGunController->attachState);
        zClass_NodePartial *const projectileNode = attachState->projectileNode;
        zClass_Camera::gwCameraSetTarget(
            g_MainCamera,
            0.0f,
            1.0f,
            1.0f
        );
        zClass_Camera::gwCameraSetPosition(
            g_MainCamera,
            0.0f,
            0.0f,
            0.0f
        );
        zClass_Class::AddChild(
            projectileNode,
            g_MainCamera
        );
        zClass_Object3D::gwObject3DSetAlphaScale(
            projectileNode,
            0.5f
        );
        zClass_Object3D::gwObject3DSetLitFlag(
            projectileNode,
            1
        );
        g_Player_State7FxPass3Ui.SetVisible(1);
        break;
    }

    case kPlayerCameraStateRestorePrevious:
        resolvedState = playerState->previousCameraState;
        if (currentState == kPlayerCameraStateProjectileAttached) {
            OptCatalogRuntimeInstanceStorage *const attachState =
                (OptCatalogRuntimeInstanceStorage *)(playerState->activeAltGunController
                        ->attachState);
            zClass_NodePartial *const projectileNode = attachState->projectileNode;
            zClass_Class::RemoveChild(
                projectileNode,
                g_MainCamera
            );
            zClass_Object3D::gwObject3DSetAlphaScale(
                projectileNode,
                1.0f
            );
            zClass_Object3D::gwObject3DSetLitFlag(
                projectileNode,
                0
            );
            UpdateThirdPersonCamera(saveState);
            g_Player_State7FxPass3Ui.SetVisible(0);
            zTag4::Clear(&g_VariantTag_Current);
            g_Variant_CurrentTag = g_VariantTag_Current;
        } else if (currentState == kPlayerCameraStateClearScreen) {
            zVideo::ExchangeClearScreenBufferEnabled(0);
            UpdateThirdPersonCamera(saveState);
        }
        break;

    default:
        break;
    }

    playerState->previousCameraState = currentState;
    playerState->cameraState = resolvedState;

    if (resolvedState == kPlayerCameraStateThirdPerson) {
        zOpt::SetCameraMode(1);
    } else if (resolvedState == kPlayerCameraStateFirstPerson) {
        zOpt::SetCameraMode(0);
    }
}

/**
 * Reimplements 0x405ec0: Player::ToggleSteeringModeAndResetMouseLook
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: Reset active mouse-look state and toggle the steering-mode option.
 * Source owner: battlesport_gameplay.player_camera_control_state_bridge,
 * not a C++ Player class and not the accepted player_camera.c source-file
 * owner.
 */
void ToggleSteeringModeAndResetMouseLook() {
    ResetMouseControlStateAndRecenterCursor((zUtil_SaveGameState *)g_GameStateOrMapTable);
    zOpt::SetSteeringMode(zOpt::GetSteeringMode() == 0 ? 1 : 0);
}

/**
 * Reimplements 0x405ee0: Player::AdjustThirdPersonCameraByOffsetProbes.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::AdjustThirdPersonCameraByOffsetProbes from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall AdjustThirdPersonCameraByOffsetProbes(
    zUtil_SaveGameState *saveState,
    zVec3 *cameraPos,
    const zVec3 *sideDir
) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kCameraSideProbeDistance = 2.0f;
    const float kSubVerticalProbeDistance = 2.0f;

    PlayerMasterModalData *const masterModalData = saveState->primaryModalState->masterModalData;
    zClass_NodePartial *const rootNode = saveState->playerState->rootNode;

    zVec3 perpDir = {0};
    zMath::Vec3PerpXZ(
        sideDir,
        &perpDir
    );
    zVec3 normalizedPerp = {0};
    zMath::Vec3NormalizeXZ(
        &perpDir,
        &normalizedPerp
    );
    normalizedPerp.y = 0.0f;

    const zVec3 sideOffset = {
        normalizedPerp.x * kCameraSideProbeDistance,
        0.0f,
        normalizedPerp.z * kCameraSideProbeDistance,
    };

    zClass_DiSegmentEndpoints segmentPairs[3] = {0};
    segmentPairs[0].start = *cameraPos;
    segmentPairs[0].end.x = cameraPos->x + sideOffset.x;
    segmentPairs[0].end.y = cameraPos->y + sideOffset.y;
    segmentPairs[0].end.z = cameraPos->z + sideOffset.z;
    segmentPairs[1].start = *cameraPos;
    segmentPairs[1].end.x = cameraPos->x - sideOffset.x;
    segmentPairs[1].end.y = cameraPos->y - sideOffset.y;
    segmentPairs[1].end.z = cameraPos->z - sideOffset.z;

    int endpointCount = 4;
    if (masterModalData->masterType == kPlayerMasterTypeSub) {
        endpointCount = 6;
        segmentPairs[2].start = *cameraPos;
        segmentPairs[2].end.x = cameraPos->x;
        segmentPairs[2].end.y = cameraPos->y + kSubVerticalProbeDistance;
        segmentPairs[2].end.z = cameraPos->z;
    }

    zClass_cls_di::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);
    zClass_Class::gwNodeSetRaycastable(
        rootNode,
        0
    );

    PlayerProbeSampleCandidateBuffer probeBatches[3] = {0};
    zClass_cls_di::BuildProbeHitBatchesForSegments(
        g_Player_RuntimeDiScene,
        segmentPairs,
        endpointCount,
        probeBatches
    );

    zClass_Class::gwNodeSetRaycastable(
        rootNode,
        1
    );
    FilterCameraProbeBlockingHits(
        probeBatches,
        endpointCount >> 1
    );

    zVec3 outHitPos = {0};
    int result = 0;
    if (FindNearestThirdPersonCameraProbePoint(
        probeBatches,
        1,
        cameraPos,
        &outHitPos
    ) != 0) {
        result = 1;
        cameraPos->x += outHitPos.x - segmentPairs[0].end.x;
        cameraPos->z += outHitPos.z - segmentPairs[0].end.z;
    } else if (FindNearestThirdPersonCameraProbePoint(&probeBatches[1], 1, cameraPos, &outHitPos) !=
               0) {
        result = 1;
        cameraPos->x += outHitPos.x - segmentPairs[1].end.x;
        cameraPos->z += outHitPos.z - segmentPairs[1].end.z;
    }

    if (masterModalData->masterType == kPlayerMasterTypeSub &&
        FindNearestThirdPersonCameraProbePoint(
            &probeBatches[2],
            1,
            cameraPos,
            &outHitPos
        ) != 0) {
        result |= 1;
        cameraPos->y += outHitPos.y - segmentPairs[2].end.y;
    }

    return result;
}

/**
 * Reimplements 0x406110: Player::AdjustThirdPersonCameraBySideProbes.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::AdjustThirdPersonCameraBySideProbes from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall AdjustThirdPersonCameraBySideProbes(
    zUtil_SaveGameState *saveState,
    zVec3 *cameraPos,
    const zVec3 *focusPos,
    zVec3 *cameraDirNext
) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kSubCameraProbeHeightOffset = 2.20000005f;
    const float kCameraPickMaxY = 500.0f;
    const float kCameraPickRiseWindow = 0.00100000005f;
    const float kCameraFloorLift = 0.5f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_NodePartial *const rootNode = playerState->rootNode;
    const zTag4Partial savedVariantTag = g_Variant_CurrentTag;
    int cameraAdjusted = 0;

    zTag4::Clear(&g_Variant_CurrentTag);

    const zVec3 sideProbeOffset = {
        -g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->x,
        -g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->y,
        -g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->z,
    };
    const zVec3 sideProbeEndpoint = {
        cameraPos->x + sideProbeOffset.x,
        cameraPos->y + sideProbeOffset.y,
        cameraPos->z + sideProbeOffset.z,
    };

    zClass_DiSegmentEndpoints segmentPairs[2] = {0};
    segmentPairs[0].start = sideProbeEndpoint;
    segmentPairs[0].end = *focusPos;
    segmentPairs[1].start = *focusPos;
    segmentPairs[1].end = sideProbeEndpoint;

    zClass_Class::gwNodeSetRaycastable(
        rootNode,
        0
    );
    zClass_cls_di::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);

    PlayerProbeSampleCandidateBuffer probeBatches[2] = {0};
    zClass_cls_di::BuildProbeHitBatchesForSegments(
        g_Player_RuntimeDiScene,
        segmentPairs,
        4,
        probeBatches
    );

    zClass_Class::gwNodeSetRaycastable(
        rootNode,
        1
    );
    FilterCameraProbeBlockingHits(
        probeBatches,
        2
    );

    zVec3 hitPos = {0};
    if (FindNearestThirdPersonCameraProbePoint(
        probeBatches,
        2,
        focusPos,
        &hitPos
    ) != 0) {
        cameraPos->x = hitPos.x + g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->x;
        cameraPos->y = hitPos.y + g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->y;
        cameraPos->z = hitPos.z + g_Player_ThirdPersonCameraSideProbeOffsetScale * cameraDirNext->z;
        cameraAdjusted = 1;
    }

    cameraAdjusted |= AdjustThirdPersonCameraByOffsetProbes(
        saveState,
        cameraPos,
        cameraDirNext
    );

    int preferAttachmentSlot1 = 0;
    if (saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub) {
        preferAttachmentSlot1 = 1;
        const float subClampY = playerState->subModeProbeBestHeight - kSubCameraProbeHeightOffset;
        if (subClampY < cameraPos->y) {
            cameraPos->y = subClampY;
        }
    }

    g_Variant_CurrentTag = savedVariantTag;
    zClass_Class::gwNodeSetCellPickable(
        rootNode,
        0
    );
    const int pickResult = zClass_cls_di::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene,
        probeBatches,
        cameraPos->x,
        kCameraPickMaxY,
        cameraPos->z
    );
    zClass_Class::gwNodeSetCellPickable(
        rootNode,
        1
    );
    if (pickResult != 0) {
        return cameraAdjusted;
    }

    int selectedCandidateIndex = 0;
    int selectedImpactSlot = 0;
    float taggedHeight = 0.0f;
    const float selectedHeight = SelectProbeSampleHeightFromCandidates(
        probeBatches,
        &selectedCandidateIndex,
        cameraPos->y,
        kCameraPickRiseWindow,
        preferAttachmentSlot1,
        &selectedImpactSlot,
        &taggedHeight
    );
    UpdateCameraVariantFromAnchor(
        probeBatches,
        cameraPos,
        selectedCandidateIndex
    );

    const float targetY = selectedHeight + kCameraFloorLift;
    g_Player_CameraVariantUpdatedThisTick = 1;
    if (targetY <= cameraPos->y) {
        return cameraAdjusted;
    }

    cameraPos->y = targetY;

    const float dx = focusPos->x - cameraPos->x;
    const float dy = focusPos->y - cameraPos->y;
    const float dz = focusPos->z - cameraPos->z;
    const float invLength = 1.0f / (float)(sqrt(dx * dx + dy * dy + dz * dz));
    cameraDirNext->x = dx * invLength;
    cameraDirNext->y = dy * invLength;
    cameraDirNext->z = dz * invLength;

    return 1;
}

/**
 * Reimplements 0x4063f0: Player::RestoreThirdPersonCameraFromObstructionState.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::RestoreThirdPersonCameraFromObstructionState from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall RestoreThirdPersonCameraFromObstructionState(
    zUtil_SaveGameState *saveState
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    gwNode::GetWorldPosition(
        g_MainCamera,
        &playerState->cameraTarget
    );
    playerState->cameraDir = playerState->cameraObstructionDir;
}

/**
 * Reimplements 0x406430: Player::UnbindCurrentSaveStateIfSinglePlayer
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: Clear the current save-state binding when the mission is not in
 * network play.
 * Source owner: battlesport_gameplay.player_camera_control_state_bridge,
 * not a C++ Player class and not the accepted player_camera.c source-file
 * owner.
 */
void UnbindCurrentSaveStateIfSinglePlayer() {
    if (zOpt::GetNetworkEnabled() == 0) {
        g_CurrentPlayerSaveState->playerState->currentSaveStateBound = 0;
        g_CurrentPlayerSaveState = 0;
    }
}

/**
 * Reimplements 0x406450: Player::BindActiveGameStateAsCurrentSaveState
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: Bind the active local game-state record as the current save state
 * for camera/control paths.
 * Source owner: battlesport_gameplay.player_camera_control_state_bridge,
 * not a C++ Player class and not the accepted player_camera.c source-file
 * owner.
 */
void BindActiveGameStateAsCurrentSaveState() {
    zUtil_SaveGameState *const activeSaveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;
    activeSaveState->playerState->currentSaveStateBound = 1;
    g_CurrentPlayerSaveState = activeSaveState;
}

/**
 * Reimplements 0x406470: Player::UpdateCameraVariantFromCameraPos.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::UpdateCameraVariantFromCameraPos from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateCameraVariantFromCameraPos(
    zUtil_SaveGameState *saveState,
    zVec3 *cameraPos
) {
    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    PlayerProbeSampleCandidateBuffer candidateBuffers[2] = {0};

    zClass_Class::gwNodeSetCellPickable(
        playerState->rootNode,
        0
    );
    const int pickResult = zClass_cls_di::BuildPickCandidateListBelowPoint(
        g_Player_RuntimeDiScene,
        candidateBuffers,
        cameraPos->x,
        500.0f,
        cameraPos->z
    );
    zClass_Class::gwNodeSetCellPickable(
        playerState->rootNode,
        1
    );

    if (pickResult == 0) {
        int selectedCandidateIndex = 0;
        int selectedImpactSlot = 0;
        float taggedHeight = 0.0f;
        SelectProbeSampleHeightFromCandidates(
            candidateBuffers,
            &selectedCandidateIndex,
            cameraPos->y,
            0.00100000005f,
            pickResult,
            &selectedImpactSlot,
            &taggedHeight
        );
        UpdateCameraVariantFromAnchor(
            candidateBuffers,
            cameraPos,
            selectedCandidateIndex
        );
    }

    g_Player_CameraVariantUpdatedThisTick = 1;
}

/**
 * Reimplements 0x406510: Player::UpdateCameraVariantFromAnchor.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::UpdateCameraVariantFromAnchor from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall UpdateCameraVariantFromAnchor(
    PlayerProbeSampleCandidateBuffer *candidates,
    zVec3 *cameraPos,
    int selectedCandidateIndex
) {
    (void)cameraPos;

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)((void *)(g_GameStateOrMapTable->playerState));
    const zTag4Partial playerVariantTag = playerState->variantTag;

    zTag4Partial finalVariantTag = g_Player_LastValidCameraVariantTag;
    if (candidates->candidateCount > 0 &&
        candidates->entries[selectedCandidateIndex].variantTag.count > 0) {
        g_VariantTag_Current = candidates->entries[selectedCandidateIndex].variantTag;

        for (int playerTagIndex = 0; playerTagIndex < playerVariantTag.count; ++playerTagIndex) {
            const unsigned char playerTag = playerVariantTag.tags[playerTagIndex];
            int tagAlreadyPresent = 0;
            for (int tagIndex = 0; tagIndex < g_VariantTag_Current.count; ++tagIndex) {
                if (playerTag == g_VariantTag_Current.tags[tagIndex]) {
                    tagAlreadyPresent = 1;
                    break;
                }
            }

            if (tagAlreadyPresent == 0 && g_VariantTag_Current.count < 3) {
                g_VariantTag_Current.tags[g_VariantTag_Current.count] = playerTag;
                ++g_VariantTag_Current.count;
            }
        }

        int tagIsComplete = 1;
        for (int tagIndex = 0; tagIndex < g_VariantTag_Current.count; ++tagIndex) {
            if (g_VariantTag_Current.tags[tagIndex] == 0xff) {
                tagIsComplete = 0;
            }
        }

        if (tagIsComplete != 0) {
            g_Player_LastValidCameraVariantTag = g_VariantTag_Current;
            finalVariantTag = g_VariantTag_Current;
        }
    }

    g_VariantTag_Current = finalVariantTag;
    g_Variant_CurrentTag = finalVariantTag;
    zClass_Camera::gwCameraSetVariantTagOverride(
        g_MainCamera,
        &g_VariantTag_Current
    );
    zEffect::SetVariantOverridePackedIdsIfComplete(&g_VariantTag_Current);
}

/**
 * Reimplements 0x406610: Player::UpdateCameraWeatherFxEmitterVisibility.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::UpdateCameraWeatherFxEmitterVisibility from the recovered
 * Battlesport gameplay source file.
 */
void UpdateCameraWeatherFxEmitterVisibility() {
    HudUiElement *const fxElement = g_HudSensorTracker.fxPass3Obj;
    if (fxElement == 0) {
        return;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    const int isSubMode =
        saveState->primaryModalState->masterModalData->masterType == kPlayerMasterTypeSub;
    if (isSubMode != 0) {
        if ((fxElement->flags & 0x10) == 0) {
            fxElement->SetVisible(0);
        }
    } else {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        zVec3 cameraTarget = {0};
        zClass_Camera::gwCameraGetTarget(
            g_MainCamera,
            &cameraTarget.x,
            &cameraTarget.y,
            &cameraTarget.z
        );
        zClass_Class::gwNodeSetRaycastable(
            playerState->rootNode,
            0
        );
        zClass_cls_di::SetStopAfterFirstHit(0x40000);
        zClass_cls_di::SetBreakOnFirstCandidate(1);

        PlayerProbeSampleCandidateBuffer raycastCandidates = {0};
        const int raycastResult = zClass_cls_di::RaycastFindClosest(
            g_Player_RuntimeDiScene,
            &raycastCandidates,
            cameraTarget.x,
            cameraTarget.y,
            cameraTarget.z,
            cameraTarget.x,
            cameraTarget.y + 50.0f,
            cameraTarget.z
        );

        zClass_cls_di::SetBreakOnFirstCandidate(0);
        zClass_Class::gwNodeSetRaycastable(
            playerState->rootNode,
            1
        );

        const int shouldHide = raycastResult == 0 && raycastCandidates.candidateCount > 0 ? 1 : 0;
        if (shouldHide != 0) {
            if ((fxElement->flags & 0x10) == 0) {
                fxElement->SetVisible(0);
            }
        } else if ((fxElement->flags & 0x10) != 0) {
            fxElement->SetVisible(1);
        }
    }

    if ((fxElement->flags & 0x10) != 0) {
        return;
    }

    HudWeatherFx *const weatherFx = (HudWeatherFx *)(fxElement);
    weatherFx->camera = g_MainCamera;
    weatherFx->activeParticleCount = zOpt::GetReplicateMode() == 0 ? 1 : 0;
}

/**
 * Reimplements 0x406730: Player::FilterCameraProbeBlockingHits.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::FilterCameraProbeBlockingHits from the recovered
 * Battlesport gameplay source file.
 */
void __fastcall FilterCameraProbeBlockingHits(
    PlayerProbeSampleCandidateBuffer *batches,
    int batchCount
) {
    const int kIgnoredCameraProbeNodeFlag = 0x8000000;
    const int kCallbackContextPresentFlag = 0x100000;
    const int kPlayerCollisionContextKind = 2;

    if (batchCount <= 0) {
        return;
    }

    PlayerProbeSampleCandidateBuffer *batch = batches;
    for (int batchIndex = 0; batchIndex < batchCount; ++batchIndex) {
        for (int hitIndex = 0; hitIndex < batch->candidateCount; ++hitIndex) {
            zClassDiPickCandidateEntry *const candidate = &batch->entries[hitIndex];
            zClass_NodePartial *const node = candidate->node;
            const int flags = node->flags;

            if ((flags & kIgnoredCameraProbeNodeFlag) != 0) {
                candidate->node = 0;
                continue;
            }

            if ((flags & kCallbackContextPresentFlag) != 0 && node->callbackContext != 0) {
                int *const contextKind = (int *)(node->callbackContext);
                if (*contextKind == kPlayerCollisionContextKind) {
                    candidate->node = 0;
                }
            } else if (g_HudSensorTracker.raceCheckpointMode != 0 &&
                       HudSensorTracker::ParseCheckpointNumberFromNode(node) != 0) {
                candidate->node = 0;
            }
        }

        ++batch;
    }
}

/**
 * Reimplements 0x4067a0: Player::AdjustSubCameraFocusForObstruction.
 * Original source path: D:\Proj\GameZRecoil\Player\player_camera.c.
 * Purpose: reimplement Player::AdjustSubCameraFocusForObstruction from the recovered
 * Battlesport gameplay source file.
 */
int __fastcall AdjustSubCameraFocusForObstruction(
    zUtil_SaveGameState *saveState,
    zVec3 *focusPos
) {
    const int kCameraProbeStopAfterFirstHitFlag = 0x40000;
    const float kSubCameraFocusObstructionYOffset = 0.200000003f;

    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
    zClass_DiSegmentEndpoints segmentPairs[2] = {0};
    segmentPairs[0].start = playerState->worldPos;
    segmentPairs[0].end = *focusPos;
    segmentPairs[1].start = *focusPos;
    segmentPairs[1].end = playerState->worldPos;

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    zClass_cls_di::SetStopAfterFirstHit(kCameraProbeStopAfterFirstHitFlag);

    PlayerProbeSampleCandidateBuffer probeBatches[2] = {0};
    zClass_cls_di::BuildProbeHitBatchesForSegments(
        g_Player_RuntimeDiScene,
        segmentPairs,
        4,
        probeBatches
    );

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        1
    );
    FilterCameraProbeBlockingHits(
        probeBatches,
        2
    );

    zVec3 hitPos = {0};
    if (FindNearestThirdPersonCameraProbePoint(probeBatches, 2, &playerState->worldPos, &hitPos) !=
        0) {
        focusPos->y -= kSubCameraFocusObstructionYOffset;
        return 1;
    }

    return 0;
}

} // namespace Player

#include "Battlesport/mfc_three_float_dialog_body.h"

namespace zStr {

/**
 * Reimplements 0x406a00: zStr::ContainsCaseInsensitive
 * (D:\Proj\Battlesport\zStr.cpp).
 *
 * Purpose: compare uppercase bounded copies of two strings and report whether
 * the needle appears in the haystack.
 */
int __fastcall ContainsCaseInsensitive(
    const char *haystack,
    const char *needle
) {
    char uppercaseHaystack[0x80];
    char uppercaseNeedle[0x80];
    int i;

    for (i = 0; i < strlen(haystack); ++i) {
        if (i >= 0x80) {
            break;
        }
        uppercaseHaystack[i] = (char)toupper(haystack[i]);
    }
    uppercaseHaystack[i] = '\0';

    for (i = 0; i < strlen(needle); ++i) {
        if (i >= 0x80) {
            break;
        }
        uppercaseNeedle[i] = (char)toupper(needle[i]);
    }
    uppercaseNeedle[i] = '\0';

    return strstr(
        uppercaseHaystack,
        uppercaseNeedle
    ) != 0 ? 1 : 0;
}

} // namespace zStr

namespace HudCheat {

const int kNanitePanelCheatSentinel = 123456789; // 0x075bcd15
const unsigned int kHudCheatPickup901MessageId = 4096;
const unsigned int kHudCheatRespawnMessageId = 4097;
const unsigned int kHudCheatPickup903MessageId = 4098;
const unsigned int kHudCheatBindCommand36MessageId = 4100;
const unsigned int kHudCheatBindCommand31MessageId = 4101;
const int kHudCheatPickup901TypeId = 901;
const int kHudCheatRespawnPickupTypeId = 902;
const int kHudCheatPickup903TypeId = 903;
const int kHudCheatBindCommand31 = 31;
const int kHudCheatBindCommand36 = 36;
const int kHudCheatLifecycleLocal = 1;
const int kHudCheatLifecycleInactive = 4;
const int kHudCheatMasterTypeSub = 2;
const int kHudCheatMasterTypeHover = 4;
const int kHudCheatMasterTypeAmphib = 5;
const int kHudCheatAltGunTransitionReset = 16;

/**
 * Reimplements 0x406af0: HudCheat::ExecuteCommandString.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Match localized cheat commands, apply pickup effects, restore respawn state, and bind HUD hotkeys.
 */
int __fastcall ExecuteCommandString(
    CString *commandString
) {
    if (commandString->IsEmpty()) {
        return 0;
    }

    char *command = commandString->GetBuffer(1);

    if (zStr::ContainsCaseInsensitive(
            command,
            zLoc::GetMessageString(kHudCheatPickup901MessageId)
        ) != 0) {
        return Pickup::ApplyEffect(
            kHudCheatPickup901TypeId,
            0,
            (zUtil_SaveGameState *)g_GameStateOrMapTable
        );
    }

    if (zStr::ContainsCaseInsensitive(command, zLoc::GetMessageString(kHudCheatRespawnMessageId)) !=
        0) {
        zUtil_PlayerStateStorage *playerState =
            ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState;
        if (playerState->recentHitValid != 0) {
            zEffectAnim::Stop(playerState->recentHitLightHandle);
            playerState->recentHitLightHandle = 0;
            playerState->recentHitValid = 0;
        }

        if (playerState->lifecycleState == kHudCheatLifecycleInactive) {
            playerState->lifecycleState = kHudCheatLifecycleLocal;
            zOpt::SetSteeringMode(g_PlayerPrevSteeringMode);
            Player::ApplyCameraState(g_PlayerPrevCameraState);
            Player::ResetMouseControlStateAndRecenterCursor(
                (zUtil_SaveGameState *)g_GameStateOrMapTable
            );
            zEffect_Anim::NodeActionCallback(
                ((zUtil_SaveGameState *)g_GameStateOrMapTable)->playerState->destroyedRespawnFxEntry,
                playerState->rootNode
            );
            Player::ResetDamageStateAndTimedHitStatus(
                (zUtil_SaveGameState *)g_GameStateOrMapTable
            );

            int masterType =
                ((zUtil_SaveGameState *)g_GameStateOrMapTable)
                    ->primaryModalState
                    ->masterModalData
                    ->masterType;
            playerState->aiMode = 0;
            playerState->nextModeSwitchAllowedTime = 0.0f;
            playerState->autoTurnSign = 0;
            playerState->motionInput = 0;
            Player::TransitionToMasterTypeTrack(
                (zUtil_SaveGameState *)g_GameStateOrMapTable,
                1
            );
            playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;

            switch (masterType) {
            case kHudCheatMasterTypeAmphib:
                Player::TransitionToMasterTypeAmphib(
                    (zUtil_SaveGameState *)g_GameStateOrMapTable,
                    0,
                    0
                );
                break;

            case kHudCheatMasterTypeHover:
                Player::TransitionToMasterTypeHover(
                    (zUtil_SaveGameState *)g_GameStateOrMapTable,
                    0
                );
                break;

            case kHudCheatMasterTypeSub:
                Player::TransitionToMasterTypeAmphib(
                    (zUtil_SaveGameState *)g_GameStateOrMapTable,
                    0,
                    1
                );
                playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;
                Player::TransitionToMasterTypeSub(
                    (zUtil_SaveGameState *)g_GameStateOrMapTable,
                    0
                );
                break;
            }
        }

        playerState->altGunTransitionState = kHudCheatAltGunTransitionReset;
        return Pickup::ApplyEffect(
            kHudCheatRespawnPickupTypeId,
            0,
            (zUtil_SaveGameState *)g_GameStateOrMapTable
        );
    }

    if (zStr::ContainsCaseInsensitive(
            command,
            zLoc::GetMessageString(kHudCheatPickup903MessageId)
        ) != 0) {
        return Pickup::ApplyEffect(
            kHudCheatPickup903TypeId,
            0,
            (zUtil_SaveGameState *)g_GameStateOrMapTable
        );
    }

    if (zStr::ContainsCaseInsensitive(
            command,
            zLoc::GetMessageString(kHudCheatBindCommand31MessageId)
        ) != 0) {
        zInput::BindMap_Current_SetCommandCallback(
            kHudCheatBindCommand31,
            (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand)
        );
    }

    if (zStr::ContainsCaseInsensitive(
            command,
            zLoc::GetMessageString(kHudCheatBindCommand36MessageId)
        ) != 0) {
        zInput::BindMap_Current_SetCommandCallback(
            kHudCheatBindCommand36,
            (zInputCommandCallbackFn)(HudUi::HandleHotkeyCommand)
        );
    }

    return 0;
}

/**
 * Reimplements 0x406cf0: HudCheat::ClearNanitePanelCheatSentinel.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Clear the local player's nanite-panel cheat sentinel after it has been consumed.
 */
void ClearNanitePanelCheatSentinel() {
    if (g_GameStateOrMapTable == 0) {
        return;
    }

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)(g_GameStateOrMapTable->playerState);
    if (playerState->nanitePanelLevel == kNanitePanelCheatSentinel) {
        playerState->nanitePanelLevel = 0;
    }
}

} // namespace HudCheat

/**
 * Original-source inline helper for the cheat-code dialog constructor.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: configure the cheat-code text input subobject before the dialog binds it.
 */
inline HudUiCheatTextInputWidget::HudUiCheatTextInputWidget()
    : HudUiNumericTextInput() {
    textInput.AllocTextBuffer(80);
    Update("");
    SetInputActive(1);
    SetRawKeyboardCapture(1);
}

/**
 * Reimplements 0x406d20: HudUiCheatCodeDialog::HudUiCheatCodeDialog.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: Construct the cheat-code dialog, configure the input widget, and bind the ZRD widgets.
 */
HudUiCheatCodeDialog::HudUiCheatCodeDialog()
    : HudUiBackground() {
    zReader::Node *const dialogRoot =
        HudUiBackground::LoadFromZrd(
            "dialog.zrd",
            "CHEAT_CODE_DIALOG",
            0
        );
    if (dialogRoot != 0) {
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &titleWidget,
            "GO"
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &cheatInputWidget,
            "CHEATCODE"
        );
        HudUiBackground::FreeLoadedTreeRoots((int)dialogRoot);
    }
}

/**
 * Reimplements 0x406e10: HudUiCheatCodeDialog::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: Run cheat-code dialog cleanup and optionally free the object for VC5 scalar delete.
 */
HudUiBackground * HudUiCheatCodeDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    this->~HudUiCheatCodeDialog();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Reimplements 0x406e30: HudUiCheatCodeDialog::~HudUiCheatCodeDialog
 * (compiler-emitted implicit destructor).
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: destroy the cheat-code input and title widgets before background cleanup.
 */

/**
 * Reimplements 0x406e90: RecoilStateCheatCode::StaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: construct the global cheat-code state and register its atexit teardown.
 */
void RecoilStateCheatCode::StaticInitAndRegisterAtExit() {
    ConstructGlobal();
    StaticInit();
}

/**
 * Reimplements 0x406ea0: RecoilStateCheatCode::ConstructGlobal.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: run explicit construction for the global cheat-code app-state object.
 */
RecoilStateCheatCode *RecoilStateCheatCode::ConstructGlobal() {
    return &((&g_RecoilStateCheatCode)->RecoilStateCheatCode::RecoilStateCheatCode());
}

/**
 * Reimplements 0x406eb0: RecoilStateCheatCode::StaticInit.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: register the global cheat-code app-state destructor with atexit.
 */
void RecoilStateCheatCode::StaticInit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x406ec0: RecoilStateCheatCode::AtExitDestructor.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: destroy the global cheat-code app-state object during CRT shutdown.
 */
void RecoilStateCheatCode::AtExitDestructor() {
    (&g_RecoilStateCheatCode)->RecoilStateCheatCode::~RecoilStateCheatCode();
}

/**
 * Reimplements 0x406ed0: RecoilStateCheatCode::RecoilStateCheatCode.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: initialize the cheat-code app state and clear its dialog pointer.
 */
RecoilStateCheatCode::RecoilStateCheatCode() {
    m_dialog = 0;
}

/**
 * Reimplements 0x406f00: RecoilStateCheatCode::Destructor.
 * Reimplements 0x406ee0: RecoilStateCheatCode::ScalarDeletingDestructor (compiler-emitted).
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: release any active cheat-code dialog and clear the app-state dialog pointer.
 */
RecoilStateCheatCode::~RecoilStateCheatCode() {
    HudUiCheatCodeDialog *dialog = (HudUiCheatCodeDialog *)m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
        m_dialog = 0;
    }
}


/**
 * Reimplements 0x406f60: RecoilStateCheatCode::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\RecoilStateCheatCode.cpp.
 * Purpose: enter the cheat-code dialog state after capturing video and audio presentation state.
 */
int RecoilStateCheatCode::OnTryBecomeCurrent() {
    if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            0,
            0
        );
    }

    m_prevHalfResAdjustMode =
        (zVideoHalfResAdjustMode)zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    zSndPlayHandleSnapshot *const audioSnapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    m_audioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
    audioSnapshot->StopAllIfPlaying();

    zSndSampleSet_InitByName(g_HudUiDialogSampleSetName);

    HudUiCheatCodeDialog *const dialog = new HudUiCheatCodeDialog;
    m_dialog = dialog;

    dialog->SetEnabled(1);
    return 1;
}

/**
 * Reimplements 0x407010: RecoilStateCheatCode::OnDeactivate.
 * Original source path: D:\Proj\Battlesport\RecoilStateCheatCode.cpp.
 * Purpose: leave the cheat-code dialog state, restore presentation state, and execute the entered command.
 */
void RecoilStateCheatCode::OnDeactivate() {
    CString commandString;

    if (m_dialog != 0) {
        commandString =
            ((HudUiCheatCodeDialog *)m_dialog)->cheatInputWidget.GetBuffer();

        zVideo::RunPostprocessOnPrimaryBuffer();

        ((HudUiCheatCodeDialog *)m_dialog)->SetEnabled(0);

        ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        if (m_dialog != 0) {
            ((HudUiCheatCodeDialog *)m_dialog)->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }

    zSndSampleSet_DestroyByName(g_HudUiDialogSampleSetName);

    zSndPlayHandleSnapshot *const audioSnapshot =
        (zSndPlayHandleSnapshot *)(unsigned int)m_audioSnapshot;
    if (audioSnapshot != 0) {
        audioSnapshot->RestoreAllWithGlobalVolumeDelta();
    }

    zVideo::SetHalfResAdjustMode(m_prevHalfResAdjustMode);
    HudUi::SetInvalidateMode(m_prevHalfResAdjustMode);
    HudUiMgr::TriggerCurrentLayoutOnActivated();
    HudCheat::ExecuteCommandString(&commandString);
}


/**
 * Reimplements 0x4070e0: HudUiCheatCodeTitleWidget::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: Queue the cheat-code state exit when the GO widget is activated.
 */
void HudUiCheatCodeTitleWidget::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x407100: HudUiCallback::QueueExitCurrentState.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Queue an immediate exit from the current Recoil application state.
 */
void HudUiCallback::QueueExitCurrentState() {
    g_RecoilApp.QueueExitCurrentState(0);
}

/**
 * Reimplements 0x407110: HudUiCallback::QueueCheatCodeState.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Queue the cheat-code state and report successful callback handling.
 */
int HudUiCallback::QueueCheatCodeState() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilStateCheatCode,
        0
    );
    return 1;
}

#include "Battlesport/cls_stubs_body.h"
// Compiler-emitted 0x407170: VC5 scalar-deleting destructor glue for the
// byte-matched 0x4ccd50 default/base table; not an authored source-map row.
#include "Battlesport/recoil_state_base_body.h"

/**
 * Original helper evidence: no standalone retail function exists; concrete
 * dialog-host state vtable slot 2 folds to the zero-argument no-op body at
 * 0x404e80.
 * Purpose: Provide an empty enter callback for hosted dialog app states.
 */
void RecoilStateDialogHost::OnEnter() {}

/**
 * Original helper evidence: no standalone retail function exists; concrete
 * dialog-host state vtable slot 3 folds to the return-one body at 0x407130.
 * Purpose: Allow default hosted dialog state transitions to become current.
 */
int RecoilStateDialogHost::OnTryBecomeCurrent() {
    return 1;
}

/**
 * Original helper evidence: no standalone retail function exists; concrete
 * dialog-host state vtable slot 5 folds to the zero-argument no-op body at
 * 0x404e80.
 * Purpose: Provide an empty exit callback for hosted dialog app states.
 */
void RecoilStateDialogHost::OnExit() {}

/**
 * Original helper evidence: no standalone retail function exists; concrete
 * dialog-host state vtable slot 8 folds to the one-argument no-op body at
 * 0x407150.
 * Purpose: Accept resume notifications for default hosted dialog app states.
 */
void RecoilStateDialogHost::OnResume(
    int
) {}

/**
 * Original helper evidence: no standalone retail function exists; concrete
 * dialog-host state vtable slot 9 folds to the two-argument return-one body at
 * 0x407160.
 * Purpose: Keep the default hosted dialog idle/dispatch loop active.
 */
int RecoilStateDialogHost::OnIdleOrDispatch(
    unsigned int,
    unsigned int
) {
    return 1;
}

/**
 * Reimplements 0x4bdc70: HudWeatherFx::Constructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Initialize the base weather particle emitter, allocate particle buffers, reset
 * particles, and create the hardware SnowFX texture resources when needed.
 */
HudWeatherFx * HudWeatherFx::Constructor(
    int newParticleCount
) {
    HudUiElement::Constructor(
        0,
        0
    );
    clipRectOrNull = 0;
    new (this) HudWeatherFx;
    maxParticles = newParticleCount;
    particleCount = newParticleCount;
    particleQuads = (HudWeatherFxParticleQuad *)(::operator new(
        sizeof(HudWeatherFxParticleQuad) * newParticleCount
    ));

    for (int index = 0; index < newParticleCount; ++index) {
        particleQuads[index].x = -1;
        particleQuads[index].y = -1;
        particleQuads[index].width = -1;
        particleQuads[index].height = -1;
    }

    packedColor16 = 0x7fff;
    alphaStartScale = 1.0f;
    alphaEndScale = 0.0500000007f;
    camera = 0;
    activeParticleCount = 0;
    sourceBufferIndex = 0;
    destBufferIndex = 1;

    const unsigned int positionBytes = sizeof(zVec3) * newParticleCount;
    particlePositions[sourceBufferIndex] = (zVec3 *)(::operator new(positionBytes));
    particlePositions[destBufferIndex] = (zVec3 *)(::operator new(positionBytes));

    for (int resetIndex = 0; resetIndex < newParticleCount; ++resetIndex) {
        ResetParticleSlot(
            resetIndex,
            1
        );
    }

    basisVector.x = 0.0f;
    basisVector.y = 1.0f;
    basisVector.z = 0.0f;
    gravity = 1.0f;
    windDirection = 0.0f;
    windVelocity = 1.0f;
    textureName = 0;
    softwareImage = 0;
    textureRecord = 0;

    if (g_zVideo_ActiveRendererPath != 0) {
        textureName = "SnowFX";
        softwareImage = zVid_Image::Create();
        zVid_Image::SetFormatCode(
            softwareImage,
            0x0b
        );
        char *const alphaMap =
            (char *)(malloc(kHudWeatherFxSnowTextureTexels));
        void *const surfacePixels =
            malloc(kHudWeatherFxSnowTextureTexels * sizeof(unsigned short));
        zVid_Image_SetPixels(
            softwareImage,
            surfacePixels,
            alphaMap
        );
        softwareImage->formatFlagsPacked |= 0x20;
        zVid_Image::SetSize(
            softwareImage,
            kHudWeatherFxSnowTextureWidth,
            kHudWeatherFxSnowTextureHeight
        );
        textureRecord = g_zVideo_pfnCreateTextureRecord(
            textureName,
            softwareImage,
            softwareImage->formatFlagsPacked & 0x02,
            1,
            1
        );
    }

    return this;
}

/**
 * Reimplements 0x4bde20: HudWeatherFx::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Run shared weather teardown and optionally free the object for scalar delete.
 */
HudUiElement * HudWeatherFx::ScalarDeletingDestructor(
    unsigned int flags
) {
    HudWeatherFx *self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }
    return self;
}

/**
 * Reimplements 0x4bde40: HudWeatherFx::Destructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Release particle buffers and renderer-backed weather texture resources.
 */
void HudWeatherFx::Destructor() {
    if (particleQuads != 0) {
        ::operator delete(particleQuads);
    }
    if (particlePositions[0] != 0) {
        ::operator delete(particlePositions[0]);
    }
    if (particlePositions[1] != 0) {
        ::operator delete(particlePositions[1]);
    }

    if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
        if (textureRecord != 0) {
            g_zVideo_pfnTextureRecordDestroy(textureRecord);
        }
        if (softwareImage != 0) {
            zVid_Image::ReleaseIfNotDefault(softwareImage);
            softwareImage = 0;
        }
    }

}

/**
 * Reimplements 0x4be210: HudWeatherFx::ArePointBatchInsideRect.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Accept a projected weather quad only when all points lie inside the viewport.
 */
int HudWeatherFxPointBatch::ArePointBatchInsideRect(
    int pointCount,
    const HudUiRect *viewportRect
) {
    if (viewportRect == 0 || pointCount <= 0) {
        return 1;
    }

    for (int index = 0; index < pointCount; ++index) {
        if (this[index].x < (float)(viewportRect->left)) {
            return 0;
        }
        if ((float)(viewportRect->right) < this[index].x) {
            return 0;
        }
        if (this[index].y < (float)(viewportRect->top)) {
            return 0;
        }
        if ((float)(viewportRect->bottom) < this[index].y) {
            return 0;
        }
    }

    return 1;
}

/**
 * Reimplements 0x4bdee0: HudWeatherFx::ResetParticleSlot.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Respawn one particle in the weather cone and copy it into the destination buffer.
 */
void HudWeatherFx::ResetParticleSlot(
    int particleIndex,
    int
) {
    zVec3 *const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
    zVec3 *const destPosition = &particlePositions[destBufferIndex][particleIndex];

    sourcePosition->z = 0.5f - (float)(rand()) * -0.0000152592547f;

    sourcePosition->x = -1.0f - (float)(rand()) * -0.0000457777642f;
    if (sourcePosition->x < -sourcePosition->z) {
        sourcePosition->x -= -1.5f;
        sourcePosition->z = 1.5f - sourcePosition->z;
    }

    sourcePosition->y = -1.0f - (float)(rand()) * -0.0000457777642f;
    if (sourcePosition->y < -sourcePosition->z) {
        sourcePosition->y -= -1.5f;
        sourcePosition->z = 1.5f - sourcePosition->z;
    }

    *destPosition = *sourcePosition;
}

/**
 * Reimplements 0x4bdfd0: HudWeatherFx::ApplyPass3.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Draw software weather lines or submit hardware textured weather quads
 * through the pass-3 HUD element callback.
 */
void HudWeatherFx::ApplyPass3() {
    if (g_zVideo_ActiveRendererPath == ZVID_RENDERER_BACKEND_SOFTWARE) {
        zVideo_FxSurface::DrawColoredLinesBatch(
            (zVideoFxColoredLineRecord *)(particleQuads),
            particleCount,
            (zVidRect32 *)(clipRectOrNull)
        );
        return;
    }

    const int swSurfaceWasLocked = zVideo::GetSwSurfaceLockedFlag();
    if (swSurfaceWasLocked != 0) {
        zVideo::Dispatch_UnlockSwSurfaceState();
    }

    unsigned short *surfacePixels = (unsigned short *)(softwareImage->pixels);
    if (*surfacePixels != packedColor16) {
        char *surfaceAlphaMap = softwareImage->alphaMap;
        int alphaValue = 0;
        while (alphaValue < 4080) {
            *surfacePixels = packedColor16;
            ++surfacePixels;
            *surfaceAlphaMap = (char)(alphaValue >> 4);
            ++surfaceAlphaMap;
            alphaValue += 255;
        }
    }

    g_zVideo_pfnTextureRecordFinalizeUpload(
        textureRecord,
        0,
        softwareImage
    );
    zVideoD3D::SceneEnter();

    for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        HudWeatherFxParticleQuad *particleQuad = &particleQuads[particleIndex];
        float xSlant = 0.0f;
        float ySlant = 0.0f;
        if (particleQuad->width > particleQuad->height) {
            xSlant = (float)(particleQuad->slantOffset);
        } else {
            ySlant = (float)(particleQuad->slantOffset);
        }

        const float depth = particlePositions[sourceBufferIndex][particleIndex].z;
        zVideo_XyzVertex clipVerts[4];
        zVideo_TexCoord texCoords[4];
        clipVerts[0].x = (float)(particleQuad->x);
        clipVerts[0].y = (float)(particleQuad->y);
        clipVerts[0].z = depth;
        texCoords[0].u = particleQuad->texCoordUStart;
        texCoords[0].v = 0.0f;

        clipVerts[1].x = (float)(particleQuad->x) + xSlant;
        clipVerts[1].y = (float)(particleQuad->y) + ySlant;
        clipVerts[1].z = depth;
        texCoords[1].u = particleQuad->texCoordUStart;
        texCoords[1].v = 0.0f;

        clipVerts[2].x = (float)(particleQuad->x + particleQuad->width) + xSlant;
        clipVerts[2].y = (float)(particleQuad->y + particleQuad->height) + ySlant;
        clipVerts[2].z = depth;
        texCoords[2].u = particleQuad->texCoordUEnd;
        texCoords[2].v = 0.0f;

        clipVerts[3].x = (float)(particleQuad->x + particleQuad->width);
        clipVerts[3].y = (float)(particleQuad->y + particleQuad->height);
        clipVerts[3].z = depth;
        texCoords[3].u = particleQuad->texCoordUEnd;
        texCoords[3].v = 0.0f;

        if (((HudWeatherFxPointBatch *)(clipVerts))
                ->ArePointBatchInsideRect(
                    4,
                    clipRectOrNull
                ) != 0) {
            g_zVideo_pfnSubmitPolyRenderClass(
                clipVerts,
                texCoords,
                4,
                (zVideo_RenderClass *)(textureRecord),
                1,
                1.0f,
                0
            );
        }
    }

    g_zVideo_pfnFlushSortedPolys();
    zVideoD3D::SceneLeave();
    if (swSurfaceWasLocked != 0) {
        zVideo::RunPostprocessOnSwBuffer();
    }
}

/**
 * Reimplements 0x4be280: HudWeatherFxSnow::Constructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Construct the shared weather emitter and initialize snow emitter defaults.
 */
HudWeatherFxSnow * HudWeatherFxSnow::Constructor(
    int particleCount
) {
    HudWeatherFx::Constructor(particleCount);
    new (this) HudWeatherFxSnow;
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
    return this;
}

/**
 * Reimplements 0x4be2c0: HudWeatherFxSnow::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Run snow weather teardown and optionally free the object for scalar delete.
 */
HudUiElement * HudWeatherFxSnow::ScalarDeletingDestructor(
    unsigned int flags
) {
    HudWeatherFxSnow *self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }
    return self;
}

/**
 * Reimplements 0x4be2e0: HudWeatherFxSnow::Destructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Delegate snow weather teardown to the shared base weather emitter destructor.
 */
void HudWeatherFxSnow::Destructor() {
    HudWeatherFx::Destructor();
}

/**
 * Reimplements 0x4be2f0: HudWeatherFxSnow::Update.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Advance snow particles from camera drift, gravity, and wind, then project quads.
 */
void HudWeatherFxSnow::Update(
    float deltaSeconds
) {
    if ((flags & 0x10) != 0) {
        return;
    }

    g_HudWeatherFxSnow_TimeAccumulator += deltaSeconds;
    if (camera == 0) {
        return;
    }

    int viewportWidth = 0;
    int viewportHeight = 0;
    if (clipRectOrNull != 0) {
        viewportWidth = clipRectOrNull->right - clipRectOrNull->left;
        viewportHeight = clipRectOrNull->bottom - clipRectOrNull->top;
    } else {
        const zVidRect32 *const primaryRect = zVideo::GetPrimarySurfaceRectScratch();
        viewportWidth = primaryRect->right - primaryRect->left;
        viewportHeight = primaryRect->bottom - primaryRect->top;
    }
    const float viewportWidthF = (float)(viewportWidth);
    const float viewportHeightF = (float)(viewportHeight);

    zVec3 cameraTarget;
    zClass_Camera::gwCameraGetTarget(
        camera,
        &cameraTarget.x,
        &cameraTarget.y,
        &cameraTarget.z
    );

    zVec3 cameraAngles;
    zClass_Camera::gwCameraGetPosition(
        camera,
        &cameraAngles.x,
        &cameraAngles.y,
        &cameraAngles.z
    );

    zVec3 cameraTargetDrift;
    cameraTargetDrift.x =
        (g_HudWeatherFxSnow_LastCameraTarget.x - cameraTarget.x) * -0.100000001f;
    cameraTargetDrift.y =
        (g_HudWeatherFxSnow_LastCameraTarget.y - cameraTarget.y) * -0.100000001f;
    cameraTargetDrift.z =
        (g_HudWeatherFxSnow_LastCameraTarget.z - cameraTarget.z) * -0.100000001f;
    g_HudWeatherFxSnow_LastCameraTarget.x = cameraTarget.x;
    g_HudWeatherFxSnow_LastCameraTarget.y = cameraTarget.y;
    g_HudWeatherFxSnow_LastCameraTarget.z = cameraTarget.z;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateX(-cameraAngles.x);
    zMath::MatRotateY(-cameraAngles.y);
    zMath::MatTransformPointBatchInPlace(
        &cameraTargetDrift,
        1
    );
    zMath::MatStackPopPtr();

    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateZ(cameraAngles.z);
    zMath::MatRotateY(cameraAngles.y);
    zMath::MatRotateX(cameraAngles.x);

    zVec3 gravityOffset;
    const float gravityScale = (float)(gravity * 0.1);
    gravityOffset.x = basisVector.x * gravityScale;
    gravityOffset.y = basisVector.y * gravityScale;
    gravityOffset.z = basisVector.z * gravityScale;
    zMath::MatTransformPointBatchInPlace(
        &gravityOffset,
        1
    );

    zVec3 windOffset;
    const float windScale = (float)(windVelocity * 0.1);
    windOffset.x = (float)(sin(windDirection)) * windScale;
    windOffset.y = 0.0f;
    windOffset.z = (float)(cos(windDirection)) * windScale;
    zMath::MatTransformPointBatchInPlace(
        &windOffset,
        1
    );
    zMath::MatStackPopPtr();

    zVec3 particleVelocity;
    particleVelocity.x = cameraTargetDrift.x + gravityOffset.x + windOffset.x;
    particleVelocity.y = cameraTargetDrift.y + gravityOffset.y + windOffset.y;
    particleVelocity.z = cameraTargetDrift.z + gravityOffset.z + windOffset.z;
    if (HudWeatherFxVec3LengthSq(&particleVelocity) >= 1.0) {
        zMath::Vec3Normalize(&particleVelocity);
    }

    zVec3 probeVelocity = particleVelocity;
    if (HudWeatherFxVec3LengthSq(&probeVelocity) >= 0.010000000000000002) {
        zMath::Vec3Normalize(&probeVelocity);
        probeVelocity.x *= 0.100000001f;
        probeVelocity.y *= 0.100000001f;
        probeVelocity.z *= 0.100000001f;
    }

    for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        const zVec3 *const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
        zVec3 *const destPosition = &particlePositions[destBufferIndex][particleIndex];
        destPosition->x = sourcePosition->x + particleVelocity.x;
        destPosition->y = sourcePosition->y + particleVelocity.y;
        destPosition->z = sourcePosition->z + particleVelocity.z;

        zVec3 probePosition;
        probePosition.x = sourcePosition->x + probeVelocity.x;
        probePosition.y = sourcePosition->y + probeVelocity.y;
        probePosition.z = sourcePosition->z + probeVelocity.z;

        const float sourceDepthFactor = 1.5f - sourcePosition->z;
        const float probeDepthFactor = 1.5f - probePosition.z;
        HudWeatherFxParticleQuad *const particleQuad = &particleQuads[particleIndex];
        particleQuad->x =
            (int)(((probeDepthFactor * probePosition.x) - -0.5f) *
                  viewportWidthF);
        particleQuad->y =
            (int)(((probeDepthFactor * probePosition.y) - -0.5f) *
                  viewportHeightF);
        particleQuad->width =
            (int)(((sourceDepthFactor * sourcePosition->x) - -0.5f) *
                  viewportWidthF) -
            particleQuad->x;
        particleQuad->height =
            (int)(((sourceDepthFactor * sourcePosition->y) - -0.5f) *
                  viewportHeightF) -
            particleQuad->y;
        particleQuad->color16 = packedColor16;
        particleQuad->texCoordUStart = probeDepthFactor * alphaStartScale;
        particleQuad->texCoordUEnd = sourceDepthFactor * alphaEndScale;
        particleQuad->slantOffset = (int)(((float)(activeParticleCount + 1)) * sourceDepthFactor *
                                          3.5);

        if (HudWeatherFxSnowNeedsReset(destPosition) != 0) {
            ResetParticleSlot(
                particleIndex,
                0
            );
        }
    }

    HudUiElement::Update(deltaSeconds);

    const int oldSourceBufferIndex = sourceBufferIndex;
    sourceBufferIndex = destBufferIndex;
    destBufferIndex = oldSourceBufferIndex;
}

/**
 * Reimplements 0x4be810: HudWeatherFxRain::Constructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Construct the shared weather emitter and initialize rain emitter defaults.
 */
HudWeatherFxRain * HudWeatherFxRain::Constructor(
    int particleCount
) {
    HudWeatherFx::Constructor(particleCount);
    new (this) HudWeatherFxRain;
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
    return this;
}

/**
 * Reimplements 0x4be850: HudWeatherFxRain::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Run rain weather teardown and optionally free the object for scalar delete.
 */
HudUiElement * HudWeatherFxRain::ScalarDeletingDestructor(
    unsigned int flags
) {
    HudWeatherFxRain *self = this;
    Destructor();
    if ((flags & 1) != 0) {
        ::operator delete(self);
    }
    return self;
}

/**
 * Reimplements 0x4be870: HudWeatherFxRain::Destructor.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Delegate rain weather teardown to the shared base weather emitter destructor.
 */
void HudWeatherFxRain::Destructor() {
    HudWeatherFx::Destructor();
}

/**
 * Reimplements 0x4be880: HudWeatherFxRain::Update.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Advance rain particles from camera drift, gravity, and wind, then project quads.
 */
void HudWeatherFxRain::Update(
    float deltaSeconds
) {
    if ((flags & 0x10) != 0) {
        return;
    }

    g_HudWeatherFxRain_TimeAccumulator += deltaSeconds;
    if (camera == 0) {
        return;
    }

    int viewportWidth = 0;
    int viewportHeight = 0;
    if (clipRectOrNull != 0) {
        viewportWidth = clipRectOrNull->right - clipRectOrNull->left;
        viewportHeight = clipRectOrNull->bottom - clipRectOrNull->top;
    } else {
        const zVidRect32 *const primaryRect = zVideo::GetPrimarySurfaceRectScratch();
        viewportWidth = primaryRect->right - primaryRect->left;
        viewportHeight = primaryRect->bottom - primaryRect->top;
    }
    const float viewportWidthF = (float)(viewportWidth);
    const float viewportHeightF = (float)(viewportHeight);

    zVec3 cameraTarget;
    zClass_Camera::gwCameraGetTarget(
        camera,
        &cameraTarget.x,
        &cameraTarget.y,
        &cameraTarget.z
    );

    zVec3 cameraAngles;
    zClass_Camera::gwCameraGetPosition(
        camera,
        &cameraAngles.x,
        &cameraAngles.y,
        &cameraAngles.z
    );

    zVec3 cameraTargetDrift;
    cameraTargetDrift.x =
        (g_HudWeatherFxRain_LastCameraTarget.x - cameraTarget.x) * -0.100000001f;
    cameraTargetDrift.y =
        (g_HudWeatherFxRain_LastCameraTarget.y - cameraTarget.y) * -0.100000001f;
    cameraTargetDrift.z =
        (g_HudWeatherFxRain_LastCameraTarget.z - cameraTarget.z) * -0.100000001f;
    g_HudWeatherFxRain_LastCameraTarget.x = cameraTarget.x;
    g_HudWeatherFxRain_LastCameraTarget.y = cameraTarget.y;
    g_HudWeatherFxRain_LastCameraTarget.z = cameraTarget.z;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateX(-cameraAngles.x);
    zMath::MatRotateY(-cameraAngles.y);
    zMath::MatTransformPointBatchInPlace(
        &cameraTargetDrift,
        1
    );
    zMath::MatStackPopPtr();

    zMath::MatStackPushPtr((float *)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateZ(cameraAngles.z);
    zMath::MatRotateY(cameraAngles.y);
    zMath::MatRotateX(cameraAngles.x);

    zVec3 gravityOffset;
    const float gravityScale = (float)(gravity * 0.1);
    gravityOffset.x = basisVector.x * gravityScale;
    gravityOffset.y = basisVector.y * gravityScale;
    gravityOffset.z = basisVector.z * gravityScale;
    zMath::MatTransformPointBatchInPlace(
        &gravityOffset,
        1
    );

    zVec3 windOffset;
    const float windScale = (float)(windVelocity * 0.1);
    windOffset.x = (float)(sin(windDirection)) * windScale;
    windOffset.y = 0.0f;
    windOffset.z = (float)(cos(windDirection)) * windScale;
    zMath::MatTransformPointBatchInPlace(
        &windOffset,
        1
    );
    zMath::MatStackPopPtr();

    zVec3 particleVelocity;
    particleVelocity.x = cameraTargetDrift.x + gravityOffset.x + windOffset.x;
    particleVelocity.y = cameraTargetDrift.y + gravityOffset.y + windOffset.y;
    particleVelocity.z = cameraTargetDrift.z + gravityOffset.z + windOffset.z;
    if (HudWeatherFxVec3LengthSq(&particleVelocity) >= 1.0) {
        zMath::Vec3Normalize(&particleVelocity);
    }

    zVec3 probeVelocity = particleVelocity;
    if (HudWeatherFxVec3LengthSq(&probeVelocity) >= 0.010000000000000002) {
        zMath::Vec3Normalize(&probeVelocity);
        probeVelocity.x *= 0.100000001f;
        probeVelocity.y *= 0.100000001f;
        probeVelocity.z *= 0.100000001f;
    }

    for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        const zVec3 *const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
        zVec3 *const destPosition = &particlePositions[destBufferIndex][particleIndex];
        destPosition->x = sourcePosition->x + particleVelocity.x;
        destPosition->y = sourcePosition->y + particleVelocity.y;
        destPosition->z = sourcePosition->z + particleVelocity.z;

        zVec3 probePosition;
        probePosition.x = sourcePosition->x + probeVelocity.x;
        probePosition.y = sourcePosition->y + probeVelocity.y;
        probePosition.z = sourcePosition->z + probeVelocity.z;

        const float sourceDepthFactor = 1.5f - sourcePosition->z;
        const float probeDepthFactor = 1.5f - probePosition.z;
        HudWeatherFxParticleQuad *const particleQuad = &particleQuads[particleIndex];
        particleQuad->x =
            (int)(((probeDepthFactor * probePosition.x) - -0.5f) *
                  viewportWidthF);
        particleQuad->y =
            (int)(((probeDepthFactor * probePosition.y) - -0.5f) *
                  viewportHeightF);
        particleQuad->width =
            (int)(((sourceDepthFactor * sourcePosition->x) - -0.5f) *
                  viewportWidthF) -
            particleQuad->x;
        particleQuad->height =
            (int)(((sourceDepthFactor * sourcePosition->y) - -0.5f) *
                  viewportHeightF) -
            particleQuad->y;
        particleQuad->color16 = packedColor16;
        particleQuad->texCoordUStart = probeDepthFactor * alphaStartScale;
        particleQuad->texCoordUEnd = sourceDepthFactor * alphaEndScale;
        particleQuad->slantOffset = kHudWeatherFxRainSlantDelta;

        ResetParticleSlot(
            particleIndex,
            0
        );
    }

    HudUiElement::Update(deltaSeconds);

    const int oldSourceBufferIndex = sourceBufferIndex;
    sourceBufferIndex = destBufferIndex;
    destBufferIndex = oldSourceBufferIndex;
}

/**
 * Reimplements 0x41c6c0: HudUiNewGamePanelOverlayOwner::QueueEnter.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Queue the global new-game panel overlay owner as the next app state.
 */
void HudUiNewGamePanelOverlayOwner::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudUiNewGamePanelOverlayOwner,
        0
    );
}

/**
 * Reimplements 0x41c5e0: HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Construct the global new-game panel overlay owner and register its shutdown cleanup.
 */
void HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Reimplements 0x41c5f0: HudUiNewGamePanelOverlayOwner::StaticInit.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Placement-construct the global new-game panel overlay owner.
 */
HudUiNewGamePanelOverlayOwner *HudUiNewGamePanelOverlayOwner::StaticInit() {
    return new (&g_HudUiNewGamePanelOverlayOwner) HudUiNewGamePanelOverlayOwner;
}

/**
 * Reimplements 0x41c6a0: HudUiNewGamePanelOverlayOwner::RegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Register the global new-game panel overlay owner destructor with CRT exit cleanup.
 */
void HudUiNewGamePanelOverlayOwner::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x41c6b0: HudUiNewGamePanelOverlayOwner::AtExitDestructor.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Run global new-game panel overlay owner cleanup during CRT exit.
 */
void HudUiNewGamePanelOverlayOwner::AtExitDestructor() {
    g_HudUiNewGamePanelOverlayOwner.~HudUiNewGamePanelOverlayOwner();
}

/**
 * Original-source inline helper evidence: No standalone retail function is
 * expected; the constructor body is inlined into 0x41c5f0
 * HudUiNewGamePanelOverlayOwner::StaticInit.
 *
 * Purpose: initialize the typed new-game overlay app-state owner.
 */
HudUiNewGamePanelOverlayOwner::HudUiNewGamePanelOverlayOwner() {
    m_dialog = 0;
}

/**
 * Reimplements 0x41c630: HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Disable and destroy the active new-game panel owned by this app state.
 */
HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner() {
    HudUiNewGamePanel *panel = (HudUiNewGamePanel *)m_dialog;
    if (panel != 0) {
        panel->SetEnabled(0);

        panel = (HudUiNewGamePanel *)m_dialog;
        if (panel != 0) {
            panel->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }
}

/**
 * Reimplements 0x41c560: HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Create, enable, and retain the new-game panel for the overlay state.
 */
int HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent() {
    HudUiNewGamePanel *const panel = new HudUiNewGamePanel;
    m_dialog = panel;
    panel->SyncIntensityFromDifficulty();
    panel->SetEnabled(1);
    return 1;
}

/**
 * Reimplements 0x41c290: HudUiNewGamePanel::HudUiNewGamePanel.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Construct the new-game panel, bind its ZRD widgets, and load the saved player name.
 */
HudUiNewGamePanel::HudUiNewGamePanel()
    : HudUiBackground() {
    zReader::Node *const loadedSection =
        HudUiBackground::LoadFromZrd(
            "dialog.zrd",
            "NEWGAMEPANEL",
            0
        );
    if (loadedSection != 0) {
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &backWidget,
            "BACK"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &startWidget,
            "START"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &nameInput,
            "NAME"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            &intensity,
            "INTENSITY"
        );
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    nameInput.Update(zOpt_GetPlayerName());
}

/**
 * Reimplements 0x41c3b0: HudUiNewGamePanel_NameInput::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Refresh and activate the player-name input with raw keyboard capture.
 */
void HudUiNewGamePanel_NameInput::OnActivate() {
    textInput.AllocTextBuffer(21);
    HudUiNumericTextInput::Update(zOpt_GetPlayerName());
    HudUiNumericTextInput::OnActivate();
    HudUiNumericTextInput::SetRawKeyboardCapture(1);
}

/**
 * Reimplements 0x41c400: HudUiNewGamePanel::Destructor.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Destroy the new-game panel child widgets and background base in reverse construction order.
 */
void HudUiNewGamePanel::Destructor() {
    intensity.DestructorCore();
    nameInput.Destructor();
    startWidget.DestructorCore();
    backWidget.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x41c3e0: HudUiNewGamePanel::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Run new-game panel destruction and optionally free the panel storage.
 */
HudUiBackground * HudUiNewGamePanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Reimplements 0x41c4e0: HudUiNewGamePanel::SyncIntensityFromDifficulty.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Reflect the saved game difficulty option in the panel intensity selector.
 */
void HudUiNewGamePanel::SyncIntensityFromDifficulty() {
    intensity.SetSelectedIndex(zOpt::GetGameDifficultyMode());
}

/**
 * Reimplements 0x41c500: HudUiNewGamePanel::StartGameFromFields.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Commit new-game options and queue the transition into mission FMV startup.
 */
void HudUiNewGamePanel::StartGameFromFields() {
    HudCheat::ClearNanitePanelCheatSentinel();
    zOpt::SetPlayerName(nameInput.GetBuffer());
    zOpt::SetGameDifficultyMode(intensity.selectedIndex);
    ((HudUiBackgroundContainer *)(&g_RecoilApp.m_missionFmvState))
        ->HudUiBackgroundContainer::SetEnabled(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_missionFmvState,
        0
    );
}

/**
 * Reimplements 0x41c270: HudUiNewGamePanel_StartButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiNewGamePanel.cpp.
 * Purpose: Start the new game through the owning panel before running normal widget activation.
 */
void HudUiNewGamePanel_StartButton::OnActivate() {
    HudUiNewGamePanel *const panel = (HudUiNewGamePanel *)(owner);
    if (panel != 0) {
        panel->StartGameFromFields();
    }

    HudUiZrdWidget::OnActivate();
}

#include "Battlesport/hud_command_binding_layer_body.h"

/**
 * HudOptionsDialog owner evidence: BN constructor 0x40c720 builds the
 * HudUiBackground base, constructs the typed options-panel child widgets,
 * installs their derived dispatch identities, binds the dialog.zrd node names,
 * and destructor 0x40cf60 destroys the same members in reverse order. The
 * retail HudUiOptionsPanel_* dispatch data is compiler-emitted class evidence,
 * not production-source permission for FTable records or table factories.
 */

/**
 * Reimplements 0x40c6e0: HudUiOptionsPanelBackButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: store the selected HUD type and leave the options panel.
 */
void HudUiOptionsPanelBackButton::OnActivate() {
    HudOptionsDialog *const ownerDialog = (HudOptionsDialog *)(owner);
    const int hudType = ownerDialog->fullHudToggle.checked != 0 ? ZOPT_HUD_TYPE_PERSPECTIVE
                                                                : ZOPT_HUD_TYPE_STANDARD;
    zOpt::SetHudTypeForCurrentHwMode(hudType);

    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x40c720: HudOptionsDialog::HudOptionsDialog.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: construct the options dialog widget tree and bind each ZRD panel control.
 */
HudOptionsDialog::HudOptionsDialog() : HudUiBackground() {
    zReader::Node *const loadedSection = LoadFromZrd(
        "dialog.zrd",
        g_HudUiOptionsPanel_SectionName,
        0
    );
    if (loadedSection != 0) {
        BindWidgetByName(
            loadedSection,
            &backButton,
            "BACK"
        );
        BindWidgetByName(
            loadedSection,
            &lightingToggle,
            g_HudUiOptionsPanel_LightingToggleNodeName
        );
        BindWidgetByName(
            loadedSection,
            &perspectiveToggle,
            g_HudUiOptionsPanel_PerspectiveToggleNodeName
        );
        BindWidgetByName(
            loadedSection,
            &fullHudToggle,
            g_HudUiOptionsPanel_FullHudToggleNodeName
        );
        BindWidgetByName(
            loadedSection,
            &objectDetailSelector,
            g_HudUiOptionsPanel_ObjectDetailSelectorNodeName
        );
        BindWidgetByName(
            loadedSection,
            &textureMemorySelector,
            g_HudUiOptionsPanel_TextureMemorySelectorNodeName
        );
        BindWidgetByName(
            loadedSection,
            &effectsSelector,
            g_EffectsZrdNodeName
        );
        BindWidgetByName(
            loadedSection,
            &soundActiveToggle,
            g_HudUiOptionsPanel_SoundActiveToggleNodeName
        );
        BindWidgetByName(
            loadedSection,
            &soundQualitySelector,
            g_HudUiOptionsPanel_SoundQualitySelectorNodeName
        );
        BindWidgetByName(
            loadedSection,
            &soundVolumeWidget,
            g_HudUiOptionsPanel_SoundVolumeWidgetNodeName
        );
        BindWidgetByName(
            loadedSection,
            &musicEnableToggle,
            g_HudUiOptionsPanel_MusicEnableToggleNodeName
        );
        BindWidgetByName(
            loadedSection,
            &musicVolumeWidget,
            g_HudUiOptionsPanel_MusicVolumeWidgetNodeName
        );
        BindWidgetByName(
            loadedSection,
            &resolutionSelector,
            g_HudUiOptionsPanel_ResolutionCycleNodeName
        );
        FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Recovered compatibility wrapper for HudUiOptionsPanel_Lighting::SyncFromOptions.
 * No standalone retail function is assigned to this wrapper; 0x40c9e0 is the
 * address-backed option-sync body in this owner cluster.
 * Purpose: toggle the global lighting graphics flag from the lighting checkbox.
 */
void HudUiOptionsPanel_Lighting::OnActivate() {
    HudUiOptionsPanel_Lighting::SyncFromOptions();
}

/**
 * Reimplements 0x40c9c0: HudUiOptionsPanel_Lighting::InitFromOptions.
 * Source-faithful helper: no standalone retail function is assigned; 0x40c9c0
 * is the address-backed option-init body in this HudOptionsDialog owner cluster.
 * The wrapper preserves the recovered virtual PostLoadFromZrd call shape.
 * Purpose: synchronize the lighting toggle from the active hardware-mode graphics flags.
 */
void HudUiOptionsPanel_Lighting::PostLoadFromZrd() {
    HudUiOptionsPanel_Lighting::InitFromOptions();
}

/**
 * Reimplements 0x40c9c0: HudUiOptionsPanel_Lighting::InitFromOptions.
 * Purpose: synchronize the lighting toggle from the active hardware-mode graphics flags.
 */
void HudUiOptionsPanel_Lighting::InitFromOptions() {
    SetChecked(zOpt::GetGraphicsFlagsForCurrentHwMode() & ZOPT_GRAPHICS_GLOBAL_LIGHT);
}

/**
 * Reimplements 0x40c9e0: HudUiOptionsPanel_Lighting::SyncFromOptions.
 * Purpose: toggle the global lighting graphics flag from the lighting checkbox.
 */
void HudUiOptionsPanel_Lighting::SyncFromOptions() {
    const int flags = zOpt::GetGraphicsFlagsForCurrentHwMode();
    HudUiCheckToggleWidget::OnActivate();
    zOpt::SetGraphicsFlagsForCurrentHwMode(
        checked != 0 ? (flags | ZOPT_GRAPHICS_GLOBAL_LIGHT) : (flags & ~ZOPT_GRAPHICS_GLOBAL_LIGHT)
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Recovered compatibility wrapper for HudUiOptionsPanel_Perspective::SyncFromOptions.
 * No standalone retail function is assigned to this wrapper; 0x40ca40 is the
 * address-backed option-sync body in this owner cluster.
 * Purpose: route activation through the recovered perspective option sync.
 */
void HudUiOptionsPanel_Perspective::OnActivate() {
    HudUiOptionsPanel_Perspective::SyncFromOptions();
}

/**
 * Reimplements 0x40ca20: HudUiOptionsPanel_Perspective::InitFromOptions.
 * Source-faithful helper: no standalone retail function is assigned; 0x40ca20
 * is the address-backed option-init body in this HudOptionsDialog owner cluster.
 * The wrapper preserves the recovered virtual PostLoadFromZrd call shape.
 * Purpose: synchronize the perspective toggle from the active hardware-mode graphics flags.
 */
void HudUiOptionsPanel_Perspective::PostLoadFromZrd() {
    HudUiOptionsPanel_Perspective::InitFromOptions();
}

/**
 * Reimplements 0x40ca20: HudUiOptionsPanel_Perspective::InitFromOptions.
 * Purpose: synchronize the perspective toggle from the active hardware-mode graphics flags.
 */
void HudUiOptionsPanel_Perspective::InitFromOptions() {
    SetChecked(zOpt::GetGraphicsFlagsForCurrentHwMode() & ZOPT_GRAPHICS_PERSPECTIVE);
}

/**
 * Reimplements 0x40ca40: HudUiOptionsPanel_Perspective::SyncFromOptions.
 * Purpose: toggle the perspective graphics flag and refresh span routine selection.
 */
void HudUiOptionsPanel_Perspective::SyncFromOptions() {
    const int flags = zOpt::GetGraphicsFlagsForCurrentHwMode();
    HudUiCheckToggleWidget::OnActivate();
    zOpt::SetGraphicsFlagsForCurrentHwMode(
        checked != 0 ? (flags | ZOPT_GRAPHICS_PERSPECTIVE) : (flags & ~ZOPT_GRAPHICS_PERSPECTIVE)
    );
    zRndr::SelectSpanRoutines();
}

/**
 * Reimplements 0x40ca80: HudUiOptionsPanel_FullHud::InitFromOptions.
 * Source-faithful helper: no standalone retail function is assigned; 0x40ca80
 * is the address-backed option-init body in this HudOptionsDialog owner cluster.
 * The wrapper preserves the recovered virtual PostLoadFromZrd call shape.
 * Purpose: synchronize the full-HUD toggle from the active hardware-mode HUD type.
 */
void HudUiOptionsPanel_FullHud::PostLoadFromZrd() {
    HudUiOptionsPanel_FullHud::InitFromOptions();
}

/**
 * Reimplements 0x40ca80: HudUiOptionsPanel_FullHud::InitFromOptions.
 * Purpose: synchronize the full-HUD toggle from the active hardware-mode HUD type.
 */
void HudUiOptionsPanel_FullHud::InitFromOptions() {
    SetChecked(zOpt::GetHudTypeForCurrentHwMode() == ZOPT_HUD_TYPE_PERSPECTIVE);
}

/**
 * Reimplements 0x40caa0: HudUiOptionsPanel_FullHud::OnActivate.
 * Purpose: run inherited toggle activation for the full-HUD option.
 */
void HudUiOptionsPanel_FullHud::OnActivate() {
    HudUiCheckToggleWidget::OnActivate();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Recovered compatibility wrapper for HudUiOptionsPanel_ObjectDetail::SyncFromOptions.
 * No standalone retail function is assigned to this wrapper; 0x40cad0 is the
 * address-backed option-sync body in this owner cluster.
 * Purpose: route activation through the recovered object-detail option sync.
 */
void HudUiOptionsPanel_ObjectDetail::OnActivate() {
    HudUiOptionsPanel_ObjectDetail::SyncFromOptions();
}

/**
 * Reimplements 0x40cab0: HudUiOptionsPanel_ObjectDetail::InitFromOptions.
 * Source-faithful helper: no standalone retail function is assigned; 0x40cab0
 * is the address-backed option-init body in this HudOptionsDialog owner cluster.
 * The wrapper preserves the recovered virtual PostLoadFromZrd call shape.
 * Purpose: synchronize the object detail selector from the active hardware-mode object LOD.
 */
void HudUiOptionsPanel_ObjectDetail::PostLoadFromZrd() {
    HudUiOptionsPanel_ObjectDetail::InitFromOptions();
}

/**
 * Reimplements 0x40cab0: HudUiOptionsPanel_ObjectDetail::InitFromOptions.
 * Purpose: synchronize the object detail selector from the active hardware-mode object LOD.
 */
void HudUiOptionsPanel_ObjectDetail::InitFromOptions() {
    SetIndexClamped(zOpt::GetObjectLODForCurrentHwMode());
}

/**
 * Reimplements 0x40cad0: HudUiOptionsPanel_ObjectDetail::SyncFromOptions.
 * Purpose: advance the object detail selector and store its object LOD option.
 */
void HudUiOptionsPanel_ObjectDetail::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetObjectLODForCurrentHwMode(selectedIndex);
}

/**
 * Reimplements 0x40cad0: HudUiOptionsPanel_ObjectDetail::SyncFromOptions.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_ObjectDetail::SyncFromOptions.
 */
void HudUiOptionsPanel_TextureMemory::OnActivate() {
    SyncFromOptions();
}

/**
 * Reimplements 0x40caf0: HudUiOptionsPanel_TextureMemory::InitFromOptions.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_TextureMemory::InitFromOptions.
 */
void HudUiOptionsPanel_TextureMemory::PostLoadFromZrd() {
    InitFromOptions();
}

/**
 * Reimplements 0x40caf0: HudUiOptionsPanel_TextureMemory::InitFromOptions.
 * Purpose: synchronize the texture memory selector from the active hardware-mode option.
 */
void HudUiOptionsPanel_TextureMemory::InitFromOptions() {
    SetIndexClamped(zOpt::GetTextureMemoryForCurrentHwMode());
}

/**
 * Reimplements 0x40cb10: HudUiOptionsPanel_TextureMemory::SyncFromOptions.
 * Purpose: advance the texture memory selector and store its option.
 */
void HudUiOptionsPanel_TextureMemory::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetTextureMemoryForCurrentHwMode(selectedIndex);
}

/**
 * Reimplements 0x40cb10: HudUiOptionsPanel_TextureMemory::SyncFromOptions.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_TextureMemory::SyncFromOptions.
 */
void HudUiOptionsPanel_Effects::OnActivate() {
    SyncFromOptions();
}

/**
 * Reimplements 0x40cb30: HudUiOptionsPanel_Effects::InitFromOptions.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_Effects::InitFromOptions.
 */
void HudUiOptionsPanel_Effects::PostLoadFromZrd() {
    InitFromOptions();
}

/**
 * Reimplements 0x40cb30: HudUiOptionsPanel_Effects::InitFromOptions.
 * Purpose: synchronize the effects selector and constrain software-renderer choices.
 */
void HudUiOptionsPanel_Effects::InitFromOptions() {
    int level = zOpt::GetEffectsLevelForCurrentHwMode();
    if (zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
        if (level == 0) {
            level = 1;
        }
        SetVisibleRange(
            1,
            3
        );
    }

    SetIndexClamped(level);
}

/**
 * Reimplements 0x40cb70: HudUiOptionsPanel_Effects::SyncFromOptions.
 * Purpose: advance the effects selector and store its effects-level option.
 */
void HudUiOptionsPanel_Effects::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetEffectsLevelForCurrentHwMode(selectedIndex);
}

/**
 * Reimplements 0x40cb70: HudUiOptionsPanel_Effects::SyncFromOptions.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_Effects::SyncFromOptions.
 */
void HudUiOptionsPanel_SoundActive::OnActivate() {
    SyncFromOptions();
}

/**
 * Reimplements 0x40cb90: HudUiOptionsPanel_SoundActive::InitFromOptions.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_SoundActive::InitFromOptions.
 */
void HudUiOptionsPanel_SoundActive::PostLoadFromZrd() {
    InitFromOptions();
}

/**
 * Reimplements 0x40cb90: HudUiOptionsPanel_SoundActive::InitFromOptions.
 * Purpose: synchronize the sound-active toggle from the mute-sound option.
 */
void HudUiOptionsPanel_SoundActive::InitFromOptions() {
    SetChecked(zOpt::GetMuteSoundOption() == 0);
}

/**
 * Reimplements 0x40cbb0: HudUiOptionsPanel_SoundActive::SyncFromOptions.
 * Purpose: toggle sound activity and store the inverse mute-sound option.
 */
void HudUiOptionsPanel_SoundActive::SyncFromOptions() {
    HudUiCheckToggleWidget::OnActivate();
    zOpt::SetMuteSoundOption(checked == 0);
}

/**
 * Reimplements 0x40cbb0: HudUiOptionsPanel_SoundActive::SyncFromOptions.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_SoundActive::SyncFromOptions.
 */
void HudUiOptionsPanel_SoundQuality::OnActivate() {
    SyncFromOptions();
}

/**
 * Reimplements 0x40cbd0: HudUiOptionsPanel_SoundQuality::InitFromOptions.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_SoundQuality::InitFromOptions.
 */
void HudUiOptionsPanel_SoundQuality::PostLoadFromZrd() {
    InitFromOptions();
}

/**
 * Reimplements 0x40cbd0: HudUiOptionsPanel_SoundQuality::InitFromOptions.
 * Purpose: synchronize the sound quality selector from the sound LOD option.
 */
void HudUiOptionsPanel_SoundQuality::InitFromOptions() {
    SetIndexClamped(zOpt::GetSoundLODOption());
}

/**
 * Reimplements 0x40cbf0: HudUiOptionsPanel_SoundQuality::SyncFromOptions.
 * Purpose: advance the sound quality selector and store its sound LOD option.
 */
void HudUiOptionsPanel_SoundQuality::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetSoundLODOption(selectedIndex);
}

/**
 * Reimplements 0x40cc10: HudUiOptionsPanel_SoundVolume::SyncFromOptions.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_SoundVolume::SyncFromOptions.
 */
void HudUiOptionsPanel_SoundVolume::PostLoadFromZrd() {
    SyncFromOptions();
}

/**
 * Reimplements 0x40cc10: HudUiOptionsPanel_SoundVolume::SyncFromOptions.
 * Purpose: synchronize the sound volume fill widget from the stored sound volume option.
 */
void HudUiOptionsPanel_SoundVolume::SyncFromOptions() {
    SetNormalizedValue(zOpt::GetSoundVolumeOption());
}

/**
 * Reimplements 0x40cc30: HudUiOptionsPanel_SoundVolume::OnActivate.
 * Purpose: update and store sound volume from the fill-widget cursor position.
 */
void HudUiOptionsPanel_SoundVolume::OnActivate() {
    UpdateNormalizedFromCursor();
    zOpt::SetSoundVolumeOption(normalizedValue);
    SetNormalizedValue(zOpt::GetSoundVolumeOption());
}

/**
 * Reimplements 0x40cc60: HudUiOptionsPanel_MusicEnable::SyncFromOptions.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_MusicEnable::SyncFromOptions.
 */
void HudUiOptionsPanel_MusicEnable::PostLoadFromZrd() {
    SyncFromOptions();
}

/**
 * Reimplements 0x40cc60: HudUiOptionsPanel_MusicEnable::SyncFromOptions.
 * Purpose: synchronize the CD-audio toggle from the stored music-enable option.
 */
void HudUiOptionsPanel_MusicEnable::SyncFromOptions() {
    SetChecked(zSnd::GetCDAudioOption());
}

/**
 * Reimplements 0x40cc80: HudUiOptionsPanel_MusicEnable::OnActivate.
 * Purpose: toggle CD audio playback and store the music-enable option.
 */
void HudUiOptionsPanel_MusicEnable::OnActivate() {
    HudUiCheckToggleWidget::OnActivate();
    if (checked == 0) {
        zSnd::SetCDAudioOption(0);
        zSndCd::Stop();
    } else {
        zSnd::SetCDAudioOption(1);
        zSndCd::PlayTrackWithMode(
            2,
            5
        );
    }
}

/**
 * Reimplements 0x40ccc0: HudUiOptionsPanel_MusicVolume::SyncFromOptions.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_MusicVolume::SyncFromOptions.
 */
void HudUiOptionsPanel_MusicVolume::PostLoadFromZrd() {
    SyncFromOptions();
}

/**
 * Reimplements 0x40ccc0: HudUiOptionsPanel_MusicVolume::SyncFromOptions.
 * Purpose: synchronize the music volume fill widget from the current CD volume.
 */
void HudUiOptionsPanel_MusicVolume::SyncFromOptions() {
    unsigned short primaryVolume = 0;
    unsigned short secondaryVolume = 0;
    zSndCd::GetVolume(
        &primaryVolume,
        &secondaryVolume
    );
    SetNormalizedValue((float)(primaryVolume)*ZSND_CD_VOLUME_TO_NORMALIZED);
}

/**
 * Reimplements 0x40cd00: HudUiOptionsPanel_MusicVolume::OnActivate.
 * Purpose: update and store CD volume from the fill-widget cursor position.
 */
void HudUiOptionsPanel_MusicVolume::OnActivate() {
    UpdateNormalizedFromCursor();
    const unsigned short volume = (unsigned short)(normalizedValue * ZSND_CD_NORMALIZED_TO_VOLUME);
    zSndCd::SetVolume(
        volume,
        volume
    );
}

/**
 * Reimplements 0x40cd30: HudUiOptionsPanel_Resolution::SyncFromOptions.
 * Source model note: Source-faithful helper recovered from address-backed callers in this
 * source file.
 * Purpose: preserve the recovered HUD behavior for HudUiOptionsPanel_Resolution::SyncFromOptions.
 */
void HudUiOptionsPanel_Resolution::PostLoadFromZrd() {
    SyncFromOptions();
}

/**
 * Reimplements 0x40cd30: HudUiOptionsPanel_Resolution::SyncFromOptions.
 * Purpose: synchronize and constrain the resolution selector for the active renderer.
 */
void HudUiOptionsPanel_Resolution::SyncFromOptions() {
    const int accelerationOption = zVid::GetAccelerationOption();
    const int modeCase = zVid::GetVideoModeIndexFromOptions() - 2;
    if ((unsigned int)(modeCase) > 5u) {
        return;
    }

    if (accelerationOption == ZVID_HW_MODE_SOFTWARE) {
        switch (modeCase) {
        case 0:
            SetIndexClamped(3);
            SetVisibleRange(
                2,
                4
            );
            return;
        case 1:
            SetIndexClamped(1);
            SetVisibleRange(
                0,
                2
            );
            return;
        case 2:
            SetIndexClamped(2);
            SetVisibleRange(
                2,
                4
            );
            return;
        case 3:
            SetIndexClamped(0);
            SetVisibleRange(
                0,
                2
            );
            return;
        case 4:
            SetIndexClamped(4);
            SetVisibleRange(
                4,
                5
            );
            return;
        case 5:
            SetIndexClamped(5);
            SetVisibleRange(
                5,
                6
            );
            return;
        }
    }

    switch (modeCase) {
    case 0:
        SetIndexClamped(3);
        SetVisibleRange(
            3,
            4
        );
        return;
    case 1:
        SetIndexClamped(1);
        SetVisibleRange(
            1,
            2
        );
        return;
    case 2:
        SetIndexClamped(2);
        SetVisibleRange(
            2,
            3
        );
        return;
    case 3:
        SetIndexClamped(0);
        SetVisibleRange(
            0,
            1
        );
        return;
    case 4:
        SetIndexClamped(4);
        SetVisibleRange(
            4,
            5
        );
        return;
    case 5:
        SetIndexClamped(5);
        SetVisibleRange(
            5,
            6
        );
        return;
    }
}

/**
 * Reimplements 0x40ce80: HudUiOptionsPanel_Resolution::OnActivate.
 * Purpose: advance the resolution selector and queue the corresponding video mode.
 */
void HudUiOptionsPanel_Resolution::OnActivate() {
    AdvanceSelectionAndActivate();
    switch (selectedIndex) {
    case 0:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_640X480);
        return;
    case 1:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_320X240_TO_640X480);
        return;
    case 2:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_640X400);
        return;
    case 3:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_320X200_TO_640X400);
        return;
    case 4:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_800X600);
        return;
    case 5:
        RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(ZVID_MODE_1024X768);
        return;
    }
}

/**
 * Provider-boundary compatibility entry for the HudOptionsDialog scalar
 * deleting destructor. The plan classifies 0x40cf00 as compiler-generated VC++
 * glue, not authored HudOptionsDialog source.
 * Purpose: run options dialog teardown and optionally free the dialog storage.
 */
HudUiBackground * HudOptionsDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    this->HudOptionsDialog::~HudOptionsDialog();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Provider boundary 0x40cf20: VC5 compiler/EH cleanup forwarding thunk.
 * Purpose: emit the complete destructor cleanup thunk for HudUiZrdWidget in the options-dialog layer.
 */
void HudUiZrdWidget::DestructorCoreThunk() {
    this->HudUiZrdWidget::~HudUiZrdWidget();
}

/**
 * Provider boundary 0x40cf30: VC5 compiler/EH cleanup forwarding thunk.
 * Purpose: emit the complete destructor cleanup thunk for HudUiCheckToggleWidget in the options-dialog layer.
 */
void HudUiCheckToggleWidget::DestructorCoreThunk() {
    this->HudUiCheckToggleWidget::~HudUiCheckToggleWidget();
}

/**
 * Provider boundary 0x40cf40: VC5 compiler/EH cleanup forwarding thunk.
 * Purpose: emit the complete destructor cleanup thunk for HudUiCycleSelectorWidget in the options-dialog layer.
 */
void HudUiCycleSelectorWidget::DestructorCoreThunk() {
    this->HudUiCycleSelectorWidget::~HudUiCycleSelectorWidget();
}

/**
 * Provider boundary 0x40cf50: VC5 compiler/EH cleanup forwarding thunk.
 * Purpose: emit the complete destructor cleanup thunk for HudUiFillBitmap in the options-dialog layer.
 */
void HudUiFillBitmap::DestructorCoreThunk() {
    this->HudUiFillBitmap::~HudUiFillBitmap();
}

/**
 * Reimplements 0x40cf60: HudOptionsDialog::~HudOptionsDialog.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: let VC5 emit the options dialog member/base teardown state machine.
 */
HudOptionsDialog::~HudOptionsDialog() {
}


/**
 * Reimplements 0x40d070: HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Construct the global options overlay owner and register its exit cleanup.
 */
void HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Reimplements 0x40d080: HudUiOptionsPanelOverlayOwner::StaticInit.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Placement-construct the global options overlay owner.
 */
HudUiOptionsPanelOverlayOwner *HudUiOptionsPanelOverlayOwner::StaticInit() {
    return new (&g_HudUiOptionsPanelOverlayOwner) HudUiOptionsPanelOverlayOwner;
}

/**
 * Reimplements 0x40d090: HudUiOptionsPanelOverlayOwner::RegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Register the global options overlay owner destructor for process exit.
 */
void HudUiOptionsPanelOverlayOwner::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x40d0a0: HudUiOptionsPanelOverlayOwner::AtExitDestructor.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Run process-exit cleanup for the global options overlay owner.
 */
void HudUiOptionsPanelOverlayOwner::AtExitDestructor() {
    g_HudUiOptionsPanelOverlayOwner.~HudUiOptionsPanelOverlayOwner();
}

/**
 * Reimplements 0x40d0b0: HudUiOptionsPanelOverlayOwner::HudUiOptionsPanelOverlayOwner.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Initialize the options overlay owner with no active panel.
 */
HudUiOptionsPanelOverlayOwner::HudUiOptionsPanelOverlayOwner() {
    m_dialog = 0;
}

/**
 * Reimplements 0x40d0e0: HudUiOptionsPanelOverlayOwner::~HudUiOptionsPanelOverlayOwner.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Disable and destroy the active options dialog panel during owner teardown.
 */
HudUiOptionsPanelOverlayOwner::~HudUiOptionsPanelOverlayOwner() {
    HudOptionsDialog *panel = (HudOptionsDialog *)m_dialog;
    if (panel != 0) {
        panel->SetEnabled(0);

        panel = (HudOptionsDialog *)m_dialog;
        if (panel != 0) {
            panel->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }
}

/**
 * Reimplements 0x40d150: HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Create and enable the options dialog panel when the overlay owner becomes current.
 */
int HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent() {
    HudOptionsDialog *const panel = new HudOptionsDialog;
    m_dialog = panel;
    panel->SetEnabled(1);
    return 1;
}

/**
 * Reimplements 0x40d1c0: HudUiOptionsPanelOverlayOwner::QueueEnter.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Queue the global options-panel overlay owner as the next app state.
 */
void HudUiOptionsPanelOverlayOwner::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudUiOptionsPanelOverlayOwner,
        0
    );
}

/**
 * Reimplements 0x409160: HudUiCreditsBackButton::OnActivate.
 * Purpose: Queue exit from the credits state and run the inherited activation behavior.
 */
void HudUiCreditsBackButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x409180: HudUiCreditsQuitButton::OnActivate.
 * Purpose: Queue the credits-exit shutdown path and run the inherited activation behavior.
 */
void HudUiCreditsQuitButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x408a30: HudUiControlsDialog::Constructor.
 * Original source path: D:\Proj\Battlesport\hud_ui_dialogs.cpp.
 * Purpose: Construct the controls dialog, bind its ZRD widgets, and seed option selectors from current input/options.
 * Evidence: BN/source slice builds HudUiBackground, resume/commands widgets, five option selectors, loads
 * dialog.zrd/CONTROLS_DIALOG, binds named controls, then seeds zInp/zOpt selector indices.
 */
HudUiControlsDialog * HudUiControlsDialog::Constructor() {
    new ((HudUiBackground *)this) HudUiBackground;

    resumeWidget.Constructor();
    commandsWidget.Constructor();
    mouseOrJoystickSelector.Constructor();
    throttleModeSelector.Constructor();
    steeringModeSelector.Constructor();
    cursorModeSelector.Constructor();
    cameraModeSelector.Constructor();

    zReader::Node *const dialogRoot = HudUiBackground::LoadFromZrd(
        "dialog.zrd",
        g_HudUiControlsDialogSectionName,
        0
    );
    if (dialogRoot != 0) {
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &resumeWidget,
            g_HudUiResumeButtonNodeName
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &commandsWidget,
            g_HudUiControlsDialog_CommandsButtonNodeName
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &mouseOrJoystickSelector,
            g_HudUiControlsDialog_MouseOrJoystickSelectorNodeName
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &throttleModeSelector,
            g_HudUiControlsDialog_ThrottleModeSelectorNodeName
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &steeringModeSelector,
            g_HudUiControlsDialog_SteeringModeSelectorNodeName
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &cursorModeSelector,
            g_HudUiControlsDialog_CursorModeSelectorNodeName
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &cameraModeSelector,
            g_HudUiControlsDialog_CameraModeSelectorNodeName
        );
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)dialogRoot);
    }

    mouseOrJoystickSelector.SetSelectedIndex(zInp::GetJoystickOption());
    throttleModeSelector.SetSelectedIndex(zOpt::GetThrottleMode());
    steeringModeSelector.SetSelectedIndex(zOpt::GetSteeringMode());
    cursorModeSelector.SetSelectedIndex(zOpt::GetCursorMode());
    cameraModeSelector.SetSelectedIndex(zOpt::GetCameraModePlayerState() == 1 ? 1 : 0);
    return this;
}

/**
 * Reimplements 0x408c20: HudUiControlsDialog_CommandsWidget::OnActivate.
 * Original source path: D:\Proj\Battlesport\hud_ui_dialogs.cpp.
 * Purpose: Queue the command-dialog state from the controls dialog Commands widget before running inherited ZRD activation.
 * Evidence: BN/source slice calls HudCmdDialogState::QueueEnter, then chains HudUiZrdWidget::OnActivate.
 */
void HudUiControlsDialog_CommandsWidget::OnActivate() {
    HudCmdDialogState::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x408c40: HudUiControlsDialog::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\hud_ui_dialogs.cpp.
 * Purpose: Run controls dialog destruction and optionally release heap storage for VC5 scalar delete.
 * Evidence: BN/source slice calls the recovered destructor, tests delete flag bit 0, conditionally
 * calls operator delete, and returns this as the HudUiBackground base pointer.
 */
HudUiBackground * HudUiControlsDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Provider boundary 0x408c60: VC5 compiler/EH cleanup forwarding thunk.
 * Purpose: emit the complete destructor cleanup thunk for the zero-data controls-dialog option selector subtype.
 */
void HudUiControlsDialog_OptionSelector::DestructorCoreThunk() {
    this->HudUiZrdWidgetEx17C::~HudUiZrdWidgetEx17C();
}

/**
 * Reimplements 0x408c70: HudUiControlsDialog::Destructor.
 * Original source path: D:\Proj\Battlesport\hud_ui_dialogs.cpp.
 * Purpose: Destroy the controls dialog child widgets in reverse construction order before background cleanup.
 * Evidence: BN/source slice tears down camera, cursor, steering, throttle, mouse/joystick selectors,
 * commands/resume widgets, then the HudUiBackground base.
 */
void HudUiControlsDialog::Destructor() {
    cameraModeSelector.DestructorCore();
    cursorModeSelector.DestructorCore();
    steeringModeSelector.DestructorCore();
    throttleModeSelector.DestructorCore();
    mouseOrJoystickSelector.DestructorCore();
    commandsWidget.DestructorCore();
    resumeWidget.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x408d20: RecoilStateControls::StaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: construct the global controls app state and register its CRT shutdown destructor.
 */
void RecoilStateControls::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *BattlesportHudCrtInitializerFn)();
/* VC5 emits this controls-state startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
BattlesportHudCrtInitializerFn s_BattlesportHudCrtInit_RecoilStateControls =
    RecoilStateControls::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
/* VC5 emits this confirm-quit-state startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
BattlesportHudCrtInitializerFn s_BattlesportHudCrtInit_RecoilStateConfirmQuit =
    RecoilStateConfirmQuit::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
/* VC5 emits this new-game-panel owner startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
BattlesportHudCrtInitializerFn s_BattlesportHudCrtInit_HudUiNewGamePanelOverlayOwner =
    HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
/* VC5 emits this options-panel owner startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
BattlesportHudCrtInitializerFn s_BattlesportHudCrtInit_HudUiOptionsPanelOverlayOwner =
    HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

#if defined(_MSC_VER) && defined(_M_IX86)
/* VC5 emits this cheat-code-state startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
BattlesportHudCrtInitializerFn s_BattlesportHudCrtInit_RecoilStateCheatCode =
    RecoilStateCheatCode::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * Reimplements 0x408d30: RecoilStateControls::StaticInit.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: placement-construct the zero-initialized global controls app state singleton.
 */
RecoilStateControls *RecoilStateControls::StaticInit() {
    return new (&g_RecoilStateControls) RecoilStateControls;
}

/**
 * Reimplements 0x408d40: RecoilStateControls::RegisterAtExit.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: register the global controls app state destructor with the CRT atexit list.
 */
void RecoilStateControls::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x408d50: RecoilStateControls::AtExitDestructor.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: destroy the global controls app state during CRT shutdown.
 */
void RecoilStateControls::AtExitDestructor() {
    g_RecoilStateControls.~RecoilStateControls();
}

/**
 * Reimplements 0x408d60: RecoilStateControls::RecoilStateControls.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: initialize the controls app state and clear its dialog pointer.
 */
RecoilStateControls::RecoilStateControls() {
    m_dialog = 0;
}

/**
 * Reimplements 0x408d90: RecoilStateControls::Destructor.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: release the owned controls dialog and clear the dialog pointer.
 */
RecoilStateControls::~RecoilStateControls() {
    HudUiControlsDialog *dialog = (HudUiControlsDialog *)m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

/**
 * Reimplements 0x408df0: RecoilStateControls::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: lazily create and enable the controls dialog, then seed option selectors.
 */
int RecoilStateControls::OnTryBecomeCurrent() {
    if (m_dialog == 0) {
        HudUiControlsDialog *dialog =
            (HudUiControlsDialog *) ::operator new(sizeof(HudUiControlsDialog));
        if (dialog != 0) {
            dialog = dialog->Constructor();
        }
        m_dialog = dialog;
    }

    HudUiControlsDialog *const dialog = (HudUiControlsDialog *)m_dialog;
    dialog->SetEnabled(1);

    dialog->mouseOrJoystickSelector.SetSelectedIndex(zInp::GetJoystickOption());
    dialog->throttleModeSelector.SetSelectedIndex(zOpt::GetThrottleMode());
    dialog->steeringModeSelector.SetSelectedIndex(zOpt::GetSteeringMode());
    dialog->cursorModeSelector.SetSelectedIndex(zOpt::GetCursorMode());
    dialog->cameraModeSelector.SetSelectedIndex(
        zOpt::GetCameraModePlayerState() == 1 ? 1 : 0
    );

    return 1;
}

/**
 * Reimplements 0x408ec0: RecoilStateControls::OnDeactivate.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: commit controls dialog selections, deactivate and blit the dialog, then delete it.
 */
void RecoilStateControls::OnDeactivate() {
    if (m_dialog == 0) {
        return;
    }

    HudUiControlsDialog *const dialog = (HudUiControlsDialog *)m_dialog;
    zInp::SetJoystickOption(
        zInput::DI_SetJoystickEnabled(dialog->mouseOrJoystickSelector.selectedIndex)
    );
    zOpt::SetCursorMode(dialog->cursorModeSelector.selectedIndex);
    zOpt::SetCameraMode(dialog->cameraModeSelector.selectedIndex);
    zOpt::SetThrottleMode(dialog->throttleModeSelector.selectedIndex);
    zOpt::SetSteeringMode(dialog->steeringModeSelector.selectedIndex);

    if (dialog->steeringModeSelector.selectedIndex == 0 && g_GameStateOrMapTable != 0) {
        Player::ResetMouseControlStateAndRecenterCursor(
            (zUtil_SaveGameState *)g_GameStateOrMapTable
        );
    }

    dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();

    HudUiControlsDialog *dialogToDelete = (HudUiControlsDialog *)m_dialog;
    if (dialogToDelete != 0) {
        dialogToDelete->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

#include "Battlesport/recoil_state_dialog_host_on_suspend_body.h"

/**
 * Reimplements 0x408fa0: RecoilStateControls::OnResume.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: resume the controls dialog after a nested app state returns.
 */
void RecoilStateControls::OnResume(
    int activateCode
) {
    (void)activateCode;

    if (m_dialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    HudUiControlsDialog *const dialog = (HudUiControlsDialog *)m_dialog;
    dialog->SetEnabled(1);
    ((HudUiContainer *)dialog)->InvalidateChildren();
    ((HudUiContainer *)dialog)->UpdateAll(0.0f);
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zOpt_ViewRectSection *const dstRect = zOpt::GetWindowSection();
    zOpt_ViewRectSection *const srcRect = zOpt::GetWindowSection();
    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)srcRect,
        (zVidRect32 *)dstRect,
        1,
        1
    );
}

/**
 * Reimplements 0x408ff0: RecoilStateControls::QueueEnter.
 * Original source path: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: queue the global controls app state on the Recoil app state stack.
 */
void RecoilStateControls::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilStateControls,
        0
    );
}

#include "Battlesport/hud_runtime_layer_body.h"

namespace HudLowMeterLoopSound {

/**
 * Reimplements 0x439b20: HudLowMeterLoopSound::SetLoopActive.
 * Original source: D:\Proj\Battlesport\Hud.cpp.
 * Purpose: Starts or stops the low-meter loop sample on active-state changes.
 */
void __fastcall SetLoopActive(
    int enabled
) {
    const int wasActive = g_Hud_LowMeterLoopActive;
    if (enabled == 0) {
        if (wasActive != 0) {
            g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
            g_Hud_LowMeterLoopActive = 0;
        }
        return;
    }

    if (wasActive == 0) {
        g_Hud_LowMeterLoopSample->PlayA3DSimple(1.0f);
        g_Hud_LowMeterLoopActive = 1;
    }
}

/**
 * Reimplements 0x439b70: HudLowMeterLoopSound::Disable.
 * Original source: D:\Proj\Battlesport\Hud.cpp.
 * Purpose: Stops both low-meter warning samples and clears the loop-active flag.
 */
void Disable() {
    g_Hud_LowMeterBeepSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopActive = 0;
}

} // namespace HudLowMeterLoopSound

#include "Battlesport/recoil_state_credits_body.h"

namespace {
const char kHudTailGlobalContextSearchPath[] = ".;zbd";
const char kHudTailCommandNameWeaponSetMaxTetherAltitude[] =
    "WeaponSetMaxTetherAltitude";
} // namespace

/**
 * Reimplements 0x414a60: zInterp_GlobalContext::StaticInitAndRegisterAtExit.
 * Source path: D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp.
 *
 * Purpose: construct the process-wide interpreter and register its shutdown
 * callback during static initialization.
 */
int zInterp_GlobalContext::StaticInitAndRegisterAtExit() {
    StaticInit();
    return RegisterAtExit();
}

/**
 * Reimplements 0x414a70: zInterp_GlobalContext::StaticInit.
 * Source path: D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp.
 *
 * Purpose: static initializer wrapper for the process-wide interpreter.
 */
zInterp_Context *zInterp_GlobalContext::StaticInit() {
    return new (&g_zInterp_GlobalContext) zInterp_GlobalContext;
}

/**
 * Reimplements 0x414a80: zInterp_GlobalContext::RegisterAtExit.
 * Source path: D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp.
 *
 * Purpose: register the global interpreter destructor with the CRT atexit list.
 */
int zInterp_GlobalContext::RegisterAtExit() {
    return atexit(AtExitDestructor);
}

/**
 * Reimplements 0x414a90: zInterp_GlobalContext::AtExitDestructor.
 * Source path: D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp.
 *
 * Purpose: tear down the process-wide interpreter during CRT shutdown.
 */
void zInterp_GlobalContext::AtExitDestructor() {
    g_zInterp_GlobalContext.Destructor();
}

/**
 * Reimplements 0x414ab0: zInterp_GlobalContext::zInterp_GlobalContext.
 * Source path: D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp.
 *
 * Purpose: construct the process-wide interpreter with the retail search path
 * and prepared script index filename.
 */
zInterp_GlobalContext::zInterp_GlobalContext() {
    zInterp_Context::Constructor(
        kHudTailGlobalContextSearchPath,
        g_zInterp_PreparedIndexFileName
    );
}

/**
 * Reimplements 0x414ad0: zInterp_GlobalContext::DispatchHook.
 * Source path: D:\Proj\GameZRecoil\zInterp\zinterp_parse.cpp.
 *
 * Purpose: handle global WeaponSetMaxTetherAltitude commands before the
 * generic context dispatch path reports them as unhandled.
 */
int zInterp_GlobalContext::DispatchHook(
    char *commandToken
) {
    zInterp_Context *const context = this;
    if (commandToken[0] != 'W' ||
        context->tokenCount == 0 ||
        strcmp(
            context->tokenList[0],
            kHudTailCommandNameWeaponSetMaxTetherAltitude
        ) != 0) {
        return 1;
    }

    zWeapon::SetMaxTetherAltitude(context->ParseFloatToken());
    return 0;
}

/**
 * Reimplements 0x414b50: shared.authored_ret4_noop_414b50
 * (standalone; not a Westwood download event-sink owner member).
 * Purpose: Handles an unused download event callback slot with a zero result.
 */
int STDMETHODCALLTYPE WestwoodOnlineUpgradeDownloadEventSink::CallbackNoOp(
    void *
) {
    return 0;
}

#include "Battlesport/hud_ui_main_menu_dialog_body.h"
#include "Battlesport/recoil_state_dialog_host_body.h"
#include "Battlesport/recoil_state_main_menu_transition_body.h"
#define HUD_UI_MAIN_MENU_DIALOG_BODY_LOAD_BUTTON_ONLY
#include "Battlesport/hud_ui_main_menu_dialog_body.h"
#undef HUD_UI_MAIN_MENU_DIALOG_BODY_LOAD_BUTTON_ONLY
#define RECOIL_STATE_MAIN_MENU_TRANSITION_BODY_CTOR_DTOR_ONLY
#include "Battlesport/recoil_state_main_menu_transition_body.h"
#undef RECOIL_STATE_MAIN_MENU_TRANSITION_BODY_CTOR_DTOR_ONLY
#include "Battlesport/recoil_state_main_menu_transition_on_try_become_current_body.h"
#include "Battlesport/recoil_state_main_menu_transition_on_resume_body.h"
#include "Battlesport/recoil_state_main_menu_transition_on_deactivate_body.h"
#include "Battlesport/recoil_state_main_menu_transition_clear_paused_audio_snapshot_body.h"
#include "Battlesport/recoil_state_main_menu_transition_queue_enter_body.h"
#include "Battlesport/recoil_state_main_menu_transition_set_deferred_video_mode_index_body.h"

/**
 * Reimplements 0x415680: HudUiBackgroundConfirmQuit::Constructor.
 * Original source path: D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp.
 * Purpose: Construct the confirm-quit dialog, bind its OK/cancel buttons, and load its ZRD layout.
 */
HudUiBackgroundConfirmQuit * HudUiBackgroundConfirmQuit::Constructor() {
    new ((HudUiBackground *)this) HudUiBackground;
    new (&okButton) HudUiConfirmQuitOkButton;
    new (&cancelButton) HudUiConfirmQuitCancelButton;

    zReader::Node *const dialogRoot = HudUiBackground::LoadFromZrd(
        "dialog.zrd",
        g_HudUiBackgroundConfirmQuit_SectionName,
        0
    );
    if (dialogRoot != 0) {
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &okButton,
            g_HudUiBackgroundConfirmQuit_OkButtonNodeName
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &cancelButton,
            g_HudUiBackgroundConfirmQuit_CancelButtonNodeName
        );
        HudUiBackground::FreeLoadedTreeRoots((int)dialogRoot);
    }

    return this;
}

/**
 * Reimplements 0x415740: HudUiConfirmQuitOkButton::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: Queue the confirm-quit transition path and run inherited activation behavior.
 */
void HudUiConfirmQuitOkButton::OnActivate() {
    g_RecoilState_MainMenuSkipExitDelay = 1;
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueExitCurrentState(0);
    g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x415790: HudUiBackgroundConfirmQuit::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp.
 * Purpose: Run confirm-quit dialog cleanup and optionally free the object for VC5 scalar delete.
 */
HudUiBackground * HudUiBackgroundConfirmQuit::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

/**
 * Reimplements 0x4157b0: HudUiBackgroundConfirmQuit::Destructor.
 * Original source path: D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp.
 * Purpose: Destroy the confirm-quit child widgets before the inherited background cleanup.
 */
void HudUiBackgroundConfirmQuit::Destructor() {
    cancelButton.~HudUiConfirmQuitCancelButton();
    okButton.~HudUiConfirmQuitOkButton();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x415810: RecoilStateConfirmQuit::StaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for RecoilStateConfirmQuit::StaticInitAndRegisterAtExit.
 */
void RecoilStateConfirmQuit::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * Reimplements 0x415820: RecoilStateConfirmQuit::StaticInit.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for RecoilStateConfirmQuit::StaticInit.
 */
RecoilStateConfirmQuit *RecoilStateConfirmQuit::StaticInit() {
    return new (&g_RecoilState_ConfirmQuit) RecoilStateConfirmQuit;
}

/**
 * Reimplements 0x415830: RecoilStateConfirmQuit::RegisterAtExit.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for RecoilStateConfirmQuit::RegisterAtExit.
 */
void RecoilStateConfirmQuit::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * Reimplements 0x415840: RecoilStateConfirmQuit::AtExitDestructor.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: run the recovered RecoilStateConfirmQuit::AtExitDestructor teardown path.
 */
void RecoilStateConfirmQuit::AtExitDestructor() {
    g_RecoilState_ConfirmQuit.~RecoilStateConfirmQuit();
}

/**
 * Reimplements 0x415850: RecoilStateConfirmQuit::RecoilStateConfirmQuit.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: initialize the confirm-quit app state and clear its dialog pointer.
 */
RecoilStateConfirmQuit::RecoilStateConfirmQuit() {
    m_dialog = 0;
}

/**
 * Reimplements 0x415880: RecoilStateConfirmQuit::~RecoilStateConfirmQuit.
 * Original source path: D:\Proj\Battlesport\RecoilStateConfirmQuit.cpp.
 * Purpose: run the recovered RecoilStateConfirmQuit::~RecoilStateConfirmQuit teardown path.
 */
RecoilStateConfirmQuit::~RecoilStateConfirmQuit() {
    HudUiBackgroundConfirmQuit *dialog = (HudUiBackgroundConfirmQuit *)m_dialog;
    if (dialog != 0) {
        dialog->SetEnabled(0);

        dialog = (HudUiBackgroundConfirmQuit *)m_dialog;
        if (dialog != 0) {
            dialog->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }
}

/**
 * Reimplements 0x4158f0: RecoilStateConfirmQuit::OnTryBecomeCurrent.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: handle the recovered HUD event path for RecoilStateConfirmQuit::OnTryBecomeCurrent.
 */
int RecoilStateConfirmQuit::OnTryBecomeCurrent() {
    HudUiBackgroundConfirmQuit *dialog =
        (HudUiBackgroundConfirmQuit *) ::operator new(sizeof(HudUiBackgroundConfirmQuit));
    if (dialog != 0) {
        dialog = dialog->Constructor();
    }
    m_dialog = dialog;

    dialog->SetEnabled(1);

    return 1;
}

/**
 * Reimplements 0x415960: RecoilStateConfirmQuit::OnDeactivate.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: handle the recovered HUD event path for RecoilStateConfirmQuit::OnDeactivate.
 */
void RecoilStateConfirmQuit::OnDeactivate() {
    if (m_dialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    HudUiBackgroundConfirmQuit *dialog = (HudUiBackgroundConfirmQuit *)m_dialog;
    dialog->SetEnabled(0);

    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    dialog = (HudUiBackgroundConfirmQuit *)m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
    Sleep(1000);
}

/**
 * Reimplements 0x4159b0: RecoilStateConfirmQuit::QueueEnter.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: queue the recovered HUD application-state transition for RecoilStateConfirmQuit::QueueEnter.
 */
void RecoilStateConfirmQuit::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilState_ConfirmQuit,
        0
    );
}

/**
 * Reimplements 0x4159d0: zFMV_Action::Update.
 * Purpose: report immediate completion for action types without update behavior.
 */
int zFMV_Action::Update(
    double
) {
    return 0;
}

/**
 * Reimplements 0x4159e0: zFMV_Action::RunBlockingTimed.
 * Purpose: run an action to completion using elapsed milliseconds from GetTickCount.
 */
void zFMV_Action::RunBlockingTimed() {
    const double startSec = (double)(GetTickCount()) * 0.00100000005;
    Begin(0.0);
    while (true) {
        const double currentSec = ((double)(GetTickCount()) * 0.00100000005) - startSec;
        if (Update(currentSec) == 0) {
            break;
        }
    }
    End();
}

/**
 * Reimplements 0x415aa0: zFMV_Action::~zFMV_Action.
 * Purpose: provide the shared virtual action destructor.
 */
zFMV_Action::~zFMV_Action() {}
