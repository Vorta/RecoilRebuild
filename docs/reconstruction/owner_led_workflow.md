# Owner-Led Reconstruction Workflow

Function addresses are evidence anchors. The accepted primary work unit is the
proven source-shaped owner: class/interface, source-file cluster, subsystem,
authored callback/record/table/global object/static class-member group,
provider boundary, or true standalone leaf.

Ordinary initialized-global, literal, constant, or storage-range groupings are
auxiliary source-owned data packets. They exist to carry data prerequisites,
data-gate evidence, and byte-readiness for a primary owner; they are not
original source constructs or primary tier `S` source-owner targets unless
current evidence proves the original source had that exact authored data
construct. Link those packets upward to the primary source-shaped owner and
treat orphan packets as parent-reconciliation blockers.

Use `.agent/SOURCE_OWNERS.json` as the durable owner-scope ledger and edit it
only through:

```powershell
python tools/recoil.py owner show <owner-id-or-address>
python tools/recoil.py owner find <query>
python tools/recoil.py owner plan [owner-id-or-address] --binary recoil --json
python tools/recoil.py owner add --id <section.owner> --kind <kind> --section <section> --name "..." --anchor 0xNNNNNN --evidence "..."
python tools/recoil.py owner link-address <owner-id> 0xNNNNNN
python tools/recoil.py owner link-data <owner-id> 0xNNNNNN --name <symbol>
python tools/recoil.py owner set-gate <owner-id> <boundary|source|data|functional|linkage|byte> <pending|accepted|blocked|none|deferred> --evidence "..."
python tools/recoil.py owner set-gates <owner-id> boundary=accepted source=accepted data=none --evidence "..."
python tools/recoil.py owner set-tier <owner-id> <X|C|B|A|S> --evidence "..." --dry-run
python tools/recoil.py owner set-tier-batch <owner-a> <owner-b> <X|C|B|A|S> --evidence "..." --dry-run
python tools/recoil.py owner audit --strict
python tools/recoil.py owner relationships <owner-id-or-address> --json
python tools/recoil.py owner migrate-linkage-gate --dry-run --json
python tools/recoil.py owner next --lane binary
python tools/recoil.py verify vc5 --owner <owner-id-or-address> --auto-chunk
```

`owner next --lane binary` can also report non-ledger work units before source
owners. `work_unit=source-file-block-map` is the top Recoil.exe scheduling
priority while physical translation-unit boundaries and header/COMDAT/provider
exceptions remain incomplete; it is not a SOURCE_OWNERS record and should drive
block-level source-owner correction before final-data layout work.

`work_unit=final-repro` is the final executable
reproducibility lane: final-build artifacts, executed summary state,
PE/resource comparison commands, and embedded final-data blockers.
`work_unit=final-data-layout` means the final linked `Recoil.exe` `.data`
section/raw/virtual/zero-fill or map placement still contradicts retail. Both
are first-class binary-lane targets, but neither is a source owner. Resolve
them by fixing the final-build or ranked object/source-file layout blockers,
not by adding synthetic owner records. They block final executable acceptance
and directly affected owner/data byte gates only. Human queue output labels
these rows `work_unit_scope=global-final-lane` so they are not confused with
owner-local tier `S` candidates.

Owner links have a normalized relationship model. Schema version 1 ledgers are
derived from `anchors`, `member_addresses`, `data_addresses`, and
`dependencies`; schema version 2 stores explicit `relationships` while keeping
those legacy fields as compatibility mirrors. Schema version 3 makes
`SOURCE_OWNERS` authoritative for each owner's target binary and
`Reimplemented [X/C/B/A/S]` tier. Use `owner relationships` to distinguish
anchor-only addresses, primary function/data ownership, and owner-to-owner
dependencies before changing gates or owner tiers. Use
`owner migrate-relationships --dry-run --json` to inspect a v2 conversion when
legacy relationship mirrors need normalization. Use
`owner migrate-linkage-gate --dry-run --json` to inspect the controlled
linkage-gate migration. Schema version 3 is already the authority model; do not
use plan-authority migration commands.

Use `owner plan` for the concise authored planning surface. It reads
`SOURCE_OWNERS` (`--binary recoil|messages`, default `recoil`) and renders
owner id/name/kind/section/state, gates, the authoritative owner
`Reimplemented [X/C/B/A/S]` tier, primary child function addresses, primary
child data addresses, conservative address metadata, source paths,
dependencies, and blocker text. The owner ledger intentionally does not carry
data child size/type metadata; Binary Ninja remains the source for that shape.
JSON mode is available for parent scheduling:

```powershell
python tools/recoil.py owner plan --limit 20
python tools/recoil.py owner plan <owner-id-or-address> --json
python tools/recoil.py owner plan --binary messages --limit 20
```

Provider-boundary owners render as provider boundaries rather than authored
Reimplemented tiers. Authored owner `S` requires the owner `byte` gate plus all
primary child evidence at `S`; `A` requires source/data/linkage gates plus
near-byte evidence; `B` requires source/data gates and `linkage=accepted` or
`linkage=none`; `C` requires functional evidence; otherwise the owner remains
`Reimplemented [X]`. Use `owner set-gates`, `owner set-tier`, and
`owner set-tier-batch` for durable owner acceptance.

Primary function and primary data children are unique across the owner ledger.
`owner add`, `owner link-address`, `owner link-address-batch`,
`owner link-data`, and `owner link-data-batch` reject a primary child address
already owned by another owner and tell the operator to unlink or move it
first. Repeating a same-owner link remains a no-op, and same-owner data links
may still update the data name.

For tier `S` byte-gate work, run VC5 verification at primary source-owner scope
with `verify vc5 --owner`. The command resolves the owner id or linked child
address, requires every linked authored function/data row to have exactly one
VC5 manifest item, and reports missing coverage before compiling. Explicit
target/address VC5 selectors are for diagnostics or manifest development unless
current evidence proves a true standalone owner. For auxiliary data packets,
accepted byte evidence means the data dependency is byte-ready; final source
owner `S` still targets the primary source-shaped owner plus every
referenced/touched/linked data packet.

The progression is per source owner. Owner/source shape and linkage define the
unit; each owner advances through `C` behavior evidence, source/data/linkage
acceptance, and then owner-scoped `S` byte evidence when that owner's own
dependencies are ready. Do not apply a whole-program phase rule that requires
all functional work to finish before all linkage work, or all linkage work to
finish before any owner-local byte work. `owner next` and `audit backlog`
human output use `work_unit_scope=owner-local-tier-s-candidate` for ready
owner-scoped byte candidates; unrelated owner/data debt and global final-lane
work do not block those candidates unless the output says that specific owner
or data byte gate is directly affected.

Owner gate meanings:

- `boundary`: BN/source evidence has identified the full owner extent.
- `source`: source-faithful owner model is implemented, not an address slice or scaffold.
- `data`: all touched authored `.data`, `.rdata`, and BSS owner facts are reconstructed.
- `functional`: current behavior evidence exists at the owner or accepted target level.
- `linkage`: relationship-level owner correctness is accepted. `linkage=accepted` means primary function/data addresses are unique, dependency owner ids exist, relationship mirrors are consistent, accepted non-standalone owners are not anchor-only, and auxiliary data packets have a source-shaped parent dependency. `linkage=none` is for provider-boundary owners or true dependency-free standalone owners. Do not use `fully_linked` here; reserve that wording for whole-executable/link-output status.
- `byte`: tier `S` owner-level byte/provider ABI evidence. For function/code owners this may be `deferred` until the complete primary source-shaped owner plus primary-owned, referenced, touched, linked, and dependency data packets are ready for that owner's byte gate; when tier `S` is in scope, `python tools/recoil.py verify vc5 --owner <owner-id-or-address> --auto-chunk` is the default verification command. Unrelated owner/data debt and orphan data such as `0x4e5954` do not block unrelated source-owner `S`. For auxiliary data packets it is accepted when the data-symbol byte and relocation identity evidence is accepted, but that is data dependency byte-readiness, not parent/source-owner tier `S` completion.

`SOURCE_OWNERS` is the only durable acceptance ledger. `Source owner ✅` maps
to accepted owner `boundary` and `source` gates, `Data reimplemented ✅/❎`
maps to accepted or none owner `data`, and `Reimplemented [C/B/A/S]` maps to
the owner's authoritative `reimplementation.tier`.

The only approved orphan initialized-data exception is the one row
`0x4e5954..0x4e5a50` (`g_zInterp_UnresolvedFloatDefaults`). It remains authored
initialized data, not provider, filler, or padding, and its accepted model is
`data-equivalent-only`, never `source-faithful`. Accept it only with exact BN
range/type/byte evidence, no base or interior xrefs, provider rejection,
adjacent-owner exclusion, emitted production data, and VC5 data-symbol byte
evidence. This exception is address-specific and cannot be reused for another
row without separate user approval.

Owner-tracked data globals are primary-data relationships on source owners.
They normally model auxiliary data packets linked to a primary source-shaped
owner, not independent source-owner targets. Do not add `.rdata` pseudo-rows or
member rows inside a larger global. Use `owner link-data`,
`owner link-data-batch`, and owner address metadata for conservative name,
source path, target, and group hints; do not duplicate Binary Ninja size/type
metadata in the owner ledger.
