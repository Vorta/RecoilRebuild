"""Recoil call-contract recoil player evidence and checks."""

from __future__ import annotations

from typing import TYPE_CHECKING

from _recoil.call_contract import catalog as _cc_catalog
from _recoil.call_contract import errors as _cc_errors
from _recoil.call_contract import providers as _cc_providers

if TYPE_CHECKING:
    from _recoil.call_contract.records import CandidateAssembly, IdentityIndexes

from typing import Any, Mapping, Sequence

from _recoil.lib.progress import normalize_address


def _r4572_player_compiler_provider_candidate_bridges(
    expected: Sequence[Mapping[str, Any]],
    candidate: CandidateAssembly,
    *,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    indexes: IdentityIndexes,
) -> dict[str, str]:
    """Acquire the nine governed Player helper spellings from live call facts.

    Caller/provider authority remains finite and retail-derived. Site offsets
    come only from the current listing and matching COFF relocations; no
    historical candidate population qualifies a provider identity.
    """

    profiles: Mapping[tuple[str, str, str], tuple[str, str]] = {
        ("0x421830", "0x421a40", "__CIasin"): (
            _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY,
        ),
        ("0x427440", "0x4279f0", "__CIasin"): (
            _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY,
        ),
        ("0x427ec0", "0x428120", "__CIasin"): (
            _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY,
        ),
        ("0x42da40", "0x42db50", "__CIasin"): (
            _cc_catalog.MSVC_CIASIN_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_CIASIN_TARGET_IDENTITY,
        ),
        ("0x423b10", "0x423c20", "__chkstk"): (
            _cc_catalog.MSVC_CHKSTK_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_CHKSTK_TARGET_IDENTITY,
        ),
        ("0x428d60", "0x4290f0", "__chkstk"): (
            _cc_catalog.MSVC_CHKSTK_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_CHKSTK_TARGET_IDENTITY,
        ),
        ("0x42bf90", "0x42c0d0", "__chkstk"): (
            _cc_catalog.MSVC_CHKSTK_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_CHKSTK_TARGET_IDENTITY,
        ),
        ("0x426770", "0x427140", "__ftol"): (
            _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY,
        ),
        ("0x429870", "0x429b40", "__ftol"): (
            _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL,
            _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY,
        ),
    }
    start = normalize_address(caller_start)
    end = normalize_address(caller_end_exclusive)
    matching_profiles = [
        value
        for (profile_start, profile_end, _label), value in profiles.items()
        if profile_start == start and profile_end == end
    ]
    if not matching_profiles:
        return {}
    if (
        len(matching_profiles) != 1
        or caller_identity != f"symbol:recoil:function:{start}"
        or indexes.by_address.get(start) != caller_identity
        or caller_identity in indexes.provider_ids
    ):
        raise _cc_errors.CandidateCallContractEvidenceError(
            "Player compiler-provider bridge requires one exact authored caller"
        )
    name, provider_identity = matching_profiles[0]
    if provider_identity not in indexes.provider_ids:
        raise _cc_errors.CandidateCallContractEvidenceError(
            "Player compiler-provider bridge lacks its exact governed provider"
        )
    return _cc_providers._prove_same_ordinal_compiler_provider_calls(
        expected, candidate, name=name, provider_identity=provider_identity,
    )


def _merge_player_ftol_expected_direct_bridges(
    compiler_generated_bridges: Mapping[str, str],
    expected_direct_identity_bridges: Mapping[str, str],
    *,
    caller_start: str,
    player_compiler_provider_bridges: Mapping[str, str],
    ftol_provider_bridges: Mapping[str, str],
) -> dict[str, str]:
    """Prefer the exact Player provider identity over ``iat:_ftol``.

    The generic ``__ftol`` candidate proof deliberately publishes the IAT
    storage identity used to reach the CRT provider.  Two current Player
    callers also have a complete caller-local OBJ/COD proof that every
    ``__ftol`` occurrence is the immutable retail provider identity, including
    sites whose retail ordinal contains a later authored call.  Admit that
    stronger classification only when both proof producers are complete and
    agree on this exact finite boundary.  No invocation row is projected,
    removed, reordered, or otherwise changed here.
    """

    merged = dict(compiler_generated_bridges)
    conflicts = merged.keys() & expected_direct_identity_bridges.keys()
    identity_conflicts = {
        name
        for name in conflicts
        if merged[name] != expected_direct_identity_bridges[name]
    }
    if identity_conflicts:
        exact_player_ftol_conflict = (
            normalize_address(caller_start) in {"0x426770", "0x429870"}
            and identity_conflicts == {_cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL}
            and player_compiler_provider_bridges
            == {
                _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL:
                _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY,
            }
            and ftol_provider_bridges
            == {
                _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL:
                _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY,
            }
            and merged.get(_cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL)
            == _cc_catalog.MSVC_FTOL_IAT_STORAGE_IDENTITY
            and expected_direct_identity_bridges.get(
                _cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL
            ) == _cc_catalog.MSVC_FTOL_PROVIDER_IDENTITY
        )
        if not exact_player_ftol_conflict:
            raise ValueError(
                "expected authored direct candidate bridge conflicts with "
                "another reviewed compiler/provider bridge"
            )
        del merged[_cc_catalog.MSVC_FTOL_CANDIDATE_SYMBOL]
    merged.update(expected_direct_identity_bridges)
    return merged


def _player_reviewed_cstring_ordinal_profile(
    *,
    callable_symbol: str,
    caller_identity: str,
    caller_start: str,
    caller_end_exclusive: str,
    provider_identity: str,
    instruction_offsets: Sequence[str],
    candidate_rows: Sequence[Mapping[str, Any]],
    expected_rows: Sequence[Mapping[str, Any]],
) -> bool:
    """Recognize exact CString dtor ordinals after finite Player inlining."""

    profiles = {
        (
            "symbol:recoil:function:0x420c60",
            "0x420c60",
            "0x420d10",
            "??1CString@@QAE@XZ",
        ): (
            "provider:recoil:function:0x4c5b88",
            ("0x9f",),
            (7,),
            (6,),
        ),
        (
            "symbol:recoil:function:0x425060",
            "0x425060",
            "0x425150",
            "??1CString@@QAE@XZ",
        ): (
            "provider:recoil:function:0x4c5b88",
            ("0x95", "0xa6", "0xce", "0xdf"),
            (6, 7, 8, 9),
            (3, 4, 5, 6),
        ),
        (
            "symbol:recoil:function:0x425060",
            "0x425060",
            "0x425150",
            "?Right@CString@@QBE?AV1@H@Z",
        ): (
            "provider:recoil:function:0x4c5cd8",
            ("0x60",),
            (2,),
            (1,),
        ),
    }
    profile = profiles.get((
        caller_identity,
        normalize_address(caller_start),
        normalize_address(caller_end_exclusive),
        callable_symbol,
    ))
    return bool(
        profile is not None
        and provider_identity == profile[0]
        and tuple(instruction_offsets) == profile[1]
        and tuple(row.get("ordinal") for row in candidate_rows)
        == profile[2]
        and tuple(row.get("ordinal") for row in expected_rows)
        == profile[3]
        and len(candidate_rows) == len(expected_rows) == len(profile[1])
        and all(
            {
                key: value
                for key, value in candidate_row.items()
                if key != "ordinal"
            }
            == {
                key: value
                for key, value in expected_row.items()
                if key != "ordinal"
            }
            for candidate_row, expected_row in zip(
                candidate_rows, expected_rows
            )
        )
    )
