# Recoil Engine Reconstruction

This repository is a native reconstruction of the Windows x86 engine for
Zipper Interactive's 1999 game *Recoil*. The goal is not to build a modern
port first; the goal is to recover a source-faithful C/C++ codebase whose
compiled output can be compared against the original retail executable.

The recovered decompiled source and original executable analysis are the
behavioral and ABI reference. Source in this workspace is rewritten from that
evidence, then checked through native builds, focused tests, and VC6
object-byte comparison. The rebuilt binary is allowed to have different
addresses, but function behavior, calling conventions, layout, imports, memory
side effects, and generated instruction structure must match the original where
a function is marked binary-safe.

This public repository is source-only. It does not redistribute the original
retail executable, game data, extracted resource images, decompiler databases,
legacy SDK payloads, compatibility DLLs, or compiler toolchains. Local
reconstruction work may use those files under ignored paths such as `support/`,
`img/`, and external toolchain folders.

The original retail executable is still the private reference for final
PE/import/resource/linkage comparison. Its known facts are tracked in
`.agent/REFERENCE_EXECUTABLE.json` so progress can be reviewed without
publishing the executable itself.

## Repository Layout

- `src/Battlesport` - recovered game shell and gameplay-facing source.
- `src/GameZRecoil` - recovered engine subsystems: app states, rendering,
  video, sound, FMV, HUD, geometry, input, networking, assets, and utility
  systems.
- `src/native` - CMake target wiring and native include bridge for the
  reconstructed C/C++ code.
- `tests/native` - CTest smoke tests, ABI checks, and focused behavior tests
  for reconstructed native code.
- `tools` - plan navigation, decompiled-source frontier reports, assembly evidence,
  and source guard scripts. See `tools/README.md`.
- `tools/vc6_verify_targets` - public VC6 assembly-verification manifests for
  reconstructed functions and dependency groups.
- `reconstruction` - durable reconstruction notes. The previous large root
  notes file is preserved as `reconstruction/NOTES.md`.
- `.agent` - long-lived reconstruction plan, implementation group notes, and
  source guard baselines used by coding agents.

Ignored local-only paths used by private verification include:

- `support/Recoil.exe` - original retail executable reference.
- `support/zbd`, `support/maps`, and runtime DLLs - original/runtime data.
- `support/sdk/DirectX_Aug2007` and `support/sdk/MFC42` - local legacy SDK
  mirrors used for native ABI work.
- `img/GAMEBMP.bmp` and `img/RECOIL.ico` - extracted resource payloads
  regenerated from a local `support/Recoil.exe`.
- `RECOIL_VC6_ROOT` or a sibling `Compiler/VC6` directory, plus local
  decompiler databases - local toolchain and reverse-engineering state.

The root `AGENTS.md` is the authoritative workflow for reconstruction agents.
It defines the binary-safety gate, source style rules, decompiled-source workflow,
plan marker meanings, and required verification process.
Use `docs/reconstruction/agent_launch_checklist.md` as the compact launch
checklist before assigning a new reconstruction agent.

## Engine Overview

The recovered source is organized around the original engine boundaries:

- `Battlesport` contains the MFC application shell, frame windows, game-state
  transitions, mission startup, player/network gameplay glue, pickups, and HUD
  integration.
- `GameZRecoil/RecoilApp` contains the reconstructed state host and state
  objects used by menus, FMV playback, gameplay, and transitions.
- `zVideo`, `zRndr`, `zImage`, and `zModel` cover DirectDraw/Direct3D-era
  video setup, software and hardware rendering paths, image/texture handling,
  model display, lighting, and material behavior.
- `zSound` and `zFMV` cover DirectSound/A3D-era audio, CD audio, sample sets,
  streamed/grouped sound playback, and Video for Windows FMV scripting.
- `zHud`, `zInput`, `zNetwork`, `zGeometry`, `zMath`, `zReader`, `zClass`, and
  `zUtil` cover UI widgets, input mapping, DirectPlay networking, clipping and
  geometry helpers, math, ZBD/ZAR asset loading, object/class runtime data, and
  save/resource utilities.

The codebase intentionally keeps late-1990s Windows C/C++ idioms where they
matter for ABI and generated assembly. Modern helpers exist only where they
make reconstruction, testing, or verification practical.

## Building

Native reconstruction work uses CMake presets. Use an x86 MSVC developer
environment for binary-safe builds. The current native build expects local
legacy DirectX/MFC42 SDK mirrors under `support/sdk/`; those files are not part
of the public source-only repository.

Before configuring from a new shell, run the environment preflight:

```powershell
python tools/recoil_env_check.py --native-x86
```

It checks that `cl.exe`, `INCLUDE`, `LIB`, the Windows SDK headers, the x86
compiler target, local DirectX/MFC42 mirrors, and `support/Recoil.exe` are
visible from the same shell that will run CMake.
`python tools/recoil_doctor.py --quick --binja` is the normal process-health
preflight; it does not replace the native x86 shell check before CMake work.

```powershell
cmake --preset ninja-x86-debug
cmake --build --preset ninja-x86-debug
ctest --preset ninja-x86-debug
```

VC6 verification manifests must compile production source through
`source_from`; manifest-local function copies and generated project-header
shadows are rejected unless they are recorded as existing baseline debt. Run
`python tools/recoil_vc6_manifest_source_guard.py` after changing verification
targets.

A Visual Studio Win32 solution can be generated with:

```powershell
cmake --preset vs-x86
```

The generated native solution is under `build/vs-x86`.

The local full verification workspace uses legacy SDK surfaces:

- `support/sdk/DirectX_Aug2007` for DirectDraw, DirectSound, DirectInput,
  DirectPlay, Direct3D Immediate Mode, and related libraries.
- `support/sdk/MFC42` for the Visual C++ 6.0-era MFC42 headers, libraries, DLL,
  and selected source evidence.

These SDK mirrors are ignored for public publication. Contributors who want to
run native ABI builds need to supply equivalent local files.

## Verification

Passing tests is necessary but not sufficient for a function to be complete.
Completion is tracked per function in `.agent/RECOIL_PLAN.md`:

- `Reconstructed` means the decompiled source model is accepted.
- `Source dependencies satisfied` means direct source/ABI dependencies are ready
  for implementation.
- `Reimplemented` means native source exists and compiles with the correct
  source-level behavior.
- `Binary-safe verified` means generated 32-bit assembly or provider ABI
  evidence has been compared against the original and accepted.

Useful commands:

```powershell
python tools/recoil_status.py
python tools/recoil_claim.py next --owner <name> --claim
python tools/recoil_plan_cli.py next
python tools/recoil_frontier.py 0x4301e0 --depth 1
python tools/recoil_asm_verify.py 0x407170
python tools/recoil_vc6_verify.py 0x407170
python tools/recoil_pe_reference.py --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil_plan_audit.py --summary
```

Use `recoil_claim.py` to coordinate active function ownership between agents.
`recoil_plan_cli.py next` is read-only navigation; it does not reserve work.

Commands that reference `support/Recoil.exe`, `support/zbd`, `support/sdk`,
`img/`, or a VC6 toolchain require local private inputs. They are documented so
the public repo can show the exact pipeline, but they are not expected to run in
a fresh public clone until those inputs are supplied locally.

Resource source is tracked in `src/Battlesport/Recoil.rc`, but the bitmap and
icon payloads are not. Regenerate them in a private workspace with:

```powershell
python tools/recoil_resource_extract.py
```

CTest also includes source guards that reject new raw original-image addresses,
raw assembly/naked functions, and `reinterpret_cast` uses in production
source. The Python tooling tests can also be run directly:

```powershell
python -m unittest discover -s tests/tools -p *_tests.py
```

## Documentation

- `AGENTS.md` - mandatory reconstruction workflow and source rules.
- `.agent/RECOIL_PLAN.md` - per-function reconstruction and verification state.
- `.agent/IMPLEMENTATION_GROUPS.md` - temporary dependency-group notes.
- `tools/README.md` - tool usage for plan navigation, dependency frontiers,
  assembly evidence, and source guards.
- `docs/reconstruction/knowledge_index.md` - entry point for durable
  reconstruction ledgers.
- `docs/reconstruction/agent_launch_checklist.md` - compact launch checklist
  for reconstruction agents.
- `docs/reconstruction/README.md` - rules for compact durable reconstruction
  notes.
- `docs/reconstruction/compiler_linker_provenance.md` - compiler/linker and
  provider evidence policy.
- `docs/reconstruction/source_file_map.md` - generated original-source to
  current-source placement map.
- `reconstruction/NOTES.md` - accumulated subsystem reconstruction notes and
  durable decompiled-source findings; archival source material, not the
  preferred location for new notes.

When adding reconstructed source, document durable facts in code where future
contributors need them: original address provenance, recovered layouts, ABI
constraints, ownership/cleanup order, provider assumptions, decompiler
limitations, and binary-verification-sensitive code shape.
Use `docs/reconstruction/` for cross-file, provider, ABI, file-format, or
toolchain facts that are likely to be rediscovered by multiple agents; keep
those notes compact and tied to evidence.

## License

The reconstructed source, tooling, tests, and documentation in this repository
are released under the MIT License; see `LICENSE`.

That license does not grant rights to the original *Recoil* game, original
retail executable, game data, extracted resources, third-party SDKs,
redistributable DLLs, decompiler databases, or compiler/toolchain payloads.
Those inputs are intentionally not distributed here.
