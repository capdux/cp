[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `zip`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:zip` ([https://github.com/zip-rs/zip2](https://github.com/zip-rs/zip2))（原库名 `zip` → CP 包名 `zip`）

Zero-heap PKZIP Stored archive writer, local file header & EOCD parser, and CRC-32 payload verifier (`writeStoredZipSingle`, `parseLocalHeader`, `findAndParseEocd`, `verifyStoredEntry`, `ZipLocalHeader`, `ZipEocd`) — clean-room CP equivalent of crates.io `zip`, composing `crc32fast` and `bytemuck`.
