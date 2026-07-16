# Final Executable Reproducibility

This document covers only Phase 5, `final-validation`, of
[`retail_executable_reproduction.md`](retail_executable_reproduction.md). It is
not an independent queue and never outranks `python tools/recoil.py progress next`.
Before Phase 5, these commands are diagnostics only when the current text cursor
requires compile/link evidence.

## Acceptance

Recoil.exe is accepted only when one unrestricted synchronized VC5SP3 build
passes every required authored/full function-order, authored/linked-byte and
linked-address, mandatory whole
output-section/storage/data, provider/import, resource, PE, candidate-size,
and whole-file SHA-256 check, with an accepted exact final-repro receipt,
against `support/Recoil.exe`. Diagnostic skips, normalized comparisons, stale
maps, post-link patching, or dry-run summaries never qualify.

Run Phase 5 with:

```powershell
python tools/recoil.py audit provenance --strict
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py verify final-build
python tools/recoil.py audit final-repro --strict --output build/vc5-final/final_repro.json
python tools/recoil.py progress evidence import-final-repro --report build/vc5-final/final_repro.json --expected-revision <revision> --dry-run
python tools/recoil.py progress audit --scope pipeline --strict
```

`audit final-repro --strict` may return nonzero because the candidate is not
identical; that is reconstruction evidence, not automatically a tool failure.
The final-build receipt must bind the canonical build configuration, effective
required order targets, toolchain/compiler context, source/object/map/resource/
candidate identities, per-report identities, and executed run identity. Those
identities remain machine-managed SHA-256 metadata inside the receipt; agents
cite the receipt/evidence path or imported evidence id rather than copying
digests unless diagnosing an integrity mismatch.
`audit final-repro` and its import record observed evidence only; neither is a
work unit, scheduler, or acceptance operation.

Reports describe the current candidate by default. A receipt for a superseded
candidate or probe must set top-level `"observation_scope": "historical"`
before import. The importer records that receipt as `historical-hash-bound`,
still verifies and content-addresses every present artifact, and never updates
current section candidates, final-repro state, blockers, owners, work items, or
pipeline state. Omitting the field means `current`; do not relabel a stale
report as current merely to populate normalized state.

## Linked Data Diagnostics

When Phase 5 or the current text cursor explicitly selects linked `.data`
layout as a required dependency, use:

```powershell
python tools/recoil.py audit final-data --include-owners --strict --json-out build/vc5-final/final_data_diff.json
python tools/recoil.py progress evidence import-final-data --report build/vc5-final/final_data_diff.json --expected-revision <revision> --dry-run
python tools/recoil.py progress output-section show recoil:section:.data
```

This report diagnoses raw/virtual/zero-fill/map placement and identifies
non-authoritative correlations to data rows or owner byte gates. It generates
no owner-action batch and accepts nothing. Inspect physical contributions with
`progress storage show`; accept storage and section dimensions only through
explicit dry-run-first `progress accept storage` and `progress accept section`
operations. Data symbols, owner data gates, storage contributions, output
sections, and final-image acceptance remain distinct. Unknown extents have no
guessed size or end. The diagnostic is otherwise deferred; it
does not create a peer scheduler, authorize unrelated owner work, or establish
source shape. Dated rejected hypotheses and numeric snapshots belong in
[`final_executable_repro_history.md`](final_executable_repro_history.md), not
in this live mechanics note.

## Companion DLL

`messages.dll` has an independent reference and validation path. It is not part
of Recoil.exe whole-file acceptance and is non-gating unless the user requests
it or the active Recoil.exe cursor proves it is a required dependency:

```powershell
python tools/recoil.py verify pe --reference support/messages.dll --manifest .agent/REFERENCE_MESSAGES_DLL.json --verify
python tools/recoil.py doctor --binary messages --quick
python tools/recoil.py verify final-build --manifest tools/_recoil/config/vc5_messages_build.json --dry-run
```

Per-owner VC5 evidence and data-symbol evidence remain owner acceptance inputs;
they do not prove final linked executable identity. Conversely, final-build
success does not independently prove an authored owner boundary, source model,
or owner tier.
