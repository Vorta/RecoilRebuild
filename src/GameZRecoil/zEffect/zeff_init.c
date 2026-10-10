#include "GameZRecoil/zClass/cls_api.h"
#include "GameZRecoil/zEffect/zeff.h"

#include "GameZRecoil/include/zimage.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
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
/* zReader and zImage entry points; zreader.h and zimage.h declare them for the C++ units only. */
Node* __fastcall zRdrFindTag(Node* parentNode, const char* name);
const char* __fastcall FindString(Node* parentNode, const char* name);
int __fastcall GetFloat(Node* parentNode, const char* name, float* outValue);
zArchiveList* __cdecl zArchiveListNew(void);
int __fastcall zArchiveListFree(zArchiveList* list);
void* __fastcall zArchiveListRemoveHead(zArchiveList* list);
zImage_TexDirEntryPartial* __fastcall TexDirFindOrAppendByPath(char* path);
int __cdecl TexDirLoadPendingEntries(void);

static const char* kZeffInitSourceFile = g_zEffect_SourceFile_ZeffInitC;

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-init.init
 * @recoil-artifact defines .text recoil:function:0x460020: zEffect::zEffInit.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: reset the runtime effect manager and initialize zEffect animation
 * state.
 */
int __fastcall zEffInit(void)
{
    g_zEffect_RuntimeManager.initialized = 0;
    g_zEffect_RuntimeManager.templateCount = 0;
    g_zEffect_RuntimeManager.loadedTemplateTree = 0;
    g_zEffect_RuntimeManager.freeList = 0;
    g_zEffect_RuntimeManager.templates = 0;
    g_zEffect_RuntimeManager.parentNode = 0;
    g_zEffect_RuntimeManager.freshAllocCount = 0;
    g_zEffect_RuntimeManager.activatedCount = 0;
    g_zEffect_RuntimeManager.recycleCount = 0;
    return zEffectAnimInit();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-init.shutdownall
 * @recoil-artifact defines .text recoil:function:0x460060: zEffect::ShutdownAll.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: reset runtime effect state and shut down animation data when it is
 * loaded.
 */
int __cdecl ShutdownAll(void)
{
    Reset();
    return ShutdownIfLoaded();
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-init.initfrompath
 * @recoil-artifact defines .text recoil:function:0x460070: zEffect::InitFromPath.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: load runtime effect templates from a zReader tree and prepare the
 * runtime free list and texture cycling data.
 */
int __fastcall InitFromPath(CZNodePartial* worldNode, CZNodePartial* cameraNode, const char* path)
{
    Node* rootNode;
    Node* effectsNode;
    int i;
    if (g_zEffect_RuntimeManager.initialized != 0) {
        return 0;
    }

    rootNode = Load(path, 0, 0);
    g_zEffect_RuntimeManager.loadedTemplateTree = (CZNodePartial*)(rootNode);
    if (rootNode == 0) {
        fprintf(stderr, g_zEffect_ReadFieldFailedFmt, g_zEffect_SourceFile_ZeffInitC, 0xd8, path);
        return -1;
    }

    effectsNode = zRdrFindTag(rootNode, g_EffectsZrdNodeName);
    g_zEffect_RuntimeManager.templateCount = effectsNode->value.nodes->value.i32 - 1;
    g_zEffect_RuntimeManager.templates
        = (zEffect_RuntimeEntry*)(calloc(g_zEffect_RuntimeManager.templateCount, sizeof(zEffect_RuntimeEntry)));
    g_zEffect_RuntimeManager.parentNode = worldNode;
    g_zEffect_RuntimeManager.listenerNode = cameraNode;

    for (i = 0; i < g_zEffect_RuntimeManager.templateCount; ++i) {
        float textureSpeed = 0.0f;
        Node* const effectNode = &effectsNode->value.nodes[i + 1];
        Node* const mapsNode = zRdrFindTag(effectNode, g_zEffect_TokenMaps);
        zEffect_RuntimeEntry* const runtimeEntry = &g_zEffect_RuntimeManager.templates[i];
        CZNodePartial* templateNode;
        void* gfxData;
        zDiPartial* displayInstance;
        int textureCount;
        Node* loopingNode;
        runtimeEntry->effectIndex = -1;
        runtimeEntry->modelNodeName = effectNode->value.nodes[1].value.str;
        runtimeEntry->effectName = (char*)(FindString(effectNode, "NAME"));

        templateNode = FindByTypeAndName(6, runtimeEntry->modelNodeName);
        runtimeEntry->effectNode = templateNode;
        if (templateNode == 0) {
            fprintf(
                stderr,
                g_zEffect_NodeLookupFailedFmt,
                g_zEffect_SourceFile_ZeffInitC,
                0xf3,
                runtimeEntry->modelNodeName,
                runtimeEntry->effectName
            );
            continue;
        }

        gfxData = FindNodeUserDataRecursive(templateNode);
        if (gfxData == 0) {
            ReportOld(
                0x400,
                g_zEffect_SourceFile_ZeffInitC,
                0xfb,
                g_zEffect_FailedToFindGfxDataFmt,
                runtimeEntry->modelNodeName
            );
            continue;
        }

        gwNodeSetCellPickable(runtimeEntry->effectNode, 0);
        gwNodeSetRaycastable(runtimeEntry->effectNode, 0);
        gwNodeSetActive(runtimeEntry->effectNode, 0);
        runtimeEntry->effectIndex = i;
        runtimeEntry->effectGfxData = gfxData;
        StoreInt32((int*)(gfxData), 1);

        displayInstance = (zDiPartial*)(gfxData);
        textureCount = mapsNode->value.nodes->value.i32 - 1;
        SetCurrentVariantCycleTextureCount(displayInstance, textureCount);

        GetFloat(effectNode, g_zEffectAnim_TokenSpeed, &textureSpeed);
        SetCurrentVariantCycleTextureSpeed(displayInstance, textureSpeed);

        loopingNode = zRdrFindTag(effectNode, g_zEffectAnim_TokenLooping);
        if (loopingNode != 0) {
            if (strcmp(loopingNode->value.nodes[1].value.str, "ON") == 0) {
                zModelInstanceSetCycleTextureLoop(displayInstance, 1);
            } else {
                zModelInstanceSetCycleTextureLoop(displayInstance, 0);
            }
        }

        {
            int textureIndex;
            for (textureIndex = 1; textureIndex <= textureCount; ++textureIndex) {
                zModelInstanceAddCycleTexture(
                    displayInstance,
                    TexDirFindOrAppendByPath(mapsNode->value.nodes[textureIndex].value.str)
                );
            }
        }
    }

    TexDirLoadPendingEntries();
    g_zEffect_RuntimeManager.freeList = zArchiveListNew();
    g_zEffect_RuntimeManager.recycleCount = 0;
    g_zEffect_RuntimeManager.initialized = 1;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zeffect.zeff-init.reset
 * @recoil-artifact defines .text recoil:function:0x460330: zEffect::Reset.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zEffect\zeff_init.c.
 * Purpose: free loaded runtime template data, delete recycled effect nodes,
 * destroy the free list, and reinitialize zEffect state.
 */
int __fastcall Reset(void)
{
    if (g_zEffect_RuntimeManager.loadedTemplateTree != 0) {
        // Retail stores zReader::Free's (zero) result back into the tree slot.
        g_zEffect_RuntimeManager.loadedTemplateTree
            = (CZNodePartial*)(Free((Node*)(g_zEffect_RuntimeManager.loadedTemplateTree)));
    }

    if (g_zEffect_RuntimeManager.templates != 0) {
        free(g_zEffect_RuntimeManager.templates);
        g_zEffect_RuntimeManager.templates = 0;
    }

    if (g_zEffect_RuntimeManager.freeList != 0) {
        zEffect_RuntimeEntry* entry
            = (zEffect_RuntimeEntry*)(zArchiveListRemoveHead(g_zEffect_RuntimeManager.freeList));
        while (entry != 0) {
            DestroyNodeRecursive(entry->effectNode);
            free(entry);
            entry = (zEffect_RuntimeEntry*)(zArchiveListRemoveHead(g_zEffect_RuntimeManager.freeList));
        }

        zArchiveListFree(g_zEffect_RuntimeManager.freeList);
        g_zEffect_RuntimeManager.freeList = 0;
        g_zEffect_RuntimeManager.recycleCount = 0;
    }

    zEffInit();
    return 0;
}
