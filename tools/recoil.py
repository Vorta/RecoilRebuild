#!/usr/bin/env python3
"""Unified agent-facing gate for Recoil reconstruction tools."""

from __future__ import annotations

from dataclasses import dataclass
import contextlib
import difflib
import importlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
from typing import Any, Iterable, Mapping, Sequence

from _recoil.lib.tooling import REPO_ROOT, configure_stdio
from _recoil.lib.progress import DEFAULT_PROGRESS_PATH
from _recoil.lib.progress_sqlite import ProgressSQLiteError, read_progress_metadata


TOOLS_DIR = REPO_ROOT / "tools"


@dataclass(frozen=True)
class CommandSpec:
    path: tuple[str, ...]
    module: str
    prepend_args: tuple[str, ...] = ()
    summary: str = ""
    description: str = ""
    category: str = "general"
    examples: tuple[str, ...] = ()
    mutates: bool = False
    needs_binja: bool = False
    required_revision_domains: tuple[str, ...] = ()
    packet_binding: str = "none"
    build_root_contract: str = "none"
    ledger_routing: str = "default"
    mutation_scope: str = "none"

    @property
    def name(self) -> str:
        return " ".join(self.path)

    @property
    def module_name(self) -> str:
        return f"_recoil.commands.{self.module}"


def spec(
    path: str,
    module: str,
    *,
    prepend: tuple[str, ...] = (),
    summary: str,
    description: str = "",
    category: str = "general",
    examples: tuple[str, ...] = (),
    mutates: bool = False,
    needs_binja: bool = False,
    required_revision_domains: tuple[str, ...] = (),
    packet_binding: str = "none",
    build_root_contract: str = "none",
    ledger_routing: str = "default",
    mutation_scope: str = "none",
) -> CommandSpec:
    return CommandSpec(
        path=tuple(path.split()),
        module=module,
        prepend_args=prepend,
        summary=summary,
        description=description or summary,
        category=category,
        examples=examples,
        mutates=mutates,
        needs_binja=needs_binja,
        required_revision_domains=required_revision_domains,
        packet_binding=packet_binding,
        build_root_contract=build_root_contract,
        ledger_routing=ledger_routing,
        mutation_scope=mutation_scope,
    )


_BASE_COMMAND_SPECS: tuple[CommandSpec, ...] = (
    spec("progress next", "progress_cli", prepend=("next",), summary="Select the sole authoritative next Recoil.exe reconstruction task from the unified progress tracker.", category="progress", examples=("python tools/recoil.py progress next", "python tools/recoil.py progress next --json")),
    spec("progress status", "progress_cli", prepend=("status",), summary="Show derived unified pipeline or selector status.", category="progress", examples=("python tools/recoil.py progress status", "python tools/recoil.py progress status --binary messages", "python tools/recoil.py progress status 0x401060 --json")),
    spec("progress show", "progress_cli", prepend=("show",), summary="Show a joined owner/block/semantic/order/link/byte view.", category="progress", examples=("python tools/recoil.py progress show 0x401000", "python tools/recoil.py progress show recoil:owner:misc_unresolved.cabout_dlg")),
    spec("progress find", "progress_cli", prepend=("find",), summary="Search all unified reconstruction progress entities.", category="progress", examples=("python tools/recoil.py progress find CAboutDlg",)),
    spec("progress audit", "progress_cli", prepend=("audit",), summary="Audit unified tracker schema, relationships, evidence, and derived pipeline invariants.", category="progress", examples=("python tools/recoil.py progress audit --strict", "python tools/recoil.py progress audit --scope evidence --strict")),
    spec("progress compact", "progress_cli", prepend=("compact",), summary="Parent-only no-archive active-only schema-v5 tracker compaction with exact scheduler parity.", category="progress", examples=("python tools/recoil.py progress compact --expected-revision <revision> --dry-run --json",), mutates=True),
    spec("progress report", "progress_cli", prepend=("report",), summary="Render an on-demand unified progress report without creating a shadow tracker.", category="progress", examples=("python tools/recoil.py progress report --format markdown",)),
    spec("docs readme-progress", "readme_progress", summary="Update or check the deterministic public README progress snapshot derived from the unified tracker.", category="docs", examples=("python tools/recoil.py docs readme-progress", "python tools/recoil.py docs readme-progress --check --json"), mutates=True),
    spec(
        "maintenance migrate-ledgers-sqlite",
        "ledger_sqlite_migration",
        summary="Parent-only guarded direct cutover of both legacy JSON authorities to a matched SQLite database pair.",
        category="validation",
        examples=("python tools/recoil.py maintenance migrate-ledgers-sqlite --progress-json <absolute-legacy-progress-json> --issues-json <absolute-legacy-issues-json> --progress-db <absolute-noncanonical-progress-db> --issues-db <absolute-noncanonical-issues-db> --expected-progress-revision <revision> --expected-issues-revision <revision> --dry-run --json",),
        mutates=True,
    ),
    spec("policy show", "policy_cli", prepend=("show",), summary="Show one machine-readable reconstruction scheduling policy.", category="docs", examples=("python tools/recoil.py policy show authored-order --json",)),
    spec("verify linked-byte", "live_byte_verify", prepend=("linked",), summary="Freshly rebuild and directly scan the linked-byte lane, stopping at the earliest real divergence.", category="verification", examples=("python tools/recoil.py verify linked-byte", "python tools/recoil.py verify linked-byte --at 0x401000")),
    spec("verify authored-byte", "live_byte_verify", prepend=("authored",), summary="Freshly rebuild and directly scan authored object, relocation, target, and linked-body semantics.", category="verification", examples=("python tools/recoil.py verify authored-byte", "python tools/recoil.py verify authored-byte --at 0x401000")),
    spec("verify authored-object-byte", "live_byte_verify", prepend=("object",), summary="Freshly compile and directly scan authored object bodies outside relocation fields.", category="verification", examples=("python tools/recoil.py verify authored-object-byte", "python tools/recoil.py verify authored-object-byte --at 0x401180")),
      spec(
          "verify call-contract",
          "call_contract_verify",
          summary="Freshly compile one deterministic authored-body slice, or one nonaccepting registered-target convergence scope, and compare exact static invocation contracts with retail Binary Ninja evidence.",
          category="verification",
          examples=(
              "python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json",
              "python tools/recoil.py verify call-contract --slice <slice-id> --packet-id <packet-id> --verification-currentness-audit --build-root <packet-root> --json",
              "python tools/recoil.py verify call-contract --target <target-id> --all-authored-bodies --build-root <fresh-root> --json",
          ),
        needs_binja=True,
    ),
    spec("verify final-image", "live_final_verify", summary="Freshly build and validate complete typed PE semantics against retail; raw file differences and COFF time are diagnostic only.", category="verification", examples=("python tools/recoil.py verify final-image", "python tools/recoil.py verify final-image --candidate build/vc5-final/Recoil.exe --map build/vc5-final/Recoil.map --json"), mutates=True),
    spec("verify authored-order scaffold", "authored_order", prepend=("scaffold",), summary="Draft or explicitly write a fail-closed current-block authored-order VC5 manifest candidate.", category="verification", examples=("python tools/recoil.py verify authored-order scaffold 0x401060 --json", "python tools/recoil.py verify authored-order scaffold 0x401060 --output build/reconstruction-evidence/candidates/order_401060.json --json"), mutates=True),
    spec("verify authored-order sweep", "authored_order", prepend=("sweep",), summary="Read-only mechanical scaffold-readiness sweep across remaining authored-order blocks.", category="verification", examples=("python tools/recoil.py verify authored-order sweep --from-current --json",)),
    spec("issue report", "workspace_issues", prepend=("report",), summary="Record an agent tooling/process problem for a future agent to fix.", category="issue", examples=("python tools/recoil.py issue report --kind tool-error --severity high --summary \"...\" --area tools/recoil.py --impact \"...\" --actual \"...\" --repro \"...\" --next-action \"...\"",), mutates=True),
    spec("issue request", "workspace_issues", prepend=("request",), summary="Record an agent tooling/process improvement request.", category="issue", examples=("python tools/recoil.py issue request --severity medium --summary \"...\" --area tools/recoil.py --impact \"...\" --requested-change \"...\" --benefit \"...\" --next-action \"...\"",), mutates=True),
    spec("issue list", "workspace_issues", prepend=("list",), summary="List open agent tooling/process issue reports.", category="issue", examples=("python tools/recoil.py issue list --status open",)),
    spec("issue show", "workspace_issues", prepend=("show",), summary="Show one agent tooling/process issue report.", category="issue", examples=("python tools/recoil.py issue show WSI-YYYYMMDD-NNN",)),
    spec("issue resolve", "workspace_issues", prepend=("resolve",), summary="Mark an agent tooling/process issue resolved.", category="issue", examples=("python tools/recoil.py issue resolve WSI-YYYYMMDD-NNN --resolution \"...\"",), mutates=True),
    spec("issue wont-fix", "workspace_issues", prepend=("wont-fix",), summary="Close an agent tooling/process issue without resolution and remove its terminal active-only rows.", category="issue", mutates=True),
    spec("issue reopen", "workspace_issues", prepend=("reopen",), summary="Reopen an agent tooling/process issue.", category="issue", examples=("python tools/recoil.py issue reopen WSI-YYYYMMDD-NNN --reason \"...\"",), mutates=True),
    spec("issue compact", "workspace_issues", prepend=("compact",), summary="Parent-only no-archive active-only workspace-issue ledger compaction.", category="issue", examples=("python tools/recoil.py issue compact --expected-revision <revision> --dry-run --json",), mutates=True),
    spec("issue audit", "workspace_issues", prepend=("audit",), summary="Validate the agent tooling/process issue ledger shape.", category="issue", examples=("python tools/recoil.py issue audit --strict",)),
    spec("issue work set", "workspace_issues", prepend=("work", "set"), summary="Create one revision-guarded workspace-issue work packet.", category="issue", mutates=True),
    spec("issue work list", "workspace_issues", prepend=("work", "list"), summary="List explicit workspace-issue work packets.", category="issue"),
    spec("issue work show", "workspace_issues", prepend=("work", "show"), summary="Show one workspace-issue packet and reservation history.", category="issue"),
    spec("issue work reserve", "workspace_issues", prepend=("work", "reserve"), summary="Reserve one ready workspace-issue packet after global conflict checking.", category="issue", mutates=True),
    spec("issue work close", "workspace_issues", prepend=("work", "close"), summary="Release and close one active workspace-issue packet.", category="issue", mutates=True),
    spec("workspace worktree status", "worktree_control", prepend=("status",), summary="Inspect canonical and linked Git worktrees, packet associations, build roots, and branch hygiene.", category="issue"),
    spec("workspace worktree create", "worktree_control", prepend=("create",), summary="Parent-only creation and reservation of one linked workspace-issue packet worktree.", category="issue", mutates=True),
    spec("workspace worktree validate", "worktree_control", prepend=("validate",), summary="Validate one linked workspace-issue packet commit and exact authored closure.", category="issue"),
    spec("workspace worktree integrate", "worktree_control", prepend=("integrate",), summary="Parent-only validated temporary-worktree integration of one packet branch into master.", category="issue", mutates=True),
    spec("workspace worktree retire", "worktree_control", prepend=("retire",), summary="Parent-only removal of one integrated packet worktree, branch, and authenticated build root.", category="issue", mutates=True),
    spec("workspace worktree hygiene", "worktree_control", prepend=("hygiene",), summary="Audit branch, linked-worktree, association, and external-build-root hygiene.", category="issue"),
    spec(
        "doctor",
        "doctor",
        summary="Run categorized full or infrastructure-only process-health checks.",
        category="validation",
        examples=(
            "python tools/recoil.py doctor --quick --binja",
            "python tools/recoil.py doctor --infrastructure-only",
        ),
        needs_binja=True,
    ),
    spec("env", "env_check", summary="Check local native build environment.", category="validation", examples=("python tools/recoil.py env --native-x86",)),
    spec("verify functional", "functional_verify", summary="List or run tier C functional smoke evidence.", category="verification", examples=("python tools/recoil.py verify functional 0xNNNNNN",)),
    spec(
        "verify functional-batch",
        "functional_verify",
        prepend=("batch",),
        summary="Run multiple tier C functional smoke targets.",
        description="Run the same functional manifest checks as verify functional for each supplied target, then print a per-target pass/fail summary and return nonzero if any target fails.",
        category="verification",
        examples=("python tools/recoil.py verify functional-batch --dry-run 0x401060 0x401180",),
    ),
    spec(
        "verify vc5",
        "vc5_verify",
        summary="List or run owner-scoped VC5SP3 COFF function/data-symbol verification.",
        description="List or run VC5SP3 COFF function/data-symbol verification. Tier S owner byte-gate evidence should use --owner so every linked source-owner row is covered before compilation; owner rows with duplicate diagnostic data-symbol coverage prefer the row's owner target metadata when it uniquely identifies a manifest. Explicit target/address selectors remain available for diagnostics and manifest development. Multiple explicit target/address selectors are grouped by identical compiles; multi-data-symbol BN comparisons are internally auto-chunked under the bridge call budget; live BN hexdumps route through the manifest target_binary or --binary override.",
        category="verification",
        examples=(
            "python tools/recoil.py verify vc5 --owner source.owner_id --auto-chunk",
            "python tools/recoil.py verify vc5 0xNNNNNN",
            "python tools/recoil.py verify vc5 data_target_name",
            "python tools/recoil.py verify vc5 --target target_name",
            "python tools/recoil.py verify vc5 target_name --auto-chunk",
            "python tools/recoil.py verify vc5 messages_lookup_data --binary messages --auto-chunk",
            "python tools/recoil.py verify vc5 target_name --chunk-size 200",
            "python tools/recoil.py verify vc5 0x401000 0x401020 --skip-bn-compare",
            "python tools/recoil.py verify vc5 --targets-json '[\"0x401000\", \"target_name\"]'",
        ),
        needs_binja=True,
    ),
    spec(
        "verify vc5-order",
        "vc5_verify",
        prepend=("--order-only",),
        summary="Compile one VC5SP3 order target and report the first retail-order divergence directly.",
        description=(
            "Run one registered function-order target in an isolated build root without Binary "
            "Ninja or byte comparison. The source-edit loop is live and receipt-free; ordinary "
            "comments do not participate in validation, while address-bearing markers remain exact. "
            "A full-order packet pairs the compiling object target with --linked-target so the same "
            "non-mutating loop reports exact linked selected-population and seam divergence."
        ),
        category="verification",
        examples=(
            "python tools/recoil.py verify vc5-order target_name --build-root build/vc5-order/target_name",
            "python tools/recoil.py verify vc5-order object_target --linked-target linked_target --build-root build/vc5-order/full-target",
        ),
        needs_binja=False,
    ),
    spec(
        "verify vc5-abi-equivalence",
        "vc5_abi_equivalence",
        summary="Prove one manifest-owned zero-argument identity is mechanically equivalent under fresh VC5 /Gd and /Gr builds.",
        description=(
            "Compile the selected manifest zeroarg_abi_equivalence identity under paired /Gd and /Gr "
            "profiles, using its exact callee_source and each caller row's exact source, and compare "
            "eligibility, callee/callsite bytes, relocation semantics, EH shape, translation-unit-wide "
            "raw definition order normalized over exact manifest pairs plus mechanically unique VC5 "
            "zero-argument definition counterparts, and COMDAT selection. "
            "A PASS authorizes only mechanical explicit __cdecl normalization for the one selected "
            "identity; it does not establish original ABI or mutate acceptance state."
        ),
        category="verification",
        examples=(
            "python tools/recoil.py verify vc5-abi-equivalence --target <target> --build-root build/vc5-abi/<target> --json",
        ),
    ),
    spec("verify asm", "asm_verify", summary="Extract or compare Binary Ninja assembly/bytes.", category="verification", examples=("python tools/recoil.py verify asm 0xNNNNNN",), needs_binja=True),
    spec(
        "verify pe",
        "pe_reference",
        summary="Verify or compare PE reference executable/DLL facts.",
        category="verification",
        examples=(
            "python tools/recoil.py verify pe --reference support/Recoil.exe --manifest .agent/REFERENCE_EXECUTABLE.json --verify",
            "python tools/recoil.py verify pe --reference support/messages.dll --manifest .agent/REFERENCE_MESSAGES_DLL.json --verify",
        ),
    ),
    spec(
        "verify final-build",
        "vc5_build",
        summary="Run the final VC5SP3 executable/DLL build pipeline.",
        category="verification",
        examples=(
            "python tools/recoil.py verify final-build --dry-run",
            "python tools/recoil.py verify final-build --manifest tools/_recoil/config/vc5_messages_build.json --dry-run",
        ),
        mutates=True,
    ),
    spec(
        "verify linked-order",
        "linked_order",
        summary="Compile/link current VC5SP3 code and report one authored/full linked-order divergence.",
        description=(
            "Run a focused live linked function-order comparison without order receipts, PE/resource "
            "comparison, byte gates, or raw whole-file validation."
        ),
        category="verification",
        examples=(
            "python tools/recoil.py verify linked-order target_name --scope authored --build-root build/vc5-order/target_name/linked",
            "python tools/recoil.py verify linked-order target_name --scope full --build-root build/vc5-order/target_name/linked",
        ),
        mutates=True,
    ),
    spec("audit artifacts", "artifact_audit", summary="Dry-run or empty the complete direct build directory while retaining build/, with explicit local-root and session-scratch modes.", category="audit", examples=("python tools/recoil.py audit artifacts", "python tools/recoil.py audit artifacts --delete", "python tools/recoil.py audit artifacts --include-vs --older-than-days 30", "python tools/recoil.py audit artifacts --session-only", "python tools/recoil.py audit artifacts --session-only --delete"), mutates=True),
    spec("audit current-metadata", "current_metadata_audit", summary="Reject stale static current-cursor narratives and validate generated current metadata against progress next.", category="audit", examples=("python tools/recoil.py audit current-metadata --strict",)),
    spec("audit state-performance", "state_performance_audit", summary="Measure fresh-process SQLite ledger reads and bounded scratch-copy transactions against governed performance ceilings.", category="audit", examples=("python tools/recoil.py audit state-performance --strict", "python tools/recoil.py audit state-performance --json")),
    spec("audit final-image-catalog", "final_image_catalog_audit", summary="Derive and audit complete typed final-image coverage live from retail plus accepted tracker facts, without building a candidate.", category="audit", examples=("python tools/recoil.py audit final-image-catalog --json",)),
    spec("audit relocation-expectations", "relocation_expectations", summary="Derive candidate-independent authored relocation expectations live from retail and accepted typed identities.", category="audit", examples=("python tools/recoil.py audit relocation-expectations --at 0x401000 --json",)),
    spec(
        "audit final-data",
        "final_data_diff",
        summary="Diagnose final-build .data section size and variable drift.",
        description="Read-only diagnostic for final-build .data drift: reports PE section deltas, candidate map boundary symbols, object .data/.bss contributors, optional per-object COFF/map/link-order traces, data-symbol manifest coverage, and available BN data coverage without updating markers or ledgers.",
        category="audit",
        examples=(
            "python tools/recoil.py audit final-data --limit 40",
            "python tools/recoil.py audit final-data --trace-object player.obj,ainet.obj --limit 40",
        ),
        needs_binja=True,
    ),
    spec(
        "audit bn-data-evidence",
        "bn_data_evidence",
        summary="Collect read-only Binary Ninja evidence for a data range.",
        description=(
            "Read-only Binary Ninja data-owner recovery probe. Reports BN data declarations, hexdump bytes, "
            "nearby data variables, xrefs to the base/range aligned addresses where the bridge exposes them, "
            "assembly-text fallback hits, derived constants, and explicit unsupported relocation/global-search fields."
        ),
        category="audit",
        examples=(
            "python tools/recoil.py audit bn-data-evidence 0x4e5954 --size 0xfc --constants float --nearby 0x60 --json",
        ),
        needs_binja=True,
    ),
    spec("audit provenance", "provenance_audit", summary="Audit compiler/linker provenance assumptions.", category="audit", examples=("python tools/recoil.py audit provenance --strict",)),
    spec(
        "audit provider-closure",
        "provider_closure_audit",
        summary="Audit provider/compiler-generated dependency closure.",
        description=(
            "Read-only audit for provider/compiler-generated dependency closure. "
            "Use --owners-only for provider row metadata without Binary Ninja, or pass addresses "
            "to audit visible frontier dependencies through BN."
        ),
        category="audit",
        examples=(
            "python tools/recoil.py audit provider-closure --owners-only --strict",
            "python tools/recoil.py audit provider-closure 0x415650 --depth 1 --json --strict",
        ),
        needs_binja=True,
    ),
    spec("audit docblocks", "function_docblock_audit", summary="Audit reconstruction docblocks and source-comment hygiene.", category="audit", examples=("python tools/recoil.py audit docblocks --path src/path/file.cpp --summary --max 50", "python tools/recoil.py audit docblocks --path src --summary --max 50")),
    spec("audit workspace", "workspace_hygiene", summary="Detect generated artifacts outside approved output roots.", category="audit", examples=("python tools/recoil.py audit workspace --strict",)),
    spec("audit agent-surface", "agent_surface_audit", summary="Audit agent-facing tool, doc, skill, and role alignment.", category="audit", examples=("python tools/recoil.py audit agent-surface --strict",)),
    spec("audit workflow-contracts", "workflow_contract_audit", summary="Exercise compact reservation, handoff, mutation-boundary, and single-validator live acceptance contracts.", category="audit", examples=("python tools/recoil.py audit workflow-contracts --strict", "python tools/recoil.py audit workflow-contracts --json")),
    spec("audit call-contract-readiness", "call_contract_readiness_audit", summary="Preflight exact dependency closure for original authored call-contract slices.", category="audit", examples=("python tools/recoil.py audit call-contract-readiness --all-slices --strict --json", "python tools/recoil.py audit call-contract-readiness --slice recoil:call-contract-slice:0x401000-0x408210 --json")),
    spec("audit pipeline-reachability", "pipeline_reachability_audit", summary="Prove every fail-closed live pipeline consumer has a reachable candidate-independent expected-fact producer.", category="audit", examples=("python tools/recoil.py audit pipeline-reachability --strict", "python tools/recoil.py audit pipeline-reachability --json")),
    spec(
        "audit live-validation-surface",
        "live_validation_surface_audit",
        summary="Reject retired integrity, receipt, snapshot, clock, and content-derived identity mechanisms across tracked and ignored authored workspace surfaces.",
        category="audit",
        examples=("python tools/recoil.py audit live-validation-surface --strict",),
    ),
    spec("audit source-fragments", "source_fragments", prepend=("--audit",), summary="Inventory temporary production source fragments, quoted source includes, .inl files, and compatibility final-build exclusions.", category="audit", examples=("python tools/recoil.py audit source-fragments --root src --json",)),
    spec(
        "audit source-trace",
        "source_trace_audit",
        summary="Audit attached source-to-retail artifact topology without changing acceptance state.",
        category="audit",
        examples=(
            "python tools/recoil.py audit source-trace --path src/path/file.cpp --json",
            "python tools/recoil.py audit source-trace --path src --policy migrated --json",
        ),
    ),
    spec("audit zinterp", "zinterp_dispatch_audit", summary="Audit zInterp dispatch literals against optional BN text dumps.", category="audit", examples=("python tools/recoil.py audit zinterp",)),
    spec("guard source-shape", "no_source_shape_scaffolds", summary="Reject source-shape scaffolds in production source.", category="guard", examples=("python tools/recoil.py guard source-shape --root src --summary",)),
    spec("guard raw-offset", "raw_offset_guard", summary="Reject raw authored runtime-state offset access.", category="guard", examples=("python tools/recoil.py guard raw-offset --root src --summary",)),
    spec("guard raw-image", "no_raw_image_addresses", summary="Reject raw original-image addresses.", category="guard", examples=("python tools/recoil.py guard raw-image --root src --allowlist .agent/RAW_ADDRESS_ALLOWLIST.txt",)),
    spec("guard raw-assembly", "no_raw_assembly", summary="Reject unallowlisted or undocumented raw assembly and naked stubs.", category="guard", examples=("python tools/recoil.py guard raw-assembly --root src --allowlist .agent/RAW_ASSEMBLY_ALLOWLIST.txt",)),
    spec("guard source-goto", "no_source_goto", summary="Reject source-level goto outside the exact reviewed migration baseline.", category="guard", examples=("python tools/recoil.py guard source-goto --root src --summary", "python tools/recoil.py guard source-goto --root src --strict-zero")),
    spec("guard modern-cpp", "no_modern_cpp_constructs", summary="Reject post-VC5 C++ constructs and forbidden call-convention helpers.", category="guard", examples=("python tools/recoil.py guard modern-cpp --root src --summary",)),
    spec("guard reinterpret-cast", "no_reinterpret_cast", summary="Reject named reinterpret_cast usage.", category="guard", examples=("python tools/recoil.py guard reinterpret-cast --root src",)),
    spec("guard provider", "provider_boundary_guard", summary="Reject fake provider internals and provider ABI shims.", category="guard", examples=("python tools/recoil.py guard provider --root src --summary",)),
    spec("guard original-symbol", "original_source_symbol_guard", summary="Audit unsupported reconstruction helper dependencies.", category="guard", examples=("python tools/recoil.py guard original-symbol --root src --max 50",)),
    spec("guard source-data", "source_data_initializer_guard", summary="Validate source data initializer rules recorded in unified progress.", category="guard", examples=("python tools/recoil.py guard source-data", "python tools/recoil.py guard source-data --path src/Battlesport/CZRecoilFrame.cpp --summary"), ledger_routing="canonical-machine-local-default"),
    spec("guard multiline", "multiline_style_guard", summary="Check multiline declaration/call style.", category="guard", examples=("python tools/recoil.py guard multiline --root src",)),
    spec("guard source-placement", "source_placement_guard", summary="Check source placement and provenance conventions.", category="guard", examples=("python tools/recoil.py guard source-placement --root src",)),
    spec("guard vc5-manifest", "vc5_manifest_source_guard", summary="Reject VC manifest-local source and generated header shadows.", category="guard", examples=("python tools/recoil.py guard vc5-manifest", "python tools/recoil.py guard vc5-manifest --path tools/vc5_verify_targets/target.json")),
    spec("guard source-fragments", "source_fragments", summary="Reject temporary production source fragments, quoted source includes, .inl files, and compatibility final-build exclusions.", category="guard", examples=("python tools/recoil.py guard source-fragments --root src",)),
    spec("build msvc-x86", "msvc_x86_run", summary="Run an arbitrary command through the x86 MSVC environment wrapper.", category="build", examples=("python tools/recoil.py build msvc-x86 -- ctest --preset ninja-x86-debug",), mutates=True),
    spec(
        "build resource",
        "resource_extract",
        summary="Extract or compare source-style resources.",
        category="build",
        examples=(
            "python tools/recoil.py build resource --compare-res build/resource-reconstruct/Recoil.res",
            "python tools/recoil.py build resource --reference support/messages.dll --no-rc --messages-mc src/Messages/messages.mc --messages-lookup src/Messages/messages_lookup.inc",
        ),
        mutates=True,
    ),
    spec(
        "binja preflight",
        "binja_preflight",
        summary="Check Binary Ninja bridge/database availability.",
        category="binja",
        examples=(
            "python tools/recoil.py binja preflight --strict",
            "python tools/recoil.py binja preflight --binary messages --strict",
        ),
        needs_binja=True,
    ),
    spec(
        "binja data-overlap",
        "binja_data_overlap",
        summary="Report overlapping Binary Ninja data-variable roots.",
        description=(
            "Read-only diagnostic for stale or ambiguous interior Binary Ninja data variables, including cases where "
            "a typed root data variable coexists with same-range interior symbols after bridge deletion limitations. "
            "It reports overlaps and never edits BN state; unresolved findings block data-gate reliance on those facts."
        ),
        category="binja",
        examples=(
            "python tools/recoil.py binja data-overlap 0x4d21d8 0x4d22d4 0x4d22d8 --strict",
            "python tools/recoil.py binja data-overlap 0x4f52c8 0x4f53ac 0x4f53b6 0x4f53d0 --strict",
        ),
        needs_binja=True,
    ),
    spec("msvc eh-dump", "msvc_eh_dump", summary="Decode MSVC EH metadata from the reference image.", category="diagnostic", examples=("python tools/recoil.py msvc eh-dump 0x4d5e68 0x4d5f18",)),
    spec("style fix-multiline", "strict_multiline_style_fix", summary="Rewrite strict multiline style issues.", category="style", examples=("python tools/recoil.py style fix-multiline src/GameZRecoil/sample.cpp",), mutates=True),
)

_PROGRESS_TYPED_SPECS: tuple[CommandSpec, ...] = (
    spec("progress handoff", "progress_cli", prepend=("handoff",), summary="Render one compact worker packet and authenticated execution context from a real active reservation; fail closed when no matching reservation exists.", category="progress", examples=("python tools/recoil.py progress handoff --packet-id <packet-id> --json",), packet_binding="active-reservation-required", build_root_contract="return-authenticated-packet-root", ledger_routing="canonical-machine-local-default"),
    spec("progress current-metadata refresh", "current_metadata_mutation", prepend=("refresh",), summary="Revision-guard regeneration of live scheduler metadata and historicalize audited stale cursor narratives.", category="progress", examples=("python tools/recoil.py progress current-metadata refresh --expected-revision <revision> --dry-run --json",), mutates=True),
    spec("progress relocation-exception set", "relocation_expectation_mutation", prepend=("set",), summary="Revision-guard one reviewed retail-relocation ambiguity exception against exact current source and target context.", category="progress", examples=("python tools/recoil.py progress relocation-exception set --source-symbol-id <physical-symbol-id> --source-address 0xNNNNNN --payload-json '<json-object>' --expected-revision <revision> --dry-run --json",), mutates=True),
    spec("progress relocation-target bind", "relocation_target_mutation", prepend=("bind",), summary="Bind one immutable-retail relocation operand to reviewed existing or exact known-extent target identity.", category="progress", examples=("python tools/recoil.py progress relocation-target bind --source-symbol-id <physical-symbol-id> --source-address 0xNNNNNN --payload-json '<reviewed-binding>' --expected-revision <revision> --dry-run --json",), mutates=True),
    spec(
        "progress provider-target register",
        "provider_target_mutation",
        prepend=("register",),
        summary="Register one retail-proven named or reviewed ordinal-function IAT slot as an accepted typed provider target.",
        description="Register one retail-proven named or reviewed ordinal-function IAT slot by parsing immutable support/Recoil.exe and revision-guarding one reviewed provider package containing the exact IAT address, DLL/import identity (and exact import ordinal for #N imports), four-byte data/storage extent, one-byte callable provider-function extent, accepted provider owner, current evidence, and reviewed VC5 __imp_ object symbol. Dry-run first; relocation-target bind remains the separate call-site binding step.",
        category="progress",
        examples=("python tools/recoil.py progress provider-target register --address 0xNNNNNN --payload-json '<reviewed-provider-target>' --expected-revision <revision> --dry-run --json",),
        mutates=True,
    ),
    spec(
        "progress provider-function register",
        "provider_function_mutation",
        prepend=("register",),
        summary="Register one existing exact non-authored retail function as a canonical VC5 static-library provider function.",
        description="In archive-member mode, parse one exact member from one canonical VC5SP3 library under DEFAULT_VC5_ROOT. In canonical-header-comdat mode, compile only a registered fixed canonical-header recipe and require its exact semantic provider, decorated external symbol, code-COMDAT selection, explicit physical-emitter and retail-ICF winner state, and logical-symbol census. Both modes compare the exact known immutable-retail extent and relocation-masked body against the independent VC5 object proof, create only a provider-boundary owner/primary relation, and grant no authored tier, gate, or call-contract acceptance. Header mode additionally records provider-boundary source traceability as not applicable. Unknown recipes, including project-looking helpers, fail closed. Dry-run first; imports/IAT targets remain owned by provider-target register.",
        category="progress",
        examples=(
            "python tools/recoil.py progress provider-function register --address 0xNNNNNN --payload-json '<reviewed-archive-provider-function>' --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress provider-function register --address 0xNNNNNN --payload-json '<reviewed-registered-header-comdat-provider-function>' --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec(
        "progress provider-function register-atlimpl-cluster",
        "provider_function_mutation",
        prepend=("register-atlimpl-cluster",),
        summary=(
            "Atomically replace the exact reviewed legacy zCom owner with the fixed "
            "three-body canonical VC5SP3 ATLIMPL provider cluster."
        ),
        description=(
            "Dry-run-first one-time parent route for exactly 0x42db50, 0x42dc30, "
            "and 0x42dcf0 after the separate pipeline-classification batch and target "
            "synchronization. It guards the complete legacy owner and post-classification "
            "function snapshots, compiles only the fixed canonical VC5SP3 ATLIMPL recipe "
            "under /MD /G5 /O2 /Ob1 /GX /Zp4 /FAcs, directly compares immutable retail "
            "bytes outside the complete supported relocation rows, verifies exact retail "
            "relocation operands and natural extents, retires the legacy owner, and creates "
            "one accepted provider-boundary owner with exactly three primaries, no tiers, "
            "and no source paths. It preserves pipeline_class and authored_order_role, "
            "forbids 0x42de00, and records neither an original TU nor retail COFF spelling."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress provider-function register-atlimpl-cluster "
            "--payload-json '<recoil-vc5-atlimpl-provider-cluster-v1-object>' "
            "--expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec("progress owner show", "progress_cli", prepend=("owner", "show"), summary="Show one unified source owner or address-linked owner set.", category="progress"),
    spec("progress owner find", "progress_cli", prepend=("owner", "find"), summary="Search unified source owners.", category="progress"),
    spec("progress owner relationships", "progress_cli", prepend=("owner", "relationships"), summary="Show normalized unified owner relationships.", category="progress"),
    spec("progress owner audit", "progress_cli", prepend=("owner", "audit"), summary="Audit unified source-owner invariants.", category="progress"),
    spec(
        "progress owner repair-primary-data-tier-x",
        "progress_cli",
        prepend=("owner", "repair-primary-data-tier-x"),
        summary=(
            "Parent-only conservative initialization of absent tier-X records for "
            "exact existing same-owner authored primary data."
        ),
        category="progress",
        description=(
            "Dry-run-first parent repair route. One reviewed "
            "recoil-owner-primary-data-tier-x-repair-v1 payload guards an exact "
            "current non-provider owner and complete current primary-data relationship "
            "snapshots. Every selected symbol must already be authored data, primary-owned, "
            "and uniquely related to that same owner, with no reimplementation entry. "
            "Apply initializes only kind=data, tier=X, evidence_ids=[]; existing entries, "
            "function rows, changed memberships, ambiguity, and positive tier state fail closed."
        ),
        examples=(
            "python tools/recoil.py progress owner repair-primary-data-tier-x --payload-json '<recoil-owner-primary-data-tier-x-repair-v1-object>' --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec(
        "progress owner downgrade",
        "progress_cli",
        prepend=("owner", "downgrade"),
        summary="Parent-only atomic conservative downgrade of selected gates and primary-entry tiers on one exact current authored owner.",
        category="progress",
        description=(
            "Dry-run-first parent route. A reviewed recoil-owner-downgrade-v1 payload "
            "guards one exact current non-provider owner, every selected current gate "
            "state, and every selected current primary-entry tier. It permits only "
            "accepted/none gate states to become blocked, pending, or deferred and "
            "only strict S/A/B/C-to-lower entry-tier changes. Apply creates current "
            "owner-scoped evidence, uses revision CAS, preserves unrelated tracker "
            "facts, and synchronizes the generated README block."
        ),
        examples=(
            "python tools/recoil.py progress owner downgrade --payload-json '<recoil-owner-downgrade-v1-object>' --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec(
        "progress owner replace-batch",
        "progress_cli",
        prepend=("owner", "replace-batch"),
        summary="Parent-only atomic replacement of exact reviewed source-owner snapshots and guarded primary-function/data memberships.",
        category="progress",
        description=(
            "Dry-run-first parent route. Reviewed v1 payloads supply exact current owner records and "
            "complete replacement owner records. V2 additionally supplies exact reviewed currently-unowned "
            "function bootstraps, primary-data reassignments, unknown-extent data-symbol bootstraps, and "
            "optional exact primary-function detachments. A detachment applies only to an already reviewed "
            "non-authored/compiler-generated-icf-representative physical row, requires complete current and "
            "replacement snapshots of its retained owner, removes the corresponding physical relationship, "
            "tier, and address metadata exactly, and preserves the row's established ownership_state. "
            "V2 permits a data-only migration only when those reviewed rows produce a real primary-data "
            "membership change; v1 and batches with no changed primary membership remain fail-closed. "
            "Large exact payloads may use the mutually exclusive --payload-file route under workspace build/. "
            "It fails closed on stale snapshots or ownership states, duplicate or "
            "dangling owners, lost or extra primary memberships, unrelated gate/relationship changes, "
            "data/storage owner loss, invented unknown extents, and stale tier retention. An updated guarded owner may retarget an existing depends-on-owner "
            "row only from an owner retired by the batch to an explicit replacement owner; apply uses "
            "revision CAS and synchronizes the generated README block."
        ),
        examples=(
            "python tools/recoil.py progress owner replace-batch --payload-json '<recoil-owner-replace-batch-v2-object>' --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress owner replace-batch --payload-file build/diagnostic/<reviewed-owner-replace-v2.json> --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec("progress block show", "progress_cli", prepend=("block", "show"), summary="Show one physical source block.", category="progress"),
    spec(
        "progress block reclassify-provider",
        "progress_cli",
        prepend=("block", "reclassify-provider"),
        summary=(
            "Parent-only exact-snapshot reclassification of one stale authored physical "
            "block to a provider boundary."
        ),
        description=(
            "Dry-run-first parent route for one reviewed physical block whose contribution "
            "kind and provisional compile/source placement are stale after provider ownership "
            "acceptance. The v1 payload supplies the complete exact current block snapshot, "
            "canonical expected provider-owner ids, an explicit placement-clearing "
            "acknowledgement, and exact replacement provider labels. Every contribution is "
            "rederived from the tracker and must be non-authored/non-authored, primary-owned, "
            "and linked to exactly one accepted provider-boundary owner. Accepted original "
            "source provenance, stale interval or membership, configured order targets, active "
            "resource conflicts, and non-provider labels fail closed. Apply changes only "
            "contribution_kind, source_path, agent_source_path, provisional_original_path, "
            "mapping.status, and mapping.confidence. It preserves mapping evidence and state, "
            "semantic spans, every order/byte/symbol/owner/tier/storage/section fact, and the "
            "complete derived scheduler."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress block reclassify-provider --payload-json '<recoil-provider-block-reclassify-v1-object>' --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec(
        "progress block accept-authored-non-gating",
        "progress_cli",
        prepend=("block", "accept-authored-non-gating"),
        summary=(
            "Parent-only atomic acceptance of exact live-cursor physical blocks with zero "
            "authored gating identities."
        ),
        description=(
            "Dry-run-first parent route for one reviewed contiguous physical-block prefix at "
            "the live authored-order cursor. The v1 payload supplies complete exact current "
            "block snapshots and one exact expected authored cursor-after. Every contribution "
            "membership is rederived live; every member must have a resolved, compatible, "
            "non-gating pipeline role, including a classified compiler-generated lifecycle "
            "role. Nonzero blocks must be explicit high-confidence "
            "provider-boundaries, while zero-row blocks must be high-confidence provider-data "
            "or padding. Configured order targets, active resource conflicts, stale snapshots, "
            "unresolved or authored gating rows, and cursor gaps fail closed. Apply records one "
            "live evidence row and changes only the five authored-order block dimensions to "
            "current accepted not-applicable; full order, symbols, bytes, owners, source/mapping "
            "provenance, tiers, spans, storage, and every other state remain unchanged. Use "
            "--payload-file for large exact memberships that exceed the Windows command-line limit."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress block accept-authored-non-gating --payload-json '<recoil-authored-non-gating-block-accept-v1-object>' --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress block accept-authored-non-gating --payload-file <reviewed-v1.json> --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec(
        "progress block replace",
        "progress_cli",
        prepend=("block", "replace"),
        summary="Parent-only atomic replacement of one reviewed physical block and all affected symbol/semantic assignments.",
        description=(
            "Dry-run-first parent route for one exact unresolved physical block. The reviewed v1 JSON "
            "payload staleness-guards the current range, source/provenance fields, contribution inventory, "
            "and semantic inventory; it then enumerates contiguous replacement blocks, every selected "
            "symbol assignment, and complete replacement semantic spans. Existing semantic observations "
            "and exact zero-symbol padding spans are preserved while a span may split only at a replacement "
            "block seam. The command fails closed on gaps, merges, drops, status/path drift, invented empty "
            "spans, overlaps, collisions, dangling current relationships, incomplete assignments, accepted "
            "original-source provenance, active work references, revision drift, or scheduler breakage. "
            "Authoritative apply uses the normal tracker CAS and README synchronization path. Use "
            "--payload-file only for a UTF-8 reviewed object under workspace build/ that exceeds the "
            "Windows command-line limit."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress block replace --payload-json '<reviewed-v1-object>' --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress block replace --payload-file <build/reviewed-v1.json> --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec("progress semantic show", "progress_cli", prepend=("semantic", "show"), summary="Show one semantic span.", category="progress"),
    spec(
        "progress symbol set-pipeline-class-batch",
        "progress_cli",
        prepend=("symbol", "set-pipeline-class-batch"),
        summary="Revision-guard a reviewed batch of exact function-row pipeline classifications.",
        description=(
            "Dry-run-first parent route for exact existing Recoil function-symbol ids and addresses. "
            "Every JSON batch item must carry reviewed=true, the current pipeline_class and "
            "authored_order_role used for staleness detection, and one compatible replacement pair. "
            "Apply changes only those two classification fields; successful authoritative apply "
            "increments the tracker revision and synchronizes the generated README progress block."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress symbol set-pipeline-class-batch --payload-json '<json-array>' --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec(
        "progress symbol set-logical-alias-group",
        "progress_cli",
        prepend=("symbol", "set-logical-alias-group"),
        summary=(
            "Parent-only revision-guard one reviewed physical ICF row and its authored logical aliases."
        ),
        description=(
            "Parent-only dry-run-first route for one exact existing Recoil physical function row. The "
            "reviewed v1-v3 contracts retain the established non-authored/compiler-generated-ICF "
            "representative model. The reviewed v1 "
            "payload guards the complete current alias-related state, selects exactly one authored "
            "logical winner plus one or more proven fold aliases, or records a winner-unknown group whose "
            "winner_identity_key is null and whose members are all proven fold aliases. Logical members use "
            "one exact ordinary authored/body or authored-lifecycle/lifecycle-body classification pair, with "
            "exact decorated VC5 symbols and accepted non-provider owners. V1 preserves its existing "
            "tracker-evidence contract. V2 instead requires a candidate-independent new_evidence review "
            "summary, provenance, durable artifacts, and validation context with candidate_output_used=false; "
            "it derives the exact physical-symbol/logical-alias/owner scope, predicts one generated evidence "
            "id during dry-run, then atomically creates and assigns that same id to the group and every alias "
            "on apply. Caller-supplied evidence ids or scopes and candidate outputs/artifacts fail closed. The "
            "v3 is an existing winner-unknown group evidence refresh. V4 is the distinct authored-linker-"
            "coalesced model: it preserves one authored physical order/byte/call gate and address-exclusive "
            "physical primary owner while recording two or more non-gating authored logical members with "
            "exclusive owners and resolved defining source edges whose attached production-source mirrors "
            "are checked live. Recovered or provisional source names are descriptive only; exact decorated "
            "object symbols remain candidate-mechanism identities. Immutable-retail call-site and vtable "
            "selectors remain expected truth. Fresh governed VC5 object reports plus paired "
            "vc5sp3_ref_noicf split and vc5sp3_ref_icf fold MAP transcripts are mandatory corroborating "
            "source/link-mechanism evidence, including distinct eligible COMDAT definitions, explicit body "
            "and inbound selector relocation partitions, and a base-implementation control with a concrete "
            "object/report-backed fold-relevant difference; candidate output never "
            "supplies retail identity or address truth. The command changes only the reviewed alias state, "
            "its generated evidence row/id sequence, and required "
            "dependent order/byte invalidations. It fails closed on stale state, unsupported geometry, "
            "unknown evidence, revision drift, or an invalid physical-block relationship; successful "
            "apply invalidates dependent order/byte facts and synchronizes the generated README block. "
            "Use exactly one of inline --payload-json or a UTF-8 --payload-file that resolves under "
            "workspace build/; both inputs use the same schema, semantic, revision-CAS, and apply path."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress symbol set-logical-alias-group --payload-json '<recoil-logical-alias-group-v1-object>' --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress symbol set-logical-alias-group --payload-json '<recoil-logical-alias-group-v2-object>' --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress symbol set-logical-alias-group --payload-json '<recoil-logical-alias-group-v4-object>' --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress symbol set-logical-alias-group --payload-file build/diagnostic/<recoil-logical-alias-group-v4.json> --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec(
        "progress symbol replace-padding",
        "progress_cli",
        prepend=("symbol", "replace-padding"),
        summary=(
            "Parent-only removal of one exact false function identity after immutable-retail "
            "NOP-padding verification."
        ),
        description=(
            "Dry-run-first parent route for one reviewed recoil-function-padding-correction-v1 "
            "payload. The command exact-guards the complete current function and semantic-span "
            "rows plus the retained physical block's scalar/count/membership state; reads bytes "
            "only from the registered immutable support/Recoil.exe; and refuses non-NOP bytes, "
            "accepted order/byte state, revision drift, or owner, target, storage, work, and "
            "other unsafe relationships. Apply deletes only the false symbol, its one physical-"
            "block membership, its one padding-span membership, and any matching optional "
            "schema-v4 migration references. It retains the block and zero-symbol padding span, preserves "
            "the inventory snapshot and all scheduler/order/byte/owner/source/storage/output/"
            "target facts, and uses the authoritative tracker CAS/README synchronization path."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress symbol replace-padding --payload-json '<recoil-function-padding-correction-v1-object>' --expected-revision <revision> --dry-run --json",
        ),
        mutates=True,
    ),
    spec(
        "progress work show",
        "progress_cli",
        prepend=("work", "show"),
        summary="Show one exact structured work item.",
        category="progress",
        examples=(
            "python tools/recoil.py progress work show <work-item-id> --json",
        ),
    ),
    spec("progress work claim-current", "progress_cli", prepend=("work", "claim-current"), summary="Atomically create and reserve compatible current packets through prioritized multi-lane or focused individual-lane claims.", category="progress", examples=("python tools/recoil.py progress work claim-current --lane all --max-packets <N> --expected-scheduler-revision <scheduler-revision> --apply --json", "python tools/recoil.py progress work claim-current --lane primary --expected-scheduler-revision <scheduler-revision> --apply --json"), mutates=True, required_revision_domains=("scheduler",), packet_binding="allocator", build_root_contract="allocate-authenticated-external-root", ledger_routing="canonical-machine-local-default", mutation_scope="scheduler"),
    spec("progress work create-explicit", "progress_cli", prepend=("work", "create-explicit"), summary="Parent-only journal-first output-root allocation followed by one final atomic activation of an exact explicitly user-selected maintenance or read-only diagnostic packet.", category="progress", examples=("python tools/recoil.py progress work create-explicit --payload-file build/diagnostics/<packet>.json --expected-scheduler-revision <revision> --expected-semantic-revision <revision> --dry-run --json", "python tools/recoil.py progress work create-explicit --payload-file build/diagnostics/<packet>.json --expected-scheduler-revision <revision> --expected-semantic-revision <revision> --apply --json"), mutates=True),
    spec("progress work reserve", "progress_cli", prepend=("work", "reserve"), summary="Reserve one scheduler-launchable or exact retry-eligible returned packet with non-expiring normalized resource claims.", category="progress", mutates=True),
    spec("progress work return", "progress_cli", prepend=("work", "return"), summary="Return one active explicit maintenance packet with bounded nonaccepting feedback.", category="progress", mutates=True),
    spec("progress work return-binja", "progress_cli", prepend=("work", "return-binja"), summary="Parent-only governed Binary Ninja read-plan execution and scheduler-CAS return for one active BN-enabled explicit packet.", category="progress", mutates=True, needs_binja=True),
    spec("progress work recover-expired", "progress_cli", prepend=("work", "recover-expired"), summary="Release one expired explicit maintenance reservation back to ready state without acceptance.", category="progress", mutates=True),
    spec("progress work recover-allocation", "progress_cli", prepend=("work", "recover-allocation"), summary="Authenticate and recover one journal-owned failed explicit output allocation without acceptance.", category="progress", examples=("python tools/recoil.py progress work recover-allocation --id <packet-id> --expected-scheduler-revision <revision> --expected-semantic-revision <revision> --dry-run --json",), mutates=True),
    spec("progress work leases", "progress_cli", prepend=("work", "leases"), summary="Show global reconstruction and workspace-issue leases, or conflicts for one packet.", category="progress"),
    spec("progress work close", "progress_cli", prepend=("work", "close"), summary="Close one structured work item.", category="progress", mutates=True),
    spec(
        "progress call-contract initialize",
        "progress_cli",
        prepend=("call-contract", "initialize"),
        summary="Parent-only one-time initialization of the accepted-authored-order-derived call-contract census while preserving all order and byte facts.",
        description=(
            "Revision-atomically initialize pending call_contract state from the complete "
            "accepted authored-order gating census. The initial reviewed migration census "
            "was 3,380 bodies; that historical count is not a permanent live invariant, "
            "and each invocation derives and validates the current census."
        ),
        category="progress",
        examples=("python tools/recoil.py progress call-contract initialize --expected-revision <revision> --dry-run --json",),
        mutates=True,
    ),
    spec(
        "progress call-contract prepare-live-convergence",
        "progress_cli",
        prepend=("call-contract", "prepare-live-convergence"),
        summary=(
            "Contained parent-only fresh no-reuse zero-divergence closeout; requires an "
            "active packet and is the only call-contract route that may authorize phase transition."
        ),
        description=(
            "Authenticate the canonical issue ledger, active explicit packet, reservation claims, "
            "and physical output root, then stop at the containment gate. No compiler, Binary "
            "Ninja expected-fact producer, convergence generation, or tracker mutation is reachable "
            "until a separately reviewed direct-comparison producer is accepted."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress call-contract prepare-live-convergence --packet-id <packet-id> --closeout --build-root <packet-root> --jobs 2 --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json",
        ),
        mutates=True,
        needs_binja=True,
    ),
    spec(
        "progress call-contract prepare-repair-continuation",
        "progress_cli",
        prepend=("call-contract", "prepare-repair-continuation"),
        summary=(
            "Parent-only fresh producer-result routing into one fail-closed repair descriptor; "
            "the later child is created only by claim-current."
        ),
        description=(
            "Authenticate the active branchless continuation producer and retained predecessor, "
            "run the producer's exact exhaustive verifier command, and store only a fresh "
            "verifier-derived route descriptor. No operator caller, owner, or path facts are "
            "accepted; this command creates no child and accepts no reconstruction evidence."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress call-contract prepare-repair-continuation --producer-packet <producer-packet-id> --returned-work-item <returned-work-id> --build-root <producer-root> --expected-revision <revision> --apply --json",
        ),
        mutates=True,
        needs_binja=True,
    ),
    spec("progress output-section show", "progress_cli", prepend=("output-section", "show"), summary="Show one normalized PE output section.", category="progress"),
    spec("progress storage show", "progress_cli", prepend=("storage", "show"), summary="Show one normalized physical storage contribution.", category="progress"),
    spec(
        "progress storage register-authored-data",
        "progress_cli",
        prepend=("storage", "register-authored-data"),
        summary=(
            "Parent-only revision-guarded registration of one exact "
            "non-overlapping authored data-symbol storage contribution."
        ),
        description=(
            "Dry-run-first parent route for one existing known-extent authored "
            "physical data artifact with exactly one existing primary-data "
            "owner. The reviewed v1 payload exact-guards canonical symbol, "
            "storage, and owner ids plus binary, output section, extent, empty "
            "current storage linkage, and the complete primary-data relationship. "
            "It rejects provider owners, stale or ambiguous ownership, section "
            "escape, physical overlap, and any existing storage identity. Apply "
            "only creates the pending data-symbol storage row and atomically "
            "appends its id to the symbol; it creates no source edge and changes "
            "no owner/gate/tier/order/byte/provider/link/final-image acceptance."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress storage register-authored-data --payload-file <reviewed-v1.json> --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress storage register-authored-data --payload-file <reviewed-v1.json> --expected-revision <revision> --apply --json",
        ),
        mutates=True,
    ),
    spec("progress verification-target sync", "progress_cli", prepend=("verification-target", "sync"), summary="Synchronize selected verification-target registrations, with a fail-closed parent-only source-policy bootstrap for one reviewed order target.", category="progress", examples=("python tools/recoil.py progress verification-target sync --target ainet_text_block_order --expected-revision 45 --dry-run", "python tools/recoil.py progress verification-target sync --target camera_449ba0_44d990_authored_order --source-policy-bootstrap --expected-revision 910 --dry-run --json"), mutates=True),
    spec(
        "progress verification-target retire",
        "progress_cli",
        prepend=("verification-target", "retire"),
        summary="Dry-run-first parent route that retires exactly one stale verification-target registration by exact id or unique name.",
        description=(
            "Fail-closed parent-only retirement of exactly one existing verification-target "
            "registration. Exact tracker target ids take precedence over names; a name must "
            "resolve uniquely. The route detaches that exact id from every symbol relationship, "
            "invalidates dependent symbol byte/call-contract/link facts and physical-block order "
            "facts through the canonical helper, preserves block order_targets so unresolved "
            "routing remains visible, and commits through tracker CAS/README synchronization."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress verification-target retire --target <exact-target-id> --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress verification-target retire --target <exact-target-id> --expected-revision <revision> --apply --json",
        ),
        mutates=True,
    ),
    spec(
        "progress advance-live-order",
        "progress_cli",
        prepend=("advance-live-order",),
        summary="Freshly validate one registered order target, derive its complete contiguous block slices, and revision-atomically accept all slices only on exact PASS; full order keeps linked acceptance separate from its paired object worker target.",
        category="progress",
        examples=("python tools/recoil.py progress advance-live-order --target <linked-target-id> --object-target <object-target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json",),
        mutates=True,
        required_revision_domains=("global",),
        build_root_contract="fresh-parent-root",
        ledger_routing="canonical-machine-local-default",
        mutation_scope="order",
    ),
    spec(
        "progress advance-live-byte",
        "progress_cli",
        prepend=("advance-live-byte",),
        summary="Freshly compile and directly compare one byte lane, accepting only explicitly matched tracker physical groups.",
        category="progress",
        examples=("python tools/recoil.py progress advance-live-byte --lane authored --build-root <fresh-root> --expected-revision <revision> --apply --json",),
        mutates=True,
        required_revision_domains=("global",),
        build_root_contract="fresh-parent-root",
        ledger_routing="canonical-machine-local-default",
        mutation_scope="byte",
    ),
    spec(
        "progress advance-live-call-contract",
        "progress_cli",
        prepend=("advance-live-call-contract",),
        summary=(
            "Contained parent route that performs one fresh build and direct retail comparison, "
            "then CAS-accepts only bodies that passed in that invocation."
        ),
        description=(
            "Authenticate the active call-contract-acceptance packet, reservation claims, physical "
            "output root, and separate semantic/evidence revision guards before building. The same "
            "invocation compares exact structured call facts and may advance only directly passing "
            "bodies; divergent bodies remain pending and no stored result substitutes for the build."
        ),
        category="progress",
        examples=("python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --packet-id <packet-id> --build-root <packet-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json",),
        mutates=True,
        needs_binja=True,
        required_revision_domains=("semantic", "evidence_generation"),
        packet_binding="active-call-contract-reservation-required",
        build_root_contract="packet-authenticated-external-root",
        ledger_routing="canonical-machine-local-default",
        mutation_scope="call-contract",
    ),
    spec(
        "progress source-trace replace-batch",
        "source_trace_progress",
        prepend=("replace-batch",),
        summary="Parent-only revision-guarded replacement of reviewed source-trace topology rows and append-only resolution of immutable legacy claims.",
        category="progress",
        examples=(
            "python tools/recoil.py progress source-trace replace-batch --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json",
            "python tools/recoil.py progress source-trace replace-batch --expected-revision <revision> --payload-file <reviewed.json> --apply --json",
        ),
        mutates=True,
    ),
    spec(
        "progress source-trace show",
        "source_trace_progress",
        prepend=("show",),
        summary="Show read-only physical/logical source-trace topology rows.",
        category="progress",
        examples=(
            "python tools/recoil.py progress source-trace show --artifact-id recoil:function:0x401000 --json",
            "python tools/recoil.py progress source-trace show --address 0x401000 --json",
        ),
    ),
    spec(
        "progress data-extent register",
        "data_extent_progress",
        summary=(
            "Parent-only revision-guarded exact extent registration for one "
            "existing physical data artifact; creates no artifact, source "
            "edge, or acceptance."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress data-extent register --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json",
            "python tools/recoil.py progress data-extent register --expected-revision <revision> --payload-file <reviewed.json> --apply --json",
        ),
        mutates=True,
    ),
    spec(
        "progress data-artifact register",
        "data_artifact_progress",
        summary=(
            "Parent-only revision-guarded registration of one exact physical "
            "data identity and extent; creates no source edge, owner link, "
            "storage contribution, or acceptance."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress data-artifact register --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json",
            "python tools/recoil.py progress data-artifact register --expected-revision <revision> --payload-file <reviewed.json> --apply --json",
        ),
        mutates=True,
    ),
    spec(
        "progress data-artifact evidence repair-observation",
        "data_artifact_evidence_repair",
        summary=(
            "Parent-only revision-guarded repair of the known invalid "
            "freshness/validation-mode pair on one reviewed non-gating "
            "data-artifact observation."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress data-artifact evidence repair-observation --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json",
            "python tools/recoil.py progress data-artifact evidence repair-observation --expected-revision <revision> --payload-file <reviewed.json> --apply --json",
        ),
        mutates=True,
    ),
    spec(
        "progress data-artifact logical-alias register-batch",
        "data_logical_alias_progress",
        summary=(
            "Parent-only revision-guarded registration of reviewed authored "
            "logical-data occurrences under one provider/compiler-pooled "
            "physical representative."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress data-artifact logical-alias register-batch --expected-revision <revision> --payload-file <reviewed.json> --dry-run --json",
            "python tools/recoil.py progress data-artifact logical-alias register-batch --expected-revision <revision> --payload-file <reviewed.json> --apply --json",
        ),
        mutates=True,
    ),
)

COMMAND_SPECS: tuple[CommandSpec, ...] = (*_BASE_COMMAND_SPECS, *_PROGRESS_TYPED_SPECS)

COMMANDS = {item.path: item for item in COMMAND_SPECS}

COMMAND_ALIASES = {
    ("audit", "function-docblocks"): ("audit", "docblocks"),
    ("audit", "modern-cpp"): ("guard", "modern-cpp"),
    ("audit", "raw-assembly"): ("guard", "raw-assembly"),
    ("audit", "source-placement"): ("guard", "source-placement"),
    ("audit", "source-shape"): ("guard", "source-shape"),
    ("guard", "source-data-initializer"): ("guard", "source-data"),
}

OBSOLETE_RAW_ASSEMBLY_ALLOWLIST = "tools/raw_assembly_allowlist.json"
CANONICAL_RAW_ASSEMBLY_ALLOWLIST = ".agent/RAW_ASSEMBLY_ALLOWLIST.txt"


def command_names() -> list[str]:
    return sorted(item.name for item in COMMAND_SPECS)


def group_children(prefix: tuple[str, ...]) -> list[CommandSpec]:
    return sorted(
        (item for item in COMMAND_SPECS if item.path[: len(prefix)] == prefix and len(item.path) > len(prefix)),
        key=lambda item: item.path,
    )


def command_to_json(item: CommandSpec) -> dict[str, object]:
    return {
        "command": item.name,
        "category": item.category,
        "summary": item.summary,
        "mutates": item.mutates,
        "needs_binja": item.needs_binja,
        "examples": list(item.examples),
        "required_revision_domains": list(item.required_revision_domains),
        "packet_binding": item.packet_binding,
        "build_root_contract": item.build_root_contract,
        "ledger_routing": item.ledger_routing,
        "mutation_scope": item.mutation_scope,
    }


def print_main_help() -> None:
    print("# Recoil Unified Tool Gate")
    print()
    print("Usage:")
    print("  python tools/recoil.py help")
    print("  python tools/recoil.py help <command-or-group>")
    print("  python tools/recoil.py commands --json")
    print("  python tools/recoil.py <command> [tool args...]")
    print("  python tools/recoil.py <command> --show-command [tool args...]")
    print()
    print("Use this gate for all agent-facing Recoil commands. Backend modules are internal.")
    print()
    print("Available commands:")
    current_category = ""
    for item in sorted(COMMAND_SPECS, key=lambda spec_item: (spec_item.category, spec_item.path)):
        if item.category != current_category:
            current_category = item.category
            print()
            print(f"{current_category}:")
        flags = []
        if item.mutates:
            flags.append("mutates")
        if item.needs_binja:
            flags.append("may need BN")
        flag_text = f" [{' ; '.join(flags)}]" if flags else ""
        print(f"  {item.name:<24} {item.summary}{flag_text}")
    print()
    print("Examples:")
    print("  python tools/recoil.py progress next")
    print("  python tools/recoil.py progress show 0x401000")
    print("  python tools/recoil.py progress owner show recoil:owner:source.owner_id")
    print("  python tools/recoil.py progress audit --strict")


def print_command_help(item: CommandSpec) -> None:
    print(f"# recoil {item.name}")
    print()
    print(item.description)
    print()
    print(f"Category: {item.category}")
    print(f"Mutates files/state: {'yes' if item.mutates else 'no'}")
    print(f"May need Binary Ninja: {'yes' if item.needs_binja else 'no'}")
    print(
        "Required revision domains: "
        + (", ".join(item.required_revision_domains) or "none")
    )
    print(f"Packet binding: {item.packet_binding}")
    print(f"Build-root contract: {item.build_root_contract}")
    print(f"Ledger routing: {item.ledger_routing}")
    print(f"Mutation scope: {item.mutation_scope}")
    if item.examples:
        print()
        print("Examples:")
        for example in item.examples:
            print(f"  {example}")
    print()
    print("Pass additional arguments after the command. Use --show-command to inspect the internal invocation.")
    print(f"Underlying parser help: python tools/recoil.py {item.name} -- --help")


def print_group_help(prefix: tuple[str, ...]) -> bool:
    children = group_children(prefix)
    if not children:
        return False
    print(f"# recoil {' '.join(prefix)}")
    print()
    print("Available subcommands:")
    for item in children:
        remainder = " ".join(item.path[len(prefix) :])
        print(f"  {remainder:<18} {item.summary}")
    print()
    print(f"Use: python tools/recoil.py help {' '.join(prefix)} <subcommand>")
    return True


def resolve_command(args: list[str]) -> tuple[CommandSpec | None, list[str]]:
    max_len = min(len(args), max(len(path) for path in COMMANDS))
    for size in range(max_len, 0, -1):
        prefix = tuple(args[:size])
        alias = COMMAND_ALIASES.get(prefix)
        if alias is not None:
            item = COMMANDS.get(alias)
            if item is not None:
                return item, args[size:]
        item = COMMANDS.get(prefix)
        if item is not None:
            return item, args[size:]
    return None, args


_EXPLICIT_PACKET_PUBLIC_GROUPS = frozenset({"verify", "audit", "guard", "doctor"})
_EXPLICIT_PACKET_BUILD_ONLY_ROUTES = frozenset({("verify", "final-build")})


def _explicit_packet_claim_keys(
    resource_claims: Iterable[Mapping[str, Any] | str],
) -> frozenset[tuple[str, str, str]]:
    """Normalize only the resource identity needed by public-route validation."""

    result: set[tuple[str, str, str]] = set()
    for claim in resource_claims:
        if isinstance(claim, Mapping):
            kind = str(claim.get("kind", "")).strip()
            identity = str(claim.get("id", "")).strip()
            access = str(claim.get("access", "")).strip()
        elif isinstance(claim, str):
            parts = claim.split(":", 2)
            if len(parts) != 3:
                raise ValueError(f"malformed explicit-packet resource claim {claim!r}")
            kind, identity, access = (part.strip() for part in parts)
        else:
            raise ValueError("explicit-packet resource claims must be mappings or strings")
        if not kind or not identity or access not in {"read", "write"}:
            raise ValueError(f"malformed explicit-packet resource claim {claim!r}")
        result.add((kind, identity, access))
    return frozenset(result)


def validate_nonmutating_public_command(
    public_args: Sequence[str],
    *,
    resource_claims: Iterable[Mapping[str, Any] | str] = (),
) -> dict[str, object]:
    """Authenticate one packet worker command against the actual public registry.

    This function imports and invokes only the selected backend's argparse parser;
    it never dispatches the command.  The explicit packet constructor calls it
    before reserving resources or allocating an output root, so an unknown route,
    malformed argument vector, authority mutation, or missing BN/whole-link claim
    fails before any compiler, linker, Binary Ninja, or filesystem work can start.
    """

    args = [str(part) for part in public_args]
    if not args or args[0] not in _EXPLICIT_PACKET_PUBLIC_GROUPS:
        raise ValueError(
            "explicit maintenance validation must use a registered verify, audit, guard, or doctor route"
        )
    item, rest = resolve_command(args)
    if item is None:
        raise ValueError(f"unknown public validation command: {' '.join(args)}")
    if item.path[0] not in _EXPLICIT_PACKET_PUBLIC_GROUPS:
        raise ValueError(f"public route {item.name!r} is not a packet validation command")
    build_only = item.path in _EXPLICIT_PACKET_BUILD_ONLY_ROUTES
    if item.mutates and not build_only:
        raise ValueError(f"public route {item.name!r} mutates authoritative workspace state")
    if any(argument in {"--apply", "--delete"} for argument in rest):
        raise ValueError(f"public route {item.name!r} includes a mutating argument")

    try:
        module = importlib.import_module(item.module_name)
    except Exception as exc:
        raise ValueError(
            f"public route {item.name!r} backend cannot be imported: {exc}"
        ) from exc
    parser_factory = getattr(module, "build_parser", None)
    if not callable(parser_factory):
        parser_factory = getattr(module, "_parser", None)
    if not callable(parser_factory):
        raise ValueError(f"public route {item.name!r} has no reviewable argument parser")
    try:
        with contextlib.redirect_stderr(io.StringIO()):
            parser_factory().parse_args([*item.prepend_args, *rest])
    except SystemExit as exc:
        raise ValueError(f"public route {item.name!r} arguments do not parse") from exc
    except Exception as exc:
        raise ValueError(
            f"public route {item.name!r} parser could not validate its arguments: {exc}"
        ) from exc

    claims = _explicit_packet_claim_keys(resource_claims)
    required_claims: set[tuple[str, str, str]] = set()
    if item.needs_binja:
        required_claims.add(("binary-ninja-db", "Recoil.bndb", "read"))
    if item.path == ("verify", "final-build"):
        required_claims.add(("whole-project-build", "recoil", "write"))
    missing = sorted(required_claims - set(claims))
    if missing:
        formatted = ", ".join(":".join(row) for row in missing)
        raise ValueError(
            f"public route {item.name!r} lacks required packet resources: {formatted}"
        )
    return {
        "path": list(item.path),
        "command": item.name,
        "module": item.module,
        "prepend_args": list(item.prepend_args),
        "needs_binja": item.needs_binja,
        "build_artifacts_only": build_only,
        "required_resource_claims": [
            {"kind": kind, "id": identity, "access": access}
            for kind, identity, access in sorted(required_claims)
        ],
    }


def format_command(command: list[str]) -> str:
    return " ".join(f'"{part}"' if any(char.isspace() for char in part) else part for part in command)


def build_command(item: CommandSpec, rest: list[str]) -> list[str]:
    return [
        sys.executable,
        "-m",
        item.module_name,
        *item.prepend_args,
        *rest,
    ]


def translate_compatibility_args(
    invoked_args: list[str],
    item: CommandSpec,
    rest: list[str],
) -> list[str]:
    if (
        tuple(invoked_args[:2]) == ("audit", "function-docblocks")
        and item.path == ("audit", "docblocks")
    ):
        return ["--path" if arg == "--root" else arg for arg in rest]

    if (
        tuple(invoked_args[:2]) != ("audit", "raw-assembly")
        or item.path != ("guard", "raw-assembly")
    ):
        return rest

    translated = list(rest)
    for index in range(len(translated) - 1):
        if (
            translated[index] == "--allowlist"
            and translated[index + 1] == OBSOLETE_RAW_ASSEMBLY_ALLOWLIST
        ):
            translated[index + 1] = CANONICAL_RAW_ASSEMBLY_ALLOWLIST
    return translated


def internal_command_env() -> dict[str, str]:
    env = os.environ.copy()
    existing = env.get("PYTHONPATH", "")
    parts = [str(TOOLS_DIR)]
    if existing:
        parts.append(existing)
    env["PYTHONPATH"] = os.pathsep.join(parts)
    return env


def progress_file_signature(path: Path = DEFAULT_PROGRESS_PATH) -> int | None:
    """Return the semantic tracker revision, independent of SQLite file metadata."""
    try:
        return read_progress_metadata(path).revision
    except ProgressSQLiteError:
        # The invoked backend remains the authority for its normal fail-closed
        # missing/corrupt-store error.  This observer must not mask that output.
        return None


def rejected_json_ledger_argument(args: list[str], item: CommandSpec) -> str | None:
    """Reject legacy JSON authorities on ordinary runtime command routes."""
    if item.path[:1] == ("maintenance",):
        return None
    ledger_options = {"--progress", "--ledger", "--issue-ledger"}
    for index, argument in enumerate(args[:-1]):
        if argument in ledger_options and Path(args[index + 1]).suffix.casefold() == ".json":
            return argument
    for argument in args:
        option, separator, value = argument.partition("=")
        if separator and option in ledger_options and Path(value).suffix.casefold() == ".json":
            return option
    return None


def sync_readme_after_progress_mutation() -> None:
    """Synchronize silently so a backend's stdout, including JSON, stays intact."""
    from _recoil.commands.readme_progress import synchronize_readme

    synchronize_readme()


def print_unknown_command(args: list[str]) -> int:
    entered = " ".join(args) if args else "<empty>"
    print(f"recoil: unknown command: {entered}", file=sys.stderr)
    if args:
        matches = difflib.get_close_matches(entered, command_names(), n=5, cutoff=0.35)
        if matches:
            print("Did you mean:", file=sys.stderr)
            for match in matches:
                print(f"  python tools/recoil.py {match}", file=sys.stderr)
    print("Run 'python tools/recoil.py help' for available commands.", file=sys.stderr)
    return 2


def command_list_json() -> int:
    print(json.dumps([command_to_json(item) for item in sorted(COMMAND_SPECS, key=lambda spec_item: spec_item.path)], indent=2))
    return 0


def main(argv: list[str] | None = None) -> int:
    configure_stdio()
    args = list(sys.argv[1:] if argv is None else argv)
    if not args or args[0] in {"-h", "--help", "help"}:
        if len(args) <= 1 or args[0] in {"-h", "--help"}:
            print_main_help()
            return 0
        target = args[1:]
        item, rest = resolve_command(target)
        if item is not None and not rest:
            print_command_help(item)
            return 0
        if print_group_help(tuple(target)):
            return 0
        return print_unknown_command(target)

    if args[0] == "commands":
        if args[1:] == ["--json"]:
            return command_list_json()
        print("recoil: commands only supports --json", file=sys.stderr)
        print("Usage: python tools/recoil.py commands --json", file=sys.stderr)
        return 2

    item, rest = resolve_command(args)
    if item is None:
        if args[-1:] and args[-1] in {"-h", "--help"} and print_group_help(tuple(args[:-1])):
            return 0
        for size in range(len(args), 0, -1):
            prefix = tuple(args[:size])
            if print_group_help(prefix):
                print(f"recoil: incomplete command group: {' '.join(prefix)}", file=sys.stderr)
                return 2
        return print_unknown_command(args)

    if rest and rest[0] in {"-h", "--help"}:
        print_command_help(item)
        return 0

    forward_rest = rest
    if forward_rest[:2] in (["--", "-h"], ["--", "--help"]):
        forward_rest = forward_rest[1:]
    show_command = False
    if "--show-command" in forward_rest:
        show_command = True
        forward_rest = [arg for arg in forward_rest if arg != "--show-command"]
    forward_rest = translate_compatibility_args(args, item, forward_rest)
    rejected_option = rejected_json_ledger_argument(forward_rest, item)
    if rejected_option is not None:
        print(
            f"recoil: {rejected_option} no longer accepts a JSON ledger; run "
            "'python tools/recoil.py maintenance migrate-ledgers-sqlite' first",
            file=sys.stderr,
        )
        return 2

    module_path = TOOLS_DIR / "_recoil" / "commands" / f"{item.module}.py"
    if not module_path.exists():
        print(f"recoil: internal command module does not exist: {module_path}", file=sys.stderr)
        print(f"Command: {item.name}", file=sys.stderr)
        return 2

    command = build_command(item, forward_rest)
    if show_command:
        print(format_command(command))
        return 0

    watches_progress = item.mutates and item.path[:1] == ("progress",)
    progress_before = progress_file_signature() if watches_progress else None
    completed = subprocess.run(command, cwd=str(REPO_ROOT), env=internal_command_env())
    progress_changed = (
        watches_progress and progress_before != progress_file_signature()
    )
    readme_sync_failure: Exception | None = None
    if progress_changed:
        try:
            sync_readme_after_progress_mutation()
        except Exception as exc:  # tracker commit already happened; never roll it back
            readme_sync_failure = exc
    if completed.returncode:
        print(
            f"recoil: command failed with exit code {completed.returncode}: {item.name}",
            file=sys.stderr,
        )
        print(f"mapped_command={format_command(command)}", file=sys.stderr)
        print(f"help=python tools/recoil.py help {item.name}", file=sys.stderr)
    if readme_sync_failure is not None:
        print(
            "recoil: tracker changed but README progress synchronization failed: "
            f"{readme_sync_failure}",
            file=sys.stderr,
        )
        print(
            "remediation=python tools/recoil.py docs readme-progress",
            file=sys.stderr,
        )
        return 3
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
