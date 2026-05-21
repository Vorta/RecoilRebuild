# Verified Patterns

This ledger captures reusable source and verification shapes that have current
binary-safe evidence. Binary Ninja, `.agent/RECOIL_PLAN.md`, and per-target VC
artifacts remain authoritative for individual functions.

## Current Patterns

- Scalar deleting destructor wrappers: verified examples include
  `0x407170` (`RecoilStateBase::ScalarDeletingDestructor`) and the
  `RecoilStateMainMenuTransition` constructor/destructor helpers. Preserve the
  original-era member ABI, destructor cleanup shape, and nearby provenance
  comments when adding similar wrappers.
- Tiny vtable/no-op helpers: verified examples include `0x407130`,
  `0x407140`, `0x407150`, and `0x407160`. Keep these as simple authored C/C++
  bodies or provider-marked glue according to the plan entry; do not replace
  them with raw assembly.
- HUD/UI leaf accessors and setters: verified examples include
  `HudUiElement::GetX` and nearby small HUD helpers. Prefer named fields and
  static layout checks over offset math once Binary Ninja types are stable.
- zSound snapshot and playback helpers: see the focused `zsnd_*_verification.md`
  notes for current matched call shapes, known byte-diff limits, and VC5SP3 vs
  VC6 profile evidence.
- zSys CPU probes: the CPU feature-detection group is a documented raw-assembly
  exception through `vc6_zsys_cpu_raw_asm`; do not generalize that exception to
  new production functions without explicit approval.

## Use

- Check this file before creating a new source idiom for destructors, thunks,
  vtable stubs, small accessors, zSound helpers, or provider glue.
- Add only patterns backed by current verification output or a durable provider
  ABI note. Keep address examples compact.
- Keep failed-byte functional lanes in their subsystem notes unless they become
  broadly reusable.

