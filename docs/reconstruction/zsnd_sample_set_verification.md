# zSnd Sample-Set Verification Notes

These notes track verification facts for sample-set registry helpers. Binary
Ninja and VC verifier artifacts remain authoritative.

## 0x4a0920 zSndSampleSetRegistry::FindByName

- Current source: `src/GameZRecoil/zSound/zsnd_sample_set.cpp`.
- Current VC target:
  `tools/vc6_verify_targets/zsnd_sample_set_registry_find_by_name.json`.
- The active target compiles with VC6 `cl` 12.00.8168,
  `/G5 /O2 /Ob0 /Zp4 /FAcs`, and fails with 94 unmasked byte mismatches,
  8 relocation-masked bytes, BN size 102, and VC object symbol size 96.
- Binary Ninja records the expected behavior as a linear scan from
  `g_zSnd_SampleSetRegistry.begin` to `.end` with MSVC's inline `strcmp`
  expansion and a distinct empty-registry return tail.

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
