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
Generated from `.agent/RECOIL_PLAN.md`.

| Plan marker | Scope | ✅ | ☑️ | ❎ | ❌ | ❓ | Total |
| --- | --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Reconstructed | all entries | 2766 | 1151 | 0 | 0 | 0 | 3917 |
| Source dependencies satisfied | authored entries | 3420 | 0 | 0 | 110 | 0 | 3530 |
| Source owner | authored entries | 821 | 0 | 0 | 2709 | 0 | 3530 |
| Data reimplemented | authored entries | 255 | 0 | 211 | 3064 | 0 | 3530 |
| Reimplemented | authored entries | 3344 | 0 | 0 | 186 | 0 | 3530 |
| Provider-boundary | provider entries | 386 | 1 | 0 | 0 | 0 | 387 |

| Reimplementation tier | Count | Percent of authored |
| --- | ---: | ---: |
| X | 186 | 5.3% |
| F | 3 | 0.1% |
| C | 2938 | 83.2% |
| B | 222 | 6.3% |
| A | 1 | 0.0% |
| S | 180 | 5.1% |
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
