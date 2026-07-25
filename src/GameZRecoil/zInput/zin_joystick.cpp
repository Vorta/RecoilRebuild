#include "zinput.h"

#include "GameZRecoil/zError/zerr.h"

#include <stdio.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-directinputerrorfmt
 * @recoil-artifact defines .data recoil:data:0x4e0cc4: DirectInput error report format.
 * Purpose: Format DirectInput failure labels for the legacy error reporter.
 */
char g_zInput_DirectInputErrorFmt[0x18] = "DirectInput Error [%s]\n";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-alreadyinitialized
 * @recoil-artifact defines .data recoil:data:0x4e0cdc: DIERR_ALREADYINITIALIZED name.
 * Purpose: Provide the DIERR_ALREADYINITIALIZED label selected by the error reporter.
 */
char g_zInput_DiErrorName_AlreadyInitialized[0x19] =
    "DIERR_ALREADYINITIALIZED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-betadirectinputversion
 * @recoil-artifact defines .data recoil:data:0x4e0cf8: DIERR_BETADIRECTINPUTVERSION name.
 * Purpose: Provide the DIERR_BETADIRECTINPUTVERSION label selected by the error reporter.
 */
char g_zInput_DiErrorName_BetaDirectInputVersion[0x1d] =
    "DIERR_BETADIRECTINPUTVERSION";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-olddirectinputversion
 * @recoil-artifact defines .data recoil:data:0x4e0d18: DIERR_OLDDIRECTINPUTVERSION name.
 * Purpose: Provide the DIERR_OLDDIRECTINPUTVERSION label selected by the error reporter.
 */
char g_zInput_DiErrorName_OldDirectInputVersion[0x1c] =
    "DIERR_OLDDIRECTINPUTVERSION";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-acquired
 * @recoil-artifact defines .data recoil:data:0x4e0d34: DIERR_ACQUIRED name.
 * Purpose: Provide the DIERR_ACQUIRED label selected by the error reporter.
 */
char g_zInput_DiErrorName_Acquired[0x0f] = "DIERR_ACQUIRED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-baddriverversion
 * @recoil-artifact defines .data recoil:data:0x4e0d44: DIERR_BADDRIVERVER name.
 * Purpose: Provide the DIERR_BADDRIVERVER label selected by the error reporter.
 */
char g_zInput_DiErrorName_BadDriverVersion[0x13] = "DIERR_BADDRIVERVER";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-invalidparam
 * @recoil-artifact defines .data recoil:data:0x4e0d58: DIERR_INVALIDPARAM name.
 * Purpose: Provide the DIERR_INVALIDPARAM label selected by the error reporter.
 */
char g_zInput_DiErrorName_InvalidParam[0x13] = "DIERR_INVALIDPARAM";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-inputlost
 * @recoil-artifact defines .data recoil:data:0x4e0d6c: DIERR_INPUTLOST name.
 * Purpose: Provide the DIERR_INPUTLOST label selected by the error reporter.
 */
char g_zInput_DiErrorName_InputLost[0x10] = "DIERR_INPUTLOST";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-notinitialized
 * @recoil-artifact defines .data recoil:data:0x4e0d7c: DIERR_NOTINITIALIZED name.
 * Purpose: Provide the DIERR_NOTINITIALIZED label selected by the error reporter.
 */
char g_zInput_DiErrorName_NotInitialized[0x15] =
    "DIERR_NOTINITIALIZED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-outofmemory
 * @recoil-artifact defines .data recoil:data:0x4e0d94: DIERR_OUTOFMEMORY name.
 * Purpose: Provide the DIERR_OUTOFMEMORY label selected by the error reporter.
 */
char g_zInput_DiErrorName_OutOfMemory[0x12] = "DIERR_OUTOFMEMORY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-notacquired
 * @recoil-artifact defines .data recoil:data:0x4e0da8: DIERR_NOTACQUIRED name.
 * Purpose: Provide the DIERR_NOTACQUIRED label selected by the error reporter.
 */
char g_zInput_DiErrorName_NotAcquired[0x12] = "DIERR_NOTACQUIRED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-readonly
 * @recoil-artifact defines .data recoil:data:0x4e0dbc: DIERR_READONLY name.
 * Purpose: Provide the DIERR_READONLY label selected by the error reporter.
 */
char g_zInput_DiErrorName_ReadOnly[0x0f] = "DIERR_READONLY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-objectnotfound
 * @recoil-artifact defines .data recoil:data:0x4e0dcc: DIERR_OBJECTNOTFOUND name.
 * Purpose: Provide the DIERR_OBJECTNOTFOUND label selected by the error reporter.
 */
char g_zInput_DiErrorName_ObjectNotFound[0x15] =
    "DIERR_OBJECTNOTFOUND";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-devicenotreg
 * @recoil-artifact defines .data recoil:data:0x4e0de4: DIERR_DEVICENOTREG name.
 * Purpose: Provide the DIERR_DEVICENOTREG label selected by the error reporter.
 */
char g_zInput_DiErrorName_DeviceNotReg[0x13] = "DIERR_DEVICENOTREG";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-noaggregation
 * @recoil-artifact defines .data recoil:data:0x4e0df8: DIERR_NOAGGREGATION name.
 * Purpose: Provide the DIERR_NOAGGREGATION label selected by the error reporter.
 */
char g_zInput_DiErrorName_NoAggregation[0x14] = "DIERR_NOAGGREGATION";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-unsupported
 * @recoil-artifact defines .data recoil:data:0x4e0e0c: DIERR_UNSUPPORTED name.
 * Purpose: Provide the DIERR_UNSUPPORTED label selected by the error reporter.
 */
char g_zInput_DiErrorName_Unsupported[0x12] = "DIERR_UNSUPPORTED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-nointerface
 * @recoil-artifact defines .data recoil:data:0x4e0e20: DIERR_NOINTERFACE name.
 * Purpose: Provide the DIERR_NOINTERFACE label selected by the error reporter.
 */
char g_zInput_DiErrorName_NoInterface[0x12] = "DIERR_NOINTERFACE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.g-zinput-dierrorname-generic
 * @recoil-artifact defines .data recoil:data:0x4e0e34: DIERR_GENERIC name.
 * Purpose: Provide the DIERR_GENERIC label selected by the error reporter.
 */
char g_zInput_DiErrorName_Generic[0x0e] = "DIERR_GENERIC";
}

namespace zInput {

const int kDiOk = 0;
const int kDiInputLost = (int)(0x8007001e);

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-initjoystickdevice
 * @recoil-artifact defines .text recoil:function:0x471e40: zInput::DI_InitJoystickDevice.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: Enumerate and configure the DirectInput joystick device, cache
 * capabilities, apply startup axis ranges, acquire the device, and mark it initialized.
 *
 * Evidence: BN enumerates attached game controllers, queries c_dfDIJoystick,
 * stores axis count and force-feedback capability bits in the accepted
 * zInput_GlobalState aggregate, chooses cooperative flags from force-feedback
 * support, applies four-axis startup ranges/deadzones, and returns 1 on success.
 */
int __fastcall DI_InitJoystickDevice(
    HWND hwnd
) {
    if (g_zInput_GlobalState == 0) {
        return 0;
    }

    g_zInput_JoystickDevice = 0;
    g_zInput_GlobalState->EnumDevices(
        4,
        DI_EnumDevicesCallback_SelectFirstJoystick,
        0,
        1
    );

    DIDevice *joystickDevice = g_zInput_JoystickDevice;
    if (joystickDevice == 0) {
        return 0;
    }

    DIDeviceCaps caps;
    caps.dwSize = 0x2c;
    joystickDevice->SetDataFormat(
        &c_dfDIJoystick
    );
    joystickDevice->GetCapabilities(
        (LPDIDEVCAPS)(&caps)
    );

    g_zInput_JoystickAxisCount = caps.dwAxes;
    g_zInput_JoystickCaps_ForceFeedback = caps.dwFlags & 0x100;
    g_zInput_JoystickCaps_FFAttack = caps.dwFlags & 0x200;
    g_zInput_JoystickCaps_FFFade = caps.dwFlags & 0x400;

    const unsigned int coopFlags = g_zInput_JoystickCaps_ForceFeedback != 0 ? 5U : 9U;
    joystickDevice->SetCooperativeLevel(
        hwnd,
        coopFlags
    );

    JoystickAxisConfig axisCfg;
    memset(
        &axisCfg,
        0,
        sizeof(axisCfg)
    );
    axisCfg.axes[0].lMin = -1000;
    axisCfg.axes[0].lMax = 1000;
    axisCfg.axes[1].lMin = -1000;
    axisCfg.axes[1].lMax = 1000;
    axisCfg.axes[2].lMin = -1000;
    axisCfg.axes[2].lMax = 1000;
    axisCfg.axes[3].lMin = -1000;
    axisCfg.axes[3].lMax = 1000;
    axisCfg.axes[0].deadzone = 1000;
    axisCfg.axes[1].deadzone = 1000;
    axisCfg.axes[2].deadzone = 0;
    axisCfg.axes[3].deadzone = 2500;
    DI_ApplyAxisConfig(&axisCfg);
    DI_AcquireJoystickDevice();
    g_zInput_JoystickInitialized = 1;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-enumdevicescallback-selectfirstjoystick
 * @recoil-artifact defines .text recoil:function:0x471f60: zInput::DI_EnumDevicesCallback_SelectFirstJoystick.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: Create the first enumerated joystick device and store the upgraded
 * IDirectInputDevice2A pointer for zInput joystick setup.
 */
int __stdcall DI_EnumDevicesCallback_SelectFirstJoystick(
    const DIDeviceInstance *instance,
    void *
) {
    DIDevice *baseDevice = 0;
    const int hr = g_zInput_GlobalState->CreateDevice(
        instance->guidInstance,
        (LPDIRECTINPUTDEVICEA *)(&baseDevice),
        0
    );
    if (hr != 0) {
        return 1;
    }

    baseDevice->QueryInterface(
        IID_IDirectInputDevice2A,
        (void **)(&g_zInput_JoystickDevice)
    );
    baseDevice->Release();
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-acquirejoystickdevice
 * @recoil-artifact defines .text recoil:function:0x471fb0: zInput::DI_AcquireJoystickDevice.
 * Purpose: Acquire the DirectInput joystick device when one is available.
 */
int DI_AcquireJoystickDevice() {
    if (g_zInput_JoystickDevice != 0) {
        const int result = g_zInput_JoystickDevice->Acquire();
        const int success = (result == 0);
        return success;
    }
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-applyaxisconfig
 * @recoil-artifact defines .text recoil:function:0x471fd0: zInput::DI_ApplyAxisConfig.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: Apply the recovered four-axis joystick range and deadzone
 * configuration to the active DirectInput joystick device.
 *
 * Evidence: BN assembly expands the same range/fallback/midpoint/deadzone
 * sequence for X/Y and conditionally for Z/Rz; keeping the sequence in this
 * function avoids a helper with no standalone retail body.
 */
int __fastcall DI_ApplyAxisConfig(
    JoystickAxisConfig *axisCfg
) {
    if (axisCfg == 0) {
        return 0;
    }

    int result = 1;
    JoystickAxisConfigEntry &axisX = axisCfg->axes[0];
    if (DI_SetAxisRange(
        DIJOFS_X,
        axisX.lMin,
        axisX.lMax
    ) < 0) {
        DI_GetAxisRange(
            DIJOFS_X,
            &axisX.lMin,
            &axisX.lMax
        );
    }
    axisX.midpoint = (float)(axisX.lMin + axisX.lMax) * 0.5f;
    axisX.normScale = 2.0f / (float)(axisX.lMax - axisX.lMin);
    result &= DI_SetAxisDeadzone(
        DIJOFS_X,
        axisX.deadzone
    ) >= 0 ? 1 : 0;

    JoystickAxisConfigEntry &axisY = axisCfg->axes[1];
    if (DI_SetAxisRange(
        DIJOFS_Y,
        axisY.lMin,
        axisY.lMax
    ) < 0) {
        DI_GetAxisRange(
            DIJOFS_Y,
            &axisY.lMin,
            &axisY.lMax
        );
    }
    axisY.midpoint = (float)(axisY.lMin + axisY.lMax) * 0.5f;
    axisY.normScale = 2.0f / (float)(axisY.lMax - axisY.lMin);
    result &= DI_SetAxisDeadzone(
        DIJOFS_Y,
        axisY.deadzone
    ) >= 0 ? 1 : 0;

    if (g_zInput_JoystickAxisCount > 2) {
        JoystickAxisConfigEntry &axisZ = axisCfg->axes[2];
        if (DI_SetAxisRange(
            DIJOFS_Z,
            axisZ.lMin,
            axisZ.lMax
        ) < 0) {
            DI_GetAxisRange(
                DIJOFS_Z,
                &axisZ.lMin,
                &axisZ.lMax
            );
        }
        axisZ.midpoint = (float)(axisZ.lMin + axisZ.lMax) * 0.5f;
        axisZ.normScale = 2.0f / (float)(axisZ.lMax - axisZ.lMin);
        result &= DI_SetAxisDeadzone(
            DIJOFS_Z,
            axisZ.deadzone
        ) >= 0 ? 1 : 0;
    }
    if (g_zInput_JoystickAxisCount > 3) {
        JoystickAxisConfigEntry &axisRz = axisCfg->axes[3];
        if (DI_SetAxisRange(
            DIJOFS_RZ,
            axisRz.lMin,
            axisRz.lMax
        ) < 0) {
            DI_GetAxisRange(
                DIJOFS_RZ,
                &axisRz.lMin,
                &axisRz.lMax
            );
        }
        axisRz.midpoint = (float)(axisRz.lMin + axisRz.lMax) * 0.5f;
        axisRz.normScale = 2.0f / (float)(axisRz.lMax - axisRz.lMin);
        result &= DI_SetAxisDeadzone(
            DIJOFS_RZ,
            axisRz.deadzone
        ) >= 0 ? 1 : 0;
    }

    g_zInput_JoystickAxisConfig = *axisCfg;
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-setaxisdeadzone
 * @recoil-artifact defines .text recoil:function:0x4721a0: zInput::DI_SetAxisDeadzone.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: Apply one DirectInput axis deadzone property by object offset.
 */
int __fastcall DI_SetAxisDeadzone(
    int axisOffset,
    int deadzone
) {
    struct DipropDwordLocal {
        unsigned int dwSize;
        unsigned int dwHeaderSize;
        unsigned int dwObj;
        unsigned int dwHow;
        unsigned int dwData;
    } prop;

    prop.dwSize = sizeof(prop);
    prop.dwHeaderSize = 0x10;
    prop.dwObj = (unsigned int)(axisOffset);
    prop.dwHow = 1;
    prop.dwData = (unsigned int)(deadzone);
    return g_zInput_JoystickDevice->SetProperty(
        DIPROP_DEADZONE,
        (LPCDIPROPHEADER)(&prop)
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-setaxisrange
 * @recoil-artifact defines .text recoil:function:0x4721e0: zInput::DI_SetAxisRange.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: Apply one DirectInput axis range property by object offset.
 */
int __fastcall DI_SetAxisRange(
    int axisOffset,
    int rangeMin,
    int rangeMax
) {
    struct DipropRangeLocal {
        unsigned int dwSize;
        unsigned int dwHeaderSize;
        unsigned int dwObj;
        unsigned int dwHow;
        int lMin;
        int lMax;
    } prop;

    prop.dwSize = sizeof(prop);
    prop.dwHeaderSize = 0x10;
    prop.dwObj = (unsigned int)(axisOffset);
    prop.dwHow = 1;
    prop.lMin = rangeMin;
    prop.lMax = rangeMax;
    return g_zInput_JoystickDevice->SetProperty(
        DIPROP_RANGE,
        (LPCDIPROPHEADER)(&prop)
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-getaxisrange
 * @recoil-artifact defines .text recoil:function:0x472230: zInput::DI_GetAxisRange.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: Read one DirectInput axis range property by object offset.
 */
int __fastcall DI_GetAxisRange(
    int axisOffset,
    int *pOutMin,
    int *pOutMax
) {
    struct DipropRangeLocal {
        unsigned int dwSize;
        unsigned int dwHeaderSize;
        unsigned int dwObj;
        unsigned int dwHow;
        int lMin;
        int lMax;
    } prop;

    prop.dwSize = sizeof(prop);
    prop.dwHeaderSize = 0x10;
    prop.dwObj = (unsigned int)(axisOffset);
    prop.dwHow = 1;
    const int result =
        g_zInput_JoystickDevice->GetProperty(
            DIPROP_RANGE,
            (LPDIPROPHEADER)(&prop)
        );
    *pOutMin = prop.lMin;
    *pOutMax = prop.lMax;
    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.joystick-shutdowndevice
 * @recoil-artifact defines .text recoil:function:0x472280: zInput::Joystick_ShutdownDevice.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: unacquire and release the joystick DirectInput device during zInput
 * shutdown.
 *
 * Evidence: BN HLIL guards g_zInput_JoystickDevice, calls Unacquire and
 * Release through the DirectInput device vtable, clears the device pointer,
 * and returns 1.
 */
int Joystick_ShutdownDevice() {
    DIDevice *const joystick = g_zInput_JoystickDevice;
    if (joystick != 0) {
        joystick->Unacquire();
        g_zInput_JoystickDevice->Release();
        g_zInput_JoystickDevice = 0;
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-isjoystickdeviceready
 * @recoil-artifact defines .text recoil:function:0x4722b0: zInput::DI_IsJoystickDeviceReady.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: Report whether joystick input is initialized and has an active
 * DirectInput device pointer.
 */
int DI_IsJoystickDeviceReady() {
    return g_zInput_JoystickInitialized == 1 ? 1 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-polljoystickstate
 * @recoil-artifact defines .text recoil:function:0x4722c0: zInput::DI_PollJoystickState.
 *
 * Purpose: poll the DirectInput joystick, normalize absent axes, and update
 * the current/previous joystick state snapshots.
 */
DIJOYSTATE2 *__fastcall DI_PollJoystickState(
    unsigned char dispatchCallbacks
) {
    if (g_zInput_JoystickInitialized == 0) {
        return 0;
    }

    DIDevice *device = g_zInput_JoystickDevice;
    device->Poll();
    const int result = device->GetDeviceState(
        sizeof(DIJOYSTATE2),
        &g_zInput_JoystickRawDIState
    );

    if (g_zInput_JoystickAxisCount < 3) {
        g_zInput_JoystickRawDIState.lZ = 0;
        g_zInput_JoystickRawDIState.lVZ = 0;
        g_zInput_JoystickRawDIState.lAZ = 0;
        g_zInput_JoystickRawDIState.lFZ = 0;
    }
    if (g_zInput_JoystickAxisCount < 4) {
        g_zInput_JoystickRawDIState.lRz = 0;
        g_zInput_JoystickRawDIState.lVRz = 0;
        g_zInput_JoystickRawDIState.lARz = 0;
        g_zInput_JoystickRawDIState.lFRz = 0;
    }

    if (result == kDiInputLost) {
        DI_AcquireJoystickDevice();
        return 0;
    }
    if (result != kDiOk) {
        return 0;
    }

    g_zInput_JoystickPreviousState = g_zInput_JoystickCurrentState;
    g_zInput_JoystickCurrentState = g_zInput_JoystickRawDIState;
    if (dispatchCallbacks != 0) {
        g_zInput_BindMap_Current->DispatchJoystickButtonCallbacks();
    }

    return DI_GetCurrentState();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-getcurrentstate
 * @recoil-artifact defines .text recoil:function:0x472390: zInput::DI_GetCurrentState.
 *
 * Purpose: return the current DirectInput joystick state snapshot.
 */
DIJOYSTATE2 *DI_GetCurrentState() {
    return &g_zInput_JoystickCurrentState;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-getbuttontransitionstate
 * @recoil-artifact defines .text recoil:function:0x4723a0: zInput::DI_GetButtonTransitionState.
 * Purpose: Return the pressed, held, released, or idle transition state for a
 * 1-based joystick button slot from the paired DirectInput state snapshots.
 */
int __fastcall DI_GetButtonTransitionState(
    int buttonIndex
) {
    if (g_zInput_JoystickCurrentState.rgbButtons[buttonIndex - 1] != 0) {
        return (g_zInput_JoystickPreviousState.rgbButtons[buttonIndex - 1] != 0 ? 1 : 0) + 1;
    }

    return g_zInput_JoystickPreviousState.rgbButtons[buttonIndex - 1] != 0 ? 4 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-waitforbuttonpress
 * @recoil-artifact defines .text recoil:function:0x4723d0: zInput::DI_WaitForButtonPress.
 * Purpose: Poll joystick state until a newly pressed button is found or the
 * caller requests a single scan.
 */
int __fastcall DI_WaitForButtonPress(
    int loopUntilPressed
) {
    int result = 0;
    do {
        DI_PollJoystickState(1);
        {
            for (int button = 1; button < 0x0b; ++button) {
                if (DI_GetButtonTransitionState(button) == 1) {
                    result = button;
                    break;
                }
            }
        }

        DI_ResetTransitionState();
    } while (result == 0 && loopUntilPressed != 0);

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-resettransitionstate
 * @recoil-artifact defines .text recoil:function:0x472410: zInput::DI_ResetTransitionState.
 *
 * Purpose: clear joystick button transition bytes and reset POV transition
 * state while preserving the untouched first button byte.
 *
 * Evidence: BN assembly at 0x472410 guards on g_zInput_JoystickInitialized,
 * zeroes current/previous rgbButtons[1..10], and writes 0xffff to
 * current/previous rgdwPOV[0..3] across the two DIJOYSTATE2 BSS globals.
 */
void DI_ResetTransitionState() {
    if (g_zInput_JoystickInitialized == 0) {
        return;
    }

    for (int i = 1; i < 11; ++i) {
        g_zInput_JoystickPreviousState.rgbButtons[i] = 0;
        g_zInput_JoystickCurrentState.rgbButtons[i] = 0;
    }

    for (int i_2916 = 0; i_2916 < 4; ++i_2916) {
        g_zInput_JoystickCurrentState.rgdwPOV[i_2916] = 0xffff;
        g_zInput_JoystickPreviousState.rgdwPOV[i_2916] = 0xffff;
    }
}

}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.zinput-di-createforcefeedbackeffect
 * @recoil-artifact defines .text recoil:function:0x472450: zInput_DI_CreateForceFeedbackEffect.
 * Physical source contribution: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * The retail order shelf places this body between joystick transition reset
 * and the adjacent force-feedback capability query; no separate zin_ff.cpp
 * contribution is proven.
 * Purpose: create a DirectInput force-feedback effect on the active joystick.
 */
zInput_DiEffect *__fastcall zInput_DI_CreateForceFeedbackEffect(
    const GUID *rguidEffect,
    const DIEFFECT *effect
) {
    if (g_zInput_JoystickDevice == 0) {
        return 0;
    }

    zInput_DiEffect *outEffect = 0;
    const int result = g_zInput_JoystickDevice->CreateEffect(
        *rguidEffect,
        effect,
        &outEffect,
        0
    );
    return result < 0 ? 0 : outEffect;
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.zinput-di-hasforcefeedback
 * @recoil-artifact defines .text recoil:function:0x472480: zInput_DI_HasForceFeedback.
 * Physical source contribution: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * No separate zin_ff.cpp contribution is proven by the retail order shelf.
 * Purpose: return the detected DirectInput joystick force-feedback capability.
 */
int zInput_DI_HasForceFeedback() {
    return g_zInput_JoystickCaps_ForceFeedback;
}
namespace zInput {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-joystick.di-reporterror
 * @recoil-artifact defines .text recoil:function:0x472490: zInput::DI_ReportError.
 * Binary Ninja shows the original zin_joystick.cpp routine inlining the
 * ordered DirectInput HRESULT compare tree before the shared sprintf/report
 * tail; there is no standalone retail error-name helper.
 * Purpose: Report failing DirectInput HRESULTs through the legacy zError path.
 */
RECOIL_NO_GS int __fastcall DI_ReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
) {
    char errorNameBuffer[0x100];
    if (hresult <= (int)(0x80040110)) {
        if (hresult != (int)(0x80040110)) {
            if (hresult != (int)(0x80004001)) {
                if (hresult != (int)(0x80004002)) {
                    if (hresult != (int)(0x80004005)) {
                        goto unknownError;
                    }
                    goto dierrGeneric;
                }
                goto dierrNoInterface;
            }
            goto dierrUnsupported;
        }
        goto dierrNoAggregation;
    } else if (hresult <= (int)(0x80070002)) {
        if (hresult != (int)(0x80070002)) {
            if (hresult != (int)(0x80040154)) {
                goto unknownError;
            }
            goto dierrDeviceNotReg;
        }
        goto dierrObjectNotFound;
    } else if (hresult <= (int)(0x8007000c)) {
        if (hresult != (int)(0x8007000c)) {
            if (hresult != (int)(0x80070005)) {
                goto unknownError;
            }
            goto dierrReadOnly;
        }
        goto dierrNotAcquired;
    } else if (hresult <= (int)(0x80070015)) {
        if (hresult != (int)(0x80070015)) {
            if (hresult != (int)(0x8007000e)) {
                goto unknownError;
            }
            goto dierrOutOfMemory;
        }
        goto dierrNotInitialized;
    } else if (hresult <= (int)(0x80070057)) {
        if (hresult != (int)(0x80070057)) {
            if (hresult != (int)(0x8007001e)) {
                goto unknownError;
            }
            goto dierrInputLost;
        }
        goto dierrInvalidParam;
    } else if (hresult <= (int)(0x800700aa)) {
        if (hresult != (int)(0x800700aa)) {
            if (hresult != (int)(0x80070077)) {
                goto unknownError;
            }
            goto dierrBadDriverVer;
        }
        goto dierrAcquired;
    } else if (hresult <= (int)(0x80070481)) {
        if (hresult != (int)(0x80070481)) {
            if (hresult != (int)(0x8007047e)) {
                goto unknownError;
            }
            goto dierrOldDirectInputVersion;
        }
        goto dierrBetaDirectInputVersion;
    } else if (hresult != (int)(0x800704df)) {
        if (hresult != kDiOk) {
            goto unknownError;
        }
        goto diOk;
    }
    goto dierrAlreadyInitialized;

dierrGeneric:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_Generic
    );
    goto reportError;
dierrNoInterface:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_NoInterface
    );
    goto reportError;
dierrUnsupported:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_Unsupported
    );
    goto reportError;
dierrNoAggregation:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_NoAggregation
    );
    goto reportError;
dierrDeviceNotReg:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_DeviceNotReg
    );
    goto reportError;
dierrObjectNotFound:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_ObjectNotFound
    );
    goto reportError;
dierrReadOnly:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_ReadOnly
    );
    goto reportError;
dierrNotAcquired:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_NotAcquired
    );
    goto reportError;
dierrOutOfMemory:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_OutOfMemory
    );
    goto reportError;
dierrNotInitialized:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_NotInitialized
    );
    goto reportError;
dierrInputLost:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_InputLost
    );
    goto reportError;
dierrInvalidParam:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_InvalidParam
    );
    goto reportError;
dierrBadDriverVer:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_BadDriverVersion
    );
    goto reportError;
dierrAcquired:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_Acquired
    );
    goto reportError;
dierrOldDirectInputVersion:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_OldDirectInputVersion
    );
    goto reportError;
dierrBetaDirectInputVersion:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_BetaDirectInputVersion
    );
    goto reportError;
unknownError:
    sprintf(
        errorNameBuffer,
        "Unknown Error"
    );
    goto reportError;
diOk:
    return 1;
dierrAlreadyInitialized:
    sprintf(
        errorNameBuffer,
        g_zInput_DiErrorName_AlreadyInitialized
    );

reportError:
    zError::ReportOld(
        0x800,
        sourceFile,
        sourceLine,
        g_zInput_DirectInputErrorFmt,
        errorNameBuffer
    );
    return 0;
}

} // namespace zInput
