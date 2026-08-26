from __future__ import annotations

from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.lib.vc5_compile_topology import (  # noqa: E402
    CanonicalMfc,
    include_trace_report,
    parse_canonical_mfc,
    validate_canonical_mfc_roots,
)


class CanonicalMfcCustomHeaderTests(unittest.TestCase):
    def parse_spec(
        self,
        repository_root: Path,
        *,
        custom_headers: object,
        required_headers: object | None = None,
    ) -> CanonicalMfc:
        data = {
            "include_root": "provider/include",
            "lib_root": "provider/lib",
            "required_headers": (
                ["AFXWIN.H"] if required_headers is None else required_headers
            ),
            "required_libs": [],
            "require_include_trace": True,
            "allowed_custom_headers": custom_headers,
        }
        result = parse_canonical_mfc(
            data,
            resolve_path=lambda value: repository_root / value,
            manifest_path=repository_root / "manifest.json",
            repository_root=repository_root,
        )
        self.assertIsNotNone(result)
        return result

    @staticmethod
    def write_log(path: Path, *headers: Path) -> None:
        path.write_text(
            "".join(f'#line 1 "{header}"\n' for header in headers),
            encoding="utf-8",
        )

    @staticmethod
    def direct_spec(
        include_root: Path,
        lib_root: Path,
        *custom_headers: Path,
    ) -> CanonicalMfc:
        return CanonicalMfc(
            include_root=include_root,
            lib_root=lib_root,
            required_headers=("AFXWIN.H",),
            required_libs=(),
            require_include_trace=True,
            allowed_custom_headers=tuple(custom_headers),
        )

    def test_exact_repository_relative_custom_header_is_resolved(self) -> None:
        with tempfile.TemporaryDirectory() as raw_root:
            root = Path(raw_root)
            expected = (root / "src" / "Battlesport" / "afxv_cfg.h").resolve()
            spec = self.parse_spec(
                root,
                custom_headers=["src/Battlesport/afxv_cfg.h"],
            )
            self.assertEqual((expected,), spec.allowed_custom_headers)

    def test_custom_header_path_shape_is_fail_closed(self) -> None:
        with tempfile.TemporaryDirectory() as raw_root:
            root = Path(raw_root)
            rejected = (
                ([str((root / "absolute.h").resolve())], "repository-relative"),
                (["../outside.h"], "stay inside the repository"),
                (["src/*.h"], "exact repository-relative path"),
                (["src/config.cpp"], "must name a .h or .inl file"),
                (["provider/include/afxv_cfg.h"], "canonical provider root"),
                (
                    ["src/afxv_cfg.h", "src/./afxv_cfg.h"],
                    "duplicate normalized path",
                ),
            )
            for value, diagnostic in rejected:
                with self.subTest(value=value):
                    with self.assertRaisesRegex(ValueError, diagnostic):
                        self.parse_spec(root, custom_headers=value)

    def test_custom_header_field_requires_nonempty_strings(self) -> None:
        with tempfile.TemporaryDirectory() as raw_root:
            root = Path(raw_root)
            for value in ("src/afxv_cfg.h", [""], [17]):
                with self.subTest(value=value):
                    with self.assertRaisesRegex(
                        ValueError,
                        "allowed_custom_headers must be strings",
                    ):
                        self.parse_spec(root, custom_headers=value)

    def test_required_provider_headers_cannot_escape_canonical_root(self) -> None:
        with tempfile.TemporaryDirectory() as raw_root:
            root = Path(raw_root)
            with self.assertRaisesRegex(ValueError, "canonical root"):
                self.parse_spec(
                    root,
                    custom_headers=[],
                    required_headers=["../outside/AFXWIN.H"],
                )

    def test_configured_custom_header_must_exist_during_root_validation(self) -> None:
        with tempfile.TemporaryDirectory() as raw_root:
            root = Path(raw_root)
            include_root = root / "provider" / "include"
            lib_root = root / "provider" / "lib"
            include_root.mkdir(parents=True)
            lib_root.mkdir(parents=True)
            afxwin = include_root / "AFXWIN.H"
            afxwin.write_text(
                "Copyright (C) 1992-1997 Microsoft Corporation\n"
                "#define __AFXWIN_H__\n",
                encoding="latin-1",
            )
            missing = root / "src" / "afxv_cfg.h"
            spec = self.direct_spec(include_root, lib_root, missing)
            with self.assertRaisesRegex(ValueError, "custom header is missing"):
                validate_canonical_mfc_roots(
                    spec,
                    (include_root,),
                    (lib_root,),
                )

    def test_include_trace_accepts_only_exact_configured_custom_header(self) -> None:
        with tempfile.TemporaryDirectory() as raw_root:
            root = Path(raw_root)
            include_root = root / "provider" / "include"
            lib_root = root / "provider" / "lib"
            custom = root / "src" / "Battlesport" / "afxv_cfg.h"
            sibling = root / "other" / "afxv_cfg.h"
            for directory in (include_root, lib_root, custom.parent, sibling.parent):
                directory.mkdir(parents=True, exist_ok=True)
            afxwin = include_root / "AFXWIN.H"
            afxwin.write_text("#define __AFXWIN_H__\n", encoding="latin-1")
            custom.write_text("#define _AFX_PORTABLE\n", encoding="latin-1")
            sibling.write_text("#define _AFX_PORTABLE\n", encoding="latin-1")
            log = root / "compile.log"
            self.write_log(log, afxwin, custom, sibling)

            report = include_trace_report(
                self.direct_spec(include_root, lib_root, custom),
                (log,),
            )

            self.assertFalse(report["ok"])
            self.assertTrue(report["afxwin_observed"])
            self.assertEqual(
                "canonical-provider",
                report["observed"][str(afxwin.resolve())]["classification"],
            )
            self.assertEqual(
                "custom-configuration-input",
                report["observed"][str(custom.resolve())]["classification"],
            )
            self.assertEqual(
                "rejected-external",
                report["observed"][str(sibling.resolve())]["classification"],
            )
            self.assertNotIn(str(custom.resolve()), report["diagnostics"])
            self.assertIn(str(sibling.resolve()), report["diagnostics"])

    def test_non_afx_named_configured_custom_header_is_reported_and_allowed(self) -> None:
        with tempfile.TemporaryDirectory() as raw_root:
            root = Path(raw_root)
            include_root = root / "provider" / "include"
            lib_root = root / "provider" / "lib"
            custom = root / "src" / "mfc_custom_config.h"
            for directory in (include_root, lib_root, custom.parent):
                directory.mkdir(parents=True, exist_ok=True)
            afxwin = include_root / "AFXWIN.H"
            afxwin.write_text("#define __AFXWIN_H__\n", encoding="latin-1")
            custom.write_text("#define _AFX_PORTABLE\n", encoding="latin-1")
            log = root / "compile.log"
            self.write_log(log, afxwin, custom)

            report = include_trace_report(
                self.direct_spec(include_root, lib_root, custom),
                (log,),
            )

            self.assertTrue(report["ok"])
            self.assertEqual(
                "custom-configuration-input",
                report["observed"][str(custom.resolve())]["classification"],
            )

    def test_external_allowed_afxwin_does_not_satisfy_canonical_provider_trace(self) -> None:
        with tempfile.TemporaryDirectory() as raw_root:
            root = Path(raw_root)
            include_root = root / "provider" / "include"
            lib_root = root / "provider" / "lib"
            external_afxwin = root / "src" / "AFXWIN.H"
            for directory in (include_root, lib_root, external_afxwin.parent):
                directory.mkdir(parents=True, exist_ok=True)
            external_afxwin.write_text(
                "#define __AFXWIN_H__\n",
                encoding="latin-1",
            )
            log = root / "compile.log"
            self.write_log(log, external_afxwin)

            report = include_trace_report(
                self.direct_spec(include_root, lib_root, external_afxwin),
                (log,),
            )

            self.assertFalse(report["ok"])
            self.assertFalse(report["afxwin_observed"])
            self.assertIn(
                "AFXWIN.H was not observed in /showIncludes output",
                report["diagnostics"],
            )
            self.assertEqual(
                "custom-configuration-input",
                report["observed"][str(external_afxwin.resolve())][
                    "classification"
                ],
            )


if __name__ == "__main__":
    unittest.main()
