# Visual Studio MCP Workflow

Use Visual Studio MCP for generated `vs-x86` solution builds when Visual Studio
is already available. MCP drives the IDE; it does not replace the installed MSVC
toolchain.

## Build Routing

- Prefer MCP for `build/vs-x86/RecoilRebuildNative.slnx` projects such as
  `recoil_native`, `recoil_native_smoke`, `ALL_BUILD`, and `RUN_TESTS`.
- Use `tools/recoil_msvc_x86_run.py -- ...` for Ninja presets, CMake configure
  commands, direct `ctest`, or commands that need arbitrary shell arguments.
- Do not hand-write `cmd /c` commands that call Visual Studio batch files under
  `Program Files`.

## MCP Sequence

1. Check the current solution with `solution_info`.
2. If no solution is open, open
   `D:\Recoil Project\RecoilRebuild\build\vs-x86\RecoilRebuildNative.slnx`.
   If a different user solution is open, ask before closing it.
3. Run `project_list` and use the returned full `.vcxproj` path when building a
   single project.
4. Build with `build_project` for a focused target or `build_solution` for the
   full solution.
5. Poll `build_status` until it reports `Done`.
6. If the build failed, inspect `errors_list` first, then read the `Build`
   output pane when available.

## Functional Smokes

Building `recoil_native_smoke` through MCP writes the executable under the
Visual Studio output tree, for example:

```text
build/vs-x86/tests/native/Debug/recoil_native_smoke.exe
```

`tools/recoil_functional_verify.py` checks this path automatically when the
Ninja smoke executable is absent. Pass `--executable` only when using a custom
output location.
