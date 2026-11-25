# CP Language (`cp-lang`) — v1.0.0

> **TypeScript's Ergonomics · Rust's Safety · Zig's Native Performance**
> A zero-GC, deterministic-RAII, AI-first systems & concurrent agent programming language compiling to native machine code.

[![License: MIT OR Apache-2.0](https://img.shields.io/badge/license-MIT%20OR%20Apache--2.0-blue.svg)](LICENSE-MIT)

---

## Highlights

- **Zero-GC Deterministic Memory Safety**: Unified 3-mode ownership (`read-only` / `*` mutable borrow / `move` / `own`) with idempotent `~init()` / `~errinit()` stack RAII destructors and compile-time discard protection (`E0220`).
- **AI-First Self-Healing Diagnostics**: Every compiler error carries `cause`, `help`, and actionable `suggestion` lines (`cpc check`, `cpc brief`, `cpc fix`).
- **Batteries-Included Standard Library (`cap:*`)**:
  - Embedded **SQLite 3.54.1** (`cap:sqlite`, 2.64M point-lookups/s `:memory:`, FTS5/JSON1/RTREE, zero install);
  - Linear-time **SIMD + Lazy DFA Regex** (`cap:text/nfaRegex`, ReDoS-free);
  - Pure-CP **Sans-IO WebRTC** (`cap:rtc`), **HTTP/1.1/2/3**, **PostgreSQL / MySQL / Redis** wire protocols;
  - Zero-dependency **IANA TimeZone** (`cap:tz`), **LEB128**, **Progress**, **ZON**, **ELF/COFF/Mach-O/PDB/SPIR-V**, **SIMD**, **Vector/RAG**, **Crypto**, and **Image** codecs.
- **All-in-One Toolchain (`cap` 2.0)**:
  - Concise `cap.json` 2.0 (`dep` / `dev` / `bin` / `sys` + `workspaces`);
  - On-demand AST link tree-shaking + `-g0` lean static library builds (72%–84% smaller `.a`);
  - Modular parallel `.o` hash cache & incremental linking (`cap build -m` / `cap run -m`, `--low` for low-mem serial);
  - Dual-format distribution: `.tar.zst` split archives (`cap pack [--strip]`) & RFC 1952 Gzip `.tgz` direct publishing to npm / Verdaccio (`cap publish`);
  - Built-in formatter (`cap fmt`), static purity linter (`cap lint`), test runner (`cap test`), microsecond benchmarks (`cap bench`), and LSP (`cpc lsp`).
- **43+ Permissive Official Native Ecosystem Packages (`@cap/*`)**:
  - AI & Vector: `@cap/llama`, `@cap/whisper`, `@cap/onnx`, `@cap/usearch`, `@cap/openblas`
  - Database & KV: `@cap/duckdb`, `@cap/lmdb`
  - Audio / Video / RTC: `@cap/opus`, `@cap/miniaudio`, `@cap/rnnoise`, `@cap/speexdsp`, `@cap/dr_libs`, `@cap/flac`, `@cap/vorbis`, `@cap/ogg`, `@cap/libyuv`, `@cap/dav1d`, `@cap/libvpx`, `@cap/libavif`, `@cap/libsrtp`, `@cap/usrsctp`
  - Vision / Graphics / Text: `@cap/opencv`, `@cap/pdfium`, `@cap/sdl3`, `@cap/stb`, `@cap/freetype`, `@cap/harfbuzz`, `@cap/cmark`, `@cap/treesitter`, `@cap/pcre2`
  - Compression / Parsing / Security / Net: `@cap/zstd`, `@cap/lz4`, `@cap/snappy`, `@cap/miniz`, `@cap/yyjson`, `@cap/yaml`, `@cap/expat`, `@cap/utf8proc`, `@cap/xxhash`, `@cap/picohttp`, `@cap/hdrhistogram`, `@cap/sodium`, `@cap/mbedtls`, `@cap/curl`
  - 100% permissive licenses (**0 GPL/AGPL/LGPL contamination**, see [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)).

---

## Benchmarks (Same-Machine 1c / 8c / 16c, Err=0)

Measured on **Intel Xeon Platinum 8175M** (Linux 6.12 x86_64, idle CPU exclusive run): **CP v1.0.0** (`-OReleaseFast`, single process) vs **Bun 1.3.14** vs **Node v26.1.0** (`Err=0` across all HTTP & WebSocket runs). Reproducible via [`bench/vs_web_frameworks/`](bench/vs_web_frameworks/).

### HTTP/1.1 Keep-Alive Scaling (`GET /`, `GET /users/:id`, `POST /json`)

| Cores | Runtime | `GET /` (QPS) | `GET /users/:id` (QPS) | `POST /json` (QPS) | Peak RSS |
| :---: | :--- | ---: | ---: | ---: | ---: |
| **1c** | **CP v1.0.0** (1-proc) | **110,893** | **97,752** | **111,296** | **6.4 MB** |
| 1c | Bun 1.3.14 (1-proc) | 66,339 | 54,344 | 44,252 | 49.0 MB |
| 1c | Node v26.1.0 (1-proc) | 43,456 | 38,521 | 31,740 | 83.6 MB |
| **8c** | **CP v1.0.0** (1-proc) | **598,957** | **719,167** | **792,562** | **19.2 MB** |
| 8c | Bun 1.3.14 (8-proc) | 435,306 | 341,943 | 322,240 | 422.8 MB |
| 8c | Node v26.1.0 (8-proc) | 324,534 | 283,145 | 201,264 | 724.2 MB |
| **16c** | **CP v1.0.0** (1-proc) | **716,523** | **907,947** | **960,055** | **33.9 MB** |
| 16c | Bun 1.3.14 (16-proc) | 582,171 | 540,996 | 442,279 | 801.3 MB |
| 16c | Node v26.1.0 (16-proc) | 438,624 | 390,054 | 264,342 | 1,378.3 MB |

### Full Web Framework Benchmark (24 Routes + Middleware + Params/Query + JSON Validation, Err=0)

CP `cap:web` vs **Bun Elysia** vs **Node Fastify** (24 identical routes: 12 static, 8 `:param`, 2 wildcard, 2 POST JSON + global Cookie/Bearer auth middleware):

| Cores | Framework | `GET /health` | `GET :id/:postId+?` | `POST /orders` (JSON) | `GET /me` (Cookie+Auth) | Peak RSS |
| :---: | :--- | ---: | ---: | ---: | ---: | ---: |
| **1c** | **CP v1.0.0 (`cap:web`)** | **116,469** | **111,592** | **111,536** | **112,327** | **6.4 MB** |
| 1c | Bun 1.3.14 (`Elysia`) | 56,109 | 42,047 | 41,106 | 51,060 | 86.4 MB |
| 1c | Node v26.1.0 (`Fastify`) | 36,267 | 33,659 | 24,267 | 29,572 | 97.4 MB |
| **8c** | **CP v1.0.0 (`cap:web`)** | **602,224** | **691,439** | **679,413** | **657,812** | **19.3 MB** |
| 8c | Bun 1.3.14 (`Elysia`) | 350,392 | 302,144 | 212,878 | 321,619 | 763.6 MB |
| 8c | Node v26.1.0 (`Fastify`) | 252,491 | 238,924 | 175,220 | 217,368 | 852.4 MB |
| **16c** | **CP v1.0.0 (`cap:web`)** | **962,608** | **984,689** | **977,586** | **957,553** | **33.9 MB** |
| 16c | Bun 1.3.14 (`Elysia`) | 547,324 | 471,870 | 345,207 | 482,075 | 1,402.7 MB |
| 16c | Node v26.1.0 (`Fastify`) | 411,132 | 394,936 | 294,450 | 420,927 | 1,621.4 MB |

### Live TCP WebSocket (128B Masked Echo, Err=0)

| Cores | Runtime | Throughput (msg/s) | P50 / P99 Latency | Peak RSS |
| :---: | :--- | ---: | :---: | ---: |
| **1c** | **CP v1.0.0** (1-proc) | **108,276** | 0.29 / 0.59 ms | **4.5 MB** |
| 1c | Bun 1.3.14 (1-proc) | 109,243 | 0.28 / 0.58 ms | 46.5 MB |
| 1c | Node v26.1.0 (1-proc) | 69,363 | 0.41 / 0.99 ms | 75.5 MB |
| **8c** | **CP v1.0.0** (1-proc) | **693,456** | **0.04 / 0.10 ms** | **17.2 MB** |
| 8c | Bun 1.3.14 (8-proc) | 577,571 | 0.05 / 0.15 ms | 397.5 MB |
| 8c | Node v26.1.0 (8-proc) | 458,735 | 0.06 / 0.23 ms | 647.6 MB |
| **16c** | **CP v1.0.0** (1-proc) | **1,002,530** | **0.04 / 0.11 ms** | **31.7 MB** |
| 16c | Bun 1.3.14 (16-proc) | 863,226 | 0.04 / 0.17 ms | 746.1 MB |
| 16c | Node v26.1.0 (16-proc) | 817,645 | 0.06 / 0.12 ms | 1,234.1 MB |

### Live TCP WebSocket Chat Broadcast (32 Clients, 1-Core, Err=0)

Matches Bun's official 32-client WebSocket chat broadcast benchmark (`publish` vs `send` vs Node `ws`), measured on the same machine with identical RFC 6455 masked frames:

| Target (1c) | Broadcast Throughput | Bandwidth | Err | Peak RSS | CP Speedup |
| :--- | ---: | ---: | :---: | ---: | :---: |
| **CP v1.0.0 (`publish`)** | **20,447,384 msgs/s** | **624.00 MB/s** | **0** | **4.5 MB** | **1.00x (baseline)** |
| Bun 1.3.14 (`publish`) | 3,541,021 msgs/s | 108.06 MB/s | 0 | 73.1 MB | **5.77x faster · 16.2x less RAM** |
| Bun 1.3.14 (`send`) | 158,685 msgs/s | 4.84 MB/s | 0 | 47.4 MB | **128.9x faster · 10.5x less RAM** |
| Node v26.1.0 (`ws`) | 122,942 msgs/s | 3.75 MB/s | 0 | 71.6 MB | **166.3x faster · 15.9x less RAM** |

### SQLite 3.54.1 Extreme Suite (WAL Reads & Row Materialization, 3-Trial Median)

| Cores / Workload | CP v1.0.0 (`cap:sqlite`) | Bun 1.3.14 (`bun:sqlite`) | Node v26.1.0 (`node:sqlite`) | CP Speedup (vs Bun / Node) |
| :--- | ---: | ---: | ---: | :---: |
| **1c** Bulk Write (100K rows) | **2,048,215 rows/s** (18.5 MB) | 1,026,905 rows/s (77.0 MB) | 953,116 rows/s (59.4 MB) | **1.99x / 2.15x** |
| **1c** Point Read (200K queries) | **635,667 QPS** (18.5 MB) | 474,994 QPS (93.6 MB) | 375,431 QPS (67.9 MB) | **1.34x / 1.69x** |
| **1c** Batch Materialization (500K rows) | **2,056,225 rows/s** (28.3 MB) | 954,430 rows/s (122.1 MB) | 502,480 rows/s (99.9 MB) | **2.15x / 4.09x** |
| **8c** Concurrent WAL Reads (1.6M queries) | **784,667 QPS** (33.3 MB) | 392,941 QPS (762.4 MB) | 409,913 QPS (539.5 MB) | **2.00x / 1.91x** |
| **16c** Concurrent WAL Reads (3.2M queries) | **776,440 QPS** (64.4 MB) | 294,873 QPS (1,521.1 MB) | 291,958 QPS (1,082.2 MB) | **2.63x / 2.66x** |

---

## Quick Install

### Linux / macOS
```bash
curl -fsSL https://raw.githubusercontent.com/cp-lang/cp/main/install.sh | sh
```

### Windows (PowerShell)
```powershell
irm https://raw.githubusercontent.com/cp-lang/cp/main/install.ps1 | iex
```

Verify installation:
```bash
cpc --version
cap --version
```

---

## 5-Minute Quickstart

Run any of the verified examples in [`examples/`](examples/):

```bash
# 1. Hello World + deterministic stack RAII (~init)
cap run examples/01_hello.cp

# 2. Built-in SQLite 3.54.1 (:memory:, zero install)
cap run examples/02_sqlite_memory.cp

# 3. Built-in zero-dependency IANA TimeZone & LEB128
cap run examples/03_tz_and_leb128.cp

# 4. Native C ecosystem packages (@cap/lz4 + @cap/yyjson)
cd examples/04_native_pkg && cap install && cap run src/main.cp
```

---

## Core CLI Commands

| Command | Description |
| :--- | :--- |
| `cap run [-m [--low]] <file.cp>` | Compile & run in Debug mode (`-m` enables modular parallel `.o` cache) |
| `cap build [-m [--low]] [-f\|-s\|-r]` | Build native binary (`-f` ReleaseFast, `-s` ReleaseSmall, `-r` cross-platform matrix) |
| `cap fmt [--check] [path]` | Format `.cp` source files deterministically |
| `cap lint [path]` | Static syntax, borrow-check & 4-gate purity audit |
| `cap test [dir]` / `cap bench <file>` | Run unit tests or ReleaseFast microsecond benchmarks |
| `cap init [app\|agent\|lib]` | Scaffold a new CP project |
| `cap add <pkg> [-d]` / `cap install` | Manage `dep`/`dev`/`bin`/`sys` dependencies & `workspaces` |
| `cap rebuild [--target=<os-arch\|all>]` | Cross-compile C/C++ `bin` static libraries (`-g0` lean mode) |
| `cap pack [--strip] [--npm]` | Pack `.tar.zst` split archives (or `--npm` `.tgz`) |
| `cap publish [--dry-run] [--registry=<url>]` | Publish `.tgz` directly to npm / Verdaccio |

---

## Documentation

- **Full Language Reference**: [`cp_ref.md`](cp_ref.md)
- **Compact Agent / Quick Grammar Spec**: [`cp_agent_ref.md`](cp_agent_ref.md)
- **Built-in Standard Library Index**: [`cp_lib_ref.md`](cp_lib_ref.md)
- **Official `@cap/*` Native Packages**: [`packages/README.md`](packages/README.md)
- **Third-Party License Notices**: [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md)

## License

Dual-licensed under **MIT** ([`LICENSE-MIT`](LICENSE-MIT)) or **Apache-2.0** ([`LICENSE-APACHE`](LICENSE-APACHE)), at your option.
