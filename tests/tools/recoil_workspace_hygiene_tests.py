from __future__ import annotations

import json
from pathlib import Path
import sys
import tempfile
import unittest


REPO_ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(REPO_ROOT / "tools"))

from _recoil.commands.workspace_hygiene import (  # noqa: E402
    DEFAULT_ALLOWED_ROOTS,
    DEVSPACE_MANIFEST_NAME,
    DEVSPACE_MANIFEST_SCHEMA,
    find_offenders,
)


class RecoilWorkspaceHygieneTests(unittest.TestCase):
    def _make_governed_scratch(
        self,
        root: Path,
        *,
        manifest_overrides: dict[str, object] | None = None,
        entry_overrides: dict[str, object] | None = None,
    ) -> tuple[Path, Path]:
        session = root / ".devspace" / "vc5-probe"
        generated = session / "nested" / "probe.obj"
        generated.parent.mkdir(parents=True)
        generated.write_bytes(b"object bytes")
        entry: dict[str, object] = {
            "path": "nested/probe.obj",
            "bytes": generated.stat().st_size,
        }
        entry.update(entry_overrides or {})
        manifest: dict[str, object] = {
            "schema": DEVSPACE_MANIFEST_SCHEMA,
            "status": "complete",
            "repo_root": str(root.resolve()),
            "purpose": "compiler-probe",
            "artifacts": [entry],
        }
        manifest.update(manifest_overrides or {})
        manifest_path = session / DEVSPACE_MANIFEST_NAME
        manifest_path.write_text(json.dumps(manifest), encoding="utf-8")
        return generated, manifest_path

    def _make_staged_upload(
        self,
        root: Path,
        *,
        metadata_overrides: dict[str, object] | None = None,
        entry_overrides: dict[str, object] | None = None,
    ) -> Path:
        run_root = root / ".devspace" / "runs" / "test-chatgpt-call"
        uploads = run_root / "uploads"
        uploads.mkdir(parents=True)
        candidate = uploads / "sample.cod"
        payload = b"VC5 compiler listing\n"
        candidate.write_bytes(payload)
        original = root / "input.txt"
        original.write_bytes(b"original compiler listing\n")
        entry: dict[str, object] = {
            "path": str(candidate.resolve()),
            "bytes": len(payload),
            "original": {
                "path": str(original.resolve()),
                "bytes": original.stat().st_size,
            },
            "staged": True,
        }
        entry.update(entry_overrides or {})
        run_metadata: dict[str, object] = {
            "project": {"repoRoot": str(root.resolve())},
            "upload": {"ok": True, "files": [entry]},
            # The response may time out after a successful upload.
            "ok": False,
            "errorCode": "chatgpt.response_timeout",
        }
        run_metadata.update(metadata_overrides or {})
        (run_root / "receipt.json").write_text(json.dumps(run_metadata), encoding="utf-8")
        return candidate

    def test_root_artifact_is_reported(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "NUL.obj").write_bytes(b"obj")
            (root / "src").mkdir()
            (root / "src" / "ok.cpp").write_text("int ok;\n", encoding="utf-8")

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertEqual(["NUL.obj"], [path.name for path in offenders])

    def test_build_artifacts_under_build_are_allowed(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build_dir = root / "build"
            build_dir.mkdir()
            (build_dir / "sample.obj").write_bytes(b"obj")

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertEqual([], offenders)

    def test_upgrade_log_is_reported_outside_allowed_roots(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "UpgradeLog.htm").write_text("generated\n", encoding="utf-8")

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertEqual(["UpgradeLog.htm"], [path.name for path in offenders])

    def test_staged_upload_metadata_is_allowed_after_response_timeout(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._make_staged_upload(root)

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertEqual([], offenders)

    def test_staged_upload_metadata_checks_fail_closed(self) -> None:
        cases = {
            "wrong repository": {
                "metadata_overrides": {"project": {"repoRoot": str(REPO_ROOT.resolve())}},
            },
            "failed upload": {
                "metadata_overrides": {"upload": {"ok": False, "files": []}},
            },
            "unstaged entry": {"entry_overrides": {"staged": False}},
            "path mismatch": {"entry_overrides": {"path": str(REPO_ROOT / "elsewhere.cod")}},
            "size mismatch": {"entry_overrides": {"bytes": 1}},
            "missing original": {"entry_overrides": {"original": {"path": str(REPO_ROOT / "missing.cod"), "bytes": 1}}},
            "original size mismatch": {"entry_overrides": {"original": {"path": str(REPO_ROOT / "README.md"), "bytes": 1}}},
        }
        for label, options in cases.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                candidate = self._make_staged_upload(root, **options)

                offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

                self.assertEqual([candidate], offenders)

    def test_missing_or_malformed_run_metadata_remains_an_offender(self) -> None:
        for contents in (None, "{not json"):
            with self.subTest(contents=contents), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                candidate = self._make_staged_upload(root)
                run_metadata = candidate.parents[1] / "receipt.json"
                if contents is None:
                    run_metadata.unlink()
                else:
                    run_metadata.write_text(contents, encoding="utf-8")

                offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

                self.assertEqual([candidate], offenders)

    def test_exact_governed_session_scratch_manifest_allows_only_listed_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            self._make_governed_scratch(root)

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertEqual([], offenders)

    def test_governed_session_scratch_manifest_fails_closed(self) -> None:
        cases = {
            "wrong schema": {"manifest_overrides": {"schema": "other"}},
            "wrong repository": {
                "manifest_overrides": {"repo_root": str(REPO_ROOT.resolve())}
            },
            "extra top-level key": {"manifest_overrides": {"comment": "not schema"}},
            "absolute path": {
                "entry_overrides": {"path": str((REPO_ROOT / "probe.obj").resolve())}
            },
            "escape": {"entry_overrides": {"path": "../probe.obj"}},
            "wrong bytes": {"entry_overrides": {"bytes": 1}},
            "extra entry key": {"entry_overrides": {"kind": "object"}},
        }
        for label, options in cases.items():
            with self.subTest(label=label), tempfile.TemporaryDirectory() as tmp:
                root = Path(tmp)
                generated, manifest = self._make_governed_scratch(root, **options)

                offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

                self.assertIn(manifest, offenders)
                self.assertIn(generated, offenders)

    def test_unlisted_generated_file_rejects_manifest_and_file(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            _generated, manifest = self._make_governed_scratch(root)
            unlisted = manifest.parent / "unlisted.pdb"
            unlisted.write_bytes(b"pdb")

            offenders = find_offenders(root, set(DEFAULT_ALLOWED_ROOTS))

        self.assertIn(manifest, offenders)
        self.assertIn(unlisted, offenders)


if __name__ == "__main__":
    unittest.main()
