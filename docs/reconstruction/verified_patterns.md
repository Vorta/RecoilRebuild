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
- MFC empty overrides with provider-like bytes: `0x401020`
  (`CAboutDlg::DoDataExchange`) is an authored empty derived-class override even
  though its body byte-matches the inherited MFC no-op. Generated symbol,
  `about.h` declaration, placement between the constructor and message map, and
  the `cabout_prelude_provider_order_current_shape` plus
  `cabout_prelude_functions` VC5 targets prove the override inside the
  continuous `about.cpp` block. Do not classify similar no-op bodies as provider
  solely from byte shape; check class/message-map/vtable and function-order
  evidence first. See `source_file_layout_audit.md` for the full About prelude
  proof.
- VC5 tail-merged duplicated source tails: `0x401060`
  (`AINet::TickAiMode2TopLevel`) byte-matches only when the path-follow and
  auto-turn switch cases contain duplicated attack/rebuild source tails. VC5
  `/O2 /Ob0` then tail-merges those duplicate source occurrences into the
  shared retail runtime block at `0x4010cb`. A hand-written source-level
  `goto` common tail preserved the visible CFG, body size, switch table size,
  and function order, but rotated register allocation and left 19 unmasked byte
  mismatches. When retail shows an earlier case jumping into a later shared
  suffix, a later case falling through into that suffix, and small register-use
  rotations across repeated call/argument-load sites, test duplicated original
  source tails before adding locals, helpers, raw offsets, or compiler-flag
  theories. Do not "clean up" duplicated source into a common label unless the
  VC5 byte target still matches. See `source_file_layout_audit.md` for the
  ai_net-specific proof and `python tools/recoil.py verify vc5 0x401060` for
  the current verifier evidence.
- Evidence-gated raw assembly: default to source-faithful C/C++ and use raw
  assembly only when valid VC5SP3 C/C++ variants cannot produce the observed
  retail bytes or `chatgpt-pro-line` confirms raw assembly is required. Keep the
  asm minimal, inline inside the recovered C/C++ function/helper/macro, and
  document the block with the address, what it does, why C/C++ was not
  sufficient, and the byte/BN/chatgpt-pro-line evidence. Add an
  address-scoped `.agent/RAW_ASSEMBLY_ALLOWLIST.txt` row with the
  `source-faithful-inline-asm` tag or a narrower existing tag. This pattern
  does not permit provider shims, whole-function assembly, raw byte emission,
  `.asm` files, naked functions, linker/order tricks, or assembly where C/C++
  remains source-faithful and byte-capable.
- AINet path-vector helper asm: `0x401180`
  (`AINet::TickAiMode2PathFollow`) is a user-approved raw-assembly exception
  for the recovered `AINET_PATH_*` vector helper macros in
  `src/Battlesport/ai_net.h`. Current VC5 `/O2 /Ob0` evidence byte-matches
  this function only when those helpers act as the observed x87 optimizer
  barrier; ordinary C, aggregate scratch temporaries, independent pointer locals,
  and volatile-lvalue barriers all failed to preserve the retail byte shape.
  This permits inline `__asm` only in the recovered path-vector helper macros,
  scoped to the `0x401180` byte-match evidence. It does not permit
  `__declspec(naked)`, `_emit`, `.asm` files, raw EBP offsets, provider shims,
  linker/order dependence, whole-function raw assembly, or unrelated AINet raw
  assembly.
- HUD/UI leaf accessors and setters: verified examples include
  `HudUiElement::GetX` and nearby small HUD helpers. Prefer named fields and
  static layout checks over offset math once Binary Ninja types are stable.
- zSound snapshot and playback helpers: see the focused `zsnd_*_verification.md`
  notes for current matched call shapes, known byte-diff limits, and VC5SP3
  profile evidence.
- zSys CPU probes: the CPU feature-detection group is a documented raw-assembly
  exception through `vc5_zsys_cpu_raw_asm`; do not generalize that exception to
  new production functions without the evidence-gated raw assembly checklist.
- zRndr overlay MMX rows: `0x48d510` and `0x48d5f0` are a user-approved
  address-scoped raw-assembly exception through
  `vc5_o2_ob0_md_zrndr_mmx_raw_asm_facs`, because VC5SP3 exposes no usable MMX
  intrinsic surface and retail uses authored qword MMX row loops. This permits
  narrow inline `__asm` loops inside ordinary C++ functions only; it does not
  permit `__declspec(naked)`, `_emit`, `.asm`, raw byte emission, other zRndr
  span/MMX families, provider shims, or unrelated raw assembly.
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
  vtable stubs, small accessors, zSound helpers, provider glue, or
  byte-sensitive control-flow cleanup.
- Add only patterns backed by current verification output or a durable provider
  ABI note. Keep address examples compact.
- Keep failed-byte functional lanes in their subsystem notes unless they become
  broadly reusable.
