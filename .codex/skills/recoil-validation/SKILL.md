---
name: recoil-validation
description: Validate work in the RecoilRebuild workspace. Use when you need to choose or run checks after reconstruction/source/docs/tool/progress-tracker changes, diagnose native build/test failures, run Recoil doctor/progress/function-order/byte/final-image verification, owner audits, source guards, CTest/native smoke, or PE reference checks.
---

# Recoil Validation

Root `AGENTS.md` is authoritative. Validation observes current source and fresh
build output. It never chooses work, mutates tracker state, or proves source
shape, ownership, provider classification, or a tier by itself.

## Select The Narrowest Check

For docs, tools, skills, roles, and workflow plumbing:

```powershell
python tools/recoil.py doctor --infrastructure-only
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit workflow-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
python tools/recoil.py audit live-validation-surface --strict
python tools/recoil.py workspace worktree status --json
python tools/recoil.py workspace worktree hygiene --strict --json
```

`agent-surface` checks static syntax and references. The two operational audits
check executable command transitions, real handoffs, target coverage, and
required expected-fact producers. A static pass alone is not workflow health.
Packet integration completes every fallible compiler, test, audit, and doctor
check in the temporary integration worktree before `master` advances. After the
fast-forward, run only deterministic Git, topology, tag, and physical-identity
assertions; do not run another fallible semantic validation command. Neither
the validation nor the deterministic assertions are reconstruction acceptance.

In linked validation, tracked source, tools, tests, policies, target manifests,
and `.agent/REFERENCE_EXECUTABLE.json` come from the executing worktree. Resolve
machine-local retail as `<canonical-control-root>/support/Recoil.exe` and live
progress/issue inputs as `<canonical-control-root>/.agent/<live-database>`.
Never substitute canonical-worktree tracked files for the linked candidate or
copy/link the machine-local inputs into the linked checkout.

For address-led work:

```powershell
python tools/recoil.py progress show 0xNNNNNN
python tools/recoil.py doctor --active 0xNNNNNN
```

For behavior, source guards, and native smoke tests, run only the registered
target or touched scope. Passing behavior proves none of function order,
object/linked bytes, source ownership, final placement, or provider identity.

## Function Order

An `order-edit-v1` worker runs only:

```powershell
python tools/recoil.py verify vc5-order <target> --build-root <isolated-root>
```

The registered target may cover one translation unit and several explicit,
contiguous physical-block slices. The command compiles current source and
reports the first missing, duplicated, unexpected, or reordered retail
identity with neighbors. It needs no Binary Ninja session, byte comparison,
evidence package, candidate hash, tracker mutation, or routine ChatGPT Pro
call. Ordinary comments are irrelevant; explicit address-bearing source
markers remain semantic inputs. Every phase-appropriate expected gating
identity must resolve exactly once in retail relative order. `authored-body`
and `authored-lifecycle-body` gate authored order; compiler-generated deleting
variants remain inventoried for full order without gating it. Inventory every
unlisted raw definition as a mechanically non-blocking diagnostic because the
linker may discard or fold it, while full linked order separately requires the
exact selected linked groups, RVAs, providers, padding, and seams.

One unresolved row anywhere in the target interval blocks the whole physical-
block packet and acceptance. A raw diagnostic over a resolved subset may PASS,
but that partial result is not launchable or acceptable.

After a worker result, the parent independently rebuilds and accepts only the
complete target-covered slices proven by that same invocation:

```powershell
python tools/recoil.py progress advance-live-order --target <target> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

Direct `--apply` is normal for this self-validating command. It builds and
validates once, then CAS-mutates against the supplied revision from that
in-memory semantic result. `--dry-run` is an optional diagnostic, not a routine
first half of a two-build acceptance cycle. A target failure accepts none of
its slices and reports the first typed divergence.

## Authored Call Contracts

After authored order, `authored-call-contract` keeps the accepted
retail-monotonic slice census as cursor windows. A slice is not an atomic
acceptance unit: the parent may accept only the individual bodies that pass the
direct comparison performed by that invocation. The normal worker diagnostic
and parent acceptance routes are:

```powershell
python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json
python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --packet-id <packet-id> --build-root <packet-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

The verifier compares exact static call order/count, direct/provider/IAT
identity, direct-versus-indirect form, virtual/interface slot or callback
storage, call-versus-tail form, and known cleanup with current retail Binary
Ninja evidence. Local branches are ignored; unresolved identity fails closed.
The worker command is nonaccepting. The parent command authenticates its active
packet and physical output root, opens retail through one stable read-only file
identity, authenticates equal Binary Ninja provider begin/end generation and
revision snapshots, performs one fresh build, and directly compares the exact
accepted-order-derived bodies. Only bodies that pass in that same invocation
may advance `call_contract`; an unrelated divergent body remains pending and
the cursor stays on the first slice containing one.

No stored body result, worker result, saved candidate, object, or receipt can
substitute for that fresh comparison. Currency is maintained by governed
source/tool/manifest mutation and conservative explicit invalidation. The
reviewed implementation coordinates are currently
`CALL_CONTRACT_VERIFIER_GENERATION = 12`,
`NORMALIZER_REGISTRY_GENERATION = 12`, and
`EXPECTED_FACT_SCHEMA_VERSION = 12`. Any verifier-component change invalidates
all current call-contract evidence; any normalizer-registry change invalidates
all users, or all evidence when its user set cannot be proven exactly. These
integer coordinates are schema controls, never candidate evidence.

A passing body accepts only `call_contract`. It does not accept full order,
bytes, owners, gates, tiers, providers, source placement, or source shape.
After every body is current, phase transition still requires one fresh,
complete-TU, no-reuse, zero-divergence scan:

```powershell
python tools/recoil.py progress call-contract prepare-live-convergence --packet-id <packet-id> --closeout --build-root <packet-root> --jobs <n> --issue-ledger .agent/WORKSPACE_ISSUES.sqlite3 --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json
```

Incremental or reused output is diagnostic and cannot substitute for this
closeout. The closeout reauthenticates the active packet, physical root,
SQLite revisions, resource claims, and governed BN reader and is the only
call-contract route that may authorize transition to `full-function-order`.

The reviewed 3,380-body census is a one-time migration guard. Live slice
population derives dynamically from current reviewed `pipeline_class` and
`authored_order_role` classifications; validation must not treat 3,380 as a
permanent invariant.

## Byte Lanes

```powershell
python tools/recoil.py audit relocation-expectations --at 0xNNNNNN --json
python tools/recoil.py progress advance-live-byte --lane <object|authored|linked> --build-root <fresh-root> --expected-revision <revision> --apply --json
```

The live command builds and validates once, then advances only the explicitly
matched physical groups from that result. Optional `--dry-run` is diagnostic.
Object validation compares body bytes outside relocation fields. Authored
validation additionally requires relocation type, immutable-retail-derived
target/provider/alias identity and addend expectations, linked presence, and
relocation-normalized linked bytes. Linked validation additionally requires
exact RVA, resolved operands and targets, and raw linked-image bytes.

Relocation expectations come from retail plus accepted typed identity,
provider, and alias facts, never from the candidate. An explicit empty set is
valid. Genuine ambiguity fails before the expensive build and routes to the
narrow reviewed exception mechanism; it is not guessed or copied from a
candidate observation. The parent resolves a genuine reviewed ambiguity with
`progress relocation-exception set`; that manual mutation remains dry-run-first
and binds exact current source/target context so later drift blocks visibly.
When retail already determines the operand but its typed existing or exact
known-extent target identity is missing, the distinct dry-run-first route is
`progress relocation-target bind`. It registers expected identity; it does not
convert missing identity into an ambiguity exception or consult candidate
output for expected truth.

## Final Image

```powershell
python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil.py audit final-image-catalog --json
python tools/recoil.py verify final-image --json
```

The catalog audit derives live typed coverage from immutable retail and
accepted tracker facts. It returns concrete gaps, overlaps, ambiguous ranges,
unknown extents, and missing providers; a legacy stored catalog blob is not a
prerequisite. Final verification fails before building while coverage is
incomplete. When complete, it performs one fresh unrestricted build and direct
typed comparison. The COFF timestamp and raw whole-file difference are
diagnostic only.

## Reporting

Lead with pass/fail and the first actionable divergence. Include the exact
command and build root, relevant identities/addresses and neighbors, and the
narrow next action. Never claim more than the direct comparison established.
