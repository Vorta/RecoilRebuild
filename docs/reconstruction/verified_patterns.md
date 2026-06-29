# Verified Patterns

This ledger captures reusable source and verification shapes that have current
tier `S` evidence. Binary Ninja, `.agent/SOURCE_OWNERS.json`, and per-target VC
artifacts remain authoritative for individual functions.

## Current Patterns

- Scalar deleting destructor wrappers: current Binary Ninja evidence classifies
  wrappers such as `0x407170` (`RecoilStateBase::ScalarDeletingDestructor`),
  `0x434980` (`HudUiSaveGameDialog::ScalarDeletingDestructor`), and `0x434dd0`
  (`HudUiLoadGameDialog::ScalarDeletingDestructor`), and `0x435ca0`
  (`RecoilStateSaveLoadTransition::ScalarDeletingDestructor`), and `0x4429b0`
  (`RecoilApp_MfcOleModule::ScalarDeletingDestructor`) as compiler-generated
  provider glue when the body only performs destructor dispatch, tests the
  delete flag, optionally calls `operator delete`, and returns `this`. Do not
  author these wrappers manually in production source; model the owning C++
  class/interface and record the wrapper as a provider boundary.
- Scalar deleting destructor wrappers with an inlined small destructor body
  follow the same source-shape rule. For example, `0x40bf50`
  (`HudCmdBindingEntry::ScalarDeletingDestructor`) is compiler glue that
  inlines the authored `HudCmdBindingEntry` display-string destructor before
  testing the delete flag. Model the destructor on the class, not as a
  hand-authored production `ScalarDeletingDestructor` method.
- Tiny vtable/no-op helpers: verified examples include `0x407130`,
  `0x407140`, `0x407150`, and `0x407160`. Keep these as simple authored C/C++
  bodies or provider-marked glue according to the owner-projection entry; do not replace
  them with raw assembly.
- HUD/UI leaf accessors and setters: verified examples include
  `HudUiElement::GetX` and nearby small HUD helpers. Prefer named fields and
  static layout checks over offset math once Binary Ninja types are stable.
- zSound snapshot and playback helpers: see the focused `zsnd_*_verification.md`
  notes for current matched call shapes, known byte-diff limits, and VC5SP3
  profile evidence.
- zSys CPU probes: the CPU feature-detection group is a documented raw-assembly
  exception through `vc5_zsys_cpu_raw_asm`; do not generalize that exception to
  new production functions without explicit approval.
- zRndr overlay MMX rows: `0x48d510` and `0x48d5f0` are a user-approved
  address-scoped raw-assembly exception through
  `vc5_o2_ob0_md_zrndr_mmx_raw_asm_facs`, because VC5SP3 exposes no usable MMX
  intrinsic surface and retail uses authored qword MMX row loops. This permits
  narrow inline `__asm` loops inside ordinary C++ functions only; it does not
  permit `__declspec(naked)`, `_emit`, `.asm`, raw byte emission, other zRndr
  span/MMX families, provider shims, or future raw assembly.
- zRndr span MMX blocks: `0x49ea80`, `0x49ec20`, `0x49e400`, `0x49e560`,
  `0x49cbb0`, `0x49cea0`, `0x49da80`, and `0x49ddb0` are a user-approved
  MMX-block-only raw-assembly exception through
  `vc5_o2_ob0_md_zrndr_span_mmx_raw_asm_facs`. C++ must retain the function
  shell, setup, scalar edge/tail behavior, scratch-buffer preparation, and portable
  fallback; inline `__asm` is limited to the BN-proven MMX blocks. This does
  not permit whole-function raw assembly, `__declspec(naked)`, `_emit`, `.asm`,
  raw byte emission, provider shims, or non-MMX blocks.
- zRndr ESP-pivot span leaves: `0x49b7e0`, `0x49bbf0`, `0x49e6c0`,
  `0x49edc0`, and `0x49f180` are a user-approved address-scoped raw-assembly
  exception through `vc5_o2_ob0_md_zrndr_esp_pivot_raw_asm_facs`, but only for
  narrow inline `__asm` loops inside ordinary C++ functions. Parent validation
  found no viable C++ codegen route for `0x49b7e0` under `/Ob0`, `/Oy`, `/Ob1`,
  or `/Ob2` profiles, and BN evidence proves real ESP-pivot stack writes. This
  exception does not permit `__declspec(naked)`, `_emit`, `.asm`, raw byte
  emission, the user-approved span-MMX block family, provider shims, or future
  raw assembly.

## Use

- Check this file before creating a new source idiom for destructors, thunks,
  vtable stubs, small accessors, zSound helpers, or provider glue.
- Add only patterns backed by current verification output or a durable provider
  ABI note. Keep address examples compact.
- Keep failed-byte functional lanes in their subsystem notes unless they become
  broadly reusable.
