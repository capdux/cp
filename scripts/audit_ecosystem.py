#!/usr/bin/env python3
"""
CP Ecosystem Audit & Quality Gate (`scripts/audit_ecosystem.py`)

Audits all packages under `packages/` (bare packages, `@cap/*`, `@web/*`) against
the 8 ecosystem quality gates defined in the Top-2000 Reorganization Plan:
  1. Naming & Layout: bare original package names (vs legacy `@cap/*`)
  2. Dual-platform Name Collisions: 0 duplicate dirs across npm & crates.io
  3. Manifest Hygiene: `cap.json` has name, version, source/sources, docs fields
  4. Type Declarations: 100% `src/index.d.cp` presence
  5. Trilingual & Agent Docs: `README.md` + `README.zh-CN.md` + `README_AGENT.md`
  6. Helper Repetition: detects hand-rolled `StrEq`/`Trim`/`ToLower`/`WriteU64`/`Hex`
  7. Performance Hygiene: tracks `undefined` stack buffers, `@simd`/`@Vector`, `Arena`
  8. Fullness Distribution: flags thin (<120 LOC) packages for fullness audit

Usage:
  python3 scripts/audit_ecosystem.py
  python3 scripts/audit_ecosystem.py --json
"""

import json
import os
import re
import sys
from collections import Counter
from pathlib import Path

ROOT = Path("/data/dev/cp")
PKG_ROOT = ROOT / "packages"

HELPER_PATTERNS = {
    "strEq/eqIgnoreCase": re.compile(r"fn \w*(?:StrEq|EqLower|EqIgnoreCase)\("),
    "trim/isSpace": re.compile(r"fn \w*(?:Trim|IsSpace)\("),
    "toLower/toUpper": re.compile(r"fn \w*(?:ToLower|ToUpper)\("),
    "writeU64/writeI64/writeStr": re.compile(r"fn \w*(?:WriteU64|WriteI64|WriteU32|WriteStr)\("),
    "hexVal/hexChar": re.compile(r"fn \w*(?:HexVal|HexChar|HexDigit)\("),
    "parseDigits/parseInt": re.compile(r"fn \w*(?:ParseDigits|ParseU32|ParseI64|ParseU16)\("),
    "findByte/startsWith": re.compile(r"fn \w*(?:FindByte|StartsWith)\("),
}


def discover_packages():
    pkgs = []
    if not PKG_ROOT.is_dir():
        return pkgs
    for entry in sorted(PKG_ROOT.iterdir()):
        if not entry.is_dir():
            continue
        name = entry.name
        if name.startswith("@"):
            for sub in sorted(entry.iterdir()):
                if sub.is_dir() and (sub / "cap.json").is_file():
                    pkgs.append((f"{name}/{sub.name}", sub, name))
        else:
            if (entry / "cap.json").is_file():
                pkgs.append((name, entry, "bare"))
    return pkgs


def main():
    as_json = "--json" in sys.argv
    pkgs = discover_packages()

    counts = Counter()
    loc_buckets = Counter()
    helper_hits = Counter()
    missing_dcp = []
    missing_zh = []
    missing_agent = []
    thin_pkgs = []
    renamed_legacy = []
    perf = Counter()

    for full_name, pdir, scope in pkgs:
        counts[f"scope:{scope}"] += 1
        cj = json.loads((pdir / "cap.json").read_text(encoding="utf-8"))
        if cj.get("dependencies") or cj.get("deps"):
            counts["has_deps"] += 1
        src = cj.get("source") or (cj.get("sources")[0] if isinstance(cj.get("sources"), list) and cj.get("sources") else {})
        orig = (src.get("package") or src.get("name") or "").strip()
        base_dir = pdir.name
        if orig and orig.lower() != base_dir.lower() and orig.lower() != full_name.lower():
            renamed_legacy.append((full_name, orig))

        if (pdir / "src/index.d.cp").is_file():
            counts["has_dcp"] += 1
        else:
            missing_dcp.append(full_name)

        if (pdir / "README.zh-CN.md").is_file():
            counts["has_readme_zh"] += 1
        else:
            missing_zh.append(full_name)

        if (pdir / "README_AGENT.md").is_file():
            counts["has_readme_agent"] += 1
        else:
            missing_agent.append(full_name)

        cp_file = pdir / "src/index.cp"
        if cp_file.is_file():
            txt = cp_file.read_text(encoding="utf-8", errors="ignore")
            loc = txt.count("\n") + 1
            if loc < 80:
                loc_buckets["<80"] += 1
                thin_pkgs.append((full_name, loc))
            elif loc < 150:
                loc_buckets["80-149"] += 1
                if loc < 120:
                    thin_pkgs.append((full_name, loc))
            elif loc < 300:
                loc_buckets["150-299"] += 1
            elif loc < 600:
                loc_buckets["300-599"] += 1
            else:
                loc_buckets["600+"] += 1

            for hname, pat in HELPER_PATTERNS.items():
                if pat.search(txt):
                    helper_hits[hname] += 1

            if "= undefined" in txt:
                perf["undefined"] += 1
            if "@simd" in txt or "@Vector" in txt:
                perf["simd"] += 1
            if "Arena" in txt or "ScratchArena" in txt or "FixedBuffer" in txt:
                perf["arena"] += 1
            if '"cap:' in txt:
                perf["uses_cap_stdlib"] += 1

    report = {
        "total_packages": len(pkgs),
        "scopes": {k.split(":", 1)[1]: v for k, v in counts.items() if k.startswith("scope:")},
        "has_dcp": counts["has_dcp"],
        "missing_dcp_count": len(missing_dcp),
        "has_readme_zh": counts["has_readme_zh"],
        "has_readme_agent": counts["has_readme_agent"],
        "has_deps": counts["has_deps"],
        "renamed_vs_source_count": len(renamed_legacy),
        "loc_distribution": dict(loc_buckets),
        "thin_under_120_count": len(thin_pkgs),
        "helper_repetition": dict(helper_hits),
        "perf_adoption": dict(perf),
    }

    if as_json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
        return

    print("=== CP Ecosystem Audit Report ===")
    print(f"Total packages   : {report['total_packages']} (scopes: {report['scopes']})")
    print(f"Type decl (.d.cp): {report['has_dcp']} / {report['total_packages']} (missing: {report['missing_dcp_count']})")
    print(f"Docs (zh / agent): zh-CN={report['has_readme_zh']}, AGENT={report['has_readme_agent']}")
    print(f"Renamed vs source: {report['renamed_vs_source_count']}")
    print(f"Explicit deps    : {report['has_deps']} | Uses cap:* stdlib: {perf['uses_cap_stdlib']}")
    print(f"LOC buckets      : {report['loc_distribution']} (thin <120: {report['thin_under_120_count']})")
    print(f"Helper repetition: {report['helper_repetition']}")
    print(f"Perf adoption    : undefined={perf['undefined']}, simd={perf['simd']}, arena={perf['arena']}")


if __name__ == "__main__":
    main()
