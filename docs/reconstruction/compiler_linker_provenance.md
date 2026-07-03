# Compiler And Linker Provenance

This ledger records the current compiler/linker assumptions used for source-faithful Recoil reconstruction. The machine-readable baseline is `tools/_recoil/config/compiler_linker_profiles.json`; `python tools/recoil.py audit provenance --strict` checks the active VC verification manifests against it.

## Role Of Each Build

- VC5SP3 `cl` 11.00.7022 is the only tier `S` compiler for authored production source.
- Current executable provenance points to VS97 SP3: VS97 SP3 `cvtres` produced 1 object and VS97 SP3 `link` 5.10.7303 linked the executable.
- Modern MSVC/CMake builds are smoke and guard builds only. They prove compile/link/test health, not tier `S` equivalence.
- Tier `S` acceptance normally requires relocation-masked COFF byte comparison. VC5 manifests can also compare generated `.data`/`.rdata` data symbols for data-gate evidence. Listings, normalized text diffs, and relocation identity reports are triage/review aids, not source-shape acceptance modes.

## Final Candidate Build Assumptions

The final candidate executable driver is `tools/_recoil/config/vc5_final_build.json`.

- Compiler environment: `D:\Recoil Project\Compiler\VC5SP3\vc5sp3-env.cmd`
- Compiler flags: `/nologo /TP /W3 /G5 /O2 /Ob0 /MD /GX /Zp4`
- Resource flags: `/r`
- Link flags: `/nologo /MACHINE:IX86 /SUBSYSTEM:WINDOWS /INCREMENTAL:NO /FIXED /BASE:0x400000`
- Runtime and provider inputs: VC5SP3 CRT/MFC import libraries, repo-local MFC42, repo-local DirectX 6 libraries, and the original PE facts in `.agent/REFERENCE_EXECUTABLE.json`.

These flags are reconstruction assumptions for the current final candidate driver, not proof that every original translation unit used this exact tuple. Per-function verification manifests must use documented VC5SP3 profiles only.

## Verification Profiles

Accepted per-target verification profiles are intentionally narrow:

- `vc5_o2_ob0_facs`: VC5SP3 object-byte verification profile for plain non-EH code.
- `vc5_o2_ob1_gx_facs`: VC5SP3 profile when one-level inlining and C++ EH, constructor, destructor, or member-call cleanup shape are relevant.
- `vc5_o2_ob1_facs`: VC5SP3 one-level inlining profile without C++ EH for plain leaf/helper code whose local evidence points to `/Ob1`.
- `vc5_o2_ob1_gx_uintptr_facs`: VC5SP3 one-level inlining profile with a `UINT_PTR` compatibility define for focused full-TU MFC frame targets whose production headers otherwise require a later SDK typedef.
- `vc5_o2_ob1_md_gx_facs`: VC5SP3 profile for functions where the original bytes use DLL CRT import-call forms such as `sprintf`.
- `vc5_o2_ob2_facs` and `vc5_o2_ob2_gx_facs`: VC5SP3 profiles for local evidence requiring aggressive inlining.
- `vc5_o2_oy_ob0_facs`: documented VC5SP3 profile with frame-pointer omission for targets whose evidence requires `/Oy`.
- `vc5_zsys_cpu_raw_asm`: documented exception for approved zSys CPU raw-assembly probes.
- `vc5_o2_ob0_md_zrndr_mmx_raw_asm_facs`: documented user-approved exception for the zRndr overlay RGB555/RGB565 MMX row leaves, enabled only with `RECOIL_ENABLE_ZRNDR_OVERLAY_MMX_RAW_ASM`. This permits narrow inline `__asm` MMX loops inside ordinary C++ functions only; it does not permit `__declspec(naked)`, `_emit`, `.asm`, raw byte emission, unrelated zRndr span/MMX families, provider shims, or future raw assembly.
- `vc5_o2_ob0_md_zrndr_span_mmx_raw_asm_facs`: documented user-approved exception for zRndr span callbacks where current BN proves authored MMX blocks: `0x49ea80`, `0x49ec20`, `0x49e400`, `0x49e560`, `0x49cbb0`, `0x49cea0`, `0x49da80`, and `0x49ddb0`. Enabled only with `RECOIL_ENABLE_ZRNDR_SPAN_MMX_RAW_ASM`. This permits inline `__asm` only for the necessary MMX blocks while C++ keeps the function shell, setup, scalar edge/tail logic, scratch-buffer preparation, and portable fallback. It does not permit whole-function raw assembly, `__declspec(naked)`, `_emit`, `.asm`, raw byte emission, provider shims, or non-MMX blocks.
- `vc5_o2_ob0_md_zrndr_esp_pivot_raw_asm_facs`: documented user-approved exception for the five zRndr ESP-pivot span leaves at `0x49b7e0`, `0x49bbf0`, `0x49e6c0`, `0x49edc0`, and `0x49f180`, enabled only with `RECOIL_ENABLE_ZRNDR_ESP_PIVOT_RAW_ASM`. This permits narrow inline `__asm` loops inside ordinary C++ functions only; it does not permit `__declspec(naked)`, `_emit`, `.asm`, raw byte emission, the user-approved span-MMX block family, provider shims, or future raw assembly.

New VC verification manifests should use `"compiler_profile": "<profile-name>"`.
Adding a new compiler environment or flag tuple requires updating this ledger
and `tools/_recoil/config/compiler_linker_profiles.json` with the reason and expected compiler
version. Do not silently add one-off flags to a VC manifest.

## Profile Sentinel Guards

Already byte-matched functions are compiler-profile sentinels. If a candidate
profile does not preserve those confirmed bytes for the same source block, that
profile is disqualified for the block; do not keep using it merely because a
later, still-unmatched function has a lower mismatch count. When a manifest has
`profile_guard.accepted_profiles`, that list is the enforced remaining valid
profile set for the manifest. `python tools/recoil.py verify vc5` rejects
explicit `--compiler-profile` use of profiles outside the accepted set or inside
`profile_guard.disqualified_profiles`, and skips them in `--profile-sweep` by
default. `--allow-disqualified-profile` is only for an explicit provenance
re-open with new sentinel evidence.

When a newly byte-matched function rules out another profile, update the same
manifest guard immediately: remove that profile from
`profile_guard.accepted_profiles`, add it to
`profile_guard.disqualified_profiles`, and record the sentinel address plus the
mismatch or compile-failure evidence. This keeps future byte-matching sweeps
restricted to the current viable profile set.

Current confirmed guards:

- `src/Battlesport/about.cpp`: the five-function
  `cabout_prelude_functions` sentinel (`0x401000`, `0x401020`,
  `0x401030`, `0x401040`, `0x401050`) byte-matches with
  `vc5_o2_ob0_md_facs`. The current accepted profile set is
  `vc5_o2_ob0_md_facs`, `vc5_o2_ob0_md_gx_gr_facs`,
  `vc5_o2_ob1_md_gx_facs`, `vc5_o2_ob1_md_facs`,
  `vc5_o2_ob2_md_facs`, `vc5_o2_ob2_md_gx_facs`, and
  `vc5_o2_oy_ob0_md_facs`. The 2026-07-04 sweep disqualified the non-`/MD`
  MFC-incompatible profiles, the explicit `/Oy-` profiles, and the `/O1`
  diagnostic profile for the About block.
- `src/Battlesport/ai_net.cpp`: `0x401060`
  `AINet::TickAiMode2TopLevel` is a byte-matched sentinel. The 2026-07-04
  sweep disqualified the non-`/MD` MFC-incompatible profiles, the `/Ob2`
  `/MD` profiles, and the `/O1` diagnostic profile for the ai_net block. The
  current accepted profile set is `vc5_o2_ob0_md_facs`,
  `vc5_o2_ob0_md_gx_gr_facs`, `vc5_o2_ob1_md_gx_facs`,
  `vc5_o2_ob1_md_facs`, `vc5_o2_oy_ob0_md_facs`,
  `vc5_o2_oyminus_ob0_md_zi_facs`, `vc5_o2_oyminus_ob0_md_z7_facs`,
  `vc5_o2_oyminus_ob0_md_gx_facs`, `vc5_o2_oyminus_ob0_md_facs`,
  `vc5_o2_oyminus_ob1_md_facs`, and
  `vc5_o2_oyminus_ob0_md_nog5_facs`. Prefer the manifest default and repair
  source/header/include shape unless new sentinels prove a different profile.

VC verification manifests may contain `functions`, `data_symbols`, or both.
`data_symbols` entries compare a BN data address and byte length against a VC5
COFF symbol with relocation masking, and write a relocation identity report for
pointer/symbol review. Passing data-symbol bytes can support initialized-data
evidence only after the normal source-owner and complete data-gate criteria are
met. The compared symbol must cover the complete touched authored data owner or
the complete initialized-global data set being accepted; a passing field-sized
slice inside a larger authored global is not enough for `Data reimplemented ✅`.

## Required Checks

Run these before relying on compiler/linker provenance:

```powershell
python tools/recoil.py audit provenance --strict
python tools/recoil.py guard vc5-manifest
python tools/recoil.py verify vc5 --all --skip-bn-compare
```

`python tools/recoil.py doctor --quick` also runs the provenance audit.
