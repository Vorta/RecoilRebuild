# Final Executable Reproducibility

This note tracks the final linked binary lane. It complements per-owner VC5
verification and data-owner evidence; it does not replace tier gates in
`AGENTS.md`.

## Work Units

Use `python tools/recoil.py audit final-repro` as the first final executable
check. It reports:

- final-build candidate executable, map, resource, and summary availability;
- whether the last summary is only a dry run or contains executed step results;
- PE and resource comparison commands against the retail reference;
- embedded `final-data-layout` blockers when Recoil.exe linked `.data` layout
  still differs from retail.

`work_unit=final-repro` is a global binary-lane target. It is not a
SOURCE_OWNERS record. Do not add a synthetic owner for it.

`work_unit=final-data-layout` is the nested Recoil.exe linked `.data`
section/raw/virtual/zero-fill/map layout target. It is also not a SOURCE_OWNERS
record. Resolve it by fixing the ranked object/source-file data layout causes,
then rerun final-build/final-data checks before accepting affected owner byte
gates.

## Commands

Preflight and dry-run command shape:

```powershell
python tools/recoil.py audit provenance --strict
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py verify pe --reference support/messages.dll --manifest .agent/REFERENCE_MESSAGES_DLL.json --verify
python tools/recoil.py verify final-build --dry-run
```

Current final executable lane:

```powershell
python tools/recoil.py audit final-repro --json
python tools/recoil.py verify final-build
```

For linked Recoil.exe `.data` drift:

```powershell
python tools/recoil.py audit final-data --include-plan --strict --json-out build/vc5-final/final_data_diff.json --plan-actions-json build/vc5-final/final_data_plan_actions.json
```

`audit final-repro --strict` returns nonzero while final executable
reproducibility is blocked. `audit final-data --strict` returns nonzero when
section deltas are present. Treat these strict failures as evidence of blocked
final byte identity, not as tool failures by themselves.

## Evidence Boundary

Per-owner `verify vc5 --owner` evidence proves generated COFF bytes or data
symbols for the owner scope. It does not prove final linked executable layout,
resources, imports, exports, or PE identity.

Final-build evidence proves the whole compile/resource/link pipeline only when
the build ran without diagnostic skips and PE/resource comparison commands
passed. A dry-run `summary.json` is useful for command-shape review, but it is
not an executed final-build result.

Final executable acceptance requires:

- VC5SP3 final build succeeds;
- Recoil.exe PE/import/resource/linkage comparison passes against
  `support/Recoil.exe`;
- messages.dll resource/export/import/PE comparison passes against
  `support/messages.dll`;
- no current final linked `.data` contradiction remains for data rows or owner
  byte gates being accepted as tier `S`.
