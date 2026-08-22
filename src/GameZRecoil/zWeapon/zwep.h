#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

struct zZbdSectionCallbackCtx;
struct zClass_NodePartial;
struct OptCatalogEntryDef;

namespace zReader {
struct Node;
}

typedef void(__fastcall *zWeaponOptCatalogEntryCallback)(
    zReader::Node *entryNode,
    OptCatalogEntryDef *entry
);

extern "C" {
extern int g_zWeapon_ZarHandlerRegistered;
extern char g_zWeapon_ArchiveName[8];
extern float g_zWeapon_MaxTetherAltitude;

int __cdecl zWepInit();
}

namespace zWeapon {
int __fastcall LoadOptCatalogFromPath(
    zClass_NodePartial *worldNode,
    const char *path,
    int networkState,
    zWeaponOptCatalogEntryCallback entryCallback
);
int __fastcall OnWeaponsSectionPreLoad(
    zZbdSectionCallbackCtx *callbackCtx,
    void *userData
);
void __fastcall OnWeaponsSectionDataReady(
    zZbdSectionCallbackCtx *callbackCtx,
    const char *sectionToken,
    void *weaponData,
    unsigned int dataSize,
    void *userData
);
void __stdcall SetMaxTetherAltitude(float altitude);
} // namespace zWeapon

namespace zWeapon_OptCatalog {
void __fastcall LoadKillVerbString(
    zReader::Node *entryNode,
    OptCatalogEntryDef *entry
);
}
