#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zinput.h"
#include "GameZRecoil/zRender/zrndr.h"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <new>

namespace {
int g_hudCmdDialogStateSetEnabledCount;
int g_hudCmdDialogStateSetEnabledValue;
int g_hudCmdDialogStateDeleteCount;
unsigned int g_hudCmdDialogStateDeleteFlags;
int g_hudCmdDialogConstructorProbeCalls;
int g_hudCmdDialogStateBlitCount;
zVidImagePartial *g_hudCmdDialogStateBlitImage;

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

struct HudCmdDialogStateFakeDialog {
    virtual void Update(float) {}
    unsigned char reserved04[0x110];
    zVidImagePartial *capturedImage;

    virtual void SetEnabled(int enabled) {
        ++g_hudCmdDialogStateSetEnabledCount;
        g_hudCmdDialogStateSetEnabledValue = enabled;
    }

    virtual HudUiBackground * ScalarDeletingDestructor(unsigned int flags) {
        ++g_hudCmdDialogStateDeleteCount;
        g_hudCmdDialogStateDeleteFlags = flags;
        return (HudUiBackground *)this;
    }
};
RECOIL_STATIC_ASSERT(offsetof(HudCmdDialogStateFakeDialog, capturedImage) == 0x114);

template <typename Method> void *MethodAddress(Method method) {
    union MethodToFunction {
        Method method;
        void *function;
    };

    MethodToFunction thunk;
    thunk.method = method;
    return thunk.function;
}

bool PatchFunctionJump(
    void *target,
    void *replacement,
    CodeFunctionPatch &patch
) {
    patch.address = static_cast<unsigned char *>(target);
    std::memcpy(
        patch.original,
        patch.address,
        sizeof(patch.original)
    );

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) == 0) {
        patch.address = 0;
        return false;
    }

    patch.address[0] = 0xe9;
    const std::intptr_t relativeOffset =
        reinterpret_cast<std::intptr_t>(replacement) -
        reinterpret_cast<std::intptr_t>(patch.address + sizeof(patch.original));
    *reinterpret_cast<std::int32_t *>(patch.address + 1) =
        static_cast<std::int32_t>(relativeOffset);

    DWORD ignored = 0;
    VirtualProtect(
        patch.address,
        sizeof(patch.original),
        oldProtect,
        &ignored
    );
    FlushInstructionCache(
        GetCurrentProcess(),
        patch.address,
        sizeof(patch.original)
    );
    return true;
}

void RestoreFunctionPatch(CodeFunctionPatch &patch) {
    if (patch.address == 0) {
        return;
    }

    DWORD oldProtect = 0;
    if (VirtualProtect(
            patch.address,
            sizeof(patch.original),
            PAGE_EXECUTE_READWRITE,
            &oldProtect
        ) != 0) {
        std::memcpy(
            patch.address,
            patch.original,
            sizeof(patch.original)
        );
        FlushInstructionCache(
            GetCurrentProcess(),
            patch.address,
            sizeof(patch.original)
        );
        DWORD ignored = 0;
        VirtualProtect(
            patch.address,
            sizeof(patch.original),
            oldProtect,
            &ignored
        );
    }
    patch.address = 0;
}

struct HudCmdDialogConstructorProbe {
    HudCmdDialog * Constructor();
};

HudCmdDialog * HudCmdDialogConstructorProbe::Constructor() {
    ++g_hudCmdDialogConstructorProbeCalls;
    new ((HudUiBackground *)this) HudUiBackground;
    return (HudCmdDialog *)this;
}

void *HudCmdDialogConstructorAddress() {
    return MethodAddress(&HudCmdDialog::Constructor);
}

void *HudCmdDialogConstructorProbeAddress() {
    return MethodAddress(&HudCmdDialogConstructorProbe::Constructor);
}

void __fastcall TestHudCmdDialogStateBltSourceToPrimary(
    zVidImagePartial *image,
    int,
    int,
    int,
    zVidRect32 *
) {
    ++g_hudCmdDialogStateBlitCount;
    g_hudCmdDialogStateBlitImage = image;
}
} // namespace

extern "C" int zhud_cmd_dialog_state_lifecycle_smoke(void) {
    union StateStorage {
        void *align;
        unsigned char bytes[sizeof(HudCmdDialogState)];
    };

    StateStorage storage;
    HudCmdDialogState *state = new (storage.bytes) HudCmdDialogState();
    if (state->m_dialog != 0) {
        return 1;
    }

    state->~HudCmdDialogState();
    if (state->m_dialog != 0) {
        return 2;
    }

    HudCmdDialogStateFakeDialog dialog = {};
    g_hudCmdDialogStateDeleteCount = 0;
    g_hudCmdDialogStateDeleteFlags = 0;

    StateStorage dialogStateStorage;
    HudCmdDialogState *stateWithDialog =
        new (dialogStateStorage.bytes) HudCmdDialogState();
    stateWithDialog->m_dialog = (HudCmdDialog *)&dialog;
    stateWithDialog->~HudCmdDialogState();
    if (stateWithDialog->m_dialog != 0 ||
        g_hudCmdDialogStateDeleteCount != 1 ||
        g_hudCmdDialogStateDeleteFlags != 1) {
        return 3;
    }

    return 0;
}

extern "C" int zhud_cmd_dialog_state_on_try_become_current_smoke(void) {
    CodeFunctionPatch constructorPatch = {};
    if (!PatchFunctionJump(
            HudCmdDialogConstructorAddress(),
            HudCmdDialogConstructorProbeAddress(),
            constructorPatch
        )) {
        return 2;
    }

    const unsigned char oldKeyboardSuspend = g_zInput_KeyboardSuspendFlags;
    g_zInput_KeyboardSuspendFlags = (unsigned char)(oldKeyboardSuspend & ~2u);
    g_hudCmdDialogConstructorProbeCalls = 0;

    union StateStorage {
        void *align;
        unsigned char bytes[sizeof(HudCmdDialogState)];
    };
    StateStorage storage = {};
    HudCmdDialogState *state = new (storage.bytes) HudCmdDialogState();
    const int became = state->OnTryBecomeCurrent();
    HudCmdDialog *const dialog = (HudCmdDialog *)state->m_dialog;
    const bool becameCurrent =
        became == 1 &&
        g_hudCmdDialogConstructorProbeCalls == 1 &&
        dialog != 0 &&
        dialog->enabled == 1 &&
        *reinterpret_cast<void **>(dialog) != 0 &&
        (g_zInput_KeyboardSuspendFlags & 2u) != 0;

    state->m_dialog = 0;
    state->~HudCmdDialogState();
    if (dialog != 0) {
        ((HudUiBackground *)dialog)->ScalarDeletingDestructor(1);
    }

    g_zInput_KeyboardSuspendFlags = oldKeyboardSuspend;
    RestoreFunctionPatch(constructorPatch);
    return becameCurrent ? 0 : 1;
}

extern "C" int zhud_cmd_dialog_state_on_deactivate_smoke(void) {
    const unsigned char oldKeyboardSuspend = g_zInput_KeyboardSuspendFlags;
    zInput_BindMapContext *const oldCurrent = g_zInput_BindMap_Current;
    const int oldKeyboardSystemReady = g_zInput_KbdSystemReady;
    zVideo_BltSourceToPrimaryProc const oldBlit = g_zVideo_pfnBltSourceToPrimary;
    zInput::KbdKeyDispatchEntry oldDispatch[0x7de];
    std::memcpy(
        oldDispatch,
        g_zInputKbdKeyDispatchTable,
        sizeof(oldDispatch)
    );

    union StateStorage {
        void *align;
        unsigned char bytes[sizeof(HudCmdDialogState)];
    };

    StateStorage nullStorage = {};
    HudCmdDialogState *nullState = new (nullStorage.bytes) HudCmdDialogState();
    g_zInput_KbdSystemReady = 0;
    g_zInput_KeyboardSuspendFlags = 2;
    g_zInput_BindMap_Current = 0;
    nullState->OnDeactivate();
    const bool nullPath =
        nullState->m_dialog == 0 && (g_zInput_KeyboardSuspendFlags & 2u) == 0;
    nullState->~HudCmdDialogState();

    zVidImagePartial image = {};
    HudCmdDialogStateFakeDialog dialog = {};
    dialog.capturedImage = &image;

    zInput_BindMapContext context = {};
    int packedBindings[2] = {};
    zInputCommandCallbackFn callbacks[2] = {};
    char label0[0x50] = {};
    char label1[0x50] = {};
    char *labels[2] = {label0, label1};
    context.m_commandCount = 2;
    context.m_packedBindings = packedBindings;
    context.m_commandCallbacks = callbacks;
    context.m_commandLabels = labels;
    packedBindings[1] = zInput::BindMap_PackBindingCode(0x1e, 0x30, 2, 1);
    context.m_primaryKeyToCommand[0x1e] = 77;
    context.m_secondaryKeyToCommand[0x30] = 88;
    context.m_joystickToCommand[2] = 99;
    context.m_mouseToCommand[1] = 100;

    g_hudCmdDialogStateSetEnabledCount = 0;
    g_hudCmdDialogStateSetEnabledValue = -1;
    g_hudCmdDialogStateDeleteCount = 0;
    g_hudCmdDialogStateDeleteFlags = 0;
    g_hudCmdDialogStateBlitCount = 0;
    g_hudCmdDialogStateBlitImage = 0;
    g_zInput_KbdSystemReady = 0;
    g_zInput_KeyboardSuspendFlags = 2;
    g_zInput_BindMap_Current = &context;
    g_zVideo_pfnBltSourceToPrimary = TestHudCmdDialogStateBltSourceToPrimary;

    StateStorage storage = {};
    HudCmdDialogState *state = new (storage.bytes) HudCmdDialogState();
    state->m_dialog = (HudCmdDialog *)&dialog;
    state->OnDeactivate();
    const bool dialogPath =
        state->m_dialog == 0 &&
        (g_zInput_KeyboardSuspendFlags & 2u) == 0 &&
        g_hudCmdDialogStateSetEnabledCount == 1 &&
        g_hudCmdDialogStateSetEnabledValue == 0 &&
        g_hudCmdDialogStateBlitCount == 1 &&
        g_hudCmdDialogStateBlitImage == &image &&
        g_hudCmdDialogStateDeleteCount == 1 &&
        g_hudCmdDialogStateDeleteFlags == 1 &&
        context.m_primaryKeyToCommand[0x1e] == 1 &&
        context.m_secondaryKeyToCommand[0x30] == 1 &&
        context.m_joystickToCommand[2] == 1 &&
        context.m_mouseToCommand[1] == 1;

    state->~HudCmdDialogState();
    g_zInput_KeyboardSuspendFlags = oldKeyboardSuspend;
    g_zInput_BindMap_Current = oldCurrent;
    g_zInput_KbdSystemReady = oldKeyboardSystemReady;
    g_zVideo_pfnBltSourceToPrimary = oldBlit;
    std::memcpy(
        g_zInputKbdKeyDispatchTable,
        oldDispatch,
        sizeof(oldDispatch)
    );

    if (!nullPath) {
        return 1;
    }
    if (!dialogPath) {
        return 2;
    }
    return 0;
}
