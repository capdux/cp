#!/usr/bin/env python3
"""
Trilingual & AI-Agent Package Documentation + `.d.cp` Generator (`scripts/gen_pkg_docs.py`)

Ensures every package under `packages/` (bare packages, `@org/pkg`, `@cap/*`, `@web/*`) has:
  1. `src/index.d.cp` (auto-emitted via `cpc emit-types` if missing)
  2. `README.md` with top GitHub-style language switcher:
     `[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)`
  3. `README.zh-CN.md` (Chinese human-readable guide with exact API table & CP usage example)
  4. `README_AGENT.md` (ultra-compact <1.2KB AI-agent quickref for low-token lookup & `cap brief`)

Usage:
  python3 scripts/gen_pkg_docs.py --dry-run
  python3 scripts/gen_pkg_docs.py --apply
"""

import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path("/data/dev/cp")
PKG_ROOT = ROOT / "packages"
CPC_BIN = ROOT / "cap/bin/cpc"

LANG_BAR_EN = "[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)"
LANG_BAR_ZH = "[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)"


def discover_packages():
    pkgs = []
    for entry in sorted(PKG_ROOT.iterdir()):
        if not entry.is_dir():
            continue
        if entry.name.startswith("@"):
            for sub in sorted(entry.iterdir()):
                if sub.is_dir() and (sub / "cap.json").is_file():
                    pkgs.append((f"{entry.name}/{sub.name}", sub))
        else:
            if (entry / "cap.json").is_file():
                pkgs.append((entry.name, entry))
    return pkgs


def extract_exported_names(cp_text: str):
    """Extract explicitly exported symbol names from `export { ... };` block if present."""
    m = re.search(r"export\s*\{([^}]+)\}\s*;", cp_text, re.DOTALL)
    if not m:
        return None
    names = set()
    for tok in re.split(r"[,\s]+", m.group(1)):
        t = tok.strip()
        if t and re.match(r"^[A-Za-z_][A-Za-z0-9_]*$", t):
            names.add(t)
    return names if names else None


def extract_api_lines(dcp_text: str, exported_names=None):
    fns, types = [], []
    for line in dcp_text.splitlines():
        s = line.strip()
        if s.startswith("pub fn ") or s.startswith("export fn "):
            m = re.search(r"fn\s+([A-Za-z_][A-Za-z0-9_]*)", s)
            if exported_names is not None and m and m.group(1) not in exported_names:
                continue
            fns.append(s)
        elif s.startswith("pub struct ") or s.startswith("pub enum ") or s.startswith("pub const ") or s.startswith("pub type ") or s.startswith("export struct ") or s.startswith("export enum ") or s.startswith("export const "):
            m = re.search(r"(?:struct|enum|const|type)\s+([A-Za-z_][A-Za-z0-9_]*)", s)
            if exported_names is not None and m and m.group(1) not in exported_names:
                continue
            types.append(s)
    return types, fns


def first_fn_name(fns):
    for f in fns:
        m = re.search(r"fn\s+([A-Za-z_][A-Za-z0-9_]*)", f)
        if m:
            return m.group(1)
    return "example"


def build_agent_doc(pkg_name: str, cj: dict, dcp_text: str, exported_names=None) -> str:
    desc = (cj.get("description") or f"CP package {pkg_name}").strip()
    ver = cj.get("version") or cj.get("ver") or "1.0.0"
    lic = cj.get("license") or "MIT"
    deps = cj.get("dependencies") or cj.get("deps") or {}
    types, fns = extract_api_lines(dcp_text, exported_names)

    lines = [
        f"# {pkg_name}@{ver} (Agent Quickref)",
        "",
        f"> {desc} | License: {lic}",
        f"> Import: `import {{ ... }} from \"{pkg_name}\";` | Full types: `src/index.d.cp`",
    ]
    if deps:
        dep_str = ", ".join(f"`{k}`" for k in sorted(deps.keys()))
        lines.append(f"> Deps: {dep_str}")
    lines.append("")
    if types:
        lines.append("## Types & Constants")
        for t in types[:18]:
            lines.append(f"- `{t.rstrip(' {')}`")
        if len(types) > 18:
            lines.append(f"- *(+{len(types) - 18} more in `src/index.d.cp`)*")
        lines.append("")
    if fns:
        lines.append("## Functions")
        for f in fns[:24]:
            lines.append(f"- `{f}`")
        if len(fns) > 24:
            lines.append(f"- *(+{len(fns) - 24} more in `src/index.d.cp`)*")
        lines.append("")
    return "\n".join(lines).rstrip() + "\n"


def build_zh_doc(pkg_name: str, cj: dict, dcp_text: str, exported_names=None) -> str:
    desc = (cj.get("description") or f"CP 官方生态包 {pkg_name}").strip()
    ver = cj.get("version") or cj.get("ver") or "1.0.0"
    lic = cj.get("license") or "MIT"
    src = cj.get("source") or {}
    sources = cj.get("sources") or ([src] if src else [])
    types, fns = extract_api_lines(dcp_text, exported_names)
    sample_fn = first_fn_name(fns)

    lines = [
        LANG_BAR_ZH,
        "",
        f"# {pkg_name}",
        "",
        f"> 版本：`{ver}` | 许可证：`{lic}` | 类型声明：[src/index.d.cp](./src/index.d.cp)",
        "",
        desc,
        "",
        "## 安装与导入",
        "",
        "```bash",
        f"cap add {pkg_name}",
        "```",
        "",
        "```cp",
        f"import {{ {sample_fn} }} from \"{pkg_name}\";",
        "```",
        "",
    ]
    if types or fns:
        lines.append("## 核心导出 API")
        lines.append("")
        for t in types[:25]:
            lines.append(f"- `{t.rstrip(' {')}`")
        for f in fns[:40]:
            lines.append(f"- `{f}`")
        if len(types) > 25 or len(fns) > 40:
            lines.append("- 完整签名请参阅 [src/index.d.cp](./src/index.d.cp)")
        lines.append("")

    if sources:
        lines.append("<!-- cap-source-attribution -->")
        lines.append("## 上游来源与许可证说明")
        lines.append("")
        for s in sources:
            eco = s.get("ecosystem") or s.get("registry") or "upstream"
            spkg = s.get("package") or s.get("name") or pkg_name
            sver = s.get("version") or ver
            repo = s.get("repository") or ""
            slic = s.get("license") or lic
            repo_part = f"（{repo}）" if repo else ""
            lines.append(f"- **上游生态**：`{eco}` (`{spkg}@{sver}`){repo_part}，许可证：{slic}")
        lines.append("")

    return "\n".join(lines).rstrip() + "\n"


def ensure_lang_bar(readme_path: Path, dry_run: bool) -> bool:
    if not readme_path.is_file():
        return False
    txt = readme_path.read_text(encoding="utf-8", errors="ignore")
    if "README.zh-CN.md" in txt and "README_AGENT.md" in txt:
        return False
    new_txt = LANG_BAR_EN + "\n\n" + txt.lstrip()
    if not dry_run:
        readme_path.write_text(new_txt, encoding="utf-8")
    return True


def main():
    apply = "--apply" in sys.argv
    force = "--force" in sys.argv
    dry_run = not apply
    pkgs = discover_packages()

    emitted_dcp = 0
    failed_dcp = 0
    updated_en = 0
    written_zh = 0
    written_agent = 0

    for pkg_name, pdir in pkgs:
        cj = json.loads((pdir / "cap.json").read_text(encoding="utf-8"))
        cp_path = pdir / "src/index.cp"
        dcp_path = pdir / "src/index.d.cp"

        # 1. Emit missing index.d.cp
        if not dcp_path.is_file() and cp_path.is_file():
            if not dry_run:
                res = subprocess.run(
                    [str(CPC_BIN), "emit-types", str(cp_path)],
                    capture_output=True, text=True
                )
                if res.returncode == 0 and res.stdout.strip():
                    dcp_path.write_text(res.stdout, encoding="utf-8")
                    emitted_dcp += 1
                elif dcp_path.is_file():
                    emitted_dcp += 1
                else:
                    failed_dcp += 1
            else:
                emitted_dcp += 1

        cp_text = cp_path.read_text(encoding="utf-8", errors="ignore") if cp_path.is_file() else ""
        exported_names = extract_exported_names(cp_text)
        dcp_text = dcp_path.read_text(encoding="utf-8", errors="ignore") if dcp_path.is_file() else ""

        # 2. Ensure language bar in README.md
        if ensure_lang_bar(pdir / "README.md", dry_run=dry_run):
            updated_en += 1

        # 3. Generate README.zh-CN.md
        zh_path = pdir / "README.zh-CN.md"
        if force or not zh_path.is_file():
            if not dry_run:
                zh_path.write_text(build_zh_doc(pkg_name, cj, dcp_text, exported_names), encoding="utf-8")
            written_zh += 1

        # 4. Generate README_AGENT.md
        ag_path = pdir / "README_AGENT.md"
        if force or not ag_path.is_file():
            if not dry_run:
                ag_path.write_text(build_agent_doc(pkg_name, cj, dcp_text, exported_names), encoding="utf-8")
            written_agent += 1

    print(
        f"Mode={'APPLY' if apply else 'DRY-RUN'} | total={len(pkgs)} | "
        f"emitted_dcp={emitted_dcp} (failed={failed_dcp}) | "
        f"updated_en={updated_en} | written_zh={written_zh} | written_agent={written_agent}"
    )


if __name__ == "__main__":
    main()
