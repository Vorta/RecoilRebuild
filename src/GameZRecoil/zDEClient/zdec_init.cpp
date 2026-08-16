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
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-quicksandenabled
 * @recoil-artifact defines .data recoil:data:0x539d10: g_zDEClient_QuickSandEnabled.
 * Purpose: Tracks whether quicksand runtime globals were initialized and need
 * shutdown cleanup.
 */
int g_zDEClient_QuickSandEnabled = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-quicksandtexturecount
 * @recoil-artifact defines .data recoil:data:0x539d14: g_zDEClient_QuickSandTextureCount.
 * Purpose: Stores the configured quicksand texture-cycle path count.
 */
int g_zDEClient_QuickSandTextureCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-quicksandanimspeed
 * @recoil-artifact defines .data recoil:data:0x539d18: g_zDEClient_QuickSandAnimSpeed.
 * Purpose: Stores the quicksand material cycle animation speed.
 */
float g_zDEClient_QuickSandAnimSpeed = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-quicksandtexturepaths
 * @recoil-artifact defines .data recoil:data:0x539d1c: g_zDEClient_QuickSandTexturePaths.
 * Purpose: Owns the quicksand texture path pointer array allocated from
 * declient.zrd configuration.
 */
char **g_zDEClient_QuickSandTexturePaths = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-quicksandmaterial
 * @recoil-artifact defines .data recoil:data:0x539d24: g_zDEClient_QuickSandMaterial.
 * Purpose: Caches the quicksand material used by generated terrain features.
 */
zModel_MaterialPartial *g_zDEClient_QuickSandMaterial = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-quicksandmaterialcycle
 * @recoil-artifact defines .data recoil:data:0x539d28: g_zDEClient_QuickSandMaterialCycle.
 * Purpose: Caches the quicksand cycle material used while feature textures
 * animate.
 */
zModel_MaterialPartial *g_zDEClient_QuickSandMaterialCycle = 0;
zDEClient_QSandEventTemplate g_zDEClient_QuickSandEventTemplateDefaults = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-craterdisplaysourcecount
 * @recoil-artifact defines .data recoil:data:0x539ce0: g_zDEClient_CraterDisplaySourceCount.
 * Purpose: Stores g zDEClient CraterDisplaySourceCount data used by engine.zeffect.zdeclient_crater_display_source_globals.
 */
int g_zDEClient_CraterDisplaySourceCount = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-craterdisplaysourcelist
 * @recoil-artifact defines .data recoil:data:0x539ce4: g_zDEClient_CraterDisplaySourceList.
 * Purpose: Stores g zDEClient CraterDisplaySourceList data used by engine.zeffect.zdeclient_crater_display_source_globals.
 */
zDEClient_CraterDisplaySourceEntry *g_zDEClient_CraterDisplaySourceList = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-cratereventtemplatedefaults
 * @recoil-artifact defines .data recoil:data:0x539ce8: g_zDEClient_CraterEventTemplateDefaults.
 * Purpose: Stores g zDEClient CraterEventTemplateDefaults data used by engine.zeffect.zdeclient_crater_event_template_defaults.
 */
zDEClient_CraterEventTemplate g_zDEClient_CraterEventTemplateDefaults = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-configreaderroot
 * @recoil-artifact defines .data recoil:data:0x539de0: g_zDEClient_ConfigReaderRoot.
 * Purpose: Holds the transient declient.zrd reader tree while config
 * resources are loaded.
 */
zReader::Node *g_zDEClient_ConfigReaderRoot = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-rebuildbltrectonreload
 * @recoil-artifact defines .data recoil:data:0x4df3c4: g_zDEClient_RebuildBltRectOnReload.
 * Purpose: Enables zDEClient ZAR reload handler registration after config
 * load completes.
 */
int g_zDEClient_RebuildBltRectOnReload = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-nodename
 * @recoil-artifact defines .data recoil:data:0x4df3c8: g_zDEClient_NodeName.
 * Purpose: Names the zDEClient ZAR section handler registered after config
 * loading.
 */
char g_zDEClient_NodeName[] = "zDEClient";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-quicksanduntexturedmsg
 * @recoil-artifact defines .data recoil:data:0x4df3d4: g_zDEClient_QuickSandUntexturedMsg.
 * Purpose: Reports the quicksand fallback when no texture path was configured.
 */
char g_zDEClient_QuickSandUntexturedMsg[] = "Quick sand will NOT be textured";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-quicksandnodename
 * @recoil-artifact defines .data recoil:data:0x4df3f4: g_zDEClient_QuickSandNodeName.
 * Purpose: Names the QUICK_SAND config node in declient.zrd.
 */
char g_zDEClient_QuickSandNodeName[] = "QUICK_SAND";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-textureanimnodename
 * @recoil-artifact defines .data recoil:data:0x4df400: g_zDEClient_TextureAnimNodeName.
 * Purpose: Names the crater texture animation config node.
 */
char g_zDEClient_TextureAnimNodeName[] = "TEXTURE_ANIM";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-defaultanimnodename
 * @recoil-artifact defines .data recoil:data:0x4df410: g_zDEClient_DefaultAnimNodeName.
 * Purpose: Names the default crater animation field.
 */
char g_zDEClient_DefaultAnimNodeName[] = "DEFAULT_ANIM";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-defaulttexturenodename
 * @recoil-artifact defines .data recoil:data:0x4df420: g_zDEClient_DefaultTextureNodeName.
 * Purpose: Names the default crater/quicksand texture field.
 */
char g_zDEClient_DefaultTextureNodeName[] = "DEFAULT_TEXTURE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-radiusfieldname
 * @recoil-artifact defines .data recoil:data:0x4df430: g_zDEClient_RadiusFieldName.
 * Purpose: Names the feature radius config field.
 */
char g_zDEClient_RadiusFieldName[] = "RADIUS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-depthfieldname
 * @recoil-artifact defines .data recoil:data:0x4df438: g_zDEClient_DepthFieldName.
 * Purpose: Names the feature depth config field.
 */
char g_zDEClient_DepthFieldName[] = "DEPTH";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-slopefieldname
 * @recoil-artifact defines .data recoil:data:0x4df440: g_zDEClient_SlopeFieldName.
 * Purpose: Names the feature slope config field.
 */
char g_zDEClient_SlopeFieldName[] = "SLOPE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-pointsfieldname
 * @recoil-artifact defines .data recoil:data:0x4df448: g_zDEClient_PointsFieldName.
 * Purpose: Names the feature point-count config field.
 */
char g_zDEClient_PointsFieldName[] = "POINTS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-craternodename
 * @recoil-artifact defines .data recoil:data:0x4df450: g_zDEClient_CraterNodeName.
 * Purpose: Names the CRATER config node in declient.zrd.
 */
char g_zDEClient_CraterNodeName[] = "CRATER";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-readdefaultsfallbackfmt
 * @recoil-artifact defines .data recoil:data:0x4df458: g_zDEClient_ReadDefaultsFallbackFmt.
 * Purpose: Formats the declient.zrd load warning that keeps built-in
 * defaults.
 */
char g_zDEClient_ReadDefaultsFallbackFmt[] = "Failed to read (%s), using defaults";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-configarchivename
 * @recoil-artifact defines .data recoil:data:0x4df47c: g_zDEClient_ConfigArchiveName.
 * Purpose: Names the zDEClient config archive loaded at startup.
 */
char g_zDEClient_ConfigArchiveName[] = "declient.zrd";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-sourcefile-zdecinitcpp
 * @recoil-artifact defines .data recoil:data:0x4df48c: g_zDEClient_SourceFile_ZdecInitCpp.
 * Purpose: Provides the original source path for zDEClient config diagnostics.
 */
char g_zDEClient_SourceFile_ZdecInitCpp[] =
    "D:\\Proj\\GameZRecoil\\zDEClient\\zdec_init.cpp";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-worldnodenullerrormsg
 * @recoil-artifact defines .data recoil:data:0x4df4b8: g_zDEClient_WorldNodeNullErrorMsg.
 * Purpose: Reports a missing world node during zDEClient config loading.
 */
char g_zDEClient_WorldNodeNullErrorMsg[] = "Failed to DEClient: world node is NULL.";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-featurenodename
 * @recoil-artifact defines .data recoil:data:0x4df59c: g_zDEClient_FeatureNodeName.
 * Purpose: Names generated zDEClient terrain feature nodes.
 */
char g_zDEClient_FeatureNodeName[] = "ZDEC_FEATURE";
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
RECOIL_STATIC_ASSERT(sizeof(g_zDEClient_FeatureNodeName) == 0x0d);
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-cameranode
 * @recoil-artifact defines .data recoil:data:0x539e18: g_zDEClient_CameraNode.
 * Purpose: Stores g zDEClient CameraNode data used by engine.zeffect.zdeclient_camera_globals.
 */
zClass_NodePartial *g_zDEClient_CameraNode = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclient-cameranodeclassdata
 * @recoil-artifact defines .data recoil:data:0x539e1c: g_zDEClient_CameraNodeClassData.
 * Purpose: Stores g zDEClient CameraNodeClassData data used by engine.zeffect.zdeclient_camera_globals.
 */
zClass_CameraDataPartial *g_zDEClient_CameraNodeClassData = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclientqsandnetrelaycallback
 * @recoil-artifact defines .data recoil:data:0x539de4: g_zDEClientQSandNetRelayCallback.
 * Purpose: Stores g zDEClientQSandNetRelayCallback data used by engine.zeffect.zdeclient_net_relay_callback_globals.
 */
zDEClient_NetRelayCallback g_zDEClientQSandNetRelayCallback = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-g-zdeclientcraternetrelaycallback
 * @recoil-artifact defines .data recoil:data:0x539de8: g_zDEClientCraterNetRelayCallback.
 * Purpose: Stores g zDEClientCraterNetRelayCallback data used by engine.zeffect.zdeclient_net_relay_callback_globals.
 */
zDEClient_NetRelayCallback g_zDEClientCraterNetRelayCallback = 0;

namespace zDEClient {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-zdeclient-loadconfigresources
 * @recoil-artifact defines .text recoil:function:0x4558f0: zDEClient::LoadConfigResources.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
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
        const int textureAnimEntryCount =
            textureAnimNode->value.nodes[0].value.i32;
        const int additionalDisplaySourceCount = (textureAnimEntryCount - 1) / 2;
        g_zDEClient_CraterDisplaySourceCount += additionalDisplaySourceCount;

        g_zDEClient_CraterDisplaySourceList = (zDEClient_CraterDisplaySourceEntry *)(realloc(
            g_zDEClient_CraterDisplaySourceList,
            (size_t)(g_zDEClient_CraterDisplaySourceCount) *
                sizeof(zDEClient_CraterDisplaySourceEntry)
        ));

        zDEClient_CraterDisplaySourceEntry *displaySource = &g_zDEClient_CraterDisplaySourceList[1];
        for (int i = 1; i < textureAnimEntryCount; i += 2) {
            if (LoadMaterialFromTexturePath_Local(
                    &displaySource->sourceMaterial,
                    textureAnimNode->value.nodes[i].value.str
                ) != 0) {
                textureLoadPending = 1;
            }

            zReader::Node *const entryNode =
                zReader_GetNamedNode(
                    textureAnimNode,
                    textureAnimNode->value.nodes[i].value.str
                );
            if (entryNode != 0) {
                if (LoadMaterialFromTexturePath_Local(
                        &displaySource->craterMaterial,
                        entryNode->value.nodes[1].value.str
                    ) != 0) {
                    textureLoadPending = 1;
                }

                if (entryNode->value.nodes[0].value.i32 > 2) {
                    displaySource->effectAnimEntry =
                        zEffectAnim::FindEntryByName(
                            entryNode->value.nodes[2].value.str
                        );
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
            (zZbdSectionCallback)(&WriteFeatureSectionsToZAR),
            (zZbdSectionCallback)(&ApplyFeatureEntry),
            0x3e8,
            0
        );
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-zdeclient-loadorcreatematerialfromtexturepath
 * @recoil-artifact defines .text recoil:function:0x455dd0: zDEClient::LoadOrCreateMaterialFromTexturePath.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
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
 * @recoil-anchor recoil:anchor:gamezrecoil-zdeclient-zdec-init-zdeclient-shutdownglobals
 * @recoil-artifact defines .text recoil:function:0x455e40: zDEClient::ShutdownGlobals.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zDEClient\zdec_init.cpp.
 * Purpose: clear feature runtime state, free loaded quicksand and crater
 * config arrays, and mark quicksand resources shut down.
 */
int __cdecl ShutdownGlobals() {
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
