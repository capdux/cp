[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/bytesize`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bytesize` ([https://github.com/hyunsik/bytesize](https://github.com/hyunsik/bytesize))

Zero-heap pure-integer byte size utility in pure CP (clean-room counterpart to Rust `bytesize`).

## Features

- **SI & IEC Constants**: `B`, `KB`, `MB`, `GB`, `TB`, `PB` (base 1000) and `KIB`, `MIB`, `GIB`, `TIB`, `PIB` (base 1024).
- **`ByteSize` Value Struct**: `.b()`, `.kb()`, `.mb()`, `.gb()`, `.tb()`, `.pb()`, `.kib()`, `.mib()`, `.gib()`, `.tib()`, `.pib()`, `.asU64()`, `.add()`, `.subSaturating()`, `.displaySi()`, `.displayIec()`.
- **Pure-Integer Parser (`parseByteSize`)**: Case-insensitive parsing with fractional decimals (`"1.5 GiB"`, `"500MB"`, `"2.5 kb"`, `"1024"`) and overflow protection.
- **Zero-Heap Formatting (`formatSi` / `formatIec`)**: Writes human-readable sizes directly into caller-provided `out: []u8`.

## Quick Start

```cp
import { ByteSize, GIB, parseByteSize, formatSi, formatIec } from "@cap/bytesize";

const sz = ByteSize.mib(512);
const total = sz.asU64(); // 536870912

const parsed = parseByteSize("1.5 GiB"); // 1610612736

let buf: [64]u8 = undefined;
const n1 = formatSi(1500000, buf[0..]);  // "1.5 MB"
const n2 = formatIec(1536, buf[0..]);    // "1.5 KiB"
```

## License

Apache-2.0
