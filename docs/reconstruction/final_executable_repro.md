# Final Executable Validation

This document covers only Phase 5, `final-validation`, of
[`retail_executable_reproduction.md`](retail_executable_reproduction.md). It is
not an independent queue and never outranks `python tools/recoil.py progress next`.

## Acceptance

Final validation is a fresh, unrestricted VC5SP3 compile and link followed by a
direct semantic comparison with `support/Recoil.exe`:

```powershell
python tools/recoil.py audit provenance --strict
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py verify final-image --json
python tools/recoil.py progress audit --scope pipeline --strict
```

`verify final-image` fails closed unless
`binaries.recoil.final_image_catalog` exists in tracker schema v5 and supplies
complete, non-overlapping typed coverage for every retail section. The catalog
must describe the full selected `.text` population, aliases and ICF winners,
relocations, initialized and pointer data, BSS, padding, provider contributions,
directories, resources, imports, exports, and the overlay. It also requires a
paired linker MAP so selected `.text` identities, object providers, RVAs, and
order are checked directly.

Acceptance requires exact timestamp-excluded PE headers, section layout and
content, data directories including absence, imports, exports, resources,
relocations, mapped targets, and every catalogued byte range. There is no
stored candidate identity gate. Each run compiles current source into a new
isolated directory and compares the actual result.

The linker-written COFF timestamp is excluded from acceptance. Raw byte ranges
that differ and the raw whole-file difference are printed as diagnostics only; they
cannot turn a semantic failure into a pass or a complete semantic pass into a
failure.

Until a reviewed catalog has been populated, use the precise audit route:

```powershell
python tools/recoil.py audit final-image-catalog --json
```

The command reports the tracker field and the first missing or invalid coverage
item. Catalog population is a parent-reviewed tracker mutation; it must not be
fabricated by migration or inferred from the current candidate.

## Linked Data Diagnostics

When the current cursor selects linked data layout as a dependency, use
`python tools/recoil.py audit final-data --include-owners --strict`. This is a
read-only explanation of layout differences. It does not accept storage,
sections, owners, or the final image. Unknown extents stay unknown; never invent
a one-byte range.

## Companion DLL

`messages.dll` has its own reference and build manifest and is not part of
Recoil.exe acceptance unless the active cursor proves it is a dependency:

```powershell
python tools/recoil.py verify pe --reference support/messages.dll --manifest .agent/REFERENCE_MESSAGES_DLL.json --verify
python tools/recoil.py doctor --binary messages --quick
python tools/recoil.py verify final-build --manifest tools/_recoil/config/vc5_messages_build.json --dry-run
```

Owner-level validation and final-image validation remain separate. Neither one
silently proves the other.
