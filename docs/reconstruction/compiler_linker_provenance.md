# Compiler And Linker Provenance

This ledger records the current compiler/linker assumptions used for source-faithful Recoil reconstruction. The machine-readable baseline is `tools/compiler_linker_profiles.json`; `tools/recoil_provenance_audit.py --strict` checks the active VC verification manifests against it.

## Role Of Each Build

- VC6 `cl` 12.00.8168 is the default binary-safety compiler for authored production source.
- VC5SP3 `cl` 11.00.7022 is an explicit exception for functions whose original bytes currently match VC5-era code generation better than the default VC6 profile.
- Modern MSVC/CMake builds are smoke and guard builds only. They prove compile/link/test health, not binary-safe equivalence.
- Binary-safe acceptance still requires relocation-masked COFF byte comparison or an explicitly accepted legacy listing comparison for the target function.

## Final Candidate Build Assumptions

The final candidate executable driver is `tools/vc6_final_build.json`.

- Compiler environment: `${RECOIL_VC6_ROOT}/vc6-env.cmd`
- Compiler flags: `/nologo /TP /W3 /G5 /O2 /Ob0 /MD /GX /GR /Zp4`
- Resource flags: `/r`
- Link flags: `/nologo /MACHINE:IX86 /SUBSYSTEM:WINDOWS /INCREMENTAL:NO /FIXED /BASE:0x400000`
- Runtime and provider inputs: VC6 CRT/MFC import libraries, repo-local MFC42, repo-local DirectX August 2007 x86 libraries, and the original PE facts in `.agent/REFERENCE_EXECUTABLE.json`.

These flags are reconstruction assumptions, not proof that every original translation unit used this exact tuple. Per-function verification manifests may use narrower flag profiles when Binary Ninja/original-byte evidence requires them.

## Verification Profiles

Accepted per-target verification profiles are intentionally narrow:

- `vc6_o2_ob0_facs`: default VC6 object-byte verification profile.
- `vc6_o2_ob1_gx_facs`: VC6 profile for small MFC/CString code where one-level inlining and C++ EH shape are needed.
- `vc6_o2_ob2_gx_facs`: VC6 focused profile where aggressive inlining with C++ EH matches current evidence.
- `vc6_zsys_cpu_raw_asm`: documented exception for approved zSys CPU raw-assembly probes.
- `vc5_o2_ob0_facs`, `vc5_o2_ob1_gx_facs`, `vc5_o2_ob2_facs`, and `vc5_o2_ob2_gx_facs`: VC5SP3 exception profiles for targets whose evidence currently requires VC5SP3 codegen.

Adding a new compiler environment or flag tuple requires updating this ledger and `tools/compiler_linker_profiles.json` with the reason and expected compiler version. Do not silently add one-off flags to a VC manifest.

## Required Checks

Run these before relying on compiler/linker provenance:

```powershell
python tools/recoil_provenance_audit.py --strict
python tools/recoil_vc6_manifest_source_guard.py
python tools/recoil_vc6_verify.py --all --skip-bn-compare
```

`python tools/recoil_doctor.py --quick` also runs the provenance audit.
