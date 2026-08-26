from __future__ import annotations

import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.source_trace_migrate import (  # noqa: E402
    INVENTORY_SCHEMA,
    SourceTraceMigrationError,
    build_migration_template,
    build_tracker_replace_payload,
    decode_source_bytes,
    inventory_legacy_source,
    load_migration_inventory,
    non_comment_token_stream,
    parse_migration_inventory,
    propose_source_trace_rewrite,
    propose_source_trace_batch,
    review_conservative_template,
    verify_comment_only_equivalence,
)
from _recoil.lib.progress import empty_progress_document  # noqa: E402
from _recoil.lib.progress_sqlite import ProgressSQLiteStore  # noqa: E402
from _recoil.lib.source_traceability import load_artifact_rows  # noqa: E402


def progress_file(root: Path) -> Path:
    path = root / "progress.sqlite3"
    document = empty_progress_document()
    document["symbols"] = {
        "recoil:function:0x401000": {
            "kind": "function",
            "pipeline_class": "authored",
            "output_section_id": "recoil:section:.text",
        },
        "recoil:function:0x401020": {
            "kind": "function",
            "pipeline_class": "authored",
            "output_section_id": "recoil:section:.text",
        },
        "recoil:function:0x401030": {
            "kind": "function",
            "pipeline_class": "authored-lifecycle",
            "output_section_id": "recoil:section:.text",
        },
    }
    document["output_sections"] = {
        "recoil:section:.text": {"name": ".text"},
    }
    ProgressSQLiteStore.create_from_mapping(
        path,
        document,
        cutover_pair_id="source-trace-migrate-test",
    )
    return path


def inventory_row(
    *,
    line: int,
    legacy: str,
    description: str,
    artifact_id: str | None,
    state: str = "resolved",
    expected_relation: str = "defines",
    relation: str = "defines",
    reason_code: str | None = None,
    record_tracker_state: bool = True,
) -> dict[str, object]:
    resolved = state == "resolved"
    return {
        "path": "src/sample.cpp",
        "line": line,
        "expected_relation": expected_relation,
        "relation": relation,
        "expected_legacy_artifact_id": legacy,
        "expected_description": description,
        "state": state,
        "artifact_id": artifact_id,
        "anchor_id": "recoil:anchor:sample" if resolved else None,
        "output_section": ".text" if resolved else None,
        "translation_unit": "src/sample.cpp" if resolved else None,
        "reason_code": reason_code,
        "reviewed": True,
        "record_tracker_state": record_tracker_state,
    }


def inventory(*rows: dict[str, object]):
    return parse_migration_inventory(
        {"schema": INVENTORY_SCHEMA, "rows": list(rows)}
    )


class RecoilSourceTraceMigrateTests(unittest.TestCase):
    def test_resolved_rewrite_preserves_utf8_bom_crlf_and_tokens(self) -> None:
        source = (
            "/**\r\n"
            " * Reimplements 0x401000: Direct body.\r\n"
            " */\r\n"
            "int Sample(int value) { return value; }\r\n"
        )
        raw = b"\xef\xbb\xbf" + source.encode("utf-8")
        with tempfile.TemporaryDirectory() as tmp:
            index = load_artifact_rows(progress_file(Path(tmp)))
            proposal = propose_source_trace_rewrite(
                raw,
                path="src/sample.cpp",
                inventory=inventory(
                    inventory_row(
                        line=2,
                        legacy="recoil:function:0x401000",
                        description="Direct body.",
                        artifact_id="recoil:function:0x401000",
                    )
                ),
                artifact_index=index,
            )

        self.assertTrue(proposal.ready)
        self.assertTrue(proposal.token_equivalent)
        self.assertEqual((), proposal.debts)
        self.assertTrue(proposal.proposed_bytes.startswith(b"\xef\xbb\xbf"))
        self.assertNotIn(b"\n", proposal.proposed_bytes.replace(b"\r\n", b""))
        self.assertIn("@recoil-anchor recoil:anchor:sample", proposal.proposed_text)
        self.assertIn(
            "@recoil-artifact defines .text recoil:function:0x401000: Direct body.",
            proposal.proposed_text,
        )
        self.assertNotIn("Reimplements 0x401000", proposal.proposed_text)
        self.assertEqual(
            non_comment_token_stream(source),
            non_comment_token_stream(proposal.proposed_text),
        )
        self.assertEqual(
            {
                "artifact_id": "recoil:function:0x401000",
                "source_traceability": {
                    "state": "resolved",
                    "source_edges": [
                        {
                            "relation": "defines",
                            "anchor_id": "recoil:anchor:sample",
                            "emission_context": {
                                "translation_unit": "src/sample.cpp"
                            },
                            "evidence_ids": [],
                        }
                    ],
                    "reason_code": None,
                },
            },
            proposal.tracker_states[0],
        )

    def test_reviewed_unresolved_and_missing_identity_remove_address_markers(self) -> None:
        source = (
            "/**\n"
            " * Reimplements 0x401000: Direct body.\n"
            " * Reimplements 0x401020: Provider lifecycle body.\n"
            " * Reimplements 0x430230: Missing legacy identity.\n"
            " */\n"
            "int Sample() { return 0; }\n"
        )
        rows = (
            inventory_row(
                line=2,
                legacy="recoil:function:0x401000",
                description="Direct body.",
                artifact_id="recoil:function:0x401000",
            ),
            inventory_row(
                line=3,
                legacy="recoil:function:0x401020",
                description="Provider lifecycle body.",
                artifact_id="recoil:function:0x401020",
                state="not-applicable",
                reason_code="provider-boundary",
            ),
            inventory_row(
                line=4,
                legacy="recoil:function:0x430230",
                description="Missing legacy identity.",
                artifact_id=None,
                state="unresolved",
                reason_code="missing-artifact-identity",
                record_tracker_state=False,
            ),
        )
        with tempfile.TemporaryDirectory() as tmp:
            index = load_artifact_rows(progress_file(Path(tmp)))
            proposal = propose_source_trace_rewrite(
                source,
                path="src/sample.cpp",
                inventory=inventory(*rows),
                artifact_index=index,
            )

        self.assertTrue(proposal.ready)
        self.assertEqual(
            {"provider-boundary", "missing-artifact-identity"},
            {item.reason_code for item in proposal.debts},
        )
        self.assertNotIn("0x401020", proposal.proposed_text)
        self.assertNotIn("0x430230", proposal.proposed_text)
        self.assertIn("Provider lifecycle body.", proposal.proposed_text)
        self.assertIn("Missing legacy identity.", proposal.proposed_text)
        self.assertEqual(2, len(proposal.tracker_states))
        unresolved = proposal.tracker_states[1]["source_traceability"]
        self.assertEqual("not-applicable", unresolved["state"])
        self.assertEqual([], unresolved["source_edges"])
        self.assertEqual("provider-boundary", unresolved["reason_code"])

    def test_legacy_reimplements_may_be_reviewed_as_canonical_emits(self) -> None:
        source = (
            "/**\n"
            " * Reimplements 0x401030: Compiler lifecycle body.\n"
            " */\n"
            "struct Sample { virtual ~Sample(); };\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            index = load_artifact_rows(progress_file(Path(tmp)))
            proposal = propose_source_trace_rewrite(
                source,
                path="src/sample.cpp",
                inventory=inventory(
                    inventory_row(
                        line=2,
                        legacy="recoil:function:0x401030",
                        description="Compiler lifecycle body.",
                        artifact_id="recoil:function:0x401030",
                        relation="emits",
                    )
                ),
                artifact_index=index,
            )

        self.assertIn("@recoil-artifact emits .text", proposal.proposed_text)
        self.assertEqual(
            "emits",
            proposal.tracker_states[0]["source_traceability"]["source_edges"][0][
                "relation"
            ],
        )

    def test_duplicate_occurrence_can_be_reviewed_as_cleanup_only(self) -> None:
        source = (
            "/**\n"
            " * Reimplements 0x401000: Direct body.\n"
            " * Reimplements 0x401000: Redundant detached registry prose.\n"
            " */\n"
            "int Sample() { return 0; }\n"
        )
        primary = inventory_row(
            line=2,
            legacy="recoil:function:0x401000",
            description="Direct body.",
            artifact_id="recoil:function:0x401000",
        )
        cleanup = inventory_row(
            line=3,
            legacy="recoil:function:0x401000",
            description="Redundant detached registry prose.",
            artifact_id="recoil:function:0x401000",
            state="unresolved",
            reason_code="redundant-detached-registry",
            record_tracker_state=False,
        )
        with tempfile.TemporaryDirectory() as tmp:
            index = load_artifact_rows(progress_file(Path(tmp)))
            proposal = propose_source_trace_rewrite(
                source,
                path="src/sample.cpp",
                inventory=inventory(primary, cleanup),
                artifact_index=index,
            )

        self.assertTrue(proposal.ready)
        self.assertEqual(1, len(proposal.tracker_states))
        self.assertEqual(
            ["redundant-detached-registry"],
            [item.reason_code for item in proposal.debts],
        )
        self.assertNotIn("Reimplements", proposal.proposed_text)
        self.assertNotIn("Redundant detached registry prose.", proposal.proposed_text)

    def test_unreviewed_or_stale_occurrences_fail_closed(self) -> None:
        source = (
            "/** Reimplements 0x401000: Direct body. */\n"
            "int Sample() { return 0; }\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            index = load_artifact_rows(progress_file(Path(tmp)))
            with self.assertRaisesRegex(
                SourceTraceMigrationError, "no exact reviewed"
            ):
                propose_source_trace_rewrite(
                    source,
                    path="src/sample.cpp",
                    inventory=inventory(),
                    artifact_index=index,
                )
            stale = inventory_row(
                line=1,
                legacy="recoil:function:0x401000",
                description="Changed body.",
                artifact_id="recoil:function:0x401000",
            )
            with self.assertRaisesRegex(SourceTraceMigrationError, "snapshot changed"):
                propose_source_trace_rewrite(
                    source,
                    path="src/sample.cpp",
                    inventory=inventory(stale),
                    artifact_index=index,
                )

    def test_inventory_reports_missing_identity_without_fabrication(self) -> None:
        source = (
            "/** Reimplements 0x430230: Missing function. */\n"
            "int Sample() { return 0; }\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            index = load_artifact_rows(progress_file(Path(tmp)))
            report = inventory_legacy_source(
                source,
                path="src/sample.cpp",
                artifact_index=index,
            )

        self.assertEqual(
            ["missing-artifact-identity"],
            [item["reason_code"] for item in report["debts"]],
        )
        self.assertEqual("recoil:function:0x430230", report["occurrences"][0]["legacy_artifact_id"])
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src" / "missing.cpp").write_text(source, encoding="utf-8")
            index = load_artifact_rows(progress_file(root))
            template = build_migration_template(
                [Path("src")],
                repo_root=root,
                artifact_index=index,
            )
        self.assertEqual(
            [
                {
                    "binary": "recoil",
                    "kind_hint": "function",
                    "address": "0x430230",
                    "reason_code": "missing-artifact-identity",
                    "source_path": "src/missing.cpp",
                }
            ],
            template["unresolved_legacy_claims"],
        )
        self.assertFalse(template["rows"][0]["record_tracker_state"])

    def test_missing_identity_kind_hint_uses_attached_construct_kind(self) -> None:
        source = (
            "/** Reimplements 0x41b8ac: Legacy syntax omitted the data keyword. */\n"
            "const unsigned char MissionClassTable[32] = { 0 };\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src" / "mission.cpp").write_text(source, encoding="utf-8")
            template = build_migration_template(
                [Path("src/mission.cpp")],
                repo_root=root,
                artifact_index=load_artifact_rows(progress_file(root)),
            )

        self.assertEqual(
            "recoil:function:0x41b8ac",
            template["rows"][0]["expected_legacy_artifact_id"],
        )
        self.assertEqual(
            "data",
            template["occurrence_facts"][0]["construct_kind"],
        )
        self.assertEqual(
            "data",
            template["unresolved_legacy_claims"][0]["kind_hint"],
        )

    def test_embedded_legacy_prose_never_auto_resolves(self) -> None:
        source = (
            "/**\n"
            " * Provenance: Reimplements 0x401000: Direct body.\n"
            " */\n"
            "int Sample() { return 0; }\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            source_path = root / "src" / "sample.cpp"
            source_path.write_text(source, encoding="utf-8")
            index = load_artifact_rows(progress_file(root))
            template = build_migration_template(
                [Path("src/sample.cpp")],
                repo_root=root,
                artifact_index=index,
            )
            reviewed = review_conservative_template(
                template,
                current_template=template,
                canonical_artifact_ids=set(),
                parent_reviewed=True,
            )
            reviewed_inventory = load_migration_inventory(
                payload_json=json.dumps(reviewed["inventory"])
            )
            rewrite = propose_source_trace_rewrite(
                source,
                path="src/sample.cpp",
                inventory=reviewed_inventory,
                artifact_index=index,
            )
            proposal = propose_source_trace_batch(
                reviewed_inventory,
                repo_root=root,
                artifact_index=index,
                apply=False,
            )

        self.assertFalse(template["rows"][0]["reviewed"])
        self.assertFalse(
            template["occurrence_facts"][0]["standalone_semantic_line"]
        )
        row = reviewed["inventory"]["rows"][0]
        self.assertEqual("unresolved", row["state"])
        self.assertEqual("embedded-legacy-prose", row["reason_code"])
        self.assertIsNone(row["anchor_id"])
        self.assertNotIn("@recoil-artifact", rewrite.proposed_text)
        self.assertIn(
            "Provenance: Direct body.",
            rewrite.proposed_text,
        )
        self.assertEqual("unresolved", proposal["tracker_states"][0]["source_traceability"]["state"])

    def test_messages_namespace_is_preserved_by_line_rewrite_validation(self) -> None:
        source = (
            "/** Reimplements 0x10001010: Message lookup. */\n"
            "int Lookup() { return 0; }\n"
        )
        row = inventory_row(
            line=1,
            legacy="messages:function:0x10001010",
            description="Message lookup.",
            artifact_id=None,
            state="unresolved",
            reason_code="missing-artifact-identity",
            record_tracker_state=False,
        )
        row["path"] = "src/Messages/messages.c"
        with tempfile.TemporaryDirectory() as tmp:
            proposal = propose_source_trace_rewrite(
                source,
                path="src/Messages/messages.c",
                inventory=inventory(row),
                artifact_index=load_artifact_rows(progress_file(Path(tmp))),
            )

        self.assertTrue(proposal.ready)
        self.assertNotIn("Reimplements 0x10001010", proposal.proposed_text)
        self.assertIn("Message lookup.", proposal.proposed_text)

    def test_resolved_vacuous_title_still_becomes_canonical_artifact_description(self) -> None:
        source = (
            "/** Reimplements 0x401000: SampleFunction. */\n"
            "int SampleFunction() { return 0; }\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            proposal = propose_source_trace_rewrite(
                source,
                path="src/sample.cpp",
                inventory=inventory(
                    inventory_row(
                        line=1,
                        legacy="recoil:function:0x401000",
                        description="SampleFunction.",
                        artifact_id="recoil:function:0x401000",
                    )
                ),
                artifact_index=load_artifact_rows(progress_file(Path(tmp))),
            )

        self.assertIn(
            "@recoil-artifact defines .text recoil:function:0x401000: SampleFunction.",
            proposal.proposed_text,
        )

    def test_unresolved_provider_path_symbol_and_placeholder_rows_are_deleted(self) -> None:
        source = (
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Reimplements 0x401020: src/Battlesport/sample.cpp\n"
            " * Reimplements 0x401030: lifecycle contribution\n"
            " * Reimplements 0x430230: Explains why the unresolved edge remains pending.\n"
            " */\n"
            "int Sample() { return 0; }\n"
        )
        rows = (
            inventory_row(
                line=2,
                legacy="recoil:function:0x401000",
                description="SampleFunction.",
                artifact_id="recoil:function:0x401000",
                state="unresolved",
                reason_code="source-topology-unresolved",
            ),
            inventory_row(
                line=3,
                legacy="recoil:function:0x401020",
                description="src/Battlesport/sample.cpp",
                artifact_id="recoil:function:0x401020",
                state="not-applicable",
                reason_code="provider-boundary",
            ),
            inventory_row(
                line=4,
                legacy="recoil:function:0x401030",
                description="lifecycle contribution",
                artifact_id="recoil:function:0x401030",
                state="unresolved",
                reason_code="lifecycle-relation-unresolved",
            ),
            inventory_row(
                line=5,
                legacy="recoil:function:0x430230",
                description="Explains why the unresolved edge remains pending.",
                artifact_id=None,
                state="unresolved",
                reason_code="missing-artifact-identity",
                record_tracker_state=False,
            ),
        )
        with tempfile.TemporaryDirectory() as tmp:
            proposal = propose_source_trace_rewrite(
                source,
                path="src/sample.cpp",
                inventory=inventory(*rows),
                artifact_index=load_artifact_rows(progress_file(Path(tmp))),
            )

        self.assertNotIn("SampleFunction.", proposal.proposed_text)
        self.assertNotIn("src/Battlesport/sample.cpp", proposal.proposed_text)
        self.assertNotIn("lifecycle contribution", proposal.proposed_text)
        self.assertIn(
            "Explains why the unresolved edge remains pending.",
            proposal.proposed_text,
        )

    def test_reviewed_legacy_migration_still_deletes_construct_only_descriptions(self) -> None:
        descriptions = (
            "Object3d::ApplyTransform.",
            "Object3d::ApplyTransform",
            "Object3d::Object3d.",
            "Object3d::~Object3d",
            "Object3d::operator=.",
            "ApplyTransform.",
            "ApplyTransform",
            "g_TransformRegistry.",
        )
        with tempfile.TemporaryDirectory() as tmp:
            index = load_artifact_rows(progress_file(Path(tmp)))
            for description in descriptions:
                with self.subTest(description=description):
                    source = (
                        f"/** Reimplements 0x401000: {description} */\n"
                        "int Sample() { return 0; }\n"
                    )
                    proposal = propose_source_trace_rewrite(
                        source,
                        path="src/sample.cpp",
                        inventory=inventory(
                            inventory_row(
                                line=1,
                                legacy="recoil:function:0x401000",
                                description=description,
                                artifact_id="recoil:function:0x401000",
                                state="unresolved",
                                reason_code="source-topology-unresolved",
                            )
                        ),
                        artifact_index=index,
                    )
                    self.assertNotIn("Reimplements", proposal.proposed_text)
                    self.assertNotIn(description, proposal.proposed_text)
                    self.assertFalse(proposal.proposed_text.startswith("/**"))

    def test_all_redundant_occurrence_reasons_delete_standalone_rows(self) -> None:
        reasons = (
            "redundant-legacy-after-canonical-source",
            "redundant-legacy-occurrence",
            "redundant-detached-registry",
        )
        with tempfile.TemporaryDirectory() as tmp:
            index = load_artifact_rows(progress_file(Path(tmp)))
            for reason in reasons:
                with self.subTest(reason=reason):
                    source = (
                        "/** Reimplements 0x401000: This prose is occurrence-only. */\n"
                        "int Sample() { return 0; }\n"
                    )
                    proposal = propose_source_trace_rewrite(
                        source,
                        path="src/sample.cpp",
                        inventory=inventory(
                            inventory_row(
                                line=1,
                                legacy="recoil:function:0x401000",
                                description="This prose is occurrence-only.",
                                artifact_id="recoil:function:0x401000",
                                state="unresolved",
                                reason_code=reason,
                                record_tracker_state=False,
                            )
                        ),
                        artifact_index=index,
                    )
                    self.assertNotIn("occurrence-only", proposal.proposed_text)
                    self.assertFalse(proposal.proposed_text.startswith("/**"))

    def test_embedded_vacuous_marker_keeps_surrounding_prose(self) -> None:
        source = (
            "/** Evidence note: Reimplements 0x401000: SampleFunction. */\n"
            "int Sample() { return 0; }\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            proposal = propose_source_trace_rewrite(
                source,
                path="src/sample.cpp",
                inventory=inventory(
                    inventory_row(
                        line=1,
                        legacy="recoil:function:0x401000",
                        description="SampleFunction.",
                        artifact_id="recoil:function:0x401000",
                        state="unresolved",
                        reason_code="embedded-legacy-prose",
                    )
                ),
                artifact_index=load_artifact_rows(progress_file(Path(tmp))),
            )

        self.assertIn("Evidence note: SampleFunction.", proposal.proposed_text)
        self.assertNotIn("Reimplements", proposal.proposed_text)

    def test_empty_touched_block_and_separator_cleanup_preserve_cp1252_crlf_tokens(self) -> None:
        source = (
            "int Before; // café\r\n"
            "\r\n"
            "/** Reimplements 0x430230: src/Battlesport/sample.cpp */\r\n"
            "\r\n"
            "int After;\r\n"
        )
        raw = source.encode("cp1252")
        with tempfile.TemporaryDirectory() as tmp:
            proposal = propose_source_trace_rewrite(
                raw,
                path="src/sample.cpp",
                inventory=inventory(
                    inventory_row(
                        line=3,
                        legacy="recoil:function:0x430230",
                        description="src/Battlesport/sample.cpp",
                        artifact_id=None,
                        state="unresolved",
                        reason_code="missing-artifact-identity",
                        record_tracker_state=False,
                    )
                ),
                artifact_index=load_artifact_rows(progress_file(Path(tmp))),
            )

        self.assertEqual("cp1252", proposal.encoding)
        self.assertEqual("\r\n", proposal.newline)
        self.assertEqual(
            "int Before; // café\r\n\r\nint After;\r\n",
            proposal.proposed_text,
        )
        self.assertEqual(
            non_comment_token_stream(source),
            non_comment_token_stream(proposal.proposed_text),
        )
        self.assertEqual(proposal.proposed_text, proposal.proposed_bytes.decode("cp1252"))

    def test_inventory_file_input_and_state_contract(self) -> None:
        row = inventory_row(
            line=2,
            legacy="recoil:function:0x401000",
            description="Direct body.",
            artifact_id="recoil:function:0x401000",
        )
        payload = {"schema": INVENTORY_SCHEMA, "rows": [row]}
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "inventory.json"
            path.write_text(json.dumps(payload), encoding="utf-8")
            parsed = load_migration_inventory(payload_file=path)
            review_path = Path(tmp) / "review.json"
            review_path.write_text(
                json.dumps(
                    {
                        "kind": "source-trace-conservative-review",
                        "inventory": payload,
                    }
                ),
                encoding="utf-8",
            )
            reviewed = load_migration_inventory(payload_file=review_path)
        self.assertEqual(1, len(parsed.rows))
        self.assertEqual(parsed, reviewed)
        with self.assertRaisesRegex(SourceTraceMigrationError, "exactly one"):
            load_migration_inventory(payload_json=json.dumps(payload), payload_file=path)
        bad = dict(row)
        bad["reason_code"] = "should-not-exist"
        with self.assertRaisesRegex(SourceTraceMigrationError, "must be null"):
            inventory(bad)

    def test_equivalence_preserves_literal_and_preprocessor_tokens(self) -> None:
        before = '#define VALUE "/* literal */"\nint value = 1; /* old */\n'
        after = '#define VALUE "/* literal */"\nint value = 1; /* new */\n'
        verify_comment_only_equivalence(before, after)
        with self.assertRaisesRegex(SourceTraceMigrationError, "equivalence failed"):
            verify_comment_only_equivalence(before, after.replace('"/* literal */"', '"changed"'))
        decoded = decode_source_bytes(
            b"\xef\xbb\xbf// comment\r\nint value;\r\n",
            path="src/sample.cpp",
        )
        self.assertTrue(decoded.bom)
        self.assertEqual("\r\n", decoded.newline)

    def test_template_and_batch_apply_scale_without_inline_payloads(self) -> None:
        source = (
            "/**\n"
            " * Reimplements 0x401000: Direct body.\n"
            " */\n"
            "int Sample() { return 0; }\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            source_path = root / "src" / "sample.cpp"
            source_path.write_text(source, encoding="utf-8")
            progress = progress_file(root)
            index = load_artifact_rows(progress)
            template = build_migration_template(
                [Path("src")],
                repo_root=root,
                artifact_index=index,
            )
            self.assertTrue(template["reviewed"])
            self.assertEqual(1, len(template["rows"]))
            self.assertTrue(template["rows"][0]["reviewed"])
            self.assertTrue(template["rows"][0]["record_tracker_state"])
            self.assertEqual("resolved", template["rows"][0]["state"])
            self.assertIsNone(template["rows"][0]["reason_code"])
            self.assertEqual(
                "recoil:anchor:src-sample-function-sample",
                template["rows"][0]["anchor_id"],
            )
            self.assertEqual(
                "auto-resolved-unique-attached-direct",
                template["occurrence_facts"][0]["classification"],
            )

            reviewed = inventory(
                inventory_row(
                    line=2,
                    legacy="recoil:function:0x401000",
                    description="Direct body.",
                    artifact_id="recoil:function:0x401000",
                )
            )
            dry = propose_source_trace_batch(
                reviewed,
                repo_root=root,
                artifact_index=index,
                apply=False,
            )
            self.assertFalse(dry["applied"])
            self.assertEqual(source, source_path.read_text(encoding="utf-8"))
            applied = propose_source_trace_batch(
                reviewed,
                repo_root=root,
                artifact_index=index,
                apply=True,
            )
            self.assertTrue(applied["applied"])
            migrated = source_path.read_text(encoding="utf-8")
            self.assertNotIn("Reimplements", migrated)
            self.assertIn("@recoil-artifact defines .text", migrated)

    def test_template_never_auto_resolves_multi_row_blocks_and_disambiguates_anchors(self) -> None:
        multi = (
            "/**\n"
            " * Reimplements 0x401000: First grouped claim.\n"
            " * Reimplements 0x401020: Second grouped claim.\n"
            " */\n"
            "int Grouped() { return 0; }\n"
        )
        overloads = (
            "/** Reimplements 0x401000: First overload. */\n"
            "int Sample() { return 0; }\n"
            "/** Reimplements 0x401020: Second overload. */\n"
            "int Sample(int value) { return value; }\n"
        )
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src" / "multi.cpp").write_text(multi, encoding="utf-8")
            index = load_artifact_rows(progress_file(root))
            grouped = build_migration_template(
                [Path("src/multi.cpp")],
                repo_root=root,
                artifact_index=index,
            )
            self.assertTrue(
                all(not row["reviewed"] for row in grouped["rows"])
            )
            self.assertEqual(
                [2, 2],
                [
                    fact["comment_legacy_row_count"]
                    for fact in grouped["occurrence_facts"]
                ],
            )
            self.assertEqual(2, len(grouped["debts"]))

            (root / "src" / "overloads.cpp").write_text(
                overloads, encoding="utf-8"
            )
            overloaded = build_migration_template(
                [Path("src/overloads.cpp")],
                repo_root=root,
                artifact_index=index,
            )
        anchors = [row["anchor_id"] for row in overloaded["rows"]]
        self.assertEqual(2, len(set(anchors)))
        self.assertEqual(
            "recoil:anchor:src-overloads-function-sample",
            anchors[0],
        )
        self.assertTrue(anchors[1].endswith("-0x401020"))
        self.assertTrue(all("recoil-function" not in anchor for anchor in anchors))
        reviewed = review_conservative_template(
            grouped,
            current_template=grouped,
            canonical_artifact_ids=set(),
            parent_reviewed=True,
        )
        rows = reviewed["inventory"]["rows"]
        self.assertTrue(rows[0]["record_tracker_state"])
        self.assertEqual("unresolved", rows[0]["state"])
        self.assertEqual("multiple-legacy-occurrences", rows[0]["reason_code"])
        self.assertTrue(rows[1]["record_tracker_state"])
        self.assertEqual("multiple-legacy-occurrences", rows[1]["reason_code"])
        self.assertEqual(2, reviewed["decision_counts"]["unresolved"])
        self.assertEqual(0, reviewed["decision_counts"]["occurrence-only"])

        canonical_cleanup = review_conservative_template(
            grouped,
            current_template=grouped,
            canonical_artifact_ids={
                "recoil:function:0x401000",
                "recoil:function:0x401020",
            },
            parent_reviewed=True,
        )
        self.assertTrue(
            all(
                not row["record_tracker_state"]
                for row in canonical_cleanup["inventory"]["rows"]
            )
        )
        with self.assertRaisesRegex(
            SourceTraceMigrationError, "parent_reviewed"
        ):
            review_conservative_template(
                grouped,
                current_template=grouped,
                canonical_artifact_ids=set(),
                parent_reviewed=False,
            )

    def test_direct_command_bootstrap_exposes_batch_help(self) -> None:
        result = subprocess.run(
            [
                sys.executable,
                "tools/_recoil/commands/source_trace_migrate.py",
                "batch-propose",
                "--help",
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(0, result.returncode, result.stderr)
        self.assertIn("--payload-file", result.stdout)
        self.assertIn("--apply", result.stdout)
        tracker = subprocess.run(
            [
                sys.executable,
                "tools/_recoil/commands/source_trace_migrate.py",
                "tracker-payload",
                "--help",
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            check=False,
        )
        self.assertEqual(0, tracker.returncode, tracker.stderr)
        self.assertIn("--header-overrides", tracker.stdout)
        self.assertIn("--output", tracker.stdout)

    def test_tracker_payload_joins_canonical_migration_headers_and_missing_claims(self) -> None:
        canonical = (
            "/**\n"
            " * @recoil-anchor recoil:anchor:sample-direct\n"
            " * @recoil-artifact defines .text recoil:function:0x401000: Direct body.\n"
            " */\n"
            "int Sample() { return 0; }\n"
        )
        header = (
            "/**\n"
            " * @recoil-anchor recoil:anchor:header-first\n"
            " * @recoil-artifact defines .text recoil:function:0x401020: First header body.\n"
            " */\n"
            "inline int HeaderFirst() { return 0; }\n"
            "/**\n"
            " * @recoil-anchor recoil:anchor:header-second\n"
            " * @recoil-artifact defines .text recoil:function:0x401030: Second header body.\n"
            " */\n"
            "inline int HeaderSecond() { return 0; }\n"
        )
        claim = {
            "binary": "recoil",
            "kind_hint": "function",
            "address": "0x430230",
            "reason_code": "missing-artifact-identity",
            "source_path": "src/missing.cpp",
        }
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src" / "sample.cpp").write_text(canonical, encoding="utf-8")
            (root / "src" / "other.cpp").write_text(
                "int OtherTranslationUnit;\n", encoding="utf-8"
            )
            (root / "src" / "sample.h").write_text(header, encoding="utf-8")
            index = load_artifact_rows(progress_file(root))
            with self.assertRaisesRegex(
                SourceTraceMigrationError, "header.*requires"
            ):
                build_tracker_replace_payload(
                    [Path("src")],
                    repo_root=root,
                    artifact_index=index,
                )
            payload = build_tracker_replace_payload(
                [Path("src")],
                repo_root=root,
                artifact_index=index,
                header_translation_units={
                    "src/sample.h": {
                        "recoil:function:0x401020": "src/sample.cpp",
                        "recoil:function:0x401030": "src/other.cpp",
                    },
                },
                unresolved_legacy_claims=[claim],
            )
            with self.assertRaisesRegex(
                SourceTraceMigrationError, "stale header artifact override"
            ):
                build_tracker_replace_payload(
                    [Path("src")],
                    repo_root=root,
                    artifact_index=index,
                    header_translation_units={
                        "src/sample.h": {
                            "recoil:function:0x401020": "src/sample.cpp",
                            "recoil:function:0x401030": "src/other.cpp",
                            "recoil:function:0x401000": "src/sample.cpp",
                        },
                    },
                )
            with self.assertRaisesRegex(
                SourceTraceMigrationError, "duplicate tracker-state decision"
            ):
                build_tracker_replace_payload(
                    [Path("src/sample.cpp")],
                    repo_root=root,
                    artifact_index=index,
                    migration_tracker_states=[
                        {
                            "artifact_id": "recoil:function:0x401000",
                            "source_traceability": {
                                "state": "unresolved",
                                "source_edges": [],
                                "reason_code": "source-topology-unresolved",
                            },
                        }
                    ],
                )

        self.assertTrue(payload["parent_reviewed"])
        self.assertEqual("replace-batch", payload["operation"])
        self.assertEqual(3, len(payload["updates"]))
        self.assertTrue(
            all(item["expected_current"] is None for item in payload["updates"])
        )
        header_update = next(
            item
            for item in payload["updates"]
            if item["artifact_id"] == "recoil:function:0x401020"
        )
        self.assertEqual(
            "src/sample.cpp",
            header_update["source_traceability"]["source_edges"][0][
                "emission_context"
            ]["translation_unit"],
        )
        second_header_update = next(
            item
            for item in payload["updates"]
            if item["artifact_id"] == "recoil:function:0x401030"
        )
        self.assertEqual(
            "src/other.cpp",
            second_header_update["source_traceability"]["source_edges"][0][
                "emission_context"
            ]["translation_unit"],
        )
        self.assertEqual([claim], payload["unresolved_legacy_claims"])


if __name__ == "__main__":
    unittest.main()
