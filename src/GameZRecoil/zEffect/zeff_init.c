#include "GameZRecoil/zEffect/zeff.h"

#include "GameZRecoil/Time/time.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zUtil/zutil.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "zdi.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

extern char g_EffectsZrdNodeName[8];

namespace {
const char *kZeffInitSourceFile = g_zEffect_SourceFile_ZeffInitC;
} // namespace

namespace zEffect {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-init.init
 * @recoil-artifact defines .text recoil:function:0x460020: zEffect::Init.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: reset the runtime effect manager and initialize zEffect animation
 * state.
 */
int Init() {
    g_zEffect_RuntimeManager.initialized = 0;
    g_zEffect_RuntimeManager.templateCount = 0;
    g_zEffect_RuntimeManager.loadedTemplateTree = 0;
    g_zEffect_RuntimeManager.freeList = 0;
    g_zEffect_RuntimeManager.templates = 0;
    g_zEffect_RuntimeManager.parentNode = 0;
    g_zEffect_RuntimeManager.freshAllocCount = 0;
    g_zEffect_RuntimeManager.activatedCount = 0;
    g_zEffect_RuntimeManager.recycleCount = 0;
    return zEffect_Anim::Init();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-init.shutdownall
 * @recoil-artifact defines .text recoil:function:0x460060: zEffect::ShutdownAll.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: reset runtime effect state and shut down animation data when it is
 * loaded.
 */
int ShutdownAll() {
    Reset();
    return zEffect_Anim::ShutdownIfLoaded();
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-init.initfrompath
 * @recoil-artifact defines .text recoil:function:0x460070: zEffect::InitFromPath.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: load runtime effect templates from a zReader tree and prepare the
 * runtime free list and texture cycling data.
 */
int __fastcall InitFromPath(
    zClass_NodePartial *worldNode,
    zClass_NodePartial *cameraNode,
    const char *path
) {
    if (g_zEffect_RuntimeManager.initialized != 0) {
        return 0;
    }

    zReader::Node *const rootNode = zReader::LoadNodeFromPath(
        path,
        0,
        0
    );
    g_zEffect_RuntimeManager.loadedTemplateTree = (zClass_NodePartial *)(rootNode);
    if (rootNode == 0) {
        fprintf(
            stderr,
            g_zEffect_ReadFieldFailedFmt,
            kZeffInitSourceFile,
            0xd8,
            path
        );
        return -1;
    }

    zReader::Node *const effectsNode = zReader_GetNamedNode(
        rootNode,
        g_EffectsZrdNodeName
    );
    g_zEffect_RuntimeManager.templateCount =
        effectsNode->value.nodes->value.i32 - 1;
    g_zEffect_RuntimeManager.templates = (zEffect_RuntimeEntry *)(calloc(
        g_zEffect_RuntimeManager.templateCount,
        sizeof(zEffect_RuntimeEntry)
    ));
    g_zEffect_RuntimeManager.parentNode = worldNode;
    g_zEffect_RuntimeManager.listenerNode = cameraNode;

    for (int i = 0; i < g_zEffect_RuntimeManager.templateCount; ++i) {
        zReader::Node *const effectNode =
            &effectsNode->value.nodes[i + 1];
        zReader::Node *const mapsNode = zReader_GetNamedNode(
            effectNode,
            g_zEffect_TokenMaps
        );
        zEffect_RuntimeEntry *const runtimeEntry = &g_zEffect_RuntimeManager.templates[i];
        runtimeEntry->effectIndex = -1;
        runtimeEntry->modelNodeName =
            effectNode->value.nodes[1].value.str;
        runtimeEntry->effectName = (char *)(zReader::ReadNamedString(
            effectNode,
            "NAME"
        ));

        zClass_NodePartial *const templateNode =
            zClass::FindByTypeAndName(
                6,
                runtimeEntry->modelNodeName
            );
        runtimeEntry->effectNode = templateNode;
        if (templateNode == 0) {
            fprintf(
                stderr,
                g_zEffect_NodeLookupFailedFmt,
                kZeffInitSourceFile,
                0xf3,
                runtimeEntry->modelNodeName,
                runtimeEntry->effectName
            );
            continue;
        }

        void *const gfxData = FindNodeUserDataRecursive(templateNode);
        if (gfxData == 0) {
            zError::ReportOld(
                0x400,
                kZeffInitSourceFile,
                0xfb,
                g_zEffect_FailedToFindGfxDataFmt,
                runtimeEntry->modelNodeName
            );
            continue;
        }

        zClass_Class::gwNodeSetCellPickable(
            runtimeEntry->effectNode,
            0
        );
        zClass_Class::gwNodeSetRaycastable(
            runtimeEntry->effectNode,
            0
        );
        zClass_Class::gwNodeSetActive(
            runtimeEntry->effectNode,
            0
        );
        runtimeEntry->effectIndex = i;
        runtimeEntry->effectGfxData = gfxData;
        zUtil::StoreInt32(
            (int *)(gfxData),
            1
        );

        zModel_MaterialPartial *const material = (zModel_MaterialPartial *)(gfxData);
        const int textureCount = mapsNode->value.nodes->value.i32 - 1;
        zModel_Material::SetCycleTextureCount(
            material,
            textureCount
        );

        float textureSpeed = 0.0f;
        zReader::ReadNamedFloat(
            effectNode,
            g_zEffectAnim_TokenSpeed,
            &textureSpeed
        );
        zModel_Material::SetCycleTextureSpeed(
            material,
            textureSpeed
        );

        zReader::Node *const loopingNode = zReader_GetNamedNode(
            effectNode,
            g_zEffectAnim_TokenLooping
        );
        if (loopingNode != 0) {
            const char *const loopingText =
                loopingNode->type == zReader::ZRDR_NODE_ARRAY
                    ? loopingNode->value.nodes[1].value.str
                    : loopingNode->value.str;
            zModel_Material::SetCycleTextureLoop(
                material,
                strcmp(
                    loopingText,
                    "ON"
                ) == 0 ? 1 : 0
            );
        }

        {
            for (int textureIndex = 1; textureIndex <= textureCount; ++textureIndex) {
                zModel_Material::AddCycleTexture(
                    material,
                    zImage::TexDir_FindOrAppendByPath(
                        mapsNode->value.nodes[textureIndex].value.str
                    )
                );
            }
        }
    }

    zImage::TexDir_LoadPendingEntries();
    g_zEffect_RuntimeManager.freeList = zArchiveList_CreateEmpty();
    g_zEffect_RuntimeManager.recycleCount = 0;
    g_zEffect_RuntimeManager.initialized = 1;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-init.reset
 * @recoil-artifact defines .text recoil:function:0x460330: zEffect::Reset.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: free loaded runtime template data, delete recycled effect nodes,
 * destroy the free list, and reinitialize zEffect state.
 */
int Reset() {
    if (g_zEffect_RuntimeManager.loadedTemplateTree != 0) {
        zReader::FreeLoadedTree((zReader::Node *)(g_zEffect_RuntimeManager.loadedTemplateTree));
        g_zEffect_RuntimeManager.loadedTemplateTree = 0;
    }

    if (g_zEffect_RuntimeManager.templates != 0) {
        free(g_zEffect_RuntimeManager.templates);
        g_zEffect_RuntimeManager.templates = 0;
    }

    zArchiveList *freeList = g_zEffect_RuntimeManager.freeList;
    if (freeList != 0) {
        zEffect_RuntimeEntry *entry =
            (zEffect_RuntimeEntry *)(zArchiveList_PopFrontPayload(freeList));
        while (entry != 0) {
            if (entry->effectNode != 0) {
                zClass_Util::DestroyNodeRecursive(entry->effectNode);
            }

            free(entry);
            entry = (zEffect_RuntimeEntry *)(zArchiveList_PopFrontPayload(freeList));
        }

        zArchiveList_Destroy(freeList);
        g_zEffect_RuntimeManager.freeList = 0;
    }

    g_zEffect_RuntimeManager.recycleCount = 0;
    Init();
    return 0;
}

} // namespace zEffect
