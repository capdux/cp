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

---

## 4. Phase 4D~4J Top 8 Newly Accelerated Packages — ReleaseFast Native Throughput

> **Date**: 2026-11-27  
> **Scope**: 8 Representative High-Frequency Encoding / Parsing / Escaping / Security Packages Accelerated in Phase 4D~4J (`@Vector(16, u8)` SIMD + `comptime` LUT + `@memcpy`)  
> **Build**: `./cap/bin/cap build -m -f temp/bench_phase4dj.cp` + `zig build-exe .cap/zig/bench_phase4dj.zig -OReleaseFast -fllvm`  
> **Anti-DCE / Anti-LICM**: Warmup + live checksum accumulators + per-iteration `std.mem.doNotOptimizeAway` on read-only scan buffers, every benchmark sampled for `>= 43 ms` (43.7 ms – 1,506 ms).

| # | Package & Operation | Optimization | Payload / Input | Iterations | Latency (`ns/op`) | Throughput (`ops/s`) | Bandwidth (`MB/s`) |
| :-: | :--- | :--- | :--- | --: | --: | --: | --: |
| 1a | **`base64ct`** (`encodeStd`) | 16-lane CT table-free SIMD | 64 KB binary → 87 KB | 5,000 | 60,265.39 | 16,593 | **1,037.08 MB/s** |
| 1b | **`base64ct`** (`decodeStd`) | 16-lane CT table-free SIMD | 87 KB Base64 → 64 KB | 5,000 | 51,614.16 | 19,375 | **1,614.59 MB/s** |
| 2a | **`hex`** (`encodeHex`) | 4-byte unrolled LUT encode | 64 KB binary → 128 KB | 5,000 | 51,949.73 | 19,249 | **1,203.09 MB/s** |
| 2b | **`hex`** (`isValidHex`) | 16-lane SIMD range check | 128 KB hex string | 10,000 | 6,013.39 | 166,295 | **20,786.93 MB/s** |
| 2c | **`hex`** (`decodeHex`) | 4-byte unrolled `comptime` LUT | 128 KB hex → 64 KB | 5,000 | 83,008.45 | 12,047 | **1,505.87 MB/s** |
| 3a | **`html-escape`** (`escapeHtml`, clean) | 16-lane SIMD scan + `@memcpy` | 64 KB clean text | 20,000 | 7,854.36 | 127,318 | **7,957.36 MB/s** |
| 3b | **`html-escape`** (`escapeHtml`, mixed) | 16-lane SIMD scan + LUT | 4 KB realistic HTML | 20,000 | 7,017.04 | 142,510 | **555.73 MB/s** |
| 3c | **`html-escape`** (`unescapeHtml`) | 16-lane `&` SIMD scan + `@memcpy` | Escaped HTML → 4 KB | 20,000 | 10,176.21 | 98,268 | **647.48 MB/s** |
| 4a | **`csv-core`** (`countCsvRecords`) | 16-lane SIMD quote/LF scan | 64 KB quoted CSV | 10,000 | 150,623.28 | 6,639 | **414.84 MB/s** |
| 4b | **`csv-core`** (`CsvReader.readField`) | 16-lane SIMD field/quote DFA | 4 KB quoted CSV (full parse) | 5,000 | 9,790.03 | 102,145 | **395.11 MB/s** |
| 5a | **`cobs`** (`encode`) | 16-lane SIMD zero-byte scan + `@memcpy` | 64 KB binary (zeros / 41 B) | 10,000 | 18,824.14 | 53,123 | **3,320.21 MB/s** |
| 5b | **`cobs`** (`decode`) | Block-copy `@memcpy` decode | 64 KB COBS frame | 10,000 | 19,611.52 | 50,990 | **3,186.95 MB/s** |
| 6a | **`bech32`** (`encodeBech32`) | `comptime` 256-entry LUT + branchless polymod | 32 B payload → 61 B Bech32m | 500,000 | 214.60 | **4,659,841** | 142.21 MB/s |
| 6b | **`bech32`** (`decodeBech32`) | `comptime` LUT + branchless polymod | 61 B Bech32m → 32 B | 500,000 | 307.56 | **3,251,389** | 189.15 MB/s |
| 6c | **`bech32`** (`verifyBech32`) | `comptime` LUT + branchless polymod | 61 B Bech32m checksum | 1,000,000 | 177.22 | **5,642,748** | 328.26 MB/s |
| 7a | **`jsonpointer`** (`countSegments`) | 16-lane SIMD `'/'` scan | 152 B path (18 segments) | 5,000,000 | 14.43 | **69,294,527** | **9,780.49 MB/s** |
| 7b | **`jsonpointer`** (`escapeToken`) | 16-lane SIMD `'~'`/`'/'` scan + `@memcpy` | 1 KB token | 200,000 | 1,567.59 | **637,922** | **620.54 MB/s** |
| 7c | **`jsonpointer`** (`unescapeToken`) | 16-lane SIMD `'~'` scan + `@memcpy` | 1.1 KB escaped token | 200,000 | 1,325.57 | **754,391** | **834.55 MB/s** |
| 8a | **`cookie-signature`** (`ctEqual`) | 16-lane SIMD constant-time XOR | 4 KB equal buffers (L1) | 500,000 | 87.35 | **11,448,098** | **44,719.13 MB/s** |
| 8b | **`cookie-signature`** (`sign`) | Pure-CP HMAC-SHA256 + Base64URL | 38 B session + 32 B secret | 200,000 | 1,765.02 | **566,566** | — |
| 8c | **`cookie-signature`** (`unsign`) | HMAC-SHA256 + 16-lane SIMD `ctEqual` | Signed cookie + 32 B secret | 200,000 | 1,798.89 | **555,900** | — |

### Phase 4D~4J Key Highlights
- **Cache-Saturating SIMD Verification & Constant-Time Ops**: `cookie-signature.ctEqual` reaches **44.72 GB/s** (87.35 ns per 4 KB L1-resident comparison), `hex.isValidHex` hits **20.79 GB/s** on 128 KB buffers, `jsonpointer.countSegments` hits **69.29M ops/s** (**9.78 GB/s**), and `html-escape.escapeHtml` clean fast-path hits **7.96 GB/s**.
- **Multi-GB/s Framing & Constant-Time Codecs**: `cobs` reaches **3.32 GB/s** encode and **3.19 GB/s** decode; table-free constant-time `base64ct` breaks the 1 GB/s barrier at **1.04 GB/s** encode and **1.61 GB/s** decode; `hex` reaches **1.20 GB/s** encode and **1.51 GB/s** decode.
- **Zero-Allocation Protocol & Crypto Throughput**: `bech32` achieves **5.64M ops/s** verification (177 ns/op), **4.66M ops/s** encoding (215 ns/op), and **3.25M ops/s** full decoding (308 ns/op) via `comptime` LUT + branchless polymod; `csv-core` sustains **414.84 MB/s** record counting and **395.11 MB/s** full quoted field streaming; `cookie-signature` sustains **566K ops/s** HMAC-SHA256 signing and **556K ops/s** constant-time verification.
