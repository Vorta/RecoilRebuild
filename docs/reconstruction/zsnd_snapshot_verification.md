# zSnd Snapshot Verification Notes

These notes track address-local verification facts for active snapshot
blockers. They do not replace Binary Ninja, plan markers, or VC verification
output.

## 0x4a0500 StopAllIfPlaying

- Current source: `src/GameZRecoil/zSound/zsnd_play.cpp`; the old
  `zsnd_snapshot_stop_all.cpp` file is a build placeholder so this function
  shares the original sound playback translation-unit register allocation.
- Current VC target: `tools/vc5_verify_targets/zsnd_snapshot_stop_all_if_playing.json`
  with VC5SP3 `cl` 11.00.7022, `/G5 /O2 /Oy /Ob0 /Zp4 /FAcs`.
- Current best verification result: `python tools/recoil.py verify vc5 zsnd_snapshot_stop_all_if_playing`
  fails with 27 unmasked byte mismatches, 8 relocation-masked bytes, BN size
  134, VC5SP3 object size 144, and 10 trailing VC5SP3 NOPs trimmed.
- BN prologue uses `push ecx; push ebx; push esi; push edi`, `xor edx, edx`,
  `mov ebx, 1`, then branchless `sete/neg/sbb/inc/test` sentinel materialization.
  The full-TU VC5SP3 build now matches the stack `GetStatus` scratch at `[esp+0xc]`,
  A3D `GetStatus` slot `0xe0`, DirectSound `GetStatus` slot `0x24`, playing
  tests through `BL`, and `StopIfActive` call shape. Remaining drift is limited
  to the initial and loop-tail sentinel predicates: VC5SP3 omits the pre-compare
  `xor edx, edx` in the first predicate, emits an extra byte `neg` when checking
  the materialized byte against zero, and tests against `BL` instead of itself.
- Predicate probes in `build/experiments/4a0500/bool_shapes.cpp` show plausible
  `unsigned char` and `int` spellings either keep the extra byte `neg` or regress
  to `mov 1; sub`, so the current source keeps the 27-mismatch best profile.
- Tier `B` evidence: `tools/functional_verify_targets/zsnd_snapshot_stop_all_if_playing.json`
  and native smoke `zsnd_snapshot_stop_all_if_playing_smoke` in `tests/native/zsnd_cd_tests.cpp`.
  `python tools/recoil.py verify functional 0x4a0500` passes; the entry remains below tier `S`.
- VC5SP3 full-TU probe (`cl` 11.00.7022, `zsnd_snapshot_stop_all_if_playing_vc5`)
  now fails with **100** unmasked mismatches and **144-byte** object code, worse
  than the VC5SP3 full-TU profile.
- A prior in-`zsnd_play.cpp` VC5 listing (see
  `build/vc5-verify/zsnd_snapshot_create_from_active_samples/zsnd_play.cod`) used `push ecx`,
  stack `outStatus`, and `do { } while`, but still used plain `cmp/jne` and an EBP frame — closer
  prologue/scratch shape, still not a byte match.
- **Runtime impact of current drift (reviewed 2026-05-21):** no known caller-visible effect.
  - Return is always `1`; BN and both call sites (`0x4152db`, `0x463035`) ignore it.
  - After the call, callers reload `ECX` (`mov ecx, imm` / `mov ecx, [esi+…]`), so missing
    `push/pop ecx` in the rebuild is not an ABI contract for these sites. BN's `push ecx` is
    reused as the `GetStatus` scratch at `[esp+0xc]`, not callee-saved `this` preservation.
  - `g_zSndStopAllStatusScratch` vs stack scratch is equivalent on a single-threaded engine unless
    `StopAllIfPlaying` reenters (not observed).
  - Branchless vs `cmp/je` loop sentinels are predicate-equivalent; backend `switch` and
    `test byte …, bl` playing checks match BN semantics (functional smoke covers DS/A3D paths).

## 0x415220 RecoilStateMainMenuTransition::OnTryBecomeCurrent

- Current source: `src/GameZRecoil/RecoilApp/RecoilStateMainMenuTransition_OnTryBecomeCurrent.cpp`
  (uses production `zFMV_ActionBlur` from `fmv.h`).
- VC target: `tools/vc5_verify_targets/recoil_state_main_menu_transition_on_try_become_current.json`
  with VC5SP3 `cl` 11.00.7022, `/G5 /O2 /Ob1 /GX /Zp4 /FAcs`, plus
  `fmv_script.cpp` for blur `Constructor` linkage.
- Tier `S`: `python tools/recoil.py verify vc5 0x415220` now has zero
  unmasked COFF-byte mismatches after 76 relocation-masked bytes. BN body is
  321 bytes, VC object symbol is 336 bytes, and 15 trailing VC NOPs are
  trimmed.
- Source-shape fix: spelling the dialog construction as
  `new HudUiMainMenuDialog(m_entryRoute)` restores the retail MSVC EH/new
  allocation state, and dispatching `SetEnabled(1)` through the recovered
  dialog-controller vtable prefix restores the BN virtual call at `0x415319`.
- Tier `B`: `tools/functional_verify_targets/recoil_state_main_menu_transition_on_try_become_current.json`
  and smoke `recoil_state_main_menu_transition_on_try_become_current_smoke`
  still pass for the frontend route.

## 0x49fff0 CreateFromActiveSamples

- Current source: `src/GameZRecoil/zSound/zsnd_play.cpp`.
- Current VC target: `tools/vc5_verify_targets/zsnd_snapshot_create_from_active_samples.json`.
- Current best verification result: `python tools/recoil.py verify vc5 0x49fff0`
  with VC5SP3 `cl` 11.00.7022, `/G5 /O2 /Ob1 /GX /Zp4 /FAcs`, fails with
  317 unmasked byte mismatches, 80 relocation-masked bytes, BN size 778, and
  VC object symbol size 784.
- The byte verifier now emits `49fff0_asm_classified_diff.txt` beside the
  COFF-byte artifacts. The current classified text report has 61 exact or
  normalized instruction matches, 56 byte-identical spelling differences, 10
  relocation-sensitive differences, 0 currently accepted schedule-equivalent
  pairs, 7 branch-displacement mismatches, 16 register-allocation mismatches,
  2 structural LCS drift blocks, and 131 remaining instruction mismatches.
- A bounded instruction-stream resync classifier probe was rejected for this
  target: it isolated the A3D duplicate guard locally but then produced
  misleading BN-only/COD-only blocks across the following backend sections, so
  the stable index-based classifier plus explicit branch/register buckets
  remains the mismatch-count source. The current diagnostic LCS section is
  shape-based only and reports structural drift without changing acceptance:
  the A3D duplicate splice has BN `test edi, edi; mov [edx], eax; je` where
  the VC object has `lea esi, payload; mov [ecx], eax`, followed by BN-only
  payload-source `lea esi, [esp+0x2c]`. The COD extractor now stops at the
  main `_TEXT ENDS` boundary so the separate `text$x` EH helper is not included
  in classified text diagnostics; the COFF byte comparison was already using
  the `.text` function section bytes and remains unchanged.
- `0x4a07c0` is modeled as `zSndPlayHandleSnapshot::NewNode`, a thiscall
  member whose body ignores `ECX` and returns with `ret 8`. This is required
  because `0x49fff0` seeds `ECX` with the owning snapshot before every call.
- The remaining byte drift is concentrated around register choice for backend
  `GetStatus` vtable calls and the A3D duplicate manual splice. BN has
  `lea edi, [eax+8]; test edi, edi; mov [prev], eax; je skip_copy` before the
  manual payload copy at `0x4a01b0`, but naive source-level guards perturb
  loop register allocation and increase mismatches.
- Current Binary Ninja bridge evidence reports no warning tags for `0x49fff0`.
  Earlier stack-merge warnings at the A3D primary-to-duplicate branch join
  (`0x4a0143`) were reviewed with `inspect_stack_merge_conflict`, which
  confirmed the indirect `GetStatus` calls at `0x4a00fe`, `0x4a016f`,
  `0x4a01f6`, and `0x4a0260` are modeled as popping their two stack arguments.
  No call-stack override is justified from current evidence.
- The manual A3D duplicate node allocation is spelled as
  `new zSndPlayHandleSnapshotItem`; VC5SP3 emits the same current 317-mismatch
  profile as the prior explicit `::operator new(sizeof(...))` spelling, while
  keeping the source closer to plausible original C++.

## Rejected Probes

- A refreshed 2026-06-03 full profile sweep against the current
  `src/GameZRecoil/zSound/zsnd_play.cpp` source found `vc5_o2_ob1_gx_facs`,
  `vc5_o2_ob1_md_gx_facs`, `vc5_o2_ob1_gx_uintptr_facs`,
  and `vc5_o2_ob1_md_gx_afx_uintptr_win32ie_facs` tied as closest at 317
  mismatches. `/Ob0` profiles were 675-677 mismatches, VC5SP3 `/Oy /Ob0`
  was 677 mismatches with a 576-byte object, plain VC5 `/Ob1`
  without `/GX` was 687 mismatches, VC5SP3 `/Ob2 /GX` profiles were 707
  mismatches, and VC5 `/Ob2` without `/GX` was 718 mismatches.
- Changing `/O2` to `/Ox` was neutral at the 317 mismatch profile. Changing
  `/G5` to `/G6` worsened the target to 384 mismatches, so the manifest was
  restored to `/G5 /O2 /Ob1`.
- Changing `/G5` to `/G4` was neutral at the 317 mismatch profile, so the
  manifest remains at the existing `/G5` setting.
- Adding `/Oa` to the active `/G5 /O2 /Ob1` flag set worsened the target from
  317 to 566 mismatches and grew the object symbol from 784 to 800 bytes, so
  the manifest was restored.
- Changing `/O2` to `/O1` worsened the target from 317 to 675 mismatches and
  shrank the object symbol from 784 to 567 bytes, so the manifest was
  restored to the speed-optimized `/O2` profile.
- Rewriting `DirectSoundBufferIsPlaying` and `A3dSourceIsPlaying` with typed
  vtable entries, explicit function-pointer temporaries, reversed argument
  order, or reference-style status parameters did not improve the object bytes.
- Moving the A3D duplicate manual splice or only its guarded copy into an inline
  helper generated worse register allocation, increasing the mismatch count.
- Adding an explicit guarded payload pointer around the A3D duplicate manual
  splice, either after `node->prev->next = node` or in the same statement order
  as `AppendPayload`, worsened the active target from 317 to 489 mismatches.
- Guarding the A3D duplicate manual payload copy with `if (node != 0)` also
  worsened the active target from 317 to 489 mismatches, so the direct copy was
  restored.
- Delaying the manual A3D duplicate copy source calculation with explicit
  `nodePrev` and `payloadDest` locals still generated the same 489-mismatch
  guarded-copy profile, so the direct copy was restored.
- Spelling the A3D duplicate payload destination as `(char *)node +
  offsetof(zSndPlayHandleSnapshotItem, payload)` with an explicit null guard
  also worsened the active target from 317 to 489 mismatches, then the source
  was restored to the direct `&node->payload` copy.
- Guarding the direct A3D duplicate copy with
  `(unsigned int)&node->payload != 0` also worsened the active target from 317
  to 489 mismatches, then the source was restored to the direct copy.
- Guarding the direct A3D duplicate copy with the lighter `if (&node->payload)`
  spelling also worsened the target from 317 to 489 mismatches, then the source
  was restored to the direct copy.
- Materializing the A3D duplicate payload destination as a `volatile`
  `nodePayload` local without an explicit guard worsened the target from 317 to
  343 mismatches and reduced trailing VC NOP trimming from 10 to 2 bytes, so the
  direct `memcpy(&node->payload, ...)` spelling was restored.
- Adding a `register` hint to the per-sample `sample` local was neutral at the
  317 mismatch profile by itself. Combining that hint with the guarded A3D
  duplicate copy failed to compile in the VC5SP3 target, so both edits were
  restored.
- Introducing explicit `DirectSoundBufferVTable` / `A3dSourceVTable`
  temporaries inside the inline `*IsPlaying(..., int *status)` helpers produced
  the same 317 mismatch profile and did not affect the register-choice drift.
- Inlining the two A3D `GetStatus` calls directly in
  `CreateFromActiveSamples` also produced the same 317 mismatch profile, so the
  source was restored to the shared `A3dSourceIsPlaying(..., &status)` helper.
- Inlining the two DirectSound `GetStatus` calls directly in
  `CreateFromActiveSamples` was also neutral at the 317 mismatch profile, so
  the source was restored to the shared
  `DirectSoundBufferIsPlaying(..., &status)` helper.
- Spelling A3D `GetStatus` as a raw vtable slot call
  `((BackendGetStatusFn)vtable[0x38])(...)` compiled to the same 317 mismatch
  profile and the same scratch-register choices as the typed helper, so the
  typed `source->vtable->GetStatus(...)` spelling was restored.
- Introducing a function-local `int *const statusPtr = &status` and passing
  that pointer to each `*IsPlaying` helper was neutral at the 317 mismatch
  profile, so the simpler `&status` call sites were restored.
- Typing the A3D `GetStatus` vtable slot as a concrete
  `int (__stdcall *)(A3dSource *, int *)` instead of the shared `void *`
  backend function-pointer type was also neutral at the 317 mismatch profile,
  so the shared `BackendGetStatusFn` slot type was restored.
- Hoisting `g_zSnd_ActiveBackend` into a local `const int activeBackend` before
  the switch was neutral at the 317 mismatch profile, so the direct switch
  expression was restored.
- Adding helper-local `int *const outStatus = status` aliases inside
  `DirectSoundBufferIsPlaying` and `A3dSourceIsPlaying` was neutral at the 317
  mismatch profile, so the direct `status` uses were restored.
- Spelling the helper status test as `(status[0] & 1) != 0` instead of
  `(*status & 1) != 0` was neutral at the 317 mismatch profile, so the original
  dereference spelling was restored.
- Changing the inline `DirectSoundBufferIsPlaying` and `A3dSourceIsPlaying`
  helper returns from `bool` to `int` was neutral at the 317 mismatch profile
  and added VC5SP3 bool-conversion warnings in `BackendHandleIsPlaying`, so the
  `bool` helper returns were restored.
- Changing only `DirectSoundBufferIsPlaying` to accept `LPDWORD status` and
  moving the `(LPDWORD)&status` casts to the DirectSound call sites was neutral
  at the 317 mismatch profile, so the shared `int *` helper contract was
  restored.
- Introducing call-site `zSndBuffer *const` locals for the A3D primary and
  duplicate backend buffers worsened the target from 317 to 497 mismatches and
  grew the object symbol from 784 to 800 bytes, so the direct field expressions
  were restored.
- Changing the `CreateFromActiveSamples` status scratch from `int status` to
  `int status[1]` and passing the array to the inlined backend status helpers
  was neutral at the 317 mismatch profile, so the scalar scratch was restored.
- Changing the same status scratch to `volatile int status` and passing
  `(int *)&status` to the inlined backend helpers was also neutral at the 317
  mismatch profile, so the non-volatile scalar scratch was restored.
- Changing the shared inlined backend-status helpers and local scratches from
  `int` to `DWORD` was rejected during compile: the same helper shape is used by
  nearby A3D availability paths whose provider `GetStatus` slots take `int *`,
  so the probe required casts across unrelated source and was restored before
  byte comparison. The 0x49fff0 target returned to the 317-mismatch baseline.
- Changing the sample-set and sample loop counters/counts from signed integers
  with explicit unsigned comparisons to `unsigned int` counters was neutral at
  the 317 mismatch profile, so the existing signed counter spelling was
  restored.
- Introducing scoped `duplicateVoices` pointer locals inside both duplicate
  loops was neutral at the 317 mismatch profile and did not change the original
  vs. VC register choices for the duplicate array load, so the direct
  `sample->duplicateVoices[voiceIndex]` expressions were restored.
- Introducing a narrower A3D-only `duplicateVoices` pointer local in the
  duplicate loop worsened the target from 317 to 522 mismatches, reduced
  relocation masking from 80 to 76 bytes, reduced VC NOP trimming from 10 to 8
  bytes, and expanded the object body from 784 to 800 bytes, so the direct
  `sample->duplicateVoices[voiceIndex]` expression remains the closest spelling.
- Rewriting the A3D duplicate loop as an early `continue` for null voices was
  neutral at the 317 mismatch profile, so the compact `voice != 0 && ...`
  condition was restored.
- Rewriting the DirectSound duplicate loop as an early `continue` for null
  voices was also neutral at the 317 mismatch profile, so the compact
  `voice != 0 && ...` condition was restored.
- Rewriting the A3D duplicate loop as an explicitly initialized `while` loop
  with a tail increment was also neutral at the 317 mismatch profile, so the
  existing `for` spelling was restored.
- Swapping the private two-argument backend-status helpers so the `status`
  pointer parameter preceded the backend buffer parameter was neutral at the
  317 mismatch profile, so the source-level buffer/status order was restored.
- Adding `register` hints to the inlined A3D backend-status helper's provider
  source local or a separate vtable local was neutral at the 317 mismatch
  profile and did not move the vtable load into the original `ECX` register
  choice, so the normal direct `source->vtable->GetStatus(...)` spelling was
  restored.
- Adding a `register` hint to the inlined DirectSound backend-status helper's
  COM buffer local was also neutral at the 317 mismatch profile, so the normal
  `LPDIRECTSOUNDBUFFER const buffer` spelling was restored.
- Writing the manual A3D duplicate splice through the saved predecessor local
  (`prev->next = node`) worsened the target from 317 to 337 mismatches, so the
  source was restored to `node->prev->next = node`.
- Materializing `node->prev` into a separate `nodePrev` local and writing
  `nodePrev->next = node` was neutral at the 317 mismatch profile, so the
  direct `node->prev->next = node` spelling was restored.
- Replacing the saved A3D duplicate predecessor local with direct
  `listHead->prev` expressions worsened the target from 317 to 338 mismatches,
  so the saved `prev` local was restored.
- Saving `&listHead->prev` in a `prevLink` pointer-to-link local and assigning
  `*prevLink = node` was neutral at the 317 mismatch profile, so the direct
  `listHead->prev = node` spelling was restored.
- Spelling the manual A3D duplicate count update as
  `snapshot->itemCount = snapshot->itemCount + 1` was neutral at the
  317 mismatch profile, so the compact preincrement was restored.
- Replacing the manual A3D duplicate splice with `snapshot->AppendPayload`
  worsened the target from 317 to 346 mismatches and changed the object symbol
  from 784 to 768 bytes, confirming the direct `operator new`/manual splice is
  still the closer source shape for this site.
- Replacing the manual A3D duplicate payload `memcpy` with plain POD assignment
  `node->payload = payload` was neutral at the 317 mismatch profile, so the
  explicit copy was restored.
- Marking the manual A3D duplicate `node` local as `volatile` worsened the
  target from 317 to 436 mismatches, reduced relocation masking from 80 to 76
  bytes, and expanded the object body from 784 to 816 bytes, so the normal
  `const` pointer local was restored.
- Removing `const` from the manual A3D duplicate `listHead`, `prev`, and
  `node` pointer locals was neutral at the 317 mismatch profile, so the
  existing const-qualified local spelling was restored.
- Replacing `new zSndPlayHandleSnapshot(backendTag)` with manual
  `::operator new(sizeof(zSndPlayHandleSnapshot))` plus field initialization
  worsened the target from 317 to 702 mismatches, reduced relocation masking
  from 80 to 64 bytes, and shrank the object body from 784 to 736 bytes. The
  inline C++ constructor spelling remains the accepted source model for the
  EH/new setup around `0x49fff0`.
- Rewriting the backend dispatch as explicit `if (g_zSnd_ActiveBackend == 0)`
  / `else if (g_zSnd_ActiveBackend == 1)` source branches worsened the target
  from 317 to 481 mismatches with the same 784-byte object symbol, so the
  `switch` spelling remains the closest current source shape.
- Placing `case 0` before `case 1` inside the backend switch was neutral at the
  317 mismatch profile, so the current case order is retained because it better
  reflects the observed A3D fall-through / DirectSound out-of-line block
  placement.
