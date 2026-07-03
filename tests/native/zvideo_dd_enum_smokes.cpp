#include "GameZRecoil/zVideo/zvid.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <ddraw.h>

namespace {
struct FakeD3D2Object {
    void **vtable;
};

struct FakeDirectDraw2Object {
    void **vtable;
};

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
    int active;
};

void *gFakeD3D2VTable[4];
void *gFakeDirectDraw2VTable[24];

int gFakeTeardownVideoSubsystemCalls;
int gFakeCreateDirectDraw2ForSelectedDeviceCalls;
int gFakeEnumerateDirect3DDevicesForRecordCalls;
zVidHwApiDeviceRecordPartial *gFakeEnumerateDirect3DDevicesForRecordEntry;
int gFakeEnumerateDirect3DDevicesForRecordResult;

HRESULT gFakeD3D2EnumDevicesResult;
int gFakeD3D2ReleaseCalls;
int gFakeD3D2EnumDevicesCalls;
LPD3DENUMDEVICESCALLBACK gFakeD3D2LastEnumDevicesCallback;
void *gFakeD3D2LastEnumDevicesContext;
int gFakeD3D2EnumDevicesInitialAcceptedCount;
int gFakeD3D2EnumDevicesAcceptedCount;

HRESULT gFakeDirectDraw2QueryInterfaceResult;
int gFakeDirectDraw2QueryInterfaceCalls;
GUID gFakeDirectDraw2LastQueryInterfaceIidValue;
const GUID *gFakeDirectDraw2LastQueryInterfaceIid;
void **gFakeDirectDraw2LastQueryInterfaceOut;
void *gFakeDirectDraw2QueryInterfaceValue;

HRESULT gFakeDirectDraw2GetCapsResult;
int gFakeDirectDraw2GetCapsCalls;
DDCAPS gFakeDirectDraw2GetCapsHalInput;
DDCAPS gFakeDirectDraw2GetCapsHelInput;
DDCAPS gFakeDirectDraw2GetCapsHalValue;
DDCAPS gFakeDirectDraw2GetCapsHelValue;

HRESULT gFakeDirectDraw2GetAvailableVidMemResult;
int gFakeDirectDraw2GetAvailableVidMemCalls;
DDSCAPS gFakeDirectDraw2LastAvailableVidMemCapsValue;
DWORD gFakeDirectDraw2AvailableVidMemTotal;
DWORD gFakeDirectDraw2AvailableVidMemFree;

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = reinterpret_cast<unsigned char *>(target);
    patch.active = 0;
    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        return false;
    }

    std::memcpy(patch.original, patch.address, sizeof(patch.original));
    const std::intptr_t relative =
        reinterpret_cast<unsigned char *>(replacement) - patch.address - 5;
    patch.address[0] = 0xe9;
    std::memcpy(patch.address + 1, &relative, 4);
    FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
    DWORD ignored = 0;
    VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    patch.active = 1;
    return true;
}

void RestoreFunctionPatch(
    CodeFunctionPatch &patch
) {
    if (patch.active == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(patch.address, patch.original, sizeof(patch.original));
        FlushInstructionCache(GetCurrentProcess(), patch.address, sizeof(patch.original));
        DWORD ignored = 0;
        VirtualProtect(patch.address, sizeof(patch.original), oldProtect, &ignored);
    }
    patch.active = 0;
}

void FakeTeardownVideoSubsystem() {
    ++gFakeTeardownVideoSubsystemCalls;
}

int FakeCreateDirectDraw2ForSelectedDevice() {
    ++gFakeCreateDirectDraw2ForSelectedDeviceCalls;
    return 0;
}

int __fastcall FakeEnumerateDirect3DDevicesForRecord(
    zVidHwApiDeviceRecordPartial *entry
) {
    ++gFakeEnumerateDirect3DDevicesForRecordCalls;
    gFakeEnumerateDirect3DDevicesForRecordEntry = entry;
    return gFakeEnumerateDirect3DDevicesForRecordResult;
}

ULONG __stdcall FakeD3D2_Release(
    IDirect3D2 *
) {
    ++gFakeD3D2ReleaseCalls;
    return 1;
}

HRESULT __stdcall FakeD3D2_EnumDevices(
    IDirect3D2 *,
    LPD3DENUMDEVICESCALLBACK callback,
    void *context
) {
    ++gFakeD3D2EnumDevicesCalls;
    gFakeD3D2LastEnumDevicesCallback = callback;
    gFakeD3D2LastEnumDevicesContext = context;
    zVidHwApiDeviceRecordPartial *entry =
        reinterpret_cast<zVidHwApiDeviceRecordPartial *>(context);
    gFakeD3D2EnumDevicesInitialAcceptedCount = entry->m_acceptedD3DDeviceCount;
    entry->m_acceptedD3DDeviceCount = gFakeD3D2EnumDevicesAcceptedCount;
    return gFakeD3D2EnumDevicesResult;
}

HRESULT __stdcall FakeDirectDraw2_QueryInterface(
    IDirectDraw2 *,
    REFIID iid,
    void **outInterface
) {
    ++gFakeDirectDraw2QueryInterfaceCalls;
    gFakeDirectDraw2LastQueryInterfaceIidValue = iid;
    gFakeDirectDraw2LastQueryInterfaceIid = &gFakeDirectDraw2LastQueryInterfaceIidValue;
    gFakeDirectDraw2LastQueryInterfaceOut = outInterface;
    if (gFakeDirectDraw2QueryInterfaceResult == DD_OK) {
        *outInterface = gFakeDirectDraw2QueryInterfaceValue;
    }
    return gFakeDirectDraw2QueryInterfaceResult;
}

HRESULT __stdcall FakeDirectDraw2_GetCaps(
    IDirectDraw2 *,
    LPDDCAPS halCaps,
    LPDDCAPS helCaps
) {
    ++gFakeDirectDraw2GetCapsCalls;
    if (halCaps != 0) {
        gFakeDirectDraw2GetCapsHalInput = *halCaps;
    }
    if (helCaps != 0) {
        gFakeDirectDraw2GetCapsHelInput = *helCaps;
    }
    if (gFakeDirectDraw2GetCapsResult == DD_OK) {
        *halCaps = gFakeDirectDraw2GetCapsHalValue;
        *helCaps = gFakeDirectDraw2GetCapsHelValue;
    }
    return gFakeDirectDraw2GetCapsResult;
}

HRESULT __stdcall FakeDirectDraw2_GetAvailableVidMem(
    IDirectDraw2 *,
    LPDDSCAPS caps,
    LPDWORD totalBytes,
    LPDWORD freeBytes
) {
    ++gFakeDirectDraw2GetAvailableVidMemCalls;
    gFakeDirectDraw2LastAvailableVidMemCapsValue = *caps;
    if (gFakeDirectDraw2GetAvailableVidMemResult == DD_OK) {
        *totalBytes = gFakeDirectDraw2AvailableVidMemTotal;
        *freeBytes = gFakeDirectDraw2AvailableVidMemFree;
    }
    return gFakeDirectDraw2GetAvailableVidMemResult;
}

void InstallFakeD3D2(
    FakeD3D2Object &d3d
) {
    std::memset(gFakeD3D2VTable, 0, sizeof(gFakeD3D2VTable));
    gFakeD3D2VTable[2] = reinterpret_cast<void *>(FakeD3D2_Release);
    gFakeD3D2VTable[3] = reinterpret_cast<void *>(FakeD3D2_EnumDevices);
    d3d.vtable = gFakeD3D2VTable;

    gFakeD3D2EnumDevicesResult = DD_OK;
    gFakeD3D2ReleaseCalls = 0;
    gFakeD3D2EnumDevicesCalls = 0;
    gFakeD3D2LastEnumDevicesCallback = 0;
    gFakeD3D2LastEnumDevicesContext = 0;
    gFakeD3D2EnumDevicesInitialAcceptedCount = -1;
    gFakeD3D2EnumDevicesAcceptedCount = 0;
}

void InstallFakeDirectDraw2(
    FakeDirectDraw2Object &directDraw
) {
    std::memset(gFakeDirectDraw2VTable, 0, sizeof(gFakeDirectDraw2VTable));
    gFakeDirectDraw2VTable[0] =
        reinterpret_cast<void *>(FakeDirectDraw2_QueryInterface);
    gFakeDirectDraw2VTable[11] = reinterpret_cast<void *>(FakeDirectDraw2_GetCaps);
    gFakeDirectDraw2VTable[23] =
        reinterpret_cast<void *>(FakeDirectDraw2_GetAvailableVidMem);
    directDraw.vtable = gFakeDirectDraw2VTable;

    gFakeDirectDraw2QueryInterfaceResult = DD_OK;
    gFakeDirectDraw2QueryInterfaceCalls = 0;
    gFakeDirectDraw2LastQueryInterfaceIidValue = GUID();
    gFakeDirectDraw2LastQueryInterfaceIid = 0;
    gFakeDirectDraw2LastQueryInterfaceOut = 0;
    gFakeDirectDraw2QueryInterfaceValue = 0;

    gFakeDirectDraw2GetCapsResult = DD_OK;
    gFakeDirectDraw2GetCapsCalls = 0;
    gFakeDirectDraw2GetCapsHalInput = DDCAPS();
    gFakeDirectDraw2GetCapsHelInput = DDCAPS();
    gFakeDirectDraw2GetCapsHalValue = DDCAPS();
    gFakeDirectDraw2GetCapsHelValue = DDCAPS();
    gFakeDirectDraw2GetCapsHalValue.dwSize = sizeof(DDCAPS);
    gFakeDirectDraw2GetCapsHelValue.dwSize = sizeof(DDCAPS);

    gFakeDirectDraw2GetAvailableVidMemResult = DD_OK;
    gFakeDirectDraw2GetAvailableVidMemCalls = 0;
    gFakeDirectDraw2LastAvailableVidMemCapsValue = DDSCAPS();
    gFakeDirectDraw2AvailableVidMemTotal = 0;
    gFakeDirectDraw2AvailableVidMemFree = 0;
}
} // namespace

extern "C" int zvideo_dd_enum_direct3d_device_callback_smoke(void) {
    const int savedAcceptedHardwareCount = g_zVid_AcceptedHardwareRendererCount;
    CodeFunctionPatch teardownPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::TeardownVideoSubsystem),
            reinterpret_cast<void *>(FakeTeardownVideoSubsystem),
            teardownPatch
        )) {
        return 1;
    }

    GUID guid = {0x12345678, 0x1111, 0x2222, {3, 4, 5, 6, 7, 8, 9, 10}};
    zVidHwApiDeviceRecordPartial entry = {};
    D3DDEVICEDESC desc = {};
    g_zVid_AcceptedHardwareRendererCount = 0;
    gFakeTeardownVideoSubsystemCalls = 0;

    desc.dwFlags = 0;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    const bool noHardwareOk =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("desc-skip-hw"),
            const_cast<LPSTR>("name-skip-hw"),
            &desc,
            0,
            &entry
        ) == 1 &&
        entry.m_acceptedD3DDeviceCount == 0 &&
        g_zVid_AcceptedHardwareRendererCount == 0;

    desc = D3DDEVICEDESC();
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_MONO;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    const bool nonRgbOk =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("desc-skip-rgb"),
            const_cast<LPSTR>("name-skip-rgb"),
            &desc,
            0,
            &entry
        ) == 1 &&
        entry.m_acceptedD3DDeviceCount == 0 &&
        g_zVid_AcceptedHardwareRendererCount == 0;

    desc = D3DDEVICEDESC();
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = 0;
    const bool noZBufferOk =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("desc-skip-z"),
            const_cast<LPSTR>("name-skip-z"),
            &desc,
            0,
            &entry
        ) == 1 &&
        entry.m_acceptedD3DDeviceCount == 0 &&
        g_zVid_AcceptedHardwareRendererCount == 0;

    desc = D3DDEVICEDESC();
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    desc.dwMaxTextureWidth = 0;
    desc.dwMaxTextureHeight = 0;
    const bool acceptWithGuidResult =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("accepted-description"),
            const_cast<LPSTR>("accepted-name"),
            &desc,
            0,
            &entry
        ) == 1;
    D3DDEVICEDESC *storedDesc0 = &entry.m_d3dDrivers[0].m_hwDesc;
    const bool acceptWithGuidOk =
        acceptWithGuidResult &&
        entry.m_acceptedD3DDeviceCount == 1 &&
        g_zVid_AcceptedHardwareRendererCount == 1 &&
        entry.m_d3dDrivers[0].pD3DDeviceGuid ==
            &entry.m_d3dDrivers[0].m_d3dDeviceGuidStorage &&
        IsEqualGUID(entry.m_d3dDrivers[0].m_d3dDeviceGuidStorage, guid) &&
        std::strcmp(entry.m_d3dDrivers[0].m_deviceName, "accepted-name") == 0 &&
        std::strcmp(
            entry.m_d3dDrivers[0].m_deviceDescription,
            "accepted-description"
        ) == 0 &&
        storedDesc0->dwDeviceZBufferBitDepth == DDBD_16 &&
        storedDesc0->dwMaxTextureWidth == 0x100 &&
        storedDesc0->dwMaxTextureHeight == 0x100;

    desc = D3DDEVICEDESC();
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    desc.dwMaxTextureWidth = 64;
    desc.dwMaxTextureHeight = 128;
    const bool acceptNullGuidResult =
        zVideo_dd::EnumDirect3DDeviceCallback(
            0,
            const_cast<LPSTR>("second-description"),
            const_cast<LPSTR>("second-name"),
            &desc,
            0,
            &entry
        ) == 1;
    D3DDEVICEDESC *storedDesc1 = &entry.m_d3dDrivers[1].m_hwDesc;
    const bool acceptNullGuidOk =
        acceptNullGuidResult &&
        entry.m_acceptedD3DDeviceCount == 2 &&
        g_zVid_AcceptedHardwareRendererCount == 2 &&
        entry.m_d3dDrivers[1].pD3DDeviceGuid == 0 &&
        storedDesc1->dwMaxTextureWidth == 64 &&
        storedDesc1->dwMaxTextureHeight == 128;

    zVidHwApiDeviceRecordPartial fullEntry = {};
    fullEntry.m_acceptedD3DDeviceCount = 4;
    desc = D3DDEVICEDESC();
    desc.dwFlags = D3DDD_COLORMODEL;
    desc.dcmColorModel = D3DCOLOR_RGB;
    desc.dwDeviceZBufferBitDepth = DDBD_16;
    const bool capacityOk =
        zVideo_dd::EnumDirect3DDeviceCallback(
            &guid,
            const_cast<LPSTR>("desc-full"),
            const_cast<LPSTR>("name-full"),
            &desc,
            0,
            &fullEntry
        ) == 0 &&
        fullEntry.m_acceptedD3DDeviceCount == 4 &&
        gFakeTeardownVideoSubsystemCalls == 1;

    RestoreFunctionPatch(teardownPatch);
    g_zVid_AcceptedHardwareRendererCount = savedAcceptedHardwareCount;
    return noHardwareOk && nonRgbOk && noZBufferOk && acceptWithGuidOk &&
                   acceptNullGuidOk && capacityOk
               ? 0
               : 2;
}

extern "C" int zvideo_dd_enumerate_direct3d_devices_for_record_smoke(void) {
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    IDirect3D2 *const savedD3D = g_zVideo_pD3D2;

    FakeDirectDraw2Object directDraw = {};
    FakeD3D2Object d3d = {};
    InstallFakeDirectDraw2(directDraw);
    InstallFakeD3D2(d3d);
    gFakeDirectDraw2QueryInterfaceValue = reinterpret_cast<void *>(&d3d);
    gFakeD3D2EnumDevicesAcceptedCount = 2;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pD3D2 = 0;

    zVidHwApiDeviceRecordPartial acceptedEntry = {};
    std::strncpy(
        acceptedEntry.m_driverName,
        "driver-a",
        sizeof(acceptedEntry.m_driverName)
    );
    acceptedEntry.m_acceptedD3DDeviceCount = 99;

    const int acceptedResult =
        zVideo_dd::EnumerateDirect3DDevicesForRecord(&acceptedEntry);
    const bool acceptedOk =
        acceptedResult == 1 &&
        gFakeDirectDraw2QueryInterfaceCalls == 1 &&
        IsEqualGUID(*gFakeDirectDraw2LastQueryInterfaceIid, IID_IDirect3D2) &&
        gFakeDirectDraw2LastQueryInterfaceOut == (void **)(&g_zVideo_pD3D2) &&
        gFakeD3D2EnumDevicesCalls == 1 &&
        gFakeD3D2LastEnumDevicesCallback ==
            zVideo_dd::EnumDirect3DDeviceCallback &&
        gFakeD3D2LastEnumDevicesContext == &acceptedEntry &&
        gFakeD3D2EnumDevicesInitialAcceptedCount == 0 &&
        acceptedEntry.m_acceptedD3DDeviceCount == 2 &&
        gFakeD3D2ReleaseCalls == 1 &&
        g_zVideo_pD3D2 == 0;

    InstallFakeDirectDraw2(directDraw);
    InstallFakeD3D2(d3d);
    gFakeDirectDraw2QueryInterfaceValue = reinterpret_cast<void *>(&d3d);
    gFakeD3D2EnumDevicesAcceptedCount = 0;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pD3D2 = 0;

    zVidHwApiDeviceRecordPartial emptyEntry = {};
    std::strncpy(
        emptyEntry.m_driverName,
        "driver-empty",
        sizeof(emptyEntry.m_driverName)
    );
    emptyEntry.m_acceptedD3DDeviceCount = 7;

    const int emptyResult =
        zVideo_dd::EnumerateDirect3DDevicesForRecord(&emptyEntry);
    const bool emptyOk =
        emptyResult == 0 &&
        gFakeD3D2EnumDevicesCalls == 1 &&
        gFakeD3D2EnumDevicesInitialAcceptedCount == 0 &&
        emptyEntry.m_acceptedD3DDeviceCount == 0 &&
        gFakeD3D2ReleaseCalls == 1 &&
        g_zVideo_pD3D2 == 0;

    InstallFakeDirectDraw2(directDraw);
    InstallFakeD3D2(d3d);
    gFakeDirectDraw2QueryInterfaceResult = DDERR_GENERIC;
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_pD3D2 = reinterpret_cast<IDirect3D2 *>(&d3d);

    zVidHwApiDeviceRecordPartial failureEntry = {};
    std::strncpy(
        failureEntry.m_driverName,
        "driver-fail",
        sizeof(failureEntry.m_driverName)
    );
    failureEntry.m_acceptedD3DDeviceCount = 3;

    const int failureResult =
        zVideo_dd::EnumerateDirect3DDevicesForRecord(&failureEntry);
    const bool failureOk =
        failureResult == 0 &&
        gFakeDirectDraw2QueryInterfaceCalls == 1 &&
        gFakeD3D2EnumDevicesCalls == 0 &&
        gFakeD3D2ReleaseCalls == 0 &&
        failureEntry.m_acceptedD3DDeviceCount == 3 &&
        g_zVideo_pD3D2 == reinterpret_cast<IDirect3D2 *>(&d3d);

    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pD3D2 = savedD3D;
    return acceptedOk && emptyOk && failureOk ? 0 : 1;
}

extern "C" int zvideo_dd_enum_directdraw_device_callback_smoke(void) {
    const int savedAcceptedCount = g_zVideo_NumAcceptedDirectDrawDevices;
    const int savedOrdinal = g_zVideo_DirectDrawEnumOrdinal;
    IDirectDraw2 *const savedDirectDraw = g_zVideo_pDirectDraw2;
    zVidHwApiDeviceRecordPartial *const savedSelected =
        g_zVideo_pSelectedHwApiDeviceRecord;
    const DDCAPS savedHalCaps = g_zVideo_DDrawCapsHal;
    const DDCAPS savedHelCaps = g_zVideo_DDrawCapsHel;
    const zVidHwApiDeviceRecordPartial savedEntry0 = g_zVideo_HwApiDeviceTable[0];
    const zVidHwApiDeviceRecordPartial savedEntry1 = g_zVideo_HwApiDeviceTable[1];

    CodeFunctionPatch createPatch = {};
    CodeFunctionPatch enumPatch = {};
    CodeFunctionPatch teardownPatch = {};
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::CreateDirectDraw2ForSelectedDevice),
            reinterpret_cast<void *>(FakeCreateDirectDraw2ForSelectedDevice),
            createPatch
        )) {
        return 1;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::EnumerateDirect3DDevicesForRecord),
            reinterpret_cast<void *>(FakeEnumerateDirect3DDevicesForRecord),
            enumPatch
        )) {
        RestoreFunctionPatch(createPatch);
        return 2;
    }
    if (!PatchFunctionJump(
            reinterpret_cast<void *>(zVideo_dd::TeardownVideoSubsystem),
            reinterpret_cast<void *>(FakeTeardownVideoSubsystem),
            teardownPatch
        )) {
        RestoreFunctionPatch(enumPatch);
        RestoreFunctionPatch(createPatch);
        return 3;
    }

    g_zVideo_NumAcceptedDirectDrawDevices = 4;
    g_zVideo_DirectDrawEnumOrdinal = 5;
    gFakeCreateDirectDraw2ForSelectedDeviceCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordCalls = 0;
    gFakeTeardownVideoSubsystemCalls = 0;
    const BOOL capacityResult =
        zVideo_dd::EnumDirectDrawDeviceCallback(
            0,
            const_cast<LPSTR>("capacity-description"),
            const_cast<LPSTR>("capacity-driver"),
            0
        );
    const bool capacityOk =
        capacityResult == FALSE &&
        g_zVideo_DirectDrawEnumOrdinal == 6 &&
        g_zVideo_NumAcceptedDirectDrawDevices == 4 &&
        gFakeCreateDirectDraw2ForSelectedDeviceCalls == 0 &&
        gFakeEnumerateDirect3DDevicesForRecordCalls == 0 &&
        gFakeTeardownVideoSubsystemCalls == 0;

    FakeDirectDraw2Object directDraw = {};
    InstallFakeDirectDraw2(directDraw);
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_NumAcceptedDirectDrawDevices = 0;
    g_zVideo_DirectDrawEnumOrdinal = 8;
    gFakeCreateDirectDraw2ForSelectedDeviceCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordEntry = 0;
    gFakeEnumerateDirect3DDevicesForRecordResult = 1;
    gFakeTeardownVideoSubsystemCalls = 0;
    gFakeDirectDraw2GetCapsHalValue.dwCaps = 0x200;
    gFakeDirectDraw2AvailableVidMemTotal = 0x400000;
    gFakeDirectDraw2AvailableVidMemFree = 0x300000;
    std::memset(&g_zVideo_HwApiDeviceTable[0], 0x7f, sizeof(g_zVideo_HwApiDeviceTable[0]));
    GUID guid = {0x87654321, 0x3333, 0x4444, {10, 9, 8, 7, 6, 5, 4, 3}};

    const BOOL acceptedResult =
        zVideo_dd::EnumDirectDrawDeviceCallback(
            &guid,
            const_cast<LPSTR>("accepted-description"),
            const_cast<LPSTR>("driver"),
            reinterpret_cast<LPVOID>(0x1234)
        );
    zVidHwApiDeviceRecordPartial &entry0 = g_zVideo_HwApiDeviceTable[0];
    const bool acceptedOk =
        acceptedResult == TRUE &&
        g_zVideo_DirectDrawEnumOrdinal == 9 &&
        g_zVideo_NumAcceptedDirectDrawDevices == 1 &&
        entry0.pDirectDrawGuid == &entry0.m_directDrawGuidStorage &&
        IsEqualGUID(entry0.m_directDrawGuidStorage, guid) &&
        std::strcmp(entry0.m_driverName, "driver[AGP]") == 0 &&
        std::strcmp(entry0.m_driverDescription, "accepted-description") == 0 &&
        entry0.m_deviceFeatureFlags == 1 &&
        entry0.m_videoMemTotalBytes == 0x400000 &&
        entry0.m_videoMemFreeBytes == 0x300000 &&
        entry0.m_textureMemTotalBytes == 0x400000 &&
        entry0.m_textureMemFreeBytes == 0x300000 &&
        g_zVideo_pSelectedHwApiDeviceRecord == &entry0 &&
        gFakeCreateDirectDraw2ForSelectedDeviceCalls == 1 &&
        gFakeDirectDraw2GetCapsCalls == 1 &&
        gFakeDirectDraw2GetCapsHalInput.dwSize == sizeof(DDCAPS) &&
        gFakeDirectDraw2GetCapsHelInput.dwSize == sizeof(DDCAPS) &&
        gFakeDirectDraw2GetAvailableVidMemCalls == 2 &&
        gFakeDirectDraw2LastAvailableVidMemCapsValue.dwCaps == DDSCAPS_TEXTURE &&
        gFakeEnumerateDirect3DDevicesForRecordCalls == 1 &&
        gFakeEnumerateDirect3DDevicesForRecordEntry == &entry0 &&
        gFakeTeardownVideoSubsystemCalls == 1;

    InstallFakeDirectDraw2(directDraw);
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_NumAcceptedDirectDrawDevices = 1;
    g_zVideo_DirectDrawEnumOrdinal = 2;
    gFakeCreateDirectDraw2ForSelectedDeviceCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordEntry = 0;
    gFakeEnumerateDirect3DDevicesForRecordResult = 0;
    gFakeTeardownVideoSubsystemCalls = 0;
    gFakeDirectDraw2GetAvailableVidMemResult = DDERR_INVALIDPARAMS;
    std::memset(&g_zVideo_HwApiDeviceTable[1], 0x7f, sizeof(g_zVideo_HwApiDeviceTable[1]));

    const BOOL rejectedResult =
        zVideo_dd::EnumDirectDrawDeviceCallback(
            0,
            const_cast<LPSTR>("rejected-description"),
            const_cast<LPSTR>("rejected-driver"),
            0
        );
    zVidHwApiDeviceRecordPartial &entry1 = g_zVideo_HwApiDeviceTable[1];
    const bool rejectedOk =
        rejectedResult == TRUE &&
        g_zVideo_DirectDrawEnumOrdinal == 3 &&
        g_zVideo_NumAcceptedDirectDrawDevices == 1 &&
        entry1.pDirectDrawGuid == 0 &&
        std::strcmp(entry1.m_driverName, "rejected-driver") == 0 &&
        std::strcmp(entry1.m_driverDescription, "rejected-description") == 0 &&
        entry1.m_deviceFeatureFlags == 0 &&
        entry1.m_videoMemTotalBytes == 0 &&
        entry1.m_videoMemFreeBytes == 0 &&
        entry1.m_textureMemTotalBytes == 0 &&
        entry1.m_textureMemFreeBytes == 0 &&
        gFakeDirectDraw2GetAvailableVidMemCalls == 2 &&
        gFakeEnumerateDirect3DDevicesForRecordCalls == 1 &&
        gFakeEnumerateDirect3DDevicesForRecordEntry == &entry1 &&
        gFakeTeardownVideoSubsystemCalls == 1;

    InstallFakeDirectDraw2(directDraw);
    g_zVideo_pDirectDraw2 = reinterpret_cast<IDirectDraw2 *>(&directDraw);
    g_zVideo_NumAcceptedDirectDrawDevices = 0;
    g_zVideo_DirectDrawEnumOrdinal = 11;
    gFakeCreateDirectDraw2ForSelectedDeviceCalls = 0;
    gFakeEnumerateDirect3DDevicesForRecordCalls = 0;
    gFakeTeardownVideoSubsystemCalls = 0;
    gFakeDirectDraw2GetCapsResult = DDERR_GENERIC;
    gFakeDirectDraw2GetAvailableVidMemResult = DD_OK;
    gFakeEnumerateDirect3DDevicesForRecordResult = 1;

    const BOOL capsFailureResult =
        zVideo_dd::EnumDirectDrawDeviceCallback(
            &guid,
            const_cast<LPSTR>("caps-fail-description"),
            const_cast<LPSTR>("caps-fail-driver"),
            0
        );
    const bool capsFailureOk =
        capsFailureResult == FALSE &&
        g_zVideo_DirectDrawEnumOrdinal == 12 &&
        g_zVideo_NumAcceptedDirectDrawDevices == 0 &&
        gFakeCreateDirectDraw2ForSelectedDeviceCalls == 1 &&
        gFakeDirectDraw2GetCapsCalls == 1 &&
        gFakeDirectDraw2GetAvailableVidMemCalls == 0 &&
        gFakeEnumerateDirect3DDevicesForRecordCalls == 0 &&
        gFakeTeardownVideoSubsystemCalls == 0;

    RestoreFunctionPatch(teardownPatch);
    RestoreFunctionPatch(enumPatch);
    RestoreFunctionPatch(createPatch);
    g_zVideo_NumAcceptedDirectDrawDevices = savedAcceptedCount;
    g_zVideo_DirectDrawEnumOrdinal = savedOrdinal;
    g_zVideo_pDirectDraw2 = savedDirectDraw;
    g_zVideo_pSelectedHwApiDeviceRecord = savedSelected;
    g_zVideo_DDrawCapsHal = savedHalCaps;
    g_zVideo_DDrawCapsHel = savedHelCaps;
    g_zVideo_HwApiDeviceTable[0] = savedEntry0;
    g_zVideo_HwApiDeviceTable[1] = savedEntry1;
    return capacityOk && acceptedOk && rejectedOk && capsFailureOk ? 0 : 4;
}

extern "C" int zvideo_dd_run_device_enumeration_smoke(void) {
    const int result = zVideo_dd::RunDirectDrawDeviceEnumeration();
    return result == 0 || result == 1 ? 0 : 1;
}

extern "C" int zvideo_dd_startup_enumerate_default_select_smoke(void) {
    zVidHwApiDeviceRecordPartial *const savedSelectedHw =
        g_zVideo_pSelectedHwApiDeviceRecord;
    zVidD3DDriverRecordPartial *const savedSelectedD3D =
        g_zVideo_pSelectedD3DDeviceInfo;

    g_zVideo_pSelectedHwApiDeviceRecord = 0;
    g_zVideo_pSelectedD3DDeviceInfo =
        reinterpret_cast<zVidD3DDriverRecordPartial *>(0x1);

    zVideo_dd::StartupEnumerateAndDefaultSelect();
    const bool result =
        g_zVideo_pSelectedHwApiDeviceRecord == &g_zVideo_HwApiDeviceTable[0] &&
        g_zVideo_pSelectedD3DDeviceInfo == 0;

    g_zVideo_pSelectedHwApiDeviceRecord = savedSelectedHw;
    g_zVideo_pSelectedD3DDeviceInfo = savedSelectedD3D;
    return result ? 0 : 1;
}
