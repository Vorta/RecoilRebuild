from __future__ import annotations

from collections.abc import Iterable

from _recoil.lib.owner_entries import normalize_address
from _recoil.lib.reference_images import reference_image, reference_image_keys


DEFAULT_TARGET_BINARY = "recoil"


def _context_prefix(context: str) -> str:
    return f"{context}: " if context else ""


def _source_binary(source_from: str) -> str | None:
    normalized = source_from.replace("\\", "/").strip().strip("/").lower()
    if not normalized:
        return None

    matches: list[tuple[int, str]] = []
    for key in reference_image_keys():
        source_root = reference_image(key).source_root.replace("\\", "/").strip().strip("/").lower()
        if normalized == source_root or normalized.startswith(source_root + "/"):
            matches.append((len(source_root), key))
    if not matches:
        return None

    longest = max(length for length, _key in matches)
    longest_keys = {key for length, key in matches if length == longest}
    if len(longest_keys) != 1:
        return None
    return next(iter(longest_keys))


def _address_binary(address: str) -> str | None:
    value = int(normalize_address(address), 16)
    image_bases = sorted(
        ((reference_image(key).image_base, key) for key in reference_image_keys()),
        reverse=True,
    )
    for image_base, key in image_bases:
        if value >= image_base:
            return key
    return None


def validated_target_binary(
    *,
    source_from: str,
    addresses: Iterable[str],
    explicit: str | None = None,
    context: str = "",
) -> str:
    prefix = _context_prefix(context)
    normalized_explicit = str(explicit).strip() if explicit is not None else ""
    if normalized_explicit and normalized_explicit not in reference_image_keys():
        valid = ", ".join(reference_image_keys())
        raise ValueError(
            f"{prefix}unknown target_binary {normalized_explicit!r}; expected one of: {valid}"
        )

    source_binary = _source_binary(source_from)
    address_binaries: set[str] = set()
    normalized_addresses: list[str] = []
    for address in addresses:
        normalized = normalize_address(address)
        normalized_addresses.append(normalized)
        inferred = _address_binary(normalized)
        if inferred is None:
            raise ValueError(f"{prefix}address {normalized} does not belong to a configured target binary")
        address_binaries.add(inferred)

    if len(address_binaries) > 1:
        formatted = ", ".join(normalized_addresses)
        raise ValueError(f"{prefix}cross-binary addresses are not allowed: {formatted}")

    address_binary = next(iter(address_binaries), None)
    if source_binary is not None and address_binary is not None and source_binary != address_binary:
        raise ValueError(
            f"{prefix}cross-binary source/address mismatch: source {source_from!r} resolves to "
            f"{source_binary}, addresses resolve to {address_binary}"
        )

    inferred_binary = address_binary or source_binary or DEFAULT_TARGET_BINARY
    if normalized_explicit and normalized_explicit != inferred_binary:
        raise ValueError(
            f"{prefix}target_binary {normalized_explicit!r} conflicts with inferred binary "
            f"{inferred_binary!r}"
        )
    return normalized_explicit or inferred_binary
