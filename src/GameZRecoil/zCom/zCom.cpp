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

bool IsIidUnknown(const GUID *iid) {
    const unsigned int * words = reinterpret_cast<const unsigned int *>(iid);
    return words[0] == 0 && words[1] == 0 && words[2] == 0x000000c0 && words[3] == 0x46000000;
}

bool IsSameIid(const GUID *lhs, const GUID *rhs) {
    const unsigned int * lhsWords = reinterpret_cast<const unsigned int *>(lhs);
    const unsigned int * rhsWords = reinterpret_cast<const unsigned int *>(rhs);
    return lhsWords[0] == rhsWords[0] && lhsWords[1] == rhsWords[1] && lhsWords[2] == rhsWords[2] &&
           lhsWords[3] == rhsWords[3];
}
} // namespace

// Reimplements 0x42db50: zCom::QueryInterfaceFromInterfaceMap
HRESULT WINAPI zCom::QueryInterfaceFromInterfaceMap(void *objectBase,
                                                    const InterfaceMapEntry *interfaceMap,
                                                    const GUID *requestedIid, void **outInterface) {
    if (outInterface == 0) {
        return E_POINTER;
    }

    *outInterface = 0;

    int interfaceOffset;
    if (IsIidUnknown(requestedIid)) {
        interfaceOffset = interfaceMap->interfaceOffset;
    } else {
        unsigned int resolverRaw = interfaceMap->resolverRaw;
        if (resolverRaw == ZCOM_INTERFACE_MAP_END) {
            return E_NOINTERFACE;
        }

        const InterfaceMapEntry *currentEntry = interfaceMap;
        while (true) {
            const GUID *entryIid = currentEntry->iid;
            const bool blindEntry = entryIid == 0;
            if (blindEntry || IsSameIid(entryIid, requestedIid)) {
                if (resolverRaw == ZCOM_INTERFACE_MAP_DIRECT) {
                    interfaceOffset = currentEntry->interfaceOffset;
                    break;
                }

                QueryInterfaceResolver resolver = reinterpret_cast<QueryInterfaceResolver>(resolverRaw);
                const HRESULT result =
                    resolver(objectBase, requestedIid, outInterface, currentEntry->interfaceOffset);
                if (result == S_OK) {
                    return result;
                }

                if (!blindEntry && result < 0) {
                    return result;
                }
            }

            ++currentEntry;
            resolverRaw = currentEntry->resolverRaw;
            if (resolverRaw == ZCOM_INTERFACE_MAP_END) {
                return E_NOINTERFACE;
            }
        }
    }

    IUnknown *const resolvedInterface = reinterpret_cast<IUnknown *>(static_cast<unsigned char *>(objectBase) + interfaceOffset);
    resolvedInterface->AddRef();
    *outInterface = resolvedInterface;
    return S_OK;
}

// Reimplements 0x42dc30: zCom::ConnectionPointContainer_Advise
HRESULT WINAPI zCom::ConnectionPointContainer_Advise(IUnknown *source, REFIID connectionPointIid,
                                                     IUnknown *sink, DWORD *cookie) {
    ComReleaseOnExit<IConnectionPointContainer> cpc = {0};
    ComReleaseOnExit<IConnectionPoint> cp = {0};

    HRESULT result = source->QueryInterface(__uuidof(IConnectionPointContainer),
                                            reinterpret_cast<void **>(&cpc.ptr));
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

    HRESULT result = source->QueryInterface(__uuidof(IConnectionPointContainer),
                                            reinterpret_cast<void **>(&cpc.ptr));
    if (result >= 0) {
        result = cpc.ptr->FindConnectionPoint(connectionPointIid, &cp.ptr);
        if (result >= 0) {
            result = cp.ptr->Unadvise(cookie);
        }
    }

    return result;
}
