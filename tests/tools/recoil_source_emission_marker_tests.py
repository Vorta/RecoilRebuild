import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib.source_emission_markers import (  # noqa: E402
    EmissionAnchor,
    collect_source_closure,
    normalize_anchor_path,
    validate_source_emission_marker,
)


class RecoilSourceEmissionMarkerTests(unittest.TestCase):
    def test_attached_type_marker_in_reachable_header(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "src").mkdir()
            (root / "src" / "sample.cpp").write_text('#include "sample.h"\n', encoding="utf-8")
            (root / "src" / "sample.h").write_text(
                "/**\n"
                " * Emits 0x401000: VC5 scalar deleting destructor for Sample.\n"
                " */\n"
                "class Sample { public: virtual ~Sample(); };\n",
                encoding="utf-8",
            )

            marker = validate_source_emission_marker(
                source_from="src/sample.cpp",
                repo_root=root,
                anchor=EmissionAnchor(
                    path="src/sample.h",
                    kind="type-definition",
                    name="Sample",
                ),
                address="0x00401000",
            )

        self.assertEqual("0x401000", marker.address)
        self.assertEqual("src/sample.h", marker.path)
        self.assertEqual(2, marker.line)

    def test_one_docblock_may_describe_distinct_emissions(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "sample.cpp").write_text(
                "/**\n"
                " * Emits 0x401000: first helper.\n"
                " * Emits 0x401010: second helper.\n"
                " */\n"
                "struct Sample { virtual ~Sample(); };\n",
                encoding="utf-8",
            )
            anchor = EmissionAnchor(path="sample.cpp", kind="type-definition", name="Sample")

            first = validate_source_emission_marker(
                source_from="sample.cpp", repo_root=root, anchor=anchor, address="0x401000"
            )
            second = validate_source_emission_marker(
                source_from="sample.cpp", repo_root=root, anchor=anchor, address="0x401010"
            )

        self.assertEqual("first helper.", first.description)
        self.assertEqual("second helper.", second.description)

    def test_detached_registry_marker_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "sample.cpp").write_text(
                "/** Emits 0x401000: detached registry row. */\n"
                "int unrelated;\n\n"
                "struct Sample { virtual ~Sample(); };\n",
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "no immediately attached"):
                validate_source_emission_marker(
                    source_from="sample.cpp",
                    repo_root=root,
                    anchor=EmissionAnchor(
                        path="sample.cpp", kind="type-definition", name="Sample"
                    ),
                    address="0x401000",
                )

    def test_duplicate_address_across_include_closure_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "sample.cpp").write_text(
                '#include "sample.h"\n/** Emits 0x401000: duplicate. */\nstruct Other {};\n',
                encoding="utf-8",
            )
            (root / "sample.h").write_text(
                "/** Emits 0x401000: primary. */\nstruct Sample {};\n",
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "duplicate Emits 0x401000"):
                validate_source_emission_marker(
                    source_from="sample.cpp",
                    repo_root=root,
                    anchor=EmissionAnchor(path="sample.h", kind="type-definition", name="Sample"),
                    address="0x401000",
                )

    def test_unreachable_anchor_path_is_rejected(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "sample.cpp").write_text("int sample;\n", encoding="utf-8")
            (root / "sample.h").write_text(
                "/** Emits 0x401000: helper. */\nstruct Sample {};\n", encoding="utf-8"
            )

            with self.assertRaisesRegex(ValueError, "is not reachable"):
                validate_source_emission_marker(
                    source_from="sample.cpp",
                    repo_root=root,
                    anchor=EmissionAnchor(path="sample.h", kind="type-definition", name="Sample"),
                    address="0x401000",
                )

    def test_forward_declaration_is_not_a_type_definition(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "sample.cpp").write_text(
                "/** Emits 0x401000: helper. */\nclass Sample;\n", encoding="utf-8"
            )

            with self.assertRaisesRegex(ValueError, "found 0"):
                validate_source_emission_marker(
                    source_from="sample.cpp",
                    repo_root=root,
                    anchor=EmissionAnchor(
                        path="sample.cpp", kind="type-definition", name="Sample"
                    ),
                    address="0x401000",
                )

    def test_function_definition_anchor(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "sample.cpp").write_text(
                "/** Emits 0x401000: VC5 EH cleanup. */\n"
                "void Sample::Run(int value) const { (void)value; }\n",
                encoding="utf-8",
            )

            marker = validate_source_emission_marker(
                source_from="sample.cpp",
                repo_root=root,
                anchor=EmissionAnchor(
                    path="sample.cpp", kind="function-definition", name="Sample::Run"
                ),
                address="0x401000",
            )

        self.assertEqual("VC5 EH cleanup.", marker.description)

    def test_data_definition_anchor_and_extern_rejection(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            source = root / "sample.cpp"
            source.write_text(
                "/** Emits 0x401000: static initializer helper. */\n"
                "SampleState g_sample_state;\n",
                encoding="utf-8",
            )
            marker = validate_source_emission_marker(
                source_from="sample.cpp",
                repo_root=root,
                anchor=EmissionAnchor(
                    path="sample.cpp", kind="data-definition", name="g_sample_state"
                ),
                address="0x401000",
            )
            self.assertEqual("static initializer helper.", marker.description)

            source.write_text(
                "/** Emits 0x401000: static initializer helper. */\n"
                "extern SampleState g_sample_state;\n",
                encoding="utf-8",
            )
            with self.assertRaisesRegex(ValueError, "found 0"):
                validate_source_emission_marker(
                    source_from="sample.cpp",
                    repo_root=root,
                    anchor=EmissionAnchor(
                        path="sample.cpp", kind="data-definition", name="g_sample_state"
                    ),
                    address="0x401000",
                )

    def test_include_cycles_are_deduplicated(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "a.h").write_text('#include "b.h"\n', encoding="utf-8")
            (root / "b.h").write_text('#include "a.h"\n', encoding="utf-8")

            closure = collect_source_closure("a.h", repo_root=root)

        self.assertEqual({"a.h", "b.h"}, {item.repo_path for item in closure})

    def test_anchor_path_must_be_normalized_and_relative(self) -> None:
        with self.assertRaisesRegex(ValueError, "repository-relative"):
            normalize_anchor_path("C:/source/sample.h")
        with self.assertRaisesRegex(ValueError, "normalized"):
            normalize_anchor_path("src/../sample.h")


if __name__ == "__main__":
    unittest.main()
