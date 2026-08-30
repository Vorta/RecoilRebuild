---
name: recoil-workspace-audit
description: Audit RecoilRebuild workspace clarity, local agent instructions, Recoil skills, local infrastructure boundaries, and non-address process health. Use when you need to review AGENTS.md, .agent/AGENTS.md, docs/reconstruction onboarding notes, local recoil-* skills, validation guidance, stale workspace metadata, or whether a non-address maintenance task is clear for future agents.
---

# Recoil Workspace Audit

This is `task_class: non-address-maintenance`. Do not select an address, require
Binary Ninja, inspect deferred queues, or mutate reconstruction state unless
the reported surface defect specifically depends on them.

## What Must Align

A fresh agent must see one mission and scheduler: live source-faithful VC5SP3
reproduction of every typed retail fact, with `progress next` as the only
no-target scheduler. The six stages, owner/source-shape boundaries,
provider/tier separation, and final typed-coverage rules live in root
`AGENTS.md` and the retail executable runbook rather than copied local policy.
The primary lane runs `authored-function-order`, then
`authored-call-contract`, then starts `full-function-order` immediately when
every call-contract slice is current without waiting for authored-byte
completion. `authored-byte-match` is an independent retail-monotonic lane.
`linked-byte-match` starts only after both full order and authored bytes are
complete, followed by `final-validation`.

Audit these mode contracts:

- `order-edit-v1` receives one registered `verify vc5-order` command and exact
  writable source/header paths. Its hot loop has no bytes, BN, tracker mutation,
  evidence package, candidate-content qualifier, or routine Pro call. A target may cover
  several explicit contiguous physical-block slices.
- `call-contract-edit-v1` receives one deterministic retail-contiguous slice,
  exact writable source/header paths, and one registered nonmutating `verify
  call-contract` command. It compares exact static invocation contracts and
  accepts no order, byte, owner, provider, gate, or tier state.
- A bare `Start` needs no extra prompt. The root parent runs `progress next`
  and leases, computes runtime-available child slots, then applies `progress
  work claim-current --lane all --max-packets <available-child-slots> ...`
  and renders every returned reservation with `progress handoff --packet-id
  <packet-id> --json`. Priority is the primary lane (order or call contract),
  authored, object; a blocked primary does not suppress bytes, authored wins
  on overlap, and tool-owned conflicts or capacity decide actual launches.
  Individual lane claims remain supported.
- `advance-live-order`, `advance-live-call-contract`, and `advance-live-byte`
  normally use direct `--apply`: one fresh validation/build result is
  CAS-applied. Optional `--dry-run` is diagnostic. Manual semantic mutations
  remain dry-run-first.
- The reviewed 3,380 authored-body census is a one-time migration guard, not a
  permanent live-count invariant. Current call-contract population is derived
  dynamically from reviewed `pipeline_class` and `authored_order_role` state.
- authored relocation expectations come live from immutable retail plus
  accepted typed identity/provider/alias facts. Explicit empty is valid;
  ambiguity blocks before build; candidate output is never expected truth.
- Missing deterministic target identity routes to dry-run-first `progress
  relocation-target bind`; genuine ambiguity alone routes to `progress
  relocation-exception set`.
- An unresolved row inside an order target blocks the entire physical-block
  packet and acceptance even when a resolved-subset raw diagnostic passes.
- final-image coverage is derived live from retail and tracker facts. A legacy
  stored catalog blob is not required; concrete gaps, overlaps, unknown
  extents, ambiguity, and missing providers block before the final build.
- call-contract acceptance performs one fresh build and direct per-body retail
  comparison in the parent invocation. Stored body results never substitute;
  the current integer coordinates are `CALL_CONTRACT_VERIFIER_GENERATION = 13`,
  `NORMALIZER_REGISTRY_GENERATION = 12`, and
  `EXPECTED_FACT_SCHEMA_VERSION = 12`. They drive conservative invalidation,
  and transition requires a fresh complete no-reuse zero-divergence scan.
- tracked-write issue and progress packets require a clean reviewed branch and use the opaque
  baseline commit plus native Git status/diff for exact closure control. Database
  no-mutation evidence uses revisions, schema/user version, row counts, and
  `PRAGMA integrity_check` under CAS.
- Every fallible compiler, test, audit, and doctor check completes in the
  temporary integration worktree before `master` advances. Afterwards, only
  deterministic Git, topology, tag, and physical-identity assertions run.
  Linked validation reads tracked files from the executing worktree and
  machine-local retail/live SQLite inputs from the validated canonical control
  root without copying or linking them into the linked checkout.
- Owner structure changes use reviewed, dry-run-first `owner replace-batch`;
  conservative gate/tier invalidation uses `owner downgrade`; functional
  registration changes use `verification-target sync`. Unsupported positive
  owner metadata/gate/tier mutations are workspace issues, never generic owner
  commands.
- ICF/COMDAT diagnosis uses two isolated `verify final-build` runs with
  `--link-profile vc5sp3_ref_icf` and
  `--link-profile vc5sp3_ref_noicf`, followed by direct OBJ and MAP inspection.
  A retired `audit vc5-comdat` receipt is not evidence.

Audit canonical ownership rather than requiring duplicated paragraphs:

- root/runbook: mission, scheduler, live acceptance, and session authority;
- source-model skill: source shape and ambiguity-driven Pro escalation;
- tier skill: hard-byte and raw-assembly exceptions;
- provider skill: authored versus provider/runtime classification;
- BN read-only versus BN reconstruction skills: stable readers versus one
  exclusive writer lease;
- compact source-worker and verifier roles: packet execution and narrow return,
  while the parent keeps scheduling, integration, acceptance, and final claims.

Flag competing schedulers, copied or hand-maintained live addresses/progress
tables, synthetic
handoffs, mutating worker commands, routine double-build acceptance, broad Pro
requirements in registered order work, saved-candidate/receipt qualification,
invented size/end fields for unknown extents, or raw whole-file equality treated as
final acceptance.
The bounded `README.md` block between `RECOIL_PROGRESS` markers is the sole
exception: it must be generated from the unified tracker through `docs
readme-progress`, synchronized automatically after authoritative tracker
mutations, and checked by infrastructure validation.

## Static And Operational Checks

```powershell
python tools/recoil.py doctor --infrastructure-only
python tools/recoil.py audit agent-surface --strict
python tools/recoil.py audit workflow-contracts --strict
python tools/recoil.py audit pipeline-reachability --strict
python tools/recoil.py audit live-validation-surface --strict
python tools/recoil.py audit workspace --summary --strict
python tools/recoil.py progress audit --scope pipeline --strict
python tools/recoil.py progress audit --scope owners --strict
python tools/recoil.py issue audit --strict
python tools/recoil.py workspace worktree status --json
python tools/recoil.py workspace worktree hygiene --strict --json
```

`agent-surface` is static syntax/reference alignment. `workflow-contracts`
exercises command transitions and real reservation-backed handoffs.
`pipeline-reachability` verifies that current order targets cover their
accepted slices and that every required fail-closed consumer has a live
producer or reviewed ambiguity route. All three are required for workflow
health.

Worktree audit requires exact packet associations, external build-root
authentication, no inactive packet branches, and the `native-git-v1` progress
adapter. Newly claimed tracked-write progress packets must not become visible
before their v4 reservation, opaque baseline, exact linked association,
normalized writable closure, and physical build-root identity are complete;
terminal legacy v3 packets are readable but non-relaunchable.

Validate every repo-local `recoil-*` skill with the skill validator under
`PYTHONUTF8=1`, every skill `agents/openai.yaml`, and every role TOML with
`tomllib`. Treat ordinary reconstruction backlog separately from a broken
tool/rule path.

Do not run Git, clear `.devspace`, mutate source/BN/ledgers, or make durable
facts depend on session scratch. Report exact checks, corrected paths,
remaining operational gaps, and a workspace-issue candidate only when a
reproducible tool or rule defect remains.
