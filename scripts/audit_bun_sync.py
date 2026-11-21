#!/usr/bin/env python3
"""
Bun 上游源码溯源头规范化与增量同步巡检脚本 (audit_bun_sync.py)

统一头格式：
  // Source: Absorbed from /data/dev/bun/src/<rel> | Synced: YYYY-MM-DDTHH:MM:SSZ (<desc>)

核心能力：
  1. --report (默认): 对比首行 Synced 时间戳与上游 /data/dev/bun/src/ 文件真实 mtime，
     列出所有上游有较新修改的 lib/ 文件，按领域分组输出。
  2. --normalize-headers: 批量将所有指向 /data/dev/bun/ 的旧格式 // Source: 头部
     规范为统一格式（初次规范化时，Synced 取当前 lib/*.cp 的 mtime，确保上游更新不会被掩盖）。
  3. --mark-synced <path ...>: 同步完成后，将指定 lib/*.cp 的 Synced 时间戳刷新为上游当前 UTC mtime。
"""
import argparse
import collections
import re
from datetime import datetime, timezone
from pathlib import Path

LIB_ROOT = Path("/data/dev/cp/lib")
SRC_PATH_RE = re.compile(r"(/data/dev/bun/\S+)")
SYNCED_RE = re.compile(r"\|\s*Synced:\s*(\d{4}-\d{2}-\d{2}T\d{2}:\d{2}:\d{2}Z)")


def fmt_utc(ts: float) -> str:
    return datetime.fromtimestamp(ts, tz=timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ")


def parse_utc(s: str) -> float:
    return datetime.strptime(s, "%Y-%m-%dT%H:%M:%SZ").replace(tzinfo=timezone.utc).timestamp()


def extract_source_info(first_line: str):
    m_path = SRC_PATH_RE.search(first_line)
    if not m_path:
        return None, None, ""
    raw_path = m_path.group(1).rstrip("),;:")
    m_sync = SYNCED_RE.search(first_line)
    synced_str = m_sync.group(1) if m_sync else None
    # Extract trailing parenthetical note if present
    note = ""
    after_path = first_line[m_path.end():]
    if m_sync:
        after_path = first_line[m_sync.end():]
    paren = re.search(r"\(([^()]+)\)\s*$", after_path)
    if paren:
        note = paren.group(1).strip()
    return raw_path, synced_str, note


def resolve_upstream(raw_path: str) -> Path | None:
    p = Path(raw_path)
    if p.is_file():
        # Prefer .cp twin if bun_agy migrated .zig -> .cp and .cp is newer
        if p.suffix == ".zig":
            cp_twin = p.with_suffix(".cp")
            if cp_twin.is_file() and cp_twin.stat().st_mtime >= p.stat().st_mtime:
                return cp_twin
        return p
    if p.suffix == ".zig":
        cp_twin = p.with_suffix(".cp")
        if cp_twin.is_file():
            return cp_twin
    return None


def build_header(raw_path: str, synced_str: str, note: str) -> str:
    base = f"// Source: Absorbed from {raw_path} | Synced: {synced_str}"
    if note:
        return f"{base} ({note})"
    return base


def normalize_headers(dry_run: bool = False) -> None:
    updated = 0
    already_ok = 0
    non_bun = 0
    for p in sorted(LIB_ROOT.rglob("*.cp")):
        text = p.read_text(encoding="utf-8", errors="ignore")
        lines = text.splitlines(keepends=True)
        if not lines:
            continue
        first = lines[0].rstrip("\r\n")
        eol = "\r\n" if lines[0].endswith("\r\n") else "\n"
        raw_path, synced_str, note = extract_source_info(first)
        if not raw_path:
            non_bun += 1
            continue
        if synced_str is None:
            synced_str = fmt_utc(p.stat().st_mtime)
        new_first = build_header(raw_path, synced_str, note)
        if first == new_first:
            already_ok += 1
            continue
        if not dry_run:
            lines[0] = new_first + eol
            p.write_text("".join(lines), encoding="utf-8")
        updated += 1
    print(f"[NORMALIZE] updated={updated}, already_ok={already_ok}, non_bun={non_bun}, dry_run={dry_run}")


def mark_synced(paths: list[str]) -> None:
    for raw in paths:
        p = Path(raw)
        if not p.is_absolute():
            p = Path("/data/dev/cp") / p
        text = p.read_text(encoding="utf-8")
        lines = text.splitlines(keepends=True)
        if not lines:
            continue
        first = lines[0].rstrip("\r\n")
        eol = "\r\n" if lines[0].endswith("\r\n") else "\n"
        raw_path, _, note = extract_source_info(first)
        if not raw_path:
            print(f"[SKIP] No /data/dev/bun/ source in {p}")
            continue
        up = resolve_upstream(raw_path)
        ts = up.stat().st_mtime if up else datetime.now(tz=timezone.utc).timestamp()
        new_path = str(up) if up else raw_path
        lines[0] = build_header(new_path, fmt_utc(ts), note) + eol
        p.write_text("".join(lines), encoding="utf-8")
        print(f"[MARK_SYNCED] {p.relative_to(Path('/data/dev/cp'))} -> {fmt_utc(ts)}")


def report(limit: int = 30) -> None:
    by_pkg = collections.defaultdict(list)
    total_bun = 0
    missing_sync_tag = 0
    for p in sorted(LIB_ROOT.rglob("*.cp")):
        try:
            first = p.read_text(encoding="utf-8", errors="ignore").splitlines()[0]
        except Exception:
            continue
        raw_path, synced_str, _ = extract_source_info(first)
        if not raw_path:
            continue
        total_bun += 1
        if synced_str is None:
            missing_sync_tag += 1
            synced_ts = p.stat().st_mtime
        else:
            synced_ts = parse_utc(synced_str)
        up = resolve_upstream(raw_path)
        if up is None:
            continue
        up_ts = up.stat().st_mtime
        if up_ts > synced_ts + 60:
            rel = str(p.relative_to(LIB_ROOT))
            pkg = rel.split("/")[0]
            by_pkg[pkg].append((rel, str(up), fmt_utc(synced_ts), fmt_utc(up_ts), int(up_ts - synced_ts)))

    total_stale = sum(len(v) for v in by_pkg.values())
    print(f"[AUDIT_BUN_SYNC] bun_sourced={total_bun}, missing_synced_tag={missing_sync_tag}, upstream_newer={total_stale}")
    for pkg, items in sorted(by_pkg.items(), key=lambda kv: (-len(kv[1]), kv[0]))[:limit]:
        print(f"  cap:{pkg} ({len(items)}):")
        for rel, up, s_ts, u_ts, delta in items[:5]:
            print(f"    lib/{rel} (synced {s_ts}) <-- {up} (mtime {u_ts}, +{delta}s)")
        if len(items) > 5:
            print(f"    ... +{len(items) - 5} more")


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--normalize-headers", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--mark-synced", nargs="+", default=[])
    ap.add_argument("--limit", type=int, default=20)
    args = ap.parse_args()
    if args.normalize_headers:
        normalize_headers(dry_run=args.dry_run)
    elif args.mark_synced:
        mark_synced(args.mark_synced)
    else:
        report(limit=args.limit)


if __name__ == "__main__":
    main()
