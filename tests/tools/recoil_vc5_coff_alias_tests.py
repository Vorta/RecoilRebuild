from __future__ import annotations

import json
import os
from dataclasses import replace
from pathlib import Path
import struct
import sys
import tempfile
import unittest
from unittest import mock

REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands import vc5_build
from _recoil.lib.coff_alias import (
    CoffAlias,
    CoffAliasSource,
    parse_alias_source_text,
    parse_coff_alias_sources,
    resolve_llvm_ml,
    validate_alias_object,
)


def zero_section_alias_object(alias: str = "_alias", target: str = "_target") -> bytes:
    def name_field(name: str) -> bytes:
        encoded = name.encode("ascii")
        if len(encoded) > 8:
            raise AssertionError("fixture uses short names")
        return encoded.ljust(8, b"\0")

    target_symbol = name_field(target) + struct.pack("<IhHBB", 0, 0, 0, 2, 0)
    alias_symbol = name_field(alias) + struct.pack("<IhHBB", 0, 0, 0, 105, 1)
    alias_aux = struct.pack("<II", 0, 3) + b"\0" * 10
    header = struct.pack("<HHIIIHH", 0x14C, 0, 0, 20, 3, 0, 0)
    return header + target_symbol + alias_symbol + alias_aux + struct.pack("<I", 4)


class CoffAliasTests(unittest.TestCase):
    def test_alias_source_accepts_only_directives_extern_alias_end(self) -> None:
        externs, aliases = parse_alias_source_text(
            ".386\n.model flat\nEXTERN _target:PROC\n"
            "ALIAS <_alias> = <_target>\nEND\n",
            path=Path("alias.asm"),
        )
        self.assertEqual(externs, {"_target"})
        self.assertEqual(aliases, (CoffAlias("_alias", "_target"),))

    def test_alias_source_rejects_code_data_macros_and_procedures(self) -> None:
        for body in (
            ".code\nEND\n",
            "_value DD 1\nEND\n",
            "thing MACRO\nENDM\nEND\n",
            "_alias PROC\nret\n_alias ENDP\nEND\n",
        ):
            with self.subTest(body=body), self.assertRaises(ValueError):
                parse_alias_source_text(body, path=Path("bad.asm"))

    def test_zero_section_weak_alias_object_passes_exact_structure(self) -> None:
        with tempfile.TemporaryDirectory(dir=REPO_ROOT / "build") as temp:
            root = Path(temp)
            source = root / "alias.asm"
            source.write_text(
                ".386\n.model flat\nEXTERN _target:PROC\n"
                "ALIAS <_alias> = <_target>\nEND\n",
                encoding="utf-8",
            )
            obj = root / "alias.obj"
            obj.write_bytes(zero_section_alias_object())
            spec = CoffAliasSource(
                source=source,
                aliases=(CoffAlias("_alias", "_target"),),
                link_after_source=REPO_ROOT / "src" / "Battlesport" / "hud.cpp",
            )
            report = validate_alias_object(obj, spec)
            self.assertTrue(report["validated"])
            self.assertEqual(report["section_count"], 0)
            self.assertEqual(report["contribution_bytes"], 0)
            self.assertEqual(report["grants"], [])

    def test_alias_object_rejects_sections_or_wrong_weak_target(self) -> None:
        with tempfile.TemporaryDirectory(dir=REPO_ROOT / "build") as temp:
            root = Path(temp)
            source = root / "alias.asm"
            source.write_text(
                ".386\n.model flat\nEXTERN _target:PROC\n"
                "ALIAS <_alias> = <_target>\nEND\n",
                encoding="utf-8",
            )
            spec = CoffAliasSource(
                source=source,
                aliases=(CoffAlias("_alias", "_other"),),
                link_after_source=REPO_ROOT / "src" / "Battlesport" / "hud.cpp",
            )
            obj = root / "alias.obj"
            obj.write_bytes(zero_section_alias_object())
            with self.assertRaisesRegex(ValueError, "exact declared extern"):
                validate_alias_object(obj, spec)
            malformed = bytearray(zero_section_alias_object())
            struct.pack_into("<H", malformed, 2, 1)
            obj.write_bytes(malformed)
            with self.assertRaisesRegex(ValueError, "zero sections"):
                validate_alias_object(obj, spec)

    def test_llvm_ml_resolution_order(self) -> None:
        with tempfile.TemporaryDirectory() as temp:
            configured = Path(temp) / "configured-llvm-ml.exe"
            configured.write_bytes(b"fixture")
            self.assertEqual(
                resolve_llvm_ml({"RECOIL_LLVM_ML": str(configured)}),
                configured.resolve(),
            )
            path_tool = Path(temp) / "path-llvm-ml.exe"
            path_tool.write_bytes(b"fixture")
            with mock.patch(
                "_recoil.lib.coff_alias.shutil.which",
                side_effect=lambda name: str(path_tool) if name == "llvm-ml.exe" else None,
            ):
                self.assertEqual(resolve_llvm_ml({}), path_tool.resolve())
            with self.assertRaisesRegex(ValueError, "RECOIL_LLVM_ML"):
                resolve_llvm_ml({"RECOIL_LLVM_ML": str(Path(temp) / "missing.exe")})

    def test_manifest_paths_are_repository_relative_and_link_anchor_is_configured(self) -> None:
        with tempfile.TemporaryDirectory(dir=REPO_ROOT / "build") as temp:
            root = Path(temp)
            source = root / "alias.asm"
            source.write_text(
                ".386\n.model flat\nEXTERN _target:PROC\n"
                "ALIAS <_alias> = <_target>\nEND\n",
                encoding="utf-8",
            )
            rel = source.relative_to(REPO_ROOT).as_posix()
            data = {
                "coff_alias_sources": [
                    {
                        "source": rel,
                        "aliases": [{"alias": "_alias", "target": "_target"}],
                        "link_after_source": "src/Battlesport/hud.cpp",
                    }
                ]
            }
            rows = parse_coff_alias_sources(
                data,
                configured_sources=(REPO_ROOT / "src" / "Battlesport" / "hud.cpp",),
                manifest_path=root / "manifest.json",
            )
            self.assertEqual(len(rows), 1)
            data["coff_alias_sources"][0]["source"] = str(source.resolve())
            with self.assertRaisesRegex(ValueError, "repository-relative"):
                parse_coff_alias_sources(
                    data,
                    configured_sources=(REPO_ROOT / "src" / "Battlesport" / "hud.cpp",),
                    manifest_path=root / "manifest.json",
                )

    def test_final_build_without_alias_field_preserves_link_order(self) -> None:
        raw = json.loads(vc5_build.DEFAULT_MANIFEST.read_text(encoding="utf-8"))
        raw.pop("coff_alias_sources", None)
        with tempfile.TemporaryDirectory(dir=REPO_ROOT / "build") as temp:
            manifest = Path(temp) / "manifest.json"
            manifest.write_text(json.dumps(raw), encoding="utf-8")
            config = vc5_build.load_config(manifest)
            self.assertEqual(config.coff_alias_sources, ())
            paths = vc5_build.build_paths(config)
            objects = [
                vc5_build.object_path(config, paths, source)
                for source in config.sources
            ]
            args = vc5_build.link_order_args(config, paths, objects)
            self.assertNotIn("coff-alias", " ".join(args).lower())
            self.assertEqual(
                [item for item in args if item.lower().endswith(".obj")],
                [str(item) for item in objects],
            )

    def test_alias_object_is_inserted_immediately_after_exact_source_anchor(
        self,
    ) -> None:
        config = vc5_build.load_config(vc5_build.DEFAULT_MANIFEST)
        paths = vc5_build.build_paths(config)
        anchor = config.sources[0]
        spec = CoffAliasSource(
            source=REPO_ROOT / "tools" / "_recoil" / "aliases" / "probe.asm",
            aliases=(CoffAlias("_alias", "_target"),),
            link_after_source=anchor,
        )
        config = replace(config, coff_alias_sources=(spec,))
        objects = [
            vc5_build.object_path(config, paths, source)
            for source in config.sources
        ]
        args = vc5_build.link_order_args(config, paths, objects)
        anchor_index = args.index(str(objects[0]))
        self.assertEqual(
            str(vc5_build.alias_object_path(paths, spec.source)),
            args[anchor_index + 1],
        )


if __name__ == "__main__":
    unittest.main()
