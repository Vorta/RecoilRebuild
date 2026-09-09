"""Recoil call-contract catalog evidence and checks."""

from __future__ import annotations

import re
from typing import Any, Mapping

from _recoil.call_contract.records import (
    ReviewedDirectIatStorageCallSpec,
    ReviewedDynamicExportCallSpec,
    ReviewedEhArrayDestructorCallSpec,
    ReviewedRegisterIatBridgeSpec,
    ReviewedVftableStorageBridgeSpec,
)
from _recoil.commands.asm_verify import IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32
from _recoil.lib.cpp_definition_closure import CallableKey
from _recoil.lib.tooling import REPO_ROOT

CALL_CONTRACT_MEMORY_TRACE_ENV = "RECOIL_CALL_CONTRACT_MEMORY_TRACE"


_R4564_DEPENDENT_CALLABLE_IDENTITIES: Mapping[
    str, tuple[CallableKey, str, int | None, str]
] = {
    "?BaseConstructor@HudUiNumericTextInput@@QAEPAU1@XZ": (
        CallableKey("HudUiNumericTextInput::BaseConstructor", ()),
        "__thiscall", 0, "HudUiNumericTextInput *",
    ),
    "??1HudUiNumericTextInput@@UAE@XZ": (
        CallableKey("HudUiNumericTextInput::~HudUiNumericTextInput", ()),
        "__thiscall", 0, "void",
    ),
    "?Constructor@HudUiZrdWidget@@QAEPAU1@XZ": (
        CallableKey("HudUiZrdWidget::Constructor", ()),
        "__thiscall", 0, "HudUiZrdWidget *",
    ),
    "?TransformPointByMatrix@Player@@YI?AUzVec3@@ABU2@ABUzMat4x3@@@Z": (
        CallableKey("Player::TransformPointByMatrix", (
            "const zVec3 &", "const zMat4x3 &",
        )),
        "__fastcall", 8, "zVec3",
    ),
    "?ResetAltGunAttachNode@Player@@YIXPAUPlayerGunFireController@@@Z": (
        CallableKey("Player::ResetAltGunAttachNode", (
            "PlayerGunFireController *",
        )),
        "__fastcall", 4, "void",
    ),
    "?Constructor@HudUiWidget@@QAEPAU1@I@Z": (
        CallableKey("HudUiWidget::Constructor", ("unsigned int",)),
        "__thiscall", 4, "HudUiWidget *",
    ),
    "?DestructorCore@HudUiZrdWidget@@QAEXXZ": (
        CallableKey("HudUiZrdWidget::DestructorCore", ()),
        "__thiscall", 0, "void",
    ),
    "?SetEnabled@HudUiBackgroundContainer@@UAEXH@Z": (
        CallableKey("HudUiBackgroundContainer::SetEnabled", ("int",)),
        "__thiscall", 4, "void",
    ),
    "?Constructor@HudUiElement@@QAEPAU1@HH@Z": (
        CallableKey("HudUiElement::Constructor", ("int", "int")),
        "__thiscall", 8, "HudUiElement *",
    ),
    "?DestructorCore@HudUiWidget@@QAEXXZ": (
        CallableKey("HudUiWidget::DestructorCore", ()),
        "__thiscall", 0, "void",
    ),
}


_R4564_NON_SOURCE_DEPENDENT_IDENTITIES: Mapping[str, str] = {
    "?Delete@CException@@QAEXXZ": "provider-api",
    "_ICDecompress": "provider-api",
    "??_M@YGXPAXIHP6EX0@Z@Z": "compiler-lifecycle",
    "??_GWestwoodOnlineUpgradeDownloadEventSink@@QAEPAXI@Z": (
        "compiler-lifecycle"
    ),
}


_R4564_CALLER_SOURCE_EXTRA_HELPERS = frozenset({
    (
        "0x43aa30", "0x43acf0", "0x43aaaa",
        "?TransformPointByMatrix@Player@@YI?AUzVec3@@ABU2@ABUzMat4x3@@@Z",
    ),
    (
        "0x43acf0", "0x43afd0", "0x43ad85",
        "?TransformPointByMatrix@Player@@YI?AUzVec3@@ABU2@ABUzMat4x3@@@Z",
    ),
    (
        "0x43c850", "0x43c950", "0x43c8d5",
        "?ResetAltGunAttachNode@Player@@YIXPAUPlayerGunFireController@@@Z",
    ),
})


CALL_CONTRACT_MEMORY_TRACE_KIND = "call-contract-memory-trace"


CALL_CONTRACT_MEMORY_TRACE_CONTRACT_VERSION = 1


CALL_CONTRACT_MEMORY_TRACE_ROOT = REPO_ROOT / "build" / "live-validation"


# The register-IAT saved-stack suffix is an abstract proof domain, not a model
# of the process stack.  Keep one fixed ceiling so malformed or genuinely
# stack-growing control flow cannot allocate without bound, while admitting
# ordinary finite VC5 local frames whose slot count can exceed the caller's
# instruction count.  Stack prefixes that differ at CFG joins are summarized
# into the finite floor below, so cycles still stabilize independently of this
# ceiling.
_RETAIL_REGISTER_EXACT_STACK_SLOT_LIMIT = 0x4000


_COD_TERMINAL_WRAP_MARKER = " ; @recoil-cod-terminal-wrap "


CALL_CONTRACT_BN_MAX_FULL_CENSUS_BODIES = 4096


CALL_CONTRACT_BN_CALLS_PER_BODY = 24


CALL_CONTRACT_BN_FIXED_CALLS = 8192


DEFAULT_PROGRESS = REPO_ROOT / ".agent/RECONSTRUCTION_PROGRESS.sqlite3"


DEFAULT_REFERENCE = REPO_ROOT / "support/Recoil.exe"


ZSND_DESTROY_OWNED_DATA_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4a3690"
)


ZSND_DESTROY_OWNED_DATA_CALLER_START = "0x4a3690"


ZSND_DESTROY_OWNED_DATA_CALLER_END_EXCLUSIVE = "0x4a3850"


ZSND_DESTROY_OWNED_DATA_CALLER_SYMBOL = (
    "?DestroyOwnedData@zSndSample@@QAEHXZ"
)


ZSND_DESTROY_OWNED_DATA_TARGET_NAME = (
    "zsnd_create_4a2ea0_4a3930_authored_order"
)


ZSND_DESTROY_OWNED_DATA_FREE_IMPORT_SYMBOL = "__imp__free"


ZSND_DESTROY_OWNED_DATA_FREE_IAT_ADDRESS = "0x4cc5b4"


ZSND_DESTROY_OWNED_DATA_FREE_LOAD_OFFSET = 0x1B


ZSND_DESTROY_OWNED_DATA_FREE_RELOCATION_OFFSET = 0x1D


ZSND_DESTROY_OWNED_DATA_CANDIDATE_SIZE = 0x170


ZSND_DESTROY_OWNED_DATA_FREE_CALL_OFFSETS = (
    0x28,
    0x34,
    0x40,
    0x4F,
    0x61,
    0x73,
    0xB5,
    0xCE,
    0x11D,
    0x136,
)


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CALLER_IDENTITY = (
    "symbol:recoil:function:0x460bc0"
)


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CALLER_START = "0x460bc0"


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CALLER_END_EXCLUSIVE = "0x460f80"


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CALLER_SYMBOL = (
    "?SaveRunningAnimRecord@zEffect_Anim@@YIHPAUzZbdSectionCallbackCtx@@"
    "PAUzEffectAnimEntry@@HH@Z"
)


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_TARGET_NAME = (
    "zeffect_anim_save_4603d0_4622f0_authored_order"
)


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zeffect_anim_save_4603d0_4622f0_authored_order.json"
)


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_SOURCE_PATH = (
    "src/GameZRecoil/zEffect/zeff_anim_save.c"
)


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IMPORT_SYMBOL = "__imp__fwrite"


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_ADDRESS = "0x4cc594"


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_IAT_IDENTITY = "iat:fwrite"


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CANDIDATE_CODE_SIZE = 0x3E4


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_CANDIDATE_COFF_SIZE = 0x3F0


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_LOAD_OFFSET = 0x1C6


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_CACHED_CALL_OFFSETS = (0x1DC, 0x206)


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_DIRECT_CALL_OFFSETS = (
    0x170,
    0x2C7,
    0x39B,
)


ZEFFECT_SAVE_RUNNING_ANIM_RECORD_FWRITE_RELOCATION_OFFSETS = (
    0x172,
    0x1C8,
    0x2C9,
    0x39D,
)


ZSND_WAVE_DATA_DESTRUCTOR_CANDIDATE_SYMBOL = "??1zSndWaveData@@QAE@XZ"


ZSND_WAVE_DATA_DESTRUCTOR_CORE_SYMBOL = (
    "?DestructorCore@zSndWaveData@@QAEXXZ"
)


ZSND_WAVE_DATA_DESTRUCTOR_CORE_NAVIGATION_NAME = (
    "zSndWaveData::DestructorCore"
)


ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_SYMBOL = (
    "??1zSndWaveData@@QAE@XZ"
)


ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4a5440"
)


ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_ADDRESS = "0x4a5440"


ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_END_EXCLUSIVE = "0x4a5460"


ZSND_WAVE_DATA_ORDINARY_DESTRUCTOR_NAVIGATION_NAME = (
    "zSndWaveData::Destructor"
)


ZSND_WAVE_DATA_DESTRUCTOR_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4a3850"
)


ZSND_WAVE_DATA_DESTRUCTOR_CALLER_START = "0x4a3850"


ZSND_WAVE_DATA_DESTRUCTOR_CALLER_END_EXCLUSIVE = "0x4a3910"


ZSND_WAVE_DATA_DESTRUCTOR_CALLER_SYMBOL = (
    "@zSndSampleCreateQueuedStreamingSample@12"
)


ZSND_WAVE_DATA_DESTRUCTOR_CALL_ORDINAL = 4


ZSND_WAVE_DATA_DESTRUCTOR_SOURCE_PATH = (
    "src/GameZRecoil/zSound/zsnd.cpp"
)


ZSND_WAVE_DATA_DESTRUCTOR_SOURCE_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil-zsound-zsnd-zsndwavedata-destructor"
)


ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_ID = (
    "recoil:vc5-target:zsnd_4a53f0_4a5670_authored_order"
)


ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_NAME = (
    "zsnd_4a53f0_4a5670_authored_order"
)


ZSND_WAVE_DATA_DESTRUCTOR_ORDER_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/zsnd_4a53f0_4a5670_authored_order.json"
)


ZSND_WAVE_DATA_DESTRUCTOR_FOCUSED_TARGET_ID = (
    "recoil:vc5-target:zsnd_wave_data_destructor"
)


ZSND_WAVE_DATA_DESTRUCTOR_FOCUSED_TARGET_NAME = (
    "zsnd_wave_data_destructor"
)


ZSND_WAVE_DATA_DESTRUCTOR_FOCUSED_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/zsnd_wave_data_destructor.json"
)


ZSND_WAVE_DATA_DESTRUCTOR_CALLER_TARGET_ID = (
    "recoil:vc5-target:zsnd_create_4a2ea0_4a3930_authored_order"
)


ZSND_WAVE_DATA_DESTRUCTOR_CALLER_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/zsnd_create_4a2ea0_4a3930_authored_order.json"
)


ZSND_WAVE_DATA_DESTRUCTOR_CALLER_SOURCE_PATH = (
    "src/GameZRecoil/zSound/zsnd_create.cpp"
)


HUD_UI_MP_EXIT_UPDATE_CALLER_IDENTITY = "symbol:recoil:function:0x419690"


HUD_UI_MP_EXIT_UPDATE_CALLER_START = "0x419690"


HUD_UI_MP_EXIT_UPDATE_CALLER_END_EXCLUSIVE = "0x419740"


HUD_UI_MP_EXIT_UPDATE_CALLER_SYMBOL = "?Update@HudUiMpExitDialog@@UAEXM@Z"


HUD_UI_MP_EXIT_UPDATE_SOURCE_PATH = "src/Battlesport/mission.cpp"


HUD_UI_MP_EXIT_UPDATE_TARGET_ID = (
    "recoil:vc5-target:mission_417350_41cc10_authored_order"
)


HUD_UI_MP_EXIT_UPDATE_TARGET_NAME = "mission_417350_41cc10_authored_order"


HUD_UI_MP_EXIT_UPDATE_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "mission_417350_41cc10_authored_order.json"
)


HUD_UI_MP_EXIT_UPDATE_STACK_SYMBOL_ID = "recoil:data:0x56bd24"


HUD_UI_MP_EXIT_UPDATE_STACK_STORAGE_ID = "recoil:storage:va:0x56bd24"


HUD_UI_MP_EXIT_UPDATE_STACK_ADDRESS = "0x56bd24"


HUD_UI_MP_EXIT_UPDATE_STACK_NAME = "g_HudUiTopMessageStack"


HUD_UI_MP_EXIT_UPDATE_STACK_OBJECT_SYMBOL = (
    "?g_HudUiTopMessageStack@@3PAUHudUiTextStack4@@A"
)


HUD_UI_MP_EXIT_UPDATE_STACK_TARGET_ID = (
    "recoil:vc5-target:hud_ui_top_message_stack_global"
)


HUD_UI_MP_EXIT_UPDATE_CALL_OFFSET = 0x8A


HUD_UI_MP_EXIT_UPDATE_CALL_ORDINAL = 5


HUD_UI_MP_EXIT_UPDATE_CANDIDATE_BODY = bytes.fromhex(
    "51568bf1578b86ecab000085c07c37d9442410d886e8ab0000d81500000000"
    "d996e8ab0000dfe0f6c4017406d95c2408eb0addd8c74424080000803f8b44"
    "240850e800000000e8000000008b8ee4ab00006a006a006a0033d2e8000000"
    "008b7c24108bce57e8000000008b86ecab000085c07c0857e800000000eb10"
    "8b0d00000000a100000000508b11ff12e800000000e8000000008bf0e80000"
    "00006a016a008bd68bc8e8000000005f5e59c20400"
)


RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x419940"
)


RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_START = "0x419940"


RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_END_EXCLUSIVE = "0x419990"


RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_SYMBOL = (
    "?OnDeactivate@RecoilApp_MpExitDialogState@@UAEXXZ"
)


RECOIL_APP_MP_EXIT_DEACTIVATE_SOURCE_PATH = "src/Battlesport/mission.cpp"


RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_ID = (
    "recoil:vc5-target:mission_417350_41cc10_authored_order"
)


RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_NAME = (
    "mission_417350_41cc10_authored_order"
)


RECOIL_APP_MP_EXIT_DEACTIVATE_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "mission_417350_41cc10_authored_order.json"
)


RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_TARGET_ID = (
    "recoil:vc5-target:recoil_app_mp_exit_dialog_state_on_deactivate"
)


RECOIL_APP_MP_EXIT_DEACTIVATE_CALLER_TARGET_NAME = (
    "recoil_app_mp_exit_dialog_state_on_deactivate"
)


RECOIL_APP_MP_EXIT_DIALOG_SYMBOL_ID = "recoil:data:0x4f329c"


RECOIL_APP_MP_EXIT_DIALOG_STORAGE_ID = "recoil:storage:va:0x4f329c"


RECOIL_APP_MP_EXIT_DIALOG_ADDRESS = "0x4f329c"


RECOIL_APP_MP_EXIT_DIALOG_NAME = "g_HudUiMpExitDialog"


RECOIL_APP_MP_EXIT_DIALOG_OBJECT_SYMBOL = (
    "?g_HudUiMpExitDialog@@3PAUHudUiMpExitDialog@@A"
)


RECOIL_APP_MP_EXIT_DIALOG_TARGET_ID = (
    "recoil:vc5-target:hud_ui_mp_exit_dialog_singleton_data"
)


RECOIL_APP_MP_EXIT_DEACTIVATE_CALL_OFFSET = 0x19


RECOIL_APP_MP_EXIT_DEACTIVATE_CALL_ORDINAL = 1


RECOIL_APP_MP_EXIT_DEACTIVATE_CANDIDATE_BODY = bytes.fromhex(
    "8b0d00000000e8000000008b0d0000000085c974078b016a01ff5008c705"
    "0000000000000000e80000000068e8030000ff1500000000b900000000e8"
    "000000006a00e800000000c39090909090909090"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_IDENTITY = (
    "symbol:recoil:function:0x419990"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_START = "0x419990"


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_END_EXCLUSIVE = "0x419aa0"


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_SYMBOL = (
    "?OnUpdateShouldQuit@RecoilApp_MpExitDialogState@@UAEHXZ"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SOURCE_PATH = (
    "src/Battlesport/mission.cpp"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_ID = (
    "recoil:vc5-target:mission_417350_41cc10_authored_order"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_NAME = (
    "mission_417350_41cc10_authored_order"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "mission_417350_41cc10_authored_order.json"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_TARGET_ID = (
    "recoil:vc5-target:recoil_app_mp_exit_dialog_state_on_update_should_quit"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALLER_TARGET_NAME = (
    "recoil_app_mp_exit_dialog_state_on_update_should_quit"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALL_OFFSET = 0x21


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CALL_ORDINAL = 2


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_FLIP_TO_GDI_SYMBOL = (
    "?FlipToGDIIfAttached@zVideo_dd@@YAXXZ"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SHUTDOWN_SYMBOL = (
    "?Shutdown@zSndSystem@@YAHXZ"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_SESSION_SHUTDOWN_SYMBOL = (
    "?ShutdownSessionRuntime@zNetwork@@YAHXZ"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_VIDEO_SHUTDOWN_SYMBOL = (
    "?ShutdownVideoSystem@zVideo@@YAHXZ"
)


# The caller-specific cdecl repairs change only the +0x9f, +0xa4, +0xa9, and
# +0xae COFF target spellings. The complete 0x110-byte candidate body,
# including the call rel32 opcodes at +0x9e, +0xa3, +0xa8, and +0xad, remains
# exact and continues to fail closed below.
RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_CANDIDATE_BODY = bytes.fromhex(
    "81ec0001000032c9e800000000e8000000008b0d000000008b150000000052"
    "8b01ff10a100000000d980e8ab0000d81d00000000dfe0f6c4410f85c600"
    "00005756b91c000000e8000000008bf883c9ff33c08d542408f2aef7d12b"
    "f98bc18bf78bfac1e902f3a58bc883e103f3a4b91d000000e8000000008b"
    "f883c9ff33c08d942488000000f2aef7d12bf98bc18bf78bfac1e902f3a5"
    "8bc883e103f3a4e800000000e800000000e800000000e8000000008d8c24"
    "880000008d54240851526800000000ff150000000083c40c68e8030000ff"
    "15000000006a10ff15000000008b15000000008d4424086a108d8c248c00"
    "0000505152ff150000000033c9e8000000005e5f33c081c400010000c39090"
)


RECOIL_APP_MP_EXIT_UPDATE_SHOULD_QUIT_COD_OFFSETS = (
    0x00,
    0x06,
    0x08,
    0x0D,
    0x12,
    0x18,
    0x1E,
    0x1F,
    0x21,
    0x23,
    0x28,
    0x2E,
    0x34,
    0x36,
    0x39,
    0x3F,
    0x40,
    0x41,
    0x46,
    0x4B,
    0x4D,
    0x50,
    0x52,
    0x56,
    0x58,
    0x5A,
    0x5C,
    0x5E,
    0x60,
    0x62,
    0x65,
    0x67,
    0x69,
    0x6C,
    0x6E,
    0x73,
    0x78,
    0x7A,
    0x7D,
    0x7F,
    0x86,
    0x88,
    0x8A,
    0x8C,
    0x8E,
    0x90,
    0x92,
    0x95,
    0x97,
    0x99,
    0x9C,
    0x9E,
    0xA3,
    0xA8,
    0xAD,
    0xB2,
    0xB9,
    0xBD,
    0xBE,
    0xBF,
    0xC4,
    0xCA,
    0xCD,
    0xD2,
    0xD8,
    0xDA,
    0xE0,
    0xE6,
    0xEA,
    0xEC,
    0xF3,
    0xF4,
    0xF5,
    0xF6,
    0xFC,
    0xFE,
    0x103,
    0x104,
    0x105,
    0x107,
    0x10D,
)


PROC_RE = re.compile(r"^\s*(\S+)\s+PROC\b", re.IGNORECASE)


ENDP_RE = re.compile(r"^\s*(\S+)\s+ENDP\b", re.IGNORECASE)


REGISTER_RE = re.compile(r"^(?:e?[abcd]x|e?[sd]i|e?[sb]p)$", re.IGNORECASE)


REGISTER32_RE = re.compile(
    r"^(?:eax|ebx|ecx|edx|esi|edi|esp|ebp)$",
    re.IGNORECASE,
)


PARTIAL_REGISTER32 = {
    "al": "eax",
    "ah": "eax",
    "ax": "eax",
    "bl": "ebx",
    "bh": "ebx",
    "bx": "ebx",
    "cl": "ecx",
    "ch": "ecx",
    "cx": "ecx",
    "dl": "edx",
    "dh": "edx",
    "dx": "edx",
    "si": "esi",
    "di": "edi",
    "sp": "esp",
    "bp": "ebp",
}


ADDRESS_RE = re.compile(r"\b0x[0-9a-fA-F]+\b")


DECORATED_RE = re.compile(
    r"(?<![A-Za-z0-9_@$?])"
    r"(?:\?[^,\s\]]+|__imp_[^,\s\]]+|@[A-Za-z][A-Za-z0-9_@$?]*"
    r"|_[A-Za-z][A-Za-z0-9_@$?]*)"
    r"(?![A-Za-z0-9_@$?])"
)


MEMORY_RE = re.compile(r"\[([^\]]+)\]")


STACK_CLEANUP_RE = re.compile(r"^add\s+esp\s*,\s*(0x[0-9a-f]+|\d+)$", re.IGNORECASE)


STACK_ALLOCATION_RE = re.compile(
    r"^sub\s+esp\s*,\s*(0x[0-9a-f]+|\d+)$", re.IGNORECASE
)


MSVC_COMPLETE_DESTRUCTOR_RE = re.compile(r"^\?\?1.+@@(?:QAE|UAE)@XZ$")


MSVC_CONSTRUCTOR_RE = re.compile(r"^\?\?0.+@@(?:QAE|UAE).+Z$")


MSVC_EXACT_ZEROARG_CONSTRUCTOR_RE = re.compile(
    r"^\?\?0"
    r"(?P<class_scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@QAE@XZ$"
)


MSVC_EXACT_SINGLE_ENUM_PARAMETER_CONSTRUCTOR_RE = re.compile(
    r"^\?\?0"
    r"(?P<class_scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@QAE@W4"
    r"(?P<enum_scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@@Z$"
)


MSVC_EXACT_NAMED_ZEROARG_CONSTRUCTOR_RE = re.compile(
    r"^\?Constructor@"
    r"(?P<class_scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@QAEPAU1@XZ$"
)


MSVC_EXACT_ZEROARG_DESTRUCTOR_RE = re.compile(
    r"^\?\?1"
    r"(?P<class_scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@(?:QAE|UAE)@XZ$"
)


MSVC_COMPLETE_DESTRUCTOR_PARTS_RE = re.compile(
    r"^\?\?1(?P<class_name>.+@@)(?P<qualifier>QAE|UAE)@XZ$"
)


MSVC_SCALAR_DELETING_DESTRUCTOR_PREFIX = "??_G"


MSVC_NONVIRTUAL_SCALAR_DELETING_DESTRUCTOR_RE = re.compile(
    r"^\?\?_G"
    r"(?P<class_scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@QAEPAXI@Z$"
)


MSVC_SCALAR_DELETING_DESTRUCTOR_CLASS_RE = re.compile(
    r"^\?\?_G"
    r"(?P<class_scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@"
)


MSVC_VIRTUAL_SCALAR_DELETING_DESTRUCTOR_RE = re.compile(
    r"^\?\?_G"
    r"(?P<class_scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@UAEPAXI@Z$"
)


SCALAR_DELETING_DESTRUCTOR_NAVIGATION_RE = re.compile(
    r"^(?P<class_name>.+)::ScalarDeletingDestructor$"
)


MSVC_VECTOR_DESTROY_PREFIX = "?_Destroy@?$vector@"


MSVC_VECTOR_NOOP_DESTROY_RE = re.compile(
    r"^\?_Destroy@\?\$vector@"
    r"(?P<element>(?:H|P(?:AU|AV)[A-Za-z0-9_?$@]+?@@|"
    r"[UV][A-Za-z0-9_?$@]+?@@))"
    r"V\?\$allocator@(?P=element)@std@@@std@@"
    r"IAEXPA(?P=element)0@Z$"
)


MSVC_POINTER_VECTOR_DESTROY_PROVIDER_RE = re.compile(
    r"^\?[A-Za-z_?$][A-Za-z0-9_?$]*@[A-Za-z_?$][A-Za-z0-9_?$]*@@"
    r"[QIA]AEX(?P<first_pointer>P[A-Za-z0-9_?$@]+)0@Z$"
)


MSVC_POINTER_VECTOR_DESTROY_NAVIGATION_RE = re.compile(
    r"^MSVC_STL::PtrVector_NoOpDestroyRange(?:_COMDAT)?$"
)


HUD_PANEL_LAYOUT_VECTOR_DESTROY_SYMBOL = (
    "?_Destroy@?$vector@UHudUiPanelLayoutEntry@@"
    "V?$allocator@UHudUiPanelLayoutEntry@@@std@@@std@@"
    "IAEXPAUHudUiPanelLayoutEntry@@0@Z"
)


HUD_COMPOSITE_PANEL_VECTOR_DESTROY_SYMBOL = (
    "?_Destroy@?$vector@UHudUiCompositePanelEntry@@"
    "V?$allocator@UHudUiCompositePanelEntry@@@std@@@std@@"
    "IAEXPAUHudUiCompositePanelEntry@@0@Z"
)


HUD_PANEL_LAYOUT_ENTRY_DESTRUCTOR_SYMBOL = (
    "??1HudUiPanelLayoutEntry@@QAE@XZ"
)


HUD_PANEL_LAYOUT_DESTROY_PROVIDER_CATALOG_SYMBOL = (
    "?DestroyRange@HudUiPanelLayoutEntry@@SGXPAU1@0@Z"
)


HUD_PANEL_LAYOUT_VECTOR_DESTROY_ADDRESS = "0x409b60"


HUD_PANEL_LAYOUT_VECTOR_DESTROY_SIZE = 0x30


HUD_PANEL_LAYOUT_VECTOR_DESTROY_RELOCATION_OFFSET = 0x11


HUD_CMD_BINDING_PTR_VECTOR_ERASE_SYMBOL = (
    "?erase@?$vector@PAUHudCmdBindingEntry@@"
    "V?$allocator@PAUHudCmdBindingEntry@@@std@@@std@@"
    "QAEPAPAUHudCmdBindingEntry@@PAPAU3@0@Z"
)


HUD_CMD_BINDING_PTR_VECTOR_ERASE_ADDRESS = "0x4ba4d0"


HUD_CMD_BINDING_PTR_VECTOR_ERASE_END_EXCLUSIVE = "0x4ba510"


HUD_CMD_BINDING_PTR_VECTOR_ERASE_IDENTITY = (
    "provider:recoil:function:0x4ba4d0"
)


HUD_CMD_BINDING_PTR_VECTOR_ERASE_NAVIGATION_NAME = (
    "MSVC_STL::VectorVoidPtr_EraseRange_COMDAT"
)


HUD_CMD_BINDING_PTR_VECTOR_ERASE_RETAIL_BODY = bytes.fromhex(
    "8b5424088b44240456578b79088bf03bd77410538b1a83c204891e83c604"
    "3bd775f25b8b51088971085f895424085ec20800"
)


MSVC_VFTABLE_RE = re.compile(r"^\?\?_7.+@@6B@$")


MSVC_EH_ARRAY_DESTRUCTOR_SYMBOL = "??_M@YGXPAXIHP6EX0@Z@Z"


HUD_PANEL_LAYOUT_VECTOR_ASSIGNMENT_SYMBOL = (
    "??4?$vector@UHudUiPanelLayoutEntry@@"
    "V?$allocator@UHudUiPanelLayoutEntry@@@std@@@std@@"
    "QAEAAV01@ABV01@@Z"
)


HUD_PANEL_LAYOUT_COPY_ASSIGN_PROVIDER_NAME = (
    "HudUiPanelLayoutEntry::CopyAssignRange"
)


HUD_PANEL_LAYOUT_COPY_ASSIGN_PROVIDER_REGEX = (
    r"\?CopyAssignRange@HudUiPanelLayoutEntry@@.*"
)


HUD_BRIEFING_OBJECTIVE_PICTURE_VFTABLE = (
    "??_7HudUiBriefingObjectivePicture@@6B@"
)


HUD_BRIEFING_RUNTIME_CONSTRUCTOR_SYMBOL = (
    "??0HudUiBriefingRuntime@@QAE@H@Z"
)


HUD_BRIEFING_OBJECTIVE_PICTURE_STORAGE_IDENTITY = (
    "storage:recoil:data:0x4cc898"
)


HUD_BRIEFING_OBJECTIVE_PICTURE_VFTABLE_SLOT = 0x20


HUD_ELEMENT_INVALIDATE_SYMBOL = "?Invalidate@HudUiElement@@UAEXXZ"


HUD_ELEMENT_CONSTRUCTOR_CALLER_START = "0x4b4070"


HUD_ELEMENT_CONSTRUCTOR_CALLER_END_EXCLUSIVE = "0x4b40c0"


HUD_ELEMENT_CONSTRUCTOR_SYMBOL = "??0HudUiElement@@QAE@HH@Z"


HUD_ELEMENT_VFTABLE_SYMBOL = "??_7HudUiElement@@6B@"


HUD_ELEMENT_VFTABLE_RETAIL_ADDRESS = "0x4cca10"


HUD_ELEMENT_VFTABLE_STORAGE_IDENTITY = "constructor-table:0x4cca10"


HUD_ELEMENT_VFTABLE_SIZE = 0x74


HUD_ELEMENT_INVALIDATE_SLOT_DISPLACEMENT = 0x20


HUD_ELEMENT_VPTR_STORE_RELOCATION_OFFSET = 0x15


HUD_ELEMENT_INVALIDATE_CALL_OFFSET = 0x25


HUD_ELEMENT_INVALIDATE_CALL_RELOCATION_OFFSET = 0x27


HUD_ELEMENT_SET_BLT_CALL_OFFSET = 0x36


HUD_ELEMENT_SET_BLT_CALL_RELOCATION_OFFSET = 0x37


HUD_ELEMENT_SET_BLT_SYMBOL = (
    "?SetBltSourceAndClipRect@HudUiElement@@UAEXPAXPBUHudUiRect@@@Z"
)


HUD_ELEMENT_SET_BLT_IDENTITY = "symbol:recoil:function:0x4b4190"


HUD_CONTAINER_CONSTRUCTOR_CALLER_START = "0x4bc780"


HUD_CONTAINER_CONSTRUCTOR_CALLER_END_EXCLUSIVE = "0x4bc7b0"


HUD_CONTAINER_CONSTRUCTOR_SYMBOL = "??0HudUiContainer@@QAE@XZ"


HUD_CONTAINER_VFTABLE_SYMBOL = "??_7HudUiContainer@@6B@"


HUD_CONTAINER_VFTABLE_RETAIL_ADDRESS = "0x4d3c58"


HUD_CONTAINER_VFTABLE_STORAGE_IDENTITY = "storage:recoil:data:0x4d3c58"


HUD_CONTAINER_VFTABLE_SIZE = 0x08


HUD_CONTAINER_SET_ENABLED_SYMBOL = "?SetEnabled@HudUiContainer@@UAEXH@Z"


HUD_CONTAINER_UPDATE_ALL_SYMBOL = "?UpdateAll@HudUiContainer@@UAEXM@Z"


HUD_BRIEFING_RUNTIME_VFTABLE = "??_7HudUiBriefingRuntime@@6B@"


HUD_BRIEFING_RUNTIME_DESTRUCTOR_SYMBOL = (
    "??1HudUiBriefingRuntime@@UAE@XZ"
)


HUD_BRIEFING_RUNTIME_STORAGE_IDENTITY = (
    "storage:recoil:data:0x4cc888"
)


HUD_BRIEFING_RUNTIME_VFTABLE_SLOT = 0x04


HUD_BACKGROUND_SET_ENABLED_SYMBOL = (
    "?SetEnabled@HudUiBackground@@UAEXH@Z"
)


HUD_BRIEFING_LOCATOR_PANEL_VFTABLE = (
    "??_7HudUiBriefingLocatorPanel@@6B@"
)


HUD_BRIEFING_LOCATOR_PANEL_CONSTRUCTOR_SYMBOL = (
    "??0HudUiBriefingLocatorPanel@@QAE@XZ"
)


HUD_BRIEFING_LOCATOR_PANEL_STORAGE_IDENTITY = (
    "storage:recoil:data:0x4cc998"
)


HUD_BRIEFING_LOCATOR_PANEL_VFTABLE_SLOT = 0x60


HUD_ELEMENT_SET_VISIBLE_SYMBOL = "?SetVisible@HudUiElement@@UAEXH@Z"


MSVC_FTOL_CANDIDATE_SYMBOL = "__ftol"


MSVC_FTOL_RETAIL_THUNK_NAME = "_ftol_import_thunk"


MSVC_FTOL_IMPORT_NAME = "_ftol"


MSVC_FTOL_PROVIDER_ADDRESS = "0x4c60a6"


MSVC_FTOL_IAT_ADDRESS = "0x4cc5ac"


MSVC_FTOL_PROVIDER_IDENTITY = "provider:recoil:function:0x4c60a6"


MSVC_FTOL_PROVIDER_SYMBOL_ID = "recoil:function:0x4c60a6"


MSVC_FTOL_PROVIDER_BLOCK_ID = "recoil:block:0x4c60a0"


MSVC_FTOL_IAT_OBJECT_SYMBOL = "__imp___ftol"


MSVC_FTOL_IAT_PROVIDER_IDENTITY = "provider:recoil:function:0x4cc5ac"


MSVC_FTOL_IAT_STORAGE_IDENTITY = "iat:_ftol"


MSVC_FTOL_IAT_OWNER_ID = "recoil:owner:provider.crt.ftol_import"


MSVC_CHKSTK_CANDIDATE_SYMBOL = "__chkstk"


MSVC_CHKSTK_RETAIL_NAME = "__alloca_probe"


MSVC_CHKSTK_TARGET_ADDRESS = "0x4c6100"


MSVC_CHKSTK_TARGET_END_EXCLUSIVE = "0x4c6130"


MSVC_CHKSTK_TARGET_SYMBOL_ID = "recoil:function:0x4c6100"


MSVC_CHKSTK_TARGET_IDENTITY = "provider:recoil:function:0x4c6100"


MSVC_CHKSTK_TARGET_BLOCK_ID = "recoil:block:0x4c60b0"


MSVC_CHKSTK_CALLER_ADDRESS = "0x415d30"


MSVC_CHKSTK_CALLER_END_EXCLUSIVE = "0x415f40"


MSVC_CHKSTK_CALLER_IDENTITY = "symbol:recoil:function:0x415d30"


MSVC_CHKSTK_CALLER_SYMBOL = (
    "?DrawOnTracker@HudSensorMapNode@@QAEHPAUHudSensorTracker@@"
    "PBUzVec3@@@Z"
)


MSVC_CHKSTK_CALLER_SPECS: Mapping[str, Mapping[str, Any]] = {
    MSVC_CHKSTK_CALLER_IDENTITY: {
        "address": MSVC_CHKSTK_CALLER_ADDRESS,
        "end_exclusive": MSVC_CHKSTK_CALLER_END_EXCLUSIVE,
        "size": 0x210,
        "physical_block_id": "recoil:block:0x415ab0",
        "symbol": MSVC_CHKSTK_CALLER_SYMBOL,
        "retail_setup_offset": 0,
        "retail_call_offset": 5,
        "retail_frame_size": 0x3024,
        "candidate_frame_size": 0x3034,
    },
    "symbol:recoil:function:0x438350": {
        "address": "0x438350",
        "end_exclusive": "0x4383e0",
        "size": 0x90,
        "physical_block_id": "recoil:block:0x438350",
        "symbol": "?ShowMessageBox@HudUi@@YIHPBD0PAX@Z",
        "retail_setup_offset": 0x0E,
        "retail_call_offset": 0x1A,
        "retail_frame_size": 0xB214,
        "candidate_frame_size": 0xB214,
        "candidate_setup_offset": 0x0E,
        "candidate_call_offset": 0x1A,
    },
    "symbol:recoil:function:0x460490": {
        "address": "0x460490",
        "end_exclusive": "0x4606d0",
        "size": 0x240,
        "physical_block_id": "recoil:block:0x4603d0",
        "symbol": (
            "?SaveActivationRecords@zEffect_Anim@@"
            "YIHPAUzZbdSectionCallbackCtx@@@Z"
        ),
        "retail_setup_offset": 0,
        "retail_call_offset": 5,
        "retail_frame_size": 0x4484,
        "candidate_frame_size": 0x448C,
    },
    "symbol:recoil:function:0x461430": {
        "address": "0x461430",
        "end_exclusive": "0x461670",
        "size": 0x240,
        "physical_block_id": "recoil:block:0x4603d0",
        "symbol": (
            "?SaveAnimRecords@zEffect_Anim@@"
            "YIHPAUzZbdSectionCallbackCtx@@@Z"
        ),
        "retail_setup_offset": 0,
        "retail_call_offset": 5,
        "retail_frame_size": 0x4480,
        "candidate_frame_size": 0x4488,
        "target_id": (
            "recoil:vc5-target:"
            "zeffect_anim_save_4603d0_4622f0_authored_order"
        ),
        "target_name": ZEFFECT_SAVE_RUNNING_ANIM_RECORD_TARGET_NAME,
        "target_manifest": ZEFFECT_SAVE_RUNNING_ANIM_RECORD_TARGET_MANIFEST,
        "source_path": ZEFFECT_SAVE_RUNNING_ANIM_RECORD_SOURCE_PATH,
        "source_anchor": (
            "recoil:anchor:gamezrecoil.zeffect.zeff-anim-save."
            "saveanimrecords"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zeffect_anim_save_4603d0_4622f0_authored_order"
            ),
        ),
    },
    "symbol:recoil:function:0x43e1c0": {
        "symbol_id": "recoil:function:0x43e1c0",
        "address": "0x43e1c0",
        "end_exclusive": "0x43e3a0",
        "size": 0x1E0,
        "physical_block_id": "recoil:block:0x43cf90",
        "symbol": (
            "?OnOK@WestwoodOnlineUpgradeDialog@@UAEXXZ"
        ),
        "retail_setup_offset": 0,
        "retail_call_offset": 5,
        "retail_frame_size": 0x1088,
        "candidate_frame_size": 0x1088,
        "target_id": (
            "recoil:vc5-target:wol_43cf90_442890_authored_order"
        ),
        "target_name": "wol_43cf90_442890_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "wol_43cf90_442890_authored_order.json"
        ),
        "source_path": "src/Battlesport/WOL.cpp",
        "target_shape": "wol-two-tu-authored",
        "target_row_name": (
            "WestwoodOnlineUpgradeDialog::"
            "SubmitVisibleSessionRequestsAndStatusText"
        ),
        "verification_target_ids": (
            "recoil:vc5-target:westwood_online_upgrade_dialog_functions",
            "recoil:vc5-target:wol_43cf90_442890_authored_order",
        ),
    },
    "symbol:recoil:function:0x43ec00": {
        "symbol_id": "recoil:function:0x43ec00",
        "address": "0x43ec00",
        "end_exclusive": "0x43ed10",
        "size": 0x110,
        "physical_block_id": "recoil:block:0x43cf90",
        "symbol": (
            "?QueueVisibleSessionRequests@"
            "WestwoodOnlineUpgradeDialog@@QAEXXZ"
        ),
        "retail_setup_offset": 0,
        "retail_call_offset": 5,
        "retail_frame_size": 0x1008,
        "candidate_frame_size": 0x1008,
        "target_id": "recoil:vc5-target:wol_43cf90_442890_authored_order",
        "target_name": "wol_43cf90_442890_authored_order",
        "target_manifest": (
            REPO_ROOT / "tools" / "vc5_verify_targets"
            / "wol_43cf90_442890_authored_order.json"
        ),
        "source_path": "src/Battlesport/WOL.cpp",
        "target_shape": "wol-two-tu-authored",
        "target_row_name": (
            "WestwoodOnlineUpgradeDialog::QueueVisibleSessionRequests"
        ),
        "verification_target_ids": (
            "recoil:vc5-target:westwood_online_upgrade_dialog_functions",
            "recoil:vc5-target:wol_43cf90_442890_authored_order",
        ),
    },
    "symbol:recoil:function:0x43ed10": {
        "symbol_id": "recoil:function:0x43ed10",
        "address": "0x43ed10",
        "end_exclusive": "0x43ee40",
        "size": 0x130,
        "physical_block_id": "recoil:block:0x43cf90",
        "symbol": (
            "?QueueVisibleSessionRequestsAndLookupBrowseRecords@"
            "WestwoodOnlineUpgradeDialog@@QAEXXZ"
        ),
        "retail_setup_offset": 0,
        "retail_call_offset": 5,
        "retail_frame_size": 0x1008,
        "candidate_frame_size": 0x1008,
        "target_id": "recoil:vc5-target:wol_43cf90_442890_authored_order",
        "target_name": "wol_43cf90_442890_authored_order",
        "target_manifest": (
            REPO_ROOT / "tools" / "vc5_verify_targets"
            / "wol_43cf90_442890_authored_order.json"
        ),
        "source_path": "src/Battlesport/WOL.cpp",
        "target_shape": "wol-two-tu-authored",
        "target_row_name": (
            "WestwoodOnlineUpgradeDialog::"
            "QueueVisibleSessionRequestsAndLookupBrowseRecords"
        ),
        "verification_target_ids": (
            "recoil:vc5-target:westwood_online_upgrade_dialog_functions",
            "recoil:vc5-target:wol_43cf90_442890_authored_order",
        ),
    },
    "symbol:recoil:function:0x43f6b0": {
        "symbol_id": "recoil:function:0x43f6b0",
        "address": "0x43f6b0",
        "end_exclusive": "0x43f830",
        "size": 0x180,
        "physical_block_id": "recoil:block:0x43cf90",
        "symbol": (
            "?OnBootstrapServerList@WestwoodOnlineUpgradeApiEventSink@@"
            "UAGHHPAUWestwoodOnlineUpgradeBootstrapServerRecord@@@Z"
        ),
        "retail_setup_offset": 0,
        "retail_call_offset": 5,
        "retail_frame_size": 0x1004,
        "candidate_frame_size": 0x1004,
        "target_id": "recoil:vc5-target:wol_43cf90_442890_authored_order",
        "target_name": "wol_43cf90_442890_authored_order",
        "target_manifest": (
            REPO_ROOT / "tools" / "vc5_verify_targets"
            / "wol_43cf90_442890_authored_order.json"
        ),
        "source_path": "src/Battlesport/WOL.cpp",
        "target_shape": "wol-two-tu-authored",
        "target_row_name": (
            "WestwoodOnlineUpgradeDialog::OnBootstrapServerList"
        ),
        "verification_target_ids": (
            "recoil:vc5-target:wol_43cf90_442890_authored_order",
        ),
    },
    "symbol:recoil:function:0x477b30": {
        "address": "0x477b30",
        "end_exclusive": "0x478c70",
        "size": 0x1140,
        "physical_block_id": "recoil:block:0x475c40",
        "symbol": (
            "?RenderNodeHardware@zModel@@"
            "YIXPAUzClass_NodePartial@@H@Z"
        ),
        "retail_setup_offset": 0x03,
        "retail_call_offset": 0x08,
        "retail_frame_size": 0x2080,
        "candidate_frame_size": 0x207C,
        "target_id": (
            "recoil:vc5-target:"
            "zmodel_gmod_init_475c40_4805b0_authored_order"
        ),
        "target_name": (
            "zmodel_gmod_init_475c40_4805b0_authored_order"
        ),
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zmodel_gmod_init_475c40_4805b0_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zModel/gmod_init.c",
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zmodel-gmod-init-"
            "zmodel-rendernodehardware"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zmodel_gmod_init_475c40_4805b0_authored_order"
            ),
            "recoil:vc5-target:zmodel_render_node_hardware",
        ),
    },
    "symbol:recoil:function:0x487f10": {
        "address": "0x487f10",
        "end_exclusive": "0x488d60",
        "size": 0xE50,
        "physical_block_id": "recoil:block:0x487a30",
        "symbol": "?SetActiveLights@zModel_Light@@YIHPAUzVec3@@HPAH1H@Z",
        "retail_setup_offset": 0x03,
        "retail_call_offset": 0x08,
        "retail_frame_size": 0x14180,
        "candidate_frame_size": 0x11180,
        "candidate_frame_sizes": (0x11178, 0x1117C, 0x11180),
        "target_id": (
            "recoil:vc5-target:"
            "zmodel_gmod_light_487a30_489d00_authored_order"
        ),
        "target_name": "zmodel_gmod_light_487a30_489d00_authored_order",
        "target_manifest": (
            REPO_ROOT / "tools" / "vc5_verify_targets"
            / "zmodel_gmod_light_487a30_489d00_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zModel/gmod_light.c",
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zmodel-gmod-light-"
            "zmodel-light-setactivelights"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zmodel_gmod_light_487a30_489d00_authored_order"
            ),
        ),
    },
    "symbol:recoil:function:0x488d60": {
        "address": "0x488d60",
        "end_exclusive": "0x4894f0",
        "size": 0x790,
        "physical_block_id": "recoil:block:0x487a30",
        "symbol": "?zModelLightBuildLightWeights@@YIHPAUzVec3@@HPAHM@Z",
        "retail_setup_offset": 0x03,
        "retail_call_offset": 0x08,
        "retail_frame_size": 0x10158,
        "candidate_frame_size": 0xD168,
        "candidate_frame_sizes": (0xD158, 0xD164, 0xD168),
        "target_id": (
            "recoil:vc5-target:"
            "zmodel_gmod_light_487a30_489d00_authored_order"
        ),
        "target_name": "zmodel_gmod_light_487a30_489d00_authored_order",
        "target_manifest": (
            REPO_ROOT / "tools" / "vc5_verify_targets"
            / "zmodel_gmod_light_487a30_489d00_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zModel/gmod_light.c",
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zmodel-gmod-light-"
            "zmodel-light-buildlightweights"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zmodel_gmod_light_487a30_489d00_authored_order"
            ),
        ),
    },
    "symbol:recoil:function:0x48daf0": {
        "address": "0x48daf0",
        "end_exclusive": "0x48e380",
        "size": 0x890,
        "physical_block_id": "recoil:block:0x48d340",
        "symbol": (
            "?FxPass3ApplyToCurrentSurface@zVideo@@"
            "YIXHHHHHMMPAUzVidRect32@@@Z"
        ),
        "candidate_symbol": MSVC_CHKSTK_RETAIL_NAME,
        "retail_call_offsets": (0x49, 0x58),
        "retail_ordinals": (0, 1),
        "retail_setup_rows": (
            (0x35, bytes.fromhex("8d 34 bd 04 00 00 00")),
            (0x3C, bytes.fromhex("8b c6")),
            (0x41, bytes.fromhex("83 c0 03")),
            (0x47, bytes.fromhex("24 fc")),
            (0x4E, bytes.fromhex("8b c6")),
            (0x53, bytes.fromhex("83 c0 03")),
            (0x56, bytes.fromhex("24 fc")),
        ),
        "candidate_call_offsets": (0x108, 0x11C),
        "candidate_setup_rows": (
            (0xFC, bytes.fromhex("8d 04 bd 04 00 00 00")),
            (0x103, bytes.fromhex("83 c0 03")),
            (0x106, bytes.fromhex("24 fc")),
            (0x10D, bytes.fromhex("8d 04 bd 04 00 00 00")),
            (0x114, bytes.fromhex("89 65 e8")),
            (0x117, bytes.fromhex("83 c0 03")),
            (0x11A, bytes.fromhex("24 fc")),
        ),
        "target_id": (
            "recoil:vc5-target:"
            "zrender_zrndr_draw_48d340_49f614_authored_order"
        ),
        "target_name": "zrender_zrndr_draw_48d340_49f614_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zrender_zrndr_draw_48d340_49f614_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zRender/zrndr_draw.c",
        "order_edit_paths": (
            "src/GameZRecoil/zRender/zrndr_draw.c",
            "src/GameZRecoil/zVideo/zvid_main.c",
            "src/GameZRecoil/zMath/zmth_main.c",
        ),
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zrender-zrndr-draw-"
            "fxpass3-applytocurrentsurface"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zrender_zrndr_draw_48d340_49f614_authored_order"
            ),
            "recoil:vc5-target:zvideo_fxpass3_surface_apply_helpers",
        ),
    },
    "symbol:recoil:function:0x49cbb0": {
        "address": "0x49cbb0",
        "end_exclusive": "0x49cea0",
        "size": 0x2F0,
        "physical_block_id": "recoil:block:0x48d340",
        "symbol": (
            "?SpanAlphaBlend565MmxFromTex16Alpha8@zRndr@@"
            "YIXHHHH@Z"
        ),
        "retail_ordinal": 0,
        "retail_setup_offset": 0x03,
        "retail_call_offset": 0x08,
        "retail_frame_size": 0x101C,
        "candidate_setup_offset": 0x03,
        "candidate_call_offset": 0x08,
        "candidate_frame_size": 0x1028,
        "target_id": (
            "recoil:vc5-target:"
            "zrender_zrndr_draw_48d340_49f614_authored_order"
        ),
        "target_name": "zrender_zrndr_draw_48d340_49f614_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zrender_zrndr_draw_48d340_49f614_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zRender/zrndr_draw.c",
        "order_edit_paths": (
            "src/GameZRecoil/zRender/zrndr_draw.c",
            "src/GameZRecoil/zVideo/zvid_main.c",
            "src/GameZRecoil/zMath/zmth_main.c",
        ),
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zrender-zrndr-draw-"
            "spanalphablend565mmxfromtex16alpha8"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zrender_zrndr_draw_48d340_49f614_authored_order"
            ),
            (
                "recoil:vc5-target:"
                "zrndr_span_alpha_blend_565_mmx_from_tex16_alpha8"
            ),
        ),
    },
    "symbol:recoil:function:0x49cea0": {
        "address": "0x49cea0",
        "end_exclusive": "0x49d1a0",
        "size": 0x300,
        "physical_block_id": "recoil:block:0x48d340",
        "symbol": (
            "?SpanAlphaBlend555MmxFromTex16Alpha8@zRndr@@"
            "YIXHHHH@Z"
        ),
        "retail_ordinal": 0,
        "retail_setup_offset": 0x03,
        "retail_call_offset": 0x08,
        "retail_frame_size": 0x101C,
        "candidate_setup_offset": 0x03,
        "candidate_call_offset": 0x08,
        "candidate_frame_size": 0x1028,
        "target_id": (
            "recoil:vc5-target:"
            "zrender_zrndr_draw_48d340_49f614_authored_order"
        ),
        "target_name": "zrender_zrndr_draw_48d340_49f614_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zrender_zrndr_draw_48d340_49f614_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zRender/zrndr_draw.c",
        "order_edit_paths": (
            "src/GameZRecoil/zRender/zrndr_draw.c",
            "src/GameZRecoil/zVideo/zvid_main.c",
            "src/GameZRecoil/zMath/zmth_main.c",
        ),
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zrender-zrndr-draw-"
            "spanalphablend555mmxfromtex16alpha8"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zrender_zrndr_draw_48d340_49f614_authored_order"
            ),
            (
                "recoil:vc5-target:"
                "zrndr_span_alpha_blend_555_mmx_from_tex16_alpha8"
            ),
        ),
    },
    "symbol:recoil:function:0x49da80": {
        "address": "0x49da80",
        "end_exclusive": "0x49ddb0",
        "size": 0x330,
        "physical_block_id": "recoil:block:0x48d340",
        "symbol": (
            "?SpanAlphaBlend565MmxFromPal8Alpha8@zRndr@@"
            "YIXHHHH@Z"
        ),
        "retail_ordinal": 0,
        "retail_setup_offset": 0x03,
        "retail_call_offset": 0x08,
        "retail_frame_size": 0x1020,
        "candidate_setup_offset": 0x03,
        "candidate_call_offset": 0x08,
        "candidate_frame_size": 0x102C,
        "target_id": (
            "recoil:vc5-target:"
            "zrender_zrndr_draw_48d340_49f614_authored_order"
        ),
        "target_name": "zrender_zrndr_draw_48d340_49f614_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zrender_zrndr_draw_48d340_49f614_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zRender/zrndr_draw.c",
        "order_edit_paths": (
            "src/GameZRecoil/zRender/zrndr_draw.c",
            "src/GameZRecoil/zVideo/zvid_main.c",
            "src/GameZRecoil/zMath/zmth_main.c",
        ),
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zrender-zrndr-draw-"
            "spanalphablend565mmxfrompal8alpha8"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zrender_zrndr_draw_48d340_49f614_authored_order"
            ),
            (
                "recoil:vc5-target:"
                "zrndr_span_alpha_blend_565_mmx_from_pal8_alpha8"
            ),
        ),
    },
    "symbol:recoil:function:0x49ddb0": {
        "address": "0x49ddb0",
        "end_exclusive": "0x49e0e0",
        "size": 0x330,
        "physical_block_id": "recoil:block:0x48d340",
        "symbol": (
            "?SpanAlphaBlend555MmxFromPal8Alpha8@zRndr@@"
            "YIXHHHH@Z"
        ),
        "retail_ordinal": 0,
        "retail_setup_offset": 0x03,
        "retail_call_offset": 0x08,
        "retail_frame_size": 0x1020,
        "candidate_setup_offset": 0x03,
        "candidate_call_offset": 0x08,
        "candidate_frame_size": 0x102C,
        "target_id": (
            "recoil:vc5-target:"
            "zrender_zrndr_draw_48d340_49f614_authored_order"
        ),
        "target_name": "zrender_zrndr_draw_48d340_49f614_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zrender_zrndr_draw_48d340_49f614_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zRender/zrndr_draw.c",
        "order_edit_paths": (
            "src/GameZRecoil/zRender/zrndr_draw.c",
            "src/GameZRecoil/zVideo/zvid_main.c",
            "src/GameZRecoil/zMath/zmth_main.c",
        ),
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zrender-zrndr-draw-"
            "spanalphablend555mmxfrompal8alpha8"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zrender_zrndr_draw_48d340_49f614_authored_order"
            ),
            (
                "recoil:vc5-target:"
                "zrndr_span_alpha_blend_555_mmx_from_pal8_alpha8"
            ),
        ),
    },
    "symbol:recoil:function:0x4af060": {
        "address": "0x4af060",
        "end_exclusive": "0x4b0530",
        "size": 0x14D0,
        "physical_block_id": "recoil:block:0x4ae380",
        "symbol": "?ProcessRuntimeInstances@OptCatalog@@YAXXZ",
        "retail_ordinal": 0,
        "retail_call_offset": 0x08,
        "retail_setup_rows": (
            (0x00, bytes.fromhex("55")),
            (0x01, bytes.fromhex("8b ec")),
            (0x03, bytes.fromhex("b8 6c 1f 00 00")),
        ),
        "candidate_call_offset": 0x05,
        "candidate_setup_rows": (
            (0x00, bytes.fromhex("b8 6c 1f 00 00")),
        ),
        "candidate_relocation_symbol_index": 289,
        "target_id": (
            "recoil:vc5-target:zwep_init_4ae380_4b2960_authored_order"
        ),
        "target_name": "zwep_init_4ae380_4b2960_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zwep_init_4ae380_4b2960_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zWeapon/zwep_init.c",
        "order_edit_paths": (
            "src/GameZRecoil/zWeapon/zwep_init.c",
            "src/GameZRecoil/zWeapon/zwep.h",
            "src/GameZRecoil/include/opt_catalog.h",
            "src/GameZRecoil/zClass/Light.c",
            "src/GameZRecoil/include/zClass.h",
            "src/GameZRecoil/zUtil/zsave_game.h",
        ),
        "source_anchor": (
            "recoil:anchor:gamezrecoil-zweapon-zwep-init-"
            "processruntimeinstances"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "zwep_init_4ae380_4b2960_authored_order"
            ),
        ),
    },
    "symbol:recoil:function:0x4b2960": {
        "address": "0x4b2960",
        "end_exclusive": "0x4b2bf0",
        "size": 0x290,
        "physical_block_id": "recoil:block:0x4b2960",
        "symbol": "?OptionsLoadFromRegistry@zGame@@YAHXZ",
        "candidate_symbol": MSVC_CHKSTK_RETAIL_NAME,
        "retail_call_offset": 0x52,
        "candidate_call_offset": 0x57,
        "retail_setup_rows": (
            (0x49, bytes.fromhex("8d 44 0a 05")),
            (0x4D, bytes.fromhex("83 c0 03")),
            (0x50, bytes.fromhex("24 fc")),
        ),
        "candidate_setup_rows": (
            (0x49, bytes.fromhex("8d 44 0a 03")),
            (0x4D, bytes.fromhex("83 c0 03")),
            (0x50, bytes.fromhex("24 fc")),
            (0x52, bytes.fromhex("83 c0 03")),
            (0x55, bytes.fromhex("24 fc")),
        ),
        "target_id": (
            "recoil:vc5-target:zgame_opt_4b2960_4b33f0_authored_order"
        ),
        "target_name": "zgame_opt_4b2960_4b33f0_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zgame_opt_4b2960_4b33f0_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zGame/zgame_opt.c",
        "order_edit_paths": (
            "src/GameZRecoil/zGame/zgame_opt.c",
            "src/GameZRecoil/zGame/zgame.h",
            "src/GameZRecoil/zSound/zsnd.h",
            "src/GameZRecoil/zSys/zsys.h",
            "src/GameZRecoil/zVideo/zvid.h",
        ),
        "source_anchor": (
            "recoil:anchor:src-gamezrecoil-zgame-zgame_opt-function-"
            "options_loadfromregistry"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "late_shelf_zgame_zsys_zui_zbd_order_current_shape"
            ),
            "recoil:vc5-target:zgame_opt_4b2960_4b33f0_authored_order",
            "recoil:vc5-target:zgame_options_load_helpers",
        ),
    },
    "symbol:recoil:function:0x4b2bf0": {
        "address": "0x4b2bf0",
        "end_exclusive": "0x4b2e80",
        "size": 0x290,
        "physical_block_id": "recoil:block:0x4b2960",
        "symbol": "?OptionsSaveToRegistry@zGame@@YAHXZ",
        "candidate_symbol": MSVC_CHKSTK_RETAIL_NAME,
        "retail_call_offset": 0x54,
        "candidate_call_offset": 0x57,
        "retail_setup_rows": (
            (0x4B, bytes.fromhex("8d 44 0a 05")),
            (0x4F, bytes.fromhex("83 c0 03")),
            (0x52, bytes.fromhex("24 fc")),
        ),
        "candidate_setup_rows": (
            (0x49, bytes.fromhex("8d 44 0a 03")),
            (0x4D, bytes.fromhex("83 c0 03")),
            (0x50, bytes.fromhex("24 fc")),
            (0x52, bytes.fromhex("83 c0 03")),
            (0x55, bytes.fromhex("24 fc")),
        ),
        "target_id": (
            "recoil:vc5-target:zgame_opt_4b2960_4b33f0_authored_order"
        ),
        "target_name": "zgame_opt_4b2960_4b33f0_authored_order",
        "target_manifest": (
            REPO_ROOT
            / "tools"
            / "vc5_verify_targets"
            / "zgame_opt_4b2960_4b33f0_authored_order.json"
        ),
        "source_path": "src/GameZRecoil/zGame/zgame_opt.c",
        "order_edit_paths": (
            "src/GameZRecoil/zGame/zgame_opt.c",
            "src/GameZRecoil/zGame/zgame.h",
            "src/GameZRecoil/zSound/zsnd.h",
            "src/GameZRecoil/zSys/zsys.h",
            "src/GameZRecoil/zVideo/zvid.h",
        ),
        "source_anchor": (
            "recoil:anchor:src-gamezrecoil-zgame-zgame_opt-function-"
            "options_savetoregistry"
        ),
        "verification_target_ids": (
            (
                "recoil:vc5-target:"
                "late_shelf_zgame_zsys_zui_zbd_order_current_shape"
            ),
            "recoil:vc5-target:zgame_opt_4b2960_4b33f0_authored_order",
            "recoil:vc5-target:zgame_options_load_helpers",
        ),
    },
}


MSVC_CHKSTK_RETAIL_BODY = bytes.fromhex(
    "51 3d 00 10 00 00 8d 4c 24 08 72 14 81 e9 00 10 00 00 "
    "2d 00 10 00 00 85 01 3d 00 10 00 00 73 ec 2b c8 8b c4 "
    "85 01 8b e1 8b 08 8b 40 04 50 c3 cc"
)


ZSND_REPORTER_TARGET_ID = (
    "recoil:vc5-target:zsnd_reporter_4a3ea0_4a44c0_authored_order"
)


ZSND_REPORTER_TARGET_NAME = "zsnd_reporter_4a3ea0_4a44c0_authored_order"


ZSND_REPORTER_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zsnd_reporter_4a3ea0_4a44c0_authored_order.json"
)


ZSND_REPORTER_SOURCE_PATH = "src/GameZRecoil/zSound/zsnd_error.cpp"


ZSND_REPORT_A3D_ADDRESS = "0x4a3ef0"


ZSND_REPORT_A3D_SYMBOL = "?ReportA3DError@zSnd@@YIHHPBDH@Z"


ZSND_REPORT_A3D_CANDIDATE_BODY_SIZE = 0x450


ZSND_REPORT_A3D_SWITCH_DISPATCH_OFFSET = 0x26


ZSND_REPORT_A3D_SWITCH_TABLE_OFFSET = 0x364


ZSND_REPORT_A3D_SWITCH_ENTRY_COUNT = 57


ZSND_REPORT_A3D_SWITCH_TARGET_OFFSETS = (
    0x02D, 0x03C, 0x046, 0x055, 0x064, 0x06E, 0x07D, 0x08C,
    0x096, 0x0A5, 0x0B4, 0x0BE, 0x0CD, 0x0DC, 0x0E6, 0x0F5,
    0x104, 0x10E, 0x11D, 0x12C, 0x136, 0x145, 0x154, 0x15E,
    0x16D, 0x17C, 0x186, 0x195, 0x1A4, 0x1AE, 0x1BD, 0x1CC,
    0x1D6, 0x1E5, 0x1F4, 0x1FE, 0x20D, 0x21C, 0x226, 0x235,
    0x244, 0x24E, 0x25D, 0x26C, 0x276, 0x285, 0x294, 0x29E,
    0x2AD, 0x2B9, 0x2C0, 0x2CC, 0x2D8, 0x2DF, 0x2EB, 0x2F7,
    0x2FE,
)


ZDECLIENT_QSAND_TARGET_ID = (
    "recoil:vc5-target:zdeclient_qsand_455ea0_456ad0_authored_order"
)


ZDECLIENT_QSAND_TARGET_NAME = "zdeclient_qsand_455ea0_456ad0_authored_order"


ZDECLIENT_QSAND_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zdeclient_qsand_455ea0_456ad0_authored_order.json"
)


ZDECLIENT_QSAND_SOURCE_PATH = "src/GameZRecoil/zDEClient/zdec_qsand.cpp"


ZDECLIENT_QSAND_CALLER_IDENTITY = "symbol:recoil:function:0x455ef0"


ZDECLIENT_QSAND_CALLER_ADDRESS = "0x455ef0"


ZDECLIENT_QSAND_CALLER_END_EXCLUSIVE = "0x456010"


ZDECLIENT_QSAND_CALLER_SYMBOL = (
    "?InstanceEventMaybeRelay@zDEClient_QSand@@"
    "YIHPAUzDEClient_QSandEventTemplate@@@Z"
)


ZDECLIENT_QSAND_CALLBACK_SYMBOL_ID = "recoil:data:0x539de4"


ZDECLIENT_QSAND_CALLBACK_ADDRESS = "0x539de4"


ZDECLIENT_QSAND_CALLBACK_STORAGE_ID = "recoil:storage:va:0x539de4"


ZDECLIENT_QSAND_CALLBACK_IDENTITY = "storage:recoil:data:0x539de4"


ZDECLIENT_QSAND_CALLBACK_RETAIL_TARGET_IDENTITY = (
    "symbol:recoil:function:0x433ca0"
)


ZDECLIENT_QSAND_CALLBACK_NAME = "g_zDEClientQSandNetRelayCallback"


ZDECLIENT_QSAND_CALLBACK_DECORATED_SYMBOL = (
    "?g_zDEClientQSandNetRelayCallback@@3P6IHPAX@ZA"
)


ZDECLIENT_QSAND_CALLBACK_OWNER_ID = (
    "recoil:owner:engine.zeffect.zdeclient_net_relay_callback_globals"
)


ZDECLIENT_QSAND_CALLBACK_TARGET_ID = (
    "recoil:vc5-target:zdeclient_net_relay_callback_globals"
)


ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/zdeclient_net_relay_callback_globals.json"
)


ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST_PATH = (
    REPO_ROOT / ZDECLIENT_QSAND_CALLBACK_TARGET_MANIFEST
)


ZDECLIENT_QSAND_CALLBACK_SOURCE_PATH = (
    "src/GameZRecoil/zDEClient/zdec_init.cpp"
)


ZDECLIENT_QSAND_CALLBACK_HEADER_PATH = "src/GameZRecoil/zDEClient/zdec.h"


ZDECLIENT_QSAND_CALLBACK_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil-zdeclient-zdec-init-"
    "g-zdeclientqsandnetrelaycallback"
)


ZDECLIENT_QSAND_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zdeclient.zdec-qsand."
    "zdeclient-qsand-instanceeventmayberelay"
)


ZDECLIENT_CRATER_TARGET_ID = (
    "recoil:vc5-target:zdeclient_crater_456ad0_458af0_authored_order"
)


ZDECLIENT_CRATER_TARGET_NAME = "zdeclient_crater_456ad0_458af0_authored_order"


ZDECLIENT_CRATER_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zdeclient_crater_456ad0_458af0_authored_order.json"
)


ZDECLIENT_CRATER_SOURCE_PATH = "src/GameZRecoil/zDEClient/zdec_crater.cpp"


ZDECLIENT_CRATER_CALLER_IDENTITY = "symbol:recoil:function:0x456c50"


ZDECLIENT_CRATER_CALLER_ADDRESS = "0x456c50"


ZDECLIENT_CRATER_CALLER_END_EXCLUSIVE = "0x456c80"


ZDECLIENT_CRATER_CALLER_SYMBOL = (
    "?InstanceEventMaybeRelay@zDEClient_Crater@@"
    "YIHPAUzDEClient_CraterEventTemplate@@@Z"
)


ZDECLIENT_CRATER_CALLBACK_SYMBOL_ID = "recoil:data:0x539de8"


ZDECLIENT_CRATER_CALLBACK_ADDRESS = "0x539de8"


ZDECLIENT_CRATER_CALLBACK_STORAGE_ID = "recoil:storage:va:0x539de8"


ZDECLIENT_CRATER_CALLBACK_IDENTITY = "storage:recoil:data:0x539de8"


ZDECLIENT_CRATER_CALLBACK_RETAIL_TARGET_IDENTITY = (
    "symbol:recoil:function:0x433ad0"
)


ZDECLIENT_CRATER_CALLBACK_NAME = "g_zDEClientCraterNetRelayCallback"


ZDECLIENT_CRATER_CALLBACK_DECORATED_SYMBOL = (
    "?g_zDEClientCraterNetRelayCallback@@3P6IHPAX@ZA"
)


ZDECLIENT_CRATER_CALLBACK_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil-zdeclient-zdec-init-"
    "g-zdeclientcraternetrelaycallback"
)


ZDECLIENT_CRATER_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil-zdeclient-zdec-crater-"
    "instanceeventmayberelay"
)


MSVC_CIASIN_CANDIDATE_SYMBOL = "__CIasin"


MSVC_CIASIN_RETAIL_NAME = "_CIasin"


MSVC_CIASIN_TARGET_ADDRESS = "0x4c6130"


MSVC_CIASIN_TARGET_END_EXCLUSIVE = "0x4c6140"


MSVC_CIASIN_TARGET_SYMBOL_ID = "recoil:function:0x4c6130"


MSVC_CIASIN_TARGET_IDENTITY = "provider:recoil:function:0x4c6130"


MSVC_CIASIN_TARGET_BLOCK_ID = "recoil:block:0x4c60b0"


MSVC_CIASIN_CALLER_ADDRESS = "0x472d30"


MSVC_CIASIN_CALLER_END_EXCLUSIVE = "0x472ed0"


MSVC_CIASIN_CALLER_IDENTITY = "symbol:recoil:function:0x472d30"


MSVC_CIASIN_CALLER_SYMBOL = "?CrtMatherrHandler@zMath@@YAHPAU_exception@@@Z"


MSVC_CIASIN_CALLER_BLOCK_ID = "recoil:block:0x472670"


MSVC_CIASIN_CALLER_SOURCE_PATH = "src/GameZRecoil/zMath/zmth_main.c"


MSVC_CIASIN_ORDER_EDIT_PATHS = (
    MSVC_CIASIN_CALLER_SOURCE_PATH,
    "src/GameZRecoil/zMath/zmth_decls.h",
)


MSVC_CIASIN_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil-zmath-zmth-main-zmath-crtmatherrhandler"
)


MSVC_CIASIN_TARGET_ID = (
    "recoil:vc5-target:zmath_zmth_main_472670_475c40_authored_order"
)


MSVC_CIASIN_TARGET_NAME = "zmath_zmth_main_472670_475c40_authored_order"


MSVC_CIASIN_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zmath_zmth_main_472670_475c40_authored_order.json"
)


MSVC_CIASIN_CALL_OFFSET = 0xEE


MSVC_CIASIN_RELOCATION_OFFSET = 0xEF


ZWEP_CALL_CONTRACT_TARGET_ID = (
    "recoil:vc5-target:zwep_init_4ae380_4b2960_authored_order"
)


ZWEP_CALL_CONTRACT_TARGET_NAME = (
    "zwep_init_4ae380_4b2960_authored_order"
)


ZWEP_CALL_CONTRACT_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zwep_init_4ae380_4b2960_authored_order.json"
)


ZWEP_CALL_CONTRACT_SOURCE_PATH = "src/GameZRecoil/zWeapon/zwep_init.c"


ZWEP_CALL_CONTRACT_ORDER_EDIT_PATHS = (
    ZWEP_CALL_CONTRACT_SOURCE_PATH,
    "src/GameZRecoil/zWeapon/zwep.h",
    "src/GameZRecoil/include/opt_catalog.h",
    "src/GameZRecoil/zClass/Light.c",
    "src/GameZRecoil/include/zClass.h",
    "src/GameZRecoil/zUtil/zsave_game.h",
)


ZWEP_CIASIN_CANDIDATE_PROFILES: Mapping[str, Mapping[str, Any]] = {
    "0x4ae660": {
        "end": "0x4aeaa0",
        "symbol": (
            "?AllocRuntimeInstance@OptCatalog@@YIPAU"
            "OptCatalogRuntimeInstanceStorage@@PAUOptCatalogEntryDef@@"
            "PAUzClass_NodePartial@@PAUzTag4Partial@@PAUzVec3@@33PAXPAU2@@Z"
        ),
        "name": "OptCatalog::AllocRuntimeInstance",
        "body_size": 0x410,
        "fsqrt_offset": 0x17B,
        "sites": ((0x250, "__CIasin"), (0x2A9, "__CIasin")),
        "retail_ordinals": (7, 9),
    },
    "0x4aee40": {
        "end": "0x4aefb0",
        "symbol": (
            "?ActivateTrailRuntimeState@OptCatalog@@YIXPAU"
            "OptCatalogTrailRuntimeState@@H@Z"
        ),
        "name": "OptCatalog::ActivateTrailRuntimeState",
        "body_size": 0x170,
        "sites": ((0xAB, "__CIasin"),),
        "retail_ordinals": (5,),
    },
    "0x4af060": {
        "end": "0x4b0530",
        "symbol": "?ProcessRuntimeInstances@OptCatalog@@YAXXZ",
        "name": "OptCatalog::ProcessRuntimeInstances",
        "body_size": 0x14C0,
        "sites": (
            (0x456, "__CIasin"),
            (0xA9D, "__CIasin"),
            (0x10C4, "__CIasin"),
        ),
        "retail_ordinals": (11, 42, 80),
    },
    "0x4b0f70": {
        "end": "0x4b0fd0",
        "symbol": (
            "?UpdateTrailSegmentVisual@OptCatalog@@YIXPAU"
            "OptCatalogTrailNodeSlot@@@Z"
        ),
        "name": "OptCatalog::UpdateTrailSegmentVisual",
        "body_size": 0x60,
        "sites": ((0x37, "__CIasin"),),
        "retail_ordinals": (2,),
    },
}


ZWEP_CIACOS_CANDIDATE_SYMBOL = "__CIacos"


ZWEP_CIACOS_RETAIL_NAME = "_CIacos"


ZWEP_CIACOS_IAT_ADDRESS = "0x4cc528"


ZWEP_CIACOS_IAT_IDENTITY = "iat:_CIacos"


ZWEP_CIACOS_TARGET_ADDRESS = "0x4c6320"


ZWEP_CIACOS_TARGET_IDENTITY = "provider:recoil:function:0x4c6320"


ZWEP_CIACOS_RETAIL_ORDINAL = 8


ZWEP_CIACOS_CALL_OFFSET = 0x30B


ZWEP_DAMAGE_CANDIDATE_PROFILES: Mapping[str, Mapping[str, Any]] = {
    "0x4b26f0": {
        "end": "0x4b2880",
        "symbol": (
            "?InvokeDamageFeedbackAndHitCallback@OptCatalog@@YIHPAU"
            "OptCatalogEntryDef@@PAUzClass_NodePartial@@PAUzVec3@@"
            "PAUOptCatalogHitEventPartial@@M@Z"
        ),
        "name": "OptCatalog::InvokeDamageFeedbackAndHitCallback",
        "body_size": 0x180,
        "call_offset": 0xD6,
        "register": "esi",
        "slot": 4,
    },
    "0x4b2880": {
        "end": "0x4b28e0",
        "symbol": (
            "?CaptureHitSnapshotAndInvokeDamageTimerCallback@OptCatalog@@"
            "YIMPAUzVec3@@PAUOptCatalogHitEventPartial@@M@Z"
        ),
        "name": (
            "OptCatalog::CaptureHitSnapshotAndInvokeDamageTimerCallback"
        ),
        "body_size": 0x60,
        "call_offset": 0x54,
        "register": "eax",
        "slot": 0x0C,
    },
}


ZWEP_RUNTIME_CALLBACK_CALLER_START = "0x4af060"


ZWEP_RUNTIME_CALLBACK_CALLER_END = "0x4b0530"


ZWEP_RUNTIME_CALLBACK_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4af060"
)


ZWEP_RUNTIME_CALLBACK_CALLER_SYMBOL = (
    "?ProcessRuntimeInstances@OptCatalog@@YAXXZ"
)


ZWEP_RUNTIME_CALLBACK_CANDIDATE_ROWS: Mapping[
    int, tuple[tuple[str, ...], str]
] = {
    0x106: (
        ("8b", "86", "88", "00", "00", "00"),
        r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[esi\+(?:136|0x88)\]",
    ),
    0x10C: (("85", "c0"), r"test\s+eax\s*,\s*eax"),
    0x10E: (("74", "04"), r"je\s+\S+"),
    0x110: (("8b", "ce"), r"mov\s+ecx\s*,\s*esi"),
    0x112: (("ff", "d0"), r"call\s+eax"),
}


ZWEP_RUNTIME_CALLBACK_BASELINE_CALL_OFFSET = 0x112


ZWEP_ENTRY_CALLBACK_CALLER_START = "0x4b1190"


ZWEP_ENTRY_CALLBACK_CALLER_END = "0x4b1d80"


ZWEP_ENTRY_CALLBACK_CALLER_IDENTITY = "symbol:recoil:function:0x4b1190"


ZWEP_ENTRY_CALLBACK_CALLER_SYMBOL = (
    "?LoadOptCatalogFromPath@zWeapon@@YIHPAUzClass_NodePartial@@PBDH"
    "P6IXPAUNode@zReader@@PAUOptCatalogEntryDef@@@Z@Z"
)


ZWEP_ENTRY_CALLBACK_CANDIDATE_BODY_SIZE = 0xC60


ZWEP_ENTRY_CALLBACK_RETAIL_INVOCATION_COUNT = 87


ZWEP_ENTRY_CALLBACK_CANDIDATE_INVOCATION_COUNT = 87


ZWEP_ENTRY_CALLBACK_RETAIL_ORDINAL = 53


ZWEP_ENTRY_CALLBACK_CANDIDATE_CALL_OFFSET = 0x707


ZWEP_ENTRY_CALLBACK_CANDIDATE_ROWS: Mapping[
    int, tuple[tuple[str, ...], str]
] = {
    0x000: (("83", "ec", "4c"), r"sub\s+esp\s*,\s*(?:76|0x4c)"),
    0x003: (("53",), r"push\s+ebx"),
    0x004: (("55",), r"push\s+ebp"),
    0x005: (("56",), r"push\s+esi"),
    0x006: (("57",), r"push\s+edi"),
    0x6F5: (
        ("8b", "44", "24", "64"),
        r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        r"_entryCallback\$\[esp\+(?:88|0x58)\]",
    ),
    0x6FF: (("3b", "c7"), r"cmp\s+eax\s*,\s*edi"),
    0x701: (("74", "06"), r"je\s+\S+"),
    0x703: (("8b", "d5"), r"mov\s+edx\s*,\s*ebp"),
    0x705: (("8b", "ce"), r"mov\s+ecx\s*,\s*esi"),
    0x707: (("ff", "d0"), r"call\s+eax"),
    0xC50: (("5f",), r"pop\s+edi"),
    0xC51: (("5e",), r"pop\s+esi"),
    0xC52: (("5d",), r"pop\s+ebp"),
    0xC53: (("5b",), r"pop\s+ebx"),
    0xC54: (("83", "c4", "4c"), r"add\s+esp\s*,\s*(?:76|0x4c)"),
    0xC57: (("c2", "08", "00"), r"ret(?:n)?\s+(?:8|0x8)"),
}


ZWEP_ENTRY_CALLBACK_RETAIL_ROWS: Mapping[
    int, tuple[tuple[str, ...], str]
] = {
    0x000: (("55",), r"push\s+ebp"),
    0x001: (("8b", "ec"), r"mov\s+ebp\s*,\s*esp"),
    0x003: (("83", "ec", "44"), r"sub\s+esp\s*,\s*(?:68|0x44)"),
    0x006: (("53",), r"push\s+ebx"),
    0x007: (("56",), r"push\s+esi"),
    0x008: (("57",), r"push\s+edi"),
    0x6C2: (
        ("8b", "45", "0c"),
        r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?\[ebp\+(?:12|0xc)\]",
    ),
    0x6CF: (("85", "c0"), r"test\s+eax\s*,\s*eax"),
    0x6D1: (("74", "06"), r"je\s+\S+"),
    0x6D3: (("8b", "d3"), r"mov\s+edx\s*,\s*ebx"),
    0x6D5: (("8b", "ce"), r"mov\s+ecx\s*,\s*esi"),
    0x6D7: (("ff", "d0"), r"call\s+eax"),
}


ZWEP_DAMAGE_TIMER_CALLER_START = "0x4b2880"


ZWEP_DAMAGE_TIMER_CALLER_END = "0x4b28e0"


ZWEP_DAMAGE_TIMER_CALLER_IDENTITY = "symbol:recoil:function:0x4b2880"


ZWEP_DAMAGE_TIMER_HANDLER_VPTR_STORAGE = (
    "dynamic:OptCatalogHitEventPartial+0x24->damage-handler+0xbc-vptr"
)


ZWEP_DAMAGE_FEEDBACK_CALLER_START = "0x4b26f0"


ZWEP_DAMAGE_FEEDBACK_CALLER_END = "0x4b2880"


ZWEP_DAMAGE_FEEDBACK_CALLER_IDENTITY = "symbol:recoil:function:0x4b26f0"


ZWEP_DAMAGE_FEEDBACK_HANDLER_VPTR_STORAGE = (
    "dynamic:OptCatalogHitEventPartial+0x24->damage-handler+0xbc-vptr"
)


WOL_CEDIT_CANDIDATE_SYMBOL = "??0CEdit@@QAE@XZ"


WOL_CEDIT_CANDIDATE_FULL_NAME = "CEdit::CEdit(CEdit* this)"


WOL_CBUTTON_CANDIDATE_SYMBOL = "??0CButton@@QAE@XZ"


WOL_CBUTTON_CANDIDATE_FULL_NAME = "CButton::CButton(CButton* this)"


WOL_CLISTBOX_CANDIDATE_SYMBOL = "??0CListBox@@QAE@XZ"


WOL_CLISTBOX_CANDIDATE_FULL_NAME = "CListBox::CListBox(CListBox* this)"


WOL_CCOMBOBOX_CANDIDATE_SYMBOL = "??0CComboBox@@QAE@XZ"


WOL_CCOMBOBOX_CANDIDATE_FULL_NAME = "CComboBox::CComboBox(CComboBox* this)"


WOL_CONTROL_CANDIDATE_SYMBOLS = (
    WOL_CEDIT_CANDIDATE_SYMBOL,
    WOL_CBUTTON_CANDIDATE_SYMBOL,
    WOL_CLISTBOX_CANDIDATE_SYMBOL,
    WOL_CCOMBOBOX_CANDIDATE_SYMBOL,
)


WOL_CONTROL_CANDIDATE_FULL_NAMES = (
    WOL_CEDIT_CANDIDATE_FULL_NAME,
    WOL_CBUTTON_CANDIDATE_FULL_NAME,
    WOL_CLISTBOX_CANDIDATE_FULL_NAME,
    WOL_CCOMBOBOX_CANDIDATE_FULL_NAME,
)


WOL_CONTROL_CANDIDATE_DESTRUCTOR_SYMBOLS = (
    "??1CEdit@@UAE@XZ",
    "??1CButton@@UAE@XZ",
    "??1CListBox@@UAE@XZ",
    "??1CComboBox@@UAE@XZ",
)


WOL_CSTRING_CANDIDATE_SYMBOL = "??0CString@@QAE@XZ"


WOL_CSTRING_CANDIDATE_FULL_NAME = "CString::CString(CString* this)"


WOL_CSTRING_TARGET_ADDRESS = "0x4c5ba0"


WOL_CSTRING_TARGET_END_EXCLUSIVE = "0x4c5ba6"


WOL_CSTRING_TARGET_SYMBOL_ID = "recoil:function:0x4c5ba0"


WOL_CSTRING_TARGET_IDENTITY = "provider:recoil:function:0x4c5ba0"


WOL_CSTRING_OWNER_ID = (
    "recoil:owner:provider.mfc42.cstring_default_constructor_thunk"
)


WOL_CSTRING_RETAIL_NAME = "CString::ConstructorDefault"


WOL_CSTRING_IMPORT_NAME = "??0CString@@QAE@XZ"


WOL_CSTRING_IMPORT_FULL_NAME = "CString::CString(CString* this)"


WOL_CSTRING_IAT_OBJECT_SYMBOL = "__imp_??0CString@@QAE@XZ"


WOL_CSTRING_IMPORT_ORDINAL = 540


WOL_CSTRING_IAT_ADDRESS = "0x4cc3c0"


WOL_CSTRING_IAT_RVA = "0xcc3c0"


WOL_CSTRING_IAT_END_RVA = "0xcc3c4"


WOL_CSTRING_MFC42_FIRST_IAT_RVA = 0xCC174


WOL_CSTRING_IMPORT_DESCRIPTOR_INDEX = 2


WOL_CSTRING_IMPORT_SLOT_INDEX = 147


WOL_CSTRING_RETAIL_CALL_SITES = (
    "0x43d90f",
    "0x43d91f",
    "0x43d92f",
)


WOL_CSTRING_FIRST_ORDINAL = 18


WOL_CSTRING_END_ORDINAL = 21


WOL_CSTRING_RELOCATION_OFFSETS = (327, 343, 359)


WOL_CONTROL_CANDIDATE_ORDER = (
    *((WOL_CEDIT_CANDIDATE_SYMBOL,) * 5),
    *((WOL_CBUTTON_CANDIDATE_SYMBOL,) * 6),
    WOL_CLISTBOX_CANDIDATE_SYMBOL,
    WOL_CCOMBOBOX_CANDIDATE_SYMBOL,
    WOL_CLISTBOX_CANDIDATE_SYMBOL,
    WOL_CEDIT_CANDIDATE_SYMBOL,
    WOL_CEDIT_CANDIDATE_SYMBOL,
    WOL_CLISTBOX_CANDIDATE_SYMBOL,
)


WOL_CONTROL_FIRST_ORDINAL = 1


WOL_CONTROL_END_ORDINAL = 18


WOL_CALL_POPULATION = 21


WOL_CEDIT_CANDIDATE_SIZE = 432


WOL_CEDIT_CANDIDATE_RELOCATION_COUNT = 26


WOL_CEDIT_RETAIL_NAME = "??0CWnd@@QAE@XZ"


WOL_CEDIT_RETAIL_FULL_NAME = "CWnd::CWnd(CWnd* this)"


WOL_CEDIT_TARGET_ADDRESS = "0x4c5bac"


WOL_CEDIT_TARGET_END_EXCLUSIVE = "0x4c5bb2"


WOL_CEDIT_TARGET_SYMBOL_ID = "recoil:function:0x4c5bac"


WOL_CEDIT_TARGET_IDENTITY = "provider:recoil:function:0x4c5bac"


WOL_CEDIT_TARGET_BLOCK_ID = "recoil:block:0x4c5a50"


WOL_CEDIT_CALLER_ADDRESS = "0x43d740"


WOL_CEDIT_CALLER_END_EXCLUSIVE = "0x43d980"


WOL_CEDIT_CALLER_TOKEN = "symbol:recoil:function:0x43d740"


WOL_CEDIT_CALLER_INDEX_IDENTITY = WOL_CEDIT_CALLER_TOKEN


WOL_CEDIT_CALLER_SYMBOL_ID = "recoil:function:0x43d740"


WOL_CEDIT_CALLER_SYMBOL = (
    "??0WestwoodOnlineUpgradeDialog@@QAE@PAVCWnd@@@Z"
)


WOL_CEDIT_CALLER_BLOCK_ID = "recoil:block:0x43cf90"


WOL_CEDIT_CALLER_SOURCE_PATH = "src/Battlesport/WOL.cpp"


WOL_PROGRESS_DIALOG_SOURCE_PATH = (
    "src/GameZRecoil/westwoodonline/WolapiProgressDialog.cpp"
)


WOL_CEDIT_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.wol."
    "westwoodonlineupgradedialog-westwoodonlineupgradedialog"
)


WOL_CEDIT_TARGET_ID = (
    "recoil:vc5-target:wol_43cf90_442890_authored_order"
)


WOL_CEDIT_TARGET_NAME = "wol_43cf90_442890_authored_order"


WOL_CEDIT_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "wol_43cf90_442890_authored_order.json"
)


WOL_CATEGORY_A_SOURCE_PATH = WOL_CEDIT_CALLER_SOURCE_PATH


WOL_CATEGORY_A_CALLER_SPECS: Mapping[str, Mapping[str, Any]] = {
    "symbol:recoil:function:0x43fa90": {
        "symbol_id": "recoil:function:0x43fa90",
        "address": "0x43fa90",
        "end_exclusive": "0x43fde0",
        "size": 0x350,
        "physical_block_id": WOL_CEDIT_CALLER_BLOCK_ID,
        "symbol": (
            "?OnApiStatus@WestwoodOnlineUpgradeApiEventSink@@"
            "UAGHHPBD@Z"
        ),
        "target_row_name": (
            "WestwoodOnlineUpgradeApiEventSink::OnApiStatus"
        ),
        "source_anchor": (
            "recoil:anchor:battlesport.wol."
            "westwoodonlineupgradeapieventsink-onapistatus"
        ),
        "verification_target_ids": (
            WOL_CEDIT_TARGET_ID,
        ),
    },
    "symbol:recoil:function:0x4407e0": {
        "symbol_id": "recoil:function:0x4407e0",
        "address": "0x4407e0",
        "end_exclusive": "0x440a30",
        "size": 0x250,
        "physical_block_id": WOL_CEDIT_CALLER_BLOCK_ID,
        "symbol": (
            "?ApplyEncodedQueryString1@"
            "WestwoodOnlineUpgradeApiEventSink@@UAGHHHHPAD@Z"
        ),
        "target_row_name": (
            "WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString1"
        ),
        "source_anchor": (
            "recoil:anchor:battlesport.wol."
            "westwoodonlineupgradeapieventsink-applyencodedquerystring1"
        ),
        "verification_target_ids": (
            WOL_CEDIT_TARGET_ID,
        ),
    },
    "symbol:recoil:function:0x440a30": {
        "symbol_id": "recoil:function:0x440a30",
        "address": "0x440a30",
        "end_exclusive": "0x440c80",
        "size": 0x250,
        "physical_block_id": WOL_CEDIT_CALLER_BLOCK_ID,
        "symbol": (
            "?ApplyEncodedQueryString0@"
            "WestwoodOnlineUpgradeApiEventSink@@UAGHHHPAD@Z"
        ),
        "target_row_name": (
            "WestwoodOnlineUpgradeApiEventSink::ApplyEncodedQueryString0"
        ),
        "source_anchor": (
            "recoil:anchor:battlesport.wol."
            "westwoodonlineupgradeapieventsink-applyencodedquerystring0"
        ),
        "verification_target_ids": (
            WOL_CEDIT_TARGET_ID,
        ),
    },
    "symbol:recoil:function:0x441750": {
        "symbol_id": "recoil:function:0x441750",
        "address": "0x441750",
        "end_exclusive": "0x441890",
        "size": 0x140,
        "physical_block_id": WOL_CEDIT_CALLER_BLOCK_ID,
        "symbol": (
            "??0WestwoodOnlineUpgradeConfigDialog@@QAE@PAVCWnd@@@Z"
        ),
        "target_row_name": (
            "WestwoodOnlineUpgradeConfigDialog::Constructor"
        ),
        "source_anchor": (
            "recoil:anchor:battlesport.wol."
            "westwoodonlineupgradeconfigdialog-"
            "westwoodonlineupgradeconfigdialog"
        ),
        "verification_target_ids": (
            WOL_CEDIT_TARGET_ID,
        ),
    },
}


for _wol_category_a_spec in WOL_CATEGORY_A_CALLER_SPECS.values():
    _wol_category_a_spec.update(
        {
            "target_id": WOL_CEDIT_TARGET_ID,
            "target_name": WOL_CEDIT_TARGET_NAME,
            "target_manifest": WOL_CEDIT_TARGET_MANIFEST,
            "source_path": WOL_CATEGORY_A_SOURCE_PATH,
        }
    )


WOL_TIME_RESET_CANDIDATE_SYMBOL = "?Reset@Time@@YIXXZ"


WOL_TIME_RESET_TARGET_IDENTITY = "symbol:recoil:function:0x4a5670"


WOL_TIME_RESET_TARGET_SYMBOL_ID = "recoil:function:0x4a5670"


WOL_TIME_RESET_TARGET_ADDRESS = "0x4a5670"


WOL_TIME_RESET_TARGET_END_EXCLUSIVE = "0x4a56d0"


WOL_TIME_RESET_TARGET_BLOCK_ID = "recoil:block:0x4a5670"


WOL_TIME_RESET_TARGET_OWNER_ID = "recoil:owner:engine.time_runtime_functions"


WOL_ATOI_CANDIDATE_SYMBOL = "__imp__atoi"


WOL_ATOI_STORAGE_IDENTITY = "iat:atoi"


WOL_ATOI_STORAGE_ADDRESS = "0x4cc5d4"


WOL_ATOI_LOAD_OFFSET = 0x93


WOL_ATOI_RELOCATION_OFFSET = 0x95


WOL_ATOI_CALL_OFFSETS = (0x9C, 0xE6, 0x124, 0x161, 0x19E, 0x1DB)


WOL_ATOI_CALL_ORDINALS = (5, 10, 14, 18, 22, 26)


WOL_CONFIG_CDIALOG_CANDIDATE_SYMBOL = "??0CDialog@@QAE@IPAVCWnd@@@Z"


WOL_CONFIG_CEDIT_CANDIDATE_SYMBOL = WOL_CEDIT_CANDIDATE_SYMBOL


WOL_CONFIG_CEDIT_CALLER_TOKEN = "symbol:recoil:function:0x441750"


WOL_CONFIG_CEDIT_CALL_ORDINAL = 2


WOL_CONFIG_CEDIT_CALL_OFFSET = 0x48


WOL_CONFIG_CEDIT_CANDIDATE_SIZE = 304


WOL_CONFIG_CEDIT_CANDIDATE_RELOCATION_COUNT = 25


WOL_CONFIG_CEDIT_INVOCATION_OFFSETS = (
    0x28,
    0x38,
    0x48,
    0x58,
    0x6A,
    0x89,
    0xA8,
    0xC7,
    0xE6,
    0xFD,
    0x102,
)


WOL_CONFIG_CEDIT_INVOCATION_ORDER = (
    WOL_CONFIG_CDIALOG_CANDIDATE_SYMBOL,
    WOL_CCOMBOBOX_CANDIDATE_SYMBOL,
    WOL_CONFIG_CEDIT_CANDIDATE_SYMBOL,
    WOL_CSTRING_CANDIDATE_SYMBOL,
    WOL_CSTRING_CANDIDATE_SYMBOL,
    "??_L@YGXPAXIHP6EX0@Z1@Z",
    "??_L@YGXPAXIHP6EX0@Z1@Z",
    "??_L@YGXPAXIHP6EX0@Z1@Z",
    "??_L@YGXPAXIHP6EX0@Z1@Z",
    "??4CString@@QAEABV0@PBD@Z",
    "?zOptGetWolPasswordFlagValue@@YIHXZ",
)


WOL_CONFIG_CSTRING_CALL_RELOCATION_OFFSETS = (0x59, 0x6B)


WOL_CONFIG_CSTRING_CALLBACK_RELOCATION_OFFSETS = (0x75, 0x94, 0xB3, 0xD2)


ZCLASS_FREEALL_CALLER_IDENTITY = "symbol:recoil:function:0x451a00"


ZCLASS_FREEALL_CALLER_SYMBOL_ID = "recoil:function:0x451a00"


ZCLASS_FREEALL_CALLER_ADDRESS = "0x451a00"


ZCLASS_FREEALL_CALLER_END_EXCLUSIVE = "0x451a60"


ZCLASS_FREEALL_CALLER_SYMBOL = "?ShutdownCore@zClass@@YAHXZ"


ZCLASS_FREEALL_CALLER_NAME = "zClass::ShutdownCore"


ZCLASS_FREEALL_CALLER_BLOCK_ID = "recoil:block:0x4518b0"


ZCLASS_FREEALL_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zclass.cls-util.shutdowncore"
)


ZCLASS_FREEALL_CALLER_SOURCE_PATH = "src/GameZRecoil/zClass/cls_util.c"


ZCLASS_FREEALL_CALLER_TARGET_ID = (
    "recoil:vc5-target:cls_util_4518b0_452920_authored_order"
)


ZCLASS_FREEALL_CALLER_TARGET_NAME = (
    "cls_util_4518b0_452920_authored_order"
)


ZCLASS_FREEALL_CALLER_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "cls_util_4518b0_452920_authored_order.json"
)


ZCLASS_FREEALL_TARGET_IDENTITY = "symbol:recoil:function:0x44e6d0"


ZCLASS_FREEALL_TARGET_SYMBOL_ID = "recoil:function:0x44e6d0"


ZCLASS_FREEALL_TARGET_ADDRESS = "0x44e6d0"


ZCLASS_FREEALL_TARGET_END_EXCLUSIVE = "0x44e700"


ZCLASS_FREEALL_TARGET_NAME = "zClass_TypeList::FreeAll"


ZCLASS_FREEALL_TARGET_BLOCK_ID = "recoil:block:0x44e630"


ZCLASS_FREEALL_TARGET_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zclass.list.freeall"
)


ZCLASS_FREEALL_TARGET_SOURCE_PATH = "src/GameZRecoil/zClass/List.c"


ZCLASS_FREEALL_TARGET_ID = (
    "recoil:vc5-target:list_44e630_44f7a0_authored_order"
)


ZCLASS_FREEALL_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "list_44e630_44f7a0_authored_order.json"
)


ZCLASS_FREEALL_HEADER_PATH = "src/GameZRecoil/include/zClass.h"


ZCLASS_FREEALL_CANDIDATE_SYMBOL = "?FreeAll@zClass_TypeList@@YIXXZ"


ZCLASS_FREEALL_REGISTERED_SYMBOL = "?FreeAll@zClass_TypeList@@YAXXZ"


ZCLASS_FREEALL_CALL_ORDINAL = 1


ZCLASS_FREEALL_CALL_OFFSET = 0x0B


ZCLASS_FREEALL_RELOCATION_OFFSET = 0x0C


ZCLASS_FREEALL_CALLER_COFF_SECTION_INDEX = 14


ZCLASS_FREEALL_CALLER_BODY = bytes.fromhex(
    "56b906000000e800000000e800000000a10000000033f63bc67e2fa100000000"
    "3bc6741050ff150000000083c404893500000000893500000000893500000000"
    "c70500000000ffffffffe80000000089350000000033c05ec390909090909090"
)


ZCLASS_FREEALL_CALLER_RELOCATIONS = (
    (0x07, IMAGE_REL_I386_REL32, "?DeleteAllOfType@zClass_List@@YIHH@Z"),
    (0x0C, IMAGE_REL_I386_REL32, ZCLASS_FREEALL_CANDIDATE_SYMBOL),
    (0x11, IMAGE_REL_I386_DIR32, "_g_zClass_NodeArraySize"),
    (0x1C, IMAGE_REL_I386_DIR32, "_g_zClass_NodeArray"),
    (0x27, IMAGE_REL_I386_DIR32, "__imp__free"),
    (0x30, IMAGE_REL_I386_DIR32, "_g_zClass_NodeArray"),
    (0x36, IMAGE_REL_I386_DIR32, "_g_zClass_NodeArraySize"),
    (0x3C, IMAGE_REL_I386_DIR32, "_g_zClass_ActiveNodeCount"),
    (0x42, IMAGE_REL_I386_DIR32, "_g_zClass_NodeFreeHeadIndex"),
    (0x4B, IMAGE_REL_I386_REL32, "?ResetCurrentZbdPath@zClass@@YIHXZ"),
    (0x51, IMAGE_REL_I386_DIR32, "_g_zClass_IsInitialized"),
)


ZCLASS_RESET_ZBD_TARGET_IDENTITY = "symbol:recoil:function:0x454360"


ZCLASS_RESET_ZBD_TARGET_SYMBOL_ID = "recoil:function:0x454360"


ZCLASS_RESET_ZBD_TARGET_ADDRESS = "0x454360"


ZCLASS_RESET_ZBD_TARGET_END_EXCLUSIVE = "0x454370"


ZCLASS_RESET_ZBD_TARGET_NAME = "zClass::ResetCurrentZbdPath"


ZCLASS_RESET_ZBD_TARGET_BLOCK_ID = "recoil:block:0x454360"


ZCLASS_RESET_ZBD_TARGET_SOURCE_PATH = "src/GameZRecoil/zClass/cls_zbd.c"


ZCLASS_RESET_ZBD_TARGET_ID = (
    "recoil:vc5-target:cls_zbd_454360_4558f0_authored_order"
)


ZCLASS_RESET_ZBD_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "cls_zbd_454360_4558f0_authored_order.json"
)


ZCLASS_RESET_ZBD_CANDIDATE_SYMBOL = "?ResetCurrentZbdPath@zClass@@YIHXZ"


ZCLASS_RESET_ZBD_REGISTERED_SYMBOL = "?ResetCurrentZbdPath@zClass@@YAHXZ"


ZCLASS_RESET_ZBD_CALL_ORDINAL = 3


ZCLASS_RESET_ZBD_CALL_OFFSET = 0x4A


ZCLASS_RESET_ZBD_RELOCATION_OFFSET = 0x4B


GETTICKCOUNT_RETAIL_THUNK_NAME = "KERNEL32_GetTickCount_ImportThunk"


GETTICKCOUNT_PROVIDER_ADDRESS = "0x4a59d0"


GETTICKCOUNT_PROVIDER_END_EXCLUSIVE = "0x4a59e0"


GETTICKCOUNT_PROVIDER_SYMBOL_ID = "recoil:function:0x4a59d0"


GETTICKCOUNT_PROVIDER_IDENTITY = "provider:recoil:function:0x4a59d0"


GETTICKCOUNT_PROVIDER_OWNER_ID = (
    "recoil:owner:provider.kernel32.gettickcount_import_thunk"
)


GETTICKCOUNT_PROVIDER_BLOCK_ID = "recoil:block:0x4a59d0"


GETTICKCOUNT_PROVIDER_PATH = "provider:kernel32-gettickcount-import-thunk"


GETTICKCOUNT_IMPORT_DLL = "KERNEL32.dll"


GETTICKCOUNT_IMPORT_NAME = "GetTickCount"


GETTICKCOUNT_IAT_ADDRESS = "0x4cc140"


GETTICKCOUNT_IAT_IDENTITY = "iat:GetTickCount"


GETTICKCOUNT_PROVIDER_ORDER_TARGET_NAME = (
    "gettickcount_4a59d0_4a59e0_provider_order"
)


GETTICKCOUNT_PROVIDER_ORDER_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "gettickcount_4a59d0_4a59e0_provider_order.json"
)


GETTICKCOUNT_CANDIDATE_CALLABLE = "_GetTickCount@0"


GETTICKCOUNT_CANDIDATE_IMPORT_SYMBOL = "__imp__GetTickCount@0"


GETTICKCOUNT_CALLER_IDENTITY = "symbol:recoil:function:0x4159e0"


GETTICKCOUNT_CALLER_START = "0x4159e0"


GETTICKCOUNT_CALLER_END_EXCLUSIVE = "0x415a80"


GETTICKCOUNT_CALLER_SYMBOL = "?RunBlockingTimed@zFMV_Action@@QAEXXZ"


GETTICKCOUNT_RETAIL_E8_CALL_BYTES = {
    0x4159EE: bytes.fromhex("e8 dd ff 08 00"),
    0x415A16: bytes.fromhex("e8 b5 ff 08 00"),
    0x415A42: bytes.fromhex("e8 89 ff 08 00"),
}


GETTICKCOUNT_CANDIDATE_LOAD_OFFSET = 0x0C


GETTICKCOUNT_CANDIDATE_RELOCATION_OFFSET = 0x0E


GETTICKCOUNT_CANDIDATE_DIRECT_CALL_OFFSETS = (0x0D, 0x33, 0x67)


GETTICKCOUNT_CANDIDATE_DIRECT_VIRTUAL_CALL_OFFSETS = (0x30, 0x60, 0x94, 0x9F)


GETTICKCOUNT_CANDIDATE_DIRECT_INVOCATION_OFFSETS = (
    0x0D,
    0x30,
    0x33,
    0x60,
    0x67,
    0x94,
    0x9F,
)


GETTICKCOUNT_CANDIDATE_DIRECT_RET_OFFSET = 0xA7


GETTICKCOUNT_CANDIDATE_DIRECT_SECTION_LENGTH = 0xB0


TIME_RESET_GETTICKCOUNT_CALLER_IDENTITY = WOL_TIME_RESET_TARGET_IDENTITY


TIME_RESET_GETTICKCOUNT_CALLER_START = WOL_TIME_RESET_TARGET_ADDRESS


TIME_RESET_GETTICKCOUNT_CALLER_END_EXCLUSIVE = (
    WOL_TIME_RESET_TARGET_END_EXCLUSIVE
)


TIME_RESET_GETTICKCOUNT_CALLER_SYMBOL = "?Reset@Time@@YAXXZ"


TIME_RESET_GETTICKCOUNT_CALL_OFFSET = 0x35


TIME_RESET_GETTICKCOUNT_RELOCATION_OFFSET = 0x36


TIME_RESET_GETTICKCOUNT_SECTION_LENGTH = 0x60


TIME_TICK_GETTICKCOUNT_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4a56d0"
)


TIME_TICK_GETTICKCOUNT_CALLER_START = "0x4a56d0"


TIME_TICK_GETTICKCOUNT_CALLER_END_EXCLUSIVE = "0x4a5780"


TIME_TICK_GETTICKCOUNT_CALLER_SYMBOL = "?Tick@Time@@YAXXZ"


TIME_TICK_GETTICKCOUNT_CALL_OFFSET = 0x03


TIME_TICK_GETTICKCOUNT_RELOCATION_OFFSET = 0x04


TIME_TICK_GETTICKCOUNT_SECTION_LENGTH = 0xB0


CACHED_FREAD_CALLER_START = "0x415bd0"


CACHED_FREAD_CALLER_END_EXCLUSIVE = "0x415c90"


CACHED_FREAD_CALLER_IDENTITY = "symbol:recoil:function:0x415bd0"


CACHED_FREAD_CALLER_NAME = "HudSensorMapNode::LoadFromStream"


CACHED_FREAD_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.map.hudsensormapnode-loadfromstream"
)


CACHED_FREAD_CALLER_SOURCE_PATH = "src/Battlesport/map.cpp"


CACHED_FREAD_CALLER_PHYSICAL_BLOCK_ID = "recoil:block:0x415ab0"


CACHED_FREAD_CALLER_TARGET_IDS = (
    "recoil:vc5-target:map_text_block_order_current_shape",
)


CACHED_FREAD_IAT_ADDRESS = "0x4cc4e0"


CACHED_FREAD_IMPORT_DLL = "MSVCRT.dll"


CACHED_FREAD_IMPORT_NAME = "fread"


CACHED_FREAD_CANDIDATE_SYMBOL = "__imp__fread"


CACHED_FREAD_IAT_IDENTITY = "iat:fread"


CACHED_FREAD_LOAD_ADDRESS = "0x415be7"


CACHED_FREAD_CALL_ADDRESSES = (
    "0x415bf6",
    "0x415c12",
    "0x415c41",
    "0x415c5e",
)


HUD_TIMER_FLOOR_CALLER_IDENTITY = "symbol:recoil:function:0x40ee60"


HUD_TIMER_FLOOR_CALLER_START = "0x40ee60"


HUD_TIMER_FLOOR_CALLER_END_EXCLUSIVE = "0x40ef00"


HUD_TIMER_FLOOR_CALLER_SYMBOL_ID = "recoil:function:0x40ee60"


HUD_TIMER_FLOOR_CALLER_NAME = (
    "HudUiTimerPanel::UpdateHMSFromSeconds"
)


HUD_TIMER_FLOOR_PHYSICAL_BLOCK_ID = "recoil:block:0x404ca0"


HUD_TIMER_FLOOR_TARGET_ID = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
)


HUD_TIMER_FLOOR_IMPORT_DLL = "MSVCRT.dll"


HUD_TIMER_FLOOR_IMPORT_NAME = "floor"


HUD_TIMER_FLOOR_IAT_ADDRESS = "0x4cc504"


HUD_TIMER_FLOOR_IAT_IDENTITY = "iat:floor"


HUD_TIMER_FLOOR_CANDIDATE_IMPORT_SYMBOL = "__imp__floor"


HUD_TIMER_FLOOR_PROVIDER_ADDRESS = "0x7c8e34"


HUD_TIMER_FLOOR_IAT_TYPE = "double (* const)(double _X)"


HUD_TIMER_FLOOR_PROVIDER_TYPE = "double(double _X)"


HUD_TIMER_FLOOR_RETAIL_CALL_ADDRESSES = (
    "0x40ee80",
    "0x40eeb2",
    "0x40eeda",
)


HUD_TIMER_PANEL_FLOAT_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40ef60"
)


HUD_TIMER_PANEL_FLOAT_CALLER_START = "0x40ef60"


HUD_TIMER_PANEL_FLOAT_CALLER_END_EXCLUSIVE = "0x40f040"


HUD_TIMER_PANEL_FLOAT_CALLER_SYMBOL = (
    "??0HudUiTimerPanelFloat@@QAE@XZ"
)


HUD_TIMER_PANEL_FLOAT_VFTABLE_SYMBOL = (
    "??_7HudUiTimerPanelFloat@@6B@"
)


HUD_TIMER_PANEL_FLOAT_DATA_ID = "recoil:data:0x4ce7d8"


HUD_TIMER_PANEL_FLOAT_STORAGE_ID = "recoil:storage:va:0x4ce7d8"


HUD_TIMER_PANEL_FLOAT_STORAGE_IDENTITY = (
    "storage:recoil:data:0x4ce7d8"
)


HUD_TIMER_PANEL_FLOAT_STORAGE_ADDRESS = "0x4ce7d8"


HUD_TIMER_PANEL_FLOAT_STORAGE_END_EXCLUSIVE = "0x4ce86c"


HUD_TIMER_PANEL_FLOAT_OWNER_ID = (
    "recoil:owner:hud_ui.hud_ui_timer_panel_float_class"
)


HUD_TIMER_PANEL_FLOAT_TABLE_SIZE = 0x94


HUD_TIMER_PANEL_FLOAT_SLOT_DISPLACEMENT = 0x60


HUD_TIMER_PANEL_FLOAT_SLOT_ADDRESS = "0x4ce838"


HUD_TIMER_PANEL_FLOAT_SLOT_SYMBOL = (
    "?SetVisible@HudUiElement@@UAEXH@Z"
)


HUD_TIMER_PANEL_FLOAT_SLOT_TARGET_ADDRESS = "0x404d20"


HUD_TIMER_PANEL_FLOAT_RETAIL_ORDINAL = 2


HUD_TIMER_PANEL_FLOAT_RETAIL_STORE_ADDRESS = "0x40f005"


HUD_TIMER_PANEL_FLOAT_RETAIL_STORE_BODY = (
    b"\xc7\x06\xd8\xe7\x4c\x00"
)


HUD_TIMER_PANEL_FLOAT_RETAIL_CALL_ADDRESS = "0x40f020"


HUD_TIMER_PANEL_FLOAT_RETAIL_CALL_BODY = (
    b"\xff\x15\x38\xe8\x4c\x00"
)


HUD_TIMER_PANEL_FLOAT_VPTR_STORE_OFFSET = 0x4B


HUD_TIMER_PANEL_FLOAT_VPTR_RELOCATION_OFFSET = 0x4D


HUD_LAYOUT_HW_VPTR_CALLER_IDENTITY = "symbol:recoil:function:0x40f2e0"


HUD_LAYOUT_HW_VPTR_CALLER_START = "0x40f2e0"


HUD_LAYOUT_HW_VPTR_CALLER_END_EXCLUSIVE = "0x40f3e0"


HUD_LAYOUT_HW_VPTR_CALLER_SYMBOL = (
    "?InitLayout@HudUiNanitePanel@@QAEXPAUNode@zReader@@@Z"
)


HUD_LAYOUT_HW_AGGREGATE_SYMBOL = (
    "?g_HudLayoutHW@@3UHudLayoutHW@@A"
)


HUD_LAYOUT_HW_DATA_ID = "recoil:data:0x4ed718"


HUD_LAYOUT_HW_STORAGE_ID = "recoil:storage:va:0x4ed718"


HUD_LAYOUT_HW_STORAGE_IDENTITY = "storage:recoil:data:0x4ed718"


HUD_LAYOUT_HW_ADDRESS = "0x4ed718"


HUD_LAYOUT_HW_OWNER_ID = "recoil:owner:hud_ui.hud_layout_classes"


HUD_LAYOUT_HW_DISPLACEMENT = 0x1B4


HUD_LAYOUT_HW_ACCESS_WIDTH = 4


HUD_LAYOUT_HW_LOAD_OFFSET = 0x13


HUD_LAYOUT_HW_RELOCATION_OFFSET = 0x14


HUD_LAYOUT_HW_CALL_OFFSET = 0x1F


HUD_LAYOUT_HW_SLOT_DISPLACEMENT = 0x64


HUD_LAYOUT_HW_EDX_LOAD_OFFSET = 0x22


HUD_LAYOUT_HW_EDX_RELOCATION_OFFSET = 0x24


HUD_LAYOUT_HW_EDX_CALL_OFFSET = 0x31


HUD_LAYOUT_HW_EDX_SLOT_DISPLACEMENT = 0x68


HUD_LAYOUT_MEMBER_RECEIVER_OFFSET = 0x1B4


HUD_LAYOUT_MEMBER_THIS_MOVE_OFFSET = 0x0F


HUD_LAYOUT_MEMBER_LEA_OFFSET = 0x90


HUD_LAYOUT_MEMBER_DIRECT_CALL_OFFSET = 0x9F


HUD_LAYOUT_MEMBER_DIRECT_CALL_RELOCATION_OFFSET = 0xA0


HUD_LAYOUT_MEMBER_VPTR_LOAD_OFFSET = 0xA4


HUD_LAYOUT_MEMBER_RECEIVER_MOVE_OFFSET = 0xA6


HUD_LAYOUT_MEMBER_VPTR_CALL_OFFSET = 0xA8


HUD_LAYOUT_MEMBER_VPTR_SLOT_DISPLACEMENT = 0x68


HUD_LAYOUT_MEMBER_EXPECTED_ORDINAL = 6


HUD_LAYOUT_MEMBER_APPLY_IMAGE_SYMBOL = (
    "?ApplyImageWidget@HudUiLayoutNode@@YIPAUzVidImagePartial@@"
    "PAUNode@zReader@@PAUHudUiWidget@@HHPBHPAU2@PAUHudUiRect@@@Z"
)


HUD_UI_MGR_INIT_LAYOUTS_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40f4c0"
)


HUD_UI_MGR_INIT_LAYOUTS_CALLER_START = "0x40f4c0"


HUD_UI_MGR_INIT_LAYOUTS_CALLER_END_EXCLUSIVE = "0x40f9e0"


HUD_UI_MGR_INIT_LAYOUTS_CALLER_SYMBOL = (
    "?InitHudLayouts@HudUiMgr@@YIHPBUHudUiRect@@0@Z"
)


HUD_UI_MGR_INIT_LAYOUTS_TARGET_NAME = (
    "hud_404ca0_415ab0_authored_order"
)


HUD_UI_MGR_INIT_LAYOUTS_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "hud_404ca0_415ab0_authored_order.json"
)


HUD_UI_MGR_INIT_LAYOUTS_SOURCE_PATH = "src/Battlesport/hud.cpp"


HUD_UI_MGR_INIT_LAYOUTS_TARGET_SYMBOL_REGEX = (
    r"\?InitHudLayouts@HudUiMgr@@.*"
)


HUD_UI_MGR_PANEL_SIMPLE_DESTRUCTOR_SYMBOL = (
    "??1HudUiPanelSimple@@UAE@XZ"
)


HUD_UI_MGR_PANEL_SIMPLE_DEFAULT_CONSTRUCTOR_CLOSURE_SYMBOL = (
    "??_FHudUiPanelSimple@@QAEXXZ"
)


HUD_UI_MGR_PANEL_SIMPLE_CONSTRUCTOR_PREFIX = "??0HudUiPanelSimple@@"


HUD_UI_MGR_LAYOUT_ARRAY_EH_CONSTRUCTOR_SYMBOL = (
    "??_L@YGXPAXIHP6EX0@Z1@Z"
)


HUD_UI_MGR_LAYOUT_ARRAY_COUNT = 0x17


HUD_UI_MGR_LAYOUT_ARRAY_FIRST_OFFSET = 0x20


HUD_UI_MGR_LAYOUT_ARRAY_STRIDE = 0x2A4


HUD_UI_MGR_LAYOUT_ARRAY_STORAGE_IDENTITY = (
    "load(address(hud-ui-panel-simple-loop-element))"
)


HUD_BRIEFING_LOCATOR_PANEL_UPDATE_SYMBOL = (
    "?Update@HudUiBriefingLocatorPanel@@UAEXM@Z"
)


HUD_TRIPLET_SORT_KEY_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40d220"
)


HUD_TRIPLET_SORT_KEY_CALLER_START = "0x40d220"


HUD_TRIPLET_SORT_KEY_CALLER_END_EXCLUSIVE = "0x40d260"


HUD_TRIPLET_SORT_KEY_CALLER_SYMBOL = (
    "?CompareSortKey@HudUiListMenuEntry@@"
    "YIHPBUHudUiScoreboardEntry@@0@Z"
)


HUD_TRIPLET_SORT_KEY_TU_LOCAL_COD_RE = re.compile(
    r"^\?HudUiTripletEntrySortKey@\?_tu_order\\00_hud\.cpp"
    r"(?P<discriminator>[0-9]+)"
    r"@@YIHABUHudUiScoreboardEntry@@@Z$"
)


HUD_TRIPLET_SORT_KEY_CALL_OFFSETS = (0x07, 0x10)


HUD_TRIPLET_ENSURE_CAPACITY_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40e590"
)


HUD_TRIPLET_ENSURE_CAPACITY_CALLER_START = "0x40e590"


HUD_TRIPLET_ENSURE_CAPACITY_CALLER_END_EXCLUSIVE = "0x40e800"


HUD_TRIPLET_ENSURE_CAPACITY_CALLER_SYMBOL = (
    "?AddEntry@HudUiTriplet@@QAEXPAUGameNetPlayerRow@@@Z"
)


HUD_TRIPLET_ENSURE_CAPACITY_TU_LOCAL_COD_RE = re.compile(
    r"^\?HudUiTripletEnsureCapacity@\?_tu_order\\00_hud\.cpp"
    r"(?P<discriminator>[0-9]+)"
    r"@@YIXAAUHudUiTripletEntries@@I@Z$"
)


HUD_TRIPLET_ENSURE_CAPACITY_CALL_OFFSET = 0x60


HUD_UI_MGR_ZRD_PAYLOAD_TU_LOCAL_COD_RE = re.compile(
    r"^\?HudUiZrdPayload@\?_tu_order\\00_hud\.cpp"
    r"(?P<discriminator>[0-9]+)"
    r"@@YIPAUNode@zReader@@PAU23@@Z$"
)


HUD_UI_MGR_ZRD_PAYLOAD_TU_LOCAL_COFF_RE = re.compile(
    r"^\?HudUiZrdPayload@\?%_tu_order\\00_hud\.cpp"
    r"(?P<discriminator>[0-9]+)"
    r"@@YIPAUNode@zReader@@PAU23@@Z$"
)


HUD_UI_MGR_ZRD_PAYLOAD_CALL_COUNT = 7


HUD_UI_MGR_ZRD_PAYLOAD_HELPER_BODY = bytes.fromhex(
    "85 c9 74 09 83 39 04 75 04 8b 41 04 c3 33 c0 c3"
)


HUD_UI_MGR_ENSURE_UNSUPPORTED_HELPER_SPECS = (
    (
        "HudUiSetFontFromRect",
        re.compile(
            r"^\?HudUiSetFontFromRect@\?_tu_order\\00_hud\.cpp"
            r"(?P<discriminator>[0-9]+)"
            r"@@YIXPAUHudUiPanel@@ABUHudUiRect@@@Z$"
        ),
        re.compile(
            r"^\?HudUiSetFontFromRect@\?%_tu_order\\00_hud\.cpp"
            r"(?P<discriminator>[0-9]+)"
            r"@@YIXPAUHudUiPanel@@ABUHudUiRect@@@Z$"
        ),
        2,
    ),
    (
        "HudUiSetPanelClipWithSource",
        re.compile(
            r"^\?HudUiSetPanelClipWithSource@\?_tu_order\\00_hud\.cpp"
            r"(?P<discriminator>[0-9]+)"
            r"@@YIXPAUHudUiPanel@@PAXPBUHudUiRect@@@Z$"
        ),
        re.compile(
            r"^\?HudUiSetPanelClipWithSource@\?%_tu_order\\00_hud\.cpp"
            r"(?P<discriminator>[0-9]+)"
            r"@@YIXPAUHudUiPanel@@PAXPBUHudUiRect@@@Z$"
        ),
        0,
    ),
    (
        "HudUiApplyStatsTripletInt3",
        re.compile(
            r"^\?HudUiApplyStatsTripletInt3@\?_tu_order\\00_hud\.cpp"
            r"(?P<discriminator>[0-9]+)"
            r"@@YIXPAUNode@zReader@@HAAH1PAH@Z$"
        ),
        re.compile(
            r"^\?HudUiApplyStatsTripletInt3@\?%_tu_order\\00_hud\.cpp"
            r"(?P<discriminator>[0-9]+)"
            r"@@YIXPAUNode@zReader@@HAAH1PAH@Z$"
        ),
        6,
    ),
    (
        "HudUiZrdStringAt",
        re.compile(
            r"^\?HudUiZrdStringAt@\?_tu_order\\00_hud\.cpp"
            r"(?P<discriminator>[0-9]+)"
            r"@@YIPBDPAUNode@zReader@@H@Z$"
        ),
        re.compile(
            r"^\?HudUiZrdStringAt@\?%_tu_order\\00_hud\.cpp"
            r"(?P<discriminator>[0-9]+)"
            r"@@YIPBDPAUNode@zReader@@H@Z$"
        ),
        4,
    ),
)


HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40eae0"
)


HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_START = "0x40eae0"


HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_END_EXCLUSIVE = "0x40eb00"


HUD_SCOREBOARD_DISPATCH_SET_SCALE_CALLER_SYMBOL = (
    "?DispatchSetScale@HudScoreboard@@YGXM@Z"
)


HUD_UI_MGR_AGGREGATE_SYMBOL_ID = "recoil:data:0x4e5ed0"


HUD_UI_MGR_AGGREGATE_STORAGE_ID = "recoil:storage:va:0x4e5ed0"


HUD_UI_MGR_AGGREGATE_ADDRESS = "0x4e5ed0"


HUD_UI_MGR_AGGREGATE_NAME = "g_HudUiMgr"


HUD_UI_MGR_AGGREGATE_DECORATED_SYMBOL = (
    "?g_HudUiMgr@@3THudUiMgrDataStorage@@A"
)


HUD_UI_MGR_AGGREGATE_SIZE = 0x7844


HUD_UI_MGR_AGGREGATE_TARGET_ID = "recoil:vc5-target:hud_ui_mgr_data"


HUD_UI_MGR_AGGREGATE_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/hud_ui_mgr_data.json"
)


HUD_UI_MGR_SHUTDOWN_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40fbd0"
)


HUD_UI_MGR_SHUTDOWN_CALLER_START = "0x40fbd0"


HUD_UI_MGR_SHUTDOWN_CALLER_END_EXCLUSIVE = "0x40fdd0"


HUD_UI_MGR_SHUTDOWN_CALLER_SYMBOL = (
    "?ShutdownResources@HudUiMgr@@YIXXZ"
)


HUD_UI_MGR_VIEWPORT_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40ff80"
)


HUD_UI_MGR_VIEWPORT_CALLER_START = "0x40ff80"


HUD_UI_MGR_VIEWPORT_CALLER_END_EXCLUSIVE = "0x410140"


HUD_UI_MGR_VIEWPORT_CALLER_SYMBOL = (
    "?OnViewportChanged@HudUiMgr@@YIXPBUHudUiRect@@0@Z"
)


HUD_UI_MGR_VIEWPORT_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.huduimgr-onviewportchanged"
)


HUD_UI_MGR_VIEWPORT_LAYOUT_DISPLACEMENT = 0x18


HUD_UI_MGR_VIEWPORT_LAYOUT_LOAD_OFFSET = 0x12E


HUD_UI_MGR_VIEWPORT_LAYOUT_RELOCATION_OFFSET = 0x130


HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_OFFSET = 0x13A


HUD_UI_MGR_VIEWPORT_LAYOUT_CALL_ORDINAL = 1


HUD_UI_MGR_VIEWPORT_LAYOUT_SLOT_DISPLACEMENT = 0x18


HUD_UI_MGR_VIEWPORT_LAYOUT_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x18))"
)


HUD_UI_MGR_ENABLE_CALLER_IDENTITY = "symbol:recoil:function:0x410e90"


HUD_UI_MGR_ENABLE_CALLER_START = "0x410e90"


HUD_UI_MGR_ENABLE_CALLER_END_EXCLUSIVE = "0x410ed0"


HUD_UI_MGR_ENABLE_CALLER_SYMBOL = "?EnableHud@HudUiMgr@@YIHXZ"


HUD_UI_MGR_ENABLE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.enablehud"
)


HUD_UI_MGR_ENABLE_LOAD_OFFSET = 0x00


HUD_UI_MGR_ENABLE_CALL_OFFSET = 0x13


HUD_UI_MGR_ENABLE_CALL_ORDINAL = 0


HUD_UI_MGR_ENABLE_SLOT_DISPLACEMENT = 0x04


HUD_UI_MGR_ENABLE_STORAGE_IDENTITY = "storage:recoil:data:0x4e5ed0"


HUD_UI_MGR_ENABLE_LOAD_PROVENANCE = (
    "load(storage:recoil:data:0x4e5ed0)"
)


HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_DISPLACEMENT = 0x18


HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_REFERENCE_OFFSET = 0x16


HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_RELOCATION_OFFSET = 0x18


HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_VPTR_LOAD_OFFSET = 0x1C


HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_CALL_OFFSET = 0x1E


HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_CALL_ORDINAL = 1


HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_SLOT_DISPLACEMENT = 0x10


HUD_UI_MGR_ENABLE_CURRENT_LAYOUT_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x18))"
)


HUD_UI_MGR_DISABLE_CALLER_IDENTITY = "symbol:recoil:function:0x410ed0"


HUD_UI_MGR_DISABLE_CALLER_START = "0x410ed0"


HUD_UI_MGR_DISABLE_CALLER_END_EXCLUSIVE = "0x410fe0"


HUD_UI_MGR_DISABLE_CALLER_SYMBOL = "?DisableHud@HudUiMgr@@YIHXZ"


HUD_UI_MGR_DISABLE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.disablehud"
)


HUD_UI_MGR_DISABLE_SLOT_BASE_DISPLACEMENT = 0xF44


HUD_UI_MGR_DISABLE_SLOT_BASE_OFFSET = 0x0D


HUD_UI_MGR_DISABLE_SLOT_BASE_RELOCATION_OFFSET = 0x0E


HUD_UI_MGR_DISABLE_TRACK_MARKER_DISPLACEMENT = 0xBC


HUD_UI_MGR_DISABLE_TRACK_MARKER_LOAD_OFFSET = 0x12


HUD_UI_MGR_DISABLE_TRACK_MARKER_RECEIVER_OFFSET = 0x18


HUD_UI_MGR_DISABLE_TRACK_MARKER_ARGUMENT_OFFSET = 0x1E


HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_OFFSET = 0x20


HUD_UI_MGR_DISABLE_TRACK_MARKER_CALL_ORDINAL = 1


HUD_UI_MGR_DISABLE_TRACK_MARKER_SLOT_DISPLACEMENT = 0x60


HUD_UI_MGR_DISABLE_TRACK_MARKER_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0xf44)+0xbc)"
)


HUD_UI_MGR_DISABLE_SLOT_BASE_VPTR_LOAD_OFFSET = 0x23


HUD_UI_MGR_DISABLE_SLOT_BASE_RECEIVER_OFFSET = 0x25


HUD_UI_MGR_DISABLE_SLOT_BASE_ARGUMENT_OFFSET = 0x27


HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_OFFSET = 0x29


HUD_UI_MGR_DISABLE_SLOT_BASE_CALL_ORDINAL = 2


HUD_UI_MGR_DISABLE_SLOT_BASE_SLOT_DISPLACEMENT = 0x60


HUD_UI_MGR_DISABLE_SLOT_BASE_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0xf44))"
)


HUD_UI_MGR_DISABLE_SET_ENABLED_LOAD_OFFSET = 0x3A


HUD_UI_MGR_DISABLE_SET_ENABLED_ARGUMENT_OFFSET = 0x3F


HUD_UI_MGR_DISABLE_SET_ENABLED_RECEIVER_OFFSET = 0x41


HUD_UI_MGR_DISABLE_SET_ENABLED_FIRST_STORE_OFFSET = 0x46


HUD_UI_MGR_DISABLE_SET_ENABLED_SECOND_STORE_OFFSET = 0x50


HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_OFFSET = 0x5A


HUD_UI_MGR_DISABLE_SET_ENABLED_CALL_ORDINAL = 3


HUD_UI_MGR_DISABLE_SET_ENABLED_SLOT_DISPLACEMENT = 0x04


HUD_UI_MGR_DISABLE_SET_ENABLED_STORAGE_IDENTITY = (
    "storage:recoil:data:0x4e5ed0"
)


HUD_UI_MGR_DISABLE_SET_ENABLED_LOAD_PROVENANCE = (
    "load(storage:recoil:data:0x4e5ed0)"
)


HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_DISPLACEMENT = 0x18


HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_REFERENCE_OFFSET = 0x5D


HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_RELOCATION_OFFSET = 0x5F


HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_VPTR_LOAD_OFFSET = 0x63


HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_OFFSET = 0x65


HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_CALL_ORDINAL = 4


HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_SLOT_DISPLACEMENT = 0x14


HUD_UI_MGR_DISABLE_CURRENT_LAYOUT_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x18))"
)


HUD_UI_MGR_DISABLE_VISIBILITY_CLUSTER_START = 0x68


HUD_UI_MGR_DISABLE_VISIBILITY_CLUSTER_END_EXCLUSIVE = 0x105


HUD_UI_MGR_DISABLE_VISIBILITY_SLOT_DISPLACEMENT = 0x60


HUD_UI_MGR_DISABLE_TIMER_PANEL_SYMBOL_ID = "recoil:data:0x4ea654"


HUD_UI_MGR_DISABLE_TIMER_PANEL_STORAGE_ID = "recoil:storage:va:0x4ea654"


HUD_UI_MGR_DISABLE_TIMER_PANEL_ADDRESS = "0x4ea654"


HUD_UI_MGR_DISABLE_TIMER_PANEL_TARGET_ID = (
    "recoil:vc5-target:hud_ui_timer_panel_global_accessors_data"
)


HUD_UI_MGR_DISABLE_ALT_CLIP_SYMBOL = "_gAltClipPassEnabled"


HUD_UI_MGR_DISABLE_DIRECT_CALLS = (
    (
        0xD6,
        12,
        "?GetAccelerationOption@zVid@@YIHXZ",
        "0x408310",
        "symbol:recoil:function:0x408310",
    ),
    (
        0xDF,
        13,
        "?SpanOcclusionResetFrame@zRndr@@YAXXZ",
        "0x490600",
        "symbol:recoil:function:0x490600",
    ),
    (
        0xE4,
        14,
        "?GetHudTypeForCurrentHwMode@zOpt@@YIHXZ",
        "0x408360",
        "symbol:recoil:function:0x408360",
    ),
)


HUD_UI_MGR_UPDATE_FRAME_CALLER_IDENTITY = (
    "symbol:recoil:function:0x410fe0"
)


HUD_UI_MGR_UPDATE_FRAME_CALLER_START = "0x410fe0"


HUD_UI_MGR_UPDATE_FRAME_CALLER_END_EXCLUSIVE = "0x411170"


HUD_UI_MGR_UPDATE_FRAME_CALLER_SYMBOL = (
    "?UpdateFrame@HudUiMgr@@YIXXZ"
)


HUD_UI_MGR_UPDATE_FRAME_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.updateframe"
)


HUD_UI_MGR_UPDATE_FRAME_LAYOUT_DISPLACEMENT = 0x18


HUD_UI_MGR_UPDATE_FRAME_LAYOUT_REFERENCE_OFFSET = 0x00


HUD_UI_MGR_UPDATE_FRAME_LAYOUT_RELOCATION_OFFSET = 0x02


HUD_UI_MGR_UPDATE_FRAME_LAYOUT_VPTR_OFFSET = 0x08


HUD_UI_MGR_UPDATE_FRAME_LAYOUT_CALL_OFFSET = 0x0A


HUD_UI_MGR_UPDATE_FRAME_LAYOUT_CALL_ORDINAL = 0


HUD_UI_MGR_UPDATE_FRAME_LAYOUT_SLOT_DISPLACEMENT = 0x0C


HUD_UI_MGR_UPDATE_FRAME_LAYOUT_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x18))"
)


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_IDENTITY = (
    "symbol:recoil:function:0x413630"
)


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_START = "0x413630"


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_END_EXCLUSIVE = "0x413640"


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_SYMBOL = (
    "?TriggerCurrentLayoutOnActivated@HudUiMgr@@YAXXZ"
)


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.triggercurrentlayoutonactivated"
)


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
    "recoil:vc5-target:"
    "hud_ui_mgr_trigger_current_layout_on_activated",
)


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_DISPLACEMENT = 0x18


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_RELOCATION_OFFSET = 0x02


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_VPTR_OFFSET = 0x0A


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALL_OFFSET = 0x0C


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_CALL_ORDINAL = 0


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_SLOT_DISPLACEMENT = 0x18


HUD_UI_MGR_TRIGGER_CURRENT_LAYOUT_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x18))"
)


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_IDENTITY = (
    "symbol:recoil:function:0x413660"
)


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_START = "0x413660"


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_END_EXCLUSIVE = "0x4136b0"


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_SYMBOL = (
    "?SwitchActiveDialog@HudUiMgr@@YIXPAUHudLayoutBase@@@Z"
)


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.switchactivedialog"
)


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
)


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_DISPLACEMENT = 0x18


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_RELOCATION_OFFSET = 0x21


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_VPTR_OFFSET = 0x29


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_ARGUMENT_OFFSET = 0x2B


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALL_OFFSET = 0x2D


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_CALL_ORDINAL = 1


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_SLOT_DISPLACEMENT = 0x08


HUD_UI_MGR_SWITCH_ACTIVE_DIALOG_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x18))"
)


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x413770"
)


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_START = "0x413770"


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_END_EXCLUSIVE = "0x4137a0"


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_SYMBOL = (
    "?SetFloatTimerVisible@HudUiMgr@@YIXH@Z"
)


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.setfloattimervisible"
)


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
)


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_DISPLACEMENT = 0x4788


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_RELOCATION_OFFSET = 0x04


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_TRUE_VPTR_OFFSET = 0x0A


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_TRUE_CALL_OFFSET = 0x0E


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_FALSE_VPTR_OFFSET = 0x12


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_FALSE_CALL_OFFSET = 0x16


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_TAIL_DIRECT_CALL_OFFSET = 0x19


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_RELOCATION_OFFSET = 0x05


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_TRUE_VPTR_OFFSET = 0x0D


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_TRUE_CALL_OFFSET = 0x11


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_FALSE_VPTR_OFFSET = 0x16


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_FALSE_CALL_OFFSET = 0x1A


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_CALL_DIRECT_CALL_OFFSET = 0x21


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_SLOT_DISPLACEMENT = 0x60


HUD_UI_MGR_SET_FLOAT_TIMER_VISIBLE_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x4788))"
)


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4137a0"
)


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_START = "0x4137a0"


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_END_EXCLUSIVE = "0x4137c0"


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_SYMBOL = (
    "?SetAuxOverlayVisible@HudUiMgr@@YIXH@Z"
)


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.setauxoverlayvisible"
)


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
)


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_DISPLACEMENT = 0x0EE0


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_RETAIL_STORAGE_ADDRESS = "0x4e6db0"


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_RELOCATION_OFFSET = 0x04


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_TRUE_VPTR_OFFSET = 0x0A


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_TRUE_CALL_OFFSET = 0x0E


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_FALSE_VPTR_OFFSET = 0x12


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_FALSE_CALL_OFFSET = 0x16


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_SLOT_DISPLACEMENT = 0x04


HUD_UI_MGR_SET_AUX_OVERLAY_VISIBLE_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0xee0))"
)


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4137f0"
)


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_START = "0x4137f0"


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_END_EXCLUSIVE = "0x4138d0"


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_SYMBOL = (
    "?UpdateTextLine@HudUiAuxOverlay@@YIXHHPBD@Z"
)


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.updatetextline"
)


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
)


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_DISPLACEMENT = 0x0EE0


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RETAIL_STORAGE_ADDRESS = "0x4e6db0"


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_ITEM_OFFSET = 0x20


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_ITEM_STRIDE = 0x2A4


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RELOCATION_OFFSET = 0x12


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_STORAGE_IDENTITY = (
    "load(indexed(load(storage:recoil:data:0x4e5ed0+0xee0)"
    "+0x20,stride=0x2a4))"
)


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_RETAIL_CALLS = (
    (0x41381A, "edx", 0x74, 8),
    (0x413830, "eax", 0x60, None),
    (0x413857, "edx", 0x60, None),
    (0x41388A, "edx", 0x74, 8),
    (0x41389F, "edx", 0x60, None),
    (0x4138C1, "eax", 0x60, None),
)


HUD_UI_AUX_OVERLAY_APPLY_TEXT_LINE_CANDIDATE_CALLS = (
    (0x24, "ecx", 0x74, 8),
    (0x30, "eax", 0x60, None),
    (0x40, "edx", 0x60, None),
    (0x59, "ecx", 0x74, 8),
    (0x65, "edx", 0x60, None),
    (0x72, "eax", 0x60, None),
)


HUD_UI_MGR_ENABLE_STACKS_CALLER_IDENTITY = (
    "symbol:recoil:function:0x413910"
)


HUD_UI_MGR_ENABLE_STACKS_CALLER_START = "0x413910"


HUD_UI_MGR_ENABLE_STACKS_CALLER_END_EXCLUSIVE = "0x413950"


HUD_UI_MGR_ENABLE_STACKS_CALLER_SYMBOL = (
    "?EnableTopAndChatStacks@HudUiMgr@@YIXXZ"
)


HUD_UI_MGR_ENABLE_STACKS_CALLER_SYMBOL_REGEX = (
    r"^(?:\?EnableTopAndChatStacks@HudUiMgr@@YAXXZ|"
    r"\?EnableTopAndChatStacks@HudUiMgr@@YIXXZ)$"
)


HUD_UI_MGR_ENABLE_STACKS_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.enabletopandchatstacks"
)


HUD_UI_MGR_ENABLE_STACKS_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
    "recoil:vc5-target:hud_ui_text_stack_show_enable",
)


HUD_UI_MGR_ENABLE_STACKS_CLEAR_SYMBOL = (
    "?Clear@HudUiTextStack4@@QAEXXZ"
)


HUD_UI_MGR_ENABLE_STACKS_CLEAR_IDENTITY = (
    "symbol:recoil:function:0x4bd2a0"
)


HUD_UI_MGR_ENABLE_STACKS_CLEAR_ADDRESS = "0x4bd2a0"


HUD_UI_MGR_ENABLE_STACKS_RETAIL_VIRTUAL_CALLS = (
    (0x413925, "eax", "recoil:data:0x56bd24"),
    (0x41393D, "edx", "recoil:data:0x56bd20"),
)


HUD_UI_MGR_ENABLE_STACKS_CANDIDATE_VIRTUAL_CALLS = (
    (0x15, "eax", "recoil:data:0x56bd24"),
    (0x2D, "edx", "recoil:data:0x56bd20"),
)


HUD_UI_MGR_DISABLE_STACKS_CALLER_IDENTITY = (
    "symbol:recoil:function:0x413950"
)


HUD_UI_MGR_DISABLE_STACKS_CALLER_START = "0x413950"


HUD_UI_MGR_DISABLE_STACKS_CALLER_END_EXCLUSIVE = "0x413990"


HUD_UI_MGR_DISABLE_STACKS_CALLER_SYMBOL = (
    "?DisableTopAndChatStacks@HudUiMgr@@YIXXZ"
)


HUD_UI_MGR_DISABLE_STACKS_CALLER_SYMBOL_REGEX = (
    r"^(?:\?DisableTopAndChatStacks@HudUiMgr@@YAXXZ|"
    r"\?DisableTopAndChatStacks@HudUiMgr@@YIXXZ)$"
)


HUD_UI_MGR_DISABLE_STACKS_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.disabletopandchatstacks"
)


HUD_UI_MGR_DISABLE_STACKS_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
)


HUD_UI_MGR_DISABLE_STACKS_RETAIL_VIRTUAL_CALLS = (
    (0x413965, "eax", "recoil:data:0x56bd24"),
    (0x41397D, "edx", "recoil:data:0x56bd20"),
)


HUD_UI_MGR_DISABLE_STACKS_CANDIDATE_VIRTUAL_CALLS = (
    (0x15, "eax", "recoil:data:0x56bd24"),
    (0x2D, "edx", "recoil:data:0x56bd20"),
)


HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_IDENTITY = (
    "symbol:recoil:function:0x413990"
)


HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_START = "0x413990"


HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_END_EXCLUSIVE = "0x413a10"


HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_SYMBOL = (
    "?ApplyTextLabel@HudUiLayoutNode@@YIHPAUNode@zReader@@"
    "PAUHudUiPanel@@HHPBH@Z"
)


HUD_LAYOUT_APPLY_TEXT_LABEL_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.applytextlabel"
)


HUD_LAYOUT_APPLY_TEXT_LABEL_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
)


HUD_LAYOUT_APPLY_TEXT_LABEL_EMPTY_LITERAL_SYMBOL = "??_C@_00A@?$AA@"


HUD_LAYOUT_APPLY_TEXT_LABEL_RECEIVER_STORAGE_IDENTITY = (
    "argument(symbol:recoil:function:0x413990,target)"
)


HUD_LAYOUT_APPLY_TEXT_LABEL_RETAIL_CALLS = (
    (0x4139D1, "edx", 0x0C, None),
    (0x4139DC, "eax", 0x74, 8),
    (0x4139F5, "ecx", 0x74, 8),
)


HUD_LAYOUT_APPLY_TEXT_LABEL_CANDIDATE_CALLS = (
    (0x41, "edx", 0x0C, None),
    (0x4C, "eax", 0x74, 8),
    (0x65, "ecx", 0x74, 8),
)


HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_TIMER_BOUNDED_START = 0x0D


HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_TIMER_BOUNDED_END_EXCLUSIVE = 0x56


HUD_UI_MGR_UPDATE_FRAME_START_HIDE_SYMBOL = (
    "?StartHide@HudUiMgrObjective@@YIXXZ"
)


HUD_UI_MGR_UPDATE_FRAME_START_HIDE_ADDRESS = "0x411ac0"


HUD_UI_MGR_UPDATE_FRAME_START_HIDE_IDENTITY = (
    "symbol:recoil:function:0x411ac0"
)


HUD_UI_MGR_UPDATE_FRAME_OBJECTIVE_PANEL_CALLS = (
    (
        0x824,
        0x2F,
        0x35,
        0x37,
        2,
        "edx",
        "load(load(storage:recoil:data:0x4e5ed0+0x824))",
        0x04,
    ),
    (
        0x974,
        0x3A,
        0x40,
        0x42,
        3,
        "eax",
        "load(load(storage:recoil:data:0x4e5ed0+0x974))",
        0x04,
    ),
)


HUD_UI_MGR_UPDATE_FRAME_TIMER_FIELD_DISPLACEMENT = 0x4784


HUD_UI_MGR_UPDATE_FRAME_TIMER_LOAD_OFFSET = 0x45


HUD_UI_MGR_UPDATE_FRAME_TIME_LOAD_OFFSET = 0x4B


HUD_UI_MGR_UPDATE_FRAME_TIME_PUSH_OFFSET = 0x50


HUD_UI_MGR_UPDATE_FRAME_TIMER_VPTR_OFFSET = 0x51


HUD_UI_MGR_UPDATE_FRAME_TIMER_CALL_OFFSET = 0x53


HUD_UI_MGR_UPDATE_FRAME_TIMER_CALL_ORDINAL = 4


HUD_UI_MGR_UPDATE_FRAME_TIMER_SLOT_DISPLACEMENT = 0x24


HUD_UI_MGR_UPDATE_FRAME_TIMER_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4ea654)"
)


HUD_UI_MGR_UPDATE_FRAME_TIME_SYMBOL_ID = "recoil:data:0x56b42c"


HUD_UI_MGR_UPDATE_FRAME_TIME_STORAGE_ID = "recoil:storage:va:0x56b42c"


HUD_UI_MGR_UPDATE_FRAME_TIME_ADDRESS = "0x56b42c"


HUD_UI_MGR_UPDATE_FRAME_TIME_NAME = "g_Time_UnscaledDeltaTimeSec"


HUD_UI_MGR_UPDATE_FRAME_TIME_OBJECT_SYMBOL = "_g_Time_UnscaledDeltaTimeSec"


HUD_UI_MGR_UPDATE_FRAME_TIME_TARGET_ID = (
    "recoil:vc5-target:time_runtime_bss_globals"
)


HUD_UI_MGR_UPDATE_FRAME_STACK_MENU_BOUNDED_START = 0x82


HUD_UI_MGR_UPDATE_FRAME_STACK_MENU_BOUNDED_END_EXCLUSIVE = 0xC3


HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_SYMBOL = (
    "?UpdateAll@HudUiContainer@@UAEXM@Z"
)


HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_ADDRESS = "0x4bc900"


HUD_UI_MGR_UPDATE_FRAME_CONTAINER_UPDATE_IDENTITY = (
    "symbol:recoil:function:0x4bc900"
)


HUD_UI_MGR_UPDATE_FRAME_CONTAINER_TIME_LOAD_OFFSET = 0x82


HUD_UI_MGR_UPDATE_FRAME_CONTAINER_TIME_PUSH_OFFSET = 0x88


HUD_UI_MGR_UPDATE_FRAME_CONTAINER_RECEIVER_OFFSET = 0x89


HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_OFFSET = 0x8E


HUD_UI_MGR_UPDATE_FRAME_CONTAINER_CALL_ORDINAL = 9


HUD_UI_MGR_UPDATE_FRAME_STACK_CALLS = (
    (
        "top",
        "recoil:data:0x56bd24",
        "recoil:storage:va:0x56bd24",
        "0x56bd24",
        "g_HudUiTopMessageStack",
        "?g_HudUiTopMessageStack@@3PAUHudUiTextStack4@@A",
        "recoil:vc5-target:hud_ui_top_message_stack_global",
        "recoil:owner:hud_ui.hud_ui_top_message_stack_global",
        (
            "recoil:anchor:gamezrecoil-zui-zui-widgets-"
            "g-huduitopmessagestack"
        ),
        0x93,
        0x99,
        0x9E,
        0x9F,
        0xA1,
        10,
    ),
    (
        "chat",
        "recoil:data:0x56bd20",
        "recoil:storage:va:0x56bd20",
        "0x56bd20",
        "g_HudUiChatMessageStack",
        "?g_HudUiChatMessageStack@@3PAUHudUiTextStack4@@A",
        "recoil:vc5-target:hud_ui_chat_message_stack_global",
        "recoil:owner:hud_ui.hud_ui_chat_message_stack_global",
        (
            "recoil:anchor:gamezrecoil-zui-zui-widgets-"
            "g-huduichatmessagestack"
        ),
        0xA3,
        0xA9,
        0xAE,
        0xAF,
        0xB1,
        11,
    ),
)


HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_DISPLACEMENT = 0xEE0


HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_LOAD_OFFSET = 0xB3


HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_TIME_LOAD_OFFSET = 0xB9


HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_TIME_PUSH_OFFSET = 0xBE


HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_VPTR_OFFSET = 0xBF


HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_OFFSET = 0xC1


HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_CALL_ORDINAL = 12


HUD_UI_MGR_UPDATE_FRAME_STRING_MENU_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0xee0))"
)


HUD_UI_MGR_ENSURE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x410160"
)


HUD_UI_MGR_ENSURE_CALLER_START = "0x410160"


HUD_UI_MGR_ENSURE_CALLER_END_EXCLUSIVE = "0x410d10"


HUD_UI_MGR_ENSURE_CALLER_SYMBOL = (
    "?EnsureHudLoaded@HudUiMgr@@YIHPBD@Z"
)


HUD_UI_MGR_ENSURE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.ensurehudloaded"
)


HUD_UI_MGR_ENSURE_MENU_POINTER_ADDRESS = "0x4e6db0"


HUD_UI_MGR_ENSURE_MENU_POINTER_DISPLACEMENT = 0xEE0


HUD_UI_MGR_ENSURE_FONT_LOOP_CALL_ADDRESS = "0x410297"


HUD_UI_MGR_ENSURE_FONT_LOOP_CALL_ORDINAL = 13


HUD_UI_MGR_ENSURE_FONT_LOOP_SLOT_DISPLACEMENT = 0x80


HUD_UI_MGR_ENSURE_FONT_LOOP_STORAGE_IDENTITY = (
    "load(address(hud-ui-string-menu-item-loop-element))"
)


HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOAD_OFFSET = 0x134


HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_LOOP_OFFSET = 0x148


HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_CALL_OFFSET = 0x167


HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_STRIDE = 0x2A4


HUD_UI_MGR_ENSURE_FONT_LOOP_CANDIDATE_COUNT = 23


HUD_UI_MGR_ENSURE_FONT_LOOP_READ_RECT_SYMBOL = (
    "?ReadRect@HudUiLayoutNode@@YIHPAUNode@zReader@@"
    "PAUHudUiRect@@@Z"
)


HUD_UI_MGR_ENSURE_SENSOR_PANEL_DISPLACEMENT = 0xC24


HUD_UI_MGR_ENSURE_SENSOR_PANEL_ADDRESS = "0x4e6af4"


HUD_UI_MGR_ENSURE_SENSOR_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0xc24)"
)


HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_IDENTITY = (
    "symbol:recoil:function:0x404d90"
)


HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_ADDRESS = "0x404d90"


HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_X_SYMBOL = (
    "?GetCenterX@HudUiWidget@@UAEHXZ"
)


HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_IDENTITY = (
    "symbol:recoil:function:0x404dd0"
)


HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_ADDRESS = "0x404dd0"


HUD_UI_MGR_ENSURE_SENSOR_GET_CENTER_Y_SYMBOL = (
    "?GetCenterY@HudUiWidget@@UAEHXZ"
)


HUD_UI_MGR_ENSURE_SENSOR_RETAIL_LOAD_X_ADDRESS = "0x41036a"


HUD_UI_MGR_ENSURE_SENSOR_RETAIL_RECEIVER_X_ADDRESS = "0x410370"


HUD_UI_MGR_ENSURE_SENSOR_RETAIL_CALL_X_ADDRESS = "0x410375"


HUD_UI_MGR_ENSURE_SENSOR_RETAIL_RESULT_X_ADDRESS = "0x410378"


HUD_UI_MGR_ENSURE_SENSOR_RETAIL_LOAD_Y_ADDRESS = "0x41037b"


HUD_UI_MGR_ENSURE_SENSOR_RETAIL_RECEIVER_Y_ADDRESS = "0x410380"


HUD_UI_MGR_ENSURE_SENSOR_RETAIL_CALL_Y_ADDRESS = "0x410385"


HUD_UI_MGR_ENSURE_SENSOR_CALL_X_ORDINAL = 24


HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_ORDINAL = 25


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_CALL_X_ORDINAL = 25


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_CALL_Y_ORDINAL = 26


HUD_UI_MGR_ENSURE_SENSOR_CALL_X_SLOT = 0x64


HUD_UI_MGR_ENSURE_SENSOR_CALL_Y_SLOT = 0x68


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_X_OFFSET = 0x268


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_RECEIVER_X_OFFSET = 0x26E


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_CALL_X_OFFSET = 0x273


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_RESULT_X_OFFSET = 0x276


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_LOAD_Y_OFFSET = 0x278


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_RECEIVER_Y_OFFSET = 0x27D


HUD_UI_MGR_ENSURE_SENSOR_CANDIDATE_CALL_Y_OFFSET = 0x282


HUD_UI_MGR_ENSURE_SENSOR_METER_IMAGE_DISPLACEMENT = 0xC60


HUD_UI_MGR_ENSURE_SENSOR_METER_DISPLACEMENT = 0xD9C


HUD_UI_MGR_ENSURE_SENSOR_METER_COLOR_DISPLACEMENT = 0xED0


HUD_UI_MGR_ENSURE_SENSOR_METER_PANEL_DISPLACEMENT = 0xC24


HUD_UI_MGR_ENSURE_SENSOR_METER_OVERLAY_DISPLACEMENT = 0xCE0


HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_IMAGE_ADDRESS = "0x4e6b30"


HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_RECEIVER_ADDRESS = "0x4e6c6c"


HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_COLOR_ADDRESS = "0x4e6da0"


HUD_UI_MGR_ENSURE_SENSOR_METER_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0xd9c)"
)


HUD_UI_MGR_ENSURE_SENSOR_METER_SLOT_IDENTITY = (
    "HudUiMeter_FTable.common.SetClip"
)


HUD_UI_MGR_ENSURE_SENSOR_METER_SLOT_DISPLACEMENT = 0x18


HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_CALL_ADDRESS = "0x410489"


HUD_UI_MGR_ENSURE_SENSOR_METER_RETAIL_CALL_ORDINAL = 31


HUD_UI_MGR_ENSURE_SENSOR_METER_CANDIDATE_CALL_ORDINAL = 32


HUD_UI_MGR_ENSURE_SENSOR_METER_CANDIDATE_LOAD_OFFSET = 0x38D


HUD_UI_MGR_ENSURE_SENSOR_METER_CANDIDATE_CALL_OFFSET = 0x3A8


HUD_UI_MGR_ENSURE_SENSOR_METER_ADD_CHILD_SYMBOL = (
    "?AddChild@HudUiContainer@@QAEHPAUHudUiElement@@@Z"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_KEY_ADDRESS = "0x4dad6c"


HUD_UI_MGR_ENSURE_OBJECTIVE_KEY_SYMBOL = (
    "?g_HudCfgKey_Objective@@3PADA"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_PHASE_DURATION_DISPLACEMENT = 0x69C


HUD_UI_MGR_ENSURE_OBJECTIVE_PHASE_DURATION_ADDRESS = "0x4e656c"


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_DISPLACEMENT = 0x6AC


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_ADDRESS = "0x4e657c"


HUD_UI_MGR_ENSURE_OBJECTIVE_GET_NAMED_NODE_SYMBOL = (
    "@zReader_GetNamedNode@8"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_GET_NAMED_NODE_ADDRESS = "0x48cf70"


HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_SYMBOL = (
    "?ApplyImageWidget@HudUiLayoutNode@@"
    "YIPAUzVidImagePartial@@PAUNode@zReader@@PAUHudUiWidget@@"
    "HHPBHPAU2@PAUHudUiRect@@@Z"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_IMAGE_WIDGET_ADDRESS = "0x413d30"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_X_ADDRESS = "0x4104e9"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_Y_ADDRESS = "0x4104f9"


# Zero-based contract ordinals; these are the 37th/38th retail calls.
HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_X_ORDINAL = 36


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_CALL_Y_ORDINAL = 37


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_CALL_X_OFFSET = 0x414


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_CALL_Y_OFFSET = 0x42F


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_CALL_X_ORDINAL = 38


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_CALL_Y_ORDINAL = 39


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0x6ac)"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_X_ADDRESS = "0x410522"


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_Y_ADDRESS = "0x410532"


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_X_ORDINAL = 39


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_RETAIL_CALL_Y_ORDINAL = 40


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_CANDIDATE_LOAD_X_OFFSET = 0x44C


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_CANDIDATE_CALL_X_OFFSET = 0x457


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_CANDIDATE_LOAD_Y_OFFSET = 0x45E


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_CANDIDATE_CALL_Y_OFFSET = 0x468


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_CANDIDATE_CALL_X_ORDINAL = 41


HUD_UI_MGR_ENSURE_OBJECTIVE_WIDGET_CANDIDATE_CALL_Y_ORDINAL = 42


HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_CORNER_TEXT_QUAD_SYMBOL = (
    "?ApplyCornerTextQuad@HudUiLayoutNode@@"
    "YIHPAUNode@zReader@@PAUHudUiBar@@PBHPAUHudUiRect@@@Z"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_CORNER_TEXT_QUAD_ADDRESS = "0x413b10"


HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_SYMBOL = (
    "?ReadInt3@HudUiLayoutNode@@YIHPAUNode@zReader@@PAH11@Z"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_READ_INT3_IDENTITY = (
    "symbol:recoil:function:0x413ad0"
)


HUD_UI_MGR_ENSURE_STATS_FIRST_READ_INT3_ORDINAL = 90


HUD_UI_MGR_ENSURE_STATS_READ_INT3_COUNT = 6


HUD_UI_MGR_ENSURE_STATS_INTERPOLATE_SYMBOL = (
    "?InterpolateLayout@HudUiTriplet@@QAEXM@Z"
)


HUD_UI_MGR_ENSURE_STATS_MEMBER_OFFSETS = (
    (0xA8, 0xAC, 0xB8),
    (0xB0, 0xB4, 0xBC),
    (0xC0, 0xC4, None),
    (0xC8, 0xCC, None),
    (0xD0, 0xD4, None),
    (0xD8, 0xDC, None),
)


HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_DISPLACEMENT = 0x824


HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_DISPLACEMENT = 0x974


HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_ADDRESS = "0x4e66f4"


HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_ADDRESS = "0x4e6844"


HUD_UI_MGR_ENSURE_OBJECTIVE_SUMMARY_PANEL_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x824))"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_DESC_PANEL_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x974))"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_TEXT_SETPOS_SLOT = 0x0C


HUD_UI_MGR_ENSURE_OBJECTIVE_WORD_WRAP_SLOT = 0x6C


HUD_UI_MGR_ENSURE_OBJECTIVE_APPLY_METER_QUAD_SYMBOL = (
    "?ApplyMeterQuad@HudUiLayoutNode@@"
    "YIHPAUNode@zReader@@PAUHudUiBar@@HHPBHPAUHudUiRect@@@Z"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_SUMMARY_ADDRESS = "0x4105a1"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_SUMMARY_ADDRESS = "0x4105c0"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_DESC_ADDRESS = "0x4105d1"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_DESC_ADDRESS = "0x4105f0"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_WORD_WRAP_ADDRESS = "0x410623"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_APPLY_METER_ADDRESS = "0x41063b"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_SUMMARY_ORDINAL = 44


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_SUMMARY_ORDINAL = 45


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_DESC_ORDINAL = 46


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_DESC_ORDINAL = 47


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_WORD_WRAP_ORDINAL = 48


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_SUMMARY_OFFSET = 0x509


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LOAD_SUMMARY_OFFSET = 0x512


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_SUMMARY_OFFSET = 0x52C


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_DESC_OFFSET = 0x53C


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LOAD_DESC_OFFSET = 0x541


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_DESC_OFFSET = 0x55F


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_WORD_WRAP_LOAD_OFFSET = 0x597


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_WORD_WRAP_OFFSET = 0x5AA


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_APPLY_METER_OFFSET = 0x5C1


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_SUMMARY_ORDINAL = 46


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_SUMMARY_ORDINAL = 47


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_DESC_ORDINAL = 48


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_DESC_ORDINAL = 49


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_WORD_WRAP_ORDINAL = 50


HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_DISPLACEMENT = 0x828


HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_ADDRESS = "0x4e66f8"


HUD_UI_MGR_ENSURE_OBJECTIVE_LABEL_PANEL_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x828))"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_LABEL_ADDRESS = "0x410698"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_LABEL_ADDRESS = "0x4106b5"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_LABEL_MESSAGE_ADDRESS = "0x4106c5"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_READ_LABEL_ORDINAL = 54


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETPOS_LABEL_ORDINAL = 55


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_LABEL_MESSAGE_ORDINAL = 56


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_LABEL_OFFSET = 0x61F


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LOAD_LABEL_OFFSET = 0x62A


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_LABEL_OFFSET = 0x63E


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LABEL_MESSAGE_OFFSET = 0x64D


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_READ_LABEL_ORDINAL = 56


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETPOS_LABEL_ORDINAL = 57


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_LABEL_MESSAGE_ORDINAL = 58


HUD_UI_MGR_ENSURE_OBJECTIVE_GET_MESSAGE_STRING_SYMBOL = (
    "?GetMessageString@zLoc@@YIPADI@Z"
)


HUD_UI_MGR_ENSURE_OBJECTIVE_SETTEXTFMT_SLOT = 0x74


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETTEXTFMT_ADDRESS = "0x4106d2"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_SETTEXTFMT_ORDINAL = 57


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_NEXT_SETPOS_ADDRESS = "0x4106f0"


HUD_UI_MGR_ENSURE_OBJECTIVE_RETAIL_NEXT_SETPOS_ORDINAL = 58


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETTEXTFMT_OFFSET = 0x65F


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_REPAIRED_SETTEXTFMT_OFFSET = 0x65A


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_SETTEXTFMT_ORDINAL = 59


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_NEXT_SETPOS_OFFSET = 0x67D


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_REPAIRED_NEXT_SETPOS_OFFSET = 0x678


HUD_UI_MGR_ENSURE_OBJECTIVE_CANDIDATE_NEXT_SETPOS_ORDINAL = 60


HUD_UI_MGR_ENSURE_CEIL_IMPORT_DLL = "MSVCRT.dll"


HUD_UI_MGR_ENSURE_CEIL_IMPORT_NAME = "ceil"


HUD_UI_MGR_ENSURE_CEIL_IAT_ADDRESS = "0x4cc500"


HUD_UI_MGR_ENSURE_CEIL_IAT_IDENTITY = "iat:ceil"


HUD_UI_MGR_ENSURE_CEIL_CANDIDATE_IMPORT_SYMBOL = "__imp__ceil"


HUD_UI_MGR_ENSURE_CEIL_PROVIDER_ADDRESS = "0x7c8e18"


HUD_UI_MGR_ENSURE_CEIL_IAT_TYPE = "double (* const)(double _X)"


HUD_UI_MGR_ENSURE_CEIL_PROVIDER_TYPE = "double(double _X)"


HUD_UI_MGR_ENSURE_CEIL_CALL_ADDRESS = "0x410651"


HUD_UI_MGR_ENSURE_CEIL_CALL_ORDINAL = 51


HUD_UI_MGR_ENSURE_CEIL_CLEANUP_BYTES = 8


HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_IDENTITY = (
    "symbol:recoil:function:0x411760"
)


HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_START = "0x411760"


HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_END_EXCLUSIVE = "0x4117f0"


HUD_UI_MGR_OBJECTIVE_CEIL_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.setvisibleandresetmeterfill"
)


HUD_UI_MGR_OBJECTIVE_CEIL_CALL_ADDRESS = "0x411787"


HUD_UI_MGR_OBJECTIVE_CEIL_CALL_ORDINAL = 2


HUD_UI_MGR_OBJECTIVE_CEIL_CLEANUP_BYTES = 8


HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4117f0"
)


HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_START = "0x4117f0"


HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_END_EXCLUSIVE = "0x4118b0"


HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.tickmeterfillanimation"
)


HUD_UI_MGR_OBJECTIVE_TICK_CEIL_CALL_SPECS = (
    ("0x41181d", 0, 8),
    ("0x411875", 3, 8),
)


HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_IDENTITY = (
    "symbol:recoil:function:0x411f10"
)


HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_START = "0x411f10"


HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_END_EXCLUSIVE = "0x412050"


HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.setshieldmessageratio"
)


HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALLER_SYMBOL = (
    "?SetShieldMessageRatio@HudUiMgrSensor@@YIXM@Z"
)


HUD_UI_MGR_SENSOR_SHIELD_CEIL_CALL_SPECS = (
    ("0x411fae", 2, 8),
    ("0x412002", 6, 8),
)


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4124b0"
)


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_START = "0x4124b0"


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_END_EXCLUSIVE = "0x412620"


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.updateselectedprogressmeter"
)


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALLER_SYMBOL = (
    "?UpdateSelectedProgressMeter@HudUiMgrTarget@@YIXH@Z"
)


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALL_ADDRESS = "0x4125cd"


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CALL_ORDINAL = 3


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CLEANUP_BYTES = 8


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CANDIDATE_CALL_OFFSET = 0x12D


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CANDIDATE_RELOCATION_OFFSET = 0x12F


HUD_UI_MGR_SELECTED_PROGRESS_CEIL_CANDIDATE_CLEANUP_OFFSET = 0x133


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_VPTR = 0x412540


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_FALSE = 0x412546


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_RECEIVER = 0x412548


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_CALL = 0x41254D


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_SUCCESSOR = 0x412550


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_RETAIL_ORDINAL = 0


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_VPTR = 0xA0


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_FALSE = 0xA6


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_RECEIVER = 0xA8


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_CALL = 0xAD


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_CANDIDATE_SUCCESSOR = 0xB0


HUD_UI_MGR_SELECTED_PROGRESS_VISIBLE_SLOT = 0x60


HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_DISPLACEMENT = 0xC20


HUD_UI_MGR_SELECTED_PROGRESS_TRACKED_SLOT_ADDRESS = "0x4e6af0"


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_IDENTITY = (
    "symbol:recoil:function:0x412620"
)


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_START = "0x412620"


# The tracker/order interval reaches the next selected function at 0x412650;
# the reviewed retail body itself ends at 0x412642.
HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_END_EXCLUSIVE = "0x412650"


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_RETAIL_BODY_END_EXCLUSIVE = 0x412642


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud."
    "hidetrackedprogressmeterifownermatches"
)


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CALLER_SYMBOL = (
    "?HideTrackedProgressMeterIfOwnerMatches@HudUiMgr@@YIXPAX@Z"
)


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_RETAIL_CALL_ADDRESS = 0x41263E


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_RETAIL_CALL_ORDINAL = 0


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_CANDIDATE_CALL_OFFSET = 0x1E


HUD_UI_MGR_HIDE_TRACKED_PROGRESS_SLOT = 0x60


HUD_UI_MESSAGE_SET_VALUE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x412650"
)


HUD_UI_MESSAGE_SET_VALUE_CALLER_START = "0x412650"


HUD_UI_MESSAGE_SET_VALUE_CALLER_END_EXCLUSIVE = "0x4126e0"


HUD_UI_MESSAGE_SET_VALUE_CALLER_SYMBOL = (
    "?SetValueIfOwnerMatches@HudUiMessage@@SIXHHM@Z"
)


HUD_UI_MESSAGE_SET_VALUE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.huduimessage-setvalueifownermatches"
)


HUD_UI_MESSAGE_SET_VALUE_CEIL_CALL_ADDRESS = "0x4126a6"


HUD_UI_MESSAGE_SET_VALUE_CEIL_CALL_ORDINAL = 1


HUD_UI_MESSAGE_SET_VALUE_CEIL_CLEANUP_BYTES = 8


HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_CALL_OFFSET = 0x66


HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_RELOCATION_OFFSET = 0x68


HUD_UI_MESSAGE_SET_VALUE_CEIL_CANDIDATE_CLEANUP_OFFSET = 0x6C


HUD_UI_MESSAGE_SET_VALUE_FTOL_CALL_ADDRESS = "0x4126af"


HUD_UI_MESSAGE_SET_VALUE_FTOL_CALL_ORDINAL = 2


HUD_UI_MESSAGE_SET_VALUE_FTOL_CANDIDATE_CALL_OFFSET = 0x6F


HUD_UI_MESSAGE_SET_VALUE_FTOL_CANDIDATE_RELOCATION_OFFSET = 0x70


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_VPTR = 0x4126B4


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_RECEIVER = 0x4126BA


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_RESULT_ARGUMENT = 0x4126C0


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_FORMAT_ARGUMENT = 0x4126C1


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_THIS_ARGUMENT = 0x4126C6


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_CALL = 0x4126C7


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_VPTR = 0x4126CA


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_CLEANUP = 0x4126CC


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_RECEIVER = 0x4126CF


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_RETAIL_NEXT_CALL = 0x4126D1


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CALL_ORDINAL = 3


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_SLOT = 0x74


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CLEANUP_BYTES = 12


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_ADDRESS = "0x4dacbc"


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_FORMAT_SYMBOL = (
    "??_C@_02MECO@?$CFd?$AA@"
)


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_VPTR = 0x54


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_RECEIVER = 0x5D


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_RESULT_ARGUMENT = 0x74


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_FORMAT_ARGUMENT = 0x75


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_FORMAT_RELOCATION = 0x76


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_THIS_ARGUMENT = 0x7A


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_CALL = 0x7B


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_VPTR = 0x7E


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_CLEANUP = 0x80


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_RECEIVER = 0x83


HUD_UI_MESSAGE_SET_VALUE_NUMERIC_CANDIDATE_NEXT_CALL = 0x85


HUD_UI_MESSAGE_SET_VALUE_INVALIDATE_CALL_ORDINAL = 4


HUD_UI_MESSAGE_SET_VALUE_INVALIDATE_SLOT = 0x20


HUD_UI_MESSAGE_SET_VALUE_RETAIL_TAIL_POP = 0x4126D4


HUD_UI_MESSAGE_SET_VALUE_RETAIL_TAIL_RETURN = 0x4126D5


HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_POP_EDI = 0x88


HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_POP_ESI = 0x89


HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_POP_EBX = 0x8A


HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_TAIL_RETURN = 0x8B


HUD_UI_MESSAGE_SET_VALUE_RETAIL_INVOCATION_ORDER = (
    0x412692,
    0x4126A6,
    0x4126AF,
    0x4126C7,
    0x4126D1,
)


HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_INVOCATION_ORDER = (
    0x44,
    0x66,
    0x6F,
    0x7B,
    0x85,
)


HUD_UI_MESSAGE_SET_VALUE_ARRAY_DISPLACEMENT = 0x4B18


HUD_UI_MESSAGE_SET_VALUE_OWNER_DISPLACEMENT = 0x4E9C


HUD_UI_MESSAGE_SET_VALUE_ELEMENT_STRIDE = 0x44C


HUD_UI_MESSAGE_SET_VALUE_PANEL_OFFSET = 0xE0


HUD_UI_MESSAGE_SET_VALUE_SLOT = 0x74


HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_ID = "recoil:data:0x4dae08"


HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_ADDRESS = "0x4dae08"


HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_NAME = (
    "g_HudUiMessage_ClearSpecialToken165"
)


HUD_UI_MESSAGE_SET_VALUE_CLEAR_TOKEN_SYMBOL = (
    "?g_HudUiMessage_ClearSpecialToken165@@3PADA"
)


HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY = (
    "load(address(indexed(storage:recoil:data:0x4e5ed0+0x4b18,"
    "stride=0x44c))+0xe0)"
)


HUD_UI_MESSAGE_SET_VALUE_MESSAGE_STORAGE_IDENTITY = (
    "address(indexed(storage:recoil:data:0x4e5ed0+0x4b18,"
    "stride=0x44c))"
)


HUD_UI_MESSAGE_SET_VALUE_MESSAGE_VPTR_IDENTITY = (
    "load(address(indexed(storage:recoil:data:0x4e5ed0+0x4b18,"
    "stride=0x44c)))"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_START = "0x4127d0"


HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_END_EXCLUSIVE = "0x412820"


HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4127d0"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_SYMBOL = (
    "?ClearDisplay@HudUiMessage@@SIXH@Z"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.huduimessage-cleardisplay"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_ORDER_TARGET_ID = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_BYTE_TARGET_ID = (
    "recoil:vc5-target:hud_ui_message_select_variant_and_clear_display"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_ADDRESS = "0x4b3e70"


HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_IDENTITY = (
    "symbol:recoil:function:0x4b3e70"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_DIRECT_TARGET_SYMBOL = (
    "?SetImageBorrowedAndInvalidate@HudUiWidget@@"
    "QAEPAUzVidImagePartial@@PAU2@@Z"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_ID = "recoil:data:0x4e5ce0"


HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_ADDRESS = "0x4e5ce0"


HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_NAME = (
    "VC5PooledEmptyStringLiteral"
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_EMPTY_LITERAL_SYMBOL = "??_C@_00A@?$AA@"


HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_CALL_ORDER = (
    0x4127E8,
    0x4127F5,
    0x41280C,
    0x412816,
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_CANDIDATE_CALL_ORDER = (
    0x18,
    0x25,
    0x3B,
    0x45,
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_CORRECTED_CANDIDATE_CALL_ORDER = (
    0x18,
    0x25,
    0x3C,
    0x46,
)


HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_CALL = 0x41280C


HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_INVALIDATE_CALL = 0x412816


HUD_UI_MESSAGE_CLEAR_DISPLAY_CANDIDATE_PANEL_CALL = 0x3B


HUD_UI_MESSAGE_CLEAR_DISPLAY_CANDIDATE_INVALIDATE_CALL = 0x45


HUD_UI_MESSAGE_CLEAR_DISPLAY_CORRECTED_CANDIDATE_PANEL_CALL = 0x3C


HUD_UI_MESSAGE_CLEAR_DISPLAY_CORRECTED_CANDIDATE_INVALIDATE_CALL = 0x46


HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_SLOT = 0x74


HUD_UI_MESSAGE_CLEAR_DISPLAY_CANDIDATE_PANEL_SLOT = 0x8C


HUD_UI_MESSAGE_CLEAR_DISPLAY_INVALIDATE_SLOT = 0x20


HUD_UI_MESSAGE_CLEAR_DISPLAY_RETAIL_PANEL_CLEANUP = 8


HUD_UI_MESSAGE_CLEAR_DISPLAY_CORRECTED_CANDIDATE_PANEL_CLEANUP = 8


HUD_UI_MESSAGE_SET_VALUE_RETAIL_CALL = 0x412692


HUD_UI_MESSAGE_SET_VALUE_CANDIDATE_CALL = 0x44


HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_START = "0x412820"


HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_END_EXCLUSIVE = "0x412b60"


HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_IDENTITY = (
    "symbol:recoil:function:0x412820"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_SYMBOL = (
    "?UpdateSelectedWeaponDisplay@HudUiMessage@@SIXHHM@Z"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud."
    "huduimessage-updateselectedweapondisplay"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_ADDRESS = "0x4126e0"


HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_IDENTITY = (
    "symbol:recoil:function:0x4126e0"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_SELECT_VARIANT_SYMBOL = (
    "?SelectVariantDisplay@HudUiMessage@@SIXHH@Z"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_ADDRESS = "0x4b3e70"


HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_IDENTITY = (
    "symbol:recoil:function:0x4b3e70"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_SET_IMAGE_SYMBOL = (
    "?SetImageBorrowedAndInvalidate@HudUiWidget@@"
    "QAEPAUzVidImagePartial@@PAU2@@Z"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_FTOL_ADDRESS = "0x4c60a6"


HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_ADDRESS = "0x4cc500"


HUD_UI_MESSAGE_UPDATE_WEAPON_CEIL_IAT_IDENTITY = "iat:ceil"


HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_SYMBOL = (
    "_kHudUiMessageClearSpecialTokenValue$S83570"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_SEMANTIC_BASENAME = (
    "_kHudUiMessageClearSpecialTokenValue"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_PRIVATE_SYMBOL_RE = re.compile(
    rf"^{re.escape(HUD_UI_MESSAGE_UPDATE_WEAPON_CLEAR_FLOAT_SEMANTIC_BASENAME)}"
    r"\$S[0-9]+$"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_ZERO_FLOAT_SYMBOL = "$T88364"


HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_CALL_ORDER = (
    0x41285E,
    0x412887,
    0x4128C0,
    0x412925,
    0x41294C,
    0x412989,
    0x4129EE,
    0x412A06,
    0x412A0F,
    0x412A27,
    0x412A31,
    0x412A52,
    0x412A79,
    0x412AB5,
    0x412AF7,
    0x412B0F,
    0x412B18,
    0x412B2E,
    0x412B3F,
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CANDIDATE_CALL_ORDER = (
    0x18,
    0x3F,
    0x54,
    0x9F,
    0xC1,
    0xCA,
    0xD6,
    0xE0,
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CORRECTED_CANDIDATE_CALL_ORDER = (
    0x40,
    0x69,
    0xA2,
    0x107,
    0x12E,
    0x16B,
    0x1A0,
    0x1C6,
    0x1FC,
    0x259,
    0x27D,
    0x286,
    0x292,
    0x29C,
)


HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_SHAPED_CANDIDATE_CALL_ORDER = (
    0x3E, 0x67, 0xA0, 0x103, 0x12A, 0x167, 0x1C6, 0x1EA, 0x1F3,
    0x1FF, 0x209, 0x22A, 0x252, 0x28C, 0x2CF, 0x2E7, 0x2F0,
    0x305, 0x316,
)


HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_SET_TEXT_CALLS = (
    (0x4129EE, "ecx", 8),
    (0x412A27, "edx", 12),
    (0x412AF7, "eax", 8),
    (0x412B2E, "ecx", 12),
)


HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_INVALIDATE_CALLS = (
    (0x412A31, "eax"),
    (0x412B3F, "edx"),
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CANDIDATE_SET_TEXT_CALLS = (
    (0x9F, "edx", 8),
    (0xD6, "ebx", 12),
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CANDIDATE_INVALIDATE_CALL = 0xE0


HUD_UI_MESSAGE_UPDATE_WEAPON_CORRECTED_CANDIDATE_SET_TEXT_CALLS = (
    (0x259, "ecx", 8),
    (0x292, "ebx", 12),
)


HUD_UI_MESSAGE_UPDATE_WEAPON_CORRECTED_CANDIDATE_INVALIDATE_CALL = 0x29C


HUD_UI_MESSAGE_UPDATE_WEAPON_CORRECTED_ZERO_FLOAT_SYMBOL = "$T88391"


HUD_UI_MESSAGE_UPDATE_WEAPON_RETAIL_SHAPED_ZERO_FLOAT_SYMBOL = "$T88393"


HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_PANEL_STORAGE_IDENTITY = (
    HUD_UI_MESSAGE_SET_VALUE_PANEL_STORAGE_IDENTITY
)


HUD_UI_MESSAGE_UPDATE_WEAPON_SELECTED_MESSAGE_VPTR_IDENTITY = (
    HUD_UI_MESSAGE_SET_VALUE_MESSAGE_VPTR_IDENTITY
)


HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_PANEL_STORAGE_IDENTITY = (
    "load(address(storage:recoil:data:0x4e5ed0+0x5044))"
)


HUD_UI_MESSAGE_UPDATE_WEAPON_FIXED_MESSAGE_VPTR_IDENTITY = (
    "load(address(storage:recoil:data:0x4e5ed0+0x4f64))"
)


HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_START = "0x414070"


HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_END_EXCLUSIVE = "0x414180"


HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_IDENTITY = (
    "symbol:recoil:function:0x414070"
)


HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_SYMBOL = (
    "?RebuildWeaponLayout@HudUiMessage@@QAEXXZ"
)


HUD_UI_MESSAGE_REBUILD_WEAPON_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.huduimessage-rebuildweaponlayout"
)


HUD_UI_MESSAGE_REBUILD_WEAPON_ORDER_TARGET_ID = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order"
)


HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_DISPLACEMENT = 0x1B4


HUD_UI_MESSAGE_REBUILD_WEAPON_ORIGIN_X_DISPLACEMENT = 0x320


HUD_UI_MESSAGE_REBUILD_WEAPON_LAYOUT_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4ed718+0x1b4)"
)


HUD_UI_MESSAGE_REBUILD_WEAPON_THIS_VPTR_IDENTITY = "load(this)"


HUD_UI_MESSAGE_REBUILD_WEAPON_PANEL_VPTR_IDENTITY = "load(this+0xe0)"


HUD_UI_MESSAGE_REBUILD_WEAPON_WIDGET_VPTR_IDENTITY = "load(this+0x390)"


HUD_UI_MESSAGE_REBUILD_WEAPON_RETAIL_CALL_ORDER = (
    0x414083,
    0x414093,
    0x4140DD,
    0x4140EB,
    0x414134,
    0x414142,
    0x414171,
)


HUD_UI_MESSAGE_REBUILD_WEAPON_CANDIDATE_CALL_ORDER = (
    0x13,
    0x23,
    0x6F,
    0x7D,
    0xC8,
    0xD6,
    0x109,
)


HUD_UI_MESSAGE_REBUILD_WEAPON_RETAIL_CODE = bytes.fromhex(
    "83 ec 24 a1 cc d8 4e 00 53 55 56 8b f1 57 b9 cc d8 4e 00 "
    "ff 50 64 8b 15 cc d8 4e 00 b9 cc d8 4e 00 8b d8 ff 52 68 "
    "8b e8 a1 f0 61 4e 00 8b be 88 03 00 00 99 2b c2 d1 f8 03 "
    "f8 8b 86 bc 00 00 00 89 7c 24 14 0f bf 48 04 03 cf 89 4c "
    "24 1c 8b 8e 8c 03 00 00 89 4c 24 18 0f bf 50 06 8b 06 03 "
    "d1 89 54 24 20 03 cd 8d 14 1f 51 52 8b ce ff 50 0c 8b 06 "
    "8d 4c 24 14 51 6a 00 8b ce ff 50 18 8b 54 24 1c 8b 4c 24 "
    "20 83 c7 03 89 4c 24 28 8d 42 fe 8d 51 0c 89 7c 24 24 03 "
    "cd 8d be e0 00 00 00 51 8b 4c 24 28 89 54 24 34 8b 17 89 "
    "44 24 30 2b c1 89 54 24 14 99 2b c2 8b 54 24 14 d1 f8 03 "
    "c1 8b cf 03 c3 50 ff 52 0c 8b 07 8d 4c 24 24 51 6a 00 8b "
    "cf ff 50 18 8d 8e 90 03 00 00 8b b6 d8 00 00 00 0f bf 46 "
    "06 8b 11 2b e8 8b 44 24 20 8d 44 05 ff 50 0f bf 46 04 2b "
    "d8 8b 44 24 20 8d 44 03 ff 50 ff 52 0c 5f 5e 5d 5b 83 c4 "
    "24 c3"
)


HUD_UI_MESSAGE_REBUILD_WEAPON_CANDIDATE_BODY = bytes.fromhex(
    "83 ec 24 a1 b4 01 00 00 53 55 56 8b f1 57 b9 b4 01 00 00 ff "
    "50 64 8b 15 b4 01 00 00 b9 b4 01 00 00 8b d8 ff 52 68 8b "
    "e8 a1 20 03 00 00 99 8b 8e bc 00 00 00 2b c2 8b f8 8b 86 "
    "88 03 00 00 d1 ff 03 f8 8b 86 8c 03 00 00 89 7c 24 14 89 "
    "44 24 18 0f bf 51 04 03 d7 89 54 24 1c 8b 16 0f bf 49 06 "
    "03 c8 03 c5 89 4c 24 20 8d 0c 1f 50 51 8b ce ff 52 0c 8b "
    "16 8d 44 24 14 50 6a 00 8b ce ff 52 18 8b 54 24 1c 8b 4c "
    "24 20 83 c7 03 89 4c 24 28 8d 42 fe 8d 51 0c 89 7c 24 24 "
    "03 cd 8d be e0 00 00 00 51 8b 4c 24 28 89 54 24 34 8b 17 "
    "89 44 24 30 2b c1 89 54 24 14 99 2b c2 8b d1 d1 f8 03 c2 "
    "8b cf 03 c3 50 8b 44 24 18 ff 50 0c 8b 17 8d 44 24 24 50 "
    "6a 00 8b cf ff 52 18 8b 86 d8 00 00 00 8b 7c 24 20 8b 96 "
    "90 03 00 00 8d 8e 90 03 00 00 0f bf 70 06 2b fe 0f bf 40 "
    "04 8d 74 2f ff 56 8b 74 24 20 2b f0 8d 44 1e ff 50 ff 52 "
    "0c 5f 5e 5d 5b 83 c4 24 c3 90 90 90 90 90 90 90 90 90 "
    "90 90 90"
)


HUD_LOADING_CHECKPOINT_CALLER_START = "0x414180"


HUD_LOADING_CHECKPOINT_CALLER_END_EXCLUSIVE = "0x414210"


HUD_LOADING_CHECKPOINT_CALLER_IDENTITY = "symbol:recoil:function:0x414180"


HUD_LOADING_CHECKPOINT_CALLER_SYMBOL = (
    "?AdvanceAndLog@HudUiLoadingCheckpoint@@YIXPBD@Z"
)


HUD_LOADING_CHECKPOINT_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.advanceandlog"
)


HUD_LOADING_CHECKPOINT_TRIGGER_TARGET_ID = (
    "recoil:vc5-target:hud_ui_mgr_trigger_current_layout_on_activated"
)


HUD_LOADING_CHECKPOINT_RETAIL_CALL_ORDER = (
    0x4141C5,
    0x4141D2,
    0x4141E4,
    0x4141EF,
    0x4141FB,
)


HUD_LOADING_CHECKPOINT_CANDIDATE_CALL_ORDER = (
    0x26,
    0x52,
    0x64,
    0x6D,
    0x79,
)


HUD_LOADING_CHECKPOINT_STDIO_IAT_SPECS = (
    {
        "import_name": "puts",
        "candidate_symbol": "__imp__puts",
        "iat_address": "0x4cc4f4",
        "provider_address": "0x7c8e7c",
        "iat_type": "int32_t (* const)(char const* _Buffer)",
        "provider_type": "int32_t(char const* _Buffer)",
        "retail_call_address": "0x4141d2",
        "candidate_call_offset": 0x52,
        "candidate_relocation_offset": 0x54,
        "ordinal": 1,
    },
    {
        "import_name": "fflush",
        "candidate_symbol": "__imp__fflush",
        "iat_address": "0x4cc4fc",
        "provider_address": "0x7c8e2c",
        "iat_type": "int32_t (* const)(FILE* _Stream)",
        "provider_type": "int32_t(FILE* _Stream)",
        "retail_call_address": "0x4141e4",
        "candidate_call_offset": 0x64,
        "candidate_relocation_offset": 0x66,
        "ordinal": 2,
    },
)


GAMENET_CHAT_COMPOSE_CALLER_START = "0x414550"


GAMENET_CHAT_COMPOSE_CALLER_END_EXCLUSIVE = "0x414590"


GAMENET_CHAT_COMPOSE_CALLER_IDENTITY = "symbol:recoil:function:0x414550"


GAMENET_CHAT_COMPOSE_CALLER_SYMBOL = (
    "?ChatComposeKeyCallback@GameNet@@YIXH@Z"
)


GAMENET_CHAT_COMPOSE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.chatcomposekeycallback"
)


GAMENET_CHAT_COMPOSE_CONTRIBUTION_SYMBOL_RE = (
    r"\?ChatComposeKeyCallback@GameNet@@.*"
)


GAMENET_CHAT_COMPOSE_AGGREGATE_POINTER_DISPLACEMENT = 0x974


GAMENET_CHAT_COMPOSE_TEXT_INPUT_DISPLACEMENT = 0xAB8


GAMENET_CHAT_COMPOSE_VIRTUAL_SLOT = 0x74


GAMENET_CHAT_COMPOSE_RETAIL_CALL_ORDER = (
    0x414550,
    0x414560,
    0x414571,
    0x41457E,
)


GAMENET_CHAT_COMPOSE_CANDIDATE_CALL_ORDER = (0x00, 0x10, 0x21, 0x2E)


GAMENET_CHAT_COMPOSE_DIRECT_CALLS = (
    (
        "0x46fba0",
        "?KeyboardTranslateDikToAscii@zInput@@YIHH@Z",
        "zInput::KeyboardTranslateDikToAscii",
    ),
    (
        "0x4b4460",
        "?DispatchKeyAction@HudUiTextInput@@QAEXH@Z",
        "HudUiTextInput::DispatchKeyAction",
    ),
    (
        "0x4b4410",
        "?GetBuffer@HudUiTextInput@@QAEPADXZ",
        "HudUiTextInput::GetBuffer",
    ),
)


GAMENET_END_CHAT_COMPOSE_CALLER_START = "0x414590"


GAMENET_END_CHAT_COMPOSE_CALLER_END_EXCLUSIVE = "0x414660"


GAMENET_END_CHAT_COMPOSE_CALLER_IDENTITY = "symbol:recoil:function:0x414590"


GAMENET_END_CHAT_COMPOSE_CALLER_SYMBOL = (
    "?EndChatComposeAndSend@GameNet@@YAXXZ"
)


GAMENET_END_CHAT_COMPOSE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.endchatcomposeandsend"
)


GAMENET_END_CHAT_COMPOSE_CONTRIBUTION_SYMBOL_RE = (
    r"\?EndChatComposeAndSend@GameNet@@.*"
)


GAMENET_END_CHAT_COMPOSE_STRNCAT_IAT_ADDRESS = "0x4cc4f0"


GAMENET_END_CHAT_COMPOSE_STRNCAT_CANDIDATE_SYMBOL = "__imp__strncat"


GAMENET_END_CHAT_COMPOSE_DIRECT_TARGETS = (
    ("0x471950", "?BindMapContextPop@zInput@@YIXXZ"),
    ("0x411a20", "?Begin@HudUiMgrObjective@@YIXXZ"),
    ("0x4b4410", "?GetBuffer@HudUiTextInput@@QAEPADXZ"),
    ("0x4138f0", "?ShowChatLine@HudUi@@YIXPBDM@Z"),
    ("0x433750", "?SendPkt0BChatMessage@GameNet@@YIXPBD@Z"),
)


GAMENET_END_CHAT_COMPOSE_RETAIL_CALL_ORDER = (
    0x4145AB,
    0x4145B0,
    0x4145BA,
    0x4145D5,
    0x414604,
    0x414624,
    0x41462F,
    0x41463D,
    0x414646,
)


GAMENET_END_CHAT_COMPOSE_CANDIDATE_CALL_ORDER = (
    0x18,
    0x1D,
    0x27,
    0x49,
    0x78,
    0x98,
    0xA3,
    0xB1,
    0xBA,
)


GAMENET_END_CHAT_COMPOSE_RETAIL_CODE = bytes.fromhex(
    "83 ec 54 a1 88 3a 4f 00 56 57 8b 70 24 33 c0 83 c6 24 "
    "88 44 24 58 a3 84 69 4e 00 e8 a0 d3 05 00 e8 6b d4 "
    "ff ff b9 88 69 4e 00 e8 51 fe 09 00 8b f8 83 c9 ff "
    "33 c0 f2 ae f7 d1 49 74 7e 6a 50 8d 4c 24 0c 56 51 "
    "ff 15 a0 c5 4c 00 8d 7c 24 14 83 c9 ff 33 c0 83 c4 "
    "0c f2 ae f7 d1 8b 35 f0 c4 4c 00 49 ba 50 00 00 00 "
    "8d 44 24 08 2b d1 52 68 48 ae 4d 00 50 ff d6 8d 7c "
    "24 14 83 c9 ff 33 c0 83 c4 0c f2 ae f7 d1 49 ba 50 "
    "00 00 00 2b d1 b9 88 69 4e 00 52 e8 e7 fd 09 00 50 "
    "8d 44 24 10 50 ff d6 83 c4 0c 8d 4c 24 08 68 00 00 "
    "a0 40 e8 ae f2 ff ff 8d 4c 24 08 e8 05 f1 01 00 5f "
    "5e 83 c4 54 c3"
)


GAMENET_END_CHAT_COMPOSE_CANDIDATE_CODE = bytes.fromhex(
    "83 ec 54 a1 00 00 00 00 56 57 8b 70 24 33 c0 88 44 24 "
    "58 a3 b4 0a 00 00 e8 00 00 00 00 e8 00 00 00 00 b9 "
    "b8 0a 00 00 e8 00 00 00 00 8b f8 83 c9 ff 33 c0 f2 "
    "ae f7 d1 49 0f 84 81 00 00 00 83 c6 24 6a 50 8d 44 "
    "24 0c 56 50 ff 15 00 00 00 00 8d 7c 24 14 83 c9 ff "
    "33 c0 83 c4 0c f2 ae f7 d1 8b 35 00 00 00 00 49 ba "
    "50 00 00 00 8d 44 24 08 2b d1 52 68 00 00 00 00 50 "
    "ff d6 8d 7c 24 14 83 c9 ff 33 c0 83 c4 0c f2 ae f7 "
    "d1 49 ba 50 00 00 00 2b d1 b9 b8 0a 00 00 52 e8 00 "
    "00 00 00 50 8d 44 24 10 50 ff d6 83 c4 0c 8d 4c 24 "
    "08 68 00 00 a0 40 e8 00 00 00 00 8d 4c 24 08 e8 00 "
    "00 00 00 5f 5e 83 c4 54 c3 90 90 90 90 90 90 90 90 "
    "90 90 90"
)


GAMENET_CHAT_COMPOSE_RETAIL_CODE = bytes.fromhex(
    "e8 4b b6 05 00 85 c0 74 2c 56 50 b9 88 69 4e 00 "
    "e8 fb fe 09 00 a1 44 68 4e 00 b9 88 69 4e 00 8b "
    "30 e8 9a fe 09 00 8b 0d 44 68 4e 00 50 51 ff 56 "
    "74 83 c4 08 5e c3 90 90 90 90 90 90 90 90 90 90"
)


GAMENET_CHAT_COMPOSE_CANDIDATE_CODE = bytes.fromhex(
    "e8 00 00 00 00 85 c0 74 2c 56 50 b9 b8 0a 00 00 "
    "e8 00 00 00 00 a1 74 09 00 00 b9 b8 0a 00 00 8b "
    "30 e8 00 00 00 00 8b 0d 74 09 00 00 50 51 ff 56 "
    "74 83 c4 08 5e c3 90 90 90 90 90 90 90 90 90 90"
)


HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_LOAD = 0x411FD6


HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_ADD = 0x411FDC


HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_VPTR = 0x411FE2


HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_CALL = 0x411FE4


HUD_UI_MGR_SENSOR_SHIELD_METER_RETAIL_ORDINAL = 5


HUD_UI_MGR_SENSOR_SHIELD_METER_DISPLACEMENT = 0x37C


HUD_UI_MGR_SENSOR_SHIELD_METER_SLOT = 0x20


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_IDENTITY = (
    "symbol:recoil:function:0x412050"
)


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_START = "0x412050"


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_END_EXCLUSIVE = "0x412070"


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.refreshcountertext"
)


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CALLER_SYMBOL = (
    "?RefreshCounterText@HudUiMgrObjective@@YIXH@Z"
)


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_LOAD = 0x412050


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_VPTR = 0x41205C


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_RECEIVER = 0x41205E


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_RECEIVER_SAVE = 0x41205F


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_CALL = 0x412061


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_NEXT_VPTR = 0x412064


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_CLEANUP = 0x412066


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_NEXT_RECEIVER = 0x412069


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_RETAIL_NEXT_CALL = 0x41206B


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_SLOT = 0x74


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_NEXT_SLOT = 0x78


HUD_UI_MGR_OBJECTIVE_REFRESH_COUNTER_CLEANUP_BYTES = 12


HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_IDENTITY = (
    "symbol:recoil:function:0x412070"
)


HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_START = "0x412070"


HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_END_EXCLUSIVE = "0x4122c0"


HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.placetrackcounterwidget"
)


HUD_UI_MGR_SENSOR_TRACK_COUNTER_CALLER_SYMBOL = (
    "?PlaceTrackCounterWidget@HudUiMgrSensor@@"
    "YIHPAUHudUiMgrSensorTrackNode@@PBUzVec3@@@Z"
)


HUD_UI_MGR_SENSOR_TRACK_COUNTER_COUNT_DISPLACEMENT = 0xEF8


HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_DISPLACEMENT = 0xEFC


HUD_UI_MGR_SENSOR_TRACK_COUNTER_ARRAY_STRIDE = 0x1C0


HUD_UI_MGR_SENSOR_TRACK_COUNTER_CAPACITY = 0x20


HUD_UI_MGR_SENSOR_TRACK_COUNTER_SLOT_DISPLACEMENT = 0x0C


HUD_UI_MGR_SENSOR_TRACK_COUNTER_STORAGE_IDENTITY = (
    "load(address(hud-ui-weapon-slot-array-element))"
)


HUD_UI_MGR_SENSOR_TRACK_COUNTER_WIDGET_STORAGE_IDENTITY = (
    "load(address(hud-ui-weapon-slot-array-element)+0x48)"
)


HUD_UI_MGR_SENSOR_TRACK_COUNTER_RETAIL_CALL = 0x4120EA


HUD_UI_MGR_SENSOR_TRACK_COUNTER_CANDIDATE_CALL_OFFSET = 0x85


HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4122c0"
)


HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_START = "0x4122c0"


HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_END_EXCLUSIVE = "0x4124b0"


HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.placetrackmarker"
)


HUD_UI_MGR_SENSOR_TRACK_MARKER_CALLER_SYMBOL = (
    "?PlaceTrackMarker@HudUiMgrSensor@@"
    "YIHHPAUPlayerProgressTargetSlotRuntime@@@Z"
)


HUD_UI_MGR_SENSOR_TRACK_MARKER_STORAGE_IDENTITY = (
    "load(address(hud-ui-weapon-slot-array-element))"
)


HUD_UI_MGR_SENSOR_TRACK_MARKER_CALL_SPECS = (
    (0, 0x9A, 0x9C, 0x9E, 0x64),
    (1, 0xAB, 0xAD, 0xAF, 0x68),
)


HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0xc20))"
)


HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_RETAIL_ORDINAL = 3


HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_LOAD_OFFSET = 0xE6


HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_RECEIVER_OFFSET = 0x116


HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_VPTR_OFFSET = 0x126


HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_CALL_OFFSET = 0x130


HUD_UI_MGR_SENSOR_TRACK_MARKER_EXIT_SLOT = 0x68


HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_RETAIL_ORDINAL = 4


HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_RECEIVER_OFFSET = 0x135


HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_VPTR_OFFSET = 0x13E


HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_CALL_OFFSET = 0x144


HUD_UI_MGR_SENSOR_TRACK_MARKER_SELECTED_SLOT = 0x64


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RETAIL_ORDINAL = 5


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0xc20)+0x104)"
)


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_FIRST_ARGUMENT_OFFSET = 0x150


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_VPTR_OFFSET = 0x155


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_SECOND_ARGUMENT_OFFSET = 0x157


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_RECEIVER_OFFSET = 0x158


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_CALL_OFFSET = 0x15A


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETPOS_SLOT = 0x0C


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RETAIL_ORDINAL = 6


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_VPTR_OFFSET = 0x15D


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_RECEIVER_OFFSET = 0x15F


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_ARGUMENT_OFFSET = 0x161


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_CALL_OFFSET = 0x163


HUD_UI_MGR_SENSOR_TRACK_MARKER_SETVISIBLE_SLOT = 0x60


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4118b0"
)


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_START = "0x4118b0"


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_END_EXCLUSIVE = "0x411900"


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.updatemeterxpoints"
)


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALLER_SYMBOL = (
    "?UpdateMeterXPoints@HudUiMgrObjective@@YIXXZ"
)


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_LOAD_OFFSET = 0x01


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_RECEIVER_OFFSET = 0x06


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_CALL_OFFSET = 0x0B


HUD_UI_MGR_OBJECTIVE_UPDATE_METER_X_SLOT_DISPLACEMENT = 0x64


HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_IDENTITY = (
    "symbol:recoil:function:0x411900"
)


HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_START = "0x411900"


HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_END_EXCLUSIVE = "0x411a20"


HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.show"
)


HUD_UI_MGR_OBJECTIVE_SHOW_CALLER_SYMBOL = (
    "?Show@HudUiMgrObjective@@YIHPAUzVidImagePartial@@PBD1M@Z"
)


HUD_UI_MGR_OBJECTIVE_SHOW_LOAD_OFFSET = 0x25


HUD_UI_MGR_OBJECTIVE_SHOW_VPTR_OFFSET = 0x2C


HUD_UI_MGR_OBJECTIVE_SHOW_CALL_OFFSET = 0x2E


HUD_UI_MGR_OBJECTIVE_SHOW_SLOT_DISPLACEMENT = 0x74


HUD_UI_MGR_OBJECTIVE_SHOW_CLEANUP_BYTES = 8


HUD_UI_MGR_OBJECTIVE_SHOW_DESC_LOAD_OFFSET = 0x31


HUD_UI_MGR_OBJECTIVE_SHOW_DESC_VPTR_OFFSET = 0x39


HUD_UI_MGR_OBJECTIVE_SHOW_DESC_CALL_OFFSET = 0x3D


HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_LOAD_OFFSET = 0x40


HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_RECEIVER_OFFSET = 0x48


HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_CALL_OFFSET = 0x4F


HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_SLOT_DISPLACEMENT = 0x60


HUD_UI_MGR_OBJECTIVE_SHOW_SENSOR_OVERLAY_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0xce0)"
)


HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_CFG_START_OFFSET = 0x9C


HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_LOAD_OFFSET = 0xA6


HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_RECEIVER_OFFSET = 0xAC


HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_CALL_OFFSET = 0xB1


HUD_UI_MGR_OBJECTIVE_SHOW_WIDGET_SLOT_DISPLACEMENT = 0x64


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_SEED_OFFSET = 0x6B


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RECEIVER_OFFSET = 0xB6


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_RESULT_STORE_OFFSET = 0xBB


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_LOAD_OFFSET = 0xC0


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_ARGUMENT_OFFSET = 0xC5


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_CALL_OFFSET = 0xC6


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_DISPLACEMENT = 0x978


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_SLOT_DISPLACEMENT = 0x60


HUD_UI_MGR_OBJECTIVE_SHOW_BAR_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0x978)"
)


HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_IDENTITY = (
    "symbol:recoil:function:0x411a20"
)


HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_START = "0x411a20"


HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_END_EXCLUSIVE = "0x411ac0"


HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.begin"
)


HUD_UI_MGR_OBJECTIVE_BEGIN_CALLER_SYMBOL = (
    "?Begin@HudUiMgrObjective@@YIXXZ"
)


HUD_UI_MGR_OBJECTIVE_BEGIN_LOAD_OFFSET = 0x17


HUD_UI_MGR_OBJECTIVE_BEGIN_VPTR_OFFSET = 0x3B


HUD_UI_MGR_OBJECTIVE_BEGIN_ARGUMENT_OFFSET = 0x3D


HUD_UI_MGR_OBJECTIVE_BEGIN_CALL_OFFSET = 0x3F


HUD_UI_MGR_OBJECTIVE_BEGIN_SLOT_DISPLACEMENT = 0x60


HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_LOAD_OFFSET = 0x42


HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_ARGUMENT_OFFSET = 0x48


HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_VPTR_OFFSET = 0x4A


HUD_UI_MGR_OBJECTIVE_BEGIN_DESC_CALL_OFFSET = 0x4C


HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_LOAD_OFFSET = 0x4F


HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_RECEIVER_OFFSET = 0x54


HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_ARGUMENT_OFFSET = 0x59


HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_CALL_OFFSET = 0x5B


HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_DISPLACEMENT = 0x768


HUD_UI_MGR_OBJECTIVE_BEGIN_SENSOR_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0x768)"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x411ac0"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_START = "0x411ac0"


HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_END_EXCLUSIVE = "0x411eb0"


HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.starthide"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_CALLER_SYMBOL = (
    "?StartHide@HudUiMgrObjective@@YIXXZ"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_DISPLACEMENT = 0x978


HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0x978)"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_LOAD_OFFSET = 0x83


HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_RECEIVER_OFFSET = 0x53


HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_CALL_OFFSET = 0x8E


HUD_UI_MGR_OBJECTIVE_START_HIDE_BAR_SLOT_DISPLACEMENT = 0x20


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_DISPLACEMENT = 0x6AC


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0x6ac)"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_FTOL_CALL_OFFSET = 0x95


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_LOAD_OFFSET = 0x9A


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_ARGUMENT_ADJUST_OFFSET = 0xA0


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_ARGUMENT_OFFSET = 0xA6


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_RECEIVER_OFFSET = 0xA1


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CALL_OFFSET = 0xA7


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_SLOT_DISPLACEMENT = 0x10


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_LOAD_OFFSET = 0xD4


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_RECEIVER_OFFSET = 0xD9


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_CALL_OFFSET = 0xDE


HUD_UI_MGR_OBJECTIVE_START_HIDE_WIDGET_CENTER_SLOT_DISPLACEMENT = 0x64


HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_SYMBOL = (
    "?DrawNoiseRect@zVid@@YIXPAUzVidRect32@@N@Z"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_DRAW_IDENTITY = (
    "symbol:recoil:function:0x48d910"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_METER_SYMBOL = (
    "?UpdateMeterXPoints@HudUiMgrObjective@@YIXXZ"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_METER_IDENTITY = (
    "symbol:recoil:function:0x4118b0"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_GET_HUD_TYPE_SYMBOL = (
    "?GetHudTypeForCurrentHwMode@zOpt@@YIHXZ"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_GET_HUD_TYPE_IDENTITY = (
    "symbol:recoil:function:0x408360"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_DIRTY_SYMBOL = (
    "?UpdateObjectiveDirtyRect@HudLayoutHW@@UAEXXZ"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_UPDATE_DIRTY_IDENTITY = (
    "symbol:recoil:function:0x4132b0"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_BEGIN_SYMBOL = (
    "?Begin@HudUiMgrObjective@@YIXXZ"
)


HUD_UI_MGR_OBJECTIVE_START_HIDE_BEGIN_IDENTITY = (
    "symbol:recoil:function:0x411a20"
)


HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x411eb0"
)


HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_START = "0x411eb0"


HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_END_EXCLUSIVE = "0x411f10"


HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.update"
)


HUD_UI_MGR_OBJECTIVE_UPDATE_CALLER_SYMBOL = (
    "?Update@HudUiMgrObjective@@YIXXZ"
)


HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALLER_SYMBOL = (
    "?SetVisibleAndResetMeterFill@HudUiMgrObjective@@YIXH@Z"
)


HUD_UI_MGR_OBJECTIVE_LABEL_DISPLACEMENT = 0x828


HUD_UI_MGR_OBJECTIVE_METER_DISPLACEMENT = 0x82C


HUD_UI_MGR_OBJECTIVE_LABEL_STORAGE_IDENTITY = (
    "load(load(storage:recoil:data:0x4e5ed0+0x828))"
)


HUD_UI_MGR_OBJECTIVE_METER_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0x82c)"
)


HUD_UI_MGR_OBJECTIVE_VISIBILITY_SLOT_DISPLACEMENT = 0x60


HUD_UI_MGR_OBJECTIVE_VISIBILITY_CALL_SPECS = (
    (0, 0x05, 0x05, 0x0C, 0x0E, 0x10, "eax", 1),
    (1, 0x13, 0x19, 0x1E, 0x13, 0x20, "edx", 1),
    (5, 0x6F, 0x6F, 0x75, 0x77, 0x79, "eax", 0),
    (6, 0x7C, 0x82, 0x87, 0x7C, 0x89, "edx", 0),
)


HUD_UI_MGR_OBJECTIVE_CANDIDATE_INVOCATION_SPECS = (
    (0, 0x10),
    (1, 0x20),
    (2, 0x27),
    (3, 0x30),
    (4, 0x3D),
    (5, 0x79),
    (6, 0x89),
)


HUD_UI_STRING_MENU_DESTRUCTOR_CANDIDATE_SYMBOL = (
    "??1HudUiStringMenu@@QAE@XZ"
)


HUD_UI_STRING_MENU_SUPERSEDED_DESTRUCTOR_CORE_SYMBOL = (
    "?DestructorCore@HudUiStringMenu@@QAEXXZ"
)


HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_IDENTITY = (
    "symbol:recoil:function:0x40fdd0"
)


HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_ADDRESS = "0x40fdd0"


HUD_UI_STRING_MENU_DESTRUCTOR_TARGET_END_EXCLUSIVE = "0x40fe30"


HUD_UI_STRING_MENU_DESTRUCTOR_CALL_ORDINAL = 18


HUD_UI_STRING_MENU_DELETE_CALL_ORDINAL = 19


HUD_UI_STRING_MENU_CALLER_SEQUENCE_START = 0xE2


HUD_UI_STRING_MENU_CALLER_SEQUENCE_END_EXCLUSIVE = 0x102


HUD_UI_STRING_MENU_AGGREGATE_DISPLACEMENT = 0xEE0


HUD_UI_STRING_MENU_CALLER_DESTRUCTOR_RELOCATION_OFFSET = 0xEF


HUD_UI_STRING_MENU_CALLER_DELETE_RELOCATION_OFFSET = 0xF5


HUD_UI_STRING_MENU_PANEL_DESTRUCTOR_SYMBOL = (
    "??1HudUiPanelSimple@@UAE@XZ"
)


HUD_UI_STRING_MENU_PANEL_DESTRUCTOR_ADDRESS = "0x40bef0"


HUD_UI_STRING_MENU_PANEL_DESTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x40bef0"
)


HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_SYMBOL = (
    "??1HudUiContainer@@QAE@XZ"
)


HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_ADDRESS = "0x4bc7b0"


HUD_UI_STRING_MENU_CONTAINER_DESTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4bc7b0"
)


HUD_UI_STRING_MENU_GLOBAL_DELETE_SYMBOL = "??3@YAXPAX@Z"


HUD_UI_STRING_MENU_GLOBAL_DELETE_ADDRESS = "0x4c5b6a"


HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_CANDIDATE_SYMBOL = (
    "??1HudUiShieldMessageWidget@@QAE@XZ"
)


HUD_UI_SHIELD_MESSAGE_SUPERSEDED_DESTRUCTOR_SYMBOL = (
    "?Destructor@HudUiShieldMessageWidget@@QAEXXZ"
)


HUD_UI_SHIELD_MESSAGE_SUPERSEDED_DESTRUCTOR_CORE_SYMBOL = (
    "?DestructorCore@HudUiShieldMessageWidget@@QAEXXZ"
)


HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_IDENTITY = (
    "symbol:recoil:function:0x40fe30"
)


HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_ADDRESS = "0x40fe30"


HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_TARGET_END_EXCLUSIVE = "0x40fe90"


HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_CODE_SIZE = 0x55


HUD_UI_SHIELD_MESSAGE_DESTRUCTOR_CALL_ORDINAL = 20


HUD_UI_SHIELD_MESSAGE_DELETE_CALL_ORDINAL = 21


HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_START = 0x102


HUD_UI_SHIELD_MESSAGE_CALLER_SEQUENCE_END_EXCLUSIVE = 0x122


HUD_UI_SHIELD_MESSAGE_AGGREGATE_DISPLACEMENT = 0xEDC


HUD_UI_SHIELD_MESSAGE_CALLER_DESTRUCTOR_RELOCATION_OFFSET = 0x10F


HUD_UI_SHIELD_MESSAGE_CALLER_DELETE_RELOCATION_OFFSET = 0x115


HUD_UI_SHIELD_MESSAGE_VFTABLE_SYMBOL = "??_7HudUiElement@@6B@"


HUD_UI_SHIELD_MESSAGE_VFTABLE_ADDRESS = "0x4cca10"


HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_SYMBOL = "??1HudUiPanel@@UAE@XZ"


HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_ADDRESS = "0x4bab40"


HUD_UI_SHIELD_MESSAGE_PANEL_DESTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4bab40"
)


HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_SYMBOL = "??1HudUiWidget@@UAE@XZ"


HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_ADDRESS = "0x4b3d50"


HUD_UI_SHIELD_MESSAGE_WIDGET_DESTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4b3d50"
)


HUD_UI_SHIELD_MESSAGE_FUNCLET_ADDRESS = "0x4c9250"


HUD_UI_SHIELD_MESSAGE_FUNCLET_SIZE = 0x15


HUD_UI_SHIELD_MESSAGE_XDATA_ADDRESS = "0x4d5360"


HUD_UI_SHIELD_MESSAGE_XDATA_SIZE = 0x28


HUD_UI_SHIELD_MESSAGE_FRAME_HANDLER_SYMBOL = "___CxxFrameHandler"


HUD_UI_SHIELD_MESSAGE_FRAME_HANDLER_ADDRESS = "0x4c60a0"


HUD_UI_MESSAGE_STACK_DESTRUCTOR_BRIDGES = {
    "??1HudUiTopMessageStack@@QAE@XZ": {
        "label": "top",
        "target_identity": "symbol:recoil:function:0x40fe90",
        "target_address": "0x40fe90",
        "target_end_exclusive": "0x40fef0",
        "call_ordinal": 28,
        "delete_ordinal": 29,
        "caller_sequence_start": 0x1A6,
        "caller_destructor_relocation_offset": 0x1B3,
        "caller_delete_relocation_offset": 0x1B9,
        "global_symbol": (
            "?g_HudUiTopMessageStack@@3PAUHudUiTextStack4@@A"
        ),
        "global_symbol_id": "recoil:data:0x56bd24",
        "global_storage_id": "recoil:storage:va:0x56bd24",
        "global_address": "0x56bd24",
        "global_navigation_name": "g_HudUiTopMessageStack",
        "anchor_id": (
            "recoil:anchor:"
            "battlesport.hud.huduitopmessagestack-destructorcore"
        ),
        "complete_type": "HudUiTopMessageStack",
        "forbidden_core": (
            "?DestructorCore@HudUiTopMessageStack@@QAEXXZ"
        ),
    },
    "??1HudUiChatMessageStack@@QAE@XZ": {
        "label": "chat",
        "target_identity": "symbol:recoil:function:0x40fef0",
        "target_address": "0x40fef0",
        "target_end_exclusive": "0x40ff50",
        "call_ordinal": 30,
        "delete_ordinal": 31,
        "caller_sequence_start": 0x1C6,
        "caller_destructor_relocation_offset": 0x1D3,
        "caller_delete_relocation_offset": 0x1D9,
        "global_symbol": (
            "?g_HudUiChatMessageStack@@3PAUHudUiTextStack4@@A"
        ),
        "global_symbol_id": "recoil:data:0x56bd20",
        "global_storage_id": "recoil:storage:va:0x56bd20",
        "global_address": "0x56bd20",
        "global_navigation_name": "g_HudUiChatMessageStack",
        "anchor_id": (
            "recoil:anchor:"
            "battlesport.hud.huduichatmessagestack-destructorcore"
        ),
        "complete_type": "HudUiChatMessageStack",
        "forbidden_core": (
            "?DestructorCore@HudUiChatMessageStack@@QAEXXZ"
        ),
    },
}


HUD_UI_MESSAGE_STACK_DESTRUCTOR_CODE_SIZE = 0x55


HUD_UI_MESSAGE_STACK_ELEMENT_COUNT = 4


HUD_UI_MESSAGE_STACK_ELEMENT_STRIDE = 0x2A4


HUD_UI_MESSAGE_STACK_ELEMENT_OFFSET = 0x10


HUD_UI_MESSAGE_STACK_SOURCE_PATH = "src/GameZRecoil/zHud/zhud_ui.h"


HUD_UI_MESSAGE_STACK_PANEL_DESTRUCTOR_SYMBOL = "??1HudUiPanel@@UAE@XZ"


HUD_UI_MESSAGE_STACK_PANEL_DESTRUCTOR_ADDRESS = "0x4bab40"


HUD_UI_MESSAGE_STACK_PANEL_DESTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4bab40"
)


HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_SYMBOL = (
    "??1HudUiContainer@@QAE@XZ"
)


HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_ADDRESS = "0x4bc7b0"


HUD_UI_MESSAGE_STACK_CONTAINER_DESTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4bc7b0"
)


HUD_UI_MESSAGE_STACK_FRAME_HANDLER_SYMBOL = "___CxxFrameHandler"


HUD_UI_MESSAGE_STACK_FRAME_HANDLER_ADDRESS = "0x4c60a0"


HUD_UI_MGR_TIMER_PANEL_SYMBOL_ID = "recoil:data:0x4ea654"


HUD_UI_MGR_TIMER_PANEL_STORAGE_ID = "recoil:storage:va:0x4ea654"


HUD_UI_MGR_TIMER_PANEL_ADDRESS = "0x4ea654"


HUD_UI_MGR_TIMER_PANEL_STORAGE_IDENTITY = (
    "storage:recoil:data:0x4ea654"
)


HUD_UI_MGR_TIMER_PANEL_TARGET_ID = (
    "recoil:vc5-target:hud_ui_timer_panel_global_accessors_data"
)


HUD_UI_MGR_TIMER_PANEL_DISPLACEMENT = 0x4784


HUD_UI_MGR_TIMER_PANEL_ACCESS_WIDTH = 4


HUD_UI_MGR_TIMER_PANEL_FLOAT_DISPLACEMENT = 0x4788


HUD_UI_MGR_TIMER_PANEL_FLOAT_ACCESS_WIDTH = 4


HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_DISPLACEMENT = 0xBC8


HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ADDRESS = "0x4e6a98"


HUD_UI_MGR_OBJECTIVE_COUNTER_PANEL_ACCESS_WIDTH = 4


HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_ORDINAL = 22


HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_LOAD_OFFSET = 0x122


HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_VPTR_OFFSET = 0x12C


HUD_UI_MGR_OBJECTIVE_COUNTER_DELETE_CALL_OFFSET = 0x130


HUD_LAYOUT_HW_SET_ACTIVE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4130d0"
)


HUD_LAYOUT_HW_SET_ACTIVE_CALLER_START = "0x4130d0"


HUD_LAYOUT_HW_SET_ACTIVE_CALLER_END_EXCLUSIVE = "0x4132b0"


HUD_LAYOUT_HW_SET_ACTIVE_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud.hudlayouthw-setactive"
)


HUD_LAYOUT_HW_SET_ACTIVE_CALLER_SYMBOL = (
    "?SetActive@HudLayoutHW@@UAEHH@Z"
)


HUD_LAYOUT_HW_SET_ACTIVE_CANDIDATE_SIZE = 0x1D0


HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_BODY_END = 0x1D1


HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_CANDIDATE_SIZE = 0x1E0


HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_LEAD_START = 0x40


HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_LEAD_CALL_OFFSET = 0x44


HUD_LAYOUT_HW_SET_ACTIVE_ACTIVE_FIRST_LEAD_END = 0x47


HUD_LAYOUT_HW_SET_ACTIVE_NANITE_DIRECT_SYMBOL = (
    "?SetBltSourceAndClipRect@HudUiElement@@UAEXPAXPBUHudUiRect@@@Z"
)


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_LOAD_OFFSET = 0x40


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RELOCATION_OFFSET = 0x42


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_FIRST_ZERO_OFFSET = 0x46


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SECOND_ZERO_OFFSET = 0x48


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_VPTR_OFFSET = 0x4A


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CALL_OFFSET = 0x4C


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_SLOT = 0x18


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_CANDIDATE_CALL_ORDINAL = 4


HUD_LAYOUT_HW_SET_ACTIVE_OBJECTIVE_RETAIL_CALL_ORDINAL = 5


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_LOAD_OFFSET = 0x4F


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_RELOCATION_OFFSET = 0x51


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_FIRST_ZERO_OFFSET = 0x55


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SECOND_ZERO_OFFSET = 0x57


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_VPTR_OFFSET = 0x59


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CALL_OFFSET = 0x5B


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_SLOT = 0x18


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_CANDIDATE_CALL_ORDINAL = 5


HUD_LAYOUT_HW_SET_ACTIVE_TIMER_RETAIL_CALL_ORDINAL = 6


HUD_LAYOUT_HW_SET_ACTIVE_NANITE_PANEL_DISPLACEMENT = 0x420


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4132b0"
)


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_START = "0x4132b0"


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_END_EXCLUSIVE = "0x413340"


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_SYMBOL = (
    "?UpdateObjectiveDirtyRect@HudLayoutHW@@UAEXXZ"
)


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud."
    "hudlayouthw-updateobjectivedirtyrect"
)


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_DISPLACEMENT = 0x6AC


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_ADDRESS = "0x4e657c"


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_WIDGET_STORAGE_IDENTITY = (
    "load(storage:recoil:data:0x4e5ed0+0x6ac)"
)


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_RETAIL_CALL_X = "0x4132d2"


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_RETAIL_CALL_Y = "0x4132f0"


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_RETAIL_CODE_END_EXCLUSIVE = "0x413339"


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CANDIDATE_CALL_X = 0x25


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_CANDIDATE_CALL_Y = 0x35


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_INVALIDATE_SYMBOL = (
    "?InvalidateRect@HudUiWidget@@QAEXPBUHudUiRect@@@Z"
)


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_ADDRESS = "0x4b4180"


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_IDENTITY = (
    "symbol:recoil:function:0x4b4180"
)


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_INVALIDATE_SYMBOL = (
    "?Invalidate@HudUiElement@@UAEXXZ"
)


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_ADDRESS = "0x40f400"


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_IDENTITY = (
    "symbol:recoil:function:0x40f400"
)


HUD_LAYOUT_HW_UPDATE_OBJECTIVE_DIRTY_DIRECT_DRAW_SYMBOL = (
    "?Draw@HudUiTripletPanel@@UAEXXZ"
)


HUD_UI_MGR_REMAINING_DELETE_PROVENANCE_TABLE = (
    (
        "StatsList",
        24,
        0x14E,
        0x7610,
        "edx",
        "0x4ed4e0",
        "storage:recoil:data:0x4ed4e0",
    ),
    (
        "ObjectiveSummary",
        25,
        0x164,
        0x824,
        "eax",
        "0x4e66f4",
        "",
    ),
    (
        "ObjectiveDesc",
        26,
        0x17A,
        0x974,
        "edx",
        "0x4e6844",
        "",
    ),
    (
        "ObjectiveLabel",
        27,
        0x190,
        0x828,
        "eax",
        "0x4e66f8",
        "",
    ),
)


HUD_UI_MGR_STATS_LIST_SYMBOL_ID = "recoil:data:0x4ed4e0"


HUD_UI_MGR_STATS_LIST_STORAGE_ID = "recoil:storage:va:0x4ed4e0"


HUD_UI_MGR_STATS_LIST_ADDRESS = "0x4ed4e0"


HUD_UI_MGR_STATS_LIST_NAME = "g_HudUiMgrStatsList"


HUD_UI_MGR_STATS_LIST_IDENTITY = "storage:recoil:data:0x4ed4e0"


HUD_UI_MGR_STATS_LIST_DISPLACEMENT = 0x7610


HUD_UI_MGR_STATS_LIST_ACCESS_WIDTH = 4


HUD_UI_MGR_OWNER_ID = "recoil:owner:hud_ui.hud_ui_mgr_data"


HUD_UI_MGR_STATS_LIST_REFERENCE_OFFSET = 0


HUD_UI_MGR_STATS_LIST_RELOCATION_OFFSET = 2


HUD_UI_MGR_STATS_LIST_RETAIL_LOAD_ADDRESS = 0x40F99F


HUD_UI_MGR_STATS_LIST_RETAIL_ARGUMENT_SEED_ADDRESS = 0x40F7B5


HUD_UI_MGR_STATS_LIST_RETAIL_FLAG_ADDRESS = 0x40F9A5


HUD_UI_MGR_STATS_LIST_RETAIL_ARGUMENT_ADDRESS = 0x40F9AB


HUD_UI_MGR_STATS_LIST_RETAIL_VPTR_LOAD_ADDRESS = 0x40F9AC


HUD_UI_MGR_STATS_LIST_RETAIL_CALL_ADDRESS = 0x40F9AE


HUD_UI_MGR_STATS_LIST_VPTR_SLOT_DISPLACEMENT = 0x60


HUD_SHIELD_LAYOUT_CALLER_IDENTITY = "symbol:recoil:function:0x40eb00"


HUD_SHIELD_LAYOUT_CALLER_START = "0x40eb00"


HUD_SHIELD_LAYOUT_CALLER_END_EXCLUSIVE = "0x40ec90"


HUD_SHIELD_LAYOUT_CALLER_SYMBOL = (
    "?ApplyLayout@HudUiShieldMessageWidget@@SGHPAUNode@zReader@@@Z"
)


HUD_SHIELD_LAYOUT_POINTER_ADDRESS = "0x4e6dac"


HUD_SHIELD_LAYOUT_POINTER_DISPLACEMENT = 0xEDC


HUD_SHIELD_LAYOUT_POINTER_ACCESS_WIDTH = 4


HUD_SHIELD_LAYOUT_POINTER_IDENTITY = (
    "storage:recoil:data:0x4e5ed0+0xedc"
)


HUD_SHIELD_LAYOUT_RETAIL_LOADS = {
    "0x40eb03": ("a1", "ac", "6d", "4e", "00"),
    "0x40eb32": ("8b", "15", "ac", "6d", "4e", "00"),
    "0x40eb41": ("8b", "0d", "ac", "6d", "4e", "00"),
    "0x40eb53": ("8b", "0d", "ac", "6d", "4e", "00"),
    "0x40eb77": ("8b", "15", "ac", "6d", "4e", "00"),
    "0x40eb90": ("8b", "0d", "ac", "6d", "4e", "00"),
    "0x40ebb6": ("a1", "ac", "6d", "4e", "00"),
    "0x40ebce": ("8b", "0d", "ac", "6d", "4e", "00"),
    "0x40ebe9": ("a1", "ac", "6d", "4e", "00"),
    "0x40ec00": ("8b", "15", "ac", "6d", "4e", "00"),
    "0x40ec25": ("a1", "ac", "6d", "4e", "00"),
    "0x40ec3e": ("8b", "0d", "ac", "6d", "4e", "00"),
    "0x40ec52": ("8b", "15", "ac", "6d", "4e", "00"),
    "0x40ec69": ("a1", "ac", "6d", "4e", "00"),
}


HUD_SHIELD_LAYOUT_ADD_CHILD_SYMBOL = (
    "?AddChild@HudUiContainer@@QAEHPAUHudUiElement@@@Z"
)


HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40d7e0"
)


HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_CALLER_START = "0x40d7e0"


HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_CALLER_END_EXCLUSIVE = "0x40d9d0"


HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_CALLER_SYMBOL = "??0HudUiMgrData@@QAE@XZ"


HUD_TEXT_INPUT_CONSTRUCTOR_ALIAS_SYMBOL = (
    "?Constructor@HudUiTextInput@@QAEPAU1@H@Z"
)


HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_SYMBOL = "??0HudUiTextInput@@QAE@H@Z"


HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_IDENTITY = (
    "symbol:recoil:function:0x4b42f0"
)


HUD_TEXT_INPUT_CONSTRUCTOR_TARGET_ADDRESS = "0x4b42f0"


HUD_TEXT_INPUT_CONSTRUCTOR_RETAIL_ORDINAL = 8


HUD_CONFIRM_QUIT_BASE_CONSTRUCTOR_SYMBOL = "??0HudUiZrdWidget@@QAE@XZ"


HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_IDENTITY = (
    "symbol:recoil:function:0x419aa0"
)


HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_SYMBOL_ID = "recoil:function:0x419aa0"


HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_START = "0x419aa0"


HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_END_EXCLUSIVE = "0x41a160"


HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_SYMBOL = (
    "??0HudUiNetGameSetupPanel@@QAE@H@Z"
)


HUD_NET_GAME_SETUP_CONSTRUCTOR_CALLER_BLOCK_ID = "recoil:block:0x417350"


HUD_NET_GAME_SETUP_LAUNCH_BUTTON_CONSTRUCTOR_SYMBOL = (
    "??0HudUiNetGameSetupPanel_LaunchButton@@QAE@XZ"
)


HUD_NET_GAME_SETUP_LAUNCH_BUTTON_VFTABLE_SYMBOL = (
    "??_7HudUiNetGameSetupPanel_LaunchButton@@6B@"
)


HUD_NET_GAME_SETUP_LAUNCH_BUTTON_RETAIL_ORDINAL = 1


HUD_NET_GAME_SETUP_LAUNCH_BUTTON_CALL_OFFSET = 0x35


HUD_NET_GAME_SETUP_CANCEL_BUTTON_CONSTRUCTOR_SYMBOL = (
    "??0HudUiNetGameSetupPanel_CancelButton@@QAE@XZ"
)


HUD_NET_GAME_SETUP_CANCEL_BUTTON_VFTABLE_SYMBOL = (
    "??_7HudUiNetGameSetupPanel_CancelButton@@6B@"
)


HUD_NET_GAME_SETUP_CANCEL_BUTTON_RETAIL_ORDINAL = 2


HUD_NET_GAME_SETUP_CANCEL_BUTTON_CALL_OFFSET = 0x45


HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SYMBOL = (
    "??0HudUiNumericTextInput@@QAE@XZ"
)


HUD_NET_GAME_SETUP_NUMERIC_WRAPPER_SYMBOL = (
    "?BaseConstructor@HudUiNumericTextInput@@QAEPAU1@XZ"
)


HUD_NET_GAME_SETUP_NUMERIC_RETAIL_IDENTITY = (
    "symbol:recoil:function:0x41a190"
)


HUD_NET_GAME_SETUP_NUMERIC_RETAIL_SYMBOL_ID = "recoil:function:0x41a190"


HUD_NET_GAME_SETUP_NUMERIC_RETAIL_ORDINAL = 3


HUD_NET_GAME_SETUP_NUMERIC_CALL_OFFSET = 0x57


HUD_NET_GAME_SETUP_WORLD_SELECTOR_LOCAL_CONSTRUCTOR_SYMBOL = (
    "??0HudUiNetGameSetupPanel_WorldSelector@@QAE@XZ"
)


HUD_NET_GAME_SETUP_WORLD_SELECTOR_VFTABLE_SYMBOL = (
    "??_7HudUiNetGameSetupPanel_WorldSelector@@6B@"
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_SYMBOL = (
    "??0HudUiCycleSelectorWidget@@QAE@XZ"
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4b7d60"
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_SYMBOL_ID = (
    "recoil:function:0x4b7d60"
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_RETAIL_ORDINAL = 4


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CALL_OFFSET = 0x70


HUD_NET_GAME_SETUP_NEXT_WORLD_LOCAL_CONSTRUCTOR_SYMBOL = (
    "??0HudUiNetGameSetupPanel_NextWorldButton@@QAE@XZ"
)


HUD_NET_GAME_SETUP_NEXT_WORLD_VFTABLE_SYMBOL = (
    "??_7HudUiNetGameSetupPanel_NextWorldButton@@6B@"
)


HUD_NET_GAME_SETUP_NEXT_WORLD_RETAIL_ORDINAL = 5


HUD_NET_GAME_SETUP_NEXT_WORLD_CALL_OFFSET = 0x88


HUD_NET_GAME_SETUP_NEXT_WORLD_CALL_RELOCATION_OFFSET = 0x89


HUD_NET_GAME_SETUP_NEXT_WORLD_VFTABLE_INSTRUCTION_OFFSET = 0x8D


HUD_NET_GAME_SETUP_NEXT_WORLD_VFTABLE_RELOCATION_OFFSET = 0x8F


HUD_NET_GAME_SETUP_PREV_WORLD_LOCAL_CONSTRUCTOR_SYMBOL = (
    "??0HudUiNetGameSetupPanel_PrevWorldButton@@QAE@XZ"
)


HUD_NET_GAME_SETUP_PREV_WORLD_VFTABLE_SYMBOL = (
    "??_7HudUiNetGameSetupPanel_PrevWorldButton@@6B@"
)


HUD_NET_GAME_SETUP_PREV_WORLD_RETAIL_ORDINAL = 6


HUD_NET_GAME_SETUP_PREV_WORLD_CALL_OFFSET = 0xA0


HUD_NET_GAME_SETUP_PREV_WORLD_CALL_RELOCATION_OFFSET = 0xA1


HUD_NET_GAME_SETUP_PREV_WORLD_VFTABLE_INSTRUCTION_OFFSET = 0xA5


HUD_NET_GAME_SETUP_PREV_WORLD_VFTABLE_RELOCATION_OFFSET = 0xA7


HUD_NET_GAME_SETUP_CLAMPED_INPUT_CONSTRUCTOR_SYMBOL = (
    "??0HudUiClampedIntTextInput@@QAE@I@Z"
)


HUD_NET_GAME_SETUP_CLAMPED_INPUT_CONSTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x41a200"
)


HUD_NET_GAME_SETUP_CLAMPED_INPUT_CONSTRUCTOR_SYMBOL_ID = (
    "recoil:function:0x41a200"
)


HUD_NET_GAME_SETUP_INLINE_INPUT_SPECS = (
    (
        "CHudUiNetGameSetupPanelTimeLimitInput",
        "??0CHudUiNetGameSetupPanelTimeLimitInput@@QAE@XZ",
        "??_7CHudUiNetGameSetupPanelTimeLimitInput@@6B@",
        4,
        7,
        HUD_NET_GAME_SETUP_NUMERIC_RETAIL_IDENTITY,
        0xAB,
        0xB1,
        0xBA,
        0xBB,
        0xBF,
        0xC1,
    ),
    (
        "CHudUiNetGameSetupPanelKillsInput",
        "??0CHudUiNetGameSetupPanelKillsInput@@QAE@XZ",
        "??_7CHudUiNetGameSetupPanelKillsInput@@6B@",
        2,
        10,
        HUD_NET_GAME_SETUP_CLAMPED_INPUT_CONSTRUCTOR_IDENTITY,
        0xF5,
        0xFB,
        0x104,
        0x105,
        0x109,
        0x10B,
    ),
    (
        "CHudUiNetGameSetupPanelMaxPlayersInput",
        "??0CHudUiNetGameSetupPanelMaxPlayersInput@@QAE@XZ",
        "??_7CHudUiNetGameSetupPanelMaxPlayersInput@@6B@",
        2,
        13,
        HUD_NET_GAME_SETUP_CLAMPED_INPUT_CONSTRUCTOR_IDENTITY,
        0x144,
        0x14A,
        0x14E,
        0x14F,
        0x153,
        0x155,
    ),
)


HUD_NET_GAME_SETUP_STEP_BUTTON_SPECS = (
    (
        "HudUiNetGameSetupPanel_IncTimeLimitButton",
        "??0HudUiNetGameSetupPanel_IncTimeLimitButton@@QAE@XZ",
        "??_7HudUiNetGameSetupPanel_IncTimeLimitButton@@6B@",
        8,
        0xC5,
        0xD2,
        0xD3,
        0xD7,
        0xD9,
    ),
    (
        "HudUiNetGameSetupPanel_DecTimeLimitButton",
        "??0HudUiNetGameSetupPanel_DecTimeLimitButton@@QAE@XZ",
        "??_7HudUiNetGameSetupPanel_DecTimeLimitButton@@6B@",
        9,
        0xDD,
        0xEA,
        0xEB,
        0xEF,
        0xF1,
    ),
    (
        "HudUiNetGameSetupPanel_IncKillsButton",
        "??0HudUiNetGameSetupPanel_IncKillsButton@@QAE@XZ",
        "??_7HudUiNetGameSetupPanel_IncKillsButton@@6B@",
        11,
        0x10F,
        0x11C,
        0x11D,
        0x121,
        0x123,
    ),
    (
        "HudUiNetGameSetupPanel_DecKillsButton",
        "??0HudUiNetGameSetupPanel_DecKillsButton@@QAE@XZ",
        "??_7HudUiNetGameSetupPanel_DecKillsButton@@6B@",
        12,
        0x127,
        0x134,
        0x135,
        0x139,
        0x13B,
    ),
    (
        "HudUiNetGameSetupPanel_IncMaxPlayersButton",
        "??0HudUiNetGameSetupPanel_IncMaxPlayersButton@@QAE@XZ",
        "??_7HudUiNetGameSetupPanel_IncMaxPlayersButton@@6B@",
        14,
        0x159,
        0x166,
        0x167,
        0x16B,
        0x16D,
    ),
    (
        "HudUiNetGameSetupPanel_DecMaxPlayersButton",
        "??0HudUiNetGameSetupPanel_DecMaxPlayersButton@@QAE@XZ",
        "??_7HudUiNetGameSetupPanel_DecMaxPlayersButton@@6B@",
        15,
        0x171,
        0x17E,
        0x17F,
        0x183,
        0x185,
    ),
)


HUD_NET_GAME_SETUP_CHECK_TOGGLE_CONSTRUCTOR_SYMBOL = (
    "??0HudUiCheckToggleWidget@@QAE@XZ"
)


HUD_NET_GAME_SETUP_CHECK_TOGGLE_CONSTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4b6fc0"
)


HUD_NET_GAME_SETUP_CHECK_TOGGLE_CONSTRUCTOR_SYMBOL_ID = (
    "recoil:function:0x4b6fc0"
)


HUD_NET_GAME_SETUP_KILLS_SWITCH_CONSTRUCTOR_SYMBOL = (
    "??0HudUiWidget@@QAE@I@Z"
)


HUD_NET_GAME_SETUP_KILLS_SWITCH_CONSTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4b3d00"
)


HUD_NET_GAME_SETUP_KILLS_SWITCH_CONSTRUCTOR_SYMBOL_ID = (
    "recoil:function:0x4b3d00"
)


HUD_NET_GAME_SETUP_LOAD_FROM_ZRD_SYMBOL = (
    "?LoadFromZrd@HudUiBackground@@QAEPAUNode@zReader@@PBD0H@Z"
)


HUD_NET_GAME_SETUP_LOAD_FROM_ZRD_IDENTITY = (
    "symbol:recoil:function:0x4b98d0"
)


HUD_NET_GAME_SETUP_LOAD_FROM_ZRD_ORDINAL = 20


HUD_NET_GAME_SETUP_LOAD_FROM_ZRD_CALL_OFFSET = 0x1F6


HUD_NET_GAME_SETUP_LOAD_FROM_ZRD_CALL_RELOCATION_OFFSET = 0x1F7


HUD_NET_GAME_SETUP_LOAD_FROM_ZRD_MODE_LITERAL = (
    "??_C@_0M@KEOC@MP_NEW_GAME?$AA@"
)


HUD_NET_GAME_SETUP_LOAD_FROM_ZRD_PATH_LITERAL = (
    "??_C@_0L@NPAH@dialog?4zrd?$AA@"
)


HUD_NET_GAME_SETUP_PANEL_VFTABLE_SYMBOL = (
    "??_7HudUiNetGameSetupPanel@@6B@"
)


HUD_NET_GAME_SETUP_BIND_PRIMITIVE_SYMBOL = (
    "?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@"
    "PAUHudUiElement@@PBD@Z"
)


HUD_NET_GAME_SETUP_BIND_PRIMITIVE_IDENTITY = (
    "symbol:recoil:function:0x4ba0e0"
)


HUD_NET_GAME_SETUP_BIND_PRIMITIVE_ORDINAL = 21


HUD_NET_GAME_SETUP_BIND_PRIMITIVE_CALL_OFFSET = 0x26D


HUD_NET_GAME_SETUP_BIND_PRIMITIVE_CALL_RELOCATION_OFFSET = 0x26E


HUD_NET_GAME_SETUP_KILLS_SWITCH_LITERAL = (
    "??_C@_0N@BGIN@KILLS_SWITCH?$AA@"
)


HUD_NET_GAME_SETUP_KILLS_VISIBILITY_ORDINAL = 41


HUD_NET_GAME_SETUP_KILLS_VISIBILITY_VPTR_LOAD_OFFSET = 0x3DB


HUD_NET_GAME_SETUP_KILLS_VISIBILITY_CALL_OFFSET = 0x3E8


HUD_NET_GAME_SETUP_KILLS_VISIBILITY_STORAGE = "load(this+0xc930)"


HUD_NET_GAME_SETUP_KILLS_VISIBILITY_SLOT_DISPLACEMENT = 96


HUD_NET_GAME_SETUP_WORLD_SELECTOR_SET_INDEX_SYMBOL = (
    "?SetIndexClamped@HudUiCycleSelectorWidget@@QAEHH@Z"
)


HUD_NET_GAME_SETUP_WORLD_SELECTOR_SET_INDEX_IDENTITY = (
    "symbol:recoil:function:0x4b7f20"
)


HUD_NET_GAME_SETUP_WORLD_SELECTOR_SET_INDEX_ORDINAL = 43


HUD_NET_GAME_SETUP_WORLD_SELECTOR_SET_INDEX_CALL_OFFSET = 0x404


HUD_NET_GAME_SETUP_WORLD_SELECTOR_SET_INDEX_RELOCATION_OFFSET = 0x405


HUD_NET_GAME_SETUP_PLAYER_NAME_GETTER_SYMBOL = "?zOptGetPlayerName@@YIPADXZ"


HUD_NET_GAME_SETUP_PLAYER_NAME_GETTER_IDENTITY = (
    "symbol:recoil:function:0x408190"
)


HUD_NET_GAME_SETUP_PLAYER_NAME_GETTER_ORDINAL = 44


HUD_NET_GAME_SETUP_PLAYER_NAME_GETTER_CALL_OFFSET = 0x409


HUD_NET_GAME_SETUP_PLAYER_NAME_GETTER_RELOCATION_OFFSET = 0x40A


HUD_NET_GAME_SETUP_SPRINTF_SYMBOL = "__imp__sprintf"


HUD_NET_GAME_SETUP_SPRINTF_IDENTITY = "iat:sprintf"


HUD_NET_GAME_SETUP_SPRINTF_ORDINAL = 45


HUD_NET_GAME_SETUP_SPRINTF_LOAD_OFFSET = 0x419


HUD_NET_GAME_SETUP_SPRINTF_RELOCATION_OFFSET = 0x41B


HUD_NET_GAME_SETUP_SPRINTF_CALL_OFFSET = 0x419


HUD_NET_GAME_SETUP_SPRINTF_CLEANUP_OFFSET = 0x41F


HUD_NET_GAME_SETUP_SPRINTF_CLEANUP_BYTES = 12


HUD_NET_GAME_SETUP_RETAINED_SPRINTF_LOAD_OFFSET = 0x40E


HUD_NET_GAME_SETUP_RETAINED_SPRINTF_RELOCATION_OFFSET = 0x410


HUD_NET_GAME_SETUP_RETAINED_SPRINTF_CALL_OFFSET = 0x41F


HUD_NET_GAME_SETUP_RETAINED_SPRINTF_CLEANUP_OFFSET = 0x421


HUD_NET_GAME_SETUP_FORMATTED_UPDATE_SYMBOL = (
    "?Update@HudUiNumericTextInput@@QAEXPBD@Z"
)


HUD_NET_GAME_SETUP_FORMATTED_UPDATE_IDENTITY = (
    "symbol:recoil:function:0x4b4e60"
)


HUD_NET_GAME_SETUP_FORMATTED_UPDATE_ORDINAL = 46


HUD_NET_GAME_SETUP_FORMATTED_UPDATE_ARGUMENT_OFFSET = 0x422


HUD_NET_GAME_SETUP_FORMATTED_UPDATE_PUSH_OFFSET = 0x426


HUD_NET_GAME_SETUP_FORMATTED_UPDATE_THIS_OFFSET = 0x427


HUD_NET_GAME_SETUP_FORMATTED_UPDATE_CALL_OFFSET = 0x429


HUD_NET_GAME_SETUP_FORMATTED_UPDATE_RELOCATION_OFFSET = 0x42A


HUD_NET_GAME_SETUP_WIDGET_ENABLE_ORDINAL = 48


HUD_NET_GAME_SETUP_WIDGET_ENABLE_VPTR_LOAD_OFFSET = 0x43D


HUD_NET_GAME_SETUP_WIDGET_ENABLE_CALL_OFFSET = 0x453


HUD_NET_GAME_SETUP_WIDGET_ENABLE_STORAGE = (
    "load(address(this+0xabe8))"
)


HUD_NET_GAME_SETUP_WIDGET_ENABLE_SLOT_DISPLACEMENT = 120


HUD_NET_GAME_SETUP_TIME_LIMIT_SPRINTF_ORDINAL = 49


HUD_NET_GAME_SETUP_TIME_LIMIT_SPRINTF_CALL_OFFSET = 0x47C


HUD_NET_GAME_SETUP_TIME_LIMIT_SPRINTF_RELOCATION_OFFSET = 0x47E


HUD_NET_GAME_SETUP_TIME_LIMIT_SPRINTF_CLEANUP_OFFSET = 0x482


HUD_NET_GAME_SETUP_TIME_LIMIT_SPRINTF_CLEANUP_BYTES = 12


HUD_NET_GAME_SETUP_KILLS_SPRINTF_ORDINAL = 51


HUD_NET_GAME_SETUP_KILLS_SPRINTF_CALL_OFFSET = 0x4B1


HUD_NET_GAME_SETUP_KILLS_SPRINTF_RELOCATION_OFFSET = 0x4B3


HUD_NET_GAME_SETUP_KILLS_SPRINTF_CLEANUP_OFFSET = 0x4B7


HUD_NET_GAME_SETUP_KILLS_SPRINTF_CLEANUP_BYTES = 12


HUD_NET_GAME_SETUP_MODEM_BRANCH_ORDINAL = 53


HUD_NET_GAME_SETUP_MODEM_BRANCH_CALL_OFFSET = 0x4CA


HUD_NET_GAME_SETUP_MODEM_BRANCH_RELOCATION_OFFSET = 0x4CB


HUD_NET_GAME_SETUP_MODEM_BRANCH_IDENTITY = "symbol:recoil:function:0x408270"


HUD_NET_GAME_SETUP_MAX_PLAYERS_REFRESH_ORDINAL = 54


HUD_NET_GAME_SETUP_MAX_PLAYERS_REFRESH_RECEIVER_OFFSET = 0x144


HUD_NET_GAME_SETUP_MAX_PLAYERS_REFRESH_VPTR_LOAD_OFFSET = 0x4D3


HUD_NET_GAME_SETUP_MAX_PLAYERS_REFRESH_THIS_OFFSET = 0x4D5


HUD_NET_GAME_SETUP_MAX_PLAYERS_REFRESH_STORE_OFFSET = 0x4D7


HUD_NET_GAME_SETUP_MAX_PLAYERS_REFRESH_CALL_OFFSET = 0x4E1


HUD_NET_GAME_SETUP_MAX_PLAYERS_REFRESH_STORAGE = "load(this+0xc044)"


HUD_NET_GAME_SETUP_MAX_PLAYERS_REFRESH_SLOT_DISPLACEMENT = 120


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_ORDINAL = 55


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_VPTR_LOAD_OFFSET = 0x4E4


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_RECEIVER_OFFSET = 0x4EA


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_ZERO_OFFSET = 0x4F0


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_STORE_OFFSET = 0x4F2


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_CALL_OFFSET = 0x4F8


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_CANDIDATE_STORAGE = "load(this+0xc3c0)"


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_RETAIL_STORAGE = (
    "load(address(this+0xc3c0))"
)


HUD_NET_GAME_SETUP_INC_MAX_REFRESH_SLOT_DISPLACEMENT = 120


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_ORDINAL = 61


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_ENABLED_OFFSET = 0x583


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_RECEIVER_OFFSET = 0x587


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_MIN_STORE_OFFSET = 0x58D


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_SENTINEL_OFFSET = 0x593


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_MAX_STORE_OFFSET = 0x596


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_VPTR_LOAD_OFFSET = 0x59C


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_STATE_STORE_OFFSET = 0x59E


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_CALL_OFFSET = 0x5A4


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_STORAGE = "load(this+0xc514)"


HUD_NET_GAME_SETUP_ACTUAL_MAX_PLAYERS_REFRESH_SLOT_DISPLACEMENT = 120


HUD_NET_GAME_SETUP_TOGGLE_SPECS = (
    (
        "HudUiNetGameSetupPanel_AllowMapsToggle",
        "??0HudUiNetGameSetupPanel_AllowMapsToggle@@QAE@XZ",
        "??_7HudUiNetGameSetupPanel_AllowMapsToggle@@6B@",
        "recoil:data:0x4cf1d0",
        "0x4cf1d0",
        "g_HudUiCheckToggleWidget_AllowMaps_Vtbl",
        16,
        0x189,
        0x196,
        0x197,
        0x19B,
        0x19D,
    ),
    (
        "HudUiNetGameSetupPanel_NameTagsToggle",
        "??0HudUiNetGameSetupPanel_NameTagsToggle@@QAE@XZ",
        "??_7HudUiNetGameSetupPanel_NameTagsToggle@@6B@",
        "recoil:data:0x4cf148",
        "0x4cf148",
        "g_HudUiCheckToggleWidget_NameTags_Vtbl",
        17,
        0x1A1,
        0x1AE,
        0x1AF,
        0x1B3,
        0x1B5,
    ),
)


HUD_NET_GAME_SETUP_TOGGLE_VFTABLE_COMMON_RELOCATIONS = (
    (0x04, IMAGE_REL_I386_DIR32, '?Draw@HudUiWidget@@UAEXXZ'),
    (0x08, IMAGE_REL_I386_DIR32, '?DrawBase@HudUiElement@@UAEXXZ'),
    (0x0C, IMAGE_REL_I386_DIR32, '?SetPos@HudUiWidget@@UAEXHH@Z'),
    (0x10, IMAGE_REL_I386_DIR32, '?SetX@HudUiElement@@UAEXH@Z'),
    (0x14, IMAGE_REL_I386_DIR32, '?SetY@HudUiElement@@UAEXH@Z'),
    (
        0x18,
        IMAGE_REL_I386_DIR32,
        '?SetBltSourceAndClipRect@HudUiElement@@UAEXPAXPBUHudUiRect@@@Z',
    ),
    (
        0x1C,
        IMAGE_REL_I386_DIR32,
        '?SetClipRect@HudUiElement@@UAEXPBUHudUiRect@@@Z',
    ),
    (0x20, IMAGE_REL_I386_DIR32, '?Invalidate@HudUiZrdWidget@@UAEXXZ'),
    (0x24, IMAGE_REL_I386_DIR32, '?Update@HudUiElement@@UAEXM@Z'),
    (0x28, IMAGE_REL_I386_DIR32, '?OnUpdateIdle@HudUiElement@@UAEXM@Z'),
    (
        0x2C,
        IMAGE_REL_I386_DIR32,
        '?GetBoundsRectOrNull@HudUiCheckToggleWidget@@UAEPAUHudUiRect@@XZ',
    ),
    (
        0x30,
        IMAGE_REL_I386_DIR32,
        '?OnActivate@HudUiCheckToggleWidget@@UAEXXZ',
    ),
    (0x34, IMAGE_REL_I386_DIR32, '?OnClearBinding@HudUiElement@@UAEXXZ'),
    (0x38, IMAGE_REL_I386_DIR32, '?OnHoverRepeat@HudUiElement@@UAEXXZ'),
    (
        0x3C,
        IMAGE_REL_I386_DIR32,
        '?ShowPreview@HudUiCheckToggleWidget@@UAEXXZ',
    ),
    (
        0x40,
        IMAGE_REL_I386_DIR32,
        '?HidePreview@HudUiCheckToggleWidget@@UAEXXZ',
    ),
    (0x44, IMAGE_REL_I386_DIR32, '?OnBeginCapture@HudUiElement@@UAEXXZ'),
    (0x48, IMAGE_REL_I386_DIR32, '?OnEndCapture@HudUiElement@@UAEXXZ'),
    (
        0x4C,
        IMAGE_REL_I386_DIR32,
        '?OnPointerButtonState@HudUiElement@@UAEXHH@Z',
    ),
    (
        0x50,
        IMAGE_REL_I386_DIR32,
        '?OnCapturedPrimaryRelease@HudUiElement@@UAEXXZ',
    ),
    (
        0x54,
        IMAGE_REL_I386_DIR32,
        '?ShouldHandleInput@HudUiElement@@UAEHPAUHudUiBackground@@H@Z',
    ),
    (
        0x58,
        IMAGE_REL_I386_DIR32,
        '?AfterInputUpdate@HudUiElement@@UAEXPAUHudUiBackground@@H@Z',
    ),
    (0x5C, IMAGE_REL_I386_DIR32, '?HitTest@HudUiWidget@@UAEHHH@Z'),
    (0x60, IMAGE_REL_I386_DIR32, '?SetVisible@HudUiElement@@UAEXH@Z'),
    (0x64, IMAGE_REL_I386_DIR32, '?GetCenterX@HudUiWidget@@UAEHXZ'),
    (0x68, IMAGE_REL_I386_DIR32, '?GetCenterY@HudUiWidget@@UAEHXZ'),
    (
        0x6C,
        IMAGE_REL_I386_DIR32,
        '?EnableWordWrapWithRect@HudUiElement@@UAEXPBUHudUiRect@@@Z',
    ),
    (
        0x70,
        IMAGE_REL_I386_DIR32,
        '?GetTextRect@HudUiElement@@UAEXPAUHudUiRect@@@Z',
    ),
    (
        0x74,
        IMAGE_REL_I386_DIR32,
        '?RebuildBltRectFromImage@HudUiWidget@@UAEXXZ',
    ),
    (
        0x78,
        IMAGE_REL_I386_DIR32,
        '?RefreshState@HudUiCheckToggleWidget@@UAEXXZ',
    ),
    (
        0x7C,
        IMAGE_REL_I386_DIR32,
        '?LoadFromZrd@HudUiCheckToggleWidget@@UAEHPAUNode@zReader@@PAUHudUiBackground@@@Z',
    ),
    (
        0x80,
        IMAGE_REL_I386_DIR32,
        '?PostLoadFromZrd@HudUiZrdWidget@@UAEXXZ',
    ),
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_FOCUSED_TARGET_ID = (
    "recoil:vc5-target:hud_ui_cycle_selector_widget_constructor"
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_FOCUSED_TARGET_NAME = (
    "hud_ui_cycle_selector_widget_constructor"
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_FOCUSED_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/hud_ui_cycle_selector_widget_constructor.json"
)


HUD_NET_GAME_SETUP_NUMERIC_WRAPPER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil-zui-zui-huduinumerictextinput-baseconstructor"
)


HUD_NET_GAME_SETUP_NUMERIC_WRAPPER_SOURCE_PATH = (
    "src/GameZRecoil/zUI/zui.cpp"
)


HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_IDENTITY = (
    "symbol:recoil:function:0x4b49e0"
)


HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SYMBOL_ID = (
    "recoil:function:0x4b49e0"
)


HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_SOURCE_PATH = (
    "src/GameZRecoil/zUI/zui_widgets.cpp"
)


HUD_NET_GAME_SETUP_NUMERIC_FOCUSED_TARGET_ID = (
    "recoil:vc5-target:hud_ui_numeric_text_input_cluster"
)


HUD_NET_GAME_SETUP_NUMERIC_FOCUSED_TARGET_NAME = (
    "hud_ui_numeric_text_input_cluster"
)


HUD_NET_GAME_SETUP_NUMERIC_FOCUSED_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/hud_ui_numeric_text_input_cluster.json"
)


HUD_NET_GAME_SETUP_NUMERIC_WRAPPER_BODY = bytes.fromhex(
    "568bf185f67405e8000000008bc65ec3"
)


HUD_NET_GAME_SETUP_NUMERIC_WRAPPER_COD_OFFSETS = (
    0x0,
    0x1,
    0x3,
    0x5,
    0x7,
    0xC,
    0xE,
    0xF,
)


HUD_NET_GAME_SETUP_NUMERIC_WRAPPER_COD_MNEMONICS = (
    "push",
    "mov",
    "test",
    "je",
    "call",
    "mov",
    "pop",
    "ret",
)


HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_BODY = bytes.fromhex(
    "6aff680000000064a10000000050648925000000005153568bf1578974240ce8000000008dbe4c01000033db68000100"
    "008bcf895c241ce800000000899f10010000c707000000008dbe60020000c6442418018bcfe800000000c70600000000"
    "889e70030000889e71030000c7866c03000001000000899e680300008b076a018bcfc644241c02ff506089b65c020000"
    "8b166a018bceff52608b4c24108bc65f5e64890d000000005b83c410c3909090"
)


HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_RELOCATIONS = (
    (0x03, IMAGE_REL_I386_DIR32, "$L83885"),
    (0x09, IMAGE_REL_I386_DIR32, "__except_list"),
    (0x11, IMAGE_REL_I386_DIR32, "__except_list"),
    (0x20, IMAGE_REL_I386_REL32, "??0HudUiZrdWidget@@QAE@XZ"),
    (0x38, IMAGE_REL_I386_REL32, "??0HudUiTextInput@@QAE@H@Z"),
    (0x44, IMAGE_REL_I386_DIR32, "??_7HudUiOwnedTextInput@@6B@"),
    (0x56, IMAGE_REL_I386_REL32, "??0HudUiSliderBorder@@QAE@XZ"),
    (0x5C, IMAGE_REL_I386_DIR32, "??_7HudUiNumericTextInput@@6B@"),
    (0xA4, IMAGE_REL_I386_DIR32, "__except_list"),
)


HUD_NET_GAME_SETUP_NUMERIC_CONSTRUCTOR_COD_MNEMONICS = (
    "push",
    "push",
    "mov",
    "push",
    "mov",
    "push",
    "push",
    "push",
    "mov",
    "push",
    "mov",
    "call",
    "lea",
    "xor",
    "push",
    "mov",
    "mov",
    "call",
    "mov",
    "mov",
    "lea",
    "mov",
    "mov",
    "call",
    "mov",
    "mov",
    "mov",
    "mov",
    "mov",
    "mov",
    "push",
    "mov",
    "mov",
    "call",
    "mov",
    "mov",
    "push",
    "mov",
    "call",
    "mov",
    "mov",
    "pop",
    "pop",
    "mov",
    "pop",
    "add",
    "ret",
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_BODY = bytes.fromhex(
    "568bf1e80000000033d2c7060000000089964c0100008996500100008d86b8010000b9140000008950b0891083c00449"
    "75f5899654010000c786580100001400000089965c0100008996640100008996600100008bc65ec39090909090909090"
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_RELOCATIONS = (
    (0x04, IMAGE_REL_I386_REL32, "??0HudUiZrdWidget@@QAE@XZ"),
    (
        0x0C,
        IMAGE_REL_I386_DIR32,
        "??_7HudUiCycleSelectorWidget@@6B@",
    ),
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_COD_OFFSETS = (
    0x00,
    0x01,
    0x03,
    0x08,
    0x0A,
    0x10,
    0x16,
    0x1C,
    0x22,
    0x27,
    0x2A,
    0x2C,
    0x2F,
    0x30,
    0x32,
    0x38,
    0x42,
    0x48,
    0x4E,
    0x54,
    0x56,
    0x57,
)


HUD_NET_GAME_SETUP_CYCLE_SELECTOR_CONSTRUCTOR_COD_MNEMONICS = (
    "push",
    "mov",
    "call",
    "xor",
    "mov",
    "mov",
    "mov",
    "lea",
    "mov",
    "mov",
    "mov",
    "add",
    "dec",
    "jne",
    "mov",
    "mov",
    "mov",
    "mov",
    "mov",
    "mov",
    "pop",
    "ret",
)


HUD_NET_GAME_SETUP_CHILD_CONSTRUCTOR_SPECS = (
    (
        HUD_NET_GAME_SETUP_LAUNCH_BUTTON_CONSTRUCTOR_SYMBOL,
        HUD_NET_GAME_SETUP_LAUNCH_BUTTON_VFTABLE_SYMBOL,
        HUD_NET_GAME_SETUP_LAUNCH_BUTTON_RETAIL_ORDINAL,
        HUD_NET_GAME_SETUP_LAUNCH_BUTTON_CALL_OFFSET,
    ),
    (
        HUD_NET_GAME_SETUP_CANCEL_BUTTON_CONSTRUCTOR_SYMBOL,
        HUD_NET_GAME_SETUP_CANCEL_BUTTON_VFTABLE_SYMBOL,
        HUD_NET_GAME_SETUP_CANCEL_BUTTON_RETAIL_ORDINAL,
        HUD_NET_GAME_SETUP_CANCEL_BUTTON_CALL_OFFSET,
    ),
)


HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_SIZE = 0x5F0


HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_BODY = bytes.fromhex(
    "6aff680000000064a100000000506489250000000083ec305355568bf15789742410e80000000033ff8d8e50a9000089"
    "7c2448e8000000008d8e9caa0000c644244801e8000000008daee8ab0000c6442448028bcde800000000c74500000000"
    "008d9e5caf0000c6442448038bcbe800000000c703000000008d9e64b10000c6442448048bcbe800000000c703000000"
    "008d9eb0b20000c6442448058bcbe800000000c703000000008d9efcb300006a048bcbc644244c06e800000000c70300"
    "0000008d8e78b70000c644244807e8000000008d8eccb80000c644244808e8000000008d8e20ba00006a02c644244c09"
    "e800000000c78620ba0000000000008d8e9cbd0000c64424480ae8000000008d8ef0be0000c64424480be8000000008d"
    "8e44c000006a02c644244c0ce800000000c78644c00000000000008d8ec0c30000c64424480de800000000c64424480e"
    "8d8e14c50000e8000000008d8e68c60000c64424480fe8000000008d8eccc70000c644244810e8000000008d8e30c900"
    "00c644244811e8000000008d8eecc90000c644244812e8000000006a158bcdc644244c13c70600000000e80000000068"
    "000000008bcde800000000578bcde800000000b801000000578986c8b8000089861cba00008986ecbe0000898640c000"
    "00898610c50000898664c600008b442454680000000068000000008bce89bec4b8000089be18ba000089bee8be000089"
    "be3cc0000089be0cc5000089be60c600008986a8ca0000e8000000008bf885ff0f84780100008d8630c9000068000000"
    "0050578bcee8000000008d86ecc90000680000000050578bcee8000000008d8650a90000680000000050578bcee80000"
    "00008d869caa0000680000000050578bcee800000000680000000055578bcee8000000008d865caf0000680000000050"
    "578bcee8000000008d8664b10000680000000050578bcee8000000008d86b0b20000680000000050578bcee800000000"
    "680000000053578bcee8000000008d8678b70000680000000050578bcee8000000008d86ccb80000680000000050578b"
    "cee8000000008d8620ba0000680000000050578bcee8000000008d869cbd0000680000000050578bcee8000000008d86"
    "f0be0000680000000050578bcee8000000008d8644c00000680000000050578bcee8000000008d86c0c3000068000000"
    "0050578bcee8000000008d8614c50000680000000050578bcee8000000008d8668c60000680000000050578bcee80000"
    "00008d86ccc70000680000000050578bcee800000000578bcee8000000008b9630c900008d8e30c900006a01ff52608b"
    "86ecc900008d8eecc900006a00ff50606a008d8e5caf0000e800000000c7864ca9000000000000e8000000008b3d0000"
    "0000508d4c242c680000000051ffd783c40c8d5424288bcd52e8000000006a168bcde8000000008b96a8ca000033c085"
    "d20f94c0894424508985c40000008b45008bcdff50786a0f8d4c2418680000000051c7837403000005000000c7837803"
    "000068010000ffd783c40c8d5424148bcb52e800000000bd01000000899ec4b8000089aec8b80000899e18ba00008d9e"
    "20ba00006a0a8d4424186800000000c7861cba0000ffffffff5089ab74030000c7837803000063000000ffd783c40c8d"
    "4c2414518bcbe800000000899ee8be000089aeecbe0000899e3cc00000c78640c00000ffffffffe80000000085c07446"
    "8b9644c000008d8e44c0000033ff89b9c4000000ff52788b86c0c300008d8ec0c3000089b9c4000000ff50788b9614c5"
    "00008d8e14c5000089b9c4000000ff5278e9880000008d9e44c000006a088d442418680000000050c783740300000200"
    "0000c7837803000008000000ffd783c40c8d4c2414518bcbe8000000008b7c24508b138bcb89bbc4000000ff52788d8e"
    "c0c3000089994c01000089a9500100008b0189b9c4000000ff50788d8e14c5000089994c010000c78150010000ffffff"
    "ff8b1189b9c4000000ff527833ff558d8e68c60000e800000000578d8eccc70000e800000000578bcee8000000008b4c"
    "24408bc65f5e5d64890d000000005b83c43cc204009090909090909090909090"
)


HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_RELOCATIONS = (
    (0x3, IMAGE_REL_I386_DIR32, '$L86230'),
    (0x9, IMAGE_REL_I386_DIR32, '__except_list'),
    (0x11, IMAGE_REL_I386_DIR32, '__except_list'),
    (0x23, IMAGE_REL_I386_REL32, '??0HudUiBackground@@QAE@XZ'),
    (0x34, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_LaunchButton@@QAE@XZ'),
    (0x44, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_CancelButton@@QAE@XZ'),
    (0x56, IMAGE_REL_I386_REL32, '??0HudUiNumericTextInput@@QAE@XZ'),
    (0x5d, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupTextInput@@6B@'),
    (0x6f, IMAGE_REL_I386_REL32, '??0HudUiCycleSelectorWidget@@QAE@XZ'),
    (0x75, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_WorldSelector@@6B@'),
    (0x87, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x8d, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_NextWorldButton@@6B@'),
    (0x9f, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0xa5, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_PrevWorldButton@@6B@'),
    (0xb9, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0xbf, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelTimeLimitInput@@6B@'),
    (0xcf, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_IncTimeLimitButton@@QAE@XZ'),
    (0xdf, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_DecTimeLimitButton@@QAE@XZ'),
    (0xf1, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0xfb, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelKillsInput@@6B@'),
    (0x10b, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_IncKillsButton@@QAE@XZ'),
    (0x11b, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_DecKillsButton@@QAE@XZ'),
    (0x12d, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0x137, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelMaxPlayersInput@@6B@'),
    (0x147, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_IncMaxPlayersButton@@QAE@XZ'),
    (0x157, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_DecMaxPlayersButton@@QAE@XZ'),
    (0x167, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_AllowMapsToggle@@QAE@XZ'),
    (0x177, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_NameTagsToggle@@QAE@XZ'),
    (0x187, IMAGE_REL_I386_REL32, '??0HudUiWidget@@QAE@XZ'),
    (0x197, IMAGE_REL_I386_REL32, '??0HudUiWidget@@QAE@XZ'),
    (0x1a6, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel@@6B@'),
    (0x1ab, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
    (0x1b0, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
    (0x1b7, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x1bf, IMAGE_REL_I386_REL32, '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z'),
    (0x1f2, IMAGE_REL_I386_DIR32, '??_C@_0M@KEOC@MP_NEW_GAME?$AA@'),
    (0x1f7, IMAGE_REL_I386_DIR32, '??_C@_0L@NPAH@dialog?4zrd?$AA@'),
    (0x228, IMAGE_REL_I386_REL32, '?LoadFromZrd@HudUiBackground@@QAEPAUNode@zReader@@PBD0H@Z'),
    (0x23d, IMAGE_REL_I386_DIR32, '??_C@_0N@BGIN@KILLS_SWITCH?$AA@'),
    (0x246, IMAGE_REL_I386_REL32, '?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiElement@@PBD@Z'),
    (0x251, IMAGE_REL_I386_DIR32, '??_C@_0M@EMP@LAPS_SWITCH?$AA@'),
    (0x25a, IMAGE_REL_I386_REL32, '?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiElement@@PBD@Z'),
    (0x265, IMAGE_REL_I386_DIR32, '??_C@_04BCHB@PLAY?$AA@'),
    (0x26e, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x279, IMAGE_REL_I386_DIR32, '??_C@_06BAHB@CANCEL?$AA@'),
    (0x282, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x287, IMAGE_REL_I386_DIR32, '??_C@_09JKDJ@GAME_NAME?$AA@'),
    (0x290, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x29b, IMAGE_REL_I386_DIR32, '??_C@_05KFHK@WORLD?$AA@'),
    (0x2a4, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2af, IMAGE_REL_I386_DIR32, '??_C@_09LMPC@INC_WORLD?$AA@'),
    (0x2b8, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2c3, IMAGE_REL_I386_DIR32, '??_C@_09PMNM@DEC_WORLD?$AA@'),
    (0x2cc, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2d1, IMAGE_REL_I386_DIR32, '??_C@_0L@GGHB@TIME_LIMIT?$AA@'),
    (0x2da, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2e5, IMAGE_REL_I386_DIR32, '??_C@_0P@OBMI@INC_TIME_LIMIT?$AA@'),
    (0x2ee, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2f9, IMAGE_REL_I386_DIR32, '??_C@_0P@LKIO@DEC_TIME_LIMIT?$AA@'),
    (0x302, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x30d, IMAGE_REL_I386_DIR32, '??_C@_05FIEB@KILLS?$AA@'),
    (0x316, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x321, IMAGE_REL_I386_DIR32, '??_C@_09EBMJ@INC_KILLS?$AA@'),
    (0x32a, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x335, IMAGE_REL_I386_DIR32, '??_C@_09BOH@DEC_KILLS?$AA@'),
    (0x33e, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x349, IMAGE_REL_I386_DIR32, '??_C@_0M@KFEM@MAX_PLAYERS?$AA@'),
    (0x352, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x35d, IMAGE_REL_I386_DIR32, '??_C@_0BA@GPML@INC_MAX_PLAYERS?$AA@'),
    (0x366, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x371, IMAGE_REL_I386_DIR32, '??_C@_0BA@FAJA@DEC_MAX_PLAYERS?$AA@'),
    (0x37a, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x385, IMAGE_REL_I386_DIR32, '??_C@_0L@BOHA@ALLOW_MAPS?$AA@'),
    (0x38e, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x399, IMAGE_REL_I386_DIR32, '??_C@_09LBCG@NAME_TAGS?$AA@'),
    (0x3a2, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3aa, IMAGE_REL_I386_REL32, '?FreeLoadedTreeRoots@HudUiBackground@@QAEXH@Z'),
    (0x3d9, IMAGE_REL_I386_REL32, '?SetIndexClamped@HudUiCycleSelectorWidget@@QAEHH@Z'),
    (0x3e8, IMAGE_REL_I386_REL32, '?zOptGetPlayerName@@YIPADXZ'),
    (0x3ee, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
    (0x3f8, IMAGE_REL_I386_DIR32, '??_C@_05LEAO@?$CF?421s?$AA@'),
    (0x40a, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x413, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
    (0x43d, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x463, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x48b, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x4b7, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x4d8, IMAGE_REL_I386_REL32, '?GetNetworkModemEnabled@zOpt@@YIHXZ'),
    (0x533, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x559, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x5b6, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
    (0x5c2, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
    (0x5ca, IMAGE_REL_I386_REL32, '?SetChildFlags@HudUiContainer@@QAEXI@Z'),
    (0x5da, IMAGE_REL_I386_DIR32, '__except_list'),
)


HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_COD_OFFSETS = (
    0x00,
    0x02,
    0x07,
    0x0D,
    0x0E,
    0x15,
    0x18,
    0x19,
    0x1A,
    0x1B,
    0x1D,
    0x1E,
    0x22,
    0x27,
    0x29,
    0x2F,
    0x33,
    0x38,
    0x3E,
    0x43,
    0x48,
    0x4E,
    0x53,
    0x55,
    0x5A,
    0x61,
    0x67,
    0x6C,
    0x6E,
    0x73,
    0x79,
    0x7F,
    0x84,
    0x86,
    0x8B,
    0x91,
    0x97,
    0x9C,
    0x9E,
    0xA3,
    0xA9,
    0xAF,
    0xB1,
    0xB3,
    0xB8,
    0xBD,
    0xC3,
    0xC9,
    0xCE,
    0xD3,
    0xD9,
    0xDE,
    0xE3,
    0xE9,
    0xEB,
    0xF0,
    0xF5,
    0xFF,
    0x105,
    0x10A,
    0x10F,
    0x115,
    0x11A,
    0x11F,
    0x125,
    0x127,
    0x12C,
    0x131,
    0x13B,
    0x141,
    0x146,
    0x14B,
    0x150,
    0x156,
    0x15B,
    0x161,
    0x166,
    0x16B,
    0x171,
    0x176,
    0x17B,
    0x181,
    0x186,
    0x18B,
    0x191,
    0x196,
    0x19B,
    0x19D,
    0x19F,
    0x1A4,
    0x1AA,
    0x1AF,
    0x1B4,
    0x1B6,
    0x1BB,
    0x1BC,
    0x1BE,
    0x1C3,
    0x1C8,
    0x1C9,
    0x1CF,
    0x1D5,
    0x1DB,
    0x1E1,
    0x1E7,
    0x1ED,
    0x1F1,
    0x1F6,
    0x1FB,
    0x1FD,
    0x203,
    0x209,
    0x20F,
    0x215,
    0x21B,
    0x221,
    0x227,
    0x22C,
    0x22E,
    0x230,
    0x236,
    0x23C,
    0x241,
    0x242,
    0x243,
    0x245,
    0x24A,
    0x250,
    0x255,
    0x256,
    0x257,
    0x259,
    0x25E,
    0x264,
    0x269,
    0x26A,
    0x26B,
    0x26D,
    0x272,
    0x278,
    0x27D,
    0x27E,
    0x27F,
    0x281,
    0x286,
    0x28B,
    0x28C,
    0x28D,
    0x28F,
    0x294,
    0x29A,
    0x29F,
    0x2A0,
    0x2A1,
    0x2A3,
    0x2A8,
    0x2AE,
    0x2B3,
    0x2B4,
    0x2B5,
    0x2B7,
    0x2BC,
    0x2C2,
    0x2C7,
    0x2C8,
    0x2C9,
    0x2CB,
    0x2D0,
    0x2D5,
    0x2D6,
    0x2D7,
    0x2D9,
    0x2DE,
    0x2E4,
    0x2E9,
    0x2EA,
    0x2EB,
    0x2ED,
    0x2F2,
    0x2F8,
    0x2FD,
    0x2FE,
    0x2FF,
    0x301,
    0x306,
    0x30C,
    0x311,
    0x312,
    0x313,
    0x315,
    0x31A,
    0x320,
    0x325,
    0x326,
    0x327,
    0x329,
    0x32E,
    0x334,
    0x339,
    0x33A,
    0x33B,
    0x33D,
    0x342,
    0x348,
    0x34D,
    0x34E,
    0x34F,
    0x351,
    0x356,
    0x35C,
    0x361,
    0x362,
    0x363,
    0x365,
    0x36A,
    0x370,
    0x375,
    0x376,
    0x377,
    0x379,
    0x37E,
    0x384,
    0x389,
    0x38A,
    0x38B,
    0x38D,
    0x392,
    0x398,
    0x39D,
    0x39E,
    0x39F,
    0x3A1,
    0x3A6,
    0x3A7,
    0x3A9,
    0x3AE,
    0x3B4,
    0x3BA,
    0x3BC,
    0x3BF,
    0x3C5,
    0x3CB,
    0x3CD,
    0x3D0,
    0x3D2,
    0x3D8,
    0x3DD,
    0x3E7,
    0x3EC,
    0x3F2,
    0x3F3,
    0x3F7,
    0x3FC,
    0x3FD,
    0x3FF,
    0x402,
    0x406,
    0x408,
    0x409,
    0x40E,
    0x410,
    0x412,
    0x417,
    0x41D,
    0x41F,
    0x421,
    0x424,
    0x428,
    0x42E,
    0x431,
    0x433,
    0x436,
    0x438,
    0x43C,
    0x441,
    0x442,
    0x44C,
    0x456,
    0x458,
    0x45B,
    0x45F,
    0x461,
    0x462,
    0x467,
    0x46C,
    0x472,
    0x478,
    0x47E,
    0x484,
    0x486,
    0x48A,
    0x48F,
    0x499,
    0x49A,
    0x4A0,
    0x4AA,
    0x4AC,
    0x4AF,
    0x4B3,
    0x4B4,
    0x4B6,
    0x4BB,
    0x4C1,
    0x4C7,
    0x4CD,
    0x4D7,
    0x4DC,
    0x4DE,
    0x4E0,
    0x4E6,
    0x4EC,
    0x4EE,
    0x4F4,
    0x4F7,
    0x4FD,
    0x503,
    0x509,
    0x50C,
    0x512,
    0x518,
    0x51E,
    0x521,
    0x526,
    0x52C,
    0x52E,
    0x532,
    0x537,
    0x538,
    0x542,
    0x54C,
    0x54E,
    0x551,
    0x555,
    0x556,
    0x558,
    0x55D,
    0x561,
    0x563,
    0x565,
    0x56B,
    0x56E,
    0x574,
    0x57A,
    0x580,
    0x582,
    0x588,
    0x58B,
    0x591,
    0x597,
    0x5A1,
    0x5A3,
    0x5A9,
    0x5AC,
    0x5AE,
    0x5AF,
    0x5B5,
    0x5BA,
    0x5BB,
    0x5C1,
    0x5C6,
    0x5C7,
    0x5C9,
    0x5CE,
    0x5D2,
    0x5D4,
    0x5D5,
    0x5D6,
    0x5D7,
    0x5DE,
    0x5DF,
    0x5E2,
)


HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_COD_MNEMONICS = (
    'push',
    'push',
    'mov',
    'push',
    'mov',
    'sub',
    'push',
    'push',
    'push',
    'mov',
    'push',
    'mov',
    'call',
    'xor',
    'lea',
    'mov',
    'call',
    'lea',
    'mov',
    'call',
    'lea',
    'mov',
    'mov',
    'call',
    'mov',
    'lea',
    'mov',
    'mov',
    'call',
    'mov',
    'lea',
    'mov',
    'mov',
    'call',
    'mov',
    'lea',
    'mov',
    'mov',
    'call',
    'mov',
    'lea',
    'push',
    'mov',
    'mov',
    'call',
    'mov',
    'lea',
    'mov',
    'call',
    'lea',
    'mov',
    'call',
    'lea',
    'push',
    'mov',
    'call',
    'mov',
    'lea',
    'mov',
    'call',
    'lea',
    'mov',
    'call',
    'lea',
    'push',
    'mov',
    'call',
    'mov',
    'lea',
    'mov',
    'call',
    'mov',
    'lea',
    'call',
    'lea',
    'mov',
    'call',
    'lea',
    'mov',
    'call',
    'lea',
    'mov',
    'call',
    'lea',
    'mov',
    'call',
    'push',
    'mov',
    'mov',
    'mov',
    'call',
    'push',
    'mov',
    'call',
    'push',
    'mov',
    'call',
    'mov',
    'push',
    'mov',
    'mov',
    'mov',
    'mov',
    'mov',
    'mov',
    'mov',
    'push',
    'push',
    'mov',
    'mov',
    'mov',
    'mov',
    'mov',
    'mov',
    'mov',
    'mov',
    'call',
    'mov',
    'test',
    'je',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'lea',
    'push',
    'push',
    'push',
    'mov',
    'call',
    'push',
    'mov',
    'call',
    'mov',
    'lea',
    'push',
    'call',
    'mov',
    'lea',
    'push',
    'call',
    'push',
    'lea',
    'call',
    'mov',
    'call',
    'mov',
    'push',
    'lea',
    'push',
    'push',
    'call',
    'add',
    'lea',
    'mov',
    'push',
    'call',
    'push',
    'mov',
    'call',
    'mov',
    'xor',
    'test',
    'sete',
    'mov',
    'mov',
    'mov',
    'mov',
    'call',
    'push',
    'lea',
    'push',
    'push',
    'mov',
    'mov',
    'call',
    'add',
    'lea',
    'mov',
    'push',
    'call',
    'mov',
    'mov',
    'mov',
    'mov',
    'lea',
    'push',
    'lea',
    'push',
    'mov',
    'push',
    'mov',
    'mov',
    'call',
    'add',
    'lea',
    'push',
    'mov',
    'call',
    'mov',
    'mov',
    'mov',
    'mov',
    'call',
    'test',
    'je',
    'mov',
    'lea',
    'xor',
    'mov',
    'call',
    'mov',
    'lea',
    'mov',
    'call',
    'mov',
    'lea',
    'mov',
    'call',
    'jmp',
    'lea',
    'push',
    'lea',
    'push',
    'push',
    'mov',
    'mov',
    'call',
    'add',
    'lea',
    'push',
    'mov',
    'call',
    'mov',
    'mov',
    'mov',
    'mov',
    'call',
    'lea',
    'mov',
    'mov',
    'mov',
    'mov',
    'call',
    'lea',
    'mov',
    'mov',
    'mov',
    'mov',
    'call',
    'xor',
    'push',
    'lea',
    'call',
    'push',
    'lea',
    'call',
    'push',
    'mov',
    'call',
    'mov',
    'mov',
    'pop',
    'pop',
    'pop',
    'mov',
    'pop',
    'add',
    'ret',
)


HUD_NET_GAME_SETUP_STEP_BUTTON_CURRENT_BODY_REGION = bytes.fromhex(
    "8d8e78b70000c644244807e800000000c78678b70000000000008d8eccb80000c644244808e800000000c786ccb80000"
    "000000008d8e20ba00006a02c644244c09e800000000c78620ba0000000000008d8e9cbd0000c64424480ae800000000"
    "c7869cbd0000000000008d8ef0be0000c64424480be800000000c786f0be0000000000008d8e44c000006a02c644244c"
    "0ce800000000c78644c0000000000000c64424480d8d8ec0c30000e800000000c786c0c30000000000008d8e14c50000"
    "c64424480ee800000000c78614c50000000000008d8e68c60000c64424480fe8000000008d8eccc70000c644244810e8"
    "000000008d8e30c90000c644244811e8000000008d8eecc90000c644244812e8000000006a158bcdc644244c13c70600"
    "000000e80000000068000000008bcde800000000578bcde800000000"
)


HUD_NET_GAME_SETUP_STEP_BUTTON_CURRENT_RELOCATIONS = (
    (0xCF, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (
        0xD9,
        IMAGE_REL_I386_DIR32,
        '??_7HudUiNetGameSetupPanel_IncTimeLimitButton@@6B@',
    ),
    (0xE9, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (
        0xF3,
        IMAGE_REL_I386_DIR32,
        '??_7HudUiNetGameSetupPanel_DecTimeLimitButton@@6B@',
    ),
    (0x105, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0x10F, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelKillsInput@@6B@'),
    (0x11F, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (
        0x129,
        IMAGE_REL_I386_DIR32,
        '??_7HudUiNetGameSetupPanel_IncKillsButton@@6B@',
    ),
    (0x139, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (
        0x143,
        IMAGE_REL_I386_DIR32,
        '??_7HudUiNetGameSetupPanel_DecKillsButton@@6B@',
    ),
    (0x155, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (
        0x15F,
        IMAGE_REL_I386_DIR32,
        '??_7CHudUiNetGameSetupPanelMaxPlayersInput@@6B@',
    ),
    (0x16F, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (
        0x179,
        IMAGE_REL_I386_DIR32,
        '??_7HudUiNetGameSetupPanel_IncMaxPlayersButton@@6B@',
    ),
    (0x189, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (
        0x193,
        IMAGE_REL_I386_DIR32,
        '??_7HudUiNetGameSetupPanel_DecMaxPlayersButton@@6B@',
    ),
)


HUD_NET_GAME_SETUP_STEP_BUTTON_CURRENT_COD_ROWS = (
    (0xC3, 'lea'),
    (0xC9, 'mov'),
    (0xCE, 'call'),
    (0xD3, 'mov'),
    (0xDD, 'lea'),
    (0xE3, 'mov'),
    (0xE8, 'call'),
    (0xED, 'mov'),
    (0xF7, 'lea'),
    (0xFD, 'push'),
    (0xFF, 'mov'),
    (0x104, 'call'),
    (0x109, 'mov'),
    (0x113, 'lea'),
    (0x119, 'mov'),
    (0x11E, 'call'),
    (0x123, 'mov'),
    (0x12D, 'lea'),
    (0x133, 'mov'),
    (0x138, 'call'),
    (0x13D, 'mov'),
    (0x147, 'lea'),
    (0x14D, 'push'),
    (0x14F, 'mov'),
    (0x154, 'call'),
    (0x159, 'mov'),
    (0x163, 'mov'),
    (0x168, 'lea'),
    (0x16E, 'call'),
    (0x173, 'mov'),
    (0x17D, 'lea'),
    (0x183, 'mov'),
    (0x188, 'call'),
    (0x18D, 'mov'),
    (0x197, 'lea'),
    (0x19D, 'mov'),
    (0x1A2, 'call'),
    (0x1A7, 'lea'),
    (0x1AD, 'mov'),
    (0x1B2, 'call'),
    (0x1B7, 'lea'),
    (0x1BD, 'mov'),
    (0x1C2, 'call'),
    (0x1C7, 'lea'),
    (0x1CD, 'mov'),
    (0x1D2, 'call'),
    (0x1D7, 'push'),
    (0x1D9, 'mov'),
    (0x1DB, 'mov'),
    (0x1E0, 'mov'),
    (0x1E6, 'call'),
    (0x1EB, 'push'),
    (0x1F0, 'mov'),
    (0x1F2, 'call'),
    (0x1F7, 'push'),
    (0x1F8, 'mov'),
    (0x1FA, 'call'),
)


HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_SIZE = 0x630


HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_BODY[:0xC3]
    + HUD_NET_GAME_SETUP_STEP_BUTTON_CURRENT_BODY_REGION
    + HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_BODY[0x1C3:]
    + b"\x90" * 4
)


HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_RELOCATIONS = (
    tuple(
        (0x03, IMAGE_REL_I386_DIR32, '$L86202')
        if row[0] == 0x03
        else row
        for row in HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_RELOCATIONS
        if row[0] < 0xC3
    )
    + HUD_NET_GAME_SETUP_STEP_BUTTON_CURRENT_RELOCATIONS
    + tuple(
        (offset + 0x3C, relocation_type, symbol_name)
        for offset, relocation_type, symbol_name
        in HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_RELOCATIONS
        if offset >= 0x167
    )
)


HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_COD_ROWS = (
    tuple(
        zip(
            (
                offset
                for offset in HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_COD_OFFSETS
                if offset < 0xC3
            ),
            HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_COD_MNEMONICS,
        )
    )
    + HUD_NET_GAME_SETUP_STEP_BUTTON_CURRENT_COD_ROWS
    + tuple(
        (offset + 0x3C, mnemonic)
        for offset, mnemonic in zip(
            HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_COD_OFFSETS,
            HUD_NET_GAME_SETUP_PRE_STEP_BUTTON_CALLER_COD_MNEMONICS,
        )
        if offset >= 0x1C3
    )
)


HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_TOGGLE_CURRENT_BODY_REGION = bytes.fromhex(
    "c78668c60000000000008d8eccc70000c644244810e800000000c786ccc70000000000008d8e30c90000c644244811e8"
    "000000008d8eecc90000c644244812e8000000006a158bcdc644244c13c70600000000e80000000068000000008bcde8"
    "00000000578bcde800000000"
)


HUD_NET_GAME_SETUP_TOGGLE_CURRENT_RELOCATIONS = (
    (0x1A3, IMAGE_REL_I386_REL32, '??0HudUiCheckToggleWidget@@QAE@XZ'),
    (
        0x1AD,
        IMAGE_REL_I386_DIR32,
        '??_7HudUiNetGameSetupPanel_AllowMapsToggle@@6B@',
    ),
    (0x1BD, IMAGE_REL_I386_REL32, '??0HudUiCheckToggleWidget@@QAE@XZ'),
    (
        0x1C7,
        IMAGE_REL_I386_DIR32,
        '??_7HudUiNetGameSetupPanel_NameTagsToggle@@6B@',
    ),
)


HUD_NET_GAME_SETUP_TOGGLE_CURRENT_COD_ROWS = (
    (0x1A7, 'mov'),
    (0x1B1, 'lea'),
    (0x1B7, 'mov'),
    (0x1BC, 'call'),
    (0x1C1, 'mov'),
    (0x1CB, 'lea'),
    (0x1D1, 'mov'),
    (0x1D6, 'call'),
    (0x1DB, 'lea'),
    (0x1E1, 'mov'),
    (0x1E6, 'call'),
    (0x1EB, 'push'),
    (0x1ED, 'mov'),
    (0x1EF, 'mov'),
    (0x1F4, 'mov'),
    (0x1FA, 'call'),
    (0x1FF, 'push'),
    (0x204, 'mov'),
    (0x206, 'call'),
    (0x20B, 'push'),
    (0x20C, 'mov'),
    (0x20E, 'call'),
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_SIZE = 0x640


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_BODY[:0x1A7]
    + HUD_NET_GAME_SETUP_TOGGLE_CURRENT_BODY_REGION
    + HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_BODY[0x1FF:-4]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    tuple(
        (0x03, IMAGE_REL_I386_DIR32, '$L86195')
        if row[0] == 0x03
        else row
        for row in HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_RELOCATIONS
        if row[0] < 0x1A3
    )
    + HUD_NET_GAME_SETUP_TOGGLE_CURRENT_RELOCATIONS
    + tuple(
        (offset + 0x14, relocation_type, symbol_name)
        for offset, relocation_type, symbol_name
        in HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_RELOCATIONS
        if offset >= 0x1C3
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    tuple(
        (offset, mnemonic)
        for offset, mnemonic in HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_COD_ROWS
        if offset < 0x1A7
    )
    + HUD_NET_GAME_SETUP_TOGGLE_CURRENT_COD_ROWS
    + tuple(
        (offset + 0x14, mnemonic)
        for offset, mnemonic in HUD_NET_GAME_SETUP_PRE_TOGGLE_CALLER_COD_ROWS
        if offset >= 0x1FF
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY
)


HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS
)


HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_BODY[:0x1D1]
    + b"\x57"
    + HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_BODY[0x1D1:0x1D4]
    + b"\x4c"
    + HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_BODY[0x1D5:-1]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = tuple(
    (
        offset + 1,
        relocation_type,
        (
            "??0HudUiWidget@@QAE@I@Z"
            if offset == 0x1D7
            else symbol_name
        ),
    )
    if offset >= 0x1D7
    else (offset, relocation_type, symbol_name)
    for offset, relocation_type, symbol_name
    in HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_RELOCATIONS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    tuple(
        row
        for row in HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_COD_ROWS
        if row[0] < 0x1D1
    )
    + ((0x1D1, "push"),)
    + tuple(
        (offset + 1, mnemonic)
        for offset, mnemonic
        in HUD_NET_GAME_SETUP_PRE_KILLS_SWITCH_CALLER_COD_ROWS
        if offset >= 0x1D1
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY
)


HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS
)


HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_BODY[:0x1E2]
    + b"\x57"
    + HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_BODY[0x1E2:0x1E5]
    + b"\x4c"
    + HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_BODY[0x1E6:-1]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = tuple(
    (
        offset + 1,
        relocation_type,
        (
            "??0HudUiWidget@@QAE@I@Z"
            if offset == 0x1E8
            else symbol_name
        ),
    )
    if offset >= 0x1E8
    else (offset, relocation_type, symbol_name)
    for offset, relocation_type, symbol_name
    in HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_RELOCATIONS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    tuple(
        row
        for row in HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_COD_ROWS
        if row[0] < 0x1E2
    )
    + ((0x1E2, "push"),)
    + tuple(
        (offset + 1, mnemonic)
        for offset, mnemonic
        in HUD_NET_GAME_SETUP_PRE_LAPS_SWITCH_CALLER_COD_ROWS
        if offset >= 0x1E2
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260804-004: moving the existing background load to the first
# constructor-body call changes the complete caller authority.  Keep the
# authority literal so later ordinal guards cannot project an older layout.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_SIZE = 0x630


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = bytes.fromhex(
    "6aff680000000064a100000000506489250000000083ec305355568bf15789742410e8000000008d8e50a90000c744244800"
    "000000e8000000008d8e9caa0000c644244801e8000000008daee8ab0000c6442448028bcde800000000c74500000000008d"
    "be5caf0000c6442448038bcfe800000000c707000000008dbe64b10000c6442448048bcfe800000000c707000000008dbeb0"
    "b20000c6442448058bcfe800000000c707000000008d9efcb300006a048bcbc644244c06e800000000c703000000008dbe78"
    "b70000c6442448078bcfe800000000c707000000008dbeccb80000c6442448088bcfe800000000c707000000008dbe20ba00"
    "006a028bcfc644244c09e800000000c707000000008dbe9cbd0000c64424480a8bcfe800000000c707000000008dbef0be00"
    "00c64424480b8bcfe800000000c70700000000c64424480c8dbe44c000006a028bcfe800000000c707000000008dbec0c300"
    "00c64424480d8bcfe800000000c707000000008dbe14c50000c64424480e8bcfe800000000c707000000008dbe68c60000c6"
    "4424480f8bcfe800000000c707000000008dbeccc70000c6442448108bcfe800000000c707000000008d8e30c900006a00c6"
    "44244c11e8000000008d8eecc900006a00c644244c12e8000000006a00680000000068000000008bcec644245413c7060000"
    "0000e8000000006a158bcd8bf8e80000000068000000008bcde8000000006a008bcde8000000008b4c245033c0ba01000000"
    "3bf88986c4b800008996c8b80000898618ba000089961cba00008986e8be00008996ecbe000089863cc00000899640c00000"
    "89860cc50000899610c50000898660c60000899664c60000898ea8ca00000f847d0100008d8630c90000680000000050578b"
    "cee8000000008d86ecc90000680000000050578bcee8000000008d8650a90000680000000050578bcee8000000008d869caa"
    "0000680000000050578bcee800000000680000000055578bcee8000000008d865caf0000680000000050578bcee800000000"
    "8d8664b10000680000000050578bcee8000000008d86b0b20000680000000050578bcee800000000680000000053578bcee8"
    "000000008d8678b70000680000000050578bcee8000000008d86ccb80000680000000050578bcee8000000008d8620ba0000"
    "680000000050578bcee8000000008d869cbd0000680000000050578bcee8000000008d86f0be0000680000000050578bcee8"
    "000000008d8644c00000680000000050578bcee8000000008d86c0c30000680000000050578bcee8000000008d8614c50000"
    "680000000050578bcee8000000008d8668c60000680000000050578bcee8000000008d86ccc70000680000000050578bcee8"
    "00000000578bcee800000000ba010000008b8630c900008d8e30c9000052ff50608b96ecc900008d8eecc900006a00ff5260"
    "6a008d8e5caf0000e800000000c7864ca9000000000000e8000000008b3d00000000508d44242c680000000050ffd783c40c"
    "8d4c2428518bcde8000000006a168bcde8000000008b8ea8ca00008b550033c085c90f94c08bcd894424508985c4000000ff"
    "52786a0f8d442418680000000050c7837403000005000000c7837803000068010000ffd783c40c8d4c2414518bcbe8000000"
    "00bd01000000899ec4b8000089aec8b80000899e18ba00008d9e20ba00006a0a8d5424186800000000c7861cba0000ffffff"
    "ff5289ab74030000c7837803000063000000ffd783c40c8d4424148bcb50e800000000899ee8be000089aeecbe0000899e3c"
    "c00000c78640c00000ffffffffe80000000085c074468b9644c000008d8e44c0000033ff89b9c4000000ff52788b86c0c300"
    "008d8ec0c3000089b9c4000000ff50788b9614c500008d8e14c5000089b9c4000000ff5278e9880000008d9e44c000006a08"
    "8d442418680000000050c7837403000002000000c7837803000008000000ffd783c40c8d4c2414518bcbe8000000008b7c24"
    "508b138bcb89bbc4000000ff52788d8ec0c3000089994c01000089a9500100008b0189b9c4000000ff50788d8e14c5000089"
    "994c010000c78150010000ffffffff8b1189b9c4000000ff527833ff558d8e68c60000e800000000578d8eccc70000e80000"
    "0000578bcee8000000008b4c24408bc65f5e5d64890d000000005b83c43cc2040090"
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    (0x003, IMAGE_REL_I386_DIR32, '$L86194'),
    (0x009, IMAGE_REL_I386_DIR32, '__except_list'),
    (0x011, IMAGE_REL_I386_DIR32, '__except_list'),
    (0x023, IMAGE_REL_I386_REL32, '??0HudUiBackground@@QAE@XZ'),
    (0x036, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_LaunchButton@@QAE@XZ'),
    (0x046, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_CancelButton@@QAE@XZ'),
    (0x058, IMAGE_REL_I386_REL32, '??0HudUiNumericTextInput@@QAE@XZ'),
    (0x05F, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupTextInput@@6B@'),
    (0x071, IMAGE_REL_I386_REL32, '??0HudUiCycleSelectorWidget@@QAE@XZ'),
    (0x077, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_WorldSelector@@6B@'),
    (0x089, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x08F, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_NextWorldButton@@6B@'),
    (0x0A1, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x0A7, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_PrevWorldButton@@6B@'),
    (0x0BB, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0x0C1, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelTimeLimitInput@@6B@'),
    (0x0D3, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x0D9, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_IncTimeLimitButton@@6B@'),
    (0x0EB, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x0F1, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_DecTimeLimitButton@@6B@'),
    (0x105, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0x10B, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelKillsInput@@6B@'),
    (0x11D, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x123, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_IncKillsButton@@6B@'),
    (0x135, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x13B, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_DecKillsButton@@6B@'),
    (0x14F, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0x155, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelMaxPlayersInput@@6B@'),
    (0x167, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x16D, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_IncMaxPlayersButton@@6B@'),
    (0x17F, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x185, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_DecMaxPlayersButton@@6B@'),
    (0x197, IMAGE_REL_I386_REL32, '??0HudUiCheckToggleWidget@@QAE@XZ'),
    (0x19D, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_AllowMapsToggle@@6B@'),
    (0x1AF, IMAGE_REL_I386_REL32, '??0HudUiCheckToggleWidget@@QAE@XZ'),
    (0x1B5, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_NameTagsToggle@@6B@'),
    (0x1C7, IMAGE_REL_I386_REL32, '??0HudUiWidget@@QAE@I@Z'),
    (0x1D9, IMAGE_REL_I386_REL32, '??0HudUiWidget@@QAE@I@Z'),
    (0x1E0, IMAGE_REL_I386_DIR32, '??_C@_0M@KEOC@MP_NEW_GAME?$AA@'),
    (0x1E5, IMAGE_REL_I386_DIR32, '??_C@_0L@NPAH@dialog?4zrd?$AA@'),
    (0x1F2, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel@@6B@'),
    (0x1F7, IMAGE_REL_I386_REL32, '?LoadFromZrd@HudUiBackground@@QAEPAUNode@zReader@@PBD0H@Z'),
    (0x202, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
    (0x207, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
    (0x20E, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x217, IMAGE_REL_I386_REL32, '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z'),
    (0x283, IMAGE_REL_I386_DIR32, '??_C@_0N@BGIN@KILLS_SWITCH?$AA@'),
    (0x28C, IMAGE_REL_I386_REL32, '?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiElement@@PBD@Z'),
    (0x297, IMAGE_REL_I386_DIR32, '??_C@_0M@EMP@LAPS_SWITCH?$AA@'),
    (0x2A0, IMAGE_REL_I386_REL32, '?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiElement@@PBD@Z'),
    (0x2AB, IMAGE_REL_I386_DIR32, '??_C@_04BCHB@PLAY?$AA@'),
    (0x2B4, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2BF, IMAGE_REL_I386_DIR32, '??_C@_06BAHB@CANCEL?$AA@'),
    (0x2C8, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2CD, IMAGE_REL_I386_DIR32, '??_C@_09JKDJ@GAME_NAME?$AA@'),
    (0x2D6, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2E1, IMAGE_REL_I386_DIR32, '??_C@_05KFHK@WORLD?$AA@'),
    (0x2EA, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2F5, IMAGE_REL_I386_DIR32, '??_C@_09LMPC@INC_WORLD?$AA@'),
    (0x2FE, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x309, IMAGE_REL_I386_DIR32, '??_C@_09PMNM@DEC_WORLD?$AA@'),
    (0x312, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x317, IMAGE_REL_I386_DIR32, '??_C@_0L@GGHB@TIME_LIMIT?$AA@'),
    (0x320, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x32B, IMAGE_REL_I386_DIR32, '??_C@_0P@OBMI@INC_TIME_LIMIT?$AA@'),
    (0x334, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x33F, IMAGE_REL_I386_DIR32, '??_C@_0P@LKIO@DEC_TIME_LIMIT?$AA@'),
    (0x348, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x353, IMAGE_REL_I386_DIR32, '??_C@_05FIEB@KILLS?$AA@'),
    (0x35C, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x367, IMAGE_REL_I386_DIR32, '??_C@_09EBMJ@INC_KILLS?$AA@'),
    (0x370, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x37B, IMAGE_REL_I386_DIR32, '??_C@_09BOH@DEC_KILLS?$AA@'),
    (0x384, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x38F, IMAGE_REL_I386_DIR32, '??_C@_0M@KFEM@MAX_PLAYERS?$AA@'),
    (0x398, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3A3, IMAGE_REL_I386_DIR32, '??_C@_0BA@GPML@INC_MAX_PLAYERS?$AA@'),
    (0x3AC, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3B7, IMAGE_REL_I386_DIR32, '??_C@_0BA@FAJA@DEC_MAX_PLAYERS?$AA@'),
    (0x3C0, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3CB, IMAGE_REL_I386_DIR32, '??_C@_0L@BOHA@ALLOW_MAPS?$AA@'),
    (0x3D4, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3DF, IMAGE_REL_I386_DIR32, '??_C@_09LBCG@NAME_TAGS?$AA@'),
    (0x3E8, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3F0, IMAGE_REL_I386_REL32, '?FreeLoadedTreeRoots@HudUiBackground@@QAEXH@Z'),
    (0x423, IMAGE_REL_I386_REL32, '?SetIndexClamped@HudUiCycleSelectorWidget@@QAEHH@Z'),
    (0x432, IMAGE_REL_I386_REL32, '?zOptGetPlayerName@@YIPADXZ'),
    (0x438, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
    (0x442, IMAGE_REL_I386_DIR32, '??_C@_05LEAO@?$CF?421s?$AA@'),
    (0x454, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x45D, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
    (0x487, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x4AD, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x4D5, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x501, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x522, IMAGE_REL_I386_REL32, '?GetNetworkModemEnabled@zOpt@@YIHXZ'),
    (0x57D, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x5A3, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x600, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
    (0x60C, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
    (0x614, IMAGE_REL_I386_REL32, '?SetChildFlags@HudUiContainer@@QAEXI@Z'),
    (0x624, IMAGE_REL_I386_DIR32, '__except_list'),
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    int(value, 16)
    for value in (
        "0 2 7 d e 15 18 19 1a 1b 1d 1e 22 27 2d 35 3a 40 45 4a 50 55 57 5c 63 69 6e 70 75 7b 81 86 88 8d 93 99 9e a0 a5 ab b1 b3 b5 ba bf c5 cb d0 d2 d7 dd e3 e8 ea ef f5 fb fd ff 104 109 10f 115 11a 11c 121 127 12d 132 134 139 13f 144 14a 14c 14e 153 159 15f 164 166 16b 171 177 17c 17e 183 189 18f 194 196 19b 1a1 1a7 1ac 1ae 1b3 1b9 1bf 1c1 1c6 1cb 1d1 1d3 1d8 1dd 1df 1e4 1e9 1eb 1f0 1f6 1fb 1fd 1ff 201 206 20b 20d 212 214 216 21b 21f 221 226 228 22e 234 23a 240 246 24c 252 258 25e 264 26a 270 276 27c 282 287 288 289 28b 290 296 29b 29c 29d 29f 2a4 2aa 2af 2b0 2b1 2b3 2b8 2be 2c3 2c4 2c5 2c7 2cc 2d1 2d2 2d3 2d5 2da 2e0 2e5 2e6 2e7 2e9 2ee 2f4 2f9 2fa 2fb 2fd 302 308 30d 30e 30f 311 316 31b 31c 31d 31f 324 32a 32f 330 331 333 338 33e 343 344 345 347 34c 352 357 358 359 35b 360 366 36b 36c 36d 36f 374 37a 37f 380 381 383 388 38e 393 394 395 397 39c 3a2 3a7 3a8 3a9 3ab 3b0 3b6 3bb 3bc 3bd 3bf 3c4 3ca 3cf 3d0 3d1 3d3 3d8 3de 3e3 3e4 3e5 3e7 3ec 3ed 3ef 3f4 3f9 3ff 405 406 409 40f 415 417 41a 41c 422 427 431 436 43c 43d 441 446 447 449 44c 450 451 453 458 45a 45c 461 467 46a 46c 46e 471 473 477 47d 480 482 486 48b 48c 496 4a0 4a2 4a5 4a9 4aa 4ac 4b1 4b6 4bc 4c2 4c8 4ce 4d0 4d4 4d9 4e3 4e4 4ea 4f4 4f6 4f9 4fd 4ff 500 505 50b 511 517 521 526 528 52a 530 536 538 53e 541 547 54d 553 556 55c 562 568 56b 570 576 578 57c 581 582 58c 596 598 59b 59f 5a0 5a2 5a7 5ab 5ad 5af 5b5 5b8 5be 5c4 5ca 5cc 5d2 5d5 5db 5e1 5eb 5ed 5f3 5f6 5f8 5f9 5ff 604 605 60b 610 611 613 618 61c 61e 61f 620 621 628 629 62c"
    ).split()
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    "push push mov push mov sub push push push mov push mov call lea mov call lea mov call lea mov mov call mov lea mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov mov call mov lea mov mov call mov lea mov mov call mov mov lea push mov call mov lea mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov call lea push mov call push push push mov mov mov call push mov mov call push mov call push mov call mov xor mov cmp mov mov mov mov mov mov mov mov mov mov mov mov mov je lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call push mov call mov mov lea push call mov lea push call push lea call mov call mov push lea push push call add lea push mov call push mov call mov mov xor test sete mov mov mov call push lea push push mov mov call add lea push mov call mov mov mov mov lea push lea push mov push mov mov call add lea mov push call mov mov mov mov call test je mov lea xor mov call mov lea mov call mov lea mov call jmp lea push lea push push mov mov call add lea push mov call mov mov mov mov call lea mov mov mov mov call lea mov mov mov mov call xor push lea call push lea call push mov call mov mov pop pop pop mov pop add ret".split()
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = tuple(
    zip(
        HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS,
        (
            HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS[:93]
            + ("mov", "mov", "call", "mov", "lea")
            + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS[93:]
        ),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260804-005: the intact gameNameInput cluster now follows the complete
# loadedSection guard.  This is the complete post-move caller authority.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = bytes.fromhex(
    "6aff680000000064a100000000506489250000000083ec305355568bf15789742410e8000000008d8e50a90000c744244800"
    "000000e8000000008d8e9caa0000c644244801e8000000008daee8ab0000c6442448028bcde800000000c74500000000008d"
    "be5caf0000c6442448038bcfe800000000c707000000008dbe64b10000c6442448048bcfe800000000c707000000008dbeb0"
    "b20000c6442448058bcfe800000000c707000000008d9efcb300006a048bcbc644244c06e800000000c703000000008dbe78"
    "b70000c6442448078bcfe800000000c707000000008dbeccb80000c6442448088bcfe800000000c707000000008dbe20ba00"
    "006a028bcfc644244c09e800000000c707000000008dbe9cbd0000c64424480a8bcfe800000000c707000000008dbef0be00"
    "00c64424480b8bcfe800000000c70700000000c64424480c8dbe44c000006a028bcfe800000000c707000000008dbec0c300"
    "00c64424480d8bcfe800000000c707000000008dbe14c50000c64424480e8bcfe800000000c707000000008dbe68c60000c6"
    "4424480f8bcfe800000000c707000000008dbeccc70000c6442448108bcfe800000000c707000000008d8e30c900006a00c6"
    "44244c11e8000000008d8eecc900006a00c644244c12e8000000006a00680000000068000000008bcec644245413c7060000"
    "0000e8000000008bf8b80100000033c98986c8b8000089861cba00008986ecbe0000898640c00000898610c50000898664c6"
    "00008b4424503bf9898ec4b80000898e18ba0000898ee8be0000898e3cc00000898e0cc50000898e60c600008986a8ca0000"
    "0f84780100008d8630c90000680000000050578bcee8000000008d86ecc90000680000000050578bcee8000000008d8650a9"
    "0000680000000050578bcee8000000008d869caa0000680000000050578bcee800000000680000000055578bcee800000000"
    "8d865caf0000680000000050578bcee8000000008d8664b10000680000000050578bcee8000000008d86b0b2000068000000"
    "0050578bcee800000000680000000053578bcee8000000008d8678b70000680000000050578bcee8000000008d86ccb80000"
    "680000000050578bcee8000000008d8620ba0000680000000050578bcee8000000008d869cbd0000680000000050578bcee8"
    "000000008d86f0be0000680000000050578bcee8000000008d8644c00000680000000050578bcee8000000008d86c0c30000"
    "680000000050578bcee8000000008d8614c50000680000000050578bcee8000000008d8668c60000680000000050578bcee8"
    "000000008d86ccc70000680000000050578bcee800000000578bcee8000000006a158bcde80000000068000000008bcde800"
    "0000006a008bcde8000000008b9630c900008d8e30c900006a01ff52608b86ecc900008d8eecc900006a00ff50606a008d8e"
    "5caf0000e800000000c7864ca9000000000000e8000000008b3d00000000508d4c242c680000000051ffd783c40c8d542428"
    "8bcd52e8000000006a168bcde8000000008b96a8ca000033c085d20f94c0894424508985c40000008b45008bcdff50786a0f"
    "8d4c2418680000000051c7837403000005000000c7837803000068010000ffd783c40c8d5424148bcb52e800000000bd0100"
    "0000899ec4b8000089aec8b80000899e18ba00008d9e20ba00006a0a8d4424186800000000c7861cba0000ffffffff5089ab"
    "74030000c7837803000063000000ffd783c40c8d4c2414518bcbe800000000899ee8be000089aeecbe0000899e3cc00000c7"
    "8640c00000ffffffffe80000000085c074468b9644c000008d8e44c0000033ff89b9c4000000ff52788b86c0c300008d8ec0"
    "c3000089b9c4000000ff50788b9614c500008d8e14c5000089b9c4000000ff5278e9880000008d9e44c000006a088d442418"
    "680000000050c7837403000002000000c7837803000008000000ffd783c40c8d4c2414518bcbe8000000008b7c24508b138b"
    "cb89bbc4000000ff52788d8ec0c3000089994c01000089a9500100008b0189b9c4000000ff50788d8e14c5000089994c0100"
    "00c78150010000ffffffff8b1189b9c4000000ff527833ff558d8e68c60000e800000000578d8eccc70000e800000000578b"
    "cee8000000008b4c24408bc65f5e5d64890d000000005b83c43cc204009090909090"
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    (0x003, IMAGE_REL_I386_DIR32, '$L86195'),
    (0x009, IMAGE_REL_I386_DIR32, '__except_list'),
    (0x011, IMAGE_REL_I386_DIR32, '__except_list'),
    (0x023, IMAGE_REL_I386_REL32, '??0HudUiBackground@@QAE@XZ'),
    (0x036, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_LaunchButton@@QAE@XZ'),
    (0x046, IMAGE_REL_I386_REL32, '??0HudUiNetGameSetupPanel_CancelButton@@QAE@XZ'),
    (0x058, IMAGE_REL_I386_REL32, '??0HudUiNumericTextInput@@QAE@XZ'),
    (0x05F, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupTextInput@@6B@'),
    (0x071, IMAGE_REL_I386_REL32, '??0HudUiCycleSelectorWidget@@QAE@XZ'),
    (0x077, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_WorldSelector@@6B@'),
    (0x089, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x08F, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_NextWorldButton@@6B@'),
    (0x0A1, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x0A7, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_PrevWorldButton@@6B@'),
    (0x0BB, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0x0C1, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelTimeLimitInput@@6B@'),
    (0x0D3, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x0D9, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_IncTimeLimitButton@@6B@'),
    (0x0EB, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x0F1, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_DecTimeLimitButton@@6B@'),
    (0x105, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0x10B, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelKillsInput@@6B@'),
    (0x11D, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x123, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_IncKillsButton@@6B@'),
    (0x135, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x13B, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_DecKillsButton@@6B@'),
    (0x14F, IMAGE_REL_I386_REL32, '??0HudUiClampedIntTextInput@@QAE@I@Z'),
    (0x155, IMAGE_REL_I386_DIR32, '??_7CHudUiNetGameSetupPanelMaxPlayersInput@@6B@'),
    (0x167, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x16D, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_IncMaxPlayersButton@@6B@'),
    (0x17F, IMAGE_REL_I386_REL32, '??0HudUiZrdWidget@@QAE@XZ'),
    (0x185, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_DecMaxPlayersButton@@6B@'),
    (0x197, IMAGE_REL_I386_REL32, '??0HudUiCheckToggleWidget@@QAE@XZ'),
    (0x19D, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_AllowMapsToggle@@6B@'),
    (0x1AF, IMAGE_REL_I386_REL32, '??0HudUiCheckToggleWidget@@QAE@XZ'),
    (0x1B5, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel_NameTagsToggle@@6B@'),
    (0x1C7, IMAGE_REL_I386_REL32, '??0HudUiWidget@@QAE@I@Z'),
    (0x1D9, IMAGE_REL_I386_REL32, '??0HudUiWidget@@QAE@I@Z'),
    (0x1E0, IMAGE_REL_I386_DIR32, '??_C@_0M@KEOC@MP_NEW_GAME?$AA@'),
    (0x1E5, IMAGE_REL_I386_DIR32, '??_C@_0L@NPAH@dialog?4zrd?$AA@'),
    (0x1F2, IMAGE_REL_I386_DIR32, '??_7HudUiNetGameSetupPanel@@6B@'),
    (0x1F7, IMAGE_REL_I386_REL32, '?LoadFromZrd@HudUiBackground@@QAEPAUNode@zReader@@PBD0H@Z'),
    (0x265, IMAGE_REL_I386_DIR32, '??_C@_0N@BGIN@KILLS_SWITCH?$AA@'),
    (0x26E, IMAGE_REL_I386_REL32, '?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiElement@@PBD@Z'),
    (0x279, IMAGE_REL_I386_DIR32, '??_C@_0M@EMP@LAPS_SWITCH?$AA@'),
    (0x282, IMAGE_REL_I386_REL32, '?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiElement@@PBD@Z'),
    (0x28D, IMAGE_REL_I386_DIR32, '??_C@_04BCHB@PLAY?$AA@'),
    (0x296, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2A1, IMAGE_REL_I386_DIR32, '??_C@_06BAHB@CANCEL?$AA@'),
    (0x2AA, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2AF, IMAGE_REL_I386_DIR32, '??_C@_09JKDJ@GAME_NAME?$AA@'),
    (0x2B8, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2C3, IMAGE_REL_I386_DIR32, '??_C@_05KFHK@WORLD?$AA@'),
    (0x2CC, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2D7, IMAGE_REL_I386_DIR32, '??_C@_09LMPC@INC_WORLD?$AA@'),
    (0x2E0, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2EB, IMAGE_REL_I386_DIR32, '??_C@_09PMNM@DEC_WORLD?$AA@'),
    (0x2F4, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x2F9, IMAGE_REL_I386_DIR32, '??_C@_0L@GGHB@TIME_LIMIT?$AA@'),
    (0x302, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x30D, IMAGE_REL_I386_DIR32, '??_C@_0P@OBMI@INC_TIME_LIMIT?$AA@'),
    (0x316, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x321, IMAGE_REL_I386_DIR32, '??_C@_0P@LKIO@DEC_TIME_LIMIT?$AA@'),
    (0x32A, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x335, IMAGE_REL_I386_DIR32, '??_C@_05FIEB@KILLS?$AA@'),
    (0x33E, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x349, IMAGE_REL_I386_DIR32, '??_C@_09EBMJ@INC_KILLS?$AA@'),
    (0x352, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x35D, IMAGE_REL_I386_DIR32, '??_C@_09BOH@DEC_KILLS?$AA@'),
    (0x366, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x371, IMAGE_REL_I386_DIR32, '??_C@_0M@KFEM@MAX_PLAYERS?$AA@'),
    (0x37A, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x385, IMAGE_REL_I386_DIR32, '??_C@_0BA@GPML@INC_MAX_PLAYERS?$AA@'),
    (0x38E, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x399, IMAGE_REL_I386_DIR32, '??_C@_0BA@FAJA@DEC_MAX_PLAYERS?$AA@'),
    (0x3A2, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3AD, IMAGE_REL_I386_DIR32, '??_C@_0L@BOHA@ALLOW_MAPS?$AA@'),
    (0x3B6, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3C1, IMAGE_REL_I386_DIR32, '??_C@_09LBCG@NAME_TAGS?$AA@'),
    (0x3CA, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
    (0x3D2, IMAGE_REL_I386_REL32, '?FreeLoadedTreeRoots@HudUiBackground@@QAEXH@Z'),
    (0x3DB, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
    (0x3E0, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
    (0x3E7, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x3F0, IMAGE_REL_I386_REL32, '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z'),
    (0x41F, IMAGE_REL_I386_REL32, '?SetIndexClamped@HudUiCycleSelectorWidget@@QAEHH@Z'),
    (0x42E, IMAGE_REL_I386_REL32, '?zOptGetPlayerName@@YIPADXZ'),
    (0x434, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
    (0x43E, IMAGE_REL_I386_DIR32, '??_C@_05LEAO@?$CF?421s?$AA@'),
    (0x450, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x459, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
    (0x483, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x4A9, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x4D1, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x4FD, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x51E, IMAGE_REL_I386_REL32, '?GetNetworkModemEnabled@zOpt@@YIHXZ'),
    (0x579, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
    (0x59F, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
    (0x5FC, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
    (0x608, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
    (0x610, IMAGE_REL_I386_REL32, '?SetChildFlags@HudUiContainer@@QAEXI@Z'),
    (0x620, IMAGE_REL_I386_DIR32, '__except_list'),
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    int(value, 16)
    for value in (
        "0 2 7 d e 15 18 19 1a 1b 1d 1e 22 27 2d 35 3a 40 45 4a 50 55 57 5c 63 69 6e 70 75 7b 81 86 88 8d 93 99 9e a0 a5 ab b1 b3 b5 ba bf c5 cb d0 d2 d7 dd e3 e8 ea ef f5 fb fd ff 104 109 10f 115 11a 11c 121 127 12d 132 134 139 13f 144 14a 14c 14e 153 159 15f 164 166 16b 171 177 17c 17e 183 189 18f 194 196 19b 1a1 1a7 1ac 1ae 1b3 1b9 1bf 1c1 1c6 1cb 1d1 1d3 1d8 1dd 1df 1e4 1e9 1eb 1f0 1f6 1fb 1fd 202 204 20a 210 216 21c 222 228 22c 22e 234 23a 240 246 24c 252 258 25e 264 269 26a 26b 26d 272 278 27d 27e 27f 281 286 28c 291 292 293 295 29a 2a0 2a5 2a6 2a7 2a9 2ae 2b3 2b4 2b5 2b7 2bc 2c2 2c7 2c8 2c9 2cb 2d0 2d6 2db 2dc 2dd 2df 2e4 2ea 2ef 2f0 2f1 2f3 2f8 2fd 2fe 2ff 301 306 30c 311 312 313 315 31a 320 325 326 327 329 32e 334 339 33a 33b 33d 342 348 34d 34e 34f 351 356 35c 361 362 363 365 36a 370 375 376 377 379 37e 384 389 38a 38b 38d 392 398 39d 39e 39f 3a1 3a6 3ac 3b1 3b2 3b3 3b5 3ba 3c0 3c5 3c6 3c7 3c9 3ce 3cf 3d1 3d6 3d8 3da 3df 3e4 3e6 3eb 3ed 3ef 3f4 3fa 400 402 405 40b 411 413 416 418 41e 423 42d 432 438 439 43d 442 443 445 448 44c 44e 44f 454 456 458 45d 463 465 467 46a 46e 474 477 479 47c 47e 482 487 488 492 49c 49e 4a1 4a5 4a7 4a8 4ad 4b2 4b8 4be 4c4 4ca 4cc 4d0 4d5 4df 4e0 4e6 4f0 4f2 4f5 4f9 4fa 4fc 501 507 50d 513 51d 522 524 526 52c 532 534 53a 53d 543 549 54f 552 558 55e 564 567 56c 572 574 578 57d 57e 588 592 594 597 59b 59c 59e 5a3 5a7 5a9 5ab 5b1 5b4 5ba 5c0 5c6 5c8 5ce 5d1 5d7 5dd 5e7 5e9 5ef 5f2 5f4 5f5 5fb 600 601 607 60c 60d 60f 614 618 61a 61b 61c 61d 624 625 628"
    ).split()
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    "push push mov push mov sub push push push mov push mov call lea mov call lea mov call lea mov mov call mov lea mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov mov call mov lea mov mov call mov lea mov mov call mov mov lea push mov call mov lea mov mov call mov lea mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov call lea push mov call push push push mov mov mov call mov mov xor mov mov mov mov mov mov mov cmp mov mov mov mov mov mov mov je lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call push mov call push mov call push mov call push mov call mov lea push call mov lea push call push lea call mov call mov push lea push push call add lea mov push call push mov call mov xor test sete mov mov mov mov call push lea push push mov mov call add lea mov push call mov mov mov mov lea push lea push mov push mov mov call add lea push mov call mov mov mov mov call test je mov lea xor mov call mov lea mov call mov lea mov call jmp lea push lea push push mov mov call add lea push mov call mov mov mov mov call lea mov mov mov mov call lea mov mov mov mov call xor push lea call push lea call push mov call mov mov pop pop pop mov pop add ret".split()
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = tuple(
    zip(
        HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS,
        (
            HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS[:237]
            + ("lea", "push", "push", "push", "mov", "call")
            + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS[237:]
        ),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260804-006: the reviewed visibility pair now precedes the intact
# gameNameInput cluster.  This is the complete post-reorder caller authority.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = bytes.fromhex(
    "6aff680000000064a100000000506489250000000083ec305355568bf15789742410e8000000008d8e50a90000c74424"
    "4800000000e8000000008d8e9caa0000c644244801e8000000008daee8ab0000c6442448028bcde800000000c7450000"
    "0000008dbe5caf0000c6442448038bcfe800000000c707000000008dbe64b10000c6442448048bcfe800000000c70700"
    "0000008dbeb0b20000c6442448058bcfe800000000c707000000008d9efcb300006a048bcbc644244c06e800000000c7"
    "03000000008dbe78b70000c6442448078bcfe800000000c707000000008dbeccb80000c6442448088bcfe800000000c7"
    "07000000008dbe20ba00006a028bcfc644244c09e800000000c707000000008dbe9cbd0000c64424480a8bcfe8000000"
    "00c707000000008dbef0be0000c64424480b8bcfe800000000c70700000000c64424480c8dbe44c000006a028bcfe800"
    "000000c707000000008dbec0c30000c64424480d8bcfe800000000c707000000008dbe14c50000c64424480e8bcfe800"
    "000000c707000000008dbe68c60000c64424480f8bcfe800000000c707000000008dbeccc70000c6442448108bcfe800"
    "000000c707000000008d8e30c900006a00c644244c11e8000000008d8eecc900006a00c644244c12e8000000006a0068"
    "0000000068000000008bcec644245413c70600000000e8000000008b4c24508bf833c0ba010000003bf88986c4b80000"
    "8996c8b80000898618ba000089961cba00008986e8be00008996ecbe000089863cc00000899640c0000089860cc50000"
    "899610c50000898660c60000899664c60000898ea8ca00000f847d0100008d8630c90000680000000050578bcee80000"
    "00008d86ecc90000680000000050578bcee8000000008d8650a90000680000000050578bcee8000000008d869caa0000"
    "680000000050578bcee800000000680000000055578bcee8000000008d865caf0000680000000050578bcee800000000"
    "8d8664b10000680000000050578bcee8000000008d86b0b20000680000000050578bcee800000000680000000053578b"
    "cee8000000008d8678b70000680000000050578bcee8000000008d86ccb80000680000000050578bcee8000000008d86"
    "20ba0000680000000050578bcee8000000008d869cbd0000680000000050578bcee8000000008d86f0be000068000000"
    "0050578bcee8000000008d8644c00000680000000050578bcee8000000008d86c0c30000680000000050578bcee80000"
    "00008d8614c50000680000000050578bcee8000000008d8668c60000680000000050578bcee8000000008d86ccc70000"
    "680000000050578bcee800000000578bcee800000000ba010000008b8630c900008d8e30c9000052ff50608b96ecc900"
    "008d8eecc900006a00ff52606a158bcde80000000068000000008bcde8000000006a008bcde8000000006a008d8e5caf"
    "0000e800000000c7864ca9000000000000e8000000008b3d00000000508d44242c680000000050ffd783c40c8d4c2428"
    "518bcde8000000006a168bcde8000000008b8ea8ca00008b550033c085c90f94c08bcd894424508985c4000000ff5278"
    "6a0f8d442418680000000050c7837403000005000000c7837803000068010000ffd783c40c8d4c2414518bcbe8000000"
    "00bd01000000899ec4b8000089aec8b80000899e18ba00008d9e20ba00006a0a8d5424186800000000c7861cba0000ff"
    "ffffff5289ab74030000c7837803000063000000ffd783c40c8d4424148bcb50e800000000899ee8be000089aeecbe00"
    "00899e3cc00000c78640c00000ffffffffe80000000085c074468b9644c000008d8e44c0000033ff89b9c4000000ff52"
    "788b86c0c300008d8ec0c3000089b9c4000000ff50788b9614c500008d8e14c5000089b9c4000000ff5278e988000000"
    "8d9e44c000006a088d442418680000000050c7837403000002000000c7837803000008000000ffd783c40c8d4c241451"
    "8bcbe8000000008b7c24508b138bcb89bbc4000000ff52788d8ec0c3000089994c01000089a9500100008b0189b9c400"
    "0000ff50788d8e14c5000089994c010000c78150010000ffffffff8b1189b9c4000000ff527833ff558d8e68c60000e8"
    "00000000578d8eccc70000e800000000578bcee8000000008b4c24408bc65f5e5d64890d000000005b83c43cc2040090"
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[:81]
    + (
        (0x401, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x406, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (0x40D, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x416, IMAGE_REL_I386_REL32, '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z'),
        (0x423, IMAGE_REL_I386_REL32, '?SetIndexClamped@HudUiCycleSelectorWidget@@QAEHH@Z'),
        (0x432, IMAGE_REL_I386_REL32, '?zOptGetPlayerName@@YIPADXZ'),
        (0x438, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (0x442, IMAGE_REL_I386_DIR32, '??_C@_05LEAO@?$CF?421s?$AA@'),
        (0x454, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x45D, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x487, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x4AD, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x4D5, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x501, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x522, IMAGE_REL_I386_REL32, '?GetNetworkModemEnabled@zOpt@@YIHXZ'),
        (0x57D, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x5A3, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x600, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
        (0x60C, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
        (0x614, IMAGE_REL_I386_REL32, '?SetChildFlags@HudUiContainer@@QAEXI@Z'),
        (0x624, IMAGE_REL_I386_DIR32, '__except_list'),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    int(value, 16)
    for value in """
        0 2 7 d e 15 18 19 1a 1b 1d 1e 22 27 2d 35 3a 40 45 4a 50 55 57 5c 63 69 6e 70 75 7b 81 86 88 8d 93 99 9e a0 a5
        ab b1 b3 b5 ba bf c5 cb d0 d2 d7 dd e3 e8 ea ef f5 fb fd ff 104 109 10f 115 11a 11c 121 127 12d 132 134 139 13f
        144 14a 14c 14e 153 159 15f 164 166 16b 171 177 17c 17e 183 189 18f 194 196 19b 1a1 1a7 1ac 1ae 1b3 1b9 1bf 1c1
        1c6 1cb 1d1 1d3 1d8 1dd 1df 1e4 1e9 1eb 1f0 1f6 1fb 1ff 201 203 208 20a 210 216 21c 222 228 22e 234 23a 240 246
        24c 252 258 25e 264 269 26a 26b 26d 272 278 27d 27e 27f 281 286 28c 291 292 293 295 29a 2a0 2a5 2a6 2a7 2a9 2ae
        2b3 2b4 2b5 2b7 2bc 2c2 2c7 2c8 2c9 2cb 2d0 2d6 2db 2dc 2dd 2df 2e4 2ea 2ef 2f0 2f1 2f3 2f8 2fd 2fe 2ff 301 306
        30c 311 312 313 315 31a 320 325 326 327 329 32e 334 339 33a 33b 33d 342 348 34d 34e 34f 351 356 35c 361 362 363
        365 36a 370 375 376 377 379 37e 384 389 38a 38b 38d 392 398 39d 39e 39f 3a1 3a6 3ac 3b1 3b2 3b3 3b5 3ba 3c0 3c5
        3c6 3c7 3c9 3ce 3cf 3d1 3d6 3db 3e1 3e7 3e8 3eb 3f1 3f7 3f9 3fc 3fe 400 405 40a 40c 411 413 415 41a 41c 422 427
        431 436 43c 43d 441 446 447 449 44c 450 451 453 458 45a 45c 461 467 46a 46c 46e 471 473 477 47d 480 482 486 48b
        48c 496 4a0 4a2 4a5 4a9 4aa 4ac 4b1 4b6 4bc 4c2 4c8 4ce 4d0 4d4 4d9 4e3 4e4 4ea 4f4 4f6 4f9 4fd 4ff 500 505 50b
        511 517 521 526 528 52a 530 536 538 53e 541 547 54d 553 556 55c 562 568 56b 570 576 578 57c 581 582 58c 596 598
        59b 59f 5a0 5a2 5a7 5ab 5ad 5af 5b5 5b8 5be 5c4 5ca 5cc 5d2 5d5 5db 5e1 5eb 5ed 5f3 5f6 5f8 5f9 5ff 604 605 60b
        610 611 613 618 61c 61e 61f 620 621 628 629 62c
    """.split()
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    """
        push push mov push mov sub push push push mov push mov call lea mov call lea mov call lea mov mov call mov lea
        mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov mov call mov lea mov mov call mov lea
        mov mov call mov lea push mov mov call mov lea mov mov call mov lea mov mov call mov mov lea push mov call mov
        lea mov mov call mov lea mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov call lea push
        mov call push push push mov mov mov call mov mov xor mov cmp mov mov mov mov mov mov mov mov mov mov mov mov mov
        je lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov
        call push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call
        push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea
        push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea
        push push push mov call lea push push push mov call lea push push push mov call push mov call mov mov lea push
        call mov lea push call push mov call push mov call push mov call push lea call mov call mov push lea push push
        call add lea push mov call push mov call mov mov xor test sete mov mov mov call push lea push push mov mov call
        add lea push mov call mov mov mov mov lea push lea push mov push mov mov call add lea mov push call mov mov mov
        mov call test je mov lea xor mov call mov lea mov call mov lea mov call jmp lea push lea push push mov mov call
        add lea push mov call mov mov mov mov call lea mov mov mov mov call lea mov mov mov mov call xor push lea call
        push lea call push mov call mov mov pop pop pop mov pop add ret
    """.split()
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = tuple(
    zip(
        HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS,
        HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS,
    )
)


# WSI-20260804-007: the existing world-selector index call now follows the
# reviewed visibility pair.  Keep the complete post-reorder body authority.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = bytes.fromhex(
    "6aff680000000064a100000000506489250000000083ec305355568bf15789742410e8000000008d8e50a90000c74424"
    "4800000000e8000000008d8e9caa0000c644244801e8000000008daee8ab0000c6442448028bcde800000000c7450000"
    "0000008dbe5caf0000c6442448038bcfe800000000c707000000008dbe64b10000c6442448048bcfe800000000c70700"
    "0000008dbeb0b20000c6442448058bcfe800000000c707000000008d9efcb300006a048bcbc644244c06e800000000c7"
    "03000000008dbe78b70000c6442448078bcfe800000000c707000000008dbeccb80000c6442448088bcfe800000000c7"
    "07000000008dbe20ba00006a028bcfc644244c09e800000000c707000000008dbe9cbd0000c64424480a8bcfe8000000"
    "00c707000000008dbef0be0000c64424480b8bcfe800000000c70700000000c64424480c8dbe44c000006a028bcfe800"
    "000000c707000000008dbec0c30000c64424480d8bcfe800000000c707000000008dbe14c50000c64424480e8bcfe800"
    "000000c707000000008dbe68c60000c64424480f8bcfe800000000c707000000008dbeccc70000c6442448108bcfe800"
    "000000c707000000008d8e30c900006a00c644244c11e8000000008d8eecc900006a00c644244c12e8000000006a0068"
    "0000000068000000008bcec644245413c70600000000e8000000008b4c24508bf833c0ba010000003bf88986c4b80000"
    "8996c8b80000898618ba000089961cba00008986e8be00008996ecbe000089863cc00000899640c0000089860cc50000"
    "899610c50000898660c60000899664c60000898ea8ca00000f847d0100008d8630c90000680000000050578bcee80000"
    "00008d86ecc90000680000000050578bcee8000000008d8650a90000680000000050578bcee8000000008d869caa0000"
    "680000000050578bcee800000000680000000055578bcee8000000008d865caf0000680000000050578bcee800000000"
    "8d8664b10000680000000050578bcee8000000008d86b0b20000680000000050578bcee800000000680000000053578b"
    "cee8000000008d8678b70000680000000050578bcee8000000008d86ccb80000680000000050578bcee8000000008d86"
    "20ba0000680000000050578bcee8000000008d869cbd0000680000000050578bcee8000000008d86f0be000068000000"
    "0050578bcee8000000008d8644c00000680000000050578bcee8000000008d86c0c30000680000000050578bcee80000"
    "00008d8614c50000680000000050578bcee8000000008d8668c60000680000000050578bcee8000000008d86ccc70000"
    "680000000050578bcee800000000578bcee800000000ba010000008b8630c900008d8e30c9000052ff50608b96ecc900"
    "008d8eecc900006a00ff52606a008d8e5caf0000e8000000006a158bcde80000000068000000008bcde8000000006a00"
    "8bcde800000000c7864ca9000000000000e8000000008b3d00000000508d44242c680000000050ffd783c40c8d4c2428"
    "518bcde8000000006a168bcde8000000008b8ea8ca00008b550033c085c90f94c08bcd894424508985c4000000ff5278"
    "6a0f8d442418680000000050c7837403000005000000c7837803000068010000ffd783c40c8d4c2414518bcbe8000000"
    "00bd01000000899ec4b8000089aec8b80000899e18ba00008d9e20ba00006a0a8d5424186800000000c7861cba0000ff"
    "ffffff5289ab74030000c7837803000063000000ffd783c40c8d4424148bcb50e800000000899ee8be000089aeecbe00"
    "00899e3cc00000c78640c00000ffffffffe80000000085c074468b9644c000008d8e44c0000033ff89b9c4000000ff52"
    "788b86c0c300008d8ec0c3000089b9c4000000ff50788b9614c500008d8e14c5000089b9c4000000ff5278e988000000"
    "8d9e44c000006a088d442418680000000050c7837403000002000000c7837803000008000000ffd783c40c8d4c241451"
    "8bcbe8000000008b7c24508b138bcb89bbc4000000ff52788d8ec0c3000089994c01000089a9500100008b0189b9c400"
    "0000ff50788d8e14c5000089994c010000c78150010000ffffffff8b1189b9c4000000ff527833ff558d8e68c60000e8"
    "00000000578d8eccc70000e800000000578bcee8000000008b4c24408bc65f5e5d64890d000000005b83c43cc2040090"
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[:81]
    + (
        (0x405, IMAGE_REL_I386_REL32, '?SetIndexClamped@HudUiCycleSelectorWidget@@QAEHH@Z'),
        (0x40E, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x413, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (0x41A, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x423, IMAGE_REL_I386_REL32, '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z'),
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[86:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[:256]
    + (
        (0x3FE, "lea"),
        (0x404, "call"),
        (0x409, "push"),
        (0x40B, "mov"),
        (0x40D, "call"),
        (0x412, "push"),
        (0x417, "mov"),
        (0x419, "call"),
        (0x41E, "push"),
        (0x420, "mov"),
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[266:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260804-008: hoisting the authored player-name getter changes register
# allocation, the complete caller extent, and the later instruction topology.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_SIZE = 0x640


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = bytes.fromhex(
    "6aff680000000064a100000000506489250000000083ec305355568bf15789742410e8000000008d8e50a90000c74424"
    "4800000000e8000000008d8e9caa0000c644244801e8000000008daee8ab0000c6442448028bcde800000000c7450000"
    "0000008dbe5caf0000c6442448038bcfe800000000c707000000008dbe64b10000c6442448048bcfe800000000c70700"
    "0000008dbeb0b20000c6442448058bcfe800000000c707000000008d9efcb300006a048bcbc644244c06e800000000c7"
    "03000000008dbe78b70000c6442448078bcfe800000000c707000000008dbeccb80000c6442448088bcfe800000000c7"
    "07000000008dbe20ba00006a028bcfc644244c09e800000000c707000000008dbe9cbd0000c64424480a8bcfe8000000"
    "00c707000000008dbef0be0000c64424480b8bcfe800000000c70700000000c64424480c8dbe44c000006a028bcfe800"
    "000000c707000000008dbec0c30000c64424480d8bcfe800000000c707000000008dbe14c50000c64424480e8bcfe800"
    "000000c707000000008dbe68c60000c64424480f8bcfe800000000c707000000008dbeccc70000c6442448108bcfe800"
    "000000c707000000008d8e30c900006a00c644244c11e8000000008d8eecc900006a00c644244c12e8000000006a0068"
    "0000000068000000008bcec644245413c70600000000e8000000008b4c24508bf833c0ba010000003bf88986c4b80000"
    "8996c8b80000898618ba000089961cba00008986e8be00008996ecbe000089863cc00000899640c0000089860cc50000"
    "899610c50000898660c60000899664c60000898ea8ca00000f847d0100008d8630c90000680000000050578bcee80000"
    "00008d86ecc90000680000000050578bcee8000000008d8650a90000680000000050578bcee8000000008d869caa0000"
    "680000000050578bcee800000000680000000055578bcee8000000008d865caf0000680000000050578bcee800000000"
    "8d8664b10000680000000050578bcee8000000008d86b0b20000680000000050578bcee800000000680000000053578b"
    "cee8000000008d8678b70000680000000050578bcee8000000008d86ccb80000680000000050578bcee8000000008d86"
    "20ba0000680000000050578bcee8000000008d869cbd0000680000000050578bcee8000000008d86f0be000068000000"
    "0050578bcee8000000008d8644c00000680000000050578bcee8000000008d86c0c30000680000000050578bcee80000"
    "00008d8614c50000680000000050578bcee8000000008d8668c60000680000000050578bcee8000000008d86ccc70000"
    "680000000050578bcee800000000578bcee800000000ba010000008b8630c900008d8e30c9000052ff50608b96ecc900"
    "008d8eecc900006a00ff52606a008d8e5caf0000e800000000e8000000006a158bcd8bf8e80000000068000000008bcd"
    "e8000000006a008bcde800000000578b3d000000008d44242c680000000050c7864ca9000000000000ffd783c40c8d4c"
    "2428518bcde8000000006a168bcde8000000008b8ea8ca00008b550033c085c90f94c08bcd894424508985c4000000ff"
    "52786a0f8d442418680000000050c7837403000005000000c7837803000068010000ffd783c40c8d4c2414518bcbe800"
    "000000bd01000000899ec4b8000089aec8b80000899e18ba00008d9e20ba00006a0a8d5424186800000000c7861cba00"
    "00ffffffff5289ab74030000c7837803000063000000ffd783c40c8d4424148bcb50e800000000899ee8be000089aeec"
    "be0000899e3cc00000c78640c00000ffffffffe80000000085c074468b9644c000008d8e44c0000033ff89b9c4000000"
    "ff52788b86c0c300008d8ec0c3000089b9c4000000ff50788b9614c500008d8e14c5000089b9c4000000ff5278e98800"
    "00008d9e44c000006a088d442418680000000050c7837403000002000000c7837803000008000000ffd783c40c8d4c24"
    "14518bcbe8000000008b7c24508b138bcb89bbc4000000ff52788d8ec0c3000089994c01000089a9500100008b0189b9"
    "c4000000ff50788d8e14c5000089994c010000c78150010000ffffffff8b1189b9c4000000ff527833ff558d8e68c600"
    "00e800000000578d8eccc70000e800000000578bcee8000000008b4c24408bc65f5e5d64890d000000005b83c43cc204"
    "00909090909090909090909090909090"
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    ((0x003, IMAGE_REL_I386_DIR32, '$L86196'),)
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[1:82]
    + (
        (0x40A, IMAGE_REL_I386_REL32, '?zOptGetPlayerName@@YIPADXZ'),
        (0x415, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x41A, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (0x421, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x42A, IMAGE_REL_I386_REL32, '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z'),
        (0x431, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (0x43A, IMAGE_REL_I386_DIR32, '??_C@_05LEAO@?$CF?421s?$AA@'),
        (0x456, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x45F, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x489, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x4AF, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x4D7, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x503, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x524, IMAGE_REL_I386_REL32, '?GetNetworkModemEnabled@zOpt@@YIHXZ'),
        (0x57F, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x5A5, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x602, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
        (0x60E, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
        (0x616, IMAGE_REL_I386_REL32, '?SetChildFlags@HudUiContainer@@QAEXI@Z'),
        (0x626, IMAGE_REL_I386_DIR32, '__except_list'),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    int(value, 16)
    for value in """
        0 2 7 d e 15 18 19 1a 1b 1d 1e 22 27 2d 35 3a 40 45 4a 50 55 57 5c 63 69 6e 70 75 7b 81 86 88 8d 93 99 9e a0 a5
        ab b1 b3 b5 ba bf c5 cb d0 d2 d7 dd e3 e8 ea ef f5 fb fd ff 104 109 10f 115 11a 11c 121 127 12d 132 134 139 13f
        144 14a 14c 14e 153 159 15f 164 166 16b 171 177 17c 17e 183 189 18f 194 196 19b 1a1 1a7 1ac 1ae 1b3 1b9 1bf 1c1
        1c6 1cb 1d1 1d3 1d8 1dd 1df 1e4 1e9 1eb 1f0 1f6 1fb 1ff 201 203 208 20a 210 216 21c 222 228 22e 234 23a 240 246
        24c 252 258 25e 264 269 26a 26b 26d 272 278 27d 27e 27f 281 286 28c 291 292 293 295 29a 2a0 2a5 2a6 2a7 2a9 2ae
        2b3 2b4 2b5 2b7 2bc 2c2 2c7 2c8 2c9 2cb 2d0 2d6 2db 2dc 2dd 2df 2e4 2ea 2ef 2f0 2f1 2f3 2f8 2fd 2fe 2ff 301 306
        30c 311 312 313 315 31a 320 325 326 327 329 32e 334 339 33a 33b 33d 342 348 34d 34e 34f 351 356 35c 361 362 363
        365 36a 370 375 376 377 379 37e 384 389 38a 38b 38d 392 398 39d 39e 39f 3a1 3a6 3ac 3b1 3b2 3b3 3b5 3ba 3c0 3c5
        3c6 3c7 3c9 3ce 3cf 3d1 3d6 3db 3e1 3e7 3e8 3eb 3f1 3f7 3f9 3fc 3fe 404 409 40e 410 412 414 419 41e 420 425 427
        429 42e 42f 435 439 43e 43f 449 44b 44e 452 453 455 45a 45c 45e 463 469 46c 46e 470 473 475 479 47f 482 484 488
        48d 48e 498 4a2 4a4 4a7 4ab 4ac 4ae 4b3 4b8 4be 4c4 4ca 4d0 4d2 4d6 4db 4e5 4e6 4ec 4f6 4f8 4fb 4ff 501 502 507
        50d 513 519 523 528 52a 52c 532 538 53a 540 543 549 54f 555 558 55e 564 56a 56d 572 578 57a 57e 583 584 58e 598
        59a 59d 5a1 5a2 5a4 5a9 5ad 5af 5b1 5b7 5ba 5c0 5c6 5cc 5ce 5d4 5d7 5dd 5e3 5ed 5ef 5f5 5f8 5fa 5fb 601 606 607
        60d 612 613 615 61a 61e 620 621 622 623 62a 62b 62e
    """.split()
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    """
        push push mov push mov sub push push push mov push mov call lea mov call lea mov call lea mov mov call mov lea
        mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov mov call mov lea mov mov call mov lea
        mov mov call mov lea push mov mov call mov lea mov mov call mov lea mov mov call mov mov lea push mov call mov
        lea mov mov call mov lea mov mov call mov lea mov mov call mov lea mov mov call mov lea push mov call lea push
        mov call push push push mov mov mov call mov mov xor mov cmp mov mov mov mov mov mov mov mov mov mov mov mov mov
        je lea push push push mov call lea push push push mov call lea push push push mov call lea push push push mov
        call push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call
        push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea
        push push push mov call lea push push push mov call lea push push push mov call lea push push push mov call lea
        push push push mov call lea push push push mov call lea push push push mov call push mov call mov mov lea push
        call mov lea push call push lea call call push mov mov call push mov call push mov call push mov lea push push
        mov call add lea push mov call push mov call mov mov xor test sete mov mov mov call push lea push push mov mov
        call add lea push mov call mov mov mov mov lea push lea push mov push mov mov call add lea mov push call mov mov
        mov mov call test je mov lea xor mov call mov lea mov call mov lea mov call jmp lea push lea push push mov mov
        call add lea push mov call mov mov mov mov call lea mov mov mov mov call lea mov mov mov mov call xor push lea
        call push lea call push mov call mov mov pop pop pop mov pop add ret
    """.split()
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = tuple(
    zip(
        HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS,
        HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS,
    )
)


# WSI-20260804-009: moving the player-name formatting block ahead of the
# numeric-text update sequence changes the complete caller tail and extent.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_SIZE = 0x630


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[:0x40E]
    + bytes.fromhex(
        "8b3d00000000508d44242c680000000050ffd783c40c8bcd6a15e80000000068000000008bcde8000000006a008bcde8"
        "000000008d4c2428c7864ca9000000000000518bcde8000000006a168bcde8000000008b8ea8ca00008b550033c085c9"
        "0f94c08bcd894424508985c4000000ff52786a0f8d442418680000000050c7837403000005000000c783780300006801"
        "0000ffd783c40c8d4c2414518bcbe800000000bd01000000899ec4b8000089aec8b80000899e18ba00008d9e20ba0000"
        "6a0a8d5424186800000000c7861cba0000ffffffff5289ab74030000c7837803000063000000ffd783c40c8d4424148b"
        "cb50e800000000899ee8be000089aeecbe0000899e3cc00000c78640c00000ffffffffe80000000085c074468b9644c0"
        "00008d8e44c0000033ff89b9c4000000ff52788b86c0c300008d8ec0c3000089b9c4000000ff50788b9614c500008d"
        "8e14c5000089b9c4000000ff5278e9880000008d9e44c000006a088d442418680000000050c7837403000002000000c7"
        "837803000008000000ffd783c40c8d4c2414518bcbe8000000008b7c24508b138bcb89bbc4000000ff52788d8ec0c300"
        "0089994c01000089a9500100008b0189b9c4000000ff50788d8e14c5000089994c010000c78150010000ffffffff8b11"
        "89b9c4000000ff527833ff558d8e68c60000e800000000578d8eccc70000e800000000578bcee8000000008b4c24408b"
        "c65f5e5d64890d000000005b83c43cc20400"
    )
    + b"\x90"
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[:83]
    + (
        (0x410, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (0x41A, IMAGE_REL_I386_DIR32, '??_C@_05LEAO@?$CF?421s?$AA@'),
        (0x429, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x42E, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (0x435, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x43E, IMAGE_REL_I386_REL32, '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z'),
        (0x454, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x45D, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x487, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x4AD, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x4D5, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x501, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x522, IMAGE_REL_I386_REL32, '?GetNetworkModemEnabled@zOpt@@YIHXZ'),
        (0x57D, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x5A3, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x600, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
        (0x60C, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
        (0x614, IMAGE_REL_I386_REL32, '?SetChildFlags@HudUiContainer@@QAEXI@Z'),
        (0x624, IMAGE_REL_I386_DIR32, '__except_list'),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[:259]
    + (
        (0x40E, 'mov'),
        (0x414, 'push'),
        (0x415, 'lea'),
        (0x419, 'push'),
        (0x41E, 'push'),
        (0x41F, 'call'),
        (0x421, 'add'),
        (0x424, 'mov'),
        (0x426, 'push'),
        (0x428, 'call'),
        (0x42D, 'push'),
        (0x432, 'mov'),
        (0x434, 'call'),
        (0x439, 'push'),
        (0x43B, 'mov'),
        (0x43D, 'call'),
        (0x442, 'lea'),
        (0x446, 'mov'),
        (0x450, 'push'),
        (0x451, 'mov'),
        (0x453, 'call'),
        (0x458, 'push'),
        (0x45A, 'mov'),
        (0x45C, 'call'),
        (0x461, 'mov'),
        (0x467, 'mov'),
        (0x46A, 'xor'),
        (0x46C, 'test'),
        (0x46E, 'sete'),
        (0x471, 'mov'),
        (0x473, 'mov'),
        (0x477, 'mov'),
        (0x47D, 'call'),
        (0x480, 'push'),
        (0x482, 'lea'),
        (0x486, 'push'),
        (0x48B, 'push'),
        (0x48C, 'mov'),
        (0x496, 'mov'),
        (0x4A0, 'call'),
        (0x4A2, 'add'),
        (0x4A5, 'lea'),
        (0x4A9, 'push'),
        (0x4AA, 'mov'),
        (0x4AC, 'call'),
        (0x4B1, 'mov'),
        (0x4B6, 'mov'),
        (0x4BC, 'mov'),
        (0x4C2, 'mov'),
        (0x4C8, 'lea'),
        (0x4CE, 'push'),
        (0x4D0, 'lea'),
        (0x4D4, 'push'),
        (0x4D9, 'mov'),
        (0x4E3, 'push'),
        (0x4E4, 'mov'),
        (0x4EA, 'mov'),
        (0x4F4, 'call'),
        (0x4F6, 'add'),
        (0x4F9, 'lea'),
        (0x4FD, 'mov'),
        (0x4FF, 'push'),
        (0x500, 'call'),
        (0x505, 'mov'),
        (0x50B, 'mov'),
        (0x511, 'mov'),
        (0x517, 'mov'),
        (0x521, 'call'),
        (0x526, 'test'),
        (0x528, 'je'),
        (0x52A, 'mov'),
        (0x530, 'lea'),
        (0x536, 'xor'),
        (0x538, 'mov'),
        (0x53E, 'call'),
        (0x541, 'mov'),
        (0x547, 'lea'),
        (0x54D, 'mov'),
        (0x553, 'call'),
        (0x556, 'mov'),
        (0x55C, 'lea'),
        (0x562, 'mov'),
        (0x568, 'call'),
        (0x56B, 'jmp'),
        (0x570, 'lea'),
        (0x576, 'push'),
        (0x578, 'lea'),
        (0x57C, 'push'),
        (0x581, 'push'),
        (0x582, 'mov'),
        (0x58C, 'mov'),
        (0x596, 'call'),
        (0x598, 'add'),
        (0x59B, 'lea'),
        (0x59F, 'push'),
        (0x5A0, 'mov'),
        (0x5A2, 'call'),
        (0x5A7, 'mov'),
        (0x5AB, 'mov'),
        (0x5AD, 'mov'),
        (0x5AF, 'mov'),
        (0x5B5, 'call'),
        (0x5B8, 'lea'),
        (0x5BE, 'mov'),
        (0x5C4, 'mov'),
        (0x5CA, 'mov'),
        (0x5CC, 'mov'),
        (0x5D2, 'call'),
        (0x5D5, 'lea'),
        (0x5DB, 'mov'),
        (0x5E1, 'mov'),
        (0x5EB, 'mov'),
        (0x5ED, 'mov'),
        (0x5F3, 'call'),
        (0x5F6, 'xor'),
        (0x5F8, 'push'),
        (0x5F9, 'lea'),
        (0x5FF, 'call'),
        (0x604, 'push'),
        (0x605, 'lea'),
        (0x60B, 'call'),
        (0x610, 'push'),
        (0x611, 'mov'),
        (0x613, 'call'),
        (0x618, 'mov'),
        (0x61C, 'mov'),
        (0x61E, 'pop'),
        (0x61F, 'pop'),
        (0x620, 'pop'),
        (0x621, 'mov'),
        (0x628, 'pop'),
        (0x629, 'add'),
        (0x62C, 'ret'),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260804-010: the formatted player-name Update now immediately follows
# sprintf.  Refresh the complete authority while preserving the same caller
# extent and every earlier reviewed call-contract row.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[:0x424]
    + bytes.fromhex(
        "8d4c2428518bcde8000000006a158bcde80000000068000000008bcde800000000"
        "6a008bcde8000000006a168bcdc7864ca9000000000000"
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[0x45C:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[:85]
    + (
        (
            0x42C,
            IMAGE_REL_I386_REL32,
            '?Update@HudUiNumericTextInput@@QAEXPBD@Z',
        ),
        (
            0x435,
            IMAGE_REL_I386_REL32,
            '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z',
        ),
        (0x43A, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (
            0x441,
            IMAGE_REL_I386_REL32,
            '?Update@HudUiNumericTextInput@@QAEXPBD@Z',
        ),
        (
            0x44A,
            IMAGE_REL_I386_REL32,
            '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z',
        ),
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[90:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[:266]
    + (
        (0x424, 'lea'),
        (0x428, 'push'),
        (0x429, 'mov'),
        (0x42B, 'call'),
        (0x430, 'push'),
        (0x432, 'mov'),
        (0x434, 'call'),
        (0x439, 'push'),
        (0x43E, 'mov'),
        (0x440, 'call'),
        (0x445, 'push'),
        (0x447, 'mov'),
        (0x449, 'call'),
        (0x44E, 'push'),
        (0x450, 'mov'),
        (0x452, 'mov'),
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[282:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260805-001: the dependency-closed enabledForNewSession and embedded
# game-name SetZrdWidgetEnabled block now precedes the later empty-string
# initialization.  Preserve one complete current-candidate authority.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[:0x439]
    + bytes.fromhex(
        "8b8ea8ca00008b550033c085c90f94c08bcd894424508985c4000000ff5278"
        "68000000008bcde8000000006a008bcde8000000006a168bcdc7864ca90000"
        "00000000e800000000"
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[0x480:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[:87]
    + (
        (0x459, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (
            0x460,
            IMAGE_REL_I386_REL32,
            '?Update@HudUiNumericTextInput@@QAEXPBD@Z',
        ),
        (
            0x469,
            IMAGE_REL_I386_REL32,
            '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z',
        ),
        (
            0x47C,
            IMAGE_REL_I386_REL32,
            '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z',
        ),
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[91:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[:273]
    + (
        (0x439, 'mov'),
        (0x43F, 'mov'),
        (0x442, 'xor'),
        (0x444, 'test'),
        (0x446, 'sete'),
        (0x449, 'mov'),
        (0x44B, 'mov'),
        (0x44F, 'mov'),
        (0x455, 'call'),
        (0x458, 'push'),
        (0x45D, 'mov'),
        (0x45F, 'call'),
        (0x464, 'push'),
        (0x466, 'mov'),
        (0x468, 'call'),
        (0x46D, 'push'),
        (0x46F, 'mov'),
        (0x471, 'mov'),
        (0x47B, 'call'),
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[292:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260805-002: the complete time-limit InitClampedInput sequence now
# immediately follows the embedded-widget enabled-state block.  Refresh the
# complete authority while retaining the reviewed shared sprintf IAT load.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[:0x458]
    + bytes.fromhex(
        "6a0f8d442418680000000050c7837403000005000000c7837803000068010000"
        "ffd783c40c8d4c2414518bcbe80000000068000000008bcde8000000006a00"
        "8bcde8000000006a168bcdc7864ca9000000000000"
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[0x4AC:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[:87]
    + (
        (0x45F, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (
            0x485,
            IMAGE_REL_I386_REL32,
            '?Update@HudUiNumericTextInput@@QAEXPBD@Z',
        ),
        (0x48A, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (
            0x491,
            IMAGE_REL_I386_REL32,
            '?Update@HudUiNumericTextInput@@QAEXPBD@Z',
        ),
        (
            0x49A,
            IMAGE_REL_I386_REL32,
            '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z',
        ),
        (
            0x4AD,
            IMAGE_REL_I386_REL32,
            '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z',
        ),
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[93:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[:283]
    + (
        (0x45A, 'lea'),
        (0x45E, 'push'),
        (0x463, 'push'),
        (0x464, 'mov'),
        (0x46E, 'mov'),
        (0x478, 'call'),
        (0x47A, 'add'),
        (0x47D, 'lea'),
        (0x481, 'push'),
        (0x482, 'mov'),
        (0x484, 'call'),
        (0x489, 'push'),
        (0x48E, 'mov'),
        (0x490, 'call'),
        (0x495, 'push'),
        (0x497, 'mov'),
        (0x499, 'call'),
        (0x49E, 'push'),
        (0x4A0, 'mov'),
        (0x4A2, 'mov'),
    )
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[303:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260805-003: moving the complete kills-input InitClampedInput sequence
# changes the current caller tail, relocation population, and padding extent.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_SIZE = 0x640


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[:0x489]
    + bytes.fromhex(
        "8dbe20ba00006a0a8d542418680000000052c7877403000001000000c7877803"
        "000063000000ff150000000083c40c8d4424148bcf50e8000000006800000000"
        "8bcde8000000006a008bcde8000000006a168bcdc7864ca9000000000000e800"
        "000000b80100000083cdff899ec4b800008986c8b80000899e18ba000089ae1c"
        "ba000089bee8be00008986ecbe000089be3cc0000089ae40c00000e800000000"
        "85c074468b9644c000008d8e44c0000033ff89b9c4000000ff52788b86c0c300"
        "008d8ec0c3000089b9c4000000ff50788b9614c500008d8e14c5000089b9c4"
        "000000ff5278e98c0000008dbe44c000006a088d442418680000000050c78774"
        "03000002000000c7877803000008000000ff150000000083c40c8d4c2414518b"
        "cfe8000000008b5c24508b178bcf899fc4000000ff52788d8ec0c3000089b94c"
        "010000c78150010000010000008b018999c4000000ff50788d8e14c5000089b9"
        "4c01000089a9500100008b118999c4000000ff527833ff6a018d8e68c60000e8"
        "00000000578d8eccc70000e800000000578bcee8000000008b4c24408bc65f5e"
        "5d64890d000000005b83c43cc20400909090909090909090"
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[:89]
    + (
        (0x496, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x4B1, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (
            0x4C0,
            IMAGE_REL_I386_REL32,
            '?Update@HudUiNumericTextInput@@QAEXPBD@Z',
        ),
        (0x4C5, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (
            0x4CC,
            IMAGE_REL_I386_REL32,
            '?Update@HudUiNumericTextInput@@QAEXPBD@Z',
        ),
        (
            0x4D5,
            IMAGE_REL_I386_REL32,
            '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z',
        ),
        (
            0x4E8,
            IMAGE_REL_I386_REL32,
            '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z',
        ),
        (0x525, IMAGE_REL_I386_REL32, '?GetNetworkModemEnabled@zOpt@@YIHXZ'),
        (0x580, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x59B, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (
            0x5AA,
            IMAGE_REL_I386_REL32,
            '?Update@HudUiNumericTextInput@@QAEXPBD@Z',
        ),
        (
            0x608,
            IMAGE_REL_I386_REL32,
            '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z',
        ),
        (
            0x614,
            IMAGE_REL_I386_REL32,
            '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z',
        ),
        (
            0x61C,
            IMAGE_REL_I386_REL32,
            '?SetChildFlags@HudUiContainer@@QAEXI@Z',
        ),
        (0x62C, IMAGE_REL_I386_DIR32, '__except_list'),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[:294]
    + (
        (0x489, 'lea'), (0x48F, 'push'), (0x491, 'lea'),
        (0x495, 'push'), (0x49A, 'push'), (0x49B, 'mov'),
        (0x4A5, 'mov'), (0x4AF, 'call'), (0x4B5, 'add'),
        (0x4B8, 'lea'), (0x4BC, 'mov'), (0x4BE, 'push'),
        (0x4BF, 'call'), (0x4C4, 'push'), (0x4C9, 'mov'),
        (0x4CB, 'call'), (0x4D0, 'push'), (0x4D2, 'mov'),
        (0x4D4, 'call'), (0x4D9, 'push'), (0x4DB, 'mov'),
        (0x4DD, 'mov'), (0x4E7, 'call'), (0x4EC, 'mov'),
        (0x4F1, 'or'), (0x4F4, 'mov'), (0x4FA, 'mov'),
        (0x500, 'mov'), (0x506, 'mov'), (0x50C, 'mov'),
        (0x512, 'mov'), (0x518, 'mov'), (0x51E, 'mov'),
        (0x524, 'call'), (0x529, 'test'), (0x52B, 'je'),
        (0x52D, 'mov'), (0x533, 'lea'), (0x539, 'xor'),
        (0x53B, 'mov'), (0x541, 'call'), (0x544, 'mov'),
        (0x54A, 'lea'), (0x550, 'mov'), (0x556, 'call'),
        (0x559, 'mov'), (0x55F, 'lea'), (0x565, 'mov'),
        (0x56B, 'call'), (0x56E, 'jmp'), (0x573, 'lea'),
        (0x579, 'push'), (0x57B, 'lea'), (0x57F, 'push'),
        (0x584, 'push'), (0x585, 'mov'), (0x58F, 'mov'),
        (0x599, 'call'), (0x59F, 'add'), (0x5A2, 'lea'),
        (0x5A6, 'push'), (0x5A7, 'mov'), (0x5A9, 'call'),
        (0x5AE, 'mov'), (0x5B2, 'mov'), (0x5B4, 'mov'),
        (0x5B6, 'mov'), (0x5BC, 'call'), (0x5BF, 'lea'),
        (0x5C5, 'mov'), (0x5CB, 'mov'), (0x5D5, 'mov'),
        (0x5D7, 'mov'), (0x5DD, 'call'), (0x5E0, 'lea'),
        (0x5E6, 'mov'), (0x5EC, 'mov'), (0x5F2, 'mov'),
        (0x5F4, 'mov'), (0x5FA, 'call'), (0x5FD, 'xor'),
        (0x5FF, 'push'), (0x601, 'lea'), (0x607, 'call'),
        (0x60C, 'push'), (0x60D, 'lea'), (0x613, 'call'),
        (0x618, 'push'), (0x619, 'mov'), (0x61B, 'call'),
        (0x620, 'mov'), (0x624, 'mov'), (0x626, 'pop'),
        (0x627, 'pop'), (0x628, 'pop'), (0x629, 'mov'),
        (0x630, 'pop'), (0x631, 'add'), (0x634, 'ret'),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260805-004: restoring the intact modem if/else changes register
# allocation from the child-construction prefix onward and grows the exact
# current caller tail without changing its reviewed linked extent.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_SIZE = 0x650


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[:0xAC]
    + bytes.fromhex(
        "befcb300006a048bcfc644244c06e800000000c707000000008dbe78b70000c6"
        "442448078bcfe800000000c707000000008dbeccb80000c6442448088bcfe800"
        "000000c707000000008dbe20ba00006a028bcfc644244c09e800000000c70700"
        "0000008dbe9cbd0000c64424480a8bcfe800000000c707000000008dbef0be00"
        "00c64424480b8bcfe800000000c70700000000c64424480c8d9e44c000006a02"
        "8bcbe800000000c703000000008dbec0c30000c64424480d8bcfe800000000c7"
        "07000000008dbe14c50000c64424480e8bcfe800000000c707000000008dbe68"
        "c60000c64424480f8bcfe800000000c707000000008dbeccc70000c644244810"
        "8bcfe800000000c707000000008d8e30c900006a00c644244c11e8000000008d"
        "8eecc900006a00c644244c12e8000000006a00680000000068000000008bcec6"
        "44245413c70600000000e8000000008b4c24508bf833c0ba010000003bf88986"
        "c4b800008996c8b80000898618ba000089961cba00008986e8be00008996ecbe"
        "000089863cc00000899640c0000089860cc50000899610c50000898660c60000"
        "899664c60000898ea8ca00000f847d0100008d8630c90000680000000050578b"
        "cee8000000008d86ecc90000680000000050578bcee8000000008d8650a90000"
        "680000000050578bcee8000000008d869caa0000680000000050578bcee80000"
        "0000680000000055578bcee8000000008d865caf0000680000000050578bcee8"
        "000000008d8664b10000680000000050578bcee8000000008d86b0b200006800"
        "00000050578bcee8000000008d86fcb30000680000000050578bcee800000000"
        "8d8678b70000680000000050578bcee8000000008d86ccb80000680000000050"
        "578bcee8000000008d8620ba0000680000000050578bcee8000000008d869cbd"
        "0000680000000050578bcee8000000008d86f0be0000680000000050578bcee8"
        "00000000680000000053578bcee8000000008d86c0c30000680000000050578b"
        "cee8000000008d8614c50000680000000050578bcee8000000008d8668c60000"
        "680000000050578bcee8000000008d86ccc70000680000000050578bcee80000"
        "0000578bcee800000000ba010000008b8630c900008d8e30c9000052ff50608b"
        "96ecc900008d8eecc900006a00ff52606a008d8e5caf0000e800000000e80000"
        "0000508d44242c680000000050ff150000000083c40c8d4c2428518bcde80000"
        "00006a158bcde8000000008b8ea8ca00008b550033c085c90f94c08bcd894424"
        "508985c4000000ff52788dbefcb300006a0f8d442418680000000050c7877403"
        "000005000000c7877803000068010000ff150000000083c40c8d4c2414518bcf"
        "e8000000006a0a8d542418680000000052c78694bd000001000000c78698bd00"
        "0063000000ff150000000083c40c8d4424148d8e20ba000050e800000000e800"
        "00000085c074458b138bcbc783c400000000000000ff52788b86c0c300008d8e"
        "c0c3000033db8999c4000000ff50788b9614c500008d8e14c500008999c40000"
        "00ff527883cbffe98f0000006a088d442418680000000050c783740300000200"
        "0000c7837803000008000000ff150000000083c40c8d4c2414518bcbe8000000"
        "008b5424508b038bcb8993c4000000ff50788b5424508d8ec0c3000089994c01"
        "0000c78150010000010000008b018991c4000000ff50788b5424508d8e14c500"
        "0089994c01000083cbff8999500100008b018991c4000000ff50786800000000"
        "8bcde8000000006a008bcde8000000006a168bcdc7864ca9000000000000e800"
        "000000b90100000089bec4b80000898ec8b8000089be18ba0000899e1cba0000"
        "8d8620ba0000898eecbe0000518986e8be00008d8e68c6000089863cc0000089"
        "9e40c00000e8000000006a008d8eccc70000e8000000006a008bcee800000000"
        "8b4c24408bc65f5e5d64890d000000005b83c43cc20400909090909090909090"
        "90909090"
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[:58]
    + (
        (0x2FF, IMAGE_REL_I386_DIR32, '??_C@_0L@GGHB@TIME_LIMIT?$AA@'),
        (0x308, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x313, IMAGE_REL_I386_DIR32, '??_C@_0P@OBMI@INC_TIME_LIMIT?$AA@'),
        (0x31C, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x327, IMAGE_REL_I386_DIR32, '??_C@_0P@LKIO@DEC_TIME_LIMIT?$AA@'),
        (0x330, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x33B, IMAGE_REL_I386_DIR32, '??_C@_05FIEB@KILLS?$AA@'),
        (0x344, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x34F, IMAGE_REL_I386_DIR32, '??_C@_09EBMJ@INC_KILLS?$AA@'),
        (0x358, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x363, IMAGE_REL_I386_DIR32, '??_C@_09BOH@DEC_KILLS?$AA@'),
        (0x36C, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x371, IMAGE_REL_I386_DIR32, '??_C@_0M@KFEM@MAX_PLAYERS?$AA@'),
        (0x37A, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x385, IMAGE_REL_I386_DIR32, '??_C@_0BA@GPML@INC_MAX_PLAYERS?$AA@'),
        (0x38E, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x399, IMAGE_REL_I386_DIR32, '??_C@_0BA@FAJA@DEC_MAX_PLAYERS?$AA@'),
        (0x3A2, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x3AD, IMAGE_REL_I386_DIR32, '??_C@_0L@BOHA@ALLOW_MAPS?$AA@'),
        (0x3B6, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x3C1, IMAGE_REL_I386_DIR32, '??_C@_09LBCG@NAME_TAGS?$AA@'),
        (0x3CA, IMAGE_REL_I386_REL32, '?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z'),
        (0x3D2, IMAGE_REL_I386_REL32, '?FreeLoadedTreeRoots@HudUiBackground@@QAEXH@Z'),
        (0x405, IMAGE_REL_I386_REL32, '?SetIndexClamped@HudUiCycleSelectorWidget@@QAEHH@Z'),
        (0x40A, IMAGE_REL_I386_REL32, '?zOptGetPlayerName@@YIPADXZ'),
        (0x414, IMAGE_REL_I386_DIR32, '??_C@_05LEAO@?$CF?421s?$AA@'),
        (0x41B, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (0x42A, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x433, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x463, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x47E, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (0x48D, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x498, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x4B3, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (0x4C6, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x4CB, IMAGE_REL_I386_REL32, '?GetNetworkModemEnabled@zOpt@@YIHXZ'),
        (0x51F, IMAGE_REL_I386_DIR32, '??_C@_02MECO@?$CFd?$AA@'),
        (0x53A, IMAGE_REL_I386_DIR32, '__imp__sprintf'),
        (0x549, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x5A8, IMAGE_REL_I386_DIR32, '??_C@_00A@?$AA@'),
        (0x5AF, IMAGE_REL_I386_REL32, '?Update@HudUiNumericTextInput@@QAEXPBD@Z'),
        (0x5B8, IMAGE_REL_I386_REL32, '?SetInputActive@HudUiNumericTextInput@@QAEHH@Z'),
        (0x5CB, IMAGE_REL_I386_REL32, '?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z'),
        (0x612, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
        (0x61F, IMAGE_REL_I386_REL32, '?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z'),
        (0x628, IMAGE_REL_I386_REL32, '?SetChildFlags@HudUiContainer@@QAEXI@Z'),
        (0x638, IMAGE_REL_I386_DIR32, '__except_list'),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS[:178]
    + (
        (0x2F8, 'lea'),
        (0x2FE, 'push'), (0x303, 'push'), (0x304, 'push'), (0x305, 'mov'),
        (0x307, 'call'), (0x30C, 'lea'), (0x312, 'push'), (0x317, 'push'),
        (0x318, 'push'), (0x319, 'mov'), (0x31B, 'call'), (0x320, 'lea'),
        (0x326, 'push'), (0x32B, 'push'), (0x32C, 'push'), (0x32D, 'mov'),
        (0x32F, 'call'), (0x334, 'lea'), (0x33A, 'push'), (0x33F, 'push'),
        (0x340, 'push'), (0x341, 'mov'), (0x343, 'call'), (0x348, 'lea'),
        (0x34E, 'push'), (0x353, 'push'), (0x354, 'push'), (0x355, 'mov'),
        (0x357, 'call'), (0x35C, 'lea'), (0x362, 'push'), (0x367, 'push'),
        (0x368, 'push'), (0x369, 'mov'), (0x36B, 'call'), (0x370, 'push'),
        (0x375, 'push'), (0x376, 'push'), (0x377, 'mov'), (0x379, 'call'),
        (0x37E, 'lea'), (0x384, 'push'), (0x389, 'push'), (0x38A, 'push'),
        (0x38B, 'mov'), (0x38D, 'call'), (0x392, 'lea'), (0x398, 'push'),
        (0x39D, 'push'), (0x39E, 'push'), (0x39F, 'mov'), (0x3A1, 'call'),
        (0x3A6, 'lea'), (0x3AC, 'push'), (0x3B1, 'push'), (0x3B2, 'push'),
        (0x3B3, 'mov'), (0x3B5, 'call'), (0x3BA, 'lea'), (0x3C0, 'push'),
        (0x3C5, 'push'), (0x3C6, 'push'), (0x3C7, 'mov'), (0x3C9, 'call'),
        (0x3CE, 'push'), (0x3CF, 'mov'), (0x3D1, 'call'), (0x3D6, 'mov'),
        (0x3DB, 'mov'), (0x3E1, 'lea'), (0x3E7, 'push'), (0x3E8, 'call'),
        (0x3EB, 'mov'), (0x3F1, 'lea'), (0x3F7, 'push'), (0x3F9, 'call'),
        (0x3FC, 'push'), (0x3FE, 'lea'), (0x404, 'call'), (0x409, 'call'),
        (0x40E, 'push'), (0x40F, 'lea'), (0x413, 'push'), (0x418, 'push'),
        (0x419, 'call'), (0x41F, 'add'), (0x422, 'lea'), (0x426, 'push'),
        (0x427, 'mov'), (0x429, 'call'), (0x42E, 'push'), (0x430, 'mov'),
        (0x432, 'call'), (0x437, 'mov'), (0x43D, 'mov'), (0x440, 'xor'),
        (0x442, 'test'), (0x444, 'sete'), (0x447, 'mov'), (0x449, 'mov'),
        (0x44D, 'mov'), (0x453, 'call'), (0x456, 'lea'), (0x45C, 'push'),
        (0x45E, 'lea'), (0x462, 'push'), (0x467, 'push'), (0x468, 'mov'),
        (0x472, 'mov'), (0x47C, 'call'), (0x482, 'add'), (0x485, 'lea'),
        (0x489, 'push'), (0x48A, 'mov'), (0x48C, 'call'), (0x491, 'push'),
        (0x493, 'lea'), (0x497, 'push'), (0x49C, 'push'), (0x49D, 'mov'),
        (0x4A7, 'mov'), (0x4B1, 'call'), (0x4B7, 'add'), (0x4BA, 'lea'),
        (0x4BE, 'lea'), (0x4C4, 'push'), (0x4C5, 'call'), (0x4CA, 'call'),
        (0x4CF, 'test'), (0x4D1, 'je'), (0x4D3, 'mov'), (0x4D5, 'mov'),
        (0x4D7, 'mov'), (0x4E1, 'call'), (0x4E4, 'mov'), (0x4EA, 'lea'),
        (0x4F0, 'xor'), (0x4F2, 'mov'), (0x4F8, 'call'), (0x4FB, 'mov'),
        (0x501, 'lea'), (0x507, 'mov'), (0x50D, 'call'), (0x510, 'or'),
        (0x513, 'jmp'), (0x518, 'push'), (0x51A, 'lea'), (0x51E, 'push'),
        (0x523, 'push'), (0x524, 'mov'), (0x52E, 'mov'), (0x538, 'call'),
        (0x53E, 'add'), (0x541, 'lea'), (0x545, 'push'), (0x546, 'mov'),
        (0x548, 'call'), (0x54D, 'mov'), (0x551, 'mov'), (0x553, 'mov'),
        (0x555, 'mov'), (0x55B, 'call'), (0x55E, 'mov'), (0x562, 'lea'),
        (0x568, 'mov'), (0x56E, 'mov'), (0x578, 'mov'), (0x57A, 'mov'),
        (0x580, 'call'), (0x583, 'mov'), (0x587, 'lea'), (0x58D, 'mov'),
        (0x593, 'or'), (0x596, 'mov'), (0x59C, 'mov'), (0x59E, 'mov'),
        (0x5A4, 'call'), (0x5A7, 'push'), (0x5AC, 'mov'), (0x5AE, 'call'),
        (0x5B3, 'push'), (0x5B5, 'mov'), (0x5B7, 'call'), (0x5BC, 'push'),
        (0x5BE, 'mov'), (0x5C0, 'mov'), (0x5CA, 'call'), (0x5CF, 'mov'),
        (0x5D4, 'mov'), (0x5DA, 'mov'), (0x5E0, 'mov'), (0x5E6, 'mov'),
        (0x5EC, 'lea'), (0x5F2, 'mov'), (0x5F8, 'push'), (0x5F9, 'mov'),
        (0x5FF, 'lea'), (0x605, 'mov'), (0x60B, 'mov'), (0x611, 'call'),
        (0x616, 'push'), (0x618, 'lea'), (0x61E, 'call'), (0x623, 'push'),
        (0x625, 'mov'), (0x627, 'call'), (0x62C, 'mov'), (0x630, 'mov'),
        (0x632, 'pop'), (0x633, 'pop'), (0x634, 'pop'), (0x635, 'mov'),
        (0x63C, 'pop'), (0x63D, 'add'), (0x640, 'ret'),
    )
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_OFFSETS = tuple(
    row[0] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_MNEMONICS = tuple(
    row[1] for row in HUD_NET_GAME_SETUP_CANDIDATE_CALLER_COD_ROWS
)


# WSI-20260805-005: the maxPlayersInput direct member store preserves the
# complete caller size/COD topology while changing three object bytes and the
# compiler-local exception label.  Keep both in the exact current snapshot.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[:0x4D8]
    + b"\x86\x08\xc1"
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[0x4DB:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    ((0x003, IMAGE_REL_I386_DIR32, '$L86194'),)
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[1:]
)


# WSI-20260805-008: the actual maxPlayersInput else-branch direct state
# store changes three object bytes and the compiler-local exception label.
# Its complete caller size, relocation population, mask, and COD rows remain
# otherwise exact and retain the same reviewed linked extent.
HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY = (
    HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[:0x59F]
    + b"\x96\xd8\xc5"
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_BODY[0x5A2:]
)


HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS = (
    ((0x003, IMAGE_REL_I386_DIR32, '$L87168'),)
    + HUD_NET_GAME_SETUP_CANDIDATE_CALLER_RELOCATIONS[1:]
)


# Exact retained r4658 source-convergent constructor artifact.  This profile
# authenticates the complete body and substantive relocation population while
# the numeric VC5 exception-local suffix is bound separately by section/value.
HUD_NET_GAME_SETUP_RETAINED_RETAIL_BODY = bytes.fromhex(
    "6aff680000000064a100000000506489250000000083ec305355568bf15789742410e8000000008d8e50a90000c74424"
    "4800000000e8000000008d8e9caa0000c644244801e8000000008d9ee8ab0000c6442448028bcbe800000000c7030000"
    "00008dbe5caf0000c6442448038bcfe800000000c707000000008dbe64b10000c6442448048bcfe800000000c7070000"
    "00008dbeb0b20000c6442448058bcfe800000000c707000000008dbefcb300006a048bcfc644244c06e800000000c707"
    "000000008dbe78b70000c6442448078bcfe800000000c707000000008dbeccb80000c6442448088bcfe800000000c707"
    "000000008dbe20ba00006a028bcfc644244c09e800000000c707000000008dbe9cbd0000c64424480a8bcfe800000000"
    "c707000000008dbef0be0000c64424480b8bcfe800000000c70700000000c64424480c8dae44c000006a028bcde80000"
    "0000c74500000000008dbec0c30000c64424480d8bcfe800000000c707000000008dbe14c50000c64424480e8bcfe800"
    "000000c707000000008dbe68c60000c64424480f8bcfe800000000c707000000008dbeccc70000c6442448108bcfe800"
    "000000c707000000008d8e30c900006a00c644244c11e8000000008d8eecc900006a00c644244c12e8000000006a0068"
    "0000000068000000008bcec644245413c70600000000e8000000008b4c24508bf833c0ba010000003bf88986c4b80000"
    "8996c8b80000898618ba000089961cba00008986e8be00008996ecbe000089863cc00000899640c0000089860cc50000"
    "899610c50000898660c60000899664c60000898ea8ca00000f847d0100008d8630c90000680000000050578bcee80000"
    "00008d86ecc90000680000000050578bcee8000000008d8650a90000680000000050578bcee8000000008d869caa0000"
    "680000000050578bcee800000000680000000053578bcee8000000008d865caf0000680000000050578bcee800000000"
    "8d8664b10000680000000050578bcee8000000008d86b0b20000680000000050578bcee8000000008d86fcb300006800"
    "00000050578bcee8000000008d8678b70000680000000050578bcee8000000008d86ccb80000680000000050578bcee8"
    "000000008d8620ba0000680000000050578bcee8000000008d869cbd0000680000000050578bcee8000000008d86f0be"
    "0000680000000050578bcee800000000680000000055578bcee8000000008d86c0c30000680000000050578bcee80000"
    "00008d8614c50000680000000050578bcee8000000008d8668c60000680000000050578bcee8000000008d86ccc70000"
    "680000000050578bcee800000000578bcee800000000ba010000008b8630c900008d8e30c9000052ff50608b96ecc900"
    "008d8eecc900006a00ff52606a008d8e5caf0000e800000000e8000000008b3d00000000508d44242c680000000050ff"
    "d783c40c8d4c2428518bcbe8000000006a158bcbe8000000008b8ea8ca00008b1333c085c90f94c08bcb894424508983"
    "c4000000ff52788d9efcb300006a0f8d442418680000000050c7837403000005000000c7837803000068010000ffd783"
    "c40c8d4c2414518bcbe8000000008dbe20ba00006a0a8d542418680000000052c7877403000001000000c78778030000"
    "63000000ff150000000083c40c8d4424148bcf50e800000000e80000000085c074468b55008bcdc78608c10000000000"
    "00ff52788b86c0c300008d8ec0c3000033ed89a9c4000000ff50788b9614c500008d8e14c5000089a9c4000000ff5278"
    "83cdffe9900000006a088d442418680000000050c7857403000002000000c7857803000008000000ff150000000083c4"
    "0c8d4c2414518bcde8000000008b5424508b45008bcd8995c4000000ff50788b5424508d8ec0c3000089a94c010000c7"
    "8150010000010000008b018991c4000000ff50788b5424508d8e14c5000089a94c01000083cdff89a9500100008b0189"
    "96d8c50000ff5078b801000000c7864ca9000000000000899ec4b800008986c8b80000899e18ba000089ae1cba000089"
    "bee8be00008986ecbe0000508d8e68c6000089be3cc0000089ae40c00000e8000000006a008d8eccc70000e800000000"
    "6a008bcee8000000008b4c24408bc65f5e5d64890d000000005b83c43cc20400"
)


# Exact current-source r4698 constructor artifact.  The source repairs retain
# the complete 0x620 extent, 65-call population, relocation coordinates, and
# return topology of the prior convergent profile while changing only these
# non-relocation instruction bytes.  Keeping the delta explicit makes drift in
# either the retained base or any repaired byte fail the full-body equality
# gate below; it does not project or synthesize any immutable retail call row.
_hud_net_game_setup_current_source_body = bytearray(
    HUD_NET_GAME_SETUP_RETAINED_RETAIL_BODY
)


for _offset, _value in (
    (0x1FC, 0x54),
    (0x202, 0xC9),
    (0x203, 0xB8),
    (0x209, 0xF9),
    (0x20B, 0x8E),
    (0x211, 0x86),
    (0x217, 0x8E),
    (0x21D, 0x86),
    (0x223, 0x8E),
    (0x229, 0x86),
    (0x22F, 0x8E),
    (0x235, 0x86),
    (0x23B, 0x8E),
    (0x241, 0x86),
    (0x247, 0x8E),
    (0x24D, 0x86),
    (0x253, 0x96),
    (0x3D6, 0xB8),
    (0x3DC, 0x96),
    (0x3E7, 0x50),
    (0x3E9, 0x52),
    (0x3EC, 0x86),
    (0x3FA, 0x50),
    (0x416, 0x4C),
    (0x41E, 0x51),
    (0x425, 0x54),
    (0x428, 0x8B),
    (0x429, 0xCB),
    (0x42A, 0x52),
    (0x43F, 0x33),
    (0x440, 0xC0),
    (0x441, 0x85),
    (0x442, 0xC9),
    (0x443, 0x0F),
    (0x444, 0x94),
    (0x445, 0xC0),
    (0x446, 0x89),
    (0x447, 0x44),
    (0x448, 0x24),
    (0x449, 0x50),
    (0x44B, 0x83),
    (0x44C, 0xC4),
    (0x44D, 0x00),
    (0x44E, 0x00),
    (0x44F, 0x00),
    (0x450, 0x8B),
    (0x451, 0x03),
    (0x452, 0x8B),
    (0x453, 0xCB),
    (0x455, 0x50),
    (0x460, 0x4C),
    (0x468, 0x51),
    (0x483, 0x54),
    (0x486, 0x8B),
    (0x487, 0xCB),
    (0x488, 0x52),
    (0x497, 0x44),
    (0x49F, 0x50),
    (0x4BE, 0x4C),
    (0x4C1, 0x51),
    (0x4C2, 0x8B),
    (0x4C3, 0xCF),
):
    _hud_net_game_setup_current_source_body[_offset] = _value


HUD_NET_GAME_SETUP_CURRENT_SOURCE_RETAIL_CONVERGENT_BODY = bytes(
    _hud_net_game_setup_current_source_body
)


del _hud_net_game_setup_current_source_body, _offset, _value


HUD_NET_GAME_SETUP_RETAINED_RETAIL_RELOCATIONS = (
    (0x3, 0x6, "$L87186"),
    (0x9, 0x6, "__except_list"),
    (0x11, 0x6, "__except_list"),
    (0x23, 0x14, "??0HudUiBackground@@QAE@XZ"),
    (0x36, 0x14, "??0HudUiNetGameSetupPanel_LaunchButton@@QAE@XZ"),
    (0x46, 0x14, "??0HudUiNetGameSetupPanel_CancelButton@@QAE@XZ"),
    (0x58, 0x14, "??0HudUiNumericTextInput@@QAE@XZ"),
    (0x5E, 0x6, "??_7HudUiNetGameSetupTextInput@@6B@"),
    (0x70, 0x14, "??0HudUiCycleSelectorWidget@@QAE@XZ"),
    (0x76, 0x6, "??_7HudUiNetGameSetupPanel_WorldSelector@@6B@"),
    (0x88, 0x14, "??0HudUiZrdWidget@@QAE@XZ"),
    (0x8E, 0x6, "??_7HudUiNetGameSetupPanel_NextWorldButton@@6B@"),
    (0xA0, 0x14, "??0HudUiZrdWidget@@QAE@XZ"),
    (0xA6, 0x6, "??_7HudUiNetGameSetupPanel_PrevWorldButton@@6B@"),
    (0xBA, 0x14, "??0HudUiClampedIntTextInput@@QAE@I@Z"),
    (0xC0, 0x6, "??_7CHudUiNetGameSetupPanelTimeLimitInput@@6B@"),
    (0xD2, 0x14, "??0HudUiZrdWidget@@QAE@XZ"),
    (0xD8, 0x6, "??_7HudUiNetGameSetupPanel_IncTimeLimitButton@@6B@"),
    (0xEA, 0x14, "??0HudUiZrdWidget@@QAE@XZ"),
    (0xF0, 0x6, "??_7HudUiNetGameSetupPanel_DecTimeLimitButton@@6B@"),
    (0x104, 0x14, "??0HudUiClampedIntTextInput@@QAE@I@Z"),
    (0x10A, 0x6, "??_7CHudUiNetGameSetupPanelKillsInput@@6B@"),
    (0x11C, 0x14, "??0HudUiZrdWidget@@QAE@XZ"),
    (0x122, 0x6, "??_7HudUiNetGameSetupPanel_IncKillsButton@@6B@"),
    (0x134, 0x14, "??0HudUiZrdWidget@@QAE@XZ"),
    (0x13A, 0x6, "??_7HudUiNetGameSetupPanel_DecKillsButton@@6B@"),
    (0x14E, 0x14, "??0HudUiClampedIntTextInput@@QAE@I@Z"),
    (0x155, 0x6, "??_7CHudUiNetGameSetupPanelMaxPlayersInput@@6B@"),
    (0x167, 0x14, "??0HudUiZrdWidget@@QAE@XZ"),
    (0x16D, 0x6, "??_7HudUiNetGameSetupPanel_IncMaxPlayersButton@@6B@"),
    (0x17F, 0x14, "??0HudUiZrdWidget@@QAE@XZ"),
    (0x185, 0x6, "??_7HudUiNetGameSetupPanel_DecMaxPlayersButton@@6B@"),
    (0x197, 0x14, "??0HudUiCheckToggleWidget@@QAE@XZ"),
    (0x19D, 0x6, "??_7HudUiNetGameSetupPanel_AllowMapsToggle@@6B@"),
    (0x1AF, 0x14, "??0HudUiCheckToggleWidget@@QAE@XZ"),
    (0x1B5, 0x6, "??_7HudUiNetGameSetupPanel_NameTagsToggle@@6B@"),
    (0x1C7, 0x14, "??0HudUiWidget@@QAE@I@Z"),
    (0x1D9, 0x14, "??0HudUiWidget@@QAE@I@Z"),
    (0x1E0, 0x6, "??_C@_0M@KEOC@MP_NEW_GAME?$AA@"),
    (0x1E5, 0x6, "??_C@_0L@NPAH@dialog?4zrd?$AA@"),
    (0x1F2, 0x6, "??_7HudUiNetGameSetupPanel@@6B@"),
    (0x1F7, 0x14, "?LoadFromZrd@HudUiBackground@@QAEPAUNode@zReader@@PBD0H@Z"),
    (0x265, 0x6, "??_C@_0N@BGIN@KILLS_SWITCH?$AA@"),
    (0x26E, 0x14, "?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiElement@@PBD@Z"),
    (0x279, 0x6, "??_C@_0M@EMP@LAPS_SWITCH?$AA@"),
    (0x282, 0x14, "?BindPrimitiveNodeToElement@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiElement@@PBD@Z"),
    (0x28D, 0x6, "??_C@_04BCHB@PLAY?$AA@"),
    (0x296, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x2A1, 0x6, "??_C@_06BAHB@CANCEL?$AA@"),
    (0x2AA, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x2AF, 0x6, "??_C@_09JKDJ@GAME_NAME?$AA@"),
    (0x2B8, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x2C3, 0x6, "??_C@_05KFHK@WORLD?$AA@"),
    (0x2CC, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x2D7, 0x6, "??_C@_09LMPC@INC_WORLD?$AA@"),
    (0x2E0, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x2EB, 0x6, "??_C@_09PMNM@DEC_WORLD?$AA@"),
    (0x2F4, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x2FF, 0x6, "??_C@_0L@GGHB@TIME_LIMIT?$AA@"),
    (0x308, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x313, 0x6, "??_C@_0P@OBMI@INC_TIME_LIMIT?$AA@"),
    (0x31C, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x327, 0x6, "??_C@_0P@LKIO@DEC_TIME_LIMIT?$AA@"),
    (0x330, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x33B, 0x6, "??_C@_05FIEB@KILLS?$AA@"),
    (0x344, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x34F, 0x6, "??_C@_09EBMJ@INC_KILLS?$AA@"),
    (0x358, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x363, 0x6, "??_C@_09BOH@DEC_KILLS?$AA@"),
    (0x36C, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x371, 0x6, "??_C@_0M@KFEM@MAX_PLAYERS?$AA@"),
    (0x37A, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x385, 0x6, "??_C@_0BA@GPML@INC_MAX_PLAYERS?$AA@"),
    (0x38E, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x399, 0x6, "??_C@_0BA@FAJA@DEC_MAX_PLAYERS?$AA@"),
    (0x3A2, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x3AD, 0x6, "??_C@_0L@BOHA@ALLOW_MAPS?$AA@"),
    (0x3B6, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x3C1, 0x6, "??_C@_09LBCG@NAME_TAGS?$AA@"),
    (0x3CA, 0x14, "?BindWidgetByName@HudUiBackground@@QAEHPAUNode@zReader@@PAUHudUiWidget@@PBD@Z"),
    (0x3D2, 0x14, "?FreeLoadedTreeRoots@HudUiBackground@@QAEXH@Z"),
    (0x405, 0x14, "?SetIndexClamped@HudUiCycleSelectorWidget@@QAEHH@Z"),
    (0x40A, 0x14, "?zOptGetPlayerName@@YIPADXZ"),
    (0x410, 0x6, "__imp__sprintf"),
    (0x41A, 0x6, "??_C@_05LEAO@?$CF?421s?$AA@"),
    (0x42C, 0x14, "?Update@HudUiNumericTextInput@@QAEXPBD@Z"),
    (0x435, 0x14, "?AllocTextBuffer@HudUiNumericTextInput@@QAEXI@Z"),
    (0x464, 0x6, "??_C@_02MECO@?$CFd?$AA@"),
    (0x48A, 0x14, "?Update@HudUiNumericTextInput@@QAEXPBD@Z"),
    (0x49B, 0x6, "??_C@_02MECO@?$CFd?$AA@"),
    (0x4B6, 0x6, "__imp__sprintf"),
    (0x4C5, 0x14, "?Update@HudUiNumericTextInput@@QAEXPBD@Z"),
    (0x4CA, 0x14, "?GetNetworkModemEnabled@zOpt@@YIHXZ"),
    (0x51F, 0x6, "??_C@_02MECO@?$CFd?$AA@"),
    (0x53A, 0x6, "__imp__sprintf"),
    (0x549, 0x14, "?Update@HudUiNumericTextInput@@QAEXPBD@Z"),
    (0x5EF, 0x14, "?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z"),
    (0x5FC, 0x14, "?SetChecked@HudUiCheckToggleWidget@@QAEHH@Z"),
    (0x605, 0x14, "?SetChildFlags@HudUiContainer@@QAEXI@Z"),
    (0x615, 0x6, "__except_list"),
)


HUD_NET_GAME_SETUP_RETAINED_RETAIL_CALL_OFFSETS = (
    0x22, 0x35, 0x45, 0x57, 0x6F, 0x87, 0x9F, 0xB9,
    0xD1, 0xE9, 0x103, 0x11B, 0x133, 0x14D, 0x166, 0x17E,
    0x196, 0x1AE, 0x1C6, 0x1D8, 0x1F6, 0x26D, 0x281, 0x295,
    0x2A9, 0x2B7, 0x2CB, 0x2DF, 0x2F3, 0x307, 0x31B, 0x32F,
    0x343, 0x357, 0x36B, 0x379, 0x38D, 0x3A1, 0x3B5, 0x3C9,
    0x3D1, 0x3E8, 0x3F9, 0x404, 0x409, 0x41F, 0x42B, 0x434,
    0x454, 0x47D, 0x489, 0x4B4, 0x4C4, 0x4C9, 0x4E1, 0x4F8,
    0x50D, 0x538, 0x548, 0x55C, 0x581, 0x5A5, 0x5EE, 0x5FB,
    0x604,
)


HUD_NET_GAME_SETUP_LAUNCH_BUTTON_CONSTRUCTOR_BODY = bytes.fromhex(
    "568bf1e800000000c706000000008bc65ec39090909090909090909090909090"
)


HUD_NET_GAME_SETUP_LAUNCH_BUTTON_CONSTRUCTOR_COD_OFFSETS = (
    0,
    1,
    3,
    8,
    14,
    16,
    17,
)


HUD_NET_GAME_SETUP_LAUNCH_BUTTON_CONSTRUCTOR_COD_MNEMONICS = (
    "push",
    "mov",
    "call",
    "mov",
    "mov",
    "pop",
    "ret",
)


HUD_ZRD_WIDGET_CONSTRUCTOR_VC5_TARGET_ID = (
    "recoil:vc5-target:hud_ui_zrd_widget_constructor"
)


HUD_ZRD_WIDGET_CONSTRUCTOR_VC5_TARGET_NAME = "hud_ui_zrd_widget_constructor"


HUD_ZRD_WIDGET_CONSTRUCTOR_VC5_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/hud_ui_zrd_widget_constructor.json"
)


HUD_ZRD_WIDGET_CONSTRUCTOR_ORDER_TARGET_ID = (
    "recoil:vc5-target:zui_4b3ce0_4bffe0_authored_order"
)


HUD_ZRD_WIDGET_CONSTRUCTOR_ORDER_TARGET_NAME = (
    "zui_4b3ce0_4bffe0_authored_order"
)


HUD_ZRD_WIDGET_CONSTRUCTOR_ORDER_TARGET_MANIFEST = (
    "tools/vc5_verify_targets/zui_4b3ce0_4bffe0_authored_order.json"
)


HUD_ZRD_WIDGET_CONSTRUCTOR_SOURCE_PATH = (
    "src/GameZRecoil/zUI/zui_widgets.cpp"
)


HUD_ZRD_WIDGET_CONSTRUCTOR_BODY = bytes.fromhex(
    "6aff680000000064a100000000506489250000000083ec085355565733ff8bf15789742418e8000000008a4424138d9e0c01"
    "0000897c24208803897b04897b08897b0c8a4c24138dae1c010000884d00897d04897d08897d0c8a54241389be30010000"
    "88962c01000089be3401000089be380100008a44241389be4001000088863c01000089be4401000089be48010000b80000"
    "803fc70600000000c786c40000000100000089bec000000089bebc00000089bec800000089bedc00000089bee400000089"
    "bee000000089bee80000008986f000000089beec00000089bef400000089bef800000089860001000089befc0000008b43"
    "088b7b043bc0c6442420048bc8740e8b1183c104891783c7043bc875f28b43088bcb5057e800000000897b088b4d088b7d"
    "043bc98bc1740e8b1083c004891783c7043bc175f28b45088bcd5057e800000000897d088b8e340100008bbe300100003b"
    "c98bc1740e8b1083c004891783c7043bc175f28b86340100008d9e2c01000050578bcbe800000000897b088b168bce66c7"
    "46400100ff52208a460c8b4c241883e0105f0c0289460c8bc65e5d64890d000000005b83c414c3"
)


HUD_ZRD_WIDGET_CONSTRUCTOR_RELOCATIONS = (
    (0x3, IMAGE_REL_I386_DIR32, '$L84570'),
    (0x9, IMAGE_REL_I386_DIR32, '__except_list'),
    (0x11, IMAGE_REL_I386_DIR32, '__except_list'),
    (0x26, IMAGE_REL_I386_REL32, '??0HudUiWidget@@QAE@I@Z'),
    (0x98, IMAGE_REL_I386_DIR32, '??_7HudUiZrdWidget@@6B@'),
    (
        0x11B,
        IMAGE_REL_I386_REL32,
        '?_Destroy@?$vector@PAUHudUiPanel@@V?$allocator@PAUHudUiPanel@@@std@@@std@@IAEXPAPAUHudUiPanel@@0@Z',
    ),
    (
        0x144,
        IMAGE_REL_I386_REL32,
        '?_Destroy@?$vector@PAUHudUiPanel@@V?$allocator@PAUHudUiPanel@@@std@@@std@@IAEXPAPAUHudUiPanel@@0@Z',
    ),
    (
        0x17C,
        IMAGE_REL_I386_REL32,
        '?_Destroy@?$vector@PAUHudUiPanel@@V?$allocator@PAUHudUiPanel@@@std@@@std@@IAEXPAPAUHudUiPanel@@0@Z',
    ),
    (0x1A7, IMAGE_REL_I386_DIR32, '__except_list'),
)


HUD_ZRD_WIDGET_CONSTRUCTOR_COD_OFFSETS = (
    0x0, 0x2, 0x7, 0xD, 0xE, 0x15, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1E,
    0x20, 0x21, 0x25, 0x2A, 0x2E, 0x34, 0x38, 0x3A, 0x3D, 0x40, 0x43,
    0x47, 0x4D, 0x50, 0x53, 0x56, 0x59, 0x5D, 0x63, 0x69, 0x6F, 0x75,
    0x79, 0x7F, 0x85, 0x8B, 0x91, 0x96, 0x9C, 0xA6, 0xAC, 0xB2, 0xB8,
    0xBE, 0xC4, 0xCA, 0xD0, 0xD6, 0xDC, 0xE2, 0xE8, 0xEE, 0xF4, 0xF7,
    0xFA, 0xFC, 0x101, 0x103, 0x105, 0x107, 0x10A, 0x10C, 0x10F, 0x111,
    0x113, 0x116, 0x118, 0x119, 0x11A, 0x11F, 0x122, 0x125, 0x128, 0x12A,
    0x12C, 0x12E, 0x130, 0x133, 0x135, 0x138, 0x13A, 0x13C, 0x13F, 0x141,
    0x142, 0x143, 0x148, 0x14B, 0x151, 0x157, 0x159, 0x15B, 0x15D, 0x15F,
    0x162, 0x164, 0x167, 0x169, 0x16B, 0x171, 0x177, 0x178, 0x179, 0x17B,
    0x180, 0x183, 0x185, 0x187, 0x18D, 0x190, 0x193, 0x197, 0x19A, 0x19B,
    0x19D, 0x1A0, 0x1A2, 0x1A3, 0x1A4, 0x1AB, 0x1AC, 0x1AF,
)


HUD_ZRD_WIDGET_CONSTRUCTOR_COD_MNEMONICS = (
    'push', 'push', 'mov', 'push', 'mov', 'sub', 'push', 'push', 'push', 'push',
    'xor', 'mov', 'push', 'mov', 'call', 'mov', 'lea', 'mov', 'mov', 'mov',
    'mov', 'mov', 'mov', 'lea', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov',
    'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov',
    'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'mov',
    'mov', 'mov', 'mov', 'mov', 'mov', 'mov', 'cmp', 'mov', 'mov', 'je',
    'mov', 'add', 'mov', 'add', 'cmp', 'jne', 'mov', 'mov', 'push', 'push',
    'call', 'mov', 'mov', 'mov', 'cmp', 'mov', 'je', 'mov', 'add', 'mov',
    'add', 'cmp', 'jne', 'mov', 'mov', 'push', 'push', 'call', 'mov', 'mov',
    'mov', 'cmp', 'mov', 'je', 'mov', 'add', 'mov', 'add', 'cmp', 'jne',
    'mov', 'lea', 'push', 'push', 'mov', 'call', 'mov', 'mov', 'mov', 'mov',
    'call', 'mov', 'mov', 'and', 'pop', 'or', 'mov', 'mov', 'pop', 'pop',
    'mov', 'pop', 'add', 'ret',
)


HUD_TRIPLET_PANEL_CONSTRUCTOR_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40d7e0"
)


HUD_TRIPLET_PANEL_CONSTRUCTOR_CALLER_START = "0x40d7e0"


HUD_TRIPLET_PANEL_CONSTRUCTOR_CALLER_END_EXCLUSIVE = "0x40d9d0"


HUD_TRIPLET_PANEL_CONSTRUCTOR_CALLER_SYMBOL = "??0HudUiMgrData@@QAE@XZ"


HUD_TRIPLET_PANEL_CONSTRUCTOR_SYMBOL = "??0HudUiTripletPanel@@QAE@XZ"


HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_IDENTITY = (
    "symbol:recoil:function:0x40f200"
)


HUD_TRIPLET_PANEL_CONSTRUCTOR_TARGET_ADDRESS = "0x40f200"


HUD_TRIPLET_PANEL_CONSTRUCTOR_RETAIL_ORDINAL = 3


HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_IDENTITY = (
    "symbol:recoil:function:0x40dbf0"
)


HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_START = "0x40dbf0"


HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_END_EXCLUSIVE = "0x40dcd0"


HUD_COUNTER_TEXT_PANEL_CONSTRUCTOR_CALLER_SYMBOL = (
    "??0HudUiCounterTextPanel@@QAE@XZ"
)


HUD_COUNTER_TEXT_PANEL_VFTABLE_SYMBOL = (
    "??_7HudUiCounterTextPanel@@6B@"
)


HUD_COUNTER_TEXT_PANEL_STORAGE_IDENTITY = (
    "storage:recoil:data:0x4ce578"
)


HUD_COUNTER_TEXT_PANEL_STORAGE_ADDRESS = "0x4ce578"


HUD_COUNTER_TEXT_PANEL_STORAGE_END_EXCLUSIVE = "0x4ce60c"


HUD_COUNTER_TEXT_PANEL_VFTABLE_SIZE = 0x94


HUD_COUNTER_TEXT_PANEL_VPTR_STORE_OFFSET = 0x4B


HUD_COUNTER_TEXT_PANEL_VPTR_RELOCATION_OFFSET = 0x4D


HUD_COUNTER_TEXT_PANEL_VPTR_LOAD_OFFSET = 0x7D


HUD_COUNTER_TEXT_PANEL_CALL_OFFSET = 0x8F


HUD_COUNTER_TEXT_PANEL_SLOT_DISPLACEMENT = 0x74


HUD_COUNTER_TEXT_PANEL_RETAIL_SLOT_ADDRESS = "0x4ce5ec"


HUD_COUNTER_TEXT_PANEL_SLOT_SYMBOL = "?SetTextFmt@HudUiPanel@@UAAXPBDZZ"


HUD_COUNTER_TEXT_PANEL_RETAIL_ORDINAL = 2


HUD_COUNTER_TEXT_PANEL_CLEANUP_BYTES = 12


IMAGE_SCN_LNK_COMDAT = 0x00001000


FINITE_SYMBOL_REGEX_MAX_ALTERNATIVES = 32


FINITE_SYMBOL_REGEX_MAX_ENCODED_LENGTH = 4096


FINITE_SYMBOL_REGEX_MAX_DECODED_NAME_LENGTH = 1024


ZEROARG_ABI_ROW_FIELDS = frozenset(
    {
        "id",
        "identity",
        "callee_source",
        "cdecl_symbol",
        "fastcall_symbol",
        "callers",
        "return_category",
        "retail_evidence",
        "eh_policy",
    }
)


ZEROARG_ABI_RETAIL_EVIDENCE_FIELDS = frozenset(
    {
        "free_or_static",
        "explicit_argument_count",
        "hidden_this",
        "hidden_sret",
        "lifecycle",
        "variadic",
        "address_taken",
        "callback",
        "vtable",
        "export",
        "import",
        "function_pointer",
        "direct_calls",
    }
)


ZEROARG_ABI_DIRECT_CALL_FIELDS = frozenset(
    {
        "address",
        "dispatch",
        "explicit_argument_count",
        "callee_return",
    }
)


FINITE_SYMBOL_REGEX_LITERAL_METACHARACTERS = frozenset(
    ".^$*+?{}[]()|\\"
)


BRIEFING_OBJECTIVE_PICTURE_VFTABLE_BRIDGE = (
    ReviewedVftableStorageBridgeSpec(
        label="briefing objective-picture vftable bridge",
        vftable_symbol=HUD_BRIEFING_OBJECTIVE_PICTURE_VFTABLE,
        caller_start="0x403930",
        caller_end_exclusive="0x403c10",
        caller_symbol=HUD_BRIEFING_RUNTIME_CONSTRUCTOR_SYMBOL,
        storage_identity=HUD_BRIEFING_OBJECTIVE_PICTURE_STORAGE_IDENTITY,
        storage_address="0x4cc898",
        storage_id="recoil:storage:va:0x4cc898",
        owner_id="recoil:owner:hud_ui.briefing_runtime_class_tables",
        owner_kind="data-owner",
        source_path="src/Battlesport/Briefing.cpp",
        table_size=0x78,
        slot_displacement=HUD_BRIEFING_OBJECTIVE_PICTURE_VFTABLE_SLOT,
        slot_symbol=HUD_ELEMENT_INVALIDATE_SYMBOL,
        slot_target_address="0x4b4180",
        vptr_store_prefix=b"\xc7\x07",
        invocation_count=1,
        provisional_identity=(
            "candidate-storage:HudUiBriefingObjectivePicture-vftable"
        ),
    )
)


BRIEFING_RUNTIME_VFTABLE_BRIDGE = ReviewedVftableStorageBridgeSpec(
    label="briefing runtime vftable bridge",
    vftable_symbol=HUD_BRIEFING_RUNTIME_VFTABLE,
    caller_start="0x403ed0",
    caller_end_exclusive="0x404070",
    caller_symbol=HUD_BRIEFING_RUNTIME_DESTRUCTOR_SYMBOL,
    storage_identity=HUD_BRIEFING_RUNTIME_STORAGE_IDENTITY,
    storage_address="0x4cc888",
    storage_id="recoil:storage:va:0x4cc888",
    owner_id="recoil:owner:hud_ui.briefing_runtime_class_tables",
    owner_kind="data-owner",
    source_path="src/Battlesport/Briefing.cpp",
    table_size=0x0C,
    slot_displacement=HUD_BRIEFING_RUNTIME_VFTABLE_SLOT,
    slot_symbol=HUD_BACKGROUND_SET_ENABLED_SYMBOL,
    slot_target_address="0x4b9850",
    vptr_store_prefix=b"\xc7\x06",
    invocation_count=1,
    provisional_identity="candidate-storage:HudUiBriefingRuntime-vftable",
)


BRIEFING_RUNTIME_EH_ARRAY_DESTRUCTOR_CALL_BRIDGE = (
    ReviewedEhArrayDestructorCallSpec(
        label="briefing runtime EH array-destructor call bridge",
        caller_identity="symbol:recoil:function:0x403ed0",
        caller_start="0x403ed0",
        caller_end_exclusive="0x404070",
        caller_symbol=HUD_BRIEFING_RUNTIME_DESTRUCTOR_SYMBOL,
        ordinal=1,
        invocation_count=1,
    )
)


ZINPUT_WAIT_SLEEP_REGISTER_IAT_BRIDGE = ReviewedRegisterIatBridgeSpec(
    label="zInput wait-key Sleep register-IAT bridge",
    caller_identity="symbol:recoil:function:0x404140",
    caller_start="0x404140",
    caller_end_exclusive="0x404180",
    caller_symbol="@zInputWaitForAnyKeyPressWithTimeoutMs@4",
    register="edi",
    import_dll="KERNEL32.dll",
    import_name="Sleep",
    owner_id="recoil:owner:provider.kernel32.sleep_import",
    retail_storage_address="0x4cc0b0",
    retail_storage_type=(
        "void (__stdcall* const)(uint32_t dwMilliseconds)"
    ),
    retail_load_address="0x40414b",
    retail_load_body=b"\x8b\x3d\xb0\xc0\x4c\x00",
    retail_call_address="0x40415e",
    retail_call_body=b"\xff\xd7",
    retail_backedge_address="0x404165",
    retail_backedge_mnemonic="jg",
    retail_backedge_body=b"\x7f\xea",
    candidate_import_symbol="__imp__Sleep@4",
    candidate_load_address="0xb",
    candidate_load_body=b"\x8b\x3d\x00\x00\x00\x00",
    candidate_call_address="0x1e",
    candidate_call_body=b"\xff\xd7",
    candidate_backedge_address="0x25",
    candidate_backedge_mnemonic="jg",
    candidate_backedge_body=b"\x7f\xea",
    candidate_relocation_offset=0x0D,
    ordinal=1,
    cleanup_bytes=None,
    invocation_count=1,
)


ZINPUT_JOYSTICK_TARGET_NAME = (
    "zinput_joystick_471e40_472670_authored_order"
)


ZINPUT_JOYSTICK_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zinput_joystick_471e40_472670_authored_order.json"
)


ZINPUT_JOYSTICK_SOURCE_PATH = (
    "src/GameZRecoil/zInput/zin_joystick.cpp"
)


ZINPUT_JOYSTICK_INIT_CALLER_START = "0x471e40"


ZINPUT_JOYSTICK_INIT_CALLER_END_EXCLUSIVE = "0x471f60"


ZINPUT_JOYSTICK_INIT_CALLER_IDENTITY = (
    "symbol:recoil:function:0x471e40"
)


ZINPUT_JOYSTICK_INIT_CALLER_SYMBOL = (
    "?DIInitJoystickDevice@zInput@@YIHPAUHWND__@@@Z"
)


ZINPUT_JOYSTICK_ACQUIRE_CALLER_START = "0x471fb0"


ZINPUT_JOYSTICK_ACQUIRE_CALLER_END_EXCLUSIVE = "0x471fd0"


ZINPUT_JOYSTICK_ACQUIRE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x471fb0"
)


ZINPUT_JOYSTICK_ACQUIRE_CALLER_SYMBOL = (
    "?DIAcquireJoystickDevice@zInput@@YAHXZ"
)


ZINPUT_JOYSTICK_ACQUIRE_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zinput.zin-joystick."
    "di-acquirejoystickdevice"
)


ZINPUT_JOYSTICK_ACQUIRE_COFF_SECTION_INDEX = 7


ZINPUT_JOYSTICK_ACQUIRE_CANDIDATE_SIZE = 0x20


ZINPUT_JOYSTICK_ACQUIRE_VPTR_OFFSET = 0x09


ZINPUT_JOYSTICK_ACQUIRE_CALL_OFFSET = 0x0C


ZINPUT_JOYSTICK_ACQUIRE_VIRTUAL_SLOT = 0x1C


ZINPUT_JOYSTICK_AGGREGATE_SYMBOL_ID = "recoil:data:0x561cb0"


ZINPUT_JOYSTICK_AGGREGATE_STORAGE_ID = "recoil:storage:va:0x561cb0"


ZINPUT_JOYSTICK_AGGREGATE_ADDRESS = "0x561cb0"


ZINPUT_JOYSTICK_AGGREGATE_NAME = "g_zInput_GlobalStateStorage"


ZINPUT_JOYSTICK_AGGREGATE_END_EXCLUSIVE = 0x565EBC


ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL = (
    "_g_zInput_GlobalStateStorage"
)


ZINPUT_KEYBOARD_AGGREGATE_FIELD_NAMES = {
    "0x561cc4": "g_zInput_KbdSystemReady",
    "0x561cc8": "g_zInput_KbdDevice",
    "0x561ccc": "g_zInput_KbdEventBuffer",
    "0x561cd0": "g_zInput_KbdModifierState",
    "0x565bc4": "g_zInput_KbdRawEventCallback",
    "0x565bc8": "g_zInput_KbdRawEventCallbackCtx",
}


ZINPUT_JOYSTICK_DEVICE_SYMBOL_ID = "recoil:data:0x565bd0"


ZINPUT_JOYSTICK_DEVICE_STORAGE_ID = "recoil:storage:va:0x565bd0"


ZINPUT_JOYSTICK_DEVICE_ADDRESS = "0x565bd0"


ZINPUT_JOYSTICK_DEVICE_NAME = "g_zInput_JoystickDevice"


ZINPUT_JOYSTICK_STORAGE_OWNER_ID = (
    "recoil:owner:engine.zinput.global_state_static_lifetime"
)


ZINPUT_JOYSTICK_DEVICE_DISPLACEMENT = 0x3F20


ZINPUT_JOYSTICK_DEVICE_VIRTUAL_SLOT = 0x2C


ZINPUT_JOYSTICK_LATER_AGGREGATE_LEAF_SPECS = {
    "0x4721a0": {
        "end": "0x4721e0",
        "symbol": "?DISetAxisDeadzone@zInput@@YIHHH@Z",
        "name": "zInput::DISetAxisDeadzone",
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zin-joystick."
            "di-setaxisdeadzone"
        ),
        "section": 12,
        "body": bytes.fromhex(
            "83 ec 14 a1 20 3f 00 00 89 54 24 10 8d 54 24 00 "
            "c7 44 24 00 14 00 00 00 c7 44 24 04 10 00 00 00 "
            "89 4c 24 08 c7 44 24 0c 01 00 00 00 8b 08 52 6a "
            "05 50 ff 51 18 83 c4 14 c3 90 90 90 90 90 90 90"
        ),
        "relocations": ((0x04, 0x06, "_g_zInput_GlobalStateStorage"),),
        "load_offsets": (0x03,),
        "load_relocation_offsets": (0x04,),
        "instruction_count": 15,
        "calls": ((0, 0x2C, "ecx", "eax", "eax", 0x32, 0x18, 20),),
        "expected_count": 1,
    },
    "0x4721e0": {
        "end": "0x472230",
        "symbol": "?DISetAxisRange@zInput@@YIHHHH@Z",
        "name": "zInput::DISetAxisRange",
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zin-joystick.di-setaxisrange"
        ),
        "section": 14,
        "body": bytes.fromhex(
            "83 ec 18 8b 44 24 1c 89 54 24 10 89 44 24 14 a1 "
            "20 3f 00 00 8d 54 24 00 c7 44 24 00 18 00 00 00 "
            "c7 44 24 04 10 00 00 00 89 4c 24 08 c7 44 24 "
            "0c 01 00 00 00 8b 08 52 6a 04 50 ff 51 18 83 c4 "
            "18 c2 04 00 90 90 90 90 90 90 90 90 90 90 90 "
            "90 90"
        ),
        "relocations": ((0x10, 0x06, "_g_zInput_GlobalStateStorage"),),
        "load_offsets": (0x0F,),
        "load_relocation_offsets": (0x10,),
        "instruction_count": 17,
        "calls": ((0, 0x34, "ecx", "eax", "eax", 0x3A, 0x18, 24),),
        "expected_count": 1,
    },
    "0x472230": {
        "end": "0x472280",
        "symbol": "?DIGetAxisRange@zInput@@YIHHPAH0@Z",
        "name": "zInput::DIGetAxisRange",
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zin-joystick.di-getaxisrange"
        ),
        "section": 16,
        "body": bytes.fromhex(
            "83 ec 18 a1 20 3f 00 00 56 8b f2 c7 44 24 04 18 "
            "00 00 00 8d 54 24 04 c7 44 24 08 10 00 00 00 89 "
            "4c 24 0c c7 44 24 10 01 00 00 00 8b 08 52 6a "
            "04 50 ff 51 14 8b 4c 24 14 8b 54 24 20 89 0e 8b "
            "4c 24 18 89 0a 5e 83 c4 18 c2 04 00 90 90 90 "
            "90 90"
        ),
        "relocations": ((0x04, 0x06, "_g_zInput_GlobalStateStorage"),),
        "load_offsets": (0x03,),
        "load_relocation_offsets": (0x04,),
        "instruction_count": 22,
        "calls": ((0, 0x2B, "ecx", "eax", "eax", 0x31, 0x14, None),),
        "expected_count": 1,
    },
    "0x472280": {
        "end": "0x4722b0",
        "symbol": "?JoystickShutdownDevice@zInput@@YAHXZ",
        "name": "zInput::JoystickShutdownDevice",
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zin-joystick."
            "joystick-shutdowndevice"
        ),
        "section": 18,
        "body": bytes.fromhex(
            "a1 20 3f 00 00 85 c0 74 1b 8b 08 50 ff 51 20 a1 "
            "20 3f 00 00 50 8b 10 ff 52 08 c7 05 20 3f 00 00 "
            "00 00 00 00 b8 01 00 00 00 c3 90 90 90 90 90 90"
        ),
        "relocations": (
            (0x01, 0x06, "_g_zInput_GlobalStateStorage"),
            (0x10, 0x06, "_g_zInput_GlobalStateStorage"),
            (0x1C, 0x06, "_g_zInput_GlobalStateStorage"),
        ),
        "load_offsets": (0x00, 0x0F),
        "load_relocation_offsets": (0x01, 0x10),
        "instruction_count": 13,
        "calls": (
            (0, 0x09, "ecx", "eax", "eax", 0x0C, 0x20, None),
            (1, 0x15, "edx", "eax", "eax", 0x17, 0x08, None),
        ),
        "expected_count": 2,
    },
    "0x4722c0": {
        "end": "0x472390",
        "symbol": "?DIPollJoystickState@zInput@@YIPAUDIJOYSTATE2@@E@Z",
        "name": "zInput::DIPollJoystickState",
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zin-joystick."
            "di-polljoystickstate"
        ),
        "section": 22,
        "body": bytes.fromhex(
            "a1 1c 3f 00 00 53 56 57 33 ff 8a d9 3b c7 75 06 "
            "33 c0 5f 5e 5b c3 8b 35 20 3f 00 00 56 8b 06 ff "
            "50 64 8b 0e 68 00 00 00 00 68 10 01 00 00 56 ff "
            "51 24 8b 0d 64 41 00 00 83 f9 03 7d 18 89 3d 08 "
            "00 00 00 89 3d b8 00 00 00 89 3d d8 00 00 00 89 "
            "3d f8 00 00 00 83 f9 04 7d 18 89 3d 14 00 00 00 "
            "89 3d c4 00 00 00 89 3d e4 00 00 00 89 3d 04 01 "
            "00 00 3d 1e 00 07 80 75 0b e8 00 00 00 00 33 c0 "
            "5f 5e 5b c3 3b c7 74 06 33 c0 5f 5e 5b c3 b9 44 "
            "00 00 00 be 24 3f 00 00 bf 34 40 00 00 f3 a5 b9 "
            "44 00 00 00 be 00 00 00 00 bf 24 3f 00 00 84 db "
            "f3 a5 74 0b 8b 0d f0 41 00 00 e8 00 00 00 00 e8 "
            "00 00 00 00 5f 5e 5b c3 90 90 90 90 90 90 90 90"
        ),
        "relocations": (
            (0x01, 0x06, "_g_zInput_GlobalStateStorage"),
            (0x18, 0x06, "_g_zInput_GlobalStateStorage"),
            (0x25, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x34, 0x06, "_g_zInput_GlobalStateStorage"),
            (0x3F, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x45, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x4B, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x51, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x5C, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x62, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x68, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x6E, 0x06, "_g_zInput_JoystickRawDIState"),
            (0x7A, 0x14, "?DIAcquireJoystickDevice@zInput@@YAHXZ"),
            (0x94, 0x06, "_g_zInput_GlobalStateStorage"),
            (0x99, 0x06, "_g_zInput_GlobalStateStorage"),
            (0xA5, 0x06, "_g_zInput_JoystickRawDIState"),
            (0xAA, 0x06, "_g_zInput_GlobalStateStorage"),
            (0xB6, 0x06, "_g_zInput_GlobalStateStorage"),
            (
                0xBB,
                0x14,
                "?DispatchJoystickButtonCallbacks@"
                "zInput_BindMapContext@@QAEXXZ",
            ),
            (0xC0, 0x14, "?DIGetCurrentState@zInput@@YAPAUDIJOYSTATE2@@XZ"),
        ),
        "load_offsets": (0x16,),
        "load_relocation_offsets": (0x18,),
        "instruction_count": 67,
        "calls": (
            (0, 0x1D, "eax", "esi", "esi", 0x1F, 0x64, None),
            (1, 0x22, "ecx", "esi", "esi", 0x2F, 0x24, None),
        ),
        "expected_count": 5,
    },
    "0x472450": {
        "end": "0x472480",
        "symbol": "@zInputDICreateForceFeedbackEffect@8",
        "name": "zInputDI::CreateForceFeedbackEffect",
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zin-joystick."
            "zinput-di-createforcefeedbackeffect"
        ),
        "section": 32,
        "body": bytes.fromhex(
            "51 a1 20 3f 00 00 85 c0 75 02 59 c3 57 56 8d 7c "
            "24 08 6a 00 57 c7 44 24 10 00 00 00 00 8b 30 52 "
            "51 50 ff 56 48 33 c9 85 c0 8b 44 24 08 5e 0f 9c "
            "c1 49 5f 23 c1 59 c3 90 90 90 90 90 90 90 90 90"
        ),
        "relocations": ((0x02, 0x06, "_g_zInput_GlobalStateStorage"),),
        "load_offsets": (0x01,),
        "load_relocation_offsets": (0x02,),
        "instruction_count": 27,
        "calls": ((0, 0x1D, "esi", "eax", "eax", 0x22, 0x48, None),),
        "expected_count": 1,
    },
}


ZINPUT_KEYBOARD_TARGET_NAME = "zinput_keyboard_46f300_470020_authored_order"


ZINPUT_KEYBOARD_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zinput_keyboard_46f300_470020_authored_order.json"
)


ZINPUT_KEYBOARD_SOURCE_PATH = "src/GameZRecoil/zInput/zin_kbd.cpp"


ZINPUT_KEYBOARD_INIT_CALLER_START = "0x46f300"


ZINPUT_KEYBOARD_INIT_CALLER_END_EXCLUSIVE = "0x46f420"


ZINPUT_KEYBOARD_INIT_CALLER_IDENTITY = "symbol:recoil:function:0x46f300"


ZINPUT_KEYBOARD_INIT_CALLER_SYMBOL = "?KeyboardInitDevice@zInput@@YAHXZ"


ZINPUT_KEYBOARD_INIT_CALLER_NAME = "zInput::KeyboardInitDevice"


ZINPUT_KEYBOARD_INIT_CALLER_ANCHOR_ID = (
    "recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_initdevice"
)


ZINPUT_KEYBOARD_INIT_COFF_SECTION_INDEX = 3


ZINPUT_KEYBOARD_INIT_CANDIDATE_SIZE = 0x120


ZINPUT_KEYBOARD_DEVICE_SYMBOL_ID = "recoil:data:0x561cc8"


ZINPUT_KEYBOARD_DEVICE_STORAGE_ID = "recoil:storage:va:0x561cc8"


ZINPUT_KEYBOARD_DEVICE_ADDRESS = "0x561cc8"


ZINPUT_KEYBOARD_DEVICE_NAME = "g_zInput_KbdDevice"


ZINPUT_KEYBOARD_DEVICE_DISPLACEMENT = 0x18


ZINPUT_KEYBOARD_INIT_AGGREGATE_LEAF_RELOCATIONS = (
    0x34,
    0x53,
    0x6C,
    0x8B,
    0xA8,
    0xC3,
)


ZINPUT_KEYBOARD_INIT_INVOCATION_OFFSETS = (
    0x5D,
    0x7C,
    0x99,
    0xB4,
    0xCA,
    0xDD,
    0xF3,
    0x10B,
    0x110,
)


ZINPUT_KEYBOARD_INIT_LEAF_CHAINS = (
    (1, 0x6B, 0x79, 0x7B, 0x7C, "edx", 0x34),
    (2, 0x8A, 0x96, 0x98, 0x99, "edx", 0x18),
    (3, 0xA7, 0xB2, 0xB1, 0xB4, "edx", 0x2C),
    (4, 0xC2, 0xC8, 0xC7, 0xCA, "ecx", 0x1C),
)


ZINPUT_KEYBOARD_SHUTDOWN_CALLER_START = "0x46f420"


ZINPUT_KEYBOARD_SHUTDOWN_CALLER_END_EXCLUSIVE = "0x46f450"


ZINPUT_KEYBOARD_SHUTDOWN_CALLER_IDENTITY = (
    "symbol:recoil:function:0x46f420"
)


ZINPUT_KEYBOARD_SHUTDOWN_CALLER_SYMBOL = (
    "?KeyboardShutdownDevice@zInput@@YAHXZ"
)


ZINPUT_KEYBOARD_SHUTDOWN_CALLER_NAME = "zInput::KeyboardShutdownDevice"


ZINPUT_KEYBOARD_SHUTDOWN_CALLER_ANCHOR_ID = (
    "recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-"
    "keyboard_shutdowndevice"
)


ZINPUT_KEYBOARD_SHUTDOWN_COFF_SECTION_INDEX = 5


ZINPUT_KEYBOARD_SHUTDOWN_CANDIDATE_SIZE = 0x30


ZINPUT_KEYBOARD_SHUTDOWN_BODY = bytes.fromhex(
    "a1 18 00 00 00 85 c0 74 11 8b 08 50 ff 51 20 "
    "a1 18 00 00 00 50 8b 10 ff 52 08 a1 1c 00 00 "
    "00 85 c0 74 0a 50 ff 15 00 00 00 00 83 c4 04 "
    "33 c0 c3"
)


ZINPUT_KEYBOARD_SHUTDOWN_RELOCATIONS = (
    (0x01, IMAGE_REL_I386_DIR32, "_g_zInput_GlobalStateStorage", 0x18),
    (0x10, IMAGE_REL_I386_DIR32, "_g_zInput_GlobalStateStorage", 0x18),
    (0x1B, IMAGE_REL_I386_DIR32, "_g_zInput_GlobalStateStorage", 0x1C),
    (0x26, IMAGE_REL_I386_DIR32, "__imp__free", 0),
)


ZINPUT_KEYBOARD_SHUTDOWN_INVOCATION_OFFSETS = (0x0C, 0x17, 0x24)


ZINPUT_KEYBOARD_SHUTDOWN_LEAF_CHAINS = (
    (0, 0x00, 0x09, 0x0B, 0x0C, "ecx", 0x20),
    (1, 0x0F, 0x15, 0x14, 0x17, "edx", 0x08),
)


ZINPUT_KEYBOARD_SHUTDOWN_BRANCH_SUCCESSORS = (
    (0x07, (0x09, 0x1A)),
    (0x21, (0x23, 0x2D)),
)


# Finite r4387 retail/runtime-dispatch facts.  These callers deliberately keep
# runtime-selected callback targets blank; only their exact storage route is
# reviewed here.  Candidate section size is separate from retail extent for
# PollState because VC5 emits a larger COMDAT while retail's next function is
# still exactly 0x46f970.
ZINPUT_RUNTIME_DISPATCH_RETAIL_SPECS = {
    "0x46f450": {
        "end": "0x46f690",
        "calls": ("0x46f483", "0x46f49d", "0x46f5cd"),
        "vptr": (("0x46f483", "ecx", 0x28), ("0x46f49d", "ecx", 0x1C)),
        "rows": (
            ("0x46f451", bytes.fromhex("a1 c4 1c 56 00")),
            ("0x46f462", bytes.fromhex("a1 c8 1c 56 00")),
            ("0x46f46e", bytes.fromhex("8b 15 cc 1c 56 00")),
            ("0x46f47d", bytes.fromhex("8b 08")),
            ("0x46f483", bytes.fromhex("ff 51 28")),
            ("0x46f495", bytes.fromhex("a1 c8 1c 56 00")),
            ("0x46f49b", bytes.fromhex("8b 08")),
            ("0x46f49d", bytes.fromhex("ff 51 1c")),
            ("0x46f4a4", bytes.fromhex("a1 cc 1c 56 00")),
            ("0x46f4d7", bytes.fromhex("8b 0d d0 1c 56 00")),
            ("0x46f4f8", bytes.fromhex("8b 0d d0 1c 56 00")),
            ("0x46f519", bytes.fromhex("8b 0d d0 1c 56 00")),
            ("0x46f537", bytes.fromhex("8b 0d d0 1c 56 00")),
            ("0x46f546", bytes.fromhex("8b 0d d0 1c 56 00")),
            ("0x46f554", bytes.fromhex("8b 0d d0 1c 56 00")),
        ),
        "non_callback_loads": (
            ("0x46f451", bytes.fromhex("a1 c4 1c 56 00"), "0x561cc4"),
            ("0x46f462", bytes.fromhex("a1 c8 1c 56 00"), "0x561cc8"),
            ("0x46f46e", bytes.fromhex("8b 15 cc 1c 56 00"), "0x561ccc"),
            ("0x46f495", bytes.fromhex("a1 c8 1c 56 00"), "0x561cc8"),
            ("0x46f4a4", bytes.fromhex("a1 cc 1c 56 00"), "0x561ccc"),
            ("0x46f4d7", bytes.fromhex("8b 0d d0 1c 56 00"), "0x561cd0"),
            ("0x46f4f8", bytes.fromhex("8b 0d d0 1c 56 00"), "0x561cd0"),
            ("0x46f519", bytes.fromhex("8b 0d d0 1c 56 00"), "0x561cd0"),
            ("0x46f537", bytes.fromhex("8b 0d d0 1c 56 00"), "0x561cd0"),
            ("0x46f546", bytes.fromhex("8b 0d d0 1c 56 00"), "0x561cd0"),
            ("0x46f554", bytes.fromhex("8b 0d d0 1c 56 00"), "0x561cd0"),
        ),
    },
    "0x46f690": {
        "end": "0x46f970",
        "calls": (
            "0x46f6c0", "0x46f6da", "0x46f7e8", "0x46f7f5",
            "0x46f870", "0x46f89d",
        ),
        "vptr": (("0x46f6c0", "ecx", 0x28), ("0x46f6da", "ecx", 0x1C)),
        "register_callbacks": (("0x46f870", "eax", "key-table", "call"),),
        "raw_callback_call": "0x46f7f5",
        "rows": (
            ("0x46f693", bytes.fromhex("a1 c8 1c 56 00")),
            ("0x46f6a3", bytes.fromhex("8b 15 cc 1c 56 00")),
            ("0x46f6bb", bytes.fromhex("8b 08")),
            ("0x46f6c0", bytes.fromhex("ff 51 28")),
            ("0x46f6d2", bytes.fromhex("a1 c8 1c 56 00")),
            ("0x46f6d8", bytes.fromhex("8b 08")),
            ("0x46f6da", bytes.fromhex("ff 51 1c")),
            ("0x46f6e1", bytes.fromhex("8b 35 cc 1c 56 00")),
            ("0x46f70f", bytes.fromhex("a1 d0 1c 56 00")),
            ("0x46f733", bytes.fromhex("a1 d0 1c 56 00")),
            ("0x46f751", bytes.fromhex("a1 d0 1c 56 00")),
            ("0x46f76c", bytes.fromhex("a1 d0 1c 56 00")),
            ("0x46f78b", bytes.fromhex("a1 d0 1c 56 00")),
            ("0x46f79d", bytes.fromhex("a1 d0 1c 56 00")),
            ("0x46f7af", bytes.fromhex("a1 d0 1c 56 00")),
            ("0x46f7df", bytes.fromhex("a1 c4 5b 56 00")),
            ("0x46f7ed", bytes.fromhex("8b 15 c8 5b 56 00")),
            ("0x46f7f3", bytes.fromhex("8b c8")),
            ("0x46f7f5", bytes.fromhex("ff 15 c4 5b 56 00")),
            ("0x46f85a", bytes.fromhex("8b 04 f5 d8 1c 56 00")),
            ("0x46f86e", bytes.fromhex("8b ce")),
            ("0x46f870", bytes.fromhex("ff d0")),
            ("0x46f81d", bytes.fromhex("8b 3d cc 1c 56 00")),
        ),
        "non_callback_loads": (
            ("0x46f693", bytes.fromhex("a1 c8 1c 56 00"), "0x561cc8"),
            ("0x46f6a3", bytes.fromhex("8b 15 cc 1c 56 00"), "0x561ccc"),
            ("0x46f6d2", bytes.fromhex("a1 c8 1c 56 00"), "0x561cc8"),
            ("0x46f6e1", bytes.fromhex("8b 35 cc 1c 56 00"), "0x561ccc"),
            ("0x46f70f", bytes.fromhex("a1 d0 1c 56 00"), "0x561cd0"),
            ("0x46f733", bytes.fromhex("a1 d0 1c 56 00"), "0x561cd0"),
            ("0x46f751", bytes.fromhex("a1 d0 1c 56 00"), "0x561cd0"),
            ("0x46f76c", bytes.fromhex("a1 d0 1c 56 00"), "0x561cd0"),
            ("0x46f78b", bytes.fromhex("a1 d0 1c 56 00"), "0x561cd0"),
            ("0x46f79d", bytes.fromhex("a1 d0 1c 56 00"), "0x561cd0"),
            ("0x46f7af", bytes.fromhex("a1 d0 1c 56 00"), "0x561cd0"),
            ("0x46f7df", bytes.fromhex("a1 c4 5b 56 00"), "0x565bc4"),
            ("0x46f7ed", bytes.fromhex("8b 15 c8 5b 56 00"), "0x565bc8"),
            ("0x46f81d", bytes.fromhex("8b 3d cc 1c 56 00"), "0x561ccc"),
        ),
    },
    "0x470d40": {
        "end": "0x470db0",
        "calls": (
            "0x470d44", "0x470d55", "0x470d66", "0x470d72",
            "0x470d83", "0x470d8f", "0x470da0",
        ),
        "register_callbacks": (
            ("0x470d66", "edx", "bindmap-this", "call"),
            ("0x470d83", "edx", "bindmap-this", "call"),
            ("0x470da0", "edx", "bindmap-this", "call"),
        ),
        "rows": (
            ("0x470d5a", bytes.fromhex("8b 4e 0c")),
            ("0x470d5d", bytes.fromhex("8b 14 81")),
            ("0x470d64", bytes.fromhex("8b c8")),
            ("0x470d66", bytes.fromhex("ff d2")),
            ("0x470d77", bytes.fromhex("8b 56 0c")),
            ("0x470d7a", bytes.fromhex("8b 14 82")),
            ("0x470d81", bytes.fromhex("8b c8")),
            ("0x470d83", bytes.fromhex("ff d2")),
            ("0x470d94", bytes.fromhex("8b 4e 0c")),
            ("0x470d97", bytes.fromhex("8b 14 81")),
            ("0x470d9e", bytes.fromhex("8b c8")),
            ("0x470da0", bytes.fromhex("ff d2")),
        ),
    },
    "0x470db0": {
        "end": "0x470df0",
        "calls": ("0x470dbb", "0x470dc7", "0x470dd8"),
        "register_callbacks": (("0x470dd8", "edx", "bindmap-this", "call"),),
        "rows": (
            ("0x470dcc", bytes.fromhex("8b 4f 0c")),
            ("0x470dcf", bytes.fromhex("8b 14 81")),
            ("0x470dd6", bytes.fromhex("8b c8")),
            ("0x470dd8", bytes.fromhex("ff d2")),
        ),
    },
    "0x470e80": {
        "end": "0x470eb0",
        "calls": ("0x470e87", "0x470e9e"),
        "register_callbacks": (("0x470e9e", "edx", "bindmap-current", "tail"),),
        "rows": (
            ("0x470e81", bytes.fromhex("8b 0d a0 5e 56 00")),
            ("0x470e8c", bytes.fromhex("8b 0d a0 5e 56 00")),
            ("0x470e92", bytes.fromhex("8b 51 0c")),
            ("0x470e95", bytes.fromhex("8b 14 82")),
            ("0x470e9c", bytes.fromhex("8b c8")),
            ("0x470e9e", bytes.fromhex("ff e2")),
        ),
    },
}


ZINPUT_POLL_REGISTER_CALLBACKS = (
    (0x18E, "esi", "raw", "call"),
    (0x20E, "eax", "key-table", "call"),
)


ZINPUT_BINDMAP_TARGET_NAME = "zinput_bindmap_4706c0_4719e0_authored_order"


ZINPUT_BINDMAP_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zinput_bindmap_4706c0_4719e0_authored_order.json"
)


ZINPUT_BINDMAP_SOURCE_PATH = "src/GameZRecoil/zInput/zInput.cpp"


ZINPUT_BINDMAP_PHYSICAL_BLOCK_ID = "recoil:block:0x4706c0"


ZINPUT_BINDMAP_CANDIDATE_DISPATCH_SPECS = {
    "0x470d40": {
        "end": "0x470db0",
        "symbol": (
            "?DispatchMouseButtonCallbacks@zInput_BindMapContext@@QAEXXZ"
        ),
        "name": (
            "zInput_BindMapContext::DispatchMouseButtonCallbacks"
        ),
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zinput."
            "zinput-bindmapcontext-dispatchmousebuttoncallbacks"
        ),
        "size": 0x70,
        "section": 46,
        "instruction_offsets": (
            0x00, 0x01, 0x02, 0x04, 0x09, 0x0B, 0x0F, 0x11,
            0x13, 0x15, 0x1A, 0x1D, 0x20, 0x22, 0x24, 0x26,
            0x28, 0x2C, 0x2E, 0x30, 0x32, 0x37, 0x3A, 0x3D,
            0x3F, 0x41, 0x43, 0x45, 0x49, 0x4B, 0x4D, 0x4F,
            0x54, 0x57, 0x5A, 0x5C, 0x5E, 0x60, 0x62, 0x63,
            0x64,
        ),
        "invocations": (0x04, 0x15, 0x26, 0x32, 0x43, 0x4F, 0x60),
        "callbacks": (
            (2, 0x1A, 0x1D, 0x20, 0x22, 0x24, 0x26, "edx", "call"),
            (4, 0x37, 0x3A, 0x3D, 0x3F, 0x41, 0x43, "edx", "call"),
            (6, 0x54, 0x57, 0x5A, 0x5C, 0x5E, 0x60, "edx", "call"),
        ),
        "direct_relocations": (
            (
                0,
                0x05,
                "?MouseGetStateSnapshotPtr@zInput@@"
                "YAPAUMouseStateSnapshot@1@XZ",
            ),
            (
                1,
                0x16,
                "?GetCommandByMouseSlot@zInput_BindMapContext@@QAEHH@Z",
            ),
            (
                3,
                0x33,
                "?GetCommandByMouseSlot@zInput_BindMapContext@@QAEHH@Z",
            ),
            (
                5,
                0x50,
                "?GetCommandByMouseSlot@zInput_BindMapContext@@QAEHH@Z",
            ),
        ),
        "body": bytes.fromhex(
            "56 57 8b f1 e8 00 00 00 00 8b f8 83 7f 20 01 75 17 "
            "6a 01 8b ce e8 00 00 00 00 8b 4e 0c 8b 14 81 85 d2 "
            "74 04 8b c8 ff d2 83 7f 24 01 75 17 6a 02 8b ce e8 "
            "00 00 00 00 8b 56 0c 8b 14 82 85 d2 74 04 8b c8 "
            "ff d2 83 7f 28 01 75 17 6a 03 8b ce e8 00 00 00 "
            "00 8b 4e 0c 8b 14 81 85 d2 74 04 8b c8 ff d2 5f "
            "5e c3 90 90 90 90 90 90 90 90 90 90 90"
        ),
    },
    "0x470db0": {
        "end": "0x470df0",
        "symbol": (
            "?DispatchJoystickButtonCallbacks@zInput_BindMapContext@@QAEXXZ"
        ),
        "name": (
            "zInput_BindMapContext::DispatchJoystickButtonCallbacks"
        ),
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zinput."
            "zinput-bindmapcontext-dispatchjoystickbuttoncallbacks"
        ),
        "size": 0x40,
        "section": 48,
        "instruction_offsets": (
            0x00, 0x01, 0x02, 0x04, 0x09, 0x0B, 0x10, 0x13,
            0x15, 0x17, 0x1C, 0x1F, 0x22, 0x24, 0x26, 0x28,
            0x2A, 0x2B, 0x2E, 0x30, 0x31, 0x32,
        ),
        "invocations": (0x0B, 0x17, 0x28),
        "callbacks": (
            (2, 0x1C, 0x1F, 0x22, 0x24, 0x26, 0x28, "edx", "call"),
        ),
        "direct_relocations": (
            (
                0,
                0x0C,
                "?DIGetButtonTransitionState@zInput@@YIHH@Z",
            ),
            (
                1,
                0x18,
                "?BindMapCurrentGetCommandByJoystickSlot@zInput@@YIHH@Z",
            ),
        ),
        "body": bytes.fromhex(
            "56 57 8b f9 be 01 00 00 00 8b ce e8 00 00 00 00 "
            "83 f8 01 75 15 8b ce e8 00 00 00 00 8b 4f 0c 8b "
            "14 81 85 d2 74 04 8b c8 ff d2 46 83 fe 0b 72 d9 "
            "5f 5e c3 90 90 90 90 90 90 90 90 90 90 90 90 90"
        ),
    },
    "0x470e80": {
        "end": "0x470eb0",
        "symbol": "@zInputBindMapContextDispatchFromKeyboardEvent@4",
        "name": "zInput_BindMapContext::DispatchFromKeyboardEvent",
        "anchor": (
            "recoil:anchor:gamezrecoil.zinput.zinput."
            "zinput-bindmapcontext-dispatchfromkeyboardevent"
        ),
        "size": 0x30,
        "section": 52,
        "instruction_offsets": (
            0x00, 0x01, 0x07, 0x0C, 0x12, 0x15, 0x18, 0x1A,
            0x1C, 0x1E, 0x20,
        ),
        "invocations": (0x07, 0x1E),
        "callbacks": (
            (1, 0x12, 0x15, 0x18, 0x1A, 0x1C, 0x1E, "edx", "tail"),
        ),
        "direct_relocations": (
            (
                0,
                0x08,
                "?GetCommandByAnyKeyboardKey@zInput_BindMapContext@@QAEHH@Z",
            ),
        ),
        "current_relocations": (
            (0x03, "_g_zInput_GlobalStateStorage", 0x41F0),
            (0x0E, "_g_zInput_GlobalStateStorage", 0x41F0),
        ),
        "body": bytes.fromhex(
            "51 8b 0d f0 41 00 00 e8 00 00 00 00 8b 0d f0 41 "
            "00 00 8b 51 0c 8b 14 82 85 d2 74 04 8b c8 ff e2 "
            "c3 90 90 90 90 90 90 90 90 90 90 90 90 90 90 90"
        ),
    },
}


ZINPUT_POLL_CALLBACK_PROVENANCE_ROWS = (
    (
        "raw-callback", 0x174, bytes.fromhex("a1 14 3f 00 00"),
        r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        r"_g_zInput_GlobalStateStorage\+16148",
    ),
    (
        "raw-callback", 0x179, bytes.fromhex("85 c0"),
        r"test\s+eax\s*,\s*eax",
    ),
    (
        "raw-callback", 0x17B, bytes.fromhex("74 23"),
        r"je\s+(?:short\s+)?\$L[0-9]+",
    ),
    (
        "raw-context", 0x17D, bytes.fromhex("8b 1d 18 3f 00 00"),
        r"mov\s+ebx\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        r"_g_zInput_GlobalStateStorage\+16152",
    ),
    (
        "raw-callback", 0x183, bytes.fromhex("8b f0"),
        r"mov\s+esi\s*,\s*eax",
    ),
    (
        "direct-translate", 0x185, bytes.fromhex("e8 00 00 00 00"),
        r"call\s+\?KeyboardTranslateDikToAscii@zInput@@YIHH@Z"
        r"(?:\s*;.*)?",
    ),
    (
        "raw-callback", 0x18A, bytes.fromhex("8b c8"),
        r"mov\s+ecx\s*,\s*eax",
    ),
    (
        "raw-context", 0x18C, bytes.fromhex("8b d3"),
        r"mov\s+edx\s*,\s*ebx",
    ),
    (
        "raw-callback", 0x18E, bytes.fromhex("ff d6"),
        r"call\s+esi",
    ),
    (
        "indexed-key-callback", 0x1F7,
        bytes.fromhex("8b 04 f5 28 00 00 00"),
        r"mov\s+eax\s*,\s*(?:dword\s+(?:ptr\s+)?)?"
        r"_g_zInput_GlobalStateStorage\[esi\*8\+40\]",
    ),
    (
        "indexed-key-callback", 0x1FE, bytes.fromhex("85 c0"),
        r"test\s+eax\s*,\s*eax",
    ),
    (
        "indexed-key-callback", 0x200, bytes.fromhex("74 23"),
        r"je\s+(?:short\s+)?\$L[0-9]+",
    ),
    (
        "indexed-key-state", 0x202,
        bytes.fromhex("f6 04 f5 24 00 00 00 01"),
        r"test\s+(?:byte\s+(?:ptr\s+)?)?"
        r"_g_zInput_GlobalStateStorage\[esi\*8\+36\]\s*,\s*1",
    ),
    (
        "indexed-key-callback", 0x20A, bytes.fromhex("74 19"),
        r"je\s+(?:short\s+)?\$L[0-9]+",
    ),
    (
        "indexed-key-callback", 0x20C, bytes.fromhex("8b ce"),
        r"mov\s+ecx\s*,\s*esi",
    ),
    (
        "indexed-key-callback", 0x20E, bytes.fromhex("ff d0"),
        r"call\s+eax",
    ),
)


ZINPUT_POLL_CALLBACK_BRANCH_TARGETS = (
    (0x17B, 0x1A0),
    (0x200, 0x225),
    (0x20A, 0x225),
)


ZINPUT_POLL_CALLBACK_PROVENANCE_RELOCATIONS = (
    (0x175, IMAGE_REL_I386_DIR32, "_g_zInput_GlobalStateStorage", 0x3F14),
    (0x17F, IMAGE_REL_I386_DIR32, "_g_zInput_GlobalStateStorage", 0x3F18),
    (
        0x186,
        IMAGE_REL_I386_REL32,
        "?KeyboardTranslateDikToAscii@zInput@@YIHH@Z",
        0,
    ),
    (0x1FA, IMAGE_REL_I386_DIR32, "_g_zInput_GlobalStateStorage", 0x28),
    (0x205, IMAGE_REL_I386_DIR32, "_g_zInput_GlobalStateStorage", 0x24),
)


ZINPUT_RUNTIME_DISPATCH_CANDIDATE_SPECS = {
    "0x46f450": {
        "end": "0x46f690", "symbol": "?KeyboardResetTransitionState@zInput@@YAXXZ",
        "candidate_aliases": (
            "?KeyboardResetTransitionState@zInput@@YIXXZ",
        ),
        "name": "zInput::KeyboardResetTransitionState",
        "anchor": "recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_resettransitionstate",
        "retail_size": 0x240, "candidate_size": 0x240, "section": 7,
        "calls": (0x33, 0x4D, 0x17D),
        "leaf_calls": ((0x12, 0x2D, 0x32, 0x33, "ecx", 0x28), (0x45, 0x4B, 0x4A, 0x4D, "ecx", 0x1C)),
        "switch": {
            "relational_labels": True,
            "rows": (
                (0x6D, bytes.fromhex("81 f9 9b 00 00 00"), r"cmp\s+ecx\s*,\s*155"),
                (0x73, bytes.fromhex("77 72"), r"ja\s+(?:short\s+)?\$L[0-9]+"),
                (0x77, bytes.fromhex("8a 91 00 00 00 00"), r"mov\s+dl\s*,\s*(?:byte\s+(?:ptr\s+)?)?\$L[0-9]+\[ecx\]"),
                (0x7D, bytes.fromhex("ff 24 95 00 00 00 00"), r"jmp\s+(?:dword\s+(?:ptr\s+)?)?\$L[0-9]+\[edx\*4\]"),
            ),
            "classifier_offset": 0x1A4,
            "classifier_load_relocation": (0x79, "classifier-data"),
            "dispatch_offset": 0x7D,
            "dispatch_relocation": (0x80, "jump-table"),
            "table_offset": 0x188,
            "table_targets": (
                ("control-case", 0xA5), ("shift-case", 0xC6),
                ("shift-case", 0xC6), ("alt-case", 0x84),
                ("control-case", 0xA5), ("alt-case", 0x84),
                ("default-case", 0xE7),
            ),
        },
    },
    "0x46f690": {
        "end": "0x46f970", "symbol": "?KeyboardPollState@zInput@@YIXE@Z",
        "candidate_aliases": (),
        "name": "zInput::KeyboardPollState",
        "anchor": "recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_pollstate",
        "retail_size": 0x2E0, "candidate_size": 0x310, "section": 9,
        "calls": (0x2B, 0x45, 0x185, 0x18E, 0x20E, 0x241),
        "leaf_calls": ((0x03, 0x25, 0x2A, 0x2B, "ecx", 0x28), (0x3D, 0x43, 0x42, 0x45, "ecx", 0x1C)),
        "switch": {
            "relational_labels": True,
            "rows": (
                (0x6C, bytes.fromhex("81 fe 9b 00 00 00"), r"cmp\s+esi\s*,\s*155"),
                (0x72, bytes.fromhex("0f 87 8d 00 00 00"), r"ja\s+\$L[0-9]+"),
                (0x7A, bytes.fromhex("8a 96 00 00 00 00"), r"mov\s+dl\s*,\s*(?:byte\s+(?:ptr\s+)?)?\$L[0-9]+\[esi\]"),
                (0x80, bytes.fromhex("ff 24 95 00 00 00 00"), r"jmp\s+(?:dword\s+(?:ptr\s+)?)?\$L[0-9]+\[edx\*4\]"),
            ),
            "classifier_offset": 0x26C,
            "classifier_load_relocation": (0x7C, "classifier-data"),
            "dispatch_offset": 0x80,
            "dispatch_relocation": (0x83, "jump-table"),
            "table_offset": 0x250,
            "table_targets": (
                ("control-case", 0xB4), ("shift-case", 0xDE),
                ("shift-case", 0xDE), ("alt-case", 0x87),
                ("control-case", 0xB4), ("alt-case", 0x87),
                ("default-case", 0x105),
            ),
            "padding": (0x24E, bytes.fromhex("8b ff"), r"npad\s+2"),
        },
        "register_callbacks": ZINPUT_POLL_REGISTER_CALLBACKS,
        "callback_rows": ZINPUT_POLL_CALLBACK_PROVENANCE_ROWS,
        "callback_branch_targets": ZINPUT_POLL_CALLBACK_BRANCH_TARGETS,
        "callback_relocations": ZINPUT_POLL_CALLBACK_PROVENANCE_RELOCATIONS,
    },
    "0x46fa10": {
        "end": "0x46fba0", "symbol": "?KeyboardWaitForAnyKeyPress@zInput@@YIHH@Z",
        "candidate_aliases": (),
        "name": "zInput::KeyboardWaitForAnyKeyPress",
        "anchor": "recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-keyboard_waitforanykeypress",
        "retail_size": 0x190, "candidate_size": 0x190, "section": 21,
        "calls": (0x38, 0x52, 0x168),
        "leaf_calls": ((0x21, 0x33, 0x37, 0x38, "ecx", 0x28), (0x4A, 0x50, 0x4F, 0x52, "ecx", 0x1C)),
        "switch": None,
    },
}


ZINPUT_RUNTIME_AGGREGATE_LEAF_LOGICAL_ORDINALS = {
    "0x46f450": (0, 1),
    "0x46f690": (0, 1),
    "0x46fa10": (0, 1),
}


ZINPUT_TRANSLATE_CALLER_START = "0x46fba0"


ZINPUT_TRANSLATE_CALLER_END = "0x46fd20"


ZINPUT_TRANSLATE_CALLER_IDENTITY = "symbol:recoil:function:0x46fba0"


ZINPUT_TRANSLATE_CALLER_SYMBOL = (
    "?KeyboardTranslateDikToAscii@zInput@@YIHH@Z"
)


ZINPUT_TRANSLATE_CALLER_NAME = "zInput::KeyboardTranslateDikToAscii"


ZINPUT_TRANSLATE_CALLER_ANCHOR = (
    "recoil:anchor:src-gamezrecoil-zinput-zin_kbd-function-"
    "keyboard_translatediktoascii"
)


ZINPUT_TRANSLATE_OWNER_ID = (
    "recoil:owner:legacy.input_script_config.source_file_zinput_zin_kbd_cpp_"
    "dik_ascii_table_owner"
)


ZINPUT_TRANSLATE_SWITCH_CLASSIFIER = bytes.fromhex(
    "00 01 02 03 04 05 06 07 08 09 0a 0b "
    "15 15 15 15 15 15 15 15 15 15 15 15 0c 0d "
    "15 15 15 15 15 15 15 15 15 15 15 0e 0f 10 15 11 "
    "15 15 15 15 15 15 15 12 13 14"
)


ZINPUT_TRANSLATE_SWITCH = {
    "relational_labels": True,
    "rows": (
        (0x44, bytes.fromhex("83 f9 33"), r"cmp\s+ecx\s*,\s*51"),
        (0x47, bytes.fromhex("0f 87 a0 00 00 00"), r"ja\s+\$L[0-9]+"),
        (0x4D, bytes.fromhex("33 d2"), r"xor\s+edx\s*,\s*edx"),
        (
            0x4F, bytes.fromhex("8a 91 00 00 00 00"),
            r"mov\s+dl\s*,\s*(?:byte\s+(?:ptr\s+)?)?\$L[0-9]+\[ecx\]",
        ),
        (
            0x55, bytes.fromhex("ff 24 95 00 00 00 00"),
            r"jmp\s+(?:dword\s+(?:ptr\s+)?)?\$L[0-9]+\[edx\*4\]",
        ),
    ),
    "classifier": ZINPUT_TRANSLATE_SWITCH_CLASSIFIER,
    "classifier_offset": 0x148,
    "classifier_load_relocation": (0x51, "classifier-data"),
    "dispatch_offset": 0x55,
    "dispatch_relocation": (0x58, "jump-table"),
    "table_offset": 0xF0,
    "table_targets": (
        ("case-00", 0x5C), ("case-01", 0x63),
        ("case-02", 0x6A), ("case-03", 0x71),
        ("case-04", 0x78), ("case-05", 0x7F),
        ("case-06", 0x86), ("case-07", 0x8D),
        ("case-08", 0x94), ("case-09", 0x9B),
        ("case-10", 0xA9), ("case-11", 0xB0),
        ("case-12", 0xBE), ("case-13", 0xC5),
        ("case-14", 0xCC), ("case-15", 0xD3),
        ("case-16", 0xA2), ("case-17", 0xB7),
        ("case-18", 0xDA), ("case-19", 0xE1),
        ("case-20", 0xE8), ("default-case", 0xED),
    ),
    "padding": (0xEF, bytes.fromhex("90"), r"npad\s+1"),
}


ZINPUT_TRANSLATE_CODE = bytes.fromhex(
    "a1000000005685c08bf1750fe800000000c70500000000010000008bc625ff0000008b04850000000083f8617c1083f8"
    "7a7f0bf7c600040000740383e8208d8efefbffff83f9330f87a000000033d28a9100000000ff249500000000b8210000"
    "005ec3b8400000005ec3b8230000005ec3b8240000005ec3b8250000005ec3b85e0000005ec3b8260000005ec3b82a00"
    "00005ec3b8280000005ec3b8290000005ec3b87e0000005ec3b85f0000005ec3b82b0000005ec3b87c0000005ec3b87b"
    "0000005ec3b87d0000005ec3b83a0000005ec3b8220000005ec3b83c0000005ec3b83e0000005ec3b83f0000005ec390"
)


ZINPUT_TRANSLATE_CANDIDATE_BODY = (
    ZINPUT_TRANSLATE_CODE
    + b"\0" * (22 * 4)
    + ZINPUT_TRANSLATE_SWITCH_CLASSIFIER
    + b"\x90" * 4
)


ZINPUT_TRANSLATE_CODE_OFFSETS = (
    0x00, 0x05, 0x06, 0x08, 0x0A, 0x0C, 0x11, 0x1B, 0x1D, 0x22,
    0x29, 0x2C, 0x2E, 0x31, 0x33, 0x39, 0x3B, 0x3E, 0x44, 0x47,
    0x4D, 0x4F, 0x55, 0x5C, 0x61, 0x62, 0x63, 0x68, 0x69, 0x6A,
    0x6F, 0x70, 0x71, 0x76, 0x77, 0x78, 0x7D, 0x7E, 0x7F, 0x84,
    0x85, 0x86, 0x8B, 0x8C, 0x8D, 0x92, 0x93, 0x94, 0x99, 0x9A,
    0x9B, 0xA0, 0xA1, 0xA2, 0xA7, 0xA8, 0xA9, 0xAE, 0xAF, 0xB0,
    0xB5, 0xB6, 0xB7, 0xBC, 0xBD, 0xBE, 0xC3, 0xC4, 0xC5, 0xCA,
    0xCB, 0xCC, 0xD1, 0xD2, 0xD3, 0xD8, 0xD9, 0xDA, 0xDF, 0xE0,
    0xE1, 0xE6, 0xE7, 0xE8, 0xED, 0xEE, 0xEF,
)


ZINPUT_TRANSLATE_RELOCATIONS = (
    (0x01, IMAGE_REL_I386_DIR32, "_g_zInput_KbdDikToAsciiTableReady", 0),
    (0x0D, IMAGE_REL_I386_REL32, "?KeyboardInitDikToAsciiTable@zInput@@YAXXZ", 0),
    (0x13, IMAGE_REL_I386_DIR32, "_g_zInput_KbdDikToAsciiTableReady", 0),
    (0x25, IMAGE_REL_I386_DIR32, "_g_zInput_KbdDikToAsciiTable", 0),
    (0x51, IMAGE_REL_I386_DIR32, "classifier-data", 0),
    (0x58, IMAGE_REL_I386_DIR32, "jump-table", 0),
    *((0xF0 + index * 4, IMAGE_REL_I386_DIR32, role, 0)
      for index, (role, _target) in enumerate(ZINPUT_TRANSLATE_SWITCH["table_targets"])),
)


ZINPUT_MOUSE_TARGET_NAME = "zinput_mouse_470020_4706c0_authored_order"


ZINPUT_MOUSE_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zinput_mouse_470020_4706c0_authored_order.json"
)


ZINPUT_MOUSE_SOURCE_PATH = "src/GameZRecoil/zInput/zin_mouse.cpp"


ZINPUT_MOUSE_INIT_CALLER_START = "0x4701f0"


ZINPUT_MOUSE_INIT_CALLER_END_EXCLUSIVE = "0x4702e0"


ZINPUT_MOUSE_INIT_CALLER_IDENTITY = "symbol:recoil:function:0x4701f0"


ZINPUT_MOUSE_INIT_CALLER_SYMBOL = "?MouseInitDevice@zInput@@YAHXZ"


ZINPUT_MOUSE_INIT_CALLER_NAME = "zInput::MouseInitDevice"


ZINPUT_MOUSE_INIT_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-initdevice"
)


ZINPUT_MOUSE_INIT_COFF_SECTION_INDEX = 17


ZINPUT_MOUSE_INIT_CANDIDATE_SIZE = 0xF0


ZINPUT_MOUSE_DEVICE_SYMBOL_ID = "recoil:data:0x565e78"


ZINPUT_MOUSE_DEVICE_STORAGE_ID = "recoil:storage:va:0x565e78"


ZINPUT_MOUSE_DEVICE_ADDRESS = "0x565e78"


ZINPUT_MOUSE_DEVICE_NAME = "g_zInput_MouseDevice"


ZINPUT_MOUSE_DEVICE_DISPLACEMENT = 0x41C8


ZINPUT_MOUSE_INIT_AGGREGATE_LEAF_RELOCATIONS = (0x25, 0x3E, 0x54, 0x78)


ZINPUT_MOUSE_INIT_INVOCATION_OFFSETS = (
    0x1D,
    0x31,
    0x3A,
    0x4A,
    0x63,
    0x92,
    0xBB,
    0xC8,
    0xCD,
)


ZINPUT_MOUSE_INIT_LEAF_CHAINS = (
    (3, 0x3D, 0x48, 0x47, 0x4A, "ecx", 0x2C),
    (4, 0x53, 0x5F, 0x62, 0x63, "edx", 0x34),
    (5, 0x77, 0x8C, 0x91, 0x92, "edx", 0x18),
)


ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_START = "0x470310"


ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_END_EXCLUSIVE = "0x470360"


ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x470310"
)


ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_SYMBOL = (
    "?MouseUpdateAcquireState@zInput@@YAXXZ"
)


ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_NAME = (
    "zInput::MouseUpdateAcquireState"
)


ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zinput.zin-mouse."
    "mouse-updateacquirestate"
)


ZINPUT_MOUSE_UPDATE_ACQUIRE_COFF_SECTION_INDEX = 21


ZINPUT_MOUSE_UPDATE_ACQUIRE_CANDIDATE_SIZE = 0x50


ZINPUT_MOUSE_UPDATE_ACQUIRE_BODY = bytes.fromhex(
    "a10000000085c0a1c8410000741e85c074378b0850ff511c85c0742d83f801"
    "7428c7050000000000000000c385c074198b1050ff522085c0740f83f801740a"
    "c7050000000001000000c3909090909090"
)


ZINPUT_MOUSE_UPDATE_ACQUIRE_RELOCATIONS = (
    (0x01, IMAGE_REL_I386_DIR32, "_g_zInput_MouseActive", 0),
    (
        0x08,
        IMAGE_REL_I386_DIR32,
        ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
        ZINPUT_MOUSE_DEVICE_DISPLACEMENT,
    ),
    (0x23, IMAGE_REL_I386_DIR32, "_g_zInput_MouseActive", 0),
    (0x41, IMAGE_REL_I386_DIR32, "_g_zInput_MouseActive", 0),
)


ZINPUT_MOUSE_UPDATE_ACQUIRE_INVOCATION_OFFSETS = (0x15, 0x33)


ZINPUT_MOUSE_UPDATE_ACQUIRE_LEAF_CHAINS = (
    (0, 0x07, 0x12, 0x14, 0x15, "ecx", 0x1C),
)


ZINPUT_MOUSE_UPDATE_ACQUIRE_BRANCH_SUCCESSORS = (
    (0x0C, (0x0E, 0x2C)),
    (0x10, (0x12, 0x49)),
    (0x1A, (0x1C, 0x49)),
    (0x1F, (0x21, 0x49)),
    (0x2E, (0x30, 0x49)),
    (0x38, (0x3A, 0x49)),
    (0x3D, (0x3F, 0x49)),
)


ZINPUT_MOUSE_SHUTDOWN_CALLER_START = "0x470360"


ZINPUT_MOUSE_SHUTDOWN_CALLER_END_EXCLUSIVE = "0x4703a0"


ZINPUT_MOUSE_SHUTDOWN_CALLER_IDENTITY = "symbol:recoil:function:0x470360"


ZINPUT_MOUSE_SHUTDOWN_CALLER_SYMBOL = "?MouseShutdownDevice@zInput@@YAHXZ"


ZINPUT_MOUSE_SHUTDOWN_CALLER_NAME = "zInput::MouseShutdownDevice"


ZINPUT_MOUSE_SHUTDOWN_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-shutdowndevice"
)


ZINPUT_MOUSE_SHUTDOWN_COFF_SECTION_INDEX = 23


ZINPUT_MOUSE_SHUTDOWN_CANDIDATE_SIZE = 0x40


ZINPUT_MOUSE_SHUTDOWN_BODY = bytes.fromhex(
    "c7050000000000000000e800000000a1c841000085c074068b0850ff5108c705"
    "c841000000000000c705c441000000000000b801000000c39090909090909090"
)


ZINPUT_MOUSE_SHUTDOWN_RELOCATIONS = (
    (0x02, IMAGE_REL_I386_DIR32, "_g_zInput_MouseActive", 0),
    (
        0x0B,
        IMAGE_REL_I386_REL32,
        ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_SYMBOL,
        0,
    ),
    (
        0x10,
        IMAGE_REL_I386_DIR32,
        ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
        ZINPUT_MOUSE_DEVICE_DISPLACEMENT,
    ),
    (
        0x20,
        IMAGE_REL_I386_DIR32,
        ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
        ZINPUT_MOUSE_DEVICE_DISPLACEMENT,
    ),
    (
        0x2A,
        IMAGE_REL_I386_DIR32,
        ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL,
        0x41C4,
    ),
)


ZINPUT_MOUSE_SHUTDOWN_INVOCATION_OFFSETS = (0x0A, 0x1B)


ZINPUT_MOUSE_SHUTDOWN_LEAF_CHAINS = (
    (1, 0x0F, 0x18, 0x1A, 0x1B, "ecx", 0x08),
)


ZINPUT_MOUSE_SHUTDOWN_BRANCH_SUCCESSORS = (
    (0x16, (0x18, 0x1E)),
)


ZINPUT_MOUSE_POLL_CALLER_START = "0x4703c0"


ZINPUT_MOUSE_POLL_CALLER_END_EXCLUSIVE = "0x4704f0"


ZINPUT_MOUSE_POLL_CALLER_IDENTITY = "symbol:recoil:function:0x4703c0"


ZINPUT_MOUSE_POLL_CALLER_SYMBOL = "?MousePollState@zInput@@YIHE@Z"


ZINPUT_MOUSE_POLL_CALLER_NAME = "zInput::MousePollState"


ZINPUT_MOUSE_POLL_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zinput.zin-mouse.mouse-pollstate"
)


ZINPUT_MOUSE_POLL_COFF_SECTION_INDEX = 29


ZINPUT_MOUSE_POLL_CANDIDATE_SIZE = 0x130


ZINPUT_MOUSE_POLL_BODY = bytes.fromhex(
    "51a1000000005685c0884c2407c7051000000000000000c7051400000000000000"
    "7520c7050000000001000000e800000000a10000000085c07508b81e0007805e"
    "59c38b35c8410000568b06ff50648b0e68000000006a1056ff51248bf081fe1e"
    "000780750ae8000000008bc65e59c385f60f85b00000008b15cc410000a1d041"
    "00008b0dd44100008915dc4100008b15d8410000a3e0410000a1000000008915"
    "e84100008b1508000000890de44100008b0d040000008915d44100008b150c00"
    "0000a3cc410000890dd04100008915d8410000a310000000890d14000000e800"
    "000000b901000000e800000000b902000000a320000000e800000000b9030000"
    "00a324000000e8000000008b0df0410000a32800000085c9740d8a44240784c0"
    "7405e8000000008bc65e59c3909090"
)


ZINPUT_MOUSE_POLL_RELOCATIONS = (
    (0x02, IMAGE_REL_I386_DIR32, "_g_zInput_MouseActive", 0),
    (0x0F, IMAGE_REL_I386_DIR32, "_g_zInput_MouseStateSnapshot", 0x10),
    (0x19, IMAGE_REL_I386_DIR32, "_g_zInput_MouseStateSnapshot", 0x14),
    (0x25, IMAGE_REL_I386_DIR32, "_g_zInput_MouseActive", 0),
    (0x2E, IMAGE_REL_I386_REL32, ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_SYMBOL, 0),
    (0x33, IMAGE_REL_I386_DIR32, "_g_zInput_MouseActive", 0),
    (0x45, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41C8),
    (0x52, IMAGE_REL_I386_DIR32, "_g_zInput_MouseRawDIState", 0),
    (0x67, IMAGE_REL_I386_REL32, ZINPUT_MOUSE_UPDATE_ACQUIRE_CALLER_SYMBOL, 0),
    (0x7A, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41CC),
    (0x7F, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41D0),
    (0x85, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41D4),
    (0x8B, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41DC),
    (0x91, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41D8),
    (0x96, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41E0),
    (0x9B, IMAGE_REL_I386_DIR32, "_g_zInput_MouseRawDIState", 0),
    (0xA1, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41E8),
    (0xA7, IMAGE_REL_I386_DIR32, "_g_zInput_MouseRawDIState", 8),
    (0xAD, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41E4),
    (0xB3, IMAGE_REL_I386_DIR32, "_g_zInput_MouseRawDIState", 4),
    (0xB9, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41D4),
    (0xBF, IMAGE_REL_I386_DIR32, "_g_zInput_MouseRawDIState", 0x0C),
    (0xC4, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41CC),
    (0xCA, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41D0),
    (0xD0, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41D8),
    (0xD5, IMAGE_REL_I386_DIR32, "_g_zInput_MouseStateSnapshot", 0x10),
    (0xDB, IMAGE_REL_I386_DIR32, "_g_zInput_MouseStateSnapshot", 0x14),
    (0xE0, IMAGE_REL_I386_REL32, "?MouseApplyAccumulatedDelta@zInput@@YAXXZ", 0),
    (0xEA, IMAGE_REL_I386_REL32, "?MouseGetButtonTransitionState@zInput@@YIHH@Z", 0),
    (0xF4, IMAGE_REL_I386_DIR32, "_g_zInput_MouseStateSnapshot", 0x20),
    (0xF9, IMAGE_REL_I386_REL32, "?MouseGetButtonTransitionState@zInput@@YIHH@Z", 0),
    (0x103, IMAGE_REL_I386_DIR32, "_g_zInput_MouseStateSnapshot", 0x24),
    (0x108, IMAGE_REL_I386_REL32, "?MouseGetButtonTransitionState@zInput@@YIHH@Z", 0),
    (0x10E, IMAGE_REL_I386_DIR32, ZINPUT_JOYSTICK_AGGREGATE_OBJECT_SYMBOL, 0x41F0),
    (0x113, IMAGE_REL_I386_DIR32, "_g_zInput_MouseStateSnapshot", 0x28),
    (0x124, IMAGE_REL_I386_REL32, "?DispatchMouseButtonCallbacks@zInput_BindMapContext@@QAEXXZ", 0),
)


ZINPUT_MOUSE_POLL_INVOCATION_OFFSETS = (
    0x2D, 0x4C, 0x59, 0x66, 0xDF, 0xE9, 0xF8, 0x107, 0x123,
)


ZINPUT_MOUSE_POLL_BRANCH_SUCCESSORS = (
    (0x21, (0x23, 0x43)),
    (0x39, (0x3B, 0x43)),
    (0x64, (0x66, 0x70)),
    (0x72, (0x78, 0x128)),
    (0x119, (0x11B, 0x128)),
    (0x121, (0x123, 0x128)),
)


ZSND_INIT_TARGET_NAME = "zsnd_init_4a12c0_4a2010_authored_order"


ZSND_INIT_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zsnd_init_4a12c0_4a2010_authored_order.json"
)


ZSND_INIT_SOURCE_PATH = "src/GameZRecoil/zSound/zsnd_init.cpp"


ZSND_DIRECTSOUND_CREATE_CALLER_START = "0x4a1e50"


ZSND_DIRECTSOUND_CREATE_CALLER_END_EXCLUSIVE = "0x4a1f40"


ZSND_DIRECTSOUND_CREATE_CALLER_IDENTITY = (
    "symbol:recoil:function:0x4a1e50"
)


ZSND_DIRECTSOUND_CREATE_CALLER_SYMBOL = "_zSndBackendInitDirectSound"


ZSND_DIRECTSOUND_CREATE_CALLER_ANCHOR_ID = (
    "recoil:anchor:gamezrecoil.zsound.zsnd-init."
    "zsndbackend-initdirectsound"
)


ZSND_DIRECTSOUND_CREATE_PROVIDER_SYMBOL_ID = "recoil:function:0x4c63d8"


ZSND_DIRECTSOUND_CREATE_PROVIDER_IDENTITY = (
    "provider:recoil:function:0x4c63d8"
)


ZSND_DIRECTSOUND_CREATE_PROVIDER_BLOCK_ID = "recoil:block:0x4c637c"


ZSND_DIRECTSOUND_CREATE_THUNK_ADDRESS = "0x4c63d8"


ZSND_DIRECTSOUND_CREATE_THUNK_END_EXCLUSIVE = "0x4c63de"


ZSND_DIRECTSOUND_CREATE_THUNK_BODY = bytes.fromhex("ff 25 5c c0 4c 00")


ZSND_DIRECTSOUND_CREATE_IAT_ADDRESS = "0x4cc05c"


ZSND_DIRECTSOUND_CREATE_IMPORT_DLL = "DSOUND.dll"


ZSND_DIRECTSOUND_CREATE_IMPORT_ORDINAL = 1


ZSND_DIRECTSOUND_CREATE_RETAIL_CALLABLE_SYMBOL = "Ordinal_DSOUND_1"


ZSND_DIRECTSOUND_CREATE_RETAIL_IAT_SYMBOL = "__imp_Ordinal_DSOUND_1"


ZSND_DIRECTSOUND_CREATE_CANDIDATE_SYMBOL = "_DirectSoundCreate@12"


ZSND_DIRECTSOUND_CREATE_CANDIDATE_IAT_SYMBOL = (
    "__imp__DirectSoundCreate@12"
)


ZSND_DIRECTSOUND_CREATE_RETAIL_CALL_ADDRESS = "0x4a1e5c"


ZSND_DIRECTSOUND_CREATE_OTHER_RETAIL_CALL_ADDRESS = "0x4b2f80"


ZSND_DIRECTSOUND_CREATE_RETAIL_CALL_BODY = bytes.fromhex("e8 77 45 02 00")


ZSND_DIRECTSOUND_CREATE_CANDIDATE_CALL_OFFSET = 0x0C


ZSND_DIRECTSOUND_CREATE_CANDIDATE_RELOCATION_OFFSET = 0x0D


ZSND_DIRECTSOUND_CREATE_COFF_SECTION_INDEX = 23


ZSND_DIRECTSOUND_CREATE_CANDIDATE_SIZE = 0xF0


ZGAMEOPT_TARGET_NAME = "zgame_opt_4b2960_4b33f0_authored_order"


ZGAMEOPT_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "zgame_opt_4b2960_4b33f0_authored_order.json"
)


ZGAMEOPT_SOURCE_PATH = "src/GameZRecoil/zGame/zgame_opt.c"


DIRECTSOUND_CREATE_CALLER_PROFILES: Mapping[str, Mapping[str, Any]] = {
    ZSND_DIRECTSOUND_CREATE_CALLER_START: {
        "identity": ZSND_DIRECTSOUND_CREATE_CALLER_IDENTITY,
        "end_exclusive": ZSND_DIRECTSOUND_CREATE_CALLER_END_EXCLUSIVE,
        "symbol": ZSND_DIRECTSOUND_CREATE_CALLER_SYMBOL,
        "navigation_name": "zSndBackend::InitDirectSound",
        "physical_block_id": "recoil:block:0x4a12c0",
        "candidate_size": ZSND_DIRECTSOUND_CREATE_CANDIDATE_SIZE,
        "candidate_section_index": ZSND_DIRECTSOUND_CREATE_COFF_SECTION_INDEX,
        "candidate_call_offset": ZSND_DIRECTSOUND_CREATE_CANDIDATE_CALL_OFFSET,
        "candidate_relocation_offset": (
            ZSND_DIRECTSOUND_CREATE_CANDIDATE_RELOCATION_OFFSET
        ),
        "retail_call_address": ZSND_DIRECTSOUND_CREATE_RETAIL_CALL_ADDRESS,
        "retail_call_body": ZSND_DIRECTSOUND_CREATE_RETAIL_CALL_BODY,
        "expected_ordinal": 0,
        "target_name": ZSND_INIT_TARGET_NAME,
        "target_manifest": ZSND_INIT_TARGET_MANIFEST,
        "source_path": ZSND_INIT_SOURCE_PATH,
        "source_trace_state": "resolved",
        "source_trace_reason": None,
        "source_edges": (
            {
                "anchor_id": ZSND_DIRECTSOUND_CREATE_CALLER_ANCHOR_ID,
                "emission_context": {
                    "translation_unit": ZSND_INIT_SOURCE_PATH
                },
                "evidence_ids": [],
                "relation": "defines",
            },
        ),
    },
    "0x4b2f50": {
        "identity": "symbol:recoil:function:0x4b2f50",
        "end_exclusive": "0x4b2fa0",
        "symbol": (
            "?AcquireCachedDirectSound@zSnd@@"
            "YIPAUIDirectSound@@PBU_GUID@@@Z"
        ),
        "navigation_name": "zSnd::AcquireCachedDirectSound",
        "physical_block_id": "recoil:block:0x4b2960",
        "candidate_size": 0x50,
        "candidate_section_index": 8,
        "candidate_call_offset": 0x2C,
        "candidate_relocation_offset": 0x2D,
        "retail_call_address": ZSND_DIRECTSOUND_CREATE_OTHER_RETAIL_CALL_ADDRESS,
        "retail_call_body": bytes.fromhex("e8 53 34 01 00"),
        "expected_ordinal": 1,
        "target_name": ZGAMEOPT_TARGET_NAME,
        "target_manifest": ZGAMEOPT_TARGET_MANIFEST,
        "source_path": ZGAMEOPT_SOURCE_PATH,
        "source_trace_state": "unresolved",
        "source_trace_reason": "multiple-legacy-occurrences",
        "source_edges": (),
    },
}


BRIEFING_START_SLEEP_REGISTER_IAT_BRIDGE = ReviewedRegisterIatBridgeSpec(
    label="Briefing StartForMission Sleep register-IAT bridge",
    caller_identity="symbol:recoil:function:0x404180",
    caller_start="0x404180",
    caller_end_exclusive="0x404280",
    caller_symbol="?StartForMission@Briefing@@YIHH@Z",
    register="esi",
    import_dll="KERNEL32.dll",
    import_name="Sleep",
    owner_id="recoil:owner:provider.kernel32.sleep_import",
    retail_storage_address="0x4cc0b0",
    retail_storage_type=(
        "void (__stdcall* const)(uint32_t dwMilliseconds)"
    ),
    retail_load_address="0x404253",
    retail_load_body=b"\x8b\x35\xb0\xc0\x4c\x00",
    retail_call_address="0x40425b",
    retail_call_body=b"\xff\xd6",
    retail_backedge_address="0x404264",
    retail_backedge_mnemonic="je",
    retail_backedge_body=b"\x74\xf3",
    candidate_import_symbol="__imp__Sleep@4",
    candidate_load_address="0xd3",
    candidate_load_body=b"\x8b\x35\x00\x00\x00\x00",
    candidate_call_address="0xdb",
    candidate_call_body=b"\xff\xd6",
    candidate_backedge_address="0xe4",
    candidate_backedge_mnemonic="je",
    candidate_backedge_body=b"\x74\xf3",
    candidate_relocation_offset=0xD5,
    ordinal=7,
    cleanup_bytes=None,
    invocation_count=1,
)


ZSYS_DIRECTDRAWCREATE_DYNAMIC_EXPORT_BRIDGE = ReviewedDynamicExportCallSpec(
    label="zSys DirectDrawCreate dynamic-export bridge",
    caller_identity="symbol:recoil:function:0x40c370",
    caller_start="0x40c370",
    caller_end_exclusive="0x40c6e0",
    caller_symbol=(
        "?ProbePlatformAndVideoCaps@zSys@@YIXPAW4zSysVideoCapsLevel@@"
        "PAW4zSysPlatformCapsLevel@@@Z"
    ),
    getproc_iat_address="0x4cc0bc",
    getproc_import_dll="KERNEL32.dll",
    getproc_import_name="GetProcAddress",
    getproc_import_symbol="__imp__GetProcAddress@8",
    export_data_id="recoil:data:0x4dab78",
    export_address="0x4dab78",
    export_navigation_name="g_zSys_ProbeDirectDrawCreateExportName",
    export_value="DirectDrawCreate",
    candidate_export_symbol_pattern=(
        r"_g_zSys_ProbeDirectDrawCreateExportName\$S\d+"
    ),
    provider_owner_id="recoil:owner:provider.directx6.directdraw",
    provider_function_id="recoil:function:0x4c63de",
    provider_address="0x4c63de",
    provider_end_exclusive="0x4c63e4",
    provider_navigation_name="DDRAW_DirectDrawCreate_ImportThunk",
    provider_physical_block_id="recoil:block:0x4c637c",
    register="eax",
    retail_getproc_call_address="0x40c496",
    retail_getproc_call_body=b"\xff\x15\xbc\xc0\x4c\x00",
    retail_test_address="0x40c49c",
    retail_test_body=b"\x85\xc0",
    retail_branch_address="0x40c49e",
    retail_branch_body=b"\x75\x22",
    retail_dynamic_call_address="0x40c4cb",
    retail_dynamic_call_body=b"\xff\xd0",
    candidate_getproc_call_address="0x126",
    candidate_getproc_call_body=b"\xff\x15\x00\x00\x00\x00",
    candidate_test_address="0x12c",
    candidate_test_body=b"\x85\xc0",
    candidate_branch_address="0x12e",
    candidate_branch_body=b"\x75\x22",
    candidate_dynamic_call_address="0x15b",
    candidate_dynamic_call_body=b"\xff\xd0",
    candidate_export_relocation_offset=0x121,
    candidate_getproc_relocation_offset=0x128,
    ordinal=11,
    cleanup_bytes=None,
    invocation_count=1,
)


BRIEFING_BEGINTHREAD_DIRECT_IAT_STORAGE_BRIDGE = (
    ReviewedDirectIatStorageCallSpec(
        label="Briefing StartForMission _beginthread direct-IAT bridge",
        caller_identity="symbol:recoil:function:0x404180",
        caller_start="0x404180",
        caller_end_exclusive="0x404280",
        caller_symbol="?StartForMission@Briefing@@YIHH@Z",
        import_dll="MSVCRT.dll",
        import_name="_beginthread",
        retail_storage_address="0x4cc5a4",
        retail_call_address="0x404214",
        retail_storage_type=(
            "uintptr_t (* const)(_beginthread_proc_type _StartAddress, "
            "uint32_t _StackSize, void* _ArgList)"
        ),
        candidate_import_symbol="__imp___beginthread",
        candidate_call_address="0x94",
        candidate_relocation_offset=0x96,
        owner_id="recoil:owner:provider.crt.beginthread_import",
        ordinal=4,
        cleanup_bytes=12,
        invocation_count=1,
    )
)


BRIEFING_STRERROR_DIRECT_IAT_STORAGE_BRIDGE = (
    ReviewedDirectIatStorageCallSpec(
        label="Briefing StartForMission strerror direct-IAT bridge",
        caller_identity="symbol:recoil:function:0x404180",
        caller_start="0x404180",
        caller_end_exclusive="0x404280",
        caller_symbol="?StartForMission@Briefing@@YIHH@Z",
        import_dll="MSVCRT.dll",
        import_name="strerror",
        retail_storage_address="0x4cc5a8",
        retail_call_address="0x404224",
        retail_storage_type="char* (* const)(int32_t _ErrorMessage)",
        candidate_import_symbol="__imp__strerror",
        candidate_call_address="0xa4",
        candidate_relocation_offset=0xA6,
        owner_id="recoil:owner:provider.crt.strerror_import",
        ordinal=5,
        cleanup_bytes=4,
        invocation_count=1,
    )
)


BRIEFING_LOCATOR_PANEL_VFTABLE_BRIDGE = ReviewedVftableStorageBridgeSpec(
    label="briefing locator-panel vftable bridge",
    vftable_symbol=HUD_BRIEFING_LOCATOR_PANEL_VFTABLE,
    caller_start="0x403c10",
    caller_end_exclusive="0x403c80",
    caller_symbol=HUD_BRIEFING_LOCATOR_PANEL_CONSTRUCTOR_SYMBOL,
    storage_identity=HUD_BRIEFING_LOCATOR_PANEL_STORAGE_IDENTITY,
    storage_address="0x4cc998",
    storage_id="recoil:storage:va:0x4cc998",
    owner_id="recoil:owner:legacy.hud_ui.class_huduibriefinglocatorpanel",
    owner_kind="class",
    source_path="src/Battlesport/Briefing.cpp",
    table_size=0x74,
    slot_displacement=HUD_BRIEFING_LOCATOR_PANEL_VFTABLE_SLOT,
    slot_symbol=HUD_ELEMENT_SET_VISIBLE_SYMBOL,
    slot_target_address="0x404d20",
    vptr_store_prefix=b"\xc7\x06",
    invocation_count=1,
    provisional_identity=(
        "candidate-storage:HudUiBriefingLocatorPanel-vftable"
    ),
)


CALL_CONTRACT_TIMING_KEYS = (
    "source_closure_dependency_setup",
    "definition_compilation",
    "target_candidate_compilation",
    "candidate_cod_file_index",
    "candidate_cod_procedure_lookup",
    "binary_ninja_inventory",
    "binary_ninja_assembly",
    "extraction_compare",
    "signature_recheck",
    "total",
)


CALL_CONTRACT_SOURCE_SUFFIXES = frozenset({".c", ".cc", ".cpp", ".cxx"})


CALL_CONTRACT_HEADER_SUFFIXES = frozenset(
    {".h", ".hh", ".hpp", ".hxx", ".inl"}
)


CALL_CONTRACT_FINAL_BUILD_MANIFEST_PATH = (
    "tools/_recoil/config/vc5_final_build.json"
)


CALL_CONTRACT_INCLUDE_DIRECTIVE_RE = re.compile(
    r"^[ \t]*\#[ \t]*include\b(?P<operand>[^\r\n]*)$",
    re.MULTILINE,
)


CALL_CONTRACT_QUOTED_INCLUDE_OPERAND_RE = re.compile(
    r'^"(?P<path>[^"]+)"[ \t]*(?:(?://[^\r\n]*)|(?:/\*[^\r\n]*\*/[ \t]*))?$'
)


APPFRAME_V9_TARGET_ID = (
    "recoil:vc5-target:appframe_442890_443730_authored_order"
)


APPFRAME_V9_TARGET_NAME = "appframe_442890_443730_authored_order"


APPFRAME_V9_MANIFEST_PATH = (
    "tools/vc5_verify_targets/appframe_442890_443730_authored_order.json"
)


APPFRAME_V9_SOURCE_PATH = "src/Battlesport/RecoilApp.cpp"


APPFRAME_V9_PROFILE_NAME = "vc5_o2_ob1_md_gx_fastcall_facs"


APPFRAME_V9_QUEUE_DEFINE = "/DRECOILAPP_VC5_STL_STATE_QUEUE_MEMBER"


APPFRAME_V9_ADDRESSES = (
    "0x442890", "0x4428a0", "0x4428b0", "0x4429d0", "0x442a10",
    "0x442a30", "0x442a50", "0x442bc0", "0x442c00", "0x442c10",
    "0x442c70", "0x442d00", "0x443140", "0x443160", "0x443310",
    "0x4434b0", "0x443650", "0x443690", "0x443700",
)


APPFRAME_V9_SYMBOLS = (
    "?GetMessageMapForRecoilApp@RecoilMfcWinAppAccess@@SGPBUAFX_MSGMAP@@XZ",
    "?GetMessageMap@RecoilApp_MfcOleModule@@UBEPBUAFX_MSGMAP@@XZ",
    "??1RecoilApp_MfcOleModule@@UAE@XZ",
    "?InitInstance@RecoilApp_MfcOleModule@@UAEHXZ",
    "?TakeSkipWaitMessage@RecoilApp@@QAEHXZ",
    "?MarkSkipWaitMessage@RecoilApp@@QAEHXZ",
    "?EngineInit@RecoilApp@@QAEHPAUHWND__@@@Z",
    "?ShutdownSubsystems@RecoilApp@@QAEXXZ",
    "?GetMainWnd@RecoilApp@@QBEPAUCZRecoilFrame@@XZ",
    "?StartEngineAndQueueStartupState@RecoilApp@@QAEHXZ",
    "??0RecoilApp_MfcOleModule@@QAE@XZ",
    "?Run@RecoilApp_MfcOleModule@@UAEHXZ",
    "?GetCurrentState@RecoilApp@@QBEPAURecoilApp_IState@@XZ",
    "?QueueSwitchCurrentState@RecoilApp@@QAEPAURecoilApp_IState@@PAU2@H@Z",
    "?QueuePushState@RecoilApp@@QAEPAURecoilApp_IState@@PAU2@H@Z",
    "?QueueExitCurrentState@RecoilApp@@QAEPAURecoilApp_IState@@H@Z",
    "?OnIdleOrDispatch@RecoilApp@@UAEHII@Z",
    "?GrowAndCenterChunkBaseList@RecoilApp_StateQueue@@QAEPAPAPAURecoilApp_StateQueueItem@@H@Z",
    "?InitFromCursor@RecoilApp_StateQueueBlock@@QAEPAU1@PAPAURecoilApp_StateQueueItem@@PAPAPAU2@@Z",
)


APPFRAME_V9_BASE_FLAGS = (
    "/nologo", "/TP", "/W3", "/MD", "/G5", "/O2", "/Ob1", "/GX",
    "/Gr", "/Zp4", "/DWIN32", "/D_WINDOWS", "/DNDEBUG", "/D_AFXDLL",
    "/D_MBCS", "/DSTRICT", "/D_CRT_SECURE_NO_WARNINGS",
    "/DWINDOWS_IGNORE_PACKING_MISMATCH", "/DWIN32_LEAN_AND_MEAN",
    "/DNOMINMAX", "/DRECOIL_ENABLE_ZSYS_CPU_RAW_ASM",
    "/DRECOIL_ENABLE_ZRNDR_OVERLAY_MMX_RAW_ASM",
    "/DRECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM", "/FAcs",
)


_CANDIDATE_BODY_ZERO_ARGUMENT_IDENTITY_RE = re.compile(
    r"^\?(?P<body>[A-Za-z_][A-Za-z0-9_]*(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@Y(?P<calling>[AGI])(?P<return>[A-Za-z0-9_?$@]+)XZ$"
)


INTENTIONALLY_INLINED_ABSENCE_PROOF_KIND = (
    "call-contract-intentionally-inlined-empty-authored-body"
)


INTENTIONALLY_INLINED_ABSENCE_PROOF_MODE = (
    "exact-inline-empty-lifecycle-linked-absence"
)


_VC5_VIRTUAL_DESTRUCTOR_IDENTITY_RE = re.compile(
    r"^\?\?1(?P<class_name>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)@@UAE@XZ$"
)


REGISTER_STATE_NAMES = (
    "eax",
    "ebx",
    "ecx",
    "edx",
    "esi",
    "edi",
    "esp",
    "ebp",
)


SHORT_JCC_MNEMONICS = (
    frozenset({"jo"}),
    frozenset({"jno"}),
    frozenset({"jb", "jc", "jnae"}),
    frozenset({"jae", "jnb", "jnc"}),
    frozenset({"je", "jz"}),
    frozenset({"jne", "jnz"}),
    frozenset({"jbe", "jna"}),
    frozenset({"ja", "jnbe"}),
    frozenset({"js"}),
    frozenset({"jns"}),
    frozenset({"jp", "jpe"}),
    frozenset({"jnp", "jpo"}),
    frozenset({"jl", "jnge"}),
    frozenset({"jge", "jnl"}),
    frozenset({"jle", "jng"}),
    frozenset({"jg", "jnle"}),
)


_COD_LOCAL_BRANCH_LABEL_RE = re.compile(
    r"(?:(?:short|near)\s+)?(?:"
    r"\$L[0-9A-Za-z_]+|"
    r"\$[A-Za-z_][0-9A-Za-z_]*\$[0-9]+"
    r")",
    re.IGNORECASE,
)


ZSND_RELEASE_UNKNOWN_COFF_NAME = (
    "?ReleaseUnknown@?%"
    + str(
        REPO_ROOT / "src" / "GameZRecoil" / "zSound" / "zsnd_init.cpp"
    )
    + "921116370@zSndBackend@@YIXAAPAX@Z"
)


ZSND_RELEASE_UNKNOWN_COD_NAME = (
    ZSND_RELEASE_UNKNOWN_COFF_NAME.replace("@?%", "@?", 1)
)


ZSND_RELEASE_UNKNOWN_SOURCE_PATH = (
    REPO_ROOT / "src" / "GameZRecoil" / "zSound" / "zsnd_init.cpp"
)


_ZSND_RELEASE_UNKNOWN_COFF_NAME_RE = re.compile(
    re.escape("?ReleaseUnknown@?%" + str(ZSND_RELEASE_UNKNOWN_SOURCE_PATH))
    + r"[0-9]+@zSndBackend@@YIXAAPAX@Z"
)


ZSND_RELEASE_UNKNOWN_BODY = bytes.fromhex(
    "56 8b f1 8b 06 85 c0 74 0c 8b 08 50 ff 51 08 c7 "
    "06 00 00 00 00 5e c3 90 90 90 90 90 90 90 90 90"
)


_COD_SPACE_BEARING_DIRECT_TARGET_RE = re.compile(
    r"(?P<head>\?[^\s]+?@\?)"
    r"(?P<source>[A-Za-z]:[\\/][^@\r\n]*[ \t][^@\r\n]*"
    r"\.(?:c|cc|cpp|cxx)[0-9]+)"
    r"(?P<scope>@@|@[A-Za-z_?$][A-Za-z0-9_?$]*@@)"
    r"(?P<suffix>[A-Za-z0-9_?$@]+Z)",
    re.IGNORECASE,
)


_COD_SPACE_BEARING_DIRECT_TARGET_COMMENT_RE = re.compile(
    r"(?P<head>\?[^\s]+?@\?)%"
    r"(?P<source>[A-Za-z]:[\\/][^@\r\n]*[ \t][^@\r\n]*"
    r"\.(?:c|cc|cpp|cxx)[0-9]+)"
    r"(?P<scope>@@|@[A-Za-z_?$][A-Za-z0-9_?$]*@@)"
    r"(?P<suffix>[A-Za-z0-9_?$@]+Z)",
    re.IGNORECASE,
)


RECOIL_MAIN_MENU_TRANSITION_CALLER_START = "0x415220"


RECOIL_MAIN_MENU_TRANSITION_CALLER_END_EXCLUSIVE = "0x415370"


RECOIL_MAIN_MENU_TRANSITION_CALLER_IDENTITY = "symbol:recoil:function:0x415220"


RECOIL_MAIN_MENU_TRANSITION_CALLER_SYMBOL = (
    "?OnTryBecomeCurrent@RecoilStateMainMenuTransition@@UAEHXZ"
)


RECOIL_MAIN_MENU_TRANSITION_CALLER_ANCHOR_ID = (
    "recoil:anchor:battlesport.hud."
    "recoilstatemainmenutransition-ontrybecomecurrent"
)


RECOIL_MAIN_MENU_TRANSITION_VERIFICATION_TARGET_IDS = (
    "recoil:vc5-target:hud_404ca0_415ab0_authored_order",
    "recoil:vc5-target:"
    "recoil_state_main_menu_transition_on_try_become_current",
)


RECOIL_MAIN_MENU_TRANSITION_CONTRIBUTION_SYMBOL_RE = (
    r"\?OnTryBecomeCurrent@RecoilStateMainMenuTransition@@.*"
)


RECOIL_MAIN_MENU_TRANSITION_BLUR_CTOR_ADDRESS = "0x463850"


RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_ADDRESS = "0x463870"


RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_ADDRESS = "0x463950"


RECOIL_MAIN_MENU_TRANSITION_BLUR_END_ADDRESS = "0x463920"


RECOIL_MAIN_MENU_TRANSITION_BLUR_VTABLE_ADDRESS = "0x4d25e0"


RECOIL_MAIN_MENU_TRANSITION_BLUR_STORAGE_IDENTITY = "load(stack+0x8)"


RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_CALL = "0x41528d"


RECOIL_MAIN_MENU_TRANSITION_BLUR_BEGIN_SLOT = 0x08


RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_CALL = "0x41529c"


RECOIL_MAIN_MENU_TRANSITION_BLUR_UPDATE_SLOT = 0x04


RECOIL_MAIN_MENU_TRANSITION_BLUR_LOOP_UPDATE_CALL = "0x4152af"


RECOIL_MAIN_MENU_TRANSITION_BLUR_END_CALL = "0x4152be"


RECOIL_MAIN_MENU_TRANSITION_BLUR_END_SLOT = 0x0C


RECOIL_MAIN_MENU_TRANSITION_RETAIL_CALL_ORDER = (
    0x41524E, 0x415256, 0x415260, 0x415274,
    0x41528D, 0x41529C, 0x4152AF, 0x4152BE,
    0x4152D1, 0x4152DB, 0x4152E5, 0x4152EF,
    0x41530D, 0x415327, 0x41532A, 0x41533D,
)


RECOIL_MAIN_MENU_TRANSITION_CANDIDATE_CALL_ORDER = (
    0x2E, 0x36, 0x40, 0x54, 0x71, 0x7E, 0x8F, 0x9C, 0xAD,
    0xB2, 0xBC, 0xC6, 0xD0, 0xEE, 0x108, 0x10B, 0x11E,
)


RECOIL_MAIN_MENU_TRANSITION_RETAIL_CODE = bytes.fromhex(
    "558bec83e4f864a1000000006aff6863944c0050a1e8bb56006489250000000083ec3485c0568bf1740a33d233c9ff15fcbb560033c9e8651f090033c9894608e8fb740a008b460c85c074656a016a048d4c2410e8d7e504008b4424086a006a008d4c2410c744244800000000ff50088b5424086a006a008d4c2410ff520485c074138b4424086a006a008d4c2410ff500485c075ed8b5424088d4c2408ff520cc7442440ffffffffc744240850ee4c00e81aad08008bc8894614e820b20800b9d8a34d00e876b5080068acb30000e882080b0083c4048944240485c0c744244001000000740d8b4e0c518bc8e8aef8ffffeb0233c08946048b106a018bc8c7442444ffffffffff5204e8f12effff85c0740fba05000000b902000000e89ed208008b4c24385e8be5c70550dc4e0000000000b80100000064890d000000005dc3"
)


RECOIL_MAIN_MENU_TRANSITION_CANDIDATE_CODE = bytes.fromhex(
    "558bec83e4f864a1000000006aff680000000050a1000000006489250000000083ec3485c0568bf1740a33d233c9ff150000000033c9e80000000033c9894608e8000000008b460c85c074666a016a048d4c2410e800000000c7442408000000006a006a008d4c2410c744244800000000e8000000006a006a008d4c2410e80000000085c074116a006a008d4c2410e80000000085c075ef8d4c2408e8000000008d4c2408c7442440ffffffffe800000000e8000000008bc8894614e800000000b900000000e80000000068acb30000e80000000083c4048944240485c0c744244001000000740d8b4e0c518bc8e800000000eb0233c08946048b106a018bc8c7442444ffffffffff5204e80000000085c0740fba05000000b902000000e8000000008b4c24385e8be5c7050000000000000000b80100000064890d000000005dc39090909090909090909090909090"
)


_VC5_COMPLETE_DESTRUCTOR_IDENTITY_RE = re.compile(
    r"^\?\?1(?P<scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)@@(?:QAE|UAE)@XZ$"
)


_VC5_COMPLETE_CONSTRUCTOR_IDENTITY_RE = re.compile(
    r"^\?\?0(?P<scope>[A-Za-z_][A-Za-z0-9_]*"
    r"(?:@[A-Za-z_][A-Za-z0-9_]*)*)@@QAE@XZ$"
)


_REVIEWED_VC5_COMPLETE_CONSTRUCTOR_IDENTITY_NAMES = {
    "??0HudUiElement@@QAE@HH@Z": "HudUiElement::Constructor",
}


APPFRAME_RUN_V9_CALLER_IDENTITY = "symbol:recoil:function:0x442d00"


APPFRAME_RUN_V9_SYMBOL = "?Run@RecoilApp_MfcOleModule@@UAEHXZ"


APPFRAME_RUN_V9_START = "0x442d00"


APPFRAME_RUN_V9_END = "0x44300b"


APPFRAME_RUN_V17_LOCAL_SYMBOL_RE = re.compile(r"\$L[0-9]+\Z")


APPFRAME_RUN_V13_CATCH_SPECS = (
    (
        0x4C, 0x54, "??_R0PAVCMemoryException@@@8", "0x44300b",
    ),
    (
        0x5C, 0x64, "??_R0PAVCFileException@@@8", "0x443032",
    ),
    (
        0x6C, 0x74, "??_R0PAVCException@@@8", "0x4430cc",
    ),
)


APPFRAME_RUN_V15_DEFINED_RTTI_DATA = {
    "??_R0PAVCMemoryException@@@8": bytes.fromhex(
        "00 00 00 00 00 00 00 00 2e 50 41 56 43 4d 65 6d "
        "6f 72 79 45 78 63 65 70 74 69 6f 6e 40 40 00"
    ),
    "??_R0PAVCFileException@@@8": bytes.fromhex(
        "00 00 00 00 00 00 00 00 2e 50 41 56 43 46 69 6c "
        "65 45 78 63 65 70 74 69 6f 6e 40 40 00"
    ),
    "??_R0PAVCException@@@8": bytes.fromhex(
        "00 00 00 00 00 00 00 00 2e 50 41 56 43 45 78 63 "
        "65 70 74 69 6f 6e 40 40 00"
    ),
}


APPFRAME_RUN_V15_RTTI_SECTION_CHARACTERISTICS = 0xC0401040


APPFRAME_RUN_V15_TYPE_INFO_VFTABLE = "??_7type_info@@6B@"


APPFRAME_RUN_V9_XDATA = bytes.fromhex(
    "20 05 93 19 02 00 00 00 00 00 00 00 01 00 00 00 "
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
    "ff ff ff ff 00 00 00 00 ff ff ff ff 00 00 00 00 "
    "00 00 00 00 00 00 00 00 01 00 00 00 03 00 00 00 "
    "00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 "
    "e8 ff ff ff 00 00 00 00 00 00 00 00 00 00 00 00 "
    "ec ff ff ff 00 00 00 00 00 00 00 00 00 00 00 00 "
    "e4 ff ff ff 00 00 00 00"
)


# Retail HudUiTextStack4::PushLine walks inline HudUiPanel rows backwards
# by 0x2a4; the other panel vectors use their distinct 0x2ac/0x2c0 records.
BOUNDED_TARGETLESS_VPTR_STRIDES = frozenset({4, 0x4C, 0x2A4, 0x2AC, 0x2C0})


BOUNDED_TARGETLESS_VPTR_SLOTS = frozenset(
    {
        0x00,
        0x0C,
        0x18,
        0x20,
        0x24,
        0x40,
        0x60,
        0x68,
        0x74,
        0x7C,
        0x80,
        0x88,
        0x8C,
        0x94,
        0xA0,
        0xD0,
        0xE0,
    }
)


_BOUNDED_TARGETLESS_ARITHMETIC_LIMIT = 0x10000


_ZSND_PLAY_ACQUIRE_HANDLE_SYMBOL = (
    "?AcquirePlayHandleDispatch@zSndSample@@QAEPAUzSndPlayHandle@@XZ"
)


_ZSND_PLAY_ACQUIRE_HANDLE_IDENTITY = "symbol:recoil:function:0x49f6d0"


_ZSND_PLAY_SELECTED_BACKEND_VPTR = (
    "load(load(runtime-object-join("
    "call-result(symbol:recoil:function:0x49f6d0)+0x8,this+0x4c)))"
)


_ZSND_PLAY_DIRECTSOUND_INBOUND_SELECTED_BACKEND_VPTR = (
    "load(load(runtime-object-join("
    "call-result(symbol:recoil:function:0x49f6d0)+0x8,"
    "entry-register(ecx)+0x4c)))"
)


_ZSND_PLAY_DIRECTSOUND_RETAIL_BACKEND_VPTR = (
    "dynamic:zSndPlayHandle-backendBuffer-vptr"
)


_ZSND_PLAYWITHDELTA_A3D_CANDIDATE_VPTR = (
    "load(load(entry-register(edx)+0x8))"
)


_ZSND_IS_MUTED_SYMBOL = "?IsMuted@zSnd@@YAHXZ"


_ZSND_IS_MUTED_IDENTITY = "symbol:recoil:function:0x4a07a0"


_ZSND_APPLY_MUTE_CANDIDATE_VPTR = (
    "load(load(call-result(symbol:recoil:function:0x49fff0):"
    "list-item-playHandle+0x8))"
)


_ZSND_APPLY_MUTE_RETAIL_VPTR = (
    "load(load(load(load(load(load(call-result("
    "symbol:recoil:function:0x49fff0)+0x4)))+0x8)+0x8))"
)


_ZSND_PLAY_SIMPLE_SYMBOL = "_zSndSamplePlaySimple@4"


_ZSND_PLAY_SIMPLE_IDENTITY = "symbol:recoil:function:0x49fa00"


_ZSND_REPORT_A3D_SYMBOL = "?ReportA3DError@zSnd@@YIHHPBDH@Z"


_ZSND_REPORT_A3D_IDENTITY = "symbol:recoil:function:0x4a3ef0"


_ZSND_REPORT_DIRECTSOUND_SYMBOL = (
    "?ReportDirectSoundError@zSnd@@YIHHPBDH@Z"
)


_ZSND_REPORT_DIRECTSOUND_IDENTITY = "symbol:recoil:function:0x4a4330"


_ZSND_PLAY_SOURCE_FILE_SYMBOL = "_kZSndPlaySourceFile$S33375"


_ZSND_BACKEND_DEVICE_SYMBOL = "_g_zSnd_BackendDevice"


_ZSND_BACKEND_DEVICE_STORAGE = "storage:recoil:data:0x56b2b0"


_ZSND_BACKEND_DEVICE_VPTR = (
    "load(load(storage:recoil:data:0x56b2b0))"
)


_ZSND_SNAPSHOT_CREATE_CALLER_IDENTITY = "symbol:recoil:function:0x49fff0"


_ZSND_SNAPSHOT_CREATE_CALLER_SYMBOL = (
    "?CreateFromActiveSamples@zSndPlayHandleSnapshot@@SAPAU1@XZ"
)


_ZSND_SNAPSHOT_CONSTRUCTOR_SYMBOL = (
    "??0zSndPlayHandleSnapshot@@QAE@E@Z"
)


_ZSND_SNAPSHOT_OPERATOR_NEW_SYMBOL = "??2@YAPAXI@Z"


_ZSND_SAMPLE_SET_COUNT_IDENTITY = "symbol:recoil:function:0x4a0900"


_ZSND_SNAPSHOT_APPEND_PAYLOAD_SYMBOL = (
    "?AppendPayload@zSndPlayHandleSnapshot@@"
    "QAEXABUzSndPlayHandleSnapshotPayload@@@Z"
)


_ZSND_SNAPSHOT_NEW_NODE_SYMBOL = (
    "?NewNode@zSndPlayHandleSnapshot@@"
    "QAEPAUzSndPlayHandleSnapshotItem@@PAU2@0@Z"
)


_ZSND_SNAPSHOT_NEW_NODE_IDENTITY = "symbol:recoil:function:0x4a07c0"


_ZNETWORK_DPLAY_TARGET_NAME = (
    "znetwork_znet_dplay_489d00_48c7d0_authored_order"
)


_ZNETWORK_DPLAY_TARGET_MANIFEST = (
    REPO_ROOT
    / "tools"
    / "vc5_verify_targets"
    / "znetwork_znet_dplay_489d00_48c7d0_authored_order.json"
)


_ZNETWORK_DPLAY_SOURCE_PATH = "src/GameZRecoil/zNetwork/znet_dplay.cpp"


_ZNETWORK_OPEN_SELECTED_SESSION_START = "0x48a520"


_ZNETWORK_OPEN_SELECTED_SESSION_ENVELOPE_END = "0x48a980"


_ZNETWORK_OPEN_SELECTED_SESSION_EXECUTION_END = "0x48a909"


_ZNETWORK_OPEN_SELECTED_SESSION_TERMINAL_RET = "0x48a908"


_ZNETWORK_OPEN_SELECTED_SESSION_TAIL = bytes.fromhex(
    "8d 49 00 "
    "f2 a7 48 00 10 a8 48 00 69 a8 48 00 d4 a7 48 00 "
    "2d a8 48 00 b7 a7 48 00 4b a8 48 00 9a a8 48 00 "
    "00 07 07 07 07 07 07 07 07 07 01 07 07 07 07 07 "
    "07 07 07 07 02 07 07 07 07 07 07 07 07 07 03 07 "
    "07 07 07 07 07 07 07 07 04 07 07 07 07 07 07 07 "
    "07 07 05 07 07 07 07 07 07 07 07 07 07 07 07 07 "
    "07 07 07 07 07 07 06 "
    "90 90 90 90 90 90 90 90 90 90 90 90 90"
)


_ZNETWORK_FATAL_CALLBACK_CALLER_IDENTITY = (
    "symbol:recoil:function:0x48afe0"
)


_ZNETWORK_FATAL_CALLBACK_CALLER_END = "0x48b3a0"


_ZNETWORK_FATAL_CALLBACK_CALLER_SYMBOL = (
    "?PumpIncomingMessages@zNetworkDPlay@@YIHPAU"
    "zNetworkDPlaySystemMessage@@@Z"
)


_ZNETWORK_FATAL_CALLBACK_OBJECT_SYMBOL = (
    "_g_zNetwork_FatalDisconnectCallback"
)


_ZNETWORK_FATAL_CALLBACK_STORAGE_IDENTITY = (
    "storage:recoil:data:0x56aafc"
)


_ZNETWORK_DISPATCH_CALLER_IDENTITY = "symbol:recoil:function:0x48c200"


_ZNETWORK_DISPATCH_CALLER_END = "0x48c250"


_ZNETWORK_DISPATCH_CALLER_SYMBOL = (
    "?DispatchPacketToHandlers@zNetwork_DPlay@@YIXH"
    "PAUzNetworkPacketHeader@@@Z"
)


_ZNETWORK_DISPATCH_LIST_OBJECT_SYMBOL = (
    "_g_zNetwork_DispatchHandlerList"
)


_ZNETWORK_DISPATCH_CANDIDATE_BODY = bytes.fromhex(
    "53 8b 1d 04 00 00 00 55 56 8b 33 33 c0 3b f3 57 "
    "0f 95 c0 84 c0 8b fa 8b e9 74 1f 8b 46 08 66 8b "
    "08 66 3b 0f 75 07 8b d7 8b cd ff 50 04 8b 36 33 "
    "c0 3b f3 0f 95 c0 84 c0 75 e1 5f 5e 5d 5b c3 90"
)


_ZNETWORK_PUMP_SWITCH_CLASSIFIER = bytes(
    {
        0: 0,
        208: 1,
        209: 2,
        210: 3,
        211: 4,
        220: 5,
    }.get(index, 6)
    for index in range(221)
)


_ZNETWORK_PUMP_SWITCH_LABELS = (
    ("$L27805", 0x1D4),
    ("$L27807", 0x1F7),
    ("$L27808", 0x20B),
    ("$L27809", 0x214),
    ("$L27810", 0x240),
    ("$L27812", 0x25D),
    ("$L27814", 0x25),
)


_ZINTERP_LOGF_CALLER_IDENTITY = "symbol:recoil:function:0x4c1b30"


_ZINTERP_LOGF_CALLER_START = "0x4c1b30"


_ZINTERP_LOGF_CALLER_END_EXCLUSIVE = "0x4c1b50"


_ZINTERP_LOGF_CANDIDATE_SYMBOL = "?Logf@CZInterp@@SAXPAU1@PBDZZ"


_ZINTERP_LOGF_FASTCALL_TAIL_BODY = bytes.fromhex(
    "8b 44 24 04 8b 40 70 85 c0 74 0a 8b 4c 24 08 "
    "8d 54 24 0c ff e0 c3"
)


_ZINTERP_LOGF_CDECL_CALL_BODY = bytes.fromhex(
    "8b 44 24 04 8b 40 70 85 c0 74 0f 8b 54 24 08 "
    "8d 4c 24 0c 51 52 ff d0 83 c4 08 c3"
)


_ZINTERP_CONTEXT_CALLBACK_IDENTITY = (
    "dynamic:zInterp_Context+0x70-callback"
)


_ZINTERP_REPORT_ERRORF_CALLER_IDENTITY = "symbol:recoil:function:0x4c5520"


_ZINTERP_REPORT_ERRORF_CALLER_START = "0x4c5520"


_ZINTERP_REPORT_ERRORF_CALLER_END_EXCLUSIVE = "0x4c5550"


_ZINTERP_REPORT_ERRORF_CANDIDATE_SYMBOL = (
    "?ReportErrorf@CZInterp@@SAXPAU1@PBDZZ"
)


_ZINTERP_REPORT_ERRORF_SEMANTIC_BODY = bytes.fromhex(
    "8b 44 24 04 c7 40 10 01 00 00 00 8b 40 70 85 c0 "
    "74 0f 8b 54 24 08 8d 4c 24 0c 51 52 ff d0 83 c4 08 c3"
)


_ZINTERP_REPORT_ERRORF_PHYSICAL_BODY = (
    _ZINTERP_REPORT_ERRORF_SEMANTIC_BODY + b"\x90" * 14
)


_ZCOM_INTERFACE_MAP_CALLER_START = "0x42db50"


_ZCOM_INTERFACE_MAP_CALLER_END_EXCLUSIVE = "0x42dc30"


_ZCOM_INTERFACE_MAP_CALLER_IDENTITY = "symbol:recoil:function:0x42db50"


_ZCOM_INTERFACE_MAP_CALLER_SYMBOL = (
    "?QueryInterfaceFromInterfaceMap@zCom@@YGJPAXPBUInterfaceMapEntry@1@"
    "PBU_GUID@@PAPAX@Z"
)


_ZCOM_INTERFACE_MAP_RESOLVER_STORAGE = (
    "dynamic:zCom::InterfaceMapEntry.resolver+0x8"
)


_ZCOM_INTERFACE_MAP_RESOLVER_RETAIL_CALL = "0x42dbe2"


_ZCOM_INTERFACE_MAP_RESOLVER_CANDIDATE_CALL = "0x92"


_ZCOM_INTERFACE_MAP_ADDREF_STORAGE = (
    "dynamic:zCom::InterfaceMapEntry.adjusted-interface-vptr"
)


_ZCOM_INTERFACE_MAP_ADDREF_CANDIDATE_CALL = "0xc0"


_ZCOM_INTERFACE_MAP_BODY = bytes.fromhex(
    "53 55 8b 6c 24 18 56 85 ed 57 75 0c b8 03 40 00 "
    "80 5f 5e 5d 5b c2 10 00 8b 7c 24 1c c7 45 00 00 "
    "00 00 00 83 3f 00 75 22 8b 47 04 85 c0 75 1b 81 "
    "7f 08 c0 00 00 00 75 12 81 7f 0c 00 00 00 46 75 "
    "09 8b 44 24 18 8b 70 04 eb 6f 8b 74 24 18 8b 4e "
    "08 85 c9 74 7a 8b 06 33 db 85 c0 0f 94 c3 85 db "
    "75 20 8b 10 3b 17 75 38 8b 50 04 3b 57 04 75 30 "
    "8b 50 08 3b 57 08 75 28 8b 40 0c 8b 57 0c 3b c2 "
    "75 1e 83 f9 01 74 2f 8b 56 04 8b 44 24 14 52 55 "
    "57 50 ff d1 85 c0 74 3c 85 db 75 04 85 c0 7c 34 "
    "8b 4e 14 83 c6 0c 85 c9 75 ab b8 02 40 00 80 5f "
    "5e 5d 5b c2 10 00 8b 76 04 03 74 24 14 56 8b 0e "
    "ff 51 04 89 75 00 33 c0 5f 5e 5d 5b c2 10 00 b8 "
    "02 40 00 80 5f 5e 5d 5b c2 10 00 90 90 90 90 90"
)


_ZCOM_INTERFACE_MAP_INSTRUCTION_BODY = _ZCOM_INTERFACE_MAP_BODY[:-5]


_R4564_CANDIDATE_CALLBACK_STORAGE_NAMES: Mapping[str, tuple[str, ...]] = {
    "0x48d7a0": ("g_pfnOverlayBlendRow",),
    "0x492000": ("g_pfnBuildSpanList",),
    "0x4927d0": ("g_pfnBuildSpanList",),
    "0x492f00": ("g_pfnBuildSpanListSecondary",),
    "0x4936d0": ("g_pfnSelectedSpanOp",),
    "0x493df0": ("g_pfnBuildSpanListSecondary",),
    "0x494af0": ("g_pfnBuildSpanList", "g_pfnBuildSpanListSecondary"),
    "0x495850": ("g_pfnBuildSpanList", "g_pfnTexturedQueuedFinalize"),
    "0x4969d0": ("g_pfnBuildSpanList",),
    "0x497ac0": ("g_pfnBuildSpanListSecondary",),
    "0x498bd0": ("g_pfnImmediateRaster4",),
    "0x498c00": ("g_pfnImmediateRaster5",),
    "0x498f90": ("g_pfnPointOpActive",),
    "0x499020": ("g_pfnPointOpActive",),
}


_R4564_CANDIDATE_DYNAMIC_SELECTION_SOURCES: Mapping[
    str,
    tuple[
        tuple[int, ...],
        str,
        tuple[str, ...],
        tuple[tuple[str, str], ...],
    ],
] = {
    "0x493df0": (
        (2,),
        "dynamic:RndrSpanLenShiftFn-selection:6320c8-6320d4",
        (
            "g_pfnFlatQueuedSpanOp_Mode0",
            "g_pfnFlatQueuedSpanOpAlt_Mode0",
            "g_pfnFlatQueuedSpanOp_Mode1",
            "g_pfnFlatQueuedSpanOpAlt_Mode1",
        ),
        (),
    ),
    "0x494af0": (
        (2,),
        "dynamic:RndrSpanLenShiftFn-selection:6320e0-6320ec",
        (
            "g_pfnPolyTlvSpanOp_Mode0",
            "g_pfnPolyTlvSpanOpAlt_Mode0",
            "g_pfnPolyTlvSpanOp_Mode1",
            "g_pfnPolyTlvSpanOpAlt_Mode1",
        ),
        (),
    ),
    "0x495850": (
        (4, 6, 7, 9),
        "dynamic:RndrSpanLenShiftFn-selection:6320b8-6320bc-or-49f180",
        (
            "g_pfnTexturedQueuedSpanOp_Mode0",
            "g_pfnTexturedQueuedSpanOp_Mode1",
        ),
        (("SpanShade16FromPal8SwitchVShift", "0x49f180"),),
    ),
    "0x4969d0": (
        (3, 4),
        "dynamic:RndrSpanLenShiftFn-selection:6320b8-6320bc",
        (
            "g_pfnTexturedQueuedSpanOp_Mode0",
            "g_pfnTexturedQueuedSpanOp_Mode1",
        ),
        (),
    ),
    "0x497ac0": (
        (3, 4),
        "dynamic:RndrSpanLenShiftFn-selection:6320d8-6320dc",
        (
            "g_pfnTexturedFanTriSpanOp_Mode0",
            "g_pfnTexturedFanTriSpanOp_Mode1",
        ),
        (),
    ),
}


_R4564_CANDIDATE_DYNAMIC_SELECTION_SOURCE_COUNTS: Mapping[
    str,
    Mapping[str, int],
] = {
    "0x4969d0": {
        "g_pfnTexturedQueuedSpanOp_Mode1": 2,
    },
    "0x497ac0": {
        "g_pfnTexturedFanTriSpanOp_Mode1": 2,
    },
}


# Retail-proven provider argument obligations. These select what to inspect;
# expected values and provenance are derived from the live retail body below.
# Blt has six stdcall arguments including its interface pointer; flags is #4.
_PROVIDER_ARGUMENT_OBLIGATIONS = {"0x4a69e0": {5: (4,)}}


# The static initializer's call is not sufficient without the concrete object's
# dispatch dependency. This is a selection rule, not saved expected table data.
_CONSTRUCTOR_DISPATCH_OBLIGATIONS = {
    # Complete table extent, but only these crash-critical target identities
    # are selected here. Other inherited/folded cells are not silently accepted
    # as logical aliases by this new dependency proof.
    "0x435a40": ("0x435c80", "??0RecoilStateSaveLoadTransition@@QAE@XZ", 10, (0, 1, 3, 6)),
}


_COD_SWITCH_INDEX_CODES = {
    "eax": 0,
    "ecx": 1,
    "edx": 2,
    "ebx": 3,
    "ebp": 5,
    "esi": 6,
    "edi": 7,
}


ZINPUT_KEYBOARD_SWITCH_CLASSIFIER = bytes(
    {
        0: 0,
        13: 1,
        25: 2,
        27: 3,
        128: 4,
        155: 5,
    }.get(index, 6)
    for index in range(156)
)


_VC5_TU_LOCAL_CALLABLE_RE = re.compile(
    r"(?P<head>\?.+?@\?%)"
    r"(?P<source>[^@]+\.(?:c|cc|cpp|cxx))"
    r"(?P<discriminator>[0-9]+)@@"
    r"(?P<suffix>.+Z)",
    re.IGNORECASE,
)


_ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_IDENTITY = (
    "symbol:recoil:function:0x473e60"
)


_ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_START = "0x473e60"


_ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_END = "0x473fc0"


_ZMATH_CAMERA_STAGE_INVERSE_ROTATION_CALLER_SYMBOL = (
    "?zMathCameraStageInverseRotation@@YIXPBUzMat4x3@@@Z"
)


_ZMATH_CAMERA_STAGE_INVERSE_ROTATION_TARGET_ID = (
    "recoil:vc5-target:zmath_zmth_main_472670_475c40_authored_order"
)


_ZMATH_CAMERA_STAGE_INVERSE_ROTATION_TARGET_NAME = (
    "zmath_zmth_main_472670_475c40_authored_order"
)


_ZMATH_CAMERA_STAGE_INVERSE_ROTATION_MANIFEST = (
    "tools/vc5_verify_targets/"
    "zmath_zmth_main_472670_475c40_authored_order.json"
)


_ZMATH_CAMERA_STAGE_INVERSE_ROTATION_SOURCE = (
    "src/GameZRecoil/zMath/zmth_main.c"
)


_ZMATH_CAMERA_NEGATE_CALL_OFFSETS = (
    0x19,
    0x2B,
    0x3D,
    0x4E,
    0x60,
    0x72,
    0xFB,
    0x10D,
    0x11E,
)


_VC5_NAMESPACE_PRIMITIVE_TYPES: dict[str, tuple[str, int, int]] = {
    # logical BN type width and x86 argument-stack slot width stay distinct.
    "D": ("char", 1, 4),
    "E": ("unsigned char", 1, 4),
    "F": ("short", 2, 4),
    "G": ("unsigned short", 2, 4),
    "H": ("int", 4, 4),
    "I": ("unsigned int", 4, 4),
    "J": ("long", 4, 4),
    "K": ("unsigned long", 4, 4),
    "M": ("float", 4, 4),
    "N": ("double", 8, 8),
    "O": ("long double", 8, 8),
}


_R4575_ZRENDER_STATIC_COMPARISON_ONLY_CALLABLES = frozenset({
    "zVideoFxPass3ClampCurrentRadius",
    "FxLineOutCode",
    "zRndrSpanDepthAtXByPartsLocal",
    "BlendLensFlarePixel",
    "CommitFogParamsIfChanged",
    "SpanTex16SampleIndex",
    "FogBlendPixel565",
    "FogBlendPixel555",
    "BlendPixel565Alpha8",
    "BlendPixel555Alpha8",
    "BlendPixel555ConstAlphaMap",
    "FogBlendPair565",
    "FogBlendPair555",
    "zVideoFxPass3ApproxRadiusIndex",
    "zVideoFxPass3ScatterDirectSymmetric",
    "zVideoFxPass3ScatterClippedSymmetric",
    "zVideoFxPass3CopyDirect",
    "zVideoFxPass3CopyScratchToSurface",
    "TruncateFloat",
})


_ZSND_STATIC_COORDINATOR_CALLER_IDENTITY = "symbol:recoil:function:0x4a0800"


_ZSND_STATIC_VECTOR_CTOR_IDENTITY = "symbol:recoil:function:0x4a0810"


_ZSND_STATIC_COORDINATOR_COFF = "_$E6"


_ZSND_STATIC_VECTOR_CTOR_COFF = "_$E3"


_ZSND_STATIC_ATEXIT_COFF = "_$E5"


_ZSND_VECTOR_CONSTRUCTOR_COFF = (
    "??0?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAE@ABV?$allocator@PAUzSndSampleSet@@@1@@Z"
)


_ZSND_VECTOR_DESTRUCTOR_COFF = (
    "??1?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAE@XZ"
)


_ZSND_VECTOR_DESTROY_COFF = (
    "?_Destroy@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@"
    "@std@@@std@@IAEXPAPAUzSndSampleSet@@0@Z"
)


_ZSND_VECTOR_DEALLOCATE_COFF = (
    "?deallocate@?$allocator@PAUzSndSampleSet@@@std@@QAEXPAXI@Z"
)


_ZSND_VECTOR_BEGIN_COFF = (
    "?begin@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAEPAPAUzSndSampleSet@@XZ"
)


_ZSND_VECTOR_END_COFF = (
    "?end@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAEPAPAUzSndSampleSet@@XZ"
)


_ZSND_VECTOR_CLEAR_COFF = (
    "?clear@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAEXXZ"
)


_ZSND_VECTOR_ERASE_COFF = (
    "?erase@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAEPAPAUzSndSampleSet@@PAPAU3@0@Z"
)


_ZSND_VECTOR_SIZE_COFF = (
    "?size@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QBEIXZ"
)


_ZSND_VECTOR_INDEX_COFF = (
    "??A?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAEAAPAUzSndSampleSet@@I@Z"
)


_ZSND_VECTOR_PUSHBACK_COFF = (
    "?push_back@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@"
    "@std@@@std@@QAEXABQAUzSndSampleSet@@@Z"
)


_ZSND_VECTOR_ITERATOR_INSERT_COFF = (
    "?insert@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAEPAPAUzSndSampleSet@@PAPAU3@ABQAU3@@Z"
)


_ZSND_VECTOR_CORE_INSERT_COFF = (
    "?insert@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@"
    "@std@@QAEXPAPAUzSndSampleSet@@IABQAU3@@Z"
)


_ZSND_VECTOR_PUSHBACK_GRAPH_STRUCTURES = {
    _ZSND_VECTOR_PUSHBACK_COFF: (
        "8b442404568bf150e800000000508bcee8000000005ec2040090909090909090",
        ((9, _ZSND_VECTOR_END_COFF), (17, _ZSND_VECTOR_ITERATOR_INSERT_COFF)),
    ),
    _ZSND_VECTOR_END_COFF: (
        "8b4108c3909090909090909090909090", ()
    ),
    _ZSND_VECTOR_ITERATOR_INSERT_COFF: (
        "56578bf9e8000000008b5424108bc88b44240c528bf06a012bf1508bcfc1fe02"
        "e8000000008bcfe8000000008d04b05f5ec20800909090909090909090909090",
        (
            (5, _ZSND_VECTOR_BEGIN_COFF),
            (33, _ZSND_VECTOR_CORE_INSERT_COFF),
            (40, _ZSND_VECTOR_BEGIN_COFF),
        ),
    ),
    _ZSND_VECTOR_BEGIN_COFF: (
        "8b4104c3909090909090909090909090", ()
    ),
    _ZSND_VECTOR_CORE_INSERT_COFF: (
        "5355568bf1578b7c24188b46088b4e0c2bc8c1f9023bcf0f83ae0000008bce"
        "e8000000003bf8730b8bcee8000000008be8eb028bef8bcee8000000008bd86a"
        "0003dd8bce53e8000000008b5424148be88b46045552508bcee8000000008b4c"
        "241c894424185157508bcee8000000008b5424188b4e088d04ba8b5424145051"
        "528bcee8000000008b46088b4e0450518bcee8000000008b46048b560c2bd08b"
        "cec1fa025250e8000000008d449d008bce89460ce80000000003c7896e048d4c"
        "8500894e085f5e5d5bc20c008b5c24148bd02bd3c1fa023bd773478d2cbd0000"
        "00008d0c2b5150538bcee8000000008b46088b54241c8bc8522bcbc1f9022bf9"
        "8bce5750e8000000008b54241c8bcb528b5608e800000000016e085f5e5d5bc2"
        "0c0085ff762ec1e70250502bc78bce50e8000000008b56088bcb522bd7e80000"
        "00008b44241c8d143b508bcbe800000000017e085f5e5d5bc20c00909090909090",
        (
            (32, _ZSND_VECTOR_SIZE_COFF),
            (43, _ZSND_VECTOR_SIZE_COFF),
            (56, _ZSND_VECTOR_SIZE_COFF),
            (70, "?allocate@?$allocator@PAUzSndSampleSet@@@std@@QAEPAPAUzSndSampleSet@@IPBX@Z"),
            (89, "?_Ucopy@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@@std@@IAEPAPAUzSndSampleSet@@PBQAU3@0PAPAU3@@Z"),
            (107, "?_Ufill@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@@std@@IAEXPAPAUzSndSampleSet@@IABQAU3@@Z"),
            (131, "?_Ucopy@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@@std@@IAEPAPAUzSndSampleSet@@PBQAU3@0PAPAU3@@Z"),
            (146, _ZSND_VECTOR_DESTROY_COFF),
            (166, _ZSND_VECTOR_DEALLOCATE_COFF),
            (180, _ZSND_VECTOR_SIZE_COFF),
            (234, "?_Ucopy@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@@std@@IAEPAPAUzSndSampleSet@@PBQAU3@0PAPAU3@@Z"),
            (260, "?_Ufill@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@@std@@IAEXPAPAUzSndSampleSet@@IABQAU3@@Z"),
            (275, "?fill@std@@YIXPAPAUzSndSampleSet@@0ABQAU2@@Z"),
            (304, "?_Ucopy@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@@std@@IAEPAPAUzSndSampleSet@@PBQAU3@0PAPAU3@@Z"),
            (317, "?copy_backward@std@@YIPAPAUzSndSampleSet@@PAPAU2@00@Z"),
            (332, "?fill@std@@YIXPAPAUzSndSampleSet@@0ABQAU2@@Z"),
        ),
    ),
    _ZSND_VECTOR_SIZE_COFF: (
        "8b510485d2750333c0c38b41082bc2c1f802c390909090909090909090909090", ()
    ),
    "?allocate@?$allocator@PAUzSndSampleSet@@@std@@QAEPAPAUzSndSampleSet@@IPBX@Z": (
        "8b4c240433d2e800000000c208009090",
        ((7, "?_Allocate@std@@YIPAPAUzSndSampleSet@@HPAPAU2@@Z"),),
    ),
    "?_Allocate@std@@YIPAPAUzSndSampleSet@@HPAPAU2@@Z": (
        "85c97d0233c98d048d0000000050e80000000083c404c3909090909090909090",
        ((15, "??2@YAPAXI@Z"),),
    ),
    "?_Ucopy@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@@std@@IAEPAPAUzSndSampleSet@@PBQAU3@0PAPAU3@@Z": (
        "53558b6c2410568b742410578b7c241c3bf58bd9741356578bcbe80000000083"
        "c60483c7043bf575ed8bc75f5e5d5bc20c009090909090909090909090909090",
        ((27, "?construct@?$allocator@PAUzSndSampleSet@@@std@@QAEXPAPAUzSndSampleSet@@ABQAU3@@Z"),),
    ),
    "?_Ufill@?$vector@PAUzSndSampleSet@@V?$allocator@PAUzSndSampleSet@@@std@@@std@@IAEXPAPAUzSndSampleSet@@IABQAU3@@Z": (
        "55578b7c24108be985ff761b568b742410538b5c241c53568bcde80000000083"
        "c6044f75f15b5e5f5dc20c0090909090",
        ((27, "?construct@?$allocator@PAUzSndSampleSet@@@std@@QAEXPAPAUzSndSampleSet@@ABQAU3@@Z"),),
    ),
    "?construct@?$allocator@PAUzSndSampleSet@@@std@@QAEXPAPAUzSndSampleSet@@ABQAU3@@Z": (
        "8b5424088b4c2404e800000000c20800",
        ((9, "?_Construct@std@@YIXPAPAUzSndSampleSet@@ABQAU2@@Z"),),
    ),
    "?_Construct@std@@YIXPAPAUzSndSampleSet@@ABQAU2@@Z": (
        "56518bf26a04e80000000083c40885c074048b0e89085ec39090909090909090",
        ((7, "??2@YAPAXIPAX@Z"),),
    ),
    "??2@YAPAXIPAX@Z": (
        "8b442408c39090909090909090909090", ()
    ),
    _ZSND_VECTOR_DESTROY_COFF: (
        "538b5c240c568b74240c573bf38bf9740f568bcfe80000000083c6043bf375f1"
        "5f5e5bc2080090909090909090909090",
        ((21, "?destroy@?$allocator@PAUzSndSampleSet@@@std@@QAEXPAPAUzSndSampleSet@@@Z"),),
    ),
    "?destroy@?$allocator@PAUzSndSampleSet@@@std@@QAEXPAPAUzSndSampleSet@@@Z": (
        "8b4c2404e800000000c2040090909090",
        ((5, "?_Destroy@std@@YIXPAPAUzSndSampleSet@@@Z"),),
    ),
    "?_Destroy@std@@YIXPAPAUzSndSampleSet@@@Z": (
        "c3909090909090909090909090909090", ()
    ),
    _ZSND_VECTOR_DEALLOCATE_COFF: (
        "8b44240450e80000000083c404c20800",
        ((6, "??3@YAXPAX@Z"),),
    ),
    "?fill@std@@YIXPAPAUzSndSampleSet@@0ABQAU2@@Z": (
        "3bca74118b442404568b30893183c1043bca75f55ec204009090909090909090", ()
    ),
    "?copy_backward@std@@YIPAPAUzSndSampleSet@@PAPAU2@00@Z": (
        "3bca74188b442404568b72fc83ea0483e8043bd1893075f15ec204008b442404"
        "c2040090909090909090909090909090", ()
    ),
}


_ZNETWORK_APPEND_PROVIDER_IDENTITY = "provider:recoil:function:0x4c5b76"


_ZNETWORK_APPEND_COPY_PROVIDER_IDENTITY = (
    "provider:recoil:function:0x40c1c0"
)


_ZNETWORK_APPEND_RETAIL_PROVIDER_TAIL = (
    ("provider:recoil:function:0x4c5b76", 4),
    ("provider:recoil:function:0x40c1c0", None),
    ("provider:recoil:function:0x40c190", None),
    ("provider:recoil:function:0x48bf10", None),
    ("provider:recoil:function:0x40bdf0", None),
    ("provider:recoil:function:0x4c5b6a", 4),
    ("provider:recoil:function:0x42a9d0", None),
    ("provider:recoil:function:0x48bf10", None),
    ("provider:recoil:function:0x40c190", None),
    ("provider:recoil:function:0x48bf10", None),
)


_ZNETWORK_APPEND_RETAIL_PROVIDER_AUTHORITIES = {
    "0x4c5b76": (
        "0x4c5b7c", "non-authored",
        bytes.fromhex("ff 25 ac c2 4c 00"),
    ),
    "0x40c1c0": (
        "0x40c1d0", "non-authored",
        bytes.fromhex(
            "85 c9 74 04 8b 02 89 01 c3 90 90 90 90 90 90 90"
        ),
    ),
    "0x40c190": (
        "0x40c1c0", "non-authored",
        bytes.fromhex(
            "8b 4c 24 08 85 c9 76 18 8b 54 24 0c 8b 44 24 04 "
            "56 85 c0 74 04 8b 32 89 30 83 c0 04 49 75 f2 5e "
            "c2 0c 00 90 90 90 90 90 90 90 90 90 90 90 90 90"
        ),
    ),
    "0x48bf10": (
        "0x48bf40", "non-authored",
        bytes.fromhex(
            "8b 4c 24 04 8b 54 24 08 3b ca 74 1b 8b 44 24 0c "
            "56 85 c0 74 04 8b 31 89 30 83 c1 04 83 c0 04 3b "
            "ca 75 ee 5e c2 0c 00 8b 44 24 0c c2 0c 00 90 90"
        ),
    ),
    "0x40bdf0": (
        "0x40be00", "compiler-generated-icf-representative",
        bytes.fromhex(
            "c2 08 00 90 90 90 90 90 90 90 90 90 90 90 90 90"
        ),
    ),
    "0x4c5b6a": (
        "0x4c5b70", "non-authored",
        bytes.fromhex("ff 25 b8 c2 4c 00"),
    ),
    "0x42a9d0": (
        "0x42a9f0", "non-authored",
        bytes.fromhex(
            "8b 51 04 85 d2 75 03 33 c0 c3 8b 41 08 2b c2 c1 "
            "f8 02 c3 90 90 90 90 90 90 90 90 90 90 90 90 90"
        ),
    ),
}


_ZNETWORK_APPEND_CALLER_SYMBOL = (
    "?EnumConnectionsCallbackAddServiceProviderInfo@zNetworkDPlay@@YGH"
    "PBU_GUID@@PAXKPBUDPNAME@@K1@Z"
)


_ZNETWORK_APPEND_COFF_RE = re.compile(
    r"\?AppendServiceProviderInfo@\?%"
    r"(?P<path>[A-Za-z]:\\(?:[^\\]+\\)*src\\GameZRecoil\\zNetwork\\"
    r"znet_dplay\.cpp)"
    r"(?P<discriminator>[0-9]+)"
    r"@@YAXPAUzNetworkDPlayServiceProviderInfo@@@Z"
)


_ZNETWORK_APPEND_CALLER_BODY = bytes.fromhex(
    "53 6a 1c e8 00 00 00 00 8b d8 83 c4 04 85 db 74 "
    "64 8b 44 24 08 8b cb 57 56 8b 10 89 11 8b 50 04 "
    "89 51 04 8b 50 08 89 51 08 8b 40 0c 89 41 0c 8b "
    "4c 24 1c 8b 51 08 52 ff 15 00 00 00 00 8b 74 24 "
    "1c 83 c4 04 89 43 10 6a 01 56 ff 15 00 00 00 00 "
    "8b ce 8b 74 24 1c 89 43 14 8b f8 8b c1 83 c4 08 "
    "c1 e9 02 f3 a5 8b c8 83 e1 03 f3 a4 8b 4c 24 20 "
    "5e 89 4b 18 5f 53 e8 00 00 00 00 83 c4 04 b8 01 "
    "00 00 00 5b c2 18 00 90 90 90 90 90 90 90 90 90"
)


_ZNETWORK_APPEND_HELPER_BODY = bytes.fromhex(
    "53 8b 1d 00 00 00 00 56 8b 73 08 8b 43 0c 3b f0 "
    "75 6c 8b 43 04 57 85 c0 55 74 07 2b f0 c1 fe 02 "
    "eb 02 33 f6 83 fe 01 8d 46 01 7e 03 8d 04 36 8d "
    "2c 85 00 00 00 00 55 e8 00 00 00 00 8b f8 83 c4 "
    "04 33 c0 85 f6 7e 10 8b 4b 04 40 3b c6 8b 54 81 "
    "fc 89 54 87 fc 7c f0 8b 44 24 14 89 04 b7 8b 4b "
    "04 51 e8 00 00 00 00 83 c4 04 89 7b 04 8d 54 b7 "
    "04 03 fd 89 7b 0c 5d 89 53 08 5f 5e 5b c3 8b 44 "
    "24 0c 89 06 8b 43 08 83 c0 04 5e 89 43 08 5b c3"
)


MISSION_COMPILER_LOCAL_EH_SYMBOL_RE = re.compile(r"\$L[0-9]+\Z")


MISSION_CONNECT_PROVIDER_OLD_LOAD_WINDOW = bytes.fromhex(
    "8b3d000000008b742410"
)


MISSION_CONNECT_PROVIDER_CURRENT_LOAD_WINDOW = bytes.fromhex(
    "8b7424108b3d00000000"
)


MISSION_CONNECT_PROVIDER_CURRENT_BODY = bytes.fromhex(
    "81ec04020000538bd955568b4320576a0250ff15000000008bab900100008b3d000000006a006a00684701000055ffd7"
    "83f8ff0f84d70100006a0050685001000055ffd78bf085f6897424100f84b00000008b4e10680000000051ff15000000"
    "0083c40885c00f84c5000000a10000000085c00f85b8000000b912000000c7050000000001000000e8000000008bf8"
    "83c9ff33c08d942414010000f2aef7d12bf98bc18bf78bfac1e902f3a58bc883e103f3a4b926000000e8000000008bf8"
    "83c9ff33c08d542414f2aef7d12bf98bc18bf78bfa8d542414c1e902f3a58bc883e103f3a48d8c2414010000e8000000"
    "0085c075335050684e01000055ff15000000006a008d8bb0000000e8000000006a008d8bf0000000e8000000005f5e5d"
    "5b81c404020000c38b7424108b3d000000008bcee8000000008b4e10680000000051ff150000000083c40885c075678b"
    "cbe80000000085c07c138b53206a0068e80300006a0252ff15000000008db3f00000006a018bcee800000000b9370000"
    "00e800000000508d8bb0000000e800000000b938000000e800000000508bcee800000000c7436c000000005f5e5d5b81"
    "c404020000c38b83500100006a006a00688401000050ffd78db3b00000006a018bcee800000000b935000000e8000000"
    "00508bcee800000000b9360000008db3f0000000e800000000508bcee8000000006a018bcee800000000c7436c010000"
    "005f5e5d5b81c404020000c39090909090"
)


MISSION_CONNECT_PROVIDER_CURRENT_RELOCATIONS = (
    (0x014, IMAGE_REL_I386_DIR32, "__imp__KillTimer@8"),
    (0x020, IMAGE_REL_I386_DIR32, "__imp__SendMessageA@16"),
    (0x056, IMAGE_REL_I386_DIR32, "_g_zNetwork_ProviderName_TcpIp"),
    (0x05D, IMAGE_REL_I386_DIR32, "__imp__strstr"),
    (0x06D, IMAGE_REL_I386_DIR32, "_g_NetUiTcpIpProviderWarningShown"),
    (0x080, IMAGE_REL_I386_DIR32, "_g_NetUiTcpIpProviderWarningShown"),
    (0x089, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
    (0x0B9, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
    (0x0EC, IMAGE_REL_I386_REL32, "?VerifyWinsock2OrPromptContinue@NetUi@@YIHPBD0@Z"),
    (0x0FE, IMAGE_REL_I386_DIR32, "__imp__SendMessageA@16"),
    (0x10B, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
    (0x118, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
    (0x12D, IMAGE_REL_I386_DIR32, "__imp__SendMessageA@16"),
    (0x134, IMAGE_REL_I386_REL32, "?SelectServiceProviderAndInitConnection@zNetworkDPlay@@YIHPAUzNetworkDPlayServiceProviderInfo@@@Z"),
    (0x13C, IMAGE_REL_I386_DIR32, "_g_zNetwork_ProviderName_Modem"),
    (0x143, IMAGE_REL_I386_DIR32, "__imp__strstr"),
    (0x151, IMAGE_REL_I386_REL32, "?RefreshSessionList@NetSessionBrowserDialog@@QAEHXZ"),
    (0x168, IMAGE_REL_I386_DIR32, "__imp__SetTimer@16"),
    (0x177, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
    (0x181, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
    (0x18D, IMAGE_REL_I386_REL32, "?SetWindowTextA@CWnd@@QAEXPBD@Z"),
    (0x197, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
    (0x19F, IMAGE_REL_I386_REL32, "?SetWindowTextA@CWnd@@QAEXPBD@Z"),
    (0x1D2, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
    (0x1DC, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
    (0x1E4, IMAGE_REL_I386_REL32, "?SetWindowTextA@CWnd@@QAEXPBD@Z"),
    (0x1F4, IMAGE_REL_I386_REL32, "?GetMessageString@zLoc@@YIPADI@Z"),
    (0x1FC, IMAGE_REL_I386_REL32, "?SetWindowTextA@CWnd@@QAEXPBD@Z"),
    (0x205, IMAGE_REL_I386_REL32, "?EnableWindow@CWnd@@QAEHH@Z"),
)


MISSION_CONNECT_PROVIDER_CURRENT_CALL_OFFSETS = (
    0x012, 0x02E, 0x042, 0x05B, 0x088, 0x0B8, 0x0EB, 0x0FC,
    0x10A, 0x117, 0x133, 0x141, 0x150, 0x166, 0x176, 0x180,
    0x18C, 0x196, 0x19E, 0x1C5, 0x1D1, 0x1DB, 0x1E3, 0x1F3,
    0x1FB, 0x204,
)


MISSION_CONNECT_PROVIDER_CURRENT_COD_ROWS = (
    (None, "sub"), (0x006, "push"), (0x007, "mov"),
    (0x009, "push"), (0x00A, "push"), (0x00B, "mov"),
    (0x00E, "push"), (0x00F, "push"), (0x011, "push"),
    (0x012, "call"), (0x018, "mov"), (0x01E, "mov"),
    (0x024, "push"), (0x026, "push"), (0x028, "push"),
    (0x02D, "push"), (0x02E, "call"), (0x030, "cmp"),
    (0x033, "je"), (0x039, "push"), (0x03B, "push"),
    (0x03C, "push"), (0x041, "push"), (0x042, "call"),
    (0x044, "mov"), (0x046, "test"), (0x048, "mov"),
    (0x04C, "je"), (0x052, "mov"), (0x055, "push"),
    (0x05A, "push"), (0x05B, "call"), (0x061, "add"),
    (0x064, "test"), (0x066, "je"), (0x06C, "mov"),
    (0x071, "test"), (0x073, "jne"), (0x079, "mov"),
    (0x07E, "mov"), (0x088, "call"), (0x08D, "mov"),
    (0x08F, "or"), (0x092, "xor"), (0x094, "lea"),
    (0x09B, "repne"), (0x09D, "not"), (0x09F, "sub"),
    (0x0A1, "mov"), (0x0A3, "mov"), (0x0A5, "mov"),
    (0x0A7, "shr"), (0x0AA, "rep"), (0x0AC, "mov"),
    (0x0AE, "and"), (0x0B1, "rep"), (0x0B3, "mov"),
    (0x0B8, "call"), (0x0BD, "mov"), (0x0BF, "or"),
    (0x0C2, "xor"), (0x0C4, "lea"), (0x0C8, "repne"),
    (0x0CA, "not"), (0x0CC, "sub"), (0x0CE, "mov"),
    (0x0D0, "mov"), (0x0D2, "mov"), (0x0D4, "lea"),
    (0x0D8, "shr"), (0x0DB, "rep"), (0x0DD, "mov"),
    (0x0DF, "and"), (0x0E2, "rep"), (0x0E4, "lea"),
    (0x0EB, "call"), (0x0F0, "test"), (0x0F2, "jne"),
    (0x0F4, "push"), (0x0F5, "push"), (0x0F6, "push"),
    (0x0FB, "push"), (0x0FC, "call"), (0x102, "push"),
    (0x104, "lea"), (0x10A, "call"), (0x10F, "push"),
    (0x111, "lea"), (0x117, "call"), (0x11C, "pop"),
    (0x11D, "pop"), (0x11E, "pop"), (0x11F, "pop"),
    (0x120, "add"), (0x126, "ret"), (0x127, "mov"),
    (0x12B, "mov"), (0x131, "mov"), (0x133, "call"),
    (0x138, "mov"), (0x13B, "push"), (0x140, "push"),
    (0x141, "call"), (0x147, "add"), (0x14A, "test"),
    (0x14C, "jne"), (0x14E, "mov"), (0x150, "call"),
    (0x155, "test"), (0x157, "jl"), (0x159, "mov"),
    (0x15C, "push"), (0x15E, "push"), (0x163, "push"),
    (0x165, "push"), (0x166, "call"), (0x16C, "lea"),
    (0x172, "push"), (0x174, "mov"), (0x176, "call"),
    (0x17B, "mov"), (0x180, "call"), (0x185, "push"),
    (0x186, "lea"), (0x18C, "call"), (0x191, "mov"),
    (0x196, "call"), (0x19B, "push"), (0x19C, "mov"),
    (0x19E, "call"), (0x1A3, "mov"), (0x1AA, "pop"),
    (0x1AB, "pop"), (0x1AC, "pop"), (0x1AD, "pop"),
    (0x1AE, "add"), (0x1B4, "ret"), (0x1B5, "mov"),
    (0x1BB, "push"), (0x1BD, "push"), (0x1BF, "push"),
    (0x1C4, "push"), (0x1C5, "call"), (0x1C7, "lea"),
    (0x1CD, "push"), (0x1CF, "mov"), (0x1D1, "call"),
    (0x1D6, "mov"), (0x1DB, "call"), (0x1E0, "push"),
    (0x1E1, "mov"), (0x1E3, "call"), (0x1E8, "mov"),
    (0x1ED, "lea"), (0x1F3, "call"), (0x1F8, "push"),
    (0x1F9, "mov"), (0x1FB, "call"), (0x200, "push"),
    (0x202, "mov"), (0x204, "call"), (0x209, "mov"),
    (0x210, "pop"), (0x211, "pop"), (0x212, "pop"),
    (0x213, "pop"), (0x214, "add"), (0x21A, "ret"),
)


MISSION_CONNECT_PROVIDER_IAT_FIRST_BODY = (
    MISSION_CONNECT_PROVIDER_CURRENT_BODY[:0x127]
    + MISSION_CONNECT_PROVIDER_OLD_LOAD_WINDOW
    + MISSION_CONNECT_PROVIDER_CURRENT_BODY[0x131:]
)


MISSION_CONNECT_PROVIDER_IAT_FIRST_RELOCATIONS = tuple(
    (0x129, relocation_type, symbol_name)
    if (
        offset == 0x12D
        and relocation_type == IMAGE_REL_I386_DIR32
        and symbol_name == "__imp__SendMessageA@16"
    )
    else (offset, relocation_type, symbol_name)
    for offset, relocation_type, symbol_name
    in MISSION_CONNECT_PROVIDER_CURRENT_RELOCATIONS
)


MISSION_CONNECT_PROVIDER_IAT_FIRST_COD_ROWS = tuple(
    (0x12D, mnemonic) if offset == 0x12B else (offset, mnemonic)
    for offset, mnemonic in MISSION_CONNECT_PROVIDER_CURRENT_COD_ROWS
)
