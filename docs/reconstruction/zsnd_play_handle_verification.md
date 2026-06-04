# zSnd Play-Handle Verification Notes

These notes track verification facts for play-handle helpers. Binary Ninja and
VC verifier artifacts remain authoritative.

## Acquire Voice Helpers

- `0x49f6f0` `zSndSample::AcquireA3dVoice` now inlines the A3D provider
  `GetStatus` availability probes instead of calling `A3dHandleIsAvailable`.
  VC5SP3 `cl` 11.00.7022 with `/MD /G5 /O2 /Ob1 /GX /Zp4 /FAcs` now fails
  with 9 unmasked byte mismatches, 24 relocation-masked bytes, BN size 319, VC
  object size 320, and 1 trimmed trailing NOP byte. The non-`/MD` source shape
  failed at 126 mismatches after the helper inlining because it emitted direct
  CRT allocation calls instead of the retail imported CRT call form.
- `0x49f830` `zSndSample::AcquireVoice` now inlines the DirectSound
  `GetStatus` availability probes instead of calling
  `DirectSoundHandleIsAvailable`. VC5SP3 `cl` 11.00.7022 with
  `/MD /G5 /O2 /Ob1 /GX /Zp4 /FAcs` now fails with 9 unmasked byte mismatches,
  16 relocation-masked bytes, BN size 292, VC object size 304, and 12 trimmed
  trailing NOP bytes. The non-`/MD` source shape failed at 122 mismatches after
  the helper inlining because it emitted direct CRT allocation calls instead of
  the retail imported CRT call form.
- Both helpers carry one duplicate-candidate local through the scan and
  duplicate-allocation paths, which better matches the Binary Ninja assembly's
  single candidate register. Remaining tier `S` drift is the duplicate-scan
  fall-through instruction order: retail clears the candidate register before
  reloading `duplicateVoiceCount`, while VC5SP3 emits the reload first. A
  source-level `while` spelling was neutral at the same 9-mismatch profile, so
  both plan entries remain tier `B`.
- Rewriting the DirectSound duplicate scan as explicit source labels and gotos
  was also neutral at the same 9-mismatch profile, so the cleaner `for` loop
  spelling remains.
- Making the DirectSound duplicate candidate pointer volatile worsened the
  target to 306 mismatches, 16 relocation-masked bytes, BN size 292, and VC
  object size 336, so the non-volatile candidate remains the accepted source
  shape.

## 0x49fda0 zSndPlayHandle::StopIfActive

- Current source: `src/GameZRecoil/zSound/zsnd_play.cpp`.
- Current VC target:
  `tools/vc5_verify_targets/zsnd_play_handle_stop_if_active.json`.
- The active target compiles with VC5SP3 `cl` 11.00.7022,
  `/G5 /O2 /Ob1 /GX /Zp4 /FAcs`, and currently fails with 135 unmasked byte
  mismatches, 60 relocation-masked bytes, BN size 275, and VC object symbol
  size 288.
- The source now spells the backend dispatch as an explicit `activeBackend != 0`
  branch with an A3D fall-through and DirectSound else path. This matches the
  original's two-case dispatch shape better than the prior `if` chain.

## Rejected Or Limited Probes

- Initial production-source coverage with the old `if (backend == 0) ...;
  if (backend == 1) ...` shape failed at 198 mismatches.
- Routing member access through a `self` local, with and without `register`, was
  neutral at 198 mismatches and was restored.
- Changing the focused target to VC5SP3 `/O2 /Ob0` without `/GX` was neutral at
  198 mismatches before the backend dispatch rewrite.
- Rewriting the backend dispatch as a `switch` improved the target to 187
  mismatches, but still emitted the wrong branch polarity around the A3D case.
- Reordering the `switch` cases was neutral at 187 mismatches.
- Retesting a `switch (g_zSnd_ActiveBackend)` with case 1 before case 0 after
  the later source-shape improvements worsened the current 135-mismatch
  baseline to 152 mismatches, so the explicit DirectSound-first branch remains
  the best current profile.
- Temporarily compiling the current source across
  `vc5_o2_ob0_facs`, `vc5_o2_ob1_gx_facs`, `vc5_o2_ob1_md_gx_facs`,
  `vc5_o2_ob2_facs`, `vc5_o2_ob2_gx_facs`, `vc5_o2_ob1_gx_facs`, and
  `vc5_o2_oy_ob0_facs` was neutral: every profile produced the same 135
  mismatches, 60 relocation-masked bytes, BN size 275, and object size 288.
- Rewriting the backend dispatch as an explicit `if (activeBackend != 0)` with
  the A3D path physically before the DirectSound path worsened the target from
  135 to 155 mismatches, so the source was restored.
- Rewriting the current dispatch with an explicit `activeBackend -= 0`, a
  `goto DirectSoundBackend`, and the A3D path physically before the DirectSound
  label was neutral at the 135-mismatch profile, so the cleaner current
  explicit DirectSound-first branch was restored.
- Testing the `switch` shape in a historical compiler run worsened the target
  to 196 mismatches, so the target remains on the VC5SP3 profile.
