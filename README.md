# Recoil Engine Reconstruction

This repository is a source-faithful native C/C++ reconstruction of the Windows
x86 engine for Zipper Interactive's 1999 game **Recoil**.

The project aims to recover readable, maintainable source that preserves the
behavior, ABI-sensitive structure, and late-1990s Windows engine boundaries of
the original game. It is a reconstruction effort, not a modern port.

## What's Included

- `src/Battlesport` - recovered game shell and gameplay-facing source.
- `src/GameZRecoil` - recovered engine subsystems for rendering, video, sound,
  FMV, HUD, geometry, input, networking, assets, and utility code.
- `src/native` - native target wiring and include bridges for the reconstructed
  source.
- `tests/native` - focused native behavior and ABI checks.
- `docs/reconstruction` - compact durable notes about recovered engine facts.
- `export` - text snapshots from executable analysis used to guide recovery.

The public repository is source-only. It does not redistribute the original
game executable, game data, extracted assets, decompiler databases, legacy SDK
payloads, redistributable DLLs, or compiler toolchains.

## Engine Overview

The recovered source is organized around the original engine boundaries:

- `Battlesport` contains the MFC application shell, frame windows, game-state
  transitions, mission startup, player and network gameplay glue, pickups, and
  HUD integration.
- `GameZRecoil/RecoilApp` contains the reconstructed state host and state
  objects used by menus, FMV playback, gameplay, and transitions.
- `zVideo`, `zRndr`, `zImage`, and `zModel` cover DirectDraw/Direct3D-era video
  setup, software and hardware rendering paths, image and texture handling,
  model display, lighting, and material behavior.
- `zSound` and `zFMV` cover DirectSound/A3D-era audio, CD audio, sample sets,
  streamed and grouped sound playback, and Video for Windows FMV scripting.
- `zHud`, `zInput`, `zNetwork`, `zGeometry`, `zMath`, `zReader`, `zClass`, and
  `zUtil` cover UI widgets, input mapping, DirectPlay networking, clipping and
  geometry helpers, math, asset loading, object/class runtime data, and save or
  resource utilities.

The codebase intentionally keeps late-1990s Windows C/C++ idioms where they
matter for layout, calling conventions, provider behavior, and generated code
shape. Modern helper code is limited to places where it makes reconstruction,
testing, or review practical without changing the recovered engine contract.

## Status

<!-- RECOIL_PROGRESS:START -->
Generated from `.agent/SOURCE_OWNERS.json` (schema version 3).

### Source-Owner Overview

| Binary | Owners | Authored owners | Provider boundaries |
| --- | ---: | ---: | ---: |
| recoil | 1213 | 1202 | 11 |
| messages | 1 | 1 | 0 |
| Total | 1214 | 1203 | 11 |

### Source-Owner Gates

| Gate | accepted | blocked | deferred | none | pending | Total |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| boundary | 1213 | 0 | 0 | 1 | 0 | 1214 |
| source | 1212 | 1 | 0 | 1 | 0 | 1214 |
| data | 961 | 1 | 0 | 202 | 50 | 1214 |
| functional | 812 | 0 | 0 | 297 | 105 | 1214 |
| linkage | 940 | 233 | 0 | 41 | 0 | 1214 |
| byte | 22 | 468 | 704 | 0 | 20 | 1214 |

### Owner Reimplementation Tiers

| Tier | Count | Percent of authored owners |
| --- | ---: | ---: |
| X | 119 | 9.9% |
| C | 212 | 17.6% |
| B | 858 | 71.3% |
| A | 1 | 0.1% |
| S | 13 | 1.1% |

### Owner Kinds

| Kind | Count | Percent of owners |
| --- | ---: | ---: |
| callback-table | 1 | 0.1% |
| class | 165 | 13.6% |
| data-owner | 429 | 35.3% |
| provider-boundary | 11 | 0.9% |
| record | 42 | 3.5% |
| source-file | 167 | 13.8% |
| standalone | 57 | 4.7% |
| subsystem | 342 | 28.2% |
<!-- RECOIL_PROGRESS:END -->

Reconstruction is ongoing. Some subsystems have source-level implementations
and focused tests, while others remain partial or documented as recovered facts.
The project favors evidence-backed source recovery over broad rewrites, so code
may retain original-era naming, structure, and implementation style where that
helps preserve behavior.

## License

The reconstructed source, tests, and documentation in this repository are
released under the MIT License; see `LICENSE`.

That license does not grant rights to the original *Recoil* game, original
retail executable, game data, extracted resources, third-party SDKs,
redistributable DLLs, decompiler databases, or compiler/toolchain payloads.
