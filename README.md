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

The unified tracker coordinates six stages with independently monotonic
primary and authored-byte lanes that join before resolved linked-byte
traversal:

1. `authored-function-order` recovers the natural VC5SP3 order of
   source-authored and authored-lifecycle bodies across retail `.text`.
2. `authored-call-contract` checks the static invocation contract of every
   currently reviewed physical authored gating body in deterministic
   retail-contiguous slices.
3. Independently, `authored-byte-match` proves each gating authored
   contribution at the object, relocation, symbolic-target, and
   relocation-normalized linked-body level.
4. After all authored call contracts are current, `full-function-order`
   restarts at the beginning without waiting for authored bytes and recovers
   the complete linked contribution set, retail addresses, order, and seams,
   including compiler, runtime, framework, and provider rows.
5. Once full order and authored-byte traversal have both completed,
   `linked-byte-match` proves every resolved linked address, relocation operand,
   reference target, and linked byte sequence.
6. `final-validation` derives complete typed final-image coverage live from
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
- `cmake/recoil_native` — native target wiring outside the reconstructed
  production source tree.
- `tools/_recoil/compat/include` — compiler, calling-convention, and ABI
  compatibility bridges used by governed builds.
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
| authored-function-order | complete | 121 / 121 | 0x4cb9e8 | — |
| authored-call-contract | current | 0 / 3369 | 0x401000 | — |
| authored-byte-match | blocked | 14 / 3369 | 0x401d50 | retail byte frontier shares an active primary source-edit physical block |
| full-function-order | waiting | 0 / 121 | 0x401000 | — |
| linked-byte-match | waiting | 0 / 4974 | 0x401000 | — |
| final-validation | waiting | typed whole image | — | — |
| authored object-byte preparation (subordinate) | blocked | 15 / 3369 | 0x401e50 | object byte frontier shares an active primary source-edit physical block |

### Source-Owner Overview

| Binary | Owners | Authored owners | Provider boundaries |
| --- | ---: | ---: | ---: |
| recoil | 1268 | 1200 | 68 |
| messages | 1 | 1 | 0 |
| Total | 1269 | 1201 | 68 |

### Source-Owner Gates

| Gate | accepted | blocked | deferred | none | pending | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| boundary | 1252 | 6 | 0 | 1 | 10 | 1269 |
| source | 1243 | 10 | 0 | 1 | 15 | 1269 |
| data | 985 | 5 | 0 | 215 | 64 | 1269 |
| functional | 806 | 0 | 0 | 346 | 117 | 1269 |
| linkage | 1025 | 143 | 0 | 88 | 13 | 1269 |
| byte | 22 | 466 | 746 | 0 | 35 | 1269 |

### Owner Reimplementation Tiers

| Tier | Count | Percent of authored owners |
| --- | ---: | ---: |
| X | 147 | 12.2% |
| C | 221 | 18.4% |
| B | 819 | 68.2% |
| A | 1 | 0.1% |
| S | 13 | 1.1% |

### Function Reimplementation Tiers

Counts durable per-primary-entry tiers for authored primary functions. Owner tiers are derived separately and may be lower because of sibling entries or owner gates.

| Tier | recoil | messages | Total | Percent of authored entries |
| --- | ---: | ---: | ---: | ---: |
| X | 484 | 0 | 484 | 13.7% |
| C | 74 | 0 | 74 | 2.1% |
| B | 2948 | 0 | 2948 | 83.2% |
| A | 0 | 0 | 0 | 0.0% |
| S | 36 | 1 | 37 | 1.0% |

### Data Reimplementation Tiers

Counts durable per-primary-entry tiers for authored primary data entries. Owner tiers are derived separately and may be lower because of sibling entries or owner gates.

| Tier | recoil | messages | Total | Percent of authored entries |
| --- | ---: | ---: | ---: | ---: |
| X | 392 | 0 | 392 | 12.0% |
| C | 1596 | 0 | 1596 | 48.9% |
| B | 1271 | 0 | 1271 | 38.9% |
| A | 4 | 0 | 4 | 0.1% |
| S | 0 | 1 | 1 | 0.0% |

### Owner Kinds

| Kind | Count | Percent of owners |
| --- | ---: | ---: |
| callback-table | 1 | 0.1% |
| class | 164 | 12.9% |
| data-owner | 431 | 34.0% |
| provider-boundary | 68 | 5.4% |
| record | 42 | 3.3% |
| source-file | 166 | 13.1% |
| standalone | 58 | 4.6% |
| subsystem | 339 | 26.7% |
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
