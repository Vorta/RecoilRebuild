# zSnd Sample Init Verification Notes

These notes track binary-lane evidence for the `zSndSample::InitFromWaveData`
backend cluster. Binary Ninja and VC verifier artifacts remain authoritative.

## 0x4a2ec0 zSndSample::InitFromWaveData_A3D

- Current source: `src/GameZRecoil/zSound/zsnd_create.cpp`.
- Current VC target:
  `tools/vc6_verify_targets/zsnd_sample_init_from_wave_data_a3d.json`.
- Current tier S verification result:
  `python tools/recoil_vc6_verify.py 0x4a2ec0` with VC5SP3
  `cl` 11.00.7022, `/MD /G5 /O2 /Ob0 /Zp4 /FAcs`, passes with zero unmasked
  byte mismatches after 48 relocation-masked bytes. BN body size is 691 bytes,
  VC object size is 704 bytes, and 13 trailing VC NOP bytes are trimmed.
- The accepted profile is `vc5_o2_ob0_md_facs`. The decisive difference from
  the previous static-CRT profile is CRT import shape: retail loads the CRT
  allocation import pointer into `edi` for marker-array allocation, matching
  VC5SP3 `/MD`.
- Current evidence command:
  `python tools/recoil_vc6_verify.py 0x4a2ec0 --build-root build/vc6-verify-final-4a2ec0-tier-s`.

## 0x4a3180 zSndSample::InitFromWaveData_DirectSound

- Current source: `src/GameZRecoil/zSound/zsnd_create.cpp`.
- Current VC target:
  `tools/vc6_verify_targets/zsnd_sample_init_from_wave_data_directsound.json`.
- Current verification result:
  `python tools/recoil_vc6_verify.py 0x4a3180` with VC6 `cl` 12.00.8168,
  `/G5 /O2 /Ob0 /Zp4 /FAcs`, fails with 543 unmasked byte mismatches after
  76 relocation-masked bytes. BN body size is 852 bytes, VC object size is
  848 bytes, and 2 trailing VC NOP bytes are trimmed.
- The retained DirectSound descriptor setup uses unsigned-byte predicates after
  explicit shifts for the replay flag tests. This restores the retail
  shift/test shape for the descriptor flag block, and moving the format-pointer
  store after the first flag branch now more closely matches retail descriptor
  setup. Descriptor local setup still differs in register choice and scheduling
  before `CreateSoundBuffer`.

## Profile Sweep

- A focused 0x4a2ec0 profile sweep before the accepted `/MD` profile update found
  `vc5_o2_ob0_facs`, `vc5_o2_ob1_gx_facs`, and `vc5_o2_ob2_gx_facs` tied at
  298 mismatches, 56 relocation-masked bytes, BN size 691, VC object size 688,
  and 4 trailing VC NOPs trimmed.
- `vc6_o2_ob0_facs` and `vc6_o2_ob1_gx_facs` both worsened 0x4a2ec0 to 586
  mismatches, 56 relocation-masked bytes, BN size 691, VC object size 704, and
  7 trailing VC NOPs trimmed.
- Retesting the current source with the closest VC5SP3 `/MD` profiles proved
  `vc5_o2_ob0_md_facs` and `vc5_o2_ob1_md_gx_facs` both pass 0x4a2ec0 with
  zero unmasked byte mismatches after 48 relocation-masked bytes. The manifest
  now uses the simpler non-EH `vc5_o2_ob0_md_facs` profile.
- A focused 0x4a3180 profile sweep against the current source confirmed the
  existing VC6 family profile is still the best choice. `vc6_o2_ob0_facs`,
  `vc6_o2_ob1_gx_facs`, `vc6_o2_ob2_gx_facs`, and `vc6_o2_oy_ob0_facs` all
  tied at 543 mismatches, 76 relocation-masked bytes, BN size 852, VC object
  size 848, and 2 trailing VC NOPs trimmed. VC5SP3 static-CRT profiles
  `vc5_o2_ob0_facs`, `vc5_o2_ob1_facs`, `vc5_o2_ob1_gx_facs`,
  `vc5_o2_ob2_facs`, and `vc5_o2_ob2_gx_facs` all reported 694 mismatches.
  VC5SP3 `/MD` profiles were also worse: `vc5_o2_ob0_md_facs` and
  `vc5_o2_ob1_md_gx_facs` both reported 734 mismatches, 68 relocation-masked
  bytes, BN size 852, VC object size 864, and 11 trailing VC NOPs trimmed.

## Rejected Probes

- Rewriting the 0x4a2ec0 spatial predicate as an unsigned shifted local plus an
  unsigned-byte test improved the A3D target from 298 to 235 mismatches.
  Moving the provider-source reload into each spatial branch then improved the
  target to 220 mismatches and is retained. This source shape reaches byte
  alignment through the post-`Rewind` spatial provider calls.
- Rewriting the 0x4a2ec0 spatial predicate as an unsigned shifted local and
  moving the provider-source reload inside both branches worsened the A3D
  target from 298 to 308 mismatches. The probe was reverted.
- Masking the shifted 0x4a2ec0 spatial predicate into the local
  (`spatialFlags &= 1; if (spatialFlags != 0)`) was neutral at 298 mismatches.
  It did not restore the retail `shr` / `test al, 1` sequence, so the probe was
  reverted.
- Spelling the 0x4a2ec0 shifted predicate as unsigned division by four
  (`spatialFlags = (unsigned int)(spatialFlags) / 4`) was also neutral at 298
  mismatches. The source was restored to the clearer right-shift form.
- Making the 0x4a2ec0 shifted predicate local `volatile` forced extra stack
  traffic and worsened the target from 298 to 341 mismatches, with the VC5
  object growing to 704 bytes and 3 trailing VC NOPs trimmed. The probe was
  reverted.
- Introducing a local allocator function pointer for the 0x4a2ec0 marker
  allocation block was neutral under the static-CRT profile: VC5 optimized it
  back to direct `_malloc` calls and stayed at 220 unmasked mismatches. The
  source probe was reverted; the accepted solution is the `/MD` verification
  profile, not a production source change.
- Reusing a single `error` local for 0x4a3180 `CreateSoundBuffer` and later
  DirectSound calls was neutral at 691 mismatches. VC6 still kept the create
  error in `ebp` instead of matching BN's stack-stored result, so the existing
  `createError` source shape was restored.
- Introducing an `initResult = 0` local for the 0x4a3180 early `createGuard`
  return was neutral at 691 mismatches. VC6 still delayed `xor eax, eax` until
  after the first saved-register pops, so the direct `return 0` source shape
  was restored.
- Moving the 0x4a3180 `fmt` local initialization down from the descriptor
  setup prelude to immediately before `desc.lpwfxFormat = fmt` worsened the
  DirectSound target from 691 to 708 mismatches. The probe was reverted.
- Rewriting the 0x4a3180 descriptor replay-flag tests as unsigned shifted
  locals plus unsigned-byte tests improved the DirectSound target from 691 to
  581 mismatches and is retained.
- Moving the 0x4a3180 `desc.lpwfxFormat = fmt` store until after the first
  replay-flag branch improved the DirectSound target from 581 to 543
  mismatches and is retained. Removing `const` from the DirectSound `fmt` local
  and adding a `register` hint to that local were byte-neutral at 543
  mismatches, so the original local spelling was kept.
- Moving the retained 0x4a3180 `desc.lpwfxFormat = fmt` store back before
  `desc.dwSize` regressed the DirectSound target from 543 to 581 mismatches.
  The delayed store remains the accepted source shape.
- Making the 0x4a3180 `createError` local `volatile` forced a stack-resident
  create result, but it expanded the VC frame and worsened the target from 543
  to 668 mismatches. The probe was reverted.
- Rewriting the 0x4a3180 `CreateSoundBuffer` assignment into the `if`
  condition was byte-neutral at 543 mismatches. The clearer initialized-local
  source shape was restored.
- Splitting the 0x4a3180 wave-format local into the retained descriptor `fmt`
  and a reloaded marker-loop `markerFmt` shortened the source live range but
  worsened the target from 543 to 619 mismatches. Retail keeps the wave-format
  pointer live through the marker loop, so the single `fmt` local remains the
  accepted source shape.
