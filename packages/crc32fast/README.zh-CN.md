[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# crc32fast

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap IEEE 802.3 CRC-32 streaming Hasher with GF(2) combine, reset, and one-shot hash (crates.io crc32fast equivalent)

## 安装与导入

```bash
cap add crc32fast
```

```cp
import { crcByteStep } from "crc32fast";
```

## 核心导出 API

- `export fn crcByteStep(crc_in: u32, b: u8): u32;`
- `export fn crcGf2MatrixTimes(mat: [32]u32, vec: u32): u32;`
- `export fn crcGf2MatrixSquare(src: [32]u32): [32]u32;`
- `export fn combineCrc32(crc1: u32, crc2: u32, len2: u64): u32;`
- `export fn hashBytes(buf: []u8): u32;`
- `export fn hashStr(s: string): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`crc32fast@1.0.0`)，许可证：MIT OR Apache-2.0
