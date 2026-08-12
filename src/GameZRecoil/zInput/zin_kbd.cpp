#include "zinput.h"

#include <stdlib.h>
#include <string.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-data-x27
 * @recoil-artifact defines .data recoil:data:0x4e08cc: g_zInput_SourceFile_ZinKbdCpp.
 * BN types this writable char[0x27] as the zin_kbd.cpp source-path literal
 * passed to DI_ReportError by keyboard DirectInput failure paths.
 * Purpose: Supplies the original keyboard source-file path for diagnostics.
 */
char g_zInput_SourceFile_ZinKbdCpp[0x27] =
    "D:\\Proj\\GameZRecoil\\zInput\\zin_kbd.cpp";
}

namespace zInput {

const int kDiOk = 0;
const int kDiInputLost = (int)(0x8007001e);
const unsigned int kZInputKeyboardEventBufferCount = 0x80;

struct DipropDwordInit {
    unsigned int dwSize;
    unsigned int dwHeaderSize;
    unsigned int dwObj;
    unsigned int dwHow;
    unsigned int dwData;
};

typedef int(__fastcall *KeyboardRawEventCallbackFn)(
    int ascii,
    void *context
);
typedef void(__fastcall *KeyboardComboCallbackFn)(int comboIdx);

/**
 * Recovered inline helper: zInput keyboard modifier state update.
 * Original-source helper evidence: No standalone retail function exists;
 * observed inline in 0x46fa10 and 0x46f690 where the keyboard wait/poll paths
 * set or clear the shared modifier mask for shift/control/alt DIK events.
 * Purpose: Apply a pressed/released transition to one keyboard modifier bit.
 */
inline void UpdateKeyboardModifierState(
    int mask,
    bool pressed
) {
    if (pressed) {
        g_zInput_KbdModifierState |= mask;
    } else {
        g_zInput_KbdModifierState &= ~mask;
    }
}

/**
 * Recovered inline helper: zInput keyboard poll event application.
 * Original-source helper evidence: No standalone retail function exists;
 * observed inline in 0x46f690 where the frame poll path updates modifier
 * state, dispatch-state slots, and optional raw ASCII callbacks.
 * Purpose: Apply one buffered keyboard poll event and return its dispatch index.
 */
inline int ApplyKeyboardPollEvent(
    DIDeviceObjectData &event
) {
    unsigned int dispatchIndex = event.dwOfs;
    switch (event.dwOfs) {
    case 0x38:
    case 0xb8:
        UpdateKeyboardModifierState(
            0x100,
            (event.dwData & 0x80) != 0
        );
        break;
    case 0x1d:
    case 0x9d:
        UpdateKeyboardModifierState(
            0x200,
            (event.dwData & 0x80) != 0
        );
        break;
    case 0x2a:
    case 0x36:
        UpdateKeyboardModifierState(
            0x400,
            (event.dwData & 0x80) != 0
        );
        break;
    default:
        if (g_zInput_KbdModifierState != 0 && g_zInputKbdKeyDispatchTable[event.dwOfs].state != 0) {
            g_zInputKbdKeyDispatchTable[event.dwOfs].state = 4;
        }
        if ((g_zInput_KbdModifierState & 0x100) != 0) {
            event.dwData |= 0x40;
        }
        if ((g_zInput_KbdModifierState & 0x200) != 0) {
            event.dwData |= 0x20;
        }
        if ((g_zInput_KbdModifierState & 0x400) != 0) {
            event.dwData |= 0x10;
        }
        dispatchIndex |= (unsigned int)(g_zInput_KbdModifierState);
        break;
    }

    KbdKeyDispatchEntry &dispatch = g_zInputKbdKeyDispatchTable[dispatchIndex];
    if ((event.dwData & 0x80) != 0) {
        dispatch.state = dispatch.state == 1 ? 3 : 1;
        if (g_zInput_KbdRawEventCallback != 0) {
            KeyboardRawEventCallbackFn callback =
                (KeyboardRawEventCallbackFn)(g_zInput_KbdRawEventCallback);
            callback(
                Keyboard_TranslateDikToAscii((int)(dispatchIndex)),
                g_zInput_KbdRawEventCallbackCtx
            );
        }
    } else {
        dispatch.state |= 4;
    }

    return (int)(dispatchIndex);
}

/**
 * Recovered inline helper: zInput keyboard callback dispatch index builder.
 * Original-source helper evidence: No standalone retail function exists;
 * observed inline in 0x46f690's second pass where processed dwData modifier
 * bits are folded back into a keyboard combo index before callback dispatch.
 * Purpose: Rebuild a modifier-aware keyboard combo index from a processed event.
 */
inline int KeyboardEventDispatchIndex(
    const DIDeviceObjectData &event
) {
    int dispatchIndex = (int)(event.dwOfs);
    if ((event.dwData & 0x40) != 0) {
        dispatchIndex |= 0x100;
    }
    if ((event.dwData & 0x20) != 0) {
        dispatchIndex |= 0x200;
    }
    if ((event.dwData & 0x10) != 0) {
        dispatchIndex |= 0x400;
    }

    return dispatchIndex;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_initdevice
 * @recoil-artifact defines .text recoil:function:0x46f300: zInput::Keyboard_InitDevice.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_kbd.cpp.
 * Purpose: Create and configure the DirectInput keyboard device, allocate the
 * buffered event storage, and clear transition/callback state.
 *
 * Evidence: BN clears the keyboard fields inside g_zInput_GlobalStateStorage,
 * configures cooperative level 10, DIPROP_BUFFERSIZE 128, c_dfDIKeyboard, and
 * Acquire, reporting zin_kbd.cpp line numbers on each provider failure.
 */
int __cdecl Keyboard_InitDevice() {
    DipropDwordInit bufferSizeProp =
        {0x14, 0x10, 0, 0, kZInputKeyboardEventBufferCount};
    g_zInput_KbdSystemReady = 0;
    g_zInput_KbdDevice = 0;
    g_zInput_KbdEventBuffer = 0;
    g_zInput_KbdModifierState = 0;
    g_zInput_KbdRawEventCallback = 0;
    g_zInput_KbdRawEventCallbackCtx = 0;

    int hr = g_zInput_GlobalState->CreateDevice(
        GUID_SysKeyboard,
        (LPDIRECTINPUTDEVICEA *)(&g_zInput_KbdDevice),
        0
    );
    if (hr != 0) {
        DI_ReportError(
            hr,
            g_zInput_SourceFile_ZinKbdCpp,
            0x95
        );
        return 1;
    }

    hr =
        g_zInput_KbdDevice->SetCooperativeLevel(
            g_zInput_hWnd,
            0xa
        );
    if (hr != 0) {
        DI_ReportError(
            hr,
            g_zInput_SourceFile_ZinKbdCpp,
            0x9d
        );
        return 1;
    }

    hr = g_zInput_KbdDevice->SetProperty(
        DIPROP_BUFFERSIZE,
        (LPCDIPROPHEADER)(&bufferSizeProp)
    );
    if (hr != 0) {
        DI_ReportError(
            hr,
            g_zInput_SourceFile_ZinKbdCpp,
            0xa5
        );
        return 1;
    }

    hr = g_zInput_KbdDevice->SetDataFormat(
        &c_dfDIKeyboard
    );
    if (hr != 0) {
        DI_ReportError(
            hr,
            g_zInput_SourceFile_ZinKbdCpp,
            0xad
        );
        return 1;
    }

    hr = g_zInput_KbdDevice->Acquire();
    if (hr != 0) {
        DI_ReportError(
            hr,
            g_zInput_SourceFile_ZinKbdCpp,
            0xb6
        );
        return 1;
    }

    g_zInput_KbdEventBuffer = (DIDeviceObjectData *)(calloc(
        kZInputKeyboardEventBufferCount,
        sizeof(DIDeviceObjectData)
    ));
    g_zInput_KbdSystemReady = 1;
    Keyboard_ResetTransitionState();
    Keyboard_ClearKeyCallbackTable();
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_shutdowndevice
 * @recoil-artifact defines .text recoil:function:0x46f420: zInput::Keyboard_ShutdownDevice.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_kbd.cpp.
 * Purpose: unacquire and release the keyboard DirectInput device, then free
 * the buffered keyboard event storage.
 *
 * Evidence: BN HLIL guards g_zInput_KbdDevice before Unacquire and Release,
 * then frees g_zInput_KbdEventBuffer when present and returns 0.
 */
int __cdecl Keyboard_ShutdownDevice() {
    DIDevice *const keyboard = g_zInput_KbdDevice;
    if (keyboard != 0) {
        keyboard->Unacquire();
        g_zInput_KbdDevice->Release();
    }

    if (g_zInput_KbdEventBuffer != 0) {
        free(g_zInput_KbdEventBuffer);
    }

    return 0;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_resettransitionstate
 * @recoil-artifact defines .text recoil:function:0x46f450: zInput::Keyboard_ResetTransitionState.
 *
 * Purpose: drain pending keyboard events, update transient modifier state, then
 * clear every key-dispatch transition state for the next input frame.
 *
 * Evidence: BN assembly at 0x46f450 calls IDirectInputDevice::GetDeviceData
 * with 0x80 DIDeviceObjectData entries, reacquires on DIERR_INPUTLOST, applies
 * the same modifier fix-up as keyboard polling, clears the 0x7de-entry
 * g_zInputKbdKeyDispatchTable state column, and resets g_zInput_KbdModifierState.
 */
void __cdecl Keyboard_ResetTransitionState() {
    if (g_zInput_KbdSystemReady == 0) {
        return;
    }

    DWORD inOutCount = kZInputKeyboardEventBufferCount;
    const int hresult = g_zInput_KbdDevice->GetDeviceData(
        sizeof(DIDeviceObjectData),
        g_zInput_KbdEventBuffer,
        &inOutCount,
        0
    );
    if (hresult != kDiOk) {
        if (hresult == kDiInputLost) {
            g_zInput_KbdDevice->Acquire();
        } else {
            DI_ReportError(
                hresult,
                g_zInput_SourceFile_ZinKbdCpp,
                257
            );
            return;
        }
    }

    DIDeviceObjectData *event = g_zInput_KbdEventBuffer;
    unsigned int controlDataFlag = 0x20;
    for (unsigned int i = 0; i < inOutCount; ++i, ++event) {
        unsigned int dispatchIndex = event->dwOfs;
        switch (dispatchIndex) {
        case 0x38:
        case 0xb8:
            if ((event->dwData & 0x80) != 0) {
                g_zInput_KbdModifierState |= 0x100;
            } else {
                g_zInput_KbdModifierState &= ~0x100;
            }
            break;
        case 0x1d:
        case 0x9d:
            if ((event->dwData & 0x80) != 0) {
                g_zInput_KbdModifierState |= 0x200;
            } else {
                g_zInput_KbdModifierState &= ~0x200;
            }
            break;
        case 0x2a:
        case 0x36:
            if ((event->dwData & 0x80) != 0) {
                g_zInput_KbdModifierState |= 0x400;
            } else {
                g_zInput_KbdModifierState &= ~0x400;
            }
            break;
        default:
            if ((g_zInput_KbdModifierState & 0x100) != 0) {
                event->dwData |= 0x40;
            }
            if ((g_zInput_KbdModifierState & 0x200) != 0) {
                event->dwData |= controlDataFlag;
            }
            if ((g_zInput_KbdModifierState & 0x400) != 0) {
                event->dwData |= 0x10;
            }

            dispatchIndex |= (unsigned int)(g_zInput_KbdModifierState);
            break;
        }

        KbdKeyDispatchEntry &dispatch = g_zInputKbdKeyDispatchTable[dispatchIndex];
        if ((event->dwData & 0x80) != 0) {
            dispatch.state = dispatch.state == 1 ? 3 : 1;
        } else {
            dispatch.state |= 4;
        }
    }

    {
        unsigned int entryIndex4;
        for (entryIndex4 = 0; entryIndex4 < sizeof(g_zInputKbdKeyDispatchTable) /
                                                sizeof(g_zInputKbdKeyDispatchTable[0]);
            ++entryIndex4) {
            KbdKeyDispatchEntry &entry = g_zInputKbdKeyDispatchTable[entryIndex4];
            entry.state = 0;
        }
    }
    g_zInput_KbdModifierState = 0;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_pollstate
 * @recoil-artifact defines .text recoil:function:0x46f690: zInput::Keyboard_PollState.
 * BN zin_kbd.cpp evidence shows a 0x80-event DirectInput GetDeviceData pump,
 * DIERR_INPUTLOST reacquire handling, modifier-aware transition updates, raw
 * ASCII callback dispatch, and an optional second pass for combo callbacks.
 * Purpose: Poll keyboard events for one frame and update or dispatch key state.
 */
void __fastcall Keyboard_PollState(
    unsigned char dispatchCallbacks
) {
    DWORD inOutCount = kZInputKeyboardEventBufferCount;
    const int hresult = g_zInput_KbdDevice->GetDeviceData(
        sizeof(DIDeviceObjectData),
        g_zInput_KbdEventBuffer,
        &inOutCount,
        0
    );
    if (hresult != kDiOk) {
        if (hresult != kDiInputLost) {
            DI_ReportError(
                hresult,
                g_zInput_SourceFile_ZinKbdCpp,
                0x170
            );
            return;
        }

        g_zInput_KbdDevice->Acquire();
    }

    for (unsigned int i = 0; i < inOutCount; ++i) {
        ApplyKeyboardPollEvent(g_zInput_KbdEventBuffer[i]);
    }

    if (dispatchCallbacks == 0) {
        return;
    }

    for (unsigned int i_2239 = 0; i_2239 < inOutCount; ++i_2239) {
        const int dispatchIndex = KeyboardEventDispatchIndex(g_zInput_KbdEventBuffer[i_2239]);
        if (dispatchIndex < 0 || dispatchIndex >= 0x7de) {
            continue;
        }

        void *const callback = g_zInputKbdKeyDispatchTable[dispatchIndex].callback;
        if (callback != 0 && (g_zInputKbdKeyDispatchTable[dispatchIndex].state & 1) != 0) {
            ((KeyboardComboCallbackFn)(callback))(dispatchIndex);
            g_zInputKbdKeyDispatchTable[dispatchIndex].state = 0;
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_setraweventcallback
 * @recoil-artifact defines .text recoil:function:0x46f970: zInput::Keyboard_SetRawEventCallback.
 * Purpose: install the raw keyboard event callback and caller context.
 */
void __fastcall Keyboard_SetRawEventCallback(
    void *callback,
    void *context
) {
    g_zInput_KbdRawEventCallback = callback;
    g_zInput_KbdRawEventCallbackCtx = context;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_getkeytransitionstate
 * @recoil-artifact defines .text recoil:function:0x46f980: zInput::Keyboard_GetKeyTransitionState.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_kbd.cpp.
 * Purpose: Return and advance the transition state for one modifier-aware
 * keyboard dispatch slot.
 */
int __fastcall Keyboard_GetKeyTransitionState(
    int keyIndex
) {
    const int state = g_zInputKbdKeyDispatchTable[keyIndex].state;
    if ((state & 4) != 0) {
        g_zInputKbdKeyDispatchTable[keyIndex].state = 0;
        return state;
    }
    if (state == 1) {
        g_zInputKbdKeyDispatchTable[keyIndex].state = 2;
    }

    return state;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_registerkeycallback
 * @recoil-artifact defines .text recoil:function:0x46f9b0: zInput::Keyboard_RegisterKeyCallback.
 * Purpose: install a keyboard dispatch callback for an unused modifier-aware key slot.
 */
int __fastcall Keyboard_RegisterKeyCallback(
    int comboIdx,
    void *callback,
    const char * /*unusedLabel*/
) {
    if (g_zInputKbdKeyDispatchTable[comboIdx].callback != 0) {
        return -1;
    }

    g_zInputKbdKeyDispatchTable[comboIdx].callback = callback;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_unregisterkeycallback
 * @recoil-artifact defines .text recoil:function:0x46f9d0: zInput::Keyboard_UnregisterKeyCallback.
 * Purpose: clear a keyboard dispatch callback slot while preserving its key state.
 */
void __fastcall Keyboard_UnregisterKeyCallback(
    int comboIdx
) {
    if (g_zInputKbdKeyDispatchTable[comboIdx].callback != 0) {
        g_zInputKbdKeyDispatchTable[comboIdx].callback = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_clearkeycallbacktable
 * @recoil-artifact defines .text recoil:function:0x46f9f0: zInput::Keyboard_ClearKeyCallbackTable.
 * Purpose: clear all keyboard dispatch callback slots while preserving key states.
 */
void __cdecl Keyboard_ClearKeyCallbackTable() {
    int entryIndex3;
    for (entryIndex3 = 0; entryIndex3 < (int)(sizeof(g_zInputKbdKeyDispatchTable) /
                                              sizeof(g_zInputKbdKeyDispatchTable[0]));
        ++entryIndex3) {
        KbdKeyDispatchEntry &entry = g_zInputKbdKeyDispatchTable[entryIndex3];
        entry.callback = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_waitforanykeypress
 * @recoil-artifact defines .text recoil:function:0x46fa10: zInput::Keyboard_WaitForAnyKeyPress.
 * BN zin_kbd.cpp evidence shows a one-event DirectInput GetDeviceData loop,
 * DIERR_INPUTLOST reacquire handling, and inline keyboard transition updates.
 * Purpose: Wait for or poll one keyboard press and return its modifier-combined key index.
 */
int __fastcall Keyboard_WaitForAnyKeyPress(
    int keepWaiting
) {
    DWORD inOutCount = 1;
    int result = 0;
    do {
        const int hresult = g_zInput_KbdDevice->GetDeviceData(
            sizeof(DIDeviceObjectData),
            g_zInput_KbdEventBuffer,
            &inOutCount,
            0
        );
        if (hresult != kDiOk) {
            if (hresult != kDiInputLost) {
                DI_ReportError(
                    hresult,
                    g_zInput_SourceFile_ZinKbdCpp,
                    0x291
                );
                return 0;
            }

            g_zInput_KbdDevice->Acquire();
        }

        DIDeviceObjectData *event = g_zInput_KbdEventBuffer;
        for (unsigned int i = 0; i < inOutCount; ++i, ++event) {
            KbdKeyDispatchEntry *dispatch;
            result = (int)(event->dwOfs);
            if (result == 0x38 || result == 0xb8) {
                if ((event->dwData & 0x80) != 0) {
                    g_zInput_KbdModifierState |= 0x100;
                } else {
                    g_zInput_KbdModifierState &= ~0x100;
                }
                dispatch = &g_zInputKbdKeyDispatchTable[result];
                result = 0;
            } else if (result == 0x1d || result == 0x9d) {
                if ((event->dwData & 0x80) != 0) {
                    g_zInput_KbdModifierState |= 0x200;
                } else {
                    g_zInput_KbdModifierState &= ~0x200;
                }
                dispatch = &g_zInputKbdKeyDispatchTable[result];
                result = 0;
            } else if (result == 0x2a || result == 0x36) {
                if ((event->dwData & 0x80) != 0) {
                    g_zInput_KbdModifierState |= 0x400;
                } else {
                    g_zInput_KbdModifierState &= ~0x400;
                }
                dispatch = &g_zInputKbdKeyDispatchTable[result];
                result = 0;
            } else {
                if ((g_zInput_KbdModifierState & 0x100) != 0) {
                    event->dwData |= 0x40;
                }
                if ((g_zInput_KbdModifierState & 0x200) != 0) {
                    event->dwData |= 0x20;
                }
                if ((g_zInput_KbdModifierState & 0x400) != 0) {
                    event->dwData |= 0x10;
                }
                result |= (int)(g_zInput_KbdModifierState);
                dispatch = &g_zInputKbdKeyDispatchTable[result];
            }

            if ((event->dwData & 0x80) != 0) {
                dispatch->state = dispatch->state == 1 ? 3 : 1;
            } else {
                dispatch->state |= 4;
                result = 0;
            }
        }
        if (result != 0) {
            break;
        }
    } while (keepWaiting != 0);

    return result;
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_translatediktoascii
 * @recoil-artifact defines .text recoil:function:0x46fba0: zInput::Keyboard_TranslateDikToAscii.
 * Purpose: Translate a modifier-combined DIK scan code to an ASCII/control code.
 */
int __fastcall Keyboard_TranslateDikToAscii(
    int comboIdx
) {
    if (g_zInput_KbdDikToAsciiTableReady == 0) {
        Keyboard_InitDikToAsciiTable();
        g_zInput_KbdDikToAsciiTableReady = 1;
    }

    int result = g_zInput_KbdDikToAsciiTable[comboIdx & 0xff];
    if (result >= 'a' && result <= 'z' && (comboIdx & 0x400) != 0) {
        result -= 0x20;
    }

    const unsigned int shiftedOffset = (unsigned int)(comboIdx - 0x402);
    if (shiftedOffset > 0x33) {
        return result;
    }

    switch (shiftedOffset) {
    case 0x00:
        return '!';
    case 0x01:
        return '@';
    case 0x02:
        return '#';
    case 0x03:
        return '$';
    case 0x04:
        return '%';
    case 0x05:
        return '^';
    case 0x06:
        return '&';
    case 0x07:
        return '*';
    case 0x08:
        return '(';
    case 0x09:
        return ')';
    case 0x27:
        return '~';
    case 0x0a:
        return '_';
    case 0x0b:
        return '+';
    case 0x29:
        return '|';
    case 0x18:
        return '{';
    case 0x19:
        return '}';
    case 0x25:
        return ':';
    case 0x26:
        return '"';
    case 0x31:
        return '<';
    case 0x32:
        return '>';
    case 0x33:
        return '?';
    default:
        return result;
    }
}

/**
 * @recoil-anchor recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_initdiktoasciitable
 * @recoil-artifact defines .text recoil:function:0x46fd20: zInput::Keyboard_InitDikToAsciiTable.
 * Purpose: Initialize the DIK scan-code to ASCII/control-code lookup table.
 */
void Keyboard_InitDikToAsciiTable() {
    memset(
        g_zInput_KbdDikToAsciiTable,
        0,
        sizeof(g_zInput_KbdDikToAsciiTable)
    );

    g_zInput_KbdDikToAsciiTable[0x02] = '1';
    g_zInput_KbdDikToAsciiTable[0x03] = '2';
    g_zInput_KbdDikToAsciiTable[0x04] = '3';
    g_zInput_KbdDikToAsciiTable[0x05] = '4';
    g_zInput_KbdDikToAsciiTable[0x06] = '5';
    g_zInput_KbdDikToAsciiTable[0x07] = '6';
    g_zInput_KbdDikToAsciiTable[0x08] = '7';
    g_zInput_KbdDikToAsciiTable[0x09] = '8';
    g_zInput_KbdDikToAsciiTable[0x0a] = '9';
    g_zInput_KbdDikToAsciiTable[0x0b] = '0';
    g_zInput_KbdDikToAsciiTable[0x0c] = '-';
    g_zInput_KbdDikToAsciiTable[0x0d] = '=';
    g_zInput_KbdDikToAsciiTable[0x0e] = 0x08;
    g_zInput_KbdDikToAsciiTable[0x0f] = 0x09;
    g_zInput_KbdDikToAsciiTable[0x10] = 'q';
    g_zInput_KbdDikToAsciiTable[0x11] = 'w';
    g_zInput_KbdDikToAsciiTable[0x12] = 'e';
    g_zInput_KbdDikToAsciiTable[0x13] = 'r';
    g_zInput_KbdDikToAsciiTable[0x14] = 't';
    g_zInput_KbdDikToAsciiTable[0x15] = 'y';
    g_zInput_KbdDikToAsciiTable[0x16] = 'u';
    g_zInput_KbdDikToAsciiTable[0x17] = 'i';
    g_zInput_KbdDikToAsciiTable[0x18] = 'o';
    g_zInput_KbdDikToAsciiTable[0x19] = 'p';
    g_zInput_KbdDikToAsciiTable[0x1a] = '[';
    g_zInput_KbdDikToAsciiTable[0x1b] = ']';
    g_zInput_KbdDikToAsciiTable[0x1c] = 0x0d;
    g_zInput_KbdDikToAsciiTable[0x1e] = 'a';
    g_zInput_KbdDikToAsciiTable[0x1f] = 's';
    g_zInput_KbdDikToAsciiTable[0x20] = 'd';
    g_zInput_KbdDikToAsciiTable[0x21] = 'f';
    g_zInput_KbdDikToAsciiTable[0x22] = 'g';
    g_zInput_KbdDikToAsciiTable[0x23] = 'h';
    g_zInput_KbdDikToAsciiTable[0x24] = 'j';
    g_zInput_KbdDikToAsciiTable[0x25] = 'k';
    g_zInput_KbdDikToAsciiTable[0x26] = 'l';
    g_zInput_KbdDikToAsciiTable[0x27] = ';';
    g_zInput_KbdDikToAsciiTable[0x28] = '\'';
    g_zInput_KbdDikToAsciiTable[0x29] = '~';
    g_zInput_KbdDikToAsciiTable[0x2b] = '\\';
    g_zInput_KbdDikToAsciiTable[0x2c] = 'z';
    g_zInput_KbdDikToAsciiTable[0x2d] = 'x';
    g_zInput_KbdDikToAsciiTable[0x2e] = 'c';
    g_zInput_KbdDikToAsciiTable[0x2f] = 'v';
    g_zInput_KbdDikToAsciiTable[0x30] = 'b';
    g_zInput_KbdDikToAsciiTable[0x31] = 'n';
    g_zInput_KbdDikToAsciiTable[0x32] = 'm';
    g_zInput_KbdDikToAsciiTable[0x33] = ',';
    g_zInput_KbdDikToAsciiTable[0x34] = '.';
    g_zInput_KbdDikToAsciiTable[0x35] = '/';
    g_zInput_KbdDikToAsciiTable[0x37] = '*';
    g_zInput_KbdDikToAsciiTable[0x39] = ' ';
    g_zInput_KbdDikToAsciiTable[0x4a] = '-';
    g_zInput_KbdDikToAsciiTable[0x4e] = '+';
    g_zInput_KbdDikToAsciiTable[0x47] = '7';
    g_zInput_KbdDikToAsciiTable[0x48] = '8';
    g_zInput_KbdDikToAsciiTable[0x49] = '9';
    g_zInput_KbdDikToAsciiTable[0x4b] = '4';
    g_zInput_KbdDikToAsciiTable[0x4c] = '5';
    g_zInput_KbdDikToAsciiTable[0x4d] = '6';
    g_zInput_KbdDikToAsciiTable[0x4f] = '1';
    g_zInput_KbdDikToAsciiTable[0x50] = '2';
    g_zInput_KbdDikToAsciiTable[0x51] = '3';
    g_zInput_KbdDikToAsciiTable[0x52] = '0';
    g_zInput_KbdDikToAsciiTable[0x53] = '.';
    g_zInput_KbdDikToAsciiTable[0x8d] = '=';
    g_zInput_KbdDikToAsciiTable[0x90] = '~';
    g_zInput_KbdDikToAsciiTable[0x91] = '@';
    g_zInput_KbdDikToAsciiTable[0x92] = ':';
    g_zInput_KbdDikToAsciiTable[0x93] = '_';
    g_zInput_KbdDikToAsciiTable[0x9c] = 0x0d;
    g_zInput_KbdDikToAsciiTable[0xb3] = ',';
    g_zInput_KbdDikToAsciiTable[0xb5] = '/';
    g_zInput_KbdDikToAsciiTable[0xcb] = 0x02;
    g_zInput_KbdDikToAsciiTable[0xcd] = 0x06;
    g_zInput_KbdDikToAsciiTable[0xd3] = 0x7f;
}

} // namespace zInput
