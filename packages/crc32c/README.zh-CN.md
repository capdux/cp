[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# crc32c

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap pure-CP CRC-32C (Castagnoli) checksum with streaming append, GF(2) combine, and LevelDB/RocksDB/GCS masking (crates.io crc32c equivalent)

## 安装与导入

```bash
cap add crc32c
```

```cp
import { c32cByteStep } from "crc32c";
```

## 核心导出 API

- `export fn c32cByteStep(crc_in: u32, b: u8): u32;`
- `export fn c32cGf2MatrixTimes(mat: [32]u32, vec: u32): u32;`
- `export fn c32cGf2MatrixSquare(src: [32]u32): [32]u32;`
- `export fn crc32cAppend(crc: u32, data: []u8): u32;`
- `export fn crc32c(data: []u8): u32;`
- `export fn crc32cStr(s: string): u32;`
- `export fn crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32;`
- `export fn maskCrc32c(crc: u32): u32;`
- `export fn unmaskCrc32c(masked: u32): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`crc32c@1.0.0`)，许可证：Apache-2.0 OR MIT
