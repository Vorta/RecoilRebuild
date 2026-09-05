#!/usr/bin/env python3
"""Unified agent-facing gate for Recoil reconstruction tools."""

from __future__ import annotations

from dataclasses import dataclass
import difflib
import json
import os
from pathlib import Path
import subprocess
import sys

sys.dont_write_bytecode = True

from _recoil.lib.tooling import REPO_ROOT, configure_stdio
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
        build_root_contract=build_root_contract,
        ledger_routing=ledger_routing,
        mutation_scope=mutation_scope,
    )


_BASE_COMMAND_SPECS: tuple[CommandSpec, ...] = (
    spec(
        "progress next",
        "progress_cli",
        prepend=("next",),
        summary=(
            "Select the sole authoritative next Recoil.exe reconstruction task, "
            "including its direct acceptance and optional stage_runner_command."
        ),
        description=(
            "The current-task contract retains exactly one direct acceptance "
            "command. During authored-call-contract its stage_runner_command also "
            "publishes replay-live as the normal serial whole-stage runner; the "
            "slice check and direct acceptance remain available for "
            "first-divergence diagnosis."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress next",
            "python tools/recoil.py progress next --json",
        ),
    ),
    spec("progress show", "progress_cli", prepend=("show",), summary="Show a joined owner/block/semantic/order/link/byte view.", category="progress", examples=("python tools/recoil.py progress show 0x401000", "python tools/recoil.py progress show recoil:owner:misc_unresolved.cabout_dlg")),
    spec("progress find", "progress_cli", prepend=("find",), summary="Search all unified reconstruction progress entities.", category="progress", examples=("python tools/recoil.py progress find CAboutDlg",)),
    spec("progress audit", "progress_cli", prepend=("audit",), summary="Audit unified tracker schema, relationships, evidence, and derived pipeline invariants.", category="progress", examples=("python tools/recoil.py progress audit --strict", "python tools/recoil.py progress audit --scope evidence --strict")),
    spec("verify linked-byte", "live_byte_verify", prepend=("linked",), summary="Freshly rebuild and directly scan linked bytes, stopping at the earliest real divergence.", category="verification", examples=("python tools/recoil.py verify linked-byte", "python tools/recoil.py verify linked-byte --at 0x401000")),
    spec("verify authored-byte", "live_byte_verify", prepend=("authored",), summary="Freshly rebuild and directly scan authored object, relocation, target, and linked-body semantics.", category="verification", examples=("python tools/recoil.py verify authored-byte", "python tools/recoil.py verify authored-byte --at 0x401000")),
      spec(
          "verify call-contract",
          "call_contract_verify",
          summary="Freshly compile one deterministic authored-body slice, or one nonaccepting registered-target convergence scope, and compare exact static invocation contracts with retail Binary Ninja evidence.",
          category="verification",
          examples=(
              "python tools/recoil.py verify call-contract --slice <slice-id> --build-root <fresh-root> --json --summary",
              "python tools/recoil.py verify call-contract --target <target-id> --all-authored-bodies --build-root <fresh-root> --json --summary",
          ),
        needs_binja=True,
    ),
    spec("verify final-image", "live_final_verify", summary="Freshly build and validate complete typed PE semantics against retail; raw file differences and COFF time are diagnostic only.", category="verification", examples=("python tools/recoil.py verify final-image", "python tools/recoil.py verify final-image --candidate build/vc5-final/Recoil.exe --map build/vc5-final/Recoil.map --json"), mutates=True),
    spec("issue report", "workspace_issues", prepend=("report",), summary="Record an agent tooling/process problem for a future work session.", category="issue", examples=("python tools/recoil.py issue report --kind tool-error --severity high --summary \"...\" --area tools/recoil.py --impact \"...\" --actual \"...\" --repro \"...\" --next-action \"...\"",), mutates=True),
    spec("issue request", "workspace_issues", prepend=("request",), summary="Record an agent tooling/process improvement request.", category="issue", examples=("python tools/recoil.py issue request --severity medium --summary \"...\" --area tools/recoil.py --impact \"...\" --requested-change \"...\" --benefit \"...\" --next-action \"...\"",), mutates=True),
    spec("issue list", "workspace_issues", prepend=("list",), summary="List open agent tooling/process issue reports.", category="issue", examples=("python tools/recoil.py issue list --status open",)),
    spec("issue show", "workspace_issues", prepend=("show",), summary="Show one agent tooling/process issue report.", category="issue", examples=("python tools/recoil.py issue show WSI-YYYYMMDD-NNN",)),
    spec("issue resolve", "workspace_issues", prepend=("resolve",), summary="Mark an agent tooling/process issue resolved.", category="issue", examples=("python tools/recoil.py issue resolve WSI-YYYYMMDD-NNN --resolution \"...\"",), mutates=True),
    spec("issue wont-fix", "workspace_issues", prepend=("wont-fix",), summary="Close an agent tooling/process issue without resolution and remove its terminal active-only rows.", category="issue", mutates=True),
    spec("issue reopen", "workspace_issues", prepend=("reopen",), summary="Reopen an agent tooling/process issue.", category="issue", examples=("python tools/recoil.py issue reopen WSI-YYYYMMDD-NNN --reason \"...\"",), mutates=True),
    spec("issue audit", "workspace_issues", prepend=("audit",), summary="Validate the agent tooling/process issue ledger shape.", category="issue", examples=("python tools/recoil.py issue audit --strict",)),
    spec(
        "doctor",
        "doctor",
        summary="Run the sequential fail-fast workspace health matrix.",
        category="validation",
        examples=(
            "python tools/recoil.py doctor",
        ),
    ),
    spec(
        "verify vc5",
        "vc5_verify",
        summary="Run a VC5SP3 toolchain smoke or owner-scoped COFF function/data verification.",
        description="List or run VC5SP3 COFF function/data-symbol verification. Tier S owner byte-gate evidence should use --owner so every linked source-owner row is covered before compilation; owner rows with duplicate diagnostic data-symbol coverage prefer the row's owner target metadata when it uniquely identifies a manifest. Explicit target/address selectors remain available for diagnostics and manifest development. Multiple explicit target/address selectors are grouped by identical compiles; multi-data-symbol BN comparisons are internally auto-chunked under the bridge call budget; live BN hexdumps route through the manifest target_binary or --binary override.",
        category="verification",
        examples=(
            "python tools/recoil.py verify vc5 --smoke --build-root build/live-validation/vc5-smoke/<fresh-root>",
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
            "A full-order validation pairs the compiling object target with --linked-target so the same "
            "non-mutating loop reports exact linked selected-population and seam divergence."
        ),
        category="verification",
        examples=(
            "python tools/recoil.py verify vc5-order target_name --build-root build/vc5-order/target_name",
            "python tools/recoil.py verify vc5-order object_target --linked-target linked_target --build-root build/vc5-order/full-target",
        ),
        needs_binja=False,
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
            "python tools/recoil.py verify final-build --playground-only --build-dir build/live-validation/playground/<fresh-root>",
            "python tools/recoil.py verify final-build --build-dir build/live-validation/linkability/<fresh-root> --clean --order-scope authored --linkability-only",
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
    spec("audit source-policy", "source_policy", summary="Run the complete production-source policy gate once.", category="audit", examples=("python tools/recoil.py audit source-policy",)),
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
    spec("audit agent-surface", "agent_surface_audit", summary="Audit the serial agent-facing tool, documentation, and skill surface.", category="audit", examples=("python tools/recoil.py audit agent-surface --strict",)),
    spec("audit pipeline-contracts", "pipeline_contract_audit", summary="Audit the one serial task projection and its direct validation/acceptance command contracts.", category="audit", examples=("python tools/recoil.py audit pipeline-contracts --strict", "python tools/recoil.py audit pipeline-contracts --json")),
    spec("audit pipeline-reachability", "pipeline_reachability_audit", summary="Prove every fail-closed live pipeline consumer has a reachable candidate-independent expected-fact producer.", category="audit", examples=("python tools/recoil.py audit pipeline-reachability --strict", "python tools/recoil.py audit pipeline-reachability --json")),
    spec(
        "audit live-validation-surface",
        "live_validation_surface_audit",
        summary="Reject retired integrity, receipt, snapshot, clock, and content-derived identity mechanisms across authored workspace surfaces.",
        category="audit",
        examples=("python tools/recoil.py audit live-validation-surface --strict",),
    ),
    spec(
        "audit source-trace",
        "source_trace_audit",
        summary="Audit attached source-to-retail artifact topology without changing acceptance state.",
        category="audit",
        examples=(
            "python tools/recoil.py audit source-trace --json",
        ),
    ),
    spec("guard source-shape", "no_source_shape_scaffolds", summary="Reject source-shape scaffolds in production source.", category="guard", examples=("python tools/recoil.py guard source-shape --root src --summary",)),
    spec("guard raw-offset", "raw_offset_guard", summary="Reject raw authored runtime-state offset access.", category="guard", examples=("python tools/recoil.py guard raw-offset --root src --summary",)),
    spec("guard raw-image", "no_raw_image_addresses", summary="Reject raw original-image addresses.", category="guard", examples=("python tools/recoil.py guard raw-image --root src --allowlist .agent/RAW_ADDRESS_ALLOWLIST.txt",)),
    spec("guard raw-assembly", "no_raw_assembly", summary="Reject unallowlisted or undocumented raw assembly and naked stubs.", category="guard", examples=("python tools/recoil.py guard raw-assembly --root src --allowlist .agent/RAW_ASSEMBLY_ALLOWLIST.txt",)),
    spec("guard source-goto", "no_source_goto", summary="Reject source-level goto in production source.", category="guard", examples=("python tools/recoil.py guard source-goto --root src --summary",)),
    spec("guard modern-cpp", "no_modern_cpp_constructs", summary="Reject post-VC5 C++ constructs and forbidden call-convention helpers.", category="guard", examples=("python tools/recoil.py guard modern-cpp --root src --summary",)),
    spec("guard provider", "provider_boundary_guard", summary="Reject fake provider internals and provider ABI shims.", category="guard", examples=("python tools/recoil.py guard provider --root src --summary",)),
    spec("guard original-symbol", "original_source_symbol_guard", summary="Audit unsupported reconstruction helper dependencies.", category="guard", examples=("python tools/recoil.py guard original-symbol --root src --max 50",)),
    spec("guard source-data", "source_data_initializer_guard", summary="Validate source data initializer rules recorded in unified progress.", category="guard", examples=("python tools/recoil.py guard source-data", "python tools/recoil.py guard source-data --path src/Battlesport/CZRecoilFrame.cpp"), ledger_routing="canonical-machine-local-default"),
    spec("guard source-placement", "source_placement_guard", summary="Check source placement and provenance conventions.", category="guard", examples=("python tools/recoil.py guard source-placement --root src",)),
    spec("guard vc5-manifest", "vc5_manifest_source_guard", summary="Reject VC manifest-local source and generated header shadows.", category="guard", examples=("python tools/recoil.py guard vc5-manifest", "python tools/recoil.py guard vc5-manifest --path tools/vc5_verify_targets/target.json")),
    spec("guard source-fragments", "source_fragments", summary="Reject temporary production source fragments, quoted source includes, and .inl files.", category="guard", examples=("python tools/recoil.py guard source-fragments --root src",)),
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
    spec("msvc eh-dump", "msvc_eh_dump", summary="Decode MSVC EH metadata from the reference image.", category="diagnostic", examples=("python tools/recoil.py msvc eh-dump 0x4d5e68 0x4d5f18",)),
)

_PROGRESS_TYPED_SPECS: tuple[CommandSpec, ...] = (
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
    spec("progress owner show", "progress_cli", prepend=("owner", "show"), summary="Show one unified source owner or address-linked owner set.", category="progress"),
    spec("progress owner audit", "progress_cli", prepend=("owner", "audit"), summary="Audit unified source-owner invariants.", category="progress"),
    spec(
        "progress owner downgrade",
        "progress_cli",
        prepend=("owner", "downgrade"),
        summary="Atomically and conservatively downgrade selected gates and primary-entry tiers on one exact current authored owner.",
        category="progress",
        description=(
            "Dry-run-first reviewed route. A recoil-owner-downgrade-v1 payload "
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
        summary="Atomically replace exact reviewed source-owner snapshots and guarded primary-function/data memberships.",
        category="progress",
        description=(
            "Dry-run-first reviewed route. V1 payloads supply exact current owner records and "
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
            "Direct exact-snapshot reclassification of one stale authored physical "
            "block to a provider boundary."
        ),
        description=(
            "Dry-run-first reviewed route for one physical block whose contribution "
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
            "Direct atomic acceptance of exact live-cursor physical blocks with zero "
            "authored gating identities."
        ),
        description=(
            "Dry-run-first reviewed route for one contiguous physical-block prefix at "
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
        summary="Atomically replace one reviewed physical block and all affected symbol/semantic assignments.",
        description=(
            "Dry-run-first reviewed route for one exact unresolved physical block. The v1 JSON "
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
    spec("audit coff-lifecycle", "coff_lifecycle", summary="Inspect complete COFF lifecycle definitions and inbound relocations without accepting reconstruction facts.", category="audit"),
    spec("progress semantic show", "progress_cli", prepend=("semantic", "show"), summary="Show one semantic span.", category="progress"),
    spec(
        "progress symbol set-pipeline-class-batch",
        "progress_cli",
        prepend=("symbol", "set-pipeline-class-batch"),
        summary="Revision-guard a reviewed batch of exact function-row pipeline classifications.",
        description=(
            "Dry-run-first reviewed route for exact existing Recoil function-symbol ids and addresses. "
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
            "Revision-guard one reviewed physical ICF row and its authored logical aliases."
        ),
        description=(
            "Dry-run-first reviewed route for one exact existing Recoil physical function row. The "
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
            "supplies retail identity or address truth. V5 additionally represents shared-header inline "
            "or implicit destructors with explicit emitting TUs and a manifest-selected logical object "
            "witness. The fresh order comparison checks every same-TU member's complete COMDAT bytes "
            "and relocation semantics; the original folded winner remains a separate fact. "
            "The command changes only the reviewed alias state, "
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
            "Direct removal of one exact false function identity after immutable-retail "
            "NOP-padding verification."
        ),
        description=(
            "Dry-run-first reviewed route for one recoil-function-padding-correction-v1 "
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
        "progress call-contract close-live",
        "progress_cli",
        prepend=("call-contract", "close-live"),
        summary=(
            "Run fresh isolated slice scans with bounded concurrency, then one "
            "no-deploy whole-program linkability gate and one direct closeout."
        ),
        description=(
            "The default eight subprocess slots may be adjusted with --max-workers "
            "from 1 through 21. Results are validated and retained in retail slice "
            "order; only the read-only slice scans overlap. Linking and the final "
            "semantic/evidence CAS mutation remain serial."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress call-contract close-live --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --max-workers 8 --apply --json",
        ),
        mutates=True,
        needs_binja=True,
        required_revision_domains=("semantic", "evidence_generation"),
        build_root_contract="fresh-direct-root",
        mutation_scope="call-contract",
    ),
    spec(
        "progress call-contract replay-live",
        "call_contract_replay",
        summary=(
            "Prove the complete authored call-contract census once and "
            "serially commit its original-slice projections."
        ),
        description=(
            "Load one atomic current task, reserve a fresh replay sibling without "
            "consuming its direct root, share unique target/definition builds, source "
            "discovery, COD indexing, and target-qualified Binary Ninja facts across "
            "one full-census proof, then commit exact original-slice projections with "
            "the existing per-body evidence and semantic/evidence CAS. Stop after the "
            "first divergent slice and return close-live without running it. Dry-run "
            "plans the whole census with no build, Binary Ninja read, or mutation."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress call-contract replay-live --dry-run --json",
            "python tools/recoil.py progress call-contract replay-live --apply --json",
        ),
        mutates=True,
        needs_binja=True,
        required_revision_domains=("semantic", "evidence_generation"),
        build_root_contract="fresh-replay-sibling",
        ledger_routing="canonical-machine-local-default",
        mutation_scope="call-contract",
    ),
    spec("progress output-section show", "progress_cli", prepend=("output-section", "show"), summary="Show one normalized PE output section.", category="progress"),
    spec("progress storage show", "progress_cli", prepend=("storage", "show"), summary="Show one normalized physical storage contribution.", category="progress"),
    spec(
        "progress storage register-authored-data",
        "progress_cli",
        prepend=("storage", "register-authored-data"),
        summary=(
            "Direct revision-guarded registration of one exact "
            "non-overlapping authored data-symbol storage contribution."
        ),
        description=(
            "Dry-run-first reviewed route for one existing known-extent authored "
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
    spec("progress verification-target sync", "progress_cli", prepend=("verification-target", "sync"), summary="Synchronize selected verification-target registrations, with a fail-closed source-policy bootstrap for one reviewed order target.", category="progress", examples=("python tools/recoil.py progress verification-target sync --target ainet_text_block_order --expected-revision 45 --dry-run", "python tools/recoil.py progress verification-target sync --target camera_449ba0_44d990_authored_order --source-policy-bootstrap --expected-revision 910 --dry-run --json"), mutates=True),
    spec(
        "progress verification-target retire",
        "progress_cli",
        prepend=("verification-target", "retire"),
        summary="Dry-run-first reviewed route that retires exactly one stale verification-target registration by exact id or unique name.",
        description=(
            "Fail-closed retirement of exactly one existing verification-target "
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
        summary="Freshly validate one registered order target, derive its complete contiguous block slices, and revision-atomically accept all slices only on exact PASS.",
        category="progress",
        examples=("python tools/recoil.py progress advance-live-order --target <linked-target-id> --object-target <object-target-id> --build-root <fresh-root> --expected-revision <revision> --apply --json",),
        mutates=True,
        required_revision_domains=("global",),
        build_root_contract="fresh-direct-root",
        ledger_routing="canonical-machine-local-default",
        mutation_scope="order",
    ),
    spec("progress advance-live-authored-byte", "progress_cli", prepend=("advance-live-authored-byte",), summary="Freshly compile and directly accept the current authored byte group.", category="progress", examples=("python tools/recoil.py progress advance-live-authored-byte --build-root <fresh-root> --expected-revision <revision> --apply --json",), mutates=True, required_revision_domains=("global",), build_root_contract="fresh-direct-root", mutation_scope="byte"),
    spec("progress advance-live-linked-byte", "progress_cli", prepend=("advance-live-linked-byte",), summary="Freshly build and directly accept the current linked byte group.", category="progress", examples=("python tools/recoil.py progress advance-live-linked-byte --build-root <fresh-root> --expected-revision <revision> --apply --json",), mutates=True, required_revision_domains=("global",), build_root_contract="fresh-direct-root", mutation_scope="byte"),
    spec(
        "progress advance-live-call-contract",
        "progress_cli",
        prepend=("advance-live-call-contract",),
        summary=(
            "Direct route that performs one fresh build and direct retail comparison, "
            "then CAS-accepts only bodies that passed in that invocation."
        ),
        description=(
            "Authenticate a fresh build root and separate semantic/evidence revision guards before building. The same "
            "invocation compares exact structured call facts and may advance only directly passing "
            "bodies; divergent bodies remain pending and no stored result substitutes for the build."
        ),
        category="progress",
        examples=("python tools/recoil.py progress advance-live-call-contract --slice <slice-id> --build-root <fresh-root> --expected-semantic-revision <semantic-revision> --expected-evidence-generation-revision <evidence-revision> --apply --json",),
        mutates=True,
        needs_binja=True,
        required_revision_domains=("semantic", "evidence_generation"),
        build_root_contract="fresh-direct-root",
        ledger_routing="canonical-machine-local-default",
        mutation_scope="call-contract",
    ),
    spec(
        "progress source-trace replace-batch",
        "source_trace_progress",
        prepend=("replace-batch",),
        summary="Revision-guarded replacement of reviewed source-trace topology rows.",
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
        "progress source-path relocate",
        "progress_cli",
        prepend=("source-path", "relocate"),
        summary=(
            "Revision-guarded relocation of one reviewed current production "
            "source-path prefix after verification-target synchronization."
        ),
        description=(
            "The v1 payload exact-guards every matching physical block, semantic "
            "span, owner, source-trace artifact, and pre-synchronized verification "
            "target. The route authenticates the new repository files, requires the "
            "old checkout prefix to be absent, rewrites only current implementation "
            "paths, preserves historical provenance and topology, and conservatively "
            "invalidates dependent order, call-contract, and byte state."
        ),
        category="progress",
        examples=(
            "python tools/recoil.py progress source-path relocate --payload-json '<recoil-source-path-relocation-v1>' --expected-revision <revision> --dry-run --json",
            "python tools/recoil.py progress source-path relocate --payload-file build/reviewed-source-path-relocation.json --expected-revision <revision> --apply --json",
        ),
        mutates=True,
        required_revision_domains=("global",),
        mutation_scope="source-path",
    ),
    spec(
        "progress data-extent register",
        "data_extent_progress",
        summary=(
            "Direct revision-guarded exact extent registration for one "
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
            "Direct revision-guarded registration of one exact physical "
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
        "progress data-artifact logical-alias register-batch",
        "data_logical_alias_progress",
        summary=(
            "Direct revision-guarded registration of reviewed authored "
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
        item = COMMANDS.get(prefix)
        if item is not None:
            return item, args[size:]
    return None, args






def format_command(command: list[str]) -> str:
    return " ".join(f'"{part}"' if any(char.isspace() for char in part) else part for part in command)


def build_command(item: CommandSpec, rest: list[str]) -> list[str]:
    return [
        sys.executable,
        "-B",
        "-m",
        item.module_name,
        *item.prepend_args,
        *rest,
    ]


def internal_command_env() -> dict[str, str]:
    env = os.environ.copy()
    env["PYTHONDONTWRITEBYTECODE"] = "1"
    existing = env.get("PYTHONPATH", "")
    parts = [str(TOOLS_DIR)]
    if existing:
        parts.append(existing)
    env["PYTHONPATH"] = os.pathsep.join(parts)
    return env


SOURCE_POLICY_APPLY_COMMANDS = frozenset(
    {
        ("progress", "advance-live-order"),
        ("progress", "advance-live-call-contract"),
        ("progress", "advance-live-authored-byte"),
        ("progress", "advance-live-linked-byte"),
        ("progress", "call-contract", "replay-live"),
        ("progress", "call-contract", "close-live"),
    }
)


def requires_source_policy(item: CommandSpec, forwarded: list[str]) -> bool:
    if item.path == ("verify", "final-image"):
        return True
    return item.path in SOURCE_POLICY_APPLY_COMMANDS and "--apply" in forwarded


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
    rejected_option = rejected_json_ledger_argument(forward_rest, item)
    if rejected_option is not None:
        print(
            f"recoil: {rejected_option} no longer accepts a JSON ledger; "
            "this workspace uses SQLite authorities only",
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

    if requires_source_policy(item, forward_rest):
        policy_command = [
            sys.executable,
            "-B",
            "-m",
            "_recoil.commands.source_policy",
        ]
        policy = subprocess.run(
            policy_command,
            cwd=str(REPO_ROOT),
            env=internal_command_env(),
        )
        if policy.returncode:
            print(
                "recoil: source-policy gate failed before " + item.name,
                file=sys.stderr,
            )
            return policy.returncode

    completed = subprocess.run(command, cwd=str(REPO_ROOT), env=internal_command_env())
    if completed.returncode:
        print(
            f"recoil: command failed with exit code {completed.returncode}: {item.name}",
            file=sys.stderr,
        )
        print(f"mapped_command={format_command(command)}", file=sys.stderr)
        print(f"help=python tools/recoil.py help {item.name}", file=sys.stderr)
    return completed.returncode


if __name__ == "__main__":
    raise SystemExit(main())
