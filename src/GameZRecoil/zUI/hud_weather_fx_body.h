#pragma once

/* Complete HudWeatherFx class-family body for the later zui.cpp physical host. */
/* Include exactly once after the owner-preceding zUI contribution run. */

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
 * Reimplements 0x4bdc70: HudWeatherFx::HudWeatherFx(int).
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Initialize the base weather particle emitter, allocate particle buffers, reset
 * particles, and create the hardware SnowFX texture resources when needed.
 */
HudWeatherFx::HudWeatherFx(
    int newParticleCount
) {
    HudUiElement::Constructor(
        0,
        0
    );
    clipRectOrNull = 0;
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

}

/**
 * Reimplements 0x4bde40: HudWeatherFx::~HudWeatherFx.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Release particle buffers and renderer-backed weather texture resources.
 */
HudWeatherFx::~HudWeatherFx() {
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
 * Reimplements 0x4be280: HudWeatherFxSnow::HudWeatherFxSnow(int).
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Construct the shared weather emitter and initialize snow emitter defaults.
 */
HudWeatherFxSnow::HudWeatherFxSnow(
    int particleCount
) : HudWeatherFx(particleCount) {
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
}

/**
 * Reimplements 0x4be2e0: HudWeatherFxSnow::~HudWeatherFxSnow.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Tear down the snow emitter and continue through the shared C++ base destructor.
 */
HudWeatherFxSnow::~HudWeatherFxSnow() {
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
 * Reimplements 0x4be810: HudWeatherFxRain::HudWeatherFxRain(int).
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Construct the shared weather emitter and initialize rain emitter defaults.
 */
HudWeatherFxRain::HudWeatherFxRain(
    int particleCount
) : HudWeatherFx(particleCount) {
    emitEnabled = 1;
    emitRadius = 20.0f;
    emitDepth = 400.0f;
}

/**
 * Reimplements 0x4be870: HudWeatherFxRain::~HudWeatherFxRain.
 * Original source path: D:\Proj\Battlesport\hud.cpp.
 * Purpose: Tear down the rain emitter and continue through the shared C++ base destructor.
 */
HudWeatherFxRain::~HudWeatherFxRain() {
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
