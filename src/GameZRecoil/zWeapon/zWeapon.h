#pragma once

#include "recoil/recoil_types.h"

#include "recoil/recoil_callconv.h"

extern "C" {
extern int g_zWeapon_ZarHandlerRegistered;
extern char g_zWeapon_ArchiveName[8];
extern float g_zWeapon_MaxTetherAltitude;

RECOIL_NOINLINE int RECOIL_CDECL zWepInit();
}
