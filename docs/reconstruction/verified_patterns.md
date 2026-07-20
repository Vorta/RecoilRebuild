# Verified Patterns

This ledger captures reusable source and verification shapes backed by current
evidence. It is not a tier ledger: individual entries state whether their
evidence is address-local, owner-scoped, or provider-scoped. Binary Ninja,
the unified reconstruction tracker and per-target VC artifacts remain authoritative
for individual functions and owner tiers.

## Current Patterns

- Scalar deleting destructor wrappers: current Binary Ninja evidence classifies
  wrappers such as `0x434980` (`HudUiSaveGameDialog::ScalarDeletingDestructor`), and `0x434dd0`
  (`HudUiLoadGameDialog::ScalarDeletingDestructor`), and `0x435ca0`
  (`RecoilStateSaveLoadTransition::ScalarDeletingDestructor`), and `0x4429b0`
  (`RecoilApp_MfcOleModule::ScalarDeletingDestructor`) as compiler-generated
  provider glue when the body only performs destructor dispatch, tests the
  delete flag, optionally calls `operator delete`, and returns `this`. Do not
  author these wrappers manually in production source; model the owning C++
  class/interface and record the wrapper as a provider boundary.
  Active caveat: `0x407170` has the direct-write scalar-deleting-wrapper byte
  shape, but its `0x4ccd50` state-base/interface table boundary and owner are
  still unresolved in the `hud.cpp` block audit. Do not cite this pattern as an
  accepted owner/provider claim for `0x407170` or `0x4ccd50`.
- Scalar deleting destructor wrappers with an inlined small destructor body
  follow the same source-shape rule. For example, `0x40bf50`
  (`HudCmdBindingEntry::ScalarDeletingDestructor`) is compiler glue that
  inlines the authored `HudCmdBindingEntry` display-string destructor before
  testing the delete flag. Model the destructor on the class, not as a
  hand-authored production `ScalarDeletingDestructor` method.
- Tiny vtable/no-op helpers: verified examples include `0x407130`,
  `0x407140`, `0x407150`, and `0x407160`. Keep these as simple authored C/C++
  bodies or provider-marked glue according to the owner-ledger entry; do not replace
  them with raw assembly.
- MFC shared no-op address groups: `0x401020` is a linked `ret 4` group. The
  `CAboutDlg` vtable DDX slot points there, but the same body shape and its
  heterogeneous xrefs do not prove a unique authored `CAboutDlg::DoDataExchange`
  override, provider identity, or source host. The current About source model is
  a conventional dynamic-MFC `CAboutDlg` with inherited DDX and a provisional
  `about.cpp`/`about.h` host. Treat tiny MFC bodies as shared linked groups until
  class/message-map/vtable, generated-symbol, object-order, and link evidence
  establish a more specific identity. See `source_file_layout_audit.md` for the
  current About prelude limits.
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
- Evidence-gated raw assembly: the complete acceptance rule lives only in
  `.codex/skills/recoil-tier-verification/SKILL.md`. Historical examples below
  do not relax its failed-source-faithful-variants **and** applicable-Pro-pass
  requirement, synchronized evidence, minimal inline scope, docblock,
  allowlist, or forbidden-form rules.
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
- AINet forward-probe final add asm: `0x401420`
  (`AINet::AiMode2ForwardProbeRequiresAutoTurn`) keeps normal C++ for the
  early checks, double-evaluating normalize clamp, endpoint scale, contact
  collection, queue checks, and returns. ChatGPT Pro source-shape review and
  VC5 evidence isolate the original raw-assembly footprint to the final
  fixed-register x87 vector-add body in
  `AINET_FORWARD_PROBE_ADD_WORLD_ASM`; `python tools/recoil.py verify vc5
  0x401420` passes with zero unmasked mismatches under `vc5_o2_ob0_md_facs`.
  The endpoint add source is the original `playerState->worldPos` pointer, not
  the raised local start point, and the normalize clamp must not be rewritten as
  a cached single-evaluation local.
  This does not permit naked/whole-function asm, `_emit`, raw EBP offsets, or
  unrelated AINet raw assembly.
- AINet path-cursor loop ordering: `0x401580`
  (`AINet::AiAdvancePathCursorAndComputeTargetVec`) is ordinary C++ through the
  reverse-edge scan and merge; only the final recurring grouped-x87 vector
  subtraction remains an inline-assembly macro. Under VC5SP3 11.00.7022 with
  `/nologo /TP /W3 /MD /G5 /O2 /Ob0 /Zp4 /FAcs` and the local frame-pointer
  region, both the `do/while` and `for (;;)` plus positive-`continue` forms
  produced 109 unmasked mismatches, a 285-byte body, three NOPs, and the second
  helper relocation at `+0xc5`. The pre-tested
  `while (branchOffset < 0x18)` produced zero mismatches, a 283-byte body in a
  288-byte extent, five NOPs, a `0x10` frame, the retail `+0xc3` relocation,
  and the direct retail `jl` loop latch. The supported cause is VC5 front-end
  CFG/basic-block ordering: it folds the statically true initial test but
  retains the pre-tested source form's block order. The exact original lexical
  tokens remain unprovable.
- AINet path-helper linkage caveat: TU-static and class-static forms of
  `0x4016a0` (`AiChooseNextPathBranchIndex`) produced byte-equivalent caller and
  callee code, so linkage was not the cause of the `0x401580` loop-layout
  difference. The class-static form remains for semantic and functional-test
  API consistency.
- Hard-byte prompt mechanics are not a reusable pattern in this document. Use
  `recoil-tier-verification`; production/upload belongs to
  `recoil_source_worker`, retail assembly to `recoil_bn_fact_mapper`, and
  validation to `recoil_verifier`.
- Hard-byte differential prompt case study (`0x401580`): ask ChatGPT Pro a
  causal side-by-side code-generation question instead of asking it to validate
  an assumed source form. The earlier review prompt prematurely assumed a
  bottom-tested loop and explicitly excluded a pre-tested `while`; the later
  self-contained differential inquiry isolated the remaining
  109-mismatch/two-byte CFG delta and asked Pro to challenge that premise.
  Follow the current canonical hard-byte evidence rule, then add frame/register/stack-slot/relocation/body
  size/function-order facts, failed one-axis probes, recurring macro evidence,
  and the full relevant HLIL and user-type exports. Ask Pro to compare exact
  `for`/`while`/`do-while`/`break`/`continue` forms, predict the critical
  assembly, separate linkage from body codegen, rank bounded one-axis probes
  with hard stops, and state contradictions or missing evidence. Pro's
  first-ranked explicit-`continue` probe failed unchanged; its second-ranked
  pre-tested `while` matched. The reusable value was the bounded hypothesis
  ladder, not treating Pro as an oracle. Reproduce with
  `python tools/recoil.py verify vc5 0x401580`. This is narrow address/block
  evidence, not source-owner acceptance, `Model: source-faithful`, or tier `S`
  evidence.
- AINet steering dispatcher source shape (`0x401710`): an exact VC5 result
  required recovering several interacting source constructs rather than
  transcribing the function. Bind three short-lived vector pointers around the
  existing generic `AINET_VECTOR_SUBTRACT`, snapshot `steerBasisNorm`, keep the
  cross/dot expressions as ordinary scalar C++, avoid retaining the target
  player pointer across the attack call, and order the switch bodies lexically
  as direct, offset, dynamic, auto-turn, turn-in-place, then path-follow. The
  final scalar declaration order lets VC5 reuse the subtraction pointer slots
  for `lateralDot`, `targetDistance`, and the vertical scale, naturally producing
  the retail `0x38` frame. The resulting contribution is exact across the
  565-byte executable body, three-byte pre-table alignment, seven-entry switch
  table, and twelve trailing NOPs, for 608 bytes through the next function at
  `0x401970`. Use `bn_byte_length: 608` for this compiler-owned contribution;
  do not create fake functions or data owners at `0x401948` or `0x401964`.
  ChatGPT Pro correctly rejected reuse of the address-sensitive XZ assembly
  helpers here because retail uses direct EBP-relative operands, and no new raw
  assembly was required. The retained differential-review transcript records
  that scoped conclusion directly.
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
