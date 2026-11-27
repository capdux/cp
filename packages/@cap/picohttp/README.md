[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`picohttpparser`](https://github.com/h2o/picohttpparser)（`MIT`）（原库名 `picohttpparser`，CP 包名 `@cap/picohttp`）。

# @cap/picohttp

Official CP native package wrapping **PicoHTTPParser** (zero-copy HTTP/1.x request/response & chunked transfer decoder).

- **License**: MIT (Upstream: `h2o/picohttpparser`)
- **Prebuilt Binaries**: `prebuilt/linux-x86_64/libpicohttp.a`, `prebuilt/linux-aarch64/libpicohttp.a`
- **Source Fallback**: `c/picohttpparser.c` + `c/picohttpparser.h` (auto-compiled via `zig cc` on other targets)
- **RAII API**: `HttpParser` and `ChunkedDecoder` with deterministic `~init()` cleanup.
