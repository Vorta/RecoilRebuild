---
name: recoil-functional-targets
description: Create, inspect, and run RecoilRebuild functional verification targets. Use when you need to add or inspect tools/functional_verify_targets manifests, choose target ids, register native smoke tests, run functional verification through python tools/recoil.py, verify tier C behavior evidence including original-source helper/docblock gates, handle missing recoil_native_smoke builds, or understand what functional evidence can and cannot justify for Reimplemented [C/B/A/S] owner tiers.
---

# Recoil Functional Targets

## Global Text Pipeline

Functional tier-C work is subordinate to `python tools/recoil.py progress next`.
Schedule it only for an explicit regression or when it is the recorded
dependency of the current `authored-function-order`,
`authored-call-contract`, `authored-byte-match`, `full-function-order`,
`linked-byte-match`, or `final-validation` cursor (including an explicitly
returned `parallel_authored_byte_cursor`); ordinary later functional candidates
are `deferred_by_pipeline_phase`.

## Core Rule

Start from root `AGENTS.md`; this skill adds functional-target workflow guidance and does not replace owner tier criteria.

Functional targets are local tier `C` behavior evidence for authored code. They live under `tools/functional_verify_targets/` as local verification state.

Use `recoil-validation` for broad build/check selection and `recoil-tier-verification` before claiming owner tier eligibility. Passing functional verification does not by itself justify tier `B`, tier `S`, or source-shape metadata.

## Manifest Requirements

Each target is a JSON file under `tools/functional_verify_targets/`.

Required fields:

- `name`: lowercase snake_case target id; must match the JSON filename stem
- `address`: primary original address
- `source_from`: production source file path
- `smoke_tests`: one or more native smoke names

Optional fields:

- `covered_addresses`: additional original addresses covered by the same target
- `description`: concise target purpose
- `vc5_attempt`: must begin with `python tools/recoil.py verify vc5` when present
- `known_limits`: current tier `S` blockers or limits
- `tier_s_evidence`: accepted tier `S` evidence, if present

Every manifest must include either `known_limits` or `tier_s_evidence`.

Do not use retired tier-S evidence keys; use `tier_s_evidence`.

Functional manifests are not VC5 verification manifests. If tier `S` byte/provider evidence or generated data-symbol evidence is needed, use `tools/vc5_verify_targets/` with production `source_from` entries and `python tools/recoil.py verify vc5 <target-or-address>`.

## Target Id Semantics

The registered functional verification target id is the lowercase snake_case
JSON filename stem matching the manifest `name`.

It is not the source symbol, implementation function name, C++ method name, or Binary Ninja name.

When a target covers multiple addresses, run the canonical
`python tools/recoil.py verify functional` command by address or target id for
the exact address that was verified. If target registration or its source
policy changes, synchronize the selected target only through:

```powershell
python tools/recoil.py progress verification-target sync --target <target-id> --expected-revision <revision> --dry-run --json
```

The parent reviews the dry-run before repeating the unchanged command with
`--apply`. `verification-target sync` is the target-registration route; it is
not an owner metadata/gate/tier setter. If a positive owner mutation is also
required and no registered command exists, report a workspace issue. Do not
hand-edit `.agent/RECONSTRUCTION_PROGRESS.sqlite3`.

## Smoke Tests

Smoke names in `smoke_tests` must be registered in `tests/native/smoke.cpp` and implemented under `tests/native`.

If `verify functional` reports an unknown smoke, inspect the manifest, smoke registration, implementation file, and native target inclusion. Repair or replace the local functional target as part of the active verification work; do not file a workspace issue for missing, unregistered, stale, or source-drift-crashing smokes. Escalate to `.agent/WORKSPACE_ISSUES.sqlite3` only when the runner/tool behaves incorrectly after the smoke is correctly registered, built, and runnable through the documented x86 wrappers.

Prefer focused smoke coverage that proves the behavior needed by the function/source group. A single smoke can be acceptable for narrow behavior, but nontrivial accepted targets should usually have enough cases to cover edge behavior, state changes, and important inputs.

Run native smoke commands through the x86 workflow when needed:

```powershell
powershell -ExecutionPolicy Bypass -File cmake\recoil_native_x86_build.ps1 -Preset ninja-x86-debug
python tools\recoil.py build msvc-x86 -- ctest --preset ninja-x86-debug
```

If plain CMake, CTest, or `recoil_native_smoke` fails because the shell is not an x86 MSVC environment, rerun through the wrappers before reporting the build unavailable.

## Commands

List targets:

```powershell
python tools/recoil.py `
  verify functional --list
```

Run by address or target id:

```powershell
python tools/recoil.py `
  verify functional 0xNNNNNN
python tools/recoil.py `
  verify functional <target_id>
```

Print smoke commands and owner tier hint without running:

```powershell
python tools/recoil.py `
  verify functional <target_id> --dry-run
```

If no native smoke executable is found, build with the Ninja x86 wrapper, build `recoil_native_smoke` from the Visual Studio `vs-x86` solution, or pass `--executable`.

## Evidence Boundaries

A passing functional target may support `Reimplemented [C]` only when:

- authored source exists and compiles
- the source matches the Binary Ninja source/ABI contract
- authored runtime-state access does not use raw offsets
- production helper calls all have original-source provenance; callers of non-original reconstruction helpers, source-shape scaffolds, provider shims, or raw authored runtime-state offsets are not reimplemented and must remain `Reimplemented [X]`/not done
- production source does not depend on local virtual dispatch views, production VTable/FTable structs/globals/slot arrays, vtable/ftable factory scaffolds, or temporary ABI/source-shape probes
- the target id matches the manifest filename stem and `name`
- smoke output passed in the current session

Functional evidence does not satisfy owner source/data/linkage gates for tier
`B`. A passing current `verify functional ...` result may be independently
reviewed as primary-function tier-C evidence, but verification never mutates
the ledger automatically and no retired positive tier setter may be implied.
If no registered positive tier mutation exists, report the missing route as a
workspace issue. Functional evidence does not satisfy generated byte/provider
ABI evidence for tier `S` or prove `Model: source-faithful`.
Normal binary-lane work should route unresolved owner blockers before creating
isolated implementation or behavior evidence. After tier `C` is accepted,
resolve owner/data blockers for the linked owner before verify-only tier `S`
attempts. Tier `S` verification is owner-scoped: the complete linked source
owner plus primary-owned, referenced, touched, and dependency data must be
ready for that owner's byte gate, unless the user explicitly directs a narrower
diagnostic. This gate eligibility does not override `progress next` scheduling.

When touched production source changed, run the original-source helper guard
and canonical source-trace audit for the touched files before using functional
evidence for owner tier work:

```powershell
python tools\recoil.py guard original-symbol --root src --max 50
python tools\recoil.py audit source-trace --path <touched-source> --policy migrated --json
```

Leave known tier `S` limits in `known_limits` until accepted tier `S` evidence exists.

When `tier_s_evidence` is copied into a functional manifest, keep it as a compact summary of an accepted VC5SP3 byte/provider run. Do not treat a `known_limits` note or failed byte comparison as accepted tier `S` evidence.

## Reporting

When reporting functional target work, include:

- target id and manifest path
- primary address and covered addresses
- source file
- smoke names and whether they are registered
- command run and pass/fail status
- owner tier command printed by the canonical `python tools/recoil.py` `verify functional` command, if used
- original-source helper and source-trace audit status when source changed
- known tier `S` limits or tier `S` evidence state
- reminder that manifests are local evidence and any durable facts were recorded elsewhere

Subagents never clear or durably depend on `.devspace`; return material
semantic conclusions and direct evidence paths with their role and scope.

