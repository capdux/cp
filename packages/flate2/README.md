[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/flate2-core`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:flate2` ([https://github.com/rust-lang/flate2-rs](https://github.com/rust-lang/flate2-rs))（原库名 `flate2` → CP 包名 `@cap/flate2-core`）

Zero-heap RFC 1952 Gzip & RFC 1950 Zlib stored-deflate frame encoder/decoder (`encodeGzipStored`, `decodeGzipStored`, `parseGzipHeader`, `encodeZlibStored`, `decodeZlibStored`, `GzipHeader`) — clean-room CP equivalent of crates.io `flate2`, composing `@cap/crc32fast`, `@cap/adler32`, and `@cap/bytemuck-crate`.
