#!/usr/bin/env python3
"""
Real Web Framework Benchmark Runner:
  CP (cap:web) vs Bun (Elysia) vs Node (Fastify / Koa / Express)
  24-route production-like app across 1c / 8c / 16c, grouped strictly by core count:
    1c (CP / Bun Elysia / Node Fastify / Node Koa / Node Express) -> 8c (...) -> 16c (...)

4 Benchmark Scenarios:
  1. GET  /health
  2. GET  /api/v1/users/42/posts/99?lang=en&limit=20
  3. POST /api/v1/orders  body={"item":"gpu-h200","qty":4,"price":32000}
  4. GET  /api/v1/me      headers: Cookie: sid=abc123; theme=dark, Authorization: Bearer tok_999

Usage examples:
  python3 run_fw_bench.py
  python3 run_fw_bench.py --only express,koa --cores 1 --duration 0.5 --warmup 0.2
  python3 run_fw_bench.py --cores 1,8,16 --duration 2.5
"""

import argparse
import http.client
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
CP_FW_SRC = SUITE_DIR / "fw_cp.cp"
CP_FW_BIN = TEMP_DIR / "fw_cp"
BUN_FW_SRC = SUITE_DIR / "fw_bun_elysia.ts"
FASTIFY_FW_SRC = SUITE_DIR / "fw_node_fastify.mjs"
KOA_FW_SRC = SUITE_DIR / "fw_node_koa.mjs"
EXPRESS_FW_SRC = SUITE_DIR / "fw_node_express.mjs"
HTTP_FW_BENCH_C = TEMP_DIR / "http_fw_bench.c"
HTTP_FW_BENCH_BIN = TEMP_DIR / "http_fw_bench_bin"
OUT_JSON = SUITE_DIR / "fw_results.json"

SCENARIOS = [
    {
        "id": "get_health",
        "label": "GET /health",
        "method": "GET",
        "path": "/health",
        "body": "",
        "headers": [],
        "expected": {"ok": True, "service": "cap-web"},
    },
    {
        "id": "get_params_query",
        "label": "GET /users/:id/posts/:postId?...",
        "method": "GET",
        "path": "/api/v1/users/42/posts/99?lang=en&limit=20",
        "body": "",
        "headers": [],
        "expected": {"userId": "42", "postId": "99", "lang": "en", "limit": "20"},
    },
    {
        "id": "post_orders",
        "label": "POST /api/v1/orders (JSON)",
        "method": "POST",
        "path": "/api/v1/orders",
        "body": '{"item":"gpu-h200","qty":4,"price":32000}',
        "headers": [],
        "expected": {"accepted": True, "item": "gpu-h200", "qty": 4},
    },
    {
        "id": "get_me_auth",
        "label": "GET /api/v1/me (Cookie+Auth)",
        "method": "GET",
        "path": "/api/v1/me",
        "body": "",
        "headers": [
            "Cookie: sid=abc123; theme=dark",
            "Authorization: Bearer tok_999",
        ],
        "expected": {"sid": "abc123", "token": "tok_999"},
    },
]

CORE_THREADS = {
    1: 16,
    8: 32,
    16: 48,
}

# Self-contained C HTTP/1.1 Keep-Alive load generator extending http_bench.c with repeatable -H/--header support.
HTTP_FW_BENCH_SOURCE = r"""
#define _GNU_SOURCE
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define MAX_LAT_SAMPLES 65536
#define MAX_EXTRA_HEADERS 16

typedef struct {
    const char *host;
    int port;
    const char *req_buf;
    size_t req_len;
    double duration_sec;
    double warmup_sec;
    uint64_t ok_count;
    uint64_t err_count;
    uint64_t total_bytes;
    uint32_t *lat_us;
    size_t lat_count;
    uint64_t sample_stride;
} worker_arg_t;

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int connect_tcp(const char *host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;
    int one = 1;
    setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));

    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(fd);
        return -1;
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static ssize_t read_http_response(int fd, char *buf, size_t cap) {
    size_t used = 0;
    char *hdr_end = NULL;
    while (used < cap - 1) {
        ssize_t n = recv(fd, buf + used, cap - 1 - used, 0);
        if (n <= 0) return -1;
        used += (size_t)n;
        buf[used] = '\0';
        hdr_end = strstr(buf, "\r\n\r\n");
        if (hdr_end) break;
    }
    if (!hdr_end) return -1;

    if (used < 12 || (memcmp(buf + 9, "200", 3) != 0)) {
        return -1;
    }

    size_t hdr_len = (size_t)(hdr_end - buf) + 4;
    long content_len = -1;
    char *p = buf;
    while (p < hdr_end) {
        char *eol = strstr(p, "\r\n");
        if (!eol || eol > hdr_end) break;
        if (eol - p > 15 && strncasecmp(p, "Content-Length:", 15) == 0) {
            content_len = strtol(p + 15, NULL, 10);
            break;
        }
        p = eol + 2;
    }

    if (content_len >= 0) {
        size_t total_need = hdr_len + (size_t)content_len;
        while (used < total_need && used < cap - 1) {
            ssize_t n = recv(fd, buf + used, cap - 1 - used, 0);
            if (n <= 0) return -1;
            used += (size_t)n;
        }
        return (used >= total_need) ? (ssize_t)used : -1;
    }

    while (!strstr(buf + hdr_len, "0\r\n\r\n") && used < cap - 1) {
        ssize_t n = recv(fd, buf + used, cap - 1 - used, 0);
        if (n <= 0) break;
        used += (size_t)n;
        buf[used] = '\0';
    }
    return (ssize_t)used;
}

static void *worker_fn(void *arg) {
    worker_arg_t *w = (worker_arg_t *)arg;
    char buf[8192];
    int fd = connect_tcp(w->host, w->port);

    uint64_t t_start = now_ns();
    uint64_t t_warmup_end = t_start + (uint64_t)(w->warmup_sec * 1e9);
    uint64_t t_end = t_warmup_end + (uint64_t)(w->duration_sec * 1e9);
    uint64_t iter = 0;

    while (1) {
        uint64_t t0 = now_ns();
        if (t0 >= t_end) break;
        int recording = (t0 >= t_warmup_end);

        if (fd < 0) {
            fd = connect_tcp(w->host, w->port);
            if (fd < 0) {
                if (recording) w->err_count++;
                usleep(1000);
                continue;
            }
        }

        ssize_t sent = send(fd, w->req_buf, w->req_len, MSG_NOSIGNAL);
        if (sent != (ssize_t)w->req_len) {
            close(fd);
            fd = -1;
            if (recording) w->err_count++;
            continue;
        }

        ssize_t r = read_http_response(fd, buf, sizeof(buf));
        uint64_t t1 = now_ns();
        if (r < 0) {
            close(fd);
            fd = -1;
            if (recording) w->err_count++;
            continue;
        }

        if (recording) {
            w->ok_count++;
            w->total_bytes += (uint64_t)r;
            if ((iter++ % w->sample_stride) == 0 && w->lat_count < MAX_LAT_SAMPLES) {
                w->lat_us[w->lat_count++] = (uint32_t)((t1 - t0) / 1000ULL);
            }
        }
    }

    if (fd >= 0) close(fd);
    return NULL;
}

static int cmp_u32(const void *a, const void *b) {
    uint32_t va = *(const uint32_t *)a;
    uint32_t vb = *(const uint32_t *)b;
    return (va > vb) - (va < vb);
}

int main(int argc, char **argv) {
    const char *host = "127.0.0.1";
    int port = 3000;
    const char *method = "GET";
    const char *path = "/";
    const char *body = "";
    const char *content_type = NULL;
    const char *extra_headers[MAX_EXTRA_HEADERS];
    int extra_header_count = 0;
    int threads = 16;
    double duration = 2.5;
    double warmup = 0.4;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--host") == 0 && i + 1 < argc) host = argv[++i];
        else if (strcmp(argv[i], "--port") == 0 && i + 1 < argc) port = atoi(argv[++i]);
        else if (strcmp(argv[i], "--method") == 0 && i + 1 < argc) method = argv[++i];
        else if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) path = argv[++i];
        else if (strcmp(argv[i], "--body") == 0 && i + 1 < argc) body = argv[++i];
        else if (strcmp(argv[i], "--content-type") == 0 && i + 1 < argc) content_type = argv[++i];
        else if ((strcmp(argv[i], "-H") == 0 || strcmp(argv[i], "--header") == 0) && i + 1 < argc) {
            if (extra_header_count < MAX_EXTRA_HEADERS) {
                extra_headers[extra_header_count++] = argv[++i];
            } else {
                i++;
            }
        }
        else if (strcmp(argv[i], "--threads") == 0 && i + 1 < argc) threads = atoi(argv[++i]);
        else if (strcmp(argv[i], "--duration") == 0 && i + 1 < argc) duration = atof(argv[++i]);
        else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) warmup = atof(argv[++i]);
    }

    char req_buf[8192];
    size_t body_len = strlen(body);
    int req_len = snprintf(req_buf, sizeof(req_buf),
        "%s %s HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Connection: keep-alive\r\n",
        method, path, host, port);

    for (int h = 0; h < extra_header_count; h++) {
        req_len += snprintf(req_buf + req_len, sizeof(req_buf) - (size_t)req_len,
            "%s\r\n", extra_headers[h]);
    }

    if (body_len > 0) {
        req_len += snprintf(req_buf + req_len, sizeof(req_buf) - (size_t)req_len,
            "Content-Type: %s\r\n"
            "Content-Length: %zu\r\n\r\n%s",
            content_type ? content_type : "application/json",
            body_len, body);
    } else {
        req_len += snprintf(req_buf + req_len, sizeof(req_buf) - (size_t)req_len, "\r\n");
    }

    pthread_t *tids = calloc((size_t)threads, sizeof(pthread_t));
    worker_arg_t *args = calloc((size_t)threads, sizeof(worker_arg_t));
    for (int i = 0; i < threads; i++) {
        args[i].host = host;
        args[i].port = port;
        args[i].req_buf = req_buf;
        args[i].req_len = (size_t)req_len;
        args[i].duration_sec = duration;
        args[i].warmup_sec = warmup;
        args[i].lat_us = malloc(MAX_LAT_SAMPLES * sizeof(uint32_t));
        args[i].lat_count = 0;
        args[i].sample_stride = 4;
    }

    uint64_t wall_t0 = now_ns();
    for (int i = 0; i < threads; i++) {
        pthread_create(&tids[i], NULL, worker_fn, &args[i]);
    }
    for (int i = 0; i < threads; i++) {
        pthread_join(tids[i], NULL);
    }
    double actual_sec = (double)(now_ns() - wall_t0) / 1e9 - warmup;
    if (actual_sec <= 0.01) actual_sec = duration;

    uint64_t total_ok = 0, total_err = 0, total_bytes = 0;
    size_t total_samples = 0;
    for (int i = 0; i < threads; i++) {
        total_ok += args[i].ok_count;
        total_err += args[i].err_count;
        total_bytes += args[i].total_bytes;
        total_samples += args[i].lat_count;
    }

    uint32_t *all_lat = malloc((total_samples > 0 ? total_samples : 1) * sizeof(uint32_t));
    size_t pos = 0;
    for (int i = 0; i < threads; i++) {
        if (args[i].lat_count > 0) {
            memcpy(all_lat + pos, args[i].lat_us, args[i].lat_count * sizeof(uint32_t));
            pos += args[i].lat_count;
        }
        free(args[i].lat_us);
    }
    double p50_ms = 0.0, p99_ms = 0.0;
    if (total_samples > 0) {
        qsort(all_lat, total_samples, sizeof(uint32_t), cmp_u32);
        p50_ms = (double)all_lat[(size_t)(total_samples * 0.50)] / 1000.0;
        p99_ms = (double)all_lat[(size_t)(total_samples * 0.99)] / 1000.0;
    }
    free(all_lat);
    free(args);
    free(tids);

    double qps = (double)total_ok / actual_sec;
    double mb_s = ((double)total_bytes / (1024.0 * 1024.0)) / actual_sec;
    printf("{\"qps\":%.2f,\"p50_ms\":%.3f,\"p99_ms\":%.3f,\"ok\":%llu,\"err\":%llu,\"mb_per_sec\":%.2f,\"duration_sec\":%.3f}\n",
           qps, p50_ms, p99_ms,
           (unsigned long long)total_ok, (unsigned long long)total_err,
           mb_s, actual_sec);
    return 0;
}
"""


def ensure_http_bench_bin() -> Path:
    TEMP_DIR.mkdir(parents=True, exist_ok=True)
    need_build = True
    if HTTP_FW_BENCH_BIN.exists() and HTTP_FW_BENCH_C.exists():
        if HTTP_FW_BENCH_C.read_text() == HTTP_FW_BENCH_SOURCE:
            need_build = False
    if need_build:
        HTTP_FW_BENCH_C.write_text(HTTP_FW_BENCH_SOURCE)
        subprocess.run(
            ["gcc", "-O3", "-pthread", str(HTTP_FW_BENCH_C), "-o", str(HTTP_FW_BENCH_BIN)],
            check=True,
        )
    return HTTP_FW_BENCH_BIN


def ensure_cp_fw_bin(force_rebuild: bool = False) -> Path:
    watch_files = [
        CP_FW_SRC,
        ROOT / "lib" / "web" / "app.cp",
        ROOT / "lib" / "web" / "server.cp",
        ROOT / "lib" / "web" / "radixRouter.cp",
    ]
    if CP_FW_BIN.exists() and not force_rebuild:
        bin_mtime = CP_FW_BIN.stat().st_mtime
        if all((not f.exists()) or bin_mtime >= f.stat().st_mtime for f in watch_files):
            return CP_FW_BIN
    print(f"[build] Compiling {CP_FW_SRC} -> {CP_FW_BIN} (ReleaseFast)...", flush=True)
    zig_out = TEMP_DIR / "fw_cp.zig"
    subprocess.run(
        ["cpc", str(CP_FW_SRC), "-I", str(ROOT / "lib"), "-o", str(zig_out)],
        cwd=str(ROOT),
        check=True,
    )
    subprocess.run(
        ["zig", "build-exe", "-O", "ReleaseFast", "-lc", str(zig_out), f"-femit-bin={CP_FW_BIN}"],
        cwd=str(ROOT),
        check=True,
    )
    return CP_FW_BIN


def get_tree_rss_mb(root_pid: int) -> float:
    pids = set()
    try:
        out = subprocess.check_output(["ps", "-eo", "pid,ppid"], text=True)
        children: dict[int, list[int]] = {}
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


def wait_port(port: int, timeout: float = 10.0) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        try:
            with socket.create_connection(("127.0.0.1", port), timeout=0.2):
                return True
        except OSError:
            time.sleep(0.05)
    return False


def verify_endpoints(port: int) -> None:
    for sc in SCENARIOS:
        conn = http.client.HTTPConnection("127.0.0.1", port, timeout=3.0)
        hdrs = {}
        for h in sc["headers"]:
            k, v = h.split(":", 1)
            hdrs[k.strip()] = v.strip()
        body = sc["body"]
        if body:
            hdrs["Content-Type"] = "application/json"
        conn.request(sc["method"], sc["path"], body=body if body else None, headers=hdrs)
        resp = conn.getresponse()
        raw = resp.read().decode("utf-8", errors="replace")
        conn.close()
        if resp.status != 200:
            raise RuntimeError(f"Endpoint check failed for {sc['id']}: HTTP {resp.status} body={raw}")
        parsed = json.loads(raw)
        if parsed != sc["expected"]:
            raise RuntimeError(
                f"Endpoint mismatch for {sc['id']}: got {parsed!r}, expected {sc['expected']!r}"
            )


def run_target(
    bench_bin: Path,
    name: str,
    cmd: list[str],
    port: int,
    threads: int,
    duration: float,
    warmup: float,
) -> dict:
    print(f"\n[fw-bench] Starting {name} on port {port} (threads={threads}): {' '.join(cmd)}", flush=True)
    proc = subprocess.Popen(
        cmd,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        preexec_fn=os.setsid,
    )
    try:
        if not wait_port(port):
            err_out = ""
            if proc.poll() is not None and proc.stderr:
                err_out = proc.stderr.read().decode("utf-8", errors="replace")
            raise RuntimeError(f"{name} failed to listen on port {port}. stderr={err_out}")
        time.sleep(0.3)
        verify_endpoints(port)

        res = {"port": port, "threads": threads, "scenarios": {}, "peak_rss_mb": 0.0}
        for sc in SCENARIOS:
            bcmd = [
                str(bench_bin),
                "--host", "127.0.0.1",
                "--port", str(port),
                "--method", sc["method"],
                "--path", sc["path"],
                "--threads", str(threads),
                "--duration", f"{duration:.2f}",
                "--warmup", f"{warmup:.2f}",
            ]
            for h in sc["headers"]:
                bcmd.extend(["-H", h])
            if sc["body"]:
                bcmd.extend(["--body", sc["body"], "--content-type", "application/json"])
            out = subprocess.check_output(bcmd, text=True).strip()
            data = json.loads(out)
            rss = get_tree_rss_mb(proc.pid)
            data["rss_mb"] = rss
            res["scenarios"][sc["id"]] = data
            if rss > res["peak_rss_mb"]:
                res["peak_rss_mb"] = rss
            print(
                f"  - {sc['label']:34s} | QPS: {data['qps']:9.0f} | P50: {data['p50_ms']:6.3f}ms | P99: {data['p99_ms']:6.3f}ms | Err: {data['err']} | RSS: {rss:5.1f} MB",
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


FW_ORDER = ["cp", "bun", "fastify", "koa", "express"]
FW_LABELS = {
    "cp": "CP (cap:web)",
    "bun": "Bun (Elysia)",
    "fastify": "Node (Fastify)",
    "koa": "Node (Koa)",
    "express": "Node (Express)",
}


def print_markdown_summary(all_res: dict, cores: list[int], frameworks: list[str]) -> None:
    ordered_fws = [fw for fw in FW_ORDER if fw in frameworks] + [fw for fw in frameworks if fw not in FW_ORDER]
    print("\n## Real Web Framework Benchmark Summary (24 Routes)\n")
    for c in cores:
        print(f"### {c} Core ({CORE_THREADS.get(c, 32)} Client Threads)\n")
        print("| Framework | GET /health (QPS) | GET :id/:postId+? (QPS) | POST /orders (QPS) | GET /me Auth+Cookie (QPS) | Total Err | Peak RSS (MB) |")
        print("| :--- | ---: | ---: | ---: | ---: | ---: | ---: |")
        for fw in ordered_fws:
            key = f"{fw}_{c}c"
            if key not in all_res:
                continue
            r = all_res[key]
            sc = r["scenarios"]
            total_err = sum(sc[s["id"]]["err"] for s in SCENARIOS)
            print(
                f"| {FW_LABELS[fw]} | "
                f"{sc['get_health']['qps']:,.0f} | "
                f"{sc['get_params_query']['qps']:,.0f} | "
                f"{sc['post_orders']['qps']:,.0f} | "
                f"{sc['get_me_auth']['qps']:,.0f} | "
                f"{total_err} | "
                f"{r['peak_rss_mb']:.1f} |"
            )
        print("")


def main():
    parser = argparse.ArgumentParser(description="Real Web Framework Benchmark: CP vs Bun (Elysia) vs Node (Fastify/Koa/Express)")
    parser.add_argument(
        "--only",
        default="cp,bun,fastify,koa,express",
        help="Comma-separated frameworks: cp,bun,fastify,koa,express (alias: node->fastify)",
    )
    parser.add_argument("--cores", default="1,8,16", help="Comma-separated core counts: 1,8,16")
    parser.add_argument("--duration", type=float, default=2.5, help="Benchmark duration per endpoint (sec)")
    parser.add_argument("--warmup", type=float, default=0.4, help="Warmup duration per endpoint (sec)")
    parser.add_argument("--rebuild-cp", action="store_true", help="Force rebuild fw_cp binary")
    parser.add_argument("--output", default=str(OUT_JSON), help="Output JSON path")
    args = parser.parse_args()

    raw_fws = [x.strip().lower() for x in args.only.split(",") if x.strip()]
    frameworks: list[str] = []
    for fw in raw_fws:
        mapped = "fastify" if fw == "node" else fw
        if mapped not in frameworks:
            frameworks.append(mapped)
    frameworks = [fw for fw in FW_ORDER if fw in frameworks] + [fw for fw in frameworks if fw not in FW_ORDER]
    cores = [int(x.strip()) for x in args.cores.split(",") if x.strip()]

    bench_bin = ensure_http_bench_bin()
    if "cp" in frameworks:
        ensure_cp_fw_bin(force_rebuild=args.rebuild_cp)

    all_res = {}
    base_port = 18300
    for c in cores:
        threads = CORE_THREADS.get(c, 32)
        for fw_idx, fw in enumerate(frameworks):
            port = base_port + c * 10 + fw_idx
            key = f"{fw}_{c}c"
            if fw == "cp":
                cmd = ["env", f"PORT={port}", f"WORKERS={c}", str(CP_FW_BIN)]
            elif fw == "bun":
                cmd = ["bun", str(BUN_FW_SRC), "--port", str(port), "--workers", str(c)]
            elif fw == "fastify":
                cmd = ["node", str(FASTIFY_FW_SRC), "--port", str(port), "--workers", str(c)]
            elif fw == "koa":
                cmd = ["node", str(KOA_FW_SRC), "--port", str(port), "--workers", str(c)]
            elif fw == "express":
                cmd = ["node", str(EXPRESS_FW_SRC), "--port", str(port), "--workers", str(c)]
            else:
                raise ValueError(f"Unknown framework: {fw}")
            all_res[key] = run_target(
                bench_bin,
                key,
                cmd,
                port=port,
                threads=threads,
                duration=args.duration,
                warmup=args.warmup,
            )

    print_markdown_summary(all_res, cores, frameworks)
    out_path = Path(args.output)
    out_path.write_text(json.dumps(all_res, indent=2))
    print(f"[ok] Saved framework benchmark results to {out_path}", flush=True)


if __name__ == "__main__":
    main()
