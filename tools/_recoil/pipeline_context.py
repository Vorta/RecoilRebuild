from __future__ import annotations

from dataclasses import dataclass
import json
from pathlib import Path
from typing import Any

from _recoil.lib.progress import ProgressStore


@dataclass(frozen=True)
class PipelineContext:
    tracker_path: str
    tracker_revision: int
    binary: str
    phase: str
    primary_lane: str
    cursor: str
    physical_block_id: str
    complete: bool
    authored_order_prefix_end: str
    authored_function_order_prefix_end: str
    authored_object_order_prefix_end: str
    authored_object_byte_eligible_order_prefix_end: str
    authored_byte_cursor: str
    authored_byte_match_frontier: str
    full_order_prefix_end: str
    full_function_order_prefix_end: str
    linked_byte_match_prefix_end: str
    parallel_authored_byte_cursor: str
    parallel_authored_object_byte_cursor: str
    next_command: str
    raw: dict[str, Any]

    def to_dict(self) -> dict[str, Any]:
        return {
            "tracker_path": self.tracker_path,
            "tracker_revision": self.tracker_revision,
            "revision": self.tracker_revision,
            "binary": self.binary,
            "phase": self.phase,
            "primary_lane": self.primary_lane,
            "cursor": self.cursor,
            "physical_block_id": self.physical_block_id,
            "complete": self.complete,
            "authored_order_prefix_end": self.authored_order_prefix_end,
            "authored_function_order_prefix_end": self.authored_function_order_prefix_end,
            "authored_object_order_prefix_end": self.authored_object_order_prefix_end,
            "authored_object_byte_eligible_order_prefix_end": self.authored_object_byte_eligible_order_prefix_end,
            "authored_byte_cursor": self.authored_byte_cursor,
            "authored_byte_match_frontier": self.authored_byte_match_frontier,
            "full_order_prefix_end": self.full_order_prefix_end,
            "full_function_order_prefix_end": self.full_function_order_prefix_end,
            "linked_byte_match_prefix_end": self.linked_byte_match_prefix_end,
            "parallel_authored_byte_cursor": self.parallel_authored_byte_cursor,
            "parallel_authored_object_byte_cursor": self.parallel_authored_object_byte_cursor,
            "next_command": self.next_command,
            "order_prefix_end": self.full_order_prefix_end,
            "byte_prefix_end": self.authored_byte_match_frontier,
            "raw": self.raw,
        }


def load_pipeline_context(
    progress_path: Path,
    *,
    binary: str = "recoil",
) -> PipelineContext:
    document = ProgressStore(progress_path).load()
    state = document.next_work(binary)
    return PipelineContext(
        tracker_path=progress_path.as_posix(),
        tracker_revision=document.revision,
        binary=str(state.get("binary", binary)),
        phase=str(state.get("phase", "")),
        primary_lane=str(state.get("primary_lane", "")),
        cursor=str(state.get("cursor", "")),
        physical_block_id=str(state.get("physical_block_id", "")),
        complete=bool(state.get("complete", False)),
        authored_order_prefix_end=str(state.get("authored_order_prefix_end", "")),
        authored_function_order_prefix_end=str(
            state.get("authored_function_order_prefix_end", "")
        ),
        authored_object_order_prefix_end=str(
            state.get("authored_object_order_prefix_end", "")
        ),
        authored_object_byte_eligible_order_prefix_end=str(
            state.get("authored_object_byte_eligible_order_prefix_end", "")
        ),
        authored_byte_cursor=str(state.get("authored_byte_cursor", "")),
        authored_byte_match_frontier=str(state.get("authored_byte_match_frontier", "")),
        full_order_prefix_end=str(state.get("full_order_prefix_end", "")),
        full_function_order_prefix_end=str(
            state.get("full_function_order_prefix_end", "")
        ),
        linked_byte_match_prefix_end=str(state.get("linked_byte_match_prefix_end", "")),
        parallel_authored_byte_cursor=str(
            state.get("parallel_authored_byte_cursor", "")
        ),
        parallel_authored_object_byte_cursor=str(
            state.get("parallel_authored_object_byte_cursor", "")
        ),
        next_command=str(state.get("next_command", "")),
        raw=state,
    )


def render_deferred_text(payload: dict[str, Any], *, label: str) -> str:
    context = payload.get("pipeline_context", payload)
    lines = [
        f"{label} is deferred by the authoritative Recoil.exe pipeline.",
        (
            "Pipeline: "
            f"{context.get('phase')} at {context.get('cursor')} "
            f"(revision {context.get('tracker_revision')})"
        ),
    ]
    if context.get("parallel_authored_byte_cursor"):
        lines.append(
            "Parallel authored-byte cursor: "
            + str(context["parallel_authored_byte_cursor"])
        )
    if context.get("parallel_authored_object_byte_cursor"):
        lines.append(
            "Parallel authored-object-byte cursor: "
            + str(context["parallel_authored_object_byte_cursor"])
        )
    lines.append("Next: " + str(context.get("next_command", "")))
    return "\n".join(lines)


def print_deferred(payload: dict[str, Any], *, label: str, json_output: bool) -> None:
    if json_output:
        print(json.dumps(payload, indent=2))
    else:
        print(render_deferred_text(payload, label=label))
