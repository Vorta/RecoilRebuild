#include "GameZRecoil/zHud/zhud_ui.h"
#include "GameZRecoil/zInput/zInput.h"

#include <cstdint>
#include <cstring>
#include <new>

namespace {
int g_hudCmdDialogStateDeleteCount;
unsigned int g_hudCmdDialogStateDeleteFlags;
int g_hudCmdDialogConstructorProbeCalls;

struct CodeFunctionPatch {
    unsigned char *address;
    unsigned char original[5];
};

struct HudCmdDialogStateFakeDialog {
    void **vftable;

    HudUiBackground * ScalarDeletingDestructor(unsigned int flags) {
        ++g_hudCmdDialogStateDeleteCount;
        g_hudCmdDialogStateDeleteFlags = flags;
        return 0;
    }
};

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

    void *dialogVtbl[3] = {
        0,
        0,
        MethodAddress(&HudCmdDialogStateFakeDialog::ScalarDeletingDestructor)
    };
    HudCmdDialogStateFakeDialog dialog = {dialogVtbl};
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
    HudCmdDialog *const dialog = state->m_dialog;
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
