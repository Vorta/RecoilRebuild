---
name: recoil-vc5-final-build
description: Run and diagnose the RecoilRebuild VC5SP3 final candidate executable build. Use when you need to run tools/recoil.py verify final-build, inspect tools/_recoil/config/vc5_final_build.json, perform dry-run, compile-only, link, resource or PE comparison steps, diagnose final build logs under build/vc5-final, or run complete typed final-image validation.
---

# Recoil VC5 Final Build

Root `AGENTS.md` is authoritative. Use this skill for the whole-project VC5SP3
build and final typed-image comparison, not modern CMake smokes or per-owner VC5
verification.

## Build Diagnostics

```powershell
python tools/recoil.py verify final-build --dry-run
python tools/recoil.py verify final-build --compile-only --build-dir <isolated-root>
python tools/recoil.py verify final-build --build-dir <isolated-root>
```

Build output and logs are disposable diagnostics. Never patch output, reuse a
saved candidate for acceptance, or treat a successful build as an owner tier or
final result. Report the first compiler, resource, linker, MAP, or PE failure.

For ICF/COMDAT diagnosis, run both reviewed reference linker profiles in
separate isolated roots and inspect the generated object and MAP facts directly:

```powershell
python tools/recoil.py verify final-build --link-profile vc5sp3_ref_icf --build-dir <isolated-icf-root>
python tools/recoil.py verify final-build --link-profile vc5sp3_ref_noicf --build-dir <isolated-noicf-root>
```

Compare the exact decorated symbol's OBJ definition/COMDAT selection and MAP
linked identity/address in both runs. Do not use the retired `audit vc5-comdat`
receipt as evidence.

## Live Typed Coverage And Final Validation

Phase 6 begins only after the order/byte join. Run:

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

`audit final-image-catalog` derives mechanical PE facts live from verified
retail and joins accepted tracker facts for functions, variables, storage,
providers, resources, directories, padding, zero-fill, relocations, and
overlay. It builds exact file-backed and loaded-RVA interval partitions. A
legacy `binaries.recoil.final_image_catalog` blob is not required.

Gaps, overlaps, unknown extents, ambiguous padding, missing providers, and
unresolved entities are concrete blockers. Deterministic retail facts are
rederived rather than copied into a manual candidate catalog; only narrow
reviewed ambiguity annotations persist. Candidate output never supplies
expected facts.

`verify final-image` fails before building while coverage is incomplete. When
coverage is complete, it performs one fresh unrestricted build and directly
compares every typed entity. Exact headers excluding the linker-written COFF
timestamp, sections, addresses, targets, providers, directories, resources,
padding, and catalogued bytes are blocking. The timestamp and raw whole-file
difference are diagnostic only.

## Scope And Reporting

- Do not overlap a whole-project linker with another canonical/final build
  window.
- Do not mutate tracker state, source, or Binary Ninja while diagnosing unless
  separately assigned.
- Report build root, candidate/MAP paths when created, live-coverage status,
  first semantic divergence, and exact commands.
- Treat final-data output as read-only navigation, never acceptance.
