// Battlesport compilation unit between player_terrain.cpp and RecoilApp.cpp,
// inferred from the retail object boundary [0x42db50, 0x42de10): the ATL
// interface-map, connection-point and module helpers called by the Westwood
// Online upgrade code, with the registrar GUIDs and registry key strings at
// [0x4d08f0, 0x4d0990). Original filename unresolved; Recoil.cpp is a
// provisional name (2026-10-02).

#include "recoil/Mfc42Abi.h"
#include "player.h"

#include "Battlesport/ai_net.h"
#include "Battlesport/game_net.h"
#include "Battlesport/pickup.h"
#include "Battlesport/turret.h"
#include "Battlesport/wol_api.h"
#include "GameZRecoil/include/zclass.h"
#include "GameZRecoil/include/zdi.h"
#include "GameZRecoil/zEffect/zeff.h"
#include "GameZRecoil/zError/zerr.h"
#include "GameZRecoil/zGame/zgame.h"
#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zLoc/zloc.h"
#include "GameZRecoil/zMath/zmth.h"
#include "GameZRecoil/zModel/gmod.h"
#include "GameZRecoil/zReader/zreader.h"
#include "GameZRecoil/zSound/zsnd.h"
#include "GameZRecoil/zTime/time.h"
#include "GameZRecoil/zUtil/zbd.h"
#include "GameZRecoil/zVideo/zvid.h"
#include "hud.h"
#include "hud_sensor_tracker.h"
#include "opt_catalog.h"

#include <algorithm>
#include <atlbase.h>
#include <ctype.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
extern char g_HudUiCounterText_PlayerLabel[];

#include "GameZRecoil/zCom/zCom.h"

/**
 *
 * Purpose: resolve an interface-map entry for a requested IID and AddRef the
 * adjusted interface pointer returned to the caller.
 */
HRESULT WINAPI zCom::QueryInterfaceFromInterfaceMap(
    void* objectBase,
    const InterfaceMapEntry* interfaceMap,
    const GUID* requestedIid,
    void** outInterface
)
{
    if (outInterface == 0) {
        return E_POINTER;
    }

    *outInterface = 0;

    const unsigned int* const requestedWords = (const unsigned int*)(requestedIid);
    unsigned int resolverRaw;
    const InterfaceMapEntry* currentEntry;
    if (requestedWords[0] == 0 && requestedWords[1] == 0 && requestedWords[2] == 0x000000c0
        && requestedWords[3] == 0x46000000) {
        IUnknown* const resolvedInterface = (IUnknown*)((DWORD)objectBase + interfaceMap->interfaceOffset);
        resolvedInterface->AddRef();
        *outInterface = resolvedInterface;
        return S_OK;
    }

    currentEntry = interfaceMap;
    while ((resolverRaw = currentEntry->resolverRaw) != ZCOM_INTERFACE_MAP_END) {
        const GUID* entryIid = currentEntry->iid;
        int blindEntry = entryIid == 0;
        const unsigned int* const entryWords = (const unsigned int*)(entryIid);
        if (blindEntry != 0
            || (entryWords[0] == requestedWords[0] && entryWords[1] == requestedWords[1]
                && entryWords[2] == requestedWords[2] && entryWords[3] == requestedWords[3])) {
            if (resolverRaw == ZCOM_INTERFACE_MAP_DIRECT) {
                IUnknown* const resolvedInterface = (IUnknown*)((DWORD)objectBase + currentEntry->interfaceOffset);
                resolvedInterface->AddRef();
                *outInterface = resolvedInterface;
                return S_OK;
            }

            QueryInterfaceResolver resolver = (QueryInterfaceResolver)(resolverRaw);
            const HRESULT result = resolver(objectBase, requestedIid, outInterface, currentEntry->interfaceOffset);
            if (result == S_OK || (!blindEntry && result < 0)) {
                return result;
            }
        }

        ++currentEntry;
    }

    return E_NOINTERFACE;
}

/**
 * VC5SP3 ATL's AtlAdvise source uses two independently unwindable CComPtr
 * locals in this order. Retail has the same two one-pointer cleanup actions.
 * Purpose: query a source for IConnectionPointContainer, find the requested
 * connection point, and advise the sink while releasing temporary interfaces.
 */
HRESULT WINAPI
zCom::ConnectionPointContainerAdvise(IUnknown* source, IUnknown* sink, REFIID connectionPointIid, DWORD* cookie)
{
    CComPtr<IConnectionPointContainer> connectionPointContainer;
    CComPtr<IConnectionPoint> connectionPoint;
    HRESULT result = source->QueryInterface(IID_IConnectionPointContainer, (void**)(&connectionPointContainer));
    if (SUCCEEDED(result))
        result = connectionPointContainer->FindConnectionPoint(connectionPointIid, &connectionPoint);
    if (SUCCEEDED(result))
        result = connectionPoint->Advise(sink, cookie);

    return result;
}

/**
 * VC5SP3 ATL's AtlUnadvise source uses the same two independent CComPtr
 * lifetimes and cleanup order as the retail function.
 * Purpose: query a source for IConnectionPointContainer, find the requested
 * connection point, and unadvise the cookie while releasing temporary interfaces.
 */
HRESULT WINAPI zCom::ConnectionPointContainerUnadvise(IUnknown* source, REFIID connectionPointIid, DWORD cookie)
{
    CComPtr<IConnectionPointContainer> connectionPointContainer;
    CComPtr<IConnectionPoint> connectionPoint;
    HRESULT result = source->QueryInterface(IID_IConnectionPointContainer, (void**)(&connectionPointContainer));
    if (SUCCEEDED(result))
        result = connectionPointContainer->FindConnectionPoint(connectionPointIid, &connectionPoint);
    if (SUCCEEDED(result))
        result = connectionPoint->Unadvise(cookie);

    return result;
}

/**
 * @recoil-anchor recoil:anchor:battlesport.recoil.westwood-online-upgrade-api-init-state-init
 * @recoil-artifact defines .text recoil:function:0x42dda0: WestwoodOnlineUpgradeApiInitState::Init.
 * @recoil-match byte
 *
 * Purpose: validate and initialize the transient WOL bootstrap-state block,
 * module handles, event-sink live count, and critical sections.
 */
HRESULT __stdcall WestwoodOnlineUpgradeApiInitState::Init(
    WestwoodOnlineUpgradeApiInitState* self,
    HANDLE bootstrapServerListEvent,
    HINSTANCE moduleHandle
)
{
    if (self == 0) {
        return E_INVALIDARG;
    }

    if (self->structSize < sizeof(WestwoodOnlineUpgradeApiInitState)) {
        return E_INVALIDARG;
    }

    self->bootstrapServerListEvent = bootstrapServerListEvent;
    self->moduleHandlePrimary = self->moduleHandleTertiary = self->moduleHandleSecondary = moduleHandle;
    self->eventSinkLiveCount = 0;
    self->failureEvent = 0;
    InitializeCriticalSection(&self->criticalSection0);
    InitializeCriticalSection(&self->criticalSection1);
    InitializeCriticalSection(&self->criticalSection2);
    return S_OK;
}
