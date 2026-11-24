# @cap/picohttp

Official CP native package wrapping **PicoHTTPParser** (zero-copy HTTP/1.x request/response & chunked transfer decoder).

- **License**: MIT (Upstream: `h2o/picohttpparser`)
- **Prebuilt Binaries**: `prebuilt/linux-x86_64/libpicohttp.a`, `prebuilt/linux-aarch64/libpicohttp.a`
- **Source Fallback**: `c/picohttpparser.c` + `c/picohttpparser.h` (auto-compiled via `zig cc` on other targets)
- **RAII API**: `HttpParser` and `ChunkedDecoder` with deterministic `~init()` cleanup.
