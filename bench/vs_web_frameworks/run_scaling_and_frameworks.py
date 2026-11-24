#!/usr/bin/env python3
"""
Multi-Core Scaling (1c / 4c / 8c / 16c) & Full Web Framework Benchmark:
  - CP `cap:web` (single process, CPU-pinned SO_REUSEPORT epoll workers: 1c, 4c, 8c, 16c)
  - Bun Web Framework (1c, 8c)
  - Node Web Framework (1c, 8c)
Measures real TCP HTTP/1.1 Keep-Alive QPS, P50/P99 latency, and Peak VmRSS (MB).
"""
import json
import os
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path("/data/dev/cp")
SUITE_DIR = ROOT / "bench" / "vs_web_frameworks"
TEMP_DIR = ROOT / "temp"
HTTP_BENCH_BIN = TEMP_DIR / "http_bench_bin"
CP_SERVER_BIN = TEMP_DIR / "server_cp"

SCENARIOS = [
    ("get_root", "GET /", "GET", "/", ""),
    ("get_id", "GET /id/123?name=bun", "GET", "/id/123?name=bun", ""),
    ("post_json", "POST /json", "POST", "/json", '{"hello":"world"}'),
]


def get_tree_rss_mb(root_pid: int) -> float:
    pids = set()
    try:
        out = subprocess.check_output(["ps", "-eo", "pid,ppid"], text=True)
        children = {}
        for line in out.splitlines()[1:]:
            parts = line.split()
            if len(parts) == 2:
                p, pp = int(parts[0]), int(parts[1])
                children.setdefault(pp, []).append(p)
        stack = [root_pid]
        while stack:
            cur = stack.pop()
            if cur not in pids:
                pids.add(cur)
                stack.extend(children.get(cur, []))
    except Exception:
        pids.add(root_pid)
    total_kb = 0
    for pid in pids:
        try:
            with open(f"/proc/{pid}/status", "r") as f:
                for line in f:
                    if line.startswith("VmRSS:"):
                        total_kb += int(line.split()[1])
                        break
        except Exception:
            pass
    return round(total_kb / 1024.0, 2)


def wait_port(port: int, timeout: float = 8.0) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return True
        except OSError:
            time.sleep(0.05)
    return False


def run_target(name: str, cmd: list[str], port: int, threads: int = 32, duration: float = 2.5):
    print(f"\n[scale] Starting {name} on port {port}: {' '.join(cmd)}", flush=True)
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        preexec_fn=os.setsid,
    )
    try:
        if not wait_port(port):
            raise RuntimeError(f"{name} failed to listen on port {port}")
        time.sleep(0.25)
        res = {"port": port, "scenarios": {}, "peak_rss_mb": 0.0}
        for sid, label, method, path, body in SCENARIOS:
            bcmd = [
                str(HTTP_BENCH_BIN),
                "--host", "127.0.0.1",
                "--port", str(port),
                "--method", method,
                "--path", path,
                "--threads", str(threads),
                "--duration", f"{duration:.2f}",
                "--warmup", "0.4",
            ]
            if body:
                bcmd.extend(["--body", body, "--content-type", "application/json"])
            out = subprocess.check_output(bcmd, text=True).strip()
            data = json.loads(out)
            rss = get_tree_rss_mb(proc.pid)
            data["rss_mb"] = rss
            res["scenarios"][sid] = data
            if rss > res["peak_rss_mb"]:
                res["peak_rss_mb"] = rss
            print(
                f"  - {label:22s} | QPS: {data['qps']:9.0f} | P50: {data['p50_ms']:6.3f}ms | P99: {data['p99_ms']:6.3f}ms | Err: {data['err']} | RSS: {rss:5.1f} MB",
                flush=True,
            )
        return res
    finally:
        try:
            os.killpg(os.getpgid(proc.pid), signal.SIGTERM)
            proc.wait(timeout=3)
        except Exception:
            try:
                os.killpg(os.getpgid(proc.pid), signal.SIGKILL)
            except Exception:
                pass
        time.sleep(0.2)


def main():
    targets = [
        ("cp_1c", ["env", "PORT=18201", "WORKERS=1", str(CP_SERVER_BIN)], 18201, 16),
        ("cp_4c", ["env", "PORT=18204", "WORKERS=4", str(CP_SERVER_BIN)], 18204, 32),
        ("cp_8c", ["env", "PORT=18208", "WORKERS=8", str(CP_SERVER_BIN)], 18208, 32),
        ("cp_16c", ["env", "PORT=18216", "WORKERS=16", str(CP_SERVER_BIN)], 18216, 48),
        ("bun_1c", ["bun", str(SUITE_DIR / "server_bun.ts"), "--port", "18221", "--workers", "1"], 18221, 16),
        ("bun_8c", ["bun", str(SUITE_DIR / "server_bun.ts"), "--port", "18228", "--workers", "8"], 18228, 32),
        ("node_1c", ["node", str(SUITE_DIR / "server_node.mjs"), "--port", "18231", "--workers", "1"], 18231, 16),
        ("node_8c", ["node", str(SUITE_DIR / "server_node.mjs"), "--port", "18238", "--workers", "8"], 18238, 32),
    ]
    all_res = {}
    for name, cmd, port, threads in targets:
        all_res[name] = run_target(name, cmd, port, threads=threads, duration=2.5)

    out_path = SUITE_DIR / "scaling_results.json"
    out_path.write_text(json.dumps(all_res, indent=2))
    print(f"\n[ok] Saved scaling results to {out_path}")


if __name__ == "__main__":
    main()
