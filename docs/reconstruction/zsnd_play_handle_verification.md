# zSnd Play-Handle Verification Notes

These notes track verification facts for play-handle helpers. Binary Ninja and
VC verifier artifacts remain authoritative.

## 0x49fda0 zSndPlayHandle::StopIfActive

- Current source: `src/GameZRecoil/zSound/zsnd_play.cpp`.
- Current VC target:
  `tools/vc6_verify_targets/zsnd_play_handle_stop_if_active.json`.
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
  `vc5_o2_ob2_facs`, `vc5_o2_ob2_gx_facs`, `vc6_o2_ob1_gx_facs`, and
  `vc6_o2_oy_ob0_facs` was neutral: every profile produced the same 135
  mismatches, 60 relocation-masked bytes, BN size 275, and object size 288.
- Rewriting the backend dispatch as an explicit `if (activeBackend != 0)` with
  the A3D path physically before the DirectSound path worsened the target from
  135 to 155 mismatches, so the source was restored.
- Testing the `switch` shape with VC6 `cl` 12.00.8168 worsened the target to
  196 mismatches, so the target remains on the VC5SP3 profile.
