# zFMV Script Cleanup/Reset Verification Notes

These notes track binary-lane evidence for the `zFMV_Script::Cleanup` and
`zFMV_Script::Reset` dependency pair used by RecoilApp FMV state teardown.

## 0x462630 zFMV_Script::Cleanup

- Current source: `src/GameZRecoil/zFMV/fmv_script.cpp`.
- Current VC target: `tools/vc5_verify_targets/zfmv_script_cleanup_reset.json`.
- Current best verification result:
  `python tools/recoil_vc5_verify.py 0x462630 --build-root
  build/vc5-verify-probe-462630-import-free` with VC5SP3 `cl` 11.00.7022,
  `/G5 /O2 /Ob0 /Zp4 /FAcs`, passes with zero unmasked byte mismatches after
  8 relocation-masked bytes. BN body size is 36 bytes, VC object size is
  48 bytes, and 12 trailing VC NOP bytes are trimmed.
- The verification spelling used for the tier S byte match uses the existing repo-local imported CRT provider
  pattern for VC5 32-bit builds: `__imp__free(m_fmvPath)` under the VC5 x86
  guard and ordinary `free(m_fmvPath)` for modern/native builds. This matches
  the retail `call dword [free]` spelling and fixes the nullable-path branch
  displacement; the direct call to `zFMV_Script::Reset` is accepted through the
  COFF relocation mask.

## 0x462660 zFMV_Script::Reset

- Current source: `src/GameZRecoil/zFMV/fmv_script.cpp`.
- Current VC target: `tools/vc5_verify_targets/zfmv_script_cleanup_reset.json`.
- Current best verification result:
  `python tools/recoil_vc5_verify.py 0x462660 --build-root
  build/vc5-verify-final-462660-tier-s` with VC5SP3 `cl` 11.00.7022,
  `/G5 /O2 /Ob0 /Zp4 /FAcs`, passes with zero unmasked byte mismatches, no
  relocation-masked bytes, BN body size 71 bytes, VC object size 80 bytes, and
  9 trailing VC NOP bytes trimmed.
- Before the dispatch overlay fix, a focused profile sweep found no better
  compiler profile. `vc5_o2_ob0_facs`, `vc5_o2_ob1_facs`,
  `vc5_o2_ob1_gx_facs`, `vc5_o2_ob2_facs`, `vc5_o2_ob2_gx_facs`,
  `vc5_o2_ob0_facs`, `vc5_o2_ob1_gx_facs`, and `vc5_o2_ob2_gx_facs` all tied
  at 35 mismatches, BN size 71, VC object size 80, and 9 trailing VC NOPs
  trimmed.
- The verification layout used for the tier S byte match keeps the raw `zFMV_Action_Vtbl` owner model for table
  construction, but uses a narrow `zFMV_ActionScalarDeletingDestructorDispatch`
  overlay at the Reset callsite. That overlay makes VC5 emit the retail
  member-call dispatch for slot 0: action in `ecx`, only the scalar-delete flag
  pushed, and a redundant post-`next` null guard retained before the indirect
  call.

## Rejected Probes

- Changing the first `zFMV_Action_Vtbl` slot to a C++ pointer-to-member
  destructor was rejected by the existing ABI guard: VC5 made the slot larger
  than the retail 4-byte vtable slot and failed
  `sizeof(zFMV_Action_Vtbl) == 0x18`.
- Adding a narrow virtual-dispatch view for the action scalar-deleting
  destructor produced the desired one-argument indirect thiscall family, but
  worsened `0x462660` from 35 to 53 mismatches. The drift shifted into branch
  lengths and loop layout, so the probe was reverted.
- Introducing a local `destroy = destroyActions` source spelling was neutral
  at 35 mismatches and was reverted.
- Introducing `register zFMV_Script *script = this` and using that owner local
  for the list fields was neutral at 35 mismatches and was reverted.
- Adding an explicit redundant `if (action != 0)` guard after loading
  `action->next` worsened `0x462660` from 35 to 61 mismatches. VC5 preserved
  the guard in a different surrounding loop shape, so the probe was reverted.
- Moving the `m_head` load into the `destroyActions` branches worsened
  `0x462660` from 35 to 41 mismatches. Although retail loads the argument
  before the head pointer, the source-order change disrupted the better VC5
  loop shape and was reverted.
