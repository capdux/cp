#!/usr/bin/env python3
"""
SQLite Extreme 1c / 8c / 16c Multi-Thread WAL Benchmark Suite:
  CP (`cap:sqlite`, `-OReleaseFast` + source-compiled `sqlite3_extreme.o`)
  vs Bun (`bun:sqlite`)
  vs Node (`node:sqlite`)

Source-level SQLite3 compilation flags:
  -O3 -march=native -DSQLITE_THREADSAFE=2 -DSQLITE_DEFAULT_MEMSTATUS=0
  -DSQLITE_OMIT_PROGRESS_CALLBACK -DSQLITE_DQS=0 -DSQLITE_MAX_EXPR_DEPTH=0
  -DSQLITE_OMIT_DEPRECATED

Unified PRAGMA tuning across all three runtimes:
  Write (1c)     : PRAGMA journal_mode=WAL; PRAGMA locking_mode=EXCLUSIVE; PRAGMA synchronous=NORMAL;
                   PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY;
  PointRead (1c) : PRAGMA busy_timeout=5000; PRAGMA locking_mode=EXCLUSIVE; PRAGMA mmap_size=268435456;
                   PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;
  WAL Read (8/16): PRAGMA busy_timeout=5000; PRAGMA locking_mode=NORMAL; PRAGMA mmap_size=268435456;
                   PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;

Five core benchmarks (strict cross-runtime checksum verification + peak VmHWM/VmRSS MB):
  1. Single-thread bulk insert (1c, EXCLUSIVE): 100,000 order rows in 1 tx + prepared statement reuse
  2. Single-thread indexed point lookup (1c, EXCLUSIVE): 200,000 x `SELECT amount FROM orders WHERE id = ?`
  3. 8-core concurrent WAL read (8c, NORMAL, 1,600,000 total queries):
     - CP   : single process, 8 pthreads (each with independent read-only connection)
     - Bun  : 8 concurrent processes (synchronized barrier start) -> total QPS & process-tree peak RSS MB
     - Node : 8 concurrent processes (synchronized barrier start) -> total QPS & process-tree peak RSS MB
  4. 16-core concurrent WAL read (16c, NORMAL, 3,200,000 total queries):
     - CP   : single process, 16 pthreads (each with independent read-only connection)
     - Bun  : 16 concurrent processes (synchronized barrier start) -> total QPS & process-tree peak RSS MB
     - Node : 16 concurrent processes (synchronized barrier start) -> total QPS & process-tree peak RSS MB
  5. Batch row materialization (1c, 500 queries x 1,000 rows = 500,000 multi-column rows):
     - Table `orders_batch(id, cat_id, amount, status, customer, sku)` with `idx_orders_cat(cat_id)`
     - CP   : `undefined` stack/Arena linear buffer zero-GC materialization (`arena_pos = 0` O(1) reset)
     - Bun  : `bun:sqlite` `stmt.all(cat)` JS object/string array materialization
     - Node : `node:sqlite` `stmt.all(cat)` JS object/string array materialization

Results saved to `/data/dev/cp/bench/vs_web_frameworks/sqlite_extreme_results.json`.
"""

from __future__ import annotations

import datetime
import json
import os
import platform
import shutil
import sqlite3
import statistics
import subprocess
import tempfile
import time
from pathlib import Path
from typing import Any

BENCH_DIR = Path(__file__).resolve().parent
CP_ROOT = BENCH_DIR.parent.parent
TEMP_DIR = CP_ROOT / "temp"
SQLITE_SRC_C = CP_ROOT / "vendor" / "sqlite" / "sqlite3.c"
SQLITE_EXTREME_OBJ = TEMP_DIR / "sqlite3_extreme.o"
CP_SRC = BENCH_DIR / "sqlite_extreme_cp.cp"
CP_ZIG = TEMP_DIR / "sqlite_extreme_cp.zig"
CP_BIN = TEMP_DIR / "sqlite_extreme_cp"
OUT_JSON = BENCH_DIR / "sqlite_extreme_results.json"

SQLITE_CFLAGS = [
    "-O3",
    "-march=native",
    "-DSQLITE_THREADSAFE=2",
    "-DSQLITE_DEFAULT_MEMSTATUS=0",
    "-DSQLITE_OMIT_PROGRESS_CALLBACK",
    "-DSQLITE_DQS=0",
    "-DSQLITE_MAX_EXPR_DEPTH=0",
    "-DSQLITE_OMIT_DEPRECATED",
]

ORDERS = 100_000
POINT_QUERIES = 200_000
CONCURRENT_WORKERS_8C = 8
CONCURRENT_WORKERS_16C = 16
TOTAL_CONCURRENT_QUERIES_8C = CONCURRENT_WORKERS_8C * POINT_QUERIES
TOTAL_CONCURRENT_QUERIES_16C = CONCURRENT_WORKERS_16C * POINT_QUERIES
BATCH_QUERIES = 500
BATCH_ROWS_PER_QUERY = 1_000
BATCH_CATS = 100
TOTAL_BATCH_ROWS = BATCH_QUERIES * BATCH_ROWS_PER_QUERY
TRIALS = 3

# ---------------------------------------------------------------------------
# Bun (`bun:sqlite`) & Node (`node:sqlite`) Benchmark Drivers
# ---------------------------------------------------------------------------

BUN_BENCH_JS = r"""
import { Database } from "bun:sqlite";
import { readFileSync } from "node:fs";

const ORDERS = 100000;
const POINT_QUERIES = 200000;
const ORDER_NOTE = "order_payload_item";

const PRAGMA_WRITE = "PRAGMA journal_mode=WAL; PRAGMA locking_mode=EXCLUSIVE; PRAGMA synchronous=NORMAL; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY;";
const PRAGMA_POINT_READ = "PRAGMA busy_timeout=5000; PRAGMA locking_mode=EXCLUSIVE; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;";
const PRAGMA_READ = "PRAGMA busy_timeout=5000; PRAGMA locking_mode=NORMAL; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;";

function readPeakRssMb() {
  const m = readFileSync("/proc/self/status", "utf8").match(/VmHWM:\s+(\d+)\s+kB/);
  return m ? Number((Number(m[1]) / 1024).toFixed(2)) : 0;
}

const mode = process.env.SQLITE_BENCH_MODE || "write";
const dbPath = process.env.SQLITE_BENCH_DB || "/tmp/bun_sqlite_extreme.db";

if (mode === "write") {
  const db = new Database(dbPath, { create: true });
  db.exec(PRAGMA_WRITE);
  db.exec("DROP TABLE IF EXISTS orders;");
  db.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, customer_id INTEGER NOT NULL, amount INTEGER NOT NULL, status INTEGER NOT NULL, note TEXT NOT NULL);");

  const ins = db.prepare("INSERT INTO orders (id, customer_id, amount, status, note) VALUES (?, ?, ?, ?, ?);");
  const insertAll = db.transaction(() => {
    for (let i = 1; i <= ORDERS; i++) {
      ins.run(i, ((i * 37) % 10000) + 1, ((i * 131) % 50000) + 100, i % 4, ORDER_NOTE);
    }
  });

  const t0 = Bun.nanoseconds();
  insertAll();
  const t1 = Bun.nanoseconds();

  db.exec("PRAGMA locking_mode=NORMAL;");
  const row = db.prepare("SELECT COUNT(*) AS cnt, SUM(amount) AS total FROM orders;").get();
  const checksum = Number(BigInt(row.cnt) + BigInt(row.total));
  db.close();

  const elapsed_ms = (t1 - t0) / 1e6;
  console.log(JSON.stringify({
    mode: "write",
    rows: ORDERS,
    elapsed_ms: Number(elapsed_ms.toFixed(3)),
    ops_per_sec: Number((ORDERS / (elapsed_ms / 1000)).toFixed(1)),
    peak_rss_mb: readPeakRssMb(),
    checksum,
  }));
} else if (mode === "point_read") {
  const db = new Database(dbPath);
  db.exec(PRAGMA_POINT_READ);
  const sel = db.prepare("SELECT amount FROM orders WHERE id = ?;");

  let checksum = 0n;
  const t0 = Bun.nanoseconds();
  for (let k = 0; k < POINT_QUERIES; k++) {
    const targetId = ((k * 7919) % ORDERS) + 1;
    const row = sel.get(targetId);
    if (row) checksum += BigInt(row.amount);
  }
  const t1 = Bun.nanoseconds();
  db.exec("PRAGMA locking_mode=NORMAL;");
  db.close();

  const elapsed_ms = (t1 - t0) / 1e6;
  console.log(JSON.stringify({
    mode: "point_read",
    queries: POINT_QUERIES,
    elapsed_ms: Number(elapsed_ms.toFixed(3)),
    ops_per_sec: Number((POINT_QUERIES / (elapsed_ms / 1000)).toFixed(1)),
    peak_rss_mb: readPeakRssMb(),
    checksum: Number(checksum),
  }));
} else if (mode === "wal_worker") {
  const tid = Number(process.env.SQLITE_BENCH_TID || "0");
  const db = new Database(dbPath, { readonly: true });
  db.exec(PRAGMA_READ);
  const sel = db.prepare("SELECT amount FROM orders WHERE id = ?;");

  // Signal ready to coordinator, then wait for GO line on stdin
  process.stdout.write("READY\n");
  const reader = Bun.stdin.stream().getReader();
  let buf = "";
  while (!buf.includes("\n")) {
    const { value, done } = await reader.read();
    if (done) break;
    buf += new TextDecoder().decode(value);
  }

  let checksum = 0n;
  const offset = tid * 131071;
  const t0 = Bun.nanoseconds();
  for (let k = 0; k < POINT_QUERIES; k++) {
    const targetId = (((k + offset) * 7919) % ORDERS) + 1;
    const row = sel.get(targetId);
    if (row) checksum += BigInt(row.amount);
  }
  const t1 = Bun.nanoseconds();
  db.close();

  const elapsed_ms = (t1 - t0) / 1e6;
  process.stdout.write(JSON.stringify({
    tid,
    queries: POINT_QUERIES,
    elapsed_ms: Number(elapsed_ms.toFixed(3)),
    peak_rss_mb: readPeakRssMb(),
    checksum: Number(checksum),
  }) + "\n");
} else if (mode === "batch_materialize") {
  const BATCH_QUERIES = 500;
  const BATCH_CATS = 100;
  const db = new Database(dbPath, { readonly: true });
  db.exec(PRAGMA_READ);
  const sel = db.prepare("SELECT id, amount, status, customer, sku FROM orders_batch WHERE cat_id = ?;");

  let checksum = 0n;
  let totalRows = 0;
  const t0 = Bun.nanoseconds();
  for (let q = 0; q < BATCH_QUERIES; q++) {
    const cat = (q * 17) % BATCH_CATS;
    const rows = sel.all(cat);
    totalRows += rows.length;
    for (let i = 0; i < rows.length; i++) {
      const r = rows[i];
      const c = r.customer;
      const s = r.sku;
      const cl = c.length;
      const sl = s.length;
      checksum += BigInt(
        r.id + r.amount + r.status + cl + sl +
        (cl > 0 ? c.charCodeAt(0) + c.charCodeAt(cl - 1) : 0) +
        (sl > 0 ? s.charCodeAt(0) + s.charCodeAt(sl - 1) : 0)
      );
    }
  }
  const t1 = Bun.nanoseconds();
  db.close();

  const elapsed_ms = (t1 - t0) / 1e6;
  const rows_per_sec = Number((totalRows / (elapsed_ms / 1000)).toFixed(1));
  console.log(JSON.stringify({
    mode: "batch_materialize",
    queries: BATCH_QUERIES,
    rows: totalRows,
    elapsed_ms: Number(elapsed_ms.toFixed(3)),
    rows_per_sec,
    ops_per_sec: rows_per_sec,
    peak_rss_mb: readPeakRssMb(),
    checksum: Number(checksum),
  }));
}
"""

NODE_BENCH_MJS = r"""
import { DatabaseSync } from "node:sqlite";
import { readFileSync } from "node:fs";
import readline from "node:readline";

const ORDERS = 100000;
const POINT_QUERIES = 200000;
const ORDER_NOTE = "order_payload_item";

const PRAGMA_WRITE = "PRAGMA journal_mode=WAL; PRAGMA locking_mode=EXCLUSIVE; PRAGMA synchronous=NORMAL; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY;";
const PRAGMA_POINT_READ = "PRAGMA busy_timeout=5000; PRAGMA locking_mode=EXCLUSIVE; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;";
const PRAGMA_READ = "PRAGMA busy_timeout=5000; PRAGMA locking_mode=NORMAL; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;";

function readPeakRssMb() {
  const m = readFileSync("/proc/self/status", "utf8").match(/VmHWM:\s+(\d+)\s+kB/);
  return m ? Number((Number(m[1]) / 1024).toFixed(2)) : 0;
}

const mode = process.env.SQLITE_BENCH_MODE || "write";
const dbPath = process.env.SQLITE_BENCH_DB || "/tmp/node_sqlite_extreme.db";

if (mode === "write") {
  const db = new DatabaseSync(dbPath);
  db.exec(PRAGMA_WRITE);
  db.exec("DROP TABLE IF EXISTS orders;");
  db.exec("CREATE TABLE orders (id INTEGER PRIMARY KEY, customer_id INTEGER NOT NULL, amount INTEGER NOT NULL, status INTEGER NOT NULL, note TEXT NOT NULL);");

  const ins = db.prepare("INSERT INTO orders (id, customer_id, amount, status, note) VALUES (?, ?, ?, ?, ?);");
  const t0 = process.hrtime.bigint();
  db.exec("BEGIN;");
  for (let i = 1; i <= ORDERS; i++) {
    ins.run(i, ((i * 37) % 10000) + 1, ((i * 131) % 50000) + 100, i % 4, ORDER_NOTE);
  }
  db.exec("COMMIT;");
  const t1 = process.hrtime.bigint();

  db.exec("PRAGMA locking_mode=NORMAL;");
  const row = db.prepare("SELECT COUNT(*) AS cnt, SUM(amount) AS total FROM orders;").get();
  const checksum = Number(BigInt(row.cnt) + BigInt(row.total));
  db.close();

  const elapsed_ms = Number(t1 - t0) / 1e6;
  console.log(JSON.stringify({
    mode: "write",
    rows: ORDERS,
    elapsed_ms: Number(elapsed_ms.toFixed(3)),
    ops_per_sec: Number((ORDERS / (elapsed_ms / 1000)).toFixed(1)),
    peak_rss_mb: readPeakRssMb(),
    checksum,
  }));
} else if (mode === "point_read") {
  const db = new DatabaseSync(dbPath);
  db.exec(PRAGMA_POINT_READ);
  const sel = db.prepare("SELECT amount FROM orders WHERE id = ?;");

  let checksum = 0n;
  const t0 = process.hrtime.bigint();
  for (let k = 0; k < POINT_QUERIES; k++) {
    const targetId = ((k * 7919) % ORDERS) + 1;
    const row = sel.get(targetId);
    if (row) checksum += BigInt(row.amount);
  }
  const t1 = process.hrtime.bigint();
  db.exec("PRAGMA locking_mode=NORMAL;");
  db.close();

  const elapsed_ms = Number(t1 - t0) / 1e6;
  console.log(JSON.stringify({
    mode: "point_read",
    queries: POINT_QUERIES,
    elapsed_ms: Number(elapsed_ms.toFixed(3)),
    ops_per_sec: Number((POINT_QUERIES / (elapsed_ms / 1000)).toFixed(1)),
    peak_rss_mb: readPeakRssMb(),
    checksum: Number(checksum),
  }));
} else if (mode === "wal_worker") {
  const tid = Number(process.env.SQLITE_BENCH_TID || "0");
  const db = new DatabaseSync(dbPath, { readOnly: true });
  db.exec(PRAGMA_READ);
  const sel = db.prepare("SELECT amount FROM orders WHERE id = ?;");

  const rl = readline.createInterface({ input: process.stdin });
  process.stdout.write("READY\n");
  await new Promise((resolve) => {
    rl.once("line", () => {
      rl.close();
      resolve();
    });
  });

  let checksum = 0n;
  const offset = tid * 131071;
  const t0 = process.hrtime.bigint();
  for (let k = 0; k < POINT_QUERIES; k++) {
    const targetId = (((k + offset) * 7919) % ORDERS) + 1;
    const row = sel.get(targetId);
    if (row) checksum += BigInt(row.amount);
  }
  const t1 = process.hrtime.bigint();
  db.close();

  const elapsed_ms = Number(t1 - t0) / 1e6;
  process.stdout.write(JSON.stringify({
    tid,
    queries: POINT_QUERIES,
    elapsed_ms: Number(elapsed_ms.toFixed(3)),
    peak_rss_mb: readPeakRssMb(),
    checksum: Number(checksum),
  }) + "\n");
} else if (mode === "batch_materialize") {
  const BATCH_QUERIES = 500;
  const BATCH_CATS = 100;
  const db = new DatabaseSync(dbPath, { readOnly: true });
  db.exec(PRAGMA_READ);
  const sel = db.prepare("SELECT id, amount, status, customer, sku FROM orders_batch WHERE cat_id = ?;");

  let checksum = 0n;
  let totalRows = 0;
  const t0 = process.hrtime.bigint();
  for (let q = 0; q < BATCH_QUERIES; q++) {
    const cat = (q * 17) % BATCH_CATS;
    const rows = sel.all(cat);
    totalRows += rows.length;
    for (let i = 0; i < rows.length; i++) {
      const r = rows[i];
      const c = r.customer;
      const s = r.sku;
      const cl = c.length;
      const sl = s.length;
      checksum += BigInt(
        r.id + r.amount + r.status + cl + sl +
        (cl > 0 ? c.charCodeAt(0) + c.charCodeAt(cl - 1) : 0) +
        (sl > 0 ? s.charCodeAt(0) + s.charCodeAt(sl - 1) : 0)
      );
    }
  }
  const t1 = process.hrtime.bigint();
  db.close();

  const elapsed_ms = Number(t1 - t0) / 1e6;
  const rows_per_sec = Number((totalRows / (elapsed_ms / 1000)).toFixed(1));
  console.log(JSON.stringify({
    mode: "batch_materialize",
    queries: BATCH_QUERIES,
    rows: totalRows,
    elapsed_ms: Number(elapsed_ms.toFixed(3)),
    rows_per_sec,
    ops_per_sec: rows_per_sec,
    peak_rss_mb: readPeakRssMb(),
    checksum: Number(checksum),
  }));
}
"""

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------


def run_cmd_simple(cmd: list[str], cwd: Path | None = None) -> tuple[int, str, str]:
    proc = subprocess.run(
        cmd,
        cwd=str(cwd) if cwd else None,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
    )
    return proc.returncode, proc.stdout.strip(), proc.stderr.strip()


def run_with_peak_rss(
    cmd: list[str],
    env: dict[str, str] | None = None,
    cwd: Path | None = None,
) -> tuple[int, str, str, float]:
    full_env = os.environ.copy()
    if env:
        full_env.update(env)
    proc = subprocess.Popen(
        cmd,
        cwd=str(cwd) if cwd else None,
        env=full_env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    )
    assert proc.stdout is not None and proc.stderr is not None
    out_bytes = proc.stdout.read()
    err_bytes = proc.stderr.read()
    proc.stdout.close()
    proc.stderr.close()

    _, status, rusage = os.wait4(proc.pid, 0)
    proc.returncode = os.waitstatus_to_exitcode(status)
    peak_rss_mb = round(rusage.ru_maxrss / 1024.0, 2)
    return (
        proc.returncode,
        out_bytes.decode("utf-8", errors="replace").strip(),
        err_bytes.decode("utf-8", errors="replace").strip(),
        peak_rss_mb,
    )


def parse_last_json(out: str, err: str, label: str) -> dict[str, Any]:
    for stream in (out, err):
        lines = [ln.strip() for ln in stream.splitlines() if ln.strip().startswith("{")]
        if lines:
            return json.loads(lines[-1])
    raise RuntimeError(f"No JSON output from {label}\nstdout: {out}\nstderr: {err}")


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
    rc, out, err = run_cmd_simple([bin_path, *args])
    if rc != 0:
        return None
    text = out or err
    return text.splitlines()[0].strip() if text else None


def get_cpu_info() -> dict[str, Any]:
    model = platform.processor() or "unknown"
    cores = os.cpu_count() or 0
    rc, out, _ = run_cmd_simple(["lscpu"])
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


def remove_db_files(db_path: Path) -> None:
    for suffix in ("", "-wal", "-shm", "-journal"):
        p = Path(str(db_path) + suffix)
        if p.exists():
            p.unlink()


def compile_sqlite_extreme_and_cp() -> dict[str, Any]:
    TEMP_DIR.mkdir(parents=True, exist_ok=True)
    if not SQLITE_SRC_C.is_file():
        raise FileNotFoundError(f"Missing SQLite amalgamation: {SQLITE_SRC_C}")

    cc_bin = find_bin("clang") or find_bin("gcc") or find_bin("cc")
    if not cc_bin:
        raise RuntimeError("No C compiler (clang/gcc/cc) found")

    t0 = time.perf_counter()
    if SQLITE_EXTREME_OBJ.is_file() and SQLITE_EXTREME_OBJ.stat().st_mtime >= SQLITE_SRC_C.stat().st_mtime:
        print(f"==> Reusing up-to-date {SQLITE_EXTREME_OBJ} ({SQLITE_EXTREME_OBJ.stat().st_size // 1024} KB)")
        sqlite_compile_sec = 0.0
    else:
        c_cmd = [
            cc_bin,
            *SQLITE_CFLAGS,
            "-c",
            str(SQLITE_SRC_C),
            "-o",
            str(SQLITE_EXTREME_OBJ),
        ]
        print(f"==> Compiling sqlite3.c -> {SQLITE_EXTREME_OBJ} ({Path(cc_bin).name})...")
        rc, out, err = run_cmd_simple(c_cmd)
        if rc != 0:
            raise RuntimeError(f"C compile failed:\n{out}\n{err}")
        sqlite_compile_sec = round(time.perf_counter() - t0, 2)

    t1 = time.perf_counter()
    cpc_cmd = ["cpc", str(CP_SRC), "-I", str(CP_ROOT / "lib"), "-o", str(CP_ZIG)]
    print(f"==> Transpiling {CP_SRC.name} -> {CP_ZIG.name}...")
    rc, out, err = run_cmd_simple(cpc_cmd)
    if rc != 0:
        raise RuntimeError(f"cpc failed:\n{out}\n{err}")

    zig_cmd = [
        "zig",
        "build-exe",
        str(CP_ZIG),
        str(SQLITE_EXTREME_OBJ),
        "-OReleaseFast",
        "-lc",
        f"-femit-bin={CP_BIN}",
    ]
    print(f"==> Static-linking {CP_BIN.name} (-OReleaseFast + sqlite3_extreme.o)...")
    rc, out, err = run_cmd_simple(zig_cmd, cwd=TEMP_DIR)
    if rc != 0:
        raise RuntimeError(f"zig build-exe failed:\n{out}\n{err}")
    cp_build_sec = round(time.perf_counter() - t1, 2)

    return {
        "cc": cc_bin,
        "cc_version": get_version(cc_bin),
        "sqlite_cflags": SQLITE_CFLAGS,
        "sqlite_obj": str(SQLITE_EXTREME_OBJ),
        "sqlite_compile_sec": sqlite_compile_sec,
        "cp_build_sec": cp_build_sec,
        "cp_bin": str(CP_BIN),
    }


def run_single_mode(
    cmd: list[str],
    mode: str,
    db_path: Path,
    recreate_db: bool,
    trials: int = TRIALS,
    threads: int = CONCURRENT_WORKERS_8C,
) -> dict[str, Any]:
    samples: list[dict[str, Any]] = []
    for _ in range(trials):
        if recreate_db:
            remove_db_files(db_path)
        env = {
            "SQLITE_BENCH_MODE": mode,
            "SQLITE_BENCH_DB": str(db_path),
            "SQLITE_BENCH_THREADS": str(threads),
        }
        rc, out, err, wait4_rss = run_with_peak_rss(cmd, env=env)
        if rc != 0:
            raise RuntimeError(f"Failed ({mode}) {' '.join(cmd)}:\nstdout: {out}\nstderr: {err}")
        data = parse_last_json(out, err, f"{mode}:{' '.join(cmd)}")
        if "error" in data:
            raise RuntimeError(f"Benchmark error ({mode}): {data}")
        data["peak_rss_mb"] = round(max(float(data.get("peak_rss_mb", 0.0)), wait4_rss), 2)
        samples.append(data)

    elapsed_ms = round(statistics.median(s["elapsed_ms"] for s in samples), 3)
    peak_rss_mb = round(statistics.median(s["peak_rss_mb"] for s in samples), 2)
    checksums = {s["checksum"] for s in samples}
    if len(checksums) != 1:
        raise AssertionError(f"Inconsistent checksums across trials for {mode}: {checksums}")
    count = samples[0].get("rows") or samples[0].get("queries")
    ops_per_sec = round(count / (elapsed_ms / 1000.0), 1)
    return {
        "elapsed_ms": elapsed_ms,
        "ops_per_sec": ops_per_sec,
        "peak_rss_mb": peak_rss_mb,
        "checksum": samples[0]["checksum"],
        "trials_ms": [s["elapsed_ms"] for s in samples],
    }


def run_multiprocess_wal_read(
    cmd: list[str],
    db_path: Path,
    workers: int = CONCURRENT_WORKERS_8C,
    trials: int = TRIALS,
) -> dict[str, Any]:
    """Launch `workers` independent processes, synchronize via READY/GO barrier, measure wall time & total tree RSS."""
    samples: list[dict[str, Any]] = []

    for _ in range(trials):
        procs: list[subprocess.Popen[bytes]] = []
        for tid in range(workers):
            env = os.environ.copy()
            env["SQLITE_BENCH_MODE"] = "wal_worker"
            env["SQLITE_BENCH_DB"] = str(db_path)
            env["SQLITE_BENCH_TID"] = str(tid)
            p = subprocess.Popen(
                cmd,
                env=env,
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )
            procs.append(p)

        # Wait until all workers have opened DB + applied PRAGMAs + prepared SQL
        for tid, p in enumerate(procs):
            assert p.stdout is not None
            line = p.stdout.readline().decode("utf-8", errors="replace").strip()
            if line != "READY":
                err = p.stderr.read().decode("utf-8", errors="replace") if p.stderr else ""
                for other in procs:
                    other.kill()
                raise RuntimeError(f"Worker {tid} did not emit READY (got {line!r}): {err}")

        # Synchronized start
        t0 = time.perf_counter_ns()
        for p in procs:
            assert p.stdin is not None
            p.stdin.write(b"GO\n")
            p.stdin.flush()
            p.stdin.close()

        worker_results: list[dict[str, Any]] = []
        for tid, p in enumerate(procs):
            assert p.stdout is not None
            out_line = p.stdout.readline().decode("utf-8", errors="replace").strip()
            if not out_line.startswith("{"):
                rest = p.stdout.read().decode("utf-8", errors="replace")
                err = p.stderr.read().decode("utf-8", errors="replace") if p.stderr else ""
                raise RuntimeError(f"Worker {tid} bad JSON: {out_line!r} {rest}\n{err}")
            worker_results.append(json.loads(out_line))
        t1 = time.perf_counter_ns()

        total_rss_mb = 0.0
        for tid, p in enumerate(procs):
            assert p.stdout is not None and p.stderr is not None
            p.stdout.close()
            p.stderr.close()
            _, status, rusage = os.wait4(p.pid, 0)
            p.returncode = os.waitstatus_to_exitcode(status)
            if p.returncode != 0:
                raise RuntimeError(f"Worker {tid} exited with code {p.returncode}")
            wait4_rss = rusage.ru_maxrss / 1024.0
            self_rss = float(worker_results[tid].get("peak_rss_mb", 0.0))
            total_rss_mb += max(wait4_rss, self_rss)

        wall_ms = (t1 - t0) / 1e6
        total_checksum = sum(int(w["checksum"]) for w in worker_results)
        samples.append({
            "elapsed_ms": round(wall_ms, 3),
            "peak_rss_mb": round(total_rss_mb, 2),
            "checksum": total_checksum,
            "worker_max_ms": round(max(float(w["elapsed_ms"]) for w in worker_results), 3),
        })

    elapsed_ms = round(statistics.median(s["elapsed_ms"] for s in samples), 3)
    peak_rss_mb = round(statistics.median(s["peak_rss_mb"] for s in samples), 2)
    checksums = {s["checksum"] for s in samples}
    if len(checksums) != 1:
        raise AssertionError(f"Inconsistent checksums across multiprocess trials: {checksums}")
    total_queries = workers * POINT_QUERIES
    ops_per_sec = round(total_queries / (elapsed_ms / 1000.0), 1)
    return {
        "elapsed_ms": elapsed_ms,
        "ops_per_sec": ops_per_sec,
        "peak_rss_mb": peak_rss_mb,
        "checksum": samples[0]["checksum"],
        "processes": workers,
        "trials_ms": [s["elapsed_ms"] for s in samples],
    }


def build_batch_wal_db(db_path: Path) -> None:
    """Create shared WAL DB for batch row materialization (100,000 rows, 100 categories x 1,000 rows)."""
    remove_db_files(db_path)
    conn = sqlite3.connect(str(db_path))
    try:
        conn.executescript(
            """
            PRAGMA journal_mode=WAL;
            PRAGMA synchronous=NORMAL;
            PRAGMA mmap_size=268435456;
            PRAGMA cache_size=-64000;
            PRAGMA temp_store=MEMORY;
            CREATE TABLE orders_batch (
                id INTEGER PRIMARY KEY,
                cat_id INTEGER NOT NULL,
                amount INTEGER NOT NULL,
                status INTEGER NOT NULL,
                customer TEXT NOT NULL,
                sku TEXT NOT NULL
            );
            """
        )
        rows = (
            (
                i,
                (i - 1) % BATCH_CATS,
                ((i * 131) % 50000) + 100,
                i % 4,
                f"customer_{i:06d}_vip",
                f"SKU-{(i * 29) % 100000:05d}-PROD",
            )
            for i in range(1, ORDERS + 1)
        )
        with conn:
            conn.executemany(
                "INSERT INTO orders_batch (id, cat_id, amount, status, customer, sku) VALUES (?, ?, ?, ?, ?, ?);",
                rows,
            )
            conn.execute("CREATE INDEX idx_orders_cat ON orders_batch(cat_id);")
        conn.execute("PRAGMA wal_checkpoint(TRUNCATE);")
    finally:
        conn.close()


def ratio(other: float | None, base: float | None) -> float | None:
    if other is None or base is None or base <= 0:
        return None
    return round(other / base, 2)


def main() -> int:
    bun_bin = find_bin("bun", ["/root/.bun/bin/bun", "/data/dev/bun/build/release/bun"])
    node_bin = find_bin("node")
    if not bun_bin or not node_bin:
        raise RuntimeError(f"Missing runtime: bun={bun_bin}, node={node_bin}")

    build_info = compile_sqlite_extreme_and_cp()
    cpu_info = get_cpu_info()

    work_dir = Path(tempfile.mkdtemp(prefix="cp_sqlite_extreme_"))
    try:
        bun_script = work_dir / "bench_sqlite_extreme_bun.js"
        node_script = work_dir / "bench_sqlite_extreme_node.mjs"
        bun_script.write_text(BUN_BENCH_JS, encoding="utf-8")
        node_script.write_text(NODE_BENCH_MJS, encoding="utf-8")

        # ------------------------------------------------------------------
        # 1. Single-thread Bulk Write (1c, EXCLUSIVE, 100,000 rows in 1 tx)
        # ------------------------------------------------------------------
        print("==> [1/5] Single-thread bulk write (1c, EXCLUSIVE, 100,000 rows, 1 tx, prepared stmt)...")
        write_res: dict[str, Any] = {}
        cp_write_db = work_dir / "write_cp.db"
        bun_write_db = work_dir / "write_bun.db"
        node_write_db = work_dir / "write_node.db"

        write_res["cp"] = run_single_mode([str(CP_BIN)], "write", cp_write_db, recreate_db=True)
        write_res["bun"] = run_single_mode([bun_bin, str(bun_script)], "write", bun_write_db, recreate_db=True)
        write_res["node"] = run_single_mode([node_bin, str(node_script)], "write", node_write_db, recreate_db=True)

        w_ck = write_res["cp"]["checksum"]
        for rt, v in write_res.items():
            if v["checksum"] != w_ck:
                raise AssertionError(f"Write checksum mismatch for {rt}: {v['checksum']} != {w_ck}")

        # Use the cleanly generated WAL database from CP write for all read benchmarks.
        # Ensure NORMAL locking mode + WAL checkpoint (keeping journal_mode=WAL) so pages reside
        # in the main DB and PRAGMA mmap_size=268435456 takes effect fairly for all three runtimes.
        shared_wal_db = cp_write_db
        run_cmd_simple([
            bun_bin,
            "-e",
            f'import {{ Database }} from "bun:sqlite"; const db = new Database({json.dumps(str(shared_wal_db))}); db.exec("PRAGMA locking_mode=NORMAL; PRAGMA wal_checkpoint(TRUNCATE);"); db.close();',
        ])

        # ------------------------------------------------------------------
        # 2. Single-thread Indexed Point Read (1c, EXCLUSIVE, 200,000 queries)
        # ------------------------------------------------------------------
        print("==> [2/5] Single-thread indexed point lookup (1c, EXCLUSIVE, 200,000 queries)...")
        point_res: dict[str, Any] = {}
        point_res["cp"] = run_single_mode([str(CP_BIN)], "point_read", shared_wal_db, recreate_db=False)
        point_res["bun"] = run_single_mode([bun_bin, str(bun_script)], "point_read", shared_wal_db, recreate_db=False)
        point_res["node"] = run_single_mode([node_bin, str(node_script)], "point_read", shared_wal_db, recreate_db=False)

        p_ck = point_res["cp"]["checksum"]
        for rt, v in point_res.items():
            if v["checksum"] != p_ck:
                raise AssertionError(f"Point-read checksum mismatch for {rt}: {v['checksum']} != {p_ck}")

        # Explicitly reset WAL DB to NORMAL locking mode before 8c/16c concurrent reads.
        run_cmd_simple([
            bun_bin,
            "-e",
            f'import {{ Database }} from "bun:sqlite"; const db = new Database({json.dumps(str(shared_wal_db))}); db.exec("PRAGMA locking_mode=NORMAL; PRAGMA wal_checkpoint(TRUNCATE);"); db.close();',
        ])

        # ------------------------------------------------------------------
        # 3. 8-Core Concurrent WAL Read (8c, NORMAL, 1,600,000 total queries)
        # ------------------------------------------------------------------
        print("==> [3/5] 8-core concurrent WAL read (8c, NORMAL, 1,600,000 total queries)...")
        wal8_res: dict[str, Any] = {}
        wal8_res["cp"] = run_single_mode(
            [str(CP_BIN)],
            "wal_read_8t",
            shared_wal_db,
            recreate_db=False,
            threads=CONCURRENT_WORKERS_8C,
        )
        wal8_res["cp"]["processes"] = 1
        wal8_res["cp"]["threads"] = CONCURRENT_WORKERS_8C
        wal8_res["bun"] = run_multiprocess_wal_read(
            [bun_bin, str(bun_script)],
            shared_wal_db,
            workers=CONCURRENT_WORKERS_8C,
        )
        wal8_res["node"] = run_multiprocess_wal_read(
            [node_bin, str(node_script)],
            shared_wal_db,
            workers=CONCURRENT_WORKERS_8C,
        )

        m8_ck = wal8_res["cp"]["checksum"]
        for rt, v in wal8_res.items():
            if v["checksum"] != m8_ck:
                raise AssertionError(f"8-core WAL read checksum mismatch for {rt}: {v['checksum']} != {m8_ck}")

        # ------------------------------------------------------------------
        # 4. 16-Core Concurrent WAL Read (16c, NORMAL, 3,200,000 total queries)
        # ------------------------------------------------------------------
        print("==> [4/5] 16-core concurrent WAL read (16c, NORMAL, 3,200,000 total queries)...")
        wal16_res: dict[str, Any] = {}
        wal16_res["cp"] = run_single_mode(
            [str(CP_BIN)],
            "wal_read_16t",
            shared_wal_db,
            recreate_db=False,
            threads=CONCURRENT_WORKERS_16C,
        )
        wal16_res["cp"]["processes"] = 1
        wal16_res["cp"]["threads"] = CONCURRENT_WORKERS_16C
        wal16_res["bun"] = run_multiprocess_wal_read(
            [bun_bin, str(bun_script)],
            shared_wal_db,
            workers=CONCURRENT_WORKERS_16C,
        )
        wal16_res["node"] = run_multiprocess_wal_read(
            [node_bin, str(node_script)],
            shared_wal_db,
            workers=CONCURRENT_WORKERS_16C,
        )

        m16_ck = wal16_res["cp"]["checksum"]
        for rt, v in wal16_res.items():
            if v["checksum"] != m16_ck:
                raise AssertionError(f"16-core WAL read checksum mismatch for {rt}: {v['checksum']} != {m16_ck}")

        # ------------------------------------------------------------------
        # 5. Batch Row Materialization (500 queries x 1,000 rows = 500,000 rows)
        # ------------------------------------------------------------------
        print("==> [5/5] Batch row materialization (500 queries x 1,000 rows = 500,000 multi-column rows)...")
        batch_wal_db = work_dir / "batch_wal.db"
        build_batch_wal_db(batch_wal_db)

        batch_res: dict[str, Any] = {}
        batch_res["cp"] = run_single_mode([str(CP_BIN)], "batch_materialize", batch_wal_db, recreate_db=False)
        batch_res["bun"] = run_single_mode([bun_bin, str(bun_script)], "batch_materialize", batch_wal_db, recreate_db=False)
        batch_res["node"] = run_single_mode([node_bin, str(node_script)], "batch_materialize", batch_wal_db, recreate_db=False)

        for v in batch_res.values():
            v["rows_per_sec"] = v["ops_per_sec"]

        b_ck = batch_res["cp"]["checksum"]
        for rt, v in batch_res.items():
            if v["checksum"] != b_ck:
                raise AssertionError(f"Batch-materialize checksum mismatch for {rt}: {v['checksum']} != {b_ck}")

        final_data = {
            "timestamp_utc": datetime.datetime.now(datetime.timezone.utc).strftime("%Y-%m-%dT%H:%M:%SZ"),
            "host": {"cpu": cpu_info},
            "build": build_info,
            "runtimes": {
                "cp": {
                    "cpc_version": get_version(find_bin("cpc")),
                    "zig_version": get_version(find_bin("zig"), ["version"]),
                    "opt_mode": "ReleaseFast",
                    "sqlite_build": "source-compiled sqlite3_extreme.o (-O3 -march=native -DSQLITE_THREADSAFE=2)",
                },
                "bun": {"path": bun_bin, "version": get_version(bun_bin)},
                "node": {"path": node_bin, "version": get_version(node_bin)},
            },
            "pragmas": {
                "write": "PRAGMA journal_mode=WAL; PRAGMA locking_mode=EXCLUSIVE; PRAGMA synchronous=NORMAL; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY;",
                "point_read": "PRAGMA busy_timeout=5000; PRAGMA locking_mode=EXCLUSIVE; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;",
                "read": "PRAGMA busy_timeout=5000; PRAGMA locking_mode=NORMAL; PRAGMA mmap_size=268435456; PRAGMA cache_size=-64000; PRAGMA temp_store=MEMORY; PRAGMA query_only=ON;",
            },
            "trials": TRIALS,
            "benchmarks": {
                "single_thread_bulk_write": {
                    "rows": ORDERS,
                    "checksum": w_ck,
                    "results": write_res,
                    "cp_speedup_vs": {
                        "bun": ratio(write_res["bun"]["elapsed_ms"], write_res["cp"]["elapsed_ms"]),
                        "node": ratio(write_res["node"]["elapsed_ms"], write_res["cp"]["elapsed_ms"]),
                    },
                },
                "single_thread_point_read": {
                    "queries": POINT_QUERIES,
                    "checksum": p_ck,
                    "results": point_res,
                    "cp_speedup_vs": {
                        "bun": ratio(point_res["bun"]["elapsed_ms"], point_res["cp"]["elapsed_ms"]),
                        "node": ratio(point_res["node"]["elapsed_ms"], point_res["cp"]["elapsed_ms"]),
                    },
                    "rss_savings_vs": {
                        "bun": ratio(point_res["bun"]["peak_rss_mb"], point_res["cp"]["peak_rss_mb"]),
                        "node": ratio(point_res["node"]["peak_rss_mb"], point_res["cp"]["peak_rss_mb"]),
                    },
                },
                "concurrent_8core_wal_read": {
                    "workers": CONCURRENT_WORKERS_8C,
                    "queries_per_worker": POINT_QUERIES,
                    "total_queries": TOTAL_CONCURRENT_QUERIES_8C,
                    "checksum": m8_ck,
                    "results": wal8_res,
                    "cp_speedup_vs": {
                        "bun": ratio(wal8_res["bun"]["elapsed_ms"], wal8_res["cp"]["elapsed_ms"]),
                        "node": ratio(wal8_res["node"]["elapsed_ms"], wal8_res["cp"]["elapsed_ms"]),
                    },
                    "rss_savings_vs": {
                        "bun": ratio(wal8_res["bun"]["peak_rss_mb"], wal8_res["cp"]["peak_rss_mb"]),
                        "node": ratio(wal8_res["node"]["peak_rss_mb"], wal8_res["cp"]["peak_rss_mb"]),
                    },
                },
                "concurrent_16core_wal_read": {
                    "workers": CONCURRENT_WORKERS_16C,
                    "queries_per_worker": POINT_QUERIES,
                    "total_queries": TOTAL_CONCURRENT_QUERIES_16C,
                    "checksum": m16_ck,
                    "results": wal16_res,
                    "cp_speedup_vs": {
                        "bun": ratio(wal16_res["bun"]["elapsed_ms"], wal16_res["cp"]["elapsed_ms"]),
                        "node": ratio(wal16_res["node"]["elapsed_ms"], wal16_res["cp"]["elapsed_ms"]),
                    },
                    "rss_savings_vs": {
                        "bun": ratio(wal16_res["bun"]["peak_rss_mb"], wal16_res["cp"]["peak_rss_mb"]),
                        "node": ratio(wal16_res["node"]["peak_rss_mb"], wal16_res["cp"]["peak_rss_mb"]),
                    },
                },
                "batch_row_materialization": {
                    "queries": BATCH_QUERIES,
                    "rows_per_query": BATCH_ROWS_PER_QUERY,
                    "total_rows": TOTAL_BATCH_ROWS,
                    "checksum": b_ck,
                    "results": batch_res,
                    "cp_speedup_vs": {
                        "bun": ratio(batch_res["bun"]["elapsed_ms"], batch_res["cp"]["elapsed_ms"]),
                        "node": ratio(batch_res["node"]["elapsed_ms"], batch_res["cp"]["elapsed_ms"]),
                    },
                    "rss_savings_vs": {
                        "bun": ratio(batch_res["bun"]["peak_rss_mb"], batch_res["cp"]["peak_rss_mb"]),
                        "node": ratio(batch_res["node"]["peak_rss_mb"], batch_res["cp"]["peak_rss_mb"]),
                    },
                },
            },
        }

        OUT_JSON.write_text(json.dumps(final_data, indent=2) + "\n", encoding="utf-8")
        print(f"==> Wrote {OUT_JSON}")
        print(json.dumps(final_data["benchmarks"], indent=2))
        return 0
    finally:
        shutil.rmtree(work_dir, ignore_errors=True)


if __name__ == "__main__":
    raise SystemExit(main())
