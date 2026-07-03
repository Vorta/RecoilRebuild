/* This source-layout fragment is included by the current compatibility container.
 * Parent build/manifests must compile this path directly after retiring the container include.
 */

/**
 * Reimplements 0x471e40: zInput::DI_InitJoystickDevice.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
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
 * Reimplements 0x471f60: zInput::DI_EnumDevicesCallback_SelectFirstJoystick.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
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
 * Reimplements 0x471fb0: zInput::DI_AcquireJoystickDevice.
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
 * Reimplements 0x471fd0: zInput::DI_ApplyAxisConfig.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
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
 * Reimplements 0x4721a0: zInput::DI_SetAxisDeadzone.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
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
 * Reimplements 0x4721e0: zInput::DI_SetAxisRange.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
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
 * Reimplements 0x472230: zInput::DI_GetAxisRange.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
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
 * Reimplements 0x472280: zInput::Joystick_ShutdownDevice.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
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
 * Reimplements 0x4722b0: zInput::DI_IsJoystickDeviceReady.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_joystick.cpp.
 * Purpose: Report whether joystick input is initialized and has an active
 * DirectInput device pointer.
 */
int DI_IsJoystickDeviceReady() {
    return g_zInput_JoystickInitialized == 1 ? 1 : 0;
}

/**
 * Reimplements 0x4722c0: zInput::DI_PollJoystickState.
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
 * Reimplements 0x472390: zInput::DI_GetCurrentState.
 *
 * Purpose: return the current DirectInput joystick state snapshot.
 */
DIJOYSTATE2 *DI_GetCurrentState() {
    return &g_zInput_JoystickCurrentState;
}

/**
 * Reimplements 0x4723a0: zInput::DI_GetButtonTransitionState.
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
 * Reimplements 0x4723d0: zInput::DI_WaitForButtonPress.
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
 * Reimplements 0x472410: zInput::DI_ResetTransitionState.
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

/**
 * Reimplements 0x472450: zInput_DI_CreateForceFeedbackEffect.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_ff.cpp.
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
 * Original-source helper evidence: ClampForceFeedbackGain.
 * No standalone retail function address is assigned; address-backed force-feedback
 * callers in this source file share this clamp shape.
 * Purpose: clamp a force-feedback gain to the normalized DirectInput range.
 */
static float ClampForceFeedbackGain(
    float gain
) {
    if (gain > 1.0f) {
        return 1.0f;
    }
    if (gain < 0.0f) {
        return 0.0f;
    }

    return gain;
}

/**
 * Original-source helper evidence: ClampForceFeedbackGainRange.
 * No standalone retail function address is assigned; address-backed force-feedback
 * callers in this source file share this bounded clamp shape.
 * Purpose: clamp a force-feedback gain to a caller-provided normalized range.
 */
static float ClampForceFeedbackGainRange(
    float gain,
    float minGain,
    float maxGain
) {
    if (gain > maxGain) {
        return maxGain;
    }
    if (gain < minGain) {
        return minGain;
    }

    return gain;
}

/**
 * Original-source helper evidence: WrapForceFeedbackPolarRadians.
 * No standalone retail function address is assigned; address-backed impact
 * direction code uses this single-turn polar angle wrap.
 * Purpose: wrap a polar angle by one full revolution when it leaves the range.
 */
static float WrapForceFeedbackPolarRadians(
    float angle
) {
    const float kTwoPi = 6.28318548f;
    if (angle < -kTwoPi) {
        angle += kTwoPi;
    } else if (angle > kTwoPi) {
        angle -= kTwoPi;
    }

    return angle;
}

/**
 * Original-source helper evidence: ForceFeedbackDirectionFromRadians.
 * No standalone retail function address is assigned; address-backed
 * force-feedback direction code converts radians to DirectInput hundredths of
 * degrees.
 * Purpose: convert a polar angle in radians to a DirectInput direction value.
 */
static int ForceFeedbackDirectionFromRadians(
    float angle
) {
    const double kRadToDeg = 57.295779513079999;
    return (int)(angle * kRadToDeg) * 100;
}

/**
 * Original-source helper evidence: ForceFeedbackDirectionFromImpact.
 * No standalone retail function address is assigned; address-backed collision
 * and damage force-feedback callers share this camera-relative bearing math.
 * Purpose: compute a camera-relative DirectInput force direction for an impact.
 */
static int ForceFeedbackDirectionFromImpact(
    const zVec3 *worldPosXZ,
    bool sourceToPlayer
) {
    const float kPi = 3.14159274f;
    const zInput_PlayerStatePartial *const playerState = g_GameStateOrMapTable->playerState;
    const float sourceBearing = sourceToPlayer ? (float)(atan2(
        worldPosXZ->z,
        worldPosXZ->x
    ))
                                               : (float)(atan2(
                                                   -worldPosXZ->z,
                                                   -worldPosXZ->x
                                               ));
    const float playerBearing =
        (float)(atan2(
            -playerState->cameraDirNextZ,
            -playerState->cameraDirNextX
        ));
    const float relativeBearing =
        WrapForceFeedbackPolarRadians(kPi - (sourceBearing - playerBearing));
    return ForceFeedbackDirectionFromRadians(relativeBearing);
}

/**
 * Original-source helper evidence: SetAndStartDirectionalForceFeedbackEffect.
 * No standalone retail function address is assigned; address-backed steer, pitch,
 * collision, and damage callers share this SetParameters/Start sequence.
 * Purpose: update a two-axis polar force-feedback effect and start it.
 */
static void SetAndStartDirectionalForceFeedbackEffect(
    zInput_DiEffect *effect,
    int direction,
    float gain
) {
    LONG polarDirection[2] = {direction, 0};
    DIEFFECT desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x20;
    desc.dwGain = (DWORD)(gain * 10000.0f);
    desc.cAxes = 2;
    desc.rglDirection = polarDirection;
    effect->SetParameters(
        &desc,
        0x44
    );
    effect->Start(
        1,
        0
    );
}

/**
 * Original-source helper evidence: FastPitchLowpassFactor.
 * No standalone retail function address is assigned; the address-backed pitch
 * force updater uses this VC-era bit construction before smoothing pitch force.
 * Purpose: compute the pitch-force lowpass factor from frame delta time.
 */
static float FastPitchLowpassFactor(
    float deltaTime
) {
    int bits = (int)(deltaTime * -3.0f * 12102200.0f);
    bits += 0x3f800000;

    float factor = 0.0f;
    memcpy(
        &factor,
        &bits,
        sizeof(factor)
    );
    return factor;
}

/**
 * Reimplements 0x472480: zInput_DI_HasForceFeedback.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_ff.cpp.
 * Purpose: return the detected DirectInput joystick force-feedback capability.
 */
int zInput_DI_HasForceFeedback() {
    return g_zInput_JoystickCaps_ForceFeedback;
}

/**
 * Reimplements 0x472490: zInput::DI_ReportError.
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

