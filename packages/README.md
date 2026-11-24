# `@cap/*` Official Native C/C++ Ecosystem Packages

All official `@cap/*` packages follow strict engineering and licensing standards:
- **100% Permissive Licenses** (MIT / Apache-2.0 / BSD / ISC / Zlib / CC0 / OLDAP-2.8 / Public Domain — **zero GPL/AGPL contamination**).
- **4-Gate Purity (0/0/0/0)**: zero `zig {}` blocks, zero `var`, zero `tuple.@N`, zero raw `anyopaque`; deterministic stack-allocated `pub struct` + `~init()` RAII wrappers.
- **Lean Prebuilt Artifacts**: `cap rebuild --target=<os-arch|all> --write-prebuilt` automatically compiles with `-g0 -ffunction-sections -fdata-sections` (70%–84% smaller `.a` archives) and supports `strip --strip-unneeded` for dynamic `.so` libraries.
- **On-Demand Link Tree-Shaking**: packages imported but unused are pruned at AST level and contribute **0 bytes** to output binaries.
- **Dual-Format Distribution**: `cap pack` produces `.tar.zst` (Zstd main + per-platform binary split); `cap publish` produces RFC 1952 Gzip `.tgz` with auto-synthesized `package.json` for official `registry.npmjs.org` / Verdaccio.

## Package Catalog

| Package | Version | License | Paradigm | Purpose |
| :--- | :--- | :--- | :--- | :--- |
| `@cap/picohttp` | `1.0.0` | MIT | C static (`bin`) | Ultra-fast zero-copy HTTP/1.x parser (H2O picohttpparser) |
| `@cap/zstd` | `1.5.7` | BSD-3-Clause | C static (`bin`) | Zstandard high-ratio & high-speed compression |
| `@cap/hdrhistogram` | `0.11.8` | CC0 / BSD-2 | C static (`bin`) | Nanosecond high-dynamic-range latency histogram |
| `@cap/lz4` | `1.10.0` | BSD-2-Clause | C static (`bin`) | GB/s real-time lossless compression (LZ4 & LZ4-HC) |
| `@cap/yyjson` | `0.10.0` | MIT | C static (`bin`) | Fastest C JSON reader/writer (yyjson) |
| `@cap/lmdb` | `0.9.33` | OLDAP-2.8 | C static (`bin`) | Zero-copy mmap embedded KV store (LMDB) |
| `@cap/treesitter` | `0.25.10` | MIT | C static (`bin`) | Incremental AST parsing runtime (tree-sitter) |
| `@cap/yaml` | `0.2.5` | MIT | C static (`bin`) | Industrial YAML 1.1/1.2 parser (libyaml) |
| `@cap/pcre2` | `10.44.0` | BSD-3-Clause | C static + SLJIT (`bin`) | Full Perl-compatible regex with native JIT (backrefs/lookbehind) |
| `@cap/usearch` | `2.26.6` | Apache-2.0 | C++17 static (`bin` + `-lc++`) | Billion-scale HNSW vector search engine (USearch) |
| `@cap/duckdb` | `1.2.1` | MIT | Prebuilt dynamic (`sys`) | Embedded OLAP analytical SQL engine (Parquet/CSV/JSON) |
| `@cap/pdfium` | `131.0.0` | Apache-2.0 | Prebuilt dynamic (`sys`) | Chrome PDFium rendering & text extraction engine |
| `@cap/onnx` | `1.32.0` | MIT | Prebuilt dynamic (`sys`) | Microsoft ONNX Runtime neural/embedding inference engine |
| `@cap/opus` | `1.5.2` | BSD-3-Clause | C static (`bin`) | Low-latency real-time speech/audio codec (WebRTC/Voice Agent) |
| `@cap/llama` | `1.0.12676` | MIT | Prebuilt dynamic (`sys`) | Local LLM inference engine (`llama.cpp` AVX2/AVX512/GPU) |
| `@cap/whisper` | `1.9.6` | MIT | Prebuilt dynamic (`sys`) | Local speech-to-text ASR engine (`whisper.cpp`, 2.6 MB `.so`) |
| `@cap/sodium` | `1.0.20` | ISC | C static (`bin`) | Modern cryptography suite (Ed25519/XChaCha20-Poly1305/BLAKE2b) |
| `@cap/utf8proc` | `2.10.0` | MIT | C static (`bin`) | Unicode 16.0 NFC/NFD/NFKC/Casefold normalization & charwidth |
| `@cap/dr_libs` | `0.13.17` | MIT-0 | C static (`bin`) | Zero-dependency WAV/MP3/FLAC audio codecs (`dr_wav`/`dr_mp3`/`dr_flac`) |

> **Built-in Standard Library Note**: Core runtime essentials such as `cap:sqlite` (embedded SQLite 3.54.1 amalgamation, 2.64M lookups/s), `cap:regex` (Rust `regex-automata`-inspired SIMD + Lazy DFA), `cap:redis`, `cap:sql` (PostgreSQL/MySQL), `cap:http`, `cap:net`, `cap:crypto`, and `cap:image` are built into `lib/` and require **no `cap install`**.

## Quick Commands

```bash
# Install into a project
cap add @cap/yyjson@0.10.0

# Cross-compile lean prebuilt static libraries
cd packages/@cap/lz4
cap rebuild --target=all --write-prebuilt

# Pack split .tar.zst main + platform archives
cap pack --out=dist/

# Preview or publish directly to npm / Verdaccio
cap publish --dry-run
CAP_TOKEN=npm_xxxx cap publish --access=public
```
