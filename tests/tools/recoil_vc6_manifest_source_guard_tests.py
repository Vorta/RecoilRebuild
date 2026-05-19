import json
import sys
import tempfile
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from recoil_vc6_manifest_source_guard import main as guard_main  # noqa: E402


def write_source_manifest(directory: Path) -> None:
    source_path = directory / "source.cpp"
    source_path.write_text(
        "// Reimplements 0x401000: Sample\nint __cdecl Sample() { return 0; }\n",
        encoding="utf-8",
    )
    (directory / "sample.json").write_text(
        json.dumps(
            {
                "name": "sample",
                "description": "sample",
                "source_filename": "source.cpp",
                "source_from": str(source_path),
                "compiler_flags": ["/nologo", "/TP"],
                "functions": [
                    {"address": "0x401000", "symbol": "?Sample@@YAHXZ", "name": "Sample"}
                ],
            }
        ),
        encoding="utf-8",
    )


def write_inline_manifest(directory: Path) -> None:
    (directory / "sample.json").write_text(
        json.dumps(
            {
                "name": "sample",
                "description": "sample",
                "source_filename": "source.cpp",
                "compiler_flags": ["/nologo", "/TP"],
                "functions": [
                    {"address": "0x401000", "symbol": "?Sample@@YAHXZ", "name": "Sample"}
                ],
                "source": ["int __cdecl Sample() { return 0; }"],
            }
        ),
        encoding="utf-8",
    )


class RecoilVc6ManifestSourceGuardTests(unittest.TestCase):
    def test_guard_accepts_source_from_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_source_manifest(manifest_dir)

            result = guard_main(["--manifest-dir", str(manifest_dir), "--baseline", str(Path(tmp) / "baseline.txt")])

        self.assertEqual(0, result)

    def test_guard_rejects_unbaselined_inline_manifest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            manifest_dir = Path(tmp) / "targets"
            manifest_dir.mkdir()
            write_inline_manifest(manifest_dir)

            result = guard_main(["--manifest-dir", str(manifest_dir), "--baseline", str(Path(tmp) / "baseline.txt")])

        self.assertEqual(1, result)


if __name__ == "__main__":
    unittest.main()
