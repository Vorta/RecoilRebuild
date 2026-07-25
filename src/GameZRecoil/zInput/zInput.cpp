#include "zinput.h"

#include "GameZRecoil/zGame/zgame.h"

#if defined(_MSC_VER) && defined(_M_IX86)
#include <intrin.h>
#endif

#include <math.h>
#if defined(_MSC_VER) && _MSC_VER < 1200
#include <vector>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_MSC_VER) && defined(_M_IX86)
extern "C" void __cdecl _ftol();
#endif

extern "C" {
/**
 * Storage group: g_zInput_GlobalStateStorage.
 * BN types this as the static zInput_GlobalState object used by 0x4719f0,
 * 0x471ab0, 0x471a10, 0x471a20, and the input activation/suspend helpers.
 * The device-registry flag bytes at 0x561cb8..0x561cba carry both device-ready
 * bit 0 and suspend bit 1 for keyboard, joystick, and mouse respectively;
 * compatibility field macros expose the recovered retail names used by the
 * rest of the zInput source, including the joystick force-feedback caps at
 * 0x565e18..0x565e20 and the current bind-map/overlay lifetime fields at
 * 0x565ea0..0x565eb8.
 * Purpose: Owns the zero-filled zInput static aggregate from DirectInput root
 * state through bind-map overlay lifetime state.
 */
zInput_GlobalState g_zInput_GlobalStateStorage = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mouseactive
 * @recoil-artifact defines .data recoil:data:0x561c74: g_zInput_MouseActive.
 * BN types this as the zero-filled mouse-active flag toggled by zInput app
 * activation/deactivation and DirectInput acquire-state recovery.
 * Purpose: Tracks whether mouse input should be acquired while the app window
 * is active.
 */
int g_zInput_MouseActive = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousesensitivityx
 * @recoil-artifact defines .data recoil:data:0x4e08f4: g_zInput_MouseSensitivityX.
 * BN stores this initialized float as 1.3 and Mouse_ApplyAccumulatedDelta uses
 * it to scale horizontal mouse movement.
 * Purpose: Stores the horizontal mouse sensitivity multiplier.
 */
float g_zInput_MouseSensitivityX = 1.3f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousesensitivityy
 * @recoil-artifact defines .data recoil:data:0x4e08f8: g_zInput_MouseSensitivityY.
 * BN stores this initialized float as 1.3 and Mouse_ApplyAccumulatedDelta uses
 * it to scale vertical mouse movement.
 * Purpose: Stores the vertical mouse sensitivity multiplier.
 */
float g_zInput_MouseSensitivityY = 1.3f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousecooplevelflags
 * @recoil-artifact defines .data recoil:data:0x4e08fc: g_zInput_MouseCoopLevelFlags.
 * BN types this as the initialized DirectInput cooperative-level flags word
 * passed to the mouse device and updated by Mouse_SetCooperativeLevelFlags.
 * Purpose: Stores the current mouse cooperative-level flags.
 */
int g_zInput_MouseCoopLevelFlags = DISCL_EXCLUSIVE | DISCL_FOREGROUND;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickaxisconfig-gameplay
 * @recoil-artifact defines .data recoil:data:0x4f3350: g_zInput_JoystickAxisConfig_Gameplay.
 * BN types this as a separate zero-filled zInput::JoystickAxisConfig record
 * used by 0x42e170 before applying gameplay joystick axis ranges.
 * Purpose: Stores the gameplay joystick axis calibration copied into the
 * DirectInput device configuration.
 */
zInput::JoystickAxisConfig g_zInput_JoystickAxisConfig_Gameplay = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickrawdistate
 * @recoil-artifact defines .data recoil:data:0x566310: g_zInput_JoystickRawDIState.
 * BN types this as the zero-filled DIJOYSTATE2 scratch buffer passed to
 * IDirectInputDevice::GetDeviceState before joystick current-state updates.
 * Purpose: Stores the latest raw DirectInput joystick state read from the provider.
 */
DIJOYSTATE2 g_zInput_JoystickRawDIState = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mouserawdistate
 * @recoil-artifact defines .data recoil:data:0x561c58: g_zInput_MouseRawDIState.
 * BN types this as the zero-filled mouse GetDeviceState scratch buffer copied
 * into the current mouse state after successful polling.
 * Purpose: Stores the latest raw DirectInput mouse state read from the provider.
 */
zInput::MouseDeviceState g_zInput_MouseRawDIState = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousestatesnapshot
 * @recoil-artifact defines .data recoil:data:0x561c80: g_zInput_MouseStateSnapshot.
 * BN types this as the shared mouse snapshot returned to callers after polling,
 * cursor recentering, and normalized-coordinate updates.
 * Purpose: Stores the derived mouse state consumed by gameplay input code.
 */
zInput::MouseStateSnapshot g_zInput_MouseStateSnapshot = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinputmouselastpollresult
 * @recoil-artifact defines .data recoil:data:0x4e0900: g_zInputMouseLastPollResult.
 * BN types this initialized HRESULT-style word as the latest mouse poll result
 * returned through Mouse_GetStateSnapshot.
 * Purpose: Preserves the most recent DirectInput mouse poll status.
 */
int g_zInputMouseLastPollResult = 1;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-commandmapkeyname
 * @recoil-artifact defines .data recoil:data:0x4e0904: g_zInput_CommandMapKeyName.
 * BN types this writable char[0x7] as the "CmdMap" option name passed by
 * zInput_BindMapContext::InitCommandMap to Options_GetOrCreateOption.
 * Purpose: Names the persisted command-map option payload.
 */
char g_zInput_CommandMapKeyName[7] = "CmdMap";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameshiftprefix
 * @recoil-artifact defines .data recoil:data:0x4e090c: g_zInput_KeyNameShiftPrefix.
 * Purpose: Stores the writable Shift modifier prefix for bind-map key names.
 */
char g_zInput_KeyNameShiftPrefix[7] = "Shift-";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamealtprefix
 * @recoil-artifact defines .data recoil:data:0x4e0914: g_zInput_KeyNameAltPrefix.
 * Purpose: Stores the writable Alt modifier prefix for bind-map key names.
 */
char g_zInput_KeyNameAltPrefix[5] = "Alt-";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamectrlprefix
 * @recoil-artifact defines .data recoil:data:0x4e091c: g_zInput_KeyNameCtrlPrefix.
 * Purpose: Stores the writable Ctrl modifier prefix for bind-map key names.
 */
char g_zInput_KeyNameCtrlPrefix[6] = "Ctrl-";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameapps
 * @recoil-artifact defines .data recoil:data:0x4e0924: g_zInput_KeyNameApps.
 * Purpose: Stores the writable APPS DIK backing key name.
 */
char g_zInput_KeyNameApps[5] = "APPS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamerwin
 * @recoil-artifact defines .data recoil:data:0x4e092c: g_zInput_KeyNameRWin.
 * Purpose: Stores the writable RWIN DIK backing key name.
 */
char g_zInput_KeyNameRWin[5] = "RWIN";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamelwin
 * @recoil-artifact defines .data recoil:data:0x4e0934: g_zInput_KeyNameLWin.
 * Purpose: Stores the writable LWIN DIK backing key name.
 */
char g_zInput_KeyNameLWin[5] = "LWIN";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamedelete
 * @recoil-artifact defines .data recoil:data:0x4e093c: g_zInput_KeyNameDelete.
 * Purpose: Stores the writable DELETE DIK backing key name.
 */
char g_zInput_KeyNameDelete[7] = "DELETE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameinsert
 * @recoil-artifact defines .data recoil:data:0x4e0944: g_zInput_KeyNameInsert.
 * Purpose: Stores the writable INSERT DIK backing key name.
 */
char g_zInput_KeyNameInsert[7] = "INSERT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenext
 * @recoil-artifact defines .data recoil:data:0x4e094c: g_zInput_KeyNameNext.
 * Purpose: Stores the writable NEXT DIK backing key name.
 */
char g_zInput_KeyNameNext[5] = "NEXT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamedown
 * @recoil-artifact defines .data recoil:data:0x4e0954: g_zInput_KeyNameDown.
 * Purpose: Stores the writable DOWN DIK backing key name.
 */
char g_zInput_KeyNameDown[5] = "DOWN";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameend
 * @recoil-artifact defines .data recoil:data:0x4e095c: g_zInput_KeyNameEnd.
 * Purpose: Stores the writable END DIK backing key name.
 */
char g_zInput_KeyNameEnd[4] = "END";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameprior
 * @recoil-artifact defines .data recoil:data:0x4e0970: g_zInput_KeyNamePrior.
 * Purpose: Stores the writable PRIOR DIK backing key name.
 */
char g_zInput_KeyNamePrior[6] = "PRIOR";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameup
 * @recoil-artifact defines .data recoil:data:0x4e0978: g_zInput_KeyNameUp.
 * Purpose: Stores the writable UP DIK backing key name.
 */
char g_zInput_KeyNameUp[3] = "UP";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamehome
 * @recoil-artifact defines .data recoil:data:0x4e097c: g_zInput_KeyNameHome.
 * Purpose: Stores the writable HOME DIK backing key name.
 */
char g_zInput_KeyNameHome[5] = "HOME";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamermenu
 * @recoil-artifact defines .data recoil:data:0x4e0984: g_zInput_KeyNameRMenu.
 * Purpose: Stores the writable RMENU DIK backing key name.
 */
char g_zInput_KeyNameRMenu[6] = "RMENU";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamesysrq
 * @recoil-artifact defines .data recoil:data:0x4e098c: g_zInput_KeyNameSysRq.
 * Purpose: Stores the writable SYSRQ DIK backing key name.
 */
char g_zInput_KeyNameSysRq[6] = "SYSRQ";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamedivide
 * @recoil-artifact defines .data recoil:data:0x4e0994: g_zInput_KeyNameDivide.
 * Purpose: Stores the writable DIVIDE DIK backing key name.
 */
char g_zInput_KeyNameDivide[7] = "DIVIDE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpadcomma
 * @recoil-artifact defines .data recoil:data:0x4e099c: g_zInput_KeyNameNumpadComma.
 * Purpose: Stores the writable NUMPADCOMMA DIK backing key name.
 */
char g_zInput_KeyNameNumpadComma[12] = "NUMPADCOMMA";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamercontrol
 * @recoil-artifact defines .data recoil:data:0x4e09a8: g_zInput_KeyNameRControl.
 * Purpose: Stores the writable RCONTROL DIK backing key name.
 */
char g_zInput_KeyNameRControl[9] = "RCONTROL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpadenter
 * @recoil-artifact defines .data recoil:data:0x4e09b4: g_zInput_KeyNameNumpadEnter.
 * Purpose: Stores the writable NUMPADENTER DIK backing key name.
 */
char g_zInput_KeyNameNumpadEnter[12] = "NUMPADENTER";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameunlabeled
 * @recoil-artifact defines .data recoil:data:0x4e09c0: g_zInput_KeyNameUnlabeled.
 * Purpose: Stores the writable UNLABELED DIK backing key name.
 */
char g_zInput_KeyNameUnlabeled[10] = "UNLABELED";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameax
 * @recoil-artifact defines .data recoil:data:0x4e09cc: g_zInput_KeyNameAx.
 * Purpose: Stores the writable AX DIK backing key name.
 */
char g_zInput_KeyNameAx[3] = "AX";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamekanji
 * @recoil-artifact defines .data recoil:data:0x4e09d0: g_zInput_KeyNameKanji.
 * Purpose: Stores the writable KANJI DIK backing key name.
 */
char g_zInput_KeyNameKanji[6] = "KANJI";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameunderline
 * @recoil-artifact defines .data recoil:data:0x4e09d8: g_zInput_KeyNameUnderline.
 * Purpose: Stores the writable UNDERLINE DIK backing key name.
 */
char g_zInput_KeyNameUnderline[10] = "UNDERLINE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamecolon
 * @recoil-artifact defines .data recoil:data:0x4e09e4: g_zInput_KeyNameColon.
 * Purpose: Stores the writable COLON DIK backing key name.
 */
char g_zInput_KeyNameColon[6] = "COLON";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameat
 * @recoil-artifact defines .data recoil:data:0x4e09ec: g_zInput_KeyNameAt.
 * Purpose: Stores the writable AT DIK backing key name.
 */
char g_zInput_KeyNameAt[3] = "AT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamecircumflex
 * @recoil-artifact defines .data recoil:data:0x4e09f0: g_zInput_KeyNameCircumflex.
 * Purpose: Stores the writable CIRCUMFLEX DIK backing key name.
 */
char g_zInput_KeyNameCircumflex[11] = "CIRCUMFLEX";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpadequals
 * @recoil-artifact defines .data recoil:data:0x4e09fc: g_zInput_KeyNameNumpadEquals.
 * Purpose: Stores the writable NUMPADEQUALS DIK backing key name.
 */
char g_zInput_KeyNameNumpadEquals[13] = "NUMPADEQUALS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameyen
 * @recoil-artifact defines .data recoil:data:0x4e0a0c: g_zInput_KeyNameYen.
 * Purpose: Stores the writable YEN DIK backing key name.
 */
char g_zInput_KeyNameYen[4] = "YEN";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenoconvert
 * @recoil-artifact defines .data recoil:data:0x4e0a10: g_zInput_KeyNameNoConvert.
 * Purpose: Stores the writable NOCONVERT DIK backing key name.
 */
char g_zInput_KeyNameNoConvert[10] = "NOCONVERT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameconvert
 * @recoil-artifact defines .data recoil:data:0x4e0a1c: g_zInput_KeyNameConvert.
 * Purpose: Stores the writable CONVERT DIK backing key name.
 */
char g_zInput_KeyNameConvert[8] = "CONVERT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamekana
 * @recoil-artifact defines .data recoil:data:0x4e0a24: g_zInput_KeyNameKana.
 * Purpose: Stores the writable KANA DIK backing key name.
 */
char g_zInput_KeyNameKana[5] = "KANA";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef15
 * @recoil-artifact defines .data recoil:data:0x4e0a2c: g_zInput_KeyNameF15.
 * Purpose: Stores the writable F15 DIK backing key name.
 */
char g_zInput_KeyNameF15[4] = "F15";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef14
 * @recoil-artifact defines .data recoil:data:0x4e0a30: g_zInput_KeyNameF14.
 * Purpose: Stores the writable F14 DIK backing key name.
 */
char g_zInput_KeyNameF14[4] = "F14";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef13
 * @recoil-artifact defines .data recoil:data:0x4e0a34: g_zInput_KeyNameF13.
 * Purpose: Stores the writable F13 DIK backing key name.
 */
char g_zInput_KeyNameF13[4] = "F13";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef12
 * @recoil-artifact defines .data recoil:data:0x4e0a38: g_zInput_KeyNameF12.
 * Purpose: Stores the writable F12 DIK backing key name.
 */
char g_zInput_KeyNameF12[4] = "F12";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef11
 * @recoil-artifact defines .data recoil:data:0x4e0a3c: g_zInput_KeyNameF11.
 * Purpose: Stores the writable F11 DIK backing key name.
 */
char g_zInput_KeyNameF11[4] = "F11";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamedecimal
 * @recoil-artifact defines .data recoil:data:0x4e0a40: g_zInput_KeyNameDecimal.
 * Purpose: Stores the writable DECIMAL DIK backing key name.
 */
char g_zInput_KeyNameDecimal[8] = "DECIMAL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad0
 * @recoil-artifact defines .data recoil:data:0x4e0a48: g_zInput_KeyNameNumpad0.
 * Purpose: Stores the writable NUMPAD0 DIK backing key name.
 */
char g_zInput_KeyNameNumpad0[8] = "NUMPAD0";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad3
 * @recoil-artifact defines .data recoil:data:0x4e0a50: g_zInput_KeyNameNumpad3.
 * Purpose: Stores the writable NUMPAD3 DIK backing key name.
 */
char g_zInput_KeyNameNumpad3[8] = "NUMPAD3";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad2
 * @recoil-artifact defines .data recoil:data:0x4e0a58: g_zInput_KeyNameNumpad2.
 * Purpose: Stores the writable NUMPAD2 DIK backing key name.
 */
char g_zInput_KeyNameNumpad2[8] = "NUMPAD2";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad1
 * @recoil-artifact defines .data recoil:data:0x4e0a60: g_zInput_KeyNameNumpad1.
 * Purpose: Stores the writable NUMPAD1 DIK backing key name.
 */
char g_zInput_KeyNameNumpad1[8] = "NUMPAD1";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameadd
 * @recoil-artifact defines .data recoil:data:0x4e0a68: g_zInput_KeyNameAdd.
 * Purpose: Stores the writable ADD DIK backing key name.
 */
char g_zInput_KeyNameAdd[4] = "ADD";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad6
 * @recoil-artifact defines .data recoil:data:0x4e0a6c: g_zInput_KeyNameNumpad6.
 * Purpose: Stores the writable NUMPAD6 DIK backing key name.
 */
char g_zInput_KeyNameNumpad6[8] = "NUMPAD6";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad5
 * @recoil-artifact defines .data recoil:data:0x4e0a74: g_zInput_KeyNameNumpad5.
 * Purpose: Stores the writable NUMPAD5 DIK backing key name.
 */
char g_zInput_KeyNameNumpad5[8] = "NUMPAD5";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad4
 * @recoil-artifact defines .data recoil:data:0x4e0a7c: g_zInput_KeyNameNumpad4.
 * Purpose: Stores the writable NUMPAD4 DIK backing key name.
 */
char g_zInput_KeyNameNumpad4[8] = "NUMPAD4";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamesubtract
 * @recoil-artifact defines .data recoil:data:0x4e0a84: g_zInput_KeyNameSubtract.
 * Purpose: Stores the writable SUBTRACT DIK backing key name.
 */
char g_zInput_KeyNameSubtract[9] = "SUBTRACT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad9
 * @recoil-artifact defines .data recoil:data:0x4e0a90: g_zInput_KeyNameNumpad9.
 * Purpose: Stores the writable NUMPAD9 DIK backing key name.
 */
char g_zInput_KeyNameNumpad9[8] = "NUMPAD9";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad8
 * @recoil-artifact defines .data recoil:data:0x4e0a98: g_zInput_KeyNameNumpad8.
 * Purpose: Stores the writable NUMPAD8 DIK backing key name.
 */
char g_zInput_KeyNameNumpad8[8] = "NUMPAD8";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumpad7
 * @recoil-artifact defines .data recoil:data:0x4e0aa0: g_zInput_KeyNameNumpad7.
 * Purpose: Stores the writable NUMPAD7 DIK backing key name.
 */
char g_zInput_KeyNameNumpad7[8] = "NUMPAD7";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamescroll
 * @recoil-artifact defines .data recoil:data:0x4e0aa8: g_zInput_KeyNameScroll.
 * Purpose: Stores the writable SCROLL DIK backing key name.
 */
char g_zInput_KeyNameScroll[7] = "SCROLL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamenumlock
 * @recoil-artifact defines .data recoil:data:0x4e0ab0: g_zInput_KeyNameNumLock.
 * Purpose: Stores the writable NUMLOCK DIK backing key name.
 */
char g_zInput_KeyNameNumLock[8] = "NUMLOCK";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef10
 * @recoil-artifact defines .data recoil:data:0x4e0ab8: g_zInput_KeyNameF10.
 * Purpose: Stores the writable F10 DIK backing key name.
 */
char g_zInput_KeyNameF10[4] = "F10";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef9
 * @recoil-artifact defines .data recoil:data:0x4e0abc: g_zInput_KeyNameF9.
 * Purpose: Stores the writable F9 DIK backing key name.
 */
char g_zInput_KeyNameF9[3] = "F9";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef8
 * @recoil-artifact defines .data recoil:data:0x4e0ac0: g_zInput_KeyNameF8.
 * Purpose: Stores the writable F8 DIK backing key name.
 */
char g_zInput_KeyNameF8[3] = "F8";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef7
 * @recoil-artifact defines .data recoil:data:0x4e0ac4: g_zInput_KeyNameF7.
 * Purpose: Stores the writable F7 DIK backing key name.
 */
char g_zInput_KeyNameF7[3] = "F7";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef6
 * @recoil-artifact defines .data recoil:data:0x4e0ac8: g_zInput_KeyNameF6.
 * Purpose: Stores the writable F6 DIK backing key name.
 */
char g_zInput_KeyNameF6[3] = "F6";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef5
 * @recoil-artifact defines .data recoil:data:0x4e0acc: g_zInput_KeyNameF5.
 * Purpose: Stores the writable F5 DIK backing key name.
 */
char g_zInput_KeyNameF5[3] = "F5";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef4
 * @recoil-artifact defines .data recoil:data:0x4e0ad0: g_zInput_KeyNameF4.
 * Purpose: Stores the writable F4 DIK backing key name.
 */
char g_zInput_KeyNameF4[3] = "F4";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef3
 * @recoil-artifact defines .data recoil:data:0x4e0ad4: g_zInput_KeyNameF3.
 * Purpose: Stores the writable F3 DIK backing key name.
 */
char g_zInput_KeyNameF3[3] = "F3";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef2
 * @recoil-artifact defines .data recoil:data:0x4e0ad8: g_zInput_KeyNameF2.
 * Purpose: Stores the writable F2 DIK backing key name.
 */
char g_zInput_KeyNameF2[3] = "F2";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamef1
 * @recoil-artifact defines .data recoil:data:0x4e0adc: g_zInput_KeyNameF1.
 * Purpose: Stores the writable F1 DIK backing key name.
 */
char g_zInput_KeyNameF1[3] = "F1";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamecapital
 * @recoil-artifact defines .data recoil:data:0x4e0ae0: g_zInput_KeyNameCapital.
 * Purpose: Stores the writable CAPITAL DIK backing key name.
 */
char g_zInput_KeyNameCapital[8] = "CAPITAL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamespace
 * @recoil-artifact defines .data recoil:data:0x4e0ae8: g_zInput_KeyNameSpace.
 * Purpose: Stores the writable SPACE DIK backing key name.
 */
char g_zInput_KeyNameSpace[6] = "SPACE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamelmenu
 * @recoil-artifact defines .data recoil:data:0x4e0af0: g_zInput_KeyNameLMenu.
 * Purpose: Stores the writable LMENU DIK backing key name.
 */
char g_zInput_KeyNameLMenu[6] = "LMENU";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamemultiply
 * @recoil-artifact defines .data recoil:data:0x4e0af8: g_zInput_KeyNameMultiply.
 * Purpose: Stores the writable MULTIPLY DIK backing key name.
 */
char g_zInput_KeyNameMultiply[9] = "MULTIPLY";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamershift
 * @recoil-artifact defines .data recoil:data:0x4e0b04: g_zInput_KeyNameRShift.
 * Purpose: Stores the writable RSHIFT DIK backing key name.
 */
char g_zInput_KeyNameRShift[7] = "RSHIFT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameslash
 * @recoil-artifact defines .data recoil:data:0x4e0b0c: g_zInput_KeyNameSlash.
 * Purpose: Stores the writable SLASH DIK backing key name.
 */
char g_zInput_KeyNameSlash[6] = "SLASH";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameperiod
 * @recoil-artifact defines .data recoil:data:0x4e0b14: g_zInput_KeyNamePeriod.
 * Purpose: Stores the writable PERIOD DIK backing key name.
 */
char g_zInput_KeyNamePeriod[7] = "PERIOD";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamecomma
 * @recoil-artifact defines .data recoil:data:0x4e0b1c: g_zInput_KeyNameComma.
 * Purpose: Stores the writable COMMA DIK backing key name.
 */
char g_zInput_KeyNameComma[6] = "COMMA";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keycharrow-mnbvcxz
 * @recoil-artifact defines .data recoil:data:0x4e0b24: g_zInput_KeyCharRow_MNBVCXZ.
 * BN types this as seven 4-byte-aligned writable one-character DIK name slots.
 * Purpose: Stores folded one-character Z/X/C/V/B/N/M key names.
 */
unsigned int g_zInput_KeyCharRow_MNBVCXZ[7] = {
    'M',
    'N',
    'B',
    'V',
    'C',
    'X',
    'Z'
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamebackslash
 * @recoil-artifact defines .data recoil:data:0x4e0b40: g_zInput_KeyNameBackslash.
 * Purpose: Stores the writable BACKSLASH DIK backing key name.
 */
char g_zInput_KeyNameBackslash[10] = "BACKSLASH";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamelshift
 * @recoil-artifact defines .data recoil:data:0x4e0b4c: g_zInput_KeyNameLShift.
 * Purpose: Stores the writable LSHIFT DIK backing key name.
 */
char g_zInput_KeyNameLShift[7] = "LSHIFT";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamegrave
 * @recoil-artifact defines .data recoil:data:0x4e0b54: g_zInput_KeyNameGrave.
 * Purpose: Stores the writable GRAVE DIK backing key name.
 */
char g_zInput_KeyNameGrave[6] = "GRAVE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameapostrophe
 * @recoil-artifact defines .data recoil:data:0x4e0b5c: g_zInput_KeyNameApostrophe.
 * Purpose: Stores the writable APOSTROPHE DIK backing key name.
 */
char g_zInput_KeyNameApostrophe[11] = "APOSTROPHE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamesemicolon
 * @recoil-artifact defines .data recoil:data:0x4e0b68: g_zInput_KeyNameSemicolon.
 * Purpose: Stores the writable SEMICOLON DIK backing key name.
 */
char g_zInput_KeyNameSemicolon[10] = "SEMICOLON";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keycharrow-lkjhgfdsa
 * @recoil-artifact defines .data recoil:data:0x4e0b74: g_zInput_KeyCharRow_LKJHGFDSA.
 * BN types this as nine 4-byte-aligned writable one-character DIK name slots.
 * Purpose: Stores folded one-character A/S/D/F/G/H/J/K/L key names.
 */
unsigned int g_zInput_KeyCharRow_LKJHGFDSA[9] = {
    'L',
    'K',
    'J',
    'H',
    'G',
    'F',
    'D',
    'S',
    'A'
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamelcontrol
 * @recoil-artifact defines .data recoil:data:0x4e0b98: g_zInput_KeyNameLControl.
 * Purpose: Stores the writable LCONTROL DIK backing key name.
 */
char g_zInput_KeyNameLControl[9] = "LCONTROL";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamereturn
 * @recoil-artifact defines .data recoil:data:0x4e0ba4: g_zInput_KeyNameReturn.
 * Purpose: Stores the writable RETURN DIK backing key name.
 */
char g_zInput_KeyNameReturn[7] = "RETURN";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamerbracket
 * @recoil-artifact defines .data recoil:data:0x4e0bac: g_zInput_KeyNameRBracket.
 * Purpose: Stores the writable RBRACKET DIK backing key name.
 */
char g_zInput_KeyNameRBracket[9] = "RBRACKET";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynamelbracket
 * @recoil-artifact defines .data recoil:data:0x4e0bb8: g_zInput_KeyNameLBracket.
 * Purpose: Stores the writable LBRACKET DIK backing key name.
 */
char g_zInput_KeyNameLBracket[9] = "LBRACKET";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keycharrow-poiuytrewq
 * @recoil-artifact defines .data recoil:data:0x4e0bc4: g_zInput_KeyCharRow_POIUYTREWQ.
 * BN types this as ten 4-byte-aligned writable one-character DIK name slots.
 * Purpose: Stores folded one-character Q/W/E/R/T/Y/U/I/O/P key names.
 */
unsigned int g_zInput_KeyCharRow_POIUYTREWQ[10] = {
    'P',
    'O',
    'I',
    'U',
    'Y',
    'T',
    'R',
    'E',
    'W',
    'Q'
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynametab
 * @recoil-artifact defines .data recoil:data:0x4e0bec: g_zInput_KeyNameTab.
 * Purpose: Stores the writable TAB DIK backing key name.
 */
char g_zInput_KeyNameTab[4] = "TAB";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameequals
 * @recoil-artifact defines .data recoil:data:0x4e0bf0: g_zInput_KeyNameEquals.
 * Purpose: Stores the writable EQUALS DIK backing key name.
 */
char g_zInput_KeyNameEquals[7] = "EQUALS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameminus
 * @recoil-artifact defines .data recoil:data:0x4e0bf8: g_zInput_KeyNameMinus.
 * Purpose: Stores the writable MINUS DIK backing key name.
 */
char g_zInput_KeyNameMinus[6] = "MINUS";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keycharrow-0987653
 * @recoil-artifact defines .data recoil:data:0x4e0c00: g_zInput_KeyCharRow_0987653.
 * BN types this as seven 4-byte-aligned writable one-character DIK name slots.
 * Purpose: Stores folded one-character 3/5/6/7/8/9/0 key names.
 */
unsigned int g_zInput_KeyCharRow_0987653[7] = {
    '0',
    '9',
    '8',
    '7',
    '6',
    '5',
    '3'
};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-keynameescape
 * @recoil-artifact defines .data recoil:data:0x4e0c1c: g_zInput_KeyNameEscape.
 * Purpose: Stores the writable ESCAPE DIK backing key name.
 */
char g_zInput_KeyNameEscape[7] = "ESCAPE";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonname8
 * @recoil-artifact defines .data recoil:data:0x4e0c24: g_zInput_JoystickButtonName8.
 * Purpose: Stores the writable joystick Button 8 bind-map name.
 */
char g_zInput_JoystickButtonName8[9] = "Button 8";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonname7
 * @recoil-artifact defines .data recoil:data:0x4e0c30: g_zInput_JoystickButtonName7.
 * Purpose: Stores the writable joystick Button 7 bind-map name.
 */
char g_zInput_JoystickButtonName7[9] = "Button 7";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonname6
 * @recoil-artifact defines .data recoil:data:0x4e0c3c: g_zInput_JoystickButtonName6.
 * Purpose: Stores the writable joystick Button 6 bind-map name.
 */
char g_zInput_JoystickButtonName6[9] = "Button 6";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonname5
 * @recoil-artifact defines .data recoil:data:0x4e0c48: g_zInput_JoystickButtonName5.
 * Purpose: Stores the writable joystick Button 5 bind-map name.
 */
char g_zInput_JoystickButtonName5[9] = "Button 5";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonname4
 * @recoil-artifact defines .data recoil:data:0x4e0c54: g_zInput_JoystickButtonName4.
 * Purpose: Stores the writable joystick Button 4 bind-map name.
 */
char g_zInput_JoystickButtonName4[9] = "Button 4";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonname3
 * @recoil-artifact defines .data recoil:data:0x4e0c60: g_zInput_JoystickButtonName3.
 * Purpose: Stores the writable joystick Button 3 bind-map name.
 */
char g_zInput_JoystickButtonName3[9] = "Button 3";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonname2
 * @recoil-artifact defines .data recoil:data:0x4e0c6c: g_zInput_JoystickButtonName2.
 * Purpose: Stores the writable joystick Button 2 bind-map name.
 */
char g_zInput_JoystickButtonName2[9] = "Button 2";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonname1
 * @recoil-artifact defines .data recoil:data:0x4e0c78: g_zInput_JoystickButtonName1.
 * Purpose: Stores the writable joystick Button 1 bind-map name.
 */
char g_zInput_JoystickButtonName1[9] = "Button 1";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousebuttonnamemiddle
 * @recoil-artifact defines .data recoil:data:0x4e0c84: g_zInput_MouseButtonNameMiddle.
 * Purpose: Stores the writable mouse Middle bind-map name.
 */
char g_zInput_MouseButtonNameMiddle[7] = "Middle";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousebuttonnameright
 * @recoil-artifact defines .data recoil:data:0x4e0c8c: g_zInput_MouseButtonNameRight.
 * Purpose: Stores the writable mouse Right bind-map name.
 */
char g_zInput_MouseButtonNameRight[6] = "Right";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousebuttonnameleft
 * @recoil-artifact defines .data recoil:data:0x4e0c94: g_zInput_MouseButtonNameLeft.
 * Purpose: Stores the writable mouse Left bind-map name.
 */
char g_zInput_MouseButtonNameLeft[5] = "Left";
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mouseclientwidth
 * @recoil-artifact defines .data recoil:data:0x561c78: g_zInput_MouseClientWidth.
 * BN types this as the zero-filled client-width word used by mouse
 * initialization, recentering, and clamped cursor math.
 * Purpose: Caches the input window client width used for mouse scaling.
 */
int g_zInput_MouseClientWidth = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mouseclientheight
 * @recoil-artifact defines .data recoil:data:0x561c7c: g_zInput_MouseClientHeight.
 * BN types this as the zero-filled client-height word used by mouse
 * initialization, recentering, and clamped cursor math.
 * Purpose: Caches the input window client height used for mouse scaling.
 */
int g_zInput_MouseClientHeight = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mouseclientcenterx
 * @recoil-artifact defines .data recoil:data:0x561c70: g_zInput_MouseClientCenterX.
 * BN types this as the zero-filled client center-x word used by normalized
 * mouse cursor math.
 * Purpose: Caches the horizontal midpoint of the input client rectangle.
 */
int g_zInput_MouseClientCenterX = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mouseclientcentery
 * @recoil-artifact defines .data recoil:data:0x561c50: g_zInput_MouseClientCenterY.
 * BN types this as the zero-filled client center-y word used by normalized
 * mouse cursor math.
 * Purpose: Caches the vertical midpoint of the input client rectangle.
 */
int g_zInput_MouseClientCenterY = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mouseinvclientcenterx
 * @recoil-artifact defines .data recoil:data:0x561c68: g_zInput_MouseInvClientCenterX.
 * BN types this zero-filled float as the horizontal normalized-coordinate
 * divisor refreshed whenever the mouse client rectangle changes.
 * Purpose: Converts client-space horizontal mouse deltas and positions.
 */
float g_zInput_MouseInvClientCenterX = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mouseinvclientcentery
 * @recoil-artifact defines .data recoil:data:0x561c6c: g_zInput_MouseInvClientCenterY.
 * BN types this zero-filled float as the vertical normalized-coordinate
 * divisor refreshed whenever the mouse client rectangle changes.
 * Purpose: Converts client-space vertical mouse deltas and positions.
 */
float g_zInput_MouseInvClientCenterY = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousewrapmodeflag
 * @recoil-artifact defines .data recoil:data:0x561cac: g_zInput_MouseWrapModeFlag.
 * Purpose: stores whether mouse motion wraps around the client area instead
 * of clamping to its bounds.
 */
int g_zInput_MouseWrapModeFlag = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-kbddiktoasciitable
 * @recoil-artifact defines .data recoil:data:0x561848: g_zInput_KbdDikToAsciiTable.
 * BN types this as a zero-filled 256-entry int table, cleared and populated by
 * Keyboard_InitDikToAsciiTable and read by Keyboard_TranslateDikToAscii.
 * zin_kbd.cpp keyboard source-state globals; BN 0x46f690/0x46f450 uses the typed DirectInput
 * event buffer, modifier bitfield, dispatch slots, and raw callback storage as one keyboard
 * polling owner.
 * Purpose: Stores base ASCII/control codes for DirectInput keyboard scan codes.
 */
int g_zInput_KbdDikToAsciiTable[0x100] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-kbddiktoasciitableready
 * @recoil-artifact defines .data recoil:data:0x561c48: g_zInput_KbdDikToAsciiTableReady.
 * BN types this as the zero-filled readiness flag guarding one-time DIK to
 * ASCII table initialization.
 * Purpose: Records whether the keyboard DIK translation table has been built.
 */
int g_zInput_KbdDikToAsciiTableReady = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinputffeffectset
 * @recoil-artifact defines .data recoil:data:0x4f36b4: g_zInputFfEffectSet.
 * BN types this as the zero-initialized zInput_FFEffectSet pointer shared by
 * the DirectInput force-feedback creation and playback helpers.
 * Purpose: Owns the active force-feedback effect set pointer.
 */
zInput_FFEffectSet *g_zInputFfEffectSet = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-gamestateormaptable
 * @recoil-artifact defines .data recoil:data:0x4f3a88: g_GameStateOrMapTable.
 * Purpose: Stores g GameStateOrMapTable data used by engine.zinput.game_state_or_map_table_data.
 */
zInput_GameStateOrMapTablePartial *g_GameStateOrMapTable = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-dipitchanglelowpassrad
 * @recoil-artifact defines .data recoil:data:0x4f3ee8: g_zInput_DiPitchAngleLowpassRad.
 * BN types this as the zero-filled DirectInput force-feedback pitch low-pass
 * accumulator updated by steering/pitch effect playback.
 * Purpose: Smooths pitch angle changes for force-feedback effects.
 */
float g_zInput_DiPitchAngleLowpassRad = 0.0f;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-dikkeynames
 * @recoil-artifact defines .data recoil:data:0x565ebc: g_zInput_DikKeyNames.
 * Binary Ninja types this owner field as a zero-filled char*[0x100] table;
 * zInput::BindMap_InitDikKeyNameTable fills the DirectInput DIK slots.
 * Purpose: Stores key-name pointers used by bind-map display formatting.
 */
const char *g_zInput_DikKeyNames[0x100] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-joystickbuttonnames
 * @recoil-artifact defines .data recoil:data:0x5662bc: g_zInput_JoystickButtonNames.
 * Binary Ninja types this owner field as a zero-filled char*[0x9] table;
 * zInput::BindMap_InitJoystickButtonNameTable fills one-based slots 1..8.
 * Purpose: Stores joystick button-name pointers used by bind-map display formatting.
 */
const char *g_zInput_JoystickButtonNames[9] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-mousebuttonnames
 * @recoil-artifact defines .data recoil:data:0x5662fc: g_zInput_MouseButtonNames.
 * Binary Ninja types this owner field as a zero-filled char*[0x4] table;
 * zInput::BindMap_InitMouseButtonNameTable fills one-based slots 1..3.
 * Purpose: Stores mouse button-name pointers used by bind-map display formatting.
 */
const char *g_zInput_MouseButtonNames[4] = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-bindgroupinfolist
 * @recoil-artifact defines .data recoil:data:0x4f3ae0: g_zInput_BindGroupInfoList.
 * Binary Ninja types the 16-byte object as the bind-group pointer vector;
 * 0x429f20/0x429f50 are the compiler-emitted static constructor/destructor for
 * this storage, with the offset-0 allocator byte exposed as an MSVC artifact.
 * Purpose: Owns the global bind-group pointer vector storage.
 */
zInput_BindGroupInfoList g_zInput_BindGroupInfoList = {0};
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-currentbindgroupindex
 * @recoil-artifact defines .data recoil:data:0x4f3ad8: g_zInput_CurrentBindGroupIndex.
 * Purpose: Stores g zInput CurrentBindGroupIndex data used by engine.zinput.bindgroup_default_globals.
 */
int g_zInput_CurrentBindGroupIndex = 0;
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.g-zinput-commandlocidtable
 * @recoil-artifact defines .data recoil:data:0x4f3af0: g_zInput_CommandLocIdTable.
 * Purpose: Stores g zInput CommandLocIdTable data used by engine.zinput.bindgroup_default_globals.
 */
int g_zInput_CommandLocIdTable[0x30] = {0};
} // extern "C"

namespace zInput {
const int kZInputCommandLabelBytes = 0x50;
} // namespace zInput
/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-initfromtemplate
 * @recoil-artifact defines .text recoil:function:0x4706c0: zInput_BindMapContext::InitFromTemplate.
 * Purpose: deep-copy an optional bind-map template and rebuild reverse lookup tables.
 */
zInput_BindMapContext * zInput_BindMapContext::InitFromTemplate(
    const zInput_BindMapContext *tmpl
) {
    m_isOverlay = 0;
    if (tmpl != 0) {
        m_commandCount = tmpl->m_commandCount;
        m_packedBindings = (int *)(calloc(
            tmpl->m_commandCount,
            sizeof(int)
        ));
        memcpy(
            m_packedBindings,
            tmpl->m_packedBindings,
            (size_t)(tmpl->m_commandCount) * sizeof(int)
        );

        m_commandCallbacks =
            (zInputCommandCallbackFn *)(calloc(
                m_commandCount,
                sizeof(zInputCommandCallbackFn)
            ));
        memcpy(
            m_commandCallbacks,
            tmpl->m_commandCallbacks,
            (size_t)(m_commandCount) * sizeof(zInputCommandCallbackFn)
        );

        m_commandLabels = (char **)(calloc(
            m_commandCount,
            sizeof(char *)
        ));
        for (int i = 0; i < m_commandCount; ++i) {
            m_commandLabels[i] = (char *)(calloc(
                1,
                zInput::kZInputCommandLabelBytes
            ));
            strncpy(
                m_commandLabels[i],
                tmpl->m_commandLabels[i],
                zInput::kZInputCommandLabelBytes
            );
        }

        RebuildLookupIndices();
    }
    return this;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-freeallbuffers
 * @recoil-artifact defines .text recoil:function:0x4707a0: zInput_BindMapContext::FreeAllBuffers.
 * Purpose: release owned binding, callback, and label buffers from a bind-map context.
 */
void zInput_BindMapContext::FreeAllBuffers() {
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
        m_commandLabels = 0;
    }

    if (m_packedBindings != 0) {
        free(m_packedBindings);
        m_packedBindings = 0;
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-rebuildlookupindices
 * @recoil-artifact defines .text recoil:function:0x470820: zInput_BindMapContext::RebuildLookupIndices.
 * Purpose: rebuild keyboard, joystick, and mouse reverse lookup tables from packed bindings.
 */
void zInput_BindMapContext::RebuildLookupIndices() {
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
            m_primaryKeyToCommand[m_packedBindings[commandId] & 0x7ff] = commandId;
            m_secondaryKeyToCommand[
                ((unsigned int)(m_packedBindings[commandId]) >> 0x0b) & 0x7ff
            ] = commandId;
            m_joystickToCommand[
                ((unsigned int)(m_packedBindings[commandId]) >> 0x16) & 0x0f
            ] = commandId;
            m_mouseToCommand[
                ((unsigned int)(m_packedBindings[commandId]) >> 0x1a) & 0x03
            ] = commandId;

            zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
            if (callback != 0) {
                SetCommandCallback(
                    commandId,
                    callback
                );
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-initcommandmap
 * @recoil-artifact defines .text recoil:function:0x4708f0: zInput_BindMapContext::InitCommandMap.
 * Purpose: Allocate command binding, lookup, callback, and label storage for a bind-map context.
 */
void zInput_BindMapContext::InitCommandMap(
    int commandCount
) {
    m_commandCount = commandCount;
    zOptionEntryPartial *option =
        zGame::Options_GetOrCreateOption(
            g_zInput_CommandMapKeyName,
            7,
            commandCount * (int)(sizeof(int)),
            1
        );
    m_packedBindings = (int *)(option->payloadOrBuffer);
    m_commandCallbacks =
        (zInputCommandCallbackFn *)(calloc(
            commandCount,
            sizeof(zInputCommandCallbackFn)
        ));
    m_commandLabels = (char **)(calloc(
        commandCount,
        sizeof(char *)
    ));
    for (int i = 0; i < commandCount; ++i) {
        m_commandLabels[i] = (char *)(calloc(
            1,
            zInput::kZInputCommandLabelBytes
        ));
    }

    ResetAllBindings();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-freenonownedbuffers
 * @recoil-artifact defines .text recoil:function:0x470960: zInput_BindMapContext::FreeNonOwnedBuffers.
 * Purpose: Release lookup and callback buffers that are not owned by a copied bind-map template.
 */
void zInput_BindMapContext::FreeNonOwnedBuffers() {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-resetallbindings
 * @recoil-artifact defines .text recoil:function:0x4709d0: zInput_BindMapContext::ResetAllBindings.
 * Purpose: clear every command binding/callback and rebuild reverse lookup indices.
 */
void zInput_BindMapContext::ResetAllBindings() {
    for (int i = 0; i < m_commandCount; ++i) {
        m_packedBindings[i] = zInput::BindMap_PackBindingCode(
            0,
            0,
            0,
            0
        );
        m_commandCallbacks[i] = 0;
    }

    RebuildLookupIndices();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmap-packbindingcode
 * @recoil-artifact defines .text recoil:function:0x470a10: zInput::BindMap_PackBindingCode.
 * Purpose: pack keyboard, joystick, and mouse binding slots into the bind-map record format.
 */
int __fastcall zInput::BindMap_PackBindingCode(
    int primary,
    int secondary,
    int joy,
    int mouse
) {
    return (((mouse & 3) << 4 | (joy & 0x0f)) << 0x0b | (secondary & 0x7ff)) << 0x0b |
           (primary & 0x7ff);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getprimarykeyboardkey
 * @recoil-artifact defines .text recoil:function:0x470a40: zInput_BindMapContext::GetPrimaryKeyboardKey.
 * Purpose: return the primary DIK key packed for a command binding.
 */
int zInput_BindMapContext::GetPrimaryKeyboardKey(
    int commandIndex
) {
    return m_packedBindings[commandIndex] & 0x7ff;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getsecondarykeyboardkey
 * @recoil-artifact defines .text recoil:function:0x470a60: zInput_BindMapContext::GetSecondaryKeyboardKey.
 * Purpose: return the secondary DIK key packed for a command binding.
 */
int zInput_BindMapContext::GetSecondaryKeyboardKey(
    int commandIndex
) {
    return ((unsigned int)(m_packedBindings[commandIndex]) >> 0x0b) & 0x7ff;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getjoystickbuttonslot
 * @recoil-artifact defines .text recoil:function:0x470a80: zInput_BindMapContext::GetJoystickButtonSlot.
 * Purpose: return the joystick button slot packed for a command binding.
 */
int zInput_BindMapContext::GetJoystickButtonSlot(
    int commandIndex
) {
    return ((unsigned int)(m_packedBindings[commandIndex]) >> 0x16) & 0x0f;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getmousebuttonslot
 * @recoil-artifact defines .text recoil:function:0x470aa0: zInput_BindMapContext::GetMouseButtonSlot.
 * Purpose: return the mouse button slot packed for a command binding.
 */
int zInput_BindMapContext::GetMouseButtonSlot(
    int commandIndex
) {
    return ((unsigned int)(m_packedBindings[commandIndex]) >> 0x1a) & 0x03;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getcommandbyprimarykey
 * @recoil-artifact defines .text recoil:function:0x470ac0: zInput_BindMapContext::GetCommandByPrimaryKey.
 * Purpose: Resolve a primary keyboard key to its command id.
 */
int zInput_BindMapContext::GetCommandByPrimaryKey(
    int keyboardKey
) {
    return m_primaryKeyToCommand[keyboardKey];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getcommandbysecondarykey
 * @recoil-artifact defines .text recoil:function:0x470ad0: zInput_BindMapContext::GetCommandBySecondaryKey.
 * Purpose: Resolve a secondary keyboard key to its command id.
 */
int zInput_BindMapContext::GetCommandBySecondaryKey(
    int keyboardKey
) {
    return m_secondaryKeyToCommand[keyboardKey];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getcommandbyanykeyboardkey
 * @recoil-artifact defines .text recoil:function:0x470ae0: zInput_BindMapContext::GetCommandByAnyKeyboardKey.
 * Purpose: Resolve either keyboard binding slot to its command id.
 */
int zInput_BindMapContext::GetCommandByAnyKeyboardKey(
    int keyboardKey
) {
    const int primary = GetCommandByPrimaryKey(keyboardKey);
    if (primary != 0) {
        return primary;
    }

    return GetCommandBySecondaryKey(keyboardKey);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getcommandbyjoystickslot
 * @recoil-artifact defines .text recoil:function:0x470b00: zInput_BindMapContext::GetCommandByJoystickSlot.
 * Purpose: Resolve a joystick button slot to its command id.
 */
int zInput_BindMapContext::GetCommandByJoystickSlot(
    int joystickSlot
) {
    return m_joystickToCommand[joystickSlot];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-getcommandbymouseslot
 * @recoil-artifact defines .text recoil:function:0x470b10: zInput_BindMapContext::GetCommandByMouseSlot.
 * Purpose: Return the command id stored for one bind-map mouse slot.
 */
int zInput_BindMapContext::GetCommandByMouseSlot(
    int mouseSlot
) {
    return m_mouseToCommand[mouseSlot];
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-setprimarykeybinding
 * @recoil-artifact defines .text recoil:function:0x470b20: zInput_BindMapContext::SetPrimaryKeyBinding.
 * Purpose: Update a command's primary keyboard binding and reverse lookup slot.
 */
void zInput_BindMapContext::SetPrimaryKeyBinding(
    int keyCode,
    int commandId
) {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-setsecondarykeybinding
 * @recoil-artifact defines .text recoil:function:0x470b80: zInput_BindMapContext::SetSecondaryKeyBinding.
 * Purpose: Update a command's secondary keyboard binding and reverse lookup slot.
 */
void zInput_BindMapContext::SetSecondaryKeyBinding(
    int keyCode,
    int commandId
) {
    if (keyCode != 0) {
        m_packedBindings[m_secondaryKeyToCommand[keyCode]] &= 0xffc007ff;
        m_secondaryKeyToCommand[keyCode] = commandId;
    }
    if (commandId == 0) {
        return;
    }

    m_secondaryKeyToCommand[((unsigned int)(m_packedBindings[commandId]) >> 0x0b) & 0x7ff] = 0;
    m_packedBindings[commandId] =
        (m_packedBindings[commandId] & 0xffc007ff) | ((keyCode & 0x7ff) << 0x0b);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-setjoystickbinding
 * @recoil-artifact defines .text recoil:function:0x470bf0: zInput_BindMapContext::SetJoystickBinding.
 * Purpose: Update a command's joystick button binding and reverse lookup slot.
 */
void zInput_BindMapContext::SetJoystickBinding(
    int joystickSlot,
    int commandId
) {
    if (joystickSlot != 0) {
        m_packedBindings[m_joystickToCommand[joystickSlot]] &= 0xfc3fffff;
        m_joystickToCommand[joystickSlot] = commandId;
    }
    if (commandId == 0) {
        return;
    }

    m_joystickToCommand[((unsigned int)(m_packedBindings[commandId]) >> 0x16) & 0x0f] = 0;
    m_packedBindings[commandId] =
        ((joystickSlot & 0x0f) << 0x16) | (m_packedBindings[commandId] & 0xfc3fffff);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-setmousebinding
 * @recoil-artifact defines .text recoil:function:0x470c60: zInput_BindMapContext::SetMouseBinding.
 * Purpose: Update a command's mouse button binding and reverse lookup slot.
 */
void zInput_BindMapContext::SetMouseBinding(
    int mouseSlot,
    int commandId
) {
    if (mouseSlot != 0) {
        m_packedBindings[m_mouseToCommand[mouseSlot]] &= 0xf3ffffff;
        m_mouseToCommand[mouseSlot] = commandId;
    }
    if (commandId == 0) {
        return;
    }

    m_mouseToCommand[((unsigned int)(m_packedBindings[commandId]) >> 0x1a) & 0x03] = 0;
    m_packedBindings[commandId] =
        ((mouseSlot & 0x03) << 0x1a) | (m_packedBindings[commandId] & 0xf3ffffff);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-setbindingrecord
 * @recoil-artifact defines .text recoil:function:0x470cd0: zInput_BindMapContext::SetBindingRecord.
 * Purpose: Store one command label and all keyboard, joystick, and mouse binding slots.
 */
void zInput_BindMapContext::SetBindingRecord(
    int commandId,
    const char *labelSrc,
    int primaryKey,
    int secondaryKey,
    int joystickSlot,
    int mouseSlot
) {
    if (labelSrc != 0 && *labelSrc != '\0') {
        strncpy(
            m_commandLabels[commandId],
            labelSrc,
            0x4f
        );
    }

    SetPrimaryKeyBinding(
        primaryKey,
        commandId
    );
    SetSecondaryKeyBinding(
        secondaryKey,
        commandId
    );
    SetJoystickBinding(
        joystickSlot,
        commandId
    );
    SetMouseBinding(
        mouseSlot,
        commandId
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-dispatchmousebuttoncallbacks
 * @recoil-artifact defines .text recoil:function:0x470d40: zInput_BindMapContext::DispatchMouseButtonCallbacks.
 * Purpose: Dispatches pressed mouse-button transitions through bind-map command callbacks.
 */
void zInput_BindMapContext::DispatchMouseButtonCallbacks() {
    zInput::MouseStateSnapshot *const state = zInput::Mouse_GetStateSnapshotPtr();
    if (state->button1Transition == 1) {
        const int commandId = GetCommandByMouseSlot(1);
        zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
        if (callback != 0) {
            callback(commandId);
        }
    }
    if (state->button2Transition == 1) {
        const int commandId = GetCommandByMouseSlot(2);
        zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
        if (callback != 0) {
            callback(commandId);
        }
    }
    if (state->button3Transition == 1) {
        const int commandId = GetCommandByMouseSlot(3);
        zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
        if (callback != 0) {
            callback(commandId);
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-dispatchjoystickbuttoncallbacks
 * @recoil-artifact defines .text recoil:function:0x470db0: zInput_BindMapContext::DispatchJoystickButtonCallbacks.
 * Purpose: Dispatches pressed joystick-button transitions through bind-map command callbacks.
 */
void zInput_BindMapContext::DispatchJoystickButtonCallbacks() {
    {
        for (unsigned int slot = 1; slot < 0x0b; ++slot) {
            if (zInput::DI_GetButtonTransitionState(slot) == 1) {
                const int commandId = zInput::BindMapCurrent_GetCommandByJoystickSlot(slot);
                zInputCommandCallbackFn callback = m_commandCallbacks[commandId];
                if (callback != 0) {
                    callback(commandId);
                }
            }
        }
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-setcommandcallback
 * @recoil-artifact defines .text recoil:function:0x470df0: zInput_BindMapContext::SetCommandCallback.
 * Purpose: store a command callback and register keyboard bridge callbacks for its keys.
 */
int zInput_BindMapContext::SetCommandCallback(
    int commandId,
    zInputCommandCallbackFn callback
) {
    const int primary = GetPrimaryKeyboardKey(commandId);
    const int secondary = GetSecondaryKeyboardKey(commandId);
    if (primary == 0 && secondary == 0) {
        return 0;
    }

    m_commandCallbacks[commandId] = callback;
    if (primary != 0) {
        zInput::Keyboard_RegisterKeyCallback(
            primary,
            (void *)(&zInput_BindMapContext_DispatchFromKeyboardEvent),
            m_commandLabels[commandId]
        );
    }
    if (secondary != 0) {
        zInput::Keyboard_RegisterKeyCallback(
            secondary,
            (void *)(&zInput_BindMapContext_DispatchFromKeyboardEvent),
            m_commandLabels[commandId]
        );
    }

    return 1;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-dispatchfromkeyboardevent
 * @recoil-artifact defines .text recoil:function:0x470e80: zInput_BindMapContext_DispatchFromKeyboardEvent.
 * Purpose: Dispatches a raw keyboard event through the active bind-map command callback.
 */
extern "C" void __fastcall zInput_BindMapContext_DispatchFromKeyboardEvent(
    int dikCode
) {
    const int commandId = g_zInput_BindMap_Current->GetCommandByAnyKeyboardKey(dikCode);
    zInputCommandCallbackFn callback = g_zInput_BindMap_Current->m_commandCallbacks[commandId];
    if (callback != 0) {
        callback(commandId);
    }
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-readcommandinputstate
 * @recoil-artifact defines .text recoil:function:0x470eb0: zInput_BindMapContext::ReadCommandInputState.
 * Purpose: Combine keyboard, joystick, and mouse transition states for one command.
 */
int zInput_BindMapContext::ReadCommandInputState(
    int commandIndex
) {
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
    if ((unsigned int)(joystickButton) > 0 && (unsigned int)(joystickButton) < 0x0b) {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.zinput-bindmapcontext-copycommandlabel
 * @recoil-artifact defines .text recoil:function:0x470f50: zInput_BindMapContext::CopyCommandLabel.
 * Binary Ninja reads the class-owned m_commandLabels table at offset 0x10,
 * returns null for a missing command label, and otherwise copies the label with
 * strncpy using the caller's byte limit.
 * Purpose: Copy the recovered command label for one bind-map command.
 */
char * zInput_BindMapContext::CopyCommandLabel(
    int commandId,
    char *destBuf,
    int maxBytes
) {
    char *source = m_commandLabels[commandId];
    if (source == 0) {
        return 0;
    }

    return strncpy(
        destBuf,
        source,
        maxBytes
    );
}

namespace zInput {

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-formatkeycomboname
 * @recoil-artifact defines .text recoil:function:0x470f80: zInput::BindMap_FormatKeyComboName.
 * @recoil-artifact emits .data recoil:logical-data:0x4e5ce0:zinput-bindmap-format-key-combo-name-empty-literal: VC5 pooled empty-string literal occurrence.
 * Binary Ninja shows the zinput.cpp helper reading g_zInput_DikKeyNames,
 * appending Ctrl/Alt/Shift prefixes in retail order, and returning an empty
 * string when the DIK slot has no name.
 * Purpose: Format a packed keyboard binding into the user-visible key name.
 */
char *__stdcall BindMap_FormatKeyComboName(
    int packedKey,
    char *destBuf,
    int maxBytes
) {
    const char *keyName = g_zInput_DikKeyNames[packedKey & 0xff];
    if (keyName == 0) {
        return "";
    }

    int remaining = maxBytes;
    *destBuf = '\0';
    if ((packedKey & 0x200) != 0) {
        strncat(
            destBuf,
            g_zInput_KeyNameCtrlPrefix,
            remaining
        );
        remaining -= (int)(strlen(destBuf));
    }
    if ((packedKey & 0x100) != 0) {
        strncat(
            destBuf,
            g_zInput_KeyNameAltPrefix,
            remaining
        );
        remaining -= (int)(strlen(destBuf));
    }
    if ((packedKey & 0x400) != 0) {
        strncat(
            destBuf,
            g_zInput_KeyNameShiftPrefix,
            remaining
        );
        remaining -= (int)(strlen(destBuf));
    }

    return strncat(
        destBuf,
        keyName,
        remaining
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-copyjoystickbuttonname
 * @recoil-artifact defines .text recoil:function:0x471040: zInput::BindMap_CopyJoystickButtonName.
 * @recoil-artifact emits .data recoil:logical-data:0x4e5ce0:zinput-bindmap-copy-joystick-button-name-empty-literal: VC5 pooled empty-string literal occurrence.
 * Binary Ninja reads the one-based g_zInput_JoystickButtonNames table, returns
 * an empty string for an empty slot, or copies the selected literal.
 * Purpose: Copy a joystick button name for bind-map display.
 */
char *__stdcall BindMap_CopyJoystickButtonName(
    int joystickSlot,
    char *outBuf,
    int bufSize
) {
    const char *source = g_zInput_JoystickButtonNames[joystickSlot];
    if (source == 0) {
        return "";
    }

    return strncpy(
        outBuf,
        source,
        bufSize
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-copymousebuttonname
 * @recoil-artifact defines .text recoil:function:0x471070: zInput::BindMap_CopyMouseButtonName.
 * @recoil-artifact emits .data recoil:logical-data:0x4e5ce0:zinput-bindmap-copy-mouse-button-name-empty-literal: VC5 pooled empty-string literal occurrence.
 * Binary Ninja reads the one-based g_zInput_MouseButtonNames table, returns
 * an empty string for an empty slot, or copies the selected literal.
 * Purpose: Copy a mouse button name for bind-map display.
 */
char *__stdcall BindMap_CopyMouseButtonName(
    int mouseSlot,
    char *outBuf,
    int bufSize
) {
    const char *source = g_zInput_MouseButtonNames[mouseSlot];
    if (source == 0) {
        return "";
    }

    return strncpy(
        outBuf,
        source,
        bufSize
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapsystem-init
 * @recoil-artifact defines .text recoil:function:0x4710a0: zInput::BindMapSystem_Init.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zinput.cpp.
 * Binary Ninja shows this bootstrap allocating the current bind-map context,
 * initializing its command map, and then seeding the DIK, joystick, and mouse
 * name-table globals owned by engine.zinput.bindmap_name_table_system.
 * Purpose: Initialize the bind-map context and input name-table subsystem.
 */
void __fastcall BindMapSystem_Init(
    int commandCount
) {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-initdikkeynametable
 * @recoil-artifact defines .text recoil:function:0x471120: zInput::BindMap_InitDikKeyNameTable.
 * Binary Ninja shows the zinput.cpp initializer filling the BSS DIK-name
 * pointer table at g_zInput_DikKeyNames with the recovered key-name literals.
 * Purpose: Populate the DirectInput key-name lookup table used by bind-map UI.
 */
void BindMap_InitDikKeyNameTable() {
    g_zInput_DikKeyNames[1] = g_zInput_KeyNameEscape;
    g_zInput_DikKeyNames[2] = "1";
    g_zInput_DikKeyNames[3] = "2";
    g_zInput_DikKeyNames[4] = (const char *)&g_zInput_KeyCharRow_0987653[6];
    g_zInput_DikKeyNames[5] = "4";
    g_zInput_DikKeyNames[6] = (const char *)&g_zInput_KeyCharRow_0987653[5];
    g_zInput_DikKeyNames[7] = (const char *)&g_zInput_KeyCharRow_0987653[4];
    g_zInput_DikKeyNames[8] = (const char *)&g_zInput_KeyCharRow_0987653[3];
    g_zInput_DikKeyNames[9] = (const char *)&g_zInput_KeyCharRow_0987653[2];
    g_zInput_DikKeyNames[0x0a] = (const char *)&g_zInput_KeyCharRow_0987653[1];
    g_zInput_DikKeyNames[0x0b] = (const char *)&g_zInput_KeyCharRow_0987653[0];
    g_zInput_DikKeyNames[0x0c] = g_zInput_KeyNameMinus;
    g_zInput_DikKeyNames[0x0d] = g_zInput_KeyNameEquals;
    g_zInput_DikKeyNames[0x0e] = "BACK";
    g_zInput_DikKeyNames[0x0f] = g_zInput_KeyNameTab;
    g_zInput_DikKeyNames[0x10] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[9];
    g_zInput_DikKeyNames[0x11] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[8];
    g_zInput_DikKeyNames[0x12] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[7];
    g_zInput_DikKeyNames[0x13] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[6];
    g_zInput_DikKeyNames[0x14] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[5];
    g_zInput_DikKeyNames[0x15] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[4];
    g_zInput_DikKeyNames[0x16] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[3];
    g_zInput_DikKeyNames[0x17] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[2];
    g_zInput_DikKeyNames[0x18] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[1];
    g_zInput_DikKeyNames[0x19] = (const char *)&g_zInput_KeyCharRow_POIUYTREWQ[0];
    g_zInput_DikKeyNames[0x1a] = "LBRACKET";
    g_zInput_DikKeyNames[0x1b] = "RBRACKET";
    g_zInput_DikKeyNames[0x1c] = "RETURN";
    g_zInput_DikKeyNames[0x1d] = "LCONTROL";
    g_zInput_DikKeyNames[0x1e] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[8];
    g_zInput_DikKeyNames[0x1f] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[7];
    g_zInput_DikKeyNames[0x20] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[6];
    g_zInput_DikKeyNames[0x21] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[5];
    g_zInput_DikKeyNames[0x22] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[4];
    g_zInput_DikKeyNames[0x23] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[3];
    g_zInput_DikKeyNames[0x24] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[2];
    g_zInput_DikKeyNames[0x25] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[1];
    g_zInput_DikKeyNames[0x26] = (const char *)&g_zInput_KeyCharRow_LKJHGFDSA[0];
    g_zInput_DikKeyNames[0x27] = g_zInput_KeyNameSemicolon;
    g_zInput_DikKeyNames[0x28] = g_zInput_KeyNameApostrophe;
    g_zInput_DikKeyNames[0x29] = g_zInput_KeyNameGrave;
    g_zInput_DikKeyNames[0x2a] = g_zInput_KeyNameLShift;
    g_zInput_DikKeyNames[0x2b] = g_zInput_KeyNameBackslash;
    g_zInput_DikKeyNames[0x2c] = (const char *)&g_zInput_KeyCharRow_MNBVCXZ[6];
    g_zInput_DikKeyNames[0x2d] = (const char *)&g_zInput_KeyCharRow_MNBVCXZ[5];
    g_zInput_DikKeyNames[0x2e] = (const char *)&g_zInput_KeyCharRow_MNBVCXZ[4];
    g_zInput_DikKeyNames[0x2f] = (const char *)&g_zInput_KeyCharRow_MNBVCXZ[3];
    g_zInput_DikKeyNames[0x30] = (const char *)&g_zInput_KeyCharRow_MNBVCXZ[2];
    g_zInput_DikKeyNames[0x31] = (const char *)&g_zInput_KeyCharRow_MNBVCXZ[1];
    g_zInput_DikKeyNames[0x32] = (const char *)&g_zInput_KeyCharRow_MNBVCXZ[0];
    g_zInput_DikKeyNames[0x33] = g_zInput_KeyNameComma;
    g_zInput_DikKeyNames[0x34] = g_zInput_KeyNamePeriod;
    g_zInput_DikKeyNames[0x35] = g_zInput_KeyNameSlash;
    g_zInput_DikKeyNames[0x36] = g_zInput_KeyNameRShift;
    g_zInput_DikKeyNames[0x37] = g_zInput_KeyNameMultiply;
    g_zInput_DikKeyNames[0x38] = g_zInput_KeyNameLMenu;
    g_zInput_DikKeyNames[0x39] = g_zInput_KeyNameSpace;
    g_zInput_DikKeyNames[0x3a] = g_zInput_KeyNameCapital;
    g_zInput_DikKeyNames[0x3b] = g_zInput_KeyNameF1;
    g_zInput_DikKeyNames[0x3c] = g_zInput_KeyNameF2;
    g_zInput_DikKeyNames[0x3d] = g_zInput_KeyNameF3;
    g_zInput_DikKeyNames[0x3e] = g_zInput_KeyNameF4;
    g_zInput_DikKeyNames[0x3f] = g_zInput_KeyNameF5;
    g_zInput_DikKeyNames[0x40] = g_zInput_KeyNameF6;
    g_zInput_DikKeyNames[0x41] = g_zInput_KeyNameF7;
    g_zInput_DikKeyNames[0x42] = g_zInput_KeyNameF8;
    g_zInput_DikKeyNames[0x43] = g_zInput_KeyNameF9;
    g_zInput_DikKeyNames[0x44] = g_zInput_KeyNameF10;
    g_zInput_DikKeyNames[0x45] = g_zInput_KeyNameNumLock;
    g_zInput_DikKeyNames[0x46] = g_zInput_KeyNameScroll;
    g_zInput_DikKeyNames[0x47] = g_zInput_KeyNameNumpad7;
    g_zInput_DikKeyNames[0x48] = g_zInput_KeyNameNumpad8;
    g_zInput_DikKeyNames[0x49] = g_zInput_KeyNameNumpad9;
    g_zInput_DikKeyNames[0x4a] = g_zInput_KeyNameSubtract;
    g_zInput_DikKeyNames[0x4b] = g_zInput_KeyNameNumpad4;
    g_zInput_DikKeyNames[0x4c] = g_zInput_KeyNameNumpad5;
    g_zInput_DikKeyNames[0x4d] = g_zInput_KeyNameNumpad6;
    g_zInput_DikKeyNames[0x4e] = g_zInput_KeyNameAdd;
    g_zInput_DikKeyNames[0x4f] = g_zInput_KeyNameNumpad1;
    g_zInput_DikKeyNames[0x50] = g_zInput_KeyNameNumpad2;
    g_zInput_DikKeyNames[0x51] = g_zInput_KeyNameNumpad3;
    g_zInput_DikKeyNames[0x52] = g_zInput_KeyNameNumpad0;
    g_zInput_DikKeyNames[0x53] = g_zInput_KeyNameDecimal;
    g_zInput_DikKeyNames[0x57] = g_zInput_KeyNameF11;
    g_zInput_DikKeyNames[0x58] = g_zInput_KeyNameF12;
    g_zInput_DikKeyNames[0x64] = g_zInput_KeyNameF13;
    g_zInput_DikKeyNames[0x65] = g_zInput_KeyNameF14;
    g_zInput_DikKeyNames[0x66] = g_zInput_KeyNameF15;
    g_zInput_DikKeyNames[0x70] = g_zInput_KeyNameKana;
    g_zInput_DikKeyNames[0x79] = g_zInput_KeyNameConvert;
    g_zInput_DikKeyNames[0x7b] = g_zInput_KeyNameNoConvert;
    g_zInput_DikKeyNames[0x7d] = g_zInput_KeyNameYen;
    g_zInput_DikKeyNames[0x8d] = g_zInput_KeyNameNumpadEquals;
    g_zInput_DikKeyNames[0x90] = g_zInput_KeyNameCircumflex;
    g_zInput_DikKeyNames[0x91] = g_zInput_KeyNameAt;
    g_zInput_DikKeyNames[0x92] = g_zInput_KeyNameColon;
    g_zInput_DikKeyNames[0x93] = g_zInput_KeyNameUnderline;
    g_zInput_DikKeyNames[0x94] = g_zInput_KeyNameKanji;
    g_zInput_DikKeyNames[0x95] = "STOP";
    g_zInput_DikKeyNames[0x96] = g_zInput_KeyNameAx;
    g_zInput_DikKeyNames[0x97] = g_zInput_KeyNameUnlabeled;
    g_zInput_DikKeyNames[0x9c] = g_zInput_KeyNameNumpadEnter;
    g_zInput_DikKeyNames[0x9d] = g_zInput_KeyNameRControl;
    g_zInput_DikKeyNames[0xb3] = g_zInput_KeyNameNumpadComma;
    g_zInput_DikKeyNames[0xb5] = g_zInput_KeyNameDivide;
    g_zInput_DikKeyNames[0xb7] = g_zInput_KeyNameSysRq;
    g_zInput_DikKeyNames[0xb8] = g_zInput_KeyNameRMenu;
    g_zInput_DikKeyNames[0xc7] = g_zInput_KeyNameHome;
    g_zInput_DikKeyNames[0xc8] = g_zInput_KeyNameUp;
    g_zInput_DikKeyNames[0xc9] = g_zInput_KeyNamePrior;
    g_zInput_DikKeyNames[0xcb] = "LEFT";
    g_zInput_DikKeyNames[0xcd] = "RIGHT";
    g_zInput_DikKeyNames[0xcf] = g_zInput_KeyNameEnd;
    g_zInput_DikKeyNames[0xd0] = g_zInput_KeyNameDown;
    g_zInput_DikKeyNames[0xd1] = g_zInput_KeyNameNext;
    g_zInput_DikKeyNames[0xd2] = g_zInput_KeyNameInsert;
    g_zInput_DikKeyNames[0xd3] = g_zInput_KeyNameDelete;
    g_zInput_DikKeyNames[0xdb] = g_zInput_KeyNameLWin;
    g_zInput_DikKeyNames[0xdc] = g_zInput_KeyNameRWin;
    g_zInput_DikKeyNames[0xdd] = g_zInput_KeyNameApps;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-initjoystickbuttonnametable
 * @recoil-artifact defines .text recoil:function:0x4715e0: zInput::BindMap_InitJoystickButtonNameTable.
 * Binary Ninja shows the zinput.cpp initializer filling one-based
 * g_zInput_JoystickButtonNames slots 1..8 with Button 1..Button 8 literals.
 * Purpose: Populate the joystick button-name lookup table used by bind-map UI.
 */
void BindMap_InitJoystickButtonNameTable() {
    g_zInput_JoystickButtonNames[1] = g_zInput_JoystickButtonName1;
    g_zInput_JoystickButtonNames[2] = g_zInput_JoystickButtonName2;
    g_zInput_JoystickButtonNames[3] = g_zInput_JoystickButtonName3;
    g_zInput_JoystickButtonNames[4] = g_zInput_JoystickButtonName4;
    g_zInput_JoystickButtonNames[5] = g_zInput_JoystickButtonName5;
    g_zInput_JoystickButtonNames[6] = g_zInput_JoystickButtonName6;
    g_zInput_JoystickButtonNames[7] = g_zInput_JoystickButtonName7;
    g_zInput_JoystickButtonNames[8] = g_zInput_JoystickButtonName8;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-initmousebuttonnametable
 * @recoil-artifact defines .text recoil:function:0x471640: zInput::BindMap_InitMouseButtonNameTable.
 * Binary Ninja shows the zinput.cpp initializer filling one-based
 * g_zInput_MouseButtonNames slots 1..3 with Left, Right, and Middle literals.
 * Purpose: Populate the mouse button-name lookup table used by bind-map UI.
 */
void BindMap_InitMouseButtonNameTable() {
    g_zInput_MouseButtonNames[1] = g_zInput_MouseButtonNameLeft;
    g_zInput_MouseButtonNames[2] = g_zInput_MouseButtonNameRight;
    g_zInput_MouseButtonNames[3] = g_zInput_MouseButtonNameMiddle;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapsystem-shutdown
 * @recoil-artifact defines .text recoil:function:0x471660: zInput::BindMapSystem_Shutdown.
 * Provisional source-placement hypothesis: D:\Proj\GameZRecoil\zInput\zinput.cpp.
 * BN drains overlay contexts through PopBindMapContextOverlay, frees the base
 * context's non-owned buffers before owned buffers, deletes the context, and
 * leaves g_zInput_BindMap_Current unchanged during process shutdown.
 * Purpose: shut down the active bind-map context stack.
 */
void BindMapSystem_Shutdown() {
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-current-rebuildlookupindices
 * @recoil-artifact defines .text recoil:function:0x4716b0: zInput::BindMap_Current_RebuildLookupIndices.
 * Purpose: Rebuild lookup indices for the active bind-map context.
 */
void BindMap_Current_RebuildLookupIndices() {
    g_zInput_BindMap_Current->RebuildLookupIndices();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-resetallbindings
 * @recoil-artifact defines .text recoil:function:0x4716c0: zInput::BindMapCurrent_ResetAllBindings.
 * Purpose: reset all bindings on the current global bind-map context.
 */
void BindMapCurrent_ResetAllBindings() {
    g_zInput_BindMap_Current->ResetAllBindings();
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-getprimarykeyboardkey
 * @recoil-artifact defines .text recoil:function:0x4716d0: zInput::BindMapCurrent_GetPrimaryKeyboardKey.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext primary-key getter.
 * Purpose: Return the current bind map's primary keyboard key for a command.
 */
int __fastcall BindMapCurrent_GetPrimaryKeyboardKey(
    int commandIndex
) {
    return g_zInput_BindMap_Current->GetPrimaryKeyboardKey(commandIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-getsecondarykeyboardkey
 * @recoil-artifact defines .text recoil:function:0x4716e0: zInput::BindMapCurrent_GetSecondaryKeyboardKey.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext secondary-key getter.
 * Purpose: Return the current bind map's secondary keyboard key for a command.
 */
int __fastcall BindMapCurrent_GetSecondaryKeyboardKey(
    int commandIndex
) {
    return g_zInput_BindMap_Current->GetSecondaryKeyboardKey(commandIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-getjoystickbuttonslot
 * @recoil-artifact defines .text recoil:function:0x4716f0: zInput::BindMapCurrent_GetJoystickButtonSlot.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext joystick-slot getter.
 * Purpose: Return the current bind map's joystick button slot for a command.
 */
int __fastcall BindMapCurrent_GetJoystickButtonSlot(
    int commandIndex
) {
    return g_zInput_BindMap_Current->GetJoystickButtonSlot(commandIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-getmousebuttonslot
 * @recoil-artifact defines .text recoil:function:0x471700: zInput::BindMapCurrent_GetMouseButtonSlot.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext mouse-slot getter.
 * Purpose: Return the current bind map's mouse button slot for a command.
 */
int __fastcall BindMapCurrent_GetMouseButtonSlot(
    int commandIndex
) {
    return g_zInput_BindMap_Current->GetMouseButtonSlot(commandIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-getcommandbyprimarykey
 * @recoil-artifact defines .text recoil:function:0x471710: zInput::BindMapCurrent_GetCommandByPrimaryKey.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext primary-key reverse lookup.
 * Purpose: Return the command bound to a primary keyboard key in the current bind map.
 */
int __fastcall BindMapCurrent_GetCommandByPrimaryKey(
    int keyboardKey
) {
    return g_zInput_BindMap_Current->GetCommandByPrimaryKey(keyboardKey);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-getcommandbysecondarykey
 * @recoil-artifact defines .text recoil:function:0x471720: zInput::BindMapCurrent_GetCommandBySecondaryKey.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext secondary-key reverse lookup.
 * Purpose: Return the command bound to a secondary keyboard key in the current bind map.
 */
int __fastcall BindMapCurrent_GetCommandBySecondaryKey(
    int keyboardKey
) {
    return g_zInput_BindMap_Current->GetCommandBySecondaryKey(keyboardKey);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-getcommandbyjoystickslot
 * @recoil-artifact defines .text recoil:function:0x471730: zInput::BindMapCurrent_GetCommandByJoystickSlot.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext joystick-slot reverse lookup.
 * Purpose: Return the command bound to a joystick button slot in the current bind map.
 */
int __fastcall BindMapCurrent_GetCommandByJoystickSlot(
    int joystickSlot
) {
    return g_zInput_BindMap_Current->GetCommandByJoystickSlot(joystickSlot);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-getcommandbymouseslot
 * @recoil-artifact defines .text recoil:function:0x471740: zInput::BindMapCurrent_GetCommandByMouseSlot.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext mouse-slot reverse lookup.
 * Purpose: Return the command bound to a mouse button slot in the current bind map.
 */
int __fastcall BindMapCurrent_GetCommandByMouseSlot(
    int mouseSlot
) {
    return g_zInput_BindMap_Current->GetCommandByMouseSlot(mouseSlot);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-setprimarykeybinding
 * @recoil-artifact defines .text recoil:function:0x471750: zInput::BindMapCurrent_SetPrimaryKeyBinding.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext primary-key binding setter.
 * Purpose: Set a command's primary keyboard binding in the current bind map.
 */
void __fastcall BindMapCurrent_SetPrimaryKeyBinding(
    int keyCode,
    int commandId
) {
    g_zInput_BindMap_Current->SetPrimaryKeyBinding(
        keyCode,
        commandId
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-setsecondarykeybinding
 * @recoil-artifact defines .text recoil:function:0x471760: zInput::BindMapCurrent_SetSecondaryKeyBinding.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext secondary-key binding setter.
 * Purpose: Set a command's secondary keyboard binding in the current bind map.
 */
void __fastcall BindMapCurrent_SetSecondaryKeyBinding(
    int keyCode,
    int commandId
) {
    g_zInput_BindMap_Current->SetSecondaryKeyBinding(
        keyCode,
        commandId
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-setjoystickbinding
 * @recoil-artifact defines .text recoil:function:0x471770: zInput::BindMapCurrent_SetJoystickBinding.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext joystick-slot binding setter.
 * Purpose: Set a command's joystick button binding in the current bind map.
 */
void __fastcall BindMapCurrent_SetJoystickBinding(
    int joystickSlot,
    int commandId
) {
    g_zInput_BindMap_Current->SetJoystickBinding(
        joystickSlot,
        commandId
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-setmousebinding
 * @recoil-artifact defines .text recoil:function:0x471780: zInput::BindMapCurrent_SetMouseBinding.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * the recovered zInput_BindMapContext mouse-slot binding setter.
 * Purpose: Set a command's mouse button binding in the current bind map.
 */
void __fastcall BindMapCurrent_SetMouseBinding(
    int mouseSlot,
    int commandId
) {
    g_zInput_BindMap_Current->SetMouseBinding(
        mouseSlot,
        commandId
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-current-setbindingrecord
 * @recoil-artifact defines .text recoil:function:0x471790: zInput::BindMap_Current_SetBindingRecord.
 * Purpose: Forward a complete binding record to the active bind-map context and return its command id.
 */
int __fastcall BindMap_Current_SetBindingRecord(
    int commandId,
    const char *labelSrc,
    int primaryKey,
    int secondaryKey,
    int joystickSlot,
    int mouseSlot
) {
    g_zInput_BindMap_Current
        ->SetBindingRecord(
            commandId,
            labelSrc,
            primaryKey,
            secondaryKey,
            joystickSlot,
            mouseSlot
        );
    return commandId;
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-current-setcommandcallback
 * @recoil-artifact defines .text recoil:function:0x4717c0: zInput::BindMapCurrent_SetCommandCallback.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * zInput_BindMapContext::SetCommandCallback with command id and callback
 * preserved.
 * Purpose: Install a command callback on the current bind map.
 */
int __fastcall BindMap_Current_SetCommandCallback(
    int commandId,
    zInputCommandCallbackFn callback
) {
    return g_zInput_BindMap_Current->SetCommandCallback(
        commandId,
        callback
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmap-current-readcommandinputstate
 * @recoil-artifact defines .text recoil:function:0x4717d0: zInput::BindMapCurrent_ReadCommandInputState.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * zInput_BindMapContext::ReadCommandInputState with the command index
 * preserved.
 * Purpose: Read the current input state for a bind-map command.
 */
int __fastcall BindMap_Current_ReadCommandInputState(
    int commandIndex
) {
    return g_zInput_BindMap_Current->ReadCommandInputState(commandIndex);
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-copycommandlabel
 * @recoil-artifact defines .text recoil:function:0x4717e0: zInput::BindMapCurrent_CopyCommandLabel.
 * Binary Ninja shows a namespace forwarder through g_zInput_BindMap_Current to
 * zInput_BindMapContext::CopyCommandLabel with command id, destination buffer,
 * and byte limit preserved.
 * Purpose: Copy a command label from the current bind map.
 */
char *__fastcall BindMapCurrent_CopyCommandLabel(
    int commandId,
    char *destBuf,
    int maxBytes
) {
    return g_zInput_BindMap_Current->CopyCommandLabel(
        commandId,
        destBuf,
        maxBytes
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-formatkeycomboname
 * @recoil-artifact defines .text recoil:function:0x471800: zInput::BindMapCurrent_FormatKeyComboName.
 * Binary Ninja shows the current-map namespace wrapper forwarding the packed
 * key, destination buffer, and byte limit to BindMap_FormatKeyComboName.
 * Purpose: Format a packed keyboard binding for the current bind map.
 */
char *__fastcall BindMapCurrent_FormatKeyComboName(
    int packedKey,
    char *destBuf,
    int maxBytes
) {
    return BindMap_FormatKeyComboName(
        packedKey,
        destBuf,
        maxBytes
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-copyjoystickbuttonname
 * @recoil-artifact defines .text recoil:function:0x471820: zInput::BindMapCurrent_CopyJoystickButtonName.
 * Binary Ninja shows the current-map namespace wrapper forwarding the slot,
 * destination buffer, and byte limit to BindMap_CopyJoystickButtonName.
 * Purpose: Copy a joystick button name for the current bind map.
 */
char *__fastcall BindMapCurrent_CopyJoystickButtonName(
    int joystickSlot,
    char *outBuf,
    int bufSize
) {
    return BindMap_CopyJoystickButtonName(
        joystickSlot,
        outBuf,
        bufSize
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcurrent-copymousebuttonname
 * @recoil-artifact defines .text recoil:function:0x471840: zInput::BindMapCurrent_CopyMouseButtonName.
 * Binary Ninja shows the current-map namespace wrapper forwarding the slot,
 * destination buffer, and byte limit to BindMap_CopyMouseButtonName.
 * Purpose: Copy a mouse button name for the current bind map.
 */
char *__fastcall BindMapCurrent_CopyMouseButtonName(
    int mouseSlot,
    char *outBuf,
    int bufSize
) {
    return BindMap_CopyMouseButtonName(
        mouseSlot,
        outBuf,
        bufSize
    );
}

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcontext-push
 * @recoil-artifact defines .text recoil:function:0x471860: zInput::BindMapContext_Push.
 * Purpose: push a bind-map overlay context and rebuild the active command lookup tables.
 */
void __fastcall BindMapContext_Push(
    zInput_BindMapContext *bindMapOrNull
) {
    zInput_BindMapContext *bindMap = bindMapOrNull;
    if (bindMap == 0) {
        bindMap = new zInput_BindMapContext(g_zInput_BindMap_Current);
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
        } else {
            node = 0;
        }
    } else {
        zInput_BindMapOverlayStackNode *next = node->next;
        g_zInput_BindMapOverlayNodeFreeList = next;
        if (next != 0) {
            next->prev = 0;
        }
        node->next = 0;
    }

    node->bindMap = (zInput_BindMapContext *)(previousCurrent);
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

/**
 * @recoil-anchor recoil:anchor:gamezrecoil.zinput.zinput.bindmapcontext-pop
 * @recoil-artifact defines .text recoil:function:0x471950: zInput::BindMapContext_Pop.
 * Purpose: pop the active bind-map overlay, recycle its stack node, and rebuild command lookup tables.
 */
void BindMapContext_Pop() {
    zInput_BindMapContext *current = g_zInput_BindMap_Current;
    if (current->m_isOverlay != 0 && current != 0) {
        current->FreeAllBuffers();
        operator delete(current);
    }

    zInput_BindMapOverlayStackNode *node = g_zInput_BindMapOverlayNodeStackHead;
    if (node == 0) {
        node = 0;
    } else {
        zInput_BindMapOverlayStackNode *next = node->next;
        g_zInput_BindMapOverlayNodeStackHead = next;
        if (next != 0) {
            next->prev = 0;
        }
        node->prev = 0;
        node->next = 0;
    }

    zInput_BindMapContext *bindMap = 0;
    if (node != 0) {
        bindMap = node->bindMap;
        node->bindMap = 0;
        if (g_zInput_BindMapOverlayNodeFreeList == 0) {
            g_zInput_BindMapOverlayNodeFreeList = node;
            node->next = 0;
            node->prev = 0;
        } else {
            node->next = g_zInput_BindMapOverlayNodeFreeList;
            node->prev = 0;
            g_zInput_BindMapOverlayNodeFreeList->prev = node;
            g_zInput_BindMapOverlayNodeFreeList = node;
        }
        --g_zInput_BindMapOverlayDepth;
    }

    g_zInput_BindMap_Current = bindMap;
    bindMap->RebuildLookupIndices();
}

} // namespace zInput
