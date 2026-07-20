# Recoil Engine Reconstruction

This repository is a source-faithful native C/C++ reconstruction of the
Windows x86 engine for Zipper Interactive's 1999 game **Recoil**. It is a
reconstruction effort, not a modern port.

Success has one exact definition: an unrestricted VC5SP3 compile and link of
the reconstructed source must reproduce every typed, acceptance-relevant
semantic fact in the immutable retail reference `support/Recoil.exe`. This
includes complete function population and order, object and linked bytes,
relocations and targets, data, resources, imports, providers, PE layout, and
all catalogued padding. The linker-written COFF timestamp and the resulting raw
whole-file difference are diagnostics, not acceptance gates. Behavioral
similarity, post-link patching, or a matching subset of the executable do not
satisfy the goal.

The unified tracker coordinates five stages with independently monotonic order
and authored-byte lanes that join before resolved linked-byte traversal:

1. `authored-function-order` recovers the natural VC5SP3 order of
   source-authored and authored-lifecycle bodies across retail `.text`.
2. Independently, `authored-byte-match` proves each gating authored
   contribution at the object, relocation, symbolic-target, and
   relocation-normalized linked-body level.
3. After authored order completes, `full-function-order` restarts at the
   beginning without waiting for authored bytes and recovers the complete
   linked contribution set, retail addresses, order, and seams, including
   compiler, runtime, framework, and provider rows.
4. Once full order and authored-byte traversal have both completed,
   `linked-byte-match` proves every resolved linked address, relocation operand,
   reference target, and linked byte sequence.
5. `final-validation` derives complete typed final-image coverage live from
   retail plus accepted tracker facts, then checks one fresh unrestricted
   build. No unmodelled byte range may pass. Raw file
   differences and the COFF timestamp are reported only to aid diagnosis.

The source remains deliberately compatible with the recovered late-1990s
Windows compiler, ABI, SDK, MFC, and DirectX boundaries. Modern-language or
modern-library helpers may be used in host-side tools and tests; they are never
substitutes for source-faithful production reconstruction under `src/`.

## Repository Layout

- `src/Battlesport` — reconstructed game shell and gameplay-facing source.
- `src/GameZRecoil` — reconstructed engine subsystems.
- `src/native` — native target wiring and include bridges.
- `tests/native` and `tests/tools` — focused behavior, ABI, guard, and tooling
  verification.
- `tools` — local reconstruction and verification infrastructure.
- `docs/reconstruction` — durable evidence and reconstruction runbooks.
- `export` — analysis exports used as supporting recovery evidence.

The canonical reproduction runbook is
[`docs/reconstruction/retail_executable_reproduction.md`](docs/reconstruction/retail_executable_reproduction.md).
Physical source-block and semantic-span mapping are internal reconstruction
state; durable placement evidence is documented in
[`source_file_layout_audit.md`](docs/reconstruction/source_file_layout_audit.md).
The tool-managed unified tracker is the only progress authority.

The public repository is source-only. It does not redistribute the original
game executable, game data, extracted assets, decompiler databases, legacy SDK
payloads, redistributable DLLs, or compiler toolchains.

## Status

<!-- RECOIL_PROGRESS:START -->
Generated from the unified reconstruction tracker. The tracker remains the sole progress authority.

### Live Reconstruction Pipeline

| Stage | State | Accepted / total | Frontier | Typed blocker |
| --- | --- | ---: | --- | --- |
| authored-function-order | current | 6 / 107 | 0x415ab0 | order-target-role-gate-blocked: 0x416790 (HudSensorTracker::MapShutdownAndResetThunk) |
| authored-byte-match | ready | 0 / 3424 | 0x401000 | — |
| full-function-order | waiting | 0 / 107 | 0x401000 | — |
| linked-byte-match | waiting | 0 / 4955 | 0x401000 | — |
| final-validation | waiting | typed whole image | — | — |
| authored object-byte preparation (subordinate) | ready | 7 / 490 | 0x401710 | — |

### Source-Owner Overview

| Binary | Owners | Authored owners | Provider boundaries |
| --- | ---: | ---: | ---: |
| recoil | 1246 | 1199 | 47 |
| messages | 1 | 1 | 0 |
| Total | 1247 | 1200 | 47 |

### Source-Owner Gates

| Gate | accepted | blocked | deferred | none | pending | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| boundary | 1232 | 5 | 0 | 1 | 9 | 1247 |
| source | 1224 | 8 | 0 | 1 | 14 | 1247 |
| data | 966 | 4 | 0 | 214 | 63 | 1247 |
| functional | 806 | 0 | 0 | 325 | 116 | 1247 |
| linkage | 1026 | 142 | 0 | 67 | 12 | 1247 |
| byte | 22 | 466 | 725 | 0 | 34 | 1247 |

### Owner Reimplementation Tiers

| Tier | Count | Percent of authored owners |
| --- | ---: | ---: |
| X | 131 | 10.9% |
| C | 223 | 18.6% |
| B | 832 | 69.3% |
| A | 1 | 0.1% |
| S | 13 | 1.1% |

### Function Reimplementation Tiers

Counts durable per-primary-entry tiers for authored primary functions. Owner tiers are derived separately and may be lower because of sibling entries or owner gates.

| Tier | recoil | messages | Total | Percent of authored entries |
| --- | ---: | ---: | ---: | ---: |
| X | 483 | 0 | 483 | 13.6% |
| C | 72 | 0 | 72 | 2.0% |
| B | 2951 | 0 | 2951 | 83.3% |
| A | 0 | 0 | 0 | 0.0% |
| S | 36 | 1 | 37 | 1.0% |

### Data Reimplementation Tiers

Counts durable per-primary-entry tiers for authored primary data entries. Owner tiers are derived separately and may be lower because of sibling entries or owner gates.

| Tier | recoil | messages | Total | Percent of authored entries |
| --- | ---: | ---: | ---: | ---: |
| X | 345 | 0 | 345 | 10.6% |
| C | 1599 | 0 | 1599 | 49.1% |
| B | 1305 | 0 | 1305 | 40.1% |
| A | 4 | 0 | 4 | 0.1% |
| S | 0 | 1 | 1 | 0.0% |

### Owner Kinds

| Kind | Count | Percent of owners |
| --- | ---: | ---: |
| callback-table | 1 | 0.1% |
| class | 164 | 13.2% |
| data-owner | 430 | 34.5% |
| provider-boundary | 47 | 3.8% |
| record | 42 | 3.4% |
| source-file | 166 | 13.3% |
| standalone | 58 | 4.7% |
| subsystem | 339 | 27.2% |
<!-- RECOIL_PROGRESS:END -->

Reconstruction is ongoing. The marker-managed block above is synchronized from
`.agent/RECONSTRUCTION_PROGRESS.json`; that unified tracker remains the sole
progress authority. The snapshot intentionally excludes leases, work packets,
evidence ids, commands, timestamps, and tracker revisions so it stays a concise
public view rather than a second operational ledger.

## License

The reconstructed source, tests, and documentation in this repository are
released under the MIT License; see `LICENSE`.

That license does not grant rights to the original *Recoil* game, retail
executable, game data, extracted resources, third-party SDKs, redistributable
DLLs, decompiler databases, or compiler/toolchain payloads.
