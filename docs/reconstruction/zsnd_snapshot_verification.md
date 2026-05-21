# zSnd Snapshot Verification Notes

These notes track address-local verification facts for active snapshot
blockers. They do not replace Binary Ninja, plan markers, or VC verification
output.

## 0x4a0500 StopAllIfPlaying

- Current source: `src/GameZRecoil/zSound/zsnd_play.cpp`; the old
  `zsnd_snapshot_stop_all.cpp` file is a build placeholder so this function
  shares the original sound playback translation-unit register allocation.
- Current VC target: `tools/vc6_verify_targets/zsnd_snapshot_stop_all_if_playing.json`
  with VC6 `cl` 12.00.8168, `/G5 /O2 /Oy /Ob0 /Zp4 /FAcs`.
- Current best verification result: `python tools/recoil_vc6_verify.py zsnd_snapshot_stop_all_if_playing`
  fails with 27 unmasked byte mismatches, 8 relocation-masked bytes, BN size
  134, VC6 object size 144, and 10 trailing VC6 NOPs trimmed.
- BN prologue uses `push ecx; push ebx; push esi; push edi`, `xor edx, edx`,
  `mov ebx, 1`, then branchless `sete/neg/sbb/inc/test` sentinel materialization.
  The full-TU VC6 build now matches the stack `GetStatus` scratch at `[esp+0xc]`,
  A3D `GetStatus` slot `0xe0`, DirectSound `GetStatus` slot `0x24`, playing
  tests through `BL`, and `StopIfActive` call shape. Remaining drift is limited
  to the initial and loop-tail sentinel predicates: VC6 omits the pre-compare
  `xor edx, edx` in the first predicate, emits an extra byte `neg` when checking
  the materialized byte against zero, and tests against `BL` instead of itself.
- Predicate probes in `build/experiments/4a0500/bool_shapes.cpp` show plausible
  `unsigned char` and `int` spellings either keep the extra byte `neg` or regress
  to `mov 1; sub`, so the current source keeps the 27-mismatch best profile.
- Functional-equivalence evidence: `tools/functional_verify_targets/zsnd_snapshot_stop_all_if_playing.json`
  and native smoke `zsnd_snapshot_stop_all_if_playing_smoke` in `tests/native/zsnd_cd_tests.cpp`.
  `python tools/recoil_functional_verify.py 0x4a0500` passes; `Binary-safe verified` stays `❌`.
- VC5SP3 full-TU probe (`cl` 11.00.7022, `zsnd_snapshot_stop_all_if_playing_vc5`)
  now fails with **100** unmasked mismatches and **144-byte** object code, worse
  than the VC6 full-TU profile.
- A prior in-`zsnd_play.cpp` VC5 listing (see
  `build/vc6-verify/zsnd_snapshot_create_from_active_samples/zsnd_play.cod`) used `push ecx`,
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
- VC target: `tools/vc6_verify_targets/recoil_state_main_menu_transition_on_try_become_current.json`
  with VC6 `cl` 12.00.8168, `/G5 /O2 /Ob1 /GX /Zp4 /FAcs`, plus `fmv_script.cpp` for blur
  `Constructor` linkage.
- Current byte result: **253** unmasked mismatches, BN 321 vs VC6 272 bytes. Root drift is missing
  retail MSVC SEH/EH scaffolding (`push ebp`, `fs:` chain, `__ehhandler_…`, `[esp+0x40/0x48]` state)
  around stack `zFMV_ActionBlur` and `operator new` for `HudUiMainMenuDialog`; verify TU emits a
  plain `sub esp` frame instead.
- Functional-equivalence: `tools/functional_verify_targets/recoil_state_main_menu_transition_on_try_become_current.json`
  and smoke `recoil_state_main_menu_transition_on_try_become_current_smoke` (frontend route).

## 0x49fff0 CreateFromActiveSamples

- Current source: `src/GameZRecoil/zSound/zsnd_play.cpp`.
- Current VC target: `tools/vc6_verify_targets/zsnd_snapshot_create_from_active_samples.json`.
- Current best verification result: `python tools/recoil_vc6_verify.py 0x49fff0`
  with VC5SP3 `cl` 11.00.7022, `/G5 /O2 /Ob1 /GX /Zp4 /FAcs`, fails with
  317 unmasked byte mismatches, 80 relocation-masked bytes, BN size 778, and
  VC object symbol size 784.
- `0x4a07c0` is modeled as `zSndPlayHandleSnapshot::NewNode`, a thiscall
  member whose body ignores `ECX` and returns with `ret 8`. This is required
  because `0x49fff0` seeds `ECX` with the owning snapshot before every call.
- The remaining byte drift is concentrated around register choice for backend
  `GetStatus` vtable calls and the A3D duplicate manual splice. BN has
  `lea edi, [eax+8]; test edi, edi; mov [prev], eax; je skip_copy` before the
  manual payload copy at `0x4a01b0`, but naive source-level guards perturb
  loop register allocation and increase mismatches.
- Binary Ninja still reports stack-merge warning tags at the A3D primary-to-
  duplicate branch join (`0x4a0143`), but `inspect_stack_merge_conflict`
  confirms the indirect `GetStatus` calls at `0x4a00fe`, `0x4a016f`,
  `0x4a01f6`, and `0x4a0260` are modeled as popping their two stack arguments.
  No call-stack override is justified from current evidence; the database was
  refreshed and saved with the warning tags left as analysis artifacts.
- The manual A3D duplicate node allocation is spelled as
  `new zSndPlayHandleSnapshotItem`; VC5SP3 emits the same current 317-mismatch
  profile as the prior explicit `::operator new(sizeof(...))` spelling, while
  keeping the source closer to plausible original C++.

## Rejected Probes

- Changing the active target to VC6 `cl` 12.00.8168 produced the same 317
  mismatch count as VC5SP3 for the current source shape.
- Changing only the active target inlining between `/Ob0`, `/Ob1`, and `/Ob2`
  showed `/Ob1` is the best current profile; `/Ob0` and `/Ob2` were worse.
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
- Introducing call-site `zSndBuffer *const` locals for the A3D primary and
  duplicate backend buffers worsened the target from 317 to 497 mismatches and
  grew the object symbol from 784 to 800 bytes, so the direct field expressions
  were restored.
- Introducing scoped `duplicateVoices` pointer locals inside both duplicate
  loops was neutral at the 317 mismatch profile and did not change the original
  vs. VC register choices for the duplicate array load, so the direct
  `sample->duplicateVoices[voiceIndex]` expressions were restored.
- Rewriting the A3D duplicate loop as an early `continue` for null voices was
  neutral at the 317 mismatch profile, so the compact `voice != 0 && ...`
  condition was restored.
- Writing the manual A3D duplicate splice through the saved predecessor local
  (`prev->next = node`) worsened the target from 317 to 337 mismatches, so the
  source was restored to `node->prev->next = node`.
- Replacing the manual A3D duplicate splice with `snapshot->AppendPayload`
  worsened the target from 317 to 346 mismatches and changed the object symbol
  from 784 to 768 bytes, confirming the direct `operator new`/manual splice is
  still the closer source shape for this site.
