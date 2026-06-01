#pragma once

#include "recoil/recoil_types.h"

#include <mmsystem.h>
#include <stddef.h>
#include <windows.h>

struct zSndBuffer;

// Imported Aureal A3D COM ABI shim. Recoil consumed these provider interfaces
// through vtable slots; this header documents that boundary and does not
// reimplement Aureal behavior.
typedef int(__stdcall *zA3dSimpleFn)(void *self);
typedef int(__stdcall *zA3dSetIntFn)(
    void *self,
    int value
);
typedef int(__stdcall *zA3dSetFloatFn)(
    void *self,
    float value
);
typedef int(__stdcall *zA3dSetVecFn)(
    void *self,
    float x,
    float y,
    float z
);
typedef int(__stdcall *zA3dGetStatusFn)(
    void *self,
    int *outStatus
);
typedef int(__stdcall *zA3dGetUint32Fn)(
    void *self,
    unsigned int *outValue
);
typedef int(__stdcall *zA3dPlayFn)(
    void *self,
    unsigned int flags
);
typedef int(__stdcall *zA3dCreateBufferFn)(
    void *self,
    int bufferKind,
    zSndBuffer **outBuffer
);
typedef int(__stdcall *zA3dDuplicateBufferFn)(
    void *self,
    zSndBuffer *source,
    zSndBuffer **outDuplicate
);
typedef int(__stdcall *zA3dSetWaveFormatFn)(
    void *self,
    WAVEFORMATEX *format
);
typedef int(__stdcall *zA3dSetRangeFn)(
    void *self,
    float rangeMin,
    float rangeMax,
    int enabled
);
typedef int(__stdcall *zA3dLockFn)(
    void *self,
    unsigned int offset,
    unsigned int bytes,
    void **outPtr1,
    int *outBytes1,
    void **outPtr2,
    int *outBytes2,
    unsigned int flags
);
typedef int(__stdcall *zA3dUnlockFn)(
    void *self,
    void *ptr1,
    int bytes1,
    void *ptr2,
    int bytes2
);
typedef int(__stdcall *zA3dSetOrientationFn)(
    void *self,
    float forwardX,
    float forwardY,
    float forwardZ,
    float upX,
    float upY,
    float upZ
);

struct zA3dProviderDeviceVTable {
    void *slots00_2c[12];
    zA3dSimpleFn Tick;
    zA3dSimpleFn CommitDeferredSettings;
    void *slots38_40[3];
    zA3dCreateBufferFn CreateBufferByKind;
    zA3dDuplicateBufferFn DuplicateBufferA3D;
};

struct zA3dProviderDevice {
    zA3dProviderDeviceVTable *vtable;
};

struct zA3dProviderSourceVTable {
    void *QueryInterface;
    void *AddRef;
    zA3dSimpleFn Release;
    void *reserved00c;
    void *reserved010;
    zA3dSetIntFn SetSampleDataSize;
    zA3dSimpleFn FreeWaveData;
    zA3dSetWaveFormatFn SetWaveFormat;
    void *slots20_28[3];
    zA3dLockFn Lock;
    zA3dUnlockFn CommitWrite;
    zA3dPlayFn Play;
    zA3dSimpleFn Stop;
    zA3dSimpleFn Rewind;
    void *slots40_44[2];
    zA3dSetIntFn SetMode;
    zA3dGetUint32Fn GetCurrentPosition;
    zA3dSetVecFn SetPosition;
    void *slots54_7c[11];
    zA3dSetVecFn SetVelocity;
    void *slots84_94[5];
    zA3dSetRangeFn SetRange;
    void *slot09c;
    zA3dSetFloatFn SetGain;
    void *slot0a4;
    zA3dSetFloatFn SetPitchScaled;
    void *slot0ac;
    zA3dSetFloatFn SetDopplerScale;
    void *slot0b4;
    zA3dSetFloatFn SetA3DDistanceScale;
    void *slotsbc_cc[5];
    zA3dSetIntFn SetSpatializationEnabled;
    void *slotsd4_dc[3];
    zA3dGetStatusFn GetStatus;
};

struct zA3dProviderSource {
    zA3dProviderSourceVTable *vtable;
};

struct zA3dProviderListenerVTable {
    void *slots00_08[3];
    zA3dSetVecFn SetPosition;
    void *slots10_28[7];
    zA3dSetOrientationFn SetOrientation;
    void *slots30_38[3];
    zA3dSetVecFn SetVelocity;
};

struct zA3dProviderListener {
    zA3dProviderListenerVTable *vtable;
};

RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderDeviceVTable,
        Tick
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderDeviceVTable,
        CommitDeferredSettings
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderDeviceVTable,
        CreateBufferByKind
    ) == 0x44
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderDeviceVTable,
        DuplicateBufferA3D
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetSampleDataSize
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        FreeWaveData
    ) == 0x18
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetWaveFormat
    ) == 0x1c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        Lock
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        CommitWrite
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        Play
    ) == 0x34
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetMode
    ) == 0x48
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        GetCurrentPosition
    ) == 0x4c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetPosition
    ) == 0x50
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetVelocity
    ) == 0x80
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetRange
    ) == 0x98
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetGain
    ) == 0xa0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetPitchScaled
    ) == 0xa8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetDopplerScale
    ) == 0xb0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetA3DDistanceScale
    ) == 0xb8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        SetSpatializationEnabled
    ) == 0xd0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderSourceVTable,
        GetStatus
    ) == 0xe0
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderListenerVTable,
        SetPosition
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderListenerVTable,
        SetOrientation
    ) == 0x2c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zA3dProviderListenerVTable,
        SetVelocity
    ) == 0x3c
);
