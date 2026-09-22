# Source guide

The directory names describe the reconstructed codebase. They do not imply that the original source files or original names were available.

## Game code

`Battlesport` contains the Windows application shell and most gameplay-facing code. Useful starting points include `RecoilApp.cpp` for application behavior, `mission.cpp` for mission state, `player.cpp` for player and control logic, `weapon.cpp` for weapons, and `hud.cpp` for the HUD and game dialogs. Networking and online-service integration are represented by `ai_net.cpp`, `WinSock.cpp`, and `WOL.cpp`.

The resource script and resource identifiers describe the application's Windows resources. Binary image assets referenced by the resource script are supplied separately.

## GameZ engine code

The `GameZRecoil` tree groups related engine code by subsystem. The `zClass` family covers world-object classes and their operations. Rendering and display code is distributed across `zRender`, `zVideo`, `zModel`, `zImage`, and `zGeometry`. Input, sound, timing, and platform services appear under `zInput`, `zSound`, `zTime`, and `zSys`. Math operations and shared geometric types are under `zMath`.

Some files retain a `.c` extension while being compiled as C++ in the reconstruction's recovered compiler context. File extensions alone therefore do not determine the language mode or calling convention.

## Other source families

`CZGameFrame` supplies application/frame integration. `Messages` reconstructs the companion message library, including its message definitions and export definition. `WinMain.cpp` provides the Windows executable entry point.

## Names, types, and dependencies

Some names are supported directly by executable evidence; others are inferred descriptions used to make the reconstruction understandable. A readable name alone is not proof of its historical spelling. Likewise, a partially reconstructed type should not be treated as a complete original class declaration.

The source uses the Windows, MFC, DirectX, and other interfaces expected by the legacy program. It also requires external compatibility headers and provider libraries.

See [reconstruction status](reconstruction-status.md) for the distinction between source recovery, authored-function matching, and whole-program verification.
