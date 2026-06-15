# zSnd Sample-Set Verification Notes

These notes track verification facts for sample-set registry helpers. Binary
Ninja and VC verifier artifacts remain authoritative.

## 0x4a0920 zSndSampleSetRegistry::FindByName

- Current source: `src/GameZRecoil/zSound/zsnd_sample_set.cpp`.
- Current VC target:
  `tools/vc5_verify_targets/zsnd_sample_set_registry_find_by_name.json`.
- The active target compiles with VC5SP3 `cl` 11.00.7022,
  `/G5 /O2 /Ob0 /MD /Zp4 /FAcs`, and passes with zero unmasked byte
  mismatches after 8 relocation-masked bytes, BN size 102, VC5 size 112, and
  10 trimmed trailing VC NOP bytes.
- Binary Ninja records the expected behavior as a linear scan from
  `g_zSnd_SampleSetRegistry.begin` to `.end` with MSVC's inline `strcmp`
  expansion and a distinct empty-registry return tail.
- Saving both `begin` and `end` into local bounds while keeping the compact
  `for` loop reproduced the retail entry/global-load scheduling and promoted
  the function to tier S.
- A temporary VC5SP3 profile sweep did not find a better flag profile. The
  active profile remained at 94 mismatches with a 96-byte object; alternate
  VC5SP3 profiles also reported 94 mismatches but grew the object to 112 bytes.

## Rejected Probes

- Rewriting the compact `for` loop as explicit saved `begin/end` locals with an
  early empty check and `do/while` worsened the target from 94 to 136
  mismatches and grew the object symbol to 160 bytes, so the compact loop was
  restored.
- Changing the focused target from `/Ob0` to `/Ob1` was neutral at 94
  mismatches.
- Hoisting only the `end` pointer into a local while keeping the compact
  `for` loop was also neutral at 94 mismatches, so the direct
  `g_zSnd_SampleSetRegistry.end` expression was restored.
- Adding only an explicit early empty-registry return improved the first
  instruction and reduced the comparison to 80 mismatches, but it grew the
  object to 112 bytes and assigned `end` to `ebp` while keeping `setName` in
  `ecx`, the opposite of the retail register contract.
- Adding a named `setName` local on top of the early empty-registry return was
  neutral with that probe at 80 mismatches and did not repair the register
  assignment.
- Rewriting the loop as saved `begin`/`end` locals plus `for (;;)` reproduced
  the known bad explicit-loop result: 136 mismatches and a 160-byte object.
- Separating only the `begin` cursor initialization from the compact `for`
  header, without adding an early-empty return or saved `end` local, was
  neutral at 94 mismatches, so the original compact loop spelling was restored.

## 0x4a0fb0 zSndSampleSet::LoadSamplesFromIndexArchive

- Current source: `src/GameZRecoil/zSound/zsnd_sample_set.cpp`.
- Current VC target:
  `tools/vc5_verify_targets/zsnd_sample_set_load_samples_from_index_archive.json`.
- The active target compiles with VC5SP3 `cl` 11.00.7022,
  `/G5 /O2 /Ob1 /MD /GX /Zp4 /FAcs`, and fails with 74 unmasked byte
  mismatches, 40 relocation-masked bytes, BN size 223, VC object symbol size
  224, and 3 trailing VC NOP bytes trimmed.
- Binary Ninja and current VC5 agree on the EH frame, constructor new-expression
  shape, EBX sample cursor, and EDI replay-flag cursor. Remaining drift is in
  the flag-update/store scheduling around the delete cleanup path.

## Rejected Probes

- Rewriting only the failed-parse branch from compound
  `replayFields->flags &= ~0x08` into a local `flags` load/clear/store was
  neutral at 74 mismatches, so the compact compound assignment was restored.
- Folding the success flag update into a single assignment expression was
  neutral at 74 mismatches; VC5 still sank the `replayFields->flags` store
  after the success/failure join and before the wave-data delete check.
- Reordering the success temporaries to clear `flags` before masking/shifting
  `initResult` was also neutral at 74 mismatches.
- Duplicating the `delete waveData` cleanup inside both success and failure
  branches forced branch-local flag stores but regressed to 81 mismatches and
  a 240-byte VC5 body, so the shared cleanup source shape was restored.

## 0x4a0c40 zSndSampleSet::Init

- Current source: `src/GameZRecoil/zSound/zsnd_sample_set.cpp`.
- Current VC target: `tools/vc5_verify_targets/zsnd_sample_set_init.json`.
- The active target compiles with VC5SP3 `cl` 11.00.7022,
  `/G5 /O2 /Ob1 /MD /GX /Zp4 /FAcs`, and fails with 344 unmasked byte
  mismatches, 104 relocation-masked bytes, BN size 499, VC object symbol size
  496, and 1 trailing VC NOP byte trimmed.
- Hoisting the archive-bank name table, archive bank index, and archive-open
  result locals before `archive.Reset()` matches the retail local setup order
  and improves the comparison from 363 to 344 mismatches without changing
  behavior. Functional target `zsnd_sample_set_init` still passes.
- Remaining drift starts at the post-`Reset()` guard and EH-state setup, then
  continues through archive-bank control flow and the loose-file fallback loop.
  The accepted `0x4a0fb0` callee is still tier B and blocks coherent tier S.

## Rejected Probes

- Rewriting the archive-bank attempt loop as `while (archiveAttempt < 3)` with
  an explicit success `break` moved the attempt/result register roles closer to
  retail, but regressed the compare to 348 mismatches and shrank the VC5 body to
  480 bytes, so the original `for` loop spelling was restored.
