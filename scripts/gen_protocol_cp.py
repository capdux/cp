#!/usr/bin/env python3
"""
gen_protocol_cp.py — Absorb Chromium CDP & WebKit protocol.d.ts into strongly-typed CP modules.

Usage:
    python3 scripts/gen_protocol_cp.py
"""

from __future__ import annotations

import os
import re
import sys
from collections import defaultdict
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, List, Optional, Set, Tuple

ROOT = Path("/data/dev/cp")
SYNC_DATE = "2026-11-28"

# Exact CP lexer keywords + Zig reserved keywords that cannot be raw struct field identifiers
RESERVED_IDENTIFIERS: Set[str] = {
    # CP lexer keywords (from cpc/src/lexer.rs)
    "let", "var", "threadlocal", "const", "match", "switch", "try", "catch",
    "null", "dyn", "if", "else", "while", "return", "break", "continue",
    "mut", "move", "own", "ref", "struct", "static", "unsafe", "super",
    "meta", "dec", "macro", "export", "pub", "priv", "private", "undefined",
    "import", "from", "type", "as", "async", "await", "fn", "class",
    "union", "trait", "impl", "for", "of", "in", "enum", "interface",
    "extends", "implements", "new", "error", "err", "Error", "Err",
    "defer", "errdefer", "arena", "fuzz", "void", "any", "auto", "anytype",
    # CP primitive type tokens
    "u8", "i8", "u16", "i16", "i32", "u32", "i64", "u64", "i128", "u128",
    "f32", "f64", "isize", "usize", "bool", "string", "cstr",
    # Zig / boolean / operator reserved words
    "true", "false", "and", "or", "orelse", "not", "inline", "comptime",
    "extern", "packed", "align", "opaque", "test", "usingnamespace",
    "nosuspend", "suspend", "resume", "unreachable", "asm", "volatile",
    "allowzero", "linksection", "callconv", "noalias", "self", "Self",
}


@dataclass
class FieldDef:
    name: str
    optional: bool
    ts_type: str
    doc: List[str] = field(default_factory=list)


@dataclass
class TypeAliasDef:
    domain: str
    name: str
    ts_rhs: str
    doc: List[str] = field(default_factory=list)


@dataclass
class StructDef:
    domain: str
    name: str
    kind: str  # "interface" or "type_obj"
    fields: List[FieldDef] = field(default_factory=list)
    doc: List[str] = field(default_factory=list)


@dataclass
class DomainDef:
    name: str
    doc: List[str] = field(default_factory=list)
    items: List[object] = field(default_factory=list)  # TypeAliasDef or StructDef


@dataclass
class ProtocolSpec:
    domains: List[DomainDef]
    events: List[Tuple[str, str]]  # ("Domain.event", "Domain.eventPayload")
    commands: List[str]  # ["Domain.method", ...]


def clean_doc_lines(raw_lines: List[str]) -> List[str]:
    out: List[str] = []
    for line in raw_lines:
        s = line.strip()
        if s.startswith("/**"):
            s = s[3:].strip()
        if s.endswith("*/"):
            s = s[:-2].strip()
        if s.startswith("*"):
            s = s[1:].strip()
        if s:
            # Collapse newlines / avoid stray comment terminators
            s = s.replace("*/", "* /")
            out.append(s)
    return out


def parse_protocol_dts(path: Path) -> ProtocolSpec:
    lines = path.read_text(encoding="utf-8").splitlines()
    domains: List[DomainDef] = []
    events: List[Tuple[str, str]] = []
    commands: List[str] = []

    i = 0
    n = len(lines)
    pending_doc: List[str] = []
    in_comment = False
    comment_buf: List[str] = []

    cur_domain: Optional[DomainDef] = None
    cur_struct: Optional[StructDef] = None
    top_section: Optional[str] = None  # "Events" | "CommandParameters" | None

    while i < n:
        line = lines[i]
        stripped = line.strip()

        # Handle multi-line / single-line JSDoc comments
        if in_comment:
            comment_buf.append(line)
            if "*/" in stripped:
                in_comment = False
                pending_doc = clean_doc_lines(comment_buf)
                comment_buf = []
            i += 1
            continue

        if stripped.startswith("/**"):
            if "*/" in stripped[3:]:
                pending_doc = clean_doc_lines([line])
            else:
                in_comment = True
                comment_buf = [line]
            i += 1
            continue

        if stripped == "" or stripped.startswith("//"):
            i += 1
            continue

        # Inside a domain
        if cur_domain is not None:
            # End of domain
            if re.match(r"^  \}", line):
                cur_domain = None
                cur_struct = None
                pending_doc = []
                i += 1
                continue

            # Inside a struct/interface/type_obj
            if cur_struct is not None:
                if re.match(r"^    \}", line):
                    cur_domain.items.append(cur_struct)
                    cur_struct = None
                    pending_doc = []
                    i += 1
                    continue

                m_field = re.match(r"^      ([a-zA-Z0-9_$]+)(\?)?:\s*(.+?);$", line)
                if m_field:
                    fname = m_field.group(1)
                    opt = bool(m_field.group(2))
                    ftype = m_field.group(3).strip()
                    cur_struct.fields.append(FieldDef(
                        name=fname,
                        optional=opt,
                        ts_type=ftype,
                        doc=pending_doc,
                    ))
                    pending_doc = []
                    i += 1
                    continue

                i += 1
                continue

            # Start of interface
            m_if = re.match(r"^    export interface (\w+)\s*\{", line)
            if m_if:
                cur_struct = StructDef(
                    domain=cur_domain.name,
                    name=m_if.group(1),
                    kind="interface",
                    doc=pending_doc,
                )
                pending_doc = []
                if stripped.endswith("}"):
                    cur_domain.items.append(cur_struct)
                    cur_struct = None
                i += 1
                continue

            # Start of multi-line object type alias: export type Foo = {
            m_to = re.match(r"^    export type (\w+)\s*=\s*\{\s*$", line)
            if m_to:
                cur_struct = StructDef(
                    domain=cur_domain.name,
                    name=m_to.group(1),
                    kind="type_obj",
                    doc=pending_doc,
                )
                pending_doc = []
                i += 1
                continue

            # Single-line type alias
            m_ta = re.match(r"^    export type (\w+)\s*=\s*(.+?);$", line)
            if m_ta:
                cur_domain.items.append(TypeAliasDef(
                    domain=cur_domain.name,
                    name=m_ta.group(1),
                    ts_rhs=m_ta.group(2).strip(),
                    doc=pending_doc,
                ))
                pending_doc = []
                i += 1
                continue

            i += 1
            continue

        # Outside a domain (inside export namespace Protocol)
        m_ns = re.match(r"^  export namespace (\w+)\s*\{", line)
        if m_ns:
            cur_domain = DomainDef(name=m_ns.group(1), doc=pending_doc)
            domains.append(cur_domain)
            pending_doc = []
            top_section = None
            i += 1
            continue

        m_top = re.match(r"^  export (?:type|interface) (Events|EventMap|CommandParameters|CommandReturnValues)\s*(?:=\s*)?\{", line)
        if m_top:
            top_section = m_top.group(1)
            pending_doc = []
            i += 1
            continue

        if top_section is not None:
            if re.match(r"^  \}", line):
                top_section = None
                i += 1
                continue
            if top_section == "Events":
                m_ev = re.match(r'^    "([^"]+)":\s*([\w.]+);$', line)
                if m_ev:
                    events.append((m_ev.group(1), m_ev.group(2)))
            elif top_section == "CommandParameters":
                m_cmd = re.match(r'^    "([^"]+)":\s*([\w.]+);$', line)
                if m_cmd:
                    commands.append(m_cmd.group(1))
            i += 1
            continue

        pending_doc = []
        i += 1

    return ProtocolSpec(domains=domains, events=events, commands=commands)


def cp_symbol(domain: str, name: str) -> str:
    return f"{domain}_{name}"


def escape_field_name(name: str) -> Tuple[str, bool]:
    if name in RESERVED_IDENTIFIERS:
        return f"{name}_", True
    return name, False


class ProtocolEmitter:
    def __init__(self, spec: ProtocolSpec, source_rel: str, prefix: str):
        self.spec = spec
        self.source_rel = source_rel
        self.prefix = prefix  # "CDP" or "WK"

        # Symbol table: (domain, name) -> ("alias", rhs) | ("struct", StructDef)
        self.symbols: Dict[Tuple[str, str], Tuple[str, object]] = {}
        for dom in spec.domains:
            for item in dom.items:
                if isinstance(item, TypeAliasDef):
                    self.symbols[(dom.name, item.name)] = ("alias", item)
                elif isinstance(item, StructDef):
                    self.symbols[(dom.name, item.name)] = ("struct", item)

        # Compute base kind for every symbol so we can pick accurate default values
        # base kinds: "string", "number", "bool", "slice", "struct"
        self.base_kind_cache: Dict[Tuple[str, str], str] = {}
        for key in self.symbols:
            self._resolve_base_kind(key, set())

        # Detect direct or mutual non-array struct cycles
        self.cycle_edges: Set[Tuple[Tuple[str, str], Tuple[str, str]]] = self._find_non_array_cycles()

    def _classify_ts_type(self, domain: str, ts_type: str) -> Tuple[str, Optional[Tuple[str, str]], bool]:
        """
        Returns (kind, ref_key_or_none, is_array)
        kind in {"string", "binary", "number", "bool", "any", "map", "string_union", "ref"}
        """
        t = ts_type.strip()
        is_arr = False
        if t.endswith("[]"):
            is_arr = True
            t = t[:-2].strip()
            if t.startswith("(") and t.endswith(")"):
                t = t[1:-1].strip()

        if t == "string":
            return ("string", None, is_arr)
        if t == "binary":
            return ("binary", None, is_arr)
        if t == "number":
            return ("number", None, is_arr)
        if t == "boolean":
            return ("bool", None, is_arr)
        if t in ("any", "unknown", "object", "void"):
            return ("any", None, is_arr)
        if t.startswith("{") and "string" in t:
            return ("map", None, is_arr)
        if t.startswith('"'):
            return ("string_union", None, is_arr)
        if "." in t:
            parts = t.split(".", 1)
            return ("ref", (parts[0], parts[1]), is_arr)
        return ("ref", (domain, t), is_arr)

    def _resolve_base_kind(self, key: Tuple[str, str], visiting: Set[Tuple[str, str]]) -> str:
        if key in self.base_kind_cache:
            return self.base_kind_cache[key]
        if key in visiting:
            return "struct"
        visiting.add(key)
        entry = self.symbols.get(key)
        if entry is None:
            res = "string"
        elif entry[0] == "struct":
            res = "struct"
        else:
            alias: TypeAliasDef = entry[1]  # type: ignore
            kind, ref_key, is_arr = self._classify_ts_type(alias.domain, alias.ts_rhs)
            if is_arr:
                res = "slice"
            elif kind in ("string", "binary", "any", "map", "string_union"):
                res = "string"
            elif kind == "number":
                res = "number"
            elif kind == "bool":
                res = "bool"
            elif kind == "ref" and ref_key is not None:
                res = self._resolve_base_kind(ref_key, visiting)
            else:
                res = "string"
        self.base_kind_cache[key] = res
        return res

    def _find_non_array_cycles(self) -> Set[Tuple[Tuple[str, str], Tuple[str, str]]]:
        """Find all (src_struct, dst_struct) edges where dst_struct can reach src_struct via non-array struct fields."""
        adj: Dict[Tuple[str, str], Set[Tuple[str, str]]] = defaultdict(set)
        for key, (kind, obj) in self.symbols.items():
            if kind != "struct":
                continue
            sdef: StructDef = obj  # type: ignore
            for f in sdef.fields:
                tk, ref_key, is_arr = self._classify_ts_type(sdef.domain, f.ts_type)
                if not is_arr and tk == "ref" and ref_key is not None:
                    if self.base_kind_cache.get(ref_key) == "struct":
                        adj[key].add(ref_key)

        cycle_edges: Set[Tuple[Tuple[str, str], Tuple[str, str]]] = set()
        # For each edge u -> v, check if v can reach u
        for u, nbrs in adj.items():
            for v in nbrs:
                if u == v:
                    cycle_edges.add((u, v))
                    continue
                # BFS from v to u
                seen = {v}
                queue = [v]
                reachable = False
                while queue:
                    cur = queue.pop()
                    if cur == u:
                        reachable = True
                        break
                    for nxt in adj.get(cur, ()):
                        if nxt not in seen:
                            seen.add(nxt)
                            queue.append(nxt)
                if reachable:
                    cycle_edges.add((u, v))
        return cycle_edges

    def map_type_expr(
        self,
        domain: str,
        ts_type: str,
        optional: bool,
        src_struct_key: Optional[Tuple[str, str]] = None,
    ) -> Tuple[str, str, Optional[Tuple[str, str]], Optional[str]]:
        """
        Returns (cp_type_str, default_val_str, external_ref_key_if_any, extra_comment_or_none).
        """
        kind, ref_key, is_arr = self._classify_ts_type(domain, ts_type)
        extra_comment: Optional[str] = None
        ext_ref: Optional[Tuple[str, str]] = None

        if kind == "string":
            base_type = "[]const u8"
            base_default = '""'
        elif kind == "binary":
            base_type = "ProtocolBinary"
            base_default = '""'
        elif kind == "number":
            base_type = "f64"
            base_default = "0.0"
        elif kind == "bool":
            base_type = "bool"
            base_default = "false"
        elif kind == "any":
            base_type = "ProtocolJsonValue"
            base_default = '""'
        elif kind == "map":
            base_type = "ProtocolStringMap"
            base_default = '""'
        elif kind == "string_union":
            base_type = "[]const u8"
            base_default = '""'
            vals = ts_type.strip()
            if len(vals) > 120:
                vals = vals[:117] + "..."
            extra_comment = f"enum: {vals}"
        elif kind == "ref" and ref_key is not None:
            rdom, rname = ref_key
            if ref_key in self.symbols:
                base_type = cp_symbol(rdom, rname)
                if rdom != domain:
                    ext_ref = ref_key
                bk = self.base_kind_cache.get(ref_key, "struct")
                if bk == "string":
                    base_default = '""'
                elif bk == "number":
                    base_default = "0.0"
                elif bk == "bool":
                    base_default = "false"
                elif bk == "slice":
                    base_default = "&.{}"
                else:
                    base_default = ".{}"
            else:
                # Unknown fallback -> strong string representation
                base_type = "ProtocolJsonValue"
                base_default = '""'
        else:
            base_type = "ProtocolJsonValue"
            base_default = '""'

        # Check non-array struct cycle -> emit pointer to break infinite value recursion
        if (
            not is_arr
            and src_struct_key is not None
            and ref_key is not None
            and (src_struct_key, ref_key) in self.cycle_edges
        ):
            return (f"(*const {base_type})?", "null", ext_ref, extra_comment)

        if is_arr:
            arr_type = f"[]const {base_type}"
            if optional:
                return (f"({arr_type})?", "null", ext_ref, extra_comment)
            return (arr_type, "&.{}", ext_ref, extra_comment)

        if optional:
            if " " in base_type or base_type.startswith("[") or base_type.startswith("*"):
                return (f"({base_type})?", "null", ext_ref, extra_comment)
            return (f"{base_type}?", "null", ext_ref, extra_comment)

        return (base_type, base_default, ext_ref, extra_comment)

    def render_domain_body(self, dom: DomainDef) -> Tuple[List[str], Set[Tuple[str, str]], Set[str], int, int]:
        """
        Render all types and structs in a domain.
        Returns (lines, external_refs, common_imports_used, struct_count, type_alias_count).
        """
        out: List[str] = []
        ext_refs: Set[Tuple[str, str]] = set()
        common_used: Set[str] = set()
        struct_count = 0
        alias_count = 0

        out.append("// ============================================================================")
        out.append(f"// Domain: {dom.name}")
        for d in dom.doc[:2]:
            out.append(f"// {d}")
        out.append("// ============================================================================")
        out.append("")

        for item in dom.items:
            if isinstance(item, TypeAliasDef):
                alias_count += 1
                sym = cp_symbol(dom.name, item.name)
                cp_type, _, ext_ref, extra_comment = self.map_type_expr(dom.name, item.ts_rhs, False, None)
                if ext_ref is not None:
                    ext_refs.add(ext_ref)
                for ct in ("ProtocolBinary", "ProtocolJsonValue", "ProtocolStringMap"):
                    if ct in cp_type:
                        common_used.add(ct)
                if item.doc:
                    out.append(f"/// {item.doc[0]}")
                if extra_comment:
                    out.append(f"/// {extra_comment}")
                out.append(f"pub type {sym} = {cp_type};")
            elif isinstance(item, StructDef):
                struct_count += 1
                sym = cp_symbol(dom.name, item.name)
                if item.doc:
                    out.append(f"/// {item.doc[0]}")
                if not item.fields:
                    out.append(f"pub struct {sym} {{}}")
                    continue
                out.append(f"pub struct {sym} {{")
                for f in item.fields:
                    fname, escaped = escape_field_name(f.name)
                    cp_type, default_val, ext_ref, extra_comment = self.map_type_expr(
                        dom.name, f.ts_type, f.optional, (dom.name, item.name)
                    )
                    if ext_ref is not None:
                        ext_refs.add(ext_ref)
                    for ct in ("ProtocolBinary", "ProtocolJsonValue", "ProtocolStringMap"):
                        if ct in cp_type:
                            common_used.add(ct)
                    notes: List[str] = []
                    if escaped:
                        notes.append(f'protocol field: "{f.name}"')
                    if extra_comment:
                        notes.append(extra_comment)
                    suffix = f" // {'; '.join(notes)}" if notes else ""
                    out.append(f"    {fname}: {cp_type} = {default_val},{suffix}")
                out.append("}")
        out.append("")
        return out, ext_refs, common_used, struct_count, alias_count


def partition_domains(
    domain_renders: List[Tuple[DomainDef, List[str], Set[Tuple[str, str]], Set[str], int, int]],
    target_min_files: int,
    target_max_files: int,
    max_lines_per_file: int = 2100,
) -> List[List[Tuple[DomainDef, List[str], Set[Tuple[str, str]], Set[str], int, int]]]:
    """
    Partition domains sequentially into balanced buckets so every file stays well under 2,500 lines
    and bucket count falls within [target_min_files, target_max_files].
    """
    total_lines = sum(len(r[1]) for r in domain_renders)
    desired_files = max(target_min_files, min(target_max_files, (total_lines + 1400) // 1500))
    target_per_file = min(max_lines_per_file, max(600, total_lines // desired_files))

    buckets: List[List[Tuple[DomainDef, List[str], Set[Tuple[str, str]], Set[str], int, int]]] = []
    cur_bucket: List[Tuple[DomainDef, List[str], Set[Tuple[str, str]], Set[str], int, int]] = []
    cur_lines = 0

    for idx, entry in enumerate(domain_renders):
        dlines = len(entry[1])
        remaining_domains = len(domain_renders) - idx
        remaining_slots = target_min_files - len(buckets) - (1 if cur_bucket else 0)

        # Force split if adding this domain exceeds max_lines_per_file or target_per_file (when we have room)
        should_split = False
        if cur_bucket:
            if cur_lines + dlines > max_lines_per_file:
                should_split = True
            elif cur_lines + dlines > target_per_file and len(buckets) + 1 < target_max_files:
                should_split = True
            elif remaining_domains <= remaining_slots:
                should_split = True

        if should_split:
            buckets.append(cur_bucket)
            cur_bucket = [entry]
            cur_lines = dlines
        else:
            cur_bucket.append(entry)
            cur_lines += dlines

    if cur_bucket:
        # Merge tiny trailing bucket (< 100 lines) into previous bucket if within limit and min_files
        if (
            buckets
            and cur_lines < 100
            and len(buckets) >= target_min_files
            and sum(len(x[1]) for x in buckets[-1]) + cur_lines <= max_lines_per_file
        ):
            buckets[-1].extend(cur_bucket)
        else:
            buckets.append(cur_bucket)

    return buckets


def make_bucket_filename(bucket: List[Tuple[DomainDef, List[str], Set[Tuple[str, str]], Set[str], int, int]], idx: int) -> str:
    first = bucket[0][0].name.lower()
    last = bucket[-1][0].name.lower()
    if len(bucket) == 1:
        return f"domain_{idx:02d}_{first}.cp"
    return f"domains_{idx:02d}_{first}_to_{last}.cp"


def generate_target(
    dts_path: Path,
    source_rel: str,
    out_dir: Path,
    prefix: str,
    target_min_files: int,
    target_max_files: int,
) -> Dict[str, object]:
    spec = parse_protocol_dts(dts_path)
    emitter = ProtocolEmitter(spec, source_rel, prefix)

    out_dir.mkdir(parents=True, exist_ok=True)
    # Clean any stale generated .cp files in out_dir
    for old in out_dir.glob("*.cp"):
        old.unlink()

    header = f"// Source: Absorbed from {source_rel} | Synced: {SYNC_DATE}"

    # Render all domains first
    domain_renders = []
    total_structs = 0
    total_aliases = 0
    for dom in spec.domains:
        body_lines, ext_refs, common_used, sc, ac = emitter.render_domain_body(dom)
        domain_renders.append((dom, body_lines, ext_refs, common_used, sc, ac))
        total_structs += sc
        total_aliases += ac

    # Partition domains into buckets
    buckets = partition_domains(domain_renders, target_min_files, target_max_files, max_lines_per_file=2050)

    # Map domain name -> bucket module stem
    domain_to_mod: Dict[str, str] = {}
    bucket_files: List[Tuple[str, List[Tuple[DomainDef, List[str], Set[Tuple[str, str]], Set[str], int, int]]]] = []
    for idx, bucket in enumerate(buckets, 1):
        fname = make_bucket_filename(bucket, idx)
        stem = fname[:-3]
        for entry in bucket:
            domain_to_mod[entry[0].name] = stem
        bucket_files.append((fname, bucket))

    generated_files: List[Tuple[Path, int]] = []

    # 1. Write common.cp
    common_lines = [
        header,
        "// ============================================================================",
        f"// {prefix} Protocol Common Primitive Aliases & Registry Metadata",
        "// ============================================================================",
        "",
        "/// Base64-encoded binary payload string in protocol messages.",
        "pub type ProtocolBinary = []const u8;",
        "",
        "/// Raw JSON value slice for dynamic protocol payload fields (no `any`).",
        "pub type ProtocolJsonValue = []const u8;",
        "",
        "/// Raw JSON object slice representing string-to-string key/value maps.",
        "pub type ProtocolStringMap = []const u8;",
        "",
        f"pub const {prefix}_DOMAIN_COUNT: usize = {len(spec.domains)};",
        f"pub const {prefix}_EVENT_COUNT: usize = {len(spec.events)};",
        f"pub const {prefix}_COMMAND_COUNT: usize = {len(spec.commands)};",
        "",
        f"pub const {prefix}_DOMAINS: []const []const u8 = &.{{",
    ]
    for dom in spec.domains:
        common_lines.append(f'    "{dom.name}",')
    common_lines.append("};")
    common_lines.append("")

    # Also include protocol event names and command names as typed constants
    common_lines.append(f"pub const {prefix}_EVENT_NAMES: []const []const u8 = &.{{")
    for ev_name, _ in spec.events:
        common_lines.append(f'    "{ev_name}",')
    common_lines.append("};")
    common_lines.append("")

    common_lines.append(f"pub const {prefix}_COMMAND_NAMES: []const []const u8 = &.{{")
    for cmd_name in spec.commands:
        common_lines.append(f'    "{cmd_name}",')
    common_lines.append("};")
    common_lines.append("")

    common_path = out_dir / "common.cp"
    common_content = "\n".join(common_lines)
    common_path.write_text(common_content, encoding="utf-8")
    generated_files.append((common_path, len(common_lines)))

    # 2. Write domain bucket files
    for fname, bucket in bucket_files:
        stem = fname[:-3]
        local_domains = {entry[0].name for entry in bucket}
        all_ext_refs: Set[Tuple[str, str]] = set()
        all_common: Set[str] = set()
        for entry in bucket:
            all_ext_refs |= entry[2]
            all_common |= entry[3]

        # Group external imports by target module (excluding symbols in the same file)
        imports_by_mod: Dict[str, List[str]] = defaultdict(list)
        for rdom, rname in sorted(all_ext_refs):
            if rdom in local_domains:
                continue
            target_mod = domain_to_mod.get(rdom)
            if target_mod and target_mod != stem:
                imports_by_mod[target_mod].append(cp_symbol(rdom, rname))

        file_lines: List[str] = [header, ""]
        if all_common:
            syms = ", ".join(sorted(all_common))
            file_lines.append(f'import {{ {syms} }} from "./common";')
        for mod_name in sorted(imports_by_mod.keys()):
            syms = sorted(set(imports_by_mod[mod_name]))
            if len(syms) <= 4:
                file_lines.append(f'import {{ {", ".join(syms)} }} from "./{mod_name}";')
            else:
                file_lines.append("import {")
                for s in syms:
                    file_lines.append(f"    {s},")
                file_lines.append(f'}} from "./{mod_name}";')
        if all_common or imports_by_mod:
            file_lines.append("")

        for entry in bucket:
            file_lines.extend(entry[1])

        fpath = out_dir / fname
        fpath.write_text("\n".join(file_lines), encoding="utf-8")
        generated_files.append((fpath, len(file_lines)))

    # 3. Write index.cp
    index_lines = [
        header,
        "// ============================================================================",
        f"// {prefix} Full Protocol Types — Unified Barrel Export",
        "// ============================================================================",
        "",
        'export * from "./common";',
    ]
    for fname, _ in bucket_files:
        stem = fname[:-3]
        index_lines.append(f'export * from "./{stem}";')
    index_lines.append("")

    index_path = out_dir / "index.cp"
    index_path.write_text("\n".join(index_lines), encoding="utf-8")
    generated_files.append((index_path, len(index_lines)))

    return {
        "prefix": prefix,
        "out_dir": str(out_dir),
        "domains": len(spec.domains),
        "structs": total_structs,
        "type_aliases": total_aliases,
        "total_types": total_structs + total_aliases,
        "events": len(spec.events),
        "commands": len(spec.commands),
        "files": [(p.name, ln) for p, ln in generated_files],
        "total_files": len(generated_files),
        "total_lines": sum(ln for _, ln in generated_files),
        "max_file_lines": max(ln for _, ln in generated_files),
    }


def main() -> int:
    targets = [
        (
            ROOT / "_ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts",
            "_ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts",
            ROOT / "lib/cdp/protocol_types",
            "CDP",
            10,
            15,
        ),
        (
            ROOT / "_ref/playwright/packages/playwright-core/src/server/webkit/protocol.d.ts",
            "_ref/playwright/packages/playwright-core/src/server/webkit/protocol.d.ts",
            ROOT / "packages/playwright/src/wk_protocol",
            "WK",
            5,
            8,
        ),
    ]

    for dts_path, source_rel, out_dir, prefix, min_f, max_f in targets:
        if not dts_path.exists():
            print(f"ERROR: missing source file {dts_path}", file=sys.stderr)
            return 1
        stats = generate_target(dts_path, source_rel, out_dir, prefix, min_f, max_f)
        print(f"=== Generated {stats['prefix']} ({stats['out_dir']}) ===")
        print(
            f"  Domains: {stats['domains']} | Structs: {stats['structs']} | TypeAliases: {stats['type_aliases']} "
            f"| Total Types: {stats['total_types']} | Events: {stats['events']} | Commands: {stats['commands']}"
        )
        print(
            f"  Files: {stats['total_files']} | Total Lines: {stats['total_lines']} | Max File Lines: {stats['max_file_lines']}"
        )
        for fname, ln in stats["files"]:  # type: ignore
            print(f"    - {fname}: {ln} lines")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
