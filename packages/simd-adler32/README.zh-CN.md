[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# simd-adler32

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation IEEE CRC-32, Castagnoli CRC-32C, Adler-32, and O(log N) CRC-32 combine engine (Rust crc32fast parity) for CP

## 安装与导入

```bash
cap add simd-adler32
```

```cp
import { crc32Ieee } from "simd-adler32";
```

## 核心导出 API

- `export fn crc32Ieee(data: string): u32;`
- `export fn crc32c(data: string): u32;`
- `export fn adler32(data: string): u32;`
- `export fn combineCrc32Ieee(crc1: u32, crc2: u32, len2: usize): u32;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`simd-adler32@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`crc-catalog@1.0.0`)，许可证：MIT
