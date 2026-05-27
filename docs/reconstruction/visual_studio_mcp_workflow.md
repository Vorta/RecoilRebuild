# Visual Studio MCP Development Workflow

Use Visual Studio MCP as the preferred IDE-backed workflow for the generated
`vs-x86` solution when Visual Studio is already available. MCP drives the IDE;
it does not replace Binary Ninja evidence, the repo verification tools, or the
installed MSVC toolchain.

## Scope

- Prefer MCP for `build/vs-x86/RecoilRebuildNative.slnx` projects such as
  `recoil_native`, `recoil_native_smoke`, `ALL_BUILD`, and `RUN_TESTS`.
- Use MCP for solution-aware navigation, build diagnostics, Error List triage,
  output panes, symbols, open editor state, and focused debugger sessions.
- Use `tools/recoil_msvc_x86_run.py -- ...` for Ninja presets, CMake configure
  commands, direct `ctest`, native smoke runs with command-line arguments, and
  any command-line work that MCP cannot express.
- Do not hand-write `cmd /c` commands that call Visual Studio batch files under
  `Program Files`.

## Session Setup

1. Check the current solution with `solution_info`.
2. If no solution is open, open
   `D:\Recoil Project\RecoilRebuild\build\vs-x86\RecoilRebuildNative.slnx`.
   If a different user solution is open, ask before closing it.
3. Run `project_list` and use the returned full `.vcxproj` path when building a
   single project.
4. Use `document_list` before editing files that may already be open in Visual
   Studio. If a relevant open document has unsaved editor state, read it with
   `document_read` and avoid overwriting the user's buffer.
5. Use `window_list` or `toolwindow_show` only when seeing or focusing a Visual
   Studio tool window will help diagnosis.

## Navigation And Context

- Use `symbol_document` for solution files when a class, method, or test file
  needs an outline faster than manual scanning.
- Use `document_read` for open buffers when the Visual Studio editor may contain
  unsaved changes that differ from disk. Use normal shell reads and `rg` for
  repo-wide source search.
- Use `selection_get` when the user has selected code or when the current
  editor selection is the intended context for a request.
- Do not treat Visual Studio navigation output as reconstruction evidence. It is
  a convenience layer over source currently loaded by the generated solution.

## Build And Diagnostics

1. Build focused targets with `build_project`; use `build_solution` only when a
   whole-solution check is appropriate.
2. Poll `build_status` until it reports `Done`.
3. If the build failed, inspect `errors_list` first.
4. Read the `Build` output pane with `output_read` when Error List entries are
   incomplete or when compiler/linker command context matters.
5. Use `clean_solution` only when stale generated outputs are a plausible cause;
   prefer normal incremental builds for routine checks.
6. Use `build_cancel` if a Visual Studio build is clearly stuck or the user
   redirects the task.

## Debugging

- Use `startup_project_get` and `startup_project_set` before launching when the
  current startup project is unknown or wrong.
- Use `debugger_add_breakpoint`, `debugger_launch`, `debugger_status`,
  `debugger_get_callstack`, `debugger_get_locals`, and step/continue commands
  for focused runtime diagnosis after the relevant target builds.
- Use `debugger_launch_without_debugging` for manual runtime sanity checks where
  no breakpoints or locals are needed.
- `recoil_native_smoke.exe` runs all registered smokes when launched without an
  argument, and the isolated runner passes one smoke name at a time. Prefer
  `tests/native/run_native_smokes.py`, `ctest`, or
  `tools/recoil_functional_verify.py` for smoke runs that need explicit
  arguments.
- `debugger_set_variable` is for temporary diagnosis only. Do not use modified
  debugger state as functional-equivalence, binary-safe, or plan-marker
  evidence.
- Stop the debugger with `debugger_stop` before ending a session if the launched
  process is still running.

## Editing Policy

- Make normal repo edits with `apply_patch` so diffs are explicit and saved.
- Do not use `document_write` or `editor_insert` for routine reconstruction
  edits. Reserve them for explicit user-directed editor interactions or tiny
  temporary experiments that will be reconciled through a normal file patch.
- Never overwrite unsaved user editor buffers. Check with `document_list` and
  `document_read` when Visual Studio is part of the current workflow.

## Functional Smokes

Building `recoil_native_smoke` through MCP writes the executable under the
Visual Studio output tree, for example:

```text
build/vs-x86/tests/native/Debug/recoil_native_smoke.exe
```

`tools/recoil_functional_verify.py` checks this path automatically when the
Ninja smoke executable is absent. Pass `--executable` only when using a custom
output location.
