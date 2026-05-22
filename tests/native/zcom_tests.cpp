#include "GameZRecoil/zCom/zCom.h"

namespace {
struct TestInterface {
    struct TestVtable *vftable;
    int addRefCount;
};

struct TestVtable {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IUnknown *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IUnknown *);
    ULONG(STDMETHODCALLTYPE *Release)(IUnknown *);
};

HRESULT STDMETHODCALLTYPE TestQueryInterface(IUnknown *, REFIID, void **) {
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE TestAddRef(IUnknown *self) {
    auto *testInterface = reinterpret_cast<TestInterface *>(self);
    return ++testInterface->addRefCount;
}

ULONG STDMETHODCALLTYPE TestRelease(IUnknown *) {
    return 1;
}

TestVtable g_testVtable = {
    TestQueryInterface,
    TestAddRef,
    TestRelease,
};

constexpr GUID kTestIid = {
    0x10203040,
    0x5060,
    0x7080,
    {0x90, 0xa0, 0xb0, 0xc0, 0xd0, 0xe0, 0xf0, 0x11},
};

constexpr GUID kIidUnknown = {
    0,
    0,
    0,
    {0xc0, 0, 0, 0, 0, 0, 0, 0x46},
};

struct FakeUnknownVtable {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IUnknown *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IUnknown *);
    ULONG(STDMETHODCALLTYPE *Release)(IUnknown *);
};

struct FakeConnectionPointContainerVtable {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IConnectionPointContainer *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IConnectionPointContainer *);
    ULONG(STDMETHODCALLTYPE *Release)(IConnectionPointContainer *);
    HRESULT(STDMETHODCALLTYPE *EnumConnectionPoints)(IConnectionPointContainer *, void **);
    HRESULT(STDMETHODCALLTYPE *FindConnectionPoint)(IConnectionPointContainer *, REFIID,
                                                    IConnectionPoint **);
};

struct FakeConnectionPointVtable {
    HRESULT(STDMETHODCALLTYPE *QueryInterface)(IConnectionPoint *, REFIID, void **);
    ULONG(STDMETHODCALLTYPE *AddRef)(IConnectionPoint *);
    ULONG(STDMETHODCALLTYPE *Release)(IConnectionPoint *);
    HRESULT(STDMETHODCALLTYPE *GetConnectionInterface)(IConnectionPoint *, IID *);
    HRESULT(STDMETHODCALLTYPE *GetConnectionPointContainer)(IConnectionPoint *,
                                                            IConnectionPointContainer **);
    HRESULT(STDMETHODCALLTYPE *Advise)(IConnectionPoint *, IUnknown *, DWORD *);
    HRESULT(STDMETHODCALLTYPE *Unadvise)(IConnectionPoint *, DWORD);
    HRESULT(STDMETHODCALLTYPE *EnumConnections)(IConnectionPoint *, void **);
};

struct FakeUnknown {
    FakeUnknownVtable *vftable;
};

struct FakeConnectionPointContainer {
    FakeConnectionPointContainerVtable *vftable;
};

struct FakeConnectionPoint {
    FakeConnectionPointVtable *vftable;
};

FakeConnectionPointContainer g_fakeCpc;
FakeConnectionPoint g_fakeCp;
IUnknown *g_adviseSink;
DWORD *g_adviseCookiePtr;
int g_sourceQueryInterfaceCalls;
int g_findConnectionPointCalls;
int g_adviseCalls;
int g_unadviseCalls;
DWORD g_unadviseCookie;
int g_cpcReleaseCalls;
int g_cpReleaseCalls;

HRESULT STDMETHODCALLTYPE FakeSourceQueryInterface(IUnknown *, REFIID iid, void **out) {
    ++g_sourceQueryInterfaceCalls;
    if (IsEqualGUID(iid, __uuidof(IConnectionPointContainer))) {
        *out = &g_fakeCpc;
        return S_OK;
    }

    *out = nullptr;
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FakeUnknownAddRef(IUnknown *) {
    return 1;
}

ULONG STDMETHODCALLTYPE FakeUnknownRelease(IUnknown *) {
    return 1;
}

HRESULT STDMETHODCALLTYPE FakeCpcQueryInterface(IConnectionPointContainer *, REFIID, void **) {
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FakeCpcAddRef(IConnectionPointContainer *) {
    return 1;
}

ULONG STDMETHODCALLTYPE FakeCpcRelease(IConnectionPointContainer *) {
    ++g_cpcReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE FakeEnumConnectionPoints(IConnectionPointContainer *, void **) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE FakeFindConnectionPoint(IConnectionPointContainer *, REFIID iid,
                                                  IConnectionPoint **out) {
    ++g_findConnectionPointCalls;
    if (IsEqualGUID(iid, kTestIid)) {
        *out = reinterpret_cast<IConnectionPoint *>(&g_fakeCp);
        return S_OK;
    }

    *out = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE FakeCpQueryInterface(IConnectionPoint *, REFIID, void **) {
    return E_NOINTERFACE;
}

ULONG STDMETHODCALLTYPE FakeCpAddRef(IConnectionPoint *) {
    return 1;
}

ULONG STDMETHODCALLTYPE FakeCpRelease(IConnectionPoint *) {
    ++g_cpReleaseCalls;
    return 1;
}

HRESULT STDMETHODCALLTYPE FakeGetConnectionInterface(IConnectionPoint *, IID *) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE FakeGetConnectionPointContainer(IConnectionPoint *,
                                                          IConnectionPointContainer **) {
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE FakeAdvise(IConnectionPoint *, IUnknown *sink, DWORD *cookie) {
    ++g_adviseCalls;
    g_adviseSink = sink;
    g_adviseCookiePtr = cookie;
    *cookie = 0x12345678;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FakeUnadvise(IConnectionPoint *, DWORD cookie) {
    ++g_unadviseCalls;
    g_unadviseCookie = cookie;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FakeEnumConnections(IConnectionPoint *, void **) {
    return E_NOTIMPL;
}

FakeUnknownVtable g_fakeUnknownVtable = {
    FakeSourceQueryInterface,
    FakeUnknownAddRef,
    FakeUnknownRelease,
};

FakeConnectionPointContainerVtable g_fakeCpcVtable = {
    FakeCpcQueryInterface,    FakeCpcAddRef,           FakeCpcRelease,
    FakeEnumConnectionPoints, FakeFindConnectionPoint,
};

FakeConnectionPointVtable g_fakeCpVtable = {
    FakeCpQueryInterface,
    FakeCpAddRef,
    FakeCpRelease,
    FakeGetConnectionInterface,
    FakeGetConnectionPointContainer,
    FakeAdvise,
    FakeUnadvise,
    FakeEnumConnections,
};
} // namespace

extern "C" int zcom_query_interface_from_interface_map_smoke(void) {
    TestInterface object{&g_testVtable, 0};
    zCom::InterfaceMapEntry map[] = {
        {&kTestIid, 0, zCom::ZCOM_INTERFACE_MAP_DIRECT},
        {nullptr, 0, zCom::ZCOM_INTERFACE_MAP_END},
    };

    void *outInterface = reinterpret_cast<void *>(0xcccccccc);
    HRESULT result = zCom::QueryInterfaceFromInterfaceMap(&object, map, &kTestIid, &outInterface);
    if (result != S_OK || outInterface != &object || object.addRefCount != 1) {
        return 1;
    }

    outInterface = reinterpret_cast<void *>(0xcccccccc);
    result = zCom::QueryInterfaceFromInterfaceMap(&object, map, &kIidUnknown, &outInterface);
    if (result != S_OK || outInterface != &object || object.addRefCount != 2) {
        return 2;
    }

    GUID otherIid = kTestIid;
    otherIid.Data1 ^= 1;
    outInterface = reinterpret_cast<void *>(0xcccccccc);
    result = zCom::QueryInterfaceFromInterfaceMap(&object, map, &otherIid, &outInterface);
    if (result != E_NOINTERFACE || outInterface != nullptr) {
        return 3;
    }

    result = zCom::QueryInterfaceFromInterfaceMap(&object, map, &kTestIid, nullptr);
    return result == E_POINTER ? 0 : 4;
}

extern "C" int zcom_connection_point_container_advise_smoke(void) {
    FakeUnknown source{&g_fakeUnknownVtable};
    FakeUnknown sink{&g_fakeUnknownVtable};
    g_fakeCpc = FakeConnectionPointContainer{&g_fakeCpcVtable};
    g_fakeCp = FakeConnectionPoint{&g_fakeCpVtable};
    g_adviseSink = nullptr;
    g_adviseCookiePtr = nullptr;
    g_sourceQueryInterfaceCalls = 0;
    g_findConnectionPointCalls = 0;
    g_adviseCalls = 0;
    g_unadviseCalls = 0;
    g_unadviseCookie = 0;
    g_cpcReleaseCalls = 0;
    g_cpReleaseCalls = 0;
    DWORD cookie = 0;

    HRESULT result =
        zCom::ConnectionPointContainer_Advise(reinterpret_cast<IUnknown *>(&source),
                                              reinterpret_cast<IUnknown *>(&sink), kTestIid,
                                              &cookie);

    if (result != S_OK || cookie != 0x12345678) {
        return 1;
    }

    if (g_sourceQueryInterfaceCalls != 1 || g_findConnectionPointCalls != 1 || g_adviseCalls != 1) {
        return 2;
    }

    if (g_adviseSink != reinterpret_cast<IUnknown *>(&sink) || g_adviseCookiePtr != &cookie) {
        return 3;
    }

    return g_cpReleaseCalls == 1 && g_cpcReleaseCalls == 1 ? 0 : 4;
}

extern "C" int zcom_connection_point_container_unadvise_smoke(void) {
    FakeUnknown source{&g_fakeUnknownVtable};
    g_fakeCpc = FakeConnectionPointContainer{&g_fakeCpcVtable};
    g_fakeCp = FakeConnectionPoint{&g_fakeCpVtable};
    g_sourceQueryInterfaceCalls = 0;
    g_findConnectionPointCalls = 0;
    g_adviseCalls = 0;
    g_unadviseCalls = 0;
    g_unadviseCookie = 0;
    g_cpcReleaseCalls = 0;
    g_cpReleaseCalls = 0;

    HRESULT result = zCom::ConnectionPointContainer_Unadvise(reinterpret_cast<IUnknown *>(&source),
                                                             kTestIid, 0x89abcdef);

    if (result != S_OK || g_unadviseCookie != 0x89abcdef) {
        return 1;
    }

    if (g_sourceQueryInterfaceCalls != 1 || g_findConnectionPointCalls != 1 ||
        g_unadviseCalls != 1 || g_adviseCalls != 0) {
        return 2;
    }

    return g_cpReleaseCalls == 1 && g_cpcReleaseCalls == 1 ? 0 : 3;
}
