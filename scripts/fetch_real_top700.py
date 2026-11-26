#!/usr/bin/env python3
"""Fetch real download-ranked Top 700 lists from crates.io & npm (npm-high-impact + api.npmjs.org)
and diff against packages/@cap/registry.json to produce strictly download-ordered work queues."""

import json
import os
import re
import time
import urllib.parse
import urllib.request
from pathlib import Path

ROOT = Path("/data/dev/cp")
OUT_DIR = ROOT / "temp"
OUT_DIR.mkdir(exist_ok=True)
UA = "cp-lang-ecosystem-audit/1.0 (https://capdux.org)"


def load_mapped():
    reg = json.loads((ROOT / "packages/@cap/registry.json").read_text())
    npm_mapped = set()
    crate_mapped = set()
    for e in reg.get("entries", []):
        src = (e.get("source_registry") or "").lower()
        name = (e.get("source_name") or "").lower()
        aliases = [a.lower() for a in e.get("aliases", [])]
        if src == "npm":
            npm_mapped.add(name)
            npm_mapped.update(aliases)
        elif src in ("crates.io", "crates", "cargo"):
            crate_mapped.add(name)
            crate_mapped.update(aliases)
    dirs = {d.lower() for d in os.listdir(ROOT / "packages/@cap") if (ROOT / "packages/@cap" / d).is_dir()}
    return npm_mapped, crate_mapped, dirs


def fetch_crates_top700():
    cache = OUT_DIR / "real_top700_crates.json"
    if cache.exists() and cache.stat().st_size > 10000:
        return json.loads(cache.read_text())
    crates = []
    for page in range(1, 8):
        url = f"https://crates.io/api/v1/crates?sort=downloads&per_page=100&page={page}"
        req = urllib.request.Request(url, headers={"User-Agent": UA})
        with urllib.request.urlopen(req, timeout=20) as r:
            data = json.loads(r.read())
            for c in data.get("crates", []):
                crates.append({
                    "rank": len(crates) + 1,
                    "name": c["name"],
                    "downloads": c["downloads"],
                    "recent_downloads": c.get("recent_downloads", 0),
                    "repository": c.get("repository") or "",
                    "description": (c.get("description") or "").strip()[:120],
                })
        time.sleep(0.3)
    cache.write_text(json.dumps(crates, indent=2, ensure_ascii=False))
    return crates


def fetch_npm_top700():
    # wooorm/npm-high-impact lib/top-download.js has the official top-downloaded npm packages
    url = "https://raw.githubusercontent.com/wooorm/npm-high-impact/main/lib/top-download.js"
    req = urllib.request.Request(url, headers={"User-Agent": UA})
    with urllib.request.urlopen(req, timeout=20) as r:
        txt = r.read().decode("utf-8", errors="ignore")
    names = [n for n in re.findall(r"'([a-zA-Z0-9._@/-]+)'", txt) if not n.startswith("@") and "/" not in n]
    unscoped = names[:1000]
    results = []
    for i in range(0, len(unscoped), 100):
        batch = unscoped[i : i + 100]
        joined = ",".join(urllib.parse.quote(x, safe="") for x in batch)
        api_url = f"https://api.npmjs.org/downloads/point/last-week/{joined}"
        req2 = urllib.request.Request(api_url, headers={"User-Agent": UA})
        try:
            with urllib.request.urlopen(req2, timeout=20) as r2:
                data = json.loads(r2.read())
                for pkg, info in data.items():
                    if isinstance(info, dict) and "downloads" in info:
                        results.append({"name": pkg, "weekly_downloads": int(info["downloads"])})
        except Exception as ex:
            print("npm batch err:", ex)
        time.sleep(0.15)
    results.sort(key=lambda x: x["weekly_downloads"], reverse=True)
    top700 = []
    for idx, item in enumerate(results[:700], 1):
        item["rank"] = idx
        top700.append(item)
    (OUT_DIR / "real_top700_npm.json").write_text(json.dumps(top700, indent=2, ensure_ascii=False))
    return top700


def main():
    npm_mapped, crate_mapped, dirs = load_mapped()
    crates = fetch_crates_top700()
    npm = fetch_npm_top700()

    c1, c700 = crates[0], crates[-1]
    n1, n700 = npm[0], npm[-1]
    print(f"=== CRATES.IO REAL TOP 700 ===")
    print(f"  #1   {c1['name']:24s} {c1['downloads']:>14,} total")
    print(f"  #700 {c700['name']:24s} {c700['downloads']:>14,} total")
    c_hit = sum(1 for c in crates if c["name"].lower() in crate_mapped or c["name"].lower().replace("_", "-") in dirs)
    print(f"  Covered in Top 700: {c_hit}/700 ({c_hit/7:.1f}%)")
    c_missing = [c for c in crates if c["name"].lower() not in crate_mapped and c["name"].lower().replace("_", "-") not in dirs]
    print("  Top 25 UNABSORBED crates.io (strictly by real downloads):")
    for c in c_missing[:25]:
        print(f"    #{c['rank']:3d} {c['name']:26s} {c['downloads']:>13,} | {c['description'][:60]}")

    print(f"\n=== NPM REAL TOP 700 (unscoped, last-week downloads) ===")
    print(f"  #1   {n1['name']:24s} {n1['weekly_downloads']:>14,}/wk")
    print(f"  #700 {n700['name']:24s} {n700['weekly_downloads']:>14,}/wk")
    n_hit = sum(1 for n in npm if n["name"].lower() in npm_mapped or n["name"].lower() in dirs)
    print(f"  Covered in Top 700: {n_hit}/700 ({n_hit/7:.1f}%)")
    n_missing = [n for n in npm if n["name"].lower() not in npm_mapped and n["name"].lower() not in dirs]
    print("  Top 25 UNABSORBED npm (strictly by real weekly downloads):")
    for n in n_missing[:25]:
        print(f"    #{n['rank']:3d} {n['name']:26s} {n['weekly_downloads']:>13,}/wk")


if __name__ == "__main__":
    main()
