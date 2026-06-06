# Verified Patterns

This ledger captures reusable source and verification shapes that have current
tier `S` evidence. Binary Ninja, `.agent/RECOIL_PLAN.md`, and per-target VC
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
  bodies or provider-marked glue according to the plan entry; do not replace
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

## Use

- Check this file before creating a new source idiom for destructors, thunks,
  vtable stubs, small accessors, zSound helpers, or provider glue.
- Add only patterns backed by current verification output or a durable provider
  ABI note. Keep address examples compact.
- Keep failed-byte functional lanes in their subsystem notes unless they become
  broadly reusable.
