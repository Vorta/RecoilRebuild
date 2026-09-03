from pathlib import Path
from types import SimpleNamespace
import sys


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))


from _recoil.commands.zui_inline_context import (
    compact_zui_inline_summary,
    summarize_zui_inline_context,
)
from _recoil.commands.call_contract_verify import (
    _canonical_zui_relocation_names,
)


def _relocation(offset: int, symbol: str, relocation_type: int = 0x14):
    return SimpleNamespace(
        offset=offset,
        symbol_index=1,
        type=relocation_type,
        symbol_name=symbol,
    )


def test_summary_reports_direct_insert_sites_and_complete_local_provider() -> None:
    symbol = (
        "?insert@?$vector@PAUHudUiPanel@@V?$allocator@PAUHudUiPanel@@@std@@@std@@"
        "QAEPAPAUHudUiPanel@@PAPAU3@IPBQAQAU3@@Z"
    )
    caller_data = bytearray(0x80)
    caller_data[0x19] = 0xE8
    caller_data[0x39] = 0xE8
    helper = SimpleNamespace(
        data=bytes(0x220),
        section_size=0x220,
        section_is_comdat=True,
        comdat_selection=2,
        source_provenance="D:/Compiler/VC5/INCLUDE/VECTOR",
        relocations=(_relocation(0x20, "_Construct"),),
    )
    candidate = SimpleNamespace(
        caller_definition=SimpleNamespace(
            symbol="?LoadFromZrd@HudUiZrdWidget@@QAEHPAUNode@zReader@@I@Z",
            data=bytes(caller_data),
            relocations=(_relocation(0x1A, symbol), _relocation(0x3A, symbol)),
        ),
        tu_local_function_definitions={symbol: helper},
    )

    result = summarize_zui_inline_context(candidate)

    assert result["caller_extent"] == "0x80"
    assert result["caller_body_sha256"] == (
        "e34639f9e0c87712423c9716d2d5f82a9c0bca37d0c97a282ea77eeee51426b4"
    )
    assert result["caller_relocation_count"] == 2
    assert len(result["caller_relocations_sha256"]) == 64
    assert len(result["caller_canonical_relocations_sha256"]) == 64
    assert [row["symbol"] for row in result["direct_symbol_calls"]] == [
        symbol,
        symbol,
    ]
    assert result["direct_insert_call_count"] == 2
    assert result["expanded_insert_site_count"] == 6
    assert [row["instruction_offset"] for row in result["direct_insert_calls"]] == [
        "0x19",
        "0x39",
    ]
    assert all(row["is_direct_call"] for row in result["direct_insert_calls"])
    assert result["insert_provider_definitions"] == [
        {
            "symbol": symbol,
            "extent": "0x220",
            "section_size": "0x220",
            "section_is_comdat": True,
            "comdat_selection": 2,
            "source_provenance": "D:/Compiler/VC5/INCLUDE/VECTOR",
            "relocations": [
                {"offset": "0x20", "type": 0x14, "symbol": "_Construct"}
            ],
        }
    ]
    assert result["direct_local_helper_definitions"] == (
        result["direct_local_vector_helper_definitions"]
    )
    assert result["direct_local_vector_helper_definitions"] == [
        {
            "symbol": symbol,
            "extent": "0x220",
            "body_sha256": (
                "44ddd2f478477ebd1c1cd5b99400af48cd46033c59173195f48870e608cec810"
            ),
            "body_hex": "00" * 0x220,
            "section_size": "0x220",
            "section_is_comdat": True,
            "comdat_selection": 2,
            "source_provenance": "D:/Compiler/VC5/INCLUDE/VECTOR",
            "relocations": [
                {"offset": "0x20", "type": 0x14, "symbol": "_Construct"}
            ],
            "direct_calls": [],
        }
    ]

    compact = compact_zui_inline_summary(result)
    assert "direct_symbol_calls" not in compact
    assert compact["direct_symbol_call_count"] == 2
    assert "body_hex" not in compact["direct_local_helper_definitions"][0]


def test_summary_accepts_related_caller_site_count() -> None:
    candidate = SimpleNamespace(
        caller_definition=SimpleNamespace(
            symbol="related",
            data=b"\x90",
            relocations=(),
        ),
        tu_local_function_definitions={},
    )

    result = summarize_zui_inline_context(candidate, append_site_count=2)

    assert result["append_site_count"] == 2
    assert result["direct_symbol_calls"] == []
    assert result["direct_insert_call_count"] == 0
    assert result["expanded_insert_site_count"] == 2
    assert result["direct_local_helper_definitions"] == []
    assert result["direct_local_vector_helper_definitions"] == []


def test_private_relocation_serials_preserve_equivalence_classes() -> None:
    helper_prefix = (
        "?ZrdArrayInt@?%D:\\Recoil Project\\RecoilRebuild\\src\\"
        "GameZRecoil\\zUI\\zui_widgets.cpp"
    )
    before = (
        _relocation(0x09, "$L85500", 0x06),
        _relocation(0x30, "$T85480", 0x06),
        _relocation(0x40, "$T84256", 0x06),
        _relocation(0x50, "$T85480", 0x06),
        _relocation(0x60, helper_prefix + "10007127@@YIHPAUNode@zReader@@HH@Z"),
        _relocation(0x70, "ordinary_symbol"),
    )
    after = (
        _relocation(0x09, "$L85504", 0x06),
        _relocation(0x30, "$T85484", 0x06),
        _relocation(0x40, "$T84260", 0x06),
        _relocation(0x50, "$T85484", 0x06),
        _relocation(0x60, helper_prefix + "233349164@@YIHPAUNode@zReader@@HH@Z"),
        _relocation(0x70, "ordinary_symbol"),
    )

    assert _canonical_zui_relocation_names(before) == (
        "$L<local-0>",
        "$T<local-0>",
        "$T<local-1>",
        "$T<local-0>",
        helper_prefix + "<tu-discriminator>@@YIHPAUNode@zReader@@HH@Z",
        "ordinary_symbol",
    )
    assert _canonical_zui_relocation_names(before) == (
        _canonical_zui_relocation_names(after)
    )
