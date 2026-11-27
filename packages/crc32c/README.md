[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/crc32c`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:crc32c` ([https://github.com/zowens/crc32c](https://github.com/zowens/crc32c))

Zero-heap pure-CP CRC-32C (Castagnoli, polynomial `0x82F63B78`, RFC 3720 / iSCSI) checksum library — clean-room CP equivalent of crates.io `crc32c`.

## Features

- `CRC32C_POLY_REVERSED` (`0x82F63B78` / `2197175160`) & `CRC32C_MASK_DELTA` (`0xA282EAD8` / `2726488792`)
- `crc32c(data: []u8): u32` — one-shot byte-slice CRC-32C
- `crc32cAppend(crc: u32, data: []u8): u32` — incremental streaming CRC-32C update
- `crc32cStr(s: string): u32` — one-shot string CRC-32C
- `crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32` — GF(2) matrix-squaring CRC-32C combination
- `maskCrc32c(crc: u32): u32` / `unmaskCrc32c(masked: u32): u32` — LevelDB / RocksDB / Snappy / GCS standard CRC-32C masking
- `Crc32cHasher` — stateful zero-heap streaming hasher
