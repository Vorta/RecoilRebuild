# Final Executable Reproducibility

This note is the live runbook for the Recoil.exe final linked-binary lane and
the separate companion `messages.dll` acceptance goal. It complements
per-owner VC5 verification and data-packet evidence; it does not replace tier
gates in `AGENTS.md`.

Final executable layout is not the first workspace scheduling priority while
source-file block evidence is incomplete. `owner next --lane binary` surfaces
`work_unit=source-file-block-map` ahead of `final-repro` and
`final-data-layout`. Use this runbook when those block-order facts expose
direct object/data layout evidence, or for final executable validation after
source/object order milestones.

## Work Units

Use `python tools/recoil.py audit final-repro` as the first final executable
check. It reports:

- final-build candidate executable, map, resource, and summary availability;
- whether the last summary is only a dry run or contains executed step results;
- PE and resource comparison commands against the retail reference;
- embedded `final-data-layout` blockers when Recoil.exe linked `.data` layout
  still differs from retail.

`work_unit=final-repro` is a global binary-lane target. It is not a
SOURCE_OWNERS record. Do not add a synthetic owner for it. Human queue output
labels it `work_unit_scope=global-final-lane`.

`work_unit=final-data-layout` is the nested Recoil.exe linked `.data`
section/raw/virtual/zero-fill/map layout target. It is also not a SOURCE_OWNERS
record. Resolve it by fixing the ranked object/source-file data layout causes,
then rerun final-build/final-data checks before accepting affected owner byte
gates. It blocks final executable acceptance and directly affected owner/data
byte gates only; unrelated owner-scoped tier `S` work remains governed by that
owner's own source/data/byte readiness. This final-lane work is not a
whole-program phase gate before all owner-local byte verification.

## Commands

Recoil.exe reference preflight and dry-run command shape:

```powershell
python tools/recoil.py audit provenance --strict
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py verify final-build --dry-run
```

Live Recoil.exe status and build:

```powershell
python tools/recoil.py audit final-repro --json
python tools/recoil.py verify final-build
```

Companion `messages.dll` reference and build status are separate:

```powershell
python tools/recoil.py verify pe --reference support/messages.dll --manifest .agent/REFERENCE_MESSAGES_DLL.json --verify
python tools/recoil.py doctor --binary messages --quick
python tools/recoil.py verify final-build --manifest tools/_recoil/config/vc5_messages_build.json --dry-run
```

For linked Recoil.exe `.data` drift:

```powershell
python tools/recoil.py audit final-data --include-owners --strict --json-out build/vc5-final/final_data_diff.json --owner-actions-json build/vc5-final/final_data_owner_actions.json
```

`audit final-repro --strict` returns nonzero while final executable
reproducibility is blocked. `audit final-data --strict` returns nonzero when
section deltas are present. Treat these strict failures as evidence of blocked
final byte identity, not as tool failures by themselves.

## Live Status

Do not preserve numeric deltas or candidate-artifact freshness as current
prose. Obtain the live state from:

```powershell
python tools/recoil.py audit final-repro --json
python tools/recoil.py audit final-data --include-owners --strict --json-out build/vc5-final/final_data_diff.json --owner-actions-json build/vc5-final/final_data_owner_actions.json
```

The durable qualitative blocker is the unresolved final linked `.data`
raw/virtual/BSS boundary shift. Automatic startup/helper emission was removed
as the explanation, but the source-faithful cause of the missing raw-backed
contribution and displaced BSS boundary is not yet proven.

The next evidence requirement is a source-owner/provider fact or linker/layout
fact that explains the boundary shift source-faithfully and either proves an
initialized contribution or identifies a layout repair that genuinely moves
the initialized-data boundary. Do not substitute another broad order probe or
zero-initialized storage tweak for that evidence.

The dated experiment record, including rejected hypotheses and prior numeric
snapshots, is preserved in
[`final_executable_repro_history.md`](final_executable_repro_history.md).

## Evidence Boundary

Per-owner `verify vc5 --owner` evidence proves generated COFF bytes or data
symbols for the owner scope. It does not prove final linked executable layout,
resources, imports, exports, or PE identity.

Final-build evidence proves the whole compile/resource/link pipeline only when
the build ran without diagnostic skips and PE/resource comparison commands
passed. A dry-run `summary.json` is useful for command-shape review, but it is
not an executed final-build result.

Recoil.exe acceptance requires:

- VC5SP3 final build succeeds;
- Recoil.exe PE/import/resource/linkage comparison passes against
  `support/Recoil.exe`;
- no current final linked `.data` contradiction remains for data rows or owner
  byte gates being accepted as tier `S`.

Companion `messages.dll` acceptance independently requires its VC5SP3 build to
succeed and its resource/export/import/code/PE comparisons to pass against
`support/messages.dll`.

The aggregate Recoil release acceptance goal means both independent binary
acceptance goals pass. A failure in one binary does not turn the other's
owner-local evidence into acceptance or rejection evidence.

Owner-scoped tier `S` remains separate from final executable acceptance. A
complete linked primary source-shaped owner can be scheduled or verified when
that owner plus its primary-owned, referenced, touched, linked, and dependency
data packets are ready for the owner byte gate. Auxiliary data packets are
evidence/prerequisite scopes, not primary source-owner targets, unless current
evidence proves the original source had that exact authored data construct.
Accepted data-packet bytes mean that data dependency is byte-ready; final
source-owner `S` still targets the primary source-shaped owner plus all
referenced/touched/linked data packets. Unrelated owner/data debt and the
address-specific orphan data row `0x4e5954..0x4e5a50` do not block unrelated
source-owner `S`.
