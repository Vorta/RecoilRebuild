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
python tools/recoil.py owner audit --strict
python tools/recoil.py owner next --lane binary
```

Owner gate meanings:

- `boundary`: BN/source evidence has identified the full owner extent.
- `source`: source-faithful owner model is implemented, not an address slice or scaffold.
- `data`: all touched authored `.data`, `.rdata`, and BSS owner facts are reconstructed.
- `functional`: current behavior evidence exists at the owner or accepted target level.
- `byte`: tier `S` owner-level byte/provider ABI evidence, or `deferred` while global owner/data blockers remain.

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
`Source owner`, and `Reimplemented [S]`; they do not carry `Source dependencies
satisfied` or `Data reimplemented`. Setting data `Reimplemented [S] ✅` requires
the linked SOURCE_OWNERS owner to have `boundary`, `source`, `data`, and `byte`
accepted.

Do not seed owners as accepted from legacy function metadata. A marker can be
current only when the owner ledger and current BN/source/build evidence agree.

For complete data-owner acceptance requirements, use `data_owner_audit.md`.
For launch sequencing, use `agent_launch_checklist.md`.
