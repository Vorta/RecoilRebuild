#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zInput/zInput.h"
#include "GameZRecoil/zLoc/zLoc.h"

#include <cstdlib>
#include <cstring>
#include <new>

namespace {
void RECOIL_CDECL BindMapContextDummyCallback() {}

bool CursorCanRoundTripTo(const POINT &target, const POINT &restore) {
    if (SetCursorPos(target.x, target.y) == 0) {
        return false;
    }

    POINT observed = {};
    if (GetCursorPos(&observed) == 0) {
        return false;
    }

    const bool ok = observed.x == target.x && observed.y == target.y;
    SetCursorPos(restore.x, restore.y);
    return ok;
}

int g_bindMapDispatchCallbackCount = 0;
int g_keyboardComboCallbackCount = 0;
int g_keyboardLastCombo = 0;
int g_keyboardRawCallbackCount = 0;
int g_keyboardLastRawAscii = 0;
void *g_keyboardLastRawContext = nullptr;

void RECOIL_CDECL BindMapContextCountingCallback() {
    ++g_bindMapDispatchCallbackCount;
}

void RECOIL_FASTCALL KeyboardComboCountingCallback(std::int32_t comboIdx) {
    ++g_keyboardComboCallbackCount;
    g_keyboardLastCombo = comboIdx;
}

std::int32_t RECOIL_FASTCALL KeyboardRawCountingCallback(std::int32_t ascii, void *context) {
    ++g_keyboardRawCallbackCount;
    g_keyboardLastRawAscii = ascii;
    g_keyboardLastRawContext = context;
    return 0;
}

int g_ffStopCount = 0;
int g_ffStartCount = 0;
int g_ffSetParametersCount = 0;
std::uint32_t g_ffLastIterations = 0;
std::uint32_t g_ffLastStartFlags = 0;
std::uint32_t g_ffLastSetFlags = 0;
std::int32_t g_ffLastGain = 0;
std::uint32_t g_ffLastEffectFlags = 0;
std::uint32_t g_ffLastAxisCount = 0;
std::int32_t g_ffLastDirection[2] = {};

std::int32_t RECOIL_STDCALL TestEffectSetParameters(zInput_DiEffect *, const void *effect,
                                                    std::uint32_t flags) {
    ++g_ffSetParametersCount;
    g_ffLastSetFlags = flags;
    struct EffectDescLocal {
        std::uint32_t dwSize;
        std::uint32_t dwFlags;
        std::uint32_t dwDuration;
        std::uint32_t dwSamplePeriod;
        std::uint32_t dwGain;
        std::uint32_t dwTriggerButton;
        std::uint32_t dwTriggerRepeatInterval;
        std::uint32_t cAxes;
        std::uint32_t *rgdwAxes;
        std::int32_t *rglDirection;
        void *lpEnvelope;
        std::uint32_t cbTypeSpecificParams;
        void *lpvTypeSpecificParams;
    };
    const auto *desc = static_cast<const EffectDescLocal *>(effect);
    g_ffLastEffectFlags = desc->dwFlags;
    g_ffLastGain = static_cast<std::int32_t>(desc->dwGain);
    g_ffLastAxisCount = desc->cAxes;
    if (desc->rglDirection != nullptr) {
        g_ffLastDirection[0] = desc->rglDirection[0];
        g_ffLastDirection[1] = desc->rglDirection[1];
    } else {
        g_ffLastDirection[0] = 0;
        g_ffLastDirection[1] = 0;
    }
    return 0;
}

std::int32_t RECOIL_STDCALL TestEffectStart(zInput_DiEffect *, std::uint32_t iterations,
                                            std::uint32_t flags) {
    ++g_ffStartCount;
    g_ffLastIterations = iterations;
    g_ffLastStartFlags = flags;
    return 0;
}

std::int32_t RECOIL_STDCALL TestEffectStop(zInput_DiEffect *) {
    ++g_ffStopCount;
    return 0;
}

void FreeOptionList() {
    zOptionEntryPartial *entry = g_zGame_Options_OptionListHead;
    while (entry != nullptr) {
        zOptionEntryPartial *const next = entry->next;
        std::free(reinterpret_cast<void *>(entry->payloadOrBuffer));
        std::free(entry->name);
        std::free(entry);
        entry = next;
    }
    g_zGame_Options_OptionListHead = nullptr;
}

void FreeOverlayNodes() {
    zInput_BindMapOverlayStackNode *node = g_zInput_BindMapOverlayNodeFreeList;
    while (node != nullptr) {
        zInput_BindMapOverlayStackNode *const next = node->next;
        delete node;
        node = next;
    }
    g_zInput_BindMapOverlayNodeFreeList = nullptr;
    g_zInput_BindMapOverlayNodeStackHead = nullptr;
    g_zInput_BindMapOverlayDepth = 0;
}

void CallCollisionImpactEffect(zInput_FFEffectSet *effectSet, const zVec3 *source, float gain) {
    zInput_DI_PlayCollisionImpactEffect(effectSet, source, gain);
}

void CallDamageHitEffect(zInput_FFEffectSet *effectSet, const zVec3 *source, float gain) {
    zInput_DI_PlayDamageHitEffect(effectSet, source, gain);
}

void ResetMouseGlobals();
} // namespace

extern "C" int directinput_create_import_provider_smoke(void) {
    HMODULE dinputModule = LoadLibraryA("dinput.dll");
    if (dinputModule == 0) {
        return 1;
    }

    FARPROC createProc = GetProcAddress(dinputModule, "DirectInputCreateA");
    FreeLibrary(dinputModule);
    return createProc != 0 ? 0 : 2;
}

extern "C" int zinput_init_fastpath_smoke(void) {
    ResetMouseGlobals();
    g_zInput_hWnd = reinterpret_cast<HWND>(0x1234);
    g_zInput_GlobalState = nullptr;
    g_zInput_DeviceRegistry = 3;
    g_zInputKeyboardPollRefCount = 4;

    const int result = zInput::Init(reinterpret_cast<HWND>(0x5678),
                                    reinterpret_cast<HINSTANCE>(0x9abc));
    return result == 1 && g_zInput_hWnd == reinterpret_cast<HWND>(0x1234) &&
                   g_zInput_GlobalState == nullptr && g_zInput_DeviceRegistry == 3 &&
                   g_zInputKeyboardPollRefCount == 4
               ? 0
               : 1;
}

extern "C" int zinput_joystick_option_accessors_smoke(void) {
    std::int32_t joystick = 0;
    std::int32_t axisCount = 0;
    std::int32_t buttonCount = 0;
    ZOPT_INPUT_JOYSTICK = &joystick;
    ZOPT_JOYSTICK_NUM_AXES = &axisCount;
    ZOPT_JOYSTICK_NUM_BUTTONS = &buttonCount;

    zInp::SetJoystickOption(1);
    if (joystick != 1 || zInp::GetJoystickOption() != 1) {
        return 1;
    }

    zInp::SetJoystickAxesCountOption(3);
    zInp::SetJoystickButtonCountOption(7);
    if (axisCount != 3 || buttonCount != 7) {
        return 2;
    }

    g_zInput_JoystickCaps_ForceFeedback = 0;
    if (zInput_DI_HasForceFeedback() != 0 || zInput_DI_IsForceFeedbackEnabled() != 0) {
        return 3;
    }

    g_zInput_JoystickCaps_ForceFeedback = 1;
    if (zInput_DI_HasForceFeedback() != 1 || zInput_DI_IsForceFeedbackEnabled() != 1) {
        return 4;
    }

    joystick = 0;
    if (zInput_DI_IsForceFeedbackEnabled() != 0) {
        return 5;
    }
    joystick = 1;
    g_zInput_JoystickCaps_ForceFeedback = 0;

    ZOPT_INPUT_JOYSTICK = nullptr;
    zInp::SetJoystickOption(0);
    ZOPT_INPUT_JOYSTICK = &joystick;
    ZOPT_JOYSTICK_NUM_AXES = nullptr;
    ZOPT_JOYSTICK_NUM_BUTTONS = nullptr;
    return joystick == 1 ? 0 : 6;
}

extern "C" int zinput_force_feedback_effect_wrappers_smoke(void) {
    zInput_DiEffectVtable vtbl = {};
    vtbl.SetParameters_18 = TestEffectSetParameters;
    vtbl.Start_1c = TestEffectStart;
    vtbl.Stop_20 = TestEffectStop;
    zInput_DiEffect primary = {&vtbl};
    zInput_DiEffect alt = {&vtbl};
    zInput_FFEffectSet effects = {};

    g_ffStopCount = 0;
    g_ffStartCount = 0;
    zInput_DI_RestartPrimaryFireEffect(&effects);
    if (g_ffStopCount != 0 || g_ffStartCount != 0) {
        return 1;
    }

    effects.PrimaryFire = &primary;
    zInput_DI_RestartPrimaryFireEffect(&effects);
    if (g_ffStopCount != 1 || g_ffStartCount != 1 || g_ffLastIterations != 1 ||
        g_ffLastStartFlags != 0) {
        return 2;
    }

    effects.AltFire = &alt;
    g_ffStopCount = 0;
    g_ffStartCount = 0;
    g_ffSetParametersCount = 0;
    zInput_DI_PlayAltFireEffect(&effects, 1.5f);
    if (g_ffStopCount != 1 || g_ffSetParametersCount != 1 || g_ffStartCount != 1 ||
        g_ffLastSetFlags != 4 || g_ffLastGain != 10000) {
        return 3;
    }

    zInput_DI_PlayAltFireEffect(&effects, 0.1f);
    if (g_ffLastGain != 2500) {
        return 4;
    }

    effects.AltFire = nullptr;
    g_ffStopCount = 0;
    zInput_DI_PlayAltFireEffect(&effects, 0.5f);
    if (g_ffStopCount != 0) {
        return 5;
    }
    return 0;
}

extern "C" int zinput_bindgroup_accessors_smoke(void) {
    if (zInput::BindGroupList_GetCount() != 0) {
        return 1;
    }

    char title[] = "Driving";
    std::int32_t commandIds[] = {3, 7, 11};
    zInput_BindGroupInfo group = {};
    group.title = title;
    group.commandIdsBegin = commandIds;
    group.commandIdsEnd = commandIds + 3;
    group.commandIdsCapacity = commandIds + 3;

    zInput_BindGroupInfo *groups[] = {&group};
    zInput_BindGroupInfoVec vec = {};
    vec.begin = groups;
    vec.end = groups + 1;
    if (vec.Count() != 1) {
        return 4;
    }

    g_zInput_BindGroupInfoList.begin = groups;
    g_zInput_BindGroupInfoList.end = groups + 1;
    g_zInput_BindGroupInfoList.capacity = groups + 1;

    g_zInput_CommandLocIdTable[7] = 100;
    g_zLoc_MessagesDllHandle = nullptr;

    if (zInput::BindGroupList_GetCount() != 1 || zInput::BindGroupList_GetGroupTitle(0) != title ||
        zInput::BindGroupList_GetGroupCommandCount(0) != 3 ||
        zInput::BindGroupList_GetGroupCommandId(0, 1) != 7 ||
        zInput::BindMap_GetCommandLabel(7) != nullptr ||
        zInput::BindMap_GetCommandHint(7) != nullptr) {
        return 2;
    }

    group.commandIdsBegin = nullptr;
    group.commandIdsEnd = nullptr;
    group.commandIdsCapacity = nullptr;
    zInput::BindGroupList_AddCommandToGroup(0, 13);
    if (zInput::BindGroupList_GetGroupCommandCount(0) != 1 ||
        zInput::BindGroupList_GetGroupCommandId(0, 0) != 13) {
        return 6;
    }

    zInput::BindMapSystem_Init(16);
    g_zInput_CurrentBindGroupIndex = 0;
    zInput::BindMap_AddDefaultBinding(5, 200, 0x11, 0x12, 2, 3);
    if (zInput::BindGroupList_GetGroupCommandCount(0) != 2 ||
        zInput::BindGroupList_GetGroupCommandId(0, 1) != 5 ||
        g_zInput_CommandLocIdTable[5] != 200 ||
        g_zInput_BindMap_Current->GetPrimaryKeyboardKey(5) != 0x11 ||
        g_zInput_BindMap_Current->GetSecondaryKeyboardKey(5) != 0x12 ||
        g_zInput_BindMap_Current->GetJoystickButtonSlot(5) != 2 ||
        g_zInput_BindMap_Current->GetMouseButtonSlot(5) != 3) {
        return 7;
    }
    zInput::BindMapSystem_Shutdown();
    g_zInput_BindMap_Current = nullptr;
    if (g_zGame_Options_OptionListHead != nullptr) {
        g_zGame_Options_OptionListHead->payloadOrBuffer = 0;
    }
    FreeOptionList();
    ::operator delete(group.commandIdsBegin);
    group.commandIdsBegin = nullptr;
    group.commandIdsEnd = nullptr;
    group.commandIdsCapacity = nullptr;
    g_zInput_CommandLocIdTable[5] = 0;

    group.commandIdsBegin = nullptr;
    if (zInput::BindGroupList_GetGroupCommandCount(0) != 0) {
        return 3;
    }

    vec.begin = nullptr;
    if (vec.Count() != 0) {
        return 5;
    }

    g_zInput_BindGroupInfoList = {};
    const std::int32_t firstGroup = zInput::BindGroupList_AddGroup("Weapons");
    const std::int32_t secondGroup = zInput::BindGroupList_AddGroup("Movement");
    if (firstGroup != 0 || secondGroup != 1 || zInput::BindGroupList_GetCount() != 2 ||
        std::strcmp(zInput::BindGroupList_GetGroupTitle(1), "Movement") != 0) {
        return 8;
    }

    zInput::BindGroupList_Clear();
    if (zInput::BindGroupList_GetCount() != 0 || g_zInput_BindGroupInfoList.begin == nullptr) {
        return 9;
    }

    ::operator delete(g_zInput_BindGroupInfoList.begin);
    g_zInput_BindGroupInfoList = {};
    g_zInput_CommandLocIdTable[7] = 0;

    zInput::BindMapSystem_Init(0x30);
    if (zInput::BindMap_InitDefaultBindings() != 1 || zInput::BindGroupList_GetCount() != 5 ||
        zInput::BindGroupList_GetGroupCommandCount(0) != 15 ||
        zInput::BindGroupList_GetGroupCommandCount(1) != 12 ||
        zInput::BindGroupList_GetGroupCommandCount(4) != 7 ||
        zInput::BindGroupList_GetGroupCommandId(0, 0) != 4 ||
        zInput::BindGroupList_GetGroupCommandId(4, 6) != 0x23 ||
        g_zInput_CommandLocIdTable[4] != 0x806 || g_zInput_CommandLocIdTable[0x2d] != 0x8b6 ||
        g_zInput_BindMap_Current->GetPrimaryKeyboardKey(0x24) != 0x418 ||
        g_zInput_BindMap_Current->GetPrimaryKeyboardKey(0x1f) != 0x22) {
        return 10;
    }

    zInput::BindGroupList_Clear();
    ::operator delete(g_zInput_BindGroupInfoList.begin);
    g_zInput_BindGroupInfoList = {};
    zInput::BindMapSystem_Shutdown();
    g_zInput_BindMap_Current = nullptr;
    if (g_zGame_Options_OptionListHead != nullptr) {
        g_zGame_Options_OptionListHead->payloadOrBuffer = 0;
    }
    FreeOptionList();
    return 0;
}

extern "C" int zinput_keyboard_dik_ascii_smoke(void) {
    std::memset(g_zInput_KbdDikToAsciiTable, 0x5a, sizeof(g_zInput_KbdDikToAsciiTable));
    g_zInput_KbdDikToAsciiTableReady = 0;

    if (zInput::Keyboard_TranslateDikToAscii(0x1e) != 'a' ||
        g_zInput_KbdDikToAsciiTableReady != 1 ||
        zInput::Keyboard_TranslateDikToAscii(0x41e) != 'A' ||
        zInput::Keyboard_TranslateDikToAscii(0x402) != '!' ||
        zInput::Keyboard_TranslateDikToAscii(0x40d) != '+' ||
        zInput::Keyboard_TranslateDikToAscii(0x42b) != '|' ||
        zInput::Keyboard_TranslateDikToAscii(0x435) != '?' ||
        zInput::Keyboard_TranslateDikToAscii(0x602) != '1' ||
        zInput::Keyboard_TranslateDikToAscii(0x0e) != 0x08 ||
        zInput::Keyboard_TranslateDikToAscii(0xd3) != 0x7f) {
        return 1;
    }

    g_zInput_KbdDikToAsciiTable[0x10] = 0x1234;
    zInput::Keyboard_InitDikToAsciiTable();
    return g_zInput_KbdDikToAsciiTable[0x10] == 'q' && g_zInput_KbdDikToAsciiTable[0x01] == 0 ? 0
                                                                                              : 2;
}

extern "C" int zinput_mouse_client_size_center_smoke(void) {
    zInput::Mouse_SetClientSizeAndCenter(640, 480);
    if (g_zInput_MouseClientWidth != 640 || g_zInput_MouseClientHeight != 480 ||
        g_zInput_MouseClientCenterX != 320 || g_zInput_MouseClientCenterY != 240 ||
        g_zInput_MouseInvClientCenterX < 0.003124f || g_zInput_MouseInvClientCenterX > 0.003126f ||
        g_zInput_MouseInvClientCenterY < 0.004166f || g_zInput_MouseInvClientCenterY > 0.004168f) {
        return 1;
    }

    zInput::Mouse_SetClientSizeAndCenter(641, 481);
    if (g_zInput_MouseClientCenterX != 320 || g_zInput_MouseClientCenterY != 240) {
        return 2;
    }

    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil", WS_POPUP, 0, 0, 321, 241, nullptr,
                                nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 5;
    }

    g_zInput_hWnd = hwnd;
    zInput::Mouse_UpdateClientRectAndCenter();
    DestroyWindow(hwnd);
    if (g_zInput_MouseClientWidth != 321 || g_zInput_MouseClientHeight != 241 ||
        g_zInput_MouseClientCenterX != 160 || g_zInput_MouseClientCenterY != 120) {
        return 6;
    }

    zInput::Mouse_SetClientSizeAndCenter(200, 100);
    zInput::Mouse_SetNormalizedCursorPos(2.0f, -1.5f);
    if (g_zInput_MouseStateSnapshot.cursorNormX != 1.0f ||
        g_zInput_MouseStateSnapshot.cursorNormY != -1.0f ||
        g_zInput_MouseStateSnapshot.cursorClientX != 200 ||
        g_zInput_MouseStateSnapshot.cursorClientY != 0) {
        return 3;
    }

    zInput::Mouse_SetNormalizedCursorPos(0.25f, 0.5f);
    return g_zInput_MouseStateSnapshot.cursorClientX == 125 &&
                   g_zInput_MouseStateSnapshot.cursorClientY == 75
               ? 0
               : 4;
}

extern "C" int zinput_mouse_apply_and_recenter_cursor_smoke(void) {
    POINT originalCursor = {};
    GetCursorPos(&originalCursor);

    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil", WS_POPUP, 30, 40, 200, 120, nullptr,
                                nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    g_zInput_hWnd = hwnd;
    g_zInput_MouseStateSnapshot.cursorClientX = 7;
    g_zInput_MouseStateSnapshot.cursorClientY = 9;
    POINT expected = {7, 9};
    ClientToScreen(hwnd, &expected);
    const bool osCursorObservable = CursorCanRoundTripTo(expected, originalCursor);
    zInput::Mouse_ApplyClientCursorPosToOS();

    POINT applied = {};
    if (osCursorObservable &&
        (GetCursorPos(&applied) == 0 || applied.x != expected.x || applied.y != expected.y)) {
        DestroyWindow(hwnd);
        SetCursorPos(originalCursor.x, originalCursor.y);
        return 2;
    }

    g_zInput_MouseClientCenterX = 40;
    g_zInput_MouseClientCenterY = 30;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.5f;
    g_zInput_MouseStateSnapshot.cursorNormY = -0.5f;
    expected = {40, 30};
    ClientToScreen(hwnd, &expected);
    zInput::Mouse_RecenterCursor();

    const bool recentered = g_zInput_MouseStateSnapshot.cursorClientX == 40 &&
                            g_zInput_MouseStateSnapshot.cursorClientY == 30 &&
                            g_zInput_MouseStateSnapshot.cursorNormX == 0.0f &&
                            g_zInput_MouseStateSnapshot.cursorNormY == 0.0f &&
                            (!osCursorObservable ||
                             (GetCursorPos(&applied) != 0 && applied.x == expected.x &&
                              applied.y == expected.y));

    g_zInput_MouseStateSnapshot.cursorClientX = 12;
    g_zInput_MouseStateSnapshot.cursorClientY = 17;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.75f;
    g_zInput_MouseStateSnapshot.cursorNormY = -0.25f;
    expected = {40, 17};
    ClientToScreen(hwnd, &expected);
    zInput::Mouse_RecenterCursorX();
    const bool recenteredX = g_zInput_MouseStateSnapshot.cursorClientX == 40 &&
                             g_zInput_MouseStateSnapshot.cursorClientY == 17 &&
                             g_zInput_MouseStateSnapshot.cursorNormX == 0.75f &&
                             g_zInput_MouseStateSnapshot.cursorNormY == -0.25f &&
                             (!osCursorObservable ||
                              (GetCursorPos(&applied) != 0 && applied.x == expected.x &&
                               applied.y == expected.y));

    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    return recentered && recenteredX ? 0 : 3;
}

extern "C" int zinput_mouse_coop_level_flags_smoke(void) {
    g_zInput_MouseCoopLevelFlags = 3;

    const std::int32_t previous = zInput::Mouse_SetCooperativeLevelFlags(5);
    if (previous != 3 || g_zInput_MouseCoopLevelFlags != 5) {
        return 1;
    }

    return zInput::Mouse_SetCooperativeLevelFlags(0) == 5 && g_zInput_MouseCoopLevelFlags == 0 ? 0
                                                                                               : 2;
}

extern "C" int zinput_mouse_button_transition_state_smoke(void) {
    g_zInput_MouseCurrentState = {};
    g_zInput_MousePreviousState = {};
    if (zInput::Mouse_GetButtonTransitionState(1) != 0) {
        return 1;
    }

    g_zInput_MouseCurrentState.rgbButtons = 0x80;
    g_zInput_MousePreviousState.rgbButtons = 0;
    if (zInput::Mouse_GetButtonTransitionState(1) != 1) {
        return 2;
    }

    g_zInput_MousePreviousState.rgbButtons = 0x80;
    if (zInput::Mouse_GetButtonTransitionState(1) != 2) {
        return 3;
    }

    g_zInput_MouseCurrentState.rgbButtons = 0;
    if (zInput::Mouse_GetButtonTransitionState(1) != 4) {
        return 4;
    }

    g_zInput_MouseCurrentState.rgbButtons = 0x800000;
    g_zInput_MousePreviousState.rgbButtons = 0;
    if (zInput::Mouse_GetButtonTransitionState(3) != 1) {
        return 5;
    }

    return 0;
}

extern "C" int zinput_mouse_apply_accumulated_delta_smoke(void) {
    auto nearFloat = [](float lhs, float rhs) {
        const float delta = lhs - rhs;
        return delta >= -0.0001f && delta <= 0.0001f;
    };

    g_zInput_MouseClientWidth = 100;
    g_zInput_MouseClientHeight = 80;
    g_zInput_MouseClientCenterX = 50;
    g_zInput_MouseClientCenterY = 40;
    g_zInput_MouseInvClientCenterX = 0.02f;
    g_zInput_MouseInvClientCenterY = 0.025f;
    g_zInput_MouseSensitivityX = 1.5f;
    g_zInput_MouseSensitivityY = 1.0f;
    g_zInput_MouseWrapModeFlag = 0;
    g_zInput_MouseStateSnapshot.cursorClientX = 50;
    g_zInput_MouseStateSnapshot.cursorClientY = 40;
    g_zInput_MouseStateSnapshot.deltaX = 20;
    g_zInput_MouseStateSnapshot.deltaY = -50;

    zInput::Mouse_ApplyAccumulatedDelta();
    if (g_zInput_MouseStateSnapshot.deltaX != 30 ||
        g_zInput_MouseStateSnapshot.deltaY != -50 ||
        g_zInput_MouseStateSnapshot.cursorClientX != 80 ||
        g_zInput_MouseStateSnapshot.cursorClientY != 0 ||
        !nearFloat(g_zInput_MouseStateSnapshot.cursorNormX, 0.6f) ||
        !nearFloat(g_zInput_MouseStateSnapshot.cursorNormY, -1.0f) ||
        !nearFloat(g_zInput_MouseStateSnapshot.deltaNormX, 0.6f) ||
        !nearFloat(g_zInput_MouseStateSnapshot.deltaNormY, -1.25f)) {
        return 1;
    }

    g_zInput_MouseSensitivityX = 1.0f;
    g_zInput_MouseSensitivityY = 1.0f;
    g_zInput_MouseWrapModeFlag = 1;
    g_zInput_MouseStateSnapshot.cursorClientX = 95;
    g_zInput_MouseStateSnapshot.cursorClientY = 2;
    g_zInput_MouseStateSnapshot.deltaX = 10;
    g_zInput_MouseStateSnapshot.deltaY = -5;

    zInput::Mouse_ApplyAccumulatedDelta();
    if (g_zInput_MouseStateSnapshot.cursorClientX != 105 ||
        g_zInput_MouseStateSnapshot.cursorClientY != -3 ||
        !nearFloat(g_zInput_MouseStateSnapshot.cursorNormX, 1.1f) ||
        !nearFloat(g_zInput_MouseStateSnapshot.cursorNormY, -1.075f) ||
        !nearFloat(g_zInput_MouseStateSnapshot.deltaNormX, 0.2f) ||
        !nearFloat(g_zInput_MouseStateSnapshot.deltaNormY, -0.125f)) {
        return 2;
    }

    return 0;
}

extern "C" int zinput_mouse_keyboard_small_accessors_smoke(void) {
    g_zInputKbdKeyDispatchTable[7].callback = reinterpret_cast<void *>(0x1234);
    zInput::Keyboard_UnregisterKeyCallback(7);
    if (g_zInputKbdKeyDispatchTable[7].callback != nullptr) {
        return 1;
    }

    g_zInput_MouseInitialized = 6;
    g_zInput_MouseStateSnapshot.cursorClientX = 11;
    g_zInput_MouseStateSnapshot.cursorClientY = 22;
    g_zInput_MouseStateSnapshot.button3Transition = 33;
    g_zInputMouseLastPollResult = 44;

    zInput::MouseStateSnapshot snapshot = {};
    if (zInput::Mouse_IsInitialized() != 6 ||
        zInput::Mouse_GetStateSnapshotPtr() != &g_zInput_MouseStateSnapshot ||
        zInput::Mouse_GetStateSnapshot(&snapshot) != 44 || snapshot.cursorClientX != 11 ||
        snapshot.cursorClientY != 22 || snapshot.button3Transition != 33 ||
        zInput::Mouse_GetStateSnapshot(nullptr) != 44) {
        return 2;
    }

    return 0;
}

extern "C" int zinput_bindmap_name_tables_smoke(void) {
    zInput::BindMap_InitDikKeyNameTable();
    zInput::BindMap_InitJoystickButtonNameTable();
    zInput::BindMap_InitMouseButtonNameTable();

    char keyName[0x40] = {};
    char joystickName[0x20] = {};
    char mouseName[0x20] = {};
    char *const formatted = zInput::BindMapCurrent_FormatKeyComboName(0x200 | 0x100 | 0x400 | 0x1e,
                                                                      keyName, sizeof(keyName));
    char *const joystickCopied =
        zInput::BindMapCurrent_CopyJoystickButtonName(4, joystickName, sizeof(joystickName));
    char *const mouseCopied =
        zInput::BindMapCurrent_CopyMouseButtonName(2, mouseName, sizeof(mouseName));

    return g_zInput_DikKeyNames[1] != nullptr &&
                   std::strcmp(g_zInput_DikKeyNames[1], "ESCAPE") == 0 &&
                   std::strcmp(g_zInput_DikKeyNames[0x0b], "0") == 0 &&
                   std::strcmp(g_zInput_DikKeyNames[0x1a], "LBRACKET") == 0 &&
                   std::strcmp(g_zInput_DikKeyNames[0x2b], "BACKSLASH") == 0 &&
                   std::strcmp(g_zInput_DikKeyNames[0x39], "SPACE") == 0 &&
                   std::strcmp(g_zInput_DikKeyNames[0x53], "DECIMAL") == 0 &&
                   std::strcmp(g_zInput_DikKeyNames[0x97], "UNLABELED") == 0 &&
                   std::strcmp(g_zInput_DikKeyNames[0xc8], "UP") == 0 &&
                   std::strcmp(g_zInput_DikKeyNames[0xdd], "APPS") == 0 &&
                   std::strcmp(g_zInput_JoystickButtonNames[8], "Button 8") == 0 &&
                   std::strcmp(g_zInput_MouseButtonNames[1], "Left") == 0 &&
                   std::strcmp(g_zInput_MouseButtonNames[3], "Middle") == 0 &&
                   formatted == keyName && std::strcmp(keyName, "Ctrl-Alt-Shift-A") == 0 &&
                   zInput::BindMap_FormatKeyComboName(0, keyName, sizeof(keyName)) != keyName &&
                   joystickCopied == joystickName && std::strcmp(joystickName, "Button 4") == 0 &&
                   mouseCopied == mouseName && std::strcmp(mouseName, "Right") == 0
               ? 0
               : 1;
}

extern "C" int zinput_bindmap_context_smoke(void) {
    FreeOptionList();
    zInput::Keyboard_ClearKeyCallbackTable();

    zInput_BindMapContext context = {};
    context.InitCommandMap(4);

    if (context.m_commandCount != 4 || context.m_packedBindings == nullptr ||
        context.m_commandCallbacks == nullptr || context.m_commandLabels == nullptr ||
        context.m_commandLabels[3] == nullptr || g_zGame_Options_OptionListHead == nullptr ||
        std::strcmp(g_zGame_Options_OptionListHead->name, "CmdMap") != 0 ||
        g_zGame_Options_OptionListHead->storageType != 7 ||
        g_zGame_Options_OptionListHead->dataSize != 16 ||
        g_zGame_Options_OptionListHead->registryScope != 1) {
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 1;
    }

    const std::int32_t packed = zInput::BindMap_PackBindingCode(0x12, 0x34, 5, 2);
    context.m_packedBindings[2] = packed;
    context.m_commandCallbacks[2] = BindMapContextDummyCallback;
    std::strcpy(context.m_commandLabels[2], "Command Two");
    context.RebuildLookupIndices();
    g_zInput_BindMap_Current = &context;

    if (context.GetPrimaryKeyboardKey(2) != 0x12 || context.GetSecondaryKeyboardKey(2) != 0x34 ||
        context.GetJoystickButtonSlot(2) != 5 || context.GetMouseButtonSlot(2) != 2 ||
        context.GetCommandByAnyKeyboardKey(0x34) != 2 || context.GetCommandByJoystickSlot(5) != 2 ||
        context.GetCommandByMouseSlot(2) != 2 ||
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(2) != 0x12 ||
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(2) != 0x34 ||
        zInput::BindMapCurrent_GetJoystickButtonSlot(2) != 5 ||
        zInput::BindMapCurrent_GetMouseButtonSlot(2) != 2 ||
        context.m_primaryKeyToCommand[0x12] != 2 || context.m_secondaryKeyToCommand[0x34] != 2 ||
        context.m_joystickToCommand[5] != 2 || context.m_mouseToCommand[2] != 2 ||
        g_zInputKbdKeyDispatchTable[0x12].callback == nullptr) {
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 2;
    }

    zInput::BindMap_Current_SetBindingRecord(1, "Command One", 0x20, 0x21, 3, 1);
    char labelBuffer[0x50] = {};
    if (zInput::BindMapCurrent_GetCommandByPrimaryKey(0x20) != 1 ||
        zInput::BindMapCurrent_GetCommandBySecondaryKey(0x21) != 1 ||
        zInput::BindMapCurrent_GetCommandByJoystickSlot(3) != 1 ||
        zInput::BindMapCurrent_GetCommandByMouseSlot(1) != 1 ||
        zInput::BindMapCurrent_CopyCommandLabel(1, labelBuffer, sizeof(labelBuffer)) !=
            labelBuffer ||
        std::strcmp(labelBuffer, "Command One") != 0) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 4;
    }

    zInput::BindMap_Current_SetBindingRecord(3, "Command Three", 0x40, 0x41, 0, 0);
    zInput::BindMapCurrent_SetPrimaryKeyBinding(0x42, 3);
    zInput::BindMapCurrent_SetSecondaryKeyBinding(0x43, 3);
    if (zInput::BindMapCurrent_GetCommandByPrimaryKey(0x40) != 0 ||
        zInput::BindMapCurrent_GetCommandBySecondaryKey(0x41) != 0 ||
        zInput::BindMapCurrent_GetCommandByPrimaryKey(0x42) != 3 ||
        zInput::BindMapCurrent_GetCommandBySecondaryKey(0x43) != 3 ||
        zInput::BindMapCurrent_GetPrimaryKeyboardKey(3) != 0x42 ||
        zInput::BindMapCurrent_GetSecondaryKeyboardKey(3) != 0x43) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 9;
    }

    g_zInputKbdKeyDispatchTable[0x20].state = 1;
    g_zInputKbdKeyDispatchTable[0x21].state = 4;
    std::memset(g_zInput_JoystickCurrentState.rgbButtons, 0,
                sizeof(g_zInput_JoystickCurrentState.rgbButtons));
    std::memset(g_zInput_JoystickPreviousState.rgbButtons, 0,
                sizeof(g_zInput_JoystickPreviousState.rgbButtons));
    g_zInput_JoystickCurrentState.rgbButtons[2] = 0x80;
    g_zInput_MouseStateSnapshot.button1Transition = 8;
    if (zInput::DI_GetCurrentState() != &g_zInput_JoystickCurrentState ||
        zInput::BindMap_Current_ReadCommandInputState(1) != 13 ||
        g_zInputKbdKeyDispatchTable[0x20].state != 2 ||
        g_zInputKbdKeyDispatchTable[0x21].state != 0 ||
        zInput::DI_GetButtonTransitionState(3) != 1 ||
        zInput::Mouse_GetButtonTransitionState(1) != 0) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 5;
    }

    g_bindMapDispatchCallbackCount = 0;
    if (zInput::BindMap_Current_SetCommandCallback(1, BindMapContextCountingCallback) != 1 ||
        context.m_commandCallbacks[1] != BindMapContextCountingCallback ||
        g_zInputKbdKeyDispatchTable[0x20].callback == nullptr ||
        g_zInputKbdKeyDispatchTable[0x21].callback == nullptr) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 8;
    }

    context.m_commandCallbacks[2] = BindMapContextCountingCallback;
    g_zInput_MouseStateSnapshot.button1Transition = 1;
    g_zInput_MouseStateSnapshot.button2Transition = 1;
    g_zInput_MouseStateSnapshot.button3Transition = 0;
    context.DispatchMouseButtonCallbacks();
    context.DispatchJoystickButtonCallbacks();
    zInput_BindMapContext_DispatchFromKeyboardEvent(0x20);
    context.m_commandCallbacks[2] = BindMapContextDummyCallback;
    if (g_bindMapDispatchCallbackCount != 4) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 7;
    }

    zInput_BindMapContext copy = {};
    copy.InitFromTemplate(&context);
    const bool copyOk = copy.m_commandCount == 4 &&
                        copy.m_packedBindings != context.m_packedBindings &&
                        copy.m_packedBindings[2] == context.m_packedBindings[2] &&
                        copy.m_commandCallbacks[2] == BindMapContextDummyCallback &&
                        copy.m_commandLabels[2] != context.m_commandLabels[2] &&
                        std::strcmp(copy.m_commandLabels[2], "Command Two") == 0 &&
                        copy.m_primaryKeyToCommand[0x12] == 2;

    copy.FreeAllBuffers();
    g_zInput_BindMap_Current = nullptr;
    context.FreeNonOwnedBuffers();
    FreeOptionList();

    zInput::BindMapSystem_Init(3);
    zInput_BindMapContext *baseContext = g_zInput_BindMap_Current;
    zInput::BindMap_Current_SetBindingRecord(1, "Base", 0x30, 0, 0, 0);
    zInput::BindMapContext_Push(nullptr);
    const bool pushOk =
        g_zInput_BindMap_Current != baseContext && g_zInput_BindMap_Current->m_isOverlay == 1 &&
        g_zInput_BindMapOverlayDepth == 1 && g_zInput_BindMapOverlayNodeStackHead != nullptr &&
        g_zInput_BindMapOverlayNodeStackHead->bindMap == baseContext &&
        g_zInput_BindMap_Current->GetPrimaryKeyboardKey(1) == 0x30;
    zInput::BindMapContext_Pop();
    const bool popOk = g_zInput_BindMap_Current == baseContext &&
                       g_zInput_BindMapOverlayDepth == 0 &&
                       g_zInput_BindMapOverlayNodeFreeList != nullptr &&
                       g_zInput_BindMap_Current->GetPrimaryKeyboardKey(1) == 0x30;
    zInput::BindMapSystem_Shutdown();
    g_zInput_BindMap_Current = nullptr;
    FreeOptionList();
    FreeOverlayNodes();

    if (!pushOk || !popOk) {
        return 6;
    }

    return copyOk ? 0 : 3;
}

extern "C" int zinput_bindmap_dispatch_mouse_callbacks_smoke(void) {
    FreeOptionList();

    zInput_BindMapContext context = {};
    context.InitCommandMap(4);
    context.m_mouseToCommand[1] = 1;
    context.m_mouseToCommand[2] = 2;
    context.m_mouseToCommand[3] = 3;
    context.m_commandCallbacks[1] = BindMapContextCountingCallback;
    context.m_commandCallbacks[2] = nullptr;
    context.m_commandCallbacks[3] = BindMapContextCountingCallback;

    g_bindMapDispatchCallbackCount = 0;
    g_zInput_MouseStateSnapshot.button1Transition = 1;
    g_zInput_MouseStateSnapshot.button2Transition = 1;
    g_zInput_MouseStateSnapshot.button3Transition = 1;
    context.DispatchMouseButtonCallbacks();
    if (g_bindMapDispatchCallbackCount != 2) {
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 1;
    }

    g_zInput_MouseStateSnapshot.button1Transition = 2;
    g_zInput_MouseStateSnapshot.button2Transition = 4;
    g_zInput_MouseStateSnapshot.button3Transition = 0;
    context.DispatchMouseButtonCallbacks();
    if (g_bindMapDispatchCallbackCount != 2) {
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 2;
    }

    context.FreeNonOwnedBuffers();
    FreeOptionList();
    return 0;
}

namespace {
struct MouseDeviceFake {
    zInput::DIDevice device;
};

struct DirectInputFake {
    zInput::DIDirectInput input;
};

int releaseCalls;
int directInputReleaseCalls;
int acquireCalls;
int unacquireCalls;
int pollCalls;
int getDeviceStateCalls;
int getDeviceDataCalls;
int queryInterfaceCalls;
int getCapabilitiesCalls;
int setDataFormatCalls;
int setCooperativeLevelCalls;
int createDeviceCalls;
int enumDevicesCalls;
int createEffectCalls;
int acquireResult;
int unacquireResult;
int getDeviceStateResult;
int getDeviceDataResult;
int createEffectResult;
int setDataFormatResult;
int setCooperativeLevelResult;
int createDeviceResult;
int setPropertyCalls;
int getPropertyCalls;
std::uint32_t lastCooperativeLevelFlags;
std::uint32_t lastSetPropertyId;
std::uint32_t lastSetPropertySize;
std::uint32_t lastSetPropertyObj;
std::uint32_t lastSetPropertyHow;
std::int32_t lastSetPropertyMin;
std::int32_t lastSetPropertyMax;
std::uint32_t lastSetPropertyData;
std::uint32_t lastGetPropertyId;
std::uint32_t lastGetPropertySize;
std::uint32_t lastGetPropertyObj;
std::uint32_t lastGetPropertyHow;
std::uint32_t getDeviceDataCount;
zInput::MouseDeviceState fakeMouseState;
zInput::JoystickStatePartial fakeJoystickState;
zInput::DIDevice *createDeviceOut;
zInput::DIDevice *queryInterfaceOut;
zInput_DiEffect *createEffectOut;
const GUID *lastCreateEffectGuid;
const void *lastCreateEffectDesc;
std::uint32_t lastEffectFlags;
std::uint32_t lastEffectDuration;
std::uint32_t lastEffectGain;
std::uint32_t lastEffectTypeParamBytes;
std::uint32_t lastEffectAxisCount;
std::uint32_t lastEffectAxes[2];
std::int32_t lastEffectDirections[2];
std::int32_t lastEffectTypeParams[4];
zInput::DIDeviceCaps fakeCaps;
zInput::DIDeviceObjectData keyboardEvents[4];
zInput::DIDeviceInstance enumDeviceInstance;

std::int32_t RECOIL_STDCALL QueryInterfaceFake(zInput::DIDevice *, const GUID *,
                                               zInput::DIDevice **outDevice) {
    ++queryInterfaceCalls;
    *outDevice = queryInterfaceOut;
    return 0;
}

std::int32_t RECOIL_STDCALL ReleaseFake(zInput::DIDevice *) {
    ++releaseCalls;
    return 1;
}

std::int32_t RECOIL_STDCALL ReleaseDirectInputFake(zInput::DIDirectInput *) {
    ++directInputReleaseCalls;
    return 1;
}

std::int32_t RECOIL_STDCALL AcquireFake(zInput::DIDevice *) {
    ++acquireCalls;
    return acquireResult;
}

std::int32_t RECOIL_STDCALL UnacquireFake(zInput::DIDevice *) {
    ++unacquireCalls;
    return unacquireResult;
}

std::int32_t RECOIL_STDCALL GetCapabilitiesFake(zInput::DIDevice *, void *capabilities) {
    ++getCapabilitiesCalls;
    *static_cast<zInput::DIDeviceCaps *>(capabilities) = fakeCaps;
    return 0;
}

std::int32_t RECOIL_STDCALL GetDeviceStateFake(zInput::DIDevice *, std::uint32_t cbData,
                                               void *outState) {
    ++getDeviceStateCalls;
    if (cbData != sizeof(zInput::MouseDeviceState)) {
        if (cbData != sizeof(zInput::JoystickStatePartial)) {
            return -3;
        }

        *static_cast<zInput::JoystickStatePartial *>(outState) = fakeJoystickState;
        return getDeviceStateResult;
    }

    *static_cast<zInput::MouseDeviceState *>(outState) = fakeMouseState;
    return getDeviceStateResult;
}

std::int32_t RECOIL_STDCALL GetDeviceDataFake(zInput::DIDevice *, std::uint32_t cbObjectData,
                                              zInput::DIDeviceObjectData *rgdod,
                                              std::uint32_t *pdwInOut, std::uint32_t flags) {
    ++getDeviceDataCalls;
    if (cbObjectData != sizeof(zInput::DIDeviceObjectData) || flags != 0) {
        return -3;
    }

    const std::uint32_t requested = *pdwInOut;
    const std::uint32_t copied = getDeviceDataCount < requested ? getDeviceDataCount : requested;
    for (std::uint32_t i = 0; i < copied; ++i) {
        rgdod[i] = keyboardEvents[i];
    }
    *pdwInOut = copied;
    return getDeviceDataResult;
}

std::int32_t RECOIL_STDCALL PollFake(zInput::DIDevice *) {
    ++pollCalls;
    return 0;
}

std::int32_t RECOIL_STDCALL CreateEffectFake(zInput::DIDevice *, const GUID *rguid,
                                             const void *effect, zInput_DiEffect **outEffect,
                                             void *) {
    ++createEffectCalls;
    lastCreateEffectGuid = rguid;
    lastCreateEffectDesc = effect;
    struct EffectDescLocal {
        std::uint32_t dwSize;
        std::uint32_t dwFlags;
        std::uint32_t dwDuration;
        std::uint32_t dwSamplePeriod;
        std::uint32_t dwGain;
        std::uint32_t dwTriggerButton;
        std::uint32_t dwTriggerRepeatInterval;
        std::uint32_t cAxes;
        const std::uint32_t *rgdwAxes;
        const std::int32_t *rglDirection;
        const void *lpEnvelope;
        std::uint32_t cbTypeSpecificParams;
        const void *lpvTypeSpecificParams;
    };
    const auto *desc = static_cast<const EffectDescLocal *>(effect);
    if (desc->dwSize == 0x34) {
        lastEffectFlags = desc->dwFlags;
        lastEffectDuration = desc->dwDuration;
        lastEffectGain = desc->dwGain;
        lastEffectTypeParamBytes = desc->cbTypeSpecificParams;
        lastEffectAxisCount = desc->cAxes;
        lastEffectAxes[0] = desc->rgdwAxes[0];
        lastEffectAxes[1] = desc->rgdwAxes[1];
        lastEffectDirections[0] = desc->rglDirection[0];
        lastEffectDirections[1] = desc->rglDirection[1];
        std::memset(lastEffectTypeParams, 0, sizeof(lastEffectTypeParams));
        std::memcpy(lastEffectTypeParams, desc->lpvTypeSpecificParams,
                    desc->cbTypeSpecificParams < sizeof(lastEffectTypeParams)
                        ? desc->cbTypeSpecificParams
                        : sizeof(lastEffectTypeParams));
    }
    *outEffect = createEffectOut;
    return createEffectResult;
}

std::int32_t RECOIL_STDCALL SetDataFormatFake(zInput::DIDevice *, const void *) {
    ++setDataFormatCalls;
    return setDataFormatResult;
}

std::int32_t RECOIL_STDCALL SetCooperativeLevelFake(zInput::DIDevice *, HWND, std::uint32_t flags) {
    ++setCooperativeLevelCalls;
    lastCooperativeLevelFlags = flags;
    return setCooperativeLevelResult;
}

std::int32_t RECOIL_STDCALL SetPropertyFake(zInput::DIDevice *, std::uint32_t propertyId,
                                            void *propHeader) {
    ++setPropertyCalls;
    struct PropDword {
        std::uint32_t dwSize;
        std::uint32_t dwHeaderSize;
        std::uint32_t dwObj;
        std::uint32_t dwHow;
        std::uint32_t dwData;
    };
    struct PropRange {
        std::uint32_t dwSize;
        std::uint32_t dwHeaderSize;
        std::uint32_t dwObj;
        std::uint32_t dwHow;
        std::int32_t lMin;
        std::int32_t lMax;
    };
    const auto *base = static_cast<const PropDword *>(propHeader);
    lastSetPropertyId = propertyId;
    lastSetPropertySize = base->dwSize;
    lastSetPropertyObj = base->dwObj;
    lastSetPropertyHow = base->dwHow;
    if (propertyId == 4) {
        const auto *range = static_cast<const PropRange *>(propHeader);
        lastSetPropertyMin = range->lMin;
        lastSetPropertyMax = range->lMax;
    } else if (propertyId == 5) {
        lastSetPropertyData = base->dwData;
    }
    return 0;
}

std::int32_t RECOIL_STDCALL CreateDeviceFake(zInput::DIDirectInput *, const GUID *,
                                             zInput::DIDevice **outDevice, void *) {
    ++createDeviceCalls;
    *outDevice = createDeviceOut;
    return createDeviceResult;
}

std::int32_t RECOIL_STDCALL EnumDevicesFake(zInput::DIDirectInput *, std::uint32_t deviceType,
                                            zInput::DIEnumDevicesCallback callback, void *ref,
                                            std::uint32_t flags) {
    ++enumDevicesCalls;
    if (deviceType != 4 || flags != 1) {
        return -1;
    }
    return callback(&enumDeviceInstance, ref);
}

std::int32_t RECOIL_STDCALL GetPropertyFake(zInput::DIDevice *, std::uint32_t propertyId,
                                            void *propHeader) {
    ++getPropertyCalls;
    struct RangeProp {
        std::uint32_t dwSize;
        std::uint32_t dwHeaderSize;
        std::uint32_t dwObj;
        std::uint32_t dwHow;
        std::int32_t lMin;
        std::int32_t lMax;
    };
    auto *range = static_cast<RangeProp *>(propHeader);
    lastGetPropertyId = propertyId;
    lastGetPropertySize = range->dwSize;
    lastGetPropertyObj = range->dwObj;
    lastGetPropertyHow = range->dwHow;
    range->lMin = -500;
    range->lMax = 500;
    return 0;
}

const zInput::DIDeviceVtable kMouseVtable = {
    QueryInterfaceFake,
    nullptr,
    ReleaseFake,
    GetCapabilitiesFake,
    nullptr,
    GetPropertyFake,
    SetPropertyFake,
    AcquireFake,
    UnacquireFake,
    GetDeviceStateFake,
    GetDeviceDataFake,
    SetDataFormatFake,
    nullptr,
    SetCooperativeLevelFake,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    CreateEffectFake,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    PollFake,
};

const zInput::DIDirectInputVtable kDirectInputVtable = {
    nullptr, nullptr, ReleaseDirectInputFake, CreateDeviceFake, EnumDevicesFake,
};

void ResetMouseGlobals() {
    releaseCalls = 0;
    directInputReleaseCalls = 0;
    acquireCalls = 0;
    unacquireCalls = 0;
    pollCalls = 0;
    getDeviceStateCalls = 0;
    getDeviceDataCalls = 0;
    queryInterfaceCalls = 0;
    getCapabilitiesCalls = 0;
    setDataFormatCalls = 0;
    setCooperativeLevelCalls = 0;
    setPropertyCalls = 0;
    getPropertyCalls = 0;
    createDeviceCalls = 0;
    enumDevicesCalls = 0;
    createEffectCalls = 0;
    acquireResult = 0;
    unacquireResult = 0;
    getDeviceStateResult = 0;
    getDeviceDataResult = 0;
    createEffectResult = 0;
    setDataFormatResult = 0;
    setCooperativeLevelResult = 0;
    createDeviceResult = 0;
    getDeviceDataCount = 0;
    fakeMouseState = {};
    fakeJoystickState = {};
    lastCooperativeLevelFlags = 0;
    lastSetPropertyId = 0;
    lastSetPropertySize = 0;
    lastSetPropertyObj = 0;
    lastSetPropertyHow = 0;
    lastSetPropertyMin = 0;
    lastSetPropertyMax = 0;
    lastSetPropertyData = 0;
    lastGetPropertyId = 0;
    lastGetPropertySize = 0;
    lastGetPropertyObj = 0;
    lastGetPropertyHow = 0;
    createDeviceOut = nullptr;
    queryInterfaceOut = nullptr;
    createEffectOut = nullptr;
    lastCreateEffectGuid = nullptr;
    lastCreateEffectDesc = nullptr;
    lastEffectFlags = 0;
    lastEffectDuration = 0;
    lastEffectGain = 0;
    lastEffectTypeParamBytes = 0;
    lastEffectAxisCount = 0;
    lastEffectAxes[0] = 0;
    lastEffectAxes[1] = 0;
    lastEffectDirections[0] = 0;
    lastEffectDirections[1] = 0;
    std::memset(lastEffectTypeParams, 0, sizeof(lastEffectTypeParams));
    g_ffStopCount = 0;
    g_ffStartCount = 0;
    g_ffSetParametersCount = 0;
    g_ffLastIterations = 0;
    g_ffLastStartFlags = 0;
    g_ffLastSetFlags = 0;
    g_ffLastGain = 0;
    g_ffLastEffectFlags = 0;
    g_ffLastAxisCount = 0;
    g_ffLastDirection[0] = 0;
    g_ffLastDirection[1] = 0;
    fakeCaps = {};
    enumDeviceInstance = {};
    g_zInput_GlobalState = nullptr;
    g_zInput_DeviceRegistry = 0;
    g_zInputKeyboardPollRefCount = 0;
    g_zInput_MouseActive = 0;
    g_zInput_MouseInitialized = 0;
    g_zInput_MouseDevice = nullptr;
    g_zInputMouseFlags = 0;
    g_zInputMousePollRefCount = 0;
    g_zInput_hWnd = nullptr;
    g_zInput_KbdSystemReady = 0;
    g_zInput_KbdDevice = nullptr;
    g_zInput_KbdEventBuffer = nullptr;
    g_zInput_KbdModifierState = 0;
    for (zInput::KbdKeyDispatchEntry &entry : g_zInputKbdKeyDispatchTable) {
        entry = {};
    }
    for (zInput::DIDeviceObjectData &event : keyboardEvents) {
        event = {};
    }
    g_zInput_KbdRawEventCallback = nullptr;
    g_zInput_KbdRawEventCallbackCtx = nullptr;
    g_zInput_JoystickCaps_ForceFeedback = 0;
    g_zInput_JoystickCaps_FFAttack = 0;
    g_zInput_JoystickCaps_FFFade = 0;
    g_zInput_JoystickInitialized = 0;
    g_zInput_JoystickDevice = nullptr;
    g_zInputJoystickFlags = 0;
    g_zInputJoystickPollRefCount = 0;
    g_zInput_JoystickAxisCount = 0;
    g_GameStateOrMapTable = nullptr;
    g_Player_DeltaTime = 0.0f;
    g_zInput_DiPitchAngleLowpassRad = 0.0f;
}
} // namespace

extern "C" int zinput_keyboard_clear_callbacks_smoke(void) {
    for (int i = 0; i < 0x7de; ++i) {
        g_zInputKbdKeyDispatchTable[i].state = i;
        g_zInputKbdKeyDispatchTable[i].callback = reinterpret_cast<void *>(0x1000 + i);
    }

    zInput::Keyboard_ClearKeyCallbackTable();
    for (int i = 0; i < 0x7de; ++i) {
        if (g_zInputKbdKeyDispatchTable[i].state != i ||
            g_zInputKbdKeyDispatchTable[i].callback != nullptr) {
            return 1;
        }
    }

    return 0;
}

extern "C" int zinput_keyboard_raw_callback_smoke(void) {
    void *callback = reinterpret_cast<void *>(0x1234);
    void *context = reinterpret_cast<void *>(0x5678);
    g_zInput_KbdRawEventCallback = nullptr;
    g_zInput_KbdRawEventCallbackCtx = nullptr;

    zInput::Keyboard_SetRawEventCallback(callback, context);
    if (g_zInput_KbdRawEventCallback != callback || g_zInput_KbdRawEventCallbackCtx != context) {
        return 1;
    }

    zInput::Keyboard_SetRawEventCallback(nullptr, nullptr);
    return g_zInput_KbdRawEventCallback == nullptr && g_zInput_KbdRawEventCallbackCtx == nullptr
               ? 0
               : 2;
}

extern "C" int zinput_keyboard_wait_for_key_press_smoke(void) {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_KbdDevice = &fake.device;
    g_zInput_KbdEventBuffer = keyboardEvents;

    getDeviceDataCount = 1;
    keyboardEvents[0] = {0x1e, 0x80, 0, 0};
    if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0x1e ||
        g_zInputKbdKeyDispatchTable[0x1e].state != 1) {
        return 1;
    }

    keyboardEvents[0] = {0x1e, 0, 0, 0};
    if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0 ||
        g_zInputKbdKeyDispatchTable[0x1e].state != 5) {
        return 2;
    }

    keyboardEvents[0] = {0x2a, 0x80, 0, 0};
    if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0 || g_zInput_KbdModifierState != 0x400 ||
        g_zInputKbdKeyDispatchTable[0x2a].state != 1) {
        return 3;
    }

    keyboardEvents[0] = {0x20, 0x80, 0, 0};
    if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0x420 || keyboardEvents[0].dwData != 0x90 ||
        g_zInputKbdKeyDispatchTable[0x420].state != 1) {
        return 4;
    }

    keyboardEvents[0] = {0x20, 0x80, 0, 0};
    if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0x420 ||
        g_zInputKbdKeyDispatchTable[0x420].state != 3) {
        return 5;
    }

    getDeviceDataResult = static_cast<std::int32_t>(0x8007001e);
    getDeviceDataCount = 0;
    if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0 || acquireCalls != 1) {
        return 6;
    }

    getDeviceDataResult = -7;
    if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0) {
        return 7;
    }

    getDeviceDataResult = 0;
    getDeviceDataCount = 1;
    keyboardEvents[0] = {0x21, 0x80, 0, 0};
    if (zInput_WaitForAnyKeyPressWithTimeoutMs(0) != 0 ||
        zInput_WaitForAnyKeyPressWithTimeoutMs(1000) != 1) {
        return 8;
    }

    getDeviceDataCount = 0;
    if (zInput_WaitForAnyKeyPressWithTimeoutMs(100) != 0) {
        return 9;
    }

    return 0;
}

extern "C" int zinput_keyboard_poll_state_smoke(void) {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_KbdDevice = &fake.device;
    g_zInput_KbdEventBuffer = keyboardEvents;
    g_zInput_KbdModifierState = 0;
    g_keyboardComboCallbackCount = 0;
    g_keyboardRawCallbackCount = 0;
    g_keyboardLastCombo = 0;
    g_keyboardLastRawAscii = 0;
    g_keyboardLastRawContext = nullptr;
    void *rawContext = reinterpret_cast<void *>(0x7654);
    zInput::Keyboard_SetRawEventCallback(reinterpret_cast<void *>(&KeyboardRawCountingCallback),
                                         rawContext);

    g_zInputKbdKeyDispatchTable[0x20].state = 1;
    g_zInputKbdKeyDispatchTable[0x420].callback =
        reinterpret_cast<void *>(&KeyboardComboCountingCallback);
    getDeviceDataCount = 2;
    keyboardEvents[0] = {0x2a, 0x80, 0, 0};
    keyboardEvents[1] = {0x20, 0x80, 0, 0};

    zInput::Keyboard_PollState(1);

    if (getDeviceDataCalls != 1 || g_zInput_KbdModifierState != 0x400 ||
        g_zInputKbdKeyDispatchTable[0x2a].state != 1 || keyboardEvents[1].dwData != 0x90) {
        return 1;
    }
    if (g_zInputKbdKeyDispatchTable[0x20].state != 4 ||
        g_zInputKbdKeyDispatchTable[0x420].state != 0 || g_keyboardComboCallbackCount != 1 ||
        g_keyboardLastCombo != 0x420) {
        return 2;
    }
    if (g_keyboardRawCallbackCount != 2 || g_keyboardLastRawAscii != 'D' ||
        g_keyboardLastRawContext != rawContext) {
        return 3;
    }

    keyboardEvents[0] = {0x20, 0, 0, 0};
    getDeviceDataCount = 1;
    zInput::Keyboard_PollState(0);
    if (g_zInputKbdKeyDispatchTable[0x420].state != 4 || g_keyboardComboCallbackCount != 1) {
        return 4;
    }

    getDeviceDataResult = static_cast<std::int32_t>(0x8007001e);
    getDeviceDataCount = 0;
    zInput::Keyboard_PollState(0);
    if (acquireCalls != 1) {
        return 5;
    }

    getDeviceDataResult = -9;
    zInput::Keyboard_PollState(0);
    return getDeviceDataCalls >= 4 ? 0 : 6;
}

extern "C" int zinput_keyboard_mouse_addref_smoke(void) {
    ResetMouseGlobals();
    g_zInput_DeviceRegistry = 1;
    g_zInputMouseFlags = 1;

    if (zInput::Keyboard_AddRef() != 1 || zInput::Keyboard_AddRef() != 2 ||
        g_zInputKeyboardPollRefCount != 2) {
        return 1;
    }

    if (zInput::Mouse_AddRef() != 1 || zInput::Mouse_AddRef() != 2 ||
        g_zInputMousePollRefCount != 2) {
        return 2;
    }

    g_zInput_DeviceRegistry = 0;
    g_zInputMouseFlags = 0;
    return zInput::Keyboard_AddRef() == 2 && zInput::Mouse_AddRef() == 2 ? 0 : 3;
}

extern "C" int zinput_force_feedback_create_effect_smoke(void) {
    ResetMouseGlobals();
    zInput_DiEffectVtable vtbl = {};
    GUID guid = {};
    int desc = 123;
    MouseDeviceFake joystick = {{&kMouseVtable}};
    zInput_DiEffect created = {&vtbl};
    createEffectOut = &created;

    if (zInput_DI_CreateForceFeedbackEffect(&guid, &desc) != nullptr) {
        return 1;
    }

    g_zInput_JoystickDevice = &joystick.device;
    if (zInput_DI_CreateForceFeedbackEffect(&guid, &desc) != &created || createEffectCalls != 1 ||
        lastCreateEffectGuid != &guid || lastCreateEffectDesc != &desc) {
        return 2;
    }

    createEffectResult = -1;
    if (zInput_DI_CreateForceFeedbackEffect(&guid, &desc) != nullptr) {
        return 3;
    }

    createEffectResult = 0;
    if (zInput_DI_CreateConstantForceEffectWithDirection(123) != &created ||
        lastEffectFlags != 0x22 || lastEffectDuration != 0xffffffff || lastEffectGain != 0 ||
        lastEffectAxisCount != 2 || lastEffectAxes[0] != 0 || lastEffectAxes[1] != 4 ||
        lastEffectDirections[0] != 123 || lastEffectDirections[1] != 0 ||
        lastEffectTypeParamBytes != 4 || lastEffectTypeParams[0] != 10000) {
        return 4;
    }

    if (zInput_DI_CreateConstantForceEffectScaled(1.5f) != &created ||
        lastEffectDuration != 100000 || lastEffectGain != 10000 || lastEffectTypeParamBytes != 4 ||
        lastEffectTypeParams[0] != 10000) {
        return 5;
    }

    zInput_DI_CreateConstantForceEffectScaled(-0.25f);
    if (lastEffectGain != 0) {
        return 6;
    }

    if (zInput_DI_CreateSineEffectScaled(0.5f) != &created || lastEffectDuration != 0xffffffff ||
        lastEffectGain != 10000 || lastEffectTypeParamBytes != 0x10 ||
        lastEffectTypeParams[0] != 5000 || lastEffectTypeParams[1] != 0 ||
        lastEffectTypeParams[2] != 0 || lastEffectTypeParams[3] != 20000) {
        return 7;
    }

    return 0;
}

extern "C" int zinput_force_feedback_effect_set_smoke(void) {
    ResetMouseGlobals();
    zInput_DiEffectVtable vtbl = {};
    vtbl.Start_1c = TestEffectStart;
    MouseDeviceFake joystick = {{&kMouseVtable}};
    zInput_DiEffect created = {&vtbl};
    zInput_FFEffectSet effects = {};
    g_zInput_JoystickDevice = &joystick.device;
    createEffectOut = &created;

    if (zInput_DI_InitForceFeedbackEffectSet(&effects) != &effects) {
        return 1;
    }
    if (createEffectCalls != 7 || g_ffStartCount != 2 || effects.PrimaryFire != &created ||
        effects.AltFire != &created || effects.AmbientSine != &created ||
        effects.CollisionImpact != &created || effects.DamageHit != &created ||
        effects.SteerForce != &created || effects.PitchForce != &created) {
        return 2;
    }

    return lastEffectDirections[0] == 0x4650 && lastEffectDirections[1] == 0 ? 0 : 3;
}

extern "C" int zinput_force_feedback_directional_runtime_smoke(void) {
    ResetMouseGlobals();
    zInput_DiEffectVtable vtbl = {};
    vtbl.SetParameters_18 = TestEffectSetParameters;
    vtbl.Start_1c = TestEffectStart;
    vtbl.Stop_20 = TestEffectStop;
    zInput_DiEffect collision = {&vtbl};
    zInput_DiEffect damage = {&vtbl};
    zInput_DiEffect steer = {&vtbl};
    zInput_DiEffect pitch = {&vtbl};
    zInput_FFEffectSet effects = {};
    zInput_PlayerStatePartial playerState = {};
    zInput_GameStateOrMapTablePartial gameState = {};
    zVec3 source = {-1.0f, 0.0f, 0.0f};

    playerState.cameraDirNextX = -1.0f;
    playerState.cameraDirNextZ = 0.0f;
    playerState.angVelYaw = -1.0f;
    playerState.yawVelocityLimit = 2.0f;
    playerState.pitchAngleRad = -0.1f;
    gameState.playerState = &playerState;
    g_GameStateOrMapTable = &gameState;

    effects.CollisionImpact = &collision;
    CallCollisionImpactEffect(&effects, &source, 0.1f);
    if (g_ffStopCount != 1 || g_ffSetParametersCount != 1 || g_ffStartCount != 1 ||
        g_ffLastSetFlags != 0x44 || g_ffLastEffectFlags != 0x20 || g_ffLastAxisCount != 2 ||
        g_ffLastDirection[0] != 18000 || g_ffLastDirection[1] != 0 || g_ffLastGain != 2000) {
        return 1;
    }

    g_ffStopCount = 0;
    g_ffStartCount = 0;
    g_ffSetParametersCount = 0;
    effects.CollisionImpact = nullptr;
    effects.DamageHit = &damage;
    source.x = 1.0f;
    CallDamageHitEffect(&effects, &source, 2.0f);
    if (g_ffStopCount != 1 || g_ffSetParametersCount != 1 || g_ffStartCount != 1 ||
        g_ffLastDirection[0] != 18000 || g_ffLastGain != 10000) {
        return 2;
    }

    g_ffStartCount = 0;
    g_ffSetParametersCount = 0;
    effects.DamageHit = nullptr;
    effects.SteerForce = &steer;
    effects.PitchForce = nullptr;
    zInput_DI_UpdateSteerAndPitchForceEffects(&effects);
    if (g_ffSetParametersCount != 1 || g_ffStartCount != 1 || g_ffLastDirection[0] != 0x5a ||
        g_ffLastGain != 5000) {
        return 3;
    }

    g_ffStartCount = 0;
    g_ffSetParametersCount = 0;
    effects.SteerForce = nullptr;
    effects.PitchForce = &pitch;
    g_Player_DeltaTime = 0.0f;
    g_zInput_DiPitchAngleLowpassRad = 0.0f;
    zInput_DI_UpdateSteerAndPitchForceEffects(&effects);
    if (g_ffSetParametersCount != 1 || g_ffStartCount != 1 || g_ffLastDirection[0] != 0 ||
        g_ffLastGain != 7500) {
        return 4;
    }

    g_GameStateOrMapTable = nullptr;
    return 0;
}

extern "C" int zinput_keyboard_init_device_smoke(void) {
    ResetMouseGlobals();
    DirectInputFake directInput{};
    MouseDeviceFake keyboard{};
    directInput.input.vtbl_00 = &kDirectInputVtable;
    keyboard.device.vtbl_00 = &kMouseVtable;
    g_zInput_GlobalState = &directInput.input;
    createDeviceOut = &keyboard.device;

    if (zInput::Keyboard_InitDevice() != 0) {
        return 1;
    }

    const bool ok = createDeviceCalls == 1 && setCooperativeLevelCalls == 1 &&
                    lastCooperativeLevelFlags == 0xa && setPropertyCalls == 1 &&
                    setDataFormatCalls == 1 && acquireCalls == 1 && g_zInput_KbdSystemReady == 1 &&
                    g_zInput_KbdDevice == &keyboard.device && g_zInput_KbdEventBuffer != nullptr;

    std::free(g_zInput_KbdEventBuffer);
    g_zInput_KbdEventBuffer = nullptr;
    return ok ? 0 : 2;
}

extern "C" int zinput_mouse_init_device_smoke(void) {
    ResetMouseGlobals();
    POINT originalCursor = {};
    GetCursorPos(&originalCursor);

    HWND hwnd = CreateWindowExA(0, "STATIC", "recoil", WS_POPUP, 50, 60, 320, 240, nullptr,
                                nullptr, GetModuleHandleA(nullptr), nullptr);
    if (hwnd == nullptr) {
        return 1;
    }

    DirectInputFake directInput{};
    MouseDeviceFake base{};
    MouseDeviceFake mouse{};
    directInput.input.vtbl_00 = &kDirectInputVtable;
    base.device.vtbl_00 = &kMouseVtable;
    mouse.device.vtbl_00 = &kMouseVtable;
    g_zInput_GlobalState = &directInput.input;
    g_zInput_hWnd = hwnd;
    g_zInput_MouseCoopLevelFlags = 5;
    g_zInput_MouseClientWidth = 0;
    createDeviceOut = &base.device;
    queryInterfaceOut = &mouse.device;

    const int result = zInput::Mouse_InitDevice();
    const bool ok = result == 1 && createDeviceCalls == 1 && queryInterfaceCalls == 1 &&
                    releaseCalls == 1 && setDataFormatCalls == 1 &&
                    setCooperativeLevelCalls == 1 && lastCooperativeLevelFlags == 5 &&
                    setPropertyCalls == 1 && acquireCalls == 1 &&
                    g_zInput_MouseInitialized == 1 && g_zInput_MouseActive == 1 &&
                    g_zInput_MouseDevice == &mouse.device && g_zInput_MouseClientWidth == 320 &&
                    g_zInput_MouseClientHeight == 240 &&
                    g_zInput_MouseStateSnapshot.cursorClientX == 160 &&
                    g_zInput_MouseStateSnapshot.cursorClientY == 120 &&
                    g_zInput_MouseStateSnapshot.cursorNormX == 0.0f &&
                    g_zInput_MouseStateSnapshot.cursorNormY == 0.0f;

    DestroyWindow(hwnd);
    SetCursorPos(originalCursor.x, originalCursor.y);
    return ok ? 0 : 2;
}

extern "C" int zinput_joystick_init_device_smoke(void) {
    ResetMouseGlobals();
    DirectInputFake directInput{};
    MouseDeviceFake base{};
    MouseDeviceFake joystick{};
    directInput.input.vtbl_00 = &kDirectInputVtable;
    base.device.vtbl_00 = &kMouseVtable;
    joystick.device.vtbl_00 = &kMouseVtable;
    g_zInput_GlobalState = &directInput.input;
    createDeviceOut = &base.device;
    queryInterfaceOut = &joystick.device;
    fakeCaps.dwSize = 0x2c;
    fakeCaps.dwFlags = 0x700;
    fakeCaps.dwAxes = 4;

    if (zInput::DI_InitJoystickDevice(reinterpret_cast<HWND>(2)) != 1) {
        return 1;
    }

    if (enumDevicesCalls != 1 || createDeviceCalls != 1 || queryInterfaceCalls != 1 ||
        releaseCalls != 1) {
        return 2;
    }
    if (setDataFormatCalls != 1 || getCapabilitiesCalls != 1 || setCooperativeLevelCalls != 1 ||
        lastCooperativeLevelFlags != 5) {
        return 3;
    }
    if (setPropertyCalls < 8 || acquireCalls != 1 || g_zInput_JoystickInitialized != 1 ||
        g_zInput_JoystickAxisCount != 4) {
        return 4;
    }

    return g_zInput_JoystickCaps_ForceFeedback == 0x100 &&
                   g_zInput_JoystickCaps_FFAttack == 0x200 && g_zInput_JoystickCaps_FFFade == 0x400
               ? 0
               : 5;
}

extern "C" int zinput_joystick_acquire_device_smoke(void) {
    ResetMouseGlobals();
    if (zInput::DI_AcquireJoystickDevice() != 0) {
        return 1;
    }

    MouseDeviceFake joystick{};
    joystick.device.vtbl_00 = &kMouseVtable;
    g_zInput_JoystickDevice = &joystick.device;
    acquireResult = 0;
    if (zInput::DI_AcquireJoystickDevice() != 1 || acquireCalls != 1) {
        return 2;
    }

    acquireResult = -1;
    return zInput::DI_AcquireJoystickDevice() == 0 && acquireCalls == 2 ? 0 : 3;
}

extern "C" int zinput_joystick_axis_property_smoke(void) {
    ResetMouseGlobals();
    MouseDeviceFake joystick{};
    joystick.device.vtbl_00 = &kMouseVtable;
    g_zInput_JoystickDevice = &joystick.device;
    g_zInput_JoystickAxisCount = 4;

    if (zInput::DI_SetAxisRange(4, -100, 100) != 0 || lastSetPropertyId != 4 ||
        lastSetPropertySize != 24 || lastSetPropertyObj != 4 || lastSetPropertyHow != 1 ||
        lastSetPropertyMin != -100 || lastSetPropertyMax != 100) {
        return 1;
    }

    if (zInput::DI_SetAxisDeadzone(8, 2500) != 0 || lastSetPropertyId != 5 ||
        lastSetPropertySize != 20 || lastSetPropertyObj != 8 || lastSetPropertyHow != 1 ||
        lastSetPropertyData != 2500) {
        return 2;
    }

    std::int32_t outMin = 0;
    std::int32_t outMax = 0;
    if (zInput::DI_GetAxisRange(20, &outMin, &outMax) != 0 || outMin != -500 ||
        outMax != 500 || lastGetPropertyId != 4 || lastGetPropertySize != 24 ||
        lastGetPropertyObj != 20 || lastGetPropertyHow != 1) {
        return 3;
    }

    zInput::JoystickAxisConfig cfg{};
    cfg.axes[0].lMin = -1000;
    cfg.axes[0].lMax = 1000;
    cfg.axes[0].deadzone = 100;
    cfg.axes[1].lMin = 0;
    cfg.axes[1].lMax = 10000;
    cfg.axes[1].deadzone = 200;
    cfg.axes[2].lMin = -3000;
    cfg.axes[2].lMax = 3000;
    cfg.axes[2].deadzone = 300;
    cfg.axes[3].lMin = -4000;
    cfg.axes[3].lMax = 4000;
    cfg.axes[3].deadzone = 400;

    setPropertyCalls = 0;
    getPropertyCalls = 0;
    if (zInput::DI_ApplyAxisConfig(&cfg) != 1 || setPropertyCalls != 8 ||
        getPropertyCalls != 0 || g_zInput_JoystickAxisConfig.axes[3].deadzone != 400 ||
        cfg.axes[0].midpoint != 0.0f || cfg.axes[0].normScale < 0.000999f ||
        cfg.axes[0].normScale > 0.001001f || cfg.axes[1].midpoint != 5000.0f ||
        cfg.axes[1].normScale < 0.000199f || cfg.axes[1].normScale > 0.000201f) {
        return 4;
    }

    return zInput::DI_ApplyAxisConfig(nullptr) == 0 ? 0 : 5;
}

extern "C" int zinput_joystick_ref_and_enable_smoke(void) {
    MouseDeviceFake joystick{};
    joystick.device.vtbl_00 = &kMouseVtable;
    g_zInput_JoystickDevice = &joystick.device;
    g_zInput_JoystickInitialized = 1;
    g_zInputJoystickFlags = 0;
    g_zInputJoystickPollRefCount = 0;
    g_zInput_JoystickAxisCount = 4;
    setPropertyCalls = 0;
    getPropertyCalls = 0;

    if (zInput::DI_IsJoystickDeviceReady() != 1 || zInput::DI_GetJoystickRefCount() != 0 ||
        zInput::DI_AddJoystickRef() != 1 || zInput::DI_GetJoystickRefCount() != 1 ||
        zInput::DI_ReleaseJoystickRef() != 0 || zInput::DI_ReleaseJoystickRef() != 0) {
        return 1;
    }

    if (zInput::DI_SetJoystickEnabled(1) != 1 || g_zInputJoystickPollRefCount != 1 ||
        g_zInput_JoystickAxisConfig_Gameplay.axes[0].lMin != -1000 ||
        g_zInput_JoystickAxisConfig_Gameplay.axes[1].lMin != -10000 ||
        g_zInput_JoystickAxisConfig_Gameplay.axes[1].lMax != 10000 ||
        g_zInput_JoystickAxisConfig_Gameplay.axes[2].deadzone != 1500 ||
        g_zInput_JoystickAxisConfig.axes[0].midpoint != 0.0f ||
        g_zInput_JoystickAxisConfig.axes[0].normScale != 0.001f || setPropertyCalls != 8 ||
        getPropertyCalls != 0) {
        return 2;
    }

    if (zInput::DI_SetJoystickEnabled(0) != 0 || g_zInputJoystickPollRefCount != 0) {
        return 3;
    }

    g_zInput_JoystickInitialized = 0;
    g_zInput_JoystickDevice = nullptr;
    return 0;
}

extern "C" int zinput_joystick_poll_and_wait_smoke(void) {
    ResetMouseGlobals();
    MouseDeviceFake joystick{};
    joystick.device.vtbl_00 = &kMouseVtable;
    g_zInput_JoystickDevice = &joystick.device;
    g_zInput_JoystickInitialized = 1;
    g_zInput_JoystickAxisCount = 2;
    g_zInput_JoystickCurrentState.lX = 11;
    g_zInput_JoystickCurrentState.rgbButtons[4] = 0;
    fakeJoystickState = {};
    fakeJoystickState.lX = 22;
    fakeJoystickState.lZ = 33;
    fakeJoystickState.lRz = 44;
    fakeJoystickState.lVZ = 55;
    fakeJoystickState.lVRz = 66;
    fakeJoystickState.lAZ = 77;
    fakeJoystickState.lARz = 88;
    fakeJoystickState.lFZ = 99;
    fakeJoystickState.lFRz = 111;
    fakeJoystickState.rgbButtons[4] = 0x80;

    if (zInput::DI_PollJoystickState(0) != &g_zInput_JoystickCurrentState || pollCalls != 1 ||
        getDeviceStateCalls != 1 || g_zInput_JoystickPreviousState.lX != 11 ||
        g_zInput_JoystickCurrentState.lX != 22 || g_zInput_JoystickCurrentState.lZ != 0 ||
        g_zInput_JoystickCurrentState.lRz != 0 || g_zInput_JoystickCurrentState.lVZ != 0 ||
        g_zInput_JoystickCurrentState.lVRz != 0 || zInput::DI_GetButtonTransitionState(5) != 1) {
        return 1;
    }

    FreeOptionList();
    zInput_BindMapContext context = {};
    context.InitCommandMap(3);
    context.m_joystickToCommand[2] = 1;
    context.m_commandCallbacks[1] = BindMapContextCountingCallback;
    g_zInput_BindMap_Current = &context;

    g_bindMapDispatchCallbackCount = 0;
    fakeJoystickState = {};
    fakeJoystickState.rgbButtons[1] = 0x80;
    if (zInput::DI_WaitForButtonPress(0) != 2) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 2;
    }
    if (g_bindMapDispatchCallbackCount != 1) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 6;
    }

    g_zInput_BindMap_Current = nullptr;
    context.FreeNonOwnedBuffers();
    FreeOptionList();

    getDeviceStateResult = static_cast<std::int32_t>(0x8007001e);
    if (zInput::DI_PollJoystickState(0) != nullptr || acquireCalls != 1) {
        return 3;
    }

    getDeviceStateResult = -5;
    if (zInput::DI_PollJoystickState(0) != nullptr) {
        return 4;
    }

    g_zInput_JoystickInitialized = 0;
    return zInput::DI_PollJoystickState(0) == nullptr ? 0 : 5;
}

extern "C" int zinput_mouse_update_acquire_state_smoke() {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_MouseDevice = &fake.device;

    g_zInput_MouseActive = 1;
    zInput::Mouse_UpdateAcquireState();
    if (acquireCalls != 1 || unacquireCalls != 0 || g_zInput_MouseActive != 1) {
        return 1;
    }

    acquireResult = -1;
    zInput::Mouse_UpdateAcquireState();
    if (g_zInput_MouseActive != 0) {
        return 2;
    }

    zInput::Mouse_UpdateAcquireState();
    if (unacquireCalls != 1 || g_zInput_MouseActive != 0) {
        return 3;
    }

    unacquireResult = -1;
    zInput::Mouse_UpdateAcquireState();
    return g_zInput_MouseActive == 1 ? 0 : 4;
}

extern "C" int zinput_mouse_shutdown_device_smoke() {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_MouseDevice = &fake.device;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseActive = 1;

    const int result = zInput::Mouse_ShutdownDevice();
    if (result != 1 || releaseCalls != 1) {
        return 1;
    }
    if (g_zInput_MouseDevice != nullptr || g_zInput_MouseInitialized != 0) {
        return 2;
    }
    return g_zInput_MouseActive == 0 ? 0 : 3;
}

extern "C" int zinput_shutdown_smoke() {
    ResetMouseGlobals();

    MouseDeviceFake mouse = {{&kMouseVtable}};
    MouseDeviceFake keyboard = {{&kMouseVtable}};
    MouseDeviceFake joystick = {{&kMouseVtable}};
    DirectInputFake directInput = {{&kDirectInputVtable}};

    g_zInput_hWnd = reinterpret_cast<HWND>(1);
    g_zInput_MouseDevice = &mouse.device;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseActive = 1;
    g_zInput_KbdDevice = &keyboard.device;
    g_zInput_KbdSystemReady = 1;
    g_zInput_KbdEventBuffer = static_cast<zInput::DIDeviceObjectData *>(std::malloc(16));
    g_zInput_JoystickDevice = &joystick.device;
    g_zInput_JoystickInitialized = 1;
    g_zInput_GlobalState = &directInput.input;

    if (zInput::Shutdown() != 0) {
        return 1;
    }

    if (g_zInput_hWnd != nullptr || g_zInput_MouseDevice != nullptr ||
        g_zInput_KbdDevice != &keyboard.device ||
        g_zInput_KbdEventBuffer == nullptr ||
        g_zInput_JoystickDevice != nullptr || g_zInput_GlobalState != &directInput.input) {
        return 2;
    }

    g_zInput_KbdDevice = nullptr;
    g_zInput_KbdEventBuffer = nullptr;
    return releaseCalls == 3 && directInputReleaseCalls == 1 ? 0 : 3;
}

extern "C" int zinput_mouse_poll_state_smoke() {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_MouseDevice = &fake.device;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseActive = 1;
    g_zInput_MouseClientWidth = 100;
    g_zInput_MouseClientHeight = 80;
    g_zInput_MouseClientCenterX = 50;
    g_zInput_MouseClientCenterY = 40;
    g_zInput_MouseInvClientCenterX = 0.02f;
    g_zInput_MouseInvClientCenterY = 0.025f;
    g_zInput_MouseSensitivityX = 1.0f;
    g_zInput_MouseSensitivityY = 1.0f;
    g_zInput_MouseWrapModeFlag = 0;
    g_zInput_MouseStateSnapshot.cursorClientX = 50;
    g_zInput_MouseStateSnapshot.cursorClientY = 40;
    g_zInput_MouseCurrentState = {1, 2, 3, 0};
    fakeMouseState = {5, -4, 0, 0x80};

    if (zInput::Mouse_PollState(0) != 0) {
        return 10;
    }
    if (pollCalls != 1 || getDeviceStateCalls != 1) {
        return 11;
    }
    if (g_zInput_MousePreviousState.lX != 1 || g_zInput_MouseCurrentState.lX != 5) {
        return 12;
    }
    if (g_zInput_MouseStateSnapshot.deltaX != 5 || g_zInput_MouseStateSnapshot.deltaY != -4) {
        return 13;
    }
    if (g_zInput_MouseStateSnapshot.button1Transition != 1) {
        return 14;
    }

    FreeOptionList();
    zInput_BindMapContext context = {};
    context.InitCommandMap(2);
    context.m_mouseToCommand[1] = 1;
    context.m_commandCallbacks[1] = BindMapContextCountingCallback;
    g_zInput_BindMap_Current = &context;

    g_bindMapDispatchCallbackCount = 0;
    g_zInput_MouseCurrentState = {};
    fakeMouseState = {0, 0, 0, 0x80};
    if (zInput::Mouse_PollState(1) != 0 || g_bindMapDispatchCallbackCount != 1) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 15;
    }

    g_bindMapDispatchCallbackCount = 0;
    g_zInput_MouseCurrentState = {};
    fakeMouseState = {0, 0, 0, 0x80};
    if (zInput::Mouse_PollState(0) != 0 || g_bindMapDispatchCallbackCount != 0) {
        g_zInput_BindMap_Current = nullptr;
        context.FreeNonOwnedBuffers();
        FreeOptionList();
        return 16;
    }

    g_zInput_BindMap_Current = nullptr;
    context.FreeNonOwnedBuffers();
    FreeOptionList();

    fakeMouseState = {0, 0, 0, 0};
    zInput::Mouse_PollAndStoreState(0);
    if (g_zInputMouseLastPollResult != 0 || g_zInput_MouseStateSnapshot.button1Transition != 4) {
        return 2;
    }

    g_zInput_MouseCurrentState = {0, 0, 0, 0x00800000};
    fakeMouseState = {0, 0, 0, 0x00800000};
    if (zInput::Mouse_WaitForButtonPress(0) != 3) {
        return 3;
    }

    g_zInput_MouseActive = 0;
    acquireResult = -1;
    if (zInput::Mouse_PollState(0) != static_cast<std::int32_t>(0x8007001e)) {
        return 4;
    }

    g_zInput_MouseActive = 1;
    acquireResult = 0;
    getDeviceStateResult = static_cast<std::int32_t>(0x8007001e);
    if (zInput::Mouse_PollState(0) != static_cast<std::int32_t>(0x8007001e) || acquireCalls < 2) {
        return 5;
    }

    return 0;
}

extern "C" int zinput_poll_active_devices_smoke() {
    ResetMouseGlobals();

    MouseDeviceFake mouse = {{&kMouseVtable}};
    MouseDeviceFake joystick{};
    joystick.device.vtbl_00 = &kMouseVtable;
    MouseDeviceFake keyboard = {{&kMouseVtable}};

    g_zInput_MouseDevice = &mouse.device;
    g_zInput_MouseActive = 1;
    g_zInputMouseFlags = 1;
    g_zInputMousePollRefCount = 1;
    g_zInput_MouseClientWidth = 100;
    g_zInput_MouseClientHeight = 80;
    g_zInput_MouseClientCenterX = 50;
    g_zInput_MouseClientCenterY = 40;
    g_zInput_MouseInvClientCenterX = 0.02f;
    g_zInput_MouseInvClientCenterY = 0.025f;
    g_zInput_MouseSensitivityX = 1.0f;
    g_zInput_MouseSensitivityY = 1.0f;
    g_zInput_MouseCurrentState = {};
    g_zInput_MouseStateSnapshot.cursorClientX = 50;
    g_zInput_MouseStateSnapshot.cursorClientY = 40;
    fakeMouseState = {2, -3, 0, 0};

    g_zInput_JoystickDevice = &joystick.device;
    g_zInput_JoystickInitialized = 1;
    g_zInput_JoystickAxisCount = 2;
    g_zInputJoystickFlags = 1;
    g_zInputJoystickPollRefCount = 1;
    fakeJoystickState = {};
    fakeJoystickState.lX = 11;

    g_zInput_KbdDevice = &keyboard.device;
    g_zInput_KbdEventBuffer = keyboardEvents;
    g_zInput_DeviceRegistry = 1;
    g_zInputKeyboardPollRefCount = 1;
    getDeviceDataCount = 1;
    keyboardEvents[0] = {0x1e, 0x80, 0, 0};

    zInput::PollActiveDevices(0);

    if (g_zInputMouseLastPollResult != 0 || g_zInput_MouseCurrentState.lX != 2 ||
        g_zInput_JoystickCurrentState.lX != 11 ||
        g_zInputKbdKeyDispatchTable[0x1e].state != 1) {
        return 1;
    }

    if (pollCalls != 2 || getDeviceStateCalls != 2 || getDeviceDataCalls != 1) {
        return 2;
    }

    return 0;
}

extern "C" int zinput_suspend_flags_smoke() {
    g_zInput_MouseSuspendFlags = 0;
    g_zInput_JoystickSuspendFlags = 0;
    g_zInput_KeyboardSuspendFlags = 0;

    if (zInput::Mouse_IsUnsuspended() != 1 || zInput::Joystick_IsUnsuspended() != 1 ||
        zInput_Keyboard_IsUnsuspended() != 1) {
        return 1;
    }

    zInput::Mouse_Suspend();
    zInput::Joystick_Suspend();
    zInput::Keyboard_Suspend();

    if (g_zInput_MouseSuspendFlags != 2 || g_zInput_JoystickSuspendFlags != 2 ||
        g_zInput_KeyboardSuspendFlags != 2) {
        return 2;
    }

    return zInput::Mouse_IsUnsuspended() == 0 && zInput::Joystick_IsUnsuspended() == 0 &&
                   zInput_Keyboard_IsUnsuspended() == 0
               ? 0
               : 3;
}

extern "C" int zinput_on_app_deactivate_smoke() {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_MouseDevice = &fake.device;
    g_zInput_MouseActive = 1;
    g_zInput_MouseSuspendFlags = 0;
    g_zInput_JoystickSuspendFlags = 0;
    g_zInput_KeyboardSuspendFlags = 0;

    zInput::OnAppDeactivate();

    if (g_zInput_MouseActive != 0 || unacquireCalls != 1) {
        return 1;
    }

    return g_zInput_MouseSuspendFlags == 2 && g_zInput_JoystickSuspendFlags == 2 &&
                   g_zInput_KeyboardSuspendFlags == 2
               ? 0
               : 2;
}

extern "C" int zinput_joystick_reset_and_resume_smoke() {
    g_zInput_JoystickInitialized = 1;
    g_zInput_JoystickSuspendFlags = 2;
    for (int i = 0; i < 11; ++i) {
        g_zInput_JoystickCurrentState.rgbButtons[i] = 0xff;
        g_zInput_JoystickPreviousState.rgbButtons[i] = 0xff;
    }
    for (int i = 0; i < 4; ++i) {
        g_zInput_JoystickCurrentState.rgdwPOV[i] = 0;
        g_zInput_JoystickPreviousState.rgdwPOV[i] = 0;
    }

    zInput::Joystick_ResumeFromSuspend();

    if (g_zInput_JoystickSuspendFlags != 0 || g_zInput_JoystickCurrentState.rgbButtons[0] != 0xff ||
        g_zInput_JoystickPreviousState.rgbButtons[0] != 0xff) {
        return 1;
    }

    for (int i = 1; i < 11; ++i) {
        if (g_zInput_JoystickCurrentState.rgbButtons[i] != 0 ||
            g_zInput_JoystickPreviousState.rgbButtons[i] != 0) {
            return 2;
        }
    }
    for (int i = 0; i < 4; ++i) {
        if (g_zInput_JoystickCurrentState.rgdwPOV[i] != 0xffff ||
            g_zInput_JoystickPreviousState.rgdwPOV[i] != 0xffff) {
            return 3;
        }
    }

    return 0;
}

extern "C" int zinput_mouse_reset_and_resume_smoke() {
    ResetMouseGlobals();
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseSuspendFlags = 2;
    g_zInput_MouseCurrentState = {4, 5, 6, 0x00800080};
    g_zInput_MousePreviousState = {};
    g_zInput_MouseStateSnapshot.cursorClientX = 8;
    g_zInput_MouseStateSnapshot.cursorClientY = 9;
    g_zInput_MouseStateSnapshot.deltaX = 3;
    g_zInput_MouseStateSnapshot.deltaY = -4;
    g_zInput_MouseStateSnapshot.button1Transition = 7;
    g_zInput_MouseStateSnapshot.button2Transition = 8;
    g_zInput_MouseStateSnapshot.button3Transition = 9;
    g_zInput_MouseClientWidth = 20;
    g_zInput_MouseClientHeight = 20;
    g_zInput_MouseClientCenterX = 10;
    g_zInput_MouseClientCenterY = 10;
    g_zInput_MouseInvClientCenterX = 0.1f;
    g_zInput_MouseInvClientCenterY = 0.1f;
    g_zInput_MouseSensitivityX = 1.0f;
    g_zInput_MouseSensitivityY = 1.0f;
    g_zInput_MouseWrapModeFlag = 0;

    zInput::Mouse_ResetTransitionState();
    if (g_zInput_MousePreviousState.lX != 4 || g_zInput_MousePreviousState.lY != 5 ||
        g_zInput_MousePreviousState.lZ != 6 ||
        g_zInput_MousePreviousState.rgbButtons != 0x00800080 ||
        g_zInput_MouseStateSnapshot.deltaX != 0 ||
        g_zInput_MouseStateSnapshot.deltaY != 0 ||
        g_zInput_MouseStateSnapshot.button1Transition != 0 ||
        g_zInput_MouseStateSnapshot.button2Transition != 0 ||
        g_zInput_MouseStateSnapshot.button3Transition != 0) {
        return 1;
    }

    g_zInput_MouseSuspendFlags = 2;
    g_zInput_MouseStateSnapshot.deltaX = 3;
    g_zInput_MouseStateSnapshot.deltaY = -4;
    g_zInput_MouseStateSnapshot.button1Transition = 7;
    g_zInput_MouseStateSnapshot.button2Transition = 8;
    g_zInput_MouseStateSnapshot.button3Transition = 9;

    zInput::Mouse_ResumeFromSuspend();

    return g_zInput_MouseSuspendFlags == 0 && g_zInput_MousePreviousState.lX == 4 &&
                   g_zInput_MousePreviousState.lY == 5 && g_zInput_MousePreviousState.lZ == 6 &&
                   g_zInput_MousePreviousState.rgbButtons == 0x00800080 &&
                   g_zInput_MouseStateSnapshot.deltaX == 0 &&
                   g_zInput_MouseStateSnapshot.deltaY == 0 &&
                   g_zInput_MouseStateSnapshot.button1Transition == 0 &&
                   g_zInput_MouseStateSnapshot.button2Transition == 0 &&
                   g_zInput_MouseStateSnapshot.button3Transition == 0
               ? 0
               : 2;
}

extern "C" int zinput_directinput_report_error_smoke() {
    return zInput::DI_ReportError(0, "test.cpp", 1) == 1 &&
                   zInput::DI_ReportError(static_cast<std::int32_t>(0x8007001e), "test.cpp", 2) == 0
               ? 0
               : 1;
}

extern "C" int zinput_keyboard_reset_and_resume_smoke() {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_KbdSystemReady = 1;
    g_zInput_KbdDevice = &fake.device;
    g_zInput_KbdEventBuffer = keyboardEvents;
    g_zInput_KeyboardSuspendFlags = 2;
    g_zInput_KbdModifierState = 0;
    g_zInputKbdKeyDispatchTable[0x21e].state = 5;
    getDeviceDataCount = 2;
    keyboardEvents[0] = {0x1d, 0x80, 0, 0};
    keyboardEvents[1] = {0x1e, 0x80, 0, 0};

    zInput::Keyboard_ResumeFromSuspend();

    if (getDeviceDataCalls != 1 || acquireCalls != 0 || g_zInput_KeyboardSuspendFlags != 0 ||
        g_zInput_KbdModifierState != 0) {
        return 1;
    }

    return g_zInputKbdKeyDispatchTable[0x21e].state == 0 && keyboardEvents[1].dwData == 0xa0 ? 0
                                                                                             : 2;
}

extern "C" int zinput_keyboard_reset_inputlost_smoke() {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_KbdSystemReady = 1;
    g_zInput_KbdDevice = &fake.device;
    g_zInput_KbdEventBuffer = keyboardEvents;
    getDeviceDataResult = static_cast<std::int32_t>(0x8007001e);

    zInput::Keyboard_ResetTransitionState();

    return getDeviceDataCalls == 1 && acquireCalls == 1 && g_zInput_KbdModifierState == 0 ? 0 : 1;
}

extern "C" int zinput_reset_all_transition_state_smoke() {
    ResetMouseGlobals();
    g_zInput_KbdSystemReady = 0;
    g_zInput_MouseInitialized = 1;
    g_zInput_MouseCurrentState = {3, 4, 5, 0x80};
    g_zInput_MousePreviousState = {};
    g_zInput_MouseStateSnapshot.deltaX = 9;
    g_zInput_MouseStateSnapshot.deltaY = -7;
    g_zInput_MouseStateSnapshot.button1Transition = 1;
    g_zInput_MouseStateSnapshot.button2Transition = 1;
    g_zInput_MouseStateSnapshot.button3Transition = 1;
    g_zInput_JoystickInitialized = 1;
    for (int i = 0; i < 11; ++i) {
        g_zInput_JoystickCurrentState.rgbButtons[i] = 0x80;
        g_zInput_JoystickPreviousState.rgbButtons[i] = 0x80;
    }
    for (int i = 0; i < 4; ++i) {
        g_zInput_JoystickCurrentState.rgdwPOV[i] = i;
        g_zInput_JoystickPreviousState.rgdwPOV[i] = i;
    }

    zInput::ResetAllTransitionState();

    bool joystickReset = true;
    for (int i = 1; i < 11; ++i) {
        joystickReset = joystickReset && g_zInput_JoystickCurrentState.rgbButtons[i] == 0 &&
                        g_zInput_JoystickPreviousState.rgbButtons[i] == 0;
    }
    for (int i = 0; i < 4; ++i) {
        joystickReset = joystickReset && g_zInput_JoystickCurrentState.rgdwPOV[i] == 0xffff &&
                        g_zInput_JoystickPreviousState.rgdwPOV[i] == 0xffff;
    }

    return g_zInput_MousePreviousState.lX == 3 && g_zInput_MousePreviousState.lY == 4 &&
                   g_zInput_MousePreviousState.lZ == 5 &&
                   g_zInput_MousePreviousState.rgbButtons == 0x80 &&
                   g_zInput_MouseStateSnapshot.deltaX == 0 &&
                   g_zInput_MouseStateSnapshot.deltaY == 0 &&
                   g_zInput_MouseStateSnapshot.button1Transition == 0 &&
                   g_zInput_MouseStateSnapshot.button2Transition == 0 &&
                   g_zInput_MouseStateSnapshot.button3Transition == 0 && joystickReset
               ? 0
               : 1;
}

extern "C" int zinput_on_app_activate_smoke() {
    ResetMouseGlobals();
    MouseDeviceFake fake = {{&kMouseVtable}};
    g_zInput_hWnd = reinterpret_cast<HWND>(1);
    g_zInput_MouseDevice = &fake.device;
    g_zInput_MouseInitialized = 0;
    g_zInput_KbdSystemReady = 1;
    g_zInput_KbdDevice = &fake.device;
    g_zInput_KbdEventBuffer = keyboardEvents;
    g_zInput_MouseSuspendFlags = 2;
    g_zInput_JoystickSuspendFlags = 2;
    g_zInput_KeyboardSuspendFlags = 2;

    zInput::OnAppActivate();

    return g_zInput_MouseActive == 1 && g_zInput_MouseSuspendFlags == 0 &&
                   g_zInput_JoystickSuspendFlags == 0 && g_zInput_KeyboardSuspendFlags == 0 &&
                   acquireCalls == 1
               ? 0
               : 1;
}
