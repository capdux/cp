# CP vs Bun vs Node.js vs Python3 — Native Cross-Runtime Benchmark Suite

100% reproducible, apples-to-apples benchmark suite executed on the same host machine with identical algorithms, payloads, and verified checksums across all runtimes.

## Test Environment

- **Date (UTC)**: `2026-11-24T15:58:25Z`
- **CPU**: `Intel(R) Xeon(R) Platinum 8175M CPU @ 2.50GHz` (96 logical CPUs, `x86_64`)
- **OS**: `Linux 6.12.0-280.el10.x86_64`
- **CP**: `cpc 1.0.0` / `cap 1.0.0` (Zig `0.17.0`, `-OReleaseFast`)
- **Bun**: `1.3.14`
- **Node.js**: `v26.1.0`
- **Python3**: `Python 3.12.15`

---

## Summary Results

### Bench 1 — SQLite `:memory:` (100,000 Batch Inserts + 100,000 PK Point Lookups)

| Runtime | Engine | Write 100K (ms) | Read 100K (ms) | Total (ms) | Throughput (ops/s) | CP Speedup |
|---|---|---:|---:|---:|---:|---:|
| **CP (`-OReleaseFast`)** | `cap:sqlite` (`libsqlite3.a`) | 117.55 ms | 150.77 ms | 268.32 ms | 745.4K/s | 1.00x (baseline) |
| Bun | `bun:sqlite` | 188.52 ms | 249.80 ms | 438.87 ms | 455.7K/s | **1.64x** |
| Node.js | `node:sqlite` | 195.68 ms | 394.25 ms | 562.48 ms | 355.6K/s | **2.10x** |
| Python3 | `sqlite3` | 294.74 ms | 772.20 ms | 1066.94 ms | 187.5K/s | **3.98x** |

> Checksum verified across all runtimes: `35002150000`

### Bench 2 — High-Frequency Compute (`fib(38)`) & Zero-GC 10M `u32` Hash Reduction

| Runtime | `fib(38)` (ms) | 10M `u32` Reduce (ms) | Total (ms) | Reduce Rate (ops/s) | CP Speedup |
|---|---:|---:|---:|---:|---:|
| **CP (`-OReleaseFast`)** | 385.04 ms | 16.33 ms | 401.48 ms | 612.44M/s | 1.00x (baseline) |
| Bun | 619.41 ms | 183.35 ms | 795.17 ms | 54.54M/s | **1.98x** |
| Node.js | 1654.33 ms | 26.54 ms | 1680.87 ms | 376.73M/s | **4.19x** |
| Python3 | 6795.51 ms | 2563.63 ms | 10391.92 ms | 3.90M/s | **25.88x** |

> Verified outputs across all runtimes: `fib(38) = 39088169`, `reduce_hash = 79652421`

### Bench 3 — Process Cold-Start Latency (Median of 20 Runs) & Binary / Runtime Size

| Runtime | Cold Start Median | Min / Max | CP Speedup | Executable / Runtime Size |
|---|---:|---:|---:|---:|
| **CP (`-OReleaseFast`)** | 1.61 ms | 1.45 / 1.80 ms | 1.00x (baseline) | 3.777 MB (standalone native binary) |
| Bun | 15.07 ms | 13.81 / 22.22 ms | **9.38x** | 88.456 MB (bun runtime binary) |
| Node.js | 32.14 ms | 28.84 / 41.68 ms | **20.01x** | 140.611 MB (node runtime binary) |
| Python3 | 12.40 ms | 12.22 / 13.40 ms | **7.72x** | 0.015 MB (python3 ELF binary (excl. stdlib)) |

### Bench 4 — JSON Parsing (10,000 Parses of `1749`-byte Payload)

| Runtime | Parser | Total 10K Parses (ms) | Parses / sec | Throughput (MB/s) | CP Speedup |
|---|---|---:|---:|---:|---:|
| **CP (`-OReleaseFast`)** | `@cap/yyjson` (`libyyjson.a`) | 22.07 ms | 453.1K/s | 755.8 MB/s | 1.00x (baseline) |
| Bun | `JSON.parse` (JSC) | 108.74 ms | 92.0K/s | 153.4 MB/s | **4.93x** |
| Node.js | `JSON.parse` (V8) | 81.37 ms | 122.9K/s | 205.0 MB/s | **3.69x** |
| Python3 | `json.loads` (`_json` C ext) | 230.44 ms | 43.4K/s | 72.4 MB/s | **10.44x** |

> Checksum verified across all runtimes: `11740000`

---

## One-Command Reproduction

```bash
python3 /data/dev/cp/bench/vs_runtimes/run_all.py
```

All CP sources are compiled into a temporary directory using `cpc` + `zig build-exe -OReleaseFast` and automatically cleaned up after execution.
