"""Registered VC5 selectors are expected identities; object names are witnesses."""
from __future__ import annotations

import re

PREFIX = "@vc5-symbol-regex:"


def object_selector(function):
    pattern = getattr(function, "symbol_regex", None)
    if not pattern:
        return str(function.symbol)
    # A regex that spells exactly one literal is the same source identity as
    # that literal registration. Preserve existing reviewed operand bindings.
    # Only decode escaped punctuation; classes, assertions, alternatives and
    # regex escapes retain the registered pattern identity.
    literal = []
    index = 0
    metacharacters = r"\.^$*+?{}[]|()"
    while index < len(pattern):
        char = pattern[index]
        if char == "\\":
            index += 1
            if index == len(pattern) or pattern[index] not in metacharacters:
                return PREFIX + pattern
            literal.append(pattern[index])
        elif char in metacharacters:
            return PREFIX + pattern
        else:
            literal.append(char)
        index += 1
    return "".join(literal)


def registered_target_selector(bindings, object_symbols):
    """Use one registered pattern only when it covers every literal registration."""
    patterns = set()
    source_paths = set()
    for binding in bindings:
        source = str(getattr(binding, "source_from", "") or
                     getattr(binding.target, "source_from", "")).replace("\\", "/")
        if source:
            source_paths.add(source)
        pattern = getattr(binding.function, "symbol_regex", None)
        if pattern and source:
            patterns.add((pattern, source))
    if not patterns:
        return None
    if len(patterns) != 1:
        return None
    pattern, source = next(iter(patterns))
    if source_paths != {source} or any(not re.fullmatch(pattern, name) for name in object_symbols):
        return None
    return dict(symbol_regex=pattern, source_from=source)


def resolve_target_definition(obj, selector):
    """Require one definition of the registered kind in its separately selected TU."""
    from _recoil.commands.vc5_verify import resolve_coff_symbol_regex

    name = resolve_coff_symbol_regex(obj, selector["symbol_regex"],
                                    item_label=selector["source_from"])
    symbol = next(s for s in obj.symbols if s.name == name)
    if symbol.section_number <= 0 or symbol.storage_class not in {2, 3}:
        raise ValueError("registered selector requires a defined external or static symbol")
    section = obj.section(symbol.section_number)
    code = bool(section.characteristics & 0x20)
    if selector["kind"] == "function":
        if not code or symbol.type != 0x20:
            raise ValueError("registered function selector resolved to non-function storage")
    elif selector["kind"] == "data":
        if code or symbol.type == 0x20 or not section.characteristics & (0x40 | 0x80):
            raise ValueError("registered data selector resolved to non-data storage")
    else:
        raise ValueError("unsupported registered selector kind")
    return name
