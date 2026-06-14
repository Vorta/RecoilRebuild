#pragma once

#include "recoil/recoil_types.h"
#include <stddef.h>

#include "Battlesport/Mfc42Abi.h"
#include <windows.h>

#include "recoil/recoil_callconv.h"

#ifndef DIRECTINPUT_VERSION
#define DIRECTINPUT_VERSION 0x0500
#endif
#include <dinput.h>

typedef void(__fastcall *zInputCommandCallbackFn)(int commandId);

struct zVec3;

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

struct zInput_CommandIdVector {
    unsigned char allocatorByte;
    unsigned char allocatorPadding[3];
    int *begin;
    int *end;
    int *capacity;
};
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_CommandIdVector,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_CommandIdVector,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zInput_CommandIdVector) == 0x10);

struct zInput_BindGroupInfo {
    CString title;
    zInput_CommandIdVector commandIds;

    void Destroy();
};
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindGroupInfo,
        title
    ) == 0x00
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindGroupInfo,
        commandIds
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_CommandIdVector,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_CommandIdVector,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zInput_BindGroupInfo) == 0x14);

struct zInput_BindGroupInfoList {
    unsigned char allocatorByte;
    unsigned char allocatorPadding[3];
    zInput_BindGroupInfo **begin;
    zInput_BindGroupInfo **end;
    zInput_BindGroupInfo **capacity;
};
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindGroupInfoList,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindGroupInfoList,
        end
    ) == 0x08
);
RECOIL_STATIC_ASSERT(sizeof(zInput_BindGroupInfoList) == 0x10);

struct zInput_BindGroupInfoVec {
    int unknown_00;
    zInput_BindGroupInfo **begin;
    zInput_BindGroupInfo **end;
    zInput_BindGroupInfo **capacity;

    int Count();
};
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindGroupInfoVec,
        begin
    ) == 0x04
);
RECOIL_STATIC_ASSERT(
    offsetof(
        zInput_BindGroupInfoVec,
        end
    ) == 0x08
);

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

void Mouse_UpdateAcquireState();
int Mouse_ShutdownDevice();
void Mouse_ApplyClientCursorPosToOS();
void Mouse_UpdateClientRectAndCenter();
void Mouse_RecenterCursor();
void Mouse_RecenterCursorX();
void __stdcall Mouse_SetNormalizedCursorPos(
    float normX,
    float normY
);
int Mouse_IsInitialized();
int Mouse_InitDevice();
void __fastcall Mouse_PollAndStoreState(unsigned char dispatchCallbacks);
int __fastcall Mouse_PollState(unsigned char dispatchCallbacks);
int Mouse_AddRef();
int __fastcall Mouse_GetButtonTransitionState(int buttonNumber);
int __fastcall Mouse_WaitForButtonPress(int pollUntilFound);
MouseStateSnapshot *Mouse_GetStateSnapshotPtr();
int __fastcall Mouse_GetStateSnapshot(MouseStateSnapshot *outState);
int Keyboard_ShutdownDevice();
int Joystick_ShutdownDevice();
int Shutdown();
void Mouse_ApplyAccumulatedDelta();
void Mouse_ResetTransitionState();
int Mouse_IsUnsuspended();
int Joystick_IsUnsuspended();
int Keyboard_IsUnsuspended();
void Mouse_Suspend();
void Joystick_Suspend();
void Keyboard_Suspend();
void Mouse_ResumeFromSuspend();
void Keyboard_ResetTransitionState();
void Keyboard_ResumeFromSuspend();
void Keyboard_ClearKeyCallbackTable();
void __fastcall Keyboard_PollState(unsigned char dispatchCallbacks);
int __fastcall Keyboard_WaitForAnyKeyPress(int keepWaiting);
void Keyboard_InitDikToAsciiTable();
int __fastcall Keyboard_TranslateDikToAscii(int comboIdx);
void __fastcall Keyboard_SetRawEventCallback(
    void *callback,
    void *context
);
int __fastcall Keyboard_GetKeyTransitionState(int keyIndex);
int __fastcall Keyboard_RegisterKeyCallback(
    int comboIdx,
    void *callback,
    const char *unusedLabel
);
void __fastcall Keyboard_UnregisterKeyCallback(int comboIdx);
void ResetAllTransitionState();
int Keyboard_InitDevice();
int Keyboard_AddRef();
void DI_ResetTransitionState();
void Joystick_ResumeFromSuspend();
int __fastcall Init(
    HWND hWnd,
    HINSTANCE hInstance
);
void BindMap_InitDikKeyNameTable();
void BindMap_InitJoystickButtonNameTable();
void BindMap_InitMouseButtonNameTable();
int DI_AddJoystickRef();
int DI_ReleaseJoystickRef();
int DI_GetJoystickRefCount();
int __fastcall DI_GetButtonTransitionState(int buttonIndex);
int __stdcall DI_EnumDevicesCallback_SelectFirstJoystick(
    const DIDeviceInstance *instance,
    void *ref
);
int DI_AcquireJoystickDevice();
int __fastcall DI_InitJoystickDevice(HWND hwnd);
int __fastcall DI_ApplyAxisConfig(JoystickAxisConfig *axisCfg);
int __fastcall DI_SetAxisDeadzone(
    int axisOffset,
    int deadzone
);
int __fastcall DI_SetAxisRange(
    int axisOffset,
    int rangeMin,
    int rangeMax
);
int __fastcall DI_GetAxisRange(
    int axisOffset,
    int *pOutMin,
    int *pOutMax
);
int DI_IsJoystickDeviceReady();
DIJOYSTATE2 *DI_GetCurrentState();
DIJOYSTATE2 *__fastcall DI_PollJoystickState(unsigned char dispatchCallbacks);
int __fastcall DI_SetJoystickEnabled(int enable);
int __fastcall DI_WaitForButtonPress(int loopUntilPressed);
int __fastcall DI_ReportError(
    int hresult,
    const char *sourceFile,
    int sourceLine
);
int __fastcall BindMap_PackBindingCode(
    int primary,
    int secondary,
    int joy,
    int mouse
);
void *__fastcall GlobalStateConstructor(void *self);
void __fastcall GlobalStateDestructor(void *self);
void *GlobalStateStaticInit();
int GlobalStateRegisterAtExit();
void GlobalStateAtExitDestructor();
int GlobalStateStaticInitAndRegisterAtExit();
void BindGroupListStaticInit();
int BindGroupListRegisterAtExit();
void BindGroupListAtExitDestructor();
int BindGroupList_StaticInitAndRegisterAtExit();
int BindGroupList_GetCount();
char *__fastcall BindGroupList_GetGroupTitle(int groupIndex);
int __fastcall BindGroupList_GetGroupCommandCount(int groupIndex);
int __fastcall BindGroupList_GetGroupCommandId(
    int groupIndex,
    int commandIndex
);
void BindGroupList_Clear();
int __fastcall BindGroupList_AddGroup(const char *title);
void __fastcall BindGroupList_AddCommandToGroup(
    int groupIndex,
    int commandId
);
char *__fastcall BindMap_GetCommandLabel(int commandId);
char *__fastcall BindMap_GetCommandHint(int commandId);
void __fastcall BindMap_AddDefaultBinding(
    int commandId,
    int messageId,
    int primaryKey,
    int secondaryKey,
    int joystickSlot,
    int mouseSlot
);
int BindMap_InitDefaultBindings();
void __fastcall BindMapSystem_Init(int commandCount);
void BindMapSystem_Shutdown();
void __fastcall BindMapContext_Push(zInput_BindMapContext *bindMapOrNull);
void BindMapContext_Pop();
void BindMap_Current_RebuildLookupIndices();
void BindMapCurrent_ResetAllBindings();
int __fastcall BindMapCurrent_GetPrimaryKeyboardKey(int commandIndex);
int __fastcall BindMapCurrent_GetSecondaryKeyboardKey(int commandIndex);
int __fastcall BindMapCurrent_GetJoystickButtonSlot(int commandIndex);
int __fastcall BindMapCurrent_GetMouseButtonSlot(int commandIndex);
int __fastcall BindMapCurrent_GetCommandByPrimaryKey(int keyboardKey);
int __fastcall BindMapCurrent_GetCommandBySecondaryKey(int keyboardKey);
int __fastcall BindMapCurrent_GetCommandByJoystickSlot(int joystickSlot);
int __fastcall BindMapCurrent_GetCommandByMouseSlot(int mouseSlot);
void __fastcall BindMapCurrent_SetPrimaryKeyBinding(
    int keyCode,
    int commandId
);
void __fastcall BindMapCurrent_SetSecondaryKeyBinding(
    int keyCode,
    int commandId
);
void __fastcall BindMapCurrent_SetJoystickBinding(
    int joystickSlot,
    int commandId
);
void __fastcall BindMapCurrent_SetMouseBinding(
    int mouseSlot,
    int commandId
);
int __fastcall BindMap_Current_SetBindingRecord(
    int commandId,
    const char *labelSrc,
    int primaryKey,
    int secondaryKey,
    int joystickSlot,
    int mouseSlot
);
int __fastcall BindMap_Current_SetCommandCallback(
    int commandId,
    zInputCommandCallbackFn callback
);
int __fastcall BindMap_Current_ReadCommandInputState(int commandIndex);
char *__fastcall BindMapCurrent_CopyCommandLabel(
    int commandId,
    char *destBuf,
    int maxBytes
);
char *__stdcall BindMap_FormatKeyComboName(
    int packedKey,
    char *destBuf,
    int maxBytes
);
char *__stdcall BindMap_CopyJoystickButtonName(
    int joystickSlot,
    char *outBuf,
    int bufSize
);
char *__stdcall BindMap_CopyMouseButtonName(
    int mouseSlot,
    char *outBuf,
    int bufSize
);
char *__fastcall BindMapCurrent_FormatKeyComboName(
    int packedKey,
    char *destBuf,
    int maxBytes
);
char *__fastcall BindMapCurrent_CopyJoystickButtonName(
    int joystickSlot,
    char *outBuf,
    int bufSize
);
char *__fastcall BindMapCurrent_CopyMouseButtonName(
    int mouseSlot,
    char *outBuf,
    int bufSize
);
int __fastcall Mouse_SetCooperativeLevelFlags(int flags);
void __fastcall Mouse_SetClientSizeAndCenter(
    int width,
    int height
);
void __fastcall PollActiveDevices(unsigned char dispatchCallbacks);
void OnAppActivate();
void OnAppDeactivate();
} // namespace zInput

namespace zInp {
void __fastcall SetJoystickOption(int enabled);
void __fastcall SetJoystickAxesCountOption(int axisCount);
void __fastcall SetJoystickButtonCountOption(int buttonCount);
int GetJoystickOption();
} // namespace zInp

extern "C" {
extern const char *g_zInput_DikKeyNames[0x100];
extern const char *g_zInput_JoystickButtonNames[9];
extern const char *g_zInput_MouseButtonNames[4];
extern zInput_BindMapContext *g_zInput_BindMap_Current;
extern int g_zInput_BindMapOverlayBlockSize;
extern zInput_BindMapOverlayStackNode *g_zInput_BindMapOverlayNodeBlockList;
extern zInput_BindMapOverlayStackNode *g_zInput_BindMapOverlayNodeFreeList;
extern zInput_BindMapOverlayStackNode *g_zInput_BindMapOverlayNodeStackHead;
extern int g_zInput_BindMapOverlayReserved;
extern int g_zInput_BindMapOverlayDepth;
extern zInput_BindGroupInfoList g_zInput_BindGroupInfoList;
extern int g_zInput_CurrentBindGroupIndex;
extern int g_zInput_CommandLocIdTable[0x30];
extern zInput::DIDirectInput *g_zInput_GlobalState;
extern unsigned char g_zInput_DeviceRegistry;
extern short g_zInputKeyboardPollRefCount;
extern int g_zInput_MouseActive;
extern int g_zInput_MouseInitialized;
extern zInput::DIDevice *g_zInput_MouseDevice;
extern int g_zInput_MouseCoopLevelFlags;
extern unsigned char g_zInputMouseFlags;
extern short g_zInputMousePollRefCount;
extern unsigned char g_zInput_KeyboardSuspendFlags;
extern unsigned char g_zInput_JoystickSuspendFlags;
extern unsigned char g_zInput_MouseSuspendFlags;
extern int g_zInput_JoystickInitialized;
extern zInput::DIDevice *g_zInput_JoystickDevice;
extern unsigned char g_zInputJoystickFlags;
extern short g_zInputJoystickPollRefCount;
extern int g_zInput_JoystickAxisCount;
extern zInput::JoystickAxisConfig g_zInput_JoystickAxisConfig;
extern zInput::JoystickAxisConfig g_zInput_JoystickAxisConfig_Gameplay;
extern DIJOYSTATE2 g_zInput_JoystickCurrentState;
extern DIJOYSTATE2 g_zInput_JoystickPreviousState;
extern DIJOYSTATE2 g_zInput_JoystickRawDIState;
extern zInput::MouseDeviceState g_zInput_MouseRawDIState;
extern zInput::MouseDeviceState g_zInput_MouseCurrentState;
extern zInput::MouseDeviceState g_zInput_MousePreviousState;
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
extern HWND g_zInput_hWnd;
extern int g_zInput_KbdSystemReady;
extern zInput::DIDevice *g_zInput_KbdDevice;
extern zInput::DIDeviceObjectData *g_zInput_KbdEventBuffer;
extern int g_zInput_KbdModifierState;
extern zInput::KbdKeyDispatchEntry g_zInputKbdKeyDispatchTable[0x7de];
extern void *g_zInput_KbdRawEventCallback;
extern void *g_zInput_KbdRawEventCallbackCtx;
extern int g_zInput_KbdDikToAsciiTable[0x100];
extern int g_zInput_KbdDikToAsciiTableReady;
extern int g_zInput_JoystickCaps_ForceFeedback;
extern zInput_FFEffectSet *g_zInputFfEffectSet;
extern int g_zInput_JoystickCaps_FFAttack;
extern int g_zInput_JoystickCaps_FFFade;
extern zInput_GameStateOrMapTablePartial *g_GameStateOrMapTable;
extern float g_Player_DeltaTime;
extern float g_Player_InvDeltaTime;
extern float g_Player_DeltaTimeScaled001;
extern float g_zInput_DiPitchAngleLowpassRad;
extern int *ZOPT_INPUT_JOYSTICK;
extern int *ZOPT_JOYSTICK_NUM_AXES;
extern int *ZOPT_JOYSTICK_NUM_BUTTONS;

int zInput_Keyboard_IsUnsuspended();
int __fastcall zInput_WaitForAnyKeyPressWithTimeoutMs(int timeoutMs);
int zInput_DI_HasForceFeedback();
int zInput_DI_IsForceFeedbackEnabled();
zInput_DiEffect *__fastcall zInput_DI_CreateForceFeedbackEffect(
    const GUID *rguidEffect,
    const DIEFFECT *effect
);
zInput_DiEffect *__stdcall zInput_DI_CreateConstantForceEffectScaled(float gain);
zInput_DiEffect *__fastcall zInput_DI_CreateConstantForceEffectWithDirection(int direction);
zInput_DiEffect *__stdcall zInput_DI_CreateSineEffectScaled(float gain);
void __fastcall zInput_BindMapContext_DispatchFromKeyboardEvent(int dikCode);
void __fastcall zInput_DI_RestartPrimaryFireEffect(zInput_FFEffectSet *effectSet);
void __fastcall zInput_DI_PlayAltFireEffect(
    zInput_FFEffectSet *effectSet,
    float gain
);
zInput_FFEffectSet *__fastcall zInput_DI_InitForceFeedbackEffectSet(
    zInput_FFEffectSet *effectSet
);
void zInput_DI_PlayCollisionImpactEffect(
    zInput_FFEffectSet *effectSet,
    const zVec3 *impactWorldPosXZ,
    float gain
);
void zInput_DI_PlayDamageHitEffect(
    zInput_FFEffectSet *effectSet,
    const zVec3 *damageSourceWorldPosXZ,
    float gain
);
void __fastcall zInput_DI_UpdateSteerAndPitchForceEffects(zInput_FFEffectSet *effectSet);
}
