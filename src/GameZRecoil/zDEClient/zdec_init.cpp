#include "zdec.h"

#include "Battlesport/game_net.h"
#include "Battlesport/player.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zNetwork/znet.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if defined(_MSC_VER) && _MSC_VER < 1200 && defined(_M_IX86)
#include <yvals.h>
#endif

/**
 * Reimplements data 0x539d10: g_zDEClient_QuickSandEnabled.
 * Purpose: Tracks whether quicksand runtime globals were initialized and need
 * shutdown cleanup.
 */
int g_zDEClient_QuickSandEnabled = 0;
/**
 * Reimplements data 0x539d14: g_zDEClient_QuickSandTextureCount.
 * Purpose: Stores the configured quicksand texture-cycle path count.
 */
int g_zDEClient_QuickSandTextureCount = 0;
/**
 * Reimplements data 0x539d18: g_zDEClient_QuickSandAnimSpeed.
 * Purpose: Stores the quicksand material cycle animation speed.
 */
float g_zDEClient_QuickSandAnimSpeed = 0.0f;
/**
 * Reimplements data 0x539d1c: g_zDEClient_QuickSandTexturePaths.
 * Purpose: Owns the quicksand texture path pointer array allocated from
 * declient.zrd configuration.
 */
char **g_zDEClient_QuickSandTexturePaths = 0;
/**
 * Reimplements data 0x539d24: g_zDEClient_QuickSandMaterial.
 * Purpose: Caches the quicksand material used by generated terrain features.
 */
zModel_MaterialPartial *g_zDEClient_QuickSandMaterial = 0;
/**
 * Reimplements data 0x539d28: g_zDEClient_QuickSandMaterialCycle.
 * Purpose: Caches the quicksand cycle material used while feature textures
 * animate.
 */
zModel_MaterialPartial *g_zDEClient_QuickSandMaterialCycle = 0;
zDEClient_QSandEventTemplate g_zDEClient_QuickSandEventTemplateDefaults = {0};
/**
 * Reimplements data 0x539ce0: g_zDEClient_CraterDisplaySourceCount.
 * Purpose: Stores g zDEClient CraterDisplaySourceCount data used by engine.zeffect.zdeclient_crater_display_source_globals.
 */
int g_zDEClient_CraterDisplaySourceCount = 0;
/**
 * Reimplements data 0x539ce4: g_zDEClient_CraterDisplaySourceList.
 * Purpose: Stores g zDEClient CraterDisplaySourceList data used by engine.zeffect.zdeclient_crater_display_source_globals.
 */
zDEClient_CraterDisplaySourceEntry *g_zDEClient_CraterDisplaySourceList = 0;
/**
 * Reimplements data 0x539ce8: g_zDEClient_CraterEventTemplateDefaults.
 * Purpose: Stores g zDEClient CraterEventTemplateDefaults data used by engine.zeffect.zdeclient_crater_event_template_defaults.
 */
zDEClient_CraterEventTemplate g_zDEClient_CraterEventTemplateDefaults = {0};
/**
 * Reimplements data 0x539de0: g_zDEClient_ConfigReaderRoot.
 * Purpose: Holds the transient declient.zrd reader tree while config
 * resources are loaded.
 */
zReader::Node *g_zDEClient_ConfigReaderRoot = 0;
/**
 * Reimplements data 0x4df3c4: g_zDEClient_RebuildBltRectOnReload.
 * Purpose: Enables zDEClient ZAR reload handler registration after config
 * load completes.
 */
int g_zDEClient_RebuildBltRectOnReload = 1;
/**
 * Reimplements data 0x4df3c8: g_zDEClient_NodeName.
 * Purpose: Names the zDEClient ZAR section handler registered after config
 * loading.
 */
char g_zDEClient_NodeName[] = "zDEClient";
/**
 * Reimplements data 0x4df3d4: g_zDEClient_QuickSandUntexturedMsg.
 * Purpose: Reports the quicksand fallback when no texture path was configured.
 */
char g_zDEClient_QuickSandUntexturedMsg[] = "Quick sand will NOT be textured";
/**
 * Reimplements data 0x4df3f4: g_zDEClient_QuickSandNodeName.
 * Purpose: Names the QUICK_SAND config node in declient.zrd.
 */
char g_zDEClient_QuickSandNodeName[] = "QUICK_SAND";
/**
 * Reimplements data 0x4df400: g_zDEClient_TextureAnimNodeName.
 * Purpose: Names the crater texture animation config node.
 */
char g_zDEClient_TextureAnimNodeName[] = "TEXTURE_ANIM";
/**
 * Reimplements data 0x4df410: g_zDEClient_DefaultAnimNodeName.
 * Purpose: Names the default crater animation field.
 */
char g_zDEClient_DefaultAnimNodeName[] = "DEFAULT_ANIM";
/**
 * Reimplements data 0x4df420: g_zDEClient_DefaultTextureNodeName.
 * Purpose: Names the default crater/quicksand texture field.
 */
char g_zDEClient_DefaultTextureNodeName[] = "DEFAULT_TEXTURE";
/**
 * Reimplements data 0x4df430: g_zDEClient_RadiusFieldName.
 * Purpose: Names the feature radius config field.
 */
char g_zDEClient_RadiusFieldName[] = "RADIUS";
/**
 * Reimplements data 0x4df438: g_zDEClient_DepthFieldName.
 * Purpose: Names the feature depth config field.
 */
char g_zDEClient_DepthFieldName[] = "DEPTH";
/**
 * Reimplements data 0x4df440: g_zDEClient_SlopeFieldName.
 * Purpose: Names the feature slope config field.
 */
char g_zDEClient_SlopeFieldName[] = "SLOPE";
/**
 * Reimplements data 0x4df448: g_zDEClient_PointsFieldName.
 * Purpose: Names the feature point-count config field.
 */
char g_zDEClient_PointsFieldName[] = "POINTS";
/**
 * Reimplements data 0x4df450: g_zDEClient_CraterNodeName.
 * Purpose: Names the CRATER config node in declient.zrd.
 */
char g_zDEClient_CraterNodeName[] = "CRATER";
/**
 * Reimplements data 0x4df458: g_zDEClient_ReadDefaultsFallbackFmt.
 * Purpose: Formats the declient.zrd load warning that keeps built-in
 * defaults.
 */
char g_zDEClient_ReadDefaultsFallbackFmt[] = "Failed to read (%s), using defaults";
/**
 * Reimplements data 0x4df47c: g_zDEClient_ConfigArchiveName.
 * Purpose: Names the zDEClient config archive loaded at startup.
 */
char g_zDEClient_ConfigArchiveName[] = "declient.zrd";
/**
 * Reimplements data 0x4df48c: g_zDEClient_SourceFile_ZdecInitCpp.
 * Purpose: Provides the original source path for zDEClient config diagnostics.
 */
char g_zDEClient_SourceFile_ZdecInitCpp[] =
    "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_init.cpp";
/**
 * Reimplements data 0x4df4b8: g_zDEClient_WorldNodeNullErrorMsg.
 * Purpose: Reports a missing world node during zDEClient config loading.
 */
char g_zDEClient_WorldNodeNullErrorMsg[] = "Failed to DEClient: world node is NULL.";
/**
 * Reimplements data 0x4df4e0: g_zDEClient_QuickSandInstanceTessellationFailedMsg.
 * Purpose: Reports quicksand instancing failure when tessellation fails.
 */
char g_zDEClient_QuickSandInstanceTessellationFailedMsg[] =
    "Failed to instance quick sand: Tesselation Failed";
/**
 * Reimplements data 0x4df514: g_zDEClient_QuickSandInstanceClipFailedMsg.
 * Purpose: Reports quicksand instancing failure when feature clipping fails.
 */
char g_zDEClient_QuickSandInstanceClipFailedMsg[] =
    "Failed to instance quick sand: Clip Failed";
/**
 * Reimplements data 0x4df540: g_zDEClient_SourceFile_ZdecQsandCpp.
 * Purpose: Provides the original source path for quicksand feature diagnostics.
 */
char g_zDEClient_SourceFile_ZdecQsandCpp[] =
    "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_qsand.cpp";
/**
 * Reimplements data 0x4df570: g_zDEClient_QuickSandInstanceBuildFailedMsg.
 * Purpose: Reports quicksand instancing failure when display construction fails.
 */
char g_zDEClient_QuickSandInstanceBuildFailedMsg[] =
    "Failed to instance quick sand: Build Failed";
/**
 * Reimplements data 0x4df59c: g_zDEClient_FeatureNodeName.
 * Purpose: Names generated zDEClient terrain feature nodes.
 */
char g_zDEClient_FeatureNodeName[] = "ZDEC_FEATURE";
/**
 * Reimplements data 0x4df5ac: g_zDEClient_CraterInstanceTessellationFailedMsg.
 * Purpose: Reports crater instancing failure when tessellation fails.
 */
char g_zDEClient_CraterInstanceTessellationFailedMsg[] =
    "Failed to instance crater: Tesselation Failed";
/**
 * Reimplements data 0x4df5dc: g_zDEClient_CraterInstanceClipFailedMsg.
 * Purpose: Reports crater instancing failure when feature clipping fails.
 */
char g_zDEClient_CraterInstanceClipFailedMsg[] =
    "Failed to instance crater: Clip Failed";
/**
 * Reimplements data 0x4df604: g_zDEClient_SourceFile_ZdecCraterCpp.
 * Purpose: Provides the original source path for crater feature diagnostics.
 */
char g_zDEClient_SourceFile_ZdecCraterCpp[] =
    "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_crater.cpp";
/**
 * Reimplements data 0x4df634: g_zDEClient_CraterInstanceBuildFailedMsg.
 * Purpose: Reports crater instancing failure when display construction fails.
 */
char g_zDEClient_CraterInstanceBuildFailedMsg[] =
    "Failed to instance crater: Build Failed";
/**
 * Reimplements data 0x4df65c: g_zDEClient_CraterNameFmt.
 * Purpose: Formats saved crater feature section names.
 */
char g_zDEClient_CraterNameFmt[] = "Crater%d";
/**
 * Reimplements data 0x4df668: g_zDEClient_QuickSandNameFmt.
 * Purpose: Formats saved quicksand feature section names.
 */
char g_zDEClient_QuickSandNameFmt[] = "QSand%d";
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_NodeName) == 0x0a);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandUntexturedMsg) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandNodeName) == 0x0b);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_TextureAnimNodeName) == 0x0d);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_DefaultAnimNodeName) == 0x0d);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_DefaultTextureNodeName) == 0x10);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_RadiusFieldName) == 0x07);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_DepthFieldName) == 0x06);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_SlopeFieldName) == 0x06);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_PointsFieldName) == 0x07);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterNodeName) == 0x07);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_ReadDefaultsFallbackFmt) == 0x24);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_ConfigArchiveName) == 0x0d);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_SourceFile_ZdecInitCpp) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_WorldNodeNullErrorMsg) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandInstanceTessellationFailedMsg) == 0x32);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandInstanceClipFailedMsg) == 0x2b);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_SourceFile_ZdecQsandCpp) == 0x2d);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandInstanceBuildFailedMsg) == 0x2c);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_FeatureNodeName) == 0x0d);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterInstanceTessellationFailedMsg) == 0x2e);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterInstanceClipFailedMsg) == 0x27);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_SourceFile_ZdecCraterCpp) == 0x2e);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterInstanceBuildFailedMsg) == 0x28);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_CraterNameFmt) == 0x09);
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_QuickSandNameFmt) == 0x08);
/**
 * Reimplements data 0x539df0: g_zDEClient_FeatureListFlags.
 * Purpose: Stores the feature entry vector initialization flag byte.
 */
int g_zDEClient_FeatureListFlags = 0;
/**
 * Reimplements data 0x539df4: g_zDEClient_FeatureListBegin.
 * Purpose: Marks the beginning of the zDEClient feature-entry vector.
 */
zDEClient_FeatureEntry *g_zDEClient_FeatureListBegin = 0;
/**
 * Reimplements data 0x539df8: g_zDEClient_FeatureListEnd.
 * Purpose: Marks the active end of the zDEClient feature-entry vector.
 */
zDEClient_FeatureEntry *g_zDEClient_FeatureListEnd = 0;
/**
 * Reimplements data 0x539dfc: g_zDEClient_FeatureListCapacityEnd.
 * Purpose: Marks the allocated capacity end of the zDEClient feature-entry
 * vector.
 */
zDEClient_FeatureEntry *g_zDEClient_FeatureListCapacityEnd = 0;
/**
 * Reimplements data 0x539e00: g_zDEClient_FeatureMapTree.
 * Purpose: Owns the map-tree index from feature display nodes to their
 * generated display-instance pairs.
 */
zDEClient_MapTreeState g_zDEClient_FeatureMapTree = {0};
/**
 * Reimplements data 0x539e10: g_zDEClient_FeatureMapTreeNil.
 * Purpose: Stores the shared nil sentinel for zDEClient map-tree instances.
 */
zDEClient_MapTreeNode *g_zDEClient_FeatureMapTreeNil = 0;
/**
 * Reimplements data 0x539e14: g_zDEClient_FeatureMapTreeNilRefCount.
 * Purpose: Reference-counts the shared map-tree nil sentinel.
 */
int g_zDEClient_FeatureMapTreeNilRefCount = 0;
/**
 * Reimplements data 0x539e18: g_zDEClient_CameraNode.
 * Purpose: Stores g zDEClient CameraNode data used by engine.zeffect.zdeclient_camera_globals.
 */
zClass_NodePartial *g_zDEClient_CameraNode = 0;
/**
 * Reimplements data 0x539e1c: g_zDEClient_CameraNodeClassData.
 * Purpose: Stores g zDEClient CameraNodeClassData data used by engine.zeffect.zdeclient_camera_globals.
 */
zClass_CameraDataPartial *g_zDEClient_CameraNodeClassData = 0;
/**
 * Reimplements data 0x539de4: g_zDEClientQSandNetRelayCallback.
 * Purpose: Stores g zDEClientQSandNetRelayCallback data used by engine.zeffect.zdeclient_net_relay_callback_globals.
 */
zDEClient_NetRelayCallback g_zDEClientQSandNetRelayCallback = 0;
/**
 * Reimplements data 0x539de8: g_zDEClientCraterNetRelayCallback.
 * Purpose: Stores g zDEClientCraterNetRelayCallback data used by engine.zeffect.zdeclient_net_relay_callback_globals.
 */
zDEClient_NetRelayCallback g_zDEClientCraterNetRelayCallback = 0;

namespace {
template <typename T>
/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed caller is 0x4558f0
 * zDEClient::LoadConfigResources where typed ZAR callbacks are passed through
 * raw section callback slots.
 * Purpose: preserve VC5 function-pointer storage while registering zDEClient
 * ZAR section callbacks.
 */
zZbdSectionCallback ZbdCallbackPtr(
    T callback
) {
    RECOIL_STATIC_ASSERT(sizeof(T) == sizeof(zZbdSectionCallback));
    union {
        T callback;
        zZbdSectionCallback raw;
    } value = {0};
    value.callback = callback;
    return value.raw;
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed callers are zDEClient
 * map-tree methods 0x457d90, 0x457e80, 0x457fe0, 0x458510, and 0x4585a0.
 * Purpose: test for the shared nil sentinel used by the zDEClient map tree.
 */
bool IsNil(
    const zDEClient_MapTreeNode *node
) {
    return node == 0 || node == g_zDEClient_FeatureMapTreeNil;
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed callers are 0x457d90,
 * 0x457e80, 0x457fe0, 0x458510, and 0x4585a0.
 * Purpose: return the leftmost non-nil node in a feature map-tree subtree.
 */
zDEClient_MapTreeNode *TreeMinimum(
    zDEClient_MapTreeNode *node
) {
    while (!IsNil(node->left)) {
        node = node->left;
    }

    return node;
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed callers are 0x457d90,
 * 0x457e80, 0x457fe0, 0x458510, and 0x4585a0.
 * Purpose: return the rightmost non-nil node in a feature map-tree subtree.
 */
zDEClient_MapTreeNode *TreeMaximum(
    zDEClient_MapTreeNode *node
) {
    while (!IsNil(node->right)) {
        node = node->right;
    }

    return node;
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed caller is 0x4585a0
 * zDEClient_MapTreeState::InsertAt.
 * Purpose: rotate a feature map-tree branch left during insertion fixup.
 */
void RotateTreeLeft(
    zDEClient_MapTreeState *tree,
    zDEClient_MapTreeNode *node
) {
    zDEClient_MapTreeNode *const pivot = node->right;
    node->right = pivot->left;
    if (!IsNil(pivot->left)) {
        pivot->left->parent = node;
    }

    pivot->parent = node->parent;
    if (node == tree->header->parent) {
        tree->header->parent = pivot;
    } else if (node == node->parent->left) {
        node->parent->left = pivot;
    } else {
        node->parent->right = pivot;
    }

    pivot->left = node;
    node->parent = pivot;
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed caller is 0x4585a0
 * zDEClient_MapTreeState::InsertAt.
 * Purpose: rotate a feature map-tree branch right during insertion fixup.
 */
void RotateTreeRight(
    zDEClient_MapTreeState *tree,
    zDEClient_MapTreeNode *node
) {
    zDEClient_MapTreeNode *const pivot = node->left;
    node->left = pivot->right;
    if (!IsNil(pivot->right)) {
        pivot->right->parent = node;
    }

    pivot->parent = node->parent;
    if (node == tree->header->parent) {
        tree->header->parent = pivot;
    } else if (node == node->parent->right) {
        node->parent->right = pivot;
    } else {
        node->parent->left = pivot;
    }

    pivot->right = node;
    node->parent = pivot;
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed callers are map-tree
 * erase and initialization paths in this source file.
 * Purpose: reset the map-tree header to the empty-tree state.
 */
void ResetHeader(
    zDEClient_MapTreeState *tree
) {
    if (tree->header == 0) {
        return;
    }

    tree->header->parent = g_zDEClient_FeatureMapTreeNil;
    tree->header->left = tree->header;
    tree->header->right = tree->header;
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed caller is 0x457fe0
 * zDEClient_MapTreeState::EraseAndAdvance.
 * Purpose: replace one map-tree node link with another during erase.
 */
void Transplant(
    zDEClient_MapTreeState *tree,
    zDEClient_MapTreeNode *oldNode,
    zDEClient_MapTreeNode *newNode
) {
    if (oldNode->parent == tree->header) {
        tree->header->parent = newNode;
    } else if (oldNode == oldNode->parent->left) {
        oldNode->parent->left = newNode;
    } else {
        oldNode->parent->right = newNode;
    }

    if (!IsNil(newNode)) {
        newNode->parent = oldNode->parent;
    }
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed callers are map-tree
 * erase paths 0x457e80 and 0x457fe0.
 * Purpose: refresh cached leftmost and rightmost header links after erase.
 */
void RefreshHeaderExtents(
    zDEClient_MapTreeState *tree
) {
    zDEClient_MapTreeNode *const root = tree->header != 0 ? tree->header->parent : 0;
    if (tree->nodeCount <= 0 || IsNil(root)) {
        ResetHeader(tree);
        return;
    }

    tree->header->left = TreeMinimum(root);
    tree->header->right = TreeMaximum(root);
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed caller is 0x457d90
 * zDEClient_MapTreeState::FindOrInsertKey.
 * Purpose: lazily allocate the feature map-tree header and shared nil node.
 */
void EnsureFeatureMapTreeInitialized(
    zDEClient_MapTreeState *tree
) {
    if (g_zDEClient_FeatureMapTreeNil == 0) {
        g_zDEClient_FeatureMapTreeNil =
            (zDEClient_MapTreeNode *)(::operator new(sizeof(zDEClient_MapTreeNode)));
        g_zDEClient_FeatureMapTreeNil->left = g_zDEClient_FeatureMapTreeNil;
        g_zDEClient_FeatureMapTreeNil->parent = g_zDEClient_FeatureMapTreeNil;
        g_zDEClient_FeatureMapTreeNil->right = g_zDEClient_FeatureMapTreeNil;
        g_zDEClient_FeatureMapTreeNil->key = 0;
        g_zDEClient_FeatureMapTreeNil->colorOrNil = 1;
        g_zDEClient_FeatureMapTreeNilRefCount = 1;
    }

    if (tree->header == 0) {
        tree->header = (zDEClient_MapTreeNode *)(::operator new(sizeof(zDEClient_MapTreeNode)));
        tree->header->left = tree->header;
        tree->header->parent = g_zDEClient_FeatureMapTreeNil;
        tree->header->right = tree->header;
        tree->header->key = 0;
        tree->header->colorOrNil = 0;
        tree->allowInsert = 0;
        tree->nodeCount = 0;
    }
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed caller is 0x4558f0
 * zDEClient::LoadConfigResources.
 * Purpose: read the element count stored at the front of a zReader array node.
 */
inline int zReaderArrayCount(
    zReader::Node *node
) {
    return node->value.nodes[0].value.i32;
}

/**
 * Recovered original inlined helper for zdec_init.cpp.
 * No standalone retail function is present; observed caller is 0x4558f0
 * zDEClient::LoadConfigResources.
 * Purpose: read a string element from a zReader array node.
 */
inline char *zReaderArrayString(
    zReader::Node *node,
    int index
) {
    return node->value.nodes[index].value.str;
}
} // namespace

namespace zDEClient_Crater {
/* Source-file block layout: the current native build still compiles this compatibility container.
 * The included fragment files below hold the ledger physical source rows.
 */
/**
 * Reimplements 0x4558f0: zDEClient::LoadConfigResources.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: load declient.zrd crater and quicksand resource defaults, bind the
 * active camera, and register feature reload callbacks.
 */
int __fastcall LoadConfigResources(
    zClass_NodePartial *worldNode
) {
    int textureLoadPending = 0;
    if (worldNode == 0) {
        zError::ReportOld(
            0x200,
            g_zDEClient_SourceFile_ZdecInitCpp,
            0x44,
            g_zDEClient_WorldNodeNullErrorMsg
        );
        return -1;
    }

    zGame::ReturnOnlyStub();
    SetCameraNode(worldNode);
    zVideo::ReturnSuccessStub();

    g_zDEClient_ConfigReaderRoot = zReader::LoadNodeFromPath(
        g_zDEClient_ConfigArchiveName,
        0,
        0
    );
    srand((unsigned int)(time(0)));

    if (g_zDEClient_ConfigReaderRoot == 0) {
        zError::ReportOld(
            0x100,
            g_zDEClient_SourceFile_ZdecInitCpp,
            0x57,
            g_zDEClient_ReadDefaultsFallbackFmt,
            g_zDEClient_ConfigArchiveName
        );
    }

    zReader::Node *const craterNode = zReader_GetNamedNode(
        g_zDEClient_ConfigReaderRoot,
        g_zDEClient_CraterNodeName
    );
    g_zDEClient_CraterEventTemplateDefaults.featureFlags = 0x100c;
    g_zDEClient_CraterEventTemplateDefaults.pointCount = 7;
    g_zDEClient_CraterEventTemplateDefaults.slope = 0.0f;
    g_zDEClient_CraterEventTemplateDefaults.depth = 4.0f;
    g_zDEClient_CraterEventTemplateDefaults.radius = 20.0f;

    zReader::ReadNamedInt(
        craterNode,
        g_zDEClient_PointsFieldName,
        &g_zDEClient_CraterEventTemplateDefaults.pointCount
    );
    zReader::ReadNamedFloat(
        craterNode,
        g_zDEClient_SlopeFieldName,
        &g_zDEClient_CraterEventTemplateDefaults.slope
    );
    zReader::ReadNamedFloat(
        craterNode,
        g_zDEClient_DepthFieldName,
        &g_zDEClient_CraterEventTemplateDefaults.depth
    );
    zReader::ReadNamedFloat(
        craterNode,
        g_zDEClient_RadiusFieldName,
        &g_zDEClient_CraterEventTemplateDefaults.radius
    );

    g_zDEClient_CraterDisplaySourceCount = 1;
    g_zDEClient_CraterDisplaySourceList = (zDEClient_CraterDisplaySourceEntry *)(calloc(
        1,
        sizeof(zDEClient_CraterDisplaySourceEntry)
    ));

    zDEClient_CraterDisplaySourceEntry *defaultDisplaySource = g_zDEClient_CraterDisplaySourceList;
    if (LoadMaterialFromTexturePath_Local(
            &defaultDisplaySource->craterMaterial,
            (char *)(zReader::ReadNamedString(
                craterNode,
                g_zDEClient_DefaultTextureNodeName
            ))
        ) != 0) {
        textureLoadPending = 1;
    }

    defaultDisplaySource->effectAnimEntry =
        zEffectAnim::FindEntryByName(zReader::ReadNamedString(
            craterNode,
            g_zDEClient_DefaultAnimNodeName
        ));

    zReader::Node *const textureAnimNode = zReader_GetNamedNode(
        craterNode,
        g_zDEClient_TextureAnimNodeName
    );
    if (textureAnimNode != 0) {
        const int additionalDisplaySourceCount = (zReaderArrayCount(textureAnimNode) - 1) / 2;
        g_zDEClient_CraterDisplaySourceCount += additionalDisplaySourceCount;

        g_zDEClient_CraterDisplaySourceList = (zDEClient_CraterDisplaySourceEntry *)(realloc(
            g_zDEClient_CraterDisplaySourceList,
            (size_t)(g_zDEClient_CraterDisplaySourceCount) *
                sizeof(zDEClient_CraterDisplaySourceEntry)
        ));

        zDEClient_CraterDisplaySourceEntry *displaySource = &g_zDEClient_CraterDisplaySourceList[1];
        for (int i = 1; i < zReaderArrayCount(textureAnimNode); i += 2) {
            if (LoadMaterialFromTexturePath_Local(
                    &displaySource->sourceMaterial,
                    zReaderArrayString(textureAnimNode, i)
                ) != 0) {
                textureLoadPending = 1;
            }

            zReader::Node *const entryNode =
                zReader_GetNamedNode(
                    textureAnimNode,
                    zReaderArrayString(textureAnimNode, i)
                );
            if (entryNode != 0) {
                if (LoadMaterialFromTexturePath_Local(
                        &displaySource->craterMaterial,
                        zReaderArrayString(entryNode, 1)
                    ) != 0) {
                    textureLoadPending = 1;
                }

                if (zReaderArrayCount(entryNode) > 2) {
                    displaySource->effectAnimEntry =
                        zEffectAnim::FindEntryByName(zReaderArrayString(
                            entryNode,
                            2
                        ));
                } else {
                    displaySource->effectAnimEntry =
                        g_zDEClient_CraterDisplaySourceList[0].effectAnimEntry;
                }
            }

            ++displaySource;
        }
    }

    zReader::Node *const quickSandNode =
        zReader_GetNamedNode(
            g_zDEClient_ConfigReaderRoot,
            g_zDEClient_QuickSandNodeName
        );
    if (quickSandNode != 0) {
        zReader::Node *const defaultTextureNode =
            zReader_GetNamedNode(
                quickSandNode,
                g_zDEClient_DefaultTextureNodeName
            );
        int textureCount = 1;
        if (defaultTextureNode != 0) {
            g_zDEClient_QuickSandAnimSpeed = defaultTextureNode->value.nodes[1].value.f32;
            textureCount = defaultTextureNode->value.nodes[0].value.i32 - 2;
            g_zDEClient_QuickSandTextureCount = textureCount;

            if (textureCount > 0) {
                g_zDEClient_QuickSandTexturePaths =
                    (char **)(malloc((size_t)(textureCount) * sizeof(char *)));
                for (int i = 0; i < g_zDEClient_QuickSandTextureCount; ++i) {
                    g_zDEClient_QuickSandTexturePaths[i] =
                        defaultTextureNode->value.nodes[i + 2].value.str;
                }
            } else {
                g_zDEClient_QuickSandTexturePaths = 0;
            }
        }

        g_zDEClient_QuickSandEventTemplateDefaults.featureFlags = 0x1008;
        g_zDEClient_QuickSandMaterial = 0;
        g_zDEClient_QuickSandMaterialCycle = 0;
        g_zDEClient_QuickSandEventTemplateDefaults.pointCount = 7;
        g_zDEClient_QuickSandEventTemplateDefaults.slope = 40.0f;
        g_zDEClient_QuickSandEventTemplateDefaults.depth = 4.0f;
        g_zDEClient_QuickSandEventTemplateDefaults.radius = 20.0f;

        zReader::ReadNamedInt(
            quickSandNode,
            g_zDEClient_PointsFieldName,
            &g_zDEClient_QuickSandEventTemplateDefaults.pointCount
        );
        zReader::ReadNamedFloat(
            quickSandNode,
            g_zDEClient_SlopeFieldName,
            &g_zDEClient_QuickSandEventTemplateDefaults.slope
        );
        zReader::ReadNamedFloat(
            quickSandNode,
            g_zDEClient_DepthFieldName,
            &g_zDEClient_QuickSandEventTemplateDefaults.depth
        );
        zReader::ReadNamedFloat(
            quickSandNode,
            g_zDEClient_RadiusFieldName,
            &g_zDEClient_QuickSandEventTemplateDefaults.radius
        );

        zModel_MaterialPartial material;
        zModel_Material::ResetDefaults(&material);
        material.flags = (unsigned short)(material.flags | 0x0100);

        if (textureCount > 1) {
            zModel_Material::SetCycleTextureCount(
                &material,
                textureCount
            );
            zModel_Material::SetCycleTextureSpeed(
                &material,
                g_zDEClient_QuickSandAnimSpeed
            );
            zModel_Material::SetCycleTextureLoop(
                &material,
                1
            );

            for (int i = 0; i < textureCount; ++i) {
                zModel_Material::AddCycleTexture(
                    &material,
                    zImage::TexDir_FindOrAppendByPath(g_zDEClient_QuickSandTexturePaths[i])
                );
            }

            textureLoadPending = 1;
            g_zDEClient_QuickSandMaterial = zModel_Material::FindOrClone(&material);
        } else if (textureCount == 1) {
            zImage::TexDir_FindOrAppendByPath(g_zDEClient_QuickSandTexturePaths[0]);
            textureLoadPending = 1;
            g_zDEClient_QuickSandMaterial = zModel_Material::FindOrClone(&material);
        } else {
            material.flags = (unsigned short)(material.flags & 0xfeff);
            g_zDEClient_QuickSandMaterial = 0;
            zError::ReportOld(
                0x100,
                g_zDEClient_SourceFile_ZdecInitCpp,
                0xef,
                g_zDEClient_QuickSandUntexturedMsg
            );
        }

        if (g_zDEClient_QuickSandMaterial != 0) {
            zModel_Material::SetUserTag(
                g_zDEClient_QuickSandMaterial,
                3
            );
        }

        if (textureCount >= 1 && g_zDEClient_QuickSandTexturePaths != 0) {
            zModel_Material::ResetDefaults(&material);
            material.flags = (unsigned short)(material.flags | 0x0100);
            material.currentTextureDirectoryEntry =
                zImage::FindTexDirEntryByName(g_zDEClient_QuickSandTexturePaths[0]);
            zModel_Material::SetUserTag(
                &material,
                0
            );
            g_zDEClient_QuickSandMaterialCycle = zModel_Material::Clone(&material);
            g_zDEClient_QuickSandEnabled = 1;
        } else {
            g_zDEClient_QuickSandMaterialCycle = 0;
            g_zDEClient_QuickSandEnabled = 1;
        }
    } else {
        g_zDEClient_QuickSandEnabled = 0;
    }

    if (textureLoadPending != 0) {
        zImage::TexDir_LoadPendingEntries();
    }

    zReader::FreeLoadedTree(g_zDEClient_ConfigReaderRoot);
    const int rebuildBltRectOnReload = g_zDEClient_RebuildBltRectOnReload;
    g_zDEClient_ConfigReaderRoot = 0;

    if (rebuildBltRectOnReload != 0) {
        zUtil_ZAR::RegisterSectionHandler(
            g_zDEClient_NodeName,
            ZbdCallbackPtr(&WriteFeatureSectionsToZAR),
            ZbdCallbackPtr(&ApplyFeatureEntry),
            0x3e8,
            0
        );
    }

    return 0;
}

/**
 * Reimplements 0x455dd0: zDEClient::LoadOrCreateMaterialFromTexturePath.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.c.
 * Purpose: resolve or create a material for a texture path and report whether
 * the texture directory entry still needs loading.
 */
RECOIL_NO_GS int __fastcall LoadMaterialFromTexturePath_Local(
    zModel_MaterialPartial **outMaterial,
    char *texturePath
) {
    int result = 0;
    zImage_TexDirEntryPartial *textureDirectoryEntry = zImage::FindTexDirEntryByName(texturePath);
    if (textureDirectoryEntry == 0) {
        textureDirectoryEntry = zImage::TexDir_FindOrAppendByPath(texturePath);
        result = 1;
        *outMaterial = 0;
    } else {
        *outMaterial = zModel_Material::FindByTexDirEntry(textureDirectoryEntry);
    }

    if (*outMaterial == 0) {
        zModel_MaterialPartial material;
        zModel_Material::ResetDefaults(&material);
        material.flags = (unsigned short)(material.flags | 0x0100);
        material.currentTextureDirectoryEntry = textureDirectoryEntry;
        *outMaterial = zModel_Material::FindOrClone(&material);
    }

    return result;
}

/**
 * Reimplements 0x455e40: zDEClient::ShutdownGlobals.
 * Original source path: D:\Proj\GameZRecoil\zDEClient\zdec_init.c.
 * Purpose: clear feature runtime state, free loaded quicksand and crater
 * config arrays, and mark quicksand resources shut down.
 */
int ShutdownGlobals() {
    if (g_zDEClient_QuickSandEnabled == 0) {
        return 0;
    }

    ClearFeatureEntriesAndMapTree();

    if (g_zDEClient_QuickSandTexturePaths != 0) {
        free(g_zDEClient_QuickSandTexturePaths);
        g_zDEClient_QuickSandTexturePaths = 0;
    }

    if (g_zDEClient_CraterDisplaySourceList != 0) {
        free(g_zDEClient_CraterDisplaySourceList);
        g_zDEClient_CraterDisplaySourceList = 0;
    }

    g_zDEClient_QuickSandEnabled = 0;
    zGame::ReturnOnlyStub();
    return 0;
}
} // namespace zDEClient
#include "zdec_qsand.cpp"
#include "zdec_crater.cpp"

/* Source-layout blocker: address-backed bodies below do not belong to the assigned contiguous ledger rows.
 * They are preserved here because their proven physical owner is outside this worker scope or still unresolved.
 */
/**
 * Reimplements 0x433ad0: zDEClient_Crater::Execute
 * (D:\Proj\GameZRecoil\RecoilApp\zDEClient_Crater.cpp).
 *
 * Purpose: prepare and send or locally relay a crater event generated by the
 * owning player node.
 */
int __fastcall Execute(
    zDEClient_CraterEventTemplate *eventTemplate
) {
    if (eventTemplate->radius <= 0.0f) {
        eventTemplate->radius = -eventTemplate->radius;
        return 1;
    }

    zUtil_SaveGameState *const saveState = (zUtil_SaveGameState *)(g_GameStateOrMapTable);
    if (eventTemplate->damageOwnerNode != saveState->playerState->rootNode) {
        return 0;
    }

    g_NetPkt0F_CraterEventRelayBuf.header.payloadDword0 = zNetwork_GetLocalPlayerKey();
    g_NetPkt0F_CraterEventRelayBuf.craterTypeId =
        zModel_MatlSlot::IndexFromPtrOrMinus1(eventTemplate->craterMaterialSlot);
    g_NetPkt0F_CraterEventRelayBuf.center = eventTemplate->center;
    g_NetPkt0F_CraterEventRelayBuf.radius = eventTemplate->radius;

    if (zNetwork::IsHost() != 0) {
        NetRelayCallback(
            zNetwork_GetLocalPlayerKey(),
            &g_NetPkt0F_CraterEventRelayBuf
        );
        return 0;
    }

    zNetwork_SendPacketReliable(&g_NetPkt0F_CraterEventRelayBuf.header);
    return 0;
}

/**
 * Reimplements 0x433b70: zDEClient_Crater::NetRelayCallback
 * (D:\Proj\GameZRecoil\RecoilApp\zDEClient_Crater.cpp).
 *
 * Purpose: receive crater network packets, instance accepted crater events,
 * and have the host rebroadcast relayed crater packets.
 */
int __fastcall NetRelayCallback(
    int,
    NetPkt0F_CraterEvent *packet
) {
    zDEClient_CraterEventTemplate eventTemplate;
    InitEventTemplateDefaults(&eventTemplate);

    if (zNetwork::IsHost() != 0) {
        eventTemplate.craterMaterialSlot = zModel_Matl::GetPoolEntry(packet->craterTypeId);
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        if (InstanceEventMaybeRelay(&eventTemplate) == 0) {
            packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
            packet->eventFlags |= 0x80u;
            zNetwork_SendPacketReliable(&packet->header);
        }
        return 1;
    }

    if ((packet->eventFlags & 0x80u) != 0) {
        eventTemplate.craterMaterialSlot = zModel_Matl::GetPoolEntry(packet->craterTypeId);
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        InstanceEventMaybeRelay(&eventTemplate);
    }

    return 1;
}

/**
 * Reimplements 0x433d40: zDEClient_QSand::NetRelayCallback
 * (D:\Proj\GameZRecoil\RecoilApp\zDEClient_QSand.cpp).
 *
 * Purpose: relay quicksand packet 10 through the host and instance received
 * quicksand events locally.
 */
int __fastcall NetRelayCallback(
    int,
    NetPkt10_QSandEvent *packet
) {
    zDEClient_QSandEventTemplate eventTemplate;
    zDEClient::CopyQSandEventTemplateDefaults(&eventTemplate);

    if (zNetwork::IsHost() != 0) {
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        if (InstanceEventMaybeRelay(&eventTemplate) == 0) {
            packet->header.payloadDword0 = zNetwork_GetLocalPlayerKey();
            packet->eventFlags |= 0x80u;
            zNetwork_SendPacketReliable(&packet->header);
        }
        return 1;
    }

    if ((packet->eventFlags & 0x80u) != 0) {
        eventTemplate.center = packet->center;
        eventTemplate.radius = -packet->radius;
        InstanceEventMaybeRelay(&eventTemplate);
    }

    return 1;
}

/**
 * Reimplements 0x46ae40: zGeometry_ClipPatchOutput::ApplyNodeDiPairs
 * (D:\Proj\GameZRecoil\zGeometry\zgeo_weiler.cpp).
 *
 * Purpose: publish generated display instances to their clip-patch nodes,
 * release replaced display instances, and clear consumed node/DI pairs.
 */
int __fastcall ApplyNodeDiPairs(
    zGeometry_ClipPatchOutputPartial *self
) {
    {
        for (int partitionIndex = 0; partitionIndex < self->partitionCount; ++partitionIndex) {
            zGeometry_ClipPatchPartitionOutput *const partition = &self->partitions[partitionIndex];
            for (int i = 0; i < partition->nodeDiPairCount; ++i) {
                zGeometry_ClipPatchNodeDiPair *const pair = &partition->nodeDiPairs[i];

                unsigned int oldDisplayInstanceValue = 0;
                zClass_Class::gwNodeGetUserData(
                    pair->node,
                    &oldDisplayInstanceValue
                );
                zClass_Class::gwNodeSetDisplayInstance(
                    pair->node,
                    pair->di
                );

                if (oldDisplayInstanceValue != 0) {
                    zModel_DiPool::FreeIfUnreferenced(
                        (zDiPartial *)((unsigned int)(oldDisplayInstanceValue))
                    );
                }
            }

            ++partition->featureGridCell->featureCount;

            if (partition->nodeDiPairs != 0) {
                free(partition->nodeDiPairs);
                partition->nodeDiPairs = 0;
                partition->nodeDiPairCount = 0;
            }
        }
    }

    return 0;
}
} // namespace zGeometry_ClipPatchOutput

namespace zDEClient {
/**
 * Reimplements 0x46af00: zGeometry_ClipPatchOutput::Create
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: allocate an empty clip-patch output record for crater and quicksand
 * feature tessellation.
 */
zGeometry_ClipPatchOutputPartial *Create() {
    zGeometry_ClipPatchOutputPartial *result =
        (zGeometry_ClipPatchOutputPartial *)(malloc(sizeof(zGeometry_ClipPatchOutputPartial)));
    result->pointCount = 0;
    result->points = 0;
    result->partitionCount = 0;
    result->partitions = 0;
    return result;
}

/**
 * Reimplements 0x46af20: zGeometry_ClipPatchOutput::Destroy
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: free the partition buffer owned by a clip-patch output record and
 * release the record itself.
 */
void __fastcall Destroy(
    zGeometry_ClipPatchOutputPartial *self
) {
    if (self->partitions != 0) {
        free(self->partitions);
    }

    free(self);
}

/**
 * Reimplements 0x46af40:
 * zDEClient::CreateFeatureNodeAndDiFromClipPatchPartition
 * (D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp).
 *
 * Purpose: allocate a feature child node and display instance for a clipped
 * partition while preserving the original node type and parent linkage.
 */
zDiPartial *__fastcall CreateFeatureNodeAndDiFromClipPatchPartition(
    zGeometry_ClipPatchPartitionOutput *partitionOutput,
    zClass_NodePartial *parentNode,
    zClass_NodePartial **outNode
) {
    if (partitionOutput == 0) {
        return 0;
    }

    zClass_NodePartial *child = zClass_Object3D::gwObject3DInit();
    if (child == 0) {
        if (outNode != 0) {
            *outNode = child;
        }

        return 0;
    }

    if (outNode != 0) {
        *outNode = child;
    }

    zClass_Class::gwNodeSetNodeType(
        child,
        0xff
    );

    for (int i = 0; i < partitionOutput->nodeDiPairCount; ++i) {
        zGeometry_ClipPatchNodeView *const node = partitionOutput->nodeDiPairs[i].node;
        if ((node->flags & 0x10000) == 0) {
            continue;
        }

        int nodeType;
        zClass_Class::gwNodeGetNodeType(
            node,
            &nodeType
        );
        if (nodeType != 0xff) {
            zClass_Class::gwNodeSetNodeType(
                child,
                nodeType
            );
            break;
        }
    }

    zClass_Class::gwNodeSetFlag17(
        child,
        1
    );

    zDiPartial *const displayInstance = zModel_DiPool::AllocFromFreeList();
    if (displayInstance == 0) {
        if (outNode != 0) {
            *outNode = 0;
        }

        zClass_Object3D::DeleteNode(child);
        return 0;
    }

    zClass_Class::AddChild(
        parentNode,
        child
    );
    zClass_Class::gwNodeSetDisplayInstance(
        child,
        displayInstance
    );
    return displayInstance;
}
} // namespace zDEClient

