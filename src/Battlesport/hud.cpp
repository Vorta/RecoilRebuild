#include "recoil/Mfc42Abi.h"
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
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zSys/zsys.h"
#include "GameZRecoil/zFMV/fmv.h"

#include "GameZRecoil/zInterp/zInterp.h"
#include "GameZRecoil/zUtil/zsave_game.h"
#include "Battlesport/turret.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "GameZRecoil/zWeapon/zwep.h"
#include "Battlesport/wol_download.h"

#include <math.h>
#include <new>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#undef g_HudUiOptionsPanelOverlayOwner
/**
 * Source owner: legacy.hud_ui.class_huduioptionspaneloverlayowner.
 * Purpose: own the zero-initialized options-panel overlay singleton storage.
 */
HudUiOptionsPanelOverlayOwnerStorage g_HudUiOptionsPanelOverlayOwner = {0};
#undef g_RecoilState_ConfirmQuit
/**
 * Purpose: own the zero-initialized confirm-quit app-state singleton storage.
 */
RecoilStateConfirmQuitStorage g_RecoilState_ConfirmQuit = {0};
extern "C" int g_RecoilState_MainMenuSkipExitDelay = 0;
#undef g_RecoilStateControls
/**
 * Source owner: legacy.app_shell.class_recoilstatecontrols.
 * Purpose: own the zero-initialized controls app-state singleton storage.
 */
RecoilStateControlsStorage g_RecoilStateControls = {0};
#undef g_RecoilStateCheatCode
/**
 * Source owner: legacy.app_shell.class_recoilstatecheatcode.
 * Purpose: own the zero-initialized cheat-code app-state singleton storage.
 */
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
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduidialogsamplesetname
 * @recoil-artifact defines .data recoil:data:0x4da3d8: g_HudUiDialogSampleSetName.
 * Source owner: hud_ui.shared_dialog_sample_set_name.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: names the shared dialog sample set loaded by HUD/menu dialog states.
 */
char g_HudUiDialogSampleSetName[0x7] = "DIALOG";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiDialogSampleSetName) == 0x7);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicontrolsdialog-cameramodeselectornodename
 * @recoil-artifact defines .data recoil:data:0x4da8d8: g_HudUiControlsDialog_CameraModeSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD camera-mode selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_CameraModeSelectorNodeName[] = "CAMERA_MODE";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicontrolsdialog-cursormodeselectornodename
 * @recoil-artifact defines .data recoil:data:0x4da8e4: g_HudUiControlsDialog_CursorModeSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD cursor-mode selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_CursorModeSelectorNodeName[] = "CURSOR_MODE";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicontrolsdialog-steeringmodeselectornodename
 * @recoil-artifact defines .data recoil:data:0x4da8f0: g_HudUiControlsDialog_SteeringModeSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD steering-mode selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_SteeringModeSelectorNodeName[] = "STEERING_MODE";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicontrolsdialog-throttlemodeselectornodename
 * @recoil-artifact defines .data recoil:data:0x4da900: g_HudUiControlsDialog_ThrottleModeSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD throttle-mode selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_ThrottleModeSelectorNodeName[] = "THROTTLE_MODE";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicontrolsdialog-mouseorjoystickselectornodename
 * @recoil-artifact defines .data recoil:data:0x4da910: g_HudUiControlsDialog_MouseOrJoystickSelectorNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD mouse-or-joystick selector node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_MouseOrJoystickSelectorNodeName[] = "MOUSE_OR_JOYSTICK";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicontrolsdialog-commandsbuttonnodename
 * @recoil-artifact defines .data recoil:data:0x4da924: g_HudUiControlsDialog_CommandsButtonNodeName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD commands button node bound by HudUiControlsDialog.
 */
char g_HudUiControlsDialog_CommandsButtonNodeName[] = "COMMANDS_BTN";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduiresumebuttonnodename
 * @recoil-artifact defines .data recoil:data:0x4da934: g_HudUiResumeButtonNodeName.
 * Source owner: hud_ui.shared_resume_button_node_name_string.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the shared resume button node bound by Controls and NetExit HUD UI dialogs.
 */
char g_HudUiResumeButtonNodeName[] = "RESUME";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicontrolsdialogsectionname
 * @recoil-artifact defines .data recoil:data:0x4da93c: g_HudUiControlsDialogSectionName.
 * Source owner: hud_ui.hud_ui_controls_dialog_strings.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZRD controls dialog section loaded by HudUiControlsDialog.
 */
char g_HudUiControlsDialogSectionName[] = "CONTROLS_DIALOG";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduibackgroundconfirmquit-cancelbuttonnodename
 * @recoil-artifact defines .data recoil:data:0x4daedc: g_HudUiBackgroundConfirmQuit_CancelButtonNodeName.
 * Source owner: hud_ui.confirm_quit_dialog_literal_strings.
 * Purpose: name the ZRD cancel button node bound by HudUiBackgroundConfirmQuit.
 */
char g_HudUiBackgroundConfirmQuit_CancelButtonNodeName[0xc] = "CANCEL_QUIT";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiBackgroundConfirmQuit_CancelButtonNodeName) == 0xc);
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduibackgroundconfirmquit-okbuttonnodename
 * @recoil-artifact defines .data recoil:data:0x4daee8: g_HudUiBackgroundConfirmQuit_OkButtonNodeName.
 * Source owner: hud_ui.confirm_quit_dialog_literal_strings.
 * Purpose: name the ZRD OK button node bound by HudUiBackgroundConfirmQuit.
 */
char g_HudUiBackgroundConfirmQuit_OkButtonNodeName[0xb] = "OK_TO_QUIT";
RECOIL_STATIC_ASSERT(sizeof(g_HudUiBackgroundConfirmQuit_OkButtonNodeName) == 0xb);
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduibackgroundconfirmquit-sectionname
 * @recoil-artifact defines .data recoil:data:0x4daef4: g_HudUiBackgroundConfirmQuit_SectionName.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-draw
 * @recoil-artifact defines .text recoil:function:0x404ca0: HudUiElement::Draw.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiElement::Draw.
 */
void HudUiElement::Draw() {
    DrawBase();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-drawbase
 * @recoil-artifact defines .text recoil:function:0x404cb0: HudUiElement::DrawBase.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-setpos
 * @recoil-artifact defines .text recoil:function:0x404cd0: HudUiElement::SetPos.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-setx
 * @recoil-artifact defines .text recoil:function:0x404cf0: HudUiElement::SetX.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update the horizontal element position and invalidate the element.
 */
void HudUiElement::SetX(
    int newX
) {
    x = newX;
    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-sety
 * @recoil-artifact defines .text recoil:function:0x404d00: HudUiElement::SetY.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update the vertical element position and invalidate the element.
 */
void HudUiElement::SetY(
    int newY
) {
    y = newY;
    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-hittesttrue
 * @recoil-artifact defines .text recoil:function:0x404d10: HudUiElement::HitTestTrue.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-setvisible
 * @recoil-artifact defines .text recoil:function:0x404d20: HudUiElement::SetVisible.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-getcenterx
 * @recoil-artifact defines .text recoil:function:0x404d50: HudUiElement::GetX.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return the base element x-coordinate from the recovered center-position virtual slot.
 */
int HudUiElement::GetCenterX() {
    return x;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduielement-getcentery
 * @recoil-artifact defines .text recoil:function:0x404d60: HudUiElement::GetY.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return the base element y-coordinate from the recovered center-position virtual slot.
 */
int HudUiElement::GetCenterY() {
    return y;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduiwidget-getcenterx
 * @recoil-artifact defines .text recoil:function:0x404d90: HudUiWidget::GetCenterX.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduiwidget-getcentery
 * @recoil-artifact defines .text recoil:function:0x404dd0: HudUiWidget::GetCenterY.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduiwidget-rebuildbltrectfromimage
 * @recoil-artifact defines .text recoil:function:0x404e10: HudUiWidget::RebuildBltRectFromImage.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicircle-hittest
 * @recoil-artifact defines .text recoil:function:0x404e60: HudUiCircle::HitTest.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * Provider boundary: stripped legacy zError reporting no-op.
 * Purpose: Preserves the stripped retail legacy-report call ABI without producing output.
 */
inline void __cdecl ReportOld(
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
 * @recoil-anchor recoil:anchor:battlesport.hud.tickactivecamerastate
 * @recoil-artifact defines .text recoil:function:0x404e90: Player::TickActiveCameraState.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.updatechasecamerafrominput
 * @recoil-artifact defines .text recoil:function:0x405040: Player::UpdateChaseCameraFromInput.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.updatethirdpersoncamera
 * @recoil-artifact defines .text recoil:function:0x405650: Player::UpdateThirdPersonCamera
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
 * @recoil-anchor recoil:anchor:battlesport.hud.updatetopdowncamerastate
 * @recoil-artifact defines .text recoil:function:0x4057d0: Player::UpdateTopDownCameraState.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.updatecamerafromstoredtargettowardplayer
 * @recoil-artifact defines .text recoil:function:0x405870: Player::UpdateCameraFromStoredTargetTowardPlayer.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.updatefirstpersoncamerafrominput
 * @recoil-artifact defines .text recoil:function:0x4059a0: Player::UpdateFirstPersonCameraFromInput.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.applycamerastate
 * @recoil-artifact defines .text recoil:function:0x405c90: Player::ApplyCameraState
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
 * @recoil-anchor recoil:anchor:battlesport.hud.togglesteeringmodeandresetmouselook
 * @recoil-artifact defines .text recoil:function:0x405ec0: Player::ToggleSteeringModeAndResetMouseLook
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.adjustthirdpersoncamerabyoffsetprobes
 * @recoil-artifact defines .text recoil:function:0x405ee0: Player::AdjustThirdPersonCameraByOffsetProbes.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.adjustthirdpersoncamerabysideprobes
 * @recoil-artifact defines .text recoil:function:0x406110: Player::AdjustThirdPersonCameraBySideProbes.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.restorethirdpersoncamerafromobstructionstate
 * @recoil-artifact defines .text recoil:function:0x4063f0: Player::RestoreThirdPersonCameraFromObstructionState.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.unbindcurrentsavestateifsingleplayer
 * @recoil-artifact defines .text recoil:function:0x406430: Player::UnbindCurrentSaveStateIfSinglePlayer
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.bindactivegamestateascurrentsavestate
 * @recoil-artifact defines .text recoil:function:0x406450: Player::BindActiveGameStateAsCurrentSaveState
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.updatecameravariantfromcamerapos
 * @recoil-artifact defines .text recoil:function:0x406470: Player::UpdateCameraVariantFromCameraPos.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.updatecameravariantfromanchor
 * @recoil-artifact defines .text recoil:function:0x406510: Player::UpdateCameraVariantFromAnchor.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.updatecameraweatherfxemittervisibility
 * @recoil-artifact defines .text recoil:function:0x406610: Player::UpdateCameraWeatherFxEmitterVisibility.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
    int shouldBeVisible = 0;
    if (isSubMode == 0) {
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
        shouldBeVisible = shouldHide == 0 ? 1 : 0;
    }

    const int isVisible = (fxElement->flags & 0x10) == 0 ? 1 : 0;
    if (isVisible != shouldBeVisible) {
        fxElement->SetVisible(shouldBeVisible);
    }

    if ((fxElement->flags & 0x10) != 0) {
        return;
    }

    HudWeatherFx *const weatherFx = (HudWeatherFx *)(fxElement);
    weatherFx->camera = g_MainCamera;
    weatherFx->activeParticleCount = zOpt::GetReplicateMode() == 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.filtercameraprobeblockinghits
 * @recoil-artifact defines .text recoil:function:0x406730: Player::FilterCameraProbeBlockingHits.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.adjustsubcamerafocusforobstruction
 * @recoil-artifact defines .text recoil:function:0x4067a0: Player::AdjustSubCameraFocusForObstruction.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\Player\player_camera.c.
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

#include "recoil/Mfc42Abi.h"

/**
 * Provider-boundary accessor for imported MFC42 CDialog metadata; this does
 * not reimplement CDialog behavior.
 */
class MfcThreeFloatCDialogMessageMapAccessor : public CDialog {
  public:
    static const AFX_MSGMAP *__stdcall GetMessageMap();
};

/**
 * Authored Recoil dialog reconstructed over imported MFC42 CDialog; MFC base
 * behavior is provided by MFC42.
 */
class MfcThreeFloatDialog : public CDialog {
  public:
    static const AFX_MSGMAP messageMap;
    static const AFX_MSGMAP_ENTRY messageEntries[];
    static const float kSpinStepPositive;
    static const float kSpinStepNegative;

    static const AFX_MSGMAP *__stdcall GetBaseMessageMapForMfc();
    const AFX_MSGMAP * GetMessageMap() const;

    void OnKillFocusValue0();
    void OnKillFocusValue1();
    void OnKillFocusValue2();
    void OnDeltaposSpinValue0(
        NMHDR *notify,
        long *result
    );
    void OnDeltaposSpinValue1(
        NMHDR *notify,
        long *result
    );
    void OnDeltaposSpinValue2(
        NMHDR *notify,
        long *result
    );
    void OnMove(
        int x,
        int y
    );
    int OnCreate(
        LPCREATESTRUCT createStruct
    );

    int unknown060;
    float value0;
    float value1;
    float value2;
};

RECOIL_STATIC_ASSERT(sizeof(CDialog) == 0x60);
RECOIL_STATIC_ASSERT(
    offsetof(
        MfcThreeFloatDialog,
        value0
    ) == 0x64
);
RECOIL_STATIC_ASSERT(
    offsetof(
        MfcThreeFloatDialog,
        value1
    ) == 0x68
);
RECOIL_STATIC_ASSERT(
    offsetof(
        MfcThreeFloatDialog,
        value2
    ) == 0x6c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        NM_UPDOWN,
        iDelta
    ) == 0x10
);

namespace {
const unsigned int kValue0EditControlId = 0x3f1;
const unsigned int kValue1EditControlId = 0x3f2;
const unsigned int kValue2EditControlId = 0x3f3;
const unsigned int kValue0SpinControlId = 0x42d;
const unsigned int kValue1SpinControlId = 0x42e;
const unsigned int kValue2SpinControlId = 0x42f;

} // namespace

/**
 * Provider-boundary: imported MFC42 CDialog message-map accessor.
 *
 * Purpose: expose the CDialog base message map for the recovered MFC map
 * chain.
 */
const AFX_MSGMAP *__stdcall MfcThreeFloatCDialogMessageMapAccessor::GetMessageMap() {
    return &CDialog::messageMap;
}

/**
 * Original helper evidence: no standalone retail function; used by the
 * MfcThreeFloatDialog MFC message-map data at 0x4ccb18.
 *
 * Purpose: return the provider CDialog base message map for MFC dispatch
 * chaining.
 */
const AFX_MSGMAP *__stdcall MfcThreeFloatDialog::GetBaseMessageMapForMfc() {
    return MfcThreeFloatCDialogMessageMapAccessor::GetMessageMap();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-messageentries
 * @recoil-artifact defines .rdata recoil:data:0x4ccb20: MfcThreeFloatDialog::messageEntries.
 *
 * Purpose: provide the terminal MFC message-map entries for the three edit
 * kill-focus handlers, three up-down delta handlers, WM_MOVE, WM_CREATE, and
 * the MFC sentinel.
 */
AFX_MSGMAP_ENTRY const MfcThreeFloatDialog::messageEntries[] = {
    {WM_COMMAND,
        EN_KILLFOCUS,
        kValue0EditControlId,
        kValue0EditControlId,
        AfxSig_vv,
        (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue0},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kValue1EditControlId,
        kValue1EditControlId,
        AfxSig_vv,
        (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue1},
    {WM_COMMAND,
        EN_KILLFOCUS,
        kValue2EditControlId,
        kValue2EditControlId,
        AfxSig_vv,
        (AFX_PMSG)&MfcThreeFloatDialog::OnKillFocusValue2},
    {WM_NOTIFY,
        (WORD)(int)UDN_DELTAPOS,
        kValue0SpinControlId,
        kValue0SpinControlId,
        AfxSig_vNMHDRpl,
        (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
            &MfcThreeFloatDialog::OnDeltaposSpinValue0},
    {WM_NOTIFY,
        (WORD)(int)UDN_DELTAPOS,
        kValue1SpinControlId,
        kValue1SpinControlId,
        AfxSig_vNMHDRpl,
        (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
            &MfcThreeFloatDialog::OnDeltaposSpinValue1},
    {WM_NOTIFY,
        (WORD)(int)UDN_DELTAPOS,
        kValue2SpinControlId,
        kValue2SpinControlId,
        AfxSig_vNMHDRpl,
        (AFX_PMSG)(void (AFX_MSG_CALL CCmdTarget::*)(NMHDR *, LRESULT *))
            &MfcThreeFloatDialog::OnDeltaposSpinValue2},
    {WM_MOVE,
        0,
        0,
        0,
        AfxSig_vvii,
        (AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(int, int))
            &MfcThreeFloatDialog::OnMove},
    {WM_CREATE,
        0,
        0,
        0,
        AfxSig_is,
        (AFX_PMSG)(AFX_PMSGW)(int (AFX_MSG_CALL CWnd::*)(LPCREATESTRUCT))
            &MfcThreeFloatDialog::OnCreate},
    {0, 0, 0, 0, AfxSig_end, 0},
};

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-messagemap
 * @recoil-artifact defines .rdata recoil:data:0x4ccb18: MfcThreeFloatDialog::messageMap.
 *
 * Purpose: link MfcThreeFloatDialog's message entries to the CDialog provider
 * message-map accessor used as the retail base-map callback.
 */
const AFX_MSGMAP MfcThreeFloatDialog::messageMap = {
    &MfcThreeFloatDialog::GetBaseMessageMapForMfc,
    &MfcThreeFloatDialog::messageEntries[0],
};

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-kspinsteppositive
 * @recoil-artifact defines .rdata recoil:data:0x4ccbf8: MfcThreeFloatDialog::kSpinStepPositive.
 *
 * Purpose: provide the recovered positive spin delta used when the up-down
 * control reports a non-positive delta.
 */
const float MfcThreeFloatDialog::kSpinStepPositive = 0.25f;

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-kspinstepnegative
 * @recoil-artifact defines .rdata recoil:data:0x4ccbfc: MfcThreeFloatDialog::kSpinStepNegative.
 *
 * Purpose: provide the recovered negative spin delta used when the up-down
 * control reports a positive delta.
 */
const float MfcThreeFloatDialog::kSpinStepNegative = -0.25f;

/**
 * Original helper evidence: no standalone retail function; used by the MFC
 * message-map vtable override for this dialog's owner.
 *
 * Purpose: return the authored dialog message-map table used by MFC command,
 * notification, and window-message dispatch.
 */
const AFX_MSGMAP * MfcThreeFloatDialog::GetMessageMap() const {
    return &MfcThreeFloatDialog::messageMap;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-onkillfocusvalue0
 * @recoil-artifact defines .text recoil:function:0x406890: MfcThreeFloatDialog::OnKillFocusValue0
 *
 * Purpose: commit edited value0 through MFC data exchange and accept the
 * dialog only when the value changed.
 */
void MfcThreeFloatDialog::OnKillFocusValue0() {
    const float oldValue = value0;
    UpdateData(TRUE);
    if (value0 != oldValue) {
        CDialog::OnOK();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-onkillfocusvalue1
 * @recoil-artifact defines .text recoil:function:0x4068c0: MfcThreeFloatDialog::OnKillFocusValue1
 *
 * Purpose: commit edited value1 through MFC data exchange and accept the
 * dialog only when the value changed.
 */
void MfcThreeFloatDialog::OnKillFocusValue1() {
    const float oldValue = value1;
    UpdateData(TRUE);
    if (value1 != oldValue) {
        CDialog::OnOK();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-onkillfocusvalue2
 * @recoil-artifact defines .text recoil:function:0x4068f0: MfcThreeFloatDialog::OnKillFocusValue2
 *
 * Purpose: commit edited value2 through MFC data exchange and accept the
 * dialog only when the value changed.
 */
void MfcThreeFloatDialog::OnKillFocusValue2() {
    const float oldValue = value2;
    UpdateData(TRUE);
    if (value2 != oldValue) {
        CDialog::OnOK();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-ondeltaposspinvalue0
 * @recoil-artifact defines .text recoil:function:0x406920: MfcThreeFloatDialog::OnDeltaposSpinValue0
 *
 * Purpose: adjust value0 by the recovered 0.25 spin step, refresh dialog data,
 * accept the value, and clear the notify result.
 */
void MfcThreeFloatDialog::OnDeltaposSpinValue0(
    NMHDR *notify,
    long *result
) {
    NM_UPDOWN *const upDown = (NM_UPDOWN *)notify;
    if (upDown->iDelta > 0) {
        value0 -= kSpinStepPositive;
    } else {
        value0 -= kSpinStepNegative;
    }

    UpdateData(FALSE);
    CDialog::OnOK();
    *result = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-ondeltaposspinvalue1
 * @recoil-artifact defines .text recoil:function:0x406960: MfcThreeFloatDialog::OnDeltaposSpinValue1
 *
 * Purpose: adjust value1 by the recovered 0.25 spin step, refresh dialog data,
 * accept the value, and clear the notify result.
 */
void MfcThreeFloatDialog::OnDeltaposSpinValue1(
    NMHDR *notify,
    long *result
) {
    NM_UPDOWN *const upDown = (NM_UPDOWN *)notify;
    if (upDown->iDelta > 0) {
        value1 -= kSpinStepPositive;
    } else {
        value1 -= kSpinStepNegative;
    }

    UpdateData(FALSE);
    CDialog::OnOK();
    *result = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-ondeltaposspinvalue2
 * @recoil-artifact defines .text recoil:function:0x4069a0: MfcThreeFloatDialog::OnDeltaposSpinValue2
 *
 * Purpose: adjust value2 by the recovered 0.25 spin step, refresh dialog data,
 * accept the value, and clear the notify result.
 */
void MfcThreeFloatDialog::OnDeltaposSpinValue2(
    NMHDR *notify,
    long *result
) {
    NM_UPDOWN *const upDown = (NM_UPDOWN *)notify;
    if (upDown->iDelta > 0) {
        value2 -= kSpinStepPositive;
    } else {
        value2 -= kSpinStepNegative;
    }

    UpdateData(FALSE);
    CDialog::OnOK();
    *result = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-onmove
 * @recoil-artifact defines .text recoil:function:0x4069e0: MfcThreeFloatDialog::OnMove
 *
 * Purpose: dispatch default MFC move handling for the dialog.
 */
void MfcThreeFloatDialog::OnMove(
    int,
    int
) {
    Default();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.mfcthreefloatdialog-oncreate
 * @recoil-artifact defines .text recoil:function:0x4069f0: MfcThreeFloatDialog::OnCreate
 *
 * Purpose: preserve the dialog creation result rule from MFC default handling,
 * returning -1 only when the provider default handler returns -1.
 */
int MfcThreeFloatDialog::OnCreate(
    LPCREATESTRUCT
) {
    return Default() == -1 ? -1 : 0;
}

namespace zStr {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.containscaseinsensitive
 * @recoil-artifact defines .text recoil:function:0x406a00: zStr::ContainsCaseInsensitive
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
 * @recoil-anchor recoil:anchor:battlesport.hud.executecommandstring
 * @recoil-artifact defines .text recoil:function:0x406af0: HudCheat::ExecuteCommandString.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.clearnanitepanelcheatsentinel
 * @recoil-artifact defines .text recoil:function:0x406cf0: HudCheat::ClearNanitePanelCheatSentinel.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
 * Original-source inline helper: no standalone retail function exists.
 * Observed caller: 0x406d20.
 * Evidence: the address-backed HudUiCheatCodeDialog constructor contains the
 * text-buffer allocation, initial update, and input-capture setup before its
 * ZRD widget bindings.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicheatcodedialog-huduicheatcodedialog
 * @recoil-artifact defines .text recoil:function:0x406d20: HudUiCheatCodeDialog::HudUiCheatCodeDialog.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecheatcode-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x406e90: RecoilStateCheatCode::StaticInitAndRegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: construct the global cheat-code state and register its atexit teardown.
 */
void RecoilStateCheatCode::StaticInitAndRegisterAtExit() {
    ConstructGlobal();
    StaticInit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecheatcode-constructglobal
 * @recoil-artifact defines .text recoil:function:0x406ea0: RecoilStateCheatCode::ConstructGlobal.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: run explicit construction for the global cheat-code app-state object.
 */
RecoilStateCheatCode *RecoilStateCheatCode::ConstructGlobal() {
    return &((&g_RecoilStateCheatCode)->RecoilStateCheatCode::RecoilStateCheatCode());
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecheatcode-staticinit
 * @recoil-artifact defines .text recoil:function:0x406eb0: RecoilStateCheatCode::StaticInit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: register the global cheat-code app-state destructor with atexit.
 */
void RecoilStateCheatCode::StaticInit() {
    atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecheatcode-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x406ec0: RecoilStateCheatCode::AtExitDestructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: destroy the global cheat-code app-state object during CRT shutdown.
 */
void RecoilStateCheatCode::AtExitDestructor() {
    (&g_RecoilStateCheatCode)->RecoilStateCheatCode::~RecoilStateCheatCode();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecheatcode-recoilstatecheatcode
 * @recoil-artifact defines .text recoil:function:0x406ed0: RecoilStateCheatCode::RecoilStateCheatCode.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: initialize the cheat-code app state and clear its dialog pointer.
 */
RecoilStateCheatCode::RecoilStateCheatCode() {
    m_dialog = 0;
}

/**
 * Original function; retail address 0x406f00.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecheatcode-ontrybecomecurrent
 * @recoil-artifact defines .text recoil:function:0x406f60: RecoilStateCheatCode::OnTryBecomeCurrent.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\RecoilStateCheatCode.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecheatcode-ondeactivate
 * @recoil-artifact defines .text recoil:function:0x407010: RecoilStateCheatCode::OnDeactivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\RecoilStateCheatCode.cpp.
 * Purpose: leave the cheat-code dialog state, restore presentation state, and execute the entered command.
 */
void RecoilStateCheatCode::OnDeactivate() {
    CString commandString;

    if (m_dialog != 0) {
        commandString =
            ((HudUiCheatCodeDialog *)m_dialog)->cheatInputWidget.GetBuffer();

        zVideo::RunPostprocessOnPrimaryBuffer();

        m_dialog->SetEnabled(0);

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
 * Original function; retail address 0x4070e0.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: queue the cheat-code state exit when the GO widget is activated.
 */
inline void HudUiCheatCodeTitleWidget::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}


/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicallback-queueexitcurrentstate
 * @recoil-artifact defines .text recoil:function:0x407100: HudUiCallback::QueueExitCurrentState.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Queue an immediate exit from the current Recoil application state.
 */
void HudUiCallback::QueueExitCurrentState() {
    g_RecoilApp.QueueExitCurrentState(0);
}

extern void (*const g_HudUiQueueExitCurrentStateCallback)() = HudUiCallback::QueueExitCurrentState;

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicallback-queuecheatcodestate
 * @recoil-artifact defines .text recoil:function:0x407110: HudUiCallback::QueueCheatCodeState.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Queue the cheat-code state and report successful callback handling.
 */
int HudUiCallback::QueueCheatCodeState() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilStateCheatCode,
        0
    );
    return 1;
}

#include "GameZRecoil/zClass/cls_stubs.h"

/**
 * Provider boundary: GameZRecoil/zClass/cls_stubs.c retail stub at 0x407130.
 * Purpose: provide a generic zClass vtable stub that returns success.
 */
int zStub::ReturnOneNoArgs() {
    return 1;
}

/**
 * Provider boundary: GameZRecoil/zClass/cls_stubs.c retail stub at 0x407140.
 * Purpose: provide a generic zClass vtable stub that returns failure or empty
 * state.
 */
int zStub::ReturnZeroNoArgs() {
    return 0;
}

/**
 * Provider boundary: GameZRecoil/zClass/cls_stubs.c retail stub at 0x407150.
 * Purpose: provide a generic one-argument zClass vtable stub with no side
 * effects.
 */
void zStub::NoOp1Arg(
    int
) {}

/**
 * Provider boundary: GameZRecoil/zClass/cls_stubs.c retail stub at 0x407160.
 * Purpose: provide a generic two-argument zClass vtable stub that returns
 * success.
 */
int zStub::ReturnOne2Args(
    int,
    int
) {
    return 1;
}
// Compiler-emitted 0x407170: VC5 scalar-deleting destructor glue for the
// byte-matched 0x4ccd50 default/base table; not an authored source-map row.
#include "Battlesport/recoil_state_base.h"

/*
 * Provisional byte-match body for the unresolved 0x407170 / 0x4ccd50 default
 * state table. These minimal virtuals are intentionally separate from
 * RecoilApp_IState default hook bodies.
 */

/**
 * Original helper evidence: the complete destructor has no standalone retail
 * body in the RecoilStateBase default-table check; this inline definition
 * feeds the compiler-emitted scalar-deleting destructor at 0x407170.
 * Purpose: preserve the matched empty destructor shape outside the public
 * header without adding a standalone complete-destructor text symbol.
 */
inline RecoilStateBase::~RecoilStateBase() {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 1 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the one-argument no-op body at
 * 0x407150; verified through recoil_state_base_default_table.
 * Purpose: Accept window activation notifications for default states.
 */
void RecoilStateBase::OnWndActivate(
    int
) {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 2 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the zero-argument no-op body at
 * 0x404e80; verified through recoil_state_base_default_table and
 * zerror_report_old_noop.
 * Purpose: Provide an empty enter callback for default states.
 */
void RecoilStateBase::OnEnter() {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 3 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the return-one body at 0x407130;
 * verified through recoil_state_base_default_table.
 * Purpose: Allow a default state transition to become current.
 */
int RecoilStateBase::OnTryBecomeCurrent() {
    return 1;
}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 4 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the return-zero body at 0x407140;
 * verified through recoil_state_base_default_table.
 * Purpose: Report that a default state does not request app shutdown.
 */
int RecoilStateBase::OnUpdateShouldQuit() {
    return 0;
}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 5 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the zero-argument no-op body at
 * 0x404e80; verified through recoil_state_base_default_table and
 * zerror_report_old_noop.
 * Purpose: Provide an empty exit callback for default states.
 */
void RecoilStateBase::OnExit() {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 6 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the zero-argument no-op body at
 * 0x404e80; verified through recoil_state_base_default_table and
 * zerror_report_old_noop.
 * Purpose: Provide an empty deactivation callback for default states.
 */
void RecoilStateBase::OnDeactivate() {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 7 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the one-argument no-op body at
 * 0x407150; verified through recoil_state_base_default_table.
 * Purpose: Accept suspend notifications for default states.
 */
void RecoilStateBase::OnSuspend(
    int
) {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 8 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the one-argument no-op body at
 * 0x407150; verified through recoil_state_base_default_table.
 * Purpose: Accept resume notifications for default states.
 */
void RecoilStateBase::OnResume(
    int
) {}

/**
 * Original helper evidence: no standalone retail function exists; vtable slot 9 in
 * g_RecoilStateBase_Vtbl @ 0x4ccd50 folds to the two-argument return-one body
 * at 0x407160; verified through recoil_state_base_default_table.
 * Purpose: Keep the default idle/dispatch loop active.
 */
int RecoilStateBase::OnIdleOrDispatch(
    unsigned int,
    unsigned int
) {
    return 1;
}
extern "C" {
/**
 * Storage group: zOpt profile and option literal pool.
 * Purpose: preserve the writable VC5-era char globals used by profile selection
 * and option registration.
 */
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-opstr-toleq
 * @recoil-artifact defines .data recoil:data:0x4da63c: g_zOpt_OpStr_TolEq.
 * Purpose: Stores the writable profile comparison token for approximate equality.
 */
char g_zOpt_OpStr_TolEq[] = "~=";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-opstr-ne
 * @recoil-artifact defines .data recoil:data:0x4da640: g_zOpt_OpStr_Ne.
 * Purpose: Stores the writable profile comparison token for inequality.
 */
char g_zOpt_OpStr_Ne[] = "!=";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-opstr-ge
 * @recoil-artifact defines .data recoil:data:0x4da644: g_zOpt_OpStr_Ge.
 * Purpose: Stores the writable profile comparison token for greater-or-equal tests.
 */
char g_zOpt_OpStr_Ge[] = ">=";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-opstr-le
 * @recoil-artifact defines .data recoil:data:0x4da648: g_zOpt_OpStr_Le.
 * Purpose: Stores the writable profile comparison token for less-or-equal tests.
 */
char g_zOpt_OpStr_Le[] = "<=";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-opstr-gt
 * @recoil-artifact defines .data recoil:data:0x4da64c: g_zOpt_OpStr_Gt.
 * Purpose: Stores the writable profile comparison token for greater-than tests.
 */
char g_zOpt_OpStr_Gt[] = ">";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-opstr-lt
 * @recoil-artifact defines .data recoil:data:0x4da650: g_zOpt_OpStr_Lt.
 * Purpose: Stores the writable profile comparison token for less-than tests.
 */
char g_zOpt_OpStr_Lt[] = "<";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-opstr-eq
 * @recoil-artifact defines .data recoil:data:0x4da654: g_zOpt_OpStr_Eq.
 * Purpose: Stores the writable profile comparison token for equality tests.
 */
char g_zOpt_OpStr_Eq[] = "==";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.k-zopt-profilemetricdefault
 * @recoil-artifact defines .data recoil:data:0x4da658: k_zOpt_ProfileMetricDefault.
 * Purpose: Stores the writable DEFAULT profile metric key.
 */
char k_zOpt_ProfileMetricDefault[] = "DEFAULT";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.k-zopt-profilemetrichwaccel
 * @recoil-artifact defines .data recoil:data:0x4da660: k_zOpt_ProfileMetricHwAccel.
 * Purpose: Stores the writable HW_ACCEL profile metric key.
 */
char k_zOpt_ProfileMetricHwAccel[] = "HW_ACCEL";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.k-zopt-profilemetricramkb
 * @recoil-artifact defines .data recoil:data:0x4da66c: k_zOpt_ProfileMetricRamKb.
 * Purpose: Stores the writable RAM_KB profile metric key.
 */
char k_zOpt_ProfileMetricRamKb[] = "RAM_KB";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.k-zopt-profilemetricvideokb
 * @recoil-artifact defines .data recoil:data:0x4da674: k_zOpt_ProfileMetricVideoKb.
 * Purpose: Stores the writable VIDEO_KB profile metric key.
 */
char k_zOpt_ProfileMetricVideoKb[] = "VIDEO_KB";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.k-zopt-profilemetriccpumhz
 * @recoil-artifact defines .data recoil:data:0x4da680: k_zOpt_ProfileMetricCpuMhz.
 * Purpose: Stores the writable CPU_MHZ profile metric key.
 */
char k_zOpt_ProfileMetricCpuMhz[] = "CPU_MHZ";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.k-zopt-profilemetriccpuclass
 * @recoil-artifact defines .data recoil:data:0x4da688: k_zOpt_ProfileMetricCpuClass.
 * Purpose: Stores the writable CPU_CLASS profile metric key.
 */
char k_zOpt_ProfileMetricCpuClass[] = "CPU_CLASS";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-vstride
 * @recoil-artifact defines .data recoil:data:0x4da694: g_zOpt_OptionName_VStride.
 * Purpose: Stores the writable option name used to register VStride.
 */
char g_zOpt_OptionName_VStride[] = "VStride";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-vmode
 * @recoil-artifact defines .data recoil:data:0x4da69c: g_zOpt_OptionName_VMode.
 * Purpose: Stores the writable option name used to register VMode.
 */
char g_zOpt_OptionName_VMode[] = "VMode";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-replicate
 * @recoil-artifact defines .data recoil:data:0x4da6a4: g_zOpt_OptionName_Replicate.
 * Purpose: Stores the writable option name used to register Replicate.
 */
char g_zOpt_OptionName_Replicate[] = "Replicate";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-window
 * @recoil-artifact defines .data recoil:data:0x4da6b0: g_zOpt_OptionName_Window.
 * Purpose: Stores the writable option name used to register Window.
 */
char g_zOpt_OptionName_Window[] = "Window";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-display
 * @recoil-artifact defines .data recoil:data:0x4da6b8: g_zOpt_OptionName_Display.
 * Purpose: Stores the writable option name used to register Display.
 */
char g_zOpt_OptionName_Display[] = "Display";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-render
 * @recoil-artifact defines .data recoil:data:0x4da6c0: g_zOpt_OptionName_Render.
 * Purpose: Stores the writable option name used to register Render.
 */
char g_zOpt_OptionName_Render[] = "Render";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-camera
 * @recoil-artifact defines .data recoil:data:0x4da6c8: g_zOpt_OptionName_Camera.
 * Purpose: Stores the writable option name used to register Camera.
 */
char g_zOpt_OptionName_Camera[] = "Camera";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-netlisten
 * @recoil-artifact defines .data recoil:data:0x4da6d0: g_zOpt_OptionName_NetListen.
 * Purpose: Stores the writable option name used to register NetListen.
 */
char g_zOpt_OptionName_NetListen[] = "NetListen";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-networkmodem
 * @recoil-artifact defines .data recoil:data:0x4da6dc: g_zOpt_OptionName_NetworkModem.
 * Purpose: Stores the writable option name used to register NetworkModem.
 */
char g_zOpt_OptionName_NetworkModem[] = "NetworkModem";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-network
 * @recoil-artifact defines .data recoil:data:0x4da6ec: g_zOpt_OptionName_Network.
 * Purpose: Stores the writable option name used to register Network.
 */
char g_zOpt_OptionName_Network[] = "Network";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-joysticknumbuttons
 * @recoil-artifact defines .data recoil:data:0x4da6f4: g_zOpt_OptionName_JoystickNumButtons.
 * Purpose: Stores the writable option name used to register JoystickNumButtons.
 */
char g_zOpt_OptionName_JoystickNumButtons[] = "JoystickNumButtons";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-joysticknumaxes
 * @recoil-artifact defines .data recoil:data:0x4da708: g_zOpt_OptionName_JoystickNumAxes.
 * Purpose: Stores the writable option name used to register JoystickNumAxes.
 */
char g_zOpt_OptionName_JoystickNumAxes[] = "JoystickNumAxes";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-wolpasswordflag
 * @recoil-artifact defines .data recoil:data:0x4da718: g_zOpt_OptionName_WOLPasswordFlag.
 * Purpose: Stores the writable option name used to register WOLPasswordFlag.
 */
char g_zOpt_OptionName_WOLPasswordFlag[] = "WOLPasswordFlag";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-joystick
 * @recoil-artifact defines .data recoil:data:0x4da728: g_zOpt_OptionName_Joystick.
 * Purpose: Stores the writable option name used to register Joystick.
 */
char g_zOpt_OptionName_Joystick[] = "Joystick";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-hwapi
 * @recoil-artifact defines .data recoil:data:0x4da734: g_zOpt_OptionName_HwApi.
 * Purpose: Stores the writable option name used to register HWAPI.
 */
char g_zOpt_OptionName_HwApi[] = "HWAPI";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-hudtypehw
 * @recoil-artifact defines .data recoil:data:0x4da73c: g_zOpt_OptionName_HudTypeHw.
 * Purpose: Stores the writable option name used to register HUDType_HW.
 */
char g_zOpt_OptionName_HudTypeHw[] = "HUDType_HW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-hudtypesw
 * @recoil-artifact defines .data recoil:data:0x4da748: g_zOpt_OptionName_HudTypeSw.
 * Purpose: Stores the writable option name used to register HUDType_SW.
 */
char g_zOpt_OptionName_HudTypeSw[] = "HUDType_SW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-hudflaghw
 * @recoil-artifact defines .data recoil:data:0x4da754: g_zOpt_OptionName_HudFlagHw.
 * Purpose: Stores the writable option name used to register HUDFlag_HW.
 */
char g_zOpt_OptionName_HudFlagHw[] = "HUDFlag_HW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-hudflagsw
 * @recoil-artifact defines .data recoil:data:0x4da760: g_zOpt_OptionName_HudFlagSw.
 * Purpose: Stores the writable option name used to register HUDFlag_SW.
 */
char g_zOpt_OptionName_HudFlagSw[] = "HUDFlag_SW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-fullscreen
 * @recoil-artifact defines .data recoil:data:0x4da76c: g_zOpt_OptionName_FullScreen.
 * Purpose: Stores the writable option name used to register FullScreen.
 */
char g_zOpt_OptionName_FullScreen[] = "FullScreen";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-cdaudio
 * @recoil-artifact defines .data recoil:data:0x4da778: g_zOpt_OptionName_CDAudio.
 * Purpose: Stores the writable option name used to register CDAudio.
 */
char g_zOpt_OptionName_CDAudio[] = "CDAudio";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-playername
 * @recoil-artifact defines .data recoil:data:0x4da780: g_zOpt_OptionName_PlayerName.
 * Purpose: Stores the writable option name used to register PlayerName.
 */
char g_zOpt_OptionName_PlayerName[] = "PlayerName";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-soundapi
 * @recoil-artifact defines .data recoil:data:0x4da78c: g_zOpt_OptionName_SoundApi.
 * Purpose: Stores the writable option name used to register SoundAPI.
 */
char g_zOpt_OptionName_SoundApi[] = "SoundAPI";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-soundlod
 * @recoil-artifact defines .data recoil:data:0x4da798: g_zOpt_OptionName_SoundLOD.
 * Purpose: Stores the writable option name used to register SoundLOD.
 */
char g_zOpt_OptionName_SoundLOD[] = "SoundLOD";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-soundvolume
 * @recoil-artifact defines .data recoil:data:0x4da7a4: g_zOpt_OptionName_SoundVolume.
 * Purpose: Stores the writable option name used to register SoundVolume.
 */
char g_zOpt_OptionName_SoundVolume[] = "SoundVolume";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-mutesound
 * @recoil-artifact defines .data recoil:data:0x4da7b0: g_zOpt_OptionName_MuteSound.
 * Purpose: Stores the writable option name used to register MuteSound.
 */
char g_zOpt_OptionName_MuteSound[] = "MuteSound";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-gameintensity
 * @recoil-artifact defines .data recoil:data:0x4da7bc: g_zOpt_OptionName_GameIntensity.
 * Purpose: Stores the writable option name used to register GameIntensity.
 */
char g_zOpt_OptionName_GameIntensity[] = "GameIntensity";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-gamectloptions
 * @recoil-artifact defines .data recoil:data:0x4da7cc: g_zOpt_OptionName_GameCtlOptions.
 * Purpose: Stores the writable option name used to register GameCtlOptions.
 */
char g_zOpt_OptionName_GameCtlOptions[] = "GameCtlOptions";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-texturememoryhw
 * @recoil-artifact defines .data recoil:data:0x4da7dc: g_zOpt_OptionName_TextureMemoryHw.
 * Purpose: Stores the writable option name used to register TextureMemory_HW.
 */
char g_zOpt_OptionName_TextureMemoryHw[] = "TextureMemory_HW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-texturememorysw
 * @recoil-artifact defines .data recoil:data:0x4da7f0: g_zOpt_OptionName_TextureMemorySw.
 * Purpose: Stores the writable option name used to register TextureMemory_SW.
 */
char g_zOpt_OptionName_TextureMemorySw[] = "TextureMemory_SW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-objectlodhw
 * @recoil-artifact defines .data recoil:data:0x4da804: g_zOpt_OptionName_ObjectLODHw.
 * Purpose: Stores the writable option name used to register ObjectLOD_HW.
 */
char g_zOpt_OptionName_ObjectLODHw[] = "ObjectLOD_HW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-objectlodsw
 * @recoil-artifact defines .data recoil:data:0x4da814: g_zOpt_OptionName_ObjectLODSw.
 * Purpose: Stores the writable option name used to register ObjectLOD_SW.
 */
char g_zOpt_OptionName_ObjectLODSw[] = "ObjectLOD_SW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-globallighthw
 * @recoil-artifact defines .data recoil:data:0x4da824: g_zOpt_OptionName_GlobalLightHw.
 * Purpose: Stores the writable option name used to register GlobalLight_HW.
 */
char g_zOpt_OptionName_GlobalLightHw[] = "GlobalLight_HW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-gfxflagshw
 * @recoil-artifact defines .data recoil:data:0x4da834: g_zOpt_OptionName_GfxFlagsHw.
 * Purpose: Stores the writable option name used to register GfxFlags_HW.
 */
char g_zOpt_OptionName_GfxFlagsHw[] = "GfxFlags_HW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-allvideobuffer
 * @recoil-artifact defines .data recoil:data:0x4da840: g_zOpt_OptionName_AllVideoBuffer.
 * Purpose: Stores the writable option name used to register AllVideoBuffer.
 */
char g_zOpt_OptionName_AllVideoBuffer[] = "AllVideoBuffer";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-globallightsw
 * @recoil-artifact defines .data recoil:data:0x4da850: g_zOpt_OptionName_GlobalLightSw.
 * Purpose: Stores the writable option name used to register GlobalLight_SW.
 */
char g_zOpt_OptionName_GlobalLightSw[] = "GlobalLight_SW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-perspective
 * @recoil-artifact defines .data recoil:data:0x4da860: g_zOpt_OptionName_Perspective.
 * Purpose: Stores the writable option name used to register Perspective.
 */
char g_zOpt_OptionName_Perspective[] = "Perspective";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-lighting
 * @recoil-artifact defines .data recoil:data:0x4da86c: g_zOpt_OptionName_Lighting.
 * Purpose: Stores the writable option name used to register Lighting.
 */
char g_zOpt_OptionName_Lighting[] = "Lighting";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-transparency
 * @recoil-artifact defines .data recoil:data:0x4da878: g_zOpt_OptionName_Transparency.
 * Purpose: Stores the writable option name used to register Transparency.
 */
char g_zOpt_OptionName_Transparency[] = "Transparency";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-gfxflagssw
 * @recoil-artifact defines .data recoil:data:0x4da888: g_zOpt_OptionName_GfxFlagsSw.
 * Purpose: Stores the writable option name used to register GfxFlags_SW.
 */
char g_zOpt_OptionName_GfxFlagsSw[] = "GfxFlags_SW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-effectslevelhw
 * @recoil-artifact defines .data recoil:data:0x4da894: g_zOpt_OptionName_EffectsLevelHw.
 * Purpose: Stores the writable option name used to register EffectsLevel_HW.
 */
char g_zOpt_OptionName_EffectsLevelHw[] = "EffectsLevel_HW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-effectslevelsw
 * @recoil-artifact defines .data recoil:data:0x4da8a4: g_zOpt_OptionName_EffectsLevelSw.
 * Purpose: Stores the writable option name used to register EffectsLevel_SW.
 */
char g_zOpt_OptionName_EffectsLevelSw[] = "EffectsLevel_SW";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-optionname-hwcardflag
 * @recoil-artifact defines .data recoil:data:0x4da8b4: g_zOpt_OptionName_HwCardFlag.
 * Purpose: Stores the writable option name used to register HWCardFlag.
 */
char g_zOpt_OptionName_HwCardFlag[] = "HWCardFlag";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-detailarchivename
 * @recoil-artifact defines .data recoil:data:0x4da8c0: g_zOpt_DetailArchiveName.
 * Purpose: Stores the writable detail archive name used by game option loading.
 */
char g_zOpt_DetailArchiveName[] = "detail.zrd";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zopt-detailoptionname-sunlight
 * @recoil-artifact defines .data recoil:data:0x4da8cc: g_zOpt_DetailOptionName_Sunlight.
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

/**
 * Source helper data: no standalone retail data artifact is assigned.
 * Evidence: address-backed caller 0x407190 performs the named scalar lookup.
 * Purpose: map the option parser's symbolic scalar names to integer values.
 */
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

} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.lookupnamedvalueasint
 * @recoil-artifact defines .text recoil:function:0x407190: zOpt::LookupNamedValueAsInt.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zopt.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.readscalarvalueasint
 * @recoil-artifact defines .text recoil:function:0x4071f0: zOpt::ReadScalarValueAsInt.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zopt.c.
 * Purpose: coerce an integer, float, or named string scalar node into an integer value.
 */
int __fastcall ReadScalarValueAsInt(
    zReader::Node *scalarValueNode
) {
    switch (scalarValueNode->type) {
    case zReader::ZRDR_NODE_INT:
        return scalarValueNode->value.i32;

    case zReader::ZRDR_NODE_FLOAT:
        return (int)(scalarValueNode->value.f32);

    case zReader::ZRDR_NODE_STRING:
        return LookupNamedValueAsInt(scalarValueNode->value.str);

    default:
        return 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.evalintcompareop
 * @recoil-artifact defines .text recoil:function:0x407220: zOpt::EvalIntCompareOp.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zopt.c.
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
        return (double)(abs(lhs - rhs)) < (double)(lhs)*ZOPT_COMPARE_TOLERANCE_PCT;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.evaluateprofilemetriccondition
 * @recoil-artifact defines .text recoil:function:0x407470: zOpt::EvaluateProfileMetricCondition.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zopt.c.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.selectprofilevalueforsystem
 * @recoil-artifact defines .text recoil:function:0x407680: zOpt::SelectProfileValueForSystem.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zopt.c.
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

/**
 * Original-source helper evidence: no standalone retail function exists.
 * Observed in caller 0x407700 from repeated option-entry pointer casts in option loading.
 * Purpose: return an option entry as the typed option-value pointer stored by zOpt globals.
 */
template <typename T>
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
inline int BuildGraphicsFlags(
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

} // namespace

/**
 * Provider boundary: empty zGame compatibility stub at retail 0x4076f0.
 * Purpose: preserve the empty zGame stub used by the option/load cluster.
 */
void __cdecl ReturnOnlyStub() {}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.options-loadgameoptions
 * @recoil-artifact defines .text recoil:function:0x407700: zGame::Options_LoadGameOptions.
 * Purpose: load detail.zrd and register the game option globals.
 */
RECOIL_NO_GS int Options_LoadGameOptions() {
    memset(
        &g_zGame_Options_PointerCache,
        0,
        sizeof(g_zGame_Options_PointerCache)
    );

    zReader::Node *const detailRoot = zReader::LoadNodeFromPath(
        g_zOpt_DetailArchiveName,
        0,
        0
    );
    if (detailRoot == 0) {
        return 0;
    }

    g_zGame_Options_RuntimeConfig.CopyDefault();

    g_zGame_Options_PointerCache.videoAcceleration = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HwCardFlag,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.videoAcceleration != 0) {
        zVid::SetAccelerationOption(ZVID_HW_MODE_HARDWARE);
    }

    g_zGame_Options_PointerCache.effectsLevelSw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_EffectsLevelSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.effectsLevelSw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetEffectsLevelForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_EffectsLevelSw, 1)
        );
    }

    g_zGame_Options_PointerCache.effectsLevelHw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_EffectsLevelHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.effectsLevelHw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetEffectsLevelForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_EffectsLevelHw, 0)
        );
    }

    g_zGame_Options_PointerCache.gfxFlagsSw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GfxFlagsSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.gfxFlagsSw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetGraphicsFlagsForCurrentHwMode(BuildGraphicsFlags(
            detailRoot,
            g_zOpt_OptionName_GlobalLightSw,
            0
        ));
    }

    g_zGame_Options_PointerCache.gfxFlagsHw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GfxFlagsHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.gfxFlagsHw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetGraphicsFlagsForCurrentHwMode(BuildGraphicsFlags(
            detailRoot,
            g_zOpt_OptionName_GlobalLightHw,
            1
        ));
    }

    g_zGame_Options_PointerCache.objectLodSw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_ObjectLODSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.objectLodSw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetObjectLODForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_ObjectLODSw, 0)
        );
    }

    g_zGame_Options_PointerCache.objectLodHw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_ObjectLODHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.objectLodHw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetObjectLODForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_ObjectLODHw, 0)
        );
    }

    g_zGame_Options_PointerCache.textureMemorySw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_TextureMemorySw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.textureMemorySw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetTextureMemoryForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_TextureMemorySw, 0)
        );
    }

    g_zGame_Options_PointerCache.textureMemoryHw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_TextureMemoryHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.textureMemoryHw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetTextureMemoryForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_TextureMemoryHw, 0)
        );
    }

    g_zGame_Options_PointerCache.gameControlOptions = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GameCtlOptions,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.gameControlOptions != 0) {
        zOpt::SetGameControlOptions(ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON);
    }

    g_zGame_Options_PointerCache.gameDifficulty = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_GameIntensity,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.gameDifficulty != 0) {
        zOpt::SetGameDifficultyMode(1);
    }

    g_zGame_Options_PointerCache.muteSound = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_MuteSound,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.muteSound != 0) {
        zOpt::SetMuteSoundOption(0);
    }

    g_zGame_Options_PointerCache.soundVolume = OptionValuePointer<float>(Options_GetOrCreateOption(
        g_zOpt_OptionName_SoundVolume,
        ZGAME_OPTION_INLINE_BINARY4,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.soundVolume != 0) {
        zOpt::SetSoundVolumeOption(1.0f);
    }

    g_zGame_Options_PointerCache.soundLod = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_SoundLOD, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (g_zGame_Options_PointerCache.soundLod != 0) {
        zOpt::SetSoundLODOption(zOpt::SelectProfileValueForSystem(
            detailRoot,
            g_zOpt_OptionName_SoundLOD,
            0
        ));
    }

    g_zGame_Options_PointerCache.audioApi = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_SoundApi, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (g_zGame_Options_PointerCache.audioApi != 0) {
        zSnd::SetAudioApiOption(1);
    }

    g_zGame_Options_PointerCache.playerName = Options_GetOrCreateOption(
        g_zOpt_OptionName_PlayerName,
        ZGAME_OPTION_STRING_BUFFER,
        0x16,
        ZGAME_OPTION_SCOPE_USER
    );
    if (g_zGame_Options_PointerCache.playerName != 0) {
        DWORD userNameSize = 0xfe;
        char userName[0x100];
        GetUserNameA(
            userName,
            &userNameSize
        );
        userName[userNameSize] = '\0';
        zOpt::SetPlayerName(userName);
    }

    g_zGame_Options_PointerCache.cdAudio = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_CDAudio, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (g_zGame_Options_PointerCache.cdAudio != 0) {
        zSnd::SetCDAudioOption(1);
    }

    g_zGame_Options_PointerCache.videoFullscreen = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_FullScreen,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.videoFullscreen != 0) {
        zOpt::SetFullscreenOption(1);
    }

    g_zGame_Options_PointerCache.hudVisibilitySw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudFlagSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.hudVisibilitySw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetHudVisibilityOption(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudFlagSw, 1)
        );
    }

    g_zGame_Options_PointerCache.hudVisibilityHw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudFlagHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.hudVisibilityHw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetHudVisibilityOption(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudFlagHw, 1)
        );
    }

    g_zGame_Options_PointerCache.hudTypeSw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudTypeSw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.hudTypeSw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_SOFTWARE;
        zOpt::SetHudTypeForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudTypeSw, 1)
        );
    }

    g_zGame_Options_PointerCache.hudTypeHw = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_HudTypeHw,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.hudTypeHw != 0) {
        g_zOpt_HwMode = ZVID_HW_MODE_HARDWARE;
        zOpt::SetHudTypeForCurrentHwMode(
            zOpt::SelectProfileValueForSystem(detailRoot, g_zOpt_OptionName_HudTypeHw, 1)
        );
    }

    g_zGame_Options_PointerCache.hardwareApi = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_HwApi, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (g_zGame_Options_PointerCache.hardwareApi != 0) {
        zVid::SetHwApiOption(1);
    }

    g_zGame_Options_PointerCache.inputJoystick = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_Joystick, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (g_zGame_Options_PointerCache.inputJoystick != 0) {
        zInp::SetJoystickOption(0);
    }

    g_zGame_Options_PointerCache.wolPasswordFlag = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_WOLPasswordFlag,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_USER
    ));
    if (g_zGame_Options_PointerCache.wolPasswordFlag != 0) {
        zOpt::SetWolPasswordFlag(1);
    }

    g_zGame_Options_PointerCache.joystickNumAxes = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_JoystickNumAxes,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zGame_Options_PointerCache.joystickNumAxes != 0) {
        zInp::SetJoystickAxesCountOption(0);
    }

    g_zGame_Options_PointerCache.joystickNumButtons = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_JoystickNumButtons,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zGame_Options_PointerCache.joystickNumButtons != 0) {
        zInp::SetJoystickButtonCountOption(0);
    }

    g_zGame_Options_PointerCache.networkEnabled = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Network,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zGame_Options_PointerCache.networkEnabled != 0) {
        zOpt::SetNetworkEnabled(0);
    }

    g_zGame_Options_PointerCache.networkModem = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_NetworkModem,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zGame_Options_PointerCache.networkModem != 0) {
        zOpt::SetNetworkModemEnabled(0);
    }

    g_zGame_Options_PointerCache.networkListen = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_NetListen,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zGame_Options_PointerCache.networkListen != 0) {
        zOpt::SetNetworkListenEnabled(0);
    }

    g_zGame_Options_PointerCache.cameraSection = OptionValuePointer<zOpt_CameraSection *>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Camera,
        ZGAME_OPTION_HEAP_BUFFER,
        0x0c,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    g_zGame_Options_PointerCache.renderSection =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Render,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    g_zGame_Options_PointerCache.displaySection =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Display,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    g_zGame_Options_PointerCache.windowSection =
        OptionValuePointer<zOpt_ViewRectSection *>(Options_GetOrCreateOption(
            g_zOpt_OptionName_Window,
            ZGAME_OPTION_HEAP_BUFFER,
            0x28,
            ZGAME_OPTION_SCOPE_TRANSIENT
        ));
    g_zGame_Options_PointerCache.replicate = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_Replicate,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));

    g_zGame_Options_PointerCache.videoMode = OptionValuePointer<int>(
        Options_GetOrCreateOption(g_zOpt_OptionName_VMode, ZGAME_OPTION_INLINE_DWORD, 0, ZGAME_OPTION_SCOPE_USER)
    );
    if (g_zGame_Options_PointerCache.videoMode != 0) {
        zVid::SetVideoModeIndex(zOpt::SelectProfileValueForSystem(
            detailRoot,
            g_zOpt_OptionName_VMode,
            5
        ));
    }

    g_zGame_Options_PointerCache.videoStride = OptionValuePointer<int>(Options_GetOrCreateOption(
        g_zOpt_OptionName_VStride,
        ZGAME_OPTION_INLINE_DWORD,
        0,
        ZGAME_OPTION_SCOPE_TRANSIENT
    ));
    if (g_zGame_Options_PointerCache.videoStride != 0) {
        *g_zGame_Options_PointerCache.videoStride = 1;
    }

    zInput::BindMap_InitDefaultBindings();
    Options_LoadFromRegistry();
    zInput::BindMap_Current_RebuildLookupIndices();
    zOpt::SetNetworkEnabled(0);
    zOpt::SetNetworkModemEnabled(0);

    if (g_zGame_Options_PointerCache.cameraSection != 0 && *g_zGame_Options_PointerCache.cameraSection != 0) {
        (*g_zGame_Options_PointerCache.cameraSection)->m_pCamera = 0;
    }
    if (g_zGame_Options_PointerCache.renderSection != 0 && *g_zGame_Options_PointerCache.renderSection != 0) {
        (*g_zGame_Options_PointerCache.renderSection)->target = 0;
    }
    if (g_zGame_Options_PointerCache.displaySection != 0 && *g_zGame_Options_PointerCache.displaySection != 0) {
        (*g_zGame_Options_PointerCache.displaySection)->target = 0;
    }
    if (g_zGame_Options_PointerCache.windowSection != 0 && *g_zGame_Options_PointerCache.windowSection != 0) {
        (*g_zGame_Options_PointerCache.windowSection)->target = 0;
    }

    zReader::FreeLoadedTree(detailRoot);
    g_zOpt_HwMode = zVid::GetAccelerationOption();
    zSnd::SetAudioApiOption(zSnd::GetAudioApiOption());
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.options-savegameoptions
 * @recoil-artifact defines .text recoil:function:0x407e00: zGame::Options_SaveGameOptions.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.setgamecontroloptions
 * @recoil-artifact defines .text recoil:function:0x407e20: zOpt::SetGameControlOptions.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: replace the packed game-control option bitmask.
 */
void __fastcall SetGameControlOptions(
    zOptGameControlFlags value
) {
    *g_zGame_Options_PointerCache.gameControlOptions = value;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setthrottlemode
 * @recoil-artifact defines .text recoil:function:0x407e30: zOpt::SetThrottleMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the throttle-control bit in the game-control option mask.
 */
void __fastcall SetThrottleMode(
    int enable
) {
    if (enable != 0) {
        *g_zGame_Options_PointerCache.gameControlOptions |= ZOPT_GAME_CONTROL_THROTTLE;
    } else {
        *g_zGame_Options_PointerCache.gameControlOptions &= ~ZOPT_GAME_CONTROL_THROTTLE;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getthrottlemode
 * @recoil-artifact defines .text recoil:function:0x407e50: zOpt::GetThrottleMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the throttle-control bit from the game-control option mask.
 */
int GetThrottleMode() {
    return *g_zGame_Options_PointerCache.gameControlOptions & ZOPT_GAME_CONTROL_THROTTLE;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setsteeringmode
 * @recoil-artifact defines .text recoil:function:0x407e60: zOpt::SetSteeringMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the steering-control bit in the game-control option mask.
 */
void __fastcall SetSteeringMode(
    int enable
) {
    if (enable != 0) {
        *g_zGame_Options_PointerCache.gameControlOptions |= ZOPT_GAME_CONTROL_STEERING;
    } else {
        *g_zGame_Options_PointerCache.gameControlOptions &= ~ZOPT_GAME_CONTROL_STEERING;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getsteeringmode
 * @recoil-artifact defines .text recoil:function:0x407e80: zOpt::GetSteeringMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the steering-control bit from the game-control option mask.
 */
int GetSteeringMode() {
    return (*g_zGame_Options_PointerCache.gameControlOptions >> 1) & 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setcursormode
 * @recoil-artifact defines .text recoil:function:0x407e90: zOpt::SetCursorMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: set or clear the cursor-control bit in the game-control option mask.
 */
void __fastcall SetCursorMode(
    int enable
) {
    if (enable != 0) {
        *g_zGame_Options_PointerCache.gameControlOptions |= ZOPT_GAME_CONTROL_CURSOR;
    } else {
        *g_zGame_Options_PointerCache.gameControlOptions &= ~ZOPT_GAME_CONTROL_CURSOR;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getcursormode
 * @recoil-artifact defines .text recoil:function:0x407eb0: zOpt::GetCursorMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the cursor-control bit from the game-control option mask.
 */
int GetCursorMode() {
    return (*g_zGame_Options_PointerCache.gameControlOptions >> 2) & 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setcameramode
 * @recoil-artifact defines .text recoil:function:0x407ec0: zOpt::SetCameraMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: store first-person or third-person camera mode and apply the player camera state.
 */
void __fastcall SetCameraMode(
    int enableThirdPerson
) {
    if (enableThirdPerson != 0) {
        *g_zGame_Options_PointerCache.gameControlOptions |= ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON;
        Player::ApplyCameraState(1);
    } else {
        *g_zGame_Options_PointerCache.gameControlOptions &= ~ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON;
        Player::ApplyCameraState(3);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getcameramodeplayerstate
 * @recoil-artifact defines .text recoil:function:0x407ef0: zOpt::GetCameraModeAsPlayerCameraState.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: map the third-person camera option bit to the player camera state value.
 */
int GetCameraModePlayerState() {
    return ((~*g_zGame_Options_PointerCache.gameControlOptions & ZOPT_GAME_CONTROL_CAMERA_THIRD_PERSON) | 4) >> 2;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setgamedifficultymode
 * @recoil-artifact defines .text recoil:function:0x407f10: zOpt::SetGameDifficultyMode.
 * Purpose: Store the current game difficulty option value.
 */
void __fastcall SetGameDifficultyMode(
    int value
) {
    *g_zGame_Options_PointerCache.gameDifficulty = value;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getgamedifficultymode
 * @recoil-artifact defines .text recoil:function:0x407f20: zOpt::GetGameDifficultyMode.
 * Purpose: Return the current game difficulty option value.
 */
int GetGameDifficultyMode() {
    return *g_zGame_Options_PointerCache.gameDifficulty;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.seteffectslevelforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x407f30: zOpt::SetEffectsLevelForCurrentHwMode.
 * Purpose: store the active hardware-mode effects level and apply the matching conditional effect level.
 */
void __fastcall SetEffectsLevelForCurrentHwMode(
    int level
) {
    *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.effectsLevelHw : g_zGame_Options_PointerCache.effectsLevelSw) = level;

    if (level == 0) {
        zEffect::SetConditionalEffectLevel(2);
    } else if (level == 1) {
        zEffect::SetConditionalEffectLevel(1);
    } else if (level == 2) {
        zEffect::SetConditionalEffectLevel(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.geteffectslevelforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x407f80: zOpt::GetEffectsLevelForCurrentHwMode.
 * Purpose: return the effects level stored for the active hardware mode.
 */
int GetEffectsLevelForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.effectsLevelHw : g_zGame_Options_PointerCache.effectsLevelSw);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setobjectlodforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x407fa0: zOpt::SetObjectLODForCurrentHwMode.
 * Purpose: store the object LOD value for the active hardware mode and apply its camera clip distance.
 */
void __fastcall SetObjectLODForCurrentHwMode(
    int level
) {
    zClass_NodePartial *const camera = zOpt_CameraSection_GetActiveCamera();
    *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.objectLodHw : g_zGame_Options_PointerCache.objectLodSw) = level;

    if (camera == 0) {
        return;
    }

    float clipDistance = 1.0f;
    switch (level) {
    case 0:
        zClass_Camera::gwCameraSetClipDistance(
            camera,
            clipDistance
        );
        break;

    case 1:
        clipDistance = 0.75f;
        zClass_Camera::gwCameraSetClipDistance(
            camera,
            clipDistance
        );
        break;

    case 2:
        clipDistance = 0.5f;
        zClass_Camera::gwCameraSetClipDistance(
            camera,
            clipDistance
        );
        break;

    default:
        zClass_Camera::gwCameraSetClipDistance(
            camera,
            clipDistance
        );
        break;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getobjectlodforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x408030: zOpt::GetObjectLODForCurrentHwMode.
 * Purpose: return the object LOD value for the active hardware mode.
 */
int GetObjectLODForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.objectLodHw : g_zGame_Options_PointerCache.objectLodSw);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setmutesoundoption
 * @recoil-artifact defines .text recoil:function:0x408050: zOpt::SetMuteSoundOption.
 * Purpose: store the mute-sound option and apply it to active sound voices.
 */
void __fastcall SetMuteSoundOption(
    int value
) {
    *g_zGame_Options_PointerCache.muteSound = value;
    zSnd::ApplyMuteStateToActiveVoices(value);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getmutesoundoption
 * @recoil-artifact defines .text recoil:function:0x408060: zOpt::GetMuteSoundOption.
 * Purpose: return the current mute-sound option value.
 */
int GetMuteSoundOption() {
    return *g_zGame_Options_PointerCache.muteSound;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setsoundvolumeoption
 * @recoil-artifact defines .text recoil:function:0x408070: zOpt::SetSoundVolumeOption.
 * Purpose: store the sound-volume option and apply the global sound scale.
 */
void __fastcall SetSoundVolumeOption(
    float volume
) {
    *g_zGame_Options_PointerCache.soundVolume = volume;
    zSnd::SetGlobalVolumeScale(volume);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getsoundvolumeoption
 * @recoil-artifact defines .text recoil:function:0x408090: zOpt::GetSoundVolumeOption.
 * Purpose: return the current sound-volume option value.
 */
float GetSoundVolumeOption() {
    return *g_zGame_Options_PointerCache.soundVolume;
}

} // namespace zOpt
namespace zSnd {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setaudioapioption
 * @recoil-artifact defines .text recoil:function:0x4080a0: zSnd::SetAudioApiOption.
 * Purpose: Store the selected audio backend option and mirror it into the pre-init backend state.
 */
int __fastcall SetAudioApiOption(
    int apiType
) {
    *g_zGame_Options_PointerCache.audioApi = apiType;
    return SetActiveBackendPreInit(apiType);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getaudioapioption
 * @recoil-artifact defines .text recoil:function:0x4080b0: zSnd::GetAudioApiOption.
 * Purpose: Return the selected audio backend option value.
 */
int GetAudioApiOption() {
    return *g_zGame_Options_PointerCache.audioApi;
}

} // namespace zSnd
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setsoundlodoption
 * @recoil-artifact defines .text recoil:function:0x4080c0: zOpt::SetSoundLODOption.
 * Purpose: store the sound LOD option value.
 */
void __fastcall SetSoundLODOption(
    int value
) {
    *g_zGame_Options_PointerCache.soundLod = value;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getsoundlodoption
 * @recoil-artifact defines .text recoil:function:0x4080d0: zOpt::GetSoundLODOption.
 * Purpose: return the current sound LOD option value.
 */
int GetSoundLODOption() {
    return *g_zGame_Options_PointerCache.soundLod;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.settexturememoryforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x4080e0: zOpt::SetTextureMemoryForCurrentHwMode.
 * Purpose: store the texture memory value for the active hardware mode.
 */
void __fastcall SetTextureMemoryForCurrentHwMode(
    int value
) {
    *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.textureMemoryHw : g_zGame_Options_PointerCache.textureMemorySw) = value;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.gettexturememoryforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x408100: zOpt::GetTextureMemoryForCurrentHwMode.
 * Purpose: return the texture memory value for the active hardware mode.
 */
int GetTextureMemoryForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.textureMemoryHw : g_zGame_Options_PointerCache.textureMemorySw);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setplayername
 * @recoil-artifact defines .text recoil:function:0x408120: zOpt::SetPlayerName.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: copy the supplied player name into the configured option buffer.
 */
void __fastcall SetPlayerName(
    const char *name
) {
    char *const buffer = (char *)(g_zGame_Options_PointerCache.playerName->payloadOrBuffer);
    const unsigned int dataSize = (unsigned int)(g_zGame_Options_PointerCache.playerName->dataSize);
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
 * @recoil-anchor recoil:anchor:battlesport.hud.zopt-getplayername
 * @recoil-artifact defines .text recoil:function:0x408190: zOpt::GetPlayerName.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the configured player-name option buffer.
 */
char *zOpt_GetPlayerName() {
    return (char *)(g_zGame_Options_PointerCache.playerName->payloadOrBuffer);
}
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setgraphicsflagsforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x4081a0: zOpt::SetGraphicsFlagsForCurrentHwMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: store the graphics option bitmask for the active hardware mode and
 * mirror its lighting bit to the sunlight node.
 */
void __fastcall SetGraphicsFlagsForCurrentHwMode(
    int flags
) {
    *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.gfxFlagsHw : g_zGame_Options_PointerCache.gfxFlagsSw) = flags;

    zClass_NodePartial *const sunlight = zClass::FindByTypeAndName(
        6,
        g_zOpt_DetailOptionName_Sunlight
    );
    if (sunlight != 0) {
        if ((flags & 0x10) != 0) {
            zClass_Class::gwNodeSetActive(
                sunlight,
                1
            );
        } else {
            zClass_Class::gwNodeSetActive(
                sunlight,
                0
            );
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getgraphicsflagsforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x4081f0: zOpt::GetGraphicsFlagsForCurrentHwMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame_Options.cpp.
 * Purpose: return the graphics option bitmask for the active hardware mode.
 */
int GetGraphicsFlagsForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.gfxFlagsHw : g_zGame_Options_PointerCache.gfxFlagsSw);
}

} // namespace zOpt
namespace zSnd {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setcdaudiooption
 * @recoil-artifact defines .text recoil:function:0x408210: zSnd::SetCDAudioOption
 * Purpose: store the CD-audio option value used by sound and options code.
 */
void __fastcall SetCDAudioOption(
    int cdAudioOption
) {
    *g_zGame_Options_PointerCache.cdAudio = cdAudioOption;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getcdaudiooption
 * @recoil-artifact defines .text recoil:function:0x408220: zSnd::GetCDAudioOption
 * Purpose: return the current CD-audio option value.
 */
int GetCDAudioOption() {
    return *g_zGame_Options_PointerCache.cdAudio;
}

} // namespace zSnd
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setnetworkenabled
 * @recoil-artifact defines .text recoil:function:0x408230: zOpt::SetNetworkEnabled.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-enabled option value through its option pointer.
 */
void __fastcall SetNetworkEnabled(
    int value
) {
    *g_zGame_Options_PointerCache.networkEnabled = value;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setnetworkmodemenabled
 * @recoil-artifact defines .text recoil:function:0x408240: zOpt::SetNetworkModemEnabled.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-modem option value through its option pointer.
 */
void __fastcall SetNetworkModemEnabled(
    int value
) {
    *g_zGame_Options_PointerCache.networkModem = value;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setnetworklistenenabled
 * @recoil-artifact defines .text recoil:function:0x408250: zOpt::SetNetworkListenEnabled.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: store the network-listen option value through its option pointer.
 */
void __fastcall SetNetworkListenEnabled(
    int value
) {
    *g_zGame_Options_PointerCache.networkListen = value;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getnetworkenabled
 * @recoil-artifact defines .text recoil:function:0x408260: zOpt::GetNetworkEnabled.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: return the network-enabled option value through its option pointer.
 */
int GetNetworkEnabled() {
    return *g_zGame_Options_PointerCache.networkEnabled;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getnetworkmodemenabled
 * @recoil-artifact defines .text recoil:function:0x408270: zOpt::GetNetworkModemEnabled.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\zOpt.cpp.
 * Purpose: return the network-modem option value through its option pointer.
 */
int GetNetworkModemEnabled() {
    return *g_zGame_Options_PointerCache.networkModem;
}

} // namespace zOpt
namespace zVid {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setaccelerationoption
 * @recoil-artifact defines .text recoil:function:0x408280: zVid::SetAccelerationOption.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: store the selected video acceleration option and mirror the active
 * hardware-mode option used by zOpt accessors.
 *
 * Evidence: BN writes ecx through g_zGame_Options_PointerCache.videoAcceleration, then stores the same
 * value into g_zOpt_HwMode; VC5SP3 zvid_option_getters byte verification is
 * exact after relocation masking.
 */
void __fastcall SetAccelerationOption(
    int accelerationOption
) {
    *g_zGame_Options_PointerCache.videoAcceleration = accelerationOption;
    g_zOpt_HwMode = accelerationOption;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.sethwapioption
 * @recoil-artifact defines .text recoil:function:0x408290: zVid::SetHwApiOption.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: store the selected hardware API/backend option.
 *
 * Evidence: BN writes ecx through g_zGame_Options_PointerCache.hardwareApi and returns without touching
 * other state; VC5SP3 zvid_option_getters byte verification is exact after
 * relocation masking.
 */
void __fastcall SetHwApiOption(
    int hwApiOption
) {
    *g_zGame_Options_PointerCache.hardwareApi = hwApiOption;
}

} // namespace zVid
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setfullscreenoption
 * @recoil-artifact defines .text recoil:function:0x4082a0: zOpt::SetFullscreenOption.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: store the persisted fullscreen/windowed option value.
 */
void __fastcall SetFullscreenOption(
    int fullscreenOption
) {
    *g_zGame_Options_PointerCache.videoFullscreen = fullscreenOption;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.sethudvisibilityoption
 * @recoil-artifact defines .text recoil:function:0x4082b0: zOpt::SetHudVisibilityOption.
 * Purpose: store the HUD visibility option for the active hardware mode.
 */
void __fastcall SetHudVisibilityOption(
    int hudVisibility
) {
    *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.hudVisibilityHw : g_zGame_Options_PointerCache.hudVisibilitySw) = hudVisibility;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.sethudtypeforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x4082d0: zOpt::SetHudTypeForCurrentHwMode.
 * Purpose: apply the requested HUD layout mode and store it for the active hardware mode.
 */
int __fastcall SetHudTypeForCurrentHwMode(
    int hudType
) {
    const int previous = HudUiMgr::ApplyHudModeSwitch(hudType);

    if (g_zOpt_HwMode != 0) {
        *g_zGame_Options_PointerCache.hudTypeHw = hudType;
        return previous;
    }

    *g_zGame_Options_PointerCache.hudTypeSw = hudType;
    return previous;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setreplicatemode
 * @recoil-artifact defines .text recoil:function:0x408300: zOpt::SetReplicateMode.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: store the active video replicate-mode option.
 *
 * Evidence: BN writes ecx through g_zGame_Options_PointerCache.replicate and returns; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall SetReplicateMode(
    int replicateMode
) {
    *g_zGame_Options_PointerCache.replicate = replicateMode;
}

} // namespace zOpt
namespace zVid {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getaccelerationoption
 * @recoil-artifact defines .text recoil:function:0x408310: zVid::GetAccelerationOption.
 * Purpose: provide the recovered zVid::GetAccelerationOption behavior.
 */
int GetAccelerationOption() {
    return *g_zGame_Options_PointerCache.videoAcceleration;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.gethwapioption
 * @recoil-artifact defines .text recoil:function:0x408320: zVid::GetHwApiOption.
 * Purpose: provide the recovered zVid::GetHwApiOption behavior.
 */
int GetHwApiOption() {
    return *g_zGame_Options_PointerCache.hardwareApi;
}

} // namespace zVid
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getfullscreenoption
 * @recoil-artifact defines .text recoil:function:0x408330: zOpt::GetFullscreenOption.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the persisted fullscreen/windowed option value.
 */
int GetFullscreenOption() {
    return *g_zGame_Options_PointerCache.videoFullscreen;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.gethudvisibilityoption
 * @recoil-artifact defines .text recoil:function:0x408340: zOpt::GetHudVisibilityOption.
 * Purpose: return the HUD visibility option for the active hardware mode.
 */
int GetHudVisibilityOption() {
    return *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.hudVisibilityHw : g_zGame_Options_PointerCache.hudVisibilitySw);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.gethudtypeforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x408360: zOpt::GetHudTypeForCurrentHwMode.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\zopt.cpp.
 * Purpose: return the HUD type option for the active hardware mode.
 */
int GetHudTypeForCurrentHwMode() {
    return *(g_zOpt_HwMode != 0 ? g_zGame_Options_PointerCache.hudTypeHw : g_zGame_Options_PointerCache.hudTypeSw);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getreplicatemode
 * @recoil-artifact defines .text recoil:function:0x408380: zOpt::GetReplicateMode
 * Purpose: return the active video replicate-mode option.
 */
int GetReplicateMode() {
    return *g_zGame_Options_PointerCache.replicate;
}

} // namespace zOpt
namespace zInp {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setjoystickoption
 * @recoil-artifact defines .text recoil:function:0x408390: zInp::SetJoystickOption.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_opt.cpp.
 * Purpose: store the joystick-enabled option when the option slot exists.
 */
void __fastcall SetJoystickOption(
    int enabled
) {
    if (g_zGame_Options_PointerCache.inputJoystick != 0) {
        *g_zGame_Options_PointerCache.inputJoystick = enabled;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setjoystickaxescountoption
 * @recoil-artifact defines .text recoil:function:0x4083a0: zInp::SetJoystickAxesCountOption.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_opt.cpp.
 * Purpose: store the detected joystick axis count option value.
 */
void __fastcall SetJoystickAxesCountOption(
    int axisCount
) {
    *g_zGame_Options_PointerCache.joystickNumAxes = axisCount;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setjoystickbuttoncountoption
 * @recoil-artifact defines .text recoil:function:0x4083b0: zInp::SetJoystickButtonCountOption.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_opt.cpp.
 * Purpose: store the detected joystick button count option value.
 */
void __fastcall SetJoystickButtonCountOption(
    int buttonCount
) {
    *g_zGame_Options_PointerCache.joystickNumButtons = buttonCount;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getjoystickoption
 * @recoil-artifact defines .text recoil:function:0x4083c0: zInp::GetJoystickOption.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_opt.cpp.
 * Purpose: return the joystick-enabled option value.
 */
int GetJoystickOption() {
    return *g_zGame_Options_PointerCache.inputJoystick;
}

} // namespace zInp
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.viewrectsection-setposition
 * @recoil-artifact defines .text recoil:function:0x4083d0: zOpt_ViewRectSection::SetPosition
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
 * @recoil-anchor recoil:anchor:battlesport.hud.viewrectsection-setsize
 * @recoil-artifact defines .text recoil:function:0x408400: zOpt_ViewRectSection::SetSize
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
 * @recoil-anchor recoil:anchor:battlesport.hud.viewrectsection-clamppointtoinclusivebounds
 * @recoil-artifact defines .text recoil:function:0x408430: zOpt::ViewRectSection_ClampPointToInclusiveBounds
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
 * @recoil-anchor recoil:anchor:battlesport.hud.camerasection-setactivecamera
 * @recoil-artifact defines .text recoil:function:0x408480: zOpt::CameraSection_SetActiveCamera
 * Purpose: store camera, recompute FOV, and reapply LOD.
 */
void __fastcall CameraSection_SetActiveCamera(
    zClass_NodePartial *camera
) {
    zOpt_CameraSection *const cameraSection = *g_zGame_Options_PointerCache.cameraSection;
    cameraSection->m_pCamera = camera;
    if (camera == 0) {
        return;
    }

    zOpt_ViewRectSection *const renderSection = *g_zGame_Options_PointerCache.renderSection;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.zopt-camerasection-getactivecamera
 * @recoil-artifact defines .text recoil:function:0x4084e0: zOpt_CameraSection_GetActiveCamera
 * Purpose: return active camera or null when unavailable.
 */
zClass_NodePartial *zOpt_CameraSection_GetActiveCamera() {
    if (g_zGame_Options_PointerCache.cameraSection == 0 || *g_zGame_Options_PointerCache.cameraSection == 0) {
        return 0;
    }

    return (*g_zGame_Options_PointerCache.cameraSection)->m_pCamera;
}
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.rendersection-setsize
 * @recoil-artifact defines .text recoil:function:0x408500: zOpt::RenderSection_SetSize.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the render-section dimensions and push the new resolution to
 * the attached window target.
 *
 * Evidence: BN forwards g_zGame_Options_PointerCache.renderSection->value to
 * zOpt_ViewRectSection::SetSize, then calls gwWindowSetResolution when the
 * section target is non-null; the shared zopt_video_section_setters VC5SP3
 * target byte-matches after relocation masking.
 */
void __fastcall RenderSection_SetSize(
    int width,
    int height
) {
    zOpt_ViewRectSection *section = *g_zGame_Options_PointerCache.renderSection;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.rendersection-setposition
 * @recoil-artifact defines .text recoil:function:0x408530: zOpt::RenderSection_SetPosition.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the render-section origin and push the new viewport rectangle
 * to the attached window target.
 *
 * Evidence: BN forwards g_zGame_Options_PointerCache.renderSection->value to
 * zOpt_ViewRectSection::SetPosition, then calls gwWindowSetResolution and
 * gwWindowSetSize when the section target is non-null; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall RenderSection_SetPosition(
    int x,
    int y
) {
    zOpt_ViewRectSection *section = *g_zGame_Options_PointerCache.renderSection;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.rendersection-settargetwindow
 * @recoil-artifact defines .text recoil:function:0x408570: zOpt::RenderSection_SetTargetWindow
 * Purpose: attach target window and apply render rectangle.
 */
void __fastcall RenderSection_SetTargetWindow(
    zClass_NodePartial *windowNode
) {
    zOpt_ViewRectSection *section = *g_zGame_Options_PointerCache.renderSection;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.getrendersection
 * @recoil-artifact defines .text recoil:function:0x4085a0: zOpt::GetRenderSection
 * Purpose: return the active render section pointer.
 */
zOpt_ViewRectSection *GetRenderSection() {
    return *g_zGame_Options_PointerCache.renderSection;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.displaysection-settargetdisplay
 * @recoil-artifact defines .text recoil:function:0x4085b0: zOpt::DisplaySection_SetTargetDisplay
 * Purpose: attach target display and apply display rectangle.
 */
void __fastcall DisplaySection_SetTargetDisplay(
    zClass_NodePartial *displayNode
) {
    zOpt_ViewRectSection *section = *g_zGame_Options_PointerCache.displaySection;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.displaysection-setposition
 * @recoil-artifact defines .text recoil:function:0x4085e0: zOpt::DisplaySection_SetPosition.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the display-section origin and push the new display rectangle
 * to the attached display target.
 *
 * Evidence: BN forwards g_zGame_Options_PointerCache.displaySection->value to
 * zOpt_ViewRectSection::SetPosition, then calls gwDisplaySetSize and
 * gwDisplaySetPosition when the section target is non-null; the shared
 * zopt_video_section_setters VC5SP3 target byte-matches after relocation
 * masking.
 */
void __fastcall DisplaySection_SetPosition(
    int x,
    int y
) {
    zOpt_ViewRectSection *section = *g_zGame_Options_PointerCache.displaySection;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.displaysection-setsize
 * @recoil-artifact defines .text recoil:function:0x408620: zOpt::DisplaySection_SetSize.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the display-section dimensions and push the new size to the
 * attached display target.
 *
 * Evidence: BN forwards g_zGame_Options_PointerCache.displaySection->value to
 * zOpt_ViewRectSection::SetSize, then calls gwDisplaySetSize when the section
 * target is non-null; the shared zopt_video_section_setters VC5SP3 target
 * byte-matches after relocation masking.
 */
void __fastcall DisplaySection_SetSize(
    int width,
    int height
) {
    zOpt_ViewRectSection *section = *g_zGame_Options_PointerCache.displaySection;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.getdisplaysection
 * @recoil-artifact defines .text recoil:function:0x408650: zOpt::GetDisplaySection.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active display view-rect option record.
 */
zOpt_ViewRectSection *GetDisplaySection() {
    return *g_zGame_Options_PointerCache.displaySection;
}

} // namespace zOpt
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zopt-displaysection-getwidth
 * @recoil-artifact defines .text recoil:function:0x408660: zOpt_DisplaySection_GetWidth.
 * Purpose: return the active display section width.
 */
int zOpt_DisplaySection_GetWidth() {
    return (*g_zGame_Options_PointerCache.displaySection)->width;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zopt-displaysection-getheight
 * @recoil-artifact defines .text recoil:function:0x408670: zOpt_DisplaySection_GetHeight.
 * Purpose: return the active display section height.
 */
int zOpt_DisplaySection_GetHeight() {
    return (*g_zGame_Options_PointerCache.displaySection)->height;
}
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.displaysection-setbitsperpixel
 * @recoil-artifact defines .text recoil:function:0x408680: zOpt::DisplaySection_SetBitsPerPixel.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: store the active display-section bit depth.
 *
 * Evidence: BN writes ecx to g_zGame_Options_PointerCache.displaySection->value->bitsPerPixel;
 * the shared zopt_video_section_setters VC5SP3 target byte-matches after
 * relocation masking.
 */
void __fastcall DisplaySection_SetBitsPerPixel(
    int bitsPerPixel
) {
    (*g_zGame_Options_PointerCache.displaySection)->bitsPerPixel = bitsPerPixel;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getdisplaysectionbitsperpixel
 * @recoil-artifact defines .text recoil:function:0x408690: zOpt::GetDisplaySectionBitsPerPixel.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active display section bit depth.
 */
int GetDisplaySectionBitsPerPixel() {
    return (*g_zGame_Options_PointerCache.displaySection)->bitsPerPixel;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getvideostridevalue
 * @recoil-artifact defines .text recoil:function:0x4086a0: zOpt::GetVideoStrideValue.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the configured video stride option value.
 */
int GetVideoStrideValue() {
    return *g_zGame_Options_PointerCache.videoStride;
}

} // namespace zOpt
namespace zVid {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getvideomodeindexfromoptions
 * @recoil-artifact defines .text recoil:function:0x4086b0: zVid::GetVideoModeIndexFromOptions.
 * Purpose: provide the recovered zVid::GetVideoModeIndexFromOptions behavior.
 */
int GetVideoModeIndexFromOptions() {
    return *g_zGame_Options_PointerCache.videoMode;
}

} // namespace zVid
namespace zOpt {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getwindowsection
 * @recoil-artifact defines .text recoil:function:0x4086c0: zOpt::GetWindowSection.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active window view-rect option record.
 */
zOpt_ViewRectSection *GetWindowSection() {
    return *g_zGame_Options_PointerCache.windowSection;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getwindowsectionheight
 * @recoil-artifact defines .text recoil:function:0x4086d0: zOpt::GetWindowSectionHeight.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zOptions\zopt.cpp.
 * Purpose: return the active window section height.
 */
int GetWindowSectionHeight() {
    return (*g_zGame_Options_PointerCache.windowSection)->height;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.windowsection-setsize
 * @recoil-artifact defines .text recoil:function:0x4086e0: zOpt::WindowSection_SetSize.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the window-section dimensions.
 *
 * Evidence: BN forwards g_zGame_Options_PointerCache.windowSection->value to
 * zOpt_ViewRectSection::SetSize; the shared zopt_video_section_setters VC5SP3
 * target byte-matches after relocation masking.
 */
void __fastcall WindowSection_SetSize(
    int width,
    int height
) {
    ViewRectSection_SetSize(
        *g_zGame_Options_PointerCache.windowSection,
        width,
        height
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.windowsection-setposition
 * @recoil-artifact defines .text recoil:function:0x408700: zOpt::WindowSection_SetPosition.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zGame\zGame.cpp.
 * Purpose: set the window-section origin.
 *
 * Evidence: BN forwards g_zGame_Options_PointerCache.windowSection->value to
 * zOpt_ViewRectSection::SetPosition; the shared zopt_video_section_setters
 * VC5SP3 target byte-matches after relocation masking.
 */
void __fastcall WindowSection_SetPosition(
    int x,
    int y
) {
    ViewRectSection_SetPosition(
        *g_zGame_Options_PointerCache.windowSection,
        x,
        y
    );
}

} // namespace zOpt
namespace zVid {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setvideomodeindex
 * @recoil-artifact defines .text recoil:function:0x408720: zVid::SetVideoModeIndex.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zVideo\zVid.cpp.
 * Purpose: apply a persisted shell video-mode preset to the render, window,
 * display, and replicate options.
 *
 * Evidence: BN selects modes 2 through 7 through the jump table, writes
 * g_zGame_Options_PointerCache.videoMode, updates the zOpt render/window/display sections, sets
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
        *g_zGame_Options_PointerCache.videoMode = 2;
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
        *g_zGame_Options_PointerCache.videoMode = 3;
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
        *g_zGame_Options_PointerCache.videoMode = 4;
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
        *g_zGame_Options_PointerCache.videoMode = 5;
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
        *g_zGame_Options_PointerCache.videoMode = 6;
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
        *g_zGame_Options_PointerCache.videoMode = 7;
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
        *g_zGame_Options_PointerCache.videoMode = 0;
        return;
    }
}

} // namespace zVid
namespace HudUiMgr {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.screentoworld
 * @recoil-artifact defines .text recoil:function:0x4089c0: HudUiMgr::ScreenToWorld.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
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
    zOpt_ViewRectSection *const renderSection = *g_zGame_Options_PointerCache.renderSection;
    zOpt_ViewRectSection *const displaySection = *g_zGame_Options_PointerCache.displaySection;

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
 * @recoil-anchor recoil:anchor:battlesport.hud.setwolpasswordflag
 * @recoil-artifact defines .text recoil:function:0x408a10: zOpt::SetWolPasswordFlag.
 * Purpose: store the WOL password flag option value through its option pointer.
 */
void __fastcall SetWolPasswordFlag(
    int value
) {
    *g_zGame_Options_PointerCache.wolPasswordFlag = value;
}

} // namespace zOpt
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zopt-getwolpasswordflagvalue
 * @recoil-artifact defines .text recoil:function:0x408a20: zOpt_GetWolPasswordFlagValue.
 * Purpose: return the WOL password flag option value through its option pointer.
 */
int zOpt_GetWolPasswordFlagValue() {
    return *g_zGame_Options_PointerCache.wolPasswordFlag;
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
 * Source model note: the ordinary empty RecoilStateDialogHost::OnSuspend identity
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicontrolsdialog-huduicontrolsdialog
 * @recoil-artifact defines .text recoil:function:0x408a30: HudUiControlsDialog::HudUiControlsDialog.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud_ui_dialogs.cpp.
 * Purpose: Construct the controls dialog, bind its ZRD widgets, and seed option selectors from current input/options.
 * Evidence: BN/source slice builds HudUiBackground, resume/commands widgets, five option selectors, loads
 * dialog.zrd/CONTROLS_DIALOG, binds named controls, then seeds zInp/zOpt selector indices.
 */
HudUiControlsDialog::HudUiControlsDialog() {
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
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicontrolsdialog-commandswidget-onactivate
 * @recoil-artifact defines .text recoil:function:0x408c20: HudUiControlsDialog_CommandsWidget::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud_ui_dialogs.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicontrolsdialog-destructor
 * @recoil-artifact defines .text recoil:function:0x408c70: HudUiControlsDialog::Destructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\hud_ui_dialogs.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x408d20: RecoilStateControls::StaticInitAndRegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-staticinit
 * @recoil-artifact defines .text recoil:function:0x408d30: RecoilStateControls::StaticInit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: placement-construct the zero-initialized global controls app state singleton.
 */
RecoilStateControls *RecoilStateControls::StaticInit() {
    return new (&g_RecoilStateControls) RecoilStateControls;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-registeratexit
 * @recoil-artifact defines .text recoil:function:0x408d40: RecoilStateControls::RegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: register the global controls app state destructor with the CRT atexit list.
 */
void RecoilStateControls::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x408d50: RecoilStateControls::AtExitDestructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: destroy the global controls app state during CRT shutdown.
 */
void RecoilStateControls::AtExitDestructor() {
    g_RecoilStateControls.~RecoilStateControls();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-recoilstatecontrols
 * @recoil-artifact defines .text recoil:function:0x408d60: RecoilStateControls::RecoilStateControls.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: initialize the controls app state and clear its dialog pointer.
 */
RecoilStateControls::RecoilStateControls() {
    m_dialog = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-destructor-recoilstatecontrols
 * @recoil-artifact defines .text recoil:function:0x408d90: RecoilStateControls::Destructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-ontrybecomecurrent
 * @recoil-artifact defines .text recoil:function:0x408df0: RecoilStateControls::OnTryBecomeCurrent.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: lazily create and enable the controls dialog, then seed option selectors.
 */
int RecoilStateControls::OnTryBecomeCurrent() {
    if (m_dialog == 0) {
        m_dialog = new HudUiControlsDialog;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-ondeactivate
 * @recoil-artifact defines .text recoil:function:0x408ec0: RecoilStateControls::OnDeactivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
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

#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zHud/zhud_ui_defs.h"
#include "GameZRecoil/zVideo/zvid.h"

/**
 * Original function; shared retail body 0x408f50; the authored controls-state override
 * shares the selected physical body with another reviewed logical identity.
 * Purpose: disable, blit, unlock, and present the hosted HUD dialog when
 * another app state is pushed on top of it.
 */
void RecoilStateControls::OnSuspend(
    int suspendParam
) {
    (void)suspendParam;

    if (m_dialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    m_dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-onresume
 * @recoil-artifact defines .text recoil:function:0x408fa0: RecoilStateControls::OnResume.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecontrols-queueenter
 * @recoil-artifact defines .text recoil:function:0x408ff0: RecoilStateControls::QueueEnter.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\recoil_state.cpp.
 * Purpose: queue the global controls app state on the Recoil app state stack.
 */
void RecoilStateControls::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilStateControls,
        0
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduizrdwidgetex17c-enablechildatindex
 * @recoil-artifact defines .text recoil:function:0x409010: HudUiZrdWidgetEx17C::EnableChildAtIndex.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicreditspanel-huduicreditspanel
 * @recoil-artifact defines .text recoil:function:0x409040: HudUiCreditsPanel::HudUiCreditsPanel.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCreditsPanel.cpp.
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
 * Original function; retail address 0x409160.
 * Purpose: queue exit from the credits state and run the inherited activation behavior.
 */
void HudUiCreditsBackButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Original function; folded with the selected retail body at 0x409160.
 * Purpose: queue exit from the controls state and run the inherited activation behavior.
 */
void HudUiControlsDialog_ResumeWidget::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Original function; folded with the selected retail body at 0x409160.
 * Purpose: queue exit from the command-binding state and run the inherited activation behavior.
 */
void HudCmdSimpleWidget::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Original function; folded with the selected retail body at 0x409160.
 * Purpose: queue exit from the quit-confirmation state and run the inherited activation behavior.
 */
void HudUiConfirmQuitCancelButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicreditsquitbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x409180: HudUiCreditsQuitButton::OnActivate.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduizrdscrollingtext-destructor-huduizrdscrollingtext
 * @recoil-artifact defines .text recoil:function:0x4091e0: HudUiZrdScrollingText::~HudUiZrdScrollingText.
 * @recoil-artifact emits .text recoil:function:0x40bef0: implicit HudUiPanelLayoutEntry destructor used by the
 * nested row-vector cleanup path.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCreditsPanel.cpp.
 * Purpose: keep the ordinary destructor inline at its source-order position
 * so the following credits-panel destructor can naturally expand member
 * teardown while VC5 emits this retained lifecycle identity in retail order.
 */
inline HudUiZrdScrollingText::~HudUiZrdScrollingText() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicreditspanel-destructor-huduicreditspanel
 * @recoil-artifact defines .text recoil:function:0x4092a0: HudUiCreditsPanel::~HudUiCreditsPanel.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCreditsPanel.cpp.
 * Purpose: invoke ordinary reverse member and base teardown for the credits
 * panel at the end of its lifetime.
 */
HudUiCreditsPanel::~HudUiCreditsPanel() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicreditspanel-updateall
 * @recoil-artifact defines .text recoil:function:0x409380: HudUiCreditsPanel::UpdateAll
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduizrdscrollingtext-update
 * @recoil-artifact defines .text recoil:function:0x409410: HudUiZrdScrollingText::Update
 * Purpose: update the scrolling credits widget and each row panel.
 */
void HudUiZrdScrollingText::Update(
    float deltaSeconds
) {
    HudUiElement::Update(deltaSeconds);

    HudUiPanelSpan *row = rows.begin();
    while (row != rows.end()) {
        HudUiPanelLayoutEntry *entry = row->begin();
        while (entry != row->end()) {
            entry->panel.Update(deltaSeconds);
            ++entry;
        }

        ++row;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduizrdscrollingtext-updatescrollpositions
 * @recoil-artifact defines .text recoil:function:0x409470: HudUiZrdScrollingText::UpdateScrollPositions
 * Purpose: position scrolling credits row entries and clip their panel visibility to the text rectangle.
 */
void HudUiZrdScrollingText::UpdateScrollPositions(
    float scrollProgress
) {
    const int left = rect.left;
    const int scrollY = (int)((float)(rect.top - totalHeight) * scrollProgress +
                              (1.0 - scrollProgress) * (float)(rect.bottom));

    HudUiPanelSpan *row = rows.begin();
    while (row != rows.end()) {
        HudUiPanelLayoutEntry *entry = row->begin();
        while (entry != row->end()) {
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduizrdscrollingtext-onactivateresetownerfade
 * @recoil-artifact defines .text recoil:function:0x409550: HudUiZrdScrollingText::OnActivateResetOwnerFade
 * Purpose: reset the owning credits panel fade progress when the scrolling credits text activates.
 */
void HudUiZrdScrollingText::OnActivateResetOwnerFade() {
    ((HudUiCreditsPanel *)(owner))->fadeProgress = 0.0f;
}

#if defined(_MSC_VER) && _MSC_VER == 1100
#if !defined(RECOIL_VC5_ABI_EQUIVALENCE_PROBE)
extern "C" void __cdecl RecoilHudUiPanelAssignAsmAlias();
namespace std {
/**
 * Purpose: provide the user-approved, provenance-agnostic exact
 * compiler-provider implementation for retail 0x40a170 after credible VC5 C++
 * variants failed; this establishes no original-source, provider-ownership, or
 * tier claim.
 */
template <>
__declspec(naked) inline HudUiPanelLayoutEntry *__fastcall copy(
    HudUiPanelLayoutEntry *first,
    HudUiPanelLayoutEntry *last,
    HudUiPanelLayoutEntry *destination
) {
    __asm {
        push ecx
        push ebp
        push esi
        mov esi, ecx
        mov dword ptr [esp + 8], edx
        cmp esi, edx
        je copy_empty
        mov ebp, dword ptr [esp + 16]
        push edi
        push ebx
        lea ebx, dword ptr [esi + 0x2a4]
        lea edi, dword ptr [ebp + 0x2a4]
    copy_loop:
        push esi
        mov ecx, ebp
        call RecoilHudUiPanelAssignAsmAlias
        mov eax, dword ptr [ebx]
        add esi, 0x2ac
        mov dword ptr [edi], eax
        mov ecx, dword ptr [ebx + 4]
        mov eax, dword ptr [esp + 16]
        mov dword ptr [edi + 4], ecx
        add ebp, 0x2ac
        add edi, 0x2ac
        add ebx, 0x2ac
        cmp esi, eax
        jne copy_loop
        pop ebx
        pop edi
        mov eax, ebp
        pop esi
        pop ebp
        pop ecx
        ret 4
    copy_empty:
        mov ebp, dword ptr [esp + 16]
        pop esi
        mov eax, ebp
        pop ebp
        pop ecx
        ret 4
    }
}
}
#endif
#pragma inline_depth(1)
inline void HudUiPanelSpan::clear() {
    iterator newEnd = std::copy(_Last, _Last, _First);
    _Destroy(newEnd, _Last);
    _Last = newEnd;
}
#pragma inline_depth()
#endif

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduizrdscrollingtext-loadfromzrd
 * @recoil-artifact defines .text recoil:function:0x409570: HudUiZrdScrollingText::LoadFromZrd.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiCreditsPanel.cpp.
 * Purpose: load scrolling credits rows from ZRD layout data and compute stacked row heights.
 */
int HudUiZrdScrollingText::LoadFromZrd(
    zReader::Node *zrdSection,
    HudUiBackground *ownerDialog
) {
    HudUiZrdWidget::LoadFromZrd(
        zrdSection,
        ownerDialog
    );

    zReader::Node *const rectNode = zReader_GetNamedNode(
        zrdSection,
        "RECT"
    );
    if (rectNode != 0) {
        rect.left =
            rectNode->value.nodes[1].value.nodes[1].value.i32 + originX;
        rect.top =
            rectNode->value.nodes[1].value.nodes[2].value.i32 + originY;
        rect.right =
            rectNode->value.nodes[2].value.nodes[1].value.i32 + originX;
        rect.bottom =
            rectNode->value.nodes[2].value.nodes[2].value.i32 + originY;
    }

    zReader::Node *const scrollRateNode = zReader_GetNamedNode(
        zrdSection,
        "SCROLL_RATE"
    );
    if (scrollRateNode != 0) {
        ((HudUiCreditsPanel *)(ownerDialog))->fadeStep = scrollRateNode->value.f32;
    }

    zReader::Node *const scrollingTextNode = zReader_GetNamedNode(
        zrdSection,
        "SCROLLING_TEXT"
    );
    if (scrollingTextNode == 0) {
        return 1;
    }

    HudUiPanelSpan templateSpan;

    const int rowCount = scrollingTextNode->value.nodes[0].value.i32;
    for (int rowIndex = 1; rowIndex < rowCount; ++rowIndex) {
        const int labelCount =
            scrollingTextNode->value.nodes[rowIndex].value.nodes[0].value.i32;

        templateSpan.clear();

        for (int labelIndex = 1; labelIndex < labelCount; ++labelIndex) {
            const char *const key =
                scrollingTextNode->value.nodes[rowIndex]
                    .value.nodes[labelIndex].value.nodes[1].value.str;
            const char *const text = zLoc::ResolveMessageKeyOrFallback(key);
            const int layoutX =
                scrollingTextNode->value.nodes[rowIndex]
                    .value.nodes[labelIndex].value.nodes[2].value.i32;
            const int layoutY =
                scrollingTextNode->value.nodes[rowIndex]
                    .value.nodes[labelIndex].value.nodes[3].value.i32;
            const int styleIndex =
                scrollingTextNode->value.nodes[rowIndex]
                    .value.nodes[labelIndex].value.nodes[4].value.i32;

            HudUiPanelLayoutEntry templateEntry(
                0,
                0,
                0
            );
            templateEntry.panel.SetTextFmt(
                "%s",
                text
            );
            templateEntry.layoutX = layoutX;
            templateEntry.layoutY = layoutY;

            const HudFontStyle *style = &owner->fontStyles[styleIndex];
            style = style->validMarker != 0 ? style : 0;
            if (style != 0) {
                templateEntry.panel.SetFont(
                    style->fontName,
                    style->fontSize,
                    style->fontWeight,
                    0,
                    0,
                    0,
                    2
                );
                templateEntry.panel.textColor0 = style->textColor;
                templateEntry.panel.textColor1 = style->textColor;
                templateEntry.panel.textDirty = 1;
                templateEntry.panel.shadowEnabled = style->shadowEnabled;
                templateEntry.panel.shadowOffsetX = 1;
                templateEntry.panel.shadowOffsetY = 1;
            }
            templateSpan.insert(
                templateSpan.end(),
                templateEntry
            );
        }

        rows.insert(
            rows.end(),
            templateSpan
        );
    }

    totalHeight = 0;
    HudUiPanelSpan *row = rows.begin();
    while (row != rows.end()) {
        int rowHeight = 0;
        HudUiPanelLayoutEntry *entry = row->begin();
        while (entry != row->end()) {
            const int entryBottom = entry->panel.QueryTextHeight() + entry->layoutY;
            if (entryBottom > rowHeight) {
                rowHeight = entryBottom;
            }

            ++entry;
        }

        entry = row->begin();
        while (entry != row->end()) {
            entry->layoutY += totalHeight;
            ++entry;
        }

        totalHeight += rowHeight;
        ++row;
    }

    return 1;
}

#if !defined(_MSC_VER) || _MSC_VER >= 1200
/**
 * Provider boundary: non-VC5 compatibility implementation; retail 0x409910 is
 * the VC5 std::vector<HudUiPanelLayoutEntry> base destructor.
 * Purpose: release panel entries and storage for non-VC5 builds.
 */
void HudUiPanelSpan::Clear() {
    HudUiPanelLayoutEntry *entry = first;
    while (entry != last) {
        entry->panel.~HudUiPanel();
        ++entry;
    }

    ::operator delete(first);
    first = 0;
    last = 0;
    limit = 0;
}
#endif

#include "Battlesport/recoil_state_credits.h"
#include "Battlesport/hud.h"
#include "GameZRecoil/zHud/zhud_ui.h"

#include <new>
#include <stdlib.h>

#undef g_RecoilStateCredits
/**
 * Source owner: legacy.app_shell.cluster_recoilstatebase.
 * Source model note: StaticInit constructs the typed object in this storage and
 * AtExitDestructor tears it down through the CRT at-exit list.
 * Purpose: own the zero-initialized credits app-state singleton storage.
 */
RecoilStateCreditsStorage g_RecoilStateCredits = {0};
#define g_RecoilStateCredits \
    (*(RecoilStateCredits *)&g_RecoilStateCredits)

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecredits-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x409950: RecoilStateCredits::StaticInitAndRegisterAtExit.
 *
 * Purpose: construct the global credits app state and register its CRT
 * shutdown destructor.
 */
void RecoilStateCredits::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *RecoilStateCreditsCrtInitializerFn)();
/* VC5 emits this credits-state startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
RecoilStateCreditsCrtInitializerFn s_RecoilStateCreditsCrtInit =
    RecoilStateCredits::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecredits-staticinit
 * @recoil-artifact defines .text recoil:function:0x409960: RecoilStateCredits::StaticInit.
 *
 * Purpose: placement-construct the zero-initialized global credits app-state
 * singleton.
 */
RecoilStateCredits *RecoilStateCredits::StaticInit() {
    return new (&g_RecoilStateCredits) RecoilStateCredits;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecredits-registeratexit
 * @recoil-artifact defines .text recoil:function:0x409970: RecoilStateCredits::RegisterAtExit.
 *
 * Purpose: register the global credits app-state destructor with the CRT
 * at-exit list.
 */
void RecoilStateCredits::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecredits-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x409980: RecoilStateCredits::AtExitDestructor.
 *
 * Purpose: destroy the global credits app state during CRT shutdown.
 */
void RecoilStateCredits::AtExitDestructor() {
    g_RecoilStateCredits.~RecoilStateCredits();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecredits-recoilstatecredits
 * @recoil-artifact defines .text recoil:function:0x409990: RecoilStateCredits::RecoilStateCredits.
 *
 * Purpose: initialize the credits app-state object and clear the active
 * credits-panel pointer.
 */
RecoilStateCredits::RecoilStateCredits() {
    m_dialog = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatedialoghost-onwndactivate
 * @recoil-artifact defines .text recoil:function:0x4099a0: RecoilStateDialogHost::OnWndActivate.
 *
 * Purpose: refresh the hosted HUD dialog surfaces when the application is
 * reactivated.
 */
void RecoilStateDialogHost::OnWndActivate(
    int activateCode
) {
    if (activateCode == 0) {
        return;
    }

    if (m_dialog == 0) {
        return;
    }

    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
    m_dialog->InvalidateChildren();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecredits-destructor-recoilstatecredits
 * @recoil-artifact defines .text recoil:function:0x4099f0: RecoilStateCredits::~RecoilStateCredits.
 *
 * Purpose: tear down the owned credits dialog during static state destruction.
 */
RecoilStateCredits::~RecoilStateCredits() {
    HudUiCreditsPanel *creditsPanel = (HudUiCreditsPanel *)m_dialog;
    if (creditsPanel != 0) {
        creditsPanel->SetEnabled(0);

        creditsPanel = (HudUiCreditsPanel *)m_dialog;
        if (creditsPanel != 0) {
            delete creditsPanel;
        }

        m_dialog = 0;
    }

    /* Late ABI reset keeps RecoilStateBase materialization after the zStub block. */
    ((RecoilStateBase *)this)->RecoilStateBase::~RecoilStateBase();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecredits-ontrybecomecurrent
 * @recoil-artifact defines .text recoil:function:0x409a60: RecoilStateCredits::OnTryBecomeCurrent.
 *
 * Purpose: allocate, construct, and enable the credits dialog when the credits
 * app state becomes current.
 */
int RecoilStateCredits::OnTryBecomeCurrent() {
    HudUiCreditsPanel *creditsPanel =
        (HudUiCreditsPanel *) ::operator new(sizeof(HudUiCreditsPanel));
    if (creditsPanel != 0) {
        creditsPanel = new (creditsPanel) HudUiCreditsPanel;
    }
    m_dialog = creditsPanel;

    creditsPanel->SetEnabled(1);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatedialoghost-ondeactivate
 * @recoil-artifact defines .text recoil:function:0x409ad0: RecoilStateDialogHost::OnDeactivate.
 *
 * Purpose: disable, repaint, destroy, and clear the active hosted HUD dialog.
 */
void RecoilStateDialogHost::OnDeactivate() {
    if (m_dialog == 0) {
        return;
    }

    m_dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();

    if (m_dialog != 0) {
        delete ((HudUiBackground *)m_dialog);
    }

    m_dialog = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatecredits-queuepush
 * @recoil-artifact defines .text recoil:function:0x409b00: RecoilStateCredits::QueuePush.
 *
 * Purpose: queue the global credits state as the next pushed RecoilApp state.
 */
void RecoilStateCredits::QueuePush() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilStateCredits,
        0
    );
}

/*
 * implicit HudUiPanelLayoutEntry::operator=.
 * implicit HudUiPanelLayoutEntry copy constructor.
 * Purpose: let the VC5 std::vector algorithms emit the natural entry special
 * members while preserving their address-backed translation-unit provenance.
 */

#if !defined(_MSC_VER) || _MSC_VER >= 1200

/**
 * Non-VC5 compatibility implementation for the bespoke panel-span container.
 * Original function; retail address 0x409b20.
 * Purpose: destroy the panel entries and release the span allocation.
 */
void HudUiPanelSpan::DestroyAndFree() {
    HudUiPanelLayoutEntry *finish = last;
    HudUiPanelLayoutEntry *entry = first;
    while (entry != finish) {
        entry->panel.HudUiPanel::~HudUiPanel();
        ++entry;
    }

    ::operator delete(first);
    first = 0;
    last = 0;
    limit = 0;
}

/**
 * Non-VC5 compatibility range teardown for the bespoke panel-span container.
 * The VC5 branch uses the natural std::vector provider algorithms.
 * Original function; retail address 0x409b60.
 * Purpose: destroy each panel-layout entry in the supplied half-open range.
 */
void __stdcall HudUiPanelLayoutEntry::DestroyRange(
    HudUiPanelLayoutEntry *start,
    HudUiPanelLayoutEntry *end
) {
    HudUiPanelLayoutEntry *entry = start;
    while (entry != end) {
        entry->panel.HudUiPanel::~HudUiPanel();
        ++entry;
    }
}

/**
 * Non-VC5 compatibility insertion for the bespoke panel-span container.
 * The VC5 branch uses std::vector<HudUiPanelLayoutEntry>::insert.
 * Original function; retail address 0x409b90.
 * Purpose: insert repeated panel-layout entries while preserving vector state.
 */
void HudUiPanelSpan::InsertN(
    HudUiPanelLayoutEntry *insertPos,
    unsigned int count,
    const HudUiPanelLayoutEntry *templatePanel
) {
    if (count == 0) {
        return;
    }

    const size_t size = first != 0 ? (size_t)(last - first) : 0;
    const size_t positionIndex = first != 0 && insertPos != 0 ? (size_t)(insertPos - first) : 0;
    const size_t capacity = first != 0 ? (size_t)(limit - first) : 0;
    const size_t tailCount = size - positionIndex;

    if (size + count <= capacity) {
        if (tailCount >= count) {
            HudUiPanelLayoutEntry *source = last - count;
            HudUiPanelLayoutEntry *dest = last;
            while (source != last) {
                new (dest) HudUiPanelLayoutEntry(*source);
                ++source;
                ++dest;
            }

            source = last - count;
            dest = last;
            while (source != first + positionIndex) {
                --source;
                --dest;
                *dest = *source;
            }

            for (unsigned int i = 0; i < count; ++i) {
                first[positionIndex + i] = *templatePanel;
            }
        } else {
            HudUiPanelLayoutEntry *dest = last;
            for (unsigned int i = 0; i < count - tailCount; ++i) {
                new (dest) HudUiPanelLayoutEntry(*templatePanel);
                ++dest;
            }

            for (HudUiPanelLayoutEntry *source = first + positionIndex; source != last;
                ++source, ++dest) {
                new (dest) HudUiPanelLayoutEntry(*source);
            }

            for (HudUiPanelLayoutEntry *entry = first + positionIndex; entry != last; ++entry) {
                *entry = *templatePanel;
            }
        }

        last += count;
        return;
    }

    const size_t growth = count < size ? size : count;
    const size_t newCapacity = size + growth;
    HudUiPanelLayoutEntry *const newBegin =
        (HudUiPanelLayoutEntry *)(::operator new(newCapacity * sizeof(HudUiPanelLayoutEntry)));
    HudUiPanelLayoutEntry *dest = newBegin;

    for (size_t prefixIndex = 0; prefixIndex < positionIndex; ++prefixIndex, ++dest) {
        new (dest) HudUiPanelLayoutEntry(first[prefixIndex]);
    }

    for (unsigned int insertIndex = 0; insertIndex < count; ++insertIndex, ++dest) {
        new (dest) HudUiPanelLayoutEntry(*templatePanel);
    }

    for (size_t suffixIndex = positionIndex; suffixIndex < size; ++suffixIndex, ++dest) {
        new (dest) HudUiPanelLayoutEntry(first[suffixIndex]);
    }

    HudUiPanelLayoutEntry *entry = first;
    while (entry != last) {
        entry->panel.HudUiPanel::~HudUiPanel();
        ++entry;
    }

    ::operator delete(first);
    first = newBegin;
    last = newBegin + size + count;
    limit = newBegin + newCapacity;
}

/**
 * Non-VC5 compatibility insertion for the bespoke row-vector container.
 * The VC5 branch uses std::vector<HudUiPanelSpan>::insert.
 * Original function; retail address 0x409f00.
 * Purpose: insert repeated panel spans while preserving nested vector state.
 */
void HudUiPanelSpanVec::InsertN(
    HudUiPanelSpan *insertPos,
    unsigned int count,
    const HudUiPanelSpan *templateSpan
) {
    if (count == 0) {
        return;
    }

    const size_t size = first != 0 ? (size_t)(last - first) : 0;
    const size_t positionIndex = first != 0 && insertPos != 0 ? (size_t)(insertPos - first) : 0;
    const size_t capacity = first != 0 ? (size_t)(limit - first) : 0;
    const size_t tailCount = size - positionIndex;

    if (size + count <= capacity) {
        if (tailCount >= count) {
            HudUiPanelSpan *source = last - count;
            HudUiPanelSpan *dest = last;
            while (source != last) {
                dest->CopyInit(source);
                ++source;
                ++dest;
            }

            source = last - count;
            dest = last;
            while (source != first + positionIndex) {
                --source;
                --dest;
                dest->CopyFrom(source);
            }

            for (unsigned int i = 0; i < count; ++i) {
                first[positionIndex + i].CopyFrom(templateSpan);
            }
        } else {
            HudUiPanelSpan *dest = last;
            for (unsigned int i = 0; i < count - tailCount; ++i) {
                dest->CopyInit(templateSpan);
                ++dest;
            }

            for (HudUiPanelSpan *source = first + positionIndex; source != last; ++source, ++dest) {
                dest->CopyInit(source);
            }

            for (HudUiPanelSpan *span = first + positionIndex; span != last; ++span) {
                span->CopyFrom(templateSpan);
            }
        }

        last += count;
        return;
    }

    const size_t growth = count < size ? size : count;
    const size_t newCapacity = size + growth;
    HudUiPanelSpan *const newBegin =
        (HudUiPanelSpan *)(::operator new(newCapacity * sizeof(HudUiPanelSpan)));
    HudUiPanelSpan *dest = newBegin;

    for (size_t prefixIndex = 0; prefixIndex < positionIndex; ++prefixIndex, ++dest) {
        dest->CopyInit(&first[prefixIndex]);
    }

    for (unsigned int insertIndex = 0; insertIndex < count; ++insertIndex, ++dest) {
        dest->CopyInit(templateSpan);
    }

    for (size_t suffixIndex = positionIndex; suffixIndex < size; ++suffixIndex, ++dest) {
        dest->CopyInit(&first[suffixIndex]);
    }

    HudUiPanelSpan *span = first;
    while (span != last) {
        span->Clear();
        ++span;
    }

    ::operator delete(first);
    first = newBegin;
    last = newBegin + size + count;
    limit = newBegin + newCapacity;
}

/**
 * Non-VC5 compatibility copy-assignment range for the bespoke container.
 * The VC5 branch uses the natural std::vector provider algorithms.
 * Original function; retail address 0x40a170.
 * Purpose: copy-assign a panel-layout range into initialized destination entries.
 */
HudUiPanelLayoutEntry *__fastcall HudUiPanelLayoutEntry::CopyAssignRange(
    const HudUiPanelLayoutEntry *sourceStart,
    const HudUiPanelLayoutEntry *sourceEnd,
    HudUiPanelLayoutEntry *dest
) {
    HudUiPanelLayoutEntry *out = dest;
    const HudUiPanelLayoutEntry *source = sourceStart;
    while (source != sourceEnd) {
        *out = *source;
        ++source;
        ++out;
    }

    return out;
}

/**
 * Non-VC5 compatibility implementation for the bespoke panel-span container.
 * Original function; retail address 0x40a240.
 * Purpose: copy-initialize a panel span and its owned layout entries.
 */
HudUiPanelSpan * HudUiPanelSpan::CopyInit(
    const HudUiPanelSpan *source
) {
    allocatorProxy = (allocatorProxy & 0xffffff00) | (source->allocatorProxy & 0xff);

    const size_t count = source->first != 0 ? (size_t)(source->last - source->first) : 0;
    HudUiPanelLayoutEntry *const newBegin =
        (HudUiPanelLayoutEntry *)(::operator new(count * sizeof(HudUiPanelLayoutEntry)));
    HudUiPanelLayoutEntry *dest = newBegin;

    const HudUiPanelLayoutEntry *sourceEntry = source->first;
    while (sourceEntry != source->last) {
        new (dest) HudUiPanelLayoutEntry(*sourceEntry);
        ++sourceEntry;
        ++dest;
    }

    first = newBegin;
    last = dest;
    limit = dest;
    return this;
}

/**
 * Non-VC5 compatibility implementation for the bespoke panel-span container.
 * Original function; retail address 0x40a300.
 * Purpose: copy-assign a panel span while reusing or replacing its allocation.
 */
HudUiPanelSpan * HudUiPanelSpan::CopyFrom(
    const HudUiPanelSpan *source
) {
    if (this == source) {
        return this;
    }

    const size_t sourceCount = source->first != 0 ? (size_t)(source->last - source->first) : 0;
    const size_t currentCount = first != 0 ? (size_t)(last - first) : 0;
    const size_t capacity = first != 0 ? (size_t)(limit - first) : 0;

    if (sourceCount <= currentCount) {
        HudUiPanelLayoutEntry *dest = first;
        const HudUiPanelLayoutEntry *sourceEntry = source->first;
        while (sourceEntry != source->last) {
            *dest = *sourceEntry;
            ++sourceEntry;
            ++dest;
        }

        HudUiPanelLayoutEntry *oldEntry = dest;
        while (oldEntry != last) {
            ++oldEntry;
        }

        last = first + sourceCount;
        return this;
    }

    if (sourceCount <= capacity) {
        HudUiPanelLayoutEntry *dest = first;
        const HudUiPanelLayoutEntry *sourceEntry = source->first;
        for (size_t i = 0; i < currentCount; ++i) {
            *dest = *sourceEntry;
            ++sourceEntry;
            ++dest;
        }

        while (sourceEntry != source->last) {
            new (dest) HudUiPanelLayoutEntry(*sourceEntry);
            ++sourceEntry;
            ++dest;
        }

        last = first + sourceCount;
        return this;
    }

    HudUiPanelLayoutEntry *oldEntry = first;
    while (oldEntry != last) {
        oldEntry->panel.HudUiPanel::~HudUiPanel();
        ++oldEntry;
    }

    ::operator delete(first);

    HudUiPanelLayoutEntry *const newBegin =
        (HudUiPanelLayoutEntry *)(::operator new(sourceCount * sizeof(HudUiPanelLayoutEntry)));
    HudUiPanelLayoutEntry *dest = newBegin;
    const HudUiPanelLayoutEntry *sourceEntry = source->first;
    while (sourceEntry != source->last) {
        new (dest) HudUiPanelLayoutEntry(*sourceEntry);
        ++sourceEntry;
        ++dest;
    }

    first = newBegin;
    last = dest;
    limit = dest;
    return this;
}

#endif

/* Body include for the physical hud.cpp command-binding layer [0x40a5b0,0x40c370). */
/* Included by src/Battlesport/hud.cpp; keep this file body-only. */

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-hudcmddialog
 * @recoil-artifact defines .text recoil:function:0x40a5b0: HudCmdDialog::HudCmdDialog.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: construct the command-binding dialog, bind its ZRD widgets, and
 * populate command groups before enabling the container children.
 */
HudCmdDialog::HudCmdDialog() {
    zReader::Node *const loadedSection = HudUiBackground::LoadFromZrd(
        "dialog.zrd",
        "COMMANDS_DIALOG",
        0
    );
    if (loadedSection != 0) {
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&resumeButton),
            "CMD_RESUME_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&resetButton),
            "CMD_RESET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&commandList),
            "CMD_COMMAND_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&keyAButton),
            "CMD_KEYA_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&keyBButton),
            "CMD_KEYB_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&joyButton),
            "CMD_JOY_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&mouseButton),
            "CMD_MOUSE_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&setList),
            "CMD_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&nextSetButton),
            "CMD_NEXT_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&prevSetButton),
            "CMD_PREV_SET_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&nextCommandButton),
            "CMD_NEXT_CMD_BTN"
        );
        HudUiBackground::BindWidgetByName(
            loadedSection,
            (HudUiZrdWidget *)(&prevCommandButton),
            "CMD_PREV_CMD_BTN"
        );

        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&promptPanel),
            "PRESS_A_KEY"
        );
        HudUiBackground::BindPrimitiveNodeToElement(
            loadedSection,
            (HudUiElement *)(&descriptionPanel),
            "CMD_DESCRIPTION"
        );
        HudUiBackground::FreeLoadedTreeRoots(
            (int)(unsigned int)loadedSection
        );
        promptPanel.SetFlashRate(1.0f);
    }

    promptPanel.SetVisible(0);

    for (int groupIndex = 0; groupIndex < zInput::BindGroupList_GetCount(); ++groupIndex) {
        setList.AddTextEntry(
            groupIndex,
            zInput::BindGroupList_GetGroupTitle(groupIndex),
            setList.originX,
            setList.originY
        );
        setList.ApplyFontStyleForEntry(
            groupIndex,
            (int)((unsigned int)(setList.fontStyleRef))
        );
    }

    RebuildCommandBindingListsForGroup(0);
    captureState = 0;
    zInput::ResetAllTransitionState();
    ((HudUiContainer *)(this))->SetChildFlags(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-destructor-hudcmddialog
 * @recoil-artifact defines .text recoil:function:0x40adf0: HudCmdDialog::~HudCmdDialog.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: let ordinary C++ member and base lifetime rules tear down the
 * command dialog in reverse construction order.
 */
HudCmdDialog::~HudCmdDialog() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-updateall
 * @recoil-artifact defines .text recoil:function:0x40b140: HudCmdDialog::UpdateAll.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: advance the recovered HUD update path through the dialog's primary virtual update.
 */
void HudCmdDialog::UpdateAll(
    float deltaTime
) {
    HudUiBackgroundContainer::UpdateAll(deltaTime);

    switch (captureState) {
    case 0:
        promptPanel.SetVisible(0);
        --g_HudCmdMouseDebounceFrames;
        break;

    case 1: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired keyboard key.");
        keyBButton.SetChecked(0);
        joyButton.SetChecked(0);
        mouseButton.SetChecked(0);

        const int keyCode = zInput::Keyboard_WaitForAnyKeyPress(0);
        if (keyCode != 0) {
            ApplyPrimaryKeyRebind(
                keyCode,
                keyAButton.selectedBindingIndex
            );
            keyAButton.SetChecked(0);
        }
        break;
    }

    case 2: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired keyboard key.");
        keyAButton.SetChecked(0);
        joyButton.SetChecked(0);
        mouseButton.SetChecked(0);

        const int keyCode = zInput::Keyboard_WaitForAnyKeyPress(0);
        if (keyCode != 0) {
            ApplySecondaryKeyRebind(
                keyCode,
                keyBButton.selectedBindingIndex
            );
            keyBButton.SetChecked(0);
        }
        break;
    }

    case 3: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired joystick button.");
        keyAButton.SetChecked(0);
        keyBButton.SetChecked(0);
        mouseButton.SetChecked(0);

        if (zInput::Keyboard_WaitForAnyKeyPress(0) == 1) {
            captureState = 0;
            zInput::ResetAllTransitionState();
            joyButton.SetChecked(0);
            return;
        }

        const int buttonCode = zInput::DI_WaitForButtonPress(0);
        if (buttonCode != 0) {
            ApplyJoystickButtonRebind(
                buttonCode,
                joyButton.selectedBindingIndex
            );
            joyButton.SetChecked(0);
        }
        break;
    }

    case 4: {
        promptPanel.SetVisible(1);
        promptPanel.SetTextFmt("Press desired mouse button.");
        keyAButton.SetChecked(0);
        keyBButton.SetChecked(0);
        joyButton.SetChecked(0);

        if (zInput::Keyboard_WaitForAnyKeyPress(0) == 1) {
            captureState = 0;
            zInput::ResetAllTransitionState();
            joyButton.SetChecked(0);
            return;
        }

        const int buttonCode = zInput::Mouse_WaitForButtonPress(0);
        if (buttonCode != 0) {
            ApplyMouseButtonRebind(
                buttonCode,
                mouseButton.selectedBindingIndex
            );
            mouseButton.SetChecked(0);
            g_HudCmdMouseDebounceFrames = 10;
        }
        break;
    }

    default:
        break;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-applyprimarykeyrebind
 * @recoil-artifact defines .text recoil:function:0x40b3e0: HudCmdDialog::ApplyPrimaryKeyRebind.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudCmdDialog::ApplyPrimaryKeyRebind.
 */
int HudCmdDialog::ApplyPrimaryKeyRebind(
    int keyCode,
    int commandIndex
) {
    if (keyCode != 1) {
        const int primaryCommand = zInput::BindMapCurrent_GetCommandByPrimaryKey(keyCode);
        const int groupIndex = setList.selectedIndex;
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        if (primaryCommand == 0 && zInput::BindMapCurrent_GetCommandBySecondaryKey(keyCode) != 0) {
            zInput::BindMapCurrent_SetSecondaryKeyBinding(
                keyCode,
                0
            );
        }

        zInput::BindMapCurrent_SetPrimaryKeyBinding(
            keyCode,
            commandId
        );
        RebuildCommandBindingListsForGroup(groupIndex);
        OnCommandSelectionChanged(commandIndex);
    }

    captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-applysecondarykeyrebind
 * @recoil-artifact defines .text recoil:function:0x40b460: HudCmdDialog::ApplySecondaryKeyRebind.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudCmdDialog::ApplySecondaryKeyRebind.
 */
int HudCmdDialog::ApplySecondaryKeyRebind(
    int keyCode,
    int commandIndex
) {
    if (keyCode != 1) {
        const int secondaryCommand = zInput::BindMapCurrent_GetCommandBySecondaryKey(keyCode);
        const int groupIndex = setList.selectedIndex;
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        if (secondaryCommand == 0 && zInput::BindMapCurrent_GetCommandByPrimaryKey(keyCode) != 0) {
            zInput::BindMapCurrent_SetPrimaryKeyBinding(
                keyCode,
                0
            );
        }

        zInput::BindMapCurrent_SetSecondaryKeyBinding(
            keyCode,
            commandId
        );
        RebuildCommandBindingListsForGroup(groupIndex);
        OnCommandSelectionChanged(commandIndex);
    }

    captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-applyjoystickbuttonrebind
 * @recoil-artifact defines .text recoil:function:0x40b4e0: HudCmdDialog::ApplyJoystickButtonRebind.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudCmdDialog::ApplyJoystickButtonRebind.
 */
int HudCmdDialog::ApplyJoystickButtonRebind(
    int buttonCode,
    int commandIndex
) {
    const int joystickCommand = zInput::BindMapCurrent_GetCommandByJoystickSlot(buttonCode);
    const int groupIndex = setList.selectedIndex;
    const int commandId = zInput::BindGroupList_GetGroupCommandId(
        groupIndex,
        commandIndex
    );
    if (joystickCommand == 0 && zInput::BindMapCurrent_GetCommandByJoystickSlot(buttonCode) != 0) {
        zInput::BindMapCurrent_SetJoystickBinding(
            buttonCode,
            0
        );
    }

    zInput::BindMapCurrent_SetJoystickBinding(
        buttonCode,
        commandId
    );
    RebuildCommandBindingListsForGroup(groupIndex);
    OnCommandSelectionChanged(commandIndex);
    captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-applymousebuttonrebind
 * @recoil-artifact defines .text recoil:function:0x40b560: HudCmdDialog::ApplyMouseButtonRebind.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudCmdDialog::ApplyMouseButtonRebind.
 */
int HudCmdDialog::ApplyMouseButtonRebind(
    int buttonCode,
    int commandIndex
) {
    const int mouseCommand = zInput::BindMapCurrent_GetCommandByMouseSlot(buttonCode);
    const int groupIndex = setList.selectedIndex;
    const int commandId = zInput::BindGroupList_GetGroupCommandId(
        groupIndex,
        commandIndex
    );
    if (mouseCommand == 0 && zInput::BindMapCurrent_GetCommandByMouseSlot(buttonCode) != 0) {
        zInput::BindMapCurrent_SetMouseBinding(
            buttonCode,
            0
        );
    }

    zInput::BindMapCurrent_SetMouseBinding(
        buttonCode,
        commandId
    );
    RebuildCommandBindingListsForGroup(groupIndex);
    OnCommandSelectionChanged(commandIndex);
    captureState = 0;
    zInput::ResetAllTransitionState();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-selectgrouprelative
 * @recoil-artifact defines .text recoil:function:0x40b5e0: HudCmdDialog::SelectGroupRelative.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for HudCmdDialog::SelectGroupRelative.
 */
int HudCmdDialog::SelectGroupRelative(
    int delta
) {
    int groupIndex = setList.selectedIndex + delta;
    if (groupIndex >= setList.itemCount) {
        groupIndex = 0;
    } else if (groupIndex < 0) {
        groupIndex = setList.itemCount - 1;
    }

    setList.SetIndexClamped(groupIndex);
    const int selectedIndex = setList.selectedIndex;
    RebuildCommandBindingListsForGroup(selectedIndex);
    return selectedIndex;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-selectcommandrelative
 * @recoil-artifact defines .text recoil:function:0x40b630: HudCmdDialog::SelectCommandRelative.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for HudCmdDialog::SelectCommandRelative.
 */
int HudCmdDialog::SelectCommandRelative(
    int delta
) {
    int selectedIndex = delta;
    selectedIndex += commandList.selectedBindingIndex;
    if (selectedIndex >= 0) {
        HudCmdBindingEntry **const begin = commandList.bindingVec.begin();
        int count;
        if (begin == 0) {
            count = 0;
        } else {
            count = (int)(commandList.bindingVec.end() - begin);
        }
        if (selectedIndex < count) {
            commandList.SetSelectedEntry(selectedIndex);
        }
    }

    const int currentIndex = commandList.selectedBindingIndex;
    OnCommandSelectionChanged(currentIndex);
    return currentIndex;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-rebuildcommandbindinglistsforgroup
 * @recoil-artifact defines .text recoil:function:0x40b680: HudCmdDialog::RebuildCommandBindingListsForGroup.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for HudCmdDialog::RebuildCommandBindingListsForGroup.
 */
void HudCmdDialog::RebuildCommandBindingListsForGroup(
    int groupIndex
) {
    commandList.ClearBindingEntries();
    keyAButton.ClearBindingEntries();
    keyBButton.ClearBindingEntries();
    joyButton.ClearBindingEntries();
    mouseButton.ClearBindingEntries();

    int commandIndex;
    for (commandIndex = 0; commandIndex < zInput::BindGroupList_GetGroupCommandCount(groupIndex);
        ++commandIndex) {
        const int commandId = zInput::BindGroupList_GetGroupCommandId(
            groupIndex,
            commandIndex
        );
        char labelBuffer[40];
        zInput::BindMapCurrent_CopyCommandLabel(
            commandId,
            labelBuffer,
            sizeof(labelBuffer)
        );
        if (strlen(labelBuffer) != 0) {
            commandList.AddBindingEntry(
                zInput::BindMap_GetCommandLabel(commandId),
                commandId
            );
            keyAButton.AddBindingEntry(
                zInput::BindMapCurrent_FormatKeyComboName(
                    zInput::BindMapCurrent_GetPrimaryKeyboardKey(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            keyBButton.AddBindingEntry(
                zInput::BindMapCurrent_FormatKeyComboName(
                    zInput::BindMapCurrent_GetSecondaryKeyboardKey(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            joyButton.AddBindingEntry(
                zInput::BindMapCurrent_CopyJoystickButtonName(
                    zInput::BindMapCurrent_GetJoystickButtonSlot(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
            mouseButton.AddBindingEntry(
                zInput::BindMapCurrent_CopyMouseButtonName(
                    zInput::BindMapCurrent_GetMouseButtonSlot(commandId),
                    labelBuffer,
                    sizeof(labelBuffer)
                ),
                commandId
            );
        }
    }

    OnCommandSelectionChanged(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdresetbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x40b930: HudCmdResetButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdResetButton::OnActivate.
 */
void HudCmdResetButton::OnActivate() {
    HudCmdDialog *const dialog = (HudCmdDialog *)(owner);
    zInput::BindMap_InitDefaultBindings();
    zInput::BindMap_Current_RebuildLookupIndices();
    dialog->RebuildCommandBindingListsForGroup(dialog->setList.selectedIndex);
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdsetlistwidget-onactivate
 * @recoil-artifact defines .text recoil:function:0x40b960: HudCmdSetListWidget::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Advance the set-list selector and rebuild command bindings for the
 * selected group.
 */
void HudCmdSetListWidget::OnActivate() {
    AdvanceSelectionAndActivate();
    ((HudCmdDialog *)(owner))->RebuildCommandBindingListsForGroup(selectedIndex);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialog-oncommandselectionchanged
 * @recoil-artifact defines .text recoil:function:0x40b980: HudCmdDialog::OnCommandSelectionChanged.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Binary Ninja clears the description panel capture state, resets zInput
 * transition state, selects the same entry in each command binding list, then
 * resolves the selected command hint through zInput::BindMap_GetCommandHint.
 * Purpose: Refresh the command dialog selection and description text.
 */
void HudCmdDialog::OnCommandSelectionChanged(
    int commandIndex
) {
    captureState = 0;
    zInput::ResetAllTransitionState();
    HudCmdBindButtonBase *const commandButton = &commandList;
    commandButton->SetSelectedEntry(commandIndex);
    keyAButton.SetSelectedEntry(commandIndex);
    keyBButton.SetSelectedEntry(commandIndex);
    joyButton.SetSelectedEntry(commandIndex);
    mouseButton.SetSelectedEntry(commandIndex);

    HudCmdBindingEntry **const entries = commandButton->bindingVec.begin();
    HudCmdBindingEntry *const selectedEntry = entries[commandButton->selectedBindingIndex];
    char *const hint = zInput::BindMap_GetCommandHint(selectedEntry->commandId);
    if (hint != 0) {
        descriptionPanel.SetTextFmt(
            "%s",
            hint
        );
    } else {
        descriptionPanel.SetTextFmt("");
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdkeyabutton-onbegincapture
 * @recoil-artifact defines .text recoil:function:0x40ba30: HudCmdKeyAButton::OnBeginCapture.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdKeyAButton::OnBeginCapture.
 */
void HudCmdKeyAButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->captureState = 1;
    zInput::ResetAllTransitionState();
    HudUiCheckToggleWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdkeyabutton-onclearbinding
 * @recoil-artifact defines .text recoil:function:0x40ba60: HudCmdKeyAButton::OnClearBinding.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: clear the primary-key binding for the selected command row.
 */
void HudCmdKeyAButton::OnClearBinding() {
    const int selectedIndex = selectedBindingIndex;
    ((HudCmdDialog *)(owner))->ApplyPrimaryKeyRebind(
        0,
        selectedIndex
    );
    SetSelectedEntry(selectedIndex);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdbindbuttonbase-onselectionchangedrefresh
 * @recoil-artifact defines .text recoil:function:0x40ba90: HudCmdBindButtonBase::OnSelectionChangedRefresh.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: forward a bind-button selection change to the owning command dialog.
 */
void HudCmdBindButtonBase::OnSelectionChangedRefresh(
    int selectedIndex
) {
    ((HudCmdDialog *)(owner))->OnCommandSelectionChanged(selectedIndex);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdkeybbutton-onbegincapture
 * @recoil-artifact defines .text recoil:function:0x40bab0: HudCmdKeyBButton::OnBeginCapture.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdKeyBButton::OnBeginCapture.
 */
void HudCmdKeyBButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->captureState = 2;
    zInput::ResetAllTransitionState();
    HudUiCheckToggleWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdkeybbutton-onclearbinding
 * @recoil-artifact defines .text recoil:function:0x40bae0: HudCmdKeyBButton::OnClearBinding.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: clear the secondary-key binding for the selected command row.
 */
void HudCmdKeyBButton::OnClearBinding() {
    ((HudCmdDialog *)(owner))->ApplySecondaryKeyRebind(
        0,
        selectedBindingIndex
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdjoybutton-onbegincapture
 * @recoil-artifact defines .text recoil:function:0x40bb00: HudCmdJoyButton::OnBeginCapture.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdJoyButton::OnBeginCapture.
 */
void HudCmdJoyButton::OnBeginCapture() {
    ((HudCmdDialog *)(owner))->captureState = 3;
    zInput::ResetAllTransitionState();
    HudUiCheckToggleWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdjoybutton-onclearbinding
 * @recoil-artifact defines .text recoil:function:0x40bb30: HudCmdJoyButton::OnClearBinding.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: clear the joystick binding for the selected command row.
 */
void HudCmdJoyButton::OnClearBinding() {
    ((HudCmdDialog *)(owner))
        ->ApplyJoystickButtonRebind(
            0,
            selectedBindingIndex
        );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdmousebutton-onbegincapture
 * @recoil-artifact defines .text recoil:function:0x40bb50: HudCmdMouseButton::OnBeginCapture.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdMouseButton::OnBeginCapture.
 */
void HudCmdMouseButton::OnBeginCapture() {
    if (g_HudCmdMouseDebounceFrames > 0) {
        return;
    }

    ((HudCmdDialog *)(owner))->captureState = 4;
    zInput::ResetAllTransitionState();
    HudUiCheckToggleWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdmousebutton-onclearbinding
 * @recoil-artifact defines .text recoil:function:0x40bb80: HudCmdMouseButton::OnClearBinding.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: clear the mouse binding for the selected command row when debounce is inactive.
 */
void HudCmdMouseButton::OnClearBinding() {
    if (g_HudCmdMouseDebounceFrames > 0) {
        return;
    }

    ((HudCmdDialog *)(owner))->ApplyMouseButtonRebind(
        0,
        selectedBindingIndex
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdnextsetbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x40bba0: HudCmdNextSetButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdNextSetButton::OnActivate.
 */
void HudCmdNextSetButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectGroupRelative(1);
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdprevsetbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x40bbc0: HudCmdPrevSetButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdPrevSetButton::OnActivate.
 */
void HudCmdPrevSetButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectGroupRelative(-1);
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdnextcommandbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x40bbe0: HudCmdNextCommandButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdNextCommandButton::OnActivate.
 */
void HudCmdNextCommandButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectCommandRelative(1);
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdprevcommandbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x40bc00: HudCmdPrevCommandButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: handle the recovered HUD event path for HudCmdPrevCommandButton::OnActivate.
 */
void HudCmdPrevCommandButton::OnActivate() {
    ((HudCmdDialog *)(owner))->SelectCommandRelative(-1);
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x40bc20: HudCmdDialogState::StaticInitAndRegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Construct the global command-dialog state and register its at-exit teardown.
 */
void HudCmdDialogState::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-staticinit
 * @recoil-artifact defines .text recoil:function:0x40bc30: HudCmdDialogState::StaticInit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Construct the command-dialog state in its static storage.
 */
HudCmdDialogState *HudCmdDialogState::StaticInit() {
    return new (&g_HudCmdDialogState) HudCmdDialogState;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-registeratexit
 * @recoil-artifact defines .text recoil:function:0x40bc40: HudCmdDialogState::RegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Register the command-dialog state static destructor with the CRT.
 */
void HudCmdDialogState::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x40bc50: HudCmdDialogState::AtExitDestructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Destroy the global command-dialog state during CRT at-exit cleanup.
 */
void HudCmdDialogState::AtExitDestructor() {
    g_HudCmdDialogState.HudCmdDialogState::~HudCmdDialogState();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *HudCmdDialogStateCrtInitializerFn)();
/* VC5 emits this command-dialog startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
HudCmdDialogStateCrtInitializerFn s_HudCmdDialogStateCrtInit =
    HudCmdDialogState::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-hudcmddialogstate
 * @recoil-artifact defines .text recoil:function:0x40bc60: HudCmdDialogState::HudCmdDialogState.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Initialize the command-dialog app state with no active dialog.
 */
HudCmdDialogState::HudCmdDialogState() {
    m_dialog = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-destructor-hudcmddialogstate
 * @recoil-artifact defines .text recoil:function:0x40bc90: HudCmdDialogState::~HudCmdDialogState.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Delete any active command dialog owned by the state during teardown.
 */
HudCmdDialogState::~HudCmdDialogState() {
    HudCmdDialog *const dialog = (HudCmdDialog *)m_dialog;
    if (dialog != 0) {
        delete dialog;
        m_dialog = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-ontrybecomecurrent
 * @recoil-artifact defines .text recoil:function:0x40bcf0: HudCmdDialogState::OnTryBecomeCurrent.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Allocate the 0xce00-byte command dialog, construct and store it,
 * enable it, suspend keyboard input, and accept the state transition.
 */
int HudCmdDialogState::OnTryBecomeCurrent() {
    HudCmdDialog *dialog = new HudCmdDialog;
    m_dialog = dialog;

    dialog->SetEnabled(1);
    zInput::Keyboard_Suspend();
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-ondeactivate
 * @recoil-artifact defines .text recoil:function:0x40bd60: HudCmdDialogState::OnDeactivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Resume keyboard input, disable and dispose the active command
 * dialog, clear it, and rebuild current input-map lookup indices.
 */
void HudCmdDialogState::OnDeactivate() {
    zInput::Keyboard_ResumeFromSuspend();

    HudCmdDialog *dialog = (HudCmdDialog *)m_dialog;
    if (dialog == 0) {
        return;
    }

    dialog->SetEnabled(0);
    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();

    dialog = (HudCmdDialog *)m_dialog;
    if (dialog != 0) {
        delete dialog;
    }

    m_dialog = 0;
    zInput::BindMap_Current_RebuildLookupIndices();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmddialogstate-queueenter
 * @recoil-artifact defines .text recoil:function:0x40bda0: HudCmdDialogState::QueueEnter.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudCmdDialog.cpp.
 * Purpose: Queue the global command-dialog app state for entry.
 */
void HudCmdDialogState::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudCmdDialogState,
        0
    );
}

/**
 * Original function; retail address 0x40bdc0.
 * Purpose: preserve the recovered HUD behavior for StdPtrVector::ClearNoOpDestroy.
 */
void StdPtrVector::ClearNoOpDestroy(
    int *begin,
    int *end
) {
    (void)begin;
    (void)end;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanel-invalidate
 * @recoil-artifact defines .text recoil:function:0x40be90: HudUiPanel::Invalidate.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::Invalidate.
 */
void HudUiPanel::Invalidate() {
    textDirty = 1;
    HudUiElement::Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanel-getfont
 * @recoil-artifact defines .text recoil:function:0x40bea0: HudUiPanel::GetFont.
 * Purpose: return the recovered HUD value exposed by HudUiPanel::GetFont.
 */
HGDIOBJ HudUiPanel::GetFont() {
    return hFont;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanel-setfonthandle
 * @recoil-artifact defines .text recoil:function:0x40beb0: HudUiPanel::SetFontHandle.
 * Purpose: apply the recovered HUD state change handled by HudUiPanel::SetFontHandle.
 */
void HudUiPanel::SetFontHandle(
    HGDIOBJ fontHandle
) {
    hFont = fontHandle;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanel-enablewordwrapwithrect
 * @recoil-artifact defines .text recoil:function:0x40bec0: HudUiPanel::EnableWordWrapWithRect.
 * Purpose: preserve the recovered HUD behavior for HudUiPanel::EnableWordWrapWithRect.
 */
void HudUiPanel::EnableWordWrapWithRect(
    const HudUiRect *rect
) {
    wordWrapEnabled = 1;
    wrapRect = *rect;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdbindingentry-destructor-hudcmdbindingentry
 * @recoil-artifact defines .text recoil:function:0x40bf00: HudCmdBindingEntry::~HudCmdBindingEntry.
 * Binary Ninja shows six ordinary destructor calls from the five concrete
 * bind-button destructors and the addressable base destructor.
 * Purpose: release the entry-owned display string before scalar delete.
 */
inline HudCmdBindingEntry::~HudCmdBindingEntry() {
    if (displayText != 0) {
        free(displayText);
        displayText = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdbindingentrydelete-operator
 * @recoil-artifact defines .text recoil:function:0x40bf20: HudCmdBindingEntryDelete::operator().
 * Purpose: delete one binding entry and replace its vector slot with null.
 */
inline HudCmdBindingEntry *HudCmdBindingEntryDelete::operator()(
    HudCmdBindingEntry *entry
) const {
    delete entry;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdbindbuttonbase-addbindingentry
 * @recoil-artifact defines .text recoil:function:0x40bf80: HudCmdBindButtonBase::AddBindingEntry.
 * Binary Ninja shows the HudCmdBindButton.cpp method allocating a
 * HudCmdBindingEntry, duplicating the display text, assigning the command id,
 * and appending it to the binding vector with growth when capacity is full.
 * Purpose: preserve the recovered HUD behavior for HudCmdBindButtonBase::AddBindingEntry.
 */
int HudCmdBindButtonBase::AddBindingEntry(
    const char *displayText,
    int commandId
) {
    const int oldCount = (int)bindingVec.size();
    HudCmdBindingEntry *const entry = new HudCmdBindingEntry(
        displayText,
        commandId
    );
    bindingVec.push_back(entry);
    return oldCount;
}

/**
 * Compiler-emitted 0x40bdc0: canonical VC5
 * std::vector<HudCmdBindingEntry *>::clear provider selected by
 * bindingVec.clear().
 * Compiler-emitted 0x40be00: canonical VC5 std::transform specialization
 * selected by the entry-deletion pass.
 * Compiler-emitted 0x40be60: canonical VC5 std::copy specialization
 * selected by vector::clear().
 *
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdbindbuttonbase-clearbindingentries
 * @recoil-artifact defines .text recoil:function:0x40c1d0: HudCmdBindButtonBase::ClearBindingEntries.
 * Purpose: delete and null every owned entry, then clear the pointer range.
 */
inline void HudCmdBindButtonBase::ClearBindingEntries() {
    std::transform(
        bindingVec.begin(),
        bindingVec.end(),
        bindingVec.begin(),
        HudCmdBindingEntryDelete()
    );
#if defined(_MSC_VER) && _MSC_VER < 1200
    bindingVec.clear();
#else
    bindingVec.erase(bindingVec.begin(), bindingVec.end());
#endif
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudcmdbindbuttonbase-destructor-hudcmdbindbuttonbase
 * @recoil-artifact defines .text recoil:function:0x40c280: HudCmdBindButtonBase::~HudCmdBindButtonBase.
 * Purpose: run the optimizer-visible entry cleanup before ordinary vector,
 * panel, and widget-base lifetime teardown.
 */
inline HudCmdBindButtonBase::~HudCmdBindButtonBase() {
    ClearBindingEntries();
}

namespace {
typedef HRESULT(WINAPI *zDirectDrawCreateFn)(
    GUID *,
    LPDIRECTDRAW *,
    IUnknown *
);
typedef HMODULE(__stdcall *zLoadLibraryAFn)(const char *);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probecreateprimarysurfacefailedmsg
 * @recoil-artifact defines .data recoil:data:0x4daaf0: g_zSys_ProbeCreatePrimarySurfaceFailedMsg.
 * Purpose: Reports primary DirectDraw surface creation failure during the platform probe.
 */
const char g_zSys_ProbeCreatePrimarySurfaceFailedMsg[] = "Couldn't CreateSurface\r\n";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probesetcooplevelfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4dab0c: g_zSys_ProbeSetCoopLevelFailedMsg.
 * Purpose: Reports DirectDraw cooperative-level setup failure during the platform probe.
 */
const char g_zSys_ProbeSetCoopLevelFailedMsg[] = "Couldn't Set coop level\r\n";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probeqiddraw2failedmsg
 * @recoil-artifact defines .data recoil:data:0x4dab28: g_zSys_ProbeQiDdraw2FailedMsg.
 * Purpose: Reports failure to query the DirectDraw2 interface during the platform probe.
 */
const char g_zSys_ProbeQiDdraw2FailedMsg[] = "Couldn't QI DDraw2\r\n";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probecreateddrawfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4dab40: g_zSys_ProbeCreateDdrawFailedMsg.
 * Purpose: Reports DirectDrawCreate failure during the platform probe.
 */
const char g_zSys_ProbeCreateDdrawFailedMsg[] = "Couldn't create DDraw\r\n";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probeloadddrawfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4dab58: g_zSys_ProbeLoadDdrawFailedMsg.
 * Purpose: Reports missing DirectDraw library support during the platform probe.
 */
const char g_zSys_ProbeLoadDdrawFailedMsg[] = "Couldn't LoadLibrary DDraw\r\n";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probedirectdrawcreateexportname
 * @recoil-artifact defines .data recoil:data:0x4dab78: g_zSys_ProbeDirectDrawCreateExportName.
 * Purpose: Names the DirectDrawCreate export resolved from DDRAW.DLL.
 */
const char g_zSys_ProbeDirectDrawCreateExportName[] = "DirectDrawCreate";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probeddrawdllname
 * @recoil-artifact defines .data recoil:data:0x4dab8c: g_zSys_ProbeDdrawDllName.
 * Purpose: Names the DirectDraw provider DLL loaded by the platform probe.
 */
const char g_zSys_ProbeDdrawDllName[] = "DDRAW.DLL";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probemissingdirectinputcreatemsg
 * @recoil-artifact defines .data recoil:data:0x4dab98: g_zSys_ProbeMissingDirectInputCreateMsg.
 * Purpose: Reports missing DirectInputCreateA export support during the platform probe.
 */
const char g_zSys_ProbeMissingDirectInputCreateMsg[] =
    "Couldn't GetProcAddress DInputCreate\r\n";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probedirectinputcreateexportname
 * @recoil-artifact defines .data recoil:data:0x4dabc0: g_zSys_ProbeDirectInputCreateExportName.
 * Purpose: Names the DirectInputCreateA export resolved from DINPUT.DLL.
 */
const char g_zSys_ProbeDirectInputCreateExportName[] = "DirectInputCreateA";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probeloaddinputfailedmsg
 * @recoil-artifact defines .data recoil:data:0x4dabd4: g_zSys_ProbeLoadDinputFailedMsg.
 * Purpose: Reports missing DirectInput library support during the platform probe.
 */
const char g_zSys_ProbeLoadDinputFailedMsg[] = "Couldn't LoadLibrary DInput\r\n";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-zsys-probedinputdllname
 * @recoil-artifact defines .data recoil:data:0x4dabf4: g_zSys_ProbeDinputDllName.
 * Purpose: Names the DirectInput provider DLL loaded by the platform probe.
 */
const char g_zSys_ProbeDinputDllName[] = "DINPUT.DLL";
} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zsys-probeplatformandvideocaps
 * @recoil-artifact defines .text recoil:function:0x40c370: zSys::ProbePlatformAndVideoCaps.
 *
 * Purpose: probe Windows, DirectDraw, DirectDrawSurface, and DirectInput
 * availability to classify the runtime platform and video capability levels.
 */
RECOIL_NO_GS void __fastcall zSys::ProbePlatformAndVideoCaps(
    zSysVideoCapsLevel *outVideoCaps,
    zSysPlatformCapsLevel *outPlatformCaps
) {
    OSVERSIONINFOA osVer;
    LPDIRECTDRAW pDDraw = 0;
    LPDIRECTDRAW2 pDDraw2 = 0;
    LPDIRECTDRAWSURFACE pSurface = 0;
    LPDIRECTDRAWSURFACE3 pSurface3 = 0;
    LPDIRECTDRAWSURFACE4 pSurface4 = 0;

    osVer.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    RECOIL_STATIC_ASSERT(sizeof(OSVERSIONINFOA) == 0x94);

    if (GetVersionExA(&osVer) == 0) {
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
        return;
    }

    if (osVer.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_NT4_PLUS;
        if (osVer.dwMajorVersion < 4) {
            *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
            return;
        }

        if (osVer.dwMajorVersion == 4) {
            *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW2;
            HMODULE dinputModule = LoadLibraryA(g_zSys_ProbeDinputDllName);
            if (dinputModule == 0) {
                OutputDebugStringA(g_zSys_ProbeLoadDinputFailedMsg);
                return;
            }

            FARPROC directInputCreate = GetProcAddress(
                dinputModule,
                g_zSys_ProbeDirectInputCreateExportName
            );
            FreeLibrary(dinputModule);
            if (directInputCreate == 0) {
                OutputDebugStringA(g_zSys_ProbeMissingDirectInputCreateMsg);
                return;
            }

            *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW2_DINPUT;
            return;
        }
    } else {
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_NON_NT;
    }

    zLoadLibraryAFn loadLibrary = LoadLibraryA;
    HMODULE ddrawModule = loadLibrary(g_zSys_ProbeDdrawDllName);
    if (ddrawModule == 0) {
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
        FreeLibrary(ddrawModule);
        return;
    }

    zDirectDrawCreateFn directDrawCreate =
        (zDirectDrawCreateFn)GetProcAddress(
            ddrawModule,
            g_zSys_ProbeDirectDrawCreateExportName
        );
    if (directDrawCreate == 0) {
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
        FreeLibrary(ddrawModule);
        OutputDebugStringA(g_zSys_ProbeLoadDdrawFailedMsg);
        return;
    }

    if (directDrawCreate(
        0,
        &pDDraw,
        0
    ) < 0) {
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        *outPlatformCaps = ZSYS_PLATFORM_CAPS_UNSUPPORTED;
        FreeLibrary(ddrawModule);
        OutputDebugStringA(g_zSys_ProbeCreateDdrawFailedMsg);
        return;
    }

    *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW;
    if (IDirectDraw_QueryInterface(
        pDDraw,
        IID_IDirectDraw2,
        (void **)&pDDraw2
    ) < 0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        OutputDebugStringA(g_zSys_ProbeQiDdraw2FailedMsg);
        return;
    }

    IDirectDraw2_Release(pDDraw2);
    *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW2;

    register HMODULE dinputModule = loadLibrary(g_zSys_ProbeDinputDllName);
    HMODULE dinputHandle = dinputModule;
    if (dinputHandle == 0) {
        OutputDebugStringA(g_zSys_ProbeLoadDinputFailedMsg);
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        return;
    }

    FARPROC directInputCreate = GetProcAddress(
        dinputHandle,
        g_zSys_ProbeDirectInputCreateExportName
    );
    dinputModule = (HMODULE)directInputCreate;
    FreeLibrary(dinputHandle);
    if (dinputModule == 0) {
        FreeLibrary(ddrawModule);
        IDirectDraw_Release(pDDraw);
        OutputDebugStringA(g_zSys_ProbeMissingDirectInputCreateMsg);
        return;
    }

    *outVideoCaps = ZSYS_VIDEO_CAPS_DDRAW2_DINPUT;

    DDSURFACEDESC desc;
    memset(
        &desc,
        0,
        sizeof(desc)
    );
    RECOIL_STATIC_ASSERT(sizeof(DDSURFACEDESC) == 0x6c);
    desc.dwSize = sizeof(DDSURFACEDESC);
    desc.dwFlags = DDSD_CAPS;
    desc.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

    if (IDirectDraw_SetCooperativeLevel(
        pDDraw,
        0,
        DDSCL_NORMAL
    ) < 0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        OutputDebugStringA(g_zSys_ProbeSetCoopLevelFailedMsg);
        return;
    }

    if (IDirectDraw_CreateSurface(
        pDDraw,
        &desc,
        &pSurface,
        0
    ) < 0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        *outVideoCaps = ZSYS_VIDEO_CAPS_NONE;
        OutputDebugStringA(g_zSys_ProbeCreatePrimarySurfaceFailedMsg);
        return;
    }

    if (IDirectDrawSurface_QueryInterface(pSurface, IID_IDirectDrawSurface3, (void **)&pSurface3) <
        0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        return;
    }

    *outVideoCaps = ZSYS_VIDEO_CAPS_SURFACE3;
    if (IDirectDrawSurface_QueryInterface(pSurface, IID_IDirectDrawSurface4, (void **)&pSurface4) <
        0) {
        IDirectDraw_Release(pDDraw);
        FreeLibrary(ddrawModule);
        return;
    }

    *outVideoCaps = ZSYS_VIDEO_CAPS_SURFACE4;
    IDirectDrawSurface_Release(pSurface);
    IDirectDraw_Release(pDDraw);
    FreeLibrary(ddrawModule);
}

/**
 * HudOptionsDialog owner evidence: BN constructor 0x40c720 builds the
 * HudUiBackground base, constructs the typed options-panel child widgets,
 * installs their derived dispatch identities, binds the dialog.zrd node names,
 * and destructor 0x40cf60 destroys the same members in reverse order. The
 * retail HudUiOptionsPanel_* dispatch data is compiler-emitted class evidence,
 * not production-source permission for FTable records or table factories.
 */

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspanelbackbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x40c6e0: HudUiOptionsPanelBackButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.hudoptionsdialog-hudoptionsdialog
 * @recoil-artifact defines .text recoil:function:0x40c720: HudOptionsDialog::HudOptionsDialog.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
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
 * Observed caller: 0x40caa0.
 * Evidence: HudUiCheckToggleWidget::OnActivateThunk invokes the bound toggle's
 * virtual OnActivate method.
 * Purpose: toggle the global lighting graphics flag from the lighting checkbox.
 */
void HudUiOptionsPanel_Lighting::OnActivate() {
    HudUiOptionsPanel_Lighting::SyncFromOptions();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the lighting toggle from the active hardware-mode graphics flags.
 */
void HudUiOptionsPanel_Lighting::PostLoadFromZrd() {
    HudUiOptionsPanel_Lighting::InitFromOptions();
}

/**
 * Original function; retail address 0x40c9c0.
 * Purpose: synchronize the lighting toggle from the active hardware-mode graphics flags.
 */
void HudUiOptionsPanel_Lighting::InitFromOptions() {
    SetChecked(zOpt::GetGraphicsFlagsForCurrentHwMode() & ZOPT_GRAPHICS_GLOBAL_LIGHT);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspanel-lighting-syncfromoptions
 * @recoil-artifact defines .text recoil:function:0x40c9e0: HudUiOptionsPanel_Lighting::SyncFromOptions.
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
 * Observed caller: 0x40caa0.
 * Evidence: HudUiCheckToggleWidget::OnActivateThunk invokes the bound toggle's
 * virtual OnActivate method.
 * Purpose: route activation through the recovered perspective option sync.
 */
void HudUiOptionsPanel_Perspective::OnActivate() {
    HudUiOptionsPanel_Perspective::SyncFromOptions();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the perspective toggle from the active hardware-mode graphics flags.
 */
void HudUiOptionsPanel_Perspective::PostLoadFromZrd() {
    HudUiOptionsPanel_Perspective::InitFromOptions();
}

/**
 * Original function; retail address 0x40ca20.
 * Purpose: synchronize the perspective toggle from the active hardware-mode graphics flags.
 */
void HudUiOptionsPanel_Perspective::InitFromOptions() {
    SetChecked(zOpt::GetGraphicsFlagsForCurrentHwMode() & ZOPT_GRAPHICS_PERSPECTIVE);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspanel-perspective-syncfromoptions
 * @recoil-artifact defines .text recoil:function:0x40ca40: HudUiOptionsPanel_Perspective::SyncFromOptions.
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
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the full-HUD toggle from the active hardware-mode HUD type.
 */
void HudUiOptionsPanel_FullHud::PostLoadFromZrd() {
    HudUiOptionsPanel_FullHud::InitFromOptions();
}

/**
 * Original function; retail address 0x40ca80.
 * Purpose: synchronize the full-HUD toggle from the active hardware-mode HUD type.
 */
void HudUiOptionsPanel_FullHud::InitFromOptions() {
    SetChecked(zOpt::GetHudTypeForCurrentHwMode() == ZOPT_HUD_TYPE_PERSPECTIVE);
}

/**
 * Original function; shared retail body 0x40caa0.
 * Evidence: the body is shared with HudUiCheckToggleWidget::OnActivateThunk.
 * Purpose: run inherited toggle activation for the full-HUD option.
 */
void HudUiOptionsPanel_FullHud::OnActivate() {
    HudUiCheckToggleWidget::OnActivate();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4b87f0.
 * Evidence: HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf invokes the owning
 * selector's virtual OnActivate method.
 * Purpose: route activation through the recovered object-detail option sync.
 */
void HudUiOptionsPanel_ObjectDetail::OnActivate() {
    HudUiOptionsPanel_ObjectDetail::SyncFromOptions();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the object detail selector from the active hardware-mode object LOD.
 */
void HudUiOptionsPanel_ObjectDetail::PostLoadFromZrd() {
    HudUiOptionsPanel_ObjectDetail::InitFromOptions();
}

/**
 * Original function; retail address 0x40cab0.
 * Purpose: synchronize the object detail selector from the active hardware-mode object LOD.
 */
void HudUiOptionsPanel_ObjectDetail::InitFromOptions() {
    SetIndexClamped(zOpt::GetObjectLODForCurrentHwMode());
}

/**
 * Original function; retail address 0x40cad0.
 * Purpose: advance the object detail selector and store its object LOD option.
 */
void HudUiOptionsPanel_ObjectDetail::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetObjectLODForCurrentHwMode(selectedIndex);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4b87f0.
 * Evidence: HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf invokes the owning
 * selector's virtual OnActivate method.
 * Purpose: route activation through the recovered texture-memory option sync.
 */
void HudUiOptionsPanel_TextureMemory::OnActivate() {
    SyncFromOptions();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the texture memory selector from the active hardware-mode option.
 */
void HudUiOptionsPanel_TextureMemory::PostLoadFromZrd() {
    InitFromOptions();
}

/**
 * Original function; retail address 0x40caf0.
 * Purpose: synchronize the texture memory selector from the active hardware-mode option.
 */
void HudUiOptionsPanel_TextureMemory::InitFromOptions() {
    SetIndexClamped(zOpt::GetTextureMemoryForCurrentHwMode());
}

/**
 * Original function; retail address 0x40cb10.
 * Purpose: advance the texture memory selector and store its option.
 */
void HudUiOptionsPanel_TextureMemory::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetTextureMemoryForCurrentHwMode(selectedIndex);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4b87f0.
 * Evidence: HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf invokes the owning
 * selector's virtual OnActivate method.
 * Purpose: route activation through the recovered effects option sync.
 */
void HudUiOptionsPanel_Effects::OnActivate() {
    SyncFromOptions();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the effects selector and constrain software-renderer choices.
 */
void HudUiOptionsPanel_Effects::PostLoadFromZrd() {
    InitFromOptions();
}

/**
 * Original function; retail address 0x40cb30.
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
 * Original function; retail address 0x40cb70.
 * Purpose: advance the effects selector and store its effects-level option.
 */
void HudUiOptionsPanel_Effects::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetEffectsLevelForCurrentHwMode(selectedIndex);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x40caa0.
 * Evidence: HudUiCheckToggleWidget::OnActivateThunk invokes the bound toggle's
 * virtual OnActivate method.
 * Purpose: route activation through the recovered sound-active option sync.
 */
void HudUiOptionsPanel_SoundActive::OnActivate() {
    SyncFromOptions();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the sound-active toggle from the mute-sound option.
 */
void HudUiOptionsPanel_SoundActive::PostLoadFromZrd() {
    InitFromOptions();
}

/**
 * Original function; retail address 0x40cb90.
 * Purpose: synchronize the sound-active toggle from the mute-sound option.
 */
void HudUiOptionsPanel_SoundActive::InitFromOptions() {
    SetChecked(zOpt::GetMuteSoundOption() == 0);
}

/**
 * Original function; retail address 0x40cbb0.
 * Purpose: toggle sound activity and store the inverse mute-sound option.
 */
void HudUiOptionsPanel_SoundActive::SyncFromOptions() {
    HudUiCheckToggleWidget::OnActivate();
    zOpt::SetMuteSoundOption(checked == 0);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4b87f0.
 * Evidence: HudUiZrdWidgetEx17C_Item::OnActivateSelectSelf invokes the owning
 * selector's virtual OnActivate method.
 * Purpose: route activation through the recovered sound-quality option sync.
 */
void HudUiOptionsPanel_SoundQuality::OnActivate() {
    SyncFromOptions();
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the sound quality selector from the sound LOD option.
 */
void HudUiOptionsPanel_SoundQuality::PostLoadFromZrd() {
    InitFromOptions();
}

/**
 * Original function; retail address 0x40cbd0.
 * Purpose: synchronize the sound quality selector from the sound LOD option.
 */
void HudUiOptionsPanel_SoundQuality::InitFromOptions() {
    SetIndexClamped(zOpt::GetSoundLODOption());
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspanel-soundquality-syncfromoptions
 * @recoil-artifact defines .text recoil:function:0x40cbf0: HudUiOptionsPanel_SoundQuality::SyncFromOptions.
 * Purpose: advance the sound quality selector and store its sound LOD option.
 */
void HudUiOptionsPanel_SoundQuality::SyncFromOptions() {
    AdvanceSelectionAndActivate();
    zOpt::SetSoundLODOption(selectedIndex);
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the sound volume fill widget from the stored sound volume option.
 */
void HudUiOptionsPanel_SoundVolume::PostLoadFromZrd() {
    SyncFromOptions();
}

/**
 * Original function; retail address 0x40cc10.
 * Purpose: synchronize the sound volume fill widget from the stored sound volume option.
 */
void HudUiOptionsPanel_SoundVolume::SyncFromOptions() {
    SetNormalizedValueAndRebuild(zOpt::GetSoundVolumeOption());
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspanel-soundvolume-onactivate
 * @recoil-artifact defines .text recoil:function:0x40cc30: HudUiOptionsPanel_SoundVolume::OnActivate.
 * Purpose: update and store sound volume from the fill-widget cursor position.
 */
void HudUiOptionsPanel_SoundVolume::OnActivate() {
    UpdateNormalizedFromCursor();
    zOpt::SetSoundVolumeOption(normalizedValue);
    SetNormalizedValueAndRebuild(zOpt::GetSoundVolumeOption());
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the CD-audio toggle from the stored music-enable option.
 */
void HudUiOptionsPanel_MusicEnable::PostLoadFromZrd() {
    SyncFromOptions();
}

/**
 * Original function; retail address 0x40cc60.
 * Purpose: synchronize the CD-audio toggle from the stored music-enable option.
 */
void HudUiOptionsPanel_MusicEnable::SyncFromOptions() {
    SetChecked(zSnd::GetCDAudioOption());
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspanel-musicenable-onactivate
 * @recoil-artifact defines .text recoil:function:0x40cc80: HudUiOptionsPanel_MusicEnable::OnActivate.
 * Purpose: toggle CD audio playback and store the music-enable option.
 */
void HudUiOptionsPanel_MusicEnable::OnActivate() {
    HudUiCheckToggleWidget::OnActivate();
    if (checked != 0) {
        zSnd::SetCDAudioOption(1);
        zSndCd::PlayTrackWithMode(
            2,
            5
        );
    } else {
        zSnd::SetCDAudioOption(0);
        zSndCd::Stop();
    }
}

/**
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize the music volume fill widget from the current CD volume.
 */
void HudUiOptionsPanel_MusicVolume::PostLoadFromZrd() {
    SyncFromOptions();
}

/**
 * Original function; retail address 0x40ccc0.
 * Purpose: synchronize the music volume fill widget from the current CD volume.
 */
void HudUiOptionsPanel_MusicVolume::SyncFromOptions() {
    unsigned short primaryVolume = 0;
    unsigned short secondaryVolume = 0;
    zSndCd::GetVolume(
        &primaryVolume,
        &secondaryVolume
    );
    SetNormalizedValueAndRebuild((float)(primaryVolume)*ZSND_CD_VOLUME_TO_NORMALIZED);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspanel-musicvolume-onactivate
 * @recoil-artifact defines .text recoil:function:0x40cd00: HudUiOptionsPanel_MusicVolume::OnActivate.
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
 * Original-source helper; no standalone retail function exists.
 * Observed caller: 0x4ba0c0.
 * Evidence: HudUiBackground::BindWidgetByName invokes the bound widget's
 * virtual PostLoadFromZrd method after loading the ZRD node.
 * Purpose: synchronize and constrain the resolution selector for the active renderer.
 */
void HudUiOptionsPanel_Resolution::PostLoadFromZrd() {
    SyncFromOptions();
}

/**
 * Original function; retail address 0x40cd30.
 * Purpose: synchronize and constrain the resolution selector for the active renderer.
 */
void HudUiOptionsPanel_Resolution::SyncFromOptions() {
    const int accelerationOption = zVid::GetAccelerationOption();

    if (accelerationOption == ZVID_HW_MODE_SOFTWARE) {
        const int modeCase = zVid::GetVideoModeIndexFromOptions() - 2;
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

    const int modeCase = zVid::GetVideoModeIndexFromOptions() - 2;
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspanel-resolution-onactivate
 * @recoil-artifact defines .text recoil:function:0x40ce80: HudUiOptionsPanel_Resolution::OnActivate.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.hudoptionsdialog-destructor-hudoptionsdialog
 * @recoil-artifact defines .text recoil:function:0x40cf60: HudOptionsDialog::~HudOptionsDialog.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: let VC5 emit the options dialog member/base teardown state machine.
 */
HudOptionsDialog::~HudOptionsDialog() {
}


/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspaneloverlayowner-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x40d070: HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Construct the global options overlay owner and register its exit cleanup.
 */
void HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspaneloverlayowner-staticinit
 * @recoil-artifact defines .text recoil:function:0x40d080: HudUiOptionsPanelOverlayOwner::StaticInit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Placement-construct the global options overlay owner.
 */
HudUiOptionsPanelOverlayOwner *HudUiOptionsPanelOverlayOwner::StaticInit() {
    return new (&g_HudUiOptionsPanelOverlayOwner) HudUiOptionsPanelOverlayOwner;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspaneloverlayowner-registeratexit
 * @recoil-artifact defines .text recoil:function:0x40d090: HudUiOptionsPanelOverlayOwner::RegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Register the global options overlay owner destructor for process exit.
 */
void HudUiOptionsPanelOverlayOwner::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspaneloverlayowner-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x40d0a0: HudUiOptionsPanelOverlayOwner::AtExitDestructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Run process-exit cleanup for the global options overlay owner.
 */
void HudUiOptionsPanelOverlayOwner::AtExitDestructor() {
    g_HudUiOptionsPanelOverlayOwner.~HudUiOptionsPanelOverlayOwner();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspaneloverlayowner-huduioptionspaneloverlayowner
 * @recoil-artifact defines .text recoil:function:0x40d0b0: HudUiOptionsPanelOverlayOwner::HudUiOptionsPanelOverlayOwner.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Initialize the options overlay owner with no active panel.
 */
HudUiOptionsPanelOverlayOwner::HudUiOptionsPanelOverlayOwner() {
    m_dialog = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspaneloverlayowner-destructor-huduioptionspaneloverlayowner
 * @recoil-artifact defines .text recoil:function:0x40d0e0: HudUiOptionsPanelOverlayOwner::~HudUiOptionsPanelOverlayOwner.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspaneloverlayowner-ontrybecomecurrent
 * @recoil-artifact defines .text recoil:function:0x40d150: HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Create and enable the options dialog panel when the overlay owner becomes current.
 */
int HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent() {
    HudOptionsDialog *const panel = new HudOptionsDialog;
    m_dialog = panel;
    panel->SetEnabled(1);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduioptionspaneloverlayowner-queueenter
 * @recoil-artifact defines .text recoil:function:0x40d1c0: HudUiOptionsPanelOverlayOwner::QueueEnter.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Queue the global options-panel overlay owner as the next app state.
 */
void HudUiOptionsPanelOverlayOwner::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_HudUiOptionsPanelOverlayOwner,
        0
    );
}

namespace HudUiListMenuEntry {

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.comparesortkey
 * @recoil-artifact defines .text recoil:function:0x40d220: HudUiListMenuEntry::CompareSortKey.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiListMenu.cpp.
 * Purpose: compare two scoreboard entries by descending lap/score key and then descending player key.
 */
int __fastcall CompareSortKey(
    const HudUiScoreboardEntry *entryA,
    const HudUiScoreboardEntry *entryB
) {
    const unsigned int keyA = (unsigned int)(entryA->score + entryA->lapCount * 1000);
    const unsigned int keyB = (unsigned int)(entryB->score + entryB->lapCount * 1000);
    if (keyA != keyB) {
        return keyB < keyA ? 1 : 0;
    }

    return (unsigned int)(entryB->playerKey) < (unsigned int)(entryA->playerKey) ? 1 : 0;
}

} // namespace HudUiListMenuEntry

LPCSTR __stdcall AfxRegisterWndClass(
    UINT classStyle,
    HCURSOR cursor,
    HBRUSH background,
    HICON icon
);

union HudUiTripletWndClassNameStorage {
    unsigned long align;
    unsigned char bytes[sizeof(CString)];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiTripletWndClassNameStorage) == 0x04);

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduitripletwndclassname
 * @recoil-artifact defines .data recoil:data:0x4ed714: g_HudUiTripletWndClassName.
 * Source model: zero-initialized provider CString storage; the explicit HUD
 * triplet CRT row constructs/destructs the object.
 * Purpose: store the registered window class name used by HUD triplet panels.
 */
HudUiTripletWndClassNameStorage g_HudUiTripletWndClassName = {0};

#define g_HudUiTripletWndClassName \
    (*(CString *)&g_HudUiTripletWndClassName)

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-hudfontname-arial
 * @recoil-artifact defines .data recoil:data:0x4dacc0: g_HudFontName_Arial.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the shared writable Arial face-name buffer used by HUD
 * panel font setup.
 */
char g_HudFontName_Arial[] = "Arial";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicountertext-killslabel
 * @recoil-artifact defines .data recoil:data:0x4dacc8: g_HudUiCounterText_KillsLabel.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: label the kills column in the HUD triplet scoreboard header.
 */
char g_HudUiCounterText_KillsLabel[] = "Kills";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicountertext-lapslabel
 * @recoil-artifact defines .data recoil:data:0x4dacd0: g_HudUiCounterText_LapsLabel.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: label the laps column in the HUD triplet scoreboard header.
 */
char g_HudUiCounterText_LapsLabel[] = "Laps";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicountertext-playerlabel
 * @recoil-artifact defines .data recoil:data:0x4dacd8: g_HudUiCounterText_PlayerLabel.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: label the player-name column in HUD triplet scoreboard headers and
 * register the Player ZAR section name.
 */
char g_HudUiCounterText_PlayerLabel[] = "Player";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduicountertext-playerindexfmt
 * @recoil-artifact defines .data recoil:data:0x4dace0: g_HudUiCounterText_PlayerIndexFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: format localized triplet header text with the active race or score target.
 */
char g_HudUiCounterText_PlayerIndexFmt[] = "%s(%d)";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduishieldmessagewidget-defaultpercenttext
 * @recoil-artifact defines .data recoil:data:0x4dace8: g_HudUiShieldMessageWidget_DefaultPercentText.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the default shield percent text for the HUD shield message widget.
 */
char g_HudUiShieldMessageWidget_DefaultPercentText[4] = "000";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduitimerpanel-zerotimestring
 * @recoil-artifact defines .data recoil:data:0x4dacec: g_HudUiTimerPanel_ZeroTimeString.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the timer panel fallback display string for zero or invalid time.
 */
char g_HudUiTimerPanel_ZeroTimeString[9] = "00:00:00";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduitimerpanel-timefmt
 * @recoil-artifact defines .data recoil:data:0x4dacf8: g_HudUiTimerPanel_TimeFmt.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the timer panel hours/minutes/seconds text format.
 */
char g_HudUiTimerPanel_TimeFmt[15] = "%02d:%02d:%02d";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduitimerpanelfloat-drawformat
 * @recoil-artifact defines .data recoil:data:0x4dad0c: g_HudUiTimerPanelFloat_DrawFormat.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: provide the floating timer panel decimal display format.
 */
char g_HudUiTimerPanelFloat_DrawFormat[6] = "%2.1f";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduitimerpanel-nodename
 * @recoil-artifact defines .data recoil:data:0x4dad14: g_HudUiTimerPanel_NodeName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the ZAR HUD timer section registered by HudUiMgr.
 */
char g_HudUiTimerPanel_NodeName[9] = "HUDTimer";
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.g-huduitimerpanel-timerdatasectionname
 * @recoil-artifact defines .data recoil:data:0x4dad20: g_HudUiTimerPanel_TimerDataSectionName.
 * Data owner gate remains pending; this docblock records source provenance only.
 * Purpose: name the serialized timer data blob written by the timer panel callback.
 */
char g_HudUiTimerPanel_TimerDataSectionName[10] = "TimerData";

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-staticinitwndclassnameandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x40d1e0: HudUiTriplet::StaticInitWndClassNameAndRegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: construct the HUD triplet window-class CString and register its
 * static destructor during CRT startup.
 */
void __cdecl HudUiTriplet::StaticInitWndClassNameAndRegisterAtExit() {
    ConstructWndClassName();
    RegisterWndClassNameDtorAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-constructwndclassname
 * @recoil-artifact defines .text recoil:function:0x40d1f0: HudUiTriplet::ConstructWndClassName.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: default-construct the HUD triplet window-class CString in its
 * global storage.
 */
CString *HudUiTriplet::ConstructWndClassName() {
    return new (&g_HudUiTripletWndClassName) CString;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-registerwndclassnamedtoratexit
 * @recoil-artifact defines .text recoil:function:0x40d200: HudUiTriplet::RegisterWndClassNameDtorAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: register the HUD triplet window-class CString destructor with the
 * CRT at-exit list.
 */
void HudUiTriplet::RegisterWndClassNameDtorAtExit() {
    atexit(DestroyWndClassName);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-destroywndclassname
 * @recoil-artifact defines .text recoil:function:0x40d210: HudUiTriplet::DestroyWndClassName.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: destroy the HUD triplet window-class CString during CRT shutdown.
 */
void __cdecl HudUiTriplet::DestroyWndClassName() {
    g_HudUiTripletWndClassName.~CString();
}

namespace HudUiListMenuEntry {

void __fastcall SortRange(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end,
    int unusedFlags
);
void InsertPivotIntoSortedPrefix(
    HudUiScoreboardEntry *slot,
    HudUiScoreboardEntry pivot
);
void __fastcall InsertionSortRange(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end,
    int
);

} // namespace HudUiListMenuEntry

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutbase-destructor
 * @recoil-artifact defines .text recoil:function:0x40d3b0: HudLayoutBase::Destructor.
 * Purpose: run the address-backed layout-base cleanup operation independently
 * of compiler-owned typed-global destruction.
 */
void HudLayoutBase::Destructor() {
    widget0.DestructorCore();
    HudUiContainer::DestructorCore();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *HudUiTripletWndClassNameCrtInitializerFn)();
typedef void (__cdecl *HudUiMgrCrtInitializerFn)();
#pragma data_seg(".CRT$XCU")
/* VC5 emits this HUD triplet CString startup callback as a direct .CRT$XCU row. */
HudUiTripletWndClassNameCrtInitializerFn s_HudUiTripletWndClassNameCrtInit =
    HudUiTriplet::StaticInitWndClassNameAndRegisterAtExit;
/* VC5 emits this HUD manager startup callback as a direct .CRT$XCU row. */
HudUiMgrCrtInitializerFn s_HudUiCrtInit_HudUiMgr =
    HudUiMgr::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

namespace {
/**
 * Original helper evidence: no standalone retail function; observed in
 * HudUiTriplet::HudUiTriplet at 0x40dcd0 as the repeated panel allocation,
 * default construction, font setup, and hidden-state initialization sequence.
 * Purpose: create one hidden simple triplet panel with the requested font shape.
 */
inline HudUiPanel *NewSimplePanel(
    int setTextColors
) {
    HudUiPanel *const panel = new HudUiPanel;
    if (setTextColors != 0) {
        panel->SetTextColorsAndMarkDirty(
            0x0020bf40,
            0x0020bf40
        );
    }
    panel->HudUiPanel::SetFont(
        g_HudFontName_Arial,
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    panel->SetShadow(
        1,
        -1,
        -1
    );
    return panel;
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for AllocateHudObject.
 */
template <typename T> T *AllocateHudObject() {
    return (T *)(::operator new(sizeof(T)));
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x414670 HudUiTripletEntries::GetCount callers.
 * Purpose: preserve the recovered HUD behavior for NewObjectivePanel.
 */
inline HudUiPanel *NewObjectivePanel() {
    return new HudUiPanelSimple;
}
} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x40d400: HudUiMgr::StaticInitAndRegisterAtExit.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Physical hud.cpp prelude order cluster for [0x40d400, 0x410160).
 * Keep these definitions in retail BN order; helper declarations above stay source-shape inputs.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::StaticInitAndRegisterAtExit.
 */
void __cdecl HudUiMgr::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-staticinit
 * @recoil-artifact defines .text recoil:function:0x40d410: HudUiMgr::StaticInit.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::StaticInit.
 */
HudUiContainer *HudUiMgr::StaticInit() {
    HudUiMgrData *const manager = new (&g_HudUiMgr) HudUiMgrData;
    return manager;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-registeratexit
 * @recoil-artifact defines .text recoil:function:0x40d420: HudUiMgr::RegisterAtExit.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::RegisterAtExit.
 */
void HudUiMgr::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x40d430: HudUiMgr::AtExitDestructor.
 * Purpose: run the recovered HudUiMgr::AtExitDestructor teardown path.
 */
void __cdecl HudUiMgr::AtExitDestructor() {
    StaticDestructor(&g_HudUiMgr);
}

/**
 * Original function; retail address 0x40d440.
 * The original object is contiguous; the current source model keeps the same embedded HUD
 * manager subobjects as recovered globals. Keep the destruction order aligned with the retail
 * static destructor.
 * Purpose: run the recovered HudUiMgr::StaticDestructor teardown path.
 */
void __fastcall HudUiMgr::StaticDestructor(
    HudUiContainer *self
) {
    ((HudUiMgrData *)self)->~HudUiMgrData();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-destructor-huduimessage
 * @recoil-artifact defines .text recoil:function:0x40d590: HudUiMessage::Destructor.
 * Purpose: Tears down the side widget, embedded text panel, and base widget in retail destruction order.
 */
HudUiMessage::~HudUiMessage() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletpanel-unwinddestructfirstitem
 * @recoil-artifact defines .text recoil:function:0x40d600: HudUiTripletPanel::UnwindDestructFirstItem.
 * Purpose: Destroys the first item widget during constructor unwind cleanup.
 */
void HudUiTripletPanel::UnwindDestructFirstItem() {
    items[0].DestructorCore();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletpanel-destructorcore
 * @recoil-artifact defines .text recoil:function:0x40d610: HudUiTripletPanel::DestructorCore.
 * Purpose: Tears down the three triplet item widgets in reverse construction order.
 */
void HudUiTripletPanel::DestructorCore() {
    {
        for (int index = 2; index >= 0; --index) {
            items[index].DestructorCore();
        }
    }

}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgrobjectiveblock-destructor-huduimgrobjectiveblock
 * @recoil-artifact defines .text recoil:function:0x40d660: HudUiMgrObjectiveBlock::~HudUiMgrObjectiveBlock.
 * Binary Ninja shows the compiler-generated member destruction order:
 * chatComposeTextInput, objectiveBar/objectiveMeter base cleanup, then the
 * objective sensor and widget subobjects.
 * Purpose: tear down the embedded objective HUD widgets as the authored C++
 * destructor owner.
 */
HudUiMgrObjectiveBlock::~HudUiMgrObjectiveBlock() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgrsensorblock-destructor-huduimgrsensorblock
 * @recoil-artifact defines .text recoil:function:0x40d6e0: HudUiMgrSensorBlock::Destructor.
 * Binary Ninja models the retail HUD manager as one contiguous object and reaches
 * sensorPanel, sensorOverlay, and sensorMeter by offsets from this block. The
 * current source keeps those recovered subobjects as named fields in the HUD
 * manager cluster, so this method applies the same teardown to those subobjects.
 * Purpose: let VC5 tear down the embedded sensor meter, overlay, and panel
 * members in source member order.
 */
HudUiMgrSensorBlock::~HudUiMgrSensorBlock() {
}

/**
 * Original-source helper for lifecycle teardown; no standalone retail function exists.
 * Observed caller: 0x40d440.
 * Evidence: the address-backed static destructor invokes this ordinary C++
 * destructor, whose empty body leaves reverse member-array teardown to VC5.
 * The body is intentionally empty so VC5 owns the reverse member-array
 * destruction sequence and can emit the EH vector-destructor cleanup helpers.
 * Purpose: let the compiler tear down HudUiMgr member arrays in source-shaped
 * object order.
 */
HudUiMgrData::~HudUiMgrData() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduislot-destructor-huduislot
 * @recoil-artifact defines .text recoil:function:0x40d780: HudUiSlot::~HudUiSlot.
 * Purpose: let VC5 tear down the marker and slot widget members in source
 * member order.
 */
HudUiSlot::~HudUiSlot() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgrdata-huduimgrdata
 * @recoil-artifact defines .text recoil:function:0x40d7e0: HudUiMgrData::HudUiMgrData.
 * Retail BN shows one complete manager constructor containing both meter-base
 * calls, the member arrays, the message array, and the tail bar construction.
 * Purpose: construct the complete contiguous HUD manager object through one
 * ordinary most-derived C++ constructor.
 */
HudUiMgrData::HudUiMgrData() : reticleWidget(0) {
    tailBar.quadHeight = 0;
    tailBar.quadLeftX = 0.0f;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicontainer-setenabled
 * @recoil-artifact defines .text recoil:function:0x40d9d0: HudUiContainer::SetEnabled.
 * Purpose: Apply the recovered HUD container enabled-state change.
 */
inline void HudUiContainer::SetEnabled(
    int enabledValue
) {
    enabled = enabledValue;
}

/**
 * Original function; retail address 0x40d9e0.
 * Retail manager construction calls this base twice before installing the
 * same manager-leaf vtable for the objective and sensor members.
 * Purpose: construct the manager-meter base and clear its fill state.
 */
HudUiManagerMeterBaseCandidate::HudUiManagerMeterBaseCandidate() : HudUiBar() {
    fillPixelsMax = 0;
    meterFlags = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-huduimessage
 * @recoil-artifact defines .text recoil:function:0x40da00: HudUiMessage::HudUiMessage.
 * Purpose: Constructs the weapon-message widget, embedded text panel, side widget, and clears image slots.
 */
HudUiMessage::HudUiMessage() : HudUiWidget(0), panel(), widget(0) {
    memset(
        variantImages,
        0,
        sizeof(variantImages) + sizeof(activeSideImages) + sizeof(sideImageSwaps)
    );
    panel.activeSideIndex = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicounter-huduicounter
 * @recoil-artifact defines .text recoil:function:0x40dac0: HudUiCounter::HudUiCounter.
 * Purpose: Constructs the widget base and clears the three HUD counter state-image slots.
 */
HudUiCounter::HudUiCounter() : HudUiWidget(0) {
    stateImages[2] = 0;
    stateImages[1] = 0;
    stateImages[0] = 0;
}

/**
 * Original function; retail address 0x40db20.
 * Purpose: Constructs the HUD element base and embedded slot widgets for a
 * weapon/sensor HUD slot; the shared retail identity with the ordinary C++
 * constructor remains unresolved.
 */
HudUiSlot * HudUiSlot::Constructor() {
    new (this) HudUiSlot;
    return this;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduislot-draw
 * @recoil-artifact defines .text recoil:function:0x40db90: HudUiSlot::Draw.
 * Purpose: Draws the visible slot and track-marker widgets in recovered HUD slot order.
 */
void HudUiSlot::Draw() {
    if (((~slotWidget.flags) & 0x10) != 0) {
        slotWidget.Draw();
    }

    if (((~trackMarkerWidget.flags) & 0x10) != 0) {
        trackMarkerWidget.Draw();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicountertextpanel-constructor
 * @recoil-artifact defines .text recoil:function:0x40dbf0: HudUiCounterTextPanel::HudUiCounterTextPanel.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Data owners: hud_ui.hud_font_name_arial_data and hud_ui.hud_ui_mgr_data.
 * Purpose: initialize the objective counter text panel defaults and register it with the HUD manager.
 */
HudUiCounterTextPanel::HudUiCounterTextPanel() : HudUiPanel() {
    textColor0 = 0x0020bf40;
    textColor1 = 0x0020bf40;
    textDirty = 1;
    HudUiPanel::SetFont(
        g_HudFontName_Arial,
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    shadowEnabled = 1;
    shadowOffsetX = -1;
    shadowOffsetY = -1;

    HudUiPanel *const panel = this;
    panel->SetTextFmt(
        "%d",
        0
    );
    panel->UpdateTextBoundsFromContent();
    panel->SetVisible(1);
    g_HudUiMgr.AddChild(this);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-huduitriplet
 * @recoil-artifact defines .text recoil:function:0x40dcd0: HudUiTriplet::HudUiTriplet.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: initialize the scoreboard triplet container, entry vector, header panels, and row-cell panels.
 */
HudUiTriplet::HudUiTriplet() : HudUiContainer() {
    entries.rowInitFlag = 0;
    entries.begin = 0;
    entries.end = 0;
    entries.cap = 0;
    lapsColumnOffsetX = 0x23;
    killsColumnOffsetX = 0x46;
    fontSize = 8;
    fontWeight = 6;

    {
        int headerIndex;
        for (headerIndex = 0; headerIndex < 3; ++headerIndex) {
            headerPanels[headerIndex] = NewSimplePanel(1);
        }

        for (headerIndex = 0; headerIndex < 3; ++headerIndex) {
            headerPanels[headerIndex]->SetFont(
                g_HudFontName_Arial,
                fontSize,
                0x1f4,
                fontWeight,
                0,
                0,
                2
            );
            headerPanels[headerIndex]->SetVisible(0);
            HudUiContainer::AddChild(
                (HudUiElement *)(headerPanels[headerIndex])
            );
        }
    }

    headerPanels[0]->alignMode = 2;
    headerPanels[1]->alignMode = 1;
    headerPanels[2]->alignMode = 1;
    headerPanels[0]->SetTextFmt(g_HudUiCounterText_PlayerLabel);
    headerPanels[1]->SetTextFmt(g_HudUiCounterText_LapsLabel);
    headerPanels[2]->SetTextFmt(g_HudUiCounterText_KillsLabel);

    {
        int row;
        for (row = 0; row < 8; ++row) {
            int column;
            for (column = 0; column < 3; ++column) {
                rowCells[row * 3 + column] = NewSimplePanel(0);
                rowCells[row * 3 + column]->textColor0 = 0x0020bf40;
                rowCells[row * 3 + column]->textColor1 = 0x0020bf40;
                rowCells[row * 3 + column]->textDirty = 1;
                rowCells[row * 3 + column]->SetFont(
                    g_HudFontName_Arial,
                    fontSize,
                    0x1f4,
                    fontWeight,
                    0,
                    0,
                    2
                );
                rowCells[row * 3 + column]->SetVisible(0);
                HudUiContainer::AddChild(
                    (HudUiElement *)(rowCells[row * 3 + column])
                );
            }

            rowCells[row * 3]->alignMode = 2;
            rowCells[row * 3 + 1]->alignMode = 1;
            rowCells[row * 3 + 2]->alignMode = 1;
        }
    }

    HudUiContainer *const container = this;
    container->SetEnabled(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanel-settextcolorsandmarkdirty
 * @recoil-artifact defines .text recoil:function:0x40e010: HudUiPanel::SetTextColorsAndMarkDirty.
 * Purpose: Stores the panel text color pair and marks cached text metrics dirty.
 */
void HudUiPanel::SetTextColorsAndMarkDirty(
    unsigned int color0,
    unsigned int color1
) {
    textColor0 = color0;
    textColor1 = color1;
    textDirty = 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanel-setshadow
 * @recoil-artifact defines .text recoil:function:0x40e040: HudUiPanel::SetShadow.
 * Purpose: Stores panel text-shadow state and returns the previous shadow flag.
 */
unsigned int HudUiPanel::SetShadow(
    unsigned int enableShadow,
    int offsetX,
    int offsetY
) {
    const unsigned int previous = shadowEnabled;
    shadowEnabled = enableShadow;
    shadowOffsetX = offsetX;
    shadowOffsetY = offsetY;
    return previous;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-destructor-huduitriplet
 * @recoil-artifact defines .text recoil:function:0x40e070: HudUiTriplet::~HudUiTriplet.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: release owned scoreboard header panels, row cells, and entry storage before container teardown.
 */
HudUiTriplet::~HudUiTriplet() {
    {
        int headerIndex;
        for (headerIndex = 0; headerIndex < 3; ++headerIndex) {
            HudUiPanel *header = headerPanels[headerIndex];
            if (header != 0) {
                delete header;
                headerPanels[headerIndex] = 0;
            }
        }
    }

    {
        int rowCellIndex;
        for (rowCellIndex = 0; rowCellIndex < 24; ++rowCellIndex) {
            HudUiPanel *rowCell = rowCells[rowCellIndex];
            if (rowCell != 0) {
                delete rowCell;
                rowCells[rowCellIndex] = 0;
            }
        }
    }

    ::operator delete(entries.begin);
    entries.begin = 0;
    entries.end = 0;
    entries.cap = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-rebuilddisplay
 * @recoil-artifact defines .text recoil:function:0x40e140: HudUiTriplet::RebuildDisplay.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: sort scoreboard entries and refresh the visible triplet rows and headers for score or lap mode.
 */
void HudUiTriplet::RebuildDisplay() {
    HudUiScoreboardEntry *const begin = entries.begin;
    HudUiScoreboardEntry *const end = entries.end;
    if (begin != 0 && begin != end) {
        if (end - begin <= 16) {
            HudUiListMenuEntry::InsertionSortRange(
                begin,
                end,
                0
            );
        } else {
            HudUiListMenuEntry::SortRange(
                begin,
                end,
                0
            );

            HudUiScoreboardEntry *insert = begin + 16;
            HudUiListMenuEntry::InsertionSortRange(
                begin,
                insert,
                0
            );

            while (insert != end) {
                HudUiScoreboardEntry pivot = *insert;
                HudUiListMenuEntry::InsertPivotIntoSortedPrefix(
                    insert,
                    pivot
                );
                ++insert;
            }
        }
    }

    HudUiScoreboardEntry *entry = entries.begin;
    const size_t entryCount =
        entries.begin != 0
            ? (size_t)(entries.end - entries.begin)
            : 0;
    size_t rowIndex = 0;
    while (entry != entries.end && rowIndex < 8) {
        {
            size_t column;
            for (column = 0; column < 3; ++column) {
                HudUiPanel *const cell = rowCells[rowIndex * 3 + column];
                cell->textColor0 = entry->playerColorPackedRgb;
                cell->textColor1 = entry->playerColorPackedRgb;
                cell->textDirty = 1;
                HudUiElement *const element = (HudUiElement *)(cell);
                if (column == 2) {
                    if (entry->lapCount >= 0) {
                        ((HudUiElement *)(rowCells[rowIndex * 3 + 2]))->SetVisible(1);
                    }
                } else {
                    element->SetVisible(1);
                }
                element->flags = (element->flags & 0x10u) | 0x0cu;
                cell->SetFont(
                    g_HudFontName_Arial,
                    fontSize,
                    0x1f4,
                    fontWeight,
                    0,
                    0,
                    2
                );
            }
        }

        const int y = baseY + (int)(rowIndex + 1) * rowPitchY;
        ((HudUiElement *)(rowCells[rowIndex * 3]))->SetPos(
            baseX,
            y
        );
        ((HudUiElement *)(rowCells[rowIndex * 3 + 1]))->SetPos(
            baseX + lapsColumnOffsetX,
            y
        );
        ((HudUiElement *)(rowCells[rowIndex * 3 + 2]))->SetPos(
            baseX + killsColumnOffsetX,
            y
        );

        if (g_HudSensorTracker.raceCheckpointMode != 0) {
            rowCells[rowIndex * 3]->SetTextFmt(
                "%s",
                entry->displayName
            );
            rowCells[rowIndex * 3 + 1]->SetTextFmt(
                "%d",
                entry->lapCount
            );
            rowCells[rowIndex * 3 + 2]->SetTextFmt(
                "%d",
                entry->score
            );
        } else {
            rowCells[rowIndex * 3]->SetTextFmt(
                "%s",
                entry->displayName
            );
            rowCells[rowIndex * 3 + 1]->SetTextFmt(
                "%d",
                entry->score
            );
            ((HudUiElement *)(rowCells[rowIndex * 3 + 2]))->SetVisible(0);
        }

        ++entry;
        ++rowIndex;
    }

    for (; rowIndex < 8; ++rowIndex) {
        {
            size_t column;
            for (column = 0; column < 3; ++column) {
                HudUiPanel *const cell = rowCells[rowIndex * 3 + column];
                HudUiElement *const element = (HudUiElement *)(cell);
                element->flags &= 0x10u;
                element->SetVisible(0);
            }
        }
    }

    if (entryCount == 0) {
        return;
    }

    ((HudUiElement *)(headerPanels[0]))->SetPos(
        baseX,
        baseY
    );
    ((HudUiElement *)(headerPanels[1]))->SetPos(
        baseX + lapsColumnOffsetX,
        baseY
    );
    ((HudUiElement *)(headerPanels[2]))->SetPos(
        baseX + killsColumnOffsetX,
        baseY
    );

    headerPanels[0]->SetFont(
        g_HudFontName_Arial,
        fontSize,
        0x1f4,
        fontWeight,
        0,
        0,
        2
    );
    headerPanels[1]->SetFont(
        g_HudFontName_Arial,
        fontSize,
        0x1f4,
        fontWeight,
        0,
        0,
        2
    );
    headerPanels[2]->SetFont(
        g_HudFontName_Arial,
        fontSize,
        0x1f4,
        fontWeight,
        0,
        0,
        2
    );

    ((HudUiElement *)(headerPanels[0]))->SetVisible(1);
    if (g_HudSensorTracker.raceCheckpointMode != 0) {
        headerPanels[1]->SetTextFmt(
            g_HudUiCounterText_PlayerIndexFmt,
            zLoc::GetMessageString(0x113),
            g_HudSensorTracker.runtimeGoalValue
        );
        headerPanels[2]->SetTextFmt(
            g_HudUiCounterText_PlayerIndexFmt,
            zLoc::GetMessageString(0x114),
            g_HudSensorTracker.runtimeGoalValue
        );
        ((HudUiElement *)(headerPanels[1]))->SetVisible(1);
        ((HudUiElement *)(headerPanels[2]))->SetVisible(1);
    } else {
        headerPanels[1]->SetTextFmt(
            g_HudUiCounterText_PlayerIndexFmt,
            zLoc::GetMessageString(0x114),
            g_HudSensorTracker.runtimeGoalValue
        );
        ((HudUiElement *)(headerPanels[1]))->SetVisible(1);
        ((HudUiElement *)(headerPanels[2]))->SetVisible(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-addentry
 * @recoil-artifact defines .text recoil:function:0x40e590: HudUiTriplet::AddEntry.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: append a new network player row to the scoreboard entry vector and rebuild the display.
 */
void HudUiTriplet::AddEntry(
    GameNetPlayerRow *entryData
) {
    HudUiScoreboardEntry sourceValue = {0};
    strncpy(
        sourceValue.displayName,
        entryData->displayName,
        0x3f
    );
    sourceValue.playerKey = entryData->playerKey;
    sourceValue.playerColorPackedRgb = entryData->playerColorPackedRgb;
    sourceValue.score = 0;
    sourceValue.lapCount = 0;

    entries.insert(
        entries.begin,
        1,
        sourceValue
    );
    RebuildDisplay();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-updateentrydata
 * @recoil-artifact defines .text recoil:function:0x40e800: HudUiTriplet::UpdateEntryData.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: update the matching scoreboard entry from a network player row and rebuild the display.
 */
void HudUiTriplet::UpdateEntryData(
    GameNetPlayerRow *entryData
) {
    HudUiScoreboardEntry *entry = entries.begin;
    for (int i = 0; entry != entries.end && i < 8; ++i) {
        if (entry->playerKey == entryData->playerKey) {
            entry->playerColorPackedRgb = entryData->playerColorPackedRgb;
            entry->score = entryData->score;
            entry->lapCount = g_HudSensorTracker.raceCheckpointMode != 0 ? entryData->lapCount : -1;
            break;
        }

        ++entry;
    }

    RebuildDisplay();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-removeentry
 * @recoil-artifact defines .text recoil:function:0x40e880: HudUiTriplet::RemoveEntry.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: remove the matching player row from the scoreboard entry vector and rebuild the display.
 */
void HudUiTriplet::RemoveEntry(
    GameNetPlayerRow *entryKey
) {
    HudUiScoreboardEntry *entry = entries.begin;
    for (int i = 0; entry != entries.end && i < 8; ++i) {
        if (entry->playerKey == entryKey->playerKey) {
            entries.erase(entry);
            break;
        }

        ++entry;
    }

    RebuildDisplay();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-interpolatelayout
 * @recoil-artifact defines .text recoil:function:0x40e910: HudUiTriplet::InterpolateLayout.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: interpolate the active scoreboard triplet layout fields between the stored start and end layouts.
 */
void HudUiTriplet::InterpolateLayout(
    float t
) {
    baseX = (int)((float)(baseXEnd - baseXStart) * t + baseXStart);
    baseY = (int)((float)(baseYEnd - baseYStart) * t + baseYStart);
    rowPitchY = (int)((float)(rowPitchYEnd - rowPitchYStart) * t + rowPitchYStart);
    lapsColumnOffsetX =
        (int)((float)(lapsColumnOffsetXEnd - lapsColumnOffsetXStart) * t + lapsColumnOffsetXStart);
    killsColumnOffsetX = (int)((float)(killsColumnOffsetXEnd - killsColumnOffsetXStart) * t +
                               killsColumnOffsetXStart);
    fontSize = (int)((float)(fontSizeEnd - fontSizeStart) * t + fontSizeStart);
    fontWeight = (int)((float)(fontWeightEnd - fontWeightStart) * t + fontWeightStart);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitriplet-islocalplayerfirstentry
 * @recoil-artifact defines .text recoil:function:0x40ea60: HudUiTriplet::IsLocalPlayerFirstEntry.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiTriplet.cpp.
 * Purpose: report whether the first scoreboard entry belongs to the local network player.
 */
int HudUiTriplet::IsLocalPlayerFirstEntry() {
    HudUiScoreboardEntry *const begin = entries.begin;
    int count = 0;
    if (begin != 0) {
        count = (int)(entries.end - begin);
    }

    if (count == 0) {
        return -1;
    }

    return zNetwork_GetLocalPlayerKey() == entries.begin->playerKey ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudscoreboard-setscaleandrebuild
 * @recoil-artifact defines .text recoil:function:0x40eab0: HudScoreboard::SetScaleAndRebuild.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudScoreboard.cpp.
 * Purpose: apply a scale to the global stats-list triplet layout and immediately rebuild its rows.
 */
void __stdcall HudScoreboard::SetScaleAndRebuild(
    float scale
) {
    g_HudUiMgrStatsList->triplet->InterpolateLayout(scale);
    g_HudUiMgrStatsList->triplet->RebuildDisplay();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudscoreboard-dispatchsetscale
 * @recoil-artifact defines .text recoil:function:0x40eae0: HudScoreboard::DispatchSetScale.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudScoreboard.cpp.
 * Purpose: dispatch delta time through the global stats-list update slot during scoreboard scaling.
 */
void __stdcall HudScoreboard::DispatchSetScale(
    float deltaTime
) {
    HudUiStatsListElement *const statsList = g_HudUiMgrStatsList;
    statsList->Update(deltaTime);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduishieldmessagewidget-applylayout
 * @recoil-artifact defines .text recoil:function:0x40eb00: HudUiShieldMessageWidget::ApplyLayout.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Binary Ninja evidence: stdcall layout callback ignores ECX and uses the
 * global shield-message widget, applies ZRD root children 1, 2, and 3 to the
 * widget, percent text panel, and meter, then attaches those children in that
 * order.
 * Purpose: Apply the shield-message HUD layout and reset the displayed
 * percent text.
 */
int __stdcall HudUiShieldMessageWidget::ApplyLayout(
    zReader::Node *layoutRoot
) {
    HudUiShieldMessageWidget *const shieldMessageWidget = g_HudUiMgrShieldMessageWidget;
    zReader::Node *const layoutPayload = layoutRoot->value.nodes;

    HudUiLayoutNode::ApplyImageWidget(
        &layoutPayload[1],
        &shieldMessageWidget->widget,
        g_HudUiMgrHudOriginX,
        g_HudUiMgrHudOriginY,
        0,
        0,
        &shieldMessageWidget->screenRect
    );

    int offsetXY[2];
    offsetXY[0] = shieldMessageWidget->widget.GetCenterX();
    offsetXY[1] = shieldMessageWidget->widget.GetCenterY();

    HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
    HudUiLayoutNode::ApplyTextLabel(
        &layoutPayload[2],
        percentTextPanel,
        0,
        0,
        offsetXY
    );

    HudUiRect clipRect;
    clipRect.left = percentTextPanel->GetCenterX() - offsetXY[0];
    clipRect.top = percentTextPanel->GetCenterY() - offsetXY[1];
    percentTextPanel->SetBltSourceAndClipRect(
        shieldMessageWidget->widget.image,
        &clipRect
    );

    percentTextPanel->SetTextFmt(g_HudUiShieldMessageWidget_DefaultPercentText);
    percentTextPanel->UpdateTextBoundsFromContent();

    HudUiLayoutNode::ApplyMeterQuad(
        &layoutPayload[3],
        &shieldMessageWidget->meter,
        0,
        0,
        offsetXY,
        &clipRect
    );

    shieldMessageWidget->meter.SetBltSourceAndClipRect(
        shieldMessageWidget->widget.image,
        &clipRect
    );

    g_HudUiMgr.AddChild((HudUiElement *)(&shieldMessageWidget->widget));
    g_HudUiMgr.AddChild((HudUiElement *)(percentTextPanel));
    g_HudUiMgr.AddChild((HudUiElement *)(&shieldMessageWidget->meter));
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutbase-shutdown-stub
 * @recoil-artifact defines .text recoil:function:0x40ec90: HudLayoutBase::Shutdown_Stub.
 * Purpose: route the HUD layout shutdown slot through the recovered no-op widget method.
 */
void HudLayoutBase::Shutdown_Stub() {
    g_HudUiMgrShieldMessageWidget->widget.Shutdown();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-setrunning
 * @recoil-artifact defines .text recoil:function:0x40eca0: HudUiTimerPanel::SetRunning.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: set the global HUD timer panel stopped flag from the running state.
 */
void __fastcall HudUiTimerPanel::SetRunning(
    int running
) {
    g_HudUiMgrTimerPanel->stopped = running == 0 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-setelapsedseconds
 * @recoil-artifact defines .text recoil:function:0x40ecc0: HudUiTimerPanel::SetElapsedSeconds.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: store the elapsed seconds on the global HUD timer panel.
 */
void __stdcall HudUiTimerPanel::SetElapsedSeconds(
    float seconds
) {
    g_HudUiMgrTimerPanel->elapsedSeconds = seconds;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-setseconds
 * @recoil-artifact defines .text recoil:function:0x40ece0: HudUiTimerPanel::SetSeconds.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: update the global HUD timer panel's elapsed display and second step.
 */
void __stdcall HudUiTimerPanel::SetSeconds(
    float elapsedSeconds,
    float secondsStep
) {
    g_HudUiMgrTimerPanel->secondsStep = (int)(secondsStep);
    g_HudUiMgrTimerPanel->UpdateHMSFromSeconds(elapsedSeconds);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-getseconds
 * @recoil-artifact defines .text recoil:function:0x40ed10: HudUiTimerPanel::GetSeconds.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: return the elapsed seconds from the global HUD timer panel.
 */
float HudUiTimerPanel::GetSeconds() {
    return g_HudUiMgrTimerPanel->elapsedSeconds;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-update
 * @recoil-artifact defines .text recoil:function:0x40ed20: HudUiTimerPanel::Update.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: advance the running timer and update base HudUiElement state.
 */
void HudUiTimerPanel::Update(
    float deltaSeconds
) {
    if (stopped == 0) {
        const float frameDelta =
            zOpt::GetNetworkEnabled() != 0 ? g_Time_UnscaledDeltaTimeSec : g_FrameDeltaTimeSec;
        const float newElapsedSeconds =
            elapsedSeconds + (float)(secondsStep) * frameDelta;
        elapsedSeconds = newElapsedSeconds;
        UpdateHMSFromSeconds(newElapsedSeconds);
    }

    HudUiElement::Update(deltaSeconds);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-huduitimerpanel
 * @recoil-artifact defines .text recoil:function:0x40ed80: HudUiTimerPanel::HudUiTimerPanel.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: initialize the timer panel font, colors, text, and default stopped state.
 */
HudUiTimerPanel::HudUiTimerPanel() : HudUiPanel(0, 0, 0) {
    textColor0 = 0x0020bf40;
    textColor1 = 0x0020bf40;
    textDirty = 1;
    HudUiPanel::SetFont(
        g_HudFontName_Arial,
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    shadowEnabled = 1;
    shadowOffsetX = -1;
    shadowOffsetY = -1;

    secondsStep = 1;
    SetTimeSeconds(
        0,
        0,
        0
    );
    stopped = 1;
    elapsedSeconds = 0.0f;
    ((HudUiElement *)this)->SetVisible(1);
    g_HudUiMgr.AddChild(this);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-updatehmsfromseconds
 * @recoil-artifact defines .text recoil:function:0x40ee60: HudUiTimerPanel::UpdateHMSFromSeconds.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: cache elapsed seconds and update the visible timer text.
 */
void HudUiTimerPanel::UpdateHMSFromSeconds(
    float seconds
) {
    elapsedSeconds = seconds;

    const int hours = (int)(floor(seconds * 0.000277777785f));
    float remaining = seconds - (float)(hours) * 3600.0f;
    const int minutes = (int)(floor(remaining * 0.0166666675f));
    remaining = (float)(remaining);
    const int displaySeconds = (int)(floor(remaining - (float)(minutes) * 60.0f));
    SetTimeSeconds(
        hours,
        minutes,
        displaySeconds
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-settimeseconds
 * @recoil-artifact defines .text recoil:function:0x40ef00: HudUiTimerPanel::SetTimeSeconds.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: format the timer panel text from hour, minute, and second fields.
 */
void HudUiTimerPanel::SetTimeSeconds(
    int hours,
    int minutes,
    int seconds
) {
    if (hours >= 0 && minutes >= 0 && seconds >= 0) {
        SetTextFmt(
            g_HudUiTimerPanel_TimeFmt,
            hours,
            minutes,
            seconds
        );
    } else {
        SetTextFmt(g_HudUiTimerPanel_ZeroTimeString);
    }

    UpdateTextBoundsFromContent();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanelfloat-constructordefault
 * @recoil-artifact defines .text recoil:function:0x40ef60: HudUiTimerPanelFloat::HudUiTimerPanelFloat.
 * Purpose: initialize the floating timer panel class state and hide it until
 * gameplay enables the overlay.
 */
HudUiTimerPanelFloat::HudUiTimerPanelFloat()
    : HudUiPanel(" ", 3, 0x1c) {
    textColor0 = 0x0020bf40;
    textColor1 = 0x0020bf40;
    textDirty = 1;
    HudUiPanel::SetFont(
        g_HudFontName_Arial,
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    shadowEnabled = 1;
    shadowOffsetX = -1;
    shadowOffsetY = -1;

    clipRect.left = x;
    clipRect.top = y;
    clipRect.right = x + 0x3c;

    sampleFrameCount = 0.0f;
    displayValue = 0.0f;
    sampleElapsedSec = 0.0f;
    clipRect.bottom = y + 0x0f;
    ((HudUiElement *)this)->SetVisible(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanelfloat-draw
 * @recoil-artifact defines .text recoil:function:0x40f040: HudUiTimerPanelFloat::Draw.
 * Purpose: refresh the floating timer display text before drawing the base
 * panel.
 */
void HudUiTimerPanelFloat::Draw() {
    Invalidate();
    SetTextFmt(
        g_HudUiTimerPanelFloat_DrawFormat,
        (double)(displayValue)
    );
    HudUiPanel::Draw();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicounter-applyfromlayoutnode
 * @recoil-artifact defines .text recoil:function:0x40f070: HudUiCounter::ApplyFromLayoutNode.
 * Purpose: Loads counter image/layout data from a ZRD array node and registers the counter with the HUD manager.
 */
int HudUiCounter::ApplyFromLayoutNode(
    zReader::Node *layoutNode
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    stateImages[0] = zImage::TexDir_FindOrCreateByPath(payload[1].value.str);
    stateImages[1] = zImage::TexDir_FindOrCreateByPath(payload[2].value.str);
    stateImages[2] = zImage::TexDir_FindOrCreateByPath(payload[3].value.str);
    layoutX = payload[4].value.i32;
    layoutY = payload[5].value.i32;

    UpdateLayoutPosition();
    SetImageBorrowedAndInvalidate(stateImages[0]);
    g_HudUiMgr.AddChild(this);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicounter-releasestateimages
 * @recoil-artifact defines .text recoil:function:0x40f0f0: HudUiCounter::ReleaseStateImages.
 * Purpose: Releases and clears the counter's three variant images.
 */
void HudUiCounter::ReleaseStateImages() {
    zVid_Image::ReleaseIfNotDefault(stateImages[0]);
    zVid_Image::ReleaseIfNotDefault(stateImages[1]);
    zVid_Image::ReleaseIfNotDefault(stateImages[2]);

    stateImages[2] = 0;
    stateImages[1] = 0;
    stateImages[0] = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduicounter-updatelayoutposition
 * @recoil-artifact defines .text recoil:function:0x40f130: HudUiCounter::UpdateLayoutPosition.
 * Purpose: Places the counter relative to the HUD origin and rebuilds the local clip viewport rectangle.
 */
void HudUiCounter::UpdateLayoutPosition() {
    const int localX = layoutX;
    const int localY = layoutY;
    SetPos(
        g_HudUiMgrHudOriginX + localX,
        g_HudUiMgrHudOriginY + localY
    );

    zVidImagePartial *const image = stateImages[0];
    clipViewportRect.left = localX;
    clipViewportRect.top = localY;
    clipViewportRect.right = localX + image->width;
    clipViewportRect.bottom = localY + image->height;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-setmodecounterstate
 * @recoil-artifact defines .text recoil:function:0x40f1a0: HudUiMgr::SetModeCounterState.
 * Purpose: apply the recovered HUD state change handled by HudUiMgr::SetModeCounterState.
 */
void __fastcall HudUiMgr::SetModeCounterState(
    int counterIndex,
    int state
) {
    if (state == 2) {
        HudUiCounter &previous = g_HudUiMgrModeCounters[g_HudUiMgrActiveModeCounterIndex];
        previous.SetImageBorrowedAndInvalidate(previous.stateImages[1]);
        g_HudUiMgrActiveModeCounterIndex = counterIndex;
    }

    HudUiCounter &counter = g_HudUiMgrModeCounters[counterIndex];
    counter.SetImageBorrowedAndInvalidate(counter.stateImages[state]);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletpanel-constructor
 * @recoil-artifact defines .text recoil:function:0x40f200: HudUiTripletPanel::Constructor.
 * Purpose: Constructs the base panel, initializes the three item widgets hidden, and attaches the panel to the HUD manager.
 */
HudUiTripletPanel::HudUiTripletPanel() : HudUiElement(0, 0) {
    visibleCount = 0;

    ((HudUiElement *)(&items[0]))->SetVisible(0);
    ((HudUiElement *)(&items[1]))->SetVisible(0);
    ((HudUiElement *)(&items[2]))->SetVisible(0);

    g_HudUiMgr.AddChild(this);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduiwidget-huduiwidget
 * @recoil-artifact defines .text recoil:function:0x40f2d0: HudUiWidget::HudUiWidget.
 * Purpose: preserve the recovered HUD behavior for HudUiWidget::HudUiWidget.
 */
HudUiWidget::HudUiWidget() {
    Constructor(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduinanitepanel-initlayout
 * @recoil-artifact defines .text recoil:function:0x40f2e0: HudUiNanitePanel::InitLayout.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiNanitePanel::InitLayout.
 */
void HudUiNanitePanel::InitLayout(
    zReader::Node *layoutRoot
) {
    HudUiWidget *const layoutWidget2 = &g_HudLayoutHW.widget2;
    const int baseX = g_HudUiMgrHudOriginX / 2;
    int anchor[2];
    anchor[0] = layoutWidget2->GetCenterX();
    anchor[1] = layoutWidget2->GetCenterY();

    zReader::Node *const layoutPayload = layoutRoot->value.nodes;
    HudUiRect clipRect;
    HudUiLayoutNode::ReadRectOffsetAndSize(
        &layoutPayload[1],
        &clipRect,
        0,
        0,
        0
    );

    zVidImagePartial *const sharedImage =
        HudUiLayoutNode::ApplyImageWidget(
            &layoutPayload[2],
            &items[0],
            baseX,
            0,
            anchor,
            0,
            0
        );
    HudUiLayoutNode::ApplyImageWidget(
        &layoutPayload[3],
        &items[1],
        baseX,
        0,
        anchor,
        sharedImage,
        0
    );
    HudUiLayoutNode::ApplyImageWidget(
        &layoutPayload[4],
        &items[2],
        baseX,
        0,
        anchor,
        sharedImage,
        0
    );

    HudUiWidget *const anchorItem = &items[2];
    const int y = anchorItem->GetCenterY();
    const int x = anchorItem->GetCenterX();
    SetPos(
        x,
        y
    );

    clipRect.left += baseX;
    clipRect.right += baseX;
    SetBltSourceAndClipRect(
        0,
        &clipRect
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletpanel-shutdownitems-stub
 * @recoil-artifact defines .text recoil:function:0x40f3e0: HudUiTripletPanel::ShutdownItems_Stub.
 * Purpose: Preserves the retail no-op shutdown calls made for each nanite triplet item.
 */
void HudUiTripletPanel::ShutdownItems_Stub() {
    g_HudUiMgrNanitePanel.items[0].Shutdown();
    g_HudUiMgrNanitePanel.items[1].Shutdown();
    g_HudUiMgrNanitePanel.items[2].Shutdown();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletpanel-draw
 * @recoil-artifact defines .text recoil:function:0x40f400: HudUiTripletPanel::Draw.
 * Purpose: Draws the triplet panel base and visible item widgets from back to front.
 */
void HudUiTripletPanel::Draw() {
    DrawBase();

    if (((~items[2].flags) & 0x10u) != 0) {
        items[2].Draw();
    }

    if (((~items[1].flags) & 0x10u) != 0) {
        items[1].Draw();
    }

    if (((~items[0].flags) & 0x10u) != 0) {
        items[0].Draw();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletpanel-setvisiblecount
 * @recoil-artifact defines .text recoil:function:0x40f460: HudUiTripletPanel::SetVisibleCount.
 * Purpose: Applies the visible item count, updates child visibility, and invalidates the panel.
 */
void HudUiTripletPanel::SetVisibleCount(
    int count
) {
    if (visibleCount == count) {
        return;
    }

    if (count > 4) {
        count = 4;
    } else if (count < 0) {
        count = 0;
    }

    visibleCount = count;

    {
        int index = 0;
        HudUiWidget *item = items;
        while (index < 3) {
            if (count > index) {
                item->SetVisible(1);
            } else {
                item->SetVisible(0);
            }
            ++index;
            ++item;
        }
    }

    Invalidate();
}

/**
 * Original inline-constructor evidence: retail InitHudLayouts has one
 * allocation-null branch enclosing base and array construction, item
 * attachment, and enabling; no standalone constructor body exists.
 * Purpose: construct and initialize the complete HUD string menu.
 */
inline HudUiStringMenu::HudUiStringMenu() {
    int y = 0x5f;
    {
        int itemIndex;
        for (itemIndex = 0;
            itemIndex < (int)(sizeof(items) / sizeof(items[0]));
            ++itemIndex) {
            HudUiPanelSimple &item = items[itemIndex];

            HudUiElement *const child = (HudUiElement *)(&item);
            child->SetPos(
                5,
                y
            );
            AddChild(child);
            child->SetVisible(1);
            y += 0x0f;
        }
    }

    ((HudUiContainer *)this)->SetEnabled(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-inithudlayouts
 * @recoil-artifact defines .text recoil:function:0x40f4c0: HudUiMgr::InitHudLayouts / InitHudLayouts.
 * Purpose: initialize the software and hardware HUD layout singletons for the current display sections.
 */
int __fastcall HudUiMgr::InitHudLayouts(
    const HudUiRect *displaySection,
    const HudUiRect *windowSection
) {
    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        return 1;
    }

    g_HudUiMgrTimerPanelFloat = new HudUiTimerPanelFloat;

    g_HudUiMgrStringMenu = new HudUiStringMenu;

    HudUiShieldMessageWidget *const shieldMessageWidget =
        AllocateHudObject<HudUiShieldMessageWidget>();
    if (shieldMessageWidget != 0) {
        new (&shieldMessageWidget->widget) HudUiWidget(0);
        HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
        percentTextPanel->ConstructorDefault(
            0,
            0,
            0
        );
        percentTextPanel->SetTextColor(0x0020bf40);
        percentTextPanel->HudUiPanel::SetFont(
            g_HudFontName_Arial,
            0x0a,
            0x1f4,
            6,
            0,
            0,
            2
        );
        percentTextPanel->SetShadow(
            1,
            -1,
            -1
        );
        new (&shieldMessageWidget->meter) HudUiShieldMeterCandidate;
    }
    g_HudUiMgrShieldMessageWidget = shieldMessageWidget;

    HudUiCounterTextPanel *const counterTextPanel = AllocateHudObject<HudUiCounterTextPanel>();
    g_HudUiMgrObjectiveCounterTextPanel =
        counterTextPanel != 0 ? new (counterTextPanel) HudUiCounterTextPanel : 0;

    HudUiTimerPanel *const timerPanel = AllocateHudObject<HudUiTimerPanel>();
    g_HudUiMgrTimerPanel = timerPanel != 0 ? new (timerPanel) HudUiTimerPanel : 0;

    HudUiStatsListElement *const statsList = AllocateHudObject<HudUiStatsListElement>();
    if (statsList != 0) {
        new (statsList) HudUiStatsListElement;

        HudUiTriplet *const statsTriplet = AllocateHudObject<HudUiTriplet>();
        statsList->triplet = statsTriplet != 0 ? new (statsTriplet) HudUiTriplet : 0;
    }
    g_HudUiMgrStatsList = statsList;

    HudUiPanel *const objectiveSummaryTextPanel = new HudUiPanel;
    objectiveSummaryTextPanel->HudUiPanel::SetFont(
        g_HudFontName_Arial,
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    g_HudUiMgrObjectiveSummaryTextPanel = objectiveSummaryTextPanel;

    HudUiPanel *const objectiveDescTextPanel = new HudUiPanel;
    if (objectiveDescTextPanel != 0) {
        objectiveDescTextPanel->SetTextColorsAndMarkDirty(
            0x0020bf40,
            0x0020bf40
        );
        objectiveDescTextPanel->HudUiPanel::SetFont(
            g_HudFontName_Arial,
            0x0a,
            0x1f4,
            6,
            0,
            0,
            2
        );
    }
    g_HudUiMgrObjectiveDescTextPanel = objectiveDescTextPanel;

    HudUiPanel *const objectiveLabelTextPanel = new HudUiPanel;
    if (objectiveLabelTextPanel != 0) {
        objectiveLabelTextPanel->SetTextColorsAndMarkDirty(
            0x0020bf40,
            0x0020bf40
        );
        objectiveLabelTextPanel->HudUiPanel::SetFont(
            g_HudFontName_Arial,
            0x0a,
            0x1f4,
            6,
            0,
            0,
            2
        );
        objectiveLabelTextPanel->SetShadow(
            1,
            -1,
            -1
        );
    }
    g_HudUiMgrObjectiveLabelTextPanel = objectiveLabelTextPanel;

    HudUiTopMessageStack *const topMessageStack = AllocateHudObject<HudUiTopMessageStack>();
    g_HudUiTopMessageStack = topMessageStack != 0 ? topMessageStack->Constructor() : 0;

    HudUiChatMessageStack *const chatMessageStack = AllocateHudObject<HudUiChatMessageStack>();
    g_HudUiChatMessageStack = chatMessageStack != 0 ? chatMessageStack->Constructor() : 0;

    g_HudUiMgrHudLoaded = 0;
    g_HudUiMgrLayoutDelayFrames = 0;
    g_HudUiMgrStatsListState1 = 0;
    g_HudUiMgrStatsListState2 = 0;
    g_HudUiMgrStatsListState3 = 0;
    g_HudUiMgrStatsListState5 = 0;

    ActivateHud(
        displaySection,
        windowSection
    );
    g_HudUiMgrHudOriginX = displaySection->right - 0x280;
    g_HudUiMgrHudOriginY = displaySection->bottom - 0x1e0;

    g_HudUiTripletWndClassName = AfxRegisterWndClass(
        0x83,
        0,
        0,
        0
    );

    zUtil_ZAR::RegisterSectionHandler(
        g_HudUiTimerPanel_NodeName,
        (zZbdSectionCallback)(&HudUiTimerPanel::ZarWriteTimerDataCallback),
        (zZbdSectionCallback)(&HudUiTimerPanel::ZarReadTimerData),
        0x64,
        g_HudUiMgrTimerPanel
    );

    g_HudUiMgrHudLayoutsInitialized = 1;
    if (g_HudUiMgrStatsList != 0) {
        g_HudUiMgrStatsList->SetVisible(1);
        g_HudUiMgr.AddChild(g_HudUiMgrStatsList);
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanel-settextcolor
 * @recoil-artifact defines .text recoil:function:0x40f9e0: HudUiPanel::SetTextColor.
 * Purpose: Sets both panel text colors, marks text metrics dirty, and returns the old primary color.
 */
unsigned int HudUiPanel::SetTextColor(
    unsigned int color
) {
    const unsigned int previous = textColor0;
    textColor0 = color;
    textColor1 = color;
    textDirty = 1;
    return previous;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduistatslistelement-update
 * @recoil-artifact defines .text recoil:function:0x40fa10: HudUiStatsListElement::Update.
 * Purpose: Forward frame updates to the owned scoreboard triplet.
 */
void HudUiStatsListElement::Update(
    float deltaSeconds
) {
    triplet->UpdateAll(deltaSeconds);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduistatslistelement-destructor-huduistatslistelement
 * @recoil-artifact defines .text recoil:function:0x40fa40: HudUiStatsListElement::~HudUiStatsListElement.
 * Purpose: Destroy the owned scoreboard triplet and clear the member during stats-list teardown.
 */
HudUiStatsListElement::~HudUiStatsListElement() {
    delete triplet;
    triplet = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduipanelsimple-constructor
 * @recoil-artifact defines .text recoil:function:0x40fac0: HudUiPanelSimple::HudUiPanelSimple.
 * Purpose: construct a simple HUD text panel with the default green font and shadow state.
 */
HudUiPanelSimple::HudUiPanelSimple(
    const char *text,
    int initX,
    int initY
) : HudUiPanel(text, initX, initY) {
    textColor0 = 0x0020bf40;
    textColor1 = 0x0020bf40;
    textDirty = 1;
    HudUiPanel::SetFont(
        g_HudFontName_Arial,
        0x0a,
        0x1f4,
        6,
        0,
        0,
        2
    );
    shadowOffsetX = -1;
    shadowOffsetY = -1;
    shadowEnabled = 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduishieldmetercandidate-huduishieldmetercandidate
 * @recoil-artifact defines .text recoil:function:0x40fb70: HudUiShieldMeterCandidate::HudUiShieldMeterCandidate.
 * Retail constructs this shield sibling directly from HudUiBar rather than
 * through the manager-meter base branch.
 * Purpose: construct the shield meter and clear its fill state.
 */
HudUiShieldMeterCandidate::HudUiShieldMeterCandidate() : HudUiBar() {
    fillPixelsMax = 0;
    meterFlags = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-zarwritetimerdatacallback
 * @recoil-artifact defines .text recoil:function:0x40fb90: HudUiTimerPanel::ZarWriteTimerDataCallback.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: write the timer elapsed-seconds blob into the HUD timer data section.
 */
void __fastcall HudUiTimerPanel::ZarWriteTimerDataCallback(
    zZbdSectionCallbackCtx *sectionCtx,
    HudUiTimerPanel *userData
) {
    zUtil_ZAR::WriteSectionBlob(
        sectionCtx,
        g_HudUiTimerPanel_TimerDataSectionName,
        &userData->elapsedSeconds,
        sizeof(userData->elapsedSeconds)
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitimerpanel-zarreadtimerdata
 * @recoil-artifact defines .text recoil:function:0x40fbb0: HudUiTimerPanel::ZarReadTimerData.
 * Source owner: hud_ui.hud_ui_timer_panel_class.
 * Purpose: load persisted timer seconds and start the objective HUD flow.
 */
void __stdcall HudUiTimerPanel::ZarReadTimerData(
    const float *buffer,
    int byteCount,
    HudUiTimerPanel *userData
) {
    (void)byteCount;

    userData->UpdateHMSFromSeconds(*buffer);
    HudUiMgrObjective::Begin();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-shutdownresources
 * @recoil-artifact defines .text recoil:function:0x40fbd0: HudUiMgr::ShutdownResources.
 * Purpose: release HUD image resources, destroy allocated HUD widgets, and reset manager-owned globals during shutdown.
 */
void HudUiMgr::ShutdownResources() {
    g_HudUiMgrSensorPanel.Shutdown();
    g_HudUiMgrSensorOverlay.Shutdown();
    g_HudUiMgrObjectiveWidget.Shutdown();

    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[0]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[1]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrReticleImages[2]);

    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrSensorTargetMarkerImages[0]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrSensorTargetMarkerImages[1]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrSensorTargetMarkerImages[2]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrSensorTargetMarkerImages[3]);
    zVid_Image::ReleaseIfNotDefault(g_HudUiMgrSensorTargetMarkerImages[4]);

    g_HudUiMgrNanitePanel.ShutdownItems_Stub();
    HudLayoutBase::Shutdown_Stub();

    {
        for (size_t index = 1; index < 10; ++index) {
            g_HudUiMgrMessages[index].ReleaseImages();
        }
    }

    {
        int counterIndex12;
        for (counterIndex12 = 0; counterIndex12 < (int)(sizeof(g_HudUiMgrModeCounters) /
                                                        sizeof(g_HudUiMgrModeCounters[0]));
            ++counterIndex12) {
            HudUiCounter &counter = g_HudUiMgrModeCounters[counterIndex12];
            counter.ReleaseStateImages();
        }
    }

    zGame::ReturnOnlyStub();
    g_HudLayoutHW.ReleaseImages();

    if (g_HudUiMgrTimerPanelFloat != 0) {
        delete ((HudUiPanel *)(g_HudUiMgrTimerPanelFloat));
        g_HudUiMgrTimerPanelFloat = 0;
    }

    if (g_HudUiMgrStringMenu != 0) {
        delete g_HudUiMgrStringMenu;
        g_HudUiMgrStringMenu = 0;
    }

    if (g_HudUiMgrShieldMessageWidget != 0) {
        delete g_HudUiMgrShieldMessageWidget;
        g_HudUiMgrShieldMessageWidget = 0;
    }

    if (g_HudUiMgrObjectiveCounterTextPanel != 0) {
        delete ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel));
        g_HudUiMgrObjectiveCounterTextPanel = 0;
    }

    if (g_HudUiMgrTimerPanel != 0) {
        delete ((HudUiPanel *)(g_HudUiMgrTimerPanel));
        g_HudUiMgrTimerPanel = 0;
    }

    if (g_HudUiMgrStatsList != 0) {
        delete g_HudUiMgrStatsList;
        g_HudUiMgrStatsList = 0;
    }

    if (g_HudUiMgrObjectiveSummaryTextPanel != 0) {
        delete g_HudUiMgrObjectiveSummaryTextPanel;
        g_HudUiMgrObjectiveSummaryTextPanel = 0;
    }

    if (g_HudUiMgrObjectiveDescTextPanel != 0) {
        delete g_HudUiMgrObjectiveDescTextPanel;
        g_HudUiMgrObjectiveDescTextPanel = 0;
    }

    if (g_HudUiMgrObjectiveLabelTextPanel != 0) {
        delete g_HudUiMgrObjectiveLabelTextPanel;
        g_HudUiMgrObjectiveLabelTextPanel = 0;
    }

    if (g_HudUiTopMessageStack != 0) {
        delete ((HudUiTopMessageStack *)(g_HudUiTopMessageStack));
        g_HudUiTopMessageStack = 0;
    }

    if (g_HudUiChatMessageStack != 0) {
        delete ((HudUiChatMessageStack *)(g_HudUiChatMessageStack));
        g_HudUiChatMessageStack = 0;
    }

    g_HudUiMgrHudLayoutsInitialized = 0;
    g_HudUiMgrHudLoaded = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-activatehud
 * @recoil-artifact defines .text recoil:function:0x40ff50: HudUiMgr::ActivateHud.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: activate the HUD viewport, reset shield message state, and enable
 * the sensor HUD block.
 */
void __fastcall HudUiMgr::ActivateHud(
    const HudUiRect *hudRectOrNull,
    const HudUiRect *viewRectOrNull
) {
    OnViewportChanged(
        hudRectOrNull,
        viewRectOrNull
    );
    g_HudUiMgrShieldMessageWidget->viewportResetFrame = -1;
    g_HudUiMgrShieldMessageWidget->state = 0;
    g_HudUiMgrSensorBlock.state = 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-onviewportchanged
 * @recoil-artifact defines .text recoil:function:0x40ff80: HudUiMgr::OnViewportChanged.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update HUD/view rectangle globals and refresh active viewport HUD
 * widgets after a viewport change.
 */
void __fastcall HudUiMgr::OnViewportChanged(
    const HudUiRect *hudRectOrNull,
    const HudUiRect *viewRectOrNull
) {
    if (hudRectOrNull != 0) {
        g_HudUiMgrHudRect = *hudRectOrNull;
    } else {
        hudRectOrNull = &g_HudUiMgrHudRect;
    }

    if (viewRectOrNull != 0) {
        g_HudUiMgrViewRect = *viewRectOrNull;
    } else {
        viewRectOrNull = &g_HudUiMgrViewRect;
    }

    const int viewWidth = viewRectOrNull->right - viewRectOrNull->left;
    const float viewWidthFloat = (float)(viewWidth);
    const int viewHeight = viewRectOrNull->bottom - viewRectOrNull->top;
    const float viewHeightFloat = (float)(viewHeight);

    g_HudUiMgrHudRectW = (float)(hudRectOrNull->right - hudRectOrNull->left);
    g_HudUiMgrHudRectH = (float)(hudRectOrNull->bottom - hudRectOrNull->top);
    g_HudUiMgrReticleMapBiasX = (float)(hudRectOrNull->left);
    g_HudUiMgrReticleMapBiasY = (float)(hudRectOrNull->top);

    const int snapRadius = viewWidth / 10;
    g_HudUiMgrReticleMapScaleHalfW = (g_HudUiMgrHudRectW / viewWidthFloat) * viewWidthFloat * 0.5f;
    g_HudUiMgrReticleSnapRadiusSq = snapRadius * snapRadius;
    g_HudUiMgrReticleMapScaleHalfH =
        (g_HudUiMgrHudRectH / viewHeightFloat) * viewHeightFloat * 0.5f;

    HudUiMgrSensor::SetViewportRect(
        g_HudUiMgrSensorFxRect.left,
        g_HudUiMgrSensorFxRect.top,
        g_HudUiMgrSensorFxViewportWidth,
        g_HudUiMgrSensorFxViewportHeight
    );

    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->OnActivated();
    }

    HudUiMgrObjective::Update();

    if (g_HudUiTopMessageStack != 0) {
        g_HudUiTopMessageStack->SetTextColors(
            0x0020bf40,
            0x0020bf40
        );
        g_HudUiTopMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
    }

    if (g_HudUiChatMessageStack != 0) {
        g_HudUiChatMessageStack->SetTextColors(
            0x0020bf40,
            0x0020bf40
        );
        g_HudUiChatMessageStack->SetXAll(zVideo::GetPrimarySurfaceWidth() / 2);
        g_HudUiChatMessageStack->SetYDescending(zVideo::GetPrimarySurfaceHeight() - 0x88);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimgr-ticklayoutdelay
 * @recoil-artifact defines .text recoil:function:0x410140: HudUiMgr::TickLayoutDelay.
 * Purpose: consume one pending HUD layout delay frame when a delayed layout
 * transition is active.
 */
int HudUiMgr::TickLayoutDelay() {
    if (g_HudUiMgrLayoutDelayFrames == 0) {
        return 0;
    }

    --g_HudUiMgrLayoutDelayFrames;
    return 1;
}
namespace {
struct HudReticleAttachStatePartial {
    unsigned char unknown_00[0x0c];
    zClass_NodePartial *projectileNode;
};

struct HudReticleAltGunControllerPartial {
    OptCatalogEntryDef *optCatalogEntry;
    unsigned char unknown_04[0x24];
    HudReticleAttachStatePartial *attachState;
};

struct HudReticlePlayerStatePartial {
    unsigned char unknown_000[0x58c];
    int cameraState;
    unsigned char unknown_590[0x54];
    HudReticleAltGunControllerPartial *activeAltGunController;
    unsigned char unknown_5e8[0x8e8];
    zClass_NodePartial *rootNode;
};

RECOIL_STATIC_ASSERT(offsetof(HudReticleAttachStatePartial, projectileNode) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(HudReticleAltGunControllerPartial, attachState) == 0x28);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, cameraState) == 0x58c);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, activeAltGunController) == 0x5e4);
RECOIL_STATIC_ASSERT(offsetof(HudReticlePlayerStatePartial, rootNode) == 0xed0);
} // namespace

extern char g_HudCfgKey_Modes[6];
extern char g_HudCfgKey_Weapon[7];
extern char g_HudCfgKey_Target[7];
extern char g_HudCfgKey_Shield[7];
extern char g_HudUiBlankSpaces8[9];
extern char g_HudCfgKey_Stats[6];
extern char g_HudCfgKey_Reticule[9];
extern char g_HudCfgKey_Objective[10];
extern char g_HudCfgKey_Sensor[7];
extern char g_HudCfgKey_Nanite[7];
extern char g_HudCfgKey_Ammo[5];
extern char g_HudCfgKey_Strings[8];
extern char g_HudCfgKey_ObjectiveDescription[16];
extern char g_HudCfgKey_ObjectiveSummary[12];
extern char g_HudCfgKey_Fonts[6];
extern char g_Hud_ImageSearchPath_Hud[26];
extern char g_Hud_SourceFile_HudCpp[28];
extern char g_HudLayout_TypeISectionName[];
extern char g_HudLayout_TypeIISectionName[];
extern char g_HudUiBlankSpaces3[4];
extern char g_HudUiTimerPanel_ZeroTimeString[9];
extern char g_HudUiMessage_ClearSpecialToken165[4];
union HudUiSensorWindowStorage {
    unsigned long align;
    unsigned char bytes[0x40];
};
RECOIL_STATIC_ASSERT(sizeof(HudUiSensorWindowStorage) == 0x40);
extern HudUiSensorWindowStorage g_HudUiSensorWindow;
extern zFMV_Playback *g_HudUiSensorWindowPlayback;
extern char g_Hud_CheckpointOverflowMsg[20];

namespace {
const float kHudUiMessageClearSpecialTokenValue = 123456792.0f;

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiSetFontFromRect.
 */
void HudUiSetFontFromRect(
    HudUiPanel *panel,
    const HudUiRect &fontSpec
) {
    panel->SetFont(
        (const char *)(fontSpec.left),
        fontSpec.right,
        fontSpec.bottom,
        fontSpec.top,
        0,
        0,
        2
    );
}

/**
 * Original-source helper; no standalone retail function exists.
 * Evidence: recovered in the HUD source cluster near address-backed 0x40d7e0 HudUiMgr::Constructor callers.
 * Purpose: preserve the recovered HUD behavior for HudUiSetPanelClipWithSource.
 */
void HudUiSetPanelClipWithSource(
    HudUiPanel *panel,
    void *source,
    const HudUiRect *clipRect
) {
    panel->SetClip(
        source,
        clipRect
    );
}

} // namespace

namespace HudUiMgr {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.ensurehudloaded
 * @recoil-artifact defines .text recoil:function:0x410160: HudUiMgr::EnsureHudLoaded.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: load the HUD archive tree, initialize layout resources, and
 * finalize HUD visibility state.
 */
int __fastcall EnsureHudLoaded(
    const char *entryPath
) {
    if (g_HudUiMgrHudLoaded != 0) {
        return 1;
    }

    zReader::Node *const root = zReader::LoadNodeFromPath(
        entryPath,
        0,
        0
    );
    if (root == 0) {
        zError::ReportOld(
            0x200,
            g_Hud_SourceFile_HudCpp,
            0x60d,
            g_HudSensorTracker_ReadFileFailedFmt,
            entryPath
        );
        return 0;
    }

    zImage_InitMissionResources(g_Hud_ImageSearchPath_Hud);
    g_HudLayoutSW.LoadTypeIFromZarRoot(root);
    g_HudLayoutHW.LoadTypeIIFromZarRoot(root);
    SwitchActiveDialog(&g_HudLayoutSW);

    HudUiRect objectiveSummaryFont = {0};
    HudUiRect objectiveDescriptionFont = {0};
    HudUiRect ammoFont = {0};

    zReader::Node *const fontsNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Fonts
    );
    if (fontsNode != 0) {
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            g_HudCfgKey_ObjectiveSummary
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &objectiveSummaryFont
            );
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            g_HudCfgKey_ObjectiveDescription
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &objectiveDescriptionFont
            );
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            g_HudCfgKey_Strings
        )) {
            HudUiPanelFontParams *const fontArgs =
                (HudUiPanelFontParams *)(&g_HudUiMgrStringMenu->unknown_10[0]);
            HudUiLayoutNode::ReadRect(
                node,
                (HudUiRect *)(fontArgs)
            );
            {
                int itemIndex4;
                for (itemIndex4 = 0; itemIndex4 < (int)(sizeof(g_HudUiMgrStringMenu->items) /
                                                        sizeof(g_HudUiMgrStringMenu->items[0]));
                    ++itemIndex4) {
                    HudUiPanelSimple &item = g_HudUiMgrStringMenu->items[itemIndex4];
                    item.SetFont(
                        fontArgs->faceName,
                        fontArgs->height,
                        fontArgs->weight,
                        fontArgs->width,
                        0,
                        0,
                        2
                    );
                }
            }
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            "MESSAGES"
        )) {
            HudUiRect messagesFont = {0};
            HudUiLayoutNode::ReadRect(
                node,
                &messagesFont
            );
            if (g_HudUiTopMessageStack != 0) {
                g_HudUiTopMessageStack->SetFontAll(
                    (const char *)(messagesFont.left),
                    messagesFont.right,
                    messagesFont.bottom,
                    messagesFont.top
                );
            }
            if (g_HudUiChatMessageStack != 0) {
                g_HudUiChatMessageStack->SetFontAll(
                    (const char *)(messagesFont.left),
                    messagesFont.right,
                    messagesFont.bottom,
                    messagesFont.top
                );
            }
        }
        if (zReader::Node *const node = zReader_GetNamedNode(
            fontsNode,
            g_HudCfgKey_Ammo
        )) {
            HudUiLayoutNode::ReadRect(
                node,
                &ammoFont
            );
        }
    }

    if (zReader::Node *const naniteNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Nanite
    )) {
        g_HudUiMgrNanitePanel.InitLayout(naniteNode);
    }

    zReader::Node *const sensorNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Sensor
    );
    int sensorCenterX = 0;
    int sensorCenterY = 0;
    if (sensorNode != 0) {
        zReader::Node *const sensorPayload = sensorNode->value.nodes;
        HudUiLayoutNode::ApplyImageWidget(
            &sensorPayload[1],
            &g_HudUiMgrSensorPanel,
            0,
            g_HudUiMgrHudOriginY,
            0,
            0,
            &g_HudUiMgrSensorBlock.sensorViewportRect
        );

        sensorCenterX = g_HudUiMgrSensorPanel.GetCenterX();
        sensorCenterY = g_HudUiMgrSensorPanel.GetCenterY();
        g_HudUiMgrSensorBlock.sensorParam = sensorPayload[2].value.f32;

        int sensorOffsetX = 0;
        int sensorOffsetY = 0;
        int sensorWidth = 0;
        int sensorHeight = 0;
        HudUiLayoutNode::ReadInt3(
            &sensorPayload[3],
            &sensorOffsetX,
            &sensorOffsetY,
            0
        );
        HudUiLayoutNode::ReadInt3(
            &sensorPayload[4],
            &sensorWidth,
            &sensorHeight,
            0
        );

        const int sensorX = sensorCenterX + sensorOffsetX;
        const int sensorY = sensorCenterY + sensorOffsetY;
        g_HudUiMgrSensorFxRect.left = sensorX;
        g_HudUiMgrSensorFxRect.top = sensorY;
        g_HudUiMgrSensorFxRect.right = sensorX + sensorWidth;
        g_HudUiMgrSensorFxRect.bottom = sensorY + sensorHeight;
        g_HudUiMgrSensorFxViewportWidth = sensorWidth;
        g_HudUiMgrSensorFxViewportHeight = sensorHeight;
        HudUiMgrSensor::SetViewportRect(
            sensorX,
            sensorY,
            sensorWidth,
            sensorHeight
        );

        const float range = sensorPayload[5].value.f32;
        float rangeBitsValue = range * range * 0.5f;
        unsigned int rangeBits = 0;
        memcpy(
            &rangeBits,
            &rangeBitsValue,
            sizeof(rangeBits)
        );
        rangeBits = (rangeBits >> 1) + 0x1fc00000u;
        memcpy(
            &rangeBitsValue,
            &rangeBits,
            sizeof(rangeBitsValue)
        );
        g_HudUiMgrSensorBlock.sensorRangeSq = rangeBitsValue + rangeBitsValue;

        const int overlayAnchor[2] = {sensorCenterX, sensorCenterY};
        HudUiLayoutNode::ApplyImageWidget(
            &sensorPayload[6],
            &g_HudUiMgrSensorOverlay,
            0,
            0,
            overlayAnchor,
            0,
            0
        );

        HudUiRect meterRect = {0};
        HudUiLayoutNode::ApplyMeterQuad(
            &sensorPayload[7],
            &g_HudUiMgrSensorMeter,
            0,
            0,
            overlayAnchor,
            &meterRect
        );
        g_HudUiMgrSensorMeter.color565 = 0x7e0;
        ((HudUiElement *)(&g_HudUiMgrSensorMeter))
            ->SetBltSourceAndClipRect(
                g_HudUiMgrSensorPanel.image,
                &meterRect
            );

        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorOverlay));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrSensorMeter));
    }

    if (zReader::Node *const objectiveNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Objective
    )) {
        zReader::Node *const objectivePayload = objectiveNode->value.nodes;
        g_HudUiMgrObjectivePhaseDurationSec = objectivePayload[1].value.f32;

        const int panelCenter[2] = {sensorCenterX != 0 ? sensorCenterX
                                                       : g_HudUiMgrSensorPanel.GetCenterX(),
            sensorCenterY != 0 ? sensorCenterY : g_HudUiMgrSensorPanel.GetCenterY()};
        HudUiLayoutNode::ApplyImageWidget(
            &objectivePayload[2],
            &g_HudUiMgrObjectiveWidget,
            0,
            0,
            panelCenter,
            0,
            0
        );

        int objectiveCenter[2] = {g_HudUiMgrObjectiveWidget.GetCenterX(),
            g_HudUiMgrObjectiveWidget.GetCenterY()};
        HudUiRect objectiveBarRect = {0};
        HudUiLayoutNode::ApplyCornerTextQuad(
            &objectivePayload[3],
            &g_HudUiMgrObjectiveBar,
            objectiveCenter,
            &objectiveBarRect
        );
        g_HudUiMgrObjectiveBar.slideRangeX =
            (float)(panelCenter[0] - objectiveBarRect.left);

        int red = 0;
        int green = 0;
        int blue = 0;
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[4],
            &red,
            &green,
            &blue
        );
        g_HudUiMgrObjectiveBar.drawParam =
            zVid_PackColorRGB(
                (unsigned char)(red),
                (unsigned char)(green),
                (unsigned char)(blue)
            ) &
            0xffffu;

        int x = 0;
        int y = 0;
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[5],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel))
            ->SetPos(
                objectiveCenter[0] + x,
                objectiveCenter[1] + y
            );
        HudUiLayoutNode::ReadInt3(
            &objectivePayload[6],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))
            ->SetPos(
                objectiveCenter[0] + x,
                objectiveCenter[1] + y
            );

        HudUiRect wrapRect = {0};
        wrapRect.left = 0;
        wrapRect.top = 0;
        wrapRect.right = panelCenter[0] - x * 2 - objectiveBarRect.left;
        wrapRect.bottom = panelCenter[1] - objectiveBarRect.bottom;
        g_HudUiMgrObjectiveDescTextPanel->EnableWordWrapWithRect(&wrapRect);

        HudUiLayoutNode::ApplyMeterQuad(
            &objectivePayload[7],
            &g_HudUiMgrObjectiveMeter,
            0,
            0,
            objectiveCenter,
            &objectiveBarRect
        );
        HudUiMgrObjective::UpdateMeterXPoints();
        const int meterTop = (int)(g_HudUiMgrObjectiveMeter.points[1].y) -
                             (int)(ceil((double)(g_HudUiMgrObjectiveMeter.fillPixelsMax)));
        g_HudUiMgrObjectiveMeter.color565 = 0x1f;
        g_HudUiMgrObjectiveMeter.points[0].y = (float)(meterTop);
        g_HudUiMgrObjectiveMeter.points[3].y = (float)(meterTop);

        HudUiLayoutNode::ReadInt3(
            &objectivePayload[8],
            &x,
            &y,
            0
        );
        ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetPos(
            x,
            y + g_HudUiMgrHudOriginY
        );
        g_HudUiMgrObjectiveLabelTextPanel->SetTextFmt(
            zLoc::GetMessageString(0x906)
        );
        ((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect))
            ->SetPos(
                g_HudUiMgrSensorFxRect.left,
                g_HudUiMgrSensorFxRect.top
            );

        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveWidget));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect));
        g_HudUiMgr.AddChild(&g_HudUiMgrObjectiveBar);
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel));
        g_HudUiMgr.AddChild((HudUiElement *)(&g_HudUiMgrObjectiveMeter));
        g_HudUiMgrObjectiveBar.SetVisible(0);

        g_HudUiMgrObjectiveState = 0;
        g_HudUiMgrObjectivePhase = 0;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        g_HudUiMgrObjectiveChatComposeActive = 0;
        g_HudUiMgrObjectiveDescTextPanel->SetFont(
            (const char *)(objectiveDescriptionFont.left),
            objectiveDescriptionFont.right,
            objectiveDescriptionFont.bottom,
            objectiveDescriptionFont.top,
            0,
            0,
            2
        );
        g_HudUiMgrObjectiveSummaryTextPanel->SetFont(
            (const char *)(objectiveSummaryFont.left),
            objectiveSummaryFont.right,
            objectiveSummaryFont.bottom,
            objectiveSummaryFont.top,
            0,
            0,
            2
        );
    }

    if (zReader::Node *const reticleNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Reticule
    )) {
        zReader::Node *const reticlePayload = reticleNode->value.nodes;
        g_HudUiMgrReticleImages[0] =
            zImage::TexDir_FindOrCreateByPath(reticlePayload[1].value.str);
        g_HudUiMgrReticleImages[1] =
            zImage::TexDir_FindOrCreateByPath(reticlePayload[2].value.str);
        g_HudUiMgrReticleImages[2] =
            zImage::TexDir_FindOrCreateByPath(reticlePayload[3].value.str);
        g_HudUiMgrReticleWidget.SetImageBorrowedAndInvalidate(g_HudUiMgrReticleImages[0]);
        g_HudUiMgrReticleWidget.imageStateWord =
            (g_HudUiMgrReticleWidget.imageStateWord & 0xffff0000u) | 1u;
        ((HudUiElement *)(&g_HudUiMgrReticleWidget))->Invalidate();
        zVidImagePartial *const image = g_HudUiMgrReticleWidget.image;
        g_HudUiMgrReticleWidgetHalfW = image != 0 ? (short)(image->width) / 2 : 0;
        g_HudUiMgrReticleWidgetHalfH = image != 0 ? (short)(image->height) / 2 : 0;
        ((HudUiElement *)(&g_HudUiMgrReticleWidget))->SetVisible(0);
    }

    if (zReader::Node *const statsNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Stats
    )) {
        zReader::Node *const statsPayload = statsNode->value.nodes;
        HudUiWidget *const layoutWidget = &g_HudLayoutHW.widget1;
        const int layoutCenterX = layoutWidget->GetCenterX();
        const int layoutCenterY = layoutWidget->GetCenterY();
        int x;
        int y;
        int z;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[1],
            &x,
            &y,
            0
        );
        const int counterX = (g_HudUiMgrHudOriginX / 2) + x;
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))
            ->SetPos(
                counterX + layoutCenterX,
                y + layoutCenterY
            );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->alignMode = 1;
        HudUiRect counterClip = {counterX - 0x14, y, counterX + 0x14, y + 0x0a};
        g_HudUiMgrObjectiveCounterTextPanel->SetBltSourceAndClipRect(
            0,
            &counterClip
        );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->SetTextFmt(g_HudUiBlankSpaces8);
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->UpdateTextBoundsFromContent();
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->SetTextFmt(
            "%d",
            0
        );
        ((HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel))->UpdateTextBoundsFromContent();

        HudUiLayoutNode::ReadInt3(
            &statsPayload[2],
            &x,
            &y,
            0
        );
        const int timerX = x + g_HudUiMgrHudOriginX;
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetPos(
            timerX + layoutCenterX,
            y + layoutCenterY
        );
        HudUiRect timerClip = {timerX, y, 0, 0};
        g_HudUiMgrTimerPanel->SetBltSourceAndClipRect(
            0,
            &timerClip
        );
        ((HudUiPanel *)(g_HudUiMgrTimerPanel))->SetTextFmt(g_HudUiTimerPanel_ZeroTimeString);

        HudUiTriplet *const triplet = g_HudUiMgrStatsList->triplet;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[3],
            &x,
            &y,
            &z
        );
        triplet->baseXStart = x + layoutCenterX + g_HudUiMgrHudOriginX;
        triplet->baseYStart = y + layoutCenterY;
        triplet->rowPitchYStart = z;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[4],
            &x,
            &y,
            &z
        );
        triplet->baseXEnd = x + layoutCenterX + g_HudUiMgrHudOriginX;
        triplet->baseYEnd = y + layoutCenterY;
        triplet->rowPitchYEnd = z;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[5],
            &x,
            &y,
            0
        );
        triplet->lapsColumnOffsetXStart = x;
        triplet->lapsColumnOffsetXEnd = y;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[6],
            &x,
            &y,
            0
        );
        triplet->killsColumnOffsetXStart = x;
        triplet->killsColumnOffsetXEnd = y;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[7],
            &x,
            &y,
            0
        );
        triplet->fontSizeStart = x;
        triplet->fontSizeEnd = y;
        HudUiLayoutNode::ReadInt3(
            &statsPayload[8],
            &x,
            &y,
            0
        );
        triplet->fontWeightStart = x;
        triplet->fontWeightEnd = y;
        triplet->InterpolateLayout(0.0f);
        triplet->RebuildDisplay();
    }

    if (zReader::Node *const shieldNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Shield
    )) {
        HudUiShieldMessageWidget::ApplyLayout(shieldNode);
    }

    if (zReader::Node *const targetNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Target
    )) {
        zReader::Node *const targetPayload = targetNode->value.nodes;
        g_HudUiMgrSensorTargetMarkerImages[0] =
            zImage::TexDir_FindOrCreateByPath(targetPayload[1].value.str);
        g_HudUiMgrSensorTargetMarkerImages[1] =
            zImage::TexDir_FindOrCreateByPath(targetPayload[2].value.str);
        g_HudUiMgrSensorTargetMarkerImages[2] =
            zImage::TexDir_FindOrCreateByPath(targetPayload[3].value.str);
        g_HudUiMgrSensorTargetMarkerImages[3] =
            zImage::TexDir_FindOrCreateByPath(targetPayload[4].value.str);
        g_HudUiMgrSensorTargetMarkerImages[4] =
            zImage::TexDir_FindOrCreateByPath(targetPayload[5].value.str);

        {
            int slotIndex5;
            for (slotIndex5 = 0; slotIndex5 < (int)(sizeof(g_HudUiMgrWeaponSlots) /
                                                    sizeof(g_HudUiMgrWeaponSlots[0]));
                ++slotIndex5) {
                HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex5];
                slot.trackMarkerWidget.imageStateWord =
                    (slot.trackMarkerWidget.imageStateWord & 0xffff0000u) | 1u;
                ((HudUiElement *)(&slot.trackMarkerWidget))->Invalidate();
            }
        }

        {
            int slotIndex6;
            for (slotIndex6 = 0; slotIndex6 < (int)(sizeof(g_HudUiMgrWeaponSlots) /
                                                    sizeof(g_HudUiMgrWeaponSlots[0]));
                ++slotIndex6) {
                HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex6];
                ((HudUiElement *)(&slot.slotWidget))->Invalidate();
                g_HudUiMgr.AddChild(&slot);
                ((HudUiElement *)(&slot.trackMarkerWidget))->SetVisible(0);
                ((HudUiElement *)(&slot.slotWidget))->SetVisible(0);
            }
        }
        g_HudUiMgrSensorTargetMarkerCount = 0;
        g_HudUiMgrWeaponState = 0;
    }

    zReader::Node *weaponNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Weapon
    );
    if (weaponNode != 0) {
        zReader::Node *const weaponPayload = weaponNode->value.nodes;
        {
            for (int index = 1; index < 10; ++index) {
                g_HudUiMgrMessages[index].LoadWeaponLayoutFromNode(
                    &weaponPayload[index],
                    (const HudUiPanelFontParams *)(&ammoFont)
                );
            }
        }
    }

    zReader::Node *modesNode = zReader_GetNamedNode(
        root,
        g_HudCfgKey_Modes
    );
    if (modesNode != 0) {
        zReader::Node *const modesPayload = modesNode->value.nodes;
        {
            for (int index = 0; index < 4; ++index) {
                g_HudUiMgrModeCounters[index].ApplyFromLayoutNode(&modesPayload[index + 1]);
            }
        }
    }

    SetModeCounterState(
        0,
        2
    );
    zReader::FreeLoadedTree(root);
    SetFloatTimerVisible(0);
    SetAuxOverlayVisible(0);
    g_HudUiMgrHudLoaded = 1;
    return 1;
}

} // namespace HudUiMgr

namespace HudUiMgrSensor {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setviewportrect
 * @recoil-artifact defines .text recoil:function:0x410d10: HudUiMgrSensor::SetViewportRect.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zhud_ui.cpp.
 * Purpose: store raw/scaled HUD sensor viewport bounds and update the active source rectangle.
 */
void __fastcall SetViewportRect(
    int x,
    int y,
    int width,
    int height
) {
    const int right = x + width;
    const int bottom = y + height;

    g_HudUiMgrSensorBlock.sensorRectRaw.left = x;
    g_HudUiMgrSensorBlock.sensorRectRaw.right = right;
    g_HudUiMgrSensorBlock.sensorRectRaw.top = y;
    g_HudUiMgrSensorBlock.sensorRectRaw.bottom = bottom;

    if (zOpt::GetReplicateMode() == 0) {
        g_HudUiMgrSensorBlock.sensorRectScaled = g_HudUiMgrSensorBlock.sensorRectRaw;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = (float)(x);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = (float)(y);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.right = (float)(right);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom = (float)(bottom);
    } else {
        const int halfX = x / 2;
        const int halfY = y / 2;
        const int halfWidth = width / 2;
        const int halfHeight = height / 2;

        g_HudUiMgrSensorBlock.sensorPiVSrcRect.left = (float)(halfX);
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.top = (float)(halfY);
        g_HudUiMgrSensorBlock.sensorRectScaled.left = halfX;
        g_HudUiMgrSensorBlock.sensorRectScaled.top = halfY;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.right =
            (float)(halfWidth) + g_HudUiMgrSensorBlock.sensorPiVSrcRect.left;
        g_HudUiMgrSensorBlock.sensorRectScaled.right = halfX + halfWidth;
        g_HudUiMgrSensorBlock.sensorRectScaled.bottom = halfY + halfHeight;
        g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom =
            (float)(halfHeight) + g_HudUiMgrSensorBlock.sensorPiVSrcRect.top;
    }

    g_HudUiMgrSensorBlock.sensorClampHalfW = (g_HudUiMgrSensorBlock.sensorPiVSrcRect.right -
                                                 g_HudUiMgrSensorBlock.sensorPiVSrcRect.left) /
                                             g_HudUiMgrSensorBlock.sensorParam;
    g_HudUiMgrSensorBlock.sensorClampHalfH = (g_HudUiMgrSensorBlock.sensorPiVSrcRect.bottom -
                                                 g_HudUiMgrSensorBlock.sensorPiVSrcRect.top) /
                                             g_HudUiMgrSensorBlock.sensorParam;
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);
}

} // namespace HudUiMgrSensor

namespace HudUiMgr {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.enablehud
 * @recoil-artifact defines .text recoil:function:0x410e90: HudUiMgr::EnableHud.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::EnableHud.
 */
int EnableHud() {
    const int previouslyEnabled = g_HudUiMgr.enabled;
    g_HudUiMgr.SetEnabled(1);

    g_HudUiMgrCurrentLayout->Enable();

    HudUiMgrObjective::Update();
    zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);
    gAltClipPassEnabled = 1;
    return previouslyEnabled;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.disablehud
 * @recoil-artifact defines .text recoil:function:0x410ed0: HudUiMgr::DisableHud.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::DisableHud.
 */
int DisableHud() {
    const int previouslyEnabled = g_HudUiMgr.enabled;
    DestroySensorWindow();

    {
        int slotIndex;
        for (slotIndex = 0;
            slotIndex < (int)(sizeof(g_HudUiMgrWeaponSlots) / sizeof(g_HudUiMgrWeaponSlots[0]));
            ++slotIndex) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex];
            slot.trackMarkerWidget.SetVisible(0);
            slot.slotWidget.SetVisible(0);
        }
    }

    g_HudUiMgrSensorTargetMarkerCount = 0;
    g_HudUiMgrWeaponState = 0;
    g_HudUiMgr.SetEnabled(0);

    g_HudUiMgrCurrentLayout->Disable();

    g_HudUiMgrObjectiveWidget.SetVisible(0);
    g_HudUiMgrObjectiveDescTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveBar.SetVisible(0);
    g_HudUiMgrObjectiveSensorRect.SetVisible(0);
    g_HudUiMgrObjectiveSummaryTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveLabelTextPanel->SetVisible(0);
    g_HudUiMgrObjectiveMeter.SetVisible(0);

    gAltClipPassEnabled = 0;
    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionResetFrame();
    }

    const int hudType = zOpt::GetHudTypeForCurrentHwMode();
    if (hudType == 2) {
        g_HudUiMgrLayoutDelayFrames = hudType;
    }

    g_HudUiMgrTimerPanel->SetVisible(1);
    return previouslyEnabled;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updateframe
 * @recoil-artifact defines .text recoil:function:0x410fe0: HudUiMgr::UpdateFrame.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: run the per-frame HudUiMgr update sequence for the active layout,
 * HUD containers, timers, reticle widget, and transient weapon slot state.
 */
void UpdateFrame() {
    g_HudUiMgrCurrentLayout->LayoutPreUpdate();

    if (g_HudUiMgr.enabled != 0) {
        if (g_HudUiMgrObjectiveState != 0) {
            HudUiMgrObjective::StartHide();
        }
    } else {
        if ((g_HudUiMgr.objective.objectiveBar.chatComposeActive) != 0) {
            g_HudUiMgrObjectiveSummaryTextPanel->Draw();
            g_HudUiMgrObjectiveDescTextPanel->Draw();
        }

        g_HudUiMgrTimerPanel->Update(
            g_Time_UnscaledDeltaTimeSec
        );
    }

    if (g_HudUiMgrObjectiveMeterFillAnimEnabled != 0) {
        HudUiMgrObjective::TickMeterFillAnimation();
    }

    g_HudSensorTracker.Update();
    zTimedTask::TickActiveList();

    g_HudUiMgrCurrentLayout->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiMgr.HudUiContainer::UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiTopMessageStack->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiChatMessageStack->UpdateAll(g_Time_UnscaledDeltaTimeSec);
    g_HudUiMgrStringMenu->UpdateAll(g_Time_UnscaledDeltaTimeSec);

    const float sampleElapsedSec =
        g_HudUiMgrTimerPanelFloat->sampleElapsedSec + g_FrameDeltaTimeSec;
    g_HudUiMgrTimerPanelFloat->sampleElapsedSec = sampleElapsedSec;

    const float sampleFrameCount =
        g_HudUiMgrTimerPanelFloat->sampleFrameCount + 1.0f;
    g_HudUiMgrTimerPanelFloat->sampleFrameCount = sampleFrameCount;
    if (sampleElapsedSec >= 1.0f) {
        g_HudUiMgrTimerPanelFloat->sampleFrameCount = 0.0f;
        g_HudUiMgrTimerPanelFloat->sampleElapsedSec = 0.0f;
        g_HudUiMgrTimerPanelFloat->displayValue =
            sampleFrameCount / sampleElapsedSec;
    }

    HudUiElement *const floatingTimerElement = (HudUiElement *)(g_HudUiMgrTimerPanelFloat);
    if ((floatingTimerElement->flags & 0x10) == 0) {
        g_HudUiMgrTimerPanelFloat->Draw();
    }

    g_HudUiMgrReticleWidget.Update(
        g_Time_UnscaledDeltaTimeSec
    );

    {
        for (int slotIndex = 0; slotIndex < 32; ++slotIndex) {
            HudUiSlot &slot = g_HudUiMgrWeaponSlots[slotIndex];
            slot.trackMarkerWidget.SetVisible(0);
            slot.slotWidget.SetVisible(0);
        }
    }

    g_HudUiMgrSensorTargetMarkerCount = 0;
    g_HudUiMgrWeaponState = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.projectpointtonormalizedclamped
 * @recoil-artifact defines .text recoil:function:0x411170: HudUiMgr::ProjectPointToNormalizedClamped.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::ProjectPointToNormalizedClamped.
 */
int __fastcall ProjectPointToNormalizedClamped(
    const zVec3 *srcPoint,
    zVec3 *projectedPoint
) {
    if (zMath::ProjectPointAndClampToScreenClip(
        srcPoint,
        projectedPoint
    ) == 0x10) {
        return 1;
    }

    const float halfHudWidth = g_HudUiMgrHudRectW * 0.5f;
    const float halfHudHeight = g_HudUiMgrHudRectH * 0.5f;
    if (zOpt::GetReplicateMode() != 0) {
        projectedPoint->x += projectedPoint->x;
        projectedPoint->y += projectedPoint->y;
    }

    projectedPoint->x = (projectedPoint->x - halfHudWidth) / halfHudWidth;
    projectedPoint->y =
        (projectedPoint->y - (float)(g_HudUiMgrHudRect.top) - halfHudHeight) / halfHudHeight;

    if (projectedPoint->x > 1.0f) {
        projectedPoint->x = 1.0f;
    } else if (projectedPoint->x < -1.0f) {
        projectedPoint->x = -1.0f;
    }

    if (projectedPoint->y > 1.0f) {
        projectedPoint->y = 1.0f;
    } else if (projectedPoint->y < -1.0f) {
        projectedPoint->y = -1.0f;
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatetargetreticlefromcursor
 * @recoil-artifact defines .text recoil:function:0x411270: HudUiMgr::UpdateTargetReticleFromCursor.
 * Purpose: advance the recovered HUD update path for HudUiMgr::UpdateTargetReticleFromCursor.
 */
int __fastcall UpdateTargetReticleFromCursor(
    int reticleMode,
    zVec3 *worldHitPoint,
    float normalizedX,
    float normalizedY
) {
    HudUiElement *const reticleElement = (HudUiElement *)(&g_HudUiMgrReticleWidget);

    float screenX =
        (normalizedX + 1.0f) * g_HudUiMgrReticleMapScaleHalfW + g_HudUiMgrReticleMapBiasX;
    float screenY =
        (normalizedY + 1.0f) * g_HudUiMgrReticleMapScaleHalfH + g_HudUiMgrReticleMapBiasY;

    const int projectedX = (int)(screenX);
    const int projectedY = (int)(screenY);
    g_HudUiMgrReticleProjectedX = projectedX;
    g_HudUiMgrReticleProjectedY = projectedY;

    switch (reticleMode) {
    case 2:
        break;
    case 1:
        reticleElement->SetVisible(1);
        return 0;
    case 0:
        reticleElement->SetVisible(0);
        return 0;
    default:
        return 0;
    }

    reticleElement->SetPos(
        projectedX - g_HudUiMgrReticleWidgetHalfW,
        projectedY - g_HudUiMgrReticleWidgetHalfH
    );

    if ((g_HudLayoutHW.reticleClipInitFlags & 1) == 0) {
        g_HudLayoutHW.reticleClipInitFlags =
            (unsigned char)(g_HudLayoutHW.reticleClipInitFlags | 1);
        atexit(&HudUiMgr::ReticleStaticAtexitStub);
    }

    RECT reticleBounds = {0};
    reticleBounds.top = g_HudUiMgrReticleWidget.GetCenterY();
    reticleBounds.bottom =
        g_HudUiMgrReticleWidget.GetCenterY() +
        (g_HudUiMgrReticleWidget.image != 0 ? g_HudUiMgrReticleWidget.image->height : 0);
    reticleBounds.left = g_HudUiMgrReticleWidget.GetCenterX();
    reticleBounds.right =
        g_HudUiMgrReticleWidget.GetCenterX() +
        (g_HudUiMgrReticleWidget.image != 0 ? g_HudUiMgrReticleWidget.image->width : 0);

    if (IntersectRect(
            (RECT *)(&g_HudLayoutHW.reticleClipRect),
            &reticleBounds,
            (const RECT *)(zOpt::GetDisplaySection())
        ) != 0) {
        g_HudLayoutHW.reticleClipRect.top -= g_HudUiMgrReticleWidget.GetCenterY();
        g_HudLayoutHW.reticleClipRect.bottom -= g_HudUiMgrReticleWidget.GetCenterY();
        g_HudLayoutHW.reticleClipRect.left -= g_HudUiMgrReticleWidget.GetCenterX();
        g_HudLayoutHW.reticleClipRect.right -= g_HudUiMgrReticleWidget.GetCenterX();

        const int clippedX =
            g_HudUiMgrReticleWidget.GetCenterX() + g_HudLayoutHW.reticleClipRect.left;
        const int clippedY =
            g_HudUiMgrReticleWidget.GetCenterY() + g_HudLayoutHW.reticleClipRect.top;
        reticleElement->SetPos(
            clippedX,
            clippedY
        );
        g_HudUiMgrReticleWidget.bltClipRectOrNull = &g_HudLayoutHW.reticleClipRect;
    }

    zProjectedPoint projectedPoint = {screenX, screenY, 0.0f};
    ScreenToWorld(&projectedPoint.x);

    HudReticlePlayerStatePartial *const playerState =
        (HudReticlePlayerStatePartial *)(g_GameStateOrMapTable->playerState);

    float nearClip = 0.0f;
    float farClip = 0.0f;
    zClass_Camera::gwCameraGetNearFarClip(
        g_MainCamera,
        &nearClip,
        &farClip
    );

    zVec3 nearPoint = {0};
    projectedPoint.reciprocalZ = 1.0f / nearClip;
    zMath_UnprojectPointBatchZBuf(
        &projectedPoint,
        &nearPoint,
        1
    );

    zVec3 farPoint = {0};
    projectedPoint.reciprocalZ = 1.0f / playerState->activeAltGunController->optCatalogEntry->range;
    zMath_UnprojectPointBatchZBuf(
        &projectedPoint,
        &farPoint,
        1
    );

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    if (playerState->cameraState == 7) {
        zClass_Class::gwNodeSetRaycastable(
            playerState->activeAltGunController->attachState->projectileNode,
            0
        );
    }

    zClass_cls_di::SetStopAfterFirstHit(0x40000);
    PlayerProbeSampleCandidateBuffer rayData = {0};
    const int raycastResult = zClass_cls_di::RaycastSelectClosestHitBetweenPoints(
        g_Player_RuntimeDiScene,
        &nearPoint,
        &farPoint,
        &rayData
    );

    zClass_Class::gwNodeSetRaycastable(
        playerState->rootNode,
        0
    );
    if (playerState->cameraState == 7) {
        zClass_Class::gwNodeSetRaycastable(
            playerState->activeAltGunController->attachState->projectileNode,
            1
        );
    }

    zVidImagePartial *reticleImage = 0;
    if (raycastResult != 0) {
        g_HudUiMgrReticleProjection[0] = farPoint.x;
        g_HudUiMgrReticleProjection[1] = farPoint.y;
        g_HudUiMgrReticleProjection[2] = farPoint.z;
        reticleImage = g_HudUiMgrReticleImages[1];
    } else {
        const zClassDiPickCandidateEntry &candidate = rayData.entries[rayData.candidateCount];
        g_HudUiMgrReticleProjection[0] = candidate.hitPos.x;
        g_HudUiMgrReticleProjection[1] = candidate.hitPos.y;
        g_HudUiMgrReticleProjection[2] = candidate.hitPos.z;

        zClass_NodeFreeListSlot *const hitSlot = (zClass_NodeFreeListSlot *)(candidate.node);
        reticleImage =
            hitSlot->damageHandler != 0 ? g_HudUiMgrReticleImages[2] : g_HudUiMgrReticleImages[0];
    }

    g_HudUiMgrReticleWidget.SetImageBorrowedAndInvalidate(reticleImage);

    worldHitPoint->x = g_HudUiMgrReticleProjection[0];
    worldHitPoint->y = g_HudUiMgrReticleProjection[1];
    worldHitPoint->z = g_HudUiMgrReticleProjection[2];

    zOpt_ViewRectSection *const renderRect = zOpt::GetRenderSection();
    const float minX = (float)(renderRect->x) + g_HudUiMgrSensorBlock.sensorClampHalfW;
    if (!(screenX >= minX)) {
        screenX = minX;
    } else {
        const float maxX =
            (float)(renderRect->rightExclusive) - g_HudUiMgrSensorBlock.sensorClampHalfW;
        if (screenX > maxX) {
            screenX = maxX;
        }
    }

    const float minY = (float)(renderRect->y) + g_HudUiMgrSensorBlock.sensorClampHalfH;
    if (!(screenY >= minY)) {
        screenY = minY;
    } else {
        const float maxY =
            (float)(renderRect->bottomExclusive) - g_HudUiMgrSensorBlock.sensorClampHalfH;
        if (screenY > maxY) {
            screenY = maxY;
        }
    }

    zClipAltFloatRect targetRect = {screenX - g_HudUiMgrSensorBlock.sensorClampHalfW,
        screenY - g_HudUiMgrSensorBlock.sensorClampHalfH,
        screenX + g_HudUiMgrSensorBlock.sensorClampHalfW,
        screenY + g_HudUiMgrSensorBlock.sensorClampHalfH};
    zClipAlt::SetTargetRect(
        &targetRect,
        zOpt::GetReplicateMode()
    );
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.reticlestaticatexitstub
 * @recoil-artifact defines .text recoil:function:0x411710: HudUiMgr::ReticleStaticAtexitStub.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::ReticleStaticAtexitStub.
 */
void __cdecl ReticleStaticAtexitStub() {}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.copyreticleprojection
 * @recoil-artifact defines .text recoil:function:0x411720: HudUiMgr::CopyReticleProjection.
 * Purpose: copy the HudUiMgr reticle projection vector into the caller-owned
 * three-float output buffer.
 */
void __fastcall CopyReticleProjection(
    float *outProjection
) {
    unsigned int *const outBits = (unsigned int *)(outProjection);
    const unsigned int *const projectionBits = (const unsigned int *)(g_HudUiMgrReticleProjection);
    outBits[0] = projectionBits[0];
    outBits[1] = projectionBits[1];
    outBits[2] = projectionBits[2];
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setreticlemode
 * @recoil-artifact defines .text recoil:function:0x411740: HudUiMgr::SetReticleMode.
 * Purpose: store the active HUD reticle mode.
 */
void __fastcall SetReticleMode(
    int mode
) {
    g_HudUiMgrReticleMode = mode;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setnanitepanelcount
 * @recoil-artifact defines .text recoil:function:0x411750: HudUiMgr::SetNanitePanelCount.
 * Purpose: apply the recovered HUD state change handled by HudUiMgr::SetNanitePanelCount.
 */
void __fastcall SetNanitePanelCount(
    int count
) {
    g_HudUiMgrNanitePanel.SetVisibleCount(count);
}

} // namespace HudUiMgr

namespace HudUiMgrObjective {
/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: repeated objective phase runtime update of the widget right
 * edge after slide-position changes.
 * Purpose: refresh the cached objective widget right edge from its current
 * center position and borrowed image width.
 */
static inline void HudUiMgrObjective_UpdateWidgetRightX() {
    const zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;
    g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + width;
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: repeated phase animation sequence updates the objective bar
 * slide edge, invalidates the bar, moves the widget, and recomputes meter X
 * points as one source-level operation.
 * Purpose: apply the objective panel slide X position and dependent meter
 * geometry.
 */
static inline void HudUiMgrObjective_SetSlidePosition(
    float slideX
) {
    g_HudUiMgrObjectiveBar.points[2].x = slideX;
    g_HudUiMgrObjectiveBar.points[3].x = slideX;
    g_HudUiMgrObjectiveBar.Invalidate();
    ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))->SetX((int)(slideX)-1);
    HudUiMgrObjective::UpdateMeterXPoints();
}

/**
 * Recovered original helper with no standalone retail function. Observed in
 * caller 0x411ac0: HudUiMgrObjective::StartHide.
 * Evidence basis: phase-3 animation branches share the same hardware-HUD dirty
 * rectangle gate through zOpt::GetHudTypeForCurrentHwMode.
 * Purpose: update the hardware HUD objective dirty rectangle only for the
 * hardware perspective HUD mode.
 */
static inline void HudUiMgrObjective_UpdateHwDirtyRectIfNeeded() {
    if (zOpt::GetHudTypeForCurrentHwMode() == 2) {
        g_HudLayoutHW.UpdateObjectiveDirtyRect();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setvisibleandresetmeterfill
 * @recoil-artifact defines .text recoil:function:0x411760: HudUiMgrObjective::SetVisibleAndResetMeterFill.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: toggle the objective label and meter visibility, and restart the
 * objective meter fill animation from the meter bottom when showing.
 */
void __fastcall SetVisibleAndResetMeterFill(
    int visible
) {
    if (visible != 0) {
        g_HudUiMgrObjectiveLabelTextPanel->SetVisible(1);
        g_HudUiMgrObjectiveMeter.SetVisible(1);

        const int meterTop = (int)(g_HudUiMgrObjectiveMeter.points[1].y) -
                             (int)(ceil((double)(g_HudUiMgrObjectiveMeter.fillPixelsMax) * 0.0));
        g_HudUiMgrObjectiveMeterFillAnimTimerSec = 0.0f;
        g_HudUiMgrObjectiveMeterFillAnimEnabled = 1;
        g_HudUiMgrObjectiveMeter.points[0].y = (float)(meterTop);
        g_HudUiMgrObjectiveMeter.points[3].y = (float)(meterTop);
    } else {
        g_HudUiMgrObjectiveLabelTextPanel->SetVisible(0);
        g_HudUiMgrObjectiveMeter.SetVisible(0);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.tickmeterfillanimation
 * @recoil-artifact defines .text recoil:function:0x4117f0: HudUiMgrObjective::TickMeterFillAnimation.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: advance the objective meter fill timer, update the animated top
 * edge, and stop the animation once the meter reaches full height.
 */
void TickMeterFillAnimation() {
    g_HudUiMgrObjectiveMeterFillAnimTimerSec += g_Time_UnscaledDeltaTimeSec;

    if (g_HudUiMgrObjectiveMeterFillAnimTimerSec >= 3.0f) {
        const int fillPixels =
            (int)(ceil((double)(g_HudUiMgrObjectiveMeter.fillPixelsMax)));
        g_HudUiMgrObjectiveMeterFillAnimEnabled = 0;
        const int top = (int)(g_HudUiMgrObjectiveMeter.points[1].y) - fillPixels;
        g_HudUiMgrObjectiveMeter.points[0].y = (float)(top);
        g_HudUiMgrObjectiveMeter.points[3].y = (float)(top);
    } else {
        const double fillRatio = (double)(g_HudUiMgrObjectiveMeterFillAnimTimerSec * 0.333332986f) *
                                 (double)(g_HudUiMgrObjectiveMeter.fillPixelsMax);
        const int fillPixels = (int)(ceil(fillRatio));
        const int top = (int)(g_HudUiMgrObjectiveMeter.points[1].y) - fillPixels;
        g_HudUiMgrObjectiveMeter.points[0].y = (float)(top);
        g_HudUiMgrObjectiveMeter.points[3].y = (float)(top);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatemeterxpoints
 * @recoil-artifact defines .text recoil:function:0x4118b0: HudUiMgrObjective::UpdateMeterXPoints.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: recompute the objective meter X edges from the objective widget
 * center position.
 */
void UpdateMeterXPoints() {
    const float left = (float)(g_HudUiMgrObjectiveWidget.GetCenterX()) + 5.0f;
    const float right = left + 7.0f;
    g_HudUiMgrObjectiveMeter.points[0].x = left;
    g_HudUiMgrObjectiveMeter.points[1].x = left;
    g_HudUiMgrObjectiveMeter.points[2].x = right;
    g_HudUiMgrObjectiveMeter.points[3].x = right;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.show
 * @recoil-artifact defines .text recoil:function:0x411900: HudUiMgrObjective::Show.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Start or update the objective HUD panel with summary text, description text, and image state.
 */
int __fastcall Show(
    zVidImagePartial *objectiveImage,
    const char *summaryFormat,
    const char *descText,
    float autoHideDelay
) {
    if (summaryFormat == 0 || descText == 0 ||g_HudUiMgrObjectiveChatComposeActive != 0) {
        return 0;
    }

    g_HudUiMgrObjectiveSummaryTextPanel->SetTextFmt(summaryFormat);
    g_HudUiMgrObjectiveDescTextPanel->SetTextFmt(descText);
    g_HudUiMgrSensorOverlay.SetVisible(0);

    const int phase = g_HudUiMgrObjectivePhase;
    if (phase == 0) {
        g_HudUiMgrObjectiveSensorRect.SetImageBorrowedAndInvalidate(objectiveImage);
        zVidImagePartial *const widgetImage = g_HudUiMgrObjectiveWidget.image;
        g_HudUiMgrObjectiveState = 1;
        g_HudUiMgrObjectivePhase = 1;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        g_HudUiMgrObjectiveShowResetUnused = 0;
        g_HudUiMgrObjectiveAutoHideDelaySec = autoHideDelay;

        const int imageWidth = widgetImage != 0 ? widgetImage->width : 0;
        g_HudUiMgrObjectiveWidgetRightX = g_HudUiMgrObjectiveWidget.GetCenterX() + imageWidth;
        g_HudUiMgrObjectiveBar.SetVisible(1);
        gAltClipPassEnabled = 0;
        return 1;
    }

    if (phase == 3) {
        g_HudUiMgrObjectivePhase = 1;
        g_HudUiMgrObjectivePhaseTimerSec =
            g_HudUiMgrObjectivePhaseDurationSec - g_HudUiMgrObjectivePhaseTimerSec;
        return 1;
    }

    g_HudUiMgrObjectiveSensorRect.SetImageBorrowedAndInvalidate(objectiveImage);
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.begin
 * @recoil-artifact defines .text recoil:function:0x411a20: HudUiMgrObjective::Begin.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Transition the objective panel into its begin/close phase while respecting chat-compose input.
 */
void Begin() {
    if ((g_HudUiMgr.objective.objectiveBar.chatComposeActive) != 0) {
        return;
    }

    const int phase = g_HudUiMgrObjectivePhase;
    if (phase == 2) {
        g_HudUiMgrObjectiveState = 1;
        g_HudUiMgrObjectivePhase = 3;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;

        g_HudUiMgrObjectiveSummaryTextPanel->SetVisible(0);
        g_HudUiMgrObjectiveDescTextPanel->SetVisible(0);
        g_HudUiMgrObjectiveSensorRect.SetVisible(0);
        g_HudUiMgrObjectiveAutoHideDelaySec = 0.0f;
        return;
    }

    if (phase == 1) {
        g_HudUiMgrObjectivePhase = 3;
        g_HudUiMgrObjectivePhaseTimerSec =
            g_HudUiMgrObjectivePhaseDurationSec - g_HudUiMgrObjectivePhaseTimerSec;
        g_HudUiMgrObjectiveAutoHideDelaySec = 0.0f;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.starthide
 * @recoil-artifact defines .text recoil:function:0x411ac0: HudUiMgrObjective::StartHide.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: advance objective panel show/hide phases, keep slide and meter
 * geometry synchronized, manage transition visibility, and trigger auto-hide
 * completion.
 */
void StartHide() {
    g_HudUiMgrObjectivePhaseTimerSec += g_Time_UnscaledDeltaTimeSec;

    float noise;

    do {
    switch (g_HudUiMgrObjectivePhase) {
    case 1: {
        if (g_HudUiMgrObjectivePhaseTimerSec < g_HudUiMgrObjectivePhaseDurationSec) {
            const float fade =
                g_HudUiMgrObjectivePhaseTimerSec / g_HudUiMgrObjectivePhaseDurationSec;
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                fade * g_HudUiMgrObjectiveBar.slideRangeX;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateWidgetRightX();
            if (g_HudUiMgrObjectiveSensorRect.image != 0) {
                noise = fade + fade;
                if (noise < 1.0f) {
                    zVid::DrawNoiseRect(
                        (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
                        (double)noise
                    );
                    continue;
                } else {
                    g_HudUiMgrObjectiveSensorRect.SetVisible(1);
                    break;
                }
            }
            continue;
        }

        const float slideX =
            g_HudUiMgrObjectiveBar.points[1].x + g_HudUiMgrObjectiveBar.slideRangeX;
        g_HudUiMgrObjectivePhase = 2;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        HudUiMgrObjective_SetSlidePosition(slideX);
        HudUiMgrObjective_UpdateWidgetRightX();
        g_HudUiMgrObjectiveSummaryTextPanel->SetVisible(1);
        g_HudUiMgrObjectiveDescTextPanel->SetVisible(1);
        g_HudUiMgrObjectiveSensorRect.SetVisible(1);
        continue;
    }

    case 2:
        ((HudUiElement *)(g_HudUiMgrObjectiveSummaryTextPanel))->Invalidate();
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->Invalidate();
        g_HudUiMgrObjectiveBar.Invalidate();
        ((HudUiElement *)(&g_HudUiMgrObjectiveSensorRect))->Invalidate();
        continue;

    case 3: {
        if (g_HudUiMgrObjectivePhaseTimerSec < g_HudUiMgrObjectivePhaseDurationSec) {
            const float fade =
                1.0f - g_HudUiMgrObjectivePhaseTimerSec / g_HudUiMgrObjectivePhaseDurationSec;
            const float slideX =
                g_HudUiMgrObjectiveBar.points[1].x +
                fade * g_HudUiMgrObjectiveBar.slideRangeX;
            HudUiMgrObjective_SetSlidePosition(slideX);
            HudUiMgrObjective_UpdateHwDirtyRectIfNeeded();
            HudUiMgrObjective_UpdateWidgetRightX();
            if (g_HudUiMgrObjectiveSensorRect.image != 0) {
                noise = fade + fade;
                if (noise < 1.0f) {
                    zVid::DrawNoiseRect(
                        (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
                        (double)noise
                    );
                    continue;
                } else {
                    g_HudUiMgrObjectiveSensorRect.SetVisible(0);
                    break;
                }
            }
            continue;
        }

        g_HudUiMgrObjectiveState = 0;
        g_HudUiMgrObjectivePhase = 0;
        g_HudUiMgrObjectivePhaseTimerSec = 0.0f;
        ((HudUiElement *)(&g_HudUiMgrObjectiveWidget))
            ->SetX((int)(g_HudUiMgrObjectiveBar.points[1].x));
        HudUiMgrObjective::UpdateMeterXPoints();
        HudUiMgrObjective_UpdateHwDirtyRectIfNeeded();
        HudUiMgrObjective_UpdateWidgetRightX();
        g_HudUiMgrObjectiveBar.SetVisible(0);
        g_HudUiMgrSensorOverlay.SetVisible(1);
        gAltClipPassEnabled = 1;
        continue;
    }

    default:
        continue;
    }

    zVid::DrawNoiseRect(
        (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw),
        (double)(2.0f - noise)
    );
    } while (0);

    if (g_HudUiMgrObjectiveAutoHideDelaySec != 0.0f) {
        if (g_HudUiMgrObjectivePhaseTimerSec >= g_HudUiMgrObjectiveAutoHideDelaySec) {
            HudUiMgrObjective::Begin();
        }

        g_HudUiMgrObjectiveState = 1;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.update
 * @recoil-artifact defines .text recoil:function:0x411eb0: HudUiMgrObjective::Update.
 * Purpose: advance the recovered HUD update path for HudUiMgrObjective::Update.
 */
void Update() {
    g_HudUiMgrObjectiveWidget.SetVisible(1);
    if (g_HudUiMgrObjectivePhase == 0) {
        return;
    }

    g_HudUiMgrObjectiveBar.SetVisible(1);
    if (g_HudUiMgrObjectivePhase != 2) {
        return;
    }

    if (g_HudUiMgrObjectiveDescTextPanel != 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveDescTextPanel))->SetVisible(1);
    }

    if (g_HudUiMgrObjectiveLabelTextPanel != 0) {
        ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->SetVisible(1);
    }

    g_HudUiMgrObjectiveSensorRect.SetVisible(1);
}

} // namespace HudUiMgrObjective

namespace HudUiMgrSensor {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setshieldmessageratio
 * @recoil-artifact defines .text recoil:function:0x411f10: HudUiMgrSensor::SetShieldMessageRatio.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: clamp the shield ratio, update the HudUiMgr shield meter, and
 * refresh the shield percent text.
 */
void __fastcall SetShieldMessageRatio(
    float ratio
) {
    if (ratio > 1.0f) {
        ratio = 1.0f;
    } else if (ratio < 0.0f) {
        ratio = 0.0f;
    }

    if (ratio < 0.25f) {
        g_HudUiMgrShieldMessageWidget->meter.color565 = zVid_PackColorRGB(
            255,
            0,
            0
        ) & 0xffffu;
    } else {
        g_HudUiMgrShieldMessageWidget->meter.color565 = zVid_PackColorRGB(
            255,
            255,
            0
        ) & 0xffffu;
    }

    HudUiShieldMessageWidget *const shieldMessageWidget = g_HudUiMgrShieldMessageWidget;
    HudUiBar *const meter = &shieldMessageWidget->meter;
    const int fillPixels = (int)(ceil((double)(meter->fillPixelsMax) * (double)(ratio)));
    const int top = (int)(meter->points[1].y) - fillPixels;
    meter->points[0].y = (float)(top);
    meter->points[3].y = (float)(top);
    meter->Invalidate();

    HudUiPanel *const percentTextPanel = (HudUiPanel *)(&shieldMessageWidget->percentTextPanel);
    const int percent = (int)(ceil((double)(ratio) * 100.0));
    percentTextPanel->SetTextFmt(
        "%d",
        percent
    );
    percentTextPanel->Invalidate();
}

} // namespace HudUiMgrSensor

namespace HudUiMgrObjective {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.refreshcountertext
 * @recoil-artifact defines .text recoil:function:0x412050: HudUiMgrObjective::RefreshCounterText.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: format the objective counter panel from the supplied integer value
 * and rebuild its text bounds.
 */
void __fastcall RefreshCounterText(
    int counterValue
) {
    HudUiPanel *const panel = (HudUiPanel *)(g_HudUiMgrObjectiveCounterTextPanel);
    panel->SetTextFmt(
        "%d",
        counterValue
    );
    panel->UpdateTextBoundsFromContent();
}

} // namespace HudUiMgrObjective

namespace HudUiMgrSensor {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.placetrackcounterwidget
 * @recoil-artifact defines .text recoil:function:0x412070: HudUiMgrSensor::PlaceTrackCounterWidget.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMgrSensor.cpp.
 * Binary Ninja/source evidence keeps this in the sensor-target runtime owner:
 * one typed HudUiSlot is taken from g_HudUiMgrWeaponSlots, projected through
 * zMath into the slot screen fields, then clamped against the recovered
 * HudUiMgrSensorBlock viewport bounds for edge marker placement.
 * Purpose: reserve and position one sensor target marker slot for a tracked
 * player or turret world point.
 */
int __fastcall PlaceTrackCounterWidget(
    HudUiMgrSensorTrackNode *trackNode,
    const zVec3 *worldPoint
) {
    const int targetMarkerCount = g_HudUiMgrSensorTargetMarkerCount;
    int inBounds = 0;
    if (targetMarkerCount >= 32) {
        return 0;
    }

    HudUiSlot *const slot = &g_HudUiMgrWeaponSlots[targetMarkerCount];
    g_HudUiMgrSensorTargetMarkerCount = targetMarkerCount + 1;

    const int screenEdgeCode =
        zMath::ProjectPointAndClampToScreenClip(
            worldPoint,
            (zVec3 *)(&slot->screenX)
        );

    int slotX;
    float slotY;
    if (zOpt::GetReplicateMode() != 0) {
        slotX = (int)(slot->screenX + slot->screenX);
        slotY = slot->screenY + slot->screenY;
    } else {
        slotX = (int)(slot->screenX);
        slotY = slot->screenY;
    }
    slot->SetPos(
        slotX,
        (int)(slotY)
    );

    switch (screenEdgeCode) {
    case 0:
        inBounds = 1;
        break;

    case 1: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[1]);

        const int halfHeight = counterWidget->image->height / 2;
        int top = slot->GetCenterY() - halfHeight;
        if (top <= g_HudUiMgrHudRect.top + halfHeight) {
            top = g_HudUiMgrHudRect.top;
        } else if (top > g_HudUiMgrSensorBlock.sensorViewportRect.top - halfHeight) {
            top = g_HudUiMgrSensorBlock.sensorViewportRect.top - halfHeight * 2;
        }
        counterWidget->SetPos(
            0,
            top
        );
        break;
    }

    case 2: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[2]);

        int top = slot->GetCenterY();
        const zVidImagePartial *const image = counterWidget->image;
        const int height = image->height;
        top -= height;
        if (top <= g_HudUiMgrHudRect.top + height) {
            top = g_HudUiMgrHudRect.top;
        } else if (top > g_HudUiMgrHudRect.bottom - height) {
            top = g_HudUiMgrHudRect.bottom - height * 2;
        }

        counterWidget->SetPos(
            slot->GetCenterX() + 1 - image->width,
            top
        );
        break;
    }

    case 4: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[3]);

        const zVidImagePartial *const image = counterWidget->image;
        counterWidget->SetPos(
            slot->GetCenterX() - image->width / 2,
            slot->GetCenterY() + 1
        );
        break;
    }

    case 8: {
        HudUiWidget *const counterWidget = &slot->slotWidget;
        counterWidget->SetVisible(1);
        counterWidget->SetImageBorrowedAndInvalidate(g_HudUiMgrSensorTargetMarkerImages[4]);

        int left = slot->GetCenterX();
        int top = slot->GetCenterY();
        if (left < g_HudUiMgrObjectiveWidgetRightX) {
            top = g_HudUiMgrSensorBlock.sensorViewportRect.top;
        }

        const zVidImagePartial *const image = counterWidget->image;
        top -= image->height;
        left -= image->width / 2;
        counterWidget->SetPos(
            left,
            top
        );
        break;
    }
    }

    slot->screenEdgeCode = screenEdgeCode;
    slot->trackNode = trackNode;
    return inBounds;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.placetrackmarker
 * @recoil-artifact defines .text recoil:function:0x4122c0: HudUiMgrSensor::PlaceTrackMarker.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMgrSensor.cpp.
 * The recovered source model walks the typed HudUiSlot sensor-marker range,
 * preserves the selected HudUiSlot pointer for progress updates, and uses the
 * track node kind as the discriminant for zUtil_SaveGameState versus
 * zTurret_Runtime payload casts before filling PlayerProgressTargetSlotRuntime.
 * Purpose: collect visible progress targets and highlight the nearest in-bounds
 * sensor marker when snap targeting is active.
 */
int __fastcall PlaceTrackMarker(
    int markerMode,
    PlayerProgressTargetSlotRuntime *outputSlots
) {
    const int HUD_SENSOR_MARKER_MODE_NEAREST = 1;
    const int HUD_SENSOR_MARKER_MODE_ALL = 2;

    HudUiSlot *const endSlot = &g_HudUiMgrWeaponSlots[g_HudUiMgrSensorTargetMarkerCount];
    HudUiSlot *slot = &g_HudUiMgrWeaponSlots[0];
    PlayerProgressTargetSlotRuntime *const firstOutputSlot = outputSlots;
    int result = 0;
    int nearestDistSq = 0x98967f;
    g_HudUiMgrSensorTrackedProgressSlot = 0;

    while (slot < endSlot) {
        if (slot->screenEdgeCode == 0) {
            if (markerMode == HUD_SENSOR_MARKER_MODE_ALL) {
                HudUiMgrSensorTrackNode *const trackNode =
                    (HudUiMgrSensorTrackNode *)(slot->trackNode);
                if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
                    zUtil_SaveGameState *const saveState =
                        (zUtil_SaveGameState *)(trackNode->payload);
                    zUtil_PlayerStateStorage *const playerState = saveState->playerState;
                    outputSlots->targetPos = &playerState->fxOffsetWorld;
                    outputSlots->targetVelocity = &playerState->projectileSpawnVel;
                    ++outputSlots;
                    ++result;
                } else if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
                    zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
                    outputSlots->targetPos = &turretRuntime->firePos;
                    outputSlots->targetVelocity = 0;
                    ++outputSlots;
                    ++result;
                }
            }

            const int dx = slot->GetCenterX() - g_HudUiMgrReticleProjectedX;
            const int dy = slot->GetCenterY() - g_HudUiMgrReticleProjectedY;
            const int distSq = dx * dx + dy * dy;
            if (distSq < nearestDistSq) {
                g_HudUiMgrSensorTrackedProgressSlot = slot;
                nearestDistSq = distSq;
            }
        }

        ++slot;
    }

    outputSlots = firstOutputSlot;
    if (markerMode != HUD_SENSOR_MARKER_MODE_NEAREST ||
        nearestDistSq >= g_HudUiMgrReticleSnapRadiusSq ||
        g_HudUiMgrSensorTrackedProgressSlot == 0) {
        return result;
    }

    HudUiSlot *const trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    trackedProgressSlot->trackMarkerWidget.SetImageBorrowedAndInvalidate(
        g_HudUiMgrSensorTargetMarkerImages[0]
    );

    const zVidImagePartial *const image = trackedProgressSlot->trackMarkerWidget.image;
    const int markerY = ((HudUiElement *)(trackedProgressSlot))->GetCenterY() - image->height / 2;
    const int markerX = ((HudUiElement *)(trackedProgressSlot))->GetCenterX() - image->width / 2;
    trackedProgressSlot->trackMarkerWidget.SetPos(
        markerX,
        markerY
    );
    trackedProgressSlot->trackMarkerWidget.SetVisible(1);

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(trackNode->payload);
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        outputSlots->targetPos = &playerState->fxOffsetWorld;
        outputSlots->targetVelocity = &playerState->projectileSpawnVel;
        return 1;
    }

    if (trackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
        zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(trackNode->payload);
        outputSlots->targetVelocity = 0;
        outputSlots->targetPos = &turretRuntime->firePos;
    }

    return 1;
}

} // namespace HudUiMgrSensor

namespace HudUiMgrTarget {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updateselectedprogressmeter
 * @recoil-artifact defines .text recoil:function:0x4124b0: HudUiMgrTarget::UpdateSelectedProgressMeter.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * The selected progress meter consumes the HudUiSlot pointer saved by the
 * sensor-target runtime, casts the HudUiMgrSensorTrackNode payload according to
 * its track-kind discriminant, remaps the slot projection through zClipAlt, and
 * updates the recovered HudUiMgrSensorBlock-owned meter.
 * Purpose: show the selected target health meter at the projected sensor marker
 * position, or clear the selection when requested.
 */
void __fastcall UpdateSelectedProgressMeter(
    int clearSelectedTrack
) {
    HudUiSlot *trackedProgressSlot = 0;
    if (clearSelectedTrack != 0) {
        g_HudUiMgrSensorTrackedProgressSlot = 0;
    } else {
        trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    }

    if (g_HudUiMgr.enabled == 0 || g_HudUiMgrObjectivePhase != 0 || trackedProgressSlot == 0) {
        return;
    }

    HudUiMgrSensorTrackNode *const selectedTrackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    float selectedHealthCurrent = 0.0f;
    float selectedHealthMax = 1.0f;
    if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_PLAYER) {
        zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(selectedTrackNode->payload);
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        selectedHealthCurrent = playerState->statusMeterValue;
        selectedHealthMax = playerState->masterCommonData->maxHealth;
    } else if (selectedTrackNode->trackKind == HUD_SENSOR_TRACK_KIND_TURRET) {
        zTurret_Runtime *const turretRuntime = (zTurret_Runtime *)(selectedTrackNode->payload);
        selectedHealthCurrent = turretRuntime->healthCurrent;
        selectedHealthMax = turretRuntime->healthMax;
    }

    if (selectedHealthCurrent == 0.0f) {
        g_HudUiMgrSensorMeter.SetVisible(0);
        trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    }

    if (zClipAlt::RemapPointXYInPlace(&trackedProgressSlot->screenX) == 0) {
        return;
    }

    if (zOpt::GetReplicateMode() != 0) {
        g_HudUiMgrSensorTrackedProgressSlot->screenX +=
            g_HudUiMgrSensorTrackedProgressSlot->screenX;
        g_HudUiMgrSensorTrackedProgressSlot->screenY +=
            g_HudUiMgrSensorTrackedProgressSlot->screenY;
    }

    float healthRatio = selectedHealthCurrent / selectedHealthMax;
    if (healthRatio > 1.0f) {
        healthRatio = 1.0f;
    } else if (healthRatio < 0.0f) {
        healthRatio = 0.0f;
    }

    const int fillPixels =
        (int)(ceil((double)(g_HudUiMgrSensorMeter.fillPixelsMax) * (double)(healthRatio)));
    const int top = (int)(g_HudUiMgrSensorMeter.points[1].y) - fillPixels;
    g_HudUiMgrSensorMeter.points[0].y = (float)(top);
    g_HudUiMgrSensorMeter.points[3].y = (float)(top);
    g_HudUiMgrSensorMeter.Invalidate();
    g_HudUiMgrSensorMeter.SetVisible(1);
}

} // namespace HudUiMgrTarget

namespace HudUiMgr {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hidetrackedprogressmeterifownermatches
 * @recoil-artifact defines .text recoil:function:0x412620: HudUiMgr::HideTrackedProgressMeterIfOwnerMatches.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::HideTrackedProgressMeterIfOwnerMatches.
 */
void __fastcall HideTrackedProgressMeterIfOwnerMatches(
    void *ownerPayload
) {
    HudUiSlot *const trackedProgressSlot = g_HudUiMgrSensorTrackedProgressSlot;
    if (trackedProgressSlot == 0) {
        return;
    }

    HudUiMgrSensorTrackNode *const trackNode =
        (HudUiMgrSensorTrackNode *)(trackedProgressSlot->trackNode);
    if (trackNode->payload == ownerPayload) {
        g_HudUiMgrSensorMeter.SetVisible(0);
    }
}

} // namespace HudUiMgr

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-setvalueifownermatches
 * @recoil-artifact defines .text recoil:function:0x412650: HudUiMessage::SetValueIfOwnerMatches.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Updates a message panel value only when the requested owner side matches the active side.
 */
void __fastcall HudUiMessage::SetValueIfOwnerMatches(
    int messageIndex,
    int ownerSideIndex,
    float valueOrClearToken
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    if (ownerSideIndex != message.panel.activeSideIndex) {
        return;
    }

    if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
        message.panel.SetTextFmt(g_HudUiMessage_ClearSpecialToken165);
        return;
    }

    message.panel.SetTextFmt(
        "%d",
        (int)(ceil(valueOrClearToken))
    );
    message.Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-selectvariantdisplay
 * @recoil-artifact defines .text recoil:function:0x4126e0: HudUiMessage::SelectVariantDisplay.
 * Purpose: Selects the visible weapon-message variant image and refreshes the active side-image state.
 */
void __fastcall HudUiMessage::SelectVariantDisplay(
    int messageIndex,
    int variantIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    message.SetImageBorrowedAndInvalidate(message.variantImages[variantIndex]);

    if (variantIndex == 0 || variantIndex == 3) {
        message.activeSideImages[0] = message.sideImageSwaps[0];
        message.widget.SetImageBorrowedAndInvalidate(message.activeSideImages[1]);
        message.panel.activeSideIndex = 0;
    }

    if (variantIndex == 5) {
        message.panel.activeSideIndex = 0;
    }

    if (variantIndex == 1 || variantIndex == 4) {
        message.activeSideImages[1] = message.sideImageSwaps[1];
        message.widget.SetImageBorrowedAndInvalidate(message.activeSideImages[0]);
        message.panel.activeSideIndex = 1;
    }

    if (variantIndex == 6) {
        message.panel.activeSideIndex = 1;
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-applysideimageswap
 * @recoil-artifact defines .text recoil:function:0x412790: HudUiMessage::ApplySideImageSwap.
 * Purpose: Applies a side-image replacement for the selected message slot and preserves the visible flag.
 */
void __fastcall HudUiMessage::ApplySideImageSwap(
    int messageIndex,
    int sideIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    zVidImagePartial *const image = message.sideImageSwaps[sideIndex];
    message.activeSideImages[sideIndex] = image;
    message.widget.SetImageBorrowedAndInvalidate(image);
    message.widget.flags &= 0x10u;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-cleardisplay
 * @recoil-artifact defines .text recoil:function:0x4127d0: HudUiMessage::ClearDisplay.
 * Purpose: Clears the message image, side image, and displayed text for one weapon-message slot.
 */
void __fastcall HudUiMessage::ClearDisplay(
    int messageIndex
) {
    HudUiMessage &message = g_HudUiMgrMessages[messageIndex];
    message.SetImageBorrowedAndInvalidate(0);
    message.widget.SetImageBorrowedAndInvalidate(0);

    message.panel.SetTextFmt("");
    message.Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-updateselectedweapondisplay
 * @recoil-artifact defines .text recoil:function:0x412820: HudUiMessage::UpdateSelectedWeaponDisplay.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Updates active weapon-message images, selected side ownership, and value text.
 */
void __fastcall HudUiMessage::UpdateSelectedWeaponDisplay(
    int weaponBankIndex,
    int weaponSideIndex,
    float valueOrClearToken
) {
    if (weaponBankIndex > 1) {
        {
            const int variantIndex = g_HudUiMgrActiveWeaponSideIndex;
            HudUiMessage &message =
                g_HudUiMgrMessages[g_HudUiMgrActiveWeaponMessageIndex];
            message.SetImageBorrowedAndInvalidate(
                message.variantImages[variantIndex]
            );

            if (variantIndex == 0 || variantIndex == 3) {
                message.activeSideImages[0] = message.sideImageSwaps[0];
                message.widget.SetImageBorrowedAndInvalidate(
                    message.activeSideImages[1]
                );
                message.panel.activeSideIndex = 0;
            }

            if (variantIndex == 5) {
                message.panel.activeSideIndex = 0;
            }

            if (variantIndex == 1 || variantIndex == 4) {
                message.activeSideImages[1] = message.sideImageSwaps[1];
                message.widget.SetImageBorrowedAndInvalidate(
                    message.activeSideImages[0]
                );
                message.panel.activeSideIndex = 1;
            }

            if (variantIndex == 6) {
                message.panel.activeSideIndex = 1;
            }
        }

        g_HudUiMgrActiveWeaponMessageIndex = weaponBankIndex;
        g_HudUiMgrActiveWeaponSideIndex = weaponSideIndex;
        if (valueOrClearToken > 0.0f) {
            const int variantIndex = weaponSideIndex + 3;
            HudUiMessage &message =
                g_HudUiMgrMessages[weaponBankIndex];
            message.SetImageBorrowedAndInvalidate(
                message.variantImages[variantIndex]
            );

            if (variantIndex == 0 || variantIndex == 3) {
                message.activeSideImages[0] = message.sideImageSwaps[0];
                message.widget.SetImageBorrowedAndInvalidate(
                    message.activeSideImages[1]
                );
                message.panel.activeSideIndex = 0;
            }

            if (variantIndex == 5) {
                message.panel.activeSideIndex = 0;
            }

            if (variantIndex == 1 || variantIndex == 4) {
                message.activeSideImages[1] = message.sideImageSwaps[1];
                message.widget.SetImageBorrowedAndInvalidate(
                    message.activeSideImages[0]
                );
                message.panel.activeSideIndex = 1;
            }

            if (variantIndex == 6) {
                message.panel.activeSideIndex = 1;
            }
        }

        HudUiMessage &message = g_HudUiMgrMessages[weaponBankIndex];
        if (weaponSideIndex != message.panel.activeSideIndex) {
            return;
        }

        if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
            message.panel.SetTextFmt(g_HudUiMessage_ClearSpecialToken165);
            return;
        }

        message.panel.SetTextFmt(
            "%d",
            (int)(ceil(valueOrClearToken))
        );
        message.Invalidate();
        return;
    } else if (weaponBankIndex == 1) {
        const int variantIndex = weaponSideIndex + 3;
        HudUiMessage &message = g_HudUiMgrMessages[1];
        message.SetImageBorrowedAndInvalidate(
            message.variantImages[variantIndex]
        );

        if (variantIndex == 0 || variantIndex == 3) {
            message.activeSideImages[0] = message.sideImageSwaps[0];
            message.widget.SetImageBorrowedAndInvalidate(
                message.activeSideImages[1]
            );
            message.panel.activeSideIndex = 0;
        }

        if (variantIndex == 5) {
            message.panel.activeSideIndex = 0;
        }

        if (variantIndex == 1 || variantIndex == 4) {
            message.activeSideImages[1] = message.sideImageSwaps[1];
            message.widget.SetImageBorrowedAndInvalidate(
                message.activeSideImages[0]
            );
            message.panel.activeSideIndex = 1;
        }

        if (variantIndex == 6) {
            message.panel.activeSideIndex = 1;
        }

        if (weaponSideIndex != message.panel.activeSideIndex) {
            return;
        }

        if (valueOrClearToken == kHudUiMessageClearSpecialTokenValue) {
            message.panel.SetTextFmt(g_HudUiMessage_ClearSpecialToken165);
            return;
        }

        message.panel.SetTextFmt(
            "%d",
            (int)(ceil(valueOrClearToken))
        );
        message.Invalidate();
        return;
    } else {
        g_HudUiMgrActiveWeaponMessageIndex = 0;
        g_HudUiMgrActiveWeaponSideIndex = 0;
    }
}

/**
 * Original inline constructor; no standalone retail function exists. The
 * 0x412b60 and 0x412ea0 derived constructors both contain the same automatic
 * HudUiContainer/widget0 construction followed by the Base vftable store and
 * AddChild call.
 * Purpose: construct the common layout base and attach its primary widget.
 */
inline HudLayoutBase::HudLayoutBase()
    : widget0(0) {
    AddChild(&widget0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutsw-hudlayoutsw
 * @recoil-artifact defines .text recoil:function:0x412b60: HudLayoutSW::HudLayoutSW.
 * Source file evidence: BN labels this function as a Battlesport hud.cpp helper.
 * Purpose: construct the software HUD layout through its automatic base lifetime.
 */
HudLayoutSW::HudLayoutSW() {
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutbase-setactive
 * @recoil-artifact defines .text recoil:function:0x412bd0: HudLayoutBase::SetActive.
 * Purpose: provide the default layout activation result for base layout callers.
 */
int HudLayoutBase::SetActive(
    int
) {
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutbase-updateall
 * @recoil-artifact defines .text recoil:function:0x412be0: HudLayoutBase::UpdateAll.
 * Purpose: forward per-frame layout updates through the recovered container base.
 */
void HudLayoutBase::UpdateAll(
    float deltaSeconds
) {
    HudUiContainer::UpdateAll(deltaSeconds);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutbase-enable
 * @recoil-artifact defines .text recoil:function:0x412bf0: HudLayoutBase::Enable.
 * Purpose: activate this HUD layout through the recovered base SetEnabled slot.
 */
void HudLayoutBase::Enable() {
    SetEnabled(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutbase-disable
 * @recoil-artifact defines .text recoil:function:0x412c00: HudLayoutBase::Disable.
 * Purpose: deactivate this HUD layout through the recovered base SetEnabled slot.
 */
void HudLayoutBase::Disable() {
    SetEnabled(0);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutbase-loadtypeifromzarroot
 * @recoil-artifact defines .text recoil:function:0x412c10: HudLayoutSW::LoadTypeIFromZarRoot.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: load the TYPEI HUD layout rectangle from the ZRD root.
 */
void HudLayoutBase::LoadTypeIFromZarRoot(
    zReader::Node *parentNode
) {
    zReader::Node *const typeINode = zReader_GetNamedNode(
        parentNode,
        g_HudLayout_TypeISectionName
    );
    if (typeINode == 0) {
        return;
    }

    HudUiLayoutNode::ReadRectOffsetAndSize(
        &typeINode->value.nodes[1],
        &layoutRect,
        0,
        0,
        0
    );
    activeRect = layoutRect;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayoutsw-setactive
 * @recoil-artifact defines .text recoil:function:0x412c60: HudLayoutSW::SetActive.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the software HUD viewport and active sensor occlusion state.
 */
int HudLayoutSW::SetActive(
    int active
) {
    if (zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
        zRndr::SpanOcclusionResetFrame();
    }

    activeRect.right = zVideo::GetPrimarySurfaceWidth();
    activeRect.bottom = layoutRect.bottom + g_HudUiMgrHudOriginY;
    HudLayout::ApplyViewportRect(&activeRect);

    if (active != 0) {
        HudUiRect outerRect;
        outerRect.top = activeRect.top + 1;
        outerRect.left = activeRect.left + 1;
        outerRect.right = activeRect.right - 1;
        outerRect.bottom = activeRect.bottom - 1;

        g_HudSensorTracker.SetBounds(
            &outerRect,
            &g_HudUiMgrSensorBlock.sensorViewportRect
        );
        g_HudUiMgr.SetChildFlags(0);
        SetChildFlags(0);
        zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);

        if (g_HudUiMgr.enabled != 0 && zVid::GetAccelerationOption() == ZVID_HW_MODE_SOFTWARE) {
            const int replicateMode = zOpt::GetReplicateMode();
            float nearClip = 0.0f;
            float farClip = 0.0f;
            zClass_Camera::gwCameraGetNearFarClip(
                g_MainCamera,
                &nearClip,
                &farClip
            );

            const float invNearClip = 1.0f / nearClip;
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrSensorBlock.sensorViewportRect,
                replicateMode,
                invNearClip
            );
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrShieldMessageWidget->screenRect,
                replicateMode,
                invNearClip
            );

            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrModeCounters[0].clipViewportRect,
                replicateMode,
                invNearClip
            );
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrModeCounters[1].clipViewportRect,
                replicateMode,
                invNearClip
            );
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrModeCounters[2].clipViewportRect,
                replicateMode,
                invNearClip
            );
            zRndr::SpanOcclusionSubmitOccluderRect(
                &g_HudUiMgrModeCounters[3].clipViewportRect,
                replicateMode,
                invNearClip
            );
        }
    }

    return 1;
}

namespace HudLayout {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.applyviewportrect
 * @recoil-artifact defines .text recoil:function:0x412db0: HudLayout::ApplyViewportRect.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: update display and render viewport sections from the active HUD rectangle.
 */
int __fastcall ApplyViewportRect(
    HudUiRect *activeRect
) {
    const int replicateMode = zOpt::GetReplicateMode();
    const int left = activeRect->left;
    const int top = activeRect->top;

    zOpt::DisplaySection_SetPosition(
        left,
        top
    );

    int renderX = left;
    int renderY = top;
    if (replicateMode != 0) {
        renderX = (left - (left >> 31)) >> 1;
        renderY = (top - (top >> 31)) >> 1;
    }

    zOpt::RenderSection_SetPosition(
        renderX,
        renderY
    );

    int width = activeRect->right - left;
    int height = activeRect->bottom - top;
    zOpt::DisplaySection_SetSize(
        width,
        height
    );

    const float viewportWidth = (float)(width);
    const float viewportHeight = (float)(height);

    if (replicateMode != 0) {
        width = (width - (width >> 31)) >> 1;
        height = (height - (height >> 31)) >> 1;
    }

    zOpt::RenderSection_SetSize(
        width,
        height
    );

    zClass_NodePartial *const camera = g_HudSensorTracker.cameraNode;
    if (camera != 0) {
        float fovX = 0.0f;
        float fovY = 0.0f;
        zClass_Camera::gwCameraGetFOV(
            camera,
            &fovX,
            &fovY
        );
        fovY = viewportHeight / viewportWidth * fovX;
        zClass_Camera::gwCameraSetFOV(
            camera,
            fovX,
            fovY
        );
    }

    zOpt_ViewRectSection *const renderSection = zOpt::GetRenderSection();
    HudUiMgr::OnViewportChanged(
        (const HudUiRect *)(zOpt::GetDisplaySection()),
        (const HudUiRect *)(renderSection)
    );
    return 1;
}

} // namespace HudLayout

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-hudlayouthw
 * @recoil-artifact defines .text recoil:function:0x412ea0: HudLayoutHW::HudLayoutHW.
 * Source file evidence: BN labels this function as a Battlesport hud.cpp helper.
 * Purpose: construct the hardware HUD layout and attach its image widgets in
 * the retail child-list order after automatic member construction.
 */
HudLayoutHW::HudLayoutHW()
    : widget1(0),
      widget2(0),
      widget3(0) {
    AddChild(&widget1);
    AddChild(&widget3);
    AddChild(&widget2);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-loadtypeiifromzarroot
 * @recoil-artifact defines .text recoil:function:0x412f70: HudLayoutHW::LoadTypeIIFromZarRoot.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: load the TYPEII HUD layout widgets and alternate image variants from ZRD data.
 */
int HudLayoutHW::LoadTypeIIFromZarRoot(
    zReader::Node *parentNode
) {
    zReader::Node *const typeIINode = zReader_GetNamedNode(
        parentNode,
        g_HudLayout_TypeIISectionName
    );
    if (typeIINode == 0) {
        return 1;
    }

    zReader::Node *const typeIIPayload = typeIINode->value.nodes;
    HudLayoutBase *const layout = (HudLayoutBase *)(this);

    HudUiLayoutNode::ReadRectOffsetAndSize(
        &typeIIPayload[1],
        &layout->layoutRect,
        0,
        0,
        0
    );
    layout->activeRect = layout->layoutRect;

    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[2],
        &widget1,
        0,
        0,
        0,
        0,
        0
    );
    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[3],
        &widget3,
        0,
        g_HudUiMgrHudOriginY,
        0,
        0,
        0
    );
    HudUiLayoutNode::ApplyImageWidget(
        &typeIIPayload[4],
        &widget2,
        0,
        g_HudUiMgrHudOriginY,
        0,
        0,
        0
    );

    zReader::Node *const imageNames = typeIIPayload[5].value.nodes;
    widget1ImageDefault = widget1.image;
    widget1Image320 = zImage::TexDir_FindOrCreateByPath(imageNames[1].value.str);
    widget1Image400 = zImage::TexDir_FindOrCreateByPath(imageNames[2].value.str);
    widget2ImageDefault = widget2.image;
    widget2Image320 = zImage::TexDir_FindOrCreateByPath(imageNames[3].value.str);
    widget2Image400 = zImage::TexDir_FindOrCreateByPath(imageNames[4].value.str);

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-releaseimages
 * @recoil-artifact defines .text recoil:function:0x413080: HudLayoutHW::ReleaseImages.
 * Purpose: release hardware HUD layout alternate images and clear their cached pointers.
 */
void HudLayoutHW::ReleaseImages() {
    zVid_Image::ReleaseIfNotDefault(widget1Image320);
    zVid_Image::ReleaseIfNotDefault(widget1Image400);
    zVid_Image::ReleaseIfNotDefault(widget2Image320);
    zVid_Image::ReleaseIfNotDefault(widget2Image400);

    widget2Image400 = 0;
    widget2Image320 = 0;
    widget1Image400 = 0;
    widget1Image320 = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-setactive
 * @recoil-artifact defines .text recoil:function:0x4130d0: HudLayoutHW::SetActive.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the hardware HUD viewport and connect or clear widget blit sources.
 */
int HudLayoutHW::SetActive(
    int active
) {
    if (zVid::GetAccelerationOption() == 0) {
        zRndr::SpanOcclusionResetFrame();
    }

    HudLayoutBase *const layout = (HudLayoutBase *)(this);
    layout->activeRect.right = zVideo::GetPrimarySurfaceWidth();
    layout->activeRect.bottom = layout->layoutRect.bottom + g_HudUiMgrHudOriginY;
    HudLayout::ApplyViewportRect(&layout->activeRect);

    if (active != 0) {
        layout->OnActivated();

        zVidImagePartial *const widget1Image = widget1.image;
        zVidImagePartial *const widget2Image = widget2.image;
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))
            ->SetBltSourceAndClipRect(
                widget1Image,
                0
            );
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetBltSourceAndClipRect(
            widget1Image,
            0
        );

        {
            for (int index = 1; index < 10; ++index) {
                HudUiMessage &message = g_HudUiMgrMessages[index];
                message.SetBltSourceAndClipRect(
                    widget2Image,
                    0
                );
                message.panel.SetBltSourceAndClipRect(
                    widget2Image,
                    0
                );
            }
        }

        g_HudUiMgrNanitePanel.HudUiElement::SetBltSourceAndClipRect(
            widget2Image,
            0
        );
        zClipAlt::SetSourceRect(&g_HudUiMgrSensorBlock.sensorPiVSrcRect);

        if (g_HudUiMgr.enabled != 0 && zVid::GetAccelerationOption() == 0) {
            HudUiRect occluderRect;
            occluderRect.left = g_HudUiMgrSensorBlock.sensorViewportRect.left;
            occluderRect.top = g_HudUiMgrSensorBlock.sensorViewportRect.top;
            occluderRect.right = g_HudUiMgrSensorBlock.sensorViewportRect.right;
            occluderRect.bottom = g_HudUiMgrHudRect.bottom;

            float nearClip = 0.0f;
            float farClip = 0.0f;
            zClass_Camera::gwCameraGetNearFarClip(
                g_MainCamera,
                &nearClip,
                &farClip
            );
            zRndr::SpanOcclusionSubmitOccluderRect(
                &occluderRect,
                zOpt::GetReplicateMode(),
                1.0f / nearClip
            );
        }
    } else {
        ((HudUiElement *)(g_HudUiMgrObjectiveCounterTextPanel))->SetBltSourceAndClipRect(
            0,
            0
        );
        ((HudUiElement *)(g_HudUiMgrTimerPanel))->SetBltSourceAndClipRect(
            0,
            0
        );
        g_HudUiMgrNanitePanel.HudUiElement::SetBltSourceAndClipRect(
            0,
            0
        );

        {
            for (int index = 0; index < 10; ++index) {
                HudUiMessage &message = g_HudUiMgrMessages[index];
                message.SetBltSourceAndClipRect(
                    0,
                    0
                );
                message.panel.SetBltSourceAndClipRect(
                    0,
                    0
                );
            }
        }

        const int clearState = zVideo::ExchangeClearScreenBufferEnabled(1);
        zVideo::CallClearPrimarySurfaceAndZBuffer(0);
        zVideo::ExchangeClearScreenBufferEnabled(clearState);
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-updateobjectivedirtyrect
 * @recoil-artifact defines .text recoil:function:0x4132b0: HudLayoutHW::UpdateObjectiveDirtyRect.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Rebuilds the objective dirty rectangle and refreshes the nanite panel after HUD layout changes.
 */
void HudLayoutHW::UpdateObjectiveDirtyRect() {
    zVidImagePartial *const image = g_HudUiMgrObjectiveWidget.image;
    const int width = image != 0 ? image->width : 0;

    const int centerX = g_HudUiMgrObjectiveWidget.GetCenterX();
    const int centerY = g_HudUiMgrObjectiveWidget.GetCenterY();

    const int height = image != 0 ? image->height : 0;
    HudUiRect dirtyRect;
    dirtyRect.left = centerX + width;
    dirtyRect.top = centerY;
    dirtyRect.right = g_HudUiMgrObjectiveWidgetRightX;
    dirtyRect.bottom = centerY + height;

    widget2.InvalidateRect(&dirtyRect);
    g_HudUiMgrNanitePanel.HudUiElement::Invalidate();
    g_HudUiMgrNanitePanel.HudUiTripletPanel::Draw();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-onactivated
 * @recoil-artifact defines .text recoil:function:0x413340: HudLayoutHW::OnActivated.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: activate hardware HUD widgets, image variants, and sensor bounds.
 */
void HudLayoutHW::OnActivated() {
    HudUi::SetInvalidateMode(zOpt::GetReplicateMode() == 0 ? 1 : 0);

    g_HudUiMgr.SetChildFlags(0x0e);
    SetChildFlags(0x0e);

    widget2.flags = (unsigned int)((unsigned char)(widget2.flags) & 0x10u);

    g_HudUiMgrObjectiveWidget.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveWidget.flags) & 0x10u);
    g_HudUiMgrObjectiveMeter.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveMeter.flags) & 0x10u);
    ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->flags =
        (unsigned int)((unsigned char)(((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))
                               ->flags) &
                       0x10u);
    g_HudUiMgrSensorOverlay.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrSensorOverlay.flags) & 0x10u);

    g_HudUiMgrStatsList->triplet->RebuildDisplay();

    {
        for (int index = 1; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            if (message.widget.image != 0) {
                message.widget.flags =
                    (unsigned int)((unsigned char)(message.widget.flags) & 0x10u);
            }
        }
    }

    HudLayoutBase *const layout = (HudLayoutBase *)(this);
    HudUiRect outerRect;
    outerRect.left = layout->activeRect.left + 1;
    outerRect.top = layout->activeRect.top + 1;
    outerRect.right = layout->activeRect.right - 1;
    outerRect.bottom = layout->activeRect.bottom - 1;

    HudUiRect *innerRect = 0;
    if (zOpt::GetReplicateMode() == 0) {
        innerRect = &g_HudUiMgrSensorBlock.sensorViewportRect;
    }
    g_HudSensorTracker.SetBounds(
        &outerRect,
        innerRect
    );

    zVidImagePartial *widget1Image = widget1ImageDefault;
    zVidImagePartial *widget2Image = widget2ImageDefault;
    if (layout->activeRect.right == 0x320) {
        widget1Image = widget1Image320;
        widget2Image = widget2Image320;
    } else if (layout->activeRect.right == 0x400) {
        widget1Image = widget1Image400;
        widget2Image = widget2Image400;
    }

    widget2.SetImageBorrowedAndInvalidate(widget2Image);
    widget1.SetImageBorrowedAndInvalidate(widget1Image);

    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        widget2.SetImageBorrowedAndInvalidate(widget2Image);
        widget1.SetImageBorrowedAndInvalidate(widget1Image);
        widget2.InvalidateRect(&g_HudUiMgrViewRect);
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-draw
 * @recoil-artifact defines .text recoil:function:0x4134e0: HudUiMessage::Draw.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Draws the weapon-message base widget and its embedded text panel.
 */
void HudUiMessage::Draw() {
    HudUiWidget::Draw();
    panel.Draw();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-updateall
 * @recoil-artifact defines .text recoil:function:0x413500: HudLayoutHW::UpdateAll.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: refresh hardware HUD replication blits before container child updates.
 */
void HudLayoutHW::UpdateAll(
    float deltaSeconds
) {
    if (g_HudUiMgr.enabled != 0 && zOpt::GetReplicateMode() != 0 && g_HudUiMgrObjectivePhase == 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectScaled),
            (zVidRect32 *)(&g_HudUiMgrSensorBlock.sensorRectRaw)
        );
    }

    HudUiContainer::UpdateAll(deltaSeconds);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-enable
 * @recoil-artifact defines .text recoil:function:0x413540: HudLayoutHW::Enable.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: enable hardware HUD layout children and mark dependent widgets visible.
 */
void HudLayoutHW::Enable() {
    g_HudUiMgr.SetChildFlags(0x0e);
    SetChildFlags(0x0e);

    widget2.flags = (unsigned int)((unsigned char)(widget2.flags) & 0x10u);

    g_HudUiMgrObjectiveWidget.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveWidget.flags) & 0x10u);
    g_HudUiMgrObjectiveMeter.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrObjectiveMeter.flags) & 0x10u);
    ((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))->flags =
        (unsigned int)((unsigned char)(((HudUiElement *)(g_HudUiMgrObjectiveLabelTextPanel))
                               ->flags) &
                       0x10u);
    g_HudUiMgrSensorOverlay.flags =
        (unsigned int)((unsigned char)(g_HudUiMgrSensorOverlay.flags) & 0x10u);

    {
        for (int index = 1; index < 10; ++index) {
            HudUiMessage &message = g_HudUiMgrMessages[index];
            if (message.widget.image != 0) {
                message.widget.flags =
                    (unsigned int)((unsigned char)(message.widget.flags) & 0x10u);
            }
        }
    }

    SetEnabled(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.hudlayouthw-disable
 * @recoil-artifact defines .text recoil:function:0x4135f0: HudLayoutHW::Disable.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: disable the hardware HUD layout container.
 */
void HudLayoutHW::Disable() {
    SetEnabled(0);
}

namespace zOpt {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.togglehudtypeforcurrenthwmode
 * @recoil-artifact defines .text recoil:function:0x413600: zOpt::ToggleHudTypeForCurrentHwMode.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Toggle the HUD type between standard and perspective for the current hardware mode.
 */
int ToggleHudTypeForCurrentHwMode() {
    const int currentHudType = GetHudTypeForCurrentHwMode();
    if (currentHudType == ZOPT_HUD_TYPE_STANDARD) {
        return GetHudTypeForCurrentHwMode();
    }
    if (currentHudType == ZOPT_HUD_TYPE_PERSPECTIVE) {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_STANDARD);
    }
    return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_PERSPECTIVE);
}

} // namespace zOpt

namespace HudUiMgr {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.triggercurrentlayoutonactivated
 * @recoil-artifact defines .text recoil:function:0x413630: HudUiMgr::TriggerCurrentLayoutOnActivated.
 * Purpose: Re-run the active HUD layout activation hook when a layout is present.
 */
void __cdecl TriggerCurrentLayoutOnActivated() {
    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->OnActivated();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.togglehud
 * @recoil-artifact defines .text recoil:function:0x413640: HudUiMgr::ToggleHud.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::ToggleHud.
 */
int ToggleHud() {
    if (g_HudUiMgr.enabled != 0) {
        DisableHud();
    } else {
        EnableHud();
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.switchactivedialog
 * @recoil-artifact defines .text recoil:function:0x413660: HudUiMgr::SwitchActiveDialog.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::SwitchActiveDialog.
 */
void __fastcall SwitchActiveDialog(
    HudLayoutBase *newDialog
) {
    const int enabled = g_HudUiMgr.enabled;
    if (enabled != 0) {
        DisableHud();
    } else {
        g_HudUiMgrLayoutDelayFrames = 2;
    }

    if (g_HudUiMgrCurrentLayout != 0) {
        g_HudUiMgrCurrentLayout->SetActive(0);
    }

    newDialog->SetActive(1);
    g_HudUiMgrCurrentLayout = newDialog;

    if (enabled != 0) {
        EnableHud();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.applyhudmodeswitch
 * @recoil-artifact defines .text recoil:function:0x4136b0: HudUiMgr::ApplyHudModeSwitch.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiMgr::ApplyHudModeSwitch.
 */
int __fastcall ApplyHudModeSwitch(
    int hudType
) {
    const int currentType = zOpt::GetHudTypeForCurrentHwMode();
    if (g_HudUiMgrHudLayoutsInitialized != 0) {
        if (hudType == 1) {
            SwitchActiveDialog((HudLayoutBase *)(&g_HudLayoutSW));
        } else if (hudType == 2) {
            SwitchActiveDialog((HudLayoutBase *)(&g_HudLayoutHW));
        }
    }

    return currentType;
}

} // namespace HudUiMgr
namespace HudUiSensorWindow {
CWnd *StaticInit();
int RegisterAtExit();
void __cdecl AtExitDestructor();

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x4136f0: HudUiSensorWindow::StaticInitAndRegisterAtExit.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: construct the global HUD sensor CWnd and register its static
 * destructor during CRT startup.
 */
void __cdecl StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.staticinit
 * @recoil-artifact defines .text recoil:function:0x413700: HudUiSensorWindow::StaticInit.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: default-construct the global HUD sensor CWnd in its static storage.
 */
CWnd *StaticInit() {
    return new (&g_HudUiSensorWindow) CWnd;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.registeratexit
 * @recoil-artifact defines .text recoil:function:0x413710: HudUiSensorWindow::RegisterAtExit.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: register the global HUD sensor CWnd destructor with the CRT
 * at-exit list.
 */
int RegisterAtExit() {
    return atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x413720: HudUiSensorWindow::AtExitDestructor.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: destroy the global HUD sensor CWnd during CRT shutdown.
 */
void __cdecl AtExitDestructor() {
    ((CWnd *)&g_HudUiSensorWindow)->~CWnd();
}
} // namespace HudUiSensorWindow

namespace HudUiMgr {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.destroysensorwindow
 * @recoil-artifact defines .text recoil:function:0x413730: HudUiMgr::DestroySensorWindow.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::DestroySensorWindow.
 */
void DestroySensorWindow() {
    zFMV_Playback *playback = g_HudUiSensorWindowPlayback;
    if (playback == 0) {
        return;
    }

    playback->StopAndClose();

    playback = g_HudUiSensorWindowPlayback;
    if (playback != 0) {
        playback->~zFMV_Playback();
        ::operator delete(playback);
    }

    g_HudUiSensorWindowPlayback = 0;
    ((CWnd *)&g_HudUiSensorWindow)->CWnd::DestroyWindow();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setfloattimervisible
 * @recoil-artifact defines .text recoil:function:0x413770: HudUiMgr::SetFloatTimerVisible.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiMgr::SetFloatTimerVisible.
 */
void __fastcall SetFloatTimerVisible(
    int visible
) {
    if (visible != 0) {
        g_HudUiMgrTimerPanelFloat->SetVisible(1);
    } else {
        g_HudUiMgrTimerPanelFloat->SetVisible(0);
    }

    if (visible == 0) {
        TriggerCurrentLayoutOnActivated();
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.setauxoverlayvisible
 * @recoil-artifact defines .text recoil:function:0x4137a0: HudUiMgr::SetAuxOverlayVisible.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD state change handled by HudUiMgr::SetAuxOverlayVisible.
 */
void __fastcall SetAuxOverlayVisible(
    int visible
) {
    if (visible != 0) {
        g_HudUiMgrStringMenu->SetEnabled(1);
    } else {
        g_HudUiMgrStringMenu->SetEnabled(0);
    }
}
} // namespace HudUiMgr

namespace HudUiAuxOverlay {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.cleartextlines
 * @recoil-artifact defines .text recoil:function:0x4137c0: HudUiAuxOverlay::ClearTextLines.
 * Purpose: clear and hide every sensor overlay text line.
 */
void ClearTextLines() {
    {
        for (int index = 0; index < 23; ++index) {
            UpdateTextLine(
                2,
                index,
                ""
            );
            UpdateTextLine(
                0,
                index,
                0
            );
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.updatetextline
 * @recoil-artifact defines .text recoil:function:0x4137f0: HudUiAuxOverlay::ApplyTextLineOp.
 * Purpose: apply one sensor overlay text-line operation to a string-menu item.
 */
void __fastcall UpdateTextLine(
    int op,
    int index,
    const char *format
) {
    HudUiPanel *const panel = (HudUiPanel *)(&g_HudUiMgrStringMenu->items[index]);

    if (op == 1) {
        panel->SetTextFmt(format);
        panel->SetVisible(1);
        return;
    }

    if (op == 0) {
        panel->SetVisible(0);
        return;
    }

    if (op == 2) {
        if (*format != '\0') {
            panel->SetTextFmt(format);
            panel->SetVisible(1);
        } else {
            panel->SetVisible(0);
        }
    }
}
} // namespace HudUiAuxOverlay

namespace HudUi {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.showtopmessageline
 * @recoil-artifact defines .text recoil:function:0x4138d0: HudUi::ShowTopMessageLine.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: show a top HUD message when the top-message stack is enabled.
 */
void __fastcall ShowTopMessageLine(
    const char *message,
    float duration
) {
    HudUiTextStack4 *const topStack = g_HudUiTopMessageStack;
    if (topStack->enabled != 0) {
        topStack->PushLine(
            message,
            duration
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.showchatline
 * @recoil-artifact defines .text recoil:function:0x4138f0: HudUi::ShowChatLine.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: show a chat HUD message when the chat stack is enabled.
 */
void __fastcall ShowChatLine(
    const char *message,
    float duration
) {
    HudUiTextStack4 *const chatStack = g_HudUiChatMessageStack;
    if (chatStack->enabled != 0) {
        chatStack->PushLine(
            message,
            duration
        );
    }
}
} // namespace HudUi

namespace HudUiMgr {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.enabletopandchatstacks
 * @recoil-artifact defines .text recoil:function:0x413910: HudUiMgr::EnableTopAndChatStacks.
 * Purpose: clear and enable the global top-message and chat text stacks.
 */
void EnableTopAndChatStacks() {
    g_HudUiTopMessageStack->Clear();
    g_HudUiTopMessageStack->SetEnabled(1);
    g_HudUiChatMessageStack->Clear();
    g_HudUiChatMessageStack->SetEnabled(1);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.disabletopandchatstacks
 * @recoil-artifact defines .text recoil:function:0x413950: HudUiMgr::DisableTopAndChatStacks.
 * Purpose: clear and disable the global top-message and chat text stacks.
 */
void DisableTopAndChatStacks() {
    g_HudUiTopMessageStack->Clear();
    g_HudUiTopMessageStack->SetEnabled(0);
    g_HudUiChatMessageStack->Clear();
    g_HudUiChatMessageStack->SetEnabled(0);
}
} // namespace HudUiMgr

namespace HudUiLayoutNode {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.applytextlabel
 * @recoil-artifact defines .text recoil:function:0x413990: HudUiLayoutNode::ApplyTextLabel.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiLayoutNode::ApplyTextLabel.
 */
int __fastcall ApplyTextLabel(
    zReader::Node *layoutNode,
    HudUiPanel *target,
    int baseX,
    int baseY,
    const int *offsetXY
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    const char *const text = payload[1].value.str;
    int x = payload[2].value.i32 + baseX;
    int y = payload[3].value.i32 + baseY;
    if (offsetXY != 0) {
        x += offsetXY[0];
        y += offsetXY[1];
    }

    target->SetPos(
        x,
        y
    );
    if (text != 0) {
        target->SetTextFmt(text);
    } else {
        target->SetTextFmt("");
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.readrectoffsetandsize
 * @recoil-artifact defines .text recoil:function:0x413a10: HudUiLayoutNode::ReadRectOffsetAndSize.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: read recovered HUD ZRD/layout data for HudUiLayoutNode::ReadRectOffsetAndSize.
 */
int __fastcall ReadRectOffsetAndSize(
    zReader::Node *node,
    HudUiRect *outRect,
    const int *offsetXY,
    int *outWidth,
    int *outHeight
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    outRect->left = arrayBase[1].value.i32;
    outRect->top = arrayBase[2].value.i32;
    outRect->right = arrayBase[3].value.i32;
    outRect->bottom = arrayBase[4].value.i32;

    if (offsetXY != 0) {
        outRect->left += offsetXY[0];
        outRect->top += offsetXY[1];
        outRect->right += offsetXY[0];
        outRect->bottom += offsetXY[1];
    }

    if (outWidth != 0) {
        *outWidth = outRect->right - outRect->left;
    }

    if (outHeight != 0) {
        *outHeight = outRect->bottom - outRect->top;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.readrect
 * @recoil-artifact defines .text recoil:function:0x413aa0: HudUiLayoutNode::ReadRect.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: read recovered HUD ZRD/layout data for HudUiLayoutNode::ReadRect.
 */
int __fastcall ReadRect(
    zReader::Node *node,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    outRect->left = arrayBase[1].value.i32;
    outRect->right = arrayBase[2].value.i32;
    outRect->top = arrayBase[3].value.i32;
    outRect->bottom = arrayBase[4].value.i32;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.readint3
 * @recoil-artifact defines .text recoil:function:0x413ad0: HudUiLayoutNode::ReadInt3.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: read recovered HUD ZRD/layout data for HudUiLayoutNode::ReadInt3.
 */
int __fastcall ReadInt3(
    zReader::Node *node,
    int *out0,
    int *out1,
    int *out2
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    if (out0 != 0) {
        *out0 = arrayBase[1].value.i32;
    }

    if (out1 != 0) {
        *out1 = arrayBase[2].value.i32;
    }

    if (out2 != 0) {
        *out2 = arrayBase[3].value.i32;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.applycornertextquad
 * @recoil-artifact defines .text recoil:function:0x413b10: HudUiLayoutNode::ApplyCornerTextQuad.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiLayoutNode::ApplyCornerTextQuad.
 */
int __fastcall ApplyCornerTextQuad(
    zReader::Node *node,
    HudUiBar *target,
    const int *offsetXY,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    int left = arrayBase[1].value.i32;
    int top = arrayBase[2].value.i32;
    int right = arrayBase[3].value.i32;
    int bottom = arrayBase[4].value.i32;

    if (offsetXY != 0) {
        left += offsetXY[0];
        top += offsetXY[1];
        right += offsetXY[0];
        bottom += offsetXY[1];
    }

    const float leftF = (float)(left);
    const float topF = (float)(top);
    const float rightF = (float)(right);
    const float bottomF = (float)(bottom);
    target->SetPointXY(
        0,
        leftF,
        topF
    );
    target->SetPointXY(
        1,
        leftF,
        bottomF
    );
    target->SetPointXY(
        2,
        rightF,
        bottomF
    );
    target->SetPointXY(
        3,
        rightF,
        topF
    );

    if (outRect != 0) {
        outRect->left = left;
        outRect->top = top;
        outRect->right = right;
        outRect->bottom = bottom;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.applymeterquad
 * @recoil-artifact defines .text recoil:function:0x413c10: HudUiLayoutNode::ApplyMeterQuad.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiLayoutNode::ApplyMeterQuad.
 */
int __fastcall ApplyMeterQuad(
    zReader::Node *node,
    HudUiBar *target,
    int xBase,
    int yBase,
    const int *offsetXY,
    HudUiRect *outRect
) {
    if (node->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const arrayBase = node->value.nodes;
    int left = arrayBase[1].value.i32;
    const int top = arrayBase[2].value.i32;
    int right = arrayBase[3].value.i32 + 1;
    const int bottom = arrayBase[4].value.i32 + 1;

    if (outRect != 0) {
        outRect->left = left;
        outRect->top = top;
        outRect->right = right;
        outRect->bottom = bottom;
    }

    right += xBase;
    int topY = top + yBase;
    int bottomY = bottom + yBase;

    if (offsetXY != 0) {
        left += offsetXY[0];
        topY += offsetXY[1];
        right += offsetXY[0];
        bottomY += offsetXY[1];
    }

    const int width = right - left;
    const int height = bottomY - topY;
    HudUiBar *const bar = (HudUiBar *)(target);
    bar->SetPointXY(
        0,
        (float)(left),
        (float)(topY)
    );
    bar->SetPointXY(
        1,
        (float)(left),
        (float)(height + topY)
    );
    bar->SetPointXY(
        2,
        (float)(width + left + 1),
        (float)(height + topY)
    );
    bar->SetPointXY(
        3,
        (float)(width + left + 1),
        (float)(topY)
    );

    target->fillPixelsMax = height;
    target->meterFlags = (unsigned int)(width);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.applyimagewidget
 * @recoil-artifact defines .text recoil:function:0x413d30: HudUiLayoutNode::ApplyImageWidget.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: apply the recovered HUD layout or option state handled by HudUiLayoutNode::ApplyImageWidget.
 */
zVidImagePartial *__fastcall ApplyImageWidget(
    zReader::Node *layoutNode,
    HudUiWidget *widget,
    int baseX,
    int baseY,
    const int *anchorOrNull,
    zVidImagePartial *preloadedImageOrNull,
    HudUiRect *outRectOrNull
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    const char *const imagePath = payload[1].value.str;
    int x = payload[2].value.i32 + baseX;
    int y = payload[3].value.i32 + baseY;

    if (anchorOrNull != 0) {
        x += anchorOrNull[0];
        y += anchorOrNull[1];
    }

    unsigned short visibleState = 0;
    int centerImage = 0;
    if (payload[0].value.i32 == 6) {
        visibleState = (unsigned short)(strcmp(
            payload[4].value.str,
            "TRUE"
        ) == 0 ? 1 : 0);
        centerImage = strcmp(
            payload[5].value.str,
            "TRUE"
        ) == 0 ? 1 : 0;
    }

    zVidImagePartial *image = preloadedImageOrNull;
    if (image != 0) {
        widget->SetImageBorrowedAndInvalidate(image);
    } else {
        image = widget->SetImageByPathOwned(imagePath);
    }

    if (image == 0) {
        return 0;
    }

    if (centerImage != 0) {
        x -= (int)(image->width) / 2;
        y -= (int)(image->height) / 2;
    }

    widget->SetPos(
        x,
        y
    );
    widget->imageStateWord = (widget->imageStateWord & 0xffff0000u) | visibleState;
    widget->Invalidate();

    if (outRectOrNull != 0) {
        outRectOrNull->left = x;
        outRectOrNull->top = y;
        outRectOrNull->right = x + image->width;
        outRectOrNull->bottom = y + image->height;
    }

    return image;
}
} // namespace HudUiLayoutNode

/**
 * Original function; shared retail body 0x413eb0.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: provide the shared empty widget shutdown hook used by the HUD
 * teardown paths.
 */
void HudUiWidget::Shutdown() {}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-loadweaponlayoutfromnode
 * @recoil-artifact defines .text recoil:function:0x413ec0: HudUiMessage::LoadWeaponLayoutFromNode.
 * Purpose: Load weapon-message images/layout and register the message owner and side widget with the HUD manager.
 */
int HudUiMessage::LoadWeaponLayoutFromNode(
    zReader::Node *layoutNode,
    const HudUiPanelFontParams *fontParams
) {
    if (layoutNode->type != zReader::ZRDR_NODE_ARRAY) {
        return 0;
    }

    zReader::Node *const payload = layoutNode->value.nodes;
    variantImages[0] = zImage::TexDir_FindOrCreateByPath(payload[1].value.str);
    variantImages[1] = zImage::TexDir_FindOrCreateByPath(payload[2].value.str);
    variantImages[2] = zImage::TexDir_FindOrCreateByPath(payload[3].value.str);
    variantImages[3] = zImage::TexDir_FindOrCreateByPath(payload[4].value.str);
    variantImages[4] = zImage::TexDir_FindOrCreateByPath(payload[5].value.str);
    sideImageSwaps[0] = zImage::TexDir_FindOrCreateByPath(payload[6].value.str);
    sideImageSwaps[1] = zImage::TexDir_FindOrCreateByPath(payload[7].value.str);
    HudUiPanelFull *const messagePanel = &panel;
    messagePanel->layoutX = payload[8].value.i32;
    messagePanel->layoutY = payload[9].value.i32;

    RebuildWeaponLayout();

    imageStateWord = (imageStateWord & 0xffff0000u) | 1u;
    Invalidate();

    messagePanel->centerText = 1;
    messagePanel->textColor0 = 0x0020bf40;
    messagePanel->textColor1 = 0x0020bf40;
    messagePanel->textDirty = 1;
    messagePanel->shadowOffsetX = -1;
    messagePanel->shadowOffsetY = -1;
    messagePanel->shadowEnabled = 1;

    messagePanel->SetFont(
        fontParams->faceName,
        fontParams->height,
        fontParams->weight,
        fontParams->width,
        0,
        0,
        2
    );
    messagePanel->SetTextFmt(g_HudUiBlankSpaces3);

    g_HudUiMgr.AddChild(this);
    g_HudUiMgr.AddChild(&widget);
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-releaseimages
 * @recoil-artifact defines .text recoil:function:0x413ff0: HudUiMessage::ReleaseImages.
 * Purpose: Releases all borrowed weapon-message variant and side-image swap references and clears their storage.
 */
void HudUiMessage::ReleaseImages() {
    zVid_Image::ReleaseIfNotDefault(variantImages[0]);
    zVid_Image::ReleaseIfNotDefault(variantImages[1]);
    zVid_Image::ReleaseIfNotDefault(variantImages[2]);
    zVid_Image::ReleaseIfNotDefault(variantImages[3]);
    zVid_Image::ReleaseIfNotDefault(variantImages[4]);
    zVid_Image::ReleaseIfNotDefault(sideImageSwaps[0]);
    zVid_Image::ReleaseIfNotDefault(sideImageSwaps[1]);

    sideImageSwaps[1] = 0;
    sideImageSwaps[0] = 0;
    variantImages[4] = 0;
    variantImages[3] = 0;
    variantImages[2] = 0;
    variantImages[1] = 0;
    variantImages[0] = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimessage-rebuildweaponlayout
 * @recoil-artifact defines .text recoil:function:0x414070: HudUiMessage::RebuildWeaponLayout.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Rebuilds the message base, text panel, and side widget geometry from the current layout anchor.
 */
void HudUiMessage::RebuildWeaponLayout() {
    HudUiWidget *const layoutWidget2 = &g_HudLayoutHW.widget2;
    const int anchorX = layoutWidget2->GetCenterX();
    const int anchorY = layoutWidget2->GetCenterY();

    const int clipLeft = panel.layoutX + (g_HudUiMgrHudOriginX / 2);
    zVidImagePartial *const baseImage = variantImages[0];
    HudUiRect widgetClipRect;
    widgetClipRect.left = clipLeft;
    widgetClipRect.top = panel.layoutY;
    widgetClipRect.right = clipLeft + baseImage->width;
    widgetClipRect.bottom = panel.layoutY + baseImage->height;

    SetPos(
        clipLeft + anchorX,
        panel.layoutY + anchorY
    );
    SetBltSourceAndClipRect(
        0,
        &widgetClipRect
    );

    HudUiRect panelClipRect;
    panelClipRect.left = clipLeft + 3;
    panelClipRect.top = widgetClipRect.bottom;
    panelClipRect.right = widgetClipRect.right - 2;
    panelClipRect.bottom = widgetClipRect.bottom + 12;

    const int textX =
        panelClipRect.left + ((panelClipRect.right - panelClipRect.left) / 2) + anchorX;
    panel.SetPos(
        textX,
        widgetClipRect.bottom + anchorY
    );
    panel.SetBltSourceAndClipRect(
        0,
        &panelClipRect
    );

    zVidImagePartial *const sideImage = sideImageSwaps[0];
    widget.SetPos(
        anchorX - sideImage->width + widgetClipRect.right - 1,
        anchorY - sideImage->height + widgetClipRect.bottom - 1
    );
}

namespace HudUiLoadingCheckpoint {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.advanceandlog
 * @recoil-artifact defines .text recoil:function:0x414180: HudUiLoadingCheckpoint::AdvanceAndLog.
 * Purpose: advance the embedded HudUiMgr loading checkpoint table, report
 * overflow, optionally log the supplied message, and update briefing progress.
 */
void __fastcall AdvanceAndLog(
    const char *messageOrNull
) {
    const unsigned int currentIndex = g_HudUiLoadingCheckpointCurrentIndex;
    const unsigned int maxIndex = g_HudUiLoadingCheckpointMaxIndex;
    if (currentIndex > maxIndex) {
        zError::ReportOld(
            0x800,
            "D:\\Proj\\Battlesport\\hud.cpp",
            0x1184,
            g_Hud_CheckpointOverflowMsg
        );
    } else {
        g_HudUiLoadingCheckpointCurrentProgress = g_HudUiLoadingCheckpointProgress[currentIndex];
        const unsigned int nextIndex = currentIndex + 1;
        g_HudUiLoadingCheckpointCurrentIndex = nextIndex;
        if (nextIndex > maxIndex) {
            g_HudUiLoadingCheckpointCurrentIndex = maxIndex;
        }
    }

    if (messageOrNull != 0) {
        puts(messageOrNull);
        fflush(stdout);
    }

    zGame::ReturnOnlyStub();
    Briefing::SetProgressAndSleep(g_HudUiLoadingCheckpointCurrentProgress);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.inittable
 * @recoil-artifact defines .text recoil:function:0x414210: HudUiLoadingCheckpoint::InitTable.
 * Purpose: seed the embedded HudUiMgr loading checkpoint table and derive
 * normalized briefing progress from the retail checkpoint second values.
 */
void InitTable() {
    static const float kRawProgress[] = {
        0.00100000005f,
        0.136999995f,
        0.237000003f,
        0.340000004f,
        0.899999976f,
        9.30000019f,
        12.3999996f,
        13.3999996f,
        20.0f,
        26.0f,
        26.2999992f,
        28.7000008f,
        31.5f,
        34.0f,
        36.2000008f,
        36.4000015f,
        53.2999992f,
        53.5999985f,
        53.7000008f,
    };

    g_HudUiLoadingCheckpointMaxIndex = 18;
    g_HudUiLoadingCheckpointCurrentIndex = 0;
    {
        for (unsigned int index = 0; index <= g_HudUiLoadingCheckpointMaxIndex; ++index) {
            g_HudUiLoadingCheckpointRawProgress[index] = kRawProgress[index];
            g_HudUiLoadingCheckpointProgress[index] =
                g_HudUiLoadingCheckpointRawProgress[index] * g_HudUiLoadingCheckpointProgressScale;
        }
    }
}
} // namespace HudUiLoadingCheckpoint

extern "C" char g_Hud_TripleStringFmt[9];

namespace HudUiListMenuEntry {
int __fastcall CompareSortKey(
    const HudUiScoreboardEntry *entryA,
    const HudUiScoreboardEntry *entryB
);
}

namespace {
const int kGameNetChatComposeTextCapacity = 0x20;
const int kGameNetChatComposeShiftModifierMask = 0x400;
const int kGameNetChatComposeDigitFirstDik = 0x02;
const int kGameNetChatComposeDigitLastDik = 0x0e;
const int kGameNetChatComposeLetterRowFirstDik = 0x10;
const int kGameNetChatComposeLetterRowLastDik = 0x2b;
const int kGameNetChatComposeHomeRowFirstDik = 0x1e;
const int kGameNetChatComposeHomeRowLastDik = 0x28;
const int kGameNetChatComposeBottomRowFirstDik = 0x2c;
const int kGameNetChatComposeBottomRowLastDik = 0x35;
const int kGameNetChatComposeSpaceDik = 0x39;

/**
 * Original helper evidence: no standalone retail function; caller 0x4143d0
 * repeats this unregister/register pair across the chat-compose key ranges and
 * the standalone space-bar binding.
 * Purpose: Register one chat-compose keyboard callback binding.
 */
inline void HudRuntimeRegisterChatComposeKey(
    int comboIdx
) {
    zInput::Keyboard_UnregisterKeyCallback(comboIdx);
    zInput::Keyboard_RegisterKeyCallback(
        comboIdx,
        (void *)(&GameNet::ChatComposeKeyCallback),
        ""
    );
}

/**
 * Original helper evidence: no standalone retail function; caller 0x4143d0
 * repeats contiguous chat-compose key registration for unmodified and modified
 * DIK ranges.
 * Purpose: Register a contiguous range of chat-compose keyboard bindings.
 */
inline void HudRuntimeRegisterChatComposeKeyRange(
    int firstComboIdx,
    int lastComboIdx
) {
    for (int comboIdx = firstComboIdx; comboIdx <= lastComboIdx; ++comboIdx) {
        HudRuntimeRegisterChatComposeKey(comboIdx);
        HudRuntimeRegisterChatComposeKey(comboIdx | kGameNetChatComposeShiftModifierMask);
    }
}

/**
 * Original helper evidence: no standalone retail function; observed callers
 * 0x414710, 0x414930, and 0x414980 in the hud.cpp list-menu layer.
 * Purpose: expose the recovered comparator as a boolean ordering predicate for
 * local sort helpers.
 */
inline bool HudRuntimeListMenuEntryComesBefore(
    const HudUiScoreboardEntry &lhs,
    const HudUiScoreboardEntry &rhs
) {
    return HudUiListMenuEntry::CompareSortKey(
        &lhs,
        &rhs
    ) != 0;
}

/**
 * Original helper evidence: no standalone retail function; observed caller
 * 0x414710 in the hud.cpp list-menu layer.
 * Purpose: select the median scoreboard entry among first, middle, and last
 * candidates for quicksort partitioning.
 */
inline HudUiScoreboardEntry *HudRuntimeListMenuMedianOfThree(
    HudUiScoreboardEntry *first,
    HudUiScoreboardEntry *middle,
    HudUiScoreboardEntry *last
) {
    if (HudRuntimeListMenuEntryComesBefore(
        *first,
        *middle
    )) {
        if (HudRuntimeListMenuEntryComesBefore(
            *middle,
            *last
        )) {
            return middle;
        }

        return HudRuntimeListMenuEntryComesBefore(
            *first,
            *last
        ) ? last : first;
    }

    if (HudRuntimeListMenuEntryComesBefore(
        *first,
        *last
    )) {
        return first;
    }

    return HudRuntimeListMenuEntryComesBefore(
        *middle,
        *last
    ) ? last : middle;
}
} // namespace

namespace HudUiMgrSensor {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.getfxrect
 * @recoil-artifact defines .text recoil:function:0x414300: HudUiMgrSensor::GetFxRect.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return the recovered HUD value exposed by HudUiMgrSensor::GetFxRect.
 */
void __fastcall GetFxRect(
    HudUiRect *outRect
) {
    *outRect = g_HudUiMgrSensorFxRect;
}
} // namespace HudUiMgrSensor

namespace GameNet {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.showplayerkillmessage
 * @recoil-artifact defines .text recoil:function:0x414330: GameNet::ShowPlayerKillMessage
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Format and display a multiplayer kill-feed message.
 */
void __fastcall ShowPlayerKillMessage(
    GameNetPlayerRow *victimRow,
    OptCatalogEntryDef *killEntry,
    GameNetPlayerRow *killerRow
) {
    const char *killVerb = "";
    if (killEntry == 0) {
        killVerb = zLoc::GetMessageString(0x253);
    } else if (killEntry->killVerbString != 0) {
        killVerb = killEntry->killVerbString;
    }

    char message[0x50];
    sprintf(
        message,
        g_Hud_TripleStringFmt,
        victimRow->displayName,
        killVerb,
        killerRow->displayName
    );
    HudUi::ShowTopMessageLine(
        message,
        2.0f
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.refreshplayerlistmenu
 * @recoil-artifact defines .text recoil:function:0x414390: GameNet::RefreshPlayerListMenu
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Forward a player row to the HUD stats list triplet for scoreboard
 * entry insertion.
 */
void __fastcall RefreshPlayerListMenu(
    GameNetPlayerRow *playerRow
) {
    g_HudUiMgrStatsList->triplet->AddEntry(playerRow);
}
} // namespace GameNet

namespace HudUiMgr {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.islocalplayerfirstinstatslist
 * @recoil-artifact defines .text recoil:function:0x4143a0: HudUiMgr::IsLocalPlayerFirstInStatsList.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUiMgr::IsLocalPlayerFirstInStatsList.
 */
int IsLocalPlayerFirstInStatsList() {
    return g_HudUiMgrStatsList->triplet->IsLocalPlayerFirstEntry();
}
} // namespace HudUiMgr

namespace HudUi {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.refreshscoreboardentryrow
 * @recoil-artifact defines .text recoil:function:0x4143b0: HudUi::RefreshScoreboardEntryRow.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: preserve the recovered HUD behavior for HudUi::RefreshScoreboardEntryRow.
 */
void __fastcall RefreshScoreboardEntryRow(
    GameNetPlayerRow *entryData
) {
    g_HudUiMgrStatsList->triplet->UpdateEntryData(entryData);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.removescoreboardentryrow
 * @recoil-artifact defines .text recoil:function:0x4143c0: HudUi::RemoveScoreboardEntryRow.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: forward a multiplayer row removal to the active scoreboard triplet.
 */
void __fastcall RemoveScoreboardEntryRow(
    GameNetPlayerRow *entryKey
) {
    g_HudUiMgrStatsList->triplet->RemoveEntry(entryKey);
}
} // namespace HudUi

namespace GameNet {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.beginchatcompose
 * @recoil-artifact defines .text recoil:function:0x4143d0: GameNet::BeginChatCompose
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Open chat-compose mode and bind text-entry keys.
 */
void __cdecl BeginChatCompose() {
    if (zOpt::GetNetworkEnabled() == 0) {
        return;
    }

    HudUiMgrObjective::Show(
        0,
        g_HudUiMessage_NodeName,
        "",
        0.0f
    );
    g_HudUiMgrObjectiveChatComposeActive = 1;
    g_HudUiMgrObjectiveChatComposeTextInput.AllocTextBuffer(kGameNetChatComposeTextCapacity);
    g_HudUiMgrObjectiveChatComposeTextInput.SetContents("");
    zInput::BindMapContext_Push(0);

    HudRuntimeRegisterChatComposeKeyRange(
        kGameNetChatComposeDigitFirstDik,
        kGameNetChatComposeDigitLastDik
    );
    HudRuntimeRegisterChatComposeKeyRange(
        kGameNetChatComposeLetterRowFirstDik,
        kGameNetChatComposeLetterRowLastDik
    );
    HudRuntimeRegisterChatComposeKeyRange(
        kGameNetChatComposeHomeRowFirstDik,
        kGameNetChatComposeHomeRowLastDik
    );
    HudRuntimeRegisterChatComposeKeyRange(
        kGameNetChatComposeBottomRowFirstDik,
        kGameNetChatComposeBottomRowLastDik
    );
    HudRuntimeRegisterChatComposeKey(kGameNetChatComposeSpaceDik);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.chatcomposekeycallback
 * @recoil-artifact defines .text recoil:function:0x414550: GameNet::ChatComposeKeyCallback
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Append a translated key to active chat-compose text and mirror the
 * buffer into the objective description panel.
 */
void __fastcall ChatComposeKeyCallback(
    int dikCodeWithMods
) {
    const int key = zInput::Keyboard_TranslateDikToAscii(dikCodeWithMods);
    if (key == 0) {
        return;
    }

    g_HudUiMgrObjectiveChatComposeTextInput.DispatchKeyAction(key);

    g_HudUiMgrObjectiveDescTextPanel->SetTextFmt(
        g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer()
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.endchatcomposeandsend
 * @recoil-artifact defines .text recoil:function:0x414590: GameNet::EndChatComposeAndSend
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Close chat compose, show the local chat line, and send packet 0x0b.
 */
void __cdecl EndChatComposeAndSend() {
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    GameNetPlayerRow *const playerRow = saveState->netPlayerRow;
    char chatLine[0x51];
    chatLine[0x50] = '\0';

    g_HudUiMgrObjectiveChatComposeActive = 0;
    zInput::BindMapContext_Pop();
    HudUiMgrObjective::Begin();

    if (strlen(g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer()) == 0) {
        return;
    }

    strncpy(
        chatLine,
        playerRow->displayName,
        0x50
    );
    strncat(
        chatLine,
        g_HudUiMessage_SeparatorColon,
        0x50 - strlen(chatLine)
    );
    strncat(
        chatLine,
        g_HudUiMgrObjectiveChatComposeTextInput.GetBuffer(),
        0x50 - strlen(chatLine)
    );
    HudUi::ShowChatLine(
        chatLine,
        5.0f
    );
    SendPkt0B_ChatMessage(chatLine);
}

/**
 * Provider boundary: dispatch thunk at retail 0x414660.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Forward the chat-compose dispatch callback to EndChatComposeAndSend.
 */
void __cdecl EndChatComposeAndSendThunk() {
    EndChatComposeAndSend();
}
} // namespace GameNet

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletentries-getcount
 * @recoil-artifact defines .text recoil:function:0x414670: HudUiTripletEntries::GetCount.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: return the number of populated entries in the recovered scoreboard vector.
 */
int HudUiTripletEntries::GetCount() {
    if (begin == 0) {
        return 0;
    }

    return (int)(end - begin);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletentries-copyrange
 * @recoil-artifact defines .text recoil:function:0x4146a0: HudUiTripletEntries::CopyRange.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: copy a range of scoreboard entries into destination vector storage.
 */
HudUiScoreboardEntry *__stdcall HudUiTripletEntries::CopyRange(
    HudUiScoreboardEntry *sourceBegin,
    HudUiScoreboardEntry *sourceEnd,
    HudUiScoreboardEntry *dest
) {
    HudUiScoreboardEntry *cursor = dest;
    while (sourceBegin != sourceEnd) {
        if (cursor != 0) {
            *cursor = *sourceBegin;
        }
        ++sourceBegin;
        ++cursor;
    }

    return cursor;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduitripletentries-filln
 * @recoil-artifact defines .text recoil:function:0x4146e0: HudUiTripletEntries::FillN.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: fill consecutive scoreboard vector slots from one source entry.
 */
void __stdcall HudUiTripletEntries::FillN(
    HudUiScoreboardEntry *dest,
    unsigned int count,
    const HudUiScoreboardEntry *sourceValue
) {
    HudUiScoreboardEntry *cursor = dest;
    while (count != 0) {
        if (cursor != 0) {
            *cursor = *sourceValue;
        }
        ++cursor;
        --count;
    }
}

namespace HudUiListMenuEntry {
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.sortrange
 * @recoil-artifact defines .text recoil:function:0x414710: HudUiListMenuEntry::SortRange.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: partition larger scoreboard-entry ranges before the final insertion-sort pass.
 */
void __fastcall SortRange(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end,
    int unusedFlags
) {
    while (end - begin > 16) {
        HudUiScoreboardEntry *left = begin;
        HudUiScoreboardEntry *right = end - 1;
        HudUiScoreboardEntry *const middle = begin + ((end - begin) / 2);
        HudUiScoreboardEntry pivot = *HudRuntimeListMenuMedianOfThree(
            begin,
            middle,
            right
        );

        for (;;) {
            while (HudRuntimeListMenuEntryComesBefore(
                *left,
                pivot
            )) {
                ++left;
            }

            while (HudRuntimeListMenuEntryComesBefore(
                pivot,
                *right
            )) {
                --right;
            }

            if (right <= left) {
                break;
            }

            HudUiScoreboardEntry temp = *left;
            *left = *right;
            *right = temp;
            ++left;
        }

        if (end - left <= left - begin) {
            SortRange(
                left,
                end,
                unusedFlags
            );
            end = left;
        } else {
            SortRange(
                begin,
                left,
                unusedFlags
            );
            begin = left;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.insertpivotintosortedprefix
 * @recoil-artifact defines .text recoil:function:0x414930: HudUiListMenuEntry::InsertPivotIntoSortedPrefix.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: shift a sorted prefix forward and store the pivot entry at its sorted position.
 */
void InsertPivotIntoSortedPrefix(
    HudUiScoreboardEntry *slot,
    HudUiScoreboardEntry pivot
) {
    HudUiScoreboardEntry *insertSlot = slot;
    HudUiScoreboardEntry *previousEntry = insertSlot - 1;
    while (HudRuntimeListMenuEntryComesBefore(
        pivot,
        *previousEntry
    )) {
        *insertSlot = *previousEntry;
        insertSlot = previousEntry;
        --previousEntry;
    }

    *insertSlot = pivot;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.insertionsortrange
 * @recoil-artifact defines .text recoil:function:0x414980: HudUiListMenuEntry::InsertionSortRange.
 * Retail literal-backed physical source block: D:\Proj\Battlesport\hud.cpp.
 * Purpose: insertion-sort a scoreboard-entry range in place using the recovered list-menu ordering.
 */
void __fastcall InsertionSortRange(
    HudUiScoreboardEntry *begin,
    HudUiScoreboardEntry *end,
    int
) {
    if (begin == end) {
        return;
    }

    HudUiScoreboardEntry *current = begin + 1;
    while (current != end) {
        HudUiScoreboardEntry candidate = *current;
        if (HudRuntimeListMenuEntryComesBefore(
            candidate,
            *begin
        )) {
            HudUiScoreboardEntry *shiftCursor = current;
            while (shiftCursor != begin) {
                *shiftCursor = *(shiftCursor - 1);
                --shiftCursor;
            }

            *begin = candidate;
        } else {
            HudUiScoreboardEntry *insertSlot = current;
            HudUiScoreboardEntry *previousEntry = insertSlot - 1;
            while (HudRuntimeListMenuEntryComesBefore(
                candidate,
                *previousEntry
            )) {
                *insertSlot = *previousEntry;
                insertSlot = previousEntry;
                --previousEntry;
            }

            *insertSlot = candidate;
        }

        ++current;
    }
}
} // namespace HudUiListMenuEntry

namespace {
const char kHudTailGlobalContextSearchPath[] = ".;zbd";
const char kHudTailCommandNameWeaponSetMaxTetherAltitude[] =
    "WeaponSetMaxTetherAltitude";
} // namespace

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zinterp-globalcontext-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x414a60: zInterp_GlobalContext::StaticInitAndRegisterAtExit.
 *
 * Purpose: construct the process-wide interpreter and register its shutdown
 * callback during static initialization.
 */
int zInterp_GlobalContext::StaticInitAndRegisterAtExit() {
    StaticInit();
    return RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zinterp-globalcontext-staticinit
 * @recoil-artifact defines .text recoil:function:0x414a70: zInterp_GlobalContext::StaticInit.
 *
 * Purpose: static initializer wrapper for the process-wide interpreter.
 */
zInterp_Context *zInterp_GlobalContext::StaticInit() {
    return new (&g_zInterp_GlobalContext) zInterp_GlobalContext;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zinterp-globalcontext-registeratexit
 * @recoil-artifact defines .text recoil:function:0x414a80: zInterp_GlobalContext::RegisterAtExit.
 *
 * Purpose: register the global interpreter destructor with the CRT atexit list.
 */
int zInterp_GlobalContext::RegisterAtExit() {
    return atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zinterp-globalcontext-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x414a90: zInterp_GlobalContext::AtExitDestructor.
 *
 * Purpose: tear down the process-wide interpreter during CRT shutdown.
 */
void zInterp_GlobalContext::AtExitDestructor() {
    g_zInterp_GlobalContext.Destructor();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zinterp-globalcontext-zinterp-globalcontext
 * @recoil-artifact defines .text recoil:function:0x414ab0: zInterp_GlobalContext::zInterp_GlobalContext.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.zinterp-globalcontext-dispatchhook
 * @recoil-artifact defines .text recoil:function:0x414ad0: zInterp_GlobalContext::DispatchHook.
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
 * Original function; retail address 0x414b50.
 * Source owner: authored Westwood download event-sink callback member.
 * Purpose: handle an unused download event slot with a zero result.
 */
int WestwoodOnlineUpgradeDownloadEventSink::CallbackNoOp(
    void *
) {
    return 0;
}

#include "Battlesport/recoil_state_main_menu_transition.h"

#include "Battlesport/hud.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zUtil/zsave_game.h"

#include <new>

namespace {
/**
 * Recovered original inline source helper: no standalone retail function.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Evidence: repeated caller bodies at 0x414b60 and 0x414b90 read
 * zUtil_PlayerStateStorage::environmentAttachmentActive at offset 0x25c.
 * Purpose: Report whether the current player state blocks save/load menu actions.
 */
inline int PlayerMenuSaveLoadBlocked(
    zUtil_PlayerStateStorage *playerState
) {
    return playerState->environmentAttachmentActive;
}

} // namespace

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the background
 * member with a HudUiBackground base subobject.
 * Purpose: Initialize the dialog background member.
 */
inline HudUiMainMenuDialogBackground::HudUiMainMenuDialogBackground() : HudUiBackground() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the background
 * member without additional body work.
 * Purpose: Tear down the dialog background member.
 */
inline HudUiMainMenuDialogBackground::~HudUiMainMenuDialogBackground() {
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the credits
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the credits button member.
 */
inline HudUiMainMenuDialog_CreditsButton::HudUiMainMenuDialog_CreditsButton() :
    HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the credits
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the credits button member.
 */
inline HudUiMainMenuDialog_CreditsButton::~HudUiMainMenuDialog_CreditsButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the save
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the save button member.
 */
inline HudUiMainMenuDialog_SaveButton::HudUiMainMenuDialog_SaveButton() : HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the save
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the save button member.
 */
inline HudUiMainMenuDialog_SaveButton::~HudUiMainMenuDialog_SaveButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the load
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the load button member.
 */
inline HudUiMainMenuDialog_LoadButton::HudUiMainMenuDialog_LoadButton() : HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the load
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the load button member.
 */
inline HudUiMainMenuDialog_LoadButton::~HudUiMainMenuDialog_LoadButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the new-game
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the new-game button member.
 */
inline HudUiMainMenuDialog_NewGameButton::HudUiMainMenuDialog_NewGameButton() :
    HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the new-game
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the new-game button member.
 */
inline HudUiMainMenuDialog_NewGameButton::~HudUiMainMenuDialog_NewGameButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the options
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the options button member.
 */
inline HudUiMainMenuDialog_OptionsButton::HudUiMainMenuDialog_OptionsButton() :
    HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the options
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the options button member.
 */
inline HudUiMainMenuDialog_OptionsButton::~HudUiMainMenuDialog_OptionsButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the quit
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the quit button member.
 */
inline HudUiMainMenuDialog_QuitButton::HudUiMainMenuDialog_QuitButton() : HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the quit
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the quit button member.
 */
inline HudUiMainMenuDialog_QuitButton::~HudUiMainMenuDialog_QuitButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * Recovered original inline constructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::HudUiMainMenuDialog constructs the controls
 * button member as a HudUiZrdWidget-derived subobject.
 * Purpose: Initialize the controls button member.
 */
inline HudUiMainMenuDialog_ControlsButton::HudUiMainMenuDialog_ControlsButton() :
    HudUiZrdWidget() {
}

/**
 * Recovered original inline destructor: no standalone retail function.
 * Evidence: HudUiMainMenuDialog::~HudUiMainMenuDialog destroys the controls
 * button member through HudUiZrdWidget::DestructorCore.
 * Purpose: Tear down the controls button member.
 */
inline HudUiMainMenuDialog_ControlsButton::~HudUiMainMenuDialog_ControlsButton() {
    HudUiZrdWidget::DestructorCore();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-canloadgame
 * @recoil-artifact defines .text recoil:function:0x414b60: HudUiMainMenuDialog::CanLoadGame.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Allow load-game navigation unless the active player state is blocked.
 */
int HudUiMainMenuDialog::CanLoadGame() {
    zUtil_PlayerStateStorage *playerState;
    zInput_GameStateOrMapTablePartial *const gameState = g_GameStateOrMapTable;
    if (gameState == 0) {
        return 1;
    }

    playerState = (zUtil_PlayerStateStorage *)gameState->playerState;
    if (playerState == 0) {
        return 1;
    }

    if (PlayerMenuSaveLoadBlocked(playerState) != 0) {
        return 0;
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-cansavegame
 * @recoil-artifact defines .text recoil:function:0x414b90: HudUiMainMenuDialog::CanSaveGame.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Allow save-game navigation only when active game state is present and not blocked.
 */
int HudUiMainMenuDialog::CanSaveGame() {
    zInput_GameStateOrMapTablePartial *const gameState = g_GameStateOrMapTable;
    if (gameState == 0) {
        return (int)gameState;
    }

    zUtil_PlayerStateStorage *const playerState =
        (zUtil_PlayerStateStorage *)gameState->playerState;
    if (playerState == 0) {
        return 1;
    }

    if (PlayerMenuSaveLoadBlocked(playerState) != 0) {
        return 0;
    }
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-huduimainmenudialog
 * @recoil-artifact defines .text recoil:function:0x414bc0: HudUiMainMenuDialog::HudUiMainMenuDialog.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Load the route-specific main-menu layout and bind its child buttons.
 */
HudUiMainMenuDialog::HudUiMainMenuDialog(
    RecoilMainMenuEntryRoute route
) {
    // Preserves the VC5SP3 register lifetime observed in BN 0x414bc0 for the
    // repeatedly bound save, load, and quit button subobjects.
    HudUiMainMenuDialog_SaveButton *const saveButton = &saveGameButton;
    HudUiMainMenuDialog_LoadButton *const loadButton = &loadGameButton;
    HudUiMainMenuDialog_QuitButton *const quitButtonPtr = &quitButton;

    if (zOpt::GetNetworkEnabled() != 0) {
        zReader::Node *const loadedSection = LoadFromZrd(
            "dialog.zrd",
            "MAINMENU2",
            0
        );
        if (loadedSection != 0) {
            BindWidgetByName(
                loadedSection,
                &optionsButton,
                "OPTIONS"
            );
            BindWidgetByName(
                loadedSection,
                &controlsButton,
                "CONTROLS"
            );
            BindWidgetByName(
                loadedSection,
                &creditsButton,
                "CREDITS"
            );
            BindWidgetByName(
                loadedSection,
                &backButton,
                "BACK"
            );
            BindWidgetByName(
                loadedSection,
                quitButtonPtr,
                "QUIT"
            );
            FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
        }
        return;
    }

    if (route != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        zInput_GameStateOrMapTablePartial *const gameState = g_GameStateOrMapTable;
        zUtil_PlayerStateStorage *const playerState =
            (zUtil_PlayerStateStorage *)gameState->playerState;
        zReader::Node *loadedSection;
        if (playerState->lifecycleState == 4) {
            loadedSection = LoadFromZrd(
                "dialog.zrd",
                "MAINMENU3",
                0
            );
            if (loadedSection == 0) {
                return;
            }
            BindWidgetByName(
                loadedSection,
                &newGameButton,
                "NEWGAME"
            );
            BindWidgetByName(
                loadedSection,
                loadButton,
                "LOADGAME"
            );
            BindWidgetByName(
                loadedSection,
                quitButtonPtr,
                "QUIT"
            );
            FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
        } else {
            loadedSection = LoadFromZrd(
                "dialog.zrd",
                "MAINMENU1",
                0
            );
            if (loadedSection == 0) {
                return;
            }
            BindWidgetByName(
                loadedSection,
                &newGameButton,
                "NEWGAME"
            );
            BindWidgetByName(
                loadedSection,
                saveButton,
                "SAVEGAME"
            );
            BindWidgetByName(
                loadedSection,
                loadButton,
                "LOADGAME"
            );
            BindWidgetByName(
                loadedSection,
                &optionsButton,
                "OPTIONS"
            );
            BindWidgetByName(
                loadedSection,
                &controlsButton,
                "CONTROLS"
            );
            BindWidgetByName(
                loadedSection,
                &creditsButton,
                "CREDITS"
            );
            BindWidgetByName(
                loadedSection,
                &backButton,
                "BACK"
            );
            BindWidgetByName(
                loadedSection,
                quitButtonPtr,
                "QUIT"
            );
            FreeLoadedTreeRoots((int)(unsigned int)loadedSection);
        }

        saveButton->modeOrEnabled = CanSaveGame();
        saveButton->RefreshState();
        loadButton->modeOrEnabled = CanLoadGame();
        ((HudUiZrdWidget *)loadedSection)->RefreshState();
        return;
    }

    zReader::Node *const frontEndSection = LoadFromZrd(
        "dialog.zrd",
        "MAINMENU0",
        0
    );
    if (frontEndSection == 0) {
        return;
    }
    BindWidgetByName(
        frontEndSection,
        &newGameButton,
        "NEWGAME"
    );
    BindWidgetByName(
        frontEndSection,
        loadButton,
        "LOADGAME"
    );
    BindWidgetByName(
        frontEndSection,
        &optionsButton,
        "OPTIONS"
    );
    BindWidgetByName(
        frontEndSection,
        &controlsButton,
        "CONTROLS"
    );
    BindWidgetByName(
        frontEndSection,
        &creditsButton,
        "CREDITS"
    );
    BindWidgetByName(
        frontEndSection,
        quitButtonPtr,
        "QUIT"
    );
    FreeLoadedTreeRoots((int)(unsigned int)frontEndSection);

    loadButton->modeOrEnabled = CanLoadGame();
    ((HudUiZrdWidget *)frontEndSection)->RefreshState();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-creditsbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x414f40: HudUiMainMenuDialog_CreditsButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Queue the credits state and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_CreditsButton::OnActivate() {
    RecoilStateCredits::QueuePush();
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-savebutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x414f60: HudUiMainMenuDialog_SaveButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Open the save dialog and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_SaveButton::OnActivate() {
    RecoilStateSaveLoadTransition::QueueOpenSaveDialog(
        RECOIL_SAVELOAD_CAPTURE_PRESENTATION_DISABLED
    );
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-newgamebutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x414f80: HudUiMainMenuDialog_NewGameButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Enter the new-game overlay and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_NewGameButton::OnActivate() {
    HudUiNewGamePanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimenubackbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x414fa0: HudUiMenuBackButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Exit the current state and refresh the active HUD layout.
 */
void HudUiMenuBackButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
    HudUiMgr::TriggerCurrentLayoutOnActivated();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-optionsbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x414fc0: HudUiMainMenuDialog_OptionsButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Enter the options overlay and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_OptionsButton::OnActivate() {
    HudUiOptionsPanelOverlayOwner::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-quitbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x414fe0: HudUiMainMenuDialog_QuitButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Enter the quit confirmation state and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_QuitButton::OnActivate() {
    RecoilStateConfirmQuit::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-controlsbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x415000: HudUiMainMenuDialog_ControlsButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Enter the controls state and complete the standard ZRD widget activation.
 */
void HudUiMainMenuDialog_ControlsButton::OnActivate() {
    RecoilStateControls::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-destructor-huduimainmenudialog
 * @recoil-artifact defines .text recoil:function:0x415040: HudUiMainMenuDialog::~HudUiMainMenuDialog.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Provide the owner-level destructor body for member teardown.
 */
HudUiMainMenuDialog::~HudUiMainMenuDialog() {}
#include "Battlesport/recoil_state_dialog_host.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zVideo/zvid.h"

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatedialoghost-onupdateshouldquit
 * @recoil-artifact defines .text recoil:function:0x435e80: RecoilStateSaveLoadTransition::OnUpdateShouldQuit
 * (BN canonical folded body).
 *
 * Source owner: app_shell.folded_dialog_update_should_quit. BN shows the
 * retail body shared by DialogHost, MainMenuTransition, SaveLoadTransition,
 * and other dialog-hosted state vtable slots; this definition preserves the
 * DialogHost typed participant.
 *
 * Original-source function evidence: folded retail body 0x435e80.
 * Purpose: update and present the hosted HUD dialog each frame while a dialog
 * app state is current.
 */
int RecoilStateDialogHost::OnUpdateShouldQuit() {
    zInput::PollActiveDevices(0);

    if (m_dialog != 0) {
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();

        m_dialog->UpdateAll(g_FrameDeltaTimeSec);

        zVideo::Dispatch_UnlockPrimarySurfaceState();
    }

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
    return 0;
}
#include "Battlesport/recoil_state_main_menu_transition.h"

#include <new>
#include <stdlib.h>

#undef g_RecoilState_MainMenuTransition
/**
 * Data owner: legacy.app_shell.class_recoilstatemainmenutransition. BN exposes
 * a 0x18-byte zero-initialized .data object at 0x4edc58. The source keeps
 * explicit aligned storage so VC5 does not emit an automatic compiler startup
 * row; StaticInit constructs the typed singleton in place, and
 * AtExitDestructor destroys that same object. BN base-object xrefs are
 * StaticInit, AtExitDestructor, and QueueEnter.
 * Purpose: own the global app-state singleton used while transitioning into
 * the main menu.
 */
RecoilStateMainMenuTransitionStorage g_RecoilState_MainMenuTransition = {0};
#define g_RecoilState_MainMenuTransition \
    (*(RecoilStateMainMenuTransition *)&g_RecoilState_MainMenuTransition)

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x415100: RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit.
 *
 * Purpose: construct the static transition state and register its at-exit
 * destructor callback.
 */
void RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-staticinit
 * @recoil-artifact defines .text recoil:function:0x415110: RecoilStateMainMenuTransition::StaticInit.
 *
 * Purpose: construct the global main-menu transition state in place and return
 * it to the static-initialization wrapper.
 */
RecoilStateMainMenuTransition *RecoilStateMainMenuTransition::StaticInit() {
    return new (&g_RecoilState_MainMenuTransition) RecoilStateMainMenuTransition;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-registeratexit
 * @recoil-artifact defines .text recoil:function:0x415120: RecoilStateMainMenuTransition::RegisterAtExit.
 *
 * Purpose: register the static transition state's destruction callback with
 * the CRT at-exit list.
 */
void RecoilStateMainMenuTransition::RegisterAtExit() {
    atexit(RecoilStateMainMenuTransition::AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x415130: RecoilStateMainMenuTransition::AtExitDestructor.
 *
 * Purpose: destroy the global main-menu transition state from the registered
 * at-exit callback.
 */
void RecoilStateMainMenuTransition::AtExitDestructor() {
    g_RecoilState_MainMenuTransition.~RecoilStateMainMenuTransition();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *MainMenuTransitionCrtInitializerFn)();
/* VC5 emits this main-menu transition startup callback as a direct .CRT$XCU row. */
#pragma data_seg(".CRT$XCU")
MainMenuTransitionCrtInitializerFn s_MainMenuTransitionCrtInit =
    RecoilStateMainMenuTransition::StaticInitAndRegisterAtExit;
#pragma data_seg()
#endif
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduimainmenudialog-loadbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x415140: HudUiMainMenuDialog_LoadButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiMainMenuDialog.cpp.
 * Purpose: Open the load dialog using the frontend or in-game transition mode.
 */
void HudUiMainMenuDialog_LoadButton::OnActivate() {
    if (g_RecoilState_MainMenuTransition.m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_FADE);
        HudUiZrdWidget::OnActivate();
        return;
    }

    RecoilStateSaveLoadTransition::QueueOpenLoadDialog(RECOIL_SAVELOAD_MODE_STANDARD);
    HudUiZrdWidget::OnActivate();
}
/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-recoilstatemainmenutransition
 * @recoil-artifact defines .text recoil:function:0x415170: RecoilStateMainMenuTransition::RecoilStateMainMenuTransition.
 *
 * Purpose: initialize the static main-menu transition app state and clear its
 * dialog/audio ownership fields.
 */
RecoilStateMainMenuTransition::RecoilStateMainMenuTransition()
    : m_mainMenuDialog(0),
      m_savedHalfResAdjustMode(0),
      m_entryRoute(RECOIL_MAINMENU_ROUTE_FRONTEND),
      m_deferredVideoModeIndex(ZVID_MODE_INVALID_COMPLEMENT),
      m_pausedAudioSnapshot(0) {}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-destructor-recoilstatemainmenutransition
 * @recoil-artifact defines .text recoil:function:0x4151b0: RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition.
 *
 * Purpose: disable and destroy the owned main-menu dialog during transition
 * state teardown.
 */
RECOIL_NO_GS RecoilStateMainMenuTransition::~RecoilStateMainMenuTransition() {
    HudUiMainMenuDialog *dialog = m_mainMenuDialog;
    if (dialog != 0) {
        dialog->SetEnabled(0);

        dialog = m_mainMenuDialog;
        if (dialog != 0) {
            delete dialog;
        }

        m_mainMenuDialog = 0;
    }
}
#include "Battlesport/recoil_state_main_menu_transition.h"

#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zVideo/zvid.h"

extern char g_HudUiDialogSampleSetName[0x7];

namespace zVideo {
int __fastcall SetHalfResAdjustMode(int mode);
}

namespace HudUi {
void __fastcall SetInvalidateMode(int mode);
}

namespace zSnd {
int GetCDAudioOption();
}

namespace zSndCd {
int __fastcall PlayTrackWithMode(
    int track,
    int mode
);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-ontrybecomecurrent
 * @recoil-artifact defines .text recoil:function:0x415220: RecoilStateMainMenuTransition::OnTryBecomeCurrent.
 *
 * Purpose: enter the main-menu transition by preparing video/HUD state,
 * pausing active sounds, loading dialog audio, constructing the menu dialog,
 * and starting CD audio when enabled.
 */
RECOIL_NO_GS int RecoilStateMainMenuTransition::OnTryBecomeCurrent() {
    if (g_zVideo_ActiveRendererPath != 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(
            0,
            0
        );
    }

    m_savedHalfResAdjustMode = zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        zFMV_ActionBlur blurAction(
            4,
            1
        );
        zFMV_Action *action = &blurAction;
        action->Begin(0.0);
        while (action->Update(0.0) != 0) {
        }
        action->End();
    }

    zSndPlayHandleSnapshot *const audioSnapshot = zSndPlayHandleSnapshot::CreateFromActiveSamples();
    m_pausedAudioSnapshot = (RecoilPtr32)(unsigned int)audioSnapshot;
    audioSnapshot->StopAllIfPlaying();

    zSndSampleSet_InitByName(g_HudUiDialogSampleSetName);

    HudUiMainMenuDialog *const dialog = new HudUiMainMenuDialog(m_entryRoute);

    m_mainMenuDialog = dialog;

    dialog->SetEnabled(1);

    if (zSnd::GetCDAudioOption() != 0) {
        zSndCd::PlayTrackWithMode(
            2,
            5
        );
    }

    g_RecoilState_MainMenuSkipExitDelay = 0;
    return 1;
}

/**
 * Original-source function evidence: BN shows the retail main-menu transition OnSuspend slot
 * sharing the 0x408f50 RecoilStateDialogHost::OnSuspend body. This typed
 * definition preserves the MainMenuTransition source participant without
 * adding table or dispatch scaffolding.
 * Purpose: disable, blit, unlock, and present the hosted main-menu dialog when
 * a submenu state is pushed on top of it.
 */
void RecoilStateMainMenuTransition::OnSuspend(
    int param
) {
    (void)param;

    if (m_mainMenuDialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    m_mainMenuDialog->SetEnabled(0);
    ((HudUiDialogController *)m_mainMenuDialog)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
}

#include "Battlesport/recoil_state_main_menu_transition.h"

#include "GameZRecoil/zGame/zgame.h"

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-onresume
 * @recoil-artifact defines .text recoil:function:0x415370: RecoilStateMainMenuTransition::OnResume.
 *
 * Purpose: re-enable and refresh the main-menu dialog after a child state
 * resumes back into the menu transition state.
 */
void RecoilStateMainMenuTransition::OnResume(
    int param
) {
    if (m_mainMenuDialog == 0 || param != 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    m_mainMenuDialog->SetEnabled(1);
    ((HudUiContainer *)m_mainMenuDialog)->InvalidateChildren();
    ((HudUiContainer *)m_mainMenuDialog)->UpdateAll(0.0f);

    zVideo::Dispatch_UnlockPrimarySurfaceState();

    zVideo::AdjustSurfacesIfEnabled(
        (zVidRect32 *)zOpt::GetWindowSection(),
        (zVidRect32 *)zOpt::GetWindowSection(),
        1,
        1
    );
}
#include "Battlesport/recoil_state_main_menu_transition.h"

#include "GameZRecoil/zGame/zgame.h"

#include <windows.h>

extern char g_HudUiDialogSampleSetName[0x7];

namespace zVideo {
int __fastcall SetHalfResAdjustMode(int mode);
int __fastcall Init_ApplyModeIndex(int modeIndex);
} // namespace zVideo

namespace HudUi {
void __fastcall SetInvalidateMode(int mode);
}

namespace zVid {
int GetVideoModeIndexFromOptions();
void __fastcall SetVideoModeIndex(int modeIndex);
} // namespace zVid

namespace {
/**
 * Original inline helper observed in caller 0x4153d0.
 *
 * Purpose: apply a deferred video mode when it differs from the current option
 * and synchronize half-resolution and HUD invalidation state.
 */
static inline void ApplyDeferredVideoMode(
    int targetMode,
    zVideoHalfResAdjustMode halfResMode
) {
    if (zVid::GetVideoModeIndexFromOptions() == targetMode) {
        return;
    }

    if (zVideo::Init_ApplyModeIndex(targetMode) != 0) {
        return;
    }

    zVid::SetVideoModeIndex(targetMode);
    zVideo::SetHalfResAdjustMode(halfResMode);
    HudUi::SetInvalidateMode(halfResMode);
}
} // namespace

namespace zOpt {
int __fastcall SetHudTypeForCurrentHwMode(int hudType);
}

namespace HudUiMgr {
void __cdecl TriggerCurrentLayoutOnActivated();
}

namespace zInput {
void __cdecl Keyboard_ResetTransitionState();
}

namespace zSnd {
int GetCDAudioOption();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-ondeactivate
 * @recoil-artifact defines .text recoil:function:0x4153d0: RecoilStateMainMenuTransition::OnDeactivate.
 *
 * Purpose: tear down the main-menu dialog, apply deferred video/HUD/audio
 * restoration, resume paused sounds, and stop CD audio when leaving the state.
 */
void RecoilStateMainMenuTransition::OnDeactivate() {
    int previousHudType;

    if (m_mainMenuDialog != 0) {
        zVideo::RunPostprocessOnPrimaryBuffer();

        HudUiMainMenuDialog *dialog = m_mainMenuDialog;
        dialog->SetEnabled(0);

        ((HudUiDialogController *)m_mainMenuDialog)->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        dialog = m_mainMenuDialog;
        if (dialog != 0) {
            delete dialog;
        }

        m_mainMenuDialog = 0;
    }

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        previousHudType = zOpt::SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_STANDARD);
    }

    switch (m_deferredVideoModeIndex) {
    case 5:
        ApplyDeferredVideoMode(
            5,
            ZVIDEO_HALFRES_ADJUST_ENABLED
        );
        break;
    case 3:
        ApplyDeferredVideoMode(
            3,
            ZVIDEO_HALFRES_ADJUST_DISABLED
        );
        break;
    case 4:
        ApplyDeferredVideoMode(
            4,
            ZVIDEO_HALFRES_ADJUST_ENABLED
        );
        break;
    case 2:
        ApplyDeferredVideoMode(
            2,
            ZVIDEO_HALFRES_ADJUST_DISABLED
        );
        break;
    case 6:
        ApplyDeferredVideoMode(
            6,
            ZVIDEO_HALFRES_ADJUST_ENABLED
        );
        break;
    case 7:
        ApplyDeferredVideoMode(
            7,
            ZVIDEO_HALFRES_ADJUST_ENABLED
        );
        break;
    default:
        zVideo::SetHalfResAdjustMode(m_savedHalfResAdjustMode);
        HudUi::SetInvalidateMode(m_savedHalfResAdjustMode);
        break;
    }

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        zOpt::SetHudTypeForCurrentHwMode(previousHudType);
    }

    if (m_entryRoute != RECOIL_MAINMENU_ROUTE_FRONTEND) {
        HudUiMgr::TriggerCurrentLayoutOnActivated();
    }

    zInput::Keyboard_ResetTransitionState();

    if (g_RecoilState_MainMenuSkipExitDelay == 0) {
        Sleep(0x3e8);
        zSndSampleSet_DestroyByName(g_HudUiDialogSampleSetName);

        zSndPlayHandleSnapshot *snapshot =
            (zSndPlayHandleSnapshot *)(unsigned int)m_pausedAudioSnapshot;
        if (snapshot != 0) {
            snapshot->RestoreAllWithGlobalVolumeDelta();
        }

        snapshot = (zSndPlayHandleSnapshot *)(unsigned int)m_pausedAudioSnapshot;
        if (snapshot != 0) {
            snapshot->Destroy();
            m_pausedAudioSnapshot = 0;
        }
    }

    if (zSnd::GetCDAudioOption() != 0) {
        zSndCd::Stop();
    }
}
#include "Battlesport/recoil_state_main_menu_transition.h"

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-clearpausedaudiosnapshot
 * @recoil-artifact defines .text recoil:function:0x415630: RecoilStateMainMenuTransition::ClearPausedAudioSnapshot.
 *
 * Purpose: destroy and clear the global main-menu transition paused-audio
 * snapshot when callers need to discard the saved audio state.
 */
void RecoilStateMainMenuTransition::ClearPausedAudioSnapshot() {
    zSndPlayHandleSnapshot *const snapshot =
        (zSndPlayHandleSnapshot *)g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot;
    if (snapshot != 0) {
        snapshot->Destroy();
        g_RecoilState_MainMenuTransition.m_pausedAudioSnapshot = 0;
    }
}
#include "Battlesport/recoil_state_main_menu_transition.h"

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-queueenter
 * @recoil-artifact defines .text recoil:function:0x415650: RecoilStateMainMenuTransition::QueueEnter.
 *
 * Purpose: record the requested main-menu entry route and queue the global
 * transition state on RecoilApp's app-state stack.
 */
void __fastcall RecoilStateMainMenuTransition::QueueEnter(
    RecoilMainMenuEntryRoute entryRoute
) {
    g_RecoilState_MainMenuTransition.m_entryRoute = entryRoute;
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilState_MainMenuTransition,
        0
    );
}
#include "Battlesport/recoil_state_main_menu_transition.h"

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstatemainmenutransition-setdeferredvideomodeindex
 * @recoil-artifact defines .text recoil:function:0x415670: RecoilStateMainMenuTransition::SetDeferredVideoModeIndex.
 *
 * Purpose: store the requested video-mode index on the global main-menu
 * transition state for deferred application during transition shutdown.
 */
void __fastcall RecoilStateMainMenuTransition::SetDeferredVideoModeIndex(
    zVidModeIndex modeIndex
) {
    g_RecoilState_MainMenuTransition.m_deferredVideoModeIndex = modeIndex;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.huduibackgroundconfirmquit-constructor
 * @recoil-artifact defines .text recoil:function:0x415680: HudUiBackgroundConfirmQuit::Constructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduiconfirmquitokbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x415740: HudUiConfirmQuitOkButton::OnActivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.huduibackgroundconfirmquit-destructor
 * @recoil-artifact defines .text recoil:function:0x4157b0: HudUiBackgroundConfirmQuit::Destructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp.
 * Purpose: Destroy the confirm-quit child widgets before the inherited background cleanup.
 */
void HudUiBackgroundConfirmQuit::Destructor() {
    cancelButton.~HudUiConfirmQuitCancelButton();
    okButton.~HudUiConfirmQuitOkButton();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-staticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x415810: RecoilStateConfirmQuit::StaticInitAndRegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for RecoilStateConfirmQuit::StaticInitAndRegisterAtExit.
 */
void RecoilStateConfirmQuit::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-staticinit
 * @recoil-artifact defines .text recoil:function:0x415820: RecoilStateConfirmQuit::StaticInit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for RecoilStateConfirmQuit::StaticInit.
 */
RecoilStateConfirmQuit *RecoilStateConfirmQuit::StaticInit() {
    return new (&g_RecoilState_ConfirmQuit) RecoilStateConfirmQuit;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-registeratexit
 * @recoil-artifact defines .text recoil:function:0x415830: RecoilStateConfirmQuit::RegisterAtExit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: preserve the recovered HUD behavior for RecoilStateConfirmQuit::RegisterAtExit.
 */
void RecoilStateConfirmQuit::RegisterAtExit() {
    atexit(AtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-atexitdestructor
 * @recoil-artifact defines .text recoil:function:0x415840: RecoilStateConfirmQuit::AtExitDestructor.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: run the recovered RecoilStateConfirmQuit::AtExitDestructor teardown path.
 */
void RecoilStateConfirmQuit::AtExitDestructor() {
    g_RecoilState_ConfirmQuit.~RecoilStateConfirmQuit();
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-recoilstateconfirmquit
 * @recoil-artifact defines .text recoil:function:0x415850: RecoilStateConfirmQuit::RecoilStateConfirmQuit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: initialize the confirm-quit app state and clear its dialog pointer.
 */
RecoilStateConfirmQuit::RecoilStateConfirmQuit() {
    m_dialog = 0;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-destructor-recoilstateconfirmquit
 * @recoil-artifact defines .text recoil:function:0x415880: RecoilStateConfirmQuit::~RecoilStateConfirmQuit.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\RecoilStateConfirmQuit.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-ontrybecomecurrent
 * @recoil-artifact defines .text recoil:function:0x4158f0: RecoilStateConfirmQuit::OnTryBecomeCurrent.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-ondeactivate
 * @recoil-artifact defines .text recoil:function:0x415960: RecoilStateConfirmQuit::OnDeactivate.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
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
 * @recoil-anchor recoil:anchor:battlesport.hud.recoilstateconfirmquit-queueenter
 * @recoil-artifact defines .text recoil:function:0x4159b0: RecoilStateConfirmQuit::QueueEnter.
 * Provisional source-placement hypothesis: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: queue the recovered HUD application-state transition for RecoilStateConfirmQuit::QueueEnter.
 */
void RecoilStateConfirmQuit::QueueEnter() {
    g_RecoilApp.QueuePushState(
        (RecoilApp_IState *)&g_RecoilState_ConfirmQuit,
        0
    );
}

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zfmv-action-update
 * @recoil-artifact defines .text recoil:function:0x4159d0: zFMV_Action::Update.
 * Purpose: report immediate completion for action types without update behavior.
 */
int zFMV_Action::Update(
    double
) {
    return 0;
}

#if defined(_MSC_VER) && _MSC_VER <= 1100
extern "C" __declspec(dllimport) unsigned long __stdcall GetTickCount();
#endif

/**
 * @recoil-anchor recoil:anchor:battlesport.hud.zfmv-action-runblockingtimed
 * @recoil-artifact defines .text recoil:function:0x4159e0: zFMV_Action::RunBlockingTimed.
 * Purpose: run an action to completion using elapsed milliseconds from GetTickCount.
 */
void zFMV_Action::RunBlockingTimed() {
    const double startSec = (double)(GetTickCount()) * 0.00100000005;
    Begin(0.0);
    double currentSec =
        ((double)(GetTickCount()) * 0.00100000005) - startSec;
    while (Update(currentSec) != 0) {
        currentSec =
            ((double)(GetTickCount()) * 0.00100000005) - startSec;
    }
    End();
}
