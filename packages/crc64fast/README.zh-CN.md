[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# crc64fast

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 64-bit CRC checksum engine supporting ECMA-182, CRC-64/XZ, and CRC-64/GO-ISO with 256-entry lookup tables and streaming hasher (Rust crc / crc64fast parity) for CP

## 安装与导入

```bash
cap add crc64fast
```

```cp
import { makeU64 } from "crc64fast";
```

## 核心导出 API

- `export fn makeU64(hi: u32, lo: u32): u64;`
- `export fn crc64Ecma(data: &[]u8): u64;`
- `export fn crc64Xz(data: &[]u8): u64;`
- `export fn crc64Iso(data: &[]u8): u64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`crc64fast@1.0.0`)，许可证：MIT
