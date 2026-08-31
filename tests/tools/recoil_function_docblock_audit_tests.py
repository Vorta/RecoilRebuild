from __future__ import annotations

import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "function_docblock_audit.py"

sys.path.insert(0, str(REPO_ROOT / "tools"))

from tests.tools.owner_fixture import ledger_payload, owner_record  # noqa: E402


def data_owner_text(*entries: tuple[str, str]) -> str:
    first_address = entries[0][0]
    owner = owner_record(
        "data.group",
        kind="data-owner",
        anchors=(first_address,),
        data=entries,
        tiers={address: "X" for address, _name in entries},
        gates={
            "boundary": "pending",
            "source": "pending",
            "data": "pending",
            "functional": "none",
            "linkage": "pending",
            "byte": "pending",
        },
        source_paths=("src/Data.cpp",),
        address_metadata={
            address: {
                "name": name,
                "source_path": "src/Data.cpp",
                "section": ".data",
                "size": "pending",
                "type": "pending",
            }
            for address, name in entries
        },
    )
    return json.dumps(ledger_payload(owner))


def load_audit_module():
    spec = importlib.util.spec_from_file_location("function_docblock_audit_under_test", SCRIPT)
    assert spec is not None
    assert spec.loader is not None
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class RecoilFunctionDocblockAuditTests(unittest.TestCase):
    def run_audit(self, source_text: str, owners_text: str | None = None) -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "sample.cpp"
            source.write_text(source_text, encoding="utf-8")
            owners = root / "SOURCE_OWNERS.json"
            if owners_text is not None:
                owners.write_text(owners_text, encoding="utf-8")
            return subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--path",
                    str(source),
                    "--progress",
                    str(owners),
                    "--summary",
                    "--max",
                    "20",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

    def test_address_backed_docblock_passes(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Purpose: Returns the selected sample value for the caller.\n"
            " */\n"
            "int SampleFunction() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_old_line_comment_reimplements_is_reported(self) -> None:
        result = self.run_audit(
            "// Reimplements 0x401000: SampleFunction\n"
            "int SampleFunction() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("missing function docblock", result.stdout)
        self.assertIn("SampleFunction", result.stdout)

    def test_line_comment_above_docblock_is_reported(self) -> None:
        result = self.run_audit(
            "// Source-faithful helper recovered from address-backed callers in this source file.\n"
            "/**\n"
            " * Original-source helper; no standalone retail function exists.\n"
            " * Purpose: Normalizes the local counter before each caller stores it.\n"
            " */\n"
            "static int NormalizeCounter(int value) {\n"
            "    return value < 0 ? 0 : value;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("line comment should be unified with docblock", result.stdout)

    def test_provenance_block_above_docblock_is_reported(self) -> None:
        result = self.run_audit(
            "/* ==================================================================\n"
            " * SampleFunction -- 0x401000\n"
            " * Returns the selected sample value for the caller.\n"
            " * ================================================================== */\n"
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Purpose: Returns the selected sample value for the caller.\n"
            " */\n"
            "int SampleFunction() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("block comment should be unified with docblock", result.stdout)

    def test_non_provenance_block_above_docblock_is_allowed(self) -> None:
        result = self.run_audit(
            "/* Local section heading kept separate from function provenance. */\n"
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Purpose: Returns the selected sample value for the caller.\n"
            " */\n"
            "int SampleFunction() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_original_source_path_inside_docblock_passes(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Original source path: D:\\Proj\\Battlesport\\sample.cpp.\n"
            " * Purpose: Returns the selected sample value for the caller.\n"
            " */\n"
            "int SampleFunction() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("redundant source-path prose", result.stdout)

    def test_line_comment_before_class_declaration_is_reported(self) -> None:
        result = self.run_audit(
            "// Provider-boundary accessor for imported MFC42 CDialog metadata.\n"
            "class DialogAccessor : public CDialog {\n"
            "};\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("line comment should be declaration docblock", result.stdout)

    def test_line_comment_before_function_declaration_is_reported(self) -> None:
        result = self.run_audit(
            "// Returns the selected sample value for the caller.\n"
            "int SampleFunction();\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("line comment should be declaration docblock", result.stdout)

    def test_docblocks_before_declarations_pass(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Provider-boundary accessor for imported MFC42 CDialog metadata.\n"
            " */\n"
            "class DialogAccessor : public CDialog {\n"
            "};\n"
            "/**\n"
            " * Returns the selected sample value for the caller.\n"
            " */\n"
            "int SampleFunction();\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_line_comment_inside_function_body_is_not_declaration_docblock(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Purpose: Returns the selected sample value for the caller.\n"
            " */\n"
            "int SampleFunction() {\n"
            "    // Loop counter for local work.\n"
            "    int index = 0;\n"
            "    return index;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_inline_helper_docblock_with_callers_passes(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Restores likely original inline helper; no standalone retail function exists.\n"
            " * Observed in callers 0x401000 and 0x401020 from a repeated clamp idiom.\n"
            " * Purpose: Normalizes the local counter before each caller stores it.\n"
            " */\n"
            "static int NormalizeCounter(int value) {\n"
            "    return value < 0 ? 0 : value;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_bare_observed_in_docblock_does_not_assert_source_claim(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Observed in caller 0x401000.\n"
            " * Purpose: Normalizes the local counter before the caller stores it.\n"
            " */\n"
            "static int NormalizeCounter(int value) {\n"
            "    return value < 0 ? 0 : value;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_provider_boundary_docblock_passes(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Imported runtime provider-boundary ABI shim.\n"
            " * Purpose: Exposes the provider cleanup slot used by authored callers.\n"
            " */\n"
            "void RuntimeProviderCleanup() {\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_missing_purpose_is_reported(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " */\n"
            "int SampleFunction() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("docblock missing purpose", result.stdout)

    def test_empty_purpose_is_reported(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Purpose:\n"
            " */\n"
            "int SampleFunction() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("docblock missing purpose", result.stdout)

    def test_qualified_call_in_wrapped_condition_is_not_function(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Purpose: Checks whether the current variant can run this path.\n"
            " */\n"
            "int SampleFunction(int flag, int type) {\n"
            "    if (flag != 0 &&\n"
            "        VariantTag::CurrentAllowsId(type) != 0) {\n"
            "        return 1;\n"
            "    }\n"
            "    return 0;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("VariantTag::CurrentAllowsId", result.stdout)

    def test_ordinary_pointer_return_definition_does_not_force_docblock(self) -> None:
        result = self.run_audit(
            "const SampleType *FindSample() {\n"
            "    return 0;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_title_only_docblock_is_permitted_without_forcing_source_claim(self) -> None:
        titled = self.run_audit(
            "/** SampleFunction */\n"
            "int SampleFunction() { return 1; }\n"
        )
        self.assertEqual(titled.returncode, 0, titled.stdout + titled.stderr)
        self.assertIn("findings: 0", titled.stdout)

        deleted = self.run_audit(
            "int SampleFunction() { return 1; }\n"
        )
        self.assertEqual(deleted.returncode, 0, deleted.stdout + deleted.stderr)
        self.assertIn("findings: 0", deleted.stdout)

    def test_real_original_inline_constructor_evidence_form_passes(self) -> None:
        result = self.run_audit(
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
        self.assertIn("findings: 0", result.stdout)

    def test_live_original_function_evidence_retail_address_form_passes(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Original function evidence: retail 0x403620 contains the approved local region.\n"
            " * Purpose: Builds the normalized path-probe basis for one segment.\n"
            " */\n"
            "void InitFromSegment() {}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_live_original_helper_no_standalone_form_passes(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Original helper evidence: no standalone retail function; used by the\n"
            " * recovered MFC message-map data at 0x4ccb18.\n"
            " * Purpose: Returns the provider base message map.\n"
            " */\n"
            "const void *GetBaseMessageMapForMfc() { return 0; }\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_original_static_helper_named_caller_cluster_supplies_provenance(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Original static helper observed in Object3D transform-mutator callers\n"
            " * Purpose: Marks transform state dirty and queues the touched node.\n"
            " */\n"
            "void QueueTransformUpdate() {}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_vague_original_static_helper_caller_claim_is_not_provenance(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Original static helper observed in callers.\n"
            " * Purpose: Marks transform state dirty and queues the touched node.\n"
            " */\n"
            "void QueueTransformUpdate() {}\n"
        )

        self.assertEqual(result.returncode, 1)
        self.assertIn("docblock missing provenance", result.stdout)

    def test_owner_tracked_data_docblock_passes_without_legacy_address(self) -> None:
        owners_text = data_owner_text(("0x4f0cc0", "g_HudSensorTracker"))
        result = self.run_audit(
            "/**\n"
            " * Purpose: Stores HUD sensor tracker state for the frame loop.\n"
            " */\n"
            "HudSensorTracker g_HudSensorTracker;\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_grouped_owner_tracked_data_docblock_covers_adjacent_declarations(self) -> None:
        owners_text = data_owner_text(
            ("0x500000", "g_FirstOwnerGlobal"),
            ("0x500004", "g_SecondOwnerGlobal"),
        )
        result = self.run_audit(
            "/**\n"
            " * Reimplements data 0x500000: g_FirstOwnerGlobal.\n"
            " * Reimplements data 0x500004: g_SecondOwnerGlobal.\n"
            " * Purpose: Stores adjacent globals recovered as one initialized-data group.\n"
            " */\n"
            "int g_FirstOwnerGlobal;\n"
            "int g_SecondOwnerGlobal;\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_declspec_function_uses_shared_construct_parser(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Purpose: Exercises a VC5 declaration prefix.\n"
            " */\n"
            "__declspec(noinline) int SampleFunction() { return 1; }\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_exact_construct_name_title_is_permitted_with_governed_docblock(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * SampleFunction\n"
            " * Reimplements 0x401000: Retail body evidence.\n"
            " * Purpose: Returns the selected sample value.\n"
            " */\n"
            "int SampleFunction() { return 1; }\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)
        self.assertNotIn("docblock missing purpose", result.stdout)

    def test_about_macro_block_permits_symbol_title_rows(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * CAboutDlg::GetMessageMap.\n"
            " *\n"
            " * Purpose: returns the authored message-map table.\n"
            " * CWnd::BeginModalState.\n"
            " * CWnd::EndModalState.\n"
            " */\n"
            "BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)\n"
            "END_MESSAGE_MAP()\n"
        )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_all_comment_styles_permit_all_standalone_symbol_title_forms(self) -> None:
        symbol_rows = (
            "Object3d::ApplyTransform.",
            "Object3d::ApplyTransform",
            "Object3d::Object3d.",
            "Object3d::~Object3d",
            "Object3d::operator=.",
            "ApplyTransform",
            "g_TransformRegistry.",
        )
        styles = {
            "doxygen": "/**\n"
            + "".join(f" * {row}\n *\n" for row in symbol_rows)
            + " */\n",
            "ordinary-block": "/*\n"
            + "".join(f" * {row}\n *\n" for row in symbol_rows)
            + " */\n",
            "line-comment-group": "".join(f"// {row}\n//\n" for row in symbol_rows),
        }
        for style, comments in styles.items():
            with self.subTest(style=style):
                result = self.run_audit(comments + "int Unrelated;\n")
                self.assertEqual(
                    0,
                    result.returncode,
                    result.stdout + result.stderr,
                )
                self.assertIn("findings: 0", result.stdout)

    def test_single_line_ordinary_block_labels_without_title_punctuation_pass(self) -> None:
        result = self.run_audit(
            "/* RECOIL_ZVID_DD_ORDER_INSERT */\n"
            "int First;\n"
            "/* pointData */\n"
            "int Second;\n"
            "/* unusedLabel */\n"
            "int Third;\n"
            "/* BATTLESPORT_PICKUP_H */\n"
            "int Fourth;\n"
        )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_permitted_symbol_titles_do_not_hide_paths_or_placeholders(self) -> None:
        result = self.run_audit(
            "/** RECOIL_ZVID_DD_ORDER_INSERT */\n"
            "int First;\n"
            "/** pointData. */\n"
            "int Second;\n"
            "/* BATTLESPORT_PICKUP_H. */\n"
            "int Third;\n"
            "/* src/GameZRecoil/zVideo/zvid_dd.c */\n"
            "int Fourth;\n"
            "/* routed to zvid_init.c. */\n"
            "int Fifth;\n"
        )

        self.assertEqual(1, result.returncode)
        self.assertEqual(
            1,
            result.stdout.count("redundant source-path prose:"),
            result.stdout,
        )
        self.assertEqual(
            1,
            result.stdout.count("redundant migration-placeholder prose:"),
            result.stdout,
        )

    def test_source_path_symbol_path_and_mechanical_rows_are_classified(self) -> None:
        result = self.run_audit(
            "/* src/Battlesport/about.cpp */\n"
            "// CAboutDlg::GetMessageMap - D:\\Retail\\about.cpp\n"
            "// SampleFunction (src/Battlesport/sample.cpp).\n"
            "// routing anchor\n"
            "// routed to about.cpp.\n"
            "// body compiles from about.cpp.\n"
            "// lifecycle contribution\n"
            "int Unrelated;\n"
        )

        self.assertEqual(1, result.returncode)
        self.assertEqual(
            3,
            result.stdout.count("redundant source-path prose:"),
            result.stdout,
        )
        self.assertEqual(
            4,
            result.stdout.count("redundant migration-placeholder prose:"),
            result.stdout,
        )

    def test_unpunctuated_symbol_title_does_not_absorb_following_hygiene_rows(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Object3d::ApplyTransform\n"
            " * src/Battlesport/object3d.cpp\n"
            " */\n"
            "/**\n"
            " * Object3d::ApplyTransform\n"
            " * lifecycle contribution\n"
            " */\n"
            "int Unrelated;\n"
        )

        self.assertEqual(1, result.returncode)
        self.assertEqual(
            1,
            result.stdout.count("redundant source-path prose:"),
            result.stdout,
        )
        self.assertEqual(
            1,
            result.stdout.count("redundant migration-placeholder prose:"),
            result.stdout,
        )

    def test_duplicate_rows_use_duplicate_category_only_without_stronger_match(self) -> None:
        result = self.run_audit(
            "// Explains why the recovered table remains empty.\n"
            "// Explains why the recovered table remains empty.\n"
            "// routed to about.cpp.\n"
            "// routed to about.cpp.\n"
            "int Unrelated;\n"
        )

        self.assertEqual(1, result.returncode)
        self.assertEqual(
            1,
            result.stdout.count("duplicate comment prose:"),
            result.stdout,
        )
        self.assertEqual(
            2,
            result.stdout.count("redundant migration-placeholder prose:"),
            result.stdout,
        )
        self.assertIn(":2: duplicate comment prose:", result.stdout)

    def test_exact_duplicate_symbol_titles_remain_duplicate_prose(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Object3d::ApplyTransform\n"
            " * Object3d::ApplyTransform\n"
            " */\n"
            "int Unrelated;\n"
        )

        self.assertEqual(1, result.returncode)
        self.assertEqual(
            1,
            result.stdout.count("duplicate comment prose:"),
            result.stdout,
        )
        self.assertIn(":3: duplicate comment prose:", result.stdout)

    def test_duplicate_detection_is_scoped_to_one_comment_and_excludes_evidence_rows(self) -> None:
        result = self.run_audit(
            "/** Evidence: recovered from the same retail call pattern. */\n"
            "int First;\n"
            "/** Evidence: recovered from the same retail call pattern. */\n"
            "int Second;\n"
            "/* Explains a shared invariant. */\n"
            "int Third;\n"
            "/* Explains a shared invariant. */\n"
            "int Fourth;\n"
        )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_canonical_substantive_wrapped_and_inline_rows_are_preserved(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * @recoil-anchor recoil:anchor:sample\n"
            " * @recoil-artifact defines .text recoil:function:0x401000: SampleFunction.\n"
            " * Purpose: returns the recovered value after checking\n"
            " * src/Battlesport/sample.cpp evidence from the neighboring caller.\n"
            " * This substantive sentence explains the recovered dispatch.\n"
            " */\n"
            "int value; // SampleFunction\n"
        )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_symbol_shaped_wrapped_evidence_continuations_are_preserved(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * Touched data: constructs the accepted zero-filled global data owner\n"
            " * g_HudSensorTracker.\n"
            " * BN evidence then\n"
            " * HudUiNumericTextInput::Update.\n"
            " * The provider callback is referenced only by\n"
            " * Checkpoint::InstantiateNamedObjects.\n"
            " * State is restored during\n"
            " * Player::InitMissionRuntimeFromWorldAndCamera.\n"
            " * The pass rectangle is consumed by\n"
            " * ApplyPass3.\n"
            " */\n"
            "int Unrelated;\n"
        )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_wrapped_source_placement_sentence_is_not_a_path_title(self) -> None:
        result = self.run_audit(
            "// Moved HUD runtime bodies live in src/Battlesport/hud_runtime_layer_body.h\n"
            "// and are included by src/Battlesport/hud.cpp for physical HUD order.\n"
            "// Source placement note: the implementation moved from\n"
            "// HudUiMessageBoxDialog.cpp.\n"
            "int Unrelated;\n"
        )

        self.assertEqual(0, result.returncode, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_canonical_defines_function_directive_supplies_provenance(self) -> None:
        result = self.run_audit(
            "/**\n"
            " * @recoil-anchor recoil:anchor:sample-function\n"
            " * @recoil-artifact defines .text recoil:function:0x401000: SampleFunction.\n"
            " * Purpose: Returns the selected sample value for the caller.\n"
            " */\n"
            "int SampleFunction() {\n"
            "    return 1;\n"
            "}\n"
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_grouped_owner_tracked_data_docblock_does_not_require_each_legacy_address(self) -> None:
        owners_text = data_owner_text(
            ("0x500000", "g_FirstOwnerGlobal"),
            ("0x500004", "g_SecondOwnerGlobal"),
        )
        result = self.run_audit(
            "/**\n"
            " * Reimplements data 0x500000: g_FirstOwnerGlobal.\n"
            " * Purpose: Stores adjacent globals recovered as one initialized-data group.\n"
            " */\n"
            "int g_FirstOwnerGlobal;\n"
            "int g_SecondOwnerGlobal;\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_duplicate_owner_tracked_data_name_accepts_matching_docblock_address(self) -> None:
        owners_text = data_owner_text(
            ("0x4e071c", "g_zImage_DefaultTextureRecord"),
            ("0x6333a8", "g_zImage_DefaultTextureRecord"),
        )
        result = self.run_audit(
            "/**\n"
            " * Reimplements data 0x4e071c: g_zImage_DefaultTextureRecord.\n"
            " * Purpose: Stores the zImage texture-directory default texture record.\n"
            " */\n"
            "zVideo_TextureRecordPartial *g_zImage_DefaultTextureRecord = 0;\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("0x6333a8", result.stdout)

    def test_owner_tracked_data_without_source_claim_needs_no_docblock(self) -> None:
        owners_text = data_owner_text(("0x4f0cc0", "g_HudSensorTracker"))
        result = self.run_audit("HudSensorTracker g_HudSensorTracker;\n", owners_text)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("missing data docblock", result.stdout)

    def test_owner_tracked_data_local_initializer_is_not_definition(self) -> None:
        owners_text = data_owner_text(("0x57c8c4", "g_Clip_PolyVerts"))
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x47a000: ClipPolyNoUvCore.\n"
            " * Purpose: Exercises a local cursor initialized from owner-tracked data.\n"
            " */\n"
            "int ClipPolyNoUvCore() {\n"
            "    const zClipVert *source = g_Clip_PolyVerts;\n"
            "    return source != 0;\n"
            "}\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("missing data docblock", result.stdout)

    def test_owner_tracked_data_function_body_read_is_not_definition(self) -> None:
        owners_text = data_owner_text(("0x4f0cc0", "g_HudFrameCounter"))
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x401000: SampleFunction.\n"
            " * Purpose: Reads the frame counter for a behavior check.\n"
            " */\n"
            "int SampleFunction() {\n"
            "    int frame = g_HudFrameCounter;\n"
            "    return frame;\n"
            "}\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("missing data docblock", result.stdout)

    def test_owner_tracked_data_return_read_is_not_definition(self) -> None:
        owners_text = data_owner_text(("0x56ab38", "g_zNetwork_ServiceProviderList"))
        result = self.run_audit(
            "/**\n"
            " * Reimplements 0x48a130: RefreshAndGetServiceProviderList.\n"
            " * Purpose: Returns the current service-provider list after refresh.\n"
            " */\n"
            "zNetworkServiceProviderListVec *RefreshAndGetServiceProviderList() {\n"
            "    return g_zNetwork_ServiceProviderList;\n"
            "}\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("missing data docblock", result.stdout)

    def test_owner_tracked_data_member_assignment_is_not_definition(self) -> None:
        owners_text = data_owner_text(("0x4f0cc0", "g_HudSensorTracker"))
        result = self.run_audit(
            "/**\n"
            " * Reimplements data 0x4f0cc0: g_HudSensorTracker.\n"
            " * Purpose: Stores HUD sensor tracker state for the frame loop.\n"
            " */\n"
            "HudSensorTracker g_HudSensorTracker;\n"
            "void TouchTracker() {\n"
            "    g_HudSensorTracker.missionFlags = 1;\n"
            "}\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertNotIn("missing data docblock", result.stdout)
        self.assertNotIn("missing function docblock", result.stdout)

    def test_owner_tracked_data_without_claim_has_no_inventory_findings(self) -> None:
        owners_text = data_owner_text(
            ("0x500000", "g_FirstOwnerGlobal"),
            ("0x500004", "g_SecondOwnerGlobal"),
        )
        result = self.run_audit(
            "int g_SecondOwnerGlobal;\n"
            "int g_FirstOwnerGlobal;\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_scoped_owner_tracked_data_name_matches_unqualified_definition(self) -> None:
        owners_text = data_owner_text(("0x5669d8", "zMath::g_zMath_Vec3Zero"))
        result = self.run_audit(
            "/**\n"
            " * Reimplements data 0x5669d8: zMath::g_zMath_Vec3Zero.\n"
            " * Purpose: Stores the shared zero vector used by zMath callers.\n"
            " */\n"
            "zVec3 g_zMath_Vec3Zero = {0};\n",
            owners_text,
        )

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn("findings: 0", result.stdout)

    def test_changed_flag_is_not_registered(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "sample.cpp"
            source.write_text("int Sample() { return 1; }\n", encoding="utf-8")
            result = subprocess.run(
                [
                    sys.executable,
                    str(SCRIPT),
                    "--changed",
                    "--summary",
                ],
                cwd=REPO_ROOT,
                capture_output=True,
                text=True,
                encoding="utf-8",
            )

        self.assertEqual(2, result.returncode)
        self.assertIn("unrecognized arguments: --changed", result.stderr)

    def test_retired_root_alias_is_rejected(self) -> None:
        module = load_audit_module()

        with self.assertRaises(SystemExit) as raised:
            module.build_parser().parse_args(
                ["--root", "src/Battlesport/hud.cpp"]
            )
        self.assertEqual(2, raised.exception.code)

    def test_default_and_explicit_src_cover_full_production_tree(self) -> None:
        module = load_audit_module()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            expected = [root / "src"]

            self.assertEqual(expected, module.resolve_audit_paths([], root))
            self.assertEqual(expected, module.resolve_audit_paths(["src"], root))

    def test_explicit_src_subpath_is_not_remapped_to_production_roots(self) -> None:
        module = load_audit_module()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)

            self.assertEqual([root / "src" / "native"], module.resolve_audit_paths(["src/native"], root))


if __name__ == "__main__":
    unittest.main()
