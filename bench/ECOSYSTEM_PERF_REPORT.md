# CP Ecosystem Phase 4C Performance Benchmark & Quality Audit Report

> **Date**: 2026-11-27  
> **Scope**: Phase 4C Top 8 SIMD / Slice-by-4 LUT / `@memcpy` Core Packages (`ReleaseFast` native LLVM binary) + Full-Repository `cap/test/testCap*.cp` Regression Audit

---

## 1. Test Environment & Toolchain

| Item | Specification |
| :--- | :--- |
| **CPU** | Intel(R) Xeon(R) Platinum 8175M CPU @ 2.50GHz (2 sockets × 24 cores × 2 threads = 96 vCPUs) |
| **Memory** | 125 GiB RAM |
| **OS / Kernel** | Linux 6.12.0-284.el10.x86_64 (x86_64) |
| **CP Compiler** | `cpc 1.0.0` (modular transpilation) |
| **CAP Toolchain** | `cap v1.0.0` (`cap/bin/cap`) |
| **Backend Compiler** | `zig 0.17.0` (`-OReleaseFast -fllvm`) |
| **Timing Primitive** | `HwTimer.nowNs()` (`cap:bench`) + `std.mem.doNotOptimizeAway(&sink)` |
| **Benchmark Source** | `temp/bench_phase4c.cp` |

### Build & Run Command

```bash
./cap/bin/cap build -m -f temp/bench_phase4c.cp
zig build-exe .cap/zig/bench_phase4c.zig -OReleaseFast -fllvm -femit-bin=bin/temp/bench_phase4c
./bin/temp/bench_phase4c
```

---

## 2. Phase 4C Top 8 Core Packages — ReleaseFast Native Throughput

All benchmarks run single-threaded after warmup, with live checksum sinks verified against exact analytical values to prevent dead-code elimination.

| # | Package & Operation | Optimization | Payload / Input | Iterations | Latency (`ns/op`) | Throughput (`ops/s`) | Bandwidth (`MB/s`) |
| :-: | :--- | :--- | :--- | --: | --: | --: | --: |
| 1 | **`crc32fast`** (`hashBytes`) | Slice-by-4 LUT | 64 KB buffer | 20,000 | 54,965.76 | 18,193 | **1,137.07 MB/s** |
| 2 | **`crc32c`** (`crc32c`) | 256-entry LUT | 64 KB buffer | 20,000 | 138,707.48 | 7,209 | **450.59 MB/s** |
| 3 | **`utf8parse`** (`validateUtf8`) | 16-lane SIMD ASCII fast path | 64 KB ASCII | 20,000 | 3,508.13 | 285,052 | **17,815.77 MB/s** |
| 4a | **`lz4_flex`** (`compressBlock`) | Hash-table + 4B match | 4 KB JSON-log text | 50,000 | 1,862.81 | 536,822 | **2,096.96 MB/s** |
| 4b | **`lz4_flex`** (`decompressBlock`) | `@memcpy` literal/match copy | 4 KB (148 B compressed) | 50,000 | 1,251.47 | 799,060 | **3,121.33 MB/s** |
| 5 | **`url`** (`WhatwgUrl.parse`) | 16-lane SIMD scan, zero-alloc | 96 B full HTTPS URL | 500,000 | 487.78 | **2,050,084** | 187.69 MB/s |
| 6a | **`qs`** (`percentDecode`) | 16-lane SIMD scan | 118 B percent-encoded | 500,000 | 170.40 | **5,868,609** | **660.42 MB/s** |
| 6b | **`qs`** (`QsParsed.parseInto`) | 16-lane SIMD + stack pool | 91 B (8 nested/array params) | 200,000 | 367.35 | **2,722,234** | 236.25 MB/s |
| 7 | **`semver`** (`parseSemver` + `compareSemver`) | 16-lane SIMD digit/ident scan | 2× full prerelease+build | 1,000,000 | 156.40 | **6,393,994** | — |
| 8 | **`picomatch`** (`isMatch`) | 16-lane SIMD segment match | 47 B path vs `src/**/codegen/*.cp` | 1,000,000 | 343.42 | **2,911,885** | — |

### Key Highlights
- **`utf8parse.validateUtf8`** reaches **17.82 GB/s** on 64 KB ASCII buffers, saturating L1/L2 cache bandwidth via 16-lane SIMD vectorization.
- **`lz4_flex`** achieves **2.10 GB/s** compression and **3.12 GB/s** `@memcpy`-accelerated decompression on structured 4 KB JSON log payloads (27.7× compression ratio: 4,096 B → 148 B).
- **`crc32fast`** Slice-by-4 LUT hits **1.14 GB/s** (2.52× faster than single-byte 256-entry LUT `crc32c` at **450.59 MB/s**).
- **Zero-allocation parsers**: `semver` (`parseSemver` + `compareSemver`) exceeds **6.39M ops/s** (156 ns/op), `qs.percentDecode` hits **5.87M ops/s** (660 MB/s), `picomatch.isMatch` hits **2.91M ops/s** (343 ns/op), `qs.parse` hits **2.72M ops/s** (367 ns/op), and full `WhatwgUrl.parse` exceeds **2.05M ops/s** (488 ns/op).

---

## 3. Full-Repository Ecosystem Regression Audit (`cap/test/testCap*.cp`)

Executed serially via `./cap/bin/cap <test_file>` (60 s per-test timeout, zero file modifications, elapsed 1,025.6 s):

| Metric | Count | Ratio |
| :--- | --: | --: |
| **TOTAL** | **854** | 100.00% |
| **PASS** | **819** | **95.90%** |
| **FAIL** | **35** | 4.10% |

### Failure Classification (35 Total)

| Category | Count | Affected Tests |
| :--- | --: | :--- |
| `import of file outside module path` | 18 | `testCapChacha20poly1305`, `testCapColorNames`, `testCapColorSpace`, `testCapCronExpr`, `testCapDer`, `testCapFixedQ16`, `testCapFixedbitset`, `testCapGovernor`, `testCapHalf`, `testCapJsonSchema`, `testCapMat2`, `testCapOciRef`, `testCapOid`, `testCapRayCast`, `testCapSiphasher`, `testCapTruncUtf8`, `testCapUnicodeSeg`, `testCapUtf8Path` |
| `duplicate struct member name` (flattened monolith collision) | 9 | `testCapAccepts`, `testCapCamelcaseKeys`, `testCapChacha20Poly1305`, `testCapCombine`, `testCapQsLite`, `testCapSec1`, `testCapSend`, `testCapSpki`, `testCapX509Cert` |
| `TIMEOUT (>60s)` | 3 | `testCapIcuLocid`, `testCapShortid`, `testCapTrie` |
| Type / API / Runtime Assertion | 5 | `testCapCookieSignature` (runtime FAIL), `testCapIpRange` (E0207 type mismatch), `testCapMediatype` (`[]u8` vs `usize`), `testCapPMap` (`[0]PMapErrorEntry` pointer mismatch), `testCapRoaringBitmap` (missing `RoaringBitmap.new`) |
