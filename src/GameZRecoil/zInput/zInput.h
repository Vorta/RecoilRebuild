#ifndef GAMEZRECOIL_ZINPUT_ZINPUT_H
#define GAMEZRECOIL_ZINPUT_ZINPUT_H

#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "recoil/Mfc42Abi.h"
#include <windows.h>

#include <vector>

#include "recoil/recoil_callconv.h"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0500
#endif
#include <dinput.h>

typedef void(__fastcall *zInputCommandCallbackFn)(int commandId);

struct zVec3;
struct zInput_GlobalState;

struct zInput_BindMapContext {
    int m_isOverlay;
    int m_commandCount;
    int *m_packedBindings;
    zInputCommandCallbackFn *m_commandCallbacks;
    char **m_commandLabels;
    int m_primaryKeyToCommand[0x7de];
    int m_secondaryKeyToCommand[0x7de];
    int m_joystickToCommand[0x10];
    int m_mouseToCommand[4];

    zInput_BindMapContext * InitFromTemplate(
        const zInput_BindMapContext *tmpl
    );
    void FreeAllBuffers();
    void RebuildLookupIndices();
    void InitCommandMap(int commandCount);
    void FreeNonOwnedBuffers();
    void ResetAllBindings();
    int GetPrimaryKeyboardKey(int commandIndex);
    int GetSecondaryKeyboardKey(int commandIndex);
    int GetJoystickButtonSlot(int commandIndex);
    int GetMouseButtonSlot(int commandIndex);
    int GetCommandByPrimaryKey(int keyboardKey);
    int GetCommandBySecondaryKey(int keyboardKey);
    int GetCommandByAnyKeyboardKey(int keyboardKey);
    int GetCommandByJoystickSlot(int joystickSlot);
    int GetCommandByMouseSlot(int mouseSlot);
    void SetPrimaryKeyBinding(
        int keyCode,
        int commandId
    );
    void SetSecondaryKeyBinding(
        int keyCode,
        int commandId
    );
    void SetJoystickBinding(
        int joystickSlot,
        int commandId
    );
    void SetMouseBinding(
        int mouseSlot,
        int commandId
    );
    void SetBindingRecord(
        int commandId,
        const char *labelSrc,
        int primaryKey,
        int secondaryKey,
        int joystickSlot,
        int mouseSlot
    );
    void DispatchMouseButtonCallbacks();
    void DispatchJoystickButtonCallbacks();
    int SetCommandCallback(
        int commandId,
        zInputCommandCallbackFn callback
    );
    int ReadCommandInputState(int commandIndex);
    char * CopyCommandLabel(
        int commandId,
        char *destBuf,
        int maxBytes
    );
    char * FormatKeyComboName(
        int packedKey,
        char *destBuf,
        int maxBytes
    );
    char * CopyJoystickButtonName(
        int joystickSlot,
        char *outBuf,
        int bufSize
    );
    char * CopyMouseButtonName(
        int mouseSlot,
        char *outBuf,
        int bufSize
    );
};

RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindMapContext,
        m_commandCount
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindMapContext,
        m_packedBindings
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindMapContext,
        m_commandCallbacks
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindMapContext,
        m_commandLabels
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindMapContext,
        m_primaryKeyToCommand
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindMapContext,
        m_secondaryKeyToCommand
    ) == 0x1f8c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindMapContext,
        m_joystickToCommand
    ) == 0x3f04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindMapContext,
        m_mouseToCommand
    ) == 0x3f44
);
RECOIL_STATIC_ASSERT(sizeof(zInput_BindMapContext) == 0x3f54);

struct zInput_BindMapOverlayStackNode {
    zInput_BindMapOverlayStackNode *next;
    zInput_BindMapOverlayStackNode *prev;
    zInput_BindMapContext *bindMap;
};
RECOIL_STATIC_ASSERT(sizeof(zInput_BindMapOverlayStackNode) == 0x0c);

typedef std::vector<int> zInput_CommandIdVector;

/**
 * Bind-group record: CString title followed by the command-id vector.
 */
struct CZInputBindGroupInfo {
    CString title;
    zInput_CommandIdVector commandIds;

    CZInputBindGroupInfo(const char *sourceTitle) {
        title = sourceTitle;
    }

    ~CZInputBindGroupInfo();
};

typedef std::vector<CZInputBindGroupInfo *> zInput_BindGroupInfoList;
#if defined(_MSC_VER) && _MSC_VER < 1200
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInputBindGroupInfo,
        title
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        CZInputBindGroupInfo,
        commandIds
    ) == 0x04
);
RECOIL_STATIC_ASSERT(sizeof(CZInputBindGroupInfo) == 0x14);
RECOIL_STATIC_ASSERT(sizeof(zInput_BindGroupInfoList) == 0x10);
#endif

typedef IDirectInputEffect zInput_DiEffect;

struct zInput_FFEffectSet {
    zInput_DiEffect *PrimaryFire;
    zInput_DiEffect *AltFire;
    zInput_DiEffect *AmbientSine;
    zInput_DiEffect *CollisionImpact;
    zInput_DiEffect *DamageHit;
    zInput_DiEffect *SteerForce;
    zInput_DiEffect *PitchForce;

    void PlayCollisionImpactEffect(
        const zVec3 *impactWorldPosXZ,
        float gain
    );
    void PlayDamageHitEffect(
        const zVec3 *damageSourceWorldPosXZ,
        float gain
    );
};
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_FFEffectSet,
        PrimaryFire
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_FFEffectSet,
        AltFire
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_FFEffectSet,
        AmbientSine
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_FFEffectSet,
        CollisionImpact
    ) == 0x0c
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_FFEffectSet,
        DamageHit
    ) == 0x10
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_FFEffectSet,
        SteerForce
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_FFEffectSet,
        PitchForce
    ) == 0x18
);
RECOIL_STATIC_ASSERT(sizeof(zInput_FFEffectSet) == 0x1c);

struct zInput_PlayerStatePartial {
    unsigned char unknown_000[0x94];
    float angVelYaw;
    unsigned char unknown_098[0x34];
    float yawVelocityLimit;
    unsigned char unknown_0d0[0x2ec];
    float pitchAngleRad;
    unsigned char unknown_3c0[0x160];
    float cameraDirNextX;
    float cameraDirNextY;
    float cameraDirNextZ;
};
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_PlayerStatePartial,
        angVelYaw
    ) == 0x94
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_PlayerStatePartial,
        yawVelocityLimit
    ) == 0xcc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_PlayerStatePartial,
        pitchAngleRad
    ) == 0x3bc
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_PlayerStatePartial,
        cameraDirNextX
    ) == 0x520
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_PlayerStatePartial,
        cameraDirNextZ
    ) == 0x528
);

struct zInput_GameStateOrMapTablePartial {
    void *unknown_00;
    zInput_PlayerStatePartial *playerState;
};
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_GameStateOrMapTablePartial,
        playerState
    ) == 0x04
);

namespace zInput {
typedef IDirectInputA DIDirectInput;
typedef IDirectInputDevice2A DIDevice;
typedef DIDEVICEINSTANCEA DIDeviceInstance;
typedef DIDEVICEOBJECTDATA DIDeviceObjectData;

typedef int(__stdcall *DIEnumDevicesCallback)(
    const DIDeviceInstance *instance,
    void *ref
);
RECOIL_STATIC_ASSERT(offsetof(DIDeviceInstance, guidInstance) == 0x04);
RECOIL_STATIC_ASSERT(sizeof(DIDeviceObjectData) == 0x10);

struct KbdKeyDispatchEntry {
    int state;
    void *callback;
};
RECOIL_STATIC_ASSERT(sizeof(KbdKeyDispatchEntry) == 0x08);

RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        lZ
    ) == 0x08
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        lRz
    ) == 0x14
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        rgdwPOV
    ) == 0x20
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        rgbButtons
    ) == 0x30
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        lVZ
    ) == 0xb8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        lVRz
    ) == 0xc4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        lAZ
    ) == 0xd8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        lARz
    ) == 0xe4
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        lFZ
    ) == 0xf8
);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIJOYSTATE2,
        lFRz
    ) == 0x104
);
RECOIL_STATIC_ASSERT(sizeof(DIJOYSTATE2) == 0x110);

struct JoystickAxisConfigEntry {
    int lMin;
    int lMax;
    float midpoint;
    float normScale;
    int deadzone;
};
RECOIL_STATIC_ASSERT(sizeof(JoystickAxisConfigEntry) == 0x14);

struct JoystickAxisConfig {
    JoystickAxisConfigEntry axes[4];
};
RECOIL_STATIC_ASSERT(sizeof(JoystickAxisConfig) == 0x50);

struct DIDeviceCaps {
    unsigned int dwSize;
    unsigned int dwFlags;
    unsigned int dwDevType;
    unsigned int dwAxes;
    unsigned int dwButtons;
    unsigned int dwPOVs;
    unsigned int dwFFSamplePeriod;
    unsigned int dwFFMinTimeResolution;
    unsigned int dwFirmwareRevision;
    unsigned int dwHardwareRevision;
    unsigned int dwFFDriverVersion;
};
RECOIL_STATIC_ASSERT(sizeof(DIDeviceCaps) == 0x2c);
RECOIL_STATIC_ASSERT(
    offsetof(
        DIDeviceCaps,
        dwAxes
    ) == 0x0c
);

struct MouseDeviceState {
    int lX;
    int lY;
    int lZ;
    unsigned int rgbButtons;
};
RECOIL_STATIC_ASSERT(offsetof(MouseDeviceState, rgbButtons) == 0x0c);
RECOIL_STATIC_ASSERT(sizeof(MouseDeviceState) == 0x10);

struct MouseStateSnapshot {
    int cursorClientX;
    int cursorClientY;
    float cursorNormX;
    float cursorNormY;
    int deltaX;
    int deltaY;
    float deltaNormX;
    float deltaNormY;
    int button1Transition;
    int button2Transition;
    int button3Transition;
};
RECOIL_STATIC_ASSERT(offsetof(MouseStateSnapshot, deltaX) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(MouseStateSnapshot, button1Transition) == 0x20);
RECOIL_STATIC_ASSERT(sizeof(MouseStateSnapshot) == 0x2c);

void __cdecl MouseUpdateAcquireState();
int __cdecl MouseShutdownDevice();
void __cdecl MouseApplyClientCursorPosToOS();
void __cdecl MouseUpdateClientRectAndCenter();
void __cdecl MouseRecenterCursor();
void __cdecl MouseRecenterCursorX();
void __stdcall MouseSetNormalizedCursorPos(
    float normX,
    float normY
);
int __cdecl MouseIsInitialized();
int __cdecl MouseInitDevice();
void __fastcall MousePollAndStoreState(unsigned char dispatchCallbacks);
int __fastcall MousePollState(unsigned char dispatchCallbacks);
int __cdecl MouseAddRef();
int __fastcall MouseGetButtonTransitionState(int buttonNumber);
int __fastcall MouseWaitForButtonPress(int pollUntilFound);
MouseStateSnapshot *__cdecl MouseGetStateSnapshotPtr();
int __fastcall MouseGetStateSnapshot(MouseStateSnapshot *outState);
int __cdecl KeyboardShutdownDevice();
int __cdecl JoystickShutdownDevice();
int __cdecl Shutdown();
void __cdecl MouseApplyAccumulatedDelta();
void __cdecl MouseResetTransitionState();
int __cdecl MouseIsUnsuspended();
int __cdecl JoystickIsUnsuspended();
void __cdecl MouseSuspend();
void __cdecl JoystickSuspend();
void __cdecl KeyboardSuspend();
void __cdecl MouseResumeFromSuspend();
void __cdecl KeyboardResetTransitionState();
void __cdecl KeyboardResumeFromSuspend();
void __cdecl KeyboardClearKeyCallbackTable();
void __fastcall KeyboardPollState(unsigned char dispatchCallbacks);
int __fastcall KeyboardWaitForAnyKeyPress(int keepWaiting);
void __cdecl KeyboardInitDikToAsciiTable();
int __fastcall KeyboardTranslateDikToAscii(int comboIdx);
void __fastcall KeyboardSetRawEventCallback(
    void *callback,
    void *context
);
int __fastcall KeyboardGetKeyTransitionState(int keyIndex);
int __fastcall KeyboardRegisterKeyCallback(
    int comboIdx,
    void *callback,
    const char *unusedLabel
);
void __fastcall KeyboardUnregisterKeyCallback(int comboIdx);
void __cdecl ResetAllTransitionState();
int __cdecl KeyboardInitDevice();
int __cdecl KeyboardAddRef();
void __cdecl DIResetTransitionState();
void __cdecl JoystickResumeFromSuspend();
int __fastcall Init(
    HWND hWnd,
    HINSTANCE hInstance
);
void __cdecl BindMapInitDikKeyNameTable();
void __cdecl BindMapInitJoystickButtonNameTable();
void __cdecl BindMapInitMouseButtonNameTable();
int __cdecl DIAddJoystickRef();
int __cdecl DIReleaseJoystickRef();
int __cdecl DIGetJoystickRefCount();
int __fastcall DIGetButtonTransitionState(int buttonIndex);
int __stdcall DIEnumDevicesCallbackSelectFirstJoystick(
    const DIDeviceInstance *instance,
    void *ref
);
int __cdecl DIAcquireJoystickDevice();
int __fastcall DIInitJoystickDevice(HWND hwnd);
int __fastcall DIApplyAxisConfig(JoystickAxisConfig *axisCfg);
int __fastcall DISetAxisDeadzone(
    int axisOffset,
    int deadzone
);
int __fastcall DISetAxisRange(
    int axisOffset,
    int rangeMin,
    int rangeMax
);
int __fastcall DIGetAxisRange(
    int axisOffset,
    int *pOutMin,
    int *pOutMax
);
int __cdecl DIIsJoystickDeviceReady();
DIJOYSTATE2 *__cdecl DIGetCurrentState();
DIJOYSTATE2 *__fastcall DIPollJoystickState(unsigned char dispatchCallbacks);
int __fastcall DISetJoystickEnabled(int enable);
int __fastcall DIWaitForButtonPress(int loopUntilPressed);
int __fastcall DIReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
);
int __fastcall BindMapPackBindingCode(
    int primary,
    int secondary,
    int joy,
    int mouse
);
void *__fastcall GlobalStateConstructor(::zInput_GlobalState *self);
void __fastcall GlobalStateDestructor(::zInput_GlobalState *self);
void *GlobalStateStaticInit();
int GlobalStateRegisterAtExit();
void __cdecl GlobalStateAtExitDestructor();
void __cdecl GlobalStateStaticInitAndRegisterAtExit();
int __cdecl BindGroupListGetCount();
char *__fastcall BindGroupListGetGroupTitle(int groupIndex);
int __fastcall BindGroupListGetGroupCommandCount(int groupIndex);
int __fastcall BindGroupListGetGroupCommandId(
    int groupIndex,
    int commandIndex
);
void __cdecl BindGroupListClear();
int __fastcall BindGroupListAddGroup(const char *title);
void __fastcall BindGroupListAddCommandToGroup(
    int groupIndex,
    int commandId
);
char *__fastcall BindMapGetCommandLabel(int commandId);
char *__fastcall BindMapGetCommandHint(int commandId);
void __fastcall BindMapAddDefaultBinding(
    int commandId,
    int messageId,
    int primaryKey,
    int secondaryKey,
    int joystickSlot,
    int mouseSlot
);
int __fastcall BindMapInitDefaultBindings();
void __fastcall BindMapSystemInit(int commandCount);
void __cdecl BindMapSystemShutdown();
void __fastcall BindMapContextPush(zInput_BindMapContext *bindMapOrNull);
void __fastcall BindMapContextPop();
void __cdecl BindMapCurrentRebuildLookupIndices();
void __cdecl BindMapCurrentResetAllBindings();
int __fastcall BindMapCurrentGetPrimaryKeyboardKey(int commandIndex);
int __fastcall BindMapCurrentGetSecondaryKeyboardKey(int commandIndex);
int __fastcall BindMapCurrentGetJoystickButtonSlot(int commandIndex);
int __fastcall BindMapCurrentGetMouseButtonSlot(int commandIndex);
int __fastcall BindMapCurrentGetCommandByPrimaryKey(int keyboardKey);
int __fastcall BindMapCurrentGetCommandBySecondaryKey(int keyboardKey);
int __fastcall BindMapCurrentGetCommandByJoystickSlot(int joystickSlot);
int __fastcall BindMapCurrentGetCommandByMouseSlot(int mouseSlot);
void __fastcall BindMapCurrentSetPrimaryKeyBinding(
    int keyCode,
    int commandId
);
void __fastcall BindMapCurrentSetSecondaryKeyBinding(
    int keyCode,
    int commandId
);
void __fastcall BindMapCurrentSetJoystickBinding(
    int joystickSlot,
    int commandId
);
void __fastcall BindMapCurrentSetMouseBinding(
    int mouseSlot,
    int commandId
);
int __fastcall BindMapCurrentSetBindingRecord(
    int commandId,
    const char *labelSrc,
    int primaryKey,
    int secondaryKey,
    int joystickSlot,
    int mouseSlot
);
int __fastcall BindMapCurrentSetCommandCallback(
    int commandId,
    zInputCommandCallbackFn callback
);
int __fastcall BindMapCurrentReadCommandInputState(int commandIndex);
char *__fastcall BindMapCurrentCopyCommandLabel(
    int commandId,
    char *destBuf,
    int maxBytes
);
char *__fastcall BindMapCurrentFormatKeyComboName(
    int packedKey,
    char *destBuf,
    int maxBytes
);
char *__fastcall BindMapCurrentCopyJoystickButtonName(
    int joystickSlot,
    char *outBuf,
    int bufSize
);
char *__fastcall BindMapCurrentCopyMouseButtonName(
    int mouseSlot,
    char *outBuf,
    int bufSize
);
int __fastcall MouseSetCooperativeLevelFlags(int flags);
void __fastcall MouseSetClientSizeAndCenter(
    int width,
    int height
);
void __fastcall PollActiveDevices(unsigned char dispatchCallbacks);
void __cdecl OnAppActivate();
void __cdecl OnAppDeactivate();
} // namespace zInput

/**
 * Static zero-filled input lifetime object at 0x561cb0..0x565ebc. BN currently
 * proves the DirectInput root, device registry, keyboard dispatch storage,
 * joystick current/previous state, force-feedback caps, mouse current/previous
 * state, and bind-map overlay tail; the 0x4144..0x4163 interval remains
 * bounded unknown storage.
 */
struct zInput_DeviceRegistry {
    unsigned char keyboardFlags;
    unsigned char joystickFlags;
    unsigned char mouseFlags;
    unsigned char unknown_03;
    unsigned short keyboardRefCount;
    unsigned short joystickRefCount;
    unsigned short mouseRefCount;
    unsigned short unknown_0a;
};
typedef char zInput_DeviceRegistry_size_check[(sizeof(zInput_DeviceRegistry) == 0x0c) ? 1 : -1];

struct zInput_GlobalState {
    zInput::DIDirectInput *directInput;
    HWND hWnd;
    zInput_DeviceRegistry deviceRegistry;
    int keyboardSystemReady;
    zInput::DIDevice *keyboardDevice;
    zInput::DIDeviceObjectData *keyboardEventBuffer;
    int keyboardModifierState;
    zInput::KbdKeyDispatchEntry keyboardDispatchTable[0x7de];
    void *keyboardRawEventCallback;
    void *keyboardRawEventCallbackCtx;
    int joystickInitialized;
    zInput::DIDevice *joystickDevice;
    DIJOYSTATE2 joystickCurrentState;
    DIJOYSTATE2 joystickPreviousState;
    unsigned char unknown_4144[0x20];
    int joystickAxisCount;
    int joystickCapsForceFeedback;
    int joystickCapsFFAttack;
    int joystickCapsFFFade;
    zInput::JoystickAxisConfig joystickAxisConfig;
    int mouseInitialized;
    zInput::DIDevice *mouseDevice;
    zInput::MouseDeviceState mouseCurrentState;
    zInput::MouseDeviceState mousePreviousState;
    unsigned char unknown_41ec[4];
    zInput_BindMapContext *currentBindMap;
    int bindMapOverlayBlockSize;
    zInput_BindMapOverlayStackNode *bindMapOverlayNodeBlockList;
    zInput_BindMapOverlayStackNode *bindMapOverlayNodeFreeList;
    zInput_BindMapOverlayStackNode *bindMapOverlayNodeStackHead;
    int bindMapOverlayReserved;
    int bindMapOverlayDepth;
};
typedef char zInput_GlobalState_directInput_check[
    (offsetof(zInput_GlobalState, directInput) == 0x00) ? 1 : -1
];
typedef char zInput_GlobalState_hWnd_check[
    (offsetof(zInput_GlobalState, hWnd) == 0x04) ? 1 : -1
];
typedef char zInput_GlobalState_deviceRegistry_check[
    (offsetof(zInput_GlobalState, deviceRegistry) == 0x08) ? 1 : -1
];
typedef char zInput_GlobalState_keyboardSystemReady_check[
    (offsetof(zInput_GlobalState, keyboardSystemReady) == 0x14) ? 1 : -1
];
typedef char zInput_GlobalState_keyboardDispatchTable_check[
    (offsetof(zInput_GlobalState, keyboardDispatchTable) == 0x24) ? 1 : -1
];
typedef char zInput_GlobalState_keyboardRawEventCallback_check[
    (offsetof(zInput_GlobalState, keyboardRawEventCallback) == 0x3f14) ? 1 : -1
];
typedef char zInput_GlobalState_joystickInitialized_check[
    (offsetof(zInput_GlobalState, joystickInitialized) == 0x3f1c) ? 1 : -1
];
typedef char zInput_GlobalState_joystickCurrentState_check[
    (offsetof(zInput_GlobalState, joystickCurrentState) == 0x3f24) ? 1 : -1
];
typedef char zInput_GlobalState_joystickPreviousState_check[
    (offsetof(zInput_GlobalState, joystickPreviousState) == 0x4034) ? 1 : -1
];
typedef char zInput_GlobalState_unknown_4144_check[
    (offsetof(zInput_GlobalState, unknown_4144) == 0x4144) ? 1 : -1
];
typedef char zInput_GlobalState_joystickAxisCount_check[
    (offsetof(zInput_GlobalState, joystickAxisCount) == 0x4164) ? 1 : -1
];
typedef char zInput_GlobalState_joystickCapsForceFeedback_check[
    (offsetof(zInput_GlobalState, joystickCapsForceFeedback) == 0x4168) ? 1 : -1
];
typedef char zInput_GlobalState_joystickCapsFFAttack_check[
    (offsetof(zInput_GlobalState, joystickCapsFFAttack) == 0x416c) ? 1 : -1
];
typedef char zInput_GlobalState_joystickCapsFFFade_check[
    (offsetof(zInput_GlobalState, joystickCapsFFFade) == 0x4170) ? 1 : -1
];
typedef char zInput_GlobalState_joystickAxisConfig_check[
    (offsetof(zInput_GlobalState, joystickAxisConfig) == 0x4174) ? 1 : -1
];
typedef char zInput_GlobalState_mouseInitialized_check[
    (offsetof(zInput_GlobalState, mouseInitialized) == 0x41c4) ? 1 : -1
];
typedef char zInput_GlobalState_mouseCurrentState_check[
    (offsetof(zInput_GlobalState, mouseCurrentState) == 0x41cc) ? 1 : -1
];
typedef char zInput_GlobalState_mousePreviousState_check[
    (offsetof(zInput_GlobalState, mousePreviousState) == 0x41dc) ? 1 : -1
];
typedef char zInput_GlobalState_currentBindMap_check[
    (offsetof(zInput_GlobalState, currentBindMap) == 0x41f0) ? 1 : -1
];
typedef char zInput_GlobalState_bindMapOverlayBlockSize_check[
    (offsetof(zInput_GlobalState, bindMapOverlayBlockSize) == 0x41f4) ? 1 : -1
];
typedef char zInput_GlobalState_bindMapOverlayNodeBlockList_check[
    (offsetof(zInput_GlobalState, bindMapOverlayNodeBlockList) == 0x41f8) ? 1 : -1
];
typedef char zInput_GlobalState_bindMapOverlayNodeFreeList_check[
    (offsetof(zInput_GlobalState, bindMapOverlayNodeFreeList) == 0x41fc) ? 1 : -1
];
typedef char zInput_GlobalState_bindMapOverlayNodeStackHead_check[
    (offsetof(zInput_GlobalState, bindMapOverlayNodeStackHead) == 0x4200) ? 1 : -1
];
typedef char zInput_GlobalState_bindMapOverlayReserved_check[
    (offsetof(zInput_GlobalState, bindMapOverlayReserved) == 0x4204) ? 1 : -1
];
typedef char zInput_GlobalState_bindMapOverlayDepth_check[
    (offsetof(zInput_GlobalState, bindMapOverlayDepth) == 0x4208) ? 1 : -1
];
typedef char zInput_GlobalState_size_check[(sizeof(zInput_GlobalState) == 0x420c) ? 1 : -1];

namespace zInp {
void __fastcall SetJoystickOption(int enabled);
void __fastcall SetJoystickAxesCountOption(int axisCount);
void __fastcall SetJoystickButtonCountOption(int buttonCount);
int GetJoystickOption();
} // namespace zInp

extern "C" {
extern zInput_GlobalState g_zInput_GlobalStateStorage;
extern const char *g_zInput_DikKeyNames[0x100];
extern const char *g_zInput_JoystickButtonNames[9];
extern const char *g_zInput_MouseButtonNames[4];
extern zInput_BindGroupInfoList g_zInput_BindGroupInfoList;
extern int g_zInput_CurrentBindGroupIndex;
extern int g_zInput_CommandLocIdTable[0x30];
extern int g_zInput_MouseActive;
extern int g_zInput_MouseCoopLevelFlags;
extern zInput::JoystickAxisConfig g_zInput_JoystickAxisConfig_Gameplay;
extern DIJOYSTATE2 g_zInput_JoystickRawDIState;
extern zInput::MouseDeviceState g_zInput_MouseRawDIState;
extern zInput::MouseStateSnapshot g_zInput_MouseStateSnapshot;
extern int g_zInputMouseLastPollResult;
extern int g_zInput_MouseClientWidth;
extern int g_zInput_MouseClientHeight;
extern int g_zInput_MouseClientCenterX;
extern int g_zInput_MouseClientCenterY;
extern float g_zInput_MouseInvClientCenterX;
extern float g_zInput_MouseInvClientCenterY;
extern float g_zInput_MouseSensitivityX;
extern float g_zInput_MouseSensitivityY;
extern int g_zInput_MouseWrapModeFlag;
extern int g_zInput_KbdDikToAsciiTable[0x100];
extern int g_zInput_KbdDikToAsciiTableReady;
extern zInput_FFEffectSet *g_zInputFfEffectSet;
extern zInput_GameStateOrMapTablePartial *g_GameStateOrMapTable;
extern float g_zInput_DiPitchAngleLowpassRad;

int __cdecl zInputKeyboardIsUnsuspended();
int __fastcall zInputWaitForAnyKeyPressWithTimeoutMs(int timeoutMs);
int __cdecl zInputDIHasForceFeedback();
int __cdecl zInputDIIsForceFeedbackEnabled();
zInput_DiEffect *__fastcall zInputDICreateForceFeedbackEffect(
    const GUID *rguidEffect,
    const DIEFFECT *effect
);
zInput_DiEffect *__stdcall zInputDICreateConstantForceEffectScaled(float gain);
zInput_DiEffect *__fastcall zInputDICreateConstantForceEffectWithDirection(int direction);
zInput_DiEffect *__stdcall zInputDICreateSineEffectScaled(float gain);
void __fastcall zInputBindMapContextDispatchFromKeyboardEvent(int dikCode);
void __fastcall zInputDIRestartPrimaryFireEffect(zInput_FFEffectSet *effectSet);
void __fastcall zInputDIPlayAltFireEffect(
    zInput_FFEffectSet *effectSet,
    float gain
);
zInput_FFEffectSet *__fastcall zInputDIInitForceFeedbackEffectSet(
    zInput_FFEffectSet *effectSet
);
void __fastcall zInputDIUpdateSteerAndPitchForceEffects(zInput_FFEffectSet *effectSet);
}

#define g_zInput_GlobalState (g_zInput_GlobalStateStorage.directInput)
#define g_zInput_hWnd (g_zInput_GlobalStateStorage.hWnd)
#define g_zInput_DeviceRegistry (g_zInput_GlobalStateStorage.deviceRegistry.keyboardFlags)
#define g_zInputJoystickFlags (g_zInput_GlobalStateStorage.deviceRegistry.joystickFlags)
#define g_zInputMouseFlags (g_zInput_GlobalStateStorage.deviceRegistry.mouseFlags)
#define g_zInput_KeyboardSuspendFlags (g_zInput_DeviceRegistry)
#define g_zInput_JoystickSuspendFlags (g_zInputJoystickFlags)
#define g_zInput_MouseSuspendFlags (g_zInputMouseFlags)
#define g_zInputKeyboardPollRefCount (g_zInput_GlobalStateStorage.deviceRegistry.keyboardRefCount)
#define g_zInputJoystickPollRefCount (g_zInput_GlobalStateStorage.deviceRegistry.joystickRefCount)
#define g_zInputMousePollRefCount (g_zInput_GlobalStateStorage.deviceRegistry.mouseRefCount)
#define g_zInput_KbdSystemReady (g_zInput_GlobalStateStorage.keyboardSystemReady)
#define g_zInput_KbdDevice (g_zInput_GlobalStateStorage.keyboardDevice)
#define g_zInput_KbdEventBuffer (g_zInput_GlobalStateStorage.keyboardEventBuffer)
#define g_zInput_KbdModifierState (g_zInput_GlobalStateStorage.keyboardModifierState)
#define g_zInputKbdKeyDispatchTable (g_zInput_GlobalStateStorage.keyboardDispatchTable)
#define g_zInput_KbdRawEventCallback (g_zInput_GlobalStateStorage.keyboardRawEventCallback)
#define g_zInput_KbdRawEventCallbackCtx (g_zInput_GlobalStateStorage.keyboardRawEventCallbackCtx)
#define g_zInput_JoystickInitialized (g_zInput_GlobalStateStorage.joystickInitialized)
#define g_zInput_JoystickDevice (g_zInput_GlobalStateStorage.joystickDevice)
#define g_zInput_JoystickCurrentState (g_zInput_GlobalStateStorage.joystickCurrentState)
#define g_zInput_JoystickPreviousState (g_zInput_GlobalStateStorage.joystickPreviousState)
#define g_zInput_JoystickAxisCount (g_zInput_GlobalStateStorage.joystickAxisCount)
#define g_zInput_JoystickCaps_ForceFeedback (g_zInput_GlobalStateStorage.joystickCapsForceFeedback)
#define g_zInput_JoystickCaps_FFAttack (g_zInput_GlobalStateStorage.joystickCapsFFAttack)
#define g_zInput_JoystickCaps_FFFade (g_zInput_GlobalStateStorage.joystickCapsFFFade)
#define g_zInput_JoystickAxisConfig (g_zInput_GlobalStateStorage.joystickAxisConfig)
#define g_zInput_MouseInitialized (g_zInput_GlobalStateStorage.mouseInitialized)
#define g_zInput_MouseDevice (g_zInput_GlobalStateStorage.mouseDevice)
#define g_zInput_MouseCurrentState (g_zInput_GlobalStateStorage.mouseCurrentState)
#define g_zInput_MousePreviousState (g_zInput_GlobalStateStorage.mousePreviousState)
#define g_zInput_BindMap_Current (g_zInput_GlobalStateStorage.currentBindMap)
#define g_zInput_BindMapOverlayBlockSize (g_zInput_GlobalStateStorage.bindMapOverlayBlockSize)
#define g_zInput_BindMapOverlayNodeBlockList (g_zInput_GlobalStateStorage.bindMapOverlayNodeBlockList)
#define g_zInput_BindMapOverlayNodeFreeList (g_zInput_GlobalStateStorage.bindMapOverlayNodeFreeList)
#define g_zInput_BindMapOverlayNodeStackHead (g_zInput_GlobalStateStorage.bindMapOverlayNodeStackHead)
#define g_zInput_BindMapOverlayReserved (g_zInput_GlobalStateStorage.bindMapOverlayReserved)
#define g_zInput_BindMapOverlayDepth (g_zInput_GlobalStateStorage.bindMapOverlayDepth)

#endif
