#include "zinput.h"

#include <string.h>

namespace zInput {

const int kDiOk = 0;
const int kDiFalse = 1;
const int kDiInputLost = (int)(0x8007001e);

struct DipropDwordInit {
    unsigned int dwSize;
    unsigned int dwHeaderSize;
    unsigned int dwObj;
    unsigned int dwHow;
    unsigned int dwData;
};

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-applyclientcursorpostoos
 * @recoil-artifact defines .text recoil:function:0x470020: zInput::Mouse_ApplyClientCursorPosToOS.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Convert the cached client mouse point to screen coordinates and
 * apply it through the Win32 cursor provider.
 */
void Mouse_ApplyClientCursorPosToOS() {
    POINT point;
    point.x = g_zInput_MouseStateSnapshot.cursorClientX;
    point.y = g_zInput_MouseStateSnapshot.cursorClientY;
    ClientToScreen(
        g_zInput_hWnd,
        &point
    );
    SetCursorPos(
        point.x,
        point.y
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-updateclientrectandcenter
 * @recoil-artifact defines .text recoil:function:0x470060: zInput::Mouse_UpdateClientRectAndCenter.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Refresh mouse client dimensions, center coordinates, and inverse
 * scaling factors from the current input window client rectangle.
 */
void Mouse_UpdateClientRectAndCenter() {
    RECT rect;
    GetClientRect(
        g_zInput_hWnd,
        &rect
    );
    g_zInput_MouseClientWidth = rect.right;
    g_zInput_MouseClientHeight = rect.bottom;
    Mouse_SetClientSizeAndCenter(
        rect.right,
        rect.bottom
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-setnormalizedcursorpos
 * @recoil-artifact defines .text recoil:function:0x4700a0: zInput::Mouse_SetNormalizedCursorPos.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Clamp normalized cursor coordinates, convert them to cached client
 * coordinates, and apply the cursor position to the OS.
 */
void __stdcall Mouse_SetNormalizedCursorPos(
    float normX,
    float normY
) {
    if (normX > 1.0f) {
        normX = 1.0f;
    }
    if (normX < -1.0f) {
        normX = -1.0f;
    }
    if (normY > 1.0f) {
        normY = 1.0f;
    }
    if (normY < -1.0f) {
        normY = -1.0f;
    }

    g_zInput_MouseStateSnapshot.cursorNormX = normX;
    g_zInput_MouseStateSnapshot.cursorNormY = normY;
    g_zInput_MouseStateSnapshot.cursorClientX =
        g_zInput_MouseClientCenterX + (int)(g_zInput_MouseClientCenterX * normX);
    g_zInput_MouseStateSnapshot.cursorClientY =
        g_zInput_MouseClientCenterY + (int)(g_zInput_MouseClientCenterY * normY);
    Mouse_ApplyClientCursorPosToOS();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-recentercursor
 * @recoil-artifact defines .text recoil:function:0x470150: zInput::Mouse_RecenterCursor.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Move the cached mouse cursor position to the client center and
 * apply the position to the OS cursor.
 */
void Mouse_RecenterCursor() {
    g_zInput_MouseStateSnapshot.cursorClientX = g_zInput_MouseClientCenterX;
    g_zInput_MouseStateSnapshot.cursorClientY = g_zInput_MouseClientCenterY;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.0f;
    g_zInput_MouseStateSnapshot.cursorNormY = 0.0f;
    Mouse_ApplyClientCursorPosToOS();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-recentercursorx
 * @recoil-artifact defines .text recoil:function:0x470180: zInput::Mouse_RecenterCursorX.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Recenter only the cached mouse client X coordinate before applying
 * the position to the OS cursor.
 */
void Mouse_RecenterCursorX() {
    g_zInput_MouseStateSnapshot.cursorClientX = g_zInput_MouseClientCenterX;
    Mouse_ApplyClientCursorPosToOS();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-isinitialized
 * @recoil-artifact defines .text recoil:function:0x470190: zInput::Mouse_IsInitialized.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Return whether the DirectInput mouse device has been initialized.
 */
int Mouse_IsInitialized() {
    return g_zInput_MouseInitialized;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-setclientsizeandcenter
 * @recoil-artifact defines .text recoil:function:0x4701a0: zInput::Mouse_SetClientSizeAndCenter.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Store explicit mouse client dimensions, signed center coordinates,
 * and inverse center scale factors.
 */
void __fastcall Mouse_SetClientSizeAndCenter(
    int width,
    int height
) {
    g_zInput_MouseClientWidth = width;
    g_zInput_MouseClientHeight = height;
    g_zInput_MouseClientCenterX = (width - (width >> 31)) >> 1;
    g_zInput_MouseClientCenterY = (height - (height >> 31)) >> 1;
    g_zInput_MouseInvClientCenterX = 1.0f / (float)(g_zInput_MouseClientCenterX);
    g_zInput_MouseInvClientCenterY = 1.0f / (float)(g_zInput_MouseClientCenterY);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-initdevice
 * @recoil-artifact defines .text recoil:function:0x4701f0: zInput::Mouse_InitDevice.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Create and configure the DirectInput mouse device, initialize the
 * shared mouse snapshot, acquire the device, and center the cursor state.
 *
 * Evidence: BN creates GUID_SysMouse through the accepted DirectInput root,
 * queries IDirectInputDevice2A, configures c_dfDIMouse, g_zInput_hWnd and
 * g_zInput_MouseCoopLevelFlags, sets a 16-event buffer, marks mouse active and
 * initialized, and returns 1.
 */
int Mouse_InitDevice() {
    DIDevice *baseDevice = 0;
    g_zInput_GlobalState->CreateDevice(
        GUID_SysMouse,
        (LPDIRECTINPUTDEVICEA *)(&baseDevice),
        0
    );
    baseDevice->QueryInterface(
        IID_IDirectInputDevice2A,
        (void **)(&g_zInput_MouseDevice)
    );
    baseDevice->Release();

    g_zInput_MouseDevice->SetDataFormat(
        &c_dfDIMouse
    );
    g_zInput_MouseDevice->SetCooperativeLevel(
        g_zInput_hWnd,
        (unsigned int)(g_zInput_MouseCoopLevelFlags)
    );

    DipropDwordInit bufferSizeProp = {0x14, 0x10, 0, 0, 0x10};
    g_zInput_MouseDevice->SetProperty(
        DIPROP_BUFFERSIZE,
        (LPCDIPROPHEADER)(&bufferSizeProp)
    );

    g_zInput_MouseStateSnapshot.button1Transition = 0;
    g_zInput_MouseStateSnapshot.button2Transition = 0;
    g_zInput_MouseStateSnapshot.button3Transition = 0;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseActive = 1;
    Mouse_UpdateAcquireState();
    if (g_zInput_MouseClientWidth <= 0) {
        Mouse_UpdateClientRectAndCenter();
    }
    Mouse_RecenterCursor();
    g_zInput_MouseInitialized = 1;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-getbuttontransitionstate
 * @recoil-artifact defines .text recoil:function:0x4702e0: zInput::Mouse_GetButtonTransitionState.
 *
 * Purpose: compare the current and previous mouse button byte and return the
 * transition mask for a 1-based mouse button number.
 *
 * Evidence: BN assembly at 0x4702e0 reads from the rgbButtons byte in the
 * typed current and previous mouse device states at 0x565e80/0x565e90,
 * treats button numbers as 1-based, returns 1 or 2 for down transitions and
 * held buttons, and uses the release-path neg/sbb idiom for result 4.
 */
int __fastcall Mouse_GetButtonTransitionState(
    int buttonNumber
) {
    const unsigned char *currentButtons =
        (const unsigned char *)(&g_zInput_MouseCurrentState.rgbButtons);

    const unsigned char current = currentButtons[buttonNumber - 1];
    if (current != 0) {
        const unsigned char *previousButtons =
            (const unsigned char *)(&g_zInput_MousePreviousState.rgbButtons);
        const unsigned char previous = previousButtons[buttonNumber - 1];
        return (previous != 0 ? 1 : 0) + 1;
    }

    const unsigned char *previousButtons =
        (const unsigned char *)(&g_zInput_MousePreviousState.rgbButtons);
    const unsigned char previous = previousButtons[buttonNumber - 1];
    return previous != 0 ? 4 : 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-updateacquirestate
 * @recoil-artifact defines .text recoil:function:0x470310: zInput::Mouse_UpdateAcquireState.
 * Purpose: Applies the current mouse-active flag to the DirectInput device
 * acquisition state and flips the flag only on real provider failures.
 *
 * Evidence: BN assembly reads g_zInput_MouseActive and g_zInput_MouseDevice,
 * calls DirectInput device vtable slot 0x1c for Acquire or slot 0x20 for
 * Unacquire, and treats DI_OK and DI_FALSE as non-failures.
 */
void Mouse_UpdateAcquireState() {
    if (g_zInput_MouseActive != 0) {
        DIDevice *device = g_zInput_MouseDevice;
        if (device != 0) {
            const int result = device->Acquire();
            if (result != kDiOk && result != kDiFalse) {
                g_zInput_MouseActive = 0;
            }
        }
    } else {
        DIDevice *device = g_zInput_MouseDevice;
        if (device != 0) {
            const int result = device->Unacquire();
            if (result != kDiOk && result != kDiFalse) {
                g_zInput_MouseActive = 1;
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-shutdowndevice
 * @recoil-artifact defines .text recoil:function:0x470360: zInput::Mouse_ShutdownDevice.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: deactivate mouse acquisition, release the mouse DirectInput device,
 * and clear mouse device lifetime state.
 *
 * Evidence: BN HLIL clears g_zInput_MouseActive, calls
 * Mouse_UpdateAcquireState, conditionally releases g_zInput_MouseDevice, then
 * clears g_zInput_MouseDevice and g_zInput_MouseInitialized before returning 1.
 */
int Mouse_ShutdownDevice() {
    g_zInput_MouseActive = 0;
    Mouse_UpdateAcquireState();

    DIDevice *device = g_zInput_MouseDevice;
    if (device != 0) {
        device->Release();
    }

    g_zInput_MouseDevice = 0;
    g_zInput_MouseInitialized = 0;
    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-getstatesnapshotptr
 * @recoil-artifact defines .text recoil:function:0x4703a0: zInput::Mouse_GetStateSnapshotPtr.
 * Purpose: Return the shared mouse state snapshot used by input consumers.
 */
MouseStateSnapshot *Mouse_GetStateSnapshotPtr() {
    return &g_zInput_MouseStateSnapshot;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-pollandstorestate
 * @recoil-artifact defines .text recoil:function:0x4703b0: zInput::Mouse_PollAndStoreState.
 * Purpose: Poll the mouse and store the latest DirectInput-style result code.
 */
void __fastcall Mouse_PollAndStoreState(
    unsigned char dispatchCallbacks
) {
    g_zInputMouseLastPollResult = Mouse_PollState(dispatchCallbacks);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-pollstate
 * @recoil-artifact defines .text recoil:function:0x4703c0: zInput::Mouse_PollState.
 * Purpose: Poll the DirectInput mouse state and update the zInput mouse snapshots.
 */
int __fastcall Mouse_PollState(
    unsigned char dispatchCallbacks
) {
    g_zInput_MouseStateSnapshot.deltaX = 0;
    g_zInput_MouseStateSnapshot.deltaY = 0;

    if (g_zInput_MouseActive == 0) {
        g_zInput_MouseActive = 1;
        Mouse_UpdateAcquireState();
        if (g_zInput_MouseActive == 0) {
            return kDiInputLost;
        }
    }

    DIDevice *device = g_zInput_MouseDevice;
    device->Poll();
    int result = device->GetDeviceState(
        sizeof(MouseDeviceState),
        &g_zInput_MouseRawDIState
    );
    if (result == kDiInputLost) {
        Mouse_UpdateAcquireState();
        return result;
    }

    if (result == kDiOk) {
        g_zInput_MousePreviousState = g_zInput_MouseCurrentState;
        g_zInput_MouseCurrentState = g_zInput_MouseRawDIState;
        g_zInput_MouseStateSnapshot.deltaX = g_zInput_MouseCurrentState.lX;
        g_zInput_MouseStateSnapshot.deltaY = g_zInput_MouseCurrentState.lY;
        Mouse_ApplyAccumulatedDelta();
        g_zInput_MouseStateSnapshot.button1Transition = Mouse_GetButtonTransitionState(1);
        g_zInput_MouseStateSnapshot.button2Transition = Mouse_GetButtonTransitionState(2);
        g_zInput_MouseStateSnapshot.button3Transition = Mouse_GetButtonTransitionState(3);

        if (g_zInput_BindMap_Current != 0 && dispatchCallbacks != 0) {
            g_zInput_BindMap_Current->DispatchMouseButtonCallbacks();
        }
    }

    return result;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-applyaccumulateddelta
 * @recoil-artifact defines .text recoil:function:0x4704f0: zInput::Mouse_ApplyAccumulatedDelta.
 *
 * Purpose: apply mouse sensitivity to accumulated deltas, update the client
 * cursor snapshot, clamp non-wrapping movement, and refresh normalized fields.
 *
 * Evidence: BN assembly at 0x4704f0 scales deltaX/deltaY through x87 _ftol,
 * reads g_zInput_MouseSensitivityX/Y at 0x4e08f4/0x4e08f8, updates the typed
 * g_zInput_MouseStateSnapshot at 0x561c80, clamps against client dimensions
 * when g_zInput_Mouse_WrapModeFlag is clear, then writes cursor/delta normals
 * from the center and inverse-center globals.
 */
void Mouse_ApplyAccumulatedDelta() {
    g_zInput_MouseStateSnapshot.deltaX =
        (int)((float)(g_zInput_MouseStateSnapshot.deltaX) * g_zInput_MouseSensitivityX);
    g_zInput_MouseStateSnapshot.deltaY =
        (int)((float)(g_zInput_MouseStateSnapshot.deltaY) * g_zInput_MouseSensitivityY);

    int cursorX = g_zInput_MouseStateSnapshot.cursorClientX + g_zInput_MouseStateSnapshot.deltaX;
    int cursorY = g_zInput_MouseStateSnapshot.cursorClientY + g_zInput_MouseStateSnapshot.deltaY;
    g_zInput_MouseStateSnapshot.cursorClientX = cursorX;
    g_zInput_MouseStateSnapshot.cursorClientY = cursorY;

    if (g_zInput_MouseWrapModeFlag == 0) {
        if (cursorX < 0) {
            cursorX = 0;
            g_zInput_MouseStateSnapshot.cursorClientX = 0;
        }
        if (cursorX >= g_zInput_MouseClientWidth) {
            cursorX = g_zInput_MouseClientWidth - 1;
            g_zInput_MouseStateSnapshot.cursorClientX = cursorX;
        }
        if (cursorY < 0) {
            cursorY = 0;
            g_zInput_MouseStateSnapshot.cursorClientY = 0;
        }
        if (cursorY >= g_zInput_MouseClientHeight) {
            cursorY = g_zInput_MouseClientHeight - 1;
            g_zInput_MouseStateSnapshot.cursorClientY = cursorY;
        }
    }

    g_zInput_MouseStateSnapshot.cursorNormX =
        (float)((double)(cursorX - g_zInput_MouseClientCenterX) * g_zInput_MouseInvClientCenterX);
    g_zInput_MouseStateSnapshot.cursorNormY =
        (float)((double)(cursorY - g_zInput_MouseClientCenterY) * g_zInput_MouseInvClientCenterY);
    g_zInput_MouseStateSnapshot.deltaNormX =
        (float)((double)(g_zInput_MouseStateSnapshot.deltaX) * g_zInput_MouseInvClientCenterX);
    g_zInput_MouseStateSnapshot.deltaNormY =
        (float)((double)(g_zInput_MouseStateSnapshot.deltaY) * g_zInput_MouseInvClientCenterY);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-getstatesnapshot
 * @recoil-artifact defines .text recoil:function:0x4705f0: zInput::Mouse_GetStateSnapshot.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Copy the current derived mouse snapshot to the caller and return
 * the last DirectInput mouse poll result.
 */
int __fastcall Mouse_GetStateSnapshot(
    MouseStateSnapshot *outState
) {
    if (outState != 0) {
        memcpy(
            outState,
            &g_zInput_MouseStateSnapshot,
            0x2c
        );
    }

    return g_zInputMouseLastPollResult;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-resettransitionstate
 * @recoil-artifact defines .text recoil:function:0x470610: zInput::Mouse_ResetTransitionState.
 *
 * Purpose: copy current mouse state into previous state, clear transition deltas,
 * and refresh the derived mouse snapshot when mouse input is initialized.
 *
 * Evidence: BN assembly at 0x470610 guards on g_zInput_MouseInitialized, copies
 * zInput_DIMouseState fields at 0x565e7c/0x565e8c, clears snapshot fields at
 * 0x561c90/0x561c94/0x561ca0/0x561ca4/0x561ca8, and tail-calls
 * Mouse_ApplyAccumulatedDelta.
 */
void Mouse_ResetTransitionState() {
    if (g_zInput_MouseInitialized != 1) {
        return;
    }

    g_zInput_MousePreviousState.lY = g_zInput_MouseCurrentState.lY;
    g_zInput_MouseStateSnapshot.deltaX = 0;
    g_zInput_MouseStateSnapshot.deltaY = 0;
    g_zInput_MouseStateSnapshot.button1Transition = 0;
    g_zInput_MouseStateSnapshot.button2Transition = 0;
    g_zInput_MouseStateSnapshot.button3Transition = 0;
    g_zInput_MousePreviousState.lX = g_zInput_MouseCurrentState.lX;
    g_zInput_MousePreviousState.lZ = g_zInput_MouseCurrentState.lZ;
    g_zInput_MousePreviousState.rgbButtons = g_zInput_MouseCurrentState.rgbButtons;
    Mouse_ApplyAccumulatedDelta();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-setcooperativelevelflags
 * @recoil-artifact defines .text recoil:function:0x470670: zInput::Mouse_SetCooperativeLevelFlags.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Replace the stored mouse DirectInput cooperative-level flags and
 * return the previous value.
 */
int __fastcall Mouse_SetCooperativeLevelFlags(
    int flags
) {
    const int previousFlags = g_zInput_MouseCoopLevelFlags;
    g_zInput_MouseCoopLevelFlags = flags;
    return previousFlags;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-waitforbuttonpress
 * @recoil-artifact defines .text recoil:function:0x470680: zInput::Mouse_WaitForButtonPress.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zin_mouse.cpp.
 * Purpose: Poll mouse input until a newly pressed button is found or the
 * caller requests a single scan.
 */
int __fastcall Mouse_WaitForButtonPress(
    int pollUntilFound
) {
    int result = 0;
    do {
        Mouse_PollState(1);
        {
            for (int button = 1; button < 4; ++button) {
                if (Mouse_GetButtonTransitionState(button) == 2) {
                    result = button;
                    break;
                }
            }
        }

        Mouse_ResetTransitionState();
    } while (result == 0 && pollUntilFound != 0);

    return result;
}

} // namespace zInput
