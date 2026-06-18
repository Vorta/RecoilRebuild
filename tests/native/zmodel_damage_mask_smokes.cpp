#include "GameZRecoil/include/OptCatalog.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zVideo/zVideo.h"

namespace {
int g_damageMaskUploadLockCount = 0;
int g_damageMaskUploadUnlockCount = 0;
int g_damageMaskUploadFinalizeCount = 0;
unsigned short *g_damageMaskUploadPixels = 0;
int g_damageMaskUploadPitchBytes = 0;
zVideo_TextureRecordPartial *g_damageMaskLastTextureRecord = 0;

int __fastcall DamageMaskLockUploadStub(
    zVideo_TextureRecordPartial *textureRecord,
    void **outPixels,
    int *outPitchBytes
) {
    ++g_damageMaskUploadLockCount;
    g_damageMaskLastTextureRecord = textureRecord;
    *outPixels = g_damageMaskUploadPixels;
    *outPitchBytes = g_damageMaskUploadPitchBytes;
    return 1;
}

int __fastcall DamageMaskUnlockUploadStub(
    zVideo_TextureRecordPartial *textureRecord
) {
    ++g_damageMaskUploadUnlockCount;
    g_damageMaskLastTextureRecord = textureRecord;
    return 1;
}

void __fastcall DamageMaskFinalizeUploadStub(
    zVideo_TextureRecordPartial *textureRecord,
    void *,
    zVidImagePartial *
) {
    ++g_damageMaskUploadFinalizeCount;
    g_damageMaskLastTextureRecord = textureRecord;
}
}

extern "C" int zmodel_damage_mask_uv_smoke() {
    int slotA = 0;
    int slotB = 0;
    int slotMissing = 0;
    g_OptCatalogDamageMaskHandles[0] = &slotA;
    g_OptCatalogDamageMaskHandles[1] = 0;
    g_OptCatalogDamageMaskHandles[2] = &slotB;

    const bool registeredOk = OptCatalog_IsDamageMaskSlotPtrRegistered(&slotA) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(&slotB) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(0) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(&slotMissing) == 0;

    g_OptCatalogDamageMaskPhaseU = 0.0f;
    g_OptCatalogDamageMaskPhaseV = 0.0f;
    g_OptCatalogDamageMaskEnabled = 0;

    OptCatalog_SetDamageMaskUv(0.25f, 0.75f);
    OptCatalog_SetDamageMaskEnabled(7);

    g_OptCatalogDamageMaskHandles[0] = 0;
    g_OptCatalogDamageMaskHandles[1] = 0;
    g_OptCatalogDamageMaskHandles[2] = 0;

    return registeredOk && g_OptCatalogDamageMaskPhaseU == 0.25f &&
                   g_OptCatalogDamageMaskPhaseV == 0.75f &&
                   OptCatalog_IsDamageMaskEnabled() == 7
               ? 0
               : 1;
}

extern "C" int zmodel_damage_mask_stamp_smoke() {
    void *const oldSlot0 = g_OptCatalogDamageMaskHandles[0];
    void *const oldSlot1 = g_OptCatalogDamageMaskHandles[1];
    void *const oldSlot2 = g_OptCatalogDamageMaskHandles[2];
    const int oldEnabled = g_OptCatalogDamageMaskEnabled;
    const int oldSlotIndex = g_OptCatalogDamageMaskSlotIndex;
    const float oldPhaseU = g_OptCatalogDamageMaskPhaseU;
    const float oldPhaseV = g_OptCatalogDamageMaskPhaseV;
    const int oldGreenBits = zRndr::g_pixelPackGreenBits;
    zVideo_TextureRecordLockUploadSurfaceProc oldLock =
        g_zVideo_pfnTextureRecordLockUploadSurface;
    zVideo_TextureRecordUnlockUploadSurfaceProc oldUnlock =
        g_zVideo_pfnTextureRecordUnlockUploadSurface;
    zVideo_TextureRecordFinalizeUploadProc oldFinalize =
        g_zVideo_pfnTextureRecordFinalizeUpload;

    unsigned short srcPixels[9] = {0, 0xf800, 0x07e0, 0x001f, 0x7fff,
                                   0xffff, 0x1234, 0x2222, 0x3333};
    OptCatalogDamageMaskSurface srcSurface = {};
    srcSurface.width = 3;
    srcSurface.height = 3;
    srcSurface.pixels = srcPixels;
    OptCatalogSurfaceTextureHandle srcHandle = {};
    srcHandle.surface = &srcSurface;

    unsigned short dstPixels[25] = {};
    for (int i = 0; i < 25; ++i) {
        dstPixels[i] = 0x1111;
    }
    OptCatalogDamageMaskSurface dstSurface = {};
    dstSurface.width = 5;
    dstSurface.height = 5;
    dstSurface.pixels = dstPixels;
    OptCatalogSurfaceTextureHandle dstHandle = {};
    dstHandle.surface = &dstSurface;
    OptCatalogSurfaceMaterialRef material = {};
    material.flags = 0x0300;
    material.textureHandle = &dstHandle;
    OptCatalogHitEventPartial hitEvent = {};
    hitEvent.surfaceRef = &material;

    g_OptCatalogDamageMaskEnabled = 0;
    OptCatalog::SetDamageMaskSlotIndex(1);
    OptCatalog::RegisterDamageMaskSlotPtr(&srcHandle);
    if (g_OptCatalogDamageMaskEnabled != 1 || g_OptCatalogDamageMaskSlotIndex != 1 ||
        g_OptCatalogDamageMaskHandles[1] != &srcHandle) {
        return 1;
    }

    g_OptCatalogDamageMaskPhaseU = 0.5f;
    g_OptCatalogDamageMaskPhaseV = 0.5f;
    OptCatalog::ApplyDamageMaskStampOnHit(&hitEvent);
    if (dstPixels[6] != 0x1111 || dstPixels[7] != 0xf800 || dstPixels[8] != 0x07e0 ||
        dstPixels[11] != 0x001f || dstPixels[12] != 0x7fff || dstPixels[13] != 0xffff ||
        dstPixels[16] != 0x1234 || dstPixels[17] != 0x2222 || dstPixels[18] != 0x3333) {
        return 2;
    }

    unsigned char alphaOne = 0x80;
    unsigned short alphaSrcPixel = 0xf800;
    unsigned short alphaDstPixel = 0x001f;
    srcSurface.width = 1;
    srcSurface.height = 1;
    srcSurface.pixels = &alphaSrcPixel;
    srcSurface.alpha = &alphaOne;
    dstSurface.width = 1;
    dstSurface.height = 1;
    dstSurface.pixels = &alphaDstPixel;
    dstHandle.textureRecord = 0;
    g_OptCatalogDamageMaskPhaseU = 0.5f;
    g_OptCatalogDamageMaskPhaseV = 0.5f;
    zRndr::g_pixelPackGreenBits = 6;
    OptCatalog::ApplyDamageMaskStampOnHit(&hitEvent);
    if (alphaDstPixel != 0x780f) {
        return 3;
    }

    alphaSrcPixel = 0x7c00;
    alphaDstPixel = 0x001f;
    zRndr::g_pixelPackGreenBits = 5;
    OptCatalog::ApplyDamageMaskStampOnHit(&hitEvent);
    if (alphaDstPixel != 0x3c0f) {
        return 4;
    }

    unsigned short uploadSrcPixel = 0x4567;
    unsigned short uploadPixels[4] = {0, 0, 0, 0};
    zVideo_TextureRecordPartial textureRecord = {};
    srcSurface.alpha = 0;
    srcSurface.pixels = &uploadSrcPixel;
    dstSurface.width = 2;
    dstSurface.height = 2;
    dstSurface.pixels = 0;
    dstHandle.textureRecord = &textureRecord;
    g_damageMaskUploadPixels = uploadPixels;
    g_damageMaskUploadPitchBytes = 4;
    g_damageMaskUploadLockCount = 0;
    g_damageMaskUploadUnlockCount = 0;
    g_damageMaskUploadFinalizeCount = 0;
    g_damageMaskLastTextureRecord = 0;
    g_zVideo_pfnTextureRecordLockUploadSurface = DamageMaskLockUploadStub;
    g_zVideo_pfnTextureRecordUnlockUploadSurface = DamageMaskUnlockUploadStub;
    g_zVideo_pfnTextureRecordFinalizeUpload = DamageMaskFinalizeUploadStub;
    g_OptCatalogDamageMaskPhaseU = 0.5f;
    g_OptCatalogDamageMaskPhaseV = 0.5f;
    OptCatalog::ApplyDamageMaskStampOnHit(&hitEvent);
    const bool uploadOk = uploadPixels[3] == 0x4567 && g_damageMaskUploadLockCount == 1 &&
                          g_damageMaskUploadUnlockCount == 1 &&
                          g_damageMaskUploadFinalizeCount == 1 &&
                          g_damageMaskLastTextureRecord == &textureRecord;

    g_OptCatalogDamageMaskHandles[0] = oldSlot0;
    g_OptCatalogDamageMaskHandles[1] = oldSlot1;
    g_OptCatalogDamageMaskHandles[2] = oldSlot2;
    g_OptCatalogDamageMaskEnabled = oldEnabled;
    g_OptCatalogDamageMaskSlotIndex = oldSlotIndex;
    g_OptCatalogDamageMaskPhaseU = oldPhaseU;
    g_OptCatalogDamageMaskPhaseV = oldPhaseV;
    zRndr::g_pixelPackGreenBits = oldGreenBits;
    g_zVideo_pfnTextureRecordLockUploadSurface = oldLock;
    g_zVideo_pfnTextureRecordUnlockUploadSurface = oldUnlock;
    g_zVideo_pfnTextureRecordFinalizeUpload = oldFinalize;

    return uploadOk ? 0 : 5;
}
