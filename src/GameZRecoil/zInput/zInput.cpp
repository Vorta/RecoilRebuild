#include "zInput.h"

#include "GameZRecoil/include/zClass.h"
#include "GameZRecoil/zError/zError.h"
#include "GameZRecoil/zGame/zGame.h"
#include "GameZRecoil/zLoc/zLoc.h"

#define DIRECTINPUT_VERSION 0x0500
#include <dinput.h>

#if defined(_MSC_VER) && defined(_M_IX86)
#include <intrin.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) && defined(_M_IX86)
extern "C" void __cdecl _ftol();
#endif

extern "C" {
zInput::DIDirectInput *g_zInput_GlobalState = 0;
unsigned char g_zInput_DeviceRegistry = 0;
short g_zInputKeyboardPollRefCount = 0;
int g_zInput_MouseActive = 0;
int g_zInput_MouseInitialized = 0;
zInput::DIDevice *g_zInput_MouseDevice = 0;
int g_zInput_MouseCoopLevelFlags = 0;
unsigned char g_zInputMouseFlags = 0;
short g_zInputMousePollRefCount = 0;
unsigned char g_zInput_KeyboardSuspendFlags = 0;
unsigned char g_zInput_JoystickSuspendFlags = 0;
unsigned char g_zInput_MouseSuspendFlags = 0;
int g_zInput_JoystickInitialized = 0;
zInput::DIDevice *g_zInput_JoystickDevice = 0;
unsigned char g_zInputJoystickFlags = 0;
short g_zInputJoystickPollRefCount = 0;
int g_zInput_JoystickAxisCount = 0;
zInput::JoystickAxisConfig g_zInput_JoystickAxisConfig = {0};
zInput::JoystickAxisConfig g_zInput_JoystickAxisConfig_Gameplay = {0};
zInput::JoystickStatePartial g_zInput_JoystickCurrentState = {0};
zInput::JoystickStatePartial g_zInput_JoystickPreviousState = {0};
zInput::MouseDeviceState g_zInput_MouseCurrentState = {0};
zInput::MouseDeviceState g_zInput_MousePreviousState = {0};
zInput::MouseStateSnapshot g_zInput_MouseStateSnapshot = {0};
int g_zInputMouseLastPollResult = 0;
int g_zInput_MouseClientWidth = 0;
int g_zInput_MouseClientHeight = 0;
int g_zInput_MouseClientCenterX = 0;
int g_zInput_MouseClientCenterY = 0;
float g_zInput_MouseInvClientCenterX = 0.0f;
float g_zInput_MouseInvClientCenterY = 0.0f;
float g_zInput_MouseSensitivityX = 1.0f;
float g_zInput_MouseSensitivityY = 1.0f;
int g_zInput_MouseWrapModeFlag = 0;
HWND g_zInput_hWnd = 0;
int g_zInput_KbdSystemReady = 0;
zInput::DIDevice *g_zInput_KbdDevice = 0;
zInput::DIDeviceObjectData *g_zInput_KbdEventBuffer = 0;
int g_zInput_KbdModifierState = 0;
zInput::KbdKeyDispatchEntry g_zInputKbdKeyDispatchTable[0x7de] = {0};
void *g_zInput_KbdRawEventCallback = 0;
void *g_zInput_KbdRawEventCallbackCtx = 0;
int g_zInput_KbdDikToAsciiTable[0x100] = {0};
int g_zInput_KbdDikToAsciiTableReady = 0;
int g_zInput_JoystickCaps_ForceFeedback = 0;
int g_zInput_JoystickCaps_FFAttack = 0;
int g_zInput_JoystickCaps_FFFade = 0;
zInput_GameStateOrMapTablePartial *g_GameStateOrMapTable = 0;
float g_Player_DeltaTime = 0.0f;
float g_Player_InvDeltaTime = 0.0f;
float g_Player_DeltaTimeScaled001 = 0.0f;
float g_zInput_DiPitchAngleLowpassRad = 0.0f;
int *ZOPT_INPUT_JOYSTICK = 0;
int *ZOPT_JOYSTICK_NUM_AXES = 0;
int *ZOPT_JOYSTICK_NUM_BUTTONS = 0;
const char *g_zInput_DikKeyNames[0x100] = {0};
const char *g_zInput_JoystickButtonNames[9] = {0};
const char *g_zInput_MouseButtonNames[4] = {0};
zInput_BindMapContext *g_zInput_BindMap_Current = 0;
zInput_BindMapOverlayStackNode *g_zInput_BindMapOverlayNodeFreeList = 0;
zInput_BindMapOverlayStackNode *g_zInput_BindMapOverlayNodeStackHead = 0;
int g_zInput_BindMapOverlayDepth = 0;
zInput_BindGroupInfoList g_zInput_BindGroupInfoList = {0};
int g_zInput_CurrentBindGroupIndex = 0;
int g_zInput_CommandLocIdTable[0x100] = {0};

// Reimplements 0x471c80: zInput_Keyboard_IsUnsuspended
RECOIL_NOINLINE int RECOIL_CDECL zInput_Keyboard_IsUnsuspended() {
    return (~g_zInput_KeyboardSuspendFlags & 2U) >> 1;
}
}

namespace {
char g_zInput_EmptyName[] = "";

struct BindMapDefaultBindingSpec {
    int commandId;
    int messageId;
    int primaryKey;
    int secondaryKey;
    int joystickSlot;
    int mouseSlot;
};

inline void CopyCommandSlots(void *dest, const void *src, int count) {
    memcpy(dest, src, static_cast<size_t>(count) * sizeof(int));
}

} // namespace

// Reimplements 0x470e80: zInput_BindMapContext_DispatchFromKeyboardEvent
extern "C" RECOIL_NOINLINE void RECOIL_FASTCALL
zInput_BindMapContext_DispatchFromKeyboardEvent(int dikCode) {
    const int commandId = g_zInput_BindMap_Current->GetCommandByAnyKeyboardKey(dikCode);
    zInputCommandCallbackFn callback = g_zInput_BindMap_Current->m_commandCallbacks[commandId];
    if (callback != 0) {
        callback();
    }
}

// Reimplements 0x4706c0: zInput_BindMapContext::InitFromTemplate
RECOIL_NOINLINE zInput_BindMapContext *RECOIL_THISCALL
zInput_BindMapContext::InitFromTemplate(const zInput_BindMapContext *tmpl) {
    m_isOverlay = 0;
    if (tmpl == 0) {
        return this;
    }

    m_commandCount = tmpl->m_commandCount;
    m_packedBindings =
        static_cast<int *>(calloc(tmpl->m_commandCount, sizeof(int)));
    CopyCommandSlots(m_packedBindings, tmpl->m_packedBindings, tmpl->m_commandCount);

    m_commandCallbacks = static_cast<zInputCommandCallbackFn *>(
        calloc(m_commandCount, sizeof(zInputCommandCallbackFn)));
    CopyCommandSlots(m_commandCallbacks, tmpl->m_commandCallbacks, m_commandCount);

    m_commandLabels = static_cast<char **>(calloc(m_commandCount, sizeof(char *)));
    for (int i = 0; i < m_commandCount; ++i) {
        m_commandLabels[i] = static_cast<char *>(calloc(1, 0x50));
        strncpy(m_commandLabels[i], tmpl->m_commandLabels[i], 0x50);
    }

    RebuildLookupIndices();
    return this;
}

// Reimplements 0x4707a0: zInput_BindMapContext::FreeAllBuffers
RECOIL_NOINLINE void RECOIL_THISCALL zInput_BindMapContext::FreeAllBuffers() {
    if (m_commandCallbacks != 0) {
        free(m_commandCallbacks);
    }
    m_commandCallbacks = 0;

    if (m_commandLabels != 0) {
        for (int i = 0; i < m_commandCount; ++i) {
            if (m_commandLabels[i] != 0) {
                free(m_commandLabels[i]);
            }
            m_commandLabels[i] = 0;
        }
        free(m_commandLabels);
    }
    m_commandLabels = 0;

    if (m_packedBindings != 0) {
        free(m_packedBindings);
    }
    m_packedBindings = 0;
}

// Reimplements 0x470820: zInput_BindMapContext::RebuildLookupIndices
RECOIL_NOINLINE void RECOIL_THISCALL zInput_BindMapContext::RebuildLookupIndices() {
    for (int i = 0; i < 0x7de; ++i) {
        m_primaryKeyToCommand[i] = 0;
        m_secondaryKeyToCommand[i] = 0;
    }
    for (int i_190 = 0; i_190 < 0x10; ++i_190) {
        m_joystickToCommand[i_190] = 0;
    }
    for (int i_193 = 0; i_193 < 4; ++i_193) {
        m_mouseToCommand[i_193] = 0;
    }

    zInput::Keyboard_ClearKeyCallbackTable();
    {
    for (int commandId = 1; commandId < m_commandCount; ++commandId) {
        const unsigned int packed = static_cast<unsigned int>(m_packedBindings[commandId]);
        m_primaryKeyToCommand[packed & 0x7ff] = commandId;
        m_secondaryKeyToCommand[(packed >> 0x0b) & 0x7ff] = commandId;
        m_joystickToCommand[(packed >> 0x16) & 0x0f] = commandId;
        m_mouseToCommand[(packed >> 0x1a) & 0x03] = commandId;

        zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
        if (callback != 0) {
            SetCommandCallback(commandId, callback);
        }
    }
    }
}

// Reimplements 0x4708f0: zInput_BindMapContext::InitCommandMap
RECOIL_NOINLINE void RECOIL_THISCALL
zInput_BindMapContext::InitCommandMap(int commandCount) {
    m_commandCount = commandCount;
    zOptionEntryPartial *option = zGame::Options_GetOrCreateOption(
        "CmdMap", 7, commandCount * static_cast<int>(sizeof(int)), 1);
    m_packedBindings = (int *)(option->payloadOrBuffer);
    m_commandCallbacks = static_cast<zInputCommandCallbackFn *>(
        calloc(commandCount, sizeof(zInputCommandCallbackFn)));
    m_commandLabels = static_cast<char **>(calloc(commandCount, sizeof(char *)));
    for (int i = 0; i < commandCount; ++i) {
        m_commandLabels[i] = static_cast<char *>(calloc(1, 0x50));
    }

    ResetAllBindings();
}

// Reimplements 0x470960: zInput_BindMapContext::FreeNonOwnedBuffers
RECOIL_NOINLINE void RECOIL_THISCALL zInput_BindMapContext::FreeNonOwnedBuffers() {
    if (m_commandCallbacks != 0) {
        free(m_commandCallbacks);
    }
    m_commandCallbacks = 0;

    if (m_commandLabels != 0) {
        for (int i = 0; i < m_commandCount; ++i) {
            if (m_commandLabels[i] != 0) {
                free(m_commandLabels[i]);
            }
            m_commandLabels[i] = 0;
        }
        free(m_commandLabels);
    }
    m_commandLabels = 0;
    m_packedBindings = 0;
}

// Reimplements 0x4709d0: zInput_BindMapContext::ResetAllBindings
RECOIL_NOINLINE void RECOIL_THISCALL zInput_BindMapContext::ResetAllBindings() {
    for (int i = 0; i < m_commandCount; ++i) {
        m_packedBindings[i] = zInput::BindMap_PackBindingCode(0, 0, 0, 0);
        m_commandCallbacks[i] = 0;
    }

    RebuildLookupIndices();
}

// Reimplements 0x470a40: zInput_BindMapContext::GetPrimaryKeyboardKey
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetPrimaryKeyboardKey(int commandIndex) {
    return m_packedBindings[commandIndex] & 0x7ff;
}

// Reimplements 0x470a60: zInput_BindMapContext::GetSecondaryKeyboardKey
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetSecondaryKeyboardKey(int commandIndex) {
    return (static_cast<unsigned int>(m_packedBindings[commandIndex]) >> 0x0b) & 0x7ff;
}

// Reimplements 0x470a80: zInput_BindMapContext::GetJoystickButtonSlot
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetJoystickButtonSlot(int commandIndex) {
    return (static_cast<unsigned int>(m_packedBindings[commandIndex]) >> 0x16) & 0x0f;
}

// Reimplements 0x470aa0: zInput_BindMapContext::GetMouseButtonSlot
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetMouseButtonSlot(int commandIndex) {
    return (static_cast<unsigned int>(m_packedBindings[commandIndex]) >> 0x1a) & 0x03;
}

// Reimplements 0x470ac0: zInput_BindMapContext::GetCommandByPrimaryKey
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetCommandByPrimaryKey(int keyboardKey) {
    return m_primaryKeyToCommand[keyboardKey];
}

// Reimplements 0x470ad0: zInput_BindMapContext::GetCommandBySecondaryKey
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetCommandBySecondaryKey(int keyboardKey) {
    return m_secondaryKeyToCommand[keyboardKey];
}

// Reimplements 0x470ae0: zInput_BindMapContext::GetCommandByAnyKeyboardKey
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetCommandByAnyKeyboardKey(int keyboardKey) {
    const int primary = GetCommandByPrimaryKey(keyboardKey);
    if (primary != 0) {
        return primary;
    }

    return GetCommandBySecondaryKey(keyboardKey);
}

// Reimplements 0x470b00: zInput_BindMapContext::GetCommandByJoystickSlot
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetCommandByJoystickSlot(int joystickSlot) {
    return m_joystickToCommand[joystickSlot];
}

// Reimplements 0x470b10: zInput_BindMapContext::GetCommandByMouseSlot
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::GetCommandByMouseSlot(int mouseSlot) {
    return m_mouseToCommand[mouseSlot];
}

// Reimplements 0x470b20: zInput_BindMapContext::SetPrimaryKeyBinding
RECOIL_NOINLINE void RECOIL_THISCALL
zInput_BindMapContext::SetPrimaryKeyBinding(int keyCode, int commandId) {
    if (keyCode != 0) {
        m_packedBindings[m_primaryKeyToCommand[keyCode]] &= 0xfffff800;
        m_primaryKeyToCommand[keyCode] = commandId;
    }
    if (commandId == 0) {
        return;
    }

    m_primaryKeyToCommand[m_packedBindings[commandId] & 0x7ff] = 0;
    m_packedBindings[commandId] =
        ((keyCode ^ m_packedBindings[commandId]) & 0x7ff) ^ m_packedBindings[commandId];
}

// Reimplements 0x470b80: zInput_BindMapContext::SetSecondaryKeyBinding
RECOIL_NOINLINE void RECOIL_THISCALL
zInput_BindMapContext::SetSecondaryKeyBinding(int keyCode, int commandId) {
    if (keyCode != 0) {
        m_packedBindings[m_secondaryKeyToCommand[keyCode]] &= 0xffc007ff;
        m_secondaryKeyToCommand[keyCode] = commandId;
    }
    if (commandId == 0) {
        return;
    }

    m_secondaryKeyToCommand[(static_cast<unsigned int>(m_packedBindings[commandId]) >> 0x0b) &
                            0x7ff] = 0;
    m_packedBindings[commandId] =
        (m_packedBindings[commandId] & 0xffc007ff) | ((keyCode & 0x7ff) << 0x0b);
}

// Reimplements 0x470bf0: zInput_BindMapContext::SetJoystickBinding
RECOIL_NOINLINE void RECOIL_THISCALL
zInput_BindMapContext::SetJoystickBinding(int joystickSlot, int commandId) {
    if (joystickSlot != 0) {
        m_packedBindings[m_joystickToCommand[joystickSlot]] &= 0xfc3fffff;
        m_joystickToCommand[joystickSlot] = commandId;
    }
    if (commandId == 0) {
        return;
    }

    m_joystickToCommand[(static_cast<unsigned int>(m_packedBindings[commandId]) >> 0x16) & 0x0f] =
        0;
    m_packedBindings[commandId] =
        ((joystickSlot & 0x0f) << 0x16) | (m_packedBindings[commandId] & 0xfc3fffff);
}

// Reimplements 0x470c60: zInput_BindMapContext::SetMouseBinding
RECOIL_NOINLINE void RECOIL_THISCALL
zInput_BindMapContext::SetMouseBinding(int mouseSlot, int commandId) {
    if (mouseSlot != 0) {
        m_packedBindings[m_mouseToCommand[mouseSlot]] &= 0xf3ffffff;
        m_mouseToCommand[mouseSlot] = commandId;
    }
    if (commandId == 0) {
        return;
    }

    m_mouseToCommand[(static_cast<unsigned int>(m_packedBindings[commandId]) >> 0x1a) & 0x03] = 0;
    m_packedBindings[commandId] =
        ((mouseSlot & 0x03) << 0x1a) | (m_packedBindings[commandId] & 0xf3ffffff);
}

// Reimplements 0x470cd0: zInput_BindMapContext::SetBindingRecord
RECOIL_NOINLINE void RECOIL_THISCALL zInput_BindMapContext::SetBindingRecord(
    int commandId, const char *labelSrc, int primaryKey,
    int secondaryKey, int joystickSlot, int mouseSlot) {
    if (labelSrc != 0 && *labelSrc != '\0') {
        strncpy(m_commandLabels[commandId], labelSrc, 0x4f);
    }

    SetPrimaryKeyBinding(primaryKey, commandId);
    SetSecondaryKeyBinding(secondaryKey, commandId);
    SetJoystickBinding(joystickSlot, commandId);
    SetMouseBinding(mouseSlot, commandId);
}

// Reimplements 0x470d40: zInput_BindMapContext::DispatchMouseButtonCallbacks
RECOIL_NOINLINE void RECOIL_THISCALL zInput_BindMapContext::DispatchMouseButtonCallbacks() {
    zInput::MouseStateSnapshot *const state = zInput::Mouse_GetStateSnapshotPtr();
    if (state->button1Transition == 1) {
        const int commandId = GetCommandByMouseSlot(1);
        zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
        if (callback != 0) {
            callback();
        }
    }
    if (state->button2Transition == 1) {
        const int commandId = GetCommandByMouseSlot(2);
        zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
        if (callback != 0) {
            callback();
        }
    }
    if (state->button3Transition == 1) {
        const int commandId = GetCommandByMouseSlot(3);
        zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
        if (callback != 0) {
            callback();
        }
    }
}

// Reimplements 0x470db0: zInput_BindMapContext::DispatchJoystickButtonCallbacks
RECOIL_NOINLINE void RECOIL_THISCALL zInput_BindMapContext::DispatchJoystickButtonCallbacks() {
    {
    for (int slot = 1; slot < 0x0b; ++slot) {
        if (zInput::DI_GetButtonTransitionState(slot) == 1) {
            const int commandId = zInput::BindMapCurrent_GetCommandByJoystickSlot(slot);
            zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
            if (callback != 0) {
                callback();
            }
        }
    }
    }
}

// Reimplements 0x470df0: zInput_BindMapContext::SetCommandCallback
RECOIL_NOINLINE int RECOIL_THISCALL zInput_BindMapContext::SetCommandCallback(
    int commandId, zInputCommandCallbackFn callback) {
    const int primary = GetPrimaryKeyboardKey(commandId);
    const int secondary = GetSecondaryKeyboardKey(commandId);
    if (primary == 0 && secondary == 0) {
        return 0;
    }

    m_commandCallbacks[commandId] = callback;
    if (primary != 0) {
        zInput::Keyboard_RegisterKeyCallback(
            primary, (void *)(&zInput_BindMapContext_DispatchFromKeyboardEvent),
            m_commandLabels[commandId]);
    }
    if (secondary != 0) {
        zInput::Keyboard_RegisterKeyCallback(
            secondary, (void *)(&zInput_BindMapContext_DispatchFromKeyboardEvent),
            m_commandLabels[commandId]);
    }

    return 1;
}

// Reimplements 0x470eb0: zInput_BindMapContext::ReadCommandInputState
RECOIL_NOINLINE int RECOIL_THISCALL
zInput_BindMapContext::ReadCommandInputState(int commandIndex) {
    int result = 0;
    const int primary = GetPrimaryKeyboardKey(commandIndex);
    const int secondary = GetSecondaryKeyboardKey(commandIndex);
    if (primary != 0) {
        result = zInput::Keyboard_GetKeyTransitionState(primary);
    }
    if (secondary != 0) {
        result |= zInput::Keyboard_GetKeyTransitionState(secondary);
    }

    const int joystickButton = GetJoystickButtonSlot(commandIndex);
    if (static_cast<unsigned int>(joystickButton) > 0 &&
        static_cast<unsigned int>(joystickButton) < 0x0b) {
        result |= zInput::DI_GetButtonTransitionState(joystickButton);
    }

    const int mouseButton = GetMouseButtonSlot(commandIndex);
    if (mouseButton == 1) {
        return result | g_zInput_MouseStateSnapshot.button1Transition;
    }
    if (mouseButton == 2) {
        return result | g_zInput_MouseStateSnapshot.button2Transition;
    }
    if (mouseButton == 3) {
        return result | g_zInput_MouseStateSnapshot.button3Transition;
    }

    return result;
}

// Reimplements 0x470f50: zInput_BindMapContext::CopyCommandLabel
RECOIL_NOINLINE char *RECOIL_THISCALL zInput_BindMapContext::CopyCommandLabel(
    int commandId, char *destBuf, int maxBytes) {
    char *source = m_commandLabels[commandId];
    if (source == 0) {
        return 0;
    }

    return strncpy(destBuf, source, maxBytes);
}

// Reimplements 0x42a9d0: zInput_BindGroupInfoVec::Count
RECOIL_NOINLINE int RECOIL_THISCALL zInput_BindGroupInfoVec::Count() {
    zInput_BindGroupInfo **const begin = this->begin;
    if (begin == 0) {
        return 0;
    }

    return static_cast<int>(end - begin);
}

namespace zInp {
// Reimplements 0x408390: zInp::SetJoystickOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetJoystickOption(int enabled) {
    if (ZOPT_INPUT_JOYSTICK != 0) {
        *ZOPT_INPUT_JOYSTICK = enabled;
    }
}

// Reimplements 0x4083a0: zInp::SetJoystickAxesCountOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetJoystickAxesCountOption(int axisCount) {
    *ZOPT_JOYSTICK_NUM_AXES = axisCount;
}

// Reimplements 0x4083b0: zInp::SetJoystickButtonCountOption
RECOIL_NOINLINE void RECOIL_FASTCALL SetJoystickButtonCountOption(int buttonCount) {
    *ZOPT_JOYSTICK_NUM_BUTTONS = buttonCount;
}

// Reimplements 0x4083c0: zInp::GetJoystickOption
RECOIL_NOINLINE int RECOIL_CDECL GetJoystickOption() {
    return *ZOPT_INPUT_JOYSTICK;
}
} // namespace zInp

extern "C" {
// Reimplements 0x472480: zInput_DI_HasForceFeedback
RECOIL_NOINLINE int RECOIL_CDECL zInput_DI_HasForceFeedback() {
    return g_zInput_JoystickCaps_ForceFeedback;
}

// Reimplements 0x42fa80: zInput_DI_IsForceFeedbackEnabled
RECOIL_NOINLINE int RECOIL_CDECL zInput_DI_IsForceFeedbackEnabled() {
    if (zInp::GetJoystickOption() != 0 && zInput_DI_HasForceFeedback() != 0) {
        return 1;
    }

    return 0;
}

// Reimplements 0x472450: zInput_DI_CreateForceFeedbackEffect
RECOIL_NOINLINE zInput_DiEffect *RECOIL_FASTCALL
zInput_DI_CreateForceFeedbackEffect(const GUID *rguidEffect, const void *effect) {
    if (g_zInput_JoystickDevice == 0) {
        return 0;
    }

    zInput_DiEffect *outEffect = 0;
    const int result = g_zInput_JoystickDevice->vtbl_00->CreateEffect_48(
        g_zInput_JoystickDevice, rguidEffect, effect, &outEffect, 0);
    return result < 0 ? 0 : outEffect;
}

static float ClampForceFeedbackGain(float gain) {
    if (gain > 1.0f) {
        return 1.0f;
    }
    if (gain < 0.0f) {
        return 0.0f;
    }

    return gain;
}

static float ClampForceFeedbackGainRange(float gain, float minGain, float maxGain) {
    if (gain > maxGain) {
        return maxGain;
    }
    if (gain < minGain) {
        return minGain;
    }

    return gain;
}

static float WrapForceFeedbackPolarRadians(float angle) {
    const float kTwoPi = 6.28318548f;
    if (angle < -kTwoPi) {
        angle += kTwoPi;
    } else if (angle > kTwoPi) {
        angle -= kTwoPi;
    }

    return angle;
}

static int ForceFeedbackDirectionFromRadians(float angle) {
    const double kRadToDeg = 57.295779513079999;
    return static_cast<int>(angle * kRadToDeg) * 100;
}

static int ForceFeedbackDirectionFromImpact(const zVec3 *worldPosXZ, bool sourceToPlayer) {
    const float kPi = 3.14159274f;
    const zInput_PlayerStatePartial *const playerState = g_GameStateOrMapTable->playerState;
    const float sourceBearing =
        sourceToPlayer ? static_cast<float>(atan2(worldPosXZ->z, worldPosXZ->x))
                       : static_cast<float>(atan2(-worldPosXZ->z, -worldPosXZ->x));
    const float playerBearing =
        static_cast<float>(atan2(-playerState->cameraDirNextZ, -playerState->cameraDirNextX));
    const float relativeBearing =
        WrapForceFeedbackPolarRadians(kPi - (sourceBearing - playerBearing));
    return ForceFeedbackDirectionFromRadians(relativeBearing);
}

static void SetAndStartDirectionalForceFeedbackEffect(zInput_DiEffect *effect,
                                                      int direction, float gain) {
    LONG polarDirection[2] = {direction, 0};
    DIEFFECT desc = {0};
    desc.dwSize = sizeof(desc);
    desc.dwFlags = 0x20;
    desc.dwGain = static_cast<DWORD>(gain * 10000.0f);
    desc.cAxes = 2;
    desc.rglDirection = polarDirection;
    effect->vtbl_00->SetParameters_18(effect, &desc, 0x44);
    effect->vtbl_00->Start_1c(effect, 1, 0);
}

static float FastPitchLowpassFactor(float deltaTime) {
    int bits = static_cast<int>(deltaTime * -3.0f * 12102200.0f);
    bits += 0x3f800000;

    float factor = 0.0f;
    memcpy(&factor, &bits, sizeof(factor));
    return factor;
}

// Reimplements 0x42ffa0: zInput_DI_CreateConstantForceEffectScaled
RECOIL_NOINLINE zInput_DiEffect *RECOIL_STDCALL
zInput_DI_CreateConstantForceEffectScaled(float gain) {
    DWORD axes[2] = {0, 4};
    LONG direction[2] = {0, 0};
    DICONSTANTFORCE constantForce = {10000};
    DIEFFECT effect = {0};
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = 100000;
    effect.dwGain = static_cast<DWORD>(ClampForceFeedbackGain(gain) * 10000.0f);
    effect.dwTriggerButton = static_cast<DWORD>(-1);
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.cbTypeSpecificParams = sizeof(constantForce);
    effect.lpvTypeSpecificParams = &constantForce;
    return zInput_DI_CreateForceFeedbackEffect(&GUID_ConstantForce, &effect);
}

// Reimplements 0x430070: zInput_DI_CreateConstantForceEffectWithDirection
RECOIL_NOINLINE zInput_DiEffect *RECOIL_FASTCALL
zInput_DI_CreateConstantForceEffectWithDirection(int directionValue) {
    DWORD axes[2] = {0, 4};
    LONG direction[2] = {directionValue, 0};
    DICONSTANTFORCE constantForce = {10000};
    DIEFFECT effect = {0};
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = static_cast<DWORD>(-1);
    effect.dwTriggerButton = static_cast<DWORD>(-1);
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.cbTypeSpecificParams = sizeof(constantForce);
    effect.lpvTypeSpecificParams = &constantForce;
    return zInput_DI_CreateForceFeedbackEffect(&GUID_ConstantForce, &effect);
}

// Reimplements 0x430100: zInput_DI_CreateSineEffectScaled
RECOIL_NOINLINE zInput_DiEffect *RECOIL_STDCALL zInput_DI_CreateSineEffectScaled(float gain) {
    DWORD axes[2] = {0, 4};
    LONG direction[2] = {0, 0};
    DIPERIODIC periodic = {0};
    periodic.dwMagnitude = static_cast<DWORD>(ClampForceFeedbackGain(gain) * 10000.0f);
    periodic.dwPeriod = 20000;

    DIEFFECT effect = {0};
    effect.dwSize = sizeof(effect);
    effect.dwFlags = 0x22;
    effect.dwDuration = static_cast<DWORD>(-1);
    effect.dwGain = 10000;
    effect.dwTriggerButton = static_cast<DWORD>(-1);
    effect.cAxes = 2;
    effect.rgdwAxes = axes;
    effect.rglDirection = direction;
    effect.cbTypeSpecificParams = sizeof(periodic);
    effect.lpvTypeSpecificParams = &periodic;
    return zInput_DI_CreateForceFeedbackEffect(&GUID_Sine, &effect);
}

// Reimplements 0x42faa0: zInput_DI_RestartPrimaryFireEffect
RECOIL_NOINLINE void RECOIL_FASTCALL
zInput_DI_RestartPrimaryFireEffect(zInput_FFEffectSet *effectSet) {
    zInput_DiEffect *const effect = effectSet->PrimaryFire;
    if (effect == 0) {
        return;
    }

    effect->vtbl_00->Stop_20(effect);
    effectSet->PrimaryFire->vtbl_00->Start_1c(effectSet->PrimaryFire, 1, 0);
}

// Reimplements 0x42fac0: zInput_DI_PlayAltFireEffect
RECOIL_NOINLINE void RECOIL_FASTCALL zInput_DI_PlayAltFireEffect(zInput_FFEffectSet *effectSet,
                                                                 float gain) {
    zInput_DiEffect *const effect = effectSet->AltFire;
    if (effect == 0) {
        return;
    }

    struct DiEffectGainDesc {
        unsigned int dwSize;
        unsigned char unknown_04[0x0c];
        int dwGain;
        unsigned char unknown_14[0x20];
    };
    RECOIL_STATIC_ASSERT(sizeof(DiEffectGainDesc) == 0x34);
    RECOIL_STATIC_ASSERT(offsetof(DiEffectGainDesc, dwGain) == 0x10);

    DiEffectGainDesc desc = {0};
    effect->vtbl_00->Stop_20(effect);
    if (gain > 1.0f) {
        gain = 1.0f;
    }
    if (gain < 0.25f) {
        gain = 0.25f;
    }

    desc.dwSize = sizeof(desc);
    desc.dwGain = static_cast<int>(gain * 10000.0f);
    effectSet->AltFire->vtbl_00->SetParameters_18(effectSet->AltFire, &desc, 4);
    effectSet->AltFire->vtbl_00->Start_1c(effectSet->AltFire, 1, 0);
}

// Reimplements 0x42f9f0: zInput_DI_InitForceFeedbackEffectSet
RECOIL_NOINLINE zInput_FFEffectSet *RECOIL_FASTCALL
zInput_DI_InitForceFeedbackEffectSet(zInput_FFEffectSet *effectSet) {
    effectSet->PrimaryFire = zInput_DI_CreateConstantForceEffectScaled(0.25f);
    effectSet->AltFire = zInput_DI_CreateConstantForceEffectScaled(0.5f);
    effectSet->CollisionImpact = zInput_DI_CreateConstantForceEffectScaled(0.5f);
    effectSet->DamageHit = zInput_DI_CreateConstantForceEffectScaled(0.5f);
    effectSet->AmbientSine = zInput_DI_CreateSineEffectScaled(0.05f);
    effectSet->SteerForce = zInput_DI_CreateConstantForceEffectWithDirection(0x6978);
    effectSet->PitchForce = zInput_DI_CreateConstantForceEffectWithDirection(0x4650);

    if (effectSet->SteerForce != 0) {
        effectSet->SteerForce->vtbl_00->Start_1c(effectSet->SteerForce, 1, 0);
    }
    if (effectSet->PitchForce != 0) {
        effectSet->PitchForce->vtbl_00->Start_1c(effectSet->PitchForce, 1, 0);
    }

    return effectSet;
}

RECOIL_NOINLINE void RECOIL_CDECL zInput_DI_PlayCollisionImpactEffect_Impl(
    zInput_FFEffectSet *effectSet, const zVec3 *impactWorldPosXZ, float gain) {
    zInput_DiEffect *const effect = effectSet->CollisionImpact;
    if (effect == 0) {
        return;
    }

    effect->vtbl_00->Stop_20(effect);
    const int direction = ForceFeedbackDirectionFromImpact(impactWorldPosXZ, false);
    gain = ClampForceFeedbackGainRange(gain, 0.2f, 1.0f);
    SetAndStartDirectionalForceFeedbackEffect(effect, direction, gain);
}

RECOIL_NOINLINE void RECOIL_CDECL zInput_DI_PlayDamageHitEffect_Impl(
    zInput_FFEffectSet *effectSet, const zVec3 *damageSourceWorldPosXZ, float gain) {
    zInput_DiEffect *const effect = effectSet->DamageHit;
    if (effect == 0) {
        return;
    }

    effect->vtbl_00->Stop_20(effect);
    const int direction = ForceFeedbackDirectionFromImpact(damageSourceWorldPosXZ, true);
    gain = ClampForceFeedbackGainRange(gain, 0.25f, 1.0f);
    SetAndStartDirectionalForceFeedbackEffect(effect, direction, gain);
}

RECOIL_NOINLINE void RECOIL_CDECL zInput_DI_PlayCollisionImpactEffect(zInput_FFEffectSet *effectSet,
                                                                      const zVec3 *impactWorldPosXZ,
                                                                      float gain) {
    zInput_DI_PlayCollisionImpactEffect_Impl(effectSet, impactWorldPosXZ, gain);
}

RECOIL_NOINLINE void RECOIL_CDECL zInput_DI_PlayDamageHitEffect(zInput_FFEffectSet *effectSet,
                                                                const zVec3 *damageSourceWorldPosXZ,
                                                                float gain) {
    zInput_DI_PlayDamageHitEffect_Impl(effectSet, damageSourceWorldPosXZ, gain);
}

// Reimplements 0x42fdc0: zInput_DI_UpdateSteerAndPitchForceEffects
RECOIL_NOINLINE void RECOIL_FASTCALL
zInput_DI_UpdateSteerAndPitchForceEffects(zInput_FFEffectSet *effectSet) {
    zInput_PlayerStatePartial *const playerState = g_GameStateOrMapTable->playerState;

    zInput_DiEffect *const steerEffect = effectSet->SteerForce;
    if (steerEffect != 0) {
        float magnitude = playerState->angVelYaw / playerState->yawVelocityLimit;
        int direction = 0x6978;
        if (magnitude < 0.0f) {
            direction = 0x5a;
            magnitude = -magnitude;
        }

        magnitude = ClampForceFeedbackGainRange(magnitude, 0.0f, 0.75f);
        SetAndStartDirectionalForceFeedbackEffect(steerEffect, direction, magnitude);
    }

    zInput_DiEffect *const pitchEffect = effectSet->PitchForce;
    if (pitchEffect == 0) {
        return;
    }

    const float lowpassFactor = FastPitchLowpassFactor(g_Player_DeltaTime);
    g_zInput_DiPitchAngleLowpassRad = (g_zInput_DiPitchAngleLowpassRad * lowpassFactor) +
                                      ((1.0f - lowpassFactor) * playerState->pitchAngleRad);

    float residual = (playerState->pitchAngleRad - g_zInput_DiPitchAngleLowpassRad) * 8.0f;
    int direction = 0x4650;
    if (residual < 0.0f) {
        direction = 0;
        residual = -residual;
    }

    residual = ClampForceFeedbackGainRange(residual, 0.0f, 0.75f);
    SetAndStartDirectionalForceFeedbackEffect(pitchEffect, direction, residual);
}
}

namespace zInput {
namespace {
const int kDiOk = 0;
const int kDiFalse = 1;
const int kDiInputLost = static_cast<int>(0x8007001e);
const unsigned char kSuspendFlag = 2;
const unsigned int kDirectInputVersion = 0x500;
const char kZInputInitSourceFile[] = "D:\\Proj\\GameZRecoil\\zInput\\zin_init.cpp";
const char kZInputKeyboardSourceFile[] = "D:\\Proj\\GameZRecoil\\zInput\\zin_kbd.cpp";

struct DipropDwordInit {
    unsigned int dwSize;
    unsigned int dwHeaderSize;
    unsigned int dwObj;
    unsigned int dwHow;
    unsigned int dwData;
};

struct DirectInputErrorName {
    int hresult;
    const char *name;
};

const DirectInputErrorName kDirectInputErrorNames[] = {
    {static_cast<int>(0x80040110), "DIERR_NOAGGREGATION"},
    {static_cast<int>(0x80004001), "DIERR_UNSUPPORTED"},
    {static_cast<int>(0x80004002), "DIERR_NOINTERFACE"},
    {static_cast<int>(0x80004005), "DIERR_GENERIC"},
    {static_cast<int>(0x80070002), "DIERR_OBJECTNOTFOUND"},
    {static_cast<int>(0x80040154), "DIERR_DEVICENOTREG"},
    {static_cast<int>(0x8007000c), "DIERR_NOTACQUIRED"},
    {static_cast<int>(0x80070005), "DIERR_READONLY"},
    {static_cast<int>(0x80070015), "DIERR_NOTINITIALIZED"},
    {static_cast<int>(0x8007000e), "DIERR_OUTOFMEMORY"},
    {static_cast<int>(0x80070057), "DIERR_INVALIDPARAM"},
    {static_cast<int>(0x8007001e), "DIERR_INPUTLOST"},
    {static_cast<int>(0x800700aa), "DIERR_ACQUIRED"},
    {static_cast<int>(0x80070077), "DIERR_BADDRIVERVER"},
    {static_cast<int>(0x80070481), "DIERR_BETADIRECTINPUTVERSION"},
    {static_cast<int>(0x8007047e), "DIERR_OLDDIRECTINPUTVERSION"},
    {static_cast<int>(0x800704df), "DIERR_ALREADYINITIALIZED"},
};

const unsigned char kKeyboardModifierClasses[0x9c] = {
    0, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 1, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 2, 6, 3, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 4, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 5,
};

int IsUnsuspended(unsigned char flags) {
    return (~flags & kSuspendFlag) >> 1;
}

const char *GetDirectInputErrorName(int hresult) {
    {
        int entryIndex1;
        for (entryIndex1 = 0; entryIndex1 < (int)(sizeof(kDirectInputErrorNames) / sizeof((kDirectInputErrorNames)[0])); ++entryIndex1) {
            const DirectInputErrorName & entry = (kDirectInputErrorNames)[entryIndex1];
        if (entry.hresult == hresult) {
            return entry.name;
        }
    }
    }

    return "Unknown Error";
}

unsigned char ClassifyKeyboardOffset(unsigned int dwOfs) {
    const unsigned int tableIndex = dwOfs - 0x1d;
    if (tableIndex > 0x9b) {
        return 6;
    }

    return kKeyboardModifierClasses[tableIndex];
}

void UpdateKeyboardModifierState(int mask, bool pressed) {
    if (pressed) {
        g_zInput_KbdModifierState |= mask;
    } else {
        g_zInput_KbdModifierState &= ~mask;
    }
}

void ApplyKeyboardKeyEvent(DIDeviceObjectData &event) {
    if ((g_zInput_KbdModifierState & 0x100) != 0) {
        event.dwData |= 0x40;
    }
    if ((g_zInput_KbdModifierState & 0x200) != 0) {
        event.dwData |= 0x20;
    }
    if ((g_zInput_KbdModifierState & 0x400) != 0) {
        event.dwData |= 0x10;
    }

    const unsigned int dwOfs = event.dwOfs | static_cast<unsigned int>(g_zInput_KbdModifierState);
    KbdKeyDispatchEntry &dispatch = g_zInputKbdKeyDispatchTable[dwOfs];
    if ((event.dwData & 0x80) != 0) {
        dispatch.state = dispatch.state == 1 ? 3 : 1;
    } else {
        dispatch.state |= 4;
    }
}

typedef int (RECOIL_FASTCALL *KeyboardRawEventCallbackFn)(int ascii,
                                                                   void *context);
typedef void (RECOIL_FASTCALL *KeyboardComboCallbackFn)(int comboIdx);

int ApplyKeyboardWaitEvent(DIDeviceObjectData &event) {
    unsigned int dispatchIndex = event.dwOfs;
    bool returnPressedKey = false;
    switch (event.dwOfs) {
    case 0x38:
    case 0xb8:
        UpdateKeyboardModifierState(0x100, (event.dwData & 0x80) != 0);
        break;
    case 0x1d:
    case 0x9d:
        UpdateKeyboardModifierState(0x200, (event.dwData & 0x80) != 0);
        break;
    case 0x2a:
    case 0x36:
        UpdateKeyboardModifierState(0x400, (event.dwData & 0x80) != 0);
        break;
    default:
        returnPressedKey = true;
        if ((g_zInput_KbdModifierState & 0x100) != 0) {
            event.dwData |= 0x40;
        }
        if ((g_zInput_KbdModifierState & 0x200) != 0) {
            event.dwData |= 0x20;
        }
        if ((g_zInput_KbdModifierState & 0x400) != 0) {
            event.dwData |= 0x10;
        }
        dispatchIndex |= static_cast<unsigned int>(g_zInput_KbdModifierState);
        break;
    }

    KbdKeyDispatchEntry &dispatch = g_zInputKbdKeyDispatchTable[dispatchIndex];
    if ((event.dwData & 0x80) != 0) {
        dispatch.state = dispatch.state == 1 ? 3 : 1;
        return returnPressedKey ? static_cast<int>(dispatchIndex) : 0;
    }

    dispatch.state |= 4;
    return 0;
}

int ApplyKeyboardPollEvent(DIDeviceObjectData &event) {
    unsigned int dispatchIndex = event.dwOfs;
    switch (event.dwOfs) {
    case 0x38:
    case 0xb8:
        UpdateKeyboardModifierState(0x100, (event.dwData & 0x80) != 0);
        break;
    case 0x1d:
    case 0x9d:
        UpdateKeyboardModifierState(0x200, (event.dwData & 0x80) != 0);
        break;
    case 0x2a:
    case 0x36:
        UpdateKeyboardModifierState(0x400, (event.dwData & 0x80) != 0);
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
        dispatchIndex |= static_cast<unsigned int>(g_zInput_KbdModifierState);
        break;
    }

    KbdKeyDispatchEntry &dispatch = g_zInputKbdKeyDispatchTable[dispatchIndex];
    if ((event.dwData & 0x80) != 0) {
        dispatch.state = dispatch.state == 1 ? 3 : 1;
        if (g_zInput_KbdRawEventCallback != 0) {
            KeyboardRawEventCallbackFn callback = (KeyboardRawEventCallbackFn)(g_zInput_KbdRawEventCallback);
            callback(Keyboard_TranslateDikToAscii(static_cast<int>(dispatchIndex)),
                     g_zInput_KbdRawEventCallbackCtx);
        }
    } else {
        dispatch.state |= 4;
    }

    return static_cast<int>(dispatchIndex);
}

int KeyboardEventDispatchIndex(const DIDeviceObjectData &event) {
    int dispatchIndex = static_cast<int>(event.dwOfs);
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
} // namespace

// Reimplements 0x472490: zInput::DI_ReportError
RECOIL_NOINLINE RECOIL_NO_GS int RECOIL_FASTCALL DI_ReportError(int hresult,
                                                                         const char *sourceFile,
                                                                         int sourceLine) {
    if (hresult == 0) {
        return 1;
    }

    char errorNameBuffer[0x100];
    sprintf(errorNameBuffer, GetDirectInputErrorName(hresult));
    zError::ReportOld(0x800, sourceFile, sourceLine, "DirectInput Error [%s]\n", errorNameBuffer);
    return 0;
}

// Reimplements 0x471120: zInput::BindMap_InitDikKeyNameTable
RECOIL_NOINLINE void RECOIL_CDECL BindMap_InitDikKeyNameTable() {
    g_zInput_DikKeyNames[1] = "ESCAPE";
    g_zInput_DikKeyNames[2] = "1";
    g_zInput_DikKeyNames[3] = "2";
    g_zInput_DikKeyNames[4] = "3";
    g_zInput_DikKeyNames[5] = "4";
    g_zInput_DikKeyNames[6] = "5";
    g_zInput_DikKeyNames[7] = "6";
    g_zInput_DikKeyNames[8] = "7";
    g_zInput_DikKeyNames[9] = "8";
    g_zInput_DikKeyNames[0x0a] = "9";
    g_zInput_DikKeyNames[0x0b] = "0";
    g_zInput_DikKeyNames[0x0c] = "MINUS";
    g_zInput_DikKeyNames[0x0d] = "EQUALS";
    g_zInput_DikKeyNames[0x0e] = "BACK";
    g_zInput_DikKeyNames[0x0f] = "TAB";
    g_zInput_DikKeyNames[0x10] = "Q";
    g_zInput_DikKeyNames[0x11] = "W";
    g_zInput_DikKeyNames[0x12] = "E";
    g_zInput_DikKeyNames[0x13] = "R";
    g_zInput_DikKeyNames[0x14] = "T";
    g_zInput_DikKeyNames[0x15] = "Y";
    g_zInput_DikKeyNames[0x16] = "U";
    g_zInput_DikKeyNames[0x17] = "I";
    g_zInput_DikKeyNames[0x18] = "O";
    g_zInput_DikKeyNames[0x19] = "P";
    g_zInput_DikKeyNames[0x1a] = "LBRACKET";
    g_zInput_DikKeyNames[0x1b] = "RBRACKET";
    g_zInput_DikKeyNames[0x1c] = "RETURN";
    g_zInput_DikKeyNames[0x1d] = "LCONTROL";
    g_zInput_DikKeyNames[0x1e] = "A";
    g_zInput_DikKeyNames[0x1f] = "S";
    g_zInput_DikKeyNames[0x20] = "D";
    g_zInput_DikKeyNames[0x21] = "F";
    g_zInput_DikKeyNames[0x22] = "G";
    g_zInput_DikKeyNames[0x23] = "H";
    g_zInput_DikKeyNames[0x24] = "J";
    g_zInput_DikKeyNames[0x25] = "K";
    g_zInput_DikKeyNames[0x26] = "L";
    g_zInput_DikKeyNames[0x27] = "SEMICOLON";
    g_zInput_DikKeyNames[0x28] = "APOSTROPHE";
    g_zInput_DikKeyNames[0x29] = "GRAVE";
    g_zInput_DikKeyNames[0x2a] = "LSHIFT";
    g_zInput_DikKeyNames[0x2b] = "BACKSLASH";
    g_zInput_DikKeyNames[0x2c] = "Z";
    g_zInput_DikKeyNames[0x2d] = "X";
    g_zInput_DikKeyNames[0x2e] = "C";
    g_zInput_DikKeyNames[0x2f] = "V";
    g_zInput_DikKeyNames[0x30] = "B";
    g_zInput_DikKeyNames[0x31] = "N";
    g_zInput_DikKeyNames[0x32] = "M";
    g_zInput_DikKeyNames[0x33] = "COMMA";
    g_zInput_DikKeyNames[0x34] = "PERIOD";
    g_zInput_DikKeyNames[0x35] = "SLASH";
    g_zInput_DikKeyNames[0x36] = "RSHIFT";
    g_zInput_DikKeyNames[0x37] = "MULTIPLY";
    g_zInput_DikKeyNames[0x38] = "LMENU";
    g_zInput_DikKeyNames[0x39] = "SPACE";
    g_zInput_DikKeyNames[0x3a] = "CAPITAL";
    g_zInput_DikKeyNames[0x3b] = "F1";
    g_zInput_DikKeyNames[0x3c] = "F2";
    g_zInput_DikKeyNames[0x3d] = "F3";
    g_zInput_DikKeyNames[0x3e] = "F4";
    g_zInput_DikKeyNames[0x3f] = "F5";
    g_zInput_DikKeyNames[0x40] = "F6";
    g_zInput_DikKeyNames[0x41] = "F7";
    g_zInput_DikKeyNames[0x42] = "F8";
    g_zInput_DikKeyNames[0x43] = "F9";
    g_zInput_DikKeyNames[0x44] = "F10";
    g_zInput_DikKeyNames[0x45] = "NUMLOCK";
    g_zInput_DikKeyNames[0x46] = "SCROLL";
    g_zInput_DikKeyNames[0x47] = "NUMPAD7";
    g_zInput_DikKeyNames[0x48] = "NUMPAD8";
    g_zInput_DikKeyNames[0x49] = "NUMPAD9";
    g_zInput_DikKeyNames[0x4a] = "SUBTRACT";
    g_zInput_DikKeyNames[0x4b] = "NUMPAD4";
    g_zInput_DikKeyNames[0x4c] = "NUMPAD5";
    g_zInput_DikKeyNames[0x4d] = "NUMPAD6";
    g_zInput_DikKeyNames[0x4e] = "ADD";
    g_zInput_DikKeyNames[0x4f] = "NUMPAD1";
    g_zInput_DikKeyNames[0x50] = "NUMPAD2";
    g_zInput_DikKeyNames[0x51] = "NUMPAD3";
    g_zInput_DikKeyNames[0x52] = "NUMPAD0";
    g_zInput_DikKeyNames[0x53] = "DECIMAL";
    g_zInput_DikKeyNames[0x57] = "F11";
    g_zInput_DikKeyNames[0x58] = "F12";
    g_zInput_DikKeyNames[0x64] = "F13";
    g_zInput_DikKeyNames[0x65] = "F14";
    g_zInput_DikKeyNames[0x66] = "F15";
    g_zInput_DikKeyNames[0x70] = "KANA";
    g_zInput_DikKeyNames[0x79] = "CONVERT";
    g_zInput_DikKeyNames[0x7b] = "NOCONVERT";
    g_zInput_DikKeyNames[0x7d] = "YEN";
    g_zInput_DikKeyNames[0x8d] = "NUMPADEQUALS";
    g_zInput_DikKeyNames[0x90] = "CIRCUMFLEX";
    g_zInput_DikKeyNames[0x91] = "AT";
    g_zInput_DikKeyNames[0x92] = "COLON";
    g_zInput_DikKeyNames[0x93] = "UNDERLINE";
    g_zInput_DikKeyNames[0x94] = "KANJI";
    g_zInput_DikKeyNames[0x95] = "STOP";
    g_zInput_DikKeyNames[0x96] = "AX";
    g_zInput_DikKeyNames[0x97] = "UNLABELED";
    g_zInput_DikKeyNames[0x9c] = "NUMPADENTER";
    g_zInput_DikKeyNames[0x9d] = "RCONTROL";
    g_zInput_DikKeyNames[0xb3] = "NUMPADCOMMA";
    g_zInput_DikKeyNames[0xb5] = "DIVIDE";
    g_zInput_DikKeyNames[0xb7] = "SYSRQ";
    g_zInput_DikKeyNames[0xb8] = "RMENU";
    g_zInput_DikKeyNames[0xc7] = "HOME";
    g_zInput_DikKeyNames[0xc8] = "UP";
    g_zInput_DikKeyNames[0xc9] = "PRIOR";
    g_zInput_DikKeyNames[0xcb] = "LEFT";
    g_zInput_DikKeyNames[0xcd] = "RIGHT";
    g_zInput_DikKeyNames[0xcf] = "END";
    g_zInput_DikKeyNames[0xd0] = "DOWN";
    g_zInput_DikKeyNames[0xd1] = "NEXT";
    g_zInput_DikKeyNames[0xd2] = "INSERT";
    g_zInput_DikKeyNames[0xd3] = "DELETE";
    g_zInput_DikKeyNames[0xdb] = "LWIN";
    g_zInput_DikKeyNames[0xdc] = "RWIN";
    g_zInput_DikKeyNames[0xdd] = "APPS";
}

// Reimplements 0x4715e0: zInput::BindMap_InitJoystickButtonNameTable
RECOIL_NOINLINE void RECOIL_CDECL BindMap_InitJoystickButtonNameTable() {
    g_zInput_JoystickButtonNames[1] = "Button 1";
    g_zInput_JoystickButtonNames[2] = "Button 2";
    g_zInput_JoystickButtonNames[3] = "Button 3";
    g_zInput_JoystickButtonNames[4] = "Button 4";
    g_zInput_JoystickButtonNames[5] = "Button 5";
    g_zInput_JoystickButtonNames[6] = "Button 6";
    g_zInput_JoystickButtonNames[7] = "Button 7";
    g_zInput_JoystickButtonNames[8] = "Button 8";
}

// Reimplements 0x471640: zInput::BindMap_InitMouseButtonNameTable
RECOIL_NOINLINE void RECOIL_CDECL BindMap_InitMouseButtonNameTable() {
    g_zInput_MouseButtonNames[1] = "Left";
    g_zInput_MouseButtonNames[2] = "Right";
    g_zInput_MouseButtonNames[3] = "Middle";
}

// Reimplements 0x470a10: zInput::BindMap_PackBindingCode
RECOIL_NOINLINE int RECOIL_FASTCALL BindMap_PackBindingCode(int primary,
                                                                     int secondary,
                                                                     int joy,
                                                                     int mouse) {
    return (((mouse & 3) << 4 | (joy & 0x0f)) << 0x0b | (secondary & 0x7ff)) << 0x0b |
           (primary & 0x7ff);
}

// Reimplements 0x42a480: zInput::BindGroupList_GetCount
RECOIL_NOINLINE int RECOIL_CDECL BindGroupList_GetCount() {
    zInput_BindGroupInfo **const begin = g_zInput_BindGroupInfoList.begin;
    if (begin == 0) {
        return 0;
    }

    return static_cast<int>(g_zInput_BindGroupInfoList.end - begin);
}

// Reimplements 0x42a4a0: zInput::BindGroupList_GetGroupTitle
RECOIL_NOINLINE char *RECOIL_FASTCALL BindGroupList_GetGroupTitle(int groupIndex) {
    return g_zInput_BindGroupInfoList.begin[groupIndex]->title;
}

// Reimplements 0x42a4b0: zInput::BindGroupList_GetGroupCommandCount
RECOIL_NOINLINE int RECOIL_FASTCALL
BindGroupList_GetGroupCommandCount(int groupIndex) {
    zInput_BindGroupInfo *const group = g_zInput_BindGroupInfoList.begin[groupIndex];
    int *const begin = group->commandIdsBegin;
    if (begin == 0) {
        return 0;
    }

    return static_cast<int>(group->commandIdsEnd - begin);
}

// Reimplements 0x42a4d0: zInput::BindGroupList_GetGroupCommandId
RECOIL_NOINLINE int RECOIL_FASTCALL
BindGroupList_GetGroupCommandId(int groupIndex, int commandIndex) {
    return g_zInput_BindGroupInfoList.begin[groupIndex]->commandIdsBegin[commandIndex];
}

// Reimplements 0x42a000: zInput::BindGroupInfo_Destroy
RECOIL_NOINLINE void RECOIL_FASTCALL BindGroupInfo_Destroy(zInput_BindGroupInfo *group) {
    free(group->title);
    group->title = 0;
    ::operator delete(group->commandIdsBegin);
    group->commandIdsBegin = 0;
    group->commandIdsEnd = 0;
    group->commandIdsCapacity = 0;
}

// Reimplements 0x429f80: zInput::BindGroupList_Clear
RECOIL_NOINLINE void RECOIL_CDECL BindGroupList_Clear() {
    zInput_BindGroupInfo **cursor = g_zInput_BindGroupInfoList.begin;
    zInput_BindGroupInfo **const end = g_zInput_BindGroupInfoList.end;
    while (cursor != end) {
        zInput_BindGroupInfo *const group = *cursor;
        if (group != 0) {
            BindGroupInfo_Destroy(group);
            ::operator delete(group);
        }
        *cursor = 0;
        ++cursor;
    }

    g_zInput_BindGroupInfoList.end = g_zInput_BindGroupInfoList.begin;
}

// Reimplements 0x42a070: zInput::BindGroupList_AddGroup
RECOIL_NOINLINE int RECOIL_FASTCALL BindGroupList_AddGroup(const char *title) {
    const int groupIndex = BindGroupList_GetCount();

    zInput_BindGroupInfo * group = static_cast<zInput_BindGroupInfo *>(::operator new(sizeof(zInput_BindGroupInfo)));
    group->title = title != 0 ? _strdup(title) : 0;
    group->unknown_04 = 0;
    group->commandIdsBegin = 0;
    group->commandIdsEnd = 0;
    group->commandIdsCapacity = 0;

    zInput_BindGroupInfo **begin = g_zInput_BindGroupInfoList.begin;
    zInput_BindGroupInfo **end = g_zInput_BindGroupInfoList.end;
    zInput_BindGroupInfo **const capacity = g_zInput_BindGroupInfoList.capacity;
    if (end != 0 && capacity != 0 && capacity - end >= 1) {
        *end = group;
        g_zInput_BindGroupInfoList.end = end + 1;
        return groupIndex;
    }

    const int count = begin != 0 ? static_cast<int>(end - begin) : 0;
    const int growth = count > 1 ? count : 1;
    const int newCapacity = count + growth;
    zInput_BindGroupInfo **const newBegin = static_cast<zInput_BindGroupInfo **>(
        ::operator new(static_cast<size_t>(newCapacity) * sizeof(zInput_BindGroupInfo *)));

    for (int i = 0; i < count; ++i) {
        newBegin[i] = begin[i];
    }
    newBegin[count] = group;

    ::operator delete(begin);
    g_zInput_BindGroupInfoList.begin = newBegin;
    g_zInput_BindGroupInfoList.end = newBegin + count + 1;
    g_zInput_BindGroupInfoList.capacity = newBegin + newCapacity;
    return groupIndex;
}

// Reimplements 0x42a2c0: zInput::BindGroupList_AddCommandToGroup
RECOIL_NOINLINE void RECOIL_FASTCALL BindGroupList_AddCommandToGroup(int groupIndex,
                                                                     int commandId) {
    zInput_BindGroupInfo *const group = g_zInput_BindGroupInfoList.begin[groupIndex];

    int *begin = group->commandIdsBegin;
    int *end = group->commandIdsEnd;
    int *const capacity = group->commandIdsCapacity;
    if (end != 0 && capacity != 0 && capacity - end >= 1) {
        *end = commandId;
        group->commandIdsEnd = end + 1;
        return;
    }

    const int count = begin != 0 ? static_cast<int>(end - begin) : 0;
    const int growth = count > 1 ? count : 1;
    const int newCapacity = count + growth;
    int *const newBegin = static_cast<int *>(
        ::operator new(static_cast<size_t>(newCapacity) * sizeof(int)));

    for (int i = 0; i < count; ++i) {
        newBegin[i] = begin[i];
    }
    newBegin[count] = commandId;

    ::operator delete(begin);
    group->commandIdsBegin = newBegin;
    group->commandIdsEnd = newBegin + count + 1;
    group->commandIdsCapacity = newBegin + newCapacity;
}

// Reimplements 0x42a4e0: zInput::BindMap_GetCommandLabel
RECOIL_NOINLINE char *RECOIL_FASTCALL BindMap_GetCommandLabel(int commandId) {
    return zLoc::GetMessageString(g_zInput_CommandLocIdTable[commandId]);
}

// Reimplements 0x42a4f0: zInput::BindMap_GetCommandHint
RECOIL_NOINLINE char *RECOIL_FASTCALL BindMap_GetCommandHint(int commandId) {
    return zLoc::GetMessageString(g_zInput_CommandLocIdTable[commandId] + 1);
}

// Reimplements 0x42a500: zInput::BindMap_AddDefaultBinding
RECOIL_NOINLINE void RECOIL_FASTCALL BindMap_AddDefaultBinding(
    int commandId, int messageId, int primaryKey,
    int secondaryKey, int joystickSlot, int mouseSlot) {
    const int boundCommandId =
        BindMap_Current_SetBindingRecord(commandId, zLoc::GetMessageString(messageId), primaryKey,
                                         secondaryKey, joystickSlot, mouseSlot);
    BindGroupList_AddCommandToGroup(g_zInput_CurrentBindGroupIndex, boundCommandId);
    g_zInput_CommandLocIdTable[commandId] = messageId;
}

// Reimplements 0x42a550: zInput::BindMap_InitDefaultBindings
RECOIL_NOINLINE int RECOIL_CDECL BindMap_InitDefaultBindings() {
    static const BindMapDefaultBindingSpec kGroup0[] = {
        {0x04, 0x806, 0x0c8, 0, 0, 0}, {0x01, 0x800, 0x0d0, 0, 0, 0}, {0x02, 0x802, 0x0cb, 0, 0, 0},
        {0x03, 0x804, 0x0cd, 0, 0, 0}, {0x2b, 0x8c6, 0x01f, 0, 0, 0}, {0x25, 0x874, 0x03b, 0, 0, 0},
        {0x26, 0x876, 0x03c, 0, 0, 0}, {0x27, 0x878, 0x03d, 0, 0, 0}, {0x28, 0x87a, 0x03e, 0, 0, 0},
        {0x05, 0x80e, 0x01e, 0, 0, 0}, {0x06, 0x810, 0x02c, 0, 0, 0}, {0x07, 0x82a, 0x02e, 0, 6, 0},
        {0x08, 0x82c, 0x02b, 0, 5, 0}, {0x09, 0x872, 0x030, 0, 0, 0}, {0x0a, 0x8c2, 0x230, 0, 0, 0},
    };
    static const BindMapDefaultBindingSpec kGroup1[] = {
        {0x0b, 0x88c, 0, 0, 1, 1},         {0x0c, 0x88e, 0, 0, 2, 2},
        {0x0d, 0x8b8, 0x039, 0, 3, 0},     {0x0f, 0x812, 0x002, 0x04f, 0, 0},
        {0x10, 0x814, 0x003, 0x050, 0, 0}, {0x11, 0x816, 0x004, 0x051, 0, 0},
        {0x12, 0x818, 0x005, 0x04b, 0, 0}, {0x13, 0x81a, 0x006, 0x04c, 0, 0},
        {0x14, 0x81c, 0x007, 0x04d, 0, 0}, {0x15, 0x81e, 0x008, 0x047, 0, 0},
        {0x16, 0x820, 0x009, 0x048, 0, 0}, {0x17, 0x822, 0x00a, 0x049, 0, 0},
    };
    static const BindMapDefaultBindingSpec kGroup2[] = {
        {0x1e, 0x84e, 0x02f, 0, 0, 0},
        {0x20, 0x888, 0x03f, 0, 0, 0},
        {0x21, 0x8a6, 0x040, 0, 0, 0},
        {0x22, 0x8a8, 0x041, 0, 0, 0},
    };
    static const BindMapDefaultBindingSpec kGroup3[] = {
        {0x19, 0x8a4, 0x013, 0, 0, 0},
        {0x18, 0x826, 0x018, 0, 0, 0},
        {0x1a, 0x8c4, 0x011, 0, 0, 0},
    };
    static const BindMapDefaultBindingSpec kGroup4[] = {
        {0x2d, 0x8b6, 0x042, 0, 0, 0}, {0x2c, 0x8b4, 0x043, 0, 0, 0}, {0x2a, 0x8bc, 0x014, 0, 0, 0},
        {0x1b, 0x864, 0x032, 0, 0, 0}, {0x1c, 0x866, 0x034, 0, 0, 0}, {0x1d, 0x868, 0x033, 0, 0, 0},
        {0x23, 0x88a, 0x22d, 0, 0, 0},
    };

    struct BindMapDefaultGroupSpec {
        int titleMessageId;
        const BindMapDefaultBindingSpec *bindings;
        size_t bindingCount;
    };
    static const BindMapDefaultGroupSpec kGroups[] = {
        {0x750, kGroup0, sizeof(kGroup0) / sizeof(kGroup0[0])},
        {0x751, kGroup1, sizeof(kGroup1) / sizeof(kGroup1[0])},
        {0x752, kGroup2, sizeof(kGroup2) / sizeof(kGroup2[0])},
        {0x753, kGroup3, sizeof(kGroup3) / sizeof(kGroup3[0])},
        {0x754, kGroup4, sizeof(kGroup4) / sizeof(kGroup4[0])},
    };

    {
        int groupIndex2;
        for (groupIndex2 = 0; groupIndex2 < (int)(sizeof(kGroups) / sizeof((kGroups)[0])); ++groupIndex2) {
            const BindMapDefaultGroupSpec & group = (kGroups)[groupIndex2];
        g_zInput_CurrentBindGroupIndex =
            BindGroupList_AddGroup(zLoc::GetMessageString(group.titleMessageId));
        for (size_t i = 0; i < group.bindingCount; ++i) {
            const BindMapDefaultBindingSpec &binding = group.bindings[i];
            BindMap_AddDefaultBinding(binding.commandId, binding.messageId, binding.primaryKey,
                                      binding.secondaryKey, binding.joystickSlot,
                                      binding.mouseSlot);
        }
    }
    }

    BindMap_Current_SetBindingRecord(0x24, zLoc::GetMessageString(0x83c), 0x418, 0, 0, 0);
    BindMap_Current_SetBindingRecord(0x1f, zLoc::GetMessageString(0x850), 0x22, 0, 0, 0);
    return 1;
}

// Reimplements 0x4710a0: zInput::BindMapSystem_Init
RECOIL_NOINLINE void RECOIL_FASTCALL BindMapSystem_Init(int commandCount) {
    zInput_BindMapContext *context = new zInput_BindMapContext;
    if (context != 0) {
        context = context->InitFromTemplate(0);
    }

    g_zInput_BindMap_Current = context;
    context->InitCommandMap(commandCount);
    BindMap_InitDikKeyNameTable();
    BindMap_InitJoystickButtonNameTable();
    BindMap_InitMouseButtonNameTable();
}

// Reimplements 0x471860: zInput::BindMapContext_Push
RECOIL_NOINLINE void RECOIL_FASTCALL BindMapContext_Push(zInput_BindMapContext *bindMapOrNull) {
    zInput_BindMapContext *bindMap = bindMapOrNull;
    if (bindMap == 0) {
        bindMap = new zInput_BindMapContext;
        if (bindMap != 0) {
            bindMap = bindMap->InitFromTemplate(g_zInput_BindMap_Current);
        }
        bindMap->m_isOverlay = 1;
    }

    zInput_BindMapOverlayStackNode *node = g_zInput_BindMapOverlayNodeFreeList;
    const zInput_BindMapContext *previousCurrent = g_zInput_BindMap_Current;
    if (node == 0) {
        node = new zInput_BindMapOverlayStackNode;
        if (node != 0) {
            node->prev = 0;
            node->next = 0;
            node->bindMap = 0;
        }
    } else {
        zInput_BindMapOverlayStackNode *next = node->next;
        g_zInput_BindMapOverlayNodeFreeList = next;
        if (next != 0) {
            next->prev = 0;
        }
        node->next = 0;
    }

    node->bindMap = const_cast<zInput_BindMapContext *>(previousCurrent);
    node->next = g_zInput_BindMapOverlayNodeStackHead;
    node->prev = 0;
    if (g_zInput_BindMapOverlayNodeStackHead != 0) {
        g_zInput_BindMapOverlayNodeStackHead->prev = node;
    }
    g_zInput_BindMapOverlayNodeStackHead = node;
    ++g_zInput_BindMapOverlayDepth;
    g_zInput_BindMap_Current = bindMap;
    bindMap->RebuildLookupIndices();
}

// Reimplements 0x471950: zInput::BindMapContext_Pop
RECOIL_NOINLINE void RECOIL_CDECL BindMapContext_Pop() {
    zInput_BindMapContext *current = g_zInput_BindMap_Current;
    if (current->m_isOverlay != 0 && current != 0) {
        current->FreeAllBuffers();
        operator delete(current);
    }

    zInput_BindMapOverlayStackNode *node = g_zInput_BindMapOverlayNodeStackHead;
    if (node != 0) {
        zInput_BindMapOverlayStackNode *next = node->next;
        g_zInput_BindMapOverlayNodeStackHead = next;
        if (next != 0) {
            next->prev = 0;
        }
        node->prev = 0;
        node->next = 0;
    } else {
        node = 0;
    }

    zInput_BindMapContext *bindMap = 0;
    if (node != 0) {
        bindMap = node->bindMap;
        node->bindMap = 0;
        if (g_zInput_BindMapOverlayNodeFreeList != 0) {
            node->next = g_zInput_BindMapOverlayNodeFreeList;
            node->prev = 0;
            g_zInput_BindMapOverlayNodeFreeList->prev = node;
            g_zInput_BindMapOverlayNodeFreeList = node;
        } else {
            g_zInput_BindMapOverlayNodeFreeList = node;
            node->next = 0;
            node->prev = 0;
        }
        --g_zInput_BindMapOverlayDepth;
    }

    g_zInput_BindMap_Current = bindMap;
    bindMap->RebuildLookupIndices();
}

// Reimplements 0x471660: zInput::BindMapSystem_Shutdown
RECOIL_NOINLINE void RECOIL_CDECL BindMapSystem_Shutdown() {
    zInput_BindMapContext *current = g_zInput_BindMap_Current;
    while (current->m_isOverlay != 0) {
        BindMapContext_Pop();
        current = g_zInput_BindMap_Current;
    }

    if (current == 0) {
        return;
    }

    current->FreeNonOwnedBuffers();
    current = g_zInput_BindMap_Current;
    if (current != 0) {
        current->FreeAllBuffers();
        operator delete(current);
    }
}

// Reimplements 0x4716b0: zInput::BindMap_Current_RebuildLookupIndices
RECOIL_NOINLINE void RECOIL_CDECL BindMap_Current_RebuildLookupIndices() {
    g_zInput_BindMap_Current->RebuildLookupIndices();
}

// Reimplements 0x4716c0: zInput::BindMapCurrent_ResetAllBindings
RECOIL_NOINLINE void RECOIL_CDECL BindMapCurrent_ResetAllBindings() {
    g_zInput_BindMap_Current->ResetAllBindings();
}

// Reimplements 0x4716d0: zInput::BindMapCurrent_GetPrimaryKeyboardKey
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMapCurrent_GetPrimaryKeyboardKey(int commandIndex) {
    return g_zInput_BindMap_Current->GetPrimaryKeyboardKey(commandIndex);
}

// Reimplements 0x4716e0: zInput::BindMapCurrent_GetSecondaryKeyboardKey
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMapCurrent_GetSecondaryKeyboardKey(int commandIndex) {
    return g_zInput_BindMap_Current->GetSecondaryKeyboardKey(commandIndex);
}

// Reimplements 0x4716f0: zInput::BindMapCurrent_GetJoystickButtonSlot
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMapCurrent_GetJoystickButtonSlot(int commandIndex) {
    return g_zInput_BindMap_Current->GetJoystickButtonSlot(commandIndex);
}

// Reimplements 0x471700: zInput::BindMapCurrent_GetMouseButtonSlot
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMapCurrent_GetMouseButtonSlot(int commandIndex) {
    return g_zInput_BindMap_Current->GetMouseButtonSlot(commandIndex);
}

// Reimplements 0x471710: zInput::BindMapCurrent_GetCommandByPrimaryKey
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMapCurrent_GetCommandByPrimaryKey(int keyboardKey) {
    return g_zInput_BindMap_Current->GetCommandByPrimaryKey(keyboardKey);
}

// Reimplements 0x471720: zInput::BindMapCurrent_GetCommandBySecondaryKey
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMapCurrent_GetCommandBySecondaryKey(int keyboardKey) {
    return g_zInput_BindMap_Current->GetCommandBySecondaryKey(keyboardKey);
}

// Reimplements 0x471730: zInput::BindMapCurrent_GetCommandByJoystickSlot
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMapCurrent_GetCommandByJoystickSlot(int joystickSlot) {
    return g_zInput_BindMap_Current->GetCommandByJoystickSlot(joystickSlot);
}

// Reimplements 0x471740: zInput::BindMapCurrent_GetCommandByMouseSlot
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMapCurrent_GetCommandByMouseSlot(int mouseSlot) {
    return g_zInput_BindMap_Current->GetCommandByMouseSlot(mouseSlot);
}

// Reimplements 0x471750: zInput::BindMapCurrent_SetPrimaryKeyBinding
RECOIL_NOINLINE void RECOIL_FASTCALL BindMapCurrent_SetPrimaryKeyBinding(int keyCode,
                                                                         int commandId) {
    g_zInput_BindMap_Current->SetPrimaryKeyBinding(keyCode, commandId);
}

// Reimplements 0x471760: zInput::BindMapCurrent_SetSecondaryKeyBinding
RECOIL_NOINLINE void RECOIL_FASTCALL BindMapCurrent_SetSecondaryKeyBinding(int keyCode,
                                                                           int commandId) {
    g_zInput_BindMap_Current->SetSecondaryKeyBinding(keyCode, commandId);
}

// Reimplements 0x471770: zInput::BindMapCurrent_SetJoystickBinding
RECOIL_NOINLINE void RECOIL_FASTCALL BindMapCurrent_SetJoystickBinding(int joystickSlot,
                                                                       int commandId) {
    g_zInput_BindMap_Current->SetJoystickBinding(joystickSlot, commandId);
}

// Reimplements 0x471780: zInput::BindMapCurrent_SetMouseBinding
RECOIL_NOINLINE void RECOIL_FASTCALL BindMapCurrent_SetMouseBinding(int mouseSlot,
                                                                    int commandId) {
    g_zInput_BindMap_Current->SetMouseBinding(mouseSlot, commandId);
}

// Reimplements 0x471790: zInput::BindMap_Current_SetBindingRecord
RECOIL_NOINLINE int RECOIL_FASTCALL BindMap_Current_SetBindingRecord(
    int commandId, const char *labelSrc, int primaryKey,
    int secondaryKey, int joystickSlot, int mouseSlot) {
    g_zInput_BindMap_Current->SetBindingRecord(commandId, labelSrc, primaryKey, secondaryKey,
                                               joystickSlot, mouseSlot);
    return commandId;
}

// Reimplements 0x4717c0: zInput::BindMap_Current_SetCommandCallback
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMap_Current_SetCommandCallback(int commandId, zInputCommandCallbackFn callback) {
    return g_zInput_BindMap_Current->SetCommandCallback(commandId, callback);
}

// Reimplements 0x4717d0: zInput::BindMap_Current_ReadCommandInputState
RECOIL_NOINLINE int RECOIL_FASTCALL
BindMap_Current_ReadCommandInputState(int commandIndex) {
    return g_zInput_BindMap_Current->ReadCommandInputState(commandIndex);
}

// Reimplements 0x4717e0: zInput::BindMapCurrent_CopyCommandLabel
RECOIL_NOINLINE char *RECOIL_FASTCALL BindMapCurrent_CopyCommandLabel(int commandId,
                                                                      char *destBuf,
                                                                      int maxBytes) {
    return g_zInput_BindMap_Current->CopyCommandLabel(commandId, destBuf, maxBytes);
}

// Reimplements 0x470f80: zInput::BindMap_FormatKeyComboName
RECOIL_NOINLINE char *RECOIL_STDCALL BindMap_FormatKeyComboName(int packedKey,
                                                                char *destBuf,
                                                                int maxBytes) {
    const char *keyName = g_zInput_DikKeyNames[packedKey & 0xff];
    if (keyName == 0) {
        return g_zInput_EmptyName;
    }

    int remaining = maxBytes;
    *destBuf = '\0';
    if ((packedKey & 0x200) != 0) {
        strncat(destBuf, "Ctrl-", remaining);
        remaining -= static_cast<int>(strlen(destBuf));
    }
    if ((packedKey & 0x100) != 0) {
        strncat(destBuf, "Alt-", remaining);
        remaining -= static_cast<int>(strlen(destBuf));
    }
    if ((packedKey & 0x400) != 0) {
        strncat(destBuf, "Shift-", remaining);
        remaining -= static_cast<int>(strlen(destBuf));
    }

    return strncat(destBuf, keyName, remaining);
}

// Reimplements 0x471040: zInput::BindMap_CopyJoystickButtonName
RECOIL_NOINLINE char *RECOIL_STDCALL BindMap_CopyJoystickButtonName(int joystickSlot,
                                                                    char *outBuf,
                                                                    int bufSize) {
    const char *source = g_zInput_JoystickButtonNames[joystickSlot];
    if (source == 0) {
        return g_zInput_EmptyName;
    }

    return strncpy(outBuf, source, bufSize);
}

// Reimplements 0x471070: zInput::BindMap_CopyMouseButtonName
RECOIL_NOINLINE char *RECOIL_STDCALL BindMap_CopyMouseButtonName(int mouseSlot,
                                                                 char *outBuf,
                                                                 int bufSize) {
    const char *source = g_zInput_MouseButtonNames[mouseSlot];
    if (source == 0) {
        return g_zInput_EmptyName;
    }

    return strncpy(outBuf, source, bufSize);
}

// Reimplements 0x471800: zInput::BindMapCurrent_FormatKeyComboName
RECOIL_NOINLINE char *RECOIL_FASTCALL BindMapCurrent_FormatKeyComboName(int packedKey,
                                                                        char *destBuf,
                                                                        int maxBytes) {
    return BindMap_FormatKeyComboName(packedKey, destBuf, maxBytes);
}

// Reimplements 0x471820: zInput::BindMapCurrent_CopyJoystickButtonName
RECOIL_NOINLINE char *RECOIL_FASTCALL BindMapCurrent_CopyJoystickButtonName(
    int joystickSlot, char *outBuf, int bufSize) {
    return BindMap_CopyJoystickButtonName(joystickSlot, outBuf, bufSize);
}

// Reimplements 0x471840: zInput::BindMapCurrent_CopyMouseButtonName
RECOIL_NOINLINE char *RECOIL_FASTCALL BindMapCurrent_CopyMouseButtonName(int mouseSlot,
                                                                         char *outBuf,
                                                                         int bufSize) {
    return BindMap_CopyMouseButtonName(mouseSlot, outBuf, bufSize);
}

// Reimplements 0x471b50: zInput::Init
RECOIL_NOINLINE int RECOIL_FASTCALL Init(HWND hWnd, HINSTANCE hInstance) {
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

    const HRESULT hr =
        DirectInputCreateA(hInstance, kDirectInputVersion,
                           (LPDIRECTINPUTA *)(&g_zInput_GlobalState), 0);
    if (hr != 0) {
        DI_ReportError(hr, kZInputInitSourceFile, 0x93);
        return -1;
    }

    g_zInput_hWnd = hWnd;
    g_zInput_DeviceRegistry = Keyboard_InitDevice() == 0 ? static_cast<unsigned char>(1) : 0;
    g_zInputMouseFlags = Mouse_InitDevice() != 0 ? static_cast<unsigned char>(1) : 0;
    g_zInputJoystickFlags = DI_InitJoystickDevice(hWnd) != 0 ? 1 : 0;
    Keyboard_AddRef();
    Mouse_AddRef();
    return 0;
}

// Reimplements 0x472280: zInput::Joystick_ShutdownDevice
RECOIL_NOINLINE int RECOIL_CDECL Joystick_ShutdownDevice() {
    DIDevice *const joystick = g_zInput_JoystickDevice;
    if (joystick != 0) {
        joystick->vtbl_00->Unacquire_20(joystick);
        g_zInput_JoystickDevice->vtbl_00->Release_08(g_zInput_JoystickDevice);
        g_zInput_JoystickDevice = 0;
    }

    return 1;
}

// Reimplements 0x46f420: zInput::Keyboard_ShutdownDevice
RECOIL_NOINLINE int RECOIL_CDECL Keyboard_ShutdownDevice() {
    DIDevice *const keyboard = g_zInput_KbdDevice;
    if (keyboard != 0) {
        keyboard->vtbl_00->Unacquire_20(keyboard);
        g_zInput_KbdDevice->vtbl_00->Release_08(g_zInput_KbdDevice);
    }

    if (g_zInput_KbdEventBuffer != 0) {
        free(g_zInput_KbdEventBuffer);
    }

    return 0;
}

// Reimplements 0x471c10: zInput::Shutdown
RECOIL_NOINLINE int RECOIL_CDECL Shutdown() {
    if (g_zInput_hWnd == 0) {
        return 1;
    }

    Joystick_ShutdownDevice();
    Keyboard_ShutdownDevice();
    Mouse_ShutdownDevice();

    if (g_zInput_GlobalState != 0) {
        g_zInput_GlobalState->vtbl_00->Release_08(g_zInput_GlobalState);
    }

    g_zInput_hWnd = 0;
    return 0;
}

// Reimplements 0x470670: zInput::Mouse_SetCooperativeLevelFlags
RECOIL_NOINLINE int RECOIL_FASTCALL Mouse_SetCooperativeLevelFlags(int flags) {
    const int previousFlags = g_zInput_MouseCoopLevelFlags;
    g_zInput_MouseCoopLevelFlags = flags;
    return previousFlags;
}

// Reimplements 0x4702e0: zInput::Mouse_GetButtonTransitionState
RECOIL_NOINLINE int RECOIL_FASTCALL
Mouse_GetButtonTransitionState(int buttonNumber) {
    const unsigned char *currentButtons =
        (const unsigned char *)(&g_zInput_MouseCurrentState.rgbButtons);
    const unsigned char *previousButtons =
        (const unsigned char *)(&g_zInput_MousePreviousState.rgbButtons);

    const unsigned char current = currentButtons[buttonNumber - 1];
    const unsigned char previous = previousButtons[buttonNumber - 1];
    if (current != 0) {
        return (previous != 0 ? 1 : 0) + 1;
    }

    return previous != 0 ? 4 : 0;
}

// Reimplements 0x470020: zInput::Mouse_ApplyClientCursorPosToOS
RECOIL_NOINLINE void RECOIL_CDECL Mouse_ApplyClientCursorPosToOS() {
    POINT point;
    point.x = g_zInput_MouseStateSnapshot.cursorClientX;
    point.y = g_zInput_MouseStateSnapshot.cursorClientY;
    ClientToScreen(g_zInput_hWnd, &point);
    SetCursorPos(point.x, point.y);
}

// Reimplements 0x470060: zInput::Mouse_UpdateClientRectAndCenter
RECOIL_NOINLINE void RECOIL_CDECL Mouse_UpdateClientRectAndCenter() {
    RECT rect;
    GetClientRect(g_zInput_hWnd, &rect);
    g_zInput_MouseClientWidth = rect.right;
    g_zInput_MouseClientHeight = rect.bottom;
    Mouse_SetClientSizeAndCenter(rect.right, rect.bottom);
}

// Reimplements 0x470150: zInput::Mouse_RecenterCursor
RECOIL_NOINLINE void RECOIL_CDECL Mouse_RecenterCursor() {
    g_zInput_MouseStateSnapshot.cursorClientX = g_zInput_MouseClientCenterX;
    g_zInput_MouseStateSnapshot.cursorClientY = g_zInput_MouseClientCenterY;
    g_zInput_MouseStateSnapshot.cursorNormX = 0.0f;
    g_zInput_MouseStateSnapshot.cursorNormY = 0.0f;
    Mouse_ApplyClientCursorPosToOS();
}

// Reimplements 0x470180: zInput::Mouse_RecenterCursorX
RECOIL_NOINLINE void RECOIL_CDECL Mouse_RecenterCursorX() {
    g_zInput_MouseStateSnapshot.cursorClientX = g_zInput_MouseClientCenterX;
    Mouse_ApplyClientCursorPosToOS();
}

// Reimplements 0x4700a0: zInput::Mouse_SetNormalizedCursorPos
RECOIL_NOINLINE void RECOIL_STDCALL Mouse_SetNormalizedCursorPos(float normX, float normY) {
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
        g_zInput_MouseClientCenterX +
        static_cast<int>(g_zInput_MouseClientCenterX * normX);
    g_zInput_MouseStateSnapshot.cursorClientY =
        g_zInput_MouseClientCenterY +
        static_cast<int>(g_zInput_MouseClientCenterY * normY);
    Mouse_ApplyClientCursorPosToOS();
}

// Reimplements 0x470190: zInput::Mouse_IsInitialized
RECOIL_NOINLINE int RECOIL_CDECL Mouse_IsInitialized() {
    return g_zInput_MouseInitialized;
}

// Reimplements 0x4701a0: zInput::Mouse_SetClientSizeAndCenter
RECOIL_NOINLINE void RECOIL_FASTCALL Mouse_SetClientSizeAndCenter(int width,
                                                                  int height) {
    g_zInput_MouseClientWidth = width;
    g_zInput_MouseClientHeight = height;
    g_zInput_MouseClientCenterX = (width - (width >> 31)) >> 1;
    g_zInput_MouseClientCenterY = (height - (height >> 31)) >> 1;
    g_zInput_MouseInvClientCenterX = 1.0f / static_cast<float>(g_zInput_MouseClientCenterX);
    g_zInput_MouseInvClientCenterY = 1.0f / static_cast<float>(g_zInput_MouseClientCenterY);
}

// Reimplements 0x4703a0: zInput::Mouse_GetStateSnapshotPtr
RECOIL_NOINLINE MouseStateSnapshot *RECOIL_CDECL Mouse_GetStateSnapshotPtr() {
    return &g_zInput_MouseStateSnapshot;
}

// Reimplements 0x4705f0: zInput::Mouse_GetStateSnapshot
RECOIL_NOINLINE int RECOIL_FASTCALL Mouse_GetStateSnapshot(MouseStateSnapshot *outState) {
    if (outState != 0) {
        memcpy(outState, &g_zInput_MouseStateSnapshot, 0x2c);
    }

    return g_zInputMouseLastPollResult;
}

// Reimplements 0x4701f0: zInput::Mouse_InitDevice
RECOIL_NOINLINE int RECOIL_CDECL Mouse_InitDevice() {
    DIDevice *baseDevice = 0;
    g_zInput_GlobalState->vtbl_00->CreateDevice_0c(g_zInput_GlobalState, &GUID_SysMouse,
                                                   &baseDevice, 0);
    baseDevice->vtbl_00->QueryInterface_00(baseDevice, &IID_IDirectInputDevice2A,
                                           &g_zInput_MouseDevice);
    baseDevice->vtbl_00->Release_08(baseDevice);

    g_zInput_MouseDevice->vtbl_00->SetDataFormat_2c(g_zInput_MouseDevice, &c_dfDIMouse);
    g_zInput_MouseDevice->vtbl_00->SetCooperativeLevel_34(
        g_zInput_MouseDevice, g_zInput_hWnd,
        static_cast<unsigned int>(g_zInput_MouseCoopLevelFlags));

    DipropDwordInit bufferSizeProp = {0x14, 0x10, 0, 0, 0x10};
    g_zInput_MouseDevice->vtbl_00->SetProperty_18(g_zInput_MouseDevice, 1, &bufferSizeProp);

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

// Reimplements 0x46f300: zInput::Keyboard_InitDevice
RECOIL_NOINLINE int RECOIL_CDECL Keyboard_InitDevice() {
    DipropDwordInit bufferSizeProp = {0x14, 0x10, 0, 0, 0x80};
    g_zInput_KbdSystemReady = 0;
    g_zInput_KbdDevice = 0;
    g_zInput_KbdEventBuffer = 0;
    g_zInput_KbdModifierState = 0;
    g_zInput_KbdRawEventCallback = 0;
    g_zInput_KbdRawEventCallbackCtx = 0;

    int hr = g_zInput_GlobalState->vtbl_00->CreateDevice_0c(
        g_zInput_GlobalState, &GUID_SysKeyboard, &g_zInput_KbdDevice, 0);
    if (hr != 0) {
        DI_ReportError(hr, kZInputKeyboardSourceFile, 0x95);
        return 1;
    }

    hr =
        g_zInput_KbdDevice->vtbl_00->SetCooperativeLevel_34(g_zInput_KbdDevice, g_zInput_hWnd, 0xa);
    if (hr != 0) {
        DI_ReportError(hr, kZInputKeyboardSourceFile, 0x9d);
        return 1;
    }

    hr = g_zInput_KbdDevice->vtbl_00->SetProperty_18(g_zInput_KbdDevice, 1, &bufferSizeProp);
    if (hr != 0) {
        DI_ReportError(hr, kZInputKeyboardSourceFile, 0xa5);
        return 1;
    }

    hr = g_zInput_KbdDevice->vtbl_00->SetDataFormat_2c(g_zInput_KbdDevice, &c_dfDIKeyboard);
    if (hr != 0) {
        DI_ReportError(hr, kZInputKeyboardSourceFile, 0xad);
        return 1;
    }

    hr = g_zInput_KbdDevice->vtbl_00->Acquire_1c(g_zInput_KbdDevice);
    if (hr != 0) {
        DI_ReportError(hr, kZInputKeyboardSourceFile, 0xb6);
        return 1;
    }

    g_zInput_KbdEventBuffer = static_cast<DIDeviceObjectData *>(calloc(1, 0x800));
    g_zInput_KbdSystemReady = 1;
    Keyboard_ResetTransitionState();
    Keyboard_ClearKeyCallbackTable();
    return 0;
}

// Reimplements 0x46f9f0: zInput::Keyboard_ClearKeyCallbackTable
RECOIL_NOINLINE void RECOIL_CDECL Keyboard_ClearKeyCallbackTable() {
    int entryIndex3;
    for (entryIndex3 = 0;
         entryIndex3 <
         (int)(sizeof(g_zInputKbdKeyDispatchTable) / sizeof(g_zInputKbdKeyDispatchTable[0]));
         ++entryIndex3) {
        KbdKeyDispatchEntry &entry = g_zInputKbdKeyDispatchTable[entryIndex3];
        entry.callback = 0;
    }
}

// Reimplements 0x46fd20: zInput::Keyboard_InitDikToAsciiTable
RECOIL_NOINLINE void RECOIL_CDECL Keyboard_InitDikToAsciiTable() {
    memset(g_zInput_KbdDikToAsciiTable, 0, sizeof(g_zInput_KbdDikToAsciiTable));

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
    g_zInput_KbdDikToAsciiTable[0x47] = '7';
    g_zInput_KbdDikToAsciiTable[0x48] = '8';
    g_zInput_KbdDikToAsciiTable[0x49] = '9';
    g_zInput_KbdDikToAsciiTable[0x4a] = '-';
    g_zInput_KbdDikToAsciiTable[0x4b] = '4';
    g_zInput_KbdDikToAsciiTable[0x4c] = '5';
    g_zInput_KbdDikToAsciiTable[0x4d] = '6';
    g_zInput_KbdDikToAsciiTable[0x4e] = '+';
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

// Reimplements 0x46fba0: zInput::Keyboard_TranslateDikToAscii
RECOIL_NOINLINE int RECOIL_FASTCALL Keyboard_TranslateDikToAscii(int comboIdx) {
    if (g_zInput_KbdDikToAsciiTableReady == 0) {
        Keyboard_InitDikToAsciiTable();
        g_zInput_KbdDikToAsciiTableReady = 1;
    }

    int result = g_zInput_KbdDikToAsciiTable[comboIdx & 0xff];
    if (result >= 'a' && result <= 'z' && (comboIdx & 0x400) != 0) {
        result -= 0x20;
    }

    if (static_cast<unsigned int>(comboIdx - 0x402) > 0x33) {
        return result;
    }

    switch (comboIdx & 0xff) {
    case 0x02:
        return '!';
    case 0x03:
        return '@';
    case 0x04:
        return '#';
    case 0x05:
        return '$';
    case 0x06:
        return '%';
    case 0x07:
        return '^';
    case 0x08:
        return '&';
    case 0x09:
        return '*';
    case 0x0a:
        return '(';
    case 0x0b:
        return ')';
    case 0x0c:
        return '_';
    case 0x0d:
        return '+';
    case 0x1a:
        return '{';
    case 0x1b:
        return '}';
    case 0x27:
        return ':';
    case 0x28:
        return '"';
    case 0x29:
        return '~';
    case 0x2b:
        return '|';
    case 0x33:
        return '<';
    case 0x34:
        return '>';
    case 0x35:
        return '?';
    default:
        return result;
    }
}

// Reimplements 0x46f980: zInput::Keyboard_GetKeyTransitionState
RECOIL_NOINLINE int RECOIL_FASTCALL Keyboard_GetKeyTransitionState(int keyIndex) {
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

// Reimplements 0x46f9b0: zInput::Keyboard_RegisterKeyCallback
RECOIL_NOINLINE int RECOIL_FASTCALL
Keyboard_RegisterKeyCallback(int comboIdx, void *callback, const char * /*unusedLabel*/) {
    if (g_zInputKbdKeyDispatchTable[comboIdx].callback != 0) {
        return -1;
    }

    g_zInputKbdKeyDispatchTable[comboIdx].callback = callback;
    return 0;
}

// Reimplements 0x46f9d0: zInput::Keyboard_UnregisterKeyCallback
RECOIL_NOINLINE void RECOIL_FASTCALL Keyboard_UnregisterKeyCallback(int comboIdx) {
    if (g_zInputKbdKeyDispatchTable[comboIdx].callback != 0) {
        g_zInputKbdKeyDispatchTable[comboIdx].callback = 0;
    }
}

// Reimplements 0x46f970: zInput::Keyboard_SetRawEventCallback
RECOIL_NOINLINE void RECOIL_FASTCALL Keyboard_SetRawEventCallback(void *callback, void *context) {
    g_zInput_KbdRawEventCallback = callback;
    g_zInput_KbdRawEventCallbackCtx = context;
}

// Reimplements 0x471d20: zInput::Keyboard_AddRef
RECOIL_NOINLINE int RECOIL_CDECL Keyboard_AddRef() {
    if ((g_zInput_DeviceRegistry & 1) != 0) {
        if (g_zInputKeyboardPollRefCount == 0) {
            Keyboard_ResetTransitionState();
        }
        ++g_zInputKeyboardPollRefCount;
    }

    return static_cast<unsigned short>(g_zInputKeyboardPollRefCount);
}

// Reimplements 0x46f690: zInput::Keyboard_PollState
RECOIL_NOINLINE void RECOIL_FASTCALL Keyboard_PollState(int dispatchCallbacks) {
    unsigned int inOutCount = 0x80;
    const int hresult = g_zInput_KbdDevice->vtbl_00->GetDeviceData_28(
        g_zInput_KbdDevice, sizeof(DIDeviceObjectData), g_zInput_KbdEventBuffer, &inOutCount, 0);
    if (hresult != kDiOk) {
        if (hresult != kDiInputLost) {
            DI_ReportError(hresult, kZInputKeyboardSourceFile, 0x170);
            return;
        }

        g_zInput_KbdDevice->vtbl_00->Acquire_1c(g_zInput_KbdDevice);
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

// Reimplements 0x46fa10: zInput::Keyboard_WaitForAnyKeyPress
RECOIL_NOINLINE int RECOIL_FASTCALL Keyboard_WaitForAnyKeyPress(int keepWaiting) {
    int result = 0;
    do {
        unsigned int inOutCount = 1;
        const int hresult = g_zInput_KbdDevice->vtbl_00->GetDeviceData_28(
            g_zInput_KbdDevice, sizeof(DIDeviceObjectData), g_zInput_KbdEventBuffer, &inOutCount,
            0);
        if (hresult != kDiOk) {
            if (hresult != kDiInputLost) {
                DI_ReportError(hresult, kZInputKeyboardSourceFile, 0x291);
                return 0;
            }

            g_zInput_KbdDevice->vtbl_00->Acquire_1c(g_zInput_KbdDevice);
        }

        for (unsigned int i = 0; i < inOutCount; ++i) {
            result = ApplyKeyboardWaitEvent(g_zInput_KbdEventBuffer[i]);
            if (result != 0) {
                return result;
            }
        }
    } while (keepWaiting != 0);

    return 0;
}

// Reimplements 0x404140: zInput_WaitForAnyKeyPressWithTimeoutMs
extern "C" RECOIL_NOINLINE int RECOIL_FASTCALL
zInput_WaitForAnyKeyPressWithTimeoutMs(int timeoutMs) {
    if (timeoutMs <= 0) {
        return 0;
    }

    int remainingMs = timeoutMs;
    do {
        if (zInput::Keyboard_WaitForAnyKeyPress(0) != 0) {
            return 1;
        }

        Sleep(100);
        remainingMs -= 100;
    } while (remainingMs > 0);

    return 0;
}

// Reimplements 0x471da0: zInput::Mouse_AddRef
RECOIL_NOINLINE int RECOIL_CDECL Mouse_AddRef() {
    if ((g_zInputMouseFlags & 1) != 0) {
        if (g_zInputMousePollRefCount == 0) {
            Mouse_ResetTransitionState();
        }
        ++g_zInputMousePollRefCount;
    }

    return static_cast<unsigned short>(g_zInputMousePollRefCount);
}

// Reimplements 0x471d50: zInput::DI_AddJoystickRef
RECOIL_NOINLINE int RECOIL_CDECL DI_AddJoystickRef() {
    if ((g_zInputJoystickFlags & 1) != 0 && g_zInputJoystickPollRefCount == 0) {
        DI_ResetTransitionState();
    }

    ++g_zInputJoystickPollRefCount;
    return g_zInputJoystickPollRefCount;
}

// Reimplements 0x471d80: zInput::DI_ReleaseJoystickRef
RECOIL_NOINLINE int RECOIL_CDECL DI_ReleaseJoystickRef() {
    short refCount = g_zInputJoystickPollRefCount;
    if (static_cast<unsigned short>(refCount) > 0) {
        --refCount;
    }

    g_zInputJoystickPollRefCount = refCount;
    return refCount;
}

// Reimplements 0x471dd0: zInput::DI_GetJoystickRefCount
RECOIL_NOINLINE int RECOIL_CDECL DI_GetJoystickRefCount() {
    return g_zInputJoystickPollRefCount;
}

// Reimplements 0x471f60: zInput::DI_EnumDevicesCallback_SelectFirstJoystick
RECOIL_NOINLINE int RECOIL_STDCALL
DI_EnumDevicesCallback_SelectFirstJoystick(const DIDeviceInstance *instance, void *) {
    DIDevice *baseDevice = 0;
    const int hr = g_zInput_GlobalState->vtbl_00->CreateDevice_0c(
        g_zInput_GlobalState, &instance->guidInstance, &baseDevice, 0);
    if (hr != 0) {
        return 1;
    }

    baseDevice->vtbl_00->QueryInterface_00(baseDevice, &IID_IDirectInputDevice2A,
                                           &g_zInput_JoystickDevice);
    baseDevice->vtbl_00->Release_08(baseDevice);
    return 0;
}

// Reimplements 0x471fb0: zInput::DI_AcquireJoystickDevice
RECOIL_NOINLINE int RECOIL_CDECL DI_AcquireJoystickDevice() {
    if (g_zInput_JoystickDevice == 0) {
        return 0;
    }

    return g_zInput_JoystickDevice->vtbl_00->Acquire_1c(g_zInput_JoystickDevice) == 0 ? 1 : 0;
}

// Reimplements 0x471e40: zInput::DI_InitJoystickDevice
RECOIL_NOINLINE int RECOIL_FASTCALL DI_InitJoystickDevice(HWND hwnd) {
    if (g_zInput_GlobalState == 0) {
        return 0;
    }

    g_zInput_JoystickDevice = 0;
    g_zInput_GlobalState->vtbl_00->EnumDevices_10(
        g_zInput_GlobalState, 4, DI_EnumDevicesCallback_SelectFirstJoystick, 0, 1);

    DIDevice *joystickDevice = g_zInput_JoystickDevice;
    if (joystickDevice == 0) {
        return 0;
    }

    DIDeviceCaps caps;
    caps.dwSize = 0x2c;
    joystickDevice->vtbl_00->SetDataFormat_2c(joystickDevice, &c_dfDIJoystick);
    joystickDevice->vtbl_00->GetCapabilities_0c(joystickDevice, &caps);

    g_zInput_JoystickAxisCount = caps.dwAxes;
    g_zInput_JoystickCaps_ForceFeedback = caps.dwFlags & 0x100;
    g_zInput_JoystickCaps_FFAttack = caps.dwFlags & 0x200;
    g_zInput_JoystickCaps_FFFade = caps.dwFlags & 0x400;

    const unsigned int coopFlags = g_zInput_JoystickCaps_ForceFeedback != 0 ? 5U : 9U;
    joystickDevice->vtbl_00->SetCooperativeLevel_34(joystickDevice, hwnd, coopFlags);

    JoystickAxisConfig axisCfg;
    memset(&axisCfg, 0, sizeof(axisCfg));
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

// Reimplements 0x4721a0: zInput::DI_SetAxisDeadzone
RECOIL_NOINLINE int RECOIL_FASTCALL DI_SetAxisDeadzone(int axisOffset,
                                                                int deadzone) {
    struct DipropDwordLocal {
        unsigned int dwSize;
        unsigned int dwHeaderSize;
        unsigned int dwObj;
        unsigned int dwHow;
        unsigned int dwData;
    } prop;

    prop.dwSize = sizeof(prop);
    prop.dwHeaderSize = 0x10;
    prop.dwObj = static_cast<unsigned int>(axisOffset);
    prop.dwHow = 1;
    prop.dwData = static_cast<unsigned int>(deadzone);
    return g_zInput_JoystickDevice->vtbl_00->SetProperty_18(g_zInput_JoystickDevice, 5, &prop);
}

// Reimplements 0x4721e0: zInput::DI_SetAxisRange
RECOIL_NOINLINE int RECOIL_FASTCALL DI_SetAxisRange(int axisOffset,
                                                             int rangeMin,
                                                             int rangeMax) {
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
    prop.dwObj = static_cast<unsigned int>(axisOffset);
    prop.dwHow = 1;
    prop.lMin = rangeMin;
    prop.lMax = rangeMax;
    return g_zInput_JoystickDevice->vtbl_00->SetProperty_18(g_zInput_JoystickDevice, 4, &prop);
}

// Reimplements 0x472230: zInput::DI_GetAxisRange
RECOIL_NOINLINE int RECOIL_FASTCALL DI_GetAxisRange(int axisOffset,
                                                             int *pOutMin,
                                                             int *pOutMax) {
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
    prop.dwObj = static_cast<unsigned int>(axisOffset);
    prop.dwHow = 1;
    const int result =
        g_zInput_JoystickDevice->vtbl_00->GetProperty_14(g_zInput_JoystickDevice, 4, &prop);
    *pOutMin = prop.lMin;
    *pOutMax = prop.lMax;
    return result;
}

namespace {
void ApplyAxisConfigEntry(JoystickAxisConfig *axisCfg, int axisIndex,
                          int axisOffset, int &result) {
    JoystickAxisConfigEntry &axis = axisCfg->axes[axisIndex];
    if (DI_SetAxisRange(axisOffset, axis.lMin, axis.lMax) < 0) {
        DI_GetAxisRange(axisOffset, &axis.lMin, &axis.lMax);
    }

    axis.midpoint = static_cast<float>(axis.lMin + axis.lMax) * 0.5f;
    axis.normScale = 2.0f / static_cast<float>(axis.lMax - axis.lMin);
    result &= DI_SetAxisDeadzone(axisOffset, axis.deadzone) >= 0 ? 1 : 0;
}
} // namespace

// Reimplements 0x471fd0: zInput::DI_ApplyAxisConfig
RECOIL_NOINLINE int RECOIL_FASTCALL DI_ApplyAxisConfig(JoystickAxisConfig *axisCfg) {
    if (axisCfg == 0) {
        return 0;
    }

    int result = 1;
    ApplyAxisConfigEntry(axisCfg, 0, 0, result);
    ApplyAxisConfigEntry(axisCfg, 1, 4, result);
    if (g_zInput_JoystickAxisCount > 2) {
        ApplyAxisConfigEntry(axisCfg, 2, 8, result);
    }
    if (g_zInput_JoystickAxisCount > 3) {
        ApplyAxisConfigEntry(axisCfg, 3, 0x14, result);
    }

    g_zInput_JoystickAxisConfig = *axisCfg;
    return result;
}

// Reimplements 0x4722b0: zInput::DI_IsJoystickDeviceReady
RECOIL_NOINLINE int RECOIL_CDECL DI_IsJoystickDeviceReady() {
    return g_zInput_JoystickInitialized == 1 ? 1 : 0;
}

// Reimplements 0x472390: zInput::DI_GetCurrentState
RECOIL_NOINLINE JoystickStatePartial *RECOIL_CDECL DI_GetCurrentState() {
    return &g_zInput_JoystickCurrentState;
}

// Reimplements 0x4722c0: zInput::DI_PollJoystickState
RECOIL_NOINLINE JoystickStatePartial *RECOIL_FASTCALL
DI_PollJoystickState(int dispatchCallbacks) {
    if (g_zInput_JoystickInitialized == 0) {
        return 0;
    }

    JoystickStatePartial polledState = {0};
    g_zInput_JoystickDevice->vtbl_00->Poll_64(g_zInput_JoystickDevice);
    const int result = g_zInput_JoystickDevice->vtbl_00->GetDeviceState_24(
        g_zInput_JoystickDevice, sizeof(JoystickStatePartial), &polledState);

    if (g_zInput_JoystickAxisCount < 3) {
        polledState.lZ = 0;
        polledState.lVZ = 0;
        polledState.lAZ = 0;
        polledState.lFZ = 0;
    }
    if (g_zInput_JoystickAxisCount < 4) {
        polledState.lRz = 0;
        polledState.lVRz = 0;
        polledState.lARz = 0;
        polledState.lFRz = 0;
    }

    if (result == kDiInputLost) {
        DI_AcquireJoystickDevice();
        return 0;
    }
    if (result != kDiOk) {
        return 0;
    }

    g_zInput_JoystickPreviousState = g_zInput_JoystickCurrentState;
    g_zInput_JoystickCurrentState = polledState;
    if (dispatchCallbacks != 0 && g_zInput_BindMap_Current != 0) {
        g_zInput_BindMap_Current->DispatchJoystickButtonCallbacks();
    }

    return DI_GetCurrentState();
}

// Reimplements 0x4723a0: zInput::DI_GetButtonTransitionState
RECOIL_NOINLINE int RECOIL_FASTCALL DI_GetButtonTransitionState(int buttonIndex) {
    const unsigned char current = g_zInput_JoystickCurrentState.rgbButtons[buttonIndex - 1];
    const unsigned char previous = g_zInput_JoystickPreviousState.rgbButtons[buttonIndex - 1];
    if (current != 0) {
        return (previous != 0 ? 1 : 0) + 1;
    }

    return previous != 0 ? 4 : 0;
}

// Reimplements 0x4723d0: zInput::DI_WaitForButtonPress
RECOIL_NOINLINE int RECOIL_FASTCALL DI_WaitForButtonPress(int loopUntilPressed) {
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

// Reimplements 0x42e170: zInput::DI_SetJoystickEnabled
RECOIL_NOINLINE int RECOIL_FASTCALL DI_SetJoystickEnabled(int enable) {
    if (enable != 0 && DI_IsJoystickDeviceReady() != 0) {
        if (DI_GetJoystickRefCount() == 0) {
            DI_AddJoystickRef();
        }

        JoystickAxisConfig &cfg = g_zInput_JoystickAxisConfig_Gameplay;
        cfg.axes[0].lMin = -1000;
        cfg.axes[0].lMax = 1000;
        cfg.axes[2].lMax = 1000;
        cfg.axes[3].lMax = 1000;
        cfg.axes[2].lMin = -1000;
        cfg.axes[3].lMin = -1000;
        cfg.axes[1].lMin = -10000;
        cfg.axes[1].lMax = 10000;
        cfg.axes[0].deadzone = 2000;
        cfg.axes[1].deadzone = 3000;
        cfg.axes[2].deadzone = 1500;
        cfg.axes[3].deadzone = 2000;
        DI_ApplyAxisConfig(&cfg);
        return 1;
    }

    if (DI_GetJoystickRefCount() != 0) {
        DI_ReleaseJoystickRef();
    }
    return 0;
}

// Reimplements 0x470310: zInput::Mouse_UpdateAcquireState
RECOIL_NOINLINE void RECOIL_CDECL Mouse_UpdateAcquireState() {
    DIDevice *device = g_zInput_MouseDevice;

    if (g_zInput_MouseActive != 0) {
        if (device != 0) {
            const int result = device->vtbl_00->Acquire_1c(device);
            if (result != kDiOk && result != kDiFalse) {
                g_zInput_MouseActive = 0;
            }
        }
    } else {
        if (device != 0) {
            const int result = device->vtbl_00->Unacquire_20(device);
            if (result != kDiOk && result != kDiFalse) {
                g_zInput_MouseActive = 1;
            }
        }
    }
}

// Reimplements 0x470360: zInput::Mouse_ShutdownDevice
RECOIL_NOINLINE int RECOIL_CDECL Mouse_ShutdownDevice() {
    g_zInput_MouseActive = 0;
    Mouse_UpdateAcquireState();

    DIDevice *device = g_zInput_MouseDevice;
    if (device != 0) {
        device->vtbl_00->Release_08(device);
    }

    g_zInput_MouseDevice = 0;
    g_zInput_MouseInitialized = 0;
    return 1;
}

// Reimplements 0x4703b0: zInput::Mouse_PollAndStoreState
RECOIL_NOINLINE void RECOIL_CDECL Mouse_PollAndStoreState() {
    g_zInputMouseLastPollResult = Mouse_PollState(0);
}

// Reimplements 0x471de0: zInput::PollActiveDevices
RECOIL_NOINLINE void RECOIL_FASTCALL PollActiveDevices(int dispatchCallbacks) {
    const int savedDispatchCallbacks = dispatchCallbacks;
    if (g_zInputMouseFlags == 1 && static_cast<unsigned short>(g_zInputMousePollRefCount) > 0) {
        Mouse_PollAndStoreState();
    }

    if (g_zInputJoystickFlags == 1 &&
        static_cast<unsigned short>(g_zInputJoystickPollRefCount) > 0) {
        DI_PollJoystickState(savedDispatchCallbacks);
    }

    if (g_zInput_DeviceRegistry == 1 &&
        static_cast<unsigned short>(g_zInputKeyboardPollRefCount) > 0) {
        Keyboard_PollState(savedDispatchCallbacks);
    }
}

// Reimplements 0x4703c0: zInput::Mouse_PollState
RECOIL_NOINLINE int RECOIL_FASTCALL Mouse_PollState(int dispatchCallbacks) {
    g_zInput_MouseStateSnapshot.deltaX = 0;
    g_zInput_MouseStateSnapshot.deltaY = 0;

    if (g_zInput_MouseActive == 0) {
        g_zInput_MouseActive = 1;
        Mouse_UpdateAcquireState();
        if (g_zInput_MouseActive == 0) {
            return kDiInputLost;
        }
    }

    MouseDeviceState deviceState = {0};
    g_zInput_MouseDevice->vtbl_00->Poll_64(g_zInput_MouseDevice);
    const int result = g_zInput_MouseDevice->vtbl_00->GetDeviceState_24(
        g_zInput_MouseDevice, sizeof(MouseDeviceState), &deviceState);
    if (result == kDiInputLost) {
        Mouse_UpdateAcquireState();
        return result;
    }
    if (result != kDiOk) {
        return result;
    }

    g_zInput_MousePreviousState = g_zInput_MouseCurrentState;
    g_zInput_MouseCurrentState = deviceState;
    g_zInput_MouseStateSnapshot.deltaX = g_zInput_MouseCurrentState.lX;
    g_zInput_MouseStateSnapshot.deltaY = g_zInput_MouseCurrentState.lY;
    Mouse_ApplyAccumulatedDelta();
    g_zInput_MouseStateSnapshot.button1Transition = Mouse_GetButtonTransitionState(1);
    g_zInput_MouseStateSnapshot.button2Transition = Mouse_GetButtonTransitionState(2);
    g_zInput_MouseStateSnapshot.button3Transition = Mouse_GetButtonTransitionState(3);

    if (g_zInput_BindMap_Current != 0 && dispatchCallbacks != 0) {
        g_zInput_BindMap_Current->DispatchMouseButtonCallbacks();
    }

    return result;
}

// Reimplements 0x470680: zInput::Mouse_WaitForButtonPress
RECOIL_NOINLINE int RECOIL_FASTCALL Mouse_WaitForButtonPress(int pollUntilFound) {
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

// Reimplements 0x4704f0: zInput::Mouse_ApplyAccumulatedDelta
RECOIL_NOINLINE void RECOIL_CDECL Mouse_ApplyAccumulatedDelta() {
    g_zInput_MouseStateSnapshot.deltaX = static_cast<int>(
        static_cast<double>(g_zInput_MouseStateSnapshot.deltaX) * g_zInput_MouseSensitivityX);
    g_zInput_MouseStateSnapshot.deltaY = static_cast<int>(
        static_cast<double>(g_zInput_MouseStateSnapshot.deltaY) * g_zInput_MouseSensitivityY);

    int cursorX =
        g_zInput_MouseStateSnapshot.cursorClientX + g_zInput_MouseStateSnapshot.deltaX;
    int cursorY =
        g_zInput_MouseStateSnapshot.cursorClientY + g_zInput_MouseStateSnapshot.deltaY;
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
        static_cast<float>(static_cast<double>(cursorX - g_zInput_MouseClientCenterX) *
                           g_zInput_MouseInvClientCenterX);
    g_zInput_MouseStateSnapshot.cursorNormY =
        static_cast<float>(static_cast<double>(cursorY - g_zInput_MouseClientCenterY) *
                           g_zInput_MouseInvClientCenterY);
    g_zInput_MouseStateSnapshot.deltaNormX = static_cast<float>(
        static_cast<double>(g_zInput_MouseStateSnapshot.deltaX) * g_zInput_MouseInvClientCenterX);
    g_zInput_MouseStateSnapshot.deltaNormY = static_cast<float>(
        static_cast<double>(g_zInput_MouseStateSnapshot.deltaY) * g_zInput_MouseInvClientCenterY);
}

// Reimplements 0x470610: zInput::Mouse_ResetTransitionState
RECOIL_NOINLINE void RECOIL_CDECL Mouse_ResetTransitionState() {
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

// Reimplements 0x471c60: zInput::Mouse_IsUnsuspended
RECOIL_NOINLINE int RECOIL_CDECL Mouse_IsUnsuspended() {
    return IsUnsuspended(g_zInput_MouseSuspendFlags);
}

// Reimplements 0x471c70: zInput::Joystick_IsUnsuspended
RECOIL_NOINLINE int RECOIL_CDECL Joystick_IsUnsuspended() {
    return IsUnsuspended(g_zInput_JoystickSuspendFlags);
}

RECOIL_NOINLINE int RECOIL_CDECL Keyboard_IsUnsuspended() {
    return zInput_Keyboard_IsUnsuspended();
}

// Reimplements 0x471cf0: zInput::Mouse_Suspend
RECOIL_NOINLINE void RECOIL_CDECL Mouse_Suspend() {
    g_zInput_MouseSuspendFlags |= kSuspendFlag;
}

// Reimplements 0x471d00: zInput::Joystick_Suspend
RECOIL_NOINLINE void RECOIL_CDECL Joystick_Suspend() {
    g_zInput_JoystickSuspendFlags |= kSuspendFlag;
}

// Reimplements 0x471d10: zInput::Keyboard_Suspend
RECOIL_NOINLINE void RECOIL_CDECL Keyboard_Suspend() {
    g_zInput_KeyboardSuspendFlags |= kSuspendFlag;
}

// Reimplements 0x471c90: zInput::Mouse_ResumeFromSuspend
RECOIL_NOINLINE void RECOIL_CDECL Mouse_ResumeFromSuspend() {
    if ((g_zInput_MouseSuspendFlags & kSuspendFlag) != 0) {
        Mouse_ResetTransitionState();
    }

    g_zInput_MouseSuspendFlags &= static_cast<unsigned char>(~kSuspendFlag);
}

// Reimplements 0x46f450: zInput::Keyboard_ResetTransitionState
RECOIL_NOINLINE void RECOIL_CDECL Keyboard_ResetTransitionState() {
    if (g_zInput_KbdSystemReady == 0) {
        return;
    }

    unsigned int inOutCount = 128;
    const int hresult = g_zInput_KbdDevice->vtbl_00->GetDeviceData_28(
        g_zInput_KbdDevice, sizeof(DIDeviceObjectData), g_zInput_KbdEventBuffer, &inOutCount, 0);
    if (hresult != kDiOk) {
        if (hresult == kDiInputLost) {
            g_zInput_KbdDevice->vtbl_00->Acquire_1c(g_zInput_KbdDevice);
        } else {
            DI_ReportError(hresult, "D:\\Proj\\GameZRecoil\\zInput\\zin_kbd.cpp", 257);
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

            dispatchIndex |= static_cast<unsigned int>(g_zInput_KbdModifierState);
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
        for (entryIndex4 = 0;
             entryIndex4 <
             sizeof(g_zInputKbdKeyDispatchTable) / sizeof(g_zInputKbdKeyDispatchTable[0]);
             ++entryIndex4) {
            KbdKeyDispatchEntry &entry = g_zInputKbdKeyDispatchTable[entryIndex4];
            entry.state = 0;
        }
    }
    g_zInput_KbdModifierState = 0;
}

// Reimplements 0x471cd0: zInput::Keyboard_ResumeFromSuspend
RECOIL_NOINLINE void RECOIL_CDECL Keyboard_ResumeFromSuspend() {
    if ((g_zInput_KeyboardSuspendFlags & kSuspendFlag) != 0) {
        Keyboard_ResetTransitionState();
    }

    g_zInput_KeyboardSuspendFlags &= static_cast<unsigned char>(~kSuspendFlag);
}

// Reimplements 0x472410: zInput::DI_ResetTransitionState
RECOIL_NOINLINE void RECOIL_CDECL DI_ResetTransitionState() {
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

// Reimplements 0x471c50: zInput::ResetAllTransitionState
RECOIL_NOINLINE void RECOIL_CDECL ResetAllTransitionState() {
    Keyboard_ResetTransitionState();
    DI_ResetTransitionState();
    Mouse_ResetTransitionState();
}

// Reimplements 0x471cb0: zInput::Joystick_ResumeFromSuspend
RECOIL_NOINLINE void RECOIL_CDECL Joystick_ResumeFromSuspend() {
    if ((g_zInput_JoystickSuspendFlags & kSuspendFlag) != 0) {
        DI_ResetTransitionState();
    }

    g_zInput_JoystickSuspendFlags &= static_cast<unsigned char>(~kSuspendFlag);
}

// Reimplements 0x471b20: zInput::OnAppActivate
RECOIL_NOINLINE void RECOIL_CDECL OnAppActivate() {
    if (g_zInput_hWnd == 0) {
        return;
    }

    Joystick_ResumeFromSuspend();
    Keyboard_ResumeFromSuspend();
    Mouse_ResumeFromSuspend();
    g_zInput_MouseActive = 1;
    Mouse_UpdateAcquireState();
}

// Reimplements 0x471ae0: zInput::OnAppDeactivate
RECOIL_NOINLINE void RECOIL_CDECL OnAppDeactivate() {
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
