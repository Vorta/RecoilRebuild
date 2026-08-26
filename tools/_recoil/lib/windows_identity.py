"""Stable Windows physical identities and read handles.

File and directory identity is based on the volume serial number plus the
kernel file index.  Stable file reads deny write and delete sharing for the
duration of the governed operation.  Non-Windows fallback identities exist
only so unit tests can exercise the same equality contract.
"""

from __future__ import annotations

from contextlib import AbstractContextManager
import ctypes
from ctypes import wintypes
from dataclasses import dataclass
import os
from pathlib import Path
from typing import BinaryIO


class WindowsIdentityError(OSError):
    pass


@dataclass(frozen=True)
class PhysicalFileIdentity:
    volume_identity: int
    file_id: int
    file_size: int
    is_directory: bool
    canonical_path: str

    def same_physical_object(self, other: "PhysicalFileIdentity") -> bool:
        return (
            self.volume_identity == other.volume_identity
            and self.file_id == other.file_id
            and self.is_directory == other.is_directory
        )

    def to_dict(self) -> dict[str, object]:
        return {
            "volume_identity": self.volume_identity,
            "file_id": self.file_id,
            "file_size": self.file_size,
            "is_directory": self.is_directory,
            "canonical_path": self.canonical_path,
        }


if os.name == "nt":
    _kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    _INVALID_HANDLE_VALUE = wintypes.HANDLE(-1).value
    _GENERIC_READ = 0x80000000
    _FILE_SHARE_READ = 0x00000001
    _OPEN_EXISTING = 3
    _FILE_ATTRIBUTE_NORMAL = 0x00000080
    _FILE_FLAG_BACKUP_SEMANTICS = 0x02000000
    _FILE_FLAG_OPEN_REPARSE_POINT = 0x00200000

    class _BY_HANDLE_FILE_INFORMATION(ctypes.Structure):
        _fields_ = [
            ("dwFileAttributes", wintypes.DWORD),
            ("ftCreationTime", wintypes.FILETIME),
            ("ftLastAccessTime", wintypes.FILETIME),
            ("ftLastWriteTime", wintypes.FILETIME),
            ("dwVolumeSerialNumber", wintypes.DWORD),
            ("nFileSizeHigh", wintypes.DWORD),
            ("nFileSizeLow", wintypes.DWORD),
            ("nNumberOfLinks", wintypes.DWORD),
            ("nFileIndexHigh", wintypes.DWORD),
            ("nFileIndexLow", wintypes.DWORD),
        ]

    _kernel32.CreateFileW.argtypes = [
        wintypes.LPCWSTR,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.LPVOID,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.HANDLE,
    ]
    _kernel32.CreateFileW.restype = wintypes.HANDLE
    _kernel32.GetFileInformationByHandle.argtypes = [
        wintypes.HANDLE,
        ctypes.POINTER(_BY_HANDLE_FILE_INFORMATION),
    ]
    _kernel32.GetFileInformationByHandle.restype = wintypes.BOOL
    _kernel32.CloseHandle.argtypes = [wintypes.HANDLE]
    _kernel32.CloseHandle.restype = wintypes.BOOL


def _identity_from_stat(path: Path) -> PhysicalFileIdentity:
    info = path.stat(follow_symlinks=False)
    return PhysicalFileIdentity(
        volume_identity=int(info.st_dev),
        file_id=int(info.st_ino),
        file_size=int(info.st_size),
        is_directory=path.is_dir(),
        canonical_path=str(path.resolve()),
    )


def _win_open(path: Path, *, directory: bool) -> int:
    flags = _FILE_ATTRIBUTE_NORMAL | _FILE_FLAG_OPEN_REPARSE_POINT
    if directory:
        flags |= _FILE_FLAG_BACKUP_SEMANTICS
    handle = _kernel32.CreateFileW(
        str(path),
        _GENERIC_READ,
        _FILE_SHARE_READ,
        None,
        _OPEN_EXISTING,
        flags,
        None,
    )
    if handle == _INVALID_HANDLE_VALUE:
        raise WindowsIdentityError(ctypes.get_last_error(), f"cannot open stable handle: {path}")
    return int(handle)


def _win_identity(handle: int, path: Path, *, directory: bool) -> PhysicalFileIdentity:
    info = _BY_HANDLE_FILE_INFORMATION()
    if not _kernel32.GetFileInformationByHandle(wintypes.HANDLE(handle), ctypes.byref(info)):
        raise WindowsIdentityError(ctypes.get_last_error(), f"cannot query physical identity: {path}")
    return PhysicalFileIdentity(
        volume_identity=int(info.dwVolumeSerialNumber),
        file_id=(int(info.nFileIndexHigh) << 32) | int(info.nFileIndexLow),
        file_size=(int(info.nFileSizeHigh) << 32) | int(info.nFileSizeLow),
        is_directory=directory,
        canonical_path=str(path.resolve()),
    )


def physical_identity(path: str | Path, *, directory: bool | None = None) -> PhysicalFileIdentity:
    resolved = Path(path).resolve(strict=True)
    expected_directory = resolved.is_dir() if directory is None else directory
    if resolved.is_dir() != expected_directory:
        raise WindowsIdentityError(f"physical identity type mismatch: {resolved}")
    if os.name != "nt":
        return _identity_from_stat(resolved)
    handle = _win_open(resolved, directory=expected_directory)
    try:
        return _win_identity(handle, resolved, directory=expected_directory)
    finally:
        _kernel32.CloseHandle(wintypes.HANDLE(handle))


class StableReadHandle(AbstractContextManager["StableReadHandle"]):
    def __init__(self, path: str | Path) -> None:
        self.path = Path(path).resolve(strict=True)
        if not self.path.is_file():
            raise WindowsIdentityError(f"stable read requires a file: {self.path}")
        self._stream: BinaryIO | None = None
        self._native_handle: int | None = None
        if os.name == "nt":
            self._native_handle = _win_open(self.path, directory=False)
            self.identity = _win_identity(self._native_handle, self.path, directory=False)
            descriptor = msvcrt.open_osfhandle(self._native_handle, os.O_RDONLY | os.O_BINARY)
            self._native_handle = None
            self._stream = os.fdopen(descriptor, "rb", closefd=True)
        else:
            self._stream = self.path.open("rb")
            self.identity = _identity_from_stat(self.path)

    @property
    def stream(self) -> BinaryIO:
        if self._stream is None:
            raise WindowsIdentityError("stable read handle is closed")
        return self._stream

    def read(self, size: int = -1) -> bytes:
        return self.stream.read(size)

    def seek(self, offset: int, whence: int = os.SEEK_SET) -> int:
        return self.stream.seek(offset, whence)

    def close(self) -> None:
        if self._stream is not None:
            self._stream.close()
            self._stream = None
        if self._native_handle is not None and os.name == "nt":
            _kernel32.CloseHandle(wintypes.HANDLE(self._native_handle))
            self._native_handle = None

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.close()


if os.name == "nt":
    import msvcrt


def require_same_physical_object(
    expected: PhysicalFileIdentity,
    observed: PhysicalFileIdentity,
    *,
    context: str,
) -> None:
    if not expected.same_physical_object(observed):
        raise WindowsIdentityError(f"{context}: physical object was replaced")


__all__ = [
    "PhysicalFileIdentity",
    "StableReadHandle",
    "WindowsIdentityError",
    "physical_identity",
    "require_same_physical_object",
]
