# zSnd Play-Handle Verification Notes

These notes track verification facts for play-handle helpers. Binary Ninja and
VC verifier artifacts remain authoritative.

## 0x49fda0 zSndPlayHandle::StopIfActive

- Current source: `src/GameZRecoil/zSound/zsnd_play.cpp`.
- Current VC target:
  `tools/vc6_verify_targets/zsnd_play_handle_stop_if_active.json`.
- The active target compiles with VC5SP3 `cl` 11.00.7022,
  `/G5 /O2 /Ob1 /GX /Zp4 /FAcs`, and fails with 183 unmasked byte
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
- Testing the `switch` shape with VC6 `cl` 12.00.8168 worsened the target to
  196 mismatches, so the target remains on the VC5SP3 profile.
