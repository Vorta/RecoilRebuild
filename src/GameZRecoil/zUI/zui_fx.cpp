// zUI compilation unit for the pass-3 element/slot, HudWeatherFx and the
// zVideoFxPass3Config singleton and widgets, inferred from the retail object
// boundary at 0x4bdb60: its pooled .rdata constants at 0x4d3e68 duplicate
// zui.cpp's own, its .bss run holds the 0x1f0-byte pass-3 singleton followed
// by the weather variables, and its single CRT initializer (0x4da0b0 ->
// 0x4bee40) constructs that singleton. Original filename unresolved;
// zui_fx.cpp is a provisional name (2026-10-02).

#include "recoil/Mfc42Abi.h"

#include "GameZRecoil/zHud/zhud_ui.h"

#include "Battlesport/game_net.h"
#include "Battlesport/hud.h"
#include "Battlesport/hud_sensor_tracker.h"
#include "Battlesport/hud_ui_net_game_setup.h"
#include "Battlesport/recoil_state_credits.h"
#include "GameZRecoil/include/opt_catalog.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zFMV/fmv.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zRender/zrndr.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zVideo/zvid_fx_pass3.h"

#include "Battlesport/turret.h"
#include "GameZRecoil/zSys/zsys.h"

#include <cctype>
#include <cstdarg>
#include <math.h>
#include <new>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3element-draw
 * @recoil-artifact defines .text recoil:function:0x4bdb60: zVideoFxPass3Element::Draw.
 * @recoil-match byte
 *
 * Draws the common HUD base, publishes the parent pass-3 source surface, then dispatches the
 * element-specific pass callback once for each configured input rectangle.
 * Purpose: provide the recovered zVideoFxPass3Element::Draw behavior.
 */
void zVideoFxPass3Element::Draw()
{
    zVideoFxPass3Config* const parentConfig = (zVideoFxPass3Config*)(parent);
    DrawBase();

    if (parentConfig != 0) {
        if (parentConfig->surfacePixels != 0) {
            zVideo::FxSetSurfaceState(
                parentConfig->surfacePixels,
                parentConfig->surfaceWidth,
                parentConfig->surfaceHeight,
                parentConfig->surfacePitchBytes
            );
        }

        int index;
        for (index = 0; index < 2; ++index) {
            HudUiRect* const inputRect = parentConfig->inputRectsOrNull[index];
            if (inputRect != 0) {
                clipRectOrNull = inputRect;
                ApplyPass3();
            }
        }

        clipRectOrNull = parentConfig->inputRectsOrNull[0];
    } else {
        ApplyPass3();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3rootelement-applypass3
 * @recoil-artifact defines .text recoil:function:0x4bdbc0: zVideoFxPass3RootElement::ApplyPass3.
 * @recoil-match byte
 *
 * Root pass-3 callback submits the currently selected input rectangle as a framebuffer overlay
 * using the root element's recovered color and alpha.
 * Purpose: provide the recovered zVideoFxPass3RootElement::ApplyPass3 behavior.
 */
void zVideoFxPass3RootElement::ApplyPass3()
{
    zRndrOverlayRectSubmit((unsigned int)(packedColor16), alpha, (zVidRect32*)(clipRectOrNull));
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3slot-zvideofxpass3slot
 * @recoil-artifact defines .text recoil:function:0x4bdbe0: zVideoFxPass3Slot::Constructor.
 * @recoil-match byte
 *
 * Constructs the pass-3 slot element and clears the input clip consumed by
 * Purpose: provide the recovered zVideoFxPass3Slot constructor behavior.
 */
zVideoFxPass3Slot::zVideoFxPass3Slot()
    : zVideoFxPass3Element(0, 0)
{
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3slot-setrectandpayload
 * @recoil-artifact defines .text recoil:function:0x4bdc00: zVideoFxPass3Slot::SetRectAndPayload.
 * @recoil-match byte
 *
 * Purpose: provide the recovered zVideoFxPass3Slot::SetRectAndPayload behavior.
 */
void zVideoFxPass3Slot::SetRectAndPayload(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreqValue,
    float sinPhaseValue
)
{
    SetPos(rectLeftPixels, rectTopPixels);

    currentRadius = currentRadiusPixels;
    maxRadius = maxRadiusPixels;
    extent = extentPixels;
    sinFreq = sinFreqValue;
    sinPhase = sinPhaseValue;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-zvideofxpass3slot-applypass3
 * @recoil-artifact defines .text recoil:function:0x4bdc40: zVideoFxPass3Slot::ApplyPass3.
 * @recoil-match byte
 *
 * The pass callback forwards the slot position, integer radius payload, sine parameters, and
 * active input clip to the shared pass-3 radial warp routine.
 * Purpose: provide the recovered zVideoFxPass3Slot::ApplyPass3 behavior.
 */
void zVideoFxPass3Slot::ApplyPass3()
{
    zVideo::FxPass3ApplyToCurrentSurface(
        x,
        y,
        currentRadius,
        maxRadius,
        extent,
        sinFreq,
        sinPhase,
        (zVidRect32*)(clipRectOrNull)
    );
}

namespace {
const int kHudWeatherFxRainSlantDelta = 1;
const int kHudWeatherFxSnowTextureWidth = 16;
const int kHudWeatherFxSnowTextureHeight = 8;
const int kHudWeatherFxSnowTextureTexels = kHudWeatherFxSnowTextureWidth * kHudWeatherFxSnowTextureHeight;

/**
 * Original inline helper; no standalone retail function exists.
 * Observed callers: 0x4be2f0 HudWeatherFxSnow::Update and 0x4be880 HudWeatherFxRain::Update.
 * Purpose: Compute a weather particle velocity vector's squared length before normalization.
 */
inline float HudWeatherFxVec3LengthSq(const zVec3* value)
{
    return value->x * value->x + value->y * value->y + value->z * value->z;
}

/**
 * Original inline helper; no standalone retail function exists.
 * Observed callers: 0x4be2f0 HudWeatherFxSnow::Update.
 * Purpose: Decide whether a snow particle left the visible weather cone and must respawn.
 */
inline int HudWeatherFxSnowNeedsReset(const zVec3* position)
{
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-g-hudweatherfxsnow-lastcameratarget
 * @recoil-artifact defines .data recoil:data:0x56bf48: g_HudWeatherFxSnow_LastCameraTarget.
 * Purpose: Retain the previous snow camera target coordinates for frame-to-frame drift.
 */
HudWeatherFxCameraTargetHistory g_HudWeatherFxSnow_LastCameraTarget = { 0.0f, 0.0f, 0.0f, 0.0f };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-g-hudweatherfxrain-lastcameratarget
 * @recoil-artifact defines .data recoil:data:0x56bf58: g_HudWeatherFxRain_LastCameraTarget.
 * Purpose: Retain the previous rain camera target coordinates for frame-to-frame drift.
 */
HudWeatherFxCameraTargetHistory g_HudWeatherFxRain_LastCameraTarget = { 0.0f, 0.0f, 0.0f, 0.0f };
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-f-0x56bf68
 * @recoil-artifact defines .data recoil:data:0x56bf68: g_HudWeatherFxSnow_TimeAccumulator.
 * Purpose: Accumulate elapsed snow update time.
 */
float g_HudWeatherFxSnow_TimeAccumulator = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-f-0x56bf6c
 * @recoil-artifact defines .data recoil:data:0x56bf6c: g_HudWeatherFxRain_TimeAccumulator.
 * Purpose: Accumulate elapsed rain update time.
 */
float g_HudWeatherFxRain_TimeAccumulator = 0.0f;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfx-hudweatherfx-0x4bdc70
 * @recoil-artifact defines .text recoil:function:0x4bdc70: HudWeatherFx::HudWeatherFx(int).
 * @recoil-match byte
 *
 * Purpose: Initialize the base weather particle emitter, allocate particle buffers, reset
 * particles, and create the hardware SnowFX texture resources when needed.
 */
HudWeatherFx::HudWeatherFx(int newParticleCount)
    : zVideoFxPass3Element(0, 0)
{
    maxParticles = newParticleCount;
    particleCount = newParticleCount;
    particleQuads = (HudWeatherFxParticleQuad*)(::operator new(sizeof(HudWeatherFxParticleQuad) * newParticleCount));

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
    particlePositions[sourceBufferIndex] = (zVec3*)(::operator new(positionBytes));
    particlePositions[destBufferIndex] = (zVec3*)(::operator new(positionBytes));

    for (int resetIndex = 0; resetIndex < newParticleCount; ++resetIndex) {
        ResetParticleSlot(resetIndex, 1);
    }

    basisVector.x = 0.0f;
    basisVector.y = 1.0f;
    basisVector.z = 0.0f;
    gravity = 1.0f;
    windDirection = 0.0f;
    windVelocity = 1.0f;

    if (g_zVideo_ActiveRendererPath != 0) {
        textureName = "SnowFX";
        softwareImage = zVid_Image::Create();
        zVid_Image::SetFormatCode(softwareImage, 0x0b);
        zVidImageSetPixels(
            softwareImage,
            malloc(kHudWeatherFxSnowTextureTexels * sizeof(unsigned short)),
            (char*)(malloc(kHudWeatherFxSnowTextureTexels))
        );
        softwareImage->formatFlagsPacked |= 0x20;
        zVid_Image::SetSize(softwareImage, kHudWeatherFxSnowTextureWidth, kHudWeatherFxSnowTextureHeight);
        textureRecord = g_zVideo_pfnCreateTextureRecord(
            textureName,
            softwareImage,
            softwareImage->formatFlagsPacked & 0x02,
            1,
            1
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfx-hudweatherfx-0x4bde40
 * @recoil-artifact defines .text recoil:function:0x4bde40: HudWeatherFx::~HudWeatherFx.
 * @recoil-match byte
 *
 * Purpose: Release particle buffers and renderer-backed weather texture resources.
 */
HudWeatherFx::~HudWeatherFx()
{
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
            softwareImage = zVid_Image::ReleaseIfNotDefault(softwareImage);
        }
    }
}

/**

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfx-resetparticleslot
 * @recoil-artifact defines .text recoil:function:0x4bdee0: HudWeatherFx::ResetParticleSlot.
 * @recoil-match byte
 *
 * Purpose: Respawn one particle in the weather cone and copy it into the destination buffer.
 */
void HudWeatherFx::ResetParticleSlot(int particleIndex, int)
{
    zVec3* const sourcePosition = &particlePositions[sourceBufferIndex][particleIndex];
    zVec3* const destPosition = &particlePositions[destBufferIndex][particleIndex];

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
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfx-applypass3
 * @recoil-artifact defines .text recoil:function:0x4bdfd0: HudWeatherFx::ApplyPass3.
 *
 *
 * Purpose: Draw software weather lines or submit hardware textured weather quads
 * through the pass-3 HUD element callback.
 */
void HudWeatherFx::ApplyPass3()
{
    if (g_zVideo_ActiveRendererPath != ZVID_RENDERER_BACKEND_SOFTWARE) {
        const int swSurfaceWasLocked = zVideo::GetSwSurfaceLockedFlag();
        if (swSurfaceWasLocked != 0) {
            zVideo::DispatchUnlockSwSurfaceState();
        }

        unsigned short* surfacePixels = (unsigned short*)(softwareImage->pixels);
        if (*surfacePixels != packedColor16) {
            char* surfaceAlphaMap = softwareImage->alphaMap;
            for (int alphaValue = 0; alphaValue < 4080; alphaValue += 255) {
                *surfacePixels++ = packedColor16;
                *surfaceAlphaMap++ = (char)(alphaValue / 16);
            }

            g_zVideo_pfnTextureRecordFinalizeUpload(textureRecord, 0, softwareImage);
        }

        zVideoD3D::SceneEnter();

        for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
            float xSlant;
            float ySlant;
            if (particleQuads[particleIndex].width > particleQuads[particleIndex].height) {
                xSlant = (float)(particleQuads[particleIndex].slantOffset);
                ySlant = 0.0f;
            } else {
                xSlant = 0.0f;
                ySlant = (float)(particleQuads[particleIndex].slantOffset);
            }

            zVideo_XyzVertex clipVerts[4];
            zVideo_TexCoord texCoords[4];
            clipVerts[0].x = (float)(particleQuads[particleIndex].x);
            clipVerts[0].y = (float)(particleQuads[particleIndex].y);
            clipVerts[0].z = particlePositions[sourceBufferIndex][particleIndex].z;
            texCoords[0].u = particleQuads[particleIndex].texCoordUStart;
            texCoords[0].v = 0.0f;

            clipVerts[1].x = (float)(particleQuads[particleIndex].x) + xSlant;
            clipVerts[1].y = (float)(particleQuads[particleIndex].y) + ySlant;
            clipVerts[1].z = particlePositions[sourceBufferIndex][particleIndex].z;
            texCoords[1].u = particleQuads[particleIndex].texCoordUStart;
            texCoords[1].v = 0.0f;

            clipVerts[2].x = (float)(particleQuads[particleIndex].x + particleQuads[particleIndex].width) + xSlant;
            clipVerts[2].y = (float)(particleQuads[particleIndex].y + particleQuads[particleIndex].height) + ySlant;
            clipVerts[2].z = particlePositions[sourceBufferIndex][particleIndex].z;
            texCoords[2].u = particleQuads[particleIndex].texCoordUEnd;
            texCoords[2].v = 0.0f;

            clipVerts[3].x = (float)(particleQuads[particleIndex].x + particleQuads[particleIndex].width);
            clipVerts[3].y = (float)(particleQuads[particleIndex].y + particleQuads[particleIndex].height);
            clipVerts[3].z = particlePositions[sourceBufferIndex][particleIndex].z;
            texCoords[3].u = particleQuads[particleIndex].texCoordUEnd;
            texCoords[3].v = 0.0f;

            if (((HudWeatherFxPointBatch*)(clipVerts))->ArePointBatchInsideRect(4, clipRectOrNull) != 0) {
                g_zVideo_pfnSubmitPolyRenderClass(
                    clipVerts,
                    texCoords,
                    4,
                    (zVideo_RenderClass*)(textureRecord),
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
    } else {
        zVideo_FxSurface::DrawColoredLinesBatch(
            (zVideoFxColoredLineRecord*)(particleQuads),
            particleCount,
            (zVidRect32*)(clipRectOrNull)
        );
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxpointbatch-arepointbatchinsiderect
 * @recoil-artifact defines .text recoil:function:0x4be210: HudWeatherFx::ArePointBatchInsideRect.
 * @recoil-match byte
 *
 * Purpose: Accept a projected weather quad only when all points lie inside the viewport.
 */
int HudWeatherFxPointBatch::ArePointBatchInsideRect(int pointCount, const HudUiRect* viewportRect)
{
    if (viewportRect == 0) {
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxsnow-hudweatherfxsnow-0x4be280
 * @recoil-artifact defines .text recoil:function:0x4be280: HudWeatherFxSnow::HudWeatherFxSnow(int).
 * @recoil-match byte
 *
 * Purpose: Construct the shared weather emitter and initialize snow emitter defaults.
 */
HudWeatherFxSnow::HudWeatherFxSnow(int particleCount)
    : HudWeatherFx(particleCount)
{
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxsnow-update
 * @recoil-artifact defines .text recoil:function:0x4be2f0: HudWeatherFxSnow::Update.
 *
 *
 * Purpose: Advance snow particles from camera drift, gravity, and wind, then project quads.
 */
void HudWeatherFxSnow::Update(float deltaSeconds)
{
    if ((~flags & 0x10) == 0) {
        return;
    }

    g_HudWeatherFxSnow_TimeAccumulator += deltaSeconds;
    if (camera == 0) {
        return;
    }

    int viewportWidth;
    int viewportHeight;
    if (clipRectOrNull != 0) {
        viewportWidth = clipRectOrNull->right - clipRectOrNull->left;
        viewportHeight = clipRectOrNull->bottom - clipRectOrNull->top;
    } else {
        // Retail discards the primary-surface rect here; the viewport size stays unset.
        zVideo::GetPrimarySurfaceRectScratch();
    }

    zVec3 cameraTarget;
    CZCamera::gwCameraGetPosition(camera, &cameraTarget.x, &cameraTarget.y, &cameraTarget.z);

    zVec3 cameraAngles;
    CZCamera::gwCameraGetEulerAngles(camera, &cameraAngles.x, &cameraAngles.y, &cameraAngles.z);

    zVec3 cameraTargetDrift;
    zMath::Vec3Subtract((const zVec3*)(&g_HudWeatherFxSnow_LastCameraTarget), &cameraTarget, &cameraTargetDrift);
    cameraTargetDrift.x *= -0.100000001f;
    cameraTargetDrift.y *= -0.100000001f;
    cameraTargetDrift.z *= -0.100000001f;
    g_HudWeatherFxSnow_LastCameraTarget.x = cameraTarget.x;
    g_HudWeatherFxSnow_LastCameraTarget.y = cameraTarget.y;
    g_HudWeatherFxSnow_LastCameraTarget.z = cameraTarget.z;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateX(-cameraAngles.x);
    zMath::MatRotateY(-cameraAngles.y);
    zMath::MatTransformPointBatchInPlace(&cameraTargetDrift, 1);
    zMath::MatStackPopPtr();

    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateZ(cameraAngles.z);
    zMath::MatRotateY(cameraAngles.y);
    zMath::MatRotateX(cameraAngles.x);

    // Retail keeps this float scale on the x87 stack and multiplies each component in memory.
    const float gravityScale = gravity * 0.1;
    zVec3 gravityOffset = basisVector;
    gravityOffset.x *= gravityScale;
    gravityOffset.y *= gravityScale;
    gravityOffset.z *= gravityScale;
    zMath::MatTransformPointBatchInPlace(&gravityOffset, 1);

    zVec3 windOffset;
    windOffset.x = (float)(sin(windDirection) * windVelocity * 0.1);
    windOffset.y = 0.0f;
    windOffset.z = (float)(cos(windDirection) * windVelocity * 0.1);
    zMath::MatTransformPointBatchInPlace(&windOffset, 1);
    zMath::MatStackPopPtr();

    zVec3 particleVelocity;
    zMath::Vec3Add(&gravityOffset, &cameraTargetDrift, &particleVelocity);
    zMath::Vec3Add(&windOffset, &particleVelocity, &particleVelocity);
    float lengthSq;
    ZMTH_VECTOR_LENGTH_SQ(lengthSq, &particleVelocity);
    if (lengthSq > 1.0) {
        zMath::Vec3Normalize(&particleVelocity);
    }

    zVec3 probeVelocity = particleVelocity;
    ZMTH_VECTOR_LENGTH_SQ(lengthSq, &probeVelocity);
    if (lengthSq > 0.010000000000000002) {
        zMath::Vec3Normalize(&probeVelocity);
        probeVelocity.x *= 0.100000001f;
        probeVelocity.y *= 0.100000001f;
        probeVelocity.z *= 0.100000001f;
    }

    for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        zMath::Vec3Add(
            &particlePositions[sourceBufferIndex][particleIndex],
            &particleVelocity,
            &particlePositions[destBufferIndex][particleIndex]
        );

        zVec3 probePosition;
        zMath::Vec3Add(&particlePositions[sourceBufferIndex][particleIndex], &probeVelocity, &probePosition);

        const float sourceDepthFactor = 1.5f - particlePositions[sourceBufferIndex][particleIndex].z;
        const float probeDepthFactor = 1.5f - probePosition.z;
        particleQuads[particleIndex].x = (int)(((probeDepthFactor * probePosition.x) - -0.5f) * (float)(viewportWidth));
        particleQuads[particleIndex].y
            = (int)(((probeDepthFactor * probePosition.y) - -0.5f) * (float)(viewportHeight));
        particleQuads[particleIndex].width
            = (int)(((sourceDepthFactor * particlePositions[sourceBufferIndex][particleIndex].x) - -0.5f)
                  * (float)(viewportWidth))
            - particleQuads[particleIndex].x;
        particleQuads[particleIndex].height
            = (int)(((sourceDepthFactor * particlePositions[sourceBufferIndex][particleIndex].y) - -0.5f)
                  * (float)(viewportHeight))
            - particleQuads[particleIndex].y;
        particleQuads[particleIndex].color16 = packedColor16;
        particleQuads[particleIndex].texCoordUStart = probeDepthFactor * alphaStartScale;
        particleQuads[particleIndex].texCoordUEnd = sourceDepthFactor * alphaEndScale;
        particleQuads[particleIndex].slantOffset = (int)(((float)(activeParticleCount + 1)) * sourceDepthFactor * 3.5);

        if (HudWeatherFxSnowNeedsReset(&particlePositions[destBufferIndex][particleIndex]) != 0) {
            ResetParticleSlot(particleIndex, 0);
        }
    }

    HudUiElement::Update(deltaSeconds);

    const int oldSourceBufferIndex = sourceBufferIndex;
    sourceBufferIndex = destBufferIndex;
    destBufferIndex = oldSourceBufferIndex;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxrain-hudweatherfxrain-0x4be810
 * @recoil-artifact defines .text recoil:function:0x4be810: HudWeatherFxRain::HudWeatherFxRain(int).
 * @recoil-match byte
 *
 * Purpose: Construct the shared weather emitter and initialize rain emitter defaults.
 */
HudWeatherFxRain::HudWeatherFxRain(int particleCount)
    : HudWeatherFx(particleCount)
{
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxrain-hudweatherfxrain-0x4be870
 * @recoil-artifact defines .text recoil:function:0x4be870: HudWeatherFxRain::~HudWeatherFxRain.
 * @recoil-match byte
 *
 * Purpose: Tear down the rain emitter and continue through the shared C++ base destructor.
 */
HudWeatherFxRain::~HudWeatherFxRain() { }

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-hudweatherfxrain-update
 * @recoil-artifact defines .text recoil:function:0x4be880: HudWeatherFxRain::Update.
 *
 *
 * Purpose: Advance rain particles from camera drift, gravity, and wind, then project quads.
 */
void HudWeatherFxRain::Update(float deltaSeconds)
{
    if ((~flags & 0x10) == 0) {
        return;
    }

    g_HudWeatherFxRain_TimeAccumulator += deltaSeconds;
    if (camera == 0) {
        return;
    }

    int viewportWidth;
    int viewportHeight;
    if (clipRectOrNull != 0) {
        viewportWidth = clipRectOrNull->right - clipRectOrNull->left;
        viewportHeight = clipRectOrNull->bottom - clipRectOrNull->top;
    } else {
        // Retail discards the primary-surface rect here; the viewport size stays unset.
        zVideo::GetPrimarySurfaceRectScratch();
    }

    zVec3 cameraTarget;
    CZCamera::gwCameraGetPosition(camera, &cameraTarget.x, &cameraTarget.y, &cameraTarget.z);

    zVec3 cameraAngles;
    CZCamera::gwCameraGetEulerAngles(camera, &cameraAngles.x, &cameraAngles.y, &cameraAngles.z);

    zVec3 cameraTargetDrift;
    zMath::Vec3Subtract((const zVec3*)(&g_HudWeatherFxRain_LastCameraTarget), &cameraTarget, &cameraTargetDrift);
    cameraTargetDrift.x *= -0.100000001f;
    cameraTargetDrift.y *= -0.100000001f;
    cameraTargetDrift.z *= -0.100000001f;
    g_HudWeatherFxRain_LastCameraTarget.x = cameraTarget.x;
    g_HudWeatherFxRain_LastCameraTarget.y = cameraTarget.y;
    g_HudWeatherFxRain_LastCameraTarget.z = cameraTarget.z;

    zMat4x3 slotBuffer;
    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateX(-cameraAngles.x);
    zMath::MatRotateY(-cameraAngles.y);
    zMath::MatTransformPointBatchInPlace(&cameraTargetDrift, 1);
    zMath::MatStackPopPtr();

    zMath::MatStackPushPtr((float*)(&slotBuffer));
    zMath::MatLoadIdentity();
    zMath::MatRotateZ(cameraAngles.z);
    zMath::MatRotateY(cameraAngles.y);
    zMath::MatRotateX(cameraAngles.x);

    // Retail keeps this float scale on the x87 stack and multiplies each component in memory.
    const float gravityScale = gravity * 0.1;
    zVec3 gravityOffset = basisVector;
    gravityOffset.x *= gravityScale;
    gravityOffset.y *= gravityScale;
    gravityOffset.z *= gravityScale;
    zMath::MatTransformPointBatchInPlace(&gravityOffset, 1);

    zVec3 windOffset;
    windOffset.x = (float)(sin(windDirection) * windVelocity * 0.1);
    windOffset.y = 0.0f;
    windOffset.z = (float)(cos(windDirection) * windVelocity * 0.1);
    zMath::MatTransformPointBatchInPlace(&windOffset, 1);
    zMath::MatStackPopPtr();

    zVec3 particleVelocity;
    zMath::Vec3Add(&gravityOffset, &cameraTargetDrift, &particleVelocity);
    zMath::Vec3Add(&windOffset, &particleVelocity, &particleVelocity);
    float lengthSq;
    ZMTH_VECTOR_LENGTH_SQ(lengthSq, &particleVelocity);
    if (lengthSq > 1.0) {
        zMath::Vec3Normalize(&particleVelocity);
    }

    zVec3 probeVelocity = particleVelocity;
    ZMTH_VECTOR_LENGTH_SQ(lengthSq, &probeVelocity);
    if (lengthSq > 0.010000000000000002) {
        zMath::Vec3Normalize(&probeVelocity);
        probeVelocity.x *= 0.100000001f;
        probeVelocity.y *= 0.100000001f;
        probeVelocity.z *= 0.100000001f;
    }

    for (int particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        zMath::Vec3Add(
            &particlePositions[sourceBufferIndex][particleIndex],
            &particleVelocity,
            &particlePositions[destBufferIndex][particleIndex]
        );

        zVec3 probePosition;
        zMath::Vec3Add(&particlePositions[sourceBufferIndex][particleIndex], &probeVelocity, &probePosition);

        const float sourceDepthFactor = 1.5f - particlePositions[sourceBufferIndex][particleIndex].z;
        const float probeDepthFactor = 1.5f - probePosition.z;
        particleQuads[particleIndex].x = (int)(((probeDepthFactor * probePosition.x) - -0.5f) * (float)(viewportWidth));
        particleQuads[particleIndex].y
            = (int)(((probeDepthFactor * probePosition.y) - -0.5f) * (float)(viewportHeight));
        particleQuads[particleIndex].width
            = (int)(((sourceDepthFactor * particlePositions[sourceBufferIndex][particleIndex].x) - -0.5f)
                  * (float)(viewportWidth))
            - particleQuads[particleIndex].x;
        particleQuads[particleIndex].height
            = (int)(((sourceDepthFactor * particlePositions[sourceBufferIndex][particleIndex].y) - -0.5f)
                  * (float)(viewportHeight))
            - particleQuads[particleIndex].y;
        particleQuads[particleIndex].color16 = packedColor16;
        particleQuads[particleIndex].texCoordUStart = probeDepthFactor * alphaStartScale;
        particleQuads[particleIndex].texCoordUEnd = sourceDepthFactor * alphaEndScale;
        particleQuads[particleIndex].slantOffset = kHudWeatherFxRainSlantDelta;

        ResetParticleSlot(particleIndex, 0);
    }

    HudUiElement::Update(deltaSeconds);

    const int oldSourceBufferIndex = sourceBufferIndex;
    sourceBufferIndex = destBufferIndex;
    destBufferIndex = oldSourceBufferIndex;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-config-update-local
 * @recoil-artifact defines .text recoil:function:0x4bed30: zVideoFxPass3Config::UpdateLocal.
 * @recoil-match byte
 *
 * Purpose: update the pass-3 children and reset the queued-slot count.
 * Retail 0x4bed30 receives this in ECX and deltaTime on the stack.
 */
void zVideoFxPass3Config::UpdateLocal(float deltaTime)
{
    HudUiContainer::UpdateAll(deltaTime);
    slotWriteIndex = 0;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-config-set-primary-element-params-local
 * @recoil-artifact defines .text recoil:function:0x4bed50: zVideoFxPass3Config::SetPrimaryElementParamsLocal.
 * @recoil-match byte
 *
 * Purpose: arm the root overlay with its packed color and alpha.
 * Retail 0x4bed50 receives this in ECX and both arguments on the stack.
 */
void zVideoFxPass3Config::SetPrimaryElementParamsLocal(unsigned short packedColor, double primaryAlpha)
{
    rootElement.packedColor16 = packedColor;
    rootElement.alpha = primaryAlpha;
    HudUiElement* const element = &rootElement;
    element->SetVisible(1);
    element->timer = 0.0f;
    element->flags |= 0x01u;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-config-queue-element-local
 * @recoil-artifact defines .text recoil:function:0x4bed90: zVideoFxPass3Config::QueueElementLocal.
 * @recoil-match byte
 *
 * Purpose: queue one radial-warp slot for the next pass-3 update.
 * Retail 0x4bed90 receives this in ECX and all seven arguments on the stack.
 */
void zVideoFxPass3Config::QueueElementLocal(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
)
{
    const int slotIndex = slotWriteIndex;
    zVideoFxPass3Slot* const slot = &slots[slotIndex];
    if (slotIndex < 4) {
        slotWriteIndex = slotIndex + 1;
    }

    slot->SetRectAndPayload(
        rectLeftPixels,
        rectTopPixels,
        currentRadiusPixels,
        maxRadiusPixels,
        extentPixels,
        sinFreq,
        sinPhase
    );
    slot->SetVisible(1);
    slot->timer = 0.0f;
    slot->flags |= 0x01u;
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-config-set-input-rect-by-index
 * @recoil-artifact defines .text recoil:function:0x4bee00: zVideoFxPass3Config::SetInputRectByIndex.
 * @recoil-match byte
 *
 * Purpose: store a provisional pass-3 input rectangle while retail source
 * placement remains unresolved.
 */
void zVideoFxPass3Config::SetInputRectByIndex(int index, HudUiRect* rectOrNull)
{
    if (index < 2) {
        inputRectsOrNull[index] = rectOrNull;
    }
}

/**
 * Purpose: store provisional raw pass-3 surface input while retail source
 * placement remains unresolved.
 */
void zVideoFxPass3Config::QueuePrimitiveRaw(void* primitive, int width, int height, int pitchBytes)
{
    surfacePixels = (unsigned short*)(primitive);
    surfaceWidth = width;
    surfaceHeight = height;
    surfacePitchBytes = pitchBytes;
}

/**
 * Purpose: provide the provisional pass-3 configuration destructor while
 * retail source placement remains unresolved.
 */
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zvideo-zvid-main-g-zvideo-fxpass3configlocal
 * @recoil-artifact defines .data recoil:data:0x56bd58: g_zVideo_FxPass3ConfigLocal.
 * Data owner evidence: retail 0x56bd58 is the authored zero-initialized
 * zVideoFxPass3Config singleton, complete size 0x1f0. Sibling pass-3 scratch,
 * clip, and surface globals are separate zVideo data owners.
 * Purpose: store the local pass-3 UI config used by the zVideo namespace
 * wrapper functions.
 */
zVideoFxPass3Config g_zVideo_FxPass3ConfigLocal;
RECOIL_STATIC_ASSERT(sizeof(g_zVideo_FxPass3ConfigLocal) == 0x1f0);

namespace zVideo {

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-set-primary-element-params-local
 * @recoil-artifact defines .text recoil:function:0x4beee0: zVideo::FxPass3SetPrimaryElementParamsLocal.
 * @recoil-match byte
 *
 * Purpose: relay provisional local pass-3 primary-element state while retail
 * source placement remains unresolved.
 */
void __fastcall FxPass3SetPrimaryElementParamsLocal(unsigned short packedColor, double primaryAlpha)
{
    g_zVideo_FxPass3ConfigLocal.SetPrimaryElementParamsLocal(packedColor, primaryAlpha);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-queue-element-local
 * @recoil-artifact defines .text recoil:function:0x4bef10: zVideo::FxPass3QueueElementLocal.
 * @recoil-match byte
 *
 * Purpose: relay provisional local pass-3 queue state while retail source
 * placement remains unresolved.
 */
void __fastcall FxPass3QueueElementLocal(
    int rectLeftPixels,
    int rectTopPixels,
    int currentRadiusPixels,
    int maxRadiusPixels,
    int extentPixels,
    float sinFreq,
    float sinPhase
)
{
    g_zVideo_FxPass3ConfigLocal.QueueElementLocal(
        rectLeftPixels,
        rectTopPixels,
        currentRadiusPixels,
        maxRadiusPixels,
        extentPixels,
        sinFreq,
        sinPhase
    );
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-set-input-rect-by-index
 * @recoil-artifact defines .text recoil:function:0x4bef40: zVideo::FxPass3SetInputRectByIndex.
 * @recoil-match byte
 *
 * Purpose: relay a provisional local pass-3 input rectangle while retail
 * source placement remains unresolved.
 */
void __fastcall FxPass3SetInputRectByIndex(int index, HudUiRect* rectOrNull)
{
    g_zVideo_FxPass3ConfigLocal.SetInputRectByIndex(index, rectOrNull);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-queue-primitive
 * @recoil-artifact defines .text recoil:function:0x4bef50: zVideo::FxPass3QueuePrimitive.
 * @recoil-match byte
 *
 * Purpose: relay provisional raw pass-3 surface input while retail source
 * placement remains unresolved.
 */
void __fastcall FxPass3QueuePrimitive(void* primitive, int width, int height, int pitchBytes)
{
    g_zVideo_FxPass3ConfigLocal.QueuePrimitiveRaw(primitive, width, height, pitchBytes);
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-update-local
 * @recoil-artifact defines .text recoil:function:0x4bef70: zVideo::FxPass3UpdateLocal.
 * @recoil-match byte
 *
 * Purpose: relay the provisional local pass-3 update while retail source
 * placement remains unresolved.
 */
void __fastcall FxPass3UpdateLocal(float deltaTime)
{
    g_zVideo_FxPass3ConfigLocal.UpdateLocal(deltaTime);
}

} // namespace zVideo

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.z-video-fx-pass3-config-z-video-fx-pass3-config
 * @recoil-artifact defines .text recoil:function:0x4bef90: zVideoFxPass3Config::zVideoFxPass3Config.
 * @recoil-match byte
 *
 * Purpose: provide the provisional pass-3 configuration constructor while
 * retail source placement remains unresolved.
 */
zVideoFxPass3Config::zVideoFxPass3Config()
{
    int slotIndex;
    for (int rectIndex = 0; rectIndex < 2; ++rectIndex) {
        inputRectsOrNull[rectIndex] = 0;
    }
    surfacePixels = 0;
    surfaceWidth = 0;
    surfaceHeight = 0;

    HudUiContainer::AddChild((HudUiElement*)(&rootElement));
    rootElement.SetVisible(0);

    for (slotIndex = 0; slotIndex < 5; ++slotIndex) {
        HudUiContainer::AddChild((HudUiElement*)(&slots[slotIndex]));
        slots[slotIndex].SetVisible(0);
    }

    slotWriteIndex = 0;
    HudUiContainer* const container = this;
    container->SetEnabled(1);
}

extern char k_msgBoxWidgetName_Message[8];
extern char k_msgBoxWidgetName_Title[6];
extern char k_msgBoxWidgetName_Cancel[10];
extern char k_msgBoxWidgetName_OK[6];

/**
 * Recovered original-source helper, no standalone retail function.
 * Observed caller: 0x4bf060 HudUiMessageBoxDialog::Constructor.
 * Purpose: divide signed fallback layout coordinates by a power of two with the
 * same toward-zero correction pattern emitted in the message-box constructor.
 */
static inline int HudUiDialogSignedDivPow2(int value, int shift)
{
    const int signMask = value >> 31;
    return (value + (signMask & ((1 << shift) - 1))) >> shift;
}

/**
 * Recovered original-source helper, no standalone retail function.
 * Observed caller: 0x4bf060 HudUiMessageBoxDialog::Constructor.
 * Purpose: allocate a 16-bit solid-color zVid image for the message-box
 * fallback path when no ZRD layout section is supplied.
 */
static inline zVidImagePartial* HudUiMessageBoxCreateSolidImage(int width, int height, unsigned short color565)
{
    zVidImagePartial* const image = zVid_Image::Create();
    zVid_Image::SetFormatCode(image, 1);
    zVid_Image::SetSize(image, (short)(width), (short)(height));

    void* const pixels = malloc(zVid_Image::QueryBytesPerPixel(image) * width * height);
    zVidImageSetPixels(image, pixels, 0);

    for (int index = 0; index < image->pixelCount; ++index) {
        ((unsigned short*)(image->pixels))[index] = color565;
    }

    return image;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-constructor
 * @recoil-artifact defines .text recoil:function:0x4bf060: HudUiMessageBoxDialog::Constructor.
 *
 *
 * Source model: class-first constructor for HudUiMessageBoxDialog; BN table
 * 0x4d4028 is installed at object offset zero by the constructed C++ object.
 * Purpose: bind the ZRD-backed message-box widgets, or build the original
 * solid-image fallback dialog and child widget graph.
 * Touched data: owns runtime image pointers only; dialog/button table globals
 * are class identity evidence, not separately promoted data.
 */
HudUiMessageBoxDialog::HudUiMessageBoxDialog(const char* zrdPath, const char* sectionName)
    : HudUiBackground()
    , backdropWidget(0)
    , messagePanel(0, 0, 0)
    , titlePanel(0, 0, 0)
    , okButton()
    , cancelButton()
{
    const zVidRect32* const primaryRect = zVideo::GetPrimarySurfaceRectScratch();
    blitRect = *primaryRect;

    if (zrdPath != 0 && sectionName != 0) {
        backgroundImage = 0;
        okButtonNormalImage = 0;
        okButtonPressedImage = 0;

        zReader::Node* const loadedSection = LoadFromZrd(zrdPath, sectionName, 0);
        if (loadedSection != 0) {
            BindWidgetByName(loadedSection, &okButton, k_msgBoxWidgetName_OK);
            BindWidgetByName(loadedSection, &cancelButton, k_msgBoxWidgetName_Cancel);
            BindPrimitiveNodeToElement(loadedSection, &titlePanel, k_msgBoxWidgetName_Title);
            BindPrimitiveNodeToElement(loadedSection, &messagePanel, k_msgBoxWidgetName_Message);
            FreeLoadedTreeRoots((int)loadedSection);
        }

        titlePanel.SetVisible(1);
        messagePanel.SetVisible(1);
        return;
    }

    const int left = blitRect.right / 2 - 150;
    fallbackWidth = 300;
    fallbackHeight = 200;
    const int top = blitRect.bottom / 2 - 100;

    backgroundImage = HudUiMessageBoxCreateSolidImage(
        fallbackWidth,
        fallbackHeight,
        (unsigned short)(zVidPackColorRGB(128, 128, 128))
    );
    okButtonNormalImage = HudUiMessageBoxCreateSolidImage(
        fallbackWidth / 4,
        fallbackHeight / 4,
        (unsigned short)(zVidPackColorRGB(192, 192, 192))
    );
    okButtonPressedImage = HudUiMessageBoxCreateSolidImage(
        fallbackWidth / 4,
        fallbackHeight / 4,
        (unsigned short)(zVidPackColorRGB(160, 192, 160))
    );

    backdropWidget.SetImageBorrowedAndInvalidate(backgroundImage);
    messagePanel.SetTextFmt("");
    titlePanel.SetTextFmt("");
    okButton.LoadFromZrd(0, this);
    okButton.defaultImage = okButton.SetImageBorrowedAndInvalidate(okButtonNormalImage);
    okButton.rolloverImage = okButtonPressedImage;

    backdropWidget.SetPos(left, top);
    titlePanel.SetPos(left + 10, top + 10);
    messagePanel.SetPos(left + 10, top + 30);
    okButton.SetPos(left + fallbackWidth / 2 - fallbackWidth / 8, top - fallbackHeight / 4 + fallbackHeight - 10);

    AddChild(&backdropWidget);
    AddChild(&messagePanel);
    AddChild(&titlePanel);
    AddChild(&okButton);
    messagePanel.SetVisible(1);
    titlePanel.SetVisible(1);
    okButton.SetVisible(0);
    SetChildFlags(0);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-destructor
 * @recoil-artifact defines .text recoil:function:0x4bf560: HudUiMessageBoxDialog::Destructor.
 * @recoil-match byte
 *
 * Source model: HudUiMessageBoxDialog class destructor; BN shows the dialog
 * table 0x4d4028 at offset zero for this owner.
 * Purpose: release fallback images and tear down message-box child widgets in
 * the recovered member cleanup order.
 * Touched data: no authored globals; releases runtime-owned image storage.
 */
HudUiMessageBoxDialog::~HudUiMessageBoxDialog()
{
    zVidImagePartial* const image = backgroundImage;
    if (image != 0) {
        if (image->pixels != 0) {
            free(image->pixels);
        }

        image->pixels = 0;
        zVid_Image::Destroy(image);
    }

    backgroundImage = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-runmodal
 * @recoil-artifact defines .text recoil:function:0x4bf630: HudUiMessageBoxDialog::RunModal.
 * @recoil-match byte
 *
 * Source model: direct HudUiMessageBoxDialog method called by
 * HudUi::ShowMessageBox, not a dialog-table slot.
 * Purpose: switch render/input state to modal drawing, pump frames until the
 * dialog records a result, then restore the previous framebuffer region.
 * Touched data: direct globals are renderer/time owners already linked by the
 * parent handoff; no dialog-owned plan-tracked data is promoted here.
 */
int HudUiMessageBoxDialog::RunModal(
    const char* messageText,
    const char* titleText,
    void* modalContext,
    float timeoutSeconds
)
{
    (void)modalContext;
    (void)timeoutSeconds;

    if (g_zVideo_ActiveRendererPath != 0) {
        g_zVideo_pfnBltSwToPrimaryRectDirect(0, 0);
    }

    const int previousHalfResMode = zVideo::SetHalfResAdjustMode(ZVIDEO_HALFRES_ADJUST_DISABLED);
    HudUi::SetInvalidateMode(0);

    zVidRect32 previousRegionRect;
    int previousBitsPerPixel;
    int previousPitchBytes;
    previousRegionRect.left = 0;
    previousRegionRect.top = 0;
    void* const previousPixels = zRndr::GetActiveRegionState(
        &previousRegionRect.right,
        &previousRegionRect.bottom,
        &previousBitsPerPixel,
        &previousPitchBytes
    );

    if (g_zVideo_ActiveRendererPath != 0) {
        zRndr::SetFrameBufferRegion(
            zVideo::GetPrimarySurfacePixels(),
            (zOpt_ViewRectSection*)(&blitRect),
            zVideo::GetDisplayModeBpp(),
            zVideo::GetPrimarySurfacePitch()
        );
    } else {
        zRndr::SetFrameBufferRegion(
            zVideo::GetSwSurfacePixels(),
            (zOpt_ViewRectSection*)(&blitRect),
            zVideo::GetDisplayModeBpp(),
            zVideo::GetSwSurfacePitch()
        );
    }

    modalResult = 0;
    modalFrameCountdown = 100000;
    SetEnabled(1);
    messagePanel.SetTextFmt(messageText);
    titlePanel.SetTextFmt(titleText);
    okButton.SetVisible(1);

    int framesRemaining = modalFrameCountdown;
    modalFrameCountdown = framesRemaining - 1;
    while (framesRemaining > 0) {
        zInput::PollActiveDevices(0);
        Time::Tick();
        zVideo::RunPostprocessOnPrimaryBuffer();
        UpdateAll(g_FrameDeltaTimeSec);
        zVideo::DispatchUnlockPrimarySurfaceState();
        zVideo::AdjustSurfacesIfEnabled(&blitRect, &blitRect, 1, 1);
        framesRemaining = modalFrameCountdown;
        modalFrameCountdown = framesRemaining - 1;
    }

    ((HudUiDialogController*)(this))->BlitOwnedSurfaceToPrimary();
    SetEnabled(0);
    zVideo::SetHalfResAdjustMode(previousHalfResMode);
    HudUi::SetInvalidateMode(previousHalfResMode);
    zRndr::SetFrameBufferRegion(
        previousPixels,
        (zOpt_ViewRectSection*)(&previousRegionRect),
        previousBitsPerPixel,
        previousPitchBytes
    );
    return modalResult;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-onok
 * @recoil-artifact defines .text recoil:function:0x4bf7c0: HudUiMessageBoxDialog::OnOk.
 * @recoil-match byte
 *
 * Source model: HudUiMessageBoxDialog table slot +0x0c in table 0x4d4028.
 * Purpose: accept the modal dialog and force the modal loop to exit.
 * Touched data: no authored globals; writes dialog modal fields only.
 */
void HudUiMessageBoxDialog::OnOk()
{
    modalResult = 1;
    modalFrameCountdown = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxdialog-oncancel
 * @recoil-artifact defines .text recoil:function:0x4bf7e0: HudUiMessageBoxDialog::OnCancel.
 * @recoil-match byte
 *
 * Source model: HudUiMessageBoxDialog table slot +0x10 in table 0x4d4028.
 * Purpose: cancel the modal dialog and force the modal loop to exit.
 * Touched data: no authored globals; writes dialog modal fields only.
 */
void HudUiMessageBoxDialog::OnCancel()
{
    modalResult = 2;
    modalFrameCountdown = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxokbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x4bf800: HudUiMessageBoxOkButton::OnActivate.
 * @recoil-match byte
 *
 * Source model: HudUiMessageBoxOkButton activation override; button table
 * 0x4d40c8 overrides slot +0x30 with this method.
 * Purpose: dispatch through the owner dialog to OnOk, then run the base
 * HudUiZrdWidget activation behavior.
 * Touched data: no authored globals; owner vptr dispatch reaches the dialog
 * table slot before HudUiZrdWidget::OnActivate.
 */
void HudUiMessageBoxOkButton::OnActivate()
{
    HudUiMessageBoxDialog* const dialog = (HudUiMessageBoxDialog*)(owner);
    dialog->OnOk();

    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduimessageboxcancelbutton-onactivate
 * @recoil-artifact defines .text recoil:function:0x4bf820: HudUiMessageBoxCancelButton::OnActivate.
 * @recoil-match byte
 *
 * Source model: HudUiMessageBoxCancelButton activation override; button table
 * 0x4d4040 overrides slot +0x30 with this method.
 * Purpose: dispatch through the owner dialog to OnCancel, then run the base
 * HudUiZrdWidget activation behavior.
 * Touched data: no authored globals; owner vptr dispatch reaches the dialog
 * table slot before HudUiZrdWidget::OnActivate.
 */
void HudUiMessageBoxCancelButton::OnActivate()
{
    HudUiMessageBoxDialog* const dialog = (HudUiMessageBoxDialog*)(owner);
    dialog->OnCancel();

    HudUiZrdWidget::OnActivate();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduipolyline-huduipolyline
 * @recoil-artifact defines .text recoil:function:0x4bf840: HudUiPolyline::HudUiPolyline.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::HudUiPolyline.
 */
HudUiPolyline::HudUiPolyline()
    : HudUiElement(0, 0)
{
    pointCount = 0;
    memset(points, 0, sizeof(points));
    Invalidate();
    clipRect = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduipolyline-setpoint
 * @recoil-artifact defines .text recoil:function:0x4bf8b0: HudUiPolyline::SetPoint.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiPolyline::SetPoint.
 */
void HudUiPolyline::SetPoint(int index, int pointX, int pointY)
{
    points[index].x = pointX;
    points[index].y = pointY;

    if (pointCount <= index) {
        pointCount = index + 1;
    }

    if (index == 0) {
        SetPos(pointX, pointY);
    }

    Invalidate();
}

/**
 * @recoil-anchor recoil:anchor:zui.zui-fx.hud-ui-polyline-draw
 * @recoil-artifact defines .text recoil:function:0x4bf900: HudUiPolyline::Draw.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiPolyline::Draw.
 */
void HudUiPolyline::Draw()
{
    DrawBase();

    if (pointCount == 0) {
        return;
    }

    if (clipRect != 0) {
        zRndrDrawClippedImmediateLineStrip((const zRndr_LinePoint2I*)(points), pointCount - 1, clipRect, color565);
        return;
    }

    HudUiPolylinePoint* point = points;
    HudUiPolylinePoint* nextPoint = points + 1;
    for (int index = 0; index < pointCount - 1; ++index, ++point, ++nextPoint) {
        zRndrDrawImmediateLine(point->x, point->y, nextPoint->x, nextPoint->y, color565);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-huduibackgroundcursorwidget-0x4bf980
 * @recoil-artifact defines .text recoil:function:0x4bf980: HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget.
 */
HudUiBackgroundCursorWidget::HudUiBackgroundCursorWidget(const char* imagePath, int initCaptureEnabled)
    : HudUiWidget(0)
{
    captureEnabled = initCaptureEnabled;
    capturedImage = 0;
    if (imagePath != 0) {
        SetImageByPathOwnedAndRefresh(imagePath);
    }

    reservedC8 = 0;
    reservedCC = 0;
    captureSourceSelector = 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-huduibackgroundcursorwidget-0x4bfa20
 * @recoil-artifact defines .text recoil:function:0x4bfa20: HudUiBackgroundCursorWidget::~HudUiBackgroundCursorWidget.
 * @recoil-match byte
 *
 * Purpose: restore the cursor widget dispatch state, release a captured image, and tear down the widget base.
 */
HudUiBackgroundCursorWidget::~HudUiBackgroundCursorWidget()
{
    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setimagebypathownedandrefresh
 * @recoil-artifact defines .text recoil:function:0x4bfa50: HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageByPathOwnedAndRefresh(const char* imagePath)
{
    if (HudUiWidget::SetImageByPathOwned(imagePath) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setimageborrowedandrefreshifchanged
 * @recoil-artifact defines .text recoil:function:0x4bfa70: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by
 * HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged.
 */
void HudUiBackgroundCursorWidget::SetImageBorrowedAndRefreshIfChanged(zVidImagePartial* image)
{
    if (HudUiWidget::SetImageBorrowedAndInvalidate(image) != 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setimageownedandrefresh
 * @recoil-artifact defines .text recoil:function:0x4bfa90: HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageOwnedAndRefresh(int newCaptureEnabled)
{
    captureEnabled = newCaptureEnabled;
    if (newCaptureEnabled == 0 && capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
        capturedImage = 0;
        SetBltSourceAndClipRect(0, 0);
        return;
    }

    if (capturedImage == 0) {
        SetImageBorrowedAndRefresh();
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setimageborrowedandrefresh
 * @recoil-artifact defines .text recoil:function:0x4bfae0: HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh.
 */
void HudUiBackgroundCursorWidget::SetImageBorrowedAndRefresh()
{
    if (captureEnabled == 0) {
        return;
    }

    zVidImagePartial* const sourceImage = image;
    if (sourceImage == 0) {
        return;
    }

    if (capturedImage != 0) {
        zVid_Image::Destroy(capturedImage);
    }

    capturedImage = zVid_Image::Create();
    if (capturedImage == 0) {
        return;
    }

    zVid_Image::SetSize(capturedImage, sourceImage->width, sourceImage->height);
    void* const pixels = malloc((size_t)(capturedImage->pixelCount) * sizeof(unsigned short));
    zVidImageSetPixels(capturedImage, pixels, 0);
    capturedImage->formatFlagsPacked = (unsigned char)(capturedImage->formatFlagsPacked | 0x20u);

    RebuildCapturedImage(GetCenterX(), GetCenterY());
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-setpos
 * @recoil-artifact defines .text recoil:function:0x4bfb70: HudUiBackgroundCursorWidget::SetPos.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiBackgroundCursorWidget::SetPos.
 */
void HudUiBackgroundCursorWidget::SetPos(int newX, int newY)
{
    HudUiWidget::SetPos(newX, newY);
    RebuildCapturedImage(x, y);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-rebuildcapturedimage
 * @recoil-artifact defines .text recoil:function:0x4bfba0: HudUiBackgroundCursorWidget::RebuildCapturedImage.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::RebuildCapturedImage.
 */
void HudUiBackgroundCursorWidget::RebuildCapturedImage(int originX, int originY)
{
    if (capturedImage == 0) {
        return;
    }

    zVidImagePartial* const cursorImage = image;
    zVidRect32 sourceRect;
    sourceRect.top = originY;
    sourceRect.bottom = originY + cursorImage->height;
    sourceRect.left = originX;
    sourceRect.right = originX + cursorImage->width;

    if (zVideo_buff::CopySurfaceRectToImage(captureSourceSelector, &sourceRect, capturedImage) != 0) {
        HudUiRect clipRect;
        clipRect.top = sourceRect.top - originY;
        clipRect.bottom = sourceRect.bottom - originY;
        clipRect.left = sourceRect.left - originX;
        clipRect.right = sourceRect.right - originX;
        SetBltSourceAndClipRect(capturedImage, &clipRect);
        return;
    }

    SetBltSourceAndClipRect(0, 0);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-draw
 * @recoil-artifact defines .text recoil:function:0x4bfc50: HudUiBackgroundCursorWidget::Draw.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::Draw.
 */
void HudUiBackgroundCursorWidget::Draw()
{
    HudUiWidget::Draw();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundcursorwidget-drawbase
 * @recoil-artifact defines .text recoil:function:0x4bfc60: HudUiBackgroundCursorWidget::DrawBase.
 * @recoil-match byte
 *
 * Purpose: preserve the recovered HUD behavior for HudUiBackgroundCursorWidget::DrawBase.
 */
inline void HudUiBackgroundCursorWidget::DrawBase()
{
    if (bltSource != 0) {
        zVid_Image::BlitToActiveTarget((zVidImagePartial*)(bltSource), x, y, 0, (zVidRect32*)(&clipRect));
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-huduibackgroundvideowidget
 * @recoil-artifact defines .text recoil:function:0x4bfc80: HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget.
 * @recoil-match byte
 *
 * Purpose: Initializes the background video element state before a stream is assigned.
 */
HudUiBackgroundVideoWidget::HudUiBackgroundVideoWidget()
    : HudUiElement(0, 0)
{
    mediaPath[0] = '\0';
    stream = 0;
    elapsedTimeSec = 0.0f;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-destructor
 * @recoil-artifact defines .text recoil:function:0x4bfcd0: HudUiBackgroundVideoWidget::~HudUiBackgroundVideoWidget.
 * @recoil-match byte
 *
 * Purpose: destroy the owned movie stream and run the base widget teardown.
 */
HudUiBackgroundVideoWidget::~HudUiBackgroundVideoWidget()
{
    zFMV_Stream* const oldStream = stream;
    if (oldStream != 0) {
        oldStream->Destructor();
        ::operator delete(oldStream);
        stream = 0;
    }
}

/**
 * Purpose: invoke the native destructor through the explicit cleanup method.
 * The virtual destructor above owns the retail body at 0x4bfcd0.
 *
 */
void HudUiBackgroundVideoWidget::Destructor()
{
    this->~HudUiBackgroundVideoWidget();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-setmediapathownedandrefresh
 * @recoil-artifact defines .text recoil:function:0x4bfd40: HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh.
 * @recoil-match byte
 *
 * Purpose: Stores the movie path, resolves missing media, opens the stream, and refreshes clipping.
 */
void HudUiBackgroundVideoWidget::SetMediaPathOwnedAndRefresh(const char* path)
{
    strncpy(mediaPath, path, 0x104);

    struct _stat statBuffer;
    if (_stat(mediaPath, &statBuffer) == -1) {
        char* const resolvedPath = zSys::FindFileOnDriveType(5, mediaPath, 0);
        if (resolvedPath != 0) {
            strncpy(mediaPath, resolvedPath, 0x104);
        }
    }

    if (_stat(mediaPath, &statBuffer) == -1) {
        stream = 0;
        return;
    }

    stream = new zFMV_Stream(mediaPath, 0);

    RebuildBltRect();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-setcolorkey565
 * @recoil-artifact defines .text recoil:function:0x4bfe20: HudUiBackgroundVideoWidget::SetColorKey565.
 * @recoil-match byte
 *
 * Purpose: Marks the active video stream format dirty and stores the 565 color key.
 */
void HudUiBackgroundVideoWidget::SetColorKey565(unsigned short colorKey)
{
    if (stream != 0) {
        stream->formatFlagsPacked |= 0x02;
    }

    colorKey565 = colorKey;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-update
 * @recoil-artifact defines .text recoil:function:0x4bfe40: HudUiBackgroundVideoWidget::Update.
 * @recoil-match byte
 *
 * Purpose: Advances decoded video frames while preserving the base element update behavior.
 */
void HudUiBackgroundVideoWidget::Update(float deltaSeconds)
{
    if (((~flags) & 0x10u) == 0) {
        return;
    }

    if (stream != 0) {
        const int frameTick = (int)((int)(stream->videoFramesPerSecond) * elapsedTimeSec);
        stream->ReadAndDecodeFrame((unsigned int)(frameTick % stream->videoFrameCount));
    }

    HudUiElement::Update(deltaSeconds);
    elapsedTimeSec += deltaSeconds;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-draw
 * @recoil-artifact defines .text recoil:function:0x4bfe90: HudUiBackgroundVideoWidget::Draw.
 * @recoil-match byte
 *
 * Purpose: Draws the background layer and blits the active stream with the stored color key.
 */
void HudUiBackgroundVideoWidget::Draw()
{
    DrawBase();

    if (stream != 0) {
        zVid_Image::BlitToActiveTarget((zVidImagePartial*)(stream), x, y, colorKey565, 0);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-drawbase
 * @recoil-artifact defines .text recoil:function:0x4bfec0: HudUiBackgroundVideoWidget::DrawBase.
 * @recoil-match byte
 *
 * Purpose: Blits the configured background source into the current clipped video area.
 */
void HudUiBackgroundVideoWidget::DrawBase()
{
    const int dstX = x > 0 ? x : 0;
    const int dstY = y > 0 ? y : 0;
    if (bltSource != 0) {
        zVid_Image::BlitToActiveTarget((zVidImagePartial*)(bltSource), dstX, dstY, 0, (zVidRect32*)(&clipRect));
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduibackgroundvideowidget-rebuildbltrect
 * @recoil-artifact defines .text recoil:function:0x4bff00: HudUiBackgroundVideoWidget::RebuildBltRect.
 * @recoil-match byte
 *
 * Purpose: Recomputes the stream clip rectangle against the background blit source.
 */
void HudUiBackgroundVideoWidget::RebuildBltRect()
{
    HudUiRect rect;
    rect.left = GetCenterX() > 0 ? GetCenterX() : 0;
    rect.top = GetCenterY() > 0 ? GetCenterY() : 0;

    if (stream == 0) {
        return;
    }

    zVidImagePartial* const bltSource = (zVidImagePartial*)(this->bltSource);
    if (bltSource != 0) {
        rect.right = rect.left + stream->width < bltSource->width ? rect.left + stream->width : bltSource->width;
        rect.bottom = rect.top + stream->height < bltSource->height ? rect.top + stream->height : bltSource->height;
    } else {
        rect.right = rect.left + stream->width;
        rect.bottom = rect.top + stream->height;
    }

    SetClipRect(&rect);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zui-zui-huduiprimitivebindtarget-setsegmentendpoints
 * @recoil-artifact defines .text recoil:function:0x4bffb0: HudUiPrimitiveBindTarget::SetSegmentEndpoints.
 * @recoil-match byte
 *
 * Purpose: apply the recovered HUD state change handled by HudUiPrimitiveBindTarget::SetSegmentEndpoints.
 */
void HudUiPrimitiveBindTarget::SetSegmentEndpoints(int startX, int startY, int newEndX, int newEndY)
{
    SetPos(startX, startY);
    endX = newEndX;
    endY = newEndY;
}
