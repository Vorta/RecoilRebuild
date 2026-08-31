from __future__ import annotations

import json
import contextlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch


REPO_ROOT = Path(__file__).resolve().parents[2]
TOOLS_ROOT = REPO_ROOT / "tools"
if str(TOOLS_ROOT) not in sys.path:
    sys.path.insert(0, str(TOOLS_ROOT))

from _recoil.lib.source_traceability import (  # noqa: E402
    load_artifact_rows,
    merge_source_trace_documents,
    parse_source_trace_path,
    parse_source_trace_text,
    validate_source_trace,
)
from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402
from _recoil.commands import source_trace_audit  # noqa: E402


class _SQLiteProgressFixture:
    """Path-like test adapter that persists partial tracker fixtures as SQLite."""

    def __init__(self, path: Path) -> None:
        self.path = path

    def __fspath__(self) -> str:
        return str(self.path)

    def __str__(self) -> str:
        return str(self.path)

    def write_text(self, text: str, *, encoding: str) -> int:
        if encoding != "utf-8":
            raise ValueError("progress fixtures must use UTF-8")
        fragment = json.loads(text)
        document = empty_progress_document()
        document.update(fragment)
        ProgressSQLiteStore.create_from_mapping(
            self.path,
            document,
            cutover_pair_id="source-traceability-test",
            overwrite=self.path.exists(),
        )
        return len(text)

    def read_text(self, *, encoding: str) -> str:
        if encoding != "utf-8":
            raise ValueError("progress fixtures must use UTF-8")
        document = ProgressSQLiteStore(self.path, read_only=True).materialize()
        return json.dumps(document)


class SourceTraceabilityTests(unittest.TestCase):
    def test_attached_anchor_can_define_and_emit_many_function_artifacts(self) -> None:
        source = (
            "/**\n"
            " * @recoil-anchor recoil:anchor:sample-function\n"
            " * @recoil-artifact defines .text recoil:function:0x401000: Direct body.\n"
            " * @recoil-artifact emits .text recoil:function:0x401020: Scalar deleting destructor.\n"
            " */\n"
            "int Sample(int value) { return value; }\n"
        )

        document = parse_source_trace_text(source, path="src/sample.cpp")

        self.assertEqual(1, len(document.anchors))
        self.assertEqual(2, len(document.artifacts))
        self.assertEqual((), document.findings)
        self.assertTrue(all(item.direct for item in document.artifacts))
        self.assertEqual("function", document.anchors[0].construct.kind)
        body_offset = source.index("return")
        self.assertEqual(
            "recoil:function:0x401000",
            document.direct_defining_function_at(body_offset).artifact_id,
        )

    def test_consecutive_line_comment_directives_are_grouped_but_nonqualifying(self) -> None:
        source = (
            "// @recoil-anchor recoil:anchor:sample-data\n"
            "// @recoil-artifact defines .data recoil:data:0x500000: Direct global.\n"
            "int g_Sample = 1;\n"
        )

        document = parse_source_trace_text(source)

        self.assertEqual(1, len(document.anchors))
        self.assertEqual("data", document.anchors[0].construct.kind)
        self.assertEqual("line", document.anchors[0].comment_style)
        self.assertEqual(["invalid-comment-style"], [item.code for item in document.findings])

    def test_ordinary_block_comment_is_not_canonical(self) -> None:
        document = parse_source_trace_text(
            "/*\n"
            " * @recoil-anchor recoil:anchor:sample\n"
            " * @recoil-artifact defines .data recoil:data:0x500000: Sample.\n"
            " */\n"
            "int g_Sample;\n"
        )
        self.assertEqual("block", document.anchors[0].comment_style)
        self.assertIn("invalid-comment-style", {item.code for item in document.findings})

    def test_extern_declaration_is_not_a_definition_anchor(self) -> None:
        document = parse_source_trace_text(
            "/**\n"
            " * @recoil-anchor recoil:anchor:extern\n"
            " * @recoil-artifact defines .data recoil:data:0x500000: Not storage.\n"
            " */\n"
            "extern int g_Sample;\n"
        )
        self.assertIsNone(document.anchors[0].construct)
        self.assertEqual("unsupported-extern-declaration", document.anchors[0].attachment_status)
        self.assertIn(
            "unsupported-extern-declaration",
            {item.code for item in document.findings},
        )

    def test_message_map_macro_anchor_is_a_supported_source_generation_region(self) -> None:
        document = parse_source_trace_text(
            "/** Reimplements 0x401000: Message map provider. */\n"
            "BEGIN_MESSAGE_MAP(CSample, CDialog)\n"
            "END_MESSAGE_MAP()\n"
        )
        self.assertEqual("attached", document.legacy_artifacts[0].attachment_status)
        self.assertEqual("macro", document.legacy_artifacts[0].construct.kind)

    def test_extern_initializer_is_data_definition_but_declaration_is_not(self) -> None:
        defined = parse_source_trace_text(
            "/**\n"
            " * @recoil-anchor recoil:anchor:external-value\n"
            " * @recoil-artifact defines .rdata recoil:data:0x500000: Value.\n"
            " */\n"
            "extern const float g_Value = 0.5f;\n"
        )
        declared = parse_source_trace_text(
            "/**\n"
            " * @recoil-anchor recoil:anchor:external-declaration\n"
            " * @recoil-artifact defines .rdata recoil:data:0x500000: Value.\n"
            " */\n"
            "extern const float g_Value;\n"
        )

        self.assertEqual("data", defined.artifacts[0].construct.kind)
        self.assertEqual("attached", defined.artifacts[0].attachment_status)
        self.assertIsNone(declared.artifacts[0].construct)
        self.assertEqual(
            "unsupported-extern-declaration",
            declared.artifacts[0].attachment_status,
        )

    def test_marker_looking_strings_are_not_inventory_or_directives(self) -> None:
        document = parse_source_trace_text(
            'const char *text = "@recoil-anchor recoil:anchor:nope '
            'Reimplements 0x401000: nope";\n'
        )

        self.assertEqual((), document.anchors)
        self.assertEqual((), document.artifacts)
        self.assertEqual((), document.legacy_artifacts)
        self.assertEqual((), document.unsupported_legacy_addresses)

    def test_unsupported_legacy_address_syntax_is_comment_only_cleanup_inventory(self) -> None:
        source = (
            "/** Reimplements data 0x500000..0x500010: Legacy data group. */\n"
            "int g_First;\n"
            "// Provenance: Reimplements 0x401000: Embedded legacy provenance.\n"
            "int First() { return 1; }\n"
            "// Emits 0x401020\n"
            "int Second() { return 2; }\n"
            'const char *text = "Reimplements 0x401030: string literal";\n'
            "/**\n"
            " * @recoil-anchor recoil:anchor:canonical\n"
            " * @recoil-artifact defines .text recoil:function:0x401040: "
            "Canonical description mentions Reimplements 0x401050 and Emits 0x401060.\n"
            " */\n"
            "int Canonical() { return 3; }\n"
        )

        document = parse_source_trace_text(source, path="src/sample.cpp")

        self.assertEqual((), document.legacy_artifacts)
        self.assertEqual(
            [
                ("Reimplements", "0x500000"),
                ("Reimplements", "0x401000"),
                ("Emits", "0x401020"),
            ],
            [
                (item.marker, item.address)
                for item in document.unsupported_legacy_addresses
            ],
        )
        self.assertEqual(1, len(document.artifacts))
        self.assertEqual("recoil:function:0x401040", document.artifacts[0].artifact_id)

    def test_unsupported_legacy_catchall_ignores_outside_source_document(self) -> None:
        document = parse_source_trace_text(
            "/** Reimplements data 0x500000..0x500010: Documentation example. */\n",
            path="docs/source_trace_notes.md",
        )

        self.assertEqual((), document.unsupported_legacy_addresses)

    def test_detached_invalid_anchor_relation_and_id_fail_closed(self) -> None:
        source = (
            "/**\n"
            " * @recoil-anchor BadAnchor\n"
            " * @recoil-artifact Defines .data recoil:data:0x500000: Wrong relation case.\n"
            " */\n"
            "int intervening;\n"
            "int Sample() { return 0; }\n"
        )

        document = parse_source_trace_text(source)
        codes = {item.code for item in document.findings}

        self.assertIn("invalid-anchor-id", codes)
        self.assertIn("invalid-relation", codes)
        # It is directly attached to data; the uppercase relation is invalid,
        # independently of whether a later function follows.
        self.assertEqual("data", document.anchors[0].construct.kind)

    def test_defines_must_match_direct_construct_kind(self) -> None:
        source = (
            "/**\n"
            " * @recoil-anchor recoil:anchor:wrong-kind\n"
            " * @recoil-artifact defines .data recoil:data:0x500000: Not a function.\n"
            " */\n"
            "int Sample() { return 0; }\n"
        )

        document = parse_source_trace_text(source)

        self.assertIn("wrong-relation", {item.code for item in document.findings})

    def test_legacy_rows_are_inventory_only_in_strict_validation(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            progress = _SQLiteProgressFixture(Path(temp_dir) / "progress.sqlite3")
            progress.write_text(
                json.dumps(
                    {
                        "symbols": {
                            "recoil:function:0x401000": {
                                "kind": "function",
                                "pipeline_class": "authored",
                                "output_section_id": "recoil:section:.text",
                            }
                        },
                        "output_sections": {"recoil:section:.text": {"name": ".text"}},
                    }
                ),
                encoding="utf-8",
            )
            document = parse_source_trace_text(
                "/** Reimplements 0x401000: Legacy body. */\n"
                "int Sample() { return 0; }\n",
                path="src/sample.cpp",
            )

            findings = validate_source_trace(document, load_artifact_rows(progress), strict=True)

        self.assertEqual(1, len(document.legacy_artifacts))
        self.assertEqual(
            ["legacy-marker-nonqualifying"],
            [item.code for item in findings],
        )

    def test_legacy_messages_path_and_explicit_binary_route_candidate_namespace(self) -> None:
        source = "/** Reimplements 0x10001010: Message export. */\nint Export() { return 1; }\n"
        inferred = parse_source_trace_text(source, path="src/Messages/messages.c")
        explicit = parse_source_trace_text(
            source,
            path="C:/scratch/messages.c",
            legacy_binary="messages",
        )

        self.assertEqual(
            "messages:function:0x10001010",
            inferred.legacy_artifacts[0].artifact_id,
        )
        self.assertEqual(
            "messages:function:0x10001010",
            explicit.legacy_artifacts[0].artifact_id,
        )

    def test_tracker_lookup_resolves_physical_and_logical_rows_and_sections(self) -> None:
        alias = "messages:logical-function:0x10001000:sample-alias"
        with tempfile.TemporaryDirectory() as temp_dir:
            progress = _SQLiteProgressFixture(Path(temp_dir) / "progress.sqlite3")
            progress.write_text(
                json.dumps(
                    {
                        "symbols": {
                            "messages:function:0x10001000": {
                                "kind": "function",
                                "output_section_id": "messages:section:.text",
                                "logical_aliases": {alias: {"kind": "function"}},
                            },
                            "recoil:data:0x500000": {
                                "kind": "data",
                                "output_section_id": None,
                            },
                        },
                        "output_sections": {
                            "messages:section:.text": {"name": ".text"},
                        },
                    }
                ),
                encoding="utf-8",
            )
            index = load_artifact_rows(progress)

        logical = index.resolve(alias)
        self.assertIsNotNone(logical)
        self.assertTrue(logical.logical)
        self.assertEqual("messages:function:0x10001000", logical.physical_id)
        self.assertEqual(".text", logical.output_section)
        self.assertIsNone(index.resolve("recoil:data:0x500000").output_section)

    def test_logical_data_requires_explicit_nested_tracker_alias(self) -> None:
        logical_id = "recoil:logical-data:0x500000:sample-field"
        source = (
            "/**\n"
            " * @recoil-anchor recoil:anchor:sample-field\n"
            f" * @recoil-artifact defines .data {logical_id}: Logical data row.\n"
            " */\n"
            "int g_Sample = 1;\n"
        )
        with tempfile.TemporaryDirectory() as temp_dir:
            progress = _SQLiteProgressFixture(Path(temp_dir) / "progress.sqlite3")
            progress.write_text(
                json.dumps(
                    {
                        "symbols": {
                            "recoil:data:0x500000": {
                                "kind": "data",
                                "output_section_id": "recoil:section:.data",
                            }
                        },
                        "output_sections": {"recoil:section:.data": {"name": ".data"}},
                    }
                ),
                encoding="utf-8",
            )
            document = parse_source_trace_text(source)
            unresolved = validate_source_trace(
                document,
                load_artifact_rows(progress),
                strict=False,
            )
            payload = json.loads(progress.read_text(encoding="utf-8"))
            payload["symbols"]["recoil:data:0x500000"]["logical_aliases"] = {
                logical_id: {"kind": "data"}
            }
            progress.write_text(json.dumps(payload), encoding="utf-8")
            resolved_index = load_artifact_rows(progress)
            resolved = validate_source_trace(document, resolved_index, strict=False)

        self.assertIn("unreviewed-logical-data", {item.code for item in unresolved})
        self.assertEqual((), resolved)
        self.assertEqual("recoil:data:0x500000", resolved_index.resolve(logical_id).physical_id)

    def test_validation_reports_wrong_unknown_and_unresolved_sections(self) -> None:
        source = (
            "/**\n"
            " * @recoil-anchor recoil:anchor:sample\n"
            " * @recoil-artifact defines .data recoil:function:0x401000: Wrong section.\n"
            " * @recoil-artifact emits .rdata recoil:function:0x401020: Missing section fact.\n"
            " * @recoil-artifact emits .text recoil:function:0x401040: Unknown row.\n"
            " */\n"
            "int Sample() { return 0; }\n"
        )
        with tempfile.TemporaryDirectory() as temp_dir:
            progress = _SQLiteProgressFixture(Path(temp_dir) / "progress.sqlite3")
            progress.write_text(
                json.dumps(
                    {
                        "symbols": {
                            "recoil:function:0x401000": {
                                "kind": "function",
                                "output_section_id": "recoil:section:.text",
                            },
                            "recoil:function:0x401020": {
                                "kind": "function",
                                "output_section_id": None,
                            },
                        },
                        "output_sections": {},
                    }
                ),
                encoding="utf-8",
            )
            findings = validate_source_trace(
                parse_source_trace_text(source, path="src/sample.cpp"),
                load_artifact_rows(progress),
                strict=False,
            )

        self.assertEqual(
            {"wrong-section", "wrong-or-unknown-section", "unknown-artifact-id"},
            {item.code for item in findings},
        )

    def test_cross_file_duplicate_ids_are_reported(self) -> None:
        first = parse_source_trace_text(
            "/**\n"
            " * @recoil-anchor recoil:anchor:same\n"
            " * @recoil-artifact defines .text recoil:function:0x401000: First.\n"
            " */\n"
            "int First() { return 1; }\n",
            path="first.cpp",
        )
        second = parse_source_trace_text(
            "/**\n"
            " * @recoil-anchor recoil:anchor:same\n"
            " * @recoil-artifact defines .text recoil:function:0x401000: Second.\n"
            " */\n"
            "int Second() { return 2; }\n",
            path="second.cpp",
        )

        findings = merge_source_trace_documents((first, second))

        self.assertEqual(
            {"duplicate-anchor-id", "duplicate-artifact-id"},
            {item.code for item in findings},
        )

    def test_path_decode_reports_encoding_and_newline_without_ignoring_bytes(self) -> None:
        source = (
            "/**\r\n"
            " * @recoil-anchor recoil:anchor:cafe\r\n"
            " * @recoil-artifact defines .data recoil:data:0x500000: Caf\xe9 global.\r\n"
            " */\r\n"
            "int g_Cafe = 1;\r\n"
        ).encode("cp1252")
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "sample.cpp"
            path.write_bytes(source)
            document = parse_source_trace_path(path)

        self.assertEqual("cp1252", document.encoding)
        self.assertEqual("crlf", document.newline)
        self.assertIn("Café", document.artifacts[0].description)

    def test_mixed_newlines_fail_clearly(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "sample.cpp"
            path.write_bytes(b"// one\r\n// two\nint value;\n")
            with self.assertRaisesRegex(ValueError, "mixed newline styles"):
                parse_source_trace_path(path)

    def test_legacy_attachment_statuses_distinguish_preprocessor_stacked_and_eof(self) -> None:
        document = parse_source_trace_text(
            "/** Reimplements 0x401000: Before preprocessor. */\n"
            "#define SAMPLE 1\n"
            "/** Reimplements 0x401020: Stacked. */\n"
            "/* intervening */\n"
            "int Sample() { return 0; }\n"
            "/** Emits 0x401040: End of file. */"
        )

        self.assertEqual(
            ["preprocessor", "stacked-comment", "eof"],
            [item.attachment_status for item in document.legacy_artifacts],
        )

    def test_recoil_cli_routes_read_only_source_trace_audit(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            source = root / "src" / "sample.cpp"
            source.parent.mkdir()
            progress = _SQLiteProgressFixture(root / "progress.sqlite3")
            source.write_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:sample\n"
                " * @recoil-artifact defines .text recoil:function:0x401000: Sample.\n"
                " */\n"
                "int Sample() { return 0; }\n",
                encoding="utf-8",
            )
            progress.write_text(
                json.dumps(
                    {
                        "symbols": {
                            "recoil:function:0x401000": {
                                "kind": "function",
                                "output_section_id": "recoil:section:.text",
                                "source_traceability": {
                                    "state": "resolved",
                                    "source_edges": [
                                        {
                                            "relation": "defines",
                                            "anchor_id": "recoil:anchor:sample",
                                            "emission_context": {
                                                "translation_unit": "src/sample.cpp",
                                            },
                                        }
                                    ],
                                },
                            }
                        },
                        "output_sections": {"recoil:section:.text": {"name": ".text"}},
                    }
                ),
                encoding="utf-8",
            )
            stdout = io.StringIO()
            with (
                patch.object(source_trace_audit, "REPO_ROOT", root),
                contextlib.redirect_stdout(stdout),
            ):
                result = source_trace_audit.main(
                    ["--progress", str(progress), "--json"]
                )

        self.assertEqual(0, result, stdout.getvalue())
        payload = json.loads(stdout.getvalue())
        self.assertEqual("source-trace-audit-v2", payload["schema"])
        self.assertEqual("passed", payload["result"])
        self.assertTrue(payload["topology_only"])
        self.assertEqual("none", payload["acceptance_effect"])

    def test_unresolved_function_cannot_claim_canonical_source_edge(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            root = Path(temp_dir)
            progress = _SQLiteProgressFixture(root / "progress.sqlite3")
            progress.write_text(
                json.dumps(
                    {
                        "symbols": {
                            "recoil:function:0x401000": {
                                "kind": "function",
                                "pipeline_class": "unresolved",
                                "output_section_id": "recoil:section:.text",
                            }
                        },
                        "output_sections": {
                            "recoil:section:.text": {"name": ".text"}
                        },
                    }
                ),
                encoding="utf-8",
            )
            index = load_artifact_rows(progress)
            document = parse_source_trace_text(
                "/**\n"
                " * @recoil-anchor recoil:anchor:sample\n"
                " * @recoil-artifact defines .text recoil:function:0x401000: Sample.\n"
                " */\n"
                "int Sample() { return 0; }\n",
                path="src/sample.cpp",
            )

        self.assertIn(
            "non-authored-function-source-edge",
            {item.code for item in validate_source_trace(document, index)},
        )


if __name__ == "__main__":
    unittest.main()
