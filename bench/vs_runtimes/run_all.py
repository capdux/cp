#!/usr/bin/env python3
"""
Cross-Runtime Benchmark Suite: CP (-OReleaseFast) vs Bun vs Node.js vs Python3.

Runs 4 strictly equivalent benchmarks on the same host machine, verifies identical
checksums across all runtimes, computes ops/sec & relative speedups, writes
results.json and README.md, and cleans up all temporary build artifacts.
"""

from __future__ import annotations

import datetime
import json
import os
import platform
import shutil
import statistics
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import Any

BENCH_DIR = Path(__file__).resolve().parent
CP_ROOT = BENCH_DIR.parent.parent
SQLITE_LIB = CP_ROOT / "vendor" / "sqlite" / "prebuilt" / "linux-x86_64" / "libsqlite3.a"
YYJSON_LIB = CP_ROOT / "packages" / "@cap" / "yyjson" / "prebuilt" / "linux-x86_64" / "libyyjson.a"

TRIALS = 3
COLD_START_WARMUP = 2
COLD_START_RUNS = 20


def run_cmd(cmd: list[str], cwd: Path | None = None) -> tuple[int, str, str]:
    proc = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return proc.returncode, proc.stdout.strip(), proc.stderr.strip()


def find_bin(name: str, extra_paths: list[str] | None = None) -> str | None:
    found = shutil.which(name)
    if found:
        return found
    for p in extra_paths or []:
        if os.path.isfile(p) and os.access(p, os.X_OK):
            return p
    return None


def get_version(bin_path: str | None, args: list[str] = ["--version"]) -> str | None:
    if not bin_path:
        return None
    rc, out, err = run_cmd([bin_path, *args])
    if rc != 0:
        return None
    text = out or err
    return text.splitlines()[0].strip() if text else None


def get_cpu_info() -> dict[str, Any]:
    model = platform.processor() or "unknown"
    cores = os.cpu_count() or 0
    rc, out, _ = run_cmd(["lscpu"])
    if rc == 0:
        for line in out.splitlines():
            if line.startswith("Model name:"):
                model = line.split(":", 1)[1].strip()
                break
    return {
        "model": model,
        "logical_cpus": cores,
        "arch": platform.machine(),
        "os": f"{platform.system()} {platform.release()}",
    }


def compile_cp(
    src_name: str,
    build_dir: Path,
    extra_includes: list[Path] | None = None,
    link_libs: list[Path] | None = None,
    link_libc: bool = True,
) -> Path:
    src = BENCH_DIR / src_name
    stem = src.stem
    zig_out = build_dir / f"{stem}.zig"
    bin_out = build_dir / stem

    cpc_cmd = ["cpc", str(src), "-I", str(CP_ROOT / "lib")]
    for inc in extra_includes or []:
        cpc_cmd += ["-I", str(inc)]
    cpc_cmd += ["-o", str(zig_out)]

    rc, out, err = run_cmd(cpc_cmd)
    if rc != 0:
        raise RuntimeError(f"cpc failed for {src_name}:\n{out}\n{err}")

    zig_cmd = ["zig", "build-exe", str(zig_out)]
    for lib in link_libs or []:
        zig_cmd.append(str(lib))
    zig_cmd.append("-OReleaseFast")
    if link_libc:
        zig_cmd.append("-lc")
    zig_cmd.append(f"-femit-bin={bin_out}")

    rc, out, err = run_cmd(zig_cmd, cwd=build_dir)
    if rc != 0:
        raise RuntimeError(f"zig build-exe failed for {src_name}:\n{out}\n{err}")

    return bin_out


def run_json_bench(cmd: list[str], trials: int = TRIALS) -> dict[str, Any]:
    samples: list[dict[str, Any]] = []
    for _ in range(trials):
        rc, out, err = run_cmd(cmd)
        if rc != 0:
            raise RuntimeError(f"Command failed: {' '.join(cmd)}\nstdout: {out}\nstderr: {err}")
        # CP std.debug.print writes to stderr; JS/Python print to stdout
        raw = out if out.startswith("{") else err
        line = [ln.strip() for ln in raw.splitlines() if ln.strip().startswith("{")][-1]
        samples.append(json.loads(line))

    # Median over numeric timing keys; preserve deterministic verification keys
    result: dict[str, Any] = {}
    keys = samples[0].keys()
    for k in keys:
        vals = [s[k] for s in samples]
        if k.endswith("_ms"):
            result[k] = round(statistics.median(vals), 3)
        else:
            result[k] = vals[0]
    return result


def measure_cold_start(cmd: list[str]) -> dict[str, Any]:
    for _ in range(COLD_START_WARMUP):
        rc, out, err = run_cmd(cmd)
        if rc != 0:
            raise RuntimeError(f"Cold-start warmup failed: {' '.join(cmd)}\n{out}\n{err}")

    times_ms: list[float] = []
    for _ in range(COLD_START_RUNS):
        t0 = time.perf_counter_ns()
        proc = subprocess.run(cmd, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        t1 = time.perf_counter_ns()
        if proc.returncode != 0:
            raise RuntimeError(f"Cold-start run failed: {' '.join(cmd)}")
        times_ms.append((t1 - t0) / 1e6)

    return {
        "median_ms": round(statistics.median(times_ms), 3),
        "min_ms": round(min(times_ms), 3),
        "max_ms": round(max(times_ms), 3),
        "runs": COLD_START_RUNS,
    }


def file_size_mb(path: str | Path | None) -> float | None:
    if not path:
        return None
    p = Path(path).resolve()
    if not p.is_file():
        return None
    return round(p.stat().st_size / (1024 * 1024), 3)


def speedup(other_ms: float | None, cp_ms: float | None) -> float | None:
    if other_ms is None or cp_ms is None or cp_ms <= 0:
        return None
    return round(other_ms / cp_ms, 2)


def fmt_ms(v: float | None) -> str:
    return f"{v:.2f} ms" if v is not None else "N/A"


def fmt_speedup(v: float | None) -> str:
    return f"**{v:.2f}x**" if v is not None else "N/A"


def fmt_ops(v: float | None) -> str:
    if v is None:
        return "N/A"
    if v >= 1_000_000:
        return f"{v / 1_000_000:.2f}M/s"
    if v >= 1_000:
        return f"{v / 1_000:.1f}K/s"
    return f"{v:.1f}/s"


def generate_readme(data: dict[str, Any]) -> str:
    cpu = data["host"]["cpu"]
    rt = data["runtimes"]
    b1 = data["benchmarks"]["sqlite_memory"]
    b2 = data["benchmarks"]["compute_zero_gc"]
    b3 = data["benchmarks"]["cold_start_and_size"]
    b4 = data["benchmarks"]["json_parse"]

    lines = [
        "# CP vs Bun vs Node.js vs Python3 — Native Cross-Runtime Benchmark Suite",
        "",
        "100% reproducible, apples-to-apples benchmark suite executed on the same host machine with identical algorithms, payloads, and verified checksums across all runtimes.",
        "",
        "## Test Environment",
        "",
        f"- **Date (UTC)**: `{data['timestamp_utc']}`",
        f"- **CPU**: `{cpu['model']}` ({cpu['logical_cpus']} logical CPUs, `{cpu['arch']}`)",
        f"- **OS**: `{cpu['os']}`",
        f"- **CP**: `{rt['cp']['cpc_version']}` / `{rt['cp']['cap_version']}` (Zig `{rt['cp']['zig_version']}`, `-OReleaseFast`)",
        f"- **Bun**: `{rt['bun']['version'] or 'N/A'}`",
        f"- **Node.js**: `{rt['node']['version'] or 'N/A'}`",
        f"- **Python3**: `{rt['python3']['version'] or 'N/A'}`",
        "",
        "---",
        "",
        "## Summary Results",
        "",
        "### Bench 1 — SQLite `:memory:` (100,000 Batch Inserts + 100,000 PK Point Lookups)",
        "",
        "| Runtime | Engine | Write 100K (ms) | Read 100K (ms) | Total (ms) | Throughput (ops/s) | CP Speedup |",
        "|---|---|---:|---:|---:|---:|---:|",
    ]

    for key, label, eng in [
        ("cp", "**CP (`-OReleaseFast`)**", "`cap:sqlite` (`libsqlite3.a`)"),
        ("bun", "Bun", "`bun:sqlite`"),
        ("node", "Node.js", "`node:sqlite`"),
        ("python3", "Python3", "`sqlite3`"),
    ]:
        r = b1["results"].get(key)
        if not r:
            lines.append(f"| {label} | {eng} | N/A | N/A | N/A | N/A | N/A |")
        else:
            sp = "1.00x (baseline)" if key == "cp" else fmt_speedup(b1["cp_speedup_vs"].get(key))
            lines.append(
                f"| {label} | {eng} | {fmt_ms(r['write_ms'])} | {fmt_ms(r['read_ms'])} | "
                f"{fmt_ms(r['total_ms'])} | {fmt_ops(r['ops_per_sec'])} | {sp} |"
            )

    lines += [
        "",
        f"> Checksum verified across all runtimes: `{b1['results']['cp']['checksum']}`",
        "",
        "### Bench 2 — High-Frequency Compute (`fib(38)`) & Zero-GC 10M `u32` Hash Reduction",
        "",
        "| Runtime | `fib(38)` (ms) | 10M `u32` Reduce (ms) | Total (ms) | Reduce Rate (ops/s) | CP Speedup |",
        "|---|---:|---:|---:|---:|---:|",
    ]

    for key, label in [
        ("cp", "**CP (`-OReleaseFast`)**"),
        ("bun", "Bun"),
        ("node", "Node.js"),
        ("python3", "Python3"),
    ]:
        r = b2["results"].get(key)
        if not r:
            lines.append(f"| {label} | N/A | N/A | N/A | N/A | N/A |")
        else:
            sp = "1.00x (baseline)" if key == "cp" else fmt_speedup(b2["cp_speedup_vs"].get(key))
            lines.append(
                f"| {label} | {fmt_ms(r['fib_ms'])} | {fmt_ms(r['reduce_ms'])} | "
                f"{fmt_ms(r['total_ms'])} | {fmt_ops(r['reduce_ops_per_sec'])} | {sp} |"
            )

    lines += [
        "",
        f"> Verified outputs across all runtimes: `fib(38) = {b2['results']['cp']['fib_res']}`, `reduce_hash = {b2['results']['cp']['reduce_hash']}`",
        "",
        "### Bench 3 — Process Cold-Start Latency (Median of 20 Runs) & Binary / Runtime Size",
        "",
        "| Runtime | Cold Start Median | Min / Max | CP Speedup | Executable / Runtime Size |",
        "|---|---:|---:|---:|---:|",
    ]

    for key, label in [
        ("cp", "**CP (`-OReleaseFast`)**"),
        ("bun", "Bun"),
        ("node", "Node.js"),
        ("python3", "Python3"),
    ]:
        r = b3["results"].get(key)
        if not r:
            lines.append(f"| {label} | N/A | N/A | N/A | N/A |")
        else:
            sp = "1.00x (baseline)" if key == "cp" else fmt_speedup(b3["cp_speedup_vs"].get(key))
            sz = f"{r['binary_size_mb']:.3f} MB ({r['size_note']})" if r["binary_size_mb"] is not None else "N/A"
            lines.append(
                f"| {label} | {fmt_ms(r['median_ms'])} | {r['min_ms']:.2f} / {r['max_ms']:.2f} ms | {sp} | {sz} |"
            )

    lines += [
        "",
        f"### Bench 4 — JSON Parsing (10,000 Parses of `{b4['payload_bytes']}`-byte Payload)",
        "",
        "| Runtime | Parser | Total 10K Parses (ms) | Parses / sec | Throughput (MB/s) | CP Speedup |",
        "|---|---|---:|---:|---:|---:|",
    ]

    for key, label, parser in [
        ("cp", "**CP (`-OReleaseFast`)**", "`@cap/yyjson` (`libyyjson.a`)"),
        ("bun", "Bun", "`JSON.parse` (JSC)"),
        ("node", "Node.js", "`JSON.parse` (V8)"),
        ("python3", "Python3", "`json.loads` (`_json` C ext)"),
    ]:
        r = b4["results"].get(key)
        if not r:
            lines.append(f"| {label} | {parser} | N/A | N/A | N/A | N/A |")
        else:
            sp = "1.00x (baseline)" if key == "cp" else fmt_speedup(b4["cp_speedup_vs"].get(key))
            lines.append(
                f"| {label} | {parser} | {fmt_ms(r['parse_ms'])} | "
                f"{fmt_ops(r['parses_per_sec'])} | {r['mb_per_sec']:.1f} MB/s | {sp} |"
            )

    lines += [
        "",
        f"> Checksum verified across all runtimes: `{b4['results']['cp']['checksum']}`",
        "",
        "---",
        "",
        "## One-Command Reproduction",
        "",
        "```bash",
        "python3 /data/dev/cp/bench/vs_runtimes/run_all.py",
        "```",
        "",
        "All CP sources are compiled into a temporary directory using `cpc` + `zig build-exe -OReleaseFast` and automatically cleaned up after execution.",
        "",
    ]
    return "\n".join(lines)


def main() -> int:
    bun_bin = find_bin("bun", ["/root/.bun/bin/bun", "/data/dev/bun/build/release/bun"])
    node_bin = find_bin("node")
    py_bin = find_bin("python3") or sys.executable

    cpu_info = get_cpu_info()
    runtimes = {
        "cp": {
            "cpc_version": get_version(find_bin("cpc")),
            "cap_version": get_version(find_bin("cap")),
            "zig_version": get_version(find_bin("zig"), ["version"]),
            "opt_mode": "ReleaseFast",
        },
        "bun": {"path": bun_bin, "version": get_version(bun_bin)},
        "node": {"path": node_bin, "version": get_version(node_bin)},
        "python3": {"path": py_bin, "version": get_version(py_bin)},
    }

    print("==> Probed environment:")
    print(f"    CPU: {cpu_info['model']} ({cpu_info['logical_cpus']} logical CPUs)")
    print(f"    CP:  {runtimes['cp']['cpc_version']} / {runtimes['cp']['cap_version']} / Zig {runtimes['cp']['zig_version']}")
    print(f"    Bun: {runtimes['bun']['version']}")
    print(f"    Node: {runtimes['node']['version']}")
    print(f"    Python3: {runtimes['python3']['version']}")

    build_dir = Path(tempfile.mkdtemp(prefix="cp_bench_vs_runtimes_"))
    try:
        shutil.copy2(BENCH_DIR / "payload.json", build_dir / "payload.json")

        print("==> Compiling CP benchmarks (-OReleaseFast)...")
        cp_hello = compile_cp("bench_hello.cp", build_dir, link_libc=False)
        cp_compute = compile_cp("bench_compute.cp", build_dir, link_libc=True)
        cp_sqlite = compile_cp("bench_sqlite.cp", build_dir, link_libs=[SQLITE_LIB], link_libc=True)
        cp_json = compile_cp(
            "bench_json.cp",
            build_dir,
            extra_includes=[CP_ROOT / "packages"],
            link_libs=[YYJSON_LIB],
            link_libc=True,
        )

        # ---- Bench 1: SQLite ----
        print("==> Running Bench 1: SQLite :memory: (100K writes + 100K PK reads)...")
        b1_res: dict[str, Any] = {}
        b1_res["cp"] = run_json_bench([str(cp_sqlite)])
        if bun_bin:
            b1_res["bun"] = run_json_bench([bun_bin, str(BENCH_DIR / "bench_sqlite_bun.js")])
        if node_bin:
            rc, _, _ = run_cmd([node_bin, "-e", "import('node:sqlite')"])
            if rc == 0:
                b1_res["node"] = run_json_bench([node_bin, str(BENCH_DIR / "bench_sqlite_node.mjs")])
        b1_res["python3"] = run_json_bench([py_bin, str(BENCH_DIR / "bench_sqlite.py")])

        expected_b1_ck = b1_res["cp"]["checksum"]
        for k, v in b1_res.items():
            if v["checksum"] != expected_b1_ck:
                raise AssertionError(f"Bench 1 checksum mismatch for {k}: {v['checksum']} != {expected_b1_ck}")
            v["ops_per_sec"] = round(200_000 / (v["total_ms"] / 1000.0), 1)

        b1_data = {
            "description": "SQLite :memory: 100,000 batch inserts (single tx) + 100,000 PK point lookups",
            "trials": TRIALS,
            "results": b1_res,
            "cp_speedup_vs": {
                k: speedup(v["total_ms"], b1_res["cp"]["total_ms"])
                for k, v in b1_res.items() if k != "cp"
            },
        }

        # ---- Bench 2: Compute & Zero-GC ----
        print("==> Running Bench 2: Compute fib(38) + 10M u32 hash reduction...")
        b2_res: dict[str, Any] = {}
        b2_res["cp"] = run_json_bench([str(cp_compute)])
        if bun_bin:
            b2_res["bun"] = run_json_bench([bun_bin, str(BENCH_DIR / "bench_compute.js")])
        if node_bin:
            b2_res["node"] = run_json_bench([node_bin, str(BENCH_DIR / "bench_compute.js")])
        b2_res["python3"] = run_json_bench([py_bin, str(BENCH_DIR / "bench_compute.py")])

        expected_fib = b2_res["cp"]["fib_res"]
        expected_hash = b2_res["cp"]["reduce_hash"]
        for k, v in b2_res.items():
            if v["fib_res"] != expected_fib or v["reduce_hash"] != expected_hash:
                raise AssertionError(
                    f"Bench 2 verification mismatch for {k}: fib={v['fib_res']} hash={v['reduce_hash']} "
                    f"vs CP fib={expected_fib} hash={expected_hash}"
                )
            v["reduce_ops_per_sec"] = round(10_000_000 / (v["reduce_ms"] / 1000.0), 1)

        b2_data = {
            "description": "Recursive fib(38) + 10,000,000 u32 FNV-1a array hash reduction (zero GC)",
            "trials": TRIALS,
            "results": b2_res,
            "cp_speedup_vs": {
                k: speedup(v["total_ms"], b2_res["cp"]["total_ms"])
                for k, v in b2_res.items() if k != "cp"
            },
        }

        # ---- Bench 3: Cold Start & Binary Size ----
        print("==> Running Bench 3: Cold-start latency (20 runs) & binary/runtime size...")
        b3_res: dict[str, Any] = {}
        b3_res["cp"] = measure_cold_start([str(cp_hello)])
        b3_res["cp"]["binary_size_mb"] = file_size_mb(cp_hello)
        b3_res["cp"]["size_note"] = "standalone native binary"

        if bun_bin:
            b3_res["bun"] = measure_cold_start([bun_bin, str(BENCH_DIR / "bench_hello.js")])
            b3_res["bun"]["binary_size_mb"] = file_size_mb(bun_bin)
            b3_res["bun"]["size_note"] = "bun runtime binary"
        if node_bin:
            b3_res["node"] = measure_cold_start([node_bin, str(BENCH_DIR / "bench_hello.js")])
            b3_res["node"]["binary_size_mb"] = file_size_mb(node_bin)
            b3_res["node"]["size_note"] = "node runtime binary"
        b3_res["python3"] = measure_cold_start([py_bin, str(BENCH_DIR / "bench_hello.py")])
        b3_res["python3"]["binary_size_mb"] = file_size_mb(py_bin)
        b3_res["python3"]["size_note"] = "python3 ELF binary (excl. stdlib)"

        b3_data = {
            "description": "Process cold-start latency (median of 20 runs) & binary/runtime size",
            "warmup_runs": COLD_START_WARMUP,
            "measured_runs": COLD_START_RUNS,
            "results": b3_res,
            "cp_speedup_vs": {
                k: speedup(v["median_ms"], b3_res["cp"]["median_ms"])
                for k, v in b3_res.items() if k != "cp"
            },
        }

        # ---- Bench 4: JSON Parse ----
        print("==> Running Bench 4: JSON parse (10,000 iterations)...")
        b4_res: dict[str, Any] = {}
        b4_res["cp"] = run_json_bench([str(cp_json)])
        if bun_bin:
            b4_res["bun"] = run_json_bench([bun_bin, str(BENCH_DIR / "bench_json.mjs")])
        if node_bin:
            b4_res["node"] = run_json_bench([node_bin, str(BENCH_DIR / "bench_json.mjs")])
        b4_res["python3"] = run_json_bench([py_bin, str(BENCH_DIR / "bench_json.py")])

        expected_b4_ck = b4_res["cp"]["checksum"]
        payload_bytes = b4_res["cp"]["payload_bytes"]
        for k, v in b4_res.items():
            if v["checksum"] != expected_b4_ck:
                raise AssertionError(f"Bench 4 checksum mismatch for {k}: {v['checksum']} != {expected_b4_ck}")
            sec = v["parse_ms"] / 1000.0
            v["parses_per_sec"] = round(v["iterations"] / sec, 1)
            v["mb_per_sec"] = round((v["iterations"] * payload_bytes) / (1024 * 1024) / sec, 2)

        b4_data = {
            "description": "10,000 parses of medium JSON payload (@cap/yyjson vs JSON.parse vs json.loads)",
            "trials": TRIALS,
            "payload_bytes": payload_bytes,
            "results": b4_res,
            "cp_speedup_vs": {
                k: speedup(v["parse_ms"], b4_res["cp"]["parse_ms"])
                for k, v in b4_res.items() if k != "cp"
            },
        }

        final_data = {
            "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "host": {"cpu": cpu_info},
            "runtimes": runtimes,
            "benchmarks": {
                "sqlite_memory": b1_data,
                "compute_zero_gc": b2_data,
                "cold_start_and_size": b3_data,
                "json_parse": b4_data,
            },
        }

        results_path = BENCH_DIR / "results.json"
        readme_path = BENCH_DIR / "README.md"
        results_path.write_text(json.dumps(final_data, indent=2) + "\n", encoding="utf-8")
        readme_path.write_text(generate_readme(final_data), encoding="utf-8")

        print(f"==> Wrote {results_path}")
        print(f"==> Wrote {readme_path}")
        return 0
    finally:
        shutil.rmtree(build_dir, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
