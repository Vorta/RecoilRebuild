# Compiler And Linker Provenance

This ledger records the current compiler/linker assumptions used for source-faithful Recoil reconstruction. The machine-readable baseline is `tools/compiler_linker_profiles.json`; `tools/recoil_provenance_audit.py --strict` checks the active VC verification manifests against it.

## Role Of Each Build

- VC5SP3 `cl` 11.00.7022 is the first-pass binary-safety compiler for authored production source.
- VC6 `cl` 12.00.8168 is a fallback for functions whose original bytes, imports, MFC/CRT behavior, or a reviewed VC5SP3 attempt indicate VS98-era code generation is more plausible.
- Current executable provenance points strongly to VS97 SP3 first: VS97 SP3 `cvtres` produced 1 object, VS97 SP3 `link` 5.10.7303 linked 293 objects, and VS98-era tooling accounts for 10 objects.
- Modern MSVC/CMake builds are smoke and guard builds only. They prove compile/link/test health, not binary-safe equivalence.
- Binary-safe acceptance still requires relocation-masked COFF byte comparison or an explicitly accepted legacy listing comparison for the target function.

## Final Candidate Build Assumptions

The final candidate executable driver is `tools/vc6_final_build.json`.

- Compiler environment: `${RECOIL_VC6_ROOT}/vc6-env.cmd`
- Compiler flags: `/nologo /TP /W3 /G5 /O2 /Ob0 /MD /GX /GR /Zp4`
- Resource flags: `/r`
- Link flags: `/nologo /MACHINE:IX86 /SUBSYSTEM:WINDOWS /INCREMENTAL:NO /FIXED /BASE:0x400000`
- Runtime and provider inputs: VC6 CRT/MFC import libraries, repo-local MFC42, repo-local DirectX August 2007 x86 libraries, and the original PE facts in `.agent/REFERENCE_EXECUTABLE.json`.

These flags are reconstruction assumptions for the current final candidate driver, not proof that every original translation unit used this exact tuple. Per-function verification manifests should start from VC5SP3 profiles and may use narrower VC5SP3 or VC6 fallback profiles when Binary Ninja/original-byte evidence requires them.

## Verification Profiles

Accepted per-target verification profiles are intentionally narrow:

- `vc5_o2_ob0_facs`: first-pass VC5SP3 object-byte verification profile for plain non-EH code.
- `vc5_o2_ob1_gx_facs`: first-pass VC5SP3 profile when one-level inlining and C++ EH, constructor, destructor, or member-call cleanup shape are relevant.
- `vc5_o2_ob1_md_gx_facs`: VC5SP3 profile for functions where the original bytes use DLL CRT import-call forms such as `sprintf`.
- `vc5_o2_ob2_facs` and `vc5_o2_ob2_gx_facs`: VC5SP3 profiles for local evidence requiring aggressive inlining.
- `vc6_o2_ob0_facs`: VC6 fallback object-byte verification profile.
- `vc6_o2_ob1_gx_facs`: VC6 fallback profile for small MFC/CString code where one-level inlining and C++ EH shape are needed.
- `vc6_o2_ob1_gx_zp4_facs`: documented legacy VC6 fallback tuple preserving the active `0x415220` manifest's existing flag order until that claimed manifest is converted to `compiler_profile` form.
- `vc6_o2_oy_ob0_facs`: documented VC6 fallback profile with frame-pointer omission for existing targets whose manifests already use `/Oy`; new authored-function targets should still try VC5SP3 first.
- `vc6_o2_ob2_gx_facs`: VC6 fallback profile where aggressive inlining with C++ EH matches current evidence.
- `vc6_zsys_cpu_raw_asm`: documented exception for approved zSys CPU raw-assembly probes.

New VC verification manifests should use `"compiler_profile": "<profile-name>"`.
Adding a new compiler environment or flag tuple requires updating this ledger
and `tools/compiler_linker_profiles.json` with the reason and expected compiler
version. Do not silently add one-off flags to a VC manifest.

## Required Checks

Run these before relying on compiler/linker provenance:

```powershell
python tools/recoil_provenance_audit.py --strict
python tools/recoil_vc6_manifest_source_guard.py
python tools/recoil_vc6_verify.py --all --skip-bn-compare
```

`python tools/recoil_doctor.py --quick` also runs the provenance audit.
