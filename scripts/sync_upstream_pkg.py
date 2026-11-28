#!/usr/bin/env python3
"""
scripts/sync_upstream_pkg.py — CP 上游源码溯源审计与一键同步辅助工具

功能：
1. 扫描指定包（packages/<pkg>/src/*.cp 或 lib/<mod>/*.cp）的头部 `// Source: ... | Synced: YYYY-MM-DD` 注释；
2. 检查上游仓库（默认 _ref/<repo>）自上次 Synced 日期以来的 git 变更（可选 --pull 先 git pull）；
3. 支持 --preview-transpile 将上游变更文件用 `cpc from --ts` 预转译至 `temp/sync_preview/<pkg>/`，生成变更合并清单；
4. 支持 --stamp 更新 `// Source:` 的 `Synced:` 日期并刷新包内 `MIGRATE.md` 的同步时间戳。
"""

import argparse
import datetime
import json
import os
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
SOURCE_RE = re.compile(r"^//\s*Source:\s*(?:Absorbed from\s+)?([^|\n]+?)(?:\s*\|\s*Synced:\s*([0-9-]+))?\s*$")


def resolve_pkg_dir(pkg: str) -> Path:
    if pkg.startswith("cap:"):
        name = pkg[4:]
        return ROOT / "lib" / name
    p = ROOT / "packages" / pkg
    if p.is_dir():
        return p
    l = ROOT / "lib" / pkg
    if l.is_dir():
        return l
    raise FileNotFoundError(f"Package or stdlib dir not found for '{pkg}'")


def detect_upstream_dir(pkg_dir: Path, override: str | None) -> Path | None:
    if override:
        p = Path(override)
        return p if p.is_absolute() else ROOT / p
    cap_json = pkg_dir / "cap.json"
    if cap_json.is_file():
        try:
            data = json.loads(cap_json.read_text(encoding="utf-8"))
            src = data.get("source", {})
            name = src.get("name", "")
            if "puppeteer" in name:
                cand = ROOT / "_ref/puppeteer/packages/puppeteer-core"
                if cand.is_dir():
                    return cand
            if "playwright" in name:
                cand = ROOT / "_ref/playwright/packages/playwright-core"
                if cand.is_dir():
                    return cand
        except Exception:
            pass
    return None


def find_git_root(p: Path) -> Path | None:
    cur = p.resolve()
    while cur != cur.parent:
        if (cur / ".git").exists():
            return cur
        cur = cur.parent
    return None


def scan_cp_sources(pkg_dir: Path) -> list[dict]:
    src_root = pkg_dir / "src" if (pkg_dir / "src").is_dir() else pkg_dir
    records = []
    for cp_file in sorted(src_root.rglob("*.cp")):
        lines = cp_file.read_text(encoding="utf-8", errors="ignore").splitlines()[:8]
        source_spec = None
        synced_at = None
        for line in lines:
            m = SOURCE_RE.match(line.strip())
            if m:
                source_spec = m.group(1).strip()
                synced_at = m.group(2).strip() if m.group(2) else None
                break
        records.append({
            "cp_file": cp_file,
            "rel": str(cp_file.relative_to(ROOT)),
            "source": source_spec,
            "synced_at": synced_at,
        })
    return records


def git_changed_files_since(git_root: Path, sub_rel: str, since_date: str) -> list[str]:
    cmd = [
        "git", "-C", str(git_root), "log",
        f"--since={since_date}",
        "--name-only", "--pretty=format:", "--", sub_rel,
    ]
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        return []
    files = sorted({line.strip() for line in res.stdout.splitlines() if line.strip()})
    return files


def main() -> int:
    ap = argparse.ArgumentParser(description="Audit and sync CP packages with upstream repos")
    ap.add_argument("--pkg", required=True, help="Package name (e.g. puppeteer, playwright, cap:cdp)")
    ap.add_argument("--upstream", default=None, help="Override upstream dir path (e.g. _ref/puppeteer/packages/puppeteer-core)")
    ap.add_argument("--pull", action="store_true", help="Run git pull --ff-only in upstream git repo first")
    ap.add_argument("--preview-transpile", action="store_true", help="Transpile changed upstream TS files to temp/sync_preview/<pkg>/")
    ap.add_argument("--stamp", action="store_true", help="Update Synced: YYYY-MM-DD in .cp headers to today")
    args = ap.parse_args()

    pkg_dir = resolve_pkg_dir(args.pkg)
    upstream_dir = detect_upstream_dir(pkg_dir, args.upstream)
    today = datetime.date.today().isoformat()

    print(f"=== Sync Audit: {args.pkg} ({pkg_dir.relative_to(ROOT)}) ===")
    if upstream_dir:
        print(f"Upstream dir : {upstream_dir}")
        git_root = find_git_root(upstream_dir)
        if args.pull and git_root:
            print(f"Pulling upstream repo {git_root} ...")
            subprocess.run(["git", "-C", str(git_root), "pull", "--ff-only"], check=False)
    else:
        git_root = None
        print("Upstream dir : (not detected)")

    records = scan_cp_sources(pkg_dir)
    with_header = [r for r in records if r["source"]]
    missing_header = [r for r in records if not r["source"]]
    print(f"CP files     : {len(records)} total | {len(with_header)} with // Source: | {len(missing_header)} missing header")
    if missing_header:
        for r in missing_header[:10]:
            print(f"  [WARN missing // Source:] {r['rel']}")

    since_dates = sorted({r["synced_at"] for r in with_header if r["synced_at"]})
    earliest_sync = since_dates[0] if since_dates else today
    print(f"Earliest Sync: {earliest_sync}")

    changed_upstream: list[str] = []
    if upstream_dir and git_root:
        sub_rel = str(upstream_dir.resolve().relative_to(git_root))
        changed_upstream = git_changed_files_since(git_root, sub_rel, earliest_sync)
        print(f"Upstream git changes since {earliest_sync}: {len(changed_upstream)} file(s)")
        for f in changed_upstream[:20]:
            print(f"  - {f}")

    if args.preview_transpile and upstream_dir:
        preview_dir = ROOT / "temp" / "sync_preview" / args.pkg.replace("cap:", "lib_")
        preview_dir.mkdir(parents=True, exist_ok=True)
        src_target = upstream_dir / "src" if (upstream_dir / "src").is_dir() else upstream_dir
        print(f"Running cpc from --ts {src_target} -o {preview_dir} ...")
        subprocess.run(["cpc", "from", "--ts", str(src_target), "-o", str(preview_dir)], check=False)
        print(f"Preview written to {preview_dir} (inspect diff/migrate.md before selective merge)")

    if args.stamp:
        updated = 0
        for r in with_header:
            p: Path = r["cp_file"]
            txt = p.read_text(encoding="utf-8")
            new_txt = re.sub(
                r"(^//\s*Source:[^\n]*\|\s*Synced:\s*)[0-9-]+",
                rf"\g<1>{today}",
                txt,
                count=1,
                flags=re.MULTILINE,
            )
            if new_txt != txt:
                p.write_text(new_txt, encoding="utf-8")
                updated += 1
        print(f"Stamped Synced: {today} on {updated} file(s).")

    return 0


if __name__ == "__main__":
    sys.exit(main())
