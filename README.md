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
Generated from `.agent/RECOIL_PLAN.md` and `.agent/RECOIL_MESSAGES_PLAN.md`.

### Recoil.exe Plan

| Plan marker | Scope | ✅ | ☑️ | ❎ | ❌ | ❓ | Total |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Reconstructed | all entries | 5963 | 1117 | 0 | 0 | 0 | 7080 |
| Source dependencies satisfied | authored entries | 3512 | 0 | 0 | 0 | 2986 | 6498 |
| Source owner | authored entries | 6448 | 0 | 0 | 50 | 0 | 6498 |
| Data reimplemented | authored entries | 2623 | 0 | 889 | 0 | 2986 | 6498 |
| Reimplemented | authored entries | 6498 | 0 | 0 | 0 | 0 | 6498 |
| Provider-boundary | provider entries | 581 | 1 | 0 | 0 | 0 | 582 |

| Reimplementation tier | Count | Percent of authored |
| --- | ---: | ---: |
| X | 0 | 0.0% |
| F | 2 | 0.0% |
| C | 48 | 0.7% |
| B | 6244 | 96.1% |
| A | 165 | 2.5% |
| S | 39 | 0.6% |

### messages.dll Plan

| Plan marker | Scope | ✅ | ☑️ | ❎ | ❌ | ❓ | Total |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Reconstructed | all entries | 152 | 0 | 0 | 0 | 0 | 152 |
| Source dependencies satisfied | authored entries | 1 | 0 | 0 | 0 | 1 | 2 |
| Source owner | authored entries | 2 | 0 | 0 | 0 | 0 | 2 |
| Data reimplemented | authored entries | 1 | 0 | 0 | 0 | 1 | 2 |
| Reimplemented | authored entries | 2 | 0 | 0 | 0 | 0 | 2 |
| Provider-boundary | provider entries | 150 | 0 | 0 | 0 | 0 | 150 |

| Reimplementation tier | Count | Percent of authored |
| --- | ---: | ---: |
| X | 0 | 0.0% |
| F | 0 | 0.0% |
| C | 0 | 0.0% |
| B | 0 | 0.0% |
| A | 0 | 0.0% |
| S | 2 | 100.0% |
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
