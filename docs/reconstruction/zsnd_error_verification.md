# zSnd Error Verification Notes

These notes track verification facts for zSound error-report helpers. Binary
Ninja and the VC6 verifier artifacts remain authoritative.

## 0x4a3ef0 zSnd::ReportA3DError

- Binary Ninja records the most likely original file as
  `GameZRecoil/zSound/zsnd_create.cpp`; the current implementation and manifest
  still compile the helper from `src/GameZRecoil/zSound/zsnd_init.cpp`.
- The active target `python tools/recoil_vc6_verify.py 0x4a3ef0` compiles with
  VC5SP3 `cl` 11.00.7022, `/G5 /O2 /Ob1 /GX /Zp4 /FAcs`. Its manifest sets
  `bn_byte_length: 1088` because the VC object symbol includes the switch jump
  table in the COMDAT byte span while Binary Ninja's function-instruction byte
  extraction stops before the adjacent original table at `0x4a4248`.
- With the 1088-byte Binary Ninja span from `0x4a3ef0` through the adjacent
  jump table and the existing COFF relocation mask, the active target fails
  with 199 mismatches, 476 relocation-masked bytes, and equal BN/VC byte sizes.
  The earlier instruction-only verifier profile failed with 414 mismatches.
  This span support is verifier coverage progress, not binary-safe acceptance.
- Rewriting the early `a3dError == 0` path as a separate immediate return made
  VC5SP3 choose a short forward branch and worsened the normal verifier profile
  from 414 to 756 mismatches, so the original nested `if (a3dError != 0)` source
  shape was restored.
