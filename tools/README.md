# Recoil Agent Tools

These tools support the dependency-driven, binary-safe workflow described in `AGENTS.md`. They produce evidence and reports; they do not replace Binary Ninja inspection or plan marker rules.

## Plan Navigation

Use `recoil_status.py` as the first-stop agent summary when choosing or verifying an address:

```powershell
python tools/recoil_status.py
python tools/recoil_status.py 0x40c370
python tools/recoil_status.py 0x40c370 --no-frontier
```

It combines the active plan entry, claim state, implementation-group context, VC6 manifest coverage, a depth-1 Binary Ninja frontier summary, and the exact next verification or manifest-scaffold command.

Use `recoil_claim.py` for local address coordination before editing function-scoped work:

```powershell
python tools/recoil_claim.py next --owner <name> --claim
python tools/recoil_claim.py claim 0x464810 --owner <name>
python tools/recoil_claim.py status 0x464810
python tools/recoil_claim.py release 0x464810 --token <token>
python tools/recoil_claim.py prune-stale
```

Claim files are local runtime locks under `.agent/claims/functions/`. They prevent duplicate agent work on the same original address; they are not progress evidence and do not replace plan markers.

Use `recoil_plan_cli.py` for normal `.agent/RECOIL_PLAN.md` navigation and one-entry marker updates:

```powershell
python tools/recoil_plan_cli.py next
python tools/recoil_plan_cli.py show 0x464810
python tools/recoil_plan_cli.py find ClipPointList
python tools/recoil_plan_cli.py milestone M27 --limit 10
python tools/recoil_plan_cli.py set 0x464810 impl ✅ --name zFoo_Bar --file src/GameZRecoil/zFoo.cpp --evidence "built and source contract checked"
```

`recoil_plan_cli.py next` is navigation only; use `recoil_claim.py next --owner <name> --claim` when assigning active work. The plan tool preserves entry shape and refuses `✅` or `☑️` updates without `--evidence`. Durable evidence belongs in Binary Ninja comments, verification output, source comments, README, or narrow subsystem docs.

## Dependency Frontier

Use `recoil_frontier.py` before implementing a caller or when choosing the next dependency:

```powershell
python tools/recoil_frontier.py 0x4301e0 --depth 1
python tools/recoil_frontier.py 0x4301e0 --depth 2
python tools/recoil_frontier.py 0x4301e0 --depth 1 --json
```

Depth-1 reports can exceed 30 seconds because the tool queries Binary Ninja. Use at least a 120-second timeout for `--depth 1` and longer timeouts for `--depth 2` or broader groups.

The report lists direct callees visible from Binary Ninja assembly/MLIL, indirect calls, plan status, VC6 manifest coverage, blockers, and a recommended next item. It does not prove vtable targets, globals, shared layouts, provider contracts, or caller-only ABI requirements; inspect Binary Ninja before source edits or marker updates.

## Assembly Evidence

Extract normalized Binary Ninja assembly:

```powershell
python tools/recoil_asm_verify.py 0x407170
```

Compare a function against a compiler `.cod` listing:

```powershell
python tools/recoil_asm_verify.py 0x407170 --cod build/ninja-x86-release/recoil_state_base.cod --symbol RecoilStateBase_ScalarDeletingDestructor
```

Compare a function against a VC6 COFF object using relocation-masked bytes:

```powershell
python tools/recoil_asm_verify.py 0x407170 --obj build/vc6-verify/recoil_state_base/RecoilStateBase.obj --cod build/vc6-verify/recoil_state_base/RecoilStateBase.cod --symbol "?ScalarDeletingDestructor@RecoilStateBase@@QAEPAURecoilApp_IState@@I@Z"
```

Outputs go to `build/verification/`. The object-byte comparison is the preferred gate: it compares original Binary Ninja bytes to VC6 object bytes and masks only COFF relocation fields. `.cod` text and normalized assembly are evidence for review, not the pass/fail authority.

## VC6 Verification

List and run tracked VC6 verification manifests:

```powershell
python tools/recoil_vc6_verify.py --list
python tools/recoil_vc6_verify.py <target-name>
python tools/recoil_vc6_verify.py 0xNNNNNN
python tools/recoil_vc6_verify.py --source-from src/GameZRecoil/zHud/zhud_ui.cpp --skip-bn-compare
python tools/recoil_vc6_verify.py --all --skip-bn-compare
python tools/recoil_vc6_verify.py --explain-missing 0xNNNNNN
```

`--list` is current coverage, not a whitelist. `--explain-missing` prints existing coverage for an address, or a starter JSON manifest shape when coverage is absent. If an implemented authored function has no target, add or extend a JSON manifest under `tools/vc6_verify_targets/`. Production functions must use `source_from` so VC6 verifies the actual `src/` implementation. Manifest-local source bodies and generated project-header shadows are rejected unless they are recorded as existing baseline debt in `.agent/VC6_MANIFEST_SOURCE_POLICY_BASELINE.txt`; that baseline should stay empty after the cleanup.

Run the source-policy guard when touching verification manifests:

```powershell
python tools/recoil_vc6_manifest_source_guard.py
```

Artifacts are written under `build/vc6-verify/<target>/` for single-target runs and `build/vc6-verify/_batch/` for grouped batch runs, including generated sources, `.cod` listings, COFF objects, relocation masks, byte diffs, byte triage reports, normalized assembly text, and summaries. Review the byte diff and triage report before changing plan markers. Passing single-target runs print a marker-update-ready evidence block; review it, but still update plan markers manually.
Default manifests use COFF byte comparison. Passing output means unmasked generated bytes match after relocation masking and trailing alignment NOP trimming. A `text` compare mode remains available only for legacy diagnostics.

## Environment Checks

Run the standard process preflight:

```powershell
python tools/recoil_doctor.py --quick
python tools/recoil_doctor.py --quick --binja
python tools/recoil_doctor.py --active 0x415220
python tools/recoil_doctor.py --active 0x415220 --bn-compare
```

The doctor command runs the manifest source policy guard, source guards, PE reference verification, VC6 manifest load, and Python tool tests. Add `--binja` when Binary Ninja is expected to be open and the bridge/database state should be checked. Plain `--quick` remains usable when Binary Ninja is intentionally unavailable or not needed. `--active` adds status plus compile-only VC6 coverage for one address unless `--bn-compare` is provided.

Check the local x86 MSVC native build environment before configuring CMake:

```powershell
python tools/recoil_env_check.py --native-x86
python tools/recoil_doctor.py --quick --native-x86
```

The check reports missing compiler, `INCLUDE`/`LIB`, Windows SDK headers, x86
target state, private `support/Recoil.exe`, and the repo-local DirectX/MFC42
mirrors. Run it from the same shell that will run `cmake --preset ninja-x86-debug`.

## Final VC6 Candidate Build

Run the standalone VC6 compile/resource/link pipeline outside modern CMake smoke builds:

```powershell
python tools/recoil_vc6_build.py --dry-run
python tools/recoil_vc6_build.py --compile-only --keep-going
python tools/recoil_vc6_build.py
```

The manifest is `tools/vc6_final_build.json`; outputs go to `build/vc6-final/`. Set `RECOIL_VC6_ROOT` when the portable VC6 toolchain is not in the default sibling `..\Compiler\VC6` location.

## Resource Extraction

Regenerate source-style resources from the private original executable:

```powershell
python tools/recoil_resource_extract.py
python tools/recoil_resource_extract.py --compare-res build/resource-reconstruct/Recoil.res
```

Expected source outputs are `src/Battlesport/Recoil.rc`, `img/GAMEBMP.bmp`, `img/RECOIL.ico`, and `.agent/RESOURCE_MANIFEST.json`. Raw payloads and direct `.res` copies are diagnostic build outputs only.

## Reference Executable

Verify recorded PE/import/resource facts or compare a rebuilt candidate:

```powershell
python tools/recoil_pe_reference.py --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify
python tools/recoil_pe_reference.py --reference support/Recoil.exe --candidate build/path/Recoil.exe
```

## Audits

Run report-only audits before broad planning or group cleanup:

```powershell
python tools/recoil_plan_audit.py --summary
python tools/recoil_plan_audit.py --strict
python tools/recoil_groups_audit.py --summary
python tools/recoil_groups_audit.py --recommendation needs-review --details
python tools/recoil_groups_audit.py --recommendation condense
```

Do not prune group notes automatically. Move durable facts to source comments, Binary Ninja comments, tests, README, or narrow subsystem docs first.

## Tool Tests

Run Python tool tests directly, or through CTest after configuring a native preset:

```powershell
python -m unittest discover -s tests/tools -p *_tests.py
ctest --preset ninja-x86-debug -R recoil_tools_unittest
```

## Source Guards

Run focused guards when touching production source:

```powershell
python tools/recoil_no_raw_image_addresses.py --root src --allowlist .agent/RAW_ADDRESS_ALLOWLIST.txt
python tools/recoil_no_raw_assembly.py --root src --allowlist .agent/RAW_ASSEMBLY_ALLOWLIST.txt
python tools/recoil_no_reinterpret_cast.py --root src
python tools/recoil_no_reinterpret_cast.py --root src --summary
```

CTest also runs the raw-address, raw-assembly, and reinterpret-cast guards for configured native builds.
