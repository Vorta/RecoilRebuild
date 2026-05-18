#include "Battlesport/HudSensorTracker.h"
#include "Battlesport/player.h"
#include "GameZRecoil/Time/Time.h"
#include "GameZRecoil/include/zClipRect.h"
#include "GameZRecoil/zEffect/zEffect.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zMath/zMath.h"
#include "GameZRecoil/zModel/zModel.h"
#include "GameZRecoil/zRndr/zRndr.h"
#include "GameZRecoil/zSound/zSound.h"
#include "GameZRecoil/zUtil/zSaveGame.h"
#include "GameZRecoil/zVideo/zVideo.h"
#include "OptCatalog.h"
#include "zClass.h"
#include "zClipAlt.h"
#include "zDi.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>

namespace {
std::int32_t RECOIL_FASTCALL zclass_test_node_type_0x42(zClass_NodePartial *node) {
    return node->nodeType == 0x42 ? 1 : 0;
}

float RECOIL_FASTCALL zclass_damage_timer_stub(void *context, float damageAmount) {
    *static_cast<float *>(context) = damageAmount;
    return damageAmount + 1.0f;
}

zProjectedPoint g_drawPointLastPoint{};
std::uint32_t g_drawPointLastColor = 0;
std::int32_t g_drawPointLastCount = 0;
std::int32_t g_drawPointCallCount = 0;

void RECOIL_FASTCALL zmodel_draw_point_color16_stub(zProjectedPoint *point,
                                                    std::uint32_t packedColor16,
                                                    std::int32_t pointCount) {
    g_drawPointLastPoint = *point;
    g_drawPointLastColor = packedColor16;
    g_drawPointLastCount = pointCount;
    ++g_drawPointCallCount;
}

void reset_zclass_type_lists_for_test() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;
}

void free_zclass_type_lists_for_test() {
    for (int i = 0; i < 16; ++i) {
        for (zClass_TypeListLink *link = zClass_TypeList::Head(i); link != nullptr;) {
            zClass_TypeListLink *const next = link->next;
            std::free(link);
            link = next;
        }
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    for (zClass_TypeListLink *link = g_zClass_TypeList_FreeLinkHead; link != nullptr;) {
        zClass_TypeListLink *const next = link->next;
        std::free(link);
        link = next;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
}
} // namespace

extern "C" int zgame_return_only_stub_smoke() {
    zGame::ReturnOnlyStub();
    return 0;
}

extern "C" int zgame_options_init_registry_context_smoke() {
    std::free(g_zGame_Options_RegKeyRoot);
    std::free(g_zGame_Options_RegKeyCurrentUser);
    std::free(g_zGame_Options_RegKeyGame);
    g_zGame_Options_RegKeyRoot = nullptr;
    g_zGame_Options_RegKeyCurrentUser = nullptr;
    g_zGame_Options_RegKeyGame = nullptr;
    g_zGame_Options_RegContextInitialized = 0;

    zOptionEntryPartial option = {};
    g_zGame_Options_OptionListHead = &option;
    zGame::Options_InitRegistryContext("root", "user", "game");

    const bool ok = g_zGame_Options_RegContextInitialized == 1 &&
                    g_zGame_Options_OptionListHead == nullptr &&
                    g_zGame_Options_RegKeyRoot != nullptr &&
                    std::strcmp(g_zGame_Options_RegKeyRoot, "root") == 0 &&
                    g_zGame_Options_RegKeyCurrentUser != nullptr &&
                    std::strcmp(g_zGame_Options_RegKeyCurrentUser, "user") == 0 &&
                    g_zGame_Options_RegKeyGame != nullptr &&
                    std::strcmp(g_zGame_Options_RegKeyGame, "game") == 0 &&
                    g_zGame_Options_RuntimeConfigDefaults.cpuVendor[0] != '\0' &&
                    g_zGame_Options_RuntimeConfigDefaults.systemRamKb != 0;

    std::free(g_zGame_Options_RegKeyRoot);
    std::free(g_zGame_Options_RegKeyCurrentUser);
    std::free(g_zGame_Options_RegKeyGame);
    g_zGame_Options_RegKeyRoot = nullptr;
    g_zGame_Options_RegKeyCurrentUser = nullptr;
    g_zGame_Options_RegKeyGame = nullptr;
    return ok ? 0 : 1;
}

extern "C" int zmodel_display_init_smoke() {
    g_zVideo_ActiveRendererPath = 0;
    gModel_DefaultGraphicsFlags = 0;
    g_zModel_GraphicsFlagsOption = nullptr;
    g_Variant_CurrentTag.count = 3;
    g_Variant_CurrentTag.tags[0] = 1;

    if (zModel_Display_Init() != 0 || g_zModel_DisplayClipMode != 2 || g_zModel_DisplayClipX != 0 ||
        g_zModel_DisplayClipY != 0 || gModel_FogEnabled != 1 || gModel_FogLinearModeEnabled != 1 ||
        gModel_FogDistanceStart != 500.0f || gModel_FogDistanceEnd != 700.0f ||
        gModel_FogDistanceInvRange != 0.005f || gModel_FogHeightHigh != 300.0f ||
        gModel_FogHeightLow != 200.0f || gModel_FogHeightInvRange != 0.01f ||
        gModel_FogDensity != 2.0f || g_zModel_InverseZTolerance != 0.01f ||
        gModel_DefaultGraphicsFlags != -1 ||
        g_zModel_GraphicsFlagsOption != &gModel_DefaultGraphicsFlags ||
        g_Variant_CurrentTag.count != 0 || g_Variant_CurrentTag.tags[0] != 0xff) {
        return 1;
    }

    char optionName[] = "GfxFlags_HW";
    zOptionEntryPartial option = {};
    option.name = optionName;
    option.next = nullptr;
    g_zGame_Options_OptionListHead = &option;
    g_zVideo_ActiveRendererPath = 1;
    g_zModel_HardwareInverseZTolerance = 0.0f;
    g_zModel_GraphicsFlagsOption = nullptr;

    const bool hardwareOk = zModel_Display_Init() == 0 && g_zModel_InverseZTolerance == 0.02f &&
                            g_zModel_HardwareInverseZTolerance == 0.02f &&
                            g_zModel_GraphicsFlagsOption == &option;

    g_zGame_Options_OptionListHead = nullptr;
    g_zVideo_ActiveRendererPath = 0;
    return hardwareOk ? 0 : 2;
}

extern "C" int zmodel_damage_mask_uv_smoke() {
    int slotA = 0;
    int slotB = 0;
    int slotMissing = 0;
    g_OptCatalogDamageMaskHandles[0] = &slotA;
    g_OptCatalogDamageMaskHandles[1] = nullptr;
    g_OptCatalogDamageMaskHandles[2] = &slotB;

    const bool registeredOk = OptCatalog_IsDamageMaskSlotPtrRegistered(&slotA) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(&slotB) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(nullptr) == 1 &&
                              OptCatalog_IsDamageMaskSlotPtrRegistered(&slotMissing) == 0;

    g_OptCatalogDamageMaskPhaseU = 0.0f;
    g_OptCatalogDamageMaskPhaseV = 0.0f;

    OptCatalog_SetDamageMaskUv(0.25f, 0.75f);

    g_OptCatalogDamageMaskHandles[0] = nullptr;
    g_OptCatalogDamageMaskHandles[1] = nullptr;
    g_OptCatalogDamageMaskHandles[2] = nullptr;

    return registeredOk && g_OptCatalogDamageMaskPhaseU == 0.25f &&
                   g_OptCatalogDamageMaskPhaseV == 0.75f
               ? 0
               : 1;
}

extern "C" int zmodel_display_shutdown_smoke() {
    g_zModel_DiPoolCapacity = 1;
    g_zModel_DiPoolInUseCount = 1;
    g_zModel_DiPoolFreeHeadIndex = 7;
    g_zModel_DiPoolBase = static_cast<zDiPartial *>(std::calloc(1, sizeof(zDiPartial)));
    g_zModel_DiPoolBase[0].entries =
        static_cast<zDiEntryPartial *>(std::malloc(sizeof(zDiEntryPartial)));
    g_zModel_MatlPool = static_cast<zModel_MaterialSlot *>(std::malloc(4));
    g_zModel_MatlPoolCapacity = 2;
    g_zModel_MatlPoolInUseCount = 1;
    g_zModel_MatlFreeHeadIndex = 3;
    g_zModel_MatlActiveHeadIndex = 4;
    g_zModel_MatlReuseCache = &g_zModel_DefaultMaterial;

    if (zModel_Display::ShutdownThunk() != 0) {
        return 1;
    }

    return g_zModel_DiPoolBase == nullptr && g_zModel_DiPoolCapacity == 0 &&
                   g_zModel_DiPoolInUseCount == 0 && g_zModel_DiPoolFreeHeadIndex == -1 &&
                   g_zModel_MatlPool == nullptr && g_zModel_MatlPoolCapacity == 0 &&
                   g_zModel_MatlPoolInUseCount == 0 && g_zModel_MatlFreeHeadIndex == -1 &&
                   g_zModel_MatlActiveHeadIndex == -1 && g_zModel_MatlReuseCache == nullptr
               ? 0
               : 2;
}

extern "C" int zmodel_material_pool_entry_smoke() {
    zModel_MaterialSlot slots[3] = {};
    g_zModel_MatlPool = slots;

    const bool ok = zModel_Matl::GetPoolEntry(-1) == nullptr &&
                    zModel_Matl::GetPoolEntry(0) == &slots[0] &&
                    zModel_Matl::GetPoolEntry(2) == &slots[2] &&
                    zModel_MatlSlot::IndexFromPtrOrMinus1(nullptr) == -1 &&
                    zModel_MatlSlot::IndexFromPtrOrMinus1(&slots[2]) == 2;

    g_zModel_MatlPool = nullptr;
    return ok ? 0 : 1;
}

extern "C" int zmodel_material_defaults_and_find_smoke() {
    zImage_TexDirEntryPartial texA{};
    zImage_TexDirEntryPartial texB{};
    zImage_TexDirEntryPartial texMissing{};

    zModel_MaterialCyclePartial cycle{};
    zModel_MaterialPartial material{};
    material.flags = 0xffff;
    material.packedColor = 1;
    material.colorRgb.red = 1.0f;
    material.currentTextureDirectoryEntry = &texA;
    material.unknown_14 = 7.0f;
    material.colorScalar = 8.0f;
    material.unknown_1c = 9.0f;
    material.userTag = 10;
    material.cycle = &cycle;

    zModel_Material::ResetDefaults(&material);
    const bool defaultsOk =
        material.flags == 0xf8ff && material.packedColor == 0x7fff &&
        material.colorRgb.red == 255.0f && material.colorRgb.green == 255.0f &&
        material.colorRgb.blue == 255.0f && material.currentTextureDirectoryEntry == nullptr &&
        material.unknown_14 == 0.0f && material.colorScalar == 0.5f &&
        material.unknown_1c == 0.5f && material.userTag == 0 && material.cycle == nullptr;

    zModel_MaterialSlot slots[3] = {};
    slots[0].material.currentTextureDirectoryEntry = &texA;
    slots[0].nextPoolIndex = 2;
    slots[2].material.currentTextureDirectoryEntry = &texB;
    slots[2].nextPoolIndex = -1;
    g_zModel_MatlPool = slots;
    g_zModel_MatlActiveHeadIndex = 0;

    const bool findOk = zModel_Material::FindByTexDirEntry(nullptr) == nullptr &&
                        zModel_Material::FindByTexDirEntry(&texA) == &slots[0].material &&
                        zModel_Material::FindByTexDirEntry(&texB) == &slots[2].material &&
                        zModel_Material::FindByTexDirEntry(&texMissing) == nullptr;

    zModel_MaterialPartial compareA = material;
    zModel_MaterialPartial compareB = material;
    compareA.currentTextureDirectoryEntry = &texA;
    compareB.currentTextureDirectoryEntry = &texA;
    compareA.userTag = 0;
    compareB.userTag = 77;
    const bool tagPromoteLeft =
        zModel_Material::CompareForReuse(&compareA, &compareB) == 0 && compareA.userTag == 77;
    compareA.userTag = 55;
    compareB.userTag = 0;
    const bool tagPromoteRight =
        zModel_Material::CompareForReuse(&compareA, &compareB) == 0 && compareB.userTag == 55;
    compareB.userTag = 66;
    const bool tagConflict = zModel_Material::CompareForReuse(&compareA, &compareB) != 0;
    compareB.userTag = 55;
    compareB.currentTextureDirectoryEntry = &texB;
    const bool textureMismatch = zModel_Material::CompareForReuse(&compareA, &compareB) == 1;
    compareB.currentTextureDirectoryEntry = &texA;
    compareB.colorRgb.red = 1.0f;
    const bool bytesMismatch = zModel_Material::CompareForReuse(&compareA, &compareB) != 0;

    zModel_MaterialPartial activeRequest = slots[2].material;
    activeRequest.userTag = 99;
    g_zModel_MatlReuseCache = nullptr;
    const bool findOrCloneActive =
        zModel_Material::FindOrClone(&activeRequest) == &slots[2].material &&
        slots[2].material.userTag == 99;

    zModel_MaterialPartial cacheMaterial = slots[0].material;
    zModel_MaterialPartial cacheRequest = cacheMaterial;
    g_zModel_MatlReuseCache = &cacheMaterial;
    const bool findOrCloneCache = zModel_Material::FindOrClone(&cacheRequest) == &cacheMaterial;

    const bool setTagOk = zModel_Material::SetUserTag(nullptr, 7) == 0 &&
                          zModel_Material::SetUserTag(&compareA, 88) == 1 && compareA.userTag == 88;

    zModel_MaterialPartial cycleMaterial = {};
    cycleMaterial.flags = 0x0001;
    const bool cycleCountDefaultReject =
        zModel_Material::SetCycleTextureCount(&g_zModel_DefaultMaterial, 2) == 0;
    const bool cycleCountOk =
        zModel_Material::SetCycleTextureCount(&cycleMaterial, 3) == 1 &&
        (cycleMaterial.flags & 0x0500) == 0x0500 && cycleMaterial.cycle != nullptr &&
        cycleMaterial.cycle->loopEnabled == 0 && cycleMaterial.cycle->currentFrame == 0.0f &&
        cycleMaterial.cycle->framesPerSecond == 15.0f && cycleMaterial.cycle->frameCount == 3 &&
        cycleMaterial.cycle->frameWriteCount == 0 && cycleMaterial.cycle->frameTable != nullptr &&
        cycleMaterial.cycle->frameTable[0] == zImage::GetDefaultImageRefPtr() &&
        reinterpret_cast<zVidImagePartial **>(zImage::GetDefaultImageRefPtr())[0] ==
            &zVid_Image::g_zImage_DefaultImage;
    const bool cycleNoResize = zModel_Material::SetCycleTextureCount(&cycleMaterial, 2) == 0;
    zImage_TexDirEntryPartial cycleTexture = {};
    const bool cycleAddOk = zModel_Material::AddCycleTexture(&cycleMaterial, &cycleTexture) == 1 &&
                            cycleMaterial.cycle->frameTable[0] == &cycleTexture &&
                            cycleMaterial.cycle->frameWriteCount == 1;
    const bool cycleLoopSpeedOk =
        zModel_Material::SetCycleTextureLoop(&cycleMaterial, 1) == 1 &&
        cycleMaterial.cycle->loopEnabled == 1 &&
        zModel_Material::SetCycleTextureSpeed(&cycleMaterial, 7.5f) == 1 &&
        cycleMaterial.cycle->framesPerSecond == 7.5f;
    zModel_MaterialPartial nonCycleMaterial = {};
    const bool cycleRejects =
        zModel_Material::AddCycleTexture(&nonCycleMaterial, &cycleTexture) == 0 &&
        zModel_Material::SetCycleTextureLoop(nullptr, 1) == 0 &&
        zModel_Material::SetCycleTextureSpeed(nullptr, 1.0f) == 0;

    std::free(cycleMaterial.cycle->frameTable);
    std::free(cycleMaterial.cycle);

    g_zModel_MatlPool = nullptr;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlReuseCache = nullptr;
    return defaultsOk && findOk && tagPromoteLeft && tagPromoteRight && tagConflict &&
                   textureMismatch && bytesMismatch && findOrCloneActive && findOrCloneCache &&
                   setTagOk && cycleCountDefaultReject && cycleCountOk && cycleNoResize &&
                   cycleAddOk && cycleLoopSpeedOk && cycleRejects
               ? 0
               : 1;
}

extern "C" int zmodel_material_write_gamez_smoke() {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));

    zImage_TexDirEntryPartial *frameTable[2] = {
        &g_zImage_TexDirEntries[2],
        nullptr,
    };
    zModel_MaterialCyclePartial cycle{};
    cycle.frameCount = 2;
    cycle.frameTable = frameTable;

    zModel_MaterialSlot slots[2]{};
    slots[0].material.flags = 0x0500;
    slots[0].material.currentTextureDirectoryEntry = &g_zImage_TexDirEntries[1];
    slots[0].material.cycle = &cycle;
    slots[0].prevPoolIndex = -1;
    slots[0].nextPoolIndex = 1;
    slots[1].material.flags = 0x0100;
    slots[1].material.currentTextureDirectoryEntry = &g_zImage_TexDirEntries[0];
    slots[1].prevPoolIndex = 0;
    slots[1].nextPoolIndex = -1;

    g_zModel_MatlPool = slots;
    g_zModel_MatlPoolCapacity = 2;
    g_zModel_MatlPoolInUseCount = 2;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = 0;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (zModel_MatlBuffer::WriteGameZ(file) != 2) {
        std::fclose(file);
        return 2;
    }

    const long poolOffset = 16;
    const long poolBytes = static_cast<long>(sizeof(zModel_MaterialSlot) * 2);
    const long cycleOffset = poolOffset + poolBytes;
    const long frameOffset = cycleOffset + static_cast<long>(sizeof(zModel_MaterialCyclePartial));
    const long expectedBytes = frameOffset + 8;
    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedBytes) {
        std::fclose(file);
        return 3;
    }

    std::int32_t header[4] = {};
    zModel_MaterialSlot writtenSlots[2]{};
    std::rewind(file);
    if (std::fread(header, sizeof(header), 1, file) != 1 || header[0] != 2 || header[1] != 2 ||
        header[2] != -1 || header[3] != 0 ||
        std::fread(writtenSlots, sizeof(writtenSlots), 1, file) != 1) {
        std::fclose(file);
        return 4;
    }

    if (reinterpret_cast<std::intptr_t>(writtenSlots[0].material.currentTextureDirectoryEntry) !=
            1 ||
        reinterpret_cast<std::intptr_t>(writtenSlots[1].material.currentTextureDirectoryEntry) !=
            0 ||
        slots[0].material.currentTextureDirectoryEntry != &g_zImage_TexDirEntries[1]) {
        std::fclose(file);
        return 5;
    }

    zModel_MaterialCyclePartial writtenCycle{};
    std::int32_t frameIndices[2] = {};
    if (std::fread(&writtenCycle, sizeof(writtenCycle), 1, file) != 1 ||
        writtenCycle.frameCount != 2 ||
        std::fread(frameIndices, sizeof(frameIndices), 1, file) != 1 || frameIndices[0] != 2 ||
        frameIndices[1] != -1) {
        std::fclose(file);
        return 6;
    }

    std::fclose(file);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    return 0;
}

extern "C" int zmodel_material_read_gamez_smoke() {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zModel_MaterialSlot serializedSlots[3] = {};
    serializedSlots[0].material.colorRgb.red = 255.0f;
    serializedSlots[0].material.colorRgb.green = 0.0f;
    serializedSlots[0].material.colorRgb.blue = 0.0f;
    serializedSlots[0].prevPoolIndex = -1;
    serializedSlots[0].nextPoolIndex = 1;

    serializedSlots[1].material.flags = 0x0100;
    serializedSlots[1].material.currentTextureDirectoryEntry =
        reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<std::intptr_t>(2));
    serializedSlots[1].prevPoolIndex = 0;
    serializedSlots[1].nextPoolIndex = 2;

    serializedSlots[2].material.flags = 0x0500;
    serializedSlots[2].material.currentTextureDirectoryEntry =
        reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<std::intptr_t>(1));
    serializedSlots[2].prevPoolIndex = 1;
    serializedSlots[2].nextPoolIndex = -1;

    zModel_MaterialCyclePartial serializedCycle = {};
    serializedCycle.currentFrame = 0.5f;
    serializedCycle.frameCount = 2;
    zImage_TexDirEntryPartial *serializedFrames[2] = {
        reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<std::intptr_t>(0)),
        reinterpret_cast<zImage_TexDirEntryPartial *>(static_cast<std::intptr_t>(-1)),
    };

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::int32_t header[4] = {3, 3, -1, 0};
    if (std::fwrite(header, sizeof(header), 1, file) != 1 ||
        std::fwrite(serializedSlots, sizeof(serializedSlots), 1, file) != 1 ||
        std::fwrite(&serializedCycle, sizeof(serializedCycle), 1, file) != 1 ||
        std::fwrite(serializedFrames, sizeof(serializedFrames), 1, file) != 1) {
        std::fclose(file);
        return 2;
    }

    g_zModel_MatlPool =
        static_cast<zModel_MaterialSlot *>(std::malloc(sizeof(zModel_MaterialSlot)));
    if (g_zModel_MatlPool == nullptr) {
        std::fclose(file);
        return 3;
    }
    g_zModel_MatlPoolCapacity = 1;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;

    std::rewind(file);
    if (zModel_MatlBuffer::ReadGameZ(file) != 3) {
        std::fclose(file);
        std::free(g_zModel_MatlPool);
        g_zModel_MatlPool = nullptr;
        g_zModel_MatlPoolCapacity = 0;
        return 4;
    }
    std::fclose(file);

    zModel_MaterialSlot *const loadedSlots = g_zModel_MatlPool;
    zModel_MaterialCyclePartial *const loadedCycle = loadedSlots[2].material.cycle;
    const bool ok =
        g_zModel_MatlPoolCapacity == 3 && g_zModel_MatlPoolInUseCount == 3 &&
        g_zModel_MatlFreeHeadIndex == -1 && g_zModel_MatlActiveHeadIndex == 0 &&
        loadedSlots[0].material.packedColor == 0xf800 &&
        loadedSlots[1].material.currentTextureDirectoryEntry == &g_zImage_TexDirEntries[2] &&
        loadedSlots[2].material.currentTextureDirectoryEntry == &g_zImage_TexDirEntries[1] &&
        loadedCycle != nullptr && loadedCycle->frameCount == 2 &&
        loadedCycle->frameTable != nullptr &&
        loadedCycle->frameTable[0] == &g_zImage_TexDirEntries[0] &&
        loadedCycle->frameTable[1] == nullptr;

    if (loadedCycle != nullptr) {
        std::free(loadedCycle->frameTable);
        std::free(loadedCycle);
    }
    std::free(g_zModel_MatlPool);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;

    return ok ? 0 : 5;
}

extern "C" int zmodel_dipool_write_to_stream_smoke() {
    zVec3 verts[1] = {{1.0f, 2.0f, 3.0f}};
    zVec3 blendVerts[1] = {{4.0f, 5.0f, 6.0f}};
    zVec3 pointCamList[1] = {{7.0f, 8.0f, 9.0f}};
    zModel_PointEntryPartial pointEntries[1] = {};
    pointEntries[0].pointCamCount = 1;
    pointEntries[0].pointCamList = pointCamList;

    std::int32_t vertexIndices[3] = {0, 1, 2};
    std::int32_t normalIndices[3] = {2, 1, 0};
    zModel_Uv uvPairs[3] = {
        {0.0f, 0.25f},
        {0.5f, 0.75f},
        {1.0f, 1.25f},
    };

    zModel_MaterialSlot materialSlots[2] = {};
    materialSlots[1].material.flags = 0x0100;
    g_zModel_MatlPool = materialSlots;

    zDiEntryPartial entries[1] = {};
    entries[0].flagsAndIndexCount = 0x0203;
    entries[0].vertexIndices = vertexIndices;
    entries[0].normalIndices = normalIndices;
    entries[0].uvPairs = uvPairs;
    entries[0].material = &materialSlots[1].material;

    zDiPartial pool[2] = {};
    pool[0].entryCount = 1;
    pool[0].vertCount = 1;
    pool[0].blendVertCount = 1;
    pool[0].pointCount = 1;
    pool[0].entries = entries;
    pool[0].verts = verts;
    pool[0].pointEntries = pointEntries;
    pool[0].blendVerts = blendVerts;
    pool[0].nextFreeIndex = -1;
    pool[1].nextFreeIndex = 7;

    g_zModel_DiPoolBase = pool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 1;
    g_zModel_DiPoolFreeHeadIndex = 1;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (zModel_DiPool::WriteToStream(file) != 2) {
        std::fclose(file);
        return 2;
    }

    const long tableOffset = 12;
    const long tableBytes = static_cast<long>(sizeof(zDiPartial) * 2);
    const long dynamicOffset = tableOffset + tableBytes;
    const long expectedBytes =
        dynamicOffset + static_cast<long>(sizeof(zVec3)) + static_cast<long>(sizeof(zVec3)) +
        static_cast<long>(sizeof(zModel_PointEntryPartial)) + static_cast<long>(sizeof(zVec3)) +
        static_cast<long>(sizeof(zDiEntryPartial)) + static_cast<long>(sizeof(vertexIndices)) +
        static_cast<long>(sizeof(normalIndices)) + static_cast<long>(sizeof(uvPairs));

    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedBytes ||
        pool[0].nextFreeIndex != dynamicOffset) {
        std::fclose(file);
        return 3;
    }

    std::int32_t header[3] = {};
    zDiPartial writtenPool[2] = {};
    std::rewind(file);
    if (std::fread(header, sizeof(header), 1, file) != 1 || header[0] != 2 || header[1] != 1 ||
        header[2] != 1 || std::fread(writtenPool, sizeof(writtenPool), 1, file) != 1 ||
        writtenPool[0].nextFreeIndex != dynamicOffset || writtenPool[1].nextFreeIndex != 7) {
        std::fclose(file);
        return 4;
    }

    zVec3 writtenVerts[2] = {};
    zModel_PointEntryPartial writtenPoint = {};
    zVec3 writtenPointCam = {};
    zDiEntryPartial writtenEntry = {};
    std::int32_t writtenVertexIndices[3] = {};
    std::int32_t writtenNormalIndices[3] = {};
    zModel_Uv writtenUvs[3] = {};
    if (std::fread(&writtenVerts[0], sizeof(zVec3), 1, file) != 1 ||
        std::fread(&writtenVerts[1], sizeof(zVec3), 1, file) != 1 ||
        std::fread(&writtenPoint, sizeof(writtenPoint), 1, file) != 1 ||
        std::fread(&writtenPointCam, sizeof(writtenPointCam), 1, file) != 1 ||
        std::fread(&writtenEntry, sizeof(writtenEntry), 1, file) != 1 ||
        std::fread(writtenVertexIndices, sizeof(writtenVertexIndices), 1, file) != 1 ||
        std::fread(writtenNormalIndices, sizeof(writtenNormalIndices), 1, file) != 1 ||
        std::fread(writtenUvs, sizeof(writtenUvs), 1, file) != 1) {
        std::fclose(file);
        return 5;
    }

    const bool ok =
        writtenVerts[0].x == 1.0f && writtenVerts[1].x == 4.0f && writtenPoint.pointCamCount == 1 &&
        writtenPointCam.z == 9.0f && reinterpret_cast<std::intptr_t>(writtenEntry.material) == 1 &&
        writtenVertexIndices[2] == 2 && writtenNormalIndices[0] == 2 && writtenUvs[2].v == 1.25f;

    std::fclose(file);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zModel_MatlPool = nullptr;
    return ok ? 0 : 6;
}

extern "C" int zmodel_dipool_read_from_stream_smoke() {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);

    zModel_MaterialSlot materialSlots[1] = {};
    materialSlots[0].material.flags = 0x0100;
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 1;

    zDiPartial serializedDi = {};
    serializedDi.entryCount = 1;
    serializedDi.vertCount = 1;
    serializedDi.normalCount = 1;
    serializedDi.blendVertCount = 1;
    serializedDi.pointCount = 1;
    serializedDi.nextFreeIndex = 12;

    zVec3 verts[1] = {{1.0f, 2.0f, 3.0f}};
    zVec3 normals[1] = {{4.0f, 5.0f, 6.0f}};
    zVec3 blendVerts[1] = {{7.0f, 8.0f, 9.0f}};
    zModel_PointEntryPartial pointEntry = {};
    pointEntry.pointCamCount = 1;
    pointEntry.colorB = 255.0f;
    pointEntry.colorG = 0.0f;
    pointEntry.colorR = 0.0f;
    pointEntry.packedColor16 = static_cast<std::int32_t>(0xabcd0000u);
    zVec3 pointCam[1] = {{10.0f, 11.0f, 12.0f}};

    zDiEntryPartial diEntry = {};
    diEntry.flagsAndIndexCount = 0x0203;
    diEntry.material = reinterpret_cast<zModel_MaterialPartial *>(static_cast<std::intptr_t>(0));
    std::int32_t vertexIndices[3] = {0, 1, 2};
    std::int32_t normalIndices[3] = {2, 1, 0};
    zModel_Uv uvs[3] = {
        {0.0f, 0.25f},
        {0.5f, 0.75f},
        {1.0f, 1.25f},
    };

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    std::int32_t header[3] = {1, 1, -1};
    if (std::fwrite(header, sizeof(header), 1, file) != 1 ||
        std::fwrite(&serializedDi, sizeof(serializedDi), 1, file) != 1 ||
        std::fwrite(verts, sizeof(verts), 1, file) != 1 ||
        std::fwrite(normals, sizeof(normals), 1, file) != 1 ||
        std::fwrite(blendVerts, sizeof(blendVerts), 1, file) != 1 ||
        std::fwrite(&pointEntry, sizeof(pointEntry), 1, file) != 1 ||
        std::fwrite(pointCam, sizeof(pointCam), 1, file) != 1 ||
        std::fwrite(&diEntry, sizeof(diEntry), 1, file) != 1 ||
        std::fwrite(vertexIndices, sizeof(vertexIndices), 1, file) != 1 ||
        std::fwrite(normalIndices, sizeof(normalIndices), 1, file) != 1 ||
        std::fwrite(uvs, sizeof(uvs), 1, file) != 1) {
        std::fclose(file);
        return 2;
    }

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    std::rewind(file);
    if (zModel_DiPool::ReadFromStream(file) != 1) {
        std::fclose(file);
        std::free(g_zModel_DiPoolBase);
        g_zModel_DiPoolBase = nullptr;
        g_zModel_DiPoolCapacity = 0;
        g_zModel_MatlPool = nullptr;
        g_zModel_MatlPoolCapacity = 0;
        return 3;
    }
    std::fclose(file);

    zDiPartial *const loadedDi = g_zModel_DiPoolBase;
    zDiEntryPartial *const loadedEntry = loadedDi->entries;
    zModel_PointEntryPartial *const loadedPoint = loadedDi->pointEntries;
    const bool ok =
        g_zModel_DiPoolCapacity == 1 && g_zModel_DiPoolInUseCount == 1 &&
        g_zModel_DiPoolFreeHeadIndex == -1 && loadedDi->verts != nullptr &&
        loadedDi->verts[0].z == 3.0f && loadedDi->normals != nullptr &&
        loadedDi->normals[0].x == 4.0f && loadedDi->blendVerts != nullptr &&
        loadedDi->blendVerts[0].y == 8.0f && loadedPoint != nullptr &&
        loadedPoint[0].pointCamList != nullptr && loadedPoint[0].pointCamList[0].z == 12.0f &&
        loadedPoint[0].packedColor16 == static_cast<std::int32_t>(0xabcdf800u) &&
        loadedEntry != nullptr && loadedEntry[0].material == &materialSlots[0].material &&
        static_cast<std::int32_t *>(loadedEntry[0].vertexIndices)[2] == 2 &&
        static_cast<std::int32_t *>(loadedEntry[0].normalIndices)[0] == 2 &&
        static_cast<zModel_Uv *>(loadedEntry[0].uvPairs)[2].v == 1.25f;

    std::free(loadedEntry[0].uvPairs);
    std::free(loadedEntry[0].normalIndices);
    std::free(loadedEntry[0].vertexIndices);
    std::free(loadedDi->entries);
    std::free(loadedPoint[0].pointCamList);
    std::free(loadedDi->pointEntries);
    std::free(loadedDi->blendVerts);
    std::free(loadedDi->normals);
    std::free(loadedDi->verts);
    std::free(g_zModel_DiPoolBase);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;

    return ok ? 0 : 4;
}

extern "C" int zmodel_dipool_read_entry_by_index_from_stream_smoke() {
    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    const std::int32_t dynamicOffset = 12 + 2 * static_cast<std::int32_t>(sizeof(zDiPartial));
    std::int32_t header[3] = {2, 1, 0};
    zDiPartial serializedSlots[2] = {};
    serializedSlots[0].mode = 11;
    serializedSlots[0].nextFreeIndex = dynamicOffset;
    serializedSlots[1].mode = 22;
    serializedSlots[1].flags = 0x44;
    serializedSlots[1].bboxRadius = 3.5f;
    serializedSlots[1].nextFreeIndex = dynamicOffset;

    if (std::fwrite(header, sizeof(header), 1, file) != 1 ||
        std::fwrite(serializedSlots, sizeof(serializedSlots), 1, file) != 1) {
        std::fclose(file);
        return 2;
    }

    zDiPartial livePool[2] = {};
    livePool[1].nextFreeIndex = 0;
    g_zModel_DiPoolBase = livePool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = 1;

    std::rewind(file);
    zDiPartial *const loaded = zModel_DiPool::ReadEntryByIndexFromStream(file, 1);
    const bool loadedOk = loaded == &livePool[1] && livePool[1].mode == 22 &&
                          livePool[1].flags == 0x44 && livePool[1].bboxRadius == 3.5f &&
                          livePool[1].nextFreeIndex == 0 && g_zModel_DiPoolInUseCount == 1 &&
                          g_zModel_DiPoolFreeHeadIndex == 0;

    std::rewind(file);
    zDiPartial *const outOfRange = zModel_DiPool::ReadEntryByIndexFromStream(file, 2);
    const bool rangeOk = outOfRange == nullptr && g_zModel_DiPoolInUseCount == 1 &&
                         g_zModel_DiPoolFreeHeadIndex == 0;

    std::fclose(file);
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    return loadedOk && rangeOk ? 0 : 3;
}

extern "C" int zmodel_material_and_di_clone_smoke() {
    zModel_MaterialPartial material = {};
    if (zModel_Material::HasAuxData(&material) != 0) {
        return 1;
    }
    material.flags = 0x0200;
    if (zModel_Material::HasAuxData(&material) != 1) {
        return 2;
    }
    material.flags = 0;
    zModel_MaterialCyclePartial cycle = {};
    material.cycle = &cycle;
    if (zModel_Material::HasAuxData(&material) != 1) {
        return 3;
    }

    zModel_MaterialSlot slots[2] = {};
    slots[0].prevPoolIndex = -1;
    slots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = slots;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlPoolInUseCount = 0;

    material.flags = 0x0100;
    material.cycle = reinterpret_cast<zModel_MaterialCyclePartial *>(0x1234);
    zModel_MaterialPartial *clone = zModel_Material::Clone(&material);
    if (clone != &slots[0].material || clone->flags != 0x0100 || clone->cycle != nullptr ||
        g_zModel_MatlFreeHeadIndex != -1 || g_zModel_MatlActiveHeadIndex != 0 ||
        g_zModel_MatlPoolInUseCount != 1) {
        return 4;
    }

    zImage_TexDirEntryPartial *frameTable[2] = {};
    zModel_MaterialCyclePartial sourceCycle = {};
    sourceCycle.frameCount = 2;
    sourceCycle.frameTable = frameTable;
    material.flags = 0x0400;
    material.cycle = &sourceCycle;
    slots[0].prevPoolIndex = -1;
    slots[0].nextPoolIndex = -1;
    slots[1].prevPoolIndex = -1;
    slots[1].nextPoolIndex = -1;
    g_zModel_MatlFreeHeadIndex = 1;
    g_zModel_MatlActiveHeadIndex = 0;
    clone = zModel_Material::Clone(&material);
    if (clone != &slots[1].material || clone->cycle == &sourceCycle || clone->cycle == nullptr ||
        clone->cycle->frameTable == frameTable || clone->cycle->frameCount != 2 ||
        slots[1].nextPoolIndex != 0 || slots[0].prevPoolIndex != 1 ||
        g_zModel_MatlActiveHeadIndex != 1) {
        return 5;
    }
    std::free(clone->cycle->frameTable);
    std::free(clone->cycle);
    clone->cycle = nullptr;

    g_zModel_DefaultMaterial.flags = 0x55;
    g_zModel_MatlFreeHeadIndex = -1;
    if (zModel_Material::Clone(&material) != &g_zModel_DefaultMaterial) {
        return 6;
    }

    zDiPartial diPool[2] = {};
    diPool[1].mode = 99;
    diPool[1].flags = 0xffffffff;
    diPool[1].nextFreeIndex = 0;
    g_zModel_DiPoolBase = diPool;
    g_zModel_DiPoolFreeHeadIndex = 1;
    g_zModel_DiPoolInUseCount = 0;
    zDiPartial *const allocated = zModel_DiPool::AllocFromFreeList();
    if (allocated != &diPool[1] || g_zModel_DiPoolFreeHeadIndex != 0 ||
        g_zModel_DiPoolInUseCount != 1 || allocated->mode != 0 || allocated->flags != 3 ||
        allocated->nextFreeIndex != 0) {
        return 7;
    }

    zDi::SetClonedFlag(allocated, 0);
    if ((allocated->flags & 0x02) != 0) {
        return 8;
    }
    zDi::SetClonedFlag(allocated, 1);
    if ((allocated->flags & 0x02) == 0) {
        return 9;
    }

    zDiEntryPartial specialEntry = {};
    zModel_MaterialPartial plainMaterial = {};
    zModel_MaterialPartial auxMaterial = {};
    auxMaterial.flags = 0x0200;
    zDiPartial specialDi = {};
    specialDi.flags = 0x08;
    if (zDi::HasSpecialFlagsOrAuxMaterialData(nullptr) != 0 ||
        zDi::HasSpecialFlagsOrAuxMaterialData(&specialDi) != 1) {
        return 10;
    }
    specialDi.flags = 0;
    specialDi.entryCount = 1;
    specialDi.entries = &specialEntry;
    specialEntry.material = &plainMaterial;
    if (zDi::HasSpecialFlagsOrAuxMaterialData(&specialDi) != 0) {
        return 11;
    }
    specialEntry.material = &auxMaterial;
    if (zDi::HasSpecialFlagsOrAuxMaterialData(&specialDi) != 1) {
        return 12;
    }

    zDiPartial source = {};
    source.mode = 0x1234;
    source.flags = 0x3f;
    source.blendScale = 1.5f;
    source.textureWorldPerMeter = 2.5f;
    source.textureWorldAxis = 3;
    source.field2c = 4;

    zVec3 pointCamList[1] = {{1.0f, 2.0f, 3.0f}};
    zModel_PointEntryPartial pointEntries[1] = {};
    pointEntries[0].pointCamCount = 1;
    pointEntries[0].pointCamList = pointCamList;
    source.pointCount = 1;
    source.pointEntries = pointEntries;

    zVec3 verts[1] = {{4.0f, 5.0f, 6.0f}};
    zVec3 normals[1] = {{7.0f, 8.0f, 9.0f}};
    zVec3 blendVerts[1] = {{10.0f, 11.0f, 12.0f}};
    source.vertCount = 1;
    source.verts = verts;
    source.normalCount = 1;
    source.normals = normals;
    source.blendVertCount = 1;
    source.blendVerts = blendVerts;

    std::uint32_t vertexIndices[2] = {11, 22};
    std::uint32_t normalIndices[2] = {33, 44};
    std::uint32_t uvPairs[4] = {55, 66, 77, 88};
    zDiEntryPartial entries[2] = {};
    zModel_MaterialPartial uvMaterial = {};
    uvMaterial.flags = 0x0100;
    entries[0].flagsAndIndexCount = 0x0200 | 2;
    entries[0].unknown_04 = 0x77777777;
    entries[0].vertexIndices = vertexIndices;
    entries[0].normalIndices = normalIndices;
    entries[0].uvPairs = uvPairs;
    entries[0].material = &uvMaterial;
    entries[0].variantTagInitialized = 1;
    entries[0].variantTag = 0x42;
    entries[1].material = &uvMaterial;
    source.entryCount = 2;
    source.entries = entries;

    zDiPartial clonePool[1] = {};
    clonePool[0].nextFreeIndex = -1;
    g_zModel_DiPoolBase = clonePool;
    g_zModel_DiPoolFreeHeadIndex = 0;
    g_zModel_DiPoolInUseCount = 0;
    zModel_MaterialSlot cloneMaterialSlots[1] = {};
    cloneMaterialSlots[0].prevPoolIndex = -1;
    cloneMaterialSlots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = cloneMaterialSlots;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlPoolInUseCount = 0;

    zDiPartial *const diClone = zDi::CloneToInstance(&source, 1, 0);
    if (diClone != &clonePool[0] || diClone->mode != source.mode ||
        (diClone->flags & 0x3f) != (source.flags & 0x3f) ||
        diClone->pointEntries == source.pointEntries ||
        diClone->pointEntries[0].pointCamList == pointCamList || diClone->verts == verts ||
        diClone->normals == normals || diClone->blendVerts == blendVerts ||
        diClone->entries == entries ||
        diClone->entries[0].material != &cloneMaterialSlots[0].material ||
        diClone->entries[1].material != &cloneMaterialSlots[0].material ||
        diClone->entries[0].vertexIndices == vertexIndices ||
        diClone->entries[0].normalIndices == normalIndices ||
        diClone->entries[0].uvPairs == uvPairs) {
        zDi::FreeContents(diClone);
        return 13;
    }

    if (diClone->entries[0].flagsAndIndexCount != entries[0].flagsAndIndexCount ||
        diClone->entries[0].unknown_04 != entries[0].unknown_04 ||
        diClone->entries[0].variantTag != 0x42 ||
        static_cast<std::uint32_t *>(diClone->entries[0].vertexIndices)[1] != 22 ||
        static_cast<std::uint32_t *>(diClone->entries[0].normalIndices)[1] != 44 ||
        static_cast<std::uint32_t *>(diClone->entries[0].uvPairs)[3] != 88) {
        zDi::FreeContents(diClone);
        return 14;
    }

    zDi::FreeContents(diClone);
    g_zModel_MatlPool = nullptr;
    g_zModel_DiPoolBase = nullptr;
    return 0;
}

extern "C" int zmodel_scrolling_texture_update_smoke(void) {
    zModel_TextureScrollInfoPartial texture{};
    texture.wrapShiftU = 0;
    texture.wrapShiftV = 1;
    zModel_Uv uvs[2] = {{1.0f, 2.0f}, {3.0f, 4.0f}};
    float scrollRates[2] = {2.0f, 4.0f};
    g_FrameDeltaTimeSec = 0.5f;
    g_zVideo_ActiveRendererPath = 0;

    zModel_Instance_UpdateScrollingTextures(&texture, uvs, scrollRates, 2);
    if (uvs[0].u != 2.0f || uvs[0].v != 4.0f || uvs[1].u != 4.0f || uvs[1].v != 6.0f) {
        return 1;
    }

    g_zVideo_ActiveRendererPath = 1;
    uvs[0] = {127.5f, 0.0f};
    uvs[1] = {127.75f, 0.0f};
    scrollRates[0] = 2.0f;
    scrollRates[1] = 0.0f;
    zModel_Instance_UpdateScrollingTextures(&texture, uvs, scrollRates, 2);
    if (uvs[0].u != -127.5f || uvs[1].u != -127.25f || uvs[0].v != 0.0f || uvs[1].v != 0.0f) {
        return 2;
    }

    zModel_TextureRefPartial textureRef{&texture};
    zModel_MaterialTextureBindingPartial material{};
    material.flags = 1;
    material.textureRef = &textureRef;
    zModel_InstanceSurfaceEntryPartial entry{};
    entry.vertexCountAndFlags = 2;
    entry.uvs = uvs;
    entry.materialBinding = &material;
    zModel_InstancePartial instance{};
    instance.surfaceEntryCount = 1;
    instance.scrollRateU = 0.0f;
    instance.scrollRateV = 1.0f;
    instance.scrollingTextureFrameTick = 11;
    instance.surfaceEntries = &entry;
    g_FrameDeltaTimeSec = 1.0f;
    g_zVideo_ActiveRendererPath = 0;
    g_zVideo_FrameTick = 12;
    uvs[0] = {0.0f, 1.0f};
    uvs[1] = {0.0f, 2.0f};

    if (zModel_Instance_UpdateScrollingTexturesIfNeeded(&instance) != 0 ||
        instance.scrollingTextureFrameTick != 12 || uvs[0].v != 2.0f || uvs[1].v != 3.0f) {
        return 3;
    }

    zModel_Instance_UpdateScrollingTexturesIfNeeded(&instance);
    if (uvs[0].v != 2.0f || uvs[1].v != 3.0f) {
        return 4;
    }

    return zModel_Instance_UpdateScrollingTexturesIfNeeded(nullptr) == -1 ? 0 : 5;
}

extern "C" int zmodel_render_point_queue_entry_smoke(void) {
    static std::int32_t matrixFlags[1] = {1};
    static float *matrixSlots[1] = {};
    zMat4x3 matrix{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    g_zMath_ProjScaleX = 1.0f;
    g_zMath_ProjScaleY = 1.0f;
    g_zMath_ProjOffsetX = 0.0f;
    g_zMath_ProjOffsetY = 0.0f;
    g_zMath_ProjSphereRadiusScale = 10.0f;
    gClipRect_Primary.zMin = 1.0f;
    g_zVideo_ProjectClipLeft = 0.0f;
    g_zVideo_ProjectClipTop = 0.0f;
    g_zVideo_ProjectClipRight = 2.0f;
    g_zVideo_ProjectClipBottom = 3.0f;
    zRndr::g_inverseDepthBias = 0.0f;
    zRndr::g_inverseDepthScale = 1.0f;
    zRndr::g_lensFlareSampleQueueCount = 0;
    g_drawPointCallCount = 0;

    zModel_PointEntryPartial entry{};
    zVec3 point{10.0f, 20.0f, 10.0f};
    g_zVideo_ActiveRendererPath = 0;
    zModel_RenderPointQueueEntry(&point, 0x1234beef, &entry);
    if (zRndr::g_lensFlareSampleQueueCount != 1 ||
        zRndr::g_lensFlareSampleQueue[0].packedColor16 != 0xbeef ||
        zRndr::g_lensFlareSampleQueue[0].x < 0.999f ||
        zRndr::g_lensFlareSampleQueue[0].x > 1.001f ||
        zRndr::g_lensFlareSampleQueue[0].y < 1.999f ||
        zRndr::g_lensFlareSampleQueue[0].y > 2.001f ||
        zRndr::g_lensFlareSampleQueue[0].reciprocalZ < 0.099f ||
        zRndr::g_lensFlareSampleQueue[0].reciprocalZ > 0.101f) {
        return 1;
    }

    point.z = 1.0f;
    zModel_RenderPointQueueEntry(&point, 0xbeef, &entry);
    if (zRndr::g_lensFlareSampleQueueCount != 1) {
        return 2;
    }

    point = {10.0f, 20.0f, 10.0f};
    entry.depthBiasWord = 2;
    g_zRndr_InverseZTolerance = 0.5f;
    zRndr::g_lensFlareSampleQueueCount = 0;
    g_zVideo_ActiveRendererPath = 1;
    g_zVideo_pfnDrawPointColor16 = static_cast<std::uint32_t>(
        reinterpret_cast<std::uintptr_t>(&zmodel_draw_point_color16_stub));
    zModel_RenderPointQueueEntry(&point, 0x1ffff, &entry);
    if (g_drawPointCallCount != 1 || g_drawPointLastColor != 0xffff || g_drawPointLastCount != 1 ||
        g_drawPointLastPoint.reciprocalZ < 1.999f || g_drawPointLastPoint.reciprocalZ > 2.001f ||
        zRndr::g_lensFlareSampleQueueCount != 1) {
        return 3;
    }

    g_zVideo_pfnDrawPointColor16 = 0;
    g_zVideo_ActiveRendererPath = 0;
    return 0;
}

extern "C" int zmodel_light_fog_fade_smoke() {
    zClass_LightDataPartial light{};
    light.range1 = 5.0f;
    light.range2 = 15.0f;
    light.invRangeDelta = 0.1f;
    if (zModel_Light::EvalDistanceWeight(&light, 15.0f) != 0.0f ||
        zModel_Light::EvalDistanceWeight(&light, 16.0f) != 0.0f ||
        zModel_Light::EvalDistanceWeight(&light, 5.0f) != 1.0f ||
        zModel_Light::EvalDistanceWeight(&light, 4.0f) != 1.0f) {
        return 5;
    }

    const float distanceFade = zModel_Light::EvalDistanceWeight(&light, 10.0f);
    if (distanceFade < 0.499f || distanceFade > 0.501f) {
        return 6;
    }

    zMath::g_zMath_CameraScratchA = {};
    zMath::g_zMath_CameraScratchA.yy = 1.0f;

    gModel_FogDistanceStart = 10.0f;
    gModel_FogDistanceEnd = 20.0f;
    gModel_FogDistanceInvRange = 0.1f;
    gModel_FogHeightHigh = 10.0f;
    gModel_FogHeightLow = 0.0f;
    gModel_FogHeightInvRange = 0.1f;

    zVec3 point{0.0f, 5.0f, 0.0f};
    if (zModel_Light::EvalSphereFogFade(&point, 1.0f) != 0.0f) {
        return 1;
    }

    gModel_FogDistanceStart = -2.0f;
    gModel_FogDistanceEnd = -1.0f;
    gModel_FogDistanceInvRange = 1.0f;
    point.y = -5.0f;
    if (zModel_Light::EvalSphereFogFade(&point, 0.5f) != 1.0f) {
        return 2;
    }

    gModel_FogDistanceStart = -1.0f;
    gModel_FogDistanceEnd = 1.0f;
    gModel_FogDistanceInvRange = 0.5f;
    point.y = 5.0f;
    const float partial = zModel_Light::EvalSphereFogFade(&point, 0.0f);
    if (partial < 0.249f || partial > 0.251f) {
        return 3;
    }

    point.y = 20.0f;
    if (zModel_Light::EvalSphereFogFade(&point, 1.0f) != 0.0f) {
        return 4;
    }

    std::memset(gModel_ActiveLights, 0, sizeof(gModel_ActiveLights));
    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    zModel_LightStatePartial lightStates[3] = {};
    zClass_LightDataPartial lights[3] = {};
    for (std::int32_t i = 0; i < 3; ++i) {
        lightStates[i].flags = 4;
        lights[i].enabled = 1;
        lights[i].lightSubMode = 1;
        lights[i].range1 = 5.0f;
        lights[i].range2 = 15.0f;
        lights[i].invRangeDelta = 0.1f;
        lights[i].falloff = 1.0f;
        lights[i].intensityScale = 1.0f;
        gModel_ActiveLights[i].light = &lights[i];
        gModel_ActiveLights[i].lightState = &lightStates[i];
    }

    lights[0].falloff = 0.25f;
    lights[0].intensityScale = 0.5f;
    lights[0].viewPos.z = 10.0f;
    lights[1].viewPos.x = 10.0f;
    lights[1].viewPos.z = 10.0f;
    lights[2].isPointMode = 1;
    gModel_ActiveLightCount = 3;
    gModel_ActiveLightSpecialIndex = -1;
    g_zVideo_ActiveRendererPath = 0;
    g_zModel_SoftwarePathActive = 0;
    zVec3 sphereCenter{0.0f, 0.0f, 10.0f};

    if (zModel_Light::PointInPolygonTestRadiusXZ(&sphereCenter, 0.0f) != 3 ||
        gModel_ActiveLights[0].useFullWeight != 1 ||
        gModel_ActiveLights[0].contributesToLighting != 1 || g_Clip_PolyAttr0[0] != 0.75f ||
        gModel_ActiveLights[1].contributesToLighting != 1 || g_Clip_PolyAttr0[1] < 0.45f ||
        g_Clip_PolyAttr0[1] > 0.56f || gModel_ActiveLightSpecialIndex != 2 ||
        g_Clip_PolyAttr1[2] < 0.45f || g_Clip_PolyAttr1[2] > 0.56f) {
        return 7;
    }

    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    g_zModel_SoftwarePathActive = 1;
    if (!(zModel_Light::PointInPolygonTestRadiusXZ(&sphereCenter, 0.0f) == 1 &&
          gModel_ActiveLights[0].contributesToLighting == 0 &&
          gModel_ActiveLights[1].contributesToLighting == 0 &&
          gModel_ActiveLights[2].contributesToLighting == 1 && g_Clip_PolyAttr1[2] > 0.45f &&
          g_Clip_PolyAttr1[2] < 0.56f)) {
        return 8;
    }

    static std::int32_t matrixFlags[1] = {0};
    static float *matrixSlots[1] = {};
    zMat4x3 matrix{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 10.0f};
    matrixSlots[0] = reinterpret_cast<float *>(&matrix);
    zMath::g_currentMatrixIdentityFlagSlot = &matrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &matrixSlots[0];

    std::memset(gModel_ActiveLights, 0, sizeof(gModel_ActiveLights));
    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    std::memset(g_Clip_PolyAttr1, 0, sizeof(g_Clip_PolyAttr1));
    gModel_ActiveLightCount = 2;
    gModel_HasActiveLights = 1;
    g_zModel_SoftwarePathActive = 0;
    g_zModel_FogTargetColorOverride = {};
    static std::int32_t graphicsFlags = 1;
    g_zModel_GraphicsFlagsOption = &graphicsFlags;
    zRndr::g_fogParamsActive = {};
    zRndr::g_fogParamsActive.colorRgb01[0] = -1.0f;
    zRndr::g_fogParamsActive.colorRgb01[1] = -1.0f;
    zRndr::g_fogParamsActive.colorRgb01[2] = -1.0f;

    zModel_LightStatePartial evalLightStates[2] = {};
    zClass_LightDataPartial evalLights[2] = {};
    for (std::int32_t i = 0; i < 2; ++i) {
        evalLightStates[i].flags = 4;
        evalLights[i].enabled = 1;
        evalLights[i].lightSubMode = 1;
        evalLights[i].range1 = 50.0f;
        evalLights[i].range2 = 100.0f;
        evalLights[i].invRangeDelta = 0.02f;
        evalLights[i].falloff = 1.0f;
        evalLights[i].intensityScale = 1.0f;
        evalLights[i].viewPos.z = 10.0f;
        gModel_ActiveLights[i].light = &evalLights[i];
        gModel_ActiveLights[i].lightState = &evalLightStates[i];
    }
    evalLights[0].specularColor.red = 1.0f;
    evalLights[1].specularColor.blue = 1.0f;

    zDiPartial di{};
    di.flags = 3;
    di.bboxCenter = {0.0f, 5.0f, 0.0f};
    di.bboxRadius = 1.0f;
    gModel_FogEnabled = 1;
    gModel_FogDistanceStart = 10.0f;
    gModel_FogDistanceEnd = 20.0f;
    gModel_FogDistanceInvRange = 0.1f;
    gModel_FogHeightHigh = 20.0f;
    gModel_FogHeightLow = 0.0f;
    gModel_FogHeightInvRange = 0.05f;

    std::int32_t depthFade = -1;
    std::int32_t activeLightState = -1;
    std::int32_t lensFlareVisible = -1;
    zDi::EvalBoundingSphereLightingFlags(&di, &depthFade, &activeLightState, &lensFlareVisible);
    if (depthFade != 1 || activeLightState != 1 || lensFlareVisible != 1 ||
        zRndr::g_fogParamsActive.colorRgb01[0] < 0.49f ||
        zRndr::g_fogParamsActive.colorRgb01[0] > 0.51f ||
        zRndr::g_fogParamsActive.colorRgb01[1] != 0.0f ||
        zRndr::g_fogParamsActive.colorRgb01[2] < 0.49f ||
        zRndr::g_fogParamsActive.colorRgb01[2] > 0.51f) {
        return 9;
    }

    g_zModel_FogTargetColorOverride.colorRgb01.green = 0.75f;
    g_zModel_FogTargetColorOverride.weight = 0.5f;
    gModel_HasActiveLights = 0;
    activeLightState = -1;
    lensFlareVisible = -1;
    zDi::EvalBoundingSphereLightingFlags(&di, &depthFade, &activeLightState, &lensFlareVisible);
    const bool overrideOnlyOk = activeLightState == 1 && lensFlareVisible == 0;

    gModel_HasActiveLights = 0;
    gModel_ActiveLightCount = 0;
    g_zModel_SoftwarePathActive = 0;
    g_zModel_FogTargetColorOverride = {};
    gModel_DefaultGraphicsFlags = 0;
    g_zModel_GraphicsFlagsOption = &gModel_DefaultGraphicsFlags;

    return overrideOnlyOk ? 0 : 10;
}

extern "C" int zmodel_light_build_light_weights_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_pixelPackRedMask = 0xf800;
    zRndr::g_pixelPackGreenMask = 0x07e0;
    zRndr::g_pixelPackBlueMask = 0x001f;
    zRndr::g_fogParamsActive = {};
    zRndr::g_fogTargetParamsStaged = {};
    zRndr::g_fogTargetParamsDirect = {};

    zClass_LightDataPartial light{};
    light.enabled = 1;
    light.viewDir = {0.0f, 0.0f, 1.0f};
    light.viewPos = {0.0f, 0.0f, 10.0f};
    light.falloff = 0.0f;
    light.intensityScale = 0.5f;
    light.specularColor = {1.0f, 0.0f, 0.0f};
    light.range1 = 0.0f;
    light.range2 = 100.0f;
    light.range2Sq = 10000.0f;
    light.invRangeDelta = 0.01f;

    zModel_LightStatePartial lightState{};
    lightState.flags = 4;
    gModel_ActiveLightCount = 1;
    gModel_ActiveLights[0] = {};
    gModel_ActiveLights[0].light = &light;
    gModel_ActiveLights[0].lightState = &lightState;
    gModel_ActiveLights[0].useFullWeight = 1;
    gModel_ActiveLights[0].contributesToLighting = 1;
    g_zModel_FogTargetColorOverride = {};

    g_Clip_PolyVertsScratch[0] = {0.0f, 0.0f, 5.0f};
    g_Clip_PolyVertsScratch[1] = {1.0f, 0.0f, 5.0f};
    g_Clip_PolyVertsScratch[2] = {0.0f, 1.0f, 5.0f};
    zVec3 normal{0.0f, 0.0f, 2.0f};
    std::int32_t packedColor = 0;
    if (zModel_Light_BuildLightWeights(&normal, 3, &packedColor, 0.0f) != 1) {
        return 1;
    }

    if (normal.z < 0.999f || normal.z > 1.001f || zRndr::g_fogParamsActive.colorRgb01[0] != 1.0f ||
        zRndr::g_fogParamsActive.colorRgb01[1] != 0.0f ||
        zRndr::g_fogParamsActive.colorRgb01[2] != 0.0f || packedColor == 0) {
        return 2;
    }

    gModel_ActiveLightCount = 0;
    gModel_ActiveLights[0] = {};
    g_zModel_FogTargetColorOverride = {};
    packedColor = 0x1234;
    return zModel_Light_BuildLightWeights(&normal, 3, &packedColor, 0.0f) == 0 &&
                   packedColor == 0x1234
               ? 0
               : 3;
}

extern "C" int zmodel_light_point_in_polygon_init_smoke(void) {
    zClass_LightDataPartial light0{};
    zClass_LightDataPartial light1{};
    light0.specularColor = {0.2f, 0.3f, 0.4f};
    light0.intensityScale = 0.25f;
    light1.specularColor = {0.7f, 0.6f, 0.5f};
    light1.intensityScale = 0.125f;
    light1.isPointMode = 1;
    zClass_LightDataPartial *lights[2] = {&light0, &light1};

    zModel_LightStatePartial state0{};
    zModel_LightStatePartial state1{};
    state1.flags = 4;
    zModel_LightStatePartial *states[2] = {&state0, &state1};

    g_zVideo_ActiveRendererPath = 0;
    gModel_FogColorRgb01 = {0.1f, 0.2f, 0.3f};
    gModel_SpecialLightPaletteRemapRecipe = {};
    zModel_Light_PointInPolygonInitXZ(lights, states, 2);
    if (gModel_LightInputDataList != lights || gModel_LightInputNodeStates != states ||
        gModel_LightInputCount != 2 || gModel_ActiveLightCount != 1 ||
        gModel_HasActiveLights != 1 || gModel_ActiveLightSpecialIndex != 0 ||
        gModel_ActiveLights[0].light != &light1 || gModel_AmbientColorRgb01.red != 0.7f ||
        gModel_AmbientIntensityFactor != 0.875f ||
        gModel_SpecialLightPaletteRemapRecipe.color1Strength != 1.0f ||
        gModel_SpecialLightPaletteRemapRecipe.color0Strength != 0.0f) {
        return 1;
    }

    state1.flags = 0;
    zModel_Light_PointInPolygonInitXZ(lights, states, 2);
    return gModel_ActiveLightCount == 0 && gModel_HasActiveLights == 0 &&
                   gModel_AmbientColorRgb01.red == gModel_FogColorRgb01.red &&
                   gModel_SpecialLightPaletteRemapRecipe.color1R == 0.0f
               ? 0
               : 2;
}

extern "C" int zmodel_light_build_attr0_depth_fade_smoke(void) {
    zMath::g_zMath_CameraScratchA = {};
    zMath::g_zMath_CameraScratchA.yy = 1.0f;

    gModel_FogDistanceStart = 10.0f;
    gModel_FogDistanceEnd = 20.0f;
    gModel_FogDistanceInvRange = 0.1f;
    gModel_FogHeightHigh = 10.0f;
    gModel_FogHeightLow = 0.0f;
    gModel_FogHeightInvRange = 0.1f;

    std::memset(g_Clip_PolyAttr0, 0, sizeof(g_Clip_PolyAttr0));
    g_Clip_PolyAttr0[2] = 99.0f;
    g_Clip_PolyVertsScratch[0] = {16.0f, 0.0f, 0.0f};
    g_Clip_PolyVertsScratch[1] = {32.0f, 5.0f, 0.0f};
    g_Clip_PolyVertsScratch[2] = {8.0f, 0.0f, 0.0f};

    std::int32_t hasVariation = 0;
    if (zModel_Light::BuildAttr0DepthFade(3, &hasVariation) != 1 || hasVariation != 1 ||
        g_Clip_PolyAttr0[0] < 152.9f || g_Clip_PolyAttr0[0] > 153.1f ||
        g_Clip_PolyAttr0[1] < 127.4f || g_Clip_PolyAttr0[1] > 127.6f ||
        g_Clip_PolyAttr0[2] != 99.0f) {
        return 1;
    }

    g_Clip_PolyVertsScratch[0] = {4.0f, 0.0f, 0.0f};
    hasVariation = 77;
    if (zModel_Light::BuildAttr0DepthFade(1, &hasVariation) != 0 || hasVariation != 77) {
        return 2;
    }

    g_Clip_PolyVertsScratch[0] = {16.0f, 20.0f, 0.0f};
    hasVariation = 77;
    if (zModel_Light::BuildAttr0DepthFade(1, &hasVariation) != 0 || hasVariation != 0) {
        return 3;
    }

    float fade = -1.0f;
    g_Clip_PolyVertsScratch[0] = {4.0f, 0.0f, 0.0f};
    if (zModel_Light::EvalBatchSphereFade(&fade) != 0 || fade != -1.0f) {
        return 4;
    }

    g_Clip_PolyVertsScratch[0] = {16.0f, 5.0f, 0.0f};
    if (zModel_Light::EvalBatchSphereFade(&fade) != 1 || fade < 0.299f || fade > 0.301f) {
        return 5;
    }

    g_Clip_PolyVertsScratch[0] = {16.0f, 20.0f, 0.0f};
    fade = -1.0f;
    if (zModel_Light::EvalBatchSphereFade(&fade) != 0 || fade != 0.0f) {
        return 6;
    }

    return 0;
}

extern "C" int zclass_world_apply_pending_fog_settings_smoke(void) {
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    g_zVideo_ActiveRendererPath = 1;
    zRndr::g_fogColorParams = {};
    gModel_FogDistanceInvRange = 0.0f;
    gModel_FogHeightInvRange = 0.0f;

    zClass_WorldDataPartial worldData{};
    zClass_NodePartial worldNode{};
    worldNode.classData = &worldData;
    if (zClass_World::SetPendingFogState(&worldNode, 1) != 0 ||
        zClass_World::SetPendingFogColorRgb01(&worldNode, 1.2f, -0.5f, 0.5f) != 0 ||
        zClass_World::SetPendingFogRange(&worldNode, 5.0f, 20.0f) != 0 ||
        zClass_World::SetPendingFogAltitudeRange(&worldNode, 2.0f, 10.0f) != 0 ||
        zClass_World::SetPendingFogDensity(&worldNode, 0.75f) != 0 || worldData.flags != 0x2f) {
        return 1;
    }

    std::int32_t childClassData = 0;
    zClass_NodePartial areaChild{};
    areaChild.classData = &childClassData;
    areaChild.flags = 0x100;
    areaChild.cachedBounds[1] = -4.0f;
    areaChild.cachedBounds[4] = 8.0f;
    zClass_NodePartial *areaChildren[] = {&areaChild};
    zWorldAreaPartial pendingArea{};
    pendingArea.areaFlags = 1;
    pendingArea.bbox[0] = 0.0f;
    pendingArea.bbox[2] = 0.0f;
    pendingArea.bbox[3] = 10.0f;
    pendingArea.bbox[5] = 10.0f;
    pendingArea.childCount = 1;
    pendingArea.childList = areaChildren;
    zWorldAreaPartial *pendingAreas[] = {&pendingArea};
    worldData.pendingAreaUpdateCount = 1;
    worldData.pendingAreaUpdates = pendingAreas;

    if (zClass_World::ApplyPendingFogSettings(&worldNode) != 0 || worldData.flags != 0 ||
        worldData.pendingAreaUpdateCount != 0 || (pendingArea.areaFlags & 1) != 0 ||
        (pendingArea.areaFlags & 0x100) == 0) {
        return 1;
    }

    if (gModel_FogEnabled != 1 || gModel_FogLinearModeEnabled != 1 ||
        gModel_FogDistanceStart != 5.0f || gModel_FogDistanceEnd != 20.0f ||
        gModel_FogDistanceInvRange < 0.066f || gModel_FogDistanceInvRange > 0.067f ||
        gModel_FogHeightHigh != 10.0f || gModel_FogHeightLow != 2.0f ||
        gModel_FogHeightInvRange != 0.125f || gModel_FogDensity != 0.75f) {
        return 2;
    }

    if (worldData.ambientColor.red != 1.0f || worldData.ambientColor.green != 0.0f ||
        worldData.ambientColor.blue != 0.5f || gModel_FogColorRgb01.red != 1.0f ||
        gModel_FogColorRgb01.green != 0.0f || gModel_FogColorRgb01.blue != 0.5f ||
        zRndr::g_fogColorParams.colorRgb01[0] != 1.0f ||
        zRndr::g_fogColorParams.colorRgb01[1] != 0.0f ||
        zRndr::g_fogColorParams.colorRgb01[2] != 0.5f) {
        return 3;
    }

    return pendingArea.bbox[1] == -4.0f && pendingArea.bbox[4] == 8.0f &&
                   pendingArea.bboxCenter.y == 2.0f && pendingArea.bboxRadius > 0.0f
               ? 0
               : 4;
}

extern "C" int zclass_shutdown_core_smoke(void) {
    g_zClass_NodeArray =
        static_cast<zClass_NodeFreeListSlot *>(std::calloc(2, sizeof(zClass_NodeFreeListSlot)));
    g_zClass_NodeArraySize = 2;
    g_zClass_ActiveNodeCount = 1;
    g_zClass_NodeFreeHeadIndex = 1;
    g_zClass_IsInitialized = 1;
    std::strcpy(g_zClass_CurrentZbdPath, "mission.zbd");

    if (zClass::Shutdown() != 0) {
        return 1;
    }

    return g_zClass_NodeArray == nullptr && g_zClass_NodeArraySize == 0 &&
                   g_zClass_ActiveNodeCount == 0 && g_zClass_NodeFreeHeadIndex == -1 &&
                   g_zClass_IsInitialized == 0 && g_zClass_CurrentZbdPath[0] == '\0'
               ? 0
               : 2;
}

extern "C" int zclass_gwnode_update_smoke(void) {
    std::int32_t classData = 0;
    zClass_NodeFreeListSlot parentSlot{};
    zClass_NodePartial child{};
    child.classData = &classData;
    child.flags = 0x100;
    child.cachedBounds[0] = -1.0f;
    child.cachedBounds[1] = 2.0f;
    child.cachedBounds[2] = -3.0f;
    child.cachedBounds[3] = 4.0f;
    child.cachedBounds[4] = 5.0f;
    child.cachedBounds[5] = 6.0f;
    zClass_NodePartial *children[] = {&child};
    parentSlot.node.classId = 6;
    parentSlot.node.classData = &classData;
    parentSlot.node.boundsFlags = 0x02;
    parentSlot.node.flags = 0x02;
    parentSlot.node.listCountB = 1;
    parentSlot.node.listB = children;
    if (zClass_Class::gwNodeUpdate(&parentSlot.node) != 0 || (parentSlot.node.flags & 0x02) != 0 ||
        (parentSlot.node.flags & 0x100) == 0 || parentSlot.node.boundsFlags != 0x04 ||
        parentSlot.node.cachedBounds[0] != -1.0f || parentSlot.node.cachedBounds[5] != 6.0f) {
        return 1;
    }

    zClass_Object3DDataPartial objectData{};
    zClass_NodeFreeListSlot objectSlot{};
    objectSlot.node.classId = 5;
    objectSlot.node.classData = &objectData;
    objectSlot.node.flags = 0x202;
    objectData.flags = 0x11;
    zBBox3f *objectBounds = reinterpret_cast<zBBox3f *>(objectSlot.unknown_8c);
    *objectBounds = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f};
    if (zClass_Class::gwNodeUpdate(&objectSlot.node) != 0 || (objectData.flags & 0x01) != 0 ||
        (objectSlot.node.flags & 0x02) != 0 || (objectSlot.node.flags & 0x100) == 0 ||
        objectSlot.node.cachedBounds[0] != 1.0f || objectSlot.node.cachedBounds[5] != 6.0f) {
        return 2;
    }

    zClass_WorldDataPartial worldData{};
    worldData.flags = 0x08;
    worldData.fogDensity = 0.375f;
    zClass_NodePartial worldNode{};
    worldNode.classId = 2;
    worldNode.flags = 0x02;
    worldNode.classData = &worldData;
    if (zClass_Class::gwNodeUpdate(&worldNode) != 0 || worldData.flags != 0 ||
        (worldNode.flags & 0x02) != 0 || gModel_FogDensity != 0.375f) {
        return 3;
    }

    zClass_NodePartial invalidNode{};
    invalidNode.classId = 3;
    return zClass_Class::gwNodeUpdate(&invalidNode) == 3 ? 0 : 4;
}

extern "C" int zgame_options_find_option_smoke() {
    zOptionEntryPartial first{};
    zOptionEntryPartial second{};
    char firstName[] = "First";
    char secondName[] = "GfxFlags_HW";
    first.name = firstName;
    first.next = &second;
    second.name = secondName;
    second.payloadOrBuffer = 0x10000;

    g_zGame_Options_OptionListHead = &first;
    if (zGame::Options_FindOption("GfxFlags_HW") != &second) {
        return 1;
    }
    if (zGame::Options_FindOption("Missing") != nullptr) {
        return 2;
    }

    g_zGame_Options_OptionListHead = nullptr;
    return zGame::Options_FindOption("First") == nullptr ? 0 : 3;
}

extern "C" int zopt_eval_int_compare_op_smoke() {
    if (zOpt::EvalIntCompareOp("==", 7, 7) != 1 || zOpt::EvalIntCompareOp("==", 7, 8) != 0 ||
        zOpt::EvalIntCompareOp("<", -2, 3) != 1 || zOpt::EvalIntCompareOp("<", 3, -2) != 0 ||
        zOpt::EvalIntCompareOp(">", 3, -2) != 1 || zOpt::EvalIntCompareOp(">", -2, 3) != 0 ||
        zOpt::EvalIntCompareOp("<=", 3, 3) != 1 || zOpt::EvalIntCompareOp("<=", 4, 3) != 0 ||
        zOpt::EvalIntCompareOp(">=", 3, 3) != 1 || zOpt::EvalIntCompareOp(">=", 2, 3) != 0 ||
        zOpt::EvalIntCompareOp("!=", 2, 3) != 1 || zOpt::EvalIntCompareOp("!=", 3, 3) != 0) {
        return 1;
    }

    if (zOpt::EvalIntCompareOp("~=", 100, 101) != 1 ||
        zOpt::EvalIntCompareOp("~=", 100, 102) != 0 || zOpt::EvalIntCompareOp("~=", 0, 0) != 0 ||
        zOpt::EvalIntCompareOp("~=", -100, -101) != 0) {
        return 2;
    }

    return zOpt::EvalIntCompareOp("??", 1, 1) == 0 ? 0 : 3;
}

extern "C" int zopt_fullscreen_accessors_smoke() {
    std::int32_t fullscreen = 0;
    std::int32_t hudSw = 1;
    std::int32_t hudHw = 0;
    std::int32_t hudTypeSw = 0;
    std::int32_t hudTypeHw = 0;
    std::int32_t gameControl = 0;
    std::int32_t difficulty = 0;
    std::int32_t effectsSw = 0;
    std::int32_t effectsHw = 0;
    std::int32_t objectLodSw = 0;
    std::int32_t objectLodHw = 0;
    std::int32_t muteSound = 0;
    float soundVolume = 0.0f;
    float globalVolume = 0.0f;
    std::int32_t soundLod = 0;
    std::int32_t textureMemorySw = 0;
    std::int32_t textureMemoryHw = 0;
    std::int32_t gfxFlagsSw = 0;
    std::int32_t gfxFlagsHw = 0;
    char playerNameBuffer[8] = {};
    zOptionEntryPartial playerNameOption = {};
    ZOPT_VIDEO_FULLSCREEN = &fullscreen;
    ZOPT_HUD_SW = &hudSw;
    ZOPT_HUD_HW = &hudHw;
    ZOPT_HUD_TYPE_SW = &hudTypeSw;
    ZOPT_HUD_TYPE_HW = &hudTypeHw;
    ZOPT_GAME_CONTROL_OPTIONS = &gameControl;
    g_zOpt_GameDifficultyOption = &difficulty;
    ZOPT_EFFECTS_LEVEL_SW = &effectsSw;
    ZOPT_EFFECTS_LEVEL_HW = &effectsHw;
    ZOPT_OBJECT_LOD_SW = &objectLodSw;
    ZOPT_OBJECT_LOD_HW = &objectLodHw;
    ZOPT_MUTE_SOUND = &muteSound;
    ZOPT_SOUND_VOLUME = &soundVolume;
    g_zSnd_GlobalVolumeScalePtr = &globalVolume;
    ZOPT_SOUND_LOD = &soundLod;
    ZOPT_TEXTURE_MEMORY_SW = &textureMemorySw;
    ZOPT_TEXTURE_MEMORY_HW = &textureMemoryHw;
    ZOPT_GFX_FLAGS_SW = &gfxFlagsSw;
    ZOPT_GFX_FLAGS_HW = &gfxFlagsHw;
    playerNameOption.payloadOrBuffer = reinterpret_cast<std::int32_t>(playerNameBuffer);
    playerNameOption.dataSize = sizeof(playerNameBuffer);
    ZOPT_PLAYER_NAME = &playerNameOption;

    zOpt::SetFullscreenOption(1);
    if (fullscreen != 1 || zOpt::GetFullscreenOption() != 1) {
        return 1;
    }

    zOpt::SetFullscreenOption(0);
    if (fullscreen != 0 || zOpt::GetFullscreenOption() != 0) {
        return 2;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetHudVisibilityOption(0);
    if (hudSw != 0 || hudHw != 0 || zOpt::GetHudVisibilityOption() != 0) {
        return 3;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetHudVisibilityOption(1);
    if (hudSw != 0 || hudHw != 1 || zOpt::GetHudVisibilityOption() != 1) {
        return 4;
    }

    g_zOpt_HwMode = 0;
    hudTypeSw = 2;
    if (zOpt::GetHudTypeForCurrentHwMode() != 2) {
        return 41;
    }

    g_zOpt_HwMode = 1;
    hudTypeHw = 3;
    if (zOpt::GetHudTypeForCurrentHwMode() != 3) {
        return 42;
    }

    zOpt::SetGameControlOptions(0);
    zOpt::SetThrottleMode(1);
    zOpt::SetSteeringMode(1);
    zOpt::SetCursorMode(1);
    if (gameControl != 7 || zOpt::GetThrottleMode() != 1 || zOpt::GetSteeringMode() != 1 ||
        zOpt::GetCursorMode() != 1 || zOpt::GetCameraModePlayerState() != 3) {
        return 5;
    }

    zOpt::SetThrottleMode(0);
    zOpt::SetSteeringMode(0);
    zOpt::SetCursorMode(0);
    gameControl |= 8;
    if (gameControl != 8 || zOpt::GetThrottleMode() != 0 || zOpt::GetSteeringMode() != 0 ||
        zOpt::GetCursorMode() != 0 || zOpt::GetCameraModePlayerState() != 1) {
        return 6;
    }

    zOpt::SetGameDifficultyMode(2);
    if (difficulty != 2 || zOpt::GetGameDifficultyMode() != 2) {
        return 7;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetEffectsLevelForCurrentHwMode(0);
    if (effectsSw != 0 || effectsHw != 0 || g_zEffect_ConditionalEffectLevel != 2 ||
        zOpt::GetEffectsLevelForCurrentHwMode() != 0) {
        return 8;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetEffectsLevelForCurrentHwMode(2);
    if (effectsHw != 2 || g_zEffect_ConditionalEffectLevel != 0 ||
        zOpt::GetEffectsLevelForCurrentHwMode() != 2) {
        return 9;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetObjectLODForCurrentHwMode(1);
    if (objectLodSw != 1 || objectLodHw != 0 || zOpt::GetObjectLODForCurrentHwMode() != 1) {
        return 10;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetObjectLODForCurrentHwMode(2);
    if (objectLodHw != 2 || zOpt::GetObjectLODForCurrentHwMode() != 2) {
        return 11;
    }

    muteSound = 1;
    if (zOpt::GetMuteSoundOption() != 1) {
        return 12;
    }

    zOpt::SetSoundVolumeOption(0.625f);
    if (soundVolume != 0.625f || globalVolume != 0.625f || zOpt::GetSoundVolumeOption() != 0.625f) {
        return 13;
    }

    zOpt::SetSoundLODOption(3);
    if (soundLod != 3 || zOpt::GetSoundLODOption() != 3) {
        return 14;
    }

    g_zOpt_HwMode = 0;
    zOpt::SetTextureMemoryForCurrentHwMode(16);
    if (textureMemorySw != 16 || zOpt::GetTextureMemoryForCurrentHwMode() != 16) {
        return 15;
    }

    g_zOpt_HwMode = 1;
    zOpt::SetTextureMemoryForCurrentHwMode(32);
    if (textureMemoryHw != 32 || zOpt::GetTextureMemoryForCurrentHwMode() != 32) {
        return 16;
    }

    g_zOpt_HwMode = 0;
    gfxFlagsSw = 0x11;
    if (zOpt::GetGraphicsFlagsForCurrentHwMode() != 0x11) {
        return 17;
    }

    g_zOpt_HwMode = 1;
    gfxFlagsHw = 0x22;
    if (zOpt::GetGraphicsFlagsForCurrentHwMode() != 0x22) {
        return 18;
    }

    zOpt::SetPlayerName("Pilot");
    if (std::strcmp(playerNameBuffer, "Pilot") != 0 || zOpt_GetPlayerName() != playerNameBuffer) {
        return 19;
    }

    zOpt::SetPlayerName("LongPlayerName");
    if (std::strcmp(playerNameBuffer, "LongPla") != 0) {
        return 20;
    }

    ZOPT_GAME_CONTROL_OPTIONS = nullptr;
    ZOPT_HUD_TYPE_SW = nullptr;
    ZOPT_HUD_TYPE_HW = nullptr;
    g_zOpt_GameDifficultyOption = nullptr;
    ZOPT_EFFECTS_LEVEL_SW = nullptr;
    ZOPT_EFFECTS_LEVEL_HW = nullptr;
    ZOPT_OBJECT_LOD_SW = nullptr;
    ZOPT_OBJECT_LOD_HW = nullptr;
    ZOPT_MUTE_SOUND = nullptr;
    ZOPT_SOUND_VOLUME = nullptr;
    g_zSnd_GlobalVolumeScalePtr = nullptr;
    ZOPT_SOUND_LOD = nullptr;
    ZOPT_TEXTURE_MEMORY_SW = nullptr;
    ZOPT_TEXTURE_MEMORY_HW = nullptr;
    ZOPT_GFX_FLAGS_SW = nullptr;
    ZOPT_GFX_FLAGS_HW = nullptr;
    ZOPT_PLAYER_NAME = nullptr;
    return 0;
}

extern "C" int zopt_network_enabled_accessor_smoke() {
    std::int32_t networkEnabled = 0;
    std::int32_t networkModem = 0;
    std::int32_t networkListen = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;
    g_zOpt_NetworkModemOption = &networkModem;
    g_zOpt_NetworkListenOption = &networkListen;

    const bool disabled = zOpt::GetNetworkEnabled() == 0;
    zOpt::SetNetworkEnabled(1);
    zOpt::SetNetworkModemEnabled(1);
    zOpt::SetNetworkListenEnabled(1);
    const bool enabled = zOpt::GetNetworkEnabled() == 1;
    const bool modemEnabled = networkModem == 1 && zOpt::GetNetworkModemEnabled() == 1;
    const bool listenEnabled = networkListen == 1;

    ZOPT_NETWORK_ENABLED = nullptr;
    g_zOpt_NetworkModemOption = nullptr;
    g_zOpt_NetworkListenOption = nullptr;
    return disabled && enabled && modemEnabled && listenEnabled ? 0 : 1;
}

extern "C" int hud_sensor_objective_slot_reset_smoke() {
    HudSensorObjectiveSlot slot{};
    slot.completedFlag = 7;
    slot.objectiveTitle[0] = 'T';
    slot.objectiveDesc[0] = 'D';
    slot.objectiveSummary[0] = 'S';

    slot.Reset();

    return slot.completedFlag == 0 && slot.objectiveImage == nullptr &&
                   slot.objectiveTitle[0] == '\0' && slot.objectiveDesc[0] == '\0' &&
                   slot.objectiveSummary[0] == '\0'
               ? 0
               : 1;
}

extern "C" int hud_sensor_tracker_unload_objectives_smoke() {
    std::int32_t networkEnabled = 0;
    ZOPT_NETWORK_ENABLED = &networkEnabled;

    HudSensorTracker tracker{};
    tracker.objectiveCount = 2;
    tracker.currentObjectiveIndex = 4;
    tracker.firstIncompleteObjectiveIndex = 5;
    tracker.completedObjectiveCount = 6;
    tracker.objectiveSlots[0].completedFlag = 1;
    tracker.objectiveSlots[0].objectiveTitle[0] = 'A';
    tracker.objectiveSlots[1].completedFlag = 1;
    tracker.objectiveSlots[1].objectiveTitle[0] = 'B';

    const bool disabledResult =
        tracker.UnloadObjectives() == 1 && tracker.currentObjectiveIndex == -1 &&
        tracker.firstIncompleteObjectiveIndex == 0 && tracker.objectiveCount == 0 &&
        tracker.completedObjectiveCount == 0 && tracker.objectiveSlots[0].completedFlag == 0 &&
        tracker.objectiveSlots[0].objectiveTitle[0] == '\0' &&
        tracker.objectiveSlots[1].completedFlag == 0 &&
        tracker.objectiveSlots[1].objectiveTitle[0] == '\0';

    networkEnabled = 1;
    tracker.objectiveCount = 1;
    tracker.currentObjectiveIndex = 8;
    tracker.completedObjectiveCount = 9;
    tracker.objectiveSlots[0].completedFlag = 3;
    tracker.objectiveSlots[0].objectiveTitle[0] = 'N';

    const bool enabledResult = tracker.UnloadObjectives() == 1 &&
                               tracker.currentObjectiveIndex == 8 && tracker.objectiveCount == 1 &&
                               tracker.completedObjectiveCount == 9 &&
                               tracker.objectiveSlots[0].completedFlag == 3 &&
                               tracker.objectiveSlots[0].objectiveTitle[0] == 'N';

    ZOPT_NETWORK_ENABLED = nullptr;

    return disabledResult && enabledResult ? 0 : 1;
}

extern "C" int zopt_view_rect_target_side_effects_smoke() {
    zOpt_ViewRectSection render{};
    zOpt_ViewRectSection display{};
    zClass_WindowDataPartial windowData{};
    zClass_DisplayDataPartial displayData{};
    zClass_NodePartial windowNode{};
    zClass_NodePartial displayNode{};
    windowNode.classId = 3;
    windowNode.classData = &windowData;
    displayNode.classId = 4;
    displayNode.classData = &displayData;
    render.target = &windowNode;
    display.target = &displayNode;
    zOpt_ViewRectSection *renderPtr = &render;
    zOpt_ViewRectSection *displayPtr = &display;
    g_zOpt_RenderSectionOption = &renderPtr;
    g_zOpt_DisplaySectionOption = &displayPtr;

    render.x = 5;
    render.y = 6;
    render.width = 320;
    render.height = 240;
    render.target = nullptr;
    zOpt::RenderSection_SetTargetWindow(&windowNode);
    if (zOpt::GetRenderSection() != &render || windowData.resolutionWidth != 320 ||
        windowData.resolutionHeight != 240 || windowData.viewportWidth != 5 ||
        windowData.viewportHeight != 6) {
        return 3;
    }

    display.x = 7;
    display.y = 8;
    display.width = 400;
    display.height = 300;
    display.target = nullptr;
    zOpt::DisplaySection_SetTargetDisplay(&displayNode);
    if (displayData.width != 400 || displayData.height != 300 || displayData.x != 7 ||
        displayData.y != 8) {
        return 4;
    }

    zOpt::RenderSection_SetSize(640, 480);
    zOpt::RenderSection_SetPosition(10, 20);
    if (windowData.resolutionWidth != 640 || windowData.resolutionHeight != 480 ||
        windowData.viewportWidth != 10 || windowData.viewportHeight != 20) {
        return 1;
    }

    zOpt::DisplaySection_SetSize(800, 600);
    zOpt::DisplaySection_SetPosition(30, 40);
    return displayData.width == 800 && displayData.height == 600 && displayData.x == 30 &&
                   displayData.y == 40
               ? 0
               : 2;
}

extern "C" int zopt_section_accessor_smoke() {
    std::int32_t videoStride = 2048;
    zOpt_ViewRectSection display{};
    zOpt_ViewRectSection window{};
    display.width = 800;
    display.height = 600;
    display.bitsPerPixel = 16;
    window.height = 480;
    std::int32_t replicate = 1;
    zOpt_ViewRectSection *displayPtr = &display;
    zOpt_ViewRectSection *windowPtr = &window;
    ZOPT_VIDEO_STRIDE = &videoStride;
    ZOPT_REPLICATE = &replicate;
    g_zOpt_DisplaySectionOption = &displayPtr;
    g_zOpt_WindowSectionOption = &windowPtr;

    zOpt_ViewRectSection clamp{};
    clamp.x = 10;
    clamp.y = 20;
    clamp.maxXInclusive = 30;
    clamp.maxYInclusive = 40;

    float belowPoint[2] = {5.0f, 15.0f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, belowPoint);
    const bool belowClamped = belowPoint[0] == 10.0f && belowPoint[1] == 20.0f;

    float insidePoint[2] = {11.5f, 25.25f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, insidePoint);
    const bool insideKept = insidePoint[0] == 11.5f && insidePoint[1] == 25.25f;

    float abovePoint[2] = {31.0f, 45.0f};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, abovePoint);
    const bool aboveClamped = abovePoint[0] == 30.0f && abovePoint[1] == 40.0f;

    float nanPoint[2] = {std::numeric_limits<float>::quiet_NaN(),
                         std::numeric_limits<float>::quiet_NaN()};
    zOpt::ViewRectSection_ClampPointToInclusiveBounds(&clamp, nanPoint);
    const bool nanClampedToMax = nanPoint[0] == 30.0f && nanPoint[1] == 40.0f;

    return zOpt::GetDisplaySection() == &display && zOpt_DisplaySection_GetWidth() == 800 &&
                   zOpt_DisplaySection_GetHeight() == 600 &&
                   zOpt::GetDisplaySectionBitsPerPixel() == 16 &&
                   zOpt::GetVideoStrideValue() == 2048 && zOpt::GetReplicateMode() == 1 &&
                   zOpt::GetWindowSection() == &window && zOpt::GetWindowSectionHeight() == 480 &&
                   belowClamped && insideKept && aboveClamped && nanClampedToMax
               ? 0
               : 1;
}

extern "C" int zclipalt_set_source_rect_smoke() {
    zClipAltFloatRect rect{3.0f, 4.0f, 19.0f, 25.0f};
    gAltClipSourceRectValid = 0;

    zClipAlt::SetSourceRect(&rect);

    return g_zClipAlt_SourceLeft == 3.0f && g_zClipAlt_SourceTop == 4.0f &&
                   g_zClipAlt_SourceRight == 19.0f && g_zClipAlt_SourceBottom == 25.0f &&
                   g_zClipAlt_SourceWidth == 16.0f && g_zClipAlt_SourceHeight == 21.0f &&
                   gAltClipSourceRectValid == 1
               ? 0
               : 1;
}

extern "C" int zclipalt_set_target_rect_smoke() {
    zClipAltFloatRect source{2.0f, 4.0f, 18.0f, 28.0f};
    zClipAltFloatRect target{10.0f, 20.0f, 42.0f, 44.0f};
    gClipRect_Primary.xMin = 100.0f;
    gClipRect_Primary.yMin = 60.0f;

    zClipAlt::SetSourceRect(&source);
    g_zClipAlt_BiasIncludesPrimaryOrigin = 0;
    zClipAlt::SetTargetRect(&target, 0);

    if (gClipRect_Alt.flags != 0x0f || gClipRect_Alt.xMin != 10.0f || gClipRect_Alt.yMin != 20.0f ||
        gClipRect_Alt.xMax != 42.0f || gClipRect_Alt.yMax != 44.0f ||
        gClipRect_Alt.xMaxAlt != 42.0f || gClipRect_Alt.yMaxAlt != 44.0f ||
        g_zClipAlt_RemapOffsetX != 8.0f || g_zClipAlt_RemapOffsetY != 16.0f ||
        g_zClipAlt_RemapScaleX != 0.5f || g_zClipAlt_RemapScaleY != 1.0f ||
        g_zClipAlt_RemapBiasX != -3.0f || g_zClipAlt_RemapBiasY != -16.0f) {
        return 1;
    }

    g_zClipAlt_BiasIncludesPrimaryOrigin = 1;
    zClipAlt::SetTargetRect(&target, 1);
    return g_zClipAlt_RemapBiasX == 47.0f && g_zClipAlt_RemapBiasY == 14.0f ? 0 : 2;
}

extern "C" int zclass_cls_di_set_stop_after_first_hit_smoke() {
    g_cls_di_StopAfterFirstHit = 0;
    zClass_cls_di::SetStopAfterFirstHit(7);

    if (g_cls_di_StopAfterFirstHit != 7) {
        return 1;
    }

    zVec3 triZ[3] = {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    zVec3 segmentStart{0.25f, 0.25f, 1.0f};
    zVec3 segmentEnd{0.25f, 0.25f, -1.0f};
    zClassDiPickCandidateEntry candidate{};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triZ, 3, 0) != 1 ||
        candidate.surfaceNormal.x != 0.0f || candidate.surfaceNormal.y != 0.0f ||
        candidate.surfaceNormal.z != 1.0f || candidate.hitPos.x != 0.25f ||
        candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.0f) {
        return 2;
    }

    segmentStart = {1.25f, 0.25f, 1.0f};
    segmentEnd = {1.25f, 0.25f, -1.0f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triZ, 3, 0) != 0) {
        return 3;
    }

    segmentStart = {0.25f, 0.25f, -1.0f};
    segmentEnd = {0.25f, 0.25f, 1.0f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triZ, 3, 0) != 0 ||
        zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triZ, 3, 1) != 1) {
        return 4;
    }

    zVec3 triNegZ[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triNegZ, 3, 0) != 1 ||
        candidate.surfaceNormal.z != -1.0f) {
        return 5;
    }

    zVec3 triX[3] = {{0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    segmentStart = {1.0f, 0.25f, 0.25f};
    segmentEnd = {-1.0f, 0.25f, 0.25f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygon(&candidate, &segmentStart, &segmentEnd,
                                                             triX, 3, 0) != 1 ||
        candidate.surfaceNormal.x != 1.0f || candidate.hitPos.x != 0.0f ||
        candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.25f) {
        return 6;
    }

    zVec3 boxCornerValues[8] = {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f},
                                {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f},
                                {1.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    zBBoxCorners boxCorners{};
    for (std::int32_t i = 0; i < 8; ++i) {
        boxCorners.values[i * 3 + 0] = boxCornerValues[i].x;
        boxCorners.values[i * 3 + 1] = boxCornerValues[i].y;
        boxCorners.values[i * 3 + 2] = boxCornerValues[i].z;
    }

    candidate.scenePayload = &candidate;
    segmentStart = {0.5f, 0.5f, 2.0f};
    segmentEnd = {0.5f, 0.5f, 0.5f};
    if (zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces(&boxCorners, &candidate,
                                                                &segmentStart, &segmentEnd) != 1 ||
        candidate.scenePayload != nullptr || candidate.surfaceNormal.z != 1.0f ||
        candidate.hitPos.x != 0.5f || candidate.hitPos.y != 0.5f || candidate.hitPos.z != 1.0f ||
        g_zClass_DiFaceVertexScratch4[1].x != 1.0f || g_zClass_DiFaceVertexScratch4[2].y != 1.0f) {
        return 7;
    }

    candidate.scenePayload = &candidate;
    segmentStart = {2.0f, 2.0f, 2.0f};
    segmentEnd = {2.0f, 2.0f, -1.0f};
    if (zClass_cls_di::BuildPickCandidatesForSegmentVsBBoxFaces(&boxCorners, &candidate,
                                                                &segmentStart, &segmentEnd) != 0 ||
        candidate.scenePayload != nullptr) {
        return 8;
    }

    static std::int32_t filterMatrixFlags[8];
    static float *filterMatrixSlots[8];
    static zMat4x3 filterMatrix;
    filterMatrixFlags[0] = 1;
    filterMatrixSlots[0] = reinterpret_cast<float *>(&filterMatrix);
    zMath::g_currentMatrixIdentityFlagSlot = &filterMatrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &filterMatrixSlots[0];

    zClass_NodePartial filterNode{};
    std::int32_t filterClassData = 0;
    filterNode.classId = 2;
    filterNode.classData = &filterClassData;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 1) {
        return 9;
    }

    filterNode.flags = 0x100;
    filterNode.cachedBounds[0] = 0.0f;
    filterNode.cachedBounds[1] = 0.0f;
    filterNode.cachedBounds[2] = 0.0f;
    filterNode.cachedBounds[3] = 1.0f;
    filterNode.cachedBounds[4] = 1.0f;
    filterNode.cachedBounds[5] = 1.0f;
    g_DiSegmentMinX = 0.25f;
    g_DiSegmentMinY = 0.25f;
    g_DiSegmentMinZ = 0.25f;
    g_DiSegmentMaxX = 0.75f;
    g_DiSegmentMaxY = 0.75f;
    g_DiSegmentMaxZ = 0.75f;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 0) {
        return 10;
    }

    g_DiSegmentMaxX = 0.0f;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 1) {
        return 11;
    }

    filterNode.flags = 0x120;
    g_DiPickCandidateCursor = &candidate;
    g_DiPickQueryPoint = {0.5f, 0.5f, 2.0f};
    g_DiSegmentEnd = {0.5f, 0.5f, 0.5f};
    g_DiSegmentMinX = 0.5f;
    g_DiSegmentMinY = 0.5f;
    g_DiSegmentMinZ = 0.5f;
    g_DiSegmentMaxX = 0.5f;
    g_DiSegmentMaxY = 0.5f;
    g_DiSegmentMaxZ = 2.0f;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 0 || candidate.hitPos.z != 1.0f) {
        return 12;
    }

    candidate.scenePayload = &candidate;
    g_DiPickQueryPoint = {0.5f, 0.5f, 0.5f};
    g_DiSegmentEnd = {0.75f, 0.75f, 0.75f};
    g_DiSegmentMinX = 0.5f;
    g_DiSegmentMinY = 0.5f;
    g_DiSegmentMinZ = 0.5f;
    g_DiSegmentMaxX = 0.75f;
    g_DiSegmentMaxY = 0.75f;
    g_DiSegmentMaxZ = 0.75f;
    if (zClass_cls_di::FilterPointsBBox(&filterNode, nullptr) != 1 ||
        candidate.scenePayload != nullptr) {
        return 13;
    }

    zModel_PickFaceUvData faceUvData = {{{0.0f, 0.0f}, {10.0f, 0.0f}, {0.0f, 20.0f}}};
    zVec2 outUv{};
    g_OptCatalogDamageMaskPhaseU = -1.0f;
    g_OptCatalogDamageMaskPhaseV = -1.0f;
    segmentStart = {0.25f, 0.25f, 1.0f};
    segmentEnd = {0.25f, 0.25f, -1.0f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv(
            &candidate, &segmentStart, &segmentEnd, triZ, &faceUvData, &outUv, 3, 0) != 1 ||
        candidate.hitPos.x != 0.25f || candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.0f ||
        outUv.x != 2.5f || outUv.y != 5.0f || g_OptCatalogDamageMaskPhaseU != 2.5f ||
        g_OptCatalogDamageMaskPhaseV != 5.0f) {
        return 14;
    }

    zModel_PickFaceUvData xFaceUvData = {{{0.0f, 0.0f}, {4.0f, 0.0f}, {0.0f, 8.0f}}};
    outUv = {};
    segmentStart = {1.0f, 0.25f, 0.25f};
    segmentEnd = {-1.0f, 0.25f, 0.25f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv(
            &candidate, &segmentStart, &segmentEnd, triX, &xFaceUvData, &outUv, 3, 0) != 1 ||
        candidate.hitPos.x != 0.0f || candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.25f ||
        outUv.x != 1.0f || outUv.y != 2.0f || g_OptCatalogDamageMaskPhaseU != 1.0f ||
        g_OptCatalogDamageMaskPhaseV != 2.0f) {
        return 15;
    }

    g_OptCatalogDamageMaskPhaseU = 123.0f;
    g_OptCatalogDamageMaskPhaseV = 456.0f;
    segmentStart = {1.25f, 0.25f, 1.0f};
    segmentEnd = {1.25f, 0.25f, -1.0f};
    if (zClass_cls_di::BuildPickCandidateForSegmentVsPolygonWithUv(
            &candidate, &segmentStart, &segmentEnd, triZ, &faceUvData, &outUv, 3, 0) != 0 ||
        g_OptCatalogDamageMaskPhaseU != 123.0f || g_OptCatalogDamageMaskPhaseV != 456.0f) {
        return 16;
    }

    g_zModel_SharedVec3ScratchA = g_zModel_SharedVec3ScratchAStorage;
    zModel_PickFaceScenePayload facePayload{};
    std::int32_t faceIndices[3] = {0, 1, 2};
    zModel_PickFaceEntry faceEntry{};
    faceEntry.flagsAndVertexCount = 3;
    faceEntry.vertexIndices = faceIndices;
    faceEntry.faceUvData = &faceUvData;
    faceEntry.scenePayload = &facePayload;

    zModel_PickFaceData faceData{};
    faceData.faceCount = 1;
    faceData.faces = &faceEntry;
    faceData.baseVertices = triZ;
    filterMatrixFlags[0] = 1;
    candidate = {};
    segmentStart = {0.25f, 0.25f, 1.0f};
    segmentEnd = {0.25f, 0.25f, -1.0f};
    if (zClass_cls_di::AppendPickCandidatesForFace(&faceData, &candidate, &segmentStart,
                                                   &segmentEnd) != 1 ||
        candidate.scenePayload != &facePayload || candidate.hitPos.x != 0.25f ||
        candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.0f ||
        candidate.surfaceNormal.z != 1.0f) {
        return 17;
    }

    facePayload.flags = 0x0200;
    g_OptCatalogDamageMaskPhaseU = -1.0f;
    g_OptCatalogDamageMaskPhaseV = -1.0f;
    candidate = {};
    if (zClass_cls_di::AppendPickCandidatesForFace(&faceData, &candidate, &segmentStart,
                                                   &segmentEnd) != 1 ||
        candidate.scenePayload != &facePayload || g_OptCatalogDamageMaskPhaseU != 2.5f ||
        g_OptCatalogDamageMaskPhaseV != 5.0f) {
        return 18;
    }

    facePayload.flags = 0;
    zVec3 morphDelta[3] = {{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    faceData.flags = 8;
    faceData.morphVertexCount = 3;
    faceData.morphWeight = 0.5f;
    faceData.morphVertices = morphDelta;
    candidate = {};
    if (zClass_cls_di::AppendPickCandidatesForFace(&faceData, &candidate, &segmentStart,
                                                   &segmentEnd) != 1 ||
        candidate.scenePayload != &facePayload || candidate.hitPos.z != 0.5f ||
        g_zModel_SharedVec3ScratchA[0].z != 0.5f || g_zModel_SharedVec3ScratchA[1].z != 0.5f ||
        g_zModel_SharedVec3ScratchA[2].z != 0.5f) {
        return 19;
    }

    faceData.flags = 0;
    faceData.morphVertexCount = 0;
    faceData.morphWeight = 0.0f;
    faceData.morphVertices = nullptr;
    filterMatrixFlags[0] = 0;
    filterMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 10.0f, 0.0f, 0.0f};
    candidate = {};
    segmentStart = {10.25f, 0.25f, 1.0f};
    segmentEnd = {10.25f, 0.25f, -1.0f};
    if (zClass_cls_di::AppendPickCandidatesForFace(&faceData, &candidate, &segmentStart,
                                                   &segmentEnd) != 1 ||
        candidate.scenePayload != &facePayload || candidate.hitPos.x != 10.25f ||
        candidate.hitPos.y != 0.25f || candidate.hitPos.z != 0.0f ||
        candidate.surfaceNormal.x != 0.0f || candidate.surfaceNormal.y != 0.0f ||
        candidate.surfaceNormal.z != 1.0f) {
        return 20;
    }

    auto setIdentityMatrix = [](float *matrixValues) {
        std::memset(matrixValues, 0, sizeof(zMat4x3));
        matrixValues[0] = 1.0f;
        matrixValues[4] = 1.0f;
        matrixValues[8] = 1.0f;
    };

    PlayerProbeSampleCandidateBuffer pickBuffer{};
    g_DiPickCandidateBuffer = &pickBuffer;
    g_DiPickCandidateCursor = pickBuffer.entries;
    g_cls_di_BreakOnFirstCandidate = 0;
    g_cls_di_StopAfterFirstHit = 0;
    g_Variant_CurrentTag = {};
    filterMatrixFlags[0] = 1;
    filterMatrix = {1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f};
    zMath::g_currentMatrixIdentityFlagSlot = &filterMatrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &filterMatrixSlots[0];
    g_DiPickQueryPoint = {0.25f, 0.25f, 1.0f};
    g_DiSegmentEnd = {0.25f, 0.25f, -1.0f};

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 8;
    zClass_NodePartial objectNode{};
    objectNode.flags = 0x114;
    objectNode.nodeType = 0xff;
    objectNode.classId = 5;
    objectNode.classData = &objectData;
    objectNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    objectNode.cachedBounds[0] = 0.0f;
    objectNode.cachedBounds[1] = 0.0f;
    objectNode.cachedBounds[2] = 0.0f;
    objectNode.cachedBounds[3] = 1.0f;
    objectNode.cachedBounds[4] = 1.0f;
    objectNode.cachedBounds[5] = 1.0f;
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&objectNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || g_DiPickCandidateCursor != &pickBuffer.entries[1] ||
        pickBuffer.entries[0].node != &objectNode ||
        pickBuffer.entries[0].scenePayload != &facePayload ||
        pickBuffer.entries[0].hitPos.z != 0.0f) {
        return 21;
    }

    g_cls_di_BreakOnFirstCandidate = 1;
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&objectNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || g_DiPickCandidateCursor != &pickBuffer.entries[1]) {
        return 22;
    }
    g_cls_di_BreakOnFirstCandidate = 0;

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_AnimateDataPartial animateData{};
    setIdentityMatrix(animateData.savedParentMatrix);
    setIdentityMatrix(animateData.animatedTransform);
    zClass_NodePartial animateNode{};
    animateNode.flags = 0x14;
    animateNode.nodeType = 0xff;
    animateNode.classId = 8;
    animateNode.classData = &animateData;
    animateNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&animateNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &animateNode ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0]) {
        return 23;
    }

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_CameraDataPartial cameraData{};
    setIdentityMatrix(cameraData.worldTransform);
    zClass_NodePartial cameraNode{};
    cameraNode.flags = 0x14;
    cameraNode.nodeType = 0xff;
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&cameraNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &cameraNode ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0]) {
        return 24;
    }

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_LightDataPartial lightData{};
    setIdentityMatrix(lightData.savedParentMatrix);
    zClass_NodePartial lightNode{};
    lightNode.flags = 0x14;
    lightNode.nodeType = 0xff;
    lightNode.classId = 9;
    lightNode.classData = &lightData;
    lightNode.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&faceData);
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&lightNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &lightNode ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0]) {
        return 25;
    }

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_SequenceDataPartial sequenceData{};
    sequenceData.isActive = 1;
    sequenceData.currentIndex = 0;
    sequenceData.entries[0].node = &objectNode;
    zClass_NodePartial sequenceNode{};
    sequenceNode.flags = 0x14;
    sequenceNode.nodeType = 0xff;
    sequenceNode.classId = 7;
    sequenceNode.classData = &sequenceData;
    if (zClass_cls_di::BuildPickCandidatesForSegmentChildFallback(&sequenceNode, 1) != 0 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &objectNode) {
        return 26;
    }

    pickBuffer = {};
    g_DiPickCandidateCursor = pickBuffer.entries;
    zClass_NodePartial *segmentChildren[1] = {&objectNode};
    zClass_NodePartial segmentRoot{};
    segmentRoot.listCountB = 1;
    segmentRoot.listB = segmentChildren;
    if (zClass_cls_di::BuildPickCandidatesForSegment(&segmentRoot) != 1 ||
        pickBuffer.candidateCount != 1 || pickBuffer.entries[0].node != &objectNode) {
        return 27;
    }

    PlayerProbeSampleCandidateBuffer rayBuffer{};
    zClass_NodePartial *areaChildren[1] = {&objectNode};
    zWorldAreaPartial areaCells[1] = {};
    areaCells[0].childCount = 1;
    areaCells[0].childList = areaChildren;
    zWorldAreaPartial *areaRows[1] = {areaCells};
    zClass_WorldDataPartial rayWorldData{};
    rayWorldData.originX = 0.0f;
    rayWorldData.originZ = -2.0f;
    rayWorldData.worldMaxX = 2.0f;
    rayWorldData.worldMaxZ = 2.0f;
    rayWorldData.areaCellSizeX = 2.0f;
    rayWorldData.areaCellSizeZ = 4.0f;
    rayWorldData.areaInvSizeX = 0.5f;
    rayWorldData.areaInvSizeZ = 0.25f;
    rayWorldData.areaGridColCount = 1;
    rayWorldData.areaGridRowCount = 1;
    rayWorldData.areaGridRows = areaRows;
    zClass_NodePartial rayWorld{};
    rayWorld.classData = &rayWorldData;
    if (zClass_cls_di::RaycastFindClosest(&rayWorld, &rayBuffer, 0.25f, 0.25f, 1.0f, 0.25f, 0.25f,
                                          -1.0f) != 0 ||
        rayBuffer.candidateCount != 1 || rayBuffer.entries[0].node != &objectNode ||
        rayBuffer.entries[0].hitPos.z != 0.0f ||
        zMath::g_currentMatrixPtrSlot != &filterMatrixSlots[0] || g_cls_di_StopAfterFirstHit != 0) {
        return 28;
    }

    zVec3 rayStart{0.25f, 0.25f, 1.0f};
    zVec3 rayEnd{0.25f, 0.25f, -1.0f};
    rayBuffer = {};
    if (zClass_cls_di::RaycastSelectClosestHitBetweenPoints(&rayWorld, &rayStart, &rayEnd,
                                                            &rayBuffer) != 0 ||
        rayBuffer.candidateCount != 0 || rayBuffer.entries[0].node != &objectNode) {
        return 29;
    }

    facePayload.flags = 0;
    zVec3 selectTris[3][3] = {
        {
            {0.0f, 0.0f, 0.0f},
            {1.0f, 0.0f, 0.0f},
            {0.0f, 1.0f, 0.0f},
        },
        {
            {0.0f, 0.0f, 0.75f},
            {1.0f, 0.0f, 0.75f},
            {0.0f, 1.0f, 0.75f},
        },
        {
            {0.0f, 0.0f, -0.5f},
            {1.0f, 0.0f, -0.5f},
            {0.0f, 1.0f, -0.5f},
        },
    };
    zModel_PickFaceEntry selectFaces[3] = {};
    zModel_PickFaceData selectFaceData[3] = {};
    zClass_NodePartial selectNodes[3] = {};
    for (std::int32_t i = 0; i < 3; ++i) {
        selectFaces[i].flagsAndVertexCount = 3;
        selectFaces[i].vertexIndices = faceIndices;
        selectFaces[i].faceUvData = &faceUvData;
        selectFaces[i].scenePayload = &facePayload;
        selectFaceData[i].faceCount = 1;
        selectFaceData[i].faces = &selectFaces[i];
        selectFaceData[i].baseVertices = selectTris[i];

        selectNodes[i].flags = 0x114;
        selectNodes[i].nodeType = 0xff;
        selectNodes[i].classId = 5;
        selectNodes[i].classData = &objectData;
        selectNodes[i].userDataOrDiRef = reinterpret_cast<std::uint32_t>(&selectFaceData[i]);
        selectNodes[i].cachedBounds[0] = 0.0f;
        selectNodes[i].cachedBounds[1] = 0.0f;
        selectNodes[i].cachedBounds[2] = -1.0f;
        selectNodes[i].cachedBounds[3] = 1.0f;
        selectNodes[i].cachedBounds[4] = 1.0f;
        selectNodes[i].cachedBounds[5] = 1.0f;
    }

    zClass_NodePartial *selectChildren[3] = {&selectNodes[0], &selectNodes[1], &selectNodes[2]};
    zClass_WorldDataPartial selectWorldData{};
    zClass_NodePartial selectWorld{};
    selectWorld.classData = &selectWorldData;
    selectWorld.listCountB = 3;
    selectWorld.listB = selectChildren;
    PlayerProbeSampleCandidateBuffer selectRayBuffer{};
    rayStart = {0.25f, 0.25f, 2.0f};
    rayEnd = {0.25f, 0.25f, -2.0f};
    if (zClass_cls_di::RaycastSelectClosestHitBetweenPoints(&selectWorld, &rayStart, &rayEnd,
                                                            &selectRayBuffer) != 0 ||
        selectRayBuffer.candidateCount != 1 || selectRayBuffer.entries[1].node != &selectNodes[1] ||
        selectRayBuffer.entries[1].hitPos.z != 0.75f) {
        return 30;
    }

    zClass_NodePartial emptyWorld{};
    emptyWorld.classData = &selectWorldData;
    PlayerProbeSampleCandidateBuffer emptyRayBuffer{};
    if (zClass_cls_di::RaycastSelectClosestHitBetweenPoints(&emptyWorld, &rayStart, &rayEnd,
                                                            &emptyRayBuffer) != 1 ||
        emptyRayBuffer.candidateCount != 0) {
        return 31;
    }

    g_DiPickCandidateBuffer = nullptr;
    g_DiPickCandidateCursor = nullptr;

    return 0;
}

extern "C" int zclass_node_propagate_transform_dirty_smoke() {
    std::int32_t parentObjectFlags = 0;
    std::int32_t childObjectFlags = 0;
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    zClass_NodePartial skippedChild{};
    zClass_NodePartial *children[2] = {&child, &skippedChild};
    parent.classId = 5;
    parent.classData = &parentObjectFlags;
    parent.listCountB = 2;
    parent.listB = children;
    child.classId = 5;
    child.classData = &childObjectFlags;
    skippedChild.flags = 0x02000000;

    zClass_Node::PropagateTransformDirtyRecursive(&parent);

    return (parentObjectFlags & 0x20) != 0 && (childObjectFlags & 0x20) != 0 &&
                   (parent.boundsFlags & 4) != 0 && (parent.flags & 0x02000000) != 0 &&
                   (child.flags & 0x02000000) != 0 && skippedChild.boundsFlags == 0
               ? 0
               : 1;
}

extern "C" int player_add_scaled_hud_counter_smoke() {
    g_Player_HudCounterValue = 10;
    g_HudSensorTracker.primaryGunDispatchCount = 0;
    g_OptCatalog_DamageFeedbackHitCount = 7;

    Player::AddScaledHudCounterValue(1.25f);
    if (g_Player_HudCounterValue != 1260) {
        return 1;
    }

    g_HudSensorTracker.primaryGunDispatchCount = 4;
    g_OptCatalog_DamageFeedbackHitCount = 3;
    Player::AddScaledHudCounterValue(2.0f);
    if (g_Player_HudCounterValue != 2760) {
        return 2;
    }

    g_HudSensorTracker.primaryGunDispatchCount = 4;
    g_OptCatalog_DamageFeedbackHitCount = 1;
    Player::AddScaledHudCounterValue(-1.5f);
    return g_Player_HudCounterValue == 2385 ? 0 : 3;
}

extern "C" int zutil_save_game_state_list_smoke() {
    zUtil_SaveGameState list{};
    list.next = reinterpret_cast<zUtil_SaveGameState *>(0x1);
    list.firstSaveState = reinterpret_cast<zUtil_SaveGameState *>(0x2);
    list.saveStateListHead = reinterpret_cast<zUtil_SaveGameState *>(0x3);
    list.saveStateListTail = reinterpret_cast<zUtil_SaveGameState *>(0x4);
    list.saveStateCount = 5;

    if (zUtil_SaveGameStateList_Init(&list) != &list || list.next != nullptr ||
        list.firstSaveState != nullptr || list.saveStateListHead != nullptr ||
        list.saveStateListTail != nullptr || list.saveStateCount != 0 ||
        list.playerState == nullptr) {
        return 1;
    }

    for (std::uint8_t value : list.playerState->bytes) {
        if (value != 0) {
            std::free(list.playerState);
            return 2;
        }
    }

    zUtil_SaveGameState *const first = zUtil_SaveGameStateList_AllocAppend(&list);
    zUtil_SaveGameState *const second = zUtil_SaveGameStateList_AllocAppend(&list);
    const bool linked = first != nullptr && second != nullptr && first->next == second &&
                        second->next == nullptr && list.firstSaveState == first &&
                        list.saveStateListHead == first && list.saveStateListTail == second &&
                        list.saveStateCount == 2;

    std::free(first);
    std::free(second);
    std::free(list.playerState);
    return linked ? 0 : 3;
}

extern "C" int zclass_object3d_visible_and_color_smoke() {
    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::gwObject3DSetVisibleFlag(&node, 1) != 0 || (data.flags & 4) == 0) {
        return 1;
    }
    if (zClass_Object3D::gwObject3DSetVisibleFlag(&node, 0) != 0 || (data.flags & 4) != 0) {
        return 2;
    }

    zColorRgb color{-1.0f, 0.5f, 2.0f};
    if (zClass_Object3D::gwObject3DSetColorAlpha(&node, &color, 1.5f) != 0) {
        return 3;
    }
    if (data.color.red != 0.0f || data.color.green != 0.5f || data.color.blue != 1.0f ||
        data.colorAlpha != 1.0f) {
        return 4;
    }

    data.color.red = 0.25f;
    if (zClass_Object3D::gwObject3DSetColorAlpha(&node, nullptr, -0.5f) != 0) {
        return 5;
    }
    if (data.color.red != 0.25f || data.colorAlpha != 0.0f) {
        return 6;
    }

    zClass_NodePartial wrongClass{};
    wrongClass.classId = 4;
    wrongClass.classData = &data;
    if (zClass_Object3D::gwObject3DSetVisibleFlag(nullptr, 1) != 5 ||
        zClass_Object3D::gwObject3DSetVisibleFlag(&wrongClass, 1) != 3) {
        return 7;
    }

    return 0;
}

extern "C" int zclass_object3d_alpha_scale_and_lit_smoke() {
    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::gwObject3DSetAlphaScale(&node, 0.375f) != 0) {
        return 1;
    }

    float alphaScale = 0.0f;
    if (zClass_Object3D::gwObject3DGetAlphaScale(&node, &alphaScale) != 0 || alphaScale != 0.375f) {
        return 2;
    }

    if (zClass_Object3D::gwObject3DSetLitFlag(&node, 1) != 0 || (data.flags & 2) == 0) {
        return 3;
    }
    if (zClass_Object3D::gwObject3DSetLitFlag(&node, 0) != 0 || (data.flags & 2) != 0) {
        return 4;
    }

    zClass_NodePartial nullDataNode{};
    nullDataNode.classId = 5;
    if (zClass_Object3D::gwObject3DGetAlphaScale(&nullDataNode, &alphaScale) != 5) {
        return 5;
    }

    return 0;
}

extern "C" int zclass_object3d_transform_getters_smoke() {
    zClass_Object3DDataPartial data{};
    data.rotation = {1.0f, 2.0f, 3.0f};
    data.scale = {4.0f, 5.0f, 6.0f};
    data.localMatrix[9] = 7.0f;
    data.localMatrix[10] = 8.0f;
    data.localMatrix[11] = 9.0f;

    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (zClass_Object3D::gwObject3DGetScale(&node, &x, &y, &z) != 0 || x != 4.0f || y != 5.0f ||
        z != 6.0f) {
        return 1;
    }

    if (zClass_Object3D::gwObject3DGetRotation(&node, &x, &y, &z) != 0 || x != 1.0f || y != 2.0f ||
        z != 3.0f) {
        return 2;
    }

    if (zClass_Object3D::gwObject3DGetPosition(&node, &x, &y, &z) != 0 || x != 7.0f || y != 8.0f ||
        z != 9.0f) {
        return 3;
    }
    if (zClass_Object3D::gwObject3DGetMatrixPtr(&node) != data.localMatrix) {
        return 4;
    }

    zClass_NodePartial wrongClass{};
    wrongClass.classId = 4;
    wrongClass.classData = &data;
    if (zClass_Object3D::gwObject3DGetScale(&wrongClass, &x, &y, &z) != 0 ||
        zClass_Object3D::gwObject3DGetPosition(&wrongClass, &x, &y, &z) != 3) {
        return 5;
    }

    return zClass_Object3D::gwObject3DGetRotation(nullptr, &x, &y, &z) == 5 ? 0 : 6;
}

extern "C" int zclass_object3d_transform_setters_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    data.flags = 0x18;
    if (zClass_Object3D::gwObject3DSetScale(&node, 2.0f, 1.0f, 1.0f) != 0 || data.scale.x != 2.0f ||
        data.scale.y != 1.0f || data.scale.z != 1.0f || (data.flags & 0x18) != 0 ||
        (data.flags & 0x21) != 0x21 || (node.flags & 0x02000003) != 0x02000003) {
        return 1;
    }

    data.flags = 0x18;
    if (zClass_Object3D::gwObject3DSetRotation(&node, 0.0f, 0.0f, 0.0f) != 0 ||
        (data.flags & 0x08) == 0 || (data.flags & 0x10) != 0) {
        return 2;
    }
    if (zClass_Object3D::gwObject3DTranslateRotation(&node, 0.5f, 1.0f, 1.5f) != 0 ||
        data.rotation.x != 0.5f || data.rotation.y != 1.0f || data.rotation.z != 1.5f ||
        (data.flags & 0x08) != 0) {
        return 3;
    }

    data.flags = 0x08;
    if (zClass_Object3D::gwObject3DSetPosition(&node, 0.0f, 0.0f, 0.0f) != 0 ||
        (data.flags & 0x08) == 0) {
        return 4;
    }
    if (zClass_Object3D::gwObject3DTranslatePosition(&node, 3.0f, 4.0f, 5.0f) != 0 ||
        data.localMatrix[9] != 3.0f || data.localMatrix[10] != 4.0f ||
        data.localMatrix[11] != 5.0f || (data.flags & 0x08) != 0) {
        return 5;
    }

    zClass_NodePartial wrongClass{};
    wrongClass.classId = 4;
    wrongClass.classData = &data;
    if (zClass_Object3D::gwObject3DTranslatePosition(&wrongClass, 1.0f, 2.0f, 3.0f) != 3) {
        return 6;
    }

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    return zClass_Object3D::gwObject3DSetScale(nullptr, 1.0f, 1.0f, 1.0f) == 5 &&
                   zClass_Object3D::gwObject3DSetRotation(nullptr, 0.0f, 0.0f, 0.0f) == 5 &&
                   zClass_Object3D::gwObject3DSetPosition(nullptr, 0.0f, 0.0f, 0.0f) == 5
               ? 0
               : 7;
}

extern "C" int zclass_object3d_reset_transform_dirty_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial data{};
    data.flags = 0x10;
    data.rotation = {1.0f, 2.0f, 3.0f};
    data.scale = {4.0f, 5.0f, 6.0f};
    for (int i = 0; i < 12; ++i) {
        data.localMatrix[i] = static_cast<float>(i + 1);
    }
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    if (zClass_Object3D::PropagateTransformDirty(&node) != 0) {
        return 1;
    }

    bool matrixOk = true;
    for (int i = 0; i < 12; ++i) {
        const float expected = i == 0 || i == 4 || i == 8 ? 1.0f : 0.0f;
        matrixOk = matrixOk && data.localMatrix[i] == expected;
    }

    const bool ok = data.rotation.x == 0.0f && data.rotation.y == 0.0f && data.rotation.z == 0.0f &&
                    data.scale.x == 1.0f && data.scale.y == 1.0f && data.scale.z == 1.0f &&
                    matrixOk && (data.flags & 0x39) == 0x29 && (data.flags & 0x10) == 0 &&
                    (node.flags & 0x02000003) == 0x02000003 && (node.boundsFlags & 0x04) != 0 &&
                    zClass_TypeList::Head(7) != nullptr && zClass_TypeList::Head(7)->node == &node;

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    zClass_NodePartial noData{};
    return ok && zClass_Object3D::PropagateTransformDirty(nullptr) == 5 &&
                   zClass_Object3D::PropagateTransformDirty(&noData) == 5
               ? 0
               : 2;
}

extern "C" int zclass_model_ref_lerp_queue_reset_smoke() {
    auto *const first = static_cast<zClass_Object3D_ModelRefLerpTask *>(
        ::operator new(sizeof(zClass_Object3D_ModelRefLerpTask)));
    auto *const second = static_cast<zClass_Object3D_ModelRefLerpTask *>(
        ::operator new(sizeof(zClass_Object3D_ModelRefLerpTask)));
    first->next = second;
    second->next = nullptr;

    g_ModelRefLerpQueueState.listAux = 0x11;
    g_ModelRefLerpQueueState.head = first;
    g_ModelRefLerpQueueState.tail = second;
    g_ModelRefLerpQueueState.count = 2;

    zClass_Object3D_ModelRefLerpQueue::Reset();

    return g_ModelRefLerpQueueState.listAux == 0 && g_ModelRefLerpQueueState.head == nullptr &&
                   g_ModelRefLerpQueueState.tail == nullptr && g_ModelRefLerpQueueState.count == 0
               ? 0
               : 1;
}

extern "C" int zclass_type_list_alloc_and_insert_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_TypeListLink freeSecond{};
    zClass_TypeListLink freeFirst{};
    freeFirst.next = &freeSecond;
    freeSecond.prev = &freeFirst;
    freeFirst.pendingRemove = 1;
    g_zClass_TypeList_FreeLinkHead = &freeFirst;

    zClass_TypeListLink *reused = zClass_TypeList::AllocLink();
    if (reused != &freeFirst || g_zClass_TypeList_FreeLinkHead != &freeSecond ||
        freeSecond.prev != nullptr || reused->next != nullptr || reused->prev != nullptr ||
        reused->pendingRemove != 0 || g_zClass_TypeList_LiveLinkCount != 1 ||
        g_zClass_TypeList_PeakLiveLinkCount != 1) {
        return 1;
    }

    zClass_TypeList::FreeLink(reused);
    if (g_zClass_TypeList_FreeLinkHead != reused || reused->next != &freeSecond ||
        freeSecond.prev != reused || g_zClass_TypeList_LiveLinkCount != 0) {
        return 2;
    }

    g_zClass_TypeList_FreeLinkHead = nullptr;
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    zClass_NodePartial worldChild{};
    zClass_NodePartial flaggedChild{};
    zClass_NodePartial *children[3] = {&child, &worldChild, &flaggedChild};
    parent.listCountA = 3;
    parent.listA = children;
    worldChild.classId = 2;
    flaggedChild.flags = 1;

    if (zClass_TypeList::Insert(7, &parent) != 0) {
        return 3;
    }

    zClass_TypeListLink *head = zClass_TypeList::Head(7);
    zClass_TypeListLink *tail = zClass_TypeList::Tail(7);
    const bool ok =
        head != nullptr && tail != nullptr && head->node == &parent && tail->node == &child &&
        head->next == tail && tail->prev == head && (parent.flags & 1) != 0 &&
        (child.flags & 1) != 0 && worldChild.flags == 0 && zClass_TypeList::CountNodes(7) == 2 &&
        zClass_TypeList::GetBucketHead(7) == head && g_zClass_TypeList_LiveLinkCount == 2 &&
        g_zClass_TypeList_PeakLiveLinkCount == 2;

    if (!ok) {
        return 4;
    }

    if (zClass_TypeList::MarkPendingRemoval(3, &parent) != 1) {
        return 5;
    }
    if (zClass_TypeList::MarkPendingRemoval(7, &child) != 0 || tail->pendingRemove != 1 ||
        zClass_TypeList::PendingRemovalDirty(7) != 1) {
        return 6;
    }

    for (zClass_TypeListLink *link = head; link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    zClass_NodePartial pendingNode{};
    if (zClass_NodeList::Insert(&pendingNode) != 0 ||
        g_zClass_NodeList_PendingFreeHead == nullptr ||
        g_zClass_NodeList_PendingFreeHead->node != &pendingNode) {
        return 7;
    }
    zClass_TypeList::FreeLink(g_zClass_NodeList_PendingFreeHead);
    g_zClass_NodeList_PendingFreeHead = nullptr;

    zClass_NodePartial removeNode{};
    zClass_NodePartial keepNode{};
    zClass_NodePartial tailNode{};
    removeNode.flags = 1;
    keepNode.flags = 3;
    zClass_TypeListLink *removeLink =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    zClass_TypeListLink *keepLink =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    zClass_TypeListLink *tailLink =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    if (removeLink == nullptr || keepLink == nullptr || tailLink == nullptr) {
        std::free(removeLink);
        std::free(keepLink);
        std::free(tailLink);
        return 8;
    }
    removeLink->node = &removeNode;
    removeLink->pendingRemove = 1;
    removeLink->next = keepLink;
    keepLink->node = &keepNode;
    keepLink->pendingRemove = 1;
    keepLink->prev = removeLink;
    keepLink->next = tailLink;
    tailLink->node = &tailNode;
    tailLink->prev = keepLink;
    zClass_TypeList::Head(7) = removeLink;
    zClass_TypeList::Tail(7) = tailLink;
    zClass_TypeList::PendingRemovalDirty(7) = 1;
    g_zClass_TypeList_LiveLinkCount = 3;

    zClass_TypeList::ProcessPendingRemovals(7);
    if (zClass_TypeList::Head(7) != keepLink || zClass_TypeList::Tail(7) != tailLink ||
        keepLink->pendingRemove != 0 || keepLink->prev != nullptr || keepLink->next != tailLink ||
        tailLink->prev != keepLink || (removeNode.flags & 1) != 0 || (keepNode.flags & 1) == 0 ||
        zClass_TypeList::PendingRemovalDirty(7) != 0 ||
        g_zClass_TypeList_FreeLinkHead != removeLink) {
        return 9;
    }
    std::free(keepLink);
    std::free(tailLink);
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    zClass_TypeListLink *freeA =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    zClass_TypeListLink *freeB =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    if (freeA == nullptr || freeB == nullptr) {
        std::free(freeA);
        std::free(freeB);
        return 10;
    }
    freeA->next = freeB;
    g_zClass_TypeList_FreeLinkHead = freeA;
    zClass_TypeList::FreeAll();
    return g_zClass_TypeList_FreeLinkHead == nullptr ? 0 : 11;
}

extern "C" int zclass_animate_update_smoke() {
    auto nearFloat = [](float a, float b) { return std::fabs(a - b) <= 0.0001f; };

    zClass_AnimateKeyframePartial keyframes[3]{};
    keyframes[0].rotation = {0.0f, 2.0f, 4.0f};
    keyframes[0].position = {10.0f, 20.0f, 30.0f};
    keyframes[0].scale = {1.0f, 2.0f, 3.0f};
    keyframes[1].rotation = {10.0f, 12.0f, 14.0f};
    keyframes[1].position = {30.0f, 40.0f, 50.0f};
    keyframes[1].scale = {3.0f, 4.0f, 5.0f};
    keyframes[2].rotation = {20.0f, 22.0f, 24.0f};
    keyframes[2].position = {50.0f, 60.0f, 70.0f};
    keyframes[2].scale = {5.0f, 6.0f, 7.0f};

    zClass_AnimateRuntimePartial runtime{};
    runtime.keyframes = keyframes;
    runtime.duration = 4.0f;
    runtime.currentTime = 1.0f;
    runtime.maxFrameIndex = 3;
    runtime.loopCount = -1;
    runtime.outputRotationScale = {2.0f, 3.0f, 4.0f};
    runtime.outputPositionScale = {1.0f, 0.5f, 0.25f};
    runtime.outputScaleScale = {1.0f, 2.0f, 3.0f};

    if (zClass_Animate::SampleTransform(&runtime) != 1 ||
        !nearFloat(runtime.sampledRotation.x, 10.0f) ||
        !nearFloat(runtime.sampledRotation.y, 21.0f) ||
        !nearFloat(runtime.sampledRotation.z, 36.0f) ||
        !nearFloat(runtime.sampledPosition.x, 20.0f) ||
        !nearFloat(runtime.sampledPosition.y, 15.0f) ||
        !nearFloat(runtime.sampledPosition.z, 10.0f) || !nearFloat(runtime.sampledScale.x, 2.0f) ||
        !nearFloat(runtime.sampledScale.y, 6.0f) || !nearFloat(runtime.sampledScale.z, 12.0f)) {
        return 1;
    }

    runtime.state = 2;
    if (zClass_Animate::SampleTransform(&runtime) != 2) {
        return 2;
    }

    zClass_AnimateRuntimePartial endRuntime{};
    endRuntime.duration = 2.0f;
    endRuntime.currentTime = 1.5f;
    endRuntime.loopCount = -1;
    if (zClass_Animate::AdvanceTime(&endRuntime, 0.75f) != 1 || endRuntime.state != 2 ||
        endRuntime.currentTime != 0.0f || zClass_Animate::AdvanceTime(&endRuntime, 0.25f) != 2) {
        return 3;
    }

    zClass_AnimateRuntimePartial loopRuntime{};
    loopRuntime.duration = 10.0f;
    loopRuntime.currentTime = 1.0f;
    loopRuntime.startTime = 1.5f;
    loopRuntime.loopBase = 0.25f;
    loopRuntime.loopCount = 0;
    if (zClass_Animate::AdvanceTime(&loopRuntime, 1.0f) != 1 ||
        !nearFloat(loopRuntime.currentTime, 0.75f)) {
        return 4;
    }

    reset_zclass_type_lists_for_test();
    zClass_AnimateDataPartial data{};
    data.statusFlags = 0x04;
    data.runtime.keyframes = keyframes;
    data.runtime.duration = 4.0f;
    data.runtime.currentTime = 1.0f;
    data.runtime.maxFrameIndex = 3;
    data.runtime.loopCount = -1;
    data.runtime.outputRotationScale = {1.0f, 1.0f, 1.0f};
    data.runtime.outputPositionScale = {1.0f, 1.0f, 1.0f};
    data.runtime.outputScaleScale = {1.0f, 1.0f, 1.0f};
    zClass_NodePartial node{};
    node.flags = 0x04;
    node.classData = &data;
    g_FrameDeltaTimeSec = 1.0f;

    if (zClass_Animate::UpdateNode(&node) != 0 || data.flags != 1 || (node.flags & 0x07) != 0x07 ||
        zClass_TypeList::Head(7) == nullptr || zClass_TypeList::Head(7)->node != &node ||
        !nearFloat(data.runtime.currentTime, 2.0f) ||
        !nearFloat(data.runtime.sampledRotation.x, 10.0f) ||
        !nearFloat(data.runtime.sampledPosition.y, 40.0f) ||
        !nearFloat(data.runtime.sampledScale.z, 5.0f)) {
        free_zclass_type_lists_for_test();
        return 5;
    }

    free_zclass_type_lists_for_test();
    return 0;
}

extern "C" int zclass_sequence_new_add_child_smoke() {
    reset_zclass_type_lists_for_test();
    zClass_NodeFreeListSlot slots[1]{};
    slots[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodePartial *sequence = zClass_Sequence::gwSequenceNew();
    if (sequence != &slots[0].node || sequence->classId != 7 || sequence->classData == nullptr ||
        g_zClass_NodeFreeHeadIndex != -1 || zClass_TypeList::Head(11) == nullptr ||
        zClass_TypeList::Head(11)->node != sequence) {
        if (sequence != nullptr) {
            std::free(sequence->classData);
        }
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_SequenceDataPartial *sequenceData =
        static_cast<zClass_SequenceDataPartial *>(sequence->classData);
    if (sequenceData->step != 1 || sequenceData->entryCount != 0) {
        std::free(sequence->classData);
        free_zclass_type_lists_for_test();
        return 2;
    }

    std::free(sequence->classData);
    free_zclass_type_lists_for_test();

    reset_zclass_type_lists_for_test();
    zClass_NodePartial parent{};
    zClass_NodePartial first{};
    zClass_NodePartial second{};
    zClass_NodePartial inserted{};
    const std::size_t twoEntryBytes =
        sizeof(zClass_SequenceDataPartial) + sizeof(zClass_SequenceEntryPartial);
    sequenceData = static_cast<zClass_SequenceDataPartial *>(std::calloc(1, twoEntryBytes));
    if (sequenceData == nullptr) {
        return 3;
    }
    sequenceData->entryCount = 2;
    sequenceData->entries[0].node = &first;
    sequenceData->entries[0].triggerTime = 1.0f;
    sequenceData->entries[1].node = &second;
    sequenceData->entries[1].triggerTime = 3.0f;
    parent.classData = sequenceData;

    if (zClass_Sequence::gwSequenceAddChild(&parent, &inserted, 1, 2.0f) != 0) {
        std::free(parent.classData);
        free_zclass_type_lists_for_test();
        return 4;
    }

    sequenceData = static_cast<zClass_SequenceDataPartial *>(parent.classData);
    if (zClass_Sequence::SetActive(&parent, 7) != 0 ||
        zClass_Sequence::SetRepeat(&parent, 8) != 0 || zClass_Sequence::SetLoop(&parent, 9) != 0 ||
        zClass_Sequence::SetPause(&parent, 10) != 0) {
        std::free(parent.listB);
        std::free(inserted.listA);
        std::free(parent.classData);
        free_zclass_type_lists_for_test();
        return 5;
    }

    const bool ok =
        sequenceData->entryCount == 3 && sequenceData->isActive == 7 &&
        sequenceData->repeatAtBounds == 8 && sequenceData->wrapAtBounds == 9 &&
        sequenceData->isPaused == 10 && sequenceData->entries[0].node == &first &&
        sequenceData->entries[0].triggerTime == 1.0f &&
        sequenceData->entries[1].node == &inserted &&
        sequenceData->entries[1].triggerTime == 2.0f && sequenceData->entries[2].node == &second &&
        sequenceData->entries[2].triggerTime == 3.0f && parent.listCountB == 1 &&
        parent.listB[0] == &inserted && inserted.listCountA == 1 && inserted.listA[0] == &parent &&
        zClass_TypeList::Head(7) != nullptr && zClass_TypeList::Head(7)->node == &parent;

    std::free(parent.listB);
    std::free(inserted.listA);
    std::free(parent.classData);
    free_zclass_type_lists_for_test();
    return ok ? 0 : 6;
}

extern "C" int zclass_object3d_set_matrix_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial data{};
    data.flags = 0x08;
    zClass_NodePartial node{};
    node.classId = 5;
    node.classData = &data;

    float matrix[12] = {};
    for (int i = 0; i < 12; ++i) {
        matrix[i] = static_cast<float>(i + 1);
    }

    if (zClass_Object3D::gwObject3DSetMatrix(&node, matrix) != 0) {
        return 1;
    }

    bool copied = true;
    for (int i = 0; i < 12; ++i) {
        copied = copied && data.localMatrix[i] == matrix[i];
    }

    const bool ok = copied && (data.flags & 0x08) == 0 && (data.flags & 0x31) == 0x31 &&
                    (node.boundsFlags & 0x04) != 0 && (node.flags & 0x02000003) == 0x02000003 &&
                    zClass_TypeList::Head(7) != nullptr && zClass_TypeList::Head(7)->node == &node;

    for (zClass_TypeListLink *link = zClass_TypeList::Head(7); link != nullptr;) {
        zClass_TypeListLink *next = link->next;
        std::free(link);
        link = next;
    }
    zClass_TypeList::Head(7) = nullptr;
    zClass_TypeList::Tail(7) = nullptr;

    return ok ? 0 : 2;
}

extern "C" int zclass_node_free_and_deferred_work_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[2]{};
    g_zClass_NodeArray = slots;
    g_zClass_ActiveNodeCount = 2;
    g_zClass_NodeFreeHeadIndex = 0x123456;

    zClass_NodePartial *node = &slots[1].node;
    node->classId = 5;
    node->listA = static_cast<zClass_NodePartial **>(std::malloc(sizeof(zClass_NodePartial *)));
    node->listB = static_cast<zClass_NodePartial **>(std::malloc(sizeof(zClass_NodePartial *)));
    node->classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    slots[1].freeTag = 0xab000000;

    if (zClass_Class::FreeNodeToFreeList(node) != 0 || node->listA != nullptr ||
        node->listB != nullptr || node->classData != nullptr || node->classId != 0 ||
        g_zClass_ActiveNodeCount != 1 || g_zClass_NodeFreeHeadIndex != 1 ||
        (slots[1].freeTag & 0xff000000) != 0xab000000 ||
        (slots[1].freeTag & 0x00ffffff) != 0x123456) {
        return 1;
    }

    zClass_NodePartial blocked{};
    blocked.listCountA = 1;
    if (zClass_Class::FreeNodeToFreeList(&blocked) != 1) {
        return 2;
    }

    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 1;
    slots[0].node.classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    slots[0].node.classId = 5;
    zClass_TypeListLink *pending =
        static_cast<zClass_TypeListLink *>(std::calloc(1, sizeof(zClass_TypeListLink)));
    pending->node = &slots[0].node;
    g_zClass_NodeList_PendingFreeHead = pending;
    g_zClass_TypeList_LiveLinkCount = 1;

    zClass_NodeList::ProcessPendingFrees();
    if (g_zClass_NodeList_PendingFreeHead != nullptr || slots[0].node.classData != nullptr ||
        g_zClass_TypeList_FreeLinkHead != pending || g_zClass_ActiveNodeCount != 0) {
        return 3;
    }
    zClass_TypeList::FreeAll();

    g_zClass_DeferredProcessingEnabled = 0;
    if (zClass::ProcessDeferredWork() != 1) {
        return 4;
    }
    g_zClass_DeferredProcessingEnabled = 1;
    zClass_TypeList::PendingRemovalDirty(3) = 1;
    if (zClass::ProcessDeferredWork() != 0 || zClass_TypeList::PendingRemovalDirty(3) != 0) {
        return 5;
    }

    return zClass_Class::FreeNodeToFreeList(nullptr) == 5 ? 0 : 6;
}

extern "C" int zclass_alloc_node_from_free_list_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[2]{};
    slots[1].freeTag = 0x00ffffff;
    slots[1].node.flags = 0xffffffff;
    slots[1].damageHandler = &slots[0];
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 1;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodePartial *node = zClass_Class::AllocNodeFromFreeList();
    if (node != &slots[1].node || g_zClass_NodeFreeHeadIndex != -1 ||
        g_zClass_ActiveNodeCount != 1 || zClass_TypeList::Head(6) == nullptr ||
        zClass_TypeList::Head(6)->node != node) {
        return 1;
    }
    if (node->flags != 0x0108001c || node->callbackPriority != 1 || node->gridCol != -1 ||
        node->gridRow != -1 || node->nodeType != 0xff ||
        std::strcmp(node->name, "Default_node_name") != 0 || slots[1].damageHandler != nullptr) {
        return 2;
    }

    zClass_List::DeleteNodeFromLists(node);
    zClass::ProcessDeferredWork();
    zClass_Class::FreeNodeToFreeList(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Class::AllocNodeFromFreeList() == nullptr ? 0 : 3;
}

extern "C" int zclass_node_flag16_flag17_smoke() {
    zClass_NodePartial node{};

    if (zClass_Class::gwNodeSetFlag16(&node, 1) != 0 || (node.flags & 0x10000) == 0) {
        return 1;
    }
    if (zClass_Class::gwNodeSetFlag16(&node, 0) != 0 || (node.flags & 0x10000) != 0) {
        return 2;
    }
    if (zClass_Class::gwNodeSetFlag17(&node, 1) != 0 || (node.flags & 0x20000) == 0) {
        return 3;
    }
    if (zClass_Class::gwNodeSetFlag17(&node, 0) != 0 || (node.flags & 0x20000) != 0) {
        return 4;
    }

    return zClass_Class::gwNodeSetFlag16(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeSetFlag17(nullptr, 1) == 5
               ? 0
               : 5;
}

extern "C" int zclass_node_metadata_accessors_smoke() {
    zClass_NodePartial node{};
    node.userDataOrDiRef = 0x12345678;
    node.nodeType = 0xab;

    std::uint32_t userData = 0;
    std::int32_t value = 0;
    if (zClass_Class::gwNodeGetUserData(&node, &userData) != 0 || userData != 0x12345678) {
        return 1;
    }
    if (zClass_Class::gwNodeGetNodeType(&node, &value) != 0 || value != 0xab) {
        return 2;
    }
    if (zClass_Class::gwNodeSetNodeType(&node, 0x135) != 0 || node.nodeType != 0x35) {
        return 3;
    }
    if (zClass_Class::gwNodeSetName(&node, "short_name") != 0 ||
        std::strcmp(node.name, "short_name") != 0 ||
        zClass_Class::gwNodeGetName(&node) != node.name) {
        return 4;
    }

    std::memset(node.name, 'Z', sizeof(node.name));
    const char *longName = "abcdefghijklmnopqrstuvwxyz0123456789LONG";
    if (zClass_Class::gwNodeSetName(&node, longName) != 0 ||
        std::strncmp(node.name, longName, 0x22) != 0 || node.name[0x22] != 'Z' ||
        node.name[0x23] != '\0') {
        return 5;
    }

    return zClass_Class::gwNodeGetUserData(nullptr, &userData) == 5 &&
                   zClass_Class::gwNodeGetNodeType(nullptr, &value) == 5 &&
                   zClass_Class::gwNodeSetNodeType(nullptr, 0) == 5 &&
                   zClass_Class::gwNodeSetName(nullptr, "name") == 5 &&
                   zClass_Class::gwNodeGetName(nullptr) == nullptr
               ? 0
               : 6;
}

extern "C" int zclass_copy_node_display_instance_smoke() {
    reset_zclass_type_lists_for_test();

    zVec3 verts[1] = {{1.0f, 2.0f, 3.0f}};
    zDiPartial displayInstance = {};
    displayInstance.mode = 0;
    displayInstance.vertCount = 1;
    displayInstance.verts = verts;

    zClass_NodeFreeListSlot sourceSlot = {};
    zClass_NodeFreeListSlot destSlot = {};
    sourceSlot.node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&displayInstance));

    g_zClass_CopyNodeCloneDiMode = 0;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 0;
    if (zClass_cls_util::CopyNodeDisplayInstance(&sourceSlot.node, &destSlot.node) != 0 ||
        destSlot.node.userDataOrDiRef != sourceSlot.node.userDataOrDiRef ||
        displayInstance.refCount != 1) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    std::memset(&destSlot, 0, sizeof(destSlot));
    displayInstance.refCount = 0;

    g_zClass_CopyNodeCloneDiMode = 1;
    g_zClass_CopyNodeDiArg0 = 1;
    g_zClass_CopyNodeDiArg1 = 1;
    if (zClass_cls_util::CopyNodeDisplayInstance(&sourceSlot.node, &destSlot.node) != 0 ||
        destSlot.node.userDataOrDiRef != sourceSlot.node.userDataOrDiRef ||
        displayInstance.refCount != 1) {
        free_zclass_type_lists_for_test();
        return 2;
    }

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    std::memset(&destSlot, 0, sizeof(destSlot));
    displayInstance.refCount = 0;
    displayInstance.flags = 0x04;

    zDiPartial clonePool[1] = {};
    clonePool[0].nextFreeIndex = -1;
    g_zModel_DiPoolBase = clonePool;
    g_zModel_DiPoolFreeHeadIndex = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zClass_CopyNodeCloneDiMode = 1;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 1;
    if (zClass_cls_util::CopyNodeDisplayInstance(&sourceSlot.node, &destSlot.node) != 0 ||
        destSlot.node.userDataOrDiRef !=
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&clonePool[0])) ||
        clonePool[0].refCount != 1 || clonePool[0].verts == verts) {
        zDi::FreeContents(&clonePool[0]);
        free_zclass_type_lists_for_test();
        return 3;
    }

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    zDi::FreeContents(&clonePool[0]);
    g_zModel_DiPoolBase = nullptr;
    free_zclass_type_lists_for_test();
    g_zClass_CopyNodeCloneDiMode = 0;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 0;
    return 0;
}

extern "C" int zclass_copy_node_base_data_smoke() {
    reset_zclass_type_lists_for_test();

    zVec3 verts[1] = {{-1.0f, 2.0f, 5.0f}};
    zDiPartial displayInstance = {};
    displayInstance.mode = 0;
    displayInstance.vertCount = 1;
    displayInstance.verts = verts;

    zClass_NodeFreeListSlot sourceSlot = {};
    zClass_NodeFreeListSlot destSlot = {};
    std::strcpy(sourceSlot.node.name, "copy_source");
    sourceSlot.node.flags = 0x70000000 | 0x00800000 | 0x00030000 | 0x000000fc;
    sourceSlot.node.auxFlags = 0x76543210;
    sourceSlot.node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&displayInstance));
    sourceSlot.node.callbackContext = &sourceSlot.node;
    sourceSlot.node.callbackPriority = 2;
    sourceSlot.node.actionCallback = &sourceSlot.node;
    sourceSlot.node.nodeType = 0x5a;

    destSlot.node.classId = 5;
    destSlot.node.flags = 0x01000000;
    g_zClass_CopyNodeCloneDiMode = 0;
    g_zClass_CopyNodeDiArg0 = 0;
    g_zClass_CopyNodeDiArg1 = 0;

    if (zClass_cls_util::CopyNodeBaseData(&sourceSlot.node, &destSlot.node) != 0) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    const std::int32_t expectedCopiedFlags = 0x70000000 | 0x00800000 | 0x00030000 | 0x000000fc;
    const bool ok =
        std::strcmp(destSlot.node.name, "copy_source") == 0 &&
        (destSlot.node.flags & expectedCopiedFlags) == expectedCopiedFlags &&
        (destSlot.node.flags & 0x01000000) == 0 && (destSlot.node.flags & 0x203) == 0x203 &&
        destSlot.node.auxFlags == sourceSlot.node.auxFlags &&
        destSlot.node.userDataOrDiRef == sourceSlot.node.userDataOrDiRef &&
        displayInstance.refCount == 1 && destSlot.node.callbackContext == nullptr &&
        destSlot.node.callbackPriority == 2 && destSlot.node.actionCallback == &sourceSlot.node &&
        destSlot.node.nodeType == 0x5a && zClass_TypeList::Head(2) != nullptr &&
        zClass_TypeList::Head(2)->node == &destSlot.node && zClass_TypeList::Head(7) != nullptr &&
        zClass_TypeList::Head(7)->node == &destSlot.node;

    zClass_Class::gwNodeSetDisplayInstance(&destSlot.node, nullptr);
    free_zclass_type_lists_for_test();
    return ok ? 0 : 2;
}

extern "C" int zclass_copy_node_unimplemented_stubs_smoke() {
    zClass_NodePartial node = {};
    return zClass_cls_util::CopyLightNode_Unimplemented(&node) == nullptr &&
                   zClass_cls_util::CopySoundNode_Unimplemented(&node) == nullptr &&
                   zClass_cls_util::CopyAnimateNode_Unimplemented(&node) == nullptr &&
                   zClass_cls_util::CopySequenceNode_Unimplemented(&node) == nullptr &&
                   zClass_cls_util::CopySwitchNode_Stub(&node) == nullptr
               ? 0
               : 1;
}

extern "C" int zclass_copy_object3d_and_lod_smoke() {
    reset_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot objectPool[1] = {};
    objectPool[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = objectPool;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;

    zClass_NodeFreeListSlot sourceObjectSlot = {};
    zClass_Object3DDataPartial sourceObjectData = {};
    std::strcpy(sourceObjectSlot.node.name, "source_object");
    sourceObjectSlot.node.classId = 5;
    sourceObjectSlot.node.classData = &sourceObjectData;
    sourceObjectData.alphaScale = 0.75f;
    sourceObjectData.flags = 0;
    sourceObjectData.rotation = {4.0f, 5.0f, 6.0f};
    sourceObjectData.scale = {7.0f, 8.0f, 9.0f};
    sourceObjectData.localMatrix[9] = 1.0f;
    sourceObjectData.localMatrix[10] = 2.0f;
    sourceObjectData.localMatrix[11] = 3.0f;

    zClass_NodePartial *const objectCopy =
        zClass_cls_util::CopyObject3DNode(&sourceObjectSlot.node);
    if (objectCopy != &objectPool[0].node || objectCopy->classId != 5 ||
        std::strcmp(objectCopy->name, "source_object") != 0) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    zClass_Object3DDataPartial *const objectCopyData =
        static_cast<zClass_Object3DDataPartial *>(objectCopy->classData);
    if (objectCopyData->alphaScale != 0.75f || objectCopyData->localMatrix[9] != 1.0f ||
        objectCopyData->localMatrix[10] != 2.0f || objectCopyData->localMatrix[11] != 3.0f ||
        objectCopyData->rotation.x != 4.0f || objectCopyData->rotation.y != 5.0f ||
        objectCopyData->rotation.z != 6.0f || objectCopyData->scale.x != 7.0f ||
        objectCopyData->scale.y != 8.0f || objectCopyData->scale.z != 9.0f) {
        std::free(objectCopyData);
        free_zclass_type_lists_for_test();
        return 2;
    }
    std::free(objectCopyData);
    free_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot lodPool[1] = {};
    lodPool[0].freeTag = 0x00ffffff;
    g_zClass_NodeArray = lodPool;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    reset_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot sourceLodSlot = {};
    zClass_NodePartial rangeNode = {};
    zClass_LodDataPartial sourceLodData = {};
    sourceLodSlot.node.classId = 6;
    sourceLodSlot.node.classData = &sourceLodData;
    sourceLodData.computeOwnDistance = 0;
    sourceLodData.nearRangeSq = 25.0f;
    sourceLodData.nearRange = 5.0f;
    sourceLodData.farRangeSq = 100.0f;
    sourceLodData.fadeWidth = {1.0f, 2.0f, 3.0f};
    sourceLodData.fadeAmount = {4.0f, 5.0f, 6.0f};
    sourceLodData.fadeEndScale = {7.0f, 8.0f, 9.0f};
    sourceLodData.fogFadeWidth = 10.0f;
    sourceLodData.fogFadeAmount = 11.0f;
    sourceLodData.fogStartDist = 12.0f;
    sourceLodData.vertexShadingAmount = 13.0f;
    sourceLodData.active = 0x44;
    sourceLodData.rangeNode = &rangeNode;
    sourceLodData.rangeSq = 16.0f;

    zClass_NodePartial *const lodCopy = zClass_cls_util::CopyLodNode(&sourceLodSlot.node);
    if (lodCopy != &lodPool[0].node || lodCopy->classId != 6) {
        free_zclass_type_lists_for_test();
        return 3;
    }

    zClass_LodDataPartial *const lodCopyData =
        static_cast<zClass_LodDataPartial *>(lodCopy->classData);
    const bool lodOk = lodCopyData->computeOwnDistance == 0 && lodCopyData->nearRangeSq == 25.0f &&
                       lodCopyData->nearRange == 5.0f && lodCopyData->farRangeSq == 100.0f &&
                       lodCopyData->fadeWidth.y == 2.0f && lodCopyData->fadeAmount.z == 6.0f &&
                       lodCopyData->fadeEndScale.x == 7.0f && lodCopyData->fogFadeWidth == 10.0f &&
                       lodCopyData->fogFadeAmount == 11.0f && lodCopyData->fogStartDist == 12.0f &&
                       lodCopyData->vertexShadingAmount == 13.0f && lodCopyData->active == 0x44 &&
                       lodCopyData->rangeNode == &rangeNode && lodCopyData->rangeSq == 16.0f;

    std::free(lodCopyData);
    g_zClass_NodeArray = nullptr;
    free_zclass_type_lists_for_test();
    return lodOk ? 0 : 4;
}

extern "C" int zclass_copy_node_dispatch_and_wrappers_smoke() {
    zClass_NodePartial sharedNode = {};
    sharedNode.flags = 0x04000000;
    g_zClass_CopyNodeCloneDiMode = 0x11;
    g_zClass_CopyNodeDiArg0 = 0x22;
    g_zClass_CopyNodeDiArg1 = 0x33;

    if (zClass_cls_util::CopyNodeDispatch(nullptr) != nullptr ||
        zClass_cls_util::CopyNodeDispatch(&sharedNode) != &sharedNode) {
        return 1;
    }

    if (zClass_cls_util::CopyNodeWithCloneOptions(&sharedNode, 0x44, 0x55) != &sharedNode ||
        g_zClass_CopyNodeCloneDiMode != 0x11 || g_zClass_CopyNodeDiArg0 != 0x22 ||
        g_zClass_CopyNodeDiArg1 != 0x33) {
        return 2;
    }

    if (zClass_cls_util::CopyNode(&sharedNode, 0x66, 0x77, 0x88) != &sharedNode ||
        g_zClass_CopyNodeCloneDiMode != 0x11 || g_zClass_CopyNodeDiArg0 != 0x22 ||
        g_zClass_CopyNodeDiArg1 != 0x33) {
        return 3;
    }

    zClass_NodePartial worldNode = {};
    worldNode.classId = 2;
    zClass_NodePartial unknownNode = {};
    unknownNode.classId = 99;
    return zClass_cls_util::CopyNodeDispatch(&worldNode) == nullptr &&
                   zClass_cls_util::CopyNodeDispatch(&unknownNode) == nullptr &&
                   zClass_cls_util::CopyNodeWithCloneOptions(nullptr, 1, 2) == nullptr &&
                   zClass_cls_util::CopyNode(nullptr, 1, 2, 3) == nullptr
               ? 0
               : 4;
}

extern "C" int zclass_node_action_callback_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial node{};
    node.callbackPriority = 3;
    if (zClass_Class::gwNodeSetActionCallback(&node, &node) != 0 || node.actionCallback != &node ||
        zClass_TypeList::Head(3) == nullptr || zClass_TypeList::Head(3)->node != &node) {
        return 1;
    }
    if (zClass_Class::gwNodeSetActionCallback(&node, nullptr) != 0 ||
        node.actionCallback != nullptr || zClass_TypeList::PendingRemovalDirty(3) == 0) {
        return 2;
    }
    zClass::ProcessDeferredWork();
    if (zClass_TypeList::Head(3) != nullptr) {
        return 3;
    }

    zClass_NodePartial first{};
    zClass_NodePartial second{};
    first.callbackPriority = 4;
    second.callbackPriority = 4;
    if (zClass_Class::gwNodeSetActionCallback(&first, &first) != 0 ||
        zClass_Class::gwNodeSetActionCallbackTail(&second, &second) != 0 ||
        zClass_TypeList::Head(4)->node != &first || zClass_TypeList::Tail(4)->node != &second) {
        return 4;
    }

    zClass_List::DeleteNodeFromLists(&first);
    zClass_List::DeleteNodeFromLists(&second);
    zClass::ProcessDeferredWork();
    zClass_TypeList::FreeAll();

    zClass_NodePartial invalid{};
    invalid.callbackPriority = 6;
    return zClass_Class::gwNodeSetActionCallback(nullptr, &node) == 5 &&
                   zClass_Class::gwNodeSetActionCallbackTail(nullptr, &node) == 5 &&
                   zClass_Class::gwNodeSetActionCallback(&invalid, &invalid) == 1
               ? 0
               : 5;
}

extern "C" int zclass_node_priority_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial node{};
    node.callbackPriority = 1;
    if (zClass_Class::gwNodeSetActionCallback(&node, &node) != 0 ||
        zClass_Class::gwNodeSetPriority(&node, 2) != 0 || node.callbackPriority != 2 ||
        zClass_TypeList::PendingRemovalDirty(1) == 0 || zClass_TypeList::Head(2) == nullptr ||
        zClass_TypeList::Head(2)->node != &node) {
        return 1;
    }

    zClass::ProcessDeferredWork();
    if (zClass_TypeList::Head(1) != nullptr || zClass_TypeList::Head(2) == nullptr) {
        return 2;
    }

    zClass_Class::gwNodeSetPriority(&node, 9);
    if (node.callbackPriority != 9 || zClass_TypeList::PendingRemovalDirty(2) == 0) {
        return 3;
    }
    zClass::ProcessDeferredWork();
    zClass_TypeList::FreeAll();

    return zClass_Class::gwNodeSetPriority(nullptr, 0) == 5 ? 0 : 4;
}

extern "C" int zclass_node_pick_flag_accessors_smoke() {
    zClass_NodePartial node{};
    std::int32_t value = 0;

    if (zClass_Class::gwNodeSetCellPickable(&node, 1) != 0 ||
        zClass_Class::gwNodeGetCellPickable(&node, &value) != 0 || value != 1) {
        return 1;
    }
    if (zClass_Class::gwNodeSetCellPickable(&node, 0) != 0 ||
        zClass_Class::gwNodeGetCellPickable(&node, &value) != 0 || value != 0) {
        return 2;
    }
    if (zClass_Class::gwNodeSetRaycastable(&node, 1) != 0 ||
        zClass_Class::gwNodeGetRaycastable(&node, &value) != 0 || value != 1) {
        return 3;
    }
    if (zClass_Class::gwNodeSetRaycastable(&node, 0) != 0 ||
        zClass_Class::gwNodeGetRaycastable(&node, &value) != 0 || value != 0) {
        return 4;
    }
    if (zClass_Class::gwNodeSetPickable(&node, 1) != 0 ||
        zClass_Class::gwNodeGetPickable(&node, &value) != 0 || value != 1) {
        return 5;
    }
    if (zClass_Class::gwNodeSetPickable(&node, 0) != 0 ||
        zClass_Class::gwNodeGetPickable(&node, &value) != 0 || value != 0) {
        return 6;
    }

    return zClass_Class::gwNodeSetCellPickable(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeGetCellPickable(nullptr, &value) == 5 &&
                   zClass_Class::gwNodeSetRaycastable(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeGetRaycastable(nullptr, &value) == 5 &&
                   zClass_Class::gwNodeSetPickable(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeGetPickable(nullptr, &value) == 5
               ? 0
               : 7;
}

extern "C" int zclass_node_extra_flag_setters_smoke() {
    zClass_NodePartial node{};
    zClass_NodePartial child{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *children[] = {&child};
    zClass_NodePartial *grandchildren[] = {&grandchild};

    if (zClass_Class::gwNodeSetHasHitCallback(&node, 1) != 0 || (node.flags & 0x40) == 0) {
        return 1;
    }
    if (zClass_Class::gwNodeSetHasHitCallback(&node, 0) != 0 || (node.flags & 0x40) != 0) {
        return 2;
    }
    if (zClass_Class::gwNodeSetBypassFarClip(&node, 1) != 0 || (node.flags & 0x80) == 0) {
        return 3;
    }
    if (zClass_Class::gwNodeSetBypassFarClip(&node, 0) != 0 || (node.flags & 0x80) != 0) {
        return 4;
    }

    node.flags = 0x01000000;
    if (zClass_Class::gwNodeClearVariantGate(&node, 1) != 0 || (node.flags & 0x01000000) == 0) {
        return 5;
    }
    if (zClass_Class::gwNodeClearVariantGate(&node, 0) != 0 || (node.flags & 0x01000000) != 0) {
        return 6;
    }

    node.listCountB = 1;
    node.listB = children;
    child.listCountB = 1;
    child.listB = grandchildren;
    zClass_Node::PropagateExtraFlagsRecursive(&node, 0x22);
    if (node.auxFlags != 0x22 || child.auxFlags != 0x22 || grandchild.auxFlags != 0x22) {
        return 7;
    }

    zClass_Node::SetContextRecursive(&node, &node, 0x200000);
    if ((node.flags & 0x200000) == 0 || (child.flags & 0x200000) == 0 ||
        (grandchild.flags & 0x200000) == 0 || node.callbackContext != &node ||
        child.callbackContext != &node || grandchild.callbackContext != &node) {
        return 8;
    }

    return zClass_Class::gwNodeSetHasHitCallback(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeSetBypassFarClip(nullptr, 1) == 5 &&
                   zClass_Class::gwNodeClearVariantGate(nullptr, 0) == 5
               ? 0
               : 9;
}

extern "C" int zclass_node_vertex_alpha_and_root_smoke() {
    zClass_NodePartial node{};

    if (zClass_Class::gwNodeSetVertexAlphaOverride(&node, 1) != 0 ||
        (node.flags & 0x00800000) == 0) {
        return 1;
    }
    if (zClass_Class::gwNodeSetVertexAlphaOverride(&node, 0) != 0 ||
        (node.flags & 0x00800000) != 0) {
        return 2;
    }

    zClass_NodePartial root{};
    zClass_NodePartial mid{};
    zClass_NodePartial child{};
    zClass_NodePartial *midParentList[] = {&root};
    zClass_NodePartial *childParentList[] = {&mid};
    mid.listCountA = 1;
    mid.listA = midParentList;
    child.listCountA = 1;
    child.listA = childParentList;

    if (zClass_Class::gwNodeGetRoot(&child) != &root) {
        return 3;
    }
    if (zClass_Class::gwNodeGetRoot(nullptr) != nullptr) {
        return 4;
    }

    zClass_NodePartial secondParent{};
    zClass_NodePartial *multiParentList[] = {&root, &secondParent};
    child.listCountA = 2;
    child.listA = multiParentList;
    if (zClass_Class::gwNodeGetRoot(&child) != nullptr) {
        return 5;
    }

    return zClass_Class::gwNodeSetVertexAlphaOverride(nullptr, 1) == 5 ? 0 : 6;
}

extern "C" int zclass_node_world_child_smoke() {
    zClass_NodePartial world{};
    zClass_NodePartial mid{};
    zClass_NodePartial child{};
    zClass_NodePartial *midParents[] = {&world};
    zClass_NodePartial *childParents[] = {&mid};
    world.classId = 2;
    mid.listCountA = 1;
    mid.listA = midParents;
    child.listCountA = 1;
    child.listA = childParents;

    if (zClass_Class::gwNodeGetWorldChild(&child) != &world ||
        zClass_Class::gwNodeGetWorldChild(&world) != nullptr ||
        zClass_Class::gwNodeGetWorldChild(nullptr) != nullptr) {
        return 1;
    }

    zClass_NodePartial otherParent{};
    zClass_NodePartial *multiParents[] = {&world, &otherParent};
    child.listCountA = 2;
    child.listA = multiParents;
    return zClass_Class::gwNodeGetWorldChild(&child) == nullptr ? 0 : 2;
}

extern "C" int zclass_gwnode_get_world_position_smoke() {
    zVec3 outPosition{9.0f, 9.0f, 9.0f};
    if (gwNode::GetWorldPosition(nullptr, &outPosition) != 1) {
        return 1;
    }

    zClass_Object3DDataPartial data{};
    zClass_NodePartial node{};
    node.classId = 5;
    node.flags = 0x00080000;
    node.classData = &data;
    data.cachedWorldMatrix[9] = 1.0f;
    data.cachedWorldMatrix[10] = 2.0f;
    data.cachedWorldMatrix[11] = 3.0f;
    if (gwNode::GetWorldPosition(&node, &outPosition) != 0 || outPosition.x != 1.0f ||
        outPosition.y != 2.0f || outPosition.z != 3.0f) {
        return 2;
    }

    data.flags = 0x20;
    data.localMatrix[0] = 1.0f;
    data.localMatrix[4] = 1.0f;
    data.localMatrix[8] = 1.0f;
    data.localMatrix[9] = 4.0f;
    data.localMatrix[10] = 5.0f;
    data.localMatrix[11] = 6.0f;
    if (gwNode::GetWorldPosition(&node, &outPosition) != 0) {
        return 3;
    }

    if (outPosition.x != 4.0f || outPosition.y != 5.0f || outPosition.z != 6.0f ||
        (data.flags & 0x20) != 0) {
        return 4;
    }

    zVec3 point{};
    if (gwNode::TransformPoint(nullptr, &point) != 1 ||
        gwNode::TransformPoint(&node, &point) != 0 || point.x != 4.0f || point.y != 5.0f ||
        point.z != 6.0f) {
        return 5;
    }

    data.flags = 0x20;
    data.localMatrix[9] = 10.0f;
    data.localMatrix[10] = 20.0f;
    data.localMatrix[11] = 30.0f;
    point = {1.0f, 2.0f, 3.0f};
    if (gwNode::TransformPoint(&node, &point) != 0 || point.x != 11.0f || point.y != 22.0f ||
        point.z != 33.0f) {
        return 6;
    }

    return 0;
}

extern "C" int zclass_world_add_child_at_grid_smoke() {
    zWorldAreaPartial row0[2]{};
    zWorldAreaPartial row1[2]{};
    row0[0].areaFlags = 1;
    row0[0].cellMinX = 0.0f;
    row0[0].cellMinZ = 100.0f;
    row0[1].areaFlags = 1;
    row0[1].cellMinX = 50.0f;
    row0[1].cellMinZ = 100.0f;
    row1[0].areaFlags = 1;
    row1[0].cellMinX = 0.0f;
    row1[0].cellMinZ = 50.0f;
    row1[1].areaFlags = 1;
    row1[1].cellMinX = 50.0f;
    row1[1].cellMinZ = 50.0f;
    zWorldAreaPartial *rows[] = {row0, row1};

    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 100.0f;
    worldData.worldSizeX = 100.0f;
    worldData.worldSizeZ = -100.0f;
    worldData.worldMaxX = 100.0f;
    worldData.worldMaxZ = 0.0f;
    worldData.areaCellSizeX = 50.0f;
    worldData.areaCellSizeZ = -50.0f;
    worldData.areaInvSizeX = 0.02f;
    worldData.areaInvSizeZ = -0.02f;
    worldData.partitionInclusionTolX = 2.0f;
    worldData.partitionInclusionTolZ = 2.0f;
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 2;
    worldData.areaGridRows = rows;

    zClass_NodePartial world{};
    world.flags = 1;
    world.classData = &worldData;

    zClass_NodePartial wholeWorldChild{};
    if (zClass_World::AddChildAtGrid(&world, &wholeWorldChild) != 0 || world.listCountB != 1 ||
        world.listB[0] != &wholeWorldChild || wholeWorldChild.gridCol != -1 ||
        wholeWorldChild.gridRow != -1) {
        return 1;
    }

    zClass_NodePartial outsideChild{};
    outsideChild.flags = 0x80;
    if (zClass_World::AddChildAtGrid(&world, &outsideChild) != 0 || world.listCountB != 2 ||
        world.listB[1] != &outsideChild || outsideChild.gridCol != -1 ||
        outsideChild.gridRow != -1) {
        return 2;
    }

    std::int32_t classData = 0;
    zClass_NodePartial bboxChild{};
    bboxChild.flags = 0x100;
    bboxChild.classData = &classData;
    bboxChild.cachedBounds[0] = 10.0f;
    bboxChild.cachedBounds[1] = 0.0f;
    bboxChild.cachedBounds[2] = 70.0f;
    bboxChild.cachedBounds[3] = 20.0f;
    bboxChild.cachedBounds[4] = 1.0f;
    bboxChild.cachedBounds[5] = 80.0f;
    if (zClass_World::AddChildAtGrid(&world, &bboxChild) != 0 || row0[0].childCount != 1 ||
        row0[0].childList[0] != &bboxChild || bboxChild.gridCol != 0 || bboxChild.gridRow != 0) {
        return 3;
    }

    std::free(row0[0].childList);
    std::free(wholeWorldChild.listA);
    std::free(outsideChild.listA);
    std::free(bboxChild.listA);
    std::free(world.listB);
    return 0;
}

extern "C" int zclass_world_remove_light_sound_smoke() {
    zClass_WorldDataPartial addWorldData{};
    zClass_NodePartial addWorld{};
    addWorld.classData = &addWorldData;

    zClass_NodePartial addedLight{};
    zClass_LightDataPartial addedLightData{};
    addedLight.classData = &addedLightData;
    if (zClass_World::AddLight(&addWorld, &addedLight) != 0 || addWorldData.lightCount != 1 ||
        addWorldData.lightNodes[0] != &addedLight ||
        addWorldData.lightDataList[0] != &addedLightData ||
        addedLightData.attachedWorldCount != 1 || addedLightData.attachedWorlds[0] != &addWorld) {
        return 10;
    }
    if (zClass_World::RemoveLight(&addWorld, &addedLight) != 0 || addWorldData.lightCount != 0 ||
        addedLightData.attachedWorldCount != 0) {
        return 11;
    }
    std::free(addWorldData.lightNodes);
    std::free(addWorldData.lightDataList);
    std::free(addedLightData.attachedWorlds);

    zClass_NodePartial addedSound{};
    zClass_SoundDataPartial addedSoundData{};
    addedSound.classData = &addedSoundData;
    if (zClass_World::AddSound(&addWorld, &addedSound) != 0 || addWorldData.soundCount != 1 ||
        addWorldData.soundNodes[0] != &addedSound ||
        addWorldData.soundDataList[0] != &addedSoundData ||
        addedSoundData.attachedWorldCount != 1 || addedSoundData.attachedWorlds[0] != &addWorld) {
        return 12;
    }
    if (zClass_World::RemoveSound(&addWorld, &addedSound) != 0 || addWorldData.soundCount != 0 ||
        addedSoundData.attachedWorldCount != 0) {
        return 13;
    }
    std::free(addWorldData.soundNodes);
    std::free(addWorldData.soundDataList);
    std::free(addedSoundData.attachedWorlds);

    zClass_WorldDataPartial worldData{};
    zClass_NodePartial world{};
    world.classData = &worldData;

    zClass_NodePartial otherWorld{};
    zClass_NodePartial extraWorld{};

    zClass_NodePartial lightA{};
    zClass_NodePartial lightB{};
    zClass_NodePartial lightC{};
    zClass_LightDataPartial lightDataA{};
    zClass_LightDataPartial lightDataB{};
    zClass_LightDataPartial lightDataC{};
    zClass_NodePartial *lightNodes[] = {&lightA, &lightB, &lightC};
    zClass_LightDataPartial *lightData[] = {&lightDataA, &lightDataB, &lightDataC};
    zClass_NodePartial *lightWorlds[] = {&otherWorld, &world, &extraWorld};
    lightDataB.attachedWorldCount = 3;
    lightDataB.attachedWorlds = lightWorlds;
    worldData.lightCount = 3;
    worldData.lightNodes = lightNodes;
    worldData.lightDataList = lightData;

    if (zClass_World::RemoveLight(&world, &lightB) != 0 || worldData.lightCount != 2 ||
        worldData.lightNodes[0] != &lightA || worldData.lightNodes[1] != &lightC ||
        worldData.lightDataList[0] != &lightDataA || worldData.lightDataList[1] != &lightDataC ||
        lightDataB.attachedWorldCount != 2 || lightDataB.attachedWorlds[0] != &otherWorld ||
        lightDataB.attachedWorlds[1] != &extraWorld) {
        return 1;
    }

    zClass_NodePartial soundA{};
    zClass_NodePartial soundB{};
    zClass_NodePartial soundC{};
    zClass_SoundDataPartial soundDataA{};
    zClass_SoundDataPartial soundDataB{};
    zClass_SoundDataPartial soundDataC{};
    zClass_NodePartial *soundNodes[] = {&soundA, &soundB, &soundC};
    zClass_SoundDataPartial *soundData[] = {&soundDataA, &soundDataB, &soundDataC};
    zClass_NodePartial *soundWorlds[] = {&world, &otherWorld};
    soundDataA.attachedWorldCount = 2;
    soundDataA.attachedWorlds = soundWorlds;
    worldData.soundCount = 3;
    worldData.soundNodes = soundNodes;
    worldData.soundDataList = soundData;

    if (zClass_World::RemoveSound(&world, &soundA) != 0 || worldData.soundCount != 2 ||
        worldData.soundNodes[0] != &soundB || worldData.soundNodes[1] != &soundC ||
        worldData.soundDataList[0] != &soundDataB || worldData.soundDataList[1] != &soundDataC ||
        soundDataA.attachedWorldCount != 1 || soundDataA.attachedWorlds[0] != &otherWorld) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_child_generic_link_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodePartial parent{};
    zClass_NodePartial otherParent{};
    zClass_NodePartial child{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *childChildren[] = {&grandchild};
    child.flags = 0x00080000;
    grandchild.flags = 0x00080000;
    child.listCountB = 1;
    child.listB = childChildren;

    if (zClass_Class::AddChildGeneric(&parent, &child) != 0 || parent.listCountB != 1 ||
        parent.listB[0] != &child || child.listCountA != 1 || child.listA[0] != &parent ||
        (parent.boundsFlags & 2) == 0 || (parent.flags & 3) != 3 ||
        zClass_TypeList::Head(7) == nullptr || zClass_TypeList::Head(7)->node != &parent) {
        return 1;
    }

    if (zClass_Class::AddChildGeneric(&otherParent, &child) != 0 || child.listCountA != 2 ||
        child.listA[1] != &otherParent || (child.flags & 0x00080000) != 0 ||
        (grandchild.flags & 0x00080000) != 0) {
        return 2;
    }

    int validatedClassData = 0;
    zClass_NodePartial validatedParent{};
    zClass_NodePartial validatedChild{};
    validatedParent.classData = &validatedClassData;
    if (zClass_Class::AddChildValidated(&validatedParent, &validatedChild) != 0 ||
        validatedParent.listCountB != 1 || validatedParent.listB[0] != &validatedChild ||
        validatedChild.listCountA != 1 || validatedChild.listA[0] != &validatedParent) {
        return 3;
    }
    if (zClass_Class::AddChildValidated(nullptr, &validatedChild) != 5 ||
        zClass_Class::AddChildValidated(&validatedParent, nullptr) != 5) {
        return 4;
    }
    validatedParent.classData = nullptr;
    if (zClass_Class::AddChildValidated(&validatedParent, &validatedChild) != 5) {
        return 5;
    }

    zClass_NodePartial dispatchParent{};
    zClass_NodePartial dispatchChild{};
    dispatchParent.classId = 3;
    if (zClass_Class::AddChild(&dispatchParent, &dispatchChild) != 0 ||
        dispatchParent.listCountB != 1 || dispatchParent.listB[0] != &dispatchChild ||
        dispatchChild.listCountA != 1 || dispatchChild.listA[0] != &dispatchParent) {
        return 6;
    }

    zClass_NodePartial animateParent{};
    zClass_NodePartial animateChild{};
    animateParent.classId = 8;
    if (zClass_Class::AddChild(&animateParent, &animateChild) != 0 ||
        animateParent.listCountB != 1 || animateChild.listCountA != 1 ||
        zClass_Animate::AddChild(nullptr, &animateChild) != 5 ||
        zClass_Animate::AddChild(&animateParent, nullptr) != 5) {
        return 7;
    }

    zClass_NodePartial sequenceParent{};
    zClass_NodePartial sequenceChild{};
    sequenceParent.classId = 7;
    if (zClass_Class::AddChild(&sequenceParent, &sequenceChild) != 1 ||
        zClass_Class::AddChild(nullptr, &sequenceChild) != 5 ||
        zClass_Class::AddChild(&sequenceParent, nullptr) != 5) {
        return 8;
    }

    std::free(parent.listB);
    std::free(otherParent.listB);
    std::free(child.listA);
    std::free(validatedParent.listB);
    std::free(validatedChild.listA);
    std::free(dispatchParent.listB);
    std::free(dispatchChild.listA);
    std::free(animateParent.listB);
    std::free(animateChild.listA);
    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();
    return 0;
}

extern "C" int zclass_child_generic_remove_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodePartial parent{};
    zClass_NodePartial remainingParent{};
    zClass_NodePartial child{};
    zClass_NodePartial sibling{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *parentChildren[] = {&child, &sibling};
    zClass_NodePartial *childParents[] = {&parent, &remainingParent};
    zClass_NodePartial *childChildren[] = {&grandchild};
    parent.flags = 0x00080000;
    parent.listCountB = 2;
    parent.listB = parentChildren;
    child.listCountA = 2;
    child.listA = childParents;
    child.listCountB = 1;
    child.listB = childChildren;

    if (zClass_Class::RemoveChildGeneric(&parent, &child) != 0 || parent.listCountB != 1 ||
        parent.listB[0] != &sibling || child.listCountA != 1 ||
        child.listA[0] != &remainingParent || (child.flags & 0x00080000) == 0 ||
        (grandchild.flags & 0x00080000) == 0 || (parent.boundsFlags & 2) == 0 ||
        (parent.flags & 3) != 3) {
        return 1;
    }

    zClass_NodePartial missing{};
    if (zClass_Class::RemoveChildGeneric(&parent, &missing) != 0 || parent.listCountB != 1 ||
        missing.listCountA != 0) {
        return 2;
    }

    zClass_NodePartial checkedParent{};
    zClass_NodePartial checkedChild{};
    zClass_NodePartial *checkedChildren[] = {&checkedChild};
    zClass_NodePartial *checkedParents[] = {&checkedParent};
    checkedParent.listCountB = 1;
    checkedParent.listB = checkedChildren;
    checkedChild.listCountA = 1;
    checkedChild.listA = checkedParents;
    if (zClass::RemoveChildChecked(&checkedParent, &checkedChild) != 0 ||
        checkedParent.listCountB != 0 || checkedChild.listCountA != 0) {
        return 3;
    }
    if (zClass::RemoveChildChecked(nullptr, &checkedChild) != 5 ||
        zClass::RemoveChildChecked(&checkedParent, nullptr) != 5) {
        return 4;
    }

    int validatedClassData = 0;
    zClass_NodePartial validatedParent{};
    zClass_NodePartial validatedChild{};
    zClass_NodePartial *validatedChildren[] = {&validatedChild};
    zClass_NodePartial *validatedParents[] = {&validatedParent};
    validatedParent.classData = &validatedClassData;
    validatedParent.listCountB = 1;
    validatedParent.listB = validatedChildren;
    validatedChild.listCountA = 1;
    validatedChild.listA = validatedParents;
    if (zClass_Class::RemoveChildValidated(&validatedParent, &validatedChild) != 0 ||
        validatedParent.listCountB != 0 || validatedChild.listCountA != 0) {
        return 5;
    }
    if (zClass_Class::RemoveChildValidated(nullptr, &validatedChild) != 5 ||
        zClass_Class::RemoveChildValidated(&validatedParent, nullptr) != 5) {
        return 6;
    }
    validatedParent.classData = nullptr;
    if (zClass_Class::RemoveChildValidated(&validatedParent, &validatedChild) != 5) {
        return 7;
    }

    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();
    return 0;
}

extern "C" int zclass_object3d_child_wrappers_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_Object3DDataPartial objectData{};
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    parent.classData = &objectData;

    if (zClass_Object3D::gwObject3DAddChild(&parent, &child) != 0 || parent.listCountB != 1 ||
        child.listCountA != 1) {
        return 1;
    }
    if (zClass_Object3D::RemoveChild(&parent, &child) != 0 || parent.listCountB != 0 ||
        child.listCountA != 0) {
        return 2;
    }

    std::free(parent.listB);
    std::free(child.listA);
    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();

    zClass_NodePartial noData{};
    return zClass_Object3D::gwObject3DAddChild(nullptr, &child) == 5 &&
                   zClass_Object3D::gwObject3DAddChild(&parent, nullptr) == 5 &&
                   zClass_Object3D::gwObject3DAddChild(&noData, &child) == 5 &&
                   zClass_Object3D::RemoveChild(nullptr, &child) == 5 &&
                   zClass_Object3D::RemoveChild(&parent, nullptr) == 5 &&
                   zClass_Object3D::RemoveChild(&noData, &child) == 5
               ? 0
               : 3;
}

extern "C" int zclass_delete_node_from_lists_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodePartial node{};
    node.classId = 1;
    node.callbackPriority = 2;
    node.actionCallback = &node;

    zClass_TypeList::Insert(8, &node);
    zClass_TypeList::Insert(7, &node);
    zClass_TypeList::Insert(2, &node);
    zClass_TypeList::Insert(6, &node);

    if (zClass_List::DeleteNodeFromLists(&node) != 0 ||
        zClass_TypeList::PendingRemovalDirty(8) == 0 ||
        zClass_TypeList::PendingRemovalDirty(7) == 0 ||
        zClass_TypeList::PendingRemovalDirty(2) == 0 ||
        zClass_TypeList::PendingRemovalDirty(6) == 0) {
        return 1;
    }
    if (zClass_TypeList::Head(8)->pendingRemove != 1 ||
        zClass_TypeList::Head(7)->pendingRemove != 1 ||
        zClass_TypeList::Head(2)->pendingRemove != 1 ||
        zClass_TypeList::Head(6)->pendingRemove != 1) {
        return 2;
    }

    zClass::ProcessDeferredWork();
    if (zClass_TypeList::Head(8) != nullptr || zClass_TypeList::Head(7) != nullptr ||
        zClass_TypeList::Head(2) != nullptr || zClass_TypeList::Head(6) != nullptr ||
        (node.flags & 1) != 0) {
        return 3;
    }

    zClass_TypeList::FreeAll();
    return 0;
}

extern "C" int zclass_find_by_name_and_filtered_iter_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial alpha{};
    zClass_NodePartial beta{};
    zClass_NodePartial alpine{};
    std::strcpy(alpha.name, "alpha");
    std::strcpy(beta.name, "beta");
    std::strcpy(alpine.name, "alpine");

    zClass_TypeList::Insert(5, &alpha);
    zClass_TypeList::Insert(5, &beta);
    zClass_TypeList::Insert(5, &alpine);

    if (zClass::FindByTypeAndName(5, "alpha") != &alpha ||
        zClass::FindByTypeAndName(5, "missing") != nullptr) {
        return 1;
    }

    if (zClass_Class::gwNodeFindNextByName("beta", 5) != nullptr ||
        zClass_Class::gwNodeFindNextByName(nullptr, 5) != &beta ||
        zClass_Class::gwNodeFindNextByName(nullptr, 5) != nullptr) {
        return 2;
    }

    if (zClass::FindNextByTypePrefix("al", 5) != nullptr ||
        zClass::FindNextByTypePrefix(nullptr, 5) != &alpine ||
        zClass::FindNextByTypePrefix(nullptr, 5) != &alpha ||
        zClass::FindNextByTypePrefix(nullptr, 5) != nullptr) {
        return 3;
    }

    zClass_NodePartial root{};
    zClass_NodePartial firstChild{};
    zClass_NodePartial secondChild{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *rootChildren[] = {&firstChild, &secondChild};
    zClass_NodePartial *secondChildren[] = {&grandchild};
    std::strcpy(root.name, "root");
    std::strcpy(firstChild.name, "shared");
    std::strcpy(secondChild.name, "shared");
    std::strcpy(grandchild.name, "deep");
    root.listCountB = 2;
    root.listB = rootChildren;
    secondChild.listCountB = 1;
    secondChild.listB = secondChildren;
    if (zClass_Class::FindSubNodeByName(&root, "root") != &root ||
        zClass_Class::FindSubNodeByName(&root, "shared") != &secondChild ||
        zClass_Class::FindSubNodeByName(&root, "deep") != &grandchild ||
        zClass_Class::FindSubNodeByName(&root, "missing") != nullptr ||
        zClass_Class::FindSubNodeByName(nullptr, "root") != nullptr) {
        return 4;
    }

    zClass_TypeList::PrintBucket(5);
    zClass_TypeList::MarkPendingRemoval(5, &alpha);
    zClass_TypeList::MarkPendingRemoval(5, &beta);
    zClass_TypeList::MarkPendingRemoval(5, &alpine);
    zClass::ProcessDeferredWork();
    zClass_TypeList::FreeAll();
    return 0;
}

extern "C" int zclass_node_predicate_helpers_smoke() {
    zDiPartial di{1, 0};
    zClass_NodePartial node{};
    node.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&di);
    if (zClass_Node::HasRenderableDiPredicate(&node) != 1) {
        return 1;
    }
    di.flags = 0x10;
    if (zClass_Node::HasRenderableDiPredicate(&node) != 0) {
        return 2;
    }
    node.userDataOrDiRef = 0;
    if (zClass_Node::HasRenderableDiPredicate(&node) != 0) {
        return 3;
    }

    zDiPartial flagTarget{0, 0x12};
    zDi::SetFlagBit0(&flagTarget, 1);
    if (flagTarget.flags != 0x13) {
        return 4;
    }
    zDi::SetFlagBit0(&flagTarget, 0);
    if (flagTarget.flags != 0x12) {
        return 5;
    }
    zDi::SetFlagBit0(nullptr, 1);
    if (zModel_Material_SetFlagBit9(nullptr, 1) != 0) {
        return 6;
    }

    zDiEntryPartial variantEntries[3] = {};
    variantEntries[1].variantTagInitialized = 1;
    variantEntries[1].variantTag = 0x44;
    zDiPartial variantDi{};
    variantDi.entryCount = 3;
    variantDi.entries = variantEntries;
    zDi::SetVariantTagIfUnset(&variantDi, 0x21);
    if (variantEntries[0].variantTagInitialized != 1 || variantEntries[0].variantTag != 0x21 ||
        variantEntries[1].variantTagInitialized != 1 || variantEntries[1].variantTag != 0x44 ||
        variantEntries[2].variantTagInitialized != 1 || variantEntries[2].variantTag != 0x21) {
        return 7;
    }
    zDi::SetVariantTagIfUnset(nullptr, 0x22);

    g_Variant_FilterEnabled = 0;
    if (VariantTag::CurrentAllowsId(0x33) != 1) {
        return 31;
    }
    g_Variant_FilterEnabled = 1;
    g_Variant_CurrentTag.count = 0;
    if (VariantTag::CurrentAllowsId(0x33) != 1 || VariantTag::CurrentAllowsId(0xff) != 1) {
        return 32;
    }
    g_Variant_CurrentTag.count = 2;
    g_Variant_CurrentTag.tags[0] = 0x44;
    g_Variant_CurrentTag.tags[1] = 0x55;
    if (VariantTag::CurrentAllowsId(0x44) != 1 || VariantTag::CurrentAllowsId(0x33) != 0) {
        return 33;
    }
    g_Variant_CurrentTag.tags[1] = 0xff;
    if (VariantTag::CurrentAllowsId(0x33) != 1) {
        return 34;
    }

    zVec3 boundsVerts[2] = {{-1.0f, 2.0f, -3.0f}, {5.0f, -6.0f, 7.0f}};
    zDiPartial boundsDi{};
    boundsDi.mode = 0;
    boundsDi.vertCount = 2;
    boundsDi.verts = boundsVerts;
    zBoundsMinMaxPartial bounds{};
    zDi::RebuildBounds(&boundsDi, &bounds);
    if (bounds.min.x != -1.0f || bounds.min.y != -6.0f || bounds.min.z != -3.0f ||
        bounds.max.x != 5.0f || bounds.max.y != 2.0f || bounds.max.z != 7.0f ||
        boundsDi.bboxCenter.x != 2.0f || boundsDi.bboxCenter.y != -2.0f ||
        boundsDi.bboxCenter.z != 2.0f || boundsDi.bboxRadius <= 0.0f) {
        return 61;
    }

    zClass_NodeFreeListSlot displaySlot{};
    displaySlot.node.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&boundsDi);
    if (zClass_Class::gwNodeUpdateDisplayInstance(&displaySlot.node) != 0 ||
        (displaySlot.node.flags & 0x200) == 0) {
        return 62;
    }
    zBoundsMinMaxPartial *displayBounds =
        reinterpret_cast<zBoundsMinMaxPartial *>(displaySlot.unknown_8c);
    if (displayBounds->min.x != -1.0f || displayBounds->max.z != 7.0f) {
        return 63;
    }
    displaySlot.node.userDataOrDiRef = 0;
    if (zClass_Class::gwNodeUpdateDisplayInstance(&displaySlot.node) != 0 ||
        (displaySlot.node.flags & 0x200) != 0 ||
        zClass_Class::gwNodeUpdateDisplayInstance(nullptr) != 5) {
        return 64;
    }

    zClass_NodePartial bboxNode{};
    std::int32_t bboxClassData = 0;
    bboxNode.classData = &bboxClassData;
    bboxNode.flags = 0x100;
    bboxNode.cachedBounds[0] = 1.0f;
    bboxNode.cachedBounds[1] = 2.0f;
    bboxNode.cachedBounds[2] = 3.0f;
    bboxNode.cachedBounds[3] = 4.0f;
    bboxNode.cachedBounds[4] = 5.0f;
    bboxNode.cachedBounds[5] = 6.0f;
    zBBoxCorners bboxCorners{};
    if (zClass_Class::gwNodeGetWorldBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 1.0f || bboxCorners.values[1] != 2.0f ||
        bboxCorners.values[2] != 6.0f || bboxCorners.values[6] != 4.0f ||
        bboxCorners.values[7] != 2.0f || bboxCorners.values[8] != 3.0f ||
        bboxCorners.values[21] != 1.0f || bboxCorners.values[22] != 5.0f ||
        bboxCorners.values[23] != 3.0f) {
        return 65;
    }
    bboxNode.flags = 0;
    if (zClass_Class::gwNodeGetWorldBBoxCorners(&bboxNode, &bboxCorners) != 1) {
        return 66;
    }
    bboxNode.classData = nullptr;
    bboxNode.flags = 0x100;
    if (zClass_Class::gwNodeGetWorldBBoxCorners(&bboxNode, &bboxCorners) != 5 ||
        zClass_Class::gwNodeGetWorldBBoxCorners(nullptr, &bboxCorners) != 5) {
        return 67;
    }

    zClass_Object3DDataPartial objectData{};
    objectData.localMatrix[0] = 1.0f;
    objectData.localMatrix[4] = 1.0f;
    objectData.localMatrix[8] = 1.0f;
    objectData.localMatrix[9] = 10.0f;
    objectData.localMatrix[10] = 20.0f;
    objectData.localMatrix[11] = 30.0f;
    bboxNode.classId = 5;
    bboxNode.classData = &objectData;
    if (zClass_Class::gwNodeGetWorldBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 11.0f || bboxCorners.values[1] != 22.0f ||
        bboxCorners.values[2] != 36.0f) {
        return 68;
    }

    static std::int32_t viewMatrixFlags[1];
    static float *viewMatrixSlots[1];
    static zMat4x3 viewCurrentMatrix;
    zMath::g_currentMatrixIdentityFlagSlot = &viewMatrixFlags[0];
    zMath::g_currentMatrixPtrSlot = &viewMatrixSlots[0];

    viewMatrixFlags[0] = 0;
    viewMatrixSlots[0] = nullptr;
    objectData.flags = 0;
    if (zClass_Class::gwNodeGetViewBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 11.0f || bboxCorners.values[1] != 22.0f ||
        bboxCorners.values[2] != 36.0f) {
        return 681;
    }

    viewCurrentMatrix = {};
    viewCurrentMatrix.xx = 1.0f;
    viewCurrentMatrix.yy = 1.0f;
    viewCurrentMatrix.zz = 1.0f;
    viewCurrentMatrix.posX = 100.0f;
    viewCurrentMatrix.posY = -10.0f;
    viewCurrentMatrix.posZ = 1.0f;
    viewMatrixFlags[0] = 0;
    viewMatrixSlots[0] = reinterpret_cast<float *>(&viewCurrentMatrix);
    objectData.flags = 0x08;
    if (zClass_Class::gwNodeGetViewBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 101.0f || bboxCorners.values[1] != -8.0f ||
        bboxCorners.values[2] != 7.0f) {
        return 682;
    }

    objectData.flags = 0;
    if (zClass_Class::gwNodeGetViewBBoxCorners(&bboxNode, &bboxCorners) != 0 ||
        bboxCorners.values[0] != 111.0f || bboxCorners.values[1] != 12.0f ||
        bboxCorners.values[2] != 37.0f) {
        return 683;
    }

    viewMatrixFlags[0] = 1;
    viewMatrixSlots[0] = reinterpret_cast<float *>(&viewCurrentMatrix);
    bboxNode.classId = 3;
    bboxNode.classData = &bboxClassData;
    if (zClass_Class::gwNodeGetViewBBoxCorners(&bboxNode, &bboxCorners) != 3 ||
        bboxCorners.values[0] != 1.0f || bboxCorners.values[1] != 2.0f ||
        bboxCorners.values[2] != 6.0f) {
        return 684;
    }
    bboxNode.classId = 5;
    bboxNode.classData = &objectData;

    zClass_NodeFreeListSlot parentSlot{};
    zClass_NodePartial childA{};
    zClass_NodePartial childB{};
    zClass_NodePartial childIgnored{};
    std::int32_t childClassData = 0;
    childA.classData = &childClassData;
    childB.classData = &childClassData;
    childIgnored.classData = &childClassData;
    childA.flags = 0x100;
    childB.flags = 0x100;
    childA.cachedBounds[0] = 0.0f;
    childA.cachedBounds[1] = 1.0f;
    childA.cachedBounds[2] = 2.0f;
    childA.cachedBounds[3] = 3.0f;
    childA.cachedBounds[4] = 4.0f;
    childA.cachedBounds[5] = 5.0f;
    childB.cachedBounds[0] = -2.0f;
    childB.cachedBounds[1] = 6.0f;
    childB.cachedBounds[2] = -1.0f;
    childB.cachedBounds[3] = 10.0f;
    childB.cachedBounds[4] = 7.0f;
    childB.cachedBounds[5] = 8.0f;
    childIgnored.cachedBounds[0] = -100.0f;
    childIgnored.cachedBounds[1] = -100.0f;
    childIgnored.cachedBounds[2] = -100.0f;
    childIgnored.cachedBounds[3] = 100.0f;
    childIgnored.cachedBounds[4] = 100.0f;
    childIgnored.cachedBounds[5] = 100.0f;
    zClass_NodePartial *children[] = {&childIgnored, &childA, &childB};
    parentSlot.node.listCountB = 3;
    parentSlot.node.listB = children;
    parentSlot.node.flags = 0x400;
    if (zClass_Class::gwNodeComputeChildBBox(&parentSlot.node) != 0 ||
        (parentSlot.node.flags & 0x400) == 0) {
        return 69;
    }
    zBBox3f *childBounds =
        reinterpret_cast<zBBox3f *>(reinterpret_cast<unsigned char *>(&parentSlot.node) + 0xa4);
    if (childBounds->minX != -2.0f || childBounds->minY != 1.0f || childBounds->minZ != -1.0f ||
        childBounds->maxX != 10.0f || childBounds->maxY != 7.0f || childBounds->maxZ != 8.0f) {
        return 70;
    }
    parentSlot.node.classId = 2;
    if (zClass_Class::gwNodeComputeChildBBox(&parentSlot.node) != 0 ||
        (parentSlot.node.flags & 0x400) != 0 ||
        zClass_Class::gwNodeComputeChildBBox(nullptr) != 5) {
        return 71;
    }

    zWorldAreaPartial row0[2]{};
    zWorldAreaPartial row1[2]{};
    row0[0].cellMinX = 0.0f;
    row0[0].cellMinZ = 100.0f;
    row0[1].cellMinX = 50.0f;
    row0[1].cellMinZ = 100.0f;
    row1[0].cellMinX = 0.0f;
    row1[0].cellMinZ = 50.0f;
    row1[1].cellMinX = 50.0f;
    row1[1].cellMinZ = 50.0f;
    zWorldAreaPartial *rows[] = {row0, row1};
    zClass_WorldDataPartial worldData{};
    worldData.originX = 0.0f;
    worldData.originZ = 100.0f;
    worldData.worldMaxX = 100.0f;
    worldData.worldMaxZ = 0.0f;
    worldData.areaCellSizeX = 50.0f;
    worldData.areaCellSizeZ = -50.0f;
    worldData.areaInvSizeX = 0.02f;
    worldData.areaInvSizeZ = -0.02f;
    worldData.partitionInclusionTolX = 2.0f;
    worldData.partitionInclusionTolZ = 2.0f;
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 2;
    worldData.areaGridRows = rows;
    zClass_NodePartial worldNode{};
    worldNode.classData = &worldData;
    std::int32_t gridCol = -99;
    std::int32_t gridRow = -99;
    if (zClass_World::WorldRectToGridIndex(&worldNode, &gridCol, 10.0f, 20.0f, 70.0f, 80.0f,
                                           &gridRow) != 0 ||
        gridCol != 0 || gridRow != 0) {
        return 72;
    }
    if (zClass_World::WorldRectToGridIndex(&worldNode, &gridCol, 60.0f, 70.0f, 20.0f, 40.0f,
                                           &gridRow) != 0 ||
        gridCol != 1 || gridRow != 1) {
        return 73;
    }
    if (zClass_World::WorldRectToGridIndex(&worldNode, &gridCol, -5.0f, 10.0f, 70.0f, 80.0f,
                                           &gridRow) != 0 ||
        gridCol != -1 || gridRow != -1) {
        return 74;
    }
    if (zClass_World::WorldRectToGridIndex(&worldNode, &gridCol, 40.0f, 60.0f, 70.0f, 80.0f,
                                           &gridRow) != 0 ||
        gridCol != -1 || gridRow != -1) {
        return 75;
    }
    worldNode.flags = 1;
    row0[0].areaFlags = 0;
    if (zClass_World::EnsureGridCellDisplayPosition(&worldNode, 0, 0) != 0 ||
        worldData.pendingAreaUpdateCount != 1 || worldData.pendingAreaUpdateCapacity != 1 ||
        worldData.pendingAreaUpdates[0] != &row0[0] || (row0[0].areaFlags & 1) == 0 ||
        (worldNode.flags & 3) != 3 || (worldData.flags & 0x10) == 0) {
        return 76;
    }
    if (zClass_World::EnsureGridCellDisplayPosition(&worldNode, 0, 0) != 0 ||
        worldData.pendingAreaUpdateCount != 1) {
        return 77;
    }

    zClass_NodePartial gridChild{};
    if (zClass_World::AddChildToGridCell(&worldNode, &gridChild, 1, 0) != 0 ||
        row0[1].childCount != 1 || row0[1].childList[0] != &gridChild || gridChild.gridCol != 1 ||
        gridChild.gridRow != 0 || gridChild.listCountA != 1 || gridChild.listA[0] != &worldNode ||
        worldData.pendingAreaUpdateCount != 2 || (row0[1].areaFlags & 1) == 0) {
        return 80;
    }
    if (zClass_World::RemoveChildAtGrid(&worldNode, &gridChild) != 0 || row0[1].childCount != 0 ||
        gridChild.gridCol != -1 || gridChild.gridRow != -1 || gridChild.listCountA != 0) {
        return 81;
    }
    std::free(row0[1].childList);
    row0[1].childList = nullptr;
    std::free(gridChild.listA);
    gridChild.listA = nullptr;

    zClass_NodePartial fallbackChild{};
    if (zClass_World::AddChildToGridCell(&worldNode, &fallbackChild, -1, -1) != 0 ||
        worldNode.listCountB != 1 || worldNode.listB[0] != &fallbackChild ||
        fallbackChild.gridCol != -1 || fallbackChild.gridRow != -1 ||
        fallbackChild.listCountA != 1 || fallbackChild.listA[0] != &worldNode) {
        return 82;
    }
    std::free(worldNode.listB);
    worldNode.listB = nullptr;
    worldNode.listCountB = 0;
    std::free(fallbackChild.listA);
    fallbackChild.listA = nullptr;
    fallbackChild.listCountA = 0;

    zClass_NodeFreeListSlot recalcSlot{};
    zClass_NodePartial recalcParent{};
    zClass_NodePartial *recalcParents[] = {&recalcParent};
    recalcParent.flags = 1;
    recalcSlot.node.classData = &childClassData;
    recalcSlot.node.flags = 0x200 | 0x400;
    recalcSlot.node.listCountA = 1;
    recalcSlot.node.listA = recalcParents;
    zBBox3f *primaryBounds = reinterpret_cast<zBBox3f *>(recalcSlot.unknown_8c);
    zBBox3f *secondaryBounds = reinterpret_cast<zBBox3f *>(recalcSlot.unknown_8c + 0x18);
    *primaryBounds = {0.0f, 2.0f, -1.0f, 4.0f, 8.0f, 6.0f};
    *secondaryBounds = {-3.0f, 3.0f, -2.0f, 2.0f, 9.0f, 5.0f};
    if (zClass_Class::gwNodeRecalcBBox(&recalcSlot.node) != 0 ||
        (recalcSlot.node.flags & 0x100) == 0 || recalcSlot.node.cachedBounds[0] != -3.0f ||
        recalcSlot.node.cachedBounds[1] != 2.0f || recalcSlot.node.cachedBounds[2] != -2.0f ||
        recalcSlot.node.cachedBounds[3] != 4.0f || recalcSlot.node.cachedBounds[4] != 9.0f ||
        recalcSlot.node.cachedBounds[5] != 6.0f || (recalcSlot.node.boundsFlags & 0x04) == 0 ||
        (recalcParent.boundsFlags & 0x02) == 0 || (recalcParent.flags & 0x02) == 0) {
        return 83;
    }
    recalcSlot.node.flags = 0x100;
    recalcSlot.node.listCountA = 0;
    if (zClass_Class::gwNodeRecalcBBox(&recalcSlot.node) != 0 ||
        (recalcSlot.node.flags & 0x100) != 0 || zClass_Class::gwNodeRecalcBBox(nullptr) != 5) {
        return 84;
    }

    zClass_NodePartial areaChildA{};
    zClass_NodePartial areaChildB{};
    areaChildA.classData = &childClassData;
    areaChildB.classData = &childClassData;
    areaChildA.flags = 0x100;
    areaChildB.flags = 0x100;
    areaChildA.cachedBounds[1] = -2.0f;
    areaChildA.cachedBounds[4] = 3.0f;
    areaChildB.cachedBounds[1] = -5.0f;
    areaChildB.cachedBounds[4] = 10.0f;
    zClass_NodePartial *areaChildren[] = {&areaChildA, &areaChildB};
    zWorldAreaPartial rebuildArea{};
    rebuildArea.areaFlags = 1;
    rebuildArea.bbox[0] = 0.0f;
    rebuildArea.bbox[2] = 0.0f;
    rebuildArea.bbox[3] = 20.0f;
    rebuildArea.bbox[5] = 20.0f;
    rebuildArea.childCount = 2;
    rebuildArea.childList = areaChildren;
    if (zClass_World::RebuildAreaBounds(&worldData, &rebuildArea) != 0 ||
        (rebuildArea.areaFlags & 1) != 0 || (rebuildArea.areaFlags & 0x100) == 0 ||
        rebuildArea.bbox[1] != -5.0f || rebuildArea.bbox[4] != 10.0f ||
        rebuildArea.bboxCenter.y != 2.5f || rebuildArea.bboxRadius <= 0.0f) {
        return 85;
    }

    std::free(worldData.pendingAreaUpdates);
    worldData.pendingAreaUpdates = nullptr;
    if (zClass_World::EnsureGridCellDisplayPosition(nullptr, 0, 0) != 5) {
        return 78;
    }
    worldNode.classData = nullptr;
    if (zClass_World::EnsureGridCellDisplayPosition(&worldNode, 0, 0) != 5) {
        return 79;
    }

    zModel_MaterialPartial materialDirect{0xffff};
    if (zModel_Material_SetFlagBit9(&materialDirect, 0) != 1 ||
        (materialDirect.flags & 0x0200) != 0 || (materialDirect.flags & 0xfdff) != 0xfdff) {
        return 7;
    }

    zClass_NodePartial root{};
    zClass_NodePartial first{};
    zClass_NodePartial second{};
    zClass_NodePartial grandchild{};
    zClass_NodePartial *rootChildren[] = {&first, &second};
    zClass_NodePartial *secondChildren[] = {&grandchild};
    zDiPartial rootDi{0, 0};
    zDiPartial firstDi{0, 1};
    zDiPartial grandchildDi{0, 0x20};
    root.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&rootDi);
    first.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&firstDi);
    grandchild.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&grandchildDi);
    root.listCountB = 2;
    root.listB = rootChildren;
    second.listCountB = 1;
    second.listB = secondChildren;
    zClass_Node::AssignInt32ToDiRecursive(&root, 1);
    if ((rootDi.flags & 1) == 0 || (firstDi.flags & 1) == 0 || (grandchildDi.flags & 1) == 0) {
        return 6;
    }
    zClass_Node::AssignInt32ToDiRecursive(&root, 0);
    if ((rootDi.flags & 1) != 0 || (firstDi.flags & 1) != 0 || (grandchildDi.flags & 1) != 0) {
        return 8;
    }

    zModel_MaterialPartial rootMaterial{0x0100};
    zModel_MaterialPartial skippedMaterial{0};
    zModel_MaterialPartial childMaterial{0x0100};
    zDiEntryPartial rootEntries[2]{};
    zDiEntryPartial childEntries[1]{};
    zDiPartial materialRootDi{};
    zDiPartial materialChildDi{};
    rootEntries[0].material = &rootMaterial;
    rootEntries[1].material = &skippedMaterial;
    childEntries[0].material = &childMaterial;
    materialRootDi.entryCount = 2;
    materialRootDi.entries = rootEntries;
    materialChildDi.entryCount = 1;
    materialChildDi.entries = childEntries;
    root.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&materialRootDi);
    first.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&materialChildDi);
    grandchild.userDataOrDiRef = 0;
    zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive(&root, 1);
    if ((rootMaterial.flags & 0x0200) == 0 || (skippedMaterial.flags & 0x0200) != 0 ||
        (childMaterial.flags & 0x0200) == 0) {
        return 9;
    }
    zClass_Node::SetMaterialFlagBit9ForFlagBit0EntriesRecursive(&root, 0);
    if ((rootMaterial.flags & 0x0200) != 0 || (childMaterial.flags & 0x0200) != 0) {
        return 10;
    }

    zVid_Image::ReleaseIfNotDefault(&zVid_Image::g_zImage_DefaultImage);
    zImage_TexDirEntryPartial loadedEntry{};
    zImage_TexDirEntryPartial stoppedEntry{};
    zVidImagePartial *loadedImage =
        static_cast<zVidImagePartial *>(std::calloc(1, sizeof(zVidImagePartial)));
    if (loadedImage == nullptr) {
        return 11;
    }
    loadedEntry.image = loadedImage;
    loadedEntry.loadState = 1;
    loadedEntry.nextVariant = &stoppedEntry;
    stoppedEntry.image = &zVid_Image::g_zImage_DefaultImage;
    stoppedEntry.loadState = 2;
    zImage::InvalidateLoadedVariantChain(&loadedEntry);
    if (loadedEntry.image != nullptr || loadedEntry.loadState != 3 ||
        stoppedEntry.image != &zVid_Image::g_zImage_DefaultImage || stoppedEntry.loadState != 2) {
        return 12;
    }

    zImage_TexDirEntryPartial materialEntry{};
    zImage_TexDirEntryPartial frameEntry{};
    materialEntry.image = &zVid_Image::g_zImage_DefaultImage;
    materialEntry.loadState = 1;
    frameEntry.image = &zVid_Image::g_zImage_DefaultImage;
    frameEntry.loadState = 1;
    zImage_TexDirEntryPartial *frameTable[] = {&frameEntry};
    zModel_MaterialCyclePartial cycle{};
    cycle.frameCount = 1;
    cycle.frameTable = frameTable;
    zModel_MaterialPartial invalidateMaterial{};
    invalidateMaterial.flags = 0x0300;
    invalidateMaterial.currentTextureDirectoryEntry = &materialEntry;
    invalidateMaterial.cycle = &cycle;
    zDiEntryPartial invalidateEntries[1]{};
    zDiPartial invalidateDi{};
    invalidateEntries[0].material = &invalidateMaterial;
    invalidateDi.entryCount = 1;
    invalidateDi.entries = invalidateEntries;
    root.userDataOrDiRef = reinterpret_cast<std::uint32_t>(&invalidateDi);
    first.userDataOrDiRef = 0;
    zClass_Node::InvalidateFlagBit8MaterialImagesRecursive(&root);
    if (materialEntry.loadState != 3 || frameEntry.loadState != 3) {
        return 13;
    }

    grandchild.nodeType = 0x42;

    if (zClass::AnyNodeMatchesPredicateRecursive(&root, zclass_test_node_type_0x42) != 1) {
        return 14;
    }
    grandchild.nodeType = 0;
    return zClass::AnyNodeMatchesPredicateRecursive(&root, zclass_test_node_type_0x42) == 0 ? 0
                                                                                            : 15;
}

extern "C" int zclass_lifecycle_leaf_smoke() {
    g_zClass_NodeArraySize = 0;
    g_zClass_IsInitialized = 0;

    zClass::SetNodeArraySize(12);
    if (g_zClass_NodeArraySize != 12 || zClass::IsInitialized() != 0) {
        return 1;
    }

    zClass::SetNodeArraySize(99);
    if (g_zClass_NodeArraySize != 12) {
        return 2;
    }

    g_zClass_IsInitialized = 1;
    return zClass::IsInitialized() == 1 ? 0 : 3;
}

extern "C" int zclass_zbd_leaf_helpers_smoke() {
    std::strcpy(g_zClass_CurrentZbdPath, "maps\\level.zbd");
    if (zClass::ResetCurrentZbdPath() != 0 || g_zClass_CurrentZbdPath[0] != 0) {
        return 1;
    }

    zClass_NodeFreeListSlot slots[3]{};
    g_zClass_NodeArray = slots;
    slots[0].freeTag = 0x01000000;
    slots[1].freeTag = 0x00ffffff;
    if (GameZ_ZBD::NodePtrToIndex(nullptr) != -1 ||
        GameZ_ZBD::NodePtrToIndex(&slots[2].node) != 2 ||
        GameZ_ZBD::NodeIndexToPtr(-1) != nullptr ||
        GameZ_ZBD::NodeIndexToPtr(1) != &slots[1].node ||
        zClass::NodePtrToValidatedIndex(nullptr) != -1 ||
        zClass::NodePtrToValidatedIndex(&slots[0].node) != 0 ||
        zClass::NodePtrToValidatedIndex(&slots[1].node) != -1) {
        return 2;
    }

    return 0;
}

extern "C" int zclass_zbd_node_ref_list_indices_smoke() {
    zClass_NodeFreeListSlot slots[3]{};
    g_zClass_NodeArray = slots;

    zClass_NodePartial *refs[3] = {
        &slots[2].node,
        nullptr,
        &slots[0].node,
    };

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (GameZ_ZBD::WriteNodeRefListIndices(refs, 3, file) != 0 ||
        GameZ_ZBD::WriteNodeRefListIndices(refs, 0, file) != 0) {
        std::fclose(file);
        return 2;
    }

    std::rewind(file);
    std::int32_t indices[3] = {};
    if (std::fread(indices, sizeof(indices), 1, file) != 1 || indices[0] != 2 || indices[1] != -1 ||
        indices[2] != 0) {
        std::fclose(file);
        return 3;
    }

    std::rewind(file);
    zClass_NodePartial *readRefs[3] = {};
    if (GameZ_ZBD::ReadNodeRefListIndices(readRefs, 3, file) != 0 ||
        readRefs[0] != &slots[2].node || readRefs[1] != nullptr || readRefs[2] != &slots[0].node ||
        GameZ_ZBD::ReadNodeRefListIndices(readRefs, 0, file) != 0) {
        std::fclose(file);
        return 4;
    }

    std::fclose(file);
    std::free(g_GameZ_Zbd_NodeIndexScratch);
    g_GameZ_Zbd_NodeIndexScratch = nullptr;
    g_GameZ_Zbd_NodeIndexScratchCapacity = 0;
    return 0;
}

extern "C" int zclass_zbd_single_node_class_data_smoke() {
    zClass_NodeFreeListSlot slots[5]{};
    g_zClass_NodeArray = slots;

    zClass_CameraDataPartial cameraData{};
    cameraData.worldNode = &slots[2].node;
    cameraData.windowNode = &slots[0].node;
    cameraData.horizonNode = nullptr;
    cameraData.horizonXZNode = &slots[4].node;
    cameraData.cameraFlags = 0x12345678;

    zClass_NodePartial *cameraListA[1] = {&slots[1].node};
    zClass_NodePartial *cameraListB[1] = {&slots[3].node};
    zClass_NodePartial cameraNode{};
    std::strcpy(cameraNode.name, "camera_node");
    cameraNode.classId = 1;
    cameraNode.classData = &cameraData;
    cameraNode.listCountA = 1;
    cameraNode.listA = cameraListA;
    cameraNode.listCountB = 1;
    cameraNode.listB = cameraListB;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (GameZ_ZBD::WriteSingleNodeClassData(&cameraNode, file) != 1) {
        std::fclose(file);
        return 2;
    }

    if (std::fseek(file, 0, SEEK_END) != 0 ||
        std::ftell(file) != static_cast<long>(sizeof(zClass_CameraDataPartial) + 8)) {
        std::fclose(file);
        return 3;
    }

    std::rewind(file);
    std::int32_t cameraIndices[4] = {};
    if (std::fread(cameraIndices, sizeof(cameraIndices), 1, file) != 1 || cameraIndices[0] != 2 ||
        cameraIndices[1] != 0 || cameraIndices[2] != -1 || cameraIndices[3] != 4) {
        std::fclose(file);
        return 4;
    }

    std::int32_t cameraListIndices[2] = {};
    if (std::fseek(file, static_cast<long>(sizeof(zClass_CameraDataPartial)), SEEK_SET) != 0 ||
        std::fread(cameraListIndices, sizeof(cameraListIndices), 1, file) != 1 ||
        cameraListIndices[0] != 1 || cameraListIndices[1] != 3) {
        std::fclose(file);
        return 5;
    }

    std::fclose(file);

    zClass_NodePartial *lightNodes[1] = {&slots[4].node};
    zClass_NodePartial *soundNodes[2] = {nullptr, &slots[1].node};
    zClass_NodePartial *areaChildren[1] = {&slots[2].node};
    zWorldAreaPartial areas[2] = {};
    areas[0].areaFlags = 0x55aa;
    areas[0].childCount = 1;
    areas[0].childList = areaChildren;
    areas[1].areaFlags = 0x33cc;
    zWorldAreaPartial *rows[1] = {areas};

    zClass_WorldDataPartial worldData{};
    worldData.areaGridColCount = 2;
    worldData.areaGridRowCount = 1;
    worldData.areaGridRows = rows;
    worldData.lightCount = 1;
    worldData.lightNodes = lightNodes;
    worldData.soundCount = 2;
    worldData.soundNodes = soundNodes;

    zClass_NodePartial worldNode{};
    std::strcpy(worldNode.name, "world_node");
    worldNode.classId = 2;
    worldNode.classData = &worldData;

    file = std::tmpfile();
    if (file == nullptr) {
        return 6;
    }

    if (GameZ_ZBD::WriteSingleNodeClassData(&worldNode, file) != 1) {
        std::fclose(file);
        return 7;
    }

    const long worldNodeListOffset = static_cast<long>(sizeof(zClass_WorldDataPartial));
    const long firstAreaOffset = worldNodeListOffset + 12;
    const long firstAreaChildRefOffset =
        firstAreaOffset + static_cast<long>(sizeof(zWorldAreaPartial));
    const long expectedWorldBytes =
        firstAreaChildRefOffset + 4 + static_cast<long>(sizeof(zWorldAreaPartial));
    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedWorldBytes) {
        std::fclose(file);
        return 8;
    }

    std::int32_t worldIndices[3] = {};
    if (std::fseek(file, worldNodeListOffset, SEEK_SET) != 0 ||
        std::fread(worldIndices, sizeof(worldIndices), 1, file) != 1 || worldIndices[0] != 4 ||
        worldIndices[1] != -1 || worldIndices[2] != 1) {
        std::fclose(file);
        return 9;
    }

    std::int32_t areaChildIndex = 0;
    zWorldAreaPartial secondArea{};
    if (std::fseek(file, firstAreaChildRefOffset, SEEK_SET) != 0 ||
        std::fread(&areaChildIndex, sizeof(areaChildIndex), 1, file) != 1 || areaChildIndex != 2 ||
        std::fread(&secondArea, sizeof(secondArea), 1, file) != 1 ||
        secondArea.areaFlags != 0x33cc) {
        std::fclose(file);
        return 10;
    }

    std::fclose(file);
    std::free(g_GameZ_Zbd_NodeIndexScratch);
    g_GameZ_Zbd_NodeIndexScratch = nullptr;
    g_GameZ_Zbd_NodeIndexScratchCapacity = 0;
    return 0;
}

extern "C" int zclass_zbd_read_single_node_class_data_smoke() {
    zClass_NodeFreeListSlot slots[6]{};
    g_zClass_NodeArray = slots;

    reset_zclass_type_lists_for_test();

    zClass_CameraDataPartial cameraDisk{};
    cameraDisk.worldNode = reinterpret_cast<zClass_NodePartial *>(2);
    cameraDisk.windowNode = reinterpret_cast<zClass_NodePartial *>(-1);
    cameraDisk.horizonNode = reinterpret_cast<zClass_NodePartial *>(0);
    cameraDisk.horizonXZNode = reinterpret_cast<zClass_NodePartial *>(4);
    cameraDisk.nearClip = 0.25f;
    cameraDisk.farClip = 500.0f;
    cameraDisk.viewportWidth = 320.0f;
    cameraDisk.viewportHeight = 200.0f;
    cameraDisk.frustumWidth = 640.0f;
    cameraDisk.frustumHeight = 480.0f;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    const std::int32_t cameraListIndices[2] = {1, 3};
    if (std::fwrite(&cameraDisk, sizeof(cameraDisk), 1, file) != 1 ||
        std::fwrite(cameraListIndices, sizeof(cameraListIndices), 1, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 2;
    }
    std::rewind(file);

    zClass_NodePartial cameraNode{};
    std::strcpy(cameraNode.name, "camera_read");
    cameraNode.classId = 1;
    cameraNode.listCountA = 1;
    cameraNode.listCountB = 1;

    if (GameZ_ZBD::ReadSingleNodeClassData(&cameraNode, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 3;
    }
    std::fclose(file);

    zClass_CameraDataPartial *cameraData =
        static_cast<zClass_CameraDataPartial *>(cameraNode.classData);
    const float maxCameraFov = 1.39600003f;
    const bool cameraOk =
        cameraData != nullptr && cameraData->worldNode == &slots[2].node &&
        cameraData->windowNode == nullptr && cameraData->horizonNode == &slots[0].node &&
        cameraData->horizonXZNode == &slots[4].node && cameraData->nearClip == 0.25f &&
        cameraData->farClip == 500.0f && cameraData->viewportWidth == 320.0f &&
        cameraData->viewportHeight == 200.0f && cameraData->fovX == maxCameraFov &&
        cameraData->fovY == maxCameraFov && cameraData->frustumVectorsDirty == 1 &&
        cameraData->localFrustumNormalsDirty == 1 && cameraNode.listA != nullptr &&
        cameraNode.listA[0] == &slots[1].node && cameraNode.listB != nullptr &&
        cameraNode.listB[0] == &slots[3].node && zClass_TypeList::Head(8) != nullptr &&
        zClass_TypeList::Head(8)->node == &cameraNode;
    std::free(cameraNode.listA);
    std::free(cameraNode.listB);
    std::free(cameraData);
    cameraNode.listA = nullptr;
    cameraNode.listB = nullptr;
    cameraNode.classData = nullptr;
    free_zclass_type_lists_for_test();
    if (!cameraOk) {
        return 4;
    }

    reset_zclass_type_lists_for_test();

    zClass_WorldDataPartial worldDisk{};
    worldDisk.fogState = 2;
    worldDisk.ambientColor.red = 0.25f;
    worldDisk.ambientColor.green = 0.5f;
    worldDisk.ambientColor.blue = 0.75f;
    worldDisk.fogDistanceStart = 12.0f;
    worldDisk.fogDistanceEnd = 64.0f;
    worldDisk.fogHeightHigh = 18.0f;
    worldDisk.fogHeightLow = -4.0f;
    worldDisk.fogDensity = 0.125f;
    worldDisk.areaGridColCount = 2;
    worldDisk.areaGridRowCount = 1;
    worldDisk.lightCount = 1;
    worldDisk.soundCount = 1;

    zWorldAreaPartial firstArea{};
    firstArea.areaFlags = 0x22;
    firstArea.childCount = 1;
    zWorldAreaPartial secondArea{};
    secondArea.areaFlags = 0x44;

    const std::int32_t worldLightIndex = 1;
    const std::int32_t worldSoundIndex = 2;
    const std::int32_t areaChildIndex = 3;

    file = std::tmpfile();
    if (file == nullptr) {
        free_zclass_type_lists_for_test();
        return 5;
    }
    if (std::fwrite(&worldDisk, sizeof(worldDisk), 1, file) != 1 ||
        std::fwrite(&worldLightIndex, sizeof(worldLightIndex), 1, file) != 1 ||
        std::fwrite(&worldSoundIndex, sizeof(worldSoundIndex), 1, file) != 1 ||
        std::fwrite(&firstArea, sizeof(firstArea), 1, file) != 1 ||
        std::fwrite(&areaChildIndex, sizeof(areaChildIndex), 1, file) != 1 ||
        std::fwrite(&secondArea, sizeof(secondArea), 1, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 6;
    }
    std::rewind(file);

    zClass_NodePartial worldNode{};
    std::strcpy(worldNode.name, "world_read");
    worldNode.classId = 2;

    if (GameZ_ZBD::ReadSingleNodeClassData(&worldNode, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 7;
    }
    std::fclose(file);

    zClass_WorldDataPartial *worldData =
        static_cast<zClass_WorldDataPartial *>(worldNode.classData);
    const bool worldOk =
        worldData != nullptr && worldData->flags == 0 && worldData->pendingAreaUpdateCount == 0 &&
        worldData->pendingAreaUpdateCapacity == 0 && worldData->pendingAreaUpdates == nullptr &&
        worldData->lightNodes != nullptr && worldData->lightNodes[0] == &slots[1].node &&
        worldData->lightDataList != nullptr && worldData->soundNodes != nullptr &&
        worldData->soundNodes[0] == &slots[2].node && worldData->soundDataList != nullptr &&
        worldData->areaGridRows != nullptr && worldData->areaGridRows[0] != nullptr &&
        worldData->areaGridRows[0][0].areaFlags == 0x22 &&
        worldData->areaGridRows[0][0].childList != nullptr &&
        worldData->areaGridRows[0][0].childList[0] == &slots[3].node &&
        worldData->areaGridRows[0][1].areaFlags == 0x44 &&
        worldData->areaGridRows[0][1].childList == nullptr &&
        zClass_TypeList::Head(13) != nullptr && zClass_TypeList::Head(13)->node == &worldNode;

    if (worldData != nullptr) {
        std::free(worldData->lightNodes);
        std::free(worldData->lightDataList);
        std::free(worldData->soundNodes);
        std::free(worldData->soundDataList);
        if (worldData->areaGridRows != nullptr) {
            if (worldData->areaGridRows[0] != nullptr) {
                std::free(worldData->areaGridRows[0][0].childList);
                std::free(worldData->areaGridRows[0]);
            }
            std::free(worldData->areaGridRows);
        }
        std::free(worldData);
    }
    free_zclass_type_lists_for_test();
    return worldOk ? 0 : 8;
}

extern "C" int zclass_zbd_read_node_table_smoke() {
    reset_zclass_type_lists_for_test();

    zClass_NodeFreeListSlot diskSlots[3]{};
    std::strcpy(diskSlots[0].node.name, "world_table");
    diskSlots[0].node.classId = 2;
    diskSlots[0].node.userDataOrDiRef = static_cast<std::uint32_t>(-1);
    diskSlots[0].freeTag = 0x12000000u;
    std::strcpy(diskSlots[1].node.name, "light_table");
    diskSlots[1].node.classId = 9;
    diskSlots[1].node.userDataOrDiRef = static_cast<std::uint32_t>(-1);
    diskSlots[1].freeTag = 0x34000000u;
    std::strcpy(diskSlots[2].node.name, "sound_table");
    diskSlots[2].node.classId = 10;
    diskSlots[2].node.userDataOrDiRef = static_cast<std::uint32_t>(-1);
    diskSlots[2].freeTag = 0x56000000u;

    zClass_WorldDataPartial worldDisk{};
    worldDisk.lightCount = 1;
    worldDisk.soundCount = 1;
    worldDisk.fogState = 0;
    worldDisk.fogDistanceStart = 10.0f;
    worldDisk.fogDistanceEnd = 20.0f;
    worldDisk.fogHeightHigh = 30.0f;
    worldDisk.fogHeightLow = 5.0f;
    worldDisk.fogDensity = 0.5f;
    const std::int32_t lightIndex = 1;
    const std::int32_t soundIndex = 2;
    zClass_LightDataPartial lightDisk{};
    zClass_SoundDataPartial soundDisk{};

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        free_zclass_type_lists_for_test();
        return 1;
    }
    if (std::fwrite(diskSlots, sizeof(diskSlots), 1, file) != 1 ||
        std::fwrite(&worldDisk, sizeof(worldDisk), 1, file) != 1 ||
        std::fwrite(&lightIndex, sizeof(lightIndex), 1, file) != 1 ||
        std::fwrite(&soundIndex, sizeof(soundIndex), 1, file) != 1 ||
        std::fwrite(&lightDisk, sizeof(lightDisk), 1, file) != 1 ||
        std::fwrite(&soundDisk, sizeof(soundDisk), 1, file) != 1) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 2;
    }
    std::rewind(file);

    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_ActiveNodeCount = 0;

    if (GameZ_ZBD::ReadNodeTable(3, file) != 3) {
        std::fclose(file);
        free_zclass_type_lists_for_test();
        return 3;
    }
    std::fclose(file);

    zClass_WorldDataPartial *worldData =
        static_cast<zClass_WorldDataPartial *>(g_zClass_NodeArray[0].node.classData);
    const bool tableOk = g_zClass_NodeArraySize == 3 && g_zClass_ActiveNodeCount == 3 &&
                         (g_zClass_NodeArray[0].freeTag & 0x01000000u) != 0 &&
                         (g_zClass_NodeArray[1].freeTag & 0x01000000u) != 0 &&
                         (g_zClass_NodeArray[2].freeTag & 0x01000000u) != 0 &&
                         g_zClass_NodeArray[0].node.actionCallback == nullptr &&
                         g_zClass_NodeArray[1].node.actionCallback == nullptr &&
                         g_zClass_NodeArray[2].node.actionCallback == nullptr &&
                         worldData != nullptr && worldData->lightNodes != nullptr &&
                         worldData->lightNodes[0] == &g_zClass_NodeArray[1].node &&
                         worldData->soundNodes != nullptr &&
                         worldData->soundNodes[0] == &g_zClass_NodeArray[2].node &&
                         worldData->lightDataList != nullptr &&
                         worldData->lightDataList[0] == g_zClass_NodeArray[1].node.classData &&
                         worldData->soundDataList != nullptr &&
                         worldData->soundDataList[0] == g_zClass_NodeArray[2].node.classData &&
                         zClass_TypeList::Head(13) != nullptr &&
                         zClass_TypeList::Head(13)->node == &g_zClass_NodeArray[0].node;

    if (worldData != nullptr) {
        std::free(worldData->lightNodes);
        std::free(worldData->lightDataList);
        std::free(worldData->soundNodes);
        std::free(worldData->soundDataList);
        std::free(worldData->areaGridRows);
        std::free(worldData);
    }
    std::free(g_zClass_NodeArray[1].node.classData);
    std::free(g_zClass_NodeArray[2].node.classData);
    std::free(g_zClass_NodeArray);
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_ActiveNodeCount = 0;
    free_zclass_type_lists_for_test();
    return tableOk ? 0 : 4;
}

extern "C" int zclass_zbd_write_node_table_smoke() {
    zClass_NodeFreeListSlot slots[2]{};
    g_zClass_NodeArray = slots;
    g_zClass_NodeArraySize = 2;

    zDiPartial diPool[2]{};
    g_zModel_DiPoolBase = diPool;

    zClass_Object3DDataPartial objectData{};
    objectData.flags = 0x1357;
    zClass_NodePartial *staleList[1] = {&slots[1].node};
    std::strcpy(slots[0].node.name, "object_node");
    slots[0].node.classId = 5;
    slots[0].node.classData = &objectData;
    slots[0].node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&diPool[1]));
    slots[0].node.actionCallback = staleList;
    slots[0].node.listCountA = 0;
    slots[0].node.listA = staleList;
    slots[0].node.listCountB = 0;
    slots[0].node.listB = staleList;
    slots[0].freeTag = 0xab123456u;

    std::strcpy(slots[1].node.name, "empty_node");
    slots[1].node.classId = 0;
    slots[1].freeTag = 0xcd654321u;

    g_GameZ_Zbd_NodeIndexScratch = static_cast<zClass_NodePartial **>(std::malloc(8));
    g_GameZ_Zbd_NodeIndexScratchCapacity = 2;

    std::FILE *file = std::tmpfile();
    if (file == nullptr) {
        return 1;
    }

    if (GameZ_ZBD::WriteNodeTable(file) != 2 || g_GameZ_Zbd_NodeIndexScratch != nullptr ||
        g_GameZ_Zbd_NodeIndexScratchCapacity != 0) {
        std::fclose(file);
        return 2;
    }

    const long tableBytes = static_cast<long>(sizeof(zClass_NodeFreeListSlot) * 2);
    const long expectedBytes = tableBytes + static_cast<long>(sizeof(zClass_Object3DDataPartial));
    if (std::fseek(file, 0, SEEK_END) != 0 || std::ftell(file) != expectedBytes) {
        std::fclose(file);
        return 3;
    }

    zClass_NodeFreeListSlot written[2]{};
    std::rewind(file);
    if (std::fread(written, sizeof(written), 1, file) != 1) {
        std::fclose(file);
        return 4;
    }

    if (written[0].node.userDataOrDiRef != 1 ||
        written[1].node.userDataOrDiRef != static_cast<std::uint32_t>(-1) ||
        written[0].node.actionCallback != nullptr || written[0].node.listA != nullptr ||
        written[0].node.listB != nullptr) {
        std::fclose(file);
        return 5;
    }

    const std::uint32_t expectedFreeTag =
        (slots[0].freeTag & 0xff000000u) | static_cast<std::uint32_t>(tableBytes);
    if (written[0].freeTag != expectedFreeTag || written[1].freeTag != slots[1].freeTag) {
        std::fclose(file);
        return 6;
    }

    zClass_Object3DDataPartial writtenObject{};
    if (std::fseek(file, tableBytes, SEEK_SET) != 0 ||
        std::fread(&writtenObject, sizeof(writtenObject), 1, file) != 1 ||
        writtenObject.flags != objectData.flags) {
        std::fclose(file);
        return 7;
    }

    std::fclose(file);
    g_zClass_NodeArraySize = 0;
    return 0;
}

extern "C" int gamez_write_zbd_file_smoke() {
    const char *path = "zbd_smoke.tmp";
    std::remove(path);

    std::strcpy(g_zClass_CurrentZbdPath, "old.zbd");
    g_zImage_TexDirEntryCount = 0;

    zModel_MaterialSlot materialSlots[1] = {};
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 1;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_NodeFreeHeadIndex = 5;
    g_zClass_NodeList_PendingFreeHead = nullptr;

    if (GameZ::WriteZBDFile(path) != 0 || std::strcmp(g_zClass_CurrentZbdPath, path) != 0) {
        std::remove(path);
        return 1;
    }

    std::FILE *file = std::fopen(path, "rb");
    if (file == nullptr) {
        std::remove(path);
        return 2;
    }

    zClass_ZbdHeader header = {};
    if (std::fread(&header, sizeof(header), 1, file) != 1) {
        std::fclose(file);
        std::remove(path);
        return 3;
    }

    const long materialOffset = 0x24;
    const long model3dOffset = materialOffset + 16 + static_cast<long>(sizeof(zModel_MaterialSlot));
    const long nodeTableOffset = model3dOffset + 12;
    const bool headerOk = header.magic == 0x02971222 && header.version == 0x0f &&
                          header.texDirArg == 0 && header.texDirOffset == 0x24 &&
                          header.matlOffset == materialOffset &&
                          header.model3dOffset == model3dOffset && header.nodeCount == 0 &&
                          header.nodeFreeHead == 5 && header.nodeTableOffset == nodeTableOffset;

    std::int32_t materialHeader[4] = {};
    const bool materialOk = std::fseek(file, materialOffset, SEEK_SET) == 0 &&
                            std::fread(materialHeader, sizeof(materialHeader), 1, file) == 1 &&
                            materialHeader[0] == 1 && materialHeader[1] == 0 &&
                            materialHeader[2] == 0 && materialHeader[3] == -1;

    const bool sizeOk = std::fseek(file, 0, SEEK_END) == 0 && std::ftell(file) == nodeTableOffset;

    std::fclose(file);
    std::remove(path);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_CurrentZbdPath[0] = '\0';

    return headerOk && materialOk && sizeOk ? 0 : 4;
}

extern "C" int gamez_read_zbd_file_smoke() {
    const char *path = "zbd_read_smoke.tmp";
    std::remove(path);

    std::strcpy(g_zClass_CurrentZbdPath, "old.zbd");
    g_zImage_TexDirEntryCount = 0;

    zModel_MaterialSlot materialSlots[1] = {};
    g_zModel_MatlPool = materialSlots;
    g_zModel_MatlPoolCapacity = 1;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlActiveHeadIndex = -1;

    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;

    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zClass_NodeFreeHeadIndex = 5;
    g_zClass_NodeList_PendingFreeHead = nullptr;

    if (GameZ::WriteZBDFile(path) != 0) {
        std::remove(path);
        return 1;
    }

    std::strcpy(g_zClass_CurrentZbdPath, "before-read.zbd");
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = -3;
    g_zModel_DiPoolInUseCount = -4;
    g_zModel_DiPoolFreeHeadIndex = -5;
    g_zClass_NodeFreeHeadIndex = -6;

    if (GameZ::ReadZBDFile(path) != 0) {
        std::free(g_zModel_MatlPool);
        g_zModel_MatlPool = nullptr;
        std::remove(path);
        return 2;
    }

    const bool ok = std::strcmp(g_zClass_CurrentZbdPath, path) == 0 &&
                    g_zClass_NodeFreeHeadIndex == 5 && g_zModel_MatlPool != nullptr &&
                    g_zModel_MatlPoolCapacity == 1 && g_zModel_MatlPoolInUseCount == 0 &&
                    g_zModel_MatlFreeHeadIndex == 0 && g_zModel_MatlActiveHeadIndex == -1 &&
                    g_zModel_DiPoolBase == nullptr && g_zModel_DiPoolCapacity == 0 &&
                    g_zModel_DiPoolInUseCount == 0 && g_zModel_DiPoolFreeHeadIndex == -1;

    std::free(g_zModel_MatlPool);
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlPoolCapacity = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_CurrentZbdPath[0] = '\0';
    std::remove(path);

    return ok ? 0 : 3;
}

extern "C" int gamez_open_and_read_zbd_header_smoke() {
    const char *path = "zbd_header_smoke.tmp";
    std::remove(path);

    zClass_ZbdHeader expected = {};
    expected.magic = 0x02971222;
    expected.version = 0x0f;
    expected.texDirArg = 7;
    expected.texDirOffset = 0x24;
    expected.matlOffset = 0x40;
    expected.model3dOffset = 0x80;
    expected.nodeCount = 3;
    expected.nodeFreeHead = 2;
    expected.nodeTableOffset = 0xc0;

    std::FILE *file = std::fopen(path, "wb");
    if (file == nullptr) {
        return 1;
    }

    std::fwrite(&expected, sizeof(expected), 1, file);
    std::fputc(0x5a, file);
    std::fclose(file);

    zClass_ZbdHeader actual = {};
    file = GameZ::OpenAndReadZBDHeader(path, &actual);
    if (file == nullptr) {
        std::remove(path);
        return 2;
    }

    const bool successOk = std::memcmp(&actual, &expected, sizeof(actual)) == 0 &&
                           std::ftell(file) == static_cast<long>(sizeof(zClass_ZbdHeader)) &&
                           std::fgetc(file) == 0x5a;
    std::fclose(file);
    if (!successOk) {
        std::remove(path);
        return 3;
    }

    file = std::fopen(path, "wb");
    if (file == nullptr) {
        std::remove(path);
        return 4;
    }
    std::fwrite(&expected, sizeof(expected) - 1, 1, file);
    std::fclose(file);
    if (GameZ::OpenAndReadZBDHeader(path, &actual) != nullptr) {
        std::remove(path);
        return 5;
    }

    file = std::fopen(path, "wb");
    if (file == nullptr) {
        std::remove(path);
        return 6;
    }
    expected.magic = 0x12345678;
    expected.version = 0x0f;
    std::fwrite(&expected, sizeof(expected), 1, file);
    std::fclose(file);
    if (GameZ::OpenAndReadZBDHeader(path, &actual) != nullptr) {
        std::remove(path);
        return 7;
    }

    file = std::fopen(path, "wb");
    if (file == nullptr) {
        std::remove(path);
        return 8;
    }
    expected.magic = 0x02971222;
    expected.version = 0x10;
    std::fwrite(&expected, sizeof(expected), 1, file);
    std::fclose(file);
    if (GameZ::OpenAndReadZBDHeader(path, &actual) != nullptr) {
        std::remove(path);
        return 9;
    }

    std::remove(path);
    return 0;
}

extern "C" int gamez_reload_display_instances_smoke() {
    reset_zclass_type_lists_for_test();

    const char *path = "zbd_reload_di_smoke.tmp";
    std::remove(path);

    zClass_NodeFreeListSlot nodeSlots[2] = {};
    zClass_NodePartial *rootChildren[1] = {&nodeSlots[1].node};
    nodeSlots[0].node.listCountB = 1;
    nodeSlots[0].node.listB = rootChildren;
    g_zClass_NodeArray = nodeSlots;
    g_zClass_NodeArraySize = 2;

    zDiPartial liveDiPool[2] = {};
    liveDiPool[0].refCount = 1;
    liveDiPool[1].nextFreeIndex = -1;
    nodeSlots[0].node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&liveDiPool[0]));

    g_zModel_DiPoolBase = liveDiPool;
    g_zModel_DiPoolCapacity = 2;
    g_zModel_DiPoolInUseCount = 1;
    g_zModel_DiPoolFreeHeadIndex = 1;

    zClass_ZbdHeader header = {};
    header.magic = 0x02971222;
    header.version = 0x0f;
    header.nodeCount = 2;
    header.nodeTableOffset = sizeof(zClass_ZbdHeader);
    header.model3dOffset =
        header.nodeTableOffset + 2 * static_cast<std::int32_t>(sizeof(zClass_NodeFreeListSlot));

    zClass_NodeFreeListSlot serializedNodes[2] = {};
    serializedNodes[0].node.userDataOrDiRef = 0;
    serializedNodes[1].node.userDataOrDiRef = 1;

    std::int32_t diHeader[3] = {2, 2, -1};
    zDiPartial serializedDi[2] = {};
    const std::int32_t dynamicOffset = header.model3dOffset +
                                       static_cast<std::int32_t>(sizeof(diHeader)) +
                                       2 * static_cast<std::int32_t>(sizeof(zDiPartial));
    serializedDi[0].textureWorldAxis = 101;
    serializedDi[0].nextFreeIndex = dynamicOffset;
    serializedDi[1].textureWorldAxis = 202;
    serializedDi[1].nextFreeIndex = dynamicOffset;

    std::FILE *file = std::fopen(path, "wb");
    if (file == nullptr) {
        free_zclass_type_lists_for_test();
        return 1;
    }

    if (std::fwrite(&header, sizeof(header), 1, file) != 1 ||
        std::fwrite(serializedNodes, sizeof(serializedNodes), 1, file) != 1 ||
        std::fwrite(diHeader, sizeof(diHeader), 1, file) != 1 ||
        std::fwrite(serializedDi, sizeof(serializedDi), 1, file) != 1) {
        std::fclose(file);
        std::remove(path);
        free_zclass_type_lists_for_test();
        return 2;
    }
    std::fclose(file);

    std::strcpy(g_zClass_CurrentZbdPath, path);
    const std::int32_t result =
        GameZ_ZBD::ReloadDisplayInstancesFromCurrentPath_Local(&nodeSlots[0].node, 1);

    zDiPartial *const rootDi = reinterpret_cast<zDiPartial *>(
        static_cast<std::uintptr_t>(nodeSlots[0].node.userDataOrDiRef));
    zDiPartial *const childDi = reinterpret_cast<zDiPartial *>(
        static_cast<std::uintptr_t>(nodeSlots[1].node.userDataOrDiRef));
    const bool ok = result == 0 && rootDi == &liveDiPool[1] && childDi == &liveDiPool[0] &&
                    rootDi->textureWorldAxis == 101 && childDi->textureWorldAxis == 202 &&
                    rootDi->refCount == 1 && childDi->refCount == 1 &&
                    g_zModel_DiPoolInUseCount == 2 && g_zModel_DiPoolFreeHeadIndex == -1;

    std::remove(path);
    g_zClass_CurrentZbdPath[0] = '\0';
    g_zClass_NodeArray = nullptr;
    g_zClass_NodeArraySize = 0;
    g_zModel_DiPoolBase = nullptr;
    g_zModel_DiPoolCapacity = 0;
    g_zModel_DiPoolInUseCount = 0;
    g_zModel_DiPoolFreeHeadIndex = -1;
    free_zclass_type_lists_for_test();

    return ok ? 0 : 3;
}

extern "C" int zclass_try_free_node_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slots[3]{};
    g_zClass_NodeArray = slots;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 2;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *immediate = &slots[1].node;
    immediate->flags = 2;
    immediate->classId = 5;
    immediate->classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    if (zClass_Class::TryFreeNode(immediate) != 0 || immediate->classData != nullptr ||
        immediate->classId != 0 || (immediate->flags & 2) != 0 || g_zClass_NodeFreeHeadIndex != 1 ||
        g_zClass_ActiveNodeCount != 1) {
        return 1;
    }

    zClass_NodePartial *deferred = &slots[2].node;
    deferred->flags = 2;
    deferred->classId = 5;
    deferred->classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    g_zClass_DeferredProcessingEnabled = 0;
    if (zClass_Class::TryFreeNode(deferred) != 0 || deferred->classData == nullptr ||
        (deferred->flags & 2) != 0 || g_zClass_NodeList_PendingFreeHead == nullptr ||
        g_zClass_NodeList_PendingFreeHead->node != deferred) {
        return 2;
    }

    g_zClass_DeferredProcessingEnabled = 1;
    zClass_NodeList::ProcessPendingFrees();
    if (g_zClass_NodeList_PendingFreeHead != nullptr || deferred->classData != nullptr ||
        g_zClass_NodeFreeHeadIndex != 2 || g_zClass_ActiveNodeCount != 0) {
        return 3;
    }

    return zClass_Class::TryFreeNode(nullptr) == 5 ? 0 : 4;
}

extern "C" int zdi_ref_and_pool_free_smoke() {
    zDiPartial pool[2]{};
    g_zModel_DiPoolBase = pool;
    g_zModel_DiPoolInUseCount = 2;
    g_zModel_DiPoolFreeHeadIndex = 7;

    if (zDi::AddRef(&pool[0]) != 0 || pool[0].refCount != 1 || zDi::Release(&pool[0]) != 0 ||
        pool[0].refCount != 0 || zDi::PtrToIndexOrMinus1(nullptr) != -1 ||
        zDi::PtrToIndexOrMinus1(&pool[1]) != 1 || zDi::IndexToPtrOrNull(-1) != nullptr ||
        zDi::IndexToPtrOrNull(1) != &pool[1]) {
        return 1;
    }

    pool[1].refCount = 1;
    if (zModel_DiPool::FreeIfUnreferenced(nullptr) != 5 ||
        zModel_DiPool::FreeIfUnreferenced(&pool[1]) != 1 || g_zModel_DiPoolInUseCount != 2) {
        return 2;
    }

    pool[0].mode = 3;
    pool[0].flags = 0x44;
    if (zModel_DiPool::FreeIfUnreferenced(&pool[0]) != 0) {
        return 3;
    }

    return pool[0].mode == 0 && pool[0].flags == 0 && pool[0].nextFreeIndex == 7 &&
                   g_zModel_DiPoolFreeHeadIndex == 0 && g_zModel_DiPoolInUseCount == 1
               ? 0
               : 4;
}

extern "C" int zclass_set_display_instance_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodePartial node{};
    zDiPartial oldDisplay{};
    zDiPartial newDisplay{};
    oldDisplay.refCount = 2;
    node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&oldDisplay));

    if (zClass_Class::gwNodeSetDisplayInstance(&node, &newDisplay) != 0 ||
        oldDisplay.refCount != 1 || newDisplay.refCount != 1 ||
        node.userDataOrDiRef !=
            static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&newDisplay)) ||
        (node.flags & 0x203) != 0x203 || (node.boundsFlags & 1) == 0 ||
        zClass_TypeList::Head(7) == nullptr) {
        return 1;
    }

    if (zClass_Class::gwNodeSetDisplayInstance(&node, nullptr) != 0 || newDisplay.refCount != 0 ||
        node.userDataOrDiRef != 0 || (node.flags & 0x200) != 0) {
        return 2;
    }

    zClass_TypeList::FreeAll();
    return zClass_Class::gwNodeSetDisplayInstance(nullptr, nullptr) == 5 ? 0 : 3;
}

extern "C" int zclass_remove_dispatch_smoke() {
    zClass_NodePartial parent{};
    zClass_NodePartial child{};
    zClass_Object3DDataPartial parentData{};
    parent.classId = 5;
    parent.classData = &parentData;

    if (zClass_Class::AddChildGeneric(&parent, &child) != 0 ||
        zClass_Class::RemoveChild(&parent, &child) != 0 || parent.listCountB != 0 ||
        child.listCountA != 0) {
        return 1;
    }

    parent.classId = 99;
    if (zClass_Class::RemoveChild(&parent, &child) != 1 ||
        zClass_Class::RemoveChild(nullptr, &child) != 5 ||
        zClass_Class::RemoveChild(&parent, nullptr) != 5) {
        return 2;
    }

    std::free(parent.listB);
    std::free(child.listA);
    return 0;
}

extern "C" int zclass_destroy_node_recursive_display_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    zDiPartial displayPool[1]{};
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = -1;
    g_zClass_ActiveNodeCount = 1;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zModel_DiPoolBase = displayPool;
    g_zModel_DiPoolFreeHeadIndex = 4;
    g_zModel_DiPoolInUseCount = 1;

    slot.node.classId = 5;
    slot.node.classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    slot.node.userDataOrDiRef =
        static_cast<std::uint32_t>(reinterpret_cast<std::uintptr_t>(&displayPool[0]));
    displayPool[0].refCount = 1;

    if (zClass_Util::DestroyNodeRecursive(&slot.node) != 0) {
        return 1;
    }

    return slot.node.classData == nullptr && slot.node.userDataOrDiRef == 0 &&
                   displayPool[0].nextFreeIndex == 4 && g_zModel_DiPoolFreeHeadIndex == 0 &&
                   g_zModel_DiPoolInUseCount == 0 && g_zClass_ActiveNodeCount == 0
               ? 0
               : 2;
}

extern "C" int zclass_object3d_delete_node_smoke() {
    zClass_NodeFreeListSlot slot{};
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0x222222;
    g_zClass_ActiveNodeCount = 1;
    g_zClass_DeferredProcessingEnabled = 1;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }

    slot.node.classId = 5;
    slot.node.classData = std::calloc(1, sizeof(zClass_Object3DDataPartial));
    if (zClass_Object3D::DeleteNode(&slot.node) != 0 || slot.node.classData != nullptr ||
        g_zClass_ActiveNodeCount != 0) {
        return 1;
    }

    return zClass_Object3D::DeleteNode(nullptr) == 5 ? 0 : 2;
}

extern "C" int zclass_sound_leaf_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *const node = zClass_Sound::gwSoundNew();
    if (node != &slot.node || node->classId != 10 || node->classData == nullptr ||
        (node->flags & 0x104) != 0x104 || g_zClass_NodeFreeHeadIndex != -1 ||
        g_zClass_ActiveNodeCount != 1 || zClass_TypeList::Head(10) == nullptr ||
        zClass_TypeList::Head(10)->node != node) {
        return 1;
    }

    const float expectedBounds[6] = {1.0f, 1.0f, -2.0f, 2.0f, 2.0f, -1.0f};
    for (int i = 0; i < 6; ++i) {
        if (node->cachedBounds[i] != expectedBounds[i]) {
            return 2;
        }
    }

    zClass_SoundDataPartial *const newData =
        static_cast<zClass_SoundDataPartial *>(node->classData);
    if (newData->runtimeFlags != 1 || newData->falloffMode != 1 || newData->rangeMin != 32.0f ||
        newData->rangeMax != 64.0f || newData->rangeMaxSq != 4096.0f ||
        newData->invRangeSpan != 0.03125f || newData->attachedWorldCount != 0 ||
        newData->attachedWorlds != nullptr) {
        return 3;
    }

    if (zClass_Sound::DeleteNode(node) != 0 || slot.node.classData != nullptr ||
        g_zClass_ActiveNodeCount != 0) {
        zClass_TypeList::FreeAll();
        return 4;
    }

    zClass_TypeList::FreeAll();

    zClass_NodePartial stackNode = {};
    zClass_SoundDataPartial data = {};
    zSndPlayHandle playHandle = {};
    stackNode.classData = &data;
    data.playHandle = &playHandle;
    data.runtimeFlags = 0x10;
    g_zSnd_IsInitialized = 0;
    if (zClass_Sound::SetSampleSetByName(&stackNode, "short") != 0 ||
        std::strcmp(data.sampleSetName, "short") != 0 || data.sample != nullptr ||
        data.playHandle != nullptr || data.runtimeFlags != 0x11) {
        return 5;
    }

    char longName[0x30] = {};
    std::memset(longName, 'a', sizeof(longName) - 1);
    data.sampleSetName[0x22] = 'Z';
    if (zClass_Sound::SetSampleSetByName(&stackNode, longName) != 0 ||
        std::strncmp(data.sampleSetName, longName, 0x22) != 0 || data.sampleSetName[0x22] != 'Z' ||
        data.sampleSetName[0x23] != '\0') {
        return 6;
    }

    data.runtimeFlags = 0;
    if (zClass_Sound::gwSoundSetPosition(&stackNode, 1.0f, 2.0f, 3.0f) != 0 ||
        data.localPosition.x != 1.0f || data.localPosition.y != 2.0f ||
        data.localPosition.z != 3.0f || data.runtimeFlags != 3) {
        return 61;
    }

    stackNode.classData = nullptr;
    if (zClass_Sound::SetSampleSetByName(nullptr, "x") != 5 ||
        zClass_Sound::SetSampleSetByName(&stackNode, "x") != 5 ||
        zClass_Sound::gwSoundSetPosition(nullptr, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Sound::gwSoundSetPosition(&stackNode, 1.0f, 2.0f, 3.0f) != 5) {
        return 7;
    }

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Sound::gwSoundNew() == nullptr ? 0 : 8;
}

extern "C" int zclass_object3d_init_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Object3D::gwObject3DInit();
    if (node != &slot.node || node->classId != 5 || node->classData == nullptr ||
        g_zClass_NodeFreeHeadIndex != -1 || g_zClass_ActiveNodeCount != 1) {
        return 1;
    }

    zClass_Object3DDataPartial *data = static_cast<zClass_Object3DDataPartial *>(node->classData);
    if (data->flags != 0x29 || data->scale.x != 1.0f || data->scale.y != 1.0f ||
        data->scale.z != 1.0f || data->localMatrix[0] != 1.0f || data->localMatrix[4] != 1.0f ||
        data->localMatrix[8] != 1.0f || (node->boundsFlags & 4) == 0 ||
        (node->flags & 0x02000003) != 0x02000003) {
        return 2;
    }

    zClass_Object3D::DeleteNode(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Object3D::gwObject3DInit() == nullptr ? 0 : 3;
}

extern "C" int zclass_window_new_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zRndr::g_frameBuffer = reinterpret_cast<void *>(0x76543210);
    zRndr::g_activeRegionWidth = 512;
    zRndr::g_activeRegionHeight = 384;
    zRndr::g_pitchBytes = 2048;
    zRndr::g_bytesPerPixel = 2;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Window::gwWindowNew();
    if (node != &slot.node || node->classId != 3 || node->classData == nullptr ||
        zClass_TypeList::Head(14) == nullptr || zClass_TypeList::Head(14)->node != node) {
        return 1;
    }

    zClass_WindowDataPartial *data = static_cast<zClass_WindowDataPartial *>(node->classData);
    if (data->viewportWidth != 0 || data->viewportHeight != 0 || data->resolutionWidth != 1 ||
        data->resolutionHeight != 1 || data->bufferIndex != -1 ||
        data->buffer != reinterpret_cast<void *>(0x76543210) || data->fbWidth != 512 ||
        data->fbHeight != 384 || data->fbBpp != 16) {
        return 2;
    }

    if (zClass_Window::gwWindowSetResolution(node, 320, 240) != 0) {
        return 3;
    }

    std::int32_t width = 0;
    std::int32_t height = 0;
    if (zClass_Window::gwWindowGetResolution(node, &width, &height) != 0 || width != 320 ||
        height != 240) {
        return 4;
    }

    if (zClass_Window::gwWindowSetSize(node, 123, 45) != 0 ||
        zClass_Window::gwWindowGetSize(node, &width, &height) != 0 || width != 123 ||
        height != 45) {
        return 5;
    }

    if (zClass_Window::gwWindowSetBuffer(node, 7) != 0 || data->bufferIndex != 7) {
        return 6;
    }

    data->clearPolyIndexFlags = 3;
    if (zClass_Window::gwWindowSetClearPolygon(node, 1) != 0 ||
        data->clearPolyIndexFlags != static_cast<std::int32_t>(0x80000003u)) {
        return 7;
    }
    if (zClass_Window::gwWindowSetClearPolygon(node, 2) != 0 || data->clearPolyIndexFlags != 3) {
        return 8;
    }

    data->clearPolyIndexFlags = 0;
    zVec3 point{9.0f, 8.0f, 7.0f};
    if (zClass_Window::gwWindowAddClearPolygonVertex(node, &point) != 0 ||
        data->clearPolys[0].vertCount != static_cast<std::int32_t>(0x80000001u) ||
        data->clearPolys[0].vertices[0].x != 9.0f || data->clearPolys[0].vertices[0].y != 8.0f ||
        data->clearPolys[0].vertices[0].z != 100.0f) {
        return 9;
    }

    zRndr::g_spanOccluderPolyCount = 0;
    for (int slotIndex = 0; slotIndex < 8; ++slotIndex) {
        zRndr::g_spanOccluderPolys[slotIndex] = {};
    }
    if (zClass_Window::gwWindowCloseClearPolygon(node) != 0 ||
        data->clearPolyIndexFlags != static_cast<std::int32_t>(0x80000001u) ||
        zRndr::g_spanOccluderPolyCount != 1 || zRndr::g_spanOccluderPolys[0].vertCount != 1 ||
        zRndr::g_spanOccluderPolys[0].vertices[0][0] != 9.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[0][1] != 8.0f ||
        zRndr::g_spanOccluderPolys[0].vertices[0][2] != 100.0f) {
        return 10;
    }

    data->clearPolys[0].vertCount = 4;
    data->clearPolyIndexFlags = 0;
    if (zClass_Window::gwWindowAddClearPolygonVertex(node, &point) != 1) {
        return 11;
    }
    data->clearPolyIndexFlags = 4;
    if (zClass_Window::gwWindowAddClearPolygonVertex(node, &point) != 1) {
        return 12;
    }
    if (zClass_Window::gwWindowCloseClearPolygon(node) != 1) {
        return 13;
    }

    zClass_NodePartial wrongClass{};
    wrongClass.classId = 4;
    wrongClass.classData = data;
    zClass_NodePartial nullData{};
    nullData.classId = 3;
    if (zClass_Window::gwWindowGetResolution(nullptr, &width, &height) != 5 ||
        zClass_Window::gwWindowGetResolution(&nullData, &width, &height) != 5 ||
        zClass_Window::gwWindowGetResolution(&wrongClass, &width, &height) != 3) {
        return 14;
    }

    if (zClass_Window::gwWindowGetSize(nullptr, &width, &height) != 5 ||
        zClass_Window::gwWindowGetSize(&nullData, &width, &height) != 5 ||
        zClass_Window::gwWindowGetSize(&wrongClass, &width, &height) != 3) {
        return 15;
    }

    if (zClass_Window::gwWindowSetBuffer(nullptr, 1) != 5 ||
        zClass_Window::gwWindowSetBuffer(&nullData, 1) != 5 ||
        zClass_Window::gwWindowSetBuffer(&wrongClass, 1) != 3) {
        return 16;
    }

    if (zClass_Window::gwWindowSetClearPolygon(nullptr, 1) != 5 ||
        zClass_Window::gwWindowSetClearPolygon(&nullData, 1) != 5 ||
        zClass_Window::gwWindowSetClearPolygon(&wrongClass, 1) != 3) {
        return 17;
    }

    if (zClass_Window::gwWindowAddClearPolygonVertex(nullptr, &point) != 5 ||
        zClass_Window::gwWindowAddClearPolygonVertex(&nullData, &point) != 5 ||
        zClass_Window::gwWindowAddClearPolygonVertex(&wrongClass, &point) != 3) {
        return 18;
    }

    if (zClass_Window::gwWindowCloseClearPolygon(nullptr) != 5 ||
        zClass_Window::gwWindowCloseClearPolygon(&nullData) != 5 ||
        zClass_Window::gwWindowCloseClearPolygon(&wrongClass) != 3) {
        return 19;
    }

    zClass_Object3D::DeleteNode(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Window::gwWindowNew() == nullptr ? 0 : 20;
}

extern "C" int zclass_display_init_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Display::gwDisplayInit();
    if (node != &slot.node || node->classId != 4 || node->classData == nullptr ||
        zClass_TypeList::Head(15) == nullptr || zClass_TypeList::Head(15)->node != node) {
        return 1;
    }

    zClass_DisplayDataPartial *data = static_cast<zClass_DisplayDataPartial *>(node->classData);
    if (data->x != 0 || data->y != 0 || data->width != 1 || data->height != 1 ||
        data->backgroundR != 0.392f || data->backgroundG != 0.392f || data->backgroundB != 1.0f) {
        return 2;
    }

    zClass_NodePartial removeParent{};
    zClass_NodePartial child{};
    zClass_NodePartial *children[] = {&child};
    zClass_NodePartial *parents[] = {&removeParent};
    removeParent.flags = 1;
    removeParent.listCountB = 1;
    removeParent.listB = children;
    child.listCountA = 1;
    child.listA = parents;
    if (zClass_Display::RemoveChild(&removeParent, &child) != 0 || removeParent.listCountB != 0 ||
        child.listCountA != 0 || zClass_Display::RemoveChild(nullptr, &child) != 5 ||
        zClass_Display::RemoveChild(&removeParent, nullptr) != 5) {
        return 3;
    }

    g_zVideo_PixelPack_RMaskShifted = 0xf8;
    g_zVideo_PixelPack_GMaskShifted = 0xfc;
    g_zVideo_PixelPack_RShift = 8;
    g_zVideo_PixelPack_GShift = 3;
    g_zVideo_PixelPack_BShiftTo8 = 3;
    g_zVideo_ClearColorPacked16 = 0;
    if (zClass_Display::gwDisplaySetBackgroundColor(node, 255.0f, 128.0f, 32.0f) != 0 ||
        data->backgroundR != 255.0f || data->backgroundG != 128.0f || data->backgroundB != 32.0f ||
        g_zVideo_ClearColorPacked16 != 0xfc04) {
        return 4;
    }

    zClass_NodePartial nullData{};
    nullData.classId = 4;
    zClass_NodePartial wrongClass{};
    wrongClass.classId = 3;
    wrongClass.classData = data;
    if (zClass_Display::gwDisplaySetBackgroundColor(nullptr, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Display::gwDisplaySetBackgroundColor(&nullData, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Display::gwDisplaySetBackgroundColor(&wrongClass, 1.0f, 2.0f, 3.0f) != 3) {
        return 5;
    }

    zClass_Object3D::DeleteNode(node);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Display::gwDisplayInit() == nullptr ? 0 : 6;
}

extern "C" int zclass_lod_leaf_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Lod::gwLodNew();
    if (node != &slot.node || node->classId != 6 || node->classData == nullptr ||
        g_zClass_ActiveNodeCount != 1) {
        return 1;
    }

    zClass_LodDataPartial *data = static_cast<zClass_LodDataPartial *>(node->classData);
    if (data->computeOwnDistance != 1 || data->nearRange != 1000.0f ||
        data->farRangeSq != 1000000.0f || data->active != 1) {
        return 2;
    }

    zClass_NodePartial child{};
    if (zClass_Lod::SetComputeOwnDistance(node, 0) != 0 || data->computeOwnDistance != 0 ||
        zClass_Lod::SetTargetNodeAndRange(node, &child, 3.0f) != 0 || data->rangeNode != &child ||
        data->rangeSq != 9.0f) {
        return 3;
    }
    if (zClass_Lod::SetTargetNodeAndRange(node, nullptr, 4.0f) != 0 || data->rangeNode != nullptr ||
        data->rangeSq != 9.0f) {
        return 4;
    }

    if (zClass_Lod::gwLodAddChild(node, &child) != 0 || node->listCountB != 1 ||
        child.listCountA != 1 || zClass_Lod::RemoveChild(node, &child) != 0 ||
        node->listCountB != 0 || child.listCountA != 0) {
        return 5;
    }

    zClass_Class::TryFreeNode(node);
    zClass_TypeList::FreeAll();
    zClass_TypeList::FreeAll();
    return g_zClass_ActiveNodeCount == 0 ? 0 : 6;
}

extern "C" int zclass_light_new_smoke() {
    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *node = zClass_Light::gwLightNew();
    if (node != &slot.node || node->classId != 9 || (node->flags & 0x104) != 0x104 ||
        node->classData == nullptr || zClass_TypeList::Head(9) == nullptr ||
        zClass_TypeList::Head(9)->node != node) {
        return 1;
    }

    if (node->cachedBounds[0] != 1.0f || node->cachedBounds[1] != 1.0f ||
        node->cachedBounds[2] != -2.0f || node->cachedBounds[3] != 2.0f ||
        node->cachedBounds[4] != 2.0f || node->cachedBounds[5] != -1.0f) {
        return 2;
    }

    zClass_LightDataPartial *data = static_cast<zClass_LightDataPartial *>(node->classData);
    if (data->dirty != 1 || data->enabled != 1 || data->worldDir.x != 0.0f ||
        data->worldDir.y != 1.0f || data->worldDir.z != 0.0f || data->specularColor.red != 1.0f ||
        data->specularColor.green != 1.0f || data->specularColor.blue != 1.0f ||
        data->falloff != 0.0f || data->intensityScale != 1.0f || data->coneAngle != 0.0f ||
        data->isPointMode != 0 || data->isDirectionalMode != 1 || data->lightParam != 1 ||
        data->lightSubMode != 1 || data->range1 != 32.0f || data->range2 != 64.0f ||
        data->range2Sq != 4096.0f || data->invRangeDelta != 0.03125f ||
        data->attachedWorldCount != 0 || data->attachedWorlds != nullptr) {
        return 3;
    }

    zClass_NodePartial removeParent{};
    zClass_NodePartial child{};
    zClass_NodePartial *children[] = {&child};
    zClass_NodePartial *parents[] = {&removeParent};
    removeParent.flags = 1;
    removeParent.listCountB = 1;
    removeParent.listB = children;
    child.listCountA = 1;
    child.listA = parents;
    if (zClass_Light::RemoveChild(&removeParent, &child) != 0 || removeParent.listCountB != 0 ||
        child.listCountA != 0 || zClass_Light::RemoveChild(nullptr, &child) != 5 ||
        zClass_Light::RemoveChild(&removeParent, nullptr) != 5) {
        return 4;
    }

    data->dirty = 0;
    if (zClass_Light::gwLightSetIntensity(node, 0.25f) != 0 || data->dirty != 1 ||
        data->intensityScale != 0.25f) {
        return 5;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetFalloff(node, 0.75f) != 0 || data->dirty != 1 ||
        data->falloff != 0.75f) {
        return 6;
    }

    zClass_NodePartial nullLightData{};
    float coneAngle = 0.5f;
    std::uint32_t coneAngleBits = 0;
    std::memcpy(&coneAngleBits, &coneAngle, sizeof(coneAngleBits));
    data->dirty = 0;
    if (zClass_Light::gwLightSetConeAngle(node, coneAngleBits) != 0 || data->dirty != 1 ||
        data->coneAngle != 0.5f) {
        return 7;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetPointMode(node) != 0 || data->dirty != 1 ||
        data->isPointMode != 1 || data->isDirectionalMode != 0) {
        return 8;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetDirectionalMode(node) != 0 || data->dirty != 1 ||
        data->isPointMode != 0 || data->isDirectionalMode != 1) {
        return 9;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetParam(node, 7) != 0 || data->dirty != 1 || data->lightParam != 7) {
        return 10;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetRange(node, 20.0f, 10.0f) != 0 || data->dirty != 1 ||
        data->range1 != 10.0f || data->range2 != 20.0f || data->range2Sq != 400.0f ||
        data->invRangeDelta != 0.1f) {
        return 11;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetRange(node, 5.0f, 5.0f) != 0 || data->dirty != 1 ||
        data->range1 != 5.0f || data->range2 != 15.0f || data->range2Sq != 225.0f ||
        data->invRangeDelta != 0.1f ||
        std::strcmp(g_zError_DebugMsgBuffer,
                    "D:\\Proj\\GameZRecoil\\zClass\\Light.c: Line 540: ERROR setting light ranges; "
                    "Range2 can't be equal to Range1.\n") != 0) {
        return 12;
    }
    float rangeA = 0.0f;
    float rangeB = 0.0f;
    if (zClass_Light::gwLightGetRange(node, &rangeA, &rangeB) != 0 || rangeA != 5.0f ||
        rangeB != 15.0f) {
        return 13;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetPosition(node, 1.0f, 2.0f, 3.0f) != 0 || data->dirty != 1 ||
        data->localPosition.x != 1.0f || data->localPosition.y != 2.0f ||
        data->localPosition.z != 3.0f) {
        return 14;
    }
    data->dirty = 0;
    if (zClass_Light::gwLightSetRotation(node, 4.0f, 5.0f, 6.0f) != 0 || data->dirty != 1 ||
        data->localRotation.x != 4.0f || data->localRotation.y != 5.0f ||
        data->localRotation.z != 6.0f) {
        return 15;
    }
    data->localRotation = {0.0f, 0.0f, 0.0f};
    data->localPosition = {1.0f, 2.0f, 3.0f};
    data->isPointMode = 0;
    data->coneAngle = 0.0f;
    if (zClass_Light::ComputeWorldTransform(node, data) != 0 || data->worldPosition.x != 1.0f ||
        data->worldPosition.y != 2.0f || data->worldPosition.z != 3.0f ||
        data->worldDir.x != 0.0f || data->worldDir.y != 0.0f || data->worldDir.z != -1.0f) {
        return 151;
    }
    data->localPosition = {0.0f, 0.0f, 0.0f};
    data->isPointMode = 1;
    if (zClass_Light::ComputeWorldTransform(node, data) != 0 || data->worldRotation.x != 0.0f ||
        data->worldRotation.y != 0.0f || data->worldRotation.z != 0.0f) {
        return 152;
    }
    data->specularColor = {0.2f, 0.4f, 0.6f};
    float specR = 0.0f;
    float specG = 0.0f;
    float specB = 0.0f;
    if (zClass_Light::gwLightGetSpecularColor(node, &specR, &specG, &specB) != 0 || specR != 0.2f ||
        specG != 0.4f || specB != 0.6f) {
        return 16;
    }
    zVideo::PixelPack_SetupFromMasks(5, 6, 5, 0xf800, 0x07e0, 0x001f);
    zRndr::g_fogTargetParamsStaged = {};
    g_zVideo_RendererType = 1;
    g_zVideo_D3DColorNormalizeChannelIndex = -1;
    data->dirty = 0;
    if (zClass_Light::gwLightSetSpecularColor(node, 1.2f, -0.5f, 0.5f) != 0 || data->dirty != 1 ||
        data->specularColor.red != 1.0f || data->specularColor.green != 0.0f ||
        data->specularColor.blue != 0.5f ||
        zRndr::g_fogTargetParamsStaged.packedColor16 != 0xf810 ||
        g_zVideo_D3DColorNormalizeChannelIndex != 0) {
        return 17;
    }
    if (zClass_Light::gwLightSetIntensity(nullptr, 1.0f) != 5 ||
        zClass_Light::gwLightSetIntensity(&nullLightData, 1.0f) != 5 ||
        zClass_Light::gwLightSetFalloff(nullptr, 1.0f) != 5 ||
        zClass_Light::gwLightSetFalloff(&nullLightData, 1.0f) != 5 ||
        zClass_Light::gwLightSetConeAngle(nullptr, coneAngleBits) != 5 ||
        zClass_Light::gwLightSetConeAngle(&nullLightData, coneAngleBits) != 5 ||
        zClass_Light::gwLightSetPointMode(nullptr) != 5 ||
        zClass_Light::gwLightSetPointMode(&nullLightData) != 5 ||
        zClass_Light::gwLightSetDirectionalMode(nullptr) != 5 ||
        zClass_Light::gwLightSetDirectionalMode(&nullLightData) != 5 ||
        zClass_Light::gwLightSetParam(nullptr, 1) != 5 ||
        zClass_Light::gwLightSetParam(&nullLightData, 1) != 5 ||
        zClass_Light::gwLightSetRange(nullptr, 1.0f, 2.0f) != 5 ||
        zClass_Light::gwLightSetRange(&nullLightData, 1.0f, 2.0f) != 5 ||
        zClass_Light::gwLightGetRange(nullptr, &rangeA, &rangeB) != 5 ||
        zClass_Light::gwLightGetRange(&nullLightData, &rangeA, &rangeB) != 5 ||
        zClass_Light::gwLightSetPosition(nullptr, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Light::gwLightSetPosition(&nullLightData, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Light::gwLightSetRotation(nullptr, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Light::gwLightSetRotation(&nullLightData, 1.0f, 2.0f, 3.0f) != 5 ||
        zClass_Light::gwLightGetSpecularColor(nullptr, &specR, &specG, &specB) != 5 ||
        zClass_Light::gwLightGetSpecularColor(&nullLightData, &specR, &specG, &specB) != 5 ||
        zClass_Light::gwLightSetSpecularColor(nullptr, 1.0f, 1.0f, 1.0f) != 5 ||
        zClass_Light::gwLightSetSpecularColor(&nullLightData, 1.0f, 1.0f, 1.0f) != 5) {
        return 18;
    }

    if (zClass_Light::DeleteNode(nullptr) != 5) {
        return 19;
    }

    zClass_NodePartial nullData{};
    if (zClass_Light::DeleteNode(&nullData) != 5) {
        return 20;
    }

    data->attachedWorldCount = 2;
    if (zClass_Light::DeleteNode(node) != 1 || node->classData == nullptr) {
        return 21;
    }

    data->attachedWorldCount = 0;
    data->attachedWorlds =
        static_cast<zClass_NodePartial **>(std::calloc(1, sizeof(zClass_NodePartial *)));
    if (data->attachedWorlds == nullptr) {
        return 22;
    }

    if (zClass_Light::DeleteNode(node) != 0 || g_zClass_ActiveNodeCount != 0) {
        return 23;
    }
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Light::gwLightNew() == nullptr ? 0 : 24;
}

extern "C" int zclass_damage_handler_smoke() {
    zClass_NodeFreeListSlot rootSlot{};
    zClass_NodeFreeListSlot childSlot{};
    zClass_NodeFreeListSlot grandchildSlot{};
    zClass_NodePartial *rootChildren[] = {&childSlot.node};
    zClass_NodePartial *childChildren[] = {&grandchildSlot.node};
    rootSlot.node.listCountB = 1;
    rootSlot.node.listB = rootChildren;
    childSlot.node.listCountB = 1;
    childSlot.node.listB = childChildren;

    void *hitCallback = reinterpret_cast<void *>(0x11111111);
    void *hitContext = reinterpret_cast<void *>(0x22222222);
    if (zClass_Node::SetDamageHitCallback(hitCallback, &rootSlot.node, hitContext) != 0 ||
        rootSlot.damageHandler == nullptr || childSlot.damageHandler != rootSlot.damageHandler ||
        grandchildSlot.damageHandler != rootSlot.damageHandler ||
        (rootSlot.node.flags & 0x40) == 0) {
        return 1;
    }

    OptCatalogDamageHandlerPartial *handler =
        static_cast<OptCatalogDamageHandlerPartial *>(rootSlot.damageHandler);
    if (handler->hitCallback != hitCallback || handler->hitContext != hitContext) {
        return 2;
    }

    void *replacement = reinterpret_cast<void *>(0x33333333);
    zClass_Node::SetDamageHitCallback(replacement, &rootSlot.node, replacement);
    if (handler->hitCallback != hitCallback || handler->hitContext != hitContext) {
        return 3;
    }

    zClass_Node::ClearDamageHandlerRecursive(&rootSlot.node, handler);
    if (rootSlot.damageHandler != nullptr || childSlot.damageHandler != nullptr ||
        grandchildSlot.damageHandler != nullptr) {
        std::free(handler);
        return 4;
    }
    std::free(handler);
    rootSlot.node.flags = 0;

    float capturedDamage = 0.0f;
    void *timerCallback = reinterpret_cast<void *>(zclass_damage_timer_stub);
    void *timerContext = &capturedDamage;
    if (zClass_Node::SetDamageTimerCallback(timerCallback, &rootSlot.node, timerContext) != 0 ||
        rootSlot.damageHandler == nullptr || childSlot.damageHandler != rootSlot.damageHandler) {
        return 5;
    }
    handler = static_cast<OptCatalogDamageHandlerPartial *>(rootSlot.damageHandler);
    if (handler->timerCallback != timerCallback || handler->timerContext != timerContext) {
        return 6;
    }

    g_OptCatalog_CaptureHitSnapshotEnabled = 1;
    zVec3 sourcePos{1.0f, 2.0f, 3.0f};
    OptCatalogHitEventPartial hitEvent{};
    hitEvent.hitPos = {4.0f, 5.0f, 6.0f};
    hitEvent.hitNode = &rootSlot.node;
    const float timerResult =
        OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback(&sourcePos, &hitEvent, 2.5f);
    if (timerResult != 3.5f || capturedDamage != 2.5f ||
        g_OptCatalog_CapturedDamageSourcePos.x != 1.0f ||
        g_OptCatalog_CapturedDamageSourcePos.y != 2.0f ||
        g_OptCatalog_CapturedDamageSourcePos.z != 3.0f ||
        g_OptCatalog_CapturedDamageHitPos.x != 4.0f ||
        g_OptCatalog_CapturedDamageHitPos.y != 5.0f ||
        g_OptCatalog_CapturedDamageHitPos.z != 6.0f) {
        return 7;
    }
    g_OptCatalog_CaptureHitSnapshotEnabled = 0;

    if (zClass_Node::ClearDamageHandler(&rootSlot.node) != 0 || rootSlot.damageHandler != nullptr ||
        childSlot.damageHandler != nullptr || (rootSlot.node.flags & 0x40) != 0) {
        return 8;
    }

    return zClass_Node::ClearDamageHandler(nullptr) == 0 ? 0 : 9;
}

extern "C" int zclass_camera_view_distance_smoke() {
    if (g_zClass_CameraAutoClipDistanceThreshold != 0.04f || g_zClass_ObjectHseTestEnabled != 1) {
        return 1;
    }

    zClass_Camera::SetViewDistance(1, 20.0f);
    if (g_zClass_CameraAutoClipDistanceAdjustEnabled != 1 ||
        g_zClass_CameraAutoClipDistanceThreshold != 0.05f) {
        return 2;
    }

    zClass_Camera::SetViewDistance(0, 0.0f);
    if (g_zClass_CameraAutoClipDistanceAdjustEnabled != 0 ||
        g_zClass_CameraAutoClipDistanceThreshold != 0.04f) {
        return 3;
    }

    for (int i = 0; i < 16; ++i) {
        zClass_TypeList::Head(i) = nullptr;
        zClass_TypeList::Tail(i) = nullptr;
        zClass_TypeList::PendingRemovalDirty(i) = 0;
    }
    g_zClass_TypeList_FreeLinkHead = nullptr;
    g_zClass_NodeList_PendingFreeHead = nullptr;
    g_zClass_TypeList_LiveLinkCount = 0;
    g_zClass_TypeList_PeakLiveLinkCount = 0;

    zClass_NodeFreeListSlot slot{};
    slot.freeTag = 0x00ffffff;
    g_zClass_NodeArray = &slot;
    g_zClass_NodeFreeHeadIndex = 0;
    g_zClass_ActiveNodeCount = 0;
    g_zClass_DeferredProcessingEnabled = 1;

    zClass_NodePartial *camera = zClass_Camera::gwCameraNew();
    if (camera != &slot.node || camera->classId != 1 || camera->classData == nullptr ||
        zClass_TypeList::Head(8) == nullptr || zClass_TypeList::Head(8)->node != camera) {
        return 4;
    }

    zClass_CameraDataPartial *data = static_cast<zClass_CameraDataPartial *>(camera->classData);
    if (data->viewportWidth != 1.0f || data->viewportHeight != 1.0f ||
        data->frustumVectorsDirty != 1 || data->transformDirty != 1 ||
        data->localFrustumNormalsDirty != 1 || data->variantOverrideEnabled != 0 ||
        data->variantTag.count != 0 || data->variantTag.tags[0] != 0xff ||
        data->variantTag.tags[1] != 0xff || data->variantTag.tags[2] != 0xff) {
        return 5;
    }

    if (zClass_Camera::gwCameraSetFlagBit0(camera, 1) != 0 || (data->cameraFlags & 1) == 0 ||
        zClass_Camera::gwCameraSetFlagBit0(camera, 0) != 0 || (data->cameraFlags & 1) != 0) {
        return 6;
    }

    zClass_NodePartial child{};
    if (zClass_Camera::gwCameraAddChild(camera, &child) != 0 || camera->listCountB != 1 ||
        child.listCountA != 1 || zClass_Camera::gwCameraRemoveChild(camera, &child) != 0 ||
        camera->listCountB != 0 || child.listCountA != 0) {
        return 7;
    }

    zClass_NodePartial world{};
    std::int32_t worldData = 0;
    world.classId = 2;
    world.classData = &worldData;
    if (zClass_Camera::gwCameraSetWorld(camera, &world) != 0 || data->worldNode != &world ||
        zClass_Camera::gwCameraGetWorld(camera) != &world) {
        return 8;
    }

    if (zClass_Camera::gwCameraSetWindow(camera, &child) != 0 || data->windowNode != &child) {
        return 9;
    }

    data->cameraFlags = 0x02;
    child.classId = 5;
    zClass_Object3DDataPartial childData{};
    child.classData = &childData;
    zClass_NodePartial *cameraChildren[] = {&child};
    camera->listCountA = 1;
    camera->listCountB = 1;
    camera->listB = cameraChildren;
    if (zClass_Camera::gwCameraSetPosition(camera, 1.0f, 2.0f, 3.0f) != 0 ||
        data->posOffset.x != 1.0f || data->posOffset.y != 2.0f || data->posOffset.z != 3.0f ||
        (data->cameraFlags & 0x06) != 0x04 || data->transformDirty != 1 ||
        (camera->flags & 0x03) != 0x03 || (child.flags & 0x02000000) == 0) {
        return 10;
    }

    if (zClass_Camera::gwCameraTranslate(camera, 0.5f, -1.0f, 4.0f) != 0 ||
        data->posOffset.x != 1.5f || data->posOffset.y != 1.0f || data->posOffset.z != 7.0f) {
        return 11;
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    if (zClass_Camera::gwCameraGetPosition(camera, &x, &y, &z) != 0 || x != 1.5f || y != 1.0f ||
        z != 7.0f) {
        return 12;
    }

    data->cameraFlags &= ~0x02;
    if (zClass_Camera::gwCameraSetTarget(camera, 8.0f, 9.0f, 10.0f) != 0 ||
        data->targetOrEuler.x != 8.0f || data->targetOrEuler.y != 9.0f ||
        data->targetOrEuler.z != 10.0f) {
        return 13;
    }
    if (zClass_Camera::gwCameraTranslateTarget(camera, 1.0f, 2.0f, 3.0f) != 0 ||
        data->targetOrEuler.x != 9.0f || data->targetOrEuler.y != 11.0f ||
        data->targetOrEuler.z != 13.0f ||
        zClass_Camera::gwCameraGetTarget(camera, &x, &y, &z) != 0 || x != 9.0f || y != 11.0f ||
        z != 13.0f) {
        return 14;
    }

    data->cameraFlags |= 0x02;
    if (zClass_Camera::gwCameraSetTarget(camera, 1.0f, 2.0f, 3.0f) != 0 ||
        zClass_Camera::gwCameraTranslateTarget(camera, 4.0f, 5.0f, 6.0f) != 0 ||
        zClass_Camera::gwCameraGetTarget(camera, &x, &y, &z) != 0 || x != 5.0f || y != 7.0f ||
        z != 9.0f || data->targetOrEuler.x != 9.0f) {
        return 15;
    }

    if (zClass_Camera::gwCameraSetNearFarClip(camera, 0.25f, 500.0f) != 0 ||
        data->nearClip != 0.25f || data->farClip != 500.0f || data->frustumVectorsDirty != 1 ||
        zClass_Camera::gwCameraGetNearFarClip(camera, &x, &y) != 0 || x != 0.25f || y != 500.0f) {
        return 16;
    }

    const float maxCameraFov = 1.39600003f;
    data->frustumWidth = 640.0f;
    data->frustumHeight = 480.0f;
    if (zClass_Camera::gwCameraSetViewport(camera, 320.0f, 200.0f) != 0 ||
        data->viewportWidth != 320.0f || data->viewportHeight != 200.0f ||
        data->fovX != maxCameraFov || data->fovY != maxCameraFov ||
        data->frustumYaw != maxCameraFov * 0.5f || data->frustumPitch != maxCameraFov * 0.5f ||
        std::fabs(data->viewportScaleX - (1.0f / std::tan(static_cast<double>(data->frustumYaw)))) >
            0.0001f ||
        std::fabs(data->viewportScaleY -
                  (1.0f / std::tan(static_cast<double>(data->frustumPitch)))) > 0.0001f ||
        data->localFrustumNormalsDirty != 1 || data->frustumVectorsDirty != 1) {
        return 17;
    }

    if (zClass_Camera::gwCameraGetViewport(camera, &x, &y) != 0 || x != 320.0f || y != 200.0f) {
        return 18;
    }

    if (zClass_Camera::gwCameraGetFOV(camera, &x, &y) != 0 || x != 640.0f || y != 480.0f) {
        return 19;
    }

    if (zClass_Camera::gwCameraSetFOV(camera, 1.0f, 0.5f) != 0 || data->frustumWidth != 1.0f ||
        data->frustumHeight != 0.5f || data->fovX != 1.0f / 320.0f || data->fovY != 0.5f / 200.0f ||
        data->frustumYaw != (1.0f / 320.0f) * 0.5f ||
        data->frustumPitch != (0.5f / 200.0f) * 0.5f ||
        std::fabs(data->viewportScaleX - (1.0f / std::tan(static_cast<double>(data->frustumYaw)))) >
            0.0001f ||
        std::fabs(data->viewportScaleY -
                  (1.0f / std::tan(static_cast<double>(data->frustumPitch)))) > 0.0001f ||
        data->localFrustumNormalsDirty != 1 || data->frustumVectorsDirty != 1) {
        return 20;
    }

    if (zClass_Camera::gwCameraSetClipDistance(camera, 4.0f) != 0 || data->clipDistance != 4.0f ||
        data->invClipDistanceSq != 0.0625f ||
        zClass_Camera::gwCameraGetClipDistance(camera, &x) != 0 || x != 4.0f) {
        return 21;
    }

    if (zClass_Camera::gwCameraSetHorizon(camera, &child) != 0 || data->horizonNode != &child ||
        zClass_Camera::gwCameraSetHorizonXZ(camera, &world) != 0 || data->horizonXZNode != &world) {
        return 22;
    }

    zClass_Camera::SetTargetNode(&child);
    if (g_zClass_CameraTargetNode != &child) {
        return 23;
    }
    if (zClass_Camera::SetActiveCamera(camera) != camera || g_zClass_CurrentCamera != camera) {
        return 24;
    }
    if (zClass_Camera::SetObjectHseTestEnabled(0) != 0 || g_zClass_ObjectHseTestEnabled != 0) {
        return 23;
    }

    zTag4Partial tag{};
    zTag4::Clear(nullptr);
    zTag4::Clear(&tag);
    if (tag.count != 0 || tag.tags[0] != 0xff || tag.tags[1] != 0xff || tag.tags[2] != 0xff) {
        return 24;
    }

    zClass_Object3D::DeleteNode(camera);
    zClass_TypeList::FreeAll();

    g_zClass_NodeFreeHeadIndex = -1;
    return zClass_Camera::gwCameraNew() == nullptr &&
                   zClass_Camera::gwCameraAddChild(nullptr, &child) == 5 &&
                   zClass_Camera::gwCameraRemoveChild(camera, nullptr) == 5 &&
                   zClass_Camera::gwCameraSetFlagBit0(nullptr, 1) == 5 &&
                   zClass_Camera::gwCameraSetFlagBit0(&world, 1) == 3 &&
                   zClass_Camera::gwCameraSetWorld(camera, nullptr) == 5 &&
                   zClass_Camera::gwCameraSetPosition(nullptr, 0.0f, 0.0f, 0.0f) == 5 &&
                   zClass_Camera::gwCameraTranslate(&world, 0.0f, 0.0f, 0.0f) == 3 &&
                   zClass_Camera::gwCameraGetPosition(nullptr, &x, &y, &z) == 5 &&
                   zClass_Camera::gwCameraSetTarget(nullptr, 0.0f, 0.0f, 0.0f) == 5 &&
                   zClass_Camera::gwCameraTranslateTarget(&world, 0.0f, 0.0f, 0.0f) == 3 &&
                   zClass_Camera::gwCameraGetTarget(nullptr, &x, &y, &z) == 5 &&
                   zClass_Camera::gwCameraSetNearFarClip(nullptr, 0.0f, 0.0f) == 5 &&
                   zClass_Camera::gwCameraGetNearFarClip(nullptr, &x, &y) == 5 &&
                   zClass_Camera::gwCameraSetViewport(nullptr, 1.0f, 1.0f) == 5 &&
                   zClass_Camera::gwCameraGetViewport(nullptr, &x, &y) == 5 &&
                   zClass_Camera::gwCameraGetFOV(nullptr, &x, &y) == 5 &&
                   zClass_Camera::gwCameraSetFOV(nullptr, 1.0f, 1.0f) == 5 &&
                   zClass_Camera::gwCameraGetClipDistance(nullptr, &x) == 5 &&
                   zClass_Camera::gwCameraSetClipDistance(nullptr, 1.0f) == 5 &&
                   zClass_Camera::gwCameraSetHorizon(nullptr, &child) == 5 &&
                   zClass_Camera::gwCameraSetHorizonXZ(nullptr, &child) == 5
               ? 0
               : 25;
}
