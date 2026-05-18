#include "GameZRecoil/zDEClient/zdec.h"

#include "GameZRecoil/zModel/zModel.h"
#include "zDi.h"

#include <cstdlib>
#include <cstring>
#include <new>

namespace {
zDEClient_MapTreeNode *AllocTreeNode() {
    auto *node =
        static_cast<zDEClient_MapTreeNode *>(::operator new(sizeof(zDEClient_MapTreeNode)));
    *node = {};
    return node;
}

void ReleaseTreeSentinels() {
    ::operator delete(g_zDEClient_FeatureMapTree.header);
    ::operator delete(g_zDEClient_FeatureMapTreeNil);
    g_zDEClient_FeatureMapTree = {};
    g_zDEClient_FeatureMapTreeNil = nullptr;
}

void SetupSingleNodeFeatureTree() {
    zDEClient_MapTreeNode *const nil = AllocTreeNode();
    nil->left = nil;
    nil->parent = nil;
    nil->right = nil;
    nil->colorOrNil = 1;

    zDEClient_MapTreeNode *const header = AllocTreeNode();
    zDEClient_MapTreeNode *const root = AllocTreeNode();

    root->left = nil;
    root->parent = header;
    root->right = nil;
    root->colorOrNil = 1;

    header->left = root;
    header->parent = root;
    header->right = root;
    header->colorOrNil = 1;

    g_zDEClient_FeatureMapTreeNil = nil;
    g_zDEClient_FeatureMapTree.header = header;
    g_zDEClient_FeatureMapTree.nodeCount = 1;
}
} // namespace

extern "C" int zdeclient_set_camera_node_smoke(void) {
    zClass_CameraDataPartial cameraData = {};
    zClass_NodePartial cameraNode = {};
    cameraNode.classData = &cameraData;

    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    zDEClient::SetCameraNode(nullptr);
    if (g_zDEClient_CameraNode != nullptr || g_zDEClient_CameraNodeClassData != nullptr) {
        return 1;
    }

    zDEClient::SetCameraNode(&cameraNode);
    const bool ok =
        g_zDEClient_CameraNode == &cameraNode && g_zDEClient_CameraNodeClassData == &cameraData;

    g_zDEClient_CameraNode = nullptr;
    g_zDEClient_CameraNodeClassData = nullptr;
    return ok ? 0 : 2;
}

extern "C" int zdeclient_load_material_from_texture_path_smoke(void) {
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 1;
    std::strcpy(g_zImage_TexDirEntries[0].baseName, "stone");
    g_zImage_TexDirEntries[0].loadState = 2;

    zModel_MaterialSlot reuseSlots[1] = {};
    reuseSlots[0].material.currentTextureDirectoryEntry = &g_zImage_TexDirEntries[0];
    reuseSlots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = reuseSlots;
    g_zModel_MatlActiveHeadIndex = 0;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlReuseCache = nullptr;

    zModel_MaterialPartial *material = nullptr;
    char existingPath[] = "stone";
    const bool reuseOk =
        zDEClient::LoadMaterialFromTexturePath_Local(&material, existingPath) == 0 &&
        material == &reuseSlots[0].material && g_zImage_TexDirEntryCount == 1;

    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zImage_TexDirEntryCount = 0;
    zModel_MaterialSlot cloneSlots[1] = {};
    cloneSlots[0].prevPoolIndex = -1;
    cloneSlots[0].nextPoolIndex = -1;
    g_zModel_MatlPool = cloneSlots;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlFreeHeadIndex = 0;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlReuseCache = nullptr;

    material = nullptr;
    char newPath[] = "newtex.tga";
    const bool appendOk = zDEClient::LoadMaterialFromTexturePath_Local(&material, newPath) == 1 &&
                          material == &cloneSlots[0].material && (material->flags & 0x0100) != 0 &&
                          material->currentTextureDirectoryEntry == &g_zImage_TexDirEntries[0] &&
                          g_zImage_TexDirEntries[0].loadState == 2 &&
                          g_zImage_TexDirEntryCount == 1 && g_zModel_MatlActiveHeadIndex == 0 &&
                          g_zModel_MatlFreeHeadIndex == -1;

    g_zImage_TexDirEntryCount = 0;
    std::memset(g_zImage_TexDirEntries, 0, sizeof(g_zImage_TexDirEntries));
    g_zModel_MatlPool = nullptr;
    g_zModel_MatlActiveHeadIndex = -1;
    g_zModel_MatlFreeHeadIndex = -1;
    g_zModel_MatlPoolInUseCount = 0;
    g_zModel_MatlReuseCache = nullptr;

    return reuseOk && appendOk ? 0 : 1;
}

extern "C" int zdeclient_shutdown_globals_smoke(void) {
    g_zDEClient_QuickSandEnabled = 0;
    if (zDEClient::ShutdownGlobals() != 0) {
        return 1;
    }

    SetupSingleNodeFeatureTree();
    auto *entries =
        static_cast<zDEClient_FeatureEntry *>(std::malloc(sizeof(zDEClient_FeatureEntry) * 2));
    g_zDEClient_FeatureListBegin = entries;
    g_zDEClient_FeatureListEnd = entries + 1;
    g_zDEClient_FeatureListCapacityEnd = entries + 2;

    if (zDEClient::ClearFeatureEntriesAndMapTree() != 0 ||
        g_zDEClient_FeatureListEnd != g_zDEClient_FeatureListBegin ||
        g_zDEClient_FeatureMapTree.nodeCount != 0 ||
        g_zDEClient_FeatureMapTree.header->parent != g_zDEClient_FeatureMapTreeNil ||
        g_zDEClient_FeatureMapTree.header->left != g_zDEClient_FeatureMapTree.header ||
        g_zDEClient_FeatureMapTree.header->right != g_zDEClient_FeatureMapTree.header) {
        std::free(entries);
        ReleaseTreeSentinels();
        return 2;
    }

    std::free(entries);
    ReleaseTreeSentinels();

    SetupSingleNodeFeatureTree();
    entries = static_cast<zDEClient_FeatureEntry *>(std::malloc(sizeof(zDEClient_FeatureEntry)));
    g_zDEClient_FeatureListBegin = entries;
    g_zDEClient_FeatureListEnd = entries + 1;
    g_zDEClient_FeatureListCapacityEnd = entries + 1;
    g_zDEClient_QuickSandEnabled = 1;
    g_zDEClient_QuickSandTexturePaths = static_cast<char **>(std::malloc(sizeof(char *)));
    g_zDEClient_CraterDisplaySourceList = static_cast<zDEClient_CraterDisplaySourceEntry *>(
        std::malloc(sizeof(zDEClient_CraterDisplaySourceEntry)));

    const int result = zDEClient::ShutdownGlobals();
    const bool ok = result == 0 && g_zDEClient_QuickSandEnabled == 0 &&
                    g_zDEClient_QuickSandTexturePaths == nullptr &&
                    g_zDEClient_CraterDisplaySourceList == nullptr &&
                    g_zDEClient_FeatureMapTree.nodeCount == 0;

    std::free(entries);
    ReleaseTreeSentinels();
    g_zDEClient_FeatureListBegin = nullptr;
    g_zDEClient_FeatureListEnd = nullptr;
    g_zDEClient_FeatureListCapacityEnd = nullptr;

    return ok ? 0 : 3;
}
