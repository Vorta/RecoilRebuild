# Recoil Agent Tools

These tools support dependency-driven binary-safe reimplementation. They are helpers, not replacements for Binary Ninja evidence or plan marker rules.

## Plan Navigation

Use `recoil_plan_cli.py` for normal `.agent/RECOIL_PLAN.md` navigation and marker
updates. Do not use line-number dependent `Get-Content | Select-Object -Skip` reads except
as a temporary read-only fallback while diagnosing a broken plan CLI.

```powershell
python tools/recoil_plan_cli.py next
python tools/recoil_plan_cli.py show 0x464810
python tools/recoil_plan_cli.py find ClipPointList
python tools/recoil_plan_cli.py milestone M27
python tools/recoil_plan_cli.py milestone M27 --limit 10
```

Marker updates are one address and one criterion at a time:

```powershell
python tools/recoil_plan_cli.py set 0x464810 deps ❌ --dry-run
python tools/recoil_plan_cli.py set 0x464810 impl ✅ --name zFoo_Bar --file src/GameZRecoil/zFoo.cpp --evidence "built and source contract checked"
python tools/recoil_plan_cli.py set 0x464810 verify ✅ --evidence "ninja-x86-release .cod compared with BN 0x464810"
```

The tool preserves the plan entry shape and refuses `✅` or `☑️` updates without an
`--evidence` note. The note is printed for command history; durable evidence still belongs
in Binary Ninja comments, verification output, README, or narrow subsystem docs as required
by `AGENTS.md`.

## Dependency Frontier

Use this before implementing a caller or when an agent is unsure what should be worked on next:

```powershell
python tools/recoil_frontier.py 0x4301e0 --depth 1
```

Because this tool queries Binary Ninja, depth-1 reports can exceed 30 seconds on
large or recently analyzed functions. Agents should run it with at least a
120-second command timeout for `--depth 1`, and a longer timeout for `--depth 2`
or broader dependency groups.

The report names the anchor, direct callees visible from BN assembly/MLIL, plan status for
those callees, blocking dependencies, and a recommended next item. Plan status includes
`recon`, `deps`, `impl`, and `verify`.

Indirect calls such as vtable dispatches are listed separately as indirect calls. They are
not treated as unresolved named callees because the target must be resolved from Binary
Ninja typing, vtables, or call-site context.

`deps` is the human-audited `Source dependencies satisfied` marker from
`.agent/RECOIL_PLAN.md`. `❌` and `❓` are strict source blockers in this report; `✅` and
`☑️` are accepted for source-readiness. This marker helps choose work, but it does not
replace Binary Ninja inspection of direct calls, indirect/vtable calls, globals, shared
layouts, provider contracts, or caller-only ABI requirements.

Do not implement a caller while its `deps` status is `❓` or `❌`. Audit unknown
dependencies first, and switch to the lowest blocking dependency or dependency group when
the marker is not satisfied.

Use a larger depth only when the immediate callees are already implemented or when a short call chain is clearly one dependency group:

```powershell
python tools/recoil_frontier.py 0x4301e0 --depth 2
```

Limitations:

- It parses direct calls from BN assembly/MLIL.
- It does not prove indirect/vtable calls, globals, shared layouts, provider contracts, or
  caller-only ABI requirements.
- Always inspect Binary Ninja before editing source or updating markers.

## Assembly Evidence

Extract normalized BN assembly:

```powershell
python tools/recoil_asm_verify.py 0x407170
```

Compare a BN function to an MSVC `.cod` listing:

```powershell
python tools/recoil_asm_verify.py 0x407170 --cod build/ninja-x86-release/recoil_state_base.cod --symbol RecoilStateBase_ScalarDeletingDestructor
```

Outputs go to `build/verification/`:

- `<address>_bn.norm.asm`
- `<address>_msvc.norm.asm`
- `<address>_diff.txt`

The diff is normal completion evidence. Resolve differences before marking `Binary-safe verified`
or document them as relocation, symbol-spelling, listing-normalization, or toolchain-only
differences that do not change instructions, operands, control flow, call targets, stack/register
behavior, memory accesses, side effects, or floating-point behavior.

## Raw Original-Address Guard

Run the source guard directly when metadata, vtables, dispatch tables, or function-pointer code is edited:

```powershell
python tools/recoil_no_raw_image_addresses.py --root src --allowlist .agent/RAW_ADDRESS_ALLOWLIST.txt
```

Original executable addresses are evidence, not rebuilt pointers. Use named symbols, imports, or typed objects instead of adding new allowlist entries.

## Original Executable Reference

`support/Recoil.exe` is the original retail executable reference for final PE/import/resource/linkage comparison.
The durable facts are tracked in `.agent/REFERENCE_EXECUTABLE.json`:

```powershell
python tools/recoil_pe_reference.py --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
```

When a rebuilt executable exists, compare its PE facts against the reference:

```powershell
python tools/recoil_pe_reference.py --reference support/Recoil.exe --candidate build/path/Recoil.exe
```

## Final VC6 Candidate Build

Use this to run the standalone VC6 compile/resource/link pipeline outside the modern CMake
smoke build:

```powershell
python tools/recoil_vc6_build.py --dry-run
python tools/recoil_vc6_build.py --compile-only --keep-going
python tools/recoil_vc6_build.py
```

The manifest is `tools/vc6_final_build.json`. Outputs go to `build/vc6-final/`,
including object files, `Recoil.res`, `Recoil.exe`, `Recoil.map`, response files,
logs, and `summary.json`.

This driver is diagnostic while source compatibility is still being recovered. Current
source may fail under base VC6 until modern helper spelling such as `<cstdint>`,
`std::int32_t`, `constexpr`, `nullptr`, and related C++11-era constructs are replaced
with source-faithful 1999-compatible forms.

## Resource Extraction

Use this to regenerate the source-style VC6 resource script, image files, and resource
manifest from the original executable:

```powershell
python tools/recoil_resource_extract.py
```

Outputs:

- `img/GAMEBMP.bmp` and `img/RECOIL.ico` are editable/viewable image forms.
- `src/Battlesport/Recoil.rc` contains the reconstructed source-style menu, dialog,
  string-table, version, bitmap, and icon resources.
- `.agent/RESOURCE_MANIFEST.json` records IDs, languages, offsets, sizes, and hashes.

To verify a compiled resource file against `support/Recoil.exe`:

```powershell
python tools/recoil_resource_extract.py --compare-res build/resource-reconstruct/Recoil.res
```

Raw payloads and a direct `.res` copy can still be generated under `build/` for diagnostics
with `--write-raw` and `--write-res`; they are not source inputs for the final rebuild.

## Plan Consistency Audit

Use this to summarize marker debt without changing `.agent/RECOIL_PLAN.md`:

```powershell
python tools/recoil_plan_audit.py --summary
python tools/recoil_plan_audit.py --strict
```

`--strict` exits nonzero when it finds suspicious marker combinations, such as
`Reimplemented ✅` while source dependencies remain `❓` or `❌`.

## Implementation Group Audit

Use this to keep `.agent/IMPLEMENTATION_GROUPS.md` from becoming a second stale
plan. The audit is report-only and compares group addresses against
`.agent/RECOIL_PLAN.md`:

```powershell
python tools/recoil_groups_audit.py --summary
python tools/recoil_groups_audit.py --recommendation needs-review --details
python tools/recoil_groups_audit.py --recommendation condense
```

Recommendations mean:

- `keep-active`: source or dependency work is still pending.
- `condense`: source markers are accepted, but binary verification remains pending.
- `move-completed`: active group entries are all accepted and can be moved or removed.
- `needs-review`: nonstandard heading, missing plan address, empty group, or completed
  section entry with pending markers.
- `completed`: completed-section group still agrees with current plan markers.

Do not prune automatically from the report. Move durable facts into source comments,
Binary Ninja comments, tests, README, or narrow subsystem docs before removing group
notes.

## Reinterpret Cast Guard

Run the production-source cast guard when touching reconstructed C/C++ source:

```powershell
python tools/recoil_no_reinterpret_cast.py --root src --baseline .agent/REINTERPRET_CAST_BASELINE.txt
```

`reinterpret_cast` is not accepted in era-faithful production source. Existing matches are
tracked as baseline debt only so current cleanup can be incremental; do not add baseline
entries for new source. Prefer recovering stronger types, or use era-appropriate C-style
casts when a low-level conversion is genuinely required.
