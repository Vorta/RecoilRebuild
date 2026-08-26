from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Callable, Iterable


_TOKEN_RE = re.compile(
    r"[A-Za-z_][A-Za-z0-9_]*|::|\.\.\.|&&|->|[{}();,=*&#\[\]<>:~]"
)
_IDENTIFIER_RE = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_CALLING_CONVENTIONS = frozenset(
    {
        "__cdecl",
        "__fastcall",
        "__stdcall",
        "__thiscall",
        "cdecl",
        "fastcall",
        "stdcall",
    }
)
_TYPE_KEYWORDS = frozenset(
    {
        "bool",
        "char",
        "const",
        "double",
        "enum",
        "float",
        "int",
        "long",
        "mutable",
        "short",
        "signed",
        "struct",
        "typename",
        "union",
        "unsigned",
        "void",
        "volatile",
        "wchar_t",
    }
)
_CONTROL_NAMES = frozenset(
    {"catch", "do", "for", "if", "sizeof", "switch", "while"}
)
_PP_TOKEN_RE = re.compile(
    r"defined|0[xX][0-9A-Fa-f]+|[0-9]+|[A-Za-z_][A-Za-z0-9_]*|"
    r"&&|\|\||==|!=|<=|>=|[!<>()]"
)


@dataclass(frozen=True, order=True)
class CallableKey:
    qualified_name: str
    parameter_shapes: tuple[str, ...]
    const_member: bool = False

    def display(self) -> str:
        suffix = " const" if self.const_member else ""
        return (
            f"{self.qualified_name}("
            + ", ".join(self.parameter_shapes)
            + f"){suffix}"
        )


@dataclass(frozen=True)
class CallableInventory:
    declarations: frozenset[CallableKey]
    definitions: frozenset[CallableKey]


@dataclass(frozen=True)
class DecodedCallableIdentity:
    decorated_identity: str
    callable_key: CallableKey
    calling_convention: str
    parameter_bytes: int | None
    return_shape: str
    identity_format: str

    def as_dict(self) -> dict[str, object]:
        return {
            "decorated_identity": self.decorated_identity,
            "qualified_name": self.callable_key.qualified_name,
            "parameter_shapes": list(self.callable_key.parameter_shapes),
            "const_member": self.callable_key.const_member,
            "calling_convention": self.calling_convention,
            "parameter_bytes": self.parameter_bytes,
            "return_shape": self.return_shape,
            "identity_format": self.identity_format,
        }


@dataclass(frozen=True)
class DependentOwnerResolution:
    mode: str
    declaration_paths: tuple[str, ...]
    definition_paths: tuple[str, ...]
    source_edit_paths: tuple[str, ...]
    candidate_independent: bool = False
    ambiguity_policy: str = "fail-closed"

    def as_dict(self) -> dict[str, object]:
        return {
            "kind": "call-contract-dependent-owner-resolution",
            "contract_version": 1,
            "mode": self.mode,
            "declaration_paths": list(self.declaration_paths),
            "definition_paths": list(self.definition_paths),
            "source_edit_paths": list(self.source_edit_paths),
            "candidate_independent": self.candidate_independent,
            "ambiguity_policy": self.ambiguity_policy,
        }


@dataclass(frozen=True)
class DefinitionSourceResolution:
    """Typed candidate-independent definition-TU resolution result."""

    mode: str
    source_paths: tuple[str, ...]
    reviewed_target_identities: tuple[str, ...]
    unresolved_target_identities: tuple[str, ...]
    candidate_independent: bool = True
    ambiguity_policy: str = "fail-closed"

    def as_dict(self) -> dict[str, object]:
        return {
            "kind": "call-contract-definition-source-resolution",
            "contract_version": 2,
            "mode": self.mode,
            "candidate_independent": self.candidate_independent,
            "ambiguity_policy": self.ambiguity_policy,
            "reviewed_target_identities": list(
                self.reviewed_target_identities
            ),
            "exact_reviewed_identity_count": (
                len(self.reviewed_target_identities)
                - len(self.unresolved_target_identities)
            ),
            "unresolved_target_identities": list(
                self.unresolved_target_identities
            ),
            "fallback_path_count": (
                len(self.source_paths)
                if self.mode == "conservative-full-closure"
                else 0
            ),
        }


CallableInventoryResolver = Callable[
    [str, tuple[str, ...]], CallableInventory
]


_VC5_CPP_ZERO_ARGUMENT_RE = re.compile(
    r"^\?(?P<body>[A-Za-z_][A-Za-z0-9_]*(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@Y(?P<calling>[AGI])(?P<return>PAD|H|X)XZ$"
)
_VC5_C_FASTCALL_ZERO_ARGUMENT_RE = re.compile(
    r"^@(?P<name>[A-Za-z_][A-Za-z0-9_]*)@(?P<bytes>[0-9]+)$"
)
_VC5_CPP_ZERO_ARGUMENT_LIFECYCLE_RE = re.compile(
    r"^\?\?(?P<kind>[01])"
    r"(?P<body>[A-Za-z_][A-Za-z0-9_]*(?:@[A-Za-z_][A-Za-z0-9_]*)*)"
    r"@@(?P<attributes>[A-Z0-9?]+)@XZ$"
)


def decode_vc5_zero_argument_callable_identity(
    decorated_identity: str,
) -> DecodedCallableIdentity | None:
    """Decode only exact VC5 zero-argument identities used for owner routing."""

    identity = str(decorated_identity)
    cpp_match = _VC5_CPP_ZERO_ARGUMENT_RE.fullmatch(identity)
    if cpp_match is not None:
        calling_conventions = {"A": "__cdecl", "G": "__stdcall", "I": "__fastcall"}
        # Keep this table deliberately small: these are the exact source-level
        # scalar shapes used by dependent-owner routing, not a general MSVC
        # type decoder.
        return_shapes = {"H": "int", "PAD": "char *", "X": "void"}
        calling_code = cpp_match.group("calling")
        return_code = cpp_match.group("return")
        calling = calling_conventions.get(calling_code)
        return_shape = return_shapes.get(return_code)
        # The bounded PAD route exists only for the observed cdecl/fastcall
        # zero-argument pair.  Do not grow this into a general MSVC pointer
        # decoder or admit a stdcall shape without reviewed evidence.
        if return_code == "PAD" and calling_code not in {"A", "I"}:
            return None
        if calling is None or return_shape is None:
            return None
        parts = cpp_match.group("body").split("@")
        qualified_name = "::".join((*reversed(parts[1:]), parts[0]))
        return DecodedCallableIdentity(
            decorated_identity=identity,
            callable_key=CallableKey(qualified_name, ()),
            calling_convention=calling,
            parameter_bytes=0,
            return_shape=return_shape,
            identity_format="msvc-cpp-global",
        )
    c_match = _VC5_C_FASTCALL_ZERO_ARGUMENT_RE.fullmatch(identity)
    if c_match is None or int(c_match.group("bytes")) != 0:
        return None
    return DecodedCallableIdentity(
        decorated_identity=identity,
        callable_key=CallableKey(c_match.group("name"), ()),
        calling_convention="__fastcall",
        parameter_bytes=0,
        return_shape="unknown-not-encoded",
        identity_format="msvc-c-fastcall",
    )


def decode_vc5_zero_argument_lifecycle_identity(
    decorated_identity: str,
) -> DecodedCallableIdentity | None:
    """Decode one exact ordinary VC5 constructor/destructor body identity.

    This intentionally excludes deleting destructors and parameterized
    lifecycle variants.  The result is used only to join a registered body or
    a candidate COMDAT to the conservative source inventory; it does not grant
    retail identity, ownership, or expected-call authority.
    """

    identity = str(decorated_identity)
    match = _VC5_CPP_ZERO_ARGUMENT_LIFECYCLE_RE.fullmatch(identity)
    if match is None:
        return None
    parts = match.group("body").split("@")
    class_name = parts[0]
    enclosing = tuple(reversed(parts[1:]))
    terminal = class_name if match.group("kind") == "0" else f"~{class_name}"
    return DecodedCallableIdentity(
        decorated_identity=identity,
        callable_key=CallableKey(
            "::".join((*enclosing, class_name, terminal)),
            (),
        ),
        calling_convention="__thiscall",
        parameter_bytes=0,
        return_shape="void",
        identity_format=(
            "msvc-cpp-complete-constructor"
            if match.group("kind") == "0"
            else "msvc-cpp-complete-destructor"
        ),
    )


def resolve_dependent_callable_owner(
    *,
    callable_key: CallableKey,
    header_texts: Iterable[tuple[str, str, Iterable[str]]],
    source_texts: Iterable[tuple[str, str, Iterable[str]]],
    inventory_resolver: CallableInventoryResolver | None = None,
) -> DependentOwnerResolution:
    """Resolve exactly one declaration header and one definition source."""

    def inventory(path: str, text: str, defines: Iterable[str]) -> CallableInventory:
        context = tuple(str(value) for value in defines)
        return (
            inventory_resolver(path, context)
            if inventory_resolver is not None
            else callable_inventory(text, defines=context)
        )

    declaration_paths = tuple(
        sorted(
            {
                path
                for path, text, defines in header_texts
                if callable_key in inventory(path, text, defines).declarations
            },
            key=lambda path: (path.casefold(), path),
        )
    )
    definition_paths = tuple(
        sorted(
            {
                path
                for path, text, defines in source_texts
                if callable_key in inventory(path, text, defines).definitions
            },
            key=lambda path: (path.casefold(), path),
        )
    )
    mode = (
        "exact"
        if len(declaration_paths) == 1 and len(definition_paths) == 1
        else (
            "ambiguous"
            if len(declaration_paths) > 1 or len(definition_paths) > 1
            else "unresolved"
        )
    )
    source_edit_paths = (
        tuple(sorted({*declaration_paths, *definition_paths}, key=lambda path: (path.casefold(), path)))
        if mode == "exact"
        else ()
    )
    return DependentOwnerResolution(
        mode=mode,
        declaration_paths=declaration_paths,
        definition_paths=definition_paths,
        source_edit_paths=source_edit_paths,
    )


def _macro_values(defines: Iterable[str]) -> dict[str, int]:
    result: dict[str, int] = {}
    for raw in defines:
        name, separator, raw_value = str(raw).partition("=")
        if not _IDENTIFIER_RE.fullmatch(name):
            raise ValueError(f"invalid call-contract definition macro: {raw!r}")
        try:
            value = int(raw_value, 0) if separator and raw_value else 1
        except ValueError:
            value = 1
        result[name] = value
    return result


class _PreprocessorExpression:
    def __init__(self, expression: str, macros: dict[str, int]) -> None:
        compact = re.sub(r"\s+", "", expression)
        tokens = _PP_TOKEN_RE.findall(expression)
        if "".join(tokens) != compact:
            raise ValueError(
                "unsupported call-contract preprocessor expression: "
                + expression.strip()
            )
        self.tokens = tokens
        self.macros = macros
        self.index = 0

    def _peek(self) -> str:
        return self.tokens[self.index] if self.index < len(self.tokens) else ""

    def _take(self, value: str | None = None) -> str:
        token = self._peek()
        if not token or (value is not None and token != value):
            raise ValueError("malformed call-contract preprocessor expression")
        self.index += 1
        return token

    def _primary(self) -> int:
        token = self._peek()
        if token == "defined":
            self._take()
            parenthesized = self._peek() == "("
            if parenthesized:
                self._take("(")
            name = self._take()
            if not _IDENTIFIER_RE.fullmatch(name):
                raise ValueError("defined() requires one macro identifier")
            if parenthesized:
                self._take(")")
            return int(name in self.macros)
        if token == "(":
            self._take("(")
            value = self._or()
            self._take(")")
            return value
        self._take()
        if _IDENTIFIER_RE.fullmatch(token):
            return self.macros.get(token, 0)
        return int(token, 0)

    def _unary(self) -> int:
        if self._peek() == "!":
            self._take("!")
            return int(not self._unary())
        return self._primary()

    def _comparison(self) -> int:
        left = self._unary()
        operator = self._peek()
        if operator not in {"==", "!=", "<", ">", "<=", ">="}:
            return left
        self._take()
        right = self._unary()
        return int(
            {
                "==": left == right,
                "!=": left != right,
                "<": left < right,
                ">": left > right,
                "<=": left <= right,
                ">=": left >= right,
            }[operator]
        )

    def _and(self) -> int:
        value = self._comparison()
        while self._peek() == "&&":
            self._take("&&")
            right = self._comparison()
            value = int(bool(value) and bool(right))
        return value

    def _or(self) -> int:
        value = self._and()
        while self._peek() == "||":
            self._take("||")
            right = self._and()
            value = int(bool(value) or bool(right))
        return value

    def evaluate(self) -> bool:
        value = self._or()
        if self.index != len(self.tokens):
            raise ValueError("malformed call-contract preprocessor expression")
        return bool(value)


def _mask_inactive_branches(text: str, defines: Iterable[str]) -> str:
    macros = _macro_values(defines)
    output: list[str] = []
    # parent-active, a prior branch was selected, current-active
    frames: list[tuple[bool, bool, bool]] = []
    active = True
    lines = text.splitlines(keepends=True)
    line_index = 0
    while line_index < len(lines):
        line_number = line_index + 1
        line = lines[line_index]
        stripped = line.lstrip()
        if not stripped.startswith("#"):
            output.append(line if active else re.sub(r"[^\r\n]", " ", line))
            line_index += 1
            continue
        physical_lines = [line]
        directive_parts = [stripped[1:].rstrip("\r\n")]
        while directive_parts[-1].rstrip().endswith("\\"):
            directive_parts[-1] = directive_parts[-1].rstrip()[:-1]
            line_index += 1
            if line_index >= len(lines):
                raise ValueError(
                    "unterminated continued preprocessor directive in the "
                    f"call-contract definition closure at line {line_number}"
                )
            continuation = lines[line_index]
            physical_lines.append(continuation)
            directive_parts.append(continuation.rstrip("\r\n"))
        directive_text = " ".join(directive_parts).strip()
        match = re.match(r"(?P<directive>[A-Za-z_][A-Za-z0-9_]*)\b(?P<operand>.*)", directive_text)
        directive = match.group("directive") if match is not None else ""
        operand = match.group("operand").strip() if match is not None else ""
        if directive in {"if", "ifdef", "ifndef"}:
            parent = active
            if directive == "ifdef":
                selected = operand in macros
            elif directive == "ifndef":
                selected = operand not in macros
            else:
                selected = _PreprocessorExpression(operand, macros).evaluate()
            current = parent and selected
            frames.append((parent, bool(selected), current))
            active = current
        elif directive == "elif":
            if not frames:
                raise ValueError("unmatched #elif in call-contract definition closure")
            parent, taken, _current = frames[-1]
            selected = (not taken) and _PreprocessorExpression(
                operand, macros
            ).evaluate()
            current = parent and selected
            frames[-1] = (parent, taken or selected, current)
            active = current
        elif directive == "else":
            if not frames:
                raise ValueError("unmatched #else in call-contract definition closure")
            parent, taken, _current = frames[-1]
            selected = not taken
            current = parent and selected
            frames[-1] = (parent, True, current)
            active = current
        elif directive == "endif":
            if not frames:
                raise ValueError("unmatched #endif in call-contract definition closure")
            frames.pop()
            active = frames[-1][2] if frames else True
        elif directive == "define" and active:
            name, _separator, raw_value = operand.partition(" ")
            if _IDENTIFIER_RE.fullmatch(name):
                try:
                    macros[name] = int(raw_value.strip(), 0) if raw_value.strip() else 1
                except ValueError:
                    macros[name] = 1
        elif directive == "undef" and active:
            macros.pop(operand, None)
        output.extend(re.sub(r"[^\r\n]", " ", row) for row in physical_lines)
        line_index += 1
    if frames:
        raise ValueError("unterminated #if in call-contract definition closure")
    return "".join(output)


def _mask_non_code(text: str, defines: Iterable[str]) -> str:
    """Mask comments, literals, and preprocessor rows while preserving braces."""

    chars = list(_mask_inactive_branches(text, defines))
    index = 0
    line_start = True
    while index < len(chars):
        character = chars[index]
        if line_start:
            cursor = index
            while cursor < len(chars) and chars[cursor] in " \t\r":
                cursor += 1
            if cursor < len(chars) and chars[cursor] == "#":
                while cursor < len(chars):
                    if chars[cursor] == "\n":
                        # A continued directive remains masked on the next row.
                        prior = cursor - 1
                        while prior >= index and chars[prior] in " \t\r":
                            prior -= 1
                        continued = prior >= index and chars[prior] == "\\"
                        if not continued:
                            break
                    if chars[cursor] != "\n":
                        chars[cursor] = " "
                    cursor += 1
                index = cursor
                line_start = True
                continue
        if character == "\n":
            line_start = True
            index += 1
            continue
        line_start = False
        if character == "/" and index + 1 < len(chars):
            following = chars[index + 1]
            if following == "/":
                chars[index] = chars[index + 1] = " "
                index += 2
                while index < len(chars) and chars[index] != "\n":
                    chars[index] = " "
                    index += 1
                continue
            if following == "*":
                chars[index] = chars[index + 1] = " "
                index += 2
                while index + 1 < len(chars):
                    if chars[index] == "*" and chars[index + 1] == "/":
                        chars[index] = chars[index + 1] = " "
                        index += 2
                        break
                    if chars[index] != "\n":
                        chars[index] = " "
                    index += 1
                continue
        if character in {'"', "'"}:
            quote = character
            chars[index] = " "
            index += 1
            while index < len(chars):
                current = chars[index]
                if current == "\\" and index + 1 < len(chars):
                    chars[index] = chars[index + 1] = " "
                    index += 2
                    continue
                if current == quote:
                    chars[index] = " "
                    index += 1
                    break
                if current != "\n":
                    chars[index] = " "
                index += 1
            continue
        index += 1
    return "".join(chars)


def _split_parameters(tokens: list[str]) -> list[list[str]] | None:
    if not tokens or tokens == ["void"]:
        return []
    result: list[list[str]] = []
    current: list[str] = []
    paren = bracket = angle = 0
    for token in tokens:
        if token == "(":
            paren += 1
        elif token == ")":
            paren -= 1
        elif token == "[":
            bracket += 1
        elif token == "]":
            bracket -= 1
        elif token == "<":
            angle += 1
        elif token == ">" and angle:
            angle -= 1
        if min(paren, bracket, angle) < 0:
            return None
        if token == "," and not paren and not bracket and not angle:
            if not current:
                return None
            result.append(current)
            current = []
        else:
            current.append(token)
    if paren or bracket or angle or not current:
        return None
    result.append(current)
    return result


def _without_default(tokens: list[str]) -> list[str]:
    paren = bracket = angle = 0
    for index, token in enumerate(tokens):
        if token == "(":
            paren += 1
        elif token == ")":
            paren -= 1
        elif token == "[":
            bracket += 1
        elif token == "]":
            bracket -= 1
        elif token == "<":
            angle += 1
        elif token == ">" and angle:
            angle -= 1
        elif token == "=" and not paren and not bracket and not angle:
            return tokens[:index]
    return tokens


def _parameter_shape(raw_tokens: list[str]) -> str | None:
    tokens = [
        token
        for token in _without_default(raw_tokens)
        if token not in _CALLING_CONVENTIONS
    ]
    if not tokens:
        return None

    # Remove a function-pointer parameter name from ``(*name)``.
    for index in range(2, len(tokens) - 1):
        if (
            tokens[index - 2] == "("
            and tokens[index - 1] in {"*", "&", "&&"}
            and _IDENTIFIER_RE.fullmatch(tokens[index])
            and tokens[index + 1] == ")"
        ):
            del tokens[index]
            break

    name_index: int | None = None
    if len(tokens) >= 2:
        for index in range(len(tokens) - 1):
            if (
                _IDENTIFIER_RE.fullmatch(tokens[index])
                and tokens[index + 1] == "["
                and (index == 0 or tokens[index - 1] != "::")
            ):
                name_index = index
        last = tokens[-1]
        if (
            name_index is None
            and _IDENTIFIER_RE.fullmatch(last)
            and last not in _TYPE_KEYWORDS
            and tokens[-2] != "::"
        ):
            prior = tokens[:-1]
            prior_identifiers = [
                token
                for token in prior
                if _IDENTIFIER_RE.fullmatch(token)
                and token not in {"const", "volatile", "struct", "class", "enum"}
            ]
            if prior_identifiers or any(
                token in {"*", "&", "&&", "]"} for token in prior
            ):
                name_index = len(tokens) - 1
    if name_index is not None:
        del tokens[name_index]
    return " ".join(tokens)


def _matching_close_paren(tokens: list[str], open_index: int) -> int | None:
    depth = 0
    for index in range(open_index, len(tokens)):
        if tokens[index] == "(":
            depth += 1
        elif tokens[index] == ")":
            depth -= 1
            if depth == 0:
                return index
    return None


def _callable_key(
    tokens: list[str],
    scope_names: tuple[str, ...],
) -> CallableKey | None:
    if not tokens or "typedef" in tokens:
        return None
    paren_depth = 0
    open_index: int | None = None
    for index, token in enumerate(tokens):
        if token == "(":
            if paren_depth == 0:
                prior = tokens[index - 1] if index else ""
                if _IDENTIFIER_RE.fullmatch(prior):
                    open_index = index
                    break
            paren_depth += 1
        elif token == ")" and paren_depth:
            paren_depth -= 1
    if open_index is None:
        return None
    close_index = _matching_close_paren(tokens, open_index)
    if close_index is None:
        return None

    cursor = open_index - 1
    terminal = tokens[cursor]
    if terminal in _CONTROL_NAMES:
        return None
    if cursor and tokens[cursor - 1] == "~":
        terminal = "~" + terminal
        cursor -= 1
    parts = [terminal]
    absolute = False
    while cursor >= 2 and tokens[cursor - 1] == "::":
        preceding = tokens[cursor - 2]
        if not _IDENTIFIER_RE.fullmatch(preceding):
            break
        parts.insert(0, preceding)
        cursor -= 2
    if cursor >= 1 and tokens[cursor - 1] == "::":
        absolute = True
    if "=" in tokens[:open_index]:
        return None

    raw_parameters = _split_parameters(tokens[open_index + 1 : close_index])
    if raw_parameters is None:
        return None
    parameter_shapes: list[str] = []
    for raw_parameter in raw_parameters:
        shape = _parameter_shape(raw_parameter)
        if shape is None:
            return None
        parameter_shapes.append(shape)

    explicit_parts = tuple(parts)
    if absolute:
        qualified_parts = explicit_parts
    elif len(explicit_parts) > 1:
        common = 0
        while (
            common < len(scope_names)
            and common < len(explicit_parts)
            and scope_names[common] == explicit_parts[common]
        ):
            common += 1
        qualified_parts = (*scope_names, *explicit_parts[common:])
    else:
        qualified_parts = (*scope_names, *explicit_parts)
    if not qualified_parts:
        return None
    trailing = tokens[close_index + 1 :]
    return CallableKey(
        qualified_name="::".join(qualified_parts),
        parameter_shapes=tuple(parameter_shapes),
        const_member="const" in trailing,
    )


def callable_inventory(
    text: str,
    *,
    defines: Iterable[str] = (),
) -> CallableInventory:
    """Extract a conservative C/C++ callable declaration/definition inventory.

    The parser intentionally recognizes only ordinary namespace/class functions,
    constructors, and destructors. Unsupported operator/macro declarators do not
    produce edges; duplicate source ownership for a recognized key is handled by
    the closure caller as an ambiguity.
    """

    tokens = _TOKEN_RE.findall(_mask_non_code(text, defines))
    frames: list[tuple[str, str]] = []
    segment: list[str] = []
    declarations: set[CallableKey] = set()
    definitions: set[CallableKey] = set()

    def in_function() -> bool:
        return any(kind == "function" for kind, _name in frames)

    def scope_names() -> tuple[str, ...]:
        return tuple(
            name
            for kind, name in frames
            if kind in {"namespace", "class"} and name
        )

    for token in tokens:
        if token == ";":
            if not in_function():
                key = _callable_key(segment, scope_names())
                if key is not None:
                    declarations.add(key)
            segment = []
            continue
        if token == "{":
            if in_function():
                frames.append(("other", ""))
            else:
                key = _callable_key(segment, scope_names())
                if key is not None:
                    definitions.add(key)
                    frames.append(("function", key.qualified_name))
                elif "namespace" in segment:
                    index = (
                        len(segment)
                        - 1
                        - segment[::-1].index("namespace")
                    )
                    name = (
                        segment[index + 1]
                        if index + 1 < len(segment)
                        and _IDENTIFIER_RE.fullmatch(segment[index + 1])
                        else ""
                    )
                    frames.append(("namespace", name))
                elif (
                    "class" in segment
                    or "struct" in segment
                    or "union" in segment
                ):
                    indexes = [
                        index
                        for index, value in enumerate(segment)
                        if value in {"class", "struct", "union"}
                    ]
                    index = indexes[-1]
                    name = (
                        segment[index + 1]
                        if index + 1 < len(segment)
                        and _IDENTIFIER_RE.fullmatch(segment[index + 1])
                        else ""
                    )
                    frames.append(("class", name))
                else:
                    frames.append(("other", ""))
            segment = []
            continue
        if token == "}":
            if frames:
                frames.pop()
            segment = []
            continue
        segment.append(token)

    return CallableInventory(
        declarations=frozenset(declarations),
        definitions=frozenset(definitions),
    )


def exact_definition_sources(
    *,
    header_texts: Iterable[tuple[str, str, Iterable[str]]],
    source_texts: Iterable[tuple[str, str, Iterable[str]]],
    inventory_resolver: CallableInventoryResolver | None = None,
) -> tuple[str, ...]:
    """Resolve the unique source TU for every recognized non-inline declaration.

    Direct callers retain text-based parsing.  A request-local owner may supply
    ``inventory_resolver`` to reuse an already parsed inventory for the exact
    path and preprocessor context; when supplied, that resolver owns content
    loading and the text tuple field is intentionally ignored.
    """

    def resolve_inventory(
        path: str,
        text: str,
        defines: Iterable[str],
    ) -> CallableInventory:
        context = tuple(str(value) for value in defines)
        if inventory_resolver is not None:
            return inventory_resolver(path, context)
        return callable_inventory(text, defines=context)

    declarations: set[CallableKey] = set()
    inline_definitions: set[CallableKey] = set()
    for path, text, defines in header_texts:
        inventory = resolve_inventory(path, text, defines)
        declarations.update(inventory.declarations)
        inline_definitions.update(inventory.definitions)
    unresolved = declarations - inline_definitions
    unresolved_names = {key.qualified_name for key in unresolved}

    sources_by_key: dict[CallableKey, set[str]] = {}
    sources_by_name: dict[str, set[str]] = {}
    for path, text, defines in source_texts:
        for key in resolve_inventory(path, text, defines).definitions:
            if key in unresolved:
                sources_by_key.setdefault(key, set()).add(path)
            if key.qualified_name in unresolved_names:
                sources_by_name.setdefault(key.qualified_name, set()).add(path)

    ambiguities = {
        key: paths for key, paths in sources_by_key.items() if len(paths) > 1
    }
    if ambiguities:
        details = "; ".join(
            f"{key.display()} -> {', '.join(sorted(paths))}"
            for key, paths in sorted(ambiguities.items())
        )
        raise ValueError(
            "call-contract definition closure has ambiguous source ownership: "
            + details
        )
    return tuple(
        sorted(
            {
                next(iter(paths))
                for paths in sources_by_key.values()
                if len(paths) == 1
            }
            | {
                path
                for paths in sources_by_name.values()
                for path in paths
            },
            key=lambda path: (path.casefold(), path),
        )
    )


def resolve_reviewed_definition_sources(
    *,
    reviewed_callables: Iterable[tuple[str, CallableKey]],
    header_texts: Iterable[tuple[str, str, Iterable[str]]],
    source_texts: Iterable[tuple[str, str, Iterable[str]]],
    inventory_resolver: CallableInventoryResolver | None = None,
) -> DefinitionSourceResolution:
    """Resolve reviewed call-target identities to exact definition TUs.

    A reviewed identity plus exact callable shape is candidate-independent
    routing input. Every supplied identity must resolve to exactly one source
    TU. Duplicate definitions are an ambiguity and fail closed. Missing
    identities do not get guessed from spelling; they trigger the typed,
    conservative full header-declaration closure instead.
    """

    reviewed_rows = tuple(
        (str(identity), key) for identity, key in reviewed_callables
    )
    identities = tuple(identity for identity, _key in reviewed_rows)
    if any(not identity for identity in identities):
        raise ValueError(
            "call-contract reviewed target identities must be non-empty"
        )
    if len(set(identities)) != len(identities):
        raise ValueError(
            "call-contract reviewed target identities must be unique"
        )

    header_rows = tuple(header_texts)
    source_rows = tuple(source_texts)

    def resolve_inventory(
        path: str,
        text: str,
        defines: Iterable[str],
    ) -> CallableInventory:
        context = tuple(str(value) for value in defines)
        if inventory_resolver is not None:
            return inventory_resolver(path, context)
        return callable_inventory(text, defines=context)

    sources_by_key: dict[CallableKey, set[str]] = {
        key: set() for _identity, key in reviewed_rows
    }
    for path, text, defines in source_rows:
        definitions = resolve_inventory(path, text, defines).definitions
        for key in sources_by_key:
            if key in definitions:
                sources_by_key[key].add(path)

    ambiguous = {
        identity: sources_by_key[key]
        for identity, key in reviewed_rows
        if len(sources_by_key[key]) > 1
    }
    if ambiguous:
        details = "; ".join(
            f"{identity} -> {', '.join(sorted(paths))}"
            for identity, paths in sorted(ambiguous.items())
        )
        raise ValueError(
            "call-contract reviewed definition identity has ambiguous source "
            "ownership: "
            + details
        )

    unresolved = tuple(
        identity
        for identity, key in reviewed_rows
        if not sources_by_key[key]
    )
    if not reviewed_rows or unresolved:
        fallback = exact_definition_sources(
            header_texts=header_rows,
            source_texts=source_rows,
            inventory_resolver=inventory_resolver,
        )
        return DefinitionSourceResolution(
            mode="conservative-full-closure",
            source_paths=fallback,
            reviewed_target_identities=identities,
            unresolved_target_identities=(unresolved or identities),
        )

    return DefinitionSourceResolution(
        mode="reviewed-call-edge-exact",
        source_paths=tuple(
            sorted(
                {
                    next(iter(sources_by_key[key]))
                    for _identity, key in reviewed_rows
                },
                key=lambda path: (path.casefold(), path),
            )
        ),
        reviewed_target_identities=identities,
        unresolved_target_identities=(),
    )
