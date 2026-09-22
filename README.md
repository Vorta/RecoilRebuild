# Recoil source reconstruction

A source-faithful native C/C++ reconstruction of **Recoil**, Zipper Interactive's 1999 Windows game. The project studies the original Windows x86 executable and reconstructs the game's source structure, types, behavior, and compiled representation using Visual C++ 5.0 Service Pack 3.

The aim is a complete, verifiable reconstruction of the retail program: authored code and data, library boundaries, resources, and the final executable layout. This is an ongoing reconstruction, not a finished source release or a modern port.

## Current state

The source tree contains the game shell and gameplay systems, the GameZ engine subsystems, and the companion Messages DLL source. Work continues on matching authored functions and on proving their relationships to data, libraries, and the linked executable.

**Status snapshot: 22 September 2026.** A fresh VC5SP3 build successfully compiled all 92 translation units in the main executable configuration, compiled the Windows resources, and linked a candidate `Recoil.exe`. This establishes buildability in the configured VC5SP3 environment; it does not establish retail byte equivalence or end-to-end playability.

The current tracker task is the **authored-function census** for [the HUD source](src/Battlesport/hud.cpp), beginning at retail address `0x404CA0`: proving that the complete expected authored identities and source bindings are represented. The source retains **915 historical function-match annotations**; the current freshness audit marks all 915 as needing revalidation. These annotations record prior classifications, not a current count of verified matches. No whole-game completion percentage or complete retail-executable match is claimed.

Individual source annotations describe reconstruction evidence at particular points in development. They should be read alongside the [status and verification terminology](docs/reconstruction-status.md); the presence of source code or a matching function does not establish completion of an entire subsystem.

## Reading the source

| Location | Contents |
| --- | --- |
| `src/Battlesport/` | Application shell, menus and HUD, missions, players, weapons, networking, and gameplay. |
| `src/GameZRecoil/` | Engine systems including rendering, models, geometry, input, sound, math, and world objects. |
| `src/CZGameFrame/` | Game frame and application integration. |
| `src/Messages/` | Companion message-library source and message definitions. |
| `src/WinMain.cpp` | Windows executable entry point. |

See the [source guide](docs/source-guide.md) for an overview of the main code families and how to interpret recovered names and types.

## Repository contents

This repository contains reconstructed game source and documentation for readers studying that source.

The checkout is not a standalone build package. Building and verifying the reconstruction requires Visual C++ 5.0 SP3, the appropriate legacy SDKs, and external build dependencies and reference inputs. Original executables, extracted assets, and game data are not distributed here.

## License

The reconstructed source and documentation are provided under the [MIT License](LICENSE). This license grants no rights to the original game or to third-party assets, executables, SDKs, runtimes, or compiler toolchains. Recoil and its original materials remain the property of their respective rights holders.
