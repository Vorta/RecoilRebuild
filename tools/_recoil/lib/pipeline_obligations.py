"""Recoil acceptance dimensions and their exact subjects.

An effect describes a command's authority, not a receipt or a claim that current
source passes. Registration, replacement and invalidation do not satisfy an
obligation to accept an existing authored entity.
"""

from __future__ import annotations

from dataclasses import dataclass

from _recoil.lib.progress import (
    AUTHORED_BYTE_DIMENSIONS, AUTHORED_ORDER_DIMENSIONS,
    CALL_CONTRACT_DIMENSION, EXACT_LINK_DIMENSIONS, FULL_ORDER_DIMENSIONS,
    OWNER_GATES, STORAGE_DIMENSIONS, TIERS,
)


@dataclass(frozen=True, order=True)
class AcceptanceEffect:
    subject: str
    dimension: str
    transition: str = "accept"

    @property
    def key(self) -> str:
        return f"{self.subject}/{self.dimension}/{self.transition}"


def effects(subject: str, dimensions: tuple[str, ...],
            transition: str = "accept") -> tuple[AcceptanceEffect, ...]:
    return tuple(AcceptanceEffect(subject, item, transition) for item in dimensions)


AUTHORED_ORDER_EFFECTS = effects("authored-block", AUTHORED_ORDER_DIMENSIONS)
FULL_ORDER_EFFECTS = effects("full-block", FULL_ORDER_DIMENSIONS)
AUTHORED_BYTE_EFFECTS = effects("authored-function", AUTHORED_BYTE_DIMENSIONS + ("object_instruction", "linked_body_instruction", "object_commutative", "linked_body_commutative"))
LINKED_BYTE_EFFECTS = effects("selected-linked-function", EXACT_LINK_DIMENSIONS + ("linked_instruction", "linked_commutative"))
CALL_EFFECTS = effects("authored-function", (CALL_CONTRACT_DIMENSION,))
CLOSEOUT_EFFECTS = effects("authored-call-census", ("fresh-scan-and-linkability",))
FINAL_EFFECTS = effects("typed-final-image", ("complete-live-comparison",), "verify")
STORAGE_EFFECTS = effects("authored-storage", STORAGE_DIMENSIONS)
OWNER_EFFECTS = effects("existing-authored-owner", OWNER_GATES)
TIER_EFFECTS = effects("existing-authored-owner-tier", TIERS[1:], "promote")
PROVIDER_IMPORT_EFFECTS = (effects("provider-import-storage", ("extent",))
                           + effects("provider-import-owner", ("boundary", "source", "data")))


def completion_obligations() -> tuple[tuple[str, AcceptanceEffect, str], ...]:
    """Use the same dimensions consumed by scheduler, storage and owner gates."""
    rows: list[tuple[str, AcceptanceEffect, str]] = []
    for stage, group in (
        ("authored-function-order", AUTHORED_ORDER_EFFECTS),
        ("authored-call-contract", CALL_EFFECTS + CLOSEOUT_EFFECTS),
        ("authored-byte-match", AUTHORED_BYTE_EFFECTS),
        ("full-function-order", FULL_ORDER_EFFECTS),
        ("linked-byte-match", LINKED_BYTE_EFFECTS),
        ("final-validation", FINAL_EFFECTS),
    ):
        rows.extend((stage, effect, "progress.ProgressDocument.pipeline") for effect in group)
    rows.extend(("final-validation", effect, "final_image_coverage._storage_accepted")
                for effect in effects("authored-storage", STORAGE_DIMENSIONS))
    rows.extend(("owner-acceptance", effect, "progress.validate_owner_invariants")
                for effect in effects("existing-authored-owner", OWNER_GATES))
    rows.extend(("owner-acceptance", effect, "SourceOwnerDocument.validate")
                for effect in effects("existing-authored-owner-tier", TIERS[1:], "promote"))
    return tuple(rows)
