#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/hud.h"

#include "Battlesport/GameNet.h"
#include "Battlesport/RecoilStateCredits.h"
#include "Battlesport/pickup.h"
#include "Battlesport/player.h"
#include "Battlesport/zStr.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zSaveGame.h"

#include <math.h>
#include <new>
#include <stdlib.h>
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
        &g_HudUiNewGamePanelOverlayOwner,
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

/**
 * Reimplements 0x40d1c0: HudUiOptionsPanelOverlayOwner::QueueEnter.
 * Original source path: D:\Proj\Battlesport\HudOptionsDialog.cpp.
 * Purpose: Queue the global options-panel overlay owner as the next app state.
 */
void HudUiOptionsPanelOverlayOwner::QueueEnter() {
    g_RecoilApp.QueuePushState(
        &g_HudUiOptionsPanelOverlayOwner,
        0
    );
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
 * Reimplements 0x4159b0: RecoilStateConfirmQuit::QueueEnter.
 * Original source path: D:\Proj\Battlesport\HudConfirmQuitDialog.cpp.
 * Purpose: queue the recovered HUD application-state transition for RecoilStateConfirmQuit::QueueEnter.
 */
void RecoilStateConfirmQuit::QueueEnter() {
    g_RecoilApp.QueuePushState(
        &g_RecoilState_ConfirmQuit,
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
 * Reimplements 0x4070e0: HudUiCheatCodeTitleWidget::OnActivate.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: Queue the cheat-code state exit when the GO widget is activated.
 */
void HudUiCheatCodeTitleWidget::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

/**
 * Reimplements 0x406d20: HudUiCheatCodeDialog::HudUiCheatCodeDialog.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: Construct the cheat-code dialog, configure the input widget, and bind the ZRD widgets.
 */
HudUiCheatCodeDialog::HudUiCheatCodeDialog()
    : HudUiBackground() {
    cheatInputWidget.textInput.AllocTextBuffer(80);
    cheatInputWidget.Update("");
    cheatInputWidget.SetInputActive(1);
    cheatInputWidget.SetRawKeyboardCapture(1);

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
 * Reimplements 0x406e30: HudUiCheatCodeDialog::Destructor.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: Destroy the cheat-code input and title widgets before background cleanup.
 */
void HudUiCheatCodeDialog::Destructor() {
    cheatInputWidget.Destructor();
    titleWidget.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

/**
 * Reimplements 0x406e10: HudUiCheatCodeDialog::ScalarDeletingDestructor.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: Run cheat-code dialog cleanup and optionally free the object for VC5 scalar delete.
 */
HudUiBackground * HudUiCheatCodeDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

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
 * Purpose: run placement construction for the global cheat-code app-state object.
 */
RecoilStateCheatCode *RecoilStateCheatCode::ConstructGlobal() {
    return new (&g_RecoilStateCheatCode) RecoilStateCheatCode;
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
    g_RecoilStateCheatCode.~RecoilStateCheatCode();
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

    HudUiCheatCodeDialog *dialog = (HudUiCheatCodeDialog *)m_dialog;
    if (dialog != 0) {
        commandString = dialog->cheatInputWidget.GetBuffer();

        zVideo::RunPostprocessOnPrimaryBuffer();

        dialog->SetEnabled(0);

        ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        dialog = (HudUiCheatCodeDialog *)m_dialog;
        if (dialog != 0) {
            dialog->ScalarDeletingDestructor(1);
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
 * Reimplements 0x406f00: RecoilStateCheatCode::Destructor.
 * Original source path: D:\Proj\Battlesport\HudUiCheatCode.cpp.
 * Purpose: release any active cheat-code dialog and clear the app-state dialog pointer.
 */
RecoilStateCheatCode::~RecoilStateCheatCode() {
    HudUiCheatCodeDialog *dialog = (HudUiCheatCodeDialog *)m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
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
    dialog->Update(0.0f);
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
        &g_RecoilStateControls,
        0
    );
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
        &g_RecoilStateCheatCode,
        0
    );
    return 1;
}

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

    char *const command = commandString->GetBuffer(1);
    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)g_GameStateOrMapTable;

    if (zStr::ContainsCaseInsensitive(
            command,
            zLoc::GetMessageString(kHudCheatPickup901MessageId)
        ) != 0) {
        return Pickup::ApplyEffect(
            kHudCheatPickup901TypeId,
            0,
            saveState
        );
    }

    if (zStr::ContainsCaseInsensitive(command, zLoc::GetMessageString(kHudCheatRespawnMessageId)) !=
        0) {
        zUtil_PlayerStateStorage *const playerState = saveState->playerState;
        if (playerState->recentHitValid != 0) {
            zEffectAnim::Stop(playerState->recentHitLightHandle);
            playerState->recentHitLightHandle = 0;
            playerState->recentHitValid = 0;
        }

        if (playerState->lifecycleState == kHudCheatLifecycleInactive) {
            playerState->lifecycleState = kHudCheatLifecycleLocal;
            zOpt::SetSteeringMode(g_PlayerPrevSteeringMode);
            Player::ApplyCameraState(g_PlayerPrevCameraState);
            Player::ResetMouseControlStateAndRecenterCursor(saveState);
            zEffect_Anim::NodeActionCallback(
                playerState->destroyedRespawnFxEntry,
                playerState->rootNode
            );
            Player::ResetDamageStateAndTimedHitStatus(saveState);

            const int masterType = saveState->primaryModalState->masterModalData->masterType;
            playerState->aiMode = 0;
            playerState->nextModeSwitchAllowedTime = 0.0f;
            playerState->autoTurnSign = 0;
            playerState->motionInput = 0;
            Player::TransitionToMasterTypeTrack(
                saveState,
                1
            );
            playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;

            if (masterType == kHudCheatMasterTypeSub) {
                Player::TransitionToMasterTypeAmphib(
                    saveState,
                    0,
                    1
                );
                playerState->primaryGunGateUntilTime = g_Time_AccumulatedTimeSec;
                Player::TransitionToMasterTypeSub(
                    saveState,
                    0
                );
            } else if (masterType == kHudCheatMasterTypeHover) {
                Player::TransitionToMasterTypeHover(
                    saveState,
                    0
                );
            } else if (masterType == kHudCheatMasterTypeAmphib) {
                Player::TransitionToMasterTypeAmphib(
                    saveState,
                    0,
                    0
                );
            }
        }

        playerState->altGunTransitionState = kHudCheatAltGunTransitionReset;
        return Pickup::ApplyEffect(
            kHudCheatRespawnPickupTypeId,
            0,
            saveState
        );
    }

    if (zStr::ContainsCaseInsensitive(
            command,
            zLoc::GetMessageString(kHudCheatPickup903MessageId)
        ) != 0) {
        return Pickup::ApplyEffect(
            kHudCheatPickup903TypeId,
            0,
            saveState
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

namespace zOpt {

/**
 * Reimplements 0x413600: zOpt::ToggleHudTypeForCurrentHwMode.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Toggle the HUD type between standard and perspective for the current hardware mode.
 */
int ToggleHudTypeForCurrentHwMode() {
    const int currentHudType = GetHudTypeForCurrentHwMode();
    if (currentHudType == ZOPT_HUD_TYPE_STANDARD) {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_PERSPECTIVE);
    }
    if (currentHudType == ZOPT_HUD_TYPE_PERSPECTIVE) {
        return SetHudTypeForCurrentHwMode(ZOPT_HUD_TYPE_STANDARD);
    }
    return GetHudTypeForCurrentHwMode();
}

} // namespace zOpt

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
