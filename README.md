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
