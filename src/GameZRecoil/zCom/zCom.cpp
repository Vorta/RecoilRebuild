#include "GameZRecoil/zCom/zCom.h"

namespace {
template <typename T> struct ComReleaseOnExit {
    T *ptr;

    ~ComReleaseOnExit() {
        if (ptr != 0) {
            ptr->Release();
        }
    }
};

} // namespace

// Reimplements 0x42db50: zCom::QueryInterfaceFromInterfaceMap
HRESULT WINAPI zCom::QueryInterfaceFromInterfaceMap(void *objectBase,
                                                    const InterfaceMapEntry *interfaceMap,
                                                    const GUID *requestedIid, void **outInterface) {
    if (outInterface == 0) {
        return E_POINTER;
    }

    *outInterface = 0;

    const unsigned int *const requestedWords = (const unsigned int *)(requestedIid);
    unsigned int resolverRaw;
    const InterfaceMapEntry *currentEntry;
    if (requestedWords[0] == 0 && requestedWords[1] == 0 && requestedWords[2] == 0x000000c0 &&
        requestedWords[3] == 0x46000000) {
        IUnknown *const resolvedInterface = (IUnknown *)((DWORD)objectBase + interfaceMap->interfaceOffset);
        resolvedInterface->AddRef();
        *outInterface = resolvedInterface;
        return S_OK;
    }

    currentEntry = interfaceMap;
    while ((resolverRaw = currentEntry->resolverRaw) != ZCOM_INTERFACE_MAP_END) {
        const GUID *entryIid = currentEntry->iid;
        int blindEntry = entryIid == 0;
        const unsigned int *const entryWords = (const unsigned int *)(entryIid);
        if (blindEntry != 0 ||
            (entryWords[0] == requestedWords[0] && entryWords[1] == requestedWords[1] &&
             entryWords[2] == requestedWords[2] && entryWords[3] == requestedWords[3])) {
            if (resolverRaw == ZCOM_INTERFACE_MAP_DIRECT) {
                IUnknown *const resolvedInterface =
                    (IUnknown *)((DWORD)objectBase + currentEntry->interfaceOffset);
                resolvedInterface->AddRef();
                *outInterface = resolvedInterface;
                return S_OK;
            }

            QueryInterfaceResolver resolver = (QueryInterfaceResolver)(resolverRaw);
            const HRESULT result =
                resolver(objectBase, requestedIid, outInterface, currentEntry->interfaceOffset);
            if (result == S_OK || (!blindEntry && result < 0)) {
                return result;
            }
        }

        ++currentEntry;
    }

    return E_NOINTERFACE;
}

// Reimplements 0x42dc30: zCom::ConnectionPointContainer_Advise
HRESULT WINAPI zCom::ConnectionPointContainer_Advise(IUnknown *source, IUnknown *sink,
                                                     REFIID connectionPointIid, DWORD *cookie) {
    ComReleaseOnExit<IConnectionPointContainer> cpc = {0};
    ComReleaseOnExit<IConnectionPoint> cp = {0};

    HRESULT result = source->QueryInterface(IID_IConnectionPointContainer,
                                            (void **)(&cpc.ptr));
    if (result >= 0) {
        result = cpc.ptr->FindConnectionPoint(connectionPointIid, &cp.ptr);
        if (result >= 0) {
            result = cp.ptr->Advise(sink, cookie);
        }
    }

    return result;
}

// Reimplements 0x42dcf0: zCom::ConnectionPointContainer_Unadvise
HRESULT WINAPI zCom::ConnectionPointContainer_Unadvise(IUnknown *source, REFIID connectionPointIid,
                                                       DWORD cookie) {
    ComReleaseOnExit<IConnectionPointContainer> cpc = {0};
    ComReleaseOnExit<IConnectionPoint> cp = {0};

    HRESULT result = source->QueryInterface(IID_IConnectionPointContainer,
                                            (void **)(&cpc.ptr));
    if (result >= 0) {
        result = cpc.ptr->FindConnectionPoint(connectionPointIid, &cp.ptr);
        if (result >= 0) {
            result = cp.ptr->Unadvise(cookie);
        }
    }

    return result;
}
