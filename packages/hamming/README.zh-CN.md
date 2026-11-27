[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hamming

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Hamming distance, byte popcount weight, 64-bit Charikar SimHash, and Hamming(7,4) error-correcting code in pure CP

## 安装与导入

```bash
cap add hamming
```

```cp
import { popcountU32 } from "hamming";
```

## 核心导出 API

- `export fn popcountU32(x: u32): u32;`
- `export fn popcountU8(b: u8): u32;`
- `export fn popcountU64(x: u64): u32;`
- `export fn hammingU64(a: u64, b: u64): u32;`
- `export fn weightBytes(data: &[]u8): u64;`
- `export fn distanceBytes(a: &[]u8, b: &[]u8): i64;`
- `export fn isTokenByte(b: u8): bool;`
- `export fn hashToken64(text: &[]u8, start: usize, end: usize): u64;`
- `export fn simhash64(text: &[]u8): u64;`
- `export fn hamming74Encode(nibble: u8): u8;`
- `export fn hamming74Decode(codeword: u8): Hamming74Decoded;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`hamming@1.0.0`)，许可证：MIT
