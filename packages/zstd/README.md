[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/zstd

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zstd` ([https://github.com/gyscos/zstd-rs](https://github.com/gyscos/zstd-rs))

Official CP native package wrapping **Zstandard (`libzstd`)** single-file amalgamation.

- **License**: BSD-3-Clause (Upstream: `facebook/zstd`)
- **Prebuilt Binaries**: `prebuilt/linux-x86_64/libzstd.a`, `prebuilt/linux-aarch64/libzstd.a`
- **Source Fallback**: `c/zstd.c` + `c/zstd.h` (auto-compiled via `zig cc` on other targets)
- **RAII API**: `Compressor` (`ZSTD_CCtx`) and `Decompressor` (`ZSTD_DCtx`) with deterministic `~init()` cleanup.
