# Recoil Tooling

`python tools/recoil.py` is the only public entry point for reconstruction
tools. Run `python tools/recoil.py help` or
`python tools/recoil.py help <command>` for current arguments.

## Serial Work

```powershell
python tools/recoil.py progress next --json
```

The command returns one `recoil-current-task-v2` task. Its
`acceptance_command` is the only current reconstruction mutation to run
without selecting an explicit target. The task contains:

- one stage and cursor;
- one stable task id;
- advisory target/source scope;
- an optional nonmutating check command;
- one direct acceptance command when ready;
- one blocker when not ready;
- transaction, semantic, and evidence-generation revisions.

There is no task allocation or secondary current-state cache. The live tracker
is the authority. All edits happen directly in the canonical checkout.

Three retained compatibility identifiers are not workflow mechanisms:

- `agent_source_path` is the stable schema name for the canonical source-side
  compile/edit path; it does not identify a role or assignment.
- The dependency-frontier `lane` argument selects a functional-versus-binary
  comparison mode; it is not a scheduler.
- Legacy `accepted_byte_facts.lane` is read compatibility only. New live byte
  acceptance writes the canonical `accepted_byte_facts.mode` key.

The serial stage order is:

1. `authored-function-order`
2. `authored-call-contract`
3. `authored-byte-match`
4. `full-function-order`
5. `linked-byte-match`
6. `final-validation`

Authored byte completion is required before full order starts.

## Direct Live Acceptance

Order:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <fresh-root>
python tools/recoil.py progress advance-live-order --target <target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Call contracts:

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

The first command is diagnostic. The second accepts only bodies that pass its
fresh direct comparison. The closeout command becomes eligible only when every
body is current; it runs every deterministic slice from fresh output with no
reuse and requires zero divergence.

The reviewed currency coordinates are:

- call-contract verifier generation 15;
- normalizer registry generation 13;
- expected-fact schema version 13.

Bytes:

```powershell
python tools/recoil.py verify authored-byte --at 0xNNNNNN
python tools/recoil.py progress advance-live-authored-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
python tools/recoil.py verify linked-byte --at 0xNNNNNN
python tools/recoil.py progress advance-live-linked-byte --build-root <fresh-root> --expected-revision <revision> --apply --json
```

The object-only verifier is diagnostic. Authored acceptance includes
`object_byte`, relocation identity, linked presence, linked target identity,
and normalized linked body bytes. Linked acceptance additionally requires
exact RVA, resolved operands/targets, and raw linked bytes.

For self-validating live commands, direct `--apply` is normal. A dry-run
would repeat an expensive build and is optional. Manual semantic changes remain
dry-run-first.

## SQLite Authorities

The live databases are:

- `.agent/RECONSTRUCTION_PROGRESS.sqlite3`: semantic schema 6, SQLite
  user-version 3;
- `.agent/WORKSPACE_ISSUES.sqlite3`: SQLite user-version 2.

The progress metadata row contains only transaction, semantic, and
evidence-generation revisions. The issue database contains issue records and
ID sequences. Both use full synchronous SQLite transactions and revision CAS.

Never hand-edit these files. There is no JSON runtime backend or exported
mirror.

The completed hard cutover is recorded in progress migration metadata. Its
validated pre-cutover SQLite backups are retained outside the repository; the
one-time migration command is no longer part of the live tool surface.

## Workspace Issues

Issues report broken tools, validation paths, setup, or rules:

```powershell
python tools/recoil.py issue list --status open
python tools/recoil.py issue show WSI-YYYYMMDD-NNN --json
python tools/recoil.py issue report --kind tool-error --severity high --summary "..." --area tools/... --impact "..." --actual "..." --next-action "..." --expected-revision <revision> --dry-run
python tools/recoil.py issue resolve WSI-YYYYMMDD-NNN --resolution "..." --expected-revision <revision> --apply
python tools/recoil.py issue audit --strict
```

Closed issues remain durable history. Ordinary reconstruction backlog belongs
in the progress tracker, not this ledger.

## Retail-Derived Expectations

Candidate output is never expected truth. Authored relocation expectations are
derived live from immutable retail plus accepted typed identity, provider, and
alias facts:

```powershell
python tools/recoil.py audit relocation-expectations --at 0xNNNNNN --json
```

When retail determines the operand but the tracker lacks its typed target:

```powershell
python tools/recoil.py progress relocation-target bind --source-symbol-id <id> --source-address 0xNNNNNN --payload-json '<reviewed-binding>' --expected-revision <revision> --dry-run --json
```

For genuine reviewed ambiguity only:

```powershell
python tools/recoil.py progress relocation-exception set --source-symbol-id <id> --source-address 0xNNNNNN --payload-json '<reviewed-exception>' --expected-revision <revision> --dry-run --json
```

Review and repeat manual semantic operations with `--apply`.

## Owner, Provider, And Trace Mutations

Use focused registered commands:

- `progress owner replace-batch` for complete owner topology/membership
  replacement;
- `progress owner downgrade` for conservative gate/tier decreases;
- `progress verification-target sync` for target registration metadata;
- `progress source-trace replace-batch` for reviewed tracker relationships
  and source mirrors;
- `progress provider-target register` and
  `progress provider-function register` for exact provider identities;
- `progress storage register-authored-data` for known-extent authored data
  storage;
- `progress symbol set-pipeline-class-batch` for reviewed classification;
- `progress symbol set-logical-alias-group` for reviewed ICF logical aliases.

Unsupported positive owner/gate/tier mutations are tooling issues, not an
invitation to add a generic mutator.

## Binary Ninja

Before BN-backed commands:

```powershell
python tools/recoil.py doctor --quick --binja
```

The already-open database remains the analysis authority. Read-only commands
must not save or mutate it. Reconstruction commands may update the explicitly
selected analysis facts, reanalyze, check propagation, and save. Source
ownership, provider classification, acceptance, and tiers remain separate
reviewed decisions.

## Final Image

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

The catalog is derived live from retail and tracker facts. Unknown extents,
gaps, overlaps, ambiguous padding, missing providers, and unresolved entities
block before the unrestricted candidate build.

## Progress Projection

The marker-managed README block is an explicit derived projection:

```powershell
python tools/recoil.py docs readme-progress
python tools/recoil.py docs readme-progress --check --json
```

It is not updated inside every tracker transaction and does not participate in
acceptance.

## Infrastructure Validation

```powershell
python tools/recoil.py doctor --infrastructure-only
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit pipeline-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
python tools/recoil.py audit live-validation-surface --strict
python tools/recoil.py audit workspace --summary --strict
python tools/recoil.py progress audit --scope pipeline --strict
python tools/recoil.py progress audit --scope owners --strict
python tools/recoil.py issue audit --strict
```

`agent-surface` checks the canonical skill/pointer structure and active
command references. `pipeline-contracts` checks the one-task direct command
envelope. `pipeline-reachability` checks expected-fact producers and target
coverage. A static pass alone is not complete workflow health.

Run the narrowest relevant unit tests for the changed module before the full
infrastructure checks. Use a fresh build root for every compiler-backed
validation. Lead reports with pass/fail and the first actionable divergence.
