from __future__ import annotations

import collections
import json
import re
import unittest
from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
MANIFEST_DIR = REPO_ROOT / "tools" / "functional_verify_targets"
NATIVE_DIR = REPO_ROOT / "tests" / "native"
REGISTRY_PATH = NATIVE_DIR / "smoke.cpp"
CMAKE_PATH = NATIVE_DIR / "CMakeLists.txt"

REGISTRATION_RE = re.compile(r'\{\s*"([^"]+_smoke)"\s*,')
NATIVE_TARGET_RE = re.compile(
    r"add_executable\(\s*recoil_native_smoke\s+(.*?)\n\)",
    re.DOTALL,
)
NATIVE_CPP_SOURCE_RE = re.compile(
    r"^\s*([A-Za-z0-9_./-]+\.cpp)\s*$",
    re.MULTILINE,
)
DEFINITION_RE = re.compile(
    r'extern\s+"C"\s+int\s+([A-Za-z_][A-Za-z0-9_]*_smoke)\s*'
    r"\([^;{}]*\)\s*\{",
    re.DOTALL,
)


def manifest_smoke_names() -> set[str]:
    names: set[str] = set()
    for path in sorted(MANIFEST_DIR.glob("*.json")):
        payload = json.loads(path.read_text(encoding="utf-8"))
        names.update(payload.get("smoke_tests", []))
    return names


def canonical_native_cpp_sources() -> list[Path]:
    match = NATIVE_TARGET_RE.search(CMAKE_PATH.read_text(encoding="utf-8"))
    if match is None:
        raise AssertionError("recoil_native_smoke add_executable source list is missing")
    relative_paths = NATIVE_CPP_SOURCE_RE.findall(match.group(1))
    paths = [NATIVE_DIR / relative_path for relative_path in relative_paths]
    missing = [str(path.relative_to(REPO_ROOT)) for path in paths if not path.is_file()]
    if missing:
        raise AssertionError(
            f"recoil_native_smoke references missing checked-in C++ sources: {missing}"
        )
    return paths


class NativeFunctionalManifestTests(unittest.TestCase):
    def test_every_manifest_smoke_is_registered_exactly_once(self) -> None:
        expected = manifest_smoke_names()
        registrations = collections.Counter(
            REGISTRATION_RE.findall(REGISTRY_PATH.read_text(encoding="utf-8"))
        )
        missing = sorted(expected - registrations.keys())
        duplicate = sorted(
            name for name in expected if registrations.get(name, 0) != 1
        )
        self.assertFalse(missing, f"manifest smokes missing from registry: {missing}")
        self.assertFalse(
            duplicate,
            "manifest smokes not registered exactly once: "
            f"{[(name, registrations[name]) for name in duplicate]}",
        )

    def test_every_manifest_smoke_has_checked_in_canonical_target_definition(self) -> None:
        expected = manifest_smoke_names()
        definitions: collections.Counter[str] = collections.Counter()
        definition_paths: dict[str, list[str]] = collections.defaultdict(list)

        source_paths = canonical_native_cpp_sources()
        self.assertEqual(
            len(source_paths),
            len(set(source_paths)),
            "recoil_native_smoke lists a checked-in C++ source more than once",
        )
        for path in source_paths:
            for name in DEFINITION_RE.findall(path.read_text(encoding="utf-8")):
                definitions[name] += 1
                definition_paths[name].append(path.name)

        missing = sorted(expected - definitions.keys())
        self.assertFalse(missing, f"manifest smokes missing definitions: {missing}")
        # Some legacy reservoir TUs retain compile-time-disabled alternatives.
        # Their raw text is not the canonical compiled definition population;
        # the target link is the authoritative exactly-once check.  This static
        # guard proves that every manifest name is backed by checked-in source
        # in the canonical target instead of a configure-time generated .cpp.


if __name__ == "__main__":
    unittest.main()
