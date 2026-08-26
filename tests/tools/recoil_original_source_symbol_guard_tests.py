from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "original_source_symbol_guard.py"

from _recoil.commands.original_source_symbol_guard import resolve_owners_argument  # noqa: E402
from tests.tools.owner_fixture import owner_record, write_ledger  # noqa: E402


def write_owners(path: Path, *, tier: str = "C", data: str = "❌") -> None:
    write_ledger(
        path,
        owner_record(
            "test.group",
            kind="source-file",
            anchors=("0x401000",),
            functions=("0x401000",),
            tiers={"0x401000": tier},
            gates={
                "boundary": "accepted",
                "source": "pending",
                "data": "accepted" if data == "✅" else "pending",
                "functional": "accepted" if tier in {"C", "B", "A", "S"} else "pending",
                "linkage": "pending",
                "byte": "accepted" if tier == "S" else "pending",
            },
            address_metadata={
                "0x401000": {
                    "name": "Caller",
                    "source_path": "src/sample.cpp",
                    "target": "caller",
                }
            },
            blocker="source-shape audit pending",
        ),
    )


class OriginalSourceSymbolGuardTests(unittest.TestCase):
    def run_guard(self, source_text: str, *, tier: str = "C", data: str = "❌") -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text(source_text, encoding="utf-8")
            owners = root / "SOURCE_OWNERS.json"
            write_owners(owners, tier=tier, data=data)

            return subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--root",
                    str(source_dir),
                    "--progress",
                    str(owners),
                    "--max",
                    "20",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

    def test_path_alias_matches_root_scan(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source_dir = root / "src"
            source_dir.mkdir()
            (source_dir / "sample.cpp").write_text(
                "int MakeSampleFTable() {\n"
                "    return 1;\n"
                "}\n",
                encoding="utf-8",
            )
            owners = root / "SOURCE_OWNERS.json"
            write_owners(owners)

            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--path",
                    str(source_dir),
                    "--progress",
                    str(owners),
                    "--max",
                    "20",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_relative_owner_argument_resolves_from_repository_root(self) -> None:
        self.assertEqual(
            REPO_ROOT / ".agent" / "SOURCE_OWNERS.json",
            resolve_owners_argument(".agent/SOURCE_OWNERS.json"),
        )

    def test_absolute_owner_argument_is_preserved(self) -> None:
        absolute = REPO_ROOT / ".agent" / "SOURCE_OWNERS.json"
        self.assertEqual(
            absolute,
            resolve_owners_argument(str(absolute)),
        )

    def test_ordinary_unclaimed_helper_definition_does_not_invent_provenance(self) -> None:
        result = self.run_guard(
            "static int MakeSampleFTable() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_tier_c_caller_to_non_original_helper_fails(self) -> None:
        result = self.run_guard(
            "// Reimplements 0x401000: Caller\n"
            "int Caller() {\n"
            "    return MakeSampleFTable();\n"
            "}\n"
            "\n"
            "static int MakeSampleFTable() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("reimplemented caller uses unsupported helper", result.stdout)
        self.assertIn("invalidates affected callers as reimplementations", result.stdout)

    def test_source_equivalent_global_initializer_to_unsupported_helper_fails(self) -> None:
        result = self.run_guard(
            "static int MakeSampleFTable() {\n"
            "    return 1;\n"
            "}\n"
            "\n"
            "int g_SampleTable = MakeSampleFTable();\n",
            tier="B",
            data="✅",
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("Data reimplemented global uses unsupported helper", result.stdout)
        self.assertIn("Data reimplemented cannot be accepted", result.stdout)

    def test_builtin_placement_new_delete_definitions_do_not_poison_callers(self) -> None:
        result = self.run_guard(
            "inline void *operator new(\n"
            "    size_t,\n"
            "    void *place\n"
            ") {\n"
            "    return place;\n"
            "}\n"
            "\n"
            "inline void operator delete(\n"
            "    void *,\n"
            "    void *\n"
            ") {}\n"
            "\n"
            "// Reimplements 0x401000: Caller\n"
            "int Caller() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_recognized_mfc_generation_macros_are_not_helper_definitions(self) -> None:
        result = self.run_guard(
            "IMPLEMENT_DYNCREATE(SampleFrame, BaseFrame)\n"
            "BEGIN_MESSAGE_MAP(SampleFrame, BaseFrame)\n"
            "END_MESSAGE_MAP()\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_canonical_defines_directive_proves_attached_helper(self) -> None:
        result = self.run_guard(
            "/**\n"
            " * @recoil-anchor recoil:anchor:helper\n"
            " * @recoil-artifact defines .text recoil:function:0x401020: Helper.\n"
            " */\n"
            "static int Helper() { return 1; }\n"
            "// Reimplements 0x401000: Caller\n"
            "int Caller() { return Helper(); }\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_pointer_return_function_body_prevents_local_global_initializer_noise(self) -> None:
        result = self.run_guard(
            "static int MakeSampleFTable() {\n"
            "    return 1;\n"
            "}\n"
            "\n"
            "// Reimplements 0x401000: Caller\n"
            "Sample *Caller() {\n"
            "    Sample *const sample = (Sample *)MakeSampleFTable();\n"
            "    return sample;\n"
            "}\n",
            tier="C",
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("unproven production helper definition", result.stdout)
        self.assertNotIn("global initializer calls unsupported helper", result.stdout)

    def test_unclaimed_member_base_name_does_not_match_unqualified_call(self) -> None:
        result = self.run_guard(
            "void Widget::SetVisible(\n"
            "    int visible\n"
            ") {\n"
            "    (void)visible;\n"
            "}\n"
            "\n"
            "// Reimplements 0x401000: Caller\n"
            "int Caller() {\n"
            "    SetVisible(1);\n"
            "    return 0;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("reimplemented caller uses unsupported helper", result.stdout)

    def test_bare_observed_address_does_not_classify_ordinary_function_as_helper(self) -> None:
        result = self.run_guard(
            "/* Observed in caller 0x401000. */\n"
            "int NormalizeCounter(int value) {\n"
            "    return value < 0 ? 0 : value;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_structural_original_inline_helper_evidence_proves_definition(self) -> None:
        result = self.run_guard(
            "// Reimplements 0x401000: Caller\n"
            "int Caller() {\n"
            "    return NormalizeCounter(-1);\n"
            "}\n"
            "\n"
            "/* Restores likely original inline helper; no standalone retail function exists.\n"
            " * Observed in callers 0x401000 and 0x401020 from a repeated clamp idiom. */\n"
            "static int NormalizeCounter(int value) {\n"
            "    return value < 0 ? 0 : value;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_real_original_inline_constructor_evidence_form_passes(self) -> None:
        result = self.run_guard(
            "// Reimplements 0x401000: Caller\n"
            "int Caller() {\n"
            "    return BriefingActionHideElement(0);\n"
            "}\n"
            "\n"
            "/**\n"
            " * Original inline constructor; no standalone retail function exists.\n"
            " * Observed in caller 0x4045b0 through the repeated action setup sequence.\n"
            " * Purpose: Constructs the hide-element action used by briefing callers.\n"
            " */\n"
            "static int BriefingActionHideElement(void *element) {\n"
            "    return element != 0;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_live_original_function_and_helper_evidence_forms_pass(self) -> None:
        result = self.run_guard(
            "// Reimplements 0x401000: Caller\n"
            "int Caller() {\n"
            "    return OriginalBody() + GetBaseMessageMapForMfc();\n"
            "}\n"
            "/**\n"
            " * Original function evidence: retail 0x403620 contains this body.\n"
            " * Purpose: Represents the address-backed original body.\n"
            " */\n"
            "static int OriginalBody() { return 1; }\n"
            "/**\n"
            " * Original helper evidence: no standalone retail function; used by\n"
            " * recovered MFC message-map data at 0x4ccb18.\n"
            " * Purpose: Returns the recovered base message map.\n"
            " */\n"
            "static int GetBaseMessageMapForMfc() { return 1; }\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_reimplements_marker_beyond_eight_contiguous_comment_lines_passes(self) -> None:
        result = self.run_guard(
            "// Reimplements 0x401000: Caller\n"
            "// Retail ABI evidence.\n"
            "// Retail data evidence.\n"
            "// Retail call evidence.\n"
            "// Retail layout evidence.\n"
            "// Retail ownership evidence.\n"
            "// Retail boundary evidence.\n"
            "// Retail linkage evidence.\n"
            "// Retail behavior evidence.\n"
            "// Purpose: exercise a long contiguous provenance block.\n"
            "int Caller() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_long_structural_inline_helper_claim_is_proof(self) -> None:
        result = self.run_guard(
            "/* Original inline helper; no standalone retail function exists.\n"
            " * Retail ABI evidence.\n"
            " * Retail data evidence.\n"
            " * Retail call evidence.\n"
            " * Retail layout evidence.\n"
            " * Retail ownership evidence.\n"
            " * Retail boundary evidence.\n"
            " * Retail linkage evidence.\n"
            " * Retail behavior evidence.\n"
            " * Purpose: exercise a long contiguous provenance block. */\n"
            "static int NormalizeCounter(int value) {\n"
            "    return value < 0 ? 0 : value;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_long_comment_without_a_source_claim_does_not_force_provenance(self) -> None:
        result = self.run_guard(
            "// Retail ABI observation.\n"
            "// Retail data observation.\n"
            "// Retail call observation.\n"
            "// Retail layout observation.\n"
            "// Retail ownership observation.\n"
            "// Retail boundary observation.\n"
            "// Retail linkage observation.\n"
            "// Retail behavior observation.\n"
            "// Additional observation.\n"
            "// Purpose: exercise a long non-provenance block.\n"
            "int UnsupportedHelper() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_blank_line_prevents_borrowing_older_provenance(self) -> None:
        result = self.run_guard(
            "// Reimplements 0x401000: UnsupportedHelper\n"
            "\n"
            "// Purpose: this adjacent comment has no provenance.\n"
            "int UnsupportedHelper() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_non_comment_line_prevents_borrowing_older_provenance(self) -> None:
        result = self.run_guard(
            "// Reimplements 0x401000: UnsupportedHelper\n"
            "typedef int SampleValue;\n"
            "// Purpose: this adjacent comment has no provenance.\n"
            "int UnsupportedHelper() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)

    def test_address_backed_and_provider_boundary_functions_pass(self) -> None:
        result = self.run_guard(
            "// Reimplements 0x401000: Caller\n"
            "int Caller() {\n"
            "    return 1;\n"
            "}\n"
            "\n"
            "// Imported runtime provider boundary; this does not reimplement runtime behavior.\n"
            "int RuntimeProviderShim() {\n"
            "    return 0;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("violations: 0", result.stdout)


if __name__ == "__main__":
    unittest.main()

