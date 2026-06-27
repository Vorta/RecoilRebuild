# Owner-Led Reconstruction Workflow

Function addresses are evidence anchors. The accepted work unit is the proven
source owner: class/interface, source-file cluster, subsystem, callback/data
record, initialized-global owner, provider boundary, or true standalone leaf.

Use `.agent/SOURCE_OWNERS.json` as the durable owner-scope ledger and edit it
only through:

```powershell
python tools/recoil.py owner show <owner-id-or-address>
python tools/recoil.py owner find <query>
python tools/recoil.py owner add --id <section.owner> --kind <kind> --section <section> --name "..." --anchor 0xNNNNNN --evidence "..."
python tools/recoil.py owner link-address <owner-id> 0xNNNNNN
python tools/recoil.py owner link-data <owner-id> 0xNNNNNN --name <symbol>
python tools/recoil.py owner set-gate <owner-id> <boundary|source|data|functional|byte> <pending|accepted|blocked|none|deferred> --evidence "..."
python tools/recoil.py owner set-gates <owner-id> boundary=accepted source=accepted data=none --evidence "..."
python tools/recoil.py owner audit --strict
python tools/recoil.py owner relationships <owner-id-or-address> --json
python tools/recoil.py owner next --lane binary
python tools/recoil.py verify vc5 --owner <owner-id-or-address> --auto-chunk
```

`owner next --lane binary` can also report global, non-ledger work units before
source owners. `work_unit=final-repro` is the final executable
reproducibility lane: final-build artifacts, executed summary state,
PE/resource comparison commands, and embedded final-data blockers.
`work_unit=final-data-layout` means the final linked `Recoil.exe` `.data`
section/raw/virtual/zero-fill or map placement still contradicts retail. Both
are first-class binary-lane targets, but neither is a source owner. Resolve
them by fixing the final-build or ranked object/source-file layout blockers,
not by adding synthetic owner records.

Owner links have a normalized relationship model. Schema version 1 ledgers are
derived from `anchors`, `member_addresses`, `data_addresses`, and
`dependencies`; schema version 2 stores explicit `relationships` while keeping
those legacy fields as compatibility mirrors. Use `owner relationships` to
distinguish anchor-only addresses, primary function/data ownership, and
owner-to-owner dependencies before changing gates or plan markers. Use
`owner migrate-relationships --dry-run --json` to inspect a v2 conversion; do
not apply it until validation findings are resolved and the migration is
explicitly authorized.

For tier `S` byte-gate work, run VC5 verification at source-owner scope with
`verify vc5 --owner`. The command resolves the owner id or linked plan address,
requires every linked authored function/data row to have exactly one VC5
manifest item, and reports missing coverage before compiling. Explicit
target/address VC5 selectors are for diagnostics or manifest development unless
current evidence proves a true standalone owner.

Owner gate meanings:

- `boundary`: BN/source evidence has identified the full owner extent.
- `source`: source-faithful owner model is implemented, not an address slice or scaffold.
- `data`: all touched authored `.data`, `.rdata`, and BSS owner facts are reconstructed.
- `functional`: current behavior evidence exists at the owner or accepted target level.
- `byte`: tier `S` owner-level byte/provider ABI evidence. For function/code owners this may be `deferred` while global owner/data blockers remain; when tier `S` is in scope, `python tools/recoil.py verify vc5 --owner <owner-id-or-address> --auto-chunk` is the default verification command. For data-owner entries it is accepted when the data-symbol byte and relocation identity evidence is accepted.

`plan set` now gates positive markers through this ledger. `Source owner ✅`
requires a linked owner whose `boundary` and `source` gates are accepted, unless
the linked owner kind is `standalone`. `Data reimplemented ✅` requires the
linked owner `data` gate to be accepted. `Data reimplemented ❎` requires owner
`data=none` or a `standalone` owner. Tier `B`, `A`, and `S` promotions require
accepted source ownership and accepted/no-data ownership.

Plan-tracked data globals use a separate data entry shape for canonical `.data`
owner ranges only. Do not add `.rdata` plan entries or member rows inside a
larger global. Add explicit entries with:

```powershell
python tools/recoil.py plan add-data 0xNNNNNN --name g_Symbol --section .data --size N --type RecoveredType --file src/Path.cpp --target vc5_data_target --group data.group --dry-run
```

Data entries carry `Reconstructed (Kind: data; Name; Section; Size; Type)`,
`Source owner`, and `Reimplemented [X/F/C/B/A/S]`; they do not carry `Source
dependencies satisfied` or `Data reimplemented`. Data `F` records a compiled
canonical source definition/declaration, `C` requires linked owner
`boundary/source` gates, `B` requires linked owner `data`, `A` records reviewed
near-byte-equivalent data-symbol evidence, and `S` requires linked owner
`byte` accepted. Use:

```powershell
python tools/recoil.py plan migrate-data-tiers --dry-run
```

to rewrite legacy data rows from current owner gates before applying the
non-dry-run command.

Do not seed owners as accepted from legacy function metadata. A marker can be
current only when the owner ledger and current BN/source/build evidence agree.

For complete data-owner acceptance requirements, use `data_owner_audit.md`.
For launch sequencing, use `agent_launch_checklist.md`.
For final executable lane details, use `final_executable_repro.md`.
