# Third-Party Notices and Attribution

The **CP Language** compiler (`cpc`), package manager (`cap`), and standard library (`lib/*`) are independently implemented in pure CP, Rust, and Zig.

## Zero-Copyleft Compliance Policy

- All CP standard library modules under `lib/` are clean-room or permissively-derived implementations using **only** permissive open-source licenses (`MIT`, `Apache-2.0`, `BSD-3-Clause`, `ISC`, `Unlicense`, `Zlib`).
- **Zero GPL / LGPL / AGPL**: No copyleft source code, test fixtures, or linked binaries (including GPL, LGPL, or AGPL components such as WebKit/JavaScriptCore) are incorporated into or required by the CP compiler or standard library.
- Local reference repositories placed under `_ref/` during behavioral auditing and test-vector hardening are excluded from version control (`.gitignore`) and are strictly audited to be permissively licensed (`MIT` / `Apache-2.0` / `Unlicense` / `Zlib`).

---

## 1. Core Ecosystem & Compiler Upstream Acknowledgments

| Project | Repository | License | Usage / Relationship | Copyright Notice |
| :--- | :--- | :--- | :--- | :--- |
| **Bun** | <https://github.com/oven-sh/bun> | MIT | Pure-logic runtime/utility modules absorbed and migrated to pure CP (marked with `// Source: Absorbed from /data/dev/bun/src/...` or `// Synced from bun/`). CP strictly excludes Bun's LGPL JavaScriptCore/WebKit components. | Copyright (c) Oven / Jarred Sumner and Bun contributors |
| **SWC** | <https://github.com/swc-project/swc> | Apache-2.0 | Compiler frontend lexer, parser, and AST architectural reference (`_ref/swc`). | Copyright (c) SWC Project contributors |
| **Zig Standard Library & Toolchain** | <https://github.com/ziglang/zig> | MIT | Native backend compilation target, `@import("std")` compatibility, and build toolchain integration. | Copyright (c) Zig contributors |

---

## 2. Standard Library Behavioral Hardening & Test-Suite References (`_ref/`)

The following permissively-licensed Rust reference implementations were audited for RFC/specification edge-case behavior, boundary hardening, and regression test-vector inheritance. Every hardened CP module carries a matching `// Reference-hardened against <owner>/<repo> (<license>)` header comment.

| Project | Repository | License | Hardened CP Module(s) | Copyright Notice |
| :--- | :--- | :--- | :--- | :--- |
| **rust-csv** (`csv-core`) | <https://github.com/BurntSushi/rust-csv> | MIT OR Unlicense | `lib/doc/csvTable.cp` | Copyright (c) 2015 Andrew Gallant |
| **hound** | <https://github.com/ruuda/hound> | Apache-2.0 | `lib/media/audioWav.cp` | Copyright (c) Ruud van Asseldonk |
| **zstd-rs** (`ruzstd`) | <https://github.com/KillingSpark/zstd-rs> | MIT | `lib/pack/zstdDecode.cp` | Copyright (c) 2019 Moritz Borcherding |
| **str0m** | <https://github.com/algesten/str0m> | MIT OR Apache-2.0 | `lib/net/webrtcIce.cp`, `lib/media/rtpJitter.cp`, `lib/net/srtpCodec.cp` | Copyright (c) 2022 Martin Algesten |
| **calamine** | <https://github.com/tafia/calamine> | MIT | `lib/doc/xlsx.cp` | Copyright (c) 2016 Johann Tuffe |
| **image-webp** | <https://github.com/image-rs/image-webp> | MIT OR Apache-2.0 | `lib/image/webp.cp` | Copyright (c) image-rs developers |
| **lopdf** | <https://github.com/J-F-Liu/lopdf> | MIT | `lib/doc/pdfInspector.cp` | Copyright (c) 2016 Junfeng Liu |
| **zune-image** (`zune-jpeg`) | <https://github.com/etemesi254/zune-image> | MIT OR Apache-2.0 OR Zlib | `lib/image/jpeg.cp` | Copyright (c) zune-image developers |
| **image-png** (`png`) | <https://github.com/image-rs/image-png> | MIT OR Apache-2.0 | `lib/image/png.cp` | Copyright (c) image-rs developers |
| **regex** (`regex-automata`) | <https://github.com/rust-lang/regex> | MIT OR Apache-2.0 | `lib/compiler/regexCompiler.cp`, `lib/compiler/regexVm.cp` | Copyright (c) 2014 The Rust Project Developers |
| **RustFFT** | <https://github.com/ejmahler/RustFFT> | MIT OR Apache-2.0 | `lib/media/fftFilter.cp` | Copyright (c) 2016 Elliott Mahler |
| **rust-brotli** | <https://github.com/dropbox/rust-brotli> | BSD-3-Clause AND MIT | `lib/pack/brotli.cp` | Copyright (c) 2016 Dropbox, Inc. |
| **nalgebra** | <https://github.com/dimforge/nalgebra> | Apache-2.0 | `lib/math/matrixDense.cp` | Copyright (c) 2013-2024 Sébastien Crozet / Dimforge |
| **tantivy** | <https://github.com/quickwit-oss/tantivy> | MIT | `lib/vector/fullTextIndex.cp`, `lib/vector/fullTextQuery.cp` | Copyright (c) 2016 Paul Masurel / Quickwit |
| **lsm-tree** (`fjall-rs`) | <https://github.com/fjall-rs/lsm-tree> | MIT OR Apache-2.0 | `lib/db/lsmEngine.cp` | Copyright (c) 2024 fjall-rs |
| **aho-corasick** | <https://github.com/BurntSushi/aho-corasick> | MIT OR Unlicense | `lib/ds/ahoCorasick.cp` | Copyright (c) 2015 Andrew Gallant |
| **webrtc-rs/sctp** | <https://github.com/webrtc-rs/sctp> | MIT OR Apache-2.0 | `lib/net/sctpDataChannel.cp` | Copyright (c) 2021 webrtc-rs developers |
| **rstar** (`georust`) | <https://github.com/georust/rstar> | MIT OR Apache-2.0 | `lib/ds/rtree.cp` | Copyright (c) 2018 The rstar project developers |
| **claxon** (`ruuda`) | <https://github.com/ruuda/claxon> | Apache-2.0 | `lib/media/flac.cp` | Copyright (c) Ruud van Asseldonk |
| **image-gif** (`gif`) | <https://github.com/image-rs/image-gif> | MIT OR Apache-2.0 | `lib/image/gif.cp` | Copyright (c) image-rs developers |
| **lz4_flex** | <https://github.com/PSeitz/lz4_flex> | MIT | `lib/pack/lz4.cp` | Copyright (c) 2020 Pascal Seitz |

---

## 3. Optional `@c` FFI External C Libraries

Where optional low-level `_*.cp` / `_*.zig` FFI bindings interface with external system or vendored C libraries, those libraries remain governed by their respective permissive licenses:

| Library | Repository / Homepage | License | Boundary / Usage |
| :--- | :--- | :--- | :--- |
| **BoringSSL** | <https://boringssl.googlesource.com/boringssl> | ISC / OpenSSL / BSD-style | Optional C FFI cryptographic bindings (`lib/crypto/_boringssl.zig`). Pure-CP crypto primitives do not require BoringSSL. |
| **uSockets** | <https://github.com/uNetworking/uSockets> | Apache-2.0 | Optional native socket event-loop C bindings (`lib/net/_usockets.cp`). |
| **picohttpparser** | <https://github.com/h2o/picohttpparser> | MIT OR Perl | Optional C HTTP/1.x parser bindings (`lib/http/_picohttp.cp`). |
| **zlib** | <https://zlib.net/> | Zlib License | Optional C compression FFI bindings where linked; `lib/pack/` provides pure-CP Deflate/Inflate/Gzip/Zlib/Zstd implementations. |

---

## 4. Permissive License Texts (Summary Reference)

### MIT License

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

### Apache License, Version 2.0

Licensed under the Apache License, Version 2.0 (the "License"); you may not use these components except in compliance with the License. You may obtain a copy of the License at:

<http://www.apache.org/licenses/LICENSE-2.0>

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.
