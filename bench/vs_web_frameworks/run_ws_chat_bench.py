#!/usr/bin/env python3
"""
Bun-official style 32-client WebSocket Chat Broadcast benchmark suite:
  - cp_publish  (env PORT=18301 WORKERS=1 /data/dev/cp/temp/ws_chat_cp)
  - bun_publish (Bun.serve ws.subscribe("chat") + server.publish("chat", msg))
  - bun_send    (Bun.serve Set loop: for (const c of clients) c.send(msg))
  - node_ws     (Native Node.js RFC 6455 HTTP Upgrade + Set broadcast loop)

Measures total broadcast messages received per second (msgs_per_sec),
throughput (mb_per_sec), error count, and process-tree peak VmRSS (MB).
Saves results to /data/dev/cp/bench/vs_web_frameworks/ws_chat_results.json.
"""
import argparse
import json
import os
import signal
import socket
import subprocess
import threading
import time
from pathlib import Path

ROOT = Path("/data/dev/cp")
SUITE_DIR = ROOT / "bench" / "vs_web_frameworks"
TEMP_DIR = ROOT / "temp"
BENCH_SRC = SUITE_DIR / "ws_chat_bench.c"
BENCH_BIN = TEMP_DIR / "ws_chat_bench_bin"
CP_CHAT_BIN = TEMP_DIR / "ws_chat_cp"
RESULTS_PATH = SUITE_DIR / "ws_chat_results.json"


def ensure_bench_bin():
    TEMP_DIR.mkdir(parents=True, exist_ok=True)
    if not BENCH_BIN.exists() or BENCH_BIN.stat().st_mtime < BENCH_SRC.stat().st_mtime:
        print("[build] Compiling ws_chat_bench.c (-O3)...", flush=True)
        subprocess.run(
            ["gcc", "-O3", str(BENCH_SRC), "-o", str(BENCH_BIN)],
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


def run_target(name, cmd, port, clients, duration, warmup, payload, inflight):
    print(f"\n[ws-chat] Starting {name} on port {port}: {' '.join(cmd)}", flush=True)
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

        time.sleep(0.30)
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
            str(BENCH_BIN),
            "--host", "127.0.0.1",
            "--port", str(port),
            "--clients", str(clients),
            "--duration", str(duration),
            "--warmup", str(warmup),
            "--payload", str(payload),
            "--inflight", str(inflight),
        ]
        out = subprocess.check_output(bench_cmd, text=True).strip()
        stop_evt.set()
        t.join(timeout=1.0)

        metrics = json.loads(out)
        peak_rss = max(rss_samples) if rss_samples else idle_rss
        avg_rss = sum(rss_samples) / len(rss_samples) if rss_samples else idle_rss
        metrics["idle_rss_mb"] = round(idle_rss, 2)
        metrics["peak_rss_mb"] = round(peak_rss, 2)
        metrics["avg_rss_mb"] = round(avg_rss, 2)
        print(
            f"  -> {name:12s} | {metrics['msgs_per_sec']:12,.0f} msgs/s | "
            f"{metrics['mb_per_sec']:7.2f} MB/s | Err: {metrics['err']} | "
            f"Peak VmRSS: {peak_rss:.1f} MB",
            flush=True,
        )
        return metrics
    finally:
        stop_server(proc)
        time.sleep(0.25)


def main():
    parser = argparse.ArgumentParser(description="32-Client WebSocket Chat Broadcast Benchmark Suite")
    parser.add_argument("--clients", type=int, default=32, help="Number of concurrent chat clients")
    parser.add_argument("--duration", type=float, default=3.0, help="Measurement duration (sec)")
    parser.add_argument("--warmup", type=float, default=0.5, help="Warmup duration (sec)")
    parser.add_argument("--payload", type=int, default=32, help="Masked text payload size (bytes)")
    parser.add_argument("--inflight", type=int, default=16, help="Max in-flight messages per client")
    parser.add_argument(
        "--targets",
        type=str,
        default="cp_publish,bun_publish,bun_send,node_ws",
        help="Comma-separated target subset to run",
    )
    args = parser.parse_args()

    ensure_bench_bin()

    all_targets = [
        ("cp_publish", ["env", "PORT=18301", "WORKERS=1", str(CP_CHAT_BIN)], 18301),
        ("bun_publish", ["bun", str(SUITE_DIR / "ws_chat_bun_publish.ts"), "--port", "18302"], 18302),
        ("bun_send", ["bun", str(SUITE_DIR / "ws_chat_bun_send.ts"), "--port", "18303"], 18303),
        ("node_ws", ["node", str(SUITE_DIR / "ws_chat_node.mjs"), "--port", "18304"], 18304),
    ]
    wanted = {t.strip() for t in args.targets.split(",") if t.strip()}
    selected = [t for t in all_targets if t[0] in wanted]

    if any(name == "cp_publish" for name, _, _ in selected) and not CP_CHAT_BIN.exists():
        raise FileNotFoundError(
            f"Missing CP chat server binary: {CP_CHAT_BIN}. "
            "Build it first or pass --targets bun_publish,bun_send,node_ws."
        )

    results = {}
    for name, cmd, port in selected:
        results[name] = run_target(
            name, cmd, port, args.clients, args.duration, args.warmup, args.payload, args.inflight
        )

    payload = {
        "timestamp": time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime()),
        "config": {
            "clients": args.clients,
            "duration": args.duration,
            "warmup": args.warmup,
            "payload": args.payload,
            "inflight": args.inflight,
        },
        "results": results,
    }
    RESULTS_PATH.write_text(json.dumps(payload, indent=2) + "\n")

    print("\n=== 32-Client WebSocket Chat Broadcast Benchmark ===")
    print("| Target | Broadcast msgs/s | Throughput (MB/s) | Err | Peak VmRSS |")
    print("|---|---:|---:|---:|---:|")
    for name, m in results.items():
        print(
            f"| {name} | {m['msgs_per_sec']:,.0f} | {m['mb_per_sec']:.2f} | "
            f"{m['err']} | {m['peak_rss_mb']:.1f} MB |"
        )
    print(f"\n[ok] Saved to {RESULTS_PATH}")


if __name__ == "__main__":
    main()
