from __future__ import annotations

from dataclasses import dataclass

from _recoil.lib.progress import DEFAULT_PROGRESS_PATH


@dataclass(frozen=True)
class ReferenceImage:
    key: str
    display_name: str
    reference_path: str
    bndb_path: str
    manifest_path: str
    source_root: str
    image_base: int
    probe_address: str
    platform: str = "windows-x86"
    arch: str = "x86"


REFERENCE_IMAGES: dict[str, ReferenceImage] = {
    "recoil": ReferenceImage(
        key="recoil",
        display_name="Recoil.exe",
        reference_path="support/Recoil.exe",
        bndb_path="D:/Recoil Project/Decomp/Recoil.bndb",
        manifest_path=".agent/REFERENCE_EXECUTABLE.json",
        source_root="src",
        image_base=0x400000,
        probe_address="0x401000",
    ),
    "messages": ReferenceImage(
        key="messages",
        display_name="messages.dll",
        reference_path="support/messages.dll",
        bndb_path="D:/Recoil Project/Decomp/messages.bndb",
        manifest_path=".agent/REFERENCE_MESSAGES_DLL.json",
        source_root="src/Messages",
        image_base=0x10000000,
        probe_address="0x10001010",
    ),
}


def reference_image(key: str) -> ReferenceImage:
    try:
        return REFERENCE_IMAGES[key]
    except KeyError as exc:
        valid = ", ".join(sorted(REFERENCE_IMAGES))
        raise ValueError(f"unknown reference image {key!r}; expected one of: {valid}") from exc


def reference_image_keys() -> tuple[str, ...]:
    return tuple(sorted(REFERENCE_IMAGES))


def default_owner_ledger_path(_binary: str) -> str:
    return str(DEFAULT_PROGRESS_PATH)


def resolve_owner_ledger_path(binary: str, override: str | None = None) -> str:
    return override if override else default_owner_ledger_path(binary)
