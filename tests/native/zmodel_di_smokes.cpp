#include "GameZRecoil/zModel/zModel.h"
#include "zDi.h"

#include <cstdint>
#include <cstdlib>

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
    entries[0].drawFlags = 0x77777777;
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
        diClone->entries[0].drawFlags != entries[0].drawFlags ||
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

extern "C" int zmodel_set_di_texture_world_per_meter_smoke() {
    g_zModel_TextureWorldBaseU = 0.0f;
    g_zModel_TextureWorldBaseV = 0.0f;
    g_zModel_TextureWorldPerMeterU = 0.0f;
    g_zModel_TextureWorldPerMeterV = 0.0f;

    zModel::SetTextureWorldBase(1.25f, -3.5f);
    if (g_zModel_TextureWorldBaseU != 1.25f || g_zModel_TextureWorldBaseV != -3.5f ||
        g_zModel_TextureWorldPerMeterU != 0.0f || g_zModel_TextureWorldPerMeterV != 0.0f) {
        return 1;
    }

    zModel::SetTextureWorldPerMeter(0.5f, 4.75f);
    if (g_zModel_TextureWorldBaseU != 1.25f || g_zModel_TextureWorldBaseV != -3.5f ||
        g_zModel_TextureWorldPerMeterU != 0.5f || g_zModel_TextureWorldPerMeterV != 4.75f) {
        return 2;
    }

    zDiPartial di{};
    di.flags = 0x73;

    if (zModel::SetDiTextureWorldPerMeter(nullptr, 1, 4.0f, 2) != 1) {
        return 3;
    }

    if (zModel::SetDiTextureWorldPerMeter(&di, 0, 2.5f, 7) != 0 ||
        di.flags != 0x53 || di.textureWorldPerMeter != 2.5f || di.textureWorldAxis != 7) {
        return 4;
    }

    if (zModel::SetDiTextureWorldPerMeter(&di, 3, 8.0f, -4) != 0 ||
        di.flags != 0x73 || di.textureWorldPerMeter != 8.0f || di.textureWorldAxis != -4) {
        return 5;
    }

    return 0;
}
