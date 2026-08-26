from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
from typing import Any, Mapping

from _recoil.lib.progress import ProgressDocument, ProgressError


@dataclass(frozen=True)
class MigrationResult:
    """Read-only status for the completed schema-v5 cutover.

    Runtime schema migration was intentionally retired after the one-time
    revision-led migration.  Keeping this tiny status object lets callers
    diagnose an old tracker without restoring an executable legacy migrator.
    """

    source_version: int
    target_version: int
    revision: int

    def to_dict(self) -> dict[str, Any]:
        return {
            "source_version": self.source_version,
            "target_version": self.target_version,
            "revision": self.revision,
            "changed": False,
            "migration_available": False,
        }


def migration_status(path: Path) -> MigrationResult:
    document = ProgressDocument.load(path)
    version = document.data.get("schema_version")
    if not isinstance(version, int) or isinstance(version, bool):
        raise ProgressError("progress tracker has no integer schema_version")
    if version != 5:
        raise ProgressError(
            f"schema {version} is no longer accepted by the live runtime; restore the "
            "pre-cutover backup and run the governed one-time migration"
        )
    return MigrationResult(version, version, document.revision)


def migrate_store(
    progress_path: Path,
    reference_manifests: Mapping[str, Path] | None = None,
    *,
    expected_revision: int | None = None,
    apply: bool = False,
) -> MigrationResult:
    """Reject runtime migrations while preserving an explicit diagnostic API."""

    del reference_manifests, apply
    result = migration_status(progress_path)
    if expected_revision is not None and result.revision != expected_revision:
        raise ProgressError(
            f"revision changed: expected {expected_revision}, found {result.revision}"
        )
    return result
