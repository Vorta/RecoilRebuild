from __future__ import annotations

from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

from tests.tools.owner_fixture import owner_record, write_ledger


REPO_ROOT = Path(__file__).resolve().parents[2]
SCRIPT = REPO_ROOT / "tools" / "_recoil" / "commands" / "raw_offset_guard.py"


class RecoilRawOffsetGuardTests(unittest.TestCase):
    def run_guard(self, root: Path, owners: Path, *extra: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [
                sys.executable,
                str(SCRIPT),
                "--root",
                str(root),
                "--progress",
                str(owners),
                "--allowlist",
                str(owners.parent / "RAW_OFFSET_ALLOWLIST.txt"),
                *extra,
            ],
            cwd=REPO_ROOT,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )

    def make_temp(self) -> tuple[tempfile.TemporaryDirectory[str], Path, Path]:
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        root = Path(temp.name) / "src"
        root.mkdir()
        owners = Path(temp.name) / "SOURCE_OWNERS.json"
        (Path(temp.name) / "RAW_OFFSET_ALLOWLIST.txt").write_text("", encoding="utf-8")
        return temp, root, owners

    def write_owners(self, owners: Path, *, tier: str, file: str = "bad.cpp") -> None:
        write_ledger(
            owners,
            owner_record(
                "test.owner",
                kind="source-file",
                anchors=("0x401000",),
                functions=("0x401000",),
                tiers={"0x401000": tier},
                gates={
                    "boundary": "accepted",
                    "source": "accepted",
                    "data": "none",
                    "functional": "accepted",
                    "linkage": "accepted",
                    "byte": "pending",
                },
                section="test",
                source_paths=(file,),
                address_metadata={
                    "0x401000": {
                        "source_path": file,
                        "target": "sample",
                        "group": "test.group",
                        "name": "Sample",
                    }
                },
                blocker="none",
            ),
        )

    def test_clean_tier_c_source_passes(self) -> None:
        _, root, owners = self.make_temp()
        self.write_owners(owners, tier="C", file="clean.cpp")
        (root / "clean.cpp").write_text("struct S { int field; }; int f(S *s) { return s->field; }\n", encoding="utf-8")

        result = self.run_guard(root, owners)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_comments_strings_and_offsetof_are_ignored(self) -> None:
        _, root, owners = self.make_temp()
        self.write_owners(owners, tier="C", file="clean.cpp")
        (root / "clean.cpp").write_text(
            '#define RECOIL_STATIC_ASSERT(x)\n'
            'const char *s = "kRawOffset";\n'
            "// kRawOffset\n"
            "/* ((char *)p + 0x10) */\n"
            "RECOIL_STATIC_ASSERT(offsetof(S, field) == 4);\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, owners)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_tier_x_raw_offset_passes(self) -> None:
        _, root, owners = self.make_temp()
        self.write_owners(owners, tier="X")
        (root / "bad.cpp").write_text("const int kFieldOffset = 12;\n", encoding="utf-8")

        result = self.run_guard(root, owners)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_tier_c_raw_offset_fails(self) -> None:
        _, root, owners = self.make_temp()
        self.write_owners(owners, tier="C")
        (root / "bad.cpp").write_text(
            "const int kFieldOffset = 12;\n"
            "int f(PlayerState *s) { return *(int *)(s->bytes + 0x3ec); }\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, owners)

        self.assertEqual(result.returncode, 1)
        self.assertIn("bad.cpp:1", result.stdout)
        self.assertIn("tiered owners: test.owner:C", result.stdout)

    def test_allowlist_allows_specific_location(self) -> None:
        temp, root, owners = self.make_temp()
        self.write_owners(owners, tier="C")
        (root / "bad.cpp").write_text("const int kFieldOffset = 12;\n", encoding="utf-8")
        (Path(temp.name) / "RAW_OFFSET_ALLOWLIST.txt").write_text(
            "bad.cpp 1 named-offset-constant\n",
            encoding="utf-8",
        )

        result = self.run_guard(root, owners)

        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()

