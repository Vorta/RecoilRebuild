/* This source-layout fragment is included by the current compatibility container.
 * Parent build/manifests must compile this path directly after retiring the container include.
 */

/**
 * Reimplements 0x4719e0: zInput::GlobalStateStaticInitAndRegisterAtExit.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly calls zInput::GlobalStateStaticInit and tail-jumps to
 * zInput::GlobalStateRegisterAtExit.
 * Purpose: perform zInput global-state static construction and register its
 * CRT shutdown callback.
 */
int GlobalStateStaticInitAndRegisterAtExit() {
    GlobalStateStaticInit();
    return GlobalStateRegisterAtExit();
}

#if defined(_MSC_VER) && defined(_M_IX86)
typedef void (__cdecl *ZInputCrtInitializerFn)();
/* VC5 emits these zInput startup callbacks as direct .CRT$XCU rows. */
#pragma data_seg(".CRT$XCU")
ZInputCrtInitializerFn s_zInputCrtInit_BindGroupList =
    (ZInputCrtInitializerFn)BindGroupList_StaticInitAndRegisterAtExit;
ZInputCrtInitializerFn s_zInputCrtInit_GlobalState =
    (ZInputCrtInitializerFn)GlobalStateStaticInitAndRegisterAtExit;
#pragma data_seg()
#endif

/**
 * Reimplements 0x4719f0: zInput::GlobalStateStaticInit.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly loads 0x561cb0 as the static-object this pointer and tail-jumps
 * to zInput_GlobalState::Constructor.
 * Purpose: run zInput global-state static construction.
 */
void *GlobalStateStaticInit() {
    return GlobalStateConstructor(&g_zInput_GlobalStateStorage);
}

/**
 * Reimplements 0x471a00: zInput::GlobalStateRegisterAtExit.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly pushes zInput::GlobalStateAtExitDestructor and calls the CRT
 * atexit provider.
 * Purpose: register the zInput global-state static destructor.
 */
int GlobalStateRegisterAtExit() {
    return atexit(GlobalStateAtExitDestructor);
}

/**
 * Reimplements 0x471a10: zInput::GlobalStateAtExitDestructor.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly loads 0x561cb0 as the static-object this pointer and tail-jumps
 * to zInput_GlobalState::Destructor.
 * Purpose: expose the zInput global-state destructor as a CRT atexit callback.
 */
void GlobalStateAtExitDestructor() {
    GlobalStateDestructor(&g_zInput_GlobalStateStorage);
}

/**
 * Reimplements 0x471a20: zInput_GlobalState::Destructor.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * BN assembly drains the overlay free-list, drains the auxiliary block-list,
 * clears both list heads plus stack head/reserved/depth, and leaves the block
 * size field intact.
 * Purpose: tear down the zInput bind-map overlay static lifetime state.
 */
void __fastcall GlobalStateDestructor(
    zInput_GlobalState *self
) {
    BindMapOverlay_DeleteNodeList(&self->bindMapOverlayNodeFreeList);
    BindMapOverlay_DeleteNodeList(&self->bindMapOverlayNodeBlockList);
    self->bindMapOverlayNodeBlockList = 0;
    self->bindMapOverlayNodeFreeList = 0;
    self->bindMapOverlayReserved = 0;
    self->bindMapOverlayNodeStackHead = 0;
    self->bindMapOverlayDepth = 0;
}

/**
 * Reimplements 0x471ab0: zInput_GlobalState::Constructor.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
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
 * Reimplements 0x471ae0: zInput::OnAppDeactivate.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: suspend active input devices during app deactivation, then mark the
 * mouse inactive and update DirectInput acquisition state.
 *
 * Evidence: BN assembly calls the joystick, mouse, and keyboard unsuspended
 * tests in that order, sets only the needed suspend bits, stores 0 to
 * g_zInput_MouseActive, and tail-jumps to Mouse_UpdateAcquireState.
 */
void OnAppDeactivate() {
    if (Joystick_IsUnsuspended() != 0) {
        Joystick_Suspend();
    }

    if (Mouse_IsUnsuspended() != 0) {
        Mouse_Suspend();
    }

    if (zInput_Keyboard_IsUnsuspended() != 0) {
        Keyboard_Suspend();
    }

    g_zInput_MouseActive = 0;
    Mouse_UpdateAcquireState();
}
} // namespace zInput
/**
 * Reimplements 0x471b20: zInput::OnAppActivate.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: resume suspended input devices when a window is active, then mark
 * the mouse active and update DirectInput acquisition state.
 *
 * Evidence: BN assembly gates on g_zInput_hWnd, resumes joystick, keyboard,
 * then mouse suspend state, stores 1 to g_zInput_MouseActive, and tail-jumps
 * to Mouse_UpdateAcquireState.
 */
void OnAppActivate() {
    if (g_zInput_hWnd == 0) {
        return;
    }

    Joystick_ResumeFromSuspend();
    Keyboard_ResumeFromSuspend();
    Mouse_ResumeFromSuspend();
    g_zInput_MouseActive = 1;
    Mouse_UpdateAcquireState();
}

/**
 * Reimplements 0x471b50: zInput::Init.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
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
        DI_ReportError(
            hr,
            g_zInput_SourceFile_ZinInitCpp,
            0x93
        );
        return -1;
    }

    g_zInput_hWnd = hWnd;
    g_zInput_DeviceRegistry = Keyboard_InitDevice() == 0 ? (unsigned char)(1) : 0;
    g_zInputMouseFlags = Mouse_InitDevice() != 0 ? (unsigned char)(1) : 0;
    g_zInputJoystickFlags = DI_InitJoystickDevice(hWnd) != 0 ? 1 : 0;
    Keyboard_AddRef();
    Mouse_AddRef();
    return 0;
}

/**
 * Reimplements 0x471c10: zInput::Shutdown.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: shut down joystick, keyboard, mouse, and DirectInput state, then
 * clear the input window handle.
 */
int Shutdown() {
    if (g_zInput_hWnd == 0) {
        return 1;
    }

    Joystick_ShutdownDevice();
    Keyboard_ShutdownDevice();
    Mouse_ShutdownDevice();

    if (g_zInput_GlobalState != 0) {
        g_zInput_GlobalState->Release();
    }

    g_zInput_hWnd = 0;
    return 0;
}

/**
 * Reimplements 0x471c50: zInput::ResetAllTransitionState.
 *
 * Purpose: reset keyboard, joystick, and mouse transition state as a single
 * zInput mode/focus transition operation.
 *
 * Evidence: BN assembly at 0x471c50 calls Keyboard_ResetTransitionState,
 * calls DI_ResetTransitionState, then tail-calls Mouse_ResetTransitionState.
 */
void ResetAllTransitionState() {
    Keyboard_ResetTransitionState();
    DI_ResetTransitionState();
    Mouse_ResetTransitionState();
}

/**
 * Reimplements 0x471c60: zInput::Mouse_IsUnsuspended.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: report whether the mouse suspend bit in the zInput device registry
 * is clear.
 */
int Mouse_IsUnsuspended() {
    return IsUnsuspended(g_zInputMouseFlags);
}

/**
 * Reimplements 0x471c70: zInput::Joystick_IsUnsuspended.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: report whether the joystick suspend bit in the zInput device
 * registry is clear.
 */
int Joystick_IsUnsuspended() {
    return IsUnsuspended(g_zInputJoystickFlags);
}

/**
 * Original-source helper evidence: zInput keyboard suspend flag query.
 * No standalone retail function exists for this namespace wrapper; activation
 * callers use the address-backed keyboard implementation at 0x471c80.
 * Purpose: expose the keyboard unsuspended query through the zInput namespace.
 */
int Keyboard_IsUnsuspended() {
    return zInput_Keyboard_IsUnsuspended();
}

/**
 * Reimplements 0x471c80: zInput_Keyboard_IsUnsuspended.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: report whether the keyboard device registry suspend bit is clear.
 *
 * Evidence: BN names the retail callee as zInputKeyboard::IsUnsuspended and
 * shows the same bit-1 clear test used by the mouse and joystick helpers.
 */
int zInput_Keyboard_IsUnsuspended() {
    return (~g_zInput_DeviceRegistry & 2U) >> 1;
}
}

namespace {
#if defined(_MSC_VER) && _MSC_VER < 1200
typedef std::vector<zInput_BindGroupInfo *> zInput_BindGroupInfoStdVector;
#endif

/**
 * Reimplements data 0x4e5ce0: k_EmptyString.
 * BN types this as a one-byte initialized empty string returned by bind-map
 * name lookup helpers when no key, joystick, or mouse label exists.
 * Purpose: Provides a stable empty C string for input binding names.
 */
char k_EmptyString[] = "";

/**
 * Original inline helper evidence: bind-map overlay list head detach.
 * No standalone retail function exists; observed caller 0x471a20 inlines this
 * list-unlink pattern for both overlay node lists before operator delete.
 * Purpose: detach the current overlay list head while preserving the recovered
 * prev/next cleanup shape used by zInput global-state teardown.
 */
zInput_BindMapOverlayStackNode *__fastcall BindMapOverlay_DetachHead(
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
void __fastcall BindMapOverlay_DeleteNodeList(
    zInput_BindMapOverlayStackNode **head
) {
    zInput_BindMapOverlayStackNode *node = BindMapOverlay_DetachHead(head);
    while (node != 0) {
        operator delete(node);
        node = BindMapOverlay_DetachHead(head);
    }
}

const int kZInputCommandLabelBytes = 0x50;

} // namespace

/**
 * Reimplements 0x471c90: zInput::Mouse_ResumeFromSuspend.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: reset mouse transition state if it was suspended, then clear the
 * mouse suspend bit.
 *
 * Evidence: reset-helper dependency is the address-backed
 * Mouse_ResetTransitionState implementation at 0x470610.
 */
void Mouse_ResumeFromSuspend() {
    if ((g_zInputMouseFlags & kSuspendFlag) != 0) {
        Mouse_ResetTransitionState();
    }

    g_zInputMouseFlags &= (unsigned char)(~kSuspendFlag);
}

/**
 * Reimplements 0x471cb0: zInput::Joystick_ResumeFromSuspend.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: reset joystick transition state if it was suspended, then clear the
 * joystick suspend bit.
 *
 * Evidence: reset-helper dependency is the address-backed
 * DI_ResetTransitionState implementation at 0x472410.
 */
void Joystick_ResumeFromSuspend() {
    if ((g_zInputJoystickFlags & kSuspendFlag) != 0) {
        DI_ResetTransitionState();
    }

    g_zInputJoystickFlags &= (unsigned char)(~kSuspendFlag);
}

/**
 * Reimplements 0x471cd0: zInput::Keyboard_ResumeFromSuspend.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: reset keyboard transition state if it was suspended, then clear the
 * keyboard suspend bit.
 *
 * Evidence: reset-helper dependency is the address-backed
 * Keyboard_ResetTransitionState implementation at 0x46f450.
 */
void Keyboard_ResumeFromSuspend() {
    if ((g_zInput_DeviceRegistry & kSuspendFlag) != 0) {
        Keyboard_ResetTransitionState();
    }

    g_zInput_DeviceRegistry &= (unsigned char)(~kSuspendFlag);
}

/**
 * Reimplements 0x471cf0: zInput::Mouse_Suspend.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: set the mouse suspend bit in the zInput device registry.
 */
void Mouse_Suspend() {
    g_zInputMouseFlags |= kSuspendFlag;
}

/**
 * Reimplements 0x471d00: zInput::Joystick_Suspend.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: set the joystick suspend bit in the zInput device registry.
 */
void Joystick_Suspend() {
    g_zInputJoystickFlags |= kSuspendFlag;
}

/**
 * Reimplements 0x471d10: zInput::Keyboard_Suspend.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: set the keyboard suspend bit in the zInput device registry.
 */
void Keyboard_Suspend() {
    g_zInput_DeviceRegistry |= kSuspendFlag;
}

/**
 * Reimplements 0x471d20: zInput::Keyboard_AddRef.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Increment the keyboard polling reference count and reset transition
 * state when the first active reference is acquired.
 */
int Keyboard_AddRef() {
    if ((g_zInput_DeviceRegistry & 1) != 0) {
        if (g_zInputKeyboardPollRefCount == 0) {
            Keyboard_ResetTransitionState();
        }
        ++g_zInputKeyboardPollRefCount;
    }

    return (unsigned short)(g_zInputKeyboardPollRefCount);
}

/**
 * Reimplements 0x471d50: zInput::DI_AddJoystickRef.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Increment the joystick polling reference count and reset transition
 * state when the first active reference is acquired.
 */
int DI_AddJoystickRef() {
    if ((g_zInputJoystickFlags & 1) != 0) {
        if (g_zInputJoystickPollRefCount == 0) {
            DI_ResetTransitionState();
        }
        ++g_zInputJoystickPollRefCount;
    }

    return g_zInputJoystickPollRefCount;
}

/**
 * Reimplements 0x471d80: zInput::DI_ReleaseJoystickRef.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Decrement the joystick polling reference count without underflow.
 */
int DI_ReleaseJoystickRef() {
    short refCount = g_zInputJoystickPollRefCount;
    if ((unsigned short)(refCount) > 0) {
        --refCount;
    }

    g_zInputJoystickPollRefCount = refCount;
    return refCount;
}

/**
 * Reimplements 0x471da0: zInput::Mouse_AddRef.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Increment the mouse polling reference count and reset transition
 * state when the first active reference is acquired.
 */
int Mouse_AddRef() {
    if ((g_zInputMouseFlags & 1) != 0) {
        if (g_zInputMousePollRefCount == 0) {
            Mouse_ResetTransitionState();
        }
        ++g_zInputMousePollRefCount;
    }

    return (unsigned short)(g_zInputMousePollRefCount);
}

/**
 * Reimplements 0x471dd0: zInput::DI_GetJoystickRefCount.
 * Original source path: D:\Proj\GameZRecoil\zInput\zin_init.cpp.
 * Purpose: Return the current joystick polling reference count.
 */
int DI_GetJoystickRefCount() {
    return g_zInputJoystickPollRefCount;
}

/**
 * Reimplements 0x471de0: zInput::PollActiveDevices.
 * Purpose: Poll enabled mouse, joystick, and keyboard devices with the caller's dispatch mode.
 */
void __fastcall PollActiveDevices(
    unsigned char dispatchCallbacks
) {
    const unsigned char savedDispatchCallbacks = dispatchCallbacks;
    if (g_zInputMouseFlags == 1 && (unsigned short)(g_zInputMousePollRefCount) > 0) {
        Mouse_PollAndStoreState(savedDispatchCallbacks);
    }

    if (g_zInputJoystickFlags == 1 && (unsigned short)(g_zInputJoystickPollRefCount) > 0) {
        DI_PollJoystickState(savedDispatchCallbacks);
    }

    if (g_zInput_DeviceRegistry == 1 && (unsigned short)(g_zInputKeyboardPollRefCount) > 0) {
        Keyboard_PollState(savedDispatchCallbacks);
    }
}

