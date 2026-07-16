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

/*
 * Ordinary virtual-destructor provenance retained for compiler-generated
 * lifecycle rows after removal of the synthetic named-slot ABI.
 * Reimplements 0x404d70: HudUiElement lifecycle contribution.
 * Reimplements 0x406e10: HudUiCheatCodeDialog lifecycle contribution.
 * Reimplements 0x408c40: HudUiControlsDialog lifecycle contribution.
 * Reimplements 0x408d70: RecoilStateControls lifecycle contribution.
 * Reimplements 0x4099d0: RecoilStateCredits lifecycle contribution.
 * Reimplements 0x40a920: HudCmdDialog lifecycle contribution.
 * Reimplements 0x40b0a0: HudCmdCommandList lifecycle contribution.
 * Reimplements 0x40b0c0: HudCmdKeyAButton lifecycle contribution.
 * Reimplements 0x40b0e0: HudCmdKeyBButton lifecycle contribution.
 * Reimplements 0x40b100: HudCmdJoyButton lifecycle contribution.
 * Reimplements 0x40b120: HudCmdMouseButton lifecycle contribution.
 * Reimplements 0x40bc70: HudCmdDialogState lifecycle contribution.
 * Reimplements 0x40bf50: HudCmdBindingEntry lifecycle contribution.
 * Reimplements 0x40c260: HudCmdBindButtonBase lifecycle contribution.
 * Reimplements 0x40cf00: HudOptionsDialog lifecycle contribution.
 * Reimplements 0x40d0c0: HudUiOptionsPanelOverlayOwner lifecycle contribution.
 * Reimplements 0x415020: HudUiMainMenuDialog lifecycle contribution.
 * Reimplements 0x415190: RecoilStateMainMenuTransition lifecycle contribution.
 * Reimplements 0x415790: HudUiBackgroundConfirmQuit lifecycle contribution.
 * Reimplements 0x415860: RecoilStateConfirmQuit lifecycle contribution.
 * Reimplements 0x415a80: zFMV_Action lifecycle contribution.
 */
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
#define g_HudUiOptionsPanelOverlayOwner \
    (*(HudUiOptionsPanelOverlayOwner *)&g_HudUiOptionsPanelOverlayOwner)
#define g_RecoilState_ConfirmQuit \
    (*(RecoilStateConfirmQuit *)&g_RecoilState_ConfirmQuit)
#define g_RecoilStateControls \
    (*(RecoilStateControls *)&g_RecoilStateControls)
#define g_RecoilStateCheatCode \
    (*(RecoilStateCheatCode *)&g_RecoilStateCheatCode)
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
enum zVideoRendererBackend {
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};
} // namespace

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
void __cdecl ReportOld(
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
 * Reimplements 0x406e10: VC5 class-specific deleting-destructor contribution
 * for the ordinary virtual HudUiCheatCodeDialog lifetime.
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
 * Reimplements 0x406ee0: RecoilStateCheatCode::compiler deleting destructor (compiler-emitted).
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: release any active cheat-code dialog and clear the app-state dialog pointer.
 */
RecoilStateCheatCode::~RecoilStateCheatCode() {
    HudUiCheatCodeDialog *dialog = (HudUiCheatCodeDialog *)m_dialog;
    if (dialog != 0) {
        delete dialog;
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
            delete ((HudUiCheatCodeDialog *)m_dialog);
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
 * Purpose: queue the cheat-code state exit when the GO widget is activated.
 */
inline void HudUiCheatCodeTitleWidget::OnActivate() {
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

extern void (*const g_HudUiQueueExitCurrentStateCallback)() = HudUiCallback::QueueExitCurrentState;

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
extern "C" {
/**
 * Reimplements data 0x4da63c..0x4da8b4: zOpt profile and option literal pool.
 * Purpose: preserve the writable VC5-era char globals used by profile selection
 * and option registration.
 */
/**
 * Reimplements data 0x4da63c: g_zOpt_OpStr_TolEq.
 * Purpose: Stores the writable profile comparison token for approximate equality.
 */
char g_zOpt_OpStr_TolEq[] = "~=";
/**
 * Reimplements data 0x4da640: g_zOpt_OpStr_Ne.
 * Purpose: Stores the writable profile comparison token for inequality.
 */
char g_zOpt_OpStr_Ne[] = "!=";
/**
 * Reimplements data 0x4da644: g_zOpt_OpStr_Ge.
 * Purpose: Stores the writable profile comparison token for greater-or-equal tests.
 */
char g_zOpt_OpStr_Ge[] = ">=";
/**
 * Reimplements data 0x4da648: g_zOpt_OpStr_Le.
 * Purpose: Stores the writable profile comparison token for less-or-equal tests.
 */
char g_zOpt_OpStr_Le[] = "<=";
/**
 * Reimplements data 0x4da64c: g_zOpt_OpStr_Gt.
 * Purpose: Stores the writable profile comparison token for greater-than tests.
 */
char g_zOpt_OpStr_Gt[] = ">";
/**
 * Reimplements data 0x4da650: g_zOpt_OpStr_Lt.
 * Purpose: Stores the writable profile comparison token for less-than tests.
 */
char g_zOpt_OpStr_Lt[] = "<";
/**
 * Reimplements data 0x4da654: g_zOpt_OpStr_Eq.
 * Purpose: Stores the writable profile comparison token for equality tests.
 */
char g_zOpt_OpStr_Eq[] = "==";
/**
 * Reimplements data 0x4da658: k_zOpt_ProfileMetricDefault.
 * Purpose: Stores the writable DEFAULT profile metric key.
 */
char k_zOpt_ProfileMetricDefault[] = "DEFAULT";
/**
 * Reimplements data 0x4da660: k_zOpt_ProfileMetricHwAccel.
 * Purpose: Stores the writable HW_ACCEL profile metric key.
 */
char k_zOpt_ProfileMetricHwAccel[] = "HW_ACCEL";
/**
 * Reimplements data 0x4da66c: k_zOpt_ProfileMetricRamKb.
 * Purpose: Stores the writable RAM_KB profile metric key.
 */
char k_zOpt_ProfileMetricRamKb[] = "RAM_KB";
/**
 * Reimplements data 0x4da674: k_zOpt_ProfileMetricVideoKb.
 * Purpose: Stores the writable VIDEO_KB profile metric key.
 */
char k_zOpt_ProfileMetricVideoKb[] = "VIDEO_KB";
/**
 * Reimplements data 0x4da680: k_zOpt_ProfileMetricCpuMhz.
 * Purpose: Stores the writable CPU_MHZ profile metric key.
 */
char k_zOpt_ProfileMetricCpuMhz[] = "CPU_MHZ";
/**
 * Reimplements data 0x4da688: k_zOpt_ProfileMetricCpuClass.
 * Purpose: Stores the writable CPU_CLASS profile metric key.
 */
char k_zOpt_ProfileMetricCpuClass[] = "CPU_CLASS";
/**
 * Reimplements data 0x4da694: g_zOpt_OptionName_VStride.
 * Purpose: Stores the writable option name used to register VStride.
 */
char g_zOpt_OptionName_VStride[] = "VStride";
/**
 * Reimplements data 0x4da69c: g_zOpt_OptionName_VMode.
 * Purpose: Stores the writable option name used to register VMode.
 */
char g_zOpt_OptionName_VMode[] = "VMode";
/**
 * Reimplements data 0x4da6a4: g_zOpt_OptionName_Replicate.
 * Purpose: Stores the writable option name used to register Replicate.
 */
char g_zOpt_OptionName_Replicate[] = "Replicate";
/**
 * Reimplements data 0x4da6b0: g_zOpt_OptionName_Window.
 * Purpose: Stores the writable option name used to register Window.
 */
char g_zOpt_OptionName_Window[] = "Window";
/**
 * Reimplements data 0x4da6b8: g_zOpt_OptionName_Display.
 * Purpose: Stores the writable option name used to register Display.
 */
char g_zOpt_OptionName_Display[] = "Display";
/**
 * Reimplements data 0x4da6c0: g_zOpt_OptionName_Render.
 * Purpose: Stores the writable option name used to register Render.
 */
char g_zOpt_OptionName_Render[] = "Render";
/**
 * Reimplements data 0x4da6c8: g_zOpt_OptionName_Camera.
 * Purpose: Stores the writable option name used to register Camera.
 */
char g_zOpt_OptionName_Camera[] = "Camera";
/**
 * Reimplements data 0x4da6d0: g_zOpt_OptionName_NetListen.
 * Purpose: Stores the writable option name used to register NetListen.
 */
char g_zOpt_OptionName_NetListen[] = "NetListen";
/**
 * Reimplements data 0x4da6dc: g_zOpt_OptionName_NetworkModem.
 * Purpose: Stores the writable option name used to register NetworkModem.
 */
char g_zOpt_OptionName_NetworkModem[] = "NetworkModem";
/**
 * Reimplements data 0x4da6ec: g_zOpt_OptionName_Network.
 * Purpose: Stores the writable option name used to register Network.
 */
char g_zOpt_OptionName_Network[] = "Network";
/**
 * Reimplements data 0x4da6f4: g_zOpt_OptionName_JoystickNumButtons.
 * Purpose: Stores the writable option name used to register JoystickNumButtons.
 */
char g_zOpt_OptionName_JoystickNumButtons[] = "JoystickNumButtons";
/**
 * Reimplements data 0x4da708: g_zOpt_OptionName_JoystickNumAxes.
 * Purpose: Stores the writable option name used to register JoystickNumAxes.
 */
char g_zOpt_OptionName_JoystickNumAxes[] = "JoystickNumAxes";
/**
 * Reimplements data 0x4da718: g_zOpt_OptionName_WOLPasswordFlag.
 * Purpose: Stores the writable option name used to register WOLPasswordFlag.
 */
char g_zOpt_OptionName_WOLPasswordFlag[] = "WOLPasswordFlag";
/**
 * Reimplements data 0x4da728: g_zOpt_OptionName_Joystick.
 * Purpose: Stores the writable option name used to register Joystick.
 */
char g_zOpt_OptionName_Joystick[] = "Joystick";
/**
 * Reimplements data 0x4da734: g_zOpt_OptionName_HwApi.
 * Purpose: Stores the writable option name used to register HWAPI.
 */
char g_zOpt_OptionName_HwApi[] = "HWAPI";
/**
 * Reimplements data 0x4da73c: g_zOpt_OptionName_HudTypeHw.
 * Purpose: Stores the writable option name used to register HUDType_HW.
 */
char g_zOpt_OptionName_HudTypeHw[] = "HUDType_HW";
/**
 * Reimplements data 0x4da748: g_zOpt_OptionName_HudTypeSw.
 * Purpose: Stores the writable option name used to register HUDType_SW.
 */
char g_zOpt_OptionName_HudTypeSw[] = "HUDType_SW";
/**
 * Reimplements data 0x4da754: g_zOpt_OptionName_HudFlagHw.
 * Purpose: Stores the writable option name used to register HUDFlag_HW.
 */
char g_zOpt_OptionName_HudFlagHw[] = "HUDFlag_HW";
/**
 * Reimplements data 0x4da760: g_zOpt_OptionName_HudFlagSw.
 * Purpose: Stores the writable option name used to register HUDFlag_SW.
 */
char g_zOpt_OptionName_HudFlagSw[] = "HUDFlag_SW";
/**
 * Reimplements data 0x4da76c: g_zOpt_OptionName_FullScreen.
 * Purpose: Stores the writable option name used to register FullScreen.
 */
char g_zOpt_OptionName_FullScreen[] = "FullScreen";
/**
 * Reimplements data 0x4da778: g_zOpt_OptionName_CDAudio.
 * Purpose: Stores the writable option name used to register CDAudio.
 */
char g_zOpt_OptionName_CDAudio[] = "CDAudio";
/**
 * Reimplements data 0x4da780: g_zOpt_OptionName_PlayerName.
 * Purpose: Stores the writable option name used to register PlayerName.
 */
char g_zOpt_OptionName_PlayerName[] = "PlayerName";
/**
 * Reimplements data 0x4da78c: g_zOpt_OptionName_SoundApi.
 * Purpose: Stores the writable option name used to register SoundAPI.
 */
char g_zOpt_OptionName_SoundApi[] = "SoundAPI";
/**
 * Reimplements data 0x4da798: g_zOpt_OptionName_SoundLOD.
 * Purpose: Stores the writable option name used to register SoundLOD.
 */
char g_zOpt_OptionName_SoundLOD[] = "SoundLOD";
/**
 * Reimplements data 0x4da7a4: g_zOpt_OptionName_SoundVolume.
 * Purpose: Stores the writable option name used to register SoundVolume.
 */
char g_zOpt_OptionName_SoundVolume[] = "SoundVolume";
/**
 * Reimplements data 0x4da7b0: g_zOpt_OptionName_MuteSound.
 * Purpose: Stores the writable option name used to register MuteSound.
 */
char g_zOpt_OptionName_MuteSound[] = "MuteSound";
/**
 * Reimplements data 0x4da7bc: g_zOpt_OptionName_GameIntensity.
 * Purpose: Stores the writable option name used to register GameIntensity.
 */
char g_zOpt_OptionName_GameIntensity[] = "GameIntensity";
/**
 * Reimplements data 0x4da7cc: g_zOpt_OptionName_GameCtlOptions.
 * Purpose: Stores the writable option name used to register GameCtlOptions.
 */
char g_zOpt_OptionName_GameCtlOptions[] = "GameCtlOptions";
/**
 * Reimplements data 0x4da7dc: g_zOpt_OptionName_TextureMemoryHw.
 * Purpose: Stores the writable option name used to register TextureMemory_HW.
 */
char g_zOpt_OptionName_TextureMemoryHw[] = "TextureMemory_HW";
/**
 * Reimplements data 0x4da7f0: g_zOpt_OptionName_TextureMemorySw.
 * Purpose: Stores the writable option name used to register TextureMemory_SW.
 */
char g_zOpt_OptionName_TextureMemorySw[] = "TextureMemory_SW";
/**
 * Reimplements data 0x4da804: g_zOpt_OptionName_ObjectLODHw.
 * Purpose: Stores the writable option name used to register ObjectLOD_HW.
 */
char g_zOpt_OptionName_ObjectLODHw[] = "ObjectLOD_HW";
/**
 * Reimplements data 0x4da814: g_zOpt_OptionName_ObjectLODSw.
 * Purpose: Stores the writable option name used to register ObjectLOD_SW.
 */
char g_zOpt_OptionName_ObjectLODSw[] = "ObjectLOD_SW";
/**
 * Reimplements data 0x4da824: g_zOpt_OptionName_GlobalLightHw.
 * Purpose: Stores the writable option name used to register GlobalLight_HW.
 */
char g_zOpt_OptionName_GlobalLightHw[] = "GlobalLight_HW";
/**
 * Reimplements data 0x4da834: g_zOpt_OptionName_GfxFlagsHw.
 * Purpose: Stores the writable option name used to register GfxFlags_HW.
 */
char g_zOpt_OptionName_GfxFlagsHw[] = "GfxFlags_HW";
/**
 * Reimplements data 0x4da840: g_zOpt_OptionName_AllVideoBuffer.
 * Purpose: Stores the writable option name used to register AllVideoBuffer.
 */
char g_zOpt_OptionName_AllVideoBuffer[] = "AllVideoBuffer";
/**
 * Reimplements data 0x4da850: g_zOpt_OptionName_GlobalLightSw.
 * Purpose: Stores the writable option name used to register GlobalLight_SW.
 */
char g_zOpt_OptionName_GlobalLightSw[] = "GlobalLight_SW";
/**
 * Reimplements data 0x4da860: g_zOpt_OptionName_Perspective.
 * Purpose: Stores the writable option name used to register Perspective.
 */
char g_zOpt_OptionName_Perspective[] = "Perspective";
/**
 * Reimplements data 0x4da86c: g_zOpt_OptionName_Lighting.
 * Purpose: Stores the writable option name used to register Lighting.
 */
char g_zOpt_OptionName_Lighting[] = "Lighting";
/**
 * Reimplements data 0x4da878: g_zOpt_OptionName_Transparency.
 * Purpose: Stores the writable option name used to register Transparency.
 */
char g_zOpt_OptionName_Transparency[] = "Transparency";
/**
 * Reimplements data 0x4da888: g_zOpt_OptionName_GfxFlagsSw.
 * Purpose: Stores the writable option name used to register GfxFlags_SW.
 */
char g_zOpt_OptionName_GfxFlagsSw[] = "GfxFlags_SW";
/**
 * Reimplements data 0x4da894: g_zOpt_OptionName_EffectsLevelHw.
 * Purpose: Stores the writable option name used to register EffectsLevel_HW.
 */
char g_zOpt_OptionName_EffectsLevelHw[] = "EffectsLevel_HW";
/**
 * Reimplements data 0x4da8a4: g_zOpt_OptionName_EffectsLevelSw.
 * Purpose: Stores the writable option name used to register EffectsLevel_SW.
 */
char g_zOpt_OptionName_EffectsLevelSw[] = "EffectsLevel_SW";
/**
 * Reimplements data 0x4da8b4: g_zOpt_OptionName_HwCardFlag.
 * Purpose: Stores the writable option name used to register HWCardFlag.
 */
char g_zOpt_OptionName_HwCardFlag[] = "HWCardFlag";
/**
 * Reimplements data 0x4da8c0: g_zOpt_DetailArchiveName.
 * Purpose: Stores the writable detail archive name used by game option loading.
 */
char g_zOpt_DetailArchiveName[] = "detail.zrd";
/**
 * Reimplements data 0x4da8cc: g_zOpt_DetailOptionName_Sunlight.
 * Purpose: Stores the writable node name used to apply the sunlight graphics flag.
 */
char g_zOpt_DetailOptionName_Sunlight[] = "sunlight";
}

namespace zOpt {
namespace {
struct zOpt_NameInt32Pair {
    const char *name;
    int value;
};

const zOpt_NameInt32Pair g_zOpt_NamedScalarValues[] = {
    {"TRUE", 1},
    {"FALSE", 0},
    {"HIGH", 0},
    {"MEDIUM", 1},
    {"LOW", 2},
    {"CPU_CLASS_8086", 0},
    {"CPU_CLASS_80286", 2},
    {"CPU_CLASS_80386", 3},
    {"CPU_CLASS_80486", 4},
    {"CPU_CLASS_PENTIUM", 5},
    {"CPU_CLASS_PENTIUM_PRO", 6},
    {"CPU_CLASS_PENTIUM_NEWER", 7},
    {"TEXMEM_MAX", 0},
    {"TEXMEM_8MB", 1},
    {"TEXMEM_6MB", 2},
    {"TEXMEM_4MB", 3},
    {"TEXMEM_2MB", 4},
    {"ZVID_320x200x16", 2},
    {"ZVID_320x240x16", 3},
    {"ZVID_640x400x16", 4},
    {"ZVID_640x480x16", 5},
    {"ZVID_800x600x16", 6},
    {"ZVID_1024x768x16", 7},
    {"HUD_TYPEI", 1},
    {"HUD_TYPEII", 2},
    {"SOUND_API_DSOUND", 0},
    {"SOUND_API_A3D", 1},
};

const double ZOPT_COMPARE_TOLERANCE_PCT = 0.02;

/**
 * Original inline/static helper; no standalone retail function exists. Observed in caller
 * 0x407220.
 * Evidence basis: the branchless signed absolute-difference idiom is embedded in the
 * "~=" comparison path of zOpt::EvalIntCompareOp, with no assigned address-backed retail
 * helper for this source-file owner.
 * Purpose: compute the absolute difference used by the profile metric tolerance compare.
 */
int WrappedAbsDifference(
    int lhs,
    int rhs
) {
    const unsigned int diff = (unsigned int)(lhs) - (unsigned int)(rhs);
    const unsigned int signMask = 0u - (diff >> 31);
    return (int)((diff ^ signMask) - signMask);
}

} // namespace

/**
 * Reimplements 0x407190: zOpt::LookupNamedValueAsInt.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: map profile scalar names to their integer option values.
 */
int __fastcall LookupNamedValueAsInt(
    const char *key
) {
    unsigned int pairIndex;
    for (pairIndex = 0;
        pairIndex < sizeof(g_zOpt_NamedScalarValues) / sizeof(g_zOpt_NamedScalarValues[0]);
        ++pairIndex) {
        if (strcmp(
            g_zOpt_NamedScalarValues[pairIndex].name,
            key
        ) == 0) {
            return g_zOpt_NamedScalarValues[pairIndex].value;
        }
    }

    return 0;
}

/**
 * Reimplements 0x4071f0: zOpt::ReadScalarValueAsInt.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: coerce an integer, float, or named string scalar node into an integer value.
 */
int __fastcall ReadScalarValueAsInt(
    zReader::Node *scalarValueNode
) {
    if (scalarValueNode->type == zReader::ZRDR_NODE_INT) {
        return scalarValueNode->value.i32;
    }
    if (scalarValueNode->type == zReader::ZRDR_NODE_FLOAT) {
        return (int)(scalarValueNode->value.f32);
    }
    if (scalarValueNode->type == zReader::ZRDR_NODE_STRING) {
        return LookupNamedValueAsInt(scalarValueNode->value.str);
    }

    return 0;
}

/**
 * Reimplements 0x407220: zOpt::EvalIntCompareOp.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: apply an integer comparison operator used by profile metric rules.
 */
int __fastcall EvalIntCompareOp(
    const char *opString,
    int lhs,
    int rhs
) {
    if (strcmp(
        opString,
        g_zOpt_OpStr_Eq
    ) == 0) {
        return lhs == rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Lt
    ) == 0) {
        return lhs < rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Gt
    ) == 0) {
        return lhs > rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Le
    ) == 0) {
        return lhs <= rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Ge
    ) == 0) {
        return lhs >= rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_Ne
    ) == 0) {
        return lhs != rhs;
    }
    if (strcmp(
        opString,
        g_zOpt_OpStr_TolEq
    ) == 0) {
        return (double)(WrappedAbsDifference(
            lhs,
            rhs
        )) < (double)(lhs)*ZOPT_COMPARE_TOLERANCE_PCT;
    }

    return 0;
}

/**
 * Reimplements 0x407470: zOpt::EvaluateProfileMetricCondition.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: evaluate one profile-selection condition against the current runtime metrics.
 */
int __fastcall EvaluateProfileMetricCondition(
    zReader::Node *metricConditionNode
) {
    if (metricConditionNode->type == zReader::ZRDR_NODE_STRING) {
        return strcmp(
            metricConditionNode->value.str,
            k_zOpt_ProfileMetricDefault
        ) == 0;
    }

    if (metricConditionNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const conditionArray = metricConditionNode->value.nodes;
    if (conditionArray[0].value.i32 != 4) {
        return 0;
    }

    const char *const metricKey = conditionArray[1].value.str;
    const char *const opString = conditionArray[2].value.str;
    const int rhs = ReadScalarValueAsInt(&conditionArray[3]);
    int currentMetricValue = 0;

    if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricCpuClass
    ) == 0) {
        currentMetricValue = g_zGame_Options_RuntimeConfig.cpuClass;
    } else if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricCpuMhz
    ) == 0) {
        currentMetricValue = g_zGame_Options_RuntimeConfig.cpuMhz;
    } else if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricVideoKb
    ) == 0) {
        currentMetricValue = (int)(g_zGame_Options_RuntimeConfig.soundHardwareMemKb);
    } else if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricRamKb
    ) == 0) {
        currentMetricValue = (int)(g_zGame_Options_RuntimeConfig.systemRamKb);
    } else if (strcmp(
        metricKey,
        k_zOpt_ProfileMetricHwAccel
    ) == 0) {
        currentMetricValue = (int)((g_zGame_Options_RuntimeConfig.defaultFlags >> 6) & 1u);
    } else {
        return 0;
    }

    return EvalIntCompareOp(
        opString,
        currentMetricValue,
        rhs
    );
}

/**
 * Reimplements 0x407680: zOpt::SelectProfileValueForSystem.
 * Original source path: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: choose the first matching profile rule value for the current system metrics.
 */
int __fastcall SelectProfileValueForSystem(
    zReader::Node *parentNode,
    const char *profileName,
    int defaultValue
) {
    if (parentNode == 0) {
        return defaultValue;
    }

    zReader::Node *const profileRuleListNode = zReader_GetNamedNode(
        parentNode,
        profileName
    );
    if (profileRuleListNode == 0) {
        return defaultValue;
    }

    zReader::Node *const ruleList = profileRuleListNode->value.nodes;
    const int count = ruleList[0].value.i32;
    {
        for (int ruleIndex = 1; ruleIndex < count; ++ruleIndex) {
            zReader::Node *const ruleCells = ruleList[ruleIndex].value.nodes;
            if (EvaluateProfileMetricCondition(&ruleCells[1]) != 0) {
                return ReadScalarValueAsInt(&ruleCells[2]);
            }
        }
    }

    return defaultValue;
}

} // namespace zOpt
namespace zGame {

namespace {
const int ZGAME_OPTION_INLINE_DWORD = 0;
const int ZGAME_OPTION_INLINE_BINARY4 = 1;
const int ZGAME_OPTION_STRING_BUFFER = 3;
const int ZGAME_OPTION_HEAP_BUFFER = 5;
const int ZGAME_OPTION_SCOPE_USER = 1;
const int ZGAME_OPTION_SCOPE_TRANSIENT = 2;
const int ZVID_HW_MODE_SOFTWARE = 0;
const int ZVID_HW_MODE_HARDWARE = 1;
const zOptGameControlFlags ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON = 0x08;
const int ZOPT_GRAPHICS_MMX = 1;
const int ZOPT_GRAPHICS_TRANSPARENCY = 2;
const int ZOPT_GRAPHICS_LIGHTING = 4;
const int ZOPT_GRAPHICS_PERSPECTIVE = 8;
const int ZOPT_GRAPHICS_GLOBAL_LIGHT = 0x10;
const int ZOPT_GRAPHICS_ALL_VIDEO_BUFFER = 0x20;

template <typename T>
/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x407700 from repeated option-entry pointer casts in option loading.
 * Purpose: return an option entry as the typed option-value pointer stored by zOpt globals.
 */
T *OptionValuePointer(
    zOptionEntryPartial *entry
) {
    return (T *)(entry);
}

/**
 * Restores likely original static helper; no standalone retail function exists.
 * Observed in caller 0x407700 from repeated profile metric selection for graphics flags.
 * Purpose: build the graphics option bitmask selected for the active profile.
 */
int BuildGraphicsFlags(
    zReader::Node *profileRoot,
    const char *globalLightKey,
    int globalLightDefault
) {
    int flags = 0;
    if ((g_zGame_Options_RuntimeConfig.defaultFlags & 1u) != 0) {
        flags |= ZOPT_GRAPHICS_MMX;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        g_zOpt_OptionName_Transparency,
        1
    ) != 0) {
        flags |= ZOPT_GRAPHICS_TRANSPARENCY;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        g_zOpt_OptionName_Lighting,
        1
    ) != 0) {
        flags |= ZOPT_GRAPHICS_LIGHTING;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        g_zOpt_OptionName_Perspective,
        1
    ) != 0) {
        flags |= ZOPT_GRAPHICS_PERSPECTIVE;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        globalLightKey,
        globalLightDefault
    ) != 0) {
        flags |= ZOPT_GRAPHICS_GLOBAL_LIGHT;
    }
    if (zOpt::SelectProfileValueForSystem(
        profileRoot,
        g_zOpt_OptionName_AllVideoBuffer,
        0
    ) != 0) {
        flags |= ZOPT_GRAPHICS_ALL_VIDEO_BUFFER;
    }

    return flags;
}

/**
 * Original inline helper; no standalone retail function exists. Observed in caller 0x407700.
 * Purpose: clear all cached option value pointers before rebuilding the option list.
 */
void ResetOptionPointers() {
    ZOPT_VIDEO_ACCELERATION = 0;
    ZOPT_VIDEO_MODE = 0;
    ZOPT_HW_API = 0;
    ZOPT_VIDEO_FULLSCREEN = 0;
    ZOPT_VIDEO_STRIDE = 0;
    ZOPT_HUD_SW = 0;
    ZOPT_HUD_HW = 0;
    ZOPT_HUD_TYPE_SW = 0;
    ZOPT_HUD_TYPE_HW = 0;
    ZOPT_REPLICATE = 0;
    ZOPT_NETWORK_ENABLED = 0;
    g_zOpt_NetworkModemOption = 0;
    g_zOpt_NetworkListenOption = 0;
    g_zOpt_GameDifficultyOption = 0;
    g_zOpt_WolPasswordFlagOption = 0;
    ZOPT_EFFECTS_LEVEL_SW = 0;
    ZOPT_EFFECTS_LEVEL_HW = 0;
    ZOPT_OBJECT_LOD_SW = 0;
    ZOPT_OBJECT_LOD_HW = 0;
    ZOPT_MUTE_SOUND = 0;
    ZOPT_SOUND_VOLUME = 0;
    ZOPT_SOUND_LOD = 0;
    ZOPT_TEXTURE_MEMORY_SW = 0;
    ZOPT_TEXTURE_MEMORY_HW = 0;
    ZOPT_PLAYER_NAME = 0;
    ZOPT_GFX_FLAGS_SW = 0;
    ZOPT_GFX_FLAGS_HW = 0;
    g_zOpt_RenderSectionOption = 0;
    g_zOpt_DisplaySectionOption = 0;
    g_zOpt_WindowSectionOption = 0;
    g_zOpt_CameraSectionOption = 0;
    ZOPT_GAME_CONTROL_OPTIONS = 0;
    ZOPT_INPUT_JOYSTICK = 0;
    ZOPT_JOYSTICK_NUM_AXES = 0;
    ZOPT_JOYSTICK_NUM_BUTTONS = 0;
    ZOPT_AUDIO_API = 0;
    ZOPT_SOUND_CDAUDIO = 0;
}
} // namespace

/**
 * Reimplements 0x4076f0: zGame::ReturnOnlyStub.
 * Purpose: preserve the empty zGame stub used by the option/load cluster.
 */
void ReturnOnlyStub() {}

/**
 * Reimplements 0x407700: zGame::Options_LoadGameOptions.
 * Purpose: load detail.zrd and register the game option globals.
 */
RECOIL_NO_GS int Options_LoadGameOptions() {
    ResetOptionPointers();

    zReader::Node *const detailRoot = zReader::LoadNodeFromPath(
        g_zOpt_DetailArchiveName,
        0,
        0
    );
    if (detailRoot == 0) {
        return 0;
    }

    g_zGame_Options_RuntimeConfig.CopyDefault();

    ZOPT_VIDEO_ACCELERATION = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HwCardFlag,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_VIDEO_ACCELERATION != 0) {
        zVid::SetAccelerationOption(ZVID_HW_MODE_HARDWARE);
    }

    ZOPT_EFFECTS_LEVEL_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_EffectsLevelSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_EFFECTS_LEVEL_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetEffectsLevelForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_EffectsLevelSw, 1)
        );
    }

    ZOPT_EFFECTS_LEVEL_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_EffectsLevelHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_EFFECTS_LEVEL_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetEffectsLevelForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_EffectsLevelHw, 0)
        );
    }

    ZOPT_GFX_FLAGS_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GfxFlagsSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_GFX_FLAGS_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetGraphicsFlagsForCurrentHwMode(BuildGraphicsFlags(
            detailRoot,
            g_zOpt_OptionName_GlobalLightSw,
            0
        ));
    }

    ZOPT_GFX_FLAGS_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GfxFlagsHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_GFX_FLAGS_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetGraphicsFlagsForCurrentHwMode(BuildGraphicsFlags(
            detailRoot,
            g_zOpt_OptionName_GlobalLightHw,
            1
        ));
    }

    ZOPT_OBJECT_LOD_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_ObjectLODSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_OBJECT_LOD_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetObjectLODForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_ObjectLODSw, 0)
        );
    }

    ZOPT_OBJECT_LOD_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_ObjectLODHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_OBJECT_LOD_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetObjectLODForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_ObjectLODHw, 0)
        );
    }

    ZOPT_TEXTURE_MEMORY_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_TextureMemorySw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_TEXTURE_MEMORY_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetTextureMemoryForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_TextureMemorySw, 0)
        );
    }

    ZOPT_TEXTURE_MEMORY_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_TextureMemoryHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_TEXTURE_MEMORY_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetTextureMemoryForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_TextureMemoryHw, 0)
        );
    }

    ZOPT_GAME_CONTROL_OPTIONS = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GameCtlOptions,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_GAME_CONTROL_OPTIONS != 0) {
        zOpt::SetGameControlOptions(ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON);
    }

    g_zOpt_GameDifficultyOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GameIntensity,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zOpt_GameDifficultyOption != 0) {
        zOpt::SetGameDifficultyMode(1);
    }

    ZOPT_MUTE_SOUND = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_MuteSound,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_MUTE_SOUND != 0) {
        zOpt::SetMuteSoundOption(0);
    }

    ZOPT_SOUND_VOLUME = OptionValuePointer<float>(Options_GetOrCreateOption(
        g_zOpt_OptionName_SoundVolume,
        ZGAME_OPTION_INLINE_BINARY4,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_SOUND_VOLUME != 0) {
        zOpt::SetSoundVolumeOption(1.0f);
    }

    ZOPT_SOUND_LOD = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_SoundLOD, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_SOUND_LOD != 0) {
        zOpt::SetSoundLODOption(zOpt::SelectProfileValueForSystem(
            detailRoot,
            g_zOpt_OptionName_SoundLOD,
            0
        ));
    }

    ZOPT_AUDIO_API = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_SoundApi, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_AUDIO_API != 0) {
        zSnd::SetAudioApiOption(1);
    }

    ZOPT_PLAYER_NAME = Options_GetOrCreateOption(
        g_zOpt_OptionName_PlayerName,
        ZGAME_OPTION_STRING_BUFFER,
        0x16,
        ZGAME_OPTION_SCOPE_USER
    );
    if (ZOPT_PLAYER_NAME != 0) {
        DWORD userNameSize = 0xfe;
        char userName[0x100];
        GetUserNameA(
            userName,
            &userNameSize
        );
        userName[userNameSize] = '\0';
        zOpt::SetPlayerName(userName);
    }

    ZOPT_SOUND_CDAUDIO = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_CDAudio, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_SOUND_CDAUDIO != 0) {
        zSnd::SetCDAudioOption(1);
    }

    ZOPT_VIDEO_FULLSCREEN = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_FullScreen,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_VIDEO_FULLSCREEN != 0) {
        zOpt::SetFullscreenOption(1);
    }

    ZOPT_HUD_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudFlagSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_HUD_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetHudVisibilityOption(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudFlagSw, 1)
        );
    }

    ZOPT_HUD_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudFlagHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_HUD_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetHudVisibilityOption(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudFlagHw, 1)
        );
    }

    ZOPT_HUD_TYPE_SW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudTypeSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_HUD_TYPE_SW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetHudTypeForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudTypeSw, 1)
        );
    }

    ZOPT_HUD_TYPE_HW = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudTypeHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (ZOPT_HUD_TYPE_HW != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetHudTypeForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudTypeHw, 1)
        );
    }

    ZOPT_HW_API = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_HwApi, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_HW_API != 0) {
        zVid::SetHwApiOption(1);
    }

    ZOPT_INPUT_JOYSTICK = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_Joystick, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_INPUT_JOYSTICK != 0) {
        zInp::SetJoystickOption(0);
    }

    g_zOpt_WolPasswordFlagOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_WOLPasswordFlag,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zOpt_WolPasswordFlagOption != 0) {
        zOpt::SetWolPasswordFlag(1);
    }

    ZOPT_JOYSTICK_NUM_AXES = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_JoystickNumAxes,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (ZOPT_JOYSTICK_NUM_AXES != 0) {
        zInp::SetJoystickAxesCountOption(0);
    }

    ZOPT_JOYSTICK_NUM_BUTTONS = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_JoystickNumButtons,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (ZOPT_JOYSTICK_NUM_BUTTONS != 0) {
        zInp::SetJoystickButtonCountOption(0);
    }

    ZOPT_NETWORK_ENABLED = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Network,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (ZOPT_NETWORK_ENABLED != 0) {
        zOpt::SetNetworkEnabled(0);
    }

    g_zOpt_NetworkModemOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_NetworkModem,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zOpt_NetworkModemOption != 0) {
        zOpt::SetNetworkModemEnabled(0);
    }

    g_zOpt_NetworkListenOption = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_NetListen,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zOpt_NetworkListenOption != 0) {
        zOpt::SetNetworkListenEnabled(0);
    }

    g_zOpt_CameraSectionOption = OptionValuePointer<zOpt_CameraSection *>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Camera,
        ZGAME_OPTION_HEAP_BUFFER,
        0x0c,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    g_zOpt_RenderSectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Render,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    g_zOpt_DisplaySectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Display,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    g_zOpt_WindowSectionOption =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Window,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    ZOPT_REPLICATE = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Replicate,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));

    ZOPT_VIDEO_MODE = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_VMode, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (ZOPT_VIDEO_MODE != 0) {
        zVid::SetVideoModeIndex(zOpt::SelectProfileValueForSystem(
            detailRoot,
            g_zOpt_OptionName_VMode,
            5
        ));
    }

    ZOPT_VIDEO_STRIDE = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_VStride,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (ZOPT_VIDEO_STRIDE != 0) {
        *ZOPT_VIDEO_STRIDE = 1;
    }

    zInput::BindMap_InitDefaultBindings();
    Options_LoadFromRegistry();
    zInput::BindMap_Current_RebuildLookupIndices();
    zOpt::SetNetworkEnabled(0);
    zOpt::SetNetworkModemEnabled(0);

    if (g_zOpt_CameraSectionOption != 0 && *g_zOpt_CameraSectionOption != 0) {
        (*g_zOpt_CameraSectionOption)->m_pCamera = 0;
    }
    if (g_zOpt_RenderSectionOption != 0 && *g_zOpt_RenderSectionOption != 0) {
        (*g_zOpt_RenderSectionOption)->target = 0;
    }
    if (g_zOpt_DisplaySectionOption != 0 && *g_zOpt_DisplaySectionOption != 0) {
        (*g_zOpt_DisplaySectionOption)->target = 0;
    }
    if (g_zOpt_WindowSectionOption != 0 && *g_zOpt_WindowSectionOption != 0) {
        (*g_zOpt_WindowSectionOption)->target = 0;
    }

    zReader::FreeLoadedTree(detailRoot);
    g_zOpt_HwMode = zVid::GetAccelerationOption();
    zSnd::SetAudioApiOption(zSnd::GetAudioApiOption());
    return 1;
}

/**
 * Reimplements 0x407e00: zGame::Options_SaveGameOptions.
 * Purpose: clear transient input/network state before saving the option registry.
 */
int Options_SaveGameOptions() {
    zInput::BindGroupList_Clear();
    zOpt::SetNetworkEnabled(0);
    zOpt::SetNetworkModemEnabled(0);
    return Options_SaveToRegistry();
}

} // namespace zGame
namespace zOpt {

const zOptGameControlFlags ZOPT_GAME_CONTROL_THROTTLE = 0x01;
const zOptGameControlFlags ZOPT_GAME_CONTROL_STEERING = 0x02;
const zOptGameControlFlags ZOPT_GAME_CONTROL_CURSOR = 0x04;
const zOptGameControlFlags ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON = 0x08;

/**
 * Reimplements 0x407e20: zOpt::SetGameControlOptions.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: replace the packed game-control option bitmask.
 */
void __fastcall SetGameControlOptions(
    zOptGameControlFlags value
) {
    *ZOPT_GAME_CONTROL_OPTIONS = value;
}

/**
 * Reimplements 0x407e30: zOpt::SetThrottleMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the throttle-control bit in the game-control option mask.
 */
void __fastcall SetThrottleMode(
    int enable
) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_THROTTLE;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_THROTTLE;
    }
}

/**
 * Reimplements 0x407e50: zOpt::GetThrottleMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the throttle-control bit from the game-control option mask.
 */
int GetThrottleMode() {
    return *ZOPT_GAME_CONTROL_OPTIONS & ZOPT_GAME_CONTROL_THROTTLE;
}

/**
 * Reimplements 0x407e60: zOpt::SetSteeringMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the steering-control bit in the game-control option mask.
 */
void __fastcall SetSteeringMode(
    int enable
) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_STEERING;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_STEERING;
    }
}

/**
 * Reimplements 0x407e80: zOpt::GetSteeringMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the steering-control bit from the game-control option mask.
 */
int GetSteeringMode() {
    return (*ZOPT_GAME_CONTROL_OPTIONS >> 1) & 1;
}

/**
 * Reimplements 0x407e90: zOpt::SetCursorMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the cursor-control bit in the game-control option mask.
 */
void __fastcall SetCursorMode(
    int enable
) {
    if (enable != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_CURSOR;
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_CURSOR;
    }
}

/**
 * Reimplements 0x407eb0: zOpt::GetCursorMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the cursor-control bit from the game-control option mask.
 */
int GetCursorMode() {
    return (*ZOPT_GAME_CONTROL_OPTIONS >> 2) & 1;
}

/**
 * Reimplements 0x407ec0: zOpt::SetCameraMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: store first-person or third-person camera mode and apply the player camera state.
 */
void __fastcall SetCameraMode(
    int enableThirdPerson
) {
    if (enableThirdPerson != 0) {
        *ZOPT_GAME_CONTROL_OPTIONS |= ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON;
        Player::ApplyCameraState(1);
    } else {
        *ZOPT_GAME_CONTROL_OPTIONS &= ~ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON;
        Player::ApplyCameraState(3);
    }
}

/**
 * Reimplements 0x407ef0: zOpt::GetCameraModeAsPlayerCameraState.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: map the third-person camera option bit to the player camera state value.
 */
int GetCameraModePlayerState() {
    return ((~*ZOPT_GAME_CONTROL_OPTIONS & ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON) | 4) >> 2;
}

/**
 * Reimplements 0x407f10: zOpt::SetGameDifficultyMode.
 * Original source: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: Store the current game difficulty option value.
 */
void __fastcall SetGameDifficultyMode(
    int value
) {
    *g_zOpt_GameDifficultyOption = value;
}

/**
 * Reimplements 0x407f20: zOpt::GetGameDifficultyMode.
 * Original source: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: Return the current game difficulty option value.
 */
int GetGameDifficultyMode() {
    return *g_zOpt_GameDifficultyOption;
}

/**
 * Reimplements 0x407f30: zOpt::SetEffectsLevelForCurrentHwMode.
 * Purpose: store the active hardware-mode effects level and apply the matching conditional effect level.
 */
void __fastcall SetEffectsLevelForCurrentHwMode(
    int level
) {
    *(g_zOpt_HwMode != 0 ? ZOPT_EFFECTS_LEVEL_HW : ZOPT_EFFECTS_LEVEL_SW) = level;

    if (level == 0) {
        zEffect::SetConditionalEffectLevel(2);
    } else if (level == 1) {
        zEffect::SetConditionalEffectLevel(1);
    } else if (level == 2) {
        zEffect::SetConditionalEffectLevel(0);
    }
}

/**
 * Reimplements 0x407f80: zOpt::GetEffectsLevelForCurrentHwMode.
 * Purpose: return the effects level stored for the active hardware mode.
 */
int GetEffectsLevelForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_EFFECTS_LEVEL_HW : ZOPT_EFFECTS_LEVEL_SW);
}

/**
 * Reimplements 0x407fa0: zOpt::SetObjectLODForCurrentHwMode.
 * Purpose: store the object LOD value for the active hardware mode and apply its camera clip distance.
 */
void __fastcall SetObjectLODForCurrentHwMode(
    int level
) {
    zClass_NodePartial *const camera = zOpt_CameraSection_GetActiveCamera();
    *(g_zOpt_HwMode != 0 ? ZOPT_OBJECT_LOD_HW : ZOPT_OBJECT_LOD_SW) = level;

    if (camera == 0) {
        return;
    }

    float clipDistance = 1.0f;
    if (level == 1) {
        clipDistance = 0.75f;
    } else if (level == 2) {
        clipDistance = 0.5f;
    }

    zClass_Camera::gwCameraSetClipDistance(
        camera,
        clipDistance
    );
}

/**
 * Reimplements 0x408030: zOpt::GetObjectLODForCurrentHwMode.
 * Purpose: return the object LOD value for the active hardware mode.
 */
int GetObjectLODForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_OBJECT_LOD_HW : ZOPT_OBJECT_LOD_SW);
}

/**
 * Reimplements 0x408050: zOpt::SetMuteSoundOption.
 * Purpose: store the mute-sound option and apply it to active sound voices.
 */
void __fastcall SetMuteSoundOption(
    int value
) {
    *ZOPT_MUTE_SOUND = value;
    zSnd::ApplyMuteStateToActiveVoices(value);
}

/**
 * Reimplements 0x408060: zOpt::GetMuteSoundOption.
 * Purpose: return the current mute-sound option value.
 */
int GetMuteSoundOption() {
    return *ZOPT_MUTE_SOUND;
}

/**
 * Reimplements 0x408070: zOpt::SetSoundVolumeOption.
 * Purpose: store the sound-volume option and apply the global sound scale.
 */
void __fastcall SetSoundVolumeOption(
    float volume
) {
    *ZOPT_SOUND_VOLUME = volume;
    zSnd::SetGlobalVolumeScale(volume);
}

/**
 * Reimplements 0x408090: zOpt::GetSoundVolumeOption.
 * Purpose: return the current sound-volume option value.
 */
float GetSoundVolumeOption() {
    return *ZOPT_SOUND_VOLUME;
}

} // namespace zOpt
namespace zSnd {

/**
 * Reimplements 0x4080a0: zSnd::SetAudioApiOption.
 * Original source: D:\Proj\GameZRecoil\zSound\zsnd_cd.cpp.
 * Purpose: Store the selected audio backend option and mirror it into the pre-init backend state.
 */
int __fastcall SetAudioApiOption(
    int apiType
) {
    *ZOPT_AUDIO_API = apiType;
    return SetActiveBackendPreInit(apiType);
}

/**
 * Reimplements 0x4080b0: zSnd::GetAudioApiOption.
 * Original source: D:\Proj\GameZRecoil\zSound\zsnd_cd.cpp.
 * Purpose: Return the selected audio backend option value.
 */
int GetAudioApiOption() {
    return *ZOPT_AUDIO_API;
}

} // namespace zSnd
namespace zOpt {

/**
 * Reimplements 0x4080c0: zOpt::SetSoundLODOption.
 * Purpose: store the sound LOD option value.
 */
void __fastcall SetSoundLODOption(
    int value
) {
    *ZOPT_SOUND_LOD = value;
}

/**
 * Reimplements 0x4080d0: zOpt::GetSoundLODOption.
 * Purpose: return the current sound LOD option value.
 */
int GetSoundLODOption() {
    return *ZOPT_SOUND_LOD;
}

/**
 * Reimplements 0x4080e0: zOpt::SetTextureMemoryForCurrentHwMode.
 * Purpose: store the texture memory value for the active hardware mode.
 */
void __fastcall SetTextureMemoryForCurrentHwMode(
    int value
) {
    *(g_zOpt_HwMode != 0 ? ZOPT_TEXTURE_MEMORY_HW : ZOPT_TEXTURE_MEMORY_SW) = value;
}

/**
 * Reimplements 0x408100: zOpt::GetTextureMemoryForCurrentHwMode.
 * Purpose: return the texture memory value for the active hardware mode.
 */
int GetTextureMemoryForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_TEXTURE_MEMORY_HW : ZOPT_TEXTURE_MEMORY_SW);
}

/**
 * Reimplements 0x408120: zOpt::SetPlayerName.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: copy the supplied player name into the configured option buffer.
 */
void __fastcall SetPlayerName(
    const char *name
) {
    char *const buffer = (char *)(ZOPT_PLAYER_NAME->payloadOrBuffer);
    const unsigned int dataSize = (unsigned int)(ZOPT_PLAYER_NAME->dataSize);
    const size_t nameLength = strlen(name);

    if (nameLength < dataSize) {
        memcpy(
            buffer,
            name,
            nameLength + 1
        );
    } else {
        strncpy(
            buffer,
            name,
            dataSize - 1
        );
        buffer[dataSize - 1] = '\0';
    }
}

} // namespace zOpt
/**
 * Reimplements 0x408190: zOpt::GetPlayerName.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the configured player-name option buffer.
 */
char *zOpt_GetPlayerName() {
    return (char *)(ZOPT_PLAYER_NAME->payloadOrBuffer);
}
namespace zOpt {

/**
 * Reimplements 0x4081a0: zOpt::SetGraphicsFlagsForCurrentHwMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: store the graphics option bitmask for the active hardware mode and
 * mirror its lighting bit to the sunlight node.
 */
void __fastcall SetGraphicsFlagsForCurrentHwMode(
    int flags
) {
    *(g_zOpt_HwMode != 0 ? ZOPT_GFX_FLAGS_HW : ZOPT_GFX_FLAGS_SW) = flags;

    zClass_NodePartial *const sunlight = zClass::FindByTypeAndName(
        6,
        g_zOpt_DetailOptionName_Sunlight
    );
    if (sunlight != 0) {
        zClass_Class::gwNodeSetActive(
            sunlight,
            (flags & 0x10) != 0 ? 1 : 0
        );
    }
}

/**
 * Reimplements 0x4081f0: zOpt::GetGraphicsFlagsForCurrentHwMode.
 * Original source path: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the graphics option bitmask for the active hardware mode.
 */
int GetGraphicsFlagsForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_GFX_FLAGS_HW : ZOPT_GFX_FLAGS_SW);
}

} // namespace zOpt
namespace zSnd {

/**
 * Reimplements 0x408210: zSnd::SetCDAudioOption
 * Purpose: store the CD-audio option value used by sound and options code.
 */
void __fastcall SetCDAudioOption(
    int cdAudioOption
) {
    *ZOPT_SOUND_CDAUDIO = cdAudioOption;
}

/**
 * Reimplements 0x408220: zSnd::GetCDAudioOption
 * Purpose: return the current CD-audio option value.
 */
int GetCDAudioOption() {
    return *ZOPT_SOUND_CDAUDIO;
}

} // namespace zSnd
namespace zOpt {

/**
 * Reimplements 0x408230: zOpt::SetNetworkEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-enabled option value through its option pointer.
 */
void __fastcall SetNetworkEnabled(
    int value
) {
    *ZOPT_NETWORK_ENABLED = value;
}

/**
 * Reimplements 0x408240: zOpt::SetNetworkModemEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-modem option value through its option pointer.
 */
void __fastcall SetNetworkModemEnabled(
    int value
) {
    *g_zOpt_NetworkModemOption = value;
}

/**
 * Reimplements 0x408250: zOpt::SetNetworkListenEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-listen option value through its option pointer.
 */
void __fastcall SetNetworkListenEnabled(
    int value
) {
    *g_zOpt_NetworkListenOption = value;
}

/**
 * Reimplements 0x408260: zOpt::GetNetworkEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: return the network-enabled option value through its option pointer.
 */
int GetNetworkEnabled() {
    return *ZOPT_NETWORK_ENABLED;
}

/**
 * Reimplements 0x408270: zOpt::GetNetworkModemEnabled.
 * Original source path: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: return the network-modem option value through its option pointer.
 */
int GetNetworkModemEnabled() {
    return *g_zOpt_NetworkModemOption;
}

} // namespace zOpt
namespace zVid {

/**
 * Reimplements 0x408280: zVid::SetAccelerationOption.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: store the selected video acceleration option and mirror the active
 * hardware-mode option used by zOpt accessors.
 *
 * Evidence: BN writes ecx through ZOPT_VIDEO_ACCELERATION, then stores the same
 * value into g_zOpt_HwMode; VC5SP3 zvid_option_getters byte verification is
 * exact after relocation masking.
 */
void __fastcall SetAccelerationOption(
    int accelerationOption
) {
    *ZOPT_VIDEO_ACCELERATION = accelerationOption;
    g_zOpt_HwMode = accelerationOption;
}

/**
 * Reimplements 0x408290: zVid::SetHwApiOption.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: store the selected hardware API/backend option.
 *
 * Evidence: BN writes ecx through ZOPT_HW_API and returns without touching
 * other state; VC5SP3 zvid_option_getters byte verification is exact after
 * relocation masking.
 */
void __fastcall SetHwApiOption(
    int hwApiOption
) {
    *ZOPT_HW_API = hwApiOption;
}

} // namespace zVid
namespace zOpt {

/**
 * Reimplements 0x4082a0: zOpt::SetFullscreenOption.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: store the persisted fullscreen/windowed option value.
 */
void __fastcall SetFullscreenOption(
    int fullscreenOption
) {
    *ZOPT_VIDEO_FULLSCREEN = fullscreenOption;
}

/**
 * Reimplements 0x4082b0: zOpt::SetHudVisibilityOption.
 * Purpose: store the HUD visibility option for the active hardware mode.
 */
void __fastcall SetHudVisibilityOption(
    int hudVisibility
) {
    *(g_zOpt_HwMode != 0 ? ZOPT_HUD_HW : ZOPT_HUD_SW) = hudVisibility;
}

/**
 * Reimplements 0x4082d0: zOpt::SetHudTypeForCurrentHwMode.
 * Purpose: apply the requested HUD layout mode and store it for the active hardware mode.
 */
int __fastcall SetHudTypeForCurrentHwMode(
    int hudType
) {
    const int previous = HudUiMgr::ApplyHudModeSwitch(hudType);

    if (g_zOpt_HwMode != 0) {
        *ZOPT_HUD_TYPE_HW = hudType;
        return previous;
    }

    *ZOPT_HUD_TYPE_SW = hudType;
    return previous;
}

/**
 * Reimplements 0x408300: zOpt::SetReplicateMode.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: store the active video replicate-mode option.
 *
 * Evidence: BN writes ecx through ZOPT_REPLICATE and returns; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall SetReplicateMode(
    int replicateMode
) {
    *ZOPT_REPLICATE = replicateMode;
}

} // namespace zOpt
namespace zVid {

/**
 * Reimplements 0x408310: zVid::GetAccelerationOption.
 * Purpose: provide the recovered zVid::GetAccelerationOption behavior.
 */
int GetAccelerationOption() {
    return *ZOPT_VIDEO_ACCELERATION;
}

/**
 * Reimplements 0x408320: zVid::GetHwApiOption.
 * Purpose: provide the recovered zVid::GetHwApiOption behavior.
 */
int GetHwApiOption() {
    return *ZOPT_HW_API;
}

} // namespace zVid
namespace zOpt {

/**
 * Reimplements 0x408330: zOpt::GetFullscreenOption.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the persisted fullscreen/windowed option value.
 */
int GetFullscreenOption() {
    return *ZOPT_VIDEO_FULLSCREEN;
}

/**
 * Reimplements 0x408340: zOpt::GetHudVisibilityOption.
 * Purpose: return the HUD visibility option for the active hardware mode.
 */
int GetHudVisibilityOption() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_HUD_HW : ZOPT_HUD_SW);
}

/**
 * Reimplements 0x408360: zOpt::GetHudTypeForCurrentHwMode.
 * Original source path: D:\Proj\Battlesport\zopt.cpp.
 * Purpose: return the HUD type option for the active hardware mode.
 */
int GetHudTypeForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? ZOPT_HUD_TYPE_HW : ZOPT_HUD_TYPE_SW);
}

/**
 * Reimplements 0x408380: zOpt::GetReplicateMode
 * Purpose: return the active video replicate-mode option.
 */
int GetReplicateMode() {
    return *ZOPT_REPLICATE;
}

} // namespace zOpt
namespace zInp {

/**
 * Reimplements 0x408390: zInp::SetJoystickOption.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_opt.cpp.
 * Purpose: store the joystick-enabled option when the option slot exists.
 */
void __fastcall SetJoystickOption(
    int enabled
) {
    if (ZOPT_INPUT_JOYSTICK != 0) {
        *ZOPT_INPUT_JOYSTICK = enabled;
    }
}

/**
 * Reimplements 0x4083a0: zInp::SetJoystickAxesCountOption.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_opt.cpp.
 * Purpose: store the detected joystick axis count option value.
 */
void __fastcall SetJoystickAxesCountOption(
    int axisCount
) {
    *ZOPT_JOYSTICK_NUM_AXES = axisCount;
}

/**
 * Reimplements 0x4083b0: zInp::SetJoystickButtonCountOption.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_opt.cpp.
 * Purpose: store the detected joystick button count option value.
 */
void __fastcall SetJoystickButtonCountOption(
    int buttonCount
) {
    *ZOPT_JOYSTICK_NUM_BUTTONS = buttonCount;
}

/**
 * Reimplements 0x4083c0: zInp::GetJoystickOption.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_opt.cpp.
 * Purpose: return the joystick-enabled option value.
 */
int GetJoystickOption() {
    return *ZOPT_INPUT_JOYSTICK;
}

} // namespace zInp
namespace zOpt {

/**
 * Reimplements 0x4083d0: zOpt_ViewRectSection::SetPosition
 * Purpose: store origin and recompute bounds from size.
 */
void __fastcall ViewRectSection_SetPosition(
    zOpt_ViewRectSection *section,
    int x,
    int y
) {
    section->x = x;
    section->y = y;
    section->rightExclusive = x + section->width;
    section->bottomExclusive = y + section->height;
    section->maxXInclusive = section->rightExclusive - 1;
    section->maxYInclusive = section->bottomExclusive - 1;
}

/**
 * Reimplements 0x408400: zOpt_ViewRectSection::SetSize
 * Purpose: store size and recompute bounds from origin.
 */
void __fastcall ViewRectSection_SetSize(
    zOpt_ViewRectSection *section,
    int width,
    int height
) {
    section->width = width;
    section->height = height;
    section->rightExclusive = section->x + width;
    section->bottomExclusive = section->y + height;
    section->maxXInclusive = section->rightExclusive - 1;
    section->maxYInclusive = section->bottomExclusive - 1;
}

/**
 * Reimplements 0x408430: zOpt::ViewRectSection_ClampPointToInclusiveBounds
 * Purpose: clamp a point to inclusive bounds.
 */
void __fastcall ViewRectSection_ClampPointToInclusiveBounds(
    zOpt_ViewRectSection *section,
    float *pointXY
) {
    if (pointXY[0] < (float)(section->x)) {
        pointXY[0] = (float)(section->x);
    } else if (!(pointXY[0] <= (float)(section->maxXInclusive))) {
        pointXY[0] = (float)(section->maxXInclusive);
    }

    if (pointXY[1] < (float)(section->y)) {
        pointXY[1] = (float)(section->y);
    } else if (!(pointXY[1] <= (float)(section->maxYInclusive))) {
        pointXY[1] = (float)(section->maxYInclusive);
    }
}

/**
 * Reimplements 0x408480: zOpt::CameraSection_SetActiveCamera
 * Purpose: store camera, recompute FOV, and reapply LOD.
 */
void __fastcall CameraSection_SetActiveCamera(
    zClass_NodePartial *camera
) {
    zOpt_CameraSection *const cameraSection = *g_zOpt_CameraSectionOption;
    cameraSection->m_pCamera = camera;
    if (camera == 0) {
        return;
    }

    zOpt_ViewRectSection *const renderSection = *g_zOpt_RenderSectionOption;
    float fovX = 0.0f;
    float fovY = 0.0f;
    zClass_Camera::gwCameraGetFOV(
        camera,
        &fovX,
        &fovY
    );

    fovX = (float)(renderSection->width) * fovY / (float)(renderSection->height);
    zClass_Camera::gwCameraSetFOV(
        cameraSection->m_pCamera,
        fovX,
        fovY
    );
    zOpt::SetObjectLODForCurrentHwMode(zOpt::GetObjectLODForCurrentHwMode());
}

} // namespace zOpt
/**
 * Reimplements 0x4084e0: zOpt_CameraSection_GetActiveCamera
 * Purpose: return active camera or null when unavailable.
 */
zClass_NodePartial *zOpt_CameraSection_GetActiveCamera() {
    if (g_zOpt_CameraSectionOption == 0 || *g_zOpt_CameraSectionOption == 0) {
        return 0;
    }

    return (*g_zOpt_CameraSectionOption)->m_pCamera;
}
namespace zOpt {

/**
 * Reimplements 0x408500: zOpt::RenderSection_SetSize.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the render-section dimensions and push the new resolution to
 * the attached window target.
 *
 * Evidence: BN forwards g_zOpt_RenderSectionOption->value to
 * zOpt_ViewRectSection::SetSize, then calls gwWindowSetResolution when the
 * section target is non-null; the shared zopt_video_section_setters VC5SP3
 * target byte-matches after relocation masking.
 */
void __fastcall RenderSection_SetSize(
    int width,
    int height
) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    ViewRectSection_SetSize(
        section,
        width,
        height
    );
    if (section->target != 0) {
        zClass_Window::gwWindowSetResolution(
            (zClass_NodePartial *)(section->target),
            section->width,
            section->height
        );
    }
}

/**
 * Reimplements 0x408530: zOpt::RenderSection_SetPosition.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the render-section origin and push the new viewport rectangle
 * to the attached window target.
 *
 * Evidence: BN forwards g_zOpt_RenderSectionOption->value to
 * zOpt_ViewRectSection::SetPosition, then calls gwWindowSetResolution and
 * gwWindowSetSize when the section target is non-null; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall RenderSection_SetPosition(
    int x,
    int y
) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    ViewRectSection_SetPosition(
        section,
        x,
        y
    );
    if (section->target != 0) {
        zClass_Window::gwWindowSetResolution(
            (zClass_NodePartial *)(section->target),
            section->width,
            section->height
        );
        zClass_Window::gwWindowSetSize(
            (zClass_NodePartial *)(section->target),
            section->x,
            section->y
        );
    }
}

/**
 * Reimplements 0x408570: zOpt::RenderSection_SetTargetWindow
 * Purpose: attach target window and apply render rectangle.
 */
void __fastcall RenderSection_SetTargetWindow(
    zClass_NodePartial *windowNode
) {
    zOpt_ViewRectSection *section = *g_zOpt_RenderSectionOption;
    section->target = windowNode;
    if (windowNode != 0) {
        zClass_Window::gwWindowSetResolution(
            windowNode,
            section->width,
            section->height
        );
        zClass_Window::gwWindowSetSize(
            (zClass_NodePartial *)(section->target),
            section->x,
            section->y
        );
    }
}

/**
 * Reimplements 0x4085a0: zOpt::GetRenderSection
 * Purpose: return the active render section pointer.
 */
zOpt_ViewRectSection *GetRenderSection() {
    return *g_zOpt_RenderSectionOption;
}

/**
 * Reimplements 0x4085b0: zOpt::DisplaySection_SetTargetDisplay
 * Purpose: attach target display and apply display rectangle.
 */
void __fastcall DisplaySection_SetTargetDisplay(
    zClass_NodePartial *displayNode
) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    section->target = displayNode;
    if (displayNode != 0) {
        zClass_Display::gwDisplaySetSize(
            displayNode,
            section->width,
            section->height
        );
        zClass_Display::gwDisplaySetPosition(
            (zClass_NodePartial *)(section->target),
            section->x,
            section->y
        );
    }
}

/**
 * Reimplements 0x4085e0: zOpt::DisplaySection_SetPosition.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the display-section origin and push the new display rectangle
 * to the attached display target.
 *
 * Evidence: BN forwards g_zOpt_DisplaySectionOption->value to
 * zOpt_ViewRectSection::SetPosition, then calls gwDisplaySetSize and
 * gwDisplaySetPosition when the section target is non-null; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall DisplaySection_SetPosition(
    int x,
    int y
) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    ViewRectSection_SetPosition(
        section,
        x,
        y
    );
    if (section->target != 0) {
        zClass_Display::gwDisplaySetSize(
            (zClass_NodePartial *)(section->target),
            section->width,
            section->height
        );
        zClass_Display::gwDisplaySetPosition(
            (zClass_NodePartial *)(section->target),
            section->x,
            section->y
        );
    }
}

/**
 * Reimplements 0x408620: zOpt::DisplaySection_SetSize.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the display-section dimensions and push the new size to the
 * attached display target.
 *
 * Evidence: BN forwards g_zOpt_DisplaySectionOption->value to
 * zOpt_ViewRectSection::SetSize, then calls gwDisplaySetSize when the section
 * target is non-null; the shared zopt_video_section_setters VC5SP3 target
 * byte-matches after relocation masking.
 */
void __fastcall DisplaySection_SetSize(
    int width,
    int height
) {
    zOpt_ViewRectSection *section = *g_zOpt_DisplaySectionOption;
    ViewRectSection_SetSize(
        section,
        width,
        height
    );
    if (section->target != 0) {
        zClass_Display::gwDisplaySetSize(
            (zClass_NodePartial *)(section->target),
            section->width,
            section->height
        );
    }
}

/**
 * Reimplements 0x408650: zOpt::GetDisplaySection.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active display view-rect option record.
 */
zOpt_ViewRectSection *GetDisplaySection() {
    return *g_zOpt_DisplaySectionOption;
}

} // namespace zOpt
/**
 * Reimplements 0x408660: zOpt_DisplaySection_GetWidth.
 * Purpose: return the active display section width.
 */
int zOpt_DisplaySection_GetWidth() {
    return (*g_zOpt_DisplaySectionOption)->width;
}

/**
 * Reimplements 0x408670: zOpt_DisplaySection_GetHeight.
 * Purpose: return the active display section height.
 */
int zOpt_DisplaySection_GetHeight() {
    return (*g_zOpt_DisplaySectionOption)->height;
}
namespace zOpt {

/**
 * Reimplements 0x408680: zOpt::DisplaySection_SetBitsPerPixel.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: store the active display-section bit depth.
 *
 * Evidence: BN writes ecx to g_zOpt_DisplaySectionOption->value->bitsPerPixel;
 * the shared zopt_video_section_setters VC5SP3 target byte-matches after
 * relocation masking.
 */
void __fastcall DisplaySection_SetBitsPerPixel(
    int bitsPerPixel
) {
    (*g_zOpt_DisplaySectionOption)->bitsPerPixel = bitsPerPixel;
}

/**
 * Reimplements 0x408690: zOpt::GetDisplaySectionBitsPerPixel.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active display section bit depth.
 */
int GetDisplaySectionBitsPerPixel() {
    return (*g_zOpt_DisplaySectionOption)->bitsPerPixel;
}

/**
 * Reimplements 0x4086a0: zOpt::GetVideoStrideValue.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the configured video stride option value.
 */
int GetVideoStrideValue() {
    return *ZOPT_VIDEO_STRIDE;
}

} // namespace zOpt
namespace zVid {

/**
 * Reimplements 0x4086b0: zVid::GetVideoModeIndexFromOptions.
 * Purpose: provide the recovered zVid::GetVideoModeIndexFromOptions behavior.
 */
int GetVideoModeIndexFromOptions() {
    return *ZOPT_VIDEO_MODE;
}

} // namespace zVid
namespace zOpt {

/**
 * Reimplements 0x4086c0: zOpt::GetWindowSection.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active window view-rect option record.
 */
zOpt_ViewRectSection *GetWindowSection() {
    return *g_zOpt_WindowSectionOption;
}

/**
 * Reimplements 0x4086d0: zOpt::GetWindowSectionHeight.
 * Original source path: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active window section height.
 */
int GetWindowSectionHeight() {
    return (*g_zOpt_WindowSectionOption)->height;
}

/**
 * Reimplements 0x4086e0: zOpt::WindowSection_SetSize.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the window-section dimensions.
 *
 * Evidence: BN forwards g_zOpt_WindowSectionOption->value to
 * zOpt_ViewRectSection::SetSize; the shared zopt_video_section_setters VC5SP3
 * target byte-matches after relocation masking.
 */
void __fastcall WindowSection_SetSize(
    int width,
    int height
) {
    ViewRectSection_SetSize(
        *g_zOpt_WindowSectionOption,
        width,
        height
    );
}

/**
 * Reimplements 0x408700: zOpt::WindowSection_SetPosition.
 * Original file: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the window-section origin.
 *
 * Evidence: BN forwards g_zOpt_WindowSectionOption->value to
 * zOpt_ViewRectSection::SetPosition; the shared zopt_video_section_setters
 * VC5SP3 target byte-matches after relocation masking.
 */
void __fastcall WindowSection_SetPosition(
    int x,
    int y
) {
    ViewRectSection_SetPosition(
        *g_zOpt_WindowSectionOption,
        x,
        y
    );
}

} // namespace zOpt
namespace zVid {

/**
 * Reimplements 0x408720: zVid::SetVideoModeIndex.
 * Original file: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: apply a persisted shell video-mode preset to the render, window,
 * display, and replicate options.
 *
 * Evidence: BN selects modes 2 through 7 through the jump table, writes
 * ZOPT_VIDEO_MODE, updates the zOpt render/window/display sections, sets
 * display bits-per-pixel to 16, and tail-calls zOpt::SetReplicateMode for
 * valid presets; invalid values write ZVID_MODE_INVALID. VC5SP3
 * zvid_set_video_mode_index byte verification is exact after relocation
 * masking.
 */
void __fastcall SetVideoModeIndex(
    int modeIndex
) {
    switch (modeIndex) {
    case 2:
        *ZOPT_VIDEO_MODE = 2;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            320,
            200
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            640,
            400
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            640,
            400
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(1);
        return;

    case 3:
        *ZOPT_VIDEO_MODE = 3;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            320,
            240
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            640,
            480
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            640,
            480
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(1);
        return;

    case 4:
        *ZOPT_VIDEO_MODE = 4;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            640,
            400
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            640,
            400
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            640,
            400
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(0);
        return;

    case 5:
        *ZOPT_VIDEO_MODE = 5;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            640,
            480
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            640,
            480
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            640,
            480
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(0);
        return;

    case 6:
        *ZOPT_VIDEO_MODE = 6;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            800,
            600
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            800,
            600
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            800,
            600
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(0);
        return;

    case 7:
        *ZOPT_VIDEO_MODE = 7;
        zOpt::RenderSection_SetPosition(
            0,
            0
        );
        zOpt::RenderSection_SetSize(
            1024,
            768
        );
        zOpt::WindowSection_SetPosition(
            0,
            0
        );
        zOpt::WindowSection_SetSize(
            1024,
            768
        );
        zOpt::DisplaySection_SetPosition(
            0,
            0
        );
        zOpt::DisplaySection_SetSize(
            1024,
            768
        );
        zOpt::DisplaySection_SetBitsPerPixel(16);
        zOpt::SetReplicateMode(0);
        return;

    default:
        *ZOPT_VIDEO_MODE = 0;
        return;
    }
}

} // namespace zVid
namespace HudUiMgr {

/**
 * Reimplements 0x4089c0: HudUiMgr::ScreenToWorld.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * BN name: MapReplicatedScreenToRenderPoint.
 * Purpose: map a replicated display-space HUD point back into render-viewport
 * coordinates before clamping it to the render section.
 * Source shape: __fastcall function with a single float *pointXY argument;
 * callers pass the address of the point's x field, and the function updates
 * pointXY[0] and pointXY[1] in place.
 * Data dependencies: zOpt render/display view-rect option globals and
 * replicate mode are covered by accepted owner
 * engine.zgame.zopt_video_section_option_globals.
 * HudUi data: touches no HudUi table, singleton, or manager-owned storage.
 */
void __fastcall ScreenToWorld(
    float *pointXY
) {
    zOpt_ViewRectSection *const renderSection = *g_zOpt_RenderSectionOption;
    zOpt_ViewRectSection *const displaySection = *g_zOpt_DisplaySectionOption;

    if (zOpt::GetReplicateMode() == 0) {
        return;
    }

    pointXY[0] *= 0.5f;
    pointXY[1] = (float)(renderSection->y) + (pointXY[1] - (float)(displaySection->y)) * 0.5f;
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(
        renderSection,
        pointXY
    );
}

} // namespace HudUiMgr
namespace zOpt {

/**
 * Reimplements 0x408a10: zOpt::SetWolPasswordFlag.
 * Purpose: store the WOL password flag option value through its option pointer.
 */
void __fastcall SetWolPasswordFlag(
    int value
) {
    *g_zOpt_WolPasswordFlagOption = value;
}

} // namespace zOpt
/**
 * Reimplements 0x408a20: zOpt_GetWolPasswordFlagValue.
 * Purpose: return the WOL password flag option value through its option pointer.
 */
int zOpt_GetWolPasswordFlagValue() {
    return *g_zOpt_WolPasswordFlagOption;
}

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
 * Reimplements the ordinary empty RecoilStateDialogHost::OnSuspend identity
 * represented by the one-argument no-op fold group at 0x407150.
 * Original function address: 0x407150.
 * Purpose: accept suspend notifications when a derived dialog host does not
 * require presentation-state work.
 */
void RecoilStateDialogHost::OnSuspend(
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
 * Reimplements 0x408a30: HudUiControlsDialog::Constructor.
 * Original source path: D:\Proj\Battlesport\hud_ui_dialogs.cpp.
 * Purpose: Construct the controls dialog, bind its ZRD widgets, and seed option selectors from current input/options.
 * Evidence: BN/source slice builds HudUiBackground, resume/commands widgets, five option selectors, loads
 * dialog.zrd/CONTROLS_DIALOG, binds named controls, then seeds zInp/zOpt selector indices.
 */
HudUiControlsDialog * HudUiControlsDialog::Constructor() {
    new ((HudUiBackground *)this) HudUiBackground;

    new (&resumeWidget) HudUiControlsDialog_ResumeWidget;
    new (&commandsWidget) HudUiControlsDialog_CommandsWidget;
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
        delete dialog;
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
        delete dialogToDelete;
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

/**
 * Reimplements 0x409010: HudUiZrdWidgetEx17C::EnableChildAtIndex.
 * Source file evidence: BN labels the source as D:\Proj\Battlesport\hudui_zrdwidget.cpp.
 * Purpose: enable an in-range option item and refresh its displayed widget state.
 */
void HudUiZrdWidgetEx17C::EnableChildAtIndex(
    int childIndex
) {
    if (childIndex >= optionCount) {
        return;
    }

    HudUiZrdWidgetEx17C_Item *const option = options[childIndex];
    option->modeOrEnabled = 1;
    option->RefreshState();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x4091e0 HudUiZrdScrollingText::Destructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiZrdScrollingText::HudUiZrdScrollingText.
 */
inline HudUiZrdScrollingText::HudUiZrdScrollingText() : HudUiZrdWidget() {
}

/**
 * Reimplements 0x409040: HudUiCreditsPanel::HudUiCreditsPanel.
 * Original source path: D:\Proj\Battlesport\HudUiCreditsPanel.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiCreditsPanel::HudUiCreditsPanel.
 */
HudUiCreditsPanel::HudUiCreditsPanel() : HudUiBackground() {
    fadeProgress = 0.0f;
    fadeStep = 0.05f;
    HudUiZrdScrollingText *const screen = &creditsScreen;

    zReader::Node *const loadedSection = HudUiBackground::LoadFromZrd(
        "dialog.zrd",
        "CREDITSPANEL",
        0
    );
    if (loadedSection != 0) {
        if (g_RecoilApp_QuitAfterCredits != 0) {
            HudUiBackground::BindWidgetByName(
                loadedSection,
                (HudUiZrdWidget *)(&quitButton),
                "QUIT"
            );
        } else {
            HudUiBackground::BindWidgetByName(
                loadedSection,
                (HudUiZrdWidget *)(&backButton),
                "BACK"
            );
        }

        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(screen),
            "CREDITS_SCREEN"
        );
        HudUiBackground::FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
    }

    unsigned int screenFlags = 0;
    screenFlags = (unsigned char)(screen->flags);
    screen->flags = screenFlags & 0x10u;
}

/**
 * Reimplements 0x409160: HudUiCreditsBackButton::OnActivate.
 * Purpose: queue exit from the credits state and run the inherited activation behavior.
 */
void HudUiCreditsBackButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x409160: HudUiControlsDialog_ResumeWidget::OnActivate.
 * Purpose: queue exit from the controls state and run the inherited activation behavior.
 */
void HudUiControlsDialog_ResumeWidget::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x409160: HudCmdSimpleWidget::OnActivate.
 * Purpose: queue exit from the command-binding state and run the inherited activation behavior.
 */
void HudCmdSimpleWidget::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x409160: HudUiConfirmQuitCancelButton::OnActivate.
 * Purpose: queue exit from the quit-confirmation state and run the inherited activation behavior.
 */
void HudUiConfirmQuitCancelButton::OnActivate() {
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
 * Reimplements 0x4091e0: HudUiZrdScrollingText::~HudUiZrdScrollingText.
 * Original source path: D:\Proj\Battlesport\HudUiCreditsPanel.cpp.
 * Purpose: invoke ordinary row-vector and inherited-widget teardown for the
 * scrolling credits member.
 */
HudUiZrdScrollingText::~HudUiZrdScrollingText() {
}

/**
 * Reimplements 0x4092a0: HudUiCreditsPanel::~HudUiCreditsPanel.
 * Original source path: D:\Proj\Battlesport\HudUiCreditsPanel.cpp.
 * Purpose: invoke ordinary reverse member and base teardown for the credits
 * panel at the end of its lifetime.
 */
HudUiCreditsPanel::~HudUiCreditsPanel() {
}

/**
 * Reimplements 0x409380: HudUiCreditsPanel::UpdateAll
 * Source: D:\Proj\Battlesport\HudUiCreditsPanel.cpp
 * Purpose: advance the credits fade, update the panel, and queue the post-credits transition.
 */
void HudUiCreditsPanel::UpdateAll(
    float deltaSeconds
) {
    creditsScreen.UpdateScrollPositions(fadeProgress);
    fadeProgress += fadeStep * deltaSeconds;
    HudUiBackgroundContainer::UpdateAll(deltaSeconds);

    if (fadeProgress < 1.0f) {
        return;
    }

    if (g_RecoilApp_QuitAfterCredits != 0) {
        g_RecoilApp.QueueExitCurrentState(1);
        g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
        g_RecoilApp.QueueSwitchCurrentState(
            &g_RecoilApp.m_leaveNetworkState,
            0
        );
        return;
    }

    g_RecoilApp.QueueExitCurrentState(0);
}

/**
 * Reimplements 0x409410: HudUiZrdScrollingText::Update
 * Source: D:\Proj\Battlesport\HudUiCreditsPanel.cpp
 * Purpose: update the scrolling credits widget and each row panel.
 */
void HudUiZrdScrollingText::Update(
    float deltaSeconds
) {
    HudUiElement::Update(deltaSeconds);

    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end) {
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end) {
            entry->panel.Update(deltaSeconds);
            ++entry;
        }

        ++row;
    }
}

/**
 * Reimplements 0x409470: HudUiZrdScrollingText::UpdateScrollPositions
 * Source: D:\Proj\Battlesport\HudUiCreditsPanel.cpp
 * Purpose: position scrolling credits row entries and clip their panel visibility to the text rectangle.
 */
void HudUiZrdScrollingText::UpdateScrollPositions(
    float scrollProgress
) {
    const int left = rect.left;
    const int scrollY = (int)((float)(rect.top - totalHeight) * scrollProgress +
                              (1.0 - scrollProgress) * (float)(rect.bottom));

    HudUiPanelSpan *row = rows.begin;
    while (row != rows.end) {
        HudUiPanelLayoutEntry *entry = row->begin;
        while (entry != row->end) {
            const int y = entry->layoutY + scrollY;
            entry->panel.SetPos(
                entry->layoutX + left,
                y
            );
            if (y > rect.top && y + entry->panel.QueryTextHeight() < rect.bottom) {
                entry->panel.SetVisible(1);
            } else {
                entry->panel.SetVisible(0);
            }

            ++entry;
        }

        ++row;
    }
}

/**
 * Reimplements 0x409550: HudUiZrdScrollingText::OnActivateResetOwnerFade
 * Source: D:\Proj\Battlesport\HudUiCreditsPanel.cpp
 * Purpose: reset the owning credits panel fade progress when the scrolling credits text activates.
 */
void HudUiZrdScrollingText::OnActivateResetOwnerFade() {
    ((HudUiCreditsPanel *)(owner))->fadeProgress = 0.0f;
}

#include "Battlesport/recoil_state_credits_body.h"
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
            delete panel;
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

#include "Battlesport/hud_runtime_layer_body.h"

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
            delete dialog;
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
        delete dialog;
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
