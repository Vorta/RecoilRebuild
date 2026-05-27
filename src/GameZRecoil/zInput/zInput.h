#pragma once

#include <stddef.h>
#include "recoil/recoil_types.h"

#include <windows.h>

#include "recoil/recoil_callconv.h"

typedef void (RECOIL_CDECL *zInputCommandCallbackFn)();

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

    RECOIL_NOINLINE zInput_BindMapContext *RECOIL_THISCALL
    InitFromTemplate(const zInput_BindMapContext *tmpl);
    RECOIL_NOINLINE void RECOIL_THISCALL FreeAllBuffers();
    RECOIL_NOINLINE void RECOIL_THISCALL RebuildLookupIndices();
    RECOIL_NOINLINE void RECOIL_THISCALL InitCommandMap(int commandCount);
    RECOIL_NOINLINE void RECOIL_THISCALL FreeNonOwnedBuffers();
    RECOIL_NOINLINE void RECOIL_THISCALL ResetAllBindings();
    RECOIL_NOINLINE int RECOIL_THISCALL GetPrimaryKeyboardKey(int commandIndex);
    RECOIL_NOINLINE int RECOIL_THISCALL GetSecondaryKeyboardKey(int commandIndex);
    RECOIL_NOINLINE int RECOIL_THISCALL GetJoystickButtonSlot(int commandIndex);
    RECOIL_NOINLINE int RECOIL_THISCALL GetMouseButtonSlot(int commandIndex);
    RECOIL_NOINLINE int RECOIL_THISCALL GetCommandByPrimaryKey(int keyboardKey);
    RECOIL_NOINLINE int RECOIL_THISCALL GetCommandBySecondaryKey(int keyboardKey);
    RECOIL_NOINLINE int RECOIL_THISCALL
    GetCommandByAnyKeyboardKey(int keyboardKey);
    RECOIL_NOINLINE int RECOIL_THISCALL
    GetCommandByJoystickSlot(int joystickSlot);
    RECOIL_NOINLINE int RECOIL_THISCALL GetCommandByMouseSlot(int mouseSlot);
    RECOIL_NOINLINE void RECOIL_THISCALL SetPrimaryKeyBinding(int keyCode,
                                                              int commandId);
    RECOIL_NOINLINE void RECOIL_THISCALL SetSecondaryKeyBinding(int keyCode,
                                                                int commandId);
    RECOIL_NOINLINE void RECOIL_THISCALL SetJoystickBinding(int joystickSlot,
                                                            int commandId);
    RECOIL_NOINLINE void RECOIL_THISCALL SetMouseBinding(int mouseSlot,
                                                         int commandId);
    RECOIL_NOINLINE void RECOIL_THISCALL
    SetBindingRecord(int commandId, const char *labelSrc, int primaryKey,
                     int secondaryKey, int joystickSlot, int mouseSlot);
    RECOIL_NOINLINE void RECOIL_THISCALL DispatchMouseButtonCallbacks();
    RECOIL_NOINLINE void RECOIL_THISCALL DispatchJoystickButtonCallbacks();
    RECOIL_NOINLINE int RECOIL_THISCALL
    SetCommandCallback(int commandId, zInputCommandCallbackFn callback);
    RECOIL_NOINLINE int RECOIL_THISCALL ReadCommandInputState(int commandIndex);
    RECOIL_NOINLINE char *RECOIL_THISCALL CopyCommandLabel(int commandId, char *destBuf,
                                                           int maxBytes);
};

RECOIL_STATIC_ASSERT(offsetof(zInput_BindMapContext, m_commandCount) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindMapContext, m_packedBindings) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindMapContext, m_commandCallbacks) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindMapContext, m_commandLabels) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindMapContext, m_primaryKeyToCommand) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindMapContext, m_secondaryKeyToCommand) == 0x1f8c);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindMapContext, m_joystickToCommand) == 0x3f04);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindMapContext, m_mouseToCommand) == 0x3f44);
RECOIL_STATIC_ASSERT(sizeof(zInput_BindMapContext) == 0x3f54);

struct zInput_BindMapOverlayStackNode {
    zInput_BindMapOverlayStackNode *next;
    zInput_BindMapOverlayStackNode *prev;
    zInput_BindMapContext *bindMap;
};
RECOIL_STATIC_ASSERT(sizeof(zInput_BindMapOverlayStackNode) == 0x0c);

struct zInput_BindGroupInfo {
    char *title;
    int unknown_04;
    int *commandIdsBegin;
    int *commandIdsEnd;
    int *commandIdsCapacity;
};
RECOIL_STATIC_ASSERT(offsetof(zInput_BindGroupInfo, title) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindGroupInfo, commandIdsBegin) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindGroupInfo, commandIdsEnd) == 0x0c);

struct zInput_BindGroupInfoList {
    zInput_BindGroupInfo **begin;
    zInput_BindGroupInfo **end;
    zInput_BindGroupInfo **capacity;
};
RECOIL_STATIC_ASSERT(offsetof(zInput_BindGroupInfoList, begin) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindGroupInfoList, end) == 0x04);

struct zInput_BindGroupInfoVec {
    int unknown_00;
    zInput_BindGroupInfo **begin;
    zInput_BindGroupInfo **end;
    zInput_BindGroupInfo **capacity;

    RECOIL_NOINLINE int RECOIL_THISCALL Count();
};
RECOIL_STATIC_ASSERT(offsetof(zInput_BindGroupInfoVec, begin) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zInput_BindGroupInfoVec, end) == 0x08);

struct zInput_DiEffect;

struct zInput_DiEffectVtable {
    void *unknown_00;
    void *unknown_04;
    void *unknown_08;
    void *unknown_0c;
    void *unknown_10;
    void *unknown_14;
    int(RECOIL_STDCALL *SetParameters_18)(zInput_DiEffect *self, const void *effect,
                                                   unsigned int flags);
    int(RECOIL_STDCALL *Start_1c)(zInput_DiEffect *self, unsigned int iterations,
                                           unsigned int flags);
    int(RECOIL_STDCALL *Stop_20)(zInput_DiEffect *self);
};
RECOIL_STATIC_ASSERT(offsetof(zInput_DiEffectVtable, SetParameters_18) == 0x18);
RECOIL_STATIC_ASSERT(offsetof(zInput_DiEffectVtable, Start_1c) == 0x1c);
RECOIL_STATIC_ASSERT(offsetof(zInput_DiEffectVtable, Stop_20) == 0x20);

struct zInput_DiEffect {
    zInput_DiEffectVtable *vtbl_00;
};

struct zInput_FFEffectSet {
    zInput_DiEffect *PrimaryFire;
    zInput_DiEffect *AltFire;
    zInput_DiEffect *AmbientSine;
    zInput_DiEffect *CollisionImpact;
    zInput_DiEffect *DamageHit;
    zInput_DiEffect *SteerForce;
    zInput_DiEffect *PitchForce;

    RECOIL_NOINLINE void RECOIL_THISCALL PlayCollisionImpactEffect(
        const zVec3 *impactWorldPosXZ, float gain);
    RECOIL_NOINLINE void RECOIL_THISCALL PlayDamageHitEffect(
        const zVec3 *damageSourceWorldPosXZ, float gain);
};
RECOIL_STATIC_ASSERT(offsetof(zInput_FFEffectSet, PrimaryFire) == 0x00);
RECOIL_STATIC_ASSERT(offsetof(zInput_FFEffectSet, AltFire) == 0x04);
RECOIL_STATIC_ASSERT(offsetof(zInput_FFEffectSet, AmbientSine) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(zInput_FFEffectSet, CollisionImpact) == 0x0c);
RECOIL_STATIC_ASSERT(offsetof(zInput_FFEffectSet, DamageHit) == 0x10);
RECOIL_STATIC_ASSERT(offsetof(zInput_FFEffectSet, SteerForce) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(zInput_FFEffectSet, PitchForce) == 0x18);
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
RECOIL_STATIC_ASSERT(offsetof(zInput_PlayerStatePartial, angVelYaw) == 0x94);
RECOIL_STATIC_ASSERT(offsetof(zInput_PlayerStatePartial, yawVelocityLimit) == 0xcc);
RECOIL_STATIC_ASSERT(offsetof(zInput_PlayerStatePartial, pitchAngleRad) == 0x3bc);
RECOIL_STATIC_ASSERT(offsetof(zInput_PlayerStatePartial, cameraDirNextX) == 0x520);
RECOIL_STATIC_ASSERT(offsetof(zInput_PlayerStatePartial, cameraDirNextZ) == 0x528);

struct zInput_GameStateOrMapTablePartial {
    void *unknown_00;
    zInput_PlayerStatePartial *playerState;
};
RECOIL_STATIC_ASSERT(offsetof(zInput_GameStateOrMapTablePartial, playerState) == 0x04);

namespace zInput {
struct DIDirectInput;
struct DIDevice;
struct DIDeviceInstance;
struct DIDeviceObjectData;

struct DIDeviceVtable {
    int(RECOIL_STDCALL *QueryInterface_00)(DIDevice *self, const GUID *iid,
                                                    DIDevice **outDevice);
    void *AddRef_04;
    int(RECOIL_STDCALL *Release_08)(DIDevice *self);
    int(RECOIL_STDCALL *GetCapabilities_0c)(DIDevice *self, void *capabilities);
    void *EnumObjects_10;
    int(RECOIL_STDCALL *GetProperty_14)(DIDevice *self, unsigned int property,
                                                 void *propHeader);
    int(RECOIL_STDCALL *SetProperty_18)(DIDevice *self, unsigned int property,
                                                 void *propHeader);
    int(RECOIL_STDCALL *Acquire_1c)(DIDevice *self);
    int(RECOIL_STDCALL *Unacquire_20)(DIDevice *self);
    int(RECOIL_STDCALL *GetDeviceState_24)(DIDevice *self, unsigned int cbData,
                                                    void *outState);
    int(RECOIL_STDCALL *GetDeviceData_28)(DIDevice *self, unsigned int cbObjectData,
                                                   DIDeviceObjectData *rgdod,
                                                   unsigned int *pdwInOut, unsigned int flags);
    int(RECOIL_STDCALL *SetDataFormat_2c)(DIDevice *self, const void *dataFormat);
    void *SetEventNotification_30;
    int(RECOIL_STDCALL *SetCooperativeLevel_34)(DIDevice *self, HWND hwnd,
                                                         unsigned int flags);
    void *unknown_38;
    void *unknown_3c;
    void *unknown_40;
    void *unknown_44;
    int(RECOIL_STDCALL *CreateEffect_48)(DIDevice *self, const GUID *rguid,
                                                  const void *effect, zInput_DiEffect **outEffect,
                                                  void *outer);
    void *unknown_4c;
    void *unknown_50;
    void *unknown_54;
    void *unknown_58;
    void *unknown_5c;
    void *unknown_60;
    int(RECOIL_STDCALL *Poll_64)(DIDevice *self);
};
RECOIL_STATIC_ASSERT(offsetof(DIDeviceVtable, GetDeviceState_24) == 0x24);
RECOIL_STATIC_ASSERT(offsetof(DIDeviceVtable, CreateEffect_48) == 0x48);
RECOIL_STATIC_ASSERT(offsetof(DIDeviceVtable, Poll_64) == 0x64);

struct DIDevice {
    const DIDeviceVtable *vtbl_00;
};

typedef int (RECOIL_STDCALL *DIEnumDevicesCallback)(const DIDeviceInstance *instance,
                                                             void *ref);

struct DIDirectInputVtable {
    void *QueryInterface_00;
    void *AddRef_04;
    int(RECOIL_STDCALL *Release_08)(DIDirectInput *self);
    int(RECOIL_STDCALL *CreateDevice_0c)(DIDirectInput *self, const GUID *guid,
                                                  DIDevice **outDevice, void *outer);
    int(RECOIL_STDCALL *EnumDevices_10)(DIDirectInput *self, unsigned int deviceType,
                                                 DIEnumDevicesCallback callback, void *ref,
                                                 unsigned int flags);
};

struct DIDirectInput {
    const DIDirectInputVtable *vtbl_00;
};

struct DIDeviceInstance {
    unsigned int dwSize;
    GUID guidInstance;
};
RECOIL_STATIC_ASSERT(offsetof(DIDeviceInstance, guidInstance) == 0x04);

struct DIDeviceObjectData {
    unsigned int dwOfs;
    unsigned int dwData;
    unsigned int dwTimeStamp;
    unsigned int dwSequence;
};
RECOIL_STATIC_ASSERT(sizeof(DIDeviceObjectData) == 0x10);

struct KbdKeyDispatchEntry {
    int state;
    void *callback;
};
RECOIL_STATIC_ASSERT(sizeof(KbdKeyDispatchEntry) == 0x08);

struct JoystickStatePartial {
    int lX;
    int lY;
    int lZ;
    int lRx;
    int lRy;
    int lRz;
    int rglSlider[2];
    unsigned int rgdwPOV[4];
    unsigned char rgbButtons[0x80];
    int lVX;
    int lVY;
    int lVZ;
    int lVRx;
    int lVRy;
    int lVRz;
    int rglVSlider[2];
    int lAX;
    int lAY;
    int lAZ;
    int lARx;
    int lARy;
    int lARz;
    int rglASlider[2];
    int lFX;
    int lFY;
    int lFZ;
    int lFRx;
    int lFRy;
    int lFRz;
    int rglFSlider[2];
};

RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, lZ) == 0x08);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, lRz) == 0x14);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, rgdwPOV) == 0x20);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, rgbButtons) == 0x30);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, lVZ) == 0xb8);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, lVRz) == 0xc4);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, lAZ) == 0xd8);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, lARz) == 0xe4);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, lFZ) == 0xf8);
RECOIL_STATIC_ASSERT(offsetof(JoystickStatePartial, lFRz) == 0x104);
RECOIL_STATIC_ASSERT(sizeof(JoystickStatePartial) == 0x110);

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
RECOIL_STATIC_ASSERT(offsetof(DIDeviceCaps, dwAxes) == 0x0c);

struct MouseDeviceState {
    int lX;
    int lY;
    int lZ;
    unsigned int rgbButtons;
};

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

void RECOIL_CDECL Mouse_UpdateAcquireState();
int RECOIL_CDECL Mouse_ShutdownDevice();
void RECOIL_CDECL Mouse_ApplyClientCursorPosToOS();
void RECOIL_CDECL Mouse_UpdateClientRectAndCenter();
void RECOIL_CDECL Mouse_RecenterCursor();
void RECOIL_CDECL Mouse_RecenterCursorX();
void RECOIL_STDCALL Mouse_SetNormalizedCursorPos(float normX, float normY);
int RECOIL_CDECL Mouse_IsInitialized();
int RECOIL_CDECL Mouse_InitDevice();
void RECOIL_FASTCALL Mouse_PollAndStoreState(int dispatchCallbacks);
int RECOIL_FASTCALL Mouse_PollState(int dispatchCallbacks);
int RECOIL_CDECL Mouse_AddRef();
int RECOIL_FASTCALL Mouse_GetButtonTransitionState(int buttonNumber);
int RECOIL_FASTCALL Mouse_WaitForButtonPress(int pollUntilFound);
MouseStateSnapshot *RECOIL_CDECL Mouse_GetStateSnapshotPtr();
int RECOIL_FASTCALL Mouse_GetStateSnapshot(MouseStateSnapshot *outState);
int RECOIL_CDECL Keyboard_ShutdownDevice();
int RECOIL_CDECL Joystick_ShutdownDevice();
int RECOIL_CDECL Shutdown();
void RECOIL_CDECL Mouse_ApplyAccumulatedDelta();
void RECOIL_CDECL Mouse_ResetTransitionState();
int RECOIL_CDECL Mouse_IsUnsuspended();
int RECOIL_CDECL Joystick_IsUnsuspended();
int RECOIL_CDECL Keyboard_IsUnsuspended();
void RECOIL_CDECL Mouse_Suspend();
void RECOIL_CDECL Joystick_Suspend();
void RECOIL_CDECL Keyboard_Suspend();
void RECOIL_CDECL Mouse_ResumeFromSuspend();
void RECOIL_CDECL Keyboard_ResetTransitionState();
void RECOIL_CDECL Keyboard_ResumeFromSuspend();
void RECOIL_CDECL Keyboard_ClearKeyCallbackTable();
void RECOIL_FASTCALL Keyboard_PollState(int dispatchCallbacks);
int RECOIL_FASTCALL Keyboard_WaitForAnyKeyPress(int keepWaiting);
void RECOIL_CDECL Keyboard_InitDikToAsciiTable();
int RECOIL_FASTCALL Keyboard_TranslateDikToAscii(int comboIdx);
void RECOIL_FASTCALL Keyboard_SetRawEventCallback(void *callback, void *context);
int RECOIL_FASTCALL Keyboard_GetKeyTransitionState(int keyIndex);
int RECOIL_FASTCALL Keyboard_RegisterKeyCallback(int comboIdx, void *callback,
                                                          const char *unusedLabel);
void RECOIL_FASTCALL Keyboard_UnregisterKeyCallback(int comboIdx);
void RECOIL_CDECL ResetAllTransitionState();
int RECOIL_CDECL Keyboard_InitDevice();
int RECOIL_CDECL Keyboard_AddRef();
void RECOIL_CDECL DI_ResetTransitionState();
void RECOIL_CDECL Joystick_ResumeFromSuspend();
int RECOIL_FASTCALL Init(HWND hWnd, HINSTANCE hInstance);
void RECOIL_CDECL BindMap_InitDikKeyNameTable();
void RECOIL_CDECL BindMap_InitJoystickButtonNameTable();
void RECOIL_CDECL BindMap_InitMouseButtonNameTable();
int RECOIL_CDECL DI_AddJoystickRef();
int RECOIL_CDECL DI_ReleaseJoystickRef();
int RECOIL_CDECL DI_GetJoystickRefCount();
int RECOIL_FASTCALL DI_GetButtonTransitionState(int buttonIndex);
int RECOIL_STDCALL
DI_EnumDevicesCallback_SelectFirstJoystick(const DIDeviceInstance *instance, void *ref);
int RECOIL_CDECL DI_AcquireJoystickDevice();
int RECOIL_FASTCALL DI_InitJoystickDevice(HWND hwnd);
int RECOIL_FASTCALL DI_ApplyAxisConfig(JoystickAxisConfig *axisCfg);
int RECOIL_FASTCALL DI_SetAxisDeadzone(int axisOffset, int deadzone);
int RECOIL_FASTCALL DI_SetAxisRange(int axisOffset, int rangeMin,
                                             int rangeMax);
int RECOIL_FASTCALL DI_GetAxisRange(int axisOffset, int *pOutMin,
                                             int *pOutMax);
int RECOIL_CDECL DI_IsJoystickDeviceReady();
JoystickStatePartial *RECOIL_CDECL DI_GetCurrentState();
JoystickStatePartial *RECOIL_FASTCALL DI_PollJoystickState(int dispatchCallbacks);
int RECOIL_FASTCALL DI_SetJoystickEnabled(int enable);
int RECOIL_FASTCALL DI_WaitForButtonPress(int loopUntilPressed);
int RECOIL_FASTCALL DI_ReportError(int hresult, const char *sourceFile,
                                            int sourceLine);
int RECOIL_FASTCALL BindMap_PackBindingCode(int primary, int secondary,
                                                     int joy, int mouse);
int RECOIL_CDECL BindGroupList_GetCount();
char *RECOIL_FASTCALL BindGroupList_GetGroupTitle(int groupIndex);
int RECOIL_FASTCALL BindGroupList_GetGroupCommandCount(int groupIndex);
int RECOIL_FASTCALL BindGroupList_GetGroupCommandId(int groupIndex,
                                                             int commandIndex);
void RECOIL_FASTCALL BindGroupInfo_Destroy(zInput_BindGroupInfo *group);
void RECOIL_CDECL BindGroupList_Clear();
int RECOIL_FASTCALL BindGroupList_AddGroup(const char *title);
void RECOIL_FASTCALL BindGroupList_AddCommandToGroup(int groupIndex,
                                                     int commandId);
char *RECOIL_FASTCALL BindMap_GetCommandLabel(int commandId);
char *RECOIL_FASTCALL BindMap_GetCommandHint(int commandId);
void RECOIL_FASTCALL BindMap_AddDefaultBinding(int commandId, int messageId,
                                               int primaryKey, int secondaryKey,
                                               int joystickSlot, int mouseSlot);
int RECOIL_CDECL BindMap_InitDefaultBindings();
void RECOIL_FASTCALL BindMapSystem_Init(int commandCount);
void RECOIL_CDECL BindMapSystem_Shutdown();
void RECOIL_FASTCALL BindMapContext_Push(zInput_BindMapContext *bindMapOrNull);
void RECOIL_CDECL BindMapContext_Pop();
void RECOIL_CDECL BindMap_Current_RebuildLookupIndices();
void RECOIL_CDECL BindMapCurrent_ResetAllBindings();
int RECOIL_FASTCALL BindMapCurrent_GetPrimaryKeyboardKey(int commandIndex);
int RECOIL_FASTCALL BindMapCurrent_GetSecondaryKeyboardKey(int commandIndex);
int RECOIL_FASTCALL BindMapCurrent_GetJoystickButtonSlot(int commandIndex);
int RECOIL_FASTCALL BindMapCurrent_GetMouseButtonSlot(int commandIndex);
int RECOIL_FASTCALL BindMapCurrent_GetCommandByPrimaryKey(int keyboardKey);
int RECOIL_FASTCALL BindMapCurrent_GetCommandBySecondaryKey(int keyboardKey);
int RECOIL_FASTCALL BindMapCurrent_GetCommandByJoystickSlot(int joystickSlot);
int RECOIL_FASTCALL BindMapCurrent_GetCommandByMouseSlot(int mouseSlot);
void RECOIL_FASTCALL BindMapCurrent_SetPrimaryKeyBinding(int keyCode,
                                                         int commandId);
void RECOIL_FASTCALL BindMapCurrent_SetSecondaryKeyBinding(int keyCode,
                                                           int commandId);
void RECOIL_FASTCALL BindMapCurrent_SetJoystickBinding(int joystickSlot,
                                                       int commandId);
void RECOIL_FASTCALL BindMapCurrent_SetMouseBinding(int mouseSlot, int commandId);
int RECOIL_FASTCALL BindMap_Current_SetBindingRecord(
    int commandId, const char *labelSrc, int primaryKey,
    int secondaryKey, int joystickSlot, int mouseSlot);
int RECOIL_FASTCALL BindMap_Current_SetCommandCallback(int commandId,
                                                                zInputCommandCallbackFn callback);
int RECOIL_FASTCALL BindMap_Current_ReadCommandInputState(int commandIndex);
char *RECOIL_FASTCALL BindMapCurrent_CopyCommandLabel(int commandId, char *destBuf,
                                                      int maxBytes);
char *RECOIL_STDCALL BindMap_FormatKeyComboName(int packedKey, char *destBuf,
                                                int maxBytes);
char *RECOIL_STDCALL BindMap_CopyJoystickButtonName(int joystickSlot, char *outBuf,
                                                    int bufSize);
char *RECOIL_STDCALL BindMap_CopyMouseButtonName(int mouseSlot, char *outBuf,
                                                 int bufSize);
char *RECOIL_FASTCALL BindMapCurrent_FormatKeyComboName(int packedKey, char *destBuf,
                                                        int maxBytes);
char *RECOIL_FASTCALL BindMapCurrent_CopyJoystickButtonName(int joystickSlot, char *outBuf,
                                                            int bufSize);
char *RECOIL_FASTCALL BindMapCurrent_CopyMouseButtonName(int mouseSlot, char *outBuf,
                                                         int bufSize);
int RECOIL_FASTCALL Mouse_SetCooperativeLevelFlags(int flags);
void RECOIL_FASTCALL Mouse_SetClientSizeAndCenter(int width, int height);
void RECOIL_FASTCALL PollActiveDevices(int dispatchCallbacks);
void RECOIL_CDECL OnAppActivate();
void RECOIL_CDECL OnAppDeactivate();
} // namespace zInput

namespace zInp {
RECOIL_NOINLINE void RECOIL_FASTCALL SetJoystickOption(int enabled);
RECOIL_NOINLINE void RECOIL_FASTCALL SetJoystickAxesCountOption(int axisCount);
RECOIL_NOINLINE void RECOIL_FASTCALL SetJoystickButtonCountOption(int buttonCount);
RECOIL_NOINLINE int RECOIL_CDECL GetJoystickOption();
} // namespace zInp

extern "C" {
extern const char *g_zInput_DikKeyNames[0x100];
extern const char *g_zInput_JoystickButtonNames[9];
extern const char *g_zInput_MouseButtonNames[4];
extern zInput_BindMapContext *g_zInput_BindMap_Current;
extern zInput_BindMapOverlayStackNode *g_zInput_BindMapOverlayNodeFreeList;
extern zInput_BindMapOverlayStackNode *g_zInput_BindMapOverlayNodeStackHead;
extern int g_zInput_BindMapOverlayDepth;
extern zInput_BindGroupInfoList g_zInput_BindGroupInfoList;
extern int g_zInput_CurrentBindGroupIndex;
extern int g_zInput_CommandLocIdTable[0x100];
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
extern zInput::JoystickStatePartial g_zInput_JoystickCurrentState;
extern zInput::JoystickStatePartial g_zInput_JoystickPreviousState;
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

int RECOIL_CDECL zInput_Keyboard_IsUnsuspended();
int RECOIL_FASTCALL zInput_WaitForAnyKeyPressWithTimeoutMs(int timeoutMs);
int RECOIL_CDECL zInput_DI_HasForceFeedback();
int RECOIL_CDECL zInput_DI_IsForceFeedbackEnabled();
zInput_DiEffect *RECOIL_FASTCALL zInput_DI_CreateForceFeedbackEffect(const GUID *rguidEffect,
                                                                     const void *effect);
zInput_DiEffect *RECOIL_STDCALL zInput_DI_CreateConstantForceEffectScaled(float gain);
zInput_DiEffect *RECOIL_FASTCALL
zInput_DI_CreateConstantForceEffectWithDirection(int direction);
zInput_DiEffect *RECOIL_STDCALL zInput_DI_CreateSineEffectScaled(float gain);
void RECOIL_FASTCALL zInput_BindMapContext_DispatchFromKeyboardEvent(int dikCode);
void RECOIL_FASTCALL zInput_DI_RestartPrimaryFireEffect(zInput_FFEffectSet *effectSet);
void RECOIL_FASTCALL zInput_DI_PlayAltFireEffect(zInput_FFEffectSet *effectSet, float gain);
zInput_FFEffectSet *RECOIL_FASTCALL
zInput_DI_InitForceFeedbackEffectSet(zInput_FFEffectSet *effectSet);
void RECOIL_CDECL zInput_DI_PlayCollisionImpactEffect(zInput_FFEffectSet *effectSet,
                                                      const zVec3 *impactWorldPosXZ, float gain);
void RECOIL_CDECL zInput_DI_PlayDamageHitEffect(zInput_FFEffectSet *effectSet,
                                                const zVec3 *damageSourceWorldPosXZ, float gain);
void RECOIL_FASTCALL zInput_DI_UpdateSteerAndPitchForceEffects(zInput_FFEffectSet *effectSet);
}
