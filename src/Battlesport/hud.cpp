#include "Battlesport/Mfc42Abi.h"
#include "Battlesport/hud.h"

#include "Battlesport/GameNet.h"
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

HudUiNewGamePanelOverlayOwner g_HudUiNewGamePanelOverlayOwner;
HudUiOptionsPanelOverlayOwner g_HudUiOptionsPanelOverlayOwner;
RecoilStateConfirmQuit g_RecoilState_ConfirmQuit;
RecoilStateControls g_RecoilStateControls;
RecoilStateCheatCode g_RecoilStateCheatCode;
zSndSample *g_Hud_LowMeterBeepSample = 0;
zSndSample *g_Hud_LowMeterLoopSample = 0;
int g_Hud_LowMeterLoopActive = 0;
float g_Hud_LowMeterBeepInterval = 0.0f;
float g_Hud_LowMeterNextBeepTime = 0.0f;

extern "C" int g_RecoilState_MainMenuSkipExitDelay;

namespace {
typedef void(__fastcall *HudWeatherFxTextureUploadProc)(
    zVideo_TextureRecordPartial *textureRecord,
    void *reserved,
    zVidImagePartial *image
);
typedef void(__fastcall *HudWeatherFxSubmitPolyProc)(
    zVideo_XyzVertex *vertices,
    zVideo_TexCoord *texCoords,
    int vertexCount,
    zVideo_RenderClass *renderClass,
    unsigned int renderParam,
    float alpha,
    int queueMode
);
typedef void(*HudWeatherFxFlushProc)();

const float kHudWeatherFxDepthRandScale = -0.0000152592547f;
const float kHudWeatherFxConeRandScale = -0.0000457777642f;
const float kHudWeatherFxDepthBase = 0.5f;
const float kHudWeatherFxConeBase = -1.0f;
const float kHudWeatherFxReflectBias = 1.5f;
const float kHudWeatherFxCameraDriftScale = -0.100000001f;
const float kHudWeatherFxForceScale = 0.100000001f;
const float kHudWeatherFxConeDepthMax = 1.5f;
const float kHudWeatherFxProjectionCenter = 0.5f;
const float kHudWeatherFxProbeScale = 0.100000001f;
const float kHudWeatherFxProbeVelocityMinSq = 0.0100000007f;
const float kHudWeatherFxVelocityMaxSq = 1.0f;
const float kHudWeatherFxSnowSlantScale = 3.5f;
const int kHudWeatherFxRainSlantDelta = 1;

// Source-faithful helper recovered from address-backed callers in this source file.
float HudWeatherFxVec3LengthSq(
    const zVec3 *value
) {
    return value->x * value->x + value->y * value->y + value->z * value->z;
}

// Source-faithful helper recovered from address-backed callers in this source file.
int HudWeatherFxSnowNeedsReset(
    const zVec3 *position
) {
    const float absZ = (float)(fabs(position->z));
    if ((float)(fabs(position->y)) > absZ) {
        return 1;
    }
    if ((float)(fabs(position->x)) > absZ) {
        return 1;
    }
    if (position->z > 1.0f) {
        return 1;
    }
    if (position->z < 0.5f) {
        return 1;
    }
    return 0;
}

enum zVideoRendererBackend {
    ZVID_RENDERER_BACKEND_SOFTWARE = 0,
};

} // namespace

float g_HudWeatherFxSnow_LastCameraTargetX = 0.0f;
float g_HudWeatherFxSnow_LastCameraTargetY = 0.0f;
float g_HudWeatherFxSnow_LastCameraTargetZ = 0.0f;
float g_HudWeatherFxSnow_TimeAccumulator = 0.0f;
float g_HudWeatherFxRain_LastCameraTargetX = 0.0f;
float g_HudWeatherFxRain_LastCameraTargetY = 0.0f;
float g_HudWeatherFxRain_LastCameraTargetZ = 0.0f;
float g_HudWeatherFxRain_TimeAccumulator = 0.0f;
// Reimplements 0x4bdc70: HudWeatherFx::Constructor
// (D:\Proj\Battlesport\hud.cpp)
HudWeatherFx * HudWeatherFx::Constructor(
    int newParticleCount
) {
    HudUiElement::Constructor(
        0,
        0
    );
    viewportRect = 0;
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
        char *const alphaMap = (char *)(malloc(0x80));
        void *const surfacePixels = malloc(0x100);
        zVid_Image_SetPixels(
            softwareImage,
            surfacePixels,
            alphaMap
        );
        softwareImage->formatFlagsPacked |= 0x20;
        zVid_Image::SetSize(
            softwareImage,
            16,
            8
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

// Reimplements 0x4bde20: HudWeatherFx::ScalarDeletingDestructor
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

// Reimplements 0x4bde40: HudWeatherFx::Destructor
// (D:\Proj\Battlesport\hud.cpp)
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
            ((zVideo_DestroyTextureRecordProc)(g_zVideo_pfnTextureRecordDestroy))(textureRecord);
        }
        if (softwareImage != 0) {
            zVid_Image::ReleaseIfNotDefault(softwareImage);
            softwareImage = 0;
        }
    }

}

// Reimplements 0x4be210: HudWeatherFx::ArePointBatchInsideRect
// (D:\Proj\Battlesport\hud.cpp)
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

// Reimplements 0x4bdee0: HudWeatherFx::ResetParticleSlot
// (D:\Proj\Battlesport\hud.cpp)
void HudWeatherFx::ResetParticleSlot(
    int particleIndex,
    int
) {
    zVec3 *const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
    zVec3 *const destPosition = &particlePositions[destBufferIndex][particleIndex];

    sourcePosition->z = kHudWeatherFxDepthBase - (float)(rand()) * kHudWeatherFxDepthRandScale;

    sourcePosition->x = kHudWeatherFxConeBase - (float)(rand()) * kHudWeatherFxConeRandScale;
    if (sourcePosition->x < -sourcePosition->z) {
        sourcePosition->x += kHudWeatherFxReflectBias;
        sourcePosition->z = kHudWeatherFxReflectBias - sourcePosition->z;
    }

    sourcePosition->y = kHudWeatherFxConeBase - (float)(rand()) * kHudWeatherFxConeRandScale;
    if (sourcePosition->y < -sourcePosition->z) {
        sourcePosition->y += kHudWeatherFxReflectBias;
        sourcePosition->z = kHudWeatherFxReflectBias - sourcePosition->z;
    }

    *destPosition = *sourcePosition;
}

// Reimplements 0x4bdfd0: HudWeatherFx::DrawParticles
// (D:\Proj\Battlesport\hud.cpp)
void HudWeatherFx::DrawParticles() {
    if (g_zVideo_ActiveRendererPath == ZVID_RENDERER_BACKEND_SOFTWARE) {
        zVideo_FxSurface::DrawColoredLinesBatch(
            (zVideoFxColoredLineRecord *)(particleQuads),
            particleCount,
            (zVidRect32 *)(viewportRect)
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

    ((HudWeatherFxTextureUploadProc)(g_zVideo_pfnTextureRecordFinalizeUpload))(
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

        if (((HudWeatherFxPointBatch *)(clipVerts))->ArePointBatchInsideRect(4, viewportRect) !=
            0) {
            ((HudWeatherFxSubmitPolyProc)(g_zVideo_pfnSubmitPolyRenderClass))(
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

    ((HudWeatherFxFlushProc)(g_zVideo_pfnFlushSortedPolys))();
    zVideoD3D::SceneLeave();
    if (swSurfaceWasLocked != 0) {
        zVideo::RunPostprocessOnSwBuffer();
    }
}

// Reimplements 0x4be280: HudWeatherFxSnow::Constructor
// (D:\Proj\Battlesport\hud.cpp)
HudWeatherFxSnow * HudWeatherFxSnow::Constructor(
    int particleCount
) {
    HudWeatherFx::Constructor(particleCount);
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
    return this;
}

// Reimplements 0x4be2c0: HudWeatherFxSnow::ScalarDeletingDestructor
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

// Reimplements 0x4be2e0: HudWeatherFxSnow::Destructor
// Snow has no additional teardown; the retail body tail-calls the shared base destructor.
void HudWeatherFxSnow::Destructor() {
    HudWeatherFx::Destructor();
}

// Reimplements 0x4be2f0: HudWeatherFxSnow::Update
// (D:\Proj\Battlesport\hud.cpp)
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
    if (viewportRect != 0) {
        viewportWidth = viewportRect->right - viewportRect->left;
        viewportHeight = viewportRect->bottom - viewportRect->top;
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
        (g_HudWeatherFxSnow_LastCameraTargetX - cameraTarget.x) * kHudWeatherFxCameraDriftScale;
    cameraTargetDrift.y =
        (g_HudWeatherFxSnow_LastCameraTargetY - cameraTarget.y) * kHudWeatherFxCameraDriftScale;
    cameraTargetDrift.z =
        (g_HudWeatherFxSnow_LastCameraTargetZ - cameraTarget.z) * kHudWeatherFxCameraDriftScale;
    g_HudWeatherFxSnow_LastCameraTargetX = cameraTarget.x;
    g_HudWeatherFxSnow_LastCameraTargetY = cameraTarget.y;
    g_HudWeatherFxSnow_LastCameraTargetZ = cameraTarget.z;

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
    const float gravityScale = gravity * kHudWeatherFxForceScale;
    gravityOffset.x = basisVector.x * gravityScale;
    gravityOffset.y = basisVector.y * gravityScale;
    gravityOffset.z = basisVector.z * gravityScale;
    zMath::MatTransformPointBatchInPlace(
        &gravityOffset,
        1
    );

    zVec3 windOffset;
    const float windScale = windVelocity * kHudWeatherFxForceScale;
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
    if (HudWeatherFxVec3LengthSq(&particleVelocity) >= kHudWeatherFxVelocityMaxSq) {
        zMath::Vec3Normalize(&particleVelocity);
    }

    zVec3 probeVelocity = particleVelocity;
    if (HudWeatherFxVec3LengthSq(&probeVelocity) >= kHudWeatherFxProbeVelocityMinSq) {
        zMath::Vec3Normalize(&probeVelocity);
        probeVelocity.x *= kHudWeatherFxProbeScale;
        probeVelocity.y *= kHudWeatherFxProbeScale;
        probeVelocity.z *= kHudWeatherFxProbeScale;
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

        const float sourceDepthFactor = kHudWeatherFxConeDepthMax - sourcePosition->z;
        const float probeDepthFactor = kHudWeatherFxConeDepthMax - probePosition.z;
        HudWeatherFxParticleQuad *const particleQuad = &particleQuads[particleIndex];
        particleQuad->x =
            (int)(((probeDepthFactor * probePosition.x) + kHudWeatherFxProjectionCenter) *
                  viewportWidthF);
        particleQuad->y =
            (int)(((probeDepthFactor * probePosition.y) + kHudWeatherFxProjectionCenter) *
                  viewportHeightF);
        particleQuad->width =
            (int)(((sourceDepthFactor * sourcePosition->x) + kHudWeatherFxProjectionCenter) *
                  viewportWidthF) -
            particleQuad->x;
        particleQuad->height =
            (int)(((sourceDepthFactor * sourcePosition->y) + kHudWeatherFxProjectionCenter) *
                  viewportHeightF) -
            particleQuad->y;
        particleQuad->color16 = packedColor16;
        particleQuad->texCoordUStart = probeDepthFactor * alphaStartScale;
        particleQuad->texCoordUEnd = sourceDepthFactor * alphaEndScale;
        particleQuad->slantOffset = (int)(((float)(activeParticleCount + 1)) * sourceDepthFactor *
                                          kHudWeatherFxSnowSlantScale);

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

// Reimplements 0x4be810: HudWeatherFxRain::Constructor
// (D:\Proj\Battlesport\hud.cpp)
HudWeatherFxRain * HudWeatherFxRain::Constructor(
    int particleCount
) {
    HudWeatherFx::Constructor(particleCount);
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
    return this;
}

// Reimplements 0x4be850: HudWeatherFxRain::ScalarDeletingDestructor
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

// Reimplements 0x4be870: HudWeatherFxRain::Destructor
// (D:\Proj\Battlesport\hud.cpp)
void HudWeatherFxRain::Destructor() {
    HudWeatherFx::Destructor();
}

// Reimplements 0x4be880: HudWeatherFxRain::Update
// (D:\Proj\Battlesport\hud.cpp)
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
    if (viewportRect != 0) {
        viewportWidth = viewportRect->right - viewportRect->left;
        viewportHeight = viewportRect->bottom - viewportRect->top;
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
        (g_HudWeatherFxRain_LastCameraTargetX - cameraTarget.x) * kHudWeatherFxCameraDriftScale;
    cameraTargetDrift.y =
        (g_HudWeatherFxRain_LastCameraTargetY - cameraTarget.y) * kHudWeatherFxCameraDriftScale;
    cameraTargetDrift.z =
        (g_HudWeatherFxRain_LastCameraTargetZ - cameraTarget.z) * kHudWeatherFxCameraDriftScale;
    g_HudWeatherFxRain_LastCameraTargetX = cameraTarget.x;
    g_HudWeatherFxRain_LastCameraTargetY = cameraTarget.y;
    g_HudWeatherFxRain_LastCameraTargetZ = cameraTarget.z;

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
    const float gravityScale = gravity * kHudWeatherFxForceScale;
    gravityOffset.x = basisVector.x * gravityScale;
    gravityOffset.y = basisVector.y * gravityScale;
    gravityOffset.z = basisVector.z * gravityScale;
    zMath::MatTransformPointBatchInPlace(
        &gravityOffset,
        1
    );

    zVec3 windOffset;
    const float windScale = windVelocity * kHudWeatherFxForceScale;
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
    if (HudWeatherFxVec3LengthSq(&particleVelocity) >= kHudWeatherFxVelocityMaxSq) {
        zMath::Vec3Normalize(&particleVelocity);
    }

    zVec3 probeVelocity = particleVelocity;
    if (HudWeatherFxVec3LengthSq(&probeVelocity) >= kHudWeatherFxProbeVelocityMinSq) {
        zMath::Vec3Normalize(&probeVelocity);
        probeVelocity.x *= kHudWeatherFxProbeScale;
        probeVelocity.y *= kHudWeatherFxProbeScale;
        probeVelocity.z *= kHudWeatherFxProbeScale;
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

        const float sourceDepthFactor = kHudWeatherFxConeDepthMax - sourcePosition->z;
        const float probeDepthFactor = kHudWeatherFxConeDepthMax - probePosition.z;
        HudWeatherFxParticleQuad *const particleQuad = &particleQuads[particleIndex];
        particleQuad->x =
            (int)(((probeDepthFactor * probePosition.x) + kHudWeatherFxProjectionCenter) *
                  viewportWidthF);
        particleQuad->y =
            (int)(((probeDepthFactor * probePosition.y) + kHudWeatherFxProjectionCenter) *
                  viewportHeightF);
        particleQuad->width =
            (int)(((sourceDepthFactor * sourcePosition->x) + kHudWeatherFxProjectionCenter) *
                  viewportWidthF) -
            particleQuad->x;
        particleQuad->height =
            (int)(((sourceDepthFactor * sourcePosition->y) + kHudWeatherFxProjectionCenter) *
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

// Reimplements 0x41c6c0: HudUiNewGamePanelOverlayOwner::QueueEnter
// (D:\Proj\Battlesport\hud.cpp)
void HudUiNewGamePanelOverlayOwner::QueueEnter() {
    g_RecoilApp.QueuePushState(
        &g_HudUiNewGamePanelOverlayOwner,
        0
    );
}

// Reimplements 0x41c5e0: HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void HudUiNewGamePanelOverlayOwner::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x41c5f0: HudUiNewGamePanelOverlayOwner::StaticInit
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
HudUiNewGamePanelOverlayOwner *HudUiNewGamePanelOverlayOwner::StaticInit() {
    return new (&g_HudUiNewGamePanelOverlayOwner) HudUiNewGamePanelOverlayOwner;
}

// Reimplements 0x41c6a0: HudUiNewGamePanelOverlayOwner::RegisterAtExit
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void HudUiNewGamePanelOverlayOwner::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x41c6b0: HudUiNewGamePanelOverlayOwner::AtExitDestructor
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void HudUiNewGamePanelOverlayOwner::AtExitDestructor() {
    g_HudUiNewGamePanelOverlayOwner.~HudUiNewGamePanelOverlayOwner();
}

/**
 * No standalone retail function; the constructor body is inlined into
 * 0x41c5f0 HudUiNewGamePanelOverlayOwner::StaticInit.
 *
 * Purpose: initialize the typed new-game overlay app-state owner.
 */
// Source-faithful helper recovered from address-backed callers in this source file.
HudUiNewGamePanelOverlayOwner::HudUiNewGamePanelOverlayOwner() : m_panel(0) {}

// Reimplements 0x41c630: HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
HudUiNewGamePanelOverlayOwner::~HudUiNewGamePanelOverlayOwner() {
    HudUiNewGamePanel *panel = m_panel;
    if (panel != 0) {
        panel->SetEnabled(0);

        panel = m_panel;
        if (panel != 0) {
            panel->ScalarDeletingDestructor(1);
        }

        m_panel = 0;
    }
}

// Reimplements 0x41c560: HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
int HudUiNewGamePanelOverlayOwner::OnTryBecomeCurrent() {
    HudUiNewGamePanel *panel = (HudUiNewGamePanel *) ::operator new(sizeof(HudUiNewGamePanel));
    if (panel != 0) {
        panel = panel->Constructor();
    }

    m_panel = panel;
    panel->SyncIntensityFromDifficulty();
    panel->SetEnabled(1);
    return 1;
}

// Reimplements 0x41c290: HudUiNewGamePanel::Constructor
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
HudUiNewGamePanel * HudUiNewGamePanel::Constructor() {
    new ((HudUiBackground *)this) HudUiBackground;

    backWidget.Constructor();
    startWidget.Constructor();
    nameInput.BaseConstructor();
    intensity.Constructor();

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
    return this;
}

// Reimplements 0x41c3b0: HudUiNewGamePanel_NameInput::OnActivate
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void HudUiNewGamePanel_NameInput::OnActivate() {
    HudUiNumericTextInput::AllocTextBuffer(21);
    HudUiNumericTextInput::Update(zOpt_GetPlayerName());
    HudUiNumericTextInput::OnActivate();
    HudUiNumericTextInput::SetRawKeyboardCapture(1);
}

// Reimplements 0x41c400: HudUiNewGamePanel::Destructor
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void HudUiNewGamePanel::Destructor() {
    intensity.DestructorCore();
    nameInput.Destructor();
    startWidget.DestructorCore();
    backWidget.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

// Reimplements 0x41c3e0: HudUiNewGamePanel::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
HudUiNewGamePanel * HudUiNewGamePanel::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x41c4e0: HudUiNewGamePanel::SyncIntensityFromDifficulty
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void HudUiNewGamePanel::SyncIntensityFromDifficulty() {
    intensity.SetSelectedIndex(zOpt::GetGameDifficultyMode());
}

// Reimplements 0x41c500: HudUiNewGamePanel::StartGameFromFields
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void HudUiNewGamePanel::StartGameFromFields() {
    HudCheat::ClearNanitePanelCheatSentinel();
    zOpt::SetPlayerName(nameInput.GetBuffer());
    zOpt::SetGameDifficultyMode(intensity.selectedIndex);
    ((HudUiBackgroundContainer *)(&g_RecoilApp.m_missionFmvState))->SetEnabled(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_missionFmvState,
        0
    );
}

// Reimplements 0x41c270: HudUiNewGamePanel_StartButton::OnActivate
// (D:\Proj\Battlesport\HudUiNewGamePanel.cpp)
void HudUiNewGamePanel_StartButton::OnActivate() {
    HudUiNewGamePanel *const panel = (HudUiNewGamePanel *)(owner);
    if (panel != 0) {
        panel->StartGameFromFields();
    }

    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x40d1c0: HudUiOptionsPanelOverlayOwner::QueueEnter
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void HudUiOptionsPanelOverlayOwner::QueueEnter() {
    g_RecoilApp.QueuePushState(
        &g_HudUiOptionsPanelOverlayOwner,
        0
    );
}

// Reimplements 0x40d070: HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void HudUiOptionsPanelOverlayOwner::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x40d080: HudUiOptionsPanelOverlayOwner::StaticInit
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
HudUiOptionsPanelOverlayOwner *HudUiOptionsPanelOverlayOwner::StaticInit() {
    return new (&g_HudUiOptionsPanelOverlayOwner) HudUiOptionsPanelOverlayOwner;
}

// Reimplements 0x40d090: HudUiOptionsPanelOverlayOwner::RegisterAtExit
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void HudUiOptionsPanelOverlayOwner::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x40d0a0: HudUiOptionsPanelOverlayOwner::AtExitDestructor
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
void HudUiOptionsPanelOverlayOwner::AtExitDestructor() {
    g_HudUiOptionsPanelOverlayOwner.~HudUiOptionsPanelOverlayOwner();
}

// Reimplements 0x40d0b0: HudUiOptionsPanelOverlayOwner::HudUiOptionsPanelOverlayOwner
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
HudUiOptionsPanelOverlayOwner::HudUiOptionsPanelOverlayOwner() : m_panel(0) {}

// Reimplements 0x40d0e0: HudUiOptionsPanelOverlayOwner::~HudUiOptionsPanelOverlayOwner
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
HudUiOptionsPanelOverlayOwner::~HudUiOptionsPanelOverlayOwner() {
    HudOptionsDialog *panel = m_panel;
    if (panel != 0) {
        panel->SetEnabled(0);

        panel = m_panel;
        if (panel != 0) {
            panel->ScalarDeletingDestructor(1);
        }

        m_panel = 0;
    }
}

// Reimplements 0x40d150: HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudOptionsDialog.cpp)
int HudUiOptionsPanelOverlayOwner::OnTryBecomeCurrent() {
    HudOptionsDialog *panel = (HudOptionsDialog *) ::operator new(sizeof(HudOptionsDialog));
    if (panel != 0) {
        panel = new (panel) HudOptionsDialog;
    }

    m_panel = panel;

    panel->SetEnabled(1);
    return 1;
}

// Reimplements 0x4159b0: RecoilStateConfirmQuit::QueueEnter
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RecoilStateConfirmQuit::QueueEnter() {
    g_RecoilApp.QueuePushState(
        &g_RecoilState_ConfirmQuit,
        0
    );
}

// Reimplements 0x409160: HudUiCreditsBackButton::OnActivate
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiCreditsBackButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x409180: HudUiCreditsQuitButton::OnActivate
// (D:\Proj\Battlesport\HudUiCreditsPanel.cpp)
void HudUiCreditsQuitButton::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(1);
    g_RecoilApp.m_missionShutdownMode = RECOILAPP_MISSION_SHUTDOWN_SKIP_GAMEPLAY;
    g_RecoilApp.QueueSwitchCurrentState(
        &g_RecoilApp.m_leaveNetworkState,
        0
    );
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x415810: RecoilStateConfirmQuit::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RecoilStateConfirmQuit::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x415820: RecoilStateConfirmQuit::StaticInit
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RecoilStateConfirmQuit *RecoilStateConfirmQuit::StaticInit() {
    return new (&g_RecoilState_ConfirmQuit) RecoilStateConfirmQuit;
}

// Reimplements 0x415830: RecoilStateConfirmQuit::RegisterAtExit
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RecoilStateConfirmQuit::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x415840: RecoilStateConfirmQuit::AtExitDestructor
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RecoilStateConfirmQuit::AtExitDestructor() {
    g_RecoilState_ConfirmQuit.~RecoilStateConfirmQuit();
}

// Reimplements 0x415740: HudUiConfirmQuitOkButton::OnActivate
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
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

// Reimplements 0x415680: HudUiBackgroundConfirmQuit::Constructor
// (D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp)
HudUiBackgroundConfirmQuit * HudUiBackgroundConfirmQuit::Constructor() {
    new ((HudUiBackground *)this) HudUiBackground;
    okButton.Constructor();
    cancelButton.Constructor();

    zReader::Node *const dialogRoot = HudUiBackground::LoadFromZrd(
        "dialog.zrd",
        "CONFIRM_QUIT",
        0
    );
    if (dialogRoot != 0) {
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &okButton,
            "OK_TO_QUIT"
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &cancelButton,
            "CANCEL_QUIT"
        );
        HudUiBackground::FreeLoadedTreeRoots((int)dialogRoot);
    }

    return this;
}

// Reimplements 0x4157b0: HudUiBackgroundConfirmQuit::Destructor
// (D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp)
void HudUiBackgroundConfirmQuit::Destructor() {
    cancelButton.DestructorCore();
    okButton.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

// Reimplements 0x415790: HudUiBackgroundConfirmQuit::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiBackgroundConfirmQuit.cpp)
HudUiBackgroundConfirmQuit * HudUiBackgroundConfirmQuit::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x4070e0: HudUiCheatTextInputWidget::OnActivate
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
void HudUiCheatTextInputWidget::OnActivate() {
    g_RecoilApp.QueueExitCurrentState(0);
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x406d20: HudUiCheatCodeDialog::Constructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
HudUiCheatCodeDialog * HudUiCheatCodeDialog::Constructor() {
    new ((HudUiBackground *)this) HudUiBackground;

    titleWidget.Constructor();

    cheatInputWidget.BaseConstructor();
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

    return this;
}

// Reimplements 0x406e30: HudUiCheatCodeDialog::Destructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
void HudUiCheatCodeDialog::Destructor() {
    cheatInputWidget.Destructor();
    titleWidget.DestructorCore();
    this->HudUiBackground::~HudUiBackground();
}

// Reimplements 0x406e10: HudUiCheatCodeDialog::ScalarDeletingDestructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
HudUiCheatCodeDialog * HudUiCheatCodeDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();

    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x406e90: RecoilStateCheatCode::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
void RecoilStateCheatCode::StaticInitAndRegisterAtExit() {
    ConstructGlobal();
    StaticInit();
}

// Reimplements 0x406ea0: RecoilStateCheatCode::ConstructGlobal
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RecoilStateCheatCode *RecoilStateCheatCode::ConstructGlobal() {
    return new (&g_RecoilStateCheatCode) RecoilStateCheatCode;
}

// Reimplements 0x406eb0: RecoilStateCheatCode::StaticInit
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
void RecoilStateCheatCode::StaticInit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x406ec0: RecoilStateCheatCode::AtExitDestructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
void RecoilStateCheatCode::AtExitDestructor() {
    g_RecoilStateCheatCode.~RecoilStateCheatCode();
}

// Reimplements 0x406ed0: RecoilStateCheatCode::RecoilStateCheatCode
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RecoilStateCheatCode::RecoilStateCheatCode() : m_dialog(0) {}

// Reimplements 0x406f60: RecoilStateCheatCode::OnTryBecomeCurrent
// (D:\Proj\Battlesport\RecoilStateCheatCode.cpp)
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

    zSndSampleSet_InitByName("DIALOG");

    HudUiCheatCodeDialog *dialog =
        (HudUiCheatCodeDialog *) ::operator new(sizeof(HudUiCheatCodeDialog));
    if (dialog != 0) {
        dialog = dialog->Constructor();
    }
    m_dialog = dialog;

    dialog->SetEnabled(1);
    return 1;
}

// Reimplements 0x407010: RecoilStateCheatCode::OnDeactivate
// (D:\Proj\Battlesport\RecoilStateCheatCode.cpp)
void RecoilStateCheatCode::OnDeactivate() {
    CString commandString;

    HudUiCheatCodeDialog *dialog = m_dialog;
    if (dialog != 0) {
        commandString = dialog->cheatInputWidget.GetBuffer();

        zVideo::RunPostprocessOnPrimaryBuffer();

        dialog->SetEnabled(0);

        ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
        zVideo::Dispatch_UnlockPrimarySurfaceState();

        dialog = m_dialog;
        if (dialog != 0) {
            dialog->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }

    zSndSampleSet_DestroyByName("DIALOG");

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

// Reimplements 0x406f00: RecoilStateCheatCode::Destructor
// (D:\Proj\Battlesport\HudUiCheatCode.cpp)
RecoilStateCheatCode::~RecoilStateCheatCode() {
    HudUiCheatCodeDialog *dialog = m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

// Reimplements 0x415850: RecoilStateConfirmQuit::RecoilStateConfirmQuit
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
RecoilStateConfirmQuit::RecoilStateConfirmQuit() : m_dialog(0) {}

// Reimplements 0x4158f0: RecoilStateConfirmQuit::OnTryBecomeCurrent
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
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

// Reimplements 0x415960: RecoilStateConfirmQuit::OnDeactivate
// (D:\Proj\Battlesport\HudConfirmQuitDialog.cpp)
void RecoilStateConfirmQuit::OnDeactivate() {
    if (m_dialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    HudUiBackgroundConfirmQuit *dialog = m_dialog;
    dialog->SetEnabled(0);

    ((HudUiDialogController *)m_dialog)->BlitOwnedSurfaceToPrimary();
    zVideo::Dispatch_UnlockPrimarySurfaceState();

    dialog = m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
    Sleep(1000);
}

// Reimplements 0x415880: RecoilStateConfirmQuit::~RecoilStateConfirmQuit
// (D:\Proj\Battlesport\RecoilStateConfirmQuit.cpp)
RecoilStateConfirmQuit::~RecoilStateConfirmQuit() {
    HudUiBackgroundConfirmQuit *dialog = m_dialog;
    if (dialog != 0) {
        dialog->SetEnabled(0);

        dialog = m_dialog;
        if (dialog != 0) {
            dialog->ScalarDeletingDestructor(1);
        }

        m_dialog = 0;
    }
}

// Reimplements 0x408d20: RecoilStateControls::StaticInitAndRegisterAtExit
// (D:\Proj\Battlesport\recoil_state.cpp)
void RecoilStateControls::StaticInitAndRegisterAtExit() {
    StaticInit();
    RegisterAtExit();
}

// Reimplements 0x408d30: RecoilStateControls::StaticInit
// (D:\Proj\Battlesport\recoil_state.cpp)
RecoilStateControls *RecoilStateControls::StaticInit() {
    return new (&g_RecoilStateControls) RecoilStateControls;
}

// Reimplements 0x408d40: RecoilStateControls::RegisterAtExit
// (D:\Proj\Battlesport\recoil_state.cpp)
void RecoilStateControls::RegisterAtExit() {
    atexit(AtExitDestructor);
}

// Reimplements 0x408d50: RecoilStateControls::AtExitDestructor
// (D:\Proj\Battlesport\recoil_state.cpp)
void RecoilStateControls::AtExitDestructor() {
    g_RecoilStateControls.~RecoilStateControls();
}

// Reimplements 0x408d60: RecoilStateControls::RecoilStateControls
// (D:\Proj\Battlesport\recoil_state.cpp)
RecoilStateControls::RecoilStateControls() : m_dialog(0) {}

// Reimplements 0x408d90: RecoilStateControls::Destructor
// (D:\Proj\Battlesport\recoil_state.cpp)
RecoilStateControls::~RecoilStateControls() {
    HudUiControlsDialog *dialog = m_dialog;
    if (dialog != 0) {
        dialog->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

// Reimplements 0x408df0: RecoilStateControls::OnTryBecomeCurrent
// (D:\Proj\Battlesport\recoil_state.cpp)
int RecoilStateControls::OnTryBecomeCurrent() {
    if (m_dialog == 0) {
        HudUiControlsDialog *dialog =
            (HudUiControlsDialog *) ::operator new(sizeof(HudUiControlsDialog));
        if (dialog != 0) {
            dialog = dialog->Constructor();
        }
        m_dialog = dialog;
    }

    HudUiControlsDialog *const dialog = m_dialog;
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

// Reimplements 0x408ec0: RecoilStateControls::OnDeactivate
// (D:\Proj\Battlesport\recoil_state.cpp)
void RecoilStateControls::OnDeactivate() {
    if (m_dialog == 0) {
        return;
    }

    HudUiControlsDialog *const dialog = m_dialog;
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

    HudUiControlsDialog *dialogToDelete = m_dialog;
    if (dialogToDelete != 0) {
        dialogToDelete->ScalarDeletingDestructor(1);
    }

    m_dialog = 0;
}

// Reimplements 0x408fa0: RecoilStateControls::OnResume
// (D:\Proj\Battlesport\recoil_state.cpp)
void RecoilStateControls::OnResume(
    int activateCode
) {
    (void)activateCode;

    if (m_dialog == 0) {
        return;
    }

    zVideo::RunPostprocessOnPrimaryBuffer();

    HudUiControlsDialog *const dialog = m_dialog;
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

// Reimplements 0x408ff0: RecoilStateControls::QueueEnter
// (D:\Proj\Battlesport\recoil_state.cpp)
void RecoilStateControls::QueueEnter() {
    g_RecoilApp.QueuePushState(
        &g_RecoilStateControls,
        0
    );
}

// Reimplements 0x408c20: HudUiControlsDialog_CommandsWidget::OnActivate
// (D:\Proj\Battlesport\hud_ui_dialogs.cpp)
void HudUiControlsDialog_CommandsWidget::OnActivate() {
    HudCmdDialogState::QueueEnter();
    HudUiZrdWidget::OnActivate();
}

// Reimplements 0x408a30: HudUiControlsDialog::Constructor
// (D:\Proj\Battlesport\hud_ui_dialogs.cpp)
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
        "CONTROLS_DIALOG",
        0
    );
    if (dialogRoot != 0) {
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &resumeWidget,
            "RESUME"
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &commandsWidget,
            "COMMANDS_BTN"
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &mouseOrJoystickSelector,
            "MOUSE_OR_JOYSTICK"
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &throttleModeSelector,
            "THROTTLE_MODE"
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &steeringModeSelector,
            "STEERING_MODE"
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &cursorModeSelector,
            "CURSOR_MODE"
        );
        HudUiBackground::BindWidgetByName(
            dialogRoot,
            &cameraModeSelector,
            "CAMERA_MODE"
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

// Reimplements 0x408c70: HudUiControlsDialog::Destructor
// (D:\Proj\Battlesport\hud_ui_dialogs.cpp)
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

// Reimplements 0x408c40: HudUiControlsDialog::ScalarDeletingDestructor
// (D:\Proj\Battlesport\hud_ui_dialogs.cpp)
HudUiControlsDialog * HudUiControlsDialog::ScalarDeletingDestructor(
    unsigned int flags
) {
    Destructor();
    if ((flags & 1u) != 0) {
        ::operator delete(this);
    }

    return this;
}

// Reimplements 0x407100: HudUiCallback::QueueExitCurrentState
// (D:\Proj\Battlesport\hud.cpp)
void HudUiCallback::QueueExitCurrentState() {
    g_RecoilApp.QueueExitCurrentState(0);
}

// Reimplements 0x407110: HudUiCallback::QueueCheatCodeState
// (D:\Proj\Battlesport\hud.cpp)
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

// Reimplements 0x406af0: HudCheat::ExecuteCommandString
// (D:\Proj\Battlesport\hud.cpp)
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

// Reimplements 0x406cf0: HudCheat::ClearNanitePanelCheatSentinel
// (D:\Proj\Battlesport\hud.cpp)
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

// Reimplements 0x413600: zOpt::ToggleHudTypeForCurrentHwMode
// (D:\Proj\Battlesport\hud.cpp)
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

// Reimplements 0x439b20: HudLowMeterLoopSound::SetLoopActive
// (D:\Proj\Battlesport\Hud.cpp)
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

// Reimplements 0x439b70: HudLowMeterLoopSound::Disable
// (D:\Proj\Battlesport\Hud.cpp)
void Disable() {
    g_Hud_LowMeterBeepSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopSample->StopActiveVoicesIfPlaying();
    g_Hud_LowMeterLoopActive = 0;
}

} // namespace HudLowMeterLoopSound
