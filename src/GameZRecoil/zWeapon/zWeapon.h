#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

struct zZbdSectionCallbackCtx;
struct zClass_NodePartial;
struct OptCatalogEntryDef;

namespace zReader {
struct Node;
}

typedef void (RECOIL_FASTCALL *zWeaponOptCatalogEntryCallback)(zReader::Node *entryNode,
                                                               OptCatalogEntryDef *entry);

extern "C" {
extern int g_zWeapon_ZarHandlerRegistered;
extern char g_zWeapon_ArchiveName[8];
extern float g_zWeapon_MaxTetherAltitude;

RECOIL_NOINLINE int RECOIL_CDECL zWepInit();
}

namespace zWeapon {
RECOIL_NOINLINE int RECOIL_FASTCALL
LoadOptCatalogFromPath(zClass_NodePartial *worldNode, const char *path, int networkState,
                       zWeaponOptCatalogEntryCallback entryCallback);
RECOIL_NOINLINE int RECOIL_FASTCALL OnWeaponsSectionPreLoad(zZbdSectionCallbackCtx *callbackCtx,
                                                            void *userData);
RECOIL_NOINLINE void RECOIL_FASTCALL
OnWeaponsSectionDataReady(zZbdSectionCallbackCtx *callbackCtx, const char *sectionToken,
                          void *weaponData, unsigned int dataSize, void *userData);
RECOIL_NOINLINE void RECOIL_STDCALL SetMaxTetherAltitude(float altitude);
}
