#!/usr/bin/env python3
"""
Real TCP WebSocket Echo benchmark suite (RFC 6455, 32/48 concurrent persistent connections,
128-byte Masked Text frames):
  - cp_ws_1c   (CP SO_REUSEPORT + epoll + CPU pin + Cork + 32B SIMD @Vector(32, u8), -OReleaseFast, 1 worker)
  - cp_ws_8c   (CP SO_REUSEPORT + epoll + CPU pin + Cork + 32B SIMD @Vector(32, u8), -OReleaseFast, 8 workers)
  - cp_ws_16c  (CP SO_REUSEPORT + epoll + CPU pin + Cork + 32B SIMD @Vector(32, u8), -OReleaseFast, 16 workers, 48 client threads)
  - bun_ws_1c  (Bun.serve({ websocket }), 1 worker)
  - bun_ws_8c  (Bun.serve({ websocket, reusePort: true }), 8 workers)
  - node_ws_1c (Native Node http upgrade RFC 6455, 1 worker)
  - node_ws_8c (Native Node cluster + http upgrade RFC 6455, 8 workers)

Measures real msg/s, P50/P99 latency (ms), error count, and process-tree peak VmRSS (MB).
Saves results to /data/dev/cp/bench/vs_web_frameworks/ws_live_results.json.
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
WS_BENCH_SRC = SUITE_DIR / "ws_bench.c"
WS_BENCH_BIN = TEMP_DIR / "ws_bench_bin"
CP_WS_SRC = SUITE_DIR / "ws_server_cp.cp"
CP_WS_ZIG = TEMP_DIR / "ws_server_cp.zig"
CP_WS_BIN = TEMP_DIR / "ws_server_cp"


def ensure_binaries():
    TEMP_DIR.mkdir(parents=True, exist_ok=True)
    if not WS_BENCH_BIN.exists() or WS_BENCH_BIN.stat().st_mtime < WS_BENCH_SRC.stat().st_mtime:
        print("[build] Compiling ws_bench.c (-O3 -pthread)...", flush=True)
        subprocess.run(
            ["gcc", "-O3", "-pthread", str(WS_BENCH_SRC), "-o", str(WS_BENCH_BIN)],
            check=True,
        )

    ws_lib = ROOT / "lib" / "web" / "wsServer.cp"
    need_cp = (
        not CP_WS_BIN.exists()
        or CP_WS_BIN.stat().st_mtime < CP_WS_SRC.stat().st_mtime
        or CP_WS_BIN.stat().st_mtime < ws_lib.stat().st_mtime
    )
    if need_cp:
        print("[build] Transpiling & compiling ws_server_cp.cp (-OReleaseFast)...", flush=True)
        zig_code = subprocess.check_output(["cpc", str(CP_WS_SRC)], cwd=str(ROOT), text=True)
        CP_WS_ZIG.write_text(zig_code)
        subprocess.run(
            ["zig", "build-exe", "-OReleaseFast", "-lc", str(CP_WS_ZIG), f"-femit-bin={CP_WS_BIN}"],
            cwd=str(ROOT),
            check=True,
        )


def get_descendant_pids(root_pid):
    children_map = {}
    for entry in os.listdir("/proc"):
        if not entry.isdigit():
            continue
        pid = int(entry)
        try:
            stat = Path(f"/proc/{pid}/stat").read_text()
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


def wait_ws_ready(host, port, timeout=10.0):
    deadline = time.time() + timeout
    req = (
        f"GET /ws HTTP/1.1\r\n"
        f"Host: {host}:{port}\r\n"
        f"Upgrade: websocket\r\n"
        f"Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        f"Sec-WebSocket-Version: 13\r\n\r\n"
    ).encode()
    while time.time() < deadline:
        try:
            with socket.create_connection((host, port), timeout=0.5) as s:
                s.sendall(req)
                data = s.recv(512)
                if b"101" in data and b"s3pPLMBiTxaQ9kYGzzhZRbK+xOo=" in data:
                    return True
        except OSError:
            time.sleep(0.05)
    return False


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


def run_target(name, cmd, port, threads, payload_size, duration, warmup):
    print(f"\n[ws-server] Starting {name} on port {port}: {' '.join(cmd)}", flush=True)
    proc = subprocess.Popen(
        cmd,
        cwd=str(ROOT),
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        start_new_session=True,
    )
    try:
        if not wait_ws_ready("127.0.0.1", port, timeout=10.0):
            err = proc.stderr.read().decode(errors="replace") if proc.stderr else ""
            raise RuntimeError(f"{name} failed to start on port {port}: {err}")

        time.sleep(0.35)
        idle_rss = sample_tree_rss_mb(proc.pid)
        rss_samples = [idle_rss]
        stop_evt = threading.Event()

        def sampler():
            while not stop_evt.is_set():
                rss_samples.append(sample_tree_rss_mb(proc.pid))
                stop_evt.wait(0.15)

        t = threading.Thread(target=sampler, daemon=True)
        t.start()

        bench_cmd = [
            str(WS_BENCH_BIN),
            "--host", "127.0.0.1",
            "--port", str(port),
            "--threads", str(threads),
            "--payload-size", str(payload_size),
            "--duration", str(duration),
            "--warmup", str(warmup),
        ]
        out = subprocess.check_output(bench_cmd, text=True).strip()
        stop_evt.set()
        t.join(timeout=1.0)

        metrics = json.loads(out)
        peak_rss = max(rss_samples) if rss_samples else idle_rss
        avg_rss = sum(rss_samples) / len(rss_samples) if rss_samples else idle_rss
        metrics["threads"] = threads
        metrics["idle_rss_mb"] = round(idle_rss, 2)
        metrics["peak_rss_mb"] = round(peak_rss, 2)
        metrics["avg_rss_mb"] = round(avg_rss, 2)
        print(
            f"  -> {name:12s} ({threads:2d}c) | {metrics['msg_per_sec']:10.0f} msg/s | "
            f"P50: {metrics['p50_ms']:6.3f}ms | P99: {metrics['p99_ms']:6.3f}ms | "
            f"Err: {metrics['err']} | Peak RSS: {peak_rss:.1f} MB",
            flush=True,
        )
        return metrics
    finally:
        stop_server(proc)
        time.sleep(0.25)


def main():
    parser = argparse.ArgumentParser(description="Real TCP WebSocket Echo Benchmark Suite")
    parser.add_argument("--threads", type=int, default=32, help="Concurrent WebSocket connections (default targets)")
    parser.add_argument("--threads-16c", type=int, default=48, help="Concurrent WebSocket connections for cp_ws_16c")
    parser.add_argument("--payload-size", type=int, default=128, help="Masked payload size in bytes")
    parser.add_argument("--duration", type=float, default=3.0, help="Measurement duration (sec)")
    parser.add_argument("--warmup", type=float, default=0.5, help="Warmup duration (sec)")
    args = parser.parse_args()

    ensure_binaries()

    targets = [
        ("cp_ws_1c", ["env", "PORT=18201", "WORKERS=1", str(CP_WS_BIN)], 18201, args.threads),
        ("cp_ws_8c", ["env", "PORT=18202", "WORKERS=8", str(CP_WS_BIN)], 18202, args.threads),
        ("cp_ws_16c", ["env", "PORT=18207", "WORKERS=16", str(CP_WS_BIN)], 18207, args.threads_16c),
        ("bun_ws_1c", ["bun", str(SUITE_DIR / "ws_server_bun.ts"), "--port", "18203", "--workers", "1"], 18203, args.threads),
        ("bun_ws_8c", ["bun", str(SUITE_DIR / "ws_server_bun.ts"), "--port", "18204", "--workers", "8"], 18204, args.threads),
        ("node_ws_1c", ["node", str(SUITE_DIR / "ws_server_node.mjs"), "--port", "18205", "--workers", "1"], 18205, args.threads),
        ("node_ws_8c", ["node", str(SUITE_DIR / "ws_server_node.mjs"), "--port", "18206", "--workers", "8"], 18206, args.threads),
    ]

    results = {}
    for name, cmd, port, target_threads in targets:
        results[name] = run_target(
            name, cmd, port, target_threads, args.payload_size, args.duration, args.warmup
        )

    out_path = SUITE_DIR / "ws_live_results.json"
    payload = {
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "config": {
            "threads": args.threads,
            "threads_16c": args.threads_16c,
            "payload_size": args.payload_size,
            "duration": args.duration,
            "warmup": args.warmup,
        },
        "results": results,
    }
    out_path.write_text(json.dumps(payload, indent=2) + "\n")

    print("\n=== WebSocket Live Echo Benchmark Summary (128B Masked) ===")
    print("| Target | Conns | msg/s | P50 (ms) | P99 (ms) | Err | Peak VmRSS |")
    print("|---|---:|---:|---:|---:|---:|---:|")
    for name, m in results.items():
        print(
            f"| {name} | {m['threads']} | {m['msg_per_sec']:,.0f} | {m['p50_ms']:.3f} | "
            f"{m['p99_ms']:.3f} | {m['err']} | {m['peak_rss_mb']:.1f} MB |"
        )
    print(f"\n[ok] Saved to {out_path}")


if __name__ == "__main__":
    main()
