#include "Battlesport/hud.h"
#include "GameZRecoil/zClass/cls_stubs.h"
#include "GameZRecoil/zRndr/zrndr.h"
#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
bool HudFloatNear(float actual, float expected) {
    const float delta = actual - expected;
    return delta > -0.0001f && delta < 0.0001f;
}

template <typename Method> std::uintptr_t MethodAddress(Method method) {
    static_assert(sizeof(method) <= sizeof(std::uintptr_t));
    std::uintptr_t address = 0;
    std::memcpy(&address, &method, sizeof(method));
    return address;
}

const char *g_weatherTextureName = nullptr;
zVidImagePartial *g_weatherTextureImage = nullptr;
int g_weatherTextureUseAlpha = 0;
int g_weatherTextureArg3 = 0;
int g_weatherTextureArg4 = 0;
int g_weatherTextureDestroyCount = 0;
zVideo_TextureRecordPartial *g_weatherTextureDestroyed = nullptr;
zVideo_TextureRecordPartial g_weatherTextureRecord = {};
int g_weatherFinalizeUploadCount = 0;
zVideo_TextureRecordPartial *g_weatherFinalizeTextureRecord = nullptr;
zVidImagePartial *g_weatherFinalizeImage = nullptr;
int g_weatherSubmitPolyCount = 0;
zVideo_XyzVertex g_weatherSubmittedVerts[4] = {};
zVideo_TexCoord g_weatherSubmittedTexCoords[4] = {};
int g_weatherSubmittedVertexCount = 0;
zVideo_RenderClass *g_weatherSubmittedRenderClass = nullptr;
unsigned int g_weatherSubmittedRenderParam = 0;
float g_weatherSubmittedAlpha = 0.0f;
int g_weatherSubmittedQueueMode = 0;
int g_weatherFlushSortedCount = 0;

zVideo_TextureRecordPartial *__fastcall
HudWeatherFxCreateTextureRecordStub(const char *textureName, zVidImagePartial *image,
                                    int useAlpha, int arg3, int arg4) {
    g_weatherTextureName = textureName;
    g_weatherTextureImage = image;
    g_weatherTextureUseAlpha = useAlpha;
    g_weatherTextureArg3 = arg3;
    g_weatherTextureArg4 = arg4;
    return &g_weatherTextureRecord;
}

void __fastcall HudWeatherFxDestroyTextureRecordStub(
    zVideo_TextureRecordPartial *textureRecord) {
    ++g_weatherTextureDestroyCount;
    g_weatherTextureDestroyed = textureRecord;
}

void __fastcall HudWeatherFxFinalizeUploadStub(
    zVideo_TextureRecordPartial *textureRecord, void *, zVidImagePartial *image) {
    ++g_weatherFinalizeUploadCount;
    g_weatherFinalizeTextureRecord = textureRecord;
    g_weatherFinalizeImage = image;
}

void __fastcall HudWeatherFxSubmitPolyStub(zVideo_XyzVertex *vertices,
                                                zVideo_TexCoord *texCoords,
                                                int vertexCount,
                                                zVideo_RenderClass *renderClass,
                                                unsigned int renderParam, float alpha,
                                                int queueMode) {
    ++g_weatherSubmitPolyCount;
    g_weatherSubmittedVertexCount = vertexCount;
    g_weatherSubmittedRenderClass = renderClass;
    g_weatherSubmittedRenderParam = renderParam;
    g_weatherSubmittedAlpha = alpha;
    g_weatherSubmittedQueueMode = queueMode;
    for (int index = 0; index < 4; ++index) {
        g_weatherSubmittedVerts[index] = vertices[index];
        g_weatherSubmittedTexCoords[index] = texCoords[index];
    }
}

void HudWeatherFxFlushSortedStub() {
    ++g_weatherFlushSortedCount;
}
} // namespace

extern "C" int hud_weather_fx_constructor_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    zVideo_CreateTextureRecordProc const oldCreateTextureRecord = g_zVideo_pfnCreateTextureRecord;

    g_weatherTextureName = nullptr;
    g_weatherTextureImage = nullptr;
    g_weatherTextureUseAlpha = 0;
    g_weatherTextureArg3 = 0;
    g_weatherTextureArg4 = 0;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_pfnCreateTextureRecord = HudWeatherFxCreateTextureRecordStub;

    std::srand(1);
    HudWeatherFx weather = {};
    HudWeatherFx *const result = weather.Constructor(3);

    bool particlesCopied = true;
    for (int index = 0; index < weather.particleCount; ++index) {
        const zVec3 &source = weather.particlePositions[weather.sourceBufferIndex][index];
        const zVec3 &dest = weather.particlePositions[weather.destBufferIndex][index];
        particlesCopied = particlesCopied && source.x == dest.x && source.y == dest.y &&
                          source.z == dest.z && source.z >= 0.5f && source.z <= 1.0f;
    }

    const bool quadsInvalid =
        weather.particleQuads[0].x == -1 && weather.particleQuads[0].y == -1 &&
        weather.particleQuads[0].width == -1 && weather.particleQuads[0].height == -1 &&
        weather.particleQuads[2].x == -1 && weather.particleQuads[2].y == -1 &&
        weather.particleQuads[2].width == -1 && weather.particleQuads[2].height == -1;

    const bool initialized =
        result == &weather &&
        weather.clipRectOrNull == nullptr && weather.maxParticles == 3 &&
        weather.particleCount == 3 && weather.packedColor16 == 0x7fff &&
        weather.alphaStartScale == 1.0f && weather.alphaEndScale == 0.0500000007f &&
        weather.camera == nullptr && weather.activeParticleCount == 0 &&
        weather.sourceBufferIndex == 0 && weather.destBufferIndex == 1 &&
        weather.basisVector.x == 0.0f && weather.basisVector.y == 1.0f &&
        weather.basisVector.z == 0.0f && weather.gravity == 1.0f &&
        weather.windDirection == 0.0f && weather.windVelocity == 1.0f;

    const bool imageOk =
        weather.textureName != nullptr && std::strcmp(weather.textureName, "SnowFX") == 0 &&
        weather.softwareImage != nullptr && weather.softwareImage->formatFlagsPacked == 0x2b &&
        weather.softwareImage->width == 16 && weather.softwareImage->height == 8 &&
        weather.softwareImage->pixelCount == 128 && weather.textureRecord == &g_weatherTextureRecord &&
        g_weatherTextureName == weather.textureName &&
        g_weatherTextureImage == weather.softwareImage && g_weatherTextureUseAlpha == 2 &&
        g_weatherTextureArg3 == 1 && g_weatherTextureArg4 == 1;

    char *const alphaMap =
        weather.softwareImage != nullptr ? weather.softwareImage->alphaMap : nullptr;
    if (weather.softwareImage != nullptr) {
        zVid_Image::Destroy(weather.softwareImage);
    }
    if (alphaMap != nullptr) {
        std::free(alphaMap);
    }
    ::operator delete(weather.particleQuads);
    ::operator delete(weather.particlePositions[0]);
    ::operator delete(weather.particlePositions[1]);

    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnCreateTextureRecord = oldCreateTextureRecord;

    return initialized && quadsInvalid && particlesCopied && imageOk ? 0 : 1;
}

extern "C" int hud_weather_fx_destructor_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    zVideo_CreateTextureRecordProc const oldCreateTextureRecord = g_zVideo_pfnCreateTextureRecord;
    zVideo_DestroyTextureRecordProc const oldDestroyTextureRecord =
        g_zVideo_pfnTextureRecordDestroy;

    g_weatherTextureDestroyCount = 0;
    g_weatherTextureDestroyed = nullptr;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_pfnCreateTextureRecord = HudWeatherFxCreateTextureRecordStub;
    g_zVideo_pfnTextureRecordDestroy = HudWeatherFxDestroyTextureRecordStub;

    std::srand(4);
    HudWeatherFx hardwareWeather = {};
    hardwareWeather.Constructor(2);
    char *const hardwareAlphaMap = hardwareWeather.softwareImage != nullptr
                                       ? hardwareWeather.softwareImage->alphaMap
                                       : nullptr;
    zVideo_TextureRecordPartial *const hardwareTextureRecord = hardwareWeather.textureRecord;
    hardwareWeather.Destructor();

    const bool hardwareDestroyed =
        hardwareWeather.softwareImage == nullptr &&
        hardwareWeather.textureRecord == hardwareTextureRecord && g_weatherTextureDestroyCount == 1 &&
        g_weatherTextureDestroyed == hardwareTextureRecord;
    if (hardwareAlphaMap != nullptr) {
        std::free(hardwareAlphaMap);
    }

    g_weatherTextureDestroyCount = 0;
    g_weatherTextureDestroyed = nullptr;
    g_zVideo_ActiveRendererPath = 0;

    std::srand(5);
    HudWeatherFx softwareWeather = {};
    softwareWeather.Constructor(1);
    softwareWeather.textureRecord = &g_weatherTextureRecord;
    softwareWeather.Destructor();

    const bool softwareSkipped =
        softwareWeather.textureRecord == &g_weatherTextureRecord &&
        softwareWeather.softwareImage == nullptr && g_weatherTextureDestroyCount == 0 &&
        g_weatherTextureDestroyed == nullptr;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_pfnCreateTextureRecord = oldCreateTextureRecord;
    g_zVideo_pfnTextureRecordDestroy = oldDestroyTextureRecord;

    return hardwareDestroyed && softwareSkipped ? 0 : 1;
}

extern "C" int hud_weather_fx_scalar_deleting_destructors_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    g_zVideo_ActiveRendererPath = 0;

    std::srand(8);
    HudWeatherFx stackBase = {};
    stackBase.Constructor(1);
    const bool baseNoDeleteOk =
        stackBase.ScalarDeletingDestructor(0) == &stackBase &&
        stackBase.softwareImage == nullptr;

    HudWeatherFx *heapBase =
        (HudWeatherFx *)(::operator new(sizeof(HudWeatherFx)));
    std::memset(heapBase, 0, sizeof(HudWeatherFx));
    std::srand(9);
    heapBase->Constructor(1);
    HudWeatherFx *const heapBaseSelf = heapBase;
    const bool baseDeleteOk =
        heapBase->ScalarDeletingDestructor(1) == heapBaseSelf;

    std::srand(10);
    HudWeatherFxSnow stackSnow = {};
    stackSnow.Constructor(1);
    const bool snowNoDeleteOk =
        stackSnow.ScalarDeletingDestructor(0) == &stackSnow &&
        stackSnow.softwareImage == nullptr;

    HudWeatherFxSnow *heapSnow =
        (HudWeatherFxSnow *)(::operator new(sizeof(HudWeatherFxSnow)));
    std::memset(heapSnow, 0, sizeof(HudWeatherFxSnow));
    std::srand(11);
    heapSnow->Constructor(1);
    HudWeatherFxSnow *const heapSnowSelf = heapSnow;
    const bool snowDeleteOk =
        heapSnow->ScalarDeletingDestructor(1) == heapSnowSelf;

    std::srand(12);
    HudWeatherFxRain stackRain = {};
    stackRain.Constructor(1);
    const bool rainNoDeleteOk =
        stackRain.ScalarDeletingDestructor(0) == &stackRain &&
        stackRain.softwareImage == nullptr;

    HudWeatherFxRain *heapRain =
        (HudWeatherFxRain *)(::operator new(sizeof(HudWeatherFxRain)));
    std::memset(heapRain, 0, sizeof(HudWeatherFxRain));
    std::srand(13);
    heapRain->Constructor(1);
    HudWeatherFxRain *const heapRainSelf = heapRain;
    const bool rainDeleteOk =
        heapRain->ScalarDeletingDestructor(1) == heapRainSelf;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    return baseNoDeleteOk && baseDeleteOk && snowNoDeleteOk && snowDeleteOk &&
                   rainNoDeleteOk && rainDeleteOk
               ? 0
               : 1;
}

extern "C" int hud_weather_fx_are_point_batch_inside_rect_smoke(void) {
    HudUiRect viewport = {10, 20, 30, 40};
    HudWeatherFxPointBatch inside[4] = {
        {10.0f, 20.0f, 0.1f},
        {30.0f, 40.0f, 0.2f},
        {15.5f, 25.5f, 0.3f},
        {29.9f, 39.9f, 0.4f},
    };
    HudWeatherFxPointBatch outsideLeft[1] = {{9.99f, 25.0f, 0.0f}};
    HudWeatherFxPointBatch outsideRight[1] = {{30.01f, 25.0f, 0.0f}};
    HudWeatherFxPointBatch outsideTop[1] = {{20.0f, 19.99f, 0.0f}};
    HudWeatherFxPointBatch outsideBottom[1] = {{20.0f, 40.01f, 0.0f}};

    const bool trivialAccepted =
        inside->ArePointBatchInsideRect(4, nullptr) == 1 &&
        inside->ArePointBatchInsideRect(0, &viewport) == 1 &&
        inside->ArePointBatchInsideRect(-1, &viewport) == 1;
    const bool inclusiveInside =
        inside->ArePointBatchInsideRect(4, &viewport) == 1;
    const bool outsideRejected =
        outsideLeft->ArePointBatchInsideRect(1, &viewport) == 0 &&
        outsideRight->ArePointBatchInsideRect(1, &viewport) == 0 &&
        outsideTop->ArePointBatchInsideRect(1, &viewport) == 0 &&
        outsideBottom->ArePointBatchInsideRect(1, &viewport) == 0;

    return trivialAccepted && inclusiveInside && outsideRejected ? 0 : 1;
}

extern "C" int hud_weather_fx_draw_particles_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    unsigned short *const oldFxPixels = g_zVideo_FxSurfacePixels16;
    const int oldFxWidth = g_zVideo_FxSurfaceWidth;
    const int oldFxHeight = g_zVideo_FxSurfaceHeight;
    const int oldFxPitchBytes = g_zVideo_FxSurfacePitchBytes;
    const int oldFxPitchPixels16 = g_zVideo_FxSurfacePitchPixels16;
    const int oldPitchBytes = zRndr::g_pitchBytes;
    zVideo_TextureRecordFinalizeUploadProc const oldFinalizeUpload =
        g_zVideo_pfnTextureRecordFinalizeUpload;
    zVideo_SubmitPolyRenderClassProc const oldSubmitPoly =
        g_zVideo_pfnSubmitPolyRenderClass;
    zVideo_FlushProc const oldFlushSorted = g_zVideo_pfnFlushSortedPolys;
    const int oldSceneDepth = g_zVideo_D3DSceneDepth;
    const int oldSwLocked = g_zVideo_SwSurfaceState.locked;

    unsigned short fxPixels[25] = {};
    for (int index = 0; index < 25; ++index) {
        fxPixels[index] = 0;
    }

    HudWeatherFx softwareWeather = {};
    HudWeatherFxParticleQuad softwareQuad = {};
    HudUiRect softwareViewport = {0, 0, 4, 4};
    softwareQuad.x = 1;
    softwareQuad.y = 2;
    softwareQuad.width = 2;
    softwareQuad.height = 0;
    softwareQuad.color16 = 0xf800;
    softwareQuad.texCoordUStart = 1.0f;
    softwareQuad.texCoordUEnd = 1.0f;
    softwareQuad.slantOffset = 1;
    softwareWeather.clipRectOrNull = &softwareViewport;
    softwareWeather.particleQuads = &softwareQuad;
    softwareWeather.particleCount = 1;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_FxSurfacePixels16 = fxPixels;
    g_zVideo_FxSurfaceWidth = 5;
    g_zVideo_FxSurfaceHeight = 5;
    g_zVideo_FxSurfacePitchBytes = 10;
    g_zVideo_FxSurfacePitchPixels16 = 5;
    zRndr::g_pitchBytes = 10;
    zRndr::g_pixelPackGreenBits = 6;
    softwareWeather.ApplyPass3();
    const bool softwareOk = fxPixels[1 + 2 * 5] == 0xf800 &&
                            fxPixels[2 + 2 * 5] == 0xf800 &&
                            fxPixels[3 + 2 * 5] == 0xf800 &&
                            fxPixels[0 + 2 * 5] == 0x0000 &&
                            fxPixels[4 + 2 * 5] == 0x0000;

    unsigned short imagePixels[16] = {};
    char alphaMap[16] = {};
    zVidImagePartial image = {};
    image.pixels = imagePixels;
    image.alphaMap = alphaMap;
    zVideo_TextureRecordPartial textureRecord = {};
    HudWeatherFxParticleQuad hardwareQuads[2] = {};
    zVec3 positions[2] = {};
    HudUiRect hardwareViewport = {0, 0, 100, 100};
    HudWeatherFx hardwareWeather = {};
    hardwareQuads[0].x = 10;
    hardwareQuads[0].y = 20;
    hardwareQuads[0].width = 10;
    hardwareQuads[0].height = 2;
    hardwareQuads[0].color16 = 0x1234;
    hardwareQuads[0].texCoordUStart = 0.25f;
    hardwareQuads[0].texCoordUEnd = 0.75f;
    hardwareQuads[0].slantOffset = 3;
    hardwareQuads[1].x = 200;
    hardwareQuads[1].y = 200;
    hardwareQuads[1].width = 4;
    hardwareQuads[1].height = 12;
    hardwareQuads[1].texCoordUStart = 0.1f;
    hardwareQuads[1].texCoordUEnd = 0.9f;
    hardwareQuads[1].slantOffset = 5;
    positions[0].z = 0.6f;
    positions[1].z = 0.7f;
    hardwareWeather.clipRectOrNull = &hardwareViewport;
    hardwareWeather.particleQuads = hardwareQuads;
    hardwareWeather.particleCount = 2;
    hardwareWeather.packedColor16 = 0x1234;
    hardwareWeather.softwareImage = &image;
    hardwareWeather.textureRecord = &textureRecord;
    hardwareWeather.sourceBufferIndex = 0;
    hardwareWeather.particlePositions[0] = positions;
    g_weatherFinalizeUploadCount = 0;
    g_weatherSubmitPolyCount = 0;
    g_weatherFlushSortedCount = 0;
    g_weatherFinalizeTextureRecord = nullptr;
    g_weatherFinalizeImage = nullptr;
    g_weatherSubmittedVertexCount = 0;
    g_weatherSubmittedRenderClass = nullptr;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_pfnTextureRecordFinalizeUpload = HudWeatherFxFinalizeUploadStub;
    g_zVideo_pfnSubmitPolyRenderClass = HudWeatherFxSubmitPolyStub;
    g_zVideo_pfnFlushSortedPolys = HudWeatherFxFlushSortedStub;
    g_zVideo_D3DSceneDepth = 2;
    g_zVideo_SwSurfaceState.locked = 0;
    hardwareWeather.ApplyPass3();

    bool uploadPatternOk = true;
    for (int index = 0; index < 16; ++index) {
        uploadPatternOk = uploadPatternOk && imagePixels[index] == 0x1234 &&
                          static_cast<unsigned char>(alphaMap[index]) ==
                              static_cast<unsigned char>((index * 255) >> 4);
    }

    const bool hardwareUploadOk =
        g_weatherFinalizeUploadCount == 1 &&
        g_weatherFinalizeTextureRecord == &textureRecord && g_weatherFinalizeImage == &image;
    const bool hardwareSubmitOk =
        g_weatherSubmitPolyCount == 1 && g_weatherSubmittedVertexCount == 4 &&
        g_weatherSubmittedRenderClass == reinterpret_cast<zVideo_RenderClass *>(&textureRecord) &&
        g_weatherSubmittedRenderParam == 1 && g_weatherSubmittedAlpha == 1.0f &&
        g_weatherSubmittedQueueMode == 0 && g_weatherFlushSortedCount == 1 &&
        g_zVideo_D3DSceneDepth == 1;
    const bool vertexOk =
        g_weatherSubmittedVerts[0].x == 10.0f && g_weatherSubmittedVerts[0].y == 20.0f &&
        g_weatherSubmittedVerts[0].z == 0.6f && g_weatherSubmittedTexCoords[0].u == 0.25f &&
        g_weatherSubmittedVerts[1].x == 13.0f && g_weatherSubmittedVerts[1].y == 20.0f &&
        g_weatherSubmittedTexCoords[1].u == 0.25f &&
        g_weatherSubmittedVerts[2].x == 23.0f && g_weatherSubmittedVerts[2].y == 22.0f &&
        g_weatherSubmittedTexCoords[2].u == 0.75f &&
        g_weatherSubmittedVerts[3].x == 20.0f && g_weatherSubmittedVerts[3].y == 22.0f &&
        g_weatherSubmittedTexCoords[3].u == 0.75f &&
        g_weatherSubmittedTexCoords[0].v == 0.0f && g_weatherSubmittedTexCoords[3].v == 0.0f;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    g_zVideo_FxSurfacePixels16 = oldFxPixels;
    g_zVideo_FxSurfaceWidth = oldFxWidth;
    g_zVideo_FxSurfaceHeight = oldFxHeight;
    g_zVideo_FxSurfacePitchBytes = oldFxPitchBytes;
    g_zVideo_FxSurfacePitchPixels16 = oldFxPitchPixels16;
    zRndr::g_pitchBytes = oldPitchBytes;
    g_zVideo_pfnTextureRecordFinalizeUpload = oldFinalizeUpload;
    g_zVideo_pfnSubmitPolyRenderClass = oldSubmitPoly;
    g_zVideo_pfnFlushSortedPolys = oldFlushSorted;
    g_zVideo_D3DSceneDepth = oldSceneDepth;
    g_zVideo_SwSurfaceState.locked = oldSwLocked;

    return softwareOk && uploadPatternOk && hardwareUploadOk && hardwareSubmitOk && vertexOk
               ? 0
               : 1;
}

extern "C" int hud_weather_fx_derived_constructors_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    g_zVideo_ActiveRendererPath = 0;

    std::srand(2);
    HudWeatherFxSnow snow = {};
    HudWeatherFxSnow *const snowResult = snow.Constructor(2);
    const bool snowOk =
        snowResult == &snow &&
        snow.maxParticles == 2 && snow.particleCount == 2 && snow.emitEnabled == 1 &&
        snow.emitRadius == 20.0f && snow.emitDepth == 400.0f &&
        snow.softwareImage == nullptr && snow.textureRecord == nullptr &&
        snow.particlePositions[0][0].x == snow.particlePositions[1][0].x &&
        snow.particlePositions[0][1].z == snow.particlePositions[1][1].z;

    std::srand(3);
    HudWeatherFxRain rain = {};
    HudWeatherFxRain *const rainResult = rain.Constructor(1);
    const bool rainOk =
        rainResult == &rain &&
        rain.maxParticles == 1 && rain.particleCount == 1 && rain.emitEnabled == 1 &&
        rain.emitRadius == 20.0f && rain.emitDepth == 400.0f &&
        rain.softwareImage == nullptr && rain.textureRecord == nullptr &&
        rain.particlePositions[0][0].y == rain.particlePositions[1][0].y;

    std::srand(14);
    HudWeatherFxSnow destructSnow = {};
    destructSnow.Constructor(1);
    const bool snowDestructorAllocated =
        destructSnow.particleQuads != nullptr &&
        destructSnow.particlePositions[0] != nullptr &&
        destructSnow.particlePositions[1] != nullptr;
    destructSnow.Destructor();
    const bool snowDestructorOk =
        snowDestructorAllocated &&
        destructSnow.softwareImage == nullptr &&
        destructSnow.textureRecord == nullptr;

    ::operator delete(snow.particleQuads);
    ::operator delete(snow.particlePositions[0]);
    ::operator delete(snow.particlePositions[1]);
    ::operator delete(rain.particleQuads);
    ::operator delete(rain.particlePositions[0]);
    ::operator delete(rain.particlePositions[1]);

    g_zVideo_ActiveRendererPath = oldRendererPath;
    return snowOk && rainOk && snowDestructorOk ? 0 : 1;
}

extern "C" int hud_weather_fx_rain_destructor_smoke(void) {
    const int oldRendererPath = g_zVideo_ActiveRendererPath;
    g_zVideo_ActiveRendererPath = 0;

    std::srand(7);
    HudWeatherFxRain rain = {};
    rain.Constructor(2);
    const bool constructed =
        rain.particleQuads != nullptr &&
        rain.particlePositions[0] != nullptr && rain.particlePositions[1] != nullptr;

    rain.Destructor();
    const bool destroyed =
        rain.softwareImage == nullptr &&
        rain.textureRecord == nullptr;

    g_zVideo_ActiveRendererPath = oldRendererPath;
    return constructed && destroyed ? 0 : 1;
}

extern "C" int hud_weather_fx_rain_update_smoke(void) {
    const float oldLastTargetX = g_HudWeatherFxRain_LastCameraTargetX;
    const float oldLastTargetY = g_HudWeatherFxRain_LastCameraTargetY;
    const float oldLastTargetZ = g_HudWeatherFxRain_LastCameraTargetZ;
    const float oldTimeAccumulator = g_HudWeatherFxRain_TimeAccumulator;

    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.targetOrEuler = zVec3_Make(0.0f, 0.0f, 0.0f);
    cameraData.posOffset = zVec3_Make(0.0f, 0.0f, 0.0f);

    HudUiRect viewport = {0, 0, 100, 80};
    HudWeatherFxParticleQuad quads[1] = {};
    zVec3 positionsA[1] = {{0.0f, 0.0f, 0.75f}};
    zVec3 positionsB[1] = {};
    HudWeatherFxRain rain = {};
    rain.flags = 0x02;
    rain.clipRectOrNull = &viewport;
    rain.particleQuads = quads;
    rain.particleCount = 1;
    rain.packedColor16 = 0x2468;
    rain.alphaStartScale = 1.0f;
    rain.alphaEndScale = 0.0500000007f;
    rain.camera = &cameraNode;
    rain.activeParticleCount = 0;
    rain.particlePositions[0] = positionsA;
    rain.particlePositions[1] = positionsB;
    rain.sourceBufferIndex = 0;
    rain.destBufferIndex = 1;
    rain.windDirection = 0.0f;
    rain.windVelocity = 0.0f;
    rain.gravity = 0.0f;
    rain.basisVector = zVec3_Make(0.0f, 0.0f, 0.0f);
    g_HudWeatherFxRain_LastCameraTargetX = 0.0f;
    g_HudWeatherFxRain_LastCameraTargetY = 0.0f;
    g_HudWeatherFxRain_LastCameraTargetZ = 0.0f;
    g_HudWeatherFxRain_TimeAccumulator = 0.0f;

    std::srand(8);
    rain.Update(0.5f);

    const bool projected =
        rain.sourceBufferIndex == 1 && rain.destBufferIndex == 0 && quads[0].x == 50 &&
        quads[0].y == 40 && quads[0].width == 0 && quads[0].height == 0 &&
        quads[0].color16 == 0x2468 && HudFloatNear(quads[0].texCoordUStart, 0.75f) &&
        HudFloatNear(quads[0].texCoordUEnd, 0.0375000015f) &&
        quads[0].slantOffset == 1;
    const bool resetCopied =
        positionsA[0].x == positionsB[0].x && positionsA[0].y == positionsB[0].y &&
        positionsA[0].z == positionsB[0].z && positionsB[0].z >= 0.5f &&
        positionsB[0].z <= 1.0f;
    const bool globalsUpdated =
        HudFloatNear(g_HudWeatherFxRain_TimeAccumulator, 0.5f) &&
        g_HudWeatherFxRain_LastCameraTargetX == 0.0f &&
        g_HudWeatherFxRain_LastCameraTargetY == 0.0f &&
        g_HudWeatherFxRain_LastCameraTargetZ == 0.0f;

    g_HudWeatherFxRain_LastCameraTargetX = oldLastTargetX;
    g_HudWeatherFxRain_LastCameraTargetY = oldLastTargetY;
    g_HudWeatherFxRain_LastCameraTargetZ = oldLastTargetZ;
    g_HudWeatherFxRain_TimeAccumulator = oldTimeAccumulator;

    return projected && resetCopied && globalsUpdated ? 0 : 1;
}

extern "C" int hud_weather_fx_snow_update_smoke(void) {
    const float oldLastTargetX = g_HudWeatherFxSnow_LastCameraTargetX;
    const float oldLastTargetY = g_HudWeatherFxSnow_LastCameraTargetY;
    const float oldLastTargetZ = g_HudWeatherFxSnow_LastCameraTargetZ;
    const float oldTimeAccumulator = g_HudWeatherFxSnow_TimeAccumulator;

    zClass_NodePartial cameraNode = {};
    zClass_CameraDataPartial cameraData = {};
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraData.targetOrEuler = zVec3_Make(0.0f, 0.0f, 0.0f);
    cameraData.posOffset = zVec3_Make(0.0f, 0.0f, 0.0f);

    HudUiRect viewport = {0, 0, 100, 80};
    HudWeatherFxParticleQuad quads[2] = {};
    zVec3 positionsA[2] = {
        {0.0f, 0.0f, 0.75f},
        {0.0f, 0.0f, 1.25f},
    };
    zVec3 positionsB[2] = {};
    HudWeatherFxSnow snow = {};
    snow.flags = 0x02;
    snow.clipRectOrNull = &viewport;
    snow.particleQuads = quads;
    snow.particleCount = 2;
    snow.packedColor16 = 0x1357;
    snow.alphaStartScale = 1.0f;
    snow.alphaEndScale = 0.0500000007f;
    snow.camera = &cameraNode;
    snow.activeParticleCount = 0;
    snow.particlePositions[0] = positionsA;
    snow.particlePositions[1] = positionsB;
    snow.sourceBufferIndex = 0;
    snow.destBufferIndex = 1;
    snow.windDirection = 0.0f;
    snow.windVelocity = 0.0f;
    snow.gravity = 0.0f;
    snow.basisVector = zVec3_Make(0.0f, 0.0f, 0.0f);
    g_HudWeatherFxSnow_LastCameraTargetX = 0.0f;
    g_HudWeatherFxSnow_LastCameraTargetY = 0.0f;
    g_HudWeatherFxSnow_LastCameraTargetZ = 0.0f;
    g_HudWeatherFxSnow_TimeAccumulator = 0.0f;

    std::srand(6);
    snow.Update(0.25f);

    const bool firstParticleProjected =
        snow.sourceBufferIndex == 1 && snow.destBufferIndex == 0 &&
        positionsB[0].x == 0.0f && positionsB[0].y == 0.0f &&
        positionsB[0].z == 0.75f && quads[0].x == 50 && quads[0].y == 40 &&
        quads[0].width == 0 && quads[0].height == 0 &&
        quads[0].color16 == 0x1357 && HudFloatNear(quads[0].texCoordUStart, 0.75f) &&
        HudFloatNear(quads[0].texCoordUEnd, 0.0375000015f) &&
        quads[0].slantOffset == 2;
    const bool resetParticleCopied =
        positionsA[1].x == positionsB[1].x && positionsA[1].y == positionsB[1].y &&
        positionsA[1].z == positionsB[1].z && positionsB[1].z >= 0.5f &&
        positionsB[1].z <= 1.0f;
    const bool globalsUpdated =
        HudFloatNear(g_HudWeatherFxSnow_TimeAccumulator, 0.25f) &&
        g_HudWeatherFxSnow_LastCameraTargetX == 0.0f &&
        g_HudWeatherFxSnow_LastCameraTargetY == 0.0f &&
        g_HudWeatherFxSnow_LastCameraTargetZ == 0.0f;

    g_HudWeatherFxSnow_LastCameraTargetX = oldLastTargetX;
    g_HudWeatherFxSnow_LastCameraTargetY = oldLastTargetY;
    g_HudWeatherFxSnow_LastCameraTargetZ = oldLastTargetZ;
    g_HudWeatherFxSnow_TimeAccumulator = oldTimeAccumulator;

    return firstParticleProjected && resetParticleCopied && globalsUpdated ? 0 : 1;
}

