"""Recoil call-contract records evidence and checks."""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Any, Mapping

from _recoil.commands.asm_verify import CoffRelocation, Instruction


@dataclass(frozen=True)
class ReviewedEhArrayDestructorCallSpec:
    label: str
    caller_identity: str
    caller_start: str
    caller_end_exclusive: str
    caller_symbol: str
    ordinal: int
    invocation_count: int


@dataclass(frozen=True)
class ReviewedRegisterIatBridgeSpec:
    label: str
    caller_identity: str
    caller_start: str
    caller_end_exclusive: str
    caller_symbol: str
    register: str
    import_dll: str
    import_name: str
    owner_id: str
    retail_storage_address: str
    retail_storage_type: str
    retail_load_address: str
    retail_load_body: bytes
    retail_call_address: str
    retail_call_body: bytes
    retail_backedge_address: str
    retail_backedge_mnemonic: str
    retail_backedge_body: bytes
    candidate_import_symbol: str
    candidate_load_address: str
    candidate_load_body: bytes
    candidate_call_address: str
    candidate_call_body: bytes
    candidate_backedge_address: str
    candidate_backedge_mnemonic: str
    candidate_backedge_body: bytes
    candidate_relocation_offset: int
    ordinal: int
    cleanup_bytes: int | None
    invocation_count: int


@dataclass(frozen=True)
class ReviewedCallResultBridge:
    register: str
    provenance: str
    target_identity: str


@dataclass(frozen=True)
class ReviewedVptrStorageBridge:
    register: str
    provenance: str
    storage_identity: str
    slot_displacement: int
    identity_kind: str = "callback"


@dataclass(frozen=True)
class ReviewedLoopVptrStorageBridge:
    register: str
    storage_identity: str
    slot_displacement: int
    assembly_source: str
    # Most object/vtable dispatches remain runtime-selected.  Only a producer
    # that reads the exact immutable retail table cell may set this target.
    target_identity: str = ""


@dataclass(frozen=True)
class ReviewedExactIndirectStorageBridge:
    """One structurally reviewed indirect call whose target remains dynamic."""

    register: str
    storage_identity: str
    slot_displacement: int
    assembly_source: str
    identity_kind: str = "virtual-slot"


@dataclass(frozen=True)
class ReviewedRegisterCallStorageBridge:
    """One exact retail ``CALL r32`` backed by reviewed storage.

    Register-state storage bridges are definition keyed.  This separate bridge
    is deliberately call-site keyed so a retail-only absolute-load proof cannot
    be advertised under a register name that the extractor never consumes.
    """

    register: str
    storage_identity: str
    identity_kind: str
    assembly_source: str = "bn"
    form: str = "call"


@dataclass(frozen=True)
class ReviewedRetailProvenanceAdapters:
    """One caller-scoped finite retail-adapter result for every live mode."""

    indexes: IdentityIndexes
    register_call_storage_bridges: Mapping[
        str, ReviewedRegisterCallStorageBridge
    ]
    vptr_storage_bridges: Mapping[str, ReviewedLoopVptrStorageBridge]
    reviewed_call_sites: tuple[str, ...]
    exact_indirect_storage_bridges: Mapping[
        str, ReviewedExactIndirectStorageBridge
    ] = field(default_factory=dict)
    non_callback_register_loads: Mapping[str, str] = field(
        default_factory=dict
    )
    preempt_generic_register_iat: bool = False


@dataclass(frozen=True)
class ReviewedInboundEntryRegisterTargetBridge:
    """Candidate-independent target truth for one entry-register call.

    The target is selected only from one complete BN inbound-xref population
    and unanimous exact immediate definitions at every direct caller.  Retail
    and candidate bodies merely consume the already-selected target by proving
    the same entry-register lineage at each exact invocation ordinal.
    """

    ordinal: int
    retail_call_address: str
    call_register: str
    entry_register: str
    target_identity: str
    lineage_cleanup_by_ordinal: tuple[tuple[int, int], ...] = ()


@dataclass(frozen=True)
class ReviewedMemberVptrStorageBridge:
    register: str
    source_register: str
    source_provenance: str
    receiver_register: str
    receiver_provenance: str
    storage_identity: str
    slot_displacement: int
    call_address: str


@dataclass(frozen=True)
class _ReviewedMemberVptrBridgeIndexes:
    """Keep reviewed member-vptr load and call addresses distinct."""

    by_load_address: Mapping[str, ReviewedMemberVptrStorageBridge]
    by_call_address: Mapping[str, ReviewedMemberVptrStorageBridge]


@dataclass(frozen=True)
class ReviewedDynamicExportCallSpec:
    label: str
    caller_identity: str
    caller_start: str
    caller_end_exclusive: str
    caller_symbol: str
    getproc_iat_address: str
    getproc_import_dll: str
    getproc_import_name: str
    getproc_import_symbol: str
    export_data_id: str
    export_address: str
    export_navigation_name: str
    export_value: str
    candidate_export_symbol_pattern: str
    provider_owner_id: str
    provider_function_id: str
    provider_address: str
    provider_end_exclusive: str
    provider_navigation_name: str
    provider_physical_block_id: str
    register: str
    retail_getproc_call_address: str
    retail_getproc_call_body: bytes
    retail_test_address: str
    retail_test_body: bytes
    retail_branch_address: str
    retail_branch_body: bytes
    retail_dynamic_call_address: str
    retail_dynamic_call_body: bytes
    candidate_getproc_call_address: str
    candidate_getproc_call_body: bytes
    candidate_test_address: str
    candidate_test_body: bytes
    candidate_branch_address: str
    candidate_branch_body: bytes
    candidate_dynamic_call_address: str
    candidate_dynamic_call_body: bytes
    candidate_export_relocation_offset: int
    candidate_getproc_relocation_offset: int
    ordinal: int
    cleanup_bytes: int | None
    invocation_count: int


@dataclass(frozen=True)
class ReviewedDirectIatStorageCallSpec:
    label: str
    caller_identity: str
    caller_start: str
    caller_end_exclusive: str
    caller_symbol: str
    import_dll: str
    import_name: str
    retail_storage_address: str
    retail_call_address: str
    retail_storage_type: str
    candidate_import_symbol: str
    candidate_call_address: str
    candidate_relocation_offset: int
    owner_id: str
    ordinal: int
    cleanup_bytes: int
    invocation_count: int


@dataclass(frozen=True)
class ReviewedLogicalAlias:
    identity: str
    original_name: str
    object_symbol: str


@dataclass(frozen=True)
class StorageContainer:
    start: int
    end_exclusive: int
    identity: str


@dataclass(frozen=True)
class RegisteredDecoratedDataNameSupplier:
    name: str
    address: str
    storage_identity: str
    target_id: str


@dataclass(frozen=True)
class ReviewedStaticStorageReferenceBridge:
    aggregate_symbol: str
    displacement: int
    access_width: int
    storage_identity: str


@dataclass(frozen=True)
class ReviewedAbsoluteStorageLoadBridge:
    register: str
    aggregate_symbol: str
    displacement: int
    access_width: int
    storage_identity: str
    canonicalize_nested_load: bool = False


@dataclass(frozen=True)
class RegisteredLifecycleDeletingDestructorSupplier:
    candidate_name: str
    complete_name: str
    address: str
    identity: str
    target_id: str


@dataclass(frozen=True)
class RegisteredScalarDeletingDestructorSupplier:
    class_identity: str
    address: str
    identity: str
    evidence_sources: tuple[str, ...]


@dataclass(frozen=True)
class RegisteredVectorAssignmentProviderSupplier:
    candidate_name: str
    address: str
    identity: str
    target_id: str


@dataclass(frozen=True)
class RegisteredPointerVectorDestroyProviderSupplier:
    address: str
    identity: str
    target_id: str
    catalog_symbol: str
    stack_cleanup_bytes: int


@dataclass(frozen=True)
class StructuralPhysicalProviderSupplier:
    candidate_name: str
    address: str
    identity: str
    stack_cleanup_bytes: int


@dataclass(frozen=True)
class CurrentIatStoragePackage:
    address: str
    import_dll: str
    import_name: str
    import_ordinal: int | None
    object_symbol: str
    identity: str


@dataclass(frozen=True)
class ProviderOrdinalImportThunk:
    provider_identity: str
    thunk_address: str
    retail_name: str
    callable_symbol: str
    iat_object_symbol: str
    iat_identity: str
    import_dll: str
    import_ordinal: int


@dataclass(frozen=True)
class ProviderNamedImportThunk:
    provider_identity: str
    thunk_address: str
    retail_name: str
    callable_symbol: str
    iat_object_symbol: str
    iat_identity: str
    iat_address: str
    import_dll: str
    import_name: str


@dataclass(frozen=True)
class ProviderPeNamedImportThunk:
    """Expected-side named FF25 thunk proven from retail PE and block facts."""

    provider_identity: str
    thunk_address: str
    retail_name: str
    iat_identity: str
    iat_address: str
    import_dll: str
    import_name: str


@dataclass(frozen=True)
class IdentityIndexes:
    by_address: Mapping[str, str]
    by_candidate_name: Mapping[str, str]
    provider_ids: frozenset[str]
    storage_by_address: Mapping[str, str]
    storage_by_name: Mapping[str, str]
    reviewed_iat_register_joins: frozenset[tuple[str, str]] = frozenset()
    reviewed_retail_iat_load_proofs: Mapping[
        str, "CandidateExactIatRegisterLoadProof"
    ] = field(default_factory=dict)
    active_retail_caller_proof_package: (
        "RetailCallerScopedProofPackage | None"
    ) = None
    reviewed_static_callback_target_by_storage: Mapping[str, str] = field(
        default_factory=dict
    )
    candidate_only_names: frozenset[str] = frozenset()
    call_only_icf_by_site: Mapping[str, Mapping[str, Any]] = field(default_factory=dict)
    reviewed_icf_group_by_address: Mapping[str, str] = field(default_factory=dict)
    reviewed_icf_group_by_logical_identity: Mapping[str, str] = field(
        default_factory=dict
    )
    reviewed_logical_aliases_by_address: Mapping[
        str, tuple[ReviewedLogicalAlias, ...]
    ] = field(default_factory=dict)
    reviewed_authored_icf_by_call_site: Mapping[
        str, tuple[str, str]
    ] = field(default_factory=dict)
    reviewed_authored_icf_candidate_by_call_site: Mapping[
        str, tuple[str, str, str, str]
    ] = field(default_factory=dict)
    reviewed_authored_icf_provisional_candidate_by_name: Mapping[
        str, tuple[str, str]
    ] = field(default_factory=dict)
    reviewed_direct_import_thunk_by_call_site: Mapping[
        str, tuple[str, str]
    ] = field(default_factory=dict)
    reviewed_exact_local_import_thunk_by_call_site: Mapping[
        str, tuple[str, str]
    ] = field(default_factory=dict)
    reviewed_authored_icf_physical_by_logical_identity: Mapping[str, str] = field(
        default_factory=dict
    )
    reviewed_authored_icf_by_vtable_selector: Mapping[
        tuple[str, int], str
    ] = field(default_factory=dict)
    reviewed_non_gating_logical_target_by_address: Mapping[str, str] = field(
        default_factory=dict
    )
    storage_containers: tuple[StorageContainer, ...] = ()
    pointer_vector_destroy_providers: tuple[
        RegisteredPointerVectorDestroyProviderSupplier, ...
    ] = ()
    hud_cmd_binding_ptr_vector_erase_provider: (
        StructuralPhysicalProviderSupplier | None
    ) = None
    scalar_deleting_destructors: tuple[
        RegisteredScalarDeletingDestructorSupplier, ...
    ] = ()
    scalar_deleting_destructor_blockers: Mapping[str, str] = field(
        default_factory=dict
    )
    scalar_deleting_destructor_legacy_fallback_names: frozenset[str] = (
        frozenset()
    )


@dataclass(frozen=True)
class CandidateAssembly:
    instructions: tuple[Instruction, ...]
    local_control_flow_indices: frozenset[int]
    local_control_flow_targets: Mapping[int, tuple[int, ...]] = field(
        default_factory=dict
    )
    target: Any | None = None
    complete_destructor_definitions: Mapping[
        str, "CandidateDestructorDefinition"
    ] = field(default_factory=dict)
    vftable_definitions: Mapping[
        str, "CandidateVftableDefinition"
    ] = field(default_factory=dict)
    tu_local_function_definitions: Mapping[
        str, "CandidateTuLocalFunctionDefinition"
    ] = field(default_factory=dict)
    compiler_local_function_definitions: Mapping[
        str, "CandidateCompilerLocalFunctionDefinition"
    ] = field(default_factory=dict)
    constructor_definitions: Mapping[
        str, "CandidateConstructorDefinition"
    ] = field(default_factory=dict)
    numeric_constructor_definitions: Mapping[
        str, "CandidateConstructorDefinition"
    ] = field(default_factory=dict)
    numeric_constructor_target: Any | None = None
    cycle_selector_constructor_definitions: Mapping[
        str, "CandidateConstructorDefinition"
    ] = field(default_factory=dict)
    cycle_selector_constructor_target: Any | None = None
    zrd_widget_constructor_definitions: Mapping[
        str, "CandidateConstructorDefinition"
    ] = field(default_factory=dict)
    zrd_widget_constructor_target: Any | None = None
    caller_definition: "CandidateCallerDefinition | None" = None
    intentionally_inlined_absence_proof: Mapping[str, Any] | None = None
    classification_only_local_control_flow_targets: Mapping[
        int, tuple[int, ...]
    ] = field(default_factory=dict)


@dataclass(frozen=True)
class CandidateExactIatRegisterLoadProof:
    """Exact definition/transfer proof for one preselected immutable IAT route.

    Candidate proofs use relative offsets.  Retail proofs additionally bind
    distinct parsed instruction indices so a definition/storage row can never
    be mistaken for its later register invocation during live extraction.
    """

    definition_offset: str
    destination: str
    object_symbol: str
    identity: str
    transfer_offsets: tuple[str, ...]
    definition_instruction_index: int | None = None
    transfer_instruction_indices: tuple[int, ...] = ()
    definition_body_offset: int | None = None
    transfer_body_offsets: tuple[int, ...] = ()
    transfer_registers: tuple[str, ...] = ()

    def __post_init__(self) -> None:
        if not self.transfer_registers:
            object.__setattr__(self, "transfer_registers", (self.destination,) * len(self.transfer_offsets))
        if len(self.transfer_registers) != len(self.transfer_offsets):
            raise ValueError("exact-IAT transfer/register populations disagree")

    def transfer_register(self, offset: str) -> str:
        if self.transfer_registers and len(self.transfer_registers) != len(self.transfer_offsets):
            raise ValueError("exact-IAT transfer/register populations disagree")
        index = self.transfer_offsets.index(offset)
        return self.transfer_registers[index] if self.transfer_registers else self.destination

    def transfer_marker(self, offset: str, *, joined: bool) -> str:
        register = self.transfer_register(offset)
        return (f"exact-iat-join({register},{self.identity})" if joined else
                f"exact-iat-load({register},{self.definition_offset},{self.identity})")


@dataclass(frozen=True)
class RetailCallerScopedProofPackage:
    """Caller-bound expected facts shared by every retail extraction stage."""

    caller_identity: str
    caller_start: str
    caller_end_exclusive: str
    instruction_objects: tuple[Instruction, ...]
    iat_instruction_objects: tuple[
        tuple[str, Instruction, tuple[Instruction, ...]], ...
    ] = ()
    stored_callback_load_indices: frozenset[int] = frozenset()
    call_cleanup_by_instruction_index: tuple[tuple[int, int], ...] = ()
    targetless_vptr_call_proofs: tuple[tuple[int, str], ...] = ()
    local_control_flow_targets: tuple[tuple[int, tuple[int, ...]], ...] = ()


@dataclass(frozen=True)
class CandidateLocalStaticDefinition:
    name: str
    section_name: str
    section_number: int
    value: int
    symbol_type: int
    storage_class: int
    aux_count: int
    data: bytes


@dataclass(frozen=True)
class CandidateCoffSymbolDefinition:
    """Immutable candidate-object symbol facts used for local-call identity.

    These facts describe only the freshly compiled COFF object.  They do not
    grant a retail address, provider classification, tracker identity, source
    model, or acceptance state.
    """

    index: int
    name: str
    value: int
    section_number: int
    symbol_type: int
    storage_class: int
    aux_count: int
    weak_external_tag_index: int | None
    weak_external_characteristics: int | None
    section_name: str
    section_size: int
    section_characteristics: int
    natural_end: int
    section_data: bytes = b""
    section_relocations: tuple[CoffRelocation, ...] = ()
    section_snapshot_number: int = 0


@dataclass(frozen=True)
class CandidateCallerDefinition:
    symbol: str
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]
    undefined_external_functions: tuple[str, ...]
    defined_external_functions: tuple[str, ...] = ()
    undefined_external_data: tuple[str, ...] = ()
    defined_external_data: tuple[str, ...] = ()
    local_static_definitions: tuple[
        CandidateLocalStaticDefinition, ...
    ] = ()
    coff_symbols: tuple[CandidateCoffSymbolDefinition, ...] = ()
    section_index: int = 0
    section_start: int = 0
    section_end: int = 0
    associated_sections: tuple[CandidateAssociatedSection, ...] = ()
    object_path: str = ""


@dataclass(frozen=True)
class CandidateVftableDefinition:
    symbol: str
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]
    section_name: str
    section_size: int
    section_is_comdat: bool
    comdat_selection: int | None
    section_external_symbols: tuple[str, ...]


@dataclass(frozen=True)
class CandidateConstructorDefinition:
    symbol: str
    instructions: tuple[Instruction, ...]
    local_control_flow_indices: frozenset[int]
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]
    section_size: int
    section_is_comdat: bool
    comdat_selection: int | None
    section_external_functions: tuple[str, ...]
    coff_symbols: tuple[CandidateCoffSymbolDefinition, ...] = ()
    section_index: int = 0
    associated_sections: tuple[CandidateAssociatedSection, ...] = ()


@dataclass(frozen=True)
class CandidateTuLocalFunctionDefinition:
    symbol: str
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]
    section_size: int
    section_external_functions: tuple[str, ...]
    section_is_comdat: bool = False
    comdat_selection: int | None = None
    instructions: tuple[Instruction, ...] = ()
    local_control_flow_indices: frozenset[int] = frozenset()
    local_control_flow_targets: Mapping[int, tuple[int, ...]] = field(
        default_factory=dict
    )
    source_provenance: str = ""
    current_source_path: str = ""
    object_path: str = ""
    symbol_index: int = -1
    section_number: int = 0
    symbol_value: int = 0
    storage_class: int = 0
    symbol_type: int = 0


@dataclass(frozen=True)
class CandidateCompilerLocalFunctionDefinition:
    """Candidate COFF facts for one static compiler-emitted COMDAT body.

    Collection alone grants no target identity.  A caller-scoped proof must
    still bind an exact name, body, relocation population, source provenance,
    ABI, and independently registered retail identity.
    """

    symbol: str
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]
    section_size: int
    section_function_symbols: tuple[str, ...]
    section_is_comdat: bool
    comdat_selection: int | None
    instructions: tuple[Instruction, ...]
    local_control_flow_indices: frozenset[int]
    local_control_flow_targets: Mapping[int, tuple[int, ...]]
    source_provenance: str


@dataclass(frozen=True)
class CandidateAssociatedSectionSymbol:
    name: str
    value: int
    section_number: int
    type: int
    storage_class: int


@dataclass(frozen=True)
class CandidateAssociatedSection:
    section_index: int
    association_section_index: int
    name: str
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]
    section_size: int
    section_is_comdat: bool
    comdat_selection: int | None
    section_external_symbols: tuple[str, ...]
    symbols: tuple[CandidateAssociatedSectionSymbol, ...]


@dataclass(frozen=True)
class CandidateDestructorDefinition:
    symbol: str
    instructions: tuple[Instruction, ...]
    local_control_flow_indices: frozenset[int]
    data: bytes
    relocations: tuple[CoffRelocation, ...]
    relocation_mask: tuple[bool, ...]
    section_size: int
    section_is_comdat: bool
    comdat_selection: int | None
    section_external_functions: tuple[str, ...]
    local_control_flow_targets: Mapping[int, tuple[int, ...]] = field(
        default_factory=dict
    )
    associated_sections: tuple[CandidateAssociatedSection, ...] = ()


@dataclass(frozen=True)
class AppFrameRunCatchFuncletProof:
    """Finite candidate partition of Run's three compiler catch funclets."""

    ranges: tuple[tuple[int, int], ...]
    lifecycle_addresses: tuple[str, ...]
    excluded_invocation_indices: tuple[int, ...]
    excluded_invocation_ordinals: tuple[int, ...]
    excluded_invocation_offsets: tuple[int, ...]
    generic_invocation_count: int


@dataclass(frozen=True)
class _Vc5NamespaceFunctionAbi:
    decorated_identity: str
    semantic_name: str
    calling_convention: str
    return_type: str
    parameter_types: tuple[str, ...]
    parameter_sizes: tuple[int, ...]
    parameter_stack_sizes: tuple[int, ...]

    @property
    def parameter_bytes(self) -> int:
        return sum(self.parameter_stack_sizes)


@dataclass(frozen=True)
class _RetailAuthoredNamespaceAbi:
    semantic_name: str
    address: str
    return_type: str
    parameter_types: tuple[str, ...]
    parameter_sizes: tuple[int, ...]
    cleanup_bytes: int


@dataclass(frozen=True)
class ReviewedVftableStorageBridgeSpec:
    label: str
    vftable_symbol: str
    caller_start: str
    caller_end_exclusive: str
    caller_symbol: str
    storage_identity: str
    storage_address: str
    storage_id: str
    owner_id: str
    owner_kind: str
    source_path: str
    table_size: int
    slot_displacement: int
    slot_symbol: str
    slot_target_address: str
    vptr_store_prefix: bytes
    invocation_count: int
    provisional_identity: str
