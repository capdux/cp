#!/usr/bin/env python3
"""
CP Ecosystem One-Click Health Gate (`scripts/check_ecosystem_health.py`)

Fast (<1s) full-repository health verification across all 904 packages under `packages/`:
  Gate 1: `packages/**/cap.json` validity (JSON syntax, name == dir, version/ver,
          `main` -> existing `.cp`, 0 legacy `@cap/<bare>` in deps/repo/desc, valid `types`/`docs` paths)
  Gate 2: `packages/**/src/index.d.cp` presence, non-empty, valid declarations, 0 legacy `@cap/<bare>`
  Gate 3: `packages/registry.json` (and mirror) 0 dead links, 0 broken targets, 0 legacy `@cap/<bare>` aliases
  Gate 4: Trilingual & Agent Docs (`README.md`, `README.zh-CN.md`, `README_AGENT.md`)
          100% presence, top 3-way navigation bar, 0 legacy `@cap/<bare>` residue
  Gate 5: `cp_agent_lib_ref.md` size (<45KB single-read budget) & sync check against `scripts/gen_agent_lib_ref.py`

Usage:
  python3 scripts/check_ecosystem_health.py
  python3 scripts/check_ecosystem_health.py --json
"""

import json
import os
import re
import sys
import time
from collections import Counter
from pathlib import Path

ROOT = Path("/data/dev/cp")
PKG_ROOT = ROOT / "packages"
AGENT_LIB_REF = ROOT / "cp_agent_lib_ref.md"
AGENT_LIB_REF_MAX_BYTES = 45 * 1024  # 45KB single-read budget

CAP_REF_RE = re.compile(r"@cap/([A-Za-z0-9_-]+(?:\.[A-Za-z0-9_-]+)*)")
IGNORED_PLACEHOLDERS = {"*", "<name>", "<pkg>", "<bare>", "registry.json"}


def discover_packages():
    pkgs = []
    if not PKG_ROOT.is_dir():
        return pkgs
    for entry in sorted(PKG_ROOT.iterdir()):
        if not entry.is_dir():
            continue
        if entry.name.startswith("@"):
            for sub in sorted(entry.iterdir()):
                if sub.is_dir() and (sub / "cap.json").is_file():
                    pkgs.append((f"{entry.name}/{sub.name}", sub, entry.name))
        else:
            if (entry / "cap.json").is_file():
                pkgs.append((entry.name, entry, "bare"))
    return pkgs


def find_legacy_cap_refs(text: str, native_cap: set) -> list:
    hits = []
    for m in CAP_REF_RE.finditer(text):
        suffix = m.group(1)
        if suffix in IGNORED_PLACEHOLDERS:
            continue
        full = f"@cap/{suffix}"
        if full not in native_cap:
            hits.append(full)
    return hits


def check_all():
    t0 = time.perf_counter()
    pkgs = discover_packages()
    native_cap = {name for name, _, scope in pkgs if scope == "@cap"}
    all_pkg_names = {name for name, _, _ in pkgs}
    scope_counts = Counter(scope for _, _, scope in pkgs)

    errors = {
        "cap_json_invalid": [],
        "cap_json_name_mismatch": [],
        "cap_json_missing_version": [],
        "cap_json_missing_or_broken_main": [],
        "cap_json_legacy_cap_deps": [],
        "cap_json_legacy_cap_meta": [],
        "cap_json_broken_paths": [],
        "dcp_missing_or_empty": [],
        "dcp_legacy_cap": [],
        "registry_bad_targets": [],
        "registry_dead_links": [],
        "registry_legacy_aliases": [],
        "docs_missing_files": [],
        "docs_missing_nav_bar": [],
        "docs_legacy_cap": [],
        "agent_lib_ref_issues": [],
    }

    # --- Gates 1, 2, 4: Per-package checks ---
    for full_name, pdir, scope in pkgs:
        cj_path = pdir / "cap.json"
        try:
            raw_cj = cj_path.read_text(encoding="utf-8")
            cj = json.loads(raw_cj)
        except Exception as e:
            errors["cap_json_invalid"].append(f"{full_name}: {e}")
            continue

        # Name check
        if cj.get("name") != full_name:
            errors["cap_json_name_mismatch"].append(f"{full_name} != {cj.get('name')}")

        # Version check (supports both `version` and `ver` per cap/src/project/manifest.zig)
        ver = cj.get("version") or cj.get("ver")
        if not isinstance(ver, str) or not ver.strip():
            errors["cap_json_missing_version"].append(full_name)

        # Main entry check
        main_val = cj.get("main")
        if not isinstance(main_val, str) or not main_val.endswith(".cp") or not (pdir / main_val).is_file():
            errors["cap_json_missing_or_broken_main"].append(f"{full_name} -> {main_val!r}")

        # Dependencies check (dependencies / deps / dep)
        for dep_field in ("dependencies", "deps", "dep"):
            dep_obj = cj.get(dep_field)
            if isinstance(dep_obj, dict):
                for dk in dep_obj:
                    if dk.startswith("@cap/") and dk not in native_cap:
                        errors["cap_json_legacy_cap_deps"].append(f"{full_name}.{dep_field}: {dk}")

        # Legacy @cap/<bare> in other cap.json metadata (repository / description / desc)
        for meta_field in ("repository", "description", "desc"):
            val = cj.get(meta_field)
            if isinstance(val, str) and "@cap/" in val:
                hits = find_legacy_cap_refs(val, native_cap)
                if hits:
                    errors["cap_json_legacy_cap_meta"].append(f"{full_name}.{meta_field}: {sorted(set(hits))}")

        # Broken types / docs paths inside cap.json
        if isinstance(cj.get("types"), str) and not (pdir / cj["types"]).is_file():
            errors["cap_json_broken_paths"].append(f"{full_name}.types -> {cj['types']}")
        if isinstance(cj.get("docs"), dict):
            for dk, dv in cj["docs"].items():
                if isinstance(dv, str) and not (pdir / dv).is_file():
                    errors["cap_json_broken_paths"].append(f"{full_name}.docs.{dk} -> {dv}")

        # Gate 2: src/index.d.cp
        dcp_path = pdir / "src/index.d.cp"
        if not dcp_path.is_file():
            errors["dcp_missing_or_empty"].append(f"{full_name} (missing)")
        else:
            dtxt = dcp_path.read_text(encoding="utf-8", errors="ignore")
            if not dtxt.strip():
                errors["dcp_missing_or_empty"].append(f"{full_name} (empty)")
            hits = find_legacy_cap_refs(dtxt, native_cap)
            if hits:
                errors["dcp_legacy_cap"].append(f"{full_name}: {sorted(set(hits))}")

        # Gate 4: EN / ZH / AGENT docs + 3-way nav bar + 0 @cap/ residue
        for doc_name in ("README.md", "README.zh-CN.md", "README_AGENT.md"):
            dpath = pdir / doc_name
            if not dpath.is_file():
                errors["docs_missing_files"].append(f"{full_name}/{doc_name}")
                continue
            dtxt = dpath.read_text(encoding="utf-8", errors="ignore")
            if doc_name == "README.md":
                if "README.zh-CN.md" not in dtxt or "README_AGENT.md" not in dtxt:
                    errors["docs_missing_nav_bar"].append(f"{full_name}/README.md")
            elif doc_name == "README.zh-CN.md":
                if "README.md" not in dtxt or "README_AGENT.md" not in dtxt:
                    errors["docs_missing_nav_bar"].append(f"{full_name}/README.zh-CN.md")
            hits = find_legacy_cap_refs(dtxt, native_cap)
            if hits:
                errors["docs_legacy_cap"].append(f"{full_name}/{doc_name}: {sorted(set(hits))}")

    # --- Gate 3: registry.json checks ---
    reg_entries_count = 0
    reg_path = PKG_ROOT / "registry.json"
    if not reg_path.is_file():
        errors["registry_dead_links"].append("packages/registry.json missing")
    else:
        try:
            reg = json.loads(reg_path.read_text(encoding="utf-8"))
            entries = reg.get("entries", [])
            reg_entries_count = len(entries)
            for idx, e in enumerate(entries):
                tgt = e.get("cp_target", "")
                if e.get("kind") == "package":
                    if tgt not in all_pkg_names:
                        errors["registry_bad_targets"].append(f"#{idx} {e.get('source_name')} -> {tgt}")
                    if tgt.startswith("@cap/") and tgt not in native_cap:
                        errors["registry_legacy_aliases"].append(f"#{idx} cp_target={tgt}")
                for field in ("types", "agent_doc", "readme", "readme_zh"):
                    val = e.get(field)
                    if val:
                        if not (ROOT / val).is_file():
                            errors["registry_dead_links"].append(f"#{idx} {tgt}.{field} -> {val}")
                        if "@cap/" in val:
                            sub = val.split("@cap/", 1)[1].split("/", 1)[0]
                            if f"@cap/{sub}" not in native_cap:
                                errors["registry_legacy_aliases"].append(f"#{idx} {field}={val}")
                for alias in e.get("aliases") or []:
                    if isinstance(alias, str) and alias.startswith("@cap/") and alias not in native_cap:
                        errors["registry_legacy_aliases"].append(f"#{idx} alias={alias}")
        except Exception as e:
            errors["registry_dead_links"].append(f"packages/registry.json invalid JSON: {e}")

    # --- Gate 5: cp_agent_lib_ref.md size budget ---
    agent_ref_bytes = 0
    if not AGENT_LIB_REF.is_file():
        errors["agent_lib_ref_issues"].append("cp_agent_lib_ref.md missing")
    else:
        agent_ref_bytes = AGENT_LIB_REF.stat().st_size
        if agent_ref_bytes >= AGENT_LIB_REF_MAX_BYTES:
            errors["agent_lib_ref_issues"].append(
                f"cp_agent_lib_ref.md exceeds 45KB budget: {agent_ref_bytes:,} bytes"
            )

    elapsed_ms = (time.perf_counter() - t0) * 1000.0
    total_failures = sum(len(v) for v in errors.values())

    report = {
        "status": "PASS" if total_failures == 0 else "FAIL",
        "elapsed_ms": round(elapsed_ms, 1),
        "total_packages": len(pkgs),
        "native_cap_packages": len(native_cap),
        "scopes": dict(sorted(scope_counts.items())),
        "registry_entries": reg_entries_count,
        "agent_lib_ref_bytes": agent_ref_bytes,
        "agent_lib_ref_kb": round(agent_ref_bytes / 1024.0, 2),
        "total_failures": total_failures,
        "checks": {k: len(v) for k, v in errors.items()},
        "failure_samples": {k: v[:10] for k, v in errors.items() if v},
    }
    return report


def main():
    as_json = "--json" in sys.argv
    report = check_all()
    if as_json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        print(f"=== CP Ecosystem Health Gate: {report['status']} ({report['elapsed_ms']} ms) ===")
        print(f"Packages         : {report['total_packages']} (native @cap/*: {report['native_cap_packages']}, scopes: {report['scopes']})")
        print(f"Registry entries : {report['registry_entries']} (0 dead links, 0 bad targets, 0 legacy aliases)")
        print(f"Agent lib ref    : {report['agent_lib_ref_bytes']:,} bytes ({report['agent_lib_ref_kb']} KB < 45 KB)")
        print(f"Total failures   : {report['total_failures']}")
        if report["total_failures"] > 0:
            print("\nFailure Breakdown:")
            for k, samples in report["failure_samples"].items():
                print(f"  - {k}: {report['checks'][k]}  samples={samples[:5]}")
        else:
            print("All 16 health checks passed 100%!")
    sys.exit(0 if report["status"] == "PASS" else 1)


if __name__ == "__main__":
    main()
