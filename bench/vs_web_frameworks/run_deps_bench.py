#!/usr/bin/env python3
"""
Same-machine dependency benchmark runner: CP (-OReleaseFast) vs Bun vs Node.
Runs real binaries/scripts, parses real throughput, computes exact speedup ratios,
and saves results to bench/vs_web_frameworks/deps_results.json.
"""
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path("/data/dev/cp")
SUITE_DIR = ROOT / "bench" / "vs_web_frameworks"
TEMP_DIR = ROOT / "temp"


def run_cmd(cmd, cwd=ROOT, merge_stderr=False):
    p = subprocess.run(cmd, cwd=str(cwd), capture_output=True, text=True, check=True)
    if merge_stderr:
        return (p.stdout + "\n" + p.stderr).strip()
    return p.stdout.strip()


def ensure_cp_binaries():
    bins = {
        "bench_web": (ROOT / "bench/web_framework/bench_web.cp", TEMP_DIR / "bench_web"),
        "bench_schema": (ROOT / "bench/web_framework/bench_schema.cp", TEMP_DIR / "bench_schema"),
        "bench_cookie_nofold": (SUITE_DIR / "bench_cookie_nofold.cp", TEMP_DIR / "bench_cookie_nofold"),
    }
    for name, (src, out_bin) in bins.items():
        if not out_bin.exists() or out_bin.stat().st_mtime < src.stat().st_mtime:
            zig_out = TEMP_DIR / f"{name}.zig"
            print(f"[build] Compiling {name} (-OReleaseFast)...", flush=True)
            with open(zig_out, "w") as f:
                subprocess.run(["cpc", str(src)], cwd=str(ROOT), stdout=f, check=True)
            subprocess.run(
                ["zig", "build-exe", str(zig_out), "-OReleaseFast", f"-femit-bin={out_bin}"],
                cwd=str(ROOT),
                check=True,
            )
    return {k: v[1] for k, v in bins.items()}


def parse_cp_benchmarks(bins):
    cp_res = {}

    # 1. bench_web (radix static & dynamic)
    out_web = run_cmd([str(bins["bench_web"])], merge_stderr=True)
    for line in out_web.splitlines():
        m = re.match(r"radix_static_5M_ns=(\d+)\s+\((\d+)\s+ops/s\)", line)
        if m:
            dt_ns, ops = int(m.group(1)), float(m.group(2))
            cp_res["router_static"] = {"iterations": 5_000_000, "dt_ns": dt_ns, "ns_per_op": dt_ns / 5_000_000, "ops_per_sec": ops}
        m = re.match(r"radix_dynamic_2M_ns=(\d+)\s+\((\d+)\s+ops/s\)", line)
        if m:
            dt_ns, ops = int(m.group(1)), float(m.group(2))
            cp_res["router_dynamic_2params"] = {"iterations": 2_000_000, "dt_ns": dt_ns, "ns_per_op": dt_ns / 2_000_000, "ops_per_sec": ops}

    # 2. bench_cookie_nofold (anti-LICM real formatSetCookie)
    out_cookie = run_cmd([str(bins["bench_cookie_nofold"])], merge_stderr=True)
    for line in out_cookie.splitlines():
        m = re.match(r"cookie_nofold_2M_ns=(\d+)\s+\((\d+)\s+ops/s\)", line)
        if m:
            dt_ns, ops = int(m.group(1)), float(m.group(2))
            cp_res["cookie_set_header"] = {"iterations": 2_000_000, "dt_ns": dt_ns, "ns_per_op": dt_ns / 2_000_000, "ops_per_sec": ops}

    # 3. bench_schema (uri ascii, uri utf8, validate, exact_mirror)
    out_schema = run_cmd([str(bins["bench_schema"])], merge_stderr=True)
    schema_map = {
        "1a_uri_ascii_5M_64B": ("uri_ascii_64B", 5_000_000),
        "1b_uri_utf8_2M_63B": ("uri_utf8_63B", 2_000_000),
        "2_validate_json_1M_5f": ("schema_validate_5f", 1_000_000),
        "3_exact_mirror_1M": ("exact_mirror_strip", 1_000_000),
    }
    for line in out_schema.splitlines():
        for prefix, (key, iters) in schema_map.items():
            if line.startswith(prefix + ":"):
                # e.g. 1a_uri_ascii_5M_64B: 31.42ms (6.3 ns/op) | 159.13M ops/s (159134309 ops/s) | ...
                m_ms = re.search(r":\s*([\d.]+)ms\s*\(([\d.]+)\s*ns/op\)", line)
                m_ops = re.search(r"\((\d+)\s*ops/s\)", line)
                if m_ms and m_ops:
                    dt_ns = int(float(m_ms.group(1)) * 1_000_000)
                    cp_res[key] = {
                        "iterations": iters,
                        "dt_ns": dt_ns,
                        "ns_per_op": float(m_ms.group(2)),
                        "ops_per_sec": float(m_ops.group(1)),
                    }
    return cp_res


def run_js_bench(runtime_cmd):
    out = run_cmd([runtime_cmd, str(SUITE_DIR / "bench_deps_vs_orig.mjs")])
    return json.loads(out)


def fmt_mops(ops):
    return f"{ops / 1e6:.2f}M"


def main():
    bins = ensure_cp_binaries()
    print("[run] Running CP (-OReleaseFast) benchmarks...", flush=True)
    cp_res = parse_cp_benchmarks(bins)

    print("[run] Running Bun JS baseline...", flush=True)
    bun_data = run_js_bench("bun")

    print("[run] Running Node JS baseline...", flush=True)
    node_data = run_js_bench("node")

    order = [
        ("uri_ascii_64B", "fast-decode-uri (ASCII 64B)"),
        ("uri_utf8_63B", "fast-decode-uri (UTF-8 %XX)"),
        ("router_static", "memoirist/Radix (static)"),
        ("router_dynamic_2params", "memoirist/Radix (:uid/:pid)"),
        ("cookie_set_header", "cookie Set-Cookie format"),
        ("schema_validate_5f", "Schema validate (5 fields)"),
        ("exact_mirror_strip", "exact-mirror strip+emit"),
    ]

    comparison = []
    print("\n| Benchmark | CP (ReleaseFast) | Bun JS | Node JS | CP vs Bun | CP vs Node |")
    print("|---|---:|---:|---:|---:|---:|")
    for key, label in order:
        c = cp_res[key]
        b = bun_data["results"][key]
        n = node_data["results"][key]
        vs_bun = c["ops_per_sec"] / b["ops_per_sec"]
        vs_node = c["ops_per_sec"] / n["ops_per_sec"]
        comparison.append({
            "key": key,
            "label": label,
            "cp_ops_per_sec": round(c["ops_per_sec"], 2),
            "cp_ns_per_op": round(c["ns_per_op"], 2),
            "bun_ops_per_sec": round(b["ops_per_sec"], 2),
            "bun_ns_per_op": round(b["ns_per_op"], 2),
            "node_ops_per_sec": round(n["ops_per_sec"], 2),
            "node_ns_per_op": round(n["ns_per_op"], 2),
            "cp_vs_bun_speedup": round(vs_bun, 2),
            "cp_vs_node_speedup": round(vs_node, 2),
        })
        print(
            f"| {label} | {fmt_mops(c['ops_per_sec'])}/s ({c['ns_per_op']:.1f}ns) "
            f"| {fmt_mops(b['ops_per_sec'])}/s ({b['ns_per_op']:.1f}ns) "
            f"| {fmt_mops(n['ops_per_sec'])}/s ({n['ns_per_op']:.1f}ns) "
            f"| **{vs_bun:.2f}x** | **{vs_node:.2f}x** |"
        )

    out_path = SUITE_DIR / "deps_results.json"
    payload = {
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "runtimes": {
            "cp": "cpc + zig -OReleaseFast",
            "bun": bun_data["runtime"],
            "node": node_data["runtime"],
        },
        "comparison": comparison,
    }
    out_path.write_text(json.dumps(payload, indent=2) + "\n")
    print(f"\n[ok] Saved to {out_path}")


if __name__ == "__main__":
    main()
