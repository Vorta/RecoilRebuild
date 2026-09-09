#include "zinput.h"

#include <stdlib.h>

extern "C" {
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-x28
 * @recoil-artifact defines .data recoil:data:0x4e0c9c: g_zInput_SourceFile_ZinInitCpp.
 * BN types this writable char[0x28] as the zin_init.cpp source-path literal
 * passed to DIReportError when DirectInputCreateA fails.
 * Purpose: Supplies the original init source-file path for diagnostics.
 */
char g_zInput_SourceFile_ZinInitCpp[0x28] =
    "D:\\Proj\\GameZRecoil\\zInput\\zin_init.cpp";
}

namespace zInput {

const unsigned char kSuspendFlag = 2;
const unsigned int kDirectInputVersion = 0x500;

inline zInput_BindMapOverlayStackNode *__fastcall BindMapOverlayDetachHead(
    zInput_BindMapOverlayStackNode **head
);
inline void __fastcall BindMapOverlayDeleteNodeList(
    zInput_BindMapOverlayStackNode **head
);

/**
 * Original-source helper evidence: zInput suspend flag test.
 * No standalone retail function exists; observed caller bodies at 0x471c60 and
 * 0x471c70 use this same bit-1 clear test, matching the keyboard variant at
 * 0x471c80.
 * Purpose: Convert a device registry flag byte into an unsuspended boolean.
 */
inline int IsUnsuspended(
    unsigned char flags
) {
    return (~flags & kSuspendFlag) >> 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-globalstatestaticinitandregisteratexit
 * @recoil-artifact defines .text recoil:function:0x4719e0: zInput::GlobalStateStaticInitAndRegisterAtExit.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly calls zInput::GlobalStateStaticInit and tail-jumps to
 * zInput::GlobalStateRegisterAtExit.
 * Purpose: perform zInput global-state static construction and register its
 * CRT shutdown callback.
 */
void __cdecl GlobalStateStaticInitAndRegisterAtExit() {
    GlobalStateStaticInit();
    GlobalStateRegisterAtExit();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *ZInputCrtInitializerFn)();
/* VC5 emits these zInput startup callbacks as direct .CRT$XCU rows. */
#pragma data_seg(".CRT$XCU")
ZInputCrtInitializerFn s_zInputCrtInit_GlobalState =
    GlobalStateStaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-globalstatestaticinit
 * @recoil-artifact defines .text recoil:function:0x4719f0: zInput::GlobalStateStaticInit.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly loads 0x561cb0 as the static-object this pointer and tail-jumps
 * to zInput_GlobalState::Constructor.
 * Purpose: run zInput global-state static construction.
 */
void *GlobalStateStaticInit() {
    return GlobalStateConstructor(&g_zInput_GlobalStateStorage);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-globalstateregisteratexit
 * @recoil-artifact defines .text recoil:function:0x471a00: zInput::GlobalStateRegisterAtExit.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly pushes zInput::GlobalStateAtExitDestructor and calls the CRT
 * atexit provider.
 * Purpose: register the zInput global-state static destructor.
 */
int GlobalStateRegisterAtExit() {
    return atexit(GlobalStateAtExitDestructor);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-globalstateatexitdestructor
 * @recoil-artifact defines .text recoil:function:0x471a10: zInput::GlobalStateAtExitDestructor.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly loads 0x561cb0 as the static-object this pointer and tail-jumps
 * to zInput_GlobalState::Destructor.
 * Purpose: expose the zInput global-state destructor as a CRT atexit callback.
 */
void __cdecl GlobalStateAtExitDestructor() {
    GlobalStateDestructor(&g_zInput_GlobalStateStorage);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-globalstatedestructor
 * @recoil-artifact defines .text recoil:function:0x471a20: zInput_GlobalState::Destructor.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly drains the overlay free-list, drains the auxiliary block-list,
 * clears both list heads plus stack head/reserved/depth, and leaves the block
 * size field intact.
 * Purpose: tear down the zInput bind-map overlay static lifetime state.
 */
void __fastcall GlobalStateDestructor(
    zInput_GlobalState *self
) {
    BindMapOverlayDeleteNodeList(&self->bindMapOverlayNodeFreeList);
    BindMapOverlayDeleteNodeList(&self->bindMapOverlayNodeBlockList);
    self->bindMapOverlayNodeBlockList = 0;
    self->bindMapOverlayNodeFreeList = 0;
    self->bindMapOverlayReserved = 0;
    self->bindMapOverlayNodeStackHead = 0;
    self->bindMapOverlayDepth = 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-globalstateconstructor
 * @recoil-artifact defines .text recoil:function:0x471ab0: zInput_GlobalState::Constructor.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly writes the overlay lifetime fields at data addresses
 * 0x565ea4..0x565eb8 through the 0x561cb0 static-object base; the rebuilt
 * source keeps the retail storage as named globals instead of adding a
 * duplicate aggregate mirror.
 * Purpose: initialize the zInput bind-map overlay static lifetime state.
 */
void *__fastcall GlobalStateConstructor(
    zInput_GlobalState *self
) {
    self->bindMapOverlayNodeBlockList = 0;
    self->bindMapOverlayNodeFreeList = 0;
    self->bindMapOverlayReserved = 0;
    self->bindMapOverlayNodeStackHead = 0;
    self->bindMapOverlayDepth = 0;
    self->bindMapOverlayBlockSize = 8;
    return self;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-onappdeactivate
 * @recoil-artifact defines .text recoil:function:0x471ae0: zInput::OnAppDeactivate.
 * @recoil-match byte
 *
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: suspend active input devices during app deactivation, then mark the
 * mouse inactive and update DirectInput acquisition state.
 * Evidence: BN assembly calls the joystick, mouse, and keyboard unsuspended
 * tests in that order, sets only the needed suspend bits, stores 0 to
 * g_zInput_MouseActive, and tail-jumps to MouseUpdateAcquireState.
 */
void __cdecl OnAppDeactivate() {
    if (JoystickIsUnsuspended() != 0) {
        JoystickSuspend();
    }

    if (MouseIsUnsuspended() != 0) {
        MouseSuspend();
    }

    if (zInputKeyboardIsUnsuspended() != 0) {
        KeyboardSuspend();
    }

    g_zInput_MouseActive = 0;
    MouseUpdateAcquireState();
}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-onappactivate
 * @recoil-artifact defines .text recoil:function:0x471b20: zInput::OnAppActivate.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: resume suspended input devices when a window is active, then mark
 * the mouse active and update DirectInput acquisition state.
 *
 * Evidence: BN assembly gates on g_zInput_hWnd, resumes joystick, keyboard,
 * then mouse suspend state, stores 1 to g_zInput_MouseActive, and tail-jumps
 * to MouseUpdateAcquireState.
 */
void __cdecl OnAppActivate() {
    if (g_zInput_hWnd == 0) {
        return;
    }

    JoystickResumeFromSuspend();
    KeyboardResumeFromSuspend();
    MouseResumeFromSuspend();
    g_zInput_MouseActive = 1;
    MouseUpdateAcquireState();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-init
 * @recoil-artifact defines .text recoil:function:0x471b50: zInput::Init.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: initialize DirectInput, clear device status state, create keyboard,
 * mouse, and joystick devices, then acquire keyboard and mouse poll refs.
 */
int __fastcall Init(
    HWND hWnd,
    HINSTANCE hInstance
) {
    if (g_zInput_hWnd != 0) {
        return 1;
    }

    g_zInput_hWnd = 0;
    g_zInput_DeviceRegistry = 0;
    g_zInputJoystickFlags = 0;
    g_zInputMouseFlags = 0;
    g_zInputKeyboardPollRefCount = 0;
    g_zInputJoystickPollRefCount = 0;
    g_zInputMousePollRefCount = 0;

    const HRESULT hr = DirectInputCreateA(
        hInstance,
        kDirectInputVersion,
        (LPDIRECTINPUTA *)(&g_zInput_GlobalState),
        0
    );
    if (hr != 0) {
        DIReportError(hr, g_zInput_SourceFile_ZinInitCpp, 0x93);
        return -1;
    }

    g_zInput_hWnd = hWnd;
    g_zInput_DeviceRegistry = KeyboardInitDevice() == 0 ? (unsigned char)(1) : 0;
    g_zInputMouseFlags = MouseInitDevice() != 0 ? (unsigned char)(1) : 0;
    g_zInputJoystickFlags = DIInitJoystickDevice(hWnd) != 0 ? 1 : 0;
    KeyboardAddRef();
    MouseAddRef();
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-shutdown
 * @recoil-artifact defines .text recoil:function:0x471c10: zInput::Shutdown.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: shut down joystick, keyboard, mouse, and DirectInput state, then
 * clear the input window handle.
 */
int __cdecl Shutdown() {
    if (g_zInput_hWnd == 0) {
        return 1;
    }

    JoystickShutdownDevice();
    KeyboardShutdownDevice();
    MouseShutdownDevice();

    if (g_zInput_GlobalState != 0) {
        g_zInput_GlobalState->Release();
    }

    g_zInput_hWnd = 0;
    return 0;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-resetalltransitionstate
 * @recoil-artifact defines .text recoil:function:0x471c50: zInput::ResetAllTransitionState.
 * @recoil-match byte
 *
 * Purpose: reset keyboard, joystick, and mouse transition state as a single
 * zInput mode/focus transition operation.
 *
 * Evidence: BN assembly at 0x471c50 calls KeyboardResetTransitionState,
 * calls DIResetTransitionState, then tail-calls MouseResetTransitionState.
 */
void __cdecl ResetAllTransitionState() {
    KeyboardResetTransitionState();
    DIResetTransitionState();
    MouseResetTransitionState();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-mouse-isunsuspended
 * @recoil-artifact defines .text recoil:function:0x471c60: zInput::MouseIsUnsuspended.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: report whether the mouse suspend bit in the zInput device registry
 * is clear.
 */
int __cdecl MouseIsUnsuspended() {
    return IsUnsuspended(g_zInputMouseFlags);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-joystick-isunsuspended
 * @recoil-artifact defines .text recoil:function:0x471c70: zInput::JoystickIsUnsuspended.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: report whether the joystick suspend bit in the zInput device
 * registry is clear.
 */
int __cdecl JoystickIsUnsuspended() {
    return IsUnsuspended(g_zInputJoystickFlags);
}

}
/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-zinput-keyboard-isunsuspended
 * @recoil-artifact defines .text recoil:function:0x471c80: zInputKeyboardIsUnsuspended.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: report whether the keyboard device registry suspend bit is clear.
 *
 * Evidence: BN names the retail callee as zInputKeyboard::IsUnsuspended and
 * shows the same bit-1 clear test used by the mouse and joystick helpers.
 */
int __cdecl zInputKeyboardIsUnsuspended() {
    return (~g_zInput_DeviceRegistry & 2U) >> 1;
}
namespace zInput {

/**
 * Original inline helper evidence: bind-map overlay list head detach.
 * No standalone retail function exists; observed caller 0x471a20 inlines this
 * list-unlink pattern for both overlay node lists before operator delete.
 * Purpose: detach the current overlay list head while preserving the recovered
 * prev/next cleanup shape used by zInput global-state teardown.
 */
inline zInput_BindMapOverlayStackNode *__fastcall BindMapOverlayDetachHead(
    zInput_BindMapOverlayStackNode **head
) {
    zInput_BindMapOverlayStackNode *node = *head;
    if (node == 0) {
        return 0;
    }

    zInput_BindMapOverlayStackNode *const next = node->next;
    *head = next;
    if (next != 0) {
        next->prev = 0;
    }
    node->prev = 0;
    node->next = 0;
    return node;
}

/**
 * Original inline helper evidence: bind-map overlay node-list deletion.
 * No standalone retail function exists; observed caller 0x471a20 emits this
 * repeated detach/delete loop for the free-list and auxiliary block-list
 * fields.
 * Purpose: delete one recovered overlay node list in zInput global-state
 * teardown.
 */
inline void __fastcall BindMapOverlayDeleteNodeList(
    zInput_BindMapOverlayStackNode **head
) {
    zInput_BindMapOverlayStackNode *node = BindMapOverlayDetachHead(head);
    while (node != 0) {
        operator delete(node);
        node = BindMapOverlayDetachHead(head);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-mouse-resumefromsuspend
 * @recoil-artifact defines .text recoil:function:0x471c90: zInput::MouseResumeFromSuspend.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: reset mouse transition state if it was suspended, then clear the
 * mouse suspend bit.
 *
 * Evidence: reset-helper dependency is the address-backed
 * MouseResetTransitionState implementation at 0x470610.
 */
void __cdecl MouseResumeFromSuspend() {
    if ((g_zInputMouseFlags & kSuspendFlag) != 0) {
        MouseResetTransitionState();
    }

    g_zInputMouseFlags &= (unsigned char)(~kSuspendFlag);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-joystick-resumefromsuspend
 * @recoil-artifact defines .text recoil:function:0x471cb0: zInput::JoystickResumeFromSuspend.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: reset joystick transition state if it was suspended, then clear the
 * joystick suspend bit.
 *
 * Evidence: reset-helper dependency is the address-backed
 * DIResetTransitionState implementation at 0x472410.
 */
void __cdecl JoystickResumeFromSuspend() {
    if ((g_zInputJoystickFlags & kSuspendFlag) != 0) {
        DIResetTransitionState();
    }

    g_zInputJoystickFlags &= (unsigned char)(~kSuspendFlag);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-keyboard-resumefromsuspend
 * @recoil-artifact defines .text recoil:function:0x471cd0: zInput::KeyboardResumeFromSuspend.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: reset keyboard transition state if it was suspended, then clear the
 * keyboard suspend bit.
 *
 * Evidence: reset-helper dependency is the address-backed
 * KeyboardResetTransitionState implementation at 0x46f450.
 */
void __cdecl KeyboardResumeFromSuspend() {
    if ((g_zInput_DeviceRegistry & kSuspendFlag) != 0) {
        KeyboardResetTransitionState();
    }

    g_zInput_DeviceRegistry &= (unsigned char)(~kSuspendFlag);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-mouse-suspend
 * @recoil-artifact defines .text recoil:function:0x471cf0: zInput::MouseSuspend.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: set the mouse suspend bit in the zInput device registry.
 */
void __cdecl MouseSuspend() {
    g_zInputMouseFlags |= kSuspendFlag;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-joystick-suspend
 * @recoil-artifact defines .text recoil:function:0x471d00: zInput::JoystickSuspend.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: set the joystick suspend bit in the zInput device registry.
 */
void __cdecl JoystickSuspend() {
    g_zInputJoystickFlags |= kSuspendFlag;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-keyboard-suspend
 * @recoil-artifact defines .text recoil:function:0x471d10: zInput::KeyboardSuspend.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: set the keyboard suspend bit in the zInput device registry.
 */
void __cdecl KeyboardSuspend() {
    g_zInput_DeviceRegistry |= kSuspendFlag;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-keyboard-addref
 * @recoil-artifact defines .text recoil:function:0x471d20: zInput::KeyboardAddRef.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Increment the keyboard polling reference count and reset transition
 * state when the first active reference is acquired.
 */
int __cdecl KeyboardAddRef() {
    if ((g_zInput_DeviceRegistry & 1) != 0) {
        if (g_zInputKeyboardPollRefCount == 0) {
            KeyboardResetTransitionState();
        }
        ++g_zInputKeyboardPollRefCount;
    }

    return (unsigned short)(g_zInputKeyboardPollRefCount);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-di-addjoystickref
 * @recoil-artifact defines .text recoil:function:0x471d50: zInput::DIAddJoystickRef.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Increment the joystick polling reference count and reset transition
 * state when the first active reference is acquired.
 */
int __cdecl DIAddJoystickRef() {
    if ((g_zInputJoystickFlags & 1) != 0) {
        if (g_zInputJoystickPollRefCount == 0) {
            DIResetTransitionState();
        }
        ++g_zInputJoystickPollRefCount;
    }

    return g_zInputJoystickPollRefCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-di-releasejoystickref
 * @recoil-artifact defines .text recoil:function:0x471d80: zInput::DIReleaseJoystickRef.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Decrement the joystick polling reference count without underflow.
 */
int __cdecl DIReleaseJoystickRef() {
    short refCount = g_zInputJoystickPollRefCount;
    if ((unsigned short)(refCount) > 0) {
        --refCount;
    }

    g_zInputJoystickPollRefCount = refCount;
    return refCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-mouse-addref
 * @recoil-artifact defines .text recoil:function:0x471da0: zInput::MouseAddRef.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Increment the mouse polling reference count and reset transition
 * state when the first active reference is acquired.
 */
int __cdecl MouseAddRef() {
    if ((g_zInputMouseFlags & 1) != 0) {
        if (g_zInputMousePollRefCount == 0) {
            MouseResetTransitionState();
        }
        ++g_zInputMousePollRefCount;
    }

    return (unsigned short)(g_zInputMousePollRefCount);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-di-getjoystickrefcount
 * @recoil-artifact defines .text recoil:function:0x471dd0: zInput::DIGetJoystickRefCount.
 * Retail literal-backed physical source block: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Return the current joystick polling reference count.
 */
int __cdecl DIGetJoystickRefCount() {
    return g_zInputJoystickPollRefCount;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil-zinput-zin-init-pollactivedevices
 * @recoil-artifact defines .text recoil:function:0x471de0: zInput::PollActiveDevices.
 * Purpose: Poll enabled mouse, joystick, and keyboard devices with the caller's dispatch mode.
 */
void __fastcall PollActiveDevices(
    unsigned char dispatchCallbacks
) {
    const unsigned char savedDispatchCallbacks = dispatchCallbacks;
    if (g_zInputMouseFlags == 1 && (unsigned short)(g_zInputMousePollRefCount) > 0) {
        MousePollAndStoreState(savedDispatchCallbacks);
    }

    if (g_zInputJoystickFlags == 1 && (unsigned short)(g_zInputJoystickPollRefCount) > 0) {
        DIPollJoystickState(savedDispatchCallbacks);
    }

    if (g_zInput_DeviceRegistry == 1 && (unsigned short)(g_zInputKeyboardPollRefCount) > 0) {
        KeyboardPollState(savedDispatchCallbacks);
    }
}

} // namespace zInput
