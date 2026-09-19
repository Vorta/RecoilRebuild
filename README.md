# Recoil Engine Reconstruction

A source-faithful native C/C++ reconstruction of Zipper Interactive's 1999 Windows x86 game **Recoil**, built with VC5SP3. The goal is complete typed reproduction of the retail executable: code, data, providers, resources, targets and layout. This is a reconstruction, not a modern port.

- `src/Battlesport` — game shell and gameplay.
- `src/GameZRecoil` — engine subsystems.
- `tools` and `tests/tools` — reconstruction tools and proof infrastructure.
- `docs/reconstruction` — procedures and supporting evidence.

Start with [AGENTS.md](AGENTS.md) and the [runbook index](docs/reconstruction/retail_executable_reproduction.md). Current status comes only from the machine-local tracker:

```powershell
python tools/recoil.py progress next --json
```

The public repository is source-only. It does not redistribute original executables, game data, extracted assets, decompiler databases, legacy SDKs, runtime DLLs or compiler toolchains.

The reconstructed source, tests and documentation use the MIT License; see `LICENSE`. That license grants no rights to the original game or any third-party assets, executable, SDK, runtime, database or toolchain payloads.
