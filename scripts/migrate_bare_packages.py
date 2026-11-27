#!/usr/bin/env python3
"""
Phase 2 Ecosystem Reorganization Script (`scripts/migrate_bare_packages.py`)

Migrates npm & crates.io absorbed packages from `packages/@cap/<alias>/` to
their exact original names `packages/<original-name>/` (preserving original
versions, dropping `@cap/` prefix, NO legacy `@cap/<old>` aliases in registry.json):
  1. Fixes 5 mis-attributed `source.package` metadata entries (`fnv`, `pem`, `shlex`, `textwrap`, `envsubst`).
  2. Resolves duplicate pairs pointing to the same upstream package (keeps the fuller implementation).
  3. Moves npm/crates.io packages to `packages/<orig>/` (keeping 39 native C/C++/CP-only under `packages/@cap/`).
  4. Rewrites `cap.json` (`name`, `docs` navigation fields) & updates all import paths in `packages/` and `cap/test/`.
  5. Rebuilds `packages/registry.json` (and `packages/@cap/registry.json` mirror) with `types`/`agent_doc`/`readme`/`readme_zh` fields.

Usage:
  python3 scripts/migrate_bare_packages.py --dry-run
  python3 scripts/migrate_bare_packages.py --apply
"""

import json
import os
import re
import shutil
import sys
from collections import defaultdict
from pathlib import Path

ROOT = Path("/data/dev/cp")
PKG_ROOT = ROOT / "packages"
CAP_ROOT = PKG_ROOT / "@cap"
TEST_ROOT = ROOT / "cap/test"

# 5 packages whose cap.json source.package was mis-attributed to a related package
SOURCE_FIXES = {
    "fnv": ("crates.io", "fnv", "https://github.com/servo/rust-fnv"),
    "pem": ("crates.io", "pem", "https://github.com/jcreekmore/pem-rs"),
    "shlex": ("crates.io", "shlex", "https://github.com/comex/rust-shlex"),
    "textwrap": ("crates.io", "textwrap", "https://github.com/mgeisler/textwrap"),
    "envsubst": ("crates.io", "envsubst", "https://github.com/droundy/envsubst"),
}

# Explicit winner overrides for known duplicate pairs (loser will be removed after merging)
DUPLICATE_WINNERS = {
    "bytes": "bytes",           # already merged with bytes-buf by Worker P1-B
    "semver": "semver",         # merged from semver-crate + semver-range by Worker P1-B
}


def loc_of(pkg_dir: Path) -> int:
    cp = pkg_dir / "src/index.cp"
    if not cp.is_file():
        return 0
    return cp.read_text(encoding="utf-8", errors="ignore").count("\n") + 1


def plan_migration():
    entries = []
    for d in sorted(os.listdir(CAP_ROOT)):
        p = CAP_ROOT / d
        cj_p = p / "cap.json"
        if not p.is_dir() or not cj_p.is_file():
            continue
        cj = json.loads(cj_p.read_text(encoding="utf-8"))
        src = cj.get("source") or (cj.get("sources")[0] if isinstance(cj.get("sources"), list) and cj.get("sources") else {})
        eco = (src.get("ecosystem") or src.get("registry") or "").lower()
        orig = (src.get("package") or src.get("name") or "").strip()

        if d in SOURCE_FIXES:
            eco, orig, _ = SOURCE_FIXES[d]

        if eco in ("npm", "crates.io", "crates", "cargo") and orig:
            entries.append({
                "old_dir": d,
                "eco": "crates.io" if eco in ("crates.io", "crates", "cargo") else "npm",
                "target": orig,
                "loc": loc_of(p),
            })

    # Group by target (case-insensitive) to resolve duplicates
    by_target = defaultdict(list)
    for e in entries:
        by_target[e["target"].lower()].append(e)

    moves = {}      # old_dir -> target_name
    drops = {}      # old_dir -> winner_target_name (duplicate to remove)
    for t_low, group in sorted(by_target.items()):
        if len(group) == 1:
            g = group[0]
            moves[g["old_dir"]] = g["target"]
            continue

        # Pick winner: explicit override > exact name match if reasonably full > largest LOC
        winner = None
        if t_low in DUPLICATE_WINNERS:
            for g in group:
                if g["old_dir"] == DUPLICATE_WINNERS[t_low]:
                    winner = g
                    break
        if winner is None:
            group_sorted = sorted(group, key=lambda x: x["loc"], reverse=True)
            winner = group_sorted[0]

        moves[winner["old_dir"]] = winner["target"]
        for g in group:
            if g["old_dir"] != winner["old_dir"]:
                drops[g["old_dir"]] = winner["target"]

    return moves, drops


def update_imports_in_file(path: Path, old_to_new_rel: dict, dry_run: bool) -> bool:
    txt = path.read_text(encoding="utf-8", errors="ignore")
    orig_txt = txt

    # 1. Rewrite `../../packages/@cap/<old>/src/index.cp` in cap/test/*.cp
    def repl_test(m):
        old_name = m.group(1)
        if old_name in old_to_new_rel:
            new_target = old_to_new_rel[old_name]
            return f'../../packages/{new_target}/src/index.cp'
        return m.group(0)

    txt = re.sub(r'\.\./\.\./packages/@cap/([a-zA-Z0-9._-]+)/src/index\.cp', repl_test, txt)

    # 2. Rewrite `from "@cap/<old>"` -> `from "<new>"`
    def repl_scoped(m):
        old_name = m.group(1)
        if old_name in old_to_new_rel:
            return f'from "{old_to_new_rel[old_name]}"'
        return m.group(0)

    txt = re.sub(r'from\s+"@cap/([a-zA-Z0-9._-]+)"', repl_scoped, txt)

    # 3. Rewrite inter-package relative `../../<old>/src/index.cp` inside packages/
    if str(path).startswith(str(PKG_ROOT)):
        pkg_rel = path.relative_to(PKG_ROOT)
        is_scoped_pkg = pkg_rel.parts[0].startswith("@")
        prefix = "../../../" if is_scoped_pkg else "../../"

        def repl_inter(m):
            old_name = m.group(1)
            if old_name in old_to_new_rel:
                return f'{prefix}{old_to_new_rel[old_name]}/src/index.cp'
            return m.group(0)

        txt = re.sub(r'\.\./\.\./([a-zA-Z0-9._-]+)/src/index\.cp', repl_inter, txt)

    if txt != orig_txt:
        if not dry_run:
            path.write_text(txt, encoding="utf-8")
        return True
    return False


def main():
    apply = "--apply" in sys.argv
    dry_run = not apply

    moves, drops = plan_migration()
    print(f"Mode: {'APPLY' if apply else 'DRY-RUN'}")
    print(f"Packages to move to bare/original name : {len(moves)}")
    print(f"Duplicate packages to merge/drop       : {len(drops)}")
    if drops:
        for d, win in sorted(drops.items()):
            print(f"  DROP @cap/{d} -> superseded by {win}")

    # Combined old_dir -> new target mapping for import rewrites
    old_to_new = dict(moves)
    old_to_new.update(drops)

    if dry_run:
        print("\nDry-run complete. Pass --apply to execute.")
        return

    # 1. Fix 5 mis-attributed source metadata entries
    for d, (eco, pkg, repo) in SOURCE_FIXES.items():
        cj_p = CAP_ROOT / d / "cap.json"
        if cj_p.is_file():
            cj = json.loads(cj_p.read_text(encoding="utf-8"))
            src = cj.get("source", {})
            src["ecosystem"] = eco
            src["package"] = pkg
            src["repository"] = repo
            if "registry" in src:
                src["registry"] = eco
            if "name" in src:
                src["name"] = pkg
            cj["source"] = src
            cj_p.write_text(json.dumps(cj, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")

    # 2. Remove superseded duplicate directories
    for d in drops:
        p = CAP_ROOT / d
        if p.is_dir():
            shutil.rmtree(p)

    # 3. Move packages from packages/@cap/<old> -> packages/<target>
    moved_cnt = 0
    for old_d, target in sorted(moves.items()):
        src_dir = CAP_ROOT / old_d
        dst_dir = PKG_ROOT / target
        if not src_dir.is_dir():
            continue
        if dst_dir.exists() and dst_dir.resolve() != src_dir.resolve():
            print(f"  [WARN] Target already exists, skipping move: {src_dir} -> {dst_dir}")
            continue
        dst_dir.parent.mkdir(parents=True, exist_ok=True)
        shutil.move(str(src_dir), str(dst_dir))

        # Update cap.json name & docs fields
        cj_p = dst_dir / "cap.json"
        if cj_p.is_file():
            cj = json.loads(cj_p.read_text(encoding="utf-8"))
            cj["name"] = target
            cj["docs"] = {
                "types": "src/index.d.cp",
                "agent": "README_AGENT.md",
                "readme": "README.md",
                "readme_zh": "README.zh-CN.md",
            }
            # Rewrite @cap/<dep> in dependencies if present
            for dep_key in ("dependencies", "deps"):
                if isinstance(cj.get(dep_key), dict):
                    new_deps = {}
                    for dk, dv in cj[dep_key].items():
                        if dk.startswith("@cap/"):
                            bare_k = dk[5:]
                            new_k = old_to_new.get(bare_k, dk)
                            new_deps[new_k] = dv
                        else:
                            new_deps[dk] = dv
                    cj[dep_key] = new_deps
            cj_p.write_text(json.dumps(cj, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
        moved_cnt += 1

    # 4. Update all import paths in cap/test/*.cp and packages/**/*.cp
    updated_files = 0
    for tf in TEST_ROOT.glob("*.cp"):
        if update_imports_in_file(tf, old_to_new, dry_run=False):
            updated_files += 1
    for cp_file in PKG_ROOT.rglob("*.cp"):
        if cp_file.name.endswith(".d.cp"):
            continue
        if update_imports_in_file(cp_file, old_to_new, dry_run=False):
            updated_files += 1

    # 5. Rewrite registry.json (drop @cap/ prefix for migrated pkgs, add docs paths, NO legacy @cap aliases)
    reg_p = CAP_ROOT / "registry.json"
    reg = json.loads(reg_p.read_text(encoding="utf-8"))
    for e in reg.get("entries", []):
        ct = e.get("cp_target", "")
        if ct.startswith("@cap/"):
            old_k = ct[5:]
            if old_k in old_to_new:
                new_t = old_to_new[old_k]
                e["cp_target"] = new_t
                e["types"] = f"packages/{new_t}/src/index.d.cp"
                e["agent_doc"] = f"packages/{new_t}/README_AGENT.md"
                e["readme"] = f"packages/{new_t}/README.md"
                e["readme_zh"] = f"packages/{new_t}/README.zh-CN.md"
        elif ct.startswith("cap:"):
            mod = ct[4:]
            e["doc"] = f"doc/lib/{mod}.md"
    reg["updated_at"] = "2026-11-27"
    # Write to canonical packages/registry.json and keep packages/@cap/registry.json synced
    reg_text = json.dumps(reg, indent=2, ensure_ascii=False) + "\n"
    (PKG_ROOT / "registry.json").write_text(reg_text, encoding="utf-8")
    reg_p.write_text(reg_text, encoding="utf-8")

    print(f"Migration applied: moved={moved_cnt}, dropped_dups={len(drops)}, updated_import_files={updated_files}")


if __name__ == "__main__":
    main()
