#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include <ocidl.h>
#include <unknwn.h>
#include <windows.h>

#include "recoil/recoil_callconv.h"

namespace zCom {
typedef HRESULT(WINAPI *QueryInterfaceResolver)(void *objectBase, const GUID *requestedIid,
                                                void **outInterface, int interfaceOffset);

enum zComInterfaceMapResolverRaw {
    ZCOM_INTERFACE_MAP_END = 0,
    ZCOM_INTERFACE_MAP_DIRECT = 1,
};

struct InterfaceMapEntry {
    const GUID *iid;
    int interfaceOffset;
    unsigned int resolverRaw;
};
RECOIL_STATIC_ASSERT(sizeof(InterfaceMapEntry) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(InterfaceMapEntry, iid) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(InterfaceMapEntry, interfaceOffset) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(InterfaceMapEntry, resolverRaw) == 0x08);

HRESULT WINAPI QueryInterfaceFromInterfaceMap(void *objectBase,
                                              const InterfaceMapEntry *interfaceMap,
                                              const GUID *requestedIid, void **outInterface);

HRESULT WINAPI ConnectionPointContainer_Advise(IUnknown *source, REFIID connectionPointIid,
                                               IUnknown *sink, DWORD *cookie);

HRESULT WINAPI ConnectionPointContainer_Unadvise(IUnknown *source, REFIID connectionPointIid,
                                                 DWORD cookie);
} // namespace zCom
