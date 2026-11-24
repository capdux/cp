# @cap/hdrhistogram

Official CP native package wrapping **HdrHistogram_c** for high-dynamic-range latency percentile statistics.

- **License**: Public Domain (CC0-1.0) / BSD-2-Clause (Upstream: `HdrHistogram/HdrHistogram_c`)
- **Prebuilt Binaries**: `prebuilt/linux-x86_64/libhdrhistogram.a`, `prebuilt/linux-aarch64/libhdrhistogram.a`
- **Source Fallback**: `c/hdr_histogram.c` + `c/hdr/hdr_histogram.h` (auto-compiled via `zig cc` on other targets)
- **RAII API**: `Histogram` (`hdr_histogram*`) with deterministic `~init()` cleanup via `hdr_close`.
