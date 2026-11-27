#!/usr/bin/env python3
"""
CP Ecosystem Tail Documentation AST Enricher (Worker CK)

Enriches un-polished mid/long-tail packages under `packages/` into production-grade
EN (`README.md`), ZH (`README.zh-CN.md`), and AI Agent (`README_AGENT.md`) docs
using real metadata from `cap.json` and real exported signatures parsed from
`src/index.d.cp` (falling back to `src/index.cp` when needed).

Protection guarantees:
- Never touches `.cp`, `.d.cp`, `cap.json`, `registry.json`, `SESSION.md`, `PROGRESS.md`.
- Skips Batch 16 in-flight packages.
- Protects all manually polished packages (nav + README_AGENT.md >= 38 lines).
- For packages with manually polished README.md (>= 60 lines with Quick Ref nav)
  but short README_AGENT.md (< 38 lines), preserves README.md / README.zh-CN.md
  and only enriches README_AGENT.md.
"""

from __future__ import annotations

import argparse
import json
import os
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import List, Optional, Tuple

REPO_ROOT = Path(__file__).resolve().parent.parent
PACKAGES_DIR = REPO_ROOT / "packages"

BATCH16_SKIP = {
    "depd",
    "semver-diff",
    "humanize-url",
    "escape-string-regexp",
    "is-extglob",
    "normalize-line-endings",
    "decamelize",
    "dunce",
    "methods",
    "is-binary-path",
    "prepend-http",
    "indent-string",
    "trim-newlines",
    "strip-ansi",
}

NAV_EN = "[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quick Ref](./README_AGENT.md)"
NAV_ZH = "[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)"


@dataclass
class ConstDecl:
    name: str
    type_str: str


@dataclass
class TypeAliasDecl:
    name: str
    target: str


@dataclass
class EnumDecl:
    name: str
    variants: List[str] = field(default_factory=list)


@dataclass
class ErrorDecl:
    name: str
    variants: List[str] = field(default_factory=list)


@dataclass
class ClassDecl:
    kind: str  # "class" or "struct"
    name: str
    fields: List[str] = field(default_factory=list)
    methods: List[str] = field(default_factory=list)


@dataclass
class FnDecl:
    name: str
    params: str
    ret: str
    raw: str


@dataclass
class ModuleExports:
    consts: List[ConstDecl] = field(default_factory=list)
    types: List[TypeAliasDecl] = field(default_factory=list)
    enums: List[EnumDecl] = field(default_factory=list)
    errors: List[ErrorDecl] = field(default_factory=list)
    classes: List[ClassDecl] = field(default_factory=list)
    functions: List[FnDecl] = field(default_factory=list)

    def total_symbols(self) -> int:
        return (
            len(self.consts)
            + len(self.types)
            + len(self.enums)
            + len(self.errors)
            + len(self.classes)
            + len(self.functions)
        )

    def all_symbol_names(self) -> List[str]:
        names: List[str] = []
        for c in self.consts:
            names.append(c.name)
        for t in self.types:
            names.append(t.name)
        for e in self.enums:
            names.append(e.name)
        for err in self.errors:
            names.append(err.name)
        for cls in self.classes:
            names.append(cls.name)
        for fn in self.functions:
            names.append(fn.name)
        # deduplicate preserving order
        seen = set()
        out = []
        for n in names:
            if n not in seen:
                seen.add(n)
                out.append(n)
        return out


def discover_packages() -> List[Tuple[str, Path]]:
    pkgs: List[Tuple[str, Path]] = []
    for entry in sorted(os.listdir(PACKAGES_DIR)):
        p = PACKAGES_DIR / entry
        if not p.is_dir():
            continue
        if entry.startswith("@"):
            for sub in sorted(os.listdir(p)):
                sp = p / sub
                if sp.is_dir() and (sp / "cap.json").exists():
                    pkgs.append((f"{entry}/{sub}", sp))
        else:
            if (p / "cap.json").exists():
                pkgs.append((entry, p))
    return pkgs


def strip_line_comments(text: str) -> str:
    # Remove single-line comments safely outside strings
    out_lines = []
    for line in text.splitlines():
        s = line.strip()
        if s.startswith("//"):
            continue
        out_lines.append(line)
    return "\n".join(out_lines)


def find_matching_brace(text: str, open_pos: int) -> int:
    depth = 0
    i = open_pos
    n = len(text)
    while i < n:
        ch = text[i]
        if ch == "{":
            depth += 1
        elif ch == "}":
            depth -= 1
            if depth == 0:
                return i
        i += 1
    return n - 1


def collapse_ws(s: str) -> str:
    return re.sub(r"\s+", " ", s).strip()


def parse_dcp(text: str) -> ModuleExports:
    mod = ModuleExports()
    clean = strip_line_comments(text)
    i = 0
    n = len(clean)

    while i < n:
        m = re.search(
            r"(?m)^export\s+(const|type|error|enum|class|struct|fn)\b", clean[i:]
        )
        if not m:
            break
        start = i + m.start()
        kw = m.group(1)
        pos = i + m.end()

        if kw == "const":
            semi = clean.find(";", pos)
            if semi == -1:
                break
            stmt = collapse_ws(clean[pos:semi])
            mc = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*:\s*(.+)$", stmt)
            if mc:
                mod.consts.append(ConstDecl(mc.group(1), mc.group(2).strip()))
            else:
                mc2 = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)$", stmt)
                if mc2:
                    mod.consts.append(ConstDecl(mc2.group(1), mc2.group(2).strip()))
            i = semi + 1

        elif kw == "type":
            semi = clean.find(";", pos)
            if semi == -1:
                break
            stmt = collapse_ws(clean[pos:semi])
            mt = re.match(r"^([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)$", stmt)
            if mt:
                mod.types.append(TypeAliasDecl(mt.group(1), mt.group(2).strip()))
            i = semi + 1

        elif kw in ("error", "enum"):
            brace = clean.find("{", pos)
            if brace == -1:
                i = pos
                continue
            name = collapse_ws(clean[pos:brace])
            close = find_matching_brace(clean, brace)
            body = clean[brace + 1 : close]
            variants = [
                v.strip().split("=")[0].strip()
                for v in re.split(r"[,\n]", body)
                if v.strip()
            ]
            if kw == "error":
                mod.errors.append(ErrorDecl(name, variants))
            else:
                mod.enums.append(EnumDecl(name, variants))
            i = close + 1

        elif kw in ("class", "struct"):
            brace = clean.find("{", pos)
            if brace == -1:
                i = pos
                continue
            name = collapse_ws(clean[pos:brace])
            close = find_matching_brace(clean, brace)
            body = clean[brace + 1 : close]
            cls = ClassDecl(kind=kw, name=name)
            for raw_line in body.splitlines():
                ln = raw_line.strip()
                if not ln:
                    continue
                if re.match(r"^(static\s+|pub\s+)?fn\b", ln):
                    cls.methods.append(ln.rstrip(";"))
                else:
                    fld = ln.rstrip(",;").strip()
                    if fld and ":" in fld:
                        cls.fields.append(fld)
            mod.classes.append(cls)
            i = close + 1

        elif kw == "fn":
            semi = clean.find(";", pos)
            if semi == -1:
                break
            sig = collapse_ws(clean[pos:semi])
            # match name(params): ret  OR  name(params)
            # handle optional generic <...>
            mf = re.match(r"^([A-Za-z_][A-Za-z0-9_]*(?:<[^>]+>)?)\s*\((.*)\)\s*(?::\s*(.+))?$", sig)
            if mf:
                fn_name = mf.group(1).strip()
                params = mf.group(2).strip()
                ret = (mf.group(3) or "void").strip()
                mod.functions.append(
                    FnDecl(
                        name=fn_name,
                        params=params,
                        ret=ret,
                        raw=f"fn {fn_name}({params})" + (f": {ret}" if mf.group(3) else ""),
                    )
                )
            i = semi + 1
        else:
            i = pos

    return mod


def parse_cp_fallback(text: str) -> ModuleExports:
    """Fallback parser for src/index.cp pub declarations."""
    mod = ModuleExports()
    clean = strip_line_comments(text)
    for m in re.finditer(
        r"(?m)^pub\s+const\s+([A-Za-z_][A-Za-z0-9_]*)\s*:\s*([^=;]+)", clean
    ):
        mod.consts.append(ConstDecl(m.group(1), collapse_ws(m.group(2))))
    for m in re.finditer(
        r"(?m)^pub\s+(class|struct)\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{", clean
    ):
        mod.classes.append(ClassDecl(kind=m.group(1), name=m.group(2)))
    for m in re.finditer(r"(?m)^pub\s+enum\s+([A-Za-z_][A-Za-z0-9_]*)\s*\{", clean):
        mod.enums.append(EnumDecl(name=m.group(1)))
    for m in re.finditer(
        r"(?m)^pub\s+fn\s+([A-Za-z_][A-Za-z0-9_]*)\s*\(([^)]*)\)\s*(?::\s*([^{\n]+))?\s*\{",
        clean,
    ):
        fn_name = m.group(1)
        params = collapse_ws(m.group(2))
        ret = collapse_ws(m.group(3)) if m.group(3) else "void"
        raw = f"fn {fn_name}({params})" + (f": {ret}" if m.group(3) else "")
        mod.functions.append(FnDecl(name=fn_name, params=params, ret=ret, raw=raw))
    return mod


def load_exports(pkg_dir: Path) -> ModuleExports:
    dcp_path = pkg_dir / "src" / "index.d.cp"
    cp_path = pkg_dir / "src" / "index.cp"
    mod = ModuleExports()
    if dcp_path.exists():
        mod = parse_dcp(dcp_path.read_text(encoding="utf-8", errors="ignore"))
    if mod.total_symbols() == 0 and cp_path.exists():
        mod = parse_cp_fallback(cp_path.read_text(encoding="utf-8", errors="ignore"))
    return mod


def md_escape(s: str) -> str:
    return s.replace("|", "\\|").replace("\n", " ")


def infer_fn_desc_en(fn: FnDecl) -> str:
    name = fn.name
    lname = name.lower()
    ret = fn.ret
    if lname.startswith("is") or lname.startswith("has") or lname.startswith("can"):
        return f"Checks predicate `{name}` and returns `bool`."
    if lname.startswith("parse") or lname.startswith("decode"):
        return f"Parses input via `{name}` and returns `{ret}`."
    if lname.startswith("format") or lname.startswith("encode") or lname.startswith("stringify"):
        return f"Serializes/formats data via `{name}` returning `{ret}`."
    if lname.startswith("validate") or lname.startswith("verify") or lname.startswith("check"):
        return f"Validates input with `{name}` returning `{ret}`."
    if lname.startswith("new") or lname.startswith("init") or lname.startswith("create"):
        return f"Constructs/initializes an instance via `{name}`."
    if lname.startswith("free") or lname.startswith("destroy") or lname.startswith("close"):
        return f"Releases native resources allocated by this module."
    if lname.startswith("get") or lname.startswith("find") or lname.startswith("lookup"):
        return f"Queries/retrieves data via `{name}` returning `{ret}`."
    if lname.startswith("set") or lname.startswith("put") or lname.startswith("add") or lname.startswith("push"):
        return f"Mutates or appends state via `{name}`."
    if lname.startswith("register"):
        return f"Registers plugin/middleware into the target application."
    if ret == "void":
        return f"Executes `{name}` in-place."
    return f"Invokes `{name}` and returns `{ret}`."


def infer_fn_desc_zh(fn: FnDecl) -> str:
    name = fn.name
    lname = name.lower()
    ret = fn.ret
    if lname.startswith("is") or lname.startswith("has") or lname.startswith("can"):
        return f"执行 `{name}` 条件判定，返回 `bool`。"
    if lname.startswith("parse") or lname.startswith("decode"):
        return f"通过 `{name}` 解析输入数据，返回 `{ret}`。"
    if lname.startswith("format") or lname.startswith("encode") or lname.startswith("stringify"):
        return f"通过 `{name}` 编码或格式化输出，返回 `{ret}`。"
    if lname.startswith("validate") or lname.startswith("verify") or lname.startswith("check"):
        return f"校验输入合法性，返回 `{ret}`。"
    if lname.startswith("new") or lname.startswith("init") or lname.startswith("create"):
        return f"创建或初始化实例，返回 `{ret}`。"
    if lname.startswith("free") or lname.startswith("destroy") or lname.startswith("close"):
        return f"释放模块分配的底层资源。"
    if lname.startswith("get") or lname.startswith("find") or lname.startswith("lookup"):
        return f"查询或读取目标数据，返回 `{ret}`。"
    if lname.startswith("set") or lname.startswith("put") or lname.startswith("add") or lname.startswith("push"):
        return f"写入或更新状态数据。"
    if lname.startswith("register"):
        return f"将插件或中间件注册到目标应用实例。"
    if ret == "void":
        return f"就地执行 `{name}` 操作。"
    return f"调用 `{name}` 并返回 `{ret}`。"


def split_params(params_str: str) -> List[Tuple[str, str]]:
    if not params_str.strip():
        return []
    parts = []
    depth = 0
    cur = []
    for ch in params_str:
        if ch in "([<":
            depth += 1
        elif ch in ")]>":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append("".join(cur).strip())
            cur = []
        else:
            cur.append(ch)
    if cur:
        parts.append("".join(cur).strip())
    res = []
    for p in parts:
        if ":" in p:
            k, v = p.split(":", 1)
            res.append((k.strip(), v.strip()))
        else:
            res.append((p.strip(), "auto"))
    return res


def pick_sample_fn(mod: ModuleExports) -> Optional[FnDecl]:
    # Prefer pure high-level functions with simple params (string, primitives, []u8)
    def score(fn: FnDecl) -> int:
        if fn.name.startswith("_") or fn.name in ("free", "malloc"):
            return -100
        if "[*" in fn.params or "*auto" in fn.params or "&mut App" in fn.params or "&mut Context" in fn.params:
            return -20
        s = 10
        if "string" in fn.params:
            s += 15
        if fn.ret not in ("void",):
            s += 5
        if len(split_params(fn.params)) <= 3:
            s += 10
        if any(
            fn.name.lower().startswith(p)
            for p in ("parse", "is", "format", "encode", "decode", "hash", "version", "get", "validate")
        ):
            s += 12
        return s

    if not mod.functions:
        return None
    return max(mod.functions, key=score)


def build_minimal_example_cp(pkg_name: str, mod: ModuleExports) -> str:
    fn = pick_sample_fn(mod)
    if fn is None:
        if mod.classes:
            cls = mod.classes[0]
            return (
                f'import {{ {cls.name} }} from "{pkg_name}";\n\n'
                f"pub fn main() {{\n"
                f"    // Refer to src/index.d.cp for {cls.name} fields and methods\n"
                f"}}"
            )
        return (
            f'// See src/index.d.cp for all exported symbols from "{pkg_name}"\n'
            f"pub fn main() {{}}"
        )

    params = split_params(fn.params)
    setup_lines: List[str] = []
    args: List[str] = []
    buf_idx = 0
    for pname, ptype in params:
        pt = ptype.strip()
        if pt == "string":
            args.append('"example"')
        elif pt in ("[]u8", "&[]u8", "&mut []u8"):
            bname = "out_buf" if buf_idx == 0 else f"buf_{buf_idx}"
            buf_idx += 1
            setup_lines.append(f"    let mut {bname}: [256]u8 = undefined;")
            args.append(f"{bname}[0..256]")
        elif pt in ("bool",):
            args.append("false")
        elif pt in ("u8", "u16", "u32", "u64", "usize", "i8", "i16", "i32", "i64", "isize"):
            args.append("0")
        elif pt in ("f32", "f64"):
            args.append("0.0")
        elif pt.startswith("[*]"):
            setup_lines.append('    let input = "example";')
            args.append("input.ptr")
        else:
            args.append(f"/* {pname}: {pt} */")

    call_expr = f"{fn.name}({', '.join(args)})"
    lines = [f'import {{ {fn.name} }} from "{pkg_name}";', "", "pub fn main() {"]
    lines.extend(setup_lines)
    if fn.ret == "void":
        lines.append(f"    {call_expr};")
    elif fn.ret.startswith("!"):
        lines.append(f"    let result = {call_expr} catch return;")
        lines.append("    _ = result;")
    else:
        lines.append(f"    let result = {call_expr};")
        lines.append("    _ = result;")
    lines.append("}")
    return "\n".join(lines)


def analyze_traits(mod: ModuleExports) -> dict:
    all_sigs = " ".join(fn.raw for fn in mod.functions)
    all_fields = " ".join(" ".join(c.fields + c.methods) for c in mod.classes)
    combined = all_sigs + " " + all_fields
    return {
        "has_out_buf": ("[]u8" in all_sigs) or ("out:" in all_sigs),
        "has_mut_borrow": ("&mut " in combined) or ("*" in all_sigs and "[*" not in all_sigs),
        "has_error_union": ("!" in all_sigs) or bool(mod.errors),
        "has_optional": ("?" in all_sigs),
        "has_c_ffi": ("[*]" in combined) or ("[*:0]" in combined) or ("*auto" in combined),
        "has_deinit": ("__cp_deinit" in combined) or any(
            fn.name.startswith("free") or fn.name.endswith("_free") or fn.name.endswith("_destroy")
            for fn in mod.functions
        ),
        "has_string_slice": ("string" in all_sigs),
    }


def extract_existing_attribution(readme_path: Path, native_cap_pkgs: set) -> Optional[str]:
    if not readme_path.exists():
        return None
    txt = readme_path.read_text(encoding="utf-8", errors="ignore")
    m = re.search(r"<!-- cap-source-attribution -->\s*\n(>[^\n]+)", txt)
    if not m:
        return None
    line = m.group(1).strip()
    # Safety: ensure no stale @cap/<non-native> in attribution
    for ref in re.findall(r"@cap/[A-Za-z0-9_.-]+", line):
        if ref not in native_cap_pkgs:
            return None
    return line


def format_import_block(pkg_name: str, mod: ModuleExports, limit: int = 8) -> str:
    syms = mod.all_symbol_names()
    if not syms:
        return f'// See src/index.d.cp for exports from "{pkg_name}"'
    chosen = syms[:limit]
    if len(chosen) <= 4:
        return f'import {{ {", ".join(chosen)} }} from "{pkg_name}";'
    inner = ",\n".join(f"    {s}" for s in chosen)
    return f'import {{\n{inner},\n}} from "{pkg_name}";'


def generate_readme_en(
    pkg_name: str,
    version: str,
    desc: str,
    license_str: str,
    mod: ModuleExports,
    attribution: Optional[str],
) -> str:
    traits = analyze_traits(mod)
    desc_text = desc.strip() or f"Native CP package `{pkg_name}`."
    lines: List[str] = [
        NAV_EN,
        "",
        f"# {pkg_name}",
        "",
        desc_text,
        "",
    ]
    if attribution:
        lines.append("<!-- cap-source-attribution -->")
        lines.append(attribution)
        lines.append("")

    lines.extend(
        [
            "| Metadata | Value |",
            "| --- | --- |",
            f"| Package | `{pkg_name}` |",
            f"| Version | `{version}` |",
            f"| License | `{license_str}` |",
            "| Type Declarations | [`src/index.d.cp`](./src/index.d.cp) |",
            "",
            "## Installation & Import",
            "",
            "```bash",
            f"cap add {pkg_name}",
            "```",
            "",
            "```cp",
            format_import_block(pkg_name, mod, limit=8),
            "```",
            "",
            "## Core API",
            "",
        ]
    )

    if mod.consts:
        lines.extend(
            [
                "### Constants",
                "",
                "| Constant | Type / Value |",
                "| --- | --- |",
            ]
        )
        for c in mod.consts[:30]:
            lines.append(f"| `{md_escape(c.name)}` | `{md_escape(c.type_str)}` |")
        if len(mod.consts) > 30:
            lines.append(f"\n*Showing 30 of {len(mod.consts)} constants; see [`src/index.d.cp`](./src/index.d.cp).*")
        lines.append("")

    if mod.types or mod.enums or mod.errors:
        lines.extend(
            [
                "### Types, Enums & Errors",
                "",
                "| Kind | Name | Definition |",
                "| --- | --- | --- |",
            ]
        )
        for t in mod.types:
            lines.append(f"| `type` | `{md_escape(t.name)}` | `= {md_escape(t.target)}` |")
        for e in mod.enums:
            vlist = ", ".join(e.variants[:8]) + (" ..." if len(e.variants) > 8 else "")
            lines.append(f"| `enum` | `{md_escape(e.name)}` | `{md_escape(vlist)}` |")
        for err in mod.errors:
            vlist = ", ".join(err.variants[:8]) + (" ..." if len(err.variants) > 8 else "")
            lines.append(f"| `error` | `{md_escape(err.name)}` | `{md_escape(vlist)}` |")
        lines.append("")

    if mod.classes:
        lines.extend(
            [
                "### Data Structures",
                "",
                "| Type | Kind | Fields & Methods |",
                "| --- | --- | --- |",
            ]
        )
        for cls in mod.classes[:30]:
            parts = []
            if cls.fields:
                f_preview = ", ".join(cls.fields[:6])
                if len(cls.fields) > 6:
                    f_preview += f", +{len(cls.fields) - 6} more"
                parts.append(f"Fields: `{md_escape(f_preview)}`")
            if cls.methods:
                m_names = [re.sub(r"^(static\s+|pub\s+)?fn\s+", "", m).split("(")[0] for m in cls.methods[:6]]
                m_preview = ", ".join(m_names)
                if len(cls.methods) > 6:
                    m_preview += f", +{len(cls.methods) - 6} more"
                parts.append(f"Methods: `{md_escape(m_preview)}`")
            summary = " · ".join(parts) if parts else "Opaque / marker container"
            lines.append(f"| `{md_escape(cls.name)}` | `{cls.kind}` | {summary} |")
        if len(mod.classes) > 30:
            lines.append(f"\n*Showing 30 of {len(mod.classes)} types; see [`src/index.d.cp`](./src/index.d.cp).*")
        lines.append("")

    if mod.functions:
        lines.extend(
            [
                "### Functions",
                "",
                "| Function | Signature | Description |",
                "| --- | --- | --- |",
            ]
        )
        fn_cap = 60
        for fn in mod.functions[:fn_cap]:
            sig = f"({fn.params}): {fn.ret}" if fn.ret != "void" else f"({fn.params})"
            lines.append(
                f"| `{md_escape(fn.name)}` | `{md_escape(sig)}` | {md_escape(infer_fn_desc_en(fn))} |"
            )
        if len(mod.functions) > fn_cap:
            lines.append(
                f"\n*Showing {fn_cap} of {len(mod.functions)} exported functions. See [`src/index.d.cp`](./src/index.d.cp) for the complete list.*"
            )
        lines.append("")

    lines.extend(
        [
            "## Minimal Example",
            "",
            "```cp",
            build_minimal_example_cp(pkg_name, mod),
            "```",
            "",
            "## CP Zero-Heap & Borrow-Safety Highlights",
            "",
        ]
    )
    if traits["has_c_ffi"]:
        lines.append(
            "- **Zero-overhead native binding**: Direct CP FFI to the underlying native C library without GC pauses or dynamic runtime wrappers."
        )
    else:
        lines.append(
            "- **Zero-heap pure CP design**: Operates on stack-allocated structs and slices without hidden heap allocations or garbage collection."
        )
    if traits["has_out_buf"]:
        lines.append(
            "- **Caller-owned output buffers**: Functions taking `[]u8` write directly into caller-provided buffers for deterministic memory usage."
        )
    if traits["has_string_slice"]:
        lines.append(
            "- **Zero-copy string slicing**: String parameters and returned slices borrow directly from caller memory without redundant copies."
        )
    if traits["has_error_union"] or traits["has_optional"]:
        lines.append(
            "- **Explicit error & optional handling**: Uses CP error unions (`!T`) and optionals (`T?`) instead of exceptions for predictable control flow."
        )
    if traits["has_deinit"]:
        lines.append(
            "- **Deterministic resource cleanup**: Native handles provide RAII `__cp_deinit` or explicit `defer` release functions to prevent leaks."
        )
    lines.append(
        "- **Strict compile-time type & borrow checks**: Verified by `cpc` borrow checking (`&mut` for mutable references, immutable borrows by default)."
    )
    return "\n".join(lines).rstrip() + "\n"


def generate_readme_zh(
    pkg_name: str,
    version: str,
    desc: str,
    license_str: str,
    mod: ModuleExports,
    attribution: Optional[str],
) -> str:
    traits = analyze_traits(mod)
    desc_text = desc.strip() or f"CP 原生生态包 `{pkg_name}`。"
    lines: List[str] = [
        NAV_ZH,
        "",
        f"# {pkg_name}",
        "",
        desc_text,
        "",
    ]
    if attribution:
        lines.append("<!-- cap-source-attribution -->")
        lines.append(attribution)
        lines.append("")

    lines.extend(
        [
            "| 元信息 | 值 |",
            "| --- | --- |",
            f"| 包名 | `{pkg_name}` |",
            f"| 版本 | `{version}` |",
            f"| 许可证 | `{license_str}` |",
            "| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |",
            "",
            "## 安装与导入",
            "",
            "```bash",
            f"cap add {pkg_name}",
            "```",
            "",
            "```cp",
            format_import_block(pkg_name, mod, limit=8),
            "```",
            "",
            "## 核心 API",
            "",
        ]
    )

    if mod.consts:
        lines.extend(
            [
                "### 常量",
                "",
                "| 常量名 | 类型 / 值 |",
                "| --- | --- |",
            ]
        )
        for c in mod.consts[:30]:
            lines.append(f"| `{md_escape(c.name)}` | `{md_escape(c.type_str)}` |")
        if len(mod.consts) > 30:
            lines.append(f"\n*已展示前 30 个（共 {len(mod.consts)} 个常量），完整定义请见 [`src/index.d.cp`](./src/index.d.cp)。*")
        lines.append("")

    if mod.types or mod.enums or mod.errors:
        lines.extend(
            [
                "### 类型、枚举与错误集",
                "",
                "| 种类 | 名称 | 定义 |",
                "| --- | --- | --- |",
            ]
        )
        for t in mod.types:
            lines.append(f"| `type` | `{md_escape(t.name)}` | `= {md_escape(t.target)}` |")
        for e in mod.enums:
            vlist = ", ".join(e.variants[:8]) + (" ..." if len(e.variants) > 8 else "")
            lines.append(f"| `enum` | `{md_escape(e.name)}` | `{md_escape(vlist)}` |")
        for err in mod.errors:
            vlist = ", ".join(err.variants[:8]) + (" ..." if len(err.variants) > 8 else "")
            lines.append(f"| `error` | `{md_escape(err.name)}` | `{md_escape(vlist)}` |")
        lines.append("")

    if mod.classes:
        lines.extend(
            [
                "### 数据结构",
                "",
                "| 类型名 | 种类 | 字段与方法 |",
                "| --- | --- | --- |",
            ]
        )
        for cls in mod.classes[:30]:
            parts = []
            if cls.fields:
                f_preview = ", ".join(cls.fields[:6])
                if len(cls.fields) > 6:
                    f_preview += f", +{len(cls.fields) - 6} 项"
                parts.append(f"字段: `{md_escape(f_preview)}`")
            if cls.methods:
                m_names = [re.sub(r"^(static\s+|pub\s+)?fn\s+", "", m).split("(")[0] for m in cls.methods[:6]]
                m_preview = ", ".join(m_names)
                if len(cls.methods) > 6:
                    m_preview += f", +{len(cls.methods) - 6} 项"
                parts.append(f"方法: `{md_escape(m_preview)}`")
            summary = " · ".join(parts) if parts else "数据容器"
            lines.append(f"| `{md_escape(cls.name)}` | `{cls.kind}` | {summary} |")
        if len(mod.classes) > 30:
            lines.append(f"\n*已展示前 30 个（共 {len(mod.classes)} 个类型），完整定义请见 [`src/index.d.cp`](./src/index.d.cp)。*")
        lines.append("")

    if mod.functions:
        lines.extend(
            [
                "### 函数列表",
                "",
                "| 函数名 | 签名 | 说明 |",
                "| --- | --- | --- |",
            ]
        )
        fn_cap = 60
        for fn in mod.functions[:fn_cap]:
            sig = f"({fn.params}): {fn.ret}" if fn.ret != "void" else f"({fn.params})"
            lines.append(
                f"| `{md_escape(fn.name)}` | `{md_escape(sig)}` | {md_escape(infer_fn_desc_zh(fn))} |"
            )
        if len(mod.functions) > fn_cap:
            lines.append(
                f"\n*已展示前 {fn_cap} 个（共 {len(mod.functions)} 个导出函数），完整签名请见 [`src/index.d.cp`](./src/index.d.cp)。*"
            )
        lines.append("")

    lines.extend(
        [
            "## 最小调用示例",
            "",
            "```cp",
            build_minimal_example_cp(pkg_name, mod),
            "```",
            "",
            "## CP 零堆与借用安全亮点",
            "",
        ]
    )
    if traits["has_c_ffi"]:
        lines.append(
            "- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。"
        )
    else:
        lines.append(
            "- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。"
        )
    if traits["has_out_buf"]:
        lines.append(
            "- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。"
        )
    if traits["has_string_slice"]:
        lines.append(
            "- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。"
        )
    if traits["has_error_union"] or traits["has_optional"]:
        lines.append(
            "- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。"
        )
    if traits["has_deinit"]:
        lines.append(
            "- **确定性资源释放**：底层句柄支持 RAII `__cp_deinit` 或 `defer` 显式释放，防止句柄与内存泄漏。"
        )
    lines.append(
        "- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。"
    )
    return "\n".join(lines).rstrip() + "\n"


def generate_readme_agent(
    pkg_name: str,
    version: str,
    desc: str,
    license_str: str,
    mod: ModuleExports,
) -> str:
    """
    Generate a high-density AI Agent Quick Ref card strictly within 35..75 lines.
    Caps exported functions at most 40 (dynamically trimmed if total lines > 75).
    """
    traits = analyze_traits(mod)
    desc_one = collapse_ws(desc) or f"CP native package `{pkg_name}`."

    def render(max_fns: int, max_types: int, max_consts: int, include_extra_notes: bool) -> List[str]:
        out: List[str] = [
            f"# {pkg_name} — AI Agent Quick Ref",
            "",
            f"{desc_one} (`v{version}`, `{license_str}`)",
            "",
            "## Import",
            "",
            "```cp",
            format_import_block(pkg_name, mod, limit=8),
            "```",
            "",
        ]

        # Types / Consts section
        type_lines: List[str] = []
        for c in mod.consts[:max_consts]:
            type_lines.append(f"const {c.name}: {c.type_str};")
        if len(mod.consts) > max_consts:
            type_lines.append(f"// ... +{len(mod.consts) - max_consts} more consts in src/index.d.cp")
        for t in mod.types:
            type_lines.append(f"type {t.name} = {t.target};")
        for err in mod.errors[:max_types]:
            type_lines.append(f"error {err.name} {{ {', '.join(err.variants[:6])} }}")
        for e in mod.enums[:max_types]:
            v = ", ".join(e.variants[:6]) + (", ..." if len(e.variants) > 6 else "")
            type_lines.append(f"enum {e.name} {{ {v} }}")
        for cls in mod.classes[:max_types]:
            flds = ", ".join(cls.fields[:4])
            if len(cls.fields) > 4:
                flds += f", /* +{len(cls.fields) - 4} fields */"
            if cls.methods:
                m_cnt = len(cls.methods)
                m_first = re.sub(r"^(static\s+|pub\s+)?fn\s+", "", cls.methods[0]).split("(")[0]
                meth = f" // methods: {m_first}(...)" + (f" +{m_cnt - 1}" if m_cnt > 1 else "")
            else:
                meth = ""
            type_lines.append(f"{cls.kind} {cls.name} {{ {flds} }}{meth}")
        if len(mod.classes) > max_types:
            type_lines.append(f"// ... +{len(mod.classes) - max_types} more classes/structs in src/index.d.cp")

        if type_lines:
            out.extend(["## Core Types & Constants", "", "```cp"])
            out.extend(type_lines)
            out.extend(["```", ""])

        # Functions section
        if mod.functions:
            out.extend(["## API Signatures", "", "```cp"])
            for fn in mod.functions[:max_fns]:
                out.append(f"{fn.raw};")
            if len(mod.functions) > max_fns:
                out.append(
                    f"// ... +{len(mod.functions) - max_fns} more functions — see src/index.d.cp for full signatures"
                )
            out.extend(["```", ""])

        # Minimal example
        out.extend(
            [
                "## Minimal Usage",
                "",
                "```cp",
                build_minimal_example_cp(pkg_name, mod),
                "```",
                "",
                "## Borrowing, Errors & Pitfalls",
                "",
            ]
        )

        # Dynamic pitfalls tailored to package traits
        out.append(f'- **Canonical import**: Always import directly from `"{pkg_name}"`; install via `cap add {pkg_name}`.')
        if traits["has_out_buf"]:
            out.append("- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.")
        if traits["has_string_slice"]:
            out.append("- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.")
        if traits["has_error_union"]:
            out.append("- **Error unions (`!T`)**: Handle fallible calls with `try`, `catch`, or `?` error propagation (CP has no `try/catch` blocks).")
        if traits["has_optional"]:
            out.append("- **Optionals (`T?`)**: Unwrap nullable results using `??` fallback or `if (val) |v| { ... }`.")
        if traits["has_mut_borrow"]:
            out.append("- **Mutable borrows**: Parameters typed `&mut T` require mutable bindings (`let mut x = ...`).")
        if traits["has_c_ffi"]:
            out.append("- **Native FFI safety**: Ensure C pointers are non-null and released deterministically via `defer` or `__cp_deinit`.")
        if include_extra_notes:
            out.append(f"- **Full declarations**: Inspect [`src/index.d.cp`](./src/index.d.cp) for complete field layouts, methods, and exact parameter types.")
            out.append("- **Compile check**: Validate integration with `cpc check` before building; avoid `any` or implicit casts.")
            out.append("- **Control flow syntax**: Enclose `if (...)` / `while (...)` conditions in parentheses per CP syntax rules.")

        return out

    # Start with generous caps (functions <= 40), then adjust to hit 38..75 lines strictly
    max_fns = min(40, len(mod.functions))
    max_types = min(12, max(len(mod.classes), len(mod.enums), len(mod.errors), 1))
    max_consts = min(8, len(mod.consts))

    lines = render(max_fns, max_types, max_consts, include_extra_notes=False)

    # Shrink if > 75 lines
    while len(lines) > 75 and (max_fns > 6 or max_types > 2 or max_consts > 2):
        if max_fns > 12:
            max_fns = max(12, max_fns - (len(lines) - 70))
        elif max_types > 4:
            max_types = max(4, max_types - 2)
        elif max_consts > 2:
            max_consts = max(2, max_consts - 2)
        elif max_fns > 6:
            max_fns -= 2
        else:
            max_types = max(2, max_types - 1)
        lines = render(max_fns, max_types, max_consts, include_extra_notes=False)

    # If still > 75 (e.g. huge class list), trim aggressively
    if len(lines) > 75:
        lines = render(min(max_fns, 5), 2, 1, include_extra_notes=False)

    # Expand if < 38 lines (still stay <= 75)
    if len(lines) < 38:
        lines = render(max_fns, max_types, max_consts, include_extra_notes=True)

    # If a tiny package (e.g. 1 fn, 0 classes) is still < 36 lines, add concise Build & Verification block
    if len(lines) < 36:
        lines.extend(
            [
                "",
                "## Quick Verification",
                "",
                "```bash",
                f"cap add {pkg_name}",
                "cpc check src/main.cp",
                "```",
            ]
        )

    return "\n".join(lines).rstrip() + "\n"


def classify_package(pkg_name: str, pkg_dir: Path) -> str:
    """
    Returns one of:
      - 'skip_batch16': Batch 16 in-flight package
      - 'skip_protected': Manually polished EN/ZH/AGENT
      - 'agent_only': Manually polished EN/ZH (>=60 lines + Quick Ref nav), only enrich README_AGENT.md
      - 'enrich_all': Enrich EN + ZH + AGENT
    """
    if pkg_name in BATCH16_SKIP:
        return "skip_batch16"

    rm = pkg_dir / "README.md"
    ra = pkg_dir / "README_AGENT.md"
    rm_lines = rm.read_text(encoding="utf-8", errors="ignore").splitlines() if rm.exists() else []
    ra_lines = ra.read_text(encoding="utf-8", errors="ignore").splitlines() if ra.exists() else []

    head = "\n".join(rm_lines[:5])
    has_quick_ref_nav = (
        "[AI Agent Quick Ref](./README_AGENT.md)" in head
        or "[AI Agent Quick Ref](README_AGENT.md)" in head
    )

    if has_quick_ref_nav and len(ra_lines) >= 38:
        return "skip_protected"
    if has_quick_ref_nav and len(rm_lines) >= 60:
        return "agent_only"
    return "enrich_all"


def sanitize_stale_cap_refs(text: str, native_cap_pkgs: set) -> str:
    def _repl(m: re.Match) -> str:
        ref = m.group(0)
        if ref in native_cap_pkgs:
            return ref
        return ref[len("@cap/"):]
    return re.sub(r"@cap/[A-Za-z0-9_.-]+", _repl, text)


def verify_no_stale_cap_ref(pkg_name: str, content: str, native_cap_pkgs: set) -> None:
    if pkg_name.startswith("@cap/"):
        return
    for ref in re.findall(r"@cap/[A-Za-z0-9_.-]+", content):
        if ref not in native_cap_pkgs:
            raise ValueError(f"Detected stale @cap reference '{ref}' while generating docs for '{pkg_name}'")


def run_self_audit(pkgs: List[Tuple[str, Path]], native_cap_pkgs: set) -> dict:
    total = len(pkgs)
    has_all_three = 0
    has_nav_en = 0
    has_nav_zh = 0
    stale_cap_refs: List[Tuple[str, str, str]] = []
    agent_in_range = 0
    agent_lt_35 = []
    agent_gt_75 = []

    for name, p in pkgs:
        rm = p / "README.md"
        rz = p / "README.zh-CN.md"
        ra = p / "README_AGENT.md"
        if rm.exists() and rz.exists() and ra.exists():
            if rm.stat().st_size > 0 and rz.stat().st_size > 0 and ra.stat().st_size > 0:
                has_all_three += 1

        if rm.exists():
            rm_txt = rm.read_text(encoding="utf-8", errors="ignore")
            head = "\n".join(rm_txt.splitlines()[:5])
            if "[AI Agent Quick Ref](./README_AGENT.md)" in head or "[AI Agent Quick Ref](README_AGENT.md)" in head:
                has_nav_en += 1
        if rz.exists():
            rz_txt = rz.read_text(encoding="utf-8", errors="ignore")
            head_z = "\n".join(rz_txt.splitlines()[:5])
            if "README_AGENT.md" in head_z and "README.md" in head_z:
                has_nav_zh += 1
        if ra.exists():
            ra_lines = len(ra.read_text(encoding="utf-8", errors="ignore").splitlines())
            if 35 <= ra_lines <= 75:
                agent_in_range += 1
            elif ra_lines < 35:
                agent_lt_35.append((name, ra_lines))
            else:
                agent_gt_75.append((name, ra_lines))

        if not name.startswith("@cap/"):
            for fn in ("README.md", "README.zh-CN.md", "README_AGENT.md"):
                fp = p / fn
                if fp.exists():
                    txt = fp.read_text(encoding="utf-8", errors="ignore")
                    for ref in re.findall(r"@cap/[A-Za-z0-9_.-]+", txt):
                        if ref not in native_cap_pkgs:
                            stale_cap_refs.append((name, fn, ref))

    return {
        "total": total,
        "has_all_three": has_all_three,
        "has_nav_en": has_nav_en,
        "has_nav_zh": has_nav_zh,
        "stale_cap_refs": stale_cap_refs,
        "agent_in_range": agent_in_range,
        "agent_lt_35": agent_lt_35,
        "agent_gt_75": agent_gt_75,
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Enrich mid/long-tail CP package docs from AST/d.cp.")
    parser.add_argument("--dry-run", action="store_true", help="Report planned actions without writing files.")
    args = parser.parse_args()

    pkgs = discover_packages()
    native_cap_pkgs = {name for name, _ in pkgs if name.startswith("@cap/")}

    stats = {
        "skip_batch16": 0,
        "skip_protected": 0,
        "agent_only": 0,
        "enrich_all": 0,
        "files_written": 0,
    }

    for pkg_name, pkg_dir in pkgs:
        action = classify_package(pkg_name, pkg_dir)
        stats[action] += 1
        if action in ("skip_batch16", "skip_protected"):
            continue

        meta = json.loads((pkg_dir / "cap.json").read_text(encoding="utf-8"))
        real_name = meta.get("name") or pkg_name
        version = str(meta.get("version") or meta.get("ver") or "1.0.0")
        desc = str(meta.get("description") or meta.get("desc") or "").strip()
        desc = sanitize_stale_cap_refs(desc, native_cap_pkgs)
        license_str = str(meta.get("license") or "MIT").strip()

        mod = load_exports(pkg_dir)
        attribution = extract_existing_attribution(pkg_dir / "README.md", native_cap_pkgs)

        agent_doc = generate_readme_agent(real_name, version, desc, license_str, mod)
        verify_no_stale_cap_ref(real_name, agent_doc, native_cap_pkgs)

        if action == "agent_only":
            if not args.dry_run:
                (pkg_dir / "README_AGENT.md").write_text(agent_doc, encoding="utf-8")
            stats["files_written"] += 1
        elif action == "enrich_all":
            en_doc = generate_readme_en(real_name, version, desc, license_str, mod, attribution)
            zh_doc = generate_readme_zh(real_name, version, desc, license_str, mod, attribution)
            verify_no_stale_cap_ref(real_name, en_doc, native_cap_pkgs)
            verify_no_stale_cap_ref(real_name, zh_doc, native_cap_pkgs)
            if not args.dry_run:
                (pkg_dir / "README.md").write_text(en_doc, encoding="utf-8")
                (pkg_dir / "README.zh-CN.md").write_text(zh_doc, encoding="utf-8")
                (pkg_dir / "README_AGENT.md").write_text(agent_doc, encoding="utf-8")
            stats["files_written"] += 3

    print("=== Enrichment Summary ===")
    print(f"Total packages scanned : {len(pkgs)}")
    print(f"Skipped (Batch 16)     : {stats['skip_batch16']}")
    print(f"Skipped (Protected)    : {stats['skip_protected']}")
    print(f"Enriched (AGENT only)  : {stats['agent_only']}")
    print(f"Enriched (EN+ZH+AGENT) : {stats['enrich_all']}")
    print(f"Total packages enriched: {stats['agent_only'] + stats['enrich_all']}")
    print(f"Total files written    : {stats['files_written']}" + (" (dry-run)" if args.dry_run else ""))

    if not args.dry_run:
        audit = run_self_audit(pkgs, native_cap_pkgs)
        print("\n=== Full-Repo Self-Audit ===")
        print(f"All 3 docs (EN/ZH/AGENT) present : {audit['has_all_three']} / {audit['total']}")
        print(f"EN top 3-tab nav coverage        : {audit['has_nav_en']} / {audit['total']} ({audit['has_nav_en']*100.0/audit['total']:.2f}%)")
        print(f"ZH top 3-tab nav coverage        : {audit['has_nav_zh']} / {audit['total']} ({audit['has_nav_zh']*100.0/audit['total']:.2f}%)")
        print(f"Stale @cap/* refs in bare pkgs   : {len(audit['stale_cap_refs'])}")
        print(f"README_AGENT in 35..75 lines     : {audit['agent_in_range']} / {audit['total']}")
        print(f"README_AGENT < 35 lines          : {len(audit['agent_lt_35'])} (samples: {audit['agent_lt_35'][:10]})")
        print(f"README_AGENT > 75 lines          : {len(audit['agent_gt_75'])} (samples: {audit['agent_gt_75'][:10]})")
        if audit["stale_cap_refs"]:
            print("ERROR: stale @cap refs found:", audit["stale_cap_refs"][:10])
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
