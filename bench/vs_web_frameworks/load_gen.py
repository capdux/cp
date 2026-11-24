#!/usr/bin/env python3
"""
Real TCP HTTP/1.1 Keep-Alive benchmark suite against Elysia official 3 scenarios:
  1. GET /                     -> "Hi"
  2. GET /id/123?name=bun      -> "123 bun" + header x-powered-by: benchmark
  3. POST /json                -> {"hello":"world"} JSON echo

Measures real QPS, P50/P99 latency (ms), and process-tree VmRSS (MB) via /proc/<pid>/status.
Supports: bun_1c, bun_8c, node_1c, node_8c, and cp_1c / cp_8c (when temp/server_cp exists).
"""
import argparse
import json
import os
import signal
import socket
import subprocess
import sys
import threading
import time
from pathlib import Path

ROOT = Path("/data/dev/cp")
SUITE_DIR = ROOT / "bench" / "vs_web_frameworks"
TEMP_DIR = ROOT / "temp"
HTTP_BENCH_SRC = SUITE_DIR / "http_bench.c"
HTTP_BENCH_BIN = TEMP_DIR / "http_bench_bin"
CP_SERVER_BIN = TEMP_DIR / "server_cp"

SCENARIOS = [
    {
        "id": "get_root",
        "label": "GET /",
        "method": "GET",
        "path": "/",
        "body": "",
    },
    {
        "id": "get_id_query",
        "label": "GET /id/123?name=bun",
        "method": "GET",
        "path": "/id/123?name=bun",
        "body": "",
    },
    {
        "id": "post_json",
        "label": "POST /json",
        "method": "POST",
        "path": "/json",
        "body": '{"hello":"world"}',
    },
]


def ensure_http_bench():
    if not HTTP_BENCH_BIN.exists() or HTTP_BENCH_BIN.stat().st_mtime < HTTP_BENCH_SRC.stat().st_mtime:
        print("[build] Compiling http_bench.c (-O3 -pthread)...", flush=True)
        subprocess.run(
            ["gcc", "-O3", "-pthread", str(HTTP_BENCH_SRC), "-o", str(HTTP_BENCH_BIN)],
            check=True,
        )


def get_descendant_pids(root_pid):
    """Collect root_pid and all descendant PIDs via /proc/*/stat."""
    children_map = {}
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        pid = int(entry)
        try:
            stat = Path(f"/proc/{pid}/stat").read_text()
            # Format: pid (comm) state ppid ...
            rparen = stat.rfind(")")
            fields = stat[rparen + 2 :].split()
            ppid = int(fields[1])
            children_map.setdefault(ppid, []).append(pid)
        except OSError:
            continue

    result = []
    stack = [root_pid]
    seen = set()
    while stack:
        p = stack.pop()
        if p in seen:
            continue
        seen.add(p)
        result.append(p)
        stack.extend(children_map.get(p, []))
    return result


def sample_tree_rss_mb(root_pid):
    total_kb = 0
    for pid in get_descendant_pids(root_pid):
        try:
            for line in Path(f"/proc/{pid}/status").read_text().splitlines():
                if line.startswith("VmRSS:"):
                    total_kb += int(line.split()[1])
                    break
        except OSError:
            continue
    return total_kb / 1024.0


def wait_port_ready(host, port, timeout=10.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5) as s:
                s.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n")
                data = s.recv(256)
                if b"200" in data and b"Hi" in data:
                    return True
        except OSError:
            time.sleep(0.05)
    return False


def verify_endpoints(host, port):
    """Sanity-check all 3 Elysia benchmark endpoints before running load."""
    checks = [
        (b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", [b"200", b"Hi"]),
        (b"GET /id/123?name=bun HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n", [b"200", b"123 bun", b"benchmark"]),
        (
            b'POST /json HTTP/1.1\r\nHost: localhost\r\nContent-Type: application/json\r\nContent-Length: 17\r\nConnection: close\r\n\r\n{"hello":"world"}',
            [b"200", b"hello", b"world"],
        ),
    ]
    for req, expected in checks:
        with socket.create_connection((host, port), timeout=2.0) as s:
            s.sendall(req)
            chunks = []
            while True:
                d = s.recv(1024)
                if not d:
                    break
                chunks.append(d)
            resp = b"".join(chunks)
            lower = resp.lower()
            for tok in expected:
                if tok.lower() not in lower:
                    raise RuntimeError(f"Endpoint verification failed! Missing {tok!r} in:\n{resp!r}")


def stop_server(proc):
    if proc.poll() is not None:
        return
    try:
        os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
    except OSError:
        proc.terminate()
    try:
        proc.wait(timeout=3.0)
    except subprocess.TimeoutExpired:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
        except OSError:
            proc.kill()
        proc.wait()


def run_target(name, cmd, port, threads, duration, warmup):
    print(f"\n[server] Starting {name} on port {port}: {' '.join(cmd)}", flush=True)
    proc = subprocess.Popen(
        cmd,
        cwd=str(ROOT),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        start_new_session=True,
    )
    try:
        if not wait_port_ready("127.0.0.1", port, timeout=10.0):
            err = proc.stderr.read().decode(errors="replace") if proc.stderr else ""
            raise RuntimeError(f"{name} failed to start on port {port}: {err}")

        verify_endpoints("127.0.0.1", port)
        idle_rss = sample_tree_rss_mb(proc.pid)

        scenario_results = {}
        peak_rss_overall = idle_rss

        for sc in SCENARIOS:
            rss_samples = []
            stop_evt = threading.Event()

            def sampler():
                while not stop_evt.is_set():
                    rss_samples.append(sample_tree_rss_mb(proc.pid))
                    stop_evt.wait(0.15)

            t = threading.Thread(target=sampler, daemon=True)
            t.start()

            bench_cmd = [
                str(HTTP_BENCH_BIN),
                "--host", "127.0.0.1",
                "--port", str(port),
                "--method", sc["method"],
                "--path", sc["path"],
                "--threads", str(threads),
                "--duration", str(duration),
                "--warmup", str(warmup),
            ]
            if sc["body"]:
                bench_cmd.extend(["--body", sc["body"], "--content-type", "application/json"])

            out = subprocess.check_output(bench_cmd, text=True).strip()
            stop_evt.set()
            t.join(timeout=1.0)

            metrics = json.loads(out)
            peak_rss = max(rss_samples) if rss_samples else idle_rss
            avg_rss = sum(rss_samples) / len(rss_samples) if rss_samples else idle_rss
            peak_rss_overall = max(peak_rss_overall, peak_rss)
            metrics["rss_peak_mb"] = round(peak_rss, 2)
            metrics["rss_avg_mb"] = round(avg_rss, 2)
            scenario_results[sc["id"]] = metrics
            print(
                f"  - {sc['label']:22s} | QPS: {metrics['qps']:10.0f} | "
                f"P50: {metrics['p50_ms']:6.3f}ms | P99: {metrics['p99_ms']:6.3f}ms | "
                f"Err: {metrics['err']} | RSS: {peak_rss:.1f} MB",
                flush=True,
            )

        return {
            "name": name,
            "idle_rss_mb": round(idle_rss, 2),
            "peak_rss_mb": round(peak_rss_overall, 2),
            "scenarios": scenario_results,
        }
    finally:
        stop_server(proc)
        time.sleep(0.25)


def main():
    parser = argparse.ArgumentParser(description="Elysia-style TCP HTTP/1.1 Benchmark Suite")
    parser.add_argument("--targets", default="all", help="Comma-separated: bun_1c,bun_8c,node_1c,node_8c,cp_1c,cp_8c or 'all'")
    parser.add_argument("--threads", type=int, default=32, help="Concurrent Keep-Alive client threads")
    parser.add_argument("--duration", type=float, default=3.0, help="Measurement duration per scenario (sec)")
    parser.add_argument("--warmup", type=float, default=0.5, help="Warmup duration per scenario (sec)")
    args = parser.parse_args()

    ensure_http_bench()

    all_defs = {
        "bun_1c": (["bun", str(SUITE_DIR / "server_bun.ts"), "--port", "18101", "--workers", "1"], 18101),
        "bun_8c": (["bun", str(SUITE_DIR / "server_bun.ts"), "--port", "18102", "--workers", "8"], 18102),
        "node_1c": (["node", str(SUITE_DIR / "server_node.mjs"), "--port", "18103", "--workers", "1"], 18103),
        "node_8c": (["node", str(SUITE_DIR / "server_node.mjs"), "--port", "18104", "--workers", "8"], 18104),
        "cp_1c": (["env", "PORT=18105", "WORKERS=1", str(CP_SERVER_BIN)], 18105),
        "cp_8c": (["env", "PORT=18106", "WORKERS=8", str(CP_SERVER_BIN)], 18106),
    }

    if args.targets == "all":
        selected = ["bun_1c", "bun_8c", "node_1c", "node_8c"]
        if CP_SERVER_BIN.exists():
            selected.extend(["cp_1c", "cp_8c"])
        else:
            print(f"[info] {CP_SERVER_BIN} not found yet; running Bun & Node baselines (cp_1c/cp_8c slot ready).")
    else:
        selected = [t.strip() for t in args.targets.split(",") if t.strip()]

    results = {}
    for tname in selected:
        if tname not in all_defs:
            print(f"[warn] Unknown target: {tname}", file=sys.stderr)
            continue
        cmd, port = all_defs[tname]
        if tname.startswith("cp_") and not CP_SERVER_BIN.exists():
            print(f"[skip] {tname}: {CP_SERVER_BIN} not built yet.")
            continue
        results[tname] = run_target(tname, cmd, port, args.threads, args.duration, args.warmup)

    out_path = SUITE_DIR / "http_results.json"
    payload = {
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "config": {"threads": args.threads, "duration": args.duration, "warmup": args.warmup},
        "results": results,
    }
    out_path.write_text(json.dumps(payload, indent=2) + "\n")

    print("\n=== Summary Table (QPS / P99 / Peak VmRSS) ===")
    print("| Target | GET / (QPS) | GET /id/:id?name=bun (QPS) | POST /json (QPS) | Peak VmRSS |")
    print("|---|---:|---:|---:|---:|")
    for tname, data in results.items():
        s = data["scenarios"]
        print(
            f"| {tname} | {s['get_root']['qps']:,.0f} ({s['get_root']['p99_ms']:.2f}ms) "
            f"| {s['get_id_query']['qps']:,.0f} ({s['get_id_query']['p99_ms']:.2f}ms) "
            f"| {s['post_json']['qps']:,.0f} ({s['post_json']['p99_ms']:.2f}ms) "
            f"| {data['peak_rss_mb']:.1f} MB |"
        )
    print(f"\n[ok] Saved to {out_path}")


if __name__ == "__main__":
    main()
